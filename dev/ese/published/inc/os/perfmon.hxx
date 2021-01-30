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

#define PERF_COLLECTION_FREQ_MAX    10
#define PERF_TIMEOUT                100
#define PERF_COPY_RETRY             1000
#define PERF_PERFINST_MAX           256
#define PERF_AGGREGATION_MAX        16
#define PERF_INVALID_CHARS          L"#/\\"
#define PERF_INVALID_CHAR_REPL      L'?'


#define PERF_OBJECT_ESE         (0)
#define PERF_OBJECT_TABLECLASS  (1)
#define PERF_OBJECT_INSTANCES   (2)
#define PERF_OBJECT_DATABASES   (3)
#define PERF_OBJECT_INVALID     (4)


UINT DwPERFCurPerfObj( void );



#define PERF_DETAIL_DEFAULT     100


#define PERF_DETAIL_DEVONLY     0x80000000


typedef void (PM_PIF_PROC) ( const wchar_t** const pwszFileName, bool* const pfRefreshInstanceList );



typedef LONG (PM_ICF_PROC) ( _In_ LONG icf, _Inout_opt_ void* const pvParam1, _Inout_opt_ void* const pvParam2 );

#define ICFData     ( 0 )
#define ICFInit     ( 1 )
#define ICFTerm     ( 2 )



typedef LONG (PM_CEF_PROC) ( LONG cef, void* pvBuf );

#define CEFInit     ( 1 )
#define CEFTerm     ( 2 )



#define PerfOffsetOf( s, m )    (DWORD_PTR)&(((s *)0)->m)
#define PerfSize( _x )          ( ( _x ) & 0x300 )
#define QWORD_MULTIPLE( _x )    roundup( _x, sizeof( unsigned __int64 ) )
#define CntrSize( _a, _b )      ( PerfSize( _a ) == 0x000 ? 4                   \
                                : ( PerfSize( _a ) == 0x100 ? 8                 \
                                : ( PerfSize( _a ) == 0x200 ? 0                 \
                                : ( _b ) ) ) )



typedef struct _GDA
{
    volatile ULONG  iInstanceMax;
} GDA, *PGDA;

#define PERF_SIZEOF_GLOBAL_DATA     0x1000

C_ASSERT( sizeof( GDA ) <= PERF_SIZEOF_GLOBAL_DATA );


const size_t    cchPerfmonInstanceNameMax   = 63;

#pragma warning ( disable : 4200 )


typedef struct _IDA
{
    ULONG           cbIDA;

    ULONG           tickConnect;

    ULONG           cbPerformanceData;

    unsigned char           rgbPerformanceData[];
} IDA, *PIDA;

C_ASSERT( sizeof( IDA ) == PerfOffsetOf( IDA, rgbPerformanceData ) );


struct _PERF_DATA_TEMPLATE;
typedef _PERF_DATA_TEMPLATE PERF_DATA_TEMPLATE;
extern const PERF_DATA_TEMPLATE PerfDataTemplateReadOnly;
extern       PERF_DATA_TEMPLATE PerfDataTemplateReadWrite;


extern const wchar_t wszPERFVersion[];


extern PM_PIF_PROC ProcInfoPIFPwszPf;
extern PM_ICF_PROC* const rgpicfPERFICF[];
extern PM_CEF_PROC* const rgpcefPERFCEF[];


extern const ULONG dwPERFNumObjects;


extern LONG rglPERFNumInstances[];
extern wchar_t* rgwszPERFInstanceList[];
extern unsigned char* rgpbPERFInstanceAggregationIDs[];


extern const ULONG dwPERFNumCounters;


extern const ULONG dwPERFMaxIndex;



#ifdef PERFMON_SUPPORT
extern BOOL g_fDisablePerfmon;
#define PERFOptDeclare( x ) x
#define PERFOpt( x ) \
    if ( !g_fDisablePerfmon ) \
    { x; }
#define PERFZeroDisabledAndDiscouraged( x ) \
    g_fDisablePerfmon ? 0 : ( x )
#else
#define PERFOptDeclare( x )
#define PERFOpt( x )
#define PERFZeroDisabledAndDiscouraged( x ) 0
#endif

#define aclPERF L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;0x120007;;;SU)(A;;0x120007;;;S-1-5-32-558)(A;;0x120007;;;S-1-5-32-559)"

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

#endif

