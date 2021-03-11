// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#pragma warning(disable:4985)
#pragma warning(disable:4100)
#pragma warning(disable:4127)
#pragma warning(disable:4512)
#pragma warning(disable:4706)

#ifdef DEBUG
#else
#pragma warning ( disable : 4189 )
#endif

#define Unused( var ) ( var )

#ifndef _ESETEST_4357768_H
#define _ESETEST_4357768_H


#pragma warning( push )
#pragma warning( disable: 4201 )

#include "EtwCollection.hxx"
#include <windows.h>

#ifndef ESE_FLAVOUR_IS_ESENT
#pragma push_macro( "DEFINE_GUID" )
#ifdef DEFINE_GUID
#undef DEFINE_GUID
#endif
#endif
#include <WinIoCtl.h>
#ifndef ESE_FLAVOUR_IS_ESENT
#pragma pop_macro( "DEFINE_GUID" )
#endif


#include <tchar.h>


#pragma warning( push )
#pragma warning( disable : 4201 )
#if defined(BUILD_ENV_IS_NT) || defined(BUILD_ENV_IS_WPHONE)
#include <esent_x.h>
#endif
#ifdef BUILD_ENV_IS_EX
#include <jet.h>
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef JetRemoveLogfile
JET_ERR JET_API
JetRemoveLogfileA(
    __in JET_PCSTR szDatabase,
    __in JET_PCSTR szLogfile,
    __in JET_GRBIT grbit );

#if ( JET_VERSION >= 0x0600 )

JET_ERR JET_API
JetRemoveLogfileW(
    __in wchar_t* szDatabase,
    __in wchar_t* szLogfile,
    __in JET_GRBIT grbit );

#ifdef JET_UNICODE
#define JetRemoveLogfile JetRemoveLogfileW
#else
#define JetRemoveLogfile JetRemoveLogfileA
#endif
#endif
#endif

#ifdef  __cplusplus
}
#endif

#pragma warning( pop )

#define SUCCESS 0
#define FAILURE     ~SUCCESS

#define SZNEWLINE       "\r\n"
#define WSZNEWLINE  L"\r\n"

#define CRLF        "\r\n"
#define WCRLF       L"\r\n"
#define TCRLF       _T( CRLF )

#define TAB         "\t"
#define WTAB        L"\t"
#define TTAB        _T( TAB )

#define ANSI_BLACK      0

#define ANSI_GRAY       FOREGROUND_INTENSITY
#define ANSI_SILVER ( FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE )

#define ANSI_RED        ( FOREGROUND_INTENSITY | FOREGROUND_RED )
#define ANSI_GREEN      ( FOREGROUND_INTENSITY | FOREGROUND_GREEN )
#define ANSI_BLUE       ( FOREGROUND_INTENSITY | FOREGROUND_BLUE )

#define ANSI_YELLOW ( ANSI_RED | ANSI_GREEN )
#define ANSI_MAGENTA    ( ANSI_RED | ANSI_BLUE )
#define ANSI_CYAN       ( ANSI_GREEN | ANSI_BLUE )

#define ANSI_WHITE      ( ANSI_RED | ANSI_GREEN | ANSI_BLUE )

#define CALLJET(func)                                               \
    jErr = (func);                                                  \
    if ( jErr < JET_errSuccess )                                    \
    {                                                               \
        tcprintf( ANSI_RED, "Failed while calling function: %s, error: %ld\r\n", #func, jErr);  \
        tcprintf( 10, "[%s line %d]\r\n", __FILE__, __LINE__ );     \
        goto LError;                                                \
    }


#define CALLJET2(func)                                              \
    jErr = (func);                                                  \
    if ( jErr != JET_errSuccess )                                   \
    {                                                               \
        tcprintf( ANSI_RED, "Failed while calling function: %s, error: %ld\r\n", #func, jErr);  \
        tcprintf( 10, "[%s line %d]\r\n", __FILE__, __LINE__ );     \
        goto LError;                                                \
    }


#define CALLJETERR(expected,func)                                   \
    jErr = (func);                                                  \
    if ( jErr != (expected) )                                           \
    {                                                               \
        tcprintf( ANSI_RED, "Failed while calling function: %s, expected: %ld  actual: %ld\r\n", #func, (expected), jErr);  \
        tcprintf( 10, "[%s line %d]\r\n", __FILE__, __LINE__ );     \
        goto LError;                                                \
    }                                                               \
    else                                                            \
    {                                                               \
        tprintf( "Received expected JET error while calling function: %s, error: %ld\r\n", #func, jErr);        \
    }


#if ( !defined( ESE_FLAVOUR_IS_ESENT ) && !defined( ESE_FLAVOUR_IS_ESE ) )
#  error You should define either ESE_FLAVOUR_IS_ESE or ESE_FLAVOUR_IS_ESENT
#  define ESE_FLAVOUR_IS_ESE
#  define ESE_FLAVOUR_IS_ESENT
#endif

#if ( defined( ESE_FLAVOUR_IS_ESENT ) && defined( ESE_FLAVOUR_IS_ESE ) )
#  error You should not have both ESE_FLAVOUR_IS_ESE and ESE_FLAVOUR_IS_ESENT defined!
#endif

#ifdef ESE_FLAVOUR_IS_ESE
#  define ESE_FLAVOUR       "ese"
#  define WESE_FLAVOUR      L"ese"
#  define   SZESEUTIL   "eseutil.exe"
#  define   WSZESEUTIL  L"eseutil.exe"
#endif

#ifdef ESE_FLAVOUR_IS_ESENT
#  define ESE_FLAVOUR       "esent"
#  define WESE_FLAVOUR      L"esent"
#  define   SZESEUTIL   "esentutl.exe"
#  define   WSZESEUTIL  L"esentutl.exe"
#endif

const char*
SzEsetestEseutil()
;

const wchar_t*
WszEsetestEseutil()
;

const char*
SzEsetestEseDll()
;

const wchar_t*
WszEsetestEseDll()
;

const char*
SzEsetestEsebackDll()
;

const wchar_t*
WszEsetestEsebackDll()
;

const char*
SzEsetestEsebcliDll()
;

const wchar_t*
WszEsetestEsebcliDll()
;

const char*
SzEsetestEseperfDll()
;

const wchar_t*
WszEsetestEseperfDll()
;

const char*
SzEsetestEseperfIni()
;

const wchar_t*
WszEsetestEseperfIni()
;

const char*
SzEsetestEseperfHxx()
;

const wchar_t*
WszEsetestEseperfHxx()
;

const char*
SzEsetestEseperfDatabase()
;

const wchar_t*
WszEsetestEseperfDatabase()
;

const char*
SzEsetestEseperfDatabaseInstances()
;

const wchar_t*
WszEsetestEseperfDatabaseInstances()
;

const char*
SzEsetestEseperfDatabaseTableClasses()
;

const wchar_t*
WszEsetestEseperfDatabaseTableClasses()
;

const char*
SzEsetestEseperfDatabaseDatabases()
;

const wchar_t*
WszEsetestEseperfDatabaseDatabases()
;

const char*
SzEsetestEseEventSource()
;

const wchar_t*
WszEsetestEseEventSource()
;

const char*
SzEsetestEsebackEventSource()
;

const wchar_t*
WszEsetestEsebackEventSource()
;

const char*
SzEsetestGetStartDotLog();

const wchar_t*
WszEsetestGetStartDotLog();

const char*
SzEsetestGetStartDotChk();

const wchar_t*
WszEsetestGetStartDotChk();

const char*
SzEsetestGetChkptName(
    __out_ecount( cch ) char*   szBuffer,
    __in size_t                 cch
);

const wchar_t*
WszEsetestGetChkptName(
    __out_ecount( cch ) wchar_t*    wszBuffer,
    __in size_t                     cch
);

char*
SzEsetestGetLogNameTemp(
    __out_ecount( cch ) char*   szBuffer,
    __in size_t                 cch
);

wchar_t*
WszEsetestGetLogNameTemp(
    __out_ecount( cch ) wchar_t*    wszBuffer,
    __in size_t                     cch
);

char*
SzEsetestGetLogNameLast(
    __out_ecount( cch ) char*   szBuffer,
    __in size_t                 cch
);

wchar_t*
WszEsetestGetLogNameLast(
    __out_ecount( cch ) wchar_t*    wszBuffer,
    __in size_t                     cch
);

char*
SzEsetestGetLogNameFromGen(
    __out_ecount( cch ) char*   szBuffer,
    __in size_t                 cch,
    __in unsigned long          ulGen
);

wchar_t*
WszEsetestGetLogNameFromGen(
    __out_ecount( cch ) wchar_t*    wszBuffer,
    __in size_t                     cch,
    __in unsigned long              ulGen
);

long
EsetestGetLogGenFromNameA(
    __in const char* szLogName
);

long
EsetestGetLogGenFromNameW(
    __in const wchar_t* wszLogName
);

#ifdef _UNICODE
#define EsetestGetLogGenFromName EsetestGetLogGenFromNameW
#else
#define EsetestGetLogGenFromName EsetestGetLogGenFromNameA
#endif

const char*
SzEsetestPerfSummaryXml();

const wchar_t*
WszEsetestPerfSummaryXml();

int
IEsetestGetPageSize()
;

bool
FEsetestCallSystemParametersBeforeInit()
;

void
EsetestSetCallSystemParametersBeforeInit(
    bool    fCallSystemParameters
    )
;

const char*
SzEsetestEseutil()
;

const wchar_t*
WszEsetestEseutil()
;

void
EsetestSetVerboseInternalLogging(
    bool    fVerboseInternalLogging
)
;

void
EsetestSetRunningStress(
    __in const bool fRunningStress
)
;



bool
FEsetestScrubbingSupported()
;

bool
FEsetestSlvSupported()
;

bool
FEsetestMultiInstanceSupported()
;

bool
FEsetestPrepInsertCopyDeleteOriginalSupported()
;

bool
FEsetestParamCacheSizeMaxSupported()
;

bool
FEsetestPageScrubbingSupported()
;

bool
FEsetestBackupSupported()
;

bool
FEsetestMultiInstanceBackupSupported()
;

bool
FEsetestLongKeysSupported()
;

bool
FEsetestTracingSupported()
;

enum EseFeaturePresent {
    EseFeatureEightK,
    EseFeatureSlv_Obsolete,
    EseFeatureMultiInstance,
    EseFeaturePrepInsertCopyDeleteOriginal,
    EseFeatureParamCacheSizeMax,
    EseFeatureBackup,
    EseFeatureMultiInstanceBackup,
    EseFeatureLongKeys,
    EseFeatureTracing,
    EseFeatureTempTableTTForwardOnly,
    EseFeatureJetInit3,
    EseFeature2GLogs,
    EseFeatureApisExportedWithA,
    EseFeatureVSSIncrementalBackup,
    EseFeatureVSSDifferentialBackup,
    EseFeatureVSSParallelBackups,
    EseFeatureLostLogResilience,
    EseFeatureCachePercPinnedPerfCtr,
    EseFeatureParamCheckpointTooDeep,
    EseFeatureParamAggressiveLogRollover,
    EseFeatureParamLogRollover,
    EseFeatureEsebackAbort,
    EseFeatureCopyDiffBkpHeader,
    EseFeatureLogFilesGeneratedPerfCounter,
    EseFeatureNewPerfCountersNaming,
    EseFeatureIncrementalReseedApis,
    EseFeatureSpaceDiscoveryApis,
    EseFeatureSpaceHintsApis,
    EseFeatureLargePageSize,
    EseFeatureOLD2,
    EseFeatureDBM,
    EseFeatureSkippableAsserts,
    EseFeatureLVCompression,
    EseFeatureWriteCoalescing,
    EseFeatureReadCoalescing,
    EseFeatureSeparateIOQueuesCtrs,
    EseFeatureChksumInRecov,
    EseFeatureParamMaxRandomIOSizeDeprecated,
    EseFeatureCachePriority,
    EseFeatureBFPerfCtrsRefactoring,
    EseFeatureHyperCacheDBA,
    EseFeatureLogGenPerfCtrsInRecovery,
    EseFeatureLogRewrite,
    EseFeatureDatabaseShrink,
    EseFeatureScrubbingOnOff,
    EseFeatureMax,
};

bool
FEsetestFeaturePresent(
    __in const EseFeaturePresent    feature
)
;

size_t
EsetestColtypMax()
;

size_t
EsetestCColKeyMost()
;

enum
EsetestEseFlavour {
    EsetestEseFlavourNil        = 0,
    EsetestEseFlavourEsent      = 1,
    EsetestEseFlavourEse        = 2,
    EsetestEseFlavourEseMax,
}
;







typedef ULONG64 EsetestEseVersion;

JET_ERR
EsetestEseVersionToParts(
    __in EsetestEseVersion      version,
    __out EsetestEseFlavour*    pexornt,
    __out PUINT                 pmajor,
    __out PUINT                 pminor,
    __out PUINT                 pspnumber,
    __out PUINT                 pbuildno
)
;

JET_ERR
EsetestEseFileVersionToParts(
    __in EsetestEseVersion      version,
    __out EsetestEseFlavour*    pexornt,
    __out PUINT                 pmajor,
    __out PUINT                 pminor,
    __out PUINT                 pbuildno,
    __out PUINT                 pprivateno
)
;

EsetestEseFlavour
EsetestGetEseFlavour()
;

JET_ERR
EsetestGetEseVersion(
    __out EsetestEseFlavour*    pflavour,
    __out EsetestEseVersion*    peseversion,
    __out bool*                 pfChecked
)
;

JET_ERR
EsetestGetEseVersionParts(
    __out   EsetestEseFlavour*  pflavour,
    __out   PUINT               pverMajor,
    __out   PUINT               pverMinor,
    __out   PUINT               pverSp,
    __out   PUINT               pverBuild,
    __out   bool*               pfChecked
)
;

JET_ERR
EsetestGetEseFileVersionParts(
    __out   EsetestEseFlavour*  pflavour,
    __out   PUINT               pverMajor,
    __out   PUINT               pverMinor,
    __out   PUINT               pverBuild,
    __out   PUINT               pverPrivate,
    __out   bool*               pfChecked
)
;

bool
FEsetestIsBugFixed(
    __in    UINT                bugnumberEse,
    __in    UINT                bugnumberEsent
)
;

bool
FEsetestVerifyVersion(
    __in    EsetestEseFlavour   flavour,
    __in    UINT                verMajor,
    __in    UINT                verMinor,
    __in    UINT                verSp,
    __in    UINT                verBuild
)
;

bool
FEsetestWidenParameters()
;

void
EsetestSetWidenParametersPercent(
    const size_t    pctApisToWiden
)
;

JET_GRBIT GrbitColumnCompression( JET_GRBIT grbit )
;

JET_GRBIT GrbitDataCompression( JET_GRBIT grbit )
;

void
EsetestSetCompressionPercent(
    const int   pctCompression
)
;

void
EsetestSetAssertAction(
    const JET_API_PTR esetestAssertAction
)
;

HMODULE
HmodEsetestEseDll()
;

HMODULE
HmodEsetestEsebackDll()
;


ULONG64
QWEsetestQueryPerformanceFrequency()
;

ULONG64
QWEsetestQueryPerformanceCounter()
;

#ifndef ESE_REGISTER_BACKUP
#define ESE_REGISTER_BACKUP             0x00000001
#endif

typedef struct              
    {
    unsigned long   cbStruct;   
    JET_SNC         snc;        
    unsigned long   ul;         
    char            sz[256];    
    } JET_SNMSG_A;

typedef struct              
    {
    unsigned long   cbStruct;   
    JET_SNC         snc;        
    unsigned long   ul;         
    WCHAR           sz[256];    
    } JET_SNMSG_W;


#ifdef  __cplusplus
extern "C" {
#endif
JET_ERR JET_API JetBeginSurrogateBackup(
    __in    JET_INSTANCE    instance,
    __in        unsigned long       lgenFirst,
    __in        unsigned long       lgenLast,
    __in        JET_GRBIT       grbit );
JET_ERR JET_API JetEndSurrogateBackup(
    __in    JET_INSTANCE    instance,
    __in        JET_GRBIT       grbit );

#ifdef  __cplusplus
}
#endif

#define JET_errSurrogateBackupInProgress    -617

void FormatLastError(
    _Out_writes_z_( iLength ) char* szTemp,
    _In_ const int iLength
);

void FormatSpecificError(
    __in_ecount( iLength ) char* szTemp,
    _In_ const int iLength,
    _In_ const DWORD dwError
);

#ifndef ESETEST_NO_OPERATOR_NEW


void*
PvMemoryHeapAlloc(
    __in const size_t   cbSize
)
;
void
MemoryHeapFree(
    __in_opt void* const    pv
)
;

inline void* __cdecl operator new( const size_t cbSize )
{
    return PvMemoryHeapAlloc( cbSize );
}
inline void* __cdecl operator new[]( const size_t cbSize )
{
    return PvMemoryHeapAlloc( cbSize );
}

inline void __cdecl operator delete( void* const pv )
{
    MemoryHeapFree( pv );
}
inline void __cdecl operator delete[]( void* const pv )
{
    MemoryHeapFree( pv );
}


#endif





void
EsetestSetResultsTxt(
    __in const char*    szResultsTxt
)
;

void
EsetestSetResultsLog(
    __in const char*    szResultsLog
)
;

void InitLogging( BOOL fLogToDisk = TRUE );
void TermLogging( void );

BOOL EsetestSetLoggingToDisk( BOOL fLogToDisk );
BOOL EsetestGetLoggingToDisk();
BOOL EsetestDisableWritingToResultsTxt();

int
InitTest(
    __in_opt const char *szCommand = NULL,
    __in BOOL fLogToDisk = TRUE,
    __in BOOL fWriteFailed = TRUE
    );
int InitTestArgv(
    const int argc,
    __in_ecount_opt( argc ) const char* const argv[],
    BOOL fLogToDisk = TRUE,
    BOOL fWriteFailed = TRUE
    );
int TermTest( int nSuccess );
int ChangeResult( int nSuccess );
int TermTestMaybeChangeResult( int nSuccess, BOOL fChangeResult );
void NTError( BOOL fLogToDisk = TRUE );

int tprintf(
    _In_ _Printf_format_string_ const char* szFormat,
    ...
);


int tprintfnothid(
    _In_ _Printf_format_string_ const char* szFormat,
    ...
);



int
tprintfPerfLog(
    _In_ _Printf_format_string_ const char* szFormat,
    ...
)
;

int tprintfResultsTxtUseWithCaution(
    _In_ _Printf_format_string_ const char* szFormat,
    ...
);

int tcprintf(
    _In_ DWORD dColour,
    _In_ _Printf_format_string_ const char* szFormat,
    ...
);

#define Esetest_bitLogDoNotLog          NO_GRBIT
#define Esetest_bitLogToScreen          0x1
#define Esetest_bitLogToDisk            0x2
#define Esetest_bitLogToDiskAndScreen   ( Esetest_bitLogToScreen | Esetest_bitLogToDisk )
#define Esetest_bitLogDoNotLogThreadId      0x4
#define Esetest_bitLogMax               0x8

int EsetestVprintf(
    __in_opt const char*                szLogFile,
    __in const JET_GRBIT            level,
    __in _Printf_format_string_ const char*             szFormat,
    __in va_list                    ap
    )
;

void FreeColumnData(
    __in_ecount( ccolumns ) JET_SETCOLUMN* pjsetcolumn,
    LONG ccolumns
    );
void FreeRetrieveColumn(
    __in_ecount( ccolumns ) JET_RETRIEVECOLUMN* pjretrievecolumn,
    LONG ccolumns
    );
BOOL IsColumnDataEqual(
    __in_ecount( ccolumns ) JET_SETCOLUMN* pjsetcolumn,
    __in_ecount( ccolumns ) JET_RETRIEVECOLUMN* pjretrievecolumn,
    ULONG ccolumns
    );



PTCHAR ParseResultsMemory(  const PTCHAR szSrc,
                                const PTCHAR szToken,
                                const PTCHAR szTokenBefore,
                                const PTCHAR szTokenAfter );
PTCHAR ParseResultsFile(    const PTCHAR szFileName,
                            const PTCHAR szToken,
                            const PTCHAR szTokenBefore,
                            const PTCHAR szTokenAfter );
void FreeParsedStr( PTCHAR szPtr );

BYTE* EsetestCircularMemCopy( BYTE* pbDst, size_t cbDst, const BYTE* pbSrc, size_t cbSrc, size_t iSrcOffset );


int FillRandom(
    __in_bcount( cbData ) BYTE* pbData,
    ULONG cbData
    );
long Rand( void );
long RandRange( long min, long max );
__int64 Rand64( void );
__int64 Rand64Range( __int64 min, __int64 max );
long GetStandardSeed( void );
void SeedStandardRand( long seed );
long StandardRand( long& lCurrentSeed );

int FillRandomUnaligned(
    __in_bcount( cbData ) BYTE *pbData,
    ULONG cbData
    );


const TCHAR g_tszRegPathDebug[]             = _T( "Debug" );
const TCHAR g_tszRegPathOs[]                = _T( "OS" );

const TCHAR g_tszRegKeyBuild[]              = _T( "BuildNumber" );
const TCHAR g_tszRegKeyPibFailures[]        = _T( "system pib failures" );
const WCHAR g_wszRegKeyOpenPerfData[]       = L"Open";
const WCHAR g_wszRegKeyOpenPerfDataVal[]    = L"OpenPerformanceData";
const WCHAR g_wszRegKeyCollectPerfData[]    = L"Collect";
const WCHAR g_wszRegKeyCollectPerfDataVal[] = L"CollectPerformanceData";
const WCHAR g_wszRegKeyClosePerfData[]      = L"Close";
const WCHAR g_wszRegKeyClosePerfDataVal[]   = L"ClosePerformanceData";
const WCHAR g_wszRegKeyPerfLibrary[]        = L"Library";
const WCHAR g_wszRegKeyAdvancedPerfCtrs[]   = L"Show Advanced Counters";


LONG EsetestRegGetValueA(
    __in         HKEY hkey,
    __in_opt     LPCTSTR pszSubKey,
    __in_opt     LPCTSTR pszValue,
    __in_opt     DWORD dwFlags,
    __out_opt    LPDWORD pdwType,
    __out_opt    PVOID pvData,
    __inout_opt  LPDWORD pcbData
    )
;

JET_ERR
ErrEsetestWriteRegistry(
    IN const HKEY           hkeyHive,
    __in const TCHAR*       szKeyPath,
    __in const TCHAR*       szKeyName,
    __in const TCHAR*       szValue
    )
;

JET_ERR
ErrEsetestReadRegistry(
    IN const HKEY       hkeyHive,
    __in const TCHAR*   szKeyPath,
    __in const TCHAR*   szKeyName,
    __out_bcount_part( *pcbValue, *pcbValue ) TCHAR* const  szValue,
    IN OUT  PDWORD      pcbValue
    )
;

JET_ERR
ErrEsetestWriteConfigToEsentGlobal(
    __in const TCHAR* szKeyPath,
    __in const TCHAR* szKeyName,
    __in const TCHAR* szValue
    )
;

JET_ERR
ErrEsetestReadConfigToEsentGlobal(
    __in const TCHAR*   szKeyPath,
    __in const TCHAR*   szKeyName,
    __out_bcount_part( *pcbValue, *pcbValue ) TCHAR* const  szValue,
    __inout PDWORD      pcbValue
    )
;

JET_ERR
ErrEsetestCopyKey(
    __in const WCHAR*   wszKeySrc,
    __in const WCHAR*   wszKeyDst,
    __in BOOL           fDelSrc,
    __in BOOL           fDelDst
    )
;


VOID SprintHex(
    __out CHAR * const      szDest,
    __in_bcount( cbSrc ) const BYTE * const rgbSrc,
    __in const INT          cbSrc,
    __in const INT          cbWidth     = 16,
    __in const INT          cbChunk     = 4,
    __in const INT          cbAddress   = 8,
    __in const INT          cbStart     = 0
)
;

void
EsetestWaitForAllObjectsInfinitely(
    __in DWORD cCount,
    __in_ecount( cCount ) const HANDLE* rgHandles
)
;

BOOL
EsetestCreateDirectoryA(
    __in PCSTR      szName
)
;

BOOL
EsetestCreateDirectoryW(
    __in PCWSTR     wszName
)
;

BOOL
EsetestRemoveDirectoryA(
    __in PCSTR      szName
)
;

BOOL
EsetestRemoveDirectoryW(
    __in PCWSTR     wszName
)
;

BOOL
EsetestCopyDirectoryA(
    __in PCSTR      szNameSrc,
    __in PCSTR      szNamedst
)
;

BOOL
EsetestCopyDirectoryW(
    __in PCWSTR     wszNameSrc,
    __in PCWSTR     wszNameDst
)
;

BOOL
EsetestEnsureFullPathA(
    __inout PSTR        szPath
)
;

BOOL
EsetestEnsureFullPathW(
    __inout PWSTR       wszPath
)
;

BOOL
EsetestEnsureFullPathExistsA(
    __in PCSTR      szPath
)
;

BOOL
EsetestEnsureFullPathExistsW(
    __in PCWSTR     wszPath
)
;

PSTR
EsetestGetFilePathA(
    __in PCSTR      szFile
)
;

PWSTR
EsetestGetFilePathW(
    __in PCWSTR     wszFile
)
;

LONGLONG
EsetestGetFileSizeA(
    __in PCSTR      szFile
)
;

LONGLONG
EsetestGetFileSizeW(
    __in PCWSTR     wszFile
)
;


DWORD
SystemTimeToSecondsSince1970(
    __in const PSYSTEMTIME  pSystemTime
)
;

VOID
SecondsSince1970ToSystemTime(
    __in DWORD          dwSecondsSince1970,
    __in PSYSTEMTIME        pSystemTime
)
;


DWORD GetAttributeListSizeA(
    __in PCSTR const szFilename,
    __out ULONG64* const pcbAttributeList
);

DWORD GetAttributeListSizeW(
    __in PCWSTR const wszFilename,
    __out ULONG64* const pcbAttributeList
);

#ifdef _UNICODE
#define GetAttributeListSize GetAttributeListSizeW
#else
#define GetAttributeListSize GetAttributeListSizeA
#endif

DWORD GetExtentCountA(
    __in PCSTR const szFilename,
    __out DWORD* const pcExtent
);

DWORD GetExtentCountW(
    __in PCWSTR const wszFilename,
    __out DWORD* const pcExtent
);

#ifdef _UNICODE
#define GetExtentCount GetExtentCountW
#else
#define GetExtentCount GetExtentCountA
#endif

DISK_GEOMETRY* GetDiskGeometry(
    __in unsigned long ulDisk
);

DISK_CACHE_INFORMATION* GetDiskCacheInfo(
    __in unsigned long ulDisk
);

BOOL SetDiskCacheInfo(
    __in const DISK_CACHE_INFORMATION* const pdc,
    __in unsigned long ulDisk
);

BOOL GetDiskReadCache(
    __in unsigned long ulDisk,
    __out BOOLEAN* pfOn
);

BOOL SetDiskReadCache(
    __in unsigned long ulDisk,
    __in BOOLEAN fOn
);

BOOL GetDiskWriteCache(
    __in unsigned long ulDisk,
    __out BOOLEAN* pfOn
);

BOOL SetDiskWriteCache(
    __in unsigned long ulDisk,
    __in BOOLEAN fOn
);

BOOL GetAdvancedDiskWriteCache(
    __in unsigned long ulDisk,
    __out BOOLEAN* pfOn
);

BOOL SetAdvancedDiskWriteCache(
    __in unsigned long ulDisk,
    __in BOOLEAN fOn
);

BOOL CreateVirtualDisk(
    __in PCWSTR const wszVhdFilePath,
    __in const ULONG cmbSize,
    __in const WCHAR* const wszMountPoint
);

BOOL DestroyVirtualDisk(
    __in PCWSTR const wszVhdFilePath
);

VOLUME_DISK_EXTENTS* GetVolumeExtents(
    __in WCHAR** pwszLogicalVolume
);

BOOL DefragmentVolumeA(
    __in PCSTR const szVolumeName
);

BOOL DefragmentVolumeW(
    __in PCWSTR const wszVolumeName
);

#ifdef _UNICODE
#define DefragmentVolume DefragmentVolumeW
#else
#define DefragmentVolume DefragmentVolumeA
#endif

DRIVE_LAYOUT_INFORMATION_EX* GetDiskPartitions(
    __in unsigned long ulDisk
);

BOOL OnlineDisk(
    __in unsigned long ulDisk
);

BOOL DeleteDiskPartition(
    __in unsigned long ulDisk,
    __in unsigned long ulPartition
);

BOOL DeleteDiskPartitions(
    __in unsigned long ulDisk
);

BOOL CreateFormatDiskPartition(
    __in unsigned long  ulDisk,
    __in ULONGLONG      cmbDiskSize,
    __in char           chLetter    
);

BOOL FormatVolume(
    __in char   chLetter
);



#define ESETEST_bitPCHelperCollectMin       0x00000001
#define ESETEST_bitPCHelperCollectMax       0x00000002
#define ESETEST_bitPCHelperCollectAvg       0x00000004
#define ESETEST_bitPCHelperCollectVar       0x00000008
#define ESETEST_bitPCHelperCollect( min, max, avg, var )        \
    (                                                           \
    ( (min) ? ESETEST_bitPCHelperCollectMin : 0x00000000 ) |    \
    ( (max) ? ESETEST_bitPCHelperCollectMax : 0x00000000 ) |    \
    ( (avg) ? ESETEST_bitPCHelperCollectAvg : 0x00000000 ) |    \
    ( (var) ? ESETEST_bitPCHelperCollectVar : 0x00000000 )      \
    )

BOOL
PerfCountersExist()
;

BOOL
PerfCountersInstallA(
    __in_opt PCSTR  szLibraryPath,
    __in_opt PCSTR  szIniFilePath
)
;

BOOL
PerfCountersInstallW(
    __in_opt PCWSTR wszLibraryPath,
    __in_opt PCWSTR wszIniFilePath
)
;

#ifdef _UNICODE
#define PerfCountersInstall PerfCountersInstallW
#else
#define PerfCountersInstall PerfCountersInstallA
#endif

BOOL
PerfCountersUninstall()
;

HANDLE
PerfCountersCreateQuery()
;

BOOL
PerfCountersAddLnCounterA(
    __in HANDLE     hQuery,
    __in_opt PCSTR  szComputerName,
    __in PCSTR      szPerfObject,
    __in PCSTR      szPerfCounter,
    __in PCSTR      szInstance
)
;

BOOL
PerfCountersAddLnCounterW(
    __in HANDLE     hQuery,
    __in_opt PCWSTR wszComputerName,
    __in PCWSTR     wszPerfObject,
    __in PCWSTR     wszPerfCounter,
    __in PCWSTR     wszInstance
)
;

#ifdef _UNICODE
#define PerfCountersAddLnCounter PerfCountersAddLnCounterW
#else
#define PerfCountersAddLnCounter PerfCountersAddLnCounterA
#endif

BOOL
PerfCountersAddCounterA(
    __in HANDLE     hQuery,
    __in_opt PCSTR  szComputerName,
    __in PCSTR      szPerfObject,
    __in PCSTR      szPerfCounter,
    __in PCSTR      szInstance
)
;

BOOL
PerfCountersAddCounterW(
    __in HANDLE     hQuery,
    __in_opt PCWSTR wszComputerName,
    __in PCWSTR     wszPerfObject,
    __in PCWSTR     wszPerfCounter,
    __in PCWSTR     wszInstance
)
;

#ifdef _UNICODE
#define PerfCountersAddCounter PerfCountersAddCounterW
#else
#define PerfCountersAddCounter PerfCountersAddCounterA
#endif

BOOL
PerfCountersGetCounterValues(
    __in HANDLE         hQuery,
    __out_opt double*   pData
)
;

BOOL
PerfCountersGetCounterValuesRaw(
    __in HANDLE         hQuery,
    __out_opt ULONG64*  pData1,
    __out_opt ULONG64*  pData2
)
;

BOOL
PerfCountersStartCollectingStatsA(
    __in HANDLE             hQuery,
    __in DWORD              dwPeriod,
    __in_opt JET_GRBIT*     pgrbitStats,
    __in_opt PCSTR          szLogFile
)
;

BOOL
PerfCountersStartCollectingStatsW(
    __in HANDLE             hQuery,
    __in DWORD              dwPeriod,
    __in_opt JET_GRBIT*     pgrbitStats,
    __in_opt PCWSTR         wszLogFile
)
;

#ifdef _UNICODE
#define PerfCountersStartCollectingStats PerfCountersStartCollectingStatsW
#else
#define PerfCountersStartCollectingStats PerfCountersStartCollectingStatsA
#endif

BOOL
PerfCountersRetrieveStats(
    __in HANDLE         hQuery,
    __out_opt double*   pMostRecent,
    __out_opt double*   pDataMin,
    __out_opt double*   pTimeMin,
    __out_opt double*   pDataMax,
    __out_opt double*   pTimeMax,
    __out_opt double*   pDataAvg,
    __out_opt double*   pDataVar,
    __out_opt ULONG64*  cCollected
)
;

BOOL
PerfCountersStopCollectingStats(
    __in HANDLE hQuery
)
;

BOOL
PerfCountersDestroyQuery(
    __in HANDLE hQuery
)
;

inline VOID
DoubleArraySet(
    __in_ecount( cSize ) double*    pArray,
    __in size_t                     cSize,
    __in double                     dValue
)
{
    while ( cSize-- ) pArray[ cSize ] = dValue;
}

inline VOID
DoubleArrayCopy(
    __in_ecount( cSize ) double*    pDst,
    __in_ecount( cSize ) double*    pSrc,
    __in size_t                     cSize
)
{
    while ( cSize-- ) pDst[ cSize ] = pSrc[ cSize ];
}

HANDLE
PerfReportingCreateFileA(
    __in PCSTR szFile
)
;

HANDLE
PerfReportingCreateFileW(
    __in PCWSTR wszFile
)
;

#ifdef _UNICODE
#define PerfReportingCreateFile PerfReportingCreateFileW
#else
#define PerfReportingCreateFile PerfReportingCreateFileA
#endif

BOOL
PerfReportingReportValueA(
    __in HANDLE     hPerfReporting,
    __in PCSTR      szCounterName,
    __in PCSTR      szCounterUnit,
    __in PCSTR      szPrintfFormat,
    __in BOOL       fHigherIsBetter,
    __in double     dblValue
)
;

BOOL
PerfReportingReportValueW(
    __in HANDLE     hPerfReporting,
    __in PCWSTR     wszCounterName,
    __in PCWSTR     wszCounterUnit,
    __in PCWSTR     wszPrintfFormat,
    __in BOOL       fHigherIsBetter,
    __in double     dblValue
)
;

#ifdef _UNICODE
#define PerfReportingReportValue PerfReportingReportValueW
#else
#define PerfReportingReportValue PerfReportingReportValueA
#endif

BOOL
PerfReportingCloseFile(
    __in HANDLE hPerfReporting
)
;


typedef struct _PerfCollectorCustomCounter PerfCollectorCustomCounter;

typedef double PFNPerfCollectorStartStopCustomCollection( HANDLE hCollector, void* pvContext, const PerfCollectorCustomCounter* const pprfcctr, const bool fStart );

typedef enum
{
    pcctypAvg = 0,
    pcctypFormatted,
    pcctypRaw
} PerfCollectorCounterType;

typedef struct _PerfCollectorCounter
{
    const char* szObject;
    const char* szInstance;
    const char* szName;
    const char* szUnit;
    const char* szFormat;
    bool fHigherIsBetter;
    PerfCollectorCounterType pcType;
    JET_GRBIT grbitStats;
} PerfCollectorCounter;

typedef struct _PerfCollectorCustomCounter
{
    const char* szName;
    const char* szUnit;
    const char* szFormat;
    bool fHigherIsBetter;
    PFNPerfCollectorStartStopCustomCollection* pfnStartStop;
} PerfCollectorCustomCounter;

HANDLE PerfCollectorCreate( const PerfCollectorCounter* const ctrAvg, const size_t cctrAvg,
                            const PerfCollectorCounter* const ctrFormatted, const size_t cctrFormatted,
                            const PerfCollectorCounter* const ctrRaw, const size_t cctrRaw,
                            const PerfCollectorCustomCounter* const ctrCustom, const size_t cctrCustom,
                            void* const pvContext );

void PerfCollectorDestroy( HANDLE hCollector );

void PerfCollectorStart( HANDLE hCollector, const DWORD cmsecSampling );

void PerfCollectorCancel( HANDLE hCollector );

void PerfCollectorStopAndReport( HANDLE hCollector, const char* const szLabel );


typedef VOID (__stdcall *PFNEVENTLOGGING)
(
    PWSTR           wszEventLog,
    PWSTR           wszEventSource,
    PSYSTEMTIME     pTimeGenerated,
    WORD            wEventType,
    WORD            wEventCategory,
    DWORD           dwEventId,
    PWSTR*          pwszStrings,
    WORD            cStrings,
    PVOID           pRawData,
    DWORD           cbRawData,
    PVOID           pUserData
);

HANDLE
EventLoggingCreateQuery(
    __in_opt PFNEVENTLOGGING                    pfnCallback,
    __in_opt PCWSTR                             wszEventLog,
    __in_ecount_opt( cEventSources ) PCWSTR*    pwszEventSources,
    __in size_t                                 cEventSources,
    __in_opt PSYSTEMTIME                        pTimeMin,
    __in_opt PSYSTEMTIME                        pTimeMax,
    __in_ecount_opt( cEventTypes ) PWORD        pEventTypes,
    __in size_t                                 cEventTypes,
    __in_ecount_opt( cEventCategories ) PWORD   pEventCategories,
    __in size_t                                 cEventCategories,
    __in_ecount_opt( cEventIds ) PDWORD         pEventIds,
    __in size_t                                 cEventIds,
    __in_opt PCWSTR                             wszLogFile,
    __in_opt PVOID                              pUserData
)
;

BOOL
EventLoggingDestroyQuery(
    __in HANDLE hQuery
)
;

PWSTR
EventLoggingFormatMessage(
    __in HMODULE        hModule,
    __in DWORD          dwEventId,
    __in LCID           dwLandIg,
    __in_opt PWSTR*     pwszStrings
)
;

HMODULE
EventLoggingModuleFromEventSource(
    __in PCWSTR     wszEventSource
)
;

VOID
EventLoggingPrintEvent(
    __in PCWSTR                         wszEventLog,
    __in PCWSTR                         wszEventSource,
    __in PSYSTEMTIME                    pTimeGenerated,
    __in WORD                           wEventType,
    __in WORD                           wEventCategory,
    __in DWORD                          dwEventId,
    __in DWORD                          dwLangId,
    __in_ecount_opt( cStrings ) PWSTR*  pwszStrings,
    __in WORD                           cStrings,
    __in_bcount_opt( cbRawData ) PVOID  pRawData,
    __in DWORD                          cbRawData
)
;




HANDLE
PearsonCorrelationCreate();

void
PearsonCorrelationReset(
    __in_opt HANDLE     h
);

double
PearsonCorrelationNewSample(
    __in_opt HANDLE     h,
    __in const double   x,
    __in const double   y
);

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
);

bool
PearsonCorrelationStopSampling(
    __in_opt HANDLE     h
);

double
PearsonCorrelationGetCoefficient(
    __in_opt HANDLE     h
);

void
PearsonCorrelationDestroy(
    __in_opt HANDLE     h
);




void __stdcall EsetestAssertFail( const char * const szMessageFormat, char const *szFilename, long lLine, ... );

#ifdef DEBUG
#define AssertSz( exp, sz )         ( ( exp ) ? (void) 0 : EsetestAssertFail( sz, __FILE__, __LINE__ ) )
#else
#define AssertSz( exp, sz )
#endif

#define VerifySz( exp, sz )         ( ( exp ) ? (void) 0 : EsetestAssertFail( sz, __FILE__, __LINE__ ) )

#undef UNICODE
#undef _UNICODE

#define AssertM( exp )          AssertSz( exp, #exp )

#define VerifyP( exp )              VerifySz( exp, #exp )

#define PremiseSz( exp, sz )        ( ( exp ) ? ( void )0 : EsetestAssertFail( sz, __FILE__, __LINE__ ) )
#define Premise( exp )          PremiseSz( exp, #exp )
#define Assume( exp )           Premise( exp )

size_t strxchg(
    __in CHAR *string,
    __in const CHAR oldChar,
    __in const CHAR newChar
);

size_t wcsxchg(
    __in WCHAR *string,
    __in const WCHAR oldChar,
    __in const WCHAR newChar
);

size_t _tcsxchg(
    __in TCHAR *string,
    __in const TCHAR oldChar,
    __in const TCHAR newChar
);

inline
char*
EsetestStrDup(
    __in const char*    sz
)
{
    const size_t    cchLen  = strlen( sz ) + 1;
    char*   szRet   = new char[ cchLen ];
    if ( !szRet )
    {
        goto Cleanup;
    }
    memcpy( szRet, sz, cchLen * sizeof( char ) );
Cleanup:
    return szRet;
}

enum ESETEST_OUTPUT_LEVEL { OUTPUT_NONE = 0, OUTPUT_ERROR, OUTPUT_WARNING, OUTPUT_INFO, OUTPUT_DEBUG, OUTPUT_ALL };
enum ESETEST_OUTPUT_COLOR { COLOR_INFO = ANSI_GREEN, COLOR_WARNING = ANSI_YELLOW, COLOR_ERROR = ANSI_RED, COLOR_DEBUG = ANSI_SILVER };

ESETEST_OUTPUT_LEVEL
EsetestGetOutputLevel()
;

void
EsetestSetOutputLevel(
    const ESETEST_OUTPUT_LEVEL  level
)
;

int OutputDebug(
    _Printf_format_string_ const char *szFormat,
    ...
    );
int OutputInfo(
    _Printf_format_string_ const char *szFormat,
    ...
    );
int OutputWarning(
    _Printf_format_string_ const char *szFormat,
    ...
    );
int OutputError(
    _Printf_format_string_ const char *szFormat,
    ...
    );

BOOL CloseHandleP( HANDLE* pH );

char*
EsetestCopyString(
    __in PSTR           szFunction,
    __in PCSTR      szToCopy
)
;

wchar_t*
EsetestCopyWideString(
    __in PSTR           szFunction,
    __in PCWSTR     wszToCopy
)
;


wchar_t*
EsetestWidenString(
    __in PSTR   szFunction,
    __in PCSTR  sz
)
;

char*
EsetestUnwidenStringAlloc(
    __in PSTR   szFunction,
    __in PCWSTR wsz
)
;

JET_ERR
EsetestUnwidenString(
    __in PSTR   szFunction,
    __in PWSTR  wsz,
    __inout PSTR    sz
)
;

JET_ERR
EsetestCleanupWidenString(
    __in PSTR   szFunction,
    __inout PWSTR   wsz,
    __in PCSTR  sz
)
;

#pragma warning( pop )

#endif
