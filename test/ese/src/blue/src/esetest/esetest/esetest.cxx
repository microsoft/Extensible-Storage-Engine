// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// ESETest.cpp
//
//  This is a generic library of functions that I have found useful when
// writing test cases outside of MUJET.  This library provides functions
// that take care of standard operations like reporting in results.txt and
// results.log so that they will work well in our School/Pupil environment.
//
// History:
// Rev  MM/DD/YYYY  E-Mail      Comment
// ---  ----------  ------      ---------------------------------------------
// [0]  10/06/1997  SOMEONE      Initial coding of MUSTRESS
// [1]  01/19/1997  SOMEONE     Moved generic code out into ESETest lib
// [2]  05/10/2000  SOMEONE      Added FormatLastError function

#include <windows.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <loadperf.h>
#include <math.h>
#include "ese_common.hxx"
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>
// presently only used for the GetEnvironment() stuff
#include <tchar.h>

#ifdef BUILD_ENV_IS_NT
// Don't pollute the Exchange esetest with any WTT references.
#include <wttlogger.h>

// Dynamically load all the WTT logger goo.
typedef
HRESULT
(WINAPI *PFNWTTLogCreateLogDevice)
        (
        IN  LPWSTR   pwszDevStr,
        OUT LONG*    phDevice
        );

typedef
HRESULT
(WINAPI *PFNWTTLogStartTest)
        (
        IN LONG     hDevice,
        IN LPWSTR   pwszTestName
        );

typedef
HRESULT
(WINAPI *PFNWTTLogEndTest)
        (
        IN  LONG     hDevice,
        IN  LPWSTR   pwszTestName, // Name of the test case
        IN  DWORD    dwResult,     // test result
        IN  LPWSTR   pwszRepro     // repro line
        );


typedef
HRESULT
(WINAPI *PFNWTTLogTrace)
        (
        IN LONG      hDevice,
        IN DWORD     dwTraceLvl,
        ...
        );

typedef
HRESULT
(WINAPI *PFNWTTLogCloseLogDevice)
        (
        IN LONG     hDevice,
        IN LPWSTR   pwszDeviceName
        );

typedef
void
(WINAPI *PFNWTTLogUninit)
        (
        );


#else

typedef void* PFNWTTLogCreateLogDevice;
typedef void* PFNWTTLogStartTest;
typedef void* PFNWTTLogEndTest;
typedef void* PFNWTTLogTrace;
typedef void* PFNWTTLogCloseLogDevice;
typedef void* PFNWTTLogUninit;
#endif 

// From ESE's cc.hxx:
#define fFalse  BOOL( 0 )
#define fTrue   BOOL( !0 )


// Sloppy coding -- the code uses TCHAR, but it isn't actually generic.
C_ASSERT( sizeof( TCHAR ) == sizeof( char ) );

// needed for _open(), etc.
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

//----------------
// Macros
// Define logging file (can be overriden at compile time with -DNO_LOGGING flag)
static char RESULTS_LOG[MAX_PATH] =
#ifdef NO_LOGGING
    "nul:";
#error NO_LOGGING will not be supported (unless someone uses it).
#else
    "results.log"
#endif
;

/*
#define JetGetSystemParameter( instance, sesid, paramid, plParam, sz, cbMax ) \
    In_esetestcxx_useDynLoadJetGetSystemParameter_instead
*/

//----------------
// Prototypes


// This function was copied from loadnarrow.cxx, and was known as BounceJetXXX().
// If that function changes, please update this one as well.
static
JET_ERR
DynLoadJetGetSystemParameter(
    JET_INSTANCE    instance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __out_bcount_opt(cbMax) JET_API_PTR     *plParam,
    __out_bcount_opt(cbMax) JET_PSTR        szParam,
    unsigned long   cbMax
)
;

// This function was copied from loadnarrow.cxx, and was known as BounceJetXXX().
// If that function changes, please update this one as well.
static
JET_ERR
DynLoadJetSetSystemParameter(
    JET_INSTANCE    *pinstance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCSTR      szParam
);

// This function was copied from loadnarrow.cxx, and was known as BounceJetXXX().
// If that function changes, please update this one as well.
static
JET_ERR
DynLoadJetConsumeLogData(
    __in    JET_INSTANCE        instance,
    __in    JET_EMITDATACTX *   pEmitLogDataCtx,
    __in    void *              pvLogData,
    __in    unsigned long       cbLogData,
    __in    JET_GRBIT           grbits
);

static
bool
FEnvironmentVariableSet(
    __in const char* const  szEnvVariable
)
;

static int IEnvironmentVariableSet(__in const char* const   szEnvVariable);


static char RESULTS_TXT[MAX_PATH] = "results.txt";
static const char ASSERT_TXT[] = "assert.txt";
static const char FAIL_STR[] = " ...FAILED" SZNEWLINE;
static const char PASS_STR[] = " ...PASSED" SZNEWLINE;

#define PRIVATE static

// Environment variables
PRIVATE const CHAR* g_szEnvironmentPagesize         = "ESETEST_PAGESIZE";
PRIVATE const CHAR* g_szSystemPibFailureEnvironment     = "ESE_PIB_FAILURES";
PRIVATE const CHAR* g_szEnvironmentTopDown              = "TOPDOWN_ALLOCATION";
PRIVATE const CHAR* g_szEnvironmentSmallConfig          = "ESE_SMALL_CONFIG";
PRIVATE const CHAR* g_szEnvironment2gLogs               = "ESETEST_2GLOGS";
PRIVATE const CHAR* g_szEnvironmentLegacyLogNames       = "ESETEST_LEGACY_LOG_NAMES";   // if 2glogs is speciifed, defaults to new names
PRIVATE const CHAR* g_szEnvironmentUseWttLog            = "ESE_WTT_LOG";
PRIVATE const CHAR* g_szEnvironmentPctWidenApis         = "PCTWIDEAPIS";            //if unicode filepath is specified, this is the perentage of widen APIs called by 
PRIVATE const CHAR* g_szEnvironmentPctCompression       = "PCTCOMPRESSION";             // <0: nothing is changed from what is passed when creating/setting columns; 0-100: defines the percentage of compressed data.
PRIVATE const CHAR* g_szEnvironmentEsetestAssertAction  = "ESETEST_ASSERT_ACTION";      // defines JET_paramAssertAction
PRIVATE const CHAR* g_szEnvironmentEseAssertAction      = "ESE_ASSERT_ACTION";      // defines JET_paramAssertAction
PRIVATE const CHAR* g_szEnvironmentExceptionAction      = "ESE_EXCEPTION_ACTION";   // defines JET_paramExceptionAction
PRIVATE const CHAR* g_szEnvironmentShadowLog            = "ESETEST_SHADOW_LOGS";    // determines if we should set JET_paramEmitLogDataCallback

// Global Variables
PRIVATE bool        g_fLoggingInitialized           = false;
PRIVATE BOOL        gfLogToFile                     = FALSE;    // Indicates whether the logging was enabled to begin with
PRIVATE bool        g_fEnableWritingToResultsTxt    = true;     // Should almost always be true
// const long           kStandardRandMax    = (1L<<30) - 2;
PRIVATE CRITICAL_SECTION    gcsLogging;
PRIVATE BOOL        g_fUsingShadowLog               = FALSE;

#if 0
// Indicates whether any of the Jet calls failed.
BOOL g_fERROR   = FALSE;
#endif

// Eventually this should be accessible via the registry or environment.
// Then the 'volatile' can be removed (I don't want the compiler to optimize
// it away).
// Currently in order to use this, it is necessary to edit it with
// the debugger.
volatile
DWORD       g_fEsetestBreakOnCall   = 0;


//----------------
// Constants

const char*
SzEsetestEseutil()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const char* szEseutil = "eseutil.exe";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const char* szEseutil = "esentutl.exe";
#endif

    return szEseutil;
}


const wchar_t*
WszEsetestEseutil()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const wchar_t* wszEseutil = L"eseutil.exe";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const wchar_t* wszEseutil = L"esentutl.exe";
#endif

    return wszEseutil;
}

//----------------
// Constants

const char*
SzEsetestEseDll()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const char* szEseDll = "ese.dll";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const char* szEseDll = "esent.dll";
#endif

    return szEseDll;
}


const wchar_t*
WszEsetestEseDll()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const wchar_t* wszEseDll = L"ese.dll";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const wchar_t* wszEseDll = L"esent.dll";
#endif

    return wszEseDll;
}

const char*
SzEsetestEsebackDll()
{
    const char* szEsetestEsebackDll = "eseback2.dll";

    return szEsetestEsebackDll;
}


const wchar_t*
WszEsetestEsebackDll()
{
    const wchar_t* wszEsetestEsebackDll = L"eseback2.dll";

    return wszEsetestEsebackDll;
}

const char*
SzEsetestEsebcliDll()
{
    const char* szEsetestEsebcliDll = "esebcli2.dll";

    return szEsetestEsebcliDll;
}


const wchar_t*
WszEsetestEsebcliDll()
{
    const wchar_t* wszEsetestEsebcliDll = L"esebcli2.dll";

    return wszEsetestEsebcliDll;
}

const char*
SzEsetestEseperfDll()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const char* szEseperfDll = "eseperf.dll";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const char* szEseperfDll = "esentprf.dll";
#endif

    return szEseperfDll;
}


const wchar_t*
WszEsetestEseperfDll()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const wchar_t* wszEsePerfDll = L"eseperf.dll";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const wchar_t* wszEsePerfDll = L"esentprf.dll";
#endif

    return wszEsePerfDll;
}

const char*
SzEsetestEseperfIni()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const char* szEseperfIni = "eseperf.ini";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const char* szEseperfIni = "esentprf.ini";
#endif

    return szEseperfIni;
}


const wchar_t*
WszEsetestEseperfIni()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const wchar_t* wszEsePerfIni = L"eseperf.ini";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const wchar_t* wszEsePerfIni = L"esentprf.ini";
#endif

    return wszEsePerfIni;
}

const char*
SzEsetestEseperfHxx()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const char* szEseperfHxx = "eseperf.hxx";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const char* szEseperfHxx = "esentprf.hxx";
#endif

    return szEseperfHxx;
}

const wchar_t*
WszEsetestEseperfHxx()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const wchar_t* wszEsePerfHxx = L"eseperf.hxx";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const wchar_t* wszEsePerfHxx = L"esentprf.hxx";
#endif

    return wszEsePerfHxx;
}

const char*
SzEsetestEseperfDatabase()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const char* szEseperfDatabase = FEsetestFeaturePresent( EseFeatureNewPerfCountersNaming ) ?
                                    "MSExchange Database" : "Database";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const char* szEseperfDatabase = "Database";
#endif

    return szEseperfDatabase;
}


const wchar_t*
WszEsetestEseperfDatabase()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const wchar_t* wszEsePerfDatabase = FEsetestFeaturePresent( EseFeatureNewPerfCountersNaming ) ?
                                        L"MSExchange Database" : L"Database";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const wchar_t* wszEsePerfDatabase = L"Database";
#endif

    return wszEsePerfDatabase;
}

const char*
SzEsetestEseperfDatabaseInstances()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const char* szEseperfDatabaseInstances = FEsetestFeaturePresent( EseFeatureNewPerfCountersNaming ) ?
                                    "MSExchange Database ==> Instances" : "Database ==> Instances";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const char* szEseperfDatabaseInstances = "Database ==> Instances";
#endif

    return szEseperfDatabaseInstances;
}

const wchar_t*
WszEsetestEseperfDatabaseInstances()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const wchar_t* wszEsePerfDatabaseInstances = FEsetestFeaturePresent( EseFeatureNewPerfCountersNaming ) ?
                                        L"MSExchange Database ==> Instances" : L"Database ==> Instances";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const wchar_t* wszEsePerfDatabaseInstances = L"Database ==> Instances";
#endif

    return wszEsePerfDatabaseInstances;
}

const char*
SzEsetestEseperfDatabaseTableClasses()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const char* szEseperfDatabaseTableClasses = FEsetestFeaturePresent( EseFeatureNewPerfCountersNaming ) ?
                                    "MSExchange Database ==> TableClasses" : "Database ==> TableClasses";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const char* szEseperfDatabaseTableClasses = "Database ==> TableClasses";
#endif

    return szEseperfDatabaseTableClasses;
}

const wchar_t*
WszEsetestEseperfDatabaseTableClasses()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const wchar_t* wszEsePerfDatabaseTableClasses = FEsetestFeaturePresent( EseFeatureNewPerfCountersNaming ) ?
                                        L"MSExchange Database ==> TableClasses" : L"Database ==> TableClasses";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const wchar_t* wszEsePerfDatabaseTableClasses = L"Database ==> TableClasses";
#endif

    return wszEsePerfDatabaseTableClasses;
}

const char*
SzEsetestEseperfDatabaseDatabases()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const char* szEseperfDatabaseDatabases = FEsetestFeaturePresent( EseFeatureNewPerfCountersNaming ) ?
                                    "MSExchange Database ==> Databases" : "Database ==> Databases";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const char* szEseperfDatabaseDatabases = "Database ==> Databases";
#endif

    return szEseperfDatabaseDatabases;
}

const wchar_t*
WszEsetestEseperfDatabaseDatabases()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const wchar_t* wszEsePerfDatabaseDatabases = FEsetestFeaturePresent( EseFeatureNewPerfCountersNaming ) ?
                                        L"MSExchange Database ==> Databases" : L"Database ==> Databases";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const wchar_t* wszEsePerfDatabaseDatabases = L"Database ==> Databases";
#endif

    return wszEsePerfDatabaseDatabases;
}

const char*
SzEsetestEseEventSource()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const char* szEsetestEseEventSource = "ESE";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const char* szEsetestEseEventSource = "ESENT";
#endif

    return szEsetestEseEventSource;
}

const wchar_t*
WszEsetestEseEventSource()
{
#ifdef ESE_FLAVOUR_IS_ESE
    const wchar_t* wszEsetestEseEventSource = L"ESE";
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
    const wchar_t* wszEsetestEseEventSource = L"ESENT";
#endif

    return wszEsetestEseEventSource;
}

const char*
SzEsetestEsebackEventSource()
{
    const char* szEsetestEsebackEventSource = "ESE BACKUP";

    return szEsetestEsebackEventSource;
}

const wchar_t*
WszEsetestEsebackEventSource()
{
    const wchar_t* wszEsetestEsebackEventSource = L"ESE BACKUP";

    return wszEsetestEsebackEventSource;
}


//-----------------------
// Naming

const char*
SzEsetestGetStartDotLog()
{
    JET_API_PTR ulFileExt;
    DynLoadJetGetSystemParameter( JET_instanceNil, JET_sesidNil, JET_paramLegacyFileNames, &ulFileExt, NULL, sizeof( ulFileExt ) );
    const char* szEsetestGetStartDotLog = ( ulFileExt & JET_bitESE98FileNames ) ? "*.log" : "*.jtx";

    return szEsetestGetStartDotLog;
}

const wchar_t*
WszEsetestGetStartDotLog()
{
    JET_API_PTR ulFileExt;
    DynLoadJetGetSystemParameter( JET_instanceNil, JET_sesidNil, JET_paramLegacyFileNames, &ulFileExt, NULL, sizeof( ulFileExt ) );
    const wchar_t* wszEsetestGetStartDotLog = ( ulFileExt & JET_bitESE98FileNames ) ? L"*.log" : L"*.jtx";

    return wszEsetestGetStartDotLog;
}

const char*
SzEsetestGetStartDotChk()
{
    JET_API_PTR ulFileExt;
    DynLoadJetGetSystemParameter( JET_instanceNil, JET_sesidNil, JET_paramLegacyFileNames, &ulFileExt, NULL, sizeof( ulFileExt ) );
    const char* szEsetestGetStartDotChk = ( ulFileExt & JET_bitESE98FileNames ) ? "*.chk" : "*.jcp";

    return szEsetestGetStartDotChk;
}

const wchar_t*
WszEsetestGetStartDotChk()
{
    JET_API_PTR ulFileExt;
    DynLoadJetGetSystemParameter( JET_instanceNil, JET_sesidNil, JET_paramLegacyFileNames, &ulFileExt, NULL, sizeof( ulFileExt ) );
    const wchar_t* wszEsetestGetStartDotChk = ( ulFileExt & JET_bitESE98FileNames ) ? L"*.chk" : L"*.jcp";

    return wszEsetestGetStartDotChk;
}

const char*
SzEsetestGetChkptName(
    __out_ecount( cch ) char*   szBuffer,
    __in size_t                 cch
)
{
    char szBase[ JET_BASE_NAME_LENGTH + 1 ];
    JET_API_PTR ulFileExt;

    DynLoadJetGetSystemParameter( JET_instanceNil, JET_sesidNil, JET_paramBaseName, NULL, szBase, _countof( szBase ) );
    DynLoadJetGetSystemParameter( JET_instanceNil, JET_sesidNil, JET_paramLegacyFileNames, &ulFileExt, NULL, sizeof( ulFileExt ) );
    StringCchPrintfA( szBuffer, cch, "%s.%s", szBase, ( ulFileExt & JET_bitESE98FileNames ) ? "chk" : "jcp" );

    return szBuffer;
}

const wchar_t*
WszEsetestGetChkptName(
    __out_ecount( cch ) wchar_t*    wszBuffer,
    __in size_t                     cch
)
{
    char* szBuffer = new char[ cch ];
    SzEsetestGetChkptName( szBuffer, cch );
    MultiByteToWideChar( CP_ACP, 0, szBuffer, -1, wszBuffer, cch );

    delete []szBuffer;
    return wszBuffer;
}

char*
SzEsetestGetLogNameTemp(
    __out_ecount( cch ) char*   szBuffer,
    __in size_t                 cch
)
{
    char szBase[ JET_BASE_NAME_LENGTH + 1 ];
    JET_API_PTR ulFileExt;

    DynLoadJetGetSystemParameter( JET_instanceNil, JET_sesidNil, JET_paramBaseName, NULL, szBase, _countof( szBase ) );
    DynLoadJetGetSystemParameter( JET_instanceNil, JET_sesidNil, JET_paramLegacyFileNames, &ulFileExt, NULL, sizeof( ulFileExt ) );
    StringCchPrintfA( szBuffer, cch, "%stmp.%s", szBase, ( ulFileExt & JET_bitESE98FileNames ) ? "log" : "jtx" );

    return szBuffer;
}

wchar_t*
WszEsetestGetLogNameTemp(
    __out_ecount( cch ) wchar_t*    wszBuffer,
    __in size_t                     cch
)
{
    char* szBuffer = new char[ cch ];
    SzEsetestGetLogNameTemp( szBuffer, cch );
    MultiByteToWideChar( CP_ACP, 0, szBuffer, -1, wszBuffer, cch );

    delete []szBuffer;
    return wszBuffer;
}

char*
SzEsetestGetLogNameLast(
    __out_ecount( cch ) char*   szBuffer,
    __in size_t                 cch
)
{
    char szBase[ JET_BASE_NAME_LENGTH + 1 ];
    JET_API_PTR ulFileExt;

    DynLoadJetGetSystemParameter( JET_instanceNil, JET_sesidNil, JET_paramBaseName, NULL, szBase, _countof( szBase ) );
    DynLoadJetGetSystemParameter( JET_instanceNil, JET_sesidNil, JET_paramLegacyFileNames, &ulFileExt, NULL, sizeof( ulFileExt ) );
    StringCchPrintfA( szBuffer, cch, "%s.%s", szBase, ( ulFileExt & JET_bitESE98FileNames ) ? "log" : "jtx" );

    return szBuffer;
}

wchar_t*
WszEsetestGetLogNameLast(
    __out_ecount( cch ) wchar_t*    wszBuffer,
    __in size_t                     cch
)
{
    char* szBuffer = new char[ cch ];
    SzEsetestGetLogNameLast( szBuffer, cch );
    MultiByteToWideChar( CP_ACP, 0, szBuffer, -1, wszBuffer, cch );

    delete []szBuffer;
    return wszBuffer;
}

char*
SzEsetestGetLogNameFromGen(
    __out_ecount( cch ) char*   szBuffer,
    __in size_t                 cch,
    __in unsigned long          ulGen
)
{
    char szBase[ JET_BASE_NAME_LENGTH + 1 ];
    JET_API_PTR ulFileExt;

    DynLoadJetGetSystemParameter( JET_instanceNil, JET_sesidNil, JET_paramBaseName, NULL, szBase, _countof( szBase ) );
    DynLoadJetGetSystemParameter( JET_instanceNil, JET_sesidNil, JET_paramLegacyFileNames, &ulFileExt, NULL, sizeof( ulFileExt ) );
    StringCchPrintfA( szBuffer, cch, "%s%0*X.%s",
                        szBase,
                        ( ulFileExt & JET_bitEightDotThreeSoftCompat ) ? ( ulGen < 0x100000 ? 5 : 8 ) : 8,
                        ulGen,
                        ( ulFileExt & JET_bitESE98FileNames ) ? "log" : "jtx" );

    return szBuffer;
}

wchar_t*
WszEsetestGetLogNameFromGen(
    __out_ecount( cch ) wchar_t*    wszBuffer,
    __in size_t                     cch,
    __in unsigned long              ulGen
)
{
    char* szBuffer = new char[ cch ];
    SzEsetestGetLogNameFromGen( szBuffer, cch, ulGen );
    MultiByteToWideChar( CP_ACP, 0, szBuffer, -1, wszBuffer, cch );

    delete []szBuffer;
    return wszBuffer;
}

#define STRINGIZE_(arg) #arg
#define STRINGIZE(arg) STRINGIZE_(arg)
long
EsetestGetLogGenFromNameA(
    __in const char* szLogName
)
{
    char szLogNameWithoutExt[MAX_PATH];
    char szBase[JET_BASE_NAME_LENGTH + 1];
    char format[] = "%" STRINGIZE(JET_BASE_NAME_LENGTH) "s%lX";
    long lGen = -1;
    _splitpath_s(szLogName, NULL, 0, NULL, 0, szLogNameWithoutExt, _countof(szLogNameWithoutExt), NULL, 0);
    sscanf_s(szLogNameWithoutExt, format, szBase, _countof(szBase), &lGen);
    if(lGen == -1)
    {
        char szJetTmpLogNameWithoutExt[JET_BASE_NAME_LENGTH + 4];
        strcpy_s(szJetTmpLogNameWithoutExt, _countof(szJetTmpLogNameWithoutExt), szBase );
        strcat_s(szJetTmpLogNameWithoutExt, _countof(szJetTmpLogNameWithoutExt), "tmp" );
        if(6 == strlen(szJetTmpLogNameWithoutExt) &&    // first clause protects from malformed base name
            0 == _stricmp(szJetTmpLogNameWithoutExt, szLogNameWithoutExt))
        {
            lGen = 0;
        }
    }
    return lGen;
}

#undef STRINGIZE_
#define STRINGIZE_(arg) L#arg
long
EsetestGetLogGenFromNameW(
    __in const wchar_t* wszLogName
)
{
    wchar_t wszLogNameWithoutExt[MAX_PATH];
    wchar_t wszBase[JET_BASE_NAME_LENGTH + 1];
    wchar_t format[] = L"%" STRINGIZE(JET_BASE_NAME_LENGTH) L"s%lX";
    long lGen = -1;
    _wsplitpath_s(wszLogName, NULL, 0, NULL, 0, wszLogNameWithoutExt, _countof(wszLogNameWithoutExt), NULL, 0);
    swscanf_s(wszLogNameWithoutExt, format, wszBase, _countof(wszBase), &lGen);
    if(lGen == -1)
    {
        wchar_t wszJetTmpLogNameWithoutExt[JET_BASE_NAME_LENGTH + 4];
        wcscpy_s(wszJetTmpLogNameWithoutExt, _countof(wszJetTmpLogNameWithoutExt), wszBase );
        wcscat_s(wszJetTmpLogNameWithoutExt, _countof(wszJetTmpLogNameWithoutExt), L"tmp" );
        if(6 == wcslen(wszJetTmpLogNameWithoutExt) &&   // first clause protects from malformed base name
            0 == _wcsicmp(wszJetTmpLogNameWithoutExt, wszLogNameWithoutExt))
        {
            lGen = 0;
        }
    }
    return lGen;
}
#undef STRINGIZE
#undef STRINGIZE_

const char*
SzEsetestPerfSummaryXml()
{
    const char* szReturn = "PerfSummary.xml";

    return szReturn;
}

const wchar_t*
WszEsetestPerfSummaryXml()
{
    const wchar_t* wszReturn = L"PerfSummary.xml";

    return wszReturn;
}


//-----------------------
// Configuration

class EsetestConfig
{
    public:
        bool    m_fCallSystemParametersBeforeInit;
        bool    m_fVerboseInternalLogging;

        bool    m_fScrubbingSupported;
        bool    m_fMultiInstanceSupported;
        bool    m_fPrepInsertCopyDeleteOriginalSupported;
        bool    m_fParamCacheSizeMaxSupported;
        bool    m_fZeroDb;
        bool    m_fBackupSupported;
        bool    m_fMultiInstanceBackupSupported;
        bool    m_fRunningStress;

        // WTT-related logging stuff
        bool    m_fWttLog;
        HMODULE m_hmodWttLog;
        LONG    m_hdevWttLog;
        PFNWTTLogCreateLogDevice    pfnWTTLogCreateLogDevice;
        PFNWTTLogStartTest          pfnWTTLogStartTest;
        PFNWTTLogEndTest            pfnWTTLogEndTest;
        PFNWTTLogTrace              pfnWTTLogTrace;
        PFNWTTLogCloseLogDevice     pfnWTTLogCloseLogDevice;
        PFNWTTLogUninit             pfnWTTLogUninit;

        ESETEST_OUTPUT_LEVEL    m_OutputLevel;

        size_t          m_pctWideApis;
        int             m_pctCompression;
        UINT        m_esetestAssertAction;

        HMODULE m_hmodEseDll;
        CRITICAL_SECTION    m_csEseDll;
        HMODULE m_hmodEsebackDll;
        CRITICAL_SECTION    m_csEsebackDll;

        EsetestConfig()
        {
            m_fCallSystemParametersBeforeInit = true;
            m_fVerboseInternalLogging = false;


            // ESE97
            m_fScrubbingSupported = true;

            // ESE98+
            // ESENT from XP forward
            m_fMultiInstanceSupported = true;

            m_fPrepInsertCopyDeleteOriginalSupported = true;

            // ESE97+
            m_fParamCacheSizeMaxSupported = true;

            m_fBackupSupported = true;

            m_fMultiInstanceBackupSupported = m_fMultiInstanceSupported && m_fBackupSupported;

            m_fRunningStress = false;


            m_OutputLevel = OUTPUT_INFO;

            m_pctWideApis           = 50;
            m_pctCompression        = 50;
            m_esetestAssertAction   = JET_AssertFailFast;

            m_fWttLog = false;
            m_hmodWttLog = NULL;
            m_hdevWttLog                = 0;
            pfnWTTLogCreateLogDevice    = NULL;
            pfnWTTLogStartTest          = NULL;
            pfnWTTLogEndTest            = NULL;
            pfnWTTLogTrace              = NULL;
            pfnWTTLogCloseLogDevice     = NULL;
            pfnWTTLogUninit             = NULL;

            m_hmodEseDll = NULL;
            InitializeCriticalSection( &m_csEseDll );

            m_hmodEsebackDll = NULL;
            InitializeCriticalSection( &m_csEsebackDll );

            // HACK:
            // This does not have anything to do with EsetestConfig.
            InitializeCriticalSection( &gcsLogging );

        }

        ~EsetestConfig()
        {
            if ( m_hmodWttLog )
            {
                FreeLibrary( m_hmodWttLog );
                m_hmodWttLog = NULL;
            }

            if ( m_hmodEseDll )
            {
                FreeLibrary( m_hmodEseDll );
                m_hmodEseDll = NULL;
            }

            if ( m_hmodEsebackDll )
            {
                FreeLibrary( m_hmodEsebackDll );
                m_hmodEsebackDll = NULL;
            }

            // I assume that in order to be called by the destructor, they must have been initialized properly.
            DeleteCriticalSection( &m_csEseDll );
            DeleteCriticalSection( &m_csEsebackDll );
            DeleteCriticalSection( &gcsLogging );
        }
}
;

///////////////////////////////////////////////////////////////
// IRandom
////////////////////////////////////////////////////////////////

// This is a copy/paste of part of the random number generator.
// The reason for this is that many tests rely on seeding the random generator
// with a known seed, insert data and then re-seed to that value and search for
// the data. This may be aliased by internal functions in esetest.dll that also
// use the random generator. Ex: PCTWIDEAPIS and PCTCOMPRESSION.

// This is Thread-local
_declspec( thread ) static long gIStandardRandSeed = 1;

long IGetStandardSeed( void )
{
    return gIStandardRandSeed;
}

void ISeedStandardRand(long seed)
{
    if ( seed != 0 )
        gIStandardRandSeed = seed;
    else
        gIStandardRandSeed = 1;
}

long IRand( void )
{
    return StandardRand( gIStandardRandSeed );
}

long IRandRange( long min, long max )
{
    // Shouldn't happen, but just in case...
    if ( max < min ){
        max = min;
    }
    return( min + ( ( unsigned )IRand() ) % ( max - min + 1 ) );
}

__int64 IRand64( void )
{
    return ( ( ( ( __int64 )IRand() ) << ( 8 * sizeof( long ) ) ) | ( ( __int64 )IRand() ) );
}

__int64 IRand64Range( __int64 min, __int64 max )
{
    // Shouldn't happen, but just in case...
    if ( max < min ){
        max = min;
    }
    return( min + ( ( unsigned __int64 )IRand() ) % ( max - min + 1 ) );
}


EsetestConfig g_esetestconfig;

int IEsetestGetPageSize()
{
    JET_API_PTR cbPageSize;
    JET_ERR err = DynLoadJetGetSystemParameter(JET_instanceNil, JET_sesidNil, JET_paramDatabasePageSize, &cbPageSize, NULL, sizeof(cbPageSize));
    if ( err < JET_errSuccess )
    {
        tprintf( __FUNCTION__ "(): Failed to get page size, ErrorCode: %ld." CRLF, err);
        return -1;
    }

    return (int) cbPageSize;
}

bool
FEsetestCallSystemParametersBeforeInit()
{
    return g_esetestconfig.m_fCallSystemParametersBeforeInit;
}

void
EsetestSetCallSystemParametersBeforeInit(
    bool    fCallSystemParameters
    )
{
    g_esetestconfig.m_fCallSystemParametersBeforeInit = fCallSystemParameters;
}

void
EsetestSetVerboseInternalLogging(
    bool    fVerboseInternalLogging
)
{
    g_esetestconfig.m_fVerboseInternalLogging = fVerboseInternalLogging;
}

void
EsetestSetRunningStress(
    __in const bool fRunningStress
)
{
    g_esetestconfig.m_fRunningStress = fRunningStress;
}

PRIVATE
bool
FEsetestVerboseInternalLogging()
{
    return g_esetestconfig.m_fVerboseInternalLogging;
}



bool
FEsetestScrubbingSupported()
{
    return g_esetestconfig.m_fScrubbingSupported;
}

bool
FEsetestSlvSupported()
{
    return false;
}

bool
FEsetestMultiInstanceSupported()
{
    return g_esetestconfig.m_fMultiInstanceSupported;
}

bool
FEsetestPrepInsertCopyDeleteOriginalSupported()
{
    return g_esetestconfig.m_fPrepInsertCopyDeleteOriginalSupported;
}

bool
FEsetestParamCacheSizeMaxSupported()
{
    return g_esetestconfig.m_fParamCacheSizeMaxSupported;
}

bool
FEsetestBackupSupported()
{
    return g_esetestconfig.m_fBackupSupported;
}

bool
FEsetestMultiInstanceBackupSupported()
{
    return g_esetestconfig.m_fMultiInstanceBackupSupported;
}

bool
FEsetestLongKeysSupported()
{
    // E12 or Longhorn
    // EXCHANGE12_CODEMERGE: Hasn't been done yet.
    if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 0, 0 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 0, 0, 0 ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool
FEsetestTracingSupported()
{
    // E12 or Longhorn
    // EXCHANGE12_CODEMERGE: Hasn't been done yet.
    if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 0, 0 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 0, 0, 0 ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool
FEsetestFeaturePresent(
    __in const EseFeaturePresent    feature
)
{
    bool    fRet = false;

    // TODO: table-based.
    switch ( feature )
    {
        case EseFeatureEightK:
            fRet = (IEsetestGetPageSize() == 8*1024);
            break;
        case EseFeatureMultiInstance:
            fRet = FEsetestMultiInstanceSupported();
            break;
        case EseFeaturePrepInsertCopyDeleteOriginal:
            fRet = FEsetestPrepInsertCopyDeleteOriginalSupported();
            break;
        case EseFeatureParamCacheSizeMax:
            fRet = FEsetestParamCacheSizeMaxSupported();
            break;
        case EseFeatureBackup:
            fRet = FEsetestBackupSupported();
            break;
        case EseFeatureMultiInstanceBackup:
            fRet = FEsetestMultiInstanceBackupSupported();
            break;
        case EseFeatureLongKeys:
            fRet = FEsetestLongKeysSupported();
            break;
        case EseFeatureTracing:
            fRet = FEsetestTracingSupported();
            break;
        case EseFeatureTempTableTTForwardOnly:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 0, 0 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 5, 2, 0, 0 ) )
            {
                fRet = true;
            }
            break;
        case EseFeatureJetInit3:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 0, 0 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 0, 0, 0 ) )
            {
                fRet = true;
            }
            break;
        case EseFeature2GLogs:
            // Checked into vbl_srv_esent on June 17 (changelist 80063), but unsure when it will make it to winmain.
            // NOTE: It's a beta2 feature, so it will make it into winmain at circa build 5215. But for now let's use
            // an environment variable override.
            if ( FEnvironmentVariableSet( g_szEnvironment2gLogs ) )
            {
                fRet = true;
            }
            else if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 0, 352 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 0, 0, 5250 ) )
            {
                fRet = true;
            }
            break;
        case EseFeatureApisExportedWithA:
            // this is part of Unicode file path feature. let us make it avaiable for Vista/LH ESENT and Ex12 ESE
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 0, 0 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 0, 0, 0 ) )
            {
                fRet = true;
            }
            break;


        case EseFeatureVSSIncrementalBackup:
        case EseFeatureVSSDifferentialBackup:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 0, 0 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 0, 0, 5048 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureVSSParallelBackups:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 0, 535 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6470 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureLostLogResilience:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 0, 462 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6470 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureCachePercPinnedPerfCtr:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 197 ) && !FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 324 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureParamCheckpointTooDeep:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 0, 634 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6470 ) )
            {
                fRet = true;
            }
            break;
        
        case EseFeatureParamAggressiveLogRollover:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 0, 685 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6470 ) )
            {
                fRet = true;
            }
            break;


        case EseFeatureParamLogRollover:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 0, 685 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6470 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureLogFilesGeneratedPerfCounter:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 1, 0, 61 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6470 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureNewPerfCountersNaming:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 1, 0, 103 ) )
            {
                fRet = true;
            }
            break;
            
        case EseFeatureEsebackAbort:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 1, 75 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6470 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureCopyDiffBkpHeader:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 1, 101 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6470 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureIncrementalReseedApis:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 119 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6470 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureSpaceDiscoveryApis:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 135 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6470 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureSpaceHintsApis:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 143 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6470 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureLargePageSize:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 153) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6470 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureOLD2:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 154) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6470 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureDBM:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 346) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6735 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureSkippableAsserts:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 147) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6470 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureLVCompression:
            if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 244) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6549 ) )
            {
                fRet = true;
            }
            break;

        case EseFeatureWriteCoalescing:
            if( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 274) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6549 ) )
                fRet = true;
            break;

        case EseFeatureReadCoalescing: // This counter has never been added.
            if( FEsetestVerifyVersion( EsetestEseFlavourEse, 99, 999, 999, 999) )
                fRet = true;
            break;

        case EseFeatureSeparateIOQueuesCtrs:
            if( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 429) )
                fRet = true;
            break;

        case EseFeatureChksumInRecov:
            if( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 299) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6735 ) )
                fRet = true;
            break;

        case EseFeatureParamMaxRandomIOSizeDeprecated:
            if( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 351 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 6735 ) )
                fRet = true;
            break;

        case EseFeatureCachePriority:
            if( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 550 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 7800 ) )
                fRet = true;
            break;

        case EseFeatureBFPerfCtrsRefactoring:
            if( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 0, 0, 565 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 7800 ) )
                fRet = true;
            break;

        case EseFeatureHyperCacheDBA:
            if( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 1, 0, 144 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 7800 ) )
                fRet = true;
            break;

        case EseFeatureLogGenPerfCtrsInRecovery:
            if( FEsetestVerifyVersion( EsetestEseFlavourEse, 14, 1, 0, 20 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 7800 ) )
                fRet = true;
            break;

        case EseFeatureLogRewrite:
            if( FEsetestVerifyVersion( EsetestEseFlavourEse, 15, 0, 0, 102 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 1, 0, 7800 ) )    //  ASC_FUTURE: fix esent version
                fRet = true;
            break;

        case EseFeatureDatabaseShrink:
            if( FEsetestVerifyVersion( EsetestEseFlavourEse, 15, 0, 0, 591 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 2, 0, 9268 ) )    //  MAC_FUTURE: fix esent version
                fRet = true;
            break;

        case EseFeatureScrubbingOnOff:
            if( FEsetestVerifyVersion( EsetestEseFlavourEse, 15, 0, 0, 901 ) || FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 2, 0, 9268 ) )    //  MAC_FUTURE: fix esent version
                fRet = true;
            break;

        case EseFeatureMax:
            __fallthrough;
        default:
            AssertM( 0 && "Unknown ESE feature!" );
    }

    return fRet;
}



size_t
EsetestColtypMax()
{
    JET_ERR         err;
    JET_API_PTR     coltypmaxFetched        = 0;
    bool            fFetchShouldSucceed = false;
    static size_t   coltypmax           = 0;
    static bool     fFetched            = false;

    if ( 0 == coltypmax )
    {
        // premature optimization: minimize the calls to FEsetestVerifyVersion().
        // Note that it is NOT properly synchronized, but that's not a problem here.
        if ( !fFetched )
        {
#ifdef ESE_FLAVOUR_IS_ESE
            const size_t    coltypmaxDefault = JET_coltypSLV + 1;
#endif
#ifdef ESE_FLAVOUR_IS_ESENT
            const size_t    coltypmaxDefault = JET_coltypSLV;
#endif

            // Pre-Longhorn would assert/crash on JET_instanceNil
            err = DynLoadJetGetSystemParameter( 0, 0, JET_paramMaxColtyp, &coltypmaxFetched, NULL, 0 );

            if ( FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 0, 0, 0 ) )
            {
                //fFetchShouldSucceed = true;
                fFetchShouldSucceed = false;
            }
            else if ( FEsetestVerifyVersion( EsetestEseFlavourEse, 8, 0, 0, 0 ) )
            {
                // EXCHANGE12_CODEMERGE: Exchange 12 _will_ support fetching JET_paramMaxColtyp, but that modification has
                // not been integraged yet.
                //
                // fFetchShouldSucceed = true;
                fFetchShouldSucceed = false;
            }

            if ( fFetchShouldSucceed )
            {
                // 2004.12.18 Re-enable the asserts at some later point in time.
                // EXCHANGE12_CODEMERGE: AssertM( JET_errSuccess == err );
            }
            else
            {
                // 2004.12.18 Re-enable the asserts at some later point in time.
                // EXCHANGE12_CODEMERGE: AssertM( JET_errInvalidParameter == err );
            }

            if ( fFetchShouldSucceed && JET_errSuccess == err )
            {
                coltypmax = coltypmaxFetched;

                // This assert will fail if any coltyps were added after JET_paramMaxColtyp was implemented.
                AssertM( coltypmaxFetched == JET_coltypMax );
            }
            else
            {
                coltypmax = coltypmaxDefault;
            }
            fFetched = true;
        }

        AssertM( coltypmax > JET_coltypLongText );
        AssertM( coltypmax >= JET_coltypSLV );
        AssertM( coltypmax <= JET_coltypMax );

    }
    AssertM( 0 != coltypmax );

    return coltypmax;
}

// Longhorn changed the size of JET_ccolKeyMost
size_t
EsetestCColKeyMost()
{
    static size_t   ccolKeyMost = 0;
    static bool     fFetched    = false;

    // premature optimization: minimize the calls to FEsetestVerifyVersion().
    // Note that it is NOT properly synchronized, but that's not a problem here.
    if ( !fFetched )
    {
        // EXCHANGE12_CODEMERGE:
        if ( FEsetestVerifyVersion( EsetestEseFlavourEsent, 6, 0, 0, 0 )
            || FEsetestVerifyVersion( EsetestEseFlavourEse, 9, 0, 0, 0 ) )
        {
            ccolKeyMost = JET_ccolKeyMost;
        }
        else
        {
            ccolKeyMost = 12;
        }
        fFetched = true;
    }

    AssertM( ccolKeyMost >= 12 );

    return ccolKeyMost;
}

EsetestEseFlavour
EsetestGetEseFlavour()
{
#ifdef ESE_FLAVOUR_IS_ESE
    return EsetestEseFlavourEse;
#elif defined( ESE_FLAVOUR_IS_ESENT )
    return EsetestEseFlavourEsent;
#else
#  error uh-oh
#endif
}

bool
FEsetestValidVersion(
    __in EsetestEseVersion  eseversion
)
{
    // TODO:
    return true;
/*
    bool                fRet    = false;
    const EsetestEseVersion eseversionnobuild   = static_cast< EsetestEseVersion >( eseversion & ~0xffff );

    for ( size_t i = 0; i < sizeof2( rgKnownVersions ); ++i )
    {
        if ( eseversionnobuild == rgKnownVersions[ i ] )
        {
            fRet = true;
            break;
        }
    }

    // If it's not found, it's a new build!
    AssertM( fRet );
    return fRet;
*/
}

/*
JET_ERR
EsetestGetEseVersion(
    __out   EsetestEseVersion*  peseversion
)
{
    JET_ERR     err     = JET_errSuccess;

    EsetestEseVersion       eseversion;
    EsetestEseFlavour       flavour     = EsetestEseFlavourNil;
    UINT                    verMajor    = 0;
    UINT                    verMinor    = 0;
    UINT                    verSp       = 0;
    UINT                    verBuild    = 0;

    if ( NULL == peseversion )
    {
        AssertM( 0 && "bad param" );
        err = JET_errInvalidParameter;
        goto Cleanup;
    }
    *peseversion = 0;

    Call( EsetestGetEseVersionParts( &flavour, &verMajor, &verMinor, &verSp, &verBuild ) );
    AssertM( EsetestEseFlavourNil != flavour );

    eseversion = static_cast< EsetestEseVersion >( EsetestEseVersionFromParts( flavour, verMajor, verMinor, verSp, verBuild ) );
    AssertM( FEsetestValidVersion( eseversion ) );


Cleanup:
    return err;
}
*/
struct EseVersionBuildToRelease {
    UINT    build;
    UINT    major;
    UINT    minor;
    UINT    sp;
}
;

// KB 158530
// Build numbers and release dates for Exchange
PRIVATE
EseVersionBuildToRelease
rgeseversionbuildtoreleaseEse[] = {
    { 4417, 6, 0, 0 },  // Pt RTM
    { 4712, 6, 0, 1 },  // Pt SP1
    { 5762, 6, 0, 2 },  // Pt SP2
    { 6249, 6, 0, 3 },  // Pt SP3

    { 6944, 6, 5, 0 },  // Ti RTM
    { 7226, 6, 5, 0 },  // Ti SP1
};

PRIVATE
EseVersionBuildToRelease
rgeseversionbuildtoreleaseEsent[] = {
//    { 3939, 5, 0, 0 },  // 2000 (built out of Exchange)
    { 2600, 5, 1, 0 },  // XP
    { 3790, 5, 2, 0 },  // 2003
};


JET_ERR
EsetestFillInSpAndBuild(
    __out   EsetestEseFlavour*  pflavour,
    __in    UINT                fileversionHighMajor,
    __in    UINT                fileversionHighMinor,
    __in    UINT                fileversionLow,
    __out   PUINT               pverSp,
    __out   PUINT               pverBuild
)
{
    JET_ERR     err     = JET_errSuccess;
    const UINT  buildno = HIWORD( fileversionLow );
    const UINT  spbuildno   = LOWORD( fileversionLow );
    size_t      i;

    switch( *pflavour )
    {
    case EsetestEseFlavourEse:
        for ( i = 0; i < ARRAYSIZE( rgeseversionbuildtoreleaseEse ); ++i )
        {
            *pverSp = 0;
            *pverBuild = buildno;
            const EseVersionBuildToRelease* pver = &rgeseversionbuildtoreleaseEse[ i ];
            if ( buildno == pver->build )
            {
                // Exchange does not use the spbuildno. Instead they produce new builds.
                // e.g. PtRtm is 6.0.4417.5, but PtSp3 is 6.0.6249.0
                *pverSp = pver->sp;
                AssertM( 0 != buildno );
                *pverBuild = buildno;
            }
        }
        break;
    case EsetestEseFlavourEsent:
        *pverSp = 0;
        *pverBuild = buildno;
        if ( spbuildno != 0 )
        {           
            if ( fileversionHighMajor < 6 ){
                // For SPs, NT will increase the spbuildno
                // e.g. XpRtm is 5.1.2600.0, and XpSp2 is 5.1.2600.2180
                *pverBuild = spbuildno;
                *pverSp = ( spbuildno / 1000 );
            // got  6.0.5270.7 for CTP, so it could be zero
            //AssertM( *pverSp > 0 );
            }
            else{
                // Kernel changes to correctly formulate CSDVersion string from SP#, build, release type.
                switch ( fileversionHighMajor ){
                    case 6:
                        if ( buildno >= 5600 && buildno < 6000 ){
                            *pverSp = buildno - 5600;
                            *pverBuild = spbuildno;
                        }
                        else if ( buildno >= 6000 && buildno < 6400 ){
                            *pverSp = buildno - 6000;
                            *pverBuild = spbuildno;
                        }
                        else if ( buildno >= 6400 ){
                            *pverSp = 0;
                            *pverBuild = buildno;
                        }
                        break;
                }
                
            }           
        }       

        break;
    default:
        AssertM( 0 && "invalid flavour" );
        err = JET_errTestError;
        goto Cleanup;
    }

Cleanup:
    return err;
}


JET_ERR
EsetestGetEseVersion(
    __out EsetestEseFlavour*    pflavour,
    __out EsetestEseVersion*    peseversion,
    __out bool*                 pfChecked
)
{
    JET_ERR     err     = JET_errSuccess;
    VS_FIXEDFILEINFO*   pfixedfileinfo = NULL;
    UINT                uiLen       = 0;
    const wchar_t*      wszEseDll   = WszEsetestEseDll();
    DWORD               dwLen       = 0;
    PVOID               pvData      = NULL;
    DWORD               dwGarbage;
    ULARGE_INTEGER      li;

    AssertM( peseversion );
    if ( !peseversion )
    {
        err = JET_errInvalidParameter;
        goto Cleanup;
    }
    *peseversion = 0i64;

    if ( pflavour )
    {
        *pflavour = EsetestEseFlavourNil;
    }

    dwLen = GetFileVersionInfoSizeExW(0, wszEseDll, &dwGarbage );

    pvData = malloc( dwLen );
    if ( NULL == pvData )
    {
        tprintf( "%s(): Failed to allocate %d bytes" CRLF, __FUNCTION__, dwLen );
        err = JET_errOutOfMemory;
        goto Cleanup;
    }

    if ( !GetFileVersionInfoExW(0, wszEseDll, 0, dwLen, pvData ) )
    {
        tprintf( "GetFileVersionInfoExW() failed. GLE = %d" CRLF, GetLastError() );
        err = JET_errTestError;
        goto Cleanup;
    }

    if ( !VerQueryValueW( pvData, L"\\", reinterpret_cast< PVOID* >( &pfixedfileinfo ), &uiLen ) )
    {
        tprintf( "VerQueryValueW() failed. GLE = %d" CRLF, GetLastError() );
        err = JET_errTestError;
        goto Cleanup;
    }



    const EsetestEseFlavour flavour = EsetestGetEseFlavour();
    li.HighPart = pfixedfileinfo->dwFileVersionMS;
    li.LowPart = pfixedfileinfo->dwFileVersionLS;
    li.HighPart |= ( static_cast<UINT>( flavour ) << 28 );

    if ( pfChecked )
    {
#ifndef ESE_CORESYSTEM_WORK_AROUND
        *pfChecked = ( pfixedfileinfo->dwFileFlags & VS_FF_DEBUG ) ? true : false;
#else
        // phone debug esent.dll somehow does not have the debug bit set - assume it is the same as current binary
#ifdef DEBUG
        *pfChecked = true;
#else
        *pfChecked = false;
#endif
#endif
    }

    if ( peseversion )
    {
        *peseversion = li.QuadPart;
    }

    if ( pflavour )
    {
        *pflavour = flavour;
    }

Cleanup:
    if ( pvData )
    {
        free( pvData );
    }
    return err;
}

JET_ERR
EsetestGetEseVersionParts(
    __out   EsetestEseFlavour*  pflavour,
    __out   PUINT               pverMajor,
    __out   PUINT               pverMinor,
    __out   PUINT               pverSp,
    __out   PUINT               pverBuild,
    __out   bool*               pfChecked
)
{
    JET_ERR     err     = JET_errSuccess;
    EsetestEseVersion   eseversion  = 0;

    AssertM( pflavour );
    AssertM( pverMajor );
    AssertM( pverMinor );
    AssertM( pverSp );
    AssertM( pverBuild );
    AssertM( pfChecked );
    if ( !pflavour || !pverMajor || !pverMinor || !pverSp || !pverBuild || !pfChecked )
    {
        err = JET_errInvalidParameter;
        goto Cleanup;
    }

    *pflavour = EsetestEseFlavourNil;
    *pverMajor = 0;
    *pverMinor = 0;
    *pverSp = 0;
    *pverBuild = 0;
    *pfChecked = false;

    Call( EsetestGetEseVersion( pflavour, &eseversion, pfChecked ) );
    Call( EsetestEseVersionToParts( eseversion, pflavour, pverMajor, pverMinor, pverSp, pverBuild ) );

Cleanup:
    return err;
}

JET_ERR
EsetestGetEseFileVersionParts(
    __out   EsetestEseFlavour*  pflavour,
    __out   PUINT               pverMajor,
    __out   PUINT               pverMinor,
    __out   PUINT               pverBuild,
    __out   PUINT               pverPrivate,
    __out   bool*               pfChecked
)
{
    JET_ERR     err     = JET_errSuccess;
    EsetestEseVersion   eseversion  = 0;

    AssertM( pflavour );
    AssertM( pverMajor );
    AssertM( pverMinor );
    AssertM( pverBuild );
    AssertM( pverPrivate );
    AssertM( pfChecked );
    if ( !pflavour || !pverMajor || !pverMinor || !pverBuild || !pverPrivate || !pfChecked )
    {
        err = JET_errInvalidParameter;
        goto Cleanup;
    }

    *pflavour = EsetestEseFlavourNil;
    *pverMajor = 0;
    *pverMinor = 0;
    *pverBuild = 0;
    *pverPrivate = 0;
    *pfChecked = false;

    Call( EsetestGetEseVersion( pflavour, &eseversion, pfChecked ) );
    Call( EsetestEseFileVersionToParts( eseversion, pflavour, pverMajor, pverMinor, pverBuild, pverPrivate ) );

Cleanup:
    return err;
}

JET_ERR
EsetestEseVersionToParts(
    __in EsetestEseVersion      version,
    __out EsetestEseFlavour*    pflavour,
    __out PUINT                 pverMajor,
    __out PUINT                 pverMinor,
    __out PUINT                 pverSp,
    __out PUINT                 pverBuild
)
{
    JET_ERR     err     = JET_errSuccess;
    ULARGE_INTEGER  li;
    li.QuadPart = version;

    // Mask the Flavour
    *pflavour = static_cast< EsetestEseFlavour >( ( li.HighPart >> 28 ) & 0xf );
    *pverMajor  = (HIWORD( li.HighPart ) & 0xfff);
    *pverMinor  = LOWORD( li.HighPart );
    Call( EsetestFillInSpAndBuild( pflavour, *pverMajor, *pverMinor, li.LowPart, pverSp, pverBuild ) );

Cleanup:
    return err;
}

JET_ERR
EsetestEseFileVersionToParts(
    __in EsetestEseVersion      version,
    __out EsetestEseFlavour*    pflavour,
    __out PUINT                 pverMajor,
    __out PUINT                 pverMinor,
    __out PUINT                 pverBuild,
    __out PUINT                 pverPrivate
)
{
    JET_ERR     err     = JET_errSuccess;
    ULARGE_INTEGER  li;
    li.QuadPart = version;

    // Mask the Flavour
    *pflavour       = static_cast< EsetestEseFlavour >( ( li.HighPart >> 28 ) & 0xf );
    *pverMajor      = (HIWORD( li.HighPart ) & 0xfff);
    *pverMinor      = LOWORD( li.HighPart );
    *pverBuild      = HIWORD( li.LowPart );
    *pverPrivate    = LOWORD( li.LowPart );

    return err;
}

bool
FEsetestIsBugFixed(
    __in    UINT                bugnumberEse,       // Can be 0
    __in    UINT                bugnumberEsent      // Can be 0
)
// Sometimes a bug is filed in either ESE or ESENT, but it will eventually be fixed in both.
// Therefore we must allow the caller to specify a bug number of zero.
{
    bool    fRet    = false;
    struct EseBugs{

        size_t              m_bugnumberEse;
        UINT                m_verMajorEse;
        UINT                m_verMinorEse;
        UINT                m_verSpEse;
        UINT                m_verBuildEse;

        size_t              m_bugnumberEsent;
        UINT                m_verMajorEsent;
        UINT                m_verMinorEsent;
        UINT                m_verSpEsent;
        UINT                m_verBuildEsent;
    };

    static const EseBugs rgesebugsEse[] =
    {
        // Test cases for the function.
        { 11,   6, 0, 0, 0,     0,      5, 2, 1, 0 },   // specify ESE only
        { 0,    6, 0, 0, 0,     12,     5, 2, 1, 0 },   // specify ESENT only
        { 0,    6, 0, 0, 0,     13,     6, 2, 1, 0 },   // not supported
        { 14,   6, 0, 0, 0,     15,     5, 2, 1, 0 },   // both bugs
        { 0,    6, 0, 0, 0,     16,     6, 1, 1, 0 },   // Longhorn+

        { 0,    8, 0, 0, 365,   950134, 6, 0, 0, 5069 }, // Backup API needs to check for buffer alignment.
        { 0,    8, 0, 0, 596,   1567397,    6, 0, 0, 5362 }, // JetGetPageInfo should ignore PGHDR::ibMicFree.
        { 0,    0xFFFFFFFF, 0, 0, 0,    1634256,    0xFFFFFFFF, 0, 0, 0 }, // JetOSSnapshot APIs should fail with JET_errOSSnapshotTimeOut if the session times out.
        { 0,    8, 0, 0, 596,   1636877,    6, 0, 0, 5444 }, // JetOSSnapshotFreezeA should validate pointers before JetOSSnapshotFreezeEx.
        { 0,    8, 0, 0, 596,   1636979,    6, 0, 0, 5444 }, // JetOSSnapshotAbort doesn't validate grbit.
        { 95306,    8, 0, 0, 663,   0,  6, 1, 0, 6470 }, // crash attempting to update an index with multi-valued key columns and more than 6 key segments (add test case).
        { 41418,    14, 0, 0, 187,  0,  6, 1, 0, 6470 }, // Hang in BTISelectRightSplit.
    }
    ;

/*
    static const EseBugs esebugsEsent[] =
    {
        // ESENT
        { 950134,   6, 0, 0, 5069 },
    }
    ;
*/
    const size_t    cbugsEse    = sizeof2( rgesebugsEse );

//  const size_t    cbugsEsent  = sizeof2( esebugsEsent );

    const EseBugs*      pesebugsT   = NULL;

    // Find the element
    for ( size_t i = 0; i < cbugsEse; ++i )
    {
        if ( ( bugnumberEse > 0 && bugnumberEse == rgesebugsEse[ i ].m_bugnumberEse )
            || ( bugnumberEsent > 0 && bugnumberEsent == rgesebugsEse[ i ].m_bugnumberEsent )
        )
        {
            pesebugsT = &rgesebugsEse[ i ];
            break;
        }
    }

    if ( pesebugsT )
    {
        // Has it been fixed in ESE?
        if ( FEsetestVerifyVersion( EsetestEseFlavourEse, pesebugsT->m_verMajorEse, pesebugsT->m_verMinorEse, pesebugsT->m_verSpEse, pesebugsT->m_verBuildEse ) )
        {
            fRet = true;
            goto Cleanup;
        }

        // Has it been fixed in ESENT?
        if ( FEsetestVerifyVersion( EsetestEseFlavourEsent, pesebugsT->m_verMajorEsent, pesebugsT->m_verMinorEsent, pesebugsT->m_verSpEsent, pesebugsT->m_verBuildEsent ) )
        {
            fRet = true;
            goto Cleanup;
        }
    }


/*
    if ( ppesebugsT )
    {
        for ( size_t i = 0; i < 2; ++i )
        {
            EseBugs*    pesebugsT   = pesebugsT[ i ];
            if ( FEsetestVerifyVersion( pesebugsT->m_flavour, pesebugsT->m_verMajor, pesebugsT->m_verMinor, pesebugsT->m_verSp, pesebugsT->m_verBuild ) )
            {
                fRet = true;
                goto Cleanup;
            }
        }
    }
*/

/*
    // Now to check for ESENT bugs
    if ( bugnumberEsent > 0 )
    {
        for ( size_t i = 0; i < cbugsEsent; ++i )
        {
            if ( bugnumberEsent == cbugsEsent[ i ].m_bugnumber )
            {
                pesebugsT = &cbugsEsent[ i ];
                break;
            }
        }
    }

    if ( pesebugsT )
    {
        if ( FEsetestVerifyVersion( EsetestEseFlavourEsent, pesebugsT->m_verMajor, pesebugsT->mverMinor, pesebugsT->mverSp, pesebugsT->mverBuild ) )
        {
            fRet = true;
            goto Cleanup;
        }
    }
*/
Cleanup:
    return fRet;
}

bool
FEsetestVerifyVersion(
    __in    EsetestEseFlavour   flavour,
    __in    UINT                verMajor,
    __in    UINT                verMinor,
    __in    UINT                verSp,
    __in    UINT                verBuild        // can be zero
)
// Returns whether the current version of ESE meets the minimum required.
{
    bool    fRet    = false;
    JET_ERR err     = JET_errSuccess;

    EsetestEseFlavour   flavourActual;
    UINT    verMajorActual;
    UINT    verMinorActual;
    UINT    verSpActual;
    UINT    verBuildActual;
    bool    fChecked;

    Call( EsetestGetEseVersionParts( &flavourActual, &verMajorActual, &verMinorActual, &verSpActual, &verBuildActual, &fChecked ) );
    
    if ( flavour != flavourActual )
    {
        goto Cleanup;
    }

    // Check the Major Version
    if ( verMajorActual > verMajor )
    {
        fRet = true;
        goto Cleanup;
    }
    else if ( verMajorActual < verMajor )
    {
        goto Cleanup;
    }

    // Check the Minor Version
    if ( verMinorActual > verMinor )
    {
        fRet = true;
        goto Cleanup;
    }
    else if ( verMinorActual < verMinor )
    {
        goto Cleanup;
    }

    // Check the Sp Version
    if ( verSpActual > verSp )
    {
        fRet = true;
        goto Cleanup;
    }
    else if ( verSpActual < verSp )
    {
        goto Cleanup;
    }

    // Check the Build Version
    if ( verBuildActual > verBuild )
    {
        fRet = true;
        goto Cleanup;
    }
    else if ( verBuildActual < verBuild )
    {
        goto Cleanup;
    }

    // exactly the required version
    AssertM( verMajor == verMajorActual );
    AssertM( verMinor == verMinorActual );
    AssertM( verSp == verSpActual );
    AssertM( verBuild == verBuildActual );
    fRet = true;

Cleanup:
    return fRet;
}



bool
FEsetestWidenParameters()
{

    bool fRet;

    //tprintf (  "PctWideApis is %d" CRLF, g_esetestconfig.m_pctWideApis );
    if ( FEsetestAlwaysNarrow() == true ) return false;

    // If it's 0 or 100, don't bother checking the random number.

    if ( 0 == g_esetestconfig.m_pctWideApis )
    {
        fRet = false;   //no widen
    }
    else if ( 100 == g_esetestconfig.m_pctWideApis )
    {
        fRet = true;    //all widen
    }
    else
    {
        fRet = ( ( static_cast< size_t >( ( ( ( unsigned )IRand() ) % 100 ) ) ) < g_esetestconfig.m_pctWideApis );
        
    }

    return fRet;
}

void
EsetestSetWidenParametersPercent(
    const size_t    pctApisToWiden
)
{
    g_esetestconfig.m_pctWideApis = pctApisToWiden;
}

JET_GRBIT GrbitColumnCompression( JET_GRBIT grbit )
{
    if ( g_esetestconfig.m_pctCompression < 0 ){
        return NO_GRBIT;
    }
    if ( grbit & JET_bitColumnCompressed ){
        return NO_GRBIT;
    }
    if ( ( ( ( unsigned )IRand() ) % 100 ) < ( unsigned )g_esetestconfig.m_pctCompression ){
        return JET_bitColumnCompressed;
    }
    else{
        return NO_GRBIT;
    }
}

JET_GRBIT GrbitDataCompression( JET_GRBIT grbit )
{
    if ( g_esetestconfig.m_pctCompression < 0 ){
        return NO_GRBIT;
    }
    if ( grbit & ( JET_bitSetCompressed | JET_bitSetUncompressed ) ){
        return NO_GRBIT;
    }
    if ( !!( ( ( unsigned )IRand() ) % 2 ) ){
        return NO_GRBIT;
    }
    if ( ( ( ( unsigned )IRand() ) % 100 ) < ( unsigned )g_esetestconfig.m_pctCompression ){
        return JET_bitSetCompressed;
    }
    else{
        return JET_bitSetUncompressed;
    }
}

void
EsetestSetCompressionPercent(
    const int   pctCompression
)
{
    if( FEsetestFeaturePresent( EseFeatureLVCompression ) ){
        g_esetestconfig.m_pctCompression = pctCompression;
    }
    else{
        g_esetestconfig.m_pctCompression = -1;
    }
}

void
EsetestSetAssertAction(
    const JET_API_PTR esetestAssertAction
)
{
    g_esetestconfig.m_esetestAssertAction = (UINT) esetestAssertAction;
}

HMODULE
HmodEsetestEseDll()
{
    if ( NULL == g_esetestconfig.m_hmodEseDll )
    {
        EnterCriticalSection( &g_esetestconfig.m_csEseDll );
        if ( NULL == g_esetestconfig.m_hmodEseDll )
        {
            const wchar_t*  wszEseDll = WszEsetestEseDll();
            g_esetestconfig.m_hmodEseDll = LoadLibraryExW( wszEseDll, NULL, 0 );
            if ( NULL == g_esetestconfig.m_hmodEseDll )
            {
                tcprintf( ANSI_RED, __FUNCTION__ "(): Failed to LoadLibraryExW( %ws , NULL, 0 ), GLE = %d", wszEseDll, GetLastError() );
            }
        }
        LeaveCriticalSection( &g_esetestconfig.m_csEseDll );
    }

    return g_esetestconfig.m_hmodEseDll;
}

HMODULE
HmodEsetestEsebackDll()
{
    if ( NULL == g_esetestconfig.m_hmodEsebackDll )
    {
        EnterCriticalSection( &g_esetestconfig.m_csEsebackDll );
        if ( NULL == g_esetestconfig.m_hmodEsebackDll )
        {
            const wchar_t*  wszEsebackDll = WszEsetestEsebackDll();
            g_esetestconfig.m_hmodEsebackDll = LoadLibraryExW( wszEsebackDll, NULL, 0 );
            if ( NULL == g_esetestconfig.m_hmodEsebackDll )
            {
                tcprintf( ANSI_RED, __FUNCTION__ "(): Failed to LoadLibraryExW( %ws, NULL, 0 ), GLE = %d", wszEsebackDll, GetLastError() );
            }
        }
        LeaveCriticalSection( &g_esetestconfig.m_csEsebackDll );
    }

    return g_esetestconfig.m_hmodEsebackDll;
}

ULONG64
QWEsetestQueryPerformanceFrequency()
// Returns the frequency of the high-resolution performance counter, in counters per sec.
{
    LARGE_INTEGER frequency;

    if ( QueryPerformanceFrequency( &frequency ) )
    {
        return frequency.QuadPart;
    }
    else
    {
        return 1000;
    }
}

ULONG64
QWEsetestQueryPerformanceCounter()
// Returns the number of milliseconds, using QueryPerformanceFrequency()
// if possible.
{
    static bool fCheckedForQpfSupport       = false;
    static bool fUseQueryPerformanceCounter = false;
    static ULONG64  ulDivisor   = 1;
    LARGE_INTEGER   liResult;

    if ( !fCheckedForQpfSupport )
    {
        LARGE_INTEGER   liTemp;
        // Not synchronized, but that shouldn't be a problem.
        if ( QueryPerformanceFrequency( &liTemp ) )
        {
            // Since we want a result in milliseconds.
            ulDivisor = liTemp.QuadPart / 1000;
            fUseQueryPerformanceCounter = true;
        }
        fCheckedForQpfSupport = true;
    }

    if ( fUseQueryPerformanceCounter )
    {
        QueryPerformanceCounter( &liResult );
        return liResult.QuadPart / ulDivisor;
    }
    else
    {
        return (ULONG64) GetTickCount();
    }
}

// Prototype
PRIVATE int AppendToFileHandleFromFileHandle(HANDLE hfileSrc, HANDLE hfileDest);
PRIVATE VOID CustomEseInitTest();
PRIVATE VOID CustomEsentInitTest();
PRIVATE VOID CustomEseAndEsentInitTest();

// Stuff for asserts:
#define Assert( exp )               AssertSz( exp, #exp )

// Also for assert stuff, but this is to minimise code change when we steal new versions from error.cxx
#ifndef LOCAL
#define LOCAL   PRIVATE
#endif

// These globals help debugging dumps.
static DWORD pidAssert;
static DWORD tidAssert;
static const _TCHAR * szFilenameAssert;
static long lLineAssert;

#define chPathDelimiter   '\\'
const CHAR szNewLine[]          = "\r\n";
const WCHAR wszNewLine[]        = L"\r\n";

const WCHAR wszAssertCaption[]  = L"JET Assertion Failure";
const WCHAR wszAssertPrompt[]   = L"Choose OK to continue execution or CANCEL to debug the process.";
const WCHAR wszAssertPrompt2[]  = L"Choose OK to continue execution (attaching the debugger is impossible during process initialization and termination).";

#define OSStrCbAppendA( szDst, cbDst, szSrc )       { if(ErrOSStrCbAppendA( szDst, cbDst, szSrc )){ AssertSz( fFalse, "Success expected"); } }
#define OSStrCbAppendW( wszDst, cbDst, wszSrc )     { if(ErrOSStrCbAppendW( wszDst, cbDst, wszSrc )){ AssertSz( fFalse, "Success expected"); } }


#define DwUtilProcessId()               ::GetCurrentProcessId()
#define DwUtilThreadId()                ::GetCurrentThreadId()

// When can't it be? I think only in DllMain()
BOOL IsDebuggerAttachable();
BOOL IsDebuggerAttached();

void UserDebugBreakPoint();

#define SzUtilImageVersionName()    _T( "unknown_esetest_binary.exe" )

/////////////////////////////////////////////////////////////////////
// Format GetLastError in a string
/////////////////////////////////////////////////////////////////////
void FormatLastError(
    _Out_writes_z_( iLength ) char* szTemp,
    _In_ const int iLength
)
{
    const DWORD dwGle = GetLastError(); 
    FormatSpecificError( szTemp, iLength, dwGle );
}

/////////////////////////////////////////////////////////////////////
// Format specific error to a string
/////////////////////////////////////////////////////////////////////
void FormatSpecificError(
    __in_ecount( iLength ) char* szTemp,
    _In_ const int iLength,
    _In_ const DWORD dwError
)
{
    LPVOID lpMsgBuf;
    DWORD cchWritten; 

    cchWritten = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL
    );

    // WinSE teams hits AV. So we need to check if FormatMessage() succeeds.
    if ( cchWritten == 0 )
    {
        // fails
        StringCchPrintfA( szTemp, iLength, "FormatMessage() for error %d failed with %d.",
                          dwError, GetLastError() );
    } else 
    {
        // succeeds
        StringCchPrintfA( szTemp, iLength, "%s", (LPCTSTR)lpMsgBuf);
        LocalFree( lpMsgBuf );
    }
}

/////////////////////////////////////////////////////////////////////
// Memory-related functions
/////////////////////////////////////////////////////////////////////
static HANDLE   g_hHeap = NULL;

void*
PvMemoryHeapAlloc(
    __in const size_t   cbSize
)
{
    if ( !g_hHeap )
    {
        // It should have been allocated in DllMain(), but sometimes we statically
        // link, in which case DllMain() is never run.
        g_hHeap = GetProcessHeap();
    }
    AssertM( g_hHeap );

    // During stress runs, out of memory errors are more likely.

    // Retry 20 times, exponentially backing off.
    // 20 times means the last attempt will take a bit over 17 minutes.
    size_t  cmsecBackoff    = 1;
    const size_t    cRetries    = g_esetestconfig.m_fRunningStress ? 20 : 4;

    for ( size_t i = 0; i < cRetries; ++i )
    {
        void* const pv = HeapAlloc( g_hHeap, 0, cbSize );
        if ( pv )
        {
            return pv;
        }
        cmsecBackoff <<= 1;
        Sleep( cmsecBackoff );
    }

    // Drat. Retrying didn't work.
    return NULL;
}

void
MemoryHeapFree(
    __in_opt void* const    pv
)
{
    if ( pv )
    {
        AssertM( g_hHeap );
        const BOOL fMemFreed = HeapFree( g_hHeap, 0, pv );
        AssertM( fMemFreed );
    }
}

extern "C"
{

#define _DECL_DLLMAIN
#include <process.h>

BOOL WINAPI DllMainEsetest( HANDLE hinstDLL, DWORD dwReason, LPVOID lpvReserved )
{
    BOOL fResult = TRUE;
    switch( dwReason ) {
        case DLL_PROCESS_ATTACH: {

            // This should be run before any possible allocations.
            g_hHeap = GetProcessHeap();

            // We do not want to see thread notifications.
            (void)DisableThreadLibraryCalls( (HMODULE)hinstDLL );

            // init /GS cookie.
            __security_init_cookie();

            // Init CRT.
            fResult = fResult && _CRT_INIT( hinstDLL, dwReason, lpvReserved );

            break;
        }
        case DLL_PROCESS_DETACH: {
            // Terminate CRT.
            (void)_CRT_INIT( hinstDLL, dwReason, lpvReserved );

            break;
        }
        case DLL_THREAD_ATTACH: {
            break;
        }
        case DLL_THREAD_DETACH: {
            break;
        }
        default:
            break;
    }
    return fResult;
}
}



/////////////////////////////////////////////////////////////////////
// Logging utility functions
/////////////////////////////////////////////////////////////////////

void
EsetestSetResultsTxt(
    __in const char*    szResultsTxt
)
{
    const size_t    cchResultsTxt   = sizeof( RESULTS_TXT ) / sizeof( RESULTS_TXT[ 0 ] );
    strncpy( RESULTS_TXT, szResultsTxt, cchResultsTxt );
    RESULTS_TXT[ cchResultsTxt - 1 ] = '\0';
}

void
EsetestSetResultsLog(
    __in const char*    szResultsLog
)
{
    const size_t    cchResultsLog   = sizeof( RESULTS_LOG ) / sizeof( RESULTS_LOG[ 0 ] );
    strncpy( RESULTS_LOG, szResultsLog, cchResultsLog );
    RESULTS_LOG[ cchResultsLog - 1 ] = '\0';
}

void InitLogging( BOOL fLogToDisk /* = TRUE */ )
{
    if ( !g_fLoggingInitialized ) {
        // Moved the InitializeCriticalSection() to the global object.
        // This means that InitLogging() won't actually re-initialize the logging
        // after TermLogging() is called.
        g_fLoggingInitialized = true;
    }
    gfLogToFile = fLogToDisk;

#ifdef BUILD_ENV_IS_NT
    EsetestConfig* const    pcfg = &g_esetestconfig;
    bool        fFailed = false;
    HRESULT     hr;

    if ( FEnvironmentVariableSet( g_szEnvironmentUseWttLog ) )
    {
        pcfg->m_fWttLog = true;
    }

    if ( pcfg->m_fWttLog )
    {
        pcfg->m_hmodWttLog = LoadLibraryExW( L"wttlog.dll", NULL, 0 );
        if ( !pcfg->m_hmodWttLog )
        {
            pcfg->m_fWttLog = false;
            tprintf( "Failed to LoadLibraryEx( wttlog.dll ), gle = %d" CRLF, GetLastError() );
            goto Cleanup;
        }

        do {
            pcfg->pfnWTTLogCreateLogDevice  = reinterpret_cast< PFNWTTLogCreateLogDevice >( GetProcAddress( pcfg->m_hmodWttLog, "WTTLogCreateLogDevice" ) );
            if ( !pcfg->pfnWTTLogCreateLogDevice ) { fFailed = true; break; }

            pcfg->pfnWTTLogStartTest            = reinterpret_cast< PFNWTTLogStartTest >( GetProcAddress( pcfg->m_hmodWttLog, "WTTLogStartTest" ) );
            if ( !pcfg->pfnWTTLogStartTest ) { fFailed = true; break; }

            pcfg->pfnWTTLogEndTest          = reinterpret_cast< PFNWTTLogEndTest >( GetProcAddress( pcfg->m_hmodWttLog, "WTTLogEndTest" ) );
            if ( !pcfg->pfnWTTLogEndTest ) { fFailed = true; break; }

            pcfg->pfnWTTLogCreateLogDevice  = reinterpret_cast< PFNWTTLogCreateLogDevice >( GetProcAddress( pcfg->m_hmodWttLog, "WTTLogCreateLogDevice" ) );
            if ( !pcfg->pfnWTTLogCreateLogDevice ) { fFailed = true; break; }

            pcfg->pfnWTTLogTrace                = reinterpret_cast< PFNWTTLogTrace >( GetProcAddress( pcfg->m_hmodWttLog, "WTTLogTrace" ) );
            if ( !pcfg->pfnWTTLogTrace ) { fFailed = true; break; }

            pcfg->pfnWTTLogCloseLogDevice   = reinterpret_cast< PFNWTTLogCloseLogDevice >( GetProcAddress( pcfg->m_hmodWttLog, "WTTLogCloseLogDevice" ) );
            if ( !pcfg->pfnWTTLogCloseLogDevice ) { fFailed = true; break; }

            pcfg->pfnWTTLogUninit           = reinterpret_cast< PFNWTTLogUninit >( GetProcAddress( pcfg->m_hmodWttLog, "WTTLogUninit" ) );
            if ( !pcfg->pfnWTTLogUninit ) { fFailed = true; break; }
        } while ( false );

        if ( fFailed )
        {
            pcfg->m_fWttLog = false;
            tprintf( "Failed to GetProcAddress one of the WTT functions, GLE = %d" CRLF, GetLastError() );
            goto Cleanup;
        }

        // Device string syntax is:
        // $<device_type>:<parameter_list>($<child_device1>[; $<child_device2>; ...])
        hr = pcfg->pfnWTTLogCreateLogDevice(
            L"$LocalPub($LogFile:"
            L"file=\"results_esetest.xml\"" // File name. By default, this will be the name of the EXE file with .WTL extension. If you want to specify the full path name, you need to escape the file name with double-quote. For example: file="c:\temp\myxmllog.xml"
            L",Shared=true"         // Specifies whether the log file is shared between more than one process. This option introduces a minor performance degradation. The log file is always open/created with shared access level in the OS. Default = false
            L",Compressed=false"    // Specifies whether the log file is compressed. Default = true if the file is created on an NTFS volume.
            L",CRC=true"            // Specifies whether the log file needs to be CRCÆd at the end. Default = false
            L",writemode=append"    // How the log file should be opened. Default = overwrite
            L",Nofscache=true"      // Specifies whether the file system cache needs to be turned off. Default = false Warning: if this option is specified, logging will become extremely slow.
            L";$Console)",
            &pcfg->m_hdevWttLog
        );

        if ( FAILED( hr ) )
        {
            pcfg->m_fWttLog = false;
            tprintf( "Failed to WTTLogCreateLogDevice, hr = %#x" CRLF, hr );
            goto Cleanup;
        }

    }
Cleanup:
    ;
#endif // BUILD_ENV_IS_NT
}

void TermLogging( void )
{
    if ( g_fLoggingInitialized )
    {
        // gcsLogging will be deleted by the global destructor on DLL detach.
        g_fLoggingInitialized = false;
    }

#ifdef BUILD_ENV_IS_NT
    EsetestConfig* const    pcfg = &g_esetestconfig;
    HRESULT     hr;

    if ( pcfg->m_fWttLog )
    {
        pcfg->m_hmodWttLog = LoadLibraryExW( L"wttlog.dll", NULL, 0 );
        // Device string syntax is:
        // $<device_type>:<parameter_list>($<child_device1>[; $<child_device2>; ...])
        hr = pcfg->pfnWTTLogCloseLogDevice (
            pcfg->m_hdevWttLog,
            NULL
        );

        if ( FAILED( hr ) )
        {
            pcfg->m_fWttLog = false;
            tprintf( "Failed to pfnWTTLogCloseLogDevice, hr = %#x" CRLF, hr );
            goto Cleanup;
        }

    }
Cleanup:
    ;
#endif // BUILD_ENV_IS_NT
}

BOOL EsetestSetLoggingToDisk( BOOL fLogToDisk )
{
    const BOOL  fOldValue = gfLogToFile;

    gfLogToFile = fLogToDisk;
    // By calling InitLogging() here, we allow usage of EsetestSetLoggingToDisk( FALSE ) and tprintf() before InitTest() is called.
    InitLogging( fLogToDisk );

    return fOldValue;
}

BOOL EsetestGetLoggingToDisk()
{
    return gfLogToFile;
}

BOOL EsetestDisableWritingToResultsTxt()
// I am not providing a corresponding 'enable' because I think this function should be used exceedingly sparingly
{
    BOOL fOld = g_fEnableWritingToResultsTxt;
    g_fEnableWritingToResultsTxt = false;
    return fOld;
}

PRIVATE
int
EsetestWttVprintf(
    _In_ _Printf_format_string_ const char*         szFormat,
    __in va_list                ap
)
{
    int         nRC         = 0;

#ifdef BUILD_ENV_IS_NT
    EsetestConfig* const    pcfg = &g_esetestconfig;
    AssertM( pcfg->pfnWTTLogTrace );
    HRESULT     hr;
    // Yes, this function uses a log of stack space. But WttLog does not do var args yet,
    // so we will sprintf() to a temporary buffer.
    const size_t    cchBufferIdioticWttLogDoesNotDoVarArgs  = 16384;
    char            rgchBufferIdioticWttLogDoesNotDoVarArgs[ cchBufferIdioticWttLogDoesNotDoVarArgs ];
    StringCchVPrintf( rgchBufferIdioticWttLogDoesNotDoVarArgs, cchBufferIdioticWttLogDoesNotDoVarArgs, szFormat, ap );

    hr = pcfg->pfnWTTLogTrace( pcfg->m_hdevWttLog, WTT_LVL_MSG, rgchBufferIdioticWttLogDoesNotDoVarArgs );
//      hr = pcfg->pfnWTTLogTrace( pcfg->m_hdevWttLog, WTT_LVL_MSG, szFormat, ap );
    if ( FAILED( hr ) )
    {
        printf( "%s(): Failed to pfnWTTLogTrace, hr = %#x. Trying again with vprintf()." CRLF, __FUNCTION__, hr );
        int cchWrittenToScreen = vprintf( szFormat, ap );
        nRC = FAILURE;
    }
#endif // BUILD_ENV_IS_NT

    fflush(stdout);
    return nRC;
}

int EsetestVprintf(
    __in_opt const char*            szLogFile,          // if NULL, will log to results.log
    __in const JET_GRBIT        level,
    _In_ _Printf_format_string_ const char*         szFormat,
    __in va_list                ap
)
{
#ifdef ESE_FLAVOUR_IS_ESE
    AssertM( !g_esetestconfig.m_fWttLog );
#endif // ESE_FLAVOUR_IS_ESE

    BOOL    fLogToScreen    = ( ( level & Esetest_bitLogToScreen )  ? TRUE : FALSE );
    // Do not log to disk if the logging subsystem hasn't been initialized yet.
    BOOL    fLogToDisk      = ( ( g_fLoggingInitialized && ( level & Esetest_bitLogToDisk ) )       ? TRUE : FALSE );
    const bool  fLogTid         = !( level & Esetest_bitLogDoNotLogThreadId );

    const char* szFileToLogTo   = szLogFile ? szLogFile : RESULTS_LOG;
    AssertM( szFileToLogTo );

    int         nRC         = 0;
    int         iTemp;
    FILE*       fileLog     = NULL;
    int         iFileLog    = EOF;
    char        rgchThreadId[20];
    int         cchWrittenToScreen  = 0;
    int         cchWrittenToDisk    = 0;

    rgchThreadId[ 0 ] = '\0';
    if ( fLogTid )
    {
        // We will record the thread id in the log as well
        _snprintf( rgchThreadId, sizeof(rgchThreadId), "TID:%lu, ", GetCurrentThreadId() );
        rgchThreadId[ sizeof( rgchThreadId ) - 1 ] = '\0';
    }

#ifdef BUILD_ENV_IS_NT
    EsetestConfig* const    pcfg = &g_esetestconfig;
    if ( pcfg->m_fWttLog )
    {
        return EsetestWttVprintf( szFormat, ap );
    }
#endif // BUILD_ENV_IS_NT


    EnterCriticalSection( &gcsLogging );
    __try {
        if ( fLogToScreen ) {
            // Log the text to the console first
            if ( fLogTid ){
                printf("%s", rgchThreadId );
            }           
            cchWrittenToScreen = vprintf( szFormat, ap );
        }

        // If the user did not turn logging on, then return immediately
        // before outputting the text to the log file
        if ( !gfLogToFile ) {
            __leave;
        }

        // Likewise if the user just doesn't want it logged to disk.
        if ( !fLogToDisk ) {
            __leave;
        }

        // Open the file and append the text to the file
        for ( int i = 0 ; i < 60 ; i++ ){
            iFileLog = _open( szFileToLogTo,
                _O_APPEND | _O_CREAT /* creates if nonexistent */ | _O_SEQUENTIAL | _O_WRONLY | _O_BINARY,
                _S_IREAD | _S_IWRITE );
            if ( EOF != iFileLog ) break;
            Sleep( 1000 );
        }

        AssertM( EOF != iFileLog );

        if ( EOF == iFileLog )
        {
            const int error = errno;
            const char* szErrorReason   = "unknown";

            switch ( error )
            {
            case EACCES:
                szErrorReason = "EACCES";
                break;

            case EEXIST:
                szErrorReason = "EEXIST";
                break;

            case EINVAL:
                szErrorReason = "EINVAL";
                break;

            case EMFILE:
                szErrorReason = "EMFILE";
                break;

            case ENOENT:
                szErrorReason = "ENOENT";
                break;

            default:
                __assume( 0 );
            }


            printf("Failed to open %s, _open() returned %s" SZNEWLINE, szFileToLogTo, szErrorReason );
            NTError(FALSE);
            __leave;
        }

        for ( int i =0 ; i < 10 ; i++ ){
            fileLog = _fdopen( iFileLog, "ab" );
            if ( NULL != fileLog ) break;
            Sleep( 1000 );
        }
        
        if ( NULL == fileLog )
        {
            printf( "Failed to fdopen( %d, ... ), which was previously opened with _open( %s )" CRLF, iFileLog, szFileToLogTo );
            NTError( FALSE );
            __leave;
        }

        // Append the text to the log file

        if ( fLogTid )
        {
            nRC = fprintf( fileLog, "%s", rgchThreadId );
            if ( nRC < 0 )
            {
                printf( " Failed to write text into %s, fprintf() returned %d" SZNEWLINE, szFileToLogTo, nRC );
                NTError( FALSE );
                __leave;
            }

            AssertM( (int) strlen( rgchThreadId ) == nRC );
        }


        nRC = vfprintf( fileLog, szFormat, ap );
        if ( nRC < 0 )
        {
            printf( " Failed to write text into %s, fprintf() returned %d" SZNEWLINE, szFileToLogTo, nRC );
            NTError( FALSE );
            __leave;
        }

        cchWrittenToDisk = nRC;

        if ( fLogToScreen )
        {
            AssertM( cchWrittenToScreen == cchWrittenToDisk );
        }
    }
    __finally {
        if ( NULL != fileLog )
        {
            iTemp = fclose( fileLog );
            // Ignore the failure case -- what should we do, anyway?
            // AssertM( 0 == iTemp );
            fileLog = NULL;

            // Apparently you do not need to _close the handle after the file pointer
            // has been fclose()ed.
            iFileLog = EOF;
        }
        else if ( EOF != iFileLog )
        {
            // in case the fdopen() failed, _close the _open()ed file.
            // Ignore the failure case -- what should we do, anyway?
            // AssertM( 0 == iTemp );
            iFileLog = EOF;
        }
        LeaveCriticalSection( &gcsLogging );

        fflush(stdout);
    }

    return nRC;
}

int EsetestVcprintf(
    __in DWORD                              dwColor,
    _In_ _Printf_format_string_ const char*     szFormat,
    __in va_list                            ap
)
{
    int nRC = 0;

    __try {
        EnterCriticalSection( &gcsLogging );

        HANDLE hConsole = NULL;
        hConsole = CreateFile( "CONOUT$",
            GENERIC_WRITE | GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL );
        if( INVALID_HANDLE_VALUE == hConsole ) {
            tprintf( CRLF "Error: Unable to open console for vOutputCommon( ). GLE( ) = %d" CRLF , ::GetLastError( ) );
            EsetestVprintf( NULL, Esetest_bitLogToDiskAndScreen, szFormat, ap );
            nRC = -1;
            __leave;
        }

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo( hConsole, &csbi );
        WORD wAttributesOld = csbi.wAttributes;

        SetConsoleTextAttribute( hConsole, ( WORD )dwColor );
        nRC = EsetestVprintf( NULL, Esetest_bitLogToDiskAndScreen, szFormat, ap );
        SetConsoleTextAttribute( hConsole, wAttributesOld );

        VerifyP( FALSE != CloseHandleP( &hConsole ) );
    }
    __finally {
        LeaveCriticalSection( &gcsLogging );
    }

    return nRC;
}


/////////////////////////////////////////////////////////////////////
// Printing
///////////////////////////////////////////////////////////////
// Logs to console and results.log, prepending the thread id.
int tprintf(
    _Printf_format_string_ const char* szFormat,
    ...
)
{
    int nRC = 0;
    va_list valst;
    va_start( valst, szFormat );

    nRC = EsetestVprintf( NULL, Esetest_bitLogToDiskAndScreen, szFormat, valst );

    va_end( valst );
    return nRC;
}

// Logs to console and results.log.
int tprintfnothid(
    _Printf_format_string_ const char* szFormat,
    ...
)
{
    int nRC = 0;
    va_list valst;
    va_start( valst, szFormat );

    nRC = EsetestVprintf(   NULL, Esetest_bitLogToDiskAndScreen | Esetest_bitLogDoNotLogThreadId, 
                            szFormat, valst );

    va_end( valst );
    return nRC;
}

int tprintfSpecifyTargets(
    __in_opt const char*            szLogFile,          // if NULL, will log to results.log
    __in const JET_GRBIT level,
    __in _Printf_format_string_ const char* szFormat,
    ...
)
/*++ level: one of
Esetest_bitLogDoNotLog
Esetest_bitLogToScreen
Esetest_bitLogToDisk
Esetest_bitLogToDiskAndScreen

--*/
{
    int nRC = 0;
    va_list valst;
    va_start( valst, szFormat );

    nRC = EsetestVprintf( szLogFile, level, szFormat, valst );

    va_end( valst );
    return nRC;
}

int tprintfSpecifyTargetsV(
    __in_opt const char*            szLogFile,          // if NULL, will log to results.log
    __in const JET_GRBIT level,
    __in _Printf_format_string_ const char* szFormat,
    __in va_list        valst
)
/*++ level: one of
Esetest_bitLogDoNotLog
Esetest_bitLogToScreen
Esetest_bitLogToDisk
Esetest_bitLogToDiskAndScreen

--*/
{
    int nRC = 0;

    nRC = EsetestVprintf( szLogFile, level, szFormat, valst );

    return nRC;
}


int
tprintfPerfLog(
    _In_ _Printf_format_string_ const char* szFormat,
    ...
)
{
    int nRC = 0;
    va_list valst;
    va_start( valst, szFormat );

    nRC = tprintfSpecifyTargetsV( "results.perf.log",
        Esetest_bitLogToDisk | Esetest_bitLogDoNotLogThreadId,
        szFormat,
        valst
    );

    va_end( valst );
    return nRC;
}

/////////////////////////////////////////////////////////////////////
// Printing
// results.txt is the file that contains the summary, like:
// foo -a -b -q .. FAILED
// printing to results.txt should therefore be done with cauion (because of how it needs to be parsed)
// I (SOMEONE, 2002.04.29) am adding this function just to make porting old tests a bit
// easier. In particular, mverify.
// (tprintf will print to results.log.)
///////////////////////////////////////////////////////////////
int tprintfResultsTxtUseWithCaution(
    _In_ _Printf_format_string_ const char* szFormat,
    ...
)
{
    int nRC = 0;
    va_list valst;
    va_start( valst, szFormat );

    nRC = EsetestVprintf( RESULTS_TXT, Esetest_bitLogToDiskAndScreen, szFormat, valst );

    va_end( valst );
    return nRC;
}

/////////////////////////////////////////////////////////////////////
// Printing with a different foreground colour
///////////////////////////////////////////////////////////////

int tcprintf(
    _In_ DWORD dColour,
    _In_ _Printf_format_string_ const char* szFormat,
    ...
)
{
    int nRC = 0;
    va_list valst;
    va_start( valst, szFormat );

    nRC = EsetestVcprintf( dColour, szFormat, valst );

    va_end( valst );
    return nRC;
}



//////////////////////////////////////////////////////////////
//  NTError
//////////////////////////////////////////////////////////////
void NTError( BOOL fLogToDisk /* = TRUE */)
{
    DWORD dwGle = GetLastError();

    if ( fLogToDisk ) {
        tcprintf( ANSI_RED, "\tNT Error: %lu" SZNEWLINE, dwGle );
    }
    else {
        tprintfSpecifyTargets( NULL, Esetest_bitLogToScreen, "\tNT Error: %lu" SZNEWLINE, dwGle );
    }
}

//////////////////////////////////////////////////////////////
//  InitTest
//////////////////////////////////////////////////////////////
int InitTestArgv( const int argc, const char* const argv[], BOOL fLogToDisk /* TRUE */, BOOL fWriteFailed /* TRUE */ )
// returns:
// SUCCESS
// FAILURE
{
    const size_t    cchCommand              = 16384;
    char*           szCommand               = new char[ cchCommand ];
    int             nCurrPos, nCount;
    int             nRet;
    if ( NULL == szCommand )
    {
        printf( "Warning: Could not allocate memory for szCommand " __FILE__ ":%d" CRLF, __LINE__ );
        goto Cleanup;
    }

    nCurrPos = sprintf( szCommand, "%s ",argv[0] );

    for ( nCount=1; nCount < argc; nCount++ ) {
        nCurrPos += sprintf( szCommand + nCurrPos, "%s ", argv[nCount] );
    }
    AssertM( strlen( szCommand ) < cchCommand );

//  // It looks better with a newline appended
//  nCurrPos += sprintf ( szCommand + nCurrPos, "%s", "" SZNEWLINE );

Cleanup:
    // InitTest() can accept NULLs
    nRet = InitTest( szCommand, fLogToDisk, fWriteFailed );

    delete[] szCommand;

    return nRet;
}

//////////////////////////////////////////////////////////////
int
InitTest(
    __in_opt const char *szCommandLine_,
    BOOL fLogToDisk,
    BOOL fWriteFailed
)
// szCommandLine can be NULL
//////////////////////////////////////////////////////////////
{
    InitLogging( fLogToDisk );

    const char* szCommandLine = szCommandLine_ ? szCommandLine_ : GetCommandLineA();

#ifdef BUILD_ENV_IS_NT
    HRESULT hr;
    EsetestConfig* const    pcfg = &g_esetestconfig;
    if ( pcfg->m_fWttLog )
    {
        AssertM( pcfg->pfnWTTLogStartTest );
        AssertM( pcfg->m_hdevWttLog );

        // It is not allowed to have a NULL wszTestName. Use
        // GetCommandeLineW() for both WTTLogStartTest and WTTLogEndTest().
        hr = pcfg->pfnWTTLogStartTest( pcfg->m_hdevWttLog, GetCommandLineW() );

        if ( FAILED( hr ) )
        {
            tprintf( "Failed to WTTLogStartTest(), hr = %#x" CRLF, hr );
        }
    }
    else
#endif // BUILD_ENV_IS_NT
    if ( g_fEnableWritingToResultsTxt ) {
        // Assume that the program will fail and only change results.txt
        // if this is not the case.
        HANDLE hFile = INVALID_HANDLE_VALUE;
        for ( int i =0 ; i < 10 ; i++ ){
            hFile = CreateFile(
                        RESULTS_TXT,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );
            if ( INVALID_HANDLE_VALUE != hFile ) break;
            Sleep( 1000 );
        }

        AssertM( INVALID_HANDLE_VALUE != hFile );

        if( INVALID_HANDLE_VALUE == hFile ||
            INVALID_SET_FILE_POINTER == SetFilePointer( hFile, 0, NULL, FILE_END ) ) {
            tprintf( "Failed to open %s" SZNEWLINE, RESULTS_TXT );
            NTError( );
            return FAILURE;
        }

        tprintf( SZNEWLINE );
        tprintf( "++++++++++++++++++++++++++++++++++++++++++++++++++" SZNEWLINE );
        tprintf( "Opening test: %s" SZNEWLINE, szCommandLine);
        tprintf( "++++++++++++++++++++++++++++++++++++++++++++++++++" SZNEWLINE );

        SYSTEMTIME stTimeGenerated;
        GetLocalTime ( &stTimeGenerated );
        DWORD dw = 0;
        char szBuffer[ 33 ];
        sprintf_s( szBuffer,
                    sizeof( szBuffer ),
                    "[%04u/%02u/%02u-%02u:%02u:%02u] ",
                    stTimeGenerated.wYear, stTimeGenerated.wMonth, stTimeGenerated.wDay,
                    stTimeGenerated.wHour, stTimeGenerated.wMinute, stTimeGenerated.wSecond );
        if ( FALSE == WriteFile( hFile, szBuffer, ( DWORD )strlen( szBuffer ), &dw, NULL) ) {
            tprintf( "Failed to write to %s" SZNEWLINE, RESULTS_TXT);
            NTError( );
            VerifyP( FALSE != CloseHandleP( &hFile ) );
            return FAILURE;
        }

        tprintf( "Timestamp: %s" SZNEWLINE, szBuffer );

        dw = 0;
        if ( FALSE == WriteFile( hFile, szCommandLine, ( DWORD )strlen( szCommandLine ), &dw, NULL) ) {
            tprintf( "Failed to write to %s" SZNEWLINE, RESULTS_TXT);
            NTError( );
            VerifyP( FALSE != CloseHandleP( &hFile ) );
            return FAILURE;
        }

        if ( fWriteFailed ) {
            if ( FALSE == WriteFile( hFile, FAIL_STR, ( DWORD )strlen( FAIL_STR ), &dw, NULL) ) {
                tprintf( "Failed to write to %s" SZNEWLINE, RESULTS_TXT );
                NTError( );
                VerifyP( FALSE != CloseHandleP( &hFile ) );
                return FAILURE;
            }
        }

        VerifyP( CloseHandleP( &hFile ) );
    }

    // I should really do something more generic here.
    CustomEseInitTest();
    CustomEsentInitTest();
    CustomEseAndEsentInitTest();

    return SUCCESS;
}

// This function was copied from loadcommon.cxx, and was known as BounceJetXXX().
// If that function changes, please update this one as well.
PRIVATE
JET_ERR
DynLoadJetSetResourceParam(
    JET_INSTANCE    instance,
    JET_RESOPER     resoper,
    JET_RESID       resid,
    JET_API_PTR     ulParam
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetSetResourceParam ) (
    JET_INSTANCE    instance,
    JET_RESOPER     resoper,
    JET_RESID       resid,
    JET_API_PTR     ulParam  );

    static PFN_JetSetResourceParam pfnJetSetResourceParam = NULL;

    // Get the procedure address if this is the first time calling this function.
    if ( NULL == pfnJetSetResourceParam )
    {
        // Do not bother with synchronization of GetProcAddress() -- sloppy, but a leak isn't a big deal.
        const HMODULE       hEseDll = HmodEsetestEseDll();

        if ( NULL != hEseDll )
        {
            pfnJetSetResourceParam = ( PFN_JetSetResourceParam ) ( GetProcAddress( hEseDll, "JetSetResourceParam" ) );
        }
        if ( NULL == hEseDll || NULL == pfnJetSetResourceParam )
        {
            tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                __FUNCTION__, hEseDll, "JetSetResourceParam", GetLastError() );
            err = JET_errTestError;
            goto Cleanup;
        }
    }

    err = (*pfnJetSetResourceParam)( instance, resoper, resid, ulParam );
    goto Cleanup;   // Need to have the explicit goto in case the function hasn't referenced 'Cleanup' yet.
Cleanup:

    return err;
}


// This function was copied from loadnarrow.cxx, and was known as BounceJetXXX().
// If that function changes, please update this one as well.
PRIVATE
JET_ERR
DynLoadJetSetSystemParameter(
    JET_INSTANCE    *pinstance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCSTR      szParam
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetSetSystemParameter ) (
    JET_INSTANCE    *pinstance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __in_opt JET_API_PTR        lParam,
    __in_opt JET_PCSTR      szParam  );

    static PFN_JetSetSystemParameter pfnJetSetSystemParameter = NULL;

    // Get the procedure address if this is the first time calling this function.
    if ( NULL == pfnJetSetSystemParameter )
    {
        // Do not bother with synchronization of GetProcAddress() -- sloppy, but a leak isn't a big deal.
        const HMODULE       hEseDll = HmodEsetestEseDll();

        if ( NULL != hEseDll )
        {
            pfnJetSetSystemParameter = ( PFN_JetSetSystemParameter ) ( GetProcAddress( hEseDll, "JetSetSystemParameter" ) );
        }
        if ( NULL == hEseDll || NULL == pfnJetSetSystemParameter )
        {
            tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                __FUNCTION__, hEseDll, "JetSetSystemParameter", GetLastError() );
            err = JET_errTestError;
            goto Cleanup;
        }
    }

    err = (*pfnJetSetSystemParameter)( pinstance, sesid, paramid, lParam, szParam );
    goto Cleanup;   // Need to have the explicit goto in case the function hasn't referenced 'Cleanup' yet.
Cleanup:

    return err;
}


// This function was copied from loadnarrow.cxx, and was known as BounceJetXXX().
// If that function changes, please update this one as well.
PRIVATE
JET_ERR
DynLoadJetGetSystemParameter(
    JET_INSTANCE    instance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __out_bcount_opt(cbMax) JET_API_PTR     *plParam,
    __out_bcount_opt(cbMax) JET_PSTR        szParam,
    unsigned long   cbMax
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetGetSystemParameter ) (
    JET_INSTANCE    instance,
    JET_SESID       sesid,
    unsigned long   paramid,
    __out_bcount_opt(cbMax) JET_API_PTR     *plParam,
    __out_bcount_opt(cbMax) JET_PSTR        szParam,
    unsigned long   cbMax  );

    static PFN_JetGetSystemParameter pfnJetGetSystemParameter = NULL;

    // Get the procedure address if this is the first time calling this function.
    if ( NULL == pfnJetGetSystemParameter )
    {
        // Do not bother with synchronization of GetProcAddress() -- sloppy, but a leak isn't a big deal.
        const HMODULE       hEseDll = HmodEsetestEseDll();

        if ( NULL != hEseDll )
        {
            pfnJetGetSystemParameter = ( PFN_JetGetSystemParameter ) ( GetProcAddress( hEseDll, "JetGetSystemParameter" ) );
        }
        if ( NULL == hEseDll || NULL == pfnJetGetSystemParameter )
        {
            tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                __FUNCTION__, hEseDll, "JetGetSystemParameter", GetLastError() );
            err = JET_errTestError;
            goto Cleanup;
        }
    }

    err = (*pfnJetGetSystemParameter)( instance, sesid, paramid, plParam, szParam, cbMax );
    goto Cleanup;   // Need to have the explicit goto in case the function hasn't referenced 'Cleanup' yet.
Cleanup:

    return err;
}

// This function was copied from loadnarrow.cxx, and was known as BounceJetXXX().
// If that function changes, please update this one as well.
PRIVATE
JET_ERR
DynLoadJetConsumeLogData(
    __in    JET_INSTANCE        instance,
    __in    JET_EMITDATACTX *   pEmitLogDataCtx,
    __in    void *              pvLogData,
    __in    unsigned long       cbLogData,
    __in    JET_GRBIT           grbits
)
{
    JET_ERR     err = JET_errSuccess;

    typedef JET_ERR ( __stdcall *PFN_JetConsumeLogData ) ( 
    __in    JET_INSTANCE        instance,
    __in    JET_EMITDATACTX *   pEmitLogDataCtx,
    __in    void *              pvLogData,
    __in    unsigned long       cbLogData,
    __in    JET_GRBIT           grbits  );

    static PFN_JetConsumeLogData pfnJetConsumeLogData = NULL;

    // Get the procedure address if this is the first time calling this function.
    if ( NULL == pfnJetConsumeLogData )
    {
        // Do not bother with synchronization of GetProcAddress() -- sloppy, but a leak isn't a big deal.
        const HMODULE       hEseDll = HmodEsetestEseDll();

        if ( NULL != hEseDll )
        {
            pfnJetConsumeLogData = ( PFN_JetConsumeLogData ) ( GetProcAddress( hEseDll, "JetConsumeLogData" ) );
        }
        if ( NULL == hEseDll || NULL == pfnJetConsumeLogData )
        {
            tprintf( "%s(): Failed to either fetch hEseDll (=%p) GetProcAddress( hEseDll, %s ), Gle = %d " CRLF,
                __FUNCTION__, hEseDll, "JetConsumeLogData", GetLastError() );
            err = JET_errTestError;
            goto Cleanup;
        }
    }

    err = (*pfnJetConsumeLogData)( instance, pEmitLogDataCtx, pvLogData, cbLogData, grbits );
    goto Cleanup;   // Need to have the explicit goto in case the function hasn't referenced 'Cleanup' yet.
Cleanup:

    return err;
}

//================================================================
// Registry API helper functions.
//================================================================
//

//  Code style doesn't match, as this is shamelessly stolen from windows RegGetValueA() ...

/*----------------------------------------------------------
Purpose: Helper for RegGetValueA()/RegGetValueW() 
*/
__inline LONG RestrictArguments(HKEY hkey, DWORD dwFlags, void *pvData, DWORD *pcbData)
{
    LONG Ret;
    DWORD dwRTFlags = dwFlags & RRF_RT_ANY;

    if (hkey
        && dwRTFlags
        && ( (dwRTFlags == RRF_RT_ANY) || !(dwFlags & RRF_RT_REG_EXPAND_SZ) || (dwFlags & RRF_NOEXPAND) )
        && (!pvData || pcbData)) {
        Ret = ERROR_SUCCESS;
    } else {
        Ret = ERROR_INVALID_PARAMETER;
    }

    return Ret;
}

/*----------------------------------------------------------
Purpose: Helpers for RegGetValueA()/RegGetValueW().
*/
LONG NullTerminateRegSzString(
    void *  pvData,         // data bytes returned from RegQueryValueEx()
    DWORD * pcbData,        // data size returned from RegQueryValueEx()
    DWORD   cbDataBuffer,   // data buffer size (actual allocated size of pvData)
    LONG    lr,             // long result returned from RegQueryValueEx()
    BOOLEAN Ansi            // ANSI or UNICODE
    )             
{
    DWORD   ElemSize = Ansi?sizeof(CHAR):sizeof(WCHAR); 

    AssertM(pcbData != NULL); // Sanity check.

    if (lr == ERROR_SUCCESS && pvData != NULL) {
        DWORD cchDataBuffer = cbDataBuffer / ElemSize; // cchDataBuffer is the actual allocated size of pvData in TCHARs
        DWORD cchData = *pcbData / ElemSize;           // cchData is the length of the string written into pvData in TCHARs (including the null terminator)
        PSTR  pszDataAnsi = (PSTR)pvData;
        PWSTR pszDataUnicode = (PWSTR)pvData;
        DWORD cNullsMissing;

        AssertM(cchDataBuffer >= cchData); // Sanity check.

        //
        // [1] string and size request with sufficient original buffer
        //     (must ensure returned string and returned size include
        //      null terminator)
        //

        if( Ansi ) {
            cNullsMissing = ((cchData >= 1) && (pszDataAnsi[cchData-1] == 0)) ? 0 : 1;
        } else {
            cNullsMissing = ((cchData >= 1) && (pszDataUnicode[cchData-1] == 0)) ? 0 : 1;
        }

        if (cNullsMissing > 0) {
            if( (cchData + cNullsMissing <= cchDataBuffer) &&
                 (cchData + cNullsMissing >= cchData)) {
                if( Ansi ) {
                    pszDataAnsi[cchData] = 0;
                } else {
                    pszDataUnicode[cchData] = 0;
                }
            } else {
                lr = ERROR_MORE_DATA;
            }
        }

        //  it is always possible for *pcbData to be odd!!!
        //  in the UNICODE case ONLY, when cchData is calculated the odd byte is truncated.
        //  this is fine when cNullsMissing is non-zero or if the buffer is large enough to begin with.
        //  but if there needs to be a resize of the buffer, then we must return the correct odd size.
        //  if we truncate we will always return a size that is insufficient by one byte.
        *pcbData = max(*pcbData, (cchData + cNullsMissing) * ElemSize);
    }
    else if ((lr == ERROR_SUCCESS && pvData == NULL) || lr == ERROR_MORE_DATA)
    {
        //
        // [2] size only request or string and size request with insufficient
        //     original buffer (must ensure returned size includes null
        //     terminator)
        //

        *pcbData += ElemSize; // *** APPROXIMATION FOR PERF -- size is
                                   // therefore not guaranteed to be exact,
                                   // merely sufficient
    }

    return lr;
}

/*----------------------------------------------------------
Purpose: Helper for SHRegQueryValueA()/SHRegQueryValueW().
*/
__inline LONG RestrictRegType(DWORD dwFlags, DWORD dwType, DWORD cbData, LONG lr)
{
    if (lr == ERROR_SUCCESS || lr == ERROR_MORE_DATA)
    {
        switch (dwType)
        {
            case REG_NONE:      if (!(dwFlags & RRF_RT_REG_NONE))              { lr = ERROR_UNSUPPORTED_TYPE; } break;
            case REG_SZ:        if (!(dwFlags & RRF_RT_REG_SZ))                { lr = ERROR_UNSUPPORTED_TYPE; } break;
            case REG_EXPAND_SZ: if (!(dwFlags & RRF_RT_REG_EXPAND_SZ))         { lr = ERROR_UNSUPPORTED_TYPE; } break;
            case REG_BINARY:
                if (dwFlags & RRF_RT_REG_BINARY)
                {
                    if ((dwFlags & RRF_RT_ANY) == RRF_RT_QWORD)
                    {
                        if (cbData > 8)
                            lr = ERROR_DATATYPE_MISMATCH;
                    }
                    else if ((dwFlags & RRF_RT_ANY) == RRF_RT_DWORD)
                    {
                        if (cbData > 4)
                            lr = ERROR_DATATYPE_MISMATCH;
                    }
                }
                else
                {
                    lr = ERROR_UNSUPPORTED_TYPE;
                }
                break;
            case REG_DWORD:     if (!(dwFlags & RRF_RT_REG_DWORD))             { lr = ERROR_UNSUPPORTED_TYPE; } break;
            case REG_MULTI_SZ:  if (!(dwFlags & RRF_RT_REG_MULTI_SZ))          { lr = ERROR_UNSUPPORTED_TYPE; } break;
            case REG_QWORD:     if (!(dwFlags & RRF_RT_REG_QWORD))             { lr = ERROR_UNSUPPORTED_TYPE; } break;
            default:            if (!((dwFlags & RRF_RT_ANY) == RRF_RT_ANY))   { lr = ERROR_UNSUPPORTED_TYPE; } break;
        }
    }

    return lr;
}


LONG FixRegData(
    HKEY    hkey,
    PVOID   pszValue,
    DWORD   dwFlags,
    DWORD * pdwType,
    void *  pvData,         // data bytes returned from RegQueryValueEx()
    DWORD * pcbData,        // data size returned from RegQueryValueEx()
    DWORD   cbDataBuffer,   // data buffer size (actual allocated size of pvData)
    LONG    lr,             // long result returned from RegQueryValueEx()
    BOOLEAN Ansi)           // ANSI or UNICODE selector  
{
    switch (*pdwType)
    {
        case REG_SZ:
            if (pcbData) {
                lr = NullTerminateRegSzString(pvData, pcbData, cbDataBuffer, lr, Ansi);
            }
            break;

        case REG_EXPAND_SZ:
/*
            if (pcbData) {
                lr = dwFlags & RRF_NOEXPAND
                    ? NullTerminateRegSzString(pvData, pcbData, cbDataBuffer, lr, Ansi)
                    : NullTerminateRegExpandSzString(hkey, pszValue, pdwType, pvData, pcbData, cbDataBuffer, lr, Ansi);
            }

            // Note:
            //  If we automatically expand the REG_EXPAND_SZ data, we change
            //  *pdwType to REG_SZ to reflect this fact.  This helps to avoid
            //  the situation where the caller could mistakenly re-expand it.
            if (!(dwFlags & RRF_NOEXPAND)) {
                *pdwType = REG_SZ;
            }
*/
        AssertM( !"We do not handle this case for simplicity." );
        return ERROR_UNSUPPORTED_TYPE;
            break;

        case REG_MULTI_SZ:
/*
            if (pcbData) {
                lr = NullTerminateRegMultiSzString(pvData, pcbData, cbDataBuffer, lr, Ansi);
            }
*/
        AssertM( !"We do not handle this case for simplicity." );
        return ERROR_UNSUPPORTED_TYPE;
            break;
    }

    return lr;
}


/*----------------------------------------------------------
Purpose: Helper for RegGetValueA().
*/
LONG
QueryValueInternalA(
    HKEY    hkey,
    PCSTR   pszValue,
    DWORD    dwFlags,
    DWORD * pdwType,
    void *  pvData,
    DWORD * pcbData)
{
    LONG  lr;
    DWORD dwType;
    DWORD cbData = 0;
    DWORD cbDataBuffer = pvData ? *pcbData : 0;

    AssertM(ERROR_SUCCESS == RestrictArguments(hkey, dwFlags, pvData, pcbData));


    if (!pdwType)
        pdwType = &dwType;

    if (!pcbData)
        pcbData = &cbData;

    lr = RegQueryValueExA(hkey, pszValue, NULL, pdwType, (LPBYTE)pvData, pcbData);
#ifdef BUILD_ENV_IS_NT
#pragma prefast(suppress:__WARNING_MISSING_ZERO_TERMINATION, "silence prefast.")
#endif
    lr = FixRegData(hkey, (PVOID)pszValue, dwFlags, pdwType, pvData, (pcbData != &cbData ? pcbData : NULL), cbDataBuffer, lr, TRUE);
    lr = RestrictRegType(dwFlags, *pdwType, *pcbData, lr);

    return lr;
}


/*----------------------------------------------------------
Purpose: Helper for RegGetValueA()/RegGetValueW().
*/
__inline void ZeroDataOnFailure(DWORD dwFlags, __in_bcount_opt(cbDataBuffer) void *pvData, DWORD cbDataBuffer, LONG lr)
{
    if ((lr != ERROR_SUCCESS) && (dwFlags & RRF_ZEROONFAILURE) && (cbDataBuffer > 0)) {
        AssertM( pvData );
        RtlZeroMemory(pvData, cbDataBuffer);
    }
}



// =============================================================================
LONG EsetestRegGetValueA(
    __in         HKEY hkey,
    __in_opt     LPCTSTR pszSubKey,
    __in_opt     LPCTSTR pszValue,
    __in_opt     DWORD dwFlags,
    __out_opt    LPDWORD pdwType,
    __out_opt    PVOID pvData,
    __inout_opt  LPDWORD pcbData
)
// =============================================================================
{
    LONG  lr;
    DWORD cbDataBuffer = pvData && pcbData ? *pcbData : 0;

    lr = RestrictArguments(hkey, dwFlags, pvData, pcbData);
    if (lr == ERROR_SUCCESS) {
        if (pszSubKey && *pszSubKey) {
            HKEY hkSubKey;

            lr = RegOpenKeyExA(hkey, pszSubKey, 0, KEY_QUERY_VALUE, &hkSubKey);
            if (lr == ERROR_SUCCESS) {
                lr = QueryValueInternalA(hkSubKey, pszValue, dwFlags, pdwType, pvData, pcbData);
                RegCloseKey(hkSubKey);
            }
        } else {
            lr = QueryValueInternalA(hkey, pszValue, dwFlags, pdwType, pvData, pcbData);
        }
    }

    ZeroDataOnFailure(dwFlags, pvData, cbDataBuffer, lr);

    return lr;
}


// The path under HKEY_LOCAL_MACHINE where ESE settings are stored
LOCAL const TCHAR tszEsentGlobalPath[]          =   _T( "SOFTWARE\\Microsoft\\" ESE_FLAVOUR "\\Global" );


JET_ERR
ErrEsetestWriteConfigToEsentGlobal(
    __in const TCHAR*   szKeyPath,
    __in const TCHAR*   szKeyName,
    __in const TCHAR*   szValue
    )
{
    const size_t    cchKeyFqPath    = MAX_PATH;
    TCHAR           szKeyFqPath[ cchKeyFqPath ];

    _sntprintf( szKeyFqPath, cchKeyFqPath, _T( "%s\\%s" ), tszEsentGlobalPath, szKeyPath );
    szKeyFqPath[ cchKeyFqPath - 1 ] = '\0';

    return ErrEsetestWriteRegistry( HKEY_LOCAL_MACHINE, szKeyFqPath, szKeyName, szValue );
}

JET_ERR
ErrEsetestWriteRegistry(
    __in const HKEY     hkeyHive,
    __in const TCHAR*       szKeyPath,
    __in const TCHAR*       szKeyName,
    __in const TCHAR*       szValue
    )
{
    JET_ERR err = JET_errSuccess;
    DWORD   disposition;
    HKEY    hkeyOperations  = NULL;
    LONG lRc;
    DWORD dwGle;

    lRc = RegCreateKeyEx(
        hkeyHive,
        szKeyPath,
        0,                          // reserved
        _T(""),                         // lpClass
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,                       // lpsa
        &hkeyOperations,
        &disposition                // REG_CREATED_NEW_KEY || REG_OPENED_EXISTING_KEY
        );
    if ( ERROR_SUCCESS != lRc ) {
        dwGle = GetLastError();
        printf( "RegCreateKeyEx failed with GLE = %d" SZNEWLINE, dwGle );
        err = dwGle;
        goto HandleError;
    }
    if ( REG_CREATED_NEW_KEY == disposition ) {
        if ( FEsetestVerboseInternalLogging() ) {
            printf( "created a new key" SZNEWLINE );
        }
    }
    else if ( REG_OPENED_EXISTING_KEY == disposition ) {
        if ( FEsetestVerboseInternalLogging() ) {
            printf( "did NOT create a new key" SZNEWLINE );
        }
    }
    else {
        printf( "Unknown disposition: %#x" SZNEWLINE, disposition );
    }

    if ( FEsetestVerboseInternalLogging() ) {
        printf( "setting regkey" SZNEWLINE );
    }
    lRc = RegSetValueEx(
        hkeyOperations,
        szKeyName,
        0,
        REG_SZ,
        (PBYTE const) szValue,
        (_tcslen( szValue ) + 1 ) * sizeof( szValue[0] )
        );
    if ( ERROR_SUCCESS != lRc ) {
        dwGle = GetLastError();
        printf( "RegSetValue failed with GLE = %d" SZNEWLINE, dwGle );
        err = dwGle;
        goto HandleError;
    }

    if ( FEsetestVerboseInternalLogging() ) {
        printf( "closing key" SZNEWLINE );
    }
    lRc = RegCloseKey( hkeyOperations );
    if ( ERROR_SUCCESS != lRc ) {
        dwGle = GetLastError();
        printf( "RegCloseKey failed with GLE = %d" SZNEWLINE, dwGle );
        err = dwGle;
        goto HandleError;
    }
HandleError:
    return err;
}

JET_ERR
ErrEsetestReadConfigToEsentGlobal(
    __in const TCHAR*   szKeyPath,
    __in const TCHAR*   szKeyName,
    __out_bcount_part( *pcbValue, *pcbValue ) TCHAR* const  szValue,        // Must point to valid buffer
    __inout PDWORD      pcbValue        // How big the buffer is going in; how many bytes written
    )
{
    const size_t    cchKeyFqPath    = MAX_PATH;
    TCHAR           szKeyFqPath[ cchKeyFqPath ];

    _sntprintf( szKeyFqPath, cchKeyFqPath, _T( "%s\\%s" ), tszEsentGlobalPath, szKeyPath );
    szKeyFqPath[ cchKeyFqPath - 1 ] = '\0';

    return ErrEsetestReadRegistry( HKEY_LOCAL_MACHINE, szKeyFqPath, szKeyName, szValue, pcbValue );

}

JET_ERR
ErrEsetestReadRegistry(
    __in const HKEY     hkeyHive,
    __in const TCHAR*   szKeyPath,
    __in const TCHAR*   szKeyName,
    __out TCHAR* const  szValue,        // Must point to valid buffer
    __inout PDWORD      pcbValue        // How big the buffer is going in; how many bytes written
    )
{
    JET_ERR err = JET_errSuccess;
    DWORD   disposition;
    HKEY    hkeyEsentGlobalOs   = NULL;
    LONG    lRc;
    DWORD dwGle;

    if ( !szValue ) {
        err = JET_errInvalidParameter;
        goto HandleError;
    }

    if ( 0 == pcbValue ) {
        err = JET_errInvalidParameter;
        goto HandleError;
    }

    lRc = RegCreateKeyEx(
        hkeyHive,
        szKeyPath,
        0,                          // reserved
        _T(""),                         // lpClass
        REG_OPTION_NON_VOLATILE,
        KEY_READ,
        NULL,                       // lpsa
        &hkeyEsentGlobalOs,
        &disposition                // REG_CREATED_NEW_KEY || REG_OPENED_EXISTING_KEY
        );
    if ( ERROR_SUCCESS != lRc ) {
        dwGle = GetLastError();
        tprintf( "RegCreateKeyEx failed with GLE = %d" SZNEWLINE, dwGle );
        err = dwGle;
        goto HandleError;
    }
    if ( REG_CREATED_NEW_KEY == disposition ) {
        if ( FEsetestVerboseInternalLogging() ) {
            tprintf( "created a new key" SZNEWLINE );
        }
    }
    else if ( REG_OPENED_EXISTING_KEY == disposition ) {
        if ( FEsetestVerboseInternalLogging() ) {
            tprintf( "did NOT create a new key" SZNEWLINE );
        }
    }
    else {
        tprintf( "Unknown disposition: %#x" SZNEWLINE, disposition );
    }

    DWORD   type;

    if ( FEsetestVerboseInternalLogging() ) {
        printf( "reading regkey" SZNEWLINE );
    }
    lRc = RegQueryValueEx(
        hkeyEsentGlobalOs,
        szKeyName,
        0,
        &type,
        (const PBYTE) szValue,
        pcbValue
        );
    if ( ERROR_SUCCESS != lRc ) {
        dwGle = GetLastError();
        // ERROR_ALREADY_EXISTS gets returned if the key exists, but the value does not.
        if ( ERROR_ALREADY_EXISTS != dwGle ) {
            tprintf( "RegQueryValueEx failed with GLE = %d" SZNEWLINE, dwGle );
            err = dwGle;
            goto HandleError;
        }
        else {
            err = JET_errBufferTooSmall;
            szValue[0] = _T('\0');
            *pcbValue = 0;
            goto HandleError;
        }
    }

    if ( REG_SZ != type ) {
        tprintf( "The registry key was not a REG_SZ!" SZNEWLINE );
        err = JET_errInvalidParameter;
        goto HandleError;
    }

HandleError:

    if ( NULL != hkeyEsentGlobalOs ) {
        if ( FEsetestVerboseInternalLogging() ) {
            tprintf( "closing key" SZNEWLINE );
        }
        lRc = RegCloseKey( hkeyEsentGlobalOs );
        if ( ERROR_SUCCESS != lRc ) {
            dwGle = GetLastError();
            tprintf( "RegCloseKey failed with GLE = %d" SZNEWLINE, dwGle );
            err = dwGle;
            goto HandleError;
        }
    }

    return err;
}

JET_ERR
ErrEsetestCopyKey(
    __in const WCHAR*   wszKeySrc,
    __in const WCHAR*   wszKeyDst,
    __in BOOL           fDelSrc,
    __in BOOL           fDelDst
)
{
    JET_ERR err     = JET_errSuccess;
    HKEY hKeySrc    = NULL;
    HKEY hKeyDst    = NULL;
    DWORD dwDisposition;
    LONG lError;

    // Try to open the source key.
    if( ERROR_SUCCESS != ( lError = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                                                    wszKeySrc,
                                                    0,
                                                    KEY_READ,
                                                    &hKeySrc ) ) ){     
        OutputError( "%s(): RegOpenKeyExW() failed with %ld!" CRLF, __FUNCTION__, lError );
        hKeySrc = NULL;
        err = GetLastError();
        goto Cleanup;
    }

    // Try to open the destination key.
    if( ERROR_SUCCESS != ( lError = RegCreateKeyExW( HKEY_LOCAL_MACHINE,
                                                        wszKeyDst,
                                                        0,
                                                        NULL,
                                                        REG_OPTION_NON_VOLATILE,
                                                        KEY_WRITE,
                                                        NULL,                                                   
                                                        &hKeyDst,
                                                        &dwDisposition ) ) ){
        OutputError( "%s(): RegCreateKeyExW() failed with %ld!" CRLF, __FUNCTION__, lError );
        hKeyDst = NULL;
        err = GetLastError();
        goto Cleanup;
    }

    // Should we delete the destination?
    if ( ( REG_OPENED_EXISTING_KEY == dwDisposition ) && fDelDst ){
        if( ERROR_SUCCESS != ( lError = RegDeleteTree( hKeyDst, NULL ) ) ){
            OutputError( "%s(): RegDeleteTree() failed with %ld!" CRLF, __FUNCTION__, lError );
            err = GetLastError();
            goto Cleanup;
        }
        if( ERROR_SUCCESS != ( lError = RegCloseKey( hKeyDst ) ) ){
            OutputError( "%s(): RegCloseKey() failed with %ld!" CRLF, __FUNCTION__, lError );
            err = GetLastError();
            goto Cleanup;
        }
        hKeyDst = NULL;
        if( ERROR_SUCCESS != ( lError = RegCreateKeyExW( HKEY_LOCAL_MACHINE,
                                                            wszKeyDst,
                                                            0,
                                                            NULL,
                                                            REG_OPTION_NON_VOLATILE,
                                                            KEY_WRITE,
                                                            NULL,                                                   
                                                            &hKeyDst,
                                                            &dwDisposition ) ) ){
            OutputError( "%s(): RegCreateKeyExW() failed with %ld!" CRLF, __FUNCTION__, lError );
            hKeyDst = NULL;
            err = GetLastError();
            goto Cleanup;
        }
    }   

    // Copy the whole tree.
    if( ERROR_SUCCESS != ( lError = RegCopyTreeW( hKeySrc, NULL, hKeyDst) ) ){
        OutputError( "%s(): RegCopyTreeW() failed with %ld!" CRLF, __FUNCTION__, lError );
        // We must be able to copy.
        goto Cleanup;
    }

    // Should we delete the source?
    if ( fDelSrc ){
        if( ERROR_SUCCESS != ( lError = RegDeleteTree( hKeySrc, NULL ) ) ){
            OutputError( "%s(): RegDeleteTree() failed with %ld!" CRLF, __FUNCTION__, lError );
            err = GetLastError();
            goto Cleanup;
        }
        if( ERROR_SUCCESS != ( lError = RegCloseKey( hKeySrc ) ) ){
            OutputError( "%s(): RegCloseKey() failed with %ld!" CRLF, __FUNCTION__, lError );
            err = GetLastError();
            goto Cleanup;
        }
        hKeyDst = NULL;
    }   

Cleanup:
    if ( NULL != hKeySrc )  RegCloseKey( hKeySrc );
    if ( NULL != hKeyDst )  RegCloseKey( hKeyDst );
    return err;
}

JET_ERR
ErrEnableSystemPibFailures(
    __in TCHAR* tszEnvVar
)
{
    return ErrEsetestWriteConfigToEsentGlobal( g_tszRegPathDebug, g_tszRegKeyPibFailures, tszEnvVar );
}

JET_ERR
ErrDisableSystemPibFailures(
)
{
    return ErrEsetestWriteConfigToEsentGlobal( g_tszRegPathDebug, g_tszRegKeyPibFailures, "0" );
}

PRIVATE
int
IEnvironmentVariableSet(
    __in const char* const  szEnvVariable
)
// IEnvironmentVariableSet() looks up the specified environment variable
// and returns the numeric value of it.
// Returns zero if it does not exist.
{
    int     iRet;
    const   size_t  cchBuffer   = 30;
    CHAR    tszEnvVarValue[ cchBuffer ];
    DWORD   dwRc;

    dwRc = GetEnvironmentVariableA( szEnvVariable, tszEnvVarValue, cchBuffer * sizeof( tszEnvVarValue[0] ) );
    if ( 0 == dwRc )
    {
        tprintf( "%%%s%% was not found in the environment." SZNEWLINE, szEnvVariable );
        iRet = 0;
    }
    else {
        iRet = _ttoi( tszEnvVarValue );
        if ( 0 == iRet )
        {
            tcprintf( ANSI_CYAN, "%%%s%% was equal to 0." SZNEWLINE, szEnvVariable );
        }
        else
        {
            tprintf( "%%%s%% (equal to [%s]) was found." SZNEWLINE, szEnvVariable, tszEnvVarValue );
        }
    }
    return iRet;
}

PRIVATE
bool
FEnvironmentVariableSet(
    __in const char* const  szEnvVariable
)
// FEnvironmentVariableSet() looks up the specified environment variable
// and returns true if the variable exists and is non-zero.
{
    bool    fRet;

    int iValue = IEnvironmentVariableSet( szEnvVariable );
    fRet = !!iValue;
    return fRet;
}

PRIVATE
bool
FEnvironmentVariableDefined(
    __in const char* const  szEnvVariable
)
// FEnvironmentVariableDefined() looks up the specified environment variable
// and returns true if the variable exists.
{
    const   size_t  cchBuffer   = 30;
    CHAR    tszEnvVarValue[ cchBuffer ];
    DWORD   dwRc;

    dwRc = GetEnvironmentVariableA( szEnvVariable, tszEnvVarValue, cchBuffer * sizeof( tszEnvVarValue[0] ) );
    if ( 0 == dwRc )
    {
        return false;
    }
    return true;
}

PRIVATE
VOID
CustomEseInitTest()
{
#ifdef ESE_FLAVOUR_IS_ESE
    bool    fSet;
    JET_ERR err;

    fSet = FEnvironmentVariableSet( g_szEnvironmentTopDown );
    if ( !fSet )
    {
        tprintf( "%%%s%% was either zero, or not found in the environment. Not enabling top-down allocation" SZNEWLINE, g_szEnvironmentTopDown );
    }
    else {
        tprintf( "%%%s%% was found. About to enable top-down allocation" SZNEWLINE, g_szEnvironmentTopDown );
        err = DynLoadJetSetResourceParam(
            NULL,   // all instances
            JET_resoperAllocTopDown,
            JET_residAll,           // Not JET_residMax
            1                       // TRUE
            );
        if ( JET_errSuccess != err ) {
            tcprintf( ANSI_RED, "Failed to set topdown allocation! JetSetResourceParam returned %d" SZNEWLINE, err );
        }
        else {
            tcprintf( ANSI_GREEN, "Success! Top-down allocation is going to happen from now on!" SZNEWLINE );
        }
    }
#endif
}


PRIVATE
VOID
CustomEsentInitTest()
{
#ifdef ESE_FLAVOUR_IS_ESENT
    if ( FEsetestCallSystemParametersBeforeInit() ) {
        JET_ERR             err;

        if ( FEnvironmentVariableSet( g_szEnvironmentSmallConfig ) )
        {
            tprintf( "Enabling small config." CRLF );
            // configSmall is 0
            err = DynLoadJetSetSystemParameter( NULL, JET_sesidNil, JET_paramConfiguration, 0, NULL );
            if ( err < JET_errSuccess )
            {
                tprintf( __FUNCTION__ "(): Failed to set small configuration (may be acceptable)." CRLF );
            }

            // Small Config disables perf counters by default. It's hard to measure its effects if
            // the perf counters can't be viewed!
            (void) DynLoadJetSetSystemParameter( NULL, JET_sesidNil, JET_paramDisablePerfmon, 0, NULL );
        }
        else
        {
            // configLegacy is 1
            (void) DynLoadJetSetSystemParameter( NULL, JET_sesidNil, JET_paramConfiguration, 1, NULL );
        }

        // Not needed for Exchange, but ESENT in Longhorn defaults to 'small configuration', which
        // reduces resource requirements, at the cost of performance
        // JET_paramConfiguration resets many parameters. So we need to re-enable JET_paramEnableAdvanced.
        (void) DynLoadJetSetSystemParameter( NULL, 0, JET_paramEnableAdvanced, 1, NULL );
    }

#endif
}

extern "C" {

PRIVATE
JET_ERR EsetestShadowLogData(
    __in    JET_INSTANCE        inst,
    __in    JET_EMITDATACTX *   pEmitLogDataCtx,
    __in    void *              pvLogData,
    __in    unsigned long           cbLogData,
    __in    void *              pvCallBackCtx )

{
    JET_PFNEMITLOGDATA pfn = EsetestShadowLogData;  // ensure this function matches callback defn
    JET_ERR err = JET_errSuccess;

    Unused( pfn );

    AssertM( g_fUsingShadowLog );
    AssertM( inst );

    if ( pEmitLogDataCtx->grbitOperationalFlags & JET_bitShadowLogEmitFirstCall )
        {
        g_fUsingShadowLog |= 0x2;   // cheap way to track we've seen this
        }

    if ( pEmitLogDataCtx->grbitOperationalFlags & JET_bitShadowLogEmitLastCall )
        {
        g_fUsingShadowLog |= 0x4;   // cheap way to track we've seen this
        }

    err = DynLoadJetConsumeLogData( inst, pEmitLogDataCtx, pvLogData, cbLogData, 0 );
    if ( err )  // short cut for this kind of thing...?
    {
        goto HandleError;
    }

HandleError:

    if ( err < JET_errSuccess )
        {
        // Not sure what to do with this.
        wprintf(L"Shadow Log Callback FAILURE, %d\n", err );
        }

    return err;
}

};  // extern "C"


PRIVATE
VOID
CustomEseAndEsentInitTest()
{
    const   size_t  cchBuffer   = 30;
    TCHAR   tszEnvVar[ cchBuffer ];
    int iVal;

    EsetestEseFlavour   flavourActual;
    UINT    verMajorActual;
    UINT    verMinorActual;
    UINT    verSpActual;
    UINT    verBuildActual;
    bool    fChecked;
    char    szEseVersion[ 10 ];

    JET_ERR err = EsetestGetEseVersionParts( &flavourActual, &verMajorActual, &verMinorActual, &verSpActual, &verBuildActual, &fChecked );
    if ( err >= JET_errSuccess )
    {
        switch ( flavourActual )
        {
            case EsetestEseFlavourEsent:
                strcpy( szEseVersion, "esent.dll" );
                break;
            case EsetestEseFlavourEse:
                strcpy( szEseVersion, "ese.dll" );
                break;
            default:
                strcpy( szEseVersion, "???.dll" );
        }
        tprintf( "Running %s, version %d.%d sp %d build %d [%s]" CRLF, szEseVersion, verMajorActual, verMinorActual, verSpActual, verBuildActual,
            fChecked ? "checked" : "free"
        );
    }
    else
    {
        tcprintf( ANSI_RED, "Could not detect version of ESE: EsetestGetEseVersionParts() returned %d." CRLF, err );
    }

    // Read environment var "ESETEST_PAGESIZE", if not present, leave it at default
    JET_API_PTR cbPageSize;
    cbPageSize = IEnvironmentVariableSet(g_szEnvironmentPagesize) * 1024;
    if(cbPageSize)
    {
        tprintf( "Setting page size to %dk" CRLF, cbPageSize/1024 );
        JET_ERR err = DynLoadJetSetSystemParameter(NULL, JET_sesidNil, JET_paramDatabasePageSize, cbPageSize, NULL );
        if ( err < JET_errSuccess )
        {
            tprintf( __FUNCTION__ "(): Failed to set page size, ErrorCode: %ld" CRLF, err);
            AssertM( err == JET_errSuccess );
        }
    }
    else
    {
        tprintf("Using the default page size of %dk\n", IEsetestGetPageSize());
    }

    iVal = IEnvironmentVariableSet( g_szSystemPibFailureEnvironment );
    if ( 0 == iVal ) {
        tcprintf( ANSI_CYAN, "Disabling PIB allocation failure (debug-only)" SZNEWLINE );
        err = ErrDisableSystemPibFailures();
        if ( JET_errSuccess != err ) {
            tprintf( "Did not set System PIB failure! ErrDisableSystemPibFailures returned %d" SZNEWLINE, err );
        }
        else {
            tcprintf( ANSI_GREEN, "Success! PIB allocation failure was successfully disabled!" SZNEWLINE );
        }
    }
    else {
        tprintf( "About to enable PIB allocation failure (debug-only)" SZNEWLINE );
        err = ErrEnableSystemPibFailures( tszEnvVar );
        if ( JET_errSuccess != err ) {
            tprintf( "Did not set System PIB failure! EnableSystemPibFailures returned %d" SZNEWLINE, err );
        }
        else {
            tcprintf( ANSI_GREEN, "Success! PIB allocation failure is going to happen from now on!" SZNEWLINE );
        }
    }

// Hmm, this did work before. It is in nturtl.h, but this is the only identifier I need from there.
#ifndef HEAP_MODE_LFH
#define HEAP_MODE_LFH                       2
#endif // HEAP_MODE_LFH

    // Enable LFH.
    // Note that the API is not in Win2000 RTM, but is in SP4.
    HANDLE  hHeap = GetProcessHeap();
    const ULONG             InfoValue               = HEAP_MODE_LFH;
    (void) HeapSetInformation( hHeap,
        HeapCompatibilityInformation,
        (void*)&InfoValue,
        sizeof( InfoValue ) );
    JET_API_PTR paramFileNames;

    // Try the new log file names.
    if ( FEsetestFeaturePresent( EseFeature2GLogs ) )
    {
        if ( FEnvironmentVariableSet( g_szEnvironmentLegacyLogNames ) )
        {
            tprintf( "%s was set -- Using the old log naming convention (.log not .jtx)." CRLF, g_szEnvironmentLegacyLogNames );
        }
        else
        {
            err =DynLoadJetGetSystemParameter( 0, 0, JET_paramLegacyFileNames, ( ULONG_PTR *)&paramFileNames, NULL, 0);
            if ( err != JET_errSuccess ) {
                tcprintf( ANSI_RED, "(JET_paramLegacyFileNames returned %d)" CRLF, err );
                paramFileNames = 0;
            }

            
            paramFileNames = paramFileNames & JET_bitEightDotThreeSoftCompat;
            err = DynLoadJetSetSystemParameter( NULL, JET_sesidNil, JET_paramLegacyFileNames, paramFileNames, NULL );
            if ( err < JET_errSuccess )
            {
                tprintf( __FUNCTION__ "(): Failed to enable the new log naming convention (.jtx)." CRLF );
            }
            else
            {
                tprintf( "Using the new log naming convention (.jtx not .log)." CRLF );
            }
        }
    }

    // for unicode file path. 
    // set the g_esetestconfig.m_pctWideApis to the environment var g_szEnvironmentPctWidenApis 
    // (if unicode file path is not supported, this value will be ignored.)
    if ( FEnvironmentVariableDefined( g_szEnvironmentPctWidenApis  ) )
    {
        EsetestSetWidenParametersPercent( IEnvironmentVariableSet( g_szEnvironmentPctWidenApis ) );
    }

    // Compression configuration.
    if ( FEnvironmentVariableDefined( g_szEnvironmentPctCompression ) ){
        EsetestSetCompressionPercent( IEnvironmentVariableSet( g_szEnvironmentPctCompression ) );
    }

    // Shadow log configuration
    if ( FEnvironmentVariableDefined( g_szEnvironmentShadowLog ) )
    {
        g_fUsingShadowLog = IEnvironmentVariableSet( g_szEnvironmentShadowLog );
        if ( g_fUsingShadowLog )
        {
            err = DynLoadJetSetSystemParameter( NULL, JET_sesidNil, JET_paramEmitLogDataCallback, (JET_API_PTR)EsetestShadowLogData, NULL );
            if ( err < JET_errSuccess ){
                tprintf( "Uh-oh we could not set our callback!\n" );
                g_fUsingShadowLog = false;
            }
        }
    }

    // Product asserts.
    JET_API_PTR assertaction;
    if ( FEnvironmentVariableDefined( g_szEnvironmentEseAssertAction ) )
    {
        assertaction = ( JET_API_PTR )IEnvironmentVariableSet( g_szEnvironmentEseAssertAction );
    }
    else{
        assertaction = JET_AssertFailFast;
    }
    err = DynLoadJetSetSystemParameter( NULL, JET_sesidNil, JET_paramAssertAction, assertaction, NULL );
    if ( err < JET_errSuccess )
    {
        tprintf( __FUNCTION__ "(): Failed to set JET_paramAssertAction." CRLF );
    }
    else
    {
        tprintf( "JET_paramAssertAction set to %#Ix." CRLF, assertaction );
    }

    // Test asserts.
    if ( FEnvironmentVariableDefined( g_szEnvironmentEsetestAssertAction ) )
    {
        assertaction = ( JET_API_PTR )IEnvironmentVariableSet( g_szEnvironmentEsetestAssertAction );
    }
    else{
        assertaction = JET_AssertSkippableMsgBox;
    }
    EsetestSetAssertAction( assertaction );

    JET_API_PTR exceptionaction;
    if ( FEnvironmentVariableSet( g_szEnvironmentExceptionAction ) )
    {
        exceptionaction = ( JET_API_PTR )IEnvironmentVariableSet( g_szEnvironmentExceptionAction );
    }
    else{
        exceptionaction = JET_ExceptionNone;
    }
    err = DynLoadJetSetSystemParameter( NULL, JET_sesidNil, JET_paramExceptionAction, exceptionaction, NULL );
    if ( err < JET_errSuccess )
    {
        tprintf( __FUNCTION__ "(): Failed to set JET_paramExceptionAction." CRLF );
    }
    else
    {
        tprintf( "JET_paramExceptionAction set to %#Ix." CRLF, exceptionaction );
    }

}

//////////////////////////////////////////////////////////////
int ChangeResult( int nSuccess )
//////////////////////////////////////////////////////////////
// Backs up in results.txt, and overwrites FAILED with PASSED
{
    int nReturn = FAILURE;

#ifdef BUILD_ENV_IS_NT
    HRESULT hr;

    if ( g_esetestconfig.m_fWttLog )
    {
        const BOOL  fPassed = ( nSuccess == SUCCESS );
        if ( fPassed )
        {
            hr = g_esetestconfig.pfnWTTLogTrace( g_esetestconfig.m_hdevWttLog, WTT_LVL_MSG,
                __FUNCTION__ "(): passed" CRLF
            );
        }
        else
        {
            hr = g_esetestconfig.pfnWTTLogTrace( g_esetestconfig.m_hdevWttLog, WTT_LVL_ERR, fPassed, WTT_ERROR_TYPE_BOOL,
                __WFILE__,  // file
                __LINE__,       // line #
                __FUNCTION__
            );
        }
        if ( SUCCEEDED( hr ) )
        {
            nReturn = SUCCESS;
        }
        else
        {
            tprintf( "%s(): Failed to pfnWTTLogTrace, hr = %#x" CRLF, __FUNCTION__, hr );
            nReturn = FAILURE;
        }
        goto LError;
    }
#endif //BUILD_ENV_IS_NT

    if ( g_fEnableWritingToResultsTxt ) {
        DWORD cbWritten = 0;
        HANDLE hFile = INVALID_HANDLE_VALUE;
        for ( int i =0 ; i < 10 ; i++ ){
            hFile = CreateFile(
                RESULTS_TXT,
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL );
            if ( INVALID_HANDLE_VALUE != hFile ) break;
            Sleep( 1000 );
        }

        AssertM( INVALID_HANDLE_VALUE != hFile );

        if ( INVALID_HANDLE_VALUE == hFile ||
            INVALID_SET_FILE_POINTER == SetFilePointer( hFile, -( LONG )strlen( FAIL_STR ), NULL, FILE_END ) ||
            FALSE == WriteFile( hFile, PASS_STR, ( DWORD )strlen( PASS_STR ), &cbWritten, NULL) ) {
            tprintf( "Failed to write PASSED to %s" SZNEWLINE, RESULTS_TXT );
            NTError();
            goto LError;
        }
        VerifyP( FALSE != CloseHandleP( &hFile ) );
    }

    nReturn = SUCCESS;
LError:
    return nReturn;
}


//////////////////////////////////////////////////////////////
//  TermTest
//////////////////////////////////////////////////////////////
int TermTest(int nSuccess)
{
    return TermTestMaybeChangeResult( nSuccess, TRUE );
}

//////////////////////////////////////////////////////////////
//  TermTestMaybeChangeResult
//////////////////////////////////////////////////////////////
int TermTestMaybeChangeResult( int nSuccess, BOOL fChangeResult )
{
    int nReturn = FAILURE;

    // Check for assert.txt and pass the test if not found
    DWORD dwFileAttr = GetFileAttributes( ASSERT_TXT );
    const bool  fAssertTxtFound = ( INVALID_FILE_ATTRIBUTES == dwFileAttr ) ? false : true;

#ifdef BUILD_ENV_IS_NT
    EsetestConfig* const    pcfg = &g_esetestconfig;
    if ( pcfg->m_fWttLog )
    {
        HRESULT     hr;
        const DWORD     dwWttResultCode = ( SUCCESS == nSuccess ) ? WTT_TESTCASE_RESULT_PASS : WTT_TESTCASE_RESULT_FAIL;

        AssertM( pcfg->pfnWTTLogStartTest );
        AssertM( pcfg->m_hdevWttLog );

        // It is not allowed to have a NULL wszTestName. Use
        // GetCommandeLineW() for both WTTLogStartTest and WTTLogEndTest().
        hr = pcfg->pfnWTTLogEndTest( pcfg->m_hdevWttLog, GetCommandLineW(), dwWttResultCode, NULL );

        if ( FAILED( hr ) )
        {
            tprintf( "Failed to pfnWTTLogEndTest(), hr = %#x" CRLF, hr );
        }
        goto LError;
    }
    else
#endif // BUILD_ENV_IS_NT
    {
        if ( fAssertTxtFound ) {
            tprintf( "%s file was found. %s", ASSERT_TXT, FAIL_STR );

            // Append the assert.txt contents to the logfile
            HANDLE hfileLog = INVALID_HANDLE_VALUE;
            for ( int i =0 ; i < 10 ; i++ ){
                hfileLog = CreateFile( RESULTS_LOG,
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_ALWAYS,
                    0,
                    NULL );
                if ( INVALID_HANDLE_VALUE != hfileLog ) break;
                Sleep( 1000 );
            }

            AssertM( INVALID_HANDLE_VALUE != hfileLog );
            
            if ( INVALID_HANDLE_VALUE == hfileLog ) {
                printf( "Failed to open %s" SZNEWLINE, RESULTS_LOG );
                NTError( FALSE );
                goto LError;
            }

            if ( INVALID_SET_FILE_POINTER == SetFilePointer( hfileLog, 0, NULL, FILE_END ) ) {
                printf( "Failed to append to %s" SZNEWLINE, RESULTS_LOG );
                NTError( FALSE );

                VerifyP( FALSE != CloseHandleP( &hfileLog ) );
                goto LError;
            }

            HANDLE hFileAssertTxt = INVALID_HANDLE_VALUE;
            for ( int i =0 ; i < 10 ; i++ ){
                hFileAssertTxt = CreateFile( ASSERT_TXT,
                                                GENERIC_READ,
                                                0,
                                                NULL,
                                                OPEN_EXISTING,
                                                FILE_ATTRIBUTE_NORMAL,
                                                NULL );
                if ( INVALID_HANDLE_VALUE != hFileAssertTxt ) break;
                Sleep( 1000 );
            }

            AssertM( INVALID_HANDLE_VALUE != hFileAssertTxt );

            if ( INVALID_HANDLE_VALUE != hFileAssertTxt ) {
                AppendToFileHandleFromFileHandle( hFileAssertTxt, hfileLog );

                VerifyP( FALSE != CloseHandleP( &hFileAssertTxt ) );
            }

            // Close the file and we are done
            VerifyP( FALSE != CloseHandleP( &hfileLog ) );
            goto LError;
        }

        printf("No %s file was found" SZNEWLINE, ASSERT_TXT);

        // Now pass the test if possible
        if ( FAILURE == nSuccess ) {
            goto LError;
        }

        nReturn = fChangeResult ? ChangeResult( nSuccess ) : SUCCESS;
    }

    /*
    if ( g_fUsingShadowLog && fChangeResult )
        {
        if ( !( g_fUsingShadowLog & 0x2 ) ||
                !( g_fUsingShadowLog & 0x4 ) )
            {
            //  We didn't register both the initial and terminating callbacks...
            //printf("\n\n\n\t\tSOMEONETODO\n\t\tSOMEONETODO\n\t\tSOMEONETODO\n\t\tSOMEONETODO\n\t\tSOMEONETODO HELLLO!? \n\t\tSOMEONETODO\n\t\tSOMEONETODO\n\t\tSOMEONETODO\n");
            nReturn = ChangeResult( ERROR_INVALID_PARAMETER );
            }
        else
            {
            //printf("\n\n\n\t\tSOMEONETODO\n\t\tSOMEONETODO\n\t\tSOMEONETODO\n\t\tSOMEONETODO\n\t\tSOMEONETODO GOODBYE!!!! \n\t\tSOMEONETODO\n\t\tSOMEONETODO\n\t\tSOMEONETODO\n");
            }
        }
    //printf("\n\n\n\t\tSOMEONETODO\n\t\tSOMEONETODO\n\t\tSOMEONETODO\n\t\tSOMEONETODO\n\t\tSOMEONETODO ummm any salutations!? \n\t\tSOMEONETODO\n\t\tSOMEONETODO\n\t\tSOMEONETODO\n");
    */

LError:
    TermLogging();
    return nReturn;
}

///////////////////////////////////////////////////////////
//  Copy File
///////////////////////////////////////////////////////////
int AppendToFileHandleFromFileHandle( HANDLE hfileSrc, HANDLE hfileDest )
{
    DWORD dwRead = 0;
    static CHAR rgbBuff[ 4096 ];

    AssertM( NULL != hfileSrc && INVALID_HANDLE_VALUE != hfileSrc );
    AssertM( NULL != hfileDest && INVALID_HANDLE_VALUE != hfileDest );

    do {
        if ( FALSE == ReadFile( hfileSrc, rgbBuff, sizeof( rgbBuff ), &dwRead, NULL) ) {
            printf( "Failed to read data" SZNEWLINE );
            NTError( FALSE );
            return FALSE;
        }

        const DWORD dwToWrite = dwRead;
        DWORD dwWritten = 0;
        if ( FALSE == WriteFile( hfileDest, rgbBuff, dwToWrite, &dwWritten, NULL ) ||
            dwToWrite != dwWritten ) {
            printf( "Failed to write data" SZNEWLINE );
            NTError( FALSE );
            return FALSE;
        }
    } while ( sizeof( rgbBuff ) == dwRead );

    return SUCCESS;
}

///////////////////////////////////////////////////////////
//  Memory string manipulation.
///////////////////////////////////////////////////////////
PTCHAR ParseResultsMemory(  const PTCHAR szSrc,
                                const PTCHAR szToken,
                                const PTCHAR szTokenBefore,
                                const PTCHAR szTokenAfter ){
    PTCHAR  szTokenBegin    = NULL;
    PTCHAR  szTokenEnd      = NULL;
    PTCHAR  szTokenSearch   = NULL; 
    PTCHAR  szResult        = NULL; 

    AssertM( NULL != szSrc );
    AssertM( NULL != szToken );
    AssertM( NULL != szTokenBefore );
    AssertM( NULL != szTokenAfter );

    if ( ( szTokenSearch = _tcsstr( szSrc, szToken ) ) != NULL){
        szTokenSearch +=  _tcsclen( szToken );
        if ( ( szTokenSearch = _tcsstr( szTokenSearch, szTokenBefore ) ) != NULL){
            szTokenBegin = szTokenSearch + _tcsclen( szTokenBefore );
            if ( *szTokenAfter ){
                if ( *szTokenBegin ){
                    szTokenEnd = _tcsstr( szTokenBegin + 1, szTokenAfter );
                }
                else{
                    szTokenEnd = szTokenBegin;
                }
            }
            else{
                szTokenEnd = szTokenBegin + _tcsclen( szTokenBegin );
            }

            if ( szTokenEnd ){
                if ( ( szResult = new TCHAR[ szTokenEnd - szTokenBegin + 1] ) != NULL ){
                    _tcsncpy( szResult, szTokenBegin, szTokenEnd - szTokenBegin );
                    *( szResult + ( szTokenEnd - szTokenBegin ) ) = 0;
                }
            }       
        }
    }

    return szResult;
}

///////////////////////////////////////////////////////////
//  File string manipulation.
///////////////////////////////////////////////////////////
PTCHAR ParseResultsFile(    const PTCHAR szFileName,
                            const PTCHAR szToken,
                            const PTCHAR szTokenBefore,
                            const PTCHAR szTokenAfter ){
    HANDLE  hFile           = INVALID_HANDLE_VALUE; 
    PTCHAR  lpBuffer            = NULL;
    DWORD   dwFileSize;
    PTCHAR  szResult            = NULL; 

    AssertM( NULL != szFileName );
    AssertM( NULL != szToken );
    AssertM( NULL != szTokenBefore );
    AssertM( NULL != szTokenAfter );
    
    hFile = CreateFile(     szFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL );
    if ( INVALID_HANDLE_VALUE == hFile ) goto Cleanup;
    dwFileSize = GetFileSize( hFile, NULL );
    if ( dwFileSize ){
        lpBuffer = new TCHAR[ dwFileSize/sizeof( TCHAR ) + 1 ]; 
        AssertM( NULL != lpBuffer );
        DWORD   dwBytesRead;
        const BOOL fReadFile = ReadFile( hFile, lpBuffer, dwFileSize, &dwBytesRead, NULL );
        AssertM( 0 != fReadFile );
        *(lpBuffer + dwFileSize/sizeof( TCHAR ) ) = 0;
        szResult = ParseResultsMemory( lpBuffer, szToken, szTokenBefore, szTokenAfter );
        delete []lpBuffer;
    }

    const BOOL fCloseHandle = CloseHandle( hFile );
    AssertM( 0 != fCloseHandle );

Cleanup:
    return szResult;    
}

//================================================================
// Mempry helper functions.
//================================================================
//
///////////////////////////////////////////////////////////
//  Frees memory.
///////////////////////////////////////////////////////////
void FreeParsedStr( PTCHAR szPtr ){
    if ( szPtr ){
        delete []szPtr;
        szPtr = NULL;
    }   
}

BYTE* EsetestCircularMemCopy( BYTE* pbDst, size_t cbDst, const BYTE* pbSrc, size_t cbSrc, size_t iSrcOffset ){
    size_t cbCopied, cbToCopy;
    cbCopied    = 0;
    cbToCopy;
    while ( cbCopied < cbDst ){
        iSrcOffset %= cbSrc;
        cbToCopy = min( cbDst - cbCopied, cbSrc - iSrcOffset );
        memcpy( pbDst + cbCopied, pbSrc + iSrcOffset, cbToCopy );
        iSrcOffset += cbToCopy;
        cbCopied += cbToCopy;
    }
    AssertM( cbCopied == cbDst );

    return pbDst;
}


///////////////////////////////////////////////////////////////
//  Fill Random
////////////////////////////////////////////////////////////////
int FillRandom( BYTE* pbData, ULONG cbData)
{
    ULONG   cbCurrent   = 0;

    if ( cbData > 3 )
    {
        for ( cbCurrent = 0 ; cbCurrent < ( cbData-( sizeof( LONG ) - 1 ) ) ; cbCurrent += 4 )
        {
            *( ULONG * )( pbData + cbCurrent ) = Rand( );
        }
    }
    for ( ; cbCurrent < cbData ; cbCurrent++ )
    {
        pbData[ cbCurrent ] = ( BYTE )Rand( );
    }

    return SUCCESS;
}

int FillRandomUnaligned( BYTE *pbData, ULONG cbData )
{
#pragma warning(disable : 4302) // 'type cast': truncation from 'BYTE *' to 'char'
    for ( ; ( ( char )pbData & 3 ) && cbData; *pbData++ = ( BYTE )Rand( ), cbData-- );
#pragma warning(default : 4302)

    return cbData && FillRandom( pbData, cbData );
}

///////////////////////////////////////////////////////////////
// Random
////////////////////////////////////////////////////////////////

/*
** Trying to force serialization of rand()
** StandardRand() returns a number in the range of
** 0-(2^31-2). The implementation is straight off
** the Standard Random number generator proposal
** in:
** "Random Number Generators : Good Ones are Hard to Find"
** Stephen K. Park and Keith W. Miller, CACM v31.No10 pp1195
**
** Verified for z(10001)=1043618065 (-1, due to our normalization)
*/

// This is Thread-local
_declspec( thread ) static long gStandardRandSeed = 1;

long GetStandardSeed( void )
{
    return gStandardRandSeed;
}

void SeedStandardRand(long seed)
{
    if ( seed != 0 )
        gStandardRandSeed = seed;
    else
        gStandardRandSeed = 1;
}

long StandardRand(long &lCurrentSeed)
{
    const long a = 16807;
    const long m = 2147483647;
    const long q = 127773;
    const long r = 2836;

    long hi = lCurrentSeed / q;
    long lo = lCurrentSeed % q;
    long test = a * lo - r * hi;

    lCurrentSeed = 0 < test ? test : test + m;
    
    return lCurrentSeed - 1;
}

long Rand( void )
{
    return StandardRand( gStandardRandSeed );
}

long RandRange( long min, long max )
{
    // Shouldn't happen, but just in case...
    if ( max < min ){
        const long temp = max;
        max = min;
        min = temp;
    }

    // The delta can overflow, so it can still be negative.
    if ( ( max - min ) == -1  )
    {
        return Rand();
    }
    else
    {
        return( min + ( long )( ( ( unsigned )Rand() ) % ( ( unsigned )( max - min + 1 ) ) ) );
    }
}

__int64 Rand64( void )
{
    return ( ( ( ( __int64 )Rand() ) << ( 8 * sizeof( long ) ) ) | ( ( __int64 )Rand() ) );
}

__int64 Rand64Range( __int64 min, __int64 max )
{
    // Shouldn't happen, but just in case...
    if ( max < min ){
        const __int64 temp = max;
        max = min;
        min = temp;
    }

    // The delta can overflow, so it can still be negative.
    if ( ( max - min ) == -1  )
    {
        return Rand64();
    }
    else
    {
        return( min + ( __int64 )( ( ( unsigned __int64 )Rand() ) % ( ( unsigned __int64 )( max - min + 1 ) ) ) );
    }
}

///////////////////////////////////////////////////////////////////////
//  Column free memory
///////////////////////////////////////////////////////////////////////
void FreeColumnData(JET_SETCOLUMN* pjsetcolumn, LONG ccolumns)
{
    long    i = 0;

    AssertM ( NULL != pjsetcolumn );
    for ( i = 0 ; i < ccolumns ; i++ )
    {
        AssertM ( NULL != pjsetcolumn[ i ].pvData );
        delete [ ]pjsetcolumn[ i ].pvData;
    }
    delete[ ] pjsetcolumn;
}

void FreeRetrieveColumn(JET_RETRIEVECOLUMN* pjretrievecolumn, LONG ccolumns)
{
    long    i = 0;

    AssertM ( NULL != pjretrievecolumn );
    for ( i = 0 ; i < ccolumns ; i++ )
    {
        AssertM ( NULL != pjretrievecolumn[ i ].pvData );
        delete [ ]pjretrievecolumn[i].pvData;
    }
    delete[ ] pjretrievecolumn;
}

BOOL IsColumnDataEqual(JET_SETCOLUMN* pjsetcolumn, JET_RETRIEVECOLUMN *pjretrievecolumn, ULONG ccolumns)
{
    ULONG i = 0;

    AssertM ( NULL != pjretrievecolumn );
    AssertM ( NULL != pjsetcolumn );

    // Compare the data column by column
    for ( i = 0 ; i < ccolumns ; i++ )
    {
        if ( pjretrievecolumn[ i ].cbActual != pjsetcolumn[ i ].cbData )
        {
            return FALSE;
        }
        if ( memcmp( pjretrievecolumn[ i ].pvData,
            pjsetcolumn[ i ].pvData,
            pjsetcolumn[ i ].cbData )  != 0 )
        {
            return FALSE;
        }
    }

    return TRUE;
}

// Sometimes the errors do not come from ESE, so the
// 'Error Trap' does not help. Or it's a FRE build, which
// does not have 'error traps.
void
BreakIfUserDesires()
{
    if ( g_fEsetestBreakOnCall )
    {
        VerifyP( 0 && "User wants to break in via g_fEsetestBreakOnCall." );
    }
}

// Has 'expected' as a parameter -- the other ReportErr() does not
void ReportErr( long err, long expected, unsigned long ulLine, const char *szFileName, const char *szFuncCalled )
{
    char name[ 256 ] = "Unknown error/warning";
    JET_ERR err2;

    if ( JET_errSuccess == expected ) {
        tcprintf( ANSI_RED, "Call to %s failed with error %d" SZNEWLINE, szFuncCalled, err );
        err2 =DynLoadJetGetSystemParameter( 0, 0, JET_paramErrorToString, ( ULONG_PTR *)&err, name, sizeof( name ));
        if ( err2 != JET_errSuccess ) {
            tcprintf( ANSI_RED, "(JET_paramErrorToString returned %d)" CRLF, err2 );
        }
        tcprintf( ANSI_RED, "( %s )" SZNEWLINE, name );
    }
    else {
        tcprintf( ANSI_RED, "Call to %s failed with error %d (expected %d)" SZNEWLINE, szFuncCalled, err, expected );
        err2 = DynLoadJetGetSystemParameter( 0, 0, JET_paramErrorToString, (ULONG_PTR *)&err, name, sizeof( name ));
        if ( err2 != JET_errSuccess ) {
            tcprintf( ANSI_RED, "(JET_paramErrorToString returned %d)" CRLF, err2 );
        }
        tcprintf( ANSI_RED, "Actual: ( %s )" SZNEWLINE, name );
        err2 = DynLoadJetGetSystemParameter( 0, 0, JET_paramErrorToString, (ULONG_PTR *)&expected, name, sizeof( name ));
        if ( err2 != JET_errSuccess ) {
            tcprintf( ANSI_RED, "(JET_paramErrorToString returned %d)" CRLF, err2 );
        }
        tcprintf( ANSI_RED, "Expected: ( %s )" SZNEWLINE, name );
    }
    tcprintf( ANSI_CYAN, "[%s, %d]" SZNEWLINE, szFileName, ulLine );

    BreakIfUserDesires();
}

int ReportWarn( long warn, long, unsigned long ulLine, const char *szFileName, const char *szFuncCalled )
{
    tcprintf( ANSI_CYAN, "WARNING: %s [%s, %d] returned %ld" SZNEWLINE, szFuncCalled, szFileName, ulLine, warn );

    char name[ 256 ] = "Unknown error/warning";
    DynLoadJetGetSystemParameter( 0, 0, JET_paramErrorToString, ( ULONG_PTR *)&warn, name, sizeof( name ) );
    tcprintf( ANSI_CYAN, "( %s )" SZNEWLINE, name );
    BreakIfUserDesires();
    return 0;
}

int ReportExpectedErr( JET_ERR expected )
{
    char name[ 256 ] = "Unknown error/warning";
    DynLoadJetGetSystemParameter( 0, 0, JET_paramErrorToString, ( ULONG_PTR *)&expected, name, sizeof( name ) );
    tcprintf( ANSI_YELLOW, "Expecting %ld, ( %s )" SZNEWLINE, ( long )expected, name );
    return 0;
}

int ReportReceivedErr( JET_ERR received )
{
    char name[ 256 ] = "Unknown error/warning";
    DynLoadJetGetSystemParameter( 0, 0, JET_paramErrorToString, ( ULONG_PTR *)&received, name, sizeof( name ) );
    tcprintf( ANSI_GREEN, "Received %ld, ( %s )" SZNEWLINE, ( long )received, name );
    BreakIfUserDesires();
    return 0;
}

#if 0
// does NOT Have 'expected' as a parameter -- the other ReportErr() does
int ReportErr( JET_ERR err, ULONG ulLine, const char *szFileName,  const char *szFuncCalled )
{
    char errName[ 256 ];
    strcpy( errName, "Unknown Error" );

    JET_ERR err2;
    err2 = DynLoadJetGetSystemParameter( 0, 0, JET_paramErrorToString, ( ULONG_PTR *)&err, errName, sizeof( errName ) );
    tcprintf( ANSI_RED, "Failed while calling %s . It generated error %d" SZNEWLINE, szFuncCalled, err );
    tcprintf( ANSI_RED, "( %s )" SZNEWLINE, errName );
    if ( err2 != JET_errSuccess ) {
        tcprintf( ANSI_RED, "(JET_paramErrorToString returned %d)" CRLF, err2 );
    }
    tcprintf( ANSI_CYAN, "[%s line %ld]" SZNEWLINE, szFileName, ulLine );

    BreakIfUserDesires();
    return 0;
}
#endif

//  ================================================================
// This is stolen from DBUTLSprintHex().
// The resultant buffer ends with a newline.
VOID SprintHex(
    __out CHAR * const      szDest,
    __in_bcount( cbSrc ) const BYTE * const rgbSrc,
    __in const INT          cbSrc,
    __in const INT          cbWidth     /* = 16 */,
    __in const INT          cbChunk     /* = 4 */,
    __in const INT          cbAddress   /* = 8 */,
    __in const INT          cbStart     /* = 0 */
)
//  ================================================================
{
    static const CHAR rgchConvert[ ] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

    const BYTE * const pbMax = rgbSrc + cbSrc;
    const INT cchHexWidth = ( cbWidth * 2 ) + (  cbWidth / cbChunk );
    const BYTE * pb = rgbSrc;

    CHAR * sz = szDest;
    while( pbMax != pb )
    {
        sz += ( 0 == cbAddress ) ? 0 : sprintf( sz, "%*.*lx    ", cbAddress, cbAddress, pb - rgbSrc + cbStart );
        CHAR * szHex = sz;
        CHAR * szText = sz + cchHexWidth;
        do
        {
            for( INT cb = 0; cbChunk > cb && pbMax != pb; ++cb, ++pb )
            {
                *szHex++    = rgchConvert[ *pb >> 4 ];
                *szHex++    = rgchConvert[ *pb & 0x0F ];
                *szText++   = isprint( *pb ) ? *pb : '.';
            }
            *szHex++ = ' ';
        } while( ( ( pb - rgbSrc ) % cbWidth ) && pbMax > pb );

        while( szHex != sz + cchHexWidth )
        {
            *szHex++ = ' ';
        }
        *szText++ = '\r';
        *szText++ = '\n';
        *szText = '\0';
        sz = szText;
    }
}

//================================================================
// Synchronization helper functions.
//================================================================
//
// =============================================================================
void EsetestWaitForAllObjectsInfinitely(
    __in DWORD cCount,
    __in_ecount( cCount ) const HANDLE* rgHandles
)
// =============================================================================
{
    while( cCount > 0 ){
        DWORD j = ( cCount <= MAXIMUM_WAIT_OBJECTS) ? cCount : MAXIMUM_WAIT_OBJECTS;
        WaitForMultipleObjectsEx( j, rgHandles, TRUE, INFINITE, FALSE);
        cCount -= j;
        rgHandles += j;
    }
}

//================================================================
// Directory manipulation helper functions.
//================================================================
//
BOOL
EsetestCreateDirectoryA(
    __in PCSTR      szName
)
{
    WCHAR* wszName      = EsetestWidenString( __FUNCTION__, szName );
    BOOL fReturn        = EsetestCreateDirectoryW( wszName );
    delete []wszName;

    return fReturn;
}

BOOL
EsetestCreateDirectoryW(
    __in PCWSTR     wszName
)
{
    const wchar_t* wcLastIndex;
    WCHAR wszParentFolder[ MAX_PATH + 1 ];
    size_t cchSubdir = 0;
    
    // If the path doesn't exist
    if ( !PathFileExistsW( wszName ) ){
        // Make sure that the parent directory is created
        wcLastIndex = wcsrchr( wszName, '\\' );
        
        // If the path does not have \ then we just try to create it
        if( wcLastIndex == NULL ){
            return CreateDirectoryW( wszName, NULL );   
        }
        
        cchSubdir = ( int )( wcLastIndex - wszName );
        wcsncpy_s( wszParentFolder, MAX_PATH + 1, wszName, cchSubdir );
        
        // Call recursively using the parent directory
        if(EsetestCreateDirectoryW( wszParentFolder ))
        {
            if ( !PathFileExistsW( wszName ) ){
                // Create the last directory
                return CreateDirectoryW( wszName, NULL );           
            }
            else
            {
                return TRUE;
            }
        }
        else
        {
            // We could not create the parent directory
            return FALSE;
        }
    }
    else{
        // Directory exists
        return TRUE;
    }
}

BOOL
EsetestRemoveDirectoryA(
    __in PCSTR      szName
)
{
    WCHAR* wszName      = EsetestWidenString( __FUNCTION__, szName );
    BOOL fReturn        = EsetestRemoveDirectoryW( wszName );
    delete []wszName;

    return fReturn;
}

BOOL
EsetestRemoveDirectoryW(
    __in PCWSTR     wszName
)
{
    BOOL fReturn = TRUE;
    WCHAR wszLongName[ MAX_PATH ];
    WCHAR wszLongNameSearch[ MAX_PATH ];
    WCHAR wszLongNameAppended[ MAX_PATH ];
    WIN32_FIND_DATAW fdFileInfo;

    // Fully-qualified path.
    if ( GetFullPathNameW( wszName, _countof( wszLongName ), wszLongName, NULL ) == 0 ){
        fReturn = FALSE;
    }

    // If the directory does not exist, assume we succeeded in deleting it.
    if ( !PathFileExistsW( wszLongName ) ){
        return TRUE;
    }

    // Search string.
    StringCchPrintfW( wszLongNameSearch,
                        _countof( wszLongNameSearch ),
                        L"%s\\*",
                        wszLongName );

    // First file/subdir.
    HANDLE hSearch = FindFirstFileW( wszLongNameSearch, &fdFileInfo );
    if ( hSearch == INVALID_HANDLE_VALUE ){
        // Error or didn't find any files?
        if ( GetLastError() != ERROR_NO_MORE_FILES ){
            fReturn = FALSE;
        }
    }
    else{
        // Rest of the files.
        do{
            // FindFile returns "." and ".." as well.
            if ( ( fdFileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
                    ( !wcscmp( fdFileInfo.cFileName, L"." ) || !wcscmp( fdFileInfo.cFileName, L".." ) ) ){
                continue;
            }
            
            // Append to the current path.
            StringCchPrintfW( wszLongNameAppended,
                            _countof( wszLongNameAppended ),
                            L"%s\\%s",
                            wszLongName,
                            fdFileInfo.cFileName );
            
            // File or subdir?
            if ( fdFileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
                if ( !EsetestRemoveDirectoryW( wszLongNameAppended ) ){
                    fReturn = FALSE;
                }
            }
            else{
                if ( !DeleteFileW( wszLongNameAppended ) ){
                    fReturn = FALSE;
                }
            }
        } while ( FindNextFileW( hSearch, &fdFileInfo ) );
        // Error or didn't find any files?
        if ( GetLastError() != ERROR_NO_MORE_FILES ){
            fReturn = FALSE;
        }
        FindClose( hSearch );
    }

    // At this point, the directory should be empty.
    if ( !RemoveDirectoryW( wszLongName ) ){
        fReturn = FALSE;
    }

    // The directory can linger on after the delete, if there are handles open to it in other processes (e.g. explorer)
    while ( PathFileExistsW( wszLongName ) ){
        Sleep(100);
    }
    return fReturn;
}

BOOL
EsetestCopyDirectoryA(
    __in PCSTR      szNameSrc,
    __in PCSTR      szNameDst
)
{
    WCHAR* wszNameSrc   = EsetestWidenString( __FUNCTION__, szNameSrc );
    WCHAR* wszNameDst   = EsetestWidenString( __FUNCTION__, szNameDst );
    BOOL fReturn        = EsetestCopyDirectoryW( wszNameSrc, wszNameDst );
    delete []wszNameSrc;
    delete []wszNameDst;

    return fReturn;
}

BOOL
EsetestCopyDirectoryW(
    __in PCWSTR     wszNameSrc,
    __in PCWSTR     wszNameDst
)
{
    BOOL fReturn = TRUE;
    WCHAR wszLongNameSrc[ MAX_PATH ];
    WCHAR wszLongNameDst[ MAX_PATH ];
    WCHAR wszLongNameSearch[ MAX_PATH ];
    WCHAR wszLongNameSrcAppended[ MAX_PATH ];
    WCHAR wszLongNameDstAppended[ MAX_PATH ];
    WIN32_FIND_DATAW fdFileInfo;

    // Fully-qualified source path.
    if ( GetFullPathNameW( wszNameSrc, _countof( wszLongNameSrc ), wszLongNameSrc, NULL ) == 0 ){
        fReturn = FALSE;
    }

    // If the directory does not exist, promptly fail.
    if ( !PathFileExistsW( wszLongNameSrc ) ){
        return FALSE;
    }

    // Fully-qualified destination path.
    if ( GetFullPathNameW( wszNameDst, _countof( wszLongNameDst ), wszLongNameDst, NULL ) == 0 ){
        fReturn = FALSE;
    }

    // If the directory does not exist, create it.
    if ( !PathFileExistsW( wszLongNameDst ) && !EsetestEnsureFullPathExistsW( wszLongNameDst ) ){
        return FALSE;
    }

    // Search string.
    StringCchPrintfW( wszLongNameSearch,
                        _countof( wszLongNameSearch ),
                        L"%s\\*",
                        wszLongNameSrc );

    // First file/subdir.
    HANDLE hSearch = FindFirstFileW( wszLongNameSearch, &fdFileInfo );
    if ( hSearch == INVALID_HANDLE_VALUE ){
        // Error or didn't find any files?
        if ( GetLastError() != ERROR_NO_MORE_FILES ){
            fReturn = FALSE;
        }
    }
    else{
        // Rest of the files.
        do{
            // FindFile returns "." and ".." as well.
            if ( ( fdFileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
                    ( !wcscmp( fdFileInfo.cFileName, L"." ) || !wcscmp( fdFileInfo.cFileName, L".." ) ) ){
                continue;
            }
            
            // Append to the current paths.
            StringCchPrintfW( wszLongNameSrcAppended,
                            _countof( wszLongNameSrcAppended ),
                            L"%s\\%s",
                            wszLongNameSrc,
                            fdFileInfo.cFileName );
            StringCchPrintfW( wszLongNameDstAppended,
                            _countof( wszLongNameDstAppended ),
                            L"%s\\%s",
                            wszLongNameDst,
                            fdFileInfo.cFileName );
            
            // File or subdir?
            if ( fdFileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
                if ( !EsetestCopyDirectoryW( wszLongNameSrcAppended, wszLongNameDstAppended ) ){
                    fReturn = FALSE;
                }
            }
            else{
                if ( !CopyFileExW( wszLongNameSrcAppended, wszLongNameDstAppended, NULL, NULL, NULL, 0 ) ){
                    fReturn = FALSE;
                }
            }
        } while ( FindNextFileW( hSearch, &fdFileInfo ) );
        // Error or didn't find any files?
        if ( GetLastError() != ERROR_NO_MORE_FILES ){
            fReturn = FALSE;
        }
        FindClose( hSearch );
    }

    return fReturn;
}

BOOL
EsetestEnsureFullPathA(
    __inout PSTR        szPath
)
{
    WCHAR wszPath[ MAX_PATH + 1 ];
    BOOL fReturn = FALSE;

    if ( szPath && SUCCEEDED( StringCchPrintfW( wszPath, _countof( wszPath ), L"%hs", szPath ) ) ){
        if ( !EsetestEnsureFullPathW( wszPath ) ){
            goto HandleError;
        }
        if ( FAILED( StringCchPrintfA( szPath, MAX_PATH, "%ls", wszPath ) ) ){
            goto HandleError;
        }
    }
    else{
        goto HandleError;
    }
    fReturn = TRUE;

HandleError:
    return fReturn;
}

BOOL
EsetestEnsureFullPathW(
    __inout PWSTR       wszPath
)
{
    WCHAR* wszRelativePath = NULL;
    WCHAR wszCurrentDirectory[ MAX_PATH + 1 ];
    BOOL fReturn = FALSE;

    if ( PathIsRelativeW( wszPath ) ){
        wszRelativePath = EsetestCopyWideString( __FUNCTION__, wszPath );
        if ( !GetCurrentDirectoryW( _countof( wszCurrentDirectory ), wszCurrentDirectory ) ){
            goto HandleError;
        }
        if( !PathCombineW( wszPath, wszCurrentDirectory, wszRelativePath ) ){
            goto HandleError;
        }

        delete []wszRelativePath;
        wszRelativePath  = NULL;
    }
    if ( !PathAddBackslashW( wszPath ) ){
        goto HandleError;
    }
    fReturn = TRUE;

HandleError:
    delete []wszRelativePath;
    return fReturn;
}

BOOL
EsetestEnsureFullPathExistsA(
    __in PCSTR      szPath
)
{
    WCHAR* wszPath      = EsetestWidenString( __FUNCTION__, szPath );
    BOOL fReturn        = EsetestEnsureFullPathExistsW( wszPath );
    delete []wszPath;

    return fReturn;
}

BOOL
EsetestEnsureFullPathExistsW(
    __in PCWSTR     wszPath
)
{
    BOOL fReturn = FALSE;
    WCHAR wszFullPath[ MAX_PATH + 1 ];
    int iError = 0;
    if ( wszPath && SUCCEEDED( StringCchCopyW( wszFullPath, _countof( wszFullPath ), wszPath ) ) &&
            EsetestEnsureFullPathW( wszFullPath ) ){
        if ( !PathFileExistsW( wszFullPath ) ){
            do
            {
                if ( !EsetestCreateDirectoryW( wszFullPath )){
                    iError = GetLastError();
                    Sleep(100);
                }
                else    {
                    iError = 0;
                }
            }
            while ( iError == ERROR_FILE_EXISTS || iError == ERROR_ALREADY_EXISTS );

            if ( iError != 0 ){
                goto HandleError;
            }
        }
    }
    else{
        goto HandleError;
    }
    fReturn = TRUE;

HandleError:
    return fReturn;
}

PSTR
EsetestGetFilePathA(
    __in PCSTR      szFile
)
{
    WCHAR* wszFile      = EsetestWidenString( __FUNCTION__, szFile );
    WCHAR* wszReturn    = EsetestGetFilePathW( wszFile );
    CHAR* szReturn      = EsetestUnwidenStringAlloc( __FUNCTION__, wszReturn );
    delete []wszFile;
    delete []wszReturn;

    return szReturn;
}

PWSTR
EsetestGetFilePathW(
    __in PCWSTR     wszFile
)
{
    size_t cch = wszFile ? wcslen( wszFile ) + 1 : 0;
    PWSTR wszPath = new WCHAR[ cch ];

    if ( cch && wszPath && SUCCEEDED( StringCchCopyW( wszPath, cch, wszFile ) ) ){
        PathRemoveFileSpecW( wszPath );
    }
    else{
        delete []wszPath;
        wszPath = NULL;
        goto HandleError;
    }

HandleError:
    return wszPath;
}

LONGLONG
EsetestGetFileSizeA(
    __in PCSTR      szFile
)
{
    WCHAR* wszFile      = EsetestWidenString( __FUNCTION__, szFile );
    LONGLONG cbFileSize = EsetestGetFileSizeW( wszFile );
    delete []wszFile;

    return cbFileSize;
}

LONGLONG
EsetestGetFileSizeW(
    __in PCWSTR     wszFile
)
{
    LONGLONG cbFileSize = -1;
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;

    if ( NULL == wszFile ) goto Cleanup;
    if ( GetFileAttributesExW( wszFile ,GetFileExInfoStandard, &fileInfo ) == FALSE ) goto Cleanup;
    cbFileSize = ( ( ( LONGLONG )fileInfo.nFileSizeHigh ) << ( 8 * sizeof( fileInfo.nFileSizeHigh ) ) ) +
                    fileInfo.nFileSizeLow;

Cleanup:
    return cbFileSize;
}

//================================================================
// Time helper functions.
//================================================================
//

static const ULONGLONG qw100NanoSecSince1601in1970 = 0x19db1ded53e8000;

DWORD
SystemTimeToSecondsSince1970(
    __in const PSYSTEMTIME  pSystemTime
)
{
    FILETIME ft, ftBuffer;
    ULARGE_INTEGER qwft;

    // Converting the date we want.
    if ( !SystemTimeToFileTime( pSystemTime, &ftBuffer ) ){
        OutputError( "%s(): SystemTimeToFileTime() failed with %lu!" CRLF, __FUNCTION__, GetLastError() );
        return 0;
    }

    // Converting local time UTC-based.
    if ( !LocalFileTimeToFileTime( &ftBuffer, &ft ) ){
        OutputError( "%s(): LocalFileTimeToFileTime() failed with %lu!" CRLF, __FUNCTION__, GetLastError() );
        return 0;
    }

    // Copying to a ULONG64 variable.
    qwft.HighPart = ft.dwHighDateTime;
    qwft.LowPart = ft.dwLowDateTime;

    // Computing in seconds, instead of 100-nanoseconds (10^-7).
    return ( DWORD )( ( qwft.QuadPart - qw100NanoSecSince1601in1970 ) / 10000000 );
}

VOID
SecondsSince1970ToSystemTime(
    __in DWORD          dwSecondsSince1970,
    __in PSYSTEMTIME    pSystemTime
)
{
    FILETIME ftBuffer, ft;
    ULARGE_INTEGER qwft;

    // Computing 10^-7 seconds since 1601.
    qwft.QuadPart = ( ( ULONGLONG )dwSecondsSince1970 ) * 10000000 + qw100NanoSecSince1601in1970;
    ftBuffer.dwHighDateTime = qwft.HighPart;
    ftBuffer.dwLowDateTime = qwft.LowPart;

    // Converting UTC-based to local time.
    if ( !FileTimeToLocalFileTime( &ftBuffer, &ft ) ){
        OutputError( "%s(): FileTimeToLocalFileTime() failed with %lu!" CRLF, __FUNCTION__, GetLastError() );
        return;
    }   

    // Converting to SystemTime.
    if ( !FileTimeToSystemTime( &ft, pSystemTime ) ){
        OutputError( "%s(): FileTimeToSystemTime() failed with %lu!" CRLF, __FUNCTION__, GetLastError() );
        pSystemTime->wYear = 1970;
        pSystemTime->wMonth = 1;
        pSystemTime->wDayOfWeek = 4;
        pSystemTime->wDay = 1;
        pSystemTime->wHour = 0;
        pSystemTime->wMinute = 0;
        pSystemTime->wSecond = 0;
        pSystemTime->wMilliseconds = 0;
    }
}

//================================================================
// Assert functionality and helper functions.
//================================================================
//

PRIVATE
int UtilMessageBoxW( IN const WCHAR * const wszText, IN const WCHAR * const wszCaption, IN const UINT uType )
    {
    typedef decltype(MessageBoxW) PFNMessageBox;
    const char * const szFunction = "MessageBoxW";

    HMODULE                 hmodUser32      = NULL;
    PFNMessageBox *         pfnMessageBox   = NULL;
    int                     retval          = 0;

    hmodUser32 = LoadLibraryW( L"user32.dll" );
    if ( !hmodUser32 )
        {
        goto NoMessageBox;
        }
    pfnMessageBox = (PFNMessageBox*)GetProcAddress( hmodUser32, szFunction );
    if ( !pfnMessageBox )
        {
        goto NoMessageBox;
        }

    retval = (*pfnMessageBox)( NULL, wszText, wszCaption, uType );

NoMessageBox:
    if ( hmodUser32 )
        {
        FreeLibrary( hmodUser32 );
        }
    return retval;
    }

PRIVATE
void OSDebugPrint( const WCHAR * const wszOutput )
    {
        {
        OutputDebugStringW( wszOutput );
        }
    }

PRIVATE
void ForceProcessCrash()
    {
    *( char* )0 = 0;
    }

#define ErrOSStrCbAppendA( szDst, cbDst, szSrc )        StringCbCatA( szDst, cbDst, szSrc )

static
void __cdecl OSStrCbFormatA ( __out_bcount(cbBuffer) PSTR szBuffer, size_t cbBuffer, __format_string PCSTR szFormat, ...)
    {
    va_list alist;
    va_start( alist, szFormat );
    (void) StringCbVPrintf( szBuffer, cbBuffer, szFormat, alist );
    va_end( alist );
    }

//  create a formatted string in a given buffer
static
void __cdecl OSStrCbVFormatA ( __out_bcount(cbBuffer) PSTR szBuffer, size_t cbBuffer, __format_string PCSTR szFormat, va_list alist )
    {
    (void) StringCbVPrintf( szBuffer, cbBuffer, szFormat, alist );
    }

static
void __cdecl OSStrCbFormatW ( __out_bcount(cbBuffer) PWSTR szBuffer, size_t cbBuffer, __format_string PCWSTR szFormat, ...)
    {
    va_list alist;
    va_start( alist, szFormat );
    (void) StringCbVPrintfW( szBuffer, cbBuffer, szFormat, alist );
    va_end( alist );
    }

static WCHAR                g_wszAssertText[1024];
static BOOL             g_fSkipAssert = fFalse;
static BOOL g_fSkipFailFast = fFalse;

// Possible ESE assert actions, as of 2007/12/18:
// #define JET_AssertExit               0x0000      /* Exit the application */
// #define JET_AssertBreak              0x0001      /* Break to debugger */
// #define JET_AssertMsgBox             0x0002      /* Display message box */
// #define JET_AssertStop               0x0004      /* Alert and stop */
// #define JET_AssertSkippableMsgBox    0x0008      /* Display skippable message box */
// #define JET_AssertSkipAll            0x0010      /* Skip all asserts */
// #define JET_AssertCrash              0x0020      /* AV (*0=0 style) */
void __stdcall EsetestAssertFail( const char * const szMessageFormat, char const *szFilename, long lLine, ... )
    {
    va_list args;
    va_start( args, lLine );
    size_t offset = 0;

#if 1
    if ( g_esetestconfig.m_esetestAssertAction == JET_AssertSkipAll )
    {
        //  we immediately return here to allow the SkipAll action to be
        //  used for disabling asserts during crashdumps
        va_end( args ); 
        return;
    }
#endif
    CHAR                szAssertText[1024];

    /*      get last error before another system call
    /**/
    DWORD dwSavedGLE = GetLastError();

#if 0
    //  allow only one assert/enforce at a time
    //
    EnterCriticalSection( &g_csError );
#endif

#if 0
    if ( g_tidAssertFired == DwUtilThreadId() )
        {
        //  Deal with the case where an assert has gone off in the middle
        //  of an assert. One case where this can happen is if the assert
        //  action causes a crash which is then intercepted by the calling 
        //  application which then calls back into Jet (typically as part
        //  of a finally block in exception handling).

        SetLastError( dwSavedGLE );
        WCHAR wszMessageFormat[ _MAX_PATH ];
        (void)ErrOSSTRAsciiToUnicode( szMessageFormat,
                                      wszMessageFormat,
                                      _countof( wszMessageFormat ) );
        HandleNestedAssert( wszMessageFormat, szFilename, lLine );

        //  Ok, blunder on like nothing is wrong.
        LeaveCriticalSection( &g_csError );
        SetLastError( dwSavedGLE );
        return; // this is not the assert we're interested in.
        }

    g_tidAssertFired = DwUtilThreadId();
#endif
    int             id;

//  szFilenameAssert = szFilename;
    lLineAssert = lLine;

    /*      select file name from file path
    /**/
    szFilename = ( NULL == strrchr( szFilename, chPathDelimiter ) ) ? szFilename : strrchr( szFilename, chPathDelimiter ) + sizeof( CHAR );

    /*      assemble monolithic assert string
    /**/

    /*  start by putting in the assert message
    /**/
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

    /*  Add the version, file, line and PID/TID info
    /**/
    OSStrCbFormatA(
        szAssertText + offset,
        sizeof( szAssertText ) - offset,
        "File %hs, Line %d, PID: %d (0x%x), TID: 0x%x \r\n\r\n",
        szFilename,
        lLine,
        DwUtilProcessId(),
        DwUtilProcessId(),
        DwUtilThreadId() );
    offset = strlen( szAssertText );

#if 0
    /*  Can't get TLS error state until OS preinit is done */
    if ( FOSLayerUp() )
        {
        /*  Add the last error info
        /**/
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
#endif

#if 0
    /*  Add the assert.txt file
    /**/
    OSStrCbFormatA(
        szAssertText + offset,
        sizeof( szAssertText ) - offset,
        "\r\nComplete information can be found in:  %ws\r\n",
        wszAssertFile );
    offset = strlen( szAssertText );
#endif

#if 0
    OSTrace( JET_tracetagAsserts, szAssertText );

    /******************************************************
    /**/
    /*      if event log environment variable set then write
    /*      assertion to event log.
    /**/
    if ( !fNoWriteAssertEvent )
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
#endif

#if 0
    // Log to UTC Telemetry
    //
    WCHAR           wszIssueSource[ g_cchIssueSourceMax ] = L"FORMAT STRING FAIL";
    C_ASSERT( _countof( wszIssueSource ) < 260 );   //  make sure we stay reasonably small
    ERRFormatIssueSource( wszIssueSource, sizeof( wszIssueSource ), dwSavedGLE, szFilename, lLine );
    OSTelemetryTrackAssertFail( szMessageFormat, wszIssueSource );

    {
    CPRINTFFILE cprintffileAssertTxt( wszAssertFile );
    cprintffileAssertTxt( "%ws", g_wszAssertText );
    }
#endif

    UINT wAssertAction = g_esetestconfig.m_esetestAssertAction;
    BOOL fDevMachine = fTrue;
#if 0
    fDevMachine = IsDevMachine();
#endif
    if ( fDevMachine && wAssertAction == JET_AssertMsgBox )
        {
        wAssertAction = JET_AssertSkippableMsgBox;
        }
    if ( fDevMachine && wAssertAction == JET_AssertFailFast )
        {
        wAssertAction = JET_AssertBreak;
        }


    /*      Print out the assert if a debugger is attached
    /**/
    OSDebugPrint( g_wszAssertText );

#if 1
    /*      Perform the Assert Action
    /**/
    if ( wAssertAction == JET_AssertExit )
        {
        TerminateProcess( GetCurrentProcess(), UINT( ~0 ) );
        }
    else if ( wAssertAction == JET_AssertBreak )
        {
        OSDebugPrint( L"\nTo continue, press 'g'.\n" );
        SetLastError(dwSavedGLE);
        DebugBreak();
        }
    else if ( wAssertAction == JET_AssertStop )
        {
        for( ; !g_fSkipAssert; )
            {
            /*  wait for developer, or anyone else, to debug the failure
            /**/
            Sleep( 100 );
            }
        g_fSkipAssert = fFalse;
        }
    else if ( wAssertAction == JET_AssertCrash )
        {
        OSDebugPrint( L"\nTo continue, \"ed ese!g_fSkipAssert 1; g\".\n" );
        __try
            {
            ForceProcessCrash();
            } 
        __except ( g_fSkipAssert ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
            {
            g_fSkipAssert = fFalse;
            }
        }
    else if ( JET_AssertFailFast == wAssertAction )
        {
        if ( fDevMachine )
            {
            OSDebugPrint( L"\n NOTE: WILL FAIL FAST UNLESS you run \"ed ese!g_fSkipFailFast 1; g\".  Don't just press 'g'.\n" );
            SetLastError( dwSavedGLE );
            UserDebugBreakPoint();
            }
        if ( !g_fSkipFailFast )
            {
            RaiseFailFastException( NULL, NULL, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS );
            }
        }
    else if (   wAssertAction == JET_AssertMsgBox ||
                wAssertAction == JET_AssertSkippableMsgBox )
#endif
        {
        size_t  cchOffset;

        //  append the debugger message
        
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
            DebugBreak();
            }
        }
    else if ( wAssertAction == JET_AssertSkipAll )
        {
        // Do nothing.
        }

#if 0
    g_tidAssertFired = 0x0;
    LeaveCriticalSection( &g_csError );
#endif

    /* End the var args ... not sure it should be before SetLastError(), but playing it safe.
    /**/
    va_end( args );

    /* Restore the last error, to avoid affecting other code paths if we ignored the assert(), and to make debuggability better
    /**/
    SetLastError(dwSavedGLE);
    return;
    }

//================================================================
// Generic and Wide vs. Narrow API helper functions.
//================================================================
//

#pragma warning( default : 4509 )

size_t strxchg(
    __in CHAR *string,
    __in const CHAR oldChar,
    __in const CHAR newChar
)
{
    if ( NULL == string || 0 == oldChar ) return 0;

    size_t count = 0;
    for ( CHAR *pChar = string; pChar = strchr( pChar, oldChar ); *pChar++ = newChar, count++ );
    return count;
}

size_t wcsxchg(
    __in WCHAR *string,
    __in const WCHAR oldChar,
    __in const WCHAR newChar
)
{
    if ( NULL == string || 0 == oldChar ) return 0;

    size_t count = 0;
    for ( WCHAR *pChar = string; pChar = wcschr( pChar, oldChar ); *pChar++ = newChar, count++ );
    return count;
}

size_t _tcsxchg(
    __in TCHAR *string,
    __in const TCHAR oldChar,
    __in const TCHAR newChar
)
{
#ifdef _UNICODE
    return wcsxchg( string, oldChar, newChar );
#else
    return strxchg( string, oldChar, newChar );
#endif
}


ESETEST_OUTPUT_LEVEL
EsetestGetOutputLevel()
{
    return g_esetestconfig.m_OutputLevel;
}

void
EsetestSetOutputLevel(
    const ESETEST_OUTPUT_LEVEL  level
)
{
    g_esetestconfig.m_OutputLevel = level;
}


int OutputDebug( const char *szFormat, ... )
{
    int nRC = 0;
    if ( OUTPUT_DEBUG <= EsetestGetOutputLevel() ) {
        va_list valist;
        va_start( valist, szFormat );
        nRC = EsetestVcprintf( COLOR_DEBUG, szFormat, valist );
        va_end( valist );
    }
    return nRC;
}

int OutputInfo( const char *szFormat, ... )
{
    int nRC = 0;
    if ( OUTPUT_INFO <= EsetestGetOutputLevel() ) {
        va_list valist;
        va_start( valist, szFormat );
        nRC = EsetestVcprintf( COLOR_INFO, szFormat, valist );
        va_end( valist );
    }
    return nRC;
}

int OutputWarning( const char *szFormat, ... )
{
    int nRC = 0;
    if ( OUTPUT_WARNING <= EsetestGetOutputLevel() ) {
        va_list valist;
        va_start( valist, szFormat );
        nRC = EsetestVcprintf( COLOR_WARNING, szFormat, valist );
        va_end( valist );
    }
    return nRC;
}

int OutputError( const char *szFormat, ... )
{
    int nRC = 0;
    if ( OUTPUT_ERROR <= EsetestGetOutputLevel() ) {
        va_list valist;
        va_start( valist, szFormat );
        nRC = EsetestVcprintf( COLOR_ERROR, szFormat, valist );
        va_end( valist );
    }
    return nRC;
}

// Closing NULL or INVALID_HANDLE_VALUE is FINE!!
// Also, Nullify the input handle
BOOL CloseHandleP( HANDLE* pH )
{
    BOOL fRc = TRUE;

    if ( NULL != *pH && INVALID_HANDLE_VALUE != *pH ) {
        fRc = CloseHandle( *pH );
        *pH = NULL;
    }

    return fRc;
}

// --------------
// The function handles the string with embedded NULL by 
// calling MultiByteToWideChar().
wchar_t*
EsetestWidenCbString(
    __in PSTR   szFunction,
    __in PCSTR  sz,
    __in const unsigned long    cchsz
)
{
    wchar_t* wsz    = NULL;
    int iRc;

    iRc = MultiByteToWideChar(CP_ACP, 0, sz, cchsz, NULL, 0);
    if ( 0 == iRc )
    {
        tprintf( "%s() failed to MultiByteToWideChar()" CRLF, szFunction );
        wsz = NULL;
        goto Cleanup;
    }

    //allocate and copy sz
    wsz = new wchar_t[ cchsz ];
    if ( NULL == wsz )
    {
        tprintf( "%s(): Failed to allocate memory for wsz (%Id DWORD requested)" CRLF, szFunction, cchsz );
        goto Cleanup;
    }

    iRc = MultiByteToWideChar(CP_ACP, 0, sz, cchsz, wsz, cchsz );
    if ( 0 == iRc )
    {
        tprintf( "%s() failed to MultiByteToWideChar()" CRLF, szFunction );
        delete[] wsz;
        wsz = NULL;
        goto Cleanup;
    }

Cleanup:
    return wsz;
    
}

JET_ERR
EsetestUnwidenCbString(
    __in PSTR   szFunction,
    __in PWSTR  wsz,
    __in const unsigned long    cchsz,
    __inout PSTR    sz
)
{
    
    JET_ERR     err     = JET_errSuccess;
    HRESULT         hr;

    // copy sz
    hr = WideCharToMultiByte( CP_ACP, 0, wsz, cchsz, sz, cchsz, NULL, NULL );
    if ( 0 ==  hr )
    {
        tprintf( "%s(): Failed to WideCharToMultiByte()" CRLF, __FUNCTION__ );
        err = JET_errTestError;
        goto Cleanup;
    }
Cleanup:

    mdelete_array( wsz );
    return err;
}

// If this is the defined special string, I am going to use a real unicode string
bool IfMySpecialString (
    __in PCSTR  sz
)
{
    char mystring[20] = "letusgotodisney.mdb";
    
    if ( sz == NULL)  return false;
    
    const int   cchsz   = strlen( sz );

    if ( cchsz != (int) strlen( mystring ) ) return false;
    
    for ( int i = 0; i < cchsz; i ++ ) 
    {
        if ( sz [ i ] != mystring[i] ) 
            return false;
    }
    
    return true;
    
}

// --------------
char*
EsetestCopyString(
    __in PSTR   szFunction,
    __in PCSTR  szToCopy
)
{
    char*       sz      = NULL;
    size_t  cchsz   = 0; 
    HRESULT         hr;

    if ( szToCopy == NULL ) return NULL;

    cchsz   = strlen( szToCopy ) + 1;
    // Allocate and copy wsz
    sz = new char[ cchsz ];
    if ( NULL == sz )
    {
        tprintf( "%s(): Failed to allocate memory for sz (%Id bytes requested)" CRLF, szFunction, cchsz );
        goto Cleanup;
    }

    hr = StringCchPrintfA( sz, cchsz, "%s", szToCopy );
    if ( FAILED( hr ) )
    {
        tprintf( "%s(): Failed to StringCchPrintf(), hr = %d" CRLF, __FUNCTION__, hr );
        delete[] sz;
        sz = NULL;
        goto Cleanup;
    }

Cleanup:
    return sz;
}

// --------------
wchar_t*
EsetestCopyWideString(
    __in PSTR   szFunction,
    __in PCWSTR wszToCopy
)
{
    wchar_t*        wsz     = NULL;
    size_t  cchwsz  = 0; 
    HRESULT         hr;

    if ( wszToCopy == NULL ) return NULL;

    cchwsz  = wcslen( wszToCopy ) + 1;
    // Allocate and copy wsz
    wsz = new wchar_t[ cchwsz ];
    if ( NULL == wsz )
    {
        tprintf( "%s(): Failed to allocate memory for wsz (%Id bytes requested)" CRLF, szFunction, cchwsz );
        goto Cleanup;
    }

    hr = StringCchPrintfW( wsz, cchwsz, L"%ls", wszToCopy );
    if ( FAILED( hr ) )
    {
        tprintf( "%s(): Failed to StringCchPrintfW(), hr = %d" CRLF, __FUNCTION__, hr );
        delete[] wsz;
        wsz = NULL;
        goto Cleanup;
    }

Cleanup:
    return wsz;
}

wchar_t*
EsetestWidenString(
    __in PSTR   szFunction,
    __in PCSTR  sz
)
{
    wchar_t*        wsz     = NULL;
    //const size_t  cchsz   = strlen( sz ) + 1;
    size_t  cchsz   = 0; 
    HRESULT         hr;

    if ( sz == NULL ) return NULL;

    cchsz   = strlen( sz ) + 1;
    // Allocate and copy sz
    wsz = new wchar_t[ cchsz ];
    if ( NULL == wsz )
    {
        tprintf( "%s(): Failed to allocate memory for wsz (%Id bytes requested)" CRLF, szFunction, cchsz );
        goto Cleanup;
    }

    // create a real unicode for testing. It will be used as an index name for tuplelong.exe
    // not a good idea, breaks tests
    //if ( 0 == strcmp ( sz, "PlusUnicode" ) )      
    //{
    //  swprintf( wsz, L"\x8fea\x65af\x0000" );
    //  goto Cleanup;
    //}

    // another special string
    //if ( IfMySpecialString ( sz ) ) 
    if ( 0 == strcmp( sz, "RealUnicodeName" ) )
    {
        swprintf( wsz, L"\x8fea\x65af\x0000" );
        goto Cleanup;   
    }
    
    if ( 0 == strcmp ( sz, "letusgotodisney.mdb" ) )        
    {
        //This becomes "disney.mdb" in Chinese (unicode).
        swprintf( wsz, L"\x8fea\x65af\x5c3c\x002e\x006d\x0064\x0062" );
        //wprintf ( L" wsz is %ws\n", wsz );
        goto Cleanup;
    }

    if ( sz[0] == '\0' )
    {
        wsz[0] = L'\0';
        goto Cleanup;
    }

    hr = StringCchPrintfW( wsz, cchsz, L"%hs", sz );
    if ( FAILED( hr ) )
    {
        tprintf( "%s(): Failed to StringCchPrintfW(), hr = %d" CRLF, __FUNCTION__, hr );
        delete[] wsz;
        wsz = NULL;
        goto Cleanup;
    }

Cleanup:
    return wsz;
}

char*
EsetestUnwidenStringAlloc(
    __in PSTR   szFunction,
    __in PCWSTR wsz
)
{
    char*       sz      = NULL;
    size_t  cchwsz      = 0; 
    HRESULT     hr;

    if ( wsz == NULL ) return NULL;

    cchwsz  = wcslen( wsz ) + 1;
    // Allocate and copy wsz
    sz = new char[ cchwsz ];
    if ( NULL == sz )
    {
        tprintf( "%s(): Failed to allocate memory for sz (%Id bytes requested)" CRLF, szFunction, cchwsz );
        goto Cleanup;
    }

    if ( wsz[0] == L'\0' )
    {
        sz[0] = '\0';
        goto Cleanup;
    }

    hr = StringCchPrintfA( sz, cchwsz, "%ws", wsz );
    if ( FAILED( hr ) )
    {
        tprintf( "%s(): Failed to StringCchPrintfA(), hr = %d" CRLF, __FUNCTION__, hr );
        delete[] sz;
        sz = NULL;
        goto Cleanup;
    }

Cleanup:
    return sz;
}

JET_ERR
EsetestUnwidenString(
    __in PSTR   szFunction,
    __in PWSTR  wsz,
    __inout PSTR    sz
)
{
    JET_ERR     err     = JET_errSuccess;
    HRESULT         hr;

    if ( wsz == NULL ) 
    {
        sz = NULL;
        goto Cleanup;
    }

    const size_t    cchwsz  = wcslen( wsz ) + 1;    
    // copy wsz
    hr = StringCchPrintfA( sz, cchwsz, "%ws", wsz );
    if ( FAILED( hr ) )
    {
        tprintf( "%s(): Failed to StringCchPrintfA(), hr = %d" CRLF, __FUNCTION__, hr );
        err = JET_errTestError;
        goto Cleanup;
    }

Cleanup:
    // should we clean up wsz? (For the time being, yes)
    // come back here
    mdelete_array( wsz );

    return err;
}

// why do we need both EsetestCleanupWidenString() and
// EsetestUnwidenString()
JET_ERR
EsetestCleanupWidenString(
    __in PSTR   szFunction,
    __inout PWSTR   wsz,
    __in PCSTR  sz
)
{
    
    JET_ERR     err     = JET_errSuccess;
    mdelete_array( wsz );
    return err;

}

//----------------StringWithLength
wchar_t*
EsetestWidenStringWithLength(
    __in PSTR   szFunction,
    __in PCSTR  sz,
    __in const unsigned long cchsz
)
{
    wchar_t*        wsz     = NULL;
    
    // Allocate and copy sz
    wsz = new wchar_t[ cchsz ];

    if ( NULL == wsz )
    {
        tprintf( "%s(): Failed to allocate memory for wsz (%Id bytes requested)" CRLF, szFunction, cchsz );
        goto Cleanup;
    }
    // we don't need to copy sz to wsz,
    // becuase wsz will be used as output buffer
    // but we should initiate it
    wsz[0] = L'\0';

Cleanup:
    return wsz;
}


JET_ERR
EsetestUnwidenStringWithLength(
    __in PSTR   szFunction,
    __in PWSTR  wsz,
    __in const unsigned long cchsz,
    __inout PSTR    sz
)
{
    JET_ERR     err     = JET_errSuccess;
    HRESULT         hr;

    if ( cchsz == NULL ) goto Cleanup;

    if ( wsz[0] == L'\0' && cchsz >= 1 )
    {
        sz[0] = '\0';
        goto Cleanup;
    }

    hr = StringCchPrintfA( sz, cchsz, "%ws", wsz );
    if ( FAILED( hr ) )
    {
        tprintf( "%s(): Failed to StringCchPrintfA(), hr = %d" CRLF, __FUNCTION__, hr );
        err = JET_errTestError;
        goto Cleanup;
    }

Cleanup:
    mdelete_array( wsz );
    return err;

}

// --------------DONE
JET_RSTMAP_W*
EsetestWidenJET_RSTMAP(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) const JET_RSTMAP*    rgstmap,
    __in const long         crstfilemap
)
{
    JET_RSTMAP_W*   wrgstmap        = NULL;
    char*   szDatabaseName      = NULL;
    char*   szNewDatabaseName   = NULL;
    
    if(rgstmap == NULL)
        goto Cleanup;

    // Allocate and copy rgstmap
    wrgstmap = new JET_RSTMAP_W[ crstfilemap ];
    if ( NULL == wrgstmap )
    {
        tprintf( "%s(): Failed to allocate memory for JET_RSTMAP" CRLF, szFunction );
        goto Cleanup;
    }

    // note: crstfilemap is the total count of  array rgstmap
    for ( int i = 0; i < crstfilemap; i ++ ) {
        
        // copy  szdatabaseName and szNewDatabaseName

        szDatabaseName = rgstmap[ i ].szDatabaseName;
        szNewDatabaseName = rgstmap[ i ].szNewDatabaseName;

        if ( szDatabaseName != NULL ) 
        {
            wrgstmap[ i ].szDatabaseName = EsetestWidenString( szFunction, szDatabaseName );
            if ( NULL == wrgstmap[ i ].szDatabaseName) 
            {
                tprintf( "%s(): Failed to EsetestWidenString()" CRLF, __FUNCTION__);
                delete[] wrgstmap;
                wrgstmap = NULL;
                goto Cleanup;
            }
        } else
        {
            wrgstmap[ i ].szDatabaseName = NULL;
        }

        if ( szNewDatabaseName != NULL ) 
        {
            wrgstmap[ i ].szNewDatabaseName = EsetestWidenString( szFunction, szNewDatabaseName );
            if ( NULL == wrgstmap[ i ].szNewDatabaseName) 
            {
                tprintf( "%s(): Failed to EsetestWidenString()" CRLF, __FUNCTION__);
                delete[] wrgstmap;
                wrgstmap = NULL;
                goto Cleanup;
            }
        } else 
        {
            wrgstmap[ i ].szNewDatabaseName = NULL;
        }

    }
    
Cleanup:
    return wrgstmap;
}

JET_RSTMAP2_W*
EsetestWidenJET_RSTMAP2(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) const JET_RSTMAP2*   rgstmap,
    __in const long         crstfilemap
)
{
    JET_RSTMAP2_W*  wrgstmap        = NULL;
    char*   szDatabaseName      = NULL;
    char*   szNewDatabaseName   = NULL;
    
    if(rgstmap == NULL)
        goto Cleanup;

    // Allocate and copy rgstmap
    wrgstmap = new JET_RSTMAP2_W[ crstfilemap ];
    if ( NULL == wrgstmap )
    {
        tprintf( "%s(): Failed to allocate memory for JET_RSTMAP" CRLF, szFunction );
        goto Cleanup;
    }

    // note: crstfilemap is the total count of  array rgstmap
    for ( int i = 0; i < crstfilemap; i ++ ) {

        wrgstmap[ i ].cbStruct = sizeof( JET_RSTMAP2_W );
        
        // copy  szdatabaseName and szNewDatabaseName

        szDatabaseName = rgstmap[ i ].szDatabaseName;
        szNewDatabaseName = rgstmap[ i ].szNewDatabaseName;

        if ( szDatabaseName != NULL ) 
        {
            wrgstmap[ i ].szDatabaseName = EsetestWidenString( szFunction, szDatabaseName );
            if ( NULL == wrgstmap[ i ].szDatabaseName) 
            {
                tprintf( "%s(): Failed to EsetestWidenString()" CRLF, __FUNCTION__);
                delete[] wrgstmap;
                wrgstmap = NULL;
                goto Cleanup;
            }
        } else
        {
            wrgstmap[ i ].szDatabaseName = NULL;
        }

        if ( szNewDatabaseName != NULL ) 
        {
            wrgstmap[ i ].szNewDatabaseName = EsetestWidenString( szFunction, szNewDatabaseName );
            if ( NULL == wrgstmap[ i ].szNewDatabaseName) 
            {
                tprintf( "%s(): Failed to EsetestWidenString()" CRLF, __FUNCTION__);
                delete[] wrgstmap;
                wrgstmap = NULL;
                goto Cleanup;
            }
        } else 
        {
            wrgstmap[ i ].szNewDatabaseName = NULL;
        }

        wrgstmap[ i ].rgsetdbparam = rgstmap[ i ].rgsetdbparam;
        wrgstmap[ i ].csetdbparam = rgstmap[ i ].csetdbparam;

        wrgstmap[ i ].grbit = rgstmap[ i ].grbit;
    }
    
Cleanup:
    return wrgstmap;
}

JET_ERR
EsetestUnwidenJET_RSTMAP(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) JET_RSTMAP_W* wrgstmap,
    __in const long         crstfilemap,
    __inout_ecount( crstfilemap ) JET_RSTMAP*   rgstmap
)
{
    JET_ERR err = JET_errSuccess;


    HRESULT         hr;
    wchar_t*    wszDatabaseName = NULL;
    wchar_t*    wszNewDatabaseName = NULL;
    
    if (wrgstmap == NULL ) 
    {
        goto Cleanup;
    }

    for ( int i = 0; i < crstfilemap; i ++ ) {
        
        // copy wszDatabaseName and wszNewDatabaseName
        wszDatabaseName = wrgstmap[ i ].szDatabaseName;
        wszNewDatabaseName = wrgstmap[ i ].szNewDatabaseName;

        if ( wszDatabaseName != NULL ) 
        {

            //hr = EsetestUnwidenString( szFunction, wszDatabaseName, rgstmap[ i ].szDatabaseName);
            hr = EsetestCleanupWidenString( szFunction, wszDatabaseName, rgstmap[ i ].szDatabaseName);
            if ( FAILED( hr ) ) 
            {
                tprintf( "%s(): Failed to EsetestCleanupWidenString(), hr = %d" CRLF, __FUNCTION__, hr );
                err = JET_errTestError;
                goto Cleanup;
            }
        } else 
        {
            rgstmap[ i ].szDatabaseName = NULL;
        }

        if ( wszNewDatabaseName != NULL ) 
        {
            //hr = EsetestUnwidenString( szFunction, wszNewDatabaseName, rgstmap[ i ].szNewDatabaseName );
            hr = EsetestCleanupWidenString( szFunction, wszNewDatabaseName, rgstmap[ i ].szNewDatabaseName );
            if ( FAILED( hr ) ) 
            {
                tprintf( "%s(): Failed to EsetestCleanupWidenString(), hr = %d" CRLF, __FUNCTION__, hr );
                err = JET_errTestError;
                goto Cleanup;
            }
        } else
        {
            rgstmap[ i ].szNewDatabaseName = NULL;
        }
        
        
    }

Cleanup:
    // free JET_RSTMAP_W
    mdelete_array( wrgstmap );
    return err;
}

JET_ERR
EsetestUnwidenJET_RSTMAP2(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) JET_RSTMAP2_W* wrgstmap,
    __in const long         crstfilemap,
    __inout_ecount( crstfilemap ) JET_RSTMAP2*  rgstmap
)
{
    JET_ERR err = JET_errSuccess;


    HRESULT         hr;
    wchar_t*    wszDatabaseName = NULL;
    wchar_t*    wszNewDatabaseName = NULL;
    
    if (wrgstmap == NULL ) 
    {
        goto Cleanup;
    }

    for ( int i = 0; i < crstfilemap; i ++ ) {

        rgstmap[ i ].cbStruct = sizeof( JET_RSTMAP2 );
        
        // copy wszDatabaseName and wszNewDatabaseName
        wszDatabaseName = wrgstmap[ i ].szDatabaseName;
        wszNewDatabaseName = wrgstmap[ i ].szNewDatabaseName;

        if ( wszDatabaseName != NULL ) 
        {

            //hr = EsetestUnwidenString( szFunction, wszDatabaseName, rgstmap[ i ].szDatabaseName);
            hr = EsetestCleanupWidenString( szFunction, wszDatabaseName, rgstmap[ i ].szDatabaseName);
            if ( FAILED( hr ) ) 
            {
                tprintf( "%s(): Failed to EsetestCleanupWidenString(), hr = %d" CRLF, __FUNCTION__, hr );
                err = JET_errTestError;
                goto Cleanup;
            }
        } else 
        {
            rgstmap[ i ].szDatabaseName = NULL;
        }

        if ( wszNewDatabaseName != NULL ) 
        {
            //hr = EsetestUnwidenString( szFunction, wszNewDatabaseName, rgstmap[ i ].szNewDatabaseName );
            hr = EsetestCleanupWidenString( szFunction, wszNewDatabaseName, rgstmap[ i ].szNewDatabaseName );
            if ( FAILED( hr ) ) 
            {
                tprintf( "%s(): Failed to EsetestCleanupWidenString(), hr = %d" CRLF, __FUNCTION__, hr );
                err = JET_errTestError;
                goto Cleanup;
            }
        } else
        {
            rgstmap[ i ].szNewDatabaseName = NULL;
        }

        rgstmap[ i ].rgsetdbparam = wrgstmap[ i ].rgsetdbparam;
        rgstmap[ i ].csetdbparam = wrgstmap[ i ].csetdbparam;
        
        rgstmap[ i ].grbit = wrgstmap[ i ].grbit;
    }

Cleanup:
    // free JET_RSTMAP_W
    mdelete_array( wrgstmap );
    return err;
}


//------------DONE
JET_RSTINFO_W*
EsetestWidenJET_RSTINFO(
    __in PSTR   szFunction,
    __in const JET_RSTINFO *prstInfo
)
{
    JET_RSTINFO_W* wprstInfo = NULL;

    // Allocate space for wprstInfo
    wprstInfo= new JET_RSTINFO_W;
    
    if ( NULL == wprstInfo )
    {
        tprintf( "%s(): Failed to allocate memory for JET_RSTINFO" CRLF, szFunction );
        goto Cleanup;
    }

    // the only thing that need to be widened inside JET_RSTINFO is rgrstmap
    if ( prstInfo->rgrstmap != NULL ) 
    {
        wprstInfo->rgrstmap = EsetestWidenJET_RSTMAP( szFunction, prstInfo->rgrstmap, prstInfo->crstmap );
        if ( wprstInfo->rgrstmap == NULL )
        {
            tprintf( "%s(): Failed to EsetestWidenJET_RSTMAP()" CRLF, __FUNCTION__);
            delete[] wprstInfo;
            wprstInfo = NULL;
            goto Cleanup;
            
        }
    } else
    {
        wprstInfo->rgrstmap = NULL;
    }
    
    // copy the rest of data
    wprstInfo -> cbStruct = prstInfo -> cbStruct;
    wprstInfo -> crstmap = prstInfo -> crstmap;
    wprstInfo -> lgposStop = prstInfo -> lgposStop;
    wprstInfo -> logtimeStop = prstInfo -> logtimeStop;
    wprstInfo -> pfnStatus = prstInfo -> pfnStatus;
Cleanup:
    return wprstInfo;
}


JET_ERR
EsetestUnwidenJET_RSTINFO(
    __in PSTR   szFunction,
    __in JET_RSTINFO_W* wrgstinfo,
    __inout JET_RSTINFO*    rgstinfo
)
{
    JET_ERR err = JET_errSuccess;
    HRESULT         hr;

    if ( wrgstinfo && rgstinfo )
    {
        // copy rgrstmap
        if ( wrgstinfo->rgrstmap != NULL ) 
        {
            hr = EsetestUnwidenJET_RSTMAP( szFunction, wrgstinfo->rgrstmap, wrgstinfo->crstmap, rgstinfo->rgrstmap );
            if ( FAILED( hr ) ) 
            {
                tprintf( "%s(): Failed to EsetestUnwidenJET_RSTMAP(), hr = %d" CRLF, __FUNCTION__, hr );
                err = JET_errTestError;
                goto Cleanup;
            }
        } else
        {
            rgstinfo->rgrstmap = NULL;
        }

        // copy the rest of data
        wrgstinfo -> cbStruct = rgstinfo -> cbStruct;
        wrgstinfo -> crstmap = rgstinfo -> crstmap;
        wrgstinfo -> lgposStop = rgstinfo -> lgposStop;
        wrgstinfo -> logtimeStop = rgstinfo -> logtimeStop;
        wrgstinfo -> pfnStatus = rgstinfo -> pfnStatus;
    }

Cleanup:
    mdelete( wrgstinfo );
    return err;
}


//------------DONE
JET_RSTINFO2_W*
EsetestWidenJET_RSTINFO2(
    __in PSTR   szFunction,
    __in const JET_RSTINFO2 *prstInfo
)
{
    JET_RSTINFO2_W* wprstInfo = NULL;

    // Allocate space for wprstInfo
    wprstInfo= new JET_RSTINFO2_W;
    
    if ( NULL == wprstInfo )
    {
        tprintf( "%s(): Failed to allocate memory for JET_RSTINFO2" CRLF, szFunction );
        goto Cleanup;
    }

    // the only thing that need to be widened inside JET_RSTINFO2 is rgrstmap
    if ( prstInfo->rgrstmap != NULL ) 
    {
        wprstInfo->rgrstmap = EsetestWidenJET_RSTMAP2( szFunction, prstInfo->rgrstmap, prstInfo->crstmap );
        if ( wprstInfo->rgrstmap == NULL )
        {
            tprintf( "%s(): Failed to EsetestWidenJET_RSTMAP2()" CRLF, __FUNCTION__);
            delete[] wprstInfo;
            wprstInfo = NULL;
            goto Cleanup;
            
        }
    } else
    {
        wprstInfo->rgrstmap = NULL;
    }
    
    // copy the rest of data
    wprstInfo -> cbStruct = prstInfo -> cbStruct;
    wprstInfo -> crstmap = prstInfo -> crstmap;
    wprstInfo -> lgposStop = prstInfo -> lgposStop;
    wprstInfo -> logtimeStop = prstInfo -> logtimeStop;
    wprstInfo -> pfnCallback = prstInfo -> pfnCallback;
    wprstInfo -> pvCallbackContext = prstInfo -> pvCallbackContext;
Cleanup:
    return wprstInfo;
}


JET_ERR
EsetestUnwidenJET_RSTINFO2(
    __in PSTR   szFunction,
    __in JET_RSTINFO2_W*    wrgstinfo,
    __inout JET_RSTINFO2*   rgstinfo
)
{
    JET_ERR err = JET_errSuccess;
    HRESULT         hr;

    if ( wrgstinfo && rgstinfo )
    {
        // copy rgrstmap
        if ( wrgstinfo->rgrstmap != NULL ) 
        {
            hr = EsetestUnwidenJET_RSTMAP2( szFunction, wrgstinfo->rgrstmap, wrgstinfo->crstmap, rgstinfo->rgrstmap );
            if ( FAILED( hr ) ) 
            {
                tprintf( "%s(): Failed to EsetestUnwidenJET_RSTMAP2(), hr = %d" CRLF, __FUNCTION__, hr );
                err = JET_errTestError;
                goto Cleanup;
            }
        } else
        {
            rgstinfo->rgrstmap = NULL;
        }

        // copy the rest of data
        wrgstinfo -> cbStruct = rgstinfo -> cbStruct;
        wrgstinfo -> crstmap = rgstinfo -> crstmap;
        wrgstinfo -> lgposStop = rgstinfo -> lgposStop;
        wrgstinfo -> logtimeStop = rgstinfo -> logtimeStop;
        wrgstinfo -> pfnCallback = rgstinfo -> pfnCallback;
        wrgstinfo -> pvCallbackContext = rgstinfo -> pvCallbackContext;
    }

Cleanup:
    mdelete( wrgstinfo );
    return err;
}

//----------------DONE
JET_SETSYSPARAM_W*
EsetestWidenJET_SETSYSPARAM(
    __in PSTR   szFunction,
    __in_ecount( csetsysparam ) const JET_SETSYSPARAM*  psetsysparam,
    __in const long         csetsysparam
)
{
    JET_SETSYSPARAM_W* wpsetsysparam = NULL;
    const char* sz = NULL;
    
    // Allocate space for wpsetsysparam
    wpsetsysparam = new JET_SETSYSPARAM_W[ csetsysparam ];
    if ( NULL == wpsetsysparam )
    {
        tprintf( "%s(): Failed to allocate memory for JET_SETSYSPARAM" CRLF, szFunction );
        goto Cleanup;
    }


    for ( int i = 0; i < csetsysparam; i++ ) 
    {
        // copy sz
        sz = psetsysparam[ i ].sz;
        if ( sz != NULL ) {
            wpsetsysparam[ i ].sz = EsetestWidenString( szFunction, sz );       
            if ( NULL == wpsetsysparam[ i ].sz ) 
            {
                tprintf( "%s(): Failed to EsetestWidenString()" CRLF, __FUNCTION__);
                delete[] wpsetsysparam;
                wpsetsysparam = NULL;
                goto Cleanup;
                }
            } else 
            {
                wpsetsysparam[ i ].sz = NULL;
            }
        // copy the rest of data
        wpsetsysparam [ i ].paramid = psetsysparam [ i ].paramid;
        wpsetsysparam [ i ].lParam = psetsysparam [ i ].lParam ;
        wpsetsysparam [ i ].err = psetsysparam [ i ].err;
    }


Cleanup:
    return wpsetsysparam;
}

JET_ERR
EsetestUnwidenJET_SETSYSPARAM(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) JET_SETSYSPARAM_W*       wpsetsysparam,
    __in const long         crstfilemap,
    __inout_ecount( crstfilemap ) JET_SETSYSPARAM*      psetsysparam
)
{
    JET_ERR err = JET_errSuccess;

    // I don't think we need to copy back sysparam data
    /*
    HRESULT         hr;
    wchar_t *wsz = NULL; 
    char *lsz = NULL;
    size_t  cchsz   = 0; 

    for ( int i = 0; i < crstfilemap; i ++ ) {
    
        // copy sz
        //lsz = psetsysparam[ i ].sz ;
        //hr = EsetestUnwidenString( szFunction, wpsetsysparam[ i ].sz, lsz );
        hr = EsetestUnwidenString( szFunction, wpsetsysparam[ i ].sz, psetsysparam[ i ].sz );
        // sz is const char, so we cannot call EsetestUnwidenString 
        // this is a hack
        cchsz =  strlen(  psetsysparam[ i ].sz ) + 1;
        lsz = new char[cchsz];

            hr = StringCchPrintfA( lsz, cchsz, "%ws", wpsetsysparam[ i ].sz);
        psetsysparam[ i ].sz  = lsz;
        //wcscpy (wsz, wpsetsysparam[ i ].sz);
        //hr = EsetestUnwidenString( szFunction, wsz, psetsysparam[ i ].sz );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }       


        // copy the rest of data
        psetsysparam [ i ].paramid = wpsetsysparam [ i ].paramid;
        psetsysparam [ i ].lParam = wpsetsysparam [ i ].lParam ;
        psetsysparam [ i ].err = wpsetsysparam [ i ].err;

    }
*/      
    // free 
    mdelete_array( wpsetsysparam );

//Cleanup:
    return err;
}


//----------------DONE
JET_TABLECREATE_W*
EsetestWidenJET_TABLECREATE(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE* ptablecreate
)
{
    JET_TABLECREATE_W* wptablecreate = NULL;
    const char* szTableName = NULL;
    const char* szTemplateTableName = NULL;
    const JET_COLUMNCREATE* rgcolumncreate = NULL;
    const JET_INDEXCREATE* rgindexcreate = NULL;

    // Allocate space for wptablecreate
    wptablecreate = new JET_TABLECREATE_W;
    if ( NULL == wptablecreate )
    {
        tprintf( "%s(): Failed to allocate memory for JET_TABLECREATE_W" CRLF, szFunction );
        goto Cleanup;
    }

    szTableName = ptablecreate -> szTableName;

    // copy szTableName 
    wptablecreate -> szTableName = EsetestWidenString( szFunction, szTableName );   
    
    if ( NULL == wptablecreate -> szTableName ) 
    {
        tprintf( "%s(): Failed to call EsetestWidenString()" CRLF, __FUNCTION__);
        delete wptablecreate;
        wptablecreate = NULL;
        goto Cleanup;
    }

    // copy szTempleTableName
    szTemplateTableName = ptablecreate -> szTemplateTableName;
    if ( szTemplateTableName != NULL )  
    {
        wptablecreate -> szTemplateTableName = EsetestWidenString( szFunction, szTemplateTableName );       

        if ( NULL == wptablecreate -> szTemplateTableName ) 
        {
            tprintf( "%s(): Failed to EsetestWidenString()" CRLF, __FUNCTION__);
            delete wptablecreate;
            wptablecreate = NULL;
            goto Cleanup;
        }
    } else 
    {
        wptablecreate -> szTemplateTableName = NULL;
    }

    // copy rgcolumncreate
    rgcolumncreate = ptablecreate -> rgcolumncreate;
    if ( rgcolumncreate != NULL )
    {
        wptablecreate -> rgcolumncreate = EsetestWidenJET_COLUMNCREATE(szFunction, rgcolumncreate, ptablecreate ->cColumns);
        if ( NULL == wptablecreate -> rgcolumncreate ) 
        {
            tprintf( "%s(): Failed to EsetestWidenJET_COLUMNCREATE()" CRLF, __FUNCTION__);
            delete rgcolumncreate;
            rgcolumncreate = NULL;
            goto Cleanup;
        }
    } else
    {
        wptablecreate -> rgcolumncreate = NULL;
    }

    // copy rgindexcreate
    rgindexcreate = ptablecreate -> rgindexcreate;

    if ( rgindexcreate != NULL )
    {
        wptablecreate -> rgindexcreate = EsetestWidenJET_INDEXCREATE(szFunction, rgindexcreate , ptablecreate ->cIndexes);
        if ( NULL == wptablecreate -> rgindexcreate ) 
        {
            tprintf( "%s(): Failed to EsetestWidenJET_INDEXCREATE()" CRLF, __FUNCTION__);
            delete rgindexcreate;
            rgindexcreate = NULL;
            goto Cleanup;
        }
    } else
    {
        wptablecreate -> rgindexcreate = NULL;
    }
    
    //copy the rest of data
    wptablecreate ->cbStruct = sizeof ( JET_TABLECREATE_W );
    wptablecreate ->ulPages = ptablecreate ->ulPages;
    wptablecreate ->ulDensity = ptablecreate ->ulDensity;
    wptablecreate ->ulDensity = ptablecreate ->ulDensity;
    wptablecreate ->cColumns = ptablecreate ->cColumns;
    wptablecreate ->cIndexes = ptablecreate ->cIndexes;
    wptablecreate ->grbit = ptablecreate ->grbit;
    wptablecreate ->tableid = ptablecreate ->tableid;
    wptablecreate ->cCreated = ptablecreate ->cCreated;
Cleanup:    
    return wptablecreate;
}

JET_ERR
EsetestUnwidenJET_TABLECREATE(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE_W*   wptablecreate,
    __inout JET_TABLECREATE*  ptablecreate
)
{
    JET_ERR err = JET_errSuccess;
    HRESULT         hr;

    // copy szTableName -> Got AV. SOMEONE feels it is probably read only.
    // only clean it up
    hr = EsetestCleanupWidenString( szFunction, wptablecreate -> szTableName, ptablecreate -> szTableName );
    if ( FAILED( hr ) ) 
    {
        tprintf( "%s(): Failed to EsetestUnwidenString(), hr = %d" CRLF, __FUNCTION__, hr );
        err = JET_errTestError;
        goto Cleanup;
    }
    
    // copy szTempleTableName
    // by the same token, just clean it up
    if ( wptablecreate -> szTemplateTableName != NULL ) 
    {
        hr = EsetestCleanupWidenString( szFunction, wptablecreate -> szTemplateTableName, ptablecreate -> szTemplateTableName );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }
    } else 
    {
        ptablecreate -> szTemplateTableName = NULL;
    }

    if ( wptablecreate -> rgcolumncreate != NULL )
    {
        // copy rgcolumncreate
        hr = EsetestUnwidenJET_COLUMNCREATE( szFunction, wptablecreate -> rgcolumncreate, wptablecreate -> cColumns, ptablecreate -> rgcolumncreate );   
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenJET_COLUMNCREATE(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }
    } else
    {
        ptablecreate -> rgcolumncreate = NULL;
    }

    // copy rgindexcreate
    if (  wptablecreate -> rgindexcreate != NULL )
    {
        hr = EsetestUnwidenJET_INDEXCREATE( szFunction, wptablecreate -> rgindexcreate, wptablecreate ->cIndexes, ptablecreate -> rgindexcreate );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenJET_INDEXCREATE(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }
    } else
    {
        ptablecreate -> rgindexcreate = NULL;
    }

    // copy the rest of data
    ptablecreate ->cbStruct = sizeof ( JET_TABLECREATE );
    ptablecreate ->ulPages = wptablecreate ->ulPages;
    ptablecreate ->ulDensity = wptablecreate ->ulDensity;
    ptablecreate ->ulDensity = wptablecreate ->ulDensity;
    ptablecreate ->cColumns = wptablecreate ->cColumns;
    ptablecreate ->cIndexes = wptablecreate ->cIndexes;
    ptablecreate ->grbit = wptablecreate ->grbit;
    ptablecreate ->tableid = wptablecreate ->tableid;
    ptablecreate ->cCreated = wptablecreate ->cCreated;

Cleanup:
    mdelete( wptablecreate );
    return err;
}


//----------------
JET_TABLECREATE2_W*
EsetestWidenJET_TABLECREATE2(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE2*    ptablecreate2
)
{

    JET_TABLECREATE2_W* wptablecreate2 = NULL;
    const char* szTableName = NULL;
    const char* szTemplateTableName = NULL;
    const JET_COLUMNCREATE* rgcolumncreate = NULL;
    const JET_INDEXCREATE* rgindexcreate = NULL;
    const char* szCallback = NULL;
    
    // Allocate wptablecreate
    wptablecreate2 = new JET_TABLECREATE2_W;
    if ( NULL == wptablecreate2 )
    {
        tprintf( "%s(): Failed to allocate memory for JET_TABLECREATE2_W" CRLF, szFunction );
        goto Cleanup;
    }

    // copy szTableName
    szTableName = ptablecreate2 -> szTableName;
    wptablecreate2 -> szTableName = EsetestWidenString( szFunction, szTableName );  
    if ( NULL == wptablecreate2 -> szTableName ) 
    {
        tprintf( "%s(): Failed to EsetestWidenString() for szTableName" CRLF, __FUNCTION__);
        delete wptablecreate2;
        wptablecreate2 = NULL;
        goto Cleanup;
    }

    // copy szTempleTableName
    szTemplateTableName = ptablecreate2 -> szTemplateTableName;
    if ( szTemplateTableName != NULL ) 
    {
        wptablecreate2 -> szTemplateTableName = EsetestWidenString( szFunction, szTemplateTableName );      
        if ( NULL == wptablecreate2 -> szTemplateTableName ) 
        {
            tprintf( "%s(): Failed to call EsetestWidenString() for szTEmplateTableName" CRLF, __FUNCTION__);
            delete wptablecreate2;
            wptablecreate2 = NULL;
            goto Cleanup;
        }
    } else
    {
        wptablecreate2 -> szTemplateTableName = NULL;
    }

    // copy rgcolumncreate
    rgcolumncreate = ptablecreate2 -> rgcolumncreate;
    if ( rgcolumncreate != NULL )
    {
        wptablecreate2 -> rgcolumncreate = EsetestWidenJET_COLUMNCREATE(szFunction, rgcolumncreate, ptablecreate2 ->cColumns);
        if ( NULL == wptablecreate2 -> rgcolumncreate ) 
        {
            tprintf( "%s(): Failed to EsetestWidenJET_COLUMNCREATE()" CRLF, __FUNCTION__);
            delete wptablecreate2;
            wptablecreate2 = NULL;
            goto Cleanup;
        }
    } else 
    {
        wptablecreate2 -> rgcolumncreate = NULL;
    }

    // copy rgindexcreate
    rgindexcreate = ptablecreate2 -> rgindexcreate;
    if ( rgindexcreate != NULL ) 
    {
        wptablecreate2 -> rgindexcreate = EsetestWidenJET_INDEXCREATE(szFunction, rgindexcreate , ptablecreate2 ->cIndexes);
        if ( NULL == wptablecreate2 -> rgindexcreate ) 
        {
            tprintf( "%s(): Failed to EsetestWidenJET_INDEXCREATE()" CRLF, __FUNCTION__);
            delete wptablecreate2;
            wptablecreate2 = NULL;
            goto Cleanup;
        }
    } else
    {
        wptablecreate2 -> rgindexcreate = NULL;
    }

    szCallback = ptablecreate2 -> szCallback;
    if ( szCallback != NULL )
        {
        wptablecreate2 -> szCallback = EsetestWidenString( szFunction, szCallback );    
        if ( NULL == wptablecreate2 -> szCallback) 
        {
            tprintf( "%s(): Failed to EsetestWidenString() for szCallback" CRLF, __FUNCTION__);
            delete wptablecreate2;
            wptablecreate2 = NULL;
            goto Cleanup;
        }
    } else
    {
        wptablecreate2 -> szCallback = NULL;
    }

    //copy the rest of data
    wptablecreate2 ->cbStruct = sizeof ( JET_TABLECREATE2_W );
    wptablecreate2 ->ulPages = ptablecreate2 ->ulPages;
    wptablecreate2 ->ulDensity = ptablecreate2 ->ulDensity;
    wptablecreate2 ->cColumns = ptablecreate2 ->cColumns;
    wptablecreate2 ->cIndexes = ptablecreate2 ->cIndexes;
    wptablecreate2 ->cbtyp = ptablecreate2 ->cbtyp;
    wptablecreate2 ->grbit = ptablecreate2 ->grbit;
    wptablecreate2 ->tableid = ptablecreate2 ->tableid;
    wptablecreate2 ->cCreated = ptablecreate2 ->cCreated;

Cleanup:    
    return wptablecreate2;

}

JET_ERR
EsetestUnwidenJET_TABLECREATE2(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE2_W*  wptablecreate2,
    __inout JET_TABLECREATE2* const ptablecreate2
)
{
    JET_ERR err = JET_errSuccess;
    HRESULT         hr;

    // copy szTableName
    hr = EsetestCleanupWidenString( szFunction, wptablecreate2 -> szTableName, ptablecreate2 -> szTableName );
    if ( FAILED( hr ) ) 
    {
        tprintf( "%s(): Failed to EsetestCleanupWidenString(), hr = %d" CRLF, __FUNCTION__, hr );
        err = JET_errTestError;
        goto Cleanup;
    }

    // copy szTempleTableName
    if ( wptablecreate2 -> szTemplateTableName != NULL ) {
        hr = EsetestCleanupWidenString( szFunction, wptablecreate2 -> szTemplateTableName, ptablecreate2 -> szTemplateTableName );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestCleanupString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }
    } else {
        ptablecreate2 -> szTemplateTableName = NULL;
    }

    // copy rgcolumncreate
    if ( wptablecreate2 -> rgcolumncreate != NULL )
    {
        hr = EsetestUnwidenJET_COLUMNCREATE( szFunction, wptablecreate2 -> rgcolumncreate, wptablecreate2 -> cColumns, ptablecreate2 -> rgcolumncreate );   
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenJET_COLUMNCREATE(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;

        }
    } else
    {
        ptablecreate2 -> rgcolumncreate = NULL;
    }

    // copy rgindexcreate
    if ( wptablecreate2 -> rgindexcreate != NULL ) 
    {
        hr = EsetestUnwidenJET_INDEXCREATE( szFunction, wptablecreate2 -> rgindexcreate, wptablecreate2 ->cIndexes, ptablecreate2 -> rgindexcreate );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenJET_INDEXCREATE(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }
    } else
    {
        ptablecreate2 -> rgindexcreate = NULL;
    }

    // copy szCallback
    if ( wptablecreate2 -> szCallback != NULL )
    {
        //hr = EsetestUnwidenString( szFunction, wptablecreate2 -> szCallback, ptablecreate2 -> szCallback );
        // AV for unwiden, so only cleanup
        hr = EsetestCleanupWidenString( szFunction, wptablecreate2 -> szCallback, ptablecreate2 -> szCallback );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestCleanupWidenString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
            
        }
    } else 
    {
        ptablecreate2 -> szCallback = NULL;
    }

    // copy the rest of data
    ptablecreate2 ->cbStruct = sizeof ( JET_TABLECREATE2 );
    ptablecreate2 ->ulPages = wptablecreate2 ->ulPages;
    ptablecreate2 ->ulDensity = wptablecreate2 ->ulDensity;
    ptablecreate2 ->cColumns = wptablecreate2 ->cColumns;
    ptablecreate2 ->cIndexes = wptablecreate2 ->cIndexes;
    ptablecreate2 ->cbtyp = wptablecreate2 ->cbtyp;
    ptablecreate2 ->grbit = wptablecreate2 ->grbit;
    ptablecreate2 ->tableid = wptablecreate2 ->tableid;
    ptablecreate2 ->cCreated = wptablecreate2 ->cCreated;

Cleanup:
    mdelete( wptablecreate2 );
    return err;
}

//----------------
JET_TABLECREATE3_W*
EsetestWidenJET_TABLECREATE3(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE3*    ptablecreate3
)
{

    JET_TABLECREATE3_W* wptablecreate3 = NULL;
    const char* szTableName = NULL;
    const char* szTemplateTableName = NULL;
    const JET_COLUMNCREATE* rgcolumncreate = NULL;
    const JET_INDEXCREATE2* rgindexcreate2 = NULL;
    const char* szCallback = NULL;
    
    // Allocate wptablecreate
    wptablecreate3 = new JET_TABLECREATE3_W;
    if ( NULL == wptablecreate3 )
    {
        tprintf( "%s(): Failed to allocate memory for JET_TABLECREATE3_W" CRLF, szFunction );
        goto Cleanup;
    }

    // copy szTableName
    szTableName = ptablecreate3 -> szTableName;
    wptablecreate3 -> szTableName = EsetestWidenString( szFunction, szTableName );  
    if ( NULL == wptablecreate3 -> szTableName ) 
    {
        tprintf( "%s(): Failed to EsetestWidenString() for szTableName" CRLF, __FUNCTION__);
        delete wptablecreate3;
        wptablecreate3 = NULL;
        goto Cleanup;
    }

    // copy szTempleTableName
    szTemplateTableName = ptablecreate3 -> szTemplateTableName;
    if ( szTemplateTableName != NULL ) 
    {
        wptablecreate3 -> szTemplateTableName = EsetestWidenString( szFunction, szTemplateTableName );      
        if ( NULL == wptablecreate3 -> szTemplateTableName ) 
        {
            tprintf( "%s(): Failed to call EsetestWidenString() for szTEmplateTableName" CRLF, __FUNCTION__);
            delete wptablecreate3;
            wptablecreate3 = NULL;
            goto Cleanup;
        }
    } else
    {
        wptablecreate3 -> szTemplateTableName = NULL;
    }

    // copy rgcolumncreate
    rgcolumncreate = ptablecreate3 -> rgcolumncreate;
    if ( rgcolumncreate != NULL )
    {
        wptablecreate3 -> rgcolumncreate = EsetestWidenJET_COLUMNCREATE(szFunction, rgcolumncreate, ptablecreate3 ->cColumns);
        if ( NULL == wptablecreate3 -> rgcolumncreate ) 
        {
            tprintf( "%s(): Failed to EsetestWidenJET_COLUMNCREATE()" CRLF, __FUNCTION__);
            delete wptablecreate3;
            wptablecreate3 = NULL;
            goto Cleanup;
        }
    } else 
    {
        wptablecreate3 -> rgcolumncreate = NULL;
    }

    // copy rgindexcreate2
    rgindexcreate2 = ptablecreate3 -> rgindexcreate;
    if ( rgindexcreate2 != NULL ) 
    {
        wptablecreate3 -> rgindexcreate = EsetestWidenJET_INDEXCREATE2(szFunction, rgindexcreate2 , ptablecreate3 ->cIndexes);
        if ( NULL == wptablecreate3 -> rgindexcreate ) 
        {
            tprintf( "%s(): Failed to EsetestWidenJET_INDEXCREATE2()" CRLF, __FUNCTION__);
            delete wptablecreate3;
            wptablecreate3 = NULL;
            goto Cleanup;
        }
    } else
    {
        wptablecreate3 -> rgindexcreate = NULL;
    }

    szCallback = ptablecreate3 -> szCallback;
    if ( szCallback != NULL )
        {
        wptablecreate3 -> szCallback = EsetestWidenString( szFunction, szCallback );    
        if ( NULL == wptablecreate3 -> szCallback) 
        {
            tprintf( "%s(): Failed to EsetestWidenString() for szCallback" CRLF, __FUNCTION__);
            delete wptablecreate3;
            wptablecreate3 = NULL;
            goto Cleanup;
        }
    } else
    {
        wptablecreate3 -> szCallback = NULL;
    }

    //copy the rest of data
    wptablecreate3 ->cbStruct = sizeof ( JET_TABLECREATE3_W );
    wptablecreate3 ->ulPages = ptablecreate3 ->ulPages;
    wptablecreate3 ->ulDensity = ptablecreate3 ->ulDensity;
    wptablecreate3 ->cColumns = ptablecreate3 ->cColumns;
    wptablecreate3 ->cIndexes = ptablecreate3 ->cIndexes;
    wptablecreate3 ->cbtyp = ptablecreate3 ->cbtyp;
    wptablecreate3 ->grbit = ptablecreate3 ->grbit;
    wptablecreate3 ->tableid = ptablecreate3 ->tableid;
    wptablecreate3 ->cCreated = ptablecreate3 ->cCreated;
    wptablecreate3 ->pSeqSpacehints = ptablecreate3 ->pSeqSpacehints;
    wptablecreate3 ->pLVSpacehints = ptablecreate3 ->pLVSpacehints;
    wptablecreate3 ->cbSeparateLV = ptablecreate3 ->cbSeparateLV;

Cleanup:    
    return wptablecreate3;

}

JET_ERR
EsetestUnwidenJET_TABLECREATE3(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE3_W*  wptablecreate3,
    __inout JET_TABLECREATE3* const ptablecreate3
)
{
    JET_ERR err = JET_errSuccess;
    HRESULT         hr;

    // copy szTableName
    hr = EsetestCleanupWidenString( szFunction, wptablecreate3 -> szTableName, ptablecreate3 -> szTableName );
    if ( FAILED( hr ) ) 
    {
        tprintf( "%s(): Failed to EsetestCleanupWidenString(), hr = %d" CRLF, __FUNCTION__, hr );
        err = JET_errTestError;
        goto Cleanup;
    }

    // copy szTempleTableName
    if ( wptablecreate3 -> szTemplateTableName != NULL ) {
        hr = EsetestCleanupWidenString( szFunction, wptablecreate3 -> szTemplateTableName, ptablecreate3 -> szTemplateTableName );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestCleanupString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }
    } else {
        ptablecreate3 -> szTemplateTableName = NULL;
    }

    // copy rgcolumncreate
    if ( wptablecreate3 -> rgcolumncreate != NULL )
    {
        hr = EsetestUnwidenJET_COLUMNCREATE( szFunction, wptablecreate3 -> rgcolumncreate, wptablecreate3 -> cColumns, ptablecreate3 -> rgcolumncreate );   
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenJET_COLUMNCREATE(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;

        }
    } else
    {
        ptablecreate3 -> rgcolumncreate = NULL;
    }

    // copy rgindexcreate2
    if ( wptablecreate3 -> rgindexcreate != NULL ) 
    {
        hr = EsetestUnwidenJET_INDEXCREATE2( szFunction, wptablecreate3 -> rgindexcreate, wptablecreate3 ->cIndexes, ptablecreate3 -> rgindexcreate );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenJET_INDEXCREATE2(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }
    } else
    {
        ptablecreate3 -> rgindexcreate = NULL;
    }

    // copy szCallback
    if ( wptablecreate3 -> szCallback != NULL )
    {
        //hr = EsetestUnwidenString( szFunction, wptablecreate3 -> szCallback, ptablecreate3 -> szCallback );
        // AV for unwiden, so only cleanup
        hr = EsetestCleanupWidenString( szFunction, wptablecreate3 -> szCallback, ptablecreate3 -> szCallback );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestCleanupWidenString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
            
        }
    } else 
    {
        ptablecreate3 -> szCallback = NULL;
    }

    // copy the rest of data
    ptablecreate3 ->cbStruct = sizeof ( JET_TABLECREATE3 );
    ptablecreate3 ->ulPages = wptablecreate3 ->ulPages;
    ptablecreate3 ->ulDensity = wptablecreate3 ->ulDensity;
    ptablecreate3 ->cColumns = wptablecreate3 ->cColumns;
    ptablecreate3 ->cIndexes = wptablecreate3 ->cIndexes;
    ptablecreate3 ->cbtyp = wptablecreate3 ->cbtyp;
    ptablecreate3 ->grbit = wptablecreate3 ->grbit;
    ptablecreate3 ->tableid = wptablecreate3 ->tableid;
    ptablecreate3 ->cCreated = wptablecreate3 ->cCreated;
    ptablecreate3 ->pSeqSpacehints = wptablecreate3 ->pSeqSpacehints;
    ptablecreate3 ->pLVSpacehints = wptablecreate3 ->pLVSpacehints;
    ptablecreate3 ->cbSeparateLV = wptablecreate3 ->cbSeparateLV;

Cleanup:
    mdelete( wptablecreate3 );
    return err;
}

//----------------
JET_TABLECREATE5_W*
EsetestWidenJET_TABLECREATE5(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE5*    ptablecreate5
)
{

    JET_TABLECREATE5_W* wptablecreate5 = NULL;
    const char* szTableName = NULL;
    const char* szTemplateTableName = NULL;
    const JET_COLUMNCREATE* rgcolumncreate = NULL;
    const JET_INDEXCREATE3* rgindexcreate3 = NULL;
    const char* szCallback = NULL;
    
    // Allocate wptablecreate
    wptablecreate5 = new JET_TABLECREATE5_W;
    if ( NULL == wptablecreate5 )
    {
        tprintf( "%s(): Failed to allocate memory for JET_TABLECREATE5_W" CRLF, szFunction );
        goto Cleanup;
    }

    // copy szTableName
    szTableName = ptablecreate5 -> szTableName;
    wptablecreate5 -> szTableName = EsetestWidenString( szFunction, szTableName );  
    if ( NULL == wptablecreate5 -> szTableName ) 
    {
        tprintf( "%s(): Failed to EsetestWidenString() for szTableName" CRLF, __FUNCTION__);
        delete wptablecreate5;
        wptablecreate5 = NULL;
        goto Cleanup;
    }

    // copy szTempleTableName
    szTemplateTableName = ptablecreate5 -> szTemplateTableName;
    if ( szTemplateTableName != NULL ) 
    {
        wptablecreate5 -> szTemplateTableName = EsetestWidenString( szFunction, szTemplateTableName );      
        if ( NULL == wptablecreate5 -> szTemplateTableName ) 
        {
            tprintf( "%s(): Failed to call EsetestWidenString() for szTEmplateTableName" CRLF, __FUNCTION__);
            delete wptablecreate5;
            wptablecreate5 = NULL;
            goto Cleanup;
        }
    } else
    {
        wptablecreate5 -> szTemplateTableName = NULL;
    }

    // copy rgcolumncreate
    rgcolumncreate = ptablecreate5 -> rgcolumncreate;
    if ( rgcolumncreate != NULL )
    {
        wptablecreate5 -> rgcolumncreate = EsetestWidenJET_COLUMNCREATE(szFunction, rgcolumncreate, ptablecreate5 ->cColumns);
        if ( NULL == wptablecreate5 -> rgcolumncreate ) 
        {
            tprintf( "%s(): Failed to EsetestWidenJET_COLUMNCREATE()" CRLF, __FUNCTION__);
            delete wptablecreate5;
            wptablecreate5 = NULL;
            goto Cleanup;
        }
    } else 
    {
        wptablecreate5 -> rgcolumncreate = NULL;
    }

    // copy rgindexcreate3
    rgindexcreate3 = ptablecreate5 -> rgindexcreate;
    if ( rgindexcreate3 != NULL ) 
    {
        wptablecreate5 -> rgindexcreate = EsetestWidenJET_INDEXCREATE3(szFunction, rgindexcreate3 , ptablecreate5 ->cIndexes);
        if ( NULL == wptablecreate5 -> rgindexcreate ) 
        {
            tprintf( "%s(): Failed to EsetestWidenJET_INDEXCREATE3()" CRLF, __FUNCTION__);
            delete wptablecreate5;
            wptablecreate5 = NULL;
            goto Cleanup;
        }
    } else
    {
        wptablecreate5 -> rgindexcreate = NULL;
    }

    szCallback = ptablecreate5 -> szCallback;
    if ( szCallback != NULL )
        {
        wptablecreate5 -> szCallback = EsetestWidenString( szFunction, szCallback );    
        if ( NULL == wptablecreate5 -> szCallback) 
        {
            tprintf( "%s(): Failed to EsetestWidenString() for szCallback" CRLF, __FUNCTION__);
            delete wptablecreate5;
            wptablecreate5 = NULL;
            goto Cleanup;
        }
    } else
    {
        wptablecreate5 -> szCallback = NULL;
    }

    //copy the rest of data
    wptablecreate5 ->cbStruct = sizeof ( JET_TABLECREATE5_W );
    wptablecreate5 ->ulPages = ptablecreate5 ->ulPages;
    wptablecreate5 ->ulDensity = ptablecreate5 ->ulDensity;
    wptablecreate5 ->cColumns = ptablecreate5 ->cColumns;
    wptablecreate5 ->cIndexes = ptablecreate5 ->cIndexes;
    wptablecreate5 ->cbtyp = ptablecreate5 ->cbtyp;
    wptablecreate5 ->grbit = ptablecreate5 ->grbit;
    wptablecreate5 ->tableid = ptablecreate5 ->tableid;
    wptablecreate5 ->cCreated = ptablecreate5 ->cCreated;
    wptablecreate5 ->pSeqSpacehints = ptablecreate5 ->pSeqSpacehints;
    wptablecreate5 ->pLVSpacehints = ptablecreate5 ->pLVSpacehints;
    wptablecreate5 ->cbSeparateLV = ptablecreate5 ->cbSeparateLV;
    wptablecreate5 ->cbLVChunkMax = ptablecreate5 ->cbLVChunkMax;

Cleanup:    
    return wptablecreate5;

}

JET_ERR
EsetestUnwidenJET_TABLECREATE5(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE5_W*  wptablecreate5,
    __inout JET_TABLECREATE5* const ptablecreate5
)
{
    JET_ERR err = JET_errSuccess;
    HRESULT         hr;

    // copy szTableName
    hr = EsetestCleanupWidenString( szFunction, wptablecreate5 -> szTableName, ptablecreate5 -> szTableName );
    if ( FAILED( hr ) ) 
    {
        tprintf( "%s(): Failed to EsetestCleanupWidenString(), hr = %d" CRLF, __FUNCTION__, hr );
        err = JET_errTestError;
        goto Cleanup;
    }

    // copy szTempleTableName
    if ( wptablecreate5 -> szTemplateTableName != NULL ) {
        hr = EsetestCleanupWidenString( szFunction, wptablecreate5 -> szTemplateTableName, ptablecreate5 -> szTemplateTableName );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestCleanupString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }
    } else {
        ptablecreate5 -> szTemplateTableName = NULL;
    }

    // copy rgcolumncreate
    if ( wptablecreate5 -> rgcolumncreate != NULL )
    {
        hr = EsetestUnwidenJET_COLUMNCREATE( szFunction, wptablecreate5 -> rgcolumncreate, wptablecreate5 -> cColumns, ptablecreate5 -> rgcolumncreate );   
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenJET_COLUMNCREATE(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;

        }
    } else
    {
        ptablecreate5 -> rgcolumncreate = NULL;
    }

    // copy rgindexcreate3
    if ( wptablecreate5 -> rgindexcreate != NULL ) 
    {
        hr = EsetestUnwidenJET_INDEXCREATE3( szFunction, wptablecreate5 -> rgindexcreate, wptablecreate5 ->cIndexes, ptablecreate5 -> rgindexcreate );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenJET_INDEXCREATE3(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }
    } else
    {
        ptablecreate5 -> rgindexcreate = NULL;
    }

    // copy szCallback
    if ( wptablecreate5 -> szCallback != NULL )
    {
        //hr = EsetestUnwidenString( szFunction, wptablecreate5 -> szCallback, ptablecreate5 -> szCallback );
        // AV for unwiden, so only cleanup
        hr = EsetestCleanupWidenString( szFunction, wptablecreate5 -> szCallback, ptablecreate5 -> szCallback );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestCleanupWidenString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
            
        }
    } else 
    {
        ptablecreate5 -> szCallback = NULL;
    }

    // copy the rest of data
    ptablecreate5 ->cbStruct = sizeof ( JET_TABLECREATE5 );
    ptablecreate5 ->ulPages = wptablecreate5 ->ulPages;
    ptablecreate5 ->ulDensity = wptablecreate5 ->ulDensity;
    ptablecreate5 ->cColumns = wptablecreate5 ->cColumns;
    ptablecreate5 ->cIndexes = wptablecreate5 ->cIndexes;
    ptablecreate5 ->cbtyp = wptablecreate5 ->cbtyp;
    ptablecreate5 ->grbit = wptablecreate5 ->grbit;
    ptablecreate5 ->tableid = wptablecreate5 ->tableid;
    ptablecreate5 ->cCreated = wptablecreate5 ->cCreated;
    ptablecreate5 ->pSeqSpacehints = wptablecreate5 ->pSeqSpacehints;
    ptablecreate5 ->pLVSpacehints = wptablecreate5 ->pLVSpacehints;
    ptablecreate5 ->cbSeparateLV = wptablecreate5 ->cbSeparateLV;
    ptablecreate5 ->cbLVChunkMax = wptablecreate5 ->cbLVChunkMax;

Cleanup:
    mdelete( wptablecreate5 );
    return err;
}

//----------------
JET_INDEXCREATE_W*
EsetestWidenJET_INDEXCREATE(
    __in PSTR   szFunction,
    __in_ecount( cindexes ) const JET_INDEXCREATE*  pindexcreate,
    __in const long     cindexes    
)
{
    JET_INDEXCREATE_W* wpindexcreate = NULL;
    const char* szIndexName = NULL;

    // Allocate wpindexcreate
    wpindexcreate = new JET_INDEXCREATE_W [ cindexes ];
    if ( NULL == wpindexcreate )
    {
        tprintf( "%s(): Failed to allocate memory for JET_INDEXCREATE_W" CRLF, szFunction );
        goto Cleanup;
    }

    for ( int i = 0; i < cindexes; i++ ) 
    {
        // copy szIndexName
        szIndexName = pindexcreate[ i ].szIndexName;
        wpindexcreate[ i ].szIndexName = EsetestWidenString( szFunction, szIndexName );
        if ( NULL == wpindexcreate[ i ].szIndexName ) 
        {
            tprintf( "%s(): Failed to EsetestWidenString()" CRLF, __FUNCTION__);
            delete wpindexcreate;
            wpindexcreate = NULL;
            goto Cleanup;
        }
    
        // copy szKey
        // Note: use EsetestWidenCbString()
        if ( NULL == pindexcreate[ i ].szKey ) 
        {
            wpindexcreate[ i ].szKey = NULL;
        } else
        {   
            wpindexcreate[ i ].szKey = EsetestWidenCbString(szFunction, 
                            pindexcreate[ i ].szKey,  
                            pindexcreate[ i ].cbKey  );
            if ( NULL == wpindexcreate[ i ].szKey ) 
            {
                tprintf( "%s(): Failed to EsetestWidenCbString()" CRLF, __FUNCTION__);
                delete wpindexcreate;
                wpindexcreate = NULL;
                goto Cleanup;
            }
        }


        // rgconditionalcolumn
        if ( NULL == pindexcreate[ i ].rgconditionalcolumn )
        {
            wpindexcreate[ i ].rgconditionalcolumn = NULL;
        } else 
        {
            wpindexcreate[ i ].rgconditionalcolumn = 
                            EsetestWidenJET_CONDITIONALCOLUMN(szFunction, 
                                                                pindexcreate[ i ].rgconditionalcolumn,  
                                                                pindexcreate[ i ].cConditionalColumn);
            if ( NULL == wpindexcreate[ i ].rgconditionalcolumn ) 
            {
                tprintf( "%s(): Failed to EsetestWidenJET_CONDITIONALCOLUMN()" CRLF, __FUNCTION__);
                delete wpindexcreate;
                wpindexcreate = NULL;
                goto Cleanup;
            }
        }

        // copy the rest of data
        wpindexcreate[ i ].cConditionalColumn = pindexcreate[ i ].cConditionalColumn;
        wpindexcreate[ i ].cbKey = 2 * pindexcreate[ i ].cbKey;     // double it
        wpindexcreate[ i ].grbit = pindexcreate[ i ].grbit;
        wpindexcreate[ i ].ulDensity = pindexcreate[ i ].ulDensity ;
        wpindexcreate[ i ].err = pindexcreate[ i ].err;
        wpindexcreate[ i ].pidxunicode = pindexcreate[ i ].pidxunicode;
        wpindexcreate[ i ].ptuplelimits = pindexcreate[ i ].ptuplelimits;

        // cbStruct 
        wpindexcreate[ i ].cbStruct = sizeof( JET_INDEXCREATE_W );
        
        // cbkeyMost???
#if ( JET_VERSION >= 0x0600 )
        wpindexcreate[ i ].cbKeyMost = pindexcreate[ i ].cbKeyMost ; 
#endif

    }
Cleanup:
    return wpindexcreate;
}


JET_ERR
EsetestUnwidenJET_INDEXCREATE(
    __in PSTR   szFunction,
    __in_ecount( cindexes ) JET_INDEXCREATE_W*      wpindexcreate,
    __in const long         cindexes,
    __inout_ecount( cindexes ) JET_INDEXCREATE* const   pindexcreate
)
{
    JET_ERR err = JET_errSuccess;
    HRESULT         hr;


    for ( int i = 0; i < cindexes; i++ ) 
    {
        // szIndexName
        //hr = EsetestUnwidenString( szFunction, wpindexcreate[ i ].szIndexName, pindexcreate[ i ].szIndexName ); 
        hr = EsetestCleanupWidenString( szFunction, wpindexcreate[ i ].szIndexName, pindexcreate[ i ].szIndexName ); 
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestCleanupWidenString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }

        // szKey
        //hr = EsetestUnwidenCbString( szFunction, 
        //          wpindexcreate[ i ].szKey, 
        //          wpindexcreate[ i ].cbKey / 2, 
        //          pindexcreate[ i ].szKey );
        // unwiden got AV
        hr = EsetestCleanupWidenString( szFunction, wpindexcreate[ i ].szKey, pindexcreate[ i ].szKey );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestCleanupWidenCbString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }

        // conditionalcolumn
        hr = EsetestUnwidenJET_CONDITIONALCOLUMN( szFunction, 
                        wpindexcreate[ i ].rgconditionalcolumn,  
                        wpindexcreate[ i ].cConditionalColumn,
                        pindexcreate[ i ].rgconditionalcolumn);  
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenJET_CONDITIONALCOLUMN(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }

        pindexcreate[ i ].cConditionalColumn = wpindexcreate[ i ].cConditionalColumn;

        // copy the rest of data
        pindexcreate[ i ].cbKey = wpindexcreate[ i ].cbKey / 2;
        pindexcreate[ i ].grbit = wpindexcreate[ i ]. grbit;
        pindexcreate[ i ].ulDensity = wpindexcreate[ i ].ulDensity ;
        pindexcreate[ i ].err = wpindexcreate[ i ].err;
        pindexcreate[ i ].pidxunicode = wpindexcreate[ i ].pidxunicode;
        pindexcreate[ i ].ptuplelimits = wpindexcreate[ i ].ptuplelimits;

        // cbStruct 
        pindexcreate[ i ].cbStruct = sizeof( JET_INDEXCREATE_A );

        // cbkeyMost???
#if ( JET_VERSION >= 0x0600 )
        pindexcreate[ i ].cbKeyMost = wpindexcreate[ i ].cbKeyMost ; 
#endif
    }
    
Cleanup:
    mdelete_array( wpindexcreate );
    return err;
}

//----------------
JET_INDEXCREATE2_W*
EsetestWidenJET_INDEXCREATE2(
    __in PSTR   szFunction,
    __in_ecount( cindexes ) const JET_INDEXCREATE2* pindexcreate2,
    __in const long     cindexes    
)
{
    JET_INDEXCREATE2_W* wpindexcreate2 = NULL;
    const char* szIndexName = NULL;

    // Allocate wpindexcreate2
    wpindexcreate2 = new JET_INDEXCREATE2_W [ cindexes ];
    if ( NULL == wpindexcreate2 )
    {
        tprintf( "%s(): Failed to allocate memory for JET_INDEXCREATE2_W" CRLF, szFunction );
        goto Cleanup;
    }

    for ( int i = 0; i < cindexes; i++ ) 
    {
        // copy szIndexName
        szIndexName = pindexcreate2[ i ].szIndexName;
        wpindexcreate2[ i ].szIndexName = EsetestWidenString( szFunction, szIndexName );
        if ( NULL == wpindexcreate2[ i ].szIndexName ) 
        {
            tprintf( "%s(): Failed to EsetestWidenString()" CRLF, __FUNCTION__);
            delete wpindexcreate2;
            wpindexcreate2 = NULL;
            goto Cleanup;
        }
    
        // copy szKey
        // Note: use EsetestWidenCbString()
        if ( NULL == pindexcreate2[ i ].szKey ) 
        {
            wpindexcreate2[ i ].szKey = NULL;
        } else
        {   
            wpindexcreate2[ i ].szKey = EsetestWidenCbString(szFunction, 
                            pindexcreate2[ i ].szKey,  
                            pindexcreate2[ i ].cbKey  );
            if ( NULL == wpindexcreate2[ i ].szKey ) 
            {
                tprintf( "%s(): Failed to EsetestWidenCbString()" CRLF, __FUNCTION__);
                delete wpindexcreate2;
                wpindexcreate2 = NULL;
                goto Cleanup;
            }
        }


        // rgconditionalcolumn
        if ( NULL == pindexcreate2[ i ].rgconditionalcolumn )
        {
            wpindexcreate2[ i ].rgconditionalcolumn = NULL;
        } else 
        {
            wpindexcreate2[ i ].rgconditionalcolumn = 
                            EsetestWidenJET_CONDITIONALCOLUMN(szFunction, 
                                                                pindexcreate2[ i ].rgconditionalcolumn,  
                                                                pindexcreate2[ i ].cConditionalColumn);
            if ( NULL == wpindexcreate2[ i ].rgconditionalcolumn ) 
            {
                tprintf( "%s(): Failed to EsetestWidenJET_CONDITIONALCOLUMN()" CRLF, __FUNCTION__);
                delete wpindexcreate2;
                wpindexcreate2 = NULL;
                goto Cleanup;
            }
        }

        // copy the rest of data
        wpindexcreate2[ i ].cConditionalColumn = pindexcreate2[ i ].cConditionalColumn;
        wpindexcreate2[ i ].cbKey = 2 * pindexcreate2[ i ].cbKey;       // double it
        wpindexcreate2[ i ].grbit = pindexcreate2[ i ].grbit;
        wpindexcreate2[ i ].ulDensity = pindexcreate2[ i ].ulDensity ;
        wpindexcreate2[ i ].err = pindexcreate2[ i ].err;
        wpindexcreate2[ i ].pidxunicode = pindexcreate2[ i ].pidxunicode;
        wpindexcreate2[ i ].ptuplelimits = pindexcreate2[ i ].ptuplelimits;
        wpindexcreate2[ i ].cbKeyMost = pindexcreate2[ i ].cbKeyMost ; 
        wpindexcreate2[ i ].pSpacehints = pindexcreate2[ i ].pSpacehints ; 

        // cbStruct
        wpindexcreate2[ i ].cbStruct = sizeof( JET_INDEXCREATE2_W );        
    }
Cleanup:
    return wpindexcreate2;
}


JET_ERR
EsetestUnwidenJET_INDEXCREATE2(
    __in PSTR   szFunction,
    __in_ecount( cindexes ) JET_INDEXCREATE2_W*     wpindexcreate2,
    __in const long         cindexes,
    __inout_ecount( cindexes ) JET_INDEXCREATE2* const  pindexcreate2
)
{
    JET_ERR err = JET_errSuccess;
    HRESULT         hr;


    for ( int i = 0; i < cindexes; i++ ) 
    {
        // szIndexName
        //hr = EsetestUnwidenString( szFunction, wpindexcreate2[ i ].szIndexName, pindexcreate2[ i ].szIndexName ); 
        hr = EsetestCleanupWidenString( szFunction, wpindexcreate2[ i ].szIndexName, pindexcreate2[ i ].szIndexName ); 
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestCleanupWidenString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }

        // szKey
        //hr = EsetestUnwidenCbString( szFunction, 
        //          wpindexcreate2[ i ].szKey, 
        //          wpindexcreate2[ i ].cbKey / 2, 
        //          pindexcreate2[ i ].szKey );
        // unwiden got AV
        hr = EsetestCleanupWidenString( szFunction, wpindexcreate2[ i ].szKey, pindexcreate2[ i ].szKey );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestCleanupWidenCbString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }

        // conditionalcolumn
        hr = EsetestUnwidenJET_CONDITIONALCOLUMN( szFunction, 
                        wpindexcreate2[ i ].rgconditionalcolumn,  
                        wpindexcreate2[ i ].cConditionalColumn,
                        pindexcreate2[ i ].rgconditionalcolumn);  
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenJET_CONDITIONALCOLUMN(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }

        pindexcreate2[ i ].cConditionalColumn = wpindexcreate2[ i ].cConditionalColumn;
        pindexcreate2[ i ].cbKey = wpindexcreate2[ i ].cbKey / 2;
        pindexcreate2[ i ].grbit = wpindexcreate2[ i ]. grbit;
        pindexcreate2[ i ].ulDensity = wpindexcreate2[ i ].ulDensity ;
        pindexcreate2[ i ].err = wpindexcreate2[ i ].err;
        pindexcreate2[ i ].pidxunicode = wpindexcreate2[ i ].pidxunicode;
        pindexcreate2[ i ].ptuplelimits = wpindexcreate2[ i ].ptuplelimits;
        pindexcreate2[ i ].cbKeyMost = wpindexcreate2[ i ].cbKeyMost ; 
        pindexcreate2[ i ].pSpacehints = wpindexcreate2[ i ].pSpacehints ; 

        // cbStruct
        pindexcreate2[ i ].cbStruct = sizeof( JET_INDEXCREATE2_A ); 
    }
    
Cleanup:
    mdelete_array( wpindexcreate2 );
    return err;
}

//----------------
JET_INDEXCREATE3_W*
EsetestWidenJET_INDEXCREATE3(
    __in PSTR   szFunction,
    __in_ecount( cindexes ) const JET_INDEXCREATE3* pindexcreate3,
    __in const long     cindexes    
)
{
    JET_INDEXCREATE3_W* wpindexcreate3 = NULL;
    const char* szIndexName = NULL;

    // Allocate wpindexcreate3
    wpindexcreate3 = new JET_INDEXCREATE3_W [ cindexes ];
    if ( NULL == wpindexcreate3 )
    {
        tprintf( "%s(): Failed to allocate memory for JET_INDEXCREATE3_W" CRLF, szFunction );
        goto Cleanup;
    }

    for ( int i = 0; i < cindexes; i++ ) 
    {
        // copy szIndexName
        szIndexName = pindexcreate3[ i ].szIndexName;
        wpindexcreate3[ i ].szIndexName = EsetestWidenString( szFunction, szIndexName );
        if ( NULL == wpindexcreate3[ i ].szIndexName ) 
        {
            tprintf( "%s(): Failed to EsetestWidenString()" CRLF, __FUNCTION__);
            delete wpindexcreate3;
            wpindexcreate3 = NULL;
            goto Cleanup;
        }
    
        // copy szKey
        // Note: use EsetestWidenCbString()
        if ( NULL == pindexcreate3[ i ].szKey ) 
        {
            wpindexcreate3[ i ].szKey = NULL;
        } else
        {   
            wpindexcreate3[ i ].szKey = EsetestWidenCbString(szFunction, 
                            pindexcreate3[ i ].szKey,  
                            pindexcreate3[ i ].cbKey  );
            if ( NULL == wpindexcreate3[ i ].szKey ) 
            {
                tprintf( "%s(): Failed to EsetestWidenCbString()" CRLF, __FUNCTION__);
                delete wpindexcreate3;
                wpindexcreate3 = NULL;
                goto Cleanup;
            }
        }


        // rgconditionalcolumn
        if ( NULL == pindexcreate3[ i ].rgconditionalcolumn )
        {
            wpindexcreate3[ i ].rgconditionalcolumn = NULL;
        } else 
        {
            wpindexcreate3[ i ].rgconditionalcolumn = 
                            EsetestWidenJET_CONDITIONALCOLUMN(szFunction, 
                                                                pindexcreate3[ i ].rgconditionalcolumn,  
                                                                pindexcreate3[ i ].cConditionalColumn);
            if ( NULL == wpindexcreate3[ i ].rgconditionalcolumn ) 
            {
                tprintf( "%s(): Failed to EsetestWidenJET_CONDITIONALCOLUMN()" CRLF, __FUNCTION__);
                delete wpindexcreate3;
                wpindexcreate3 = NULL;
                goto Cleanup;
            }
        }

        // copy the rest of data
        wpindexcreate3[ i ].cConditionalColumn = pindexcreate3[ i ].cConditionalColumn;
        wpindexcreate3[ i ].cbKey = 2 * pindexcreate3[ i ].cbKey;       // double it
        wpindexcreate3[ i ].grbit = pindexcreate3[ i ].grbit;
        wpindexcreate3[ i ].ulDensity = pindexcreate3[ i ].ulDensity ;
        wpindexcreate3[ i ].err = pindexcreate3[ i ].err;
        wpindexcreate3[ i ].pidxunicode = pindexcreate3[ i ].pidxunicode;
        wpindexcreate3[ i ].ptuplelimits = pindexcreate3[ i ].ptuplelimits;
        wpindexcreate3[ i ].cbKeyMost = pindexcreate3[ i ].cbKeyMost ; 
        wpindexcreate3[ i ].pSpacehints = pindexcreate3[ i ].pSpacehints ; 

        // cbStruct
        wpindexcreate3[ i ].cbStruct = sizeof( JET_INDEXCREATE3_W );        
    }
Cleanup:
    return wpindexcreate3;
}


JET_ERR
EsetestUnwidenJET_INDEXCREATE3(
    __in PSTR   szFunction,
    __in_ecount( cindexes ) JET_INDEXCREATE3_W*     wpindexcreate3,
    __in const long         cindexes,
    __inout_ecount( cindexes ) JET_INDEXCREATE3* const  pindexcreate3
)
{
    JET_ERR err = JET_errSuccess;
    HRESULT         hr;


    for ( int i = 0; i < cindexes; i++ ) 
    {
        // szIndexName
        //hr = EsetestUnwidenString( szFunction, wpindexcreate3[ i ].szIndexName, pindexcreate3[ i ].szIndexName ); 
        hr = EsetestCleanupWidenString( szFunction, wpindexcreate3[ i ].szIndexName, pindexcreate3[ i ].szIndexName ); 
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestCleanupWidenString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }

        // szKey
        //hr = EsetestUnwidenCbString( szFunction, 
        //          wpindexcreate3[ i ].szKey, 
        //          wpindexcreate3[ i ].cbKey / 3, 
        //          pindexcreate3[ i ].szKey );
        // unwiden got AV
        hr = EsetestCleanupWidenString( szFunction, wpindexcreate3[ i ].szKey, pindexcreate3[ i ].szKey );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestCleanupWidenCbString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }

        // conditionalcolumn
        hr = EsetestUnwidenJET_CONDITIONALCOLUMN( szFunction, 
                        wpindexcreate3[ i ].rgconditionalcolumn,  
                        wpindexcreate3[ i ].cConditionalColumn,
                        pindexcreate3[ i ].rgconditionalcolumn);  
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenJET_CONDITIONALCOLUMN(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }

        pindexcreate3[ i ].cConditionalColumn = wpindexcreate3[ i ].cConditionalColumn;
        pindexcreate3[ i ].cbKey = wpindexcreate3[ i ].cbKey / 2;
        pindexcreate3[ i ].grbit = wpindexcreate3[ i ]. grbit;
        pindexcreate3[ i ].ulDensity = wpindexcreate3[ i ].ulDensity ;
        pindexcreate3[ i ].err = wpindexcreate3[ i ].err;
        pindexcreate3[ i ].pidxunicode = wpindexcreate3[ i ].pidxunicode;
        pindexcreate3[ i ].ptuplelimits = wpindexcreate3[ i ].ptuplelimits;
        pindexcreate3[ i ].cbKeyMost = wpindexcreate3[ i ].cbKeyMost ; 
        pindexcreate3[ i ].pSpacehints = wpindexcreate3[ i ].pSpacehints ; 

        // cbStruct
        pindexcreate3[ i ].cbStruct = sizeof( JET_INDEXCREATE3_A ); 
    }
    
Cleanup:
    mdelete_array( wpindexcreate3 );
    return err;
}

//----------------DONE
JET_CONVERT_W*
EsetestWidenJET_CONVERT(
    __in PSTR   szFunction,
    __in const JET_CONVERT* pconvert
)
{
    JET_CONVERT_W* wpconvert = NULL;
    const char* szOldDll = NULL;
        
    // Allocate pconvert
    wpconvert = new JET_CONVERT_W;
    if ( NULL == wpconvert )
    {
        tprintf( "%s(): Failed to allocate memory for JET_CONVERT_W" CRLF, szFunction );
        goto Cleanup;
    }

    // copy szOldDll
    szOldDll = pconvert -> szOldDll;
    wpconvert -> szOldDll = EsetestWidenString( szFunction, szOldDll ); 
    if ( NULL == wpconvert -> szOldDll) 
    {
        tprintf( "%s(): Failed to EsetestWidenString()" CRLF, __FUNCTION__);
        delete wpconvert;
        wpconvert = NULL;
        goto Cleanup;
    }

Cleanup:
    return wpconvert;
}


JET_ERR
EsetestUnwidenJET_CONVERT(
    __in PSTR   szFunction,
    __in JET_CONVERT_W*     wpconvert,
    __inout JET_CONVERT* const  pconvert
)
{
    JET_ERR err = JET_errSuccess;
    HRESULT         hr;
    
    // copy szOldDll
    hr = EsetestUnwidenString( szFunction, wpconvert-> szOldDll, pconvert-> szOldDll );
    if ( FAILED( hr ) ) 
    {
        tprintf( "%s(): Failed to EsetestUnwidenString(), hr = %d" CRLF, __FUNCTION__, hr );
        err = JET_errTestError;
        goto Cleanup;
    }

Cleanup:
    mdelete( wpconvert );
    return err;
}

//----------------DONE
JET_CONDITIONALCOLUMN_W*
EsetestWidenJET_CONDITIONALCOLUMN(
    __in PSTR   szFunction,
    __in_ecount( ccolumns) const JET_CONDITIONALCOLUMN* rgconditionalcolumn,
    __in const long         ccolumns
)
{
    JET_CONDITIONALCOLUMN_W*    wrgconditionalcolumn;

    tprintf (" widen conditionalcolumn" CRLF );
    
    // Allocate spce for wrgcolumncreate
    wrgconditionalcolumn = new JET_CONDITIONALCOLUMN_W[ ccolumns ];
    if ( NULL == wrgconditionalcolumn )
    {
        tprintf( "%s(): Failed to allocate memory for JET_CONDITIONALCOLUMN_W" CRLF, szFunction );
        goto Cleanup;

    }

    for ( int i = 0; i < ccolumns; i++ ) 
    {
        // copy szColumnName
        wrgconditionalcolumn[ i ].szColumnName = EsetestWidenString( szFunction, rgconditionalcolumn[ i ].szColumnName );       
        if ( NULL == wrgconditionalcolumn [ i ].szColumnName ) 
        {
            tprintf( "%s(): Failed to EsetestWidenString()" CRLF, __FUNCTION__);
            delete wrgconditionalcolumn;
            wrgconditionalcolumn = NULL;
            goto Cleanup;
        }

        // copy the rest of data
        wrgconditionalcolumn[ i ].grbit = rgconditionalcolumn[ i ].grbit; 
        wrgconditionalcolumn[ i ].cbStruct = sizeof( JET_CONDITIONALCOLUMN_W );

    } 
Cleanup:
    return wrgconditionalcolumn;
}

JET_ERR
EsetestUnwidenJET_CONDITIONALCOLUMN(
    __in PSTR   szFunction,
    __in JET_CONDITIONALCOLUMN_W*       wrgconditionalcolumn,
    __in const long         ccolumns,
    __inout JET_CONDITIONALCOLUMN* const    rgconditionalcolumn
)
{
    JET_ERR err = JET_errSuccess;
    HRESULT         hr;

    for ( int i = 0; i < ccolumns; i++ ) 
    {
        //hr = EsetestUnwidenString( szFunction, wrgconditionalcolumn[ i ].szColumnName,  rgconditionalcolumn[ i ].szColumnName );
        hr = EsetestCleanupWidenString( szFunction, wrgconditionalcolumn[ i ].szColumnName,  rgconditionalcolumn[ i ].szColumnName );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }
        
        // copy the rest of data
        rgconditionalcolumn[ i ].grbit = wrgconditionalcolumn[ i ].grbit; 
        rgconditionalcolumn[ i ].cbStruct = sizeof( JET_CONDITIONALCOLUMN_A );
    }
Cleanup:
    mdelete( wrgconditionalcolumn );
    return err;
}

    
//----------------DONE
JET_COLUMNCREATE_W*
EsetestWidenJET_COLUMNCREATE(
    __in PSTR   szFunction,
    __in_ecount( ccolumns) const JET_COLUMNCREATE*  rgcolumncreate,
    __in const long         ccolumns
)
{
    JET_COLUMNCREATE_W* wrgcolumncreate;

    // Allocate space for wrgcolumncreate
    wrgcolumncreate = new JET_COLUMNCREATE_W[ ccolumns ];
    if ( NULL == wrgcolumncreate )
    {
        tprintf( "%s(): Failed to allocate memory for JET_COLUMNCREATE_W" CRLF, szFunction );
        goto Cleanup;
    }

    for ( int i = 0; i < ccolumns; i++ ) 
    {
        // copy szColumnName
        wrgcolumncreate[ i ].szColumnName = EsetestWidenString( szFunction, rgcolumncreate[ i ].szColumnName );     
        if ( NULL == wrgcolumncreate[ i ].szColumnName ) 
        {
            tprintf( "%s(): Failed to EsetestWidenString()" CRLF, __FUNCTION__);
            delete wrgcolumncreate;
            wrgcolumncreate = NULL;
            goto Cleanup;
        }

        // copy the rest of data
        wrgcolumncreate[ i ].cbStruct = sizeof( JET_COLUMNCREATE_W );
        wrgcolumncreate[ i ].coltyp = rgcolumncreate[ i ].coltyp; 
        wrgcolumncreate[ i ].cbMax = rgcolumncreate[ i ].cbMax; 
        wrgcolumncreate[ i ].grbit = rgcolumncreate[ i ].grbit; 
        wrgcolumncreate[ i ].cp = rgcolumncreate[ i ].cp; 
        wrgcolumncreate[ i ].columnid = rgcolumncreate[ i ].columnid; 
        wrgcolumncreate[ i ].err = rgcolumncreate[ i ].err; 
        wrgcolumncreate[ i ].pvDefault= rgcolumncreate[ i ].pvDefault; 
        wrgcolumncreate[ i ].cbDefault= rgcolumncreate[ i ].cbDefault; 
        
        
    }
Cleanup:
    return wrgcolumncreate;
}

JET_ERR
EsetestUnwidenJET_COLUMNCREATE(
    __in PSTR   szFunction,
    __in JET_COLUMNCREATE_W*        wrgcolumncreate,
    __in const long         ccolumns,
    __inout JET_COLUMNCREATE* const rgcolumncreate
)
{
    JET_ERR err = JET_errSuccess;
    HRESULT         hr;

    for ( int i = 0; i < ccolumns; i++ ) 
    {
        //hr = EsetestUnwidenString( szFunction, wrgcolumncreate[ i ].szColumnName,  rgcolumncreate[ i ].szColumnName );
        hr = EsetestCleanupWidenString( szFunction, wrgcolumncreate[ i ].szColumnName,  rgcolumncreate[ i ].szColumnName );
        if ( FAILED( hr ) ) 
        {
            tprintf( "%s(): Failed to EsetestUnwidenString(), hr = %d" CRLF, __FUNCTION__, hr );
            err = JET_errTestError;
            goto Cleanup;
        }

        // copy the rest of data
        rgcolumncreate[ i ].cbStruct = sizeof( JET_COLUMNCREATE_A );
        rgcolumncreate[ i ].coltyp = wrgcolumncreate[ i ].coltyp; 
        rgcolumncreate[ i ].cbMax = wrgcolumncreate[ i ].cbMax; 
        rgcolumncreate[ i ].grbit = wrgcolumncreate[ i ].grbit; 
        rgcolumncreate[ i ].cp = wrgcolumncreate[ i ].cp; 
        rgcolumncreate[ i ].columnid = wrgcolumncreate[ i ].columnid; 
        rgcolumncreate[ i ].err = wrgcolumncreate[ i ].err; 
        rgcolumncreate[ i ].pvDefault= wrgcolumncreate[ i ].pvDefault; 
        rgcolumncreate[ i ].cbDefault= wrgcolumncreate[ i ].cbDefault; 
        
    }

Cleanup:
    mdelete( wrgcolumncreate );
    return err;
}

// not done yet
JET_LOGINFO_W*
EsetestWidenJET_LOGINFO(
    __in PSTR   szFunction,
    __in const JET_LOGINFO*     pLogInfo
)
{

    JET_LOGINFO_W* wpLogInfo = NULL;
    HRESULT         hr;
        
    // Allocate memory
    wpLogInfo = new JET_LOGINFO_W;
    if ( NULL == wpLogInfo )
    {
        tprintf( "%s(): Failed to allocate memory for JET_LOGINFO_W" CRLF, szFunction );
        goto Cleanup;
    }

    //wpLogInfo->szBaseName = EsetestWidenString( szFunction, pLogInfo->szBaseName );
    hr = StringCchPrintfW( wpLogInfo->szBaseName, JET_BASE_NAME_LENGTH + 1 , L"%hs", pLogInfo->szBaseName );
    if ( FAILED( hr ) )
    {
        tprintf( "%s(): Failed to StringCchPrintfW(), hr = %d" CRLF, __FUNCTION__, hr );
        delete wpLogInfo;
        wpLogInfo = NULL;
        goto Cleanup;
    }


    wpLogInfo->cbSize = sizeof( JET_LOGINFO_W );
    wpLogInfo->ulGenLow = pLogInfo->ulGenLow;
    wpLogInfo->ulGenHigh = pLogInfo->ulGenHigh;
    
    
Cleanup:
    return wpLogInfo;
}

JET_ERR
EsetestUnwidenJET_LOGINFO(
    __in PSTR   szFunction,
    __in JET_LOGINFO_W* wpLogInfo,
    __inout JET_LOGINFO* const  pLogInfo
)
{
    JET_ERR err = JET_errSuccess;

    if ( !pLogInfo ) 
        goto Cleanup;
    
        
    pLogInfo->cbSize = sizeof( JET_LOGINFO );
    pLogInfo->ulGenLow = wpLogInfo->ulGenLow;
    pLogInfo->ulGenHigh = wpLogInfo->ulGenHigh;
    
    mdelete( wpLogInfo );

Cleanup:
    
    return err;
}


// This function will detect if pindexcreateInUnknownFormat is JET_INDEXCREATE or
// JET_INDEXCREATEOLD, and then depending on which DLL is loaded, output
// pindexcreate in the appropraite structure.
// Return value:
// true: pindexcreateInUnknownFormat has been converted to pindexcreate.
// false: pindexcreateInUnknownFormat was acceptable; pindexcreate is NULL.
JET_ERR
EsetestAdaptJET_INDEXCREATE(
    __in PSTR   szFunction,
    __in_ecount_opt( cIndexCreate ) JET_INDEXCREATE*    rgindexcreateInUnknownFormat,
    unsigned long   cIndexCreate,
    __deref_out_ecount( cIndexCreate ) JET_INDEXCREATE**    prgindexcreate,
    __out bool*     pfAdapted
)
{
    JET_ERR err = JET_errSuccess;
    bool    fOldFormatPassedIn;
    bool    fOldFormatNeeded;
    JET_INDEXCREATEOLD* rgindexcreate   = NULL;

    AssertM( NULL != prgindexcreate );
    AssertM( NULL == *prgindexcreate );

    *pfAdapted = false;
    *prgindexcreate = NULL;

    if ( NULL == rgindexcreateInUnknownFormat )
    {
        // It's not an error to pass in a NULL indexcreate.
        goto Cleanup;
    }

    switch ( rgindexcreateInUnknownFormat->cbStruct )
    {
        case sizeof( JET_INDEXCREATE ):
            fOldFormatPassedIn = false;
            break;
        case sizeof( JET_INDEXCREATEOLD ):
            fOldFormatPassedIn = true;
            break;
        default:
            AssertM( 0 && "Invalid cbStruct" );
            err = JET_errTestError;
            goto Cleanup;
    }

    fOldFormatNeeded = !FEsetestLongKeysSupported();

    if ( fOldFormatNeeded && !fOldFormatPassedIn )
    {
        rgindexcreate = new JET_INDEXCREATEOLD[ cIndexCreate ];
        if ( NULL == rgindexcreate )
        {
            tprintf( "%s(): Failed to allocate memory for JET_INDEXCREATEOLD[ %Id ]" CRLF, szFunction, cIndexCreate );
            err = JET_errOutOfMemory;
            goto Cleanup;
        }

        // Copy the elements over
        for ( size_t i = 0; i < cIndexCreate; ++i )
        {
            C_ASSERT( sizeof( JET_INDEXCREATEOLD ) <= sizeof( JET_INDEXCREATE ) );
            memcpy( &rgindexcreate[ i ], &rgindexcreateInUnknownFormat[ i ], sizeof( rgindexcreate[ 0 ] ) );
            // Since the old structure does not have cbKey, we only need to fix up the cbStruct
            rgindexcreate[ i ].cbStruct = sizeof( rgindexcreate[ i ] );
        }
        *pfAdapted = true;
    }

    *prgindexcreate = reinterpret_cast< JET_INDEXCREATE* >( rgindexcreate );

Cleanup:
    if ( JET_errSuccess != err )
    {
        mdelete_array( rgindexcreate );
        *prgindexcreate = NULL;
    }
    return err;
}

// copies info over from pindexcreate to pindexcreateInUnknownFormat
// pindexcreate will be deleted.
JET_ERR
EsetestUnadaptJET_INDEXCREATE(
    __in PSTR   szFunction,
    __inout_ecount( cIndexCreate ) JET_INDEXCREATE*     pindexcreateNewFormat,      // the user buffer
    unsigned long   cIndexCreate,
    __inout_ecount( cIndexCreate ) JET_INDEXCREATE**    prgindexcreateOldFormat     // has the new data, will get deleted
)
{
    JET_ERR err = JET_errSuccess;

    JET_INDEXCREATEOLD* const       rgindexcreate = reinterpret_cast< JET_INDEXCREATEOLD* >( *prgindexcreateOldFormat );
    // Copy the elements over
    for ( size_t i = 0; i < cIndexCreate; ++i )
    {
        AssertM( rgindexcreate[ i ].cbStruct == sizeof( JET_INDEXCREATEOLD ) );
        AssertM( pindexcreateNewFormat[ i ].cbStruct == sizeof( JET_INDEXCREATE ) );

        AssertM( pindexcreateNewFormat[ i ].szIndexName == rgindexcreate[ i ].szIndexName );
        AssertM( pindexcreateNewFormat[ i ].szKey == rgindexcreate[ i ].szKey );
        AssertM( pindexcreateNewFormat[ i ].cbKey == rgindexcreate[ i ].cbKey );
        AssertM( pindexcreateNewFormat[ i ].grbit == rgindexcreate[ i ].grbit );
        AssertM( pindexcreateNewFormat[ i ].ulDensity == rgindexcreate[ i ].ulDensity );
        AssertM( pindexcreateNewFormat[ i ].lcid == rgindexcreate[ i ].lcid );
        AssertM( pindexcreateNewFormat[ i ].cbVarSegMac == rgindexcreate[ i ].cbVarSegMac );
        AssertM( pindexcreateNewFormat[ i ].rgconditionalcolumn == rgindexcreate[ i ].rgconditionalcolumn );
        AssertM( pindexcreateNewFormat[ i ].cConditionalColumn == rgindexcreate[ i ].cConditionalColumn );

        // I think err is the only writable field.
        pindexcreateNewFormat[ i ].err = rgindexcreate[ i ].err;
    }

    mdelete_array( *prgindexcreateOldFormat );
    return err;
}

 // This function will adapt the JET_TABLECREATE structure to use the proper JET_INDEXCREATE
// structure.
// Return value:
// true: pindexcreateInUnknownFormat has been converted to pindexcreate.
// false: pindexcreateInUnknownFormat was acceptable; pindexcreate is NULL.
JET_ERR
EsetestAdaptJET_TABLECREATE(
    __in PSTR   szFunction,
    __in JET_TABLECREATE*   ptablecreateInNewFormat,
    __deref_out JET_TABLECREATE**   pptablecreate,
    __out bool*     pfAdapted
)
{
    JET_ERR     err     = JET_errSuccess;

    JET_INDEXCREATE*        pindexcreateIn  = ptablecreateInNewFormat->rgindexcreate;
    JET_INDEXCREATE*        pindexcreateOut = NULL;
    JET_TABLECREATE*        ptablecreate    = NULL;
    bool    fAdapted    = false;

    Call( EsetestAdaptJET_INDEXCREATE( szFunction, pindexcreateIn, ptablecreateInNewFormat->cIndexes, &pindexcreateOut, pfAdapted ) );

    fAdapted    = *pfAdapted;
    if ( fAdapted )
    {
        AssertM( NULL != pindexcreateOut );
        ptablecreate = new JET_TABLECREATE;
        if ( NULL == ptablecreate )
        {
            tprintf( "%s(): Failed to allocate memory for JET_TABLECREATE" CRLF, szFunction );
            err = JET_errOutOfMemory;
            goto Cleanup;
        }

        memcpy( ptablecreate, ptablecreateInNewFormat, sizeof( *ptablecreateInNewFormat ) );
        ptablecreate->rgindexcreate = pindexcreateOut;

        *pptablecreate = ptablecreate;
    }

Cleanup:
    if ( err < JET_errSuccess )
    {
        if ( fAdapted )
        {
            const JET_ERR errT = EsetestUnadaptJET_INDEXCREATE( szFunction, pindexcreateIn, ptablecreateInNewFormat->cIndexes, &pindexcreateOut );
            if ( JET_errSuccess != errT )
            {
                tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnadaptJET_INDEXCREATE" );
                if ( JET_errSuccess == err )
                {
                    // Only overwrite err if it's not a success.
                    err = errT;
                }
            }
        }

        mdelete( ptablecreate );
    }

    return err;
}

// copies info over from ptablecreate to ptablecreateInNewFormat
// ptablecreate will be deleted.
JET_ERR
EsetestUnadaptJET_TABLECREATE(
    __in PSTR   szFunction,
    __inout JET_TABLECREATE*    ptablecreateInNewFormat,        // the user buffer
    __inout JET_TABLECREATE**   pptablecreate                   // has the new data, will get deleted
)
{
    JET_ERR     err     = JET_errSuccess;
    AssertM( NULL != ptablecreateInNewFormat );
    AssertM( NULL != pptablecreate );
    AssertM( NULL != *pptablecreate );

    const JET_TABLECREATE*  ptablecreateToRead   = (*pptablecreate);
    AssertM( ptablecreateInNewFormat->szTableName == ptablecreateToRead->szTableName );
    AssertM( ptablecreateInNewFormat->szTemplateTableName == ptablecreateToRead->szTemplateTableName );
    AssertM( ptablecreateInNewFormat->ulPages == ptablecreateToRead->ulPages );
    AssertM( ptablecreateInNewFormat->ulDensity == ptablecreateToRead->ulDensity );
    AssertM( ptablecreateInNewFormat->ulDensity == ptablecreateToRead->ulDensity );
    AssertM( ptablecreateInNewFormat->rgcolumncreate == ptablecreateToRead->rgcolumncreate );
    AssertM( ptablecreateInNewFormat->cColumns == ptablecreateToRead->cColumns );
    AssertM( ptablecreateInNewFormat->cIndexes == ptablecreateToRead->cIndexes );
    AssertM( ptablecreateInNewFormat->grbit == ptablecreateToRead->grbit );

    ptablecreateInNewFormat->tableid  = ptablecreateToRead->tableid;
    ptablecreateInNewFormat->cCreated = ptablecreateToRead->cCreated;


    Call( EsetestUnadaptJET_INDEXCREATE(
        szFunction,
        ptablecreateInNewFormat->rgindexcreate,
        ptablecreateInNewFormat->cIndexes,
        &(*pptablecreate)->rgindexcreate
        ) );

    // Was it deleted properly in EsetestUnadaptJET_INDEXCREATE()??
    AssertM( NULL == ptablecreateToRead->rgindexcreate );

Cleanup:
    // possible leak of rgindexcreate if EsetestUnadaptJET_INDEXCREATE() fails
    mdelete( *pptablecreate );
    return err;
}



// This function will adapt the JET_TABLECREATE structure to use the proper JET_INDEXCREATE
// structure.
// Return value:
// true: pindexcreateInUnknownFormat has been converted to pindexcreate.
// false: pindexcreateInUnknownFormat was acceptable; pindexcreate is NULL.
JET_ERR
EsetestAdaptJET_TABLECREATE2(
    __in PSTR   szFunction,
    __in JET_TABLECREATE2*  ptablecreateInNewFormat,
    __deref_out JET_TABLECREATE2**  pptablecreate,
    __out bool*     pfAdapted
)
{
    JET_ERR     err     = JET_errSuccess;

    JET_INDEXCREATE*        pindexcreateIn  = ptablecreateInNewFormat->rgindexcreate;
    JET_INDEXCREATE*        pindexcreateOut = NULL;
    JET_TABLECREATE2*       ptablecreate    = NULL;
    bool    fAdapted = false;

    Call( EsetestAdaptJET_INDEXCREATE( szFunction, pindexcreateIn, ptablecreateInNewFormat->cIndexes, &pindexcreateOut, pfAdapted ) );

    fAdapted    = *pfAdapted;
    if ( fAdapted )
    {
        AssertM( NULL != pindexcreateOut );
        ptablecreate = new JET_TABLECREATE2;
        if ( NULL == ptablecreate )
        {
            tprintf( "%s(): Failed to allocate memory for JET_TABLECREATE2" CRLF, szFunction );
            err = JET_errOutOfMemory;
            goto Cleanup;
        }

        memcpy( ptablecreate, ptablecreateInNewFormat, sizeof( *ptablecreateInNewFormat ) );
        ptablecreate->rgindexcreate = pindexcreateOut;

        *pptablecreate = ptablecreate;
    }

Cleanup:
    if ( err < JET_errSuccess )
    {
        if ( fAdapted )
        {
            const JET_ERR errT = EsetestUnadaptJET_INDEXCREATE( szFunction, pindexcreateIn, ptablecreateInNewFormat->cIndexes, &pindexcreateOut );
            if ( JET_errSuccess != errT )
            {
                tprintf( "%s():%d Failed to %s()" CRLF, __FUNCTION__, __LINE__, "EsetestUnadaptJET_INDEXCREATE" );
                if ( JET_errSuccess == err )
                {
                    // Only overwrite err if it's not a success.
                    err = errT;
                }
            }
        }

        mdelete( ptablecreate );
    }

    return err;
}

// copies info over from ptablecreate to ptablecreateInNewFormat
// ptablecreate will be deleted.
JET_ERR
EsetestUnadaptJET_TABLECREATE2(
    __in PSTR   szFunction,
    __inout JET_TABLECREATE2*   ptablecreateInNewFormat,        // the user buffer
    __inout JET_TABLECREATE2**  pptablecreate                   // has the new data, will get deleted
)
{
    JET_ERR     err     = JET_errSuccess;
    AssertM( NULL != ptablecreateInNewFormat );
    AssertM( NULL != pptablecreate );
    AssertM( NULL != *pptablecreate );

    const JET_TABLECREATE2* ptablecreateToRead   = (*pptablecreate);
    AssertM( ptablecreateInNewFormat->szTableName == ptablecreateToRead->szTableName );
    AssertM( ptablecreateInNewFormat->szTemplateTableName == ptablecreateToRead->szTemplateTableName );
    AssertM( ptablecreateInNewFormat->ulPages == ptablecreateToRead->ulPages );
    AssertM( ptablecreateInNewFormat->ulDensity == ptablecreateToRead->ulDensity );
    AssertM( ptablecreateInNewFormat->ulDensity == ptablecreateToRead->ulDensity );
    AssertM( ptablecreateInNewFormat->rgcolumncreate == ptablecreateToRead->rgcolumncreate );
    AssertM( ptablecreateInNewFormat->cColumns == ptablecreateToRead->cColumns );
    AssertM( ptablecreateInNewFormat->cIndexes == ptablecreateToRead->cIndexes );
    AssertM( ptablecreateInNewFormat->grbit == ptablecreateToRead->grbit );

    AssertM( ptablecreateInNewFormat->szCallback == ptablecreateToRead->szCallback );
    AssertM( ptablecreateInNewFormat->cbtyp == ptablecreateToRead->cbtyp );

    ptablecreateInNewFormat->tableid  = ptablecreateToRead->tableid;
    ptablecreateInNewFormat->cCreated = ptablecreateToRead->cCreated;

    Call( EsetestUnadaptJET_INDEXCREATE(
        szFunction,
        ptablecreateInNewFormat->rgindexcreate,
        ptablecreateInNewFormat->cIndexes,
        &(*pptablecreate)->rgindexcreate
        ) );

    // Was it deleted properly in EsetestUnadaptJET_INDEXCREATE()??
    AssertM( NULL == ptablecreateToRead->rgindexcreate );

Cleanup:
    // possible leak of rgindexcreate if EsetestUnadaptJET_INDEXCREATE() fails
    mdelete( *pptablecreate );
    return err;
}


JET_COLUMNDEF*
EsetestCompressJET_COLUMNDEF(
    __in const JET_COLUMNDEF*   pcolumndef,
    __out bool*                 pfAllocated
)
{
    JET_COLUMNDEF* pcolumndefReturn = NULL;
    *pfAllocated = false;
    if ( !pcolumndef ||
        ( pcolumndef->coltyp != JET_coltypLongBinary &&
            pcolumndef->coltyp != JET_coltypLongText ) ){
        goto Cleanup;
    }
    pcolumndefReturn = new JET_COLUMNDEF;
    if ( pcolumndefReturn ){
        memcpy( pcolumndefReturn, pcolumndef, sizeof( *pcolumndef ) );
        pcolumndefReturn->grbit |= GrbitColumnCompression( pcolumndefReturn->grbit );
        *pfAllocated = true;
    }

Cleanup:
    if ( !( *pfAllocated ) ){
        pcolumndefReturn = ( JET_COLUMNDEF* )pcolumndef;
    }
    return pcolumndefReturn;
}

JET_COLUMNCREATE*
EsetestCompressJET_COLUMNCREATE(
    __in_ecount_opt(cColumns) const JET_COLUMNCREATE*   pcolumncreate,
    unsigned long                                       cColumns,
    __out bool*                                         pfAllocated
)
{
    JET_COLUMNCREATE* pcolumncreateReturn = NULL;
    *pfAllocated = false;
    if ( !pcolumncreate || !cColumns ){
        goto Cleanup;
    }
    pcolumncreateReturn = new JET_COLUMNCREATE[ cColumns ];
    if ( pcolumncreateReturn ){
        memcpy( pcolumncreateReturn, pcolumncreate, sizeof( *pcolumncreate ) * cColumns );
        for ( unsigned long i = 0 ; i < cColumns ; i++ ){
            if ( pcolumncreateReturn[ i ].coltyp == JET_coltypLongBinary ||
                    pcolumncreateReturn[ i ].coltyp == JET_coltypLongText ){
                pcolumncreateReturn[ i ].grbit |= GrbitColumnCompression( pcolumncreateReturn[ i ].grbit );
            }
        }
        *pfAllocated = true;
    }

Cleanup:
    if ( !( *pfAllocated ) ){
        pcolumncreateReturn = ( JET_COLUMNCREATE* )pcolumncreate;
    }
    return pcolumncreateReturn;
}

JET_SETCOLUMN*
EsetestCompressJET_SETCOLUMN(
    __in_ecount_opt(csetcolumn) const JET_SETCOLUMN*    psetcolumn,
    unsigned long                                       csetcolumn,
    __out bool*                                         pfAllocated
)
{
    JET_SETCOLUMN* psetcolumnReturn = NULL;
    *pfAllocated = false;
    if ( !psetcolumn || !csetcolumn ){
        goto Cleanup;
    }
    psetcolumnReturn = new JET_SETCOLUMN[ csetcolumn ];
    if ( psetcolumnReturn ){
        memcpy( psetcolumnReturn, psetcolumn, sizeof( *psetcolumn ) * csetcolumn );
        for ( unsigned long i = 0 ; i < csetcolumn ; i++ ){
            psetcolumnReturn[ i ].grbit |= GrbitDataCompression( psetcolumnReturn[ i ].grbit );
        }
        *pfAllocated = true;
    }

Cleanup:
    if ( !( *pfAllocated ) ){
        psetcolumnReturn = ( JET_SETCOLUMN* )psetcolumn;
    }
    return psetcolumnReturn;
}

JET_ERR JET_API dummyColumnUserCallBack(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLEID     tableid,
    JET_CBTYP       cbtyp,
    void*           pvArg1,
    void*           pvArg2,
    void*           pvContext,
    ULONG_PTR       ulUnused )
{
    void*   pvData      = pvArg1;
    ULONG   cbData      = *((ULONG*)pvArg2);


    memset( pvData, 0xAA, cbData );
        
    //pvArg1 = NULL;
    // *pcbActual   = 0;
    printf (" hello from dummy callback\n" );
    return JET_errSuccess;
}

//================================================================
//  Helper APIs to compute the Pearson statistical correlation
//  coefficient between two variables.
//  These functions are all thread-safe, except for destroying
//  the object.
//================================================================
//

typedef struct
{
    ULONG64             m_iSample;
    double              m_dblSumSqX;
    double              m_dblSumSqY;
    double              m_dblStdDevX;
    double              m_dblStdDevY;
    double              m_dblCovXY;
    double              m_dblSumCoProduct;
    double              m_dblAvgX;
    double              m_dblAvgY;
    double              m_dblCorrelation;
    HANDLE              m_hQuery;
    HANDLE              m_hThread;
    bool                m_fThreadRunning;
    DWORD               m_dwSamplingPeriod;
} EsePerfPearsonCorrelation;


DWORD WINAPI PearsonCorrelationSampling_( LPVOID lpParameter )
{
    EsePerfPearsonCorrelation* pPCor = (EsePerfPearsonCorrelation*)lpParameter;

    while( pPCor->m_fThreadRunning )
    {
        double dblXY[ 2 ];
        PerfCountersGetCounterValues( pPCor->m_hQuery, dblXY );
        (void)PearsonCorrelationNewSample( (HANDLE)pPCor, dblXY[ 0 ], dblXY[ 1 ] );
        Sleep( pPCor->m_dwSamplingPeriod );
    }

    return 0;
}

HANDLE PearsonCorrelationCreate()
{
    bool fSuccess = true;
    
    //  Allocate the opaque handle.
    EsePerfPearsonCorrelation* pPCor = new EsePerfPearsonCorrelation;
    if ( pPCor == NULL )
    {
        fSuccess = false;
        goto Cleanup;
    }
    memset( (void*)pPCor, 0, sizeof( EsePerfPearsonCorrelation ) );

    pPCor->m_hQuery = NULL;
    pPCor->m_hThread = NULL;
    pPCor->m_fThreadRunning = false;
    pPCor->m_dwSamplingPeriod = 0;
    PearsonCorrelationReset( (HANDLE)pPCor );

Cleanup:
    if ( !fSuccess && pPCor != NULL )
    {
        delete pPCor;
        pPCor = NULL;
    }

    return (HANDLE)pPCor;
}

void PearsonCorrelationReset( HANDLE h )
{
    EsePerfPearsonCorrelation* pPCor = (EsePerfPearsonCorrelation*)h;

    if ( pPCor == NULL )
    {
        return;
    }

    pPCor->m_iSample = 0;
    pPCor->m_dblSumSqX = 0.0;
    pPCor->m_dblSumSqY = 0.0;
    pPCor->m_dblStdDevX = 0.0;
    pPCor->m_dblStdDevY = 0.0;
    pPCor->m_dblCovXY = 0.0;
    pPCor->m_dblSumCoProduct = 0.0;
    pPCor->m_dblAvgX = 0.0;
    pPCor->m_dblAvgY = 0.0;
    pPCor->m_dblCorrelation = 0.0;
}

double PearsonCorrelationNewSample( HANDLE h, const double x, const double y )
{
    EsePerfPearsonCorrelation* pPCor = (EsePerfPearsonCorrelation*)h;
    double dblCurrCor = 0.0;

    if ( pPCor == NULL )
    {
        return 0.0;
    }

    //  We've reached the maximum number of samples, but it doesn't matter. We'll just be slightly
    //  off when computing the new average.
    if ( pPCor->m_iSample + 2 == 0 )
    {
        pPCor->m_iSample--;
    }

    //  First time, we have to initialize the variables.
    if ( pPCor->m_iSample == 0 )
    {
        pPCor->m_dblSumSqX = 0.0;
        pPCor->m_dblSumSqY = 0.0;
        pPCor->m_dblSumCoProduct = 0.0;
        pPCor->m_dblAvgX = x;
        pPCor->m_dblAvgY = y;
    }
    else
    {
        const double dblWeight = (double)pPCor->m_iSample / (double)( pPCor->m_iSample + 1 );
        const double dblDeltaX = x - pPCor->m_dblAvgX;
        const double dblDeltaY = y - pPCor->m_dblAvgY;
        pPCor->m_dblSumSqX += ( dblDeltaX * dblDeltaX * dblWeight );
        pPCor->m_dblSumSqY += ( dblDeltaY * dblDeltaY * dblWeight );
        pPCor->m_dblSumCoProduct += ( dblDeltaX * dblDeltaY * dblWeight );
        pPCor->m_dblAvgX += ( dblDeltaX / (double)( pPCor->m_iSample + 1 ) );
        pPCor->m_dblAvgY += ( dblDeltaY / (double)( pPCor->m_iSample + 1 ) );
    }
    pPCor->m_dblStdDevX = sqrt( pPCor->m_dblSumSqX );
    pPCor->m_dblStdDevY = sqrt( pPCor->m_dblSumSqY );
    pPCor->m_dblCovXY = pPCor->m_dblSumCoProduct;
    const double dblProdT = pPCor->m_dblStdDevX * pPCor->m_dblStdDevY;  
    if ( dblProdT != 0.0 )
    {
        dblCurrCor = pPCor->m_dblCorrelation = pPCor->m_dblSumCoProduct / dblProdT;
    }
    else
    {
        dblCurrCor = pPCor->m_dblCorrelation = 0.0;
    }
    pPCor->m_iSample++;

    return dblCurrCor;
}

bool
PearsonCorrelationStartSampling(
    __in_opt HANDLE     h,
    __in_opt PCSTR      szComputerNameX,
    __in PCSTR          szPerfObjectX,
    __in PCSTR          szPerfCounterX,
    __in PCSTR          szInstanceX,
    __in_opt PCSTR      szComputerNameY,
    __in PCSTR          szPerfObjectY,
    __in PCSTR          szPerfCounterY,
    __in PCSTR          szInstanceY,
    __in DWORD          dwPeriod
)
{
    EsePerfPearsonCorrelation* pPCor = (EsePerfPearsonCorrelation*)h;
    bool fSuccess = false;

    if ( pPCor == NULL )
    {
        return false;
    }

    //  Are we already collecting?
    if ( pPCor->m_hThread != NULL )
    {
        goto Cleanup;
    }

    //  Let's create the collection object.
    pPCor->m_hQuery = PerfCountersCreateQuery();
    if ( pPCor->m_hQuery == NULL ||
            !PerfCountersAddLnCounter( pPCor->m_hQuery, szComputerNameX, szPerfObjectX, szPerfCounterX, szInstanceX ) ||
            !PerfCountersAddLnCounter( pPCor->m_hQuery, szComputerNameY, szPerfObjectY, szPerfCounterY, szInstanceY ) )
    {
        goto Cleanup;
    }

    //  Let's create the thread.
    pPCor->m_dwSamplingPeriod = dwPeriod;
    pPCor->m_fThreadRunning = true;
    pPCor->m_hThread = CreateThread( NULL, 0, PearsonCorrelationSampling_, (LPVOID)pPCor, 0, NULL );
    if ( pPCor->m_hThread == NULL )
    {
        goto Cleanup;
    }

    fSuccess = true;

Cleanup:
    if ( !fSuccess && pPCor->m_hQuery != NULL )
    {
        (void)PerfCountersDestroyQuery( pPCor->m_hQuery );
    }

    return fSuccess;
}

bool
PearsonCorrelationStopSampling(
    __in_opt HANDLE     h
)
{
    EsePerfPearsonCorrelation* pPCor = (EsePerfPearsonCorrelation*)h;
    bool fSuccess = false;

    if ( pPCor == NULL )
    {
        return false;
    }

    //  Are we collecting?
    if ( pPCor->m_hThread == NULL )
    {
        goto Cleanup;
    }

    //  Let's now signal and wait.
    pPCor->m_fThreadRunning = false;
    (void)WaitForSingleObject( pPCor->m_hThread, INFINITE );
    (void)CloseHandle( pPCor->m_hThread );
    pPCor->m_hThread = NULL;
    (void)PerfCountersDestroyQuery( pPCor->m_hQuery );
    pPCor->m_hQuery = NULL;

    fSuccess = true;

Cleanup:
    return fSuccess;
}

double PearsonCorrelationGetCoefficient( HANDLE h )
{
    EsePerfPearsonCorrelation* pPCor = (EsePerfPearsonCorrelation*)h;

    if ( pPCor == NULL )
    {
        return 0.0;
    }

    double dblCurrCor = pPCor->m_dblCorrelation;

    return dblCurrCor;
}

void PearsonCorrelationDestroy( HANDLE h )
{
    EsePerfPearsonCorrelation* pPCor = (EsePerfPearsonCorrelation*)h;

    if ( pPCor == NULL )
    {
        return;
    }

    delete pPCor;
}

//*******************************************************************

