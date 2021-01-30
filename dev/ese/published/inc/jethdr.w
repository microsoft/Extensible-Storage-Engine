// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#if _MSC_VER > 1000
#pragma once
#endif

#if !defined(_JET_INCLUDED)
#define _JET_INCLUDED

#include <winapifamily.h>

#ifdef  __cplusplus
extern "C" {
#endif

#include <specstrings.h>

#define eseVersion 0x5600



#ifndef JET_VERSION
#  ifdef WINVER
#    define JET_VERSION WINVER
#  else
#    define JET_VERSION 0x0A00
#  endif
#endif


#define JET_cbPage  4096

#if defined(_WIN64)
#include <pshpack8.h>
#else
#include <pshpack4.h>
#endif


#pragma warning(push)
#pragma warning(disable: 4201)


#define JET_API     __stdcall
#define JET_NODSAPI __stdcall

#if defined(_WIN64)
    typedef unsigned __int64 JET_API_PTR;
#elif !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300
    typedef __w64 unsigned long JET_API_PTR;
#else
    typedef unsigned long JET_API_PTR;
#endif

typedef _Return_type_success_( return >= 0 ) long JET_ERR;

#if ( JET_VERSION >= 0x0A01 )
typedef unsigned long JET_ENGINEFORMATVERSION;
#endif

typedef JET_API_PTR JET_HANDLE;
typedef JET_API_PTR JET_INSTANCE;
typedef JET_API_PTR JET_SESID;
typedef JET_API_PTR JET_TABLEID;
#if ( JET_VERSION >= 0x0501 )
typedef JET_API_PTR JET_LS;
#endif
#if ( JET_VERSION >= 0x0601 )
typedef JET_API_PTR JET_HISTO;
#endif

typedef unsigned long JET_COLUMNID;

typedef struct tagJET_INDEXID
{
    unsigned long   cbStruct;
    unsigned char   rgbIndexId[sizeof(JET_API_PTR)+sizeof(unsigned long)+sizeof(unsigned long)];
} JET_INDEXID;

typedef unsigned long JET_DBID;
typedef unsigned long JET_OBJTYP;
typedef unsigned long JET_COLTYP;
typedef unsigned long JET_GRBIT;

typedef unsigned long JET_SNP;
typedef unsigned long JET_SNT;
typedef unsigned long JET_SNC;
typedef double JET_DATESERIAL;
typedef unsigned long JET_DLLID;
#if ( JET_VERSION >= 0x0501 )
typedef unsigned long JET_CBTYP;
#endif

typedef JET_ERR (JET_API *JET_PFNSTATUS)(
    _In_ JET_SESID  sesid,
    _In_ JET_SNP    snp,
    _In_ JET_SNT    snt,
    _In_opt_ void * pv );


#if ( JET_VERSION >= 0x0A01 )

typedef JET_ERR (JET_API * JET_PFNINITCALLBACK)(
    _In_ JET_SNP    snp,
    _In_ JET_SNT    snt,
    _In_opt_ void * pv,
    _In_opt_ void * pvContext );

#endif


#if !defined(_NATIVE_WCHAR_T_DEFINED)
typedef unsigned short WCHAR;
#else
typedef wchar_t WCHAR;
#endif

typedef _Null_terminated_ char *  JET_PSTR;
typedef _Null_terminated_ const char *  JET_PCSTR;
typedef _Null_terminated_ WCHAR * JET_PWSTR;
typedef _Null_terminated_ const WCHAR * JET_PCWSTR;

typedef struct
{
    char            *szDatabaseName;
    char            *szNewDatabaseName;
} JET_RSTMAP_A;

typedef struct
{
    WCHAR           *szDatabaseName;
    WCHAR           *szNewDatabaseName;
} JET_RSTMAP_W;

#ifdef JET_UNICODE
#define JET_RSTMAP JET_RSTMAP_W
#else
#define JET_RSTMAP JET_RSTMAP_A
#endif


#if ( JET_VERSION >= 0x0A01 )

typedef struct tagJET_SETDBPARAM
{
    unsigned long                           dbparamid;

    _Field_size_bytes_( cbParam ) void *    pvParam;

    unsigned long                           cbParam;
} JET_SETDBPARAM;

typedef struct
{
    unsigned long                                   cbStruct;
    char                                            *szDatabaseName;
    char                                            *szNewDatabaseName;
    _Field_size_opt_( csetdbparam ) JET_SETDBPARAM  *rgsetdbparam;
    unsigned long                                   csetdbparam;
    JET_GRBIT                                       grbit;
} JET_RSTMAP2_A;

typedef struct
{
    unsigned long                                   cbStruct;
    WCHAR                                           *szDatabaseName;
    WCHAR                                           *szNewDatabaseName;
    _Field_size_opt_( csetdbparam ) JET_SETDBPARAM  *rgsetdbparam;
    unsigned long                                   csetdbparam;
    JET_GRBIT                                       grbit;
} JET_RSTMAP2_W;

#ifdef JET_UNICODE
#define JET_RSTMAP2 JET_RSTMAP2_W
#else
#define JET_RSTMAP2 JET_RSTMAP2_A
#endif

#endif



typedef struct tagCONVERT_A
{
    char                    *szOldDll;
    union
    {
        unsigned long       fFlags;
        struct
        {
            unsigned long   fSchemaChangesOnly:1;
        };
    };
} JET_CONVERT_A;

typedef struct tagCONVERT_W
{
    WCHAR                   *szOldDll;
    union
    {
        unsigned long       fFlags;
        struct
        {
            unsigned long   fSchemaChangesOnly:1;
        };
    };
} JET_CONVERT_W;

#ifdef JET_UNICODE
#define JET_CONVERT JET_CONVERT_W
#else
#define JET_CONVERT JET_CONVERT_A
#endif


typedef enum
{


    opDBUTILConsistency,
    opDBUTILDumpData,
    opDBUTILDumpMetaData,
    opDBUTILDumpPage,
    opDBUTILDumpNode,
    opDBUTILDumpSpace,
    opDBUTILSetHeaderState,
    opDBUTILDumpHeader,
    opDBUTILDumpLogfile,
    opDBUTILDumpLogfileTrackNode,
    opDBUTILDumpCheckpoint,
    opDBUTILEDBDump,
    opDBUTILEDBRepair,
    opDBUTILMunge,
    opDBUTILEDBScrub,
    opDBUTILSLVMove_ObsoleteAndUnused,
    opDBUTILDBConvertRecords_ObsoleteAndUnused,
    opDBUTILDBDefragment,
    opDBUTILDumpExchangeSLVInfo_ObsoleteAndUnused,
    opDBUTILDumpUnicodeFixupTable_ObsoleteAndUnused,
    opDBUTILDumpPageUsage,
    opDBUTILUpdateDBHeaderFromTrailer,
    opDBUTILChecksumLogFromMemory,
    opDBUTILDumpFTLHeader,
    opDBUTILDBTrim,
    opDBUTILDumpFlushMapFile,
    opDBUTILDumpSpaceCategory,
    opDBUTILDumpCachedFileHeader,
    opDBUTILDumpCacheFile,
    opDBUTILDumpRBSHeader,
    opDBUTILDumpRBSPages,
} DBUTIL_OP;

typedef enum
{
    opEDBDumpTables,
    opEDBDumpIndexes,
    opEDBDumpColumns,
    opEDBDumpCallbacks,
    opEDBDumpPage,
} EDBDUMP_OP;

typedef struct tagDBUTIL_A
{
    unsigned long   cbStruct;

    JET_SESID       sesid;
    JET_DBID        dbid;
    JET_TABLEID     tableid;

    DBUTIL_OP       op;
    EDBDUMP_OP      edbdump;
    JET_GRBIT       grbitOptions;


    union
    {
        struct
        {
            char           *szDatabase;
            char           *szSLV_ObsoleteAndUnused;
            char           *szBackup;
            const char     *szTable;
            const char     *szIndex;
            char           *szIntegPrefix;

            long            pgno;
            long            iline;

            long            lGeneration;
            long            isec;
            long            ib;

            long            cRetry;

            void *          pfnCallback;
            void *          pvCallback;
        };

        struct
        {
            char            *szLog;
            char            *szBase;
            void            *pvBuffer;
            long             cbBuffer;
        } checksumlogfrommemory;

        struct
        {
            char               *szDatabase;
            unsigned long       pgnoFirst;
            unsigned long       pgnoLast;
            void               *pfnSpaceCatCallback;
            void               *pvContext;
        } spcatOptions;

        struct
        {
            char               *szDatabase;
            unsigned long       pgnoFirst;
            unsigned long       pgnoLast;
        } rbsOptions;

    };

} JET_DBUTIL_A;

typedef struct tagDBUTIL_W
{
    unsigned long   cbStruct;

    JET_SESID       sesid;
    JET_DBID        dbid;
    JET_TABLEID     tableid;

    DBUTIL_OP       op;
    EDBDUMP_OP      edbdump;
    JET_GRBIT       grbitOptions;


    union
    {
        struct
        {
            WCHAR          *szDatabase;
            WCHAR          *szSLV_ObsoleteAndUnused;
            WCHAR          *szBackup;
            const WCHAR    *szTable;
            const WCHAR    *szIndex;
            WCHAR          *szIntegPrefix;

            long            pgno;
            long            iline;

            long            lGeneration;
            long            isec;
            long            ib;

            long            cRetry;

            void           *pfnCallback;
            void           *pvCallback;
        };

        struct
        {
            WCHAR           *szLog;
            WCHAR           *szBase;
            void            *pvBuffer;
            long             cbBuffer;
        } checksumlogfrommemory;

        struct
        {
            WCHAR              *szDatabase;
            unsigned long       pgnoFirst;
            unsigned long       pgnoLast;
            void               *pfnSpaceCatCallback;
            void               *pvContext;
        } spcatOptions;

        struct
        {
            WCHAR               *szDatabase;
            unsigned long       pgnoFirst;
            unsigned long       pgnoLast;
        } rbsOptions;

    };

} JET_DBUTIL_W;

#ifdef JET_UNICODE
#define JET_DBUTIL JET_DBUTIL_W
#else
#define JET_DBUTIL JET_DBUTIL_A
#endif


#if ( JET_VERSION >= 0x0A01 )
typedef enum
{
    spcatfNone               = 0x00000000,

    spcatfStrictlyLeaf       = 0x00000001,
    spcatfStrictlyInternal   = 0x00000002,
    spcatfRoot               = 0x00000004,
    spcatfSplitBuffer        = 0x00000008,
    spcatfSmallSpace         = 0x00000010,
    spcatfSpaceOE            = 0x00000020,
    spcatfSpaceAE            = 0x00000040,
    spcatfAvailable          = 0x00000080,

    spcatfIndeterminate      = 0x04000000,
    spcatfInconsistent       = 0x08000000,
    spcatfLeaked             = 0x10000000,
    spcatfNotOwned           = 0x20000000,
    spcatfNotOwnedEof        = 0x40000000,
    spcatfUnknown            = 0x80000000,
} SpaceCategoryFlags;

typedef void (JET_API *JET_SPCATCALLBACK)( _In_ const unsigned long pgno, _In_ const unsigned long objid, _In_ const SpaceCategoryFlags spcatf, _In_opt_ void* const pvContext );
#endif

#define JET_bitDBUtilSpaceInfoBasicCatalog          0x00000001
#define JET_bitDBUtilSpaceInfoSpaceTrees                0x00000002
#define JET_bitDBUtilSpaceInfoParentOfLeaf          0x00000004
#define JET_bitDBUtilSpaceInfoFullWalk              0x00000008

#if ( JET_VERSION >= 0x0A01 )
#define JET_bitDBUtilFullCategorization         0x00000001
#endif

#define JET_bitDBUtilOptionAllNodes             0x00000001
#define JET_bitDBUtilOptionKeyStats             0x00000002
#define JET_bitDBUtilOptionPageDump             0x00000004
#define JET_bitDBUtilOptionStats                0x00000008
#define JET_bitDBUtilOptionSuppressConsoleOutput 0x00000010
#define JET_bitDBUtilOptionIgnoreErrors         0x00000020
#define JET_bitDBUtilOptionVerify               0x00000040
#define JET_bitDBUtilOptionReportErrors         0x00000080
#define JET_bitDBUtilOptionDontRepair           0x00000100
#define JET_bitDBUtilOptionRepairAll            0x00000200
#define JET_bitDBUtilOptionRepairIndexes        0x00000400
#define JET_bitDBUtilOptionDontBuildIndexes     0x00000800
#define JET_bitDBUtilOptionSuppressLogo         0x00008000
#define JET_bitDBUtilOptionRepairCheckOnly      0x00010000
#define JET_bitDBUtilOptionDumpLVPageUsage      0x00020000
#if ( JET_VERSION >= 0x0600 )
#define JET_bitDBUtilOptionDumpLogInfoCSV           0x00040000
#define JET_bitDBUtilOptionDumpLogPermitPatching    0x00080000
#endif
#define JET_bitDBUtilOptionSkipMinLogChecksUpdateHeader 0x00100000
#define JET_bitDBUtilOptionDumpVerbose          0x10000000
#define JET_bitDBUtilOptionDumpVerboseLevel1    JET_bitDBUtilOptionDumpVerbose
#define JET_bitDBUtilOptionDumpVerboseLevel2    0x20000000
#define JET_bitDBUtilOptionDumpLogSummary       0x40000000






#define JET_wszConfigStoreReadControl                           L"CsReadControl"
#define JET_bitConfigStoreReadControlInhibitRead                 0x1
#define JET_bitConfigStoreReadControlDisableAll                  0x2
#define JET_bitConfigStoreReadControlDisableSetParamRead         0x4
#define JET_bitConfigStoreReadControlDisableGlobalInitRead       0x8
#define JET_bitConfigStoreReadControlDisableInstInitRead        0x10
#define JET_bitConfigStoreReadControlDisableLiveRead            0x20
#define JET_bitConfigStoreReadControlDefault                     0x0


#define JET_wszConfigStorePopulateControl                       L"CsPopulateControl"
#define JET_bitConfigStorePopulateControlOff                    0x00
#define JET_bitConfigStorePopulateControlOn                     0x01

#define JET_wszConfigStoreRelPathSysParamDefault        L"SysParamDefault"
#define JET_wszConfigStoreRelPathSysParamOverride       L"SysParamOverride"


#if ( JET_VERSION >= 0x0A01 )


#define JET_efvExchange55Rtm                     500
#define JET_efvWindows2000Rtm                    600
#define JET_efvExchange2000Rtm                   1000
#define JET_efvWindowsXpRtm                      1200
#define JET_efvExchange2010Rtm                   5520
#define JET_efvExchange2010Sp1                   5920
#define JET_efvExchange2010Sp2                   6080
#define JET_efvScrubLastNodeOnEmptyPage                     6960
#define JET_efvExchange2013Rtm                   7020
#define JET_efvExtHdrRootFieldAutoIncStorageStagedToDebug   7060
#define JET_efvLogExtendDbLrAfterCreateDbLr                 7940
#define JET_efvRollbackInsertSpaceStagedToTest              7960
#define JET_efvWindows10Rtm                     (8020)
#define JET_efvQfeMsysLocalesGuidRefCountFixup              8023
#define JET_efvPersistedLostFlushMapStagedToDebug           8220
#define JET_efvTriStatePageFlushTypeStagedToProd            8280
#define JET_efvLogSplitMergeCompressionReleased             8400
#define JET_efvTriStatePageFlushTypeReleased                8420
#define JET_efvIrsDbidBasedMacroInfo2                       8440
#define JET_efvLostFlushMapCrashResiliency                  8460
#define JET_efvExchange2016Rtm                   8520
#define JET_efvMsysLocalesGuidRefCountFixup                 8620
#define JET_efvWindows10v2Rtm                    8620
#define JET_efvEncryptedColumns                             8720
#define JET_efvLoggingDeleteOfTableFcbs                     8820
#define JET_efvEmptyIdxUpgradesUpdateMsysLocales            8860
#define JET_efvSupportNlsInvariantLocale                    8880
#define JET_efvExchange2016Cu1Rtm                8920
#define JET_efvWindows19H1Rtm                    8920
#define JET_efvSetDbVersion                                 8940
#define JET_efvExtHdrRootFieldAutoIncStorageReleased        8960
#define JET_efvXpress9Compression                           8980
#define JET_efvUppercaseTextNormalization                   9000
#define JET_efvDbscanHeaderHighestPageSeen                  9020
#define JET_efvEscrow64                                     9040
#define JET_efvSynchronousLVCleanup                         9060
#define JET_efvLid64                                        9080
#define JET_efvShrinkEof                                    9100
#define JET_efvLogNewPage                                   9120
#define JET_efvRootPageMove                                 9140
#define JET_efvScanCheck2                                   9160
#define JET_efvLgposLastResize                              9180
#define JET_efvShelvedPages                                 9200
#define JET_efvShelvedPagesRevert                           9220
#define JET_efvShelvedPages2                                9240
#define JET_efvLogtimeGenMaxRequired                        9260
#define JET_efvVariableDbHdrSignatureSnapshot               9280
#define JET_efvLowerMinReqLogGenOnRedo                      9300
#define JET_efvDbTimeShrink                                 9320
#define JET_efvXpress10Compression                          9340
#define JET_efvRevertSnapshot                               9360
#define JET_efvApplyRevertSnapshot                          9380

#define JET_efvUseEngineDefault             (0x40000001)
#define JET_efvUsePersistedFormat           (0x40000002)
#define JET_efvAllowHigherPersistedFormat   (0x41000000)

#endif


#if ( JET_VERSION >= 0x0601 )
#define JET_bitDatabaseScanBatchStart           0x00000010
#define JET_bitDatabaseScanBatchStop            0x00000020
#define JET_bitDatabaseScanZeroPages            0x00000040
#define JET_bitDatabaseScanBatchStartContinuous 0x00000080
#endif


#define JET_bitDefragmentBatchStart             0x00000001
#define JET_bitDefragmentBatchStop              0x00000002

#if ( JET_VERSION >= 0x0501 )
#define JET_bitDefragmentAvailSpaceTreesOnly    0x00000040
#endif
#if ( JET_VERSION >= 0x0601 )
#define JET_bitDefragmentNoPartialMerges        0x00000080

#define JET_bitDefragmentBTree                  0x00000100
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitDefragmentBTreeBatch             0x00000200
#endif



#endif

#if ( JET_VERSION >= 0x0501 )


#define JET_cbtypNull                           0x00000000
#define JET_cbtypFinalize                       0x00000001
#define JET_cbtypBeforeInsert                   0x00000002
#define JET_cbtypAfterInsert                    0x00000004
#define JET_cbtypBeforeReplace                  0x00000008
#define JET_cbtypAfterReplace                   0x00000010
#define JET_cbtypBeforeDelete                   0x00000020
#define JET_cbtypAfterDelete                    0x00000040
#define JET_cbtypUserDefinedDefaultValue        0x00000080
#define JET_cbtypOnlineDefragCompleted          0x00000100
#define JET_cbtypFreeCursorLS                   0x00000200
#define JET_cbtypFreeTableLS                    0x00000400
#if ( JET_VERSION >= 0x0601 )
#define JET_bitOld2Start                        0x00000001
#define JET_bitOld2Suspend                      0x00000002
#define JET_bitOld2Resume                       0x00000004
#define JET_bitOld2End                          0x00000008

#define JET_cbtypScanProgress                   0x00004000
#define JET_cbtypScanCompleted                  0x00008000
#endif
#define JET_cbtypDTCQueryPreparedTransaction    0x00001000
#define JET_cbtypOnlineDefragProgress           0x00002000
#define JET_cbtypOld2Action          0x00004000




typedef JET_ERR (JET_API *JET_CALLBACK)(
    _In_ JET_SESID      sesid,
    _In_ JET_DBID       dbid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_CBTYP      cbtyp,
    _Inout_opt_ void *  pvArg1,
    _Inout_opt_ void *  pvArg2,
    _In_opt_ void *     pvContext,
    _In_ JET_API_PTR    ulUnused );
#endif

#define JET_cbCallbackUserDataMost              1024

#define JET_cbCallbackDataAllMost               4096


typedef JET_ERR (JET_API *JET_ABORTRETRYFAILCALLBACK_A)(
    _In_ char *         szFile,
    _In_ unsigned long  Offset,
    _In_ unsigned long  OffsetHigh,
    _In_ unsigned long  Length,
    _In_ JET_ERR        err );

typedef JET_ERR (JET_API *JET_ABORTRETRYFAILCALLBACK_W)(
    _In_ WCHAR *        szFile,
    _In_ unsigned long  Offset,
    _In_ unsigned long  OffsetHigh,
    _In_ unsigned long  Length,
    _In_ JET_ERR        err );

#ifdef JET_UNICODE
#define JET_ABORTRETRYFAILCALLBACK JET_ABORTRETRYFAILCALLBACK_W
#else
#define JET_ABORTRETRYFAILCALLBACK JET_ABORTRETRYFAILCALLBACK_A
#endif


#if ( JET_VERSION >= 0x0600 )
typedef enum
{
    JET_tracetagNull,
    JET_tracetagInformation,
    JET_tracetagErrors,
    JET_tracetagAsserts,
    JET_tracetagAPI,
    JET_tracetagInitTerm,
    JET_tracetagBufferManager,
    JET_tracetagBufferManagerHashedLatches,
    JET_tracetagIO,
    JET_tracetagMemory,
    JET_tracetagVersionStore,
    JET_tracetagVersionStoreOOM,
    JET_tracetagVersionCleanup,
    JET_tracetagCatalog,
    JET_tracetagDDLRead,
    JET_tracetagDDLWrite,
    JET_tracetagDMLRead,
    JET_tracetagDMLWrite,
    JET_tracetagDMLConflicts,
    JET_tracetagInstances,
    JET_tracetagDatabases,
    JET_tracetagSessions,
    JET_tracetagCursors,
    JET_tracetagCursorNavigation,
    JET_tracetagCursorPageRefs,
    JET_tracetagBtree,
    JET_tracetagSpace,
    JET_tracetagFCBs,
    JET_tracetagTransactions,
    JET_tracetagLogging,
    JET_tracetagRecovery,
    JET_tracetagBackup,
    JET_tracetagRestore,
    JET_tracetagOLD,
    JET_tracetagEventlog,
#if ( JET_VERSION >= 0x0601 )
    JET_tracetagBufferManagerMaintTasks,
    JET_tracetagSpaceManagement,
    JET_tracetagSpaceInternal,
    JET_tracetagIOQueue,
    JET_tracetagDiskVolumeManagement,
#if ( JET_VERSION >= 0x0602 )
    JET_tracetagCallbacks,
    JET_tracetagIOProblems,
    JET_tracetagUpgrade,
    JET_tracetagRecoveryValidation,
    JET_tracetagBufferManagerBufferCacheState,
    JET_tracetagBufferManagerBufferDirtyState,
    JET_tracetagTimerQueue,
    JET_tracetagSortPerf,
#if ( JET_VERSION >= 0x0A00 )
    JET_tracetagOLDRegistration,
    JET_tracetagOLDWork,
    JET_tracetagSysInitTerm,
#if ( JET_VERSION >= 0x0A01 )
    JET_tracetagVersionAndStagingChecks,
    JET_tracetagFile,
    JET_tracetagFlushFileBuffers,
    JET_tracetagCheckpointUpdate,
    JET_tracetagDiagnostics,
    JET_tracetagBlockCache,
    JET_tracetagRBS,
    JET_tracetagRBSCleaner,

#endif
#endif
#endif
#endif
    JET_tracetagMax,
} JET_TRACETAG;


typedef void (JET_API *JET_PFNTRACEEMIT)(
    _In_ const JET_TRACETAG tag,
    _In_ JET_PCSTR          szPrefix,
    _In_ JET_PCSTR          szTrace,
    _In_ const JET_API_PTR  ul );
typedef void (JET_API *JET_PFNTRACEREGISTER)(
    _In_ const JET_TRACETAG tag,
    _In_ JET_PCSTR          szDesc,
    _Out_ JET_API_PTR *     pul );


typedef enum
{
    JET_traceopNull,
    JET_traceopSetGlobal,
    JET_traceopSetTag,
    JET_traceopSetAllTags,
    JET_traceopSetMessagePrefix,
    JET_traceopRegisterTag,
    JET_traceopRegisterAllTags,
    JET_traceopSetEmitCallback,
    JET_traceopSetThreadidFilter,
    JET_traceopSetDbidFilter,
    JET_traceopMax
} JET_TRACEOP;
#endif




#define JET_bitCIMCommitted                     0x00000001
#define JET_bitCIMDirty                         0x00000002
#define JET_bitAggregateTransaction             0x00000008

#if ( JET_VERSION >= 0x0600 )
typedef struct JET_SESSIONINFO
{
    unsigned long   ulTrxBegin0;
    unsigned long   ulTrxLevel;
    unsigned long   ulProcid;
    unsigned long   ulFlags;
    JET_API_PTR     ulTrxContext;
} JET_SESSIONINFO;
#endif



typedef struct
{
    unsigned long   cbStruct;
    unsigned long   cunitDone;
    unsigned long   cunitTotal;
} JET_SNPROG;

typedef struct
{
    unsigned long           cbStruct;

    unsigned long           cbFilesizeLow;
    unsigned long           cbFilesizeHigh;

    unsigned long           cbFreeSpaceRequiredLow;
    unsigned long           cbFreeSpaceRequiredHigh;

    unsigned long           csecToUpgrade;

    union
    {
        unsigned long       ulFlags;
        struct
        {
            unsigned long   fUpgradable:1;
            unsigned long   fAlreadyUpgraded:1;
        };
    };
} JET_DBINFOUPGRADE;

typedef struct
{
    unsigned long       cbStruct;
    JET_OBJTYP          objtyp;
    JET_DATESERIAL      dtCreate;
    JET_DATESERIAL      dtUpdate;
    JET_GRBIT           grbit;
    unsigned long       flags;
    unsigned long       cRecord;
    unsigned long       cPage;
} JET_OBJECTINFO;



#define JET_bitTableInfoUpdatable   0x00000001
#define JET_bitTableInfoBookmark    0x00000002
#define JET_bitTableInfoRollback    0x00000004



#define JET_bitObjectSystem         0x80000000
#define JET_bitObjectTableFixedDDL  0x40000000
#define JET_bitObjectTableTemplate  0x20000000
#define JET_bitObjectTableDerived   0x10000000
#define JET_bitObjectSystemDynamic  (JET_bitObjectSystem|0x08000000)
#if ( JET_VERSION >= 0x0501 )
#define JET_bitObjectTableNoFixedVarColumnsInDerivedTables  0x04000000
#endif

typedef struct
{
    unsigned long   cbStruct;
    JET_TABLEID     tableid;
    unsigned long   cRecord;
    JET_COLUMNID    columnidcontainername;
    JET_COLUMNID    columnidobjectname;
    JET_COLUMNID    columnidobjtyp;
    JET_COLUMNID    columniddtCreate;
    JET_COLUMNID    columniddtUpdate;
    JET_COLUMNID    columnidgrbit;
    JET_COLUMNID    columnidflags;
    JET_COLUMNID    columnidcRecord;
    JET_COLUMNID    columnidcPage;
} JET_OBJECTLIST;

#define cObjectInfoCols 9

typedef struct
{
    unsigned long   cbStruct;
    JET_TABLEID     tableid;
    unsigned long   cRecord;
    JET_COLUMNID    columnidPresentationOrder;
    JET_COLUMNID    columnidcolumnname;
    JET_COLUMNID    columnidcolumnid;
    JET_COLUMNID    columnidcoltyp;
    JET_COLUMNID    columnidCountry;
    JET_COLUMNID    columnidLangid;
    JET_COLUMNID    columnidCp;
    JET_COLUMNID    columnidCollate;
    JET_COLUMNID    columnidcbMax;
    JET_COLUMNID    columnidgrbit;
    JET_COLUMNID    columnidDefault;
    JET_COLUMNID    columnidBaseTableName;
    JET_COLUMNID    columnidBaseColumnName;
    JET_COLUMNID    columnidDefinitionName;
} JET_COLUMNLIST;

#define cColumnInfoCols 14

typedef struct
{
    unsigned long   cbStruct;
    JET_COLUMNID    columnid;
    JET_COLTYP      coltyp;
    unsigned short  wCountry;
    unsigned short  langid;
    unsigned short  cp;
    unsigned short  wCollate;
    unsigned long   cbMax;
    JET_GRBIT       grbit;
} JET_COLUMNDEF;


typedef struct
{
    unsigned long   cbStruct;
    JET_COLUMNID    columnid;
    JET_COLTYP      coltyp;
    unsigned short  wCountry;
    unsigned short  langid;
    unsigned short  cp;
    unsigned short  wFiller;
    unsigned long   cbMax;
    JET_GRBIT       grbit;
    char            szBaseTableName[256];
    char            szBaseColumnName[256];
} JET_COLUMNBASE_A;


typedef struct
{
    unsigned long   cbStruct;
    JET_COLUMNID    columnid;
    JET_COLTYP      coltyp;
    unsigned short  wCountry;
    unsigned short  langid;
    unsigned short  cp;
    unsigned short  wFiller;
    unsigned long   cbMax;
    JET_GRBIT       grbit;
    WCHAR           szBaseTableName[256];
    WCHAR           szBaseColumnName[256];
} JET_COLUMNBASE_W;


#ifdef JET_UNICODE
#define JET_COLUMNBASE JET_COLUMNBASE_W
#else
#define JET_COLUMNBASE JET_COLUMNBASE_A
#endif


typedef struct
{
    unsigned long   cbStruct;
    JET_TABLEID     tableid;
    unsigned long   cRecord;
    JET_COLUMNID    columnidindexname;
    JET_COLUMNID    columnidgrbitIndex;
    JET_COLUMNID    columnidcKey;
    JET_COLUMNID    columnidcEntry;
    JET_COLUMNID    columnidcPage;
    JET_COLUMNID    columnidcColumn;
    JET_COLUMNID    columnidiColumn;
    JET_COLUMNID    columnidcolumnid;
    JET_COLUMNID    columnidcoltyp;
    JET_COLUMNID    columnidCountry;
    JET_COLUMNID    columnidLangid;
    JET_COLUMNID    columnidCp;
    JET_COLUMNID    columnidCollate;
    JET_COLUMNID    columnidgrbitColumn;
    JET_COLUMNID    columnidcolumnname;
    JET_COLUMNID    columnidLCMapFlags;
} JET_INDEXLIST;


#define cIndexInfoCols 15

typedef struct tag_JET_COLUMNCREATE_A
{
    unsigned long   cbStruct;
    char            *szColumnName;
    JET_COLTYP      coltyp;
    unsigned long   cbMax;
    JET_GRBIT       grbit;
    void            *pvDefault;
    unsigned long   cbDefault;
    unsigned long   cp;
    JET_COLUMNID    columnid;
    JET_ERR         err;
} JET_COLUMNCREATE_A;

typedef struct tag_JET_COLUMNCREATE_W
{
    unsigned long   cbStruct;
    WCHAR           *szColumnName;
    JET_COLTYP      coltyp;
    unsigned long   cbMax;
    JET_GRBIT       grbit;
    void            *pvDefault;
    unsigned long   cbDefault;
    unsigned long   cp;
    JET_COLUMNID    columnid;
    JET_ERR         err;
} JET_COLUMNCREATE_W;

#ifdef JET_UNICODE
#define JET_COLUMNCREATE JET_COLUMNCREATE_W
#else
#define JET_COLUMNCREATE JET_COLUMNCREATE_A
#endif

#if ( JET_VERSION >= 0x0501 )

typedef struct tag_JET_USERDEFINEDDEFAULT_A
{
    char * szCallback;
    unsigned char * pbUserData;
    unsigned long cbUserData;
    char * szDependantColumns;
} JET_USERDEFINEDDEFAULT_A;

typedef struct tag_JET_USERDEFINEDDEFAULT_W
{
    WCHAR * szCallback;
    unsigned char * pbUserData;
    unsigned long cbUserData;
    WCHAR * szDependantColumns;
} JET_USERDEFINEDDEFAULT_W;

#ifdef JET_UNICODE
#define JET_USERDEFINEDDEFAULT JET_USERDEFINEDDEFAULT_W
#else
#define JET_USERDEFINEDDEFAULT JET_USERDEFINEDDEFAULT_A
#endif

#endif

typedef struct tagJET_CONDITIONALCOLUMN_A
{
    unsigned long   cbStruct;
    char            *szColumnName;
    JET_GRBIT       grbit;
} JET_CONDITIONALCOLUMN_A;

typedef struct tagJET_CONDITIONALCOLUMN_W
{
    unsigned long   cbStruct;
    WCHAR           *szColumnName;
    JET_GRBIT       grbit;
} JET_CONDITIONALCOLUMN_W;

#ifdef JET_UNICODE
#define JET_CONDITIONALCOLUMN JET_CONDITIONALCOLUMN_W
#else
#define JET_CONDITIONALCOLUMN JET_CONDITIONALCOLUMN_A
#endif

typedef struct tagJET_UNICODEINDEX
{
    unsigned long   lcid;
    unsigned long   dwMapFlags;
} JET_UNICODEINDEX;

#if ( JET_VERSION >= 0x0602 )
typedef struct tagJET_UNICODEINDEX2
{
    _Field_z_ WCHAR         *szLocaleName;
    unsigned long   dwMapFlags;
} JET_UNICODEINDEX2;
#endif

#if ( JET_VERSION >= 0x0502 )
typedef struct tagJET_TUPLELIMITS
{
    unsigned long   chLengthMin;
    unsigned long   chLengthMax;
    unsigned long   chToIndexMax;
#if ( JET_VERSION >= 0x0600 )
    unsigned long   cchIncrement;
    unsigned long   ichStart;
#endif
} JET_TUPLELIMITS;
#endif

#if ( JET_VERSION >= 0x0601 )
typedef struct tagJET_SPACEHINTS
{
    unsigned long       cbStruct;
    unsigned long       ulInitialDensity;
    unsigned long       cbInitial;

    JET_GRBIT           grbit;
    unsigned long       ulMaintDensity;
    unsigned long       ulGrowth;
    unsigned long       cbMinExtent;
    unsigned long       cbMaxExtent;
} JET_SPACEHINTS;
#endif


typedef struct tagJET_INDEXCREATEOLD_A
{
    unsigned long           cbStruct;
    char                    *szIndexName;
    char                    *szKey;
    unsigned long           cbKey;
    JET_GRBIT               grbit;
    unsigned long           ulDensity;

    union
    {
        unsigned long       lcid;
        JET_UNICODEINDEX    *pidxunicode;
    };

    union
    {
        unsigned long       cbVarSegMac;
#if ( JET_VERSION >= 0x0502 )
        JET_TUPLELIMITS     *ptuplelimits;
#endif
    };

    JET_CONDITIONALCOLUMN_A *rgconditionalcolumn;
    unsigned long           cConditionalColumn;
    JET_ERR                 err;
} JET_INDEXCREATEOLD_A;

typedef struct tagJET_INDEXCREATEOLD_W
{
    unsigned long           cbStruct;
    WCHAR                   *szIndexName;
    WCHAR                   *szKey;
    unsigned long           cbKey;
    JET_GRBIT               grbit;
    unsigned long           ulDensity;

    union
    {
        unsigned long       lcid;
        JET_UNICODEINDEX    *pidxunicode;
    };

    union
    {
        unsigned long       cbVarSegMac;
#if ( JET_VERSION >= 0x0502 )
        JET_TUPLELIMITS     *ptuplelimits;
#endif
    };

    JET_CONDITIONALCOLUMN_W *rgconditionalcolumn;
    unsigned long           cConditionalColumn;
    JET_ERR                 err;
} JET_INDEXCREATEOLD_W;

#ifdef JET_UNICODE
#define JET_INDEXCREATEOLD JET_INDEXCREATEOLD_W
#else
#define JET_INDEXCREATEOLD JET_INDEXCREATEOLD_A
#endif


typedef struct tagJET_INDEXCREATE_A
{
    unsigned long           cbStruct;
    char                    *szIndexName;
    char                    *szKey;
    unsigned long           cbKey;
    JET_GRBIT               grbit;
    unsigned long           ulDensity;

    union
    {
        unsigned long       lcid;
        JET_UNICODEINDEX    *pidxunicode;
    };

    union
    {
        unsigned long       cbVarSegMac;
#if ( JET_VERSION >= 0x0502 )
        JET_TUPLELIMITS     *ptuplelimits;
#endif
    };

    JET_CONDITIONALCOLUMN_A *rgconditionalcolumn;
    unsigned long           cConditionalColumn;
    JET_ERR                 err;
#if ( JET_VERSION >= 0x0600 )
    unsigned long           cbKeyMost;
#endif
} JET_INDEXCREATE_A;

typedef struct tagJET_INDEXCREATE_W
{
    unsigned long           cbStruct;
    WCHAR                   *szIndexName;
    WCHAR                   *szKey;
    unsigned long           cbKey;
    JET_GRBIT               grbit;
    unsigned long           ulDensity;

    union
    {
        unsigned long       lcid;
        JET_UNICODEINDEX    *pidxunicode;
    };

    union
    {
        unsigned long       cbVarSegMac;
#if ( JET_VERSION >= 0x0502 )
        JET_TUPLELIMITS     *ptuplelimits;
#endif
    };

    JET_CONDITIONALCOLUMN_W *rgconditionalcolumn;
    unsigned long           cConditionalColumn;
    JET_ERR                 err;
#if ( JET_VERSION >= 0x0600 )
    unsigned long           cbKeyMost;
#endif
} JET_INDEXCREATE_W;

#ifdef JET_UNICODE
#define JET_INDEXCREATE JET_INDEXCREATE_W
#else
#define JET_INDEXCREATE JET_INDEXCREATE_A
#endif

#if ( JET_VERSION >= 0x0601 )

typedef struct tagJET_INDEXCREATE2_A
{
    unsigned long           cbStruct;
    char                    *szIndexName;
    char                    *szKey;
    unsigned long           cbKey;
    JET_GRBIT               grbit;
    unsigned long           ulDensity;

    union
    {
        unsigned long       lcid;
        JET_UNICODEINDEX    *pidxunicode;
    };

    union
    {
        unsigned long       cbVarSegMac;
        JET_TUPLELIMITS     *ptuplelimits;
    };

    JET_CONDITIONALCOLUMN_A *rgconditionalcolumn;
    unsigned long           cConditionalColumn;
    JET_ERR                 err;
    unsigned long           cbKeyMost;
    JET_SPACEHINTS *        pSpacehints;
} JET_INDEXCREATE2_A;

typedef struct tagJET_INDEXCREATE2_W
{
    unsigned long           cbStruct;
    WCHAR                   *szIndexName;
    WCHAR                   *szKey;
    unsigned long           cbKey;
    JET_GRBIT               grbit;
    unsigned long           ulDensity;

    union
    {
        unsigned long       lcid;
        JET_UNICODEINDEX    *pidxunicode;
    };

    union
    {
        unsigned long       cbVarSegMac;
        JET_TUPLELIMITS     *ptuplelimits;
    };

    JET_CONDITIONALCOLUMN_W *rgconditionalcolumn;
    unsigned long           cConditionalColumn;
    JET_ERR                 err;
    unsigned long           cbKeyMost;
    JET_SPACEHINTS *        pSpacehints;
} JET_INDEXCREATE2_W;

#ifdef JET_UNICODE
#define JET_INDEXCREATE2 JET_INDEXCREATE2_W
#else
#define JET_INDEXCREATE2 JET_INDEXCREATE2_A
#endif
#endif

#if ( JET_VERSION >= 0x0602 )

typedef struct tagJET_INDEXCREATE3_A
{
    unsigned long           cbStruct;
    char                    *szIndexName;
    char                    *szKey;
    unsigned long           cbKey;
    JET_GRBIT               grbit;
    unsigned long           ulDensity;
    JET_UNICODEINDEX2       *pidxunicode;

    union
    {
        unsigned long       cbVarSegMac;
        JET_TUPLELIMITS     *ptuplelimits;
    };

    JET_CONDITIONALCOLUMN_A *rgconditionalcolumn;
    unsigned long           cConditionalColumn;
    JET_ERR                 err;
    unsigned long           cbKeyMost;
    JET_SPACEHINTS *        pSpacehints;
} JET_INDEXCREATE3_A;

typedef struct tagJET_INDEXCREATE3_W
{
    unsigned long           cbStruct;
    WCHAR                   *szIndexName;
    WCHAR                   *szKey;
    unsigned long           cbKey;
    JET_GRBIT               grbit;
    unsigned long           ulDensity;
    JET_UNICODEINDEX2       *pidxunicode;

    union
    {
        unsigned long       cbVarSegMac;
        JET_TUPLELIMITS     *ptuplelimits;
    };

    JET_CONDITIONALCOLUMN_W *rgconditionalcolumn;
    unsigned long           cConditionalColumn;
    JET_ERR                 err;
    unsigned long           cbKeyMost;
    JET_SPACEHINTS *        pSpacehints;
} JET_INDEXCREATE3_W;

#ifdef JET_UNICODE
#define JET_INDEXCREATE3 JET_INDEXCREATE3_W
#else
#define JET_INDEXCREATE3 JET_INDEXCREATE3_A
#endif
#endif


typedef struct tagJET_TABLECREATE_A
{
    unsigned long       cbStruct;
    char                *szTableName;
    char                *szTemplateTableName;
    unsigned long       ulPages;
    unsigned long       ulDensity;
    JET_COLUMNCREATE_A  *rgcolumncreate;
    unsigned long       cColumns;
    JET_INDEXCREATE_A       *rgindexcreate;
    unsigned long       cIndexes;
    JET_GRBIT           grbit;
    JET_TABLEID         tableid;
    unsigned long       cCreated;
} JET_TABLECREATE_A;

typedef struct tagJET_TABLECREATE_W
{
    unsigned long       cbStruct;
    WCHAR               *szTableName;
    WCHAR               *szTemplateTableName;
    unsigned long       ulPages;
    unsigned long       ulDensity;
    JET_COLUMNCREATE_W  *rgcolumncreate;
    unsigned long       cColumns;
    JET_INDEXCREATE_W       *rgindexcreate;
    unsigned long       cIndexes;
    JET_GRBIT           grbit;
    JET_TABLEID         tableid;
    unsigned long       cCreated;
} JET_TABLECREATE_W;

#ifdef JET_UNICODE
#define JET_TABLECREATE JET_TABLECREATE_W
#else
#define JET_TABLECREATE JET_TABLECREATE_A
#endif

#if ( JET_VERSION >= 0x0501 )
typedef struct tagJET_TABLECREATE2_A
{
    unsigned long       cbStruct;
    char                *szTableName;
    char                *szTemplateTableName;
    unsigned long       ulPages;
    unsigned long       ulDensity;
    JET_COLUMNCREATE_A  *rgcolumncreate;
    unsigned long       cColumns;
    JET_INDEXCREATE_A   *rgindexcreate;
    unsigned long       cIndexes;
    char                *szCallback;
    JET_CBTYP           cbtyp;
    JET_GRBIT           grbit;
    JET_TABLEID         tableid;
    unsigned long       cCreated;
} JET_TABLECREATE2_A;

typedef struct tagJET_TABLECREATE2_W
{
    unsigned long       cbStruct;
    WCHAR               *szTableName;
    WCHAR               *szTemplateTableName;
    unsigned long       ulPages;
    unsigned long       ulDensity;
    JET_COLUMNCREATE_W  *rgcolumncreate;
    unsigned long       cColumns;
    JET_INDEXCREATE_W   *rgindexcreate;
    unsigned long       cIndexes;
    WCHAR               *szCallback;
    JET_CBTYP           cbtyp;
    JET_GRBIT           grbit;
    JET_TABLEID         tableid;
    unsigned long       cCreated;
} JET_TABLECREATE2_W;

#ifdef JET_UNICODE
#define JET_TABLECREATE2 JET_TABLECREATE2_W
#else
#define JET_TABLECREATE2 JET_TABLECREATE2_A
#endif

#endif


#if ( JET_VERSION >= 0x0601 )
typedef struct tagJET_TABLECREATE3_A
{
    unsigned long       cbStruct;
    char                *szTableName;
    char                *szTemplateTableName;
    unsigned long       ulPages;
    unsigned long       ulDensity;
    JET_COLUMNCREATE_A  *rgcolumncreate;
    unsigned long       cColumns;
    JET_INDEXCREATE2_A  *rgindexcreate;
    unsigned long       cIndexes;
    char                *szCallback;
    JET_CBTYP           cbtyp;
    JET_GRBIT           grbit;
    JET_SPACEHINTS *    pSeqSpacehints;
    JET_SPACEHINTS *    pLVSpacehints;
    unsigned long       cbSeparateLV;

    JET_TABLEID         tableid;
    unsigned long       cCreated;
} JET_TABLECREATE3_A;

typedef struct tagJET_TABLECREATE3_W
{
    unsigned long       cbStruct;
    WCHAR               *szTableName;
    WCHAR               *szTemplateTableName;
    unsigned long       ulPages;
    unsigned long       ulDensity;
    JET_COLUMNCREATE_W  *rgcolumncreate;
    unsigned long       cColumns;
    JET_INDEXCREATE2_W  *rgindexcreate;
    unsigned long       cIndexes;
    WCHAR               *szCallback;
    JET_CBTYP           cbtyp;
    JET_GRBIT           grbit;
    JET_SPACEHINTS *    pSeqSpacehints;
    JET_SPACEHINTS *    pLVSpacehints;
    unsigned long       cbSeparateLV;
    JET_TABLEID         tableid;
    unsigned long       cCreated;
} JET_TABLECREATE3_W;

#ifdef JET_UNICODE
#define JET_TABLECREATE3 JET_TABLECREATE3_W
#else
#define JET_TABLECREATE3 JET_TABLECREATE3_A
#endif

#endif

#if ( JET_VERSION >= 0x0602 )
typedef struct tagJET_TABLECREATE4_A
{
    unsigned long       cbStruct;
    char                *szTableName;
    char                *szTemplateTableName;
    unsigned long       ulPages;
    unsigned long       ulDensity;
    JET_COLUMNCREATE_A  *rgcolumncreate;
    unsigned long       cColumns;
    JET_INDEXCREATE3_A  *rgindexcreate;
    unsigned long       cIndexes;
    char                *szCallback;
    JET_CBTYP           cbtyp;
    JET_GRBIT           grbit;
    JET_SPACEHINTS *    pSeqSpacehints;
    JET_SPACEHINTS *    pLVSpacehints;
    unsigned long       cbSeparateLV;

    JET_TABLEID         tableid;
    unsigned long       cCreated;
} JET_TABLECREATE4_A;

typedef struct tagJET_TABLECREATE4_W
{
    unsigned long       cbStruct;
    WCHAR               *szTableName;
    WCHAR               *szTemplateTableName;
    unsigned long       ulPages;
    unsigned long       ulDensity;
    JET_COLUMNCREATE_W  *rgcolumncreate;
    unsigned long       cColumns;
    JET_INDEXCREATE3_W  *rgindexcreate;
    unsigned long       cIndexes;
    WCHAR               *szCallback;
    JET_CBTYP           cbtyp;
    JET_GRBIT           grbit;
    JET_SPACEHINTS *    pSeqSpacehints;
    JET_SPACEHINTS *    pLVSpacehints;
    unsigned long       cbSeparateLV;

    JET_TABLEID         tableid;
    unsigned long       cCreated;
} JET_TABLECREATE4_W;

#ifdef JET_UNICODE
#define JET_TABLECREATE4 JET_TABLECREATE4_W
#else
#define JET_TABLECREATE4 JET_TABLECREATE4_A
#endif

#endif

#if ( JET_VERSION >= 0x0A01 )
typedef struct tagJET_TABLECREATE5_A
{
    unsigned long       cbStruct;
    char                *szTableName;
    char                *szTemplateTableName;
    unsigned long       ulPages;
    unsigned long       ulDensity;
    JET_COLUMNCREATE_A  *rgcolumncreate;
    unsigned long       cColumns;
    JET_INDEXCREATE3_A  *rgindexcreate;
    unsigned long       cIndexes;
    char                *szCallback;
    JET_CBTYP           cbtyp;
    JET_GRBIT           grbit;
    JET_SPACEHINTS *    pSeqSpacehints;
    JET_SPACEHINTS *    pLVSpacehints;
    unsigned long       cbSeparateLV;
    unsigned long       cbLVChunkMax;

    JET_TABLEID         tableid;
    unsigned long       cCreated;
} JET_TABLECREATE5_A;

typedef struct tagJET_TABLECREATE5_W
{
    unsigned long       cbStruct;
    WCHAR               *szTableName;
    WCHAR               *szTemplateTableName;
    unsigned long       ulPages;
    unsigned long       ulDensity;
    JET_COLUMNCREATE_W  *rgcolumncreate;
    unsigned long       cColumns;
    JET_INDEXCREATE3_W  *rgindexcreate;
    unsigned long       cIndexes;
    WCHAR               *szCallback;
    JET_CBTYP           cbtyp;
    JET_GRBIT           grbit;
    JET_SPACEHINTS *    pSeqSpacehints;
    JET_SPACEHINTS *    pLVSpacehints;
    unsigned long       cbSeparateLV;
    unsigned long       cbLVChunkMax;

    JET_TABLEID         tableid;
    unsigned long       cCreated;
} JET_TABLECREATE5_W;

#ifdef JET_UNICODE
#define JET_TABLECREATE5 JET_TABLECREATE5_W
#else
#define JET_TABLECREATE5 JET_TABLECREATE5_A
#endif

#endif

#if ( JET_VERSION >= 0x0600 )
typedef struct tagJET_OPENTEMPORARYTABLE
{
    unsigned long       cbStruct;
    const JET_COLUMNDEF *prgcolumndef;
    unsigned long       ccolumn;
    JET_UNICODEINDEX    *pidxunicode;
    JET_GRBIT           grbit;
    JET_COLUMNID        *prgcolumnid;
    unsigned long       cbKeyMost;
    unsigned long       cbVarSegMac;
    JET_TABLEID         tableid;
} JET_OPENTEMPORARYTABLE;
#endif

#if ( JET_VERSION >= 0x0602 )
typedef struct tagJET_OPENTEMPORARYTABLE2
{
    unsigned long       cbStruct;
    const JET_COLUMNDEF *prgcolumndef;
    unsigned long       ccolumn;
    JET_UNICODEINDEX2   *pidxunicode;
    JET_GRBIT           grbit;
    JET_COLUMNID        *prgcolumnid;
    unsigned long       cbKeyMost;
    unsigned long       cbVarSegMac;
    JET_TABLEID         tableid;
} JET_OPENTEMPORARYTABLE2;
#endif

typedef struct
{
    unsigned long   cbStruct;
    unsigned long   ibLongValue;
    unsigned long   itagSequence;
    JET_COLUMNID    columnidNextTagged;
} JET_RETINFO;

typedef struct
{
    unsigned long   cbStruct;
    unsigned long   ibLongValue;
    unsigned long   itagSequence;
} JET_SETINFO;

typedef struct
{
    unsigned long   cbStruct;
    unsigned long   centriesLT;
    unsigned long   centriesInRange;
    unsigned long   centriesTotal;
} JET_RECPOS;

typedef struct
{
    unsigned long   cbStruct;
    JET_TABLEID     tableid;
    unsigned long   cRecord;
    JET_COLUMNID    columnidBookmark;
} JET_RECORDLIST;

typedef struct
{
    unsigned long   cbStruct;
    JET_TABLEID     tableid;
    JET_GRBIT       grbit;
} JET_INDEXRANGE;

#if ( JET_VERSION >= 0x0602 )
typedef enum
{
    JET_relopEquals = 0,
    JET_relopPrefixEquals,

    JET_relopNotEquals,
    JET_relopLessThanOrEqual,
    JET_relopLessThan,
    JET_relopGreaterThanOrEqual,
    JET_relopGreaterThan,
    JET_relopBitmaskEqualsZero,
    JET_relopBitmaskNotEqualsZero,
} JET_RELOP;

typedef struct
{
    JET_COLUMNID    columnid;
    JET_RELOP       relop;
    void *          pv;
    unsigned long   cb;
    JET_GRBIT       grbit;
} JET_INDEX_COLUMN;

typedef struct
{
    JET_INDEX_COLUMN *  rgStartColumns;
    unsigned long       cStartColumns;
    JET_INDEX_COLUMN *  rgEndColumns;
    unsigned long       cEndColumns;
} JET_INDEX_RANGE;
#endif


typedef enum
{
    opDDLConvNull = 0,
    opDDLConvAddCallback = 1,
    opDDLConvChangeColumn = 2,
    opDDLConvAddConditionalColumnsToAllIndexes = 3,
    opDDLConvAddColumnCallback = 4,
    opDDLConvIncreaseMaxColumnSize = 5,
    opDDLConvChangeIndexDensity = 6,
    opDDLConvChangeCallbackDLL = 7,
    opDDLConvMax = 8
} JET_OPDDLCONV;

#if ( JET_VERSION >= 0x0501 )
typedef struct tagDDLADDCALLBACK_A
{
    char        *szTable;
    char        *szCallback;
    JET_CBTYP   cbtyp;
} JET_DDLADDCALLBACK_A;

typedef struct tagDDLADDCALLBACK_W
{
    WCHAR       *szTable;
    WCHAR       *szCallback;
    JET_CBTYP   cbtyp;
} JET_DDLADDCALLBACK_W;

#ifdef JET_UNICODE
#define JET_DDLADDCALLBACK JET_DDLADDCALLBACK_W
#else
#define JET_DDLADDCALLBACK JET_DDLADDCALLBACK_A
#endif
#endif

typedef struct tagDDLCHANGECOLUMN_A
{
    char        *szTable;
    char        *szColumn;
    JET_COLTYP  coltypNew;
    JET_GRBIT   grbitNew;
} JET_DDLCHANGECOLUMN_A;

typedef struct tagDDLCHANGECOLUMN_W
{
    WCHAR       *szTable;
    WCHAR       *szColumn;
    JET_COLTYP  coltypNew;
    JET_GRBIT   grbitNew;
} JET_DDLCHANGECOLUMN_W;


#ifdef JET_UNICODE
#define JET_DDLCHANGECOLUMN JET_DDLCHANGECOLUMN_W
#else
#define JET_DDLCHANGECOLUMN JET_DDLCHANGECOLUMN_A
#endif


typedef struct tagDDLMAXCOLUMNSIZE_A
{
    char            *szTable;
    char            *szColumn;
    unsigned long   cbMax;
} JET_DDLMAXCOLUMNSIZE_A;

typedef struct tagDDLMAXCOLUMNSIZE_W
{
    WCHAR           *szTable;
    WCHAR           *szColumn;
    unsigned long   cbMax;
} JET_DDLMAXCOLUMNSIZE_W;

#ifdef JET_UNICODE
#define JET_DDLMAXCOLUMNSIZE JET_DDLMAXCOLUMNSIZE_W
#else
#define JET_DDLMAXCOLUMNSIZE JET_DDLMAXCOLUMNSIZE_A
#endif

typedef struct tagDDLADDCONDITIONALCOLUMNSTOALLINDEXES_A
{
    char                    * szTable;
    JET_CONDITIONALCOLUMN_A * rgconditionalcolumn;
    unsigned long           cConditionalColumn;
} JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_A;

typedef struct tagDDLADDCONDITIONALCOLUMNSTOALLINDEXES_W
{
    WCHAR                   * szTable;
    JET_CONDITIONALCOLUMN_W * rgconditionalcolumn;
    unsigned long           cConditionalColumn;
} JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_W;

#ifdef JET_UNICODE
#define JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_W
#else
#define JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_A
#endif


typedef struct tagDDLADDCOLUMCALLBACK_A
{
    char            *szTable;
    char            *szColumn;
    char            *szCallback;
    void            *pvCallbackData;
    unsigned long   cbCallbackData;
} JET_DDLADDCOLUMNCALLBACK_A;

typedef struct tagDDLADDCOLUMCALLBACK_W
{
    WCHAR           *szTable;
    WCHAR           *szColumn;
    WCHAR           *szCallback;
    void            *pvCallbackData;
    unsigned long   cbCallbackData;
} JET_DDLADDCOLUMNCALLBACK_W;

#ifdef JET_UNICODE
#define JET_DDLADDCOLUMNCALLBACK JET_DDLADDCOLUMNCALLBACK_W
#else
#define JET_DDLADDCOLUMNCALLBACK JET_DDLADDCOLUMNCALLBACK_A
#endif

typedef struct tagDDLINDEXDENSITY_A
{
    char            *szTable;
    char            *szIndex;
    unsigned long   ulDensity;
} JET_DDLINDEXDENSITY_A;

typedef struct tagDDLINDEXDENSITY_W
{
    WCHAR           *szTable;
    WCHAR           *szIndex;
    unsigned long   ulDensity;
} JET_DDLINDEXDENSITY_W;

#ifdef JET_UNICODE
#define JET_DDLINDEXDENSITY JET_DDLINDEXDENSITY_W
#else
#define JET_DDLINDEXDENSITY JET_DDLINDEXDENSITY_A
#endif

typedef struct tagDDLCALLBACKDLL_A
{
    char            *szOldDLL;
    char            *szNewDLL;
} JET_DDLCALLBACKDLL_A;

typedef struct tagDDLCALLBACKDLL_W
{
    WCHAR           *szOldDLL;
    WCHAR           *szNewDLL;
} JET_DDLCALLBACKDLL_W;

#ifdef JET_UNICODE
#define JET_DDLCALLBACKDLL JET_DDLCALLBACKDLL_W
#else
#define JET_DDLCALLBACKDLL JET_DDLCALLBACKDLL_A
#endif



typedef struct
{
    void    *pvReserved1;
    void    *pvReserved2;
    unsigned long cbActual;
    JET_HANDLE  hSig;
    JET_ERR     err;
} JET_OLP;


#include <pshpack1.h>
#define JET_MAX_COMPUTERNAME_LENGTH 15

typedef struct
{
    char        bSeconds;
    char        bMinutes;
    char        bHours;
    char        bDay;
    char        bMonth;
    char        bYear;
    union
    {
        char        bFiller1;
        struct
        {
            unsigned char fTimeIsUTC:1;
            unsigned char bMillisecondsLow:7;
        };
    };
    union
    {
        char        bFiller2;
        struct
        {
            unsigned char fReserved:1;
            unsigned char bMillisecondsHigh:3;
            unsigned char fUnused:4;
        };
    };
} JET_LOGTIME;

#if ( JET_VERSION >= 0x0600 )
typedef struct
{
    char        bSeconds;
    char        bMinutes;
    char        bHours;
    char        bDay;
    char        bMonth;
    char        bYear;
    union
    {
        char        bFiller1;
        struct
        {
            unsigned char fTimeIsUTC:1;
            unsigned char bMillisecondsLow:7;
        };
    };
    union
    {
        char        bFiller2;
        struct
        {
            unsigned char fOSSnapshot:1;
            unsigned char bMillisecondsHigh:3;
            unsigned char fReserved:4;
        };
    };
} JET_BKLOGTIME;
#endif

typedef struct
{
    unsigned short  ib;
    unsigned short  isec;
    long            lGeneration;
} JET_LGPOS;

typedef struct
{
    unsigned long   ulRandom;
    JET_LOGTIME     logtimeCreate;
    char            szComputerName[ JET_MAX_COMPUTERNAME_LENGTH + 1 ];
} JET_SIGNATURE;

#if ( JET_VERSION >= 0x0600 )
typedef struct
{
    unsigned long   genMin;
    unsigned long   genMax;
    JET_LOGTIME     logtimeGenMaxCreate;
} JET_CHECKPOINTINFO;
#endif

typedef struct
{
    JET_LGPOS       lgposMark;
    union
    {
        JET_LOGTIME     logtimeMark;
#if ( JET_VERSION >= 0x0600 )
        JET_BKLOGTIME   bklogtimeMark;
#endif
    };
    unsigned long   genLow;
    unsigned long   genHigh;
} JET_BKINFO;

#include <poppack.h>

typedef struct
{
    unsigned long   ulVersion;
    unsigned long   ulUpdate;
    JET_SIGNATURE   signDb;
    unsigned long   dbstate;

    JET_LGPOS       lgposConsistent;
    JET_LOGTIME     logtimeConsistent;

    JET_LOGTIME     logtimeAttach;
    JET_LGPOS       lgposAttach;

    JET_LOGTIME     logtimeDetach;
    JET_LGPOS       lgposDetach;

    JET_SIGNATURE   signLog;

    JET_BKINFO      bkinfoFullPrev;

    JET_BKINFO      bkinfoIncPrev;
    JET_BKINFO      bkinfoFullCur;
    unsigned long   fShadowingDisabled;
    unsigned long   fUpgradeDb;


    unsigned long   dwMajorVersion;
    unsigned long   dwMinorVersion;
    unsigned long   dwBuildNumber;
    long            lSPNumber;

    unsigned long   cbPageSize;

} JET_DBINFOMISC;

#if ( JET_VERSION >= 0x0600 )
typedef struct
{
    unsigned long   ulVersion;
    unsigned long   ulUpdate;
    JET_SIGNATURE   signDb;
    unsigned long   dbstate;

    JET_LGPOS       lgposConsistent;
    JET_LOGTIME     logtimeConsistent;

    JET_LOGTIME     logtimeAttach;
    JET_LGPOS       lgposAttach;

    JET_LOGTIME     logtimeDetach;
    JET_LGPOS       lgposDetach;

    JET_SIGNATURE   signLog;

    JET_BKINFO      bkinfoFullPrev;

    JET_BKINFO      bkinfoIncPrev;
    JET_BKINFO      bkinfoFullCur;
    unsigned long   fShadowingDisabled;
    unsigned long   fUpgradeDb;


    unsigned long   dwMajorVersion;
    unsigned long   dwMinorVersion;
    unsigned long   dwBuildNumber;
    long            lSPNumber;

    unsigned long   cbPageSize;

    unsigned long   genMinRequired;
    unsigned long   genMaxRequired;
    JET_LOGTIME     logtimeGenMaxCreate;

    unsigned long   ulRepairCount;
    JET_LOGTIME     logtimeRepair;
    unsigned long   ulRepairCountOld;

    unsigned long   ulECCFixSuccess;
    JET_LOGTIME     logtimeECCFixSuccess;
    unsigned long   ulECCFixSuccessOld;

    unsigned long   ulECCFixFail;
    JET_LOGTIME     logtimeECCFixFail;
    unsigned long   ulECCFixFailOld;

    unsigned long   ulBadChecksum;
    JET_LOGTIME     logtimeBadChecksum;
    unsigned long   ulBadChecksumOld;

} JET_DBINFOMISC2;
#endif

#if ( JET_VERSION >= 0x0601 )
typedef struct
{
    unsigned long   ulVersion;
    unsigned long   ulUpdate;
    JET_SIGNATURE   signDb;
    unsigned long   dbstate;

    JET_LGPOS       lgposConsistent;
    JET_LOGTIME     logtimeConsistent;

    JET_LOGTIME     logtimeAttach;
    JET_LGPOS       lgposAttach;

    JET_LOGTIME     logtimeDetach;
    JET_LGPOS       lgposDetach;

    JET_SIGNATURE   signLog;

    JET_BKINFO      bkinfoFullPrev;

    JET_BKINFO      bkinfoIncPrev;
    JET_BKINFO      bkinfoFullCur;
    unsigned long   fShadowingDisabled;
    unsigned long   fUpgradeDb;


    unsigned long   dwMajorVersion;
    unsigned long   dwMinorVersion;
    unsigned long   dwBuildNumber;
    long            lSPNumber;

    unsigned long   cbPageSize;

    unsigned long   genMinRequired;
    unsigned long   genMaxRequired;
    JET_LOGTIME     logtimeGenMaxCreate;

    unsigned long   ulRepairCount;
    JET_LOGTIME     logtimeRepair;
    unsigned long   ulRepairCountOld;

    unsigned long   ulECCFixSuccess;
    JET_LOGTIME     logtimeECCFixSuccess;
    unsigned long   ulECCFixSuccessOld;

    unsigned long   ulECCFixFail;
    JET_LOGTIME     logtimeECCFixFail;
    unsigned long   ulECCFixFailOld;

    unsigned long   ulBadChecksum;
    JET_LOGTIME     logtimeBadChecksum;
    unsigned long   ulBadChecksumOld;

    unsigned long   genCommitted;

} JET_DBINFOMISC3;

typedef struct
{
    unsigned long   ulVersion;
    unsigned long   ulUpdate;
    JET_SIGNATURE   signDb;
    unsigned long   dbstate;

    JET_LGPOS       lgposConsistent;
    JET_LOGTIME     logtimeConsistent;

    JET_LOGTIME     logtimeAttach;
    JET_LGPOS       lgposAttach;

    JET_LOGTIME     logtimeDetach;
    JET_LGPOS       lgposDetach;

    JET_SIGNATURE   signLog;

    JET_BKINFO      bkinfoFullPrev;

    JET_BKINFO      bkinfoIncPrev;
    JET_BKINFO      bkinfoFullCur;
    unsigned long   fShadowingDisabled;
    unsigned long   fUpgradeDb;


    unsigned long   dwMajorVersion;
    unsigned long   dwMinorVersion;
    unsigned long   dwBuildNumber;
    long            lSPNumber;

    unsigned long   cbPageSize;

    unsigned long   genMinRequired;
    unsigned long   genMaxRequired;
    JET_LOGTIME     logtimeGenMaxCreate;

    unsigned long   ulRepairCount;
    JET_LOGTIME     logtimeRepair;
    unsigned long   ulRepairCountOld;

    unsigned long   ulECCFixSuccess;
    JET_LOGTIME     logtimeECCFixSuccess;
    unsigned long   ulECCFixSuccessOld;

    unsigned long   ulECCFixFail;
    JET_LOGTIME     logtimeECCFixFail;
    unsigned long   ulECCFixFailOld;

    unsigned long   ulBadChecksum;
    JET_LOGTIME     logtimeBadChecksum;
    unsigned long   ulBadChecksumOld;

    unsigned long   genCommitted;

    JET_BKINFO  bkinfoCopyPrev;
    JET_BKINFO  bkinfoDiffPrev;
} JET_DBINFOMISC4;
#endif

#if ( JET_VERSION >= 0x0601 )
typedef struct
{
    unsigned long   ulVersion;
    unsigned long   ulUpdate;
    JET_SIGNATURE   signDb;
    unsigned long   dbstate;

    JET_LGPOS       lgposConsistent;
    JET_LOGTIME     logtimeConsistent;

    JET_LOGTIME     logtimeAttach;
    JET_LGPOS       lgposAttach;

    JET_LOGTIME     logtimeDetach;
    JET_LGPOS       lgposDetach;

    JET_SIGNATURE   signLog;

    JET_BKINFO      bkinfoFullPrev;

    JET_BKINFO      bkinfoIncPrev;
    JET_BKINFO      bkinfoFullCur;
    unsigned long   fShadowingDisabled;
    unsigned long   fUpgradeDb;


    unsigned long   dwMajorVersion;
    unsigned long   dwMinorVersion;
    unsigned long   dwBuildNumber;
    long            lSPNumber;

    unsigned long   cbPageSize;

    unsigned long   genMinRequired;
    unsigned long   genMaxRequired;
    JET_LOGTIME     logtimeGenMaxCreate;

    unsigned long   ulRepairCount;
    JET_LOGTIME     logtimeRepair;
    unsigned long   ulRepairCountOld;

    unsigned long   ulECCFixSuccess;
    JET_LOGTIME     logtimeECCFixSuccess;
    unsigned long   ulECCFixSuccessOld;

    unsigned long   ulECCFixFail;
    JET_LOGTIME     logtimeECCFixFail;
    unsigned long   ulECCFixFailOld;

    unsigned long   ulBadChecksum;
    JET_LOGTIME     logtimeBadChecksum;
    unsigned long   ulBadChecksumOld;

    unsigned long   genCommitted;

    JET_BKINFO  bkinfoCopyPrev;
    JET_BKINFO  bkinfoDiffPrev;

    unsigned long   ulIncrementalReseedCount;
    JET_LOGTIME     logtimeIncrementalReseed;
    unsigned long   ulIncrementalReseedCountOld;

    unsigned long   ulPagePatchCount;
    JET_LOGTIME     logtimePagePatch;
    unsigned long   ulPagePatchCountOld;
} JET_DBINFOMISC5;

typedef struct
{
    unsigned long   ulVersion;
    unsigned long   ulUpdate;
    JET_SIGNATURE   signDb;
    unsigned long   dbstate;

    JET_LGPOS       lgposConsistent;
    JET_LOGTIME     logtimeConsistent;

    JET_LOGTIME     logtimeAttach;
    JET_LGPOS       lgposAttach;

    JET_LOGTIME     logtimeDetach;
    JET_LGPOS       lgposDetach;

    JET_SIGNATURE   signLog;

    JET_BKINFO      bkinfoFullPrev;

    JET_BKINFO      bkinfoIncPrev;
    JET_BKINFO      bkinfoFullCur;
    unsigned long   fShadowingDisabled;
    unsigned long   fUpgradeDb;


    unsigned long   dwMajorVersion;
    unsigned long   dwMinorVersion;
    unsigned long   dwBuildNumber;
    long            lSPNumber;

    unsigned long   cbPageSize;

    unsigned long   genMinRequired;
    unsigned long   genMaxRequired;
    JET_LOGTIME     logtimeGenMaxCreate;

    unsigned long   ulRepairCount;
    JET_LOGTIME     logtimeRepair;
    unsigned long   ulRepairCountOld;

    unsigned long   ulECCFixSuccess;
    JET_LOGTIME     logtimeECCFixSuccess;
    unsigned long   ulECCFixSuccessOld;

    unsigned long   ulECCFixFail;
    JET_LOGTIME     logtimeECCFixFail;
    unsigned long   ulECCFixFailOld;

    unsigned long   ulBadChecksum;
    JET_LOGTIME     logtimeBadChecksum;
    unsigned long   ulBadChecksumOld;

    unsigned long   genCommitted;

    JET_BKINFO  bkinfoCopyPrev;
    JET_BKINFO  bkinfoDiffPrev;

    unsigned long   ulIncrementalReseedCount;
    JET_LOGTIME     logtimeIncrementalReseed;
    unsigned long   ulIncrementalReseedCountOld;

    unsigned long   ulPagePatchCount;
    JET_LOGTIME     logtimePagePatch;
    unsigned long   ulPagePatchCountOld;

    JET_LOGTIME logtimeChecksumPrev;
    JET_LOGTIME logtimeChecksumStart;
    unsigned long cpgDatabaseChecked;
} JET_DBINFOMISC6;
#endif

#if ( JET_VERSION >= 0x0A00 )
typedef struct
{
    unsigned long   ulVersion;
    unsigned long   ulUpdate;
    JET_SIGNATURE   signDb;
    unsigned long   dbstate;

    JET_LGPOS       lgposConsistent;
    JET_LOGTIME     logtimeConsistent;

    JET_LOGTIME     logtimeAttach;
    JET_LGPOS       lgposAttach;

    JET_LOGTIME     logtimeDetach;
    JET_LGPOS       lgposDetach;

    JET_SIGNATURE   signLog;

    JET_BKINFO      bkinfoFullPrev;

    JET_BKINFO      bkinfoIncPrev;
    JET_BKINFO      bkinfoFullCur;
    unsigned long   fShadowingDisabled;
    unsigned long   fUpgradeDb;


    unsigned long   dwMajorVersion;
    unsigned long   dwMinorVersion;
    unsigned long   dwBuildNumber;
    long            lSPNumber;

    unsigned long   cbPageSize;

    unsigned long   genMinRequired;
    unsigned long   genMaxRequired;
    JET_LOGTIME     logtimeGenMaxCreate;

    unsigned long   ulRepairCount;
    JET_LOGTIME     logtimeRepair;
    unsigned long   ulRepairCountOld;

    unsigned long   ulECCFixSuccess;
    JET_LOGTIME     logtimeECCFixSuccess;
    unsigned long   ulECCFixSuccessOld;

    unsigned long   ulECCFixFail;
    JET_LOGTIME     logtimeECCFixFail;
    unsigned long   ulECCFixFailOld;

    unsigned long   ulBadChecksum;
    JET_LOGTIME     logtimeBadChecksum;
    unsigned long   ulBadChecksumOld;

    unsigned long   genCommitted;

    JET_BKINFO  bkinfoCopyPrev;
    JET_BKINFO  bkinfoDiffPrev;

    unsigned long   ulIncrementalReseedCount;
    JET_LOGTIME     logtimeIncrementalReseed;
    unsigned long   ulIncrementalReseedCountOld;

    unsigned long   ulPagePatchCount;
    JET_LOGTIME     logtimePagePatch;
    unsigned long   ulPagePatchCountOld;

    JET_LOGTIME logtimeChecksumPrev;
    JET_LOGTIME logtimeChecksumStart;
    unsigned long cpgDatabaseChecked;

    JET_LOGTIME     logtimeLastReAttach;
    JET_LGPOS       lgposLastReAttach;
} JET_DBINFOMISC7;
#endif

typedef struct
{
    unsigned long   ulGeneration;
    JET_SIGNATURE   signLog;

    JET_LOGTIME     logtimeCreate;
    JET_LOGTIME     logtimePreviousGeneration;

    unsigned long   ulFlags;

    unsigned long   ulVersionMajor;
    unsigned long   ulVersionMinor;
    unsigned long   ulVersionUpdate;

    unsigned long   cbSectorSize;
    unsigned long   cbHeader;
    unsigned long   cbFile;
    unsigned long   cbDatabasePageSize;
} JET_LOGINFOMISC;

#if ( JET_VERSION >= 0x0601 )
typedef struct
{
    unsigned long   ulGeneration;
    JET_SIGNATURE   signLog;

    JET_LOGTIME     logtimeCreate;
    JET_LOGTIME     logtimePreviousGeneration;

    unsigned long   ulFlags;

    unsigned long   ulVersionMajor;
    unsigned long   ulVersionMinor;
    unsigned long   ulVersionUpdate;

    unsigned long   cbSectorSize;
    unsigned long   cbHeader;
    unsigned long   cbFile;
    unsigned long   cbDatabasePageSize;

    JET_LGPOS       lgposCheckpoint;
} JET_LOGINFOMISC2;

#endif

#if ( JET_VERSION >= 0x0A01 )
typedef struct
{
    unsigned long   ulGeneration;
    JET_SIGNATURE   signLog;

    JET_LOGTIME     logtimeCreate;
    JET_LOGTIME     logtimePreviousGeneration;

    unsigned long   ulFlags;

    unsigned long   ulVersionMajor;
    unsigned long   ulVersionUpdateMajor;
    unsigned long   ulVersionUpdateMinor;

    unsigned long   cbSectorSize;
    unsigned long   cbHeader;
    unsigned long   cbFile;
    unsigned long   cbDatabasePageSize;

    JET_LGPOS       lgposCheckpoint;

    unsigned long   ulVersionMinorDeprecated;

    unsigned __int64    checksumPrevLogAllSegments;

} JET_LOGINFOMISC3;

#endif

#if ( JET_VERSION >= 0x0A00 )


#define JET_OpenLogForRecoveryCheckingAndPatching       1
#define JET_OpenLogForRedo                  2
#define JET_OpenLogForUndo                  3
#define JET_OpenLogForDo                    4

#define JET_MissingLogMustFail              1
#define JET_MissingLogContinueToRedo        2
#define JET_MissingLogContinueTryCurrentLog 3
#define JET_MissingLogContinueToUndo        4
#define JET_MissingLogCreateNewLogStream    5

typedef struct
{
    unsigned long       cbStruct;
    JET_ERR             errDefault;
    JET_INSTANCE        instance;

    JET_SNT             sntUnion;

    union {

        struct
        {
            unsigned long       cbStruct;
            unsigned long       lGenNext;
            unsigned char       fCurrentLog:1;
            unsigned char       eReason;
            unsigned char       rgbReserved[6];
            WCHAR *             wszLogFile;
            unsigned long       cdbinfomisc;
            JET_DBINFOMISC7 *   rgdbinfomisc;
        } OpenLog;

        struct
        {
            unsigned long       cbStruct;
            WCHAR *             wszCheckpoint;
        } OpenCheckpoint;


        struct
        {
            unsigned long       cbStruct;
            unsigned long       lGenMissing;
            unsigned char       fCurrentLog:1;
            unsigned char       eNextAction;
            unsigned char       rgbReserved[6];
            WCHAR *             wszLogFile;
            unsigned long       cdbinfomisc;
            JET_DBINFOMISC7 *   rgdbinfomisc;
        } MissingLog;

        struct
        {
            unsigned long       cbStruct;
            unsigned long       cdbinfomisc;
            JET_DBINFOMISC7 *   rgdbinfomisc;
        } BeginUndo;

        struct
        {
            unsigned long       cbStruct;
            unsigned long       EventID;
        } NotificationEvent;

        struct
        {
            unsigned long       cbStruct;
        } SignalErrorCondition;

        struct
        {
            unsigned long       cbStruct;
            const WCHAR *       wszDbPath;
        } AttachedDb;

        struct
        {
            unsigned long       cbStruct;
            const WCHAR *       wszDbPath;
        } DetachingDb;

        struct
        {
            unsigned long       cbStruct;
            const void *        pbCommitCtx;
            unsigned long       cbCommitCtx;
        } CommitCtx;
    };
} JET_RECOVERYCONTROL;
#endif

#if ( JET_VERSION >= 0x0600 )
typedef struct
{
    unsigned long   cbStruct;
    JET_SNC         snc;
    unsigned long   ul;
    char            sz[256];
} JET_SNMSG;
#endif

#if ( JET_VERSION >= 0x0A01 )

typedef struct
{
    unsigned long   cbStruct;
    unsigned long   pageNumber;
    const WCHAR *   szLogFile;
    JET_INSTANCE    instance;
    JET_DBINFOMISC7 dbinfomisc;
    const void *    pvToken;
    unsigned long   cbToken;
    const void *    pvData;
    unsigned long   cbData;
    JET_DBID        dbid;
} JET_SNPATCHREQUEST;

typedef struct
{
    unsigned long   cbStruct;
    const WCHAR *   wszDatabase;
    JET_DBID        dbid;
    JET_DBINFOMISC7 dbinfomisc;
    unsigned long   pageNumber;
} JET_SNCORRUPTEDPAGE;
#endif

typedef struct
{
    unsigned long   cpageOwned;
    unsigned long   cpageAvail;
} JET_STREAMINGFILESPACEINFO;


#if ( JET_VERSION >= 0x0600 )
struct JET_THREADSTATS
{
    unsigned long   cbStruct;
    unsigned long   cPageReferenced;
    unsigned long   cPageRead;
    unsigned long   cPagePreread;
    unsigned long   cPageDirtied;
    unsigned long   cPageRedirtied;
    unsigned long   cLogRecord;
    unsigned long   cbLogRecord;
};
#endif

#if ( JET_VERSION >= 0x0A00 )
struct JET_THREADSTATS2
{
    unsigned long       cbStruct;
    unsigned long       cPageReferenced;
    unsigned long       cPageRead;
    unsigned long       cPagePreread;
    unsigned long       cPageDirtied;
    unsigned long       cPageRedirtied;
    unsigned long       cLogRecord;
    unsigned long       cbLogRecord;
    unsigned __int64    cusecPageCacheMiss;
    unsigned long       cPageCacheMiss;
};
#endif

#if ( JET_VERSION >= 0x0A01 )
struct JET_THREADSTATS3
{
    unsigned long       cbStruct;
    unsigned long       cPageReferenced;
    unsigned long       cPageRead;
    unsigned long       cPagePreread;
    unsigned long       cPageDirtied;
    unsigned long       cPageRedirtied;
    unsigned long       cLogRecord;
    unsigned long       cbLogRecord;
    unsigned __int64    cusecPageCacheMiss;
    unsigned long       cPageCacheMiss;
    unsigned long       cSeparatedLongValueRead;
    unsigned __int64    cusecLongValuePageCacheMiss;
    unsigned long       cLongValuePageCacheMiss;
};
#endif

#if ( JET_VERSION >= 0x0A01 )
struct JET_THREADSTATS4
{
    unsigned long       cbStruct;
    unsigned long       cPageReferenced;
    unsigned long       cPageRead;
    unsigned long       cPagePreread;
    unsigned long       cPageDirtied;
    unsigned long       cPageRedirtied;
    unsigned long       cLogRecord;
    unsigned long       cbLogRecord;
    unsigned __int64    cusecPageCacheMiss;
    unsigned long       cPageCacheMiss;
    unsigned long       cSeparatedLongValueRead;
    unsigned __int64    cusecLongValuePageCacheMiss;
    unsigned long       cLongValuePageCacheMiss;
    unsigned long       cSeparatedLongValueCreated;
    unsigned long       cPageUniqueCacheHits;
    unsigned long       cPageUniqueCacheRequests;
    unsigned long       cDatabaseReads;
    unsigned long       cSumDatabaseReadQueueDepthImpact;
    unsigned long       cSumDatabaseReadQueueDepth;
    unsigned __int64    cusecWait;
    unsigned long       cWait;
    unsigned long       cNodesFlagDeleted;
    unsigned long       cbNodesFlagDeleted;
    unsigned long       cPageTableAllocated;
    unsigned long       cPageTableReleased;
    unsigned long       cPageUpdateAllocated;
    unsigned long       cPageUpdateReleased;
    unsigned long       cPageUniqueModified;
};
#endif

#if ( JET_VERSION >= 0x0603 )

typedef enum
{
    JET_residNull,
    JET_residFCB,
    JET_residFUCB,
    JET_residTDB,
    JET_residIDB,
    JET_residPIB,
    JET_residSCB,
    JET_residVERBUCKET,
    JET_residPAGE,
    JET_residSPLIT,
    JET_residSPLITPATH,
    JET_residMERGE,
    JET_residMERGEPATH,
    JET_residINST,
    JET_residLOG,
    JET_residKEY,
    JET_residBOOKMARK,
    JET_residLRUKHIST,
    JET_residRBSBuf,
    JET_residTest,
    JET_residMax,
    JET_residAll = 0x0fffffff
} JET_RESID;

#endif

#if ( JET_VERSION >= 0x0600 )


typedef enum
{
    JET_resoperNull,
    JET_resoperTag,
    JET_resoperSize,
    JET_resoperAlign,
    JET_resoperMinUse,
    JET_resoperMaxUse,
    JET_resoperObjectsPerChunk,
    JET_resoperCurrentUse,
    JET_resoperChunkSize,
    JET_resoperLookasideEntries,
    JET_resoperRFOLOffset,
    JET_resoperGuard,
    JET_resoperAllocTopDown,
    JET_resoperPreserveFreed,
    JET_resoperAllocFromHeap,
    JET_resoperEnableMaxUse,
    JET_resoperRCIList,
    JET_resoperSectionHeader,
    JET_resoperMax
} JET_RESOPER;

#endif


#if ( JET_VERSION >= 0x0600 )

typedef struct
{
    unsigned long           cbStruct;

    JET_RSTMAP_A *          rgrstmap;
    long                    crstmap;

    JET_LGPOS               lgposStop;
    JET_LOGTIME             logtimeStop;

    JET_PFNSTATUS           pfnStatus;
} JET_RSTINFO_A;

typedef struct
{
    unsigned long           cbStruct;

    JET_RSTMAP_W *          rgrstmap;
    long                    crstmap;

    JET_LGPOS               lgposStop;
    JET_LOGTIME             logtimeStop;

    JET_PFNSTATUS           pfnStatus;
} JET_RSTINFO_W;

#ifdef JET_UNICODE
#define JET_RSTINFO JET_RSTINFO_W
#else
#define JET_RSTINFO JET_RSTINFO_A
#endif

#endif


#if ( JET_VERSION >= 0x0A01 )

typedef struct
{
    unsigned long           cbStruct;

    JET_RSTMAP2_A *         rgrstmap;
    long                    crstmap;

    JET_LGPOS               lgposStop;
    JET_LOGTIME             logtimeStop;

    JET_PFNINITCALLBACK     pfnCallback;
    void *                  pvCallbackContext;
} JET_RSTINFO2_A;

typedef struct
{
    unsigned long           cbStruct;

    JET_RSTMAP2_W *         rgrstmap;
    long                    crstmap;

    JET_LGPOS               lgposStop;
    JET_LOGTIME             logtimeStop;

    JET_PFNINITCALLBACK     pfnCallback;
    void *                  pvCallbackContext;
} JET_RSTINFO2_W;

#ifdef JET_UNICODE
#define JET_RSTINFO2 JET_RSTINFO2_W
#else
#define JET_RSTINFO2 JET_RSTINFO2_A
#endif

#endif

#if ( JET_VERSION >= 0x0601 )
typedef enum
{
    eBTreeTypeInvalid = 0,
    eBTreeTypeInternalDbRootSpace,
    eBTreeTypeInternalSpaceOE,
    eBTreeTypeInternalSpaceAE,
    eBTreeTypeUserClusteredIndex,
    eBTreeTypeInternalLongValue,
    eBTreeTypeUserSecondaryIndex,
    eBTreeTypeMax
} JET_BTREETYPE;


typedef struct _BTREE_STATS_BASIC_CATALOG
{
    unsigned long                   cbStruct;
    JET_BTREETYPE                   eType;
    WCHAR                           rgName[64];
    unsigned long                   objidFDP;
    unsigned long                   pgnoFDP;
    JET_SPACEHINTS *            pSpaceHints;
} BTREE_STATS_BASIC_CATALOG;

typedef struct _BTREE_SPACE_EXTENT_INFO
{
    unsigned long                   iPool;
    unsigned long                   pgnoLast;
    unsigned long                   cpgExtent;
    unsigned long                   pgnoSpaceNode;
} BTREE_SPACE_EXTENT_INFO;

typedef struct _BTREE_STATS_SPACE_TREES
{
    unsigned long                   cbStruct;
    unsigned long                   cpgPrimary;
    unsigned long                   cpgLastAlloc;
    unsigned long                   fMultiExtent;
    unsigned long                   pgnoOE;
    unsigned long                   pgnoAE;
    unsigned long                   cpgOwned;
    unsigned long                   cpgAvailable;
    unsigned long                   cpgReserved;
    unsigned long                   cpgShelved;
    int                             fAutoIncPresents;
    unsigned __int64                qwAutoInc;
    unsigned long                   cOwnedExtents;
    _Field_size_opt_(cOwnedExtents) BTREE_SPACE_EXTENT_INFO *       prgOwnedExtents;
    unsigned long                   cAvailExtents;
    _Field_size_opt_(cAvailExtents) BTREE_SPACE_EXTENT_INFO *       prgAvailExtents;
} BTREE_STATS_SPACE_TREES;

typedef struct
{
    unsigned long                   cbStruct;
    JET_HISTO *                     phistoFreeBytes;
    JET_HISTO *                     phistoNodeCounts;
    JET_HISTO *                     phistoKeySizes;
    JET_HISTO *                     phistoDataSizes;
    JET_HISTO *                     phistoKeyCompression;
    JET_HISTO *                     phistoUnreclaimedBytes;
#if ( JET_VERSION >= 0x0602 )
    __int64                         cVersionedNodes;
#endif
} BTREE_STATS_PAGE_SPACE;


typedef struct _BTREE_STATS_PARENT_OF_LEAF
{
    unsigned long                   cbStruct;
    unsigned long                   fEmpty;
    unsigned long                   cpgInternal;
    unsigned long                   cpgData;
    unsigned long                   cDepth;
    JET_HISTO *                     phistoIOContiguousRuns;
    unsigned long                   cForwardScans;
    BTREE_STATS_PAGE_SPACE *        pInternalPageStats;
} BTREE_STATS_PARENT_OF_LEAF;

#if ( JET_VERSION >= 0x0602 )

typedef struct _BTREE_STATS_LV
{
    unsigned long                   cbStruct;
    __int64                         cLVRefs;
    __int64                         cCorruptLVs;
    __int64                         cSeparatedRootChunks;
    __int64                         cPartiallyDeletedLVs;
    unsigned __int64                lidMax;
    int                             cbLVChunkMax;
    JET_HISTO *                     phistoLVSize;
    JET_HISTO *                     phistoLVComp;
    JET_HISTO *                     phistoLVRatio;
    JET_HISTO *                     phistoLVSeeks;
    JET_HISTO *                     phistoLVBytes;
    JET_HISTO *                     phistoLVExtraSeeks;
    JET_HISTO *                     phistoLVExtraBytes;
} BTREE_STATS_LV;

#endif

typedef struct _BTREE_STATS
{
    unsigned long                   cbStruct;
    unsigned long                   grbitData;
    struct _BTREE_STATS *           pParent;
    BTREE_STATS_BASIC_CATALOG *     pBasicCatalog;
    BTREE_STATS_SPACE_TREES *       pSpaceTrees;
    BTREE_STATS_PARENT_OF_LEAF *    pParentOfLeaf;
    BTREE_STATS_PAGE_SPACE *        pFullWalk;
#if ( JET_VERSION >= 0x0602 )
    BTREE_STATS_LV *                pLvData;
#endif
} BTREE_STATS;

typedef JET_ERR (JET_API *JET_PFNSPACEDATA)(
    _In_ BTREE_STATS *      pBTreeStats,
    _In_ JET_API_PTR        pvContext );
#endif



#if ( JET_VERSION >= 0x0602 )




typedef enum
{
    JET_errcatUnknown = 0,
    JET_errcatError,
    JET_errcatOperation,
    JET_errcatFatal,
    JET_errcatIO,
    JET_errcatResource,
    JET_errcatMemory,
    JET_errcatQuota,
    JET_errcatDisk,
    JET_errcatData,
    JET_errcatCorruption,
    JET_errcatInconsistent,
    JET_errcatFragmentation,
    JET_errcatApi,
    JET_errcatUsage,
    JET_errcatState,
    JET_errcatObsolete,
    JET_errcatMax,
} JET_ERRCAT;

typedef struct
{
    unsigned long       cbStruct;
    JET_ERR             errValue;
    JET_ERRCAT          errcatMostSpecific;
    unsigned char       rgCategoricalHierarchy[8];
    unsigned long       lSourceLine;
    WCHAR               rgszSourceFile[64];
} JET_ERRINFOBASIC_W;

#if ( JET_VERSION >= 0x0A00 )
#define JET_bitDurableCommitCallbackLogUnavailable      0x00000001
#endif

typedef struct
{
    JET_SIGNATURE   signLog;
    int             reserved;
    __int64         commitId;
} JET_COMMIT_ID;


typedef JET_ERR (JET_API *JET_PFNDURABLECOMMITCALLBACK)(
    _In_ JET_INSTANCE   instance,
    _In_ JET_COMMIT_ID *pCommitIdSeen,
    _In_ JET_GRBIT      grbit );

#endif

typedef struct
{
    long                    lRBSGeneration;

    JET_LOGTIME             logtimeCreate;
    JET_LOGTIME             logtimeCreatePrevRBS;

    unsigned long           ulMajor;
    unsigned long           ulMinor;

    unsigned long long      cbLogicalFileSize;
} JET_RBSINFOMISC;

typedef struct
{
    long                    lGenMinRevertStart;
    long                    lGenMaxRevertStart;

    long                    lGenMinRevertEnd;
    long                    lGenMaxRevertEnd;

    JET_LOGTIME             logtimeRevertFrom;

    unsigned long long      cSecRevert;
    unsigned long long      cPagesReverted;
} JET_RBSREVERTINFOMISC;





#if ( JET_VERSION >= 0x0501 )
#define JET_instanceNil             (~(JET_INSTANCE)0)
#endif
#define JET_sesidNil                (~(JET_SESID)0)
#define JET_tableidNil              (~(JET_TABLEID)0)
#define JET_columnidNil             (~(JET_COLUMNID)0)
#define JET_bitNil                  ((JET_GRBIT)0)



#define JET_cbBookmarkMost          256
#if ( JET_VERSION >= 0x0601 )
#define JET_cbBookmarkMostMost      JET_cbKeyMostMost
#endif



#ifndef JET_UNICODE
#define JET_cbNameMost              64
#else
#define JET_cbNameMost              128
#endif



#ifndef JET_UNICODE
#define JET_cbFullNameMost          255
#else
#define JET_cbFullNameMost          510
#endif




#define JET_cbColumnLVPageOverhead      82

#define JET_cbColumnLVChunkMost     ( 4096 - 82 )
#define JET_cbColumnLVChunkMost_OLD 4035



#define JET_cbLVDefaultValueMost    255



#define JET_cbColumnMost            255



#define JET_cbLVColumnMost          0x7FFFFFFF



#if ( JET_VERSION >= 0x0601 )
#define JET_cbKeyMostMost               JET_cbKeyMost32KBytePage
#define JET_cbKeyMost32KBytePage        JET_cbKeyMost8KBytePage
#define JET_cbKeyMost16KBytePage        JET_cbKeyMost8KBytePage
#endif
#if ( JET_VERSION >= 0x0600 )
#define JET_cbKeyMost8KBytePage     2000
#define JET_cbKeyMost4KBytePage     1000
#define JET_cbKeyMost2KBytePage     500
#define JET_cbKeyMostMin            255
#endif

#define JET_cbKeyMost               255
#define JET_cbLimitKeyMost          256
#define JET_cbPrimaryKeyMost        255
#define JET_cbSecondaryKeyMost      255
#define JET_cbKeyMost_OLD           255




#if ( JET_VERSION >= 0x0600 )
#define JET_ccolKeyMost             16
#else
#define JET_ccolKeyMost             12
#endif

#if ( JET_VERSION >= 0x0501 )
#define JET_ccolMost                0x0000fee0
#else
#define JET_ccolMost                0x00007ffe
#endif
#define JET_ccolFixedMost           0x0000007f
#define JET_ccolVarMost             0x00000080
#define JET_ccolTaggedMost          ( JET_ccolMost - 0x000000ff )

#if ( JET_VERSION >= 0x0501 )
#define JET_EventLoggingDisable     0
#if ( JET_VERSION >= 0x0601 )
#define JET_EventLoggingLevelMin    1
#define JET_EventLoggingLevelLow    25
#define JET_EventLoggingLevelMedium 50
#define JET_EventLoggingLevelHigh   75
#endif
#define JET_EventLoggingLevelMax    100
#endif

#if ( JET_VERSION >= 0x0603 )
typedef enum
{
    JET_IndexCheckingOff = 0,
    JET_IndexCheckingOn = 1,
    JET_IndexCheckingDeferToOpenTable = 2,
    JET_IndexCheckingMax = 3,
} JET_INDEXCHECKING;
#endif


#if ( JET_VERSION >= 0x0600 )
#define JET_IOPriorityNormal                    0x0
#define JET_IOPriorityLow                       0x1
#endif
#if ( JET_VERSION >= 0x0603 )
#define JET_IOPriorityLowForCheckpoint          0x2
#define JET_IOPriorityLowForScavenge            0x4

#define JET_IOPriorityUserClassIdMask           0x0F000000
#define JET_IOPriorityMarkAsMaintenance         0x40000000
#endif

#if ( JET_VERSION >= 0x0602 )
#define JET_configDefault                       0x0001
#define JET_configRemoveQuotas                  0x0002
#define JET_configLowDiskFootprint              0x0004
#define JET_configMediumDiskFootprint           0x0008
#define JET_configLowMemory                     0x0010
#define JET_configDynamicMediumMemory           0x0020
#define JET_configLowPower                      0x0040
#define JET_configSSDProfileIO                  0x0080
#define JET_configRunSilent                     0x0100
#if ( JET_VERSION >= 0x0A00 )
#define JET_configUnthrottledMemory             0x0200
#define JET_configHighConcurrencyScaling        0x0400
#endif

#define JET_configMask                          (JET_configDefault|JET_configRemoveQuotas|JET_configLowDiskFootprint|JET_configMediumDiskFootprint|JET_configLowMemory|JET_configDynamicMediumMemory|JET_configLowPower|JET_configSSDProfileIO|JET_configRunSilent|JET_configUnthrottledMemory|JET_configHighConcurrencyScaling)
#endif

#define JET_paramSystemPath                     0
#define JET_paramTempPath                       1
#define JET_paramLogFilePath                    2
#define JET_paramBaseName                       3
#define JET_paramEventSource                    4

#define JET_paramMaxSessions                    5
#define JET_paramMaxOpenTables                  6
#define JET_paramPreferredMaxOpenTables         7
#if ( JET_VERSION >= 0x0600 )
#define JET_paramCachedClosedTables             125
#endif
#define JET_paramMaxCursors                     8
#define JET_paramMaxVerPages                    9
#define JET_paramPreferredVerPages              63
#if ( JET_VERSION >= 0x0501 )
#define JET_paramGlobalMinVerPages              81
#define JET_paramVersionStoreTaskQueueMax       105
#endif
#define JET_paramMaxTemporaryTables             10
#define JET_paramLogFileSize                    11
#define JET_paramLogBuffers                     12
#define JET_paramWaitLogFlush                   13
#define JET_paramLogCheckpointPeriod            14
#define JET_paramLogWaitingUserMax              15
#define JET_paramCommitDefault                  16
#define JET_paramCircularLog                    17
#define JET_paramDbExtensionSize                18
#define JET_paramPageTempDBMin                  19
#define JET_paramPageFragment                   20
#define JET_paramPageReadAheadMax               21
#if ( JET_VERSION >= 0x0600 )
#define JET_paramEnableFileCache                126
#define JET_paramVerPageSize                    128
#define JET_paramConfiguration                  129
#define JET_paramEnableAdvanced                 130
#define JET_paramMaxColtyp                      131
#endif

#define JET_paramBatchIOBufferMax               22
#define JET_paramCacheSize                      41
#define JET_paramCacheSizeMin                   60
#define JET_paramCacheSizeMax                   23
#define JET_paramCheckpointDepthMax             24
#define JET_paramLRUKCorrInterval               25
#define JET_paramLRUKHistoryMax                 26
#define JET_paramLRUKPolicy                     27
#define JET_paramLRUKTimeout                    28
#define JET_paramLRUKTrxCorrInterval            29
#define JET_paramOutstandingIOMax               30
#define JET_paramStartFlushThreshold            31
#define JET_paramStopFlushThreshold             32
#define JET_paramTableClassName                 33
#define JET_paramIdleFlushTime                  106
#define JET_paramVAReserve                      109
#define JET_paramDBAPageAvailMin                120
#define JET_paramDBAPageAvailThreshold          121
#define JET_paramDBAK1                          122
#define JET_paramDBAK2                          123
#define JET_paramMaxRandomIOSize                124
#if ( JET_VERSION >= 0x0600 )
#define JET_paramEnableViewCache                127
#define JET_paramCheckpointIOMax                135
#endif


#if ( JET_VERSION >= 0x0600 )
#define JET_paramTableClass1Name                137
#define JET_paramTableClass2Name                138
#define JET_paramTableClass3Name                139
#define JET_paramTableClass4Name                140
#define JET_paramTableClass5Name                141
#define JET_paramTableClass6Name                142
#define JET_paramTableClass7Name                143
#define JET_paramTableClass8Name                144
#define JET_paramTableClass9Name                145
#define JET_paramTableClass10Name               146
#define JET_paramTableClass11Name               147
#define JET_paramTableClass12Name               148
#define JET_paramTableClass13Name               149
#define JET_paramTableClass14Name               150
#define JET_paramTableClass15Name               151
#endif

#if ( JET_VERSION >= 0x0A01 )
#define JET_paramTableClass16Name               196
#define JET_paramTableClass17Name               197
#define JET_paramTableClass18Name               198
#define JET_paramTableClass19Name               199
#define JET_paramTableClass20Name               200
#define JET_paramTableClass21Name               201
#define JET_paramTableClass22Name               202
#define JET_paramTableClass23Name               203
#define JET_paramTableClass24Name               204
#define JET_paramTableClass25Name               205
#define JET_paramTableClass26Name               206
#define JET_paramTableClass27Name               207
#define JET_paramTableClass28Name               208
#define JET_paramTableClass29Name               209
#define JET_paramTableClass30Name               210
#define JET_paramTableClass31Name               211
#endif

#define JET_paramIOPriority                     152

#define JET_paramRecovery                       34
#define JET_paramOnLineCompact                  35
#define JET_paramEnableOnlineDefrag             35

#define JET_paramAssertAction                   36
#define JET_paramPrintFunction                  37
#define JET_paramTransactionLevel               38
#define JET_paramRFS2IOsPermitted               39
#define JET_paramRFS2AllocsPermitted            40
#define JET_paramCacheRequests                  42
#define JET_paramCacheHits                      43

#define JET_paramCheckFormatWhenOpenFail        44
#define JET_paramEnableTempTableVersioning      46
#define JET_paramIgnoreLogVersion               47
#define JET_paramDeleteOldLogs                  48
#define JET_paramEventSourceKey                 49
#define JET_paramNoInformationEvent             50
#if ( JET_VERSION >= 0x0501 )
#define JET_paramEventLoggingLevel              51
#define JET_paramDeleteOutOfRangeLogs           52
#define JET_paramAccessDeniedRetryPeriod        53
#endif


#define JET_paramEnableIndexChecking            45
#if ( JET_VERSION >= 0x0502 )
#define JET_paramEnableIndexCleanup             54
#endif

#if ( JET_VERSION >= 0x0A01 )
#define JET_paramFlight_SmoothIoTestPermillage  55
#define JET_paramFlight_ElasticWaypointLatency  56
#define JET_paramFlight_SynchronousLVCleanup    57
#define JET_paramFlight_RBSRevertIOUrgentLevel  58
#define JET_paramFlight_EnableXpress10Compression 59
#endif

#define JET_paramLogFileFailoverPath            61
#define JET_paramEnableImprovedSeekShortcut     62
#define JET_paramDatabasePageSize               64
#if ( JET_VERSION >= 0x0501 )
#define JET_paramDisableCallbacks               65
#endif
#if ( JET_VERSION >= 0x0501 )
#define JET_paramAbortRetryFailCallback         108

#define JET_paramBackupChunkSize                66
#define JET_paramBackupOutstandingReads         67

#define JET_paramLogFileCreateAsynch            69
#endif
#define JET_paramErrorToString                  70
#if ( JET_VERSION >= 0x0501 )
#define JET_paramZeroDatabaseDuringBackup       71
#endif
#define JET_paramUnicodeIndexDefault            72
#if ( JET_VERSION >= 0x0501 )
#define JET_paramRuntimeCallback                73
#define JET_paramFlight_EnableReattachRaceBugFix 74
#define JET_paramEnableSortedRetrieveColumns    76
#endif
#define JET_paramCleanupMismatchedLogFiles      77
#if ( JET_VERSION >= 0x0501 )
#define JET_paramRecordUpgradeDirtyLevel        78
#define JET_paramRecoveryCurrentLogfile         79
#define JET_paramReplayingReplicatedLogfiles    80
#define JET_paramOSSnapshotTimeout              82

#if ( JET_VERSION >= 0x0A01 )

#define JET_paramFlight_NewQueueOptions                         84
#define JET_paramFlight_ConcurrentMetedOps                      85
#define JET_paramFlight_LowMetedOpsThreshold                    86
#define JET_paramFlight_MetedOpStarvedThreshold                 87

#define JET_paramFlight_EnableShrinkArchiving                   89

#define JET_paramFlight_EnableBackupDuringRecovery              90

#define JET_paramFlight_RBSRollIntervalSec                      91
#define JET_paramFlight_RBSMaxRequiredRange                     92
#define JET_paramFlight_RBSCleanupEnabled                       93
#define JET_paramFlight_RBSLowDiskSpaceThresholdGb              94
#define JET_paramFlight_RBSMaxSpaceWhenLowDiskSpaceGb           95
#define JET_paramFlight_RBSMaxTimeSpanSec                       96
#define JET_paramFlight_RBSCleanupIntervalMinSec                97

#endif

#endif

#define JET_paramExceptionAction                98
#define JET_paramEventLogCache                  99

#if ( JET_VERSION >= 0x0501 )
#define JET_paramCreatePathIfNotExist           100
#define JET_paramPageHintCacheSize              101
#define JET_paramOneDatabasePerSession          102
#define JET_paramMaxDatabasesPerInstance        103
#define JET_paramMaxInstances                   104
#define JET_paramDisablePerfmon                 107

#define JET_paramIndexTuplesLengthMin           110
#define JET_paramIndexTuplesLengthMax           111
#define JET_paramIndexTuplesToIndexMax          112
#endif

#if ( JET_VERSION >= 0x0502 )
#define JET_paramAlternateDatabaseRecoveryPath  113
#endif

#if ( JET_VERSION >= 0x0600 )
#define JET_paramIndexTupleIncrement            132
#define JET_paramIndexTupleStart                133
#define JET_paramKeyMost                        134
#define JET_paramLegacyFileNames                136
#define JET_paramEnablePersistedCallbacks       156
#endif

#if ( JET_VERSION >= 0x0601 )
#define JET_paramWaypointLatency                153
#define JET_paramCheckpointTooDeep              154
#define JET_paramDisableBlockVerification       155
#define JET_paramAggressiveLogRollover          157
#define JET_paramPeriodicLogRolloverLLR         158
#define JET_paramUsePageDependencies            159
#define JET_paramDefragmentSequentialBTrees     160
#define JET_paramDefragmentSequentialBTreesDensityCheckFrequency    161
#define JET_paramIOThrottlingTimeQuanta         162
#define JET_paramLVChunkSizeMost                163
#define JET_paramMaxCoalesceReadSize            164
#define JET_paramMaxCoalesceWriteSize           165
#define JET_paramMaxCoalesceReadGapSize         166
#define JET_paramMaxCoalesceWriteGapSize        167
#define JET_paramEnableHaPublish                168
#define JET_paramEnableDBScanInRecovery         169
#define JET_paramDbScanThrottle                 170
#define JET_paramDbScanIntervalMinSec           171
#define JET_paramDbScanIntervalMaxSec           172
#define JET_paramEmitLogDataCallback            173
#define JET_paramEmitLogDataCallbackCtx         174
#endif

#if ( JET_VERSION >= 0x0602 )
#define JET_paramEnableExternalAutoHealing      175
#define JET_paramPatchRequestTimeout            176

#define JET_paramCachePriority                  177

#define JET_paramMaxTransactionSize             178
#define JET_paramPrereadIOMax                   179
#define JET_paramEnableDBScanSerialization      180
#define JET_paramHungIOThreshold                181
#define JET_paramHungIOActions                  182
#define JET_paramMinDataForXpress               183
#endif

#if ( JET_VERSION >= 0x0603 )
#define JET_paramEnableShrinkDatabase           184



#endif

#if ( JET_VERSION >= 0x0602 )
#define JET_paramProcessFriendlyName            186
#define JET_paramDurableCommitCallback          187
#endif

#if ( JET_VERSION >= 0x0603 )
#define JET_paramEnableSqm                      188
#endif

#if ( JET_VERSION >= 0x0A00 )

#define JET_paramConfigStoreSpec                189

#define JET_paramStageFlighting                 190
#define JET_paramZeroDatabaseUnusedSpace        191
#define JET_paramDisableVerifications           192

#endif


#if ( JET_VERSION >= 0x0A01 )
#define JET_paramPersistedLostFlushDetection    193
#define JET_paramEngineFormatVersion            194
#define JET_paramReplayThrottlingLevel          195

#define JET_paramBlockCacheConfiguration        212

#define JET_paramRecordSizeMost                 213
#endif

#if ( JET_VERSION >= 0x0A01 )
#define JET_paramUseFlushForWriteDurability     214

#define JET_paramEnableRBS                      215
#define JET_paramRBSFilePath                    216

#endif


#define JET_paramMaxValueInvalid                217

#if ( JET_VERSION >= 0x0A01 )



#define JET_bitIOSessTraceReads             0x01
#define JET_bitIOSessTraceWrites            0x02
#define JET_bitIOSessTraceHDD               0x04
#define JET_bitIOSessTraceSSD               0x08

#endif



#define JET_sesparamCommitDefault           4097
#define JET_sesparamCommitGenericContext    4098
#if ( JET_VERSION >= 0x0A00 )
#define JET_sesparamTransactionLevel        4099
#define JET_sesparamOperationContext        4100
#define JET_sesparamCorrelationID           4101

#if ( JET_VERSION >= 0x0A01 )
#define JET_sesparamCachePriority           4102

#define JET_sesparamClientComponentDesc     4103
#define JET_sesparamClientActionDesc        4104
#define JET_sesparamClientActionContextDesc 4105
#define JET_sesparamClientActivityId        4106
#define JET_sesparamIOSessTraceFlags        4107
#define JET_sesparamIOPriority              4108

#define JET_sesparamCommitContextContainsCustomerData   4109
#endif

#define JET_sesparamMaxValueInvalid         4110

typedef struct
{
    unsigned long   ulUserID;
    unsigned char   nOperationID;
    unsigned char   nOperationType;
    unsigned char   nClientType;
    unsigned char   fFlags;
} JET_OPERATIONCONTEXT;
#endif

#if ( JET_VERSION >= 0x0600 )



#define JET_bitESE98FileNames           0x00000001
#define JET_bitEightDotThreeSoftCompat  0x00000002
#endif



#define JET_bitHungIOEvent                  0x00000001

#define JET_bitHungIOCancel                 0x00000002
#define JET_bitHungIODebug                  0x00000004
#define JET_bitHungIOEnforce                0x00000008
#define JET_bitHungIOTimeout                0x00000010



#define JET_bitPersistedLostFlushDetectionEnabled           0x00000001
#define JET_bitPersistedLostFlushDetectionCreateNew         0x00000002
#define JET_bitPersistedLostFlushDetectionFailOnRuntimeOnly 0x00000004



#define JET_ReplayThrottlingNone            0
#define JET_ReplayThrottlingSleep           1


#if ( JET_VERSION >= 0x0603 )
#define JET_bitShrinkDatabaseOff            0x0
#define JET_bitShrinkDatabaseOn             0x1
#define JET_bitShrinkDatabaseRealtime       0x2


#define JET_bitShrinkDatabasePeriodically   0x8000

#define JET_bitShrinkDatabaseTrim           0x1

#endif



#define JET_bitReplayReplicatedLogFiles     0x00000001
#if ( JET_VERSION >= 0x0501 )
#define JET_bitReplayIgnoreMissingDB        0x00000004
#endif
#if ( JET_VERSION >= 0x0600 )
#define JET_bitRecoveryWithoutUndo          0x00000008
#define JET_bitTruncateLogsAfterRecovery    0x00000010
#define JET_bitReplayMissingMapEntryDB      0x00000020
#define JET_bitLogStreamMustExist           0x00000040
#endif
#if ( JET_VERSION >= 0x0601 )
#define JET_bitReplayIgnoreLostLogs         0x00000080
#endif
#if ( JET_VERSION >= 0x0602 )
#define JET_bitAllowMissingCurrentLog       0x00000100
#define JET_bitAllowSoftRecoveryOnBackup    0x00000200
#define JET_bitSkipLostLogsEvent            0x00000400
#define JET_bitExternalRecoveryControl      0x00000800
#define JET_bitKeepDbAttachedAtEndOfRecovery 0x00001000
#endif
#if ( JET_VERSION >= 0x0A01 )

#define JET_bitReplayIgnoreLogRecordsBeforeMinRequiredLog 0x00002000
#define JET_bitReplayInferCheckpointFromRstmapDbs 0x00004000



#define JET_bitRestoreMapIgnoreWhenMissing  0x00000001
#define JET_bitRestoreMapFailWhenMissing    0x00000002
#define JET_bitRestoreMapOverwriteOnCreate  0x00000004

#endif




#define JET_bitTermComplete             0x00000001
#define JET_bitTermAbrupt               0x00000002
#define JET_bitTermStopBackup           0x00000004
#if ( JET_VERSION >= 0x0601 )
#define JET_bitTermDirty                0x00000008
#endif
#if ( JET_VERSION >= 0x0602 )
#endif



#define JET_bitIdleFlushBuffers         0x00000001
#define JET_bitIdleCompact              0x00000002
#define JET_bitIdleStatus               0x00000004
#define JET_bitIdleVersionStoreTest     0x00000008
#if ( JET_VERSION >= 0x0603 )
#define JET_bitIdleCompactAsync         0x00000010
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitIdleAvailBuffersStatus   0x00000020
#define JET_bitIdleWaitForAsyncActivity 0x00000040
#endif
#endif



#define JET_bitForceSessionClosed       0x00000001



#define JET_bitDbReadOnly               0x00000001
#define JET_bitDbExclusive              0x00000002
#define JET_bitDbSingleExclusive        0x00000002
#define JET_bitDbDeleteCorruptIndexes   0x00000010
#define JET_bitDbRebuildCorruptIndexes  0x00000020
#if ( JET_VERSION >= 0x0502 )
#define JET_bitDbDeleteUnicodeIndexes   0x00000400
#endif
#if ( JET_VERSION >= 0x0501 )
#define JET_bitDbUpgrade                0x00000200
#endif
#if ( JET_VERSION >= 0x0601 )
#define JET_bitDbEnableBackgroundMaintenance    0x00000800
#endif
#if ( JET_VERSION >= 0x0602 )
#define JET_bitDbPurgeCacheOnAttach     0x00001000
#endif
#define bitDbOpenForRecovery            0x00002000



#if ( JET_VERSION >= 0x0501 )
#define JET_bitForceDetach                  0x00000001
#define JET_bitForceCloseAndDetach          (0x00000002 | JET_bitForceDetach)
#endif






#define JET_bitDbRecoveryOff            0x00000008
#define JET_bitDbVersioningOff          0x00000040
#define JET_bitDbShadowingOff           0x00000080
#define JET_bitDbCreateStreamingFile    0x00000100
#if ( JET_VERSION >= 0x0501 )
#define JET_bitDbOverwriteExisting      0x00000200
#endif
#define bitCreateDbImplicitly           0x00000400



#define JET_bitBackupIncremental        0x00000001
#define JET_bitKeepOldLogs              0x00000002
#define JET_bitBackupAtomic             0x00000004
#define JET_bitBackupFullWithAllLogs    0x00000008
#if ( JET_VERSION >= 0x0501 )
#define JET_bitBackupSnapshot           0x00000010
#endif
#if ( JET_VERSION >= 0x0601 )
#define JET_bitBackupSurrogate          0x00000020
#endif
#define JET_bitInternalCopy         0x00000040



#if ( JET_VERSION >= 0x0501 )
#define JET_bitBackupEndNormal              0x0001
#define JET_bitBackupEndAbort               0x0002
#endif
#if ( JET_VERSION >= 0x0600 )
#define JET_bitBackupTruncateDone           0x0100
#endif
#if ( JET_VERSION >= 0x0601 )
#define JET_bitBackupNoDbHeaderUpdate       0x0200
#endif



#define JET_dbidNil         ((JET_DBID) 0xFFFFFFFF)
#define JET_dbidNoValid     ((JET_DBID) 0xFFFFFFFE)



#define JET_bitTableCreateFixedDDL          0x00000001
#define JET_bitTableCreateTemplateTable     0x00000002
#if ( JET_VERSION >= 0x0501 )
#define JET_bitTableCreateNoFixedVarColumnsInDerivedTables  0x00000004
#endif
#if JET_VERSION >= 0x0A00
#define JET_bitTableCreateImmutableStructure    0x00000008
#endif
#define JET_bitTableCreateSystemTable       0x80000000




#define JET_bitColumnFixed              0x00000001
#define JET_bitColumnTagged             0x00000002
#define JET_bitColumnNotNULL            0x00000004
#define JET_bitColumnVersion                0x00000008
#define JET_bitColumnAutoincrement      0x00000010
#define JET_bitColumnUpdatable          0x00000020
#define JET_bitColumnTTKey              0x00000040
#define JET_bitColumnTTDescending       0x00000080
#define JET_bitColumnMultiValued            0x00000400
#define JET_bitColumnEscrowUpdate       0x00000800
#define JET_bitColumnUnversioned        0x00001000
#if ( JET_VERSION >= 0x0501 )
#define JET_bitColumnMaybeNull          0x00002000
#define JET_bitColumnFinalize           0x00004000
#define JET_bitColumnUserDefinedDefault 0x00008000
#define JET_bitColumnRenameConvertToPrimaryIndexPlaceholder 0x00010000
#endif
#if ( JET_VERSION >= 0x0502 )
#define JET_bitColumnDeleteOnZero       0x00020000
#endif
#if ( JET_VERSION >= 0x0600 )
#define JET_bitColumnVariable           0x00040000
#endif
#if ( JET_VERSION >= 0x0601 )
#define JET_bitColumnCompressed         0x00080000
#endif
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitColumnEncrypted          0x00100000
#endif

#if ( JET_VERSION >= 0x0501 )
#define JET_bitDeleteColumnIgnoreTemplateColumns    0x00000001
#endif




#define JET_bitMoveFirst                0x00000000
#define JET_bitMoveBeforeFirst          0x00000001
#define JET_bitNoMove                   0x00000002



#define JET_bitNewKey                   0x00000001
#define JET_bitStrLimit                 0x00000002
#define JET_bitSubStrLimit              0x00000004
#define JET_bitNormalizedKey            0x00000008
#define JET_bitKeyDataZeroLength        0x00000010
#if ( JET_VERSION >= 0x0501 )
#define JET_bitKeyOverridePrimaryIndexPlaceholder   0x00000020
#endif

#define JET_maskLimitOptions            0x00000f00
#if ( JET_VERSION >= 0x0501 )
#define JET_bitFullColumnStartLimit     0x00000100
#define JET_bitFullColumnEndLimit       0x00000200
#define JET_bitPartialColumnStartLimit  0x00000400
#define JET_bitPartialColumnEndLimit    0x00000800
#endif



#define JET_bitRangeInclusive           0x00000001
#define JET_bitRangeUpperLimit          0x00000002
#define JET_bitRangeInstantDuration     0x00000004
#define JET_bitRangeRemove              0x00000008



#define JET_bitReadLock                 0x00000001
#define JET_bitWriteLock                0x00000002



#define JET_MoveFirst                   (0x80000000)
#define JET_MovePrevious                (-1)
#define JET_MoveNext                    (+1)
#define JET_MoveLast                    (0x7fffffff)



#define JET_bitMoveKeyNE                0x00000001



#define JET_bitSeekEQ                   0x00000001
#define JET_bitSeekLT                   0x00000002
#define JET_bitSeekLE                   0x00000004
#define JET_bitSeekGE                   0x00000008
#define JET_bitSeekGT                   0x00000010
#define JET_bitSetIndexRange            0x00000020
#if ( JET_VERSION >= 0x0502 )
#define JET_bitCheckUniqueness          0x00000040
#endif

#if ( JET_VERSION >= 0x0501 )
#define JET_bitBookmarkPermitVirtualCurrency    0x00000001
#endif


#define JET_bitIndexColumnMustBeNull    0x00000001
#define JET_bitIndexColumnMustBeNonNull 0x00000002


#define JET_bitRecordInIndex            0x00000001
#define JET_bitRecordNotInIndex         0x00000002



#define JET_bitIndexUnique              0x00000001
#define JET_bitIndexPrimary             0x00000002
#define JET_bitIndexDisallowNull        0x00000004
#define JET_bitIndexIgnoreNull          0x00000008
#define JET_bitIndexClustered40         0x00000010
#define JET_bitIndexIgnoreAnyNull       0x00000020
#define JET_bitIndexIgnoreFirstNull     0x00000040
#define JET_bitIndexLazyFlush           0x00000080
#define JET_bitIndexEmpty               0x00000100
#define JET_bitIndexUnversioned         0x00000200
#define JET_bitIndexSortNullsHigh       0x00000400
#define JET_bitIndexUnicode             0x00000800
#if ( JET_VERSION >= 0x0501 )
#define JET_bitIndexTuples              0x00001000
#endif
#if ( JET_VERSION >= 0x0502 )
#define JET_bitIndexTupleLimits         0x00002000
#endif
#if ( JET_VERSION >= 0x0600 )
#define JET_bitIndexCrossProduct        0x00004000
#define JET_bitIndexKeyMost             0x00008000
#define JET_bitIndexDisallowTruncation  0x00010000
#define JET_bitIndexNestedTable         0x00020000
#endif
#if ( JET_VERSION >= 0x0602 )
#define JET_bitIndexDotNetGuid          0x00040000
#endif
#if ( JET_VERSION >= 0x0A00 )
#define JET_bitIndexImmutableStructure  0x00080000
#endif





#define JET_bitKeyAscending             0x00000000
#define JET_bitKeyDescending            0x00000001



#define JET_bitTableDenyWrite           0x00000001
#define JET_bitTableDenyRead            0x00000002
#define JET_bitTableReadOnly            0x00000004
#define JET_bitTableUpdatable           0x00000008
#define JET_bitTablePermitDDL           0x00000010
#define JET_bitTableNoCache             0x00000020
#define JET_bitTablePreread             0x00000040
#define JET_bitTableOpportuneRead       0x00000080
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitTableAllowOutOfDate      0x00000100
#endif
#define JET_bitTableSequential          0x00008000
#define JET_bitTableTryPurgeOnClose     0x01000000
#define JET_bitTableDelete              0x10000000
#define JET_bitTableCreate              0x20000000
#define bitTableUpdatableDuringRecovery 0x40000000

#define JET_bitTableClassMask       0x001F0000
#define JET_bitTableClassNone       0x00000000
#define JET_bitTableClass1          0x00010000
#define JET_bitTableClass2          0x00020000
#define JET_bitTableClass3          0x00030000
#define JET_bitTableClass4          0x00040000
#define JET_bitTableClass5          0x00050000
#define JET_bitTableClass6          0x00060000
#define JET_bitTableClass7          0x00070000
#define JET_bitTableClass8          0x00080000
#define JET_bitTableClass9          0x00090000
#define JET_bitTableClass10         0x000A0000
#define JET_bitTableClass11         0x000B0000
#define JET_bitTableClass12         0x000C0000
#define JET_bitTableClass13         0x000D0000
#define JET_bitTableClass14         0x000E0000
#define JET_bitTableClass15         0x000F0000

#if ( JET_VERSION >= 0x0A01 )
#define JET_bitTableClass16         0x00100000
#define JET_bitTableClass17         0x00110000
#define JET_bitTableClass18         0x00120000
#define JET_bitTableClass19         0x00130000
#define JET_bitTableClass20         0x00140000
#define JET_bitTableClass21         0x00150000
#define JET_bitTableClass22         0x00160000
#define JET_bitTableClass23         0x00170000
#define JET_bitTableClass24         0x00180000
#define JET_bitTableClass25         0x00190000
#define JET_bitTableClass26         0x001A0000
#define JET_bitTableClass27         0x001B0000
#define JET_bitTableClass28         0x001C0000
#define JET_bitTableClass29         0x001D0000
#define JET_bitTableClass30         0x001E0000
#define JET_bitTableClass31         0x001F0000
#endif

#if ( JET_VERSION >= 0x0501 )
#define JET_bitLSReset              0x00000001
#define JET_bitLSCursor             0x00000002
#define JET_bitLSTable              0x00000004

#define JET_LSNil                   (~(JET_LS)0)
#endif

#if ( JET_VERSION >= 0x0601 )


#define JET_bitPrereadForward       0x00000001
#define JET_bitPrereadBackward      0x00000002
#if ( JET_VERSION >= 0x0602 )
#define JET_bitPrereadFirstPage     0x00000004
#define JET_bitPrereadNormalizedKey 0x00000008
#define bitPrereadSingletonRanges   0x00000010
#define bitPrereadDoNotDoOLD2       0x00000020
#endif
#endif



#define JET_bitTTIndexed            0x00000001
#define JET_bitTTUnique             0x00000002
#define JET_bitTTUpdatable          0x00000004
#define JET_bitTTScrollable         0x00000008
#define JET_bitTTSortNullsHigh      0x00000010
#define JET_bitTTForceMaterialization       0x00000020
#if ( JET_VERSION >= 0x0501 )
#define JET_bitTTErrorOnDuplicateInsertion  JET_bitTTForceMaterialization
#endif
#if ( JET_VERSION >= 0x0502 )
#define JET_bitTTForwardOnly        0x00000040
#endif
#if ( JET_VERSION >= 0x0601 )
#define JET_bitTTIntrinsicLVsOnly   0x00000080
#endif
#if ( JET_VERSION >= 0x0602 )
#define JET_bitTTDotNetGuid         0x00000100
#endif




#define JET_bitSetAppendLV                  0x00000001
#define JET_bitSetOverwriteLV               0x00000004
#define JET_bitSetSizeLV                    0x00000008
#define JET_bitSetZeroLength                0x00000020
#define JET_bitSetSeparateLV                0x00000040
#define JET_bitSetUniqueMultiValues         0x00000080
#define JET_bitSetUniqueNormalizedMultiValues   0x00000100
#if ( JET_VERSION >= 0x0501 )
#define JET_bitSetRevertToDefaultValue      0x00000200
#define JET_bitSetIntrinsicLV               0x00000400
#endif
#if ( JET_VERSION >= 0x0601 )
#define JET_bitSetUncompressed              0x00010000
#define JET_bitSetCompressed                0x00020000
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitSetContiguousLV              0x00040000
#endif
#endif


#if ( JET_VERSION >= 0x0601 )


#define JET_bitSpaceHintsUtilizeParentSpace         0x00000001
#define JET_bitCreateHintAppendSequential           0x00000002
#define JET_bitCreateHintHotpointSequential         0x00000004
#define JET_bitRetrieveHintReserve1                 0x00000008
#define JET_bitRetrieveHintTableScanForward         0x00000010
#define JET_bitRetrieveHintTableScanBackward        0x00000020
#define JET_bitRetrieveHintReserve2                 0x00000040
#define JET_bitRetrieveHintReserve3                 0x00000080
#define JET_bitDeleteHintTableSequential            0x00000100
#endif

#if ( JET_VERSION >= 0x0A01 )
#define JET_bitSpaceHintsUtilizeExactExtents        0x00000200
#endif




typedef struct
{
    JET_COLUMNID            columnid;
    const void              *pvData;
    unsigned long           cbData;
    JET_GRBIT               grbit;
    unsigned long           ibLongValue;
    unsigned long           itagSequence;
    JET_ERR                 err;
} JET_SETCOLUMN;

#if ( JET_VERSION >= 0x0501 )
typedef struct
{
    unsigned long   paramid;
    JET_API_PTR     lParam;
    const char      *sz;
    JET_ERR         err;
} JET_SETSYSPARAM_A;

typedef struct
{
    unsigned long   paramid;
    JET_API_PTR     lParam;
    const WCHAR     *sz;
    JET_ERR         err;
} JET_SETSYSPARAM_W;


#ifdef JET_UNICODE
#define JET_SETSYSPARAM JET_SETSYSPARAM_W
#else
#define JET_SETSYSPARAM JET_SETSYSPARAM_A
#endif

#endif



#define JET_prepInsert                      0
#define JET_prepReplace                     2
#define JET_prepCancel                      3
#define JET_prepReplaceNoLock               4
#define JET_prepInsertCopy                  5
#if ( JET_VERSION >= 0x0501 )
#define JET_prepInsertCopyDeleteOriginal    7
#endif
#define JET_prepReadOnlyCopy                8
#if ( JET_VERSION >= 0x0603 )
#define JET_prepInsertCopyReplaceOriginal   9
#endif

#if ( JET_VERSION >= 0x0603 )
#define JET_sqmDisable                      0
#define JET_sqmEnable                       1
#define JET_sqmFromCEIP                     2
#endif

#if ( JET_VERSION >= 0x0502 )
#define JET_bitUpdateCheckESE97Compatibility    0x00000001
#endif
#define JET_bitUpdateNoVersion                  0x00000002


#define JET_bitEscrowNoRollback             0x0001



#define JET_bitRetrieveCopy                 0x00000001
#define JET_bitRetrieveFromIndex            0x00000002
#define JET_bitRetrieveFromPrimaryBookmark  0x00000004
#define JET_bitRetrieveTag                  0x00000008
#define JET_bitRetrieveNull                 0x00000010
#define JET_bitRetrieveIgnoreDefault        0x00000020
#define JET_bitRetrieveLongId               0x00000040
#define JET_bitRetrieveLongValueRefCount    0x00000080



#if ( JET_VERSION >= 0x0600 )
#define JET_bitRetrieveTuple                0x00000800
#endif
#if ( JET_VERSION >= 0x0601 )
#define JET_bitRetrievePageNumber           0x00001000
#endif

#define JET_bitRetrieveCopyIntrinsic        0x00002000

#define JET_bitRetrievePrereadOnly          0x00004000
#define JET_bitRetrievePrereadMany          0x00008000
#if ( JET_VERSION >= 0x0603 )
#define JET_bitRetrievePhysicalSize         0x00010000
#endif
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitRetrieveAsRefIfNotInRecord   0x00020000
#endif


#if ( JET_VERSION >= 0x0602 )

#define JET_bitZeroLength                   0x00000001
#endif



typedef struct
{
    JET_COLUMNID        columnid;
    void                *pvData;
    unsigned long       cbData;
    unsigned long       cbActual;
    JET_GRBIT           grbit;
    unsigned long       ibLongValue;
    unsigned long       itagSequence;
    JET_COLUMNID        columnidNextTagged;
    JET_ERR             err;
} JET_RETRIEVECOLUMN;


typedef struct
{
    JET_COLUMNID            columnid;
    unsigned short          cMultiValues;

    union
    {
        unsigned short      usFlags;
        struct
        {
            unsigned short  fLongValue:1;
            unsigned short  fDefaultValue:1;
            unsigned short  fNullOverride:1;
            unsigned short  fDerived:1;
        };
    };
} JET_RETRIEVEMULTIVALUECOUNT;


#if ( JET_VERSION >= 0x0501 )


#define JET_bitEnumerateCopy                        JET_bitRetrieveCopy
#define JET_bitEnumerateIgnoreDefault               JET_bitRetrieveIgnoreDefault
#define JET_bitEnumerateLocal                       0x00010000
#define JET_bitEnumeratePresenceOnly                0x00020000
#define JET_bitEnumerateTaggedOnly                  0x00040000
#define JET_bitEnumerateCompressOutput              0x00080000
#if ( JET_VERSION >= 0x0502 )
#define JET_bitEnumerateIgnoreUserDefinedDefault    0x00100000
#endif
#if ( JET_VERSION >= 0x0601 )
#define JET_bitEnumerateInRecordOnly                0x00200000
#endif
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitEnumerateAsRefIfNotInRecord          0x00400000
#endif



typedef struct
{
    JET_COLUMNID            columnid;
    unsigned long           ctagSequence;
    unsigned long*          rgtagSequence;
} JET_ENUMCOLUMNID;

typedef struct
{
    unsigned long           itagSequence;
    JET_ERR                 err;
    unsigned long           cbData;
    void*                   pvData;
} JET_ENUMCOLUMNVALUE;

typedef struct
{
    JET_COLUMNID            columnid;
    JET_ERR                 err;
    union
    {
        struct
        {
            unsigned long           cEnumColumnValue;
            JET_ENUMCOLUMNVALUE*    rgEnumColumnValue;
        };
        struct
        {
            unsigned long           cbData;
            void*                   pvData;
        };
    };
} JET_ENUMCOLUMN;



typedef void* (JET_API *JET_PFNREALLOC)(
    _In_opt_ void *     pvContext,
    _In_opt_ void *     pv,
    _In_ unsigned long  cb );

#endif

#if ( JET_VERSION >= 0x0A01 )



#define JET_bitStreamForward            0x00000001
#define JET_bitStreamBackward           0x00000002
#define JET_bitStreamColumnReferences   0x00000004

#endif


#if ( JET_VERSION >= 0x0600 )


#define JET_bitRecordSizeInCopyBuffer           0x00000001
#define JET_bitRecordSizeRunningTotal           0x00000002
#define JET_bitRecordSizeLocal                  0x00000004
#define JET_bitRecordSizeIncludeDefaultValues   0x00000008
#define JET_bitRecordSizeSecondaryIndexKeyOnly  0x00000010
#define JET_bitRecordSizeIntrinsicPhysicalOnly  0x00000020
#define JET_bitRecordSizeExtrinsicLogicalOnly   0x00000040



typedef struct
{
    unsigned __int64    cbData;
    unsigned __int64    cbLongValueData;
    unsigned __int64    cbOverhead;
    unsigned __int64    cbLongValueOverhead;
    unsigned __int64    cNonTaggedColumns;
    unsigned __int64    cTaggedColumns;
    unsigned __int64    cLongValues;
    unsigned __int64    cMultiValues;
} JET_RECSIZE;
#endif


#if ( JET_VERSION >= 0x0600 )
typedef struct tagJET_PAGEINFO
{
    unsigned long       pgno;
    unsigned long       fPageIsInitialized:1;
    unsigned long       fCorrectableError:1;
    unsigned __int64    checksumActual;
    unsigned __int64    checksumExpected;
    unsigned __int64    dbtime;
    unsigned __int64    structureChecksum;
    unsigned __int64    flags;
} JET_PAGEINFO;
#endif

#if ( JET_VERSION >= 0x0601 )
typedef struct
{
    unsigned __int64    cbData;
    unsigned __int64    cbLongValueData;
    unsigned __int64    cbOverhead;
    unsigned __int64    cbLongValueOverhead;
    unsigned __int64    cNonTaggedColumns;
    unsigned __int64    cTaggedColumns;
    unsigned __int64    cLongValues;
    unsigned __int64    cMultiValues;
    unsigned __int64    cCompressedColumns;
    unsigned __int64    cbDataCompressed;
    unsigned __int64    cbLongValueDataCompressed;
} JET_RECSIZE2;
#endif
#if ( JET_VERSION >= 0x0A01 )
typedef struct
{
    unsigned __int64    cbData;
    unsigned __int64    cbLongValueData;
    unsigned __int64    cbOverhead;
    unsigned __int64    cbLongValueOverhead;
    unsigned __int64    cNonTaggedColumns;
    unsigned __int64    cTaggedColumns;
    unsigned __int64    cLongValues;
    unsigned __int64    cMultiValues;
    unsigned __int64    cCompressedColumns;
    unsigned __int64    cbDataCompressed;
    unsigned __int64    cbLongValueDataCompressed;
    unsigned __int64    cbIntrinsicLongValueData;
    unsigned __int64    cbIntrinsicLongValueDataCompressed;
    unsigned __int64    cIntrinsicLongValues;
    unsigned __int64    cbKey;
} JET_RECSIZE3;

#endif

#if ( JET_VERSION >= 0x0601 )
typedef struct tagJET_PAGEINFO2
{
    JET_PAGEINFO        pageInfo;
    unsigned __int64    rgChecksumActual[ 3 ];
    unsigned __int64    rgChecksumExpected[ 3];
} JET_PAGEINFO2;
#endif


#pragma warning(pop)




#if ( JET_VERSION >= 0x0501 )
#define JET_bitTransactionReadOnly      0x00000001
#endif
#define JET_bitDistributedTransaction   0x00000002
#define bitTransactionWritableDuringRecovery 0x00000004



#define JET_bitCommitLazyFlush          0x00000001
#define JET_bitWaitLastLevel0Commit     0x00000002
#if ( JET_VERSION >= 0x0502 )
#define JET_bitWaitAllLevel0Commit      0x00000008
#endif
#if ( JET_VERSION >= 0x0601 )
#define JET_bitForceNewLog              0x00000010
#endif
#if ( JET_VERSION >= 0x0A00 )
#define JET_bitCommitRedoCallback       0x00000020
#endif

#define JET_bitCommitFlush_OLD          0x00000001
#define JET_bitCommitLazyFlush_OLD      0x00000004
#define JET_bitWaitLastLevel0Commit_OLD 0x00000010



#define JET_bitRollbackAll              0x00000001
#if ( JET_VERSION >= 0x0A00 )
#define JET_bitRollbackRedoCallback     0x00000020
#endif


#if ( JET_VERSION >= 0x0600 )



#define JET_bitIncrementalSnapshot      0x00000001
#define JET_bitCopySnapshot             0x00000002
#define JET_bitContinueAfterThaw        0x00000004
#if ( JET_VERSION >= 0x0601 )
#define JET_bitExplicitPrepare          0x00000008
#endif


#define JET_bitAllDatabasesSnapshot     0x00000001


#define JET_bitAbortSnapshot            0x00000001
#endif

#if ( JET_VERSION >= 0x0601 )



#define JET_bitShadowLogEmitFirstCall           0x00000001
#define JET_bitShadowLogEmitLastCall            0x00000002
#define JET_bitShadowLogEmitCancel              0x00000004
#define JET_bitShadowLogEmitDataBuffers     0x00000008
#define JET_bitShadowLogEmitLogComplete     0x00000010



typedef struct tag_JET_EMITDATACTX
{
    unsigned long               cbStruct;
    unsigned long               dwVersion;
    unsigned __int64            qwSequenceNum;
    JET_GRBIT                   grbitOperationalFlags;
    JET_LOGTIME                 logtimeEmit;
    JET_LGPOS                   lgposLogData;
    unsigned long               cbLogData;
} JET_EMITDATACTX;



typedef JET_ERR (JET_API * JET_PFNEMITLOGDATA)(
    _In_    JET_INSTANCE        instance,
    _In_    JET_EMITDATACTX *   pEmitLogDataCtx,
    _In_    void *              pvLogData,
    _In_    unsigned long       cbLogData,
    _In_    void *              callbackCtx );

#endif



#define JET_DbInfoFilename          0
#define JET_DbInfoConnect           1
#define JET_DbInfoCountry           2
#if ( JET_VERSION >= 0x0501 )
#define JET_DbInfoLCID              3
#endif
#define JET_DbInfoLangid            3
#define JET_DbInfoCp                4
#define JET_DbInfoCollate           5
#define JET_DbInfoOptions           6
#define JET_DbInfoTransactions      7
#define JET_DbInfoVersion           8
#define JET_DbInfoIsam              9
#define JET_DbInfoFilesize          10
#define JET_DbInfoSpaceOwned        11
#define JET_DbInfoSpaceAvailable    12
#define JET_DbInfoUpgrade           13
#define JET_DbInfoMisc              14
#if ( JET_VERSION >= 0x0501 )
#define JET_DbInfoDBInUse           15
#define JET_DbInfoHasSLVFile_Obsolete       16
#define JET_DbInfoPageSize          17
#endif
#define JET_DbInfoStreamingFileSpace_Obsolete       18
#if ( JET_VERSION >= 0x0600 )
#define JET_DbInfoFileType          19
#define JET_DbInfoStreamingFileSize_Obsolete        20
#if ( JET_VERSION >= 0x603 )
#define JET_DbInfoFilesizeOnDisk    21
#endif
#if ( JET_VERSION >= 0x0A01 )
#define dbInfoSpaceShelved          22
#endif
#define JET_DbInfoUseCachedResult   0x40000000



#define JET_LogInfoMisc             0
#if ( JET_VERSION >= 0x0601 )
#define JET_LogInfoMisc2            1
#if ( JET_VERSION >= 0x0A01 )
#define JET_LogInfoMisc3            2
#endif
#endif



#define JET_RBSFileInfoMisc              0


#endif



#define JET_dbstateJustCreated                  1
#define JET_dbstateDirtyShutdown                2
#define JET_dbstateCleanShutdown                3
#define JET_dbstateBeingConverted               4
#if ( JET_VERSION >= 0x0501 )
#define JET_dbstateForceDetach                  5
#endif
#if ( JET_VERSION >= 0x0601 )
#define JET_dbstateIncrementalReseedInProgress  6
#define JET_dbstateDirtyAndPatchedShutdown      7
#endif
#if ( JET_VERSION >= 0x0A01 )
#define JET_dbstateRevertInProgress             8
#endif

#if ( JET_VERSION >= 0x0600 )


#define JET_filetypeUnknown                 0
#define JET_filetypeDatabase                1
#define JET_filetypeStreamingFile           2
#define JET_filetypeLog                     3
#define JET_filetypeCheckpoint              4
#define JET_filetypeTempDatabase            5
#define JET_filetypeFTL                     6
#define JET_filetypeFlushMap                7
#define JET_filetypeCachedFile              8
#define JET_filetypeCachingFile             9
#define JET_filetypeSnapshot                10
#define JET_filetypeRBSRevertCheckpoint     11
#define JET_filetypeMax                     12

#endif



#if ( JET_VERSION >= 0x0A01 )
#define JET_revertstateNone                 0
#define JET_revertstateInProgress           1
#define JET_revertstateCopingLogs           2
#define JET_revertstateCompleted            3
#endif



#if ( JET_VERSION >= 0x0A01 )
#define JET_bitDeleteAllExistingLogs  0x00000001
#endif



#define JET_coltypNil               0
#define JET_coltypBit               1
#define JET_coltypUnsignedByte      2
#define JET_coltypShort             3
#define JET_coltypLong              4
#define JET_coltypCurrency          5
#define JET_coltypIEEESingle        6
#define JET_coltypIEEEDouble        7
#define JET_coltypDateTime          8
#define JET_coltypBinary            9
#define JET_coltypText              10
#define JET_coltypLongBinary        11
#define JET_coltypLongText          12

#if ( JET_VERSION < 0x0501 )
#define JET_coltypMax               13


#endif

#if ( JET_VERSION >= 0x0501 )
#define JET_coltypSLV               13

#if ( JET_VERSION < 0x0600 )
#define JET_coltypMax               14


#endif

#endif

#if ( JET_VERSION >= 0x0600 )
#define JET_coltypUnsignedLong      14
#define JET_coltypLongLong          15
#define JET_coltypGUID              16
#define JET_coltypUnsignedShort     17

#if ( JET_VERSION >= 0x0600 && JET_VERSION <= 0x0603 )
#define JET_coltypMax               18


#endif

#endif

#if ( JET_VERSION >= 0x0A00 )
#define JET_coltypUnsignedLongLong  18
#define JET_coltypMax               19


#endif


#if ( JET_VERSION >= 0x0600 )


#define JET_SessionInfo             0U
#endif



#define JET_ObjInfo                 0U
#define JET_ObjInfoListNoStats      1U
#define JET_ObjInfoList             2U
#define JET_ObjInfoSysTabCursor     3U
#define JET_ObjInfoListACM          4U
#define JET_ObjInfoNoStats          5U
#define JET_ObjInfoSysTabReadOnly   6U
#define JET_ObjInfoRulesLoaded      7U
#define JET_ObjInfoMax              8U



#define JET_TblInfo             0U
#define JET_TblInfoName         1U
#define JET_TblInfoDbid         2U
#define JET_TblInfoMostMany     3U
#define JET_TblInfoRvt          4U
#define JET_TblInfoOLC          5U
#define JET_TblInfoResetOLC     6U
#define JET_TblInfoSpaceUsage   7U
#define JET_TblInfoDumpTable    8U
#define JET_TblInfoSpaceAlloc   9U
#define JET_TblInfoSpaceOwned   10U
#define JET_TblInfoSpaceAvailable       11U
#define JET_TblInfoTemplateTableName    12U
#if ( JET_VERSION >= 0x0A01 )
#define JET_TblInfoLVChunkMax   13U
#define JET_TblInfoEncryptionKey    14U
#endif



#define JET_IdxInfo                 0U
#define JET_IdxInfoList             1U
#define JET_IdxInfoSysTabCursor     2U
#define JET_IdxInfoOLC              3U
#define JET_IdxInfoResetOLC         4U
#define JET_IdxInfoSpaceAlloc       5U
#if ( JET_VERSION >= 0x0501 )
#define JET_IdxInfoLCID             6U
#endif
#define JET_IdxInfoLangid           6U
#define JET_IdxInfoCount            7U
#define JET_IdxInfoVarSegMac        8U
#define JET_IdxInfoIndexId          9U
#if ( JET_VERSION >= 0x0600 )
#define JET_IdxInfoKeyMost          10U
#endif
#if ( JET_VERSION >= 0x0601 )
#define JET_IdxInfoCreateIndex      11U
#define JET_IdxInfoCreateIndex2     12U
#endif
#if ( JET_VERSION >= 0x0602 )
#define JET_IdxInfoCreateIndex3     13U
#define JET_IdxInfoLocaleName       14U
#endif
#if ( JET_VERSION >= 0x0A00 )
#define JET_IdxInfoSortVersion      15U
#define JET_IdxInfoDefinedSortVersion   16U
#define JET_IdxInfoSortId           17U
#endif




#define JET_ColInfo                 0U
#define JET_ColInfoList             1U
#define JET_ColInfoSysTabCursor     3U
#define JET_ColInfoBase             4U
#define JET_ColInfoListCompact      5U
#if ( JET_VERSION >= 0x0501 )
#define JET_ColInfoByColid          6U
#define JET_ColInfoListSortColumnid 7U
#endif
#if ( JET_VERSION >= 0x0600 )
#define JET_ColInfoBaseByColid      8U
#endif

#if ( JET_VERSION >= 0x0600 )

#define JET_ColInfoGrbitNonDerivedColumnsOnly   0x80000000
#define JET_ColInfoGrbitMinimalInfo             0x40000000
#define JET_ColInfoGrbitSortByColumnid          0x20000000
#define JET_ColInfoGrbitCompacting              0x10000000

#endif

#if ( JET_VERSION >= 0x0600 )



#define JET_InstanceMiscInfoLogSignature    0U
#define JET_InstanceMiscInfoCheckpoint      1U

#endif

#if ( JET_VERSION >= 0x0A01 )

#define JET_InstanceMiscInfoRBS             2U

#endif

#if ( JET_VERSION >= 0x0600 )



#define JET_PageInfo                0U
#if ( JET_VERSION >= 0x0601 )
#define JET_PageInfo2           1U
#endif



#if ( JET_VERSION >= 0x0602 )
#define JET_bitPageInfoNoStructureChecksum  0x00000001
#endif

#endif


#if ( JET_VERSION >= 0x0601 )



#define JET_bitTestUninitShrunkPageImage    0x00000001
#define JET_bitPatchingCorruptPage          0x00000002

#endif

#if ( JET_VERSION >= 0x602 )




#endif

#if ( JET_VERSION >= 0x0601 )



#define JET_bitEndDatabaseIncrementalReseedCancel       0x00000001

#endif






#define JET_objtypNil               0
#define JET_objtypTable             1
#define JET_objtypDb                2
#define JET_objtypContainer         3
#define JET_objtypLongRoot          9



#define JET_bitCompactStats             0x00000020
#define JET_bitCompactRepair            0x00000040
#if ( JET_VERSION >= 0x0600 )
#define JET_bitCompactPreserveOriginal  0x00000100
#endif



#define JET_snpRepair                   2
#define JET_snpCompact                  4
#define JET_snpRestore                  8
#define JET_snpBackup                   9
#define JET_snpUpgrade                  10
#if ( JET_VERSION >= 0x0501 )
#define JET_snpScrub                    11
#define JET_snpUpgradeRecordFormat      12
#endif
#if ( JET_VERSION >= 0x0602 )
#define JET_snpRecoveryControl          18
#define JET_snpExternalAutoHealing      19
#endif
#if ( JET_VERSION >= 0x0A01 )
#define JET_snpSpaceCategorization      20
#endif




#define JET_sntBegin            5
#define JET_sntRequirements     7
#define JET_sntProgress         0
#define JET_sntComplete         6
#define JET_sntFail             3

#if ( JET_VERSION >= 0x0602 )


#define JET_sntOpenLog                  1001
#define JET_sntOpenCheckpoint           1002
#define JET_sntMissingLog               1004
#define JET_sntBeginUndo                1005
#define JET_sntNotificationEvent        1006
#define JET_sntSignalErrorCondition     1007
#define JET_sntAttachedDb               1008
#define JET_sntDetachingDb              1009
#define JET_sntCommitCtx                1010

#define JET_sntPagePatchRequest         1101
#define JET_sntCorruptedPage            1102
#endif



#define JET_ExceptionMsgBox     0x0001
#define JET_ExceptionNone       0x0002
#define JET_ExceptionFailFast   0x0004



#define JET_AssertExit              0x0000
#define JET_AssertBreak             0x0001
#define JET_AssertMsgBox            0x0002
#define JET_AssertStop              0x0004
#if ( JET_VERSION >= 0x0601 )
#define JET_AssertSkippableMsgBox   0x0008
#define JET_AssertSkipAll           0x0010
#define JET_AssertCrash             0x0020
#define JET_AssertFailFast          0x0040
#endif


#if ( JET_VERSION >= 0x0501 )
#define JET_OnlineDefragDisable         0x0000
#define JET_OnlineDefragAllOBSOLETE     0x0001
#define JET_OnlineDefragDatabases       0x0002
#define JET_OnlineDefragSpaceTrees      0x0004
#define JET_OnlineDefragStreamingFiles  0x0008
#define JET_OnlineDefragAll             0xffff

#endif




#define ctAccessPage            1
#define ctLatchConflict         2
#define ctSplitRetry            3
#define ctNeighborPageScanned   4
#define ctSplits                5


#if ( JET_VERSION >= 0x0600 )


#define JET_resTagSize          4
#endif

#if ( JET_VERSION >= 0x0602 )

#define JET_ErrorInfoSpecificErr        1U


#define JET_bitResizeDatabaseOnlyGrow               0x00000001
#endif

#if ( JET_VERSION >= 0x0603 )
#define JET_bitResizeDatabaseOnlyShrink             0x00000002



#endif

#if ( JET_VERSION >= 0x0602 )
#define JET_bitStopServiceAll                       0x00000000
#define JET_bitStopServiceBackgroundUserTasks       0x00000002
#define JET_bitStopServiceQuiesceCaches             0x00000004

#define JET_bitStopServiceResume                    0x80000000
#endif

#if ( JET_VERSION >= 0x0A01 )
#define JET_bitStopServiceStopAndEmitLog            0x00000008
#endif

#define JET_bitStageTestEnvLocalMode                    0x04
#define JET_bitStageTestEnvAlphaMode                    0x10
#define JET_bitStageTestEnvBetaMode                     0x40

#define JET_bitStageSelfhostLocalMode                 0x0400
#define JET_bitStageSelfhostAlphaMode                 0x1000
#define JET_bitStageSelfhostBetaMode                  0x4000

#define JET_bitStageProdLocalMode                   0x040000
#define JET_bitStageProdAlphaMode                   0x100000
#define JET_bitStageProdBetaMode                    0x400000

#if ( JET_VERSION >= 0x0A01 )
#define JET_EncryptionAlgorithmAes256       1
#endif










#define JET_errSuccess                       0



#define JET_wrnNyi                          -1


#define JET_errRfsFailure                   -100
#define JET_errRfsNotArmed                  -101
#define JET_errFileClose                    -102
#define JET_errOutOfThreads                 -103
#define JET_errTooManyIO                    -105
#define JET_errTaskDropped                  -106
#define JET_errInternalError                -107
#define errCodeInconsistency                -108
#define errNotFound                         -109
#define wrnSlow                              110
#define wrnLossy                             111

#define JET_errDisabledFunctionality        -112
#define JET_errUnloadableOSFunctionality    -113
#define JET_errDeviceMissing                -114
#define JET_errDeviceMisconfigured          -115
#define JET_errDeviceTimeout                -116
#define errDeviceBusy                       -117
#define JET_errDeviceFailure                -118


#define wrnBFCacheMiss                       200
#define errBFPageNotCached                  -201
#define errBFLatchConflict                  -202
#define errBFPageCached                     -203
#define wrnBFPageFlushPending                204
#define wrnBFPageFault                       205
#define wrnBFBadLatchHint                    206
#define wrnBFLatchMaintConflict              207
#define wrnBFIWriteIOComplete                208

#define errBFIPageEvicted                   -250
#define errBFIPageCached                    -251
#define errBFIOutOfOLPs                     -252
#define errBFIOutOfBatchIOBuffers           -253
#define errBFINoBufferAvailable             -254
#define JET_errDatabaseBufferDependenciesCorrupted  -255
#define errBFIRemainingDependencies         -256
#define errBFIPageFlushPending              -257
#define errBFIPageDirty                     -258
#define errBFIPageFlushed                   -259
#define errBFIPageFaultPending              -260
#define errBFIPageNotVerified               -261
#define errBFIDependentPurged               -262
#define errBFIPageFlushDisallowedOnIOThread -263
#define errBFIPageTouchTooRecent            -264
#define errBFICheckpointWorkRemaining       -266
#define errBFIPageRemapNotReVerified        -267
#define errBFIReqSyncFlushMapWriteFailed    -268
#define errBFIPageFlushPendingHungIO        -269
#define errBFIPageFaultPendingHungIO        -270
#define errBFIPageFlushPendingSlowIO        -271
#define errBFIPageAbandoned                 -272

#define wrnVERRCEMoved                       275


#define errPMOutOfPageSpace                 -300
#define errPMItagTooBig                     -301
#define errPMRecDeleted                     -302
#define errPMTagsUsedUp                     -303
#define wrnBMConflict                        304
#define errDIRNoShortCircuit                -305
#define errDIRCannotSplit                   -306
#define errDIRTop                           -307
#define errDIRFDP                            308
#define errDIRNotSynchronous                -309
#define wrnDIREmptyPage                      310
#define errSPConflict                       -311
#define wrnNDFoundLess                       312
#define wrnNDFoundGreater                    313
#define wrnNDNotFoundInPage                  314
#define errNDOutItemRange                   -315
#define errNDGreaterThanAllItems            -316
#define errNDLastItemNode                   -317
#define errNDFirstItemNode                  -318
#define wrnNDDuplicateItem                   319
#define errNDNoItem                         -320
#define JET_wrnRemainingVersions             321
#define JET_errPreviousVersion              -322
#define JET_errPageBoundary                 -323
#define JET_errKeyBoundary                  -324
#define errDIRInPageFather                  -325
#define errBMMaxKeyInPage                   -326
#define JET_errBadPageLink                  -327
#define JET_errBadBookmark                  -328
#define wrnBMCleanNullOp                     329
#define errBTOperNone                       -330
#define errSPOutOfAvailExtCacheSpace        -331
#define errSPOutOfOwnExtCacheSpace          -332
#define wrnBTMultipageOLC                    333
#define JET_errNTSystemCallFailed           -334
#define wrnBTShallowTree                     335
#define errBTMergeNotSynchronous            -336
#define wrnSPReservedPages                   337
#define JET_errBadParentPageLink            -338
#define wrnSPBuildAvailExtCache              339
#define JET_errSPAvailExtCacheOutOfSync     -340
#define JET_errSPAvailExtCorrupted          -341
#define JET_errSPAvailExtCacheOutOfMemory   -342
#define JET_errSPOwnExtCorrupted            -343
#define JET_errDbTimeCorrupted              -344
#define JET_wrnUniqueKey                     345
#define JET_errKeyTruncated                 -346
#define errSPNoSpaceForYou                  -347
#define JET_errDatabaseLeakInSpace          -348
#define errNDNotFound                       -349
#define errNDOutSonRange                    -350
#define JET_errBadEmptyPage                 -351
#define wrnBTNotVisibleRejected              352
#define wrnBTNotVisibleAccumulated           353
#define JET_errBadLineCount                 -354
#define errSPNoSpaceBelowShrinkTarget       -355
#define wrnSPRequestSpBufRefill              356
#define JET_errPageTagCorrupted             -357
#define JET_errNodeCorrupted                -358


#define wrnFLDKeyTooBig                      400
#define errFLDTooManySegments               -401
#define wrnFLDNullKey                        402
#define wrnFLDOutOfKeys                      403
#define wrnFLDNullSeg                        404
#define wrnFLDNotPresentInIndex              405
#define JET_wrnSeparateLongValue             406
#define wrnRECLongField                      407
#define JET_wrnRecordFoundGreater           JET_wrnSeekNotEqual
#define JET_wrnRecordFoundLess              JET_wrnSeekNotEqual
#define JET_errColumnIllegalNull            JET_errNullInvalid
#define JET_errKeyTooBig                    -408
#define wrnRECUserDefinedDefault             409
#define wrnRECSeparatedLV                    410
#define wrnRECIntrinsicLV                    411
#define wrnFLDIndexUpdated                   414
#define wrnFLDOutOfTuples                    415
#define JET_errCannotSeparateIntrinsicLV    -416
#define wrnRECCompressed                     417
#define errRECCannotCompress                -418
#define errRECCompressionNotPossible        -419
#define wrnRECCompressionScrubDetected       420
#define JET_errSeparatedLongValue           -421
#define wrnFLDNullFirstSeg                   422
#define JET_errMustBeSeparateLongValue      -423
#define JET_errInvalidPreread               -424
#define wrnRECSeparatedEncryptedLV           425
#define JET_errInvalidColumnReference       -426
#define JET_errStaleColumnReference         -427
#define JET_wrnNoMoreRecords                 428
#define errRECColumnNotFound                -429
#define errRECNoCurrentColumnValue          -430
#define JET_errCompressionIntegrityCheckFailed  -431


#define JET_errInvalidLoggedOperation       -500
#define JET_errLogFileCorrupt               -501
#define errLGNoMoreRecords                  -502
#define JET_errNoBackupDirectory            -503
#define JET_errBackupDirectoryNotEmpty      -504
#define JET_errBackupInProgress             -505
#define JET_errRestoreInProgress            -506
#define JET_errMissingPreviousLogFile       -509
#define JET_errLogWriteFail                 -510
#define JET_errLogDisabledDueToRecoveryFailure  -511
#define JET_errCannotLogDuringRecoveryRedo      -512
#define JET_errLogGenerationMismatch        -513
#define JET_errBadLogVersion                -514
#define JET_errInvalidLogSequence           -515
#define JET_errLoggingDisabled              -516
#define JET_errLogBufferTooSmall            -517
#define errLGNotSynchronous                 -518
#define JET_errLogSequenceEnd               -519
#define JET_errNoBackup                     -520
#define JET_errInvalidBackupSequence        -521
#define JET_errBackupNotAllowedYet          -523
#define JET_errDeleteBackupFileFail         -524
#define JET_errMakeBackupDirectoryFail      -525
#define JET_errInvalidBackup                -526
#define JET_errRecoveredWithErrors          -527
#define JET_errMissingLogFile               -528
#define JET_errLogDiskFull                  -529
#define JET_errBadLogSignature              -530
#define JET_errBadDbSignature               -531
#define JET_errBadCheckpointSignature       -532
#define JET_errCheckpointCorrupt            -533
#define JET_errMissingPatchPage             -534
#define JET_errBadPatchPage                 -535
#define JET_errRedoAbruptEnded              -536
#define JET_errPatchFileMissing             -538
#define JET_errDatabaseLogSetMismatch       -539
#define JET_errDatabaseStreamingFileMismatch    -540
#define JET_errLogFileSizeMismatch          -541
#define JET_errCheckpointFileNotFound       -542
#define JET_errRequiredLogFilesMissing      -543
#define JET_errSoftRecoveryOnBackupDatabase -544
#define JET_errLogFileSizeMismatchDatabasesConsistent   -545
#define JET_errLogSectorSizeMismatch        -546
#define JET_errLogSectorSizeMismatchDatabasesConsistent -547
#define JET_errLogSequenceEndDatabasesConsistent        -548

#define JET_errStreamingDataNotLogged       -549

#define JET_errDatabaseDirtyShutdown        -550
#define JET_errDatabaseInconsistent         JET_errDatabaseDirtyShutdown
#define JET_errConsistentTimeMismatch       -551
#define JET_errDatabasePatchFileMismatch    -552
#define JET_errEndingRestoreLogTooLow       -553
#define JET_errStartingRestoreLogTooHigh    -554
#define JET_errGivenLogFileHasBadSignature  -555
#define JET_errGivenLogFileIsNotContiguous  -556
#define JET_errMissingRestoreLogFiles       -557
#define JET_wrnExistingLogFileHasBadSignature   558
#define JET_wrnExistingLogFileIsNotContiguous   559
#define JET_errMissingFullBackup            -560
#define JET_errBadBackupDatabaseSize        -561
#define JET_errDatabaseAlreadyUpgraded      -562
#define JET_errDatabaseIncompleteUpgrade    -563
#define JET_wrnSkipThisRecord                564
#define JET_errMissingCurrentLogFiles       -565

#define JET_errDbTimeTooOld                     -566
#define JET_errDbTimeTooNew                     -567
#define JET_errMissingFileToBackup              -569

#define JET_errLogTornWriteDuringHardRestore    -570
#define JET_errLogTornWriteDuringHardRecovery   -571
#define JET_errLogCorruptDuringHardRestore      -573
#define JET_errLogCorruptDuringHardRecovery     -574

#define JET_errMustDisableLoggingForDbUpgrade   -575
#define errLGRecordDataInaccessible             -576

#define JET_errBadRestoreTargetInstance         -577
#define JET_wrnTargetInstanceRunning             578

#define JET_errRecoveredWithoutUndo             -579

#define JET_errDatabasesNotFromSameSnapshot     -580
#define JET_errSoftRecoveryOnSnapshot           -581
#define JET_errCommittedLogFilesMissing         -582
#define JET_errSectorSizeNotSupported           -583
#define JET_errRecoveredWithoutUndoDatabasesConsistent  -584
#define JET_wrnCommittedLogFilesLost            585
#define JET_errCommittedLogFileCorrupt          -586
#define JET_wrnCommittedLogFilesRemoved         587
#define JET_wrnFinishWithUndo                   588
#define errSkipLogRedoOperation                 -589
#define JET_errLogSequenceChecksumMismatch      -590

#define JET_wrnDatabaseRepaired                  595
#define JET_errPageInitializedMismatch          -596


#define JET_errUnicodeTranslationBufferTooSmall -601
#define JET_errUnicodeTranslationFail           -602
#define JET_errUnicodeNormalizationNotSupported -603
#define JET_errUnicodeLanguageValidationFailure -604

#define JET_errExistingLogFileHasBadSignature   -610
#define JET_errExistingLogFileIsNotContiguous   -611

#define JET_errLogReadVerifyFailure         -612

#define JET_errCheckpointDepthTooDeep       -614

#define JET_errRestoreOfNonBackupDatabase   -615
#define JET_errLogFileNotCopied             -616
#define JET_errSurrogateBackupInProgress    -617
#define JET_errTransactionTooLong           -618

#define JET_errEngineFormatVersionNoLongerSupportedTooLow           -619
#define JET_errEngineFormatVersionNotYetImplementedTooHigh          -620
#define JET_errEngineFormatVersionParamTooLowForRequestedFeature    -621
#define JET_errEngineFormatVersionSpecifiedTooLowForLogVersion                      -622
#define JET_errEngineFormatVersionSpecifiedTooLowForDatabaseVersion                 -623


#define errLogServiceStopped                -624

#define errBackupAbortByCaller              -800
#define JET_errBackupAbortByServer          -801
#define errTooManyPatchRequests             -802

#define JET_errInvalidGrbit                 -900

#define JET_errTermInProgress               -1000
#define JET_errFeatureNotAvailable          -1001
#define JET_errInvalidName                  -1002
#define JET_errInvalidParameter             -1003
#define JET_wrnColumnNull                    1004
#define JET_wrnBufferTruncated               1006
#define JET_wrnDatabaseAttached              1007
#define JET_errDatabaseFileReadOnly         -1008
#define JET_wrnSortOverflow                  1009
#define JET_errInvalidDatabaseId            -1010
#define JET_errOutOfMemory                  -1011
#define JET_errOutOfDatabaseSpace           -1012
#define JET_errOutOfCursors                 -1013
#define JET_errOutOfBuffers                 -1014
#define JET_errTooManyIndexes               -1015
#define JET_errTooManyKeys                  -1016
#define JET_errRecordDeleted                -1017
#define JET_errReadVerifyFailure            -1018
#define JET_errPageNotInitialized           -1019
#define JET_errOutOfFileHandles             -1020
#define JET_errDiskReadVerificationFailure  -1021
#define JET_errDiskIO                       -1022
#define JET_errInvalidPath                  -1023
#define JET_errInvalidSystemPath            -1024
#define JET_errInvalidLogDirectory          -1025
#define JET_errRecordTooBig                 -1026
#define JET_errTooManyOpenDatabases         -1027
#define JET_errInvalidDatabase              -1028
#define JET_errNotInitialized               -1029
#define JET_errAlreadyInitialized           -1030
#define JET_errInitInProgress               -1031
#define JET_errFileAccessDenied             -1032
#define JET_errQueryNotSupported            -1034
#define JET_errSQLLinkNotSupported          -1035
#define JET_errBufferTooSmall               -1038
#define JET_wrnSeekNotEqual                  1039
#define JET_errTooManyColumns               -1040
#define JET_errContainerNotEmpty            -1043
#define JET_errInvalidFilename              -1044
#define JET_errInvalidBookmark              -1045
#define JET_errColumnInUse                  -1046
#define JET_errInvalidBufferSize            -1047
#define JET_errColumnNotUpdatable           -1048
#define JET_errIndexInUse                   -1051
#define JET_errLinkNotSupported             -1052
#define JET_errNullKeyDisallowed            -1053
#define JET_errNotInTransaction             -1054
#define JET_wrnNoErrorInfo                   1055
#define JET_errMustRollback                 -1057
#define JET_wrnNoIdleActivity                1058
#define JET_errTooManyActiveUsers           -1059
#define JET_errInvalidCountry               -1061
#define JET_errInvalidLanguageId            -1062
#define JET_errInvalidCodePage              -1063
#define JET_errInvalidLCMapStringFlags      -1064
#define JET_errVersionStoreEntryTooBig      -1065
#define JET_errVersionStoreOutOfMemoryAndCleanupTimedOut    -1066
#define JET_wrnNoWriteLock                   1067
#define JET_wrnColumnSetNull                 1068
#define JET_errVersionStoreOutOfMemory      -1069
#define JET_errCurrencyStackOutOfMemory     -1070
#define JET_errCannotIndex                  -1071
#define JET_errRecordNotDeleted             -1072
#define JET_errTooManyMempoolEntries        -1073
#define JET_errOutOfObjectIDs               -1074
#define JET_errOutOfLongValueIDs            -1075
#define JET_errOutOfAutoincrementValues     -1076
#define JET_errOutOfDbtimeValues            -1077
#define JET_errOutOfSequentialIndexValues   -1078

#define JET_errRunningInOneInstanceMode     -1080
#define JET_errRunningInMultiInstanceMode   -1081
#define JET_errSystemParamsAlreadySet       -1082

#define JET_errSystemPathInUse              -1083
#define JET_errLogFilePathInUse             -1084
#define JET_errTempPathInUse                -1085
#define JET_errInstanceNameInUse            -1086
#define JET_errSystemParameterConflict      -1087

#define JET_errInstanceUnavailable          -1090
#define JET_errDatabaseUnavailable          -1091
#define JET_errInstanceUnavailableDueToFatalLogDiskFull -1092
#define JET_errInvalidSesparamId            -1093

#define JET_errTooManyRecords               -1094

#define JET_errInvalidDbparamId             -1095

#define JET_errOutOfSessions                -1101
#define JET_errWriteConflict                -1102
#define JET_errTransTooDeep                 -1103
#define JET_errInvalidSesid                 -1104
#define JET_errWriteConflictPrimaryIndex    -1105
#define JET_errInTransaction                -1108
#define JET_errRollbackRequired             -1109
#define JET_errTransReadOnly                -1110
#define JET_errSessionWriteConflict         -1111

#define JET_errRecordTooBigForBackwardCompatibility             -1112
#define JET_errCannotMaterializeForwardOnlySort                 -1113

#define JET_errSesidTableIdMismatch         -1114
#define JET_errInvalidInstance              -1115
#define JET_errDirtyShutdown                -1116
#define JET_errReadPgnoVerifyFailure        -1118
#define JET_errReadLostFlushVerifyFailure   -1119
#define errCantRetrieveDebuggeeMemory           -1120
#define JET_errFileSystemCorruption             -1121
#define JET_wrnShrinkNotPossible                1122
#define JET_errRecoveryVerifyFailure            -1123

#define JET_errFilteredMoveNotSupported         -1124

#define JET_errMustCommitDistributedTransactionToLevel0         -1150
#define JET_errDistributedTransactionAlreadyPreparedToCommit    -1151
#define JET_errNotInDistributedTransaction                      -1152
#define JET_errDistributedTransactionNotYetPreparedToCommit     -1153
#define JET_errCannotNestDistributedTransactions                -1154
#define JET_errDTCMissingCallback                               -1160
#define JET_errDTCMissingCallbackOnRecovery                     -1161
#define JET_errDTCCallbackUnexpectedError                       -1162
#define JET_wrnDTCCommitTransaction                              1163
#define JET_wrnDTCRollbackTransaction                            1164

#define JET_errDatabaseDuplicate            -1201
#define JET_errDatabaseInUse                -1202
#define JET_errDatabaseNotFound             -1203
#define JET_errDatabaseInvalidName          -1204
#define JET_errDatabaseInvalidPages         -1205
#define JET_errDatabaseCorrupted            -1206
#define JET_errDatabaseLocked               -1207
#define JET_errCannotDisableVersioning      -1208
#define JET_errInvalidDatabaseVersion       -1209


#define JET_errDatabase200Format            -1210
#define JET_errDatabase400Format            -1211
#define JET_errDatabase500Format            -1212

#define JET_errPageSizeMismatch             -1213
#define JET_errTooManyInstances             -1214
#define JET_errDatabaseSharingViolation     -1215
#define JET_errAttachedDatabaseMismatch     -1216
#define JET_errDatabaseInvalidPath          -1217
#define JET_errDatabaseIdInUse              -1218
#define JET_errForceDetachNotAllowed        -1219
#define JET_errCatalogCorrupted             -1220
#define JET_errPartiallyAttachedDB          -1221
#define JET_errDatabaseSignInUse            -1222
#define errSkippedDbHeaderUpdate            -1223

#define JET_errDatabaseCorruptedNoRepair    -1224
#define JET_errInvalidCreateDbVersion       -1225

#define JET_errDatabaseIncompleteIncrementalReseed  -1226
#define JET_errDatabaseInvalidIncrementalReseed     -1227
#define JET_errDatabaseFailedIncrementalReseed      -1228
#define JET_errNoAttachmentsFailedIncrementalReseed -1229

#define JET_errDatabaseNotReady             -1230
#define JET_errDatabaseAttachedForRecovery  -1231
#define JET_errTransactionsNotReadyDuringRecovery -1232


#define JET_wrnTableEmpty                    1301
#define JET_errTableLocked                  -1302
#define JET_errTableDuplicate               -1303
#define JET_errTableInUse                   -1304
#define JET_errObjectNotFound               -1305
#define JET_errDensityInvalid               -1307
#define JET_errTableNotEmpty                -1308
#define JET_errInvalidTableId               -1310
#define JET_errTooManyOpenTables            -1311
#define JET_errIllegalOperation             -1312
#define JET_errTooManyOpenTablesAndCleanupTimedOut  -1313
#define JET_errObjectDuplicate              -1314
#define JET_errInvalidObject                -1316
#define JET_errCannotDeleteTempTable        -1317
#define JET_errCannotDeleteSystemTable      -1318
#define JET_errCannotDeleteTemplateTable    -1319
#define errFCBTooManyOpen                   -1320
#define errFCBAboveThreshold                -1321
#define JET_errExclusiveTableLockRequired   -1322
#define JET_errFixedDDL                     -1323
#define JET_errFixedInheritedDDL            -1324
#define JET_errCannotNestDDL                -1325
#define JET_errDDLNotInheritable            -1326
#define JET_wrnTableInUseBySystem            1327
#define JET_errInvalidSettings              -1328
#define JET_errClientRequestToStopJetService            -1329
#define JET_errCannotAddFixedVarColumnToDerivedTable    -1330
#define errFCBExists                        -1331
#define errFCBUnusable                      -1332
#define wrnCATNoMoreRecords                  1333


#define JET_errIndexCantBuild               -1401
#define JET_errIndexHasPrimary              -1402
#define JET_errIndexDuplicate               -1403
#define JET_errIndexNotFound                -1404
#define JET_errIndexMustStay                -1405
#define JET_errIndexInvalidDef              -1406
#define JET_errInvalidCreateIndex           -1409
#define JET_errTooManyOpenIndexes           -1410
#define JET_errMultiValuedIndexViolation    -1411
#define JET_errIndexBuildCorrupted          -1412
#define JET_errPrimaryIndexCorrupted        -1413
#define JET_errSecondaryIndexCorrupted      -1414
#define JET_wrnCorruptIndexDeleted           1415
#define JET_errInvalidIndexId               -1416
#define JET_wrnPrimaryIndexOutOfDate         1417
#define JET_wrnSecondaryIndexOutOfDate       1418

#define JET_errIndexTuplesSecondaryIndexOnly        -1430
#define JET_errIndexTuplesTooManyColumns            -1431
#define JET_errIndexTuplesOneColumnOnly             JET_errIndexTuplesTooManyColumns
#define JET_errIndexTuplesNonUniqueOnly             -1432
#define JET_errIndexTuplesTextBinaryColumnsOnly     -1433
#define JET_errIndexTuplesTextColumnsOnly           JET_errIndexTuplesTextBinaryColumnsOnly
#define JET_errIndexTuplesVarSegMacNotAllowed       -1434
#define JET_errIndexTuplesInvalidLimits             -1435
#define JET_errIndexTuplesCannotRetrieveFromIndex   -1436
#define JET_errIndexTuplesKeyTooSmall               -1437
#define JET_errInvalidLVChunkSize                   -1438
#define JET_errColumnCannotBeEncrypted              -1439
#define JET_errCannotIndexOnEncryptedColumn         -1440


#define JET_errColumnLong                   -1501
#define JET_errColumnNoChunk                -1502
#define JET_errColumnDoesNotFit             -1503
#define JET_errNullInvalid                  -1504
#define JET_errColumnIndexed                -1505
#define JET_errColumnTooBig                 -1506
#define JET_errColumnNotFound               -1507
#define JET_errColumnDuplicate              -1508
#define JET_errMultiValuedColumnMustBeTagged    -1509
#define JET_errColumnRedundant              -1510
#define JET_errInvalidColumnType            -1511
#define JET_wrnColumnMaxTruncated            1512
#define JET_errTaggedNotNULL                -1514
#define JET_errNoCurrentIndex               -1515
#define JET_errKeyIsMade                    -1516
#define JET_errBadColumnId                  -1517
#define JET_errBadItagSequence              -1518
#define JET_errColumnInRelationship         -1519
#define JET_wrnCopyLongValue                 1520
#define JET_errCannotBeTagged               -1521
#define wrnLVNoLongValues                    1522
#define JET_wrnTaggedColumnsRemaining        1523
#define JET_errDefaultValueTooBig           -1524
#define JET_errMultiValuedDuplicate         -1525
#define JET_errLVCorrupted                  -1526
#define wrnLVNoMoreData                      1527
#define JET_errMultiValuedDuplicateAfterTruncation  -1528
#define JET_errDerivedColumnCorruption      -1529
#define JET_errInvalidPlaceholderColumn     -1530
#define JET_wrnColumnSkipped                 1531
#define JET_wrnColumnNotLocal                1532
#define JET_wrnColumnMoreTags                1533
#define JET_wrnColumnTruncated               1534
#define JET_wrnColumnPresent                 1535
#define JET_wrnColumnSingleValue             1536
#define JET_wrnColumnDefault                 1537
#define JET_errColumnCannotBeCompressed     -1538
#define JET_wrnColumnNotInRecord             1539
#define JET_errColumnNoEncryptionKey        -1540
#define JET_wrnColumnReference               1541

#define JET_errRecordNotFound               -1601
#define JET_errRecordNoCopy                 -1602
#define JET_errNoCurrentRecord              -1603
#define JET_errRecordPrimaryChanged         -1604
#define JET_errKeyDuplicate                 -1605
#define JET_errAlreadyPrepared              -1607
#define JET_errKeyNotMade                   -1608
#define JET_errUpdateNotPrepared            -1609
#define JET_wrnDataHasChanged                1610
#define JET_errDataHasChanged               -1611
#define JET_wrnKeyChanged                    1618
#define JET_errLanguageNotSupported         -1619
#define JET_errDecompressionFailed          -1620
#define JET_errUpdateMustVersion            -1621
#define JET_errDecryptionFailed             -1622
#define JET_errEncryptionBadItag            -1623


#define JET_errTooManySorts                 -1701
#define JET_errInvalidOnSort                -1702


#define JET_errTempFileOpenError            -1803
#define JET_errTooManyAttachedDatabases     -1805
#define JET_errDiskFull                     -1808
#define JET_errPermissionDenied             -1809
#define JET_errFileNotFound                 -1811
#define JET_errFileInvalidType              -1812
#define JET_wrnFileOpenReadOnly              1813
#define JET_errFileAlreadyExists            -1814


#define JET_errAfterInitialization          -1850
#define JET_errLogCorrupted                 -1852


#define JET_errInvalidOperation             -1906
#define JET_errAccessDenied                 -1907
#define JET_wrnIdleFull                      1908
#define JET_errTooManySplits                -1909
#define JET_errSessionSharingViolation      -1910
#define JET_errEntryPointNotFound           -1911
#define JET_errSessionContextAlreadySet     -1912
#define JET_errSessionContextNotSetByThisThread -1913
#define JET_errSessionInUse                 -1914
#define JET_errRecordFormatConversionFailed -1915
#define JET_errOneDatabasePerSession        -1916
#define JET_errRollbackError                -1917
#define JET_errFlushMapVersionUnsupported   -1918
#define JET_errFlushMapDatabaseMismatch     -1919
#define JET_errFlushMapUnrecoverable        -1920

#define JET_errRBSFileCorrupt               -1921
#define JET_errRBSHeaderCorrupt             -1922
#define JET_errRBSDbMismatch                -1923
#define errRBSAttachInfoNotFound            -1924
#define JET_errBadRBSVersion                -1925
#define JET_errOutOfRBSSpace                -1926
#define JET_errRBSInvalidSign               -1927
#define JET_errRBSInvalidRecord             -1928
#define JET_errRBSRCInvalidRBS              -1929
#define JET_errRBSRCNoRBSFound              -1930
#define JET_errRBSRCBadDbState              -1931
#define JET_errRBSMissingReqLogs            -1932
#define JET_errRBSLogDivergenceFailed       -1933
#define JET_errRBSRCCopyLogsRevertState     -1934
#define JET_errDatabaseIncompleteRevert     -1935
#define JET_errRBSRCRevertCancelled         -1936
#define JET_errRBSRCInvalidDbFormatVersion  -1937
#define JET_errRBSCannotDetermineDivergence -1938
#define errRBSRequiredRangeTooLarge         -1939

#define JET_wrnDefragAlreadyRunning          2000
#define JET_wrnDefragNotRunning              2001
#define JET_wrnDatabaseScanAlreadyRunning    2002
#define JET_wrnDatabaseScanNotRunning        2003
#define JET_errDatabaseAlreadyRunningMaintenance -2004
#define wrnOLD2TaskSlotFull                  2005


#define JET_wrnCallbackNotRegistered         2100
#define JET_errCallbackFailed               -2101
#define JET_errCallbackNotResolved          -2102

#define JET_errSpaceHintsInvalid            -2103

#define JET_errOSSnapshotInvalidSequence    -2401
#define JET_errOSSnapshotTimeOut            -2402
#define JET_errOSSnapshotNotAllowed         -2403
#define JET_errOSSnapshotInvalidSnapId      -2404
#define errOSSnapshotNewLogStopped          -2405


#define JET_errTooManyTestInjections        -2501
#define JET_errTestInjectionNotSupported    -2502


#define JET_errInvalidLogDataSequence       -2601
#define JET_wrnPreviousLogFileIncomplete    2602

#define JET_errLSCallbackNotSpecified       -3000
#define JET_errLSAlreadySet                 -3001
#define JET_errLSNotSet                     -3002


#define JET_errFileIOSparse                 -4000
#define JET_errFileIOBeyondEOF              -4001
#define JET_errFileIOAbort                  -4002
#define JET_errFileIORetry                  -4003
#define JET_errFileIOFail                   -4004
#define JET_errFileCompressed               -4005
#define errDiskTilt                             -4006
#define wrnIOPending                        4009
#define wrnIOSlow                       4010





#if !defined(_JET_NOPROTOTYPES)

#ifdef __cplusplus
extern "C" {
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetInit(
    _Inout_opt_ JET_INSTANCE *  pinstance );


#if ( JET_VERSION >= 0x0501 )
JET_ERR JET_API
JetInit2(
    _Inout_opt_ JET_INSTANCE *  pinstance,
    _In_ JET_GRBIT              grbit );

#endif

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )
#if ( JET_VERSION < 0x0600 )
#define JetInit3A JetInit3
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetInit3A(
    _Inout_opt_ JET_INSTANCE *  pinstance,
    _In_opt_ JET_RSTINFO_A *    prstInfo,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetInit3W(
    _Inout_opt_ JET_INSTANCE *  pinstance,
    _In_opt_ JET_RSTINFO_W *    prstInfo,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetInit3 JetInit3W
#else
#define JetInit3 JetInit3A
#endif
#endif


#endif

#if ( JET_VERSION >= 0x0A01 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetInit4A(
    _Inout_opt_ JET_INSTANCE *  pinstance,
    _In_opt_ JET_RSTINFO2_A *   prstInfo,
    _In_ JET_GRBIT              grbit );

JET_ERR JET_API
JetInit4W(
    _Inout_opt_ JET_INSTANCE *  pinstance,
    _In_opt_ JET_RSTINFO2_W *   prstInfo,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetInit4 JetInit4W
#else
#define JetInit4 JetInit4A
#endif

#endif

#if ( JET_VERSION >= 0x0501 )
#if ( JET_VERSION < 0x0600 )
#define JetCreateInstanceA JetCreateInstance
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateInstanceA(
    _Out_ JET_INSTANCE *    pinstance,
    _In_opt_ JET_PCSTR      szInstanceName );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateInstanceW(
    _Out_ JET_INSTANCE *    pinstance,
    _In_opt_ JET_PCWSTR     szInstanceName );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateInstance JetCreateInstanceW
#else
#define JetCreateInstance JetCreateInstanceA
#endif
#endif

#if ( JET_VERSION < 0x0600 )
#define JetCreateInstance2A JetCreateInstance2
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateInstance2A(
    _Out_ JET_INSTANCE *    pinstance,
    _In_opt_ JET_PCSTR      szInstanceName,
    _In_opt_ JET_PCSTR      szDisplayName,
    _In_ JET_GRBIT          grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateInstance2W(
    _Out_ JET_INSTANCE *    pinstance,
    _In_opt_ JET_PCWSTR     szInstanceName,
    _In_opt_ JET_PCWSTR     szDisplayName,
    _In_ JET_GRBIT          grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateInstance2 JetCreateInstance2W
#else
#define JetCreateInstance2 JetCreateInstance2A
#endif
#endif

#endif

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetInstanceMiscInfo(
    _In_ JET_INSTANCE               instance,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetTerm(
    _In_ JET_INSTANCE   instance );

#endif
#pragma endregion

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetTerm2(
    _In_ JET_INSTANCE   instance,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetStopService();

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetStopServiceInstance(
    _In_ JET_INSTANCE   instance );

#endif
#pragma endregion

#endif

#if ( JET_VERSION >= 0x0602 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetStopServiceInstance2(
    _In_ JET_INSTANCE       instance,
    _In_ const JET_GRBIT    grbit );

#endif
#pragma endregion

#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetStopBackup();

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetStopBackupInstance(
    _In_ JET_INSTANCE   instance );

#endif
#pragma endregion

#endif

#if ( JET_VERSION < 0x0600 )
#define JetSetSystemParameterA JetSetSystemParameter
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetSystemParameterA(
    _Inout_opt_ JET_INSTANCE *  pinstance,
    _In_opt_ JET_SESID          sesid,
    _In_ unsigned long          paramid,
    _In_opt_ JET_API_PTR        lParam,
    _In_opt_ JET_PCSTR          szParam );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetSystemParameterW(
    _Inout_opt_ JET_INSTANCE *  pinstance,
    _In_opt_ JET_SESID          sesid,
    _In_ unsigned long          paramid,
    _In_opt_ JET_API_PTR        lParam,
    _In_opt_ JET_PCWSTR         szParam );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetSetSystemParameter JetSetSystemParameterW
#else
#define JetSetSystemParameter JetSetSystemParameterA
#endif
#endif

#if ( JET_VERSION < 0x0600 )
#define JetGetSystemParameterA JetGetSystemParameter
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetSystemParameterA(
    _In_ JET_INSTANCE                   instance,
    _In_opt_ JET_SESID                  sesid,
    _In_ unsigned long                  paramid,
    _Out_opt_ JET_API_PTR *             plParam,
    _Out_writes_bytes_opt_( cbMax ) JET_PSTR    szParam,
    _In_ unsigned long                  cbMax );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetSystemParameterW(
    _In_ JET_INSTANCE                   instance,
    _In_opt_ JET_SESID                  sesid,
    _In_ unsigned long                  paramid,
    _Out_opt_ JET_API_PTR *             plParam,
    _Out_writes_bytes_opt_( cbMax ) JET_PWSTR   szParam,
    _In_ unsigned long                  cbMax );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetSystemParameter JetGetSystemParameterW
#else
#define JetGetSystemParameter JetGetSystemParameterA
#endif
#endif


#if ( JET_VERSION >= 0x0603 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetSetResourceParam(
    _In_ JET_INSTANCE   instance,
    _In_ JET_RESOPER    resoper,
    _In_ JET_RESID      resid,
    _In_ JET_API_PTR    ulParam );

JET_ERR JET_API
JetGetResourceParam(
    _In_ JET_INSTANCE   instance,
    _In_ JET_RESOPER    resoper,
    _In_ JET_RESID      resid,
    _Out_ JET_API_PTR*  pulParam );

#endif
#pragma endregion

#endif

#if ( JET_VERSION >= 0x0501 )

#if ( JET_VERSION < 0x0600 )
#define JetEnableMultiInstanceA JetEnableMultiInstance
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetEnableMultiInstanceA(
    _In_reads_opt_( csetsysparam ) JET_SETSYSPARAM_A *  psetsysparam,
    _In_ unsigned long                                  csetsysparam,
    _Out_opt_ unsigned long *                           pcsetsucceed );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetEnableMultiInstanceW(
    _In_reads_opt_( csetsysparam ) JET_SETSYSPARAM_W *  psetsysparam,
    _In_ unsigned long                                  csetsysparam,
    _Out_opt_ unsigned long *                           pcsetsucceed );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetEnableMultiInstance JetEnableMultiInstanceW
#else
#define JetEnableMultiInstance JetEnableMultiInstanceA
#endif
#endif

#endif


#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetResetCounter(
    _In_ JET_SESID  sesid,
    _In_ long       CounterType );

JET_ERR JET_API
JetGetCounter(
    _In_ JET_SESID  sesid,
    _In_ long       CounterType,
    _Out_ long *    plValue );

#endif
#pragma endregion


#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetThreadStats(
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax );

#endif
#pragma endregion

#endif

#if ( JET_VERSION < 0x0600 )
#define JetBeginSessionA JetBeginSession
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetBeginSessionA(
    _In_ JET_INSTANCE   instance,
    _Out_ JET_SESID *   psesid,
    _In_opt_ JET_PCSTR  szUserName,
    _In_opt_ JET_PCSTR  szPassword );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetBeginSessionW(
    _In_ JET_INSTANCE   instance,
    _Out_ JET_SESID *   psesid,
    _In_opt_ JET_PCWSTR szUserName,
    _In_opt_ JET_PCWSTR szPassword );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetBeginSession JetBeginSessionW
#else
#define JetBeginSession JetBeginSessionA
#endif
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDupSession(
    _In_ JET_SESID      sesid,
    _Out_ JET_SESID *   psesid );

#endif
#pragma endregion

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetEndSession(
    _In_ JET_SESID  sesid,
    _In_ JET_GRBIT  grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetGetSessionInfo(
    _In_ JET_SESID                  sesid,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ const unsigned long        cbMax,
    _In_ const unsigned long        ulInfoLevel );

#endif
#pragma endregion

#endif


#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetVersion(
    _In_ JET_SESID          sesid,
    _Out_ unsigned long *   pwVersion );

JET_ERR JET_API
JetIdle(
    _In_ JET_SESID  sesid,
    _In_ JET_GRBIT  grbit );

#endif
#pragma endregion


#if ( JET_VERSION >= 0x0A01 )


#define JET_dbparamDbSizeMaxPages           8193

#define JET_dbparamCachePriority            8194

#define JET_dbparamShrinkDatabaseOptions    8195

#define JET_dbparamShrinkDatabaseTimeQuota  8196

#define JET_dbparamShrinkDatabaseSizeLimit  8197

#define JET_dbparamLeakReclaimerEnabled     8198

#define JET_dbparamLeakReclaimerTimeQuota   8199

#define JET_dbparamMaxValueInvalid          8200



#define JET_bitShrinkDatabaseEofOnAttach                                0x00000001

#define JET_bitShrinkDatabaseFullCategorizationOnAttach                 0x00000002

#define JET_bitShrinkDatabaseDontMoveRootsOnAttach                      0x00000004

#define JET_bitShrinkDatabaseDontTruncateLeakedPagesOnAttach            0x00000008

#define JET_bitShrinkDatabaseDontTruncateIndeterminatePagesOnAttach     0x00000010

#endif


#if ( JET_VERSION < 0x0600 )
#define JetCreateDatabaseA JetCreateDatabase
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateDatabaseA(
    _In_ JET_SESID      sesid,
    _In_ JET_PCSTR      szFilename,
    _In_opt_ JET_PCSTR  szConnect,
    _Out_ JET_DBID *    pdbid,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateDatabaseW(
    _In_ JET_SESID      sesid,
    _In_ JET_PCWSTR     szFilename,
    _In_opt_ JET_PCWSTR szConnect,
    _Out_ JET_DBID *    pdbid,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateDatabase JetCreateDatabaseW
#else
#define JetCreateDatabase JetCreateDatabaseA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetCreateDatabase2A JetCreateDatabase2
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateDatabase2A(
    _In_ JET_SESID              sesid,
    _In_ JET_PCSTR              szFilename,
    _In_ const unsigned long    cpgDatabaseSizeMax,
    _Out_ JET_DBID *            pdbid,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetCreateDatabase2W(
    _In_ JET_SESID              sesid,
    _In_ JET_PCWSTR             szFilename,
    _In_ const unsigned long    cpgDatabaseSizeMax,
    _Out_ JET_DBID *            pdbid,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateDatabase2 JetCreateDatabase2W
#else
#define JetCreateDatabase2 JetCreateDatabase2A
#endif
#endif


#if ( JET_VERSION >= 0x0A01 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetCreateDatabase3A(
    _In_ JET_SESID                                  sesid,
    _In_ JET_PCSTR                                  szFilename,
    _Out_ JET_DBID *                                pdbid,
    _In_reads_opt_( csetdbparam ) JET_SETDBPARAM *  rgsetdbparam,
    _In_ unsigned long                              csetdbparam,
    _In_ JET_GRBIT                                  grbit );

JET_ERR JET_API JetCreateDatabase3W(
    _In_ JET_SESID                                  sesid,
    _In_ JET_PCWSTR                                 szFilename,
    _Out_ JET_DBID *                                pdbid,
    _In_reads_opt_( csetdbparam ) JET_SETDBPARAM *  rgsetdbparam,
    _In_ unsigned long                              csetdbparam,
    _In_ JET_GRBIT                                  grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateDatabase3 JetCreateDatabase3W
#else
#define JetCreateDatabase3 JetCreateDatabase3A
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetAttachDatabaseA JetAttachDatabase
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetAttachDatabaseA(
    _In_ JET_SESID  sesid,
    _In_ JET_PCSTR  szFilename,
    _In_ JET_GRBIT  grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetAttachDatabaseW(
    _In_ JET_SESID  sesid,
    _In_ JET_PCWSTR szFilename,
    _In_ JET_GRBIT  grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetAttachDatabase JetAttachDatabaseW
#else
#define JetAttachDatabase JetAttachDatabaseA
#endif
#endif

#if ( JET_VERSION < 0x0600 )
#define JetAttachDatabase2A JetAttachDatabase2
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetAttachDatabase2A(
    _In_ JET_SESID              sesid,
    _In_ JET_PCSTR              szFilename,
    _In_ const unsigned long    cpgDatabaseSizeMax,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetAttachDatabase2W(
    _In_ JET_SESID              sesid,
    _In_ JET_PCWSTR             szFilename,
    _In_ const unsigned long    cpgDatabaseSizeMax,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetAttachDatabase2 JetAttachDatabase2W
#else
#define JetAttachDatabase2 JetAttachDatabase2A
#endif
#endif


#if ( JET_VERSION >= 0x0A01 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetAttachDatabase3A(
    _In_ JET_SESID                                  sesid,
    _In_ JET_PCSTR                                  szFilename,
    _In_reads_opt_( csetdbparam ) JET_SETDBPARAM *  rgsetdbparam,
    _In_ unsigned long                              csetdbparam,
    _In_ JET_GRBIT                                  grbit );

JET_ERR JET_API
JetAttachDatabase3W(
    _In_ JET_SESID                                  sesid,
    _In_ JET_PCWSTR                                 szFilename,
    _In_reads_opt_( csetdbparam ) JET_SETDBPARAM *  rgsetdbparam,
    _In_ unsigned long                              csetdbparam,
    _In_ JET_GRBIT                                  grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetAttachDatabase3 JetAttachDatabase3W
#else
#define JetAttachDatabase3 JetAttachDatabase3A
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetDetachDatabaseA JetDetachDatabase
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDetachDatabaseA(
    _In_ JET_SESID  sesid,
    _In_opt_ JET_PCSTR  szFilename );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDetachDatabaseW(
    _In_ JET_SESID  sesid,
    _In_opt_ JET_PCWSTR szFilename );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetDetachDatabase JetDetachDatabaseW
#else
#define JetDetachDatabase JetDetachDatabaseA
#endif
#endif

#if ( JET_VERSION >= 0x0501 )
#if ( JET_VERSION < 0x0600 )
#define JetDetachDatabase2A JetDetachDatabase2
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDetachDatabase2A(
    _In_ JET_SESID  sesid,
    _In_opt_ JET_PCSTR  szFilename,
    _In_ JET_GRBIT  grbit);

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDetachDatabase2W(
    _In_ JET_SESID  sesid,
    _In_opt_ JET_PCWSTR szFilename,
    _In_ JET_GRBIT  grbit);

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetDetachDatabase2 JetDetachDatabase2W
#else
#define JetDetachDatabase2 JetDetachDatabase2A
#endif
#endif

#endif


#if ( JET_VERSION < 0x0600 )
#define JetGetObjectInfoA JetGetObjectInfo
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetObjectInfoA(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_OBJTYP                 objtyp,
    _In_opt_ JET_PCSTR              szContainerName,
    _In_opt_ JET_PCSTR              szObjectName,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetObjectInfoW(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_OBJTYP                 objtyp,
    _In_opt_ JET_PCWSTR             szContainerName,
    _In_opt_ JET_PCWSTR             szObjectName,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetObjectInfo JetGetObjectInfoW
#else
#define JetGetObjectInfo JetGetObjectInfoA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetGetTableInfoA JetGetTableInfo
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetTableInfoA(
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetTableInfoW(
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetTableInfo JetGetTableInfoW
#else
#define JetGetTableInfo JetGetTableInfoA
#endif
#endif

#if ( JET_VERSION >= 0x0A01 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetTableInfoW(
    _In_opt_ JET_SESID                              sesid,
    _In_ JET_TABLEID                                tableid,
    _In_reads_bytes_opt_( cbParam ) const void *    pvParam,
    _In_ unsigned long                              cbParam,
    _In_ unsigned long                              InfoLevel );


JET_ERR JET_API
JetSetTableInfoA(
    _In_opt_ JET_SESID                              sesid,
    _In_ JET_TABLEID                                tableid,
    _In_reads_bytes_opt_( cbParam ) const void *    pvParam,
    _In_ unsigned long                              cbParam,
    _In_ unsigned long                              InfoLevel );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetSetTableInfo JetSetTableInfoW
#else
#define JetSetTableInfo JetSetTableInfoA
#endif

JET_ERR JET_API
JetCreateEncryptionKey(
    _In_ unsigned long                                      encryptionAlgorithm,
    _Out_writes_bytes_to_opt_( cbKey, *pcbActual ) void *   pvKey,
    _In_ unsigned long                                      cbKey,
    _Out_opt_ unsigned long *                               pcbActual );

#endif

#if ( JET_VERSION < 0x0600 )
#define JetCreateTableA JetCreateTable
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateTableA(
    _In_ JET_SESID      sesid,
    _In_ JET_DBID       dbid,
    _In_ JET_PCSTR      szTableName,
    _In_ unsigned long  lPages,
    _In_ unsigned long  lDensity,
    _Out_ JET_TABLEID * ptableid );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateTableW(
    _In_ JET_SESID      sesid,
    _In_ JET_DBID       dbid,
    _In_ JET_PCWSTR     szTableName,
    _In_ unsigned long  lPages,
    _In_ unsigned long  lDensity,
    _Out_ JET_TABLEID * ptableid );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateTable JetCreateTableW
#else
#define JetCreateTable JetCreateTableA
#endif
#endif

#if ( JET_VERSION < 0x0600 )
#define JetCreateTableColumnIndexA JetCreateTableColumnIndex
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateTableColumnIndexA(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    _Inout_ JET_TABLECREATE_A * ptablecreate );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateTableColumnIndexW(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    _Inout_ JET_TABLECREATE_W * ptablecreate );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateTableColumnIndex JetCreateTableColumnIndexW
#else
#define JetCreateTableColumnIndex JetCreateTableColumnIndexA
#endif
#endif


#if ( JET_VERSION >= 0x0501 )
#if ( JET_VERSION < 0x0600 )
#define JetCreateTableColumnIndex2A JetCreateTableColumnIndex2
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateTableColumnIndex2A(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _Inout_ JET_TABLECREATE2_A *    ptablecreate );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateTableColumnIndex2W(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _Inout_ JET_TABLECREATE2_W *    ptablecreate );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateTableColumnIndex2 JetCreateTableColumnIndex2W
#else
#define JetCreateTableColumnIndex2 JetCreateTableColumnIndex2A
#endif
#endif
#endif

#if ( JET_VERSION >= 0x0601 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateTableColumnIndex3A(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _Inout_ JET_TABLECREATE3_A *    ptablecreate );

JET_ERR JET_API
JetCreateTableColumnIndex3W(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _Inout_ JET_TABLECREATE3_W *    ptablecreate );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateTableColumnIndex3 JetCreateTableColumnIndex3W
#else
#define JetCreateTableColumnIndex3 JetCreateTableColumnIndex3A
#endif
#endif

#if ( JET_VERSION >= 0x0602 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateTableColumnIndex4A(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _Inout_ JET_TABLECREATE4_A *    ptablecreate );

#endif
#pragma endregion

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateTableColumnIndex4W(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _Inout_ JET_TABLECREATE4_W *    ptablecreate );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateTableColumnIndex4 JetCreateTableColumnIndex4W
#else
#define JetCreateTableColumnIndex4 JetCreateTableColumnIndex4A
#endif
#endif

#if ( JET_VERSION >= 0x0A01 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateTableColumnIndex5A(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _Inout_ JET_TABLECREATE5_A *    ptablecreate );

JET_ERR JET_API
JetCreateTableColumnIndex5W(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _Inout_ JET_TABLECREATE5_W *    ptablecreate );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateTableColumnIndex5 JetCreateTableColumnIndex5W
#else
#define JetCreateTableColumnIndex5 JetCreateTableColumnIndex5A
#endif
#endif

#if ( JET_VERSION < 0x0600 )
#define JetDeleteTableA JetDeleteTable
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDeleteTableA(
    _In_ JET_SESID  sesid,
    _In_ JET_DBID   dbid,
    _In_ JET_PCSTR  szTableName );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDeleteTableW(
    _In_ JET_SESID  sesid,
    _In_ JET_DBID   dbid,
    _In_ JET_PCWSTR szTableName );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetDeleteTable JetDeleteTableW
#else
#define JetDeleteTable JetDeleteTableA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetRenameTableA JetRenameTable
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetRenameTableA(
    _In_ JET_SESID  sesid,
    _In_ JET_DBID   dbid,
    _In_ JET_PCSTR  szName,
    _In_ JET_PCSTR  szNameNew );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetRenameTableW(
    _In_ JET_SESID  sesid,
    _In_ JET_DBID   dbid,
    _In_ JET_PCWSTR szName,
    _In_ JET_PCWSTR szNameNew );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetRenameTable JetRenameTableW
#else
#define JetRenameTable JetRenameTableA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetGetTableColumnInfoA JetGetTableColumnInfo
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetTableColumnInfoA(
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    _In_opt_ JET_PCSTR              szColumnName,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetGetTableColumnInfoW(
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    _In_opt_ JET_PCWSTR             szColumnName,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetTableColumnInfo JetGetTableColumnInfoW
#else
#define JetGetTableColumnInfo JetGetTableColumnInfoA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetGetColumnInfoA JetGetColumnInfo
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetColumnInfoA(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_PCSTR                  szTableName,
    _In_opt_ JET_PCSTR              pColumnNameOrId,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetGetColumnInfoW(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_PCWSTR                 szTableName,
    _In_opt_ JET_PCWSTR             pwColumnNameOrId,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetColumnInfo JetGetColumnInfoW
#else
#define JetGetColumnInfo JetGetColumnInfoA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetAddColumnA JetAddColumn
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetAddColumnA(
    _In_ JET_SESID                              sesid,
    _In_ JET_TABLEID                            tableid,
    _In_ JET_PCSTR                              szColumnName,
    _In_ const JET_COLUMNDEF *                  pcolumndef,
    _In_reads_bytes_opt_( cbDefault ) const void *  pvDefault,
    _In_ unsigned long                          cbDefault,
    _Out_opt_ JET_COLUMNID *                    pcolumnid );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetAddColumnW(
    _In_ JET_SESID                              sesid,
    _In_ JET_TABLEID                            tableid,
    _In_ JET_PCWSTR                             szColumnName,
    _In_ const JET_COLUMNDEF *                  pcolumndef,
    _In_reads_bytes_opt_( cbDefault ) const void *  pvDefault,
    _In_ unsigned long                          cbDefault,
    _Out_opt_ JET_COLUMNID *                    pcolumnid );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetAddColumn JetAddColumnW
#else
#define JetAddColumn JetAddColumnA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetDeleteColumnA JetDeleteColumn
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDeleteColumnA(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCSTR      szColumnName );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDeleteColumnW(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCWSTR     szColumnName );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetDeleteColumn JetDeleteColumnW
#else
#define JetDeleteColumn JetDeleteColumnA
#endif
#endif


#if ( JET_VERSION >= 0x0501 )
#if ( JET_VERSION < 0x0600 )
#define JetDeleteColumn2A JetDeleteColumn2
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDeleteColumn2A(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _In_ JET_PCSTR          szColumnName,
    _In_ const JET_GRBIT    grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDeleteColumn2W(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _In_ JET_PCWSTR         szColumnName,
    _In_ const JET_GRBIT    grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetDeleteColumn2 JetDeleteColumn2W
#else
#define JetDeleteColumn2 JetDeleteColumn2A
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetRenameColumnA JetRenameColumn
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetRenameColumnA(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCSTR      szName,
    _In_ JET_PCSTR      szNameNew,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetRenameColumnW(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCWSTR     szName,
    _In_ JET_PCWSTR     szNameNew,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetRenameColumn JetRenameColumnW
#else
#define JetRenameColumn JetRenameColumnA
#endif
#endif


#endif


#if ( JET_VERSION < 0x0600 )
#define JetSetColumnDefaultValueA JetSetColumnDefaultValue
#endif


#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetColumnDefaultValueA(
    _In_ JET_SESID                      sesid,
    _In_ JET_DBID                       dbid,
    _In_ JET_PCSTR                      szTableName,
    _In_ JET_PCSTR                      szColumnName,
    _In_reads_bytes_( cbData ) const void * pvData,
    _In_ const unsigned long            cbData,
    _In_ const JET_GRBIT                grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetColumnDefaultValueW(
    _In_ JET_SESID                      sesid,
    _In_ JET_DBID                       dbid,
    _In_ JET_PCWSTR                     szTableName,
    _In_ JET_PCWSTR                     szColumnName,
    _In_reads_bytes_( cbData ) const void * pvData,
    _In_ const unsigned long            cbData,
    _In_ const JET_GRBIT                grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetSetColumnDefaultValue JetSetColumnDefaultValueW
#else
#define JetSetColumnDefaultValue JetSetColumnDefaultValueA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetGetTableIndexInfoA JetGetTableIndexInfo
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetTableIndexInfoA(
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    _In_opt_ JET_PCSTR              szIndexName,
    _Out_writes_bytes_( cbResult ) void *   pvResult,
    _In_ unsigned long              cbResult,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetTableIndexInfoW(
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    _In_opt_ JET_PCWSTR             szIndexName,
    _Out_writes_bytes_( cbResult ) void *   pvResult,
    _In_ unsigned long              cbResult,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetTableIndexInfo JetGetTableIndexInfoW
#else
#define JetGetTableIndexInfo JetGetTableIndexInfoA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetGetIndexInfoA JetGetIndexInfo
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetIndexInfoA(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_PCSTR                  szTableName,
    _In_opt_ JET_PCSTR              szIndexName,
    _Out_writes_bytes_( cbResult ) void *   pvResult,
    _In_ unsigned long              cbResult,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetIndexInfoW(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_PCWSTR                 szTableName,
    _In_opt_ JET_PCWSTR             szIndexName,
    _Out_writes_bytes_( cbResult ) void *   pvResult,
    _In_ unsigned long              cbResult,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetIndexInfo JetGetIndexInfoW
#else
#define JetGetIndexInfo JetGetIndexInfoA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetCreateIndexA JetCreateIndex
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateIndexA(
    _In_ JET_SESID                      sesid,
    _In_ JET_TABLEID                    tableid,
    _In_ JET_PCSTR                      szIndexName,
    _In_ JET_GRBIT                      grbit,
    _In_reads_bytes_( cbKey ) const char *  szKey,
    _In_ unsigned long                  cbKey,
    _In_ unsigned long                  lDensity );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateIndexW(
    _In_ JET_SESID                      sesid,
    _In_ JET_TABLEID                    tableid,
    _In_ JET_PCWSTR                     szIndexName,
    _In_ JET_GRBIT                      grbit,
    _In_reads_bytes_( cbKey ) const WCHAR * szKey,
    _In_ unsigned long                  cbKey,
    _In_ unsigned long                  lDensity );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateIndex JetCreateIndexW
#else
#define JetCreateIndex JetCreateIndexA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetCreateIndex2A JetCreateIndex2
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateIndex2A(
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    _In_reads_( cIndexCreate ) JET_INDEXCREATE_A *  pindexcreate,
    _In_ unsigned long                              cIndexCreate );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateIndex2W(
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    _In_reads_( cIndexCreate ) JET_INDEXCREATE_W *  pindexcreate,
    _In_ unsigned long                              cIndexCreate );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateIndex2 JetCreateIndex2W
#else
#define JetCreateIndex2 JetCreateIndex2A
#endif
#endif

#if ( JET_VERSION >= 0x0601 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateIndex3A(
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    _In_reads_( cIndexCreate ) JET_INDEXCREATE2_A *pindexcreate,
    _In_ unsigned long                              cIndexCreate );

JET_ERR JET_API
JetCreateIndex3W(
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    _In_reads_( cIndexCreate ) JET_INDEXCREATE2_W *pindexcreate,
    _In_ unsigned long                              cIndexCreate );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateIndex3 JetCreateIndex3W
#else
#define JetCreateIndex3 JetCreateIndex3A
#endif

#endif

#if ( JET_VERSION >= 0x0602 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateIndex4A(
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    _In_reads_( cIndexCreate ) JET_INDEXCREATE3_A *pindexcreate,
    _In_ unsigned long                              cIndexCreate );

#endif
#pragma endregion

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateIndex4W(
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    _In_reads_( cIndexCreate ) JET_INDEXCREATE3_W *pindexcreate,
    _In_ unsigned long                              cIndexCreate );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateIndex4 JetCreateIndex4W
#else
#define JetCreateIndex4 JetCreateIndex4A
#endif

#endif

#if ( JET_VERSION < 0x0600 )
#define JetDeleteIndexA JetDeleteIndex
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDeleteIndexA(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCSTR      szIndexName );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDeleteIndexW(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCWSTR     szIndexName );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetDeleteIndex JetDeleteIndexW
#else
#define JetDeleteIndex JetDeleteIndexA
#endif
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetBeginTransaction(
    _In_ JET_SESID  sesid );

JET_ERR JET_API
JetBeginTransaction2(
    _In_ JET_SESID  sesid,
    _In_ JET_GRBIT  grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0602 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetBeginTransaction3(
    _In_ JET_SESID      sesid,
    _In_ __int64        trxid,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#endif


#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetPrepareToCommitTransaction(
    _In_ JET_SESID                      sesid,
    _In_reads_bytes_( cbData ) const void * pvData,
    _In_ unsigned long                  cbData,
    _In_ JET_GRBIT                      grbit );

#endif
#pragma endregion


#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCommitTransaction(
    _In_ JET_SESID  sesid,
    _In_ JET_GRBIT  grbit );

#if ( JET_VERSION >= 0x0602 )
JET_ERR JET_API
JetCommitTransaction2(
    _In_ JET_SESID              sesid,
    _In_ JET_GRBIT              grbit,
    _In_ unsigned long          cmsecDurableCommit,
    _Out_opt_ JET_COMMIT_ID *   pCommitId );
#endif

JET_ERR JET_API
JetRollback(
    _In_ JET_SESID  sesid,
    _In_ JET_GRBIT  grbit );

#endif
#pragma endregion

#if ( JET_VERSION < 0x0600 )
#define JetGetDatabaseInfoA JetGetDatabaseInfo
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetGetDatabaseInfoA(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetDatabaseInfoW(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetDatabaseInfo JetGetDatabaseInfoW
#else
#define JetGetDatabaseInfo JetGetDatabaseInfoA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetGetDatabaseFileInfoA JetGetDatabaseFileInfo
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetDatabaseFileInfoA(
    _In_ JET_PCSTR                  szDatabaseName,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetDatabaseFileInfoW(
    _In_ JET_PCWSTR                 szDatabaseName,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetDatabaseFileInfo JetGetDatabaseFileInfoW
#else
#define JetGetDatabaseFileInfo JetGetDatabaseFileInfoA
#endif
#endif

#if ( JET_VERSION >= 0x0600 )


#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetGetLogFileInfoA(
    _In_ JET_PCSTR                  szLog,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ const unsigned long        cbMax,
    _In_ const unsigned long        InfoLevel );

JET_ERR JET_API
JetGetLogFileInfoW(
    _In_ JET_PCWSTR                 szLog,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ const unsigned long        cbMax,
    _In_ const unsigned long        InfoLevel );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetLogFileInfo JetGetLogFileInfoW
#else
#define JetGetLogFileInfo JetGetLogFileInfoA
#endif
#endif

#if ( JET_VERSION < 0x0600 )
#define JetOpenDatabaseA JetOpenDatabase
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOpenDatabaseA(
    _In_ JET_SESID      sesid,
    _In_ JET_PCSTR      szFilename,
    _In_opt_ JET_PCSTR  szConnect,
    _Out_ JET_DBID*     pdbid,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOpenDatabaseW(
    _In_ JET_SESID      sesid,
    _In_ JET_PCWSTR     szFilename,
    _In_opt_ JET_PCWSTR szConnect,
    _Out_ JET_DBID*     pdbid,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetOpenDatabase JetOpenDatabaseW
#else
#define JetOpenDatabase JetOpenDatabaseA
#endif
#endif

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCloseDatabase(
    _In_ JET_SESID  sesid,
    _In_ JET_DBID   dbid,
    _In_ JET_GRBIT  grbit );

#endif
#pragma endregion

#if ( JET_VERSION < 0x0600 )
#define JetOpenTableA JetOpenTable
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOpenTableA(
    _In_ JET_SESID                                  sesid,
    _In_ JET_DBID                                   dbid,
    _In_ JET_PCSTR                                  szTableName,
    _In_reads_bytes_opt_( cbParameters ) const void *   pvParameters,
    _In_ unsigned long                              cbParameters,
    _In_ JET_GRBIT                                  grbit,
    _Out_ JET_TABLEID *                             ptableid );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOpenTableW(
    _In_ JET_SESID                                  sesid,
    _In_ JET_DBID                                   dbid,
    _In_ JET_PCWSTR                                 szTableName,
    _In_reads_bytes_opt_( cbParameters ) const void *   pvParameters,
    _In_ unsigned long                              cbParameters,
    _In_ JET_GRBIT                                  grbit,
    _Out_ JET_TABLEID *                             ptableid );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetOpenTable JetOpenTableW
#else
#define JetOpenTable JetOpenTableA
#endif
#endif


#if ( JET_VERSION >= 0x0501 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetTableSequential(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_GRBIT      grbit );

JET_ERR JET_API
JetResetTableSequential(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#endif

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCloseTable(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid );

JET_ERR JET_API
JetDelete(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid );

#endif
#pragma endregion

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetUpdate(
    _In_ JET_SESID                                          sesid,
    _In_ JET_TABLEID                                        tableid,
    _Out_writes_bytes_to_opt_( cbBookmark, *pcbActual ) void *  pvBookmark,
    _In_ unsigned long                                      cbBookmark,
    _Out_opt_ unsigned long *                               pcbActual );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0502 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetUpdate2(
    _In_ JET_SESID                                          sesid,
    _In_ JET_TABLEID                                        tableid,
    _Out_writes_bytes_to_opt_( cbBookmark, *pcbActual ) void *  pvBookmark,
    _In_ unsigned long                                      cbBookmark,
    _Out_opt_ unsigned long *                               pcbActual,
    _In_ const JET_GRBIT                                    grbit );

#endif
#pragma endregion

#endif

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetEscrowUpdate(
    _In_ JET_SESID                                          sesid,
    _In_ JET_TABLEID                                        tableid,
    _In_ JET_COLUMNID                                       columnid,
    _In_reads_bytes_( cbMax ) void *                        pv,
    _In_ unsigned long                                      cbMax,
    _Out_writes_bytes_to_opt_( cbOldMax, *pcbOldActual ) void * pvOld,
    _In_ unsigned long                                      cbOldMax,
    _Out_opt_ unsigned long *                               pcbOldActual,
    _In_ JET_GRBIT                                          grbit );

JET_ERR JET_API
JetRetrieveColumn(
    _In_ JET_SESID                                      sesid,
    _In_ JET_TABLEID                                    tableid,
    _In_ JET_COLUMNID                                   columnid,
    _Out_writes_bytes_to_opt_( cbData, min( cbData, *pcbActual ) ) void *   pvData,
    _In_ unsigned long                                  cbData,
    _Out_opt_ unsigned long *                           pcbActual,
    _In_ JET_GRBIT                                      grbit,
    _Inout_opt_ JET_RETINFO *                           pretinfo );

JET_ERR JET_API
JetRetrieveColumns(
    _In_ JET_SESID                                              sesid,
    _In_ JET_TABLEID                                            tableid,
    _Inout_updates_opt_( cretrievecolumn ) JET_RETRIEVECOLUMN * pretrievecolumn,
    _In_ unsigned long                                          cretrievecolumn );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetEnumerateColumns(
    _In_ JET_SESID                                              sesid,
    _In_ JET_TABLEID                                            tableid,
    _In_ unsigned long                                          cEnumColumnId,
    _In_reads_opt_( cEnumColumnId ) JET_ENUMCOLUMNID *          rgEnumColumnId,
    _Out_ unsigned long *                                       pcEnumColumn,
    _Outptr_result_buffer_( *pcEnumColumn ) JET_ENUMCOLUMN **   prgEnumColumn,
    _In_ JET_PFNREALLOC                                         pfnRealloc,
    _In_opt_ void *                                             pvReallocContext,
    _In_ unsigned long                                          cbDataMost,
    _In_ JET_GRBIT                                              grbit );

#endif
#pragma endregion

#endif


#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetRetrieveTaggedColumnList(
    _In_ JET_SESID                                                                              sesid,
    _In_ JET_TABLEID                                                                            tableid,
    _Out_ unsigned long *                                                                       pcColumns,
    _Out_writes_bytes_to_opt_( cbData, *pcColumns * sizeof( JET_RETRIEVEMULTIVALUECOUNT ) )     void *  pvData,
    _In_ unsigned long                                                                          cbData,
    _In_ JET_COLUMNID                                                                           columnidStart,
    _In_ JET_GRBIT                                                                              grbit );

#endif
#pragma endregion


#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

#if ( JET_VERSION >= 0x0600 )
JET_ERR JET_API
JetGetRecordSize(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _Inout_ JET_RECSIZE *   precsize,
    _In_ const JET_GRBIT    grbit );

#endif

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0601 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetRecordSize2(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _Inout_ JET_RECSIZE2 *  precsize,
    _In_ const JET_GRBIT    grbit );

#endif
#pragma endregion

#endif


#if ( JET_VERSION >= 0x0A01 )

JET_ERR JET_API
JetGetRecordSize3(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _Inout_ JET_RECSIZE3 *    precsize,
    _In_ const JET_GRBIT    grbit );

#endif


#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetColumn(
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    _In_ JET_COLUMNID                       columnid,
    _In_reads_bytes_opt_( cbData ) const void * pvData,
    _In_ unsigned long                      cbData,
    _In_ JET_GRBIT                          grbit,
    _In_opt_ JET_SETINFO *                  psetinfo );

JET_ERR JET_API
JetSetColumns(
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    _In_reads_opt_( csetcolumn ) JET_SETCOLUMN *    psetcolumn,
    _In_ unsigned long                              csetcolumn );

JET_ERR JET_API
JetPrepareUpdate(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ unsigned long  prep );

JET_ERR JET_API
JetGetRecordPosition(
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    _Out_writes_bytes_( cbRecpos ) JET_RECPOS * precpos,
    _In_ unsigned long                      cbRecpos );

JET_ERR JET_API
JetGotoPosition(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_RECPOS *   precpos );

#endif
#pragma endregion

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetCursorInfo(
    _In_ JET_SESID                  sesid,
    _In_ JET_TABLEID                tableid,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

JET_ERR JET_API
JetDupCursor(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _Out_ JET_TABLEID * ptableid,
    _In_ JET_GRBIT      grbit );

#if ( JET_VERSION < 0x0600 )
#define JetGetCurrentIndexA JetGetCurrentIndex
#endif

JET_ERR JET_API
JetGetCurrentIndexA(
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    _Out_writes_bytes_( cbIndexName ) JET_PSTR  szIndexName,
    _In_ unsigned long                      cbIndexName );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetCurrentIndexW(
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    _Out_writes_bytes_( cbIndexName ) JET_PWSTR szIndexName,
    _In_ unsigned long                      cbIndexName );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetCurrentIndex JetGetCurrentIndexW
#else
#define JetGetCurrentIndex JetGetCurrentIndexA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetSetCurrentIndexA JetSetCurrentIndex
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetCurrentIndexA(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_opt_ JET_PCSTR  szIndexName );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetCurrentIndexW(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_opt_ JET_PCWSTR szIndexName );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetSetCurrentIndex JetSetCurrentIndexW
#else
#define JetSetCurrentIndex JetSetCurrentIndexA
#endif
#endif

#if ( JET_VERSION < 0x0600 )
#define JetSetCurrentIndex2A JetSetCurrentIndex2
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetCurrentIndex2A(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_opt_ JET_PCSTR  szIndexName,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetCurrentIndex2W(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_opt_ JET_PCWSTR szIndexName,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetSetCurrentIndex2 JetSetCurrentIndex2W
#else
#define JetSetCurrentIndex2 JetSetCurrentIndex2A
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetSetCurrentIndex3A JetSetCurrentIndex3
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetCurrentIndex3A(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_opt_ JET_PCSTR  szIndexName,
    _In_ JET_GRBIT      grbit,
    _In_ unsigned long  itagSequence );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetCurrentIndex3W(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_opt_ JET_PCWSTR szIndexName,
    _In_ JET_GRBIT      grbit,
    _In_ unsigned long  itagSequence );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetSetCurrentIndex3 JetSetCurrentIndex3W
#else
#define JetSetCurrentIndex3 JetSetCurrentIndex3A
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetSetCurrentIndex4A JetSetCurrentIndex4
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetCurrentIndex4A(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _In_opt_ JET_PCSTR      szIndexName,
    _In_opt_ JET_INDEXID *  pindexid,
    _In_ JET_GRBIT          grbit,
    _In_ unsigned long      itagSequence );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetCurrentIndex4W(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _In_opt_ JET_PCWSTR     szIndexName,
    _In_opt_ JET_INDEXID *  pindexid,
    _In_ JET_GRBIT          grbit,
    _In_ unsigned long      itagSequence );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetSetCurrentIndex4 JetSetCurrentIndex4W
#else
#define JetSetCurrentIndex4 JetSetCurrentIndex4A
#endif
#endif


#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetMove(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ long           cRow,
    _In_ JET_GRBIT      grbit );

#if ( JET_VERSION >= 0x0602 )
JET_ERR JET_API
JetSetCursorFilter(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _In_reads_( cColumnFilters ) JET_INDEX_COLUMN *rgColumnFilters,
    _In_ unsigned long      cColumnFilters,
    _In_ JET_GRBIT          grbit );
#endif

#endif
#pragma endregion

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetLock(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetMakeKey(
    _In_ JET_SESID                          sesid,
    _In_ JET_TABLEID                        tableid,
    _In_reads_bytes_opt_( cbData ) const void * pvData,
    _In_ unsigned long                      cbData,
    _In_ JET_GRBIT                          grbit );

JET_ERR JET_API
JetSeek(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0601 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetPrereadKeys(
    _In_ JET_SESID                                      sesid,
    _In_ JET_TABLEID                                    tableid,
    _In_reads_(ckeys) const void **                 rgpvKeys,
    _In_reads_(ckeys) const unsigned long *         rgcbKeys,
    _In_ long                                           ckeys,
    _Out_opt_ long *                                    pckeysPreread,
    _In_ JET_GRBIT                                      grbit );

#endif
#pragma endregion

#endif

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

#if ( JET_VERSION >= 0x0602 )

JET_ERR JET_API
JetPrereadIndexRanges(
    _In_ JET_SESID                                              sesid,
    _In_ JET_TABLEID                                            tableid,
    _In_reads_(cIndexRanges) const JET_INDEX_RANGE * const      rgIndexRanges,
    _In_ const unsigned long                                    cIndexRanges,
    _Out_opt_ unsigned long * const                             pcRangesPreread,
    _In_reads_(ccolumnidPreread) const JET_COLUMNID * const     rgcolumnidPreread,
    _In_ const unsigned long                                    ccolumnidPreread,
    _In_ JET_GRBIT                                              grbit );

#endif

JET_ERR JET_API
JetGetBookmark(
    _In_ JET_SESID                                      sesid,
    _In_ JET_TABLEID                                    tableid,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) void *   pvBookmark,
    _In_ unsigned long                                  cbMax,
    _Out_opt_ unsigned long *                           pcbActual );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetSecondaryIndexBookmark(
    _In_ JET_SESID                                                                  sesid,
    _In_ JET_TABLEID                                                                tableid,
    _Out_writes_bytes_to_opt_( cbSecondaryKeyMax, *pcbSecondaryKeyActual ) void *       pvSecondaryKey,
    _In_ unsigned long                                                              cbSecondaryKeyMax,
    _Out_opt_ unsigned long *                                                       pcbSecondaryKeyActual,
    _Out_writes_bytes_to_opt_( cbPrimaryBookmarkMax, *pcbPrimaryBookmarkActual ) void * pvPrimaryBookmark,
    _In_ unsigned long                                                              cbPrimaryBookmarkMax,
    _Out_opt_ unsigned long *                                                       pcbPrimaryBookmarkActual,
    _In_ const JET_GRBIT                                                            grbit );

#endif
#pragma endregion

#endif


#if ( JET_VERSION < 0x0600 )
#define JetCompactA JetCompact
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCompactA(
    _In_ JET_SESID              sesid,
    _In_ JET_PCSTR              szDatabaseSrc,
    _In_ JET_PCSTR              szDatabaseDest,
    _In_ JET_PFNSTATUS          pfnStatus,
    _In_opt_ JET_CONVERT_A *    pconvert,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCompactW(
    _In_ JET_SESID              sesid,
    _In_ JET_PCWSTR             szDatabaseSrc,
    _In_ JET_PCWSTR             szDatabaseDest,
    _In_ JET_PFNSTATUS          pfnStatus,
    _In_opt_ JET_CONVERT_W *    pconvert,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetCompact JetCompactW
#else
#define JetCompact JetCompactA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetDefragmentA JetDefragment
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDefragmentA(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    _In_opt_ JET_PCSTR          szTableName,
    _Inout_opt_ unsigned long * pcPasses,
    _Inout_opt_ unsigned long * pcSeconds,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDefragmentW(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    _In_opt_ JET_PCWSTR         szTableName,
    _Inout_opt_ unsigned long * pcPasses,
    _Inout_opt_ unsigned long * pcSeconds,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetDefragment JetDefragmentW
#else
#define JetDefragment JetDefragmentA
#endif
#endif


#if ( JET_VERSION >= 0x0501 )
#if ( JET_VERSION < 0x0600 )
#define JetDefragment2A JetDefragment2
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDefragment2A(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    _In_opt_ JET_PCSTR          szTableName,
    _Inout_opt_ unsigned long * pcPasses,
    _Inout_opt_ unsigned long * pcSeconds,
    _In_ JET_CALLBACK           callback,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDefragment2W(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    _In_opt_ JET_PCWSTR         szTableName,
    _Inout_opt_ unsigned long * pcPasses,
    _Inout_opt_ unsigned long * pcSeconds,
    _In_ JET_CALLBACK           callback,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetDefragment2 JetDefragment2W
#else
#define JetDefragment2 JetDefragment2A
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetDefragment3A JetDefragment3
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDefragment3A(
    _In_ JET_SESID              sesid,
    _In_ JET_PCSTR              szDatabaseName,
    _In_opt_ JET_PCSTR          szTableName,
    _Inout_opt_ unsigned long * pcPasses,
    _Inout_opt_ unsigned long * pcSeconds,
    _In_ JET_CALLBACK           callback,
    _In_ void *                 pvContext,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDefragment3W(
    _In_ JET_SESID              sesid,
    _In_ JET_PCWSTR             szDatabaseName,
    _In_opt_ JET_PCWSTR         szTableName,
    _Inout_opt_ unsigned long * pcPasses,
    _Inout_opt_ unsigned long * pcSeconds,
    _In_ JET_CALLBACK           callback,
    _In_ void *                 pvContext,
    _In_ JET_GRBIT              grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetDefragment3 JetDefragment3W
#else
#define JetDefragment3 JetDefragment3A
#endif
#endif

#endif


#if ( JET_VERSION >= 0x0601 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetDatabaseScan(
    _In_ JET_SESID      sesid,
    _In_ JET_DBID       dbid,
    _Inout_opt_ unsigned long * pcSecondsMax,
    _In_ unsigned long  cmsecSleep,
    _In_ JET_CALLBACK   pfnCallback,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#endif

#if ( JET_VERSION < 0x0600 )
#define JetConvertDDLA JetConvertDDL
#endif

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetConvertDDLA(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_OPDDLCONV              convtyp,
    _Out_writes_bytes_( cbData ) void * pvData,
    _In_ unsigned long              cbData );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetConvertDDLW(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _In_ JET_OPDDLCONV              convtyp,
    _Out_writes_bytes_( cbData ) void * pvData,
    _In_ unsigned long              cbData );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetConvertDDL JetConvertDDLW
#else
#define JetConvertDDL JetConvertDDLA
#endif
#endif


#if ( JET_VERSION >= 0x0501 )
#if ( JET_VERSION < 0x0600 )
#define JetUpgradeDatabaseA JetUpgradeDatabase
#endif

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetUpgradeDatabaseA(
    _In_ JET_SESID          sesid,
    _In_ JET_PCSTR          szDbFileName,
    _Reserved_ JET_PCSTR    szSLVFileName,
    _In_ const JET_GRBIT    grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetUpgradeDatabaseW(
    _In_ JET_SESID          sesid,
    _In_ JET_PCWSTR         szDbFileName,
    _Reserved_ JET_PCWSTR   szSLVFileName,
    _In_ const JET_GRBIT    grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetUpgradeDatabase JetUpgradeDatabaseW
#else
#define JetUpgradeDatabase JetUpgradeDatabaseA
#endif
#endif

#endif

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetSetMaxDatabaseSize(
    _In_ JET_SESID      sesid,
    _In_ JET_DBID       dbid,
    _In_ unsigned long  cpg,
    _In_ JET_GRBIT      grbit );

JET_ERR JET_API
JetGetMaxDatabaseSize(
    _In_ JET_SESID          sesid,
    _In_ JET_DBID           dbid,
    _Out_ unsigned long *   pcpg,
    _In_ JET_GRBIT          grbit );

#endif
#pragma endregion

#endif

#if ( JET_VERSION < 0x0600 )
#define JetSetDatabaseSizeA JetSetDatabaseSize
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetDatabaseSizeA(
    _In_ JET_SESID          sesid,
    _In_ JET_PCSTR          szDatabaseName,
    _In_ unsigned long      cpg,
    _Out_ unsigned long *   pcpgReal );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetDatabaseSizeW(
    _In_ JET_SESID          sesid,
    _In_ JET_PCWSTR         szDatabaseName,
    _In_ unsigned long      cpg,
    _Out_ unsigned long *   pcpgReal );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetSetDatabaseSize JetSetDatabaseSizeW
#else
#define JetSetDatabaseSize JetSetDatabaseSizeA
#endif
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGrowDatabase(
    _In_ JET_SESID          sesid,
    _In_ JET_DBID           dbid,
    _In_ unsigned long      cpg,
    _In_ unsigned long *    pcpgReal );

#endif
#pragma endregion

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

#if ( JET_VERSION >= 0x0602 )
JET_ERR JET_API
JetResizeDatabase(
    _In_  JET_SESID         sesid,
    _In_  JET_DBID          dbid,
    _In_  unsigned long     cpgTarget,
    _Out_ unsigned long *   pcpgActual,
    _In_  const JET_GRBIT   grbit );
#endif

JET_ERR JET_API
JetSetSessionContext(
    _In_ JET_SESID      sesid,
    _In_ JET_API_PTR    ulContext );

JET_ERR JET_API
JetResetSessionContext(
    _In_ JET_SESID      sesid );

#endif
#pragma endregion



#if ( JET_VERSION < 0x0600 )
#define JetDBUtilitiesA JetDBUtilities
#endif
JET_ERR JET_API JetDBUtilitiesA( JET_DBUTIL_A *pdbutil );

#if ( JET_VERSION >= 0x0600 )
JET_ERR JET_API JetDBUtilitiesW( JET_DBUTIL_W *pdbutil );
#ifdef JET_UNICODE
#define JetDBUtilities JetDBUtilitiesW
#else
#define JetDBUtilities JetDBUtilitiesA
#endif
#endif



#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGotoBookmark(
    _In_ JET_SESID                      sesid,
    _In_ JET_TABLEID                    tableid,
    _In_reads_bytes_( cbBookmark ) void *   pvBookmark,
    _In_ unsigned long                  cbBookmark );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGotoSecondaryIndexBookmark(
    _In_ JET_SESID                              sesid,
    _In_ JET_TABLEID                            tableid,
    _In_reads_bytes_( cbSecondaryKey ) void *       pvSecondaryKey,
    _In_ unsigned long                          cbSecondaryKey,
    _In_reads_bytes_opt_( cbPrimaryBookmark ) void *    pvPrimaryBookmark,
    _In_ unsigned long                          cbPrimaryBookmark,
    _In_ const JET_GRBIT                        grbit );


#endif
#pragma endregion

#endif

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetIntersectIndexes(
    _In_ JET_SESID                              sesid,
    _In_reads_( cindexrange ) JET_INDEXRANGE *  rgindexrange,
    _In_ unsigned long                          cindexrange,
    _Inout_ JET_RECORDLIST *                    precordlist,
    _In_ JET_GRBIT                              grbit );

#endif
#pragma endregion

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetComputeStats(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid );

JET_ERR JET_API
JetOpenTempTable(
    _In_ JET_SESID                                  sesid,
    _In_reads_( ccolumn ) const JET_COLUMNDEF * prgcolumndef,
    _In_ unsigned long                              ccolumn,
    _In_ JET_GRBIT                                  grbit,
    _Out_ JET_TABLEID *                             ptableid,
    _Out_writes_( ccolumn ) JET_COLUMNID *          prgcolumnid );

JET_ERR JET_API
JetOpenTempTable2(
    _In_ JET_SESID                                  sesid,
    _In_reads_( ccolumn ) const JET_COLUMNDEF * prgcolumndef,
    _In_ unsigned long                              ccolumn,
    _In_ unsigned long                              lcid,
    _In_ JET_GRBIT                                  grbit,
    _Out_ JET_TABLEID *                             ptableid,
    _Out_writes_( ccolumn ) JET_COLUMNID *          prgcolumnid );

#endif
#pragma endregion

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOpenTempTable3(
    _In_ JET_SESID                                  sesid,
    _In_reads_( ccolumn ) const JET_COLUMNDEF * prgcolumndef,
    _In_ unsigned long                              ccolumn,
    _In_opt_ JET_UNICODEINDEX *                     pidxunicode,
    _In_ JET_GRBIT                                  grbit,
    _Out_ JET_TABLEID *                             ptableid,
    _Out_writes_( ccolumn ) JET_COLUMNID *          prgcolumnid );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOpenTemporaryTable(
    _In_ JET_SESID                  sesid,
    _In_ JET_OPENTEMPORARYTABLE *   popentemporarytable );

#endif
#pragma endregion

#endif

#if ( JET_VERSION >= 0x0602 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOpenTemporaryTable2(
    _In_ JET_SESID                  sesid,
    _In_ JET_OPENTEMPORARYTABLE2 *  popentemporarytable );

#endif
#pragma endregion

#endif


#if ( JET_VERSION < 0x0600 )
#define JetBackupA JetBackup
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetBackupA(
    _In_ JET_PCSTR      szBackupPath,
    _In_ JET_GRBIT      grbit,
    _In_opt_ JET_PFNSTATUS  pfnStatus );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetBackupW(
    _In_ JET_PCWSTR     szBackupPath,
    _In_ JET_GRBIT      grbit,
    _In_opt_ JET_PFNSTATUS  pfnStatus );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetBackup JetBackupW
#else
#define JetBackup JetBackupA
#endif
#endif

#if ( JET_VERSION >= 0x0501 )
#if ( JET_VERSION < 0x0600 )
#define JetBackupInstanceA JetBackupInstance
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetBackupInstanceA(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCSTR      szBackupPath,
    _In_ JET_GRBIT      grbit,
    _In_opt_ JET_PFNSTATUS  pfnStatus );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetBackupInstanceW(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCWSTR     szBackupPath,
    _In_ JET_GRBIT      grbit,
    _In_opt_ JET_PFNSTATUS  pfnStatus );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetBackupInstance JetBackupInstanceW
#else
#define JetBackupInstance JetBackupInstanceA
#endif
#endif

#endif


#if ( JET_VERSION < 0x0600 )
#define JetRestoreA JetRestore
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetRestoreA(
    _In_ JET_PCSTR      szSource,
    _In_opt_ JET_PFNSTATUS  pfn );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetRestoreW(
    _In_ JET_PCWSTR     szSource,
    _In_opt_ JET_PFNSTATUS  pfn );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetRestore JetRestoreW
#else
#define JetRestore JetRestoreA
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetRestore2A JetRestore2
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetRestore2A(
    _In_ JET_PCSTR      sz,
    _In_opt_ JET_PCSTR  szDest,
    _In_opt_ JET_PFNSTATUS  pfn );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetRestore2W(
    _In_ JET_PCWSTR     sz,
    _In_opt_ JET_PCWSTR szDest,
    _In_opt_ JET_PFNSTATUS  pfn );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetRestore2 JetRestore2W
#else
#define JetRestore2 JetRestore2A
#endif
#endif


#if ( JET_VERSION >= 0x0501 )
#if ( JET_VERSION < 0x0600 )
#define JetRestoreInstanceA JetRestoreInstance
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetRestoreInstanceA(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCSTR      sz,
    _In_opt_ JET_PCSTR  szDest,
    _In_opt_ JET_PFNSTATUS  pfn );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetRestoreInstanceW(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCWSTR     sz,
    _In_opt_ JET_PCWSTR szDest,
    _In_opt_ JET_PFNSTATUS  pfn );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetRestoreInstance JetRestoreInstanceW
#else
#define JetRestoreInstance JetRestoreInstanceA
#endif
#endif

#endif

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetIndexRange(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableidSrc,
    _In_ JET_GRBIT      grbit );

JET_ERR JET_API
JetIndexRecordCount(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _Out_ unsigned long *   pcrec,
    _In_ unsigned long      crecMax );

#if ( JET_VERSION >= 0x0A01 )

JET_ERR JET_API
JetIndexRecordCount2(
    _In_ JET_SESID              sesid,
    _In_ JET_TABLEID            tableid,
    _Out_ unsigned __int64 *    pcrec,
    _In_ unsigned __int64       crecMax );

#endif

JET_ERR JET_API
JetRetrieveKey(
    _In_ JET_SESID                                      sesid,
    _In_ JET_TABLEID                                    tableid,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) void *   pvKey,
    _In_ unsigned long                                  cbMax,
    _Out_opt_ unsigned long *                           pcbActual,
    _In_ JET_GRBIT                                      grbit );

#endif
#pragma endregion

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetBeginExternalBackup(
    _In_ JET_GRBIT grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetBeginExternalBackupInstance(
    _In_ JET_INSTANCE instance,
    _In_ JET_GRBIT grbit );

#endif
#pragma endregion

#endif

#if ( JET_VERSION >= 0x0601 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API JetBeginSurrogateBackup(
    _In_    JET_INSTANCE    instance,
    _In_        unsigned long       lgenFirst,
    _In_        unsigned long       lgenLast,
    _In_        JET_GRBIT       grbit );

#endif
#pragma endregion

#endif

#if ( JET_VERSION < 0x0600 )
#define JetGetAttachInfoA JetGetAttachInfo
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetAttachInfoA(
#if ( JET_VERSION < 0x0600 )
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) void *   pv,
#else
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PSTR szzDatabases,
#endif
    _In_ unsigned long                                  cbMax,
    _Out_opt_ unsigned long *                           pcbActual );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetAttachInfoW(
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PWSTR    wszzDatabases,
    _In_ unsigned long                                      cbMax,
    _Out_opt_ unsigned long *                               pcbActual );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetAttachInfo JetGetAttachInfoW
#else
#define JetGetAttachInfo JetGetAttachInfoA
#endif
#endif


#if ( JET_VERSION >= 0x0501 )
#if ( JET_VERSION < 0x0600 )
#define JetGetAttachInfoInstanceA JetGetAttachInfoInstance
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetAttachInfoInstanceA(
    _In_ JET_INSTANCE                                   instance,
#if ( JET_VERSION < 0x0600 )
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) void *   pv,
#else
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PSTR szzDatabases,
#endif
    _In_ unsigned long                                  cbMax,
    _Out_opt_ unsigned long *                           pcbActual );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetAttachInfoInstanceW(
    _In_ JET_INSTANCE                                       instance,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PWSTR    szzDatabases,
    _In_ unsigned long                                      cbMax,
    _Out_opt_ unsigned long *                               pcbActual );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetAttachInfoInstance JetGetAttachInfoInstanceW
#else
#define JetGetAttachInfoInstance JetGetAttachInfoInstanceA
#endif
#endif

#endif


#if ( JET_VERSION < 0x0600 )
#define JetOpenFileA JetOpenFile
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOpenFileA(
    _In_ JET_PCSTR          szFileName,
    _Out_ JET_HANDLE *      phfFile,
    _Out_ unsigned long *   pulFileSizeLow,
    _Out_ unsigned long *   pulFileSizeHigh );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOpenFileW(
    _In_ JET_PCWSTR         szFileName,
    _Out_ JET_HANDLE *      phfFile,
    _Out_ unsigned long *   pulFileSizeLow,
    _Out_ unsigned long *   pulFileSizeHigh );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetOpenFile JetOpenFileW
#else
#define JetOpenFile JetOpenFileA
#endif
#endif


#if ( JET_VERSION >= 0x0501 )
#if ( JET_VERSION < 0x0600 )
#define JetOpenFileInstanceA JetOpenFileInstance
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOpenFileInstanceA(
    _In_ JET_INSTANCE       instance,
    _In_ JET_PCSTR          szFileName,
    _Out_ JET_HANDLE *      phfFile,
    _Out_ unsigned long *   pulFileSizeLow,
    _Out_ unsigned long *   pulFileSizeHigh );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOpenFileInstanceW(
    _In_ JET_INSTANCE       instance,
    _In_ JET_PCWSTR         szFileName,
    _Out_ JET_HANDLE *      phfFile,
    _Out_ unsigned long *   pulFileSizeLow,
    _Out_ unsigned long *   pulFileSizeHigh );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetOpenFileInstance JetOpenFileInstanceW
#else
#define JetOpenFileInstance JetOpenFileInstanceA
#endif
#endif

#endif

#if ( JET_VERSION < 0x0600 )
#define JetOpenFileSectionInstanceA JetOpenFileSectionInstance
#endif

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetOpenFileSectionInstanceA(
    _In_ JET_INSTANCE       instance,
    _In_ JET_PSTR           szFile,
    _Out_ JET_HANDLE *      phFile,
    _In_ long               iSection,
    _In_ long               cSections,
    _In_ unsigned __int64   ibRead,
    _Out_ unsigned long *   pulSectionSizeLow,
    _Out_ long *            plSectionSizeHigh );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetOpenFileSectionInstanceW(
    _In_ JET_INSTANCE       instance,
    _In_ JET_PWSTR          szFile,
    _Out_ JET_HANDLE *      phFile,
    _In_ long               iSection,
    _In_ long               cSections,
    _In_ unsigned __int64   ibRead,
    _Out_ unsigned long *   pulSectionSizeLow,
    _Out_ long *            plSectionSizeHigh );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetOpenFileSectionInstance JetOpenFileSectionInstanceW
#else
#define JetOpenFileSectionInstance JetOpenFileSectionInstanceA
#endif
#endif



#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetReadFile(
    _In_ JET_HANDLE                             hfFile,
    _Out_writes_bytes_to_( cb, *pcbActual ) void *  pv,
    _In_ unsigned long                          cb,
    _Out_opt_ unsigned long *                   pcbActual );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetReadFileInstance(
    _In_ JET_INSTANCE                           instance,
    _In_ JET_HANDLE                             hfFile,
    _Out_writes_bytes_to_( cb, *pcbActual ) void *  pv,
    _In_ unsigned long                          cb,
    _Out_opt_ unsigned long *                   pcbActual );

#endif
#pragma endregion

#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCloseFile(
    _In_ JET_HANDLE     hfFile );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCloseFileInstance(
    _In_ JET_INSTANCE   instance,
    _In_ JET_HANDLE     hfFile );

#endif
#pragma endregion

#endif

#if ( JET_VERSION < 0x0600 )
#define JetGetLogInfoA JetGetLogInfo
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetLogInfoA(
#if ( JET_VERSION < 0x0600 )
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) void *   pv,
#else
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PSTR szzLogs,
#endif
    _In_ unsigned long                                  cbMax,
    _Out_opt_ unsigned long *                           pcbActual );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetLogInfoW(
        _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PWSTR    szzLogs,
        _In_ unsigned long                                      cbMax,
        _Out_opt_ unsigned long *                               pcbActual );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetLogInfo JetGetLogInfoW
#else
#define JetGetLogInfo JetGetLogInfoA
#endif
#endif


#if ( JET_VERSION >= 0x0501 )
#if ( JET_VERSION < 0x0600 )
#define JetGetLogInfoInstanceA JetGetLogInfoInstance
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetLogInfoInstanceA(
    _In_ JET_INSTANCE                                   instance,
#if ( JET_VERSION < 0x0600 )
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) void *   pv,
#else
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PSTR szzLogs,
#endif
    _In_ unsigned long                                  cbMax,
    _Out_opt_ unsigned long *                           pcbActual );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetLogInfoInstanceW(
    _In_ JET_INSTANCE                                       instance,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PWSTR    wszzLogs,
    _In_ unsigned long                                      cbMax,
    _Out_opt_ unsigned long *                               pcbActual );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetLogInfoInstance JetGetLogInfoInstanceW
#else
#define JetGetLogInfoInstance JetGetLogInfoInstanceA
#endif
#endif


#define JET_BASE_NAME_LENGTH    3
typedef struct
{
    unsigned long   cbSize;
    unsigned long   ulGenLow;
    unsigned long   ulGenHigh;
    char            szBaseName[ JET_BASE_NAME_LENGTH + 1 ];
} JET_LOGINFO_A;

typedef struct
{
    unsigned long   cbSize;
    unsigned long   ulGenLow;
    unsigned long   ulGenHigh;
    WCHAR           szBaseName[ JET_BASE_NAME_LENGTH + 1 ];
} JET_LOGINFO_W;

#ifdef JET_UNICODE
#define JET_LOGINFO JET_LOGINFO_W
#else
#define JET_LOGINFO JET_LOGINFO_A
#endif

#if ( JET_VERSION < 0x0600 )
#define JetGetLogInfoInstance2A JetGetLogInfoInstance2
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetLogInfoInstance2A(
    _In_ JET_INSTANCE                                   instance,
#if ( JET_VERSION < 0x0600 )
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) void *   pv,
#else
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PSTR szzLogs,
#endif
    _In_ unsigned long                                  cbMax,
    _Out_opt_ unsigned long *                           pcbActual,
    _Inout_opt_ JET_LOGINFO_A *                         pLogInfo );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetLogInfoInstance2W(
    _In_ JET_INSTANCE                                       instance,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PWSTR    wszzLogs,
    _In_ unsigned long                                      cbMax,
    _Out_opt_ unsigned long *                               pcbActual,
    _Inout_opt_ JET_LOGINFO_W *                             pLogInfo );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetLogInfoInstance2 JetGetLogInfoInstance2W
#else
#define JetGetLogInfoInstance2 JetGetLogInfoInstance2A
#endif
#endif


#if ( JET_VERSION < 0x0600 )
#define JetGetTruncateLogInfoInstanceA JetGetTruncateLogInfoInstance
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetTruncateLogInfoInstanceA(
    _In_ JET_INSTANCE                                   instance,
#if ( JET_VERSION < 0x0600 )
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) void *   pv,
#else
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PSTR szzLogs,
#endif
    _In_ unsigned long                                  cbMax,
    _Out_opt_ unsigned long *                           pcbActual );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetTruncateLogInfoInstanceW(
    _In_ JET_INSTANCE                                       instance,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PWSTR    wszzLogs,
    _In_ unsigned long                                      cbMax,
    _Out_opt_ unsigned long *                               pcbActual );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetTruncateLogInfoInstance JetGetTruncateLogInfoInstanceW
#else
#define JetGetTruncateLogInfoInstance JetGetTruncateLogInfoInstanceA
#endif
#endif


#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetTruncateLog( void );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetTruncateLogInstance(
    _In_ JET_INSTANCE   instance );

#endif
#pragma endregion

#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetEndExternalBackup( void );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetEndExternalBackupInstance(
    _In_ JET_INSTANCE   instance );

JET_ERR JET_API
JetEndExternalBackupInstance2(
    _In_ JET_INSTANCE   instance,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#endif


#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#if ( JET_VERSION >= 0x0601 )
JET_ERR JET_API JetEndSurrogateBackup(
    _In_    JET_INSTANCE    instance,
    _In_        JET_GRBIT       grbit );

#endif

#endif
#pragma endregion


#if ( JET_VERSION < 0x0600 )
#define JetExternalRestoreA JetExternalRestore
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetExternalRestoreA(
    _In_ JET_PSTR                                   szCheckpointFilePath,
    _In_ JET_PSTR                                   szLogPath,
    _In_reads_opt_( crstfilemap ) JET_RSTMAP_A *    rgrstmap,
    _In_ long                                       crstfilemap,
    _In_ JET_PSTR                                   szBackupLogPath,
    _In_ long                                       genLow,
    _In_ long                                       genHigh,
    _In_ JET_PFNSTATUS                              pfn );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetExternalRestoreW(
    _In_ JET_PWSTR                                  szCheckpointFilePath,
    _In_ JET_PWSTR                                  szLogPath,
    _In_reads_opt_( crstfilemap ) JET_RSTMAP_W *    rgrstmap,
    _In_ long                                       crstfilemap,
    _In_ JET_PWSTR                                  szBackupLogPath,
    _In_ long                                       genLow,
    _In_ long                                       genHigh,
    _In_ JET_PFNSTATUS                              pfn );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetExternalRestore JetExternalRestoreW
#else
#define JetExternalRestore JetExternalRestoreA
#endif
#endif


#if JET_VERSION >= 0x0501
#if ( JET_VERSION < 0x0600 )
#define JetExternalRestore2A JetExternalRestore2
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetExternalRestore2A(
    _In_ JET_PSTR                                   szCheckpointFilePath,
    _In_ JET_PSTR                                   szLogPath,
    _In_reads_opt_( crstfilemap ) JET_RSTMAP_A *    rgrstmap,
    _In_ long                                       crstfilemap,
    _In_ JET_PSTR                                   szBackupLogPath,
    _Inout_ JET_LOGINFO_A *                         pLogInfo,
    _In_opt_ JET_PSTR                               szTargetInstanceName,
    _In_opt_ JET_PSTR                               szTargetInstanceLogPath,
    _In_opt_ JET_PSTR                               szTargetInstanceCheckpointPath,
    _In_ JET_PFNSTATUS                              pfn );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetExternalRestore2W(
    _In_ JET_PWSTR                                  szCheckpointFilePath,
    _In_ JET_PWSTR                                  szLogPath,
    _In_reads_opt_( crstfilemap ) JET_RSTMAP_W *    rgrstmap,
    _In_ long                                       crstfilemap,
    _In_ JET_PWSTR                                  szBackupLogPath,
    _Inout_ JET_LOGINFO_W *                         pLogInfo,
    _In_opt_ JET_PWSTR                              szTargetInstanceName,
    _In_opt_ JET_PWSTR                              szTargetInstanceLogPath,
    _In_opt_ JET_PWSTR                              szTargetInstanceCheckpointPath,
    _In_ JET_PFNSTATUS                              pfn );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetExternalRestore2 JetExternalRestore2W
#else
#define JetExternalRestore2 JetExternalRestore2A
#endif
#endif

#if ( JET_VERSION < 0x0600 )
#define JetSnapshotStartA JetSnapshotStart
#endif

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetSnapshotStartA(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PSTR       szDatabases,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetSnapshotStartW(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PWSTR      szDatabases,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetSnapshotStart JetSnapshotStartW
#else
#define JetSnapshotStart JetSnapshotStartA
#endif
#endif

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetSnapshotStop(
    _In_ JET_INSTANCE   instance,
    _In_ JET_GRBIT      grbit);

#endif
#pragma endregion


#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetRegisterCallback(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_CBTYP      cbtyp,
    _In_ JET_CALLBACK   pCallback,
    _In_opt_ void *     pvContext,
    _In_ JET_HANDLE *   phCallbackId );

JET_ERR JET_API
JetUnregisterCallback(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_CBTYP      cbtyp,
    _In_ JET_HANDLE     hCallbackId );

#endif
#pragma endregion

typedef struct _JET_INSTANCE_INFO_A
{
    JET_INSTANCE        hInstanceId;
    char *              szInstanceName;

    JET_API_PTR         cDatabases;
    char **             szDatabaseFileName;
    char **             szDatabaseDisplayName;
    char **             szDatabaseSLVFileName_Obsolete;
} JET_INSTANCE_INFO_A;

typedef struct _JET_INSTANCE_INFO_W
{
    JET_INSTANCE        hInstanceId;
    WCHAR *             szInstanceName;

    JET_API_PTR         cDatabases;
    WCHAR **            szDatabaseFileName;
    WCHAR **            szDatabaseDisplayName;
    WCHAR **            szDatabaseSLVFileName_Obsolete;
} JET_INSTANCE_INFO_W;

#ifdef JET_UNICODE
#define JET_INSTANCE_INFO JET_INSTANCE_INFO_W
#else
#define JET_INSTANCE_INFO JET_INSTANCE_INFO_A
#endif

#if ( JET_VERSION < 0x0600 )
#define JetGetInstanceInfoA JetGetInstanceInfo
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetInstanceInfoA(
    _Out_ unsigned long *                                           pcInstanceInfo,
    _Outptr_result_buffer_( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetInstanceInfoW(
    _Out_ unsigned long *                                           pcInstanceInfo,
    _Outptr_result_buffer_( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetGetInstanceInfo JetGetInstanceInfoW
#else
#define JetGetInstanceInfo JetGetInstanceInfoA
#endif
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetFreeBuffer(
    _Pre_notnull_ char *    pbBuf );

JET_ERR JET_API
JetSetLS(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_LS         ls,
    _In_ JET_GRBIT      grbit );

JET_ERR JET_API
JetGetLS(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _Out_ JET_LS *      pls,
    _In_ JET_GRBIT      grbit );

#endif
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetTracing(
    _In_ const JET_TRACEOP  traceop,
    _In_ const JET_TRACETAG tracetag,
    _In_ const JET_API_PTR  ul );

#endif
#pragma endregion

#endif


#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

typedef JET_API_PTR JET_OSSNAPID;

JET_ERR JET_API
JetOSSnapshotPrepare(
    _Out_ JET_OSSNAPID *    psnapId,
    _In_ const JET_GRBIT    grbit );
#if ( JET_VERSION >= 0x0600 )
JET_ERR JET_API
JetOSSnapshotPrepareInstance(
    _In_ JET_OSSNAPID       snapId,
    _In_ JET_INSTANCE       instance,
    _In_ const JET_GRBIT    grbit );

#endif

#if ( JET_VERSION < 0x0600 )
#define JetOSSnapshotFreezeA JetOSSnapshotFreeze
#endif

JET_ERR JET_API
JetOSSnapshotFreezeA(
    _In_ const JET_OSSNAPID                                         snapId,
    _Out_ unsigned long *                                           pcInstanceInfo,
    _Outptr_result_buffer_( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo,
    _In_ const JET_GRBIT                                            grbit );

#if ( JET_VERSION >= 0x0600 )

JET_ERR JET_API
JetOSSnapshotFreezeW(
    _In_ const JET_OSSNAPID                                         snapId,
    _Out_ unsigned long *                                           pcInstanceInfo,
    _Outptr_result_buffer_( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo,
    _In_ const JET_GRBIT                                            grbit );

#ifdef JET_UNICODE
#define JetOSSnapshotFreeze JetOSSnapshotFreezeW
#else
#define JetOSSnapshotFreeze JetOSSnapshotFreezeA
#endif
#endif

JET_ERR JET_API
JetOSSnapshotThaw(
    _In_ const JET_OSSNAPID snapId,
    _In_ const JET_GRBIT    grbit );

#endif
#pragma endregion

#endif

#if ( JET_VERSION >= 0x0502 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOSSnapshotAbort(
    _In_ const JET_OSSNAPID snapId,
    _In_ const JET_GRBIT    grbit );

#endif
#pragma endregion

#endif

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOSSnapshotTruncateLog(
    _In_ const JET_OSSNAPID snapId,
    _In_ const JET_GRBIT    grbit );

JET_ERR JET_API
JetOSSnapshotTruncateLogInstance(
    _In_ const JET_OSSNAPID snapId,
    _In_ JET_INSTANCE       instance,
    _In_ const JET_GRBIT    grbit );

#if ( JET_VERSION < 0x0600 )
#define JetOSSnapshotGetFreezeInfoA JetOSSnapshotGetFreezeInfo
#endif

JET_ERR JET_API
JetOSSnapshotGetFreezeInfoA(
    _In_ const JET_OSSNAPID                                         snapId,
    _Out_ unsigned long *                                           pcInstanceInfo,
    _Outptr_result_buffer_( *pcInstanceInfo ) JET_INSTANCE_INFO_A **    paInstanceInfo,
    _In_ const JET_GRBIT                                            grbit );

#endif
#pragma endregion

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOSSnapshotGetFreezeInfoW(
    _In_ const JET_OSSNAPID                                         snapId,
    _Out_ unsigned long *                                           pcInstanceInfo,
    _Outptr_result_buffer_( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo,
    _In_ const JET_GRBIT                                            grbit );

#ifdef JET_UNICODE
#define JetOSSnapshotGetFreezeInfo JetOSSnapshotGetFreezeInfoW
#else
#define JetOSSnapshotGetFreezeInfo JetOSSnapshotGetFreezeInfoA
#endif

JET_ERR JET_API
JetOSSnapshotEnd(
    _In_ const JET_OSSNAPID snapId,
    _In_ const JET_GRBIT    grbit );

#endif
#pragma endregion

#endif

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetGetPageInfo(
    _In_reads_bytes_( cbData ) void * const         pvPages,
    _In_ unsigned long                          cbData,
    _Inout_updates_bytes_( cbPageInfo ) JET_PAGEINFO *  rgPageInfo,
    _In_ unsigned long                          cbPageInfo,
    _In_ JET_GRBIT                              grbit,
    _In_ unsigned long                          ulInfoLevel );

#endif
#pragma endregion

#endif

#if ( JET_VERSION >= 0x0601 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetGetPageInfo2(
    _In_reads_bytes_( cbData ) void * const         pvPages,
    _In_ unsigned long                          cbData,
    _Inout_updates_bytes_( cbPageInfo ) void * const    rgPageInfo,
    _In_ unsigned long                          cbPageInfo,
    _In_ JET_GRBIT                              grbit,
    _In_ unsigned long                          ulInfoLevel );

JET_ERR JET_API
JetGetDatabasePages(
    _In_ JET_SESID                              sesid,
    _In_ JET_DBID                               dbid,
    _In_ unsigned long                          pgnoStart,
    _In_ unsigned long                          cpg,
    _Out_writes_bytes_to_( cb, *pcbActual ) void *  pv,
    _In_ unsigned long                          cb,
    _Out_ unsigned long *                       pcbActual,
    _In_ JET_GRBIT                              grbit );

#endif
#pragma endregion


#if ( JET_VERSION >= 0x602 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetOnlinePatchDatabasePage(
    _In_ JET_SESID                              sesid,
    _In_ JET_DBID                               dbid,
    _In_ unsigned long                          pgno,
    _In_reads_bytes_(cbToken) const void *          pvToken,
    _In_ unsigned long                          cbToken,
    _In_reads_bytes_(cbData)    const void *            pvData,
    _In_ unsigned long                          cbData,
    _In_ JET_GRBIT                              grbit );

#endif
#pragma endregion

#endif

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetRemoveLogfileA(
    _In_z_ JET_PCSTR szDatabase,
    _In_z_ JET_PCSTR szLogfile,
    _In_ JET_GRBIT grbit );

JET_ERR JET_API
JetRemoveLogfileW(
    _In_z_ JET_PCWSTR wszDatabase,
    _In_z_ JET_PCWSTR wszLogfile,
    _In_ JET_GRBIT grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetRemoveLogfile JetRemoveLogfileW
#else
#define JetRemoveLogfile JetRemoveLogfileA
#endif

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetBeginDatabaseIncrementalReseedA(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCSTR      szDatabase,
    _In_ JET_GRBIT      grbit );

JET_ERR JET_API
JetBeginDatabaseIncrementalReseedW(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCWSTR     szDatabase,
    _In_ JET_GRBIT      grbit );

#ifdef JET_UNICODE
#define JetBeginDatabaseIncrementalReseed JetBeginDatabaseIncrementalReseedW
#else
#define JetBeginDatabaseIncrementalReseed JetBeginDatabaseIncrementalReseedA
#endif

JET_ERR JET_API
JetEndDatabaseIncrementalReseedA(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCSTR      szDatabase,
    _In_ unsigned long  genMinRequired,
    _In_ unsigned long  genFirstDivergedLog,
    _In_ unsigned long  genMaxRequired,
    _In_ JET_GRBIT      grbit );

JET_ERR JET_API
JetEndDatabaseIncrementalReseedW(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCWSTR     szDatabase,
    _In_ unsigned long  genMinRequired,
    _In_ unsigned long  genFirstDivergedLog,
    _In_ unsigned long  genMaxRequired,
    _In_ JET_GRBIT      grbit );

#ifdef JET_UNICODE
#define JetEndDatabaseIncrementalReseed JetEndDatabaseIncrementalReseedW
#else
#define JetEndDatabaseIncrementalReseed JetEndDatabaseIncrementalReseedA
#endif

JET_ERR JET_API
JetPatchDatabasePagesA(
    _In_ JET_INSTANCE               instance,
    _In_ JET_PCSTR                  szDatabase,
    _In_ unsigned long              pgnoStart,
    _In_ unsigned long              cpg,
    _In_reads_bytes_( cb ) const void * pv,
    _In_ unsigned long              cb,
    _In_ JET_GRBIT                  grbit );

JET_ERR JET_API
JetPatchDatabasePagesW(
    _In_ JET_INSTANCE               instance,
    _In_ JET_PCWSTR                 szDatabase,
    _In_ unsigned long              pgnoStart,
    _In_ unsigned long              cpg,
    _In_reads_bytes_( cb ) const void * pv,
    _In_ unsigned long              cb,
    _In_ JET_GRBIT                  grbit );

#endif
#pragma endregion

#ifdef JET_UNICODE
#define JetPatchDatabasePages JetPatchDatabasePagesW
#else
#define JetPatchDatabasePages JetPatchDatabasePagesA
#endif


#endif

#if ( JET_VERSION >= 0x0A01 )

JET_ERR JET_API
JetGetRBSFileInfoA(
    _In_ JET_PCSTR                  szRBSFileName,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

JET_ERR JET_API
JetGetRBSFileInfoW(
    _In_ JET_PCWSTR                 szRBSFileName,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#ifdef JET_UNICODE
#define JetGetRBSFileInfo JetGetRBSFileInfoW
#else
#define JetGetRBSFileInfo JetGetRBSFileInfoA
#endif

JET_ERR JET_API
JetRBSPrepareRevert(
    __in    JET_INSTANCE    instance,
    __in    JET_LOGTIME     jltRevertExpected,
    __in    long            cpgCache,
    __in    JET_GRBIT       grbit,
    _Out_   JET_LOGTIME*    pjltRevertActual );

JET_ERR JET_API
JetRBSExecuteRevert(
    __in    JET_INSTANCE    instance,
    __in    JET_GRBIT       grbit,
    _Out_   JET_RBSREVERTINFOMISC*  prbsrevertinfomisc );

JET_ERR JET_API
JetRBSCancelRevert(
    __in    JET_INSTANCE    instance );

#endif



#if ( JET_VERSION >= 0x0601 )


#define JET_bitDumpMinimum                      0x00000001
#define JET_bitDumpMaximum                      0x00000002
#define JET_bitDumpCacheMinimum                 0x00000004
#define JET_bitDumpCacheMaximum                 0x00000008
#define JET_bitDumpCacheIncludeDirtyPages       0x00000010
#define JET_bitDumpCacheIncludeCachedPages      0x00000020
#define JET_bitDumpCacheIncludeCorruptedPages   0x00000040
#define JET_bitDumpCacheNoDecommit              0x00000080


#define JET_bitDumpUnitTest                     0x80000000




#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetConfigureProcessForCrashDump(
    _In_ const JET_GRBIT grbit );

#endif
#pragma endregion

#endif

#if ( JET_VERSION >= 0x0601 )

typedef enum
{
    opTestHookUnitTests,
    opTestHookTestInjection,
    opTestHookHookNtQueryInformationProcess,
    opTestHookHookNtQuerySystemInformation,
    opTestHookHookGlobalMemoryStatus,
    opTestHookSetNegativeTesting,
    opTestHookResetNegativeTesting,
    opTestHookThrowError,
    opTestHookThrowAssert,
    opTestHookUnitTests2,
    opTestHookEnforceContextFail,
    opTestHookGetBFLowMemoryCallback,
    opTestHookTraceTestMarker,
    opTestHookGetCheckpointDepth,
    opTestHookGetOLD2Status,
    opTestHookGetEngineTickNow,
    opTestHookSetEngineTickTime,
    opTestHookCacheQuery,
    opTestHookEvictCache,
    opTestHookCorrupt,
    opTestHookEnableAutoIncDeprecated,
    opTestHookSetErrorTrap,
    opTestHookGetTablePgnoFDP,
    opTestHookAlterDatabaseFileHeader,
    opTestHookGetLogTip,
} TESTHOOK_OP;



enum
{
    fDeletingLogFiles                       = 0x00000001,
    fCorruptingLogFiles                     = 0x00000002,
    fLockingCheckpointFile                  = 0x00000004,
    fCorruptingDbHeaders                    = 0x00000008,
    fCorruptingPagePgnos                    = 0x00000010,
    fLeakStuff                              = 0x00000020,
    fCorruptingWithLostFlush                = 0x00000040,
    fDisableTimeoutDeadlockDetection        = 0x00000080,
    fCorruptingPages                        = 0x00000100,
    fDiskIOError                            = 0x00000200,
    fInvalidAPIUsage                        = 0x00000400,
    fInvalidUsage                           = 0x00000800,
    fCorruptingPageLogically                = 0x00001000,
    fOutOfMemory                            = 0x00002000,
    fLeakingUnflushedIos                    = 0x00004000,
    fHangingIOs                             = 0x00008000,
    fCorruptingWithLostFlushWithinReqRange  = 0x00010000,
    fStrictIoPerfTesting                    = 0x00020000,
    fDisableAssertReqRangeConsistentLgpos   = 0x00040000,
};

typedef struct tagJET_TESTHOOKUNITTEST2
{
    unsigned long       cbStruct;
    char *              szTestName;
    JET_DBID            dbidTestOn;
} JET_TESTHOOKUNITTEST2;

#define bitHangInjectSleep          0x40000000
#define mskHangInjectOptions        ( bitHangInjectSleep | 0xFFFF0000 )

typedef enum
{
    JET_TestInjectInvalid = 0,
    JET_TestInjectMin,
    JET_TestInjectFault = JET_TestInjectMin,
    JET_TestInjectConfigOverride,
    JET_TestInjectHang,
    JET_TestInjectMax
} JET_TESTINJECTIONTYPE;


#define JET_bitInjectionProbabilityPct          0x00000001
#define JET_bitInjectionProbabilityCount        0x00000002
#define JET_bitInjectionProbabilityPermanent    0x00000004
#define JET_bitInjectionProbabilityFailUntil    0x00000008
#define JET_bitInjectionProbabilitySuppress     0x40000000
#define JET_bitInjectionProbabilityCleanup      0x80000000

typedef struct tagJET_TESTHOOKTESTINJECTION
{
    unsigned long           cbStruct;
    unsigned long           ulID;
    JET_API_PTR             pv;
    JET_TESTINJECTIONTYPE   type;
    unsigned long           ulProbability;
    JET_GRBIT               grbit;
} JET_TESTHOOKTESTINJECTION;

typedef struct tagJET_TESTHOOKAPIHOOKING
{
    unsigned long   cbStruct;
    const void *    pfnOld;
    const void *    pfnNew;
} JET_TESTHOOKAPIHOOKING;

typedef struct tagJET_TESTHOOKTRACETESTMARKER
{
    unsigned long       cbStruct;
    const char *        szAnnotation;
    unsigned __int64    qwMarkerID;
} JET_TESTHOOKTRACETESTMARKER;

typedef struct tagJET_TESTHOOKTIMEINJECTION
{
    unsigned long       cbStruct;
    unsigned long       tickNow;
    unsigned long       eTimeInjWrapMode;
    unsigned long       dtickTimeInjWrapOffset;
    unsigned long       dtickTimeInjAccelerant;
} JET_TESTHOOKTIMEINJECTION;

typedef struct tagJET_TESTHOOKCACHEQUERY
{
    unsigned long       cbStruct;

    long                cCacheQuery;
    char **             rgszCacheQuery;

    void *              pvOut;
} JET_TESTHOOKCACHEQUERY;

#define JET_bitTestHookEvictDataByPgno  0x00000001

typedef struct tagJET_TESTHOOKEVICTCACHE
{
    unsigned long           cbStruct;
    JET_API_PTR         ulTargetContext;
    JET_API_PTR         ulTargetData;
    JET_GRBIT           grbit;
} JET_TESTHOOKEVICTCACHE;



#define JET_bitTestHookCorruptDatabaseFile  0x80000000
#define JET_bitTestHookCorruptDatabasePageImage 0x40000000
#define JET_mskTestHookCorruptFileType      ( JET_bitTestHookCorruptDatabaseFile | JET_bitTestHookCorruptDatabasePageImage )

#define JET_bitTestHookCorruptPageChksumRand    0x00000001
#define JET_bitTestHookCorruptPageChksumSafe    0x00000002
#define JET_bitTestHookCorruptPageSingleFld 0x00000004
#define JET_bitTestHookCorruptPageRemoveNode    0x00000008
#define JET_bitTestHookCorruptPageDbtimeDelta   0x00000010
#define JET_bitTestHookCorruptNodePrefix    0x00000020
#define JET_bitTestHookCorruptNodeSuffix    0x00000040

#define JET_mskTestHookCorruptDataType      ( JET_bitTestHookCorruptPageChksumRand | JET_bitTestHookCorruptPageChksumSafe | JET_bitTestHookCorruptPageSingleFld | JET_bitTestHookCorruptPageRemoveNode | JET_bitTestHookCorruptPageDbtimeDelta )

#define JET_bitTestHookCorruptSizeLargerThanNode    0x00010000
#define JET_bitTestHookCorruptSizeShortWrapSmall    0x00020000
#define JET_bitTestHookCorruptSizeShortWrapLarge    0x00040000

#define JET_mskTestHookCorruptSpecific          0x00FF0000


#define JET_bitTestHookCorruptLeaveChecksum 0x01000000

#define JET_pgnoTestHookCorruptRandom       0xFFFFFFFFFFFFFFFFLL

typedef struct tagJET_TESTHOOKCORRUPT
{
    unsigned long           cbStruct;
    JET_GRBIT           grbit;

    union
    {
#include <pshpack8.h>
        struct
        {
            JET_PWSTR   wszDatabaseFilePath;
            __int64     pgnoTarget;
            __int64     iSubTarget;
        } CorruptDatabaseFile;
#include <poppack.h>

        struct
        {
            JET_API_PTR pbPageImageTarget;
            unsigned long   cbPageImage;
            __int64     pgnoTarget;
            __int64     iSubTarget;
        } CorruptDatabasePageImage;
    };

} JET_TESTHOOKCORRUPT;



#define JET_bitAlterDbFileHdrAddField                       0x1

#define JET_ibfieldDbFileHdrMajorVersion                    0x008
#define JET_ibfieldDbFileHdrUpdateMajor                     0x0e8
#define JET_ibfieldDbFileHdrUpdateMinor                     0x284


typedef struct tagJET_TESTHOOKALTERDBFILEHDR
{
    JET_PWSTR           szDatabase;
    unsigned long       ibField;
    unsigned long       cbField;
    char *              pbField;
    JET_GRBIT           grbit;
} JET_TESTHOOKALTERDBFILEHDR;


#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API JetTestHook(
    _In_        const TESTHOOK_OP   opcode,
    _Inout_opt_ void * const        pv );


JET_ERR JET_API JetConsumeLogData(
    _In_    JET_INSTANCE        instance,
    _In_    JET_EMITDATACTX *   pEmitLogDataCtx,
    _In_    void *              pvLogData,
    _In_    unsigned long       cbLogData,
    _In_    JET_GRBIT           grbits );

#endif
#pragma endregion


#endif

#if ( JET_VERSION >= 0x0602 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetGetErrorInfoW(
    _In_opt_ void *                 pvContext,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel,
    _In_ JET_GRBIT                  grbit );

#ifdef JET_UNICODE
#define JetGetErrorInfo JetGetErrorInfoW
#else
#define JetGetErrorInfo JetGetErrorInfoA_DoesNotExist_OnlyUnicodeVersionOfThisAPI_UseExcplicit_JetGetErrorInfoW_Instead
#endif

JET_ERR JET_API
JetSetSessionParameter(
    _In_opt_ JET_SESID                                          sesid,
    _In_ unsigned long                                          sesparamid,
    _In_reads_bytes_opt_( cbParam ) void *                      pvParam,
    _In_ unsigned long                                          cbParam );

JET_ERR JET_API
JetGetSessionParameter(
    _In_opt_ JET_SESID                                          sesid,
    _In_ unsigned long                                          sesparamid,
    _Out_cap_post_count_(cbParamMax, *pcbParamActual) void *    pvParam,
    _In_ unsigned long                                          cbParamMax,
    _Out_opt_ unsigned long *                                   pcbParamActual );

#endif
#pragma endregion


#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API JetPrereadTablesW(
    _In_ JET_SESID                          sesid,
    _In_ JET_DBID                           dbid,
    _In_reads_( cwszTables ) JET_PCWSTR *   rgwszTables,
    _In_ long                               cwszTables,
    _In_ JET_GRBIT                          grbit );

#ifdef JET_UNICODE
#define JetPrereadTables JetPrereadTablesW
#else
#define JetPrereadTables JetPrereadTablesA_DoesNotExist_OnlyUnicodeVersionOfThisAPI_UseExcplicit_JetPrereadTablesW_Instead
#endif

#endif
#pragma endregion


#endif


#if ( JET_VERSION >= 0x0A00 )

JET_ERR JET_API
JetPrereadIndexRange(
    _In_ JET_SESID                      sesid,
    _In_ JET_TABLEID                    tableid,
    _In_ const JET_INDEX_RANGE * const  pIndexRange,
    _In_ const unsigned long            cPageCacheMin,
    _In_ const unsigned long            cPageCacheMax,
    _In_ JET_GRBIT                      grbit,
    _Out_opt_ unsigned long * const     pcPageCacheActual );

#endif

#if ( JET_VERSION >= 0x0A01 )

JET_ERR JET_API JetRetrieveColumnByReference(
    _In_ const JET_SESID                                            sesid,
    _In_ const JET_TABLEID                                          tableid,
    _In_reads_bytes_( cbReference ) const void * const              pvReference,
    _In_ const unsigned long                                        cbReference,
    _In_ const unsigned long                                        ibData,
    _Out_writes_bytes_to_opt_( cbData, min( cbData, *pcbActual ) ) void * const pvData,
    _In_ const unsigned long                                        cbData,
    _Out_opt_ unsigned long * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit );

JET_ERR JET_API JetPrereadColumnsByReference(
    _In_ const JET_SESID                                    sesid,
    _In_ const JET_TABLEID                                  tableid,
    _In_reads_( cReferences ) const void * const * const    rgpvReferences,
    _In_reads_( cReferences ) const unsigned long * const   rgcbReferences,
    _In_ const unsigned long                                cReferences,
    _In_ const unsigned long                                cPageCacheMin,
    _In_ const unsigned long                                cPageCacheMax,
    _Out_opt_ unsigned long * const                         pcReferencesPreread,
    _In_ const JET_GRBIT                                    grbit );

#endif

#if ( JET_VERSION >= 0x0A01 )

JET_ERR JET_API JetStreamRecords(
    _In_ JET_SESID                                                  sesid,
    _In_ JET_TABLEID                                                tableid,
    _In_ const unsigned long                                        ccolumnid,
    _In_reads_opt_( ccolumnid ) const JET_COLUMNID * const          rgcolumnid,
    _Out_writes_bytes_to_opt_( cbData, *pcbActual ) void * const    pvData,
    _In_ const unsigned long                                        cbData,
    _Out_opt_ unsigned long * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit );

JET_ERR JET_API JetRetrieveColumnFromRecordStream(
    _Inout_updates_bytes_( cbData ) void * const    pvData,
    _In_ const unsigned long                        cbData,
    _Out_ unsigned long * const                     piRecord,
    _Out_ JET_COLUMNID * const                      pcolumnid,
    _Out_ unsigned long * const                     pitagSequence,
    _Out_ unsigned long * const                     pibValue,
    _Out_ unsigned long * const                     pcbValue );

#endif


#ifdef  __cplusplus
}
#endif

#endif

#include <poppack.h>

#ifdef  __cplusplus
}
#endif

#endif


