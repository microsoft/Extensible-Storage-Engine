// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// begin_PubEsent
#pragma once

#if !defined(_JET_INCLUDED)
#define _JET_INCLUDED

#include <winapifamily.h>

#ifdef  __cplusplus
extern "C" {
#endif

#include <specstrings.h>

// end_PubEsent
#define eseVersion 0x5600
//  HISTORY:
//      0x5500  - ESE97 (Exchange 5.5)
//      0x5580  - ESENT97 (W2k)
//      0x5600  - ESENT98 (Whistler)
//      0x6000  - ESE98 (Exchange 2000)
//      0x6200  - ESE98 (Windows Server 2003)
// begin_PubEsent

// JET_VERSION is similar to WINVER. It allows the most recent header to be used
// against older targets. Supported versions are:
// 0x0500  - Windows 2000
// 0x0501  - Windows XP
// 0x0502  - Windows Server 2003
// 0x0600  - Windows Vista / Windows Server 2008
// 0x0601  - Windows 7 / Windows Server 2008 R2
// 0x0602  - Windows 8 / Windows Server 2012
// 0x0603  - Windows 8.1 / Windows Server 2012 R2
// 0x0A00  - Windows 10
// end_PubEsent
// 0x0A01  - Beyond Windows 10 (private-only Threshold 2 APIs, there were no public Threshold 2 APIs) and/or Redstone
// begin_PubEsent


#ifndef JET_VERSION
#  ifdef WINVER
#    define JET_VERSION WINVER
#  else
        // JET_VERSION has not been specified. Assume all functions are available.
#    define JET_VERSION 0x0A00
#  endif
#endif


#define JET_cbPage  4096            //  UNDONE: Remove when no more components reference this. // not_PubEsent

#if defined(_WIN64)
#include <pshpack8.h>
#else
#include <pshpack4.h>
#endif


#pragma warning(push)
#pragma warning(disable: 4201)      //  nonstandard extension used : nameless struct/union


#define JET_API     __stdcall
#define JET_NODSAPI __stdcall

// end_PubEsent
//
//  UNDONE: should we just remove this redefinition and
//  include basetsd.h (then typedef JET_API_PTR to ULONG_PTR)??
//
// begin_PubEsent
#if defined(_WIN64)
    typedef unsigned __int64 JET_API_PTR;
#elif !defined(__midl) && (defined(_X86_) || defined(_M_IX86))
    typedef __w64 unsigned long JET_API_PTR;
#else
    typedef unsigned long JET_API_PTR;
#endif

typedef _Return_type_success_( return >= 0 ) long JET_ERR;

// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )
typedef unsigned long JET_ENGINEFORMATVERSION;  /* efv - engine format version specification */
#endif // JET_VERSION >= 0x0A01
// begin_PubEsent

typedef JET_API_PTR JET_HANDLE; /* backup file handle */
typedef JET_API_PTR JET_INSTANCE;   /* Instance Identifier */
typedef JET_API_PTR JET_SESID;      /* Session Identifier */
typedef JET_API_PTR JET_TABLEID;    /* Table Identifier */
#if ( JET_VERSION >= 0x0501 )
typedef JET_API_PTR JET_LS;     /* Local Storage */
#endif // JET_VERSION >= 0x0501
// end_PubEsent
#if ( JET_VERSION >= 0x0601 )
typedef JET_API_PTR JET_HISTO;
#endif // JET_VERSION >= 0x0601
// begin_PubEsent

typedef unsigned long JET_COLUMNID; /* Column Identifier */

typedef struct tagJET_INDEXID
{
    unsigned long   cbStruct;
    unsigned char   rgbIndexId[sizeof(JET_API_PTR)+sizeof(unsigned long)+sizeof(unsigned long)];
} JET_INDEXID;

typedef unsigned long JET_DBID;     /* Database Identifier */
typedef unsigned long JET_OBJTYP;   /* Object Type */
typedef unsigned long JET_COLTYP;   /* Column Type */
typedef unsigned long JET_GRBIT;    /* Group of Bits */

typedef unsigned long JET_SNP;      /* Status Notification Process */
typedef unsigned long JET_SNT;      /* Status Notification Type */
// end_PubEsent
typedef unsigned long JET_SNC;      /* Status Notification Code */
// begin_PubEsent
typedef double JET_DATESERIAL;      /* JET_coltypDateTime format */
// end_PubEsent
typedef unsigned long JET_DLLID;      /* ID of DLL for hook functions */
// begin_PubEsent
#if ( JET_VERSION >= 0x0501 )
typedef unsigned long JET_CBTYP;    /* Callback Types */
#endif // JET_VERSION >= 0x0501

typedef JET_ERR (JET_API *JET_PFNSTATUS)(
    _In_ JET_SESID  sesid,
    _In_ JET_SNP    snp,
    _In_ JET_SNT    snt,
    _In_opt_ void * pv );

// end_PubEsent

#if ( JET_VERSION >= 0x0A01 )

// This callback is used by JetInit4. Compared to JET_PFNSTATUS
// it has a user-provided context and eliminates the unused sesid
// parameter.
typedef JET_ERR (JET_API * JET_PFNINITCALLBACK)(
    _In_ JET_SNP    snp,
    _In_ JET_SNT    snt,
    _In_opt_ void * pv,             // depends on the snp, snt
    _In_opt_ void * pvContext );    // provided in JetInit4

#endif // JET_VERSION >= 0x0A01

// begin_PubEsent

#if !defined(_NATIVE_WCHAR_T_DEFINED)
typedef unsigned short WCHAR;
#else
typedef wchar_t WCHAR;
#endif

typedef _Null_terminated_ char *  JET_PSTR;    /* ASCII string (char *) null terminated */
typedef _Null_terminated_ const char *  JET_PCSTR;   /* const ASCII string (char *) null terminated */
typedef _Null_terminated_ WCHAR * JET_PWSTR;   /* Unicode string (char *) null terminated */
typedef _Null_terminated_ const WCHAR * JET_PCWSTR;  /* const Unicode string (char *) null terminated */

typedef struct
{
    char            *szDatabaseName;
    char            *szNewDatabaseName;
} JET_RSTMAP_A;         /* restore map */

typedef struct
{
    WCHAR           *szDatabaseName;
    WCHAR           *szNewDatabaseName;
} JET_RSTMAP_W;         /* restore map */

#ifdef JET_UNICODE
#define JET_RSTMAP JET_RSTMAP_W
#else
#define JET_RSTMAP JET_RSTMAP_A
#endif

// end_PubEsent

#if ( JET_VERSION >= 0x0A01 )

typedef struct tagJET_SETDBPARAM
{
    unsigned long                           dbparamid;  //  One of the JET_dbparams.

    _Field_size_bytes_( cbParam ) void *    pvParam;    //  Address of the value of the parameter. Note that even for integral types, a valid
                                                        //  memory location must be passed, as opposed to the numerical value cast to a void*.

    unsigned long                           cbParam;    //  The size of the data, in bytes, pointed to by pvParam.
} JET_SETDBPARAM;

typedef struct
{
    unsigned long                                   cbStruct;           //  size of this structure (for future expansion)
    char                                            *szDatabaseName;    //  (optional) original database path
    char                                            *szNewDatabaseName; //  new database path
    _Field_size_opt_( csetdbparam ) JET_SETDBPARAM  *rgsetdbparam;      //  (optional) array of database parameters
    unsigned long                                   csetdbparam;        //  number of elements in rgsetdbparam
    JET_GRBIT                                       grbit;              //  recovery options
} JET_RSTMAP2_A;

typedef struct
{
    unsigned long                                   cbStruct;           //  size of this structure (for future expansion)
    WCHAR                                           *szDatabaseName;    //  (optional) original database path
    WCHAR                                           *szNewDatabaseName; //  new database path
    _Field_size_opt_( csetdbparam ) JET_SETDBPARAM  *rgsetdbparam;      //  (optional) array of database parameters
    unsigned long                                   csetdbparam;        //  number of elements in rgsetdbparam
    JET_GRBIT                                       grbit;              //  recovery options
} JET_RSTMAP2_W;

#ifdef JET_UNICODE
#define JET_RSTMAP2 JET_RSTMAP2_W
#else
#define JET_RSTMAP2 JET_RSTMAP2_A
#endif

#endif // JET_VERSION >= 0x0A01

// begin_PubEsent

//  For edbutil convert and JetConvert() only.

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

// end_PubEsent

typedef enum
{

    //  Database operations

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
    opDBUTILSLVMove_ObsoleteAndUnused, // No longer used. Left in to preserve the subsequent enum values.
    opDBUTILDBConvertRecords_ObsoleteAndUnused, // No longer used. Left in to preserve the subsequent enum values.
    opDBUTILDBDefragment,
    opDBUTILDumpExchangeSLVInfo_ObsoleteAndUnused, // No longer used. Left in to preserve the subsequent enum values.
    opDBUTILDumpUnicodeFixupTable_ObsoleteAndUnused, // No longer used. Left in to preserve the subsequent enum values.
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

    // When adding to this union; you must use
    // fewer bytes than legacy to maintain forward/backward 
    // compatibility for all clients that use it.

    union
    {
        // legacy elements
        struct
        {
            char           *szDatabase;
            char           *szSLV_ObsoleteAndUnused; // No longer used. Left in to preserve the subsequent values;
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
            
        // ChecksumLogFromMemory
        struct
        {
            char            *szLog;     // Name of the Log file
            char            *szBase;    // Base name used e.g. "edb" or "E01"
            void            *pvBuffer;  // Pointer to buffer containing the log 
            long             cbBuffer;  // Length of buffer
        } checksumlogfrommemory;

        // opDBUTILDumpSpaceCategory
        struct
        {
            char               *szDatabase;            // Database from which to dump the space category of pages.
            unsigned long       pgnoFirst;             // First page to dump the category for. The first page in the database is 1.
            unsigned long       pgnoLast;              // Last page to dump the category for. The last page in the database can be passed in as (unsigned long)-1.
            void               *pfnSpaceCatCallback;   // Callback to receive each page's category (JET_SPCATCALLBACK).
            void               *pvContext;             // General purpose context which is passed back to the client callback (pfnSpaceCatCallback).
        } spcatOptions;

        // opDBUTILDumpRBS
        struct
        {
            char               *szDatabase;            // Database from which to dump the space category of pages.
            unsigned long       pgnoFirst;             // First page to dump the category for. The first page in the database is 1.
            unsigned long       pgnoLast;              // Last page to dump the category for. The last page in the database can be passed in as (unsigned long)-1.
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

    // When adding to this union; you must use
    // fewer bytes than legacy to maintain forward/backward 
    // compatibility for all clients that use it.
    
    union
    {
        // legacy elements
        struct
        {
            WCHAR          *szDatabase;
            WCHAR          *szSLV_ObsoleteAndUnused; // No longer used. Left in to preserve the subsequent values;
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
            
        // ChecksumLogFromMemory
        struct
        {
            WCHAR           *szLog;     // Name of the Log file
            WCHAR           *szBase;    // Base name used e.g. "edb" or "E01"
            void            *pvBuffer;  // Pointer to buffer containing the log 
            long             cbBuffer;  // Length of buffer
        } checksumlogfrommemory;

        // opDBUTILDumpSpaceCategory
        struct
        {
            WCHAR              *szDatabase;            // Database from which to dump the space category of pages.
            unsigned long       pgnoFirst;             // First page to dump the category for. The first page in the database is 1.
            unsigned long       pgnoLast;              // Last page to dump the category for. The last page in the database can be passed in as (unsigned long)-1.
            void               *pfnSpaceCatCallback;   // Callback to receive each page's category (JET_SPCATCALLBACK).
            void               *pvContext;             // General purpose context.
        } spcatOptions;

        // opDBUTILDumpRBS
        struct
        {
            WCHAR               *szDatabase;           // Database from which to dump the space category of pages.
            unsigned long       pgnoFirst;             // First page to dump the category for. The first page in the database is 1.
            unsigned long       pgnoLast;              // Last page to dump the category for. The last page in the database can be passed in as (unsigned long)-1.
        } rbsOptions;

    };

} JET_DBUTIL_W;

#ifdef JET_UNICODE
#define JET_DBUTIL JET_DBUTIL_W
#else
#define JET_DBUTIL JET_DBUTIL_A
#endif

//
//  Command (DBUTIL_OP op) specific DBUtil options
//

#if ( JET_VERSION >= 0x0A01 )
// Space category flags. Returned by opDBUTILDumpSpaceCategory.
typedef enum
{
    spcatfNone               = 0x00000000,   // Not a real flag, just a constant to signal no flags.

    // Expected/consistent categories.
    spcatfStrictlyLeaf       = 0x00000001,   // Page is strictly a leaf (i.e., not a root).
    spcatfStrictlyInternal   = 0x00000002,   // Page is strictly an internal page (i.e., not a root).
    spcatfRoot               = 0x00000004,   // Page is a root.
    spcatfSplitBuffer        = 0x00000008,   // Page is part of a split buffer.
    spcatfSmallSpace         = 0x00000010,   // Page is part of a small space tree.
    spcatfSpaceOE            = 0x00000020,   // Page belongs to an OE tree.
    spcatfSpaceAE            = 0x00000040,   // Page belongs to an AE tree (to the space tree itself, not a free/available page).
    spcatfAvailable          = 0x00000080,   // Page is free/available at the root or object level.
    // spcatf... add in increasing bit significance order (make sure it doesn't collide with the exceptional states below).

    // Exceptional categories.
    spcatfIndeterminate      = 0x04000000,   // Page type could not be determined because full category search is not enabled.
    spcatfInconsistent       = 0x08000000,   // Page was found to be inconsistent during lookup.
    spcatfLeaked             = 0x10000000,   // Page is leaked at the root or object level.
    spcatfNotOwned           = 0x20000000,   // Page is not owned by the DB root and is before or at the last known-owned page (this means root OE corruption).
    spcatfNotOwnedEof        = 0x40000000,   // Page is not owned by the DB root and is beyond the last known-owned page.
    spcatfUnknown            = 0x80000000,   // Unknown page type.
    // spcatf... add in decreasing bit significance order (i.e., top of the list above).
} SpaceCategoryFlags;

// Callback used by opDBUTILDumpSpaceCategory to return page space categories.
typedef void (JET_API *JET_SPCATCALLBACK)( _In_ const unsigned long pgno, _In_ const unsigned long objid, _In_ const SpaceCategoryFlags spcatf, _In_opt_ void* const pvContext );
#endif // JET_VERSION >= 0x0A01

//  DBUTIL_OP op = opDBUTILDumpSpace
//
#define JET_bitDBUtilSpaceInfoBasicCatalog          0x00000001
#define JET_bitDBUtilSpaceInfoSpaceTrees                0x00000002
#define JET_bitDBUtilSpaceInfoParentOfLeaf          0x00000004
#define JET_bitDBUtilSpaceInfoFullWalk              0x00000008
//  This command also utilizes this option:
//      JET_bitDBUtilOptionDumpVerbose          0x10000000

#if ( JET_VERSION >= 0x0A01 )
//  DBUTIL_OP op = opDBUTILDumpSpaceCategory
//
#define JET_bitDBUtilFullCategorization         0x00000001
#endif // JET_VERSION >= 0x0A01

//  DBUTIL_OP op = <all others>
//
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
//#define JET_bitDBUtilOptionRepairSLVChecksum  0x00001000  //  DEPRECATED
//#define JET_bitDBUtilOptionRepairMissingStream    0x00002000  //  DEPRECATED
//#define JET_bitDBUtilOptionIgnoreDbSLVMismatch    0x00004000  //  DEPRECATED
#define JET_bitDBUtilOptionSuppressLogo         0x00008000
#define JET_bitDBUtilOptionRepairCheckOnly      0x00010000
#define JET_bitDBUtilOptionDumpLVPageUsage      0x00020000
#if ( JET_VERSION >= 0x0600 )
#define JET_bitDBUtilOptionDumpLogInfoCSV           0x00040000
#define JET_bitDBUtilOptionDumpLogPermitPatching    0x00080000  //  permit the log dumper to patch edb.jtx/log if necessary
#endif // JET_VERSION >= 0x0600
#define JET_bitDBUtilOptionSkipMinLogChecksUpdateHeader 0x00100000
#define JET_bitDBUtilOptionDumpVerbose          0x10000000
#define JET_bitDBUtilOptionDumpVerboseLevel1    JET_bitDBUtilOptionDumpVerbose
#define JET_bitDBUtilOptionDumpVerboseLevel2    0x20000000
//#define JET_bitDBUtilOptionCheckBTree           0x20000000  //  DEPRECATED
#define JET_bitDBUtilOptionDumpLogSummary       0x40000000

// begin_PubEsent

//  Configuration Store 
//
//  ESE has the ability to use an external config store for ESE database engine and instance
//  settings.
//
//  Using the registry this might look something like setting the JET param to this:
//
//      JET_paramConfigStoreSpec    "reg:HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\PopServer(Inst1)"
//
//          where "PopServer(Inst1)" is just an exaple name, you should pick a different name or 
//          even a different part of the registry if appropriate.  You are limited however to 
//          beginning under: HKEY_LOCAL_MACHINE or HKEY_CURRENT_USER.               
//
//      And configuring the registry thusly:
//
//Windows Registry Editor Version 5.00
//
//[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\PopServer(Inst1)]
//
//[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\PopServer(Inst1)\SysParamDefault]
//"JET_paramDatabasePageSize"="8192"
//"JET_paramEnableFileCache"="1"
//
//  All values should be set as a string version of a decimal of hex values (such as "0x2000").
//


//
//  The security settings / [D]ACL of the registry key provided to JET_paramConfigStoreSpec
//  should be locked down to the same security context as the client application that uses ESE
//  and opens the database, otherwise there is a possible Escalation of Privilege attack.
//

//  The JET_wszConfigStoreReadControl in the registry are registry values under the root 
//  registry key passed to JET_paramConfigStoreSpec.

#define JET_wszConfigStoreReadControl                           L"CsReadControl"
#define JET_bitConfigStoreReadControlInhibitRead                 0x1        //  Will stop reading from the registry config store, and pause reading until flag is removed (this will stall some JET initialization APIs).
#define JET_bitConfigStoreReadControlDisableAll                  0x2        //  Simply disables the registry config store from being read or used.
// end_PubEsent
#define JET_bitConfigStoreReadControlDisableSetParamRead         0x4        //  If ESE should not read the config data at the time we set the JET_paramConfigStoreSpec via JetSetSystemParameter().
#define JET_bitConfigStoreReadControlDisableGlobalInitRead       0x8        //  If ESE should not read the config data at the time of global initialization / when ESE creates the very first instance.
#define JET_bitConfigStoreReadControlDisableInstInitRead        0x10        //  If ESE should not read the config data at the time we initialize the instance.
#define JET_bitConfigStoreReadControlDisableLiveRead            0x20        //  [Reserved for future usage at some point] If ESE should not read / update the values live.
// begin_PubEsent
#define JET_bitConfigStoreReadControlDefault                     0x0        //  Use default ESE behavior.

// end_PubEsent
//  Useful for debugging and rapid development, this will fill out the ESE Config
//  Store registry key with all the values that the engine reads during runtime.

#define JET_wszConfigStorePopulateControl                       L"CsPopulateControl"
#define JET_bitConfigStorePopulateControlOff                    0x00
#define JET_bitConfigStorePopulateControlOn                     0x01
// begin_PubEsent

//  The JET_wszConfigStoreRelPathSysParamDefault and JET_wszConfigStoreRelPathSysParamOverride in 
//  the registry are registry sub-keys under the root registry key passed to JET_paramConfigStoreSpec.
#define JET_wszConfigStoreRelPathSysParamDefault        L"SysParamDefault"
#define JET_wszConfigStoreRelPathSysParamOverride       L"SysParamOverride"


// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )

// These are _not_ mis-tabbed, intentionally tabbed in twice for released format versions to be able to easily
// distinguish them from independent format features, while keeping them in order. 4 space tab width.

#define JET_efvExchange55Rtm                     500                //  Final format version for Exchange 5.5 at RTM.  This is basically ESE97.
#define JET_efvWindows2000Rtm                    600                //  Final format version for Windows 2000 at RTM.  This is basically ESE97.1.
#define JET_efvExchange2000Rtm                   1000               //  Final format version for Exchange 2000 at RTM.  This is basically ESE98.
#define JET_efvWindowsXpRtm                      1200               //  Final format version for Windows XP at RTM.  This is basically ESE98.1.
#define JET_efvExchange2010Rtm                   5520               //  Final format version for Exchange 2010 (E14) at RTM.
#define JET_efvExchange2010Sp1                   5920               //  Final format version for Exchange 2010 (E14) at SP1.
#define JET_efvExchange2010Sp2                   6080               //  Final format version for Exchange 2010 (E14) at SP2.
//      JET_efvWindows8                          ~6520              //  Final format version for Windows 8 RTM.
#define JET_efvScrubLastNodeOnEmptyPage                     6960    //  Scrub the remaining prefix key on an empty page.
#define JET_efvExchange2013Rtm                   7020               //  Final format version for Exchange 2013 (E15) at RTM.
#define JET_efvExtHdrRootFieldAutoIncStorageStagedToDebug   7060    //  Extend node/page's extended header to have multiple root fields values therein; And store the max Auto-Increment Column value used in a new extended header root field.
//      JET_efvExchange2013Cu6                   7900               //  Final format version for Exchange 2013 (E15) at CU6.
#define JET_efvLogExtendDbLrAfterCreateDbLr                 7940    //  Allow the recently (as of 6 months prior) variable initial DB size to be logged with an ExtendDB LR so that recovery can always recover the DB to the exact same size as intended by the ESE consumer.
#define JET_efvRollbackInsertSpaceStagedToTest              7960    //  Implemented rolling back of insert operations triggers delete/merge to cleanup the previously used space.
#define JET_efvWindows10Rtm                     (8020)              //  Final format version for Windows Threshold / 10.
#define JET_efvQfeMsysLocalesGuidRefCountFixup              8023    //  See below JET_efvMsysLocalesGuidRefCountFixup.
#define JET_efvPersistedLostFlushMapStagedToDebug           8220    //  Added signDbHdrFlush, signFlushMapHdrFlush, le_lGenMinConsistent and the new persisted flush map.
#define JET_efvTriStatePageFlushTypeStagedToProd            8280    //  Using a 2-bit state in the database page hdr to indicate flush state.
#define JET_efvLogSplitMergeCompressionReleased             8400    //  Graduate Split/Merge log record compression to retail.
#define JET_efvTriStatePageFlushTypeReleased                8420    //  Using a 2-bit state in the database page hdr to indicate flush state in retail.
#define JET_efvIrsDbidBasedMacroInfo2                       8440    //  Adds lrtypMacroInfo2 for IRS of multiple DBs attached to a single log stream.
#define JET_efvLostFlushMapCrashResiliency                  8460    //  Add crash resiliency to the lost flush map.
#define JET_efvExchange2016Rtm                   8520               //  Final format version for Exchange 2016 RTM.  Actually released summer of 2015.
#define JET_efvMsysLocalesGuidRefCountFixup                 8620    //  Updated MsysLocales init code to fixup the ref counts of GUIDs based upon right version correctly.
#define JET_efvWindows10v2Rtm                    8620               //  Final format version for Windows 10 Version 1511 (Threshold 2).
#define JET_efvEncryptedColumns                             8720    //  Encrypted columns.
#define JET_efvLoggingDeleteOfTableFcbs                     8820    //  Signal deleted of FCBs to keep recovery cache correct.
#define JET_efvEmptyIdxUpgradesUpdateMsysLocales            8860    //  Ensure we update the MSysLocales table when we upgraded for empty indices.
#define JET_efvSupportNlsInvariantLocale                    8880    //  Supports creating a table or index with the invariant locale sorting purposes.
#define JET_efvExchange2016Cu1Rtm                8920               //  More of a .1 release, as its not just QFEs. CU1 forked 2015/01/05 - 15.1.396.
#define JET_efvWindows19H1Rtm                    8920               //  Last pre-efv version, shipped in windows 10 until 19H1
#define JET_efvSetDbVersion                                 8940    //  Added lrtypSetDbVersion to log DB version updates, and a UpdateMinor and a efv"LastAttached" version to the DB file header.
                                                                    //      Notes: Guarantees via recovery that a DB version representative of what the transaction log 
                                                                    //      has redone to the DB file (layering violation info: and most importantly to fix replicas 
                                                                    //      in Exchange).
#define JET_efvExtHdrRootFieldAutoIncStorageReleased        8960    //  Moves to retail the extend node/page's extended header to have multiple root fields values therein; And store the max Auto-Increment Column value used in a new extended header root field.
#define JET_efvXpress9Compression                           8980    //  Adds support for compressing/decompressing data using Xpress9.
#define JET_efvUppercaseTextNormalization                   9000    //  Allow LCMAP_UPPERCASE for OS text normalization flags in index definitions.
#define JET_efvDbscanHeaderHighestPageSeen                  9020    //  Adds support for tracking (in the header) the highest page seen by dbscan follower
#define JET_efvEscrow64                                     9040    //  Adds support for 64-bit escrow columns.
#define JET_efvSynchronousLVCleanup                         9060    //  Adds support for cleaning up LVs synchronously
#define JET_efvLid64                                        9080    //  New LID format: LIDs become 64-bit numbers.
#define JET_efvShrinkEof                                    9100    //  Added lrtypShrinkDB2, which changes the the meaning of cpgShrunk in the existing lrtypShrinkDB and how it is replayed.
#define JET_efvLogNewPage                                   9120    //  Added lrtypNewPage, which logs new page operations to better handle rolling page incomplete operations that require new pages.
#define JET_efvRootPageMove                                 9140    //  Added support for moving tree roots and their respective space tree roots (including new lrtypRootPageMove and lrtypSignalAttachDb).
#define JET_efvScanCheck2                                   9160    //  Added new log record lrtypScanCheck2 to allow for for reporting of the initiator of the ScanCheck log record.
#define JET_efvLgposLastResize                              9180    //  Stamps the log position of the last database resize operation to the database header.
#define JET_efvShelvedPages                                 9200    //  Added the concept of shelved pages (available or leaked pages beyond EOF).
#define JET_efvShelvedPagesRevert                           9220    //  Reverts JET_efvShelvedPages, with additional code to make it a safe revert.
#define JET_efvShelvedPages2                                9240    //  Redoes JET_efvShelvedPages.
#define JET_efvLogtimeGenMaxRequired                        9260    //  Adds logtimeGenMaxRequired to database header
#define JET_efvVariableDbHdrSignatureSnapshot               9280    //  Adds ability to append fields to the DB header without doing a major update by storing a variable-sized header signature.
#define JET_efvLowerMinReqLogGenOnRedo                      9300    //  Brings the minimum required log generation down if we start replaying below the current min. required.
#define JET_efvDbTimeShrink                                 9320    //  Added lrtypShrinkDB3, which logs the database's running dbtime at the time of the shrink operation, to allow for replaying the shrink operation more granularly.
#define JET_efvXpress10Compression                          9340    //  Adds support for compressing/decompressing data using Xpress10.
#define JET_efvRevertSnapshot                               9360    //  Added revert snapshot flush signature to database header and added lrtypExtentFreed, which logs details about the extent freed to allow for revert snapshot to capture the pages of the freed extent.
#define JET_efvApplyRevertSnapshot                          9380    //  Added le_lgposCommitBeforeRevert to the database header which captures the last commit lgpos before revert was done and is used to ignore JET_errDbTimeTooOld errors on pasive copies.
#define JET_efvExtentPageCountCache                         9400    //  Adds support for the ExtentPageCountCache table.
#define JET_efvLz4Compression                               9420    //  Adds support for compressing/decompressing data using Lz4.

// Special format specifiers here
#define JET_efvUseEngineDefault             (0x40000001)    //  Instructs the engine to use the maximal default supported Engine Format Version. (default)
#define JET_efvUsePersistedFormat           (0x40000002)    //  Instructs the engine to use the minimal Engine Format Version of all loaded log and DB files.
#define JET_efvAllowHigherPersistedFormat   (0x41000000)    //  Can be combined with a specific EngineFormatVersion but will not fail if persisted files are ahead of the specified EngineFormatVersion.  Will still fail if the persisted version is ahead of what the engine actually can read/understand.

#endif // JET_VERSION >= 0x0A01


#if ( JET_VERSION >= 0x0601 )
//  JetDatabaseScan options
#define JET_bitDatabaseScanBatchStart           0x00000010  //  Starts a single-pass Database Maintenance thread.
#define JET_bitDatabaseScanBatchStop            0x00000020  //  Stops the Database Maintenance thread and frees any resources associated with it.
#define JET_bitDatabaseScanZeroPages            0x00000040  //  DEPRECATED
#define JET_bitDatabaseScanBatchStartContinuous 0x00000080  //  Starts a continuously-running Database Maintenance thread. pcSecondsMax, cmsecSleep and pfnCallback API parameters are not honored (system parameters should be used to fine-tune DBM).
#endif // JET_VERSION >= 0x0601

// begin_PubEsent

//  Online defragmentation (JetDefragment/JetDefragment2) options
#define JET_bitDefragmentBatchStart             0x00000001
#define JET_bitDefragmentBatchStop              0x00000002

// end_PubEsent
// Obsolete:
// #define JET_bitDefragmentTest                    0x00000004  /* run internal tests (non-RTM builds only) */
// #define JET_bitDefragmentSLVBatchStart           0x00000008
// #define JET_bitDefragmentSLVBatchStop            0x00000010
// #define JET_bitDefragmentScrubSLV                0x00000020  /* synchronously zero free pages in the streaming file */
// begin_PubEsent
#if ( JET_VERSION >= 0x0501 )
#define JET_bitDefragmentAvailSpaceTreesOnly    0x00000040  /* only defrag AvailExt trees */
#endif // JET_VERSION >= 0x0501
#if ( JET_VERSION >= 0x0601 )
#define JET_bitDefragmentNoPartialMerges        0x00000080  /* don't do partial merges during OLD */

#define JET_bitDefragmentBTree                  0x00000100  /* defrag one B-Tree */
// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitDefragmentBTreeBatch             0x00000200  /* specifies options pertain to OLD2 / B-Tree defrag, such as JET_bitDefragmentBatchStart */
#endif // JET_VERSION >= 0x0A01

// begin_PubEsent


#endif // JET_VERSION >= 0x0601

#if ( JET_VERSION >= 0x0501 )
    /* Callback-function types */

#define JET_cbtypNull                           0x00000000
#define JET_cbtypFinalize                       0x00000001  /* DEPRECATED: a finalizable column has gone to zero */
#define JET_cbtypBeforeInsert                   0x00000002  /* about to insert a record */
#define JET_cbtypAfterInsert                    0x00000004  /* finished inserting a record */
#define JET_cbtypBeforeReplace                  0x00000008  /* about to modify a record */
#define JET_cbtypAfterReplace                   0x00000010  /* finished modifying a record */
#define JET_cbtypBeforeDelete                   0x00000020  /* about to delete a record */
#define JET_cbtypAfterDelete                    0x00000040  /* finished deleting the record */
#define JET_cbtypUserDefinedDefaultValue        0x00000080  /* calculating a user-defined default */
#define JET_cbtypOnlineDefragCompleted          0x00000100  /* a call to JetDefragment2 has completed */
#define JET_cbtypFreeCursorLS                   0x00000200  /* the Local Storage associated with a cursor must be freed */
#define JET_cbtypFreeTableLS                    0x00000400  /* the Local Storage associated with a table must be freed */
// end_PubEsent
#if ( JET_VERSION >= 0x0601 )
//  actions for the defragmentation callback
#define JET_bitOld2Start                        0x00000001  /* defrag action start (testing only) */
#define JET_bitOld2Suspend                      0x00000002  /* defrag action suspend (testing only) */
#define JET_bitOld2Resume                       0x00000004  /* defrag action resume (testing only) */
#define JET_bitOld2End                          0x00000008  /* defrag action end (testing only) */

#define JET_cbtypScanProgress                   0x00004000  /* JetDatabaseScan progress */
#define JET_cbtypScanCompleted                  0x00008000  /* JetDatabaseScan completed a pass */
#endif // JET_VERSION >= 0x0601
#define JET_cbtypDTCQueryPreparedTransaction    0x00001000  /* recovery is attempting to resolve a PreparedToCommit transaction */
#define JET_cbtypOnlineDefragProgress           0x00002000  /* online defragmentation has made progress */
//  callback for JetDefragment2 actions (testing only)
//  pvArg1 is ptr to table name, pvArg2 is ptr to action code JET_bitOld2Start, etc...
#define JET_cbtypOld2Action          0x00004000  

// begin_PubEsent

    /* Callback-function prototype */

typedef JET_ERR (JET_API *JET_CALLBACK)(
    _In_ JET_SESID      sesid,
    _In_ JET_DBID       dbid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_CBTYP      cbtyp,
    _Inout_opt_ void *  pvArg1,
    _Inout_opt_ void *  pvArg2,
    _In_opt_ void *     pvContext,
    _In_ JET_API_PTR    ulUnused );
#endif // JET_VERSION >= 0x0501
// end_PubEsent

//  changes to JET_cbCallbackUserDataMost should affect JET_cbCallbackDataAllMost as well
//  (see below)
//
#define JET_cbCallbackUserDataMost              1024

//  callback data most is the following:
//
//  sizeof(JET_USERDEFINEDDEFAULT)
//  + JET_cbNameMost + 1 /* for callback name */
//  + JET_cbCallbackUserDataMost
//  + ( ( JET_ccolKeyMost * ( JET_cbNameMost + 1 ) ) + 1 ) /* for list of dependent columns */
//  = ~ 3234 - 3246 bytes
//
#define JET_cbCallbackDataAllMost               4096


//  return JET_errFileIOAbort, JET_errFileIORetry, or JET_errFileIOFail
//
//  currently UNSUPPORTED
//
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
//  trace-tag support
//
typedef enum
{
    JET_tracetagNull,
    JET_tracetagInformation,            // [Informational] Information trace tag.
    JET_tracetagErrors,                 // [Informational] Errors trace tag.
    JET_tracetagAsserts,                // [Error] Asserts trace tag.
    JET_tracetagAPI,
    JET_tracetagInitTerm,               // High level status traces for beginning and finishing of instance-level init/term.
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
    JET_tracetagBufferManagerMaintTasks,    // BufferManagerMaintTasks trace tag. Introduced in Windows 7.
    JET_tracetagSpaceManagement,
    JET_tracetagSpaceInternal,
    JET_tracetagIOQueue,
    JET_tracetagDiskVolumeManagement,
#if ( JET_VERSION >= 0x0602 )
    JET_tracetagCallbacks,              // Callbacks trace tag. Introduced in Windows 8.
    JET_tracetagIOProblems,             // [Error] I/O Problems.
    JET_tracetagUpgrade,
    JET_tracetagRecoveryValidation,
    JET_tracetagBufferManagerBufferCacheState,
    JET_tracetagBufferManagerBufferDirtyState,
    JET_tracetagTimerQueue,
    JET_tracetagSortPerf,
#if ( JET_VERSION >= 0x0A00 )
    JET_tracetagOLDRegistration,        // Registration events for online defragmentation. Introduced in Windows 10.
    JET_tracetagOLDWork,                // Work events for online defragmentation progress. Introduced in Windows 10.
    JET_tracetagSysInitTerm,            // High level status traces for beginning and finishing of one-time system init/term operations during DLL load/unload. Contrast to JET_tracetagInitTerm. Introduced in Windows 10.
#if ( JET_VERSION >= 0x0A01 )
    JET_tracetagVersionAndStagingChecks,
    JET_tracetagFile,
    JET_tracetagFlushFileBuffers,       // Traces for flush file buffers (including suppressed FFB calls).
    JET_tracetagCheckpointUpdate,
    JET_tracetagDiagnostics,
    JET_tracetagBlockCache,
    JET_tracetagRBS,
    JET_tracetagRBSCleaner,

    //  Add all new tracetags here, must be in order ...
#endif // JET_VERSION >= 0x0A01
#endif // JET_VERSION >= 0x0A00
#endif // JET_VERSION >= 0x0602
#endif // JET_VERSION >= 0x0601
    JET_tracetagMax,                    //  Maximum trace tag value (invalid).
} JET_TRACETAG;


// UNICODE_UNDONE_DEFERRED: change the APIs below to use UNICODE
//
//  tracing callbacks
//
typedef void (JET_API *JET_PFNTRACEEMIT)(
    _In_ const JET_TRACETAG tag,
    _In_ JET_PCSTR          szPrefix,
    _In_ JET_PCSTR          szTrace,
    _In_ const JET_API_PTR  ul );
typedef void (JET_API *JET_PFNTRACEREGISTER)(
    _In_ const JET_TRACETAG tag,
    _In_ JET_PCSTR          szDesc,
    _Out_ JET_API_PTR *     pul );


//  JetTracing operations
//
typedef enum
{
    JET_traceopNull,
    JET_traceopSetGlobal,               //  enable/disable tracing ("ul" param cast to BOOL)
    JET_traceopSetTag,                  //  enable/disable tracing for specified tag ("ul" param cast to BOOL)
    JET_traceopSetAllTags,              //  enable/disable tracing for all tags ("ul" param cast to BOOL)
    JET_traceopSetMessagePrefix,        //  text which should prefix all emitted messages ("ul" param cast to CHAR*)
    JET_traceopRegisterTag,             //  callback to register a trace tag ("ul" param cast to JET_PFNTRACEREGISTER)
    JET_traceopRegisterAllTags,         //  callback to register all trace tags ("ul" param cast to JET_PFNTRACEREGISTER)
    JET_traceopSetEmitCallback,         //  override default trace emit function with specified function, or pass NULL to revert to default ("ul" param ast to JET_PFNTRACEEMIT)
    JET_traceopSetThreadidFilter,       //  threadid to use to filter traces (0==all threads, -1==no threads)
    JET_traceopSetDbidFilter,           //  JET_DBID to use to filter traces (0x7fffffff==all db's, JET_dbidNil==no db's)
    JET_traceopMax
} JET_TRACEOP;
#endif // JET_VERSION >= 0x0600


    /*  Session information bits */

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
#endif // JET_VERSION >= 0x0600
// begin_PubEsent

    /* Status Notification Structures */

typedef struct              /* Status Notification Progress */
{
    unsigned long   cbStruct;   /* Size of this structure */
    unsigned long   cunitDone;  /* Number of units of work completed */
    unsigned long   cunitTotal; /* Total number of units of work */
} JET_SNPROG;

typedef struct
{
    unsigned long           cbStruct;

    unsigned long           cbFilesizeLow;          //  file's current size (low DWORD)
    unsigned long           cbFilesizeHigh;         //  file's current size (high DWORD)

    unsigned long           cbFreeSpaceRequiredLow; //  estimate of free disk space required for in-place upgrade (low DWORD)
    unsigned long           cbFreeSpaceRequiredHigh;//  estimate of free disk space required for in-place upgrade (high DWORD)

    unsigned long           csecToUpgrade;          //  estimate of time required, in seconds, for upgrade

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
    JET_DATESERIAL      dtCreate;   //  Deprecated.
    JET_DATESERIAL      dtUpdate;   //  Deprecated.
    JET_GRBIT           grbit;
    unsigned long       flags;
    unsigned long       cRecord;
    unsigned long       cPage;
} JET_OBJECTINFO;

    /* The following flags appear in the grbit field above */

#define JET_bitTableInfoUpdatable   0x00000001
#define JET_bitTableInfoBookmark    0x00000002
#define JET_bitTableInfoRollback    0x00000004

    /* The following flags occur in the flags field above */

#define JET_bitObjectSystem         0x80000000  // Internal use only
#define JET_bitObjectTableFixedDDL  0x40000000  // Table's DDL is fixed
#define JET_bitObjectTableTemplate  0x20000000  // Table's DDL is inheritable (implies FixedDDL)
#define JET_bitObjectTableDerived   0x10000000  // Table's DDL is inherited from a template table
// end_PubEsent
#define JET_bitObjectSystemDynamic  (JET_bitObjectSystem|0x08000000)    //  Internal use only (dynamic system objects)
// begin_PubEsent
#if ( JET_VERSION >= 0x0501 )
#define JET_bitObjectTableNoFixedVarColumnsInDerivedTables  0x04000000  //  used in conjunction with JET_bitObjectTableTemplate
                                                                        //    to disallow fixed/var columns in derived tables (so that
                                                                        //    fixed/var columns may be added to the template in the future)
#endif // JET_VERSION >= 0x0501

typedef struct
{
    unsigned long   cbStruct;
    JET_TABLEID     tableid;
    unsigned long   cRecord;
    JET_COLUMNID    columnidcontainername;
    JET_COLUMNID    columnidobjectname;
    JET_COLUMNID    columnidobjtyp;
    JET_COLUMNID    columniddtCreate;   //  XXX -- to be deleted
    JET_COLUMNID    columniddtUpdate;   //  XXX -- to be deleted
    JET_COLUMNID    columnidgrbit;
    JET_COLUMNID    columnidflags;
    JET_COLUMNID    columnidcRecord;    /* Level 2 info */
    JET_COLUMNID    columnidcPage;      /* Level 2 info */
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
    JET_COLUMNID    columnidCountry;        // specifies the columnid for the country/region field
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
    unsigned short  wCountry;           // sepcifies the country/region for the column definition
    unsigned short  langid;
    unsigned short  cp;
    unsigned short  wCollate;       /* Must be 0 */
    unsigned long   cbMax;
    JET_GRBIT       grbit;
} JET_COLUMNDEF;


typedef struct
{
    unsigned long   cbStruct;
    JET_COLUMNID    columnid;
    JET_COLTYP      coltyp;
    unsigned short  wCountry;           // specifies the columnid for the country/region field
    unsigned short  langid;
    unsigned short  cp;
    unsigned short  wFiller;       /* Must be 0 */
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
    unsigned short  wCountry;           // specifies the columnid for the country/region field
    unsigned short  langid;
    unsigned short  cp;
    unsigned short  wFiller;       /* Must be 0 */
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
    JET_COLUMNID    columnidCountry;        // specifies the columnid for the country/region field
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
    unsigned long   cbStruct;               // size of this structure (for future expansion)
    char            *szColumnName;          // column name
    JET_COLTYP      coltyp;                 // column type
    unsigned long   cbMax;                  // the maximum length of this column (only relevant for binary and text columns)
    JET_GRBIT       grbit;                  // column options
    void            *pvDefault;             // default value (NULL if none)
    unsigned long   cbDefault;              // length of default value
    unsigned long   cp;                     // code page (for text columns only)
    JET_COLUMNID    columnid;               // returned column id
    JET_ERR         err;                    // returned error code
} JET_COLUMNCREATE_A;

typedef struct tag_JET_COLUMNCREATE_W
{
    unsigned long   cbStruct;               // size of this structure (for future expansion)
    WCHAR           *szColumnName;          // column name
    JET_COLTYP      coltyp;                 // column type
    unsigned long   cbMax;                  // the maximum length of this column (only relevant for binary and text columns)
    JET_GRBIT       grbit;                  // column options
    void            *pvDefault;             // default value (NULL if none)
    unsigned long   cbDefault;              // length of default value
    unsigned long   cp;                     // code page (for text columns only)
    JET_COLUMNID    columnid;               // returned column id
    JET_ERR         err;                    // returned error code
} JET_COLUMNCREATE_W;

#ifdef JET_UNICODE
#define JET_COLUMNCREATE JET_COLUMNCREATE_W
#else
#define JET_COLUMNCREATE JET_COLUMNCREATE_A
#endif

#if ( JET_VERSION >= 0x0501 )
//  This is the information needed to create a column with a user-defined default. It should be passed in using
//  the pvDefault and cbDefault in a JET_COLUMNCREATE structure

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

#endif // JET_VERSION >= 0x0501

typedef struct tagJET_CONDITIONALCOLUMN_A
{
    unsigned long   cbStruct;               // size of this structure (for future expansion)
    char            *szColumnName;          // column that we are conditionally indexed on
    JET_GRBIT       grbit;                  // conditional column options
} JET_CONDITIONALCOLUMN_A;

typedef struct tagJET_CONDITIONALCOLUMN_W
{
    unsigned long   cbStruct;               // size of this structure (for future expansion)
    WCHAR           *szColumnName;          // column that we are conditionally indexed on
    JET_GRBIT       grbit;                  // conditional column options
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
#endif //JET_VERSION >= 0x0602

#if ( JET_VERSION >= 0x0502 )
typedef struct tagJET_TUPLELIMITS
{
    unsigned long   chLengthMin;
    unsigned long   chLengthMax;
    unsigned long   chToIndexMax;
#if ( JET_VERSION >= 0x0600 )
    unsigned long   cchIncrement;
    unsigned long   ichStart;
#endif // JET_VERSION >= 0x0600
} JET_TUPLELIMITS;
#endif // JET_VERSION >= 0x0502

#if ( JET_VERSION >= 0x0601 )
//  This structure describes some of the hints we can give to a given B-tree, be it a
//  table, index, or the internal long values tree.
typedef struct tagJET_SPACEHINTS
{
    unsigned long       cbStruct;           //  size of this structure 
    unsigned long       ulInitialDensity;   //  density at (append) layout.
    unsigned long       cbInitial;          //  initial size (in bytes).

    JET_GRBIT           grbit;              //  Combination of one or more flags from
                                            //      JET_bitSpaceHints* flags
                                            //      JET_bitCreateHints* flags
                                            //      JET_bitRetrieveHints* flags
                                            //      JET_bitUpdateHints* flags
                                            //      JET_bitDeleteHints* flags
    unsigned long       ulMaintDensity;     //  density to maintain at.
    unsigned long       ulGrowth;           //  percent growth from:
                                            //    last growth or initial size (possibly rounded to nearest native JET allocation size).
    unsigned long       cbMinExtent;        //  This overrides ulGrowth if too small.
    unsigned long       cbMaxExtent;        //  This caps ulGrowth.
} JET_SPACEHINTS;
#endif // JET_VERSION >= 0x0601

// end_PubEsent

// Needed to detect if the older JET_INDEXCREATE structure
// was used (backward compatibility).
typedef struct tagJET_INDEXCREATEOLD_A
{
    unsigned long           cbStruct;               // size of this structure (for future expansion)
    char                    *szIndexName;           // index name
    char                    *szKey;                 // index key definition
    unsigned long           cbKey;                  // size of key definition in szKey
    JET_GRBIT               grbit;                  // index options
    unsigned long           ulDensity;              // index density

    union
    {
        unsigned long       lcid;                   // lcid for the index (if JET_bitIndexUnicode NOT specified)
        JET_UNICODEINDEX    *pidxunicode;           // pointer to JET_UNICODEINDEX struct (if JET_bitIndexUnicode specified)
    };

    union
    {
        unsigned long       cbVarSegMac;            // maximum length of variable length columns in index key (if JET_bitIndexTupleLimits not specified)
#if ( JET_VERSION >= 0x0502 )
        JET_TUPLELIMITS     *ptuplelimits;          // pointer to JET_TUPLELIMITS struct (if JET_bitIndexTupleLimits specified)
#endif // ! JET_VERSION >= 0x0502
    };

    JET_CONDITIONALCOLUMN_A *rgconditionalcolumn;   // pointer to conditional column structure
    unsigned long           cConditionalColumn;     // number of conditional columns
    JET_ERR                 err;                    // returned error code
} JET_INDEXCREATEOLD_A;

typedef struct tagJET_INDEXCREATEOLD_W
{
    unsigned long           cbStruct;               // size of this structure (for future expansion)
    WCHAR                   *szIndexName;           // index name
    WCHAR                   *szKey;                 // index key definition
    unsigned long           cbKey;                  // size of key definition in szKey
    JET_GRBIT               grbit;                  // index options
    unsigned long           ulDensity;              // index density

    union
    {
        unsigned long       lcid;                   // lcid for the index (if JET_bitIndexUnicode NOT specified)
        JET_UNICODEINDEX    *pidxunicode;           // pointer to JET_UNICODEINDEX struct (if JET_bitIndexUnicode specified)
    };

    union
    {
        unsigned long       cbVarSegMac;            // maximum length of variable length columns in index key (if JET_bitIndexTupleLimits not specified)
#if ( JET_VERSION >= 0x0502 )
        JET_TUPLELIMITS     *ptuplelimits;          // pointer to JET_TUPLELIMITS struct (if JET_bitIndexTupleLimits specified)
#endif // ! JET_VERSION >= 0x0502
    };

    JET_CONDITIONALCOLUMN_W *rgconditionalcolumn;   // pointer to conditional column structure
    unsigned long           cConditionalColumn;     // number of conditional columns
    JET_ERR                 err;                    // returned error code
} JET_INDEXCREATEOLD_W;

#ifdef JET_UNICODE
#define JET_INDEXCREATEOLD JET_INDEXCREATEOLD_W
#else
#define JET_INDEXCREATEOLD JET_INDEXCREATEOLD_A
#endif

// begin_PubEsent

typedef struct tagJET_INDEXCREATE_A
{
    unsigned long           cbStruct;               // size of this structure (for future expansion)
    char                    *szIndexName;           // index name
    char                    *szKey;                 // index key definition
    unsigned long           cbKey;                  // size of key definition in szKey
    JET_GRBIT               grbit;                  // index options
    unsigned long           ulDensity;              // index density

    union
    {
        unsigned long       lcid;                   // lcid for the index (if JET_bitIndexUnicode NOT specified)
        JET_UNICODEINDEX    *pidxunicode;           // pointer to JET_UNICODEINDEX struct (if JET_bitIndexUnicode specified)
    };

    union
    {
        unsigned long       cbVarSegMac;            // maximum length of variable length columns in index key (if JET_bitIndexTupleLimits not specified)
#if ( JET_VERSION >= 0x0502 )
        JET_TUPLELIMITS     *ptuplelimits;          // pointer to JET_TUPLELIMITS struct (if JET_bitIndexTupleLimits specified)
#endif // ! JET_VERSION >= 0x0502
    };

    JET_CONDITIONALCOLUMN_A *rgconditionalcolumn;   // pointer to conditional column structure
    unsigned long           cConditionalColumn;     // number of conditional columns
    JET_ERR                 err;                    // returned error code
#if ( JET_VERSION >= 0x0600 )
    unsigned long           cbKeyMost;              // size of key preserved in index, e.g. without truncation (if JET_bitIndexKeyMost specified)
#endif // JET_VERSION >= 0x0600
} JET_INDEXCREATE_A;

typedef struct tagJET_INDEXCREATE_W
{
    unsigned long           cbStruct;               // size of this structure (for future expansion)
    WCHAR                   *szIndexName;           // index name
    WCHAR                   *szKey;                 // index key definition
    unsigned long           cbKey;                  // size of key definition in szKey
    JET_GRBIT               grbit;                  // index options
    unsigned long           ulDensity;              // index density

    union
    {
        unsigned long       lcid;                   // lcid for the index (if JET_bitIndexUnicode NOT specified)
        JET_UNICODEINDEX    *pidxunicode;           // pointer to JET_UNICODEINDEX struct (if JET_bitIndexUnicode specified)
    };

    union
    {
        unsigned long       cbVarSegMac;            // maximum length of variable length columns in index key (if JET_bitIndexTupleLimits not specified)
#if ( JET_VERSION >= 0x0502 )
        JET_TUPLELIMITS     *ptuplelimits;          // pointer to JET_TUPLELIMITS struct (if JET_bitIndexTupleLimits specified)
#endif // ! JET_VERSION >= 0x0502
    };

    JET_CONDITIONALCOLUMN_W *rgconditionalcolumn;   // pointer to conditional column structure
    unsigned long           cConditionalColumn;     // number of conditional columns
    JET_ERR                 err;                    // returned error code
#if ( JET_VERSION >= 0x0600 )
    unsigned long           cbKeyMost;              // size of key preserved in index, e.g. without truncation (if JET_bitIndexKeyMost specified)
#endif // JET_VERSION >= 0x0600
} JET_INDEXCREATE_W;

#ifdef JET_UNICODE
#define JET_INDEXCREATE JET_INDEXCREATE_W
#else
#define JET_INDEXCREATE JET_INDEXCREATE_A
#endif

#if ( JET_VERSION >= 0x0601 )

typedef struct tagJET_INDEXCREATE2_A
{
    unsigned long           cbStruct;               // size of this structure (for future expansion)
    char                    *szIndexName;           // index name
    char                    *szKey;                 // index key definition
    unsigned long           cbKey;                  // size of key definition in szKey
    JET_GRBIT               grbit;                  // index options
    unsigned long           ulDensity;              // index density

    union
    {
        unsigned long       lcid;                   // lcid for the index (if JET_bitIndexUnicode NOT specified)
        JET_UNICODEINDEX    *pidxunicode;           // pointer to JET_UNICODEINDEX struct (if JET_bitIndexUnicode specified)
    };

    union
    {
        unsigned long       cbVarSegMac;            // maximum length of variable length columns in index key (if JET_bitIndexTupleLimits not specified)
        JET_TUPLELIMITS     *ptuplelimits;          // pointer to JET_TUPLELIMITS struct (if JET_bitIndexTupleLimits specified)
    };

    JET_CONDITIONALCOLUMN_A *rgconditionalcolumn;   // pointer to conditional column structure
    unsigned long           cConditionalColumn;     // number of conditional columns
    JET_ERR                 err;                    // returned error code
    unsigned long           cbKeyMost;              // size of key preserved in index, e.g. without truncation (if JET_bitIndexKeyMost specified)
    JET_SPACEHINTS *        pSpacehints;            // space allocation, maintenance, and usage hints 
} JET_INDEXCREATE2_A;

typedef struct tagJET_INDEXCREATE2_W
{
    unsigned long           cbStruct;               // size of this structure (for future expansion)
    WCHAR                   *szIndexName;           // index name
    WCHAR                   *szKey;                 // index key definition
    unsigned long           cbKey;                  // size of key definition in szKey
    JET_GRBIT               grbit;                  // index options
    unsigned long           ulDensity;              // index density

    union
    {
        unsigned long       lcid;                   // lcid for the index (if JET_bitIndexUnicode NOT specified)
        JET_UNICODEINDEX    *pidxunicode;           // pointer to JET_UNICODEINDEX struct (if JET_bitIndexUnicode specified)
    };

    union
    {
        unsigned long       cbVarSegMac;            // maximum length of variable length columns in index key (if JET_bitIndexTupleLimits not specified)
        JET_TUPLELIMITS     *ptuplelimits;          // pointer to JET_TUPLELIMITS struct (if JET_bitIndexTupleLimits specified)
    };

    JET_CONDITIONALCOLUMN_W *rgconditionalcolumn;   // pointer to conditional column structure
    unsigned long           cConditionalColumn;     // number of conditional columns
    JET_ERR                 err;                    // returned error code
    unsigned long           cbKeyMost;              // size of key preserved in index, e.g. without truncation (if JET_bitIndexKeyMost specified)
    JET_SPACEHINTS *        pSpacehints;            // space allocation, maintenance, and usage hints
} JET_INDEXCREATE2_W;

#ifdef JET_UNICODE
#define JET_INDEXCREATE2 JET_INDEXCREATE2_W
#else
#define JET_INDEXCREATE2 JET_INDEXCREATE2_A
#endif
#endif // JET_VERSION >= 0x0601

#if ( JET_VERSION >= 0x0602 )

typedef struct tagJET_INDEXCREATE3_A
{
    unsigned long           cbStruct;               // size of this structure (for future expansion)
    char                    *szIndexName;           // index name
    char                    *szKey;                 // index key definition
    unsigned long           cbKey;                  // size of key definition in szKey
    JET_GRBIT               grbit;                  // index options
    unsigned long           ulDensity;              // index density
    JET_UNICODEINDEX2       *pidxunicode;           // pointer to JET_UNICODEINDEX2 struct (if JET_bitIndexUnicode specified)

    union
    {
        unsigned long       cbVarSegMac;            // maximum length of variable length columns in index key (if JET_bitIndexTupleLimits not specified)
        JET_TUPLELIMITS     *ptuplelimits;          // pointer to JET_TUPLELIMITS struct (if JET_bitIndexTupleLimits specified)
    };

    JET_CONDITIONALCOLUMN_A *rgconditionalcolumn;   // pointer to conditional column structure
    unsigned long           cConditionalColumn;     // number of conditional columns
    JET_ERR                 err;                    // returned error code
    unsigned long           cbKeyMost;              // size of key preserved in index, e.g. without truncation (if JET_bitIndexKeyMost specified)
    JET_SPACEHINTS *        pSpacehints;            // space allocation, maintenance, and usage hints 
} JET_INDEXCREATE3_A;

typedef struct tagJET_INDEXCREATE3_W
{
    unsigned long           cbStruct;               // size of this structure (for future expansion)
    WCHAR                   *szIndexName;           // index name
    WCHAR                   *szKey;                 // index key definition
    unsigned long           cbKey;                  // size of key definition in szKey
    JET_GRBIT               grbit;                  // index options
    unsigned long           ulDensity;              // index density
    JET_UNICODEINDEX2       *pidxunicode;           // pointer to JET_UNICODEINDEX2 struct (if JET_bitIndexUnicode specified)

    union
    {
        unsigned long       cbVarSegMac;            // maximum length of variable length columns in index key (if JET_bitIndexTupleLimits not specified)
        JET_TUPLELIMITS     *ptuplelimits;          // pointer to JET_TUPLELIMITS struct (if JET_bitIndexTupleLimits specified)
    };

    JET_CONDITIONALCOLUMN_W *rgconditionalcolumn;   // pointer to conditional column structure
    unsigned long           cConditionalColumn;     // number of conditional columns
    JET_ERR                 err;                    // returned error code
    unsigned long           cbKeyMost;              // size of key preserved in index, e.g. without truncation (if JET_bitIndexKeyMost specified)
    JET_SPACEHINTS *        pSpacehints;            // space allocation, maintenance, and usage hints
} JET_INDEXCREATE3_W;

#ifdef JET_UNICODE
#define JET_INDEXCREATE3 JET_INDEXCREATE3_W
#else
#define JET_INDEXCREATE3 JET_INDEXCREATE3_A
#endif
#endif // JET_VERSION >= 0x0602

//
//      Table Creation Structures
//

typedef struct tagJET_TABLECREATE_A
{
    unsigned long       cbStruct;               // size of this structure (for future expansion)
    char                *szTableName;           // name of table to create.
    char                *szTemplateTableName;   // name of table from which to inherit base DDL
    unsigned long       ulPages;                // initial pages to allocate for table.
    unsigned long       ulDensity;              // table density.
    JET_COLUMNCREATE_A  *rgcolumncreate;        // array of column creation info
    unsigned long       cColumns;               // number of columns to create
    JET_INDEXCREATE_A       *rgindexcreate;         // array of index creation info
    unsigned long       cIndexes;               // number of indexes to create
    JET_GRBIT           grbit;
    JET_TABLEID         tableid;                // returned tableid.
    unsigned long       cCreated;               // count of objects created (columns+table+indexes).
} JET_TABLECREATE_A;

typedef struct tagJET_TABLECREATE_W
{
    unsigned long       cbStruct;               // size of this structure (for future expansion)
    WCHAR               *szTableName;           // name of table to create.
    WCHAR               *szTemplateTableName;   // name of table from which to inherit base DDL
    unsigned long       ulPages;                // initial pages to allocate for table.
    unsigned long       ulDensity;              // table density.
    JET_COLUMNCREATE_W  *rgcolumncreate;        // array of column creation info
    unsigned long       cColumns;               // number of columns to create
    JET_INDEXCREATE_W       *rgindexcreate;         // array of index creation info
    unsigned long       cIndexes;               // number of indexes to create
    JET_GRBIT           grbit;
    JET_TABLEID         tableid;                // returned tableid.
    unsigned long       cCreated;               // count of objects created (columns+table+indexes).
} JET_TABLECREATE_W;

#ifdef JET_UNICODE
#define JET_TABLECREATE JET_TABLECREATE_W
#else
#define JET_TABLECREATE JET_TABLECREATE_A
#endif

#if ( JET_VERSION >= 0x0501 )
typedef struct tagJET_TABLECREATE2_A
{
    unsigned long       cbStruct;               // size of this structure (for future expansion)
    char                *szTableName;           // name of table to create.
    char                *szTemplateTableName;   // name of table from which to inherit base DDL
    unsigned long       ulPages;                // initial pages to allocate for table.
    unsigned long       ulDensity;              // table density.
    JET_COLUMNCREATE_A  *rgcolumncreate;        // array of column creation info
    unsigned long       cColumns;               // number of columns to create
    JET_INDEXCREATE_A   *rgindexcreate;         // array of index creation info
    unsigned long       cIndexes;               // number of indexes to create
    char                *szCallback;            // callback to use for this table
    JET_CBTYP           cbtyp;                  // when the callback should be called
    JET_GRBIT           grbit;
    JET_TABLEID         tableid;                // returned tableid.
    unsigned long       cCreated;               // count of objects created (columns+table+indexes+callbacks).
} JET_TABLECREATE2_A;

typedef struct tagJET_TABLECREATE2_W
{
    unsigned long       cbStruct;               // size of this structure (for future expansion)
    WCHAR               *szTableName;           // name of table to create.
    WCHAR               *szTemplateTableName;   // name of table from which to inherit base DDL
    unsigned long       ulPages;                // initial pages to allocate for table.
    unsigned long       ulDensity;              // table density.
    JET_COLUMNCREATE_W  *rgcolumncreate;        // array of column creation info
    unsigned long       cColumns;               // number of columns to create
    JET_INDEXCREATE_W   *rgindexcreate;         // array of index creation info
    unsigned long       cIndexes;               // number of indexes to create
    WCHAR               *szCallback;            // callback to use for this table
    JET_CBTYP           cbtyp;                  // when the callback should be called
    JET_GRBIT           grbit;
    JET_TABLEID         tableid;                // returned tableid.
    unsigned long       cCreated;               // count of objects created (columns+table+indexes+callbacks).
} JET_TABLECREATE2_W;

#ifdef JET_UNICODE
#define JET_TABLECREATE2 JET_TABLECREATE2_W
#else
#define JET_TABLECREATE2 JET_TABLECREATE2_A
#endif

#endif // JET_VERSION >= 0x0501


#if ( JET_VERSION >= 0x0601 )
typedef struct tagJET_TABLECREATE3_A
{
    unsigned long       cbStruct;               // size of this structure (for future expansion)
    char                *szTableName;           // name of table to create.
    char                *szTemplateTableName;   // name of table from which to inherit base DDL
    unsigned long       ulPages;                // initial pages to allocate for table.
    unsigned long       ulDensity;              // table density.
    JET_COLUMNCREATE_A  *rgcolumncreate;        // array of column creation info
    unsigned long       cColumns;               // number of columns to create
    JET_INDEXCREATE2_A  *rgindexcreate;         // array of index creation info
    unsigned long       cIndexes;               // number of indexes to create
    char                *szCallback;            // callback to use for this table
    JET_CBTYP           cbtyp;                  // when the callback should be called
    JET_GRBIT           grbit;
    JET_SPACEHINTS *    pSeqSpacehints;         // space allocation, maintenance, and usage hints for default sequential index 
    JET_SPACEHINTS *    pLVSpacehints;          // space allocation, maintenance, and usage hints for Separated LV tree.
    unsigned long       cbSeparateLV;           // heuristic size to separate a intrinsic LV from the primary record

    JET_TABLEID         tableid;                // returned tableid.
    unsigned long       cCreated;               // count of objects created (columns+table+indexes+callbacks).
} JET_TABLECREATE3_A;

typedef struct tagJET_TABLECREATE3_W
{
    unsigned long       cbStruct;               // size of this structure (for future expansion)
    WCHAR               *szTableName;           // name of table to create.
    WCHAR               *szTemplateTableName;   // name of table from which to inherit base DDL
    unsigned long       ulPages;                // initial pages to allocate for table.
    unsigned long       ulDensity;              // table density.
    JET_COLUMNCREATE_W  *rgcolumncreate;        // array of column creation info
    unsigned long       cColumns;               // number of columns to create
    JET_INDEXCREATE2_W  *rgindexcreate;         // array of index creation info
    unsigned long       cIndexes;               // number of indexes to create
    WCHAR               *szCallback;            // callback to use for this table
    JET_CBTYP           cbtyp;                  // when the callback should be called
    JET_GRBIT           grbit;
    JET_SPACEHINTS *    pSeqSpacehints;         // space allocation, maintenance, and usage hints for default sequential index 
    JET_SPACEHINTS *    pLVSpacehints;          // space allocation, maintenance, and usage hints for Separated LV tree.
    unsigned long       cbSeparateLV;           // heuristic size to separate a intrinsic LV from the primary record
    JET_TABLEID         tableid;                // returned tableid.
    unsigned long       cCreated;               // count of objects created (columns+table+indexes+callbacks).
} JET_TABLECREATE3_W;

#ifdef JET_UNICODE
#define JET_TABLECREATE3 JET_TABLECREATE3_W
#else
#define JET_TABLECREATE3 JET_TABLECREATE3_A
#endif

#endif // JET_VERSION >= 0x0601

#if ( JET_VERSION >= 0x0602 )
typedef struct tagJET_TABLECREATE4_A
{
    unsigned long       cbStruct;               // size of this structure (for future expansion)
    char                *szTableName;           // name of table to create.
    char                *szTemplateTableName;   // name of table from which to inherit base DDL
    unsigned long       ulPages;                // initial pages to allocate for table.
    unsigned long       ulDensity;              // table density.
    JET_COLUMNCREATE_A  *rgcolumncreate;        // array of column creation info
    unsigned long       cColumns;               // number of columns to create
    JET_INDEXCREATE3_A  *rgindexcreate;         // array of index creation info
    unsigned long       cIndexes;               // number of indexes to create
    char                *szCallback;            // callback to use for this table
    JET_CBTYP           cbtyp;                  // when the callback should be called
    JET_GRBIT           grbit;
    JET_SPACEHINTS *    pSeqSpacehints;         // space allocation, maintenance, and usage hints for default sequential index 
    JET_SPACEHINTS *    pLVSpacehints;          // space allocation, maintenance, and usage hints for Separated LV tree.
    unsigned long       cbSeparateLV;           // heuristic size to separate a intrinsic LV from the primary record

    JET_TABLEID         tableid;                // returned tableid.
    unsigned long       cCreated;               // count of objects created (columns+table+indexes+callbacks).
} JET_TABLECREATE4_A;

typedef struct tagJET_TABLECREATE4_W
{
    unsigned long       cbStruct;               // size of this structure (for future expansion)
    WCHAR               *szTableName;           // name of table to create.
    WCHAR               *szTemplateTableName;   // name of table from which to inherit base DDL
    unsigned long       ulPages;                // initial pages to allocate for table.
    unsigned long       ulDensity;              // table density.
    JET_COLUMNCREATE_W  *rgcolumncreate;        // array of column creation info
    unsigned long       cColumns;               // number of columns to create
    JET_INDEXCREATE3_W  *rgindexcreate;         // array of index creation info
    unsigned long       cIndexes;               // number of indexes to create
    WCHAR               *szCallback;            // callback to use for this table
    JET_CBTYP           cbtyp;                  // when the callback should be called
    JET_GRBIT           grbit;
    JET_SPACEHINTS *    pSeqSpacehints;         // space allocation, maintenance, and usage hints for default sequential index 
    JET_SPACEHINTS *    pLVSpacehints;          // space allocation, maintenance, and usage hints for Separated LV tree.
    unsigned long       cbSeparateLV;           // heuristic size to separate a intrinsic LV from the primary record

    JET_TABLEID         tableid;                // returned tableid.
    unsigned long       cCreated;               // count of objects created (columns+table+indexes+callbacks).
} JET_TABLECREATE4_W;

#ifdef JET_UNICODE
#define JET_TABLECREATE4 JET_TABLECREATE4_W
#else
#define JET_TABLECREATE4 JET_TABLECREATE4_A
#endif

#endif // JET_VERSION >= 0x0602

// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )
typedef struct tagJET_TABLECREATE5_A
{
    unsigned long       cbStruct;               // size of this structure (for future expansion)
    char                *szTableName;           // name of table to create.
    char                *szTemplateTableName;   // name of table from which to inherit base DDL
    unsigned long       ulPages;                // initial pages to allocate for table.
    unsigned long       ulDensity;              // table density.
    JET_COLUMNCREATE_A  *rgcolumncreate;        // array of column creation info
    unsigned long       cColumns;               // number of columns to create
    JET_INDEXCREATE3_A  *rgindexcreate;         // array of index creation info
    unsigned long       cIndexes;               // number of indexes to create
    char                *szCallback;            // callback to use for this table
    JET_CBTYP           cbtyp;                  // when the callback should be called
    JET_GRBIT           grbit;
    JET_SPACEHINTS *    pSeqSpacehints;         // space allocation, maintenance, and usage hints for default sequential index 
    JET_SPACEHINTS *    pLVSpacehints;          // space allocation, maintenance, and usage hints for Separated LV tree.
    unsigned long       cbSeparateLV;           // heuristic size to separate a intrinsic LV from the primary record
    unsigned long       cbLVChunkMax;           // Maximum chunk size to use for Separated LVs

    JET_TABLEID         tableid;                // returned tableid.
    unsigned long       cCreated;               // count of objects created (columns+table+indexes+callbacks).
} JET_TABLECREATE5_A;

typedef struct tagJET_TABLECREATE5_W
{
    unsigned long       cbStruct;               // size of this structure (for future expansion)
    WCHAR               *szTableName;           // name of table to create.
    WCHAR               *szTemplateTableName;   // name of table from which to inherit base DDL
    unsigned long       ulPages;                // initial pages to allocate for table.
    unsigned long       ulDensity;              // table density.
    JET_COLUMNCREATE_W  *rgcolumncreate;        // array of column creation info
    unsigned long       cColumns;               // number of columns to create
    JET_INDEXCREATE3_W  *rgindexcreate;         // array of index creation info
    unsigned long       cIndexes;               // number of indexes to create
    WCHAR               *szCallback;            // callback to use for this table
    JET_CBTYP           cbtyp;                  // when the callback should be called
    JET_GRBIT           grbit;
    JET_SPACEHINTS *    pSeqSpacehints;         // space allocation, maintenance, and usage hints for default sequential index 
    JET_SPACEHINTS *    pLVSpacehints;          // space allocation, maintenance, and usage hints for Separated LV tree.
    unsigned long       cbSeparateLV;           // heuristic size to separate a intrinsic LV from the primary record
    unsigned long       cbLVChunkMax;           // Maximum chunk size to use for Separated LVs

    JET_TABLEID         tableid;                // returned tableid.
    unsigned long       cCreated;               // count of objects created (columns+table+indexes+callbacks).
} JET_TABLECREATE5_W;

#ifdef JET_UNICODE
#define JET_TABLECREATE5 JET_TABLECREATE5_W
#else
#define JET_TABLECREATE5 JET_TABLECREATE5_A
#endif

#endif // JET_VERSION >= 0x0A01
// begin_PubEsent

#if ( JET_VERSION >= 0x0600 )
typedef struct tagJET_OPENTEMPORARYTABLE
{
    unsigned long       cbStruct;               // size of this structure (for future expansion)
    const JET_COLUMNDEF *prgcolumndef;
    unsigned long       ccolumn;
    JET_UNICODEINDEX    *pidxunicode;
    JET_GRBIT           grbit;
    JET_COLUMNID        *prgcolumnid;
    unsigned long       cbKeyMost;
    unsigned long       cbVarSegMac;
    JET_TABLEID         tableid;
} JET_OPENTEMPORARYTABLE;
#endif // JET_VERSION >= 0x0600

#if ( JET_VERSION >= 0x0602 )
typedef struct tagJET_OPENTEMPORARYTABLE2
{
    unsigned long       cbStruct;               // size of this structure (for future expansion)
    const JET_COLUMNDEF *prgcolumndef;
    unsigned long       ccolumn;
    JET_UNICODEINDEX2   *pidxunicode;
    JET_GRBIT           grbit;
    JET_COLUMNID        *prgcolumnid;
    unsigned long       cbKeyMost;
    unsigned long       cbVarSegMac;
    JET_TABLEID         tableid;
} JET_OPENTEMPORARYTABLE2;
#endif // JET_VERSION >= 0x0602

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
    // can be used for index (JetPrereadIndexRanges) or residual predicate (JetSetCursorFilter)
    JET_relopEquals = 0,
    JET_relopPrefixEquals,

    // can only be used for residual predicate (JetSetCursorFilter)
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
    JET_COLUMNID    columnid;   //  columnid of the column
    JET_RELOP       relop;      //  relational operator
    void *          pv;         //  pointer to the value to use
    unsigned long   cb;         //  size of the value to use
    JET_GRBIT       grbit;      //  optional grbits
} JET_INDEX_COLUMN;

typedef struct
{
    JET_INDEX_COLUMN *  rgStartColumns;
    unsigned long       cStartColumns;
    JET_INDEX_COLUMN *  rgEndColumns;
    unsigned long       cEndColumns;
} JET_INDEX_RANGE;
#endif  //  JET_VERSION >= 0x0602

// end_PubEsent
//  for database DDL conversion

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
#endif // JET_VERSION >= 0x0501

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
    char                    * szTable;                  // name of table to convert
    JET_CONDITIONALCOLUMN_A * rgconditionalcolumn;      // pointer to conditional column structure
    unsigned long           cConditionalColumn;         // number of conditional columns
} JET_DDLADDCONDITIONALCOLUMNSTOALLINDEXES_A;

typedef struct tagDDLADDCONDITIONALCOLUMNSTOALLINDEXES_W
{
    WCHAR                   * szTable;                  // name of table to convert
    JET_CONDITIONALCOLUMN_W * rgconditionalcolumn;      // pointer to conditional column structure
    unsigned long           cConditionalColumn;         // number of conditional columns
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
    char            *szIndex;       //  pass NULL to change density of primary index
    unsigned long   ulDensity;
} JET_DDLINDEXDENSITY_A;

typedef struct tagDDLINDEXDENSITY_W
{
    WCHAR           *szTable;
    WCHAR           *szIndex;       //  pass NULL to change density of primary index
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


//  The caller need to setup JET_OLP with a signal wait for the signal to be set.

typedef struct
{
    void    *pvReserved1;       // internally use
    void    *pvReserved2;
    unsigned long cbActual;     // the actual number of bytes read through this IO
    JET_HANDLE  hSig;           // a manual reset signal to wait for the IO to complete.
    JET_ERR     err;                // Err code for this assync IO.
} JET_OLP;
// begin_PubEsent


#include <pshpack1.h>
#define JET_MAX_COMPUTERNAME_LENGTH 15

typedef struct
{
    char        bSeconds;               //  0 - 59
    char        bMinutes;               //  0 - 59
    char        bHours;                 //  0 - 23
    char        bDay;                   //  1 - 31
    char        bMonth;                 //  1 - 12
    char        bYear;                  //  current year - 1900
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
// the JET_BKLOGTIME is an extention of JET_LOGTIME to be used
// in the JET_BKINFO structure. They should have the same size for
// compatibility reasons
typedef struct
{
    char        bSeconds;               //  0 - 59
    char        bMinutes;               //  0 - 59
    char        bHours;                 //  0 - 23
    char        bDay;                   //  1 - 31
    char        bMonth;                 //  1 - 12
    char        bYear;                  //  current year - 1900
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
#endif // JET_VERSION >= 0x0600

typedef struct
{
    unsigned short  ib;             // must be the last so that lgpos can
    unsigned short  isec;           // index of disksec starting logsec
    long            lGeneration;    // generation of logsec
} JET_LGPOS;                    // be casted to TIME.

typedef struct
{
    unsigned long   ulRandom;           //  a random number
    JET_LOGTIME     logtimeCreate;      //  time db created, in logtime format
    char            szComputerName[ JET_MAX_COMPUTERNAME_LENGTH + 1 ];  // where db is created
} JET_SIGNATURE;
// end_PubEsent

#if ( JET_VERSION >= 0x0600 )
typedef struct
{
    unsigned long   genMin;
    unsigned long   genMax;
    JET_LOGTIME     logtimeGenMaxCreate;
} JET_CHECKPOINTINFO;
#endif // JET_VERSION >= 0x0600
// begin_PubEsent

typedef struct
{
    JET_LGPOS       lgposMark;          //  id for this backup
    union
    {
        JET_LOGTIME     logtimeMark;
#if ( JET_VERSION >= 0x0600 )
        JET_BKLOGTIME   bklogtimeMark;
#endif // JET_VERSION >= 0x0600
    };
    unsigned long   genLow;
    unsigned long   genHigh;
} JET_BKINFO;

#include <poppack.h>

typedef struct
{
    unsigned long   ulVersion;      //  the major (incompatible) version of DAE from the last engine attach/create.
    unsigned long   ulUpdate;       //  used to track incremental database format "update (major)" version from the
                                    //  last attach/create that is a backward-compatible major update.
    JET_SIGNATURE   signDb;         //  (28 bytes) signature of the db (incl. creation time).
    unsigned long   dbstate;        //  consistent/inconsistent state

    JET_LGPOS       lgposConsistent;    //  null if in inconsistent state
    JET_LOGTIME     logtimeConsistent;  // null if in inconsistent state

    JET_LOGTIME     logtimeAttach;  //  Last attach time.
    JET_LGPOS       lgposAttach;

    JET_LOGTIME     logtimeDetach;  //  Last detach time.
    JET_LGPOS       lgposDetach;

    JET_SIGNATURE   signLog;        //  (28 bytes) log signature for this attachments

    JET_BKINFO      bkinfoFullPrev; //  Last successful full backup.

    JET_BKINFO      bkinfoIncPrev;  //  Last successful Incremental backup.
                                    //  Reset when bkinfoFullPrev is set
    JET_BKINFO      bkinfoFullCur;  //  current backup. Succeed if a
                                    //  corresponding pat file generated.
    unsigned long   fShadowingDisabled;
    unsigned long   fUpgradeDb;

    //  NT version information. This is needed to decide if an index need
    //  be recreated due to sort table changes.

    unsigned long   dwMajorVersion;     /*  OS version info                             */
    unsigned long   dwMinorVersion;
    unsigned long   dwBuildNumber;
    long            lSPNumber;

    unsigned long   cbPageSize;         //  database page size (0 = 4k pages)

} JET_DBINFOMISC;

#if ( JET_VERSION >= 0x0600 )
typedef struct
{
    unsigned long   ulVersion;      //  the major (incompatible) version of DAE from the last engine attach/create.
    unsigned long   ulUpdate;       //  used to track incremental database format "update (major)" version from the
                                    //  last attach/create that is a backward-compatible major update.
    JET_SIGNATURE   signDb;         //  (28 bytes) signature of the db (incl. creation time).
    unsigned long   dbstate;        //  consistent/inconsistent state

    JET_LGPOS       lgposConsistent;    //  null if in inconsistent state
    JET_LOGTIME     logtimeConsistent;  // null if in inconsistent state

    JET_LOGTIME     logtimeAttach;  //  Last attach time.
    JET_LGPOS       lgposAttach;

    JET_LOGTIME     logtimeDetach;  //  Last detach time.
    JET_LGPOS       lgposDetach;

    JET_SIGNATURE   signLog;        //  (28 bytes) log signature for this attachments

    JET_BKINFO      bkinfoFullPrev; //  Last successful full backup.

    JET_BKINFO      bkinfoIncPrev;  //  Last successful Incremental backup.
                                    //  Reset when bkinfoFullPrev is set
    JET_BKINFO      bkinfoFullCur;  //  current backup. Succeed if a
                                    //  corresponding pat file generated.
    unsigned long   fShadowingDisabled;
    unsigned long   fUpgradeDb;

    //  NT version information. This is needed to decide if an index need
    //  be recreated due to sort table changes.

    unsigned long   dwMajorVersion;     /*  OS version info                             */
    unsigned long   dwMinorVersion;
    unsigned long   dwBuildNumber;
    long            lSPNumber;

    unsigned long   cbPageSize;         //  database page size (0 = 4k pages)

    // new fields added on top of the above JET_DBINFOMISC
    unsigned long   genMinRequired;         //  the minimum log generation required for replaying the logs. Typically the checkpoint generation
    unsigned long   genMaxRequired;         //  the maximum log generation required for replaying the logs.
    JET_LOGTIME     logtimeGenMaxCreate;    //  creation time of the genMax log file

    unsigned long   ulRepairCount;          //  number of times repair has been called on this database
    JET_LOGTIME     logtimeRepair;          //  the date of the last time that repair was run
    unsigned long   ulRepairCountOld;       //  number of times ErrREPAIRAttachForRepair has been called on this database before the last defrag

    unsigned long   ulECCFixSuccess;        //  number of times a one bit error was fixed and resulted in a good page
    JET_LOGTIME     logtimeECCFixSuccess;   //  the date of the last time that a one bit error was fixed and resulted in a good page
    unsigned long   ulECCFixSuccessOld;     //  number of times a one bit error was fixed and resulted in a good page before last repair

    unsigned long   ulECCFixFail;           //  number of times a one bit error was fixed and resulted in a bad page
    JET_LOGTIME     logtimeECCFixFail;      //  the date of the last time that a one bit error was fixed and resulted in a bad page
    unsigned long   ulECCFixFailOld;        //  number of times a one bit error was fixed and resulted in a bad page before last repair

    unsigned long   ulBadChecksum;          //  number of times a non-correctable ECC/checksum error was found
    JET_LOGTIME     logtimeBadChecksum;     //  the date of the last time that a non-correctable ECC/checksum error was found
    unsigned long   ulBadChecksumOld;       //  number of times a non-correctable ECC/checksum error was found before last repair

} JET_DBINFOMISC2;
#endif // JET_VERSION >= 0x0600

#if ( JET_VERSION >= 0x0601 )
typedef struct
{
    unsigned long   ulVersion;      //  the major (incompatible) version of DAE from the last engine attach/create.
    unsigned long   ulUpdate;       //  used to track incremental database format "update (major)" version from the
                                    //  last attach/create that is a backward-compatible major update.
    JET_SIGNATURE   signDb;         //  (28 bytes) signature of the db (incl. creation time).
    unsigned long   dbstate;        //  consistent/inconsistent state

    JET_LGPOS       lgposConsistent;    //  null if in inconsistent state
    JET_LOGTIME     logtimeConsistent;  // null if in inconsistent state

    JET_LOGTIME     logtimeAttach;  //  Last attach time.
    JET_LGPOS       lgposAttach;

    JET_LOGTIME     logtimeDetach;  //  Last detach time.
    JET_LGPOS       lgposDetach;

    JET_SIGNATURE   signLog;        //  (28 bytes) log signature for this attachments

    JET_BKINFO      bkinfoFullPrev; //  Last successful full backup.

    JET_BKINFO      bkinfoIncPrev;  //  Last successful Incremental backup.
                                    //  Reset when bkinfoFullPrev is set
    JET_BKINFO      bkinfoFullCur;  //  current backup. Succeed if a
                                    //  corresponding pat file generated.
    unsigned long   fShadowingDisabled;
    unsigned long   fUpgradeDb;

    //  NT version information. This is needed to decide if an index need
    //  be recreated due to sort table changes.

    unsigned long   dwMajorVersion;     /*  OS version info                             */
    unsigned long   dwMinorVersion;
    unsigned long   dwBuildNumber;
    long            lSPNumber;

    unsigned long   cbPageSize;         //  database page size (0 = 4k pages)

    // new fields added on top of the above JET_DBINFOMISC
    unsigned long   genMinRequired;         //  the minimum log generation required for replaying the logs. Typically the checkpoint generation
    unsigned long   genMaxRequired;         //  the maximum log generation required for replaying the logs.
    JET_LOGTIME     logtimeGenMaxCreate;    //  creation time of the genMax log file

    unsigned long   ulRepairCount;          //  number of times repair has been called on this database
    JET_LOGTIME     logtimeRepair;          //  the date of the last time that repair was run
    unsigned long   ulRepairCountOld;       //  number of times ErrREPAIRAttachForRepair has been called on this database before the last defrag

    unsigned long   ulECCFixSuccess;        //  number of times a one bit error was fixed and resulted in a good page
    JET_LOGTIME     logtimeECCFixSuccess;   //  the date of the last time that a one bit error was fixed and resulted in a good page
    unsigned long   ulECCFixSuccessOld;     //  number of times a one bit error was fixed and resulted in a good page before last repair

    unsigned long   ulECCFixFail;           //  number of times a one bit error was fixed and resulted in a bad page
    JET_LOGTIME     logtimeECCFixFail;      //  the date of the last time that a one bit error was fixed and resulted in a bad page
    unsigned long   ulECCFixFailOld;        //  number of times a one bit error was fixed and resulted in a bad page before last repair

    unsigned long   ulBadChecksum;          //  number of times a non-correctable ECC/checksum error was found
    JET_LOGTIME     logtimeBadChecksum;     //  the date of the last time that a non-correctable ECC/checksum error was found
    unsigned long   ulBadChecksumOld;       //  number of times a non-correctable ECC/checksum error was found before last repair

    // new fields added on top of the above JET_DBINFOMISC2
    unsigned long   genCommitted;           //  the maximum log generation committed to the database. Typically the current log generation

} JET_DBINFOMISC3;

typedef struct
{
    unsigned long   ulVersion;      //  the major (incompatible) version of DAE from the last engine attach/create.
    unsigned long   ulUpdate;       //  used to track incremental database format "update (major)" version from the
                                    //  last attach/create that is a backward-compatible major update.
    JET_SIGNATURE   signDb;         //  (28 bytes) signature of the db (incl. creation time).
    unsigned long   dbstate;        //  consistent/inconsistent state

    JET_LGPOS       lgposConsistent;    //  null if in inconsistent state
    JET_LOGTIME     logtimeConsistent;  // null if in inconsistent state

    JET_LOGTIME     logtimeAttach;  //  Last attach time.
    JET_LGPOS       lgposAttach;

    JET_LOGTIME     logtimeDetach;  //  Last detach time.
    JET_LGPOS       lgposDetach;

    JET_SIGNATURE   signLog;        //  (28 bytes) log signature for this attachments

    JET_BKINFO      bkinfoFullPrev; //  Last successful full backup.

    JET_BKINFO      bkinfoIncPrev;  //  Last successful Incremental backup.
                                    //  Reset when bkinfoFullPrev is set
    JET_BKINFO      bkinfoFullCur;  //  current backup. Succeed if a
                                    //  corresponding pat file generated.
    unsigned long   fShadowingDisabled;
    unsigned long   fUpgradeDb;

    //  NT version information. This is needed to decide if an index need
    //  be recreated due to sort table changes.

    unsigned long   dwMajorVersion;     /*  OS version info                             */
    unsigned long   dwMinorVersion;
    unsigned long   dwBuildNumber;
    long            lSPNumber;

    unsigned long   cbPageSize;         //  database page size (0 = 4k pages)

    // new fields added on top of the above JET_DBINFOMISC
    unsigned long   genMinRequired;         //  the minimum log generation required for replaying the logs. Typically the checkpoint generation
    unsigned long   genMaxRequired;         //  the maximum log generation required for replaying the logs.
    JET_LOGTIME     logtimeGenMaxCreate;    //  creation time of the genMax log file

    unsigned long   ulRepairCount;          //  number of times repair has been called on this database
    JET_LOGTIME     logtimeRepair;          //  the date of the last time that repair was run
    unsigned long   ulRepairCountOld;       //  number of times ErrREPAIRAttachForRepair has been called on this database before the last defrag

    unsigned long   ulECCFixSuccess;        //  number of times a one bit error was fixed and resulted in a good page
    JET_LOGTIME     logtimeECCFixSuccess;   //  the date of the last time that a one bit error was fixed and resulted in a good page
    unsigned long   ulECCFixSuccessOld;     //  number of times a one bit error was fixed and resulted in a good page before last repair

    unsigned long   ulECCFixFail;           //  number of times a one bit error was fixed and resulted in a bad page
    JET_LOGTIME     logtimeECCFixFail;      //  the date of the last time that a one bit error was fixed and resulted in a bad page
    unsigned long   ulECCFixFailOld;        //  number of times a one bit error was fixed and resulted in a bad page before last repair

    unsigned long   ulBadChecksum;          //  number of times a non-correctable ECC/checksum error was found
    JET_LOGTIME     logtimeBadChecksum;     //  the date of the last time that a non-correctable ECC/checksum error was found
    unsigned long   ulBadChecksumOld;       //  number of times a non-correctable ECC/checksum error was found before last repair

    // new fields added on top of the above JET_DBINFOMISC2
    unsigned long   genCommitted;           //  the maximum log generation committed to the database. Typically the current log generation

    // new fields added on top of the above JET_DBINFOMISC3
    JET_BKINFO  bkinfoCopyPrev;         //  Last successful Copy backup
    JET_BKINFO  bkinfoDiffPrev;         //  Last successful Differential backup, reset when bkinfoFullPrev is set
} JET_DBINFOMISC4;
#endif // JET_VERSION >= 0x0601
// end_PubEsent

#if ( JET_VERSION >= 0x0601 )
typedef struct
{
    unsigned long   ulVersion;      //  the major (incompatible) version of DAE from the last engine attach/create.
    unsigned long   ulUpdate;       //  used to track incremental database format "update (major)" version from the
                                    //  last attach/create that is a backward-compatible major update.
    JET_SIGNATURE   signDb;         //  (28 bytes) signature of the db (incl. creation time).
    unsigned long   dbstate;        //  consistent/inconsistent state

    JET_LGPOS       lgposConsistent;    //  null if in inconsistent state
    JET_LOGTIME     logtimeConsistent;  // null if in inconsistent state

    JET_LOGTIME     logtimeAttach;  //  Last attach time.
    JET_LGPOS       lgposAttach;

    JET_LOGTIME     logtimeDetach;  //  Last detach time.
    JET_LGPOS       lgposDetach;

    JET_SIGNATURE   signLog;        //  (28 bytes) log signature for this attachments

    JET_BKINFO      bkinfoFullPrev; //  Last successful full backup.

    JET_BKINFO      bkinfoIncPrev;  //  Last successful Incremental backup.
                                    //  Reset when bkinfoFullPrev is set
    JET_BKINFO      bkinfoFullCur;  //  current backup. Succeed if a
                                    //  corresponding pat file generated.
    unsigned long   fShadowingDisabled;
    unsigned long   fUpgradeDb;

    //  NT version information. This is needed to decide if an index need
    //  be recreated due to sort table changes.

    unsigned long   dwMajorVersion;     /*  OS version info                             */
    unsigned long   dwMinorVersion;
    unsigned long   dwBuildNumber;
    long            lSPNumber;

    unsigned long   cbPageSize;         //  database page size (0 = 4k pages)

    // new fields added on top of the above JET_DBINFOMISC
    unsigned long   genMinRequired;         //  the minimum log generation required for replaying the logs. Typically the checkpoint generation
    unsigned long   genMaxRequired;         //  the maximum log generation required for replaying the logs.
    JET_LOGTIME     logtimeGenMaxCreate;    //  creation time of the genMax log file

    unsigned long   ulRepairCount;          //  number of times repair has been called on this database
    JET_LOGTIME     logtimeRepair;          //  the date of the last time that repair was run
    unsigned long   ulRepairCountOld;       //  number of times ErrREPAIRAttachForRepair has been called on this database before the last defrag

    unsigned long   ulECCFixSuccess;        //  number of times a one bit error was fixed and resulted in a good page
    JET_LOGTIME     logtimeECCFixSuccess;   //  the date of the last time that a one bit error was fixed and resulted in a good page
    unsigned long   ulECCFixSuccessOld;     //  number of times a one bit error was fixed and resulted in a good page before last repair

    unsigned long   ulECCFixFail;           //  number of times a one bit error was fixed and resulted in a bad page
    JET_LOGTIME     logtimeECCFixFail;      //  the date of the last time that a one bit error was fixed and resulted in a bad page
    unsigned long   ulECCFixFailOld;        //  number of times a one bit error was fixed and resulted in a bad page before last repair

    unsigned long   ulBadChecksum;          //  number of times a non-correctable ECC/checksum error was found
    JET_LOGTIME     logtimeBadChecksum;     //  the date of the last time that a non-correctable ECC/checksum error was found
    unsigned long   ulBadChecksumOld;       //  number of times a non-correctable ECC/checksum error was found before last repair

    // new fields added on top of the above JET_DBINFOMISC2
    unsigned long   genCommitted;           //  the maximum log generation committed to the database. Typically the current log generation

    // new fields added on top of the above JET_DBINFOMISC3
    JET_BKINFO  bkinfoCopyPrev;         //  Last successful Copy backup
    JET_BKINFO  bkinfoDiffPrev;         //  Last successful Differential backup, reset when bkinfoFullPrev is set

    // new fields added on top of the above JET_DBINFOMISC4
    unsigned long   ulIncrementalReseedCount;       //  number of times incremental reseed has been initiated on this database
    JET_LOGTIME     logtimeIncrementalReseed;       //  the date of the last time that incremental reseed was initiated on this database
    unsigned long   ulIncrementalReseedCountOld;    //  number of times incremental reseed was initiated on this database before the last defrag

    unsigned long   ulPagePatchCount;               //  number of pages patched in the database as a part of incremental reseed
    JET_LOGTIME     logtimePagePatch;               //  the date of the last time that a page was patched as a part of incremental reseed
    unsigned long   ulPagePatchCountOld;            //  number of pages patched in the database as a part of incremental reseed before the last defrag
} JET_DBINFOMISC5;

typedef struct
{
    unsigned long   ulVersion;      //  the major (incompatible) version of DAE from the last engine attach/create.
    unsigned long   ulUpdate;       //  used to track incremental database format "update (major)" version from the
                                    //  last attach/create that is a backward-compatible major update.
    JET_SIGNATURE   signDb;         //  (28 bytes) signature of the db (incl. creation time).
    unsigned long   dbstate;        //  consistent/inconsistent state

    JET_LGPOS       lgposConsistent;    //  null if in inconsistent state
    JET_LOGTIME     logtimeConsistent;  // null if in inconsistent state

    JET_LOGTIME     logtimeAttach;  //  Last attach time.
    JET_LGPOS       lgposAttach;

    JET_LOGTIME     logtimeDetach;  //  Last detach time.
    JET_LGPOS       lgposDetach;

    JET_SIGNATURE   signLog;        //  (28 bytes) log signature for this attachments

    JET_BKINFO      bkinfoFullPrev; //  Last successful full backup.

    JET_BKINFO      bkinfoIncPrev;  //  Last successful Incremental backup.
                                    //  Reset when bkinfoFullPrev is set
    JET_BKINFO      bkinfoFullCur;  //  current backup. Succeed if a
                                    //  corresponding pat file generated.
    unsigned long   fShadowingDisabled;
    unsigned long   fUpgradeDb;

    //  NT version information. This is needed to decide if an index need
    //  be recreated due to sort table changes.

    unsigned long   dwMajorVersion;     /*  OS version info                             */
    unsigned long   dwMinorVersion;
    unsigned long   dwBuildNumber;
    long            lSPNumber;

    unsigned long   cbPageSize;         //  database page size (0 = 4k pages)

    // new fields added on top of the above JET_DBINFOMISC
    unsigned long   genMinRequired;         //  the minimum log generation required for replaying the logs. Typically the checkpoint generation
    unsigned long   genMaxRequired;         //  the maximum log generation required for replaying the logs.
    JET_LOGTIME     logtimeGenMaxCreate;    //  creation time of the genMax log file

    unsigned long   ulRepairCount;          //  number of times repair has been called on this database
    JET_LOGTIME     logtimeRepair;          //  the date of the last time that repair was run
    unsigned long   ulRepairCountOld;       //  number of times ErrREPAIRAttachForRepair has been called on this database before the last defrag

    unsigned long   ulECCFixSuccess;        //  number of times a one bit error was fixed and resulted in a good page
    JET_LOGTIME     logtimeECCFixSuccess;   //  the date of the last time that a one bit error was fixed and resulted in a good page
    unsigned long   ulECCFixSuccessOld;     //  number of times a one bit error was fixed and resulted in a good page before last repair

    unsigned long   ulECCFixFail;           //  number of times a one bit error was fixed and resulted in a bad page
    JET_LOGTIME     logtimeECCFixFail;      //  the date of the last time that a one bit error was fixed and resulted in a bad page
    unsigned long   ulECCFixFailOld;        //  number of times a one bit error was fixed and resulted in a bad page before last repair

    unsigned long   ulBadChecksum;          //  number of times a non-correctable ECC/checksum error was found
    JET_LOGTIME     logtimeBadChecksum;     //  the date of the last time that a non-correctable ECC/checksum error was found
    unsigned long   ulBadChecksumOld;       //  number of times a non-correctable ECC/checksum error was found before last repair

    // new fields added on top of the above JET_DBINFOMISC2
    unsigned long   genCommitted;           //  the maximum log generation committed to the database. Typically the current log generation

    // new fields added on top of the above JET_DBINFOMISC3
    JET_BKINFO  bkinfoCopyPrev;         //  Last successful Copy backup
    JET_BKINFO  bkinfoDiffPrev;         //  Last successful Differential backup, reset when bkinfoFullPrev is set

    // new fields added on top of the above JET_DBINFOMISC4
    unsigned long   ulIncrementalReseedCount;       //  number of times incremental reseed has been initiated on this database
    JET_LOGTIME     logtimeIncrementalReseed;       //  the date of the last time that incremental reseed was initiated on this database
    unsigned long   ulIncrementalReseedCountOld;    //  number of times incremental reseed was initiated on this database before the last defrag

    unsigned long   ulPagePatchCount;               //  number of pages patched in the database as a part of incremental reseed
    JET_LOGTIME     logtimePagePatch;               //  the date of the last time that a page was patched as a part of incremental reseed
    unsigned long   ulPagePatchCountOld;            //  number of pages patched in the database as a part of incremental reseed before the last defrag

    // new fields added on top of the above JET_DBINFOMISC5
    JET_LOGTIME logtimeChecksumPrev;    // last checksum pass finish time (UTC - 1900y)
    JET_LOGTIME logtimeChecksumStart;   // current checksum pass start time (UTC - 1900y)
    unsigned long cpgDatabaseChecked;   // # of page checked for current pass
} JET_DBINFOMISC6;
#endif // JET_VERSION >= 0x0601

#if ( JET_VERSION >= 0x0A00 )
typedef struct
{
    unsigned long   ulVersion;      //  the major (incompatible) version of DAE from the last engine attach/create.
    unsigned long   ulUpdate;       //  used to track incremental database format "update (major)" version from the
                                    //  last attach/create that is a backward-compatible major update.
    JET_SIGNATURE   signDb;         //  (28 bytes) signature of the db (incl. creation time).
    unsigned long   dbstate;        //  consistent/inconsistent state

    JET_LGPOS       lgposConsistent;    //  null if in inconsistent state
    JET_LOGTIME     logtimeConsistent;  // null if in inconsistent state

    JET_LOGTIME     logtimeAttach;  //  Last attach time.
    JET_LGPOS       lgposAttach;

    JET_LOGTIME     logtimeDetach;  //  Last detach time.
    JET_LGPOS       lgposDetach;

    JET_SIGNATURE   signLog;        //  (28 bytes) log signature for this attachments

    JET_BKINFO      bkinfoFullPrev; //  Last successful full backup.

    JET_BKINFO      bkinfoIncPrev;  //  Last successful Incremental backup.
                                    //  Reset when bkinfoFullPrev is set
    JET_BKINFO      bkinfoFullCur;  //  current backup. Succeed if a
                                    //  corresponding pat file generated.
    unsigned long   fShadowingDisabled;
    unsigned long   fUpgradeDb;

    //  NT version information. This is needed to decide if an index need
    //  be recreated due to sort table changes.

    unsigned long   dwMajorVersion;     /*  OS version info                             */
    unsigned long   dwMinorVersion;
    unsigned long   dwBuildNumber;
    long            lSPNumber;

    unsigned long   cbPageSize;         //  database page size (0 = 4k pages)

    // new fields added on top of the above JET_DBINFOMISC
    unsigned long   genMinRequired;         //  the minimum log generation required for replaying the logs. Typically the checkpoint generation
    unsigned long   genMaxRequired;         //  the maximum log generation required for replaying the logs.
    JET_LOGTIME     logtimeGenMaxCreate;    //  creation time of the genMax log file

    unsigned long   ulRepairCount;          //  number of times repair has been called on this database
    JET_LOGTIME     logtimeRepair;          //  the date of the last time that repair was run
    unsigned long   ulRepairCountOld;       //  number of times ErrREPAIRAttachForRepair has been called on this database before the last defrag

    unsigned long   ulECCFixSuccess;        //  number of times a one bit error was fixed and resulted in a good page
    JET_LOGTIME     logtimeECCFixSuccess;   //  the date of the last time that a one bit error was fixed and resulted in a good page
    unsigned long   ulECCFixSuccessOld;     //  number of times a one bit error was fixed and resulted in a good page before last repair

    unsigned long   ulECCFixFail;           //  number of times a one bit error was fixed and resulted in a bad page
    JET_LOGTIME     logtimeECCFixFail;      //  the date of the last time that a one bit error was fixed and resulted in a bad page
    unsigned long   ulECCFixFailOld;        //  number of times a one bit error was fixed and resulted in a bad page before last repair

    unsigned long   ulBadChecksum;          //  number of times a non-correctable ECC/checksum error was found
    JET_LOGTIME     logtimeBadChecksum;     //  the date of the last time that a non-correctable ECC/checksum error was found
    unsigned long   ulBadChecksumOld;       //  number of times a non-correctable ECC/checksum error was found before last repair

    // new fields added on top of the above JET_DBINFOMISC2
    unsigned long   genCommitted;           //  the maximum log generation committed to the database. Typically the current log generation

    // new fields added on top of the above JET_DBINFOMISC3
    JET_BKINFO  bkinfoCopyPrev;         //  Last successful Copy backup
    JET_BKINFO  bkinfoDiffPrev;         //  Last successful Differential backup, reset when bkinfoFullPrev is set

    // new fields added on top of the above JET_DBINFOMISC4
    unsigned long   ulIncrementalReseedCount;       //  number of times incremental reseed has been initiated on this database
    JET_LOGTIME     logtimeIncrementalReseed;       //  the date of the last time that incremental reseed was initiated on this database
    unsigned long   ulIncrementalReseedCountOld;    //  number of times incremental reseed was initiated on this database before the last defrag

    unsigned long   ulPagePatchCount;               //  number of pages patched in the database as a part of incremental reseed
    JET_LOGTIME     logtimePagePatch;               //  the date of the last time that a page was patched as a part of incremental reseed
    unsigned long   ulPagePatchCountOld;            //  number of pages patched in the database as a part of incremental reseed before the last defrag

    // new fields added on top of the above JET_DBINFOMISC5
    JET_LOGTIME logtimeChecksumPrev;    // last checksum pass finish time (UTC - 1900y)
    JET_LOGTIME logtimeChecksumStart;   // current checksum pass start time (UTC - 1900y)
    unsigned long cpgDatabaseChecked;   // # of page checked for current pass

    // new fields added on top of the above JET_DBINFOMISC6
    JET_LOGTIME     logtimeLastReAttach;    //  Last attach time.
    JET_LGPOS       lgposLastReAttach;
} JET_DBINFOMISC7;
#endif // JET_VERSION >= 0x0A00

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

    unsigned long   ulVersionMinorDeprecated;       //  deprecated

    unsigned __int64    checksumPrevLogAllSegments;
    
} JET_LOGINFOMISC3;

#endif

// We do not attempt to maintain version compatibility with private data structures.
// People using the private version of the header shouldn't need to target
// down-level anyay.
#if ( JET_VERSION >= 0x0A00 )

//
//  This is the list of callback sequences you could get when using JetInit4() 
//  with JET_bitExternalRecoveryControl
//  Note: For all such sequences JET_snpRecoveryControl will be the JET_SNP value.
//
//  In an empty log directory:
//      
//      JET_sntOpenLog, JET_OpenLogForRecoveryCheckingAndPatching, fCurrent         // pre-check
//      JET_sntMissingLog, JET_MissingLogContinueToRedo
//      JET_sntMissingLog, JET_MissingLogCreateNewLogStream                         JET_bitLogStreamMustExist -> JET_errMissingLogFile
//
//  No edb.log, no edbtmp.log, X archive logs ... the "external log provider" case.
//
//      JET_sntOpenLog, JET_OpenLogForRecoveryCheckingAndPatching, fCurrent         // pre-check
//      JET_sntOpenLog, JET_OpenLogForRecoveryCheckingAndPatching, lGenHigh         // we will close this fairly soon
//      JET_sntMissingLog, JET_MissingLogContinueToRedo
//      JET_sntOpenLog, JET_OpenLogForRedo, lGenLow/Checkpoint
//      JET_sntOpenLog, JET_OpenLogForRedo, lGenLow+1
//      JET_sntOpenLog, JET_OpenLogForRedo, lGenLow+2
//      JET_sntOpenLog, JET_OpenLogForRedo, lGenLow+3
//      JET_sntOpenLog, JET_OpenLogForRedo, lGenLow+4/lGenHighest-2
//      JET_sntOpenLog, JET_OpenLogForRedo, lGenHighest-1
//      JET_sntOpenLog, JET_OpenLogForRedo, lGenHighest
//      JET_sntOpenLog, JET_OpenLogForRedo, lGenHighest+1
//      JET_sntMissingLog, JET_MissingLogContinueTryCurrentLog (possibly x2)
//      JET_sntOpenLog, JET_OpenLogForRedo, fCurrent
//      JET_sntMissingLog, JET_MissingLogContinueToUndo
//      JET_sntBeginUndo
//      JET_sntOpenLog, JET_OpenLogForUndo
//
//  Once a decision has been made to either fail out or succeed (and continue) from 
//  the JET_sntBeginUndo it is irrevocable.  The logs may be changed from that point
//  on.  If you've decided to fail JET_sntBeginUndo with JET_errRecoveredWithoutUndo
//  then you should also fail JET_sntOpenLog+JET_OpenLogForUndo with the other
//  JET_errRecoveredWithoutUndoDatabasesConsistent error for recovery without undo.
//  Or another way ... once you've decided to fail out "without undo", then you 
//  should both return to JET_sntBeginUndo with JET_errRecoveredWithoutUndo and you
//  should also fail JET_sntOpenLog+JET_OpenLogForUndo with the other 
//  JET_errRecoveredWithoutUndoDatabasesConsistent error for consistent results.
//

#define JET_OpenLogForRecoveryCheckingAndPatching       1   /* Indicates the log file is being opened for the purposes of checking if recovery is necessary and/or patching the shadow sector. */
#define JET_OpenLogForRedo                  2   /* Indicates the log file is being opened for purposes of performing recovery redo / log replay. */
#define JET_OpenLogForUndo                  3   /* Indicates the log file is being opened for purposes of performing recovery undo. */
#define JET_OpenLogForDo                    4   /* Indicates the log file is being opened for purposes of performing do operations. */

#define JET_MissingLogMustFail              1   /* Indicates that no continuation is possible for this error. */
#define JET_MissingLogContinueToRedo        2   /* Indicates that continuing with success will tell recovery to proceed to recovery redo. */
#define JET_MissingLogContinueTryCurrentLog 3   /* Indicates that continuing with success will tell recovery to proceed to try the current / edb.jtx|log log file. */
#define JET_MissingLogContinueToUndo        4   /* Indicates that continuing with success will tell recovery to proceed to recovery undo. */
#define JET_MissingLogCreateNewLogStream    5   /* Indicates that continuing with success will tell recovery to proceed to create a new log stream. */

typedef struct
{
    unsigned long       cbStruct;       /* size of this structure */
    JET_ERR             errDefault;     /* given no desired special treatment, the client should return this */
    JET_INSTANCE        instance;       /* the instance for which recovery is run */

    JET_SNT             sntUnion;       /* indicates the type for the union */

    union {

        //  JET_sntOpenLog
        struct
        {
            unsigned long       cbStruct;       /* size of this structure */
            unsigned long       lGenNext;       /* next log to be replayed */
            unsigned char       fCurrentLog:1;  /* 0 if log with full / archive name */
            unsigned char       eReason;        /* the open disposition or reason - JET_OpenLog* */
            unsigned char       rgbReserved[6]; /* will be 0 */
            WCHAR *             wszLogFile;     /* full path of the log file we will open */
            unsigned long       cdbinfomisc;    /* number of database headers */
            JET_DBINFOMISC7 *   rgdbinfomisc;   /* array of database headers for attached databases */
        } OpenLog;

        //  JET_sntOpenCheckpoint
        struct
        {
            unsigned long       cbStruct;       /* size of this structure */
            WCHAR *             wszCheckpoint;  /* full path of the checkpoint file we will open */
        } OpenCheckpoint;

        //  JET_sntOpenDatabase not yet implemented.

        //  JET_sntMissingLog
        struct
        {
            unsigned long       cbStruct;       /* size of this structure */
            unsigned long       lGenMissing;    /* next log to be replayed */
            unsigned char       fCurrentLog:1;  /* 0 if log with full / archive name */
            unsigned char       eNextAction;    /* if success is returned, what action will we take */
            unsigned char       rgbReserved[6]; /* will be 0 */
            WCHAR *             wszLogFile;     /* full path of the log file we will open */
            unsigned long       cdbinfomisc;    /* number of database headers */
            JET_DBINFOMISC7 *   rgdbinfomisc;   /* array of database headers for attached databases */
        } MissingLog;

        //  JET_sntBeginUndo
        struct
        {
            unsigned long       cbStruct;       /* size of this structure */
            unsigned long       cdbinfomisc;    /* number of database headers */
            JET_DBINFOMISC7 *   rgdbinfomisc;   /* array of database headers for attached databases */
        } BeginUndo;

        //  JET_sntNotificationEvent
        struct
        {
            unsigned long       cbStruct;       /* size of this structure */
            unsigned long       EventID;        /* ID of the event we would publish */
        } NotificationEvent;

        //  JET_sntSignalErrorCondition
        struct
        {
            unsigned long       cbStruct;       /* size of this structure */
            //  no extra info beyond errDefault above
        } SignalErrorCondition;

        //  JET_sntAttachedDb
        struct
        {
            unsigned long       cbStruct;       /* size of this structure */
            const WCHAR *       wszDbPath;      /* full path of the database file */
        } AttachedDb;

        //  JET_sntDetachingDb
        struct
        {
            unsigned long       cbStruct;       /* size of this structure */
            const WCHAR *       wszDbPath;      /* full path of the database file */
        } DetachingDb;

        //  JET_sntCommitCtx
        struct
        {
            unsigned long       cbStruct;       /* size of this structure */
            const void *        pbCommitCtx;    /* commit context */
            unsigned long       cbCommitCtx;    /* size of commit context */
            unsigned long       fCallbackType;  /* type of callback */
        } CommitCtx;
    };
} JET_RECOVERYCONTROL;

#define fCommitCtxLegacyCommitCallback  1
#define fCommitCtxPreCommitCallback     2
#define fCommitCtxPostCommitCallback    3
#endif // JET_VERSION >= 0x0A00

#if ( JET_VERSION >= 0x0600 )
typedef struct              /* Status Notification Message */
{
    unsigned long   cbStruct;   /* Size of this structure */
    JET_SNC         snc;        /* Status Notification Code */
    unsigned long   ul;         /* Numeric identifier */
    char            sz[256];    /* Identifier */
} JET_SNMSG;
#endif // JET_VERSION >= 0x0600

// Introduced in Win7, but format changed post Win10.
#if ( JET_VERSION >= 0x0A01 )

typedef struct              // Status Notification Page Patch Request
{
    unsigned long   cbStruct;       // Size of this structure
    unsigned long   pageNumber;     // Page being patched
    const WCHAR *   szLogFile;      // Full path of the current logfile
    JET_INSTANCE    instance;       // Instance that is running recovery
    JET_DBINFOMISC7 dbinfomisc;     // Database header for the database being patched
    const void *    pvToken;        // Patch token
    unsigned long   cbToken;        // Size of the patch token
    const void *    pvData;         // Patch data (the database page)
    unsigned long   cbData;         // Size of the patch data
    JET_DBID        dbid;           // JET_DBID of database being patched
} JET_SNPATCHREQUEST;

typedef struct              // Status Notification Corrupted Page
{
    unsigned long   cbStruct;       // Size of this structure
    const WCHAR *   wszDatabase;    // File name of the database corrupted
    JET_DBID        dbid;           // JET_DBID of database corrupted
    JET_DBINFOMISC7 dbinfomisc;     // Database header for corrupted database
    unsigned long   pageNumber;     // That is corrupted
} JET_SNCORRUPTEDPAGE;
#endif // JET_VERSION >= 0x0A01

typedef struct
{
    unsigned long   cpageOwned;     //  number of owned pages in the streaming file
    unsigned long   cpageAvail;     //  number of available pages in the streaming file (subset of cpageOwned)
} JET_STREAMINGFILESPACEINFO;

// begin_PubEsent

#if ( JET_VERSION >= 0x0600 )
//  JET performance counters accumulated by thread
//
struct JET_THREADSTATS
{
    unsigned long   cbStruct;           //  size of this struct
    unsigned long   cPageReferenced;    //  pages referenced
    unsigned long   cPageRead;          //  pages read from disk
    unsigned long   cPagePreread;       //  pages preread from disk
    unsigned long   cPageDirtied;       //  clean pages modified
    unsigned long   cPageRedirtied;     //  dirty pages modified
    unsigned long   cLogRecord;         //  log records generated
    unsigned long   cbLogRecord;        //  log record bytes generated
};
#endif // JET_VERSION >= 0x0600

#if ( JET_VERSION >= 0x0A00 )
//  JET performance counters accumulated by thread
//
struct JET_THREADSTATS2
{
    unsigned long       cbStruct;               //  size of this struct
    unsigned long       cPageReferenced;        //  pages referenced
    unsigned long       cPageRead;              //  pages read from disk
    unsigned long       cPagePreread;           //  pages preread from disk
    unsigned long       cPageDirtied;           //  clean pages modified
    unsigned long       cPageRedirtied;         //  dirty pages modified
    unsigned long       cLogRecord;             //  log records generated
    unsigned long       cbLogRecord;            //  log record bytes generated
    unsigned __int64    cusecPageCacheMiss;     //  page cache miss latency in microseconds
    unsigned long       cPageCacheMiss;         //  page cache misses
};
#endif // JET_VERSION >= 0x0A00

#if ( JET_VERSION >= 0x0A01 )
//  JET performance counters accumulated by thread
//
struct JET_THREADSTATS3
{
    unsigned long       cbStruct;                       //  size of this struct
    unsigned long       cPageReferenced;                //  pages referenced
    unsigned long       cPageRead;                      //  pages read from disk
    unsigned long       cPagePreread;                   //  pages preread from disk
    unsigned long       cPageDirtied;                   //  clean pages modified
    unsigned long       cPageRedirtied;                 //  dirty pages modified
    unsigned long       cLogRecord;                     //  log records generated
    unsigned long       cbLogRecord;                    //  log record bytes generated
    unsigned __int64    cusecPageCacheMiss;             //  page cache miss latency in microseconds
    unsigned long       cPageCacheMiss;                 //  page cache misses
    unsigned long       cSeparatedLongValueRead;        //  separated LV reads
    unsigned __int64    cusecLongValuePageCacheMiss;    //  page cache miss latency in microseconds while reading separated LV data
    unsigned long       cLongValuePageCacheMiss;        //  page cache misses while reading separated LV data
};
#endif // JET_VERSION >= 0x0A01
// end_PubEsent

#if ( JET_VERSION >= 0x0A01 )
//  JET performance counters accumulated by thread
//
struct JET_THREADSTATS4
{
    unsigned long       cbStruct;                           //  size of this struct
    unsigned long       cPageReferenced;                    //  pages referenced
    unsigned long       cPageRead;                          //  pages read from disk
    unsigned long       cPagePreread;                       //  pages preread from disk
    unsigned long       cPageDirtied;                       //  clean pages modified
    unsigned long       cPageRedirtied;                     //  dirty pages modified
    unsigned long       cLogRecord;                         //  log records generated
    unsigned long       cbLogRecord;                        //  log record bytes generated
    unsigned __int64    cusecPageCacheMiss;                 //  page cache miss latency in microseconds
    unsigned long       cPageCacheMiss;                     //  page cache misses
    unsigned long       cSeparatedLongValueRead;            //  separated LV reads
    unsigned __int64    cusecLongValuePageCacheMiss;        //  page cache miss latency in microseconds while reading separated LV data
    unsigned long       cLongValuePageCacheMiss;            //  page cache misses while reading separated LV data
    unsigned long       cSeparatedLongValueCreated;         //  separated LV creations
    unsigned long       cPageUniqueCacheHits;               //  number of unique pages for which requests could be fulfilled by the buffer cache
    unsigned long       cPageUniqueCacheRequests;           //  number of unique pages for which requests were made to the buffer cache
    unsigned long       cDatabaseReads;                     //  number of database reads from disk
    unsigned long       cSumDatabaseReadQueueDepthImpact;   //  sum of the impact on disk queue depth made by each database read from disk
    unsigned long       cSumDatabaseReadQueueDepth;         //  sum of the actual disk queue depths experienced by each database read from disk
    unsigned __int64    cusecWait;                          //  elapsed thread wait time in microseconds
    unsigned long       cWait;                              //  number of thread waits
    unsigned long       cNodesFlagDeleted;                  //  number of nodes marked for delete
    unsigned long       cbNodesFlagDeleted;                 //  size of nodes marked for delete
    unsigned long       cPageTableAllocated;                //  number of pages allocated by a table from the database
    unsigned long       cPageTableReleased;                 //  number of pages released by a table to the database
    unsigned long       cPageUpdateAllocated;               //  number of pages allocated as a side effect of an update
    unsigned long       cPageUpdateReleased;                //  number of pages released as a side effect of an update
    unsigned long       cPageUniqueModified;                //  number of unique pages modified
};
#endif // JET_VERSION >= 0x0A01

#if ( JET_VERSION >= 0x0603 )
//  Resources supported by the JET
//  NOTE: Instanceless resources are not bound to a specific instance
//

typedef enum
{
    JET_residNull,                  //  invalid (null) resource id
    JET_residFCB,                   //  tables
    JET_residFUCB,                  //  cursors
    JET_residTDB,                   //  table descriptors
    JET_residIDB,                   //  index descriptors
    JET_residPIB,                   //  sessions
    JET_residSCB,                   //  sorted tables
    JET_residVERBUCKET,             //  buckets used by Vesrion store
    JET_residPAGE,                  //  [deprecated] general purposes page usage (instanceless)
    JET_residSPLIT,                 //  split struct used by BTree page spilt (instanceless)
    JET_residSPLITPATH,             //  another split struct used by BTree page split (instanceless)
    JET_residMERGE,                 //  merge sruct used by BTree pages merge (instanceless)
    JET_residMERGEPATH,             //  another merge struct used by BTree page merge (instanceless)
    JET_residINST,                  //  instances (instanceless)
    JET_residLOG,                   //  log instances (instanceless)
    // JET_residVER,                //  WAS version store win8 and prior. Removed in Win8.1. Therefore JET_residKEY has a different numerical value for Win8 and Win8.1!
                                    //  Since we removed JET_residVER at some point in the past, if at some future time makes this public, it should be locked down to
                                    //  JET_VERSION 0xXxx that is as or more recent than the JET_residVER removal.
    JET_residKEY,                   //  key buffers (instanceless)
    JET_residBOOKMARK,              //  bookmark buffers (instanceless)
    JET_residLRUKHIST,              //  LRUK history records (instanceless)
    JET_residRBSBuf,                //  Revert snapshot buffers
    JET_residTest,                  //  internal use only: for testing the resource manager
    JET_residMax,
    JET_residAll = 0x0fffffff       //  special value to be used when a setting is to be applied to all known resources
} JET_RESID;

#endif // JET_VERSION >= 0x0603

#if ( JET_VERSION >= 0x0600 )

//  Operations related with JET resources management
//  NOTE: on get the instance param is ignored, unless other is said

typedef enum
{
    JET_resoperNull,
    JET_resoperTag,         //  Each object class has (unique) tag, get only
    JET_resoperSize,        //  The size of the object, get only
    JET_resoperAlign,       //  Alignmet of the object, Set before jetinit
    JET_resoperMinUse,      //  Min number of preallocated objects, Set before jetinit
    JET_resoperMaxUse,      //  Max number of allocated object, After jetinit is set per instance
                            //  Befre jetinit is set for the whole process
                            //      and is rounded up to the full chunk
                            //  the count is object based
    JET_resoperObjectsPerChunk, //  Get only
    JET_resoperCurrentUse,  //  Get only, with nonnull instance param the count is object based
                            //  with null instance param the count is chunks based
    JET_resoperChunkSize,   //  Set before jetinit only
    JET_resoperLookasideEntries,    // Set before jetinit only
    JET_resoperRFOLOffset,  //  offset where to store the pointer to next free object
                            //  set before jet init only
                            //  NOTE: Debug and JET dev only
    JET_resoperGuard,       //  Turn the guard page at the end of the section on and off
                            //  Set before jetinit only
    JET_resoperAllocTopDown,    //  chunks are allocated using MEM_TOP_DOWN
    JET_resoperPreserveFreed,   //  freed chunks are not decommitted/released back to the OS
    JET_resoperAllocFromHeap,   //  objects are allocated directly from the heap, bypassing resource management
    JET_resoperEnableMaxUse,    //  Enables the enforcement of the max number of allocated objects
    JET_resoperRCIList,         //  Expose m_pRCIList for debug extension
    JET_resoperSectionHeader,   //  Expose cbSectionHeader for debug extension
    JET_resoperMax
} JET_RESOPER;

#endif // JET_VERSION >= 0x0600

// begin_PubEsent

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

#endif // JET_VERSION >= 0x0600

// end_PubEsent

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

#endif // JET_VERSION >= 0x0A01

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

//
//  This explains the heirarchy of callbacks a database will generate:
//
//  o eBTreeTypeInternalDbRootSpace (not really a B-Tree)
//      o eBTreeTypeInternalSpaceOE (of DB root)
//      o eBTreeTypeInternalSpaceAE (of DB root)
//      o eBTreeTypeUserClusteredIndex x N times (i.e. user defined tables)
//          o eBTreeTypeInternalSpaceOE (optional)
//          o eBTreeTypeInternalSpaceAE (optional)
//          o eBTreeTypeInternalLongValue (optional)
//              o eBTreeTypeInternalSpaceOE (optional)
//              o eBTreeTypeInternalSpaceAE (optional)
//          o eBTreeTypeUserSecondaryIndex x M times (i.e. user defined indices)
//              o eBTreeTypeInternalSpaceOE (optional)
//              o eBTreeTypeInternalSpaceAE (optional)
//      o ...more eBTreeTypeUserClusteredIndex 
//          o eBTreeTypeInternalSpaceOE (optional)
//          o eBTreeTypeInternalSpaceAE (optional)
//          o eBTreeTypeInternalLongValue (optional)
//              o eBTreeTypeInternalSpaceOE (optional)
//              o eBTreeTypeInternalSpaceAE (optional)
//          o ...more eBTreeTypeUserSecondaryIndex
//              o eBTreeTypeInternalSpaceOE (optional)
//              o eBTreeTypeInternalSpaceAE (optional)
//
// Note: When we say user defined tables and indices, its just where they would get
// called, but the callbacks for the BTree types will happen for system tables as 
// well such as the catalog, defrag table, etc.
//

//  Retrieved with JET_bitSpaceInfoBasicCatalog
//
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

//  Retrieved with JET_bitSpaceInfoSpaceTrees
//
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

//  Retrieved with JET_bitSpaceInfoFullWalk for data page.
//
typedef struct
{
    unsigned long                   cbStruct;
    JET_HISTO *                     phistoFreeBytes;            // per page
    JET_HISTO *                     phistoNodeCounts;           // per page (not including TAG 0)
    JET_HISTO *                     phistoKeySizes;             // per node
    JET_HISTO *                     phistoDataSizes;            // per node
    JET_HISTO *                     phistoKeyCompression;       // per compressed node
    JET_HISTO *                     phistoUnreclaimedBytes;     // per deleted node
#if ( JET_VERSION >= 0x0602 )
    __int64                         cVersionedNodes;            // node accumulation    
#endif
} BTREE_STATS_PAGE_SPACE;

//  Retrieved with JET_bitSpaceInfoParentOfLeaf
//

//  Some idea of some edge cases for this structure ...
//
//  Assumes 8 KB page size, here is some of the simple boundary cases...
//
//                  fEmpty  cDepth  cpgIntr cpgData Owned   Avail
//  DbRoot (4MB)    fFalse  0       0       0       512     34
//  -- tbl | idx --
//  no data         fTrue   1       0       1       1       0
//  1 small row     fFalse  1       0       1       1       0
//  5- 7.8k rows    fFalse  2       1       5       8       3
//      (w/8 pg PriExt)
//  -- oe | ae
//  OE:1-pg,2-node  fFalse? 1       0       1       1       0
//  AE:1-pg,0-node  fTrue?  1       0       1       1       0
//  AE:1-pg,1-node  fFalse? 1       0       1       1       0       0 avail, inspite of fact it represents avail pages.
//  OE:3-pg,many    fFalse? 2       1       2       3       0
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
    //
    //  Version and specified data.
    //
    unsigned long                   cbStruct;
    unsigned long                   grbitData;
    //
    //  ESE's B+ Trees / space are heirarchical.
    //
    struct _BTREE_STATS *           pParent;
    //
    //  Broken out data, by amount of effort.
    //
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
#endif // JET_VERSION >= 0x0601


//typedef struct
//  {
//  unsigned long   cDiscont;
//  unsigned long   cUnfixedMessyPage;
//  unsigned long   centriesLT;
//  unsigned long   centriesTotal;
//  unsigned long   cpgCompactFreed;
//  } JET_OLCSTAT;
// begin_PubEsent

#if ( JET_VERSION >= 0x0602 )

// JET_errcatError
//    |
//    |-- JET_errcatOperation
//    |     |-- JET_errcatFatal
//    |     |-- JET_errcatIO                //  bad IO issues, may or may not be transient.
//    |     |-- JET_errcatResource
//    |           |-- JET_errcatMemory      //  out of memory (all variants)
//    |           |-- JET_errcatQuota   
//    |           |-- JET_errcatDisk        //  out of disk space (all variants)
//    |-- JET_errcatData
//    |     |-- JET_errcatCorruption
//    |     |-- JET_errcatInconsistent      //  typically caused by user Mishandling
//    |     |-- JET_errcatFragmentation
//    |-- JET_errcatApi
//          |-- JET_errcatUsage
//          |-- JET_errcatState

// end_PubEsent
// The above hierarchy is represented in errorhierarchy.cxx as rgerrorHierarchies.
// Both places need to be updated together.
// begin_PubEsent

// A brief description of each error type
//
//  Operation(al) - Errors that can usually happen any time due to uncontrollable 
//                  conditions.  Frequently temporary, but not always.
//
//                  Recovery: Probably retry, or eventually inform the operator.
//
//      Fatal -     This sort error happens only when ESE encounters an error condition
//                  so grave, that we can not continue on in a safe (often transactional)
//                  way, and rather than corrupt data we throw errors of this category.
//
//                  Recovery: Restart the instance or process.  If the problem persists
//                  inform the operator.
//
//      IO -        IO errors come from the OS, and are out of ESE's control, this sort
//                  of error is possibly temporary, possibly not.
//
//                  Recovery: Retry.  If not resolved, ask operator about disk issue.
//
//      Resource -  This is a category that indicates one of many potential out-of-resource
//                  conditions.
//
//          Memory  Classic out of memory condition.
//
//                  Recovery: Wait a while and retry, free up memory, or quit.
//
//          Quota   Certain "specialty" resources are in pools of a certain size, making
//                  it easier to detect leaks of these resources.
//
//                  Recovery: Bug fix, generally the application should Assert() on these
//                  conditions so as to detect these issues during development.  However,
//                  in retail code, the best to hope for is to treat like Memory.
//
//          Disk    Out of disk conditions.
//
//                  Recovery: Can retry later in the hope more space is available, or 
//                  ask the operator to free some disk space.
//  Data
//
//      Corruption  My hard drive ate my homework.  Classic corruption issues, frequently
//                  permanent without corrective action.
//
//                  Recovery: Restore from backup, perhaps the ese utilities repair 
//                  operation (which only salvages what data is left / lossy).  Also
//                  in the case of recovery(JetInit) perhaps recovery can be performed
//                  by allowing data loss.
//
//      Inconsistent This is similar to Corruption in that the database and/or log files
//                  are in a state that is inconsistent and unreconcilable with each 
//                  other. Often this is caused by application/administrator mishandling.
//
//                  Recovery: Restore from backup, perhaps the ese utilities repair 
//                  operation (which only salvages what data is left / lossy).  Also
//                  in the case of recovery(JetInit) perhaps recovery can be performed
//                  by allowing data loss.
//
//      Fragmentation   This is a class of errors where some persisted internal resource ran
//                  out.
//
//                  Recovery: For database errors, offline defragmentation will rectify
//                  the problem, for the log files _first_ recover all attached databases
//                  to a clean shutdown, and then delete all the log files and checkpoint.
//
//  Api
//
//      Usage       Classic usage error, this means the client code did not pass correct
//                  arguments to the JET API.  This error will likely not go away with
//                  retry.
//
//                  Recovery: Generally speaking client code should Assert() this class
//                  of errors is not returned, so issues can be caught during development.
//                  In retail, the app will probably have little option but to return
//                  the issue up to the operator.
//
//      State       This is the classification for different signals the API could return
//                  describe the state of the database, a classic case is JET_errRecordNotFound
//                  which can be returned by JetSeek() when the record you asked for
//                  was not found.
//
//                  Recovery: Not really relevant, depends greatly on the API.
//

typedef enum
{
    JET_errcatUnknown = 0,  //      unknown, error retrieving err category
    JET_errcatError,        //      top level (no errors should be of this class)
    JET_errcatOperation,
    JET_errcatFatal,
    JET_errcatIO,           //      bad IO issues, may or may not be transient.
    JET_errcatResource,
    JET_errcatMemory,       //      out of memory (all variants)
    JET_errcatQuota,    
    JET_errcatDisk,         //      out of disk space (all variants)
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

// Output structure for JetGetErrorInfoW(). Not all fields may
// be populated by all error levels.
typedef struct
{
    unsigned long       cbStruct;
    JET_ERR             errValue;                   //  The error value for the requested info level.
    JET_ERRCAT          errcatMostSpecific;         //  The most specific category of the error.
    unsigned char       rgCategoricalHierarchy[8];  //  Hierarchy of error categories. Position 0 is the highest level in the hierarchy, and the rest are JET_errcatUnknown.
    unsigned long       lSourceLine;                //  The source file line for the requested info level.
    WCHAR               rgszSourceFile[64];         //  The source file name for the requested info level.
} JET_ERRINFOBASIC_W;

// grbits for JET_PFNDURABLECOMMITCALLBACK
#if ( JET_VERSION >= 0x0A00 )
#define JET_bitDurableCommitCallbackLogUnavailable      0x00000001  // Passed back to durable commit callback to let it know that log is down (and all pending commits will not be flushed to disk)
#endif

// commit-id from JetCommitTransaction2
typedef struct
{
    JET_SIGNATURE   signLog;
    int             reserved; // for packing so int64 below is 8-byte aligned on 32-bits despite the pshpack4 above
    __int64         commitId;
} JET_COMMIT_ID;

// assert that commit-id is 8-byte aligned so managed interop works correctly
// C_ASSERT( offsetof( JET_COMMIT_ID, commitId ) % 8 == 0 );

// callback for JET_paramDurableCommitCallback
typedef JET_ERR (JET_API *JET_PFNDURABLECOMMITCALLBACK)(
    _In_ JET_INSTANCE   instance,
    _In_ JET_COMMIT_ID *pCommitIdSeen,
    _In_ JET_GRBIT      grbit );

#endif // JET_VERSION >= 0x0602
// end_PubEsent

typedef struct
{
    long                    lRBSGeneration;             //  Revert snapshot generation.

    JET_LOGTIME             logtimeCreate;              //  date time file creation
    JET_LOGTIME             logtimeCreatePrevRBS;       //  date time prev file creation

    unsigned long           ulMajor;                    //  major version number
    unsigned long           ulMinor;                    //  minor version number

    unsigned long long      cbLogicalFileSize;          //  Logical file size
} JET_RBSINFOMISC;

typedef struct
{
    long                    lGenMinRevertStart;         // Min log generation across databases at start of revert.
    long                    lGenMaxRevertStart;         // Max log generation across databases at start of revert.

    long                    lGenMinRevertEnd;           // Min log generation across databases at end of revert.
    long                    lGenMaxRevertEnd;           // Max log generation across databases at end of revert.

    JET_LOGTIME             logtimeRevertFrom;          // The time we started reverting from. We will skip adding reverting to time as the caller already gets that info as part of prepare call.

    unsigned long long      cSecRevert;                 // Total secs spent in revert process.
    unsigned long long      cPagesReverted;             // Total pages reverted across all the database files as part of the revert.

    long                    lGenRBSMaxApplied;          // Max revert snapshot generation applied during revert.
    long                    lGenRBSMinApplied;          // Min revert snapshot generation applied during revert.
} JET_RBSREVERTINFOMISC;

// begin_PubEsent

/************************************************************************/
/*************************     JET CONSTANTS     ************************/
/************************************************************************/

#if ( JET_VERSION >= 0x0501 )
#define JET_instanceNil             (~(JET_INSTANCE)0)
#endif // JET_VERSION >= 0x0501
#define JET_sesidNil                (~(JET_SESID)0)
#define JET_tableidNil              (~(JET_TABLEID)0)
// end_PubEsent
#define JET_columnidNil             (~(JET_COLUMNID)0)
// begin_PubEsent
#define JET_bitNil                  ((JET_GRBIT)0)

    /* Max size of a bookmark */

#define JET_cbBookmarkMost          256
#if ( JET_VERSION >= 0x0601 )
#define JET_cbBookmarkMostMost      JET_cbKeyMostMost
#endif // JET_VERSION >= 0x0601

    /* Max length of a object/column/index/property name */

#ifndef JET_UNICODE
#define JET_cbNameMost              64
#else
#define JET_cbNameMost              128
#endif

    /* Max length of a "name.name.name..." construct */

#ifndef JET_UNICODE
#define JET_cbFullNameMost          255
#else
#define JET_cbFullNameMost          510
#endif

    /* Max size of long-value (LongBinary or LongText) column chunk */

//  #define JET_cbColumnLVChunkMost     ( JET_cbPage - 82 ) to the following:
//  Get cbPage from GetSystemParameter.
//  changed JET_cbColumnLVChunkMost reference to cbPage - JET_cbColumnLVPageOverhead

#define JET_cbColumnLVPageOverhead      82      // ONLY for small (<=8kiB) page, otherwise, query JET_paramLVChunkSizeMost

// end_PubEsent
#define JET_cbColumnLVChunkMost     ( 4096 - 82 ) // This def will be removed after other components change not to use this def
#define JET_cbColumnLVChunkMost_OLD 4035
// begin_PubEsent

    /* Max size of long-value (LongBinary or LongText) column default value */

#define JET_cbLVDefaultValueMost    255

    /* Max size of non-long-value column data */

#define JET_cbColumnMost            255

    /* Max size of long-value column data. */

#define JET_cbLVColumnMost          0x7FFFFFFF

    /* Max size of a sort/index key */

#if ( JET_VERSION >= 0x0601 )
#define JET_cbKeyMostMost               JET_cbKeyMost32KBytePage
#define JET_cbKeyMost32KBytePage        JET_cbKeyMost8KBytePage
#define JET_cbKeyMost16KBytePage        JET_cbKeyMost8KBytePage
#endif // JET_VERSION >= 0x0601
#if ( JET_VERSION >= 0x0600 )
#define JET_cbKeyMost8KBytePage     2000
#define JET_cbKeyMost4KBytePage     1000
#define JET_cbKeyMost2KBytePage     500
#define JET_cbKeyMostMin            255
#endif // JET_VERSION >= 0x0600

#define JET_cbKeyMost               255     //  defunct constant retained for backward compatibility
#define JET_cbLimitKeyMost          256     //  defunct constant retained for backward compatibility
#define JET_cbPrimaryKeyMost        255     //  defunct constant retained for backward compatibility
#define JET_cbSecondaryKeyMost      255     //  defunct constant retained for backward compatibility
// end_PubEsent
#define JET_cbKeyMost_OLD           255
// begin_PubEsent


    /* Max number of components in a sort/index key */

#if ( JET_VERSION >= 0x0600 )
#define JET_ccolKeyMost             16
#else // !JET_VERSION >= 0x0600
#define JET_ccolKeyMost             12
#endif // !JET_VERSION >= 0x0600

//  maximum number of columns
#if ( JET_VERSION >= 0x0501 )
#define JET_ccolMost                0x0000fee0
#else // !JET_VERSION >= 0x0501
#define JET_ccolMost                0x00007ffe
#endif // !JET_VERSION >= 0x0501
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
#endif // JET_VERSION >= 0x0601
#define JET_EventLoggingLevelMax    100
#endif // JET_VERSION >= 0x0501

#if ( JET_VERSION >= 0x0603 )
// Values for JET_paramEnableIndexChecking.
typedef enum
{
    JET_IndexCheckingOff = 0,
    JET_IndexCheckingOn = 1,
    JET_IndexCheckingDeferToOpenTable = 2,
    JET_IndexCheckingMax = 3,
} JET_INDEXCHECKING;
#endif

// end_PubEsent

// begin_PubEsent
// The following values are bit-fields that JET_paramIOPriority can be set to
#if ( JET_VERSION >= 0x0600 )
// Values for JET_paramIOPriority
#define JET_IOPriorityNormal                    0x0       // default
#define JET_IOPriorityLow                       0x1
#endif // JET_VERSION >= 0x0600
// end_PubEsent
#if ( JET_VERSION >= 0x0603 )
#define JET_IOPriorityLowForCheckpoint          0x2
#define JET_IOPriorityLowForScavenge            0x4

// Diagnostic IO Priority Flags - No behavioral effects.
#define JET_IOPriorityUserClassIdMask           0x0F000000  //  User Level Priority Class ID.  Only used for tracing, produces no ESE behavioral changes.
#define JET_IOPriorityMarkAsMaintenance         0x40000000  //  Identifies the IO activity produced by this session as Maintenance (vs. Transactional) for perfmon.  Only used for perfmon, produces no ESE behavioral changes.
#endif // JET_VERSION >= 0x0603
// begin_PubEsent

#if ( JET_VERSION >= 0x0602 )
//  Values for usage with JET_paramConfiguration
//
//  Can set the optimization configs one at a time.
//
#define JET_configDefault                       0x0001  //  Resets ALL parameters to their default value
#define JET_configRemoveQuotas                  0x0002  //  Unrestricts the quota enforcement (by setting to as high as possible) for any ESE handle types where memory is not pre-allocated or used as a cache.
#define JET_configLowDiskFootprint              0x0004  //  Set appropriate parameters to optimize the engine to use a small amount of disk space.  Uses circular logging.
#define JET_configMediumDiskFootprint           0x0008  //  Set appropriate parameters to optimize the engine to use a medium amount of disk space.  Uses circular logging.
#define JET_configLowMemory                     0x0010  //  Set appropriate parameters to optimize the engine to use a small amount of memory/working set at the cost of CPU efficiency and some disk efficiency.
#define JET_configDynamicMediumMemory           0x0020  //  Set appropriate parameters to optimize the engine to use a modest amount of memory/working set at the cost of CPU efficiency, dynamically adjusting for bursts in activity.
#define JET_configLowPower                      0x0040  //  Set appropriate parameters to optimize the engine to attempt to conserve power over keeping everything the most up to date, or memory usage.
#define JET_configSSDProfileIO                  0x0080  //  Set appropriate parameters to optimize the engine to be using the SSD profile IO parameters.
#define JET_configRunSilent                     0x0100  //  Turns off all externally visible signs of the library running (event logs, perfmon, tracing, etc).  NOTE: This makes debugging issues difficult, best if app policy has way to configure this off or on.
#if ( JET_VERSION >= 0x0A00 )
#define JET_configUnthrottledMemory             0x0200  //  Allows ESE to grow to most of memory because this is likely a single purpose server for this machine, or wants to allow our variable memory caches to grow to use most of memory if in use.
#define JET_configHighConcurrencyScaling        0x0400  //  Ensures ESE uses all its high concurrency scaling methods to achieve high levels of performance on multi-CPU systems (SMP, Multi-Core, Hyper-Threading, etc) for server scale applications, at a higher fixed memory overhead.
#endif // JET_VERSION >= 0x0A00

// end_PubEsent
#define JET_configMask                          (JET_configDefault|JET_configRemoveQuotas|JET_configLowDiskFootprint|JET_configMediumDiskFootprint|JET_configLowMemory|JET_configDynamicMediumMemory|JET_configLowPower|JET_configSSDProfileIO|JET_configRunSilent|JET_configUnthrottledMemory|JET_configHighConcurrencyScaling)
// begin_PubEsent
#endif // JET_VERSION >= 0x0602

//  system parameters
//
//  NOTE:  the default values of these parameters used to be documented here.
//  this can no longer be done because we now support multiple sets of default
//  values as set by JET_paramConfiguration
//
//  location parameters
//
#define JET_paramSystemPath                     0   //  path to check point file
#define JET_paramTempPath                       1   //  path to the temporary database
#define JET_paramLogFilePath                    2   //  path to the log file directory
#define JET_paramBaseName                       3   //  base name for all DBMS object names
#define JET_paramEventSource                    4   //  language independent process descriptor string

//  performance parameters
//
#define JET_paramMaxSessions                    5   //  maximum number of sessions
#define JET_paramMaxOpenTables                  6   //  maximum number of open directories
                                                    //      need 1 for each open table index,
                                                    //      plus 1 for each open table with no indexes,
                                                    //      plus 1 for each table with long column data,
                                                    //      plus a few more.
                                                    //      for 4.1, 1/3 for regular table, 2/3 for index
#define JET_paramPreferredMaxOpenTables         7   //  preferred maximum number of open directories
#if ( JET_VERSION >= 0x0600 )
#define JET_paramCachedClosedTables             125 //  number of closed tables to cache the meta-data for
#endif // JET_VERSION >= 0x0600
#define JET_paramMaxCursors                     8   //  maximum number of open cursors
#define JET_paramMaxVerPages                    9   //  maximum version store size in version pages
#define JET_paramPreferredVerPages              63  //  preferred version store size in version pages
#if ( JET_VERSION >= 0x0501 )
#define JET_paramGlobalMinVerPages              81  //  minimum version store size for all instances in version pages
#define JET_paramVersionStoreTaskQueueMax       105 //  maximum number of tasks in the task queue before start dropping the tasks
#endif // JET_VERSION >= 0x0501
#define JET_paramMaxTemporaryTables             10  //  maximum concurrent open temporary table/index creation
#define JET_paramLogFileSize                    11  //  log file size in kBytes
#define JET_paramLogBuffers                     12  //  log buffers in 512 byte units.
#define JET_paramWaitLogFlush                   13  //  log flush wait time in milliseconds
#define JET_paramLogCheckpointPeriod            14  //  checkpoint period in sectors
#define JET_paramLogWaitingUserMax              15  //  maximum sessions waiting log flush
#define JET_paramCommitDefault                  16  //  default grbit for JetCommitTransaction
#define JET_paramCircularLog                    17  //  boolean flag for circular logging
#define JET_paramDbExtensionSize                18  //  database extension size in pages
#define JET_paramPageTempDBMin                  19  //  minimum size temporary database in pages
#define JET_paramPageFragment                   20  //  maximum disk extent considered fragment in pages
// end_PubEsent
#define JET_paramPageReadAheadMax               21  //  maximum read-ahead in pages
// begin_PubEsent
#if ( JET_VERSION >= 0x0600 )
#define JET_paramEnableFileCache                126 //  enable the use of the OS file cache for all managed files
#define JET_paramVerPageSize                    128 //  the version store page size
#define JET_paramConfiguration                  129 //  RESETs all parameters to their default for a given configuration
#define JET_paramEnableAdvanced                 130 //  enables the modification of advanced settings
#define JET_paramMaxColtyp                      131 //  maximum coltyp supported by this version of ESE
#endif // JET_VERSION >= 0x0600

//  cache performance parameters
//
#define JET_paramBatchIOBufferMax               22  //  maximum batch I/O buffers in pages
#define JET_paramCacheSize                      41  //  current cache size in pages
#define JET_paramCacheSizeMin                   60  //  minimum cache size in pages
#define JET_paramCacheSizeMax                   23  //  maximum cache size in pages
#define JET_paramCheckpointDepthMax             24  //  maximum checkpoint depth in bytes
#define JET_paramLRUKCorrInterval               25  //  time (usec) under which page accesses are correlated
#define JET_paramLRUKHistoryMax                 26  //  maximum LRUK history records
#define JET_paramLRUKPolicy                     27  //  K-ness of LRUK page eviction algorithm (1...2)
#define JET_paramLRUKTimeout                    28  //  time (sec) after which cached pages are always evictable
#define JET_paramLRUKTrxCorrInterval            29  //  Not Used: time (usec) under which page accesses by the same transaction are correlated
#define JET_paramOutstandingIOMax               30  //  maximum outstanding I/Os
#define JET_paramStartFlushThreshold            31  //  evictable pages at which to start a flush (proportional to CacheSizeMax)
#define JET_paramStopFlushThreshold             32  //  evictable pages at which to stop a flush (proportional to CacheSizeMax)
// end_PubEsent
#define JET_paramTableClassName                 33  //  table stats class name (class #, string)
#define JET_paramIdleFlushTime                  106 //  time interval (msec) over which all dirty pages should be written to disk after idle conditions are detected.
#define JET_paramVAReserve                      109 //  amount of address space (bytes) to reserve from use by the cache
#define JET_paramDBAPageAvailMin                120 //  limit of VM pages available at which NT starts to page out processes
#define JET_paramDBAPageAvailThreshold          121 //  constant used in internal calculation of page eviction rate *THIS IS A DOUBLE, PASS A POINTER*
#define JET_paramDBAK1                          122 //  constant used in internal DBA calculation *THIS IS A DOUBLE, PASS A POINTER*
#define JET_paramDBAK2                          123 //  constant used in internal DBA calculation *THIS IS A DOUBLE, PASS A POINTER*
#define JET_paramMaxRandomIOSize                124 //  maximum size of scatter/gather I/O in bytes
// begin_PubEsent
#if ( JET_VERSION >= 0x0600 )
#define JET_paramEnableViewCache                127 //  enable the use of memory mapped file I/O for database files
#define JET_paramCheckpointIOMax                135 //  maxiumum number of pending flush writes
#endif // JET_VERSION >= 0x0600


#if ( JET_VERSION >= 0x0600 )
// TableClass names
#define JET_paramTableClass1Name                137     // name of tableclass1
#define JET_paramTableClass2Name                138     // name of tableclass2
#define JET_paramTableClass3Name                139     // name of tableclass3
#define JET_paramTableClass4Name                140     // name of tableclass4
#define JET_paramTableClass5Name                141     // name of tableclass5
#define JET_paramTableClass6Name                142     // name of tableclass6
#define JET_paramTableClass7Name                143     // name of tableclass7
#define JET_paramTableClass8Name                144     // name of tableclass8
#define JET_paramTableClass9Name                145     // name of tableclass9
#define JET_paramTableClass10Name               146     // name of tableclass10
#define JET_paramTableClass11Name               147     // name of tableclass11
#define JET_paramTableClass12Name               148     // name of tableclass12
#define JET_paramTableClass13Name               149     // name of tableclass13
#define JET_paramTableClass14Name               150     // name of tableclass14
#define JET_paramTableClass15Name               151     // name of tableclass15
#endif // JET_VERSION >= 0x0600

// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )
// TableClass names (continued)
#define JET_paramTableClass16Name               196     // name of tableclass16
#define JET_paramTableClass17Name               197     // name of tableclass17
#define JET_paramTableClass18Name               198     // name of tableclass18
#define JET_paramTableClass19Name               199     // name of tableclass19
#define JET_paramTableClass20Name               200     // name of tableclass20
#define JET_paramTableClass21Name               201     // name of tableclass21
#define JET_paramTableClass22Name               202     // name of tableclass22
#define JET_paramTableClass23Name               203     // name of tableclass23
#define JET_paramTableClass24Name               204     // name of tableclass24
#define JET_paramTableClass25Name               205     // name of tableclass25
#define JET_paramTableClass26Name               206     // name of tableclass26
#define JET_paramTableClass27Name               207     // name of tableclass27
#define JET_paramTableClass28Name               208     // name of tableclass28
#define JET_paramTableClass29Name               209     // name of tableclass29
#define JET_paramTableClass30Name               210     // name of tableclass30
#define JET_paramTableClass31Name               211     // name of tableclass31
#endif // JET_VERSION >= 0x0A01
// begin_PubEsent

#define JET_paramIOPriority                     152     //  adjust IO priority per instance, anytime. Mainly for background recovery
                                                        //  Doesn't affect pending IOs, just subsequent ones

#define JET_paramRecovery                       34  //  enable recovery via setting the string "On" or "Off"
// end_PubEsent
#define JET_paramOnLineCompact                  35  //  enable online defrag
// begin_PubEsent
#define JET_paramEnableOnlineDefrag             35  //  enable online defrag
// end_PubEsent

//  debug only parameters
//
#define JET_paramAssertAction                   36  //  action on assert
#define JET_paramPrintFunction                  37  //  synched print function
#define JET_paramTransactionLevel               38  //  transaction level of session [deprecated - use JetGetSessionParameter( sesid, JET_sesparamTransactionLevel ... )]
#define JET_paramRFS2IOsPermitted               39  //  #IOs permitted to succeed [-1 = all]
#define JET_paramRFS2AllocsPermitted            40  //  #allocs permitted to success [-1 = all]
//                                              41  //  JET_paramCacheSize defined above
#define JET_paramCacheRequests                  42  //  #cache requests (Read Only)
#define JET_paramCacheHits                      43  //  #cache hits (Read Only)
// begin_PubEsent

//  Application specific parameter
//
#define JET_paramCheckFormatWhenOpenFail        44  //  JetInit may return JET_errDatabaseXXXformat instead of database corrupt when it is set
#define JET_paramEnableTempTableVersioning      46  //  Enable versioning of temp tables
#define JET_paramIgnoreLogVersion               47  //  Do not check the log version
#define JET_paramDeleteOldLogs                  48  //  Delete the log files if the version is old, after deleting may make database non-recoverable
#define JET_paramEventSourceKey                 49  //  Event source registration key value
#define JET_paramNoInformationEvent             50  //  Disable logging information event
#if ( JET_VERSION >= 0x0501 )
#define JET_paramEventLoggingLevel              51  //  Set the type of information that goes to event log
#define JET_paramDeleteOutOfRangeLogs           52  //  Delete the log files that are not matching (generation wise) during soft recovery
#define JET_paramAccessDeniedRetryPeriod        53  //  Number of milliseconds to retry when about to fail with AccessDenied
#endif // JET_VERSION >= 0x0501

//  Index-checking parameters
//
//  After Windows 7, it was discovered that JET_paramEnableIndexCleanup had some implementation limitations, reducing its effectiveness.
//  Rather than update it to work with locale names, the functionality is removed altogether.
//
//  Unfortunately JET_paramEnableIndexCleanup can not be ignored altogether. JET_paramEnableIndexChecking defaults to false, so if
//  JET_paramEnableIndexCleanup were to be removed entirely, then by default there were would be no checks for NLS changes!
//
//  The current behavious (when enabled) is to track the language sort versions for the indices, and when the sort version for that
//  particular locale changes, the engine knows which indices are now invalid. For example, if the sort version for only "de-de" changes,
//  then the "de-de" indices are invalid, but the "en-us" indices will be fine.
//
//  Post-Windows 8:
//  JET_paramEnableIndexChecking accepts JET_INDEXCHECKING (which is an enum). The values of '0' and '1' have the same meaning as before,
//  but '2' is JET_IndexCheckingDeferToOpenTable, which means that the NLS up-to-date-ness is NOT checked when the database is attached.
//  It is deferred to JetOpenTable(), which may now fail with JET_errPrimaryIndexCorrupted or JET_errSecondaryIndexCorrupted (which
//  are NOT actual corruptions, but instead reflect an NLS sort change).
//
//  IN SUMMARY:
//  New code should explicitly set both IndexChecking and IndexCleanup to the same value.
//
//
//  OLDER NOTES (up to and including Windows 7)
//
//  Different versions of windows normalize unicode text in different ways. That means indexes built under one version of Windows may
//  not work on other versions. Windows Server 2003 Beta 3 introduced GetNLSVersion() which can be used to determine the version of unicode normalization
//  that the OS currently provides. Indexes built in server 2003 are flagged with the version of unicode normalization that they were
//  built with (older indexes have no version information). Most unicode normalization changes consist of adding new characters -- codepoints
//  which were previously undefined are defined and normalize differently. Thus, if binary data is stored in a unicode column it will normalize
//  differently as new codepoints are defined.
//
//  As of Windows Server 2003 RC1 ESENT tracks unicode index entries that contain undefined codepoints. These can be used to fixup an index when the
//  set of defined unicode characters changes.
//
//  These parameters control what happens when ESENT attaches to a database that was last used under a different build of the OS (the OS version
//  is stamped in the database header).
//
//  If JET_paramEnableIndexChecking is TRUE JetAttachDatabase() will delete indexes if JET_bitDbDeleteCorruptIndexes or return an error if
//  the grbit was not specified and there are indexes which need deletion. If it is set to FALSE then JetAttachDatabase() will succeed, even
//  if there are potentially corrupt indexes.
//
//  If JET_paramEnableIndexCleanup is set, the internal fixup table will be used to fixup index entries. This may not fixup all index corruptions
//  but will be transparent to the application.
//

#define JET_paramEnableIndexChecking            45  //  Enable checking OS version for indexes (false by default).
#if ( JET_VERSION >= 0x0502 )
#define JET_paramEnableIndexCleanup             54  //  Enable cleanup of out-of-date index entries (Windows 2003 through Windows 7); Does NLS version checking (Windows 2003 and later).
#endif // JET_VERSION >= 0x0502

// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )
#define JET_paramFlight_SmoothIoTestPermillage  55  //  The per mille of total (or one thousandths, or tenths of a percent) of IO should be made smooth.  Ex(s): 995‰ = 99.5% smooth, 10‰ = 1%, etc.  0 = disabled.
#define JET_paramFlight_ElasticWaypointLatency  56  //  Amount of extra elastic waypoint latency
#define JET_paramFlight_SynchronousLVCleanup    57  //  Perform synchronous cleanup (actual delete) of LVs instead of flag delete with cleanup happening later
#define JET_paramFlight_RBSRevertIOUrgentLevel  58  // IO urgent level for reverting the databases using RBS. Used to decide how many outstanding I/Os will be allowed.
#define JET_paramFlight_EnableXpress10Compression 59 //  Enable Xpress10 compression using corsica hardware
#endif // JET_VERSION >= 0x0A01
// begin_PubEsent

//                                              60  //  JET_paramCacheSizeMin defined above
// end_PubEsent
#define JET_paramLogFileFailoverPath            61  //  path to use if the log file disk should fail
#define JET_paramEnableImprovedSeekShortcut     62  //  check to see if we are seeking for the record we are currently on
// begin_PubEsent
//                                              63  //  JET_paramPreferredVerPages defined above
#define JET_paramDatabasePageSize               64  //  set database page size
#if ( JET_VERSION >= 0x0501 )
#define JET_paramDisableCallbacks               65  //  turn off callback resolution (for defrag/repair)
#endif // JET_VERSION >= 0x0501
#if ( JET_VERSION >= 0x0501 )
// end_PubEsent
#define JET_paramAbortRetryFailCallback         108 //  I/O error callback (JET_ABORTRETRYFAILCALLBACK)

//  Backup performance parameters
//
#define JET_paramBackupChunkSize                66  //  backup read size in pages
#define JET_paramBackupOutstandingReads         67  //  backup maximum reads outstanding

#define JET_paramFlight_CheckRedoNeededBeyondRequiredRange 68 //  Check to make sure that all updates beyond required range need to be applied during redo (i.e. are not already written out to the database)
// begin_PubEsent
#define JET_paramLogFileCreateAsynch            69  //  prepares next log file while logging to the current one to smooth response time
#endif // JET_VERSION >= 0x0501
#define JET_paramErrorToString                  70  //  turns a JET_err into a string (taken from the comment in jet.h)
#if ( JET_VERSION >= 0x0501 )
#define JET_paramZeroDatabaseDuringBackup       71  //  Overwrite deleted records/LVs during backup
#endif // JET_VERSION >= 0x0501
#define JET_paramUnicodeIndexDefault            72  //  default LCMapString() lcid and flags to use for CreateIndex() and unique multi-values check
                                                    //      (pass pointer to JET_UNICODEINDEX structure for plParam and sizeof(JET_UNICODE_INDEX) for cbMax)
#if ( JET_VERSION >= 0x0501 )
#define JET_paramRuntimeCallback                73  //  pointer to runtime-only callback function
// end_PubEsent
#define JET_paramFlight_EnableReattachRaceBugFix 74 //  Enable bug fix for race between dirty-cache-keep-alive database reattach and checkpoint update
// #define JET_paramSLVDefragFreeThreshold      74  //  chunks whose free % is > this will be allocated from
#define JET_paramFlight_EnableLz4Compression    75 //  Enable Lz4 compression
// #define JET_paramSLVDefragMoveThreshold      75  //  chunks whose free % is > this will be relocated
#define JET_paramEnableSortedRetrieveColumns    76  //  internally sort (in a dynamically allocated parallel array) JET_RETRIEVECOLUMN structures passed to JetRetrieveColumns()
// begin_PubEsent
#endif // JET_VERSION >= 0x0501
#define JET_paramCleanupMismatchedLogFiles      77  //  instead of erroring out after a successful recovery with JET_errLogFileSizeMismatchDatabasesConsistent, ESE will silently delete the old log files and checkpoint file and continue operations
#if ( JET_VERSION >= 0x0501 )
#define JET_paramRecordUpgradeDirtyLevel        78  //  how aggresively should pages with their record format converted be flushed (0-3)
// end_PubEsent
#define JET_paramRecoveryCurrentLogfile         79  //  which generation is currently being replayed (read only)
// begin_PubEsent
//                                              81  //  JET_paramGlobalMinVerPages defined above
#define JET_paramOSSnapshotTimeout              82  //  timeout for the freeze period in msec
// end_PubEsent
#define JET_paramFlight_SkipDbHeaderWriteForLgenCommittedUpdate 83  //  Skip database header write only for lgenCommitted update (lgenMinRequired and lgenMaxRequired updates would still trigger the write)

#if ( JET_VERSION >= 0x0A01 )

#define JET_paramFlight_RBSForceRollIntervalSec                 80  // Time after which we should force roll into new revert snapshot by raising failure item and letting HA remount. This is temporary till we have live roll.

#define JET_paramFlight_NewQueueOptions                         84  //  Controls options for new Meted IO Queue
#define JET_paramFlight_ConcurrentMetedOps                      85  //  Controls how many IOs we leave out at once for the new Meted IO Queue.
#define JET_paramFlight_LowMetedOpsThreshold                    86  //  Controls the transition from 1 meted op to JET_paramFlight_ConcurrentMetedOps (which is the max).
#define JET_paramFlight_MetedOpStarvedThreshold                 87  //  Milliseconds until a meted IO op is considered starved and dispatched no matter what.

#define JET_paramFlight_MaxRBSBuffers                           88  //  Max number of buffers to allocate for revert snapshot.

#define JET_paramFlight_EnableShrinkArchiving                   89  //  Turns on archiving truncated data when shrinking a database (subject to efv).

#define JET_paramFlight_EnableBackupDuringRecovery              90  //  Turns on backup during recovery (i.e. seed from passive copy).

#define JET_paramFlight_RBSRollIntervalSec                      91 // Time after which we should roll into new revert snapshot.
#define JET_paramFlight_RBSMaxRequiredRange                     92 // Max required range allowed for revert snapshot. If combined required range of the dbs is greater than this we will skip creating the revert snapshot 
#define JET_paramFlight_RBSCleanupEnabled                       93 // Turns on clean up for revert snapshot.
#define JET_paramFlight_RBSLowDiskSpaceThresholdGb              94 // Low disk space in gigabytes at which we will start cleaning up RBS aggressively.
#define JET_paramFlight_RBSMaxSpaceWhenLowDiskSpaceGb           95 // Max alloted space in gigabytes for revert snapshots when the disk space is low.
#define JET_paramFlight_RBSMaxTimeSpanSec                       96 // Max timespan of a revert snapshot
#define JET_paramFlight_RBSCleanupIntervalMinSec                97 // Min time between cleanup attempts of revert snapshots.

#endif // JET_VERSION >= 0x0A01

// begin_PubEsent
#endif // JET_VERSION >= 0x0501

#define JET_paramExceptionAction                98  //  what to do with exceptions generated within JET
#define JET_paramEventLogCache                  99  //  number of bytes of eventlog records to cache if service is not available

#if ( JET_VERSION >= 0x0501 )
#define JET_paramCreatePathIfNotExist           100 //  create system/temp/log/log-failover paths if they do not exist
#define JET_paramPageHintCacheSize              101 //  maximum size of the fast page latch hint cache in bytes
#define JET_paramOneDatabasePerSession          102 //  allow just one open user database per session
// end_PubEsent
#define JET_paramMaxDatabasesPerInstance        103 //  maximum number of databases per instance
// begin_PubEsent
#define JET_paramMaxInstances                   104 //  maximum number of instances per process
// end_PubEsent
//                                              105 //  JET_paramVersionStoreTaskQueueMax
//                                              106 //  JET_paramIdleFlushTime
// begin_PubEsent
#define JET_paramDisablePerfmon                 107 //  disable perfmon support for this process
// end_PubEsent
//                                              108 //  JET_paramAbortRetryFailCallback
//                                              109 //  JET_paramVAReserve
// begin_PubEsent

#define JET_paramIndexTuplesLengthMin           110 //  for tuple indexes, minimum length of a tuple
#define JET_paramIndexTuplesLengthMax           111 //  for tuple indexes, maximum length of a tuple
#define JET_paramIndexTuplesToIndexMax          112 //  for tuple indexes, maximum number of characters in a given string to index
#endif // JET_VERSION >= 0x0501

// Parameters added in Windows 2003/XP64.
#if ( JET_VERSION >= 0x0502 )
#define JET_paramAlternateDatabaseRecoveryPath  113 //  recovery-only - search for dirty-shutdown databases in specified location only
#endif // JET_VERSION >= 0x0502
// end_PubEsent
//                                              120 //  JET_paramDBAPageAvailMin
//                                              121 //  JET_paramDBAPageAvailThreshold
//                                              122 //  JET_paramDBAK1
//                                              123 //  JET_paramDBAK2
//                                              124 //  JET_paramMaxRandomIOSize
//                                              125 //  JET_paramCachedClosedTables
// begin_PubEsent

// Parameters added in Windows Vista.
#if ( JET_VERSION >= 0x0600 )
#define JET_paramIndexTupleIncrement            132 //  for tuple indexes, offset increment for each succesive tuple
#define JET_paramIndexTupleStart                133 //  for tuple indexes, offset to start tuple indexing
#define JET_paramKeyMost                        134 //  read only maximum settable key length before key trunctation occurs
#define JET_paramLegacyFileNames                136  // Legacy  file name characteristics to preserve ( JET_bitESE98FileNames | JET_bitEightDotThreeSoftCompat )
#define JET_paramEnablePersistedCallbacks       156  //  allow the database engine to resolve and use callbacks persisted in a database
#endif // JET_VERSION >= 0x0600

// Parameters added in Windows 7.
#if ( JET_VERSION >= 0x0601 )
#define JET_paramWaypointLatency                153  // The latency (in logs) behind the tip / highest committed log to defer database page flushes.
// end_PubEsent
#define JET_paramCheckpointTooDeep              154  // The maximum allowed depth (in logs) of the checkpoint.  Once this limit is reached, updates will fail with JET_errCheckpointDepthTooDeep.
#define JET_paramDisableBlockVerification       155  // TEST ONLY:  use to disable block checksum verification for file fuzz testing
#define JET_paramAggressiveLogRollover          157  // force log rollover after certain operations
#define JET_paramPeriodicLogRolloverLLR         158  // force log rollover after a certain length of inactivity (currently requires setting a waypoint to take affect)
#define JET_paramUsePageDependencies            159  // use page dependencies when logging splits/merges (reduces data logged)
// begin_PubEsent
#define JET_paramDefragmentSequentialBTrees     160 //  Turn on/off automatic sequential B-tree defragmentation tasks (On by default, but also requires JET_SPACEHINTS flags / JET_bitRetrieveHintTableScan* to trigger on any given tables).
#define JET_paramDefragmentSequentialBTreesDensityCheckFrequency    161 //  Determine how frequently B-tree density is checked
#define JET_paramIOThrottlingTimeQuanta         162 //  Max time (in MS) that the I/O throttling mechanism gives a task to run for it to be considered 'completed'.
#define JET_paramLVChunkSizeMost                163 //  Max LV chunk size supported wrt the chosen page size (R/O)
#define JET_paramMaxCoalesceReadSize            164 //  Max number of bytes that can be grouped for a coalesced read operation.
#define JET_paramMaxCoalesceWriteSize           165 //  Max number of bytes that can be grouped for a coalesced write operation.
#define JET_paramMaxCoalesceReadGapSize         166 //  Max number of bytes that can be gapped for a coalesced read IO operation.
#define JET_paramMaxCoalesceWriteGapSize        167 //  Max number of bytes that can be gapped for a coalesced write IO operation.
// end_PubEsent
#define JET_paramEnableHaPublish                168 //  Event through HA publishing mechanism.
// begin_PubEsent
#define JET_paramEnableDBScanInRecovery         169 //  Do checksumming of the database during recovery.
#define JET_paramDbScanThrottle                 170 //  throttle (mSec).
#define JET_paramDbScanIntervalMinSec           171 //  Min internal to repeat checksumming (Sec).
#define JET_paramDbScanIntervalMaxSec           172 //  Max internal checksumming must finish (Sec).
// end_PubEsent
#define JET_paramEmitLogDataCallback            173 //  Set the callback for emitting log data to an external 3rd party.
#define JET_paramEmitLogDataCallbackCtx         174 //  Context for the the callback for emitting log data to an external 3rd party.
// begin_PubEsent
#endif // JET_VERSION >= 0x0601

#if ( JET_VERSION >= 0x0602 )
// end_PubEsent
#define JET_paramEnableExternalAutoHealing      175 //  Enable logging of page patch request and callback on page patch request processing (and corrupt page notification) during recovery.
#define JET_paramPatchRequestTimeout            176 //  Time before an outstanding patch request is considered stale (Seconds).
// begin_PubEsent

#define JET_paramCachePriority                  177 //  Per-instance property for relative cache priorities (default = 100).
                                                    //
                                                    //  There are three scopes for which cache priority may be assigned:
                                                    //
                                                    //    - Instance: by calling JetSetSystemParameter and setting JET_paramCachePriority for a specific
                                                    //                ESE instance. The cache priority for this scope is always defined (default is 100),
                                                    //                even if the client does not set the priority explicitly using the system parameter.
                                                    //    - Session: by calling JetSetSessionParameter and setting JET_sesparamCachePriority for a specific
                                                    //               ESE session. The cache priority for this scope is undefined by default.
                                                    //    - Database: by calling JetCreateDatabase3 (or above) or JetAttachDatabase3 (or above) and setting
                                                    //                JET_dbparamCachePriority for a specific new or attached database. The cache priority
                                                    //                for this scope is undefined by default.
                                                    //
                                                    //  The way cache priority for those three scopes interact is as follows:
                                                    //    - If only the priority for the instance scope is defined, it is used for all operations related
                                                    //      to that instance.
                                                    //    - If only the priorities for the instance and session scopes are defined, the session scope priority
                                                    //      is used for all operations related to that session.
                                                    //    - If only the priorities for the instance and database scopes are defined, the database scope priority
                                                    //      is used for all operations related to that database.
                                                    //    - If the priorities for all three scopes are defined, the lowest of session and database scope
                                                    //      priorities is used for all operations related to that session/database combination. For everything
                                                    //      else, the rules above apply on a per-database-page basis.

#define JET_paramMaxTransactionSize             178 //  Percentage of version store that can be used by oldest transaction before JET_errVersionStoreOutOfMemory (default = 100).
#define JET_paramPrereadIOMax                   179 //  Maximum number of I/O operations dispatched for a given purpose.
#define JET_paramEnableDBScanSerialization      180 //  Database Maintenance serialization is enabled for databases sharing the same disk.
#define JET_paramHungIOThreshold                181 //  The threshold for what is considered a hung IO that should be acted upon.
#define JET_paramHungIOActions                  182 //  A set of actions to be taken on IOs that appear hung.
#define JET_paramMinDataForXpress               183 //  Smallest amount of data that should be compressed with xpress compression.
#endif // JET_VERSION >= 0x0602

#if ( JET_VERSION >= 0x0603 )
#define JET_paramEnableShrinkDatabase           184 //  Release space back to the OS when deleting data. This may require an OS feature of Sparse Files, and is subject to change.
// end_PubEsent

// DEPRECATED: this was once used in the first implementation of DB shrink.
// #define JET_paramAutomaticShrinkDatabaseFreeSpaceThreshold   185 //  DEPRECATED: Minimum threshold (percentage of the database size) that determines if the periodic shrink and/or shrink at JetTerm will take place or not.

// begin_PubEsent

#endif // JET_VERSION >= 0x0603

// Parameters added in Windows 8.
#if ( JET_VERSION >= 0x0602 )
// System parameter 185 is reserved.
#define JET_paramProcessFriendlyName            186 //  Friendly name for this instance of the process (e.g. performance counter global instance name, event logs).
#define JET_paramDurableCommitCallback          187 //  callback for when log is flushed
#endif // JET_VERSION >= 0x0602

// Parameters added in Windows 8.1.
#if ( JET_VERSION >= 0x0603 )
#define JET_paramEnableSqm                      188 //  Deprecated / ignored param.
#endif // JET_VERSION >= 0x0603

// Parameters added in Windows 10.
#if ( JET_VERSION >= 0x0A00 )

#define JET_paramConfigStoreSpec                189 //  Custom path that allows the consumer to specify a path (currently from in the registry) from which to pull custom ESE configuration.

// end_PubEsent
#define JET_paramStageFlighting                 190 //  It's like stage fright-ing but different - use JET_bitStage* bits.
#define JET_paramZeroDatabaseUnusedSpace        191 //  Controls scrubbing of unused database space.
#define JET_paramDisableVerifications           192 //  Verification modes disabled
// begin_PubEsent

#endif // JET_VERSION >= 0x0A00

// end_PubEsent

#if ( JET_VERSION >= 0x0A01 )
#define JET_paramPersistedLostFlushDetection    193 //  Configures persisted lost flush detection for databases while attached to an instance.
#define JET_paramEngineFormatVersion            194 //  Engine format version - specifies the maximum format version the engine should allow, ensuring no format features are used beyond this (allowing the DB / logs to be forward compatible).
#define JET_paramReplayThrottlingLevel          195 //  Should replay be throttled so as not to generate too much disk IO

#define JET_paramBlockCacheConfiguration        212 //  Configuration for the ESE Block Cache via an IBlockCacheConfiguration* (optional).

#define JET_paramRecordSizeMost                 213 //  Read only param that returns the maximum record size supported by the current pagesize.
                                                    //  This includes storage overhead. Use in combination with JetGetRecordSize().
#endif // JET_VERSION >= 0x0A01

// begin_PubEsent
#if ( JET_VERSION >= 0x0A01 )
#define JET_paramUseFlushForWriteDurability     214 //  This controls whether ESE uses Flush or FUA to make sure a write to disk is durable.

#define JET_paramEnableRBS                      215 // Turns on revert snapshot. Not an ESE flight as we will let the variant be controlled outside ESE (like HA can enable this when lag is disabled)
#define JET_paramRBSFilePath                    216 // path to the revert snapshot directory

#endif // JET_VERSION >= 0x0A01


#define JET_paramMaxValueInvalid                217 //  This is not a valid parameter. It can change from release to release!

// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )

    /* Flags for JET_sesparamIOSessTraceFlags */

#define JET_bitIOSessTraceReads             0x01
#define JET_bitIOSessTraceWrites            0x02
#define JET_bitIOSessTraceHDD               0x04
#define JET_bitIOSessTraceSSD               0x08

#endif // JET_VERSION >= 0x0A01
// begin_PubEsent


//  Session parameters
//
//      JET_sesparamBase                    4096    //  All JET_sesparams designed to be distinct from system / JET_params and JET_dbparams for code defense.

#define JET_sesparamCommitDefault           4097    //  Default grbit for JetCommitTransaction
// end_PubEsent
#define JET_sesparamCommitGenericContext    4098    //  A generic context to be logged with the Commit0 LR
// begin_PubEsent
#if ( JET_VERSION >= 0x0A00 )
#define JET_sesparamTransactionLevel        4099    //  Retrieves (read-only, no set) the current number of nested levels of transactions begun.  0 = not in a transaction.
#define JET_sesparamOperationContext        4100    //  a client context that the engine uses to track and trace operations (such as IOs)
#define JET_sesparamCorrelationID           4101    //  an ID that is logged in traces and can be used by clients to correlate ESE actions with their activity
// end_PubEsent

#if ( JET_VERSION >= 0x0A01 )
#define JET_sesparamCachePriority           4102    //  Cache priority to be assigned to database pages accessed by the session.
                                                    //  See comment next to JET_paramCachePriority for how JET_sesparamCachePriority,
                                                    //  JET_dbparamCachePriority and JET_paramCachePriority interact.

// Store specific trace context parameters
#define JET_sesparamClientComponentDesc     4103
#define JET_sesparamClientActionDesc        4104
#define JET_sesparamClientActionContextDesc 4105
#define JET_sesparamClientActivityId        4106
#define JET_sesparamIOSessTraceFlags        4107
#define JET_sesparamIOPriority              4108    //  Specifies IO Priority flags to use (see JET_IOPriority* flags)

#define JET_sesparamCommitContextContainsCustomerData   4109    //  Boolean value specifying whether the value specified with JET_sesparamCommitGenericContext contains customer data.
#define JET_sesparamCommitContextNeedPreCommitCallback  4110    //  Boolean value specifying whether the application wants pre/post commit callbacks with the generic context.
#endif // JET_VERSION >= 0x0A01

// begin_PubEsent
#define JET_sesparamMaxValueInvalid         4111    //  This is not a valid session parameter. It can change from release to release!

typedef struct
{
    unsigned long   ulUserID;
    unsigned char   nOperationID;
    unsigned char   nOperationType;
    unsigned char   nClientType;
    unsigned char   fFlags;
} JET_OPERATIONCONTEXT;
#endif // JET_VERSION >= 0x0A00

#if ( JET_VERSION >= 0x0600 )

    /* Flags for JET_paramLegacyFileNames */

#define JET_bitESE98FileNames           0x00000001  //  Preserve the .log and .chk extension for compatibility reasons (i.e. Exchange)
#define JET_bitEightDotThreeSoftCompat  0x00000002  //  Preserve the 8.3 naming syntax for as long as possible. (this should not be changed, w/o ensuring there are no log files)
#endif // JET_VERSION >= 0x0600

    /* Flags for JET_paramHungIOActions */

#define JET_bitHungIOEvent                  0x00000001  // Log event when an IO appears to be hung for over the IO threshold.
// end_PubEsent

#define JET_bitHungIOCancel                 0x00000002  // Cancel an IO when an IO appears to be hung for over 2 x the IO threshhold.
#define JET_bitHungIODebug                  0x00000004  // Crash the process when an IO appears to be hung for over 3 x the IO threshhold.
#define JET_bitHungIOEnforce                0x00000008  // Crash the process when an IO appears to be hung for over 3 x the IO threshhold.
#define JET_bitHungIOTimeout                0x00000010  // Failure item when an IO appears to be hung for over 4 x the IO threshhold (considered timed out).

    /* Flags for JET_paramPersistedLostFlushDetection */

#define JET_bitPersistedLostFlushDetectionEnabled           0x00000001  // Enables persisted lost flush detection.
#define JET_bitPersistedLostFlushDetectionCreateNew         0x00000002  // If set, the persisted flush map is re-created on every database attachment.
#define JET_bitPersistedLostFlushDetectionFailOnRuntimeOnly 0x00000004  // If set, lost flush errors are only returned if the flush state was set at runtime.

    /* values for JET_paramReplayThrottlingLevel */

#define JET_ReplayThrottlingNone            0   //  No throttling
#define JET_ReplayThrottlingSleep           1   //  Sleep between replaying log segments which were generated slowly by active

// begin_PubEsent

#if ( JET_VERSION >= 0x0603 )
// Values for JET_paramEnableShrinkDatabase.
#define JET_bitShrinkDatabaseOff            0x0
#define JET_bitShrinkDatabaseOn             0x1     // Uses the file system's Sparse Files feature to release space in the middle of a file.
#define JET_bitShrinkDatabaseRealtime       0x2     // Attempts to reclaim space back to the file system after freeing significant amounts of data (when space is marked as Available to the Root space tree).
// end_PubEsent

// DEPRECATED: this was once used by the instance-wide JET_paramEnableShrinkDatabase parameter.
//             A new value is now defined as a database-wide parameter.
// #define JET_bitShrinkDatabaseEofOnAttach    0x4000  // Resizes the database file during its attachment. All fully available extents at the end of the database are truncated out. It currently does not rearrange data.

#define JET_bitShrinkDatabasePeriodically   0x8000  // Periodically try to trim the database.
// begin_PubEsent

// DEPRECATED:
#define JET_bitShrinkDatabaseTrim           0x1     // DEPRECATED: Deprecated value for JET_bitShrinkDatabaseOn; Will be removed!

#endif // JET_VERSION >= 0x0603

    /* Flags for JetInit2, JetInit3 */

// end_PubEsent
#define JET_bitReplayReplicatedLogFiles     0x00000001  //  OBSOLETE and UNSUPPORTED, current log shipping implementations use JET_bitRecoveryWithoutUndo.
// #define JET_bitCreateSFSVolumeIfNotExist 0x00000002  //  OBSOLETE and UNSUPPORTED, but needed to prevent Exchange Store compilation errors
// begin_PubEsent
#if ( JET_VERSION >= 0x0501 )
#define JET_bitReplayIgnoreMissingDB        0x00000004  //  Ignore missing databases during recovery. This is a very dangerous option and may irrevocably produce an inconsistent database if improperly used. Normal ESE usage does not typically require this dangerous option.
#endif // JET_VERSION >= 0x0501
#if ( JET_VERSION >= 0x0600 )
#define JET_bitRecoveryWithoutUndo          0x00000008  //  perform recovery, but halt at the Undo phase
#define JET_bitTruncateLogsAfterRecovery    0x00000010  //  on successful soft recovery, truncate log files
#define JET_bitReplayMissingMapEntryDB      0x00000020  //  missing database map entry default to same location
#define JET_bitLogStreamMustExist           0x00000040  //  transaction logs must exist in the logfile directory (ie. cannot auto-start a new log stream)
#endif // JET_VERSION >= 0x0600
#if ( JET_VERSION >= 0x0601 )
#define JET_bitReplayIgnoreLostLogs         0x00000080  //  ignore logs lost from the end of the log stream
#endif // JET_VERSION >= 0x0601
#if ( JET_VERSION >= 0x0602 )
// end_PubEsent
#define JET_bitAllowMissingCurrentLog       0x00000100  //  this allows JetInitX() to ignore the fact that we are missing edb.jtx|log and edbtmp.jtx|log
#define JET_bitAllowSoftRecoveryOnBackup    0x00000200  //  this allows JetInitX() to perform soft recovery on a backed up database, essentially implementing hard recovery via JetInitX()
#define JET_bitSkipLostLogsEvent            0x00000400  //  this supresses the event for lost committed logs for HA incremental reseed V1
#define JET_bitExternalRecoveryControl      0x00000800  //  this for absolute control of the recovery process via invoking a callback to be made for all significant state decisions during recovery
// begin_PubEsent
#define JET_bitKeepDbAttachedAtEndOfRecovery 0x00001000 //  this allows db to remain attached at the end of recovery (for faster transition to running state)
#endif // JET_VERSION >= 0x0602
// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )

#define JET_bitReplayIgnoreLogRecordsBeforeMinRequiredLog 0x00002000    //  Ignore log records before the min required log for an attached database.
#define JET_bitReplayInferCheckpointFromRstmapDbs 0x00004000 //   When no checkpoint file is present use databases in restore-map to infer checkpoint instead of starting from oldest log.

/* Flags for JetInit4 JET_RSTMAP2 */

#define JET_bitRestoreMapIgnoreWhenMissing  0x00000001  //  Ignore missing database when replaying an attach or create database during recovery
#define JET_bitRestoreMapFailWhenMissing    0x00000002  //  Fail early on a missing database when replaying an attach or create database during recovery
#define JET_bitRestoreMapOverwriteOnCreate  0x00000004  //  Overwrite existing database when replaying a create database during recovery

#endif // JET_VERSION >= 0x0A01

// begin_PubEsent

    /* Flags for JetTerm2 */

#define JET_bitTermComplete             0x00000001
#define JET_bitTermAbrupt               0x00000002
#define JET_bitTermStopBackup           0x00000004
#if ( JET_VERSION >= 0x0601 )
#define JET_bitTermDirty                0x00000008
#endif // JET_VERSION >= 0x0601
// end_PubEsent
#if ( JET_VERSION >= 0x0602 )
// DEPRECATED: this was once used in the first implementation of DB shrink.
// #define JET_bitTermShrink                0x00000010
#endif // JET_VERSION >= 0x0602
// begin_PubEsent

    /* Flags for JetIdle */

#define JET_bitIdleFlushBuffers         0x00000001
#define JET_bitIdleCompact              0x00000002
#define JET_bitIdleStatus               0x00000004
// end_PubEsent
#define JET_bitIdleVersionStoreTest     0x00000008 /* INTERNAL USE ONLY. call version store consistency check */
#if ( JET_VERSION >= 0x0603 )
// following can only be used in combination with JET_bitIdleCompact
#define JET_bitIdleCompactAsync         0x00000010
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitIdleAvailBuffersStatus   0x00000020  //  Returns JET_wrnIdleFull when database cache available buffers is less than the JET_paramStartFlushThreshold setting.
#define JET_bitIdleWaitForAsyncActivity 0x00000040  //  Waits for all async activity to quiesce. Returns JET_wrnRemainingVersions if there are still pending version store buckets.
#endif // JET_VERSION >= 0x0A01
#endif // JET_VERSION >= 0x0603
// begin_PubEsent

    /* Flags for JetEndSession */

// end_PubEsent
#define JET_bitForceSessionClosed       0x00000001
// begin_PubEsent

    /* Flags for JetAttachDatabase/JetOpenDatabase */

#define JET_bitDbReadOnly               0x00000001
#define JET_bitDbExclusive              0x00000002 /* multiple opens allowed */
// end_PubEsent
#define JET_bitDbSingleExclusive        0x00000002 /* NOT CURRENTLY IMPLEMENTED - currently maps to JET_bitDbExclusive */
// RESERVED                             0x00000008 /* JET_bitDbRecoveryOff defined under JetCreateDatabase() */
// begin_PubEsent
#define JET_bitDbDeleteCorruptIndexes   0x00000010 /* delete indexes possibly corrupted by NT version upgrade */
// end_PubEsent
#define JET_bitDbRebuildCorruptIndexes  0x00000020 /* NOT CURRENTLY IMPLEMENTED - recreate indexes possibly corrupted by NT version upgrade */
// RESERVED                             0x00000040 /* JET_bitDbVersioningOff defined under JetCreateDatabase() */
// begin_PubEsent
#if ( JET_VERSION >= 0x0502 )
#define JET_bitDbDeleteUnicodeIndexes   0x00000400 /* delete all indexes with unicode columns */
#endif // JET_VERSION >= 0x0502
#if ( JET_VERSION >= 0x0501 )
#define JET_bitDbUpgrade                0x00000200 /* */
#endif // JET_VERSION >= 0x0501
#if ( JET_VERSION >= 0x0601 )
#define JET_bitDbEnableBackgroundMaintenance    0x00000800  /* the database engine will initiate automatic background database maintenance */
#endif
#if ( JET_VERSION >= 0x0602 )
#define JET_bitDbPurgeCacheOnAttach     0x00001000 /* used to ensure any kept alive cache is purged for this DB before attach */
#endif
// end_PubEsent
#define bitDbOpenForRecovery            0x00002000 /* internal flag used by recovery */
// begin_PubEsent

    /* Flags for JetDetachDatabase2 */

#if ( JET_VERSION >= 0x0501 )
#define JET_bitForceDetach                  0x00000001
#define JET_bitForceCloseAndDetach          (0x00000002 | JET_bitForceDetach)
#endif // JET_VERSION >= 0x0501

// end_PubEsent

// DEPRECATED: this was once used in the first implementation of DB shrink.
// #define JET_bitDetachShrink                  0x00000004

// begin_PubEsent

    /* Flags for JetCreateDatabase */

#define JET_bitDbRecoveryOff            0x00000008 /* disable logging/recovery for this database */
// end_PubEsent
#define JET_bitDbVersioningOff          0x00000040 /* INTERNAL USE ONLY */
// begin_PubEsent
#define JET_bitDbShadowingOff           0x00000080 /* disable catalog shadowing */
// end_PubEsent
#define JET_bitDbCreateStreamingFile    0x00000100 /* create streaming file with same name as db */
// begin_PubEsent
#if ( JET_VERSION >= 0x0501 )
#define JET_bitDbOverwriteExisting      0x00000200 /* overwrite existing database with same name */
#endif // JET_VERSION >= 0x0501
// end_PubEsent
#define bitCreateDbImplicitly           0x00000400 /* internal use only: create database implicitly (unlogged) */
// RESERVED                             0x00000800 /* JET_bitDbEnableBackgroundMaintenance defined under JetAttachDatabase() */
// begin_PubEsent

    /* Flags for JetBackup, JetBeginExternalBackup, JetBeginExternalBackupInstance, JetBeginSurrogateBackup */

#define JET_bitBackupIncremental        0x00000001
// end_PubEsent
#define JET_bitKeepOldLogs              0x00000002
// begin_PubEsent
#define JET_bitBackupAtomic             0x00000004
// end_PubEsent
#define JET_bitBackupFullWithAllLogs    0x00000008
// begin_PubEsent
#if ( JET_VERSION >= 0x0501 )
#define JET_bitBackupSnapshot           0x00000010
#endif // JET_VERSION >= 0x0501
// end_PubEsent
#if ( JET_VERSION >= 0x0601 )
#define JET_bitBackupSurrogate          0x00000020
#endif // JET_VERSION >= 0x0601
#define JET_bitInternalCopy         0x00000040
// begin_PubEsent

    /* Flags for JetEndExternalBackupInstance2, JetEndSurrogateBackup */

#if ( JET_VERSION >= 0x0501 )
#define JET_bitBackupEndNormal              0x0001
#define JET_bitBackupEndAbort               0x0002
#endif // JET_VERSION >= 0x0501
#if ( JET_VERSION >= 0x0600 )
#define JET_bitBackupTruncateDone           0x0100
#endif // JET_VERSION >= 0x0600
// end_PubEsent
#if ( JET_VERSION >= 0x0601 )
#define JET_bitBackupNoDbHeaderUpdate       0x0200
#endif // JET_VERSION >= 0x0601
// begin_PubEsent

    /* Database types */

#define JET_dbidNil         ((JET_DBID) 0xFFFFFFFF)
// end_PubEsent
#define JET_dbidNoValid     ((JET_DBID) 0xFFFFFFFE) /* used as a flag to indicate that there is no valid dbid */
// begin_PubEsent


    /* Flags for JetCreateTableColumnIndex */
#define JET_bitTableCreateFixedDDL          0x00000001  /* DDL is fixed */
#define JET_bitTableCreateTemplateTable     0x00000002  /* DDL is inheritable (implies FixedDDL) */
#if ( JET_VERSION >= 0x0501 )
#define JET_bitTableCreateNoFixedVarColumnsInDerivedTables  0x00000004
                                                        //  used in conjunction with JET_bitTableCreateTemplateTable
                                                        //  to disallow fixed/var columns in derived tables (so that
                                                        //  fixed/var columns may be added to the template in the future)
#endif // JET_VERSION >= 0x0501
#if JET_VERSION >= 0x0A00
#define JET_bitTableCreateImmutableStructure    0x00000008  // Do not write to the input structures. Additionally, do not return any auto-opened tableid.
#endif // JET_VERSION >= 0x0A00
// end_PubEsent
#define JET_bitTableCreateSystemTable       0x80000000  /*  INTERNAL USE ONLY */
// begin_PubEsent


    /* Flags for JetAddColumn, JetGetColumnInfo, JetOpenTempTable */

#define JET_bitColumnFixed              0x00000001
#define JET_bitColumnTagged             0x00000002
#define JET_bitColumnNotNULL            0x00000004
#define JET_bitColumnVersion                0x00000008
#define JET_bitColumnAutoincrement      0x00000010
#define JET_bitColumnUpdatable          0x00000020 /* JetGetColumnInfo only */
#define JET_bitColumnTTKey              0x00000040 /* JetOpenTempTable only */
#define JET_bitColumnTTDescending       0x00000080 /* JetOpenTempTable only */
#define JET_bitColumnMultiValued            0x00000400
#define JET_bitColumnEscrowUpdate       0x00000800 /* escrow updated, supported coltyps are long and longlong */
#define JET_bitColumnUnversioned        0x00001000 /* for add column only - add column unversioned */
#if ( JET_VERSION >= 0x0501 )
#define JET_bitColumnMaybeNull          0x00002000 /* for retrieve column info of outer join where no match from the inner table */
#define JET_bitColumnFinalize           0x00004000 /* DEPRECATED / Not Fully Implemented: use JET_bitColumnDeleteOnZero instead. */
#define JET_bitColumnUserDefinedDefault 0x00008000 /* default value from a user-provided callback */
// end_PubEsent
#define JET_bitColumnRenameConvertToPrimaryIndexPlaceholder 0x00010000  //  FOR JetRenameColumn() ONLY: rename and convert to primary index placeholder (ie. no longer part of primary index ecxept as a placeholder)
// begin_PubEsent
#endif // JET_VERSION >= 0x0501
#if ( JET_VERSION >= 0x0502 )
#define JET_bitColumnDeleteOnZero       0x00020000 /* When the escrow-update column reaches a value of zero (after all versions are resolve), the record will be deleted. A common use for a column that can be finalized is to use it as a reference count field, and when the field reaches zero the record gets deleted. A Delete-on-zero column must be an escrow update / JET_bitColumnEscrowUpdate column. JET_bitColumnDeleteOnZero cannot be used with JET_bitColumnFinalize. JET_bitColumnDeleteOnZero cannot be used with user defined default columns. */
#endif // JET_VERSION >= 0x0502
// end_PubEsent
#if ( JET_VERSION >= 0x0600 )
// Note: this bit only used in the C# interop provider
#define JET_bitColumnVariable           0x00040000 /* make column a variable length column */
#endif // JET_VERSION >= 0x0600
// begin_PubEsent
#if ( JET_VERSION >= 0x0601 )
#define JET_bitColumnCompressed         0x00080000 /* data in the column can be compressed */
#endif
// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitColumnEncrypted          0x00100000 /* data in the column is encrypted */
#endif
// begin_PubEsent

#if ( JET_VERSION >= 0x0501 )
//  flags for JetDeleteColumn
#define JET_bitDeleteColumnIgnoreTemplateColumns    0x00000001  //  for derived tables, don't bother looking in template columns
#endif // JET_VERSION >= 0x0501


    /* Flags for JetSetCurrentIndex */

#define JET_bitMoveFirst                0x00000000
// end_PubEsent
#define JET_bitMoveBeforeFirst          0x00000001  // unsupported -- DO NOT USE
// begin_PubEsent
#define JET_bitNoMove                   0x00000002

    /* Flags for JetMakeKey */

#define JET_bitNewKey                   0x00000001
#define JET_bitStrLimit                 0x00000002
#define JET_bitSubStrLimit              0x00000004
#define JET_bitNormalizedKey            0x00000008
#define JET_bitKeyDataZeroLength        0x00000010
// end_PubEsent
#if ( JET_VERSION >= 0x0501 )
#define JET_bitKeyOverridePrimaryIndexPlaceholder   0x00000020
#endif // JET_VERSION >= 0x0501

#define JET_maskLimitOptions            0x00000f00
// begin_PubEsent
#if ( JET_VERSION >= 0x0501 )
#define JET_bitFullColumnStartLimit     0x00000100
#define JET_bitFullColumnEndLimit       0x00000200
#define JET_bitPartialColumnStartLimit  0x00000400
#define JET_bitPartialColumnEndLimit    0x00000800
#endif // JET_VERSION >= 0x0501

    /* Flags for JetSetIndexRange */

#define JET_bitRangeInclusive           0x00000001
#define JET_bitRangeUpperLimit          0x00000002
#define JET_bitRangeInstantDuration     0x00000004
#define JET_bitRangeRemove              0x00000008

    /* Flags for JetGetLock */

#define JET_bitReadLock                 0x00000001
#define JET_bitWriteLock                0x00000002

    /* Constants for JetMove */

#define JET_MoveFirst                   (0x80000000)
#define JET_MovePrevious                (-1)
#define JET_MoveNext                    (+1)
#define JET_MoveLast                    (0x7fffffff)

    /* Flags for JetMove */

#define JET_bitMoveKeyNE                0x00000001

    /* Flags for JetSeek */

#define JET_bitSeekEQ                   0x00000001
#define JET_bitSeekLT                   0x00000002
#define JET_bitSeekLE                   0x00000004
#define JET_bitSeekGE                   0x00000008
#define JET_bitSeekGT                   0x00000010
#define JET_bitSetIndexRange            0x00000020
#if ( JET_VERSION >= 0x0502 )
#define JET_bitCheckUniqueness          0x00000040  //  to be used with JET_bitSeekEQ only, returns JET_wrnUniqueKey if seek lands on a key which has no dupes
#endif // JET_VERSION >= 0x0502

#if ( JET_VERSION >= 0x0501 )
    //  Flags for JetGotoSecondaryIndexBookmark
#define JET_bitBookmarkPermitVirtualCurrency    0x00000001  //  place cursor on relative position in index if specified bookmark no longer exists
#endif // JET_VERSION >= 0x0501

    /* Flags for JET_CONDITIONALCOLUMN */
#define JET_bitIndexColumnMustBeNull    0x00000001
#define JET_bitIndexColumnMustBeNonNull 0x00000002

    /* Flags for JET_INDEXRANGE */
#define JET_bitRecordInIndex            0x00000001
#define JET_bitRecordNotInIndex         0x00000002

    /* Flags for JetCreateIndex */

#define JET_bitIndexUnique              0x00000001
#define JET_bitIndexPrimary             0x00000002
#define JET_bitIndexDisallowNull        0x00000004
#define JET_bitIndexIgnoreNull          0x00000008
// end_PubEsent
#define JET_bitIndexClustered40         0x00000010  /*  for backward compatibility */
// begin_PubEsent
#define JET_bitIndexIgnoreAnyNull       0x00000020
#define JET_bitIndexIgnoreFirstNull     0x00000040
#define JET_bitIndexLazyFlush           0x00000080
#define JET_bitIndexEmpty               0x00000100  // don't attempt to build index, because all entries would evaluate to NULL (MUST also specify JET_bitIgnoreAnyNull)
#define JET_bitIndexUnversioned         0x00000200
#define JET_bitIndexSortNullsHigh       0x00000400  // NULL sorts after data for all columns in the index
#define JET_bitIndexUnicode             0x00000800  // LCID field of JET_INDEXCREATE actually points to a JET_UNICODEINDEX struct to allow user-defined LCMapString() flags
#if ( JET_VERSION >= 0x0501 )
#define JET_bitIndexTuples              0x00001000  // index on substring tuples (text columns only)
#endif // JET_VERSION >= 0x0501
#if ( JET_VERSION >= 0x0502 )
#define JET_bitIndexTupleLimits         0x00002000  // cbVarSegMac field of JET_INDEXCREATE actually points to a JET_TUPLELIMITS struct to allow custom tuple index limits (implies JET_bitIndexTuples)
#endif // JET_VERSION >= 0x0502
#if ( JET_VERSION >= 0x0600 )
#define JET_bitIndexCrossProduct        0x00004000  // index over multiple multi-valued columns has full cross product
#define JET_bitIndexKeyMost             0x00008000  // custom index key size set instead of default of 255 bytes
#define JET_bitIndexDisallowTruncation  0x00010000  // fail update rather than truncate index keys
#define JET_bitIndexNestedTable         0x00020000  // index over multiple multi-valued columns but only with values of same itagSequence
#endif // JET_VERSION >= 0x0600
#if ( JET_VERSION >= 0x0602 )
#define JET_bitIndexDotNetGuid          0x00040000  // index over GUID column according to .Net GUID sort order
#endif // JET_VERSION >= 0x602
#if ( JET_VERSION >= 0x0A00 )
#define JET_bitIndexImmutableStructure  0x00080000  // Do not write to the input structures during a JetCreateIndexN call.
#endif // JET_VERSION >= 0x0A00

// end_PubEsent
// These are not persisted anywhere. These are bits used by the 'Isam layer', a simpler C#-based
// interface to access ESE databases.
// 
// #define JET_bitIndexAllowTruncation          0x01000000  // Isam-layer only. Specifies that index keys may be truncated (default in ESE is to allow truncation).

// begin_PubEsent

    /* Flags for index key definition */

#define JET_bitKeyAscending             0x00000000
#define JET_bitKeyDescending            0x00000001

    /* Flags for JetOpenTable */

#define JET_bitTableDenyWrite           0x00000001
#define JET_bitTableDenyRead            0x00000002
#define JET_bitTableReadOnly            0x00000004
#define JET_bitTableUpdatable           0x00000008
#define JET_bitTablePermitDDL           0x00000010  /*  override table flagged as FixedDDL (must be used with DenyRead) */
#define JET_bitTableNoCache             0x00000020  /*  don't cache the pages for this table */
#define JET_bitTablePreread             0x00000040  /*  assume the table is probably not in the buffer cache */
#define JET_bitTableOpportuneRead       0x00000080  /*  attempt to opportunely read physically adjacent leaf pages using larger physical IOs */
// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitTableAllowOutOfDate      0x00000100  /*  allow opening with indexes using out-of-date (but valid) sort versions */
#endif
// begin_PubEsent
#define JET_bitTableSequential          0x00008000  /*  assume the table will be scanned sequentially */
// end_PubEsent
#define JET_bitTableTryPurgeOnClose     0x01000000  /*  INTERNAL USE ONLY: attempt to cleanup resources when table is closed */
#define JET_bitTableAllowSensitiveOperation 0x08000000 /*  INTERNAL USE ONLY */
#define JET_bitTableDelete              0x10000000  /*  INTERNAL USE ONLY */
#define JET_bitTableCreate              0x20000000  /*  INTERNAL USE ONLY */
#define bitTableUpdatableDuringRecovery 0x40000000  /*  INTERNAL USE ONLY */
// begin_PubEsent

#define JET_bitTableClassMask       0x001F0000  /*  table stats class mask  */
#define JET_bitTableClassNone       0x00000000  /*  table belongs to no stats class (default)  */
#define JET_bitTableClass1          0x00010000  /*  table belongs to stats class 1  */
#define JET_bitTableClass2          0x00020000  /*  table belongs to stats class 2  */
#define JET_bitTableClass3          0x00030000  /*  table belongs to stats class 3  */
#define JET_bitTableClass4          0x00040000  /*  table belongs to stats class 4  */
#define JET_bitTableClass5          0x00050000  /*  table belongs to stats class 5  */
#define JET_bitTableClass6          0x00060000  /*  table belongs to stats class 6  */
#define JET_bitTableClass7          0x00070000  /*  table belongs to stats class 7  */
#define JET_bitTableClass8          0x00080000  /*  table belongs to stats class 8  */
#define JET_bitTableClass9          0x00090000  /*  table belongs to stats class 9  */
#define JET_bitTableClass10         0x000A0000  /*  table belongs to stats class 10  */
#define JET_bitTableClass11         0x000B0000  /*  table belongs to stats class 11  */
#define JET_bitTableClass12         0x000C0000  /*  table belongs to stats class 12  */
#define JET_bitTableClass13         0x000D0000  /*  table belongs to stats class 13  */
#define JET_bitTableClass14         0x000E0000  /*  table belongs to stats class 14  */
#define JET_bitTableClass15         0x000F0000  /*  table belongs to stats class 15  */

// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitTableClass16         0x00100000  /*  table belongs to stats class 16  */
#define JET_bitTableClass17         0x00110000  /*  table belongs to stats class 17  */
#define JET_bitTableClass18         0x00120000  /*  table belongs to stats class 18  */
#define JET_bitTableClass19         0x00130000  /*  table belongs to stats class 19  */
#define JET_bitTableClass20         0x00140000  /*  table belongs to stats class 20  */
#define JET_bitTableClass21         0x00150000  /*  table belongs to stats class 21  */
#define JET_bitTableClass22         0x00160000  /*  table belongs to stats class 22  */
#define JET_bitTableClass23         0x00170000  /*  table belongs to stats class 23  */
#define JET_bitTableClass24         0x00180000  /*  table belongs to stats class 24  */
#define JET_bitTableClass25         0x00190000  /*  table belongs to stats class 25  */
#define JET_bitTableClass26         0x001A0000  /*  table belongs to stats class 26  */
#define JET_bitTableClass27         0x001B0000  /*  table belongs to stats class 27  */
#define JET_bitTableClass28         0x001C0000  /*  table belongs to stats class 28  */
#define JET_bitTableClass29         0x001D0000  /*  table belongs to stats class 29  */
#define JET_bitTableClass30         0x001E0000  /*  table belongs to stats class 30  */
#define JET_bitTableClass31         0x001F0000  /*  table belongs to stats class 31  */
#endif // JET_VERSION >= 0x0A01
// begin_PubEsent

#if ( JET_VERSION >= 0x0501 )
#define JET_bitLSReset              0x00000001  /*  reset LS value */
#define JET_bitLSCursor             0x00000002  /*  set/retrieve LS of table cursor */
#define JET_bitLSTable              0x00000004  /*  set/retrieve LS of table */

#define JET_LSNil                   (~(JET_LS)0)
#endif // JET_VERSION >= 0x0501

#if ( JET_VERSION >= 0x0601 )
    /* Flags for JetSetTableSequential and JetPrereadIndexRanges */

#define JET_bitPrereadForward       0x00000001  /*  Hint that the sequential traversal will be in the forward direction */
#define JET_bitPrereadBackward      0x00000002  /*  Hint that the sequential traversal will be in the backward direction */
#if ( JET_VERSION >= 0x0602 )
#define JET_bitPrereadFirstPage     0x00000004  /*  Only first page of long values should be preread */
#define JET_bitPrereadNormalizedKey 0x00000008  /*  Normalized key/bookmark provided instead of column value */
// end_PubEsent
#define bitPrereadSingletonRanges   0x00000010  /*  Internal: All ranges are singleton ranges */
#define bitPrereadDoNotDoOLD2       0x00000020  /*  Internal: Do not perform OLD2 if fragmentation detected, used by delete cleanup task */
// begin_PubEsent
#endif // JET_VERSION >= 0x0602
#endif // JET_VERSION >= 0x0601

    /* Flags for JetOpenTempTable */

#define JET_bitTTIndexed            0x00000001  /* Allow seek */
#define JET_bitTTUnique             0x00000002  /* Remove duplicates */
#define JET_bitTTUpdatable          0x00000004  /* Allow updates */
#define JET_bitTTScrollable         0x00000008  /* Allow backwards scrolling */
#define JET_bitTTSortNullsHigh      0x00000010  /* NULL sorts after data for all columns in the index */
#define JET_bitTTForceMaterialization       0x00000020                      /* Forces temp. table to be materialized into a btree (allows for duplicate detection) */
#if ( JET_VERSION >= 0x0501 )
#define JET_bitTTErrorOnDuplicateInsertion  JET_bitTTForceMaterialization   /* Error always returned when duplicate is inserted (instead of dupe being silently removed) */
#endif // JET_VERSION >= 0x0501
#if ( JET_VERSION >= 0x0502 )
#define JET_bitTTForwardOnly        0x00000040  /* Prevents temp. table from being materialized into a btree (and enables duplicate keys) */
#endif // JET_VERSION >= 0x0502
#if ( JET_VERSION >= 0x0601 )
#define JET_bitTTIntrinsicLVsOnly   0x00000080  //  permit only intrinsic LV's (so materialisation is not required simply because a TT has an LV column)
#endif // JET_VERSION >= 0x0601
#if ( JET_VERSION >= 0x0602 )
#define JET_bitTTDotNetGuid         0x00000100  //  sort all JET_coltypGUID columns according to .Net Guid sort order
#endif // JET_VERSION >= 0x0601


    /* Flags for JetSetColumn */

#define JET_bitSetAppendLV                  0x00000001
#define JET_bitSetOverwriteLV               0x00000004 /* overwrite JET_coltypLong* byte range */
#define JET_bitSetSizeLV                    0x00000008 /* set JET_coltypLong* size */
#define JET_bitSetZeroLength                0x00000020
#define JET_bitSetSeparateLV                0x00000040 /* force LV separation */
#define JET_bitSetUniqueMultiValues         0x00000080 /* prevent duplicate multi-values */
#define JET_bitSetUniqueNormalizedMultiValues   0x00000100 /* prevent duplicate multi-values, normalizing all data before performing comparisons */
#if ( JET_VERSION >= 0x0501 )
#define JET_bitSetRevertToDefaultValue      0x00000200 /* if setting last tagged instance to NULL, revert to default value instead if one exists */
#define JET_bitSetIntrinsicLV               0x00000400 /* store whole LV in record without bursting or return an error */
#endif // JET_VERSION >= 0x0501
#if ( JET_VERSION >= 0x0601 )
#define JET_bitSetUncompressed              0x00010000 /* don't attempt compression when storing the data */
#define JET_bitSetCompressed                0x00020000 /* attempt compression when storing the data */
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitSetContiguousLV              0x00040000 /* Allocates the long-value across contiguous pages (at potentialy space saving costs) for better IO behavior. Valid only with JET_bitSetSeparateLV. Invalid (or not implemented) with certain long-value operations such as replace, and certain column options such as compression. Use across many varying LVs sizes may cause space fragmentation / allocation issues. */
#endif // JET_VERSION >= 0x0A01
#endif // JET_VERSION >= 0x0601


#if ( JET_VERSION >= 0x0601 )
    /*  Space Hint Flags / JET_SPACEHINTS   */

//  Generic 
#define JET_bitSpaceHintsUtilizeParentSpace         0x00000001  //  This changes the internal allocation policy to get space hierarchically from a B-Tree's immediate parent.
//  Create
#define JET_bitCreateHintAppendSequential           0x00000002  //  This bit will enable Append split behavior to grow according to the growth dynamics of the table (set by cbMinExtent, ulGrowth, cbMaxExtent).
#define JET_bitCreateHintHotpointSequential         0x00000004  //  This bit will enable Hotpoint split behavior to grow according to the growth dynamics of the table (set by cbMinExtent, ulGrowth, cbMaxExtent).
//  Retrieve
#define JET_bitRetrieveHintReserve1                 0x00000008  //  Reserved and ignored
#define JET_bitRetrieveHintTableScanForward         0x00000010  //  By setting this the client indicates that forward sequential scan is the predominant usage pattern of this table (causing B+ Tree defrag to be auto-triggered to clean it up if fragmented).
#define JET_bitRetrieveHintTableScanBackward        0x00000020  //  By setting this the client indicates that backwards sequential scan is the predominant usage pattern of this table (causing B+ Tree defrag to be auto-triggered to clean it up if fragmented).
#define JET_bitRetrieveHintReserve2                 0x00000040  //  Reserved and ignored
#define JET_bitRetrieveHintReserve3                 0x00000080  //  Reserved and ignored
//  Update
//#define JET_bitUpdateReserved                     0x00000000  //  TBD.
//  Delete
#define JET_bitDeleteHintTableSequential            0x00000100  //  This means that the application expects this table to be cleaned up in-order sequentially (from lowest key to highest key)
#endif // JET_VERSION >= 0x0601

#if ( JET_VERSION >= 0x0A01 )
// end_PubEsent
#define JET_bitSpaceHintsUtilizeExactExtents        0x00000200  //  This changes the internal allocation policy to always allocate extents of the size requested by the space hints.
// begin_PubEsent
#endif // JET_VERSION >= 0x0A01


    /*  Set column parameter structure for JetSetColumns */

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

#endif // JET_VERSION >= 0x0501

    /* Options for JetPrepareUpdate */

#define JET_prepInsert                      0
#define JET_prepReplace                     2
#define JET_prepCancel                      3
#define JET_prepReplaceNoLock               4
#define JET_prepInsertCopy                  5
// end_PubEsent
// #define JET_prepInsertCopyWithoutSLVColumns  6   //  same as InsertCopy, except that SLV columns are nullified instead of copied in the new record */
// begin_PubEsent
#if ( JET_VERSION >= 0x0501 )
#define JET_prepInsertCopyDeleteOriginal    7   //  used for updating a record in the primary key; avoids the delete/insert process and updates autoinc */
#endif // JET_VERSION >= 0x0501
// end_PubEsent
#define JET_prepReadOnlyCopy                8   //  copy record into copy buffer for read-only purposes
// begin_PubEsent
#if ( JET_VERSION >= 0x0603 )
#define JET_prepInsertCopyReplaceOriginal   9   //  used for updating a record in the primary key; avoids the delete/insert process and keeps autoinc */
#endif // JET_VERSION >= 0x0603

#if ( JET_VERSION >= 0x0603 )
// Values for JET_paramEnableSqm
#define JET_sqmDisable                      0   //  Explicitly disable SQM
#define JET_sqmEnable                       1   //  Explicitly enable SQM
#define JET_sqmFromCEIP                     2   //  Enables SQM based on Customer Experience Improvement Program opt-in
#endif // JET_VERSION >= 0x0603

    //  Flags for JetUpdate
#if ( JET_VERSION >= 0x0502 )
#define JET_bitUpdateCheckESE97Compatibility    0x00000001  //  check whether record fits if represented in ESE97 database format
#endif // JET_VERSION >= 0x0502
// end_PubEsent
#define JET_bitUpdateNoVersion                  0x00000002  //  do not create rollback or versioning information for update
// begin_PubEsent

    /* Flags for JetEscrowUpdate */
#define JET_bitEscrowNoRollback             0x0001

    /* Flags for JetRetrieveColumn */

#define JET_bitRetrieveCopy                 0x00000001
#define JET_bitRetrieveFromIndex            0x00000002
#define JET_bitRetrieveFromPrimaryBookmark  0x00000004
#define JET_bitRetrieveTag                  0x00000008
#define JET_bitRetrieveNull                 0x00000010  /*  for columnid 0 only */
#define JET_bitRetrieveIgnoreDefault        0x00000020  /*  for columnid 0 only */
// end_PubEsent
#define JET_bitRetrieveLongId               0x00000040
#define JET_bitRetrieveLongValueRefCount    0x00000080  /*  for testing use only */
// #define JET_bitRetrieveSLVAsSLVInfo          0x00000100  /*  internal use only  */

    /* Flags for JetRetrieveColumn when the SLV Provider is enabled  */

// #define JET_bitRetrieveSLVAsSLVFile          0x00000200 /* retrieve SLV as an SLV File handle */
// #define JET_bitRetrieveSLVAsSLVEA            0x00000400 /* retrieve SLV as an SLV EA list */
// begin_PubEsent
#if ( JET_VERSION >= 0x0600 )
#define JET_bitRetrieveTuple                0x00000800 /* retrieve tuple fragment from index */
#endif // JET_VERSION >= 0x0600
// end_PubEsent
#if ( JET_VERSION >= 0x0601 )
#define JET_bitRetrievePageNumber           0x00001000 /* page number list for column */
#endif // JET_VERSION >= 0x0600

#define JET_bitRetrieveCopyIntrinsic        0x00002000  /*  retrieves size of data that can be added to a record before offloading LONG columns.  Fixed sized columns return 0 or column size. */

//  Has no effect on non-separate long value retrievals.  On separated long values,
//  initiate read of separate long value data without waiting for read to complete.  
//  cbActual will be 0 becuase no data is read for separate LVs.
//  Currently only reads one chunk at ibOffset given and will not read all data based on cbMax. 
//  If cbMax greater than single chunk given then JET_wrnNyi returned.
//
#define JET_bitRetrievePrereadOnly          0x00004000
//  Causes more pages to be preread than might be needed in an effort to improve performance.
//
#define JET_bitRetrievePrereadMany          0x00008000  
#if ( JET_VERSION >= 0x0603 )
#define JET_bitRetrievePhysicalSize         0x00010000  /* retrieves compressed size only in cbActual; no data is retrieved */
#endif // JET_VERSION >= 0x0603
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitRetrieveAsRefIfNotInRecord   0x00020000 /* Retrieves the column value as a reference if it is not in the record. */
#endif // JET_VERSION >= 0x0A01

// begin_PubEsent

#if ( JET_VERSION >= 0x0602 )
    /* Flags for JET_INDEX_COLUMN */
#define JET_bitZeroLength                   0x00000001
#endif

    /* Retrieve column parameter structure for JetRetrieveColumns */

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


// end_PubEsent
typedef struct
{
    JET_COLUMNID            columnid;
    unsigned short          cMultiValues;

    union
    {
        unsigned short      usFlags;
        struct
        {
            unsigned short  fLongValue:1;           //  is column LongText/Binary?
            unsigned short  fDefaultValue:1;        //  was a default value retrieved?
            unsigned short  fNullOverride:1;        //  was there an explicit null to override a default value?
            unsigned short  fDerived:1;             //  was column derived from template table?
        };
    };
} JET_RETRIEVEMULTIVALUECOUNT;
// begin_PubEsent


#if ( JET_VERSION >= 0x0501 )
    /* Flags for JetEnumerateColumns */

#define JET_bitEnumerateCopy                        JET_bitRetrieveCopy
#define JET_bitEnumerateIgnoreDefault               JET_bitRetrieveIgnoreDefault
// end_PubEsent
#define JET_bitEnumerateLocal                       0x00010000
// begin_PubEsent
#define JET_bitEnumeratePresenceOnly                0x00020000
#define JET_bitEnumerateTaggedOnly                  0x00040000
#define JET_bitEnumerateCompressOutput              0x00080000
#if ( JET_VERSION >= 0x0502 )
// Available on Server 2003 SP1
#define JET_bitEnumerateIgnoreUserDefinedDefault    0x00100000
#endif // JET_VERSION >= 0x0502
#if ( JET_VERSION >= 0x0601 )
#define JET_bitEnumerateInRecordOnly                0x00200000
#endif // JET_VERSION >= 0x0601
// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitEnumerateAsRefIfNotInRecord          0x00400000 /* Retrieves the column value as a reference if it is not in the record. */
#endif // JET_VERSION >= 0x0A01
// begin_PubEsent

    /* Parameter structures for JetEnumerateColumns */

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
        struct /* err != JET_wrnColumnSingleValue */
        { 
            unsigned long           cEnumColumnValue;
            JET_ENUMCOLUMNVALUE*    rgEnumColumnValue;
        };
        struct /* err == JET_wrnColumnSingleValue */
        {
            unsigned long           cbData;
            void*                   pvData;
        };
    };
} JET_ENUMCOLUMN;

    /* Realloc callback for JetEnumerateColumns */

typedef void* (JET_API *JET_PFNREALLOC)(
    _In_opt_ void *     pvContext,
    _In_opt_ void *     pv,
    _In_ unsigned long  cb );

#endif // JET_VERSION >= 0x0501

// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )

/* Flags for JetStreamRecords */

#define JET_bitStreamForward            0x00000001  /*  Stream records in ascending key order. */
#define JET_bitStreamBackward           0x00000002  /*  Stream records in descending key order. */
#define JET_bitStreamColumnReferences   0x00000004  /*  Stream column values as column references when not in the record. */

#endif // JET_VERSION >= 0x0A01
// begin_PubEsent


#if ( JET_VERSION >= 0x0600 )
    /* Flags for JetGetRecordSize */

#define JET_bitRecordSizeInCopyBuffer           0x00000001  //  use record in copy buffer
#define JET_bitRecordSizeRunningTotal           0x00000002  //  increment totals in output buffer instead of setting them
#define JET_bitRecordSizeLocal                  0x00000004  //  ignore Long Values (and other data otherwise not in the same page as the record)
// end_PubEsent
// These will remain private until we need them.
#define JET_bitRecordSizeIncludeDefaultValues   0x00000008  //  compute size of default-valued columns (CURRENTLY UNSUPPORTED)
#define JET_bitRecordSizeSecondaryIndexKeyOnly  0x00000010  //  compute size of secondary index key instead of record size (CURRENTLY UNSUPPORTED)
#define JET_bitRecordSizeIntrinsicPhysicalOnly  0x00000020  //  only get physical size for intrinsic columns (cheaper)
#define JET_bitRecordSizeExtrinsicLogicalOnly   0x00000040  //  only get logical size for extrinsic columns (cheaper)
// begin_PubEsent

    /* parameter structures for JetGetRecordSize */

typedef struct
{
    unsigned __int64    cbData;                 //  user data in record
    unsigned __int64    cbLongValueData;        //  user data associated with the record but stored in the long-value tree (NOTE: does NOT count intrinsic long-values)
    unsigned __int64    cbOverhead;             //  record overhead
    unsigned __int64    cbLongValueOverhead;    //  overhead of long-value data (NOTE: does not count intrinsic long-values)
    unsigned __int64    cNonTaggedColumns;      //  total number of fixed/variable columns
    unsigned __int64    cTaggedColumns;         //  total number of tagged columns
    unsigned __int64    cLongValues;            //  total number of values stored in the long-value tree for this record (NOTE: does NOT count intrinsic long-values)
    unsigned __int64    cMultiValues;           //  total number of values beyond the first for each column in the record
} JET_RECSIZE;
#endif // JET_VERSION >= 0x0600


// end_PubEsent
#if ( JET_VERSION >= 0x0600 )
typedef struct tagJET_PAGEINFO
{
    unsigned long       pgno;                   //  pgno for the page. must be passed in
    unsigned long       fPageIsInitialized:1;   //  false if the page is zeroed
    unsigned long       fCorrectableError:1;    //  correctable error found on page
    unsigned __int64    checksumActual;         //  checksum stored on the page
    unsigned __int64    checksumExpected;       //  checksum expected for the page
    unsigned __int64    dbtime;                 //  dbtime on the page
    unsigned __int64    structureChecksum;      //  checksum of the page structure
    unsigned __int64    flags;                  //  currently unused
} JET_PAGEINFO;
#endif // JET_VERSION >= 0x0600

// begin_PubEsent
#if ( JET_VERSION >= 0x0601 )
typedef struct
{
    unsigned __int64    cbData;                 //  user data in record
    unsigned __int64    cbLongValueData;        //  user data associated with the record but stored in the long-value tree (NOTE: does NOT count intrinsic long-values)
    unsigned __int64    cbOverhead;             //  record overhead
    unsigned __int64    cbLongValueOverhead;    //  overhead of long-value data (NOTE: does not count intrinsic long-values)
    unsigned __int64    cNonTaggedColumns;      //  total number of fixed/variable columns
    unsigned __int64    cTaggedColumns;         //  total number of tagged columns
    unsigned __int64    cLongValues;            //  total number of values stored in the long-value tree for this record (NOTE: does NOT count intrinsic long-values)
    unsigned __int64    cMultiValues;           //  total number of values beyond the first for each column in the record
    unsigned __int64    cCompressedColumns;     //  total number of columns which are compressed
    unsigned __int64    cbDataCompressed;       //  compressed size of user data in record (same as cbData if no intrinsic long-values are compressed)
    unsigned __int64    cbLongValueDataCompressed;  // compressed size of user data in the long-value tree (same as cbLongValue data if no separated long values are compressed)
} JET_RECSIZE2;
#endif // JET_VERSION >= 0x0601
// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )
typedef struct
{
    unsigned __int64    cbData;                 //  user data in record
    unsigned __int64    cbLongValueData;        //  user data associated with the record but stored in the long-value tree (NOTE: does NOT count intrinsic long-values)
    unsigned __int64    cbOverhead;             //  record overhead
    unsigned __int64    cbLongValueOverhead;    //  overhead of long-value data (NOTE: does not count intrinsic long-values)
    unsigned __int64    cNonTaggedColumns;      //  total number of fixed/variable columns
    unsigned __int64    cTaggedColumns;         //  total number of tagged columns
    unsigned __int64    cLongValues;            //  total number of values stored in the long-value tree for this record (NOTE: does NOT count intrinsic long-values)
    unsigned __int64    cMultiValues;           //  total number of values beyond the first for each column in the record
    unsigned __int64    cCompressedColumns;     //  total number of columns which are compressed
    unsigned __int64    cbDataCompressed;       //  compressed size of user data in record (same as cbData if no intrinsic long-values are compressed)
    unsigned __int64    cbLongValueDataCompressed;  // compressed size of user data in the long-value tree (same as cbLongValue data if no separated long values are compressed)
    unsigned __int64    cbIntrinsicLongValueData;   // user data stored in intrinsic LVs (in the record).
    unsigned __int64    cbIntrinsicLongValueDataCompressed;   // compressed size of user data stored in intrinsic LVs (in the record).
    unsigned __int64    cIntrinsicLongValues;       // total number of intrinsic LVs stored in the record.
    unsigned __int64    cbKey;                  //  Key size in bytes. Doesn't include storage overhead. Does include key normalization overhead.
} JET_RECSIZE3;

#endif // JET_VERSION >= 0x0A01

#if ( JET_VERSION >= 0x0601 )
typedef struct tagJET_PAGEINFO2
{
    JET_PAGEINFO        pageInfo;
    unsigned __int64    rgChecksumActual[ 3 ];  //  more checksum stored on the page
    unsigned __int64    rgChecksumExpected[ 3]; //  more checksum expected for the page
} JET_PAGEINFO2;
#endif // JET_VERSION >= 0x0601

// begin_PubEsent

#pragma warning(pop)        //  nonstandard extension used : nameless struct/union


    /* Flags for JetBeginTransaction2 */

#if ( JET_VERSION >= 0x0501 )
#define JET_bitTransactionReadOnly      0x00000001  /* transaction will not modify the database */
#endif // JET_VERSION >= 0x0501
// end_PubEsent
#define JET_bitDistributedTransaction   0x00000002  /* transaction will require two-phase commit */
#define bitTransactionWritableDuringRecovery 0x00000004 /* internal updatable transaction during recovery */
// begin_PubEsent

    /* Flags for JetCommitTransaction */

#define JET_bitCommitLazyFlush          0x00000001  /* lazy flush log buffers. */
#define JET_bitWaitLastLevel0Commit     0x00000002  /* wait for last level 0 commit record flushed */
#if ( JET_VERSION >= 0x0502 )
#define JET_bitWaitAllLevel0Commit      0x00000008  /* wait for all level 0 commits to be flushed */
#endif // JET_VERSION >= 0x0502
#if ( JET_VERSION >= 0x0601 )
#define JET_bitForceNewLog              0x00000010
#endif // JET_VERSION >= 0x0601
// end_PubEsent
#if ( JET_VERSION >= 0x0A00 )
#define JET_bitCommitRedoCallback       0x00000020
#endif // JET_VERSION >= 0x0A00

#define JET_bitCommitFlush_OLD          0x00000001  /* commit and flush page buffers. */
#define JET_bitCommitLazyFlush_OLD      0x00000004  /* lazy flush log buffers. */
#define JET_bitWaitLastLevel0Commit_OLD 0x00000010  /* wait for last level 0 commit record flushed */
// begin_PubEsent

    /* Flags for JetRollback */

#define JET_bitRollbackAll              0x00000001
// end_PubEsent
#if ( JET_VERSION >= 0x0A00 )
#define JET_bitRollbackRedoCallback     0x00000020
#endif // JET_VERSION >= 0x0A00
// begin_PubEsent


#if ( JET_VERSION >= 0x0600 )
    /* Flags for JetOSSnapshot APIs */

    /* Flags for JetOSSnapshotPrepare */
#define JET_bitIncrementalSnapshot      0x00000001  /* bit 0: full (0) or incremental (1) snapshot */
#define JET_bitCopySnapshot             0x00000002  /* bit 1: normal (0) or copy (1) snapshot */
#define JET_bitContinueAfterThaw        0x00000004  /* bit 2: end on thaw (0) or wait for [truncate +] end snapshot */
#if ( JET_VERSION >= 0x0601 )
#define JET_bitExplicitPrepare          0x00000008  /* bit 3: all instaces prepared by default (0) or no instance prepared by default (1)  */
#endif // JET_VERSION >= 0x0601

    /* Flags for JetOSSnapshotTruncateLog & JetOSSnapshotTruncateLogInstance */
#define JET_bitAllDatabasesSnapshot     0x00000001  /* bit 0: there are detached dbs in the instance (i.e. can't truncate logs) */

    /* Flags for JetOSSnapshotEnd */
#define JET_bitAbortSnapshot            0x00000001  /* snapshot process failed */
#endif // JET_VERSION >= 0x0600
// end_PubEsent

#if ( JET_VERSION >= 0x0601 )

    /* Flags for JET_EMITDATACTX and used by JET_PFNEMITLOGDATA and JetConsumeLogData */

#define JET_bitShadowLogEmitFirstCall           0x00000001  //  the very first emit has only this bit set, log data will likely follow next
#define JET_bitShadowLogEmitLastCall            0x00000002  //  the very last emit has only this bit set, no more log data will follow
#define JET_bitShadowLogEmitCancel              0x00000004  //  future: user requested this be cancelled via resetting the JET_param to NULL
#define JET_bitShadowLogEmitDataBuffers     0x00000008  //  callback emits some portion of the log buffer and position in log
#define JET_bitShadowLogEmitLogComplete     0x00000010  //  callback emits signal that the current log file is completed

    /* Information context surrounded data emitted from JET_PFNEMITLOGDATA */

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
// 40 bytes

    /* Callback for JET_param JET_paramEmitLogDataCallback */

typedef JET_ERR (JET_API * JET_PFNEMITLOGDATA)(
    _In_    JET_INSTANCE        instance,
    _In_    JET_EMITDATACTX *   pEmitLogDataCtx,
    _In_    void *              pvLogData,
    _In_    unsigned long       cbLogData,
    _In_    void *              callbackCtx );

#endif // JET_VERSION >= 0x0601

// begin_PubEsent
    /* Info parameter for JetGetDatabaseInfo and JetGetDatabaseFileInfo */

#define JET_DbInfoFilename          0
#define JET_DbInfoConnect           1
#define JET_DbInfoCountry           2   //  retrieves the default country/region
#if ( JET_VERSION >= 0x0501 )
#define JET_DbInfoLCID              3
#endif // JET_VERSION >= 0x0501
#define JET_DbInfoLangid            3       // OBSOLETE: use JET_DbInfoLCID instead
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
#define JET_DbInfoHasSLVFile_Obsolete       16  // not_PubEsent
#define JET_DbInfoPageSize          17
#endif // JET_VERSION >= 0x0501
// end_PubEsent
#define JET_DbInfoStreamingFileSpace_Obsolete       18  //  SLV owned and available space (may be slow because this sequentially scans the SLV space tree)
// begin_PubEsent
#if ( JET_VERSION >= 0x0600 )
#define JET_DbInfoFileType          19
// end_PubEsent
#define JET_DbInfoStreamingFileSize_Obsolete        20      //  SLV owned space only (fast because it does NOT scan the SLV space tree)
// begin_PubEsent
#if ( JET_VERSION >= 0x603 )
#define JET_DbInfoFilesizeOnDisk    21
#endif
// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )
#define dbInfoSpaceShelved          22  /*  INTERNAL USE ONLY */
#endif
#define JET_DbInfoUseCachedResult   0x40000000

    /* Info parameter for JetGetLogFileInfo */

#define JET_LogInfoMisc             0
#if ( JET_VERSION >= 0x0601 )
#define JET_LogInfoMisc2            1
#if ( JET_VERSION >= 0x0A01 )
#define JET_LogInfoMisc3            2
#endif
#endif

/* Info parameter for JetGetRBSFileInfo */

#define JET_RBSFileInfoMisc              0

// begin_PubEsent

#endif // JET_VERSION >= 0x0600

    /* Dbstates from JetGetDatabaseFileInfo */

#define JET_dbstateJustCreated                  1
#define JET_dbstateDirtyShutdown                2
#define JET_dbstateCleanShutdown                3
#define JET_dbstateBeingConverted               4
#if ( JET_VERSION >= 0x0501 )
#define JET_dbstateForceDetach                  5
#endif // JET_VERSION >= 0x0501
// end_PubEsent
#if ( JET_VERSION >= 0x0601 )
#define JET_dbstateIncrementalReseedInProgress  6
#define JET_dbstateDirtyAndPatchedShutdown      7   // Database has extensive multi-log recovery requirements due to some form of database page patching.
#endif // JET_VERSION >= 0x0601
#if ( JET_VERSION >= 0x0A01 )
#define JET_dbstateRevertInProgress             8   // Database is currently being revert to a previous state using the revert snapshots.
#endif // JET_VERSION >= 0x0A01
// begin_PubEsent

#if ( JET_VERSION >= 0x0600 )

    //  supported file types (returned from JetGetDatabaseFileInfo with JET_DbInfoFileType)

#define JET_filetypeUnknown                 0
#define JET_filetypeDatabase                1
// end_PubEsent
#define JET_filetypeStreamingFile           2
// begin_PubEsent
#define JET_filetypeLog                     3
#define JET_filetypeCheckpoint              4
#define JET_filetypeTempDatabase            5
// end_PubEsent
#define JET_filetypeFTL                     6
// begin_PubEsent
#define JET_filetypeFlushMap                7
// end_PubEsent
#define JET_filetypeCachedFile              8
#define JET_filetypeCachingFile             9
#define JET_filetypeSnapshot                10
#define JET_filetypeRBSRevertCheckpoint     11
#define JET_filetypeMax                     12
// begin_PubEsent

#endif // JET_VERSION >= 0x0600

    /* Column data types */

#define JET_coltypNil               0
#define JET_coltypBit               1   /* True, False, or NULL */
#define JET_coltypUnsignedByte      2   /* 1-byte integer, unsigned */
#define JET_coltypShort             3   /* 2-byte integer, signed */
#define JET_coltypLong              4   /* 4-byte integer, signed */
#define JET_coltypCurrency          5   /* 8 byte integer, signed */
#define JET_coltypIEEESingle        6   /* 4-byte IEEE single precision */
#define JET_coltypIEEEDouble        7   /* 8-byte IEEE double precision */
#define JET_coltypDateTime          8   /* Integral date, fractional time */
#define JET_coltypBinary            9   /* Binary data, < 255 bytes */
#define JET_coltypText              10  /* ANSI text, case insensitive, < 255 bytes */
#define JET_coltypLongBinary        11  /* Binary data, long value */
#define JET_coltypLongText          12  /* ANSI text, long value */

// Pre XP
#if ( JET_VERSION < 0x0501 )
#define JET_coltypMax               13  /* the number of column types  */
                                        /* used for validity tests and */
                                        /* array declarations.         */
#endif // JET_VERSION < 0x0501

// Windows XP
#if ( JET_VERSION >= 0x0501 )
#define JET_coltypSLV               13  /* SLV's. Obsolete. */

#if ( JET_VERSION < 0x0600 )
#define JET_coltypMax               14  /* the number of column types  */
                                        /* used for validity tests and */
                                        /* array declarations.         */
#endif // JET_VERSION == 0x0501

#endif // JET_VERSION >= 0x0501

// Windows Vista to Windows 8.1
#if ( JET_VERSION >= 0x0600 )
#define JET_coltypUnsignedLong      14  /* 4-byte unsigned integer */
#define JET_coltypLongLong          15  /* 8-byte signed integer */
#define JET_coltypGUID              16  /* 16-byte globally unique identifier */
#define JET_coltypUnsignedShort     17  /* 2-byte unsigned integer */

#if ( JET_VERSION >= 0x0600 && JET_VERSION <= 0x0603 )
#define JET_coltypMax               18  /* the number of column types  */
                                        /* used for validity tests and */
                                        /* array declarations.         */
#endif // ( JET_VERSION >= 0x0600 && JET_VERSION <= 0x0603 )

#endif // JET_VERSION >= 0x0600

// Windows 10
#if ( JET_VERSION >= 0x0A00 )
#define JET_coltypUnsignedLongLong  18  /* 8-byte unsigned integer */
#define JET_coltypMax               19  /* the number of column types  */
                                        /* used for validity tests and */
                                        /* array declarations.         */
#endif // JET_VERSION >= 0x0A00

// end_PubEsent

    /* RBS revert states */

#if ( JET_VERSION >= 0x0A01 )
#define JET_revertstateNone                 0   // Revert has not yet started/default state.
#define JET_revertstateInProgress           1   // Revert snapshots are currently being applied to the databases.
#define JET_revertstateCopingLogs           2   // The required logs to bring databases to a clean state are being copied to the log directory after revert.
#define JET_revertstateBackupSnapshot       3   // Backs up revert snapshots for investigation purposes.
#define JET_revertstateRemoveSnapshot       4   // Removes the snapshot which have been applied to the databases and backed up.
#endif // JET_VERSION >= 0x0A01

    /* RBS revert grbits */

#if ( JET_VERSION >= 0x0A01 )
#define JET_bitDeleteAllExistingLogs  0x00000001  /* Delete all the existing log files at the end of revert. */
#endif // JET_VERSION >= 0x0A01

#if ( JET_VERSION >= 0x0600 )
        /* Info levels for JetGetSessionInfo */

#define JET_SessionInfo             0U
#endif // !JET_VERSION >= 0x0600
// begin_PubEsent

    /* Info levels for JetGetObjectInfo */

#define JET_ObjInfo                 0U
#define JET_ObjInfoListNoStats      1U
#define JET_ObjInfoList             2U
#define JET_ObjInfoSysTabCursor     3U
#define JET_ObjInfoListACM          4U /* Blocked by JetGetObjectInfo */
#define JET_ObjInfoNoStats          5U
#define JET_ObjInfoSysTabReadOnly   6U
#define JET_ObjInfoRulesLoaded      7U
#define JET_ObjInfoMax              8U

    /* Info levels for JetGetTableInfo/JetSetTableInfo */

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
#define JET_TblInfoSpaceOwned   10U                 // OwnExt
#define JET_TblInfoSpaceAvailable       11U         // AvailExt
#define JET_TblInfoTemplateTableName    12U
// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )
#define JET_TblInfoLVChunkMax   13U
#define JET_TblInfoEncryptionKey    14U
#endif
// begin_PubEsent

    /* Info levels for JetGetIndexInfo and JetGetTableIndexInfo */

#define JET_IdxInfo                 0U
#define JET_IdxInfoList             1U
#define JET_IdxInfoSysTabCursor     2U      //  OBSOLETE and unused.
#define JET_IdxInfoOLC              3U      //  OBSOLETE and unused.
#define JET_IdxInfoResetOLC         4U      //  OBSOLETE and unused.
#define JET_IdxInfoSpaceAlloc       5U
#if ( JET_VERSION >= 0x0501 )
#define JET_IdxInfoLCID             6U
#endif // JET_VERSION >= 0x0501
#define JET_IdxInfoLangid           6U      //  OBSOLETE: use JET_IdxInfoLCID instead
#define JET_IdxInfoCount            7U
#define JET_IdxInfoVarSegMac        8U
#define JET_IdxInfoIndexId          9U
#if ( JET_VERSION >= 0x0600 )
#define JET_IdxInfoKeyMost          10U
#endif // JET_VERSION >= 0x0600
#if ( JET_VERSION >= 0x0601 )
#define JET_IdxInfoCreateIndex      11U     //  return a JET_INDEXCREATE structure suitable for use by JetCreateIndex2()
#define JET_IdxInfoCreateIndex2     12U     //  return a JET_INDEXCREATE2 structure suitable for use by JetCreateIndex3()
#endif // JET_VERSION >= 0x0601
#if ( JET_VERSION >= 0x0602 )
#define JET_IdxInfoCreateIndex3     13U     //  return a JET_INDEXCREATE3 structure suitable for use by JetCreateIndex4()
#define JET_IdxInfoLocaleName       14U     //  Returns the locale name, which can be a wide string of up to LOCALE_NAME_MAX_LENGTH (including null).
#endif // JET_VERSION >= 0x0602
// end_PubEsent
// These bits aren't public at the moment. Not because they are confidential, but
// because it's leaking OS implementation details. If they are useful to clients,
// then they should be exposed.
#if ( JET_VERSION >= 0x0A00 )
#define JET_IdxInfoSortVersion      15U     //  Returns a 32-bit integer representing the sort version for Unicode indices. (NLSVERSIONINFO.dwNLSVersion)
#define JET_IdxInfoDefinedSortVersion   16U     //  Returns a 32-bit integer representing the sort version for Unicode indices. (NLSVERSIONINFO.dwDefinedVersion). Only relevant on pre-Windows8.
#define JET_IdxInfoSortId           17U     //  Returns a Sort ID (GUID) used for sorting Unicode text.
#endif // JET_VERSION >= 0x0A00
// begin_PubEsent


    /* Info levels for JetGetColumnInfo and JetGetTableColumnInfo */

#define JET_ColInfo                 0U
#define JET_ColInfoList             1U
#define JET_ColInfoSysTabCursor     3U
#define JET_ColInfoBase             4U
#define JET_ColInfoListCompact      5U      //  INTERNAL USE ONLY
#if ( JET_VERSION >= 0x0501 )
#define JET_ColInfoByColid          6U
#define JET_ColInfoListSortColumnid 7U      //  OBSOLETE: use grbit instead
#endif // JET_VERSION >= 0x0501
#if ( JET_VERSION >= 0x0600 )
#define JET_ColInfoBaseByColid      8U
#endif // JET_VERSION >= 0x0600

#if ( JET_VERSION >= 0x0600 )

        // Grbits for JET_GetColumnInfo and JetGetTableColumnInfo (OR together with the info level)
#define JET_ColInfoGrbitNonDerivedColumnsOnly   0x80000000  //  for lists, only return non-derived columns (if the table is derived from a template)
#define JET_ColInfoGrbitMinimalInfo             0x40000000  //  for lists, only return the column name and columnid of each column
#define JET_ColInfoGrbitSortByColumnid          0x20000000  //  for lists, sort returned column list by columnid (default is to sort list by column name)
// end_PubEsent
#define JET_ColInfoGrbitCompacting              0x10000000  //  INTERNAL USE ONLY
// begin_PubEsent

#endif // JET_VERSION >= 0x0600

#if ( JET_VERSION >= 0x0600 )

    /* Info levels for JetGetInstanceMiscInfo, which is very different than JetGetInstanceInfo, as that retrieves a list of all instances */

#define JET_InstanceMiscInfoLogSignature    0U
// end_PubEsent
#define JET_InstanceMiscInfoCheckpoint      1U
// begin_PubEsent

#endif // JET_VERSION >= 0x0600

#if ( JET_VERSION >= 0x0A01 )

#define JET_InstanceMiscInfoRBS             2U // Retrieve revert snapshot info for the instance.

#endif // JET_VERSION >= 0x0A01

// end_PubEsent
#if ( JET_VERSION >= 0x0600 )

    /* Info parameter for JetGetPageInfo */

#define JET_PageInfo                0U
#if ( JET_VERSION >= 0x0601 )
#define JET_PageInfo2           1U
#endif // JET_VERSION >= 0x0601

    /* grbits for JetGetPageInfo */

#if ( JET_VERSION >= 0x0602 )
#define JET_bitPageInfoNoStructureChecksum  0x00000001  /* Do not compute structure checksum */
#endif // JET_VERSION >= 0x0602

#endif // JET_VERSION >= 0x0600


#if ( JET_VERSION >= 0x0601 )

    /* Flags for JetPatchDatabasePages */

#define JET_bitTestUninitShrunkPageImage    0x00000001
#define JET_bitPatchingCorruptPage          0x00000002

#endif // JET_VERSION >= 0x0601

#if ( JET_VERSION >= 0x602 )

    /* Flags for JetOnlinePatchDatabasePage */

// #define JET_bitPatchAllowCorruption      0x00000002              //  OBSOLETE and UNSUPPORTED - don't checksum the page

#endif // JET_VERSION >= 0x0602

#if ( JET_VERSION >= 0x0601 )

    /* Flags for JetEndDatabaseIncrementalReseed */

#define JET_bitEndDatabaseIncrementalReseedCancel       0x00000001      //  Stop an incremental reseed operation prematurely for any (failing) reason.  Database will be left in inconsistent JET_dbstateIncrementalReseedInProgress state.

#endif // JET_VERSION >= 0x0601

#if ( JET_VERSION >= 0x0A01 )

    /* Flags for JetBeginDatabaseIncrementalReseed */

#define JET_bitBeginDatabaseIncrementalReseedPatchRBS   0x00000001      //  Try to patch RBS file as part of increseed. If this flag is not passed, we will lose the RBS files and the ability to revert back to the past.

#endif // JET_VERSION >= 0x0A01


// begin_PubEsent


    /* Engine Object Types */

#define JET_objtypNil               0
#define JET_objtypTable             1
// end_PubEsent
#define JET_objtypDb                2
#define JET_objtypContainer         3
#define JET_objtypLongRoot          9   /*  INTERNAL USE ONLY */
// begin_PubEsent

    /* Compact Options */

#define JET_bitCompactStats             0x00000020  /* Dump off-line compaction stats (only when progress meter also specified) */
#define JET_bitCompactRepair            0x00000040  /* Don't preread and ignore duplicate keys */
// end_PubEsent
// #define JET_bitCompactSLVCopy            0x00000080  /* Recreate SLV file, do not reuse the existing one */
#if ( JET_VERSION >= 0x0600 )
#define JET_bitCompactPreserveOriginal  0x00000100  /* Preserve original database */
#endif // JET_VERSION >= 0x0600
// begin_PubEsent

    /* Status Notification Processes */

#define JET_snpRepair                   2
#define JET_snpCompact                  4
#define JET_snpRestore                  8
#define JET_snpBackup                   9
#define JET_snpUpgrade                  10
#if ( JET_VERSION >= 0x0501 )
#define JET_snpScrub                    11
#define JET_snpUpgradeRecordFormat      12
#endif // JET_VERSION >= 0x0501
// end_PubEsent
// #define JET_snpSLVChecksum               17
#if ( JET_VERSION >= 0x0602 )
#define JET_snpRecoveryControl          18
#define JET_snpExternalAutoHealing      19
#endif // JET_VERSION >= 0x0602
#if ( JET_VERSION >= 0x0A01 )
#define JET_snpSpaceCategorization      20
#endif // JET_VERSION >= 0x0A01
// begin_PubEsent


    /* Status Notification Types */

//  Generic Status Notification Types
#define JET_sntBegin            5   /* callback for beginning of operation */
#define JET_sntRequirements     7   /* callback for returning operation requirements */
#define JET_sntProgress         0   /* callback for progress */
#define JET_sntComplete         6   /* callback for completion of operation */
#define JET_sntFail             3   /* callback for failure during progress */

// end_PubEsent
#if ( JET_VERSION >= 0x0602 )

//  Operation Specific "Status Notification" Types

//  JET_snpRecoveryControl specific JET_SNTs (controlled by JET_bitExternalRecoveryControl)
//
#define JET_sntOpenLog                  1001    /* callback for opening a new log */
#define JET_sntOpenCheckpoint           1002    /* callback for opening a checkpoint */
// Reserved JET_sntOpenDatabase         1003    /* callback for opening a database during recovery */
#define JET_sntMissingLog               1004    /* callback due to an expected log is missing */
#define JET_sntBeginUndo                1005    /* callback to indicate the desire to being undo */
#define JET_sntNotificationEvent        1006    /* callback to indicate an event that we would normally submit to the event log */
#define JET_sntSignalErrorCondition     1007    /* callback to indicate a generic error condition has arrisen */
// Reserved JET_sntHitRecoveryPoint     100?    /* callback to indicate we have reached the lgposStop set in the JET_RSTINFO[2]_* structure */
#define JET_sntAttachedDb               1008    /* database attached during recovery */
#define JET_sntDetachingDb              1009    /* database about to be detached during recovery */
#define JET_sntCommitCtx                1010    /* commit context found during recovery */

//  JET_snpExternalAutoHealing specific JET_SNTs (controlled by JET_paramEnableExternalAutoHealing)
//
#define JET_sntPagePatchRequest         1101    /* a log record request a page patch was encountered */
#define JET_sntCorruptedPage            1102    /* a corrupted page was encountered */
#endif
// begin_PubEsent

    /* Exception action / JET_paramExceptionAction */

#define JET_ExceptionMsgBox     0x0001      /* Display message box on exception */
#define JET_ExceptionNone       0x0002      /* Do nothing on exceptions */
#define JET_ExceptionFailFast   0x0004      /* Use the Windows RaiseFailFastException API to force a crash */

// end_PubEsent
    /* AssertAction / JET_paramAssertAction  */

#define JET_AssertExit              0x0000      /* Exit the application */
#define JET_AssertBreak             0x0001      /* Break to debugger */
#define JET_AssertMsgBox            0x0002      /* Display message box */
#define JET_AssertStop              0x0004      /* Alert and stop */
#if ( JET_VERSION >= 0x0601 )
#define JET_AssertSkippableMsgBox   0x0008      /* Display skippable message box */
#define JET_AssertSkipAll           0x0010      /* Skip all asserts */
#define JET_AssertCrash             0x0020      /* AV (*0=0 style) */
#define JET_AssertFailFast          0x0040      /* Use the Windows RaiseFailFastException API to force a crash */
#endif // JET_VERSION >= 0x0601

// begin_PubEsent

#if ( JET_VERSION >= 0x0501 )
    //  Online defragmentation options
#define JET_OnlineDefragDisable         0x0000      //  disable online defrag
#define JET_OnlineDefragAllOBSOLETE     0x0001      //  enable online defrag for everything (must be 1 for backward compatibility)
#define JET_OnlineDefragDatabases       0x0002      //  enable online defrag of databases
#define JET_OnlineDefragSpaceTrees      0x0004      //  enable online defrag of space trees
// end_PubEsent
#define JET_OnlineDefragStreamingFiles  0x0008      //  enable online defrag of streaming files
// begin_PubEsent
#define JET_OnlineDefragAll             0xffff      //  enable online defrag for everything

#endif // JET_VERSION >= 0x0501


// end_PubEsent
    /* Counter flags */         // For XJET only, not for JET97

#define ctAccessPage            1
#define ctLatchConflict         2
#define ctSplitRetry            3
#define ctNeighborPageScanned   4
#define ctSplits                5


#if ( JET_VERSION >= 0x0600 )
    /* Resource manager tag size */

#define JET_resTagSize          4
#endif // JET_VERSION >= 0x0600
// begin_PubEsent

#if ( JET_VERSION >= 0x0602 )

// Info levels for JetGetErrorInfo:
#define JET_ErrorInfoSpecificErr        1U          //  Info about the specific error passed in pvContext.

// grbits for JetGetErrorInfoW:
// None yet.

// grbits for JetResizeDatabase:
#define JET_bitResizeDatabaseOnlyGrow               0x00000001  // Only grow the database. If the resize call would shrink the database, do nothing.
#endif // JET_VERSION >= 0x0602

#if ( JET_VERSION >= 0x0603 )
#define JET_bitResizeDatabaseOnlyShrink             0x00000002  // Only shrink the database. If the resize call would grow the database, do nothing. The file may end up smaller than requested.
// end_PubEsent

// DEPRECATED: this was once used in the first implementation of DB shrink.
// #define JET_bitResizeDatabaseShrinkCompactSize       0x00000004  // DEPRECATED: Shrink the database to the smallest possible size without keeping an empty extent at the end.

// begin_PubEsent

#endif // JET_VERSION >= 0x0603

#if ( JET_VERSION >= 0x0602 )
#define JET_bitStopServiceAll                       0x00000000  //  Stops all ESE services for the specified instance.
// end_PubEsent
// reserved:                                        0x00000001  //  (Internal StopServiceAll indicator)
// begin_PubEsent
#define JET_bitStopServiceBackgroundUserTasks       0x00000002  //  Stops restartable client specificed background maintenance tasks (B+ Tree Defrag for example).
#define JET_bitStopServiceQuiesceCaches             0x00000004  //  Quiesces all dirty caches to disk. Asynchronous. Cancellable.

// Warning: This bit can only be used to resume StopServiceBackgroundUserTasks and JET_bitStopServiceQuiesceCaches, if you 
// previously called with JET_bitStopServiceAll, attempting to use JET_bitStopServiceResume will fail. 
#define JET_bitStopServiceResume                    0x80000000  //  Resumes previously issued StopService operations, i.e. "restarts service".  Can be combined with above grbits to Resume specific services, or with JET_bitStopServiceAll to Resume all previously stopped services.
#endif // JET_VERSION >= 0x0602

// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )
#define JET_bitStopServiceStopAndEmitLog            0x00000008  //  Stop any further logging and emit last chunk of log.
#endif

// These bits are used with JET_paramStageFlighting for private staging, alpha, beta, and test in production staging.  Should not be used by
// any consumers without contacting the ESE dev team directly.
#define JET_bitStageTestEnvLocalMode                    0x04
#define JET_bitStageTestEnvAlphaMode                    0x10
#define JET_bitStageTestEnvBetaMode                     0x40

#define JET_bitStageSelfhostLocalMode                 0x0400
#define JET_bitStageSelfhostAlphaMode                 0x1000
#define JET_bitStageSelfhostBetaMode                  0x4000

#define JET_bitStageProdLocalMode                   0x040000
#define JET_bitStageProdAlphaMode                   0x100000
#define JET_bitStageProdBetaMode                    0x400000

// Encryption algorithms for JetCreateEncryptionKey
#if ( JET_VERSION >= 0x0A01 )
#define JET_EncryptionAlgorithmAes256       1
#endif

// begin_PubEsent

/**********************************************************************/
/***********************     ERROR CODES     **************************/
/**********************************************************************/

/* The Error codes are not versioned with WINVER. */

/* SUCCESS */

#define JET_errSuccess                       0    /* Successful Operation */

/* ERRORS */

#define JET_wrnNyi                          -1    /* Function Not Yet Implemented */

/*  SYSTEM errors
/**/
#define JET_errRfsFailure                   -100  /* Resource Failure Simulator failure */
#define JET_errRfsNotArmed                  -101  /* Resource Failure Simulator not initialized */
#define JET_errFileClose                    -102  /* Could not close file */
#define JET_errOutOfThreads                 -103  /* Could not start thread */
#define JET_errTooManyIO                    -105  /* System busy due to too many IOs */
#define JET_errTaskDropped                  -106  /* A requested async task could not be executed */
#define JET_errInternalError                -107  /* Fatal internal error */
// end_PubEsent
#define errCodeInconsistency                -108 /* Fatal internal error */
#define errNotFound                         -109  /* Generic not found error, internal only */
#define wrnSlow                              110  /* Generic inefficiency detected, internal only */
#define wrnLossy                             111  /* Generic lossy process taken, fidelity lost, internal only */

// begin_PubEsent
#define JET_errDisabledFunctionality        -112  /* You are running MinESE, that does not have all features compiled in.  This functionality is only supported in a full version of ESE. */
#define JET_errUnloadableOSFunctionality    -113  /* The desired OS functionality could not be located and loaded / linked. */
// end_PubEsent
#define JET_errDeviceMissing                -114  /* A required hardware device or functionality was missing. */
#define JET_errDeviceMisconfigured          -115  /* A required hardware device was misconfigured externally. */
#define JET_errDeviceTimeout                -116  /* Timeout occurred while waiting for a hardware device to respond. */
#define errDeviceBusy                       -117  /* Hardware device is busy doing other operations. */
#define JET_errDeviceFailure                -118  /* A required hardware device didn't function as expected. */

// begin_PubEsent

//  BUFFER MANAGER errors
//
// end_PubEsent
#define wrnBFCacheMiss                       200  /*  ese97,esent only:  page latch caused a cache miss  */
#define errBFPageNotCached                  -201  /*  page is not cached  */
#define errBFLatchConflict                  -202  /*  page latch conflict  */
#define errBFPageCached                     -203  /*  page is cached  */
#define wrnBFPageFlushPending                204  /*  page is currently being written  */
#define wrnBFPageFault                       205  /*  page latch caused a page fault  */
#define wrnBFBadLatchHint                    206  /*  page latch hint was incorrect  */
#define wrnBFLatchMaintConflict              207  /*  page latch conflict with foreground maintenance  */
#define wrnBFIWriteIOComplete                208  /*  signal a successful write IO from the async IO completion function  */

#define errBFIPageEvicted                   -250  /*  ese97,esent only:  page evicted from the cache  */
#define errBFIPageCached                    -251  /*  ese97,esent only:  page already cached  */
#define errBFIOutOfOLPs                     -252  /*  ese97,esent only:  out of OLPs  */
#define errBFIOutOfBatchIOBuffers           -253  /*  out of Batch I/O (Opportune write) Buffers  */
#define errBFINoBufferAvailable             -254  /*  no buffer available for immediate use  */
// begin_PubEsent
#define JET_errDatabaseBufferDependenciesCorrupted  -255    /* Buffer dependencies improperly set. Recovery failure */
// end_PubEsent
#define errBFIRemainingDependencies         -256  /*  dependencies remain on this buffer  */
#define errBFIPageFlushPending              -257  /*  page is currently being written  */
#define errBFIPageDirty                     -258  /*  the page could not be evicted from the cache because it or its versions were not clean enough */
#define errBFIPageFlushed                   -259  /*  page write initiated  */
#define errBFIPageFaultPending              -260  /*  page is currently being read  */
#define errBFIPageNotVerified               -261  /*  page data has not been verified  */
#define errBFIDependentPurged               -262  /*  page cannot be flushed due to purged dependencies  */
#define errBFIPageFlushDisallowedOnIOThread -263  /*  the page couldn't be written because ErrBFIFlushPage is being called from the I/O thread  */
#define errBFIPageTouchTooRecent            -264  /*  the page could not be flushed because a recent page touch would offend the waypoint */
#define errBFICheckpointWorkRemaining       -266  /*  checkpoint depth maintenance is not finished due to page flushes or dependency flushes remaining */
#define errBFIPageRemapNotReVerified        -267  /*  page is remapped after a write, which means it needs to be reverified */
#define errBFIReqSyncFlushMapWriteFailed    -268  /*  required synchronous write to the flush map failed  */
#define errBFIPageFlushPendingHungIO        -269  /*  page is currently being written and the write I/O is hung */
#define errBFIPageFaultPendingHungIO        -270  /*  page is currently being read and the read I/O is hung */
#define errBFIPageFlushPendingSlowIO        -271  /*  page is currently being written and the write I/O is slow */
#define errBFIPageAbandoned                 -272  /*  page is currently in an abandoned state */
// #define errBFIPrereadPageBeyondEOF          -273  /*  DEPRECATED: we tried to preread a page beyond EOF */

//  VERSION STORE errors
//
#define wrnVERRCEMoved                       275  /*  RCE was moved instead of being cleaned */
// begin_PubEsent

/*  DIRECTORY MANAGER errors
/**/
// end_PubEsent
#define errPMOutOfPageSpace                 -300  /* Out of page space */
#define errPMItagTooBig                     -301  /* Itag too big */                    //  XXX -- to be deleted
#define errPMRecDeleted                     -302  /* Record deleted */                  //  XXX -- to be deleted
#define errPMTagsUsedUp                     -303  /* Tags used up */                    //  XXX -- to be deleted
#define wrnBMConflict                        304  /* conflict in BM Clean up */
#define errDIRNoShortCircuit                -305  /* No Short Circuit Avail */
#define errDIRCannotSplit                   -306  /* Cannot horizontally split FDP */
#define errDIRTop                           -307  /* Cannot go up */
#define errDIRFDP                            308  /* On an FDP Node */
#define errDIRNotSynchronous                -309  /* May have left critical section */
#define wrnDIREmptyPage                      310  /* Moved through empty page */
#define errSPConflict                       -311  /* Device extent being extended */
#define wrnNDFoundLess                       312  /* Found Less */
//moved: errNDNotFound                      -312  /* new value: -349 */
#define wrnNDFoundGreater                    313  /* Found Greater */
#define wrnNDNotFoundInPage                  314  /* for smart refresh */
//moved: errNDOutSonRange                   -314  /* new value: -350 */
#define errNDOutItemRange                   -315  /* Item out of range */
#define errNDGreaterThanAllItems            -316  /* Greater than all items */
#define errNDLastItemNode                   -317  /* Last node of item list */
#define errNDFirstItemNode                  -318  /* First node of item list */
#define wrnNDDuplicateItem                   319  /* Duplicated Item */
#define errNDNoItem                         -320  /* Item not there */
// begin_PubEsent
#define JET_wrnRemainingVersions             321  /* The version store is still active */
#define JET_errPreviousVersion              -322  /* Version already existed. Recovery failure */
#define JET_errPageBoundary                 -323  /* Reached Page Boundary */
#define JET_errKeyBoundary                  -324  /* Reached Key Boundary */
// end_PubEsent
#define errDIRInPageFather                  -325  /* sridFather in page to free */
#define errBMMaxKeyInPage                   -326  /* used by OLC to avoid cleanup of parent pages */
// begin_PubEsent
#define JET_errBadPageLink                  -327  /* Database corrupted */
#define JET_errBadBookmark                  -328  /* Bookmark has no corresponding address in database */
// end_PubEsent
#define wrnBMCleanNullOp                     329  // BMClean returns this on encountering a page
                                                  // deleted MaxKeyInPage [but there was no conflict]
#define errBTOperNone                       -330  // Split with no accompanying
                                                  // insert/replace
#define errSPOutOfAvailExtCacheSpace        -331  // unable to make update to AvailExt tree since
                                                  // in-cursor space cache is depleted
#define errSPOutOfOwnExtCacheSpace          -332  // unable to make update to OwnExt tree since
                                                  // in-cursor space cache is depleted
#define wrnBTMultipageOLC                    333  // needs multipage OLC operation
// begin_PubEsent
#define JET_errNTSystemCallFailed           -334  // A call to the operating system failed
// end_PubEsent
#define wrnBTShallowTree                     335  // BTree is only one or two levels deep
#define errBTMergeNotSynchronous            -336  // Multiple threads attempting to perform merge/split on same page (likely OLD vs. RCEClean)
#define wrnSPReservedPages                   337  // space manager reserved pages for future space tree splits
// begin_PubEsent
#define JET_errBadParentPageLink            -338  // Database corrupted
// end_PubEsent
#define wrnSPBuildAvailExtCache              339  // AvailExt tree is sufficiently large that it should be cached
// begin_PubEsent
#define JET_errSPAvailExtCacheOutOfSync     -340  // AvailExt cache doesn't match btree
#define JET_errSPAvailExtCorrupted          -341  // AvailExt space tree is corrupt
#define JET_errSPAvailExtCacheOutOfMemory   -342  // Out of memory allocating an AvailExt cache node
#define JET_errSPOwnExtCorrupted            -343  // OwnExt space tree is corrupt
#define JET_errDbTimeCorrupted              -344  // Dbtime on current page is greater than global database dbtime
#define JET_wrnUniqueKey                     345  // seek on non-unique index yielded a unique key
#define JET_errKeyTruncated                 -346  // key truncated on index that disallows key truncation
// end_PubEsent
#define errSPNoSpaceForYou                  -347  // There was no space of this type or in this avail tree for the caller.
// begin_PubEsent
#define JET_errDatabaseLeakInSpace          -348  // Some database pages have become unreachable even from the avail tree, only an offline defragmentation can return the lost space.
// end_PubEsent
#define errNDNotFound                       -349  /* Not found */
#define errNDOutSonRange                    -350  /* Son out of range */
// begin_PubEsent
#define JET_errBadEmptyPage                 -351  // Database corrupted. Searching an unexpectedly empty page.
#define wrnBTNotVisibleRejected              352  /* Current entry is not visible because it has been rejected by a move filter */
#define wrnBTNotVisibleAccumulated           353  /* Current entry is not visible because it is being accumulated by a move filter */
#define JET_errBadLineCount                 -354  /* Number of lines on the page is too few compared to the line being operated on */
// end_PubEsent
#define errSPNoSpaceBelowShrinkTarget       -355  // There was no space available which falls below the current page number we are trying to shrink to.
#define wrnSPRequestSpBufRefill              356  // A split buffer page may have been consumed in the process of refilling split buffers and the space manager should request new split buffer refill.
// begin_PubEsent
#define JET_errPageTagCorrupted             -357  // A tag / line on page is logically corrupted, offset or size is bad, or tag count on page is bad.
#define JET_errNodeCorrupted                -358  // A node or prefix node is logically corrupted, the key suffix size is larger than the node or line's size.

/*  RECORD MANAGER errors
/**/
// end_PubEsent
#define wrnFLDKeyTooBig                      400  /* Key too big (truncated it) */
#define errFLDTooManySegments               -401  /* Too many key segments */
#define wrnFLDNullKey                        402  /* Key is entirely NULL */
#define wrnFLDOutOfKeys                      403  /* No more keys to extract */
#define wrnFLDNullSeg                        404  /* Null segment in key */
#define wrnFLDNotPresentInIndex              405
// begin_PubEsent
#define JET_wrnSeparateLongValue             406  /* Column is a separated long-value */
// end_PubEsent
#define wrnRECLongField                      407  /* Long value */
// begin_PubEsent
#define JET_wrnRecordFoundGreater           JET_wrnSeekNotEqual
#define JET_wrnRecordFoundLess              JET_wrnSeekNotEqual
#define JET_errColumnIllegalNull            JET_errNullInvalid
// end_PubEsent
//moved: wrnFLDNullFirstSeg                  408  /* new value: 422 */
// begin_PubEsent
#define JET_errKeyTooBig                    -408  /* Key is too large */
// end_PubEsent
#define wrnRECUserDefinedDefault             409  /* User-defined default value */
#define wrnRECSeparatedLV                    410  /* LV stored in LV tree */
#define wrnRECIntrinsicLV                    411  /* LV stored in the record */
#define wrnFLDIndexUpdated                   414    // index update performed
#define wrnFLDOutOfTuples                    415    // no more tuples for current string
// begin_PubEsent
#define JET_errCannotSeparateIntrinsicLV    -416    // illegal attempt to separate an LV which must be intrinsic
// end_PubEsent
#define wrnRECCompressed                     417  /* LV stored in the record in compressed form */
#define errRECCannotCompress                -418  /* column cannot be compressed */
#define errRECCompressionNotPossible        -419  /* can't store column in compressed form */
#define wrnRECCompressionScrubDetected       420  /* Returned when the record has been scrubbed. It is invalid to try and retrieve any data from this record. This warning is used for internal signaling only. */
// begin_PubEsent
#define JET_errSeparatedLongValue           -421 /* Operation not supported on separated long-value */
// end_PubEsent
#define wrnFLDNullFirstSeg                   422  /* Null first segment in key */
// begin_PubEsent
#define JET_errMustBeSeparateLongValue      -423  /* Can only preread long value columns that can be separate, e.g. not size constrained so that they are fixed or variable columns */
#define JET_errInvalidPreread               -424  /* Cannot preread long values when current index secondary */
// end_PubEsent
#define wrnRECSeparatedEncryptedLV           425  /* LV stored encrypted in LV tree */
#define JET_errInvalidColumnReference       -426  /* Column reference is invalid */
#define JET_errStaleColumnReference         -427  /* Column reference is stale */
#define JET_wrnNoMoreRecords                 428  /* No more records to stream */
#define errRECColumnNotFound                -429  /* Column value not found in record */
#define errRECNoCurrentColumnValue          -430  /* No current column value in record */
#define JET_errCompressionIntegrityCheckFailed  -431  /* A compression integrity check failed. Decompressing data failed the integrity checksum indicating a data corruption in the compress/decompress pipeline. */
// begin_PubEsent

/*  LOGGING/RECOVERY errors
/**/
#define JET_errInvalidLoggedOperation       -500  /* Logged operation cannot be redone */
#define JET_errLogFileCorrupt               -501  /* Log file is corrupt */
// end_PubEsent
#define errLGNoMoreRecords                  -502  /* Last log record read */
// begin_PubEsent
#define JET_errNoBackupDirectory            -503  /* No backup directory given */
#define JET_errBackupDirectoryNotEmpty      -504  /* The backup directory is not empty */
#define JET_errBackupInProgress             -505  /* Backup is active already */
#define JET_errRestoreInProgress            -506  /* Restore in progress */
#define JET_errMissingPreviousLogFile       -509  /* Missing the log file for check point */
#define JET_errLogWriteFail                 -510  /* Failure writing to log file */
#define JET_errLogDisabledDueToRecoveryFailure  -511 /* Try to log something after recovery failed */
#define JET_errCannotLogDuringRecoveryRedo      -512    /* Try to log something during recovery redo */
#define JET_errLogGenerationMismatch        -513  /* Name of logfile does not match internal generation number */
#define JET_errBadLogVersion                -514  /* Version of log file is not compatible with Jet version */
#define JET_errInvalidLogSequence           -515  /* Timestamp in next log does not match expected */
#define JET_errLoggingDisabled              -516  /* Log is not active */
#define JET_errLogBufferTooSmall            -517  /* An operation generated a log record which was too large to fit in the log buffer or in a single log file */
// end_PubEsent
#define errLGNotSynchronous                 -518  /* retry to LGLogRec */
// begin_PubEsent
#define JET_errLogSequenceEnd               -519  /* Maximum log file number exceeded */
#define JET_errNoBackup                     -520  /* No backup in progress */
#define JET_errInvalidBackupSequence        -521  /* Backup call out of sequence */
#define JET_errBackupNotAllowedYet          -523  /* Cannot do backup now */
#define JET_errDeleteBackupFileFail         -524  /* Could not delete backup file */
#define JET_errMakeBackupDirectoryFail      -525  /* Could not make backup temp directory */
#define JET_errInvalidBackup                -526  /* Cannot perform incremental backup when circular logging enabled */
#define JET_errRecoveredWithErrors          -527  /* Restored with errors */
#define JET_errMissingLogFile               -528  /* Current log file missing */
#define JET_errLogDiskFull                  -529  /* Log disk full */
#define JET_errBadLogSignature              -530  /* Bad signature for a log file */
#define JET_errBadDbSignature               -531  /* Bad signature for a db file */
#define JET_errBadCheckpointSignature       -532  /* Bad signature for a checkpoint file */
#define JET_errCheckpointCorrupt            -533  /* Checkpoint file not found or corrupt */
#define JET_errMissingPatchPage             -534  /* Patch file page not found during recovery */
#define JET_errBadPatchPage                 -535  /* Patch file page is not valid */
#define JET_errRedoAbruptEnded              -536  /* Redo abruptly ended due to sudden failure in reading logs from log file */
#define JET_errPatchFileMissing             -538  /* Hard restore detected that patch file is missing from backup set */
#define JET_errDatabaseLogSetMismatch       -539  /* Database does not belong with the current set of log files */
#define JET_errDatabaseStreamingFileMismatch    -540 /* Database and streaming file do not match each other */
#define JET_errLogFileSizeMismatch          -541  /* actual log file size does not match JET_paramLogFileSize */
#define JET_errCheckpointFileNotFound       -542  /* Could not locate checkpoint file */
#define JET_errRequiredLogFilesMissing      -543  /* The required log files for recovery is missing. */
#define JET_errSoftRecoveryOnBackupDatabase -544  /* Soft recovery is intended on a backup database. Restore should be used instead */
#define JET_errLogFileSizeMismatchDatabasesConsistent   -545  /* databases have been recovered, but the log file size used during recovery does not match JET_paramLogFileSize */
#define JET_errLogSectorSizeMismatch        -546  /* the log file sector size does not match the current volume's sector size */
#define JET_errLogSectorSizeMismatchDatabasesConsistent -547  /* databases have been recovered, but the log file sector size (used during recovery) does not match the current volume's sector size */
#define JET_errLogSequenceEndDatabasesConsistent        -548 /* databases have been recovered, but all possible log generations in the current sequence are used; delete all log files and the checkpoint file and backup the databases before continuing */

#define JET_errStreamingDataNotLogged       -549  /* Illegal attempt to replay a streaming file operation where the data wasn't logged. Probably caused by an attempt to roll-forward with circular logging enabled */

#define JET_errDatabaseDirtyShutdown        -550  /* Database was not shutdown cleanly. Recovery must first be run to properly complete database operations for the previous shutdown. */
#define JET_errDatabaseInconsistent         JET_errDatabaseDirtyShutdown    /* OBSOLETE */
#define JET_errConsistentTimeMismatch       -551  /* Database last consistent time unmatched */
#define JET_errDatabasePatchFileMismatch    -552  /* Patch file is not generated from this backup */
#define JET_errEndingRestoreLogTooLow       -553  /* The starting log number too low for the restore */
#define JET_errStartingRestoreLogTooHigh    -554  /* The starting log number too high for the restore */
#define JET_errGivenLogFileHasBadSignature  -555  /* Restore log file has bad signature */
#define JET_errGivenLogFileIsNotContiguous  -556  /* Restore log file is not contiguous */
#define JET_errMissingRestoreLogFiles       -557  /* Some restore log files are missing */
#define JET_wrnExistingLogFileHasBadSignature   558  /* Existing log file has bad signature */
#define JET_wrnExistingLogFileIsNotContiguous   559  /* Existing log file is not contiguous */
#define JET_errMissingFullBackup            -560  /* The database missed a previous full backup before incremental backup */
#define JET_errBadBackupDatabaseSize        -561  /* The backup database size is not in 4k */
#define JET_errDatabaseAlreadyUpgraded      -562  /* Attempted to upgrade a database that is already current */
#define JET_errDatabaseIncompleteUpgrade    -563  /* Attempted to use a database which was only partially converted to the current format -- must restore from backup */
#define JET_wrnSkipThisRecord                564  /* INTERNAL ERROR */
#define JET_errMissingCurrentLogFiles       -565  /* Some current log files are missing for continuous restore */

#define JET_errDbTimeTooOld                     -566  /* dbtime on page smaller than dbtimeBefore in record */
#define JET_errDbTimeTooNew                     -567  /* dbtime on page in advance of the dbtimeBefore and below dbtimeAfter in record */
// end_PubEsent
//#define wrnCleanedUpMismatchedFiles                568  /* INTERNAL WARNING: indicates that the redo function cleaned up logs/checkpoint because of a size mismatch (see JET_paramCleanupMismatchedLogFiles) */
// begin_PubEsent
#define JET_errMissingFileToBackup              -569  /* Some log or patch files are missing during backup */

#define JET_errLogTornWriteDuringHardRestore    -570    /* torn-write was detected in a backup set during hard restore */
#define JET_errLogTornWriteDuringHardRecovery   -571    /* torn-write was detected during hard recovery (log was not part of a backup set) */
#define JET_errLogCorruptDuringHardRestore      -573    /* corruption was detected in a backup set during hard restore */
#define JET_errLogCorruptDuringHardRecovery     -574    /* corruption was detected during hard recovery (log was not part of a backup set) */

#define JET_errMustDisableLoggingForDbUpgrade   -575    /* Cannot have logging enabled while attempting to upgrade db */
// end_PubEsent
#define errLGRecordDataInaccessible             -576    /* an incomplete log record was created because all the data to be logged was not accessible */
// begin_PubEsent

#define JET_errBadRestoreTargetInstance         -577    /* TargetInstance specified for restore is not found or log files don't match */
#define JET_wrnTargetInstanceRunning             578    /* TargetInstance specified for restore is running */

#define JET_errRecoveredWithoutUndo             -579    /* Soft recovery successfully replayed all operations, but the Undo phase of recovery was skipped */

#define JET_errDatabasesNotFromSameSnapshot     -580    /* Databases to be restored are not from the same shadow copy backup */
#define JET_errSoftRecoveryOnSnapshot           -581    /* Soft recovery on a database from a shadow copy backup set */
#define JET_errCommittedLogFilesMissing         -582    /* One or more logs that were committed to this database, are missing.  These log files are required to maintain durable ACID semantics, but not required to maintain consistency if the JET_bitReplayIgnoreLostLogs bit is specified during recovery. */
#define JET_errSectorSizeNotSupported           -583    /* The physical sector size reported by the disk subsystem, is unsupported by ESE for a specific file type. */
#define JET_errRecoveredWithoutUndoDatabasesConsistent  -584    /* Soft recovery successfully replayed all operations and intended to skip the Undo phase of recovery, but the Undo phase was not required */
#define JET_wrnCommittedLogFilesLost            585     /* One or more logs that were committed to this database, were not recovered.  The database is still clean/consistent, as though the lost log's transactions were committed lazily (and lost). */
#define JET_errCommittedLogFileCorrupt          -586    /* One or more logs were found to be corrupt during recovery.  These log files are required to maintain durable ACID semantics, but not required to maintain consistency if the JET_bitIgnoreLostLogs bit and JET_paramDeleteOutOfRangeLogs is specified during recovery. */
#define JET_wrnCommittedLogFilesRemoved         587     /* One or more logs that were committed to this database, were no recovered.  The database is still clean/consistent, as though the corrupted log's transactions were committed lazily (and lost). */
#define JET_wrnFinishWithUndo                   588     /* Signal used by clients to indicate JetInit() finished with undo */
// end_PubEsent
#define errSkipLogRedoOperation                 -589    /* The log redo operation should be skipped */
// begin_PubEsent
#define JET_errLogSequenceChecksumMismatch      -590    /* The previous log's accumulated segment checksum doesn't match the next log */

#define JET_wrnDatabaseRepaired                  595    /* Database corruption has been repaired */
#define JET_errPageInitializedMismatch          -596    /* Database divergence mismatch. Page was uninitialized on remote node, but initialized on local node. */


#define JET_errUnicodeTranslationBufferTooSmall -601    /* Unicode translation buffer too small */
#define JET_errUnicodeTranslationFail           -602    /* Unicode normalization failed */
#define JET_errUnicodeNormalizationNotSupported -603    /* OS does not provide support for Unicode normalisation (and no normalisation callback was specified) */
#define JET_errUnicodeLanguageValidationFailure -604    /* Can not validate the language */

#define JET_errExistingLogFileHasBadSignature   -610    /* Existing log file has bad signature */
#define JET_errExistingLogFileIsNotContiguous   -611    /* Existing log file is not contiguous */

#define JET_errLogReadVerifyFailure         -612  /* Checksum error in log file during backup */

#define JET_errCheckpointDepthTooDeep       -614    //  too many outstanding generations between checkpoint and current generation

#define JET_errRestoreOfNonBackupDatabase   -615    //  hard recovery attempted on a database that wasn't a backup database
#define JET_errLogFileNotCopied             -616    //  log truncation attempted but not all required logs were copied
// end_PubEsent
#define JET_errSurrogateBackupInProgress    -617    //  A surrogate backup is in progress.
// begin_PubEsent
#define JET_errTransactionTooLong           -618    //  Too many outstanding generations between JetBeginTransaction and current generation.

#define JET_errEngineFormatVersionNoLongerSupportedTooLow           -619 /* The specified JET_ENGINEFORMATVERSION value is too low to be supported by this version of ESE. */
#define JET_errEngineFormatVersionNotYetImplementedTooHigh          -620 /* The specified JET_ENGINEFORMATVERSION value is too high, higher than this version of ESE knows about. */
#define JET_errEngineFormatVersionParamTooLowForRequestedFeature    -621 /* Thrown by a format feature (not at JetSetSystemParameter) if the client requests a feature that requires a version higher than that set for the JET_paramEngineFormatVersion. */
#define JET_errEngineFormatVersionSpecifiedTooLowForLogVersion                      -622 /* The specified JET_ENGINEFORMATVERSION is set too low for this log stream, the log files have already been upgraded to a higher version.  A higher JET_ENGINEFORMATVERSION value must be set in the param. */
#define JET_errEngineFormatVersionSpecifiedTooLowForDatabaseVersion                 -623 /* The specified JET_ENGINEFORMATVERSION is set too low for this database file, the database file has already been upgraded to a higher version.  A higher JET_ENGINEFORMATVERSION value must be set in the param. */
// end_PubEsent
#define errLogServiceStopped                -624  /* Logging has been stopped via JetStopServiceInstance2 JET_bitStopServiceStopAndEmitLog */
// begin_PubEsent
#define JET_errDbTimeBeyondMaxRequired      -625  /* dbtime on page greater than or equal to dbtimeAfter in record, but record is outside required range for the database */
// end_PubEsent

#define errBackupAbortByCaller              -800  /* INTERNAL ERROR: Backup was aborted by client or RPC connection with client failed */
// begin_PubEsent
#define JET_errBackupAbortByServer          -801  /* Backup was aborted by server by calling JetTerm with JET_bitTermStopBackup or by calling JetStopBackup */
// end_PubEsent
#define errTooManyPatchRequests             -802  /* Too many patch requests in the patch request list */
// begin_PubEsent

#define JET_errInvalidGrbit                 -900  /* Invalid flags parameter */

#define JET_errTermInProgress               -1000 /* Termination in progress */
#define JET_errFeatureNotAvailable          -1001 /* API not supported */
#define JET_errInvalidName                  -1002 /* Invalid name */
#define JET_errInvalidParameter             -1003 /* Invalid API parameter */
#define JET_wrnColumnNull                    1004 /* Column is NULL-valued */
#define JET_wrnBufferTruncated               1006 /* Buffer too small for data */
#define JET_wrnDatabaseAttached              1007 /* Database is already attached */
#define JET_errDatabaseFileReadOnly         -1008 /* Tried to attach a read-only database file for read/write operations */
#define JET_wrnSortOverflow                  1009 /* Sort does not fit in memory */
#define JET_errInvalidDatabaseId            -1010 /* Invalid database id */
#define JET_errOutOfMemory                  -1011 /* Out of Memory */
#define JET_errOutOfDatabaseSpace           -1012 /* Maximum database size reached */
#define JET_errOutOfCursors                 -1013 /* Out of table cursors */
#define JET_errOutOfBuffers                 -1014 /* Out of database page buffers */
#define JET_errTooManyIndexes               -1015 /* Too many indexes */
#define JET_errTooManyKeys                  -1016 /* Too many columns in an index */
#define JET_errRecordDeleted                -1017 /* Record has been deleted */
#define JET_errReadVerifyFailure            -1018 /* Checksum error on a database page */
#define JET_errPageNotInitialized           -1019 /* Blank database page */
#define JET_errOutOfFileHandles             -1020 /* Out of file handles */
#define JET_errDiskReadVerificationFailure  -1021 /* The OS returned ERROR_CRC from file IO */
#define JET_errDiskIO                       -1022 /* Disk IO error */
#define JET_errInvalidPath                  -1023 /* Invalid file path */
#define JET_errInvalidSystemPath            -1024 /* Invalid system path */
#define JET_errInvalidLogDirectory          -1025 /* Invalid log directory */
#define JET_errRecordTooBig                 -1026 /* Record larger than maximum size */
#define JET_errTooManyOpenDatabases         -1027 /* Too many open databases */
#define JET_errInvalidDatabase              -1028 /* Not a database file */
#define JET_errNotInitialized               -1029 /* Database engine not initialized */
#define JET_errAlreadyInitialized           -1030 /* Database engine already initialized */
#define JET_errInitInProgress               -1031 /* Database engine is being initialized */
#define JET_errFileAccessDenied             -1032 /* Cannot access file, the file is locked or in use */
// end_PubEsent
#define JET_errQueryNotSupported            -1034 /* Query support unavailable */               //  XXX -- to be deleted
#define JET_errSQLLinkNotSupported          -1035 /* SQL Link support unavailable */            //  XXX -- to be deleted
// begin_PubEsent
#define JET_errBufferTooSmall               -1038 /* Buffer is too small */
#define JET_wrnSeekNotEqual                  1039 /* Exact match not found during seek */
#define JET_errTooManyColumns               -1040 /* Too many columns defined */
#define JET_errContainerNotEmpty            -1043 /* Container is not empty */
#define JET_errInvalidFilename              -1044 /* Filename is invalid */
#define JET_errInvalidBookmark              -1045 /* Invalid bookmark */
#define JET_errColumnInUse                  -1046 /* Column used in an index */
#define JET_errInvalidBufferSize            -1047 /* Data buffer doesn't match column size */
#define JET_errColumnNotUpdatable           -1048 /* Cannot set column value */
#define JET_errIndexInUse                   -1051 /* Index is in use */
#define JET_errLinkNotSupported             -1052 /* Link support unavailable */
#define JET_errNullKeyDisallowed            -1053 /* Null keys are disallowed on index */
#define JET_errNotInTransaction             -1054 /* Operation must be within a transaction */
#define JET_wrnNoErrorInfo                   1055 /* No extended error information */
#define JET_errMustRollback                 -1057 /* Transaction must rollback because failure of unversioned update */
#define JET_wrnNoIdleActivity                1058 /* No idle activity occurred */
#define JET_errTooManyActiveUsers           -1059 /* Too many active database users */
#define JET_errInvalidCountry               -1061 /* Invalid or unknown country/region code */
#define JET_errInvalidLanguageId            -1062 /* Invalid or unknown language id */
#define JET_errInvalidCodePage              -1063 /* Invalid or unknown code page */
#define JET_errInvalidLCMapStringFlags      -1064 /* Invalid flags for LCMapString() */
#define JET_errVersionStoreEntryTooBig      -1065 /* Attempted to create a version store entry (RCE) larger than a version bucket */
#define JET_errVersionStoreOutOfMemoryAndCleanupTimedOut    -1066 /* Version store out of memory (and cleanup attempt failed to complete) */
#define JET_wrnNoWriteLock                   1067 /* No write lock at transaction level 0 */
#define JET_wrnColumnSetNull                 1068 /* Column set to NULL-value */
#define JET_errVersionStoreOutOfMemory      -1069 /* Version store out of memory (cleanup already attempted) */
// end_PubEsent
#define JET_errCurrencyStackOutOfMemory     -1070 /* UNUSED: lCSRPerfFUCB * g_lCursorsMax exceeded (XJET only) */
// begin_PubEsent
#define JET_errCannotIndex                  -1071 /* Cannot index escrow column */
#define JET_errRecordNotDeleted             -1072 /* Record has not been deleted */
#define JET_errTooManyMempoolEntries        -1073 /* Too many mempool entries requested */
#define JET_errOutOfObjectIDs               -1074 /* Out of btree ObjectIDs (perform offline defrag to reclaim freed/unused ObjectIds) */
#define JET_errOutOfLongValueIDs            -1075 /* Long-value ID counter has reached maximum value. (perform offline defrag to reclaim free/unused LongValueIDs) */
#define JET_errOutOfAutoincrementValues     -1076 /* Auto-increment counter has reached maximum value (offline defrag WILL NOT be able to reclaim free/unused Auto-increment values). */
#define JET_errOutOfDbtimeValues            -1077 /* Dbtime counter has reached maximum value (perform offline defrag to reclaim free/unused Dbtime values) */
#define JET_errOutOfSequentialIndexValues   -1078 /* Sequential index counter has reached maximum value (perform offline defrag to reclaim free/unused SequentialIndex values) */

#define JET_errRunningInOneInstanceMode     -1080 /* Multi-instance call with single-instance mode enabled */
#define JET_errRunningInMultiInstanceMode   -1081 /* Single-instance call with multi-instance mode enabled */
#define JET_errSystemParamsAlreadySet       -1082 /* Global system parameters have already been set */

#define JET_errSystemPathInUse              -1083 /* System path already used by another database instance */
#define JET_errLogFilePathInUse             -1084 /* Logfile path already used by another database instance */
#define JET_errTempPathInUse                -1085 /* Temp path already used by another database instance */
#define JET_errInstanceNameInUse            -1086 /* Instance Name already in use */
#define JET_errSystemParameterConflict      -1087 /* Global system parameters have already been set, but to a conflicting or disagreeable state to the specified values. */

#define JET_errInstanceUnavailable          -1090 /* This instance cannot be used because it encountered a fatal error */
#define JET_errDatabaseUnavailable          -1091 /* This database cannot be used because it encountered a fatal error */
#define JET_errInstanceUnavailableDueToFatalLogDiskFull -1092 /* This instance cannot be used because it encountered a log-disk-full error performing an operation (likely transaction rollback) that could not tolerate failure */
#define JET_errInvalidSesparamId            -1093 /* This JET_sesparam* identifier is not known to the ESE engine. */

#define JET_errTooManyRecords               -1094 /* There are too many records to enumerate, switch to an API that handles 64-bit numbers */

#define JET_errInvalidDbparamId             -1095 /* This JET_dbparam* identifier is not known to the ESE engine. */

#define JET_errOutOfSessions                -1101 /* Out of sessions */
#define JET_errWriteConflict                -1102 /* Write lock failed due to outstanding write lock */
#define JET_errTransTooDeep                 -1103 /* Transactions nested too deeply */
#define JET_errInvalidSesid                 -1104 /* Invalid session handle */
#define JET_errWriteConflictPrimaryIndex    -1105 /* Update attempted on uncommitted primary index */
#define JET_errInTransaction                -1108 /* Operation not allowed within a transaction */
#define JET_errRollbackRequired             -1109 /* Must rollback current transaction -- cannot commit or begin a new one */
#define JET_errTransReadOnly                -1110 /* Read-only transaction tried to modify the database */
#define JET_errSessionWriteConflict         -1111 /* Attempt to replace the same record by two different cursors in the same session */

#define JET_errRecordTooBigForBackwardCompatibility             -1112 /* record would be too big if represented in a database format from a previous version of Jet */
#define JET_errCannotMaterializeForwardOnlySort                 -1113 /* The temp table could not be created due to parameters that conflict with JET_bitTTForwardOnly */

#define JET_errSesidTableIdMismatch         -1114 /* This session handle can't be used with this table id */
#define JET_errInvalidInstance              -1115 /* Invalid instance handle */
#define JET_errDirtyShutdown                -1116 /* The instance was shutdown successfully but all the attached databases were left in a dirty state by request via JET_bitTermDirty */
// unused -1117
#define JET_errReadPgnoVerifyFailure        -1118 /* The database page read from disk had the wrong page number. */
#define JET_errReadLostFlushVerifyFailure   -1119 /* The database page read from disk had a previous write not represented on the page. */
// end_PubEsent
#define errCantRetrieveDebuggeeMemory           -1120 /* Can not retrieve the requested memory from the debuggee */
// begin_PubEsent
#define JET_errFileSystemCorruption             -1121 /* File system operation failed with an error indicating the file system is corrupt. */
#define JET_wrnShrinkNotPossible                1122 /* Database file could not be shrunk because there is not enough internal free space available or there is unmovable data present. */
#define JET_errRecoveryVerifyFailure            -1123 /* One or more database pages read from disk during recovery do not match the expected state. */

#define JET_errFilteredMoveNotSupported         -1124 /* Attempted to provide a filter to JetSetCursorFilter() in an unsupported scenario. */

// end_PubEsent
#define JET_errMustCommitDistributedTransactionToLevel0         -1150 /* Attempted to PrepareToCommit a distributed transaction to non-zero level */
#define JET_errDistributedTransactionAlreadyPreparedToCommit    -1151 /* Attempted a write-operation after a distributed transaction has called PrepareToCommit */
#define JET_errNotInDistributedTransaction                      -1152 /* Attempted to PrepareToCommit a non-distributed transaction */
#define JET_errDistributedTransactionNotYetPreparedToCommit     -1153 /* Attempted to commit a distributed transaction, but PrepareToCommit has not yet been called */
#define JET_errCannotNestDistributedTransactions                -1154 /* Attempted to begin a distributed transaction when not at level 0 */
#define JET_errDTCMissingCallback                               -1160 /* Attempted to begin a distributed transaction but no callback for DTC coordination was specified on initialisation */
#define JET_errDTCMissingCallbackOnRecovery                     -1161 /* Attempted to recover a distributed transaction but no callback for DTC coordination was specified on initialisation */
#define JET_errDTCCallbackUnexpectedError                       -1162 /* Unexpected error code returned from DTC callback */
#define JET_wrnDTCCommitTransaction                              1163 /* Warning code DTC callback should return if the specified transaction is to be committed */
#define JET_wrnDTCRollbackTransaction                            1164 /* Warning code DTC callback should return if the specified transaction is to be rolled back */
// begin_PubEsent

#define JET_errDatabaseDuplicate            -1201 /* Database already exists */
#define JET_errDatabaseInUse                -1202 /* Database in use */
#define JET_errDatabaseNotFound             -1203 /* No such database */
#define JET_errDatabaseInvalidName          -1204 /* Invalid database name */
#define JET_errDatabaseInvalidPages         -1205 /* Invalid number of pages */
#define JET_errDatabaseCorrupted            -1206 /* Non database file or corrupted db */
#define JET_errDatabaseLocked               -1207 /* Database exclusively locked */
#define JET_errCannotDisableVersioning      -1208 /* Cannot disable versioning for this database */
#define JET_errInvalidDatabaseVersion       -1209 /* Database engine is incompatible with database */

/*  The following error code are for NT clients only. It will return such error during
 *  JetInit if JET_paramCheckFormatWhenOpenFail is set.
 */
#define JET_errDatabase200Format            -1210 /* The database is in an older (200) format */
#define JET_errDatabase400Format            -1211 /* The database is in an older (400) format */
#define JET_errDatabase500Format            -1212 /* The database is in an older (500) format */

#define JET_errPageSizeMismatch             -1213 /* The database page size does not match the engine */
#define JET_errTooManyInstances             -1214 /* Cannot start any more database instances */
#define JET_errDatabaseSharingViolation     -1215 /* A different database instance is using this database */
#define JET_errAttachedDatabaseMismatch     -1216 /* An outstanding database attachment has been detected at the start or end of recovery, but database is missing or does not match attachment info */
#define JET_errDatabaseInvalidPath          -1217 /* Specified path to database file is illegal */
#define JET_errDatabaseIdInUse              -1218 /* A database is being assigned an id already in use */
#define JET_errForceDetachNotAllowed        -1219 /* Force Detach allowed only after normal detach errored out */
#define JET_errCatalogCorrupted             -1220 /* Corruption detected in catalog */
#define JET_errPartiallyAttachedDB          -1221 /* Database is partially attached. Cannot complete attach operation */
#define JET_errDatabaseSignInUse            -1222 /* Database with same signature in use */
// end_PubEsent
#define errSkippedDbHeaderUpdate            -1223 /* some db header weren't update becase there were during detach */
// begin_PubEsent

#define JET_errDatabaseCorruptedNoRepair    -1224 /* Corrupted db but repair not allowed */
#define JET_errInvalidCreateDbVersion       -1225 /* recovery tried to replay a database creation, but the database was originally created with an incompatible (likely older) version of the database engine */

// end_PubEsent
#define JET_errDatabaseIncompleteIncrementalReseed  -1226 /* The database cannot be attached because it is currently being rebuilt as part of an incremental reseed. */
#define JET_errDatabaseInvalidIncrementalReseed     -1227 /* The database is not a valid state to perform an incremental reseed. */
#define JET_errDatabaseFailedIncrementalReseed      -1228 /* The incremental reseed being performed on the specified database cannot be completed due to a fatal error.  A full reseed is required to recover this database. */
#define JET_errNoAttachmentsFailedIncrementalReseed -1229 /* The incremental reseed being performed on the specified database cannot be completed because the min required log contains no attachment info.  A full reseed is required to recover this database. */
// begin_PubEsent

#define JET_errDatabaseNotReady             -1230 /* Recovery on this database has not yet completed enough to permit access. */
#define JET_errDatabaseAttachedForRecovery  -1231 /* Database is attached but only for recovery.  It must be explicitly attached before it can be opened.  */
#define JET_errTransactionsNotReadyDuringRecovery -1232  /* Recovery has not seen any Begin0/Commit0 records and so does not know what trxBegin0 to assign to this transaction */


#define JET_wrnTableEmpty                    1301 /* Opened an empty table */
#define JET_errTableLocked                  -1302 /* Table is exclusively locked */
#define JET_errTableDuplicate               -1303 /* Table already exists */
#define JET_errTableInUse                   -1304 /* Table is in use, cannot lock */
#define JET_errObjectNotFound               -1305 /* No such table or object */
#define JET_errDensityInvalid               -1307 /* Bad file/index density */
#define JET_errTableNotEmpty                -1308 /* Table is not empty */
#define JET_errInvalidTableId               -1310 /* Invalid table id */
#define JET_errTooManyOpenTables            -1311 /* Cannot open any more tables (cleanup already attempted) */
#define JET_errIllegalOperation             -1312 /* Oper. not supported on table */
#define JET_errTooManyOpenTablesAndCleanupTimedOut  -1313 /* Cannot open any more tables (cleanup attempt failed to complete) */
#define JET_errObjectDuplicate              -1314 /* Table or object name in use */
#define JET_errInvalidObject                -1316 /* Object is invalid for operation */
#define JET_errCannotDeleteTempTable        -1317 /* Use CloseTable instead of DeleteTable to delete temp table */
#define JET_errCannotDeleteSystemTable      -1318 /* Illegal attempt to delete a system table */
#define JET_errCannotDeleteTemplateTable    -1319 /* Illegal attempt to delete a template table */
// end_PubEsent
#define errFCBTooManyOpen                   -1320 /* Cannot open any more FCB's (cleanup not yet attempted) */
#define errFCBAboveThreshold                -1321 /* Can only allocate FCB above preferred threshold (cleanup not yet attempted) */
// begin_PubEsent
#define JET_errExclusiveTableLockRequired   -1322 /* Must have exclusive lock on table. */
#define JET_errFixedDDL                     -1323 /* DDL operations prohibited on this table */
#define JET_errFixedInheritedDDL            -1324 /* On a derived table, DDL operations are prohibited on inherited portion of DDL */
#define JET_errCannotNestDDL                -1325 /* Nesting of hierarchical DDL is not currently supported. */
#define JET_errDDLNotInheritable            -1326 /* Tried to inherit DDL from a table not marked as a template table. */
#define JET_wrnTableInUseBySystem            1327 /* System cleanup has a cursor open on the table */
#define JET_errInvalidSettings              -1328 /* System parameters were set improperly */
#define JET_errClientRequestToStopJetService            -1329   /* Client has requested stop service */
#define JET_errCannotAddFixedVarColumnToDerivedTable    -1330   /* Template table was created with NoFixedVarColumnsInDerivedTables */
// end_PubEsent
#define errFCBExists                        -1331 /* Tried to create an FCB that already exists */
#define errFCBUnusable                      -1332 /* Placeholder to mark an FCB that must be purged as unusable */
#define wrnCATNoMoreRecords                  1333 /* Attempted to navigate past the end of the catalog */
// begin_PubEsent

/*  DDL errors
/**/
// Note: Some DDL errors have snuck into other categories.
#define JET_errIndexCantBuild               -1401 /* Index build failed */
#define JET_errIndexHasPrimary              -1402 /* Primary index already defined */
#define JET_errIndexDuplicate               -1403 /* Index is already defined */
#define JET_errIndexNotFound                -1404 /* No such index */
#define JET_errIndexMustStay                -1405 /* Cannot delete clustered index */
#define JET_errIndexInvalidDef              -1406 /* Illegal index definition */
#define JET_errInvalidCreateIndex           -1409 /* Invalid create index description */
#define JET_errTooManyOpenIndexes           -1410 /* Out of index description blocks */
#define JET_errMultiValuedIndexViolation    -1411 /* Non-unique inter-record index keys generated for a multivalued index */
#define JET_errIndexBuildCorrupted          -1412 /* Failed to build a secondary index that properly reflects primary index */
#define JET_errPrimaryIndexCorrupted        -1413 /* Primary index is corrupt. The database must be defragmented or the table deleted. */
#define JET_errSecondaryIndexCorrupted      -1414 /* Secondary index is corrupt. The database must be defragmented or the affected index must be deleted. If the corrupt index is over Unicode text, a likely cause is a sort-order change. */
#define JET_wrnCorruptIndexDeleted           1415 /* Out of date index removed */
#define JET_errInvalidIndexId               -1416 /* Illegal index id */
#define JET_wrnPrimaryIndexOutOfDate         1417 /* The Primary index is created with an incompatible OS sort version. The table can not be safely modified. */
#define JET_wrnSecondaryIndexOutOfDate       1418 /* One or more Secondary index is created with an incompatible OS sort version. Any index over Unicode text should be deleted. */

#define JET_errIndexTuplesSecondaryIndexOnly        -1430   //  tuple index can only be on a secondary index
#define JET_errIndexTuplesTooManyColumns            -1431   //  tuple index may only have eleven columns in the index
#define JET_errIndexTuplesOneColumnOnly             JET_errIndexTuplesTooManyColumns    /* OBSOLETE */
#define JET_errIndexTuplesNonUniqueOnly             -1432   //  tuple index must be a non-unique index
#define JET_errIndexTuplesTextBinaryColumnsOnly     -1433   //  tuple index must be on a text/binary column
#define JET_errIndexTuplesTextColumnsOnly           JET_errIndexTuplesTextBinaryColumnsOnly     /* OBSOLETE */
#define JET_errIndexTuplesVarSegMacNotAllowed       -1434   //  tuple index does not allow setting cbVarSegMac
#define JET_errIndexTuplesInvalidLimits             -1435   //  invalid min/max tuple length or max characters to index specified
#define JET_errIndexTuplesCannotRetrieveFromIndex   -1436   //  cannot call RetrieveColumn() with RetrieveFromIndex on a tuple index
#define JET_errIndexTuplesKeyTooSmall               -1437   //  specified key does not meet minimum tuple length
#define JET_errInvalidLVChunkSize                   -1438   //  Specified LV chunk size is not supported
#define JET_errColumnCannotBeEncrypted              -1439   //  Only JET_coltypLongText and JET_coltypLongBinary columns without default values can be encrypted
#define JET_errCannotIndexOnEncryptedColumn         -1440   //  Cannot index encrypted column

/*  DML errors
/**/
// Note: Some DML errors have snuck into other categories.
// Note: Some DDL errors have inappropriately snuck in here.
#define JET_errColumnLong                   -1501 /* Column value is long */
#define JET_errColumnNoChunk                -1502 /* No such chunk in long value */
#define JET_errColumnDoesNotFit             -1503 /* Field will not fit in record */
#define JET_errNullInvalid                  -1504 /* Null not valid */
#define JET_errColumnIndexed                -1505 /* Column indexed, cannot delete */
#define JET_errColumnTooBig                 -1506 /* Field length is greater than maximum */
#define JET_errColumnNotFound               -1507 /* No such column */
#define JET_errColumnDuplicate              -1508 /* Field is already defined */
#define JET_errMultiValuedColumnMustBeTagged    -1509 /* Attempted to create a multi-valued column, but column was not Tagged */
#define JET_errColumnRedundant              -1510 /* Second autoincrement or version column */
#define JET_errInvalidColumnType            -1511 /* Invalid column data type */
#define JET_wrnColumnMaxTruncated            1512 /* Max length too big, truncated */
#define JET_errTaggedNotNULL                -1514 /* No non-NULL tagged columns */
#define JET_errNoCurrentIndex               -1515 /* Invalid w/o a current index */
#define JET_errKeyIsMade                    -1516 /* The key is completely made */
#define JET_errBadColumnId                  -1517 /* Column Id Incorrect */
#define JET_errBadItagSequence              -1518 /* Bad itagSequence for tagged column */
#define JET_errColumnInRelationship         -1519 /* Cannot delete, column participates in relationship */
#define JET_wrnCopyLongValue                 1520 /* Single instance column bursted */
#define JET_errCannotBeTagged               -1521 /* AutoIncrement and Version cannot be tagged */
// end_PubEsent
#define wrnLVNoLongValues                    1522 /* Table does not have a long value tree */
#define JET_wrnTaggedColumnsRemaining        1523 /* RetrieveTaggedColumnList ran out of copy buffer before retrieving all tagged columns */
// begin_PubEsent
#define JET_errDefaultValueTooBig           -1524 /* Default value exceeds maximum size */
#define JET_errMultiValuedDuplicate         -1525 /* Duplicate detected on a unique multi-valued column */
#define JET_errLVCorrupted                  -1526 /* Corruption encountered in long-value tree */
// end_PubEsent
#define wrnLVNoMoreData                      1527 /* Reached end of LV data */
// begin_PubEsent
#define JET_errMultiValuedDuplicateAfterTruncation  -1528 /* Duplicate detected on a unique multi-valued column after data was normalized, and normalizing truncated the data before comparison */
#define JET_errDerivedColumnCorruption      -1529 /* Invalid column in derived table */
#define JET_errInvalidPlaceholderColumn     -1530 /* Tried to convert column to a primary index placeholder, but column doesn't meet necessary criteria */
#define JET_wrnColumnSkipped                 1531 /* Column value(s) not returned because the corresponding column id or itagSequence requested for enumeration was null */
#define JET_wrnColumnNotLocal                1532 /* Column value(s) not returned because they could not be reconstructed from the data at hand */
#define JET_wrnColumnMoreTags                1533 /* Column values exist that were not requested for enumeration */
#define JET_wrnColumnTruncated               1534 /* Column value truncated at the requested size limit during enumeration */
#define JET_wrnColumnPresent                 1535 /* Column values exist but were not returned by request */
#define JET_wrnColumnSingleValue             1536 /* Column value returned in JET_COLUMNENUM as a result of JET_bitEnumerateCompressOutput */
#define JET_wrnColumnDefault                 1537 /* Column value(s) not returned because they were set to their default value(s) and JET_bitEnumerateIgnoreDefault was specified */
#define JET_errColumnCannotBeCompressed     -1538 /* Only JET_coltypLongText and JET_coltypLongBinary columns can be compressed */
#define JET_wrnColumnNotInRecord             1539 /* Column value(s) not returned because they could not be reconstructed from the data in the record */
#define JET_errColumnNoEncryptionKey        -1540 /* Cannot retrieve/set encrypted column without an encryption key */
#define JET_wrnColumnReference               1541 /* Column value returned as a reference because it could not be reconstructed from the data in the record */

#define JET_errRecordNotFound               -1601 /* The key was not found */
#define JET_errRecordNoCopy                 -1602 /* No working buffer */
#define JET_errNoCurrentRecord              -1603 /* Currency not on a record */
#define JET_errRecordPrimaryChanged         -1604 /* Primary key may not change */
#define JET_errKeyDuplicate                 -1605 /* Illegal duplicate key */
#define JET_errAlreadyPrepared              -1607 /* Attempted to update record when record update was already in progress */
#define JET_errKeyNotMade                   -1608 /* No call to JetMakeKey */
#define JET_errUpdateNotPrepared            -1609 /* No call to JetPrepareUpdate */
#define JET_wrnDataHasChanged                1610 /* Data has changed */
#define JET_errDataHasChanged               -1611 /* Data has changed, operation aborted */
#define JET_wrnKeyChanged                    1618 /* Moved to new key */
#define JET_errLanguageNotSupported         -1619 /* Windows installation does not support language */
#define JET_errDecompressionFailed          -1620 /* Internal error: data could not be decompressed */
#define JET_errUpdateMustVersion            -1621 /* No version updates only for uncommitted tables */
#define JET_errDecryptionFailed             -1622 /* Data could not be decrypted */
#define JET_errEncryptionBadItag            -1623 /* Cannot encrypt tagged columns with itag>1 */

/*  Sort Table errors
/**/
#define JET_errTooManySorts                 -1701 /* Too many sort processes */
#define JET_errInvalidOnSort                -1702 /* Invalid operation on Sort */

/*  Other errors
/**/
#define JET_errTempFileOpenError            -1803 /* Temp file could not be opened */
#define JET_errTooManyAttachedDatabases     -1805 /* Too many open databases */
#define JET_errDiskFull                     -1808 /* No space left on disk */
#define JET_errPermissionDenied             -1809 /* Permission denied */
#define JET_errFileNotFound                 -1811 /* File not found */
#define JET_errFileInvalidType              -1812 /* Invalid file type */
#define JET_wrnFileOpenReadOnly              1813 /* Database file is read only */
#define JET_errFileAlreadyExists            -1814 /* File already exists */


#define JET_errAfterInitialization          -1850 /* Cannot Restore after init. */
#define JET_errLogCorrupted                 -1852 /* Logs could not be interpreted */


#define JET_errInvalidOperation             -1906 /* Invalid operation */
#define JET_errAccessDenied                 -1907 /* Access denied */
#define JET_wrnIdleFull                      1908 /* Idle registry full */
#define JET_errTooManySplits                -1909 /* Infinite split */
#define JET_errSessionSharingViolation      -1910 /* Multiple threads are using the same session */
#define JET_errEntryPointNotFound           -1911 /* An entry point in a DLL we require could not be found */
#define JET_errSessionContextAlreadySet     -1912 /* Specified session already has a session context set */
#define JET_errSessionContextNotSetByThisThread -1913 /* Tried to reset session context, but current thread did not originally set the session context */
#define JET_errSessionInUse                 -1914 /* Tried to terminate session in use */
#define JET_errRecordFormatConversionFailed -1915 /* Internal error during dynamic record format conversion */
#define JET_errOneDatabasePerSession        -1916 /* Just one open user database per session is allowed (JET_paramOneDatabasePerSession) */
#define JET_errRollbackError                -1917 /* error during rollback */
#define JET_errFlushMapVersionUnsupported   -1918 /* The version of the persisted flush map is not supported by this version of the engine. */
#define JET_errFlushMapDatabaseMismatch     -1919 /* The persisted flush map and the database do not match. */
#define JET_errFlushMapUnrecoverable        -1920 /* The persisted flush map cannot be reconstructed. */

// end_PubEsent
#define JET_errRBSFileCorrupt               -1921  /* RBS file is corrupt */
#define JET_errRBSHeaderCorrupt             -1922  /* RBS header is corrupt */
#define JET_errRBSDbMismatch                -1923  /* RBS is out of sync with the database file */
#define errRBSAttachInfoNotFound            -1924  /* Couldn't find the RBS attach info we wanted */
#define JET_errBadRBSVersion                -1925  /* Version of revert snapshot file is not compatible with Jet version */
#define JET_errOutOfRBSSpace                -1926  /* Revert snapshot file has reached its maximum size */
#define JET_errRBSInvalidSign               -1927  /* RBS signature is not set in the RBS header */
#define JET_errRBSInvalidRecord             -1928  /* Invalid RBS record found in the revert snapshot */
#define JET_errRBSRCInvalidRBS              -1929  /* The database cannot be reverted to the expected time as there are some invalid revert snapshots to revert to that time. */
#define JET_errRBSRCNoRBSFound              -1930  /* The database cannot be reverted to the expected time as there no revert snapshots to revert to that time. */
#define JET_errRBSRCBadDbState              -1931  /* The database revert to the expected time failed as the database has a bad dbstate. */
#define JET_errRBSMissingReqLogs            -1932  /* The required logs for the revert snapshot are missing. */
#define JET_errRBSLogDivergenceFailed       -1933  /* The required logs for the revert snapshot are diverged with logs in the log directory. */
#define JET_errRBSRCCopyLogsRevertState     -1934  /* The database cannot be reverted to the expected time as we are in copying logs stage from previous revert request and a further revert in the past is requested which might leave logs in corrupt state. */
#define JET_errDatabaseIncompleteRevert     -1935  /* The database cannot be attached because it is currently being reverted using revert snapshot. */
#define JET_errRBSRCRevertCancelled         -1936  /* The database revert has been cancelled. */
#define JET_errRBSRCInvalidDbFormatVersion  -1937  /* The database format version for the databases to be reverted doesn't support applying the revert snapshot. */
#define JET_errRBSCannotDetermineDivergence -1938  /* The required logs for the revert snapshot are missing in log directory and hence we cannot determine if those logs are diverged with the logs in snapshot directory. */
#define errRBSRequiredRangeTooLarge         -1939  /* RBS was not created as the required range was too large and we don't want to start revert snapshot from such a state. */
#define errRBSPatching                      -1940  /* RBS was not created as RBS is being attached for patching purposes, usually due to incremental reseed or page patching on the databases attached to the RBS. */
// begin_PubEsent

#define JET_wrnDefragAlreadyRunning          2000 /* Online defrag already running on specified database */
#define JET_wrnDefragNotRunning              2001 /* Online defrag not running on specified database */
// end_PubEsent
#define JET_wrnDatabaseScanAlreadyRunning    2002 /* JetDatabaseScan already running on specified database */
#define JET_wrnDatabaseScanNotRunning        2003 /* JetDatabaseScan not running on specified database */
// begin_PubEsent
#define JET_errDatabaseAlreadyRunningMaintenance -2004  /* The operation did not complete successfully because the database is already running maintenance on specified database */
// end_PubEsent
#define wrnOLD2TaskSlotFull                  2005 /* Online defrag task slots are full */
// begin_PubEsent


#define JET_wrnCallbackNotRegistered         2100 /* Unregistered a non-existent callback function */
#define JET_errCallbackFailed               -2101 /* A callback failed */
#define JET_errCallbackNotResolved          -2102 /* A callback function could not be found */

#define JET_errSpaceHintsInvalid            -2103 /* An element of the JET space hints structure was not correct or actionable. */

#define JET_errOSSnapshotInvalidSequence    -2401 /* OS Shadow copy API used in an invalid sequence */
#define JET_errOSSnapshotTimeOut            -2402 /* OS Shadow copy ended with time-out */
#define JET_errOSSnapshotNotAllowed         -2403 /* OS Shadow copy not allowed (backup or recovery in progress) */
#define JET_errOSSnapshotInvalidSnapId      -2404 /* invalid JET_OSSNAPID */
// end_PubEsent
#define errOSSnapshotNewLogStopped          -2405 /* new log file creation stopped in with edbtmp.[log|jtx] created */

/** TEST INJECTION ERRORS
 **/
#define JET_errTooManyTestInjections        -2501 /* Internal test injection limit hit */
#define JET_errTestInjectionNotSupported    -2502 /* Test injection not supported */
// begin_PubEsent

// end_PubEsent
/** PRODUCE and CONSUME LOG DATA ERRORS
 **/
#define JET_errInvalidLogDataSequence       -2601 /* Some how the log data provided got out of sequence with the current state of the instance */
#define JET_wrnPreviousLogFileIncomplete    2602 /* The log data provided jumped to the next log suddenly, we have deleted the incomplete log file as a precautionary measure */
// begin_PubEsent

#define JET_errLSCallbackNotSpecified       -3000 /* Attempted to use Local Storage without a callback function being specified */
#define JET_errLSAlreadySet                 -3001 /* Attempted to set Local Storage for an object which already had it set */
#define JET_errLSNotSet                     -3002 /* Attempted to retrieve Local Storage from an object which didn't have it set */

/** FILE and DISK ERRORS
 **/
//JET_errFileAccessDenied                   -1032
//JET_errFileNotFound                       -1811
//JET_errInvalidFilename                    -1044
#define JET_errFileIOSparse                 -4000 /* an I/O was issued to a location that was sparse */
#define JET_errFileIOBeyondEOF              -4001 /* a read was issued to a location beyond EOF (writes will expand the file) */
#define JET_errFileIOAbort                  -4002 /* instructs the JET_ABORTRETRYFAILCALLBACK caller to abort the specified I/O */
#define JET_errFileIORetry                  -4003 /* instructs the JET_ABORTRETRYFAILCALLBACK caller to retry the specified I/O */
#define JET_errFileIOFail                   -4004 /* instructs the JET_ABORTRETRYFAILCALLBACK caller to fail the specified I/O */
#define JET_errFileCompressed               -4005 /* read/write access is not supported on compressed files */
// end_PubEsent
#define errDiskTilt                             -4006 /* too much concurrent IO outstanding */
// was: wrnDiskGameOn                       4007 /* the respective IO dispatch queue has gotten low */
// wrnIOHeapNotReserved                     4008
#define wrnIOPending                        4009 /* IO is pending in the OS */
#define wrnIOSlow                       4010 /* IO completed but took abnormally long to return from the OS */
// begin_PubEsent

/**********************************************************************/
/***********************     PROTOTYPES      **************************/
/**********************************************************************/

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

#endif // JET_VERSION >= 0x0501

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetInit3W(
    _Inout_opt_ JET_INSTANCE *  pinstance,
    _In_opt_ JET_RSTINFO_W *    prstInfo,
    _In_ JET_GRBIT              grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetInit3 JetInit3W
#else
#define JetInit3 JetInit3A
#endif
#endif


#endif // JET_VERSION >= 0x0600

// end_PubEsent
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#ifdef JET_UNICODE
#define JetInit4 JetInit4W
#else
#define JetInit4 JetInit4A
#endif

#endif // JET_VERSION >= 0x0A01
// begin_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateInstanceW(
    _Out_ JET_INSTANCE *    pinstance,
    _In_opt_ JET_PCWSTR     szInstanceName );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateInstance2 JetCreateInstance2W
#else
#define JetCreateInstance2 JetCreateInstance2A
#endif
#endif

#endif // JET_VERSION >= 0x0501

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetInstanceMiscInfo(
    _In_ JET_INSTANCE               instance,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax,
    _In_ unsigned long              InfoLevel );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0600

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetTerm(
    _In_ JET_INSTANCE   instance );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetTerm2(
    _In_ JET_INSTANCE   instance,
    _In_ JET_GRBIT      grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetStopService();

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetStopServiceInstance(
    _In_ JET_INSTANCE   instance );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0501

#if ( JET_VERSION >= 0x0602 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetStopServiceInstance2(
    _In_ JET_INSTANCE       instance,
    _In_ const JET_GRBIT    grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0602

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetStopBackup();

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetStopBackupInstance(
    _In_ JET_INSTANCE   instance );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0501

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetGetSystemParameter JetGetSystemParameterW
#else
#define JetGetSystemParameter JetGetSystemParameterA
#endif
#endif

// end_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif // JET_VERSION >= 0x0603
// begin_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetEnableMultiInstanceW(
    _In_reads_opt_( csetsysparam ) JET_SETSYSPARAM_W *  psetsysparam,
    _In_ unsigned long                                  csetsysparam,
    _Out_opt_ unsigned long *                           pcsetsucceed );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetEnableMultiInstance JetEnableMultiInstanceW
#else
#define JetEnableMultiInstance JetEnableMultiInstanceA
#endif
#endif

#endif // JET_VERSION >= 0x0501

// end_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

// begin_PubEsent

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetThreadStats(
    _Out_writes_bytes_( cbMax ) void *  pvResult,
    _In_ unsigned long              cbMax );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0600

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetBeginSessionW(
    _In_ JET_INSTANCE   instance,
    _Out_ JET_SESID *   psesid,
    _In_opt_ JET_PCWSTR szUserName,
    _In_opt_ JET_PCWSTR szPassword );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetEndSession(
    _In_ JET_SESID  sesid,
    _In_ JET_GRBIT  grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

// end_PubEsent
#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetGetSessionInfo(
    _In_ JET_SESID                  sesid,
    _Out_writes_bytes_( cbMax ) void *  pvResult,
     _In_ const unsigned long        cbMax,
      _In_ const unsigned long        ulInfoLevel );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif // JET_VERSION >= 0x0600

// begin_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

// end_PubEsent

#if ( JET_VERSION >= 0x0A01 )

//  Database parameters
//
//      JET_dbparamBase                     8192     //  All JET_dbparams designed to be distinct from system / JET_params and JET_sesparams for code defense.

#define JET_dbparamDbSizeMaxPages           8193     //  This is equivalent to the cpgDatabaseSizeMax parameter passed into JetAttachDatabase2 and
                                                     //  JetCreateDatabase2, i.e., it determines the maximum size of the database in number of pages.

#define JET_dbparamCachePriority            8194     //  Cache priority to be assigned to database pages from a specific database.
                                                     //  See comment next to JET_paramCachePriority for how JET_sesparamCachePriority,
                                                     //  JET_dbparamCachePriority and JET_paramCachePriority interact.

#define JET_dbparamShrinkDatabaseOptions    8195     //  Options for database shrink. The default value is 0 (no options set).

#define JET_dbparamShrinkDatabaseTimeQuota  8196     //  Time quota (in msec) allocated to database shrink. -1 means "unlimited".
                                                     //  Valid range is 0 - 7 days, in addition to -1. The default value is -1.

#define JET_dbparamShrinkDatabaseSizeLimit  8197     //  Size below which the engine should stop attempting to shrink a database during attach, in pages.
                                                     //  The default value is 0 (i.e., shrink until it times out or there is no available space to move
                                                     //  data into).

#define JET_dbparamLeakReclaimerEnabled     8198    //  Enable reclaiming leaks during database attachment.

#define JET_dbparamLeakReclaimerTimeQuota   8199    //  Time quota (in msec) allocated to database leak reclaimer. -1 means "unlimited".
                                                    //  Valid range is 0 - 7 days, in addition to -1. The default value is -1.

#define JET_dbparamMaintainExtentPageCountCache 8200 //  If non-zero, the ExtentPageCountCache table will be created and maintained.
                                                     //  If zero, the ExtentPageCountCache table will be removed, if it exists.
                                                     //  The default value is 0 (i.e. remove unless explicitly told to keep).

#define JET_dbparamMaxValueInvalid          8201     //  This is not a valid database parameter. It can change from release to release!


// Values for JET_dbparamShrinkDatabaseOptions.
//

#define JET_bitShrinkDatabaseEofOnAttach                                0x00000001  // Resizes the database file during its attachment.

#define JET_bitShrinkDatabaseFullCategorizationOnAttach                 0x00000002  // Enables full space categorization when shrinking the database
                                                                                    // at attachment time. Enabling this may incur extra cost when
                                                                                    // shrinking the database, but avoids a potential small extra
                                                                                    // cost afterwards, when operating on the shrunk database.

#define JET_bitShrinkDatabaseDontMoveRootsOnAttach                      0x00000004  // Disable root moves when shrinking the database
                                                                                    // at attachment time. NOTE: temporary, for flighting only.

#define JET_bitShrinkDatabaseDontTruncateLeakedPagesOnAttach            0x00000008  // Disable truncating leaked pages when shrinking the database
                                                                                    // at attachment time. NOTE: temporary, for flighting only.

#define JET_bitShrinkDatabaseDontTruncateIndeterminatePagesOnAttach     0x00000010  // Disable truncating indeterminate/uncategorized pages when
                                                                                    // shrinking the database at attachment time.
                                                                                    // NOTE: temporary, for flighting only.

#endif // JET_VERSION >= 0x0A01

// begin_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateDatabase2 JetCreateDatabase2W
#else
#define JetCreateDatabase2 JetCreateDatabase2A
#endif
#endif

// end_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateDatabase3 JetCreateDatabase3W
#else
#define JetCreateDatabase3 JetCreateDatabase3A
#endif
#endif // JET_VERSION >= 0x0A01

// begin_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetAttachDatabaseW(
    _In_ JET_SESID  sesid,
    _In_ JET_PCWSTR szFilename,
    _In_ JET_GRBIT  grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetAttachDatabase2 JetAttachDatabase2W
#else
#define JetAttachDatabase2 JetAttachDatabase2A
#endif
#endif

// end_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetAttachDatabase3 JetAttachDatabase3W
#else
#define JetAttachDatabase3 JetAttachDatabase3A
#endif
#endif // JET_VERSION >= 0x0A01

// begin_PubEsent

#if ( JET_VERSION < 0x0600 )
#define JetDetachDatabaseA JetDetachDatabase
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDetachDatabaseA(
    _In_ JET_SESID  sesid,
    _In_opt_ JET_PCSTR  szFilename );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDetachDatabaseW(
    _In_ JET_SESID  sesid,
    _In_opt_ JET_PCWSTR szFilename );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDetachDatabase2W(
    _In_ JET_SESID  sesid,
    _In_opt_ JET_PCWSTR szFilename,
    _In_ JET_GRBIT  grbit);

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetDetachDatabase2 JetDetachDatabase2W
#else
#define JetDetachDatabase2 JetDetachDatabase2A
#endif
#endif

#endif // JET_VERSION >= 0x0501


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetGetTableInfo JetGetTableInfoW
#else
#define JetGetTableInfo JetGetTableInfoA
#endif
#endif

// end_PubEsent
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif // JET_VERSION >= 0x0A01
// begin_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateTableColumnIndexW(
    _In_ JET_SESID              sesid,
    _In_ JET_DBID               dbid,
    _Inout_ JET_TABLECREATE_W * ptablecreate );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateTableColumnIndex2W(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _Inout_ JET_TABLECREATE2_W *    ptablecreate );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateTableColumnIndex2 JetCreateTableColumnIndex2W
#else
#define JetCreateTableColumnIndex2 JetCreateTableColumnIndex2A
#endif
#endif // JET_VERSION >= 0x0600
#endif // JET_VERSION >= 0x0501

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateTableColumnIndex3 JetCreateTableColumnIndex3W
#else
#define JetCreateTableColumnIndex3 JetCreateTableColumnIndex3A
#endif
#endif // JET_VERSION >= 0x0601

#if ( JET_VERSION >= 0x0602 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateTableColumnIndex4A(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _Inout_ JET_TABLECREATE4_A *    ptablecreate );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateTableColumnIndex4W(
    _In_ JET_SESID                  sesid,
    _In_ JET_DBID                   dbid,
    _Inout_ JET_TABLECREATE4_W *    ptablecreate );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateTableColumnIndex4 JetCreateTableColumnIndex4W
#else
#define JetCreateTableColumnIndex4 JetCreateTableColumnIndex4A
#endif
#endif // JET_VERSION >= 0x0602

// end_PubEsent
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetCreateTableColumnIndex5 JetCreateTableColumnIndex5W
#else
#define JetCreateTableColumnIndex5 JetCreateTableColumnIndex5A
#endif
#endif // JET_VERSION >= 0x0A01
// begin_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDeleteTableW(
    _In_ JET_SESID  sesid,
    _In_ JET_DBID   dbid,
    _In_ JET_PCWSTR szTableName );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetRenameTableW(
    _In_ JET_SESID  sesid,
    _In_ JET_DBID   dbid,
    _In_ JET_PCWSTR szName,
    _In_ JET_PCWSTR szNameNew );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDeleteColumnW(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCWSTR     szColumnName );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetRenameColumn JetRenameColumnW
#else
#define JetRenameColumn JetRenameColumnA
#endif
#endif


#endif // JET_VERSION >= 0x0501


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCreateIndex4W(
    _In_ JET_SESID                                  sesid,
    _In_ JET_TABLEID                                tableid,
    _In_reads_( cIndexCreate ) JET_INDEXCREATE3_W *pindexcreate,
    _In_ unsigned long                              cIndexCreate );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetDeleteIndexW(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_PCWSTR     szIndexName );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0602 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetBeginTransaction3(
    _In_ JET_SESID      sesid,
    _In_ __int64        trxid,
    _In_ JET_GRBIT      grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0602

// end_PubEsent

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetPrepareToCommitTransaction(
    _In_ JET_SESID                      sesid,
    _In_reads_bytes_( cbData ) const void * pvData,
    _In_ unsigned long                  cbData,
    _In_ JET_GRBIT                      grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

// begin_PubEsent

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
#endif // JET_VERSION >= 0x0602

JET_ERR JET_API
JetRollback(
    _In_ JET_SESID  sesid,
    _In_ JET_GRBIT  grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetGetDatabaseInfo JetGetDatabaseInfoW
#else
#define JetGetDatabaseInfo JetGetDatabaseInfoA
#endif
#endif // JET_VERSION >= 0x0600


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetGetDatabaseFileInfo JetGetDatabaseFileInfoW
#else
#define JetGetDatabaseFileInfo JetGetDatabaseFileInfoA
#endif
#endif // JET_VERSION >= 0x0600

// end_PubEsent
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#ifdef JET_UNICODE
#define JetGetLogFileInfo JetGetLogFileInfoW
#else
#define JetGetLogFileInfo JetGetLogFileInfoA
#endif
#endif // JET_VERSION >= 0x0600
// begin_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetOpenDatabase JetOpenDatabaseW
#else
#define JetOpenDatabase JetOpenDatabaseA
#endif
#endif // JET_VERSION >= 0x0600

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCloseDatabase(
    _In_ JET_SESID  sesid,
    _In_ JET_DBID   dbid,
    _In_ JET_GRBIT  grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetOpenTable JetOpenTableW
#else
#define JetOpenTable JetOpenTableA
#endif
#endif // JET_VERSION >= 0x0600


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0501

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0502

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0501

// end_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

// begin_PubEsent

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

#if ( JET_VERSION >= 0x0600 )
JET_ERR JET_API
JetGetRecordSize(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _Inout_ JET_RECSIZE *   precsize,
    _In_ const JET_GRBIT    grbit );

#endif // JET_VERSION >= 0x0600

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0601

// end_PubEsent

#if ( JET_VERSION >= 0x0A01 )

JET_ERR JET_API
JetGetRecordSize3(
    _In_ JET_SESID          sesid,
    _In_ JET_TABLEID        tableid,
    _Inout_ JET_RECSIZE3 *    precsize,
    _In_ const JET_GRBIT    grbit );

#endif // JET_VERSION >= 0x0A01

// begin_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetGetCurrentIndex JetGetCurrentIndexW
#else
#define JetGetCurrentIndex JetGetCurrentIndexA
#endif
#endif // JET_VERSION >= 0x0600


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetSetCurrentIndexW(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_opt_ JET_PCWSTR szIndexName );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetSetCurrentIndex JetSetCurrentIndexW
#else
#define JetSetCurrentIndex JetSetCurrentIndexA
#endif
#endif // JET_VERSION >= 0x0600

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetSetCurrentIndex2 JetSetCurrentIndex2W
#else
#define JetSetCurrentIndex2 JetSetCurrentIndex2A
#endif
#endif // JET_VERSION >= 0x0600


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetSetCurrentIndex3 JetSetCurrentIndex3W
#else
#define JetSetCurrentIndex3 JetSetCurrentIndex3A
#endif
#endif // JET_VERSION >= 0x0600


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetSetCurrentIndex4 JetSetCurrentIndex4W
#else
#define JetSetCurrentIndex4 JetSetCurrentIndex4A
#endif
#endif // JET_VERSION >= 0x0600


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
#endif  //  JET_VERSION >= 0x0602

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetLock(
    _In_ JET_SESID      sesid,
    _In_ JET_TABLEID    tableid,
    _In_ JET_GRBIT      grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0601

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
    _In_ JET_GRBIT                                              grbit ); // JET_bitPrereadForward, JET_bitPrereadBackward

#endif // JET_VERSION >= 0x0602

JET_ERR JET_API
JetGetBookmark(
    _In_ JET_SESID                                      sesid,
    _In_ JET_TABLEID                                    tableid,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) void *   pvBookmark,
    _In_ unsigned long                                  cbMax,
    _Out_opt_ unsigned long *                           pcbActual );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0501


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetCompact JetCompactW
#else
#define JetCompact JetCompactA
#endif
#endif // JET_VERSION >= 0x0600


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetDefragment2 JetDefragment2W
#else
#define JetDefragment2 JetDefragment2A
#endif
#endif // JET_VERSION >= 0x0600


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetDefragment3 JetDefragment3W
#else
#define JetDefragment3 JetDefragment3A
#endif
#endif // JET_VERSION >= 0x0600

#endif // JET_VERSION >= 0x0501

// end_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif // JET_VERSION >= 0x0601

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#ifdef JET_UNICODE
#define JetConvertDDL JetConvertDDLW
#else
#define JetConvertDDL JetConvertDDLA
#endif
#endif // JET_VERSION >= 0x0600


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#ifdef JET_UNICODE
#define JetUpgradeDatabase JetUpgradeDatabaseW
#else
#define JetUpgradeDatabase JetUpgradeDatabaseA
#endif
#endif // JET_VERSION >= 0x0600

#endif // JET_VERSION >= 0x0501

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif // JET_VERSION >= 0x0600

// begin_PubEsent
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetSetDatabaseSize JetSetDatabaseSizeW
#else
#define JetSetDatabaseSize JetSetDatabaseSizeA
#endif
#endif // JET_VERSION >= 0x0600

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGrowDatabase(
    _In_ JET_SESID          sesid,
    _In_ JET_DBID           dbid,
    _In_ unsigned long      cpg,
    _In_ unsigned long *    pcpgReal );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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
#endif // JET_VERSION >= 0x0602

JET_ERR JET_API
JetSetSessionContext(
    _In_ JET_SESID      sesid,
    _In_ JET_API_PTR    ulContext );

JET_ERR JET_API
JetResetSessionContext(
    _In_ JET_SESID      sesid );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

// end_PubEsent


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
#endif // JET_VERSION >= 0x0600


// begin_PubEsent

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGotoBookmark(
    _In_ JET_SESID                      sesid,
    _In_ JET_TABLEID                    tableid,
    _In_reads_bytes_( cbBookmark ) void *   pvBookmark,
    _In_ unsigned long                  cbBookmark );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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


#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0501

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetIntersectIndexes(
    _In_ JET_SESID                              sesid,
    _In_reads_( cindexrange ) JET_INDEXRANGE *  rgindexrange,
    _In_ unsigned long                          cindexrange,
    _Inout_ JET_RECORDLIST *                    precordlist,
    _In_ JET_GRBIT                              grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOpenTemporaryTable(
    _In_ JET_SESID                  sesid,
    _In_ JET_OPENTEMPORARYTABLE *   popentemporarytable );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0600

#if ( JET_VERSION >= 0x0602 )

#pragma region Application Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOpenTemporaryTable2(
    _In_ JET_SESID                  sesid,
    _In_ JET_OPENTEMPORARYTABLE2 *  popentemporarytable );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0602


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetBackupW(
    _In_ JET_PCWSTR     szBackupPath,
    _In_ JET_GRBIT      grbit,
    _In_opt_ JET_PFNSTATUS  pfnStatus );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetBackup JetBackupW
#else
#define JetBackup JetBackupA
#endif
#endif // JET_VERSION >= 0x0600

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetBackupInstance JetBackupInstanceW
#else
#define JetBackupInstance JetBackupInstanceA
#endif
#endif // JET_VERSION >= 0x0600

#endif // JET_VERSION >= 0x0501


#if ( JET_VERSION < 0x0600 )
#define JetRestoreA JetRestore
#endif

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetRestoreA(
    _In_ JET_PCSTR      szSource,
    _In_opt_ JET_PFNSTATUS  pfn );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetRestoreW(
    _In_ JET_PCWSTR     szSource,
    _In_opt_ JET_PFNSTATUS  pfn );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetRestore JetRestoreW
#else
#define JetRestore JetRestoreA
#endif
#endif // JET_VERSION >= 0x0600


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetRestore2W(
    _In_ JET_PCWSTR     sz,
    _In_opt_ JET_PCWSTR szDest,
    _In_opt_ JET_PFNSTATUS  pfn );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetRestore2 JetRestore2W
#else
#define JetRestore2 JetRestore2A
#endif
#endif // JET_VERSION >= 0x0600


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetRestoreInstance JetRestoreInstanceW
#else
#define JetRestoreInstance JetRestoreInstanceA
#endif
#endif // JET_VERSION >= 0x0600

#endif // JET_VERSION >= 0x0501

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

// end_PubEsent
#if ( JET_VERSION >= 0x0A01 )

JET_ERR JET_API
JetIndexRecordCount2(
    _In_ JET_SESID              sesid,
    _In_ JET_TABLEID            tableid,
    _Out_ unsigned __int64 *    pcrec,
    _In_ unsigned __int64       crecMax );

#endif // JET_VERSION >= 0x0A01
// begin_PubEsent

JET_ERR JET_API
JetRetrieveKey(
    _In_ JET_SESID                                      sesid,
    _In_ JET_TABLEID                                    tableid,
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) void *   pvKey,
    _In_ unsigned long                                  cbMax,
    _Out_opt_ unsigned long *                           pcbActual,
    _In_ JET_GRBIT                                      grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetBeginExternalBackup(
    _In_ JET_GRBIT grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetBeginExternalBackupInstance(
    _In_ JET_INSTANCE instance,
    _In_ JET_GRBIT grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0501

// end_PubEsent
#if ( JET_VERSION >= 0x0601 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API JetBeginSurrogateBackup(
    _In_    JET_INSTANCE    instance,
    _In_        unsigned long       lgenFirst,
    _In_        unsigned long       lgenLast,
    _In_        JET_GRBIT       grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif // JET_VERSION >= 0x0601

// begin_PubEsent
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetAttachInfoW(
    _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PWSTR    wszzDatabases,
    _In_ unsigned long                                      cbMax,
    _Out_opt_ unsigned long *                               pcbActual );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetGetAttachInfo JetGetAttachInfoW
#else
#define JetGetAttachInfo JetGetAttachInfoA
#endif
#endif // JET_VERSION >= 0x0600


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetGetAttachInfoInstance JetGetAttachInfoInstanceW
#else
#define JetGetAttachInfoInstance JetGetAttachInfoInstanceA
#endif
#endif // JET_VERSION >= 0x0600

#endif // JET_VERSION >= 0x0501


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetOpenFileInstance JetOpenFileInstanceW
#else
#define JetOpenFileInstance JetOpenFileInstanceA
#endif
#endif // JET_VERSION >= 0x0600

#endif // JET_VERSION >= 0x0501

// end_PubEsent
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#ifdef JET_UNICODE
#define JetOpenFileSectionInstance JetOpenFileSectionInstanceW
#else
#define JetOpenFileSectionInstance JetOpenFileSectionInstanceA
#endif
#endif // JET_VERSION >= 0x0600


// begin_PubEsent

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetReadFile(
    _In_ JET_HANDLE                             hfFile,
    _Out_writes_bytes_to_( cb, *pcbActual ) void *  pv,
    _In_ unsigned long                          cb,
    _Out_opt_ unsigned long *                   pcbActual );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0501

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCloseFile(
    _In_ JET_HANDLE     hfFile );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetCloseFileInstance(
    _In_ JET_INSTANCE   instance,
    _In_ JET_HANDLE     hfFile );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0501

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetLogInfoW(
        _Out_writes_bytes_to_opt_( cbMax, *pcbActual ) JET_PWSTR    szzLogs,
        _In_ unsigned long                                      cbMax,
        _Out_opt_ unsigned long *                               pcbActual );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetGetLogInfo JetGetLogInfoW
#else
#define JetGetLogInfo JetGetLogInfoA
#endif
#endif // JET_VERSION >= 0x0600


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetGetLogInfoInstance JetGetLogInfoInstanceW
#else
#define JetGetLogInfoInstance JetGetLogInfoInstanceA
#endif
#endif // JET_VERSION >= 0x0600


// JET_LOGINFO is used by JetGetLogInfoInstance2(), which is internal.  // not_PubEsent
// But it's also used by JetExternalRestore2().                 // not_PubEsent
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetGetLogInfoInstance2 JetGetLogInfoInstance2W
#else
#define JetGetLogInfoInstance2 JetGetLogInfoInstance2A
#endif
#endif // JET_VERSION >= 0x0600


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetGetTruncateLogInfoInstance JetGetTruncateLogInfoInstanceW
#else
#define JetGetTruncateLogInfoInstance JetGetTruncateLogInfoInstanceA
#endif
#endif // JET_VERSION >= 0x0600


#endif // JET_VERSION >= 0x0501

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetTruncateLog( void );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0501 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetTruncateLogInstance(
    _In_ JET_INSTANCE   instance );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0501

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API JetEndExternalBackup( void );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0501

// end_PubEsent

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#if ( JET_VERSION >= 0x0601 )
JET_ERR JET_API JetEndSurrogateBackup(
    _In_    JET_INSTANCE    instance,
    _In_        JET_GRBIT       grbit );

#endif // JET_VERSION >= 0x0601

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

// begin_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetExternalRestore JetExternalRestoreW
#else
#define JetExternalRestore JetExternalRestoreA
#endif
#endif // JET_VERSION >= 0x0600


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetExternalRestore2 JetExternalRestore2W
#else
#define JetExternalRestore2 JetExternalRestore2A
#endif
#endif // JET_VERSION >= 0x0600

// end_PubEsent
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetSnapshotStartW(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PWSTR      szDatabases,
    _In_ JET_GRBIT      grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#ifdef JET_UNICODE
#define JetSnapshotStart JetSnapshotStartW
#else
#define JetSnapshotStart JetSnapshotStartA
#endif
#endif // JET_VERSION >= 0x0600

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetSnapshotStop(
    _In_ JET_INSTANCE   instance,
    _In_ JET_GRBIT      grbit);

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

// begin_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetGetInstanceInfoW(
    _Out_ unsigned long *                                           pcInstanceInfo,
    _Outptr_result_buffer_( *pcInstanceInfo ) JET_INSTANCE_INFO_W **    paInstanceInfo );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#ifdef JET_UNICODE
#define JetGetInstanceInfo JetGetInstanceInfoW
#else
#define JetGetInstanceInfo JetGetInstanceInfoA
#endif
#endif // JET_VERSION >= 0x0600

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

// end_PubEsent
#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetTracing(
    _In_ const JET_TRACEOP  traceop,
    _In_ const JET_TRACETAG tracetag,
    _In_ const JET_API_PTR  ul );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif // JET_VERSION >= 0x0600

// begin_PubEsent

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

typedef JET_API_PTR JET_OSSNAPID;   /* Snapshot Session Identifier */

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

#endif // JET_VERSION >= 0x0600

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
#endif // JET_VERSION >= 0x0600

JET_ERR JET_API
JetOSSnapshotThaw(
    _In_ const JET_OSSNAPID snapId,
    _In_ const JET_GRBIT    grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0501

#if ( JET_VERSION >= 0x0502 )

#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetOSSnapshotAbort(
    _In_ const JET_OSSNAPID snapId,
    _In_ const JET_GRBIT    grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0502

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0600

// end_PubEsent
#if ( JET_VERSION >= 0x0600 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetGetPageInfo(
    _In_reads_bytes_( cbData ) void * const         pvPages,        //  raw page data
    _In_ unsigned long                          cbData,         //  size of raw page data
    _Inout_updates_bytes_( cbPageInfo ) JET_PAGEINFO *  rgPageInfo,     //  array of pageinfo structures
    _In_ unsigned long                          cbPageInfo,     //  length of buffer for pageinfo array
    _In_ JET_GRBIT                              grbit,          //  options
    _In_ unsigned long                          ulInfoLevel );  //  info level

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif // JET_VERSION >= 0x0600

#if ( JET_VERSION >= 0x0601 )

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

JET_ERR JET_API
JetGetPageInfo2(
    _In_reads_bytes_( cbData ) void * const         pvPages,        //  raw page data
    _In_ unsigned long                          cbData,         //  size of raw page data
    _Inout_updates_bytes_( cbPageInfo ) void * const    rgPageInfo,     //  array of pageinfo structures
    _In_ unsigned long                          cbPageInfo,     //  length of buffer for pageinfo array
    _In_ JET_GRBIT                              grbit,          //  options
    _In_ unsigned long                          ulInfoLevel );  //  info level

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

// JetPatchDatabasePages works on an offline database and updates
// the header. This API operates on an online database and doesn't
// update the header.

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif // JET_VERSION >= 0x602

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
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
    _In_ unsigned long  genFirstDivergedLog,
    _In_ JET_GRBIT      grbit );

JET_ERR JET_API
JetBeginDatabaseIncrementalReseedW(
    _In_ JET_INSTANCE   instance,
    _In_ JET_PCWSTR     szDatabase,
    _In_ unsigned long  genFirstDivergedLog,
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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#ifdef JET_UNICODE
#define JetPatchDatabasePages JetPatchDatabasePagesW
#else
#define JetPatchDatabasePages JetPatchDatabasePagesA
#endif


#endif // JET_VERSION >= 0x0601

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
    _In_    JET_INSTANCE    instance,
    _In_    JET_LOGTIME     jltRevertExpected,
    _In_    long            cpgCache,
    _In_    JET_GRBIT       grbit,
    _Out_   JET_LOGTIME*    pjltRevertActual );

JET_ERR JET_API 
JetRBSExecuteRevert(
    _In_    JET_INSTANCE    instance,
    _In_    JET_GRBIT       grbit,
    _Out_   JET_RBSREVERTINFOMISC*  prbsrevertinfomisc );

JET_ERR JET_API
JetRBSCancelRevert(
    _In_    JET_INSTANCE    instance );

#endif // JET_VERSION >= 0x0A01

// begin_PubEsent


#if ( JET_VERSION >= 0x0601 )

//  Options for JetConfigureProcessForCrashDump

#define JET_bitDumpMinimum                      0x00000001
//  dump minimum includes cache minimum
#define JET_bitDumpMaximum                      0x00000002
//  dump maximum includes dump minimum
//  dump maximum includes cache maximum
#define JET_bitDumpCacheMinimum                 0x00000004
//  cache minimum includes pages that are latched
//  cache minimum includes pages that are used for memory
//  cache minimum includes pages that are flagged with errors
#define JET_bitDumpCacheMaximum                 0x00000008
//  cache maximum includes cache minimum
//  cache maximum includes the entire cache image
#define JET_bitDumpCacheIncludeDirtyPages       0x00000010
//  dump includes pages that are modified
#define JET_bitDumpCacheIncludeCachedPages      0x00000020
//  dump includes pages that contain valid data
#define JET_bitDumpCacheIncludeCorruptedPages   0x00000040
//  dump includes pages that are corrupted (expensive to compute)
#define JET_bitDumpCacheNoDecommit              0x00000080
//  do not decommit any pages not intending to include in crash dump

// end_PubEsent

#define JET_bitDumpUnitTest                     0x80000000

//  Looks at all pages for internal testing purposes

// begin_PubEsent


#pragma region Desktop Family or Esent Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT)

JET_ERR JET_API
JetConfigureProcessForCrashDump(
    _In_ const JET_GRBIT grbit );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

#endif // JET_VERSION >= 0x0601

#if ( JET_VERSION >= 0x0601 )

// end_PubEsent
//  Opcodes for JetTestHook
typedef enum
{
    opTestHookUnitTests,                                //  takes a char*
    opTestHookTestInjection,                            //  takes a JET_TESTHOOKTESTINJECTION*
    opTestHookHookNtQueryInformationProcess,            //  takes a JET_TESTHOOKAPIHOOKING*
    opTestHookHookNtQuerySystemInformation,             //  takes a JET_TESTHOOKAPIHOOKING*
    opTestHookHookGlobalMemoryStatus,                   //  takes a JET_TESTHOOKAPIHOOKING*
    opTestHookSetNegativeTesting,                       //  takes a DWORD*
    opTestHookResetNegativeTesting,                     //  takes a DWORD*
    opTestHookThrowError,                               //  takes a JET_ERR
    opTestHookThrowAssert,                              //  takes nothing
    opTestHookUnitTests2,                               //  takes a JET_TESTHOOKUNITTEST2
    opTestHookEnforceContextFail,                       //  takes a JET_TESTHOOKAPIHOOKING*, replaces g_pfnEnforceContextFail with user-supplied callback
    opTestHookGetBFLowMemoryCallback,                   //  return pointer to function
    opTestHookTraceTestMarker,                          //  traces a test marker ETW/OSTrace event
    opTestHookGetCheckpointDepth,                       //  return value of the current checkpoint depth
    opTestHookGetOLD2Status,                            //  return whether we are processing B+ tree defrag tasks in the defrag manager
    opTestHookGetEngineTickNow,                         //  returns the TickOSTimeCurrent() from inside the engine
    opTestHookSetEngineTickTime,                        //  takes a JET_TESTHOOKTIMEINJECTION structure
    opTestHookCacheQuery,                               //  takes a JET_TESTHOOKCACHEQUERY*
    opTestHookEvictCache,                               //  takes a JET_TESTHOOKEVICTCACHE*
    opTestHookCorrupt,                                  //  takes a JET_TESTHOOKCORRUPT*
    opTestHookEnableAutoIncDeprecated,                  //  REMOVED - takes a DWORD*
    opTestHookSetErrorTrap,                             //  takes a DWORD*, set g_errTrap
    opTestHookGetTablePgnoFDP,                          //  takes a JET_TABLEID in, returns a pgno
    opTestHookAlterDatabaseFileHeader,                  //  takes a JET_TESTHOOKALTERDBFILEHDR.
    opTestHookGetLogTip,                                //  returns the current log tip LGPOS as a 64 bit integer
} TESTHOOK_OP;

//  This is the list of "forgiveable" sins that test can commit against ESE:
//  you must merely ask for forgiveness by setting the ESE\Debug\Negative Testing
//  registry value (keep in mind decimal, not hex) -or
//  via opTestHookSetNegativeTesting/ResetNegativeTesting

//  The integer values of the appropriate sins are here:

enum
{
    fDeletingLogFiles                       = 0x00000001,
    fCorruptingLogFiles                     = 0x00000002,
    fLockingCheckpointFile                  = 0x00000004,
    fCorruptingDbHeaders                    = 0x00000008,
    fCorruptingPagePgnos                    = 0x00000010,  // but checksum is ok.
    fLeakStuff                              = 0x00000020,
    fCorruptingWithLostFlush                = 0x00000040,
    fDisableTimeoutDeadlockDetection        = 0x00000080,
    fCorruptingPages                        = 0x00000100,
    fDiskIOError                            = 0x00000200,  // Injecting ERROR_IO_DEVICE (results in Jet_errDiskIO)
    fInvalidAPIUsage                        = 0x00000400,  // invalid usage of the JET API
    fInvalidUsage                           = 0x00000800,  // invalid usage of some sub-component during unit testing
    fCorruptingPageLogically                = 0x00001000,  // but checksum (and perhaps structure / consistency) is ok.
    fOutOfMemory                            = 0x00002000,
    fLeakingUnflushedIos                    = 0x00004000,  // for internal tests that do not flush file buffers before deleting the pfapi.
    fHangingIOs                             = 0x00008000,  // Simulate hung IOs
    fCorruptingWithLostFlushWithinReqRange  = 0x00010000,  // Used to disable specific asserts if lost flushes are within required range.
    fStrictIoPerfTesting                    = 0x00020000,  // Use this to turn off any fault injection or test randomization that causes unnecessary / repairative IOs or adverse IO performance effects.
    fDisableAssertReqRangeConsistentLgpos   = 0x00040000,  // Use this to turn off asserts related to setting lgposOB0 or lgposModify below min required.
};

typedef struct tagJET_TESTHOOKUNITTEST2
{
    unsigned long       cbStruct;       //  size of this structure
    char *              szTestName;     //  test name / test wildcard
    JET_DBID            dbidTestOn;     //  database to perform the internal tests against
} JET_TESTHOOKUNITTEST2;

//  Flags to be passed for JET_TestInjectHang type test injection
#define bitHangInjectSleep          0x40000000
#define mskHangInjectOptions        ( bitHangInjectSleep | 0xFFFF0000 )

//  Type of context that is being passed to JET_TESTHOOKTESTINJECTION, when opTestHookTestInjection
//  is used.
typedef enum
{
    JET_TestInjectInvalid = 0,
    JET_TestInjectMin,
    JET_TestInjectFault = JET_TestInjectMin,
    JET_TestInjectConfigOverride,
    JET_TestInjectHang,
    JET_TestInjectMax
} JET_TESTINJECTIONTYPE;

//  Options for JetTestHook( opTestHookTestInjection / JET_TESTHOOKTESTINJECTION )

#define JET_bitInjectionProbabilityPct          0x00000001  //  default, with 100% ulProbability.
#define JET_bitInjectionProbabilityCount        0x00000002  //  one shot on nth (ulProbability) evaluation.
#define JET_bitInjectionProbabilityPermanent    0x00000004  //  same as one shot, but permanent after fire (must be OR'd with JET_bitInjectionProbabilityCount).
#define JET_bitInjectionProbabilityFailUntil    0x00000008  //  same as one shot, but fails until fire (must be OR'd with JET_bitInjectionProbabilityCount).
#define JET_bitInjectionProbabilitySuppress     0x40000000  //  removes injection temporarily
#define JET_bitInjectionProbabilityCleanup      0x80000000  //  removes injection entry for the ID.

//  pv struct for opTestHookTestInjection
typedef struct tagJET_TESTHOOKTESTINJECTION
{
    unsigned long           cbStruct;
    unsigned long           ulID;
    JET_API_PTR             pv;
    JET_TESTINJECTIONTYPE   type;
    unsigned long           ulProbability;
    JET_GRBIT               grbit;
} JET_TESTHOOKTESTINJECTION;

//  pv struct for opTestHookHookNtQueryInformationProcess, opTestHookHookNtQuerySystemInformation
//  and opTestHookHookGlobalMemoryStatus
typedef struct tagJET_TESTHOOKAPIHOOKING
{
    unsigned long   cbStruct;
    const void *    pfnOld;
    const void *    pfnNew;
} JET_TESTHOOKAPIHOOKING;

//  pv struct for opTestHookTraceTestMarker
typedef struct tagJET_TESTHOOKTRACETESTMARKER
{
    unsigned long       cbStruct;
    const char *        szAnnotation;
    unsigned __int64    qwMarkerID;
} JET_TESTHOOKTRACETESTMARKER;

//  pv struct for opTestHookSetEngineTickTime
typedef struct tagJET_TESTHOOKTIMEINJECTION
{
    unsigned long       cbStruct;
    unsigned long       tickNow;
    unsigned long       eTimeInjWrapMode;
    unsigned long       dtickTimeInjWrapOffset;
    unsigned long       dtickTimeInjAccelerant;
} JET_TESTHOOKTIMEINJECTION;

//  pv struct for opTestHookCacheQuery
typedef struct tagJET_TESTHOOKCACHEQUERY
{
    unsigned long       cbStruct;

    //  in args
    long                cCacheQuery;
    char **             rgszCacheQuery;

    //  out arg
    void *              pvOut;
} JET_TESTHOOKCACHEQUERY;

#define JET_bitTestHookEvictDataByPgno  0x00000001      //  Specifies that we are evicting data from the database cache, specified by pgno.

//  pv struct for opTestHookEvictCache
typedef struct tagJET_TESTHOOKEVICTCACHE
{
    unsigned long           cbStruct;
    JET_API_PTR         ulTargetContext;        //  For ..EvictDataByPgno = JET_DBID
    JET_API_PTR         ulTargetData;           //  For ..EvictDataByPgno = PageNumber/pgno
    JET_GRBIT           grbit;
} JET_TESTHOOKEVICTCACHE;


//  args for opTestHookCorrupt

#define JET_bitTestHookCorruptDatabaseFile  0x80000000      //  Use .CorruptDatabaseFile
#define JET_bitTestHookCorruptDatabasePageImage 0x40000000      //  Use .CorruptDatabasePageImage
#define JET_mskTestHookCorruptFileType      ( JET_bitTestHookCorruptDatabaseFile | JET_bitTestHookCorruptDatabasePageImage )

#define JET_bitTestHookCorruptPageChksumRand    0x00000001      //  usable with ...CorruptDatabase*
#define JET_bitTestHookCorruptPageChksumSafe    0x00000002      //  usable with ...CorruptDatabase*
#define JET_bitTestHookCorruptPageSingleFld 0x00000004      //  usable with ...CorruptDatabase*
#define JET_bitTestHookCorruptPageRemoveNode    0x00000008      //  usable with ...CorruptDatabase*
#define JET_bitTestHookCorruptPageDbtimeDelta   0x00000010      //  usable with ...CorruptDatabase*
#define JET_bitTestHookCorruptNodePrefix    0x00000020      //  usable with ...CorruptDatabase*
#define JET_bitTestHookCorruptNodeSuffix    0x00000040      //  usable with ...CorruptDatabase*

#define JET_mskTestHookCorruptDataType      ( JET_bitTestHookCorruptPageChksumRand | JET_bitTestHookCorruptPageChksumSafe | JET_bitTestHookCorruptPageSingleFld | JET_bitTestHookCorruptPageRemoveNode | JET_bitTestHookCorruptPageDbtimeDelta )

//  Following usable only with JET_bitTestHookCorruptNodePrefix | JET_bitTestHookCorruptNodeSuffix    
#define JET_bitTestHookCorruptSizeLargerThanNode    0x00010000  //  adds a cb that is just larger than the node / line.cb size.
#define JET_bitTestHookCorruptSizeShortWrapSmall    0x00020000  //  adds 0x8000 to the cb.
#define JET_bitTestHookCorruptSizeShortWrapLarge    0x00040000  //  adds 0xF000 to the cb.

#define JET_mskTestHookCorruptSpecific          0x00FF0000  //  reserved for bits of a specific Node and someday Page Corruption types.


#define JET_bitTestHookCorruptLeaveChecksum 0x01000000      //  usable with ...CorruptDatabase*

#define JET_pgnoTestHookCorruptRandom       0xFFFFFFFFFFFFFFFFLL    //  Specifies to select the page number randomly

typedef struct tagJET_TESTHOOKCORRUPT
{
    unsigned long           cbStruct;
    JET_GRBIT           grbit;

    union
    {
        // ESE tries to pack as densely as possible, which can result in misalignment.
#include <pshpack8.h>
        struct // CorruptDatabaseFile
        {
            JET_PWSTR   wszDatabaseFilePath;        //  Name of the database file
            __int64     pgnoTarget;         //  Page number target, or JET_pgnoTestHookCorruptRandom
            __int64     iSubTarget;         //  Depends upon the JET_bitTestHookCorruptPage* type.
        } CorruptDatabaseFile;
#include <poppack.h>

        struct // CorruptDatabasePageImage
        {
            JET_API_PTR pbPageImageTarget;      //  Pointer to the page image to corrupt
            unsigned long   cbPageImage;
            __int64     pgnoTarget;         //  Page number target (note: this may not seem like it should be required, but it is b/c 4 KB pages xor this into the checksum)
            __int64     iSubTarget;         //  Depends upon the JET_bitTestHookCorruptPage* type.
        } CorruptDatabasePageImage;
    };

} JET_TESTHOOKCORRUPT;


//  args for opTestHookAlterDatabaseFileHeader / JET_TESTHOOKALTERDBFILEHDR

#define JET_bitAlterDbFileHdrAddField                       0x1     //  Makes it so the pbField (but only if cbField is 4 or 8 bytes) be interpreted as an long or long long.

#define JET_ibfieldDbFileHdrMajorVersion                    0x008   //  Sets or alters the DB's Major Version value.
#define JET_ibfieldDbFileHdrUpdateMajor                     0x0e8   //  Sets or alters the DAE Update Major version value.
#define JET_ibfieldDbFileHdrUpdateMinor                     0x284   //  Sets or alters the DAE Update Minor version value.


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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

// begin_PubEsent

#endif // JET_VERSION >= 0x0601

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_PKG_ESENT) */
#pragma endregion

// end_PubEsent

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

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

// begin_PubEsent

#endif // JET_VERSION >= 0x0602

// end_PubEsent

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

#endif // JET_VERSION >= 0x0A00

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

#endif // JET_VERSION >= 0x0A01

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

#endif // JET_VERSION >= 0x0A01

// begin_PubEsent

#ifdef  __cplusplus
} // extern "C" - Note: from the beginning of the #if !defined(_JET_NOPROTOTYPES) section.
#endif

#endif  /* _JET_NOPROTOTYPES */

#include <poppack.h>

#ifdef  __cplusplus
} // extern "C"
#endif

#endif  /* _JET_INCLUDED */

// end_PubEsent

