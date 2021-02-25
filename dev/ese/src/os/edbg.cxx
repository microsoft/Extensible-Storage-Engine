// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

//  we use LoadLibrary in here to test loading our own DLL for EDBGLoad
#undef LoadLibraryExW

// SOURCE INSIGHT USERS:
// Add the following to your c.tom file:
/*
DEBUG_EXT( name )       VOID name(  const PDEBUG_CLIENT pdebugClient, const INT argc, const CHAR * const argv[]  )
*/

// Needed only for the debugger versioning.
#if defined(ESENT)
#include "ntverp.h"
#elif defined(ESEEX)
#include "bldver.h"
#elif !( defined(PRODUCT_MAJOR) && defined(PRODUCT_MINOR) && defined(BUILD_MAJOR) && defined(BUILD_MINOR) )
#error "Please define PRODUCT_MAJOR, PRODUCT_MINOR, BUILD_MAJOR, and BUILD_MINOR in the build environment"
#endif


#pragma push_macro( "Alloc" )
// The custom Alloc macro messes up structures in dbgeng.h.
#undef Alloc
#include <dbgeng.h>
#pragma pop_macro( "Alloc" )

// Allows easier porting from the older wdbgexts-style extensions.
#define dprintf EDBGPrintf

#ifdef DEBUGGER_EXTENSION

#ifdef DEBUGGER_EXTENSION_ESE

#include "esestd.hxx"
#include "_bf.hxx"
#include "_tls.hxx"
#include "_cresmgr.hxx"
#include "errdata.hxx"
#include "_logstream.hxx"
#include "_logwrite.hxx"
#include "_logread.hxx"

#include "malloc.h" // needed for alloca()

// Not worth fixing FORMAT_xxx macros which generate bit shift warnings.
#pragma warning(disable:4293)

// Until Exchange updates their dbgeng.h file:
#ifndef DEBUG_OUTCTL_AMBIENT_DML
#define DEBUG_OUTCTL_AMBIENT_DML       0xfffffffe
#endif

#ifndef DEBUG_OUTCTL_AMBIENT_TEXT
#define DEBUG_OUTCTL_AMBIENT_TEXT      0xffffffff
#endif

// We may need to store symbols, including their modules.
#define MAX_SYMBOL                      (2 * MAX_PATH + 1)

// Pdls() redirected 
VOID AssertEDBGDebugger();

#ifdef DEBUG
#define AssertEDBGSz( f, sz )           \
        if ( !(f) )                     \
        {                               \
            dprintf( "EDBG Constraint Violation: \"%s\".\n", #sz );                         \
            dprintf( "File %s, Line %d.\n", __FILE__, __LINE__ );                           \
            dprintf( "WARNING: blindly plundering on (may cause edbg malfunction).\n" );    \
        }
#define AssertEDBG( sz )    AssertEDBGSz( sz, sz );
#else
#define AssertEDBGSz( f, sz )
#define AssertEDBG( sz )
#endif


// for now just assert, but some point would love to throw an exception and catch
// it in the !ese main, so that we don't AV the debugger.
#define EDBGPanic       AssertEDBG

// for possible future use ...

#define ErrEDBGCheck( err )         ErrERRCheck( err )

#define EDBGAddGlobal( x, command ) EDBGGlobals( #x, (ULONG_PTR)&x, command )

extern const SIZE_T     cEDBGGlobals;
extern DWORD            g_cAllocHeap;
extern DWORD            g_cFreeHeap;
extern DWORD            g_cbAllocHeap;
extern DWORD_PTR        g_cbReservePage;
extern DWORD_PTR        g_cbCommitPage;
extern CJetParam* const g_rgparam;
extern const size_t     g_cparam;
extern DWORD            g_cioOutstandingMax;
extern volatile DWORD   g_cioreqInUse;
extern HRT              g_hrtLastGiven;
extern HRT              g_hrtHRTFreq;
extern TICK             g_tickLastGiven;
extern _TLS*            g_ptlsGlobal;
extern ULONG            g_cpinstMax;
extern LONG             g_cioConcurrentMetedOpsMax;
extern LONG             g_cioLowQueueThreshold;
extern LONG             g_dtickStarvedMetedOpThreshold;
extern DWORD            g_cbIoreqChunk;
extern IOREQCHUNK *     g_pioreqchunkRoot;

namespace OSSYNC
{
    extern VOID *       g_rgPLS[];
    extern DWORD        g_cProcessorMax;
}

extern TableClassNamesLifetimeManager g_tableclassnames;

// In order to picked up by the DATA export in the DLL, it needs
// to be marked as extern C.
extern "C" {
extern const EDBGGlobals rgEDBGGlobals[];
} // extern "C"

const EDBGGlobals rgEDBGGlobals[] =
{
    EDBGAddGlobal( cEDBGGlobals, NULL ),  //  must be the first entry of the array

    EDBGAddGlobal( g_tickLastGiven, NULL ),
    EDBGAddGlobal( g_cparam, NULL ),
    EDBGAddGlobal( g_rgparam, NULL ),
    EDBGAddGlobal( g_rgfmp, "!ese dump fmp" ),
    EDBGAddGlobal( g_ifmpMax, NULL ),
    EDBGAddGlobal( FMP::s_ifmpMacCommitted, NULL ),
    EDBGAddGlobal( g_rgpinst, "!ese dump inst" ),
    EDBGAddGlobal( g_cpinstMax, NULL ),
    EDBGAddGlobal( g_pRMContainer, NULL ),
    EDBGAddGlobal( g_cbfChunk, NULL ),
    EDBGAddGlobal( g_rgpbfChunk, NULL ),
    EDBGAddGlobal( cbfCacheAddressable, NULL ),
    EDBGAddGlobal( cbfCacheSize, NULL ),
    EDBGAddGlobal( cbfCacheTarget, NULL ),
    EDBGAddGlobal( g_cbfCacheTargetOptimal, NULL ),
    EDBGAddGlobal( g_cbfCacheResident, NULL ),
    EDBGAddGlobal( g_cbfNewlyCommitted, NULL ),
    EDBGAddGlobal( g_cbCacheCommittedSize, NULL ),
    EDBGAddGlobal( g_cbCacheReservedSize, NULL ),
    EDBGAddGlobal( g_rgcbfCachePages, NULL ),
    EDBGAddGlobal( g_rgcbPageSize, NULL ),
    EDBGAddGlobal( g_pBFAllocLookasideList, NULL ),
    EDBGAddGlobal( cbfInit, NULL ),
    EDBGAddGlobal( g_cpgChunk, NULL ),
    EDBGAddGlobal( g_rgpvChunk, NULL ),
    EDBGAddGlobal( g_cAllocHeap, NULL ),
    EDBGAddGlobal( g_cFreeHeap, NULL ),
    EDBGAddGlobal( g_cbAllocHeap, NULL ),
    EDBGAddGlobal( g_cbReservePage, NULL ),
    EDBGAddGlobal( g_cbCommitPage, NULL ),
    EDBGAddGlobal( CPAGE::cbHintCache, NULL ),
    EDBGAddGlobal( CPAGE::maskHintCache, NULL ),
    EDBGAddGlobal( g_cioOutstandingMax, NULL ),
    EDBGAddGlobal( g_cioreqInUse, NULL ),
    EDBGAddGlobal( RESINST, NULL ),
    EDBGAddGlobal( RESLOG, NULL ),
    EDBGAddGlobal( RESSPLIT, NULL ),
    EDBGAddGlobal( RESSPLITPATH, NULL ),
    EDBGAddGlobal( RESMERGE, NULL ),
    EDBGAddGlobal( RESMERGEPATH, NULL ),
    EDBGAddGlobal( RESKEY, NULL ),
    EDBGAddGlobal( RESBOOKMARK, NULL ),
    EDBGAddGlobal( RESLRUKHIST, NULL ),
    EDBGAddGlobal( g_cbPlsMemRequiredPerPerfInstance, NULL ),
    EDBGAddGlobal( OSSYNC::g_rgPLS, NULL ),
    EDBGAddGlobal( OSSYNC::g_cProcessorMax, NULL ),
    EDBGAddGlobal( PLS::s_pbPerfCounters, NULL ),
    EDBGAddGlobal( PLS::s_cbPerfCounters, NULL ),
    EDBGAddGlobal( g_fMemCheck, NULL ),
    EDBGAddGlobal( g_hrtLastGiven, NULL ),
    EDBGAddGlobal( g_ptlsGlobal, NULL ),
    EDBGAddGlobal( g_tableclassnames, NULL ),
    EDBGAddGlobal( g_cioConcurrentMetedOpsMax, NULL ),
    EDBGAddGlobal( g_cioLowQueueThreshold, NULL ),
    EDBGAddGlobal( g_dtickStarvedMetedOpThreshold, NULL ),
    EDBGAddGlobal( g_cbIoreqChunk, NULL ),
    EDBGAddGlobal( g_pioreqchunkRoot, NULL ),
    EDBGAddGlobal( g_hrtLastGiven, NULL ),
    EDBGAddGlobal( g_hrtHRTFreq, NULL ),
    EDBGAddGlobal( g_rgScavengeTimeSeq, NULL ),
    EDBGAddGlobal( g_rgScavengeLastRuns, NULL ),
#ifndef RTM
    EDBGAddGlobal( g_rgScavengeTimeSeqCumulative, NULL ),
#endif
    EDBGAddGlobal( g_cScavengeTimeSeq, NULL ),
    EDBGAddGlobal( g_iScavengeTimeSeqLast, NULL ),
    EDBGAddGlobal( g_iScavengeLastRun, NULL ),
};

const SIZE_T cEDBGGlobals = _countof(rgEDBGGlobals);

// Used by INST.
const EDBGGlobals* rgEDBGGlobalsArray = rgEDBGGlobals;

//  debugger's copy of the globals table
EDBGGlobals * g_rgEDBGGlobalsDebugger = NULL;


//  ****************************************************************
//  STRUCTURES AND CLASSES
//  ****************************************************************


//  ================================================================
typedef VOID (*EDBGFUNC)(
//  ================================================================
    const PDEBUG_CLIENT pdebugClient,
    const INT argc,
    const CHAR * const argv[]
    );


//  ================================================================
class CPRINTFWDBG : public CPRINTF
//  ================================================================
{
    public:
        VOID __cdecl operator()( const char * szFormat, ... );
        static CPRINTF * PcprintfInstance();

        ~CPRINTFWDBG() {}

    private:
        CPRINTFWDBG() {}
        static CHAR szBuf[1024];    //  WARNING: not multi-threaded safe!
};


//  ================================================================
class CDUMP
//  ================================================================
{
    public:
        CDUMP() {}

        virtual VOID Dump( PDEBUG_CLIENT, INT, const CHAR * const [] ) const = 0;
};


//  ================================================================
template< class _STRUCT>
class CDUMPA : public CDUMP
//  ================================================================
{
    public:
        VOID Dump(
                PDEBUG_CLIENT pdebugClient,
                INT argc,
                const CHAR * const argv[] ) const;
        static CDUMPA   instance;
};


//  ================================================================
struct EDBGFUNCMAP
//  ================================================================
{
    const char *    szCommand;
    EDBGFUNC        function;
    const char *    szHelp;
};

#define bitEdbgDumpKeys     (0x10)

//  ================================================================
struct CDUMPMAP
//  ================================================================
{
    const char *    szCommand;
    CDUMP      *    pcdump;
    const char *    szHelp;
};


//  HACK: declare dummy instances of these classes to allow dumping them
//
typedef CDynamicHashTable<DWORD,DWORD>                  CDynamicHashTableEDBG;
typedef CApproximateIndex<DWORD,DWORD,0>                CApproximateIndexEDBG;
typedef CInvasiveList<DWORD,0>                          CInvasiveListEDBG;
typedef CLRUKResourceUtilityManager<2,DWORD,0,DWORD>    CLRUKResourceUtilityManagerEDBG;


//  ****************************************************************
//  PROTOTYPES
//  ****************************************************************


#define VariableNameToString( var ) #var

#define DEBUG_EXT( name )                   \
    LOCAL VOID name(                        \
        const PDEBUG_CLIENT pdebugClient,   \
        const INT argc,                     \
        const CHAR * const argv[]  )

HRESULT
DPrintf(
    __in PCSTR szFormat,
    ...
)
;

LOCAL BOOL FFetchGlobalParamsArray(
    _Deref_out_ CJetParam** prgparam,
    _Out_ size_t* pcparam );

DEBUG_EXT( EDBGBFOB0FindOldest );
DEBUG_EXT( EDBGCacheFind );
DEBUG_EXT( EDBGCacheFindOldest );
DEBUG_EXT( EDBGCacheQuery );
DEBUG_EXT( EDBGCacheScavengeRuns );
DEBUG_EXT( EDBGCacheSum );
DEBUG_EXT( EDBGCacheMap );
DEBUG_EXT( EDBGChecksum );
DEBUG_EXT( EDBGDebug );
DEBUG_EXT( EDBGDecompress );
DEBUG_EXT( EDBGDecrypt );
DEBUG_EXT( EDBGDump );
DEBUG_EXT( EDBGDumpCacheInfo );
DEBUG_EXT( EDBGDumpCacheMap );
DEBUG_EXT( EDBGDumpDBDiskPage );
DEBUG_EXT( EDBGDumpFCBInfo );
DEBUG_EXT( EDBGDumpFCBs );
DEBUG_EXT( EDBGDumpPIBs );
DEBUG_EXT( EDBGDumpInvasiveList );
DEBUG_EXT( EDBGDumpIOREQs );
#if LOCAL_IS_WORKING
LOCAL_BROKEN DEBUG_EXT( EDBGDumpPendingIO );
#endif
DEBUG_EXT( EDBGDumpLinkedList );
DEBUG_EXT( EDBGDumpMetaData );
DEBUG_EXT( EDBGDumpNode );
DEBUG_EXT( EDBGDumpPerfctr );
DEBUG_EXT( EDBGDumpLR );
DEBUG_EXT( EDBGErr );
DEBUG_EXT( EDBGFCBFind );
DEBUG_EXT( EDBGGlobal );
DEBUG_EXT( EDBGHash );
DEBUG_EXT( EDBGHelp );
DEBUG_EXT( EDBGHelpDump );
DEBUG_EXT( EDBGLoad );
DEBUG_EXT( EDBGMemory );
DEBUG_EXT( EDBGParam );
DEBUG_EXT( EDBGSeek );
DEBUG_EXT( EDBGSync );
DEBUG_EXT( EDBGTableFind );
DEBUG_EXT( EDBGTest );
DEBUG_EXT( EDBGThreadErr );
DEBUG_EXT( EDBGTid2PIB );
DEBUG_EXT( EDBGTid2TLS );
DEBUG_EXT( EDBGUnload );
DEBUG_EXT( EDBGVersion );
DEBUG_EXT( EDBGVerStore );
DEBUG_EXT( EDBGVerHashSum );
DEBUG_EXT( EDBGDumpAllFMPs );
DEBUG_EXT( EDBGDumpAllINSTs );
DEBUG_EXT( EDBGDumpRESPointer );
DEBUG_EXT( EDBGDumpRMResID );
DEBUG_EXT( EDBGDumpRCIOutStandingResObjs );
DEBUG_EXT( EDBGDumpRMOutStandingResObjs );
DEBUG_EXT( EDBGCopyFile );
DEBUG_EXT( EDBGDumpReferenceLog );
DEBUG_EXT( EDBGSetImplicitDB );
DEBUG_EXT( EDBGSetImplicitInst );
DEBUG_EXT( EDBGSetImplicitBT );
DEBUG_EXT( EDBGSetPii );


extern VOID DBUTLDumpRec( const LONG cbPage, const FUCB * const pfucbTable, const VOID * const pv, const INT cb, CPRINTF * pcprintf, const INT cbWidth );



//  ****************************************************************
//  GLOBALS
//  ****************************************************************



LOCAL PDEBUG_CLIENT5 g_DebugClient;
LOCAL PDEBUG_CONTROL4 g_DebugControl;
LOCAL PDEBUG_SYMBOLS g_DebugSymbols;
LOCAL PDEBUG_SYSTEM_OBJECTS g_DebugSystemObjects;
LOCAL PDEBUG_DATA_SPACES g_DebugDataSpaces;

//  EDBG Debugging facilities, which is basically "printf" debugging...
//
enum
{
    eDebugModeNone      = 0,    //
    eDebugModeErrors    = 1,    //  [Default] Print errors and AssertEDBG()s.
    eDebugModeAlternate = 2,    //  Use alternate / legacy command implementations.
    eDebugModeBasic     = 3,    //  Basic debug tracing / stuff.
    eDebugModeMore      = 4,    //
    eDebugModeVerbose   = 5,    //
    eDebugModeMicroOps  = 6,
};
LOCAL ULONG     g_eDebugMode        = eDebugModeErrors;
#define         fDebugMode          (g_eDebugMode >= eDebugModeBasic )

LOCAL BOOL      g_fTryInitEDBGGlobals = fFalse;   //  debugger extension has tried to auto-load the globals table
LOCAL ULONG     g_cSymInitFail        = 0;        //  number of times attempts to initialise the symbols sub-system failed
const ULONG     cSymInitAttemptsMax = 3;        //  max attempts to initialise the symbols sub-system

// REVIEW: Is this obsolete with the new dbgeng functions?
LOCAL HINSTANCE g_hLibrary            = NULL;     //  if we load outselves

CHAR CPRINTFWDBG::szBuf[1024];

template< class _STRUCT>
CDUMPA<_STRUCT> CDUMPA<_STRUCT>::instance;

//  ================================================================
LOCAL const EDBGFUNCMAP rgfuncmap[] = {
//  ================================================================

{
        "HELP",             EDBGHelp,
        "<link cmd=\"!ese help\">HELP</link>                               - Print this help message"
},
{
        ".db",              EDBGSetImplicitDB,
        ".db &lt;dbname|ifmp|.&gt;                - Sets the implicit FMP / ifmp (and also .inst) to specified DB or ifmp array slot or to '.' (picks single attached DB)."
},
{
        ".inst",            EDBGSetImplicitInst,
        ".inst &lt;instname|ipinst&gt;            - Sets  the implicit INST to the specified instance name or ipinst array slot."
},
{
        ".bt",              EDBGSetImplicitBT,
        ".bt &lt;tablename|objidFDP&gt;[\\&lt;IndexName|'(LV)'&gt;][&lt;'[OE]'|'[AE]'&gt;]\n\t"
        "                                   - Sets the implicit B+ Tree (BT) to the specified table\\index|sub-tree name or objidFDP."
},
{
        ".pii",             EDBGSetPii,
        ".pii On|Off                        - Sets whether to allow dumping of private data."
},
{
        "BFOB0FINDOLDEST",  EDBGBFOB0FindOldest,
        "BFOB0FINDOLDEST &lt;ifmp|.&gt; [&lt;gen&gt;]   - Finds the BFOB0 entry of the given ifmp containing the oldest lgposBegin0"
},
{
        "CACHEFIND",        EDBGCacheFind,
        "CACHEFIND &lt;ifmp|.&gt; &lt;pgno&gt;          - Finds the BF containing the given ifmp:pgno"
},
{
        "CACHEFINDOLDEST",  EDBGCacheFindOldest,
        "CACHEFINDOLDEST &lt;ifmp|.&gt; [&lt;gen&gt;]   - Finds the BF of the given ifmp containing the oldest lgposBegin0"
},
{
        "CACHEQUERY",       EDBGCacheQuery,
        "CACHEQUERY &lt;query cond&gt; &lt;action&gt;   - Finds the BF with the specified query options."
},
{
        "CACHESCAVENGERUNS",EDBGCacheScavengeRuns,
        "<link cmd=\"!ese cachescavengeruns\">CACHESCAVENGERUNS</link>                  - Summarizes BFs results the last runs of scavenging."
},
{
        "CACHESUM",         EDBGCacheSum,
        "<link cmd=\"!ese cachesum\">CACHESUM</link> [verbose]                 - Summarizes the contents of the cache from BFs perspective."
},
{
        "CACHEMAP",         EDBGCacheMap,
        "CACHEMAP &lt;address&gt;                 - Performs pv =&gt; pbf or pbf =&gt; pv mapping"
},
{
        "CHECKSUM",         EDBGChecksum,
        "CHECKSUM &lt;address&gt; [&lt;length&gt;] [&lt;pgno&gt;]\n\t"
        "                                   - Checksum a &lt;JET_paramDatabasePageSize&gt; range of memory"
},
{
        "COPYFILE",         EDBGCopyFile,
        "COPYFILE &lt;handle&gt; &lt;destination&gt; [offsetStart offsetEnd]\n\t"
        "                                   - Copies the specified file handle to a new file"
},
{
        "DEBUG",            EDBGDebug,
        "<link cmd=\"!ese debug\">DEBUG</link>                              - Toggle debug mode"
},
{
        "DECOMPRESS",       EDBGDecompress,
        "DECOMPRESS &lt;pb&gt; &lt;cb&gt;               - Decompress a compressed byte stream"
},
{
        "DECRYPT",          EDBGDecrypt,
        "DECRYPT &lt;pb&gt; &lt;cb&gt;                  - Decrypt an encrypted byte stream"
},
{
        "DUMP",             EDBGDump,
        "DUMP &lt;class&gt; &lt;address&gt;             - Dump an ESE structure at the given address"
},
{
        "DUMPCACHEINFO",    EDBGDumpCacheInfo,
        "<link cmd=\"!ese dumpcacheinfo\">DUMPCACHEINFO</link> [&lt;szTable&gt;] [&lt;ifmp|.&gt;] - Dumps info on pages cached in the buffer manager"
},
{
        "DUMPCACHEMAP",     EDBGDumpCacheMap,
        "<link cmd=\"!ese dumpcachemap\">DUMPCACHEMAP</link>                       - Dump the database cache memory map"
},
{
        "DUMPDBDISKPAGE",   EDBGDumpDBDiskPage,
        "DUMPDBDISKPAGE &lt;ifmp|.&gt; &lt;pgno&gt; [a|b|h|t|*|2|4|8|16|32]\n\t"
        "                                   - Read a database page from disk and dump it"
},
{
        "DUMPFCBINFO",      EDBGDumpFCBInfo,
            "<link cmd=\"!ese DUMPFCBINFO\">DUMPFCBINFO</link> [&lt;pinst|.&gt;]            - Dump information on all FCBs from an instance or from all instances"
},
{
        "DUMPFCBS",         EDBGDumpFCBs,
            "<link cmd=\"!ese DUMPFCBS\">DUMPFCBS</link> [*|i|a|p|u] [&lt;pinst|.&gt;]   - Dump all FCBs from an instance or from all instances, optionally\n\t"
            "                                     filtering by FCB usage (All, In-Use, Available, Purgeable, Unpurgeable)"
},
{
        "DUMPFMPS",         EDBGDumpAllFMPs,
        "<link cmd=\"!ese dumpfmps\">DUMPFMPS</link> [&lt;g_rgfmp&gt; &lt;g_ifmpMax&gt;] [*]   - Dump all used FMPs ([*] - also dump unused FMPs)"
},
{
        "DUMPINSTS",        EDBGDumpAllINSTs,
        "<link cmd=\"!ese dumpinsts\">DUMPINSTS</link> [&lt;g_rgpinst&gt;] [&lt;g_cinstMax&gt;]  - Dump all instances"
},
{
        "DUMPINVASIVELIST", EDBGDumpInvasiveList,
        "DUMPINVASIVELIST &lt;address&gt; [+/-]&lt;offset&gt; [&lt;dwords to display&gt; &lt;max elements&gt;]\n\t"
        "                                   - Dump elements of an invasive list"
},
{
        "DUMPIOREQS",       EDBGDumpIOREQs,
        "<link cmd=\"!ese dumpioreqs\">DUMPIOREQS</link> [&lt;ioreqtype&gt;] [dumpallio|dumpallioreq] - Dumps count of all IOREQ's by type"
},
{
        "DUMPLINKEDLIST",   EDBGDumpLinkedList,
        "DUMPLINKEDLIST &lt;address&gt; &lt;offset&gt; [&lt;dwords to display&gt; &lt;max elements&gt;]\n\t"
        "                                   - Dump elements of a linked list"
},
{
        "DUMPLR",           EDBGDumpLR,
        "DUMPLR &lt;address&gt; [&lt;max recs&gt; [old]]\n\t"
        "                                   - Dump a log record"
},
{
        "DUMPMETADATA",     EDBGDumpMetaData,
        "DUMPMETADATA &lt;pfcb|.&gt;              - Dumps meta-data for the specified FCB"
},
{
        "DUMPNODE",         EDBGDumpNode,
        "DUMPNODE &lt;pvPage&gt; &lt;iLine&gt; [k|v|2|4|8|16|32]\n\t"
        "                                   - Dumps the contents of a node on a page"
},
{
        "DUMPPERFCTR",      EDBGDumpPerfctr,
        "DUMPPERFCTR &lt;pperfinstance&gt; [&lt;cbSize&gt;]\n\t"
        "                                   - Dumps the specified performance counter"
},
{
        "DUMPPIBS",     EDBGDumpPIBs,
            "<link cmd=\"!ese DUMPPIBS\">DUMPPIBS</link> [&lt;pinst|.&gt;]               - Dump all database sessions (PIBs) from an instance or from all instances"
},
{
        "DUMPRCIOUTSTANDINGRESOBJECTS",     EDBGDumpRCIOutStandingResObjs,
        "DUMPRCIOUTSTANDINGRESOBJECTS &lt;RCIAddress&gt;\n\t"
        "                                   - Dump outstanding res objects in the ResourceChunkInfo/RCI"
},
{
        "DUMPRMOUTSTANDINGRESOBJECTS",      EDBGDumpRMOutStandingResObjs,
        "DUMPRMOUTSTANDINGRESOBJECTS &lt;RMAddress&gt;\n\t"
        "                                   - Dump outstanding res objects in the ResrourceManager/RM"
},
{
        "GLOBALS",          EDBGGlobal,
        "<link cmd=\"!ese globals\">GLOBALS</link> [&lt;rgEDBGGlobals&gt;]          - Provides pointer to the debuggee's table of globals (for use when symbol mapping does not work)"
},
{
        "ERR",              EDBGErr,
        "ERR &lt;error&gt;                        - Turn an error number into a string"
},
{
        "FCBFIND",          EDBGFCBFind,
        "FCBFIND &lt;FDP&gt; [&lt;instance&gt;]         - Finds all FCB's with an objidFDP or pgnoFDP matching &lt;FDP&gt;"
},
{
        "HASH",             EDBGHash,
        "HASH &lt;ifmp|.&gt; &lt;pgnoFDP&gt; &lt;pbPrefix&gt; &lt;cbPrefix&gt; &lt;pbSuffix&gt; &lt;cbSuffix&gt; &lt;pbData&gt; &lt;cbData&gt;\n\t"
        "                                   - Generate the version store hash for a given key"
},
{
        "LOAD",             EDBGLoad,
        "<link cmd=\"!ese load\">LOAD</link> [&lt;rgEDBGGlobals&gt;]             - Load the DLL and optionally load the debuggee's table of globals"
},
{
        "MEMORY",           EDBGMemory,
        "<link cmd=\"!ese memory\">MEMORY</link>                             - Dump memory usage information"
},
{
        "PARAM",            EDBGParam,
        "<link cmd=\"!ese param\">PARAM</link> [&lt;pinst|rgparam|.&gt;] [substr] - Dump system parameter settings"
},
{
        "REF",          EDBGDumpReferenceLog,
        "REF &lt;trace_log&gt;                    - Dump the specified ref-trace log"
},
{
        "RESOBJECT",        EDBGDumpRESPointer,
        "RESOBJECT &lt;address&gt;                - Identify RES object by pointer"
},
{
        "RMRESID",      EDBGDumpRMResID,
        "RMRESID [&lt;ResID&gt;]                  - Dump Resource Manager by ResID. If no ResID is specified, dump all"
},
{
        "SEEK",             EDBGSeek,
        "SEEK &lt;ifmp|.&gt; &lt;pgnoRoot|.&gt; &lt;pbPrefix&gt; &lt;cbPrefix&gt; [&lt;pbSuffix&gt; &lt;cbSuffix&gt; [&lt;pbData&gt; &lt;cbData&gt;]]\n\t"
        "                                   - Seeks to the specified bookmark starting from the specified page"
},
{
        "SYNC",             EDBGSync,
        "SYNC ...                           - Synchronization Library Debugger Extensions"
},
{
        "TABLEFIND",        EDBGTableFind,
        "TABLEFIND &lt;szTable&gt; [&lt;instance&gt;]   - Finds the FCB for the specified table"
},
{
        "TEST",             EDBGTest,
        "<link cmd=\"!ese test\">TEST</link>                               - Test function"
},
{
        "THREADERR",        EDBGThreadErr,
        "<link cmd=\"!ese threaderr\">THREADERR</link> [&lt;tid&gt;|<link cmd=\"!ese threaderr *\">*</link>]                - Print the thread state for a given TID (default assumes current debugger's TID)"
},
{
        "TID2PIB",          EDBGTid2PIB,
        "<link cmd=\"!ese tid2pib\">TID2PIB</link> [&lt;tid&gt;]                    - Find the PIB for a given TID (default assumes current debugger's TID)"
},
{
        "TID2TLS",          EDBGTid2TLS,
        "<link cmd=\"!ese tid2tls\">TID2TLS</link> [&lt;tid&gt;]                    - Find the TLS(_TLS,OSTLS,TLS) for a given TID (default assumes current debugger's TID)"
},
{
        "UNLOAD",           EDBGUnload,
        "<link cmd=\"!ese unload\">UNLOAD</link>                             - Unload the DLL"
},
{
        "VERSION",          EDBGVersion,
        "<link cmd=\"!ese version\">VERSION</link>                            - Display version info for the DLL"
},
{
        "VERSTORE",         EDBGVerStore,
        "VERSTORE &lt;instance|.&gt; [&lt;filters&gt;] [/u] [/d]\n\t"
        "                                   - Dump version store usage information for the specified instance"
},
{
        "VERHASHSUM",       EDBGVerHashSum,
        "VERHASHSUM &lt;instance&gt; [verbose]    - Dump version store hash stats information for the specified instance"
},
};

const INT cfuncmap = sizeof( rgfuncmap ) / sizeof( EDBGFUNCMAP );


#define DUMPA( _struct )            { #_struct, &(CDUMPA<_struct>::instance), #_struct " <address>" }
#define DUMPAA( _struct, addlargs ) { #_struct, &(CDUMPA<_struct>::instance), #_struct " <address> " addlargs }


//  ================================================================
LOCAL const CDUMPMAP rgcdumpmap[] = {
//  ================================================================

    DUMPA( BACKUP_CONTEXT ),
    DUMPA( BF ),
    DUMPA( CHECKPOINT ),
    DUMPA( RCE ),
    DUMPA( PIB ),
    DUMPA( TrxidStack ),
    DUMPA( FUCB ),
    DUMPA( CSR ),
    DUMPA( REC ),
    DUMPA( FCB ),
    DUMPA( IDB ),
    DUMPA( TDB ),
    DUMPA( INST ),
    DUMPA( FMP ),
    DUMPA( LOG ),
    DUMPA( LOG_BUFFER ),
    DUMPA( LOG_STREAM ),
    DUMPA( LOG_WRITE_BUFFER ),
    DUMPA( VER ),
    DUMPAA( MEMPOOL, "[<itag>|*]              - <itag>=specified tag only, *=all tags" ),
    DUMPA( SPLIT ),
    DUMPA( SPLITPATH ),
    DUMPA( MERGE ),
    DUMPA( MERGEPATH ),
    DUMPA( DBFILEHDR ),
    { "CDynamicHashTable", &(CDUMPA<CDynamicHashTableEDBG>::instance), "CDynamicHashTable <address>" },
    { "CApproximateIndex", &(CDUMPA<CApproximateIndexEDBG>::instance), "CApproximateIndex <address>" },
    { "g_bflruk", &(CDUMPA<CLRUKResourceUtilityManagerEDBG>::instance), "g_bflruk ese!g_bflruk" },
    DUMPA( COSDisk ),
    DUMPA( COSFile ),
    DUMPA( COSFileFind ),
    DUMPA( COSFileSystem ),
    DUMPAA( IOREQ, "[dumpall|norunstats]" ),
    { "PAGE", &(CDUMPA<CPAGE>::instance), 
         "PAGE <pgno> <address|.> [a|b|h|t|*|2|4|8|16|32]   - a=alloc map, b=binary dump, h=header, t=tags, *=all, 2/4/8/16/32=pagesize" },
    DUMPA( CResource ),
    DUMPA( CResourceManager ),
};

const INT ccdumpmap = sizeof( rgcdumpmap ) / sizeof( CDUMPMAP );



//  ****************************************************************
//  FUNCTIONS
//  ****************************************************************


// strstr() is case sensitive, wanted case insensitive, couldn't find it, wrote it.  stristr() is case insensitive.
//  ================================================================
#define stristr( szString, szSearchStr )    strhstr( szString, szSearchStr, fFalse )
//  ================================================================

// Then I changed stristr() to have heuristic case sensitivity...
//  Anything the user asks us to match (in szSearchStr) that is capital we match 
//  exactly, anything lower case (in szSearchStr) we match either case.  This works
//  much better for strings like "IO" vs. "io" and "Rec" vs. "rec".  Is this just 
//  going to confuse people?
//  ================================================================
const char * const strhstr(
    __in_z const char * const szString,
    __in_z const char * const szSearchStr,
    BOOL fHeuristicCaseSensitivity )
//  ================================================================
{
    ULONG i = 0;
    ULONG j = 0;

    for(i = 0; szString[i]; i++)
    {
        for(j = 0;
            szString[i+j] && szSearchStr[j] &&
                ( ( fHeuristicCaseSensitivity && isupper( szSearchStr[j] ) ) ?
                    ( szString[i+j] == szSearchStr[j] ) :
                    ( __ascii_toupper( szString[i+j] ) == __ascii_toupper( szSearchStr[j] ) )
                )
                ;
            j++)
        {
            ; // do nothing
        }
        if ( szSearchStr[j] == '\0' )
        {
            // we got to the end of the search string, we have a match!!
            return ( &(szString[i]) );
        }
    }
    return NULL;
}

//  ================================================================
inline VOID __cdecl CPRINTFWDBG::operator()( const char * szFormat, ... )
//  ================================================================
{
    va_list arg_ptr;
    va_start( arg_ptr, szFormat );
    // Don't output DML just yet. One of the problems is that the basic DUMP commands
    // have formats that look like this:
    // m_rgfmp <0x1033AEF4,  4>:  0x00000000
    // and the angle brackets <> need to be escaped as &lt; and &gt;
    (void) g_DebugControl->ControlledOutputVaList(
        DEBUG_OUTCTL_AMBIENT_TEXT, DEBUG_OUTPUT_NORMAL, szFormat, arg_ptr);
    va_end( arg_ptr );
}

//  ================================================================
CPRINTF * CPRINTFWDBG::PcprintfInstance()
//  ================================================================
{
    static CPRINTFWDBG s_CPrintfWdbg;
    return &s_CPrintfWdbg;
}

//  ================================================================
LOCAL INT SzToRgsz( __out_ecount( cszMax ) CHAR * rgsz[], __in_z CHAR * const sz, const INT cszMax )
//  ================================================================
{
    INT irgsz = 0;
    CHAR * szT = sz;
    CHAR * szNextToken = NULL;
    
    while( NULL != ( rgsz[irgsz] = strtok_s( szT, " \t\n", &szNextToken ) ) )
    {
        ++irgsz;
        if ( irgsz >= cszMax )
        {
            return -1;
        }
        szT = NULL;
    }
    return irgsz;
}


//  ================================================================
LOCAL BOOL FArgumentMatch( const CHAR * const sz, const CHAR * const szCommand )
//  ================================================================
{
    const BOOL fMatch = ( ( strlen( sz ) == strlen( szCommand ) )
            && !( _strnicmp( sz, szCommand, strlen( szCommand ) ) ) );
    return fMatch;
}

namespace OSSYM {

//  ================================================================
LOCAL BOOL FUlFromSz( const char* const sz, ULONG* const pul, const INT base = 16 )
//  ================================================================
{
    if( sz && *sz )
    {
        char* pchEnd;
        *pul = strtoul( sz, &pchEnd, base );
        return *pchEnd == '\0';
    }
    return fFalse;
}

//  ================================================================
template< class T >
LOCAL BOOL FAddressFromSz( const char* const sz, T** const ppt )
//  ================================================================
{
    BOOL f = fFalse;

    if ( sz && *sz )
    {
        if ( sz[0] == '.' && sz[1] == '\0' )
        {
            // GetExpression() turns . into a pointer (like 00007ffd88030bb2), but not sure what it relates to.
            return fFalse;
        }

        *ppt = (T*) GetExpression( sz );
        if ( *ppt != 0 )
        {
            f = fTrue;
        }
    }
    return f;
}

template< class T >
BOOL FHintAddressFromGlobal( const char* const szGlobal, T** const ppt );

template< class T >
BOOL FAddressFromGlobal( const char* const szGlobal, T** const ppt );

//  ================================================================
template< class T >
LOCAL BOOL FSymbolFromAddress( T* const pt, __out_bcount(cbMax) PSTR szGlobal, const SIZE_T cbMax, DWORD_PTR* const pdwOffset = NULL )
//  ================================================================
{
    ULONG64 ulAddress = (ULONG64) pt;
    DWORD64 dwOffset;
    DWORD   cbActual;

    AssertEDBGDebugger();

    HRESULT hr;
    hr = g_DebugSymbols->GetNameByOffset(
        ulAddress,
        szGlobal,
        (ULONG) cbMax,
        &cbActual,
        &dwOffset
        );

    if ( pdwOffset )
    {
        *pdwOffset = (DWORD_PTR) dwOffset;
    }

    return SUCCEEDED( hr );
}

//  ================================================================
template< class T >
LOCAL BOOL FAddressFromGlobal_( const char* const szGlobal, T** const ppt )
//  ================================================================
{
    AssertEDBGDebugger();

    if ( FHintAddressFromGlobal( szGlobal, ppt ) )
    {
        return fTrue;
    }

    HRESULT hr = S_OK;
    ULONG EndIdx;
    DEBUG_VALUE FullValue;
    ULONG64 Address = 0;

    hr = g_DebugControl->Evaluate( szGlobal, DEBUG_VALUE_INT64, &FullValue, &EndIdx );

    if ( SUCCEEDED( hr ) && ppt != NULL )
    {
        Address = FullValue.I64;

        //  return the address of the symbol
        *ppt = (T*)(DWORD_PTR)Address;  //  HACK: cast to DWORD_PTR then to T* in order to permit compiling with /Wp64
    }

    return SUCCEEDED( hr );
}

char g_szNormalizedGlobalPrefix[ MAX_SYMBOL ] = { '\0' };
//  WARNING: this function is not thread safe!
//
//  ================================================================
template< class T >
LOCAL_BROKEN BOOL FAddressFromGlobal( const char* const szGlobal, T** const ppt )
//  ================================================================
{
    AssertEDBGDebugger();

    //  have we explicitly supplied a module name?
    //
    if ( strchr( szGlobal, '!' ) != NULL )
    {
        return FAddressFromGlobal_( szGlobal, ppt );
    }

    const size_t cbNormalizedPrefix = sizeof( g_szNormalizedGlobalPrefix );
    char szNormalizedGlobal[ MAX_SYMBOL ] = { '\0' };
    const size_t cchNormalizedGlobal = _countof( szNormalizedGlobal );
    BOOL fSucceeded = fFalse;
    BOOL fRetry;

    do
    {
        fRetry = fFalse;

        //  have we done this before?
        //
        if ( g_szNormalizedGlobalPrefix[ 0 ] != '\0' )
        {
            //  at least 3 characters (prefix, '!' and suffix).
            //
            if ( sprintf_s( szNormalizedGlobal, cchNormalizedGlobal, "%s!%s", g_szNormalizedGlobalPrefix, szGlobal ) >= 3 )
            {
                fSucceeded = FAddressFromGlobal_( szNormalizedGlobal, ppt );

                //  if we failed here, we'll uninitialize the prefix and try again.
                //
                if ( !fSucceeded )
                {
                    g_szNormalizedGlobalPrefix[ 0 ] = '\0';
                    fRetry = fTrue;
                }
            }
            else
            {
                g_szNormalizedGlobalPrefix[ 0 ] = '\0';
                fSucceeded = FAddressFromGlobal_( szGlobal, ppt );
            }
        }
        else
        {
            //  perform symbol lookup to try and obtain a module name.
            //
            fSucceeded = FAddressFromGlobal_( szGlobal, ppt );

            //  use that module name for future symbol lookups.
            //
            if ( fSucceeded && FSymbolFromAddress( *ppt, g_szNormalizedGlobalPrefix, cbNormalizedPrefix ) )
            {
                //  now, validate the module name.
                //
                char* const pchSuffixEnd = strchr( g_szNormalizedGlobalPrefix, '!' );
                if ( pchSuffixEnd != NULL )
                {
                    *pchSuffixEnd = '\0';
                    dprintf( "Global symbols will be prefixed with \"%s!\" by default.\n", g_szNormalizedGlobalPrefix );
                }
            }
        }
    } while ( fRetry );

    return fSucceeded;
}

//  ================================================================
template< class T >
INLINE BOOL FReadVariable( T* const rgtDebuggee, T* const rgt, const SIZE_T ct = 1 )
//  ================================================================
{
    AssertEDBGDebugger();

    // debugger extensions only, not real danger but no regression danger either
    //
    // check to see if it fits in a SIZE_T
    if ( ( ~( SIZE_T( 0 ) ) / sizeof( T ) ) <= ct )
    {
        dprintf( "FReadVariable failed with data overflow\n" );
        return fFalse;
    }

    // it won't overflow a SIZE_T but we need a DWORD
    //
    if ( SIZE_T( sizeof( T ) * ct ) >= SIZE_T( ~( DWORD( 0 ) ) ) )
    {
        dprintf( "FReadVariable failed with data too big\n" );
        return fFalse;
    }

    ULONG cbRead;
    return FEDBGMemoryRead(
                (ULONG64)rgtDebuggee,
                (VOID *)rgt,
                DWORD( sizeof( T ) * ct ),
                &cbRead );
}

//  ================================================================
template< class T >
INLINE BOOL FReadVariable(const T* const rgtDebuggee, T* const rgt, const SIZE_T ct = 1 )
//  ================================================================
{
    return FReadVariable( (T* const)rgtDebuggee, rgt, ct );
}

//  ================================================================
template< class T >
LOCAL BOOL FFetchVariable( T* const rgtDebuggee, T** const prgt, SIZE_T ct = 1 )
//  ================================================================
{
    AssertEDBGDebugger();

    // debugger extensions only, not real danger but no regression danger either
    //
    // check to see if it fits in a SIZE_T
    if ( ( ~( SIZE_T( 0 ) ) / sizeof( T ) ) <= ct )
    {
        dprintf( "FFetchVariable failed with data overflow\n" );
        return fFalse;
    }

    //  allocate enough storage to retrieve the requested type array

    if ( !( *prgt = (T*)LocalAlloc( 0, sizeof( T ) * ct ) ) )
    {
        return fFalse;
    }

    //  retrieve the requested type array

    if ( !FReadVariable( rgtDebuggee, *prgt, ct ) )
    {
        LocalFree( (VOID *)*prgt );
        *prgt = NULL;
        return fFalse;
    }

    return fTrue;
}

//  ================================================================
template< class T >
LOCAL BOOL FFetchAlignedVariable( __in T* const rgtDebuggee, __deref_out_ecount( ct ) T** const prgt, SIZE_T ct = 1 )
//  ================================================================
{
    AssertEDBGDebugger();

    // debugger extensions only, not real danger but no regression danger either
    //
    // check to see if it fits in a SIZE_T
    if ( ( ~( SIZE_T( 0 ) ) / sizeof( T ) ) <= ct )
    {
        dprintf( "FFetchVariable failed with data overflow\n" );
        return fFalse;
    }

    //  allocate enough storage to retrieve the requested type array

    if ( !( *prgt = (T*)VirtualAlloc( 0, sizeof( T ) * ct, MEM_COMMIT, PAGE_READWRITE ) ) )
    {
        return fFalse;
    }

    //  retrieve the requested type array

    if ( !FReadVariable( rgtDebuggee, *prgt, ct ) )
    {
        VirtualFree( (VOID *)*prgt, 0, MEM_RELEASE);
        *prgt = NULL;
        return fFalse;
    }

    return fTrue;
}

//  ================================================================
template< class T >
LOCAL BOOL FReadGlobal( const CHAR * const szGlobal, T* const rgt, const SIZE_T ct = 1 )
//  ================================================================
{
    AssertEDBGDebugger();

    //  get the address of the global in the debuggee and fetch it

    T*  rgtDebuggee;

    if ( FAddressFromGlobal( szGlobal, &rgtDebuggee )
        && FReadVariable( rgtDebuggee, rgt, ct ) )
    {
        return fTrue;
    }
    else
    {
        dprintf( "Error: Could not read global variable '%s'.\n", szGlobal );
        return fFalse;
    }
}

//  ================================================================
template< class T >
LOCAL BOOL FReadGlobalAndFetchVariable( const CHAR * const szGlobal, T** const prgt, const SIZE_T ct = 1 )
//  ================================================================
{
    AssertEDBGDebugger();

    //  get the address of the global in the debuggee and fetch its contents

    T*  rgtDebuggee;

    if ( FReadGlobal( szGlobal, &rgtDebuggee ) )
    {
        if ( FFetchVariable( rgtDebuggee, prgt, ct ) )
            return fTrue;
        else
            dprintf( "Error: Could not fetch global variable '%s'.\n", szGlobal );
    }

    return fFalse;
}

//  ================================================================
template< class T >
LOCAL BOOL FFetchGlobal( const CHAR * const szGlobal, T** const prgt, SIZE_T ct = 1 )
//  ================================================================
{
    AssertEDBGDebugger();

    //  get the address of the global in the debuggee and fetch it

    T*  rgtDebuggee;

    if ( FAddressFromGlobal( szGlobal, &rgtDebuggee )
        && FFetchVariable( rgtDebuggee, prgt, ct ) )
    {
        return fTrue;
    }
    else
    {
        dprintf( "Error: Could not fetch global variable '%s'.\n", szGlobal );
        return fFalse;
    }
}

//  ================================================================
template< class T >
LOCAL BOOL FFetchSz( __in T* const szDebuggee, __deref_out_z T** const psz )
//  ================================================================
{
    //  scan for the null terminator in the debuggee starting at the given
    //  address to get the size of the string

    const SIZE_T    ctScan              = 256;
    const SIZE_T    cbScan              = ctScan * sizeof( T );
    BYTE            rgbScan[ cbScan ];
    T*              rgtScan             = (T*)rgbScan;  //  because T can be const
    SIZE_T          itScan              = ~( SIZE_T( 0 ) ) ;
    SIZE_T          itScanLim           = 0;

    AssertEDBGDebugger();

    do  {
        if ( !( ++itScan % ctScan ) )
        {
            ULONG   cbRead;
            FEDBGMemoryRead(
                                ULONG64( szDebuggee + itScan ),
                                (void*)rgbScan,
                                cbScan,
                                &cbRead );

            itScanLim = itScan + cbRead / sizeof( T );
        }
    }
    while ( itScan < itScanLim && rgtScan[ itScan % ctScan ] );

    //  we found a null terminator

    if ( itScan < itScanLim )
    {
        //  fetch the string using the determined string length

        return FFetchVariable( szDebuggee, psz, itScan + 1 );
    }

    //  we did not find a null terminator

    else
    {
        //  fail the operation

        return fFalse;
    }
}

//  ================================================================
template< class T >
LOCAL void Unfetch( T* const rgt )
//  ================================================================
{
    LocalFree( (void*)rgt );
}

//  ================================================================
template< class T >
LOCAL void UnfetchAligned( T* const rgt )
//  ================================================================
{
    VirtualFree( (void*)rgt, 0, MEM_RELEASE );
}

//  ================================================================

//  These are declared high, so that PageSizeClean.hxx can be included
//  as high as possible.

size_t CbEDBGILoadSYSMaxPageSize_()
{
    size_t      cbRet       = 0;
    size_t      cparam      = NULL;
    CJetParam*  rgparam     = NULL;

    if ( !FFetchGlobalParamsArray( &rgparam, &cparam ) )
    {
        //  Well we tried.
        dprintf( "JET_paramDatabasePageSize = [Error: cannot determine the global page size MAX of debuggee.]\n" );
    }
    else
    {
        cbRet = (ULONG)rgparam[ JET_paramDatabasePageSize ].Value();
    }
    Unfetch( rgparam );

    return cbRet;
}

size_t CbPageOfInst_( INST * pinstDebuggee, INST * pinst )
{
    size_t        cparam        = 9;
    CJetParam *   pjpDebuggee   = NULL;
    CJetParam *   rgparam       = NULL;

    pjpDebuggee = pinst->m_rgparam;

    if ( FReadGlobal( "g_cparam", &cparam ) &&
         pjpDebuggee &&
         FFetchVariable( pjpDebuggee, &rgparam, cparam ) )
    {
        size_t cbPageInstMax = rgparam[ JET_paramDatabasePageSize ].Value();
        Unfetch( rgparam );
        return cbPageInstMax;
    }
    dprintf( "Error fetching m_rgparam 0x%N from pinst 0x%N\n", pjpDebuggee, pinstDebuggee );
    return 0;
}

void EDBGDeprecatedSetGlobalPageSize( _In_ const ULONG cbPage )
{
    CallS( SetParam( pinstNil, ppibNil, JET_paramDatabasePageSize, cbPage, NULL ) );
}

#include "..\ese\PageSizeClean.hxx"

//  ================================================================

template <class T>
class FetchWrap
{
    private:
        T m_t;
        FetchWrap &operator=( FetchWrap const & ); // forbidden

    public:
        FetchWrap() { m_t = NULL; }
        ~FetchWrap() { Unfetch(); }
        BOOL FVariable( T const rgtDebuggee, SIZE_T ct = 1 ) { Unfetch(); return FFetchVariable( rgtDebuggee, &m_t, ct ); }
        BOOL FGlobal( const char * const szGlobal, SIZE_T ct = 1 ) { Unfetch(); return FFetchGlobal( szGlobal, &m_t, ct ); }
        BOOL FSz( T const szDebuggee ) { Unfetch(); return FFetchSz( szDebuggee, &m_t ); }
        VOID Unfetch() { OSSYM::Unfetch( m_t ); }
        T Release() { T t = m_t; m_t = NULL; return t; }    //      dereference the pointer, so the user will take care to Unfetch

        operator T() { return m_t; }
        T operator->() { return m_t; }
};

//  ================================================================
template< class T >
LOCAL_BROKEN BOOL FHintAddressFromGlobal(
    const char* const szGlobal,
    T** const ppt )
//  ================================================================
{
    FetchWrap<EDBGGlobals *>    pEG;
    FetchWrap<SIZE_T *>         pcArraySize;
    FetchWrap<const CHAR *>     szName;

    AssertEDBGDebugger();

    //  if we can fetch the debuggee table globals

    if ( NULL != g_rgEDBGGlobalsDebugger
        && pEG.FVariable( g_rgEDBGGlobalsDebugger )
        && pcArraySize.FVariable( (SIZE_T *)pEG->pvData )
        && szName.FSz( pEG->szName )
        && 0 == strcmp( szName, "cEDBGGlobals" )
        && pEG.FVariable( g_rgEDBGGlobalsDebugger, *pcArraySize ) )
    {
        // For the manual string searching, we don't want the module
        // qualification (e.g. "ese!" or "esent!").
        const char* szGlobalUnqualified = strchr( szGlobal, '!' );

        if ( szGlobalUnqualified != NULL )
        {
            // If there was a "!", then advance beyond that character.
            ++szGlobalUnqualified;
        }
        else
        {
            // It was an unqualified symbol to begin with!
            szGlobalUnqualified = szGlobal;
        }

        //  search in the table for the particular global name
        SIZE_T i = 0;
        for ( i = 1; i < *pcArraySize; i++ )
        {
            if ( szName.FSz( pEG[i].szName )
                && 0 == _stricmp( szName, szGlobalUnqualified ) )
            {
                *ppt = (T*)pEG[i].pvData;
                return fTrue;
            }
        }

        dprintf( "Warning: Symbol %hs wasn't found in rgEDBGGlobals table. This command will only work with private symbols.\n",
                 szGlobalUnqualified );
    }

    return fFalse;
}

//  ================================================================
LOCAL BOOL FEDBGDebuggerExecute( PCSTR szCommand )
//  ================================================================
{
    return S_OK == g_DebugControl->Execute( DEBUG_OUTCTL_ALL_CLIENTS, szCommand, DEBUG_EXECUTE_ECHO | DEBUG_EXECUTE_NO_REPEAT );
}

}; // namespace OSSYM


#define FCall( x, szError ) { if ( !( x ) ) { dprintf szError; goto HandleError; } }
#define FCallR( x, szError ) { if ( !( x ) ) { dprintf szError; return; } }
#define CallDbg( fn )       \
    hr = fn;            \
    if ( FAILED( hr ) ) \
    {           \
        g_DebugControl->Output( DEBUG_OUTPUT_ERROR, "%s(): Failed on " #fn " %#x [line %d]\n", __FUNCTION__, hr,  __LINE__ ); \
        goto Cleanup;   \
    }               \


using namespace OSSYM;

// ====================================================================================
//
//  ESE Debugger Local Store
//
// ====================================================================================

class DEBUGGER_LOCAL_STORE
    : CZeroInit
{

private:
    // as I understand it debugger ext executes single threaded, but check anyway
    DWORD       tid;

    BOOL        m_fLocalProcess;

public:
    DEBUGGER_LOCAL_STORE() :
        CZeroInit( sizeof(DEBUGGER_LOCAL_STORE) )
    {
    }
    ~DEBUGGER_LOCAL_STORE()
    {
    }

    DWORD DebugTID( )
    {
        // anything else to assert here? not for now.
        return tid;
    }

    BOOL FLocalProcess() const
    {
        return m_fLocalProcess;
    }

    ERR ErrDLSValid()
    {
        return tid == GetCurrentThreadId() ? JET_errSuccess : ErrEDBGCheck( JET_errSessionContextNotSetByThisThread );
    }

public:

    //  Init / Term functions ...
    static ERR ErrDlsInit( const BOOL fLocalProcess  );
    static void DlsDestroy( void );


    //
    //  Debugger Thead Support
    //

public:
    DWORD TidCurrent() const
    {
        DWORD dwTid = 0;
        HRESULT hr = g_DebugSystemObjects->GetCurrentThreadSystemId( &dwTid );
        if ( FAILED( hr ) )
        {
            dprintf( "Could not read the current TID\n" );
        }
        return dwTid;
    }


    //
    //  Debugger Local Alloc state
    //

private:
    typedef struct _tagPVDLSALLOC {
        struct _tagPVDLSALLOC *     pvPrev;
        struct _tagPVDLSALLOC *     pvNext;
        BYTE                        rgbData[];
} PVDLSALLOC;

    C_ASSERT( 2 * sizeof(void*) == sizeof(PVDLSALLOC) );

    PVDLSALLOC *            m_pvAllocsHead;

public:

    //  Initializes the debugger local state for debugger local allocs

    ERR ErrDLSInit( void )
    {
        m_pvAllocsHead = NULL;
        return JET_errSuccess;
    }

    //  This is an allocator that has all it's memory freed at the end of the debugger
    //  command.  This is a method of convience, kind of like GC but not really, so care
    //  must be used to ensure that the leak during the command does not get too big.
    #define PvDLSAlloc( cb )    PvDLSAlloc_( cb, __LINE__ )
    void * PvDLSAlloc_( ULONG cb, ULONG ulLine )
    {
        PVDLSALLOC * pvdlsAlloc = (PVDLSALLOC *) malloc( cb + sizeof(PVDLSALLOC) );
        if ( NULL == pvdlsAlloc )
        {
            return NULL;
        }
        pvdlsAlloc->pvNext = m_pvAllocsHead;
        pvdlsAlloc->pvPrev = NULL;
        if ( pvdlsAlloc->pvNext )
        {
            pvdlsAlloc->pvNext->pvPrev = pvdlsAlloc;
        }
        m_pvAllocsHead = pvdlsAlloc;

        //debug: dprintf( "Alloc: %p / %p @ %d", pvdlsAlloc, pvdlsAlloc->rgbData, ulLine );
        return pvdlsAlloc->rgbData;
    }

    //  DFree will free the pointer only if in the current chunk.  Note the free is
    //  more and more inefficient if there are intermediate allocs.  Optimal if the
    //  free are in inverse order from the allocs.
    void DLSFree( void * pv )
    {
        if ( pv )
        {
            PVDLSALLOC * pvdlsAlloc = (PVDLSALLOC *) ( (BYTE*)pv - sizeof(PVDLSALLOC) );
            if ( pvdlsAlloc->pvPrev )
            {
                pvdlsAlloc->pvPrev->pvNext = pvdlsAlloc->pvNext;
            }
            else
            {
                AssertEDBG( m_pvAllocsHead == pvdlsAlloc );
                m_pvAllocsHead = pvdlsAlloc->pvNext;
            }
            if ( pvdlsAlloc->pvNext )
            {
                pvdlsAlloc->pvNext->pvPrev = pvdlsAlloc->pvPrev;
            }
            //debug: dprintf( "Free: %p / %p", pvdlsAlloc, pvdlsAlloc->rgbData );
            free( pvdlsAlloc );
        }
    }

    void FreeDebuggerLocalState( void )
    {
        ULONG cTotal = 0;

        while ( m_pvAllocsHead )
        {
            DLSFree( m_pvAllocsHead->rgbData );
            cTotal++;
        }

        // !ese cachesum verbose is up to 411 outstanding allocations!  It's ok as
        // long as there is no alloc per buffer, which I verified on a datacenter
        // dump.
        if ( cTotal > 500 )
        {
            // if you get this warning, the only really important thing is that 
            // the # of allocations isn't O(n) with a large number of objects that
            // you are processing ... then it limites the scalability of your debugger
            // command.
            dprintf( "Warning: This debugger command has %d left over allocations outstanding, this may be inefficient.\n", cTotal );
        }
        AssertEDBG( NULL == m_pvAllocsHead );
    }

    //
    //  Deferred Warning Facility
    //

private:
    ULONG       m_cWarningNotes;
    ULONG *     m_rgcWarningNotes;
    CHAR **     m_pszWarningNotes;

public:

    void AddWarning( __in_z const CHAR * szWarningMessage )
    {
        CHAR ** pszNewNotes = NULL;
        ULONG * rgcNewNotes = NULL;
        CHAR * szNewNote = NULL;

        AssertEDBG( !m_fLocalProcess );

        for( ULONG iWarning = 0; iWarning < m_cWarningNotes; iWarning++ )
        {
            if ( 0 == strcmp( m_pszWarningNotes[iWarning], szWarningMessage ) )
            {
                m_rgcWarningNotes[iWarning]++;
                if ( fDebugMode )
                {
                    dprintf("Supressing duplicate AddWarning() messages.\n");
                }
                return;
            }
        }

        pszNewNotes = (CHAR**)malloc( ( m_cWarningNotes + 1 ) * sizeof(CHAR*) );
        rgcNewNotes = (ULONG*)malloc( ( m_cWarningNotes + 1 ) * sizeof(ULONG) );
        szNewNote = (CHAR*)malloc( strlen( szWarningMessage ) + 1 );
        if ( NULL == pszNewNotes || NULL == rgcNewNotes || NULL == szNewNote )
        {
            if ( pszNewNotes )
            {
                free( pszNewNotes );
            }
            if ( rgcNewNotes )
            {
                free( rgcNewNotes );
            }
            dprintf( "Out of memory, couldn't defer print warning note: %s\n", szWarningMessage );
            return;
        }

        if ( m_pszWarningNotes )
        {
            AssertEDBG( m_cWarningNotes );
            AssertEDBG( m_rgcWarningNotes );
            memcpy( pszNewNotes, m_pszWarningNotes, m_cWarningNotes * sizeof(CHAR*) );
            memcpy( rgcNewNotes, m_rgcWarningNotes, m_cWarningNotes * sizeof(ULONG) );
            // must NOT fail after this point ...
            free( m_pszWarningNotes );
            free( m_rgcWarningNotes );
        }
        else
        {
            AssertEDBG( 0 == m_cWarningNotes );
        }

        OSStrCbCopyA( szNewNote, strlen( szWarningMessage ) + 1, szWarningMessage );

        m_pszWarningNotes = pszNewNotes;
        m_rgcWarningNotes = rgcNewNotes;
        m_pszWarningNotes[m_cWarningNotes] = szNewNote;
        m_rgcWarningNotes[m_cWarningNotes] = 1;
        m_cWarningNotes++;
    }

    void CompleteWarnings( void )
    {
        if ( m_cWarningNotes )
        {
            // if there were warnings, give an extra line return
            dprintf("\n");
        }
        for( ULONG iWarning = 0; iWarning < m_cWarningNotes; iWarning++ )
        {
            dprintf( "[%dx]: %s", m_rgcWarningNotes[iWarning], m_pszWarningNotes[iWarning] );
            free( m_pszWarningNotes[iWarning] );
        }
        if ( m_cWarningNotes )
        {
            // if there were warnings, give an extra line return
            dprintf("\n");
        }

        free( m_pszWarningNotes );
        free( m_rgcWarningNotes );
        m_pszWarningNotes = NULL;
        m_rgcWarningNotes = NULL;
        m_cWarningNotes = 0;
    }


    //
    //  Global stuff
    //

private:
    TICK        m_tickLastGivenDebuggee;

public:

    ERR ErrTIMEDLSInit( void )
    {
        if ( m_fLocalProcess )
        {
            m_tickLastGivenDebuggee = g_tickLastGiven;
        }
        else
        {
            if ( !FReadGlobal( "g_tickLastGiven", &m_tickLastGivenDebuggee ) )
            {
                AssertEDBGSz( fFalse, "Could not retrieve / read g_tickLastGiven" );
                //  should we just try to go on w/o error?
                return ErrERRCheck( errCantRetrieveDebuggeeMemory );
            }
        }
        return JET_errSuccess;
    }

    TICK TickDLSCurrent() const
    {
        return m_tickLastGivenDebuggee;
    }

    //
    //  BF Support / Caches
    //

private:
    size_t      m_cbPageGlobalMax;

    BF **       m_rgpbfChunk;
    LONG_PTR    m_cbfChunk;

    VOID **     m_rgpvChunk;
    LONG_PTR    m_cpgChunk;
    
    BF *        m_prgbfLastChunkDebuggee;
    BF *        m_prgbfLastChunk;

    size_t CbPageGlobalMax_( )
    {
        AssertEDBG( !m_fLocalProcess ); 

        if ( 0 == m_cbPageGlobalMax )
        {
            m_cbPageGlobalMax = CbEDBGILoadSYSMaxPageSize_();
        }
        return m_cbPageGlobalMax;
    }

public:

    //  Used too often, just make it public.
    LONG_PTR    m_cbfCacheAddressable;
    LONG_PTR    m_cbfCacheSize;
    LONG_PTR    m_cbfInit;

    ERR ErrBFDLSInit( void )
    {
        //  for IbfBFICachePbf().
        m_rgpbfChunk            = NULL;
        m_cbfChunk              = 0;
        m_rgpvChunk             = NULL;
        m_cpgChunk              = 0;
        m_cbfCacheAddressable   = 0;
        m_cbfCacheSize  = 0;
        m_cbfInit               = 0;
        m_cbPageGlobalMax       = 0;
        m_prgbfLastChunkDebuggee = NULL;;
        m_prgbfLastChunk = NULL;
        //  page image cache FLatchPageImage() / UnlatchPageImage()
        memset( m_rgpvpvPairs, 0, sizeof( m_rgpvpvPairs ) );
        m_ipvpvNextCache = 0;
        m_pvLastEvictedDebuggee = NULL;
        return JET_errSuccess;
    }

    ERR ErrBFInitCacheMap( BF ** rgpbfChunkT, LONG_PTR cbfChunkT, VOID ** rgpvChunkT, LONG_PTR cpgChunkT )
    {
        //  This version of our ibf/pbf cache init allows manual configuration of parameters.
        AssertEDBG( m_cbfCacheAddressable );
        AssertEDBG( m_cbfCacheSize );
        AssertEDBG( m_cbfInit );
        m_cbfChunk = cbfChunkT;
        m_rgpbfChunk = rgpbfChunkT;
        m_cpgChunk = cpgChunkT;
        m_rgpvChunk = rgpvChunkT;
        return JET_errSuccess;
    }

    
    ERR ErrBFInitCacheMap( void )
    {
        BF **       rgpbfChunkT         = NULL;
        LONG_PTR    cbfChunkT           = 0;

        VOID **     rgpvChunkT          = NULL;
        LONG_PTR    cpgChunkT           = 0;

        if ( m_fLocalProcess )
        {
            extern volatile LONG_PTR        cbfCacheAddressable;    // total all up, for all buffer sizes
            extern volatile LONG_PTR        cbfCacheSize;           // total, not including quiesced buffers (i.e., dehydrated to zero)
            extern LONG_PTR     cbfInit;
            m_cbfCacheAddressable = cbfCacheAddressable;
            m_cbfCacheSize = cbfCacheSize;
            m_cbfInit = cbfInit;

            //  since we're actually running cachequery in the local process we can
            //  just initialize ourselves right off the regular cache variables.
            extern BF**         g_rgpbfChunk;
            extern LONG_PTR     g_cbfChunk;
            extern LONG_PTR     g_cpgChunk;
            extern void**       g_rgpvChunk;
            return ErrBFInitCacheMap( g_rgpbfChunk, g_cbfChunk, g_rgpvChunk, g_cpgChunk );
        }
        else
        {
            if ( !FReadGlobal( "g_cbfChunk", &cbfChunkT )
                || !FReadGlobal( "cbfCacheAddressable", &m_cbfCacheAddressable )
                || !FReadGlobal( "cbfCacheSize", &m_cbfCacheSize )
                || !FReadGlobal( "cbfInit", &m_cbfInit )
                || !FReadGlobalAndFetchVariable( "g_rgpbfChunk", &rgpbfChunkT, cCacheChunkMax )
                || !FReadGlobal( "g_cpgChunk", &cpgChunkT )
                || !FReadGlobalAndFetchVariable( "g_rgpvChunk", &rgpvChunkT, cCacheChunkMax ) )
            {
                dprintf( "DLS_Error: Could not load BF parameters.\n\n" );
                return ErrEDBGCheck( errCantRetrieveDebuggeeMemory );
            }
            
            return ErrBFInitCacheMap( rgpbfChunkT, cbfChunkT, rgpvChunkT, cpgChunkT );
        }
    }

    void BFTermCacheMap( void )
    {
        m_prgbfLastChunkDebuggee = NULL;
        if ( m_prgbfLastChunk )
        {
            LocalFree( m_prgbfLastChunk );
        }
        m_prgbfLastChunk = NULL;
        if ( m_rgpbfChunk )
        {
            Unfetch( m_rgpbfChunk );
        }
        if ( m_rgpvChunk )
        {
            Unfetch( m_rgpvChunk );
        }
        m_rgpbfChunk = NULL;
        m_cbfChunk = 0;
        m_rgpvChunk = NULL;
        m_cpgChunk = 0;
        m_cbfCacheAddressable = 0;
        m_cbfCacheSize = 0;
        m_cbfInit = 0;
    }

    ULONG CCacheChunks()
    {
        AssertEDBG( !m_fLocalProcess );
        AssertEDBG( m_rgpvChunk );
        if ( m_rgpvChunk )
        {
            ULONG cChunk;
            for( cChunk = 0; cChunk < cCacheChunkMax; cChunk++ )
            {
                if ( NULL == m_rgpvChunk[cChunk] )
                {
                    break;
                }
            }
            return cChunk;
        }

        return 0;
    }

    __int64 CbCacheReserve()
    {
        AssertEDBG( !m_fLocalProcess );
        return (__int64)CbPageGlobalMax_() * (__int64)m_cpgChunk * (__int64)CCacheChunks();
    }

    LONG_PTR CbfInit( )
    {
        return m_cbfInit;
    }

    //  compute the address of the target BF
    //
    PBF PbfBFCacheIbf( const IBF ibf )
    {
        AssertEDBG( m_rgpbfChunk ); // return NULL in this case?
        return m_rgpbfChunk[ ibf / m_cbfChunk ] + ibf % m_cbfChunk;
    }

    //  compute the ibf of the target bf
    //
    IBF IbfBFCachePbf( const PBF pbfDebuggee, const BOOL fExact = fTrue )
    {
        PBF pbf = pbfDebuggee; // done for simplicity, and matching of the code below ...

        AssertEDBG( !m_fLocalProcess );

        EDBGPanic( m_rgpbfChunk );
        EDBGPanic( m_cbfChunk );

        //  scan the PBF chunk table looking for a chunk that fits in this range

        LONG_PTR ibfChunk;
        for ( ibfChunk = 0; ibfChunk < cCacheChunkMax; ibfChunk++ )
        {
            //  our PBF is part of this chunk and is aligned properly

            if (    m_rgpbfChunk[ ibfChunk ] &&
                    m_rgpbfChunk[ ibfChunk ] <= pbf &&
                    pbf < m_rgpbfChunk[ ibfChunk ] + m_cbfChunk )
            {
                if ( !fExact ||
                    ( DWORD_PTR( pbf ) - DWORD_PTR( m_rgpbfChunk[ ibfChunk ] ) ) % sizeof( BF ) == 0 )
                {
                    //  compute the IBF for this PBF
                    
                    const IBF ibf = ibfChunk * m_cbfChunk + pbf - m_rgpbfChunk[ ibfChunk ];
                    
                    AssertEDBG( !fExact || PbfBFCacheIbf( ibf ) == pbfDebuggee );

                    return ibf;
                }
            }
        }

        //  our PBF isn't part of any chunk so return nil

        return ibfNil;
    }

    //  compute the address of the target ipg
    //
    void * PvBFCacheIpg( const IPG ipg )
    {
        AssertEDBG( !m_fLocalProcess );
        AssertEDBG( m_rgpvChunk );  // return NULL in this case?
        return (void*) ( (BYTE*)m_rgpvChunk[ ipg / m_cpgChunk ] + ( ipg % m_cpgChunk ) * CbPageGlobalMax_() );
    }

    //  compute the ipg/ibf of the target pv/page/buffer
    //  
    IBF IpgBFCachePv( const void * pvOffset, const BOOL fExact = fTrue )
    {
        AssertEDBG( !m_fLocalProcess );

        EDBGPanic( m_rgpvChunk );
        EDBGPanic( m_cpgChunk );

        IPG ipg = ipgNil;
        for ( LONG_PTR ipgChunk = 0; ipgChunk < cCacheChunkMax; ipgChunk++ )
        {
            if (    m_rgpvChunk[ ipgChunk ] &&
                    m_rgpvChunk[ ipgChunk ] <= pvOffset &&
                    pvOffset < (BYTE*)m_rgpvChunk[ ipgChunk ] + m_cpgChunk * CbPageGlobalMax_() )
            {
                if ( !fExact ||
                    ( ( DWORD_PTR( pvOffset ) - DWORD_PTR( m_rgpvChunk[ ipgChunk ] ) ) % CbPageGlobalMax_() == 0 ) )
                {
                    ipg = ipgChunk * m_cpgChunk + ( (BYTE*)pvOffset - (BYTE*)m_rgpvChunk[ ipgChunk ] ) / CbPageGlobalMax_();

                    AssertEDBG( !fExact || PvBFCacheIpg( ipg ) == pvOffset );
                }
            }
        }

        return ipg;
    }

    //  retrieve the pbfDebuggee / pbf from a cached context if possible
    //
    ERR ErrBFCacheRetrieveNextBF( const IBF ibf, PBF * ppbfDebuggee, PBF * ppbf )
    {
        AssertEDBG( m_rgpbfChunk ); // return NULL in this case?
        AssertEDBG( ppbfDebuggee );
        AssertEDBG( ppbf );

        if ( NULL == m_prgbfLastChunk )
        {
            //  NOTE: This is a bit too aggressive of an allocation! :P  We're going to let it fly, and
            //  see if we survive.  This is 1/128th of RAM from the _target machine_!  So if we were 
            //  debugging a 128 GB RAM dump, we'd allocate 1 GB here (even if we only had 512 MBs of
            //  RAM locally).
            m_prgbfLastChunk = (BF*) LocalAlloc( 0, m_cbfChunk * sizeof(BF) );
            if ( NULL == m_prgbfLastChunk )
            {
                dprintf( "Failed to allocate %d bytes\n", m_cbfChunk * sizeof(BF) );
                return ErrERRCheck( JET_errOutOfMemory );
            }
        }

        *ppbfDebuggee = PbfBFCacheIbf( ibf );

        if ( m_prgbfLastChunkDebuggee &&
                m_prgbfLastChunkDebuggee < *ppbfDebuggee &&
                *ppbfDebuggee < m_prgbfLastChunkDebuggee + m_cbfChunk )
        {
            //  Yeah, cached hit ...

            *ppbf = m_prgbfLastChunk + ibf % m_cbfChunk;

            return JET_errSuccess;
        }

        //  else the m_prgbfLastChunk doesn't have the current chunk, fetch it ...

        if ( m_fLocalProcess )
        {
            LONG_PTR cbf;
            if ( m_cbfInit > ( m_cbfInit / m_cbfChunk * m_cbfChunk ) )
            {
                cbf = m_cbfInit % m_cbfChunk;
            }
            else
            {
                cbf = m_cbfChunk;
            }
            memcpy( m_prgbfLastChunk, m_rgpbfChunk[ ibf / m_cbfChunk ], cbf * sizeof(BF) );
        }
        else
        {
            if ( !FReadVariable( m_rgpbfChunk[ ibf / m_cbfChunk ], m_prgbfLastChunk, m_cbfChunk ) )
            {
                return ErrERRCheck( JET_errOutOfMemory );
            }
        }

        m_prgbfLastChunkDebuggee = m_rgpbfChunk[ ibf / m_cbfChunk ];

        *ppbf = m_prgbfLastChunk + ibf % m_cbfChunk;

        return JET_errSuccess;
    }

private:
    typedef struct
    {
        void *          pvDebuggee;
        void *          pvFetched;
        ULONG           cLatches;   // using the term "latch" loosely ...
        ULONG           ulReserved;
    } MEM_CACHE_PAIR;

    MEM_CACHE_PAIR  m_rgpvpvPairs[3];
    ULONG           m_ipvpvNextCache;           // this tracks where the next evict and cache pointer will go
    void *          m_pvLastEvictedDebuggee;    // debugging variable, not important

    //  "Latches" the requested memory, returns NULL on failure / element
    //  not present in cache.

    void * PvBFGetCached( void * pvDebuggee )
    {
        AssertEDBG( !m_fLocalProcess );
        for( ULONG i = 0; i < _countof( m_rgpvpvPairs ); i++ )
        {
            if ( m_rgpvpvPairs[i].pvDebuggee == pvDebuggee )
            {
                //  "latch" and return the requested memory
                m_rgpvpvPairs[i].cLatches++;
                return m_rgpvpvPairs[i].pvFetched;
            }
        }

        return NULL;
    }

    //  "Unlatches" the previously latched memory.

    void BFUngetCached( void * pvLocal )
    {
        AssertEDBG( !m_fLocalProcess );
        for( ULONG i = 0; i < _countof( m_rgpvpvPairs ); i++ )
        {
            if ( m_rgpvpvPairs[i].pvFetched == pvLocal )
            {
                //  "unlatch" the memory
                m_rgpvpvPairs[i].cLatches--;
                return;
            }
        }
    }

    //  evicts a slot in the cache, making it available, returning the memory
    //  that was allocated there.  No effect if nothing needs evicting.

    void * PvBFEvictTarget( void )
    {
        AssertEDBG( !m_fLocalProcess );
        AssertPREFIX( m_ipvpvNextCache < _countof( m_rgpvpvPairs ) );

        void * pvRet = m_rgpvpvPairs[m_ipvpvNextCache].pvFetched;
        if ( pvRet != NULL )
        {
            //  we're actually evicting something ...
            //
            if ( m_rgpvpvPairs[m_ipvpvNextCache].cLatches != 0 )
            {
                AddWarning( "Our cache of page images is not big enough to hold all latched pages, need to grow.  Most likely very bad things will be happening now.\n" );
            }
            AssertEDBG( m_rgpvpvPairs[m_ipvpvNextCache].cLatches == 0 );    // assert it as well

            //  track what we're evicting for debugging / checking the size of the cache purposes.
            m_pvLastEvictedDebuggee = m_rgpvpvPairs[m_ipvpvNextCache].pvDebuggee;
            AssertEDBG( m_pvLastEvictedDebuggee );

            //  actually evict the target from the cache
            m_rgpvpvPairs[m_ipvpvNextCache].pvDebuggee = NULL;
            m_rgpvpvPairs[m_ipvpvNextCache].pvFetched = NULL;
        }

        //  must return the pvFetched as this memory is externally allocated
        //  and provided to the cache, and so must be externally freed.
        return pvRet;
    }

    //  takes some allocated memory (pvFetched) and caches it under the lookup
    //  key (pvDebuggee), so that later latches can have / utilize this memory.

    VOID SetBFCached( void * pvDebuggee, void * pvFetched )
    {
        AssertEDBG( !m_fLocalProcess );
        AssertEDBG( m_rgpvpvPairs[m_ipvpvNextCache].pvDebuggee == NULL );
        AssertEDBG( m_rgpvpvPairs[m_ipvpvNextCache].pvFetched == NULL );    // or we're leaking memory

        //  check cache is big enough, just a debug check
        if( m_pvLastEvictedDebuggee == pvDebuggee )
        {
            AddWarning( "We re-cached a recently evicted pointer, should we increase the number of cache slots / m_rgpvpvPairs?" );
        }

        //  cache the target
        m_rgpvpvPairs[m_ipvpvNextCache].pvDebuggee = pvDebuggee;
        m_rgpvpvPairs[m_ipvpvNextCache].pvFetched = pvFetched;

        //  move the next evict + cache location "forward" 1
        m_ipvpvNextCache = ( ( m_ipvpvNextCache+1 ) % _countof( m_rgpvpvPairs ) );
    }

public:

    //  Retrieves a page, checking a small lookaside cache first, caching it otherwise
    //
    enum DLSCachePage { ePageHdrOnly = 1 /* rest NYI */ };

    BOOL FLatchPageImage( DLSCachePage ePageStateReq, void * pvPageDebuggee, __out CPAGE::PGHDR ** pppghdr )
    {
        AssertEDBG( !m_fLocalProcess );
        AssertEDBG( ePageStateReq == ePageHdrOnly );
        AssertEDBG( pvPageDebuggee );
        AssertEDBG( pppghdr );

        const void * pvCached = PvBFGetCached( pvPageDebuggee );
        if ( pvCached )
        {
            // debug: AddWarning( "Utilized cached image / cache hit.\n" );
            *pppghdr = (CPAGE::PGHDR *)pvCached;
            return fTrue;
        }

        //debug: AddWarning( "We're getting a new page image / cache fault.\n" );

        void * pvEvict = PvBFEvictTarget();
        if ( pvEvict )
        {
            //debug: AddWarning( "We're evicting a page to make room for a new one.\n" );
            DLSFree( pvEvict );
        }

        CPAGE::PGHDR * ppghdr = (CPAGE::PGHDR*)PvDLSAlloc( ( CbPage() < 16 * 1024 ) ?
                                                                sizeof(CPAGE::PGHDR) :
                                                                sizeof(CPAGE::PGHDR2) );
        if ( ( CbPage() < 16 * 1024 ) ?
                !FReadVariable( (CPAGE::PGHDR*)pvPageDebuggee, ppghdr ) :
                !FReadVariable( (CPAGE::PGHDR2*)pvPageDebuggee, (CPAGE::PGHDR2*)ppghdr ) )
        {
            return fFalse;
        }

        SetBFCached( pvPageDebuggee, (void*)ppghdr );

        *pppghdr = (CPAGE::PGHDR *)PvBFGetCached( pvPageDebuggee );
        AssertEDBG( *pppghdr );
        return fTrue;
    }
    
    void UnlatchPageImage( CPAGE::PGHDR * ppghdr )
    {
        AssertEDBG( !m_fLocalProcess );
        BFUngetCached( ppghdr );
    }


    //
    //  FMP Info / Caches
    //

private:
    bool *      m_rgfIsTempDB;
    FMP *       m_prgfmp;
    ULONG       m_cfmpCommitted;

public:

    ULONG CfmpAllocated() const
    {
        return m_cfmpCommitted;
    }

    ERR ErrFMPDLSInit( void )
    {
        FMP *   rgfmpDebuggee               = NULL;
        ULONG   ifmpMaxDebuggee             = 0;        //  don't use ifmpNil because its type is IFMP
        ULONG   cfmpMacCommittedDebuggee    = 0;

        if ( m_fLocalProcess )
        {
            rgfmpDebuggee = g_rgfmp;
            AssertEDBG( g_ifmpMax < 64 * 1024 );
            ifmpMaxDebuggee = (ULONG)g_ifmpMax;
            cfmpMacCommittedDebuggee = FMP::s_ifmpMacCommitted;
        }
        else
        {
            if ( !FReadGlobal( "g_rgfmp", &rgfmpDebuggee )
                || NULL == rgfmpDebuggee
                || !FReadGlobal( "g_ifmpMax", &ifmpMaxDebuggee )
                || !FReadGlobal( "FMP::s_ifmpMacCommitted", &cfmpMacCommittedDebuggee ) )
            {
                dprintf( "Error: Could not fetch FMP variables.\n" );
                return errCantRetrieveDebuggeeMemory;
            }
        }

        // be better to trip this by FMP::ifmpMacInUse ...
        m_cfmpCommitted = cfmpMacCommittedDebuggee;

        if ( cfmpMacCommittedDebuggee )
        {
            Assert( ifmpMaxDebuggee >= cfmpMacCommittedDebuggee );
            // over allocate to g_ifmpMax, even though FMP::s_ifmpMacCommitted is all that is allocated by process.
            m_prgfmp = (FMP*)PvDLSAlloc( ifmpMaxDebuggee * sizeof(FMP) );
            if ( NULL == m_prgfmp )
            {
                return ErrERRCheck( JET_errOutOfMemory );
            }

            m_rgfIsTempDB = (bool*) PvDLSAlloc( ifmpMaxDebuggee * sizeof(bool) );
            if ( NULL == m_rgfIsTempDB )
            {
                return ErrERRCheck( JET_errOutOfMemory );
            }
        }

        for ( IFMP ifmp = cfmpReserved; ifmp < cfmpMacCommittedDebuggee; ifmp++ )
        {
            FMP *   pfmp                = NULL;

            if ( m_fLocalProcess )
            {
                memcpy( &(m_prgfmp[ifmp]), rgfmpDebuggee + ifmp, sizeof(FMP) );
                m_rgfIsTempDB[ifmp] = ( dbidTemp == (rgfmpDebuggee + ifmp)->Dbid() );
            }
            else
            {
                if ( !FReadVariable( rgfmpDebuggee + ifmp, &(m_prgfmp[ifmp]) ) )
                {
                    AddWarning( "WARNING: Could not retrieve FMP for FMP cache!  Results may be off.\n" );
                }

                if ( FFetchVariable( rgfmpDebuggee + ifmp, &pfmp ) )
                {

                    m_rgfIsTempDB[ifmp] = ( dbidTemp == pfmp->Dbid() );
                }
                else
                {
                    AddWarning( "WARNING: Could not retrieve FMP.\n" );
                    m_rgfIsTempDB[ifmp] = false;
                }

                Unfetch( pfmp );
            }

        }

        return JET_errSuccess;
    }

    bool FIsTempDB( __in const IFMP ifmp )
    {
        AssertEDBG( !m_fLocalProcess );
        if ( ifmp == 0x7fffffff || ifmp == 0xffffffff )
        {
            AddWarning( "ERROR: Asked the temp DB cache about ifmpNil / 0x7fffffff!\n" );
            return fFalse;
        }
        if ( NULL == m_rgfIsTempDB )
        {
            AddWarning( "ERROR: The temp DB cache was not initialized!\n" );
            return fFalse;
        }
        AssertEDBG( m_rgfIsTempDB[ifmp] == ( PfmpCache( ifmp )->Dbid() == dbidTemp ) );
        return m_rgfIsTempDB[ifmp];
    }

    const FMP * PfmpCache( __in const IFMP ifmp )
    {
        AssertEDBG( !m_fLocalProcess );
        AssertEDBG( ifmp >= cfmpReserved );
        AssertEDBG( ifmp < m_cfmpCommitted );
        return &(m_prgfmp[ifmp]);
    }

    //
    //  Implicit state caches ...
    //

private:
    BOOL         m_fPii                          = OnDebugOrRetail( fTrue, fFalse );
    IFMP         m_ifmpCurrentImplicit           = ifmpNil;
    ULONG        m_ipinstCurrentImplicit         = IpinstNil();

    //  Need multiple properties for the implicit BT
    const FCB *  m_pfcbImplicitBtTableDebuggee   = pfcbNil;
    const FCB *  m_pfcbImplicitBtSubTreeDebuggee = pfcbNil;
    OBJID        m_objidImplicitBt               = objidNil;
    PGNO         m_pgnoImplicitBt                = pgnoNull;

public:

    void SetPII( _In_ const BOOL fOn )
    {
        m_fPii = fOn;
    }

    BOOL FPII() const
    {
        return m_fPii;
    }

    void SetImplicitIfmp( __in const IFMP ifmp )
    {
        m_ifmpCurrentImplicit = ifmp;
    }

    IFMP IfmpCurrent() const
    {
        AssertEDBG( !m_fLocalProcess );
        return m_ifmpCurrentImplicit;
    }

    static ULONG IpinstNil()
    {
        return 0x7ffffff1;  //  large, but not -1, or lMax.
    }

    void SetImplicitIpinst( __in const ULONG ipinst )
    {
        m_ipinstCurrentImplicit = ipinst;
    }

    ULONG IpinstCurrent() const
    {
        AssertEDBG( !m_fLocalProcess );
        return m_ipinstCurrentImplicit;
    }

    void SetImplicitBt( _In_ const FCB * const pfcbTableDebuggee, _In_ const FCB * const pfcbSubTreeDebuggee, const OBJID objidFDP, const PGNO pgnoFDP )
    {
        m_pfcbImplicitBtTableDebuggee   = pfcbTableDebuggee;
        m_pfcbImplicitBtSubTreeDebuggee = pfcbSubTreeDebuggee;
        m_objidImplicitBt               = objidFDP;
        m_pgnoImplicitBt                = pgnoFDP;
    }

    const FCB * PfcbCurrentTableDebuggee() const
    {
        AssertEDBG( !m_fLocalProcess );
        return m_pfcbImplicitBtTableDebuggee;
    }

    const FCB * PfcbCurrentSubTreeDebuggee() const
    {
        AssertEDBG( !m_fLocalProcess );
        return m_pfcbImplicitBtSubTreeDebuggee;
    }

    OBJID ObjidCurrentBt() const
    {
        AssertEDBG( !m_fLocalProcess );
        return m_objidImplicitBt;
    }

    PGNO PgnoCurrentBt() const
    {
        AssertEDBG( !m_fLocalProcess );
        return m_pgnoImplicitBt;
    }


    //
    //  INST Info / Caches
    //

private:
    ULONG       m_cinstMax;
    //  All 3 arrays here are tracked by m_cinst.
    INST **     m_rgpinstDebuggee;
    INST *      m_rginst;   //  Note we fold the array down to one level from a INST ** / g_rgpinst
    BOOL *      m_rgfInstLoaded;

public:

    //  Designed to be called so that a specific extension can fail if
    //  this INST cache doesn't load.

    ERR ErrINSTDLSInitCheck( void )
    {
        if ( m_cinstMax != 0 &&
                m_rginst &&
                m_rgpinstDebuggee &&
                m_rgfInstLoaded )
        {
            return JET_errSuccess;
        }
        dprintf( "Error: Some element of the INSTDLS state failed to initialize:\n"
                    "\tm_cinstMax = %d\n\tm_rgpinstDebuggee = %p\n\tm_rginst = %p\n\tm_rgfInstLoaded = %p\n",
                    m_cinstMax, m_rgpinstDebuggee, m_rginst, m_rgfInstLoaded );

        return ErrERRCheck( errNotFound );
    }

    ERR ErrINSTDLSInit( void )
    {
        ERR err = JET_errSuccess;

        m_cinstMax = 0;
        m_rgpinstDebuggee = NULL;
        m_rginst = NULL;
        m_rgfInstLoaded = NULL;

        if ( !FReadGlobal( "g_cpinstMax", &m_cinstMax ) )
        {
            dprintf( "Error: Could not fetch instance table size.\n" );
            return ErrERRCheck( errNotFound );
        }

        Alloc( m_rginst = (INST*)PvDLSAlloc( sizeof(INST) * m_cinstMax ) ); //  probably a fairly large alloc on large inst server systems.
        memset( m_rginst, 0, sizeof(INST) * m_cinstMax );

        Alloc( m_rgfInstLoaded = (BOOL*)PvDLSAlloc( sizeof(BOOL) * m_cinstMax ) );
        memset( m_rgfInstLoaded, 0, sizeof(BOOL) * m_cinstMax );

        INST ** prgpinstDebuggeeT = NULL;
        if ( !FReadGlobal( "g_rgpinst", &prgpinstDebuggeeT ) )
        {
            dprintf( "Error: Could not fetch instance table address.\n" );
            return ErrERRCheck( errNotFound );
        }
        Alloc( m_rgpinstDebuggee = (INST**)PvDLSAlloc( sizeof(INST*) * m_cinstMax ) );
        if ( !FReadVariable( prgpinstDebuggeeT, m_rgpinstDebuggee, m_cinstMax ) )
        {
            dprintf( "Error: Could not fetch INST * table m_rgpinst.\n" );
            m_rgpinstDebuggee = NULL;
            return ErrERRCheck( errNotFound );
        }
        
    HandleError:

        return err;
    }

    ULONG Cinst() const
    {
        return m_cinstMax;
    }

    INST * PinstDebuggee( const SIZE_T ipinst )
    {
        if ( m_rgpinstDebuggee == NULL )
        {
            dprintf( "Error: Could not fetch instance table during ErrINSTDLSInit() ... debug that.\n" );
            return NULL;
        }

        if ( ipinst >= m_cinstMax )
        {
            dprintf( "Error: ipinst specified %d was larger than max ipinst claimed by ese.dll.\n", ipinst, m_cinstMax );
            return NULL;
        }

        return m_rgpinstDebuggee[ipinst];
    }

    INST * Pinst( const SIZE_T ipinst )
    {
        if ( m_rgfInstLoaded == NULL )
        {
            dprintf( "Error: loading or allocating memory during ErrINSTDLSInit() ... debug that.\n" );
            return NULL;
        }

        if ( ipinst >= m_cinstMax )
        {
            dprintf( "Error: asked for ipinst = %d, larger than max possible %d\n", (ULONG)ipinst, (ULONG)m_cinstMax );
            return NULL;
        }

        if ( m_rgfInstLoaded[ipinst] )
        {
            return &( m_rginst[ipinst] );
        }
        //  else it is not loaded yet, try to load it ...
        
        if ( PinstDebuggee( ipinst ) == NULL )
        {
            //  error printed by PinstDebuggee( ipinst )
            return NULL;
        }

        if ( !FReadVariable( PinstDebuggee( ipinst ), &( m_rginst[ipinst] ) ) )
        {
            dprintf( "Error: Reading g_rgpinst[%d] = %N.\n", ipinst, PinstDebuggee( ipinst ) );
            return NULL;
        }

        m_rgfInstLoaded[ipinst] = fTrue;    //  short cut in future.
        return &( m_rginst[ipinst] );
    }

private:

    size_t * m_mpcbifmpPageSize;
    size_t * m_mpcbipinstPageSize;

    size_t CbPageOfIfmp_( const IFMP ifmpCurr )
    {
        AssertEDBG( !m_fLocalProcess ); 

        if ( ifmpCurr != ifmpNil && ifmpCurr < CfmpAllocated() )
        {
            if ( m_mpcbifmpPageSize[ ifmpCurr ] == 0 )
            {
                const FMP * pfmp = PfmpCache( ifmpCurr );
                if ( pfmp )
                {
                    INST * pinstDebuggee = pfmp->Pinst();
                    INST * pinstTarget = NULL;
                    if ( FFetchVariable( pinstDebuggee, &pinstTarget ) )
                    {
                        ULONG ipinstTarget = pinstTarget->m_iInstance - 1;
                        AssertEDBG( Pinst( ipinstTarget ) != NULL &&
                                    ( Pinst( ipinstTarget )->m_iInstance ) == pinstTarget->m_iInstance &&
                                    PinstDebuggee( ipinstTarget ) == pinstDebuggee );
                        if ( Pinst( ipinstTarget ) != NULL &&
                             ( Pinst( ipinstTarget )->m_iInstance ) == pinstTarget->m_iInstance &&
                             PinstDebuggee( ipinstTarget ) == pinstDebuggee )
                        {
                            m_mpcbifmpPageSize[ ifmpCurr ] = CbPageOfInst_( pinstDebuggee, pinstTarget );
                        }
                        Unfetch( pinstTarget );
                        // Note: if cache does not have it, we just leak pinst.
                    }
                }
            }
            if ( m_mpcbifmpPageSize[ ifmpCurr ] != 0 )
            {
                AssertEDBG( m_mpcbifmpPageSize[ ifmpCurr ] <= 32 * 1024 );
                return m_mpcbifmpPageSize[ ifmpCurr ];
            }
        }

        //  Returning 0 is failure ...

        return 0;
    }

public:

    ERR ErrPAGESIZEDLSInit( void )
    {
        JET_ERR err;

        size_t cbAlloc;

        cbAlloc = sizeof(size_t) * m_cfmpCommitted;
        Alloc( m_mpcbifmpPageSize   = (size_t*)PvDLSAlloc( cbAlloc ) );
        memset( m_mpcbifmpPageSize, 0, cbAlloc );

        cbAlloc = sizeof(size_t) * m_cfmpCommitted;
        Alloc( m_mpcbipinstPageSize = (size_t*)PvDLSAlloc( cbAlloc ) );
        memset( m_mpcbipinstPageSize, 0, cbAlloc );
        
    HandleError:

        return err;
    }

    void PAGESIZEDLSTerm( void )
    {
        DLSFree( m_mpcbifmpPageSize );
        DLSFree( m_mpcbipinstPageSize );
        m_mpcbifmpPageSize = NULL;
        m_mpcbipinstPageSize = NULL;
    }

    size_t CbPage( )
    {
        AssertEDBG( !m_fLocalProcess ); 

        const IFMP ifmpCurr = IfmpCurrent();
        if ( ifmpCurr != ifmpNil && ifmpCurr < CfmpAllocated() )
        {
            const size_t cbPage = CbPageOfIfmp_( ifmpCurr );
            if ( cbPage != 0 )
            {
                AssertEDBG( cbPage <= 32 * 1024 );
                return cbPage;
            }
        }

        const size_t ipinstCurr = IpinstCurrent();
        if ( ipinstCurr != DEBUGGER_LOCAL_STORE::IpinstNil() && ipinstCurr < Cinst() )
        {
            if ( m_mpcbipinstPageSize[ ipinstCurr ] == 0 )
            {
                INST * pinstTarget = Pinst( ipinstCurr );
                if ( pinstTarget != NULL )
                {
                    m_mpcbipinstPageSize[ ipinstCurr ] = CbPageOfInst_( PinstDebuggee( ipinstCurr ), pinstTarget );
                }
            }
            if ( m_mpcbipinstPageSize[ ipinstCurr ]  != 0 )
            {
                AssertEDBG( m_mpcbipinstPageSize[ ipinstCurr ] <= 32 * 1024 );
                return m_mpcbipinstPageSize[ ipinstCurr ];
            }
        }

        AddWarning( "WARNING:  No implicit FMP or INST set via .db or .inst, using global page size instead.\n" );

        return CbPageGlobalMax_( );
    }

    size_t CbPage( const IFMP ifmpCurr )
    {
        AssertEDBG( !m_fLocalProcess ); 

        if ( ifmpCurr != ifmpNil && ifmpCurr < CfmpAllocated() )
        {
            const size_t cbPage = CbPageOfIfmp_( ifmpCurr );
            if ( cbPage != 0 )
            {
                AssertEDBG( cbPage <= 32 * 1024 );
                return cbPage;
            }
        }

        if ( ifmpCurr != 0 && ifmpCurr != ifmpNil )
        {
            //  if we couldn't load our page size, print a warning

            AddWarning( "WARNING:  Could not load page size of requested ifmp, falling back to .inst context or even global page size max.\n" );
        }

        size_t cbLastDitch = CbPage();
        AssertEDBG( cbLastDitch <= 32 * 1024 );

        return cbLastDitch;
    }

};

DEBUGGER_LOCAL_STORE g_dls;

DEBUGGER_LOCAL_STORE * Pdls( void )
{
    EDBGPanic( g_dls.ErrDLSValid() >= JET_errSuccess );

    return &g_dls;
}

ERR DEBUGGER_LOCAL_STORE::ErrDlsInit( const BOOL fLocalProcess )
{
    ERR err = JET_errSuccess;

    //  Note, until the debugger ext TID is set, we can't call Pdls() as it 
    //  will throw.

    if ( g_dls.tid )
    {
        dprintf( "DLS: Something has gone wrong, during init debugger ext TID set to %d.", g_dls.tid );
    }
    g_dls.tid = GetCurrentThreadId();

    g_dls.m_fLocalProcess = fLocalProcess;

    Call( g_dls.ErrDLSInit() );

    Call( g_dls.ErrTIMEDLSInit() );
    Call( g_dls.ErrBFDLSInit() );
    Call( g_dls.ErrINSTDLSInit() );
    Call( g_dls.ErrFMPDLSInit() );
    Call( g_dls.ErrPAGESIZEDLSInit() );  //  must be after ErrINSTDLSInit() & ErrFMPDLSInit()
    Call( ErrPKInitCompression( g_cbPageMax, 0, g_cbPageMax ) );

    //  Validate that we succeeded.
    Call( Pdls()->ErrDLSValid() );
    CallS( err );

HandleError:

    return err;
}

void DEBUGGER_LOCAL_STORE::DlsDestroy( void )
{
    PKTermCompression();
    g_dls.PAGESIZEDLSTerm();

    if ( !Pdls() )
    {
        dprintf( "DLS: No debugger local storage at DlsDestroy.  Why?\n" );
    }
    else
    {
        Pdls()->FreeDebuggerLocalState();
    }

    g_dls.tid = 0;
    
    return;
}

#define CbPageOfInst_     CbPageOfInst_NoOneShouldBeUsingInstPageSizeBeyondDLS


BOOL FImplicitArg( const CHAR * const szArg )
{
    return szArg[0] == '.' && szArg[1] == '\0';
}

BOOL FAutoFmp( const CHAR * const szArg )
{
    return FImplicitArg( szArg );
}


template< class T >
INLINE const CHAR * SzType( T** ppt )
{
    #define TYPESZTYPE( t )    \
        else if constexpr( is_same<T, t>::value )                return #t;

    if constexpr( fFalse )  return "FALSE_DOESNT_WORK";
        //  All the rest of else if ( logically'vartype == TypeX' ) return "TypeX";
        TYPESZTYPE( INST )
        TYPESZTYPE( VER )
        TYPESZTYPE( LOG )
        TYPESZTYPE( LOG_BUFFER )
        TYPESZTYPE( LOG_STREAM )
        TYPESZTYPE( LOG_WRITE_BUFFER )
        TYPESZTYPE( CHECKPOINT )
        TYPESZTYPE( COSFile )
        TYPESZTYPE( COSDisk )

        TYPESZTYPE( BF )
        TYPESZTYPE( FCB )
        TYPESZTYPE( SCB )
        TYPESZTYPE( TDB )
        TYPESZTYPE( IDB )
        TYPESZTYPE( FUCB )
        TYPESZTYPE( CSR )
        TYPESZTYPE( FMP )
        TYPESZTYPE( PIB )

    // Do NOT add a default return, since everything is compile time, if you hit an error 
    // that "SzType<XXXX>" does not a return a value, it means you do not have a proper 
    // else if constexpr( for XXXX ) type check case above.
    // return "NO!";
}

COSVolume * PosvEDBGAccessor( const COSFile * const posf )
{
   return posf->m_posv;
}

CHECKPOINT * PcheckpointEDBGAccessor( const LOG * const plog )
{
   return plog->m_pcheckpoint;
}

const LOG_BUFFER * PlogbufferEDBGAddrAccessor( const LOG * const plog )
{
   return &plog->m_LogBuffer;
}

ILogStream * PlogstreamEDBGAccessor( const LOG * const plog )
{
   return plog->m_pLogStream;
}

LOG_WRITE_BUFFER * PlogwritebufferEDBGAccessor( const LOG * const plog )
{
   return plog->m_pLogWriteBuffer;
}

DBFILEHDR * PdbfilehdrEDBGAccessor( const FMP * const pfmp )
{
    return pfmp->m_dbfilehdrLock.m_pdbfilehdr;
}

// Do _NOT_ implement this function!  It is used as a cheap way to make sure an else clause
// won't actually be compiled once all the consexpr-based templating is done.
void * IntentionallyDoesNotLink();

//  This is an enhanced version of FAddressFromSz, that if the arg provided is 'the implicit arg' / i.e. ".", then
//  the function will (if the type is a singleton per DB or per INST) retrieve the default / implicit version of
//  that type and return that address.   If the arg is not ".", it will pass through to FAddressFromSz().
#pragma warning(push)
#pragma warning(disable: 4702)     // The last else clause due to the if constexpr's above it is unreachable in some versions of this function.
template< class T >
BOOL FAutoAddressFromSz( _In_ const char* const szArg, _Out_ T** const ppt )
{
    if ( FImplicitArg( szArg ) )
    {
        if constexpr( is_same<T, INST>::value )
        {
            if ( Pdls()->IpinstCurrent() == DEBUGGER_LOCAL_STORE::IpinstNil() )
            {
                dprintf( "ERROR: Tried to select implicit INST, but no implicit INST is selected.  Use !ese .inst <InstName|Index> to select.\n" );
                *ppt = (T*)NULL;
                return fFalse;
            }
            *ppt = Pdls()->PinstDebuggee( Pdls()->IpinstCurrent() );
            dprintf( "Retrieving default/implicit INST: 0x%p\n", *ppt );
        }
        else if constexpr( is_same<T, LOG>::value )
        {
            if ( Pdls()->IpinstCurrent() == DEBUGGER_LOCAL_STORE::IpinstNil() )
            {
                dprintf( "ERROR: Tried to select implicit LOG / INST, but no implicit INST is selected.  Use !ese .inst <InstName|Index> to select INST first.\n" );
                *ppt = (T*)NULL;
                return fFalse;
            }
            *ppt = Pdls()->Pinst( Pdls()->IpinstCurrent() )->m_plog;
            dprintf( "Retrieving default/implicit LOG: %#p\n", *ppt );
        }
        else if constexpr( is_same<T, VER>::value )
        {
            if ( Pdls()->IpinstCurrent() == DEBUGGER_LOCAL_STORE::IpinstNil() )
            {
                dprintf( "ERROR: Tried to select implicit LOG / INST, but no implicit INST is selected.  Use !ese .inst <InstName|Index> to select INST first.\n" );
                *ppt = (T*)NULL;
                return fFalse;
            }
            *ppt = Pdls()->Pinst( Pdls()->IpinstCurrent() )->m_pver;
            dprintf( "Retrieving default/implicit VER: %p\n", *ppt );
        }
        else if constexpr( is_same<T, CHECKPOINT>::value ||
                           is_same<T, LOG_BUFFER>::value ||
                           is_same<T, LOG_STREAM>::value ||
                           is_same<T, LOG_WRITE_BUFFER>::value )
        {
            const CHAR * szType = SzType( ppt );

            if ( Pdls()->IpinstCurrent() == DEBUGGER_LOCAL_STORE::IpinstNil() )
            {
                dprintf( "ERROR: Tried to select implicit %hs type / INST, but no implicit INST is selected.  Use !ese .inst <InstName|Index> to select INST first.\n", szType );
                *ppt = (T*)NULL;
                return fFalse;
            }

            LOG * plogDebuggee = Pdls()->Pinst( Pdls()->IpinstCurrent() )->m_plog;
            LOG * plog = NULL;
            if ( !FFetchVariable( plogDebuggee, &plog ) )
            {
                dprintf( "ERROR: We have implicit INST, but could not retrieve LOG object off it, to find your type: %hs\n", szType );
            }

            //  ok we've gotten the inst and fetched the log, now resolve to type requested.

            if constexpr( is_same<T, CHECKPOINT>::value )
            {
                *ppt = PcheckpointEDBGAccessor( plog );
            }
            else if constexpr( is_same<T, LOG_BUFFER>::value )
            {
                *ppt = (LOG_BUFFER*)PlogbufferEDBGAddrAccessor( plogDebuggee ); // must be debuggee, b/c this is an embedded type.
            }
            else if constexpr( is_same<T, LOG_STREAM>::value )
            {
                *ppt = (LOG_STREAM*)PlogstreamEDBGAccessor( plog );
            }
            else if constexpr( is_same<T, LOG_WRITE_BUFFER>::value )
            {
                *ppt = PlogwritebufferEDBGAccessor( plog );
            }
            else
            {
                IntentionallyDoesNotLink();
            }
            Unfetch( plog );
            dprintf( "Retrieving default/implicit %hs: %p\n", szType, *ppt );
        }
        else if constexpr( is_same<T, FMP>::value )
        {
            if ( Pdls()->IfmpCurrent() == ifmpNil )
            {
                dprintf( "ERROR: Tried to select implicit FMP, but no implicit FMP is selected.  Use !ese .db <DbFileName|Ifmp> to select.\n" );
                *ppt = (T*)NULL;
                return fFalse;
            }
            FMP * rgfmpDebuggee;
            if ( !FReadGlobal( "g_rgfmp", &rgfmpDebuggee ) )
            {
                dprintf( "ERROR: We have a implicit FMP, but could not retrieve the g_rgfmp array for address computation!\n" );
                *ppt = (T*)NULL;
                return fFalse;
            }
            *ppt = rgfmpDebuggee + Pdls()->IfmpCurrent();
            dprintf( "Retrieving default/implicit FMP: %p\n", *ppt );
        }
        else if constexpr( is_same<T, DBFILEHDR>::value )
        {
            if ( Pdls()->IfmpCurrent() == ifmpNil )
            {
                dprintf( "ERROR: Tried to select implicit DBFILEHDR / FMP, but no implicit FMP is selected.  Use !ese .db <DbFileName|Ifmp> to select.\n" );
                *ppt = (T*)NULL;
                return fFalse;
            }
            const FMP * pfmp = Pdls()->PfmpCache( Pdls()->IfmpCurrent() );
            if ( pfmp == NULL )
            {
                dprintf( "ERROR: We have a implicit FMP, but could not retrieve the cached value!\n" );
                *ppt = (T*)NULL;
                return fFalse;
            }
            const DBFILEHDR * pdbfilehdr = PdbfilehdrEDBGAccessor( pfmp );
            *ppt = (T*)pdbfilehdr;
            dprintf( "Retrieving default/implicit DBFILEHDR: %p\n", *ppt );
        }
        else if constexpr( is_same<T, FCB>::value )
        {
            if ( Pdls()->ObjidCurrentBt() == objidNil )
            {
                dprintf( "ERROR: Tried to select implicit BT, but no implicit BT is selected.  Use !ese .bt <TableName>\\[<IndexName>]|<ObjidFDP> to select.\n" );
                *ppt = (T*)NULL;
                return fFalse;
            }
            const BOOL fSubTree = Pdls()->PfcbCurrentTableDebuggee() && Pdls()->PfcbCurrentSubTreeDebuggee() &&
                                  Pdls()->PfcbCurrentTableDebuggee() != Pdls()->PfcbCurrentSubTreeDebuggee();
            if ( !fSubTree )
            {
                *ppt = (T*)Pdls()->PfcbCurrentTableDebuggee();
            }
            else
            {
                *ppt = (T*)Pdls()->PfcbCurrentSubTreeDebuggee();
            }
            dprintf( "Retrieving default/implicit FCB: %p\n", *ppt );
        }
        else
        {
            dprintf( "ERROR: Sorry, this particular type (size %d bytes) does not have an implicit arg loader yet.  Feel free to implement it.\n", sizeof( T ) );
            *ppt = (T*)NULL;
            return fFalse;
        }

        return fTrue;
    }
    else if constexpr( is_same<T, COSFile>::value || is_same<T, COSDisk>::value )
    {
        const CHAR * szType = SzType( ppt );
        COSFile * posfDebuggee = NULL;

        //  Whether we need a file or a disk, first we do need a file first
        if ( FArgumentMatch( szArg, ".db" ) || FArgumentMatch( szArg, ".edb" ) )
        {
            if ( Pdls()->IfmpCurrent() == ifmpNil )
            {
                dprintf( "ERROR: Tried to select implicit / FMP, but no implicit FMP is selected.  Use !ese .db <DbFileName|Ifmp> to select.\n" );
                *ppt = (T*)NULL;
                return fFalse;
            }
            const FMP * pfmp = Pdls()->PfmpCache( Pdls()->IfmpCurrent() );
            if ( pfmp == NULL )
            {
                dprintf( "ERROR: We have a implicit FMP, but could not retrieve the cached value!\n" );
                *ppt = (T*)NULL;
                return fFalse;
            }
            posfDebuggee = (COSFile*)pfmp->Pfapi();
        }
        //  else if ( as needed, implement like ".log", ".jfm", etc ) ... but mostly we need the DB.

        if ( posfDebuggee )
        {
            if constexpr( is_same<T, COSFile>::value )
            {
                *ppt = posfDebuggee;
                dprintf( "Retrieving default/implicit %hs: %p\n", szType, *ppt );
                return fTrue;
            }

            COSFile * posf = NULL;
            if ( !FFetchVariable( posfDebuggee, &posf ) )
            {
                dprintf( "ERROR: We have implicit FILE, but could not retrieve COSVolume object off it, to find your type: %hs\n", szType );
                *ppt = (T*)NULL;
                return fFalse;
            }
            COSVolume * posvDebuggee = PosvEDBGAccessor( posf );
            COSVolume * posv;
            if ( !FFetchVariable( posvDebuggee, &posv ) )
            {
                dprintf( "ERROR: We have implicit FILE / COSVolume, but could not retrieve COSDisk object off it, to find your type: %hs\n", szType );
                Unfetch( posf );
                *ppt = (T*)NULL;
                return fFalse;
            }

            AssertEDBG( ( is_same<T, COSDisk>::value ) );
            *ppt = (T*)PosdEDBGGetDisk( posv );
            dprintf( "Retrieving default/implicit %hs: %p\n", szType, *ppt );

            Unfetch( posv );
            Unfetch( posf );

            return fTrue;
        }
        // else we didn't get a .implicit type arg we like, then fall through to FAddressFromSz()
    }

    return FAddressFromSz( szArg, ppt );
}
#pragma warning(pop)


//  This is an enhanced version of FUlFromSz, that is specific to IFMPs, and if the arg provided is 'the implicit 
//  arg' / i.e. ".", then the function will return the default / implicit IFMP / FMP.  If the arg is not ".", it 
//  will pass through to FUlFromSz().
LOCAL BOOL FAutoIfmpFromSz( const char* const szArg, ULONG* const pul, const INT base = 16 )
{
    if ( FImplicitArg( szArg ) )
    {
        if ( Pdls()->IfmpCurrent() == ifmpNil )
        {
            dprintf( "ERROR: Tried to select implicit FMP, but no implicit FMP is selected.  Use !ese .db <DbFileName|Ifmp> to select.\n" );
            return fFalse;
        }
        *pul = Pdls()->IfmpCurrent();
        return fTrue;
    }

    return FUlFromSz( szArg, pul, base );
}

BOOL FAutoPgnoRootFromSz( const CHAR* const szArg, PGNO * ppgnoRoot )
{
    if ( FImplicitArg( szArg ) )
    {
        if ( Pdls()->PfcbCurrentTableDebuggee() == NULL || 
             Pdls()->PgnoCurrentBt() == pgnoNull )
        {
            dprintf( "ERROR: Tried to select implicit BT / PgnoRoot, but no implicit BT is selected.  Use !ese .bt <TableName>\\<Index> to select.\n" );
            return fFalse;
        }

        *ppgnoRoot = Pdls()->PgnoCurrentBt();
        return fTrue;
    }

    return FUlFromSz( szArg, ppgnoRoot );
}

class SaveDlsDefaults // sdd
{
private:
    IFMP         m_ifmp;
    ULONG        m_ipinst;
    const FCB *  m_pfcbTableBt;
    const FCB *  m_pfcbSubTreeBt;
    PGNO         m_pgnoBt;
    OBJID        m_objidBt;

public:
    SaveDlsDefaults()
    {
        m_ifmp           = Pdls()->IfmpCurrent();
        m_ipinst         = Pdls()->IpinstCurrent();
        m_pfcbTableBt    = Pdls()->PfcbCurrentTableDebuggee();
        m_pfcbSubTreeBt  = Pdls()->PfcbCurrentSubTreeDebuggee();
        m_objidBt        = Pdls()->ObjidCurrentBt();
        m_pgnoBt         = Pdls()->PgnoCurrentBt();
    }

    ~SaveDlsDefaults()
    {
        if ( Pdls()->IfmpCurrent() != m_ifmp )
        {
            dprintf( "Note: Returning default / implicit .db (ifmp) to %d\n", m_ifmp );
        }
        if ( Pdls()->IpinstCurrent() != m_ipinst )
        {
            dprintf( "Note: Returning default / implicit .inst (ipinst) to %d\n", m_ipinst );
        }
        if ( Pdls()->ObjidCurrentBt() != m_objidBt )
        {
            dprintf( "Note: Returning default / implicit .bt (objid) to %d\n", m_objidBt );
        }

        Pdls()->SetImplicitIfmp( m_ifmp );
        Pdls()->SetImplicitIpinst( m_ipinst );
        Pdls()->SetImplicitBt( m_pfcbTableBt, m_pfcbSubTreeBt, m_objidBt, m_pgnoBt );
    }
};


//  Most of the iteration query extensions needed are defined in the DebuggerLocalState / DLS, so this
//  is the first place we can sensibly include this for now.

#define FITQUDebugMode            fDebugMode
#define PvITQUAlloc( cb )         ( Pdls()->PvDLSAlloc( cb ) )
#define ITQUFree( p )             ( Pdls()->DLSFree( p ) )
#define ITQUAddWarning( sz )      Pdls()->AddWarning( sz );
#define ITQUPrintf( szFmt, ... )  dprintf( szFmt, __VA_ARGS__ )
#define ITQUAssert                AssertEDBG
#define ITQUAssertSz              AssertEDBGSz
#define ITQUSetExprCxt( pentry )  Pdls()->SetImplicitIfmp( ((PBF)pentry)->ifmp )

#include "iterquery.hxx"


VOID
ReportEnforceFailure(
    const WCHAR * wszContext,
    const CHAR* szMessage,
    const WCHAR* wszIssueSource
    )
{
    Pdls()->AddWarning( szMessage );
}

//  indicates some code that is only designed to run in the debugger

VOID AssertEDBGDebugger()
{
    AssertEDBG( !g_dls.FLocalProcess() );
}

//  ================================================================
LOCAL void OSEdbgCreateDebuggerInterface(
PDEBUG_CLIENT   pdebugClient
)
//  ================================================================
{
    HRESULT hr;

    if ((hr = pdebugClient->QueryInterface(__uuidof(IDebugClient5),
                                          (void **)&g_DebugClient)) != S_OK)
    {
        goto HandleError;
    }

    if ((hr = g_DebugClient->QueryInterface(__uuidof(IDebugControl4),
                                          (void **)&g_DebugControl)) != S_OK)
    {
        goto HandleError;
    }

    if ((hr = g_DebugClient->QueryInterface(__uuidof(IDebugSymbols),
                                          (void **)&g_DebugSymbols)) != S_OK)
    {
        goto HandleError;
    }

    if ((hr = g_DebugClient->QueryInterface(__uuidof(IDebugSystemObjects),
                                          (void **)&g_DebugSystemObjects)) != S_OK)
    {
        goto HandleError;
    }

    if ((hr = g_DebugClient->QueryInterface(__uuidof(IDebugDataSpaces),
                                          (void **)&g_DebugDataSpaces)) != S_OK)
    {
        goto HandleError;
    }


HandleError:
    if ( FAILED( hr ) )
    {
        // Do I have to release the pointers?
        g_DebugClient = NULL;
        g_DebugControl = NULL;
    }
}

//  ================================================================
HRESULT
EDBGPrintfDml(
    __in PCSTR szFormat,
    ...
//  ================================================================
)
{
    HRESULT hr = S_OK;
    va_list Args;
    static BOOL fDmlAttemptedAndFailed = fFalse;

    va_start(Args, szFormat);
    if ( g_DebugControl == NULL )
    {
        dprintf( "Using an old debugger! No DML output.\n" );
    }
    else
    {
        hr = g_DebugControl->ControlledOutputVaList(
            DEBUG_OUTCTL_AMBIENT_DML, DEBUG_OUTPUT_NORMAL, szFormat, Args );
        if ( FAILED( hr ) )
        {
            if ( ! fDmlAttemptedAndFailed )
            {
                fDmlAttemptedAndFailed = fTrue;
                g_DebugControl->Output(
                    DEBUG_OUTPUT_ERROR, "Failed to output DML (%#x)! Please use a debugger 6.6.7 or newer.\n", hr );
            }
            hr = g_DebugControl->ControlledOutputVaList(
                DEBUG_OUTCTL_AMBIENT_TEXT, DEBUG_OUTPUT_NORMAL, szFormat, Args );
            if ( FAILED( hr ) )
            {
                g_DebugControl->Output(
                    DEBUG_OUTPUT_ERROR, "Failed to write regular output. hr = %#x.\n", hr );
            }
        }
    }
    va_end(Args);
    return hr;
}

//  ================================================================
HRESULT
EDBGPrintfDmlW(
    __in PCWSTR wszFormat,
    ...
//  ================================================================
)
{
    HRESULT hr = S_OK;
    va_list Args;
    static BOOL fDmlAttemptedAndFailed = fFalse;

    va_start(Args, wszFormat);
    if ( g_DebugControl == NULL )
    {
        dprintf( "Using an old debugger! No DML output.\n" );
    }
    else
    {
        hr = g_DebugControl->ControlledOutputVaListWide(
            DEBUG_OUTCTL_AMBIENT_DML, DEBUG_OUTPUT_NORMAL, wszFormat, Args );
        if ( FAILED( hr ) )
        {
            if ( ! fDmlAttemptedAndFailed )
            {
                fDmlAttemptedAndFailed = fTrue;
                g_DebugControl->Output(
                    DEBUG_OUTPUT_ERROR, "Failed to output DML (%#x)! Please use a debugger 6.6.7 or newer.\n", hr );
            }
            hr = g_DebugControl->ControlledOutputVaListWide(
                DEBUG_OUTCTL_AMBIENT_TEXT, DEBUG_OUTPUT_NORMAL, wszFormat, Args );
            if ( FAILED( hr ) )
            {
                g_DebugControl->Output(
                    DEBUG_OUTPUT_ERROR, "Failed to write regular output. hr = %#x.\n", hr );
            }
        }
    }
    va_end(Args);
    return hr;
}

//  ================================================================
HRESULT
EDBGPrintf(
    __in PCSTR szFormat,
    ...
)
//  ================================================================
{
    HRESULT hr = S_OK;
    va_list Args;

    va_start(Args, szFormat);
    if ( g_DebugControl == NULL )
    {
        // Can't print out a failure message -- it would just recurse.
    }
    else
    {
        hr = g_DebugControl->ControlledOutputVaList(
            DEBUG_OUTCTL_AMBIENT_TEXT, DEBUG_OUTPUT_NORMAL, szFormat, Args );
    }
    va_end(Args);
    return hr;
}

//  ================================================================
HRESULT
EDBGPrintfW(
    __in PCWSTR wszFormat,
    ...
)
//  ================================================================
{
    HRESULT hr = S_OK;
    va_list Args;

    va_start(Args, wszFormat);
    if ( g_DebugControl == NULL )
    {
        // Can't print out a failure message -- it would just recurse.
    }
    else
    {
        hr = g_DebugControl->ControlledOutputVaListWide(
            DEBUG_OUTCTL_AMBIENT_TEXT, DEBUG_OUTPUT_NORMAL, wszFormat, Args );
    }
    va_end(Args);
    return hr;
}


//  ================================================================
BOOL
FEDBGCheckForCtrlC()
//  ================================================================
{
    if ( g_DebugControl->GetInterrupt() == S_OK )
    {
        return fTrue;
    }
    else
    {
        return fFalse;
    }
}


//  ================================================================
ULONG64
GetExpression(
    __in PCSTR  szExpression
)
//  ================================================================
{
    HRESULT hr = S_OK;
    ULONG EndIdx;
    DEBUG_VALUE FullValue;
    ULONG64 Address = 0;

    CallDbg( g_DebugControl->Evaluate( szExpression, DEBUG_VALUE_INT64, &FullValue, &EndIdx ) );
    Address = FullValue.I64;

Cleanup:
    return Address;
}

//  ================================================================
BOOL
FEDBGMemoryRead(
    ULONG64                         ulAddressInDebuggee,
    __out_bcount(cbBuffer) PVOID    pbBuffer,
    __in ULONG                      cbBuffer,
    __out PULONG                    pcbRead
)
//  ================================================================
{
    HRESULT hr;
    hr = g_DebugDataSpaces->ReadVirtual( ulAddressInDebuggee, pbBuffer, cbBuffer, pcbRead );

    if ( FAILED( hr ) )
    {
        dprintf( "Error: Couldn't read memory at %#I64x, hr=%#x\n", ulAddressInDebuggee, hr );
    }
    return SUCCEEDED( hr );
}

//  ================================================================
BOOL FDuplicateHandle( const HANDLE hSourceProcess, const HANDLE hSource, HANDLE * const phDest )
//  ================================================================
{
    const HANDLE hDestinationProcess    = GetCurrentProcess();
    const DWORD dwDesiredAccess         = GENERIC_READ;
    const BOOL bInheritHandle           = FALSE;
    const DWORD dwOptions               = 0;

    const BOOL fSuccess = DuplicateHandle(
            hSourceProcess,
            hSource,
            hDestinationProcess,
            phDest,
            dwDesiredAccess,
            bInheritHandle,
            dwOptions );

    if( !fSuccess )
    {
        dprintf( "DuplicateHandle failed with error %d\n", GetLastError() );
    }
    return fSuccess;
}


//  ================================================================
const CHAR * SzEDBGHexDump( const VOID * const pv, const INT cb )
//  ================================================================
//
//  WARNING: not multi-threaded safe
//
{
    static CHAR rgchBuf[1024];
    rgchBuf[0] = '\n';
    rgchBuf[1] = '\0';

    if( NULL == pv )
    {
        return rgchBuf;
    }

    DBUTLSprintHex(
        rgchBuf,
        sizeof(rgchBuf),
        (BYTE *)pv,
        cb,
        cb + 1,
        4,
        0,
        0 );

    return rgchBuf;
}


//  ================================================================
DEBUG_EXT( EDBGVersion )
//  ================================================================
{
    BOOL        fMemCheck   = fFalse;

    if ( !FReadGlobal( "g_fMemCheck", &fMemCheck ) )
    {
        dprintf( "Error: Could not load debuggee data.\n" );
    }
    else
    {
        dprintf(    "%ws version %d.%02d.%04d.%04d (%ws)\n",
                    WszUtilImageVersionName(),
                    DwUtilImageVersionMajor(),
                    DwUtilImageVersionMinor(),
                    DwUtilImageBuildNumberMajor(),
                    DwUtilImageBuildNumberMinor(),
                    WszUtilImageBuildClass() );
        dprintf(    "\tDAE version      = 0x%x.%x\n",
                    ulDAEVersionMax,
                    ulDAEUpdateMajorMax );
        dprintf(    "\tLog version      = %d.%04d.%02d.%02d\n",
                    ulLGVersionMajorMax,
                    ulLGVersionMinorFinalDeprecatedValue,
                    ulLGVersionUpdateMajorMax,
                    ulLGVersionUpdateMinorMax );
        dprintf(    "\tFormat versions     = %d  (note these are from debugger ext, not the process)\n", PfmtversEngineMax()->efv );
        dprintf(    "\t    DB version      = %d.%d.%d\n",
                    PfmtversEngineMax()->dbv.ulDbMajorVersion,
                    PfmtversEngineMax()->dbv.ulDbUpdateMajor,
                    PfmtversEngineMax()->dbv.ulDbUpdateMinor );
        dprintf(    "\t   LOG version      = %d.%d.%d\n",
                    PfmtversEngineMax()->lgv.ulLGVersionMajor,
                    PfmtversEngineMax()->lgv.ulLGVersionUpdateMajor,
                    PfmtversEngineMax()->lgv.ulLGVersionUpdateMinor );
        dprintf(    "\t    FM version      = %d.%d.%d\n",
                    PfmtversEngineMax()->fmv.ulVersionMajor,
                    PfmtversEngineMax()->fmv.ulVersionUpdateMajor,
                    PfmtversEngineMax()->fmv.ulVersionUpdateMinor );
        if ( fMemCheck )
        {
            dprintf(    "\tMemory allocation tracking is Enabled\n" );
        }
        else
        {
            dprintf(    "\tMemory allocation tracking is Disabled\n" );
        }
    }
}


//  ================================================================
DEBUG_EXT( EDBGDebug )
//  ================================================================
{
    ULONG eDebugMode;
    if ( FUlFromSz( argv[ 0 ], &eDebugMode ) )
    {
        dprintf( "changing to specific debug mode %d\n", eDebugMode );
        g_eDebugMode = eDebugMode;
    }
    else if( fDebugMode )
    {
        dprintf( "changing to non-debug mode\n" );
        g_eDebugMode = eDebugModeErrors;
    }
    else
    {
        dprintf( "changing to debug mode\n" );
        g_eDebugMode = eDebugModeBasic;
    }
}

//  This takes a string that may be a hex or a decimal string, or not a number
//  at all and smartly tries to predict which.
//     "13"    -> fTrue  / pul = 13
//     "13 "   -> fTrue  / pul = 13
//     "13("   -> fTrue  / pul = 13
//     "0x13"  -> fTrue  / pul = 19
//     "1f"    -> fTrue  / pul = 31
//     "base"  -> fFalse / pul = <not-set> ... note it starts with ba, so is sort of a hex number.
//     "Col"   -> fFalse / pul = <not-set>
//
BOOL FGenUlFromSz( _In_z_ const CHAR * const szArg, _Out_ ULONG * pul )
{
    AssertEDBG( szArg != NULL ); // don't be rude!
    if ( szArg == NULL ) return fFalse;

    BOOL fHex = fFalse;
    for( size_t ich = 0; szArg[ich] != '0'; ich++ )
    {
        if ( !isdigit( szArg[ich] ) && !isxdigit( szArg[ich] ) )
        {
            if ( isalpha( szArg[ich] ) )
            {
                return fFalse;
            }
            break;
        }
        fHex |= isxdigit( szArg[ich] );
    }

    if ( fHex || ( szArg[0] == '0' && szArg[1] == 'x' ) )
    {
        return FUlFromSz( szArg, pul, 16 );
    }
    // else assume decimal number

    return FUlFromSz( szArg, pul, 10 );
}

BOOL FReadAsIndexArg( _In_z_ const CHAR * const szArg, _Out_ ULONG * pul )
{
    BOOL fRet = fFalse;
    ULONG ulRead = ulMax;

    const LONG ichLast = strlen( szArg ) - 1;
    const BOOL fArraySyntax = szArg[0] == '[' && szArg[ichLast] == ']';

    if ( fArraySyntax )
    {
        CHAR szNum[20];
        OSStrCbCopyA( szNum, sizeof( szNum ), &( szArg[1] ) );
        szNum[ sizeof(szNum) - 1 ] = '\0';
        fRet = FGenUlFromSz( szNum, &ulRead );
    }
    else
    {
        //  else - we also allow sloppy straight up ints to be passed, though we may deny this someday.

        fRet = FGenUlFromSz( szArg, &ulRead );
    }

    if ( fRet )
    {
        *pul = ulRead;
    }

    return fRet;
}

//  Simple (CRT-based) case-insenstive equal for simple debugger extension checks.

LOCAL BOOL FEqualIW( const WCHAR * const wszArg1, const WCHAR * const wszArg2 )
{
    const BOOL fMatch = ( ( wcslen( wszArg1 ) == wcslen( wszArg2 ) )
            && !( _wcsnicmp( wszArg1, wszArg2, wcslen( wszArg2 ) ) ) );
    return fMatch;
}

LOCAL BOOL FEqualIA( const CHAR * const szArg1, const CHAR * const szArg2 )
{
    const BOOL fMatch = ( ( strlen( szArg1 ) == strlen( szArg2 ) )
            && !( _strnicmp( szArg1, szArg2, strlen( szArg2 ) ) ) );
    return fMatch;
}

LOCAL BOOL FMatchISuffixA( const CHAR * const szSuffix, const CHAR * const szString )
{
    const size_t cchString = strlen( szString );
    const size_t cchSuffix = strlen( szSuffix );
    if ( cchSuffix > cchString )
    {
        return fFalse;
    }
    return 0 == _stricmp( &( szString[ cchString - cchSuffix ] ), szSuffix );
}

LOCAL BOOL FMatchISuffixW( const WCHAR * const wszSuffix, const WCHAR * const wszString )
{
    const size_t cchString = wcslen( wszString );
    const size_t cchSuffix = wcslen( wszSuffix );
    if ( cchSuffix > cchString )
    {
        return fFalse;
    }
    return 0 == _wcsicmp( &( wszString[ cchString - cchSuffix ] ), wszSuffix );
}

//  Tells you if user is asking for HELP!  In nearly any form:
//     -? /? -h -H /h /H -help /help

LOCAL BOOL FIsHelpArg( const CHAR * const szArg )
{
    if ( szArg == NULL )
    {
        return fFalse;  //  though should it be true?
    }
    return ( ( szArg[ 0 ] == '-' ) || ( szArg[ 0 ] == '/' ) ) &&
           ( ( szArg[ 1 ] == '?' ) || ( tolower( szArg[ 0 ] ) == 'h' ) ) &&
           ( ( szArg[ 2 ] == '\0' ) || FEqualIA( &(szArg[ 1 ]), "help" ) );
}

//  ================================================================
DEBUG_EXT( EDBGSetPii )
//  ================================================================
{
    BOOL fOn = fFalse;
    BOOL fValid = argc == 1 &&
                  (   ( fOn = FArgumentMatch( argv[0], "On" ) ) ||
                      ( FArgumentMatch( argv[0], "Off" ) )
                  );
    if ( !fValid )
    {
        dprintf( "ERROR: takes only one arg, provide a 'On' or 'Off'.\n" );
        return;
    }
    Pdls()->SetPII( fOn );
}

//  ================================================================
DEBUG_EXT( EDBGSetImplicitDB )
//  ================================================================
{
    if ( argc != 1 || FIsHelpArg( argv[ 0 ] ) )
    {
        if ( !FIsHelpArg( argv[ 0 ] ) )
        {
            dprintf( "ERROR: always takes only one arg, provide a DB, index, etc.\n" );
        }
        dprintf( "Takes one of these:\n" );
        dprintf( "    - DB name or partial name.\n" );
        dprintf( "    - A numerical ifmp index of the g_rgfmp array.\n" );
        dprintf( "    - A '.' means select / find singular non-temp DB.\n" );
        dprintf( "    - Or 'ifmpNil' to reset implicit .db state.\n" );
        dprintf( "\n" );

        size_t ifmpCurrDefault = Pdls()->IfmpCurrent();
        if ( ifmpCurrDefault != ifmpNil && ifmpCurrDefault != 0 && Pdls()->PfmpCache( ifmpCurrDefault ) != NULL )
        {
            WCHAR * wszDbFetchFail = L"UnableToFetchDbName";
            WCHAR * wszDatabaseName = wszDbFetchFail;
            (void)FFetchSz( Pdls()->PfmpCache( ifmpCurrDefault )->WszDatabaseName(), &wszDatabaseName );
            dprintf( "Note:  Current implicit FMP -->  [%d] %ws\n", ifmpCurrDefault, wszDatabaseName );
            Unfetch( wszDatabaseName != wszDbFetchFail ? wszDatabaseName : NULL );
        }
        else
        {
            dprintf( "Note:  Current implicit FMP/DB is unset\n\n" );
        }

        return;
    }

    if ( 0 == _stricmp( argv[ 0 ], "ifmpNil" ) )
    {
        dprintf( "Resetting implicit FMP (to ifmpNil).\n" );
        Pdls()->SetImplicitIfmp( ifmpNil );
        return;
    }

    ULONG    ifmpSelected             = ifmpNil;
    WCHAR *  wszDatabaseNameSelected  = NULL;

    if ( !FReadAsIndexArg( argv[ 0 ], &ifmpSelected ) )
    {
        // The argument did not look like an index.

        const BOOL fAutoSelect = FImplicitArg( argv[0] );
        WCHAR wszTargetName[ IFileSystemAPI::cchPathMax ];
        OSStrCbFormatW( wszTargetName, sizeof( wszTargetName ), L"%hs", argv[0] );

        dprintf( "Arg does not look like a index, searching g_rgfmp array for DB name: %ws\n", fAutoSelect ? L"<Auto-select singleton>" : wszTargetName );

        for ( IFMP ifmp = cfmpReserved; ifmp < Pdls()->CfmpAllocated(); ifmp++ )
        {
            const FMP *  pfmp                = NULL;
            WCHAR *      wszDatabaseName     = NULL;

            if ( ( pfmp = Pdls()->PfmpCache( ifmp ) ) != NULL &&
                //  consider: Should we drop FInUse()?
                ( pfmp->FInUse() && !FFetchSz( pfmp->WszDatabaseName(), &wszDatabaseName ) ) )
            {
                dprintf( "\n g_rgfmp[0x%x]  Error: Could not fetch FMP or DbName/Path at 0x%N. Aborting.\n", ifmp, pfmp );

                //  force out of loop
                ifmp = Pdls()->CfmpAllocated();
            }
            else
            {
                if ( wszDatabaseName )
                {
                    if ( FEqualIW( wszTargetName, wszDatabaseName ) ||
                         FMatchISuffixW( wszTargetName, wszDatabaseName ) )
                    {
                        if ( ifmpSelected != ifmpNil && ifmpSelected != ifmp )
                        {
                            dprintf( "WARNING:  Found two FMPs that match!!!  Use a more specific name or bypass and use the index.\n"
                                     "    [%d] - %ws\n"
                                     "    [%d] - %ws\n",
                                     ifmpSelected, wszDatabaseNameSelected,
                                     ifmp, wszDatabaseName );
                            Unfetch( wszDatabaseNameSelected );
                            wszDatabaseNameSelected = NULL;

                            return; // do not set anything.
                        }

                        ifmpSelected = ifmp;
                        wszDatabaseNameSelected = wszDatabaseName;
                        wszDatabaseName = NULL;
                    }
                    if ( fAutoSelect && !pfmp->FIsTempDB() )
                    {
                        if ( ifmpSelected != ifmpNil && ifmpSelected != ifmp )
                        {
                            dprintf( "ERROR:  Tried to auto-select single DB, but found two FMPs!  You will have to pick one\n"
                                     "    [%d] - %ws\n"
                                     "    [%d] - %ws\n",
                                     ifmpSelected, wszDatabaseNameSelected,
                                     ifmp, wszDatabaseName );
                            Unfetch( wszDatabaseNameSelected );
                            wszDatabaseNameSelected = NULL;

                            return; // do not set anything.
                        }
                        ifmpSelected = ifmp;
                        wszDatabaseNameSelected = wszDatabaseName;
                        wszDatabaseName = NULL;
                    }
                }
            }

            Unfetch( wszDatabaseName );
        }
    }

    if ( ifmpSelected >= cfmpReserved && ifmpSelected != ifmpNil && ifmpSelected != ulMax && Pdls()->PfmpCache( ifmpSelected ) != NULL )
    {
        const FMP * pfmp = Pdls()->PfmpCache( ifmpSelected );
        if ( wszDatabaseNameSelected == NULL )  // selected by direct index
        {
            (void)FFetchSz( pfmp->WszDatabaseName(), &wszDatabaseNameSelected );
        }

        //  Now, actually set implicit / default FMP!
        dprintf( "Selected implicit FMP --> [%d] %ws\n", ifmpSelected, wszDatabaseNameSelected ? wszDatabaseNameSelected : L"CouldNotLoadDbName" );
        Pdls()->SetImplicitIfmp( ifmpSelected );

        //  And since an FMP implies an INST, set implicit INST as well!
        INST * pinstDebuggee = pfmp->Pinst();
        INST * pinstTarget = NULL;
        if ( FFetchVariable( pinstDebuggee, &pinstTarget ) )
        {
            ULONG ipinstTarget = pinstTarget->m_iInstance - 1;
            //  If this doesn't hold, can make a Pdls()->IpinstFromPinst( pinstDebuggee ) that walks 
            //  the DLS's pinst debuggee array to find the right ipinst.
            AssertEDBG( Pdls()->Pinst( ipinstTarget ) != NULL &&
                        ( Pdls()->Pinst( ipinstTarget )->m_iInstance ) == pinstTarget->m_iInstance &&
                        Pdls()->PinstDebuggee( ipinstTarget ) == pinstDebuggee );
            if ( Pdls()->Pinst( ipinstTarget ) != NULL &&
                 ( Pdls()->Pinst( ipinstTarget )->m_iInstance ) == pinstTarget->m_iInstance &&
                 Pdls()->PinstDebuggee( ipinstTarget ) == pinstDebuggee )
            {
                dprintf( "[Also] Implicitly selected implicit INST --> [%d]\n" );
                Pdls()->SetImplicitIpinst( ipinstTarget );
            }
            Unfetch( pinstTarget );
            // Note: if cache does not have it, we just leak pinst.
        }
    }
    else
    {
        dprintf( "ERROR: Could not locate the FMP / Database specified: %d \\ %hs \n", ifmpSelected, argv[0] );
    }

    dprintf( "\n" );

    Unfetch( wszDatabaseNameSelected );
}

//  ================================================================
DEBUG_EXT( EDBGSetImplicitInst )
//  ================================================================
{
    WCHAR * wszInstanceNameSelected = NULL;
    WCHAR * wszDisplayNameSelected = NULL;

    if ( argc != 1 || FIsHelpArg( argv[ 0 ] ) )
    {
        if ( !FIsHelpArg( argv[ 0 ] ) )
        {
            dprintf( "ERROR: always takes only one arg, provide a ipinst, inst name, etc.\n" );
        }
        dprintf( "Takes one of these:\n" );
        dprintf( "    - An instance or display name or partial name.\n" );
        dprintf( "    - A numerical ipinst index of the g_rgpinst array.\n" );
        dprintf( "    - Or 'ipinstNil' to reset implicit .inst state.\n" );
        dprintf( "\n" );

        size_t ipinstCurrDefault = Pdls()->IpinstCurrent();
        if ( ipinstCurrDefault != DEBUGGER_LOCAL_STORE::IpinstNil() && Pdls()->PinstDebuggee( ipinstCurrDefault ) != NULL )
        {
            WCHAR * wszInstFetchFail = L"UnableToFetchInstName";
            WCHAR * wszDispFetchFail = L"UnableToFetchDispName";
            WCHAR * wszInstanceName = wszInstFetchFail;
            WCHAR * wszDisplayName = wszDispFetchFail;
            INST * pinst = Pdls()->Pinst( ipinstCurrDefault );
            (void)FFetchSz( pinst->m_wszInstanceName, &wszInstanceName );
            (void)FFetchSz( pinst->m_wszDisplayName, &wszDisplayName );
            Pdls()->SetImplicitIpinst( (ULONG)ipinstCurrDefault );
            dprintf( "Note:  Current implicit INST --> [%d] %ws \\ %ws\n\n", ipinstCurrDefault, wszDisplayNameSelected, wszInstanceNameSelected );
            Unfetch( wszInstanceName != wszInstFetchFail ? wszInstanceName : NULL );
            Unfetch( wszDisplayName != wszDispFetchFail ? wszDisplayName : NULL );
        }
        else
        {
            dprintf( "Note:  Current implicit INST is unset\n\n" );
        }

        return;
    }

    if ( 0 == _stricmp( argv[ 0 ], "ipinstNil" ) )
    {
        dprintf( "Resetting implicit INST (to ipinstNil).\n" );
        Pdls()->SetImplicitIpinst( DEBUGGER_LOCAL_STORE::IpinstNil() );
        return;
    }

    ULONG ipinstSelected = DEBUGGER_LOCAL_STORE::IpinstNil();

    if ( FReadAsIndexArg( argv[ 0 ], &ipinstSelected ) &&
         ipinstSelected != DEBUGGER_LOCAL_STORE::IpinstNil() )
    {
        INST * pinstDebuggee = Pdls()->PinstDebuggee( ipinstSelected );
        INST * pinst = NULL;
        if ( pinstDebuggee == NULL || !FFetchVariable( pinstDebuggee, &pinst ) )
        {
            dprintf( "ERROR: The ipinst %d array slot chosen does not seem to be a live instance, or can not load the INST from DLS pinst cache: %p\n", 
                     ipinstSelected, Pdls()->PinstDebuggee( ipinstSelected ) );
            return;
        }
        (void)FFetchSz( pinst->m_wszInstanceName, &wszInstanceNameSelected );
        (void)FFetchSz( pinst->m_wszDisplayName, &wszDisplayNameSelected );
        dprintf( "Selected implicit INST --> [%d] %ws \\ %ws\n", ipinstSelected, wszDisplayNameSelected, wszInstanceNameSelected );
        Pdls()->SetImplicitIpinst( ipinstSelected );
        Unfetch( wszInstanceNameSelected );
        Unfetch( wszDisplayNameSelected );
        return;
    }

    // The argument did not look like an index.

    WCHAR wszTargetName[ IFileSystemAPI::cchPathMax ];
    OSStrCbFormatW( wszTargetName, sizeof( wszTargetName ), L"%hs", argv[0] );

    dprintf( "Arg does not look like a index .." );

    INST **     rgpinstDebuggee     = NULL;
    INST **     rgpinst             = NULL;
    ULONG       cpinstMax           = 0;    //  cMaxInstances is now variable in essence.

    if ( cpinstMax == 0 &&
            !FReadGlobal( "g_cpinstMax", &cpinstMax ) )
    {
        dprintf( "\nERROR: Could not fetch instance table size.\n" );
        return;
    }
    if ( !FReadGlobal( "g_rgpinst", &rgpinstDebuggee ) )
    {
        dprintf( "\nERROR: Could not fetch instance table address.\n" );
        return;
    }
    if ( !FFetchVariable( rgpinstDebuggee, &rgpinst, cpinstMax ) )
    {
        dprintf( "\nERROR: Could not fetch instance table.\n" );
        return;
    }

    dprintf( "..scanning 0x%X (of %#X possible) INST's starting at 0x%N for the specified inst: %ws.\n", cpinstMax, cMaxInstances, rgpinstDebuggee, wszTargetName );

    for ( ULONG ipinst = 0; ipinst < cpinstMax; ipinst++ )
    {
        if ( rgpinst[ipinst] != pinstNil )
        {
            INST *  pinst   = NULL;

            //  validate our DLS
            if ( rgpinst[ ipinst ] != Pdls()->PinstDebuggee( ipinst ) )
            {
                Pdls()->AddWarning( "Debugger DLS - PinstDebuggee() state doesn't match, some debugger extensions not trustable. May happen on live debug though." );
            }

            if ( !FFetchVariable( rgpinst[ ipinst ], &pinst ) )
            {
                dprintf(    "\n g_rgpinst[0x%x]  Error: Could not fetch INST at 0x%N. Aborting.\n",
                            ipinst,
                            rgpinst[ipinst] );

                //  force out of loop
                ipinst = cpinstMax;
            }
            else
            {
                WCHAR *  wszInstanceName = NULL;
                WCHAR *  wszDisplayName  = NULL;

                //  validate our DLS
                if ( ( Pdls()->Pinst( ipinst ) != NULL ) &&
                        ( 0 != memcmp( Pdls()->Pinst( ipinst ), pinst, sizeof(INST) ) ) )
                {
                    Pdls()->AddWarning( "Debugger DLS - Pinst() state doesn't match, some debugger extensions not trustable. May happen on live debug though." );
                }

                /* Example:
                    g_rgpinst[0x0]  (INST 0x00000212DE130020)
                                           m_wszInstanceName : 1ded8fa0-2afb-4d47-8c1e-82cfe4724f46
                                            m_wszDisplayName : NAMPR10DG140-db124
                                                      m_pver : 0x00000212E2C50000 (version bucket usage: 2/24578)
                                                      m_plog : 0x00000212DE810020 (checkpoint: 519 gens [min=0x25bb95f, max=0x25bbb66], rec state: RecNone )
                */

                if ( FFetchSz( pinst->m_wszInstanceName, &wszInstanceName ) &&
                     FEqualIW( wszInstanceName, wszTargetName ) )
                {
                    (void)FFetchSz( pinst->m_wszDisplayName, &wszDisplayName );

                    if ( ipinstSelected != DEBUGGER_LOCAL_STORE::IpinstNil() && ipinstSelected != ipinst )
                    {
                        dprintf( "ERROR:  Found two INSTs that match!!!  Use a more specific name or bypass and use the index.\n"
                                 "    [%d] - %ws \\ %ws\n"
                                 "    [%d] - %ws \\ %ws\n",
                                 ipinstSelected, wszDisplayNameSelected, wszInstanceNameSelected,
                                 ipinst, wszDisplayName, wszInstanceName );
                        Unfetch( wszInstanceNameSelected );
                        Unfetch( wszDisplayNameSelected );
                        wszInstanceNameSelected = NULL; // for saftey, clobbered immediately
                        wszDisplayNameSelected = NULL;

                        return; // do not set anything.
                    }
                    ipinstSelected = ipinst;
                    wszInstanceNameSelected = wszInstanceName;
                    wszInstanceName = NULL;
                    wszDisplayNameSelected = wszDisplayName;
                    wszDisplayName = NULL;
                }
                else if ( FFetchSz( pinst->m_wszDisplayName, &wszDisplayName ) && 
                          FEqualIW( wszDisplayName, wszTargetName ) )
                {
                    if ( ipinstSelected != DEBUGGER_LOCAL_STORE::IpinstNil() && ipinstSelected != ipinst )
                    {
                        dprintf( "ERROR:  Found two INSTs that match!!!  Use a more specific name or bypass and use the index.\n"
                                 "    [%d] - %ws \\ %ws\n"
                                 "    [%d] - %ws \\ %ws\n",
                                 ipinstSelected, wszDisplayNameSelected, wszInstanceNameSelected,
                                 ipinst, wszDisplayName, wszInstanceName );
                        Unfetch( wszInstanceNameSelected );
                        Unfetch( wszDisplayNameSelected );
                        wszInstanceNameSelected = NULL; // for saftey, clobbered immediately
                        wszDisplayNameSelected = NULL;

                        return; // do not set anything.
                    }
                    ipinstSelected = ipinst;
                    wszInstanceNameSelected = wszInstanceName;
                    wszInstanceName = NULL;
                    wszDisplayNameSelected = wszDisplayName;
                    wszDisplayName = NULL;
                }

                Unfetch( wszInstanceName );
                Unfetch( wszDisplayName );
            }

            Unfetch( pinst );
        }
    }

    Unfetch( rgpinst );

    if ( ipinstSelected != DEBUGGER_LOCAL_STORE::IpinstNil() && ipinstSelected != ulMax && Pdls()->Pinst( ipinstSelected ) != NULL )
    {
        //  Now, actually set implicit / default INST!
        dprintf( "Selected implicit INST --> [%d] %ws \\ %ws\n", ipinstSelected, wszDisplayNameSelected, wszInstanceNameSelected );
        Pdls()->SetImplicitIpinst( ipinstSelected );
    }
    else
    {
        dprintf( "ERROR: Could not locate the INST / instance specified: %hs\n", argv[0] );
    }

    dprintf( "\n" );

    Unfetch( wszInstanceNameSelected );
    Unfetch( wszDisplayNameSelected );
}

LOCAL BOOL FEDBGFetchTableMetaData(
    const FCB * const pfcbTableDebuggee,
    FCB **            ppfcbTable );

enum EdbgSpaceTreeFind // edbgsptf
{
    edbgsptfNone         = 0,  //  i.e. the regular tree
    edbgsptfOwnedExtent  = 1,
    edbgsptfAvailExtent  = 2
};

const CHAR * szLvIndexSelect = "(LV)";  //  Yes, the LV is not an index, but we pass it in the index arg.

//  ================================================================
LOCAL BOOL FEDBGFindAndSetImplicitBt(
    _In_ const ULONG              ipinstTarget,
    _In_ const IFMP               ifmpTarget,
    _In_z_ const CHAR *           szTableSearch,
    _In_z_ const CHAR *           szSubTree,
    _In_ const OBJID              objidSearch,
    _In_ const EdbgSpaceTreeFind  edbgsptf,
    _Out_ FCB **                  ppfcbTable,
    _Out_ FCB **                  ppfcbSubTree,
    _Out_ OBJID *                 pobjidFDP,
    _Out_ PGNO *                  ppgnoFDP )
//  ================================================================
{
    INST *                  pinst               = pinstNil;
    ULONG                   cFoundTableFCBs     = 0;
    BOOL                    fError              = fFalse;
    const ULONG             cchTableName        = szTableSearch ? strlen( szTableSearch ) : 0;

    const BOOL              fTryPrefixMatch     = ( ( cchTableName > 0 ) && ( '*' == szTableSearch[ cchTableName - 1 ] ) );
    const BOOL              fTrySuffixMatch     = ( ( cchTableName > 0 ) && ( '*' == szTableSearch[ 0 ] ) );

    const BOOL              fFindLV             = ( ( szSubTree != NULL ) && FArgumentMatch( szSubTree, szLvIndexSelect ) );

    IFMP                    ifmpLastMatch       = ifmpNil;
    SIZE_T                  ipinstLastMatch     = DEBUGGER_LOCAL_STORE::IpinstNil();

    AssertEDBG( ( szTableSearch == NULL ) ^ ( objidSearch == objidNil ) );

    *ppfcbTable    = pfcbNil;
    *ppfcbSubTree  = pfcbNil;
    *pobjidFDP     = objidNil;
    *ppgnoFDP      = pgnoNull;

    if ( fTryPrefixMatch && fTrySuffixMatch )
    {
        dprintf( "ERROR: Doing both a prefix and suffix wildcard is NYI." );
        fError = fTrue;
        goto HandleError;
    }

    //  HACK! HACK!
    //
    //  The shadow catalog is always named MSysObjects in the TDB
    //  (see ErrCATIInitCatalogTDB) so when searching for one
    //  of the catalog tables, can't use a name match and must
    //  instead use an objid match
    //
    const BOOL              fFindCatalog        = szTableSearch && FCATSystemTable( szTableSearch );
    const OBJID             objidFindMSO        = ( fFindCatalog ? ObjidCATTable( szTableSearch ) : objidNil );

    if ( Pdls()->ErrINSTDLSInitCheck() < JET_errSuccess )
    {
        fError = fTrue;
        goto HandleError;
    }

    //  scan FCB list of all INST's
    //
    for ( SIZE_T ipinst = 0; ipinst < Pdls()->Cinst() && !fError; ipinst++ )
    {
        if ( pinstNil == Pdls()->PinstDebuggee( ipinst ) )
        {
            continue;
        }

        if ( ipinstTarget != DEBUGGER_LOCAL_STORE::IpinstNil() && ipinstTarget != ipinst )
        {
            continue;
        }

        dprintf( "\nScanning all FCB's of instance 0x%N...\n", Pdls()->PinstDebuggee( ipinst ) );
        pinst = Pdls()->Pinst( ipinst );

        if ( pinst == NULL )
        {
            dprintf( "    g_rgpinst[0x%x]  Error: Could not read INST at 0x%N. Aborting.\n", ipinst, Pdls()->PinstDebuggee( ipinst ) );
            fError = fTrue;
            goto HandleError;
        }

        FCB *   pfcbTableDebuggee    = pinst->m_pfcbList;
        FCB *   pfcbTable            = NULL;

        FCB *   pfcbSubTreeDebuggee  = NULL;
        FCB *   pfcbSubTree          = NULL;

        while ( pfcbNil != pfcbTableDebuggee )
        {
            BOOL    fMatchingTable = fFalse;

            //  retrieve meta-data
            //
            if ( !FEDBGFetchTableMetaData( pfcbTableDebuggee, &pfcbTable ) )
            {
                dprintf( "ERROR: Could not retrieve table metadata on pfcb = %p\n", pfcbTableDebuggee );
                fError = fTrue;
                break;
            }

            if ( ifmpNil != ifmpTarget && pfcbTable->Ifmp() != ifmpTarget )
            {
                goto NextFcb;
            }

            //  see if this FCB is a match
            //

            if ( objidSearch != objidNil && pfcbTable->ObjidFDP() == objidSearch )
            {
                AssertEDBG( szTableSearch == NULL );
                // may not be actual table ... 
                if ( !pfcbTable->FTypeTable() )
                {
                    // This is either a secondary index or LV, rearrange so the rest of the code works.
                    pfcbSubTree = pfcbTable;
                    pfcbSubTreeDebuggee = pfcbTableDebuggee;
                    pfcbTableDebuggee = pfcbSubTree->PfcbTable();
                    
                    if ( !FEDBGFetchTableMetaData( pfcbTableDebuggee, &pfcbTable ) )
                    {
                        dprintf( "ERROR: Could not retrieve table metad data on pfcb = %p\n", pfcbTableDebuggee );
                        fError = fTrue;
                        break;
                    }
                }

                fMatchingTable = fTrue;
            }

            if ( szTableSearch != NULL && pfcbTable->FTypeTable() )
            {
                AssertEDBG( objidSearch == objidNil );

                if ( fFindCatalog )
                {
                    fMatchingTable = ( objidFindMSO == pfcbTable->ObjidFDP() );
                }
                else if ( pfcbTable->Ptdb() != NULL )
                {
                    if ( fTryPrefixMatch )
                    {
                        fMatchingTable = ( 0 == _strnicmp( szTableSearch, pfcbTable->Ptdb()->SzTableName(), cchTableName - 1 ) );  //  -1 because we don't want to compare the wildcard character
                    }
                    else if ( fTrySuffixMatch )
                    {
                        fMatchingTable = FMatchISuffixA( &( szTableSearch[ 1 ] ), pfcbTable->Ptdb()->SzTableName() );  //  skip the search term past the * \ wildcard
                    }
                    else
                    {
                        fMatchingTable = ( 0 == UtilCmpName( szTableSearch, pfcbTable->Ptdb()->SzTableName() ) );
                    }
                }

            } // szTableSearch && FTypeTable()


            if ( fMatchingTable )
            {
                FCB * pfcbIndex = NULL;
                IDB * pidbIndex = NULL;

                FCB * pfcbSubTreeDebuggeePrevious = pfcbSubTreeDebuggee;
                pfcbSubTreeDebuggee = NULL;

                cFoundTableFCBs++;

                if ( pfcbSubTreeDebuggee == NULL && // for case where we located this by objid of sub-tree specified above in objidSearch clause.
                     fFindLV && pfcbTable->FTypeTable() )
                {
                    if ( pfcbTable->Ptdb()->PfcbLV() == NULL )
                    {
                        dprintf( "ERROR: Found the table you wanted: %hs / objid=%d, but had NULL PfcbLV()\n", pfcbTable->Ptdb()->SzTableName(), pfcbTable->ObjidFDP() );
                        fError = fTrue;
                        break;
                    }
                    pfcbSubTreeDebuggee = pfcbTable->Ptdb()->PfcbLV();
                    if ( !FFetchVariable( pfcbSubTreeDebuggee, &pfcbSubTree ) )
                    {
                        dprintf( "ERROR: Found a table / %hs you wanted w/ LV tree / FCB / objid=%d, but could not fetch FCB from debugger.\n", pfcbTable->Ptdb()->SzTableName(), pfcbTable->ObjidFDP() );
                        fError = fTrue;
                        break;
                    }

                } else if ( pfcbSubTreeDebuggee == NULL && 
                            szSubTree != NULL ) {
                    FCB * pfcbIndexNextDebuggee = pfcbTable->PfcbNextIndex();
                    while( pfcbIndexNextDebuggee != pfcbNil )
                    {
                        if ( !FFetchVariable( pfcbIndexNextDebuggee, &pfcbIndex ) )
                        {
                            dprintf( "ERROR: Found a table / %hs / objid=%d, you wanted w/ indices but could not fetch Index FCBs from debugger.\n", pfcbTable->Ptdb()->SzTableName(), pfcbTable->ObjidFDP() );
                            fError = fTrue;
                            break;
                        }

                        AssertEDBG( pfcbIndex->FTypeSecondaryIndex() );

                        if ( !FFetchVariable( pfcbIndex->Pidb(), &pidbIndex ) )
                        {
                            dprintf( "ERROR: Found a table / %hs and index objid=%d, you wanted w/ indices but could not fetch Index IDBs from debugger.\n", pfcbTable->Ptdb()->SzTableName(), pfcbIndex->ObjidFDP() );
                            Unfetch( pfcbIndex );
                            pfcbIndex = NULL;
                            fError = fTrue;
                            break;
                        }

                        const CHAR * szIndexName = pfcbTable->Ptdb()->SzIndexName( pidbIndex->ItagIndexName(), pfcbIndex->FDerivedIndex() );
                        if ( 0 == UtilCmpName( szSubTree, szIndexName ) )
                        {
                            if ( pfcbIndexNextDebuggee != pfcbTableDebuggee && pfcbSubTreeDebuggeePrevious != NULL )
                            {
                                dprintf( "ERROR: Found a 2nd table + index / %hs \\ %hs / objid=%d (ifmp = 0x%x), requested, bailing out name too ambiguous.\n", pfcbTable->Ptdb()->SzTableName(), szIndexName, pfcbTable->ObjidFDP(), pfcbTable->Ifmp() );
                                fError = fTrue;
                                break;
                            }
                            pfcbSubTreeDebuggee = pfcbIndexNextDebuggee;
                            pfcbSubTree = pfcbIndex;
                            pfcbIndex = NULL;
                            break;
                        }

                        pfcbIndexNextDebuggee = pfcbIndex->PfcbNextIndex();

                        Unfetch( pidbIndex );
                        pidbIndex = NULL;
                        Unfetch( pfcbIndex );
                        pfcbIndex = NULL;
                    }
                    if ( pfcbSubTreeDebuggee == NULL )
                    {
                        dprintf( "ERROR: Found a table %hs / objid=%d you wanted but no matching index: %hs\n", pfcbTable->Ptdb()->SzTableName(), pfcbTable->ObjidFDP(), szSubTree );
                        //  Should we really set fError?  What if the table name is ambiguous like "MSysObj*" but with index name it 
                        //  is not:  "MSysObj*\Name" (remember MSysObjectsShadow has no secondary indices).  We'll for now, just force
                        //  people to fully specify table name (which should create 1 match) to get to right sub-index name.
                        fError = fTrue;
                        goto NextFcb;
                    }

                } // else just pure-table selection


                //
                //     We have a match, print details and set implicit BT.
                //

                const BOOL fSelectingSubTree = ( pfcbSubTreeDebuggee != NULL && pfcbSubTree != NULL );

                *ppfcbTable = pfcbTableDebuggee;
                *ppfcbSubTree = pfcbSubTreeDebuggee;
                *pobjidFDP = ( fSelectingSubTree ? pfcbSubTree->ObjidFDP() : pfcbTable->ObjidFDP() );

                switch( edbgsptf )
                {
                case edbgsptfNone:
                    *ppgnoFDP = ( fSelectingSubTree ? pfcbSubTree->PgnoFDP() : pfcbTable->PgnoFDP() );
                    break;

                case edbgsptfOwnedExtent:
                    *ppgnoFDP = ( fSelectingSubTree ? pfcbSubTree->PgnoOE() : pfcbTable->PgnoOE() );
                    break;

                case edbgsptfAvailExtent:
                    *ppgnoFDP = ( fSelectingSubTree ? pfcbSubTree->PgnoAE() : pfcbTable->PgnoAE() );
                    break;

                default: AssertEDBG( fFalse );
                }

                if ( !fFindLV && szSubTree && pfcbSubTreeDebuggee )
                {
                    AssertEDBG( pidbIndex ); // needed for printing in next dprintf.
                }

                dprintf( "Selected implicit BT --> 0x%N / %N [ifmp:0x%x, objidFDP:0x%x%hs, pgnoFDP:0x%x] - %hs%hs%hs%hs\n",
                        pfcbTableDebuggee,
                        pfcbSubTreeDebuggee,
                        pfcbTable->Ifmp(),
                        *pobjidFDP,
                        ( edbgsptf == edbgsptfNone ) ? "" : ( ( edbgsptf == edbgsptfOwnedExtent ) ? "[OE]" : "[AE]" ),
                        *ppgnoFDP,
                        pfcbTable->Ptdb()->SzTableName(),
                        ( pfcbSubTreeDebuggee ) ? "\\" : "",
                        ( fFindLV && pfcbSubTreeDebuggee ) ? 
                            szLvIndexSelect :
                            ( ( szSubTree && pfcbSubTreeDebuggee ) ?
                               pfcbTable->Ptdb()->SzIndexName( pidbIndex->ItagIndexName(), pfcbSubTree->FDerivedIndex() ) :
                               "" ),
                        ( !fFindLV && szSubTree && pfcbSubTreeDebuggee && pfcbSubTree->FDerivedIndex() ) ? "(derived)" : ""
                        );
                Pdls()->SetImplicitBt( *ppfcbTable, *ppfcbSubTree, *pobjidFDP, *ppgnoFDP );

                ifmpLastMatch = pfcbTable->Ifmp();
                ipinstLastMatch = ipinst;

                Unfetch( pfcbIndex );
                pfcbIndex = NULL;
                Unfetch( pidbIndex );
                pidbIndex = NULL;
                Unfetch( pfcbSubTree );
                pfcbSubTree = NULL;

            } // fMatchingTable

          NextFcb:

            pfcbTableDebuggee = pfcbTable->PfcbNextList();
            Unfetch( pfcbTable );
            pfcbTable = NULL;
        } // while

        Unfetch( pfcbTable );
        pfcbTable = NULL;

    } // for each inst

    if ( 0 == cFoundTableFCBs )
    {
        //  we did not find the FCB
        //
        dprintf( "\nERROR: Could not find any FCB's with table name \"%s\".\n", szTableSearch );
        fError = fTrue;
        goto HandleError;
    }
    else if ( 1 != cFoundTableFCBs )
    {
        dprintf( "\nERROR: Found %d FCB's with table name \"%s\", search specification too ambiguous!\n", cFoundTableFCBs, szTableSearch );
        Pdls()->SetImplicitBt( pfcbNil, pfcbNil, objidNil, pgnoNull ); // reset everything, don't let last FCB we saw linger as implicit state.
        fError = fTrue;
        goto HandleError;
    }

    //  Yay!  We only found one, return success (and set other implicit contexts)

    if ( ifmpLastMatch != ifmpNil )
    {
        WCHAR * wszDbFetchFail = L"UnableToFetchDbName";
        WCHAR * wszDatabaseName = wszDbFetchFail;
        (void)FFetchSz( Pdls()->PfmpCache( ifmpLastMatch )->WszDatabaseName(), &wszDatabaseName );
        dprintf( "[Also] selecting implicit FMP -->  [%d] %ws\n", ifmpLastMatch, wszDatabaseName );
        Pdls()->SetImplicitIfmp( ifmpLastMatch );
        Unfetch( wszDatabaseName != wszDbFetchFail ? wszDatabaseName : NULL );
    }

    if ( ipinstLastMatch != DEBUGGER_LOCAL_STORE::IpinstNil() )
    {
        WCHAR * wszInstFetchFail = L"UnableToFetchInstName";
        WCHAR * wszDispFetchFail = L"UnableToFetchDispName";
        WCHAR * wszInstanceName = wszInstFetchFail;
        WCHAR * wszDisplayName = wszDispFetchFail;
        pinst = Pdls()->Pinst( ipinstLastMatch );
        if ( pinst->m_wszInstanceName ) (void)FFetchSz( pinst->m_wszInstanceName, &wszInstanceName );
        if ( pinst->m_wszDisplayName )  (void)FFetchSz( pinst->m_wszDisplayName, &wszDisplayName );
        Pdls()->SetImplicitIpinst( (ULONG)ipinstLastMatch );
        dprintf( "[Also] selecting implicit INST --> [%d] / %p - %ws \\ %ws\n", ipinstLastMatch, Pdls()->PinstDebuggee( ipinstLastMatch ), 
                 pinst->m_wszDisplayName ? wszDisplayName : L"NULL", pinst->m_wszInstanceName ? wszInstanceName : L"NULL" );
        Unfetch( wszInstanceName != wszInstFetchFail ? wszInstanceName : NULL );
        Unfetch( wszDisplayName != wszDispFetchFail ? wszDisplayName : NULL );
    }

    fError = fFalse;
    dprintf( "\n" );

HandleError:

    return !fError;
}


LOCAL BOOL FEDBGFindAndSetImplicitBt(
    _In_ const ULONG              ipinstTarget,
    _In_ const IFMP               ifmpTarget,
    _In_z_ const CHAR *           szTableSearch,
    _In_z_ const CHAR *           szSubTree,
    _In_ const EdbgSpaceTreeFind  edbgsptf,
    _Out_ FCB **                  ppfcbTable,
    _Out_ FCB **                  ppfcbSubTree,
    _Out_ OBJID *                 pobjidFDP,
    _Out_ PGNO *                  ppgnoFDP )
{
    return FEDBGFindAndSetImplicitBt( ipinstTarget, ifmpTarget, szTableSearch, szSubTree, objidNil, edbgsptf, ppfcbTable, ppfcbSubTree, pobjidFDP, ppgnoFDP );
}

LOCAL BOOL FEDBGFindAndSetImplicitBt(
    _In_ const ULONG              ipinstTarget,
    _In_ const IFMP               ifmpTarget,
    _In_ const OBJID              objidSearch,
    _In_z_ const CHAR *           szSubTree,
    _In_ const EdbgSpaceTreeFind  edbgsptf,
    _Out_ FCB **                  ppfcbTable,
    _Out_ FCB **                  ppfcbSubTree,
    _Out_ OBJID *                 pobjidFDP,
    _Out_ PGNO *                  ppgnoFDP )
{
    return FEDBGFindAndSetImplicitBt( ipinstTarget, ifmpTarget, NULL, NULL, objidSearch, edbgsptf, ppfcbTable, ppfcbSubTree, pobjidFDP, ppgnoFDP );
}

//  ================================================================
DEBUG_EXT( EDBGSetImplicitBT )
//  ================================================================
{
    ULONG objidSearch = objidNil;

    if ( argc == 0 || FIsHelpArg( argv[ 0 ] ) )
    {
        if ( !FIsHelpArg( argv[ 0 ] ) )
        {
            dprintf( "\nERROR: Must specify a table or objid in one of various forms.\n\n" );
        }
        dprintf( "Specify a table + index or sub-tree ... examples:\n" );
        dprintf( "    !ese .bt MSysObjects\n" );
        dprintf( "    !ese .bt MSysObjects\\Name   (for a secondary index)\n" );
        dprintf( "    !ese .bt MSysObjects[OE]\n" );
        dprintf( "    !ese .bt MSysObjects[AE]\n" );
        dprintf( "    !ese .bt MSysObjects\\(LV)\n" );
        dprintf( "    !ese .bt MSysObjects\\Name[AE]   (AE tree of a secondary index)\n" );
        dprintf( "    !ese .bt 1      (note an objidFDP)\n" );
        dprintf( "    !ese .bt 1[OE]  (yes that works!  And all other sub-tree options)\n" );
        dprintf( "    !ese .bt *SysObjects\\Name[AE]   (Yes that works too)\n" );
        dprintf( "    Note: If a client has a table name that starts with a number, we'll have to adjust this policy.\n" );
        dprintf( "\n" );

        if ( Pdls()->ObjidCurrentBt() != 0 )
        {
            dprintf( "Note:  Current implicit BT -->  ifmp:0x%x, objidFDP:0x%x, pgnoFDP:0x%x, pfcbDebuggee(s)=%p / %p\n", 
                 Pdls()->IfmpCurrent(), Pdls()->ObjidCurrentBt(), Pdls()->PgnoCurrentBt(), 
                 Pdls()->PfcbCurrentTableDebuggee(), Pdls()->PfcbCurrentSubTreeDebuggee() );
        }
        else
        {
            dprintf( "Note:  Current implicit BT is unset\n\n" );
        }

         return;
    }

    if ( FGenUlFromSz( argv[0], &objidSearch ) )
    {
        dprintf( "Read objidTree = %d ... is that right? \n\n", objidSearch );
    }

    CHAR szSearchParts[ 300 ];  //  this is just much > than two catalog names, ?64 chars?
    OSStrCbCopyA( szSearchParts, sizeof( szSearchParts ), argv[0] );

    CHAR * szIndexName = strchr( szSearchParts, '\\' );
    CHAR * szSpecialTree = strchr( szSearchParts, '[' );
    BOOL fLV = fFalse;
    BOOL fOE = fFalse;
    BOOL fAE = fFalse;

    if ( szIndexName )
    {
        szIndexName[ 0 ] = '\0';
        szIndexName++;

        fLV = FArgumentMatch( szIndexName, szLvIndexSelect ); 
    }
    if ( szSpecialTree )
    {
        szSpecialTree[ 0 ] = '\0';
        szSpecialTree++;
        CHAR * szEndDelim = strchr( szSpecialTree, ']' );
        AssertEDBGSz( szEndDelim, "Expected end delimiter of ]" );
        if ( szEndDelim )
        {
            szEndDelim[ 0 ] = '\0';
        }
        fOE = FArgumentMatch( szSpecialTree, "OE" );
        fAE = FArgumentMatch( szSpecialTree, "AE" );
        if ( !fOE && !fAE )
        {
            dprintf( "ERROR: Unexpected special tree specifier.\n" );
            return;
        }
        //  LV is handled as subtree.
    }

    CHAR * szTableName = szSearchParts;
    // Should be no more special / delimiters left in table name, except wildcard *.
    AssertEDBG( NULL == strchr( szTableName, '\\' ) );
    AssertEDBG( NULL == strchr( szTableName, '[' ) );
    AssertEDBG( NULL == strchr( szTableName, ']' ) );

    dprintf( "Searching for table: %d | %hs + sub-object: %hs + sub-tree: %hs\n", objidSearch, szTableName, szIndexName, szSpecialTree );

    OBJID objidFDP = objidNil;
    PGNO pgnoFDP = pgnoNull;
    FCB * pfcbTable = pfcbNil;
    FCB * pfcbSubTree = pfcbNil;

    if ( objidSearch != objidNil )
    {
        (void)FEDBGFindAndSetImplicitBt(
                 Pdls()->IpinstCurrent(),
                 Pdls()->IfmpCurrent(),
                 objidSearch,
                 szIndexName,
                 fOE ? edbgsptfOwnedExtent : ( fAE ? edbgsptfAvailExtent : edbgsptfNone ),
                 &pfcbTable,
                 &pfcbSubTree,
                 &objidFDP,
                 &pgnoFDP );
    }
    else
    {
        (void)FEDBGFindAndSetImplicitBt(
                 Pdls()->IpinstCurrent(),
                 Pdls()->IfmpCurrent(),
                 szTableName,
                 szIndexName,
                 fOE ? edbgsptfOwnedExtent : ( fAE ? edbgsptfAvailExtent : edbgsptfNone ),
                 &pfcbTable,
                 &pfcbSubTree,
                 &objidFDP,
                 &pgnoFDP );
    }
}


//  ================================================================
DEBUG_EXT( EDBGTest )
//  ================================================================
{
    dprintf( "================================================================\n" );
    dprintf( "fDebugMode = %d\n", fDebugMode );
    dprintf( "g_fTryInitEDBGGlobals = %d\n", g_fTryInitEDBGGlobals );
    dprintf( "\n" );

    #define DprintStructSize( str )  \
        dprintf( "sizeof %5hs = %4d (0x%04x) ... %hs uses %5.2f cachelines.\n", SzType( ((str**)NULL) ), sizeof(str), sizeof(str), #str, ( (double)sizeof(str) / (double)64 ) );

    DprintStructSize( INST );
    DprintStructSize( LOG );
    DprintStructSize( VER );
    DprintStructSize( FMP );
    dprintf( "\n" );
    DprintStructSize( FCB );
    DprintStructSize( TDB );
    DprintStructSize( IDB );
    DprintStructSize( SCB );
    dprintf( "\n" );
    DprintStructSize( PIB );
    DprintStructSize( FUCB );
    DprintStructSize( CSR );
    dprintf( "\n" );
    DprintStructSize( CSR );
    DprintStructSize( BF );
    dprintf( "\n" );

    dprintf( "Current global table address = 0x%N\n", g_rgEDBGGlobalsDebugger );
    dprintf( "\n" );

    if( NULL != g_hLibrary )
    {
        dprintf( "Library has been loaded internally.\n" );
    }
    else
    {
        dprintf( "Library has NOT been loaded internally.\n" );
    }
    dprintf( "\n" );

    dprintf( "Implicit / set contexts\n" );
    dprintf( "    IfmpCurrent()    = %d   (cbPage = %d)\n", Pdls()->IfmpCurrent(), Pdls()->IfmpCurrent() != ifmpNil ? Pdls()->CbPage() : 0 );
    dprintf( "    IpinstCurrent()  = %d   (cbPage = %d)\n", Pdls()->IpinstCurrent(), Pdls()->IpinstCurrent() != DEBUGGER_LOCAL_STORE::IpinstNil() ? Pdls()->CbPage() : 0 );
    dprintf( "    *CurrentBt*()    = %d   (pgno, objid=%d, pfcbT=%p, pfcbS=%p\n", Pdls()->PgnoCurrentBt(), Pdls()->ObjidCurrentBt(), Pdls()->PfcbCurrentTableDebuggee(), Pdls()->PfcbCurrentSubTreeDebuggee() );
    dprintf( "\n" );

    dprintf( "[Cached]DatabasePageSize = %d.\n", Pdls()->CbPage() );
    dprintf( "\n" );

    if ( argc >= 1 )
    {
        void* pv;

        dprintf( "FAddressFromGlobal( %s, &pv ) = ", argv[0] );
        if ( FAddressFromGlobal( argv[ 0 ], &pv ) )
        {
            dprintf( "true. pv = 0x%N.\n", pv );
        }
        else
        {
            dprintf( "false. Could not find the symbol.\n" );
        }
        dprintf( "\n" );

        dprintf( "GetExpression( %s ) = ", argv[0] );
        if ( pv = (void*) GetExpression( argv[0] ) )
        {
            dprintf( "0x%N\n", pv );
        }
        else
        {
            dprintf( "NULL (couldn't evaluate).\n" );
        }
        dprintf( "\n" );

    }

    if ( argc >= 2 )
    {
        void* pv;
        if ( FAddressFromSz( argv[ 1 ], &pv ) )
        {
            char        szGlobal[ 1024 ];
            DWORD_PTR   dwOffset;
            if ( FSymbolFromAddress( pv, szGlobal, sizeof( szGlobal ), &dwOffset ) )
            {
                dprintf(    "The symbol closest to 0x%N is %s+0x%p.\n",
                            pv,
                            szGlobal,
                            QWORD( dwOffset ) );
            }
            else
            {
                dprintf( "Could not map this address to a symbol.\n" );
            }
        }
        else
        {
            dprintf( "That is not a valid address.\n" );
        }
    }

    dprintf( "================================================================\n" );
    dprintf( "\n" );
}

//  ================================================================
BOOL FLoadEDBGGlobals( EDBGGlobals * pEGT )
//  ================================================================
{
    FetchWrap<EDBGGlobals *>    pEG;
    FetchWrap<SIZE_T *>         pcArraySize;
    FetchWrap<const char *>     szName;

    if ( !pEG.FVariable( pEGT )
            || !pcArraySize.FVariable( (SIZE_T *)pEG->pvData )
            || !szName.FSz( pEG->szName )
            || 0 != _stricmp( szName, "cEDBGGlobals" ) )
    {
        return fFalse;
    }
    g_rgEDBGGlobalsDebugger = pEGT;
    
    return fTrue;
}

//  ================================================================
DEBUG_EXT( EDBGGlobal )
//  ================================================================
{
    FetchWrap<EDBGGlobals *>    pEG;
    FetchWrap<SIZE_T *>         pcArraySize;
    FetchWrap<const char *>     szName;
    const char *                szDebuggerCommand;

    if ( argc > 1 || ( 0 == argc && NULL == g_rgEDBGGlobalsDebugger ) )
    {
        dprintf( "Usage: GLOBALS [<rgEDBGGlobals>]\n" );
        dprintf( "    Provides a pointer to a debuggee table of globals when the symbols are not present.\n" );
        dprintf( "    You can get that pointer from m_rgEDBGGlobals when you dump an instance.\n" );
        dprintf( "    Without parameters it will dump the current global table.\n" );
    }
    else if ( 1 == argc )
    {
        EDBGGlobals *pEGT;
        if ( FAddressFromSz( argv[0], &pEGT ) )
        {
            if ( NULL != pEGT &&
                    !FLoadEDBGGlobals( pEGT ) )
            {
                dprintf( "Invalid global table pointer (0x%N)\n", pEGT );
            }
            else
            {
                dprintf( "Global table address = 0x%N.\n", g_rgEDBGGlobalsDebugger );
            }
        }
        else
        {
            dprintf( "Cannot obtain global table address.\n" );
        }
    }
    else
    {
        FCallR( pEG.FVariable( g_rgEDBGGlobalsDebugger )
                && pcArraySize.FVariable( (SIZE_T *)pEG->pvData )
                && szName.FSz( pEG->szName )
                && 0 == strcmp( szName, "cEDBGGlobals" ),
                ( "Cannot fetch global table pointer (0x%N)\n", g_rgEDBGGlobalsDebugger ) );
        FCallR( pEG.FVariable( g_rgEDBGGlobalsDebugger, *pcArraySize ),
                ( "Cannot fetch the whole global table (0x%N) with %i(0x%X) entries.\n",
                    g_rgEDBGGlobalsDebugger, (INT)*pcArraySize - 1, (INT)*pcArraySize - 1 ) );
        dprintf( "Debuggee global table 0x%N has %i(0x%X) entries.\n",
                g_rgEDBGGlobalsDebugger, (INT)*pcArraySize - 1, (INT)*pcArraySize - 1 );
        for ( SIZE_T i = 1; i < *pcArraySize; i++ )
        {
            FCallR( szName.FSz( pEG[i].szName ),
                ("    *** Cannot dump to the end.\n") );
            szDebuggerCommand = rgEDBGGlobals[i].szDebuggerCommand;
            const ULONG64 pvData = (ULONG64) pEG[i].pvData;
            if ( szDebuggerCommand == NULL )
            {
                dprintf( "    %-16s 0x%N\n", szName, pEG[i].pvData );
            }
            else
            {
                EDBGPrintfDml( "    %-16s <link cmd=\"%s poi(0x%p)\">0x%N</link>\n",
                    szName, szDebuggerCommand, pvData, pEG[i].pvData );
            }
        }
    }
}


//  ================================================================
DEBUG_EXT( EDBGLoad )
//  ================================================================
{
    g_hLibrary = LoadLibraryExW( WszUtilImagePath(), NULL, 0 );
    if ( NULL != g_hLibrary )
    {
        dprintf( "Loaded %ws!\n", WszUtilImagePath() );
        if ( 1 == argc )
        {
            EDBGGlobal( pdebugClient, argc, argv );
        }
    }
    else
    {
        dprintf( "Unable to load %ws!\n", WszUtilImagePath() );
    }
}


//  ================================================================
DEBUG_EXT( EDBGUnload )
//  ================================================================
{
    if( NULL != g_hLibrary )
    {
        FreeLibrary( g_hLibrary );
    }
    else
    {
        dprintf( "%ws not loaded!", WszUtilImagePath() );
    }
}


//  ================================================================
DEBUG_EXT( EDBGErr )
//  ================================================================
{
    LONG lErr;
    if( 1 == argc
        && FGenUlFromSz( argv[0], (ULONG *)&lErr ) )
    {
        const CHAR * szError = "";
        const CHAR * szErrorText = "";
#ifndef MINIMAL_FUNCTIONALITY
        JetErrorToString( lErr, &szError, &szErrorText );
#endif
        dprintf( "0x%x, %d: %s (%s)\n", lErr, lErr, szError, szErrorText );
    }
    else
    {
        dprintf( "Usage: ERR <error>\n" );
    }
}


//  ================================================================
DEBUG_EXT( EDBGHelp )
//  ================================================================
{
    INT ifuncmap;
    for( ifuncmap = 0; ifuncmap < cfuncmap; ifuncmap++ )
    {
        EDBGPrintfDml( "\t%s\n", rgfuncmap[ifuncmap].szHelp );
    }
    dprintf( "\n--------------------\n\n" );
}


namespace OSSYNC
{
    VOID OSSYNCAPI OSSyncDebuggerExtension(
        PDEBUG_CLIENT pdebugClient,
        const INT argc,
        const CHAR * const argv[] );
};


//  ================================================================
DEBUG_EXT( EDBGSync )
//  ================================================================
{
    OSSyncDebuggerExtension(    pdebugClient,
                                argc,
                                argv );
}


DEBUG_EXT( EDBGCacheFind )
//  ================================================================
{
    ULONG       ifmp                    = 0;        //  don't use ifmpNil because its type is IFMP
    ULONG       pgno                    = pgnoNull;
    PBF         pbf                     = pbfNil;
    BF **       rgpbfChunkDebuggee      = NULL;
    BF **       rgpbfChunkT             = NULL;
    ULONG       cbfChunkDebuggee        = 0;
    LONG_PTR    cbfChunkT               = 0;
    ULONG       cbfCacheAddressableDebuggee = 0;
    LONG_PTR    cbfCacheAddressableT            = 0;
    BOOL        fValidUsage;

    switch ( argc )
    {
        case 2:
            ifmp = FAutoFmp( argv[0] ) ? Pdls()->IfmpCurrent() : (ULONG) GetExpression( argv[0] );
            pgno = (ULONG) GetExpression( argv[1] );
            fValidUsage = ( pgno > 0 );
            break;

        case 5:
            ifmp = FAutoFmp( argv[0] ) ? Pdls()->IfmpCurrent() : (ULONG) GetExpression( argv[0] );
            pgno = (ULONG) GetExpression( argv[1] );
            cbfChunkDebuggee = (ULONG) GetExpression( argv[3] );
            cbfCacheAddressableDebuggee = (ULONG) GetExpression( argv[4] );
            fValidUsage = ( pgno > 0
                            && FAddressFromSz( argv[2], &rgpbfChunkDebuggee ) );
            break;

        default:
            fValidUsage = fFalse;
            break;
    }

    dprintf( "\n" );

    if ( !fValidUsage )
    {
        dprintf( "Usage: CACHEFIND <ifmp|.> <pgno> [<g_rgpbfChunk> <g_cbfChunk> <cbfCacheAddressable>]\n\n" );
        dprintf( "    <ifmp> is the index to the FMP entry for the desired database file\n" );
        dprintf( "    <pgno> is the desired page number from this FMP\n\n" );
        return;
    }

    if ( NULL == rgpbfChunkDebuggee )
    {
        if ( !FReadGlobal( "g_cbfChunk", &cbfChunkT )
            || !FReadGlobal( "cbfCacheAddressable", &cbfCacheAddressableT )
            || !FReadGlobal( "g_rgpbfChunk", &rgpbfChunkDebuggee ) )
        {
            dprintf( "Error: Could not load BF parameters.\n\n" );
            return;
        }
    }
    else
    {
        cbfChunkT = cbfChunkDebuggee;
        cbfCacheAddressableT = cbfCacheAddressableDebuggee;
    }

    if ( !FFetchVariable( rgpbfChunkDebuggee, &rgpbfChunkT, cCacheChunkMax ) )
    {
        dprintf( "Error: Could not load BF parameters.\n\n" );
        return;
    }

    pbf = (PBF)LocalAlloc( 0, sizeof(BF) );
    if ( NULL == pbf )
    {
        dprintf( "Error: Could not allocate BF buffer.\n" );
        Unfetch( rgpbfChunkT );
        return;
    }

    //  scan all valid BFs looking for this IFMP / PGNO

    BOOL fFoundBF = fFalse;
    for ( LONG_PTR ibf = 0; ibf < cbfCacheAddressableT; ibf++ )
    {
        //  compute the address of the target BF

        PBF pbfDebuggee = rgpbfChunkT[ ibf / cbfChunkT ] + ibf % cbfChunkT;

        //  we failed to read this BF

        if ( !FReadVariable( pbfDebuggee, pbf ) )
        {
            dprintf( "Error: Could not read BF at 0x%N. Aborting.\n", pbfDebuggee );
            fFoundBF = fTrue;
            break;
        }

        //  this BF contains this IFMP / PGNO

        if (    pbf->ifmp == IFMP( ifmp ) &&
                pbf->pgno == PGNO( pgno ) )
        {
            EDBGPrintfDml(  "%X:%08X is cached in BF <link cmd=\"!ese dump bf 0x%N\">0x%N</link> ( Version = %s, pv = <link cmd=\"!ese dump page %#x 0x%N ht\">0x%N</link> ).\n",
                        ifmp,
                        pgno,
                        pbfDebuggee,
                        pbfDebuggee,
                        ( pbf->fCurrentVersion ? "Current" : ( pbf->fOlderVersion ? "Older" : "<null>" ) ),
                        pgno,
                        pbf->pv,
                        pbf->pv );
            fFoundBF = fTrue;
        }
    }

    //  we did not find the IFMP / PGNO

    if ( fFoundBF )
    {
        dprintf( "\n" );
    }
    else
    {
        dprintf( "%X:%08X is not cached.\n\n", ifmp, pgno );
    }

    //  unload BF parameters

    LocalFree( pbf );
    Unfetch( rgpbfChunkT );
}

DEBUG_EXT( EDBGCacheFindOldest )
//  ================================================================
{
    ULONG       ifmp                    = 0;        //  don't use ifmpNil because its type is IFMP
    ULONG       lgen                    = 0;
    PBF         pbf                     = pbfNil;
    BF **       rgpbfChunkDebuggee      = NULL;
    BF **       rgpbfChunkT             = NULL;
    ULONG       cbfChunkDebuggee        = 0;
    LONG_PTR    cbfChunkT               = 0;
    ULONG       cbfCacheAddressableDebuggee = 0;
    LONG_PTR    cbfCacheAddressableT            = 0;
    BOOL        fValidUsage;
    PBF         pbfOldestBegin0         = pbfNil;
    LGPOS       lgposOldestBegin0       = lgposMax;
    BOOL        fFoundTargetGen         = fFalse;

    switch ( argc )
    {
        case 1:
            fValidUsage = ( FAutoIfmpFromSz( argv[0], &ifmp ) );
            break;

        case 2:
            fValidUsage = ( FAutoIfmpFromSz( argv[0], &ifmp )
                            && FUlFromSz( argv[1], &lgen )
                            && lgen > 0 );
            break;

        case 4:
            fValidUsage = ( FAutoIfmpFromSz( argv[0], &ifmp )
                            && FAddressFromSz( argv[2], &rgpbfChunkDebuggee )
                            && FUlFromSz( argv[3], &cbfChunkDebuggee )
                            && FUlFromSz( argv[4], &cbfCacheAddressableDebuggee ) );
            break;

        case 5:
            fValidUsage = ( FAutoIfmpFromSz( argv[0], &ifmp )
                            && FUlFromSz( argv[1], &lgen )
                            && lgen > 0
                            && FAddressFromSz( argv[2], &rgpbfChunkDebuggee )
                            && FUlFromSz( argv[3], &cbfChunkDebuggee )
                            && FUlFromSz( argv[4], &cbfCacheAddressableDebuggee ) );
            break;

        default:
            fValidUsage = fFalse;
            break;
    }

    dprintf( "\n" );

    if ( !fValidUsage )
    {
        dprintf( "Usage: CACHEFINDOLDEST <ifmp|.> [<gen>] [<g_rgpbfChunk> <g_cbfChunk> <cbfCacheAddressable>]\n" );
        dprintf( "\n" );
        dprintf( "    <ifmp|.> - index to the FMP entry for the desired database file\n" );
        dprintf( "    <gen>    - find all BF's with lgposBegin0 less than or equal to\n" );
        dprintf( "               specified log generation\n" );
        return;
    }

    if ( NULL == rgpbfChunkDebuggee )
    {
        if ( !FReadGlobal( "g_cbfChunk", &cbfChunkT )
            || !FReadGlobal( "cbfCacheAddressable", &cbfCacheAddressableT )
            || !FReadGlobal( "g_rgpbfChunk", &rgpbfChunkDebuggee ) )
        {
            dprintf( "Error: Could not load BF parameters.\n" );
            return;
        }
    }
    else
    {
        cbfChunkT = cbfChunkDebuggee;
        cbfCacheAddressableT = cbfCacheAddressableDebuggee;
    }

    if ( !FFetchVariable( rgpbfChunkDebuggee, &rgpbfChunkT, cCacheChunkMax ) )
    {
        dprintf( "Error: Could not load BF parameters.\n" );
        return;
    }

    pbf = (PBF)LocalAlloc( 0, sizeof(BF) );
    if ( NULL == pbf )
    {
        dprintf( "Error: Could not allocate BF buffer.\n" );
        goto HandleError;
    }

    //  scan all valid BFs looking for the oldest lgposBegin0 of this IFMP

    if ( lgen > 0 )
    {
        dprintf(
            "Scanning all BF's for ifmp 0x%x with lgposBegin0 less than or equal to generation %d (0x%x):\n",
            ifmp,
            lgen,
            lgen );
    }
    else
    {
        dprintf( "Scanning %d (0x%x) BF's...\n", cbfCacheAddressableT, cbfCacheAddressableT );
    }

    for ( LONG_PTR ibf = 0; ibf < cbfCacheAddressableT; ibf++ )
    {
        //  compute the address of the target BF

        PBF pbfDebuggee = rgpbfChunkT[ ibf / cbfChunkT ] + ibf % cbfChunkT;

        //  we failed to read this BF

        if ( !FReadVariable( pbfDebuggee, pbf ) )
        {
            dprintf( "Error: Could not read BF at 0x%N. Aborting.\n", pbfDebuggee );
            goto HandleError;
        }

        //  compare lgposBegin0 of this IFMP / PGNO

        if ( pbf->ifmp == IFMP( ifmp )
            && ( pbf->fCurrentVersion || pbf->fOlderVersion ) )
        {
            if ( lgen > 0 )
            {
                if ( (ULONG)pbf->lgposOldestBegin0.lGeneration <= lgen )
                {
                    dprintf(
                        "    BF 0x%N: gen %d (0x%x)\n",
                        pbfDebuggee,
                        pbf->lgposOldestBegin0.lGeneration,
                        pbf->lgposOldestBegin0.lGeneration );
                    fFoundTargetGen = fTrue;
                }
            }

            if ( CmpLgpos( &pbf->lgposOldestBegin0, &lgposOldestBegin0 ) < 0 )
            {
                lgposOldestBegin0 = pbf->lgposOldestBegin0;
                pbfOldestBegin0 = pbfDebuggee;
            }
        }
    }

    //  report BF with oldest Begin0

    if ( lgen > 0 && !fFoundTargetGen )
    {
        //  didn't find any BF for this ifmp
        //
        dprintf( "    <none>\n" );
    }

    if ( pbfNil != pbfOldestBegin0 )
    {
        dprintf(
            "Oldest lgposBegin0 (generation 0x%x) for ifmp 0x%x is cached in BF 0x%N.\n",
            lgposOldestBegin0.lGeneration,
            ifmp,
            pbfOldestBegin0 );
    }
    else
    {
        dprintf( "No pages cached for ifmp 0x%x.\n", ifmp );
    }

    //  unload BF parameters
HandleError:
    dprintf( "\n" );

    LocalFree( pbf );
    Unfetch( rgpbfChunkT );
}

// -------------------------------------------------------------
//
//  Cache IterationQuery / ErrEDBGQueryCache() support 
//

//  "Special" LGPOS values that get interpeted at query runtime ...
//
//  All values should have same lGeneration and isec ...
// 
const static LGPOS      lgposEDBGSignalWaypoint     = { 0xff01, 0xfeff, 0x7fffffff };

BOOL LgposReadVal( const char * szLgpos, void ** ppvValue )
{
    BOOL    fRet = fFalse;

    LGPOS lgpos = { 0 };
    char * pchEnd = NULL;

    *ppvValue = Pdls()->PvDLSAlloc(sizeof(LGPOS));
    if ( *ppvValue == NULL )
    {
        dprintf( "Allocation failure.\n" );
        return fFalse;
    }

    if ( 0 == _stricmp( szLgpos, "lgposMin" ) )
    {
        lgpos = lgposMin;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szLgpos, "lgposMax" ) )
    {
        lgpos = lgposMax;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szLgpos, "lgposWaypoint" ) )
    {
        lgpos = lgposEDBGSignalWaypoint;
        fRet = fTrue;
    }
    else
    {
        lgpos.lGeneration = strtoul( szLgpos, &pchEnd, 16 );
        if ( pchEnd != NULL &&
            ( *pchEnd == ':' || *pchEnd == ',' ) )
        {
            lgpos.isec = (USHORT)strtoul( pchEnd+1, &pchEnd, 16 );

            if ( pchEnd != NULL &&
                ( *pchEnd == ':' || *pchEnd == ',' ) )
            {
                lgpos.ib = (USHORT)strtoul( pchEnd+1, &pchEnd, 16 );
                fRet = !( *pchEnd );
            }
        }
        
        AssertEDBGSz( 0 != CmpLgpos( &lgposEDBGSignalWaypoint, &lgpos ), "provided LGPOS matches lgposEDBGSignalWaypoint, this will most likely cause not the query you hoped for!" );
    }

    if ( fRet )
    {
        memcpy( *ppvValue, &lgpos, sizeof(LGPOS) );
    }
    else
    {
        Pdls()->DLSFree( *ppvValue );
        dprintf("Couldn't transform \"%s\" into a LGPOS.\n", szLgpos );
    }

    return fRet;
}

INT LgposExprEval( void * pVal1, void * pVal2 )
{
    if ( ((LGPOS*)pVal2)->isec != lgposEDBGSignalWaypoint.isec &&
            ((LGPOS*)pVal2)->lGeneration != lgposEDBGSignalWaypoint.lGeneration )
    {
        return CmpLgpos( (LGPOS*)pVal1, (LGPOS*) pVal2 );
    }

    if ( 0 == CmpLgpos( (LGPOS*)pVal2, &lgposEDBGSignalWaypoint ) )
    {
        const LGPOS lgposWaypoint = ( Pdls()->IfmpCurrent() == ifmpNil ) ?
                                        lgposMax :
                                        Pdls()->PfmpCache( Pdls()->IfmpCurrent() )->LgposWaypoint();
        return CmpLgpos( (LGPOS*)pVal1, &lgposWaypoint );
    }

    return CmpLgpos( (LGPOS*)pVal1, (LGPOS*) pVal2 );
}

void LgposPrintVal( const void * pVal )
{
    LGPOS lgpos = *(LGPOS*)pVal;
    dprintf(" %08x:%04x:%04x", lgpos.lGeneration, lgpos.isec, lgpos.ib );
}

BOOL RbsposReadVal( const char * szRbspos, void ** ppvValue )
{
    BOOL    fRet = fFalse;

    RBS_POS rbspos = { 0 };
    char * pchEnd = NULL;

    *ppvValue = Pdls()->PvDLSAlloc(sizeof(RBS_POS));
    if ( *ppvValue == NULL )
    {
        dprintf( "Allocation failure.\n" );
        return fFalse;
    }

    if ( 0 == _stricmp( szRbspos, "rbsposMin" ) )
    {
        rbspos = rbsposMin;
        fRet = fTrue;
    }
    else
    {
        rbspos.lGeneration = strtoul( szRbspos, &pchEnd, 16 );
        if ( pchEnd != NULL &&
            ( *pchEnd == ':' || *pchEnd == ',' ) )
        {
            rbspos.iSegment = (USHORT)strtoul( pchEnd+1, &pchEnd, 16 );
        }
    }

    if ( fRet )
    {
        memcpy( *ppvValue, &rbspos, sizeof(RBS_POS) );
    }
    else
    {
        Pdls()->DLSFree( *ppvValue );
        dprintf("Couldn't transform \"%s\" into a RBS_POS.\n", szRbspos );
    }

    return fRet;
}

INT RbsposExprEval( void * pVal1, void * pVal2 )
{
    return CmpRbspos( (RBS_POS*)pVal1, (RBS_POS*)pVal2 );
}

void RbsposPrintVal( const void * pVal )
{
    RBS_POS rbspos = *(RBS_POS*)pVal;
    dprintf(" %08x:%08x", rbspos.lGeneration, rbspos.iSegment );
}

BOOL BFATReadVal( const char * szBFAT, void ** ppvValue )
{
    BFAllocType bfat = bfatNone;
    BOOL fRet = fFalse;

    // Note this returns 4
    //dprintf("Fyi sizeof(BFAllocType) = %d\n", sizeof(BFAllocType));

    *ppvValue = Pdls()->PvDLSAlloc(sizeof(BFAllocType));

    if ( 0 == _stricmp( szBFAT, "none" ) ||
        0 == _stricmp( szBFAT, "bfatNone" ) )
    {
        bfat = bfatNone;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szBFAT, "frac" ) ||
            0 == _stricmp( szBFAT, "bfatFracCommit" ) )
    {
        bfat = bfatFracCommit;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szBFAT, "view" ) ||
            0 == _stricmp( szBFAT, "bfatViewMapped" ) )
    {
        bfat = bfatViewMapped;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szBFAT, "alloc" ) ||
            0 == _stricmp( szBFAT, "bfatPageAlloc" ) )
    {
        bfat = bfatPageAlloc;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szBFAT, "0" ) ||
                0 == _stricmp( szBFAT, "0x0" ))
    {
        bfat = bfatNone;
        fRet = fTrue;
    }
    else
    {
        ULONG ul = strtoul( szBFAT, NULL, 16 );
        if ( ul != 0 && ul < bfdfMax )
        {
            bfat = (BFAllocType) ul;
            fRet = fTrue;
        }
    }

    if ( fRet )
    {
        memcpy( *ppvValue, &bfat, sizeof(BFAllocType) );
    }
    else
    {
        Pdls()->DLSFree( *ppvValue );
        dprintf("Couldn't transform \"%s\" into a BFAllocType.\n", szBFAT );
    }

    return fRet;
}


void BFATPrintVal( const void * pVal )
{
    BFAllocType bfat = *(BFAllocType*)pVal;
    switch ( bfat )
    {
        case bfatNone:
            dprintf( "        None" );
            break;
        case bfatFracCommit:
            dprintf( "	FracCommit" );
            break;
        case bfatViewMapped:
            dprintf( "  ViewMapped" );
            break;
        case bfatPageAlloc:
            dprintf( "   PageAlloc" );
            break;
        default:
            dprintf( " Issue, unknown alloc type" );
            break;
    }
}



BOOL BFDFReadVal( const char * szBFDF, void ** ppvValue )
{
    BFDirtyFlags bfdf = bfdfClean;
    BOOL fRet = fFalse;

    // Note this returns 4
    //dprintf("Fyi sizeof(BFDirtyFlags) = %d\n", sizeof(BFDirtyFlags));

    *ppvValue = Pdls()->PvDLSAlloc(sizeof(BFDirtyFlags));

    if ( 0 == _stricmp( szBFDF, "clean" ) ||
        0 == _stricmp( szBFDF, "bfdfClean" ) )
    {
        bfdf = bfdfClean;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szBFDF, "untidy" ) ||
            0 == _stricmp( szBFDF, "bfdfUntidy" ) )
    {
        bfdf = bfdfUntidy;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szBFDF, "dirty" ) ||
            0 == _stricmp( szBFDF, "bfdfDirty" ) )
    {
        bfdf = bfdfDirty;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szBFDF, "filthy" ) ||
            0 == _stricmp( szBFDF, "bfdfFilthy" ) )
    {
        bfdf = bfdfFilthy;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szBFDF, "0" ) ||
                0 == _stricmp( szBFDF, "0x0" ))
    {
        bfdf = bfdfClean;
        fRet = fTrue;
    }
    else
    {
        ULONG ul = strtoul( szBFDF, NULL, 16 );
        if ( ul != 0 && ul < bfdfMax )
        {
            bfdf = (BFDirtyFlags) ul;
            fRet = fTrue;
        }
    }

    if ( fRet )
    {
        memcpy( *ppvValue, &bfdf, sizeof(BFDirtyFlags) );
    }
    else
    {
        Pdls()->DLSFree( *ppvValue );
        dprintf("Couldn't transform \"%s\" into a BFDirtyFlags.\n", szBFDF );
    }

    return fRet;
}


void BFDFPrintVal( const void * pVal )
{
    BFDirtyFlags bfdf = *(BFDirtyFlags*)pVal;
    switch ( bfdf )
    {
        case bfdfClean:
            dprintf( "  clean" );
            break;
        case bfdfUntidy:
            dprintf( " untidy" );
            break;
        case bfdfDirty:
            dprintf( "  dirty" );
            break;
        case bfdfFilthy:
            dprintf( " filthy" );
            break;
        default:
            dprintf( " Issue, unknown dirty flag" );
            break;
    }
}

BOOL BFRSReadVal( const char * szBFRS, void ** ppvValue )
{
    BFResidenceState bfrs = bfrsNotCommitted;
    BOOL fRet = fFalse;

    *ppvValue = Pdls()->PvDLSAlloc(sizeof(BFResidenceState));

    if ( 0 == _stricmp( szBFRS, "notcommitted" ) ||
        0 == _stricmp( szBFRS, "bfrsNotCommitted" ) ||
        0 == _stricmp( szBFRS, "NotCom" ) )
    {
        bfrs = bfrsNotCommitted;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szBFRS, "NewlyCommitted" ) ||
            0 == _stricmp( szBFRS, "bfrsNewlyCommitted" ) ||
            0 == _stricmp( szBFRS, "NewlyCom" ) )
    {
        bfrs = bfrsNewlyCommitted;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szBFRS, "NotResident" ) ||
            0 == _stricmp( szBFRS, "bfrsNotResident" ) ||
            0 == _stricmp( szBFRS, "NotResid" ) )
    {
        bfrs = bfrsNotResident;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szBFRS, "Resident" ) ||
            0 == _stricmp( szBFRS, "bfrsResident" ) ||
            0 == _stricmp( szBFRS, "Resident" ) )
    {
        bfrs = bfrsResident;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szBFRS, "0" ) ||
                0 == _stricmp( szBFRS, "0x0" ))
    {
        bfrs = bfrsNotCommitted;
        fRet = fTrue;
    }
    else
    {
        ULONG ul = strtoul( szBFRS, NULL, 16 );
        if ( ul != 0 && ul < bfrsMax )
        {
            bfrs = (BFResidenceState) ul;
            fRet = fTrue;
        }
    }

    if ( fRet )
    {
        memcpy( *ppvValue, &bfrs, sizeof(BFResidenceState) );
    }
    else
    {
        Pdls()->DLSFree( *ppvValue );
        dprintf("Couldn't transform \"%s\" into a BFResidenceState.\n", szBFRS );
    }

    return fRet;
}

void BFRSPrintVal( const void * pVal )
{
    BFResidenceState bfrs = *(BFResidenceState*)pVal;
    switch ( bfrs )
    {
        case bfrsNotCommitted:
            dprintf( "   NotCom" );
            break;
        case bfrsNewlyCommitted:
            dprintf( " NewlyCom" );
            break;
        case bfrsNotResident:
            dprintf( " NotResid" );
            break;
        case bfrsResident:
            dprintf( " Resident" );
            break;
        default:
            dprintf( " Issue, unknown residence state" );
            break;
    }
}


BOOL PGFTReadVal( const char * szPGFT, void ** ppvValue )
{
    CPAGE::PageFlushType pgft = CPAGE::pgftUnknown;
    BOOL fRet = fFalse;

    *ppvValue = Pdls()->PvDLSAlloc(sizeof(CPAGE::PageFlushType));

    if ( 0 == _stricmp( szPGFT, "unknown" ) ||
        0 == _stricmp( szPGFT, "pgftUnknown" ) )
    {
        pgft = CPAGE::pgftUnknown;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szPGFT, "rock" ) ||
            0 == _stricmp( szPGFT, "pgftRockWrite" ) )
    {
        pgft = CPAGE::pgftRockWrite;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szPGFT, "paper" ) ||
            0 == _stricmp( szPGFT, "pgftPaperWrite" ) )
    {
        pgft = CPAGE::pgftPaperWrite;
        fRet = fTrue;
    }
    else if ( 0 == _stricmp( szPGFT, "scissors" ) ||
            0 == _stricmp( szPGFT, "pgftScissorsWrite" ) )
    {
        pgft = CPAGE::pgftScissorsWrite;
        fRet = fTrue;
    }
    else
    {
        ULONG ul = strtoul( szPGFT, NULL, 16 );
        if ( ul != 0 && ul < CPAGE::pgftMax )
        {
            pgft = (CPAGE::PageFlushType) ul;
            fRet = fTrue;
        }
    }

    if ( fRet )
    {
        memcpy( *ppvValue, &pgft, sizeof(CPAGE::PageFlushType) );
    }
    else
    {
        Pdls()->DLSFree( *ppvValue );
        dprintf("Couldn't transform \"%s\" into a CPAGE::PageFlushType.\n", szPGFT );
    }

    return fRet;
}

void PGFTPrintVal( const void * pVal )
{
    CPAGE::PageFlushType pgft = *(CPAGE::PageFlushType*)pVal;
    switch ( pgft )
    {
        case CPAGE::pgftUnknown:
            dprintf( "  unknown" );
            break;
        case CPAGE::pgftRockWrite:
            dprintf( "     rock" );
            break;
        case CPAGE::pgftPaperWrite:
            dprintf( "    paper" );
            break;
        case CPAGE::pgftScissorsWrite:
            dprintf( " scissors" );
            break;
        default:
            dprintf( " Issue, unknown page flush type" );
            break;
    }
}

//
//  Virtual and BitField Member enumerators
//

enum eBFVirtualMembers
{
    eBFBFNONE = 0,

    //  first set of bit fields
    eBFBFfLazyIO,
    eBFBFfNewlyEvicted,
    eBFBFfQuiesced,
    eBFBFfAvailable,
    eBFBFfWARLatch,
    eBFBFbfdf,
    eBFBFfInOB0OL,
    eBFBFirangelock,
    eBFBFfCurrentVersion,
    eBFBFfOlderVersion,
    eBFBFfFlushed,
    eBFBFbfls,
    //  second (atomic) set of bit fields
    eBFBFFDependentPurged,
    eBFBFFImpedingCheckpoint,
    eBFBFFRangeLocked,
    //  third quasi-bit field
    eBFBFtce,
    //  fourth quasi-bit field
    eBFBFicbBuffer,
    eBFBFicbPage,
    eBFBFfSuspiciouslySlowRead,
    eBFBFfSyncRead,
    eBFBFbfat,
    eBFBFfAbandoned,

    //  hack to allow access to virtual member "ibf"
    eBFibf,

    //  LRU-K invasive context accessors
    eBFLRUKTickLastTouch,
    eBFLRUKTickLastTouchTime,
    eBFLRUKkLrukPool,
    eBFLRUKTick1,
    eBFLRUKTick2,
    eBFLRUKTickIndex,
    eBFLRUKTickIndexTarget,
    eBFLRUKTickIndexTime,
    eBFLRUKTickIndexTargetTime,
    eBFLRUKPctCachePriority,
    eBFLRUKFSuperColded,
    eBFLRUKFSuperColdedIndex,
    eBFLRUKfCorrelatedTouch,
    eBFLRUKDtickCorrelatedRange,
    eBFLRUKDtickTickKRange,
    eBFLRUKDtickMisIndexedRange,
    eBFLRUKFResourceLocked,

    //  SXWL member accessors
    eBFSXWLFLatched,
    eBFSXWLFSharedLatched,
    eBFSXWLFExclusiveLatched,
    eBFSXWLFWriteLatched,
    eBFSXWLCSharers,
    eBFSXWLCWaitSharedLatch,
    eBFSXWLCWaitExclusiveLatch,
    eBFSXWLCWaitWriteLatch,
    eBFSXWLCWaiters,

    //  page member accessors
    eBFCPAGEchecksum,
    eBFCPAGEdbtimeDirtied,
    eBFCPAGEpgnoPrev,
    eBFCPAGEpgnoNext,
    eBFCPAGEobjidFDP,
    eBFCPAGEcbFree,
    eBFCPAGEcbUncommittedFree,
    eBFCPAGEibMicFree,
    eBFCPAGEitagMicFree,
    eBFCPAGEfFlags,
    //  extended (large) page member accessors
    eBFCPAGEpgno,
    eBFCPAGErgChecksum2,
    eBFCPAGErgChecksum3,
    eBFCPAGErgChecksum4,
    //  page flags accessors
    eBFCPAGEFLeafPage,
    eBFCPAGEFInvisibleSons,
    eBFCPAGEFRootPage,
    eBFCPAGEFFDPPage,
    eBFCPAGEFEmptyPage,
    eBFCPAGEFPreInitPage,
    eBFCPAGEFParentOfLeaf,
    eBFCPAGEFSpaceTree,
    eBFCPAGEFScrubbed,
    eBFCPAGEFRepairedPage,
    eBFCPAGEFPrimaryPage,
    eBFCPAGEFIndexPage,
    eBFCPAGEFNonUniqueKeys,
    eBFCPAGEFLongValuePage,
    eBFCPAGEFNewRecordFormat,
    eBFCPAGEFNewChecksumFormat,
    eBFCPAGEPgft,
    eBFCPAGECbFreeActual,   //  adjusts for dehydrated buffers by adding the dehydrated space to eBFCPAGEcbFree
    eBFCPAGECbContiguousDehydrateRequired,
    eBFCPAGECbReorganizedDehydrateRequired,

    // virtual members
    eBFVIRTFBFIDatabasePage,
    eBFVIRTCrceUndoInfoNext,
    eBFVIRTCbfTimeDepChainNext,
    eBFVIRTCbfTimeDepChainPrev,
    eBFVIRTCbfTimeDepChain,
    eBFVIRTFExcludedFromDump,
};


//
//  Member extraction routines
//

ERR ErrBFBitFieldElemFromStruct( size_t eBFMember, size_t cbUnused, QwEntryAddr pvDebuggee, PcvEntry pv, ULONGLONG * pullValue )
{
    ULONGLONG ullValue1 = 0x0;

    AssertEDBG( pullValue );
    *pullValue = 0xBAADF00D;

    const PBF pbfDebuggee = reinterpret_cast<const PBF>(pvDebuggee);
    const PBF pbf         = (const PBF)pv;

    switch ( eBFMember )
    {

        #define ExtrudeBFBitField( fBFFlag )                \
            case eBFBF##fBFFlag:                            \
                ullValue1 = pbf->fBFFlag ? fTrue : fFalse;  \
                break;

        #define ExtrudeBFAtomicBitField( fBFFlag )          \
            case eBFBF##fBFFlag:                            \
                ullValue1 = pbf->bfbitfield.fBFFlag();      \
                break;

        //  first set of bit fields
        ExtrudeBFBitField( fLazyIO )
        ExtrudeBFBitField( fNewlyEvicted )
        ExtrudeBFBitField( fQuiesced )
        ExtrudeBFBitField( fAvailable )
        ExtrudeBFBitField( fWARLatch )
        ExtrudeBFBitField( fInOB0OL )
        case eBFBFirangelock:
            ullValue1 = (unsigned __int64) pbf->irangelock;
            break;

        ExtrudeBFBitField( fCurrentVersion )
        ExtrudeBFBitField( fOlderVersion )
        ExtrudeBFBitField( fFlushed )

        //  second (atomic) set of bit fields
        ExtrudeBFAtomicBitField( FDependentPurged )
        ExtrudeBFAtomicBitField( FImpedingCheckpoint )
        ExtrudeBFAtomicBitField( FRangeLocked )

        ExtrudeBFBitField( fSuspiciouslySlowRead )
        ExtrudeBFBitField( fSyncRead )

        case eBFBFbfat:
            ullValue1 = (unsigned __int64) pbf->bfat;
            break;

        ExtrudeBFBitField( fAbandoned )

        //  these three are special, in that they're not booleans, but still bit fields.
        case eBFBFbfdf:
            ullValue1 = (unsigned __int64) pbf->bfdf;
            break;
        case eBFBFbfls:
            ullValue1 = (unsigned __int64) pbf->bfls;
            break;
        case eBFBFtce:
            ullValue1 = (unsigned __int64) pbf->tce;
            break;
        case eBFBFicbBuffer:
            ullValue1 = (unsigned __int64) pbf->icbBuffer;
            break;
        case eBFBFicbPage:
            ullValue1 = (unsigned __int64) pbf->icbPage;
            break;


        //  other odd cases ...
        case eBFibf:
            ullValue1 = Pdls()->IbfBFCachePbf( pbfDebuggee );
            break;

        //  LRU-K invasive context accessors
        case eBFLRUKTickLastTouch:
            ullValue1 = pbf->lrukic.TickLastTouch();
            break;
        case eBFLRUKTickLastTouchTime:
            ullValue1 = pbf->lrukic.TickLastTouchTime();
            break;
        case eBFLRUKkLrukPool:
            if ( pbf->err == errBFIPageNotVerified )
            {
                //  SOMEONE had a clever idea of defining pre-read unused as k=0th pool, hard
                //  to define in resmgr as it has no access to err, but we can define it for
                //  cache query!
                ullValue1 = 0;
            }
            else
            {
                ullValue1 = pbf->lrukic.kLrukPool();
            }
            break;
        case eBFLRUKTick1:
            ullValue1 = pbf->lrukic.TickKthTouch( 1 );
            break;
        case eBFLRUKTick2:
            ullValue1 = pbf->lrukic.TickKthTouch( 2 );
            break;
        case eBFLRUKTickIndex:
            ullValue1 = pbf->lrukic.TickIndex();
            break;
        case eBFLRUKTickIndexTarget:
            ullValue1 = pbf->lrukic.TickIndexTarget();
            break;
        case eBFLRUKTickIndexTime:
            ullValue1 = pbf->lrukic.TickIndexTime();
            break;
        case eBFLRUKTickIndexTargetTime:
            ullValue1 = pbf->lrukic.TickIndexTargetTime();
            break;
        case eBFLRUKPctCachePriority:
            ullValue1 = pbf->lrukic.PctCachePriority();
            break;
        case eBFLRUKFSuperColded:
            ullValue1 = pbf->lrukic.FSuperColded();
            break;
        case eBFLRUKFSuperColdedIndex:
            ullValue1 = pbf->lrukic.FSuperColdedIndex();
            break;
        case eBFLRUKfCorrelatedTouch:
            ullValue1 = pbf->lrukic.FCorrelatedTouch();
            break;
        case eBFLRUKDtickCorrelatedRange:
            ullValue1 = DtickDelta( pbf->lrukic.TickKthTouch( 1 ), pbf->lrukic.TickLastTouch() );   //  generally comes out positive
            break;
        case eBFLRUKDtickTickKRange:
            ullValue1 = DtickDelta( pbf->lrukic.TickKthTouch( 1 ), pbf->lrukic.TickKthTouch( 2 ) ); //  comes out positive
            break;
        case eBFLRUKDtickMisIndexedRange:
            ullValue1 = DtickDelta( pbf->lrukic.TickIndexTarget(), pbf->lrukic.TickIndex() );       //  generally comes out zero, or negative
            break;
        case eBFLRUKFResourceLocked:
            ullValue1 = pbf->lrukic.FResourceLocked();
            break;

        case eBFBFNONE:
        default:
            dprintf("Unrecognized BF bit-field type\n");
            AssertEDBG( !"Unrecognized BF bit-field type." );
            return ErrERRCheck( JET_errInternalError );

    }

    *pullValue = ullValue1;

    return JET_errSuccess;
}

ERR ErrBFPageElemFromStruct( size_t eBFMember, size_t cbUnused, QwEntryAddr pvDebuggee, PcvEntry pv, ULONGLONG * pullValue )
{
    ULONGLONG ullValue1 = 0x0;

    AssertEDBG( pullValue );
    *pullValue = 0xBAADF00D;

    const PBF pbf         = (const PBF)pv;

    if ( pbf->ifmp == 0x7fffffff ||
            pbf->ifmp == 0xffffffff ||
            (QWORD)pbf->ifmp == 0x7fffffffffffffff ||
            (QWORD)pbf->ifmp == 0xffffffffffffffff ||
            Pdls()->FIsTempDB( pbf->ifmp ) )
    {
        Pdls()->AddWarning( "INFO: Some elements / buffers were not evaluated due to them not being a Database Page type buffer.\n" );
        return errNotFound;
    }

    CPAGE::PGHDR * ppghdr = NULL;
#ifdef SLOW_BUT_SURE_REFETCH_PAGE_EACH_TIME_WAY
    if ( ( Pdls()->CbPage() < 16 * 1024 ) ?
            !FFetchVariable( (CPAGE::PGHDR*)pbf->pv, &ppghdr ) :
            !FFetchVariable( (CPAGE::PGHDR2*)pbf->pv, (CPAGE::PGHDR2**)&ppghdr ) )
    {
        Pdls()->AddWarning( "ERROR: Could not fetch page header state for query (skinny dump?), results may be incorrect.\n" );
        return errBFPageNotCached;  // reusing an existing error ...
    }
#else
    //  FLatchPageImage() actually knows to get a PGHDR2 if we've a 16+ KB page size
    if ( !Pdls()->FLatchPageImage( DEBUGGER_LOCAL_STORE::ePageHdrOnly, pbf->pv, &ppghdr ) )
    {
        Pdls()->AddWarning( "ERROR: Could not fetch page header state for query (skinny dump?), results may be incorrect.\n" );
        return errBFPageNotCached;  // reusing an existing error ...
    }
#endif

    switch ( eBFMember )
    {

        #define ExtrudeCPAGEElem( cpagefield )      \
            case eBFCPAGE##cpagefield:              \
                ullValue1 = ppghdr->cpagefield;     \
                break;

        //  page member accessors
        ExtrudeCPAGEElem( checksum );
        ExtrudeCPAGEElem( dbtimeDirtied );
        ExtrudeCPAGEElem( pgnoPrev );
        ExtrudeCPAGEElem( pgnoNext );
        ExtrudeCPAGEElem( objidFDP );
        ExtrudeCPAGEElem( cbFree );
        ExtrudeCPAGEElem( cbUncommittedFree );
        ExtrudeCPAGEElem( ibMicFree );
        ExtrudeCPAGEElem( itagMicFree );
        ExtrudeCPAGEElem( fFlags );

        //  extended (large) page member accessors
        case eBFCPAGECbFreeActual:
        {
            const USHORT cbFreeActual = (USHORT)( ppghdr->cbFree + ( Pdls()->CbPage() - g_rgcbPageSize[pbf->icbBuffer] ) );
            AssertEDBG( cbFreeActual >= ppghdr->cbFree );
            AssertEDBG( Pdls()->CbPage() >= (size_t)g_rgcbPageSize[pbf->icbBuffer] );
            AssertEDBG( cbFreeActual <= 32 * 1024 );
            ullValue1 = cbFreeActual;
        }
            break;
        case eBFCPAGECbContiguousDehydrateRequired:
        {
            CPAGE cpage;
            cpage.LoadPage( ppghdr, g_rgcbPageSize[pbf->icbBuffer] );
            ullValue1 = cpage.CbContiguousDehydrateRequired_();
        }
            break;
        case eBFCPAGECbReorganizedDehydrateRequired:
        {
            CPAGE cpage;
            cpage.LoadPage( ppghdr, g_rgcbPageSize[pbf->icbBuffer] );
            ullValue1 = cpage.CbReorganizedDehydrateRequired_();
        }
            break;
        case eBFCPAGEpgno:
            if ( Pdls()->CbPage() < 16 * 1024 )
            {
                //  On "small" pages (aka less than 16 KB), we do not have the extended header
                Unfetch( ppghdr );
                return errNotFound;
            }
            ullValue1 = ((CPAGE::PGHDR2*)ppghdr)->pgno;
            break;
        case eBFCPAGErgChecksum2:
            if ( Pdls()->CbPage() < 16 * 1024 )
            {
                Unfetch( ppghdr );
                return errNotFound;
            }
            ullValue1 = ((CPAGE::PGHDR2*)ppghdr)->rgChecksum[0];
            break;
        case eBFCPAGErgChecksum3:
            if ( Pdls()->CbPage() < 16 * 1024 )
            {
                Unfetch( ppghdr );
                return errNotFound;
            }
            ullValue1 = ((CPAGE::PGHDR2*)ppghdr)->rgChecksum[1];
            break;
        case eBFCPAGErgChecksum4:
            if ( Pdls()->CbPage() < 16 * 1024 )
            {
                Unfetch( ppghdr );
                return errNotFound;
            }
            ullValue1 = ((CPAGE::PGHDR2*)ppghdr)->rgChecksum[2];
            break;

        #define ExtrudeCPAGEFunc( cpagefunc )       \
            case eBFCPAGE##cpagefunc:               \
            {                                       \
                CPAGE cpage;                        \
                cpage.LoadPage( (void*)ppghdr, sizeof(CPAGE::PGHDR) );  \
                ullValue1 = cpage.cpagefunc() ? fTrue : fFalse;     \
            }                                       \
                break;

        ExtrudeCPAGEFunc( FLeafPage );
        ExtrudeCPAGEFunc( FInvisibleSons );
        ExtrudeCPAGEFunc( FRootPage );
        ExtrudeCPAGEFunc( FFDPPage );
        ExtrudeCPAGEFunc( FEmptyPage );
        ExtrudeCPAGEFunc( FPreInitPage );
        ExtrudeCPAGEFunc( FParentOfLeaf );
        ExtrudeCPAGEFunc( FSpaceTree );
        ExtrudeCPAGEFunc( FScrubbed );
        ExtrudeCPAGEFunc( FRepairedPage );
        ExtrudeCPAGEFunc( FIndexPage );
        ExtrudeCPAGEFunc( FNonUniqueKeys );
        ExtrudeCPAGEFunc( FLongValuePage );
        ExtrudeCPAGEFunc( FNewRecordFormat );
        ExtrudeCPAGEFunc( FNewChecksumFormat );

        case eBFCPAGEPgft:
        {
            CPAGE cpage;
            cpage.LoadPage( (void*)ppghdr, sizeof(CPAGE::PGHDR) );
            ullValue1 = cpage.Pgft();
        }
            break;

        //  The FPrimaryPage() indicator is a little special in that its the absence of
        //  several bits.
        case eBFCPAGEFPrimaryPage:
        {
            CPAGE cpage;
            cpage.LoadPage( (void*)ppghdr, sizeof(CPAGE::PGHDR) );
            ullValue1 = !cpage.FSpaceTree() && !cpage.FIndexPage() && !cpage.FLongValuePage();
        }
            break;

        case eBFBFNONE:
        default:
            dprintf("Unrecognized BF bit-field type\n");
            AssertEDBG( !"Unrecognized BF bit-field type." );
            return ErrERRCheck( JET_errInternalError );
    }

#ifdef SLOW_BUT_SURE_REFETCH_PAGE_EACH_TIME_WAY
    Unfetch( ppghdr );
#else
    Pdls()->UnlatchPageImage( ppghdr );
#endif

    *pullValue = ullValue1;

    return JET_errSuccess;
}

ULONGLONG CrceVERICountBFUndoInfo( const RCE * prceDebuggee )
{
    ULONGLONG crceUndoInfo = 0;

    BYTE rgbRceT[ sizeof(RCE) + 16 ] = { 0 };
    RCE * prceT = (RCE*)roundup( (ULONG_PTR)rgbRceT, 8 );  // not sure I need this re-aligned, but just in case.

    while( prceDebuggee )
    {
        crceUndoInfo++;

        const BOOL f = FReadVariable( prceDebuggee, prceT );
        AssertEDBG( f );
        if ( f )
        {
            prceDebuggee = prceT->PrceUndoInfoNext();
        }
        else
        {
            break;
        }
    }

    return crceUndoInfo;
}

ULONGLONG CbfBFIVersionsI( const BF * pbf, BOOL fForwardOlder )
{
    ULONGLONG cbf = 0;

    Assert( pbf );
    BYTE rgbBfT[ sizeof(BF) + 16 ];
    BF * pbfT = (BF*)roundup( (ULONG_PTR)rgbBfT, 8 );  // not sure I need this re-aligned, but just in case.
    memcpy( pbfT, pbf, sizeof(BF) );

    while( fForwardOlder ? pbfT->pbfTimeDepChainNext : pbfT->pbfTimeDepChainPrev )
    {
        cbf++;

        const BOOL f = FReadVariable( fForwardOlder ? pbfT->pbfTimeDepChainNext : pbfT->pbfTimeDepChainPrev, pbfT );
        AssertEDBG( f );
        if ( f )
        {
            //  success.
        }
        else
        {
            break;
        }
    }

    return cbf;
}

ULONGLONG CbfBFIOlderVersions( BF * pbfDebuggee )
{
    return CbfBFIVersionsI( pbfDebuggee, fTrue );
}

ULONGLONG CbfBFINewerVersions( BF * pbfDebuggee )
{
    return CbfBFIVersionsI( pbfDebuggee, fFalse );
}

ULONGLONG CbfBFIAllVersions( BF * pbfDebuggee )
{
    return CbfBFINewerVersions( pbfDebuggee ) + 1 + CbfBFIOlderVersions( pbfDebuggee );
}


//  This routine does not do a pure extraction of members from the BF or page image or 
//  CSXWLatch and return them, like the other evaulation functions.   It concentrates 
//  logic that is about the buffer, but not directly represented in the BF itself.
//

ERR ErrBFVirtualElement( size_t eBFMember, size_t cbUnused, QwEntryAddr pvDebuggee, PcvEntry pv, ULONGLONG * pullValue )
{
    ULONGLONG ullValue1 = 0x0;

    AssertEDBG( pullValue );
    *pullValue = 0xBAADF00D;

    const PBF pbf         = (const PBF)pv;

    switch ( eBFMember )
    {
        case eBFVIRTFBFIDatabasePage:
            ullValue1 = !pbf->fAvailable &&
                        !pbf->fQuiesced &&
                        !Pdls()->FIsTempDB( pbf->ifmp );
            break;

        case eBFVIRTCrceUndoInfoNext:
            ullValue1 = CrceVERICountBFUndoInfo( pbf->prceUndoInfoNext );
            break;

        case eBFVIRTCbfTimeDepChainNext:
            ullValue1 = CbfBFIOlderVersions( pbf );
            break;
        case eBFVIRTCbfTimeDepChainPrev:
            ullValue1 = CbfBFINewerVersions( pbf );
            break;
        case eBFVIRTCbfTimeDepChain:
            ullValue1 = CbfBFIAllVersions( pbf );
            break;

        case eBFVIRTFExcludedFromDump:
        {
            CPAGE::PGHDR * ppghdr = NULL;
#ifdef SLOW_BUT_SURE_REFETCH_PAGE_EACH_TIME_WAY
            if ( ( Pdls()->CbPage() < 16 * 1024 ) ?
                    !FFetchVariable( (CPAGE::PGHDR*)pbf->pv, &ppghdr ) :
                    !FFetchVariable( (CPAGE::PGHDR2*)pbf->pv, (CPAGE::PGHDR2**)&ppghdr ) )
            {
                ullValue1 = fTrue;
            }
#else
            //  FCachePage() actually knows to get a PGHDR2 if we've a 16+ KB page size
            if ( !Pdls()->FLatchPageImage( DEBUGGER_LOCAL_STORE::ePageHdrOnly, pbf->pv, &ppghdr ) )
            {
                ullValue1 = fTrue;
            }
#endif
            else
            {
                ullValue1 = fFalse;
            }
#ifdef SLOW_BUT_SURE_REFETCH_PAGE_EACH_TIME_WAY
            Unfetch( ppghdr );
#else
            Pdls()->UnlatchPageImage( ppghdr );
#endif
        }
            break;

        case eBFBFNONE:
        default:
            dprintf("Unrecognized BF bit-field type\n");
            AssertEDBG( !"Unrecognized BF bit-field type." );
            return ErrERRCheck( JET_errInternalError );
    }

    *pullValue = ullValue1;

    return JET_errSuccess;
}

ERR ErrBFSXWLStateFromStruct( size_t eBFMember, size_t cbUnused, QwEntryAddr pvDebuggee, PcvEntry pv, ULONGLONG * pullValue )
{
    ULONGLONG ullValue1 = 0x0;

    AssertEDBG( pullValue );
    *pullValue = 0xBAADF00D;

    const PBF pbf         = (const PBF)pv;

    switch ( eBFMember )
    {
        //  SXWL member accessors
#ifdef DEBUG
        //  The debugger state for this case is a pointer off this func.
        case eBFSXWLFLatched:
        case eBFSXWLFSharedLatched:
        case eBFSXWLFExclusiveLatched:
        case eBFSXWLFWriteLatched:
        case eBFSXWLCSharers:
        case eBFSXWLCWaitSharedLatch:
        case eBFSXWLCWaitExclusiveLatch:
        case eBFSXWLCWaitWriteLatch:
        case eBFSXWLCWaiters:
            Unused( pbf );
            
            Pdls()->AddWarning( "ERROR: In debug the sxwl member accessors are unavailable\n" );
            ullValue1 = fFalse;
            return ErrERRCheck( JET_errInternalError );
#else
        case eBFSXWLFLatched:
            ullValue1 = pbf->sxwl.FLatched() ? fTrue : fFalse;
            break;
        case eBFSXWLFSharedLatched:
            ullValue1 = pbf->sxwl.FSharedLatched() ? fTrue : fFalse;
            break;
        case eBFSXWLFExclusiveLatched:
            ullValue1 = pbf->sxwl.FExclusiveLatched() ?  fTrue : fFalse;
            break;
        case eBFSXWLFWriteLatched:
            ullValue1 = pbf->sxwl.FWriteLatched() ? fTrue : fFalse;
            break;
        case eBFSXWLCSharers:
            ullValue1 = pbf->sxwl.CSharers();
            break;
        case eBFSXWLCWaitSharedLatch:
            ullValue1 = pbf->sxwl.CWaitSharedLatch();
            break;
        case eBFSXWLCWaitExclusiveLatch:
            ullValue1 = pbf->sxwl.CWaitExclusiveLatch();
            break;
        case eBFSXWLCWaitWriteLatch:
            ullValue1 = pbf->sxwl.CWaitWriteLatch();
            break;
        case eBFSXWLCWaiters:
            ullValue1 = pbf->sxwl.CWaitSharedLatch() + pbf->sxwl.CWaitExclusiveLatch() + pbf->sxwl.CWaitWriteLatch();
            break;
#endif

        case eBFBFNONE:
        default:
            dprintf("Unrecognized BF bit-field type\n");
            AssertEDBG( !"Unrecognized BF bit-field type." );
            return ErrERRCheck( JET_errInternalError );
    }

#ifndef DEBUG
    *pullValue = ullValue1;

    return JET_errSuccess;
#endif
}


//
//  Querying BFs specifically
//

#define QBF( type, member, histoinfo, expreval, statseval, readval, printval )  \
    { #member, ErrBFBitFieldElemFromStruct, eBFBF##member, 0x0, histoinfo, expreval, statseval, readval, printval },

#define QL( type, member, histoinfo, expreval, statseval, readval, printval )   \
    {    "sxwl." #member "()",  ErrBFSXWLStateFromStruct, eBFSXWL##member, 0x0, histoinfo, expreval, statseval, readval, printval },

#define QPG( type, member, histoinfo, expreval, statseval, readval, printval )  \
    {    "pv->" #member,    ErrBFPageElemFromStruct, eBFCPAGE##member, 0x0, histoinfo, expreval, statseval, readval, printval },

#define QPGF( type, member, histoinfo, expreval, statseval, readval, printval ) \
    {    "pv->" #member "()",   ErrBFPageElemFromStruct, eBFCPAGE##member, 0x0, histoinfo, expreval, statseval, readval, printval },

CMemberDescriptor const rgmdBfEntryMembers[] =
{

    // possible add a pbfDebuggee | "this" type elem ... 
    //{ "this", eBFBFNONE, 0, 0,        NULL, NULL, NULL, NULL }

    //  Special virtual field, index of pbf.
    { "ibf", ErrBFBitFieldElemFromStruct, eBFibf, 0, eNoHistoSupport, UlongExprEval, NULL, UlongReadVal, UlongPrintVal },

    //  Basic Fields
    //

    // Skipping: ob0ic.
    // Consider 64KB?
    QEF(   BF, lgposOldestBegin0,   ePartialHisto|eAttemptContinousPrint|262144,    LgposExprEval, NULL, LgposReadVal, LgposPrintVal )
    QEF(   BF, lgposModify,         ePartialHisto|eAttemptContinousPrint|262144,    LgposExprEval, NULL, LgposReadVal, LgposPrintVal )
    QEF(   BF, rbsposSnapshot,      ePartialHisto|eAttemptContinousPrint|262144,    RbsposExprEval, NULL, RbsposReadVal, RbsposPrintVal )
    QEF(   BF, pbfTimeDepChainPrev, eNoHistoSupport,    PtrExprEval, NULL, PtrReadVal, PtrPrintVal )
    QEF(   BF, pbfTimeDepChainNext, eNoHistoSupport,    PtrExprEval, NULL, PtrReadVal, PtrPrintVal )
    //QEF(   BF, sxwl,                  NULL, NULL, NULL, NULL )
#ifndef DEBUG
    QL(  SXWL, FLatched,            ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QL(  SXWL, FSharedLatched,      ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QL(  SXWL, FExclusiveLatched,   ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QL(  SXWL, FWriteLatched,       ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QL(  SXWL, CSharers,            ePerfectHisto,      UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
    QL(  SXWL, CWaitSharedLatch,    ePerfectHisto,      UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
    QL(  SXWL, CWaitExclusiveLatch, ePerfectHisto,      UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
    QL(  SXWL, CWaitWriteLatch,     ePerfectHisto,      UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
    QL(  SXWL, CWaiters,            ePerfectHisto,      UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
#endif
    // Note ifmp is 8-bytes on 64-bit architectures, but this is ok I think b/c ifmp's are low values.
    QEF(   BF, ifmp,                ePerfectHisto,      DwordExprEval, NULL, DwordReadVal, DwordPrintVal )
    QEF(   BF, pgno,                ePartialHisto|1024, UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
    QEF(   BF, pv,                  eNoHistoSupport,    PtrExprEval, NULL, PtrReadVal, PtrPrintVal )
    QEF(   BF, pWriteSignalComplete,ePerfectHisto,      UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
    QPG( PAGE, checksum,            eNoHistoSupport,    QwordExprEval, NULL, QwordReadVal, QwordPrintVal )
    QPG( PAGE, dbtimeDirtied,       ePerfectHisto,      QwordExprEval, NULL, QwordReadVal, QwordPrintVal )
    QPG( PAGE, pgnoPrev,            ePartialHisto|1024, UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
    QPG( PAGE, pgnoNext,            ePartialHisto|1024, UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
    QPG( PAGE, objidFDP,            ePerfectHisto,      UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
    QPG( PAGE, cbFree,              ePerfectHisto,      ShortExprEval, NULL, ShortReadVal, ShortPrintVal )
    QPG( PAGE, cbUncommittedFree,   ePerfectHisto,      ShortExprEval, NULL, ShortReadVal, ShortPrintVal )
    QPG( PAGE, ibMicFree,           ePerfectHisto,      ShortExprEval, NULL, ShortReadVal, ShortPrintVal )
    QPG( PAGE, itagMicFree,         ePerfectHisto,      ShortExprEval, NULL, ShortReadVal, ShortPrintVal )
    QPG( PAGE, fFlags,              ePerfectHisto,      DwordExprEval, NULL, DwordReadVal, DwordPrintVal )
    QPG( PAGE, pgno,                ePartialHisto|1024, UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
    QPG( PAGE, rgChecksum2,         eNoHistoSupport,    QwordExprEval, NULL, QwordReadVal, QwordPrintVal )
    QPG( PAGE, rgChecksum3,         eNoHistoSupport,    QwordExprEval, NULL, QwordReadVal, QwordPrintVal )
    QPG( PAGE, rgChecksum4,         eNoHistoSupport,    QwordExprEval, NULL, QwordReadVal, QwordPrintVal )
    QPGF(PAGE, FLeafPage,           ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FInvisibleSons,      ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FRootPage,           ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FFDPPage,            ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FEmptyPage,          ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FPreInitPage,        ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FParentOfLeaf,       ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FSpaceTree,          ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FScrubbed,           ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FRepairedPage,       ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FPrimaryPage,        ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FIndexPage,          ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FNonUniqueKeys,      ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FLongValuePage,      ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FNewRecordFormat,    ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, FNewChecksumFormat,  ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QPGF(PAGE, Pgft,                ePerfectHisto,      UlongExprEval, NULL, PGFTReadVal, PGFTPrintVal )
     {  "pv->CbFreeActual()",       ErrBFPageElemFromStruct, eBFCPAGECbFreeActual, 0, ePerfectHisto,    ShortExprEval, NULL, ShortReadVal, ShortPrintVal },
     {  "pv->CbContiguousDehydrateRequired_()", ErrBFPageElemFromStruct, eBFCPAGECbContiguousDehydrateRequired, 0, ePerfectHisto,   UlongExprEval, NULL, UlongReadVal, UlongPrintVal },
     {  "pv->CbReorganizedDehydrateRequired_()",    ErrBFPageElemFromStruct, eBFCPAGECbReorganizedDehydrateRequired, 0, ePerfectHisto,  UlongExprEval, NULL, UlongReadVal, UlongPrintVal },

    QEF(  BF, err,                  ePerfectHisto,      ShortExprEval, NULL, ShortReadVal, ShortPrintVal )
    QBF(  BF, fLazyIO,              ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QBF(  BF, fNewlyEvicted,        ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QBF(  BF, fQuiesced,            ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QBF(  BF, fAvailable,           ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QBF(  BF, fWARLatch,            ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QBF(  BF, bfdf,                 ePerfectHisto,      UlongExprEval, NULL, BFDFReadVal, BFDFPrintVal )    // 2 bits
    QBF(  BF, fInOB0OL,             ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QBF(  BF, FRangeLocked,         ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QBF(  BF, irangelock,           ePerfectHisto,      ShortExprEval, NULL, ShortReadVal, ShortPrintVal )
    QBF(  BF, fCurrentVersion,      ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QBF(  BF, fOlderVersion,        ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QBF(  BF, fFlushed,             ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    // Did not break out bfls enum to strings because bfls will probably go away
    QBF(  BF, bfls,                 ePerfectHisto,      UlongExprEval, NULL, UlongReadVal, UlongPrintVal )  // 3 bits
    QEF(  BF, tickEligibleForNomination,    ePartialHisto|eAttemptContinousPrint|1000,      UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
    QEF(  BF, bfrs,                 ePerfectHisto,      UlongExprEval, NULL, BFRSReadVal, BFRSPrintVal )
    QEF(  BF, prceUndoInfoNext,     eNoHistoSupport,    PtrExprEval, NULL, PtrReadVal, PtrPrintVal )
    QBF(  BF, FDependentPurged,     ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QBF(  BF, FImpedingCheckpoint,  ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
     {  "lrukic.TickLastTouch()",       ErrBFBitFieldElemFromStruct, eBFLRUKTickLastTouch, 4, ePartialHisto|eAttemptContinousPrint|1000,    DwordExprEval, NULL, DwordReadVal, DwordPrintVal },
     {  "lrukic.TickLastTouchTime()",   ErrBFBitFieldElemFromStruct, eBFLRUKTickLastTouchTime, 4, ePartialHisto|eAttemptContinousPrint|1000,    DwordExprEval, NULL, DwordReadVal, DwordPrintVal },
     {  "lrukic.kLrukPool()",           ErrBFBitFieldElemFromStruct, eBFLRUKkLrukPool, 1, ePerfectHisto,    UlongExprEval, NULL, UlongReadVal, UlongPrintVal },
     {  "lrukic.Tick1()",               ErrBFBitFieldElemFromStruct, eBFLRUKTick1, 4, ePartialHisto|eAttemptContinousPrint|1000,    DwordExprEval, NULL, DwordReadVal, DwordPrintVal },
     {  "lrukic.Tick2()",               ErrBFBitFieldElemFromStruct, eBFLRUKTick2, 4, ePartialHisto|eAttemptContinousPrint|1000,   DwordExprEval, NULL, DwordReadVal, DwordPrintVal },
     {  "lrukic.TickIndex()",           ErrBFBitFieldElemFromStruct, eBFLRUKTickIndex, 4, ePartialHisto|eAttemptContinousPrint|1000,   DwordExprEval, NULL, DwordReadVal, DwordPrintVal },
     {  "lrukic.TickIndexTarget()",     ErrBFBitFieldElemFromStruct, eBFLRUKTickIndexTarget, 4, ePartialHisto|eAttemptContinousPrint|1000,   DwordExprEval, NULL, DwordReadVal, DwordPrintVal },
     {  "lrukic.TickIndexTime()",       ErrBFBitFieldElemFromStruct, eBFLRUKTickIndexTime, 4, ePartialHisto|eAttemptContinousPrint|1000,   DwordExprEval, NULL, DwordReadVal, DwordPrintVal },
     {  "lrukic.TickIndexTargetTime()", ErrBFBitFieldElemFromStruct, eBFLRUKTickIndexTargetTime, 4, ePartialHisto|eAttemptContinousPrint|1000,   DwordExprEval, NULL, DwordReadVal, DwordPrintVal },
     {  "lrukic.PctCachePriority()",    ErrBFBitFieldElemFromStruct, eBFLRUKPctCachePriority, 1, ePerfectHisto,    UshortExprEval, NULL, UshortReadVal, UshortPrintVal },
     {  "lrukic.FSuperColded()",        ErrBFBitFieldElemFromStruct, eBFLRUKFSuperColded, 1, ePerfectHisto,   BoolExprEval, NULL, BoolReadVal, BoolPrintVal },
     {  "lrukic.FSuperColdedIndex()",   ErrBFBitFieldElemFromStruct, eBFLRUKFSuperColdedIndex, 1, ePerfectHisto,   BoolExprEval, NULL, BoolReadVal, BoolPrintVal },
     {  "lrukic.FCorrelatedTouch()",    ErrBFBitFieldElemFromStruct, eBFLRUKfCorrelatedTouch, 1, ePerfectHisto,   BoolExprEval, NULL, BoolReadVal, BoolPrintVal },
     {  "lrukic.DtickCorrelatedRange()",ErrBFBitFieldElemFromStruct, eBFLRUKDtickCorrelatedRange, 4, ePartialHisto|10,   UlongExprEval, NULL, UlongReadVal, UlongPrintVal },
     {  "lrukic.DtickTickKRange()",     ErrBFBitFieldElemFromStruct, eBFLRUKDtickTickKRange, 4, ePartialHisto|10,   UlongExprEval, NULL, UlongReadVal, UlongPrintVal },
     {  "lrukic.DtickMisIndexedRange()",ErrBFBitFieldElemFromStruct, eBFLRUKDtickMisIndexedRange, 4, ePartialHisto|10,   UlongExprEval, NULL, UlongReadVal, UlongPrintVal },
     {  "lrukic.FResourceLocked()",     ErrBFBitFieldElemFromStruct, eBFLRUKFResourceLocked, 1, ePerfectHisto,   BoolExprEval, NULL, BoolReadVal, BoolPrintVal },

    QBF(  BF, tce,                  ePerfectHisto,      UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
    QEF(  BF, pvIOContext,          eNoHistoSupport,    PtrExprEval, NULL, PtrReadVal, PtrPrintVal )
    QBF(  BF, icbBuffer,            ePerfectHisto,      UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
    QBF(  BF, icbPage,              ePerfectHisto,      UlongExprEval, NULL, UlongReadVal, UlongPrintVal )
    QBF(  BF, fSuspiciouslySlowRead,ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QBF(  BF, fSyncRead,            ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )
    QBF(  BF, bfat,                 ePerfectHisto,      UlongExprEval, NULL, BFATReadVal, BFATPrintVal )    // 2 bits
    QBF(  BF, fAbandoned,           ePerfectHisto,      BoolExprEval, NULL, BoolReadVal, BoolPrintVal )

    
     {  "FBFIDatabasePage()",       ErrBFVirtualElement, eBFVIRTFBFIDatabasePage, 0, ePerfectHisto, BoolExprEval, NULL, BoolReadVal, BoolPrintVal },
     {  "CrceUndoInfoNext()",       ErrBFVirtualElement, eBFVIRTCrceUndoInfoNext, 0, ePerfectHisto, UlongExprEval, NULL, UlongReadVal, UlongPrintVal },
     {  "CbfTimeDepChainNext()",    ErrBFVirtualElement, eBFVIRTCbfTimeDepChainNext, 0, ePerfectHisto, UlongExprEval, NULL, UlongReadVal, UlongPrintVal },
     {  "CbfTimeDepChainPrev()",    ErrBFVirtualElement, eBFVIRTCbfTimeDepChainPrev, 0, ePerfectHisto, UlongExprEval, NULL, UlongReadVal, UlongPrintVal },
     {  "CbfTimeDepChain()",        ErrBFVirtualElement, eBFVIRTCbfTimeDepChain, 0, ePerfectHisto, UlongExprEval, NULL, UlongReadVal, UlongPrintVal },
     {  "FExcludedFromDump()",      ErrBFVirtualElement, eBFVIRTFExcludedFromDump, 0, ePerfectHisto, BoolExprEval, NULL, BoolReadVal, BoolPrintVal },
     
};


class BfEntryDescriptor : public IEntryDescriptor
{
    ULONG CbEntry( PcvEntry pvStruct ) const   { return sizeof( BF ); }
    const CHAR * SzDefaultPrintList() const    { return "print:ifmp,pgno"; }

    void DumpEntry( QwEntryAddr qwAddr, PcvEntry pcvEntry ) const
    {
        dprintf( "   pbf = %N\n", (void*)qwAddr );

        // Here is how to dump ... from the templated dump func ...
        //  pt->Dump( CPRINTFWDBG::PcprintfInstance(), (BYTE*)ptDebuggee - (BYTE*)pt );

        ((PBF)pcvEntry)->Dump( CPRINTFWDBG::PcprintfInstance(), (BYTE*)qwAddr - (BYTE*)pcvEntry );
        dprintf("\n" );
    }

    const CMemberDescriptor * PmdMatch( const char * szTarget, const CHAR chDelim = '\0' ) const
    {
        return PmdMemberDescriptorLookupHelper( rgmdBfEntryMembers, _countof( rgmdBfEntryMembers ), szTarget, chDelim );
    }

    ULONG CMembers() const
    {
        return 1 + ULONG( PmdMatch( SzIQLastMember ) - PmdMatch( SzIQFirstMember ) );
    }

};

BfEntryDescriptor g_iedBf;

//  Currently the CacheQuery / Iteration Query is implemented with the iteration loop out here
//  in edbg, instead of in iterquery.hxx.  It would be possible to create a IIterator type
//  interface that gives each BF to the CCacheQuery predicate evaluator, but I am not sure
//  it is of value or necessary.  The lion's share of complicated logic has been commonalized
//  though.
//
ERR ErrEDBGQueryCache(
    ULONG       ciq,
    CIterQuery **  rgpiq
    )
{
    SaveDlsDefaults sdd; // saves here, and then restores the implicit defaults on .dtor.
    ERR err = JET_errSuccess;
    PBF pbf = NULL;

    const TICK tickStart = TickOSTimeCurrent();

    Call( Pdls()->ErrBFInitCacheMap() );

    BOOL fPrints = fFalse;
    for ( ULONG iiq = 0; iiq < ciq; iiq++ )
    {
        fPrints = fPrints ? fTrue : rgpiq[iiq]->FPrints();
    }

    //  scan all valid BFs looking for this IFMP / PGNO

    if ( !fPrints )
    {
        //  If the query action does not print, we'll print status ...
        dprintf( "\t..." );
    }

    C_ASSERT( sizeof( PBF ) <= sizeof( QwEntryAddr )  ); // for FEvalExpr() cast of pbfDebuggee to QwEntryAddr

    // Note we could load cbfCacheAddressable / cbfCacheInit to know how many pbf's we'll be 
    //  processing, so we could even do a proper [....   ] type progress bar.
    TICK tickCheck = tickStart;
    for ( LONG_PTR ibf = 0; ibf < Pdls()->CbfInit(); ibf++ )
    {
        //  compute the address of the target BF, and retrieve contents ...
        PBF pbfDebuggee;
        if ( Pdls()->ErrBFCacheRetrieveNextBF( ibf, &pbfDebuggee, &pbf ) )
        {
            //  we failed to read this BF
            dprintf( "Error: Could not read BF[%d]. Aborting.\n", ibf );
            goto HandleError;
        }

        if ( !fPrints &&
                ( 0 == ibf % 13107 ) )
        {
            //  a "." of status every 13107 pages (80 dots / 8 GB of cache w/ 8k pages).
            dprintf( "." );
        }
        if ( 0 == ibf % 250 && DtickDelta( tickCheck, TickOSTimeCurrent() ) > 250 )
        {
            if ( FEDBGCheckForCtrlC() )
            {
                dprintf( "Aborting early (due to ctrl-c from you), at ibf = %d of %d total.\n", ibf, Pdls()->CbfInit() );
                break;
            }
            tickCheck = TickOSTimeCurrent();
        }

        for ( ULONG iiq = 0; iiq < ciq; iiq++ )
        {
            if ( rgpiq[iiq]->FEvaluatePredicate( (QwEntryAddr)pbfDebuggee, pbf ) )
            {
                rgpiq[iiq]->PerformAction( (QwEntryAddr)pbfDebuggee, pbf );
            }
        }

    }

    if ( !fPrints )
    {
        dprintf( ". Done.\n" );
    }

    for ( ULONG iiq = 0; iiq < ciq; iiq++ )
    {
        rgpiq[iiq]->FinalAction();
    }

HandleError:

    //  unload BF parameters

    Pdls()->BFTermCacheMap();

    const TICK dtick = TickOSTimeCurrent() - tickStart;
    dprintf( "Elapsed Time: %d.%03d\n", dtick / 1000, dtick % 1000 );

    return err;
}

const CHAR * SzReadFormat( const CMemberDescriptor * pmd )
{
    AssertEDBG( pmd );

    if ( pmd->m_pfnFReadVal == LgposReadVal )
    {
        return "3-part tuple (hex integers)";
    }
    else if ( pmd->m_pfnFReadVal == PtrReadVal )
    {
        return "Pointer (hex) or NULL";
    }
    else if ( pmd->m_pfnFReadVal == BoolReadVal )
    {
        return "fTrue or fFalse";
    }
    else if ( pmd->m_pfnFReadVal == ShortReadVal )
    {
        return "Decimal integer";
    }
    else if ( pmd->m_pfnFReadVal == UlongReadVal )
    {
        return "Decimal integer";
    }
    else if ( pmd->m_pfnFReadVal == DwordReadVal )
    {
        return "DWORD (hex integer)";
    }
    else if ( pmd->m_pfnFReadVal == QwordReadVal )
    {
        return "QWORD (hex integer)";
    }

    return "Custom parsing";
}

//  ================================================================
DEBUG_EXT( EDBGCacheQuery )
//  ================================================================
{
    ERR err = JET_errSuccess;
    CIterQuery * piq = NULL;

    Call( ErrIQCreateIterQuery( &g_iedBf, argc, argv, &piq ) );

    if ( fDebugMode )
    {
        piq->PrintQuery();
    }

    dprintf( "\n" );

    Call( ErrEDBGQueryCache( 1, &piq ) );

HandleError:

    if ( piq )
    {
        delete piq;
    }

    if ( err < JET_errSuccess && err != errCantRetrieveDebuggeeMemory )
    {
        dprintf( "Usage: CACHEQUERY <query constraints> <query action>\n\n" );
        dprintf( "    The query options are like so:\n" );
        dprintf( "        <query constraints> - Can be zero or more clauses evaluating the pbf, latch, and/or\n" );
        dprintf( "                              page data members.  The query constraints can be ==, !=,\n" );
        dprintf( "                              <=, >=, <, > evaluated against any of the following elements:\n" );
        dprintf( "                                  * - means all BFs satisfy query\n" );
        for ( ULONG imd = 0; imd < _countof(rgmdBfEntryMembers); imd++ )
        {
            dprintf( "                                  %s - parsed as %s\n", rgmdBfEntryMembers[imd].szMember, SzReadFormat( &rgmdBfEntryMembers[imd] ) );
        }
        dprintf( "        <query action>      - The action to take, can be any of:\n" );
        dprintf( "                                  count - Counts number of BFs that match the query.\n" );
        dprintf( "                                  dump - Calls BF::Dump() on every structure that matche the query.\n" );
        dprintf( "                                  print:xxx,yyy - xxx,yyy are attributes from the member list.\n" );
        dprintf( "                                  min:xxx - Finds the min of all those members\n" );
        dprintf( "                                  accum:xxx - Accumulates the element.\n" );
        dprintf( "                                  histo:xxx - Makes a histogram out of the element values.\n" );
        dprintf( "    It is easiest to describe the queries possible with a few examples:\n" );
        dprintf( "        !ese cachequery bfdf >= dirty count /* counts all dirty pages in the cache.*/\n" );
        dprintf( "        !ese cachequery ifmp == 0x2 && bfdf >= dirty count /* counts all dirty pages for IFMP 2 */\n" );
        dprintf( "        !ese cachequery lgposOldestBegin0 <= 4f4,0800,004d print:ifmp,pgno,lgposModify /* prints the IFMP, pgno and lgposModify for all pbf's that have a oldest begin 0, older than 4f4,0800,004d */\n" );
        dprintf( "        When evaluating a clause with && and ||, the &&'s are higher precedence.\n" );
        dprintf( "    There is a special value that can be used with lgpos valued things:\n" );
        dprintf( "        !ese cachequery lgposModify > lgposWaypoint print:ifmp,pgno,lgposModify /* prints the IFMP, pgno and lgposModify for all pbf's that have a lgposModify greater than the waypoint value for that DB */\n" );
        dprintf( "\n" );
    }

}

//  ================================================================
DEBUG_EXT( EDBGCacheSum )
//  ================================================================
{
    ERR err = JET_errSuccess;
    BOOL fVerbose = fFalse;

    /*
    I have many future thoughts of where this might go ...

    Make first argument an optional ifmp, and then scope all the
    subsequent queries by ifmp?

    Many of the other query elements I've thought of adding:

        o err != -261 && err != xxx printsz:BadBFError: <- to print out all BFs w/ bad errors in them.
            o make it a histogram of errors too ...

        o bfdf >= dirty && lgposOldestBegin0 <= x,x,x       <- where x,x,x is the preferred checkpoint, # of dirty buffers past preferred checkpoint
            o might be interesting (since I have a few per IFMP things I'm debating to
                figure out a short and long IFMP data ... ergo:
                    !ese cachesum       -> prints out short per IFMP summary
                    !ese cachesum v     -> prints out what it does
                    !ese cachesum 4     -> prints out a long per IFMP summary for ifmp = 4
                    !ese cachesum *     -> prints out long per IFMP summary for all ifmps.
                    o OR maybe just make it a different command.

        o * histo:lrukic.TickLastTouch()            <- print lifetime over cache... tricky ...
                                                        actually maybe not so tricky, grab g_tickLastGiven and start minus'ing from there!

        o make an entry like this for every single attached FMP we have ...
                ifmp == xxx && lgposModify >= y,y,y     <- where y,y,y is the waypoint for the specific ifmp xxx, gives # of dirty buffers pinned.

        o instead of "* histo:bfdf" we're approximating, one could even enumerate all FMPs, and
                then ask for ifmp == x histo:bfdf, and then make a cunning two level histogram,
                ifmp on the vertical, bfdf, along the top also with the length of the cache bar
                representing the amount of cache that ifmp has ...
            something like this:
             ifmp  12345678901234567890123456789012345678901234567890 <-- that's 50 chars...
             ifmp  |        |         |         |         |         |
              0x0  cd                                                  [ 1%]
              0x1  ccccccccuuddddddddddff                              [30%]
              0x2  ccccccccccccccccccccccccccccccccccccccccccccdddddd  [64%]
              0x3  cccdddf                                             [ 6%]
            where IFMP 0x3 has only 7% of the cache and its a little over 50% dirty for it.
            ? Or put dirty at the other end, so there are effectively two graphs.
            ? Or do it vertically...

        o be cool to track disassociated DBs (i.e. keep cache alive feature), and % that pertains to that.

        o Be cool to be able to show buffers related to FMPs/Insts in recover and not

        o by checking objid's in page images we could detect how many schema / catalog pages are cached ... this
            is something we should only do at a higher verbosity ... as the query would take longer
            x this will be tricky b/c of temp DB pages ... need some sort of "ifmp != #tempdb" like clause ...
                update: think this is taken care of by the pv-> based accessors

    */

    if ( ( argc != 0 ) && ( argc != 1 ) )
    {
        dprintf( "Invalid number of arguments: %d\n", argc );
        dprintf( "!ese cachesum [verbose]\n" );
        return;
    }

    if ( argc == 1 &&
        !( fVerbose = ( 0 == _stricmp( argv[0], "v" ) ||
                        0 == _stricmp( argv[0], "verbose" ) ) ) )
    {
        // whatever that argument was, we don't know about it
        dprintf( "Invalid argument: %hs\n", argv[0] );
        dprintf( "!ese cachesum [verbose]\n" );
        return;
    }


    LONG_PTR    cbfCacheAddressableT;
    CHAR        szCacheQuiesceMin[13] = "0";
    if ( !FReadGlobal( "cbfCacheAddressable", &cbfCacheAddressableT ) )
    {
        dprintf( "Warning: Could not read cbfCacheAddressable ... won't be able to check for lost quiesced buffers.\n" );
    }
    else
    {
        OSStrCbFormatA( szCacheQuiesceMin, sizeof(szCacheQuiesceMin), "%I64d", (LONGLONG)cbfCacheAddressableT );
    }


    CIterQuery * rgpiqCacheSum[200];
    ULONG cQA = 0;
    CPerfectHistogramStats * rgphisto[20];
    ULONG chisto = 0;

#define Setup1( cbfCount )                                  \
        __int64 cbfCount = 0;                               \
        CHAR * rgsz##cbfCount []
#define Setup2( cbfCount )                                  \
        Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgsz##cbfCount)-1, rgsz##cbfCount, (void*)&cbfCount, &(rgpiqCacheSum[cQA++]) ) );
#define SetupH( phisto )                                    \
        CPerfectHistogramStats * phisto = new CPerfectHistogramStats();     \
        Alloc( phisto );                                    \
        rgphisto[ chisto++ ] = phisto;                      \
        CHAR * rgsz##phisto []
// Note: Must cast phisto to CStats * before void* to get the proper virtual interface conversion ...
#define SetupI( phisto )                                    \
        Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgsz##phisto)-1, rgsz##phisto, (void*)((CStats*)phisto), &(rgpiqCacheSum[cQA++]) ) );

    Setup1( cbfTotal )  = { "*", "count", NULL };
    Setup2( cbfTotal );
    Setup1( cbfAvail )  = { "fAvailable", "==", "fTrue", "count", NULL };
    Setup2( cbfAvail );
    Setup1( cbfQuiesced )   = { "fQuiesced", "==", "fTrue", "count", NULL };
    Setup2( cbfQuiesced );
    Setup1( cbfQuiescedAddressable ) = { "fQuiesced", "==", "fTrue", "&&", "ibf", "<", szCacheQuiesceMin, "count", NULL };
    Setup2( cbfQuiescedAddressable );
    Setup1( cbfClean )  = { "bfdf", "==", "bfdfClean", "&&", "fAvailable", "==", "fFalse", "&&", "fQuiesced", "==", "fFalse", "count", NULL };
    Setup2( cbfClean );
    Setup1( cbfUntidy ) = { "bfdf", "==", "bfdfUntidy", "count", NULL };
    Setup2( cbfUntidy );
    Setup1( cbfDirty )  = { "bfdf", "==", "bfdfDirty",  "count", NULL };
    Setup2( cbfDirty );
    Setup1( cbfFilthy ) = { "bfdf", "==", "bfdfFilthy", "count", NULL };
    Setup2( cbfFilthy );

    Setup1( cbfUnquiescedTooHigh ) = { "fQuiesced", "==", "fFalse", "&&", "ibf", ">=", szCacheQuiesceMin, "count", NULL };
    Setup2( cbfUnquiescedTooHigh );
    Setup1( cbfAvailTooHigh )      = { "fAvailable", "==", "fTrue", "&&", "ibf", ">=", szCacheQuiesceMin, "count", NULL };
    Setup2( cbfAvailTooHigh );

    Setup1( cbfErrBFIPageFaultPending ) = { "err", "==", "-260" /* errBFIPageFaultPending */, "count", NULL };
    Setup2( cbfErrBFIPageFaultPending );
    Setup1( cbfErrBFIPageNotVerified )  = { "err", "==", "-261" /* errBFIPageNotVerified  */, "count", NULL };
    Setup2( cbfErrBFIPageNotVerified );
    Setup1( cbfWrnBFPageFlushPending )  = { "err", "==",  "204" /* wrnBFPageFlushPending  */, "&&", "pWriteSignalComplete", "==", "0", "count", NULL };
    Setup2( cbfWrnBFPageFlushPending );
    Setup1( cbfWrnBFPageFlushCompleted )= { "err", "==",  "204" /* wrnBFPageFlushPending  */, "&&", "pWriteSignalComplete", "!=", "0", "count", NULL };
    Setup2( cbfWrnBFPageFlushCompleted );
    Setup1( cbfWrnBFPageFlushCompletedWithErr )= { "err", "==",  "204" /* wrnBFPageFlushPending  */, "&&", "pWriteSignalComplete", "!=", "0", "&&", "pWriteSignalComplete", "!=", "208", "count", NULL };
    Setup2( cbfWrnBFPageFlushCompletedWithErr );

#ifndef DEBUG
    Setup1( cbfLatched )            = { "fAvailable", "==", "fFalse", "&&", "fQuiesced", "==", "fFalse", "&&", "err", "!=", "-260", "&&", "sxwl.FLatched()", "==", "fTrue", "count", NULL };
    Setup2( cbfLatched );
    Setup1( cbfSharedLatched )      = { "sxwl.FSharedLatched()", "==", "fTrue", "count", NULL };
    Setup2( cbfSharedLatched );
    Setup1( cbfExclusiveLatched )   = { "sxwl.FExclusiveLatched()", "==", "fTrue", "count", NULL };
    Setup2( cbfExclusiveLatched );
    Setup1( cbfWriteLatched )       = { "fAvailable", "==", "fFalse", "&&", "fQuiesced", "==", "fFalse", "&&", "err", "!=", "-260", "&&", "sxwl.FWriteLatched()", "==", "fTrue", "count", NULL };
    Setup2( cbfWriteLatched );
    Setup1( cSharers )              = { "sxwl.CSharers()", ">", "0", "accum:sxwl.CSharers()", NULL };
    Setup2( cSharers );
    Setup1( cWaiters )              = { "sxwl.CWaiters()", ">", "0", "accum:sxwl.CWaiters()", NULL };
    Setup2( cWaiters );
    Setup1( cSharedWaiters )        = { "sxwl.CWaitSharedLatch()", ">", "0", "accum:sxwl.CWaitSharedLatch()", NULL };
    Setup2( cSharedWaiters );
    Setup1( cExclusiveWaiters )     = { "sxwl.CWaitExclusiveLatch()", ">", "0", "accum:sxwl.CWaitExclusiveLatch()", NULL };
    Setup2( cExclusiveWaiters );
    Setup1( cWriteWaiters )         = { "sxwl.CWaitWriteLatch()", ">", "0", "accum:sxwl.CWaitWriteLatch()", NULL };
    Setup2( cWriteWaiters );
#endif

    CHAR szIcbPage[20];
    OSStrCbFormatA( szIcbPage, sizeof(szIcbPage), "%d", IcbBFIBufferSize( Pdls()->CbPage() ) );

    Setup1( cbfDehydrated )         = { "fAvailable", "==", "fFalse", "&&", "icbBuffer", "!=", szIcbPage, "count", NULL };
    Setup2( cbfDehydrated );

    SetupH( phistoUsedBfat )            = { "ifmp", "!=", "0x7fffffff", "histo:bfat", NULL };
    SetupI( phistoUsedBfat );

    SetupH( phistoBfrs )            = { "ifmp", "!=", "0x7fffffff", "histo:bfrs", NULL };
    SetupI( phistoBfrs );

    Setup1( cbfCleanMapped )        = { "ifmp", "!=", "0x7fffffff", "&&", "bfat", "==", "bfatViewMapped", "&&", "bfdf", "==", "bfdfClean", "count", NULL };
    Setup2( cbfCleanMapped );
    Setup1( cbfCleanMappedCHECK )   = { "bfdf", "==", "bfdfClean", "&&", "fAvailable", "==", "fFalse", "&&", "fQuiesced", "==", "fFalse", "&&", "bfat", "==", "bfatViewMapped", "count", NULL };
    Setup2( cbfCleanMappedCHECK );
    Setup1( cbfCopyMapped )         = { "ifmp", "!=", "0x7fffffff", "&&", "bfat", "==", "bfatViewMapped", "&&", "bfdf", ">", "bfdfClean", "count", NULL };
    Setup2( cbfCopyMapped );

    // Note: Those with err == 204 are actually "write complete" (and thus remapped already), anything else is just regular copy mapped, consider
    // separating.
    Setup1( cbfNonCleanNonWriteCompletedMapped )= { "ifmp", "!=", "0x7fffffff", "&&", "bfat", "==", "bfatViewMapped", "&&", "bfdf", ">", "bfdfClean", "&&", "err", "!=", "204", "count", NULL };
    Setup2( cbfNonCleanNonWriteCompletedMapped );
    Setup1( cbfCopyMappedCHECK )= { "bfdf", ">", "bfdfClean", "&&", "fAvailable", "==", "fFalse", "&&", "fQuiesced", "==", "fFalse", "&&", "bfat", "==", "bfatViewMapped", "count", NULL };
    Setup2( cbfCopyMappedCHECK );

    Setup1( cbfFracCommitClean )            = { "ifmp", "!=", "0x7fffffff", "&&", "bfat", "==", "bfatFracCommit", "&&", "bfdf", "==", "bfdfClean", "count", NULL };
    Setup2( cbfFracCommitClean );
    Setup1( cbfFracCommitUntidy )           = { "ifmp", "!=", "0x7fffffff", "&&", "bfat", "==", "bfatFracCommit", "&&", "bfdf", "==", "bfdfUntidy", "count", NULL };
    Setup2( cbfFracCommitUntidy );
    Setup1( cbfFracCommitDirty )            = { "ifmp", "!=", "0x7fffffff", "&&", "bfat", "==", "bfatFracCommit", "&&", "bfdf", "==", "bfdfDirty", "count", NULL };
    Setup2( cbfFracCommitDirty );
    Setup1( cbfFracCommitFilthy )           = { "ifmp", "!=", "0x7fffffff", "&&", "bfat", "==", "bfatFracCommit", "&&", "bfdf", "==", "bfdfFilthy", "count", NULL };
    Setup2( cbfFracCommitFilthy );
    Setup1( cbfMappedClean )            = { "ifmp", "!=", "0x7fffffff", "&&", "bfat", "==", "bfatViewMapped", "&&", "bfdf", "==", "bfdfClean", "count", NULL };
    Setup2( cbfMappedClean );
    Setup1( cbfMappedUntidy )           = { "ifmp", "!=", "0x7fffffff", "&&", "bfat", "==", "bfatViewMapped", "&&", "bfdf", "==", "bfdfUntidy", "count", NULL };
    Setup2( cbfMappedUntidy );
    Setup1( cbfMappedUntidyWC )         = { "ifmp", "!=", "0x7fffffff", "&&", "bfat", "==", "bfatViewMapped", "&&", "bfdf", "==", "bfdfUntidy", "&&", "err", "==", "204", "count", NULL };
    Setup2( cbfMappedUntidyWC );
    Setup1( cbfMappedDirty )            = { "ifmp", "!=", "0x7fffffff", "&&", "bfat", "==", "bfatViewMapped", "&&", "bfdf", "==", "bfdfDirty", "count", NULL };
    Setup2( cbfMappedDirty );
    Setup1( cbfMappedDirtyWC )          = { "ifmp", "!=", "0x7fffffff", "&&", "bfat", "==", "bfatViewMapped", "&&", "bfdf", "==", "bfdfDirty", "&&", "err", "==", "204", "count", NULL };
    Setup2( cbfMappedDirtyWC );
    Setup1( cbfMappedFilthy )           = { "ifmp", "!=", "0x7fffffff", "&&", "bfat", "==", "bfatViewMapped", "&&", "bfdf", "==", "bfdfFilthy", "count", NULL };
    Setup2( cbfMappedFilthy );
    Setup1( cbfMappedFilthyWC )         = { "ifmp", "!=", "0x7fffffff", "&&", "bfat", "==", "bfatViewMapped", "&&", "bfdf", "==", "bfdfFilthy", "&&", "err", "==", "204", "count", NULL };
    Setup2( cbfMappedFilthyWC );
    #define cbfMappedTot                    ( cbfMappedClean + cbfMappedUntidy + cbfMappedDirty + cbfMappedFilthy )

    Setup1( cbfAbandoned )          = { "ifmp", "!=", "0x7fffffff", "&&", "fAbandoned", "==", "fTrue", "count", NULL };
    Setup2( cbfAbandoned );

    Setup1( cbfUndoInfo )           = { "prceUndoInfoNext", "!=", "NULL", "count", NULL };
    Setup2( cbfUndoInfo );

    Setup1( cbfVersioned )          = { "pbfTimeDepChainNext", "!=", "NULL", "count", NULL };
    Setup2( cbfVersioned );
    Setup1( cbfOlderVersions )      = { "fOlderVersion", "==", "fTrue", "count", NULL };
    Setup2( cbfOlderVersions );

    SetupH( phistoIfmp )            = { "*", "histo:ifmp", NULL };
    SetupI( phistoIfmp );

    SetupH( phistoIcbBuffer )       = { "FBFIDatabasePage()", "==", "fTrue", "histo:icbBuffer", NULL };
    SetupI( phistoIcbBuffer );

    Setup1( cbfSuperColdedInt )     = { "ifmp", "!=", "0x7fffffff", "&&", "lrukic.FSuperColded()", "==", "fTrue", "&&", "fOlderVersion", "==", "fTrue", "count", NULL };
    Setup2( cbfSuperColdedInt );
    Setup1( cbfSuperColdedExt )     = { "ifmp", "!=", "0x7fffffff", "&&", "lrukic.FSuperColded()", "==", "fTrue", "&&", "fOlderVersion", "==", "fFalse", "count", NULL };
    Setup2( cbfSuperColdedExt );
    SetupH( phistoKPool )           = { "ifmp", "!=", "0x7fffffff", "&&", "lrukic.FSuperColded()", "==", "fFalse", "histo:lrukic.kLrukPool()", NULL };
    SetupI( phistoKPool );
    Setup1( cbfSuperColdMisIndexed )= { "ifmp", "!=", "0x7fffffff", "&&", "lrukic.FSuperColded()", "==", "fTrue", "&&", "lrukic.FSuperColdedIndex()", "==", "fFalse", "count", NULL };
    Setup2( cbfSuperColdMisIndexed );

    SetupH( phistoErrs )            = { "ifmp", "!=", "0x7fffffff", "histo:err", NULL };
    SetupI( phistoErrs );

    //  All page elements queries, must have the verbose option 
    //      (because fetching pages is expensive)
    //

    //  Missed Dehydration opportunities, that we could have handled.
    //
    //  These are dehydrations that we could have handled today with
    //  the code as it exists today.  We know such holes exist in the 
    //  code due to limitations of what the buffer manager can do today, 
    //  but we won't worry about it unless this count gets too high.
    //
    ULONG rgcbfHyperPlainMissed[icbPageMax] = { 0 };
    ULONG rgcbfHyperReorgMissed[icbPageMax] = { 0 };

    //  Enhanced Dehydration opportunities.
    //
    //  These are dehydrations that we could perform if we changed 
    //  the code to handle it.  These require a serious DCR to fix.
    //
    ULONG rgcbfHyperNonPowerOf2[icbPageMax] = { 0 };
    ULONG rgcbfHyperDirty[icbPageMax] = { 0 };

    //  u-Page Dehydration opportunities
    //
    //  Dehydrating to less than the OS commit size.  This requires a
    //  serious feature to fix.
    //
    ULONG rgcbfHyperuPages[icbPageMax] = { 0 };
    ULONG rgcbfHyperUDNPu[icbPageMax] = { 0 };  // dirty, non-power-of-2, u-Page based... optimal.

    //  we setup the variables here, but only add them to the query / Setup2() if we
    //  we're approved to do a verbose cache summary
    Setup1( cbfSinglePageTrees )    = { "pv->FRootPage()", "==", "fTrue", "&&", "pv->FLeafPage()", "==", "fTrue", "count", NULL };
    Setup1( cbf2LvlRootPages )      = { "pv->FRootPage()", "==", "fTrue", "&&", "pv->FParentOfLeaf()", "==", "fTrue", "count", NULL };
    Setup1( cbf3LvlRootPages )      = { "pv->FRootPage()", "==", "fTrue", "&&", "pv->FParentOfLeaf()", "==", "fFalse", "&&", "pv->FLeafPage()", "==", "fFalse", "count", NULL };
    Setup1( cbf3LvlInternalPages )  = { "pv->FRootPage()", "==", "fFalse", "&&", "pv->FParentOfLeaf()", "==", "fTrue", "||", "pv->FRootPage()", "==", "fFalse", "&&", "pv->FParentOfLeaf()", "==", "fFalse", "&&", "pv->FLeafPage()", "==", "fFalse", "count", NULL };
    Setup1( cbfLeafPages )          = { "pv->FRootPage()", "==", "fFalse", "&&", "pv->FLeafPage()", "==", "fTrue", "count", NULL };

    Setup1( cbfImpossibleFlagCombo1 ) = { "pv->FParentOfLeaf()", "==", "fTrue", "&&", "pv->FLeafPage()", "==", "fTrue", "count", NULL };
    Setup1( cbfExcludedFromDump )   = { "fQuiesced", "==", "fFalse", "&&", "FExcludedFromDump()", "==", "fTrue", "count", NULL };

    Setup1( cbfTreePagesCheck )     = { "FBFIDatabasePage()", "==", "fTrue", "count", NULL };
    Setup2( cbfTreePagesCheck );

    if ( fVerbose )
    {

        ICBPage icbRealBufferSizePrevious = icbPageInvalid;
        for( ICBPage icb = icbPageInvalid; icb < icbPageMax; icb++ )
        {
            CHAR szIcb[15];
            CHAR szLimitLow[15];
            CHAR szRealBufferLimitLow[15];
            CHAR szLimitHigh[15];

            OSStrCbFormatA( szIcb, sizeof(szIcb), "%d", icb );
            OSStrCbFormatA( szLimitLow, sizeof(szLimitLow), "%d", g_rgcbPageSize[icb?icb-1:0] );
            OSStrCbFormatA( szRealBufferLimitLow, sizeof(szRealBufferLimitLow), "%d", g_rgcbPageSize[icbRealBufferSizePrevious] );
            OSStrCbFormatA( szLimitHigh, sizeof(szLimitHigh), "%d", g_rgcbPageSize[icb] );

            // this makes sure we only work with buffers ESE can already deal with

            if ( g_rgcbPageSize[icb] == CbBFGetBufferSize( g_rgcbPageSize[icb] ) )
            {

                icbRealBufferSizePrevious = icb;

                //  Plain missed dehydrations.
                //

                if ( Pdls()->CbPage() != (size_t)g_rgcbPageSize[icb] )
                {
                    CHAR * rgszArgsT[] = {
                                "FBFIDatabasePage()", "==", "fTrue",
                                    "&&", "bfdf", "<", "bfdfDirty",
                                    // maybe should be only errBFIPrereadUnverified, but we probably wouldn't 
                                    // dehydrate buffers in error state
                                    "&&", "err", ">=", "0",
                                    "&&", "pv->CbContiguousDehydrateRequired_()", ">", szRealBufferLimitLow,
                                    "&&", "pv->CbContiguousDehydrateRequired_()", "<=", szLimitHigh,
                                "count", NULL };
                    Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperPlainMissed[icb]), &(rgpiqCacheSum[cQA++]) ) );
                }
                else
                {
                    // For the last bucket we accept all (dirty, err'd) because they're all
                    // un-dehydratable to any lower ICBPage size.
                    CHAR * rgszArgsT[] = {
                                "FBFIDatabasePage()", "==", "fTrue",
                                    "&&", "pv->CbContiguousDehydrateRequired_()", ">", szRealBufferLimitLow,
                                    "&&", "pv->CbContiguousDehydrateRequired_()", "<=", szLimitHigh,
                                "||", "FBFIDatabasePage()", "==", "fTrue",
                                    "&&", "bfdf", ">=", "bfdfDirty",
                                "||", "FBFIDatabasePage()", "==", "fTrue",
                                    // not sure we should have this clause?
                                    "&&", "err", "<", "0",
                                "count", NULL };
                    Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperPlainMissed[icb]), &(rgpiqCacheSum[cQA++]) ) );
                }

                //  Re-org missed dehydrations.
                //

                if ( Pdls()->CbPage() != (size_t)g_rgcbPageSize[icb] )
                {
                    CHAR * rgszArgsT[] = {
                                "FBFIDatabasePage()", "==", "fTrue",
                                    "&&", "bfdf", "<", "bfdfDirty",
                                    "&&", "err", ">=", "0",
                                    "&&", "pv->CbReorganizedDehydrateRequired_()", ">", szRealBufferLimitLow,
                                    "&&", "pv->CbReorganizedDehydrateRequired_()", "<=", szLimitHigh,
                                "count", NULL };
                    Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperReorgMissed[icb]), &(rgpiqCacheSum[cQA++]) ) );
                }
                else
                {
                    CHAR * rgszArgsT[] = {
                                "FBFIDatabasePage()", "==", "fTrue",
                                    "&&", "pv->CbReorganizedDehydrateRequired_()", ">", szRealBufferLimitLow,
                                    "&&", "pv->CbReorganizedDehydrateRequired_()", "<=", szLimitHigh,
                                "||", "FBFIDatabasePage()", "==", "fTrue",
                                    "&&", "bfdf", ">=", "bfdfDirty",
                                "||", "FBFIDatabasePage()", "==", "fTrue",
                                    // not sure we should have this clause?
                                    "&&", "err", "<", "0",
                                "count", NULL };
                    Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperReorgMissed[icb]), &(rgpiqCacheSum[cQA++]) ) );
                }

                //  Dirty Dehydrations ...
                //

                if ( Pdls()->CbPage() != (size_t)g_rgcbPageSize[icb] )
                {
                    CHAR * rgszArgsT[] = {
                                "FBFIDatabasePage()", "==", "fTrue",
                                    "&&", "err", ">=", "0",
                                    "&&", "pv->CbContiguousDehydrateRequired_()", ">", szRealBufferLimitLow,
                                    "&&", "pv->CbContiguousDehydrateRequired_()", "<=", szLimitHigh,
                                "count", NULL };
                    Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperDirty[icb]), &(rgpiqCacheSum[cQA++]) ) );
                }
                else
                {
                    CHAR * rgszArgsT[] = {
                                "FBFIDatabasePage()", "==", "fTrue",
                                    "&&", "err", ">=", "0",
                                    "&&", "pv->CbContiguousDehydrateRequired_()", ">", szRealBufferLimitLow,
                                    "&&", "pv->CbContiguousDehydrateRequired_()", "<=", szLimitHigh,
                                "||", "FBFIDatabasePage()", "==", "fTrue",
                                    // not sure we should have this clause?
                                    "&&", "err", "<", "0",
                                "count", NULL };
                    Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperDirty[icb]), &(rgpiqCacheSum[cQA++]) ) );
                }

            } // existing buffer sizes

            //  Non-PowerOf2
            //

            if ( icb >= icbPage4KB )
            {

                if ( icb == icbPage4KB )
                {
                    CHAR * rgszArgsT[] = {
                                "FBFIDatabasePage()", "==", "fTrue",
                                    "&&", "bfdf", "<", "bfdfDirty",
                                    "&&", "err", ">=", "0",
                                    "&&", "pv->CbReorganizedDehydrateRequired_()", ">", "0",
                                    "&&", "pv->CbReorganizedDehydrateRequired_()", "<=", szLimitHigh,
                                "count", NULL };
                    Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperNonPowerOf2[icb]), &(rgpiqCacheSum[cQA++]) ) );
                }
                else if ( Pdls()->CbPage() != (size_t)g_rgcbPageSize[icb] )
                {
                    CHAR * rgszArgsT[] = {
                                "FBFIDatabasePage()", "==", "fTrue",
                                    "&&", "bfdf", "<", "bfdfDirty",
                                    "&&", "err", ">=", "0",
                                    "&&", "pv->CbReorganizedDehydrateRequired_()", ">", szLimitLow,
                                    "&&", "pv->CbReorganizedDehydrateRequired_()", "<=", szLimitHigh,
                                "count", NULL };
                    Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperNonPowerOf2[icb]), &(rgpiqCacheSum[cQA++]) ) );
                }
                else
                {
                    CHAR * rgszArgsT[] = {
                                "FBFIDatabasePage()", "==", "fTrue",
                                    "&&", "err", ">=", "0",
                                    "&&", "pv->CbReorganizedDehydrateRequired_()", ">", szLimitLow,
                                    "&&", "pv->CbReorganizedDehydrateRequired_()", "<=", szLimitHigh,
                                "||", "FBFIDatabasePage()", "==", "fTrue",
                                    "&&", "bfdf", ">=", "bfdfDirty",
                                "||", "FBFIDatabasePage()", "==", "fTrue",
                                    // not sure we should have this clause?
                                    "&&", "err", "<", "0",
                                "count", NULL };
                    Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperNonPowerOf2[icb]), &(rgpiqCacheSum[cQA++]) ) );
                }
            } // 4 KB or greater buffer sizes

            //  u-Pages
            //

            if ( Pdls()->CbPage() != (size_t)g_rgcbPageSize[icb] )
            {
                CHAR * rgszArgsT[] = {
                            "FBFIDatabasePage()", "==", "fTrue",
                                "&&", "bfdf", "<", "bfdfDirty",
                                "&&", "err", ">=", "0",
                                "&&", "pv->CbReorganizedDehydrateRequired_()", ">", szLimitLow,
                                "&&", "pv->CbReorganizedDehydrateRequired_()", "<=", szLimitHigh,
                            "count", NULL };
                Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperuPages[icb]), &(rgpiqCacheSum[cQA++]) ) );
            }
            else
            {
                CHAR * rgszArgsT[] = {
                            "FBFIDatabasePage()", "==", "fTrue",
                                // not sure we should have this clause?
                                "&&", "err", ">=", "0",
                                "&&", "pv->CbReorganizedDehydrateRequired_()", ">", szLimitLow,
                                "&&", "pv->CbReorganizedDehydrateRequired_()", "<=", szLimitHigh,
                            "||", "FBFIDatabasePage()", "==", "fTrue",
                                "&&", "bfdf", ">=", "bfdfDirty",
                            "||", "FBFIDatabasePage()", "==", "fTrue",
                                // not sure we should have this clause?
                                "&&", "err", "<", "0",
                            "count", NULL };
                Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperuPages[icb]), &(rgpiqCacheSum[cQA++]) ) );
            }

            //  Full Solution: u-Pages, Non-PowerOf2, Dirty (4KB, not-u), no missed cases.
            //

            //      This will not work for 2 KB page size ... but no one cares about 2 KB pages.
            if ( icb < icbPage4KB )
            {
                CHAR * rgszArgsT[] = {
                            "FBFIDatabasePage()", "==", "fTrue",
                                "&&", "bfdf", "<", "bfdfDirty",
                                "&&", "err", ">=", "0",
                                "&&", "pv->CbReorganizedDehydrateRequired_()", ">", szLimitLow,
                                "&&", "pv->CbReorganizedDehydrateRequired_()", "<=", szLimitHigh,
                            "count", NULL };
                Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperUDNPu[icb]), &(rgpiqCacheSum[cQA++]) ) );
            }
            else if ( icb == icbPage4KB )
            {
                CHAR * rgszArgsT[] = {
                            "FBFIDatabasePage()", "==", "fTrue",
                                "&&", "bfdf", "<", "bfdfDirty",
                                "&&", "err", ">=", "0",
                                "&&", "pv->CbReorganizedDehydrateRequired_()", ">", szLimitLow,
                                "&&", "pv->CbReorganizedDehydrateRequired_()", "<=", szLimitHigh,
                            "||", "FBFIDatabasePage()", "==", "fTrue",
                                "&&", "bfdf", ">=", "bfdfDirty",
                                "&&", "pv->CbReorganizedDehydrateRequired_()", ">", "0",
                                "&&", "pv->CbReorganizedDehydrateRequired_()", "<=", szLimitHigh,
                            "count", NULL };
                Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperUDNPu[icb]), &(rgpiqCacheSum[cQA++]) ) );
            }
            else if ( Pdls()->CbPage() != (size_t)g_rgcbPageSize[icb] )
            {
                CHAR * rgszArgsT[] = {
                            "FBFIDatabasePage()", "==", "fTrue",
                                // not sure we should have this clause?
                                "&&", "err", ">=", "0",
                                "&&", "pv->CbReorganizedDehydrateRequired_()", ">", szLimitLow,
                                "&&", "pv->CbReorganizedDehydrateRequired_()", "<=", szLimitHigh,
                            "count", NULL };
                Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperUDNPu[icb]), &(rgpiqCacheSum[cQA++]) ) );
            }
            else
            {
                CHAR * rgszArgsT[] = {
                            "FBFIDatabasePage()", "==", "fTrue",
                                // not sure we should have this clause?
                                "&&", "err", ">=", "0",
                                "&&", "pv->CbReorganizedDehydrateRequired_()", ">", szLimitLow,
                                "&&", "pv->CbReorganizedDehydrateRequired_()", "<=", szLimitHigh,
                            "||", "FBFIDatabasePage()", "==", "fTrue",
                                // not sure we should have this clause?
                                "&&", "err", "<", "0",
                            "count", NULL };
                Call( ErrIQCreateIterQueryCount( &g_iedBf, _countof(rgszArgsT)-1, rgszArgsT, (void*)&(rgcbfHyperUDNPu[icb]), &(rgpiqCacheSum[cQA++]) ) );
            }

        } // for each icbBuffer size ...

        Setup2( cbfSinglePageTrees );
        Setup2( cbf2LvlRootPages );
        Setup2( cbf3LvlRootPages );
        Setup2( cbf3LvlInternalPages );
        Setup2( cbfLeafPages );
        Setup2( cbfImpossibleFlagCombo1 );
        Setup2( cbfExcludedFromDump );
    } // fVerbose

    // Actually perform the query ...
    //

    dprintf("CacheSum is executing %d simultaneous queries.\n", cQA );

#ifndef DEBUG
    //  One more query ... this is special query clause in that it prints out stuff during the query.
    dprintf( "\n" );
    EDBGPrintfDml("<link cmd=\"!ese cachequery sxwl.FLatched() == fTrue && err != -260 && fAvailable == fFalse && fQuiesced == fFalse || sxw.CWaiters() &gt;= 1 print:ifmp,pgno,err,sxwl.CWaiters(),sxwl.CSharers(),sxwl.FExclusiveLatched(),sxwl.FWriteLatched(),sxwl.CWaitSharedLatch(),sxwl.CWaitExclusiveLatch(),sxwl.CWaitWriteLatch()\">Latched Pages</link>:\n" );
#ifdef _WIN64
    dprintf(      " pbf=xxxxxxxxxxxxxxxx:" );
#else
    dprintf(      " pbf=xxxxxxxx:" );
#endif
    dprintf(      "        ifmp,     pgno,   err, cWaiters, cSharers,fXLatch,fWLatch,cSWaiters,cXWaiters,cWWaiters.\n" );

    //  We dump Latched BFs, BUT exclude BFs that have no waiters AND are 
    //      1. waiting for pre-reads to complete (err = -260 / errBFIPageFaultPending) OR
    //      2. an available buffer (note: ESE leaves such buffers w-latched in the avail pool) OR
    //      3. a quiesced buffer (note: ESE leaves such buffers w-latched in this state)
    //      ? Do we need something for recently grown buffers / non-resident ?
    CHAR * rgszLatchesOfInterest [] = {
                "sxwl.FLatched()", "==", "fTrue",       //  page is in a latched state
                    "&&", "err", "!=", "-260",          //  not in pre-read
                    "&&", "fAvailable", "==", "fFalse", //  not available (these are left w-latched)
                    "&&", "fQuiesced", "==", "fFalse",  //  not quiesced (these are also left w-latched)
                "||", "sxwl.CWaiters()", ">=", "1",     //  or has 1 or more waiters ...
            "print:ifmp,pgno,err,sxwl.CWaiters(),sxwl.CSharers(),sxwl.FExclusiveLatched(),sxwl.FWriteLatched(),sxwl.CWaitSharedLatch(),sxwl.CWaitExclusiveLatch(),sxwl.CWaitWriteLatch()", NULL };
    Call( ErrIQCreateIterQuery( &g_iedBf, _countof(rgszLatchesOfInterest)-1, rgszLatchesOfInterest, &(rgpiqCacheSum[cQA++]) ) );
#endif

    dprintf( "\n" );

    Call( ErrEDBGQueryCache( cQA, rgpiqCacheSum ) );
        dprintf("\n" );

    // process results ...
    //

    #define PctOf( Num, Denom )     ( ( Num == 0 && Denom == 0 ) ? 0.0 : ( (float)(Num) / (float)(Denom) * (float)100.0 ) )

    EDBGPrintfDml( "Cache Summary:\n" );
    EDBGPrintfDml("\n" );
    EDBGPrintfDml( "    Total Buffers...:  %8I64d\n", cbfTotal );
    EDBGPrintfDml( "               <link cmd=\"!ese cachequery fAvailable == fTrue count\">Avail</link>:  %8I64d (%6.2f%%)\n", cbfAvail, PctOf( cbfAvail, cbfTotal ) );
    EDBGPrintfDml( "            <link cmd=\"!ese cachequery fQuiesced == fTrue count\">Quiesced</link>:  %8I64d (%6.2f%%) (Addressable %8I64d)\n", cbfQuiesced, PctOf( cbfQuiesced, cbfTotal ), cbfQuiescedAddressable );

    if ( cbfUnquiescedTooHigh )
    {
        dprintf( "                                    WARNING: cbfUnquiescedTooHigh = %8I64d\n", cbfUnquiescedTooHigh );
    }
    if ( cbfAvailTooHigh )
    {
        dprintf( "                                    WARNING: cbfAvailTooHigh = %8I64d\n", cbfAvailTooHigh );
    }
    AssertEDBG( cbfCleanMapped == cbfCleanMappedCHECK );        //  or the values after clean may be inaccurate, and should be moved.
    EDBGPrintfDml( "               <link cmd=\"!ese cachequery fAvailable == fFalse &amp;&amp; fQuiesced == fFalse &amp;&amp; bfdf == bfdfClean count\">Clean</link>:  %8I64d (%6.2f%%)", cbfClean, PctOf( cbfClean, cbfTotal ) );
    if ( cbfMappedTot )
    {
        EDBGPrintfDml( "   [Mapped = %5I64d / %5.1f%%, PageAlloc = %5I64d, FracCommit = %5I64d]\n",
                        cbfMappedClean, PctOf( cbfMappedClean, cbfClean ),
                        cbfClean - cbfMappedClean - cbfFracCommitClean, //  that _should_ be ~right?
                        cbfFracCommitClean );
    }
    else
    {
        EDBGPrintfDml( "\n" );
    }
    AssertEDBG( cbfCopyMapped == cbfCopyMappedCHECK );      //  or the values after untidy may be inaccurate, and should be moved.
    EDBGPrintfDml( "              <link cmd=\"!ese cachequery bfdf == bfdfUntidy count\">Untidy</link>:  %8I64d (%6.2f%%)", cbfUntidy, PctOf( cbfUntidy, cbfTotal ) );
    if ( cbfMappedTot )
    {
        //  Note the JustReMapped category applies both to the fact that they were probably (although not always) recently 
        //  remapped because they are in the WriteCompleted state, but not actually clean state.  AND 
        EDBGPrintfDml( "   [JustReMapped = %5I64d / %5.1f%%, Copied = %5I64d / %5.1f%%, PageAlloc = %5I64d, FracCommit = %5I64d]\n",
                            cbfMappedUntidyWC, PctOf( cbfMappedUntidyWC, cbfUntidy ),
                            cbfMappedUntidy - cbfMappedUntidyWC, PctOf( cbfMappedUntidy - cbfMappedUntidyWC, cbfUntidy ),
                            cbfUntidy - cbfMappedUntidy,
                            cbfFracCommitUntidy );
    }
    else
    {
        EDBGPrintfDml( "\n" );
    }
    EDBGPrintfDml( "               <link cmd=\"!ese cachequery bfdf == bfdfDirty count\">Dirty</link>:  %8I64d (%6.2f%%)", cbfDirty, PctOf( cbfDirty, cbfTotal ) );
    if ( cbfMappedTot )
    {
        EDBGPrintfDml( "   [JustReMapped = %5I64d / %5.1f%%, Copied = %5I64d / %5.1f%%, PageAlloc = %5I64d, FracCommit = %5I64d]\n",
                            cbfMappedDirtyWC, PctOf( cbfMappedDirtyWC, cbfDirty ),
                            cbfMappedDirty - cbfMappedDirtyWC, PctOf( cbfMappedDirty - cbfMappedDirtyWC, cbfDirty ),
                            cbfDirty - cbfMappedDirty,
                            cbfFracCommitDirty );
    }
    else
    {
        EDBGPrintfDml( "\n" );
    }
    EDBGPrintfDml( "              <link cmd=\"!ese cachequery bfdf == bfdfFilthy count\">Filthy</link>:  %8I64d (%6.2f%%)", cbfFilthy, PctOf( cbfFilthy, cbfTotal ) );
    if ( cbfMappedTot )
    {
        EDBGPrintfDml( "   [JustReMapped = %5I64d / %5.1f%%, Copied = %5I64d / %5.1f%%, PageAlloc = %5I64d, FracCommit = %5I64d]\n",
                            cbfMappedFilthyWC, PctOf( cbfMappedFilthyWC, cbfFilthy ),
                            cbfMappedFilthy - cbfMappedFilthyWC, PctOf( cbfMappedFilthy - cbfMappedFilthyWC, cbfFilthy ),
                            cbfFilthy - cbfMappedFilthy,
                            cbfFracCommitFilthy );
    }
    else
    {
        EDBGPrintfDml( "\n" );
    }
    EDBGPrintfDml( "\n" );

    EDBGPrintfDml( "    I/O Related.....:  %8I64d\n", cbfErrBFIPageFaultPending + cbfWrnBFPageFlushPending );
    EDBGPrintfDml( "             <link cmd=\"!ese cachequery err == -260 count\">Reading</link>:  %8I64d\n", cbfErrBFIPageFaultPending );
    EDBGPrintfDml( "             <link cmd=\"!ese cachequery err == 204 &amp;&amp; pWriteSignalComplete == 0 count\">Writing</link>:  %8I64d\n", cbfWrnBFPageFlushPending );
    EDBGPrintfDml( "         <link cmd=\"!ese cachequery err == -261 count\">NotVerified</link>:  %8I64d (either being pre-read, or was pre-read but unused)\n", cbfErrBFIPageNotVerified );
    EDBGPrintfDml( "       <link cmd=\"!ese cachequery err == 204 &amp;&amp; pWriteSignalComplete != 0 count\">WriteComplete</link>:  %8I64d (write completions with err: %I64d)\n", cbfWrnBFPageFlushCompleted, cbfWrnBFPageFlushCompletedWithErr );
    EDBGPrintfDml( "\n" );

#ifndef DEBUG
    EDBGPrintfDml( "    <link cmd=\"!ese cachequery fAvailable == fFalse &amp;&amp; fQuiesced == fFalse &amp;&amp; err != -260 &amp;&amp; sxwl.FLatched() == fTrue count\">Total Latched...</link>:  %8I64d (%6.2f%%, excluding avail, memory, and read-IO)\n", cbfLatched, PctOf( cbfLatched, cbfTotal ) );
    EDBGPrintfDml( "                <link cmd=\"!ese cachequery sxwl.FSharedLatched() == fTrue count\">Read</link>:  %8I64d (%6.2f%% of latched)\n", cbfSharedLatched, PctOf( cbfSharedLatched, cbfLatched ) );
    EDBGPrintfDml( "           <link cmd=\"!ese cachequery sxwl.FExclusiveLatched() == fTrue count\">Exclusive</link>:  %8I64d (%6.2f%% of latched)\n", cbfExclusiveLatched, PctOf( cbfExclusiveLatched, cbfLatched ) );
    EDBGPrintfDml( "               <link cmd=\"!ese cachequery fAvailable == fFalse &amp;&amp; fQuiesced == fFalse &amp;&amp; err != -260 &amp;&amp; sxwl.FWriteLatched() == fTrue count\">Write</link>:  %8I64d (%6.2f%% of latched)\n", cbfWriteLatched, PctOf( cbfWriteLatched, cbfLatched ) );
    EDBGPrintfDml( "            <link cmd=\"!ese cachequery sxwl.CSharers() &gt; 0 accum:sxwl.CSharers()\">cSharers</link>:  %8I64d amongst <link cmd=\"!ese cachequery sxwl.FSharedLatched() == fTrue count\">%I64d buffers</link>\n", cSharers, cbfSharedLatched );
    EDBGPrintfDml( "            <link cmd=\"!ese cachequery sxwl.CWaiters() &gt; 0 accum:sxwl.CWaiters()\">cWaiters</link>:  %8I64d (<link cmd=\"!ese cachequery sxwl.CWaitSharedLatch() &gt; 0 accum:sxwl.CWaitSharedLatch()\">Shared</link> = %I64d, <link cmd=\"!ese cachequery sxwl.CWaitExclusiveLatch() &gt; 0 accum:sxwl.CWaitExclusiveLatch()\">Exclusive</link> = %I64d, <link cmd=\"!ese cachequery sxwl.CWaitWriteLatch() &gt; 0 accum:sxwl.CWaitWriteLatch()\">Write</link> = %I64d) \n", cWaiters, cSharedWaiters, cExclusiveWaiters, cWriteWaiters );
    EDBGPrintfDml( "\n" );
#endif

    EDBGPrintfDml( "    Other Statistics:\n" );
    EDBGPrintfDml( "          <link cmd=\"!ese cachequery fAvailable == fFalse &amp;&amp; icbBuffer != %hs count\">Dehydrated</link>:  %8I64d\n", szIcbPage, cbfDehydrated );
    EDBGPrintfDml( "       <link cmd=\"!ese cachequery prceUndoInfoNext != NULL count\">Def Undo Info</link>:  %8I64d\n", cbfUndoInfo );
    EDBGPrintfDml( "           <link cmd=\"!ese cachequery pbfTimeDepChainNext != NULL count\">Versioned</link>:  %8I64d\n", cbfVersioned );
    EDBGPrintfDml( "      <link cmd=\"!ese cachequery fOlderVersion == fTrue count\">Older Versions</link>:  %8I64d\n", cbfOlderVersions );
    EDBGPrintfDml( "        <link cmd=\"!ese cachequery ifmp != 0x7fffffff &amp;&amp; lrukic.FSuperColded() == fTrue count\">Super Colded</link>:  %8I64d (Int/Ext: <link cmd=\"!ese cachequery ifmp != 0x7fffffff &amp;&amp; lrukic.FSuperColded() == fTrue &amp;&amp; fOlderVersion == fTrue count\">%d</link> / <link cmd=\"!ese cachequery ifmp != 0x7fffffff &amp;&amp; lrukic.FSuperColded() == fTrue &amp;&amp; fOlderVersion == fFalse count\">%d</link>, MisIndexed: <link cmd=\"!ese cachequery ifmp != 0x7fffffff &amp;&amp; lrukic.FSuperColded() == fTrue &amp;&amp; lrukic.FSuperColdedIndex() == fFalse count\">%d</link>)\n", cbfSuperColdedInt + cbfSuperColdedExt, cbfSuperColdedInt, cbfSuperColdedExt, cbfSuperColdMisIndexed );
    EDBGPrintfDml( "      <link cmd=\"!ese cachequery FBFIDatabasePage() == fTrue count\">Database Pages</link>:  %8I64d\n", cbfTreePagesCheck );
    EDBGPrintfDml( "              <link cmd=\"!ese cachequery ifmp != 0x7fffffff &amp;&amp; bfat == bfatViewMapped count\">Mapped</link>:  %8I64d (Non-Copied: %I64d)\n", cbfCleanMapped + cbfCopyMapped, cbfCleanMapped + ( cbfCopyMapped - cbfNonCleanNonWriteCompletedMapped ) );
    EDBGPrintfDml( "           <link cmd=\"!ese cachequery ifmp != 0x7fffffff &amp;&amp; fAbandoned == fTrue count\">Abandoned</link>:  %8I64d (%4.2f%%)\n", cbfAbandoned, ( cbfTotal > cbfAvail ) ? ( (double)cbfAbandoned / ( cbfTotal - cbfAvail ) * 100.0 ) : 0.0 );

    EDBGPrintfDml( "\n" );

    EDBGPrintfDml( "Buffers in err states:\n" );
    IQPrintStats( phistoErrs, ShortPrintVal, 0, 2, 0, false );
    EDBGPrintfDml( "\n" );

    if ( cbfMappedTot )
    {
        EDBGPrintfDml( "Alloc Type states (bfatNone=%d, bfatFracCommit=%d, bfatViewMapped=%d, bfatPageAlloc=%d):\n",
                        bfatNone, bfatFracCommit, bfatViewMapped, bfatPageAlloc );
        IQPrintStats( phistoUsedBfat, ShortPrintVal, 0, 2, 0, false );
        EDBGPrintfDml( "\n" );
    }

    EDBGPrintfDml( "Resident states (bfrsNotCommitted=%d, bfrsNewlyCommitted=%d, bfrsNotResident=%d, bfrsResident=%d):\n",
                    bfrsNotCommitted, bfrsNewlyCommitted, bfrsNotResident, bfrsResident );
    IQPrintStats( phistoBfrs, ShortPrintVal, 0, 2, 0, false );
    EDBGPrintfDml( "\n" );

    EDBGPrintfDml( "Buffers by IFMP:\n" );
    IQPrintStats( phistoIfmp, UlongPrintVal /* or DWORDPrintVal? */, 0, 4, 0, false );
    EDBGPrintfDml( "\n" );

    CStats::ERR csErr;

    EDBGPrintfDml( "Buffers by k-Pool:\n" );
    if ( CStats::ERR::errSuccess != ( csErr = phistoKPool->ErrReset() ) )
    {
        dprintf( "   ERROR: %d reseting stat class.\n", csErr );
        return;
    }
    else
    {
        STAT::CHITS cbfK0 = 0;
        STAT::CHITS cbfK1 = 0;
        STAT::CHITS cbfK2 = 0;
        
        csErr = phistoKPool->ErrGetSampleHits( 0, &cbfK0 );
        if ( CStats::ERR::errSuccess != csErr )
        {
            AssertEDBG( CStats::ERR::wrnOutOfSamples == csErr );
            cbfK1 = 0;
        }
        csErr = phistoKPool->ErrGetSampleHits( 1, &cbfK1 );
        if ( CStats::ERR::errSuccess != csErr )
        {
            AssertEDBG( CStats::ERR::wrnOutOfSamples == csErr );
            cbfK1 = 0;
        }
        csErr = phistoKPool->ErrGetSampleHits( 2, &cbfK2 );
        if ( CStats::ERR::errSuccess != csErr )
        {
            AssertEDBG( CStats::ERR::wrnOutOfSamples == csErr );
            cbfK2 = 0;
        }

#ifdef DEBUG
        STAT::CHITS cbfFake;
        AssertEDBG( CStats::ERR::wrnOutOfSamples == phistoKPool->ErrGetSampleHits( 3, &cbfFake ) );
#endif

        EDBGPrintfDml( "   k=0: %8d  (preread untouched: %d, super colded[int/ext]: %d / %d)\n",
                            cbfK0 + cbfSuperColdedInt + cbfSuperColdedExt,
                            cbfK0,
                            cbfSuperColdedInt,
                            cbfSuperColdedExt );
        EDBGPrintfDml( "   k=1: %8d \n", cbfK1 );
        EDBGPrintfDml( "   k=2: %8d \n", cbfK2 );

        (void)phistoKPool->ErrReset();
    }
    EDBGPrintfDml( "\n" );

    if ( fVerbose )
    {
        EDBGPrintfDml( "Buffers by B+ Tree Hierarchy ( --- means unknown / unknowable):\n" );
        EDBGPrintfDml( "                            Root Internal     Leaf\n" );
        EDBGPrintfDml( "   Single Page Trees:   -------- -------- %8I64d\n", cbfSinglePageTrees );
        EDBGPrintfDml( "   2-Level Trees:       %8I64d -------- --------\n", cbf2LvlRootPages );
        EDBGPrintfDml( "   3+Level Trees:       %8I64d %8I64d --------\n", cbf3LvlRootPages, cbf3LvlInternalPages );
        EDBGPrintfDml( "   Leaves of Trees:     -------- -------- %8I64d\n", cbfLeafPages );

        AssertEDBG( cbfImpossibleFlagCombo1 == 0 );
        ULONG cBFMayBeRemovedFromCrashDumpDebuggee = (ULONG)-1;
        if ( FReadGlobal( "cBFMayBeRemovedFromCrashDump", &cBFMayBeRemovedFromCrashDumpDebuggee ) )
        {
            AssertEDBG( cbfExcludedFromDump == cBFMayBeRemovedFromCrashDumpDebuggee );
        }
        AssertEDBG( cbfTreePagesCheck - cbfExcludedFromDump == cbfSinglePageTrees + cbf2LvlRootPages + cbf3LvlRootPages + cbf3LvlInternalPages + cbfLeafPages );
        AssertEDBG( cbfTreePagesCheck >= cbfSinglePageTrees + cbf2LvlRootPages + cbf3LvlRootPages + cbf3LvlInternalPages + cbfLeafPages );
        AssertEDBG( cbfTreePagesCheck - cbfExcludedFromDump <= cbfSinglePageTrees + cbf2LvlRootPages + cbf3LvlRootPages + cbf3LvlInternalPages + cbfLeafPages );

        EDBGPrintfDml( "\n" );
    }

    EDBGPrintfDml( "Hyper-Cached Buffers:\n" );

    if ( phistoIcbBuffer->C() == 0 )
    {
        dprintf( "      No samples!\n" );
        return;
    }
    if ( CStats::ERR::errSuccess != ( csErr = phistoIcbBuffer->ErrReset() ) )
    {
        dprintf( "      ERROR: %d reseting stat class.\n", csErr );
        return;
    }

    __int64 cbRAMTotal = 0;
    __int64 cbEffectiveTotal = 0;
    ULONG cbfTotalCheck = 0;
    ULONG cbfFullPageSized = 0;

    ICBPage icbPage = icbPageInvalid;

    ULONG   cbfHyperPlainMissedTotal = 0;
    __int64 cbHyperPlainMissedTotal = 0;
    ULONG   cbfHyperReorgMissedTotal = 0;
    __int64 cbHyperReorgMissedTotal = 0;

    ULONG   cbfHyperNonPowerOf2Total = 0;
    __int64 cbHyperNonPowerOf2Total = 0;

    ULONG   cbfHyperDirtyTotal = 0;
    __int64 cbHyperDirtyTotal = 0;

    ULONG   cbfHyperuPagesTotal = 0;
    __int64 cbHyperuPagesTotal = 0;

    ULONG   cbfHyperUDNPuTotal = 0;
    __int64 cbHyperUDNPuTotal = 0;

    EDBGPrintfDml( "  [icb] xxx KB, cbEffec MB,   cbCache(   cbf)" );
    if ( fVerbose )
    {
        EDBGPrintfDml( ",           Missed,      ReorgMissed,      NonPowerOf2,            Dirty,           uPages,             Full\n" );
    }
    EDBGPrintfDml( "\n" );

    for( ICBPage icb = icbPageInvalid; icb < icbPageMax; icb++ )
    {

        CHITS hits;
        csErr = phistoIcbBuffer->ErrGetSampleHits( (SAMPLE)icb, &hits );
        if ( csErr != CStats::ERR::errSuccess )
        {
            break;
        }

        __int64 cbRAM       = (__int64)hits * (__int64)g_rgcbPageSize[icb];
        __int64 cbEffective = (__int64)hits * (__int64)Pdls()->CbPage();

        CHAR szBufferSize[10];
        if ( g_rgcbPageSize[icb] < 1024 )
        {
            OSStrCbFormatA( szBufferSize, sizeof(szBufferSize), "%3d B ", g_rgcbPageSize[icb] );
        }
        else
        {
            OSStrCbFormatA( szBufferSize, sizeof(szBufferSize), "%3d KB", g_rgcbPageSize[icb] / 1024 );
        }

        AssertEDBG( hits <= cbfTotal - cbfAvail - cbfQuiesced );

        //  Add regular summaries up.
        //

        cbfTotalCheck += (ULONG)hits;
        cbRAMTotal += cbRAM;
        cbEffectiveTotal += cbEffective;

        if ( Pdls()->CbPage() == (size_t)g_rgcbPageSize[icb] )
        {
            icbPage = icb;
            cbfFullPageSized = (ULONG)hits;
        }

        //  Print out totals of buffers and size of buffers at this icbPage size.
        //

        EDBGPrintfDml( "  [%03d] %hs, %6d.%03d, %5d.%03d(%6d)",
                        (ULONG)icb, szBufferSize,
                        DwMBs( cbEffective ), DwMBsFracKB( cbEffective ),
                        DwMBs( cbRAM ), DwMBsFracKB( cbRAM ), hits );
        if ( fVerbose )
        {
            EDBGPrintfDml( ",%5d.%03d(%6d),%5d.%03d(%6d),%5d.%03d(%6d),%5d.%03d(%6d),%5d.%03d(%6d),%5d.%03d(%6d)",
                    DwMBs( CbBuffSize( rgcbfHyperPlainMissed[icb], g_rgcbPageSize[icb] ) ),
                        DwMBsFracKB( CbBuffSize( rgcbfHyperPlainMissed[icb], g_rgcbPageSize[icb] ) ),
                        rgcbfHyperPlainMissed[icb],
                    DwMBs( CbBuffSize( rgcbfHyperReorgMissed[icb], g_rgcbPageSize[icb] ) ),
                        DwMBsFracKB( CbBuffSize( rgcbfHyperReorgMissed[icb], g_rgcbPageSize[icb] ) ),
                        rgcbfHyperReorgMissed[icb],
                    DwMBs( CbBuffSize( rgcbfHyperNonPowerOf2[icb], g_rgcbPageSize[icb] ) ),
                        DwMBsFracKB( CbBuffSize( rgcbfHyperNonPowerOf2[icb], g_rgcbPageSize[icb] ) ),
                        rgcbfHyperNonPowerOf2[icb],
                    DwMBs( CbBuffSize( rgcbfHyperDirty[icb], g_rgcbPageSize[icb] ) ),
                        DwMBsFracKB( CbBuffSize( rgcbfHyperDirty[icb], g_rgcbPageSize[icb] ) ),
                        rgcbfHyperDirty[icb],
                    DwMBs( CbBuffSize( rgcbfHyperuPages[icb], g_rgcbPageSize[icb] ) ),
                        DwMBsFracKB( CbBuffSize( rgcbfHyperuPages[icb], g_rgcbPageSize[icb] ) ),
                        rgcbfHyperuPages[icb],
                    DwMBs( CbBuffSize( rgcbfHyperUDNPu[icb], g_rgcbPageSize[icb] ) ),
                        DwMBsFracKB( CbBuffSize( rgcbfHyperUDNPu[icb], g_rgcbPageSize[icb] ) ),
                        rgcbfHyperUDNPu[icb]
                    );

            cbfHyperPlainMissedTotal += rgcbfHyperPlainMissed[icb];
            cbHyperPlainMissedTotal += CbBuffSize( rgcbfHyperPlainMissed[icb], g_rgcbPageSize[icb] );
            cbfHyperReorgMissedTotal += rgcbfHyperReorgMissed[icb];
            cbHyperReorgMissedTotal += CbBuffSize( rgcbfHyperReorgMissed[icb], g_rgcbPageSize[icb] );

            cbfHyperNonPowerOf2Total += rgcbfHyperNonPowerOf2[icb];
            cbHyperNonPowerOf2Total += CbBuffSize( rgcbfHyperNonPowerOf2[icb], g_rgcbPageSize[icb] );

            cbfHyperDirtyTotal += rgcbfHyperDirty[icb];
            cbHyperDirtyTotal += CbBuffSize( rgcbfHyperDirty[icb], g_rgcbPageSize[icb] );
            
            cbfHyperuPagesTotal += rgcbfHyperuPages[icb];
            cbHyperuPagesTotal += CbBuffSize( rgcbfHyperuPages[icb], g_rgcbPageSize[icb] );

            cbfHyperUDNPuTotal += rgcbfHyperUDNPu[icb];
            cbHyperUDNPuTotal += CbBuffSize( rgcbfHyperUDNPu[icb], g_rgcbPageSize[icb] );
        }
        EDBGPrintfDml( "\n" );
    }   // for each icb buffer size

    EDBGPrintfDml( "\n" );

    EDBGPrintfDml( "  [tot] xxx KB,%7I64d.%03d, %5d.%03d(%6d)",
                    DwMBs( cbEffectiveTotal ), DwMBsFracKB( cbEffectiveTotal ),
                    DwMBs( cbRAMTotal ), DwMBsFracKB( cbRAMTotal ), cbfTotalCheck );
    if ( fVerbose )
    {
        EDBGPrintfDml( ",%5d.%03d(%6d),%5d.%03d(%6d),%5d.%03d(%6d),%5d.%03d(%6d),%5d.%03d(%6d),%5d.%03d(%6d)",
                        DwMBs( cbHyperPlainMissedTotal ), DwMBsFracKB( cbHyperPlainMissedTotal ), cbfHyperPlainMissedTotal,
                        DwMBs( cbHyperReorgMissedTotal ), DwMBsFracKB( cbHyperReorgMissedTotal ), cbfHyperReorgMissedTotal,
                        DwMBs( cbHyperNonPowerOf2Total ), DwMBsFracKB( cbHyperNonPowerOf2Total ), cbfHyperNonPowerOf2Total,
                        DwMBs( cbHyperDirtyTotal ), DwMBsFracKB( cbHyperDirtyTotal ), cbfHyperDirtyTotal,
                        DwMBs( cbHyperuPagesTotal ), DwMBsFracKB( cbHyperuPagesTotal ), cbfHyperuPagesTotal,
                        DwMBs( cbHyperUDNPuTotal ), DwMBsFracKB( cbHyperUDNPuTotal ), cbfHyperUDNPuTotal );
        EDBGPrintfDml( "\n" );

        EDBGPrintfDml( "  [%%%%%%] vs. current,      ,            %5.1f%%,           %5.1f%%,           %5.1f%%,           %5.1f%%,           %5.1f%%,           %5.1f%%,           %5.1f%%\n",
                        100.0, // real answer is to accumulate a cbCurrent from the size of all the hyper cached buffers, and check that against cbRAMTotal (should always == 100)
                        PctOf( cbHyperPlainMissedTotal, cbRAMTotal ),
                        PctOf( cbHyperReorgMissedTotal, cbRAMTotal ),
                        PctOf( cbHyperNonPowerOf2Total, cbRAMTotal ),
                        PctOf( cbHyperDirtyTotal, cbRAMTotal ),
                        PctOf( cbHyperuPagesTotal, cbRAMTotal ),
                        PctOf( cbHyperUDNPuTotal, cbRAMTotal ) );
    }
    else
    {
        EDBGPrintfDml( "\n" );
    }

    EDBGPrintfDml( "  [%%%%%%] vs. logical,      ,            %5.1f%%",
                    PctOf( cbRAMTotal, cbEffectiveTotal ) );
    if ( fVerbose )
    {
        EDBGPrintfDml( ",           %5.1f%%,           %5.1f%%,           %5.1f%%,           %5.1f%%,           %5.1f%%,           %5.1f%%",
                        PctOf( cbHyperPlainMissedTotal, cbEffectiveTotal ),
                        PctOf( cbHyperReorgMissedTotal, cbEffectiveTotal ),
                        PctOf( cbHyperNonPowerOf2Total, cbEffectiveTotal ),
                        PctOf( cbHyperDirtyTotal, cbEffectiveTotal ),
                        PctOf( cbHyperuPagesTotal, cbEffectiveTotal ),
                        PctOf( cbHyperUDNPuTotal, cbEffectiveTotal ) );
    }

    EDBGPrintfDml( "\n" );

    EDBGPrintfDml( "  [%%%%%%] dehydrated%%,      ,            %5.1f%%",
                    PctOf( cbfTotalCheck - cbfFullPageSized, cbfTotalCheck ) );
    if ( fVerbose )
    {
        EDBGPrintfDml( ",           %5.1f%%,           %5.1f%%,           %5.1f%%,           %5.1f%%,           %5.1f%%,           %5.1f%%",
                        PctOf( cbfTotalCheck - rgcbfHyperPlainMissed[icbPage], cbfTotalCheck ),
                        PctOf( cbfTotalCheck - rgcbfHyperReorgMissed[icbPage], cbfTotalCheck ),
                        PctOf( cbfTotalCheck - rgcbfHyperNonPowerOf2[icbPage], cbfTotalCheck ),
                        PctOf( cbfTotalCheck - rgcbfHyperDirty[icbPage], cbfTotalCheck ),
                        PctOf( cbfTotalCheck - rgcbfHyperuPages[icbPage], cbfTotalCheck ),
                        PctOf( cbfTotalCheck - rgcbfHyperUDNPu[icbPage], cbfTotalCheck ) );

        EDBGPrintfDml( "\n" );

        //  if this doesn't add up, we've probably misssed something
        AssertEDBG( cbfTotalCheck == cbfHyperPlainMissedTotal );
        AssertEDBG( cbfTotalCheck == cbfHyperReorgMissedTotal );
        AssertEDBG( cbfTotalCheck == cbfHyperNonPowerOf2Total );
        AssertEDBG( cbfTotalCheck == cbfHyperDirtyTotal );
        AssertEDBG( cbfTotalCheck == cbfHyperuPagesTotal );
        AssertEDBG( cbfTotalCheck == cbfHyperUDNPuTotal );

        //  should be a % / i.e. lower than total
        AssertEDBG( (size_t)g_rgcbPageSize[icbPage] == Pdls()->CbPage() );
        AssertEDBG( cbfTotalCheck - rgcbfHyperPlainMissed[icbPage] <= cbfTotalCheck );
        AssertEDBG( cbfTotalCheck - rgcbfHyperReorgMissed[icbPage] <= cbfTotalCheck );
        AssertEDBG( cbfTotalCheck - rgcbfHyperNonPowerOf2[icbPage] <= cbfTotalCheck );
        AssertEDBG( cbfTotalCheck - rgcbfHyperDirty[icbPage] <= cbfTotalCheck );
        AssertEDBG( cbfTotalCheck - rgcbfHyperuPages[icbPage] <= cbfTotalCheck );
        AssertEDBG( cbfTotalCheck - rgcbfHyperUDNPu[icbPage] <= cbfTotalCheck );
    }

    EDBGPrintfDml( "\n" );

HandleError:

    for ( ULONG iQA = 0; iQA < cQA; iQA++ )
    {
        delete rgpiqCacheSum[iQA];
    }
    for ( ULONG ihisto = 0; ihisto < chisto; ihisto++ )
    {
        delete rgphisto[ ihisto ];
    }

    if ( err )
    {
        dprintf( "We failed to execute or get to the cache query, %d\n", err );
    }

}

INT CDigits( __int64 i )
{
    INT c = 0;
    while( i )
    {
        c++;
        i = i / 10;
    }
    return c;
}

const CHAR * SzSpace( const ULONG cch )
{
    const static CHAR * szSpaceBuff = "                   ";
    ULONG cchMax = strlen( szSpaceBuff );
    if ( cch >= cchMax )
    {
        dprintf( "Assert: asked for more space than static space buffer in SzSpace()" );
        return szSpaceBuff;
    }
    return &( szSpaceBuff[ cchMax - cch ] );
}

void EDBGPrintScavengeSequenceFriendly(
    __in const BOOL fFullHeader,
    __in const BFScavengeStats * const rgScavenge,
    __in const size_t cScavenge,
    __in const ULONG iScavengeCurrentRun )
{
    if ( fFullHeader )
    {
        dprintf( "Scavenge Stats:\n" );
        dprintf( "                      v-- delta ms from last run\n" );
        dprintf( "                      |\n" );
        dprintf( "                      |\n" );
    }

    // consider: sampling a couple rgScavenge[i].iRun values, and adjusting to the width of this field as it's the most dynamic

    BOOL fFirst = fTrue;
    BOOL fFirstPrint = fTrue;
    ULONG iFirst = IrrNext( iScavengeCurrentRun, cScavenge );

    INT cRunDigits = max( 4, CDigits( rgScavenge[iFirst].iRun ) + 1 ); // the +1 is in case we're near a boundary like 9999 runs, about to cross to 10000 runs.

    dprintf(     " [%hsiRun/  i]    +druns +xxxxxx ms CacheSize/Visited/Bst(  KB) EvAvPool/EvShrink/EvFromAvPool cbfTarget/cbfStartShr/ShrinkDur AvailPool/AvailLow/AvailHigh/AvailTarget  Flush/FlushSlow/FlushHung PermErrs/...OOM Latched/Dependent/TooRecent/..Tilt Flushed   Hung Stop    err\n",
                    SzSpace( cRunDigits - 4 /* for "iRun" */ ) );

    ULONG i;
    for ( i = iFirst; fFirst || i != iFirst; i = IrrNext( i, cScavenge ) )
    {
        fFirst = fFalse;

        if ( rgScavenge[i].iRun == 0 )
        {
            // blank entry
            continue;
        }

        //         [iRun/  i]    +druns +xxxxxx ms CacheSize/Visited/Bst(  KB) EvAvPool/EvShrink/EvFromAvPool cbfTarget/cbfStartShr/ShrinkDur AvailPool/AvailLow/AvailHigh/AvailTarget  Flush/FlushSlow/FlushHung PermErrs/...OOM Latched/Dependent/TooRecent/..Tilt Flushed   Hung Stop    err
        //         [0127/021]    +    0 +000000 ms      1000/    100/  1(  32)       10/      15/           3       900/       1200/     1234        20/      10/       20/         15     25/        2/        1        2/     0       1/        0/        0/     0       0      0    4    110
        dprintf( " [%*I64d/%03u]    + %4u +%06u ms  %8d/ %6d/%3d(%4I64d)   %6d/  %6d/      %6d  %8d/   %8d/ %8u  %8d/%8d/ %8d/   %8d %6d/   %6d/   %6d   %6d/%6d  %6d/   %6d/   %6d/%6d  %6d %6d    %1d %6d\n",
                    cRunDigits, rgScavenge[i].iRun,
                    i,
                    DWORD( fFirstPrint ? 0 : ( rgScavenge[i].iRun - rgScavenge[IrrPrev( i, cScavenge )].iRun ) ),
                    ( fFirstPrint ? 0 : ( rgScavenge[i].tickStart - rgScavenge[IrrPrev( i, cScavenge )].tickStart ) ),

                    rgScavenge[i].cbfCacheSize,
                    rgScavenge[i].cbfVisited,
                    ( fFirstPrint ? 0 : ( rgScavenge[i].cCacheBoosts - rgScavenge[IrrPrev( i, cScavenge )].cCacheBoosts ) ),
                    ( fFirstPrint ? 0 : ( ( rgScavenge[i].cbCacheBoosted - rgScavenge[IrrPrev( i, cScavenge )].cbCacheBoosted ) / 1024 ) ),

                    rgScavenge[i].cbfEvictedAvailPool,
                    rgScavenge[i].cbfEvictedShrink,
                    rgScavenge[i].cbfShrinkFromAvailPool,

                    rgScavenge[i].cbfCacheTarget,
                    rgScavenge[i].cbfCacheSizeStartShrink,
                    rgScavenge[i].dtickShrinkDuration,

                    rgScavenge[i].cbfAvail,
                    rgScavenge[i].cbfAvailPoolLow,
                    rgScavenge[i].cbfAvailPoolHigh,
                    rgScavenge[i].cbfAvailPoolTarget,

                    rgScavenge[i].cbfFlushPending,
                    rgScavenge[i].cbfFlushPendingSlow,
                    rgScavenge[i].cbfFlushPendingHung,

                    rgScavenge[i].cbfPermanentErrs,
                    rgScavenge[i].cbfOutOfMemory,

                    rgScavenge[i].cbfLatched,
                    rgScavenge[i].cbfDependent,
                    rgScavenge[i].cbfTouchTooRecent,
                    rgScavenge[i].cbfDiskTilt,
                    rgScavenge[i].cbfFlushed,
                    rgScavenge[i].cbfHungIOs,

                    (INT)rgScavenge[i].eStopReason,
                    rgScavenge[i].errRun
                    );
        fFirstPrint = fFalse;
    }
    if ( i == iFirst && fFirstPrint )
    {
        dprintf( " No Entries!\n" );
    }

}

const CHAR * SzBFIScavengeSatisfiedReason( BFIScavengeSatisfiedReason e )
{
    switch( e )
    {
        case eScavengeInvalid:
            return "eScavengeInvalid";
        case eScavengeCompleted:
            return "eScavengeCompleted";
        case eScavengeCompletedPendingWrites:
            return "eScavengeCompletedPendingWrites";
        case eScavengeBailedDiskTilt:
            return "eScavengeBailedDiskTilt";
        case eScavengeBailedExternalResize:
            return "eScavengeBailedExternalResize";
        case eScavengeVisitedAllLrukEntries:
            return "eScavengeVisitedAllLrukEntries";
#ifdef DEBUG
        case eScavengeBailedRandomlyDebug:
            return "eScavengeBailedRandomlyDebug";
#endif
    }

    return "<Unknown>";
}


//  ================================================================
DEBUG_EXT( EDBGCacheScavengeRuns )
//  ================================================================
{
    ERR err = JET_errSuccess;

    ULONG               iScavengeTimeSeqLast                = 0;
    size_t              cScavengeTimeSeq                    = 0;
    BFScavengeStats *   rgScavengeTimeSeqDebuggee           = NULL;
    BFScavengeStats *   rgScavengeTimeSeq                   = NULL;
    BFScavengeStats *   rgScavengeTimeSeqCumulativeDebuggee = NULL;
    BFScavengeStats *   rgScavengeTimeSeqCumulative         = NULL;
    ULONG               iScavengeLastRun                    = 0;
    BFScavengeStats *   rgScavengeLastRunsDebuggee          = NULL;
    BFScavengeStats *   rgScavengeLastRuns                  = NULL;

    if ( argc != 0 )
    {
        dprintf( "Invalid number of arguments: %d\n", argc );
        dprintf( "!ese cachescavengeruns\n" );
        return;
    }
    
    if ( !FReadGlobal( "g_iScavengeTimeSeqLast", &iScavengeTimeSeqLast ) ||
            !FReadGlobal( "g_cScavengeTimeSeq", &cScavengeTimeSeq ) ||
            !FReadGlobal( "g_rgScavengeTimeSeq", &rgScavengeTimeSeqDebuggee ) ||
            !FFetchVariable( rgScavengeTimeSeqDebuggee, &rgScavengeTimeSeq, cScavengeTimeSeq ) ||
#ifndef RTM
            !FReadGlobal( "g_rgScavengeTimeSeqCumulative ", &rgScavengeTimeSeqCumulativeDebuggee ) ||
            !FFetchVariable( rgScavengeTimeSeqCumulativeDebuggee, &rgScavengeTimeSeqCumulative, cScavengeTimeSeq ) ||
#endif
            !FReadGlobal( "g_iScavengeLastRun", &iScavengeLastRun ) ||
            !FReadGlobal( "g_rgScavengeLastRuns", &rgScavengeLastRunsDebuggee ) ||
            !FFetchVariable( rgScavengeLastRunsDebuggee, &rgScavengeLastRuns, g_cScavengeLastRuns )
            )
    {
        dprintf( "Failed to fetch all the global variables\n" );
        goto HandleError;
    }

    dprintf( "\n" );

    dprintf( "Scavenge stats mem usage stats:\n" );
    dprintf( "    sizeof(BFScavengeStats)                         = %I64u bytes\n", (ULONG64)sizeof(BFScavengeStats) );
    dprintf( "    sizeof(g_rgScavengeTimeSeq[%3I64u])                = %I64u bytes\n", (ULONG64)cScavengeTimeSeq, (ULONG64)sizeof(g_rgScavengeTimeSeq) );
    dprintf( "    sizeof(g_rgScavengeLastRuns[%3I64u])               = %I64u bytes\n", (ULONG64)g_cScavengeLastRuns, (ULONG64)sizeof(g_rgScavengeLastRuns) );
#ifndef RTM
    dprintf( "    sizeof(g_rgScavengeTimeSeqCumulative[%3I64u])      = %I64u bytes\n", (ULONG64)cScavengeTimeSeq, (ULONG64)sizeof(g_rgScavengeTimeSeqCumulative) );
#endif

    //  first and last ticks ...

    ULONG iFirst = IrrPrev( iScavengeTimeSeqLast, cScavengeTimeSeq );
    TICK tickFirst = 0xFEFEFEFE;
    if ( rgScavengeTimeSeq[iFirst].tickStart ||
            TickCmp( rgScavengeTimeSeq[1].tickStart, rgScavengeTimeSeq[iFirst].tickStart ) < 0 )
    {
        iFirst = 1;
        tickFirst = rgScavengeTimeSeq[1].tickStart;
    }
    else
    {
        //iFirst already set
        tickFirst = rgScavengeTimeSeq[iFirst].tickStart;
    }

    ULONG iLast;
    TICK tickLast;
    ULONG iLastLast = IrrNext( iScavengeLastRun, g_cScavengeLastRuns );
    if ( rgScavengeLastRuns[iLastLast].tickEnd &&
            TickCmp( rgScavengeLastRuns[iScavengeLastRun].tickEnd, rgScavengeLastRuns[iLastLast].tickEnd ) < 0 )
    {
        iLast = iLastLast;
        tickLast = rgScavengeLastRuns[iLastLast].tickEnd;
    }
    else
    {
        iLast = iScavengeLastRun;
        tickLast = rgScavengeLastRuns[iScavengeLastRun].tickEnd;
    }

    dprintf( "    g_rgScavengeTimeSeq[%03u].tickStart              = %u (0x%x)\n", iFirst, tickFirst, tickFirst );
    dprintf( "    g_rgScavengeLastRuns[%03u].tickEnd               = %u (0x%x)\n", iLast, tickLast, tickLast );
    dprintf( "    Delta seconds.ms(tick)                          = %03.3f (0x%x)\n",
                ( (double)tickLast - (double)tickFirst ) / 1000.0, tickLast - tickFirst );
    dprintf( "\n" );

    //  First we print out the longer time sequence series
    //

    dprintf( "Printing out Time Seq decision array:\n" );
    dprintf( "\n" );
    EDBGPrintScavengeSequenceFriendly( fTrue, rgScavengeTimeSeq, cScavengeTimeSeq, iScavengeTimeSeqLast );
    dprintf( "\n" );

#ifndef RTM
    //  Second we print out the cumulative sequence series
    //

    dprintf( "Printing out Time Seq cumulative information:\n" );
    dprintf( "\n" );
    EDBGPrintScavengeSequenceFriendly( fFalse, rgScavengeTimeSeqCumulative, cScavengeTimeSeq, iScavengeTimeSeqLast );
    dprintf( "\n" );
#endif

    //  Lastly we print out the last runs series
    //

    dprintf( "Printing out Last %I64u runs:\n", (ULONG64)g_cScavengeLastRuns );
    dprintf( "\n" );
    EDBGPrintScavengeSequenceFriendly( fFalse, rgScavengeLastRuns, g_cScavengeLastRuns, iScavengeLastRun );
    dprintf( "\n" );

HandleError:

    Unfetch( rgScavengeTimeSeq );
    Unfetch( rgScavengeTimeSeqCumulative );
    Unfetch( rgScavengeLastRuns );

    if ( err )
    {
        dprintf( "We failed to execute or get to the cache query, %d\n", err );
    }

}

//  ================================================================
LOCAL BOOL FFetchGlobalParamsArray(
    _Deref_out_ CJetParam** prgparam,
    _Out_ size_t* pcparam )
//  ================================================================
{
    BOOL        fSucceeded  = fTrue;
    CJetParam * rgparamDebuggee     = NULL;

    if (    !FReadGlobal( "g_cparam", pcparam ) ||
            !FReadGlobal( "g_rgparam", &rgparamDebuggee ) ||
            !FFetchVariable( rgparamDebuggee, prgparam, *pcparam ) )
    {
        dprintf( "FFetchGlobalParamsArray Error: Can't fetch the global rgparam array.\n" );
        fSucceeded = fFalse;
    }

    return fSucceeded;
}


//  ================================================================
LOCAL BOOL FEDBGFetchTableMetaData(
    const FCB * const pfcbTableDebuggee,
    FCB **            ppfcbTable )
//  ================================================================
{
    FCB *       pfcbTable               = pfcbNil;
    TDB *       ptdb                    = ptdbNil;
    FCB *       pfcbTemplate            = pfcbNil;
    TDB *       ptdbTemplate            = ptdbNil;
    FCB *       pfcbBuffer              = pfcbNil;
    ULONG       cFieldsInitial          = 0;
    ULONG       cTemplateFieldsInitial  = 0;
    ULONG       cbAlloc                 = sizeof(FCB) + sizeof(TDB);
    BOOL        fResult                 = fTrue;

    if ( !FFetchVariable( (FCB* const)pfcbTableDebuggee, &pfcbTable ) )
    {
        dprintf( "Error: Could not fetch FCB at 0x%N.\n", pfcbTableDebuggee );
        fResult = fFalse;
        goto HandleError;
    }

    if ( !pfcbTable->FTypeTable() || pfcbTable->Ptdb() == NULL )
    {
        //  not actually a table FCB, so just return the FCB
        //
        *ppfcbTable = pfcbTable;
        return fTrue;
    }

    if ( !FFetchVariable( pfcbTable->Ptdb(), &ptdb ) )
    {
        dprintf( "Error: Could not fetch TDB at 0x%N.\n", pfcbTable->Ptdb() );
        fResult = fFalse;
        goto HandleError;
    }

    //  adjust buffer by size of MEMPOOL buffer
    //
    cbAlloc += ptdb->MemPool().CbBufSize();

    //  adjust buffer for initial fields
    //

    cFieldsInitial = ( ptdb->FidFixedLastInitial() + 1 - ptdb->FidFixedFirst() );
    cFieldsInitial += ( ptdb->FidVarLastInitial() + 1 - ptdb->FidVarFirst() );
    cFieldsInitial += ( ptdb->FidTaggedLastInitial() + 1 - ptdb->FidTaggedFirst() );
    cbAlloc += ( cFieldsInitial * sizeof(FIELD) );

    if ( pfcbNil != ptdb->PfcbTemplateTable() )
    {
        if ( !FFetchVariable( ptdb->PfcbTemplateTable(), &pfcbTemplate ) )
        {
            dprintf( "Error: Could not fetch FCB at 0x%N.\n", ptdb->PfcbTemplateTable() );
            fResult = fFalse;
            goto HandleError;
        }

        if ( !FFetchVariable( pfcbTemplate->Ptdb(), &ptdbTemplate ) )
        {
            dprintf( "Error: Could not fetch TDB at 0x%N.\n", pfcbTemplate->Ptdb() );
            fResult = fFalse;
            goto HandleError;
        }

        //  adjust buffer by size of template table FCB, TDB, and MEMPOOL buffer
        //
        cbAlloc += sizeof(FCB) + sizeof(TDB) + ptdbTemplate->MemPool().CbBufSize();

        //  adjust buffer for initial fields
        //
        cTemplateFieldsInitial = ( ptdbTemplate->FidFixedLastInitial() + 1 - ptdbTemplate->FidFixedFirst() );
        cTemplateFieldsInitial += ( ptdbTemplate->FidVarLastInitial() + 1 - ptdbTemplate->FidVarFirst() );
        cTemplateFieldsInitial += ( ptdbTemplate->FidTaggedLastInitial() + 1 - ptdbTemplate->FidTaggedFirst() );
        cbAlloc += ( cTemplateFieldsInitial * sizeof(FIELD) );
    }

    //  now allocate one big buffer for the table (and template table) FCB, TDB, and MEMPOOL buffer
    //
    pfcbBuffer = (FCB *)LocalAlloc( 0, cbAlloc );
    if ( NULL == pfcbBuffer )
    {
        dprintf( "Error: Could not allocate %d-byte buffer for meta-data.\n", cbAlloc );
        fResult = fFalse;
        goto HandleError;
    }

    //  copy everything to final buffer
    //
    memcpy( pfcbBuffer, pfcbTable, sizeof(FCB) );

    //  TDB allocated after the FCB
    //
    pfcbBuffer->SetPtdb( (TDB *)( pfcbBuffer + 1 ) );
    memcpy( pfcbBuffer->Ptdb(), ptdb, sizeof(TDB) );

    //  MEMPOOL buffer allocated after the TDB
    //
    pfcbBuffer->Ptdb()->MemPool().SetPbuf( (BYTE *)( pfcbBuffer->Ptdb() + 1 ) );
    if ( !FReadVariable( ptdb->MemPool().Pbuf(), pfcbBuffer->Ptdb()->MemPool().Pbuf(), ptdb->MemPool().CbBufSize() ) )
    {
        dprintf(
            "Error: Could not read %d-byte MEMPOOL buffer at 0x%N.\n",
            ptdb->MemPool().CbBufSize(),
            ptdb->MemPool().Pbuf() );
        fResult = fFalse;
        goto HandleError;
    }

    //  buffer for initial fields allocated after MEMPOOL
    //
    pfcbBuffer->Ptdb()->SetPfieldInitial( (FIELD *)( pfcbBuffer->Ptdb()->MemPool().Pbuf() + pfcbBuffer->Ptdb()->MemPool().CbBufSize() ) );
    if ( !FReadVariable( ptdb->PfieldsInitial(), pfcbBuffer->Ptdb()->PfieldsInitial(), cFieldsInitial ) )
    {
        dprintf(
            "Error: Could not read %d-byte FIELD buffer at 0x%N.\n",
            cFieldsInitial * sizeof(FIELD),
            ptdb->PfieldsInitial() );
        fResult = fFalse;
        goto HandleError;
    }

    if ( pfcbNil != pfcbTemplate )
    {
        //  template table follows derived table
        //
        FCB *   pfcbT   = (FCB *)( pfcbBuffer->Ptdb()->PfieldsInitial() + cFieldsInitial );

        pfcbBuffer->Ptdb()->SetPfcbTemplateTable( pfcbT );
        memcpy( pfcbT, pfcbTemplate, sizeof(FCB) );

        pfcbT->SetPtdb( (TDB *)( pfcbT + 1 ) );
        memcpy( pfcbT->Ptdb(), ptdbTemplate, sizeof(TDB) );

        pfcbT->Ptdb()->MemPool().SetPbuf( (BYTE *)( pfcbT->Ptdb() + 1 ) );
        if ( !FReadVariable( ptdbTemplate->MemPool().Pbuf(), pfcbT->Ptdb()->MemPool().Pbuf(), ptdbTemplate->MemPool().CbBufSize() ) )
        {
            dprintf(
                "Error: Could not read %d-byte MEMPOOL buffer at 0x%N.\n",
                ptdbTemplate->MemPool().CbBufSize(),
                ptdbTemplate->MemPool().Pbuf() );
            fResult = fFalse;
            goto HandleError;
        }

        pfcbT->Ptdb()->SetPfieldInitial( (FIELD *)( pfcbT->Ptdb()->MemPool().Pbuf() + pfcbT->Ptdb()->MemPool().CbBufSize() ) );
        if ( !FReadVariable( ptdbTemplate->PfieldsInitial(), pfcbT->Ptdb()->PfieldsInitial(), cTemplateFieldsInitial ) )
        {
            dprintf(
                "Error: Could not read %d-byte FIELD buffer at 0x%N.\n",
                cTemplateFieldsInitial * sizeof(FIELD),
                ptdbTemplate->PfieldsInitial() );
            fResult = fFalse;
            goto HandleError;
        }
    }

    //  return the buffer
    //
    *ppfcbTable = pfcbBuffer;

    //  since we're returning the buffer, ensure it doesn't get freed
    //
    pfcbBuffer = pfcbNil;


HandleError:
    LocalFree( pfcbBuffer );
    Unfetch( ptdbTemplate );
    Unfetch( pfcbTemplate );
    Unfetch( ptdb );
    Unfetch( pfcbTable );

    return fResult;
}


//  UNDONE: should use the existing OBJIDLIST,
//  but I wasn't sure if the memory allocation
//  routines used by that class will work when
//  invoked from debugger extensions
//
struct OBJIDLIST_EDBG
{
    OBJID * rgobjid;
    ULONG   cobjid;
};


//  ================================================================
LOCAL BOOL FEDBGFetchTableObjids(
    FCB * const             pfcbTable,
    OBJIDLIST_EDBG * const  pobjidlist )
//  ================================================================
{
    FCB *                   pfcb    = pfcbNil;
    ULONG                   cobjid  = 1;        //  1 for the table
    BOOL                    fError  = fFalse;

    if ( NULL != pobjidlist->rgobjid || 0 != pobjidlist->cobjid )
    {
        dprintf( "Error: OBJIDLIST corruption detected (entry already in use).\n" );
        fError = fTrue;
        goto HandleError;
    }

    //  allocate a buffer for the FCB's we'll be examining
    //
    pfcb = (FCB *)LocalAlloc( 0, sizeof(FCB) );
    if ( NULL == pfcb )
    {
        dprintf( "Error: Could not allocate FCB buffer.\n" );
        fError = fTrue;
        goto HandleError;
    }

    //  determine if this table has a long-value tree
    //
    FCB* pfcbLV = pfcbTable->Ptdb()->PfcbLV();
    if ( pfcbNil != pfcbLV )
    {
        cobjid++;
    }

    //  determine the number of secondary indexes owned by this table
    //
    for ( FCB * pfcbT = pfcbTable->PfcbNextIndex();
        pfcbNil != pfcbT;
        pfcbT = pfcb->PfcbNextIndex() )
    {
        if ( !FReadVariable( pfcbT, pfcb ) )
        {
            dprintf( "Error: Could not read FCB at 0x%N.\n", pfcbT );
            fError = fTrue;
            goto HandleError;
        }

        cobjid++;
    }

    //  allocate the list of objid's (caller will free it)
    //
    pobjidlist->rgobjid = (OBJID *)LocalAlloc( 0, sizeof(OBJID) * cobjid );
    if ( NULL == pobjidlist->rgobjid )
    {
        dprintf( "Error: Could not allocate buffer for OBJID list.\n" );
        fError = fTrue;
        goto HandleError;
    }

    pobjidlist->cobjid = cobjid;

    //  account for table objid
    //
    cobjid = 0;
    pobjidlist->rgobjid[cobjid++] = pfcbTable->ObjidFDP();

    //  account for LV objid
    //
    if ( pfcbNil != pfcbLV )
    {
        if ( !FReadVariable( pfcbTable->Ptdb()->PfcbLV(), pfcb ) )
        {
            dprintf( "Error: Could not read FCB at 0x%N.\n", pfcbTable->Ptdb()->PfcbLV() );
            fError = fTrue;
            goto HandleError;
        }

        pobjidlist->rgobjid[cobjid++] = pfcb->ObjidFDP();
    }

    //  account for secondary index objids
    //
    for ( FCB * pfcbT = pfcbTable->PfcbNextIndex();
        pfcbNil != pfcbT;
        pfcbT = pfcb->PfcbNextIndex() )
    {
        if ( !FReadVariable( pfcbT, pfcb ) )
        {
            dprintf( "Error: Could not read FCB at 0x%N.\n", pfcbT );
            fError = fTrue;
            goto HandleError;
        }

        AssertPREFIX( cobjid < pobjidlist->cobjid );

        pobjidlist->rgobjid[cobjid++] = pfcb->ObjidFDP();
    }

    Assert( cobjid == pobjidlist->cobjid );

HandleError:
    //  free allocated resources
    //
    LocalFree( pfcb );

    return !fError;
}

//  ================================================================
LOCAL BOOL FEDBGTableFind(
    const CHAR *            szTableName,
    INST * const            pinstTarget,
    OBJIDLIST_EDBG * const  rgobjidlistFilter,
    const IFMP              ifmpFilter )
//  ================================================================
{
    INST *                  pinst               = pinstNil;
    BOOL                    fFoundFCB           = fFalse;
    BOOL                    fError              = fFalse;
    const ULONG             cchTableName        = strlen( szTableName );
    const BOOL              fTryPrefixMatch     = ( ( cchTableName > 0 ) && ( '*' == szTableName[ cchTableName - 1 ] ) );

    //  HACK! HACK!
    //
    //  The shadow catalog is always named MSysObjects in the TDB
    //  (see ErrCATIInitCatalogTDB) so when searching for one
    //  of the catalog tables, can't use a name match and must
    //  instead use an objid match
    //
    const BOOL              fFindCatalog        = FCATSystemTable( szTableName );
    const OBJID             objidFindMSO        = ( fFindCatalog ? ObjidCATTable( szTableName ) : objidNil );

    fError = ( Pdls()->ErrINSTDLSInitCheck() < JET_errSuccess );
    if ( fError )
    {
        goto HandleError;
    }

    //  scan FCB list of all INST's
    //
    for ( SIZE_T ipinst = 0; ipinst < Pdls()->Cinst(); ipinst++ )
    {
        if ( pinstNil == Pdls()->PinstDebuggee( ipinst ) )
        {
            continue;
        }

        if ( pinstTarget && pinstTarget != Pdls()->PinstDebuggee( ipinst ) )
        {
            continue;
        }

        dprintf( "\nScanning all FCB's of instance 0x%N...\n", Pdls()->PinstDebuggee( ipinst ) );
        if ( ( pinst = Pdls()->Pinst( ipinst ) ) )
        {
            FCB *   pfcbDebuggee    = pinst->m_pfcbList;

            while ( pfcbNil != pfcbDebuggee )
            {
                FCB *   pfcb;

                //  retrieve meta-data
                //
                if ( !FEDBGFetchTableMetaData( pfcbDebuggee, &pfcb ) )
                {
                    fError = fTrue;
                    goto HandleError;
                }

                //  see if this FCB is a match
                //
                if ( pfcb->FTypeTable() )
                {
                    BOOL    fMatchingTable = fFalse;

                    if ( fFindCatalog )
                    {
                        fMatchingTable = ( objidFindMSO == pfcb->ObjidFDP() );
                    }
                    else if ( pfcb->Ptdb() != NULL )
                    {
                        if ( fTryPrefixMatch )
                        {
                            fMatchingTable = ( 0 == _strnicmp( szTableName, pfcb->Ptdb()->SzTableName(), cchTableName - 1 ) );  //  -1 because we don't want to compare the wildcard character
                        }
                        else
                        {
                            fMatchingTable = ( 0 == UtilCmpName( szTableName, pfcb->Ptdb()->SzTableName() ) );
                        }
                    }

                    if ( fMatchingTable )
                    {
                        dprintf(
                            "    %s%s0x%N [ifmp:0x%x, objidFDP:0x%x, pgnoFDP:0x%x]\n",
                            ( fTryPrefixMatch ? pfcb->Ptdb()->SzTableName() : "" ),
                            ( fTryPrefixMatch ? ": " : "" ),
                            pfcbDebuggee,
                            pfcb->Ifmp(),
                            pfcb->ObjidFDP(),
                            pfcb->PgnoFDP() );
                        fFoundFCB = fTrue;

                        if ( NULL != rgobjidlistFilter
                            && ( ifmpNil == ifmpFilter || pfcb->Ifmp() == ifmpFilter ) )
                        {
                            if ( !FEDBGFetchTableObjids( pfcb, rgobjidlistFilter + pfcb->Ifmp() ) )
                            {
                                fError = fTrue;
                                goto HandleError;
                            }
                        }
                    }
                }

                pfcbDebuggee = pfcb->PfcbNextList();
                Unfetch( pfcb );
            }
        }
        else
        {
            if ( NULL != pinstTarget )
            {
                dprintf( "    Error: Could not read INST at 0x%N. Aborting.\n", pinstTarget );
            }
            else
            {
                dprintf(
                    "    g_rgpinst[0x%x]  Error: Could not read INST at 0x%N. Aborting.\n",
                    ipinst,
                    Pdls()->PinstDebuggee( ipinst ) );
            }

            fError = fTrue;
            goto HandleError;
        }
    }

    if ( !fFoundFCB )
    {
        //  we did not find the FCB
        //
        dprintf( "\nCould not find any FCB's with table name \"%s\".\n", szTableName );
        fError = fTrue;
        goto HandleError;
    }

HandleError:

    return !fError;
}

//  ================================================================
LOCAL BOOL FEDBGIsObjidInList(
    const OBJID                     objidTarget,
    const OBJIDLIST_EDBG * const    pobjidlistFilter )
//  ================================================================
{
    for ( ULONG i = 0; i < pobjidlistFilter->cobjid; i++ )
    {
        if ( objidTarget == pobjidlistFilter->rgobjid[ i ] )
            return fTrue;
    }

    return fFalse;
}


enum TREETYPE
{
    treetypeData,
    treetypeIndex,
    treetypeLongValue,
    treetypeSpace,
    treetypeMax
};

struct PGCOUNTS
{
    DWORD_PTR   cpages;
    DWORD_PTR   cnodes;
    QWORD       cbBufferFree;
    QWORD       cbPageFree;
    DWORD_PTR   cpagesPreread;
    DWORD_PTR   cpagesEmpty;
    DWORD_PTR   cpagesPreInit;
    DWORD_PTR   cpagesVersioned;        //  fOlderVersion == TRUE
    DWORD_PTR   cpagesUnversioned;      //  neither fCurrentVersion nor fOlderVersion
    DWORD_PTR   cpagesDirty;
    DWORD_PTR   cpagesCatalog;
    DWORD_PTR   cpagesZeroNodes;
    DWORD_PTR   cpagesSingleLiveNode;
    DWORD_PTR   cpagesSingleDeletedNode;
};
struct TREELEVELINFO
{
    PGCOUNTS    pgcountsInternalPages;
    PGCOUNTS    pgcountsParentOfLeafPages;
    PGCOUNTS    pgcountsLeafPages;
};
struct TREEINFO
{
    CHAR            szTreeType[16];
    TREELEVELINFO   treelevelinfoRootPages;
    TREELEVELINFO   treelevelinfoNonRootPages;
};

LOCAL VOID EDBGPrintPgCounts( const PGCOUNTS * const ppgcounts )
{
    dprintf(
        "%10d  %10d  %10d  %10d  %11d  %12d  %10d  %10d  %16d  %19d  %14.2f  %23I64d  %23I64d\n",
        ppgcounts->cpages,
        ppgcounts->cpagesPreread,
        ppgcounts->cpagesEmpty,
        ppgcounts->cpagesVersioned,
        ppgcounts->cpagesUnversioned,
        ppgcounts->cpagesDirty,
        ppgcounts->cpagesCatalog,
        ppgcounts->cpagesZeroNodes,
        ppgcounts->cpagesSingleLiveNode,
        ppgcounts->cpagesSingleDeletedNode,
        ( ppgcounts->cpages > 0 ? (double)ppgcounts->cnodes / (double)ppgcounts->cpages : 0.0 ),
        ppgcounts->cbBufferFree,
        ppgcounts->cbPageFree );
}

LOCAL VOID EDBGPrintTreeLevelInfo( const TREELEVELINFO * const ptreelevelinfo )
{
    dprintf( "        Internal:      " );
    EDBGPrintPgCounts( &ptreelevelinfo->pgcountsInternalPages );
    dprintf( "        ParentOfLeaf:  " );
    EDBGPrintPgCounts( &ptreelevelinfo->pgcountsParentOfLeafPages );
    dprintf( "        Leaf:          " );
    EDBGPrintPgCounts( &ptreelevelinfo->pgcountsLeafPages );
}

LOCAL VOID EDBGPrintTreeInfo( const TREEINFO * const ptreeinfo )
{
    dprintf( "%s Pages:\n", ptreeinfo->szTreeType );
    dprintf( "    Root:\n" );
    EDBGPrintTreeLevelInfo( &ptreeinfo->treelevelinfoRootPages );
    dprintf( "    Non-Root:\n" );
    EDBGPrintTreeLevelInfo( &ptreeinfo->treelevelinfoNonRootPages );
    dprintf( "\n" );
}

LOCAL VOID EDBGUpdatePgCounts(
    TREEINFO * const    rgtreeinfo,
    const BF * const    pbf,
    const CPAGE * const pcpage )
{
    TREETYPE            treetype;
    TREELEVELINFO *     ptreelevelinfo;
    PGCOUNTS *          ppgcounts;

    //  determine type of page
    //
    if ( pcpage->FSpaceTree() )
    {
        treetype = treetypeSpace;
    }
    else if ( pcpage->FIndexPage() )
    {
        treetype = treetypeIndex;
    }
    else if ( pcpage->FLongValuePage() )
    {
        treetype = treetypeLongValue;
    }
    else
    {
        //  must assume data page
        //
        treetype = treetypeData;
    }

    //  determine btree level of page
    //
    ptreelevelinfo = &( pcpage->FRootPage() ?
                            rgtreeinfo[treetype].treelevelinfoRootPages :
                            rgtreeinfo[treetype].treelevelinfoNonRootPages );
    if ( pcpage->FParentOfLeaf() )
    {
        ppgcounts = &ptreelevelinfo->pgcountsParentOfLeafPages;
    }
    else if ( pcpage->FLeafPage() )
    {
        ppgcounts = &ptreelevelinfo->pgcountsLeafPages;
    }
    else
    {
        //  must assume internal page
        //
        ppgcounts = &ptreelevelinfo->pgcountsInternalPages;
    }

    //  increment page counts
    //
    ppgcounts->cpages++;
    ppgcounts->cnodes += pcpage->Clines();

    if ( errBFIPageNotVerified == pbf->err )
        ppgcounts->cpagesPreread++;
    if ( pbf->fOlderVersion )
        ppgcounts->cpagesVersioned++;
    else if ( !pbf->fCurrentVersion )
        ppgcounts->cpagesUnversioned++;
    if ( pbf->bfdf >= bfdfDirty )
        ppgcounts->cpagesDirty++;
    if ( pcpage->ObjidFDP() <= objidFDPMSO_RootObjectIndex && pcpage->ObjidFDP() >= objidFDPMSO )
        ppgcounts->cpagesCatalog++;

    if ( pcpage->FEmptyPage() )
        ppgcounts->cpagesEmpty++;
    if ( pcpage->FPreInitPage() )
        ppgcounts->cpagesPreInit++;
    else if ( 0 == pcpage->Clines() )
        ppgcounts->cpagesZeroNodes++;
    else if ( 1 == pcpage->Clines() )
    {
        LINE line;
        pcpage->GetPtr( 0, &line );
        if ( line.fFlags & fNDDeleted )
            ppgcounts->cpagesSingleDeletedNode++;
        else
            ppgcounts->cpagesSingleLiveNode++;
    }

    ppgcounts->cbBufferFree += ((CPAGE::PGHDR*)pcpage->m_bfl.pv)->cbFree;
    ppgcounts->cbPageFree += pcpage->CbPageFree();
}

DEBUG_EXT( EDBGDumpCacheInfo )
//  ================================================================
{
    PBF                 pbf                 = pbfNil;
    DWORD_PTR           ibf;
    CPAGE               cpage;
    BYTE *              pbPageT             = NULL;
    ULONG               cbPageT             = 0;
    IFMP                ifmpMaxT            = 0;
    BF **               rgpbfChunkT         = NULL;
    LONG_PTR            cbfChunkT;
    LONG_PTR            cbfCacheAddressableT;
    DWORD_PTR           cAvailBuffers       = 0;
    DWORD_PTR           cTempDbBuffers      = 0;
    DWORD_PTR           cUninitBuffers      = 0;
    DWORD_PTR           cUninitPages        = 0;
    DWORD_PTR           cFilteredBuffers    = 0;
    DWORD_PTR           cbfNotCommitted     = 0;
    DWORD_PTR           cbfNewlyCommitted   = 0;
    DWORD_PTR           cbfNotResident      = 0;
    DWORD_PTR           cbfResident         = 0;
    TREEINFO            rgtreeinfo[treetypeMax];
    const CHAR *        szTableName         = NULL;
    ULONG               ifmpFilter          = (ULONG)ifmpNil;
    OBJIDLIST_EDBG *    rgobjidlistFilter   = NULL;
    BOOL                fValidUsage         = fTrue;

    memset( rgtreeinfo, 0, sizeof(rgtreeinfo) );

    OSStrCbCopyA( rgtreeinfo[treetypeData].szTreeType, sizeof(rgtreeinfo[treetypeData].szTreeType), "Data" );
    OSStrCbCopyA( rgtreeinfo[treetypeIndex].szTreeType, sizeof(rgtreeinfo[treetypeIndex].szTreeType), "Index" );
    OSStrCbCopyA( rgtreeinfo[treetypeLongValue].szTreeType, sizeof(rgtreeinfo[treetypeLongValue].szTreeType), "LongValue" );
    OSStrCbCopyA( rgtreeinfo[treetypeSpace].szTreeType, sizeof(rgtreeinfo[treetypeSpace].szTreeType), "Space" );

    switch ( argc )
    {
        case 2:
            if ( !FAutoIfmpFromSz( argv[ 1 ], &ifmpFilter ) )
            {
                fValidUsage = fFalse;
                break;
            }
            //  FALL THROUGH
        case 1:
            szTableName = argv[ 0 ];
            break;
        case 0:
            break;
        default:
            fValidUsage = fFalse;
    }

    if ( !fValidUsage )
    {
        dprintf( "\nUsage: DUMPCACHEINFO [<szTable> [<ifmp>]]\n\n" );
        dprintf( "    <szTable> - name of table for which to dump cache info\n" );
        dprintf( "                (specify '*' to dump info for all cached pages)\n" );
        dprintf( "    <ifmp|.>  - an optional parameter specifying the database for\n" );
        dprintf( "                which to dump cache info (if omitted, all databases\n" );
        dprintf( "                are included)\n" );
        goto HandleError;
    }

    if ( !FReadGlobal( "g_cbfChunk", &cbfChunkT )
        || !FReadGlobal( "cbfCacheAddressable", &cbfCacheAddressableT )
        || !FReadGlobalAndFetchVariable( "g_rgpbfChunk", &rgpbfChunkT, cCacheChunkMax ) )
    {
        dprintf( "\nError: Could not read global BF variables.\n" );
        goto HandleError;
    }

    cbPageT = Pdls()->CbPage();
    dprintf( "g_cbPage = %d\n", cbPageT );

    if ( !FReadGlobal( "g_ifmpMax", &ifmpMaxT ) )
    {
        dprintf( "\nError: Could not read global FMP variable.\n" );
        goto HandleError;
    }
    else if ( ifmpNil != ifmpFilter && ifmpFilter > ifmpMaxT )
    {
        dprintf( "\nError: Illegal value specified for the IFMP parameter.\n" );
        goto HandleError;
    }

    //  if not dumping all cached pages, allocate array to hold filter
    //
    if ( NULL != szTableName && 0 != strcmp( "*", szTableName ) )
    {
        rgobjidlistFilter = (OBJIDLIST_EDBG *)LocalAlloc( 0, sizeof(OBJIDLIST_EDBG) * ifmpMaxT );
        if ( NULL == rgobjidlistFilter )
        {
            dprintf( "\nError: Could not allocate objid buffer.\n" );
            goto HandleError;
        }

        memset( rgobjidlistFilter, 0, sizeof(OBJIDLIST_EDBG) * ifmpMaxT );

        //  UNDONE: support filtering for just one instance
        //
        if ( !FEDBGTableFind( szTableName, NULL, rgobjidlistFilter, ifmpFilter ) )
        {
            goto HandleError;
        }
    }

    pbf = (PBF)LocalAlloc( 0, sizeof(BF) );
    if ( NULL == pbf )
    {
        dprintf( "\nError: Could not allocate BF buffer.\n" );
        goto HandleError;
    }

    pbPageT = (BYTE *)LocalAlloc( 0, cbPageT );
    if ( NULL == pbPageT )
    {
        dprintf( "\nError: Could not allocate page buffer.\n" );
        goto HandleError;
    }

    cpage.LoadPage( pbPageT, cbPageT );

    //  scan all valid BFs
    //
    dprintf( "\nScanning %d (0x%x) BF's...", cbfCacheAddressableT, cbfCacheAddressableT );
    for ( ibf = 0; (LONG_PTR)ibf < cbfCacheAddressableT; ibf++ )
    {
        if ( ibf % 10000 == 0 && ibf > 0 )
        {
            //  I'm seeing that it can take a REALLY long time to
            //  scan through all BF's and read the PGHDR from each
            //  page, so report progress every once in a while
            //
            dprintf( "\n\t%d BF's scanned...", ibf );
        }

        if ( FEDBGCheckForCtrlC() )
        {
            dprintf( "Aborting.\n" );
            break;
        }

        //  compute the address of the target BF

        PBF pbfDebuggee = rgpbfChunkT[ ibf / cbfChunkT ] + ibf % cbfChunkT;

        //  we failed to read this BF

        if ( !FReadVariable( pbfDebuggee, pbf ) )
        {
            dprintf( "Error: Could not read BF at 0x%N. Aborting.\n", pbfDebuggee );
            break;
        }

        const size_t    cbBufferT   = ( pbf->icbBuffer < icbPageMax ? g_rgcbPageSize[pbf->icbBuffer] : cbPageT );

        if ( pbf->fAvailable )
        {
            cAvailBuffers++;
        }
        else if ( NULL == pbf->pv
            || errBFIPageFaultPending == pbf->err   //  page is being read in
            || pbf->fQuiesced )                     //  cache is shrinking and this buffer will be eliminated
        {
            cUninitBuffers++;
        }
        else if ( Pdls()->FIsTempDB( pbf->ifmp ) )
        {
            cTempDbBuffers++;
        }
        else if ( !FReadVariable( (BYTE *)pbf->pv, pbPageT, cbPageT ) )
        {
            dprintf( "Error: Could not read page at 0x%N of BF at 0x%N. Aborting.\n", pbf->pv, pbfDebuggee );
            break;
        }
        else if ( !cpage.FPageIsInitialized() )
        {
            cUninitPages++;
        }
        else if ( ( NULL == rgobjidlistFilter && ifmpNil == ifmpFilter )        //  no filter
            || ( NULL == rgobjidlistFilter && pbf->ifmp == ifmpFilter )         //  ifmp filter
            || ( NULL != rgobjidlistFilter && FEDBGIsObjidInList( cpage.ObjidFDP(), rgobjidlistFilter + pbf->ifmp ) ) ) //  table filter
        {
            //  we'll be reading more than just the page header, so must
            //  load the dehydrated page
            //
            cpage.LoadDehydratedPage( pbf->ifmp, pbf->pgno, pbPageT, cbBufferT, cbPageT );
            EDBGUpdatePgCounts( rgtreeinfo, pbf, &cpage );
            cFilteredBuffers++;
        }

        switch ( pbf->bfrs )
        {
            case bfrsNotCommitted:
                cbfNotCommitted++;
                break;
            case bfrsNewlyCommitted:
                cbfNewlyCommitted++;
                break;
            case bfrsNotResident:
                cbfNotResident++;
            case bfrsResident:
                cbfResident++;
                break;
        }
    }

    //  report results
    //
    dprintf( "\n\n" );

    dprintf( "Total BF's scanned:   %d\n", ibf );
    dprintf( "Available BF's:       %d\n", cAvailBuffers );
    dprintf( "Temp. database BF's:  %d\n", cTempDbBuffers );
    dprintf( "Not committed BF's    %d\n", cbfNotCommitted );
    dprintf( "Newly committed BF's  %d\n", cbfNewlyCommitted );
    dprintf( "Not resident BF's     %d\n", cbfNotResident );
    dprintf( "Resident BF's         %d\n", cbfResident );
    dprintf( "Uninitialised BF's:   %d\n", cUninitBuffers );
    dprintf( "Uninitialised pages:  %d\n\n\n", cUninitPages );

    if ( NULL != rgobjidlistFilter )
    {
        if ( ifmpNil == ifmpFilter )
            dprintf( "BF's for table '%s' (all FMP's): %d\n\n", szTableName, cFilteredBuffers );
        else
            dprintf( "BF's for table '%s' (IFMP 0x%x): %d\n\n", szTableName, ifmpFilter, cFilteredBuffers );
    }
    else if ( ifmpNil != ifmpFilter )
        dprintf( "BF's for IFMP 0x%x: %d\n\n", ifmpFilter, cFilteredBuffers );

    dprintf( "                         TOTAL      Preread      Empty     Versioned   Unversioned  Dirty/Filthy   Catalog    Zero nodes  Single live node  Single deleted node  Avg nodes/page  Free buff space (bytes)  Free page space (bytes)\n" );
    dprintf( "                       ----------  ----------  ----------  ----------  -----------  ------------  ----------  ----------  ----------------  -------------------  --------------  -----------------------  -----------------------\n" );

    for ( DWORD_PTR itreetype = 0; itreetype < treetypeMax; itreetype++ )
    {
        EDBGPrintTreeInfo( rgtreeinfo + itreetype );
    }

HandleError:
    dprintf( "\n--------------------\n\n" );

    //  unload BF parameters

    if ( NULL != rgobjidlistFilter )
    {
        for ( ULONG i = 0; i < ifmpMaxT; i++ )
        {
            LocalFree( rgobjidlistFilter[i].rgobjid );
        }
        LocalFree( rgobjidlistFilter );
    }
    LocalFree( pbPageT );
    LocalFree( pbf );
    Unfetch( rgpbfChunkT );
}


//  This creates a chain of EDBGIOREQCHUNKs that has the whole set of all allocated
//  IOREQCHUNKSs (and thus IOREQs) in it.

typedef struct _EDBGIOREQCHUNK
{
    IOREQCHUNK *                pioreqchunkDebuggee;    // do not deref
    IOREQCHUNK *                pioreqchunkDebugger;
    struct _EDBGIOREQCHUNK *    pNext;
} EDBGIOREQCHUNK;

BOOL FFetchIOREQCHUNKSLIST( EDBGIOREQCHUNK ** pedbgioreqchunkHead )
{
    DWORD   cbIoreqChunkT;

    IOREQCHUNK * pioreqchunkDebuggee = NULL;
    IOREQCHUNK * pioreqchunkT = NULL;

    EDBGIOREQCHUNK * pedbgioreqchunkT = NULL;

    if ( !FReadGlobal( "g_cbIoreqChunk", &cbIoreqChunkT )
        || !FReadGlobal( "g_pioreqchunkRoot", &pioreqchunkDebuggee ) )
    {
        return fFalse;
    }

    while ( pioreqchunkDebuggee )
    {
        if ( !FFetchVariable( (BYTE*)pioreqchunkDebuggee, (BYTE**)&pioreqchunkT, cbIoreqChunkT ) )
        {
            return fFalse;
        }

        if ( pedbgioreqchunkT )
        {
            pedbgioreqchunkT->pNext = new EDBGIOREQCHUNK;
            pedbgioreqchunkT = pedbgioreqchunkT->pNext;
        }
        else
        {
            pedbgioreqchunkT = new EDBGIOREQCHUNK;
        }
        //  Whoops allocation failure ...
        if ( NULL == pedbgioreqchunkT )
        {
            return fFalse;  // drip, drip, drip ...
        }
        pedbgioreqchunkT->pNext = NULL;

        if ( NULL == *pedbgioreqchunkHead )
        {
            //  set return value to the very first chunk / g_pioreqchunkRoot.
            *pedbgioreqchunkHead = pedbgioreqchunkT;
        }

        pedbgioreqchunkT->pioreqchunkDebuggee = pioreqchunkDebuggee;
        pedbgioreqchunkT->pioreqchunkDebugger = pioreqchunkT;
        pedbgioreqchunkT->pNext = NULL;

        pioreqchunkDebuggee = pioreqchunkT->pioreqchunkNext;
    }

    return fTrue;
}


void UnfetchIOREQCHUNKSLIST( EDBGIOREQCHUNK * pedbgioreqchunkRoot )
{
    EDBGIOREQCHUNK * pedbgioreqchunkT = pedbgioreqchunkRoot;
    while ( pedbgioreqchunkT )
    {
        EDBGIOREQCHUNK * pedbgioreqchunkNext = pedbgioreqchunkT->pNext;
        Unfetch( pedbgioreqchunkT->pioreqchunkDebugger );
        delete pedbgioreqchunkT;
        pedbgioreqchunkT = pedbgioreqchunkNext;
    }
}


#if LOCAL_IS_WORKING
//  ================================================================
LOCAL_BROKEN DEBUG_EXT( EDBGDumpPendingIO )
//  ================================================================
{
    COSFile *           pcosf               = NULL;
    P_OSFILE            posf                = NULL;

    EDBGIOREQCHUNK *    pedbgioreqchunkRoot = NULL;
    EDBGIOREQCHUNK *    pedbgioreqchunkT    = NULL;

    HRT                 hrtLastGiven        = 0;
    HRT                 hrtHRTFreq          = 0;
    BOOL                fHasHRTLastGiven    = fTrue;
    BOOL                fHasHRTFreq         = fTrue;

    dprintf( "\n" );

    if ( !FFetchIOREQCHUNKSLIST( &pedbgioreqchunkRoot ) )
    {
        dprintf( "Error: Could not read some/all global IOREQ variables.\n" );
        goto HandleError;
    }

    if ( !FReadGlobal( "g_hrtLastGiven", &hrtLastGiven ) )
    {
        dprintf( "Warning: could not fetch a consistent value for g_hrtLastGiven. Some debugging capabilities will be disabled!\n" );
        fHasHRTLastGiven = fFalse;
    }

    if ( !FReadGlobal( "g_hrtHRTFreq", &hrtHRTFreq ) || hrtHRTFreq == 0 )
    {
        dprintf( "Warning: could not fetch a consistent value for g_hrtHRTFreq. Some debugging capabilities will be disabled!\n" );
        fHasHRTFreq = fFalse;
    }

    ULONG iChunk = 0;
    pedbgioreqchunkT = pedbgioreqchunkRoot;
    while ( pedbgioreqchunkT )
    {

        dprintf( "Scanning %d IOREQ's at 0x%p...\n",
                    pedbgioreqchunkT->pioreqchunkDebugger->cioreqMac,
                    pedbgioreqchunkT->pioreqchunkDebuggee->rgioreq );

        for ( ULONG iioreq = 0; iioreq < pedbgioreqchunkT->pioreqchunkDebugger->cioreqMac; iioreq++ )
        {
            IOREQ * const           pioreqT         = &(pedbgioreqchunkT->pioreqchunkDebugger->rgioreq[iioreq]);

            //  still skipping set size and extending write ops ... and I'm not sure
            //  of this STATUS_PENDING clause, I think it might be throwing out false
            //  positives ...
            if ( pioreqT->ovlp.Internal != STATUS_PENDING &&
                    !pioreqT->FOSIssuedState() )
                continue;

            if ( !FFetchVariable( (P_OSFILE)pioreqT->p_osf, &posf ) )
            {
                dprintf( "Error: Could not read p_posf at 0x%p for IOREQ.\n", pioreqT->p_osf );
                goto HandleError;
            }

            if ( !FFetchVariable( (COSFile *)posf->keyFileIOComplete, &pcosf ) )
            {
                dprintf( "Error: Could not read COSFile at 0x%p for specified OSFILE.\n", posf->keyFileIOComplete );
                goto HandleError;
            }

            dprintf( "\tOutstanding IOREQ chunk:index %i:%i:\n", iChunk, iioreq );
            dprintf( "\t%s Request\n", pioreqT->fWrite? "Write" : "Read" );
            dprintf( "\tStart(hrt):%I64u\n", pioreqT->hrtIOStart );
            if ( fHasHRTLastGiven )
            {
                const HRT dhrtPending = hrtLastGiven - pioreqT->hrtIOStart;

                dprintf( "\tPending(hrt):%I64u\n", dhrtPending );
                if ( fHasHRTFreq )
                {
                    dprintf( "\tPending(msec):%I64u\n", CmsecHRTFromDhrt( dhrtPending, hrtHRTFreq ) );
                }
            }
            dprintf( "\tBytes:0x%08X\n", pioreqT->cbData );
            dprintf( "\tOffset:0x%I64X\n", pioreqT->ibOffset );
            dprintf( "\tFile:%ws\n", pcosf->WszFile());
            dprintf( "\n" );
            Unfetch( posf );
            posf = NULL;
            Unfetch( pcosf );
            pcosf = NULL;
        }

        iChunk++;
        pedbgioreqchunkT = pedbgioreqchunkT->pNext;
    }

HandleError:
    //  unload parameters
    //
    dprintf( "\n--------------------\n\n" );
    Unfetch( posf );
    Unfetch( pcosf );
    UnfetchIOREQCHUNKSLIST( pedbgioreqchunkRoot );
}
#endif

BOOL FExistsPrecursorIOREQ(
    IOREQ *             pioreqTarget,
    EDBGIOREQCHUNK *    pedbgioreqchunkRoot )
{
    while ( pedbgioreqchunkRoot )
    {
        for ( ULONG iioreq = 0; iioreq < pedbgioreqchunkRoot->pioreqchunkDebugger->cioreqMac; iioreq++ )
        {
            IOREQ * const           pioreqT     = &(pedbgioreqchunkRoot->pioreqchunkDebugger->rgioreq[iioreq]);
            if ( pioreqT->pioreqIorunNext == pioreqTarget )
            {
                return fTrue;
            }
        }
        pedbgioreqchunkRoot = pedbgioreqchunkRoot->pNext;
    }
    return fFalse;
}


const CHAR mpioreqtypesz[ IOREQ::ioreqMax - IOREQ::ioreqUnknown ][ 29 ] =
{
    "ioreqUnknown",
    "ioreqInAvailPool",
    "ioreqCachedInTLS",
    "ioreqInReservePool",
    "ioreqAllocFromAvail",
    "ioreqAllocFromEwreqLookaside",
    "ioreqInIOCombiningList",
    "ioreqEnqueuedInIoHeap",
    "ioreqEnqueuedInVipList",
    "ioreqEnqueuedInMetedQ",
    "ioreqRemovedFromQueue",
    "ioreqIssuedSyncIO",
    "ioreqIssuedAsyncIO",
    "ioreqSetSize",
    "ioreqExtendingWriteIssued",
    "ioreqCompleted",
};

SAMPLE MinHisto( CStats * phisto )
{
    return phisto->C() ? phisto->Min() : 0;
}

SAMPLE MedianHisto( CStats * phisto )
{
    SAMPLE ret;
    ULONG ulFifty = 50;
    if ( phisto->ErrGetPercentileHits( &ulFifty, &ret ) == CStats::ERR::errSuccess )
    {
        return ret;
    }
    // assert?
    return 0;
}

//  decls from osdisk.cxx
ULONG CmsecLowWaitFromIOTime( const ULONG ciotime );
ULONG CmsecHighWaitFromIOTime( const ULONG ciotime );
//  translate to usec ...
ULONG CusecLowWaitFromIOTime( const ULONG ciotime )   { return CmsecLowWaitFromIOTime( ciotime ) * 1000; }
ULONG CusecHighWaitFromIOTime( const ULONG ciotime )  { return CmsecHighWaitFromIOTime( ciotime ) * 1000; }


//  ================================================================
DEBUG_EXT( EDBGDumpIOREQs )
//  ================================================================
{
    DWORD               cioOutstandingMaxT  = 0;
    DWORD               cioreqInUseT            = 0;
    EDBGIOREQCHUNK *    pedbgioreqchunkRoot = NULL;
    EDBGIOREQCHUNK *    pedbgioreqchunkT        = NULL;
    IOREQ::IOREQTYPE    ioreqtypeToDump         = IOREQ::ioreqInAvailPool;
    BOOL                fDumpNotType            = fTrue;
    const IOREQ::IOREQTYPE ioreqtypeAll         = IOREQ::IOREQTYPE( IOREQ::ioreqMax * 2 + 1 );  // impossible value

    #define iread  (0)
    #define iwrite (1)
    DWORD_PTR               rgrgcioreqs[iwrite+1][IOREQ::ioreqMax];
    DWORD_PTR               rgcioreqHeads[iwrite+1][IOREQ::ioreqMax];
    CPerfectHistogramStats  rgrghistoQueueWait[iwrite+1][IOREQ::ioreqMax];
    CPerfectHistogramStats  rgrghistoIoWait[iwrite+1][IOREQ::ioreqMax];


    BOOL                fDumpAllIOs             = fFalse;
    BOOL                fDumpAllIOREQs          = fFalse;
    BOOL                fPrintIOs               = fFalse;
    BOOL                fPrintIOREQs            = fTrue;
    BOOL                fValidUsage             = fTrue;

    HRT                 hrtLastGiven        = 0;
    HRT                 hrtHRTFreq          = 0;
    BOOL                fHasHRTFreq         = fTrue;

    extern HRT g_hrtLastGiven;
    extern HRT g_hrtHRTFreq;

    memset( rgrgcioreqs, 0, sizeof(rgrgcioreqs) );
    memset( rgcioreqHeads, 0, sizeof(rgcioreqHeads) );

    dprintf( "\n" );

    switch ( argc )
    {
        case 0:
            break;

        case 2:
            fDumpAllIOs = ( 0 == _stricmp( argv[1], "dumpallio" ) ||
                            0 == _stricmp( argv[1], "dumpallios" ) );
            fDumpAllIOREQs = ( 0 == _stricmp( argv[1], "dumpallioreq" ) ||
                               0 == _stricmp( argv[1], "dumpallioreqs" ) );

            fPrintIOs = ( 0 == _stricmp( argv[1], "printio" ) ||
                          0 == _stricmp( argv[1], "printios" ) );
            fPrintIOREQs = ( 0 == _stricmp( argv[1], "printioreq" ) ||
                             0 == _stricmp( argv[1], "printioreqs" ) );

            fValidUsage = fDumpAllIOREQs || fDumpAllIOs || fPrintIOs || fPrintIOREQs;

            // fall through to process type of IO to dump or print
        case 1:
            if ( 0 == _stricmp( argv[0], "*" ) )
            {
                ioreqtypeToDump = ioreqtypeAll; 
            }
            else
            {
                fDumpNotType = ( argv[0][0] == '-' || argv[0][0] == '!' /* b/c it is more intuitive */ );
                fValidUsage = ( fValidUsage
                                && FUlFromSz( &( argv[0][fDumpNotType ? 1 : 0] ), (ULONG *)&ioreqtypeToDump )
                                && ( ioreqtypeToDump < IOREQ::ioreqMax ) );
            }
            break;

        default:
            fValidUsage = fFalse;
    }

    AssertEDBG( ( fDumpAllIOs + fDumpAllIOREQs + fPrintIOs + fPrintIOREQs ) <= fTrue ); // expected only 1 print|dump option selected

    if ( !fValidUsage )
    {
        dprintf( "Usage: DUMPIOREQS [<ioreqtype>] [dumpallio|dumpallioreq\n\n" );
        dprintf( "    <ioreqtype>  - Selects * for all, or ioreqtype (in hex), or !ioreqtype (for not that type) to specify the\n"
                 "                   IOREQs to be printed or dumped.  Default = !%d (as in not InAvailPool)\n", IOREQ::ioreqInAvailPool );
        dprintf( "    printio      - if specified, selected IO heads will get 1 line summary print.\n" );
        dprintf( "    printioreq   - if specified, selected IOREQs will get 1 line summary print. [Default]\n" );
        dprintf( "    dumpallio    - if specified, selected IO heads will be dumped.\n" );
        dprintf( "    dumpallioreq - if specified, selected IOREQs will be dumped.\n" );
        goto HandleError;
    }

    if ( !FReadGlobal( "g_cioOutstandingMax", &cioOutstandingMaxT )
        || !FReadGlobal( "g_cioreqInUse", &cioreqInUseT )
        || !FReadGlobal( "g_hrtLastGiven", &hrtLastGiven )
        || !FReadGlobal( "g_hrtHRTFreq", &hrtHRTFreq )
        || !FFetchIOREQCHUNKSLIST( &pedbgioreqchunkRoot ) )
    {
        dprintf( "Error: Could not read some/all global IOREQ variables.\n" );
        goto HandleError;
    }

    if ( hrtHRTFreq == 0 )
    {
        dprintf( "Warning: could not fetch a consistent value for g_hrtHRTFreq. Some debugging capabilities will be disabled!\n" );
        fHasHRTFreq = fFalse;
    }

    if ( fPrintIOs || fPrintIOREQs || fDumpAllIOs || fDumpAllIOREQs )
    {
        dprintf( "Scanning %d IOREQ's for ioreqtype %hs %hs(%d)...\n", cioreqInUseT, 
                    ( ioreqtypeToDump == ioreqtypeAll ) ? "ALL" : ( fDumpNotType ? "NOT" : "" ),
                    mpioreqtypesz[ ioreqtypeToDump ],
                    ioreqtypeToDump );
    }
    else
    {
        dprintf( "Scanning %d IOREQ's to summarize...\n", cioreqInUseT );
    }
    if ( fPrintIOs || fPrintIOREQs )
    {
        EDBGPrintfDml( "\t%hs  typ(head)       ibOffset.cbData    grbitQOS   Queue Wait (usec) / IO Wait (usec) \n", 
                ( sizeof(void*) == 4 ) ? "  pioreq  " : "      pioreq      " );
    }



    QWORD cusecWaitingTotal = 0;    // theoretically could wrap, but unlikely ...
    QWORD cusecLowest = INT_MAX;
    QWORD cusecHighest = 0x0;
    QWORD cusecIoTimeWaitingTotal = 0;
    QWORD cusecIoTimeLowest = INT_MAX;
    QWORD cusecIoTimeHighest = 0;
    ULONG cIOsOver2Secs = 0;
    ULONG cIOsOver10Secs = 0;
    ULONG cIOsOver100Secs = 0;
    ULONG cNonHeadIOREQAdjustment = 0;
    ULONG cIoreqTotal = 0;

    ULONG cChunk = 0;
    pedbgioreqchunkT = pedbgioreqchunkRoot;
    while ( pedbgioreqchunkT )
    {
        if ( fDumpAllIOs || fDumpAllIOREQs )
        {
            //  This is nosiy and rarely useful, so only dump it if we want to see the IOs/IOREQs in the
            //  list, then we can show where in the IOREQCHUNK chain they are.
            dprintf( "Scanning %d IOREQ's at 0x%p...\n",
                        pedbgioreqchunkT->pioreqchunkDebugger->cioreqMac,
                        pedbgioreqchunkT->pioreqchunkDebuggee->rgioreq );
        }

        for ( ULONG iioreq = 0; iioreq < pedbgioreqchunkT->pioreqchunkDebugger->cioreqMac; iioreq++ )
        {
            IOREQ * const           pioreqT         = &(pedbgioreqchunkT->pioreqchunkDebugger->rgioreq[iioreq]);
            IOREQ * const           pioreqTDebuggee = &(pedbgioreqchunkT->pioreqchunkDebuggee->rgioreq[iioreq]);
            IOREQ::IOREQTYPE        ioreqtypeT = pioreqT->Ioreqtype();

            cIoreqTotal++;

            if ( ioreqtypeT < 0 || ioreqtypeT >= IOREQ::ioreqMax )
            {
                ioreqtypeT = IOREQ::ioreqUnknown;
            }
            
            rgrgcioreqs[!!pioreqT->fWrite][ioreqtypeT]++;

            // YUCK! Just made this O(n^2) b/c we need to know we're the head IOREQ if we're going to count ...
            BOOL fHeadIO = !FExistsPrecursorIOREQ( pioreqTDebuggee, pedbgioreqchunkRoot );

            if ( fHeadIO &&
                    ioreqtypeT != IOREQ::ioreqInAvailPool &&
                    ioreqtypeT != IOREQ::ioreqCachedInTLS &&
                    ioreqtypeT != IOREQ::ioreqInReservePool &&
                    ioreqtypeT != IOREQ::ioreqAllocFromAvail &&
                    ioreqtypeT != IOREQ::ioreqAllocFromEwreqLookaside )
            {
                rgcioreqHeads[!!pioreqT->fWrite][ioreqtypeT]++;
            }

            //
            // NOTE: This macro, must be matched with this if statement, or else the math won't work out...
            //
            #define     COutstandingIOREQs()    ( rgrgcioreqs[iread][IOREQ::ioreqRemovedFromQueue] + rgrgcioreqs[iwrite][IOREQ::ioreqRemovedFromQueue] + \
                                                    rgrgcioreqs[iread][IOREQ::ioreqIssuedSyncIO] + rgrgcioreqs[iwrite][IOREQ::ioreqIssuedSyncIO] + \
                                                    rgrgcioreqs[iread][IOREQ::ioreqIssuedAsyncIO] + rgrgcioreqs[iwrite][IOREQ::ioreqIssuedAsyncIO] + \
                                                    rgrgcioreqs[iread][IOREQ::ioreqSetSize] + rgrgcioreqs[iwrite][IOREQ::ioreqSetSize] + \
                                                    rgrgcioreqs[iread][IOREQ::ioreqExtendingWriteIssued] + rgrgcioreqs[iwrite][IOREQ::ioreqExtendingWriteIssued] )
                                                    // it is debatable if ioreqAllocFromEwreqLookaside is outstanding, but
                                                    // it may not be fully or properly initialized ... so rather not risk
                                                    // interpretation of the not fully init'd IOREQ.
            #define     FQueueWaiting( p )      ( !(p)->FInIssuedState() &&                        \
                                                    (p)->Ioreqtype() != IOREQ::ioreqInAvailPool &&   \
                                                    (p)->Ioreqtype() != IOREQ::ioreqCachedInTLS &&   \
                                                    (p)->Ioreqtype() != IOREQ::ioreqInReservePool && \
                                                    (p)->Ioreqtype() != IOREQ::ioreqCompleted )

            const HRT hrtIOStart = pioreqT->hrtIOStart;
            const HRT dhrtWaiting = ( ( (__int64)( hrtLastGiven - hrtIOStart ) ) < 0 ) ?
                                            0 : hrtLastGiven - hrtIOStart;
            const QWORD cusecWaiting = fHasHRTFreq ? CusecHRTFromDhrt( dhrtWaiting, hrtHRTFreq ) :  0;
            if ( pioreqT->FInIssuedState() )
            {
                if ( fHeadIO )
                {

                    if ( fHasHRTFreq )
                    {

                        cusecWaitingTotal += cusecWaiting;
                        if ( cusecWaiting > 0x20000000 ||
                            cusecWaitingTotal > 0x20000000 )
                        {
                            dprintf( "Improbable!  An IO or all IOs have been (cumulatively) waiting for more than 6 days ...?  Check math.\n" );
                        }

                        // give IO benefit of the doubt
                        cusecIoTimeWaitingTotal += CusecLowWaitFromIOTime( pioreqT->Ciotime() );
                        
                        if ( cusecWaiting < cusecLowest )
                        {
                            cusecLowest = cusecWaiting;
                        }
                        if ( CusecLowWaitFromIOTime( pioreqT->Ciotime() ) < cusecIoTimeLowest )
                        {
                            cusecIoTimeLowest = CusecLowWaitFromIOTime( pioreqT->Ciotime() );
                        }

                        if ( cusecWaiting > cusecHighest )
                        {
                            cusecHighest = cusecWaiting;
                        }
                        if ( CusecHighWaitFromIOTime( pioreqT->Ciotime() ) > cusecIoTimeHighest )
                        {
                            cusecIoTimeHighest = CusecHighWaitFromIOTime( pioreqT->Ciotime() );
                        }

                        if ( 100 * 1000 * 1000 < cusecWaiting )
                        {
                            cIOsOver100Secs++;
                        }
                        else if ( 10 * 1000 * 1000 < cusecWaiting )
                        {
                            cIOsOver10Secs++;
                        }
                        else if ( 2 * 1000 * 1000 < cusecWaiting )
                        {
                            cIOsOver2Secs++;
                        }

                        AssertEDBG( 0 == !!pioreqT->fWrite || 1 == !!pioreqT->fWrite );
                        rgrghistoIoWait[ !!pioreqT->fWrite ][ ioreqtypeT ].ErrAddSample( cusecWaiting );
                    
                    }

                }
                else
                {
                    cNonHeadIOREQAdjustment++;
                }
            } // pioreq->FInIssuedState() ...
            else if ( FQueueWaiting( pioreqT ) )
            {
                Assert( ioreqtypeT == IOREQ::ioreqAllocFromAvail ||
                        ioreqtypeT == IOREQ::ioreqAllocFromEwreqLookaside ||
                        ioreqtypeT == IOREQ::ioreqInIOCombiningList ||
                        ioreqtypeT == IOREQ::ioreqEnqueuedInIoHeap ||
                        ioreqtypeT == IOREQ::ioreqEnqueuedInVipList ||
                        ioreqtypeT == IOREQ::ioreqEnqueuedInMetedQ ||
                        ioreqtypeT == IOREQ::ioreqRemovedFromQueue );
                if ( fHeadIO && fHasHRTFreq )
                {
                    AssertEDBG( 0 == !!pioreqT->fWrite || 1 == !!pioreqT->fWrite );
                    rgrghistoQueueWait[ !!pioreqT->fWrite ][ ioreqtypeT ].ErrAddSample( cusecWaiting );
                }                
            }

            if ( ioreqtypeToDump == ioreqtypeAll ||     //  "all" signal"
                    ( !fDumpNotType && ioreqtypeToDump == ioreqtypeT ) ||    //  exact match
                    ( fDumpNotType && ioreqtypeToDump != ioreqtypeT ) )      //  inverted match
            {
                if ( fDumpAllIOs && fHeadIO )
                {
                    CHAR szAddr[32];
                    CHAR * rgArgv[2] = { szAddr, NULL };
                    OSStrCbFormatA( szAddr, sizeof(szAddr), "%p", pioreqTDebuggee );
                    CDUMP * pcdump = &(CDUMPA<IOREQ>::instance);
                    pcdump->Dump( pdebugClient, _countof(rgArgv)-1, rgArgv );
                }
                else if ( fDumpAllIOREQs )
                {
                    CHAR szAddr[32];
                    CHAR * rgArgv[3] = { szAddr, "norunstats", NULL };
                    OSStrCbFormatA( szAddr, sizeof(szAddr), "%p", pioreqTDebuggee );
                    CDUMP * pcdump = &(CDUMPA<IOREQ>::instance);
                    pcdump->Dump( pdebugClient, _countof(rgArgv)-1, rgArgv );
                }
                else if ( ( fPrintIOs && fHeadIO ) || fPrintIOREQs )
                {
                    EDBGPrintfDml( "\t<link cmd=\"!ese dump IOREQ %N\">0x%N</link>   %x %s %14I64d.%-7d  0x%08x   Q/IO Wait = %I64d / %I64d \n", 
                        pioreqTDebuggee, pioreqTDebuggee, (DWORD)ioreqtypeT, fHeadIO ? "(head)" : "      ", 
                        pioreqT->ibOffset, pioreqT->cbData,
                        pioreqT->grbitQOS,
                        FQueueWaiting( pioreqT ) ? cusecWaiting : 0, 
                        pioreqT->FInIssuedState() ? cusecWaiting : 0 );
                }
            }
        } // for each iioreq ...

        cChunk++;
        pedbgioreqchunkT = pedbgioreqchunkT->pNext;
    }

    //  report totals
    //

    if ( ioreqtypeToDump != ioreqtypeAll )
    {
        dprintf( "\n" );
        dprintf( "Total R/W of IOREQTYPE %hs%d: %u / %u\n", 
                    fDumpNotType ? "NOT " : "", ioreqtypeToDump, 
                    fDumpNotType ? ( cIoreqTotal - rgrgcioreqs[ iread ][ ioreqtypeToDump ] ) : rgrgcioreqs[ iread ][ ioreqtypeToDump ], 
                    fDumpNotType ? ( cIoreqTotal - rgrgcioreqs[ iwrite ][ ioreqtypeToDump ] ) : rgrgcioreqs[ iwrite ][ ioreqtypeToDump ] );
    }

    dprintf( "\n\n" );
    dprintf( "       g_pioreqchunkRoot: 0x%N\n", pedbgioreqchunkRoot->pioreqchunkDebuggee );
    dprintf( "     g_cioOutstandingMax: %lu (0x%lx)\n", cioOutstandingMaxT, cioOutstandingMaxT );
    dprintf( "          <cioreqchunks>: %lu (0x%lx)\n", cChunk, cChunk );
    dprintf( "             g_cioreqInUse: %lu (0x%lx)\n", cioreqInUseT, cioreqInUseT );
    dprintf( "\n" );

    CHAR szSpaces [] = "                             ";

    //  print out the summary counts for each IOREQTYPE accumulated ...

    for ( IOREQ::IOREQTYPE ioreqtypeT = IOREQ::ioreqUnknown; ioreqtypeT < IOREQ::ioreqMax; ioreqtypeT = (IOREQ::IOREQTYPE) ( ioreqtypeT + 1 ) )
    {
        const char * ioreqtypeSz = mpioreqtypesz[ioreqtypeT];
        if ( 0 == _strnicmp( ioreqtypeSz, "ioreq", 5 ) )
        {
            ioreqtypeSz += 5;
        }

        AssertEDBG( rgcioreqHeads[iread][ioreqtypeT] + rgcioreqHeads[iwrite][ioreqtypeT] <= ( rgrgcioreqs[iread][ioreqtypeT] + rgrgcioreqs[iwrite][ioreqtypeT] ) );

        EDBGPrintfDml( "%hs<link cmd=\"!ese dumpioreqs %x\">%s</link> (%x) R/W: %5u / %5u ",
                        szSpaces + strlen(ioreqtypeSz),
                        ioreqtypeT, ioreqtypeSz, ioreqtypeT, rgrgcioreqs[iread][ioreqtypeT], rgrgcioreqs[iwrite][ioreqtypeT] );

        if ( rgcioreqHeads[iread][ioreqtypeT] || rgcioreqHeads[iwrite][ioreqtypeT] )
        {
            dprintf( "  IOs (i.e. Heads): %5u  / %5u ", rgcioreqHeads[iread][ioreqtypeT], rgcioreqHeads[iwrite][ioreqtypeT] );

            switch( ioreqtypeT )
            {
            case IOREQ::ioreqInAvailPool:           //  in Avail pool
            case IOREQ::ioreqCachedInTLS:           //  cached in a ptls
            case IOREQ::ioreqInReservePool:         //  in the pool of reserved IO
                AssertEDBGSz( fFalse, "Shouldn't be any extra heads in these states anyways." );
                break; // no extra info worthy of printing out for these.

            case IOREQ::ioreqAllocFromAvail:        //  allocated from Avail pool (though not yet used)
            case IOREQ::ioreqAllocFromEwreqLookaside:// in the lookaside slot for a deferred extending write

            case IOREQ::ioreqInIOCombiningList:     //  in I/O combining link list
                break; // Could / should we track time in Alloc to get into queue??  queue contention time?

            case IOREQ::ioreqEnqueuedInIoHeap:      //  in I/O heap
            case IOREQ::ioreqEnqueuedInVipList:     //  in I/O VIP list
            case IOREQ::ioreqEnqueuedInMetedQ:      //  in I/O lower priority / meted queue
                if ( rgrghistoQueueWait[ iread ][ ioreqtypeT ].C() || rgrghistoQueueWait[ iwrite ][ ioreqtypeT ].C() )
                {
                    dprintf( "   Queue Lat: min %I64u / %I64u, ave-med %I64u - %I64u / %I64u - %I64u, max %I64u / %I64u",
                                MinHisto( &rgrghistoQueueWait[ iread ][ ioreqtypeT ] ), MinHisto( &rgrghistoQueueWait[ iwrite ][ ioreqtypeT ] ),
                                rgrghistoQueueWait[ iread ][ ioreqtypeT ].Ave(), MedianHisto( &rgrghistoQueueWait[ iread ][ ioreqtypeT ] ),
                                rgrghistoQueueWait[ iwrite ][ ioreqtypeT ].Ave(), MedianHisto( &rgrghistoQueueWait[ iwrite ][ ioreqtypeT ] ),
                                rgrghistoQueueWait[ iread ][ ioreqtypeT ].Max(), rgrghistoQueueWait[ iwrite ][ ioreqtypeT ].Max()
                               
                               );
                }
                break;

            case IOREQ::ioreqRemovedFromQueue:      //  removed from queue (heap || VIP) and I/O about to be issued

            case IOREQ::ioreqIssuedSyncIO:          //  "sync" I/O issued (or about to be issued) from foreground thread
            case IOREQ::ioreqIssuedAsyncIO:         //  async I/O issued (or about to be issued) from IO thread

            case IOREQ::ioreqSetSize:               //  servicing a request to set file size
            case IOREQ::ioreqExtendingWriteIssued:  //  extending write I/O issued (or about to be issued)

                if ( rgrghistoIoWait[ iread ][ ioreqtypeT ].C() || rgrghistoIoWait[ iwrite ][ ioreqtypeT ].C() )
                {
                    dprintf( "   IoSvc Lat: min %I64u / %I64u, ave-med %I64u - %I64u / %I64u - %I64u, max %I64u / %I64u",
                                MinHisto( &rgrghistoQueueWait[ iread ][ ioreqtypeT ] ), MinHisto( &rgrghistoQueueWait[ iwrite ][ ioreqtypeT ] ),
                                rgrghistoIoWait[ iread ][ ioreqtypeT ].Ave(), MedianHisto( &rgrghistoIoWait[ iread ][ ioreqtypeT ] ),
                                rgrghistoIoWait[ iwrite ][ ioreqtypeT ].Ave(), MedianHisto( &rgrghistoIoWait[ iwrite ][ ioreqtypeT ] ),
                                rgrghistoIoWait[ iread ][ ioreqtypeT ].Max(), rgrghistoIoWait[ iwrite ][ ioreqtypeT ].Max()
                               
                               );
                }
                break;

            case IOREQ::ioreqCompleted:             //  OS completed the IOREQ: processing completion
                break; // might be something interesting we could print out about this ...

            case IOREQ::ioreqMax:
            case IOREQ::ioreqUnknown:
            default:
                dprintf( "Unknown %d type\n", ioreqtypeT );
                AssertEDBGSz( fFalse, "Unknown ioreqtypeT" );
                break;
            }
        }

        dprintf( "\n" );
    }

    dprintf( "\n" );

    #define     COutstandingIOs()       ( COutstandingIOREQs() - cNonHeadIOREQAdjustment )

    if ( COutstandingIOs() )
    {
        //  There are outstanding IOs, report stats about them...
        //
        if ( fHasHRTFreq )
        {
            dprintf( "min/ave/max outstanding IOs(us): %I64u / %I64u / %I64u from %lu dispatched IOs (using %lu IOREQs)\n",
                        cusecLowest,
                        cusecWaitingTotal / COutstandingIOs(),
                        cusecHighest,
                        COutstandingIOs(),
                        COutstandingIOREQs() );
            dprintf( "min/ave/max IoTime-base IOs(us): %I64u / %I64u / %I64u (debugger adjusted values)\n",
                        cusecIoTimeLowest,
                        cusecIoTimeWaitingTotal / COutstandingIOs(),
                        cusecIoTimeHighest );
            if ( cIOsOver2Secs )
            {
                dprintf( "Note: %lu IOs waiting more than 2 seconds (but less than 10 secs).\n", cIOsOver2Secs );
            }
            if ( cIOsOver10Secs )
            {
                dprintf( "Warning: %lu IOs waiting more than 10 seconds (but less than 100 secs).\n", cIOsOver10Secs );
            }
            if ( cIOsOver100Secs )
            {
                dprintf( "Danger Danger: %lu IOs waiting more than 100 seconds! (Lassie, go get help!)\n", cIOsOver100Secs );
            }
        }
        else
        {
            dprintf( "Warning: could not fetch a consistent value for g_hrtHRTFreq. Latency statistics could not be computed!\n" );
            dprintf( "outstanding IOs: %lu dispatched IOs (using %lu IOREQs)\n",
                        COutstandingIOs(),
                        COutstandingIOREQs() );
        }

        dprintf( "\n" );

    }

HandleError:
    //  unload parameters
    //
    dprintf( "\n--------------------\n\n" );

    UnfetchIOREQCHUNKSLIST( pedbgioreqchunkRoot );
}


//  ================================================================
DEBUG_EXT( EDBGDumpPerfctr )
//  ================================================================
{
    ULONG   cbCounter               = sizeof(LONG);
    ULONG   cInstanceMax            = 0;
    VOID *  pvPerfInstance          = NULL;
    ULONG   ulOffset                = 0;
    PLS **  rgPLS                   = NULL;
    PLS *   pplsT                   = NULL;
    SIZE_T  cbPLS;
    ULONG   cpinstMaxT;
    ULONG   ifmpMaxT;
    ULONG   cProcsT;
    LONG    cbPlsMemRequiredPerPerfInstanceT;
    BOOL    fValidUsage             = fTrue;
    BYTE*   pbPerfCountersDebuggee;
    ULONG   cbPerfCountersT;

    dprintf( "\n" );

    switch ( argc )
    {
        case 2:
            fValidUsage = ( fValidUsage
                            && FUlFromSz( argv[1], &cbCounter )
                            && ( sizeof(LONG) == cbCounter || sizeof(QWORD) == cbCounter ) );
            //  FALL THROUGH to validate rest of args

        case 1:
            fValidUsage = ( fValidUsage
                            && FAddressFromSz( argv[0], &pvPerfInstance ) );
            break;

        default:
            fValidUsage = fFalse;
    }

    if ( !fValidUsage )
    {
        dprintf( "Usage: DUMPPERFCTR <pperfinstance> [<cbSize>]\n\n" );
        dprintf( "    <pperfinstance> - address of PERFInstance of desired performance counter\n" );
        dprintf( "    <cbSize> - size, in bytes, of the counter (default is sizeof(LONG))\n" );
        goto HandleError;
    }

    if ( !FReadGlobal( "g_cpinstMax", &cpinstMaxT )
        || !FReadGlobal( "g_ifmpMax", &ifmpMaxT )
        || !FReadGlobal( "OSSYNC::g_cProcessorMax", &cProcsT )
        || !FFetchGlobal( "OSSYNC::g_rgPLS", &rgPLS, cProcsT )
        || !FReadGlobal( "PLS::s_pbPerfCounters", &pbPerfCountersDebuggee )
        || !FReadGlobal( "PLS::s_cbPerfCounters", &cbPerfCountersT )
        || !FReadGlobal( "g_cbPlsMemRequiredPerPerfInstance", &cbPlsMemRequiredPerPerfInstanceT ) )
    {
        dprintf( "Error: Could not read some/all global variables.\n" );
        goto HandleError;
    }

    //  UNDONE: we're using a hack to extract m_iOffset, so try to detect
    //  if PERFInstance ever changes
    //
    if ( sizeof(PERFInstance<>) != sizeof(LONG) )
    {
        dprintf( "Error: Debugger extension has obsolete PERFInstance definition.\n" );
        goto HandleError;
    }

    //  UNDONE: huge hack here assumes that the counter offset
    //  is the first (and only) member of the class
    //
    if ( !FReadVariable( (ULONG *)pvPerfInstance, &ulOffset ) )
    {
        dprintf( "Error: Could not fetch specified performance counter.\n" );
        goto HandleError;
    }

    if ( ( pbPerfCountersDebuggee == NULL ) || ( cbPerfCountersT == 0 ) )
    {
        dprintf( "Error: Performance counter data not present.\n" );
        goto HandleError;
    }

    cInstanceMax = max( max( cpinstMaxT, ifmpMaxT ), tceMax ) + 1;

    cbPLS = sizeof(PLS);
    pplsT = (PLS *)( new BYTE[cbPLS] );
    if ( NULL == pplsT )
    {
        dprintf( "Error: Could not allocate PLS buffer.\n" );
        goto HandleError;
    }

    dprintf( "Instances not shown are ZERO.\n" );

    //  go through all instances of this counter
    //
    for ( ULONG iInstance = 0; iInstance < cInstanceMax; iInstance++ )
    {
        QWORD   qwCounter   = 0;
        DWORD   dwCounter   = 0;

        for ( ULONG iproc = 0; iproc < cProcsT; iproc++ )
        {
            //  fetch PLS for each proc
            //
            PLS * pplsDebuggee  = rgPLS[iproc];
            if ( !FReadVariable( (BYTE *)pplsDebuggee, (BYTE *)pplsT, cbPLS ) )
            {
                dprintf( "Error: Could not read PLS at 0x%N. Aborting.\n", pplsDebuggee );
                goto HandleError;
            }

            //  check if it points to a valid buffer
            //
            if ( ( pplsT->pbPerfCounters == NULL ) || ( pplsT->cbPerfCountersCommitted == 0 ) )
            {
                continue;
            }

            //  compute offset and check if it's within the committed region
            //
            const ULONG ibData = iInstance * cbPlsMemRequiredPerPerfInstanceT + ulOffset;
            const ULONG cbData = ibData + cbCounter;
            if ( cbData > pplsT->cbPerfCountersCommitted )
            {
                continue;
            }

            BYTE* const pbInstancePerfCountersDebuggee = pplsT->pbPerfCounters + ibData;
            AssertEDBGSz( ( pbInstancePerfCountersDebuggee + cbData ) <= ( pbPerfCountersDebuggee + cbPerfCountersT ) , "Inconsistent performance counter pointers." );

            //  fetch data
            //
            QWORD data = 0;
            if ( !FReadVariable( (BYTE *)pbInstancePerfCountersDebuggee, (BYTE *)&data, cbCounter ) )
            {
                dprintf( "Error: Could not read data at 0x%N. Aborting.\n", pbPerfCountersDebuggee );
                goto HandleError;
            }

            if ( sizeof(QWORD) == cbCounter )
            {
                qwCounter += data;
            }
            else
            {
                dwCounter += ( *(DWORD*)&data );
            }
        }

        if ( sizeof(QWORD) == cbCounter )
        {
            if ( qwCounter != 0 )
            {
                dprintf( "iInstance %d: %I64d (0x%I64x)\n", iInstance, qwCounter, qwCounter );
            }
        }
        else
        {
            if ( dwCounter != 0 )
            {
                dprintf( "iInstance %d: %d (0x%x)\n", iInstance, dwCounter, dwCounter );
            }
        }
    }

HandleError:
    //  unload parameters
    //
    dprintf( "\n--------------------\n\n" );

    delete[] (BYTE*)pplsT;
    Unfetch( rgPLS );
}


//  ================================================================
LOCAL VOID EDBGDumpIndexMetaData(
    const FCB * const   pfcbIndex,
    const FCB * const   pfcbTable,
    const FCB * const   pfcbIndexDebuggee )
//  ================================================================
{
    const TDB * const   ptdb        = pfcbTable->Ptdb();
    IDB *               pidb        = pidbNil;
    const IDXSEG *      rgidxseg;
    JET_SPACEHINTS      jsph        = { 0 };

    if ( !FFetchVariable( pfcbIndex->Pidb(), &pidb ) )
    {
        dprintf( "Error: Could not fetch IDB at 0x%N.\n", pfcbIndex->Pidb() );
        goto HandleError;
    }

    pfcbIndex->Pfcbspacehints()->GetAPISpaceHints( &jsph, Pdls()->CbPage() );

    dprintf( "Index: \"%s\"", ptdb->SzIndexName( pidb->ItagIndexName(), pfcbIndex->FDerivedIndex() ) );

    if ( pfcbIndex->FDerivedIndex() )
        dprintf( " (derived)" );

    dprintf( "\n" );
    dprintf( "    pfcb=0x%N\n", pfcbIndexDebuggee );
    dprintf( "    pidb=0x%N\n", pfcbIndex->Pidb() );
    dprintf( "    ifmp=0x%x\n", pfcbIndex->Ifmp() );
    dprintf( "    objidFDP=0x%x\n", pfcbIndex->ObjidFDP() );
    dprintf( "    pgnoFDP=0x%x\n", pfcbIndex->PgnoFDP() );
    dprintf( "    flags=0x%04x\n", pidb->FPersistedFlags() );
    dprintf( "    extended flags=0x%04x\n", pidb->FPersistedFlagsX() );
    dprintf( "    spacehints={ cbInitial=%d (%d,%d), grow=%d%%, init=%d%%, maint=%d%%, grbit=0x%x }\n",
             jsph.cbInitial, jsph.cbMinExtent, jsph.cbMaxExtent, jsph.ulGrowth, jsph.ulInitialDensity,
             jsph.ulMaintDensity, jsph.grbit );
    if ( pidb->FTuples() )
    {
        dprintf( "    tuple (min=%hd, max=%hd, indexmax=%hd, inc=%hd, start=%hd)\n",
        pidb->CchTuplesLengthMin(),
        pidb->CchTuplesLengthMax(),
        pidb->IchTuplesToIndexMax(),
        pidb->CchTuplesIncrement(),
        pidb->IchTuplesStart() );
    }

    dprintf( "    key segments (%d):\n", pidb->Cidxseg() );
    rgidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb );
    for ( ULONG i = 0; i < pidb->Cidxseg(); i++ )
    {
        const BOOL          fDerived    = ( rgidxseg[i].FTemplateColumn() && !ptdb->FTemplateTable() );
        const FIELD * const pfield      = ptdb->Pfield( rgidxseg[i].Columnid() );
        dprintf(
            "        %c%s (0x%08x)\n",
            ( rgidxseg[i].FDescending() ? '-' : '+' ),
            ptdb->SzFieldName( pfield->itagFieldName, fDerived ),
            rgidxseg[i].Columnid() );
    }

    if ( pidb->CidxsegConditional() > 0 )
    {
        dprintf( "    conditional columns (%d):\n", pidb->CidxsegConditional() );
        rgidxseg = PidxsegIDBGetIdxSegConditional( pidb, ptdb );
        for ( ULONG i = 0; i < pidb->CidxsegConditional(); i++ )
        {
            const BOOL          fDerived    = ( rgidxseg[i].FTemplateColumn() && !ptdb->FTemplateTable() );
            const FIELD * const pfield      = ptdb->Pfield( rgidxseg[i].Columnid() );
            dprintf(
                "        %s (0x%08x, %s)\n",
                ptdb->SzFieldName( pfield->itagFieldName, fDerived ),
                rgidxseg[i].Columnid(),
                ( rgidxseg[i].FMustBeNull() ? "MustBeNull" : "MustBeNonNull" ) );
        }
    }

    dprintf( "\n" );

HandleError:
    Unfetch( pidb );
}

//  ================================================================
LOCAL VOID EDBGDumpColumnMetaData(
    const TDB * const       ptdb,
    const COLUMNID          columnid )
//  ================================================================
{
    const FIELD * const     pfield      = ptdb->Pfield( columnid );
    const BOOL              fDeleted    = ( 0 == pfield->itagFieldName );
    const CHAR * const      szType      = ( fDeleted ? "<deleted>" : SzColumnType( pfield->coltyp ) );

    dprintf( "    0x%04x - ", FidOfColumnid( columnid ) );

    if ( fDeleted )
    {
        dprintf( "<deleted>" );
    }
    else
    {
        dprintf( "\"%s\"", ptdb->SzFieldName( pfield->itagFieldName, fFalse ) );
    }

    dprintf(
        "  [%s%s, ",
        szType,
        ( FRECTextColumn( pfield->coltyp ) && usUniCodePage == pfield->cp ? " (Unicode)" : "" ) );

    if ( FidOfColumnid( columnid ).FFixed() )
    {
        dprintf( "offset=0x%04x, ", pfield->ibRecordOffset );
    }

    dprintf( "flags=0x%04x]\n", pfield->ffield );
}

//  ================================================================
LOCAL VOID EDBGDumpTableMetaData(
    FCB * const         pfcbDebuggee,
    const TDB * const   ptdbDebuggee )
//  ================================================================
{
    FCB *               pfcbTable   = pfcbNil;
    FCB *               pfcbIndex   = pfcbNil;

    if ( FEDBGFetchTableMetaData( pfcbDebuggee, &pfcbTable ) )
    {
        const TDB * const   ptdb    = pfcbTable->Ptdb();
        JET_SPACEHINTS      jsph    = { 0 };

        pfcbTable->Pfcbspacehints()->GetAPISpaceHints( &jsph, Pdls()->CbPage() );

        dprintf( "Table: \"%s\"", ptdb->SzTableName() );

        if ( pfcbTable->FDerivedTable() )
            dprintf( " (derived)" );

        dprintf( "\n" );
        dprintf( "    pfcb=0x%N\n", pfcbDebuggee );
        dprintf( "    ptdb=0x%N\n", ptdbDebuggee );
        dprintf( "    ifmp=0x%x\n", pfcbTable->Ifmp() );
        dprintf( "    objidFDP=0x%x\n", pfcbTable->ObjidFDP() );
        dprintf( "    pgnoFDP=0x%x\n", pfcbTable->PgnoFDP() );
        dprintf( "    pfcbLV=0x%N\n", ptdb->PfcbLV() );
        dprintf( "    spacehints={ cbInitial=%d (%d,%d), grow=%d%%, init=%d%%, maint=%d%%, grbit=0x%x }\n",
                 jsph.cbInitial, jsph.cbMinExtent, jsph.cbMaxExtent, jsph.ulGrowth, jsph.ulInitialDensity,
                 jsph.ulMaintDensity, jsph.grbit );
        dprintf( "\n" );

        //  if primary index exists, dump it
        //
        if ( pidbNil != pfcbTable->Pidb() )
        {
            EDBGDumpIndexMetaData( pfcbTable, pfcbTable, pfcbDebuggee );
        }

        pfcbIndex = (FCB *)LocalAlloc( 0, sizeof(FCB) );
        if ( pfcbNil == pfcbIndex )
        {
            dprintf( "Error: Could not allocate FCB buffer.\n" );
            goto HandleError;
        }

        //  dump secondary indexes
        //
        for ( FCB * pfcbT = pfcbTable->PfcbNextIndex();
            pfcbNil != pfcbT;
            pfcbT = pfcbIndex->PfcbNextIndex() )
        {
            if ( !FReadVariable( pfcbT, pfcbIndex ) )
            {
                dprintf( "Error: Could not read FCB at 0x%N.\n", pfcbT );
                goto HandleError;
            }

            EDBGDumpIndexMetaData( pfcbIndex, pfcbTable, pfcbT );
        }

        //  dump columns
        //
        dprintf( "Columns:\n" );

        for ( FID fid = ptdb->FidFixedFirst(); fid <= ptdb->FidFixedLast(); fid++ )
        {
            EDBGDumpColumnMetaData( ptdb, ColumnidOfFid( fid, ptdb->FTemplateTable() ) );
        }
        for ( FID fid = ptdb->FidVarFirst(); fid <= ptdb->FidVarLast(); fid++ )
        {
            EDBGDumpColumnMetaData( ptdb, ColumnidOfFid( fid, ptdb->FTemplateTable() ) );
        }
        for ( FID fid = ptdb->FidTaggedFirst(); fid <= ptdb->FidTaggedLast(); fid++ )
        {
            EDBGDumpColumnMetaData( ptdb, ColumnidOfFid( fid, ptdb->FTemplateTable() ) );
        }

        dprintf( "\n" );
    }


HandleError:
    LocalFree( pfcbIndex );
    Unfetch( pfcbTable );
}


//  ================================================================
DEBUG_EXT( EDBGDumpMetaData )
//  ================================================================
{
    SaveDlsDefaults sdd; // saves here, and then restores the implicit defaults on .dtor.

    FCB *       pfcbDebuggee        = pfcbNil;
    FCB *       pfcb                = pfcbNil;
    const BOOL  fValidUsage         = ( 1 == argc && FAutoAddressFromSz( argv[0], &pfcbDebuggee ) );

    dprintf( "\n" );

    if ( !fValidUsage )
    {
        dprintf( "Usage: DUMPMETADATA <pfcb>\n\n" );
        dprintf( "    <pfcb>|. - address of FCB for which meta-data is to be dumped\n" );
        goto HandleError;
    }

    if ( !FFetchVariable( pfcbDebuggee, &pfcb ) )
    {
        dprintf( "Error: Could not fetch FCB at 0x%N.\n", pfcbDebuggee );
        goto HandleError;
    }

    if ( pfcb->FTypeTable() )
    {
        EDBGDumpTableMetaData( pfcbDebuggee, pfcb->Ptdb() );
    }
    else if ( pfcb->FTypeSecondaryIndex() )
    {
        FCB *   pfcbTable;

        if ( FEDBGFetchTableMetaData( pfcb->PfcbTable(), &pfcbTable ) )
        {
            EDBGDumpIndexMetaData( pfcb, pfcbTable, pfcbDebuggee );
            // REVIEW: Unfetch( pfcbTable )?
        }
    }
    else
    {
        CHAR *  szBtree;

        if ( pfcb->FTypeDatabase() )
        {
            szBtree = "Database";
        }
        else if ( pfcb->FTypeTemporaryTable() )
        {
            szBtree = "TempTable";
        }
        else if ( pfcb->FTypeSort() )
        {
            szBtree = "Sort";
        }
        else if ( pfcb->FTypeLV() )
        {
            szBtree = "Long-Value";
        }
        else
        {
            szBtree = "UNKNOWN";
        }

        dprintf(
            "Btree: <%s>  [pfcb=0x%N, ifmp=0x%x, objidFDP=0x%x, pgnoFDP=0x%x, pfcbTable=0x%N]\n",
            szBtree,
            pfcbDebuggee,
            pfcb->Ifmp(),
            pfcb->ObjidFDP(),
            pfcb->PgnoFDP(),
            pfcb->PfcbTable() );
    }

HandleError:

    //  unload parameters
    //
    dprintf( "\n--------------------\n\n" );

    Unfetch( pfcb );
}


//  ================================================================
LOCAL BOOL FEDBGReadAndCheckFCB(
    FCB * const     pfcbDebuggee,
    FCB * const     pfcb,
    const ULONG     ulFDP,
    BOOL * const    pfFoundFCB )
//  ================================================================
{
    const BOOL      fResult     = FReadVariable( pfcbDebuggee, pfcb );

    if ( fResult )
    {
        if ( pfcb->ObjidFDP() == ulFDP || pfcb->PgnoFDP() == ulFDP )
        {
            EDBGPrintfDmlW(
                L"	   <link cmd=\"dt %ws!FCB 0x%N\">0x%N</link> [ref: %ld, fInLRU:%d, fPurgeable:%d, type: %ws, ifmp:0x%x, objidFDP:0x%x, pgnoFDP:0x%x]\n",
                WszUtilImageName(),
                pfcbDebuggee,
                pfcbDebuggee,
                pfcb->WRefCount(),
                pfcb->FInLRU(),
                pfcb->FDebuggerExtPurgableEstimate(),
                pfcb->WszFCBType(),
                pfcb->Ifmp(),
                pfcb->ObjidFDP(),
                pfcb->PgnoFDP());

            *pfFoundFCB = fTrue;
        }
    }
    else
    {
        dprintf( "    Error: Could not read FCB at 0x%N. Aborting.\n", pfcbDebuggee );
    }

    return fResult;
}

//  ================================================================
DEBUG_EXT( EDBGFCBFind )
//  ================================================================
{
    ULONG       ulFDP               = 0;
    INST *      pinstTarget         = NULL;
    INST *      pinst               = pinstNil;
    FCB *       pfcb                = pfcbNil;
    TDB *       ptdb                = ptdbNil;
    BOOL        fFoundFCB           = fFalse;
    BOOL        fValidUsage         = fTrue;

    switch ( argc )
    {
        case 2:
            fValidUsage = ( fValidUsage && FAddressFromSz( argv[1], &pinstTarget ) );
            //  FALL THROUGH to validate rest of args

        case 1:
            fValidUsage = ( fValidUsage
                            && FUlFromSz( argv[0], &ulFDP )
                            && ulFDP > 0 );
            break;

        default:
            fValidUsage = fFalse;
    }

    if ( !fValidUsage )
    {
        dprintf( "\nUsage: FCBFIND <FDP> [<instance>]\n\n" );
        dprintf( "    <FDP> - the objidFDP or pgnoFDP to search for\n" );
        dprintf( "    <instance> - an optional parameter specifying the pointer\n" );
        dprintf( "                 to the instance for which to limit the scan\n" );
        goto HandleError;
    }

    pfcb = (FCB *)LocalAlloc( 0, sizeof(FCB) );
    if ( pfcbNil == pfcb )
    {
        dprintf( "\nError: Could not allocate FCB buffer.\n" );
        goto HandleError;
    }

    ptdb = (TDB *)LocalAlloc( 0, sizeof(TDB) );
    if ( ptdbNil == ptdb )
    {
        dprintf( "\nError: Could not allocate TDB buffer.\n" );
        goto HandleError;
    }

    if ( Pdls()->ErrINSTDLSInitCheck() < JET_errSuccess )
    {
        goto HandleError;
    }

    //  scan FCB list of all INST's
    //
    for ( SIZE_T ipinst = 0; ipinst < Pdls()->Cinst(); ipinst++ )
    {
        if ( pinstNil == Pdls()->PinstDebuggee( ipinst ) )
        {
            continue;
        }

        if ( pinstTarget && pinstTarget != Pdls()->PinstDebuggee( ipinst ) )
        {
            continue;
        }

        dprintf( "\nScanning all FCB's of instance 0x%N...\n", Pdls()->PinstDebuggee( ipinst ) );
        if ( ( pinst = Pdls()->Pinst( ipinst ) ) )
        {
            FCB *   pfcbDebuggee;
            FCB *   pfcbNextInList  = pfcbNil;

            for ( pfcbDebuggee = pinst->m_pfcbList;
                pfcbNil != pfcbDebuggee;
                pfcbDebuggee = pfcbNextInList )
            {
                FCB *   pfcbIndexes;

                //  check main FCB
                //
                if ( !FEDBGReadAndCheckFCB( pfcbDebuggee, pfcb, ulFDP, &fFoundFCB ) )
                {
                    goto HandleError;
                }

                //  pfcb variable will be re-used, so save
                //  off pointer to next FCB in global list
                //
                pfcbNextInList = pfcb->PfcbNextList();
                pfcbIndexes = pfcb->PfcbNextIndex();

                //  check possible LV FCB
                //
                if ( NULL != pfcb->Ptdb() )
                {
                    if ( !FReadVariable( pfcb->Ptdb(), ptdb ) )
                    {
                        dprintf( "    Error: Could not read TDB at 0x%N. Aborting.\n", pfcb->Ptdb() );
                        goto HandleError;
                    }

                    if ( NULL != ptdb->PfcbLV()
                        && !FEDBGReadAndCheckFCB( ptdb->PfcbLV(), pfcb, ulFDP, &fFoundFCB ) )
                    {
                        goto HandleError;
                    }
                }

                //  check possible index FCB's
                //
                for ( pfcbDebuggee = pfcbIndexes;
                    pfcbNil != pfcbDebuggee;
                    pfcbDebuggee = pfcb->PfcbNextIndex() )
                {
                    if ( !FEDBGReadAndCheckFCB( pfcbDebuggee, pfcb, ulFDP, &fFoundFCB ) )
                    {
                        goto HandleError;
                    }
                }
            }
        }
        else
        {
            if ( NULL != pinstTarget )
            {
                dprintf( "    Error: Could not read INST at 0x%N. Aborting.\n", pinstTarget );
            }
            else
            {
                dprintf(
                    "    g_rgpinst[0x%x]  Error: Could not read INST at 0x%N. Aborting.\n",
                    ipinst,
                    Pdls()->PinstDebuggee( ipinst ) );
            }
            goto HandleError;
        }
    }

    if ( !fFoundFCB )
    {
        //  we did not find the FCB
        //
        dprintf( "\nCould not find any FCB's with an objidFDP or pgnoFDP of %d (0x%x).\n", ulFDP, ulFDP );
    }


HandleError:

    //  unload parameters
    //
    dprintf( "\n--------------------\n\n" );

    LocalFree( ptdb );
    LocalFree( pfcb );
}

enum DumpFCBFilter
{
    dumpfcbfilterInvalid = 0,
    dumpfcbfilterInUse,
    dumpfcbfilterAvailable,
    dumpfcbfilterPurgeable,
    dumpfcbfilterUnpurgeable,
    dumpfcbfilterAll
};

//  ================================================================
LOCAL VOID DumpFCB(
    _In_ const INST * const     pinst,
    _In_ const FCB * const      pfcbDebuggee,
    _In_ const FCB * const      pfcb,
    _In_ const TDB * const      ptdb,
    _In_ const BOOL             fSubFCB
    )
//  ================================================================
{
    if ( fSubFCB )
    {
        dprintf("    ");
    }

    CHAR *   szTableName  = ( ptdb && ptdb->SzTableName() ) ? ptdb->SzTableName() : "   ";

    if ( pfcb->ObjidFDP() == objidSystemRoot )
    {
        szTableName = "[DbRoot]";
    }
    //   Ah-hem ... someone made the name of the MSysObjectsShadow in in the TDB == MSysObjects (with 
    //   no Shadow), see comments in FEDBGTableFind().
    else if ( pfcb->ObjidFDP() == objidFDPMSOShadow )
    {
        szTableName = "MSysObjectsShadow";
    }

    IDB *         pidb           = NULL;
    const CHAR *  szIndexName    = ""; //  Also for LVs
    const CHAR *  szIndexAppend  = "";
    if ( pfcb->FTypeLV() )
    {
        szIndexName = szLvIndexSelect;
    }
    else if ( pfcb->FTypeSecondaryIndex() )
    {
        if ( FFetchVariable( pfcb->Pidb(), &pidb ) )
        {
            szIndexName = ptdb->SzIndexName( pidb->ItagIndexName(), pfcb->FDerivedIndex() );
            if ( pfcb->FDerivedIndex() )
            {
                szIndexAppend = " (derived)";
            }
        }
    }

    //  Now that the name is complete, the type is a bit superflous.
    EDBGPrintfDmlW(
        L"    <link cmd=\"!ese dump FCB 0x%N\">0x%N</link>%ws [ref: %6ld, fInLRU:%d, fPurgeable:%d, "
            L"type: <link cmd=\"!ese dumpmetadata 0x%N\">%24s</link>, ifmp:0x%x, objidFDP:%#5x, pgnoFDP:%#8x, name: %hs%hs%hs%hs ]\n",
        pfcbDebuggee,
        pfcbDebuggee,
        !fSubFCB ? L"    " : L"",
        pfcb->WRefCount(),
        pfcb->FInLRU(),
        pfcb->FDebuggerExtPurgableEstimate(),
        pfcbDebuggee,
        pfcb->WszFCBType(),
        pfcb->Ifmp(),
        pfcb->ObjidFDP(),
        pfcb->PgnoFDP(),
        szTableName,
        ( szIndexName[0] != '\0' ) ? "\\" : "",
        szIndexName, szIndexAppend );

    Unfetch( pidb );
}

//  ================================================================
LOCAL VOID CollectFCBInfo(
    __in const FCB * const      pfcb,
    __in ULONG * const          pcFCBs,
    __inout BOOL * const        pfUnpurgeableFCB,
    __inout BOOL * const        pfAvailableFCB,
    __inout_ecount(tceMax) ULONG * const        rgcFCBsByTCE
    )
//  ================================================================
{
    FetchWrap<FCB *>    pfcbTable;

    // This function assumes that it is inspecting a tree,
    // and as such it will mark the entire tree as unavailable
    // at finding the first FCB in use.
    //
    // Likewise, at the moment we find the first unpurgeable,
    // the whole tree is assume unpurgeable.        
    //
    if ( pfcb->FDebuggerExtInUse() )
    {
        (*pfAvailableFCB) = fFalse;
    }
    else
    {
        if ( !pfcb->FDebuggerExtPurgableEstimate() )
        {
            (*pfUnpurgeableFCB) = fTrue;
        }
    }

    (*pcFCBs)++;
    if ( rgcFCBsByTCE )
    {
        // We might need the table FCB in order to compute
        // the tableclass, so fetch it if present.
        FCB * const     pfcbTableDebuggee   = pfcb->PfcbTable();
        if ( NULL != pfcbTableDebuggee && !pfcbTable.FVariable( pfcbTableDebuggee ) )
        {
            dprintf( "    Error: Could not read PfcbTable at 0x%N.\n", pfcb->PfcbTable() );
            rgcFCBsByTCE[tceNone]++;
        }
        else
        {
            // Temporarily re-direct the debugger's copy of the FCB to point
            // to the debugger's copy of pfcbTable so that we can retrieve the
            // tableclass for the FCB.
            ( (FCB*)pfcb )->SetPfcbTable( pfcbTable );
            TCE tce = pfcb->TCE();
            if ( tce < tceMax )
            {
                rgcFCBsByTCE[tce]++;
            }
            else
            {
                rgcFCBsByTCE[tceNone]++;
            }
            ( (FCB*)pfcb )->SetPfcbTable( pfcbTableDebuggee );
        }
    }
}

//  ================================================================
LOCAL VOID CollectFCBTreeInfo(
    __in const INST * const     pinst,
    __in const FCB * const      pfcb,
    __inout ULONG * const       pcFCBs,
    __inout BOOL * const        pfUnpurgeableFCB,
    __inout BOOL * const        pfAvailableFCB,
    __inout_ecount(tceMax) ULONG * const        rgcFCBsByTCE
    )
//  ================================================================
{
    FCB *                   pfcbNextIndexDebuggee   = pfcbNil;
    FetchWrap<TDB *>        ptdb;
    FetchWrap<FCB *>        pfcbLV;

    // We will consider a tree unpurgeable and available as
    // we walk through it. CollectFCBInfo will set the tree
    // as unpurgeable/in-use as it encounters such cases
    // on a case-by-case inspection of FCBs.
    //
    (*pfUnpurgeableFCB) = fFalse;
    (*pfAvailableFCB) = fTrue;

    // Inspect the immediate FCB ("root" FCB). 
    //
    CollectFCBInfo(
        pfcb,
        pcFCBs,
        pfUnpurgeableFCB,
        pfAvailableFCB,
        rgcFCBsByTCE );

    //  Inspect the LV FCB out of the TDB.
    //
    if ( NULL != pfcb->Ptdb() )
    {
        if ( !ptdb.FVariable( pfcb->Ptdb() ) )
        {
            dprintf( "    Error: Could not read TDB at 0x%N. Aborting.\n", pfcb->Ptdb() );
            return;
        }

        if ( NULL != ptdb->PfcbLV() )
        {
            if ( !pfcbLV.FVariable( ptdb->PfcbLV() ) )
            {
                dprintf( "    Error: Could not read pfcbLV at 0x%N. Aborting.\n", ptdb->PfcbLV() );
                return;
            }

            // Check how the LV FCB is doing.
            //
            CollectFCBInfo(
                pfcbLV,
                pcFCBs,
                pfUnpurgeableFCB,
                pfAvailableFCB,
                rgcFCBsByTCE );
        }
    }

    //  Check possible index FCBs.
    //
    for ( FCB * pfcbIndexDebuggee = pfcb->PfcbNextIndex();
        pfcbNil != pfcbIndexDebuggee;
        pfcbIndexDebuggee = pfcbNextIndexDebuggee )
    {
        FetchWrap<FCB *>        pfcbIndex;

        if ( !pfcbIndex.FVariable( pfcbIndexDebuggee ) )
        {
            dprintf( "    Error: Could not read pfcbIndex at 0x%N. Aborting.\n", pfcbIndexDebuggee );
            return;
        }
        
        CollectFCBInfo(
            pfcbIndex,
            pcFCBs,
            pfUnpurgeableFCB,
            pfAvailableFCB,
            rgcFCBsByTCE );
            
        pfcbNextIndexDebuggee = pfcbIndex->PfcbNextIndex();
    }
}

//  ================================================================
LOCAL VOID DumpFCBList(
    __in INST * const   pinstDebuggee,
    DumpFCBFilter       edumpFCBFilter
    )
//  ================================================================
{
    FetchWrap<INST *>       pinst;
    
    dprintf( "\nScanning all FCB's of instance 0x%N...\n", pinstDebuggee );
    if ( !pinst.FVariable( pinstDebuggee ) )
    {
        dprintf( "    Error: Could not read pinst at 0x%N. Aborting.\n", pinstDebuggee );
        return;
    }

    FCB *   pfcbDebuggee            = pfcbNil;
    FCB *   pfcbNextInListDebuggee  = pfcbNil;
    FCB *   pfcbNextIndexDebuggee   = pfcbNil;

    // We'll walk across all the FCBs in the list of the INST.
    //
    for ( pfcbDebuggee = pinst->m_pfcbList;
        pfcbNil != pfcbDebuggee;
        pfcbDebuggee = pfcbNextInListDebuggee )
    {
        FetchWrap<TDB *>        ptdb;
        FetchWrap<FCB *>        pfcb;
        FetchWrap<FCB *>        pfcbLV;
        
        if ( !pfcb.FVariable( pfcbDebuggee ) )
        {
            dprintf( "    Error: Could not read pfcb at 0x%N. Aborting.\n", pfcbDebuggee );
            return;
        }

        pfcbNextInListDebuggee = pfcb->PfcbNextList();

        // If a filter is specified, we need to determine the
        // FCB tree info and then make a call based on the
        // filtering.
        //
        if ( dumpfcbfilterInvalid != edumpFCBFilter &&
            dumpfcbfilterAll != edumpFCBFilter )
        {
            // We will assume a tree is purgeable and available
            // unless during the collection below it is shown otherwise.
            //
            ULONG   cFCBTree = 0;
            BOOL    fUnpurgeableFCB = fFalse;
            BOOL    fAvailableFCB = fTrue;

            // Collect the state of the FCB tree.
            //
            CollectFCBTreeInfo(
                pinst,
                pfcb,
                &cFCBTree,
                &fUnpurgeableFCB,
                &fAvailableFCB,
                NULL );

            switch( edumpFCBFilter )
            {
                case dumpfcbfilterInUse:
                    if ( fAvailableFCB )
                    {
                        continue;
                    }
                    break;
            
                case dumpfcbfilterAvailable:
                    if ( !fAvailableFCB )
                    {
                        continue;
                    }
                    break;
            
                case dumpfcbfilterPurgeable:
                    if ( !fAvailableFCB || fUnpurgeableFCB )
                    {
                        continue;
                    }
                    break;
            
                case dumpfcbfilterUnpurgeable:
                    if ( !fAvailableFCB || !fUnpurgeableFCB )
                    {
                        continue;
                    }
                    break;

                case dumpfcbfilterInvalid:
                case dumpfcbfilterAll:
                    AssertEDBGSz( fFalse, "We should not be filtering if we have no filter." );
                    break;
            
                default:
                    break;
            }
        }

        // Just the cheapest way to get the table's metadata, including TDB and table name hanging
        // off that for friendly printing.
        FCB * pfcbWithPtdb = NULL;
        (void)FEDBGFetchTableMetaData( pfcbDebuggee, &pfcbWithPtdb );

        DumpFCB( pinst, pfcbDebuggee, pfcb, pfcbWithPtdb->Ptdb(), fFalse );

        //  Check possible LV FCB.
        //
        if ( NULL != pfcb->Ptdb() )
        {
            if ( !ptdb.FVariable( pfcb->Ptdb() ) )
            {
                dprintf( "	  Error: Could not read TDB at 0x%N. Aborting.\n", pfcb->Ptdb() );
                return;
            }

            if ( NULL != ptdb->PfcbLV() )
            {
                if ( !pfcbLV.FVariable( ptdb->PfcbLV() ) )
                {
                    dprintf( "    Error: Could not read pfcbLV at 0x%N. Aborting.\n", ptdb->PfcbLV() );
                    return;
                }
                
                DumpFCB( pinst, ptdb->PfcbLV(), pfcbLV, pfcbWithPtdb->Ptdb(), fTrue );
            }
        }

        //  check possible index FCB's
        //
        for ( FCB * pfcbIndexDebuggee = pfcb->PfcbNextIndex();
            pfcbNil != pfcbIndexDebuggee;
            pfcbIndexDebuggee = pfcbNextIndexDebuggee )
        {
            FetchWrap<FCB *>        pfcbIndex;

            if ( !pfcbIndex.FVariable( pfcbIndexDebuggee ) )
            {
                dprintf( "    Error: Could not read pfcbIndex at 0x%N. Aborting.\n", pfcbIndexDebuggee );
                return;
            }
            
            DumpFCB( pinst, pfcbIndexDebuggee, pfcbIndex, pfcbWithPtdb->Ptdb(), fTrue );

            pfcbNextIndexDebuggee = pfcbIndex->PfcbNextIndex();
        }

        Unfetch( pfcbWithPtdb );
    }
}

//  ================================================================
LOCAL DumpFCBFilter ParseEDumpFCBFilter( const CHAR cArg )
//  ================================================================
{
    DumpFCBFilter edumpFCBFilter;
    
    switch( cArg )
    {
        case '*':
            edumpFCBFilter = dumpfcbfilterAll;
            break;

        case 'i':
            edumpFCBFilter = dumpfcbfilterInUse;
            break;

        case 'a':
            edumpFCBFilter = dumpfcbfilterAvailable;
            break;

        case 'p':
            edumpFCBFilter = dumpfcbfilterPurgeable;
            break;

        case 'u':
            edumpFCBFilter = dumpfcbfilterUnpurgeable;
            break;

        default:
            edumpFCBFilter = dumpfcbfilterInvalid;
            break;
    }

    return edumpFCBFilter;
}
    
//  ================================================================
DEBUG_EXT( EDBGDumpFCBs )
//  ================================================================
{
    BOOL fValidUsage                    = fTrue;
    INST *  pinst                       = NULL;
    DumpFCBFilter edumpFCBFilter        = dumpfcbfilterInvalid;

    switch ( argc )
    {
        case 2:
            if ( !FAutoAddressFromSz( argv[1], &pinst ) )
            {
                fValidUsage = fFalse;
            }
            
            __fallthrough;
            //  FALL THROUGH to validate rest of args
    
        case 1:
            edumpFCBFilter = ParseEDumpFCBFilter( argv[0][0] );

            if ( dumpfcbfilterInvalid == edumpFCBFilter )
            {
                fValidUsage = fFalse;
            }
            break;

        case 0:
            edumpFCBFilter = dumpfcbfilterAll;
            fValidUsage = fTrue;
            break;
    
        default:
            fValidUsage = fFalse;
            break;
    }
    
    if ( !fValidUsage )
    {
        dprintf( "\nUsage: DUMPFCBS [*|i|a|p|u] [<pinst>]\n\n" );
        dprintf( "        *      - dump all FCBs\n" );
        dprintf( "        i      - dump in-use FCBs\n" );
        dprintf( "        a      - dump available FCBs\n" );
        dprintf( "        p      - dump purgeable FCBs\n" );
        dprintf( "        u      - dump un-purgeable FCBs\n" );
        dprintf( "    <instance> - instance that contains the FCBs.\n" );
        goto HandleError;
    }

    if ( pinstNil == pinst )
    {
        if ( Pdls()->ErrINSTDLSInitCheck() < JET_errSuccess )
        {
            goto HandleError;
        }

        for ( SIZE_T ipinst = 0; ipinst < Pdls()->Cinst(); ipinst++ )
        {
            INST * pinstDebuggee    = Pdls()->PinstDebuggee( ipinst );
        
            if ( pinstNil == pinstDebuggee )
            {
                continue;
            }
            DumpFCBList( pinstDebuggee, edumpFCBFilter );
        }
    }
    else
    {
        DumpFCBList( pinst, edumpFCBFilter );
    }

HandleError:
    dprintf( "\n--------------------\n\n" );
}

//  ================================================================
LOCAL VOID DumpInstFCBs(
    __in INST * const pinstDebuggee
    )
//  ================================================================
{
    FetchWrap<INST *>               pinst;
    TableClassNamesLifetimeManager  tableclassnames;
    FetchWrap<WCHAR *>              wszTableClassNames;
    WCHAR*                          wszTableClassNamesDebuggee  = NULL;
    SIZE_T                          cchTableClassNamesDebuggee  = 0;
    
    dprintf( "\nScanning all FCB's of instance 0x%N...\n", pinstDebuggee );
    if ( !pinst.FVariable( pinstDebuggee ) )
    {
        dprintf( "    Error: Could not read pinst at 0x%N. Aborting.\n", pinstDebuggee );
        return;
    }
    if ( !FReadGlobal( "g_tableclassnames", &tableclassnames ) )
    {
        dprintf( "    Error: Could not read g_tableclassnames. Aborting.\n" );
        return;
    }
    wszTableClassNamesDebuggee = tableclassnames.WszTableClassNames();
    cchTableClassNamesDebuggee = tableclassnames.CchTableClassNames();
    new ( &tableclassnames ) TableClassNamesLifetimeManager;
    if ( !wszTableClassNames.FVariable( wszTableClassNamesDebuggee, cchTableClassNamesDebuggee ) )
    {
        dprintf( "    Error: Could not read tableclassnames.WszTableClassNames() at 0x%N. Aborting.\n", tableclassnames.WszTableClassNames() );
        return;
    }

    ULONG   cFCBs                       = 0;
    ULONG   cFCBsInList                 = 0;
    ULONG   cUnpurgeableFCBs            = 0;
    ULONG   cInUseFCBs                  = 0;
    ULONG   cAvailableFCBs              = 0;
    ULONG   rgcFCBsByTCE[tceMax]        = { 0 };
    FCB *   pfcbDebuggee                = pfcbNil;
    FCB *   pfcbNextInListDebuggee      = pfcbNil;

    // We'll walk across all the FCBs in the list of the INST.
    //
    for ( pfcbDebuggee = pinst->m_pfcbList;
        pfcbNil != pfcbDebuggee;
        pfcbDebuggee = pfcbNextInListDebuggee )
    {
        FetchWrap<FCB *>        pfcb;
        ULONG   cFCBTree = 0;
        BOOL    fUnpurgeableFCB = fFalse;
        BOOL    fAvailableFCB = fTrue;

        cFCBsInList++;

        //  check main FCB
        //
        if ( !pfcb.FVariable( pfcbDebuggee ) )
        {
            dprintf( "    Error: Could not read pfcb at 0x%N. Aborting.\n", pfcbDebuggee );
            return;
        }

        pfcbNextInListDebuggee = pfcb->PfcbNextList();

        CollectFCBTreeInfo(
            pinst,
            pfcb,
            &cFCBTree,
            &fUnpurgeableFCB,
            &fAvailableFCB,
            rgcFCBsByTCE );

        // We will now consider all the tree's
        // nodes as a whole as either available,
        // in-use, purgeable or unpurgeable.
        //
        cFCBs += cFCBTree;

        if ( fAvailableFCB )
        {
            cAvailableFCBs += cFCBTree;
            
            if ( fUnpurgeableFCB )
            {
                cUnpurgeableFCBs += cFCBTree;
            }
        }
        else
        {
            cInUseFCBs += cFCBTree;
        }
    }
    
    EDBGPrintfDmlW( L"      Total Number of FCBs in Instance List:       %10ld\n", cFCBsInList);
    EDBGPrintfDmlW( L"      <link cmd=\"!ese dumpfcbs * 0x%N \">Total Number of FCBs</link>:                        %10ld\n", pinstDebuggee, cFCBs );
    EDBGPrintfDmlW( L"          <link cmd=\"!ese dumpfcbs i 0x%N \">In Use</link>:                                  %10ld\n", pinstDebuggee, cInUseFCBs );
    EDBGPrintfDmlW( L"          <link cmd=\"!ese dumpfcbs a 0x%N \">Available</link>:                               %10ld\n", pinstDebuggee, cAvailableFCBs );
    EDBGPrintfDmlW( L"              <link cmd=\"!ese dumpfcbs p 0x%N \">Purgeable FCBs (estimated*)</link>:         %10ld\n", pinstDebuggee, ( cAvailableFCBs - cUnpurgeableFCBs ));
    EDBGPrintfDmlW( L"              <link cmd=\"!ese dumpfcbs u 0x%N \">Unpurgeable FCBs (estimated*)</link>:       %10ld\n", pinstDebuggee, cUnpurgeableFCBs );
    EDBGPrintfDmlW( L"          TCE Distribution:\n");

    WCHAR* wszTableClassName = wszTableClassNames;
    for ( INT iTce = 0; iTce < tceMax; iTce++ )
    {
        const SIZE_T cwchDefaultTableClassName = 16;
        WCHAR wszDefaultTableClassName[cwchDefaultTableClassName];
        swprintf_s( wszDefaultTableClassName, cwchDefaultTableClassName, L"%d", iTce );

        const SIZE_T cwchTableClassName = wcslen( wszTableClassName );
        if ( !FIsSpace( (TCE)iTce ) && ( cwchTableClassName || rgcFCBsByTCE[iTce] ) )
        {
            EDBGPrintfDmlW( L"              TCE %32.32s:%10ld\n",
                            cwchTableClassName ? wszTableClassName : wszDefaultTableClassName,
                            rgcFCBsByTCE[iTce] );
        }
        wszTableClassName = wszTableClassName + ( cwchTableClassName ? wcslen( wszTableClassName ) + 1 : 0 );
    }
    EDBGPrintfDmlW( L"\n\n");
    EDBGPrintfDmlW( L"      (*estimation only, may differ from actual runtime)\n\n" );
}

//  ================================================================
DEBUG_EXT( EDBGDumpFCBInfo )
//  ================================================================
{
    INST *  pinst                       = NULL;

    if ( 0 != argc
        && ( 1 != argc || !FAutoAddressFromSz( argv[0], &pinst ) ) )
    {
        dprintf( "\nUsage: DUMPFCBINFO [<instance>]\n\n" );
        dprintf( "    <instance> - instance that contains the FCBs.\n" );
        goto HandleError;
    }

    if ( pinstNil == pinst )
    {
        if ( Pdls()->ErrINSTDLSInitCheck() < JET_errSuccess )
        {
            goto HandleError;
        }

        for ( SIZE_T ipinst = 0; ipinst < Pdls()->Cinst(); ipinst++ )
        {
            INST * pinstDebuggee    = Pdls()->PinstDebuggee( ipinst );
        
            if ( pinstNil == pinstDebuggee )
            {
                continue;
            }

            DumpInstFCBs( pinstDebuggee );
        }
    }
    else
    {
        DumpInstFCBs( pinst );
    }

HandleError:
    dprintf( "\n--------------------\n\n" );
}

//  ================================================================
DEBUG_EXT( EDBGTableFind )
//  ================================================================
{
    const CHAR *    szTableName         = NULL;
    INST *          pinstTarget         = NULL;
    BOOL            fValidUsage         = fTrue;

    switch ( argc )
    {
        case 2:
            fValidUsage = ( fValidUsage && FAutoAddressFromSz( argv[1], &pinstTarget ) );
            //  FALL THROUGH to validate rest of args

        case 1:
            szTableName = argv[0];
            break;

        default:
            fValidUsage = fFalse;
    }

    if ( !fValidUsage )
    {
        dprintf( "\nUsage: TABLEFIND <szTable> [<instance>]\n\n" );
        dprintf( "    <szTable> - name of table to search for\n" );
        dprintf( "    <instance> - an optional parameter specifying the pointer\n" );
        dprintf( "                 to the instance for which to limit the scan\n" );
        goto HandleError;
    }

    (VOID) FEDBGTableFind( szTableName, pinstTarget, NULL, ifmpNil );

HandleError:
    dprintf( "\n--------------------\n\n" );
}

// note: do not reimplement as ErrEDBGQueryCache() query, because this
// method is lightning fast and cachequery is slow on big caches.
//  ================================================================
DEBUG_EXT( EDBGCacheMap )
//  ================================================================
{
    ERR                 err                 = JET_errSuccess;

    void *pvOffset;
    if ( argc == 1 && FAddressFromSz( argv[ 0 ], &pvOffset ) )
    {
        //  load BF parameters

        Call( Pdls()->ErrBFInitCacheMap() );

        //  lookup this offset in both tables

        const IBF ibf = Pdls()->IbfBFCachePbf( (PBF)pvOffset, fFalse /* non-exact match */ );

        const IPG ipg = Pdls()->IpgBFCachePv( pvOffset, fFalse /* non-exact match */ );

        //  this is a BF

        if ( ibf != ibfNil )
        {
            PBF     pbfDebuggee = Pdls()->PbfBFCacheIbf( ibf );
            void*   pvDebuggee  = Pdls()->PvBFCacheIpg( ibf );
            bool    fExactMatch = ( pvOffset == pbfDebuggee );

            dprintf(    "0x%p detected as BF 0x%N%s\n",
                        QWORD( pvOffset ),
                        pbfDebuggee,
                        fExactMatch ? "" : " (pointer into BF, not exact)" );
            dprintf(    "\tPage Image is at 0x%N\n",
                        pvDebuggee );
        }

        //  this is a page pointer

        else if ( ipg != ipgNil )
        {
            PBF     pbfDebuggee = Pdls()->PbfBFCacheIbf( ipg );
            void*   pvDebuggee  = Pdls()->PvBFCacheIpg( ipg );
            bool    fExactMatch = ( pvOffset == pvDebuggee );

            dprintf(    "0x%p detected as Page Image 0x%N%s\n",
                        QWORD( pvOffset ),
                        pvDebuggee,
                        fExactMatch ? "" : " (pointer into page image, not exact)" );
            dprintf(    "\tBF is at 0x%N\n",
                        pbfDebuggee );
        }

        //  this is an unknown pointer

        else
        {
            dprintf(    "0x%N is not part of the cache.\n",
                        pvOffset );
        }

        //  unload BF parameters

    }
    else
    {
        dprintf( "Usage: CACHEMAP <address>\n" );
        dprintf( "\n" );
        dprintf( "    <address> can be any address within a valid BF or page image\n" );
    }

HandleError:

    Pdls()->BFTermCacheMap();
    
}

//  ================================================================
LOCAL VOID DumpFUCB(
    __in const FUCB * const         pfucbDebuggee,
    __in const FUCB * const         pfucb
    )
//  ================================================================
{
    EDBGPrintfDmlW(
        L"        <link cmd=\"!ese dump FUCB 0x%N\">0x%N</link> [FCB: <link cmd=\"!ese dump FCB 0x%N\">0x%N</link>, Versioned: %d, levelOpen: %d, levelReuse: %d, fDeferClose:%d, type: %15s]\n",
        pfucbDebuggee,
        pfucbDebuggee,
        pfucb->u.pfcb,
        pfucb->u.pfcb,
        FFUCBVersioned( pfucb ),
        pfucb->levelOpen,
        pfucb->levelReuse,
        FFUCBDeferClosed( pfucb ),
        pfucb->WszFUCBType() );
}

//  ================================================================
LOCAL VOID DumpPIB(
    __in const PIB * const      ppibDebuggee,
    __in const PIB * const      ppib
    )
//  ================================================================
{
    WCHAR wszTrxStack[128];
    wszTrxStack[0] = L'\0';
    ppib->TrxidStack().ErrDump( wszTrxStack, _countof( wszTrxStack ), L" " );
    
    EDBGPrintfDmlW(
        L"    <link cmd=\"dt %ws!PIB 0x%N\">0x%N</link> [User: %s, Level: %d, Cursors: %3d, TrxStack: %40s, Ctxt: <link cmd=\"~~[0x%N]s\">0x%N</link>]\n",
        WszUtilImageName(),
        ppibDebuggee,
        ppibDebuggee,
        ppib->FUserSession() ? L"y" : L"n",
        ppib->Level(),
        ppib->cCursors,
        wszTrxStack,
        ppib->TidActive(),
        ppib->TidActive() );
}

//  ================================================================
LOCAL VOID DumpPIBList(
    __in INST * const   pinstDebuggee
    )
//  ================================================================
{
    FetchWrap<INST *>       pinst;

    dprintf( "\nScanning all database sessions of instance 0x%N...\n", pinstDebuggee );
    if ( !pinst.FVariable( pinstDebuggee ) )
    {
        dprintf( "    Error: Could not read pinst at 0x%N. Aborting.\n", pinstDebuggee );
        return;
    }

    PIB *   ppibDebuggee        = ppibNil;
    PIB *   ppibNextDebuggee    = ppibNil;

    for ( ppibDebuggee = pinst->m_ppibGlobal;
        ppibNil != ppibDebuggee;
        ppibDebuggee = ppibNextDebuggee )
    {
        FetchWrap<PIB *>        ppib;
        FUCB *  pfucbDebuggee           = pfucbNil;
        FUCB *  pfucbNextInListDebuggee = pfucbNil;

        if ( !ppib.FVariable( ppibDebuggee ) )
        {
            dprintf( "	  Error: Could not read pib at 0x%N. Aborting.\n", ppibDebuggee );
            return;
        }

        DumpPIB( ppibDebuggee, ppib );

        // We'll walk across all the FUCBs in the list of the PIB.
        //
        for ( pfucbDebuggee = ppib->pfucbOfSession;
            pfucbNil != pfucbDebuggee;
            pfucbDebuggee = pfucbNextInListDebuggee )
        {
            FetchWrap<FUCB *>       pfucb;
            
            if ( !pfucb.FVariable( pfucbDebuggee ) )
            {
                dprintf( "	  Error: Could not read pfucb at 0x%N. Aborting.\n", pfucbDebuggee );
                return;
            }
        
            pfucbNextInListDebuggee = pfucb->pfucbNextOfSession;
        
            DumpFUCB( pfucbDebuggee, pfucb );
        }

        ppibNextDebuggee = ppib->ppibNext;
    }
}

//  ================================================================
DEBUG_EXT( EDBGDumpPIBs )
//  ================================================================
{
    BOOL fValidUsage                    = fTrue;
    INST *  pinst                       = NULL;

    switch ( argc )
    {
        case 1:
            if ( !FAutoAddressFromSz( argv[0], &pinst ) )
            {
                fValidUsage = fFalse;
            }
            break;

        case 0:
            fValidUsage = fTrue;
            break;
    
        default:
            fValidUsage = fFalse;
            break;
    }
    
    if ( !fValidUsage )
    {
        dprintf( "\nUsage: DUMPPIBS [<instance>]\n\n" );
        dprintf( "	  <instance> - instance that contains the database sessions (PIBs).\n" );
        goto HandleError;
    }

    if ( pinstNil == pinst )
    {
        if ( Pdls()->ErrINSTDLSInitCheck() < JET_errSuccess )
        {
            goto HandleError;
        }

        for ( SIZE_T ipinst = 0; ipinst < Pdls()->Cinst(); ipinst++ )
        {
            INST * pinstDebuggee    = Pdls()->PinstDebuggee( ipinst );
        
            if ( pinstNil == pinstDebuggee )
            {
                continue;
            }

            DumpPIBList( pinstDebuggee );
        }
    }
    else
    {
        DumpPIBList( pinst );
    }

HandleError:
    dprintf( "\n--------------------\n\n" );
}

//  ================================================================
DEBUG_EXT( EDBGDumpCacheMap )
//  ================================================================
{
    //  load BF parameters

    LONG_PTR    cbfChunkT;
    BF **       rgpbfChunkT = NULL;
    LONG_PTR    cpgChunkT;
    VOID **     rgpvChunkT  = NULL;

    if ( !FReadGlobal( "g_cbfChunk", &cbfChunkT )
        || !FReadGlobalAndFetchVariable( "g_rgpbfChunk", &rgpbfChunkT, cCacheChunkMax )
        || !FReadGlobal( "g_cpgChunk", &cpgChunkT )
        || !FReadGlobalAndFetchVariable( "g_rgpvChunk", &rgpvChunkT, cCacheChunkMax ) )
    {
        dprintf( "Error: Could not load BF parameters.\n" );
    }

    else
    {
        dprintf( "BF:\n" );
        for ( LONG_PTR ibfChunk = 0; ibfChunk < cCacheChunkMax && rgpbfChunkT[ ibfChunk ]; ibfChunk++ )
        {
            dprintf(    "\t[0x%p, 0x%p)\n",
                        QWORD( rgpbfChunkT[ ibfChunk ] ),
                        QWORD( rgpbfChunkT[ ibfChunk ] + cbfChunkT ) );
        }
        dprintf( "\n" );

        dprintf( "Page Image:\n" );
        for ( LONG_PTR ipvChunk = 0; ipvChunk < cCacheChunkMax && rgpvChunkT[ ipvChunk ]; ipvChunk++ )
        {
            dprintf(    "\t[0x%p, 0x%p)\n",
                        QWORD( rgpvChunkT[ ipvChunk ] ),
                        QWORD( (BYTE*)rgpvChunkT[ ipvChunk ] + cpgChunkT * CbEDBGILoadSYSMaxPageSize_() ) );
        }
        dprintf( "\n" );
    }

    //  unload BF parameters

    Unfetch( rgpbfChunkT );
    Unfetch( rgpvChunkT );
}

//  Fetches the first _TLS structure from the global head pointer.  Must Unfetch when 
//  done. Optionally gives back the debuggee address of the first _TLS structure if 
//  desired.

_TLS * P_tlsFetchHead( __out_opt _TLS ** pp_tlsDebuggeeHead = NULL );
_TLS * P_tlsFetchHead( __out_opt _TLS ** pp_tlsDebuggeeHead )
{
    _TLS ** pp_tlsGlobalDebuggee = NULL;
    _TLS ** pp_tlsGlobal = NULL;
    _TLS * p_tlsHeadDebuggee = NULL;
    _TLS * p_tlsHead = NULL;

    if ( pp_tlsDebuggeeHead )
    {
        *pp_tlsDebuggeeHead = NULL;
    }
    
    if ( !FAddressFromGlobal( "g_ptlsGlobal", &pp_tlsGlobalDebuggee ) )
    {
        dprintf( "Error: Could not retrieve the symbol ese[nt]!g_ptlsGlobal address.\n" );
        return NULL;
    }

    if ( !FFetchVariable( pp_tlsGlobalDebuggee, &pp_tlsGlobal ) )
    {
        dprintf( "Error: Could not fetch the contents of g_ptlsGlobal variable. (data: %p)\n", pp_tlsGlobalDebuggee );
        return NULL;
    }
    p_tlsHeadDebuggee = *pp_tlsGlobal;
    Unfetch( pp_tlsGlobal );

    if ( !FFetchVariable( p_tlsHeadDebuggee, &p_tlsHead ) )
    {
        dprintf( "Error: Could not fetch the head of TLS / g_ptlsGlobal pointer. (data: %p, %p)\n", pp_tlsGlobalDebuggee, p_tlsHeadDebuggee );
        return NULL;
    }

    if ( pp_tlsDebuggeeHead )
    {
        *pp_tlsDebuggeeHead = p_tlsHeadDebuggee;
    }
    return p_tlsHead;
}

//  Fetches the first _TLS structure associated with the specified TID.  Must Unfetch 
//  when done. Optionally gives back the debuggee address of the first _TLS structure
//  if desired.

_TLS * P_tlsFetch( __in const ULONG ulTid, __out_opt _TLS ** pp_tlsDebuggee = NULL );
_TLS * P_tlsFetch( __in const ULONG ulTid, __out_opt _TLS ** pp_tlsDebuggee )
{
    _TLS * p_tlsDebuggee = NULL;
    _TLS * p_tls = NULL;

    if ( pp_tlsDebuggee )
    {
        *pp_tlsDebuggee = NULL;
    }

    p_tls = P_tlsFetchHead( &p_tlsDebuggee );

    while ( p_tls )
    {

        //  Is this the one we're looking for
        //

        if ( p_tls->dwThreadId == ulTid )
        {
            if ( pp_tlsDebuggee )
            {
                *pp_tlsDebuggee = p_tlsDebuggee;
            }
            return p_tls;
        }

        //  Move to next TLS
        //

        p_tlsDebuggee = p_tls->ptlsNext;
        Unfetch( p_tls );
        p_tls = NULL;
        FFetchVariable( p_tlsDebuggee, &p_tls );
    }

    return NULL;
}

void PrintThreadErr( DWORD ulTid )
{
    _TLS * p_tlsDebuggee = NULL;
    _TLS * p_tls = P_tlsFetch( ulTid, &p_tlsDebuggee );

    OSTLS * postls = NULL;
    const CHAR * szFileLastErr = NULL;

    if ( NULL == p_tls || NULL == p_tlsDebuggee )
    {
        dprintf( "Could not locate a _TLS associated with TID 0x%x, perhaps not a JET thread?\n", ulTid );
        goto HandleError;
    }

    if ( !FFetchVariable( PostlsFromIntTLS( p_tlsDebuggee ), &postls ) || NULL == postls )
    {
        dprintf( "Located _TLS associated with TID 0x%x, but could not fetch the TLS \n", ulTid );
        goto HandleError;
    }

    //  Try to get the file information.
    FFetchSz( postls->m_efLastErr.SzFile(), &szFileLastErr );

    //  print out thread error state
    //

    EDBGPrintfDmlW( L"Located the TLS(<link cmd=\"dt %ws!_TLS 0x%N\">_TLS</link>,<link cmd=\"dt %ws!OSTLS 0x%N\">OSTLS</link>,<link cmd=\"dt %ws!TLS 0x%N\">TLS</link>) for TID 0x%x\n",
        WszUtilImageName(),
        p_tlsDebuggee,
        WszUtilImageName(),
        PostlsFromIntTLS( p_tlsDebuggee ),
        WszUtilImageName(),
        PtlsFromIntTLS( p_tlsDebuggee ),
        ulTid );

    EDBGPrintfDml( "   Last Error Thrown:\n" );
    EDBGPrintfDml( "      File          = %hs\n", szFileLastErr );
    EDBGPrintfDml( "      Line          = %d\n", postls->m_efLastErr.UlLine() );
    EDBGPrintfDml( "      Error         = %d\n", postls->m_efLastErr.Err() );

HandleError:

    Unfetch( szFileLastErr );
    Unfetch( postls );
    Unfetch( p_tls );
}

void PrintAllThreadsErrs()
{
    ULONG cThreads = 0;
    ULONG * rgtid = NULL;

    HRESULT hr = g_DebugSystemObjects->GetNumberThreads( &cThreads );
    if ( FAILED( hr ) )
    {
        dprintf( "Failed to retrieve the number of threads.\n" );
    }

    rgtid = (ULONG*)alloca( sizeof(ULONG) * cThreads );

    hr = g_DebugSystemObjects->GetThreadIdsByIndex( 0, cThreads, NULL, rgtid );
    if ( FAILED( hr ) )
    {
        dprintf( "Failed to retrieve the thread IDs of the process.\n" );
    }

    for ( ULONG iThread = 0; iThread < cThreads; iThread++ )
    {
        if ( rgtid[iThread] )
        {
            PrintThreadErr( rgtid[iThread] );
        }
        dprintf( "\n" );
    }

}

#include "_testinjection.hxx"

CHAR * SzFaultInjectionState( const JET_GRBIT grbit, const QWORD cEvals, const ULONG ulProb, QWORD cHits )
{
    if ( grbit & JET_bitInjectionProbabilitySuppress )
    {
        return cHits ? "fired/off/suppressed" : "off/suppressed";
    }

    if ( grbit & JET_bitInjectionProbabilityPct )
    {
        return ulProb >= 100 ? "on/always   " : "on/sometimes";
    }

    if ( grbit & JET_bitInjectionProbabilityCount )
    {
        if ( grbit & JET_bitInjectionProbabilityPermanent )
        {
            return cEvals >= (QWORD)ulProb ? "on/firing   " : "pending     ";
        }
        else if ( grbit & JET_bitInjectionProbabilityFailUntil )
        {
            return cEvals <= (QWORD)ulProb ? "on/firing   " : "off/fired   ";
        }
        else
        {
            if ( cEvals == (QWORD)ulProb )
                return "on/fired    ";
            else if ( cEvals < (QWORD)ulProb )
                return "pending     ";
            else
                return "off/fired   ";
        }
    }

    return "unknown";
}

void TESTINJECTION::DumpLite( const ULONG iinj )
{
    const BOOL fSuppressed = m_grbit & JET_bitInjectionProbabilitySuppress;

    dprintf( "   [%d] = ", iinj );
    if ( m_grbit & JET_bitInjectionProbabilityPct )
    {
        dprintf( "Probability : ID %u  =  Value = %5d,  Prob = %3u%% - %hs,  Hits / Evals = %I64d / %I64d,  Options = JET_bitInjectionProbabilityPct | %hs0x%x",
            m_ulID,
            (LONGLONG)m_pv,  // note: Using Pv() will affect other member variable state.
            m_ulProb, SzFaultInjectionState( m_grbit, m_cEvals, m_ulProb, m_cHits ),
            m_cHits, m_cEvals,
            fSuppressed ? "_JET_bitInjectionProbabilitySuppress_ | " : "",  // with underscores to emphasize it
            m_grbit & ~( JET_bitInjectionProbabilityPct | JET_bitInjectionProbabilitySuppress ) );
    }
    else if( m_grbit & JET_bitInjectionProbabilityCount )
    {
        const BOOL fProbPermanent = m_grbit & JET_bitInjectionProbabilityPermanent;
        const BOOL fProbFailUntil = m_grbit & JET_bitInjectionProbabilityFailUntil;

        dprintf( "Count Down  : ID %u  =  Value = %5d, Count = %u/%u  - %hs,  Hits / Evals = %I64d / %I64d,  Options = JET_bitInjectionProbabilityCount | %hs%hs%hs0x%x",
            m_ulID,
            (LONGLONG)m_pv,  // note: Using Pv() will affect other member variable state.
            m_cEvals, m_ulProb, SzFaultInjectionState( m_grbit, m_cEvals, m_ulProb, m_cHits ),
            m_cHits, m_cEvals,
            fSuppressed ? "_JET_bitInjectionProbabilitySuppress_ | " : "",  // with underscores to emphasize it
            fProbPermanent ? "JET_bitInjectionProbabilityPermanent | " : "",
            fProbFailUntil ? "JET_bitInjectionProbabilityFailUntil | " : "",
            m_grbit & ~( JET_bitInjectionProbabilityCount | JET_bitInjectionProbabilitySuppress | JET_bitInjectionProbabilityCount | JET_bitInjectionProbabilityPermanent | JET_bitInjectionProbabilityFailUntil ) );
    }
    else
    {
        dprintf( "%d,%I64d,%d,0x%x - %I64d / %I64d  - NYI advanced dump, exercise for the reader.", m_ulID, (LONGLONG)m_pv, m_ulProb, m_grbit, m_cHits, m_cEvals );
    }
    dprintf( "\n" );
}

//  ================================================================
DEBUG_EXT( EDBGThreadErr )
//  ================================================================
{
    const DWORD ulTidAll            = 0xFFFFFFFF;   // signal to print all TIDs
    DWORD       ulTid               = 0;
    BOOL        fValidUsage         = fFalse;

    switch ( argc )
    {
        case 0:
            ulTid = Pdls()->TidCurrent();
            fValidUsage = ( ulTid != 0 );
            break;
        case 1:
            if ( 0 == _stricmp( argv[0], "*" ) )
            {
                fValidUsage = fTrue;
                ulTid = ulTidAll;
            }
            else
            {
                fValidUsage = FUlFromSz( argv[0], &ulTid );
            }
            break;
        default:
            fValidUsage = fFalse;
            break;
    }

    if ( !fValidUsage )
    {
        dprintf( "Usage: THREADERR [<tid>]\n" );
        return;
    }

    if ( ulTidAll == ulTid )
    {
        PrintAllThreadsErrs();
    }
    else if ( ulTid )
    {
        PrintThreadErr( ulTid );
    }
    else
    {
        dprintf( "No TID retrieved, can't print thread error.\n" );
    }
    
#ifdef TEST_INJECTION
    TESTINJECTION rgTestInjections[g_cTestInjectionsMax];
    memset( rgTestInjections, 0, sizeof(rgTestInjections) );
    if ( FReadGlobal( "g_rgTestInjections", rgTestInjections, _countof(rgTestInjections) ) )
    {
        dprintf( "\n\nTest Injections: \n" );
        BOOL fFoundInj = fFalse;
        for ( ULONG iinj = 0; iinj < _countof(rgTestInjections); iinj++ )
        {
            if ( rgTestInjections[iinj].Id() != 0 && rgTestInjections[iinj].Id() != -1 )
            {
                rgTestInjections[iinj].DumpLite( iinj );
                fFoundInj = fTrue;
            }
        }
        if ( !fFoundInj )
        {
            dprintf( "  <empty>\n" );
        }
        dprintf( "\n" );
    }
#endif
}



//  ================================================================
DEBUG_EXT( EDBGTid2PIB )
//  ================================================================
{
    ULONG       ulTid               = 0;
    BOOL        fFoundPIB           = fFalse;
    BOOL        fValidUsage;

    switch ( argc )
    {
        case 0:
            ulTid = Pdls()->TidCurrent();
            fValidUsage = ( ulTid != 0 );
            break;
        case 1:
            fValidUsage = FUlFromSz( argv[0], &ulTid );
            break;
        default:
            fValidUsage = fFalse;
            break;
    }

    if ( !fValidUsage )
    {
        dprintf( "Usage: TID2PIB <tid>\n" );
        return;
    }

    if ( Pdls()->ErrINSTDLSInitCheck() < JET_errSuccess )
    {
        return;
    }

    dprintf( "\nScanning 0x%X INST's ...\n", Pdls()->Cinst() );

    for ( SIZE_T ipinst = 0; ipinst < Pdls()->Cinst(); ipinst++ )
    {
        if ( Pdls()->PinstDebuggee( ipinst ) != pinstNil )
        {
            INST * pinst = Pdls()->Pinst( ipinst );
            if ( pinst == NULL )
            {
                dprintf( "Error: Could not fetch instance definition at 0x%N.\n", Pdls()->PinstDebuggee( ipinst ) );
                return;
            }

            PIB* ppibDebuggee = pinst->m_ppibGlobal;

            while ( ppibDebuggee != ppibNil )
            {
                PIB* ppib;
                if ( !FFetchVariable( ppibDebuggee, &ppib ) )
                {
                    dprintf( "Error: Could not fetch PIB at 0x%N.\n", ppibDebuggee );
                    return;
                }

                // Note this doesn't get the whole TLS, just the OS layer portion, but that is all we need.
                _TLS* ptlsDebuggee = (_TLS*)ppib->ptlsTrxBeginLast;

                if ( ptlsDebuggee )
                {
                    _TLS* ptls;
                    if ( !FFetchVariable( ptlsDebuggee, &ptls ) )
                    {
                        dprintf( "Error: Could not fetch TLS at 0x%N.\n", ptlsDebuggee );
                        Unfetch( ppib );
                        return;
                    }

                    if ( ptls->dwThreadId == ulTid )
                    {
                        dprintf( "TID %d (0x%x) was the last user of PIB 0x%N.\n",
                                    ptls->dwThreadId,
                                    ptls->dwThreadId,
                                    ppibDebuggee );

                        fFoundPIB = fTrue;
                    }

                    Unfetch( ptls );
                }

                ppibDebuggee = ppib->ppibNext;

                Unfetch( ppib );
            }
        }
    }

    if ( !fFoundPIB )
    {
        dprintf( "This thread was not the last user of any PIBs.\n" );
    }
}

//  ================================================================
DEBUG_EXT( EDBGTid2TLS )
//  ================================================================
{
    ULONG       ulTid               = 0;
    BOOL        fValidUsage         = fFalse;

    switch ( argc )
    {
        case 0:
            ulTid = Pdls()->TidCurrent();
            fValidUsage = ( ulTid != 0 );
            break;
        case 1:
            fValidUsage = FUlFromSz( argv[0], &ulTid );
            break;
        default:
            fValidUsage = fFalse;
            break;
    }

    if ( !fValidUsage )
    {
        dprintf( "Usage: TID2TLS [<tid>]\n" );
        return;
    }

    _TLS * p_tlsDebuggee = NULL;
    _TLS * p_tls = P_tlsFetch( ulTid, &p_tlsDebuggee );

    if ( p_tls && p_tlsDebuggee )
    {
        EDBGPrintfDmlW( L"Located the TLS for TID 0x%x\n", ulTid );
        EDBGPrintfDmlW( L"    _TLS *  = <link cmd=\"dt %ws!_TLS 0x%p\">0x%p</link>\n",
            WszUtilImageName(),
            p_tlsDebuggee,
            p_tlsDebuggee );
        
        EDBGPrintfDmlW( L"    OSTLS * = <link cmd=\"dt %ws!OSTLS 0x%p\">0x%p</link>\n",
            WszUtilImageName(),
            PostlsFromIntTLS( p_tlsDebuggee ),
            PostlsFromIntTLS( p_tlsDebuggee ) );

        EDBGPrintfDmlW( L"    TLS *   = <link cmd=\"dt %ws!TLS 0x%p\">0x%p</link>\n",
            WszUtilImageName(),
            PtlsFromIntTLS( p_tlsDebuggee ),
            PtlsFromIntTLS( p_tlsDebuggee ) );
    }
    else
    {
        dprintf( "Could not locate a _TLS associated with TID 0x%x\n", ulTid );
    }

    Unfetch( p_tls );

}

//  ================================================================
DEBUG_EXT( EDBGChecksum )
//  ================================================================
{
    BYTE *  rgbDebuggee;
    PGNO    pgno        = 0;
    ULONG   cb          = 0;
    BYTE *  rgb;

    if (    argc < 1 ||
            argc > 3 ||
            !FAddressFromSz( argv[ 0 ], &rgbDebuggee ) ||
            argc >= 2 && !FUlFromSz( argv[ 1 ], &cb ) ||
            argc >= 3 && !FUlFromSz( argv[ 2 ], &pgno ) )
    {
        dprintf( "Usage: CHECKSUM <address> [<length>] [<pgno>]\n" );
        return;
    }

    cb = ( argc == 1 ? Pdls()->CbPage() : cb );
    dprintf( "Checksumming page / buffer %p for %d bytes ...\n", rgbDebuggee, cb );

    if ( !FFetchVariable( rgbDebuggee, &rgb, cb ) )
    {
        dprintf( "Error: Could not retrieve data to checksum.\n" );
        return;
    }


    PAGECHECKSUM    checksumExpected    = 0;
    PAGECHECKSUM    checksumActual      = 0;
    BOOL            fCorrectableError   = fFalse;
    INT             ibitCorrupted       = -1;

    ChecksumAndPossiblyFixPage(
                rgb,
                cb,
                databaseHeader,
                0,
                fFalse,
                &checksumExpected,
                &checksumActual,
                &fCorrectableError,
                &ibitCorrupted );

    dprintf(    "Old format checksum of 0x%X bytes starting at 0x%N is:  %I64i (0x%016I64x).\n",
                cb,
                rgbDebuggee,
                checksumActual,
                checksumActual );

    ChecksumAndPossiblyFixPage(
                rgb,
                cb,
                databasePage,
                0,
                fFalse,
                &checksumExpected,
                &checksumActual,
                &fCorrectableError,
                &ibitCorrupted );

    dprintf(    "New format checksum of 0x%X bytes starting at 0x%N is:  %I64i (0x%016I64x).\n",
                cb,
                rgbDebuggee,
                checksumActual,
                checksumActual );

    DumpPageChecksumInfo(
                rgb,
                cb,
                databasePage,
                pgno,
                CPRINTFWDBG::PcprintfInstance() );

    Unfetch( rgb );
}

DEBUG_EXT( EDBGCopyFile )
{
    HANDLE  hFileDebuggee = 0;
    QWORD   qwOffsetStart = 0;
    QWORD   qwOffsetStop = 0;
    ULONG64 ulCurrentProcess;
    HANDLE hCurrentProcess;
    HRESULT hr;

    BOOL fConsoleDump = FALSE;

    if ( ( argc != 2 && argc != 4 ) || !FAddressFromSz( argv[ 0 ], (BYTE**)&hFileDebuggee ) ||
        (argc != 2 && (!FAddressFromSz( argv[ 2 ], (BYTE **)&qwOffsetStart ) || !FAddressFromSz( argv[3], (BYTE **)&qwOffsetStop ) ) ) )
    {
        dprintf( "Usage: COPYFILE <handle> <destination> [offsetStart offsetEnd]\n" );
        return;
    }

    if ( qwOffsetStart != 0 && qwOffsetStart >= qwOffsetStop )
    {
        dprintf( "Usage: COPYFILE <handle> <destination> [offsetStart offsetEnd]\n" );
        return;
    }

    const CHAR * const szDestination =  argv[1];

    if ( 0 == strcmp( szDestination, ".") )
    {
        fConsoleDump = TRUE;
    }
    else
    {
        fConsoleDump = FALSE;
    }

    hr = g_DebugSystemObjects->GetCurrentProcessHandle( &ulCurrentProcess );
    hCurrentProcess = (HANDLE) ulCurrentProcess;
    if ( FAILED( hr ) )
    {
        dprintf( "Failed to fetch process handle: %#x\n", hr );
        return;
    }

    dprintf( "Copy file handle 0x%0P of process 0x%0X to file %s (range 0x%0I64X - 0x%0I64X).\n", hFileDebuggee, hCurrentProcess,
        fConsoleDump?"<debug console>":szDestination, qwOffsetStart, qwOffsetStop );

    HANDLE hFileDebugger;

    if (!DuplicateHandle( hCurrentProcess, hFileDebuggee,
          GetCurrentProcess( ), &hFileDebugger,
          GENERIC_READ,
          FALSE, 0x0 ) )
    {
        dprintf( "Error: Could not duplicate handle (error 0x%X (%d)).\n", GetLastError( ), GetLastError( ) );
        return;
    }

    // a "." destination means dump to console
    HANDLE hFileDestination = INVALID_HANDLE_VALUE;

    if ( !fConsoleDump )
    {
        WCHAR wszDestination[ _MAX_PATH ];
        (void)ErrOSSTRAsciiToUnicode( szDestination,
                                      wszDestination,
                                      _countof( wszDestination ) );
        OSStrCbFormatW( wszDestination, sizeof(wszDestination), L"%hs", szDestination );
        hFileDestination = CreateFileW( wszDestination, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

        if (INVALID_HANDLE_VALUE == hFileDestination)
        {
            dprintf( "Error: Could not create file %ws (error 0x%X (%d)).\n", wszDestination, GetLastError( ), GetLastError( ) );
            CloseHandle( hFileDebugger );
            return;
        }
    }




    const DWORD cbAlign = 4 * 1024; // UNDONE: should be disk sector alligned ...
    const DWORD cbBuffer = 16 * cbAlign;

    BYTE * rgBuffer = (BYTE *)VirtualAlloc( NULL, cbBuffer, MEM_COMMIT, PAGE_READWRITE);

    if ( !rgBuffer )
    {
        dprintf( "Error: Could not allocate memory (error 0x%X (%d)).\n", GetLastError( ), GetLastError( ) );
        CloseHandle( hFileDebugger );
        CloseHandle( hFileDestination );
        return;
    }

    DWORD cbRead;

    // we start with the offset which we need but we align
    QWORD qwOffsetRead = rounddn( qwOffsetStart, cbAlign );

    BOOL fKeepReading = TRUE;
    OVERLAPPED overlapped;

    overlapped.hEvent = CreateEventW( NULL, FALSE, TRUE, NULL );
    if ( NULL == overlapped.hEvent )
    {
        dprintf( "Error: Could not create event (error 0x%X (%d)).\n", GetLastError( ), GetLastError( ) );

        CloseHandle( hFileDebugger );
        CloseHandle( hFileDestination );
        VirtualFree( rgBuffer, 0, MEM_RELEASE );
        return;
    }
    //  This is required or the _debuggee_'s completion thread will get a completion packet for
    //  this IO request with our / debugger's virtual addresses as the completion context, which 
    //  will then cause it to (best case) crash when it tries to deref it.
    //  Unknown: How is this handled if the file is opened and not registered for completion port 
    //  completions like most (all?) of our files.
    overlapped.hEvent = HANDLE( (DWORD_PTR)overlapped.hEvent | hNoCPEvent );

    while ( fKeepReading )
    {
        DWORD gle = ERROR_SUCCESS;


        cbRead = 0;
        overlapped.Offset = (DWORD)(qwOffsetRead & ~( DWORD(0) ) );
        overlapped.OffsetHigh = (DWORD)(qwOffsetRead >> 32);

        if ( !ReadFile( hFileDebugger, rgBuffer, cbBuffer, &cbRead, &overlapped ) )
        {
            gle = GetLastError( );
            if ( ERROR_IO_PENDING == gle)
            {
                if ( !GetOverlappedResult( hFileDebugger, &overlapped, &cbRead, TRUE ) )
                {
                    gle = GetLastError( );
                }
                else
                {
                    gle = ERROR_SUCCESS;
                }
            }

            if ( ERROR_HANDLE_EOF == gle )
            {
                // we are ok but we should exit
                // after processing the current data if any
                fKeepReading = FALSE;
            }
            else if ( ERROR_SUCCESS != gle)
            {
                dprintf( "Error: Could not read file (error 0x%X (%d)).\n", gle, gle );
                fKeepReading = FALSE;
            }
        }

        if (cbRead)
        {

            DWORD cbStart;
            DWORD cbStop;

            // the offsets in the buffer which is data that we care
            if (qwOffsetRead < qwOffsetStart)
            {
                cbStart = (DWORD)(qwOffsetStart - qwOffsetRead);
            }
            else
            {
                cbStart = 0;
            }

            if (qwOffsetStop && ( qwOffsetRead + cbRead > qwOffsetStop ) )
            {
                cbStop = (DWORD)( qwOffsetStop - qwOffsetRead );
            }
            else
            {
                cbStop = cbRead;
            }

            if (fConsoleDump)
            {
                // display 16 bytes per row
                const INT cBytesPerRow = 16;

                // text to display the ASCII bytes into
                char szText[cBytesPerRow + 1];
                memset( szText, '\0', sizeof( szText ) );


                for( DWORD iOffset = rounddn( cbStart, cBytesPerRow );
                    iOffset < roundup( cbStop + 1, cBytesPerRow ) ;
                    iOffset++ )
                {

                    if ( 0 == ( iOffset % cBytesPerRow ) )
                    {
                        dprintf("\n0x%08X ", iOffset);
                    }

                    if ( iOffset >= cbStart && iOffset <= cbStop )
                    {
                        dprintf(" %02x", rgBuffer[iOffset] );
                        szText[iOffset % cBytesPerRow] = isprint( rgBuffer[iOffset] ) ? rgBuffer[iOffset]  : '.';
                    }
                    else
                    {
                        dprintf(" %2s", "" );
                        szText[iOffset % cBytesPerRow] = ' ';
                    }

                    if ( ( cBytesPerRow -1 ) == ( iOffset % cBytesPerRow ) )
                    {
                        dprintf("  %s", szText );
                    }
                }

            }
            else
            {
                DWORD cbWrite;

                if (!WriteFile( hFileDestination, rgBuffer + cbStart, cbStop - cbStart, &cbWrite, NULL ) )
                {
                    dprintf( "Error: Could not write file (error 0x%X (%d)).\n", GetLastError( ), GetLastError( ) );
                    fKeepReading = FALSE;
                }

            }

            // update the read pointer
            qwOffsetRead += (QWORD)cbRead;
        }

            // check if we need to stop due to end of needed range (if present)
            if (fKeepReading && qwOffsetStop && qwOffsetRead >= qwOffsetStop)
            {
                fKeepReading = FALSE;
            }
    }

    dprintf("\nTOTAL READ 0x%0I64X\n", qwOffsetRead);

    overlapped.hEvent = HANDLE( (DWORD_PTR)overlapped.hEvent & ~hNoCPEvent );
    CloseHandle( overlapped.hEvent );
    CloseHandle( hFileDebugger );
    CloseHandle( hFileDestination );
    VirtualFree( rgBuffer, 0, MEM_RELEASE );

    return;
}

//  ================================================================
DEBUG_EXT( EDBGDumpLinkedList )
//  ================================================================
{
    ULONG           ulOffset                = 0;
    ULONG           cDwordsToDisplay        = 8;
    ULONG           cbToFetch               = 0;
    ULONG           cMaxElements            = 32;
    ULONG           cElements               = 0;
    const ULONG     cDwordsPerLine          = 4;
    const ULONG     cDwordsToDisplayMax     = 0x1000;
    const ULONG     cMaxElementsAbsolute    = 0x8000;
    BYTE *          pbDebuggee              = NULL;
    BYTE *          rgbBuf                  = NULL;
    BOOL            fValidUsage             = fTrue;

    switch ( argc )
    {
        case 4:
            fValidUsage = ( fValidUsage && FUlFromSz( argv[3], &cMaxElements ) );
            //  FALL THROUGH to validate rest of args

        case 3:
            fValidUsage = ( fValidUsage && FUlFromSz( argv[2], &cDwordsToDisplay ) );
            //  FALL THROUGH to validate rest of args

        case 2:
            fValidUsage = ( fValidUsage
                            && FAddressFromSz( argv[0], &pbDebuggee )
                            && FUlFromSz( argv[1], &ulOffset ) );
            break;

        default:
            fValidUsage = fFalse;
            break;
    }

    dprintf( "\n" );

    if ( !fValidUsage )
    {
        dprintf( "Usage: DUMPLINKEDLIST <address> <offset> [<dwords to display> <max elements>]\n" );
        dprintf( "\n" );
        dprintf( "    <address> - address of initial linked list element\n" );
        dprintf( "    <offset>  - offset (in bytes) of pointer to next linked list element\n" );
        dprintf( "    <dwords to display> - number of dwords to display per element\n" );
        dprintf( "    <max elements> - maximum linked list elements to traverse\n" );
        dprintf( "\n" );
        return;
    }

    //  for each element, we need to ensure we fetch enough
    //  to read the next pointer and to display the requested
    //  number of dwords
    //
    cbToFetch = max( ulOffset + sizeof(VOID *), cDwordsToDisplay * sizeof(DWORD) );
    rgbBuf = (BYTE *)LocalAlloc( 0, cbToFetch );
    if ( NULL == rgbBuf )
    {
        dprintf( "Error: Could not allocate %d-byte buffer for linked list elements.\n", cbToFetch );
        goto HandleError;
    }

    //  cap maximum dwords to display per element
    //  to ensure we don't dump too much
    //
    cDwordsToDisplay = min( cDwordsToDisplay, cDwordsToDisplayMax );

    //  cap maximum elements to traverse to ensure
    //  we don't loop forever
    //
    cMaxElements = min( cMaxElements, cMaxElementsAbsolute );

    //  traverse linked list and dump each element
    //
    while ( NULL != pbDebuggee
        && cElements < cMaxElements )
    {
        if ( !FReadVariable( pbDebuggee, rgbBuf, cbToFetch ) )
        {
            dprintf( "Error: Could not fetch linked list element at 0x%N.\n", pbDebuggee );
            goto HandleError;
        }

        dprintf( "0x%N%s\n", pbDebuggee, ( cDwordsToDisplay > 0 ? ":" : ""  ) );
        for ( ULONG ib = 0; ib < cDwordsToDisplay * sizeof(DWORD); NULL )
        {
            dprintf( "\t" );
            for ( ULONG ibMaxThisLine = min( ib + ( sizeof(DWORD) * cDwordsPerLine ), cDwordsToDisplay * sizeof(DWORD) );
                ib < ibMaxThisLine;
                ib += sizeof(DWORD) )
            {
                dprintf( "%08x ", *(ULONG *)( rgbBuf + ib ) );
            }
            dprintf( "\n" );
        }
        cElements++;

        if ( pbDebuggee == *(BYTE **)( rgbBuf + ulOffset ) )
        {
            //  SPECIAL-CASE: next pointer points back to this element
            //  (we terminate some linked lists in this fashion)
            //
            break;
        }

        pbDebuggee = *(BYTE **)( rgbBuf + ulOffset );
    }

    dprintf( "\nElements traversed: %d\n", cElements );
    if ( NULL != pbDebuggee )
    {
        dprintf( "(End of list was not reached. Next element: 0x%N)\n", pbDebuggee );
    }

HandleError:
    dprintf( "\n" );

    LocalFree( rgbBuf );
}


//  ================================================================
DEBUG_EXT( EDBGDumpInvasiveList )
//  ================================================================
{
    ULONG           ulOffset                = 0;
    ULONG           cDwordsToDisplay        = 8;
    ULONG           cbToFetch               = 0;
    ULONG           cMaxElements            = 32;
    ULONG           cElements               = 0;
    BOOL            fFollowPrevPointer      = fFalse;
    const ULONG     cDwordsPerLine          = 4;
    const ULONG     cDwordsToDisplayMax     = 0x1000;
    const ULONG     cMaxElementsAbsolute    = 0x8000;
    BYTE *          pbDebuggee              = NULL;
    BYTE *          rgbBuf                  = NULL;
    BOOL            fValidUsage             = fTrue;

    switch ( argc )
    {
        case 4:
            fValidUsage = ( fValidUsage && FUlFromSz( argv[3], &cMaxElements ) );
            //  FALL THROUGH to validate rest of args

        case 3:
            fValidUsage = ( fValidUsage && FUlFromSz( argv[2], &cDwordsToDisplay ) );
            //  FALL THROUGH to validate rest of args

        case 2:
            fFollowPrevPointer = ( '-' == argv[1][0] );
            fValidUsage = ( fValidUsage
                            && FAddressFromSz( argv[0], &pbDebuggee )
                            && FUlFromSz( argv[1] + ( fFollowPrevPointer ? 1 : 0 ), &ulOffset ) );
            break;

        default:
            fValidUsage = fFalse;
            break;
    }

    dprintf( "\n" );

    if ( !fValidUsage )
    {
        dprintf( "Usage: DUMPINVASIVELIST <address> [<+/->]<offset> [<dwords to display> <max elements>]\n" );
        dprintf( "\n" );
        dprintf( "    <address> - address of initial invasive list element (NOT address of invasive list head)\n" );
        dprintf( "    <+/-> - specify '+' to follow \"next\" pointer, '-' to follow \"prev\" pointer\n" );
        dprintf( "    <offset>  - offset (in bytes) of invasive list within the element\n" );
        dprintf( "    <dwords to display> - number of dwords to display per element\n" );
        dprintf( "    <max elements> - maximum invasive list elements to traverse\n" );
        dprintf( "\n" );
        return;
    }

    //  for each element, we need to ensure we fetch enough
    //  to read the next pointer and to display the requested
    //  number of dwords
    //
    cbToFetch = max( ulOffset + sizeof(CInvasiveListEDBG::CElement), cDwordsToDisplay * sizeof(DWORD) );
    rgbBuf = (BYTE *)LocalAlloc( 0, cbToFetch );
    if ( NULL == rgbBuf )
    {
        dprintf( "Error: Could not allocate %d-byte buffer for linked list elements.\n", cbToFetch );
        goto HandleError;
    }

    //  cap maximum dwords to display per element
    //  to ensure we don't dump too much
    //
    cDwordsToDisplay = min( cDwordsToDisplay, cDwordsToDisplayMax );

    //  cap maximum elements to traverse to ensure
    //  we don't loop forever
    //
    cMaxElements = min( cMaxElements, cMaxElementsAbsolute );

    //  traverse linked list and dump each element
    //
    while ( NULL != pbDebuggee
        && cElements < cMaxElements )
    {
        if ( !FReadVariable( pbDebuggee, rgbBuf, cbToFetch ) )
        {
            dprintf( "Error: Could not fetch invasive list element at 0x%N.\n", pbDebuggee );
            goto HandleError;
        }

        dprintf( "0x%N%s\n", pbDebuggee, ( cDwordsToDisplay > 0 ? ":" : ""  ) );
        for ( ULONG ib = 0; ib < cDwordsToDisplay * sizeof(DWORD); NULL )
        {
            dprintf( "\t" );
            for ( ULONG ibMaxThisLine = min( ib + ( sizeof(DWORD) * cDwordsPerLine ), cDwordsToDisplay * sizeof(DWORD) );
                ib < ibMaxThisLine;
                ib += sizeof(DWORD) )
            {
                dprintf( "%08x ", *(ULONG *)( rgbBuf + ib ) );
            }
            dprintf( "\n" );
        }
        cElements++;

        pbDebuggee = *(BYTE **)(
                            rgbBuf
                            + ulOffset
                            + ( fFollowPrevPointer ?
                                    0 :                 //  OffsetOf( CInvasiveList::CElement, m_pilePrev )
                                    sizeof(VOID *) ) ); //  OffsetOf( CInvasiveList::CElement, m_pileNext )

        if ( NULL == pbDebuggee             //  should be impossible, but just in case
            || (VOID *)-1 == pbDebuggee )   //  this element is not actually in an invasive list
        {
            break;
        }

        pbDebuggee -= ulOffset;
    }

    dprintf( "\nElements traversed: %d\n", cElements );
    if ( NULL != pbDebuggee )
    {
        dprintf( "(End of list was not reached. Next element: 0x%N)\n", pbDebuggee );
    }

HandleError:
    dprintf( "\n" );

    LocalFree( rgbBuf );
}


//  ================================================================
DEBUG_EXT( EDBGDumpLR )
//  ================================================================
{
    LR *    plrDebuggee     = NULL;
    BOOL    fValidUsage     = fTrue;
    ULONG   cMaxRecs        = 1;
    BOOL    fSkipNewSegmentHeader = fTrue;

    switch ( argc )
    {
        case 3:
            fSkipNewSegmentHeader = (_stricmp( argv[2], "old" ) != 0 );
            __fallthrough;

        case 2:
            fValidUsage = ( fValidUsage && FUlFromSz( argv[1], &cMaxRecs ) );
            __fallthrough;

        case 1:
            fValidUsage = ( fValidUsage && FAddressFromSz( argv[0], &plrDebuggee ) );
            break;

        default:
            fValidUsage = fFalse;
            break;
    }


    if ( !fValidUsage )
    {
        dprintf( "Usage: DUMPLR <address> [<max recs>]\n" );
        return;
    }

    for ( ULONG crecs = 0; crecs < cMaxRecs; crecs++ )
    {
        // skip over segment header
        if ( fSkipNewSegmentHeader &&
             (ULONG_PTR)plrDebuggee % LOG_SEGMENT_SIZE == 0 )
        {
            LGSEGHDR *pSegHdr;
            if ( FFetchVariable( (LGSEGHDR *)plrDebuggee, &pSegHdr ) )
            {
                dprintf( "Segment at 0x%I64X Checksum: 0x%016I64x, LGPOS (%08lX,%04hX,%04hX), logtimeStart (%hu/%hu/%hu:%hu:%hu:%hu.%3.3hu)\n",
                         (__int64)plrDebuggee,
                         (XECHECKSUM)pSegHdr->checksum,
                         (INT)pSegHdr->le_lgposSegment.le_lGeneration,
                         (SHORT)pSegHdr->le_lgposSegment.le_isec,
                         (SHORT)pSegHdr->le_lgposSegment.le_ib,
                         (USHORT)( 1900 + pSegHdr->logtimeSegmentStart.bYear ),
                         (USHORT)pSegHdr->logtimeSegmentStart.bMonth,
                         (USHORT)pSegHdr->logtimeSegmentStart.bDay,
                         (USHORT)pSegHdr->logtimeSegmentStart.bHours,
                         (USHORT)pSegHdr->logtimeSegmentStart.bMinutes,
                         (USHORT)pSegHdr->logtimeSegmentStart.bSeconds,
                         pSegHdr->logtimeSegmentStart.Milliseconds() );
                Unfetch( pSegHdr );
            }

            plrDebuggee += sizeof( LGSEGHDR );
        }

        //  get the fixed size of this log record

        LR lr( sizeof( LR ) );
        if ( !FReadVariable( plrDebuggee, &lr ) )
        {
            dprintf( "Error: Could not fetch the log record type.\n" );
            return;
        }
        const SIZE_T    cbLRFixed   = CbLGFixedSizeOfRec( &lr );

        //  get the full size of this log record

        LR* plrFixed;
        if ( !FFetchVariable( (BYTE*)plrDebuggee, (BYTE**)&plrFixed, cbLRFixed ) )
        {
            dprintf( "Error: Could not fetch the fixed-sized part of this log record.\n" );
            return;
        }
        const SIZE_T    cbLR        = CbLGSizeOfRec( plrFixed );

        Unfetch( (BYTE*)plrFixed );

        //  get the full log record

        LR* plr;
        if ( !FFetchVariable( (BYTE*)plrDebuggee, (BYTE**)&plr, cbLR ) )
        {
            dprintf( "Error: Could not fetch the entire log record.\n" );
            return;
        }

        if ( lrtypNOP == plr->lrtyp )
        {
            //  special handling for NOP's to collapse them all into one
            //
            SIZE_T  cNOP;
            for ( cNOP = 1;
                  ( (ULONG_PTR)plrDebuggee + cNOP ) % LOG_SEGMENT_SIZE != 0 &&
                   FReadVariable( plrDebuggee + cNOP, &lr ) &&
                   lrtypNOP == lr.lrtyp;
                  cNOP++ )
            {
                NULL;
            }

            dprintf( "0x%N bytes @ 0x%N\n", cNOP, plrDebuggee );
            dprintf( " %s (Total: %I64d)\n", SzLrtyp( lrtypNOP ), QWORD( cNOP ) );

            plrDebuggee += cNOP;
        }
        else
        {
            //  dump the log record

            dprintf( "0x%N bytes @ 0x%N\n", cbLR, plrDebuggee );

            // every other place to use LOG_REC_STRING_MAX, which is only 1024 right now.
            CHAR        szBuf[ 2048 ];
            LrToSz( plr, szBuf, sizeof(szBuf), NULL );
            dprintf( "%s\n", szBuf );

            //  special-case: for ReplaceD, report
            //  individual diffs
            //
            if ( lrtypReplaceD == plr->lrtyp )
            {
                LGDumpDiff( NULL, plr, CPRINTFWDBG::PcprintfInstance(), 1 );
            }

            plrDebuggee += cbLR;
        }

        Unfetch( (BYTE*)plr );
    }
}


//  ================================================================
DEBUG_EXT( EDBGDumpRESPointer )
//  ================================================================
{
    VOID *pv;
    FetchWrap<CResourceSection *>   pRS;
    FetchWrap<CResourceChunkInfo *> pRCI;
    FetchWrap<CResourceManager *>   pRM;
    DWORD_PTR                       cbChunkSize = 0;
    DWORD_PTR                       cbObjectSize = 0;
    DWORD_PTR                       cbAlign = 0;
    DWORD_PTR                       cObjectsPerChunk = 0;
    DWORD_PTR                       cbAlignedObject = 0;
    DWORD_PTR                       cSectionObjectIndex = 0;
    ERR                             err = JET_errSuccess;
    CHAR                            m_szTag[JET_resTagSize+1];
    INT                             i;

    if ( 1 != argc || !FAddressFromSz( argv[ 0 ], &pv ) )
    {
        dprintf( "usage: RESOBJECT <address>\n" );
        return;
    }

    if ( NULL == pv || 0 == ( (DWORD_PTR)pv & maskSection ) )
    {
        dprintf( "NULL is invalid parameter\n" );
        return;
    }

    dprintf( "Pointer: %N\n", pv );
    FCall( pRS.FVariable( (CResourceSection *)( (DWORD_PTR)pv & maskSection ) ),
        ( "Cannot find section %N\n", (DWORD_PTR)pv & maskSection ) );
    dprintf( "ResourceSection: %N\n", (CResourceSection *)( (DWORD_PTR)pv & maskSection ) );
    if ( NULL == pRS->m_pRCI )
    {
        dprintf( "pRCI == NULL" );
        goto HandleError;
    }
    dprintf( "Resource Chunk Info: %N\n", pRS->m_pRCI );
    FCall( pRCI.FVariable( pRS->m_pRCI ),
        ( "Cannot fetch the pRCI %N\n", pRS->m_pRCI ) );
    if ( NULL == pRCI->m_pRMOwner )
    {
        dprintf( "NULL == pRM\n" );
        goto HandleError;
    }

    FCall( pRM.FVariable( pRCI->m_pRMOwner ),
        ( "Cannot fetch pRM %N\n", pRCI->m_pRMOwner ) );
    dprintf( "ResourceManager %N\n", pRCI->m_pRMOwner );
    Call( pRM->ErrGetParam( JET_resoperChunkSize, &cbChunkSize ) );
    Call( pRM->ErrGetParam( JET_resoperTag, (DWORD_PTR *)m_szTag ) );
    if ( pRCI->m_pvData >= pv || (CHAR *)pRCI->m_pvData + cbChunkSize <= (CHAR *)pv )
    {
        dprintf( "The pointer is outside of the chunk area\n" );
        goto HandleError;
    }
    if ( memcmp( pRS->m_rgchTag, m_szTag, JET_resTagSize ) != 0 )
    {
        dprintf( "Tag mismatch %08X %08X\n", *(DWORD *)pRS->m_rgchTag, *(DWORD *)m_szTag );
        goto HandleError;
    }
    Call( pRM->ErrGetParam( JET_resoperObjectsPerChunk, &cObjectsPerChunk ) );
    cObjectsPerChunk /= cbChunkSize/cbSectionSize;
    Call( pRM->ErrGetParam( JET_resoperAlign, &cbAlign ) );
    Call( pRM->ErrGetParam( JET_resoperSize, &cbObjectSize ) );
    cbAlignedObject = DWORD_PTR( cbObjectSize + cbAlign - 1 ) & DWORD_PTR( 0 - cbAlign );
    cSectionObjectIndex = ( (DWORD_PTR)pv - ((DWORD_PTR)pv & maskSection) - pRS->m_cbSectionHeader ) / cbAlignedObject;
    if ( (DWORD_PTR)pRS->m_cbSectionHeader > (DWORD_PTR)pv - ( (DWORD_PTR)pv & maskSection )
        || cSectionObjectIndex > cObjectsPerChunk )
    {
        dprintf( "The pointer is in the junk area of the section\n" );
        goto HandleError;
    }
    dprintf( "RESID: %i TAG: ", pRM->ResID() );
    for ( i = 0; i < JET_resTagSize; i++ )
    {
        if ( ( 0x20 > m_szTag[i] && 0 != m_szTag[i] ) || 0x7f < m_szTag[i] )
        {
            m_szTag[i] = '.';
        }
    }
    m_szTag[JET_resTagSize] = 0;
    dprintf( "\"%s\" <0x%08X>\n", m_szTag, *(DWORD *)pRS->m_rgchTag );
    if ( ( ((DWORD_PTR)pv & maskSection) + pRS->m_cbSectionHeader + cSectionObjectIndex*cbAlignedObject ) != (DWORD_PTR)pv )
    {
        dprintf( "This pointer is inside of the object, not at the begining.\nObject address is: %N\n",
            ( ((DWORD_PTR)pv & maskSection) + pRS->m_cbSectionHeader + cSectionObjectIndex*cbAlignedObject ) );
    }
    dprintf( "*** Cannot retrieve object allocation information\n" );
    return;

HandleError:
    dprintf( "The pointer is not part of any Resource manager\n" );
    return;
}

void FindRM( ULONG resid )
{
    CResourceManager*   prmDebuggee = NULL;
    prmDebuggee = CRMContainer::EDBGPRMFind( (JET_RESID)resid );
    if ( !prmDebuggee )
    {
        dprintf( "The ResID is invalid\n" );
        return;
    }
    
    EDBGPrintfDml(  "%s %i: <link cmd=\"dt CResourceManager 0x%N\">0x%N</link> \n",
                    "CResourceManager",
                    resid,
                    prmDebuggee,
                    prmDebuggee );
}

//  ================================================================
DEBUG_EXT( EDBGDumpRMResID )
//  ================================================================
{
    dprintf( "\n" );
    
    //  validate arg
    if ( 1 == argc )
    {
        ULONG resid = (ULONG) GetExpression( argv[0] );
        FindRM( resid );
        return;
    }
    else if ( 0 == argc )
    {
            for ( ULONG resid = 1; resid < JET_residMax; resid++ )
        {
            FindRM( resid );
        }
        return;
    }
    else
    {
        dprintf( "Usage: RMResID [<ResID>]\n" );
        return;
    }
}
 
/*CResourceChunkInfo.m_pvData
    ------------------------------------------------------------
    |m_cbSectionHeader|cbAlignedObject|cbAlignedObject|.........|
    -------------------------------------------------------------
*/
//  ================================================================
DEBUG_EXT( EDBGDumpRCIOutStandingResObjs )
//  ================================================================
{
        CResourceChunkInfo              *pv = NULL;
        FetchWrap<CResourceChunkInfo *> pRCI;
        FetchWrap<CResourceSection *>   pRS;
        FetchWrap<CResourceManager *>   pRM;
        CResourceFreeObjectList*        pRFO = NULL;
        CResourceFreeObjectList         curRFO;
        CResourceFreeObjectList*        pCurRFO = &curRFO;
        BYTE *                          rgfOutstanding = NULL;
        DWORD_PTR                       cbChunkSize = 0;
        DWORD_PTR                       cbObjectSize = 0;
        DWORD_PTR                       cbRFOLOffset = 0;
        DWORD_PTR                       cbAlign = 0;
        DWORD_PTR                       cObjectsPerChunk = 0;
        DWORD_PTR                       cbAlignedObject = 0;
        DWORD_PTR                       cSectionObjectIndex = 0;
        DWORD_PTR                       cbSectionHeader = 0;
        CHAR                            m_szTag[ JET_resTagSize + 1 ];
        ERR                             err = JET_errSuccess;

        if ( 1 != argc || !FAddressFromSz( argv[ 0 ], &pv ) )
        {
            dprintf( "usage: DUMPRCIOUTSTANDINGRESOBJECTS <RCIAddress>\n" );
            return;
        }

        if ( NULL == pv )
        {
            dprintf( "pv is invalid pointer to CResourceChunkInfo\n" );
            return;
        }
        
        FCall( pRCI.FVariable( pv ),
            ( "Cannot find CResourceChunkInfo %N\n", pv ) );

        if ( NULL == pRCI->m_pRMOwner )
        {
            dprintf( "NULL == pRCI->m_pRMOwner: the pointer is not part of any Resource manager\n" );
            return;
        }

        if ( pRCI->m_cUsed <= 0 )
        {
            dprintf( "All resource objects are freed\n" );
            return;
        }
    
        FCall( pRS.FVariable( (CResourceSection *)( (DWORD_PTR)pRCI->m_pvData ) ),
            ( "Cannot find section %N\n", (DWORD_PTR)pRCI->m_pvData ) );

        FCall( pRM.FVariable( pRCI->m_pRMOwner ),
            ( "Cannot fetch pRM %N\n", pRCI->m_pRMOwner ) );

        Call( pRM->ErrGetParam( JET_resoperObjectsPerChunk, &cObjectsPerChunk ) );
        Call( pRM->ErrGetParam( JET_resoperChunkSize, &cbChunkSize ) );
        Call( pRM->ErrGetParam( JET_resoperRFOLOffset, &cbRFOLOffset ) );
        Call( pRM->ErrGetParam( JET_resoperAlign, &cbAlign ) );
        Call( pRM->ErrGetParam( JET_resoperSize, &cbObjectSize ) );
        Call( pRM->ErrGetParam( JET_resoperSectionHeader, &cbSectionHeader ) );
        memset( m_szTag, 0, sizeof( m_szTag ) );
        Call( pRM->ErrGetParam( JET_resoperTag, (DWORD_PTR *)m_szTag ) );
        cbAlignedObject = DWORD_PTR( cbObjectSize + cbAlign - 1 ) & DWORD_PTR( 0 - cbAlign );
        dprintf( "cbAlignedObject: %i\n", cbAlignedObject );
        dprintf( "cbSectionHeader: %i\n", cbSectionHeader );
        dprintf( "cbRFOLOffset: %i\n", cbRFOLOffset );
        dprintf( "pvData: <0x%016X>\n", pRCI->m_pvData );
        
#ifdef RM_DEFERRED_FREE
        pRFO = pRCI->m_pRFOLHead;
#else // RM_DEFERRED_FREE
        pRFO = pRCI->m_pRFOL;
#endif // !RM_DEFERRED_FREE

        rgfOutstanding = (BYTE *)LocalAlloc( LMEM_ZEROINIT, cObjectsPerChunk );


        //Iterate the CResourceFreeObjectList to mark freed objects in the bitMap       
        INT i = 0;
        while( pRFO!= NULL )
        {
            cSectionObjectIndex = ( (DWORD_PTR)pRFO - ((DWORD_PTR)pRFO & maskSection) - cbSectionHeader - cbRFOLOffset ) / cbAlignedObject;
            if ( cSectionObjectIndex >= cObjectsPerChunk )
            {
                dprintf( "index out of bitMap range\n" );
                goto HandleError;
            }
            rgfOutstanding[cSectionObjectIndex] = fTrue;
            i++;
            if ( !FReadVariable( pRFO, pCurRFO ) )
            {
                dprintf( "Error: Could not fetch the next RFO at 0x%N.\n",  pRFO->m_pRFOLNext );
                goto HandleError;
            }
            pRFO = pCurRFO->m_pRFOLNext;
        }
        for ( ULONG j = 0; j < cObjectsPerChunk; j++ )
        {
            if ( rgfOutstanding[j] != fTrue )
            {
                pv = (CResourceChunkInfo *)( j * cbAlignedObject + cbSectionHeader + (DWORD_PTR) pRCI->m_pvData );
                dprintf( "RESID: %i ", pRM->ResID() );
                dprintf( "Index: %i ", j );
                EDBGPrintfDml( "%s: <link cmd=\"?? (%s *) 0x%N\">0x%N</link> not free\n",
                    m_szTag,
                    m_szTag,
                    pv,
                    pv );
            }
        }
HandleError:
        LocalFree( rgfOutstanding );
        return;
}

//  ================================================================
DEBUG_EXT( EDBGDumpRMOutStandingResObjs )
//  ================================================================
{
        CResourceManager *              pv = NULL;
        FetchWrap<CResourceManager *>   pRM;
        CResourceChunkInfo*             pCurRCI = NULL;
        CResourceChunkInfo*             pRCI = NULL;
        ERR                             err = JET_errSuccess;
        
        if ( 1 != argc || !FAddressFromSz( argv[ 0 ], &pv ) )
        {
            dprintf( "usage: DUMPRMOUTSTANDINGRESOBJECTS <RMAddress>\n" );
            return;
        }
        
        if ( NULL == pv )
        {
            dprintf( "pv is invalid pointer to CResourceManager\n" );
            return;
        }
        FCall( pRM.FVariable( pv ),
                    ( "Cannot find CResourceManager %N\n", pv ) );

        Call( pRM->ErrGetParam( JET_resoperRCIList, (DWORD_PTR *)&pRCI ) );

        pCurRCI = (CResourceChunkInfo *)LocalAlloc( LMEM_ZEROINIT, sizeof(CResourceChunkInfo) );
        while (pRCI != NULL)
        {
            if ( !FReadVariable( pRCI, pCurRCI ) )
            {
                dprintf( "Error: Could not fetch the next pRCI at 0x%N.\n",  pRCI );
                goto HandleError;
            }
            if (pCurRCI->m_cUsed > 0)
            {
                EDBGPrintfDml( "%s: <link cmd=\"!ese DUMPRCIOUTSTANDINGRESOBJECTS 0x%N\">0x%N</link> \n",
                    "CResourceChunkInfo",
                    pRCI,
                    pRCI );
            }
            pRCI = pCurRCI->m_pRCINext;
        }
        HandleError:
            LocalFree( pCurRCI );
            return;
}

//  ================================================================
DEBUG_EXT( EDBGHash )
//  ================================================================
{
    ULONG   ifmp;
    ULONG   pgnoFDP;
    BYTE*   rgbPrefixDebuggee;
    ULONG   cbPrefix;
    BYTE*   rgbSuffixDebuggee;
    ULONG   cbSuffix;
    BYTE*   rgbDataDebuggee;
    ULONG   cbData;

    dprintf( "\n" );

    if (    argc != 8 ||
            !FAutoIfmpFromSz( argv[ 0 ], &ifmp ) ||
            !FUlFromSz( argv[ 1 ], &pgnoFDP ) ||
            !FAddressFromSz( argv[ 2 ], &rgbPrefixDebuggee ) ||
            !FUlFromSz( argv[ 3 ], &cbPrefix ) ||
            !FAddressFromSz( argv[ 4 ], &rgbSuffixDebuggee ) ||
            !FUlFromSz( argv[ 5 ], &cbSuffix ) ||
            !FAddressFromSz( argv[ 6 ], &rgbDataDebuggee ) ||
            !FUlFromSz( argv[ 7 ], &cbData ) )
    {
        dprintf( "Usage: HASH <ifmp|.> <pgnoFDP> <pbPrefix> <cbPrefix> <pbSuffix> <cbSuffix> <pbData> <cbData>\n\n" );
        return;
    }

    FMP*    rgfmpDebuggee   = NULL;
    FMP*    pfmp            = NULL;
    INST*   pinst           = NULL;
    VER*    pver            = NULL;
    BYTE*   rgbPrefix       = NULL;
    BYTE*   rgbSuffix       = NULL;
    BYTE*   rgbData         = NULL;
    RCE*    prce            = NULL;
    BYTE    rgbRceBuffer[ sizeof( RCE ) ];

    if ( !FReadGlobal( "g_rgfmp", &rgfmpDebuggee )
        || NULL == rgfmpDebuggee
        || !FFetchVariable( rgfmpDebuggee + ifmp, &pfmp )
        || !FFetchVariable( pfmp->Pinst(), &pinst )
        || !FFetchVariable( pinst->m_pver, &pver ) )
    {
        dprintf( "Error: Couldn't fetch the hash table size from the debuggee.\n" );
    }
    else if ( !FFetchVariable( rgbPrefixDebuggee, &rgbPrefix, cbPrefix )
            || !FFetchVariable( rgbSuffixDebuggee, &rgbSuffix, cbSuffix )
            || !FFetchVariable( rgbDataDebuggee, &rgbData, cbData ) )
    {
        dprintf( "Error: Couldn't fetch the prefix / suffix / data from the debuggee.\n" );
    }
    else
    {
        BOOKMARK bookmark;
        bookmark.key.prefix.SetPv( rgbPrefix );
        bookmark.key.prefix.SetCb( cbPrefix );
        bookmark.key.suffix.SetPv( rgbSuffix );
        bookmark.key.suffix.SetCb( cbSuffix );
        bookmark.data.SetPv( rgbData );
        bookmark.data.SetCb( cbData );

        //  compute the hash value
        //
        size_t crcehead = pver->m_crceheadHashTable;
        const ULONG ulVERChecksum = UiVERHash( IFMP( ifmp ), PGNO( pgnoFDP ), bookmark, crcehead );
        dprintf( "VER checksum is: %u (0x%08X)\n", ulVERChecksum, ulVERChecksum );

        //  reload the VER structure, but this time include the hash table dangling off the end
        //
        Unfetch( pver );
        pver = NULL;
        if ( FFetchVariable( (BYTE *)( pinst->m_pver ), (BYTE **)&pver, sizeof(VER) + ( VER::cbrcehead * crcehead ) ) )
        {
            //  find the RCE chain corresponding to the hash value
            //
            RCE * prceHead  = pver->GetChain( ulVERChecksum );
            dprintf( "Head of RCE hash chain: 0x%p\n", prceHead );

            //  try to find a matching RCE
            //
            RCE * prceDebuggee  = prceHead;
            while ( prceNil != prceDebuggee )
            {
                //  first read in the RCE without any of the trailing data
                //
                if ( !FReadVariable( prceDebuggee, (RCE *)rgbRceBuffer ) )
                {
                    dprintf( "Error: Couldn't read RCE at 0x%p from the debuggee.\n", prceDebuggee );
                    break;
                }

                //  retrieve the full size of the RCE
                //
                const INT cbFullRCE = ( (RCE *)rgbRceBuffer )->CbRceEDBG();

                //  now that we can tell how much data follows the RCE,
                //  fetch the full RCE
                //
                if ( !FFetchVariable( (BYTE *)prceDebuggee, (BYTE **)&prce, cbFullRCE ) )
                {
                    dprintf( "Error: Couldn't fetch %d-byte RCE at 0x%p from the debuggee.\n", cbFullRCE, prceDebuggee );
                    break;
                }

                //  see if this RCE matches
                //
                if ( prce->FRCECorrectEDBG( IFMP( ifmp ), PGNO( pgnoFDP ), bookmark ) )
                {
                    dprintf( "Matching RCE: 0x%p\n", prceDebuggee );
                    break;
                }

                //  not a match, so continue to the next entry in the overflow chain
                //
                prceDebuggee = prce->PrceHashOverflowEDBG();

                //  free the current RCE
                //
                Unfetch( prce );
                prce = NULL;
            }

            if ( prceNil == prceDebuggee )
            {
                dprintf( "No matching RCE found.\n" );
            }
        }
        else
        {
            dprintf( "Couldn't fetch VER with %d-element hash table at 0x%p from the debuggee.\n", crcehead, pinst->m_pver );
        }
    }

    dprintf( "\n" );

    Unfetch( prce );
    Unfetch( rgbPrefix );
    Unfetch( rgbSuffix );
    Unfetch( rgbData );
    Unfetch( pver );
    Unfetch( pinst );
    Unfetch( pfmp );
}


LOCAL BOOL FEDBGLoadPage_(
    const IFMP  ifmp,
    const PGNO  pgno,
    BYTE *      rgbPageBuf,
    PBF         pbfBuf,
    ULONG *     pcbBuffer,
    const ULONG cbPage )
//  ================================================================
{
    //  scan all valid BFs looking for this IFMP / PGNO
    //
    for ( LONG_PTR ibf = 0; ibf < cbfCacheAddressable; ibf++ )
    {
        //  compute the address of the target BF
        //
        PBF pbfDebuggee = g_rgpbfChunk[ ibf / g_cbfChunk ] + ibf % g_cbfChunk;

        //  we failed to read this BF
        //
        if ( !FReadVariable( pbfDebuggee, pbfBuf ) )
        {
            dprintf( "Error: Could not read BF at 0x%N.\n", pbfDebuggee );
            return fFalse;
        }

        //  see if this BF contains this IFMP / PGNO
        //
        if ( pbfBuf->ifmp == IFMP( ifmp )
            && pbfBuf->pgno == PGNO( pgno )
            && pbfBuf->fCurrentVersion )
        {
            if ( FReadVariable( (BYTE *)pbfBuf->pv, rgbPageBuf, cbPage ) )
            {
                *pcbBuffer = ( pbfBuf->icbBuffer < icbPageMax ? g_rgcbPageSize[pbfBuf->icbBuffer] : cbPage );
                return fTrue;
            }
            else
            {
                dprintf( "Error: Could not read %d-byte PAGE at 0x%N.\n", cbPage, pbfBuf->pv );
                return fFalse;
            }
        }
    }

    dprintf( "Error: [%d:%d] is not cached.\n", ifmp, pgno );
    return fFalse;
}

//  ================================================================
LOCAL VOID EDBGSeek_(
    const IFMP  ifmp,
    const PGNO  pgnoRoot,
    BOOKMARK&   bm,
    const ULONG cbPage )
//  ================================================================
{
    CPAGE       cpage;
    INT         compare;
    INT         iline;
    BYTE *      rgbPageBuf;
    PBF         pbfBuf;
    ULONG       cbBuffer    = 0;
    PGNO        pgnoCurr    = pgnoRoot;

    rgbPageBuf = (BYTE *)LocalAlloc( 0, cbPage );
    if ( NULL == rgbPageBuf )
    {
        dprintf( "Error: Could not allocate PAGE buffer.\n" );
        return;
    }

    pbfBuf = (PBF)LocalAlloc( 0, sizeof(BF) );
    if ( NULL == pbfBuf )
    {
        dprintf( "Error: Could not allocate BF buffer.\n" );
        LocalFree( rgbPageBuf );
        return;
    }

    dprintf( "Path to bookmark:\n" );

    if ( !FEDBGLoadPage_( ifmp, pgnoRoot, rgbPageBuf, pbfBuf, &cbBuffer, cbPage ) )
        goto HandleError;

    cpage.LoadDehydratedPage( ifmp, pgnoRoot, rgbPageBuf, cbBuffer, cbPage );

    //  now seek down the btree for the bookmark
    //
    while ( !cpage.FLeafPage() )
    {
        iline = IlineNDISeekGEQInternal( cpage, bm, &compare );

        if ( 0 == compare )
        {
            //  see ErrNDISeekInternalPage() for an
            //  explanation of why we increment by 1 here
            //
            iline++;
        }

        dprintf( "\t[%d:%d:%d]  (", ifmp, pgnoCurr, iline );
        if ( cpage.FRootPage() )
            dprintf( "root," );
        if ( cpage.FParentOfLeaf() )
            dprintf( "parent-of-leaf," );
        else
            dprintf( "internal," );
        dprintf( "pv==0x%N)\n", pbfBuf->pv );

        KEYDATAFLAGS    kdf;
        NDIGetKeydataflags( cpage, iline, &kdf );

        if ( sizeof(PGNO) != kdf.data.Cb() )
        {
            dprintf( "Error: bad parent page pointer.\n" );
            goto HandleError;
        }

        const PGNO  pgnoChild   = *(UnalignedLittleEndian< PGNO > *)kdf.data.Pv();

        if ( !FEDBGLoadPage_( ifmp, pgnoChild, rgbPageBuf, pbfBuf, &cbBuffer, cbPage ) )
            goto HandleError;

        cpage.LoadDehydratedPage( ifmp, pgnoChild, rgbPageBuf, cbBuffer, cbPage );

        pgnoCurr = pgnoChild;
    }

    //  find the first node in the leaf page that's
    //  greater than or equal to the specified bookmark
    //
    iline = IlineNDISeekGEQ( cpage, bm, !cpage.FNonUniqueKeys(), &compare );
    Assert( iline < cpage.Clines( ) );
    if ( iline < 0 )
    {
        //  all nodes in page are less than key
        //
        iline = cpage.Clines( ) - 1;
    }

    dprintf(
        "\t[%d:%d:%d]  (%sleaf,pvPage==0x%N)\n",
        ifmp,
        pgnoCurr,
        iline,
        ( cpage.FRootPage() ? "root," : "" ),
        pbfBuf->pv );

HandleError:
    if ( cpage.FLoadedPage() )
    {
        cpage.UnloadPage();
    }
    LocalFree( pbfBuf );
    LocalFree( rgbPageBuf );
}

//  ================================================================
DEBUG_EXT( EDBGSeek )
//  ================================================================
{
    ULONG       ifmp                = 0;        //  don't use ifmpNil because its type is IFMP
    ULONG       pgnoRoot            = pgnoNull;
    BYTE *      rgbPrefixDebuggee   = NULL;
    BYTE *      rgbPrefix           = NULL;
    ULONG       cbPrefix            = 0;
    BYTE *      rgbSuffixDebuggee   = NULL;
    BYTE *      rgbSuffix           = NULL;
    ULONG       cbSuffix            = 0;
    BYTE *      rgbDataDebuggee     = NULL;
    BYTE *      rgbData             = NULL;
    ULONG       cbData              = 0;
    ULONG       cbPage              = 0;
    BF **       rgpbfChunkDebuggee  = NULL;
    BF **       rgpbfChunkT         = NULL;
    LONG_PTR    cbfChunkT           = 0;
    LONG_PTR    cbfCacheAddressableT            = 0;
    LONG_PTR    cbfCacheSizeT   = 0;
    BOOL        fBadUsage           = fFalse;

    dprintf( "\n" );

    switch ( argc )
    {
        case 8:
            if ( !FAddressFromSz( argv[ 6 ], &rgbDataDebuggee )
                || !FUlFromSz( argv[ 7 ], &cbData ) )
            {
                fBadUsage = fTrue;
                break;
            }
            //  FALL THROUGH
        case 6:
            if ( !FAddressFromSz( argv[ 4 ], &rgbSuffixDebuggee )
                || !FUlFromSz( argv[ 5 ], &cbSuffix ) )
            {
                fBadUsage = fTrue;
                break;
            }
            //  FALL THROUGH
        case 4:
            if ( FAutoIfmpFromSz( argv[ 0 ], &ifmp )
                && FAutoPgnoRootFromSz( argv[ 1 ], &pgnoRoot )
                && FAddressFromSz( argv[ 2 ], &rgbPrefixDebuggee )
                && FUlFromSz( argv[ 3 ], &cbPrefix ) )
            {
                break;
            }
            //  FALL THROUGH
        default:
            fBadUsage = fTrue;
    }

    if ( fBadUsage )
    {
        dprintf( "Usage: SEEK <ifmp|.> <pgnoRoot|.> <pbPrefix> <cbPrefix> [<pbSuffix> <cbSuffix> [<pbData> <cbData>]]\n" );
    }
    else if ( ( cbPrefix > 0 && !FFetchVariable( rgbPrefixDebuggee, &rgbPrefix, cbPrefix ) )
        || ( cbSuffix > 0 && !FFetchVariable( rgbSuffixDebuggee, &rgbSuffix, cbSuffix ) )
        || ( cbData > 0 && !FFetchVariable( rgbDataDebuggee, &rgbData, cbData ) ) )
    {
        dprintf( "Error: Couldn't fetch the bookmark from the debuggee.\n" );
    }
    else if ( !FReadGlobal( "g_cbfChunk", &cbfChunkT )
        || !FReadGlobal( "cbfCacheAddressable", &cbfCacheAddressableT )
        || !FReadGlobal( "cbfCacheSize", &cbfCacheSizeT )
        || !FReadGlobal( "g_rgpbfChunk", &rgpbfChunkDebuggee )
        || !FFetchVariable( rgpbfChunkDebuggee, &rgpbfChunkT, cCacheChunkMax ) )
    {
        dprintf( "Error: Could not load BF parameters.\n" );
    }
    else
    {
        cbPage = Pdls()->CbPage();

        g_cbfChunk = cbfChunkT;
        cbfCacheAddressable = cbfCacheAddressableT;
        cbfCacheSize = cbfCacheSizeT;
        g_rgpbfChunk = rgpbfChunkT;

        //  perform seek to bookmark
        //
        BOOKMARK bm;
        bm.key.prefix.SetPv( rgbPrefix );
        bm.key.prefix.SetCb( cbPrefix );
        bm.key.suffix.SetPv( rgbSuffix );
        bm.key.suffix.SetCb( cbSuffix );
        bm.data.SetPv( rgbData );
        bm.data.SetCb( cbData );

        EDBGSeek_( IFMP( ifmp ), PGNO( pgnoRoot ), bm, cbPage );
    }

    dprintf( "\n" );

    Unfetch( rgpbfChunkT );
    Unfetch( rgbPrefix );
    Unfetch( rgbSuffix );
    Unfetch( rgbData );
}


/*
    =============================================
    VERSTORE helper data structures and functions
    =============================================
*/

// VERSTORE helper struct to combine a trxBegin0
//  and a pgnoFDP into a single data structure for
//  comparison purposes.
//
struct TrxBegin0AndPgnoFDP
{
    TRX         trxBegin0;
    IFMPPGNO    ifmppgnoFDP;

    BOOL operator==( const TrxBegin0AndPgnoFDP& trxBegin0AndPgnoFDP ) const
    {
        return( trxBegin0 == trxBegin0AndPgnoFDP.trxBegin0 && ifmppgnoFDP == trxBegin0AndPgnoFDP.ifmppgnoFDP );
    }

    BOOL operator<( const TrxBegin0AndPgnoFDP& trxBegin0AndPgnoFDP ) const
    {
        if ( trxBegin0 < trxBegin0AndPgnoFDP.trxBegin0 )
            return fTrue;
        else if ( trxBegin0 > trxBegin0AndPgnoFDP.trxBegin0 )
            return fFalse;
        else
            return ( ifmppgnoFDP < trxBegin0AndPgnoFDP.ifmppgnoFDP );
    }

    BOOL operator>( const TrxBegin0AndPgnoFDP& trxBegin0AndPgnoFDP ) const
    {
        if ( trxBegin0 > trxBegin0AndPgnoFDP.trxBegin0 )
            return fTrue;
        else if ( trxBegin0 < trxBegin0AndPgnoFDP.trxBegin0 )
            return fFalse;
        else
            return ( ifmppgnoFDP > trxBegin0AndPgnoFDP.ifmppgnoFDP );
    }
};


//  VERSTORE helper class to track version store usage
//  by number of entries and by aggregate RCE size.
//  This class is intended to be used as an abstract
//  class, and derived classes will define the criteria
//  by which version store usage will be tracked (e.g.
//  unique pgnoFDP, unique trxBegin0, etc.).
//
class CVersionStoreUsage
{
    ULONG       m_cEntries;
    ULONG_PTR   m_cbAggregateSize;

protected:
    CVersionStoreUsage() : m_cEntries( 0 ), m_cbAggregateSize( 0 )  { }

public:
    CVersionStoreUsage( const ULONG_PTR cbSize ) : m_cEntries( 1 ), m_cbAggregateSize( cbSize ) { }

    VOID AddUsage( const ULONG_PTR cbSize )
    {
        m_cEntries++;
        m_cbAggregateSize += cbSize;
    }

    ULONG CEntries() const              { return m_cEntries; }
    ULONG_PTR CbAggregateSize() const   { return m_cbAggregateSize; }
};


//  VERSTORE helper function to add a entry to
//  a red-black tree if the key does not already,
//  exist or update the existing entry if it
//  already exists.
//
template<class CKey>
ERR ErrAddReference(
    CRedBlackTree<CKey, CVersionStoreUsage> * prbt,
    const CKey&     key,
    const ULONG     cbSize,
    BOOL *          pfEntryAdded )
{
    ERR err = JET_errSuccess;

    Assert( NULL != prbt );
    Assert( NULL != pfEntryAdded );

    *pfEntryAdded = fFalse;

    //  see if the key already exists
    //
    const CRedBlackTreeNode<CKey, CVersionStoreUsage> * pnode = prbt->FindNode(key);
    if ( NULL != pnode )
    {
        //  key already exists, so just update the entry.
        //
        CVersionStoreUsage  vsusage = pnode->Data();
        vsusage.AddUsage( cbSize );
        const_cast<CRedBlackTreeNode<CKey, CVersionStoreUsage> *>(pnode)->SetData( vsusage );
    }
    else
    {
        //  key does not yet exist, so add it now
        //
        CVersionStoreUsage  vsusage( cbSize );
        CRedBlackTree<CKey, CVersionStoreUsage>::ERR errRBT = prbt->ErrInsert( key, vsusage );
        if ( CRedBlackTree<CKey, CVersionStoreUsage>::ERR::errSuccess == errRBT )
        {
            *pfEntryAdded = fTrue;
        }
    }

    return err;
}


//  VERSTORE helper class, derived from CVersionStoreUsage
//  to track version store usage for a single unique pgnoFDP.
//
class CPgnoFDPUsage : public CVersionStoreUsage
{
    IFMPPGNO    m_ifmppgnoFDP;

public:
    CPgnoFDPUsage() : m_ifmppgnoFDP( 0, 0 ) { }
    CPgnoFDPUsage( const IFMPPGNO& ifmppgnoFDP, const CVersionStoreUsage& vsusage ) :
        CVersionStoreUsage( vsusage ),
        m_ifmppgnoFDP( ifmppgnoFDP.ifmp, ifmppgnoFDP.pgno ) { }

    const IFMPPGNO& IfmpPgnoFDP() const { return m_ifmppgnoFDP; }

    static INT __cdecl CmpFrequency( const CPgnoFDPUsage * pusage1, const CPgnoFDPUsage * pusage2 )
    {
        const INT cmp   = (INT)( pusage2->CEntries() - pusage1->CEntries() );
        return ( 0 != cmp ? cmp : IFMPPGNO::Cmp( &pusage1->m_ifmppgnoFDP, &pusage2->m_ifmppgnoFDP ) );
    }

    static INT __cdecl CmpSize( const CPgnoFDPUsage * pusage1, const CPgnoFDPUsage * pusage2 )
    {
        const INT cmp   = (INT)( pusage2->CbAggregateSize() - pusage1->CbAggregateSize() );
        return ( 0 != cmp ? cmp : IFMPPGNO::Cmp( &pusage1->m_ifmppgnoFDP, &pusage2->m_ifmppgnoFDP ) );
    }
};


//  VERSTORE helper class, derived from CVersionStoreUsage
//  to track version store usage for a single unique trxBegin0.
//
class CTrxBegin0Usage : public CVersionStoreUsage
{
    TRX     m_trxBegin0;

public:
    CTrxBegin0Usage() : m_trxBegin0( 0 )    { }
    CTrxBegin0Usage( const TRX trxBegin0, const CVersionStoreUsage& vsusage  ) :
        CVersionStoreUsage( vsusage ),
        m_trxBegin0( trxBegin0 )    { }

    TRX TrxBegin0() const   { return m_trxBegin0; }

    static INT __cdecl CmpFrequency( const CTrxBegin0Usage * pusage1, const CTrxBegin0Usage * pusage2 )
    {
        const INT cmp   = (INT)( pusage2->CEntries() - pusage1->CEntries() );
        return ( 0 != cmp ? cmp : (INT)( pusage1->TrxBegin0() - pusage2->TrxBegin0() ) );
    }

    static INT __cdecl CmpSize( const CTrxBegin0Usage * pusage1, const CTrxBegin0Usage * pusage2 )
    {
        const INT cmp   = (INT)( pusage2->CbAggregateSize() - pusage1->CbAggregateSize() );
        return ( 0 != cmp ? cmp : (INT)( pusage1->TrxBegin0() - pusage2->TrxBegin0() ) );
    }
};


//  VERSTORE helper class, derived from CVersionStoreUsage
//  to track version store usage for a signle unique
//  trxBegin0+pgnoFDP.
//
class CTrxBegin0AndPgnoFDPUsage : public CVersionStoreUsage
{
    TRX         m_trxBegin0;
    IFMPPGNO    m_ifmppgnoFDP;

public:
    CTrxBegin0AndPgnoFDPUsage() : m_trxBegin0( 0 ), m_ifmppgnoFDP( 0, 0 )   { }
    CTrxBegin0AndPgnoFDPUsage( const TrxBegin0AndPgnoFDP& trxBegin0AndPgnoFDP, const CVersionStoreUsage& vsusage  ) :
        CVersionStoreUsage( vsusage ),
        m_trxBegin0( trxBegin0AndPgnoFDP.trxBegin0 ),
        m_ifmppgnoFDP( trxBegin0AndPgnoFDP.ifmppgnoFDP.ifmp, trxBegin0AndPgnoFDP.ifmppgnoFDP.pgno ) { }

    TRX TrxBegin0() const   { return m_trxBegin0; }
    const IFMPPGNO& IfmpPgnoFDP() const { return m_ifmppgnoFDP; }

    static INT __cdecl CmpFrequency( const CTrxBegin0AndPgnoFDPUsage * pusage1, const CTrxBegin0AndPgnoFDPUsage * pusage2 )
    {
        INT cmp     = (INT)( pusage2->CEntries() - pusage1->CEntries() );

        if ( 0 == cmp )
        {
            cmp = TrxCmp( pusage1->TrxBegin0(), pusage2->TrxBegin0() );
        }

        return ( 0 != cmp ? cmp : IFMPPGNO::Cmp( &pusage1->m_ifmppgnoFDP, &pusage2->m_ifmppgnoFDP ) );
    }

    static INT __cdecl CmpSize( const CTrxBegin0AndPgnoFDPUsage * pusage1, const CTrxBegin0AndPgnoFDPUsage * pusage2 )
    {
        INT cmp     = (INT)( pusage2->CbAggregateSize() - pusage1->CbAggregateSize() );

        if ( 0 == cmp )
        {
            cmp = TrxCmp( pusage1->TrxBegin0(), pusage2->TrxBegin0() );
        }

        return ( 0 != cmp ? cmp : IFMPPGNO::Cmp( &pusage1->m_ifmppgnoFDP, &pusage2->m_ifmppgnoFDP ) );
    }
};


//  VERSTORE helper function to copy the contents of
//  a node in a red-black tree to an array entry. It
//  also calls itself recursively in order to process
//  the child nodes of the current node.
//
template<class CEntry, class CKey>
ERR ErrPopulateUsageArray(CArray<CEntry> * rgUsage, const CRedBlackTreeNode<CKey, CVersionStoreUsage> * const pnode)
{
    ERR err = JET_errSuccess;

    if ( NULL != pnode )
    {
        err = ErrPopulateUsageArray<CEntry, CKey>( rgUsage, pnode->PnodeLeft() );
        if ( JET_errSuccess != err )
            goto HandleError;

        err = ErrPopulateUsageArray<CEntry, CKey>( rgUsage, pnode->PnodeRight() );
        if ( JET_errSuccess != err )
            goto HandleError;

        CEntry entryToAdd( pnode->Key(), pnode->Data() );
        if ( rgUsage->ErrSetEntry( rgUsage->Size(), entryToAdd ) != CArray<CEntry>::ERR::errSuccess )
        {
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }
    }

HandleError:
    return err;
}


//  VERSTORE helper function to copy the contents of
//  a red-black tree to an array. It's useful to copy
//  the contents to an array because then the array
//  can be sorted by various different criteria.
//
template<class CEntry, class CKey>
ERR ErrPrepareUsageArray(
    CArray<CEntry> *    rgUsage,
    const CRedBlackTreeNode<CKey, CVersionStoreUsage> * const   pnodeRoot,
    const ULONG_PTR     cEntries,
    const CHAR *        szDesc )
{
    ERR                 err                 = JET_errSuccess;

    CArray<CEntry>::ERR errArray = rgUsage->ErrSetCapacity( cEntries );
    if ( CArray<CEntry>::ERR::errSuccess != errArray )
    {
        Assert( errArray == CArray<CEntry>::ERR::errOutOfMemory );
        err = ErrERRCheck( JET_errOutOfMemory );
        dprintf( "Error 0x%x attempting to allocate array with %d entries for reporting top usage by %s. Aborting.\n", err, cEntries, szDesc );
        goto HandleError;
    }

    err = ErrPopulateUsageArray<CEntry, CKey>( rgUsage, pnodeRoot );
    if ( JET_errSuccess != err )
    {
        dprintf( "Error 0x%x attempting to populate %s usage array with %d entries. Aborting.\n", err, szDesc, cEntries );
        goto HandleError;
    }
    
HandleError:
    return err;
}


// VERSTORE helper function to walk the global FCB list
// hanging off INST and generate a red-black tree to
// map pgnoFDP's to FCB's.
//
ERR ErrBuildPgnoFDPToFCBMap(
    CRedBlackTree<IFMPPGNO, FCB*> * prbtPgnoFDPToFCB,
    INST *  pinst )
{
    ERR     err                                 = JET_errSuccess;
    CRedBlackTree<IFMPPGNO, FCB*>::ERR  errRBT  = CRedBlackTree<IFMPPGNO, FCB*>::ERR::errSuccess;
    FCB *   pfcb                                = pfcbNil;
    TDB *   ptdb                                = ptdbNil;
    FCB *   pfcbIndexes                         = pfcbNil;
    FCB *   pfcbNextInList                      = pfcbNil;

    pfcb = (FCB *)LocalAlloc( 0, sizeof(FCB) );
    if ( pfcbNil == pfcb )
    {
        dprintf( "\nError: Could not allocate FCB buffer. Cannot perform PgnoFDP-to-FCB mappings.\n" );
        goto HandleError;
    }

    ptdb = (TDB *)LocalAlloc( 0, sizeof(TDB) );
    if ( ptdbNil == ptdb )
    {
        dprintf( "\nError: Could not allocate TDB buffer. Cannot perform PgnoFDP-to-FCB mappings.\n" );
        goto HandleError;
    }

    for ( FCB * pfcbDebuggee = pinst->m_pfcbList;
        pfcbNil != pfcbDebuggee;
        pfcbDebuggee = pfcbNextInList )
    {
        //  check main FCB
        //
        if ( !FReadVariable( pfcbDebuggee, pfcb ) )
        {
            dprintf( "Error: Could not read table FCB at 0x%p. Aborting.\n", pfcbDebuggee );
            err = ErrERRCheck( JET_errOutOfMemory );
            goto HandleError;
        }

        //  pfcb variable will be re-used, so save
        //  off pointer to next FCB in global list
        //
        pfcbNextInList = pfcb->PfcbNextList();
        pfcbIndexes = pfcb->PfcbNextIndex();

        IFMPPGNO ifmppgnoFDPTable( pfcb->Ifmp(), pfcb->PgnoFDP() );
        errRBT = prbtPgnoFDPToFCB->ErrInsert( ifmppgnoFDPTable, pfcbDebuggee );
        if ( CRedBlackTree<IFMPPGNO, FCB*>::ERR::errSuccess != errRBT )
        {
            dprintf( "Error 0x%x adding table ifmp-pgnoFDP==0x%x-0x%x to the pgnoFDP-to-FCB map. Aborting.\n", err, pfcb->Ifmp(), pfcb->PgnoFDP() );
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }

        //  pfcb variable will be re-used, so save
        //  off pointer to next FCB in global list
        //
        pfcbNextInList = pfcb->PfcbNextList();
        pfcbIndexes = pfcb->PfcbNextIndex();

        //  check possible LV FCB
        //
        if ( NULL != pfcb->Ptdb() )
        {
            if ( !FReadVariable( pfcb->Ptdb(), ptdb ) )
            {
                dprintf( "Error: Could not read TDB at 0x%p. Aborting.\n", pfcb->Ptdb() );
                err = ErrERRCheck( JET_errOutOfMemory );
                goto HandleError;
            }

            if ( NULL != ptdb->PfcbLV() )
            {
                if ( !FReadVariable( ptdb->PfcbLV(), pfcb ) )
                {
                    dprintf( "Error: Could not read LV FCB at 0x%p. Aborting.\n", ptdb->PfcbLV() );
                    err = ErrERRCheck( JET_errOutOfMemory );
                    goto HandleError;
                }

                IFMPPGNO ifmppgnoFDPLongValue( pfcb->Ifmp(), pfcb->PgnoFDP() );
                errRBT = prbtPgnoFDPToFCB->ErrInsert( ifmppgnoFDPLongValue, ptdb->PfcbLV() );
                if ( CRedBlackTree<IFMPPGNO, FCB*>::ERR::errSuccess != errRBT )
                {
                    dprintf( "Error 0x%x adding LV ifmp-pgnoFDP==0x%x-0x%x to the pgnoFDP-to-FCB map. Aborting.\n", err, pfcb->Ifmp(), pfcb->PgnoFDP() );
                    Error( ErrERRCheck( JET_errOutOfMemory ) );
                }
            }
        }

        //  check possible index FCB's
        //
        for ( pfcbDebuggee = pfcbIndexes;
            pfcbNil != pfcbDebuggee;
            pfcbDebuggee = pfcb->PfcbNextIndex() )
        {
            if ( !FReadVariable( pfcbDebuggee, pfcb ) )
            {
                dprintf( "Error: Could not read index FCB at 0x%p. Aborting.\n", pfcbDebuggee );
                err = ErrERRCheck( JET_errOutOfMemory );
                goto HandleError;
            }
            
            IFMPPGNO ifmppgnoFDPIndex( pfcb->Ifmp(), pfcb->PgnoFDP() );
            errRBT = prbtPgnoFDPToFCB->ErrInsert( ifmppgnoFDPIndex, pfcbDebuggee );
            if ( CRedBlackTree<IFMPPGNO, FCB*>::ERR::errSuccess != errRBT )
            {
                dprintf( "Error 0x%x adding index ifmp-pgnoFDP==0x%x-0x%x to the pgnoFDP-to-FCB map. Aborting.\n", err, pfcb->Ifmp(), pfcb->PgnoFDP() );
                Error( ErrERRCheck( JET_errOutOfMemory ) );
            }
        }
    }

HandleError:
    LocalFree( ptdb );
    LocalFree( pfcb );

    return err;
}


//  VERSTORE helper function to report additional meta-data for a
//  btree given its ifmp and pgnoFDP
//
VOID ReportPgnoFDPInfo(const IFMPPGNO& ifmppgnoFDP, CRedBlackTree<IFMPPGNO, FCB*> * prbtPgnoFDPToFCB, const BOOL fRecovering )
{
    FCB *       pfcbDebuggee        = pfcbNil;
    FCB *       pfcb                = pfcbNil;
    FCB *       pfcbTable           = pfcbNil;
    IDB *       pidb                = pidbNil;
    CHAR *      szName              = NULL;
    CHAR *      szType              = NULL;

    if ( fRecovering )
    {
        szName = "<Indeterminate - in recovery>";
        szType = "<Unknown>";
    }
    else if ( CRedBlackTree<IFMPPGNO, FCB*>::ERR::errSuccess == prbtPgnoFDPToFCB->ErrFind( ifmppgnoFDP, &pfcbDebuggee ) )
    {
        if ( !FFetchVariable( pfcbDebuggee, &pfcb ) )
        {
            dprintf( "Error: Could not fetch FCB at 0x%N.", pfcbDebuggee );
            goto HandleError;
        }

        if ( pfcb->FTypeTable() )
        {
            szType = "Table";
            if ( FEDBGFetchTableMetaData( pfcbDebuggee, &pfcbTable ) )
            {
                szName = pfcbTable->Ptdb()->SzTableName();
            }
            else
            {
                szName = "<Unknown>";
            }
        }
        else if ( pfcb->FTypeSecondaryIndex() )
        {
            szType = "Index";
            if ( FEDBGFetchTableMetaData( pfcb->PfcbTable(), &pfcbTable )
                && FFetchVariable( pfcb->Pidb(), &pidb ) )
            {
                szName = pfcbTable->Ptdb()->SzIndexName( pidb->ItagIndexName(), pfcb->FDerivedIndex() );
            }
            else
            {
                szName = "<Unknown>";
            }
        }
        else if ( pfcb->FTypeLV() )
        {
            szType = "Long-Value";
            if ( FEDBGFetchTableMetaData( pfcb->PfcbTable(), &pfcbTable ) )
            {
                szName = pfcbTable->Ptdb()->SzTableName();
            }
            else
            {
                szName = "<Unknown>";
            }
        }
        else
        {
            szName = "<None>";
            if ( pfcb->FTypeDatabase() )
            {
                szType = "Database";
            }
            else if ( pfcb->FTypeTemporaryTable() )
            {
                szType = "TempTable";
            }
            else if ( pfcb->FTypeSort() )
            {
                szType = "Sort";
            }
            else
            {
                szType = "<Unknown>";
            }
        }
    }
    else
    {
        szName = "<Unknown>";
        szType = "<Unknown>";
    }

    dprintf(
        "%-40.40s    %-11.11s    0x%p\n",
        szName,
        szType,
        pfcbDebuggee );

HandleError:
    Unfetch( pidb );
    Unfetch( pfcbTable );
    Unfetch( pfcb );
}

ERR ErrFetchLogStream( const INST * const pinst, _Outptr_ LOG_STREAM ** pplgstream )
{
    //  try to retrieve tip of the log
    //
    LOG * plog = NULL;

    if ( NULL != pinst->m_plog && FFetchVariable( pinst->m_plog, &plog ) )
    {
        CHAR szLogSymbol[64];
        CHAR szLogStreamSymbol[64];
        ULONG typeIdLog;
        ULONG64 moduleLog;
        ULONG typeIdLogStream;
        ULONG64 moduleLogStream;

        OSStrCbFormatA( szLogSymbol, sizeof(szLogSymbol), "%ws", WszUtilImageName() );
        OSStrCbAppendA( szLogSymbol, sizeof(szLogSymbol), "!LOG" );
        OSStrCbFormatA( szLogStreamSymbol, sizeof(szLogStreamSymbol), "%ws", WszUtilImageName() );
        OSStrCbAppendA( szLogStreamSymbol, sizeof(szLogStreamSymbol), "!LOG_STREAM" );
        if ( SUCCEEDED( g_DebugSymbols->GetSymbolTypeId( szLogSymbol, &typeIdLog, &moduleLog ) ) &&
             SUCCEEDED( g_DebugSymbols->GetSymbolTypeId( szLogStreamSymbol, &typeIdLogStream, &moduleLogStream ) ) )
        {
            ULONG offset;

            if ( SUCCEEDED( g_DebugSymbols->GetFieldOffset( moduleLog, typeIdLog, "m_pLogStream", &offset ) ) &&
                 FFetchVariable( *((LOG_STREAM **)(((BYTE *)plog) + offset)), pplgstream ) )
            {
                Unfetch( plog );
                return JET_errSuccess;
            }
        }
        Unfetch( plog );
    }

    return ErrERRCheck( errNotFound );
}

ERR ErrFetchCheckpoint( const INST * const pinst, _Outptr_ CHECKPOINT ** ppcheckpoint )
{
    ULONG offset;

    CHAR szLogSymbol[64];
    ULONG typeIdLog;
    ULONG64 moduleLog;

    OSStrCbFormatA( szLogSymbol, sizeof(szLogSymbol), "%ws", WszUtilImageName() );
    OSStrCbAppendA( szLogSymbol, sizeof(szLogSymbol), "!LOG" );
    if ( !SUCCEEDED( g_DebugSymbols->GetSymbolTypeId( szLogSymbol, &typeIdLog, &moduleLog ) ) )
    {
        return ErrERRCheck( errNotFound );
    }

    LOG * plog = NULL;
    CHECKPOINT *    pcheckpoint     = NULL;

    if ( NULL != pinst->m_plog && FFetchVariable( pinst->m_plog, &plog ) )
    {
        if ( SUCCEEDED( g_DebugSymbols->GetFieldOffset( moduleLog, typeIdLog, "m_pcheckpoint", &offset ) ) &&
             FFetchVariable( *((CHECKPOINT **)(((BYTE *)plog) + offset)), ppcheckpoint ) )
        {
            Unfetch( plog );
            return JET_errSuccess;
        }
        Unfetch( plog );
    }

    Unfetch( pcheckpoint );

    return ErrERRCheck( errNotFound );
}

ERR ErrFetchCurrentLgfilehdr( const INST * const pinst, _Outptr_ LGFILEHDR ** pplgfilehdr )
{
    ERR err = JET_errSuccess;

    CHAR szLogStreamSymbol[64];
    ULONG typeIdLogStream;
    ULONG64 moduleLogStream;

    OSStrCbFormatA( szLogStreamSymbol, sizeof(szLogStreamSymbol), "%ws", WszUtilImageName() );
    OSStrCbAppendA( szLogStreamSymbol, sizeof(szLogStreamSymbol), "!LOG_STREAM" );
    if ( !SUCCEEDED( g_DebugSymbols->GetSymbolTypeId( szLogStreamSymbol, &typeIdLogStream, &moduleLogStream ) ) )
    {
        return ErrERRCheck( errNotFound );
    }

    LOG_STREAM * plgstream = NULL;
    CallR( ErrFetchLogStream( pinst, &plgstream ) );

    ULONG offset;
    LGFILEHDR *     plgfilehdr      = NULL;

    if ( SUCCEEDED( g_DebugSymbols->GetFieldOffset( moduleLogStream, typeIdLogStream, "m_plgfilehdr", &offset ) ) &&
         FFetchVariable( *((LGFILEHDR **)(((BYTE *)plgstream) + offset)), pplgfilehdr ) )
    {
        Unfetch( plgstream );
        Unfetch( plgfilehdr );
        return JET_errSuccess;
    }

    Unfetch( plgstream );
    Unfetch( plgfilehdr );

    return ErrERRCheck( errNotFound );
}

ERR ErrEDBGReadInstLgenCheckpoint( const INST * const pinst, _Out_ LONG * plgenCheckpoint )
{
    ERR err = JET_errSuccess;

    *plgenCheckpoint = 0;

    CHECKPOINT * pcheckpoint = NULL;
    CallR( ErrFetchCheckpoint( pinst, &pcheckpoint ) );

    *plgenCheckpoint = pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration;

    Unfetch( pcheckpoint );

    return JET_errSuccess;
}

ERR ErrEDBGReadInstLgenTip( const INST * const pinst, _Out_ LONG * plgenTip )
{
    ERR err = JET_errSuccess;

    *plgenTip = 0;

    LGFILEHDR * plgfilehdr = NULL;
    CallR( ErrFetchCurrentLgfilehdr( pinst, &plgfilehdr ) );

    *plgenTip = plgfilehdr->lgfilehdr.le_lGeneration;

    Unfetch( plgfilehdr );

    return JET_errSuccess;
}

VOID PIB::DumpOpenTrxUserDetails( _In_ CPRINTF * const pcprintf, _In_ const DWORD_PTR dwOffset, _In_ LONG lgenTip ) const
{
    (*pcprintf)( "                   Session Type:   %hs\n", FUserSession() ? "USER" : "SYSTEM (ESE-internal)" );
    if ( pcprintf == CPRINTFWDBG::PcprintfInstance() )
    {
        EDBGPrintfDml( "                       ThreadId:   <link cmd=\"~~[%I64x]s\">%I64d (%#I64x)</link>\n", (QWORD)TidActive(), (QWORD)TidActive(), (QWORD)TidActive() );
    }
    else
    {
        (*pcprintf)( "                       ThreadId:   %I64d (%#I64x)\n", (QWORD)TidActive(), (QWORD)TidActive() );
    }

    AssertEDBGSz( TidActive(), "The ThreadID/TID should not be 0, should be set if we are in a transaction.\n" );
    if ( m_fUseSessionContextForTrxContext )
    {
        (*pcprintf)( "                    UserContext:   %Ix\n", dwTrxContext );
        //  Currently (2013/02/024) I(SOMEONE) am only like 73% sure of this next assert.
        AssertEDBGSz( dwTrxContext == dwSessionContext, "When in a transaction the session and trx context is supposed to match ... right?" );
    }
    (*pcprintf)( "                          Level:   %d%hs", Level(), m_level ? " (>= 1 means open transaction)" : "" );
    if ( clevelsDeferBegin )
    {
        (*pcprintf)( " (%d = clevelsDeferBegin)\n", clevelsDeferBegin );
    }
    else
    {
        (*pcprintf)( "\n" );
    }
    (*pcprintf)( "                      Read Only:   %hs\n", FReadOnlyTrx() ? "Read Only" : "False - Read Write" );
    (*pcprintf)( "                       Run Time:   %0.3f seconds\n", (double)DtickDelta( m_tickLevel0Begin, Pdls()->TickDLSCurrent() )/1000 );
    (*pcprintf)( "               Session Updating:   %hs\n", FBegin0Logged() ? "Yes (fBegin0Logged = fTrue)" : "No (m_fBegin0Logged = fFalse)" );
    if ( CmpLgpos( lgposStart, lgposMax ) && lgenTip )
    {
        (*pcprintf)( "            Required Checkpoint:   %d logs", lgenTip - lgposStart.lGeneration );
    }
    else
    {
        (*pcprintf)( "            Required Checkpoint:   <none>" );
    }
    if ( CmpLgpos( lgposStart, lgposMax ) == 0 )
    {
        (*pcprintf)( " (lgposStart = lgposMax)\n", lgposStart.lGeneration, lgposStart.isec, lgposStart.ib );
    }
    else
    {
        (*pcprintf)( " (lgposStart = %08x:%04x:%04x)\n", lgposStart.lGeneration, lgposStart.isec, lgposStart.ib );
    }
    (*pcprintf)( "         Threads In JET on Sess:   %d\n", m_cInJetAPI );
    (*pcprintf)( "                        Cursors:   %d\n", cCursors );
    const OPERATION_CONTEXT * const poc = (OPERATION_CONTEXT *)PvOperationContext();
    (*pcprintf)( "                  Context Types:   %d.%d.%d (ClientType.OpType.OpId)\n", (DWORD)poc->nClientType, (DWORD)poc->nOperationType, (DWORD)poc->nOperationID );
    (*pcprintf)( "              Context User Info:   0x%x - 0x%x : 0x%x (UserId - CorrID : Flags)\n", poc->dwUserID, *(DWORD*)PvCorrelationID(), (DWORD)poc->fFlags );
    if ( pcprintf == CPRINTFWDBG::PcprintfInstance() )
    {
        EDBGPrintfDmlW( L"                        ESE TLS:   <link cmd=\"dt %ws!TLS 0x%N\">%N</link>\n", WszUtilImageName(), ptlsTrxBeginLast, ptlsTrxBeginLast );
    }
    else
    {
        (*pcprintf)( "                        ESE TLS:   %p\n", ptlsTrxBeginLast );
    }
    WCHAR wszTrxidStack[300];
    if ( TrxidStack().ErrDump( wszTrxidStack, _countof(wszTrxidStack), L"\n                         " ) >= JET_errSuccess )
    {
        (*pcprintf)( "                    TrxidStack[]\n");
        (*pcprintf)( "                         %ws\n", wszTrxidStack );
    }
    else
    {
        AssertEDBGSz( fFalse, "Unexpected that this would fail, should have enough buffer.  Debug, or increase buffer." );
    }
}
    

//  ================================================================
DEBUG_EXT( EDBGVerStore )
//  ================================================================
{
    ERR         err                     = JET_errSuccess;
    INST *      pinstDebuggee           = NULL;
    INST *      pinst                   = NULL;
    INST::PLS * rgpls                   = NULL;
    PIB *       ppibTrxOldestDebuggee   = NULL;
    PIB *       ppibTrxOldest           = NULL;
    VER *       pver                    = NULL;
    LOG *       plog                    = NULL;
    size_t      cbVERBUCKET             = 0;
    BOOL        fFilterByPgnoFDP        = fFalse;
    PGNO        pgnoFDPFilter           = pgnoNull;
    BOOL        fFilterByTrxBegin0      = fFalse;
    TRX         trxBegin0Filter         = trxMin;
    BOOL        fFilterByOper           = fFalse;
    OPER        operFilter              = 0;
    BOOL        fComputeTopUsage        = fFalse;
    BOOL        fDumpMode               = fFalse;
    BOOL        fDumpStack              = fFalse;
    BOOL        fBadUsage               = fFalse;

    dprintf( "\n" );

    if ( argc < 1
        || argc > 6
        || !FAutoAddressFromSz( argv[0], &pinstDebuggee ) )
    {
        fBadUsage = fTrue;
    }
    else
    {
        for ( INT iarg = 1; iarg < argc && !fBadUsage; iarg++ )
        {
            if ( argv[iarg][0] != '/' && argv[iarg][0] != '-' )
            {
                fBadUsage = fTrue;
            }
            else
            {
                switch ( argv[iarg][1] )
                {
                    case 'p':
                    case 'P':
                        if ( fFilterByPgnoFDP || !FUlFromSz( argv[iarg] + 2, &pgnoFDPFilter ) )
                        {
                            //  can't specify multiple pgnoFDP filters
                            //
                            fBadUsage = fTrue;
                        }
                        else
                        {
                            dprintf( "pgnoFDP filter: 0x%x\n", pgnoFDPFilter );
                            fFilterByPgnoFDP = fTrue;
                        }
                        break;

                    case 't':
                    case 'T':
                        if ( fFilterByTrxBegin0 || !FUlFromSz( argv[iarg] + 2, &trxBegin0Filter ) )
                        {
                            //  can't specify multiple trxBegin0 filters
                            //
                            fBadUsage = fTrue;
                        }
                        else
                        {
                            dprintf( "trxBegin0 filter: 0x%x\n", trxBegin0Filter );
                            fFilterByTrxBegin0 = fTrue;
                        }
                        break;

                    case 'o':
                    case 'O':
                        if ( fFilterByOper || !FUlFromSz( argv[iarg] + 2, (ULONG *)&operFilter ) )
                        {
                            //  can't specify multiple oper filters
                            //
                            fBadUsage = fTrue;
                        }
                        else
                        {
                            dprintf( "oper filter: 0x%x\n", operFilter );
                            fFilterByOper = fTrue;
                        }
                        break;

                    case 'u':
                    case 'U':
                        if ( fComputeTopUsage )
                        {
                            //  can't specify top-usage flag multiple times
                            //
                            fBadUsage = fTrue;
                        }
                        else
                        {
                            dprintf( "ComputeTopUsage==TRUE\n" );
                            fComputeTopUsage = fTrue;
                        }
                        break;

                    case 'd':
                    case 'D':
                        if ( fDumpMode )
                        {
                            //  can't specify dump-mode flag multiple times
                            //
                            fBadUsage = fTrue;
                        }
                        else
                        {
                            dprintf( "DumpMode==TRUE\n" );
                            fDumpMode = fTrue;
                        }
                        break;

                    case 's':
                    case 'S':
                        if ( fDumpStack )
                        {
                            //  can't specify dump-mode flag multiple times
                            //
                            fBadUsage = fTrue;
                        }
                        else
                        {
                            dprintf( "DumpStack==TRUE\n" );
                            fDumpStack = fTrue;
                        }
                        break;

                    default:
                        fBadUsage = fTrue;
                        break;
                }

            }
        }
    }

    if ( fBadUsage )
    {
        dprintf( "Usage: VERSTORE <instance|.> [<filters>] [/u] [/d]\n" );
        dprintf( "\n" );
        dprintf( "       <instance|.> is the pointer to the instance for which to\n" );
        dprintf( "         dump version store information. If you do not know the\n" );
        dprintf( "         instance id, you may be able to find it by manually\n" );
        dprintf( "         scanning the instance id array, which begins at\n" );
        dprintf( "         '%ws!%s' and contains '%ws!%s' entries.\n",
                        WszUtilImageName(),
                        VariableNameToString( g_rgpinst ),
                        WszUtilImageName(),
                        VariableNameToString( g_cpinstInit ) );
        dprintf( "\n" );
        dprintf( "      <filters> are one or more filters which restrict the\n" );
        dprintf( "        version store entries scanned to only those matching\n" );
        dprintf( "        the specified criteria. Valid filters are:\n" );
        dprintf( "            /p<n> - filter by pgnoFDP (e.g. '/p100')\n" );
        dprintf( "            /t<n> - filter by trxBegin0 (e.g. '/t0x12345')\n" );
        dprintf( "            /o<n> - filter by oper (e.g. '/o0x0001')\n" );
        dprintf( "\n" );
        dprintf( "      /u specifies to compute top usage statistics (if\n" );
        dprintf( "        filtering options are also specified, only matching\n" );
        dprintf( "        entries contribute to the statistics computed).\n" );
        dprintf( "\n" );
        dprintf( "      /d specifies dump mode, which causes the address of\n" );
        dprintf( "        all version store entries scanned to be reported (if\n" );
        dprintf( "        filtering options are also specified, only matching\n" );
        dprintf( "        entries are reported).\n" );
        dprintf( "\n" );
        dprintf( "      /s specifies to also dump the stack of oldest trx.\n" );
        dprintf( "\n\n" );
        return;
    }

    if ( FFetchVariable( pinstDebuggee, &pinst )
        && FFetchVariable( pinst->m_rgpls, &rgpls, pinst->m_cpls )
        && FFetchVariable( pinst->m_pver, &pver ) )
    {
        SIZE_T              dwOffset;
        CResourceManager*   prm;

        //  try to determine whether recovery is being run,
        //  but don't fail if we couldn't fetch LOG (it's
        //  not crucial because we're only fetching LOG to
        //  be able to see if the instance is in recovery
        //  in order to facilitate a bit of reporting).
        //
        const BOOL          fRecovering     = ( NULL != pinst->m_plog && FFetchVariable( pinst->m_plog, &plog ) && plog->FRecovering() );

        //  dump version store-related members of various structs

        dwOffset = (BYTE *)pinstDebuggee - (BYTE *)pinst;
        dprintf( "\n" );
        dprintf( "INSTANCE: 0x%N\n", pinstDebuggee );
        dprintf( FORMAT_POINTER( INST, pinst, m_pver, dwOffset ) );
        dprintf( FORMAT_INT( INST, pinst, m_trxNewest, dwOffset ) );
        dprintf( "\n" );


        dprintf( "OLDEST TRANSACTION CANDIDATES:\n" );
        for ( size_t iProc = 0; iProc < pinst->m_cpls; iProc++ )
        {
            PIB *   ppibTrxOldestCandidateDebuggee  = rgpls[ iProc ].m_ilTrxOldest.PrevMost();

            dprintf( "\tProc %d: ", iProc );

            if ( NULL != ppibTrxOldestCandidateDebuggee )
            {
                PIB *   ppibTrxOldestCandidate;

                if ( !FFetchVariable( ppibTrxOldestCandidateDebuggee, &ppibTrxOldestCandidate ) )
                {
                    dprintf( "Error: Could not fetch ppibTrxOldestCandidate from processor-local-storage. Aborting.\n" );
                    goto HandleError;
                }

                EDBGPrintfDml( "<link cmd=\"!ese dump pib 0x%N\">0x%N</link> (trxBegin0=0x%x)\n", ppibTrxOldestCandidateDebuggee, ppibTrxOldestCandidateDebuggee, ppibTrxOldestCandidate->trxBegin0 );

                if ( !ppibTrxOldest
                    || TrxCmp( ppibTrxOldestCandidate->trxBegin0, ppibTrxOldest->trxBegin0 ) < 0 )
                {
                    Unfetch( ppibTrxOldest );
                    ppibTrxOldestDebuggee = ppibTrxOldestCandidateDebuggee;
                    ppibTrxOldest = ppibTrxOldestCandidate;
                }
                else
                {
                    Unfetch( ppibTrxOldestCandidate );
                }
            }
            else
            {
                dprintf( "<null>\n" );
            }
        }

        dprintf( "\n" );

        if ( NULL != ppibTrxOldest )
        {
            CHAR szStackDump[ 70 ];
            //  try to retrieve tip of the log
            //
            LONG lgenTip = 0;
            const ERR errT = ErrEDBGReadInstLgenTip( pinst, &lgenTip );
            AssertEDBG( errT == JET_errSuccess || lgenTip == 0 );

            //  print basic user info
            //
            dwOffset = (BYTE *)ppibTrxOldestDebuggee - (BYTE *)ppibTrxOldest;
            EDBGPrintfDmlW( L"OLDEST TRANSACTION: <link cmd=\"!ese dump PIB 0x%N\">0x%N</link>\n", ppibTrxOldestDebuggee, ppibTrxOldestDebuggee );
            ppibTrxOldest->DumpOpenTrxUserDetails( CPRINTFWDBG::PcprintfInstance(), dwOffset, lgenTip );

            if ( fDumpStack )
            {
                //  now also dump the stack
                //
                DWORD_PTR tidTrxOldest = ppibTrxOldest->TidActive();
                //  NOTE: We're adding !clrstack even though it may not seem necessary because in some dynamic debug scenarios, it
                //  does not seem to print out with k alone.
                //  NOTE2: The .loadby sos clr is not needed on BE PROD machines, but is needed locally on really old debuggers
                //  to see !clrstack.  But the .loadby doesn't seem to cause any harm to !clrstack to work, so leaving it in for
                //  a while.
                OSStrCbFormatA( szStackDump, sizeof( szStackDump ), "~~[0x%I64x]s; k; .loadby sos clr; !clrstack; ~~[0x%I64x]s", 
                    (ULONG64)tidTrxOldest, 
                    (ULONG64)Pdls()->TidCurrent() /* restores debuggers current thread */ );
                dprintf( "                      Callstack: %hs\n", szStackDump );
                const BOOL fT = FEDBGDebuggerExecute( szStackDump );
                dprintf( "        Done( %hs )\n", fT ? "StackSuccess" : "StackFault" );
            }
        }
        else
        {
            dprintf( "OLDEST TRANSACTION: <none>\n" );
        }
        dprintf( "\n" );

        dwOffset = (BYTE *)pinst->m_pver - (BYTE *)pver;
        dprintf( "VER: 0x%N\n", pinst->m_pver );
        dprintf( FORMAT_POINTER( VER, pver, m_pbucketGlobalHead, dwOffset ) );
        dprintf( FORMAT_POINTER( VER, pver, m_pbucketGlobalTail, dwOffset ) );

        dprintf( FORMAT_VOID( VER, pver, m_cresBucket, dwOffset ) );
        dprintf( FORMAT_UINT( VER, pver, m_cbBucket, dwOffset ) );

        dprintf( FORMAT_UINT( VER, pver, m_trxBegin0LastLongRunningTransaction, dwOffset ) );
        dprintf( FORMAT_POINTER( VER, pver, m_ppibTrxOldestLastLongRunningTransaction, dwOffset ) );
        dprintf( FORMAT_UINT( VER, pver, m_dwTrxContextLastLongRunningTransaction, dwOffset ) );
        dprintf( "\n" );

        dprintf( "STATISTICS:\n" );
        dprintf( "-----------\n" );
        prm = CRMContainer::EDBGPRMFind( JET_residVERBUCKET );
        if ( NULL != prm )
        {
            FetchWrap<CResourceManager *> prmFetch;
            if ( prmFetch.FVariable( prm ) )
            {
                DWORD_PTR dwChunkSize = 0;
                DWORD_PTR dwChunksUsed = 0;
                DWORD_PTR dwObjectSize = 0;
                prmFetch->ErrGetParam( JET_resoperSize, &dwObjectSize );
                dprintf( "Bucket size (in bytes): %d (0x%08x)\n",
                    dwObjectSize, dwObjectSize );
                prmFetch->ErrGetParam( JET_resoperChunkSize, &dwChunkSize );
                prmFetch->ErrGetParam( JET_resoperCurrentUse, &dwChunksUsed );
                dprintf( "Global reserved space for buckets: %d (0x%08x)\n",
                    dwChunkSize * dwChunksUsed, dwChunkSize * dwChunksUsed );
                dprintf( "\n" );

                cbVERBUCKET = dwObjectSize;
            }
        }

        dprintf( "Scanning buckets for this instance...\n" );
        ULONG_PTR   cBuckets                    = 0;
        ULONG_PTR   cRCETotal                   = 0;
        ULONG_PTR   cbRCESizeTotal              = 0;
        ULONG_PTR   cbRCESizeMax                = 0;
        ULONG_PTR   cRCEMoved                   = 0;
        ULONG_PTR   cbRCEMoved                  = 0;
        ULONG_PTR   cRCEProxy                   = 0;
        ULONG_PTR   cbRCEProxy                  = 0;
        ULONG_PTR   cRCERolledBack              = 0;
        ULONG_PTR   cbRCERolledBack             = 0;
        ULONG_PTR   cRCENullified               = 0;
        ULONG_PTR   cbRCENullified              = 0;
        ULONG_PTR   cRCEUncommitted             = 0;
        ULONG_PTR   cbRCEUncommitted            = 0;
        const ULONG iEntries                    = 0;
        const ULONG iSize                       = 1;
        const ULONG iCleanable                  = 0;
        const ULONG iUncleanable                = 1;
        ULONG_PTR   cRCE[2][2]                  = { 0 };    //  first dimension is entries/size, second dimension is cleanable/uncleanable
        ULONG_PTR   cRCEFlagDelete[2][2]        = { 0 };
        ULONG_PTR   cRCEDelta[2][2]             = { 0 };
        ULONG_PTR   cRCEInsert[2][2]            = { 0 };
        ULONG_PTR   cRCEReplace[2][2]           = { 0 };
        ULONG_PTR   cRCEWriteLock[2][2]         = { 0 };

        TRX         trxBegin0OfCommittedWithOldestBegin0    = trxMax;
        TRX         trxCommit0OfCommittedWithOldestBegin0   = trxMax;
        TRX         trxBegin0OfCommittedWithOldestCommit0   = trxMax;
        TRX         trxCommit0OfCommittedWithOldestCommit0  = trxMax;
        TRX         trxBegin0OfCommittedWithNewestBegin0    = trxMin;
        TRX         trxCommit0OfCommittedWithNewestBegin0   = trxMin;
        TRX         trxBegin0OfCommittedWithNewestCommit0   = trxMin;
        TRX         trxCommit0OfCommittedWithNewestCommit0  = trxMin;
        TRX         trxBegin0OfWidestCommitted              = trxMin;
        TRX         trxCommit0OfWidestCommitted             = trxMin;
        ULONG       dWidestTrx                              = 0;

        CRedBlackTree<IFMPPGNO, CVersionStoreUsage>             rbtPgnoFDP;
        CRedBlackTree<TRX, CVersionStoreUsage>                  rbtTrxBegin0;
        CRedBlackTree<TrxBegin0AndPgnoFDP, CVersionStoreUsage>  rbtTrxBegin0AndPgnoFDP;
        CRedBlackTree<IFMPPGNO, FCB*>                           rbtPgnoFDPToFCB;

        ULONG_PTR   cUniquePgnoFDP              = 0;
        ULONG_PTR   cUniqueTrxBegin0            = 0;
        ULONG_PTR   cUniqueTrxBegin0AndPgnoFDP  = 0;

        BUCKET *    pbucket;
        BUCKET *    pbucketDebuggee;
        for ( pbucketDebuggee = pver->m_pbucketGlobalTail;
            NULL != pbucketDebuggee;
            pbucketDebuggee = pbucket->hdr.pbucketNext )
        {

            if ( !FFetchVariable( (BYTE*)pbucketDebuggee, (BYTE**)&pbucket, cbVERBUCKET ) )
            {
                dprintf( "Error: Could not read BUCKET memory. Aborting.\n\n" );
                break;
            }

            //  determine whether bucket was pre-reserved or
            //  dynamically allocated and increment appropriate
            //  counter
            cBuckets++;

            //  scan RCE's in this bucket
            const RCE *         prce            = (RCE *)pbucket->rgb;
            const RCE * const   prceNextNew     = (RCE *)( (BYTE *)pbucket->hdr.prceNextNew - (BYTE *)pbucketDebuggee + (BYTE *)pbucket );
            const BYTE * const  pbLastDelete    = pbucket->hdr.pbLastDelete - (BYTE *)pbucketDebuggee + (BYTE *)pbucket;

            while ( prceNextNew != prce )
            {
                const ULONG     cbRCE       = prce->CbRceEDBG();
                const BYTE *    pbNextRce   = reinterpret_cast<BYTE *>( PvAlignForThisPlatform( (BYTE *)prce + cbRCE ) );
                BOOL            fEntryAdded = fFalse;

                if ( ( fFilterByPgnoFDP && prce->PgnoFDP() != pgnoFDPFilter )
                    || ( fFilterByTrxBegin0 && prce->TrxBegin0() != trxBegin0Filter )
                    || ( fFilterByOper && prce->Oper() != operFilter ) )
                {
                    //  one or more filters were specified and this RCE doesn't
                    //  meet the criteria, so skip to the next RCE
                    //
                    goto NextRCE;
                }
                else
                {
                    //  either no filters are in effect or one or more filters are
                    //  in effect and this RCE matches the criteria, so go ahead
                    //  and process it
                    //
                    if ( fDumpMode )
                    {
                        dprintf( "    0x%p\n", (BYTE *)prce - (BYTE *)pbucket + (BYTE *)pbucketDebuggee );
                    }

                    //  increment total and size counters
                    cRCETotal++;
                    cbRCESizeTotal += cbRCE;
                    cbRCESizeMax = max( cbRCESizeMax, cbRCE );

                    //  if top-usage stats were requested,
                    //  track various aspects of the RCE
                    //
                    if ( fComputeTopUsage )
                    {
                        //  use a red-black tree to track RCE's by unique pgnoFDP
                        //
                        IFMPPGNO    ifmppgnoFDP( prce->Ifmp(), prce->PgnoFDP() );
                        err = ErrAddReference<IFMPPGNO>( &rbtPgnoFDP, ifmppgnoFDP, cbRCE, &fEntryAdded );
                        if ( JET_errSuccess != err )
                        {
                            dprintf( "Error 0x%x returned attempting to add an entry to the pgnoFDP list. Aborting.\n\n", err );
                            goto HandleError;
                        }
                        else if ( fEntryAdded )
                        {
                            cUniquePgnoFDP++;
                        }

                        //  use a red-black tree to track RCE's by unique trxBegin0
                        //
                        err = ErrAddReference<TRX>( &rbtTrxBegin0, prce->TrxBegin0(), cbRCE, &fEntryAdded );
                        if ( JET_errSuccess != err )
                        {
                            dprintf( "Error 0x%x returned attempting to add an entry to the trxBegin0 list. Aborting.\n\n", err );
                            goto HandleError;
                        }
                        else if ( fEntryAdded )
                        {
                            cUniqueTrxBegin0++;
                        }

                        //  use a red-black tree to track RCE's by unique trxBegin0+pgnoFDP
                        //
                        TrxBegin0AndPgnoFDP trxBegin0AndPgnoFDP = { prce->TrxBegin0(), ifmppgnoFDP };
                        err = ErrAddReference<TrxBegin0AndPgnoFDP>( &rbtTrxBegin0AndPgnoFDP, trxBegin0AndPgnoFDP, cbRCE, &fEntryAdded );
                        if ( JET_errSuccess != err )
                        {
                            dprintf( "Error 0x%x returned attempting to add an entry to the trxBegin0-pgnoFDP list. Aborting.\n\n", err );
                            goto HandleError;
                        }
                        else if ( fEntryAdded )
                        {
                            cUniqueTrxBegin0AndPgnoFDP++;
                        }
                    }
                }

                //  was RCE moved?
                if ( prce->FMoved() )
                {
                    cRCEMoved++;
                    cbRCEMoved += cbRCE;
                }

                //  was RCE created by proxy?
                if ( prce->FProxy() )
                {
                    cRCEProxy++;
                    cbRCEProxy += cbRCE;
                }

                //  was RCE rolled back?
                if ( prce->FRolledBackEDBG() )
                {
                    cRCERolledBack++;
                    cbRCERolledBack += cbRCE;
                }

                //  determine the state of the RCE and increment
                //  appropriate counters
                if ( prce->FOperNull() )
                {
                    cRCENullified++;
                    cbRCENullified += cbRCE;
                }
                else if ( trxMax == prce->TrxCommittedEDBG() )
                {
                    cRCEUncommitted++;
                    cbRCEUncommitted += cbRCE;
                }
                else
                {
                    //  track the committed transaction with the oldest trxBegin0
                    //
                    if ( TrxCmp( prce->TrxBegin0(), trxBegin0OfCommittedWithOldestBegin0 ) < 0 )
                    {
                        trxBegin0OfCommittedWithOldestBegin0 = prce->TrxBegin0();
                        trxCommit0OfCommittedWithOldestBegin0 = prce->TrxCommittedEDBG();
                    }

                    //  track the committed transaction with the oldest trxCommit0
                    //
                    if ( TrxCmp( prce->TrxCommittedEDBG(), trxCommit0OfCommittedWithOldestCommit0 ) < 0 )
                    {
                        trxBegin0OfCommittedWithOldestCommit0 = prce->TrxBegin0();
                        trxCommit0OfCommittedWithOldestCommit0 = prce->TrxCommittedEDBG();
                    }

                    //  track the committed transaction with the newest trxBegin0
                    //
                    if ( TrxCmp( prce->TrxBegin0(), trxBegin0OfCommittedWithNewestBegin0 ) > 0 )
                    {
                        trxBegin0OfCommittedWithNewestBegin0 = prce->TrxBegin0();
                        trxCommit0OfCommittedWithNewestBegin0 = prce->TrxCommittedEDBG();
                    }

                    //  track the committed transaction with the newest trxCommit0
                    //
                    if ( TrxCmp( prce->TrxCommittedEDBG(), trxCommit0OfCommittedWithNewestCommit0 ) > 0 )
                    {
                        trxBegin0OfCommittedWithNewestCommit0 = prce->TrxBegin0();
                        trxCommit0OfCommittedWithNewestCommit0 = prce->TrxCommittedEDBG();
                    }

                    //  track the widest committed transaction (when computing the widest committed
                    //  transaction, we need to be careful to handle trx wraparound)
                    //
                    const ULONG dTrxWidth   = ( prce->TrxCommittedEDBG() > prce->TrxBegin0()
                        ? prce->TrxCommittedEDBG() - prce->TrxBegin0()
                        : prce->TrxCommittedEDBG() - (LONG)prce->TrxBegin0() );
                    if ( dTrxWidth > dWidestTrx )
                    {
                        dWidestTrx = dTrxWidth;
                        trxBegin0OfWidestCommitted = prce->TrxBegin0();
                        trxCommit0OfWidestCommitted = prce->TrxCommittedEDBG();
                    }

                    if ( NULL == ppibTrxOldest
                        || TrxCmp( prce->TrxCommittedEDBG(), ppibTrxOldest->trxBegin0 ) < 0 )
                    {
                        cRCE[iEntries][iCleanable]++;
                        cRCE[iSize][iCleanable] += cbRCE;
                    
                        switch ( prce->Oper() )
                        {
                            //  these are expensive to clean
                            //
                            case operFlagDelete:
                                cRCEFlagDelete[iEntries][iCleanable]++;
                                cRCEFlagDelete[iSize][iCleanable] += cbRCE;
                                break;
                            case operDelta:
                                cRCEDelta[iEntries][iCleanable]++;
                                cRCEDelta[iSize][iCleanable] += cbRCE;
                                break;
                    
                            //  these are cheap to clean
                            //
                            case operInsert:
                                cRCEInsert[iEntries][iCleanable]++;
                                cRCEInsert[iSize][iCleanable] += cbRCE;
                                break;
                            case operReplace:
                                cRCEReplace[iEntries][iCleanable]++;
                                cRCEReplace[iSize][iCleanable] += cbRCE;
                                break;
                            case operWriteLock:
                                cRCEWriteLock[iEntries][iCleanable]++;
                                cRCEWriteLock[iSize][iCleanable] += cbRCE;
                                break;
                            default:
                                break;
                        }
                    }
                    else
                    {
                        cRCE[iEntries][iUncleanable]++;
                        cRCE[iSize][iUncleanable] += cbRCE;
                    
                        switch ( prce->Oper() )
                        {
                            case operFlagDelete:
                                cRCEFlagDelete[iEntries][iUncleanable]++;
                                cRCEFlagDelete[iSize][iUncleanable] += cbRCE;
                                break;
                            case operDelta:
                                cRCEDelta[iEntries][iUncleanable]++;
                                cRCEDelta[iSize][iUncleanable] += cbRCE;
                                break;
                            case operInsert:
                                cRCEInsert[iEntries][iUncleanable]++;
                                cRCEInsert[iSize][iUncleanable] += cbRCE;
                                break;
                            case operReplace:
                                cRCEReplace[iEntries][iUncleanable]++;
                                cRCEReplace[iSize][iUncleanable] += cbRCE;
                                break;
                            case operWriteLock:
                                cRCEWriteLock[iEntries][iUncleanable]++;
                                cRCEWriteLock[iSize][iUncleanable] += cbRCE;
                                break;
                            default:
                                break;
                        }
                    }
                }

NextRCE:
                //  move to next RCE, being careful to account
                //  for case where we are on the last moved RCE
                if ( pbNextRce != pbLastDelete )
                {
                    prce = (RCE *)pbNextRce;
                }
                else
                {
                    prce = (RCE *)( (BYTE *)pbucket->hdr.prceOldest - (BYTE *)pbucketDebuggee + (BYTE *)pbucket );
                }
            }
        }

        dprintf( "\n" );
        dprintf( "  Buckets in use: %d (0x%p)\n", cBuckets, QWORD( cBuckets ) );
        dprintf( "\n" );

        dprintf( "  Total version store entries: %d (0x%0*I64x)\n", cRCETotal, sizeof(ULONG_PTR)*2, QWORD( cRCETotal ) );
        dprintf( "  Total size of all entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cbRCESizeTotal ), sizeof(ULONG_PTR)*2, QWORD( cbRCESizeTotal ) );
        dprintf( "  Max. entry size (in bytes): %I64d (0x%0*I64x)\n", QWORD( cbRCESizeMax ), sizeof(ULONG_PTR)*2, QWORD( cbRCESizeMax ) );
        dprintf( "  Average entry size (in bytes): %I64d (0x%0*I64x)\n", QWORD( 0 != cRCETotal ? cbRCESizeTotal/cRCETotal : 0 ), sizeof(ULONG_PTR)*2, QWORD( 0 != cRCETotal ? cbRCESizeTotal/cRCETotal : 0 ) );
        dprintf( "\n" );


        dprintf( "  Entries moved: %d (0x%0*I64x)\n", cRCEMoved, sizeof(ULONG_PTR)*2, QWORD( cRCEMoved ) );
        dprintf( "  Total size of moved entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cbRCEMoved ), sizeof(ULONG_PTR)*2, QWORD( cbRCEMoved ) );
        dprintf( "\n" );

        dprintf( "  Entries created by proxy: %d (0x%0*I64x)\n", cRCEProxy, sizeof(ULONG_PTR)*2, QWORD( cRCEProxy ) );
        dprintf( "  Total size of entries created by proxy (in bytes): %I64d (0x%0*I64x)\n", QWORD( cbRCEProxy ), sizeof(ULONG_PTR)*2, QWORD( cbRCEProxy ) );
        dprintf( "\n" );

        dprintf( "  Entries rolled-back: %d (0x%0*I64x)\n", cRCERolledBack, sizeof(ULONG_PTR)*2, QWORD( cRCERolledBack ) );
        dprintf( "  Total size of rolled-back entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cbRCERolledBack ), sizeof(ULONG_PTR)*2, QWORD( cbRCERolledBack ) );
        dprintf( "\n" );

        dprintf( "  Entries nullified: %d (0x%0*I64x)\n", cRCENullified, sizeof(ULONG_PTR)*2, QWORD( cRCENullified ) );
        dprintf( "  Total size of nullified entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cbRCENullified ), sizeof(ULONG_PTR)*2, QWORD( cbRCENullified ) );
        dprintf( "\n" );

        dprintf( "  Entries uncommitted: %d (0x%0*I64x)\n", cRCEUncommitted, sizeof(ULONG_PTR)*2, QWORD( cRCEUncommitted ) );
        dprintf( "  Total size of uncommitted entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cbRCEUncommitted ), sizeof(ULONG_PTR)*2, QWORD( cbRCEUncommitted ) );
        dprintf( "\n" );

        dprintf( "  Entries committed: %d (0x%0*I64x)\n", cRCE[iEntries][iCleanable]+cRCE[iEntries][iUncleanable], sizeof(ULONG_PTR)*2, QWORD( cRCE[iEntries][iCleanable]+cRCE[iEntries][iUncleanable] ) );
        dprintf( "  Total size of committed entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cRCE[iSize][iCleanable]+cRCE[iSize][iUncleanable] ), sizeof(ULONG_PTR)*2, QWORD( cRCE[iSize][iCleanable]+cRCE[iSize][iUncleanable] ) );
        if ( ( cRCE[iEntries][iCleanable]+cRCE[iEntries][iUncleanable] ) != 0 ||
                ( cRCE[iSize][iCleanable]+cRCE[iSize][iUncleanable] ) != 0 )
        {
            dprintf( "    Cleanable Entries: %d (0x%0*I64x)\n", cRCE[iEntries][iCleanable], sizeof(ULONG_PTR)*2, QWORD( cRCE[iEntries][iCleanable] ) );
            dprintf( "      Cleanable (expensive) 'FlagDelete' entries: %d (0x%0*I64x)\n", cRCEFlagDelete[iEntries][iCleanable], sizeof(ULONG_PTR)*2, QWORD( cRCEFlagDelete[iEntries][iCleanable] ) );
            dprintf( "      Cleanable (expensive) 'Delta' entries: %d (0x%0*I64x)\n", cRCEDelta[iEntries][iCleanable], sizeof(ULONG_PTR)*2, QWORD( cRCEDelta[iEntries][iCleanable] ) );
            dprintf( "      Cleanable (cheap) 'Insert' entries: %d (0x%0*I64x)\n", cRCEInsert[iEntries][iCleanable], sizeof(ULONG_PTR)*2, QWORD( cRCEInsert[iEntries][iCleanable] ) );
            dprintf( "      Cleanable (cheap) 'Replace' entries: %d (0x%0*I64x)\n", cRCEReplace[iEntries][iCleanable], sizeof(ULONG_PTR)*2, QWORD( cRCEReplace[iEntries][iCleanable] ) );
            dprintf( "      Cleanable (cheap) 'WriteLock' entries: %d (0x%0*I64x)\n", cRCEWriteLock[iEntries][iCleanable], sizeof(ULONG_PTR)*2, QWORD( cRCEWriteLock[iEntries][iCleanable] ) );
            dprintf( "    Total size of cleanable entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cRCE[iSize][iCleanable] ), sizeof(ULONG_PTR)*2, QWORD( cRCE[iSize][iCleanable] ) );
            dprintf( "      Size of cleanable 'FlagDelete' entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cRCEFlagDelete[iSize][iCleanable] ), sizeof(ULONG_PTR)*2, QWORD( cRCEFlagDelete[iSize][iCleanable] ) );
            dprintf( "      Size of cleanable 'Delta' entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cRCEDelta[iSize][iCleanable] ), sizeof(ULONG_PTR)*2, QWORD( cRCEDelta[iSize][iCleanable] ) );
            dprintf( "      Size of cleanable 'Insert' entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cRCEInsert[iSize][iCleanable] ), sizeof(ULONG_PTR)*2, QWORD( cRCEInsert[iSize][iCleanable] ) );
            dprintf( "      Size of cleanable 'Replace' entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cRCEReplace[iSize][iCleanable] ), sizeof(ULONG_PTR)*2, QWORD( cRCEReplace[iSize][iCleanable] ) );
            dprintf( "      Size of cleanable 'WriteLock' entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cRCEWriteLock[iSize][iCleanable] ), sizeof(ULONG_PTR)*2, QWORD( cRCEWriteLock[iSize][iCleanable] ) );
            dprintf( "    Uncleanable Entries: %d (0x%0*I64x)\n", cRCE[iEntries][iUncleanable], sizeof(ULONG_PTR)*2, QWORD( cRCE[iEntries][iUncleanable] ) );
            dprintf( "      Uncleanable 'FlagDelete' entries: %d (0x%0*I64x)\n", cRCEFlagDelete[iEntries][iUncleanable], sizeof(ULONG_PTR)*2, QWORD( cRCEFlagDelete[iEntries][iUncleanable] ) );
            dprintf( "      Uncleanable 'Delta' entries: %d (0x%0*I64x)\n", cRCEDelta[iEntries][iUncleanable], sizeof(ULONG_PTR)*2, QWORD( cRCEDelta[iEntries][iUncleanable] ) );
            dprintf( "      Uncleanable 'Insert' entries: %d (0x%0*I64x)\n", cRCEInsert[iEntries][iUncleanable], sizeof(ULONG_PTR)*2, QWORD( cRCEInsert[iEntries][iUncleanable] ) );
            dprintf( "      Uncleanable 'Replace' entries: %d (0x%0*I64x)\n", cRCEReplace[iEntries][iUncleanable], sizeof(ULONG_PTR)*2, QWORD( cRCEReplace[iEntries][iUncleanable] ) );
            dprintf( "      Uncleanable 'WriteLock' entries: %d (0x%0*I64x)\n", cRCEWriteLock[iEntries][iUncleanable], sizeof(ULONG_PTR)*2, QWORD( cRCEWriteLock[iEntries][iUncleanable] ) );
            dprintf( "    Total size of Uncleanable entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cRCE[iSize][iUncleanable] ), sizeof(ULONG_PTR)*2, QWORD( cRCE[iSize][iUncleanable] ) );
            dprintf( "      Size of uncleanable 'FlagDelete' entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cRCEFlagDelete[iSize][iUncleanable] ), sizeof(ULONG_PTR)*2, QWORD( cRCEFlagDelete[iSize][iUncleanable] ) );
            dprintf( "      Size of uncleanable 'Delta' entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cRCEDelta[iSize][iUncleanable] ), sizeof(ULONG_PTR)*2, QWORD( cRCEDelta[iSize][iUncleanable] ) );
            dprintf( "      Size of uncleanable 'Insert' entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cRCEInsert[iSize][iUncleanable] ), sizeof(ULONG_PTR)*2, QWORD( cRCEInsert[iSize][iUncleanable] ) );
            dprintf( "      Size of uncleanable 'Replace' entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cRCEReplace[iSize][iUncleanable] ), sizeof(ULONG_PTR)*2, QWORD( cRCEReplace[iSize][iUncleanable] ) );
            dprintf( "      Size of uncleanable 'WriteLock' entries (in bytes): %I64d (0x%0*I64x)\n", QWORD( cRCEWriteLock[iSize][iUncleanable] ), sizeof(ULONG_PTR)*2, QWORD( cRCEWriteLock[iSize][iUncleanable] ) );
        }
        else
        {
            dprintf( "    <No committed entries, skipping committed details>\n" );
        }
        dprintf( "\n" );

        dprintf( "  Committed transactions:\n" );
        dprintf( "    Oldest Begin0: 0x%x (Commit0==0x%x)\n", trxBegin0OfCommittedWithOldestBegin0, trxCommit0OfCommittedWithOldestBegin0 );
        dprintf( "    Oldest Commit0: 0x%x (Begin0==0x%x)\n", trxBegin0OfCommittedWithOldestCommit0, trxCommit0OfCommittedWithOldestCommit0 );
        dprintf( "    Newest Begin0: 0x%x (Commit0==0x%x)\n", trxBegin0OfCommittedWithNewestBegin0, trxCommit0OfCommittedWithNewestBegin0 );
        dprintf( "    Newest Commit0: 0x%x (Begin0==0x%x)\n", trxBegin0OfCommittedWithNewestCommit0, trxCommit0OfCommittedWithNewestCommit0 );
        dprintf( "    Widest Transaction: 0x%x (Begin0==0x%x, Commit0==0x%x)", dWidestTrx, trxBegin0OfWidestCommitted, trxCommit0OfWidestCommitted );
        dprintf( "\n\n" );

        if ( fComputeTopUsage )
    {
            if ( cUniquePgnoFDP > 0 || cUniqueTrxBegin0AndPgnoFDP > 0 )
        {
                dprintf( "Computing top usage statistics...\n" );

                //  we have some unique pgnoFDP's, so if not recovering
                //  (because FCB's are not fully hydrated with meta-data
                //  during recovery), build pgnoFDP-to-FCB map with which
                //  we'll be able to look up additional meta-data (such
                //  as btree name and btree type) for reporting purposes
                //  (NOTE: this can be time-consuming because it walks the
                //  instance's entire FCB list)
                //
                if ( !fRecovering )
                {
                    err = ErrBuildPgnoFDPToFCBMap( &rbtPgnoFDPToFCB, pinst );
                    if ( JET_errSuccess != err )
                        goto HandleError;
                }
        }
            else
        {
                dprintf(
                    "NOTE: Top usage statistics were not computed (because no %sentries were detected).\n",
                    ( fFilterByPgnoFDP || fFilterByTrxBegin0 || fFilterByOper ? "matching " : "" ) );
        }
    }

        const ULONG_PTR cTopUsage   = 20;

        //  report top usage by unique pgnoFDP
        //
        if ( cUniquePgnoFDP > 0 )
        {
            const ULONG_PTR         centriesToReport    = min( cUniquePgnoFDP, cTopUsage );
            CArray<CPgnoFDPUsage>   rgPgnoFDP;

            //  copy the red-black tree containing the list
            //  of unique pgnoFDP's to an array
            //
            err = ErrPrepareUsageArray<CPgnoFDPUsage, IFMPPGNO>(
                &rgPgnoFDP,
                rbtPgnoFDP.PnodeRoot(),
                cUniquePgnoFDP,
                "PgnoFDP" );
            if ( JET_errSuccess != err )
                goto HandleError;

            //  sort the array by descending number of RCE's
            //
            rgPgnoFDP.Sort( CPgnoFDPUsage::CmpFrequency );

            //  report top consumers
            //
            dprintf( "\n" );
            dprintf( "Top %d PgnoFDP by Number of Entries (out of %d unique values):\n", centriesToReport, cUniquePgnoFDP );
            dprintf( "===================================================================\n" );
            dprintf( "    Entries       Aggregate Size    Ifmp PgnoFDP       B-Tree Name                                 B-Tree Type    FCB*\n" );
            dprintf( "    ----------    --------------    ---- ----------    ----------------------------------------    -----------    ------------------\n" );

            for ( ULONG_PTR i = 0; i < centriesToReport; i++ )
            {
                const CPgnoFDPUsage *   pentry  = rgPgnoFDP.PEntry( i );
                dprintf(
                    "    %10d    %14I64d    0x%02x 0x%08x    ",
                    pentry->CEntries(),
                    pentry->CbAggregateSize(),
                    pentry->IfmpPgnoFDP().ifmp,
                    pentry->IfmpPgnoFDP().pgno );
                ReportPgnoFDPInfo( pentry->IfmpPgnoFDP(), &rbtPgnoFDPToFCB, fRecovering );
            }
            dprintf( "\n\n" );

            //  sort the array by descending aggregate RCE size
            //
            rgPgnoFDP.Sort( CPgnoFDPUsage::CmpSize );

            dprintf( "Top %d PgnoFDP by Aggregate Size (out of %d unique values):\n", centriesToReport, cUniquePgnoFDP );
            dprintf( "================================================================\n" );
            dprintf( "    Entries       Aggregate Size    Ifmp PgnoFDP       B-Tree Name                                 B-Tree Type    FCB*\n" );
            dprintf( "    ----------    --------------    ---- ----------    ----------------------------------------    -----------    ------------------\n" );

            //  report top consumers
            //
            for ( ULONG_PTR i = 0; i < centriesToReport; i++ )
            {
                const CPgnoFDPUsage *   pentry  = rgPgnoFDP.PEntry( i );
                dprintf(
                    "    %10d    %14I64d    0x%02x 0x%08x    ",
                    pentry->CEntries(),
                    pentry->CbAggregateSize(),
                    pentry->IfmpPgnoFDP().ifmp,
                    pentry->IfmpPgnoFDP().pgno );
                ReportPgnoFDPInfo( pentry->IfmpPgnoFDP(), &rbtPgnoFDPToFCB, fRecovering );
            }
            dprintf( "\n\n" );
        }

        //  report top usage by unique trxBegin0
        //
        if ( cUniqueTrxBegin0 > 0 )
        {
            const ULONG_PTR         centriesToReport    = min( cUniqueTrxBegin0, cTopUsage );
            CArray<CTrxBegin0Usage> rgTrxBegin0;

            //  copy the red-black tree containing the list
            //  of unique trxBegin0's to an array
            //
            err = ErrPrepareUsageArray<CTrxBegin0Usage, TRX>(
                &rgTrxBegin0,
                rbtTrxBegin0.PnodeRoot(),
                cUniqueTrxBegin0,
                "TrxBegin0" );
            if ( JET_errSuccess != err )
                goto HandleError;

            //  sort the array by descending number of RCE's
            //
            rgTrxBegin0.Sort( CTrxBegin0Usage::CmpFrequency );

            //  report top consumers
            //
            dprintf( "Top %d TrxBegin0 by Number of Entries (out of %d unique values):\n", centriesToReport, cUniqueTrxBegin0 );
            dprintf( "=====================================================================\n" );
            dprintf( "    Entries       Aggregate Size    TrxBegin0\n" );
            dprintf( "    ----------    --------------    ----------\n" );

            for ( ULONG_PTR i = 0; i < centriesToReport; i++ )
            {
                const CTrxBegin0Usage * pentry  = rgTrxBegin0.PEntry( i );
                dprintf(
                    "    %10d    %14I64d    0x%08x\n",
                    pentry->CEntries(),
                    pentry->CbAggregateSize(),
                    pentry->TrxBegin0() );
            }
            dprintf( "\n\n" );

            //  sort the array by descending aggregate RCE size
            //
            rgTrxBegin0.Sort( CTrxBegin0Usage::CmpSize );

            //  report top consumers
            //
            dprintf( "Top %d TrxBegin0 by Aggregate Size (out of %d unique values):\n", centriesToReport, cUniqueTrxBegin0 );
            dprintf( "==================================================================\n" );
            dprintf( "    Entries       Aggregate Size    TrxBegin0\n" );
            dprintf( "    ----------    --------------    ----------\n" );

            for ( ULONG_PTR i = 0; i < centriesToReport; i++ )
            {
                const CTrxBegin0Usage * pentry  = rgTrxBegin0.PEntry( i );
                dprintf(
                    "    %10d    %14I64d    0x%08x\n",
                    pentry->CEntries(),
                    pentry->CbAggregateSize(),
                    pentry->TrxBegin0() );
            }
            dprintf( "\n\n" );
        }

        //  report top usage by unique trxBegin0+pgnoFDP
        //
        if ( cUniqueTrxBegin0AndPgnoFDP > 0 )
        {
            const ULONG_PTR                     centriesToReport    = min( cUniqueTrxBegin0AndPgnoFDP, cTopUsage );
            CArray<CTrxBegin0AndPgnoFDPUsage>   rgTrxBegin0AndPgnoFDP;

            //  copy the red-black tree containing the list
            //  of unique trxBegin0+pgnoFDP entries to an array
            //
            err = ErrPrepareUsageArray<CTrxBegin0AndPgnoFDPUsage, TrxBegin0AndPgnoFDP>(
                &rgTrxBegin0AndPgnoFDP,
                rbtTrxBegin0AndPgnoFDP.PnodeRoot(),
                cUniqueTrxBegin0AndPgnoFDP,
                "TrxBegin0+PgnoFDP" );
            if ( JET_errSuccess != err )
                goto HandleError;

            //  sort the array by descending number of RCE's
            //
            rgTrxBegin0AndPgnoFDP.Sort( CTrxBegin0AndPgnoFDPUsage::CmpFrequency );

            //  report top consumers
            //
            dprintf( "Top %d TrxBegin0+PgnoFDP by Number of Entries (out of %d unique values):\n", centriesToReport, cUniqueTrxBegin0AndPgnoFDP );
            dprintf( "=============================================================================\n" );
            dprintf( "    Entries       Aggregate Size    TrxBegin0     Ifmp PgnoFDP       B-Tree Name                                 B-Tree Type    FCB*\n" );
            dprintf( "    ----------    --------------    ----------    ---- ----------    ----------------------------------------    -----------    ------------------\n" );

            for ( ULONG_PTR i = 0; i < centriesToReport; i++ )
            {
                const CTrxBegin0AndPgnoFDPUsage *   pentry  = rgTrxBegin0AndPgnoFDP.PEntry( i );
                dprintf(
                    "    %10d    %14I64d    0x%08x    0x%02x 0x%08x    ",
                    pentry->CEntries(),
                    pentry->CbAggregateSize(),
                    pentry->TrxBegin0(),
                    pentry->IfmpPgnoFDP().ifmp,
                    pentry->IfmpPgnoFDP().pgno );
                ReportPgnoFDPInfo( pentry->IfmpPgnoFDP(), &rbtPgnoFDPToFCB, fRecovering );
            }
            dprintf( "\n\n" );

            //  sort the array by descending aggregate RCE size
            //
            rgTrxBegin0AndPgnoFDP.Sort( CTrxBegin0AndPgnoFDPUsage::CmpSize );

            //  report top consumers
            //
            dprintf( "Top %d TrxBegin0+PgnoFDP by Aggregate Size (out of %d unique values):\n", centriesToReport, cUniqueTrxBegin0AndPgnoFDP );
            dprintf( "==========================================================================\n" );
            dprintf( "    Entries       Aggregate Size    TrxBegin0     Ifmp PgnoFDP       B-Tree Name                                 B-Tree Type    FCB*\n" );
            dprintf( "    ----------    --------------    ----------    ---- ----------    ----------------------------------------    -----------    ------------------\n" );

            for ( ULONG_PTR i = 0; i < centriesToReport; i++ )
            {
                const CTrxBegin0AndPgnoFDPUsage *   pentry  = rgTrxBegin0AndPgnoFDP.PEntry( i );
                dprintf(
                    "    %10d    %14I64d    0x%08x    0x%02x 0x%08x    ",
                    pentry->CEntries(),
                    pentry->CbAggregateSize(),
                    pentry->TrxBegin0(),
                    pentry->IfmpPgnoFDP().ifmp,
                    pentry->IfmpPgnoFDP().pgno );
                ReportPgnoFDPInfo( pentry->IfmpPgnoFDP(), &rbtPgnoFDPToFCB, fRecovering );
            }
            dprintf( "\n\n" );
        }
    }
    else
    {
        dprintf( "Error: Could not read some/all version store variables.\n" );
    }

HandleError:
    dprintf( "\n--------------------\n\n" );

    Unfetch( plog );
    Unfetch( pver );
    Unfetch( ppibTrxOldest );
    Unfetch( rgpls );
    Unfetch( pinst );
}

//  ================================================================
VOID EDBGVerHashSum( INST * pinstDebuggee, BOOL fVerbose )
//  ================================================================
{
    INST *      pinst                   = NULL;
    VER *       pver                    = NULL;
    VER::RCEHEAD *  prcehead            = NULL;
    STAT::CPerfectHistogramStats    histoChains;

    dprintf( "\n" );

    if ( !FFetchVariable( pinstDebuggee, &pinst ) ||
            !FFetchVariable( pinst->m_pver, &pver ) )
    {
        dprintf( "Failed to get INST %N or VER %N\n", pinstDebuggee, pinst ? pinst->m_pver : NULL );
        goto HandleError;
    }

    dprintf( "Scanning verstore hash for distribution...\n" );
    INT             crcechainsNonNull = 0;
    INT             crcechainsEntries = 0;
    INT             crcechainsEntriesCheck = 0;
    INT             crces = 0;

    if ( !FFetchVariable( pinst->m_pver->m_rgrceheadHashTable, &prcehead, pver->m_crceheadHashTable ) )
    {
        dprintf( "Failed to get m_rgrceheadHashTable %N for %d entries\n", pinst->m_pver->m_rgrceheadHashTable, pver->m_crceheadHashTable );
        goto HandleError;
    }
    dprintf( "  m_rgrceheadHashTable = %N\n", pinst->m_pver->m_rgrceheadHashTable );
    dprintf( "  m_crceheadHashTable = %d\n", pver->m_crceheadHashTable );
    for( INT ircehead = 0; (size_t)ircehead < pver->m_crceheadHashTable; ircehead++ )
    {
        VER::RCEHEAD * prceheadCurr = &(prcehead[ircehead]);

        if ( fVerbose )
        {
            dprintf( "    m_rgrceheadHashTable[%04d].prceChain = %p", ircehead, prceheadCurr->prceChain );
        }

        if ( prceheadCurr->prceChain )
        {
            crcechainsNonNull++;
        }


        INT chashOverflow = 0;
        RCE * prceCurr = NULL;
        for( RCE * prceCurrDebuggee = prceheadCurr->prceChain; prceCurrDebuggee; prceCurrDebuggee = prceCurr->m_prceHashOverflow )
        {
            AssertEDBG( prceCurrDebuggee );

            crcechainsEntries++;
            chashOverflow++;

            if ( prceCurr )
            {
                Unfetch( prceCurr );
            }
            if ( !FFetchVariable( prceCurrDebuggee, &prceCurr ) )
            {
                dprintf( "Failed to get RCE %N\n ", prceCurrDebuggee );
                goto HandleError;
            }


            AssertEDBG( prceCurr->UiHash() == ( ircehead % pver->m_crceheadHashTable ) );
            INT cnodeRCEs = 0;
            RCE * prceCurrNode = NULL;
            for ( RCE * prceCurrNodeDebuggee = prceCurrDebuggee; prceCurrNodeDebuggee; prceCurrNodeDebuggee = prceCurrNode->m_prcePrevOfNode )
            {
                cnodeRCEs++;
                crces++;

                if ( prceCurrNode )
                {
                    Unfetch( prceCurrNode );
                }
                if ( !FFetchVariable( prceCurrNodeDebuggee, &prceCurrNode ) )
                {
                    dprintf( "Failed to fetch the RCE %N\n", prceCurrNodeDebuggee );
                    goto HandleError;
                }
            }
            if ( prceCurrNode )
            {
                Unfetch( prceCurrNode );
            }

            if ( fVerbose )
            {
                dprintf (", %d", cnodeRCEs );
            }

        }
        if ( prceCurr )
        {
            Unfetch( prceCurr );
        }

        if ( fVerbose )
        {
            dprintf( ", chashOverflow = %d", chashOverflow );
        }

        crcechainsEntriesCheck += chashOverflow;
        const CPerfectHistogramStats::ERR errStats = histoChains.ErrAddSample( chashOverflow );
        if ( CPerfectHistogramStats::ERR::errOutOfMemory == errStats )
        {
            Pdls()->AddWarning( "WARNING: Out of memory trying to add stat value during verhashsum.\n" );
            goto HandleError;
        }
        else if ( errStats != CPerfectHistogramStats::ERR::errSuccess )
        {
            Pdls()->AddWarning( "WARNING: Could not add stat value during verhashsum.\n" );
            //ErrERRCheck( JET_errInternalError );
            goto HandleError;
        }

        if ( fVerbose )
        {
            dprintf( "\n" );
        }
    }

    AssertEDBG( crcechainsEntries == crcechainsEntriesCheck );
    AssertEDBG( crcechainsEntries == histoChains.Total() );

    dprintf( "  crcechainsNonNull = %d\n", crcechainsNonNull );
    dprintf( "  crcechainsEntries = %d\n", crcechainsEntries );
    dprintf( "  crces = %d\n", crces );
    
    dprintf( "  histoChains:\n" );
    IQPrintStats( &histoChains, UlongPrintVal /* or DWORDPrintVal? */, 0, 4, 0, true );
    dprintf( "\n" );

HandleError:
    Unfetch( prcehead );

    Unfetch( pver );
    Unfetch( pinst );
}

//  ================================================================
DEBUG_EXT( EDBGVerHashSum )
//  ================================================================
{
    INST *      pinstDebuggee           = NULL;

    if ( argc != 1 && argc != 2 )
    {
        dprintf( "Usage: VERHASHSUM <instance> [verbose]\n" );
        dprintf( "\n" );
        dprintf( "       <instance> is the pointer to the instance\n" );
        dprintf( "       for which to dump version store hash summary stats.\n" );
        dprintf( "       [verbose] will print out info on all of the hash\n" );
        dprintf( "       bucket values and entries.  This can print up to\n" );
        dprintf( "       4000 lines (today, may change in future).\n" );
        return;
    }

    if ( !FAddressFromSz( argv[0], &pinstDebuggee ) )
    {
        dprintf( "Expects pinst for 1st arg.\n" );
        return;
    }

    const BOOL fVerbose = ( 0 == _stricmp( argv[0], "verbose" ) ) || ( argc == 2 && 0 == _stricmp( argv[1], "verbose" ) );

    EDBGVerHashSum( pinstDebuggee, fVerbose );

    return;
}


//  ================================================================
LOCAL VOID EDBGPrintResMemUsage( const CHAR * szBucketSpec, DWORD_PTR dwObjectsInUse, __int64 cbCommitted, __int64 cbUsed, DWORD_PTR dwObjectSize )
//  ================================================================
{
    // All sizes take the form of this, which does a good job of showing big numbers, into the multi-GB range, and
    // conveys small numbers, down to 1 KB granularity (below which is often not of interest).
    //
    //  Tables (FCB):
    //    Objects in use:           0
    //    MBs committed:       72.312
    //    MBs used:            12.024   (ave=320 bytes)
    //  Key buffers (RESKEY):
    //    Objects in use:           0
    //    MBs committed:        0.062
    //    MBs used:             0.000   (ave=2016 bytes)
    //

    const BOOL fOld = fFalse;
    if ( fOld )
    {
        dprintf( "  %hs:\n", szBucketSpec );

        dprintf( "    Objects in use:  %14d\n", dwObjectsInUse );

        dprintf( "    MBs committed:   %10d.%03d\n",
                        DwMBs( cbCommitted ),
                        DwMBsFracKB( cbCommitted ) );

        dprintf( "    MBs used:        %10d.%03d   (ave=%d bytes)\n",
                        DwMBs( cbUsed ),
                        DwMBsFracKB( cbUsed ),
                        dwObjectSize );
    }
    else if ( szBucketSpec == NULL )
    {
        dprintf( "   Bucket (TYPE)                :  ObjInUse       Committed           Used   PerObject \n" );
        dprintf( "                                :   (count)       (MBs.KBs)       (MBs.KBs)  (bytes) \n" );
    }
    else
    {        
        dprintf( "  %-30hs:  %7d  %10d.%03d  %10d.%03d   %d bytes\n",
                 szBucketSpec,
                 dwObjectsInUse,
                 DwMBs( cbCommitted ), DwMBsFracKB( cbCommitted ),
                 DwMBs( cbUsed ), DwMBsFracKB( cbUsed ),
                 dwObjectSize );
    }
}

//  ================================================================
LOCAL VOID EDBGPrintResMemUsage( const CHAR * szBucketSpec, JET_RESID resid, DWORD_PTR dwObjectsInUse )
//  ================================================================
{
    CResourceManager    *prm;
    FetchWrap<CResourceManager *> prmFetch;
    DWORD_PTR dwChunksUse = 0;
    DWORD_PTR dwChunkSize = 0;
    DWORD_PTR dwAlign = 0;
    DWORD_PTR dwObjectSize = 1;

    prm = CRMContainer::EDBGPRMFind( resid );

    if ( NULL != prm && prmFetch.FVariable( prm ) )
    {
        prmFetch->ErrGetParam( JET_resoperCurrentUse, &dwChunksUse );
        prmFetch->ErrGetParam( JET_resoperChunkSize, &dwChunkSize );
        prmFetch->ErrGetParam( JET_resoperAlign, &dwAlign );
        prmFetch->ErrGetParam( JET_resoperSize, &dwObjectSize );
        dwObjectSize = ( dwObjectSize + dwAlign - 1 )/dwAlign*dwAlign;

        EDBGPrintResMemUsage( szBucketSpec, dwObjectsInUse, CbBuffSize( dwChunksUse, dwChunkSize ), CbBuffSize( dwObjectsInUse, dwObjectSize ), dwObjectSize );
    }
}

ERR ErrSumTDB( FCB * const pfcbHead, ULONG_PTR * pcFCBsOnInsts, ULONG_PTR * pcTDBsOnFCBs, ULONG_PTR * pcbTDBMemPools )
{
    FCB * pfcbCurrDebuggee = pfcbHead;

    BYTE rgbFcb[sizeof(FCB) + 8 /* to ensure no unaligned access issues, see rounding next */];     // no default .ctor, cheat ...
    BYTE rgbTdb[sizeof(TDB) + 8 /* to ensure no unaligned access issues, see rounding next */];     // no default .ctor, cheat ...
    FCB * pfcbCurr = (FCB*)roundup( (__int64)rgbFcb, 8 );
    TDB * ptdbCurr = (TDB*)roundup( (__int64)rgbTdb, 8 );

    while( pfcbCurrDebuggee )
    {
        if ( !FReadVariable( pfcbCurrDebuggee, pfcbCurr ) )
        {
            dprintf( "We couldn't read %p pfcbCurr\n", pfcbCurr );
            break;
        }
        (*pcFCBsOnInsts)++;
        if ( pfcbCurr->Ptdb() )
        {
            if ( !FReadVariable( pfcbCurr->Ptdb(), ptdbCurr ) )
            {
                dprintf( "We couldn't read %p pfcbCurr->ptdb\n", pfcbCurr->Ptdb() );
                break;
            }
            (*pcTDBsOnFCBs)++;
            (*pcbTDBMemPools) += ( ptdbCurr->MemPool().CbBufSize() );
        }

        //  increment to next ...
        pfcbCurrDebuggee = pfcbCurr->PfcbNextList();
    }

    if ( pfcbCurr != NULL )
    {
        //  something went wrong, we didn't exhaust the list
        return ErrERRCheck( JET_errOutOfMemory );
    }

    return JET_errSuccess;
}


//  ================================================================
LOCAL VOID EDBGUpdateFlushMapMemoryUsage(
    const CFlushMap* const pfm,
    DWORD* const pcbfmdReserved,
    DWORD* const pcbfmdCommitted,
    DWORD* const pcbfmAllocated )
//  ================================================================
{
    *pcbfmdReserved += pfm->m_cbfmdReserved;
    *pcbfmdCommitted += pfm->m_cbfmdCommitted;
    *pcbfmAllocated += pfm->m_cbfmAllocated;

    if ( pfm->m_fmdFmHdr.pv != NULL )
    {
        *pcbfmAllocated += pfm->m_cbFmPageInMemory;
    }

    if ( pfm->m_fPersisted )
    {
        *pcbfmAllocated += ( pfm->m_cfmpgAllocated * pfm->m_cbRuntimeBitmapPage );
    }
}


//  ================================================================
DEBUG_EXT( EDBGMemory )
//  ================================================================
{
    DWORD           cAllocHeapT;
    DWORD           cFreeHeapT;
    DWORD           cbAllocHeapT;
    DWORD_PTR       cbReservePageT;
    DWORD_PTR       cbCommitPageT;
    size_t          cparam              = NULL;
    CJetParam*      rgparam             = NULL;
    LONG_PTR        cbfCacheAddressableT;
    LONG_PTR        cbfCacheSizeT;
    LONG_PTR        cbfCacheTargetT;
    LONG_PTR        cbfCacheTargetOptimalT;
    LONG_PTR        cbfInitT;
    ULONG_PTR       cbCacheSizeCommitted;
    LONG *          rgcbfCachePages = NULL;
    CSmallLookasideCache *  pBFAllocLookasideListDebuggee = NULL;
    CSmallLookasideCache *  pBFAllocLookasideList = NULL;
    SIZE_T          cbHintCacheT;
    SIZE_T          maskHintCacheT;

    dprintf( "\n" );

    if ( !FReadGlobal( "g_cAllocHeap", &cAllocHeapT )
        || !FReadGlobal( "g_cFreeHeap", &cFreeHeapT )
        || !FReadGlobal( "g_cbAllocHeap", &cbAllocHeapT )
        || !FReadGlobal( "g_cbReservePage", &cbReservePageT )
        || !FReadGlobal( "g_cbCommitPage", &cbCommitPageT )
        || !FFetchGlobalParamsArray( &rgparam, &cparam )
        || !FReadGlobal( "cbfCacheAddressable", &cbfCacheAddressableT )
        || !FReadGlobal( "cbfCacheSize", &cbfCacheSizeT )
        || !FReadGlobal( "cbfCacheTarget", &cbfCacheTargetT )
        || !FReadGlobal( "g_cbfCacheTargetOptimal", &cbfCacheTargetOptimalT )
        || !FReadGlobal( "cbfInit", &cbfInitT )
        || !FReadGlobal( "g_cbCacheCommittedSize", &cbCacheSizeCommitted )
        || !FFetchGlobal( "g_rgcbfCachePages", &rgcbfCachePages, icbPageMax )
        || !FReadGlobal( "g_pBFAllocLookasideList", &pBFAllocLookasideListDebuggee )
        || !FFetchVariable( pBFAllocLookasideListDebuggee, &pBFAllocLookasideList )
        || !FReadGlobal( "CPAGE::cbHintCache", &cbHintCacheT )
        || !FReadGlobal( "CPAGE::maskHintCache", &maskHintCacheT ) )
    {
        dprintf( "Error: Could not read some/all memory-related variables.\n" );
        return;
    }

    Assert( Pdls()->Cinst() < cMaxInstances );

    if ( Pdls()->ErrBFInitCacheMap() < JET_errSuccess )
    {
        dprintf( "Error: Could not init some BF cache map related variables.\n" );
        return;
    }

    dprintf( "Heap Usage\n" );
    dprintf( "----------\n" );
    dprintf( "  Bytes currently allocated: %d (0x%0*I64x)\n", cbAllocHeapT, sizeof(DWORD_PTR)*2, QWORD( cbAllocHeapT ) );
    dprintf( "  Current allocations: %d (0x%0*I64x)\n", cAllocHeapT - cFreeHeapT, sizeof(DWORD_PTR)*2, QWORD( cAllocHeapT - cFreeHeapT ) );
    dprintf( "  Total allocations (life of instance): %d (0x%0*I64x)\n", cAllocHeapT, sizeof(DWORD_PTR)*2, QWORD( cAllocHeapT ) );
    dprintf( "  Total de-allocations (life of instance): %d (0x%0*I64x)\n", cFreeHeapT, sizeof(DWORD_PTR)*2, QWORD( cFreeHeapT ) );
    dprintf( "\n" );

    dprintf( "Virtual Address Space Usage\n" );
    dprintf( "---------------------------\n" );
    // All sizes take the form of this, which does a good job of showing big numbers, into the multi-GB range, and
    // conveys small numbers, down to 1 KB granularity (below which is often not of interest).
    //
    //  Tables (FCB):
    //    Objects in use:               0
    //    MBs committed:           72.312
    //    MBs used:                12.024   (ave=320 bytes)
    //  Key buffers (RESKEY):
    //    Objects in use:               0
    //    MBs committed:            0.062
    //    MBs used:                 0.000   (ave=2016 bytes)
    //
    dprintf( " (All sizes in \"MBs.KBs\" - unless otherwise noted)\n" );
    dprintf( "\n" );

    dprintf( "  MBs reserved:       %13d.%03d (currently innaccurate)\n", DwMBs( cbReservePageT ), DwMBsFracKB( cbReservePageT ) );
    dprintf( "  MBs committed:      %13d.%03d (currently innaccurate)\n", DwMBs( cbCommitPageT ), DwMBsFracKB( cbCommitPageT ) );
    dprintf( "\n" );


    //  break down virtual address space usage
    //  into its major consumers

    ULONG_PTR   cFCBUsed                = 0;
    ULONG_PTR   cTDBUsed                = 0;
    ULONG_PTR   cIDBUsed                = 0;
    ULONG_PTR   cCursorsUsed            = 0;
    ULONG_PTR   cSessionsUsed           = 0;
    ULONG_PTR   cSortsUsed              = 0;
    ULONG_PTR   cVERBucketsUsed         = 0;

    ULONG_PTR   cFCBsOnInsts            = 0;
    ULONG_PTR   cTDBsOnFCBs             = 0;
    ULONG_PTR   cbTDBMemPools           = 0;

    //  sum up individual instance totals to
    //  yield totals for the process

    for ( ULONG ipinst = 0; ipinst < Pdls()->Cinst(); ipinst++ )
    {
        if ( pinstNil == Pdls()->PinstDebuggee( ipinst ) )
            continue;

        FetchWrap<INST *>       pinst;
        FetchWrap<VER *>        pver;

        if ( pinst.FVariable( Pdls()->PinstDebuggee( ipinst ) )
            && pver.FVariable( pinst->m_pver ) )
        {
            DWORD_PTR dwT;
            if ( JET_errSuccess == pinst->m_cresFCB.ErrGetParam( JET_resoperCurrentUse, &dwT ) )
                { cFCBUsed += dwT; }
            if ( JET_errSuccess == pinst->m_cresTDB.ErrGetParam( JET_resoperCurrentUse, &dwT ) )
                { cTDBUsed += dwT; }
            if ( JET_errSuccess == pinst->m_cresIDB.ErrGetParam( JET_resoperCurrentUse, &dwT ) )
                { cIDBUsed += dwT; }
            if ( JET_errSuccess == pinst->m_cresSCB.ErrGetParam( JET_resoperCurrentUse, &dwT ) )
                { cSortsUsed += dwT; }
            if ( JET_errSuccess == pinst->m_cresFUCB.ErrGetParam( JET_resoperCurrentUse, &dwT ) )
                { cCursorsUsed += dwT; }
            if ( JET_errSuccess == pinst->m_cresPIB.ErrGetParam( JET_resoperCurrentUse, &dwT ) )
                { cSessionsUsed += dwT; }
            if ( JET_errSuccess == pver->m_cresBucket.ErrGetParam( JET_resoperCurrentUse, &dwT ) )
                { cVERBucketsUsed += dwT; }

            if ( pinst->m_pfcbList )
            {
                ErrSumTDB( pinst->m_pfcbList, &cFCBsOnInsts, &cTDBsOnFCBs, &cbTDBMemPools );
            }
        }
        else
        {
            dprintf( "Error: Could not read INST information. Skipped an instance.\n" );
        }
    }

    //  sum up some buffer manager totals

    ICBPage icbPage = IcbBFIPageSize( (INT)CbEDBGILoadSYSMaxPageSize_() );

    __int64 cbCacheSize = 0;
    __int64 cbCacheEffectiveSize = 0;
    __int64 cbCacheFragmentedSize = 0;
    for( LONG icb = 0; icb < icbPageMax; icb++ )
    {
        if ( rgcbfCachePages[icb] )
        {
            Assert( icb <= icbPage );
            cbCacheEffectiveSize += CbBuffSize( g_rgcbPageSize[icbPage], rgcbfCachePages[icb] );
            cbCacheSize += CbBuffSize( g_rgcbPageSize[icb], rgcbfCachePages[icb] );
            if ( icb < icbPage )
            {
                cbCacheFragmentedSize += CbBuffSize( ( g_rgcbPageSize[icbPage] - g_rgcbPageSize[icb] ), rgcbfCachePages[icb] );
            }
        }
    }

    //  flush map related memory

    DWORD cbfmdReserved = 0;
    DWORD cbfmdCommitted = 0;
    DWORD cbfmAllocated = 0;
    DWORD_PTR cFMPs = 0;
    __int64 cbFmpCommitted = roundup( ( Pdls()->CfmpAllocated() - cfmpReserved ) * sizeof( FMP ), OSMemoryPageCommitGranularity() );
    IFMP ifmpMic = 0;
    IFMP ifmpMac = 0;

    for ( IFMP ifmp = cfmpReserved; ifmp < Pdls()->CfmpAllocated(); ifmp++ )
    {
        const FMP* const pfmp = Pdls()->PfmpCache( ifmp );
        if ( pfmp == NULL )
        {
            dprintf( "Warning: Could not fetch FMP from the Pdls() FMP cache? Skipping ifmp %d.\n", ifmp );
            continue;
        }
        if ( !pfmp->FInUse() )
        {
            continue;
        }

        cFMPs++;
        if ( ifmpMic == 0 )
        {
            ifmpMic = ifmp;
        }
        ifmpMac = ifmp;

        //AssertEDBG( pfmp->Dbid() != dbidTemp ); // - not sure why anyone thought this would hold.
        if ( pfmp->Dbid() == dbidTemp )
        {
            AssertEDBG( pfmp->PFlushMap() == NULL ); // - try this assert instead.
            continue;
        }
        

        if ( pfmp->PFlushMap() != NULL )
        {
            const CFlushMapForAttachedDb* pfm = NULL;
            if ( FFetchVariable( (CFlushMapForAttachedDb*)pfmp->PFlushMap(), (CFlushMapForAttachedDb**)&pfm ) )
            {
                EDBGUpdateFlushMapMemoryUsage( pfm, &cbfmdReserved, &cbfmdCommitted, &cbfmAllocated );
                Unfetch( pfm );
            }
            else
            {
                dprintf( "Warning: Could not fetch CFlushMapForAttachedDb for ifmp %I64u.\n", (QWORD)ifmp );
            }
        }
    }
    __int64 cbFmpUsed = ( ( ifmpMac - ifmpMic + 1 ) * sizeof( FMP ) );

    dprintf( "  Buffer Manager:\n" );
    dprintf( "    Current addressable buffers:          %17Id\n", cbfCacheAddressableT );
    dprintf( "    Current quiesced buffers:             %17Id\n", cbfCacheAddressableT - cbfCacheSizeT );
    dprintf( "    Current unquiesced buffers:           %17Id\n", cbfCacheSizeT );
    dprintf( "    Target cache buffers:                 %17Id\n", cbfCacheTargetT );
    dprintf( "    Target cache buffers (optimal):       %17Id\n", cbfCacheTargetOptimalT );
    const __int64 cbCacheStatus = roundup( CbBuffSize( cbfInitT, sizeof(BF) ), OSMemoryPageCommitGranularity() );
    dprintf( "    Cache status overhead:                %13d.%03d   (cbfInit = %d)\n", DwMBs( cbCacheStatus ), DwMBsFracKB( cbCacheStatus ), cbfInitT );
    dprintf( "    Cache size reserved:                  %13d.%03d   (chunks = %d/%d)\n", DwMBs( Pdls()->CbCacheReserve() ), DwMBsFracKB( Pdls()->CbCacheReserve() ), Pdls()->CCacheChunks(), cCacheChunkMax );
    dprintf( "    Cache size committed:                 %13d.%03d\n", DwMBs( cbCacheSizeCommitted ), DwMBsFracKB( cbCacheSizeCommitted ) );
    dprintf( "    Cache size de-committed:              %13d.%03d\n", DwMBs( cbCacheFragmentedSize ), DwMBsFracKB( cbCacheFragmentedSize ) );
    dprintf( "    Cache size (cached pages):            %13d.%03d\n", DwMBs( cbCacheSize ), DwMBsFracKB( cbCacheSize ) );
    dprintf( "    Cache size logical (cached pages):    %13d.%03d\n", DwMBs( cbCacheEffectiveSize ), DwMBsFracKB( cbCacheEffectiveSize ) );
    dprintf( "    Lookaside cache avail:                %13d.%03d\n", DwMBs( pBFAllocLookasideList->CbCacheSize()  ), DwMBsFracKB( pBFAllocLookasideList->CbCacheSize() ) );
    dprintf( "    Hint cache size:                      %13d.%03d\n", DwMBs( cbHintCacheT ), DwMBsFracKB( cbHintCacheT ) );
    dprintf( "    Hint cache in use:                    %13d.%03d\n", DwMBs( ( maskHintCacheT + 1 ) * sizeof(DWORD_PTR) ), DwMBsFracKB( ( maskHintCacheT + 1 ) * sizeof(DWORD_PTR) ) );
    dprintf( "    Flush map descriptors (reserved):     %17d KB\n", DwKBs( cbfmdReserved ) );
    dprintf( "    Flush map descriptors (committed):    %17d KB\n", DwKBs( cbfmdCommitted ) );
    dprintf( "    Flush map pages:                      %17d KB\n", DwKBs( cbfmAllocated ) );
    dprintf( "\n" );

    // Prints headers
    EDBGPrintResMemUsage( NULL, 0, 0, 0, 0 );

    EDBGPrintResMemUsage( "Version Store", JET_residVERBUCKET, cVERBucketsUsed );
    dprintf( "\n" );

    EDBGPrintResMemUsage( "Tables (FCB)", JET_residFCB, cFCBUsed );
    EDBGPrintResMemUsage( "Tables (TDB)", JET_residTDB, cTDBUsed );
    EDBGPrintResMemUsage( "TDB.MemPools", cTDBsOnFCBs, cbTDBMemPools, cbTDBMemPools, cTDBsOnFCBs ? ( cbTDBMemPools / cTDBsOnFCBs ) : 0  ); // faked for TDBs\MemPools
    EDBGPrintResMemUsage( "Indices (IDB)", JET_residIDB, cIDBUsed );
    EDBGPrintResMemUsage( "Sorts+Temp Tables (SCB)", JET_residSCB, cSortsUsed );
    EDBGPrintResMemUsage( "Cursors (FUCB)", JET_residFUCB, cCursorsUsed );
    EDBGPrintResMemUsage( "Sessions (PIB)", JET_residPIB, cSessionsUsed );
    EDBGPrintResMemUsage( "Databases (FMP)", cFMPs, cbFmpCommitted, cbFmpUsed, sizeof( FMP ) );

    FetchWrap<CResource *> pcres;

    struct DUMPCRESUSE
    {
        const JET_RESID  resid;
        const char *     szName;
        const char *     szDescription;
    };

    DUMPCRESUSE rgcresuse[] =
    {
            { JET_residINST,       "RESINST", "INST" },
            { JET_residLOG,        "RESLOG", "LOG" },
            { JET_residSPLIT,      "RESSPLIT", "SPLIT" },
            { JET_residSPLITPATH,  "RESSPLITPATH", "SPLITPATH" },
            { JET_residMERGE,      "RESMERGE", "MERGE" },
            { JET_residMERGEPATH,  "RESMERGEPATH", "MERGEPATH" },
            { JET_residKEY,        "RESKEY", "Key buffs" },
            { JET_residBOOKMARK,   "RESBOOKMARK", "Bookmark buffs" },
            { JET_residLRUKHIST,   "RESLRUKHIST", "LRUK hist recs" },
    };

    INT icresuseMax = sizeof( rgcresuse ) / sizeof( rgcresuse[0] );
    INT icresuse;
    for( icresuse = 0; icresuse < icresuseMax; ++icresuse )
    {
        if ( pcres.FGlobal( rgcresuse[icresuse].szName ) )
        {
            CHAR szBucketSpec[100];
            OSStrCbFormatA( szBucketSpec, sizeof( szBucketSpec ), "%s (%s)", rgcresuse[icresuse].szDescription, rgcresuse[icresuse].szName );
            DWORD_PTR dwT;
            pcres->ErrGetParam( JET_resoperCurrentUse, &dwT );
            EDBGPrintResMemUsage( szBucketSpec, rgcresuse[icresuse].resid, dwT );
        }
    }

    dprintf( "\n--------------------\n\n" );

    Unfetch( pBFAllocLookasideList );
    Unfetch( rgcbfCachePages);
    Unfetch( rgparam );
}

BOOL FRetrieveInstance( __in_opt const char * const szInstArg, INST ** ppinstDebuggee, INST ** ppinst, BOOL fPrintParseErrors = fTrue );

//  ================================================================
BOOL FRetrieveInstance( __in_opt const char * const szInstArg, INST ** ppinstDebuggee, INST ** ppinst, BOOL fPrintParseErrors )
//  ================================================================
//  This routine takes szInstArg and retrieves an acceptable instance.  If the szInstArg
//  is a pointer to an instance, then this routine validates the pointer is actually an
//  instance and retrieves the instance.  If szInstArg is NULL, the routine tries to find
//  out if the target process has one and only one instance, and then returns that inst
//  if so.
//
//  Caller will recieve a ppinst from the DLS INST cache and should not free the returned
//  value.
//
//  returning fFalse means failure.  On failure pinstDebuggee may be set, but is not
//  an allocated var, so doesn't matter.
//
//  Use fPrintParseErrors = false (default true) if caller is unsure this is an inst arg
//  and wants to not print anything, except for very odd errors.
//
{
    AssertEDBG( ppinst );
    AssertEDBG( ppinstDebuggee );

    if ( Pdls()->ErrINSTDLSInitCheck() < JET_errSuccess )
    {
        return fFalse;
    }

    if ( szInstArg )
    {
        if ( !FAutoAddressFromSz( szInstArg, ppinstDebuggee ) )
        {
            if ( fPrintParseErrors )
            {
                dprintf( "Parse Error: reading pinst address from arg: %s\n", szInstArg );
            }
            return fFalse;
        }
    }

    ULONG ipinstTarget = cMaxInstances;
    ULONG ipinst = 0;
    for ( ipinst = 0; ipinst < Pdls()->Cinst(); ipinst++ )
    {
        if ( Pdls()->PinstDebuggee( ipinst ) != pinstNil )
        {
            // we have an intance ...
            if ( *ppinstDebuggee != NULL )
            {
                // if user is interested in a specific instance, check if this is it ...
                if ( Pdls()->PinstDebuggee( ipinst ) == *ppinstDebuggee )
                {
                    ipinstTarget = ipinst;
                    break;
                }
            }
            else
            {
                // user wants only one instance 
                if ( ipinstTarget == cMaxInstances )
                {
                    // no instance selected, selec this one ...
                    ipinstTarget = ipinst;
                    // but continue to allow this to fail if there are two instances.
                }
                else
                {
                    ipinstTarget = cMaxInstances;
                    break;
                }
            }
        }
    }


    if ( *ppinstDebuggee == NULL )
    {
        // User wants us to select the instance, but only if there is one and only one inst.
        if ( ipinstTarget == cMaxInstances )
        {
            // unfortunately we couldn't find an instance, b/c ...
            if ( ipinst == cMaxInstances )
            {
                dprintf( "Error selecting instance, no instances available.\n" );
            }
            else
            {
                dprintf( "Error selecting instance, more than one instance exists, can't auto-select instance.\n" );
            }
            return fFalse;
        }

        // we found an single instance.
        *ppinstDebuggee = Pdls()->PinstDebuggee( ipinstTarget );
        
    }
    else
    {
    }

    AssertEDBG( *ppinstDebuggee );

    // User gave us a target inst...
    if ( ipinstTarget == cMaxInstances )
    {
        if ( fPrintParseErrors )
        {
            dprintf( "Provided target %N is not an inst pointer.\n", *ppinstDebuggee );
        }
        return fFalse;
    }

    // we think we have an instance pointer the caller will be happy with ... can we actually read it ...

    if ( ( *ppinst = Pdls()->Pinst( ipinst ) ) == NULL )
    {
        // should this be if fPrintParseErrors...?
        dprintf( "Error retrieving the instance 0x%N from debuggee\n", *ppinstDebuggee );
        return fFalse;
    }
    return fTrue;
}

//  ================================================================
DEBUG_EXT( EDBGParam )
//  ================================================================
{
    size_t          cparam          = NULL;

    CJetParam *     pjpDebuggeeGlobal = NULL;
    CJetParam *     rgparamGlobal   = NULL;

    CJetParam *     pjpDebuggee     = NULL;
    CJetParam *     rgparam         = NULL;

    ULONG           iParam          = 0;
    INT             iParamGlobal    = 0;    // intentionally an int, not ULONG ...

    BOOL            fPrintHelp      = fTrue; // set to false if command is successful.

    const char *    szParamTar      = NULL;

    //  Get global information ...
    if ( !FAddressFromGlobal( "g_rgparam", &pjpDebuggeeGlobal ) ||
         !FFetchGlobalParamsArray( &rgparamGlobal, &cparam ) )
    {
        dprintf(" Couldn't fetch the global g_rgparam data\n");
        goto HandleError;
    }

    //
    //  Process the args, see we can retrieve all the appropriate information.
    //
    if ( argc >= 1 )
    {
        INST * pinstDebuggee = NULL;
        INST * pinst = NULL;
        INT iArgJetParamNames = 1; // assume 2nd arg, set it to 1st arg if 1st arg isn't an inst/rgparam.

        //
        //  Try to detect the arg specifying the param array to dump ...
        //
        // because I'm lazy in the debugger, there is a bit too much heuristic logic here.
        //
        if ( ( 0 == _stricmp( argv[0], "global" ) ||
              0 == _stricmp( argv[0], "g_rgparam" ) ) )
        {
            // They ask for the global params table ... 
            pjpDebuggee = pjpDebuggeeGlobal;
            rgparam = rgparamGlobal;
            dprintf( "\n" );
            dprintf( "Dumping params for g_rgparam = 0x%N\n", pjpDebuggee );
            dprintf( "\n" );
        }
        else if ( FRetrieveInstance( argv[0], &pinstDebuggee, &pinst, fFalse ) )
        {
            // We think the first arg is an explicit instance pointer get its params ...

            pjpDebuggee = pinst->m_rgparam;

            if ( pjpDebuggee &&
                 FFetchVariable( pjpDebuggee, &rgparam, cparam ) )
            {
                dprintf( "\n" );
                dprintf( "Dumping params for pinst(0x%N)->m_rgparam = 0x%N\n", pinstDebuggee, pjpDebuggee );
                dprintf( "\n" );
            }
            else
            {
                dprintf( "Error fetching m_rgparam 0x%N from pinst 0x%N\n", pjpDebuggee, pinstDebuggee );
                goto HandleError;
            }
        }
        else if ( FAddressFromSz( argv[0], &pjpDebuggee ) )
        {
            // We think the first arg is an rgparam pointer ...
            AssertEDBG( pjpDebuggee );
            if ( FFetchVariable( pjpDebuggee, &rgparam, cparam ) )
            {
                dprintf( "\n" );
                dprintf( "Dumping params for explicit param array, rgparam = 0x%N\n", pjpDebuggee );
                dprintf( "\n" );
            }
            else
            {
                dprintf( "Error fetching m_rgparam 0x%N\n", pjpDebuggee );
                goto HandleError;
            }
        }
        else
        {
            // The first arg wasn't and instance, assume it was a param name substring arg
            iArgJetParamNames = 0;
        }

        //
        // See if we have a substring arg for the params names we want to dump out ...
        //
        if ( argc >= ( iArgJetParamNames + 1 ) )
        {
            szParamTar = argv[iArgJetParamNames];
        }
    }

    // if no rgparam debuggee target at this point, then the arg for rgparam | inst was NULL, so 
    // see if we can auto-select an instance.
    if ( NULL == pjpDebuggee )
    {
        INST * pinstDebuggee = NULL;
        INST * pinst = NULL;

        // See if there is one, and exactly one instance, and if so, select it.
        if ( !FRetrieveInstance( NULL, &pinstDebuggee, &pinst ) )
        {
            dprintf( "No instance is active, you will have to pass a specific rgparam pointer to the param.\n" );
            goto HandleError;
        }

        pjpDebuggee = pinst->m_rgparam;

        if ( pjpDebuggee &&
             FFetchVariable( pjpDebuggee, &rgparam, cparam ) )
        {
            dprintf( "\n" );
            dprintf( "Dumping params for pinst(0x%N)->m_rgparam = 0x%N\n", pinstDebuggee, pjpDebuggee );
            dprintf( "\n" );
            AssertEDBG( rgparam );
        }
        else
        {
            dprintf( "Error fetching m_rgparam 0x%N from pinst 0x%N\n", pjpDebuggee, pinstDebuggee );
            goto HandleError;
        }
    }

    // if no param name substring was specified ...
    if ( szParamTar == NULL )
    {
        // ... this will select / match all _valid_ JET params.
        szParamTar = "JET_param";
    }

    if ( !( pjpDebuggee == NULL && rgparam == NULL ) &&
            !( pjpDebuggee != NULL && rgparam != NULL ) )
    {
        dprintf( " Confused by args, not parsing correctly.\n" );
        goto HandleError;
    }

    //
    //  Fetch the JET parameter names into the debugger.
    //
    for ( iParam = 0; iParam < cparam; iParam++ )
    {
        if ( !FFetchSz( rgparamGlobal[iParam].m_szParamName, &(rgparamGlobal[iParam].m_szParamName) ) )
        {
            break;
        }
        iParamGlobal++;
    }
    if ( iParam != cparam )
    {
        // we failed prematurely to get all the string vars...
        dprintf( "Failed to fetch paramid %d into the debugger, you're on your own.\n", iParam );
        goto HandleError;
    }

    //
    //  Actually dump all the parameters from the selected parameter table.
    //
    AssertEDBG( pjpDebuggee );
    AssertEDBG( rgparam );
    AssertEDBG( szParamTar );

    CJetParam::Config configLegacy = rgparam[JET_paramConfiguration].m_valueCurrent & JET_configDefault;
    if ( rgparam[JET_paramConfiguration].m_valueCurrent & ~JET_configDefault )
    {
        //  if we do have any other config params, warn ...
        dprintf( "WARNING: We have non-base settings, so some of the below claimed defaults may be off!  config = 0x%x\n", rgparam[JET_paramConfiguration].m_valueCurrent );
    }

    for ( iParam = 0; iParam < cparam; iParam++ )
    {
        if ( rgparam[iParam].m_paramid != iParam )
        {
            dprintf( "Uh oh!  The param index doesn't match the paramid!?!?  param table bug.\n" );
        }
        
        // Note: only the global has the m_szParamName filled in, so we check against the
        // global array of rgparams.
        if ( !( szParamTar[0] == '*' && szParamTar[1] == '\0' ) &&
             0 == strhstr( rgparamGlobal[iParam].m_szParamName, szParamTar, fTrue ) )
        {
            continue; // skip this param, doesn't match the hit.
        }

        // if we print at least one param, then help is not needed.
        fPrintHelp = fFalse;

        CHAR * szSetSource = NULL;
        if ( rgparam[iParam].m_fRegOverride )
        {
            szSetSource = "(fRegO)";
        }
        else if ( rgparam[iParam].m_fRegDefault )
        {
            szSetSource = "(fRegD)";
        }
        else if ( rgparam[iParam].m_fWritten )
        {
            // this means explicitly set by app (or possibly ESE) code ... as opposed
            // to just the default value.
            szSetSource = "( fSet)";
        }
        else
        {
            szSetSource = "(     )";
        }

        // Note: only the global has the m_szParamName filled in.
        dprintf( "  [%3d]=%39s  %s ", iParam, rgparamGlobal[iParam].m_szParamName, szSetSource );
        switch ( rgparam[iParam].m_type )
        {
            case JetParam::typeBoolean:
                dprintf( "=%20s  (def=%s)\n",
                            ((BOOL) rgparam[iParam].m_valueCurrent) ? "true" : "false",
                            ((BOOL) rgparam[iParam].m_valueDefault[configLegacy]) ? "true" : "false" );
                break;
            case JetParam::typeBlockSize:
            case JetParam::typeInteger:

#ifdef _X86_
                dprintf( "=%20d  (%d,%d,%d)\n", rgparam[iParam].m_valueCurrent,
#else
                dprintf( "=%20I64d  (%d,%d,%d)\n", rgparam[iParam].m_valueCurrent,
#endif
                    rgparam[iParam].m_rangeLow,
                    rgparam[iParam].m_valueDefault[configLegacy],
                    rgparam[iParam].m_rangeHigh );
                break;
                
            case JetParam::typeString:
            case JetParam::typeFolder:
            case JetParam::typePath:
                WCHAR * wszStrParam;
                if ( rgparam[iParam].m_valueCurrent == NULL )
                {
                    dprintf( "NULL\n" );
                }
                else if ( FFetchSz( (WCHAR*)rgparam[iParam].m_valueCurrent, &wszStrParam ) )
                {
                    dprintf( "=  0x%N =%d:\"%mu\"  ", rgparam[iParam].m_valueCurrent, wcslen(wszStrParam), rgparam[iParam].m_valueCurrent );
                    Unfetch( wszStrParam );
                    dprintf( "(def=\"%mu\")\n", rgparam[iParam].m_valueDefault[configLegacy] );
                }
                else
                {
                    dprintf( "<error, debugger couldn't fetch string>\n" );
                }
                break;
            case JetParam::typeGrbit:
                dprintf( "=%#20x  (%#x,%#x,%#x)\n",
                    rgparam[iParam].m_valueCurrent,
                    rgparam[iParam].m_rangeLow,
                    rgparam[iParam].m_valueDefault[configLegacy],
                    rgparam[iParam].m_rangeHigh );
                break;
            case JetParam::typePointer:
                dprintf( "=  0x%N  (%N)\n",
                    rgparam[iParam].m_valueCurrent,
                    rgparam[iParam].m_valueDefault[configLegacy] );
                break;
            case JetParam::typeUserDefined:
                dprintf( "=         UserDefined\n" );
                break;
            case JetParam::typeNull:
                dprintf( "=            typeNull\n" );
                break;
            default:
                dprintf( "=     <UNKNOWN TYPE!>\n" );
                break;
        }
    }


HandleError:
    if ( fPrintHelp )
    {
        dprintf( "\n" );
        dprintf( "Usage: !ese.ese param [global|<inst>|<rgparam>|.] [substr]\n" );
        dprintf( "if no inst or rgparam address is passed, will try to locate single inst and use that.\n" );
        dprintf( "substr is quasi-case sensitive try, \"!ese.ese param global IO\" and \"!ese.ese param global io\" to see diff.\n" );
    }
    if ( rgparam != rgparamGlobal )
    {
        Unfetch( rgparam );
    }
    if ( rgparamGlobal )
    {
        for ( iParamGlobal-- /* iParamGlobal is count, one too high */; iParamGlobal >= 0; iParamGlobal-- )
        {
            if ( rgparamGlobal[iParamGlobal].m_szParamName != NULL )
            {
                Unfetch( rgparamGlobal[iParamGlobal].m_szParamName );
            }
        }
        Unfetch( rgparamGlobal );
    }
}

LOCAL BOOL FEDBGParsePageSizeOption(
    const CHAR *    pchPageSize,
    size_t *        pcbPage )
{
    if ( NULL != pchPageSize )
    {
        if( pchPageSize[0] == '1' && pchPageSize[1] == '6' )
        {
            *pcbPage = 16 * 1024;
        }
        else if ( pchPageSize[0] == '3' && pchPageSize[1] == '2' )
        {
            *pcbPage = 32 * 1024;
        }
        else if ( '2' == *pchPageSize
                || '4' == *pchPageSize
                || '8' == *pchPageSize )
        {
            *pcbPage = ( *pchPageSize - '0' ) * 1024;
        }
        else
        {
            dprintf( "Error: Could not understand page size option." );
            return fFalse;
        }
    }

    return fTrue;
}

LOCAL BOOL FEDBGParseDumpPageOptions(
    const CHAR *    szOptions,
    size_t *        pcbPage,
    BOOL *          pfDumpHeader,
    BOOL *          pfDumpTags,
    BOOL *          pfDumpAllocMap,
    BOOL *          pfDumpBinary )
{
    const CHAR *    pchPageSize     = NULL;

    if ( NULL != szOptions )
    {
        pchPageSize     = (CHAR *)strpbrk( szOptions, "12348" );
        *pfDumpHeader   = ( strpbrk( szOptions, "hH*" ) != NULL );
        *pfDumpTags     = ( strpbrk( szOptions, "tT*" ) != NULL );
        *pfDumpAllocMap = ( strpbrk( szOptions, "aA*" ) != NULL );
        *pfDumpBinary   = ( strpbrk( szOptions, "bB*" ) != NULL );

        //  default to header dump if no other dumps specified
        //
        if ( !(*pfDumpAllocMap) && !(*pfDumpBinary) && !(*pfDumpTags) )
            *pfDumpHeader = fTrue;
    }
    else
    {
        //  no options specified, so default is to dump header
        //
        *pfDumpHeader = fTrue;
    }

    return FEDBGParsePageSizeOption( pchPageSize, pcbPage );
}


LOCAL BOOL FEDBGGetDbDiskPage(
    const HANDLE    hProcess,
    const HANDLE    hFileDebuggee,
    const PGNO      pgno,
    BYTE *          pbPage,
    const size_t    cbPage )
{
    BOOL            fResult         = fFalse;
    HANDLE          hFileDebugger   = NULL;
    OVERLAPPED      overlapped;
    DWORD           error;
    DWORD           cbRead          = (DWORD)cbPage;
    QWORDX          qwOffset;

    overlapped.hEvent = NULL;

    if ( !DuplicateHandle(
                hProcess,
                hFileDebuggee,
                GetCurrentProcess( ),
                &hFileDebugger,
                GENERIC_READ,
                FALSE,
                0x0 ) )
    {
        error = GetLastError();
        dprintf(
            "Error: Could not duplicate handle 0x%x. Error %d (0x%x).\n",
            hFileDebuggee,
            error,
            error );
        goto HandleError;
    }

    overlapped.hEvent = CreateEventW( NULL, FALSE, TRUE, NULL );
    if ( NULL == overlapped.hEvent )
    {
        error = GetLastError();
        dprintf( "Error: Could not create event. Error %d (0x%x).\n", error, error );
        goto HandleError;
    }
    //  This is required or the _debuggee_'s completion thread will get a completion packet for
    //  this IO request with our / debugger's virtual addresses as the completion context, which 
    //  will then cause it to (best case) crash when it tries to deref it.
    //  Unknown: How is this handled if the file is opened and not registered for completion port 
    //  completions like most (all?) of our files.
    overlapped.hEvent = HANDLE( (DWORD_PTR)overlapped.hEvent | hNoCPEvent );

    qwOffset.SetQw( QWORD( pgno + cpgDBReserved - 1 ) * cbPage );
    overlapped.Offset = qwOffset.DwLow();
    overlapped.OffsetHigh = qwOffset.DwHigh();

    if ( !ReadFile( hFileDebugger, pbPage, cbRead, &cbRead, &overlapped ) )
    {
        error = GetLastError();
        if ( ERROR_IO_PENDING == error )
        {
            error = ( GetOverlappedResult( hFileDebugger, &overlapped, &cbRead, TRUE ) ? ERROR_SUCCESS : GetLastError() );
        }
        if ( ERROR_SUCCESS != error )
        {
            dprintf( "Error: Could not read file. Error 0x%d (0x%x).\n", error, error );
            goto HandleError;
        }
    }

    fResult = fTrue;

HandleError:
    if ( NULL != overlapped.hEvent )
    {
        overlapped.hEvent = HANDLE( (DWORD_PTR)overlapped.hEvent & ~hNoCPEvent );
        CloseHandle( overlapped.hEvent );
    }
    if ( NULL != hFileDebugger )
    {
        CloseHandle( hFileDebugger );
    }

    return fResult;
}


//  ===================================
VOID EDBGDumpRawData(
    CPRINTF *           pcprintf,
    const BYTE * const  pbData,
    const size_t        cbData,
    const BOOL          fShowAddress )
//  ===================================
{
    CHAR *              szBuf   = NULL;
    const size_t        cbWidth = 16;
    const size_t        cbBuf   = ( cbData + cbWidth - 1 ) * 8; //  add some padding to cbData to ensure we can print last chunk
    
    // enough space to convert page into hex
    if ( ((~(size_t)0) / 8) > cbData)
    {
        szBuf = (CHAR *)LocalAlloc( 0, cbBuf );
    }
    
    if ( szBuf )
    {
        DBUTLSprintHex( (CHAR * const)szBuf, cbBuf, pbData, cbData, cbWidth, 4, ( fShowAddress ? 8 : 0 ) );
    
        CHAR *szNextToken = NULL;
        
        for ( CHAR * szLine = strtok_s( (CHAR * const)szBuf, "\n", &szNextToken );
            NULL != szLine;
            szLine = strtok_s( NULL, "\n", &szNextToken ) )
        {
            (*pcprintf)( "%s\n", szLine );
        }
    
        LocalFree( szBuf );
    }
    else
    {
        (*pcprintf)( "Error: Could not allocate %d * 8 bytes for output buffer.\n", cbData );
    }
}


LOCAL VOID EDBGDumpPage(
    const IFMP      ifmp,
    const PGNO      pgno,
    BYTE *          pbPage,
    const size_t    cbBuffer,
    const size_t    cbPage,
    const DWORD_PTR dwOffset,
    const BOOL      fDumpHeader,
    const BOOL      fDumpTags,
    const BOOL      fDumpAllocMap,
    const BOOL      fDumpBinary )
{
    CPAGE           cpage;

    cpage.LoadDehydratedPage( ifmp, pgno, pbPage, cbBuffer, cbPage );

    if ( fDumpHeader )
    {
        (VOID)cpage.DumpHeader( CPRINTFWDBG::PcprintfInstance(), dwOffset );
    }
    if ( fDumpTags )
    {
        (VOID)cpage.DumpTags( CPRINTFWDBG::PcprintfInstance(), dwOffset );
    }
    if ( fDumpAllocMap )
    {
        (VOID)cpage.DumpAllocMap( CPRINTFWDBG::PcprintfInstance() );
        if ( cbBuffer != cbPage )
        {
            AssertEDBG( cbBuffer < cbPage );
            if ( cbBuffer < cbPage )
            {
                dprintf("Note: There is another %u bytes of un-printed empty space(...) due to HyperCache.\n", cbPage - cbBuffer );
            }
        }
    }

    if ( fDumpBinary )
    {
        if ( Pdls()->FPII() )
        {
            (*CPRINTFWDBG::PcprintfInstance())( "Raw dump (cbPage = %d / cbBuffer = %d) in big-endian quartets of bytes (not little endian DWORDs):\n", (LONG)cbPage, (LONG)cbBuffer );
            EDBGDumpRawData( CPRINTFWDBG::PcprintfInstance(), pbPage, cbBuffer, fTrue );
        }
        else
        {
            dprintf( "Dump binary option only allowed if PII is enabled, run: !ese .pii on" );
        }
    }

    cpage.UnloadPage();
}


//  ================================================================
DEBUG_EXT( EDBGDumpDBDiskPage )
//  ================================================================
{
    ULONG       ifmp;
    PGNO        pgno;
    size_t      cbPage          = 0;
    BOOL        fDumpAllocMap   = fFalse;
    BOOL        fDumpBinary     = fFalse;
    BOOL        fDumpHeader     = fFalse;
    BOOL        fDumpTags       = fFalse;
    BYTE *      pbPage          = NULL;
    COSFile *   posf            = NULL;
    HRESULT     hr              = S_OK;
    HANDLE      hCurrentProcess;
    ULONG64     ulCurrentProcess;

    dprintf( "\n" );

    if ( argc < 2
        || argc > 3
        || !FAutoIfmpFromSz( argv[0], &ifmp )
        || !FUlFromSz( argv[1], &pgno ) )
    {
        //  invalid usage
        //
        dprintf( "Usage: DUMPDBDISKPAGE <ifmp|.> <pgno> [a|b|h|t|*|2|4|8|16|32]\n" );
        dprintf( "    <ifmp|.> is the index to the FMP entry for the desired database file\n" );
        dprintf( "    <pgno> is the desired page number from this FMP\n" );
        dprintf( "    [a=alloc map, b=binary dump, h=header, t=tags, *=all, 2/4/8/16/32=pagesize]\n" );
        goto HandleError;
    }
    else if ( !FEDBGParseDumpPageOptions(
                            ( argc < 3 ? NULL : argv[2] ),
                            &cbPage,
                            &fDumpHeader,
                            &fDumpTags,
                            &fDumpAllocMap,
                            &fDumpBinary ) )
    {
        goto HandleError;
    }

    if ( ifmp > Pdls()->CfmpAllocated() )
    {
        dprintf( "Error: Invalid ifmp = %d.\n", ifmp );
    }
    else if ( Pdls()->PfmpCache( ifmp ) == NULL )
    {
        dprintf( "Error: Could not read global FMP variables for ifmp = %d.\n", ifmp );
        goto HandleError;
    }
    else if ( pgno < 1 )        //  UNDONE: don't currently support dumping page header
    {
        dprintf( "Error: Invalid pgno.\n" );
        goto HandleError;
    }
    if ( cbPage == 0 )
    {
        //  Auto loading cbPage based upon ifmp.
        cbPage = Pdls()->CbPage( ifmp );
    }

    //  UNDONE: currently assumes all databases are COSFile
    //
    if ( !FFetchVariable( (COSFile *)( Pdls()->PfmpCache( ifmp ) )->Pfapi(), &posf ) )
    {
        dprintf( "Error: Could not read COSFile at 0x%N for specified FMP.\n", ( Pdls()->PfmpCache( ifmp ) )->Pfapi() );
        goto HandleError;
    }

    //  VirtualAlloc() the buffer to ensure alignment
    //
    pbPage = (BYTE *)VirtualAlloc( NULL, cbPage, MEM_COMMIT, PAGE_READWRITE );
    if ( NULL == pbPage )
    {
        dprintf( "Error: Could not allocate page buffer (%d bytes) via VA !\n", cbPage );
        goto HandleError;
    }

    hr = g_DebugSystemObjects->GetCurrentProcessHandle( &ulCurrentProcess );
    hCurrentProcess = (HANDLE) ulCurrentProcess;
    if ( FAILED( hr ) )
    {
        dprintf( "Failed to fetch process handle: %#x\n", hr );
        goto HandleError;
    }

    if ( FEDBGGetDbDiskPage( hCurrentProcess, posf->Handle(), pgno, pbPage, cbPage ) )
    {
        dprintf( "File: %ws\n", posf->WszFile() );
        dprintf( "Handle: 0x%x\n", posf->Handle() );
        dprintf( "Page: %d (0x%x)\n", pgno, pgno );
        dprintf( "Offset: 0x%I64x\n", QWORD( pgno + cpgDBReserved - 1 ) * cbPage );
        dprintf( "\n" );
        EDBGDumpPage(
                ifmp,
                pgno,
                pbPage,
                cbPage,
                cbPage,
                DWORD_PTR( QWORD( pgno + cpgDBReserved - 1 ) * cbPage - QWORD( pbPage ) ),  //  UNDONE: offsets may not get reported correctly if greater than 4Gb
                fDumpHeader,
                fDumpTags,
                fDumpAllocMap,
                fDumpBinary );
    }

HandleError:
    dprintf( "\n" );

    if ( NULL != pbPage )
    {
        VirtualFree( pbPage, 0, MEM_RELEASE );
    }
}


//  ===================================
BOOL FEDBGGetBufferInfoFromPv(
    _In_ const BYTE * const pvOffset,
    _Out_ size_t * const    pcbBuffer,
    _Out_ IFMP * const      pifmp,
    _Out_ PGNO * const      ppgno,
    _Out_ size_t * const    pcbPage )
//  ===================================
{
    ERR                 err                 = JET_errSuccess;

    Call( Pdls()->ErrBFInitCacheMap() );

    //  Init out param

    if ( pifmp )
    {
        *pifmp = ifmpNil;
    }
    *pcbBuffer = 0;
    *ppgno = 0;
    *pcbPage = 0;

    //  Try to find a BF w/ the right ->pv element

    IPG ipg = Pdls()->IpgBFCachePv( pvOffset );

    if ( ipgNil != ipg )
    {

        // map to PBF from ipg (which is also a ibf)

        PBF pbfDebuggee = Pdls()->PbfBFCacheIbf( (IBF)ipg );

        // process results

        PBF pbf = NULL;
        if ( !FFetchVariable( pbfDebuggee, &pbf ) )
        {
            //  we failed to read this BF
            dprintf( "Error: Could not read BF at 0x%N. Aborting.\n", pbfDebuggee );
            goto HandleError;
        }

        *ppgno = pbf->pgno;
        if ( pifmp )
        {
            *pifmp = pbf->ifmp;
        }

        if ( pcbPage != NULL && pbf->icbPage < icbPageMax )
        {
            AssertEDBG( Pdls()->CbPage( pbf->ifmp ) == g_rgcbPageSize[ pbf->icbPage ] );
            *pcbPage = g_rgcbPageSize[ pbf->icbPage ];
        }

        if ( pbf->icbBuffer < icbPageMax )
        {
            *pcbBuffer = g_rgcbPageSize[ pbf->icbBuffer ];
            Unfetch( pbf );
            return fTrue;
        }

        Unfetch( pbf );
    }

HandleError:
    Assert( *pcbBuffer == 0 );

    Pdls()->BFTermCacheMap();

    return fFalse;
}


//  =========================================
ERR ErrEDBGFetchPage(
    _In_ BYTE * const       rgbPageDebuggee,
    _Out_ IFMP * const      pifmp,
    _Inout_ PGNO * const    ppgno,
    _Inout_ size_t * const  pcbPage,
    _Out_ BYTE ** const     prgbPage,
    _Out_ size_t * const    pcbBuffer )
//  =========================================
{
    AssertEDBG( NULL != rgbPageDebuggee );
    AssertEDBG( NULL != ppgno );
    AssertEDBG( NULL != prgbPage );
    AssertEDBG( NULL != pcbBuffer );

    ERR     err     = JET_errSuccess;
    PGNO    pgnoT   = 0;
    size_t  cbPageT = 0;

    if ( FEDBGGetBufferInfoFromPv( rgbPageDebuggee, pcbBuffer, pifmp, &pgnoT, &cbPageT ) )
    {
        if ( 0 == *ppgno )
        {
            dprintf( "Note: auto-detected pgno from BF using buffer/pv = 0x%N, found pgno = %d (0x%x)\n", rgbPageDebuggee, pgnoT, pgnoT );
            *ppgno = pgnoT;
        }
        else if ( *ppgno != pgnoT )
        {
            dprintf( "WARNING: auto-detected pgno per BF based buffer/pv is different than pgno provided, pgno detected = %d (consider trying to dump with this pgno)\n", pgnoT );
        }

        if ( *pcbBuffer != *pcbPage )
        {
            dprintf( "Note: auto-detected buffer size from BF based buffer/pv, found cbBuffer = %d\n", *pcbBuffer );
        }

        if ( 0 == *pcbPage )
        {
            dprintf( "Note: auto-detected page size from BF using buffer/pv = 0x%N, found cbPage = %d\n", rgbPageDebuggee, cbPageT );
            *pcbPage = cbPageT;
        }
        else if ( *pcbPage != cbPageT )
        {
            dprintf( "WARNING: auto-detected page size per BF based buffer/pv is different than pgno provided, pgno detected = %d (consider trying to dump with this pgno)\n", cbPageT );
        }
    }
    else
    {
        dprintf( "WARNING: could not find matching BF for pv = %p, assuming page buffer size to be equal to page sized = %u (i.e. hydrated)\n", rgbPageDebuggee, (ULONG)*pcbPage );
        *pcbBuffer = *pcbPage;
    }

    if ( !FFetchAlignedVariable( rgbPageDebuggee, prgbPage, *pcbBuffer ) )
    {
        dprintf( "Error: Could not retrieve the required data from the debuggee.\n" );
        err = ErrEDBGCheck( errCantRetrieveDebuggeeMemory );
        goto HandleError;
    }

HandleError:
    return err;
}


//  ================================================================
DEBUG_EXT( EDBGDecompress )
//  ================================================================
{
    ERR             err                 = JET_errSuccess;
    BYTE *          pbDebuggee          = NULL;
    BYTE *          pbToDecompress      = NULL;
    ULONG           cbToDecompress      = 0;
    BYTE *          pbDecompressed      = NULL;
    ULONG           cbDecompressed      = 0;
    DATA            dataToDecompress;

    dprintf( "\n" );

    //  validate args
    //
    if ( argc != 2
        || !FAddressFromSz( argv[0], &pbDebuggee )
        || !FUlFromSz( argv[1], (ULONG *)&cbToDecompress ) )
    {
        //  invalid usage
        //
        dprintf( "Usage: DECOMPRESS <pb> <cb>\n" );
        dprintf( "    <pb> is the address of the byte stream to decompress\n" );
        dprintf( "    <cb> is the size of the byte stream to decompress\n" );
        err = ErrEDBGCheck( JET_errInvalidParameter );
        goto HandleError;
    }

    //  retrieve the compressed byte stream
    //
    if ( !FFetchVariable( pbDebuggee, (BYTE**)&pbToDecompress, cbToDecompress ) )
    {
        dprintf( "Error: Could not fetch the byte stream to decompress.\n" );
        err = ErrEDBGCheck( JET_errOutOfMemory );
        goto HandleError;
    }
    dataToDecompress.SetPv( pbToDecompress );
    dataToDecompress.SetCb( cbToDecompress );

    //  Get the size needed and allocate buffer
    //
    (void)ErrPKDecompressData( dataToDecompress, NULL, NULL, 0, (INT *)&cbDecompressed );
    pbDecompressed = new BYTE[cbDecompressed];
    if ( NULL == pbDecompressed )
    {
        dprintf( "Error: Could not allocate temporary buffer to hold decompressed byte stream.\n" );
        err = ErrEDBGCheck( JET_errOutOfMemory );
        goto HandleError;
    }

    //  decompress the byte stream
    //
    err = ErrPKDecompressData( dataToDecompress, NULL, pbDecompressed, cbDecompressed, (INT *)&cbDecompressed );
    if ( JET_errSuccess != err )
    {
        dprintf( "Error %d (0x%x) attempting to decompress byte stream.\n", err, err );
        goto HandleError;
    }

    //  dump the decompressed byte stream
    //
    dprintf( "%d (0x%x) bytes at 0x%p decompressed to %d (0x%x) bytes:\n\n", cbToDecompress, cbToDecompress, pbDebuggee, cbDecompressed, cbDecompressed );
    EDBGDumpRawData( CPRINTFWDBG::PcprintfInstance(), pbDecompressed, cbDecompressed, fFalse );
    
HandleError:
    delete[] pbDecompressed;
    Unfetch( pbToDecompress );
    dprintf( "\n" );
}

//  ================================================================
DEBUG_EXT( EDBGDecrypt )
//  ================================================================
{
    ERR             err                 = JET_errSuccess;
    BYTE *          pbBufferDebuggee    = NULL;
    ULONG           cbBuffer            = 0;
    BYTE *          pbKeyDebuggee       = NULL;
    ULONG           cbKey               = 0;
    BYTE *          pbBuffer            = NULL;
    BYTE *          pbKey               = NULL;
    BYTE *          pbDecrypted         = NULL;
    ULONG           cbDecrypted         = 0;
    BOOL            fDecompress         = fFalse;
    BYTE *          pbDecompressed      = NULL;
    ULONG           cbDecompressed      = 0;
    DATA            data;

    dprintf( "\n" );

    //  validate args
    //
    if ( ( argc != 4 && argc != 5 )
        || !FAddressFromSz( argv[0], &pbBufferDebuggee )
        || !FUlFromSz( argv[1], &cbBuffer )
        || !FAddressFromSz( argv[2], &pbKeyDebuggee )
        || !FUlFromSz( argv[3], &cbKey ) )
    {
        //  invalid usage
        //
        dprintf( "Usage: DECRYPT <pbBuffer> <cbBuffer> <pbKey> <cbKey> [d]\n" );
        dprintf( "    <pbBuffer> is the address of the byte stream to decrypt (and optionally decompress)\n" );
        dprintf( "    <cbBuffer> is the size of the byte stream to decrypt (and optionally decompress)\n" );
        dprintf( "    <pbKey> is the address of the encryption key\n" );
        dprintf( "    <cbKey> is the size of the encryption key\n" );
        dprintf( "    [d] - optionally decompress after decryption\n" );
        err = ErrEDBGCheck( JET_errInvalidParameter );
        goto HandleError;
    }
    fDecompress = ( argc == 5 );

    //  retrieve the original byte stream
    //
    if ( !FFetchVariable( pbBufferDebuggee, &pbBuffer, cbBuffer ) ||
         !FFetchVariable( pbKeyDebuggee, &pbKey, cbKey ) )
    {
        dprintf( "Error: Could not fetch the byte stream/key to decrypt.\n" );
        err = ErrEDBGCheck( JET_errOutOfMemory );
        goto HandleError;
    }

    //  allocate a buffer to hold the decrypted byte stream
    //
    cbDecrypted = cbBuffer;
    pbDecrypted = new BYTE[cbDecrypted];
    if ( NULL == pbDecrypted )
    {
        dprintf( "Error: Could not allocate temporary buffer to hold decrypted byte stream.\n" );
        err = ErrEDBGCheck( JET_errOutOfMemory );
        goto HandleError;
    }

    err = ErrOSDecryptWithAes256(
            pbBuffer,
            pbDecrypted,
            &cbDecrypted,
            pbKey,
            cbKey );
    if ( JET_errSuccess != err )
    {
        dprintf( "Error %d (0x%x) attempting to decrypt byte stream.\n", err, err );
        goto HandleError;
    }

    data.SetPv( pbDecrypted );
    data.SetCb( cbDecrypted );

    if ( fDecompress )
    {
        //  Get the size needed and allocate buffer
        //
        (void)ErrPKDecompressData( data, NULL, NULL, 0, (INT *)&cbDecompressed );
        pbDecompressed = new BYTE[cbDecompressed];
        if ( NULL == pbDecompressed )
        {
            dprintf( "Error: Could not allocate temporary buffer to hold decompressed byte stream.\n" );
            err = ErrEDBGCheck( JET_errOutOfMemory );
            goto HandleError;
        }

        //  decompress the byte stream
        //
        err = ErrPKDecompressData( data, NULL, pbDecompressed, cbDecompressed, (INT *)&cbDecompressed );
        if ( JET_errSuccess != err )
        {
            dprintf( "Error %d (0x%x) attempting to decompress byte stream.\n", err, err );
            goto HandleError;
        }

        data.SetPv( pbDecompressed );
        data.SetCb( cbDecompressed );
    }

    //  dump the decompressed byte stream
    //
    dprintf( "%d (0x%x) bytes at 0x%p decrypted/decompressed to %d (0x%x) bytes:\n\n", cbBuffer, cbBuffer, pbBufferDebuggee, data.Cb(), data.Cb() );
    EDBGDumpRawData( CPRINTFWDBG::PcprintfInstance(), (BYTE*)data.Pv(), data.Cb(), fFalse );
    
HandleError:
    delete[] pbDecompressed;
    delete[] pbDecrypted;
    Unfetch( pbKey );
    Unfetch( pbBuffer );
    dprintf( "\n" );
}


//  ================================================================
LOCAL VOID EDBGDumpNodeInfo( CPRINTF * pcprintf, const CPAGE * const pcpage, const INT iline, const BOOL fKeyOnly, const BOOL fVerbose, DWORD_PTR dwOffset )
//  ================================================================
{
    KEYDATAFLAGS        kdf;
    BYTE *              rgbPrefix           = NULL;
    BYTE *              rgbSuffix           = NULL;
    BYTE *              rgbData             = NULL;
    BOOL                fDumpRawData        = fVerbose;

    AssertEDBG( NULL != pcprintf );
    AssertEDBG( NULL != pcpage );
    AssertEDBG( iline >= 0 );
    AssertEDBG( iline < pcpage->Clines() );
    AssertEDBG( 0 != dwOffset );

    pcpage->DumpTag( pcprintf, iline, dwOffset );

    NDIGetKeydataflags( *pcpage, iline, &kdf );

    //  fetch and dump prefix, if any
    //
    if ( kdf.key.prefix.Cb() > 0 )
    {
        if ( FFetchVariable( (BYTE *)kdf.key.prefix.Pv() + dwOffset, &rgbPrefix, kdf.key.prefix.Cb() ) )
        {
            (*pcprintf)( _T( "Prefix (%d bytes):%c" ), kdf.key.prefix.Cb(), ( kdf.key.prefix.Cb() > 16 ? '\n' : ' ' ) );
            EDBGDumpRawData( pcprintf, rgbPrefix, kdf.key.prefix.Cb(), fFalse );
        }
        else
        {
            (*pcprintf)( _T( "Error: Failed fetching node prefix.\n" ) );
        }
    }
    else
    {
        (*pcprintf)( _T( "Prefix: <null>\n" ) );
    }

    //  fetch and dump suffix, if any
    //
    if ( kdf.key.suffix.Cb() > 0 )
    {
        if ( FFetchVariable( (BYTE *)kdf.key.suffix.Pv() + dwOffset, &rgbSuffix, kdf.key.suffix.Cb() ) )
        {
            (*pcprintf)( _T( "Suffix (%d bytes):%c" ), kdf.key.suffix.Cb(), ( kdf.key.suffix.Cb() > 16 ? '\n' : ' ' ) );
            EDBGDumpRawData( pcprintf, rgbSuffix, kdf.key.suffix.Cb(), fFalse );
        }
        else
        {
            (*pcprintf)( _T( "Error: Failed fetching node suffix.\n" ) );
        }
    }
    else
    {
        (*pcprintf)( _T( "Suffix: <null>\n" ) );
    }

    //  only fetch data if not performing key-only dump,
    //  or if raw data was explicitly requested
    //
    if ( !fKeyOnly || fDumpRawData )
    {
        if ( kdf.data.Cb() > 0 )
        {
            if ( !FFetchVariable( (BYTE *)kdf.data.Pv() + dwOffset, &rgbData, kdf.data.Cb() ) )
            {
                (*pcprintf)( _T( "Error: Failed fetching node data.\n" ) );
                goto HandleError;
            }
        }
        else
        {
            (*pcprintf)( _T( "Data: <null>\n" ) );
            goto HandleError;
        }
    }

    //  dump formatted node data if not performing
    //  key-only dump
    //
    if ( !fKeyOnly )
    {
        if ( !pcpage->FLeafPage() )
        {
            (*pcprintf)(
                    _T( "Page Pointer: %d (0x%x)\n" ),
                    (PGNO)*((LittleEndian<PGNO>*)rgbData),
                    (PGNO)*((LittleEndian<PGNO>*)rgbData) );
            fDumpRawData = fFalse;
        }
        else if ( pcpage->FSpaceTree() )
        {
            PGNO       pgnoLast    = 0xFFFFFFFF;
            CPG        cpgExtent   = 0xFFFFFFFF;
            PCWSTR     wszPoolName = NULL;

            if( ErrSPREPAIRValidateSpaceNode( &kdf, &pgnoLast, &cpgExtent, &wszPoolName ) >= JET_errSuccess )
            {
                (*pcprintf)(
                        _T( "Space Data (%d bytes): Pool:%ws, cpg:%d, page range:%d-%d\n" ),
                        kdf.data.Cb(),
                        wszPoolName,
                        cpgExtent,
                        pgnoLast - cpgExtent + 1,
                        pgnoLast );
            }
            else
            {
                (*pcprintf)( _T( "Space Data (%d bytes): <could not parse space node data>\n" ), kdf.data.Cb() );
                fDumpRawData = fTrue;
            }
        }
        else if ( pcpage->FLongValuePage() )
        {
            (*pcprintf)( _T( "Long-Value Data (%d bytes):\n" ), kdf.data.Cb() );
            EDBGDumpRawData( pcprintf, rgbData, kdf.data.Cb(), fTrue );
            fDumpRawData = fFalse;
        }
        else if ( pcpage->FIndexPage() )
        {
            (*pcprintf)( _T( "Primary Bookmark (%d bytes):%c" ), kdf.data.Cb(), ( kdf.data.Cb() > 16 ? '\n' : ' ' ) );
            EDBGDumpRawData( pcprintf, rgbData, kdf.data.Cb(), fFalse );
            fDumpRawData = fFalse;
        }
        else
        {
            SaveDlsDefaults sdd; // saves here, and then restores the implicit defaults on .dtor.

            FCB * pfcbTable = pfcbNil;
            BYTE rgbFucbBuffer[ sizeof( FUCB ) + 16 ];
            memset( rgbFucbBuffer, 0, sizeof( rgbFucbBuffer ) );
            FUCB * pfucbSchemaOnly = (FUCB*)roundup( (ULONG_PTR)rgbFucbBuffer, 8 ); // probably aligned, but just in case.
            if ( Pdls()->ObjidCurrentBt() != pcpage->ObjidFDP() )
            {
                FCB * pfcbTableDebuggee = pfcbNil;
                FCB * pfcbT = pfcbNil;
                OBJID objid = objidNil;
                PGNO pgnoRoot = pgnoNull;

                (void)FEDBGFindAndSetImplicitBt( Pdls()->IpinstCurrent(), Pdls()->IfmpCurrent(), pcpage->ObjidFDP(), NULL, edbgsptfNone, &pfcbTableDebuggee, &pfcbT, &objid, &pgnoRoot );
            }
            if ( Pdls()->ObjidCurrentBt() == pcpage->ObjidFDP() && Pdls()->PfcbCurrentTableDebuggee() != pfcbNil )
            {
                (void)FEDBGFetchTableMetaData( Pdls()->PfcbCurrentTableDebuggee(), &pfcbTable );

                pfucbSchemaOnly->u.pfcb = pfcbTable;
                // Don't seem to need any csr elements loaded at the moment.
                //pfucbSchemaOnly->csr.ErrLoadPage( ppibNil, ifmpNil, pgno, pcpage->PvBuffer(), Pdls()->CbPage(), latchNone );
                //pfucbSchemaOnly->csr.SetILine( iline );
            }
            if ( !pfcbTable )
            {
                dprintf( "WARNING: Could not retrieve table metadata on pfcb = %p, so will be missing some column data.\n", Pdls()->PfcbCurrentTableDebuggee() );
            }

            (*pcprintf)( _T( "Data Record (%d bytes):\n"), kdf.data.Cb() );

            // Note: pfcbTable ? pfucbSchemaOnly : NULL is _correct_.  We are just using the pfucbSchemaOnly to 
            // pass the FCB really, as that's what DBUTLDumpRec() expects.
            DBUTLDumpRec( Pdls()->CbPage(), pfcbTable ? pfucbSchemaOnly : NULL, rgbData, kdf.data.Cb(), pcprintf, 16 );

            Unfetch( pfcbTable );
        }
    }

    //  dump raw node data if requested
    //
    if ( fDumpRawData )
    {
        (*pcprintf)( _T( "Raw Data (%d bytes):\n"), kdf.data.Cb() );
        EDBGDumpRawData( pcprintf, rgbData, kdf.data.Cb(), fTrue );
    }

HandleError:
    (*pcprintf)( _T( "\n" ) );
    Unfetch( rgbPrefix );
    Unfetch( rgbSuffix );
    Unfetch( rgbData );
}


//  ================================================================
DEBUG_EXT( EDBGDumpNode )
//  ================================================================
{
    SaveDlsDefaults sdd; // saves here, and then restores the implicit defaults on .dtor.

    ERR             err             = JET_errSuccess;
    const CHAR *    pchPageSize     = NULL;
    BOOL            fAllNodes       = fFalse;
    BOOL            fKeyOnly        = fFalse;
    BOOL            fVerbose        = fFalse;
    PGNO            pgno            = 0;
    BYTE *          rgbPageDebuggee = NULL;
    BYTE *          rgbPage         = NULL;
    size_t          cbPageUser      = 0;
    size_t          cbBuffer        = 0;
    INT             iline           = 0;
    DWORD_PTR       dwOffset        = 0;
    CPAGE           cpage;

    dprintf( "\n" );

    if ( argc < 2
        || argc > 3
        || !FAddressFromSz( argv[0], &rgbPageDebuggee )
        || (!(fAllNodes = ('*' == argv[1][0])) && !FUlFromSz( argv[1], (ULONG *)&iline ) ) )
    {
        //  invalid usage
        //
        dprintf( "Usage: DUMPNODE <pvPage> <iLine> [k|v|2|4|8|16|32]\n" );
        dprintf( "    <pvPage> is the address of the database page\n" );
        dprintf( "    <iLine> is the iLine (NOT itag) of the desired node, or * for all nodes\n" );
        dprintf( "    [k=key only, v=verbose data dump (raw data bytes), 2/4/8/16/32=pagesize]\n" );
        err = ErrEDBGCheck( JET_errInvalidParameter );
        goto HandleError;
    }

    //  parse options, if provided
    //
    if ( argc > 2 )
    {
        pchPageSize = (CHAR *)strpbrk( argv[2], "12348" );
        fKeyOnly = ( strpbrk( argv[2], "kK" ) != NULL );
        fVerbose = ( strpbrk( argv[2], "vV" ) != NULL );
    }

    //  validate page size option, if provided
    //
    if ( !FEDBGParsePageSizeOption( pchPageSize, &cbPageUser ) )
    {
        dprintf( "ERROR: Did not understand user page option.\n" );
        err = ErrEDBGCheck( JET_errInvalidParameter );
        goto HandleError;
    }

    //  if this is a cached page, retrieve relevant info from BF cache (ifmp, page size, and buffer size) and
    //  no matter what, get the page buffer itself.
    //
    IFMP ifmp = 0;
    size_t cbPageAuto = cbPageUser;
    Call( ErrEDBGFetchPage( rgbPageDebuggee, &ifmp, &pgno, &cbPageAuto, &rgbPage, &cbBuffer ) );

    AssertEDBG( ifmp != 0 );
    if ( ifmp != 0 )
    {
        dprintf( "Note: Setting / updating implicit .ifmp context to %d in DumpNode so that objid searching is accurate.\n", ifmp );
        Pdls()->SetImplicitIfmp( ifmp );
        AssertEDBG( Pdls()->CbPage() == cbPageAuto );
    }

    if ( cbPageUser != 0 && cbPageUser != cbPageAuto )
    {
        dprintf( "Ignoring auto-loaded page size, and using user specified page size of %d.  If it does not work, trying not specifying page size and let ESE pick.\n", cbPageUser );
    }
    const size_t cbPage = cbPageUser ? cbPageUser : cbPageAuto;

    cpage.LoadDehydratedPage( ifmpNil, pgno, rgbPage, cbBuffer, cbPage );

    EDBGDeprecatedSetGlobalPageSize( cbPage );

    dprintf( "\n" );
    
    dwOffset = rgbPageDebuggee - rgbPage;

    if ( fAllNodes )
    {
        for ( iline = 0; iline < cpage.Clines(); iline++ )
        {
            EDBGDumpNodeInfo( CPRINTFWDBG::PcprintfInstance(), &cpage, iline, fKeyOnly, fVerbose, dwOffset );
        }
    }
    else if (iline < 0 || iline > cpage.Clines())
    {
        //  TODO: can't dump external header
        //
        dprintf("Error: iLine %d is out-of-range for the specified page.\n", iline);
        err = ErrEDBGCheck( JET_errInvalidParameter );
        goto HandleError;
    }
    else
    {
        EDBGDumpNodeInfo( CPRINTFWDBG::PcprintfInstance(), &cpage, iline, fKeyOnly, fVerbose, dwOffset );
    }

HandleError:
    UnfetchAligned( rgbPage );
    dprintf( "\n" );
}


//  ================================================================
DEBUG_EXT( EDBGHelpDump )
//  ================================================================
{
    INT icdumpmap;
    for( icdumpmap = 0; icdumpmap < ccdumpmap; icdumpmap++ )
    {
        dprintf( "\t%s\n", rgcdumpmap[icdumpmap].szHelp );
    }
    dprintf( "\n--------------------\n\n" );
}


//  ================================================================
DEBUG_EXT( EDBGDump )
//  ================================================================
{
    if( argc < 2 )
    {
        EDBGHelpDump( pdebugClient, argc, argv );
        return;
    }

    INT icdumpmap;
    for( icdumpmap = 0; icdumpmap < ccdumpmap; ++icdumpmap )
    {
        if( FArgumentMatch( argv[0], rgcdumpmap[icdumpmap].szCommand ) )
        {
            (rgcdumpmap[icdumpmap].pcdump)->Dump(
                pdebugClient,
                argc - 1, argv + 1 );
            return;
        }
    }
    EDBGHelpDump( pdebugClient, argc, argv );
}


//  ================================================================
template< class _STRUCT>
VOID CDUMPA<_STRUCT>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT argc, const CHAR * const argv[]
    ) const
//  ================================================================
{
    _STRUCT* ptDebuggee = NULL;
    if (    argc != 1 ||
            !FAutoAddressFromSz( argv[ 0 ], &ptDebuggee ) )
    {
        dprintf( "Usage: DUMP <class> <address>\n" );
        return;
    }

    _STRUCT* pt = NULL;
    if ( FFetchVariable( ptDebuggee, &pt ) )
    {
        dprintf(    "0x%p bytes @ 0x%N\n",
                    QWORD( sizeof( _STRUCT ) ),
                    ptDebuggee );
        pt->Dump( CPRINTFWDBG::PcprintfInstance(), (BYTE*)ptDebuggee - (BYTE*)pt );
        Unfetch( pt );
    }
}


//  ================================================================
VOID CDUMPA<CPAGE>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT argc, const CHAR * const argv[]
    ) const
//  ================================================================
{
    ERR     err             = JET_errSuccess;
    PGNO    pgno;
    BYTE *  rgbPageDebuggee = NULL;
    BYTE *  rgbPage         = NULL;
    size_t  cbPage          = 0;
    size_t  cbBuffer        = 0;
    BOOL    fDumpAllocMap   = fFalse;
    BOOL    fDumpBinary     = fFalse;
    BOOL    fDumpHeader     = fFalse;
    BOOL    fDumpTags       = fFalse;
    ULONG   cpiqPageFind    = 0;
    CIterQuery * rgpiqPageFind[ 3 ] = { 0 };

    dprintf( "\n" );

    if ( argc < 2
        || argc > 3
        || !FUlFromSz( argv[0], &pgno ) )
    {
        //  invalid usage
        //
        dprintf( "Usage: DUMP PAGE <pgno> <address> [a|b|h|t|*|2|4|8|16|32]\n" );
        dprintf( "    <pgno> is the page number (required for checksumming purposes only)\n" );
        dprintf( "    <address|.> is the pointer to the memory buffer containing the page contents\n" );
        dprintf( "    [a=alloc map, b=binary dump, h=header, t=tags, *=all, 2/4/8/16/32s=pagesize]\n" );
        err = ErrEDBGCheck( JET_errInvalidParameter );
        goto HandleError;
    }

    if ( !FEDBGParseDumpPageOptions(
                            ( argc < 3 ? NULL : argv[2] ),
                            &cbPage,
                            &fDumpHeader,
                            &fDumpTags,
                            &fDumpAllocMap,
                            &fDumpBinary ) )
    {
        err = ErrEDBGCheck( JET_errInvalidParameter );
        goto HandleError;
    }

    if ( FImplicitArg( argv[1] ) )
    {
        CHAR szIfmp[12];
        CHAR szPgnoDec[12];

        const IFMP ifmpImplicit = Pdls()->IfmpCurrent();
        if ( cbPage == 0 )
        {
            //  Auto loading cbPage based upon ifmp.
            cbPage = Pdls()->CbPage( ifmpImplicit );
        }

        OSStrCbFormatA( szIfmp, sizeof( szIfmp ), "%d", ifmpImplicit );
        OSStrCbFormatA( szPgnoDec, sizeof( szPgnoDec ), "%d", pgno );

        QWORD qwPagePv = NULL;
        __int64 cpgMatches = 0;

        CHAR * rgszGetIfmpPage [] = { "ifmp", "==", szIfmp, "&&", "pgno", "==", szPgnoDec, "&&", "fCurrentVersion", "==", "fTrue", "accum:pv" };
        CHAR * rgszGetPage []     =                             { "pgno", "==", szPgnoDec, "&&", "fCurrentVersion", "==", "fTrue", "accum:pv" };

        Call( ErrIQCreateIterQueryCount( &g_iedBf, 
                                         ifmpImplicit != ifmpNil ? _countof(rgszGetIfmpPage) : _countof(rgszGetPage),
                                         ifmpImplicit != ifmpNil ? rgszGetIfmpPage : rgszGetPage, 
                                         (void*)&qwPagePv,
                                         &(rgpiqPageFind[cpiqPageFind++]) ) );

        // Update the action and add another query
        AssertEDBG( 0 == LOSStrCompareA( rgszGetIfmpPage[ 11 ], "accum:pv" ) );
        AssertEDBG( 0 == LOSStrCompareA( rgszGetPage[ 7 ], "accum:pv" ) );
        rgszGetIfmpPage[ 11 ] = "count";
        rgszGetPage[ 7 ]      = "count";

        Call( ErrIQCreateIterQueryCount( &g_iedBf, 
                                         ifmpImplicit != ifmpNil ? _countof(rgszGetIfmpPage) : _countof(rgszGetPage),
                                         ifmpImplicit != ifmpNil ? rgszGetIfmpPage : rgszGetPage, 
                                         (void*)&cpgMatches,
                                         &(rgpiqPageFind[cpiqPageFind++]) ) );

        // Cut out the last clause, and update the action and add a last query
        AssertEDBG( 0 == LOSStrCompareA( rgszGetIfmpPage[ 7 ], "&&" ) );
        AssertEDBG( 0 == LOSStrCompareA( rgszGetPage[ 3 ], "&&" ) );
        CHAR * szPrintFields = "print:ifmp,pgno,fCurrentVersion,err,bfdf,pv";
        rgszGetIfmpPage[ 7 ] = szPrintFields;
        rgszGetIfmpPage[ 8 ] = NULL;
        rgszGetPage[ 3 ]     = szPrintFields;
        rgszGetPage[ 4 ]     = NULL;

        Call( ErrIQCreateIterQuery( &g_iedBf, 
                                         ifmpImplicit != ifmpNil ? _countof(rgszGetIfmpPage) - 4 : _countof(rgszGetPage) - 4,
                                         ifmpImplicit != ifmpNil ? rgszGetIfmpPage : rgszGetPage, 
                                         &(rgpiqPageFind[cpiqPageFind++]) ) );

        AssertEDBG( cpiqPageFind == 3 ); // just an expected

        dprintf( "Searching ESE cache ...\n" );
        dprintf( " pcvEntry=      <pbf>     :  %hs\n", szPrintFields );
        Call( ErrEDBGQueryCache( cpiqPageFind, rgpiqPageFind ) );
        dprintf( "Found: %d page.\n", cpgMatches );

        if ( cpgMatches != 1 )
        {
            // We failed to get just one hit ...
            dprintf( "ERROR: Your pgno request returned more or less than 1 page.  Did you run !ese .db to set implicit DB/ifmp?" );
            goto HandleError;
        }
        AssertEDBGSz( cpgMatches == 1, "Note you can't let anything but 1 go, because we're 'accumulating' / accum:pv above, so 2 pages pv's added together is only a pointer in hyper-space-based RAM hardware." );

        dprintf( "Best address: 0x%I64x\n", qwPagePv );
        rgbPageDebuggee = (BYTE*)qwPagePv;
    }
    else if ( !FAddressFromSz( argv[1], &rgbPageDebuggee ) )
    {
        dprintf( "Error: Could not retrieve the required data from the debuggee.\n" );
        err = ErrEDBGCheck( errCantRetrieveDebuggeeMemory );
        goto HandleError;
    }

    size_t cbPageAuto = cbPage;
    Call( ErrEDBGFetchPage( rgbPageDebuggee, NULL, &pgno, &cbPageAuto, &rgbPage, &cbBuffer ) );
    if ( cbPage == 0 && cbPageAuto != cbPage )
    {
        dprintf( "Note: Overriding auto-loaded cbPage = %d with user cbPage = %d.  If this doesn't work, try not specifying page size explicitly.", cbPageAuto, cbPage );
        cbPage = cbPageAuto;
    }

    EDBGDumpPage(
            ifmpNil,
            pgno,   //  used solely for checksumming purposes
            rgbPage,
            cbBuffer,
            cbPage,
            rgbPageDebuggee - rgbPage,
            fDumpHeader,
            fDumpTags,
            fDumpAllocMap,
            fDumpBinary );

    UnfetchAligned( rgbPage );

HandleError:

    for ( ULONG ipiq = 0; ipiq < cpiqPageFind; ipiq++ )
    {
        delete rgpiqPageFind[ ipiq ];
    }

    dprintf( "\n" );
}


//  ================================================================
VOID CDUMPA<REC>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT argc, const CHAR * const argv[]
    ) const
//  ================================================================
{
    SaveDlsDefaults sdd; // saves here, and then restores the implicit defaults on .dtor.
    BYTE*           rgbRecDebuggee;
    BYTE*           rgbRec;
    ULONG           cbRec;

    FCB *           pfcbTable = NULL;
    BYTE            rgbFucbBuffer[ sizeof( FUCB ) + 16 ];
    FUCB *          pfucbSchemaOnly = (FUCB*)roundup( (ULONG_PTR)rgbFucbBuffer, 8 ); // probably aligned, but just in case.
    memset( rgbFucbBuffer, 0, sizeof( rgbFucbBuffer ) );

    if (    ( argc != 2 && argc != 3 ) ||
            !FAddressFromSz( argv[ 0 ], &rgbRecDebuggee ) ||
            !FUlFromSz( argv[ 1 ], &cbRec ) )
    {
        dprintf( "Usage: DUMP REC <address> <length> [<TableName>|<objid>|.]\n" );
        return;
    }

    //  Alternatively instead of forcing a table name for the best record dump, SOMEONE notes we could do the trick we do 
    //  in !ese dumpnode, where instead of auto-configuring trying to locate an exact pbf->pv == pvPage, we could search 
    //  for buffers where pv is contained _within_ the BF page, a sort of: 
    //  if ( pbf->pv < rgbRecDebuggee && rgbRecDebuggeed < ( pbf->pv + g_rgcbPageSizes[ pbf->icbBuffer ] ) )... set context.
    //  Nice idea.

    if ( argc == 3 )
    {
        // dev offers a table name, see if we can locate it and load it's schema.

        EDBGSetImplicitBT( pdebugClient, 1, &argv[2] );
        if ( Pdls()->ObjidCurrentBt() == 0 || Pdls()->PfcbCurrentTableDebuggee() == pfcbNil )
        {
            dprintf( "\nWARNING: Tried to find and set table context, but failed.  Perhaps table name was too ambiguous.  Dropping back to dumb rec dump with no fixed columns.\n\n");
        }

        if ( Pdls()->ObjidCurrentBt() != 0 && Pdls()->PfcbCurrentTableDebuggee() != pfcbNil )
        {
            (void)FEDBGFetchTableMetaData( Pdls()->PfcbCurrentTableDebuggee(), &pfcbTable );

             pfucbSchemaOnly->u.pfcb = pfcbTable;
        }
        if ( !pfcbTable )
        {
            dprintf( "WARNING: Could not retrieve table metadata on pfcb = %p, so will be missing some column data.\n", Pdls()->PfcbCurrentTableDebuggee() );
        }
    }

    if ( !FFetchVariable( rgbRecDebuggee, &rgbRec, cbRec ) )
    {
        dprintf( "Error: Could not fetch record from debuggee.\n" );
        return;
    }

    EDBGDeprecatedSetGlobalPageSize( Pdls()->CbPage() );

    dprintf( "\n" );

    // Note: pfcbTable ? pfucbSchemaOnly : NULL is _correct_.  We are just using the pfucbSchemaOnly to 
    // pass the FCB really, as that's what DBUTLDumpRec() expects.
    DBUTLDumpRec( Pdls()->CbPage(), pfcbTable ? pfucbSchemaOnly : NULL, rgbRec, cbRec, CPRINTFWDBG::PcprintfInstance(), 16 );

    Unfetch( rgbRec );
    Unfetch( pfcbTable );
}


//  ================================================================
VOID CDUMPA<MEMPOOL>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT argc, const CHAR * const argv[]
    ) const
//  ================================================================
{
    MEMPOOL *   pmempool;
    MEMPOOL *   pmempoolDebuggee;
    BOOL        fDumpTags       = fFalse;
    BOOL        fDumpAll        = fFalse;
    ULONG       itagDump        = 0;

    dprintf( "\n" );

    if ( argc < 1
        || argc > 2
        || !FAddressFromSz( argv[0], &pmempoolDebuggee )
        || ( ( fDumpTags = ( argc == 2 ) )
            && !( fDumpAll = ( strpbrk( argv[1], "*" ) != NULL ) )
            && !FUlFromSz( argv[1], &itagDump ) ) )
    {
        dprintf( "Usage: DUMP MEMPOOL <address> [<itag>|*]\n" );
        return;
    }

    if ( !FFetchVariable( pmempoolDebuggee, &pmempool ) )
    {
        dprintf( "Error: Could not fetch MEMPOOL.\n" );
        return;
    }

    pmempool->Dump(
        CPRINTFWDBG::PcprintfInstance(),
        fDumpTags,
        fDumpAll,
        (MEMPOOL::ITAG)itagDump,
        (BYTE *)pmempoolDebuggee - (BYTE *)pmempool );

    Unfetch( pmempool );
}

const static CHAR * const mpcsrlatchsz[ LATCH::latchWAR + 1 ] =
{
    "latchNone",
    "latchReadTouch",
    "latchReadNoTouch",
    "latchRIW",
    "latchWrite",
    "latchWAR",
};

const static CHAR * const mppagetrimsz[ PAGETRIM::pagetrimMax ] =
{
    "pagetrimNormal",
    "pagetrimTrimmed",
//  "pagetrimMax",
};

const CHAR * const mpdbstatesz[ JET_dbstateDirtyAndPatchedShutdown + 1 ] =
{
    "0",
    "JET_dbstateJustCreated",
    "JET_dbstateDirtyShutdown",
    "JET_dbstateCleanShutdown",
    "JET_dbstateBeingConverted",
    "JET_dbstateForceDetach",
    "JET_dbstateIncrementalReseedInProgress",
    "JET_dbstateDirtyAndPatchedShutdown",
};

//  ================================================================
VOID CSR::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    (*pcprintf)( FORMAT_INT( CSR, this, m_dbtimeSeen, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CSR, this, m_pgno, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CSR, this, m_iline, dwOffset ) );
    (*pcprintf)( FORMAT_ENUM( CSR, this, m_latch, dwOffset, mpcsrlatchsz, latchNone, latchWAR + 1 ) );
    (*pcprintf)( FORMAT_VOID( CSR, this, m_cpage, dwOffset ) );
    (*pcprintf)( FORMAT_ENUM( CSR, this, m_pagetrimState, dwOffset, mppagetrimsz, pagetrimNormal, pagetrimMax ) );

    dprintf( "\n\t [CPAGE] 0x%p bytes @ 0x%N\n",
            QWORD( sizeof( CPAGE ) ),
            (char *)this + dwOffset + OffsetOf( CSR, m_cpage )
            );

    (VOID)m_cpage.Dump( CPRINTFWDBG::PcprintfInstance(), dwOffset );
}


//  ================================================================
VOID RCE::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    (*pcprintf)( FORMAT_UINT( RCE, this, m_trxBegin0, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( RCE, this, m_trxCommittedInactive, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( RCE, this, m_ptrxCommitted, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( RCE, this, m_ifmp, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( RCE, this, m_pgnoFDP, dwOffset ) );

    (*pcprintf)( FORMAT_INT( RCE, this, m_ulFlags, dwOffset ) );

    PRINT_METHOD_FLAG( pcprintf, FOperNull );
    PRINT_METHOD_FLAG( pcprintf, FOperDDL );
    PRINT_METHOD_FLAG( pcprintf, FUndoableLoggedOper );
    PRINT_METHOD_FLAG( pcprintf, FOperInHashTable );
    PRINT_METHOD_FLAG( pcprintf, FOperReplace );
    PRINT_METHOD_FLAG( pcprintf, FOperConcurrent );
    PRINT_METHOD_FLAG( pcprintf, FOperAffectsSecondaryIndex );
    PRINT_METHOD_FLAG( pcprintf, FRolledBack );
    PRINT_METHOD_FLAG( pcprintf, FMoved );
    PRINT_METHOD_FLAG( pcprintf, FProxy );
    PRINT_METHOD_FLAG( pcprintf, FEmptyDiff );

    (*pcprintf)( FORMAT_INT_BF( RCE, this, m_level, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( RCE, this, m_oper, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( RCE, this, m_cbBookmarkKey, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( RCE, this, m_cbBookmarkData, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( RCE, this, m_cbData, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( RCE, this, m_rceid, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( RCE, this, m_updateid, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( RCE, this, m_pfcb, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( RCE, this, m_pfucb, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( RCE, this, m_prceNextOfSession, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( RCE, this, m_prcePrevOfSession, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( RCE, this, m_prceNextOfFCB, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( RCE, this, m_prcePrevOfFCB, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( RCE, this, m_prceNextOfNode, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( RCE, this, m_prcePrevOfNode, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( RCE, this, m_prceUndoInfoNext, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( RCE, this, m_prceUndoInfoPrev, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( RCE, this, m_pgnoUndoInfo, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( RCE, this, m_uiHash, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( RCE, this, m_prceHashOverflow, dwOffset ) );

    (*pcprintf)( FORMAT_0ARRAY( RCE, this, m_rgbData, dwOffset ) );

    //  dump pointer to next RCE in bucket to facilitate debugging
    //
    const ULONG     cbRCE       = CbRceEDBG();
    const BYTE *    pbNextRCE   = reinterpret_cast<BYTE *>( PvAlignForThisPlatform( (BYTE *)this + dwOffset + cbRCE ) );

    (*pcprintf)(
        "\t%*.*s <0x%x,%3i>:  0x%N\n",
        SYMBOL_LEN_MAX,
        SYMBOL_LEN_MAX,
        "prceNextInBucket",
        0,
        0,
        pbNextRCE );
}

const static WCHAR * const mpfcbtypenameswsz[FCB::fcbtypeMax] = {
      L"fcbtypeNull",
      L"fcbtypeDatabase",
      L"fcbtypeTable",
      L"fcbtypeSecondaryIndex",
      L"fcbtypeTemporaryTable",
      L"fcbtypeSort",
      L"fcbtypeLV",
      };

//  ================================================================
const WCHAR * FCB::WszFCBType() const
//  ================================================================
{
    C_ASSERT( _countof( mpfcbtypenameswsz ) == ( fcbtypeMax - fcbtypeNull ) );
    return mpfcbtypenameswsz[m_fcbtype];
}

//  ================================================================
BOOL FCB::FDebuggerExtInUse() const
//  ================================================================
{
    return ( 0 != WRefCount() );
}

//  ================================================================
BOOL FCB::FDebuggerExtPurgableEstimate() const
//  ================================================================
{
    return !FDebuggerExtInUse() &&
        !FDeletePending() &&
        !FTemplateTable() &&
        PgnoFDP() != pgnoSystemRoot &&
         0 == CTasksActive() &&
        ( prceNil == PrceOldest() );
}

//  ================================================================
VOID FCB::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    EDBGPrintfDml(  "--- FCB 0x%p <link cmd=\"!ese dumpmetadata 0x%p\">Dump Metadata</link> (p is %p, N is %N, and dwOffset is %I64x) ---\n",
                ((char*)(this) + dwOffset), ((char*)(this) + dwOffset)  );

    (*pcprintf)( FORMAT_POINTER( FCB, this, m_precdangling, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( FCB, this, m_ls, dwOffset ) );

    EDBGDumplinkDml( FCB, this, TDB, m_ptdb, dwOffset );
    EDBGDumplinkDml( FCB, this, FCB, m_pfcbNextIndex, dwOffset );
    EDBGDumplinkDml( FCB, this, FCB, m_pfcbLRU, dwOffset );
    EDBGDumplinkDml( FCB, this, FCB, m_pfcbMRU, dwOffset );
    EDBGDumplinkDml( FCB, this, FCB, m_pfcbNextList, dwOffset );
    EDBGDumplinkDml( FCB, this, FCB, m_pfcbPrevList, dwOffset );
    EDBGDumplinkDml( FCB, this, FCB, m_pfcbTable, dwOffset );
    (*pcprintf)( FORMAT_INT( FCB, this, m_tableclass, dwOffset ) );
    (*pcprintf)( "                              Note: m_tableclass may be inaccurate if m_pfcbTable is non-NULL.\n" );
    EDBGDumplinkDml( FCB, this, IDB, m_pidb, dwOffset );

    m_icmsFucbList.Dump(pcprintf, dwOffset );

    (*pcprintf)( FORMAT_INT( FCB, this, m_wRefCount, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FCB, this, m_objidFDP, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FCB, this, m_pgnoFDP, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FCB, this, m_pgnoOE, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FCB, this, m_pgnoAE, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FCB, this, m_ifmp, dwOffset ) );

    PRINT_METHOD_FLAG( pcprintf, FInList );
    PRINT_METHOD_FLAG( pcprintf, FInLRU );

    (*pcprintf)( FORMAT_ENUM_BF_WSZ( FCB, this, m_fcbtype, dwOffset, mpfcbtypenameswsz, fcbtypeNull, fcbtypeMax ) );

    (*pcprintf)( FORMAT_INT( FCB, this, m_ulFCBFlags, dwOffset ) );

    PRINT_METHOD_FLAG( pcprintf, FPrimaryIndex );
    PRINT_METHOD_FLAG( pcprintf, FSequentialIndex );
    PRINT_METHOD_FLAG( pcprintf, FFixedDDL );
    PRINT_METHOD_FLAG( pcprintf, FTemplateTable );
    PRINT_METHOD_FLAG( pcprintf, FDerivedTable );
    PRINT_METHOD_FLAG( pcprintf, FTemplateIndex );
    PRINT_METHOD_FLAG( pcprintf, FDerivedIndex );
    PRINT_METHOD_FLAG( pcprintf, FInitialIndex );
    PRINT_METHOD_FLAG( pcprintf, FInitialized );
    PRINT_METHOD_FLAG( pcprintf, FDeletePending );
    PRINT_METHOD_FLAG( pcprintf, FDeleteCommitted );
    PRINT_METHOD_FLAG( pcprintf, FUnique );
    PRINT_METHOD_FLAG( pcprintf, FNonUnique );
    PRINT_METHOD_FLAG( pcprintf, FNoCache );
    PRINT_METHOD_FLAG( pcprintf, FPreread );
    PRINT_METHOD_FLAG( pcprintf, FSpaceInitialized );
    PRINT_METHOD_FLAG( pcprintf, FTryPurgeOnClose );
    PRINT_METHOD_FLAG( pcprintf, FDontLogSpaceOps );
    PRINT_METHOD_FLAG( pcprintf, FIntrinsicLVsOnly );
    PRINT_METHOD_FLAG( pcprintf, FOLD2Running );
    PRINT_METHOD_FLAG( pcprintf, FUncommitted );
    PRINT_METHOD_FLAG( pcprintf, FValidatedCurrentLocales );
    PRINT_METHOD_FLAG( pcprintf, FTemplateStatic );
    PRINT_METHOD_FLAG( pcprintf, FInitedForRecovery );
    PRINT_METHOD_FLAG( pcprintf, FDoingAdditionalInitializationDuringRecovery );
    PRINT_METHOD_FLAG( pcprintf, FNoMoreTasks );

    EDBGDumplinkDml( FCB, this, RCE, m_prceNewest, dwOffset );
    EDBGDumplinkDml( FCB, this, RCE, m_prceOldest, dwOffset );

    EDBGDumplinkDml( FCB, this, PIB, m_ppibDomainDenyRead, dwOffset );
    (*pcprintf)( FORMAT_INT( FCB, this, m_crefDomainDenyRead, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FCB, this, m_crefDomainDenyWrite, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FCB, this, m_pgnoNextAvailSE, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( FCB, this, m_psplitbufdangling, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( FCB, this, m_bflPgnoFDP, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( FCB, this, m_bflPgnoOE, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( FCB, this, m_bflPgnoAE, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FCB, this, m_ctasksActive, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FCB, this, m_errInit, dwOffset ) );
    PRINT_SZ_ON_HEAP( pcprintf, FCB, this, m_szInitFile, dwOffset );
    (*pcprintf)( FORMAT_INT( FCB, this, m_lInitLine, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( FCB, this, m_critRCEList, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( FCB, this, m_sxwl, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( FCB, this, m_spacehints, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FCB, this, m_tickMNVNLastReported, dwOffset ) );

    PRINT_METHOD_FLAG( pcprintf, CMNVNIncidentsSinceLastReported );
    PRINT_METHOD_FLAG( pcprintf, CMNVNNodesSkippedSinceLastReported );
    PRINT_METHOD_FLAG( pcprintf, CMNVNPagesVisitedSinceLastReported );
}


//  ================================================================
VOID IDB::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    (*pcprintf)( FORMAT_INT( IDB, this, m_crefCurrentIndex, dwOffset ) );

    (*pcprintf)( FORMAT_INT( IDB, this, m_fidbPersisted, dwOffset ) );
    PRINT_METHOD_FLAG( pcprintf, FUnique );
    PRINT_METHOD_FLAG( pcprintf, FAllowAllNulls );
    PRINT_METHOD_FLAG( pcprintf, FAllowFirstNull );
    PRINT_METHOD_FLAG( pcprintf, FAllowSomeNulls );
    PRINT_METHOD_FLAG( pcprintf, FNoNullSeg );
    PRINT_METHOD_FLAG( pcprintf, FPrimary );
    PRINT_METHOD_FLAG( pcprintf, FLocaleSet );
    PRINT_METHOD_FLAG( pcprintf, FMultivalued );
    PRINT_METHOD_FLAG( pcprintf, FTemplateIndex );
    PRINT_METHOD_FLAG( pcprintf, FDerivedIndex );
    PRINT_METHOD_FLAG( pcprintf, FLocalizedText );
    PRINT_METHOD_FLAG( pcprintf, FSortNullsHigh );
//  PRINT_METHOD_FLAG( pcprintf, FUnicodeFixupOn_Deprecated );
    PRINT_METHOD_FLAG( pcprintf, FCrossProduct );
    PRINT_METHOD_FLAG( pcprintf, FDisallowTruncation );
    PRINT_METHOD_FLAG( pcprintf, FNestedTable );

    (*pcprintf)( FORMAT_INT( IDB, this, m_fidbNonPersisted, dwOffset ) );
    PRINT_METHOD_FLAG( pcprintf, FVersioned );
    PRINT_METHOD_FLAG( pcprintf, FDeleted );
    PRINT_METHOD_FLAG( pcprintf, FVersionedCreate );
    PRINT_METHOD_FLAG( pcprintf, FHasPlaceholderColumn );
    PRINT_METHOD_FLAG( pcprintf, FSparseIndex );
    PRINT_METHOD_FLAG( pcprintf, FSparseConditionalIndex );
    PRINT_METHOD_FLAG( pcprintf, FTuples );
    PRINT_METHOD_FLAG( pcprintf, FBadSortVersion );

    (*pcprintf)( FORMAT_INT( IDB, this, m_fidxPersisted, dwOffset ) );
    PRINT_METHOD_FLAG( pcprintf, FDotNetGuid );

    (*pcprintf)( FORMAT_INT( IDB, this, m_crefVersionCheck, dwOffset ) );
    (*pcprintf)( FORMAT_INT( IDB, this, m_itagIndexName, dwOffset ) );
    (*pcprintf)( FORMAT_INT( IDB, this, m_cbVarSegMac, dwOffset ) );
    (*pcprintf)( FORMAT_INT( IDB, this, m_cbKeyMost, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( IDB, this, m_cidxseg, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( IDB, this, m_cidxsegConditional, dwOffset ) );
    if ( FIsRgidxsegInMempool() )
    {
        (*pcprintf)( "\tItagRgidxseg: %d\n", ItagRgidxseg() );
    }
    else
    {
        (*pcprintf)( FORMAT_VOID( IDB, this, rgidxseg, dwOffset ) );
    }
    if ( FIsRgidxsegConditionalInMempool() )
    {
        (*pcprintf)( "\tItagRgidxsegConditional: %d\n", ItagRgidxsegConditional() );
    }
    else
    {
        (*pcprintf)( FORMAT_VOID( IDB, this, rgidxsegConditional, dwOffset ) );
    }
    if ( FTuples() )
    {
        (*pcprintf)( FORMAT_INT( IDB, this, m_cchTuplesLengthMin, dwOffset ) );
        (*pcprintf)( FORMAT_INT( IDB, this, m_cchTuplesLengthMin, dwOffset ) );
        (*pcprintf)( FORMAT_INT( IDB, this, m_ichTuplesToIndexMax, dwOffset ) );
        (*pcprintf)( FORMAT_INT( IDB, this, m_cchTuplesIncrement, dwOffset ) );
        (*pcprintf)( FORMAT_INT( IDB, this, m_ichTuplesStart, dwOffset ) );
    }

    (*pcprintf)( FORMAT_POINTER( IDB, this, m_pinst, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( IDB, this, m_rgbitIdx, dwOffset ) );

    NORM_LOCALE_VER nlvDebugger = { 0 };
    if ( FReadVariable<NORM_LOCALE_VER>( (NORM_LOCALE_VER*)( (DWORD_PTR)this + (DWORD_PTR)dwOffset + (DWORD_PTR)offsetof( IDB, m_nlv ) ), &nlvDebugger ) )
    {
        (*pcprintf)( FORMAT_WSZ( NORM_LOCALE_VER, &nlvDebugger, m_wszLocaleName, dwOffset ) );
        (*pcprintf)( FORMAT_INT( NORM_LOCALE_VER, &nlvDebugger, m_dwNormalizationFlags, dwOffset ) );
        (*pcprintf)( FORMAT_INT( NORM_LOCALE_VER, &nlvDebugger, m_dwNlsVersion, dwOffset ) );
        (*pcprintf)( FORMAT_INT( NORM_LOCALE_VER, &nlvDebugger, m_dwDefinedNlsVersion, dwOffset ) );
        (*pcprintf)( FORMAT_GUID( NORM_LOCALE_VER, &nlvDebugger, m_sortidCustomSortVersion, dwOffset ) );
    }
}


//  ================================================================
VOID MEMPOOL::Dump(
    CPRINTF *       pcprintf,
    const BOOL      fDumpTags,
    const BOOL      fDumpAll,
    const ITAG      itagDump,
    const DWORD_PTR dwOffset )
//  ================================================================
{
    (*pcprintf)( FORMAT_POINTER( MEMPOOL, this, m_pbuf, dwOffset ) );
    (*pcprintf)( FORMAT_INT( MEMPOOL, this, m_cbBufSize, dwOffset ) );
    (*pcprintf)( FORMAT_INT( MEMPOOL, this, m_ibBufFree, dwOffset ) );
    (*pcprintf)( FORMAT_INT( MEMPOOL, this, m_itagUnused, dwOffset ) );
    (*pcprintf)( FORMAT_INT( MEMPOOL, this, m_itagFreed, dwOffset ) );

    if ( fDumpTags )
    {
        BYTE *  rgbBufDebuggee  = Pbuf();
        BYTE *  rgbBuf;

        if ( FFetchVariable( rgbBufDebuggee, &rgbBuf, CbBufSize() ) )
        {
            SetPbuf( rgbBuf );

            dprintf( "\n" );
            if ( fDumpAll )
            {
                //  dump the entire mempool
                for ( ITAG itag = 0; itag < ItagUnused(); itag++ )
                {
                    DumpTag( pcprintf, itag, rgbBufDebuggee - rgbBuf );
                }
            }
            else
            {
                //  dump just the specified tag
                DumpTag( pcprintf, itagDump, rgbBufDebuggee - rgbBuf );
            }
            dprintf( "\n--------------------\n\n" );
            Unfetch( rgbBuf );
        }
        else
        {
            dprintf( "Error: Could not fetch MEMPOOL buffer.\n" );
        }
    }
}


//  ================================================================
VOID MEMPOOL::DumpTag( CPRINTF * pcprintf, const ITAG itag, const SIZE_T lOffset ) const
//  ================================================================
{
    MEMPOOLTAG  * const rgbTags = (MEMPOOLTAG *)Pbuf();
    if( 0 != rgbTags[itag].cb )
    {
        //  this tag is used
        (*pcprintf)( "TAG %3d        address:0x%N    cb:0x%04x    ib:0x%04x\n",
                    itag, (BYTE *)(&(rgbTags[itag])) + lOffset, rgbTags[itag].cb, rgbTags[itag].ib );
        (*pcprintf)( "\t%s", SzEDBGHexDump( Pbuf() + rgbTags[itag].ib, min( 32, rgbTags[itag].cb ) ) );
    }
    else
    {
        //  this is a free tag
        (*pcprintf)( "TAG %3d (FREE) address:0x%N    cb:0x%04x    ib:0x%04x\n",
                    itag, (BYTE *)(&(rgbTags[itag])) + lOffset, rgbTags[itag].cb, rgbTags[itag].ib );
    }
}


//  ================================================================
VOID PIB::DumpBasic( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    (*pcprintf)( FORMAT_UINT( PIB, this, dwTrxContext, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( PIB, this, trxBegin0, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( PIB, this, trxCommit0, dwOffset ) );
    (*pcprintf)( FORMAT_INT( PIB, this, m_level, dwOffset ) );
    (*pcprintf)( FORMAT_INT( PIB, this, levelRollback, dwOffset ) );
    (*pcprintf)( FORMAT_INT( PIB, this, levelBegin, dwOffset ) );
    (*pcprintf)( FORMAT_INT( PIB, this, clevelsDeferBegin, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( PIB, this, dwSessionContext, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( PIB, this, dwSessionContextThreadId, dwOffset ) );
}

//  ================================================================
VOID PIB::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    (*pcprintf)( FORMAT_UINT( PIB, this, trxBegin0, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( PIB, this, trxCommit0, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( PIB, this, m_pinst, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( PIB, this, dwTrxContext, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( PIB, this, m_fFlags, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fUserSession, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fAfterFirstBT, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fRecoveringEndAllSessions, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fOLD, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fSystemCallback, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fCIMCommitted, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fCIMDirty, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fSetAttachDB, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fUseSessionContextForTrxContext, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fBegin0Logged, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fLGWaiting, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fReadOnlyTrx, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fBatchIndexCreation, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fDistributedTrx, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( PIB, this, m_fPreparedToCommitTrx, dwOffset ) );

    (*pcprintf)( FORMAT_INT( PIB, this, m_cInJetAPI, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( PIB, this, prceNewest, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( PIB, this, pfucbOfSession, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( PIB, this, procid, dwOffset ) );
    (*pcprintf)( FORMAT_( PIB, this, rgcdbOpen, dwOffset ) );
    ULONG cMax = 0, cTotal = 0;
    for ( DBID dbid = 0; dbid < dbidMax; dbid++ )
    {
        (*pcprintf)( "[%hu]=%hu ", (USHORT)dbid, rgcdbOpen[dbid] );
        if ( dbid != dbidTemp )
        {
            cMax = max( cMax, (ULONG)(rgcdbOpen[dbid]) );
            cTotal += (ULONG)(rgcdbOpen[dbid]);
        }
    }
    if ( cMax == cTotal )
    {
        (*pcprintf)( "\n" );    // normal case
    }
    else
    {
        (*pcprintf)( "(Wow!!! More than one DB open on a single session, report this interesting client to eseall.)\n" );
    }

    (*pcprintf)( FORMAT_POINTER( PIB, this, ptlsApi, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( PIB, this, ptlsTrxBeginLast, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( PIB, this, ppibNext, dwOffset ) );

    (*pcprintf)( FORMAT_INT( PIB, this, m_level, dwOffset ) );
    (*pcprintf)( FORMAT_INT( PIB, this, levelBegin, dwOffset ) );
    (*pcprintf)( FORMAT_INT( PIB, this, clevelsDeferBegin, dwOffset ) );
    (*pcprintf)( FORMAT_INT( PIB, this, levelRollback, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( PIB, this, grbitCommitDefault, dwOffset ) );

    (*pcprintf)( FORMAT_INT( PIB, this, updateid, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( PIB, this, cCursors, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( PIB, this, m_ileTrxOldest, dwOffset ) );

    (*pcprintf)( FORMAT_LGPOS( PIB, this, lgposStart, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( PIB, this, lgposCommit0, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( PIB, this, m_pplsTrxOldest, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( PIB, this, dwSessionContext, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( PIB, this, dwSessionContextThreadId, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( PIB, this, m_pvRecordFormatConversionBuffer, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( PIB, this, critCursors, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( PIB, this, critConcurrentDDL, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( PIB, this, critLogBeginTrx, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( PIB, this, asigWaitLogWrite, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( PIB, this, ppibNextWaitWrite, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( PIB, this, ppibPrevWaitWrite, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( PIB, this, lgposWaitLogWrite, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( PIB, this, ppibHashOverflow, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( PIB, this, m_redblacktreeRceidDeferred, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( PIB, this, m_redblacktreePrceOfSession, dwOffset ) );
    
    (*pcprintf)( FORMAT_POINTER( PIB, this, m_pMacroNext, dwOffset ) );

    (*pcprintf)( FORMAT_INT( PIB, this, m_errRollbackFailure, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( PIB, this, m_trxidstack, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( PIB, this, m_critCachePriority, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( PIB, this, m_pctCachePriority, dwOffset ) );
    (*pcprintf)( FORMAT_( PIB, this, m_rgpctCachePriority, dwOffset ) );
    for ( DBID dbid = 0; dbid < dbidMax; dbid++ )
    {
        (*pcprintf)( "[%hu]=%hu ", (USHORT)dbid, m_rgpctCachePriority[dbid] );
    }
    (*pcprintf)( "\n" );

    (*pcprintf)( FORMAT_UINT( PIB, this, m_grbitUserIoPriority, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( PIB, this, m_qosIoPriority, dwOffset ) );
}

//  ================================================================
const WCHAR * FUCB::WszFUCBType() const
//  ================================================================
{
    if ( fSecondary )
    {
        return L"SecondaryIndex";
    }
    else if ( fIndex )
    {

        return L"PrimaryIndex";
    }
    else if ( fLV )
    {
        return L"LV";
    }
    else if ( fSort )
    {
        return L"Sort";
    }
    else
    {
        return L"Database";
    }
}

//  ================================================================
VOID FUCB::Dump( CPRINTF * pcprintf, DWORD_PTR ulBase ) const
//  ================================================================
{
    if( 0 == ulBase )
    {
        ulBase = reinterpret_cast<DWORD_PTR>( this );
    }

    (*pcprintf)( FORMAT_POINTER( FUCB, this, pvtfndef, ulBase ) );
    EDBGDumplinkDml( FUCB, this, PIB, ppib, ulBase );
    EDBGDumplinkDml( FUCB, this, FUCB, pfucbNextOfSession, ulBase );
    EDBGDumplinkDml( FUCB, this, FCB, u.pfcb, ulBase );
    (*pcprintf)( FORMAT_POINTER( FUCB, this, u.pscb, ulBase ) );
    (*pcprintf)( FORMAT_INT( FUCB, this, ifmp, ulBase ) );

    (*pcprintf)( FORMAT_UINT( FUCB, this, ulFlags, ulBase ) );

    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fIndex                 , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fSecondary             , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fCurrentSecondary      , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fLV                    , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fSort                  , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fSystemTable           , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fWrite                 , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fDenyRead              , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fDenyWrite             , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fPermitDDL             , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fDeferClose            , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fVersioned             , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fLimstat               , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fInclusive             , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fUpper                 , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fMayCacheLVCursor      , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fUpdateSeparateLV      , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fAnyColumnSet          , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fDeferredChecksum      , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fAvailExt              , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fOwnExt                , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fSequential            , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fPreread               , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fPrereadForward        , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fPrereadBackward       , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fBookmarkPreviouslySaved, ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fTouch                 , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fRepair                , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fAlwaysRetrieveCopy    , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fNeverRetrieveCopy     , ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fTagImplicitOp         , ulBase ) );

    (*pcprintf)( FORMAT_VOID( FUCB, this, csr, ulBase ) );

    (*pcprintf)( FORMAT_VOID( FUCB, this, kdfCurr, ulBase ) );
    (*pcprintf)( FORMAT_INT( FUCB, this, locLogical, ulBase ) );

    (*pcprintf)( FORMAT_POINTER( FUCB, this, pvBMBuffer, ulBase ) );
    (*pcprintf)( FORMAT_VOID( FUCB, this, bmCurr, ulBase ) );
    (*pcprintf)( FORMAT_VOID( FUCB, this, rgbBMCache, ulBase ) );

    EDBGDumplinkDml( FUCB, this, FUCB, pfucbCurIndex, ulBase );
    EDBGDumplinkDml( FUCB, this, FUCB, pfucbLV, ulBase );

    (*pcprintf)( FORMAT_INT( FUCB, this, ispairCurr, ulBase ) );

    (*pcprintf)( FORMAT_INT( FUCB, this, keystat, ulBase ) );
    (*pcprintf)( FORMAT_INT( FUCB, this, cColumnsInSearchKey, ulBase ) );

    (*pcprintf)( FORMAT_UINT( FUCB, this, usFlags, ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fUsingTableSearchKeyBuffer, ulBase ) );
    (*pcprintf)( FORMAT_BOOL_BF( FUCB, this, fInRecoveryTableHash, ulBase ) );

    (*pcprintf)( FORMAT_VOID( FUCB, this, dataSearchKey, ulBase ) );

    (*pcprintf)( FORMAT_INT( FUCB, this, rceidBeginUpdate, ulBase ) );
    (*pcprintf)( FORMAT_INT( FUCB, this, updateid, ulBase ) );
    (*pcprintf)( FORMAT_UINT( FUCB, this, ulChecksum, ulBase ) );

    (*pcprintf)( FORMAT_INT( FUCB, this, levelOpen, ulBase ) );
    (*pcprintf)( FORMAT_INT( FUCB, this, levelNavigate, ulBase ) );
    (*pcprintf)( FORMAT_INT( FUCB, this, levelPrep, ulBase ) );
    (*pcprintf)( FORMAT_INT( FUCB, this, levelReuse, ulBase ) );

    (*pcprintf)( FORMAT_INT( FUCB, this, cbstat, ulBase ) );

    (*pcprintf)( FORMAT_VOID( FUCB, this, dataWorkBuf, ulBase ) );
    (*pcprintf)( FORMAT_POINTER( FUCB, this, pvWorkBuf, ulBase ) );
    (*pcprintf)( FORMAT_POINTER( FUCB, this, pvRCEBuffer, ulBase ) );

    (*pcprintf)( FORMAT_VOID( FUCB, this, rgbitSet, ulBase ) );

    (*pcprintf)( FORMAT_INT( FUCB, this, cpgPreread, ulBase ) );
    (*pcprintf)( FORMAT_INT( FUCB, this, cpgPrereadNotConsumed, ulBase ) );
    (*pcprintf)( FORMAT_INT( FUCB, this, cbSequentialDataRead, ulBase ) );

    EDBGDumplinkDml( FUCB, this, FUCB, pfucbTable, ulBase );

    (*pcprintf)( FORMAT_UINT( FUCB, this, ulLTLast, ulBase ) );
    (*pcprintf)( FORMAT_UINT( FUCB, this, ulTotalLast, ulBase ) );
    (*pcprintf)( FORMAT_UINT( FUCB, this, ulLTCurr, ulBase ) );
    (*pcprintf)( FORMAT_UINT( FUCB, this, ulTotalCurr, ulBase ) );

    EDBGDumplinkDml( FUCB, this, CSR, pcsrRoot, ulBase );

    (*pcprintf)( FORMAT_VOID( FUCB, this, ls, ulBase ) );

    EDBGDumplinkDml( FUCB, this, FUCB, pfucbHashOverflow, ulBase );

    (*pcprintf)( FORMAT_UINT( FUCB, this, cbEncryptionKey, ulBase ) );
    (*pcprintf)( FORMAT_POINTER( FUCB, this, pbEncryptionKey, ulBase ) );

    (*pcprintf)( FORMAT_POINTER( FUCB, this, pmoveFilterContext, ulBase ) );

    (*pcprintf)( FORMAT_UINT( FUCB, this, cpgSpaceRequestReserve, ulBase ) );
}


//  ================================================================
VOID TDB::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    (*pcprintf)( FORMAT_INT( TDB, this, m_fidTaggedFirst, dwOffset ) );
    (*pcprintf)( FORMAT_INT( TDB, this, m_fidTaggedLastInitial, dwOffset ) );
    (*pcprintf)( FORMAT_INT( TDB, this, m_fidFixedFirst, dwOffset ) );
    (*pcprintf)( FORMAT_INT( TDB, this, m_fidFixedLastInitial, dwOffset ) );
    (*pcprintf)( FORMAT_INT( TDB, this, m_fidVarFirst, dwOffset ) );
    (*pcprintf)( FORMAT_INT( TDB, this, m_fidVarLastInitial, dwOffset ) );
    (*pcprintf)( FORMAT_INT( TDB, this, m_fidTaggedLast, dwOffset ) );
    (*pcprintf)( FORMAT_INT( TDB, this, m_fidFixedLast, dwOffset ) );
    (*pcprintf)( FORMAT_INT( TDB, this, m_fidVarLast, dwOffset ) );
    (*pcprintf)( FORMAT_INT( TDB, this, m_itagTableName, dwOffset ) );
    (*pcprintf)( FORMAT_INT( TDB, this, m_ibEndFixedColumns, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( TDB, this, m_pfieldsInitial, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( TDB, this, m_pdataDefaultRecord, dwOffset ) );

    EDBGDumplinkDml( TDB, this, FCB, m_pfcbTemplateTable, dwOffset );

    EDBGDumplinkDml( TDB, this, FCB, m_pfcbLV, dwOffset );
    (*pcprintf)( FORMAT_INT( TDB, this, m_lidLast, dwOffset ) );

    (*pcprintf)( FORMAT_INT( TDB, this, m_fidTaggedLastOfESE97Template, dwOffset ) );

    (*pcprintf)( FORMAT_INT( TDB, this, m_ulFlags, dwOffset ) );
    PRINT_METHOD_FLAG( pcprintf, FTemplateTable );
    PRINT_METHOD_FLAG( pcprintf, FESE97TemplateTable );
    PRINT_METHOD_FLAG( pcprintf, FDerivedTable );
    PRINT_METHOD_FLAG( pcprintf, FESE97DerivedTable );
    PRINT_METHOD_FLAG( pcprintf, F8BytesAutoInc );
    PRINT_METHOD_FLAG( pcprintf, FTableHasDefault );
    PRINT_METHOD_FLAG( pcprintf, FTableHasNonEscrowDefault );
    PRINT_METHOD_FLAG( pcprintf, FTableHasUserDefinedDefault );
    PRINT_METHOD_FLAG( pcprintf, FTableHasNonNullFixedColumn );
    PRINT_METHOD_FLAG( pcprintf, FTableHasNonNullVarColumn );
    PRINT_METHOD_FLAG( pcprintf, FTableHasNonNullTaggedColumn );
    PRINT_METHOD_FLAG( pcprintf, FInitialisingDefaultRecord );

    (*pcprintf)( FORMAT_INT( TDB, this, m_qwAutoincrement, dwOffset ) );
    (*pcprintf)( FORMAT_INT( TDB, this, m_fidVersion, dwOffset ) );
    (*pcprintf)( FORMAT_INT( TDB, this, m_fidAutoincrement, dwOffset ) );
    (*pcprintf)( FORMAT_INT( TDB, this, m_dbkMost, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( TDB, this, m_mempool, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( TDB, this, m_blIndexes, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( TDB, this, m_rgbitAllIndex, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( TDB, this, m_rwlDDL, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( TDB, this, m_pcbdesc, dwOffset ) );

    EDBGDumplinkDml( TDB, this, INST, m_pinst, dwOffset );

    (*pcprintf)( FORMAT_INT( TDB, this, m_cbLVChunkMost, dwOffset ) );
}

//  ================================================================
VOID INST::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    (*pcprintf)( FORMAT_POINTER( INST, this, m_rgEDBGGlobals, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( INST, this, m_rgfmp, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( INST, this, m_rgpinst, dwOffset ) );

    PRINT_WSZ_ON_HEAP( pcprintf, INST, this, m_wszInstanceName, dwOffset );
    PRINT_WSZ_ON_HEAP( pcprintf, INST, this, m_wszDisplayName, dwOffset );
    (*pcprintf)( FORMAT_INT( INST, this, m_iInstance, dwOffset ) );

    (*pcprintf)( FORMAT_INT( INST, this, m_cSessionInJetAPI, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( INST, this, m_fJetInitialized, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( INST, this, m_fTermInProgress, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( INST, this, m_fTermAbruptly, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( INST, this, m_fSTInit, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( INST, this, m_fNoWaypointLatency, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( INST, this, m_fBackupAllowed, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( INST, this, m_fStopJetService, dwOffset ) );
    (*pcprintf)( FORMAT_INT( INST, this, m_errInstanceUnavailable, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( INST, this, m_pOSSnapshotSession, dwOffset ) );

    EDBGPrintfDml(  "\t%*.*s &lt;0x%0*I64X,%3I64u&gt;:  <link cmd=\"!ese param 0x%N\">0x%N</link>  (<link cmd=\"!ese param global\">global</link>)\n",
                SYMBOL_LEN_MAX,
                SYMBOL_LEN_MAX,
                "m_rgparam",
                INT( 2 * sizeof( this ) ),
                __int64( (char*)(this) + dwOffset + OffsetOf( INST, m_rgparam ) ),
                __int64( sizeof( (this)->m_rgparam ) ),
                __int64( (char*)(this) + dwOffset ) /* debugee offset of this, not m_rgparam */,
                m_rgparam );

    (*pcprintf)( FORMAT_INT( INST, this, m_grbitCommitDefault, dwOffset ) );
    (*pcprintf)( FORMAT_INT( INST, this, m_dwLCMapFlagsDefault, dwOffset ) );
    (*pcprintf)( FORMAT_WSZ( INST, this, m_wszLocaleNameDefault, dwOffset ) );

    (*pcprintf)( FORMAT_INT( INST, this, m_config, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( INST, this, m_critLV, dwOffset ) );
    EDBGDumplinkDml( INST, this, PIB, m_ppibLV, dwOffset );

    EDBGDumplinkDml( INST, this, LOG, m_plog, dwOffset );
    EDBGDumplinkDml( INST, this, VER, m_pver, dwOffset );

    (*pcprintf)( FORMAT_VOID( INST, this, m_mpdbidifmp, dwOffset ) );

    (*pcprintf)( FORMAT_INT( INST, this, m_updateid, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( INST, this, m_critPIB, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( INST, this, m_rwlpoolPIBTrx, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( INST, this, m_cresPIB, dwOffset ) );
    EDBGDumplinkDml( INST, this, PIB, m_ppibGlobal, dwOffset );
    (*pcprintf)( FORMAT_INT( INST, this, m_trxNewest, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( INST, this, m_sigTrxOldestDuringRecovery, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( INST, this, m_cresFCB, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( INST, this, m_cresTDB, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( INST, this, m_cresIDB, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( INST, this, m_pfcbhash, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( INST, this, m_critFCBList, dwOffset ) );
    EDBGDumplinkDml( INST, this, FCB, m_pfcbList, dwOffset );
    EDBGDumplinkDml( INST, this, FCB, m_pfcbAvailMRU, dwOffset );
    EDBGDumplinkDml( INST, this, FCB, m_pfcbAvailLRU, dwOffset );
    (*pcprintf)( FORMAT_UINT( INST, this, m_cFCBAvail, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( INST, this, m_cFCBAlloc, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( INST, this, m_critFCBCreate, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( INST, this, m_cresFUCB, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( INST, this, m_cresSCB, dwOffset ) );

    (*pcprintf)( FORMAT_BOOL( INST, this, m_fFlushLogForCleanThread, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( INST, this, m_taskmgr, dwOffset ) );
    (*pcprintf)( FORMAT_INT( INST, this, m_cOpenedSystemPibs, dwOffset ) );
    (*pcprintf)( FORMAT_INT( INST, this, m_cUsedSystemPibs, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( INST, this, m_rgoldstatDB, dwOffset ) );

    EDBGDumplinkDml( INST, this, COSFileSystem, m_pfsapi, dwOffset );

    (*pcprintf)( FORMAT_POINTER_NOLINE( INST, this, m_rgpirs, dwOffset ) );
    CIrsOpContext ** rgpirs = NULL;
    if ( m_rgpirs && FFetchVariable( m_rgpirs, &rgpirs, dbidMax ) )
    {
        (*pcprintf)( " = { " );
        JET_DBID dbidMac = dbidMax - 1;
        while( dbidMac > 0 && rgpirs[dbidMac] == NULL )
            dbidMac--;  // just counting down to first non-null
        dbidMac++;
        for ( JET_DBID dbid = 0; dbid < dbidMac; dbid++ )
        {
            EDBGPrintfDml( "<link cmd=\"dt %ws!CIrsOpContext 0x%N\">0x%N</link>%hs ", WszUtilImageName(), rgpirs[dbid], rgpirs[dbid], dbid < ( dbidMac - 1 ) ? "," : ""  );
        }
        Unfetch( rgpirs );
    }
    (*pcprintf)( "}\n" );

    (*pcprintf)( FORMAT_INT( INST, this, m_cIOReserved, dwOffset ) );
    (*pcprintf)( FORMAT_INT( INST, this, m_cNonLoggedIndexCreators, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( INST, this, m_plnppibBegin, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( INST, this, m_plnppibEnd, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( INST, this, m_critLNPPIB, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( INST, this, m_critTempDbCreate, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( INST, this, m_cpls, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( INST, this, m_rgpls, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( INST, this, m_tickStopped, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( INST, this, m_tickStopCanceled, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( INST, this, m_grbitStopped, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( INST, this, m_fCheckpointQuiesce, dwOffset ) );

    // m_mpdbidifmp

    FMP *       rgfmpDebuggee = NULL;
    IFMP        ifmpMaxDebuggee     = g_ifmpMax;
    const BOOL  fReadGlobals        = ( FReadGlobal( "g_rgfmp", &rgfmpDebuggee )
                                        || NULL == rgfmpDebuggee
                                        && FReadGlobal( "g_ifmpMax", &ifmpMaxDebuggee ));

    (*pcprintf)( "\t\tDatabases:\n" );

    for ( DBID dbid = 0; dbid < dbidMax; dbid++ )
    {
        //  Unfortunately g_ifmpMax is the sentinel value used.  We should move to like ifmpNil or something.
        if ( m_mpdbidifmp[dbid] < ifmpMaxDebuggee )
        {
            (*pcprintf)( "\t\t\t [%d] = IFMP:0x%x (FMP:",
                            dbid,
                            m_mpdbidifmp[dbid] );
            if ( fReadGlobals )
            {
                (*pcprintf)( "0x%N)\n", rgfmpDebuggee + m_mpdbidifmp[dbid] );
            }
            else
            {
                (*pcprintf)( "\?\?\?)\n" );
            }
        }
    }

    dprintf( "\n--------------------\n\n" );
}


//  ================================================================
VOID FMP::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    (*pcprintf)( FORMAT_WSZ_IN_TARGET( FMP, this, m_wszDatabaseName, dwOffset ) );
    EDBGDumplinkDml( FMP, this, INST, m_pinst, dwOffset );
    (*pcprintf)( FORMAT_INT( FMP, this, m_dbid, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( FMP, this, m_fFlags, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fCreatingDB, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fAttachingDB, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fDetachingDB, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fExclusiveOpen, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fReadOnlyAttach, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fLogOn, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fVersioningOff, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fSkippedAttach, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fAttached, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fDeferredAttach, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fDeferredAttachConsistentFuture, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fIgnoreDeferredAttach, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fFailFastDeferredAttach, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fOverwriteOnCreate, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fRunningOLD, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fInBackupSession, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fAllowHeaderUpdate, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fDefragPreserveOriginal, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fCopiedPatchHeader, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fEDBBackupDone, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fDontRegisterOLD2Tasks, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fCacheAvail, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fMaintainMSObjids, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fNoWaypointLatency, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fAttachedForRecovery, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fRecoveryChecksDone, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fTrimSupported, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fContainsDataFromFutureLogs, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fOlderDemandExtendDb, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FMP, this, m_dbtimeLast, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FMP, this, m_dbtimeOldestGuaranteed, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FMP, this, m_dbtimeOldestCandidate, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FMP, this, m_dbtimeOldestTarget, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( FMP, this, m_trxOldestCandidate, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( FMP, this, m_trxOldestTarget, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( FMP, this, m_critDbtime, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FMP, this, m_dbtimeBeginRBS, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fRBSOn, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( FMP, this, m_fNeedUpdateDbtimeBeginRBS, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FMP, this, m_objidLast, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FMP, this, m_ctasksActive, dwOffset ) );

    EDBGDumplinkDml( FMP, this, COSFile, m_pfapi, dwOffset );

    (*pcprintf)( FORMAT_POINTER( FMP, this, m_pflushmap, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( FMP, this, m_pkvpsMSysLocales, dwOffset ) );

    EDBGDumplinkDml( FMP, this, DBFILEHDR, m_dbfilehdrLock.m_pdbfilehdr, dwOffset );
    (*pcprintf)( FORMAT_VOID( FMP, this, m_dbfilehdrLock.m_rwl, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( FMP, this, m_dbfilehdrLock.m_ptlsWriter, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FMP, this, m_dbfilehdrLock.m_cRecursion, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( FMP, this, m_critLatch, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( FMP, this, m_gateWriteLatch, dwOffset ) );

    (*pcprintf)( FORMAT_( FMP, this, m_cbOwnedFileSize, dwOffset ) );
    (*pcprintf)( "%I64u (0x%016I64x \\ PgnoLast() = %lu)\n", m_cbOwnedFileSize, m_cbOwnedFileSize, PgnoLast() );
    (*pcprintf)( FORMAT_( FMP, this, m_cbFsFileSizeAsyncTarget, dwOffset ) );
    AssertEDBG( m_cbOwnedFileSize <= m_cbFsFileSizeAsyncTarget );
    (*pcprintf)( "%I64u (0x%016I64x \\ PgnoLast() = %lu)\n", m_cbFsFileSizeAsyncTarget, m_cbFsFileSizeAsyncTarget, (ULONG)( m_cbFsFileSizeAsyncTarget / Pdls()->CbPage() - 2 ) );

    (*pcprintf)( FORMAT_VOID( FMP, this, m_semIOSizeChange, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FMP, this, m_cPin, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FMP, this, m_crefWriteLatch, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( FMP, this, m_ppibWriteLatch, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( FMP, this, m_ppibExclusiveOpen, dwOffset ) );

    (*pcprintf)( FORMAT_LGPOS( FMP, this, m_lgposAttach, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( FMP, this, m_lgposDetach, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( FMP, this, m_rwlDetaching, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( FMP, this, m_trxNewestWhenDiscardsLastReported, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FMP, this, m_cpgDatabaseSizeMax, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FMP, this, m_pgnoBackupMost, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FMP, this, m_pgnoBackupCopyMost, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FMP, this, m_pgnoSnapBackupMost, dwOffset ) );

#ifdef DEBUG
    (*pcprintf)( FORMAT_VOID( FMP, this, m_semRangeLock, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( FMP, this, m_msRangeLock, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( FMP, this, m_rgprangelock, dwOffset ) );
#else
    const BOOL fInRangeLockSetup = ( 0 == m_semRangeLock.CAvail() );
    AssertEDBG( m_semRangeLock.CAvail() == 0 || m_semRangeLock.CAvail() == 1 );
    (*pcprintf)( FORMAT_( FMP, this, m_semRangeLock, dwOffset ) );
    (*pcprintf)( "%d - %hs\n", m_semRangeLock.CAvail(), fInRangeLockSetup ? "LocksTransitioning!" : "LocksStabl[ish]" );

    const INT igroup = m_msRangeLock.GroupActive();
    const BOOL fQuiescing = m_msRangeLock.FQuiescing();
    (*pcprintf)( FORMAT_( FMP, this, m_msRangeLock, dwOffset ) );
    (*pcprintf)( "ActiveGroup = %d ( c = %d )", igroup, m_msRangeLock.CActiveUsers() );
    if ( fQuiescing )
    {
        (*pcprintf)( " QuiescingGroup ( c = %d )", m_msRangeLock.CQuiescingUsers() );
    }
    (*pcprintf)( "\n" );

    (*pcprintf)( FORMAT_( FMP, this, m_rgprangelock, dwOffset ) );
    for( LONG iprangelock = 0; iprangelock < _countof(m_rgprangelock); iprangelock++ )
    {
        const RANGELOCK * const prangelockDebuggee = m_rgprangelock[iprangelock];
        RANGELOCK rangelockBase;

        //  Consider not printing prangelockDebuggee out to save space ...
        (*pcprintf)( "[%d-%hs]( ", iprangelock,
                    ( iprangelock == igroup ) ? "Curr" : ( fQuiescing ? "Quiesce" : "Old" ) );
        
        if ( !FReadVariable( (RANGELOCK *)prangelockDebuggee, (RANGELOCK *)&rangelockBase ) )
        {
            (*pcprintf)( "EDBG-FAILED: Reading base rangelock " );
        }
        else
        {
            const SIZE_T cbrangelock = sizeof( RANGELOCK ) + rangelockBase.crangeMax * sizeof( RANGE );
            BYTE rgbRangelockReasonable[sizeof( RANGELOCK ) + 20 /* that'd be A LOT of range locks */ * sizeof( RANGE ) ];
            const RANGELOCK * const prangelock = (RANGELOCK *)rgbRangelockReasonable;   // do away with casts distributed through code

            (*pcprintf)( "%d/%d ", rangelockBase.crange, rangelockBase.crangeMax );

            if ( cbrangelock >= sizeof(rgbRangelockReasonable) ||
                    !FReadVariable( (BYTE*)prangelockDebuggee, rgbRangelockReasonable, min( cbrangelock, sizeof(rgbRangelockReasonable) ) ) )
            {
                (*pcprintf)( "[xxx]EDBG-FAILED: Reading %d ranges ", rangelockBase.crangeMax );
            }
            else
            {
                AssertEDBG( rangelockBase.crange == prangelock->crange );
                AssertEDBG( rangelockBase.crangeMax == prangelock->crangeMax );
                
                if ( prangelock->crange )
                {
                    for( ULONG irange = 0; irange < prangelock->crange; irange++ )
                    {
                        (*pcprintf)( "[%d]0x%x-0x%x ", irange,
                                        prangelock->rgrange[irange].pgnoStart,
                                        prangelock->rgrange[irange].pgnoEnd );
                    }
                }
                else
                {
                    (*pcprintf)( "[Empty] " );
                }
            }
            
        }
        (*pcprintf)( ") " );
    }
    (*pcprintf)( "\n" );
#endif  //  DEBUG

    (*pcprintf)( FORMAT_POINTER( FMP, this, m_dwBFContext, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( FMP, this, m_rwlBFContext, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FMP, this, m_cpagePatch, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FMP, this, m_cPatchIO, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( FMP, this, m_pfapiPatch, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( FMP, this, m_ppatchhdr, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FMP, this, m_dbtimeCurrentDuringRecovery, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FMP, this, m_dbtimeLastScrub, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( FMP, this, m_logtimeScrub, dwOffset ) );

    PRINT_WSZ_ON_HEAP( pcprintf, FMP, this, m_wszPatchPath, dwOffset );

    (*pcprintf)( FORMAT_POINTER( FMP, this, m_patchchk, dwOffset ) );

    (*pcprintf)( FORMAT_LGPOS( FMP, this, m_lgposWaypoint, dwOffset ) );

    (*pcprintf)( FORMAT_INT( FMP, this, m_cAsyncIOForViewCachePending, dwOffset ) );

    (*pcprintf)( FORMAT_BOOL( FMP, this, m_fScheduledPeriodicTrim, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( FMP, this, m_fDontStartDBM, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( FMP, this, m_fDontStartOLD, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( FMP, this, m_fDontStartTrimTask, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( FMP, this, m_pctCachePriority, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( FMP, this, m_grbitShrinkDatabaseOptions, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FMP, this, m_dtickShrinkDatabaseTimeQuota, dwOffset ) );
    (*pcprintf)( FORMAT_INT( FMP, this, m_cpgShrinkDatabaseSizeLimit, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( FMP, this, m_sxwlRedoMaps, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( FMP, this, m_pLogRedoMapZeroed, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( FMP, this, m_pLogRedoMapBadDbtime, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( FMP, this, m_pLogRedoMapDbtimeRevert, dwOffset ) );
}

INLINE ERR CHECKPOINT::Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset ) const
{

    (*pcprintf)( FORMAT_UINT( CHECKPOINT, this, checkpoint.le_ulChecksum, dwOffset ) );
    (*pcprintf)( FORMAT_LE_LGPOS( CHECKPOINT, this, checkpoint.le_lgposLastFullBackupCheckpoint, dwOffset ) );
    (*pcprintf)( FORMAT_LE_LGPOS( CHECKPOINT, this, checkpoint.le_lgposCheckpoint, dwOffset ) );
    (*pcprintf)( FORMAT_LE_LGPOS( CHECKPOINT, this, checkpoint.le_lgposDbConsistency, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( CHECKPOINT, this, checkpoint.signLog, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( CHECKPOINT, this, checkpoint.dbms_param, dwOffset ) );

    (*pcprintf)( FORMAT_LE_LGPOS( CHECKPOINT, this, checkpoint.le_lgposFullBackup, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( CHECKPOINT, this, checkpoint.logtimeFullBackup, dwOffset ) );
    (*pcprintf)( FORMAT_LE_LGPOS( CHECKPOINT, this, checkpoint.le_lgposIncBackup, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( CHECKPOINT, this, checkpoint.logtimeIncBackup, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( CHECKPOINT, this, checkpoint.rgbReserved1, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( CHECKPOINT, this, checkpoint.le_filetype, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( CHECKPOINT, this, checkpoint.rgbReserved2, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( CHECKPOINT, this, rgbAttach, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( CHECKPOINT, this, rgbPadding, dwOffset ) );

    return JET_errSuccess;
}

//  ================================================================
VOID LOG::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    EDBGDumplinkDml( LOG, this, INST, m_pinst, dwOffset );

    EDBGDumplinkDml( LOG, this, LOG_STREAM, m_pLogStream, dwOffset );
    (*pcprintf)( FORMAT_POINTER( LOG, this, m_pLogReadBuffer, dwOffset ) );
    EDBGDumplinkDml( LOG, this, LOG_WRITE_BUFFER, m_pLogWriteBuffer, dwOffset );

    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fLogInitialized, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fLogDisabled, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fLogDisabledDueToRecoveryFailure, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fLGNoMoreLogWrite, dwOffset ) );
    (*pcprintf)( FORMAT_INT( LOG, this, m_errNoMoreLogWrite, dwOffset ) );

    (*pcprintf)( FORMAT_INT( LOG, this, m_errCheckpointUpdate, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( LOG, this, m_lgposCheckpointUpdateError, dwOffset ) );

    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fRecovering, dwOffset ) );
    (*pcprintf)( FORMAT_INT( LOG, this, m_fRecoveringMode, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fRecoveryUndoLogged, dwOffset ) );

    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fHardRestore, dwOffset ) );

    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fLGFMPLoaded, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( LOG, this, m_signLog, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fSignLogSet, dwOffset ) );

    PRINT_WSZ_ON_HEAP( pcprintf, LOG, this, m_wszLogCurrent, dwOffset );

    PRINT_WSZ_ON_HEAP( pcprintf, LOG, this, m_wszChkExt, dwOffset );

    (*pcprintf)( FORMAT_LGPOS( LOG, this, m_lgposStart, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( LOG, this, m_lgposRecoveryUndo, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( LOG, this, m_critLGTrace, dwOffset ) );

    EDBGDumplinkDml( LOG, this, CHECKPOINT, m_pcheckpoint, dwOffset );

    (*pcprintf)( FORMAT_VOID( LOG, this, m_critCheckpoint, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( LOG, this, m_fDisableCheckpoint, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( LOG, this, m_fPendingRedoMapEntries, dwOffset ) );

    (*pcprintf)( FORMAT_BOOL_BF( LOG, this, m_fTruncateLogsAfterRecovery, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( LOG, this, m_fIgnoreLostLogs, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( LOG, this, m_fLostLogs, dwOffset ) );
#ifdef DEBUG
    (*pcprintf)( FORMAT_BOOL_BF( LOG, this, m_fEventedLLRDatabases, dwOffset ) );
#endif
    (*pcprintf)( FORMAT_BOOL_BF( LOG, this, m_fReplayingIgnoreMissingDB, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( LOG, this, m_fReplayMissingMapEntryDB, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( LOG, this, m_fReplayIgnoreLogRecordsBeforeMinRequiredLog, dwOffset ) );

    (*pcprintf)( FORMAT_LGPOS( LOG, this, m_lgposRedo, dwOffset ) );

    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fAbruptEnd, dwOffset ) );

    (*pcprintf)( FORMAT_LGPOS( LOG, this, m_lgposPbNextPreread, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( LOG, this, m_lgposPbNextNextPreread, dwOffset ) );
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    (*pcprintf)( FORMAT_LGPOS( LOG, this, m_lgposLastChecksumPreread, dwOffset ) );
#endif
    (*pcprintf)( FORMAT_POINTER( LOG, this, m_pPrereadWatermarks, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fPreread, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( LOG, this, m_plpreread, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fIODuringRecovery, dwOffset ) );

    (*pcprintf)( FORMAT_INT( LOG, this, m_errGlobalRedoError, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( LOG, this, m_fAfterEndAllSessions, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( LOG, this, m_fLastLRIsShutdown, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( LOG, this, m_fNeedInitialDbList, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( LOG, this, m_lgposRedoShutDownMarkGlobal, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( LOG, this, m_lgposRedoLastTerm, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( LOG, this, m_pctablehash, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( LOG, this, m_pcsessionhash, dwOffset ) );

    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fUseRecoveryLogFileSize, dwOffset ) );
    (*pcprintf)( FORMAT_INT( LOG, this, m_lLogFileSizeDuringRecovery, dwOffset ) );

    (*pcprintf)( FORMAT_LGPOS( LOG, this, m_lgposRecoveryStop, dwOffset ) );

    (*pcprintf)( FORMAT_WSZ( LOG, this, m_wszRestorePath, dwOffset ) );
    (*pcprintf)( FORMAT_WSZ( LOG, this, m_wszNewDestination, dwOffset ) );

    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fExternalRestore, dwOffset ) );
    (*pcprintf)( FORMAT_INT( LOG, this, m_lGenLowRestore, dwOffset ) );
    (*pcprintf)( FORMAT_INT( LOG, this, m_lGenHighRestore, dwOffset ) );

    (*pcprintf)( FORMAT_WSZ( LOG, this, m_wszTargetInstanceLogPath, dwOffset ) );
    (*pcprintf)( FORMAT_INT( LOG, this, m_lGenHighTargetInstance, dwOffset ) );

    (*pcprintf)( FORMAT_BOOL_BF( LOG, this, m_fDumpingLogs, dwOffset ) );
    (*pcprintf)( FORMAT_UINT_BF( LOG, this, m_iDumpVerbosityLevel, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( LOG, this, m_pttFirst, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( LOG, this, m_pttLast, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( LOG, this, m_rgrstmap, dwOffset ) );
    (*pcprintf)( FORMAT_INT( LOG, this, m_irstmapMac, dwOffset ) );

#ifdef DEBUG
    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fDBGFreezeCheckpoint, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fDBGTraceRedo, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fDBGTraceBR, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG, this, m_fDBGNoLog, dwOffset ) );
#endif

    (*pcprintf)( FORMAT_INT( LOG, this, m_cNOP, dwOffset ) );
}


//  ================================================================
VOID BACKUP_CONTEXT::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    (*pcprintf)( FORMAT_LGPOS( BACKUP_CONTEXT, this, m_lgposFullBackup, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( BACKUP_CONTEXT, this, m_logtimeFullBackup, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( BACKUP_CONTEXT, this, m_lgposIncBackup, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( BACKUP_CONTEXT, this, m_logtimeIncBackup, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( BACKUP_CONTEXT, this, m_critBackupInProgress, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( BACKUP_CONTEXT, this, m_fBackupInProgressAny, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( BACKUP_CONTEXT, this, m_fBackupInProgressLocal, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( BACKUP_CONTEXT, this, m_fStopBackup, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( BACKUP_CONTEXT, this, m_lgposFullBackupMark, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( BACKUP_CONTEXT, this, m_logtimeFullBackupMark, dwOffset ) );
    (*pcprintf)( FORMAT_INT( BACKUP_CONTEXT, this, m_lgenCopyMic, dwOffset ) );
    (*pcprintf)( FORMAT_INT( BACKUP_CONTEXT, this, m_lgenCopyMac, dwOffset ) );
    (*pcprintf)( FORMAT_INT( BACKUP_CONTEXT, this, m_lgenDeleteMic, dwOffset ) );
    (*pcprintf)( FORMAT_INT( BACKUP_CONTEXT, this, m_lgenDeleteMac, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( BACKUP_CONTEXT, this, m_ppibBackup, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( BACKUP_CONTEXT, this, m_fBackupFull, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( BACKUP_CONTEXT, this, m_fBackupBeginNewLogFile, dwOffset ) );

    (*pcprintf)( FORMAT_INT( BACKUP_CONTEXT, this, m_fBackupStatus, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( BACKUP_CONTEXT, this, m_rgrhf, dwOffset ) );
    (*pcprintf)( FORMAT_INT( BACKUP_CONTEXT, this, m_crhfMac, dwOffset ) );

#ifdef DEBUG
    (*pcprintf)( FORMAT_INT( BACKUP_CONTEXT, this, m_cbDBGCopied, dwOffset ) );
#endif
}


//  ================================================================
VOID LOG_BUFFER::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    (*pcprintf)( FORMAT_UINT( LOG_BUFFER, this, _csecLGBuf, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( LOG_BUFFER, this, _pbLGBufMin, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( LOG_BUFFER, this, _pbLGBufMax, dwOffset ) );
    
    (*pcprintf)( FORMAT_UINT( LOG_BUFFER, this, _cbLGBuf, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( LOG_BUFFER, this, _pbEntry, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( LOG_BUFFER, this, _pbWrite, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( LOG_BUFFER, this, _isecWrite, dwOffset ) );

    (*pcprintf)( FORMAT_LGPOS( LOG_BUFFER, this, _lgposToWrite, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( LOG_BUFFER, this, _lgposFlushTip, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( LOG_BUFFER, this, _lgposMaxWritePoint, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( LOG_BUFFER, this, _pbLGFileEnd, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( LOG_BUFFER, this, _isecLGFileEnd, dwOffset ) );
}

//  ================================================================
VOID LOG_WRITE_BUFFER::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    (*pcprintf)( FORMAT_BOOL( LOG_WRITE_BUFFER, this, m_fNewLogRecordAdded, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( LOG_WRITE_BUFFER, this, m_lgposLogRec, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( LOG_WRITE_BUFFER, this, m_lgposLogRecBasis, dwOffset ) );

    (*pcprintf)( FORMAT_BOOL( LOG_WRITE_BUFFER, this, m_fHaveShadow, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( LOG_WRITE_BUFFER, this, m_semLogSignal, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( LOG_WRITE_BUFFER, this, m_semLogWrite, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( LOG_WRITE_BUFFER, this, m_semWaitForLogBufferSpace, dwOffset ) );

    dprintf( "                     m_critLGBuf <                  ,  .>:  m_critLGBuf is a virtual member that redirects to: m_pLogBuffer->_critLGBuf.\n" );
    //(*pcprintf)( FORMAT_VOID( LOG_WRITE_BUFFER, this, m_critLGBuf, dwOffset ) ); // this crashes debugger, someone should complete the log rewrite and remove refs to m_critLGBuf in LOG_WRITE_BUFFER, then remove #define so this wouldn't compile.
    (*pcprintf)( FORMAT_VOID( LOG_WRITE_BUFFER, this, m_critLGWaitQ, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( LOG_WRITE_BUFFER, this, m_msLGPendingCopyIntoBuffer, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( LOG_WRITE_BUFFER, this, m_ppibLGWriteQHead, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( LOG_WRITE_BUFFER, this, m_ppibLGWriteQTail, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( LOG_WRITE_BUFFER, this, m_pvPartialSegment, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( LOG_WRITE_BUFFER, this, m_critNextLazyCommit, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( LOG_WRITE_BUFFER, this, m_tickNextLazyCommit, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( LOG_WRITE_BUFFER, this, m_lgposNextLazyCommit, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( LOG_WRITE_BUFFER, this, m_cLGWrapAround, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG_WRITE_BUFFER, this, m_fStopOnNewLogGeneration, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG_WRITE_BUFFER, this, m_fWaitForLogRecordAfterStopOnNewGen, dwOffset ) );

    (*pcprintf)( FORMAT_BOOL( LOG_WRITE_BUFFER, this, m_fLogPaused, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( LOG_WRITE_BUFFER, this, m_sigLogPaused, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( LOG_WRITE_BUFFER, this, m_tickLastWrite, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( LOG_WRITE_BUFFER, this, m_tickDecommitTaskSchedule, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( LOG_WRITE_BUFFER, this, m_postLazyCommitTask, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( LOG_WRITE_BUFFER, this, m_postDecommitTask, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG_WRITE_BUFFER, this, m_fDecommitTaskScheduled, dwOffset ) );

#ifdef DEBUG
    (*pcprintf)( FORMAT_LGPOS( LOG_WRITE_BUFFER, this, m_lgposLastLogRec, dwOffset ) );
    (*pcprintf)( FORMAT_INT( LOG_WRITE_BUFFER, this, m_dwDBGLogThreadId, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG_WRITE_BUFFER, this, m_fDBGTraceLog, dwOffset ) );
#endif
}


//  ================================================================
VOID LOG_STREAM::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    (*pcprintf)( FORMAT_INT( LOG_STREAM, this, m_ls, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG_STREAM, this, m_fLogSequenceEnd, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( LOG_STREAM, this, m_csecLGFile, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( LOG_STREAM, this, m_plgfilehdr, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( LOG_STREAM, this, m_plgfilehdrT, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( LOG_STREAM, this, m_pfapiLog, dwOffset ) );

    (*pcprintf)( FORMAT_WSZ( LOG_STREAM, this, m_wszLogName, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( LOG_STREAM, this, m_tickNewLogFile, dwOffset ) );

    (*pcprintf)( FORMAT_POINTER( LOG_STREAM, this, m_pfapiJetTmpLog, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG_STREAM, this, m_fCreateAsynchResUsed, dwOffset ) );
    (*pcprintf)( FORMAT_INT( LOG_STREAM, this, m_errCreateAsynch, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( LOG_STREAM, this, m_asigCreateAsynchIOCompleted, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( LOG_STREAM, this, m_critCreateAsynchIOExecuting, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( LOG_STREAM, this, m_critJetTmpLog, dwOffset ) );
    (*pcprintf)( FORMAT_INT( LOG_STREAM, this, m_ibJetTmpLog, dwOffset ) );
    (*pcprintf)( FORMAT_INT( LOG_STREAM, this, m_cIOTmpLog, dwOffset ) );
    (*pcprintf)( FORMAT_INT( LOG_STREAM, this, m_cIOTmpLogMax, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( LOG_STREAM, this, m_rgibTmpLog, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( LOG_STREAM, this, m_rgcbTmpLog, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( LOG_STREAM, this, m_lgposCreateAsynchTrigger, dwOffset ) );

    PRINT_WSZ_ON_HEAP( pcprintf, LOG_STREAM, this, m_wszLogExt, dwOffset );

    (*pcprintf)( FORMAT_LGPOS( LOG_STREAM, this, m_lgposLogRecAsyncIOIssue, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( LOG_STREAM, this, m_cbSec_, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( LOG_STREAM, this, m_cbSecVolume, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( LOG_STREAM, this, m_csecHeader, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( LOG_STREAM, this, m_rwlLGResFiles, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( LOG_STREAM, this, m_critLGWrite, dwOffset ) );

#ifdef DEBUG
    (*pcprintf)( FORMAT_BOOL_BF( LOG_STREAM, this, m_fResettingLogStream, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( LOG_STREAM, this, m_fDBGTraceBR, dwOffset ) );
#endif
    (*pcprintf)( FORMAT_BOOL_BF( LOG_STREAM, this, m_cRemovedLogs, dwOffset ) );
}


//  ================================================================
VOID VER::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    EDBGDumplinkDml( VER, this, INST, m_pinst, dwOffset );

    (*pcprintf)( FORMAT_INT( VER, this, m_fVERCleanUpWait, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( VER, this, m_tickLastRCEClean, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( VER, this, m_msigRCECleanPerformedRecently, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( VER, this, m_asigRCECleanDone, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( VER, this, m_critRCEClean, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( VER, this, m_critBucketGlobal, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( VER, this, m_pbucketGlobalHead, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( VER, this, m_pbucketGlobalTail, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( VER, this, m_cresBucket, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( VER, this, m_cbBucket, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( VER, this, m_rceidLast, dwOffset ) );
    EDBGDumplinkDml( VER, this, PIB, m_ppibRCEClean, dwOffset );
    EDBGDumplinkDml( VER, this, PIB, m_ppibRCECleanCallback, dwOffset );
    EDBGDumplinkDml( VER, this, PIB, m_ppibTrxOldestLastLongRunningTransaction, dwOffset );
    (*pcprintf)( FORMAT_UINT( VER, this, m_dwTrxContextLastLongRunningTransaction, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( VER, this, m_trxBegin0LastLongRunningTransaction, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( VER, this, m_fSyncronousTasks, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( VER, this, m_crceheadHashTable, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( VER, this, m_rectaskbatcher, dwOffset ) );
//  (*pcprintf)( FORMAT_VOID( VER, this, m_rgrceheadHashTable, dwOffset ) );
}


//  ================================================================
VOID CResource::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    (*pcprintf)( FORMAT_POINTER( CResource, this, m_pRM, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( CResource, this, m_quota, dwOffset ) );
}


//  ================================================================
VOID CResourceManager::Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const
//  ================================================================
{
    (*pcprintf)( FORMAT_VOID( CResourceManager, this, m_lookaside, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CResourceManager, this, m_cAvgCount, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( CResourceManager, this, m_pRCIList, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( CResourceManager, this, m_pRCINotFullList, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( CResourceManager, this, m_pRCIFreeList, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cFreeRCI, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cAllocatedRCI, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cCResourceLinks, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cbSectionHeader, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cbAlignedObject, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cObjectsPerSection, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cObjectsPerChunk, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cAllocatedRCIMax, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cAllocatedChunksMin, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_resid, dwOffset ) );
    (*pcprintf)( FORMAT_SZ( CResourceManager, this, m_rgchTag, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cbObjectAlign, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cbObjectSize, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cbChunkSize, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cbRFOLOffset, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cObjectsMax, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_cObjectsMin, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CResourceManager, this, m_ulFlags, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( CResourceManager, this, m_fGuarded, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( CResourceManager, this, m_fAllocTopDown, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( CResourceManager, this, m_fPreserveFreed, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( CResourceManager, this, m_fAllocFromHeap, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( CResourceManager, this, m_critLARefiller, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( CResourceManager, this, m_critInitTerm, dwOffset ) );
}

//  ================================================================
CResourceManager *CRMContainer::EDBGPRMFind( JET_RESID resid )
//  ================================================================
{
    FetchWrap<CRMContainer *> pRMC;
    CRMContainer *pRMCNext;
    if ( FReadGlobal( "g_pRMContainer", &pRMCNext ) )
    {
        while ( NULL != pRMCNext )
        {
            if ( pRMC.FVariable( pRMCNext ) )
            {
                if ( resid == pRMC->m_RM.ResID() )
                {
                    return (CResourceManager *)( (CHAR *)pRMCNext + OffsetOf( CRMContainer, m_RM ) );
                }
                pRMCNext = pRMC->m_pNext;
            }
            else
            {
                dprintf( "Cannot fetch the resource manager.\n" );
            }
        }
    }
    else
    {
        dprintf( "Cannot access the RM container.\n" );
    }
    return NULL;
}


//  ================================================================
DEBUG_EXT( EDBGDumpAllFMPs )
//  ================================================================
{
    FMP *   rgfmpDebuggee               = NULL;
    ULONG   ifmpMaxDebuggee             = 0;        //  don't use ifmpNil because its type is IFMP
    ULONG   cfmpMacCommittedDebuggee    = 0;
    BOOL    fDumpAll                    = fFalse;
    BOOL    fValidUsage;

    switch ( argc )
    {
        case 0:
            //  use defaults
            fValidUsage = fTrue;
            break;
        case 1:
            //  '*' only
            fValidUsage = ( '*' == argv[0][0] );
            break;
        case 2:
            //  <g_rgfmp> and <g_ifmpMax>
            fValidUsage = ( FAddressFromSz( argv[0], &rgfmpDebuggee )
                            && FUlFromSz( argv[1], &ifmpMaxDebuggee ) );
            break;
        case 3:
            if ( '*' == argv[0][0] )
            {
                //  '*' followed by <g_rgfmp> and <g_ifmpMax>
                fDumpAll = fTrue;
                fValidUsage = ( FAddressFromSz( argv[1], &rgfmpDebuggee )
                                && FUlFromSz( argv[2], &ifmpMaxDebuggee ) );
            }
            else if ( '*' == argv[2][0] )
            {
                //  <g_rgfmp> and <g_ifmpMax> followed by '*'
                fDumpAll = fTrue;
                fValidUsage = ( FAddressFromSz( argv[0], &rgfmpDebuggee )
                                && FUlFromSz( argv[1], &ifmpMaxDebuggee ) );
            }
            else
            {
                //  neither first nor third argument is a '*', so must be an error
                fValidUsage = fFalse;
            }
            break;
        default:
            fValidUsage = fFalse;
            break;
    }

    if ( !fValidUsage )
    {
        dprintf( "Usage: DUMPFMPS [<g_rgfmp> <g_ifmpMax>] [*]\n" );
        return;
    }

    if ( NULL == rgfmpDebuggee )
    {
        if ( !FReadGlobal( "g_rgfmp", &rgfmpDebuggee )
            || NULL == rgfmpDebuggee
            || !FReadGlobal( "g_ifmpMax", &ifmpMaxDebuggee )
            || !FReadGlobal( "FMP::s_ifmpMacCommitted", &cfmpMacCommittedDebuggee ) )
        {
            dprintf( "Error: Could not fetch FMP variables.\n" );
            return;
        }
    }

    dprintf( "\nScanning 0x%X (of 0x%X) FMPs starting at 0x%N...\n", cfmpMacCommittedDebuggee, ifmpMaxDebuggee, rgfmpDebuggee );

    for ( IFMP ifmp = cfmpReserved; ifmp < cfmpMacCommittedDebuggee; ifmp++ )
    {
        FMP *   pfmp                = NULL;
        WCHAR * wszDatabaseName     = NULL;

        if ( !FFetchVariable( rgfmpDebuggee + ifmp, &pfmp ) ||
            ( pfmp->FInUse() && !FFetchSz( pfmp->WszDatabaseName(), &wszDatabaseName ) ) )
        {
            dprintf(    "\n g_rgfmp[0x%x]  Error: Could not fetch FMP at 0x%N. Aborting.\n",
                        ifmp,
                        rgfmpDebuggee + ifmp );

            //  force out of loop
            ifmp = cfmpMacCommittedDebuggee;
        }
        else
        {
            if ( wszDatabaseName || fDumpAll )
            {
                EDBGPrintfDml(  "\n g_rgfmp[0x%x]  (FMP <link cmd=\"!ese dump fmp 0x%N\">0x%N</link>)\n",
                            ifmp,
                            rgfmpDebuggee + ifmp,
                            rgfmpDebuggee + ifmp );
                dprintf(    "           \t %*.*s : %ws\n",
                            SYMBOL_LEN_MAX,
                            SYMBOL_LEN_MAX,
                            "m_wszDatabaseName",
                            wszDatabaseName );
                EDBGPrintfDml(  "           \t %*.*s : <link cmd=\"!ese dump inst 0x%N\">0x%N</link>\n",
                            SYMBOL_LEN_MAX,
                            SYMBOL_LEN_MAX,
                            "m_pinst",
                            pfmp->Pinst(),
                            pfmp->Pinst() );
            }
        }

        Unfetch( pfmp );
        Unfetch( wszDatabaseName );
    }

    dprintf( "\n--------------------\n\n" );
}

//  ================================================================
DEBUG_EXT( EDBGDumpAllINSTs )
//  ================================================================
{
    INST **     rgpinstDebuggee     = NULL;
    INST **     rgpinst             = NULL;
    ULONG       cpinstMax           = 0;    //  cMaxInstances is now variable in essence.

    if ( 0 != argc
        && ( 1 != argc || !FAddressFromSz( argv[0], &rgpinstDebuggee ) )
        && ( 2 != argc || !FAddressFromSz( argv[0], &rgpinstDebuggee ) || !FUlFromSz( argv[1], &cpinstMax, 10 ) ) )
    {
        dprintf( "Usage: DUMPINSTS [<g_rgpinst>] [<g_cinstMax>]\n" );
        return;
    }

    if ( cpinstMax == 0 &&
            !FReadGlobal( "g_cpinstMax", &cpinstMax ) )
    {
        dprintf( "Error: Could not fetch instance table size.\n" );
        return;
    }

    if ( NULL == rgpinstDebuggee &&
            !FReadGlobal( "g_rgpinst", &rgpinstDebuggee ) )
    {
        dprintf( "Error: Could not fetch instance table address.\n" );
    }

    if ( !FFetchVariable( rgpinstDebuggee, &rgpinst, cpinstMax ) )
    {
        dprintf( "Error: Could not fetch instance table.\n" );
        return;
    }

    dprintf( "\nScanning 0x%X (of %#X possible) INST's starting at 0x%N...\n", cpinstMax, cMaxInstances, rgpinstDebuggee );

    for ( SIZE_T ipinst = 0; ipinst < cpinstMax; ipinst++ )
    {
        if ( rgpinst[ipinst] != pinstNil )
        {
            INST *  pinst   = NULL;

            //  validate our DLS
            if ( rgpinst[ipinst] != Pdls()->PinstDebuggee( ipinst ) )
            {
                Pdls()->AddWarning( "Debugger DLS - PinstDebuggee() state doesn't match, some debugger extensions not trustable. May happen on live debug though." );
            }

            if ( !FFetchVariable( rgpinst[ ipinst ], &pinst ) )
            {
                dprintf(    "\n g_rgpinst[0x%x]  Error: Could not fetch INST at 0x%N. Aborting.\n",
                            ipinst,
                            rgpinst[ipinst] );

                //  force out of loop
                ipinst = cpinstMax;
            }
            else
            {
                WCHAR *     wszInstanceName = NULL;
                WCHAR *     wszDisplayName  = NULL;
                VER *       pver            = NULL;
                LOG *       plog            = NULL;
                DWORD_PTR   cbucket         = 0;
                DWORD_PTR   cbucketMost     = 0;
                LONG        lgenMin         = 0;
                LONG        lgenMax         = 0;

                //  validate our DLS
                if ( ( Pdls()->Pinst( ipinst ) != NULL ) &&
                        ( 0 != memcmp( Pdls()->Pinst( ipinst ), pinst, sizeof(INST) ) ) )
                {
                    Pdls()->AddWarning( "Debugger DLS - Pinst() state doesn't match, some debugger extensions not trustable. May happen on live debug though." );
                }

                //  try to retrieve version bucket usage
                //
                if ( NULL != pinst->m_pver && FFetchVariable( pinst->m_pver, &pver ) )
                {
                    CallS( pver->m_cresBucket.ErrGetParam( JET_resoperCurrentUse, &cbucket ) );
                    CallS( pver->m_cresBucket.ErrGetParam( JET_resoperMaxUse, &cbucketMost ) );
                }

                //  try to retrieve bounds of lgens, recmode, etc.
                //
                RECOVERING_MODE recmode = ( NULL != pinst->m_plog &&
                                            FFetchVariable( pinst->m_plog, &plog ) ) ?
                                                plog->FRecoveringMode() :
                                                RECOVERING_MODE( fRestoreRedo + 40 );
                Unfetch( plog );
                plog = NULL;

                (void)ErrEDBGReadInstLgenCheckpoint( pinst, &lgenMin );
                (void)ErrEDBGReadInstLgenTip( pinst, &lgenMax );

                EDBGPrintfDml(  "\n g_rgpinst[0x%x]  (INST <link cmd=\"!ese dump inst 0x%N\">0x%N</link>)\n",
                            ipinst,
                            rgpinst[ipinst],
                            rgpinst[ipinst] );
                dprintf(    "              \t %*.*s : %ws\n",
                            SYMBOL_LEN_MAX,
                            SYMBOL_LEN_MAX,
                            "m_wszInstanceName",
                            ( NULL == pinst->m_wszInstanceName || FFetchSz( pinst->m_wszInstanceName, &wszInstanceName ) ? wszInstanceName : L"???" ) );
                dprintf(    "              \t %*.*s : %ws\n",
                            SYMBOL_LEN_MAX,
                            SYMBOL_LEN_MAX,
                            "m_wszDisplayName",
                            ( NULL == pinst->m_wszDisplayName || FFetchSz( pinst->m_wszDisplayName, &wszDisplayName ) ? wszDisplayName : L"???" ) );
                EDBGPrintfDml(  "              \t %*.*s : <link cmd=\"!ese dump ver 0x%N\">0x%N</link> (version bucket usage: %d/%d)\n",
                            SYMBOL_LEN_MAX,
                            SYMBOL_LEN_MAX,
                            "m_pver",
                            pinst->m_pver,
                            pinst->m_pver,
                            cbucket,
                            cbucketMost );
#define SzRecMode( rm )     (   ( ( rm == fRecoveringNone ) ?       \
                                                                "RecNone" :                     \
                                                            ( rm == fRecoveringRedo ) ? \
                                                                "RecRedo" :                     \
                                                            ( rm == fRecoveringUndo ) ? \
                                                                "RecUndo" :                     \
                                                                "RecOTHER?!?" ) )
                EDBGPrintfDml(  "              \t %*.*s : <link cmd=\"!ese dump log 0x%N\">0x%N</link> (checkpoint: %d gens [min=0x%x, max=0x%x], rec state: %hs )\n",
                            SYMBOL_LEN_MAX,
                            SYMBOL_LEN_MAX,
                            "m_plog",
                            pinst->m_plog,
                            pinst->m_plog,
                            lgenMax - lgenMin,
                            lgenMin,
                            lgenMax,
                            SzRecMode( recmode ) );

                Unfetch( wszInstanceName );
                Unfetch( wszDisplayName );
                Unfetch( pver );
            }

            Unfetch( pinst );
        }
    }

    dprintf( "\n--------------------\n\n" );

    Unfetch( rgpinst );
}

//  SPLIT dumping

const CHAR mpsplittypesz[ splittypeMax - splittypeMin ][ 32 ] =
{
    "splittypeVertical",
    "splittypeRight",
    "splittypeAppend",
};

const CHAR mpsplitopersz[ splitoperMax - splitoperMin ][ 64 ] =
{
    "splitoperNone",
    "splitoperInsert",
    "splitoperReplace",
    "splitoperFlagInsertAndReplaceData",
};

//  ================================================================
VOID CDUMPA<SPLIT>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT argc, const CHAR * const argv[]
    ) const
//  ================================================================
{
    SPLIT *     psplitDebuggee      = NULL;
    SPLIT *     psplit              = NULL;

    if ( argc != 1 || !FAddressFromSz( argv[0], &psplitDebuggee ) )
    {
        dprintf( "Usage: DUMP SPLIT <address>\n" );
        return;
    }

    if ( FFetchVariable( psplitDebuggee, &psplit ) )
    {
        const SIZE_T    dwOffset    = (BYTE *)psplitDebuggee - (BYTE *)psplit;

        dprintf(    "[SPLIT] 0x%p bytes @ 0x%N\n",
                    QWORD( sizeof( SPLIT ) ),
                    psplitDebuggee );

        dprintf( FORMAT_VOID( SPLIT, psplit, csrNew, dwOffset ) );
        dprintf( FORMAT_VOID( SPLIT, psplit, csrRight, dwOffset ) );
        dprintf( FORMAT_UINT( SPLIT, psplit, pgnoSplit, dwOffset ) );
        dprintf( FORMAT_UINT( SPLIT, psplit, pgnoNew, dwOffset ) );
        dprintf( FORMAT_INT( SPLIT, psplit, dbtimeRightBefore, dwOffset ) );
        dprintf( FORMAT_UINT( SPLIT, psplit, fFlagsRightBefore, dwOffset ) );
        dprintf( FORMAT_POINTER( SPLIT, psplit, psplitPath, dwOffset ) );
        dprintf( FORMAT_ENUM_BF( SPLIT, psplit, splittype, dwOffset, mpsplittypesz, splittypeMin, splittypeMax ) );
        dprintf( FORMAT_ENUM_BF( SPLIT, psplit, splitoper, dwOffset, mpsplitopersz, splitoperMin, splitoperMax ) );
        dprintf( FORMAT_BOOL_BF( SPLIT, psplit, fAllocParent, dwOffset ) );
        dprintf( FORMAT_BOOL_BF( SPLIT, psplit, fHotpoint, dwOffset ) );
        dprintf( FORMAT_INT( SPLIT, psplit, ilineOper, dwOffset ) );
        dprintf( FORMAT_INT( SPLIT, psplit, clines, dwOffset ) );
        dprintf( FORMAT_UINT( SPLIT, psplit, fNewPageFlags, dwOffset ) );
        dprintf( FORMAT_UINT( SPLIT, psplit, fSplitPageFlags, dwOffset ) );
        dprintf( FORMAT_INT( SPLIT, psplit, cbUncFreeDest, dwOffset ) );
        dprintf( FORMAT_INT( SPLIT, psplit, cbUncFreeSrc, dwOffset ) );
        dprintf( FORMAT_INT( SPLIT, psplit, ilineSplit, dwOffset ) );
        dprintf( FORMAT_VOID( SPLIT, psplit, prefixinfoSplit, dwOffset ) );
        dprintf( FORMAT_VOID( SPLIT, psplit, prefixSplitOld, dwOffset ) );
        dprintf( FORMAT_VOID( SPLIT, psplit, prefixSplitNew, dwOffset ) );
        dprintf( FORMAT_VOID( SPLIT, psplit, prefixinfoNew, dwOffset ) );
        dprintf( FORMAT_VOID( SPLIT, psplit, kdfParent, dwOffset ) );
        dprintf( FORMAT_POINTER( SPLIT, psplit, rglineinfo, dwOffset ) );

        Unfetch( psplit );
    }
}

// SPLITPATH dumping
//  ================================================================
VOID CDUMPA<SPLITPATH>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT argc, const CHAR * const argv[]
    ) const
//  ================================================================
{
    SPLITPATH *     psplitpathDebuggee  = NULL;
    SPLITPATH *     psplitpath          = NULL;

    if ( argc != 1 || !FAddressFromSz( argv[0], &psplitpathDebuggee ) )
    {
        dprintf( "Usage: DUMP SPLITPATH <address>\n" );
        return;
    }

    if ( FFetchVariable( psplitpathDebuggee, &psplitpath ) )
    {
        const SIZE_T    dwOffset        = (BYTE *)psplitpathDebuggee - (BYTE *)psplitpath;

        dprintf(    "[SPLITPATH] 0x%p bytes @ 0x%N\n",
                    QWORD( sizeof( SPLITPATH ) ),
                    psplitpathDebuggee );

        dprintf( FORMAT_VOID( SPLITPATH, psplitpath, csr, dwOffset ) );
        dprintf( FORMAT_INT( SPLITPATH, psplitpath, dbtimeBefore, dwOffset ) );
        dprintf( FORMAT_UINT( SPLITPATH, psplitpath, fFlagsBefore, dwOffset ) );
        dprintf( FORMAT_POINTER( SPLITPATH, psplitpath, psplitPathParent, dwOffset ) );
        dprintf( FORMAT_POINTER( SPLITPATH, psplitpath, psplitPathChild, dwOffset ) );
        dprintf( FORMAT_POINTER( SPLITPATH, psplitpath, psplit, dwOffset ) );

        Unfetch( psplitpath );
    }
}


//  MERGE dumping

const CHAR mpmergetypesz[ mergetypeMax - mergetypeMin ][ 32 ] =
{
    "mergetypeNone",
    "mergetypeEmptyPage",
    "mergetypeFullRight",
    "mergetypePartialRight",
    "mergetypeEmptyTree",
    "mergetypeFullLeft",
    "mergetypePartialLeft",
    "mergetypePageMove"
};

//  ================================================================
VOID CDUMPA<MERGE>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT argc, const CHAR * const argv[]
    ) const
//  ================================================================
{
    MERGE *     pmergeDebuggee      = NULL;
    MERGE *     pmerge              = NULL;

    if ( argc != 1 || !FAddressFromSz( argv[0], &pmergeDebuggee ) )
    {
        dprintf( "Usage: DUMP MERGE <address>\n" );
        return;
    }

    if ( FFetchVariable( pmergeDebuggee, &pmerge ) )
    {
        const SIZE_T    dwOffset    = (BYTE *)pmergeDebuggee - (BYTE *)pmerge;

        dprintf(    "[MERGE] 0x%p bytes @ 0x%N\n",
                    QWORD( sizeof( MERGE ) ),
                    pmergeDebuggee );

        dprintf( FORMAT_VOID( MERGE, pmerge, csrLeft, dwOffset ) );
        dprintf( FORMAT_VOID( MERGE, pmerge, csrRight, dwOffset ) );
        dprintf( FORMAT_INT( MERGE, pmerge, dbtimeLeftBefore, dwOffset ) );
        dprintf( FORMAT_UINT( MERGE, pmerge, fFlagsLeftBefore, dwOffset ) );
        dprintf( FORMAT_INT( MERGE, pmerge, dbtimeRightBefore, dwOffset ) );
        dprintf( FORMAT_UINT( MERGE, pmerge, fFlagsRightBefore, dwOffset ) );
        dprintf( FORMAT_POINTER( MERGE, pmerge, pmergePath, dwOffset ) );
        dprintf( FORMAT_VOID( MERGE, pmerge, kdfParentSep, dwOffset ) );
        dprintf( FORMAT_ENUM_BF( MERGE, pmerge, mergetype, dwOffset, mpmergetypesz, mergetypeMin, mergetypeMax ) );
        dprintf( FORMAT_BOOL_BF( MERGE, pmerge, fAllocParentSep, dwOffset ) );
        dprintf( FORMAT_INT( MERGE, pmerge, ilineMerge, dwOffset ) );
        dprintf( FORMAT_INT( MERGE, pmerge, cbSavings, dwOffset ) );
        dprintf( FORMAT_INT( MERGE, pmerge, cbSizeTotal, dwOffset ) );
        dprintf( FORMAT_INT( MERGE, pmerge, cbSizeMaxTotal, dwOffset ) );
        dprintf( FORMAT_INT( MERGE, pmerge, cbUncFreeDest, dwOffset ) );
        dprintf( FORMAT_INT( MERGE, pmerge, clines, dwOffset ) );
        dprintf( FORMAT_POINTER( MERGE, pmerge, rglineinfo, dwOffset ) );

        Unfetch( pmerge );
    }
}

// MERGEPATH dumping
//  ================================================================
VOID CDUMPA<MERGEPATH>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT argc, const CHAR * const argv[]
    ) const
//  ================================================================
{
    MERGEPATH *     pmergepathDebuggee  = NULL;
    MERGEPATH *     pmergepath          = NULL;

    if ( argc != 1 || !FAddressFromSz( argv[0], &pmergepathDebuggee ) )
    {
        dprintf( "Usage: DUMP MERGEPATH <address>\n" );
        return;
    }

    if ( FFetchVariable( pmergepathDebuggee, &pmergepath ) )
    {
        const SIZE_T    dwOffset        = (BYTE *)pmergepathDebuggee - (BYTE *)pmergepath;

        dprintf(    "[MERGEPATH] 0x%p bytes @ 0x%N\n",
                    QWORD( sizeof( MERGEPATH ) ),
                    pmergepathDebuggee );

        dprintf( FORMAT_VOID( MERGEPATH, pmergepath, csr, dwOffset ) );
        dprintf( FORMAT_INT( MERGEPATH, pmergepath, dbtimeBefore, dwOffset ) );
        dprintf( FORMAT_UINT( MERGEPATH, pmergepath, fFlagsBefore, dwOffset ) );
        dprintf( FORMAT_POINTER( MERGEPATH, pmergepath, pmergePathParent, dwOffset ) );
        dprintf( FORMAT_POINTER( MERGEPATH, pmergepath, pmergePathChild, dwOffset ) );
        dprintf( FORMAT_POINTER( MERGEPATH, pmergepath, pmerge, dwOffset ) );
        dprintf( FORMAT_INT( MERGEPATH, pmergepath, iLine, dwOffset ) );
        dprintf( FORMAT_BOOL_BF( MERGEPATH, pmergepath, fKeyChange, dwOffset ) );
        dprintf( FORMAT_BOOL_BF( MERGEPATH, pmergepath, fDeleteNode, dwOffset ) );
        dprintf( FORMAT_BOOL_BF( MERGEPATH, pmergepath, fEmptyPage, dwOffset ) );

        Unfetch( pmergepath );
    }
}


// DBFILEHDR dumping / "DBFILEHDR::Dump"
//  ================================================================
VOID CDUMPA<DBFILEHDR>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT argc, const CHAR * const argv[]
    ) const
//  ================================================================
{
    DBFILEHDR *     pdbfilehdrDebuggee  = NULL;
    DBFILEHDR *     pdbfilehdr          = NULL;

    const CHAR * const szMemDump = "mem";

    if ( argc == 0 || !FAutoAddressFromSz( argv[0], &pdbfilehdrDebuggee ) )
    {
        dprintf( "Usage: DUMP DBFILEHDR <address> [%hs]\n", szMemDump );
        return;
    }

    const BOOL fMemDump = argc >= 2 && 0 == _stricmp( argv[1], szMemDump );

    if ( argc > 2 || ( argc == 2 && !fMemDump ) )
    {
        dprintf( "Usage: DUMP DBFILEHDR <address> [%hs]\n", szMemDump );
        return;
    }

    if ( FFetchVariable( pdbfilehdrDebuggee, &pdbfilehdr ) )
    {
        const SIZE_T    dwOffset        = (BYTE *)pdbfilehdrDebuggee - (BYTE *)pdbfilehdr;

        dprintf(    "[DBFILEHDR] 0x%p bytes @ 0x%N\n",
                    QWORD( sizeof( DBFILEHDR ) ),
                    pdbfilehdrDebuggee );
        if ( fMemDump )
        {
            (VOID)( pdbfilehdr->Dump( CPRINTFWDBG::PcprintfInstance(), dwOffset ) );
        }
        else
        {
            (VOID)( pdbfilehdr->DumpLite( CPRINTFWDBG::PcprintfInstance(), "\n", dwOffset ) );
        }
        
        Unfetch( pdbfilehdr );
    }
}

// TrxidStack dumping
//  ================================================================
VOID CDUMPA<TrxidStack>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT argc, const CHAR * const argv[]
    ) const
//  ================================================================
{
    TrxidStack *        ptrxidstackDebuggee = NULL;
    TrxidStack *        ptrxidstack         = NULL;

    if ( argc != 1 || !FAddressFromSz( argv[0], &ptrxidstackDebuggee ) )
    {
        dprintf( "Usage: DUMP TRXIDSTACK <address>\n" );
        return;
    }

    if ( FFetchVariable( ptrxidstackDebuggee, &ptrxidstack ) )
    {
        EDBGPrintfW(    L"[TrxidStack] 0x%p bytes @ 0x%N\n",
                        QWORD( sizeof( TrxidStack ) ),
                        ptrxidstackDebuggee );

        WCHAR wszBuf[1024];
        ERR err;
        if (JET_errSuccess == (err = ptrxidstack->ErrDump(wszBuf, _countof(wszBuf))))
        {
            EDBGPrintfW(L"%s", wszBuf);
        }
        else
        {
            EDBGPrintfW(L"TrxidStack::ErrDump returns %d\n", err);
        }
        Unfetch( ptrxidstack );
    }
}

// Dump members of CPAGE class
VOID CPAGE::Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset ) const
{
    (*pcprintf)( FORMAT_POINTER( CPAGE, this, m_ppib, dwOffset ) );
    (*pcprintf)( FORMAT_INT( CPAGE, this, m_ifmp, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CPAGE, this, m_pgno, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( CPAGE, this, m_bfl, dwOffset ) );
}


//  BF Dumping

const CHAR mpbflssz[ bflsMax - bflsMin ][ 16 ] =
{
    "bflsNormal",
    "bflsNominee1",
    "bflsNominee2",
    "bflsNominee3",
    "bflsElect",
    "bflsHashed",
};

const CHAR mpbfrssz[ bfrsMax - bfrsMin ][ 32 ] =
{
    "bfrsNotCommitted",
    "bfrsNewlyCommitted",
    "bfrsNotResident",
    "bfrsResident",
};

void BF::Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset ) const
{
    const ErrData * perrdata = PerrdataLookupErrValue( err );

#ifdef _WIN64

    (*pcprintf)( FORMAT_VOID( BF, this, ob0ic, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( BF, this, lgposOldestBegin0, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( BF, this, lgposModify, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( BF, this, sxwl, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( BF, this, ifmp, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( BF, this, pgno, dwOffset ) );
    (*pcprintf)( FORMAT_INT_NOLINE( BF, this, err, dwOffset ) );
    (*pcprintf)( " - %hs\n", perrdata->szSymbol );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fLazyIO, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fNewlyEvicted, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fQuiesced, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fAvailable, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fAbandoned, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fWARLatch, dwOffset ) );
    (*pcprintf)( FORMAT_ENUM_BF( BF, this, bfdf, dwOffset, mpbfdfsz, bfdfMin, bfdfMax ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fInOB0OL, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, bfbitfield.FRangeLocked(), dwOffset ) );
    (*pcprintf)( FORMAT_UINT_BF( BF, this, irangelock, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fCurrentVersion, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fOlderVersion, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fFlushed, dwOffset ) );
    (*pcprintf)( FORMAT_ENUM_BF( BF, this, bfls, dwOffset, mpbflssz, bflsMin, bflsMax ) );
    if ( bfls == bflsHashed )
    {
        (*pcprintf)( FORMAT_UINT( BF, this, iHashedLatch, dwOffset ) );
    }
    else
    {
        (*pcprintf)( FORMAT_UINT( BF, this, tickEligibleForNomination, dwOffset ) );
    }
    (*pcprintf)( FORMAT_ENUM_BF( BF, this, bfrs, dwOffset, mpbfrssz, bfrsMin, bfrsMax ) );
    (*pcprintf)( FORMAT_POINTER( BF, this, pv, dwOffset ) );
    (*pcprintf)( FORMAT_( BF, this, lrukic, dwOffset ) );
    const INT dtickLastVsK1 = DtickDelta( lrukic.TickKthTouch( 1 ), lrukic.TickLastTouch() );   // k1 should be <= last
    const INT dtickIndexVsIndexTarget = DtickDelta( lrukic.TickIndexTarget(), lrukic.TickIndex() );     // generally index-target should be >= index, should come out negative
    const BOOL fSC = lrukic.FSuperColded();
    const BOOL fSCConsistent = fSC && lrukic.FSuperColded() && lrukic.FSuperColdedIndex();
    (*pcprintf)( "{ k=%d %hs, [Pri]=%hu, [1]=%u [2]=%u, [Index]=%u, [IndexTarget]=%u, ",
                    lrukic.kLrukPool(),
                    fSC ? ( fSCConsistent ? "SC" : "SC--" ) : "NT",
                    lrukic.PctCachePriority(),
                    lrukic.TickKthTouch( 1 ),
                    lrukic.TickKthTouch( 2 ),
                    lrukic.TickIndex(),
                    lrukic.TickIndexTarget() );
    (*pcprintf)( "l=%u (%ws%d) ",
                    lrukic.TickLastTouch(),
                    dtickLastVsK1 >= 0 ? L"+" : L"", dtickLastVsK1 );
    if ( lrukic.TickIndex() == BFLRUK::tickUnindexed )
    {
        (*pcprintf)( "i=tickUnindexed (0x%x) }\n", BFLRUK::tickUnindexed );
    }
    else
    {
        (*pcprintf)( "[Index]=%u, [IndexTarget]=%u (%ws%d) }\n",
                        lrukic.TickIndex(),
                        lrukic.TickIndexTarget(),
                        dtickIndexVsIndexTarget > 0 ? L"+" : ( dtickIndexVsIndexTarget == 0 ? L"-" : L"" ), dtickIndexVsIndexTarget );
    }

    // union of pWriteSignalComplete / pbfNext
    if ( wrnBFPageFlushPending == err )
    {
        (*pcprintf)( FORMAT_POINTER_NOLINE( BF, this, pWriteSignalComplete, dwOffset ) );
        if ( pWriteSignalComplete != 0 )
        {
            (*pcprintf)( " (IO signaled, err=%d)\n", ErrBFIWriteSignalIError( pWriteSignalComplete ) );
        }
        else
        {
            (*pcprintf)( " (IO unsignaled)\n" );
        }
    }
    else
    {
        (*pcprintf)( FORMAT_POINTER( BF, this, pWriteSignalComplete, dwOffset ) );
    }
    (*pcprintf)( FORMAT_UINT_BF_NOLINE( BF, this, icbPage, dwOffset ) );
    (*pcprintf)( " (cbPage = %d)\n", CbBFISize( (ICBPage)icbPage ) );
    (*pcprintf)( FORMAT_UINT_BF_NOLINE( BF, this, icbBuffer, dwOffset ) );
    (*pcprintf)( " (cbBuffer = %d)\n", CbBFISize( (ICBPage)icbBuffer ) );

    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fSuspiciouslySlowRead, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fSyncRead, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( BF, this, tce, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, bfbitfield.FDependentPurged(), dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, bfbitfield.FImpedingCheckpoint(), dwOffset ) );
    EDBGDumplinkDml( BF, this, BF, pbfTimeDepChainPrev, dwOffset );
    EDBGDumplinkDml( BF, this, BF, pbfTimeDepChainNext, dwOffset );
    EDBGDumplinkDml( BF, this, RCE, prceUndoInfoNext, dwOffset );

    EDBGDumplinkDml( BF, this, IOREQ, pvIOContext, dwOffset );
    (*pcprintf)( FORMAT_RBSPOS( BF, this, rbsposSnapshot, dwOffset ) );
#else  //  !_WIN64

    (*pcprintf)( FORMAT_VOID( BF, this, ob0ic, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( BF, this, lgposOldestBegin0, dwOffset ) );
    (*pcprintf)( FORMAT_LGPOS( BF, this, lgposModify, dwOffset ) );

    EDBGDumplinkDml( BF, this, BF, pbfTimeDepChainPrev, dwOffset );
    EDBGDumplinkDml( BF, this, BF, pbfTimeDepChainNext, dwOffset );

    (*pcprintf)( FORMAT_VOID( BF, this, sxwl, dwOffset ) );

    EDBGDumplinkDml( BF, this, IOREQ, pvIOContext, dwOffset );

    (*pcprintf)( FORMAT_POINTER( BF, this, ifmp, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( BF, this, pgno, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( BF, this, pv, dwOffset ) );
    (*pcprintf)( FORMAT_INT_NOLINE( BF, this, err, dwOffset ) );
    (*pcprintf)( " - %hs\n", perrdata->szSymbol );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fLazyIO, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fNewlyEvicted, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fQuiesced, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fAvailable, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fAbandoned, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fWARLatch, dwOffset ) );
    (*pcprintf)( FORMAT_ENUM_BF( BF, this, bfdf, dwOffset, mpbfdfsz, bfdfMin, bfdfMax ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fInOB0OL, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, bfbitfield.FRangeLocked(), dwOffset ) );
    (*pcprintf)( FORMAT_UINT_BF( BF, this, irangelock, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fCurrentVersion, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fOlderVersion, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fFlushed, dwOffset ) );
    (*pcprintf)( FORMAT_ENUM_BF( BF, this, bfls, dwOffset, mpbflssz, bflsMin, bflsMax ) );
    if ( bfls == bflsHashed )
    {
        (*pcprintf)( FORMAT_UINT( BF, this, iHashedLatch, dwOffset ) );
    }
    else
    {
        (*pcprintf)( FORMAT_UINT( BF, this, tickEligibleForNomination, dwOffset ) );
    }
    (*pcprintf)( FORMAT_ENUM_BF( BF, this, bfrs, dwOffset, mpbfrssz, bfrsMin, bfrsMax ) );
    EDBGDumplinkDml( BF, this, RCE, prceUndoInfoNext, dwOffset );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, bfbitfield.FDependentPurged(), dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, bfbitfield.FImpedingCheckpoint(), dwOffset ) );
    (*pcprintf)( FORMAT_( BF, this, lrukic, dwOffset ) );
    const INT dtickLastVsK1 = DtickDelta( lrukic.TickKthTouch( 1 ), lrukic.TickLastTouch() );   // k1 should be <= last
    const INT dtickIndexVsIndexTarget = DtickDelta( lrukic.TickIndexTarget(), lrukic.TickIndex() );     // generally index-target should be >= index, should come out negative
    const BOOL fSC = lrukic.FSuperColded();
    const BOOL fSCConsistent = fSC && lrukic.FSuperColded() && lrukic.FSuperColdedIndex();
    (*pcprintf)( "{ k=%d %hs, [Pri]=%hu, [1]=%u [2]=%u, l=%u (%ws%d) [Index]=%u [IndexTarget]=%u  (%ws%d)}\n",
                    lrukic.kLrukPool(),
                    fSC ? ( fSCConsistent ? "SC" : "SC--" ) : "NT",
                    lrukic.PctCachePriority(),
                    lrukic.TickKthTouch( 1 ),
                    lrukic.TickKthTouch( 2 ),
                    lrukic.TickLastTouch(),
                    dtickLastVsK1 >= 0 ? L"+" : L"", dtickLastVsK1,
                    lrukic.TickIndex(),
                    lrukic.TickIndexTarget(),
                    dtickIndexVsIndexTarget > 0 ? L"+" : ( dtickIndexVsIndexTarget == 0 ? L"-" : L"" ), dtickIndexVsIndexTarget );
    // union of pWriteSignalComplete / pbfNext
    if ( wrnBFPageFlushPending == err )
    {
        (*pcprintf)( FORMAT_POINTER_NOLINE( BF, this, pWriteSignalComplete, dwOffset ) );
        if ( pWriteSignalComplete != 0 )
        {
            (*pcprintf)( " (IO signaled, err=%d)\n", ErrBFIWriteSignalIError( pWriteSignalComplete ) );
        }
        else
        {
            (*pcprintf)( " (IO unsignaled)\n" );
        }
    }
    else
    {
        (*pcprintf)( FORMAT_POINTER( BF, this, pWriteSignalComplete, dwOffset ) );
    }

    (*pcprintf)( FORMAT_UINT_BF_NOLINE( BF, this, icbPage, dwOffset ) );
    (*pcprintf)( " (cbPage = %d)\n", CbBFISize( (ICBPage)icbPage ) );
    (*pcprintf)( FORMAT_UINT_BF_NOLINE( BF, this, icbBuffer, dwOffset ) );
    (*pcprintf)( " (cbBuffer = %d)\n", CbBFISize( (ICBPage)icbBuffer ) );

    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fSuspiciouslySlowRead, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL_BF( BF, this, fSyncRead, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( BF, this, tce, dwOffset ) );
    (*pcprintf)( FORMAT_RBSPOS( BF, this, rbsposSnapshot, dwOffset ) );
#endif  //  _WIN64

}


template< class CKey, class CEntry >
VOID CDynamicHashTable< CKey, CEntry >::Dump(
    CPRINTF *           pcprintf,
    const DWORD_PTR     dwOffset ) const
{
    (*pcprintf)( FORMAT_UINT( CDynamicHashTable, this, m_cLoadFactor, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CDynamicHashTable, this, m_centryBucket, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CDynamicHashTable, this, m_cbBucket, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CDynamicHashTable, this, m_rankDHTrwlBucket, dwOffset ) );
    (*pcprintf)( FORMAT_POINTER( CDynamicHashTable, this, m_rghs, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CDynamicHashTable, this, m_chs, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CDynamicHashTable, this, m_cbucketMin, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( CDynamicHashTable, this, m_rgbRsvdNever, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( CDynamicHashTable, this, m_dirptrs, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( CDynamicHashTable, this, m_rgrgBucket, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( CDynamicHashTable, this, m_cOpSensitivity, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CDynamicHashTable, this, m_cBucketPreferred, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CDynamicHashTable, this, m_stateCur, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( CDynamicHashTable, this, m_rgbRsvdOften, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( CDynamicHashTable, this, m_semPolicy, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CDynamicHashTable, this, m_cCompletions, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( CDynamicHashTable, this, m_rgbRsvdAlways, dwOffset ) );
}

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline VOID CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::Dump(
    CPRINTF *           pcprintf,
    const DWORD_PTR     dwOffset ) const
{
    (*pcprintf)( FORMAT_POINTER( CInvasiveConcurrentModSet, this, m_pObject, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CInvasiveConcurrentModSet, this, m_ulArrayAllocated, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CInvasiveConcurrentModSet, this, m_ulArrayUsed, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CInvasiveConcurrentModSet, this, m_flhFreeListHead.ullFreeListHead, dwOffset ) );
}

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
VOID CApproximateIndex< CKey, CEntry, OffsetOfIC >::Dump(
    CPRINTF *           pcprintf,
    const DWORD_PTR     dwOffset,
    const DWORD         grbit ) const
{
    const BOOL fPrintableKeySize = ( sizeof(CKey) == 4 || sizeof(CKey) == 8 );
    
    (*pcprintf)( FORMAT_UINT( CApproximateIndex, this, m_shfKeyPrecision, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CApproximateIndex, this, m_shfKeyUncertainty, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CApproximateIndex, this, m_shfBucketHash, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CApproximateIndex, this, m_shfFillMSB, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CApproximateIndex, this, m_maskBucketKey, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CApproximateIndex, this, m_maskBucketPtr, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CApproximateIndex, this, m_maskBucketID, dwOffset ) );
    (*pcprintf)( FORMAT_( CApproximateIndex, this, m_didRangeMost, dwOffset ) );
    (*pcprintf)( "%d (0x%x)", m_didRangeMost, m_didRangeMost );
    if ( fPrintableKeySize && grbit & bitEdbgDumpKeys )
    {
        //  we cast it upto a 64-bit number to get consistent printing ...
        const __int64 keyInsertLeast = (__int64)KeyInsertLeast();
        const __int64 keyInsertMost = (__int64)KeyInsertMost();
        (*pcprintf)( "  [KeyRange: %I64d (0x%I64x) - %I64d (0x%I64x)]", keyInsertLeast, keyInsertLeast, keyInsertMost, keyInsertMost );
    }
    (*pcprintf)( "\n" );

    (*pcprintf)( FORMAT_VOID( CApproximateIndex, this, m_critUpdateIdRange, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CApproximateIndex, this, m_cidRange, dwOffset ) );

    (*pcprintf)( FORMAT_( CApproximateIndex, this, m_idRangeFirst, dwOffset ) );
    (*pcprintf)( "%d (0x%x)", m_idRangeFirst, m_idRangeFirst );
    if ( fPrintableKeySize && grbit & bitEdbgDumpKeys )
    {
        const __int64 keyRangeFirst = (__int64)_KeyFromId( m_idRangeFirst );
        (*pcprintf)( "  [Key: %I64d (0x%I64x)]", keyRangeFirst, keyRangeFirst );
    }
    (*pcprintf)( "\n" );
    
    (*pcprintf)( FORMAT_( CApproximateIndex, this, m_idRangeLast, dwOffset ) );
    (*pcprintf)( "%d (0x%x)", m_idRangeLast, m_idRangeLast );
    if ( fPrintableKeySize && grbit & bitEdbgDumpKeys )
    {
        const __int64 keyRangeLast = (__int64)_KeyFromId( m_idRangeLast );
        (*pcprintf)( "  [Key: %I64d (0x%I64x)]", keyRangeLast, keyRangeLast );
    }
    (*pcprintf)( "\n" );
    (*pcprintf)( FORMAT_VOID( CApproximateIndex, this, m_rgbReserved2, dwOffset ) );


    (*pcprintf)( FORMAT_VOID( CApproximateIndex, this, m_bt, dwOffset ) );
}

VOID CDUMPA<CDynamicHashTableEDBG>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT                     argc,
    const CHAR * const      argv[] ) const
//  ================================================================
{
    CDynamicHashTableEDBG * pdhtDebuggee    = NULL;
    CDynamicHashTableEDBG * pdht            = NULL;

    if ( argc != 1 || !FAddressFromSz( argv[0], &pdhtDebuggee ) )
    {
        dprintf( "Usage: DUMP CDynamicHashTable <address>\n" );
        return;
    }

    if ( FFetchVariable( pdhtDebuggee, &pdht ) )
    {
        const SIZE_T    dwOffset        = (BYTE *)pdhtDebuggee - (BYTE *)pdht;

        dprintf(    "[CDynamicHashTable] 0x%p bytes @ 0x%N\n",
                    QWORD( sizeof( CDynamicHashTableEDBG ) ),
                    pdhtDebuggee );

        pdht->Dump( CPRINTFWDBG::PcprintfInstance(), dwOffset );

        Unfetch( pdht );
    }
}

VOID CDUMPA<CApproximateIndexEDBG>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT                     argc,
    const CHAR * const      argv[] ) const
//  ================================================================
{
    CApproximateIndexEDBG * paiDebuggee = NULL;
    CApproximateIndexEDBG * pai         = NULL;

    if ( argc != 1 || !FAddressFromSz( argv[0], &paiDebuggee ) )
    {
        dprintf( "Usage: DUMP CApproximateIndex <address>\n" );
        return;
    }

    if ( FFetchVariable( paiDebuggee, &pai ) )
    {
        const SIZE_T    dwOffset        = (BYTE *)paiDebuggee - (BYTE *)pai;

        dprintf(    "[CApproximateIndex] 0x%p bytes @ 0x%N\n",
                    QWORD( sizeof( CApproximateIndexEDBG ) ),
                    paiDebuggee );

        pai->Dump( CPRINTFWDBG::PcprintfInstance(), dwOffset, 0x0 );

        Unfetch( pai );
    }
}

// RESMGR::CLRUKResourceUtilityManager< 2, BF , &BF::OffsetOfLRUKIC, IFMPPGNO >
template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
VOID CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::Dump(
    CPRINTF *           pcprintf,
    const DWORD_PTR     dwOffset ) const
{
    (*pcprintf)( "\n  State:\n" );
    (*pcprintf)( "      m_Kmax = %d\n", m_Kmax );
    (*pcprintf)( FORMAT_BOOL( CLRUKResourceUtilityManager, this, m_fInitialized, dwOffset ) );

    (*pcprintf)( "\n  Config:\n" );
    (*pcprintf)( FORMAT_UINT( CLRUKResourceUtilityManager, this, m_K, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CLRUKResourceUtilityManager, this, m_ctickCorrelatedTouch, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CLRUKResourceUtilityManager, this, m_ctickTimeout, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CLRUKResourceUtilityManager, this, m_dtickUncertainty, dwOffset ) );

    (*pcprintf)( "\n  Runtime Statistics:\n" );
    (*pcprintf)( FORMAT_UINT( CLRUKResourceUtilityManager, this, m_cHistoryRecord, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CLRUKResourceUtilityManager, this, m_cHistoryHit, dwOffset ) );
    (*pcprintf)( FORMAT_( CLRUKResourceUtilityManager, this, m_cHistoryReq, dwOffset ) );
    (*pcprintf)( "%I64u (0x%I64X, %02.1f%% hit)\n", m_cHistoryReq, m_cHistoryReq, (double)m_cHistoryHit / (double)m_cHistoryReq * (double)100 );
    (*pcprintf)( FORMAT_UINT( CLRUKResourceUtilityManager, this, m_cResourceScanned, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CLRUKResourceUtilityManager, this, m_cResourceScannedOutOfOrder, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( CLRUKResourceUtilityManager, this, m_cSuperCold, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( CLRUKResourceUtilityManager, this, m_tickStart, dwOffset ) );
    
    (*pcprintf)( FORMAT_UINT_NOLINE( CLRUKResourceUtilityManager, this, m_tickScanFirstFoundAll, dwOffset ) );
    (*pcprintf)( "   %hs\n", !_FTickSuperColded( m_tickScanFirstFoundAll ) ? "NormalTouch" : "SuperColded (ERR: should be normal touch!)" );
    (*pcprintf)( FORMAT_UINT_NOLINE( CLRUKResourceUtilityManager, this, m_tickScanFirstFoundNormal, dwOffset ) );
    INT dtickDelta = DtickDelta( m_tickScanFirstFoundAll, m_tickScanFirstFoundNormal );
    (*pcprintf)( "   %hs  %hs%d\n",
                        !_FTickSuperColded( m_tickScanFirstFoundNormal ) ? "NormalTouch" : "SuperColded (ERR: should be normal touch!)",
                        dtickDelta >= 0 ? "+" : "", dtickDelta );
    (*pcprintf)( FORMAT_UINT_NOLINE( CLRUKResourceUtilityManager, this, m_tickLatestScanFirstFoundNormalUpdate, dwOffset ) );
    (*pcprintf)( "   (%d ms old)\n", DtickDelta( m_tickLatestScanFirstFoundNormalUpdate, Pdls()->TickDLSCurrent() ) );
    dtickDelta = DtickDelta( m_tickScanFirstFoundAll, m_tickScanLastFound );
    (*pcprintf)( FORMAT_UINT_NOLINE( CLRUKResourceUtilityManager, this, m_tickScanLastFound, dwOffset ) );
    (*pcprintf)( "   %hs  %hs%d\n",
                        !_FTickSuperColded( m_tickScanLastFound ) ? "NormalTouch" : "SuperColded (ERR: should be normal touch!)",
                        dtickDelta >= 0 ? "+" : "", dtickDelta );

    dtickDelta = DtickDelta( m_tickScanLastFound, m_tickScanFirstEvictedIndexTarget );
    (*pcprintf)( FORMAT_UINT_NOLINE( CLRUKResourceUtilityManager, this, m_tickScanFirstEvictedIndexTarget, dwOffset ) );
    (*pcprintf)( "   %hs  %hs%d\n",
                        !_FTickSuperColded( m_tickScanFirstEvictedIndexTarget ) ? "NormalTouch" : "SuperColded (ERR: should be normal touch!)",
                        dtickDelta >= 0 ? "+" : "", dtickDelta );
    (*pcprintf)( FORMAT_UINT_NOLINE( CLRUKResourceUtilityManager, this, m_tickLatestScanFirstEvictedIndexTargetUpdate, dwOffset ) );
    (*pcprintf)( "   (%d ms old)\n", DtickDelta( m_tickLatestScanFirstEvictedIndexTargetUpdate, Pdls()->TickDLSCurrent() ) );

    dtickDelta = DtickDelta( m_tickScanFirstEvictedIndexTarget, m_tickScanFirstEvictedIndexTargetHW );
    (*pcprintf)( FORMAT_UINT_NOLINE( CLRUKResourceUtilityManager, this, m_tickScanFirstEvictedIndexTargetHW, dwOffset ) );
    (*pcprintf)( "   %hs  %hs%d\n",
                        !_FTickSuperColded( m_tickScanFirstEvictedIndexTargetHW ) ? "NormalTouch" : "SuperColded (ERR: should be normal touch!)",
                        dtickDelta >= 0 ? "+" : "", dtickDelta );
    (*pcprintf)( FORMAT_UINT_NOLINE( CLRUKResourceUtilityManager, this, m_tickLatestScanFirstEvictedIndexTargetHWUpdate, dwOffset ) );
    (*pcprintf)( "   (%d ms old)\n", DtickDelta( m_tickLatestScanFirstEvictedIndexTargetHWUpdate, Pdls()->TickDLSCurrent() ) );

    (*pcprintf)( FORMAT_UINT_NOLINE( CLRUKResourceUtilityManager, this, m_tickScanFirstEvictedTouchK1, dwOffset ) );
    (*pcprintf)( FORMAT_UINT_NOLINE( CLRUKResourceUtilityManager, this, m_tickScanFirstEvictedTouchK2, dwOffset ) );


    (*pcprintf)( "\n  Data Structure Support:\n" );
    (*pcprintf)( FORMAT_VOID( CLRUKResourceUtilityManager, this, m_HistoryLRUK, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( CLRUKResourceUtilityManager, this, m_KeyHistory, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( CLRUKResourceUtilityManager, this, m_ResourceLRUK, dwOffset ) );

    (*pcprintf)( "\n  m_HistoryLRUK Details:\n" );
    m_HistoryLRUK.Dump( pcprintf, dwOffset, bitEdbgDumpKeys );
    (*pcprintf)( "\n  m_KeyHistory Details:\n" );
    m_KeyHistory.Dump( pcprintf, dwOffset );
    (*pcprintf)( "\n  m_ResourceLRUK Details:\n" );
    (*pcprintf)( "\n  WARNING:  The keys listed here are NOT necessarily in the right number space ... \n" );
    m_ResourceLRUK.Dump( pcprintf, dwOffset, bitEdbgDumpKeys );

}

VOID CDUMPA<CLRUKResourceUtilityManagerEDBG>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT                     argc,
    const CHAR * const      argv[] ) const
//  ================================================================
{
    CLRUKResourceUtilityManagerEDBG *   paiDebuggee = NULL;
    CLRUKResourceUtilityManagerEDBG *   pai         = NULL;

    if ( argc != 1 || !FAddressFromSz( argv[0], &paiDebuggee ) )
    {
        dprintf( "Usage: DUMP CLRUKResourceUtilityManager <address>\n" );
        return;
    }

    if ( FFetchVariable( paiDebuggee, &pai ) )
    {
        const SIZE_T    dwOffset        = (BYTE *)paiDebuggee - (BYTE *)pai;

        dprintf(    "[CLRUKResourceUtilityManager] 0x%p bytes @ 0x%N\n",
                    QWORD( sizeof( CLRUKResourceUtilityManagerEDBG ) ),
                    paiDebuggee );

        pai->Dump( CPRINTFWDBG::PcprintfInstance(), dwOffset );

        Unfetch( pai );
    }
}

struct IFMPGEN
{
    IFMP    ifmp;
    ULONG   lgen;
};

DEBUG_EXT( EDBGBFOB0FindOldest )
{
    IFMPGEN         ifmpgen             = { 0, 0 };
    FMP *           rgfmpDebuggee       = NULL;
    FMP *           pfmp                = NULL;
    ULONG           ifmpMaxDebuggee;
    BFFMPContext *  pbffmp;
    BOOL            fValidUsage;

    switch ( argc )
    {
        case 1:
            fValidUsage = ( FAutoIfmpFromSz( argv[0], (ULONG *)&ifmpgen.ifmp ) );
            break;

        case 2:
            fValidUsage = ( FAutoIfmpFromSz( argv[0], (ULONG *)&ifmpgen.ifmp )
                            && FUlFromSz( argv[1], &ifmpgen.lgen )
                            && ifmpgen.lgen > 0 );
            break;

        default:
            fValidUsage = fFalse;
            break;
    }

    dprintf( "\n" );

    if ( !fValidUsage )
    {
        dprintf( "Usage: BFOB0FINDOLDEST <ifmp> [<gen>]\n" );
        dprintf( "\n" );
        dprintf( "    <ifmp> - index to the FMP entry for the desired database file\n" );
        dprintf( "    <gen>  - find all BF's with lgposBegin0 less than or equal to\n" );
        dprintf( "             specified log generation\n" );
        return;
    }

    if ( !FReadGlobal( "g_rgfmp", &rgfmpDebuggee )
        || NULL == rgfmpDebuggee
        || !FReadGlobal( "g_ifmpMax", &ifmpMaxDebuggee ) )
    {
        dprintf( "Error: Could not fetch FMP variables.\n" );
        return;
    }

    if ( ifmpgen.ifmp >= ifmpMaxDebuggee )
    {
        dprintf( "Error: Illegal ifmp (g_ifmpMax==0x%x)\n", ifmpMaxDebuggee );
        return;
    }

    if ( !FFetchVariable( rgfmpDebuggee + ifmpgen.ifmp, &pfmp ) )
    {
        dprintf( "Error: Couldn't fetch pfmp %N\n", rgfmpDebuggee + ifmpgen.ifmp );
        return;
    }

    if ( FFetchVariable( (BFFMPContext *)pfmp->DwBFContext(), &pbffmp ) )
    {
        dprintf( "BFOB0: 0x%N\n", pfmp->DwBFContext() );
        pbffmp->bfob0.Scan( CPRINTFWDBG::PcprintfInstance(), (VOID *)&ifmpgen );

        Unfetch( pbffmp );
    }

    Unfetch( pfmp );
}

VOID BFOB0::CBucketTable::Scan( CPRINTF * pcprintf, VOID * pv ) const
{
    const IFMPGEN * pifmpgen                = (IFMPGEN *)pv;
    const SIZE_T    crgBucket               = sizeof(m_rgrgBucket) / sizeof(m_rgrgBucket[0]);
    BYTE *          rgbBucket               = NULL;
    BYTE *          pbBucket                = NULL;
    PBF             pbf                     = pbfNil;
    PBF             pbfOldestBegin0         = pbfNil;
    LGPOS           lgposOldestBegin0       = lgposMax;
    SIZE_T          ientryOldestBegin0      = 0;
    SIZE_T          iBucketOldestBegin0     = 0;
    SIZE_T          irgBucketOldestBegin0   = 0;
    BOOL            fFoundTargetGen         = fFalse;

    pbBucket = (BYTE *)LocalAlloc( 0, m_cbBucket );
    if ( NULL == pbBucket )
    {
        (*pcprintf)( "Error: Could not allocate BUCKET buffer.\n" );
        goto HandleError;
    }

    pbf = (PBF)LocalAlloc( 0, sizeof(BF) );
    if ( NULL == pbf )
    {
        (*pcprintf)( "Error: Could not allocate BF buffer.\n" );
        goto HandleError;
    }

    if ( pifmpgen->lgen > 0 )
    {
        (*pcprintf)(
            "Scanning BFOB0 [bucket array/bucket/entry] for ifmp 0x%x with lgposBegin0 at generation %d (0x%x) or older:\n",
            pifmpgen->ifmp,
            pifmpgen->lgen,
            pifmpgen->lgen );
    }
    else
    {
        (*pcprintf)( "Scanning all BFOB0 entries [bucket array/bucket/entry]...\n" );
    }

    //  scan each DHT directory entry
    //
    for ( SIZE_T irgBucket = 0; irgBucket < crgBucket && NULL != m_rgrgBucket[irgBucket]; irgBucket++ )
    {
        //  number of buckets grows exponentially, with a minimum of 2
        //
        const PBUCKET   pbucket             = PBUCKET( pbBucket );
        const SIZE_T    cBuckets            = max( 2, 1 << irgBucket );
        BYTE * const    rgbBucketDebuggee   = m_rgrgBucket[irgBucket];

        if ( !FFetchVariable( rgbBucketDebuggee, &rgbBucket, m_cbBucket * cBuckets ) )
        {
            (*pcprintf)( "Error: Could not fetch bucket array at 0x%N.\n", rgbBucketDebuggee );
            goto HandleError;
        }

        //  scan all DHT buckets in this directory entry
        //
        for ( SIZE_T iBucket = 0; iBucket < cBuckets; iBucket++ )
        {
            //  scan this bucket and its overflow buckets
            //
            BOOL        fLastBucketInChain  = fFalse;
            BYTE *      pbBucketDebuggee    = rgbBucketDebuggee + ( iBucket * m_cbBucket );
            while ( !fLastBucketInChain )
            {
                const CKeyEntry *   pEntryMost;

                //  UNDONE: don't actually need to read the first bucket (it's
                //  already read in in the bucket array as the head of this
                //  chain), but it makes the loop easier to code if we do
                //
                if ( !FReadVariable( pbBucketDebuggee, pbBucket, m_cbBucket ) )
                {
                    (*pcprintf)( "Error: Could not read bucket at 0x%N.\n", pbBucketDebuggee );
                    goto HandleError;
                }

                if ( NULL == pbucket->m_pb )
                {
                    //  bucket was empty, so end of chain was reached
                    //
                    fLastBucketInChain = fTrue;
                    pEntryMost = pbucket->m_rgEntry;
                }
                else if ( pbucket->m_pb > pbBucketDebuggee
                    && pbucket->m_pb < pbBucketDebuggee + m_cbBucket )
                {
                    //  m_pEntryLast is valid, so end of chain was reached
                    //
                    fLastBucketInChain = fTrue;
                    pEntryMost = (CKeyEntry *)( pbBucket + ( pbucket->m_pb - pbBucketDebuggee ) );

                    //  m_pEntryLast actually points to the last valid entry,
                    //  so move one past
                    //
                    pEntryMost++;
                }
                else
                {
                    pEntryMost = pbucket->m_rgEntry + m_centryBucket;
                }

                //  scan all entries in this bucket
                //

                for ( CKeyEntry * pEntryThis = pbucket->m_rgEntry;
                    pEntryThis < pEntryMost;
                    pEntryThis++ )
                {
                    //  scan invasive list for this entry
                    //
                    BFOB0::CBucket      listHead;
                    pEntryThis->GetEntry( &listHead );

                    for ( PBF pbfDebuggee = listHead.m_il.PrevMost();
                        pbfNil != pbfDebuggee;
                        pbfDebuggee = listHead.m_il.Next( pbf ) )
                    {
                        if ( !FReadVariable( pbfDebuggee, pbf ) )
                        {
                            (*pcprintf)( "Error: Could not read BF at 0x%N.\n", pbfDebuggee );
                            goto HandleError;
                        }

                        if ( pbf->ifmp == pifmpgen->ifmp
                            && ( pbf->fCurrentVersion || pbf->fOlderVersion ) )
                        {
                            if ( pifmpgen->lgen > 0 )
                            {
                                if ( (ULONG)pbf->lgposOldestBegin0.lGeneration <= pifmpgen->lgen )
                                {
                                    (*pcprintf)(
                                        "    [%d/%d/%d] BF 0x%N: gen %d (0x%x)\n",
                                        irgBucket,
                                        iBucket,
                                        ( pEntryThis - pbucket->m_rgEntry ),
                                        pbfDebuggee,
                                        pbf->lgposOldestBegin0.lGeneration,
                                        pbf->lgposOldestBegin0.lGeneration );
                                    fFoundTargetGen = fTrue;
                                }
                            }

                            if ( CmpLgpos( &pbf->lgposOldestBegin0, &lgposOldestBegin0 ) < 0 )
                            {
                                lgposOldestBegin0 = pbf->lgposOldestBegin0;
                                pbfOldestBegin0 = pbfDebuggee;
                                irgBucketOldestBegin0 = irgBucket;
                                iBucketOldestBegin0 = iBucket;
                                ientryOldestBegin0 = ( pEntryThis - pbucket->m_rgEntry );
                            }
                        }
                    }
                }

                pbBucketDebuggee = (BYTE *)pbucket->m_pBucketNext;
            }
        }

        Unfetch( rgbBucket );
        rgbBucket = NULL;
    }

    if ( pifmpgen->lgen > 0 && !fFoundTargetGen )
    {
        //  didn't find any BF for this ifmp
        //
        (*pcprintf)( "    <none>\n" );
    }

    if ( pbfNil != pbfOldestBegin0 )
    {
        (*pcprintf)(
            "Oldest lgposBegin0 (generation 0x%x) for ifmp 0x%x is cached in BF 0x%N, ",
            lgposOldestBegin0.lGeneration,
            pifmpgen->ifmp,
            pbfOldestBegin0 );
        (*pcprintf)(
            "which is in the BFOB0 at [%d/%d/%d].\n",
            irgBucketOldestBegin0,
            iBucketOldestBegin0,
            ientryOldestBegin0 );
    }
    else
    {
        (*pcprintf)( "No pages found for ifmp 0x%x.\n", pifmpgen->ifmp );
    }

HandleError:
    (*pcprintf)( "\n" );

    LocalFree( pbf );
    LocalFree( pbBucket );
    Unfetch( rgbBucket );
}


void COSFileSystem::Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset ) const
{
    (*pcprintf)( FORMAT_VOID( COSFileSystem, this, m_pfsconfig, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( COSFileSystem, this, m_critVolumePathCache, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( COSFileSystem, this, m_ilVolumePathCache, dwOffset ) );
}

void COSFileFind::Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset ) const
{
    EDBGDumplinkDml( COSFileFind, this, COSFileSystem, m_posfs, dwOffset );
    (*pcprintf)( FORMAT_WSZ( COSFileFind, this, m_wszFindPath, dwOffset ) );
    (*pcprintf)( FORMAT_WSZ( COSFileFind, this, m_wszAbsFindPath, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( COSFileFind, this, m_fBeforeFirst, dwOffset ) );
    (*pcprintf)( FORMAT_INT( COSFileFind, this, m_errFirst, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( COSFileFind, this, m_hFileFind, dwOffset ) );
    (*pcprintf)( FORMAT_INT( COSFileFind, this, m_errCurrent, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( COSFileFind, this, m_fFolder, dwOffset ) );
    (*pcprintf)( FORMAT_WSZ( COSFileFind, this, m_wszAbsFoundPath, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( COSFileFind, this, m_cbSize, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( COSFileFind, this, m_fReadOnly, dwOffset ) );
}

typedef struct VALUESZMAP_
{
    __int64             value;
    CHAR *              sz;
} VALUESZMAP;

#define MapValue( enumSymbol )      { enumSymbol, #enumSymbol }


const VALUESZMAP mpfmfsz[] =
{
    MapValue( IFileAPI::fmfCached ),
    MapValue( IFileAPI::fmfStorageWriteBack ),
    MapValue( IFileAPI::fmfLossyWriteBack ),
    MapValue( IFileAPI::fmfSparse ),
    MapValue( IFileAPI::fmfTemporary ),
    MapValue( IFileAPI::fmfOverwriteExisting ),
    MapValue( IFileAPI::fmfReadOnly ),
};

#define QwAllFlags( mp )        QwAllFlags_( mp, _countof(mp) )
__int64 QwAllFlags_( const VALUESZMAP * mp, DWORD cmp )
{
    __int64 qwRet = 0;
    for( ULONG imp = 0; imp < _countof( mpfmfsz ); imp++ )
    {
        qwRet |= mp[imp].value;
    }
    return qwRet;
}

#define PrintFormatFlagsOneLine( pcp, qwF, mp )     PrintFormatFlagsOneLine_( pcp, qwF, mp, _countof(mp) )
void PrintFormatFlagsOneLine_( CPRINTF * const pcprintf, __int64 qwFlags, const VALUESZMAP * const mp, DWORD cmp )
{
    const __int64 qwLeftOvers = qwFlags & ~QwAllFlags_( mp, cmp );
    __int64 qwPrinted = 0;
    if ( qwLeftOvers )
    {
        (*pcprintf)( " = ( %#I64x ", qwLeftOvers );
        qwPrinted = qwLeftOvers;
    }
    else
    {
        (*pcprintf)( " = ( " );
    }
    for( ULONG i = 0; i < cmp; i++ )
    {
        if ( qwFlags & mp[i].value )
        {
            (*pcprintf)( "%s%s ", ( ( qwPrinted != 0 ) ? "| " : "" ), mp[i].sz  );
            qwPrinted |= mp[i].value;
        }
    }
    AssertEDBG( qwPrinted == qwFlags );
    (*pcprintf)( ")\n" );
}

const CHAR mpiomethodsz[ 1 + IOREQ::iomethodScatterGather - IOREQ::iomethodNone ][ 35 ] =
{
    "IOREQ::iomethodNone",
    "IOREQ::iomethodSync",
    "IOREQ::iomethodSemiSync",
    "IOREQ::iomethodAsync",
    "IOREQ::iomethodScatterGather",
};

COSDisk * PosdEDBGGetDisk( _In_ const COSVolume * const posv )
{
    return posv->m_posd;
}

void COSFile::Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset ) const
{
    EDBGDumplinkDml( COSFile, this, COSFileSystem, m_posfs, dwOffset );
    (*pcprintf)( FORMAT_WSZ( COSFile, this, m_wszAbsPath, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( COSFile, this, m_hFile, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( COSFile, this, m_cbFileSize, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( COSFile, this, m_cbIOSize, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( COSFile, this, m_cbSectorSize, dwOffset ) );
    (*pcprintf)( FORMAT_UINT_NOLINE( COSFile, this, m_fmf, dwOffset ) );
    PrintFormatFlagsOneLine( pcprintf, m_fmf, mpfmfsz );

    dprintf( FORMAT_POINTER_NOLINE( COSFile, this, m_p_osf, dwOffset ) );
    _OSFILE _osf;
    if ( FReadVariable( m_p_osf, &_osf ) )
        {
            EDBGPrintfDml( " = { fRegistered = %u, iomethodMost = %hs }\n",
                _osf.fRegistered, mpiomethodsz[ _osf.iomethodMost ] );
            memset( &_osf, 0, sizeof(_osf) ); // defeating .dtor freeing stuff
        }
        else
        {
            dprintf( " = { #failed load# }\n" );
        }
    (*pcprintf)( FORMAT_POINTER( COSFile, this, m_posv, dwOffset ) );
    dprintf( FORMAT_POINTER_NOLINE( COSFile, this, m_posv, dwOffset ) );
    COSVolume osv;
    if ( FReadVariable( m_posv, &osv ) )
    {
        COSDisk * posdOsv = PosdEDBGGetDisk( &osv );
        EDBGPrintfDml( " = { m_cref = %u, <link cmd=\"!ese dump COSDisk 0x%I64x\">m_posd</link> = <link cmd=\"dt %ws!COSDisk 0x%I64x\">0x%I64x</link>, paths = %ws -> %ws }\n",
            osv.CRef(), __int64( posdOsv ), WszUtilImageName(), __int64( posdOsv ), __int64( posdOsv ), osv.WszVolPath(), osv.WszVolCanonicalPath() );
    }
    else
    {
        dprintf( " = { #failed load# }\n" );
    }

    (*pcprintf)( FORMAT_UINT( COSFile, this, m_cioUnflushed, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( COSFile, this, m_cioFlushing, dwOffset ) );
    (*pcprintf)( FORMAT_INT( COSFile, this, m_errFlushFailed, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( COSFile, this, m_msFileSize, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( COSFile, this, m_rgcbFileSize, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( COSFile, this, m_semChangeFileSize, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( COSFile, this, m_critDefer, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( COSFile, this, m_ilDefer, dwOffset ) );
}

const VALUESZMAP mpeszDiskState[] =
{
    MapValue( COSDisk::eOSDiskInitCtor ),
    MapValue( COSDisk::eOSDiskConnected ),
};

// temporary until flighting removed
void FlightConcurrentMetedOps( INT cioOpsMax, INT cioLowThreshold, TICK dtickStarvation );

void COSDisk::Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset ) const
{
    //  Basic Disk Id

    EDBGPrintfDml( "Dumping COSDisk <link cmd=\"dt %ws!COSDisk %I64x\">0x%0*I64X</link> ...\n", WszUtilImageName(), __int64( (char*)(this) + dwOffset ), INT( 2 * sizeof( this ) ), __int64( (char*)(this) + dwOffset ) );

    (*pcprintf)( "\n    Basic Disk Id:\n" );
    // skip: CInvasiveList< COSDisk, OffsetOfILE >::CElement m_ile;
    (*pcprintf)( FORMAT_UINT_NOLINE( COSDisk, this, m_eState, dwOffset ) );
    PrintFormatFlagsOneLine( pcprintf, m_eState, mpeszDiskState );
    (*pcprintf)( FORMAT_UINT( COSDisk, this, m_dwDiskNumber, dwOffset ) );
    // skip: COSEventTraceIdCheck   m_traceidcheckDisk;
    (*pcprintf)( FORMAT_WSZ( COSDisk, this, m_wszDiskPathId, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( COSDisk, this, m_hDisk, dwOffset ) );

    //  Disk Hw Info

    (*pcprintf)( "\n    Disk Hardware Info:\n" );
    (*pcprintf)( FORMAT_( COSDisk, this, m_osdi, dwOffset ) );
    EDBGPrintfDml( "{ FSeekPenalty() = %d (retail only), StorAdap.MaxTranLen = %0.3f KB, <link cmd=\"dt -r %ws!OSDiskInfo %I64x\">details</link> }\n",
            // note: Added "retail only" because this fails to tell seek penalty if running with fault injection
            // and fFakeNoSeekPenalty would be TRUE in debugged binary.
            FSeekPenalty(),
            double( m_osdi.m_ossad.MaximumTransferLength ) / 1024.0,
            WszUtilImageName(), __int64( (char*)(this) + dwOffset + OffsetOf( COSDisk, m_osdi ) ) );
    if ( m_osdi.m_szDiskModelSmart[0] != '\0' &&
            strstr( m_osdi.m_szDiskModelSmart, m_osdi.m_szDiskModel ) != NULL )
    {
        //  We only print smart model if it's longer / superset of info over disk model.
        (*pcprintf)( FORMAT_SZ( COSDisk, this, m_osdi.m_szDiskModelSmart, dwOffset ) );
    }
    else
    {
        (*pcprintf)( FORMAT_SZ( COSDisk, this, m_osdi.m_szDiskModel, dwOffset ) );
    }
    (*pcprintf)( FORMAT_SZ( COSDisk, this, m_osdi.m_szDiskFirmwareRev, dwOffset ) );
    (*pcprintf)( FORMAT_SZ( COSDisk, this, m_osdi.m_szDiskSerialNumber, dwOffset ) );

    //  Active I/O

    (*pcprintf)( "\n    Active I/O Counts:\n" );
    (*pcprintf)( FORMAT_INT( COSDisk, this, m_cioForeground, dwOffset ) );
    (*pcprintf)( FORMAT_INT( COSDisk, this, m_cioDispatching, dwOffset ) );
    (*pcprintf)( FORMAT_INT( COSDisk, this, m_cioAsyncReadDispatching, dwOffset ) );
    (*pcprintf)( FORMAT_INT( COSDisk, this, m_cioAsyncWriteDispatching, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( COSDisk, this, m_fExclusiveIo, dwOffset ) );
    (*pcprintf)( FORMAT_INT( COSDisk, this, m_cFfbOutstanding, dwOffset ) );

    //  "Last" X Tracking ...

    (*pcprintf)( "\n    Last Action Times / Info:\n" );
    // be nice to make this describe it's latency from real time
    (*pcprintf)( FORMAT_UINT( COSDisk, this, m_hrtLastFfb, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( COSDisk, this, m_hrtLastMetedDispatch, dwOffset ) );
    (*pcprintf)( FORMAT_UINT_NOLINE( COSDisk, this, m_tickPerformanceLastMeasured, dwOffset ) );
    LONG dtickOsQdLatency = DtickDelta( m_tickPerformanceLastMeasured, Pdls()->TickDLSCurrent() );
    (*pcprintf)( " ( %d ms old )\n", dtickOsQdLatency );
    (*pcprintf)( FORMAT_UINT_NOLINE( COSDisk, this, m_cioOsQueueDepth, dwOffset ) );
    (*pcprintf)( " ( %d ms old )\n", dtickOsQdLatency ); // dup print b/c not obvious m_cioOsQueueDepth relates to m_tickPerformanceLastMeasured
    

    //  I/O Q

    (*pcprintf)( "\n    I/O Queues:\n" );

    EDBGDumptypeDml( COSDisk, this, COSDisk::IOQueue, m_pIOQueue, dwOffset );
    BYTE rgbIoQueue[ sizeof( IOQueue ) + 32 ];
    memset( rgbIoQueue, 0, sizeof( rgbIoQueue ) );
    IOQueue * pIOQueue = (IOQueue*)rgbIoQueue;
    if ( FReadVariable( m_pIOQueue, pIOQueue ) )
    {
        IOQueue * pIOQueueDebuggee = this->m_pIOQueue;
        ((COSDisk*)this)->m_pIOQueue = pIOQueue; //  What could go wrong?

        //  Esoteric enough options, not sure of general usage yet ... 
        #define SUB_OBJ_FORMAT_UINT( CLASS, pointer, member, submember, offset, SZEND ) \
                        "\t%*.*s <0x%0*I64X,%3I64u>:  %I64u (0x%I64X)" SZEND, \
                        SYMBOL_LEN_MAX + 4, \
                        SYMBOL_LEN_MAX + 4, \
                        #member "->" #submember, \
                        INT( 2 * sizeof( pointer ) - 4 ), \
                        __int64( OffsetOf( CLASS, submember ) ), \
                        __int64( sizeof( (pointer)->member->submember ) ), \
                        unsigned __int64( (pointer)->member->submember ), \
                        unsigned __int64( (pointer)->member->submember )

        #define SUB_OBJ_FORMAT_POINTER_TYPE_DML( CLASS, pointer, member, SUBMEMBERCLASSSZ, submember, offset )  \
                        "\t%*.*s &lt;0x%0*I64X,%3I64u&gt;:  <link cmd=\"dt %ws!%hs 0x%I64x\">0x%I64X</link>\n", \
                        SYMBOL_LEN_MAX + 4, \
                        SYMBOL_LEN_MAX + 4, \
                        #member "->" #submember, \
                        INT( 2 * sizeof( pointer ) - 4 ), \
                        __int64( OffsetOf( CLASS, submember ) ), \
                        __int64( sizeof( (pointer)->member->submember ) ), \
                        WszUtilImageName(), \
                        SUBMEMBERCLASSSZ, \
                        unsigned __int64( (pointer)->member->submember ), \
                        unsigned __int64( (pointer)->member->submember )

        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, m_cDispatchPass, dwOffset, "\n" ) );
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, m_cDispatchContinues, dwOffset, "\n" ) );
        (*pcprintf)( FORMAT_UINT( COSDisk, this, m_cioDequeued, dwOffset ) );

        //  From here on out since, we're printing symbols nested below m_pIOQueue->, the names get deep, so
        //  temporarily double the symbol len max.
        const LONG cchSymMax = SYMBOL_LEN_MAX * 2;
        #pragma push_macro( "SYMBOL_LEN_MAX" )
        #undef SYMBOL_LEN_MAX
        #define SYMBOL_LEN_MAX  (cchSymMax)

        (*pcprintf)( "\n" );
        (*pcprintf)( "               I/O Heap Config: ---------------------------- \n" );
        // need bigger symbols names here ... 
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, m_cioQosBackgroundMax, dwOffset, "\n" ) );
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, m_cioreqQOSBackgroundLow, dwOffset, "\n" ) );
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, m_cioQosBackgroundCurrent, dwOffset, "\n" ) );
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, m_cioQosUrgentBackgroundMax, dwOffset, "\n" ) );
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, m_cioQosUrgentBackgroundCurrent, dwOffset, "\n" ) );

        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, m_cioQosBackgroundMax, dwOffset, "\n" ) );
        // related IO heap, but variable name not obvious ... rename maybe? 
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, m_cioreqMax, dwOffset, "\n" ) );
        (*pcprintf)( "               I/O Heap Pos: ------------------------------- \n" );
        // skip: CCriticalSection * m_pcritIOHeap;
        // skip: CSemaphore         m_semIOQueue;
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, fUseHeapA, dwOffset, "\n" ) );
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, iFileCurrent, dwOffset, "\n" ) );
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, ibOffsetCurrent, dwOffset, "\n" ) );

        // Note: Be smart to take the HeapA.ipioreqIOAHeapMac & HeapB.ipioreqIOBHeapMic to determine / print
        // how full, but not sure our old IO heap is long for this world, if I (SOMEONE) have my ways and
        // move everything to a R/B tree for a more dynamic Q.
        EDBGPrintfDml( SUB_OBJ_FORMAT_POINTER_TYPE_DML( COSDisk::IOQueue, this, m_pIOQueue, "COSDisk::IOQueue::IOHeapA", m_pIOHeapA, dwOffset ) );
        EDBGPrintfDml( SUB_OBJ_FORMAT_POINTER_TYPE_DML( COSDisk::IOQueue, this, m_pIOQueue, "COSDisk::IOQueue::IOHeapB", m_pIOHeapB, dwOffset ) );

        (*pcprintf)( "               VIP List: ----------------------------------- \n" );
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, m_cioVIPList, dwOffset, "\n" ) );
        // skip: CLocklessLinkedList<IOREQ>     
        // dicey - this basically destroys the structure (but in debugger memory ... so only problem is 
        // someone wants to use VIP list element lower down)
        const IOREQ * pioreqVipHeadDebuggee = m_pIOQueue->m_VIPListHead.AtomicRemoveList();
        // could add some DML for ese!COLL:CLocklessLinkedList<IOREQ> ...
        (*pcprintf)( "\t%*.*s <0x%0*I64X,--->:  0x%I64X\n", SYMBOL_LEN_MAX + 4, SYMBOL_LEN_MAX + 4, "m_pIOQueue->m_VIPListHead",
                         INT( 2 * sizeof( void* ) - 4 ), &pIOQueueDebuggee->m_VIPListHead, pioreqVipHeadDebuggee );


        (*pcprintf)( "               Meted Read Q: ------------------------------- \n" );
        //  To use the meted Q operations, need to setup a little bit of stuff

        LONG cioConcurrentMetedOpsMax = 42;
        LONG cioLowQueueThreshold = 43;
        TICK dtickStarvedMetedOpThreshold = 44;
        if ( !FReadGlobal( "g_cioConcurrentMetedOpsMax", &cioConcurrentMetedOpsMax ) ||
                !FReadGlobal( "g_cioLowQueueThreshold", &cioLowQueueThreshold ) ||
                !FReadGlobal( "g_dtickStarvedMetedOpThreshold", &dtickStarvedMetedOpThreshold ) )
        {
            (*pcprintf)( "      #failed read and set of globals, may be inaccurate values#\n" );
        }
        else
        {
            //  Be nice to get the flighted Meted Q flags, but it's in all inst's params.
            (*pcprintf)( "                                     g_cioConcurrentMetedOps <              ,   >:  %d  (max concurrent meted ops allowed)\n", cioConcurrentMetedOpsMax );
            (*pcprintf)( "                                      g_cioLowQueueThreshold <              ,   >:  %d\n", cioLowQueueThreshold );
            (*pcprintf)( "                              g_dtickStarvedMetedOpThreshold <              ,   >:  %d\n", dtickStarvedMetedOpThreshold );
            //  we set this because it is consumed in some of the member access functions below ...
            FlightConcurrentMetedOps( cioConcurrentMetedOpsMax, cioLowQueueThreshold, dtickStarvedMetedOpThreshold );
        }
    
        // skip: CCriticalSection *             m_pcritController;
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, m_qMetedAsyncReadIo.m_cioEnqueued, dwOffset, "" ) );
        (*pcprintf)( " ( aka cioWaitingQ or CioMetedReadQueue() )\n" );
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, m_qMetedAsyncReadIo.m_cioreqEnqueued, dwOffset, "\n" ) );
        // skip: CSimpleQueue<IOREQ>                m_ilQueue;
        //  Not sure why I don't need to set FIOThread() to access CioAllowedMetedOps - probably someone turned off asserts in EDBG somewhere.
        (*pcprintf)( "\t%*.*s <0x%0*I64X,--->:  0x%I64X\n", SYMBOL_LEN_MAX + 4, SYMBOL_LEN_MAX + 4, "CioAllowedMetedOps()",
                INT( 2 * sizeof( void* ) - 4 ), 0, (__int64)CioAllowedMetedOps( m_pIOQueue->CioMetedReadQueue() /* note = m_qMetedAsyncReadIo.m_cioEnqueued */ ) );
        
        const LONG cioOpenSlots = ( m_cioAsyncReadDispatching > CioAllowedMetedOps( m_pIOQueue->CioMetedReadQueue() ) ) ?
                                            0 :
                                            ( CioAllowedMetedOps( m_pIOQueue->CioMetedReadQueue() ) - m_cioAsyncReadDispatching );
        (*pcprintf)( "\t%*.*s <0x%0*I64X,--->:  0x%I64X\n", SYMBOL_LEN_MAX + 4, SYMBOL_LEN_MAX + 4, "CioReadyMetedEnqueued()", INT( 2 * sizeof( void* ) - 4 ), 0, min( m_pIOQueue->CioMetedReadQueue(), cioOpenSlots ) );

        #define ProotIoreqReal( rbt )        ( (SIZE_T)rbt.PnodeRoot() == (SIZE_T)-(LONGLONG)IOREQ::OffsetOfMetedQueueIC() ? NULL : rbt.PnodeRoot() ) 

        const IOREQ * pioreqReadBuildingRoot = ProotIoreqReal( m_pIOQueue->m_qMetedAsyncReadIo.m_irbtBuilding );
        const IOREQ * pioreqReadDrainingRoot = ProotIoreqReal( m_pIOQueue->m_qMetedAsyncReadIo.m_irbtDraining );
        // could add some DML for ese!COLL::InvasiveRedBlackTree<IFILEIBOFFSET,IOREQ,&IOREQ::OffsetOfMetedQueueIC>
        (*pcprintf)( "\t%*.*s <0x%0*I64X,--->:  0x%I64X\n", SYMBOL_LEN_MAX + 4, SYMBOL_LEN_MAX + 4, "m_pIOQueue->m_qMetedAsyncReadIo.m_irbtBuilding",
                         INT( 2 * sizeof( void* ) - 4 ), &pIOQueueDebuggee->m_qMetedAsyncReadIo.m_irbtBuilding, pioreqReadBuildingRoot );
        (*pcprintf)( "\t%*.*s <0x%0*I64X,--->:  0x%I64X\n", SYMBOL_LEN_MAX + 4, SYMBOL_LEN_MAX + 4, "m_pIOQueue->m_qMetedAsyncReadIo.m_irbtDraining",
                         INT( 2 * sizeof( void* ) - 4 ), &pIOQueueDebuggee->m_qMetedAsyncReadIo.m_irbtDraining, pioreqReadDrainingRoot );

        (*pcprintf)( "               Async Write Q: ------------------------------- \n" );
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, m_qWriteIo.m_cioEnqueued, dwOffset, "\n" ) );
        (*pcprintf)( SUB_OBJ_FORMAT_UINT( COSDisk::IOQueue, this, m_pIOQueue, m_qWriteIo.m_cioreqEnqueued, dwOffset, "\n" ) );

        const IOREQ * pioreqWriteBuilding = ProotIoreqReal( m_pIOQueue->m_qWriteIo.m_irbtBuilding );
        const IOREQ * pioreqWriteDraining = ProotIoreqReal( m_pIOQueue->m_qWriteIo.m_irbtDraining );
        (*pcprintf)( "\t%*.*s <0x%0*I64X,--->:  0x%I64X\n", SYMBOL_LEN_MAX + 4, SYMBOL_LEN_MAX + 4, "m_pIOQueue->m_qWriteIo.m_irbtBuilding", 
                         INT( 2 * sizeof( void* ) - 4 ), &pIOQueueDebuggee->m_qWriteIo.m_irbtBuilding, pioreqWriteBuilding );
        (*pcprintf)( "\t%*.*s <0x%0*I64X,--->:  0x%I64X\n", SYMBOL_LEN_MAX + 4, SYMBOL_LEN_MAX + 4, "m_pIOQueue->m_qWriteIo.m_irbtDraining",
                         INT( 2 * sizeof( void* ) - 4 ), &pIOQueueDebuggee->m_qWriteIo.m_irbtDraining, pioreqWriteDraining );

        #pragma pop_macro( "SYMBOL_LEN_MAX" )

        ((COSDisk*)this)->m_pIOQueue = pIOQueueDebuggee; //  restore it.
    }
    else
    {
        (*pcprintf)( "                       #failed load of m_pIOQueue#\n" );
    }
    // skip: CCriticalSection       m_critIOQueue;  

    (*pcprintf)( "\n" );
}

const VALUESZMAP rgiorpsz[] =
{
        //  Generic OS Layer reasons
    MapValue( iorpInvalid ),
    MapValue( iorpHeader ),
    MapValue( iorpOsLayerTracing ),
        //  Database I Reasons
    MapValue( iorpBFRead ),
        //  Database O Reasons
    MapValue( iorpBFCheckpointAdv ),
    MapValue( iorpBFAvailPool ),
    MapValue( iorpBFShrink ),
    MapValue( iorpBFFilthyFlush ),
    MapValue( iorpBFDatabaseFlush ),
        //  Transaction Log I/O Reasons
    MapValue( iorpLog ),
    MapValue( iorpLGWriteCommit ),
    MapValue( iorpLGWriteNewGen ),
    MapValue( iorpLGWriteSignal ),
    MapValue( iorpLGFlushAll ),
    MapValue( iorpLogRecRedo ),
    MapValue( iorpShadowLog ),
        //  Specialty reasons ...
    MapValue( iorpCheckpoint ),
    MapValue( iorpDirectAccessUtil ),
    MapValue( iorpBackup ),
    MapValue( iorpRestore ),
    MapValue( iorpPatchFix ),
    MapValue( iorpDatabaseExtension ),
    MapValue( iorpDatabaseShrink ),
    MapValue( iorpFlushMap )
};

const CHAR * SzIOIorp( IOREASONPRIMARY iorpTarget )
{
    for( ULONG i = 0; i < _countof(rgiorpsz); i++ )
    {
        if ( rgiorpsz[i].value == (__int64)iorpTarget )
        {
            return rgiorpsz[i].sz;
        }
    }
    return "Unknown IORP!";
}

VOID DumpOneIOREQ(
    const IOREQ * const pioreqDebuggee,
    const IOREQ * const pioreq
    )
{
    const SIZE_T    dwOffset        = (BYTE *)pioreqDebuggee - (BYTE *)pioreq;
    DWORD_PTR       dwPfnOffset;
    CHAR            szCompletion[200] = "";

    CHAR            szIOSyncComplete[40];
    CHAR            szBFIAsyncWriteComplete[40];

    OSStrCbFormatA( szIOSyncComplete, sizeof(szIOSyncComplete), "%ws!COSFile::IOSyncComplete_", WszUtilImageName() );
    OSStrCbFormatA( szBFIAsyncWriteComplete, sizeof(szBFIAsyncWriteComplete), "%ws!BFIAsyncWriteComplete", WszUtilImageName() );

    dprintf( FORMAT_VOID( IOREQ, pioreq, ovlp, dwOffset ) );
    dprintf( FORMAT_VOID( IOREQ, pioreq, m_apic, dwOffset ) );
    dprintf( FORMAT_UINT( IOREQ, pioreq, ipioreqHeap, dwOffset ) );
    dprintf( FORMAT_ENUM_BF( IOREQ, pioreq, m_ioreqtype, dwOffset, mpioreqtypesz, IOREQ::ioreqUnknown, IOREQ::ioreqMax ) );
    dprintf( FORMAT_ENUM_BF( IOREQ, pioreq, m_iomethod, dwOffset, mpiomethodsz, IOREQ::iomethodNone, IOREQ::ioreqMax ) );
    dprintf( FORMAT_UINT_BF( IOREQ, pioreq, m_ciotime, dwOffset ) );
    dprintf( FORMAT_VOID( IOREQ, pioreq, m_crit, dwOffset ) );
    dprintf( FORMAT_UINT_BF( IOREQ, pioreq, m_grbitHungActionsTaken, dwOffset ) );
    dprintf( FORMAT_POINTER( IOREQ, pioreq, hrtIOStart, dwOffset ) );
    _OSFILE _osf;
    memset( &_osf, 0, sizeof( _osf ) );
    if ( FReadVariable( pioreq->p_osf, &_osf ) )
    {
        dprintf( FORMAT_( IOREQ, pioreq, p_osf, dwOffset ) );
        EDBGPrintfDml( "0x%0*I64X = { <link cmd=\"!ese dump COSDisk 0x%I64x\">m_posd</link> = <link cmd=\"dt %ws!COSDisk 0x%I64x\">0x%I64x</link>, hFile = 0x%I64x }\n",
            INT( 2 * sizeof( pioreq->p_osf ) ), __int64( pioreq->p_osf ),
            __int64( _osf.m_posd ), WszUtilImageName(), __int64( _osf.m_posd ), __int64( _osf.m_posd ),
            __int64( _osf.hFile ) );
        memset( &_osf, 0, sizeof( _osf ) ); // defeat .dtor from trying to free things
    }
    else
    {
        dprintf( FORMAT_POINTER( IOREQ, pioreq, p_osf, dwOffset ) );
    }
    dprintf( FORMAT_POINTER( IOREQ, pioreq, m_posdCurrentIO, dwOffset ) );
    dprintf( FORMAT_BOOL_BF( IOREQ, pioreq, fWrite, dwOffset ) );
    dprintf( FORMAT_UINT_BF( IOREQ, pioreq, group, dwOffset ) );
    dprintf( FORMAT_BOOL_BF( IOREQ, pioreq, fCoalesced, dwOffset ) );
    dprintf( FORMAT_BOOL_BF( IOREQ, pioreq, fFromReservePool, dwOffset ) );
    dprintf( FORMAT_BOOL_BF( IOREQ, pioreq, m_fFromTLSCache, dwOffset ) );
    dprintf( FORMAT_BOOL_BF( IOREQ, pioreq, m_fHasHeapReservation, dwOffset ) );
    dprintf( FORMAT_BOOL_BF( IOREQ, pioreq, m_fHasBackgroundReservation, dwOffset ) );
    dprintf( FORMAT_BOOL_BF( IOREQ, pioreq, m_fHasUrgentBackgroundReservation, dwOffset ) );
    dprintf( FORMAT_BOOL_BF( IOREQ, pioreq, m_fCanCombineIO, dwOffset ) );
#ifndef RTM
    dprintf( FORMAT_BOOL_BF( IOREQ, pioreq, m_fDequeueCombineIO, dwOffset ) );

    dprintf( FORMAT_BOOL_BF( IOREQ, pioreq, m_fSplitIO, dwOffset ) );
    dprintf( FORMAT_BOOL_BF( IOREQ, pioreq, m_fReMergeSplitIO, dwOffset ) );
    dprintf( FORMAT_BOOL_BF( IOREQ, pioreq, m_fOutOfMemory, dwOffset ) );
    dprintf( FORMAT_INT_BF( IOREQ, pioreq, m_cTooComplex, dwOffset ) );
    dprintf( FORMAT_INT_BF( IOREQ, pioreq, m_cRetries, dwOffset ) );
#endif

    dprintf( FORMAT_VOID( IOREQ, pioreq, m_hiic, dwOffset ) );
    EDBGDprintfDumplinkDml( IOREQ, pioreq, IOREQ, pioreqIorunNext, dwOffset );
    EDBGDprintfDumplinkDml( IOREQ, pioreq, IOREQ, pioreqVipList, dwOffset );

    dprintf( FORMAT_( IOREQ, pioreq, grbitQOS, dwOffset ) );
    dprintf( "0x%08x = { ", pioreq->grbitQOS );
    if ( pioreq->grbitQOS & qosIOOSMask )
    {
        AssertEDBG( qosIOOSLowPriority == ( pioreq->grbitQOS & qosIOOSLowPriority ) );
        dprintf( "IOOS = 0x%x (%hs), ", qosIOOSLowPriority, "qosIOOSLowPriority" );
    }
    if ( pioreq->grbitQOS & qosIODispatchMask )
    {
        dprintf( "IODispatch = 0x%x (", pioreq->grbitQOS & qosIODispatchMask );
        if ( ( pioreq->grbitQOS & qosIODispatchMask ) == qosIODispatchImmediate )
        {
            dprintf( "qosIODispatchImmediate" );
        }
        if ( ( pioreq->grbitQOS & qosIODispatchMask ) == qosIODispatchBackground )
        {
            dprintf( "qosIODispatchBackground" );
        }
        if ( pioreq->grbitQOS & qosIODispatchUrgentBackgroundMask )
        {
            dprintf( "qosIODispatchUrgentBackgroundMask, cio = %d", CioOSDiskPerfCounterIOMaxFromUrgentQOS( pioreq->grbitQOS ) );
        }
        if ( ( pioreq->grbitQOS & qosIODispatchMask ) == qosIODispatchIdle )
        {
            dprintf( "qosIODispatchIdle" );
        }
        if ( ( pioreq->grbitQOS & qosIODispatchWriteMeted ) == qosIODispatchWriteMeted )
        {
            dprintf( ", qosIODispatchWriteMeted" );
        }
        dprintf( "), " );
    }
    if ( pioreq->grbitQOS & qosIOOptimizeMask )
    {
        AssertEDBG( 0 == ( pioreq->grbitQOS & ( qosIOOptimizeCombinable | qosIOOptimizeOverrideMaxIOLimits | qosIOOptimizeOverwriteTracing ) /* do NOT use qosIOOptimizeMask */ ) );
        dprintf( "IOOptimize = 0x%x (%hs | %hs | %hs), ",
                pioreq->grbitQOS & qosIOOptimizeMask,
                pioreq->grbitQOS & qosIOOptimizeCombinable ? "qosIOOptimizeCombinable" : "",
                pioreq->grbitQOS & qosIOOptimizeOverrideMaxIOLimits ? "qosIOOptimizeOverrideMaxIOLimits" : "",
                pioreq->grbitQOS & qosIOOptimizeOverwriteTracing ? "qosIOOptimizeOverwriteTracing" : "" );
    }
    if ( pioreq->grbitQOS & qosIOPoolReserved )
    {
        dprintf( "IOPool = 0x%x (%hs), ", qosIOPoolReserved, "qosIOPoolReserved" );
    }
    if ( pioreq->grbitQOS & qosIOSignalSlowSyncIO )
    {
        dprintf( "IOSignal = 0x%x (%hs), ", pioreq->grbitQOS & qosIOSignalSlowSyncIO, "qosIOSignalSlowSyncIO" );
    }
    if ( pioreq->grbitQOS & qosIOCompleteMask )
    {
        dprintf( "IOComplete = 0x%x (", pioreq->grbitQOS & qosIOCompleteMask );
        if ( pioreq->grbitQOS & qosIOCompleteIoCombined )
        {
            dprintf( "qosIOCompleteIoCombined " );
        }
        if ( pioreq->grbitQOS & qosIOCompleteReadGameOn )
        {
            dprintf( "qosIOCompleteReadGameOn " );
        }
        if ( pioreq->grbitQOS & qosIOCompleteWriteGameOn )
        {
            dprintf( "qosIOCompleteWriteGameOn " );
        }
        if ( pioreq->grbitQOS & qosIOCompleteIoSlow )
        {
            dprintf( "qosIOCompleteIoSlow " );
        }
        dprintf( "), " );
    }
    if ( pioreq->grbitQOS & qosIOReadCopyMask )
    {
        const ULONG icopy = IReadCopyFromQos( pioreq->grbitQOS );
        dprintf( "IOReadCopy = %d, ", icopy );
    }
    dprintf( "}\n" );

    dprintf( FORMAT_( IOREQ, pioreq, m_tc.etc.iorReason, dwOffset ) );
    dprintf( "0x%0I64X = { iorp = %hs(%d), iors = %d, iort = %d, ioru = %d (JetOp), iorf = 0x%x }\n",
                ( pioreq->m_tc.etc.iorReason.QwIor() ),
                SzIOIorp( pioreq->m_tc.etc.iorReason.Iorp() ), pioreq->m_tc.etc.iorReason.Iorp(),
                pioreq->m_tc.etc.iorReason.Iors(),
                pioreq->m_tc.etc.iorReason.Iort(),
                pioreq->m_tc.etc.iorReason.Ioru(),
                pioreq->m_tc.etc.iorReason.Iorf() );

    if ( FSymbolFromAddress( pioreq->pfnCompletion, szCompletion, sizeof( szCompletion ), &dwPfnOffset ) )
    {
        dprintf( FORMAT_POINTER_NOLINE( IOREQ, pioreq, pfnCompletion, dwOffset ) );
        dprintf( " = %hs\n", szCompletion, dwPfnOffset );
        // Some hits ... 
        //      ESE!COSFile::IOSyncComplete_
        //      ese!BFIAsyncWriteComplete
        //  Peculiar that one symbol is upper case, and another is lower case.
    }
    else
    {
        dprintf( FORMAT_POINTER( IOREQ, pioreq, pfnCompletion, dwOffset ) );
    }

    //  parse some context from this IO, so we can better print the remaing elements
    //
    BYTE rgbIOComplete[sizeof(COSFile::CIOComplete)];
    COSFile::CIOComplete * piocomplete = (COSFile::CIOComplete*)rgbIOComplete;
    const BOOL fSyncIOContext = 0 == _stricmp( szIOSyncComplete, szCompletion ) &&
                                    FReadVariable( (COSFile::CIOComplete*)pioreq->dwCompletionKey, piocomplete );
    const BOOL fBF = ( pioreq->m_tc.etc.iorReason.Iorp() == iorpBFRead ) ||
                        ( 0 == _stricmp( szBFIAsyncWriteComplete, szCompletion ) );

    //  dump the completion key if it's sync or not a BF IO
    if ( fSyncIOContext )
    {
        dprintf( FORMAT_POINTER_NOLINE( IOREQ, pioreq, dwCompletionKey, dwOffset ) );
        EDBGPrintfDml( " { m_sig = %hs, m_err = %d (0x%x), m_tidWait = <link cmd=\"~~[%I64x]s\">%d (0x%x)</link>, m_keyIOComplete = %Ix, m_pfn... }\n",
#ifdef DEBUG
                        "unknown",
#else
                        piocomplete->m_signal.FTryWait() ? "Set (completed)" : "Unset (outstanding)",
#endif
                        piocomplete->m_err, piocomplete->m_err,
                        piocomplete->m_tidWait, piocomplete->m_tidWait, piocomplete->m_tidWait, piocomplete->m_keyIOComplete );
    }
    else if ( !fBF )
    {
        dprintf( FORMAT_POINTER( IOREQ, pioreq, dwCompletionKey, dwOffset ) );
    }

    if ( fBF )
    {
        //  We have an IO mapping to a BF IO / i.e. a page.

        ULONG cbPage = Pdls()->CbPage();

        if ( fBF && !fSyncIOContext )
        {
#define FORMAT_POINTER_BF_DML( CLASS, pointer, member, offset ) \
                "\t%*.*s &lt;0x%0*I64X,%3I64u&gt;:  <link cmd=\"!ese dump BF 0x%0*I64X\">0x%0*I64X</link>\n", \
                SYMBOL_LEN_MAX, \
                SYMBOL_LEN_MAX, \
                #member, \
                INT( 2 * sizeof( pointer ) ), \
                __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
                __int64( sizeof( (pointer)->member ) ), \
                INT( 2 * sizeof( (pointer)->member ) ), \
                __int64( (pointer)->member ), \
                INT( 2 * sizeof( (pointer)->member ) ), \
                __int64( (pointer)->member )
            EDBGPrintfDml( FORMAT_POINTER_BF_DML( IOREQ, pioreq, dwCompletionKey, dwOffset ) );
        }

        BF * pbf = NULL;
        if ( FFetchVariable( (BF*)pioreq->dwCompletionKey, &pbf ) )
        {
            dprintf( FORMAT_( IOREQ, pioreq, ibOffset, dwOffset ) );
            AssertEDBG( pbf->pgno == PgnoOfOffset( pioreq->ibOffset ) );
            EDBGPrintfDml( "%I64u (0x%I64X, <link cmd=\"!ese cachefind %x 0x%0I64X\">pgno=%d</link>)\n",
                pioreq->ibOffset, pioreq->ibOffset,
                pbf->ifmp, PgnoOfOffset( pioreq->ibOffset ),
                PgnoOfOffset( pioreq->ibOffset ) );
            Unfetch( pbf );
        }
        else
        {
            dprintf( FORMAT_( IOREQ, pioreq, ibOffset, dwOffset ) );
            EDBGPrintfDml( "%I64u (0x%I64X, pgno=%d)\n", pioreq->ibOffset, pioreq->ibOffset, PgnoOfOffset( pioreq->ibOffset ) );
        }

        dprintf( FORMAT_( IOREQ, pioreq, pbData, dwOffset ) );
        EDBGPrintfDml( "<link cmd=\"!ese dump page %x 0x%0I64X ht\">%I64u (0x%I64X)</link>\n",
                                PgnoOfOffset( pioreq->ibOffset ), pioreq->pbData,
                                pioreq->pbData, pioreq->pbData );
        dprintf( FORMAT_( IOREQ, pioreq, cbData, dwOffset ) );
        EDBGPrintfDml( "%I64u (0x%I64X, cpg=%d)\n", pioreq->cbData, pioreq->cbData, pioreq->cbData / cbPage );
    }
    else if ( pioreq->m_tc.etc.iorReason.Iorp() == iorpHeader ||
                pioreq->m_tc.etc.iorReason.Iors() == iorsHeader )
    {
        //  We have a header of one of the jet file types ...

        BYTE * pbHeader = (BYTE*)malloc( pioreq->cbData );

        AssertEDBG( pioreq->ibOffset == 0 );    // anything else wouldn't make sense.
        AssertEDBG( pioreq->cbData >= OffsetOf( DBFILEHDR, le_filetype ) );

        dprintf( FORMAT_UINT( IOREQ, pioreq, ibOffset, dwOffset ) );
        if ( pbHeader &&
                pioreq->ibOffset == 0 &&
                pioreq->cbData >= OffsetOf( DBFILEHDR, le_filetype ) &&
                FReadVariable( (BYTE *)(pioreq->pbData), pbHeader, pioreq->cbData ) )
        {
            switch( ((DBFILEHDR*)pbHeader)->le_filetype )
            {
                case JET_filetypeDatabase:
                    dprintf( FORMAT_( IOREQ, pioreq, pbData, dwOffset ) );
                    EDBGPrintfDml( "<link cmd=\"!ese dump DBFILEHDR 0x%0I64X\">0x%0I64X</link>\n", pioreq->pbData, pioreq->pbData, pioreq->pbData );
                    break;

                case JET_filetypeLog:
                    dprintf( FORMAT_( IOREQ, pioreq, pbData, dwOffset ) );
                    EDBGPrintfDml( "<link cmd=\"dt -r %ws!LGFILEHDR 0x%0I64X\">0x%0I64X</link>\n", WszUtilImageName(), pioreq->pbData, pioreq->pbData, pioreq->pbData );
                    break;

                case JET_filetypeCheckpoint:
                    dprintf( FORMAT_( IOREQ, pioreq, pbData, dwOffset ) );
                    EDBGPrintfDml( "<link cmd=\"dt -r %ws!CHECKPOINT 0x%0I64X ht\">0x%0I64X</link>\n", WszUtilImageName(), pioreq->pbData, pioreq->pbData, pioreq->pbData );
                    break;

                case JET_filetypeTempDatabase:
                case JET_filetypeFTL:
                case JET_filetypeFlushMap:
                    dprintf( FORMAT_POINTER( IOREQ, pioreq, pbData, dwOffset ) );   // could improve
                    break;

                case JET_filetypeStreamingFile:
                    AssertEDBGSz( fFalse, "Legacy header type, you looking at streaming files?" );
                    dprintf( FORMAT_POINTER( IOREQ, pioreq, pbData, dwOffset ) );
                    break;

                case JET_filetypeUnknown:
                default:
                    AssertEDBGSz( fFalse, "Unknown header type, you sure this is an ESE file?" );
                    dprintf( FORMAT_POINTER( IOREQ, pioreq, pbData, dwOffset ) );
                    break;
            }
        }
        else
        {
            dprintf( FORMAT_POINTER( IOREQ, pioreq, pbData, dwOffset ) );
        }
        dprintf( FORMAT_UINT( IOREQ, pioreq, cbData, dwOffset ) );
        if ( pbHeader )
        {
            free( pbHeader );
        }
    }
    else if ( pioreq->m_tc.etc.iorReason.Iorp() >= iorpLog &&
                pioreq->m_tc.etc.iorReason.Iorp() <= iorpShadowLog )
    {
        //  We have an IO mapping to a Log IO.

        struct LGSEGHDR * plgseghdr = (struct LGSEGHDR *)malloc( pioreq->cbData );
        if ( plgseghdr &&
                FReadVariable( (BYTE*)(pioreq->pbData), (BYTE*)plgseghdr, pioreq->cbData ) )
        {
            dprintf( FORMAT_( IOREQ, pioreq, ibOffset, dwOffset ) );
            EDBGPrintfDml( "%I64u (0x%I64X, lgpos=%08x:%04x:%04x)\n", pioreq->ibOffset, pioreq->ibOffset,
                                (ULONG)(plgseghdr->le_lgposSegment.le_lGeneration), (ULONG)(plgseghdr->le_lgposSegment.le_isec), (ULONG)(plgseghdr->le_lgposSegment.le_ib) );
            dprintf( FORMAT_( IOREQ, pioreq, pbData, dwOffset ) );
            EDBGPrintfDml( "<link cmd=\"dt -r %ws!LGSEGHDR 0x%0I64X\">0x%0I64X</link>\n", WszUtilImageName(), pioreq->pbData, pioreq->pbData, pioreq->pbData );
            dprintf( FORMAT_UINT( IOREQ, pioreq, cbData, dwOffset ) );
        }
        else
        {
            dprintf( FORMAT_UINT( IOREQ, pioreq, ibOffset, dwOffset ) );
            dprintf( FORMAT_POINTER( IOREQ, pioreq, pbData, dwOffset ) );
            dprintf( FORMAT_UINT( IOREQ, pioreq, cbData, dwOffset ) );
        }
    }
    else
    {
        //  catch all for anything we really didn't understand ... could be eseutil / iorpDirectAccessUtil,
        //  or tasks such as iorpBackup, or iorpShrink, etc.

        dprintf( FORMAT_UINT( IOREQ, pioreq, ibOffset, dwOffset ) );
        dprintf( FORMAT_POINTER( IOREQ, pioreq, pbData, dwOffset ) );
        dprintf( FORMAT_UINT( IOREQ, pioreq, cbData, dwOffset ) );
    }

    dprintf( FORMAT_UINT( IOREQ, pioreq, m_tidAlloc, dwOffset ) );
    dprintf( FORMAT_UINT( IOREQ, pioreq, m_tickAlloc, dwOffset ) );
}

VOID CDUMPA<IOREQ>::Dump(
    PDEBUG_CLIENT pdebugClient,
    INT argc, const CHAR * const argv[]
    ) const
    // Don't understand why there are two versions of Dump type functions?  freaky shtuff going down.
    //void ::Dump( CPRINTF * pcprintf, DWORD_PTR ulBase ) const
{
    EDBGIOREQCHUNK *        pedbgioreqchunkRoot = NULL;

    IOREQ *     pioreqDebuggeeHead  = NULL;
    IOREQ *     pioreqHead          = NULL;

    IOREQ *     pioreqDebuggeeCurr  = NULL;
    IOREQ *     pioreqCurr          = NULL;

    if ( argc < 1 || !FAddressFromSz( argv[0], &pioreqDebuggeeHead ) )
    {
        dprintf( "Usage: DUMP IOREQ <address> [rundump|norunstats]\n" );
        return;
    }

    BOOL fRunDump   = fFalse;
    BOOL fRunStats  = fTrue;    // defaults to on

    if ( argc >= 2 )
    {
        if ( 0 == _stricmp( argv[1], "dumpall" ) )
        {
            fRunDump = fTrue;
        }
        if ( 0 == _stricmp( argv[1], "norunstats" ) )
        {
            //  using this makes this debugger ext more robust, obviously.
            fRunStats = fFalse;
        }
    }

    if ( fRunDump || fRunStats )
    {
        if ( !FFetchIOREQCHUNKSLIST( &pedbgioreqchunkRoot ) )
        {
            dprintf( "Error: Could not read some/all global IOREQ variables.\n" );
            goto HandleError;
        }

        dprintf( "[IOREQ] 0x%p bytes @ 0x%N\n", QWORD( sizeof( IOREQ ) ), pioreqDebuggeeHead );

        const BOOL bHead = !FExistsPrecursorIOREQ( pioreqDebuggeeHead, pedbgioreqchunkRoot );
        if ( !bHead )
        {
            dprintf( "  Warning: This is not the head most IOREQ in this chain!  Run stats may be off.\n" );
        }
    }

    if( FFetchVariable( pioreqDebuggeeHead, &pioreqHead ) )
    {
        DumpOneIOREQ( pioreqDebuggeeHead, pioreqHead );
    }

    if ( !fRunDump && !fRunStats )
    {
        //  we are done, this is all user asked us to do.
        goto HandleError;
    }

    ULONG cioreq        = 0;
    QWORD ibOffsetStart = 0xFFFFFFFFFFFFFFFF;
    QWORD ibOffsetEnd   = 0;
    ULONG cbGapTotal    = 0;

    //  validation vars
    IOREQ * pioreqDebuggeeLast = NULL;
    QWORD ibOffsetLast  = 0;
    DWORD cbLast        = 0;

    dprintf( "IOREQ Chain\n" );

    pioreqDebuggeeCurr  = pioreqDebuggeeHead;
    while( pioreqDebuggeeCurr && FFetchVariable( pioreqDebuggeeCurr, &pioreqCurr ) )
    {
        cioreq++;

        if ( ibOffsetStart > pioreqCurr->ibOffset )
        {
            ibOffsetStart = pioreqCurr->ibOffset;
        }

        if ( ibOffsetEnd < ( pioreqCurr->ibOffset + pioreqCurr->cbData ) )
        {
            ibOffsetEnd = ( pioreqCurr->ibOffset + pioreqCurr->cbData );
        }

        if ( pioreqDebuggeeCurr != pioreqDebuggeeHead )
        {
            if ( ibOffsetLast >= pioreqCurr->ibOffset )
            {
                dprintf("      ERROR: Non-strictly ascending offsets in %p vs. %p (gap calculations will be wonky)\n", pioreqDebuggeeLast, pioreqCurr );
            }
            if ( ( ibOffsetLast + cbLast ) > pioreqCurr->ibOffset )
            {
                dprintf("      ERROR: Overlapping IOREQ spans in %p vs. %p\n", pioreqDebuggeeLast, pioreqCurr );
            }
            DWORD dwGap = DWORD ( pioreqCurr->ibOffset - ( ibOffsetLast + cbLast ) );

            if ( dwGap )
            {
                dprintf( "    IOREQ[xx] = (read gap), %d bytes\n", dwGap );
                if ( pioreqHead->fWrite || pioreqCurr->fWrite )
                {
                    dprintf("      ERROR: Whoa! Nope, gap in a chain with writable IOs!\n", pioreqDebuggeeLast, pioreqCurr );
                }

                cbGapTotal +=  dwGap;
            }
        }
        dprintf( "    IOREQ[%02d] = 0x%N, %d bytes\n", cioreq, pioreqDebuggeeCurr, pioreqCurr->cbData );

        if ( fRunDump )
        {
            DumpOneIOREQ( pioreqDebuggeeCurr, pioreqCurr );
        }

        ibOffsetLast = pioreqCurr->ibOffset;
        cbLast = pioreqCurr->cbData;
        pioreqDebuggeeLast = pioreqDebuggeeCurr;
    
        pioreqDebuggeeCurr = pioreqCurr->pioreqIorunNext;
        Unfetch( pioreqCurr );
        pioreqCurr = NULL;
    }

    DWORD cbRun = (DWORD) ( ibOffsetEnd - ibOffsetStart );
    dprintf( "  cioreq = %d, cbRun = %d, cbGapTotal = %d\n", cioreq, cbRun, cbGapTotal );

HandleError:

    dprintf( "\n--------------------\n\n" );

    UnfetchIOREQCHUNKSLIST( pedbgioreqchunkRoot );
    if ( pioreqHead )
    {
        Unfetch( pioreqHead );
    }
    if ( pioreqCurr )
    {
        Unfetch( pioreqCurr );
    }
}

DEBUG_EXT( EDBGDumpReferenceLog )
/*++

Routine Description:

    Dumps the specified reference log

Return Value:

    None.

--*/

{
    ULONGLONG refLogAddress = 0;
    ULONGLONG entryAddress;
    LONG numEntries;
    REF_TRACE_LOG logHeader;
    REF_TRACE_LOG_ENTRY logEntry = { 0 };
    LONG i;
    DWORD_PTR offset;
    PCHAR format;
    LONG index;
    CHAR symbol[MAX_SYMBOL];

    if ( argc < 1 )
    {
        dprintf( "Usage: ref <address>" );
        return;
    }

    refLogAddress = GetExpression( argv[0] );
    if ( refLogAddress == 0 )
    {
        dprintf( "cannot evaluate %s\n", argv[0] );
        return;
    }

    //
    // Read the log header, perform some sanity checks.
    //

    if( !FEDBGMemoryRead(
            refLogAddress,
            &logHeader,
            sizeof(logHeader),
            NULL
            ) )
    {
        dprintf(
            "cannot read memory @ %p\n",
            (PVOID)refLogAddress
            );

        return;
    }

    dprintf(
        "log @ %p:\n"
        "    Signature = %08lx (%s)\n"
        "    LogSize   = %lu\n"
        "    NextEntry = %lu\n"
        "    EntrySize = %lu\n"
        "    LogBuffer = %p\n",
        (PVOID)refLogAddress,
        logHeader.Signature,
        logHeader.Signature == REF_TRACE_LOG_SIGNATURE
            ? "OK"
            : logHeader.Signature == REF_TRACE_LOG_SIGNATURE_X
              ? "FREED"
              : "INVALID",
        logHeader.cLogSize,
        logHeader.NextEntry,
        logHeader.cbEntrySize,
        logHeader.pLogBuffer
        );

    if( logHeader.pLogBuffer > ( (PUCHAR)refLogAddress + sizeof(logHeader) ) )
    {
        dprintf(
            "    Extra Data @ %p\n",
            (PVOID)( refLogAddress + sizeof(logHeader) )
            );
    }

    if( logHeader.Signature != REF_TRACE_LOG_SIGNATURE &&
        logHeader.Signature != REF_TRACE_LOG_SIGNATURE_X )
    {
        dprintf(
            "log @ %p has invalid signature %08lx:\n",
            (PVOID)refLogAddress,
            logHeader.Signature
            );

        return;
    }

    if( logHeader.cbEntrySize < sizeof(logEntry) )
    {
        dprintf(
            "log @ %p is not a ref count log\n",
            (PVOID)refLogAddress
            );

        return;
    }

    if( logHeader.NextEntry == REF_TRACE_LOG_NO_ENTRY )
    {
        dprintf(
            "empty log @ %p\n",
            (PVOID)refLogAddress
            );

        return;
    }

    //
    // Calculate the starting address and number of entries.
    //

    if( logHeader.NextEntry < logHeader.cLogSize )
    {
        numEntries = logHeader.NextEntry + 1;
        index = 0;
    }
    else
    {
        numEntries = logHeader.cLogSize;
        index = ( logHeader.NextEntry + 1 ) % logHeader.cLogSize;
    }

    entryAddress = (ULONG_PTR)logHeader.pLogBuffer + (ULONG_PTR)( index * logHeader.cbEntrySize );

    //
    // Dump the log.
    //

    for( ;
         numEntries > 0;
         index += 1,
         numEntries--,
         entryAddress += logHeader.cbEntrySize )
    {
        if( FEDBGCheckForCtrlC() )
        {
            break;
        }

        if( index >= logHeader.cLogSize )
        {
            index = 0;
            entryAddress = (ULONG_PTR)logHeader.pLogBuffer;
        }

        if( !FEDBGMemoryRead(
                entryAddress,
                &logEntry,
                sizeof(logEntry),
                NULL
                ) )
        {
            dprintf(
                "cannot read memory @ %p\n",
                (ULONG_PTR)entryAddress
                );

            return;
        }

        dprintf(
            "\nThread = %08p, hrt = %I64d, Context = %08p, NewRefCount = %-10ld : %ld\n",
            logEntry.ThreadId,
            logEntry.hrt,
            logEntry.pContext,
            logEntry.NewRefCount,
            index
            );

        for ( i = 0 ; i < REF_TRACE_LOG_STACK_DEPTH ; i++ )
        {
            if( logEntry.Stack[i] == NULL )
            {
                break;
            }

            (void)FSymbolFromAddress( (ULONG_PTR*)logEntry.Stack[i], symbol, sizeof(symbol), &offset );

            if( symbol[0] == '\0' )
            {
                format = "    %11p\n";
            }
            else if( offset == 0 )
            {
                format = "    %11p : %s\n";
            }
            else
            {
                format = "    %11p : %s+0x%0p\n";
            }

            dprintf(
                format,
                logEntry.Stack[i],
                symbol,
                offset
                );
        }

        if ( logHeader.cbEntrySize > sizeof(logEntry) )
        {
            dprintf( "Extra information: address %0p size %d\n",
                     entryAddress + sizeof(logEntry),
                     logHeader.cbEntrySize - sizeof(logEntry) );
        }
    }
} // DumpReferenceLog

ERR ErrEDBGInitLocal()
{
    const ERR errDls = DEBUGGER_LOCAL_STORE::ErrDlsInit( fTrue );
    if ( errDls )
    {
        dprintf( "Failure initializing debugger local state, %d (0x%x) @ %d\n", errDls, errDls, Postls()->m_efLastErr.UlLine() );
    }
    return errDls;
}

VOID EDBGTermLocal()
{
    Pdls()->CompleteWarnings();
    DEBUGGER_LOCAL_STORE::DlsDestroy();
}

ERR ErrEDBGCacheQueryLocal(
        __in const INT argc,
        __in_ecount(argc) const CHAR * const argv[],
        __inout void * pvResult )
{
    ERR err = JET_errSuccess;

    Call( ErrEDBGInitLocal() );

    //  there are a couple conditions we would probably not handle well in CacheQuery / Local, so
    //  trim out and reject those scenarios as a pre-emtive strike.

    for( INT iarg = 0; iarg < argc && argv[iarg]; iarg++ )
    {
        if ( ( 0 == strncmp( argv[iarg], "pv->", 4 ) ) ||
                ( 0 == strncmp( argv[iarg], "histo:pv->", 10 ) ) )
        {
            AssertSz( fFalse, "We can't be sure this arg (%hs) will work on a live ESE, probably need locking.", argv[iarg] );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    if ( 0 == cbfInit )
    {
        AssertSz( fFalse, "Cache uninitialized ... shouldn't be calling this API in this state." );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    CIterQuery * piq = NULL;
    Call( ErrIQCreateIterQueryCount( &g_iedBf, argc, argv, pvResult, &piq ) );

    Call( ErrEDBGQueryCache( 1, &piq ) );

HandleError:

    EDBGTermLocal();

    return err;
}


#pragma warning(default:4293)

#endif  //  DEBUGGER_EXTENSION_ESE

#endif  //  DEBUGGER_EXTENSION


extern UINT g_wAssertAction;
extern BOOL g_fOverrideEnforceFailure;
extern VOID (*g_pfnReportEnforceFailure)( const WCHAR * wszContext, const CHAR* szMessage, const WCHAR* wszIssueSource );

extern "C" {

//  ================================================================
HRESULT CALLBACK ese(
    __in PDEBUG_CLIENT  pdebugClient,
    __in PCSTR  lpArgumentString
    )
//  ================================================================
{

#if DBG
    // Compile error if the prototype is incorrect.
    // Make it debug-only to avoid the PREfast warning of
    // an unused variable.
    PDEBUG_EXTENSION_CALL pdebugcall = ese;

    Unused( pdebugcall );
#endif

    HRESULT hr = S_OK;
#ifdef DEBUGGER_EXTENSION

#ifdef DEBUGGER_EXTENSION_ESE

    TRY
    {
        //  we don't want assertions appearing on the screen during debugging
        if( !fDebugMode )
        {
            g_wAssertAction = JET_AssertSkipAll;
            g_fOverrideEnforceFailure = fTrue;
            g_pfnReportEnforceFailure = ReportEnforceFailure;
        }
        else
        {
            g_wAssertAction = JET_AssertMsgBox;
            g_fOverrideEnforceFailure = fFalse;
            g_pfnReportEnforceFailure = NULL;
        }

        if ( g_DebugClient == NULL || g_DebugControl == NULL )
        {
            OSEdbgCreateDebuggerInterface( pdebugClient );
        }

        // disable perf counters
        g_fDisablePerfmon = fTrue;

        if ( !g_fTryInitEDBGGlobals )
        {
            EDBGGlobals * pEGT = NULL;

            // The unqualified version will work if the symbols have already been loaded.
            // Otherwise by explicitly specifying the module, the prefix will be cached
            // for later.
            CHAR szQUalifiedGlobalsName[ 64 ];
            OSStrCbFormatA( szQUalifiedGlobalsName, sizeof( szQUalifiedGlobalsName ), "%ws!rgEDBGGlobals", WszUtilImageName() );

            if ( !FAddressFromGlobal( "rgEDBGGlobals", &pEGT )
                 && !FAddressFromGlobal( szQUalifiedGlobalsName, &pEGT ) )
            {
                dprintf( "Couldn't automatically resolve symbol rgEDBGGlobals (also tried '%s').\n", szQUalifiedGlobalsName );
            }

            if ( pEGT && FLoadEDBGGlobals( pEGT ) )
            {
                dprintf( "Note: auto-loaded global table = 0x%N\n", g_rgEDBGGlobalsDebugger );
            }
            else
            {
                dprintf( "Couldn't auto-load global table, must use !ese.ese globals <rgEDBGGlobals>.\n" );
            }
            g_fTryInitEDBGGlobals = fTrue;
        }

        const ERR errDls = DEBUGGER_LOCAL_STORE::ErrDlsInit( fFalse );
        if ( errDls )
        {
            dprintf( "Failure initializing debugger local state, %d (0x%x) @ %d\n", errDls, errDls, Postls()->m_efLastErr.UlLine() );
        }

        VOID * pv = LocalAlloc( LMEM_ZEROINIT, strlen( lpArgumentString ) + 1 );
        if ( NULL == pv )
        {
            dprintf( "ERROR: Could not parse argument string (OutOfMemory).\n" );
            goto Return;
        }

        memcpy( pv, lpArgumentString, strlen( lpArgumentString ) );
        CHAR * argv[256];
        INT argc = SzToRgsz( argv, reinterpret_cast<CHAR *>( pv ), _countof(argv) );

        if( argc < 1 )
        {
            EDBGHelp( pdebugClient, argc, (const CHAR **)argv );
            goto Return;
        }

        INT ifuncmap;
        for( ifuncmap = 0; ifuncmap < cfuncmap; ifuncmap++ )
        {
            if( FArgumentMatch( argv[0], rgfuncmap[ifuncmap].szCommand ) )
            {
                (rgfuncmap[ifuncmap].function)(
                    pdebugClient,
                    argc - 1, (const CHAR **)( argv + 1 ) );
                goto Return;
            }
        }

        EDBGHelp( pdebugClient, argc, (const CHAR **)argv );
        goto Return;

    Return:
        Pdls()->CompleteWarnings();
        DEBUGGER_LOCAL_STORE::DlsDestroy();
        LocalFree( pv );
    }
    EXCEPT( fDebugMode ? ExceptionFail( _T( "ESE Debugger Extension" ) ) : efaContinueSearch )
    {
        DEBUGGER_LOCAL_STORE::DlsDestroy();
        AssertPREFIX( !"This code path should be impossible (the exception-handler should have terminated the process)." );
    }
    ENDEXCEPT

#endif  //  DEBUGGER_EXTENSION_ESE

#endif  //  DEBUGGER_EXTENSION
    return hr;
}


HRESULT CALLBACK
DebugExtensionInitialize(__out PULONG Version,
             __out PULONG Flags)
{
#ifdef ESENT
    //  these constants come from ntverp.h
    const DWORD dwImageVersionMajor = VER_PRODUCTMAJORVERSION;
    const DWORD dwImageVersionMinor = VER_PRODUCTMINORVERSION;
#else
    const DWORD dwImageVersionMajor = atoi( PRODUCT_MAJOR );
    const DWORD dwImageVersionMinor = atoi( PRODUCT_MINOR );
#endif

    *Version = DEBUG_EXTENSION_VERSION(dwImageVersionMajor, dwImageVersionMinor);
    *Flags = 0;

    return S_OK;
}

void CALLBACK
DebugExtensionUninitialize(void)
{
    // Do nothing.
    return;
}

void CALLBACK
DebugExtensionNotify(
    __in ULONG /*Notify*/,
    __in ULONG64 /*Argument*/
)
{
    // Notify can be one of 
    //    case DEBUG_NOTIFY_SESSION_ACTIVE:
    //    case DEBUG_NOTIFY_SESSION_INACTIVE:
    //    case DEBUG_NOTIFY_SESSION_ACCESSIBLE:
    //    case DEBUG_NOTIFY_SESSION_INACCESSIBLE:

    // Do nothing.
}



#ifdef DEBUG

#ifdef DEBUGGER_EXTENSION_ESE

//  sample callback function
//
//  ================================================================
JET_ERR JET_API YouHaveBadSymbols(
    JET_SESID       sesid,
    JET_DBID        ifmp,
    JET_TABLEID     tableid,
    JET_CBTYP       cbtyp,
    void *          pvArg1,
    void *          pvArg2,
    void *          pvContext,
    ULONG_PTR       ulUnused )
//  ================================================================
{
#if DBG
    //  this line should only compile if the signatures match
    //  Make it debug-only to avoid the PREfast warning of
    //  an unused variable.
    JET_CALLBACK    callback = YouHaveBadSymbols;

    Unused( callback );
#endif

    static BOOL fMessageBox = fTrue;

    const WCHAR * szCbtyp = L"UNKNOWN";
    switch( cbtyp )
    {
        case JET_cbtypNull:
            szCbtyp = L"NULL";
            break;
        case JET_cbtypBeforeInsert:
            szCbtyp = L"BeforeInsert";
            break;
        case JET_cbtypAfterInsert:
            szCbtyp = L"AfterInsert";
            break;
        case JET_cbtypBeforeReplace:
            szCbtyp = L"BeforeReplace";
            break;
        case JET_cbtypAfterReplace:
            szCbtyp = L"AfterReplace";
            break;
        case JET_cbtypBeforeDelete:
            szCbtyp = L"BeforeDelete";
            break;
        case JET_cbtypAfterDelete:
            szCbtyp = L"AfterDelete";
            break;
    }

    WCHAR szMessage[384];
    OSStrCbFormatW( szMessage, sizeof(szMessage),
            L"YouHaveBadSymbols:\n"
            L"\tsesid   		= 0x%p\n"
            L"\tifmp    		= 0x%x\n"
            L"\ttableid 		= 0x%p\n"
            L"\tcbtyp   		= 0x%x (%s)\n"
            L"\tpvArg1  		= 0x%0*p\n"
            L"\tpvArg2 		= 0x%0*p\n"
            L"\tpvContext  	= 0x%0*p\n"
            L"\tulUnused  	= 0x%I64x\n"
            L"\n",
            (VOID*)sesid, ifmp, (VOID*)tableid, cbtyp, szCbtyp,
                (DWORD)sizeof( LONG_PTR )*2, pvArg1,
                (DWORD)sizeof( LONG_PTR )*2, pvArg2,
                (DWORD)sizeof( LONG_PTR )*2, pvContext,
                (ULONGLONG)ulUnused );
    dprintf( "%ws", szMessage );

    JET_ERR err;
    WCHAR szName[JET_cbNameMost+1];

    err = JetGetTableInfoW( sesid, tableid, szName, sizeof( szName ), JET_TblInfoName );
    if( JET_errSuccess != err )
    {
        dprintf( "JetGetTableInfo returns %d\n", err );
    }
    else
    {
        WCHAR szBuf[JET_cbColumnMost + 16];
        OSStrCbFormatW( szBuf, sizeof(szBuf), L"Table \"%s\"\n", szName );
        OSStrCbAppendW( szMessage, sizeof(szMessage), szBuf );
        dprintf( "%ws", szBuf );
    }

    err = JetGetCurrentIndexW( sesid, tableid, szName, sizeof( szName ) );
    if( JET_errSuccess != err )
    {
        dprintf( "JetGetCurrentIndex returns %d\n", err );
    }
    else
    {
        WCHAR szBuf[JET_cbColumnMost + 16];
        OSStrCbFormatW( szBuf, sizeof(szBuf), L"Index \"%s\"\n", szName );
        OSStrCbAppendW( szMessage, sizeof(szMessage), szBuf );
        dprintf( "%ws", szBuf );
    }

    err = JetMove( sesid, tableid, JET_MoveFirst, NO_GRBIT );
    Assert( JET_errSuccess != err );
    err = JetSetCurrentIndex( sesid, tableid, NULL );
    Assert( JET_errSuccess != err );
    JET_TABLEID tableid2;
    err = JetDupCursor( sesid, tableid, &tableid2, NO_GRBIT );
    Assert( JET_errSuccess == err );
    err = JetCloseTable( sesid, tableid );
    Assert( JET_errSuccess != err );
    err = JetCloseTable( sesid, tableid2 );
    Assert( JET_errSuccess == err );

    err = JET_errSuccess;

    switch( cbtyp )
    {
        case JET_cbtypBeforeInsert:
        case JET_cbtypBeforeReplace:
        case JET_cbtypBeforeDelete:
            if( fMessageBox )
            {
                OSStrCbAppendW( szMessage, sizeof(szMessage),
                        L"\n"
                        L"Allow the callback to succeed?" );
                const INT id = UtilMessageBoxW(
                    szMessage,
                    L"YouHaveBadSymbols",
                    MB_SERVICE_NOTIFICATION | MB_SYSTEMMODAL | MB_ICONQUESTION | MB_YESNOCANCEL );
                if( IDNO == id )
                {
                    err = ErrERRCheck( JET_errCallbackFailed );
                }
                else if( IDCANCEL == id )
                {
                    fMessageBox = fFalse;
                }
            }
            break;
    }

    dprintf( "YouHaveBadSymbols returns %d.\n\n", err );
    return err;
}

#endif  //  DEBUGGER_EXTENSION_ESE

#endif  //  DEBUG

} // extern "C"


//  post-terminate edbg subsystem

void OSEdbgPostterm()
{
    //  nop
}

//  pre-init edbg subsystem

BOOL FOSEdbgPreinit()
{
    //  nop

    return fTrue;
}


//  terminate edbg subsystem

void OSEdbgTerm()
{
    //  term OSSYM
    //  nop
}

//  init edbg subsystem

ERR ErrOSEdbgInit()
{
    //  nop

    return JET_errSuccess;
}

