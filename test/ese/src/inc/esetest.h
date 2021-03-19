// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#pragma warning(disable:4985) // Warning generated during ORCAs upgrade
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4127) // conditional expression is constant
#pragma warning(disable:4512) // assignment operator could not be generated
#pragma warning(disable:4706) // assignment within conditional expression

#ifdef DEBUG
#else // DEBUG
#pragma warning ( disable : 4189 )  //  local variable is initialized but not referenced
#endif // !DEBUG

#define Unused( var ) ( var )

// this macro is needed at least in jetglue.hxx
#ifndef _ESETEST_4357768_H
#define _ESETEST_4357768_H

// ESETest.h
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
// [2]  25/10/2000  SOMEONE      Added tcprintf function
// [3]  02/07/2002  SOMEONE     Added the rest of the ANSI_* names

// disable 'nonstandard extension used: nameless struct/union'
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

// If including tchar.h breaks anything, just do something like this. (Since it's only used for _T in this file)
//#ifndef _INC_TCHAR
//#define _T( foo ) L ## foo
//#endif

#include <tchar.h>

// SOMEONE, Oct2002. I would like to change all the tests to not refer to jet.h. Then in this header
// we will #include either <esent.h> or <jet.h> as apropriate.

#pragma warning( push )
#pragma warning( disable : 4201 )       // nonstandard extension used : nameless struct/union
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
    _In_ JET_PCSTR szDatabase,
    _In_ JET_PCSTR szLogfile,
    _In_ JET_GRBIT grbit );

#if ( JET_VERSION >= 0x0600 )

JET_ERR JET_API
JetRemoveLogfileW(
    _In_ wchar_t* szDatabase,
    _In_ wchar_t* szLogfile,
    _In_ JET_GRBIT grbit );

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

/////////////////////////////////////////////////////////////////////
// Macros
//*******************************************************************
// All of the int functions in this library will return one of the
// following error values
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

// look in wincon.h
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

//*******************************************************************
// When calling JET and you want to fail the test case if any JET
// error is returned, then you can use this macro and it will
// automatically log the error in results.log and goto the LError
// handler where you can mark the test case as failed.
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

// extern const char*       g_szEseutil;
// extern const WCHAR*  g_wszEseutil;

#if ( !defined( ESE_FLAVOUR_IS_ESENT ) && !defined( ESE_FLAVOUR_IS_ESE ) )
#  error You should define either ESE_FLAVOUR_IS_ESE or ESE_FLAVOUR_IS_ESENT
// Source Insight hack: If we don't #define it somewhere, it won't be a nice pretty colour :)
#  define ESE_FLAVOUR_IS_ESE
#  define ESE_FLAVOUR_IS_ESENT
#endif

#if ( defined( ESE_FLAVOUR_IS_ESENT ) && defined( ESE_FLAVOUR_IS_ESE ) )
#  error You should not have both ESE_FLAVOUR_IS_ESE and ESE_FLAVOUR_IS_ESENT defined!
#endif

#ifdef ESE_FLAVOUR_IS_ESE
#  define ESE_FLAVOUR       "ese"
#  define WESE_FLAVOUR      L"ese"
// Macros are easier for concatenating
#  define   SZESEUTIL   "eseutil.exe"
#  define   WSZESEUTIL  L"eseutil.exe"
#endif // ESE_FLAVOUR_IS_ESE

#ifdef ESE_FLAVOUR_IS_ESENT
#  define ESE_FLAVOUR       "esent"
#  define WESE_FLAVOUR      L"esent"
// Macros are easier for concatenating
#  define   SZESEUTIL   "esentutl.exe"
#  define   WSZESEUTIL  L"esentutl.exe"
#endif // ESE_FLAVOUR_IS_ESENT

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
    _In_ size_t                 cch
);

const wchar_t*
WszEsetestGetChkptName(
    __out_ecount( cch ) wchar_t*    wszBuffer,
    _In_ size_t                     cch
);

char*
SzEsetestGetLogNameTemp(
    __out_ecount( cch ) char*   szBuffer,
    _In_ size_t                 cch
);

wchar_t*
WszEsetestGetLogNameTemp(
    __out_ecount( cch ) wchar_t*    wszBuffer,
    _In_ size_t                     cch
);

char*
SzEsetestGetLogNameLast(
    __out_ecount( cch ) char*   szBuffer,
    _In_ size_t                 cch
);

wchar_t*
WszEsetestGetLogNameLast(
    __out_ecount( cch ) wchar_t*    wszBuffer,
    _In_ size_t                     cch
);

char*
SzEsetestGetLogNameFromGen(
    __out_ecount( cch ) char*   szBuffer,
    _In_ size_t                 cch,
    _In_ unsigned long          ulGen
);

wchar_t*
WszEsetestGetLogNameFromGen(
    __out_ecount( cch ) wchar_t*    wszBuffer,
    _In_ size_t                     cch,
    _In_ unsigned long              ulGen
);

long
EsetestGetLogGenFromNameA(
    _In_ const char* szLogName
);

long
EsetestGetLogGenFromNameW(
    _In_ const wchar_t* wszLogName
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

// Skip calling System Parameters in InitTest()?
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

// EsetestSetRunningStress affects some things such as retry-logic for memory
// allocations.
void
EsetestSetRunningStress(
    _In_ const bool fRunningStress
)
;

/*
extern BOOL g_fVerbose;
extern BOOL g_fVerbose2;

// An attempt to get rid of ugly #ifdef's
extern BOOL g_fScrubbingSupported;
extern BOOL g_fSlvSupported;
extern BOOL g_fMultiInstanceSupported;
extern BOOL g_fPrepInsertCopyDeleteOriginalSupported;
extern BOOL g_fParamCacheSizeMaxSupported;
extern BOOL g_fZeroDb;
extern BOOL g_fBackupSupported;
extern BOOL g_fMultiInstanceBackupSupported;
*/

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

// Constantly adding APIs is too cumbersome
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
    EseFeatureLargePageSize,    // indicates 16k, 32k pagesize support
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
    _In_ const EseFeaturePresent    feature
)
;

size_t
EsetestColtypMax()
;

size_t
EsetestCColKeyMost()
;

//*******************************************************************
// Versioning Functions
enum
EsetestEseFlavour {
    EsetestEseFlavourNil        = 0,
    EsetestEseFlavourEsent      = 1,        // esent.dll
    EsetestEseFlavourEse        = 2,        // ese.dll
    EsetestEseFlavourEseMax,
}
;

//#pragma push_macro( ESEVERSION )
//#undef ESEVERSION
//#pragma push_macro( esent )
//#define esent EsetestEseFlavourEsent
//#pragma push_macro( EsetestEseFlavourEse )
//#define ese 2

/*
__forceinline DWORD
EsetestEseVersionFromParts(
    EsetestEseFlavour exornt,
    UINT major,
    UINT minor,
    UINT spnumber,
    UINT buildno

)
{
    return ( ( exornt << 28 ) | ( major << 24 ) | ( minor << 20 ) | ( spnumber << 16 ) | ( buildno ) );
}
*/
//#define EsetestEseVersionFromParts( exornt, major, minor, spnumber, buildno ) \
//  ( ( exornt << 28 ) | ( major << 24 ) | ( minor << 20 ) | ( spnumber << 16 ) | ( buildno ) )

/*
enum
EsetestEseVersion {
    EseVersionNil               = 0,
    // ESENT ----------

    // Win 2000
    EseVersionEsent2000         = EsetestEseVersionFromParts( EsetestEseFlavourEsent, 5, 0, 0, 0 ),

    // Whistler, XP
    EseVersionEsentXpRtm        = EsetestEseVersionFromParts( EsetestEseFlavourEsent, 5, 1, 0, 0 ),
    EseVersionEsentXpSp1        = EsetestEseVersionFromParts( EsetestEseFlavourEsent, 5, 1, 1, 0 ),
    EseVersionEsentXpSp2        = EsetestEseVersionFromParts( EsetestEseFlavourEsent, 5, 1, 2, 0 ),

    // Whistler, Server 2003
    EseVersionEsent2003Rtm      = EsetestEseVersionFromParts( EsetestEseFlavourEsent, 5, 2, 0, 0 ),
    EseVersionEsent2003Sp1      = EsetestEseVersionFromParts( EsetestEseFlavourEsent, 5, 2, 1, 0 ),
    EseVersionEsent2003Sp2      = EsetestEseVersionFromParts( EsetestEseFlavourEsent, 5, 2, 2, 0 ),

    // Longhorn
    EseVersionEsentLonghornRtm  = EsetestEseVersionFromParts( EsetestEseFlavourEsent, 6, 0, 0, 0 ),

    // ESE -----------

    // Platinum, Exchange 2000
    EseVersionEsePtRtm          = EsetestEseVersionFromParts( EsetestEseFlavourEse, 6, 0, 0, 0 ),

    // Titanium, Exchange 2003
    EseVersionEseTiRtm          = EsetestEseVersionFromParts( EsetestEseFlavourEse, 6, 5, 0, 0 ),
    EseVersionEseTiSp1          = EsetestEseVersionFromParts( EsetestEseFlavourEse, 6, 5, 1, 0 ),
    EseVersionEseTiSp2          = EsetestEseVersionFromParts( EsetestEseFlavourEse, 6, 5, 2, 0 ),

    // Exchange 12, Exchange ???
    EseVersionEse12Rtm          = EsetestEseVersionFromParts( EsetestEseFlavourEse, 8, 0, 0, 0 ),

};
*/
// #pragma pop_macro( helper )
//#pragma pop_macro( esent )
//#pragma pop_macro( ese )


// EsetestEseVersion is the 64-bit value from FileVersionMS and FileVersionLS,
// with the high-nybble a EsetestEseFlavour
typedef ULONG64 EsetestEseVersion;
/*
EsetestEseVersion
rgKnownVersions [] = {
    // Win 2000
    EseVersionEsent2000,

    // Whistler, XP
    EseVersionEsentXpRtm,
    EseVersionEsentXpSp1,
    EseVersionEsentXpSp2,

    // Whistler, Server 2003
    EseVersionEsent2003Rtm,
    EseVersionEsent2003Sp1,
    EseVersionEsent2003Sp2,

    // Longhorn
    EseVersionEsentLonghornRtm,

    // ESE -----------

    // Platinum, Exchange 2000
    EseVersionEsePtRtm,

    // Titanium, Exchange 2003
    EseVersionEseTiRtm,
    EseVersionEseTiSp1,
    EseVersionEseTiSp2,

    // Exchange 12, Exchange ???
    EseVersionEse12Rtm,
}
;
*/
JET_ERR
EsetestEseVersionToParts(
    _In_ EsetestEseVersion      version,
    _Out_ EsetestEseFlavour*    pexornt,
    _Out_ PUINT                 pmajor,
    _Out_ PUINT                 pminor,
    _Out_ PUINT                 pspnumber,
    _Out_ PUINT                 pbuildno
)
;

JET_ERR
EsetestEseFileVersionToParts(
    _In_ EsetestEseVersion      version,
    _Out_ EsetestEseFlavour*    pexornt,
    _Out_ PUINT                 pmajor,
    _Out_ PUINT                 pminor,
    _Out_ PUINT                 pbuildno,
    _Out_ PUINT                 pprivateno
)
;

EsetestEseFlavour
EsetestGetEseFlavour()
;

JET_ERR
EsetestGetEseVersion(
    _Out_ EsetestEseFlavour*    pflavour,
    _Out_ EsetestEseVersion*    peseversion,
    _Out_ bool*                 pfChecked
)
;

JET_ERR
EsetestGetEseVersionParts(
    _Out_   EsetestEseFlavour*  pflavour,
    _Out_   PUINT               pverMajor,
    _Out_   PUINT               pverMinor,
    _Out_   PUINT               pverSp,
    _Out_   PUINT               pverBuild,
    _Out_   bool*               pfChecked
)
;

JET_ERR
EsetestGetEseFileVersionParts(
    _Out_   EsetestEseFlavour*  pflavour,
    _Out_   PUINT               pverMajor,
    _Out_   PUINT               pverMinor,
    _Out_   PUINT               pverBuild,
    _Out_   PUINT               pverPrivate,
    _Out_   bool*               pfChecked
)
;

bool
FEsetestIsBugFixed(
    _In_    UINT                bugnumberEse,
    _In_    UINT                bugnumberEsent
)
;

bool
FEsetestVerifyVersion(
    _In_    EsetestEseFlavour   flavour,
    _In_    UINT                verMajor,
    _In_    UINT                verMinor,
    _In_    UINT                verSp,
    _In_    UINT                verBuild        // can be zero
)
// Returns whether the current version of ESE meets the minimum required.
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


// Counter resolution functions.
ULONG64
QWEsetestQueryPerformanceFrequency()
;

ULONG64
QWEsetestQueryPerformanceCounter()
;

// Defines from ESEBack2.H
#ifndef ESE_REGISTER_BACKUP
#define ESE_REGISTER_BACKUP             0x00000001
#endif

// esent.w says that it is not used 
typedef struct              /* Status Notification Message */
    {
    unsigned long   cbStruct;   /* Size of this structure */
    JET_SNC         snc;        /* Status Notification Code */
    unsigned long   ul;         /* Numeric identifier */
    char            sz[256];    /* Identifier */
    } JET_SNMSG_A;

typedef struct              /* Status Notification Message */
    {
    unsigned long   cbStruct;   /* Size of this structure */
    JET_SNC         snc;        /* Status Notification Code */
    unsigned long   ul;         /* Numeric identifier */
    WCHAR           sz[256];    /* Identifier */
    } JET_SNMSG_W;


// backup from replica
#ifdef  __cplusplus
extern "C" {
#endif
JET_ERR JET_API JetBeginSurrogateBackup(
    _In_    JET_INSTANCE    instance,
    _In_        unsigned long       lgenFirst,
    _In_        unsigned long       lgenLast,
    _In_        JET_GRBIT       grbit );
JET_ERR JET_API JetEndSurrogateBackup(
    _In_    JET_INSTANCE    instance,
    _In_        JET_GRBIT       grbit );

#ifdef  __cplusplus
}
#endif

// should be in esent_x.h and will be when it is merged from ex12 to LH
#define JET_errSurrogateBackupInProgress    -617    //  A surrogate backup is in progress. 
//=====================================

// Format GetLastError in a string
void FormatLastError(
    _Out_writes_z_( iLength ) char* szTemp,
    _In_ const int iLength
);

// Format specific error to a string
void FormatSpecificError(
    __in_ecount( iLength ) char* szTemp,
    _In_ const int iLength,
    _In_ const DWORD dwError
);

#ifndef ESETEST_NO_OPERATOR_NEW

/////////////////////////////////////////////////////////////////////
// Memory-related functions
/////////////////////////////////////////////////////////////////////

void*
PvMemoryHeapAlloc(
    _In_ const size_t   cbSize
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


#endif // ESETEST_NO_OPERATOR_NEW


/////////////////////////////////////////////////////////////////////
// Function Prototypes
//*******************************************************************
// The following functions are the most useful for ESE test case generation.
// InitLogging()/TermLogging -- for (older) tests that just want to use tprintf, but
//          don't want to do the whole PASSED/FAILED thing
// SuspendLogging()/ActivateLogging() -- for older tests (mujet) that have the notion
//          of LogOff() and LogOn().
// InitTest() set up the test case by initializing results.txt and results.log
// tprintf() is a replacement for printf() that will log to both the console and
//          results.log
// tcprintf() -- tprintf(), with colour!
// NTError() allows the user to log unexpected NT errors to the console and
//          results.log
// TermTest() will log the final result of the test case into results.txt.  It
//          determines whether the test case passed or failed by the value of
//          the nSuccess parameter.
// ChangeResult() will erase FAILED and substitute whatever status is desired.



void
EsetestSetResultsTxt(
    _In_ const char*    szResultsTxt
)
;

void
EsetestSetResultsLog(
    _In_ const char*    szResultsLog
)
;

void InitLogging( BOOL fLogToDisk = TRUE );
//BOOL SuspendLogging();
//BOOL ActivateLogging();
//BOOL GetLoggingStatus();
void TermLogging( void );

// Please use these APIs sparingly: Abuse of them can make things ugly.
BOOL EsetestSetLoggingToDisk( BOOL fLogToDisk );
BOOL EsetestGetLoggingToDisk();
// I am not providing a corresponding 'enable' becasue I think this functio should be used exceedingly sparingly
BOOL EsetestDisableWritingToResultsTxt();

int
InitTest(
    __in_opt const char *szCommand = NULL,
    _In_ BOOL fLogToDisk = TRUE,
    _In_ BOOL fWriteFailed = TRUE
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

// Logs to console and results.log, prepending the thread id.
int tprintf(
    _In_ _Printf_format_string_ const char* szFormat,
    ...
);


// Logs to console and results.log.
int tprintfnothid(
    _In_ _Printf_format_string_ const char* szFormat,
    ...
);

/*
// Currently internal, but could easily be made external.
int tprintfSpecifyTargets(
    __in_opt const char*            szLogFile,          // if NULL, will log to results.log
    _In_ const JET_GRBIT level,
    _In_ _Printf_format_string_ const char* szFormat,
    ...
)
;
*/

int
tprintfPerfLog(
    _In_ _Printf_format_string_ const char* szFormat,
    ...
)
;

// Logs to console and results.txt (not results.log).
int tprintfResultsTxtUseWithCaution(
    _In_ _Printf_format_string_ const char* szFormat,
    ...
);

// Logs to console, specifying the foreground colour, and to results.log.
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
// Obsolete
//#define   Esetest_bitLogToResultsTxtInsteadOfResultsLogUseWithCaution 0x40000000  // Allow select program to write to results.txt instead.

// The function that actually does the printing.
//int EsetestVprintf( EsetestLogLevel level, const char* szFormat, va_list ap );
int EsetestVprintf(
    __in_opt const char*                szLogFile,          // if NULL, will log to results.log
    _In_ const JET_GRBIT            level,
    _In_ _Printf_format_string_ const char*             szFormat,
    _In_ va_list                    ap
    )
;

//*******************************************************************
// Some JET_SETCOLUMN and JET_RETRIEVECOLUMN helper functions.
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


//*******************************************************************
// Useful functions to be used when parsing result strings or files.
//
// ParseResultsMemory searches for szToken in szSrc. If szToken is
// found, searches for szTokenBefore and szTokenAfter. If found,
// returns the segment between szTokenBefore and szTokenAfter.
// A value of "" for szToken means the beginning of szSrc.
// A value of "" for szTokenBefore means the first char following
// szToken.
// A value of "" for szTokenAfter means the end of szSrc.
// ParseResultsFile works the same way, but takes szSrc from
// szFileName.
// Both ParseResultsMemory and ParseResultsFile return an internally
// allocated string. Therefore, FreeParsedStr must be used to
// free this memory.
// Example:
// Assume that example.txt contains:
//      cbDbPage: 8192
//      dbtime: 43769 (0xaaf9)
//      State: Clean Shutdown
// Calling ParseResultsFile (   _T( "example.txt" ),
//                              _T( "dbtime" ),
//                              _T( "(" ),
//                              _T( ")" ) )
// will return "0xaaf9".

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

//*******************************************************************
// Random number generator (not C's random number generator but the one
// from Typhoon)
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

//*******************************************************************
// Copies one file to another
// 2002.04.18 Changed the name from the generic CopyFile()
// 2002.06.11 Only used in the .cpp, so I'll move it there and make it PRIVATE
// int AppendToFileHandleFromFileHandle(HANDLE hfileSrc, HANDLE hfileDest);
//*******************************************************************

// Registry paths
const TCHAR g_tszRegPathDebug[]             = _T( "Debug" );
const TCHAR g_tszRegPathOs[]                = _T( "OS" );

// Registry Keys
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

//*******************************************************************
// Registry functions:

LONG EsetestRegGetValueA(
    _In_         HKEY hkey,
    __in_opt     LPCTSTR pszSubKey,
    __in_opt     LPCTSTR pszValue,
    __in_opt     DWORD dwFlags,
    __out_opt    LPDWORD pdwType,
    __out_opt    PVOID pvData,
    __inout_opt  LPDWORD pcbData
    )
;

// Generic registry wrappers
JET_ERR
ErrEsetestWriteRegistry(
    _In_ const HKEY         hkeyHive,
    _In_ const TCHAR*       szKeyPath,
    _In_ const TCHAR*       szKeyName,
    _In_ const TCHAR*       szValue
    )
;

JET_ERR
ErrEsetestReadRegistry(
    _In_ const HKEY     hkeyHive,
    _In_ const TCHAR*   szKeyPath,
    _In_ const TCHAR*   szKeyName,
    __out_bcount_part( *pcbValue, *pcbValue ) TCHAR* const  szValue,        // Must point to valid buffer
    _Inout_  PDWORD     pcbValue        // How big the buffer is going in; how many bytes written
    )
;

// These default to write into hklm\software\microsoft\ese\global or hklm\software\microsoft\esent\global, as appropriate.
JET_ERR
ErrEsetestWriteConfigToEsentGlobal(
    _In_ const TCHAR* szKeyPath,
    _In_ const TCHAR* szKeyName,
    _In_ const TCHAR* szValue
    )
;

JET_ERR
ErrEsetestReadConfigToEsentGlobal(
    _In_ const TCHAR*   szKeyPath,
    _In_ const TCHAR*   szKeyName,
    __out_bcount_part( *pcbValue, *pcbValue ) TCHAR* const  szValue,        // Must point to valid buffer
    __inout PDWORD      pcbValue        // How big the buffer is going in; how many bytes written
    )
;

JET_ERR
ErrEsetestCopyKey(
    _In_ const WCHAR*   wszKeySrc,
    _In_ const WCHAR*   wszKeyDst,
    _In_ BOOL           fDelSrc,
    _In_ BOOL           fDelDst
    )
;


//*******************************************************************
// This is stolen from DBUTLSprintHex().
VOID SprintHex(
    _Out_ CHAR * const      szDest,
    __in_bcount( cbSrc ) const BYTE * const rgbSrc,
    _In_ const INT          cbSrc,
    _In_ const INT          cbWidth     = 16,
    _In_ const INT          cbChunk     = 4,
    _In_ const INT          cbAddress   = 8,
    _In_ const INT          cbStart     = 0
)
;

//================================================================
// Synchronization helper functions.
//================================================================
//
// =============================================================================
void
EsetestWaitForAllObjectsInfinitely(
    _In_ DWORD cCount,
    __in_ecount( cCount ) const HANDLE* rgHandles
)
;

//================================================================
// Directory manipulation helper functions.
//================================================================
//
BOOL
EsetestCreateDirectoryA(
    _In_ PCSTR      szName
)
;

BOOL
EsetestCreateDirectoryW(
    _In_ PCWSTR     wszName
)
;

BOOL
EsetestRemoveDirectoryA(
    _In_ PCSTR      szName
)
;

BOOL
EsetestRemoveDirectoryW(
    _In_ PCWSTR     wszName
)
;

BOOL
EsetestCopyDirectoryA(
    _In_ PCSTR      szNameSrc,
    _In_ PCSTR      szNamedst
)
;

BOOL
EsetestCopyDirectoryW(
    _In_ PCWSTR     wszNameSrc,
    _In_ PCWSTR     wszNameDst
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
    _In_ PCSTR      szPath
)
;

BOOL
EsetestEnsureFullPathExistsW(
    _In_ PCWSTR     wszPath
)
;

PSTR
EsetestGetFilePathA(
    _In_ PCSTR      szFile
)
;

PWSTR
EsetestGetFilePathW(
    _In_ PCWSTR     wszFile
)
;

LONGLONG
EsetestGetFileSizeA(
    _In_ PCSTR      szFile
)
;

LONGLONG
EsetestGetFileSizeW(
    _In_ PCWSTR     wszFile
)
;

//================================================================
// Time helper functions.
//================================================================
//

// Description: converts a SystemTime structure to the number of seconds since 00:00:00 01/01/1970.
// pSystemTime: self-descriptive.
// Return value: self-descriptive.
DWORD
SystemTimeToSecondsSince1970(
    _In_ const PSYSTEMTIME  pSystemTime
)
;

// Description: converts the number of seconds since 00:00:00 01/01/1970 to a SystemTime structure .
// dwSecondsSince1970: self-descriptive.
// pSystemTime: self-descriptive.
VOID
SecondsSince1970ToSystemTime(
    _In_ DWORD          dwSecondsSince1970,
    _In_ PSYSTEMTIME        pSystemTime
)
;

//================================================================
// Disk helper functions.
//================================================================
//

DWORD GetAttributeListSizeA(
    _In_ PCSTR const szFilename,
    _Out_ ULONG64* const pcbAttributeList
);

DWORD GetAttributeListSizeW(
    _In_ PCWSTR const wszFilename,
    _Out_ ULONG64* const pcbAttributeList
);

#ifdef _UNICODE
#define GetAttributeListSize GetAttributeListSizeW
#else
#define GetAttributeListSize GetAttributeListSizeA
#endif

DWORD GetExtentCountA(
    _In_ PCSTR const szFilename,
    _Out_ DWORD* const pcExtent
);

DWORD GetExtentCountW(
    _In_ PCWSTR const wszFilename,
    _Out_ DWORD* const pcExtent
);

#ifdef _UNICODE
#define GetExtentCount GetExtentCountW
#else
#define GetExtentCount GetExtentCountA
#endif

DISK_GEOMETRY* GetDiskGeometry(
    _In_ unsigned long ulDisk
);

DISK_CACHE_INFORMATION* GetDiskCacheInfo(
    _In_ unsigned long ulDisk
);

BOOL SetDiskCacheInfo(
    _In_ const DISK_CACHE_INFORMATION* const pdc,
    _In_ unsigned long ulDisk
);

BOOL GetDiskReadCache(
    _In_ unsigned long ulDisk,
    _Out_ BOOLEAN* pfOn
);

BOOL SetDiskReadCache(
    _In_ unsigned long ulDisk,
    _In_ BOOLEAN fOn
);

BOOL GetDiskWriteCache(
    _In_ unsigned long ulDisk,
    _Out_ BOOLEAN* pfOn
);

BOOL SetDiskWriteCache(
    _In_ unsigned long ulDisk,
    _In_ BOOLEAN fOn
);

BOOL GetAdvancedDiskWriteCache(
    _In_ unsigned long ulDisk,
    _Out_ BOOLEAN* pfOn
);

BOOL SetAdvancedDiskWriteCache(
    _In_ unsigned long ulDisk,
    _In_ BOOLEAN fOn
);

BOOL CreateVirtualDisk(
    _In_ PCWSTR const wszVhdFilePath,
    _In_ const ULONG cmbSize,
    _In_ const WCHAR* const wszMountPoint
);

BOOL DestroyVirtualDisk(
    _In_ PCWSTR const wszVhdFilePath
);

VOLUME_DISK_EXTENTS* GetVolumeExtents(
    _In_ WCHAR** pwszLogicalVolume
);

BOOL DefragmentVolumeA(
    _In_ PCSTR const szVolumeName
);

BOOL DefragmentVolumeW(
    _In_ PCWSTR const wszVolumeName
);

#ifdef _UNICODE
#define DefragmentVolume DefragmentVolumeW
#else
#define DefragmentVolume DefragmentVolumeA
#endif

DRIVE_LAYOUT_INFORMATION_EX* GetDiskPartitions(
    _In_ unsigned long ulDisk
);

BOOL OnlineDisk(
    _In_ unsigned long ulDisk
);

BOOL DeleteDiskPartition(
    _In_ unsigned long ulDisk,
    _In_ unsigned long ulPartition
);

BOOL DeleteDiskPartitions(
    _In_ unsigned long ulDisk
);

BOOL CreateFormatDiskPartition(
    _In_ unsigned long  ulDisk,
    _In_ ULONGLONG      cmbDiskSize,
    _In_ char           chLetter    
);

BOOL FormatVolume(
    _In_ char   chLetter
);


//================================================================
// Performance counters helper functions.
//================================================================
// Similarly to the C++ Standard Library collection classes, we'll make this helper library thread safe for
// reading only.
// A single object (i.e. a query handle) is thread safe for reading from multiple threads. For example,
// given a query A, it is safe to read A from thread 1 and from thread 2 simultaneously.
// If a single query is being written to (i.e. counters are being added or the query is being destroyed) by one
// thread, then all reads and writes to that query on the same or other threads must be protected. For example,
// given a query A, if thread 1 is writing to A, then thread 2 must be prevented from reading from or writing to A.
// It is safe to read and write to one query even if another thread is reading or writing to a different query For
// example, given queries A and B, it is safe if A is being written in thread 1 and B is being read in thread 2.
//

// Bits to be used when starting collection of statistical data.
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

// Description: Determines if the ESE perf counters are installed.
// Return value: TRUE if it succeeds, FALSE, otherwise.
BOOL
PerfCountersExist()
;

// Description: Configures the proper registry keys to have the ESE performance counters installed.
// szLibraryPath: Directory hat contains <szEseperfDll>. NULL or "" defaults to the current dir.
// wszIniFilePath: Directory hat contains <szEseperfIni>. NULL or "" defaults to the same dir
//  pointed by szLibraryPath.
// Return value: TRUE if it succeeds, FALSE, otherwise.
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

// Description: Unistall the ESE performance counters..
// Return value: TRUE if it succeeds, FALSE, otherwise. If the counters are not registered, TRUE will
//  also be rturned.
BOOL
PerfCountersUninstall()
;

// Description: Allocates a new query object.
// Return value: a handle to be used in subsequent calls that requires a query handle. NULL if it fails.
HANDLE
PerfCountersCreateQuery()
;

// Description: adds a specific perf counter from a specific instance to the query object.
// hQuery: query handle returned by a previous call to PerfCountersCreateQuery().
// szComputerName: computer from which the performance counter is to be retrieved. NULL defaults to the local machine.
// szPerfObject: performance counter object to be added. Since PdhAddEnglishCounter() will be used internally,
//  this should be in English, regardless of the language of the binaries under test.
// szPerfCounter: name of the performance counter to be added. Since PdhAddEnglishCounter() will be
//  used internally, this should be in English, regardless of the language of the binaries under test.
// szInstance: name of the instance from which we want to obtain the performance counter value.
// Return value: TRUE if it succeeds, FALSE, otherwise.
BOOL
PerfCountersAddLnCounterA(
    _In_ HANDLE     hQuery,
    __in_opt PCSTR  szComputerName,
    _In_ PCSTR      szPerfObject,
    _In_ PCSTR      szPerfCounter,
    _In_ PCSTR      szInstance
)
;

BOOL
PerfCountersAddLnCounterW(
    _In_ HANDLE     hQuery,
    __in_opt PCWSTR wszComputerName,
    _In_ PCWSTR     wszPerfObject,
    _In_ PCWSTR     wszPerfCounter,
    _In_ PCWSTR     wszInstance
)
;

#ifdef _UNICODE
#define PerfCountersAddLnCounter PerfCountersAddLnCounterW
#else
#define PerfCountersAddLnCounter PerfCountersAddLnCounterA
#endif

// Description: exactly the same as above, but adds the counter using PdhAddCounter() instead. This makes it
//  non-robust in terms of localization, but allows us to test for perf counter localization.
BOOL
PerfCountersAddCounterA(
    _In_ HANDLE     hQuery,
    __in_opt PCSTR  szComputerName,
    _In_ PCSTR      szPerfObject,
    _In_ PCSTR      szPerfCounter,
    _In_ PCSTR      szInstance
)
;

BOOL
PerfCountersAddCounterW(
    _In_ HANDLE     hQuery,
    __in_opt PCWSTR wszComputerName,
    _In_ PCWSTR     wszPerfObject,
    _In_ PCWSTR     wszPerfCounter,
    _In_ PCWSTR     wszInstance
)
;

#ifdef _UNICODE
#define PerfCountersAddCounter PerfCountersAddCounterW
#else
#define PerfCountersAddCounter PerfCountersAddCounterA
#endif

// Description: retrives the counter values.
// hQuery: query handle returned by a previous call to PerfCountersCreateQuery().
// pData: pointer to a user-allocated buffer, which must be capable to hold n * sizeof( double ) bytes, where
//  n is the number of counters added to the query. The values will be stored in the order the counters were
//  added with PerfCountersAddCounter().
// Return value: TRUE if it succeeds, FALSE, otherwise. FALSE will be returned if at least one of the counters
//  can't be retrived. That particular element won't be changed in pData.
BOOL
PerfCountersGetCounterValues(
    _In_ HANDLE         hQuery,
    __out_opt double*   pData
)
;

// Description: retrives the raw counter values.
// hQuery: query handle returned by a previous call to PerfCountersCreateQuery().
// pData: pointer to a user-allocated buffer, which must be capable to hold n * sizeof( ULONG64 ) bytes, where
//  n is the number of counters added to the query. The values will be stored in the order the counters were
//  added with PerfCountersAddCounter().
// Return value: TRUE if it succeeds, FALSE, otherwise. FALSE will be returned if at least one of the counters
//  can't be retrived. That particular element won't be changed in pData.
BOOL
PerfCountersGetCounterValuesRaw(
    _In_ HANDLE         hQuery,
    __out_opt ULONG64*  pData1,
    __out_opt ULONG64*  pData2
)
;

// Description: start collecting statistics based on the data being returned from the specified query.
// hQuery: query handle returned by a previous call to PerfCountersCreateQuery().
// dwPeriod: time elapsed between samples, in milliseconds.
// pgrbitStats: combination of bits, indicating which stats should be collected. The exact number of
//  counters previously added should be passed. One JET_GRBIT for each counter. NULL indicates that
//  all the statistics should be collected for all the counters. It is invalid to collect the variance
//  without collecting the average.
// szLogFile: file to log the results to. If NULL, won't log anywhere.
// Return value: TRUE if it succeeds, FALSE, otherwise.
BOOL
PerfCountersStartCollectingStatsA(
    _In_ HANDLE             hQuery,
    _In_ DWORD              dwPeriod,
    __in_opt JET_GRBIT*     pgrbitStats,
    __in_opt PCSTR          szLogFile
)
;

BOOL
PerfCountersStartCollectingStatsW(
    _In_ HANDLE             hQuery,
    _In_ DWORD              dwPeriod,
    __in_opt JET_GRBIT*     pgrbitStats,
    __in_opt PCWSTR         wszLogFile
)
;

#ifdef _UNICODE
#define PerfCountersStartCollectingStats PerfCountersStartCollectingStatsW
#else
#define PerfCountersStartCollectingStats PerfCountersStartCollectingStatsA
#endif

// Description: retrieve statistics from the background collection under way.
// hQuery: query handle returned by a previous call to PerfCountersCreateQuery().
// pMostRecent: pointer to a user-allocated buffer, which must be capable to hold n * sizeof( double ) bytes, where
//  n is the number of counters added to the query. The values will be stored in the order the counters were
//  added with PerfCountersAddCounter().
// pDataMin: same as pMostRecent, but will hold the minimum values of the counters.
// pTimeMin: same as pMostRecent, but will store the times when the min values occurred.
// pDataMax: same as pMostRecent, but will hold the maximum values of the counters.
// pTimeMax: same as pMostRecent, but will store the times when the max values occurred.
// pDataAvg: same as pMostRecent, but will hold the average values of the counters.
// pDataVar: same as pMostRecent, but will hold the variance values of the counters.
// cCollected: this will receive the number of data collections performed.
// Return value: TRUE if it succeeds, FALSE, otherwise.
BOOL
PerfCountersRetrieveStats(
    _In_ HANDLE         hQuery,
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

// Description: stop collecting statistics based on the data being returned from the specified query.
// hQuery: query handle returned by a previous call to PerfCountersCreateQuery().
// Return value: TRUE if it succeeds, FALSE, otherwise.
BOOL
PerfCountersStopCollectingStats(
    _In_ HANDLE hQuery
)
;

// Description: destroys a query object created by PerfCountersCreateQuery().
// Return value: TRUE if it succeeds, FALSE, otherwise.
BOOL
PerfCountersDestroyQuery(
    _In_ HANDLE hQuery
)
;

inline VOID
DoubleArraySet(
    __in_ecount( cSize ) double*    pArray,
    _In_ size_t                     cSize,
    _In_ double                     dValue
)
{
    while ( cSize-- ) pArray[ cSize ] = dValue;
}

inline VOID
DoubleArrayCopy(
    __in_ecount( cSize ) double*    pDst,
    __in_ecount( cSize ) double*    pSrc,
    _In_ size_t                     cSize
)
{
    while ( cSize-- ) pDst[ cSize ] = pSrc[ cSize ];
}

//================================================================
// Performance reporting functions.
//================================================================
//
// Description: creates/opens a files for performance reporting.
// szFile: the file to be used (an example would be to use SzEsetestPerfSummaryXml()/WszEsetestPerfSummaryXml()).
// Return value: a handle to be used in subsequent calls that requires a perf reporting handle. INVALID_HANDLE_VALUE if it fails.
HANDLE
PerfReportingCreateFileA(
    _In_ PCSTR szFile
)
;

HANDLE
PerfReportingCreateFileW(
    _In_ PCWSTR wszFile
)
;

#ifdef _UNICODE
#define PerfReportingCreateFile PerfReportingCreateFileW
#else
#define PerfReportingCreateFile PerfReportingCreateFileA
#endif

// Description: writes one result to the file.
// hPerfReporting: handle returned by PerfReportingCreateFile().
// szCounterName: name of the metric to write. This string must be in printf format (ex: "%" must be passed as "%%").
// szCounterUnit: unit of the metric to write. This string must be in printf format (ex: "%" must be passed as "%%").
// szPrintfFormat: format to apply to the value. This string must be in printf format (ex: "%.3f").
// fHigherIsBetter: if a higher value is better in terms of performance improvement
// dblValue: the actual value to report.
// Return value: TRUE if it succeeds, FALSE, otherwise.
BOOL
PerfReportingReportValueA(
    _In_ HANDLE     hPerfReporting,
    _In_ PCSTR      szCounterName,
    _In_ PCSTR      szCounterUnit,
    _In_ PCSTR      szPrintfFormat,
    _In_ BOOL       fHigherIsBetter,
    _In_ double     dblValue
)
;

BOOL
PerfReportingReportValueW(
    _In_ HANDLE     hPerfReporting,
    _In_ PCWSTR     wszCounterName,
    _In_ PCWSTR     wszCounterUnit,
    _In_ PCWSTR     wszPrintfFormat,
    _In_ BOOL       fHigherIsBetter,
    _In_ double     dblValue
)
;

#ifdef _UNICODE
#define PerfReportingReportValue PerfReportingReportValueW
#else
#define PerfReportingReportValue PerfReportingReportValueA
#endif

// Description: closes/saves a performance report.
// hPerfReporting: handle returned by PerfReportingCreateFile().
// Return value: TRUE if it succeeds, FALSE, otherwise.
BOOL
PerfReportingCloseFile(
    _In_ HANDLE hPerfReporting
)
;

//================================================================
//  Performance counter collection wrapper.
//  Consolidates all the infrastructure defined above into four
//  simple functions.
//================================================================

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


//================================================================
// Event logging helper functions.
//================================================================
//
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

// Description: starts listening for events posted to the Windows Event Logs. Also, before the call
//  returns, the callback passed will be called once per each event that matches the given criteria and
//  occurred before the time of the call. The events generated after the time of the call and
//  that meet the criteria will be returned in the same callback either during or after the call
//  to this function.
// pfnCallback: callback to be called when events that match the passed criteria occur. Also, it will be called
//  before this function returns for any event that match the criteria and occurred in the past.
// wszEventLog: event log to be queried. Example: "Application". If you pass NULL, it will default to "Application".
// wszEventSources: event sources to be queried. Example: "ESENT", "ESE BACKUP".
// cEventSources: element count of wszEventSources.
// pTimeMin: pointer to a PSYSTEMTIME containing the time lower limit for event entries to be returned. NULL
//  will return since the oldest entry in the event log. It must be local time (use GetLocalTime()).
// pTimeMax: pointer to a PSYSTEMTIME containing the time higher limit for event entries to be returned. NULL
//  will query forever, until EventLoggingDestroyQuery() is called. It must be local time (use GetLocalTime()).
// pEventTypes: array of event types to filter. NULL for this parameter or an ampty array mean that we should
//  return all the typs of events.
// cEventTypes: element count of pEventTypes.
// pEventCategories: same as pEventTypes, but filters by event category.
// cEventCategories: element count of pEventCategories.
// pEventIds: same as pEventTypes, but filters by event ID
// cEventIds: element count of pEventIds.
// wszLogFile: file to log the results to. If NULL, won't log anywhere.
// pUserData: pointer to custom user data. Might be useful for identifying a specific query, when the same
//  callback is used by more then one query. Also, to identify a specific query before the handle is returned,
//  which can happen for past events.
// Return value: a handle to be used in subsequent calls that require a query handle. NULL if it fails.
HANDLE
EventLoggingCreateQuery(
    __in_opt PFNEVENTLOGGING                    pfnCallback,
    __in_opt PCWSTR                             wszEventLog,
    __in_ecount_opt( cEventSources ) PCWSTR*    pwszEventSources,
    _In_ size_t                                 cEventSources,
    __in_opt PSYSTEMTIME                        pTimeMin,
    __in_opt PSYSTEMTIME                        pTimeMax,
    __in_ecount_opt( cEventTypes ) PWORD        pEventTypes,
    _In_ size_t                                 cEventTypes,
    __in_ecount_opt( cEventCategories ) PWORD   pEventCategories,
    _In_ size_t                                 cEventCategories,
    __in_ecount_opt( cEventIds ) PDWORD         pEventIds,
    _In_ size_t                                 cEventIds,
    __in_opt PCWSTR                             wszLogFile,
    __in_opt PVOID                              pUserData
)
;

// Description: stops listening for events. Even if the callback passed to EventLoggingCreateQuery() is not
//  expected to be called anymore after EventLoggingCreateQuery() returns (for example, if you passed a
//  pTimeMax less than the current time), you still must call EventLoggingDestroyQuery() in order to finalize
//  the handle and free the resources.
// Return value: TRUE if it succeeds, FALSE, otherwise.
BOOL
EventLoggingDestroyQuery(
    _In_ HANDLE hQuery
)
;

// Description: formats an event log message.
// hModule: handle to the module that contains the message table definition (for example: ese.dll, esent.dll,
//  eseback2.dll, etc...). BTW, events generated by either eseback2.dll and esebcli2.dll can be mapped using
//  either one of these two modules. This is the handle returned by LoadLibrary() or GetModuleHandle().
// dwEventId: event ID.
// dwLandIg: language ID. Example: use GetSystemDefaultLCID() to follow the OS language.
// pwszStrings: array of pointers to PWSTRs containing the texts to be replaced in the message definition.
// Return value: a pointer to the formatted message. This is allocated internally and therefore
//  be freed used LocalFree().
PWSTR
EventLoggingFormatMessage(
    _In_ HMODULE        hModule,
    _In_ DWORD          dwEventId,
    _In_ LCID           dwLandIg,
    __in_opt PWSTR*     pwszStrings
)
;

// Description: returns the ESE-related (ese.dll, esent.dll or eseback2.dll) module handle from the event log
//  source description. Example: passing "ESE BACKUP" returns a handle to eseback2.dll. This is useful to
//  trieve a module handle to pass to EventLoggingFormatMessage(). However, we can retrieve the handle to any
//  module we want by doing LoadLibrary() (or GetModuleHandle() if it is already loaded) against the module
//  that contains the message definitions. We can find this out in:
//  HKLM\SYSTEM\CurrentControlSet\Services\EventLog\<EventLog>\<EventSource>\EventMessageFile
// wszEventSource: event log source.
// Return value: handle to the module, as explained above. NULL if it fails.
HMODULE
EventLoggingModuleFromEventSource(
    _In_ PCWSTR     wszEventSource
)
;

// Description: prints an event occurence on screen.
// Input parameters: see comments on the functions above. The names are self-descriptive.
// Return value: handle to the module, as explained above. NULL if it fails.
VOID
EventLoggingPrintEvent(
    _In_ PCWSTR                         wszEventLog,
    _In_ PCWSTR                         wszEventSource,
    _In_ PSYSTEMTIME                    pTimeGenerated,
    _In_ WORD                           wEventType,
    _In_ WORD                           wEventCategory,
    _In_ DWORD                          dwEventId,
    _In_ DWORD                          dwLangId,
    __in_ecount_opt( cStrings ) PWSTR*  pwszStrings,
    _In_ WORD                           cStrings,
    __in_bcount_opt( cbRawData ) PVOID  pRawData,
    _In_ DWORD                          cbRawData
)
;

//*******************************************************************


//================================================================
//  Helper APIs to compute the Pearson statistical correlation
//  coefficient between two variables.
//  These functions are all thread-safe, except for destroying
//  the object.
//================================================================
//

//  Description: creates an opaque handle to use with the PearsonCorrelation family of functions.
//  Return value: an opaque handle to use with the PearsonCorrelation family of functions.
HANDLE
PearsonCorrelationCreate();

//  Description: reset the statistics (PearsonCorrelationCreate() automatically resets them).
//  h: handle created by PearsonCorrelationCreate().
void
PearsonCorrelationReset(
    __in_opt HANDLE     h
);

//  Description: feeds the object with a new sample. Calling this function
//      when a background performance counter collection is under way will
//      add the sample as an additional data point.
//  h: handle created by PearsonCorrelationCreate().
//  x: sample of variable X.
//  y: sample of variable Y.
//  Return value: the updated value of the correlation coefficient.
double
PearsonCorrelationNewSample(
    __in_opt HANDLE     h,
    _In_ const double   x,
    _In_ const double   y
);

//  Description: starts sampling performance counters and computing the pearson
//      coefficient on the background.
//  h: handle created by PearsonCorrelationCreate().
//  szComputerNameX: see above on the PerfCounters family of functions.
//  szPerfObjectX: see above on the PerfCounters family of functions.
//  szPerfCounterX: see above on the PerfCounters family of functions.
//  szInstanceX: see above on the PerfCounters family of functions.
//  szComputerNameY: see above on the PerfCounters family of functions.
//  szPerfObjectY: see above on the PerfCounters family of functions.
//  szPerfCounterY: see above on the PerfCounters family of functions.
//  szInstanceY: see above on the PerfCounters family of functions.
//  dwPeriod: time elapsed between samples, in milliseconds.
//  Return value: true if it succeeds, false, otherwise.
bool
PearsonCorrelationStartSampling(
    __in_opt HANDLE     h,
    __in_opt PCSTR      szComputerNameX,
    _In_ PCSTR          szPerfObjectX,
    _In_ PCSTR          szPerfCounterX,
    _In_ PCSTR          szInstanceX,
    __in_opt PCSTR      szComputerNameY,
    _In_ PCSTR          szPerfObjectY,
    _In_ PCSTR          szPerfCounterY,
    _In_ PCSTR          szInstanceY,
    _In_ DWORD          dwPeriod
);

//  Description: stops sampling performance counters.
//  h: handle created by PearsonCorrelationCreate().
//  Return value: true if it succeeds, false, otherwise.
bool
PearsonCorrelationStopSampling(
    __in_opt HANDLE     h
);

//  Description: returns the current correlation coefficient.
//  h: handle created by PearsonCorrelationCreate().
//  Return value: the current value of the correlation coefficient.
double
PearsonCorrelationGetCoefficient(
    __in_opt HANDLE     h
);

//  Description: destroys the object.
//  h: handle created by PearsonCorrelationCreate().
void
PearsonCorrelationDestroy(
    __in_opt HANDLE     h
);

//*******************************************************************


// These should be defined in ese_common.hxx
// ese_common.hxx is the 'newer' version of this file. The intent of ese_common.hxx is (taken from the top of the file) :
// This file includes niceties like 'Call' and NO_GRBIT. It's a superset of esetest.h If I changed esetest.h directly, I'd have to
// change ALL the tests at once. This allows a phase-in.

// Don't support Unicode text just yet.
void __stdcall EsetestAssertFail( const char * const szMessageFormat, char const *szFilename, long lLine, ... );

#ifdef DEBUG
//#define AssertSz( exp, sz )       ( ( exp ) ? (void) 0 : EsetestAssertFail( _T( sz ), _T( __FILE__ ), __LINE__ ) )
#define AssertSz( exp, sz )         ( ( exp ) ? (void) 0 : EsetestAssertFail( sz, __FILE__, __LINE__ ) )
#else  //  !DEBUG
#define AssertSz( exp, sz )
#endif  //  DEBUG

#define VerifySz( exp, sz )         ( ( exp ) ? (void) 0 : EsetestAssertFail( sz, __FILE__, __LINE__ ) )

#undef UNICODE
#undef _UNICODE

// Our custom assert
#define AssertM( exp )          AssertSz( exp, #exp )

// Verify is like an assert, but it also exists in retail/free builds
#define VerifyP( exp )              VerifySz( exp, #exp )

#define PremiseSz( exp, sz )        ( ( exp ) ? ( void )0 : EsetestAssertFail( sz, __FILE__, __LINE__ ) )
// What's a Premise()?
#define Premise( exp )          PremiseSz( exp, #exp )
// What's an Assume()?
#define Assume( exp )           Premise( exp )

//================================================================
// Scan through string, substitute oldChar to NewChar, return the number of changes
// if ( NULL==string or 0 == oldChar ) { don't touch string, return 0; }
//================================================================
size_t strxchg(
    _In_ CHAR *string,
    _In_ const CHAR oldChar,
    _In_ const CHAR newChar
);

size_t wcsxchg(
    _In_ WCHAR *string,
    _In_ const WCHAR oldChar,
    _In_ const WCHAR newChar
);

size_t _tcsxchg(
    _In_ TCHAR *string,
    _In_ const TCHAR oldChar,
    _In_ const TCHAR newChar
);

//================================================================
// Like strdup(), but uses new[].
inline
char*
EsetestStrDup(
    _In_ const char*    sz
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

//================================================================
// Constants and functions to facilitate Output Message
//================================================================
enum ESETEST_OUTPUT_LEVEL { OUTPUT_NONE = 0, OUTPUT_ERROR, OUTPUT_WARNING, OUTPUT_INFO, OUTPUT_DEBUG, OUTPUT_ALL };
enum ESETEST_OUTPUT_COLOR { COLOR_INFO = ANSI_GREEN, COLOR_WARNING = ANSI_YELLOW, COLOR_ERROR = ANSI_RED, COLOR_DEBUG = ANSI_SILVER };

// extern size_t g_OutputLevel;
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
    _In_ PSTR           szFunction,
    _In_ PCSTR      szToCopy
)
;

wchar_t*
EsetestCopyWideString(
    _In_ PSTR           szFunction,
    _In_ PCWSTR     wszToCopy
)
;


wchar_t*
EsetestWidenString(
    _In_ PSTR   szFunction,
    _In_ PCSTR  sz
)
;

char*
EsetestUnwidenStringAlloc(
    _In_ PSTR   szFunction,
    _In_ PCWSTR wsz
)
;

JET_ERR
EsetestUnwidenString(
    _In_ PSTR   szFunction,
    _In_ PWSTR  wsz,
    __inout PSTR    sz
)
;

JET_ERR
EsetestCleanupWidenString(
    _In_ PSTR   szFunction,
    __inout PWSTR   wsz,
    _In_ PCSTR  sz
)
;

// re-enable warnings
#pragma warning( pop )

#endif  // _ESETEST_4357768_H
