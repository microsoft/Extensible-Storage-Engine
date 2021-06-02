// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_PERFMON_HXX_INCLUDED
#define _OS_PERFMON_HXX_INCLUDED

#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with intsafe, someone else owns that.")
#pragma prefast(disable:28252, "Do not bother us with intsafe, someone else owns that.")
#pragma prefast(disable:28253, "Do not bother us with intsafe, someone else owns that.")
#include <intsafe.h>
#pragma prefast(pop)

#include "cc.hxx"
#include "math.hxx"

#define PERF_COLLECTION_FREQ_MAX    10      //  Maximum number of times collection can be triggered per second.
#define PERF_TIMEOUT                100     //  Timeout waiting for server processes to respond.
#define PERF_COPY_RETRY             1000    //  Number of times we should retry copying data to shared memory (defense-in-depth).
#define PERF_PERFINST_MAX           256     //  Maximum number of server processes.
#define PERF_AGGREGATION_MAX        16      //  Maximum number of process classes requesting aggregation.
#define PERF_INVALID_CHARS          L"#/\\" //  Invalid characters for perf. counter instances.
#define PERF_INVALID_CHAR_REPL      L'?'    //  Replacement character for when invalid characters are found.

//
//  ADDING A PERFORMANCE OBJECT OR COUNTER
//
//  In order to add an object or counter to a translation unit, you must
//  perform the following steps:
//
//      o  add the object/counter's information to the performance
//         database in perfdata.txt (see this file for details)
//      o  define all data/functions referenced by the object/counter's
//         information in the database
//

#define PERF_OBJECT_ESE         (0)
#define PERF_OBJECT_TABLECLASS  (1)
#define PERF_OBJECT_INSTANCES   (2)
#define PERF_OBJECT_DATABASES   (3)
#define PERF_OBJECT_INVALID     (4)

//  The performance object currently being processed by the thread

// Using unsigned int b/c DWORD is not defined everywhere.
UINT DwPERFCurPerfObj( void );  // perf object for which data is currently being collected


//  Default detail level

#define PERF_DETAIL_DEFAULT     100

//  Development detail level (made visible by a special registry key)

#define PERF_DETAIL_DEVONLY     0x80000000

//  Process Information Function (PIF)
//
//  Returns process information.
//
//  NOTE:  The caller is NOT responsible for freeing the string's buffer.
//         The caller is also not permitted to modify the buffer.

typedef void (PM_PIF_PROC) ( const wchar_t** const pwszFileName, bool* const pfRefreshInstanceList );


//  Instance Catalog Function (ICF)
//
//  Takes in an opcode and two generic pointers. Returns a 32-bit signed integer.
//
//  Supported opcodes are:
//
//      ICFInit:
//        o Performs any required initialization actions;
//        o Gets called when the collection thread on the host process starts;
//        o Generic pointers are not used for anything;
//        o Returns the maximum number of instances for that particular category.
//
//      ICFTerm:
//        o Performs any required cleanup actions;
//        o Gets called when the collection thread on the host process exits;
//        o Generic pointers are not used for anything;
//        o Return code is not meaningful.
//
//      ICFData:
//        o Retrieves the catalog of instances for that particular category;
//        o Gets called for every collection cycle, i.e., upon execution of
//          CollectPerformanceData();
//        o pvParam1 is interpreted as a pointer to a string array (i.e., double
//          pointer to wchar_t) and receives the list of instance names;
//        o pvParam2 is interpreted as a pointer to a byte array (i.e., double
//          pointer to unsigned char) and receives the list of aggregation IDs.
//          Counters belonging  to the same perf. counter category (i.e., same counter object)
//          with a matching aggregation ID could be potentially aggregated across multiple instances
//          of the same process. "0" means aggregation does not apply for that specific instance.
//        o Returns the cuurent number of instances.
//        o For both pvParam1 and pvParam2, the caller is NOT responsible for freeing the buffers, neither
//          it is permitted to modify the buffer.
//

typedef LONG (PM_ICF_PROC) ( _In_ LONG icf, _Inout_opt_ void* const pvParam1, _Inout_opt_ void* const pvParam2 );

#define ICFData     ( 0 )
#define ICFInit     ( 1 )
#define ICFTerm     ( 2 )


//  Counter Evaluation Function (CEF)
//
//  The function is given the index of the Instance that
//  we need counter data for and a pointer to the location
//  to store that data.
//
//  If the pointer is NULL, the passed long has the following
//  special meanings:
//
//      CEFInit:  Initialize counter for all instances (return 0 on success)
//      CEFTerm:  Terminate counter for all instances (return 0 on success)

typedef LONG (PM_CEF_PROC) ( LONG cef, void* pvBuf );

#define CEFInit     ( 1 )
#define CEFTerm     ( 2 )


//  Calculate the true size of a counter, accounting for DWORD padding

#define PerfOffsetOf( s, m )    (DWORD_PTR)&(((s *)0)->m)
#define PerfSize( _x )          ( ( _x ) & 0x300 )
#define QWORD_MULTIPLE( _x )    roundup( _x, sizeof( unsigned __int64 ) )
#define CntrSize( _a, _b )      ( PerfSize( _a ) == 0x000 ? 4                   \
                                : ( PerfSize( _a ) == 0x100 ? 8                 \
                                : ( PerfSize( _a ) == 0x200 ? 0                 \
                                : ( _b ) ) ) )


//  shared global data area

typedef struct _GDA
{
    volatile ULONG  iInstanceMax;           //  maximum instance ID ever connected
} GDA, *PGDA;

#define PERF_SIZEOF_GLOBAL_DATA     0x1000

C_ASSERT( sizeof( GDA ) <= PERF_SIZEOF_GLOBAL_DATA );

//  shared instance data area

const size_t    cchPerfmonInstanceNameMax   = 63;   //  This should be ( SIZEOF_INST_NAME - 1 ). "-1" accounts for the double NULL-termination.

#pragma warning ( disable : 4200 )  //  we allow zero sized arrays

//  IDA (Input Data Area): describes a memory region that is produced by
//  server processes and copied to shared memory before being consumed
//  by the client DLL (eseperf/esentprf).

typedef struct _IDA
{
    ULONG           cbIDA;                  //  Total allocated size of the data area.

    ULONG           tickConnect;            //  System tick count on connect.

    ULONG           cbPerformanceData;      //  Size of collected performance data.

    unsigned char           rgbPerformanceData[];   //  Collected performance data.
} IDA, *PIDA;

C_ASSERT( sizeof( IDA ) == PerfOffsetOf( IDA, rgbPerformanceData ) );

//  extern pointing to generated PERF_DATA_TEMPLATE in perfdata.c
//
//  NOTE:  the PerformanceData functions access this structure using
//  its self contained offset tree, NOT using any declaration

struct _PERF_DATA_TEMPLATE;
typedef _PERF_DATA_TEMPLATE PERF_DATA_TEMPLATE;
extern const PERF_DATA_TEMPLATE PerfDataTemplateReadOnly;
extern       PERF_DATA_TEMPLATE PerfDataTemplateReadWrite;

//  performance data version string (used to correctly match the ESE DLL
//  and eseperf DLL versions as the name of the file mapping)

extern const wchar_t wszPERFVersion[];

//  PIF/ICF/CEF function pointers in perfdata.c

extern PM_PIF_PROC ProcInfoPIFPwszPf;
extern PM_ICF_PROC* const rgpicfPERFICF[];
extern PM_CEF_PROC* const rgpcefPERFCEF[];

//  # objects in perfdata.c

extern const ULONG dwPERFNumObjects;

//  object instance data tables in perfdata.c

extern LONG rglPERFNumInstances[];
extern wchar_t* rgwszPERFInstanceList[];
extern unsigned char* rgpbPERFInstanceAggregationIDs[];

//  # counters in perfdata.c

extern const ULONG dwPERFNumCounters;

//  maximum index used for name/help text in perfdata.c

extern const ULONG dwPERFMaxIndex;


//  Performance Counter Macros
//
//  Wrap every operation on a performance counter in the appropriate macro to
//  empower us to enable/disable sets of them
//
//  -  PERFOptDeclare( x ):  Declares a variable that is used in a later 'PERFOpt' statement.
//  -  PERFOpt( x ):  this counter doesn't need to be collected
//  -  PERFZeroDisabledAndDiscouraged( x ); this counter should be collected if perfmon is enabled, but if 
//                       it is compiled out will return zero. Usage of this is DISCOURAGED.

#ifdef PERFMON_SUPPORT
extern BOOL g_fDisablePerfmon;
#define PERFOptDeclare( x ) x
#define PERFOpt( x ) \
    if ( !g_fDisablePerfmon ) \
    { x; }
#define PERFZeroDisabledAndDiscouraged( x ) \
    g_fDisablePerfmon ? 0 : ( x )
#else  //  !PERFMON_SUPPORT
#define PERFOptDeclare( x )
#define PERFOpt( x )
#define PERFZeroDisabledAndDiscouraged( x ) 0
#endif  //  PERFMON_SUPPORT

//  Some accounts need write access to performance counter objects.
//
//  This ACL was too permissive:
//      D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;AU)
//
//  Use this one instead:
//      "D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;0x120007;;;SU)(A;;0x120007;;;S-1-5-32-558)(A;;0x120007;;;S-1-5-32-559)"
//
//  The sddl gives the following groups of users the ability to provide or query
//  the performance counter data:
//      NT AUTHORITY\SYSTEM (full)
//      BUILTIN\Administrators (full)
//      NT AUTHORITY\SERVICE (restricted)
//      BUILTIN\Performance Monitor Users (restricted)
//      BUILTIN\Performance Log Users (restricted)
//
//  Note that we use explicit SIDs for MU and LU to preserve compatibility with Win2k.
//
//  The sddl is common across file mappings, mutexes, and events. The following
//  bits have the same meaning for each:
//      SYNCHRONIZE         (0x00100000)
//      READ_CONTROL        (0x00020000)
//
//  Subsequent bits vary per object type, but basically allow Read/Write access:
//
//  File mappings
//      SECTION_QUERY       (0x00000001)
//      SECTION_MAP_WRITE   (0x00000002)
//      SECTION_MAP_READ    (0x00000004)
//  Mutexes
//      MUTEX_MODIFY_STATE  (0x00000001)
//      ignored             (0x00000006)
//  Events
//      EVENT_MODIFY_STATE  (0x00000002)
//      ignored             (0x00000005)
//
#define aclPERF L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;0x120007;;;SU)(A;;0x120007;;;S-1-5-32-558)(A;;0x120007;;;S-1-5-32-559)"

// retry create/open to avoid the scenario where the object is deleted
// between the create and the open.
#define PERFCreateOpen( _hPerf, _create, _open ) \
{ \
    DWORD _nretry = 0; \
    while (    !( _hPerf = _create ) \
            && ( ERROR_ACCESS_DENIED == GetLastError() ) \
            && !( _hPerf = _open ) \
            && ( ERROR_FILE_NOT_FOUND == GetLastError() ) \
            && ( ++_nretry < 100 ) ) \
    { \
        Sleep(2); \
    } \
}

#define PERFCreateMutex( _hPerf, _sa, _wszT ) \
    PERFCreateOpen( _hPerf,  \
                    CreateMutexW( &_sa, FALSE, _wszT ), \
                    OpenMutexW( (SYNCHRONIZE | READ_CONTROL | MUTEX_MODIFY_STATE), FALSE, _wszT ) )

#define PERFCreateEvent(_hPerf, _sa, _wszT ) \
    PERFCreateOpen( _hPerf,  \
                    CreateEventW( &_sa, TRUE, FALSE, _wszT ), \
                    OpenEventW( (SYNCHRONIZE | READ_CONTROL | EVENT_MODIFY_STATE), FALSE, _wszT ) )

#define PERFCreateFileMapping( _hPerf, _sa, _wszT, _size ) \
    PERFCreateOpen( _hPerf,  \
                    CreateFileMappingW( INVALID_HANDLE_VALUE, &_sa, PAGE_READWRITE | SEC_COMMIT, 0, _size, _wszT ), \
                    OpenFileMappingW( (FILE_MAP_READ | FILE_MAP_WRITE), FALSE, _wszT ) )

#define PERFIsValidCharacter( _wch ) \
    ( wcschr( PERF_INVALID_CHARS, _wch ) == NULL )

#endif  //  _OS_PERFMON_HXX_INCLUDED

