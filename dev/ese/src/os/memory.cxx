// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

#include <math.h>
#include <malloc.h>

#include <wincred.h>
#include <SubAuth.h>
#include <Psapi.h>


// Hack - 10/08/2007 - The following definitions are coming from the Windows SDK. We cannot use them directly from the headers because of
// some incompatibilities. 
// BEGINING of Hack

/////////////////////////////////////////////////////////////////////////
//
//  Standard Error values
//
/////////////////////////////////////////////////////////////////////////

//
// MessageId: STATUS_UNSUCCESSFUL
//
// MessageText:
//
// {Operation Failed}
// The requested operation was unsuccessful.
//
#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)


//
// The success status codes 0 - 63 are reserved for wait completion status.
// FacilityCodes 0x5 - 0xF have been allocated by various drivers.
//
#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L) // ntsubauth


//
// MessageId: STATUS_INFO_LENGTH_MISMATCH
//
// MessageText:
//
// The specified information record length does not match the length required for the specified information class.
//
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)


// begin_ntddk begin_ntifs
//
// Process Information Classes
//

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,          // Note: this is kernel mode only
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    ProcessDeviceMap,
    ProcessSessionInformation,
    ProcessForegroundInformation,
    ProcessWow64Information,
    ProcessImageFileName,
    ProcessLUIDDeviceMapsEnabled,
    ProcessBreakOnTermination,
    ProcessDebugObjectHandle,
    ProcessDebugFlags,
    ProcessHandleTracing,
    MaxProcessInfoClass             // MaxProcessInfoClass should always be the last enum
} PROCESSINFOCLASS;


//
// System Information Classes.
//

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation,
    SystemProcessorInformation,             // obsolete...delete
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemPathInformation,
    SystemProcessInformation,
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
    SystemFlagsInformation,
    SystemCallTimeInformation,
    SystemModuleInformation,
    SystemLocksInformation,
    SystemStackTraceInformation,
    SystemPagedPoolInformation,
    SystemNonPagedPoolInformation,
    SystemHandleInformation,
    SystemObjectInformation,
    SystemPageFileInformation,
    SystemVdmInstemulInformation,
    SystemVdmBopInformation,
    SystemFileCacheInformation,
    SystemPoolTagInformation,
    SystemInterruptInformation,
    SystemDpcBehaviorInformation,
    SystemFullMemoryInformation,
    SystemLoadGdiDriverInformation,
    SystemUnloadGdiDriverInformation,
    SystemTimeAdjustmentInformation,
    SystemSummaryMemoryInformation,
    SystemMirrorMemoryInformation,
    SystemPerformanceTraceInformation,
    SystemObsolete0,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemExtendServiceTableInformation,
    SystemPrioritySeperation,
    SystemVerifierAddDriverInformation,
    SystemVerifierRemoveDriverInformation,
    SystemProcessorIdleInformation,
    SystemLegacyDriverInformation,
    SystemCurrentTimeZoneInformation,
    SystemLookasideInformation,
    SystemTimeSlipNotification,
    SystemSessionCreate,
    SystemSessionDetach,
    SystemSessionInformation,
    SystemRangeStartInformation,
    SystemVerifierInformation,
    SystemVerifierThunkExtend,
    SystemSessionProcessInformation,
    SystemLoadGdiDriverInSystemSpace,
    SystemNumaProcessorMap,
    SystemPrefetcherInformation,
    SystemExtendedProcessInformation,
    SystemRecommendedSharedDataAlignment,
    SystemComPlusPackage,
    SystemNumaAvailableMemory,
    SystemProcessorPowerInformation,
    SystemEmulationBasicInformation,
    SystemEmulationProcessorInformation,
    SystemExtendedHandleInformation,
    SystemLostDelayedWriteInformation,
    SystemBigPoolInformation,
    SystemSessionPoolTagInformation,
    SystemSessionMappedViewInformation
} SYSTEM_INFORMATION_CLASS;


typedef struct _SYSTEM_FILECACHE_INFORMATION {
    SIZE_T CurrentSize;
    SIZE_T PeakSize;
    ULONG PageFaultCount;
    SIZE_T MinimumWorkingSet;
    SIZE_T MaximumWorkingSet;
    SIZE_T CurrentSizeIncludingTransitionInPages;
    SIZE_T PeakSizeIncludingTransitionInPages;
    ULONG TransitionRePurposeCount;
    ULONG Flags;
} SYSTEM_FILECACHE_INFORMATION, *PSYSTEM_FILECACHE_INFORMATION;

#if defined(_WIN64)
typedef ULONG SYSINF_PAGE_COUNT;
#else
typedef SIZE_T SYSINF_PAGE_COUNT;
#endif

typedef struct _SYSTEM_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleProcessTime;
    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
    ULONG IoReadOperationCount;
    ULONG IoWriteOperationCount;
    ULONG IoOtherOperationCount;
    ULONG AvailablePages;
    SYSINF_PAGE_COUNT CommittedPages;
    SYSINF_PAGE_COUNT CommitLimit;
    SYSINF_PAGE_COUNT PeakCommitment;
    ULONG PageFaultCount;
    ULONG CopyOnWriteCount;
    ULONG TransitionCount;
    ULONG CacheTransitionCount;
    ULONG DemandZeroCount;
    ULONG PageReadCount;
    ULONG PageReadIoCount;
    ULONG CacheReadCount;
    ULONG CacheIoCount;
    ULONG DirtyPagesWriteCount;
    ULONG DirtyWriteIoCount;
    ULONG MappedPagesWriteCount;
    ULONG MappedWriteIoCount;
    ULONG PagedPoolPages;
    ULONG NonPagedPoolPages;
    ULONG PagedPoolAllocs;
    ULONG PagedPoolFrees;
    ULONG NonPagedPoolAllocs;
    ULONG NonPagedPoolFrees;
    ULONG FreeSystemPtes;
    ULONG ResidentSystemCodePage;
    ULONG TotalSystemDriverPages;
    ULONG TotalSystemCodePages;
    ULONG NonPagedPoolLookasideHits;
    ULONG PagedPoolLookasideHits;
    ULONG AvailablePagedPoolPages;
    ULONG ResidentSystemCachePage;
    ULONG ResidentPagedPoolPage;
    ULONG ResidentSystemDriverPage;
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadResourceMiss;
    ULONG CcFastReadNotPossible;
    ULONG CcFastMdlReadNoWait;
    ULONG CcFastMdlReadWait;
    ULONG CcFastMdlReadResourceMiss;
    ULONG CcFastMdlReadNotPossible;
    ULONG CcMapDataNoWait;
    ULONG CcMapDataWait;
    ULONG CcMapDataNoWaitMiss;
    ULONG CcMapDataWaitMiss;
    ULONG CcPinMappedDataCount;
    ULONG CcPinReadNoWait;
    ULONG CcPinReadWait;
    ULONG CcPinReadNoWaitMiss;
    ULONG CcPinReadWaitMiss;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
    ULONG CcCopyReadWaitMiss;
    ULONG CcMdlReadNoWait;
    ULONG CcMdlReadWait;
    ULONG CcMdlReadNoWaitMiss;
    ULONG CcMdlReadWaitMiss;
    ULONG CcReadAheadIos;
    ULONG CcLazyWriteIos;
    ULONG CcLazyWritePages;
    ULONG CcDataFlushes;
    ULONG CcDataPages;
    ULONG ContextSwitches;
    ULONG FirstLevelTbFills;
    ULONG SecondLevelTbFills;
    ULONG SystemCalls;
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;


//
// Process Virtual Memory Counters
//  NtQueryInformationProcess using ProcessVmCounters
//

typedef struct _VM_COUNTERS {
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} VM_COUNTERS;
typedef VM_COUNTERS *PVM_COUNTERS;

// END of Hack

#if 0
#if defined( MINIMAL_FUNCTIONALITY )        //  defined( MINIMAL_FUNCTIONALITY )
//#define ENABLE_VM_MEM_COUNTERS
//#define ENABLE_MM_MEM_COUNTERS
#elif defined( DEBUG )                      //  !defined( MINIMAL_FUNCTIONALITY ) && defined( DEBUG )
#define ENABLE_VM_MEM_COUNTERS
//#define ENABLE_MM_MEM_COUNTERS
#else                                       //  !defined( MINIMAL_FUNCTIONALITY ) && !defined( DEBUG )
//#define ENABLE_VM_MEM_COUNTERS
//#define ENABLE_MM_MEM_COUNTERS
#endif
#endif

#if defined( MINIMAL_FUNCTIONALITY )        //  defined( MINIMAL_FUNCTIONALITY )
//#define ENABLE_HEAP_MEM_COUNTERS
#elif defined( DEBUG )                      //  !defined( MINIMAL_FUNCTIONALITY ) && defined( DEBUG )
#define ENABLE_HEAP_MEM_COUNTERS
#else                                       //  !defined( MINIMAL_FUNCTIONALITY ) && !defined( DEBUG )
//#define ENABLE_HEAP_MEM_COUNTERS
#endif

#ifdef MEM_CHECK

void OSMemoryIDumpAlloc( const WCHAR* szDumpFile, const BOOL memDump = fTrue );

#endif  //  MEM_CHECK

INT g_fMemCheck = fFalse;

//  Memory Counters

DWORD g_cAllocHeap;
DWORD g_cFreeHeap;
DWORD g_cbAllocHeap;
DWORD_PTR g_cbReservePage;
DWORD_PTR g_cbCommitPage;


//  System Memory Attributes

//  returns the system page reserve granularity

LOCAL DWORD g_dwPageReserveGran;

DWORD OSMemoryPageReserveGranularity()
{
    return g_dwPageReserveGran;
}

//  returns the system page commit granularity

LOCAL DWORD g_dwPageCommitGran;

DWORD OSMemoryPageCommitGranularity()
{
    return g_dwPageCommitGran;
}

//  returns the current available physical memory in the system

NTOSFuncStd( g_pfnGlobalMemoryStatusEx, g_mwszzSysInfoLibs, GlobalMemoryStatusEx, oslfExpectedOnWin5x | oslfRequired | oslfHookable );

LOCAL QWORD g_cbAvailPhysLast = 0;
LOCAL DWORD g_dwMemoryLoadLast = 0;

QWORD OSMemoryAvailable()
{
    //  return the amount of available physical memory as reported by NT

    MEMORYSTATUSEX ms = { sizeof( MEMORYSTATUSEX ) };
    g_pfnGlobalMemoryStatusEx( &ms );
    g_cbAvailPhysLast = ms.ullAvailPhys;
    g_dwMemoryLoadLast = ms.dwMemoryLoad;
    return ms.ullAvailPhys;
}

//  returns the total physical memory in the system
//  NOTE: This can return more memory than the process can actually address on an AWE system or
//        in a WOW process on a 64-bit system. Consider using OSMemoryPageReserveTotal() as well.
LOCAL QWORD g_cbMemoryTotal;

QWORD OSMemoryTotal()
{
    return g_cbMemoryTotal;
}

//  returns the current available virtual address space in the process

LOCAL QWORD g_cbAvailVirtualLast = 0;

DWORD_PTR OSMemoryPageReserveAvailable()
{
    MEMORYSTATUSEX ms = { sizeof( MEMORYSTATUSEX ) };
    g_pfnGlobalMemoryStatusEx( &ms );
    Assert( ms.ullAvailVirtual == (DWORD_PTR)ms.ullAvailVirtual );
    g_cbAvailVirtualLast = ms.ullAvailVirtual;
    return (DWORD_PTR)ms.ullAvailVirtual;
}

//  returns the total virtual address space in the process

LOCAL DWORD_PTR g_cbPageReserveTotal;
LOCAL QWORD g_cbPageFileTotal;

DWORD_PTR OSMemoryPageReserveTotal()
{
    return g_cbPageReserveTotal;
}


//  returns the peak working set size of the process

NTSTATUS WINAPI NtQueryInformationProcess(
    _In_       HANDLE ProcessHandle,
    _In_       PROCESSINFOCLASS ProcessInformationClass,
    _Out_writes_bytes_opt_(ProcessInformationLength)      PVOID ProcessInformation,
    _In_       ULONG ProcessInformationLength,
    _Out_opt_  PULONG ReturnLength
);

NTOSFuncNtStd( g_pfnNtQueryInformationProcess, g_mwszzNtdllLibs, NtQueryInformationProcess, oslfExpectedOnWin6/*?*/ | oslfRequired | oslfHookable );

DWORD_PTR OSMemoryPageWorkingSetPeak()
{
    VM_COUNTERS vmcounters;
    ULONG cbStructActual = 0;
    NTSTATUS status         = g_pfnNtQueryInformationProcess(   GetCurrentProcess(),
                                                                ProcessVmCounters,
                                                                &vmcounters,
                                                                sizeof( vmcounters ),
                                                                &cbStructActual );
    if( NT_SUCCESS( status ) && cbStructActual >= CbElementInStruct( vmcounters, PeakWorkingSetSize ) )
    {
        return vmcounters.PeakWorkingSetSize;
    }
    const QWORD cbMemTotal = OSMemoryTotal();
    const DWORD_PTR dwMemPageReserveTotal = OSMemoryPageReserveTotal();
    return (DWORD_PTR)min( cbMemTotal, dwMemPageReserveTotal );
}


//  returns the total number of physical memory pages evicted from the system

NTSTATUS WINAPI NtQuerySystemInformation(
    __in       SYSTEM_INFORMATION_CLASS SystemInformationClass,
    __inout    PVOID SystemInformation,
    __in       ULONG SystemInformationLength,
    __out_opt  PULONG ReturnLength
);

NTOSFuncNtStd( g_pfnNtQuerySystemInformation, g_mwszzNtdllLibs, NtQuerySystemInformation, oslfExpectedOnWin5x/*?*/ | oslfRequired | oslfHookable );

BOOL                            g_fEvictStatsInit;
BOOL                            g_fEvictStatsApproximated;

enum    { cRollingAvgDepth = 3 };
QWORD   g_cAvailPage[ cRollingAvgDepth ];
INT     g_icAvailPageOldest;
double  g_cAvailPageSum;
double  g_cAvailPageAvg;
LONG    g_cPageAllocLast;
DWORD   g_cPageEvict;

DWORD OSMemoryPageEvictionCount()
{
    //  try to read the page eviction rate straight from NT
    ULONG cbStructActual = 0;
    SYSTEM_FILECACHE_INFORMATION sysfcinfo  = { 0 };
    NTSTATUS status                         = g_pfnNtQuerySystemInformation(    SystemFileCacheInformation,
                                                                                &sysfcinfo,
                                                                                sizeof( sysfcinfo ),
                                                                                &cbStructActual );

    if ( NT_SUCCESS( status ) && cbStructActual >= CbElementInStruct( sysfcinfo, TransitionRePurposeCount ) )
    {
        //  we are running on a version of NT that exposes the page eviction
        //  rate directly

        //  read the page eviction rate straight from NT

        g_cPageEvict = sysfcinfo.TransitionRePurposeCount;
    }
    else
    {
        //  NT isn't courteous enough to expose its page eviction rate so we must
        //  go through a lot of machinations to estimate it.  we do this by
        //  ASSUMING that as memory gets short, all page faults indirectly cause
        //  a page eviction to satisfy them.  note that this number is less
        //  accurate when someone on the system is churning quickly through a
        //  small pool of memory but not actually applying memory pressure
        //
        //  NOTE:  this function must be called often to be accurate

        const size_t cAvailPageMin = 1280;  //  NT's page eviction threshold, adjusted for better effect

        if ( !g_fEvictStatsInit )
        {
            QWORD cAvailPageInit = OSMemoryAvailable() / OSMemoryPageCommitGranularity();
            for ( g_icAvailPageOldest = cRollingAvgDepth - 1; g_icAvailPageOldest >= 0; g_icAvailPageOldest-- )
            {
                g_cAvailPage[ g_icAvailPageOldest ] = cAvailPageInit;
            }
            g_icAvailPageOldest = 0;
            g_cAvailPageSum = cRollingAvgDepth * (double)cAvailPageInit;
            g_cAvailPageAvg = (double)cAvailPageInit;

            SYSTEM_PERFORMANCE_INFORMATION sysperfinfo;
            cbStructActual = 0;
            status         = g_pfnNtQuerySystemInformation(    SystemPerformanceInformation,
                                                               &sysperfinfo,
                                                               sizeof( sysperfinfo ),
                                                               &cbStructActual );
            if( NT_SUCCESS( status ) && cbStructActual >= CbElementInStruct( sysperfinfo, PageFaultCount ) )
            {
                g_cPageAllocLast  = sysperfinfo.PageFaultCount;
            }
            else
            {
                g_cPageAllocLast  = 0;
            }

            g_cPageEvict      = 0;

            g_fEvictStatsInit = fTrue;
            g_fEvictStatsApproximated = fTrue;
        }
        else
        {
            g_cAvailPageSum -= g_cAvailPage[ g_icAvailPageOldest ];
            g_cAvailPage[ g_icAvailPageOldest ] = OSMemoryAvailable() / OSMemoryPageCommitGranularity();
            g_cAvailPageSum += g_cAvailPage[ g_icAvailPageOldest ];
            g_icAvailPageOldest = ( g_icAvailPageOldest + 1 ) % cRollingAvgDepth;
            g_cAvailPageAvg = (double)g_cAvailPageSum / (double)cRollingAvgDepth;

            SYSTEM_PERFORMANCE_INFORMATION sysperfinfo;
            cbStructActual = 0;
            status         = g_pfnNtQuerySystemInformation(    SystemPerformanceInformation,
                                                               &sysperfinfo,
                                                               sizeof( sysperfinfo ),
                                                               &cbStructActual );
            LONG cPageAlloc;
            if( NT_SUCCESS( status ) && cbStructActual >= CbElementInStruct( sysperfinfo, PageFaultCount ) )
            {
                cPageAlloc = sysperfinfo.PageFaultCount;
            }
            else
            {
                cPageAlloc = 0;
            }

            LONG dcPageAlloc = cPageAlloc - g_cPageAllocLast;

            double  k;
            if ( g_cAvailPageAvg > 1.125 * cAvailPageMin )
            {
                k = 0;
            }
            else if ( g_cAvailPageAvg > 1.0 * cAvailPageMin )
            {
                k = 1 - ( g_cAvailPageAvg - 1.0 * cAvailPageMin ) / ( ( 1.125 - 1.0 ) * cAvailPageMin );
            }
            else
            {
                k = 1;
            }

            double  dcPageEvict;
            modf( k * dcPageAlloc + 0.5, &dcPageEvict );

            g_cPageAllocLast  = cPageAlloc;
            g_cPageEvict      = g_cPageEvict + LONG( dcPageEvict );
        }
    }

    //  return the current page eviction count

    return g_cPageEvict;
}

NTOSFuncStd( g_pfnGetProcessMemoryInfo, g_mwszzProcessMemLibs, GetProcessMemoryInfo, oslfExpectedOnWin5x | oslfRequired | oslfHookable );

void OSMemoryGetProcessMemStats( MEMSTAT * const pmemstat )
{
    PROCESS_MEMORY_COUNTERS_EX osmemstat = { 0 };
    osmemstat.cb = sizeof(osmemstat);

    memset( pmemstat, 0, sizeof(*pmemstat) );

    const BOOL f = g_pfnGetProcessMemoryInfo( GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&osmemstat, sizeof(osmemstat) );
    Assert( f );    // can this fail on our self?
    if ( f )
    {
        //  success copy everything (except the paged pool stats)
        pmemstat->cPageFaultCount = osmemstat.PageFaultCount;
        pmemstat->cbPeakWorkingSetSize = osmemstat.PeakWorkingSetSize;
        pmemstat->cbWorkingSetSize = osmemstat.WorkingSetSize;
        pmemstat->cbPagefileUsage = osmemstat.PagefileUsage;
        pmemstat->cbPeakPagefileUsage = osmemstat.PeakPagefileUsage;
        pmemstat->cbPrivateUsage = osmemstat.PrivateUsage;
    }
}


//  Fixed bitmap which is an allocationless bitmap (b/c the buffer is passed in, presumably from the stack)

CFixedBitmap::CFixedBitmap( _Out_writes_bytes_(cbBuffer) void * pbBuffer, _In_ const ULONG cbBuffer )
    :   m_cbit( cbBuffer * 8 ),
        m_rgbit( pbBuffer )
{
    memset( m_rgbit, 0, m_cbit / 8  );
}

IBitmapAPI::ERR CFixedBitmap::ErrInitBitmap( _In_ const size_t cbit )
{
    C_ASSERT( CHAR_BIT == 8 );  //  SOMEONE/SOMEONE like this constant, just check it is what they say.  I'm not sure this didn't decrease clarity.
    if ( m_rgbit == NULL ||
            cbit == 0 ||
            //  not even byte number of bits
            ( ( cbit % CHAR_BIT ) != 0 ) )
    {
        return IBitmapAPI::ERR::errInvalidParameter;
    }

    AssertSz( fFalse, "Do not call me.  Not needed." );

    return IBitmapAPI::ERR::errSuccess;
}

CFixedBitmap::~CFixedBitmap()
{
    m_cbit = 0;
    m_rgbit = NULL;
}

IBitmapAPI::ERR CFixedBitmap::ErrSet( _In_ const size_t iBit, _In_ const BOOL fValue )
{
    BYTE    bit;
    BYTE*   pbyte;
    BYTE    set;
    BYTE    mask;

    if ( m_rgbit == NULL || iBit >= m_cbit )
    {
        return IBitmapAPI::ERR::errInvalidParameter;
    }

    bit     = (BYTE)( 1 << ( iBit & 7 ) );
    pbyte   = (BYTE*)m_rgbit + iBit / 8;
    set     = (BYTE)( fValue ? bit : 0 );
    mask    = bit ^ 0xFF;

    const BYTE bOld = *pbyte;
    *pbyte = *pbyte & mask | set;

    // If you hit this assert, then it's likely that there is a race condition somewhere.
    // Consider writing CAtomicFixedBitmap instead.
    AssertSz( *pbyte == bOld || // values are the same
            ( ( ( *pbyte ^ bOld ) - 1 ) & ( *pbyte ^ bOld ) ) == 0, // values differ by a single bit
              "After setting/clearing a bit, the old and new values should differ by at most one bit." );

    return IBitmapAPI::ERR::errSuccess;
}

IBitmapAPI::ERR CFixedBitmap::ErrGet( _In_ const size_t iBit, _Out_ BOOL* const pfValue )
{
    BYTE    bit;
    BYTE*   pbyte;

    if ( m_rgbit == NULL || iBit >= m_cbit )
    {
        return IBitmapAPI::ERR::errInvalidParameter;
    }

    bit     = (BYTE)( 1 << ( iBit & 7 ) );
    pbyte   = (BYTE*)m_rgbit + iBit / 8;

    *pfValue = !!( *pbyte & bit );

    return IBitmapAPI::ERR::errSuccess;
}

//  Sparse Bitmap implementation of IBitmapAPI

CSparseBitmap::CSparseBitmap()
    :   m_cbit( 0 ),
        m_rgbit( NULL ),
        m_cbitUpdate( 0 ),
        m_cbitCommit( 0 ),
        m_rgbitCommit( NULL ),
        m_shfCommit( 0 )
{
}

IBitmapAPI::ERR CSparseBitmap::ErrInitBitmap( const size_t cbit )
{
    if ( m_cbit || !cbit )
    {
        return IBitmapAPI::ERR::errInvalidParameter;
    }

    m_cbit  = cbit;
    Assert( m_rgbit == NULL );
    m_rgbit = PvOSMemoryPageReserve( ( m_cbit + 7 ) / 8, NULL );

    if ( !m_rgbit )
    {
        m_cbit = 0;
        return IBitmapAPI::ERR::errOutOfMemory;;
    }

    m_cbitUpdate = m_cbit;

    m_cbitCommit    = ( ( m_cbit + 7 ) / 8 + OSMemoryPageCommitGranularity() - 1 ) / OSMemoryPageCommitGranularity();
    m_rgbitCommit   = new BYTE[ ( m_cbitCommit + 7 ) / 8 ];

    if ( !m_rgbitCommit )
    {
        m_cbit = 0;
        OSMemoryPageFree( m_rgbit );
        m_rgbit = NULL;
        m_cbitUpdate = 0;
        m_cbitCommit = 0;
        return IBitmapAPI::ERR::errOutOfMemory;;
    }

    memset( m_rgbitCommit, 0, ( m_cbitCommit + 7 ) / 8 );

    for ( m_shfCommit = 0; size_t( 1ull << m_shfCommit ) < 8 * OSMemoryPageCommitGranularity(); m_shfCommit++ );

    return IBitmapAPI::ERR::errSuccess;
}

IBitmapAPI::ERR CSparseBitmap::ErrDisableUpdates()
{
    if ( !m_cbit )
    {
        return IBitmapAPI::ERR::errInvalidParameter;
    }

    m_cbitUpdate = 0;

    return IBitmapAPI::ERR::errSuccess;
}

IBitmapAPI::ERR CSparseBitmap::ErrReset( const size_t cbit )
{
    if ( !m_cbit )
    {
        return IBitmapAPI::ERR::errInvalidParameter;
    }

    m_cbitUpdate = cbit;

    return IBitmapAPI::ERR::errSuccess;
}

CSparseBitmap::~CSparseBitmap()
{
    OSMemoryPageDecommit( m_rgbit, ( m_cbit + 7 ) / 8 );
    OSMemoryPageFree( m_rgbit );
    m_cbit = 0;
    m_rgbit = NULL;
    m_cbitUpdate = 0;
    m_cbitCommit = 0;
    delete [] m_rgbitCommit;
    m_rgbitCommit = NULL;
    m_shfCommit = 0;
}

IBitmapAPI::ERR CSparseBitmap::ErrSet( const size_t iBit, const BOOL fValue )
{
    size_t  iBitCommit;
    BYTE    bitCommit;
    BYTE*   pbyteCommit;
    BYTE    bit;
    BYTE*   pbyte;
    BYTE    set;
    BYTE    mask;

    if ( iBit >= m_cbitUpdate )
    {
        return IBitmapAPI::ERR::errInvalidParameter;
    }

    iBitCommit  = iBit >> m_shfCommit;
    bitCommit   = (BYTE)( 1 << ( iBitCommit & 7 ) );
    pbyteCommit = (BYTE*)m_rgbitCommit + iBitCommit / 8;

    bit     = (BYTE)( 1 << ( iBit & 7 ) );
    pbyte   = (BYTE*)m_rgbit + iBit / 8;
    set     = (BYTE)( fValue ? bit : 0 );
    mask    = bit ^ 0xFF;

    if ( !( *pbyteCommit & bitCommit ) )
    {
        Assert( (BYTE*)m_rgbit + iBitCommit * OSMemoryPageCommitGranularity() <= pbyte );
        Assert( pbyte < (BYTE*)m_rgbit + ( iBitCommit + 1 ) * OSMemoryPageCommitGranularity() );
        if ( !FOSMemoryPageCommit( (BYTE*)m_rgbit + iBitCommit * OSMemoryPageCommitGranularity(), 1 ) )
        {
            return IBitmapAPI::ERR::errOutOfMemory;
        }
        *pbyteCommit = *pbyteCommit | bitCommit;
    }

    const BYTE bOld = *pbyte;
    *pbyte = *pbyte & mask | set;

    // If you hit this assert, then it's likely that there is a race condition somewhere.
    // Consider implementing an atomic sparse bitmap instead.
    AssertSz( *pbyte == bOld || // values are the same
            ( ( ( *pbyte ^ bOld ) - 1 ) & ( *pbyte ^ bOld ) ) == 0, // values differ by a single bit
              "After setting/clearing a bit, the old and new values should differ by at most one bit." );

    return IBitmapAPI::ERR::errSuccess;
}

IBitmapAPI::ERR CSparseBitmap::ErrGet( _In_ const size_t iBit, _Out_ BOOL* const pfValue )
{
    size_t  iBitCommit;
    BYTE    bitCommit;
    BYTE*   pbyteCommit;
    BYTE    bit;
    BYTE*   pbyte;

    if ( iBit >= m_cbit )
    {
        return IBitmapAPI::ERR::errInvalidParameter;
    }

    iBitCommit  = iBit >> m_shfCommit;
    bitCommit   = (BYTE)( 1 << ( iBitCommit & 7 ) );
    pbyteCommit = (BYTE*)m_rgbitCommit + iBitCommit / 8;

    bit     = (BYTE)( 1 << ( iBit & 7 ) );
    pbyte   = (BYTE*)m_rgbit + iBit / 8;

    if ( !( *pbyteCommit & bitCommit ) )
    {
        Assert( (BYTE*)m_rgbit + iBitCommit * OSMemoryPageCommitGranularity() <= pbyte );
        Assert( pbyte < (BYTE*)m_rgbit + ( iBitCommit + 1 ) * OSMemoryPageCommitGranularity() );
        if ( !FOSMemoryPageCommit( (BYTE*)m_rgbit + iBitCommit * OSMemoryPageCommitGranularity(), 1 ) )
        {
            return IBitmapAPI::ERR::errOutOfMemory;
        }
        *pbyteCommit = *pbyteCommit | bitCommit;
    }

    *pfValue = !!( *pbyte & bit );

    return IBitmapAPI::ERR::errSuccess;
}


//  The LID: 62160, can be used to induce 100% residency map recalculating.

//  returns fTrue if the virtual memory pages backing the specified buffer are
//  either in the working set of the process or are held elsewhere in system
//  memory

NTOSFuncStd( g_pfnQueryWorkingSetEx, g_mwszzWorkingSetLibs, QueryWorkingSetEx, oslfExpectedOnWin6 );

#pragma warning(push)
#pragma warning(disable:4201)       // unnamed struct

typedef enum _MEMORY_WORKING_SET_EX_LOCATION {
    MemoryLocationInvalid,
    MemoryLocationResident,
    MemoryLocationPagefile,
    MemoryLocationReserved
} MEMORY_WORKING_SET_EX_LOCATION, *PMEMORY_WORKING_SET_EX_LOCATION;

typedef struct _MEMORY_WORKING_SET_EX_BLOCK {
    union {
        struct {
            ULONG_PTR Valid : 1;
            ULONG_PTR ShareCount : 3;
            ULONG_PTR Win32Protection : 11;
            ULONG_PTR Shared : 1;
            ULONG_PTR Node : 6;
            ULONG_PTR Locked : 1;
            ULONG_PTR LargePage : 1;
            ULONG_PTR Priority : 3;
            ULONG_PTR Reserved : 5;

#if defined(_WIN64)
            ULONG_PTR ReservedUlong : 32;
#endif
    } DUMMYSTRUCTNAME;
        struct {
            ULONG_PTR Valid : 1;            // Valid = 0 in this format.
            ULONG_PTR Reserved0 : 14;
            ULONG_PTR Shared : 1;
            ULONG_PTR Reserved1 : 6;
            ULONG_PTR Location : 2;         // MEMORY_WORKING_SET_EX_LOCATION
            ULONG_PTR Reserved2 : 8;

#if defined(_WIN64)
            ULONG_PTR ReservedUlong : 32;
#endif
    } Invalid;
} DUMMYUNIONNAME;
} MEMORY_WORKING_SET_EX_BLOCK, *PMEMORY_WORKING_SET_EX_BLOCK;

#pragma warning(pop)

typedef struct _MEMORY_WORKING_SET_EX_INFORMATION {
    PVOID VirtualAddress;
    union {
        MEMORY_WORKING_SET_EX_BLOCK VirtualAttributes;
        ULONG_PTR Long;
} u1;
} MEMORY_WORKING_SET_EX_INFORMATION, *PMEMORY_WORKING_SET_EX_INFORMATION;

//  QueryWorkingSetEx() aggressiveness control
//
//  After some testing we found that no lock convoying was possible at 512 KB, and that
//  very little lock convoying was caused at 64 MBs.  Further we found out that at 2 MBs
//  nominal perf impact from syscall overhead started to happen.
// 
//  We picked a high 8 MBs as our initial fix, but we can lower this if it becomes necessary.
//
//  Note: That just the move from QueryWorkingSet() to QueryWorkingSetEx() is a 80 - 100% 
//  regression (i.e. 2x worse) for getting the cache working set. :P
//
size_t  g_cbQWSChunkSize    = (size_t)8 * 1024 * 1024;  // how big of a chunk to request from a QwsEx()
INT     g_cmsecQWSThrottle  = -1;                       // how long to wait between QwsEx() calls. (-1 = No Wait)

//  Performance tracking data
//
double  g_dblQWSExTimeLast    = 0.0;
INT     g_cQWSExCalls         = 0;

//  Query Working Set scan context
//
typedef struct _OSMEM_RESIDENCE_MAP_CONTEXT
{
#ifdef DEBUG
    //  "Lock" defense in-depth
    DWORD           tidInUse;
#endif

    //  Scan-wide settings
    size_t          cbMax;
    DWORD           iMapReq;

    //  Inter-Scan update state variables
    DWORD           cPageOutput;
    DWORD           dwUpdateId;

    //  Allocated / cached variables
    CSparseBitmap *                     psbm;           // held between scans
    PPSAPI_WORKING_SET_EX_INFORMATION   pmwsexinfo;     // good for a scan / until stop
    size_t                              cbmwsexinfoMax;

} OSMEM_RESIDENCE_MAP_CONTEXT;

OSMEM_RESIDENCE_MAP_CONTEXT g_residencemap;

VOID OSMemoryIPageResidenceMapPreinit()
{
    Assert( g_residencemap.cbMax == 0 && g_residencemap.dwUpdateId == 0 );
    Assert( g_residencemap.psbm == NULL && g_residencemap.pmwsexinfo == NULL );     // esp. bad for allocated vars
    memset( &g_residencemap, 0, sizeof(g_residencemap) );   // just in case
    g_residencemap.cPageOutput = (DWORD)-1;
}

VOID OSMemoryIPageResidenceMapPostterm()
{
    g_residencemap.cbMax = 0;
    g_residencemap.dwUpdateId = 0;
    Assert( g_residencemap.psbm == NULL );
}

ERR ErrOSMemoryPageResidenceMapScanStart( const size_t cbMax, __out DWORD * const pdwUpdateId )
{
    ERR err = JET_errSuccess;
    DWORD cPageOutputCurrent = 0;

    Assert( ( cbMax % OSMemoryPageCommitGranularity() ) == 0 );

    g_dblQWSExTimeLast = 0.0;
    g_cQWSExCalls = 0;


    g_residencemap.cbMax = cbMax;

    const size_t cPage = g_residencemap.cbMax / OSMemoryPageCommitGranularity();
    const size_t cbit = LNextPowerOf2( cPage );
#ifdef DEBUG
    size_t cbitCheck;
    for ( cbitCheck = 1; cbitCheck < cPage; cbitCheck *= 2 );
    Assert( cbitCheck == cbit );
#endif

    //  get a new sparse bitmap object

    Assert( g_residencemap.psbm == NULL );
    Alloc( g_residencemap.psbm = new CSparseBitmap );

    Call( ErrFaultInjection( 37584 /* should fail with JET_errOutOfMemory */ ) );
    IBitmapAPI::ERR errBM = g_residencemap.psbm->ErrInitBitmap( cbit );
    if ( errBM != IBitmapAPI::ERR::errSuccess )
    {
        Assert( errBM == IBitmapAPI::ERR::errOutOfMemory );
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    Assert( g_residencemap.cbmwsexinfoMax == 0 );
    Assert( g_residencemap.pmwsexinfo == NULL );
    g_residencemap.cbmwsexinfoMax = ( cPage ) * sizeof( PSAPI_WORKING_SET_EX_INFORMATION );

    Alloc( g_residencemap.pmwsexinfo = (PPSAPI_WORKING_SET_EX_INFORMATION)PvOSMemoryPageAlloc( g_residencemap.cbmwsexinfoMax, NULL ) );

    //  we need to recompute our page residence map if the kernel has trimmed memory since the last call

    Call( g_pfnNtQuerySystemInformation.ErrIsPresent() );

    const BOOL fAlwaysRun = BOOL( UlConfigOverrideInjection( 62160, 0 )  +
                                    UlConfigOverrideInjection( 41680, 0 ) );

    SYSTEM_PERFORMANCE_INFORMATION sysperfinfo;
    ULONG cbStructActual = 0;
    NTSTATUS status         = g_pfnNtQuerySystemInformation(    SystemPerformanceInformation,
                                                            &sysperfinfo,
                                                            sizeof( sysperfinfo ),
                                                            &cbStructActual );

    if (    NT_SUCCESS( status ) &&
            cbStructActual >= CbElementInStruct( sysperfinfo, DirtyPagesWriteCount ) )
    {
        cPageOutputCurrent = sysperfinfo.DirtyPagesWriteCount;
    }

    if ( ( cPageOutputCurrent != g_residencemap.cPageOutput ) || fAlwaysRun )
    {
        g_residencemap.cPageOutput = cPageOutputCurrent;
        g_residencemap.dwUpdateId++;
    }

    g_residencemap.iMapReq = 0;

    //  this set of functions (ErrOSMemoryPageResidenceMapScanStart/ErrOSMemoryPageResidenceMapRetrieve/ErrOSMemoryPageResidenceMapScanStop)
    //  should be used from the same thread because they are not thread safe 

    Assert( g_residencemap.tidInUse == 0 );
    OnDebug( g_residencemap.tidInUse = DwUtilThreadId() );

    *pdwUpdateId = g_residencemap.dwUpdateId;

    return JET_errSuccess;

HandleError:

    Assert( err < JET_errSuccess );

    OSMemoryPageResidenceMapScanStop();

    return err;
}

VOID OSMemoryPageResidenceMapScanStop()
{

    //  this set of functions (ErrOSMemoryPageResidenceMapScanStart/ErrOSMemoryPageResidenceMapRetrieve/ErrOSMemoryPageResidenceMapScanStop)
    //  should be used from the same thread because they are not thread safe    

    Assert( g_residencemap.tidInUse == DwUtilThreadId() || g_residencemap.tidInUse == 0 );

    delete g_residencemap.psbm;
    g_residencemap.psbm = NULL;

    OSMemoryPageFree( g_residencemap.pmwsexinfo );
    g_residencemap.pmwsexinfo = NULL;
    g_residencemap.cbmwsexinfoMax = 0;

    OnDebug( g_residencemap.tidInUse = 0 );
}

ERR ErrOSMemoryPageResidenceMapRetrieve( void* const pv, const size_t cb, IBitmapAPI** const ppbmapi )
{
    ERR                                 err         = JET_errSuccess;

    Assert( ( (ULONG_PTR)pv % OSMemoryPageCommitGranularity() ) == 0 );
    Assert( ( cb % OSMemoryPageCommitGranularity() ) == 0 );

    //  this set of functions (ErrOSMemoryPageResidenceMapScanStart/ErrOSMemoryPageResidenceMapRetrieve/ErrOSMemoryPageResidenceMapScanStop)
    //  should be used from the same thread because they are not thread safe 
    
    Assert( g_residencemap.tidInUse == DwUtilThreadId() );
    Assert( g_residencemap.cbMax >= cb );

    Assert( g_residencemap.psbm );
    Assert( g_residencemap.pmwsexinfo );

    if ( g_residencemap.iMapReq != 0 && g_cmsecQWSThrottle != -1 )
    {
        // do we need any backoff?
        UtilSleep( g_cmsecQWSThrottle );
    }

    g_residencemap.iMapReq++;

    //  This paticular piece of memory is out of date with the last dirty page write
    //  count update.

    IBitmapAPI::ERR errBM = IBitmapAPI::ERR::errSuccess;

    //  check we have a bitmap object big enough to represent every
    //  possible VM page of the requested size

    const size_t cbit = LNextPowerOf2( cb / OSMemoryPageCommitGranularity() );
#ifdef DEBUG
    size_t cbitCheck;
    for ( cbitCheck = 1; cbitCheck < cb / OSMemoryPageCommitGranularity(); cbitCheck *= 2 );
    Assert( cbitCheck == cbit );
#endif

    //  reset the sparse bitmap object

    errBM = g_residencemap.psbm->ErrReset( cbit );
    if ( errBM != IBitmapAPI::ERR::errSuccess )
    {
        Assert( errBM == IBitmapAPI::ERR::errOutOfMemory );
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    const size_t cpageQWSChunkSize = g_cbQWSChunkSize / OSMemoryPageCommitGranularity();

    // we roundup() by 1 if we don't have an even number of cpageQWSChunkSizes.
    const INT cIter = roundup( cb, cpageQWSChunkSize * OSMemoryPageCommitGranularity() ) / ( cpageQWSChunkSize * OSMemoryPageCommitGranularity() );
    Assert( ( cb % ( cpageQWSChunkSize * OSMemoryPageCommitGranularity() ) % OSMemoryPageCommitGranularity() ) == 0 );

    for( INT iIter = 0; iIter < cIter; iIter++ )
    {
        NTSTATUS    status  = STATUS_INFO_LENGTH_MISMATCH;

        size_t cPage;
        if ( ( iIter == cIter - 1 ) &&
                ( ( cb % ( cpageQWSChunkSize * OSMemoryPageCommitGranularity() ) ) != 0 ) )
        {
            //  last iter, if not full sized chunk, then need to calculate special ...
            cPage = ( cb % ( cpageQWSChunkSize * OSMemoryPageCommitGranularity() ) ) / OSMemoryPageCommitGranularity();
        }
        else
        {
            cPage = cpageQWSChunkSize;
        }
        Assert( cPage );

        void * pvChunk = (BYTE*)pv + ( iIter * cpageQWSChunkSize * OSMemoryPageCommitGranularity() );

        //  fetch our working set list

        for( size_t iPage = 0; iPage < cPage; iPage++ )
        {
            g_residencemap.pmwsexinfo[iPage].VirtualAddress = (BYTE*)pvChunk + ( iPage * OSMemoryPageCommitGranularity() );
            g_residencemap.pmwsexinfo[iPage].VirtualAttributes.Flags = 0x0;
            // should we set to zero the 4 bytes of invisible pad in the 64-bit structure
        }

        C_ASSERT( sizeof( PSAPI_WORKING_SET_EX_INFORMATION ) == sizeof(g_residencemap.pmwsexinfo[0]) );
        const size_t cbmwsexinfo = cPage * sizeof(g_residencemap.pmwsexinfo[0]);
        AssertRTL( cbmwsexinfo <= g_residencemap.cbmwsexinfoMax );
        const HRT hrtBegin = HrtHRTCount();
        status = (  g_pfnQueryWorkingSetEx( GetCurrentProcess(), (void*)g_residencemap.pmwsexinfo, cbmwsexinfo ) ?
                        STATUS_SUCCESS :
                        STATUS_UNSUCCESSFUL );
        g_dblQWSExTimeLast += DblHRTElapsedTimeFromHrtStart( hrtBegin );
        g_cQWSExCalls++;

        if ( !NT_SUCCESS( status ) )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        //  fill the bitmap with our working set data

        //  set a bit in the bitmap for each VM page that is present in the
        //  working set list

        for ( size_t iWSInfo = 0; iWSInfo < cPage; iWSInfo++ )
        {
            //  explicitly not pvChunk, should be pv ...
            size_t iPage = ( (BYTE*)( g_residencemap.pmwsexinfo[ iWSInfo ].VirtualAddress ) - (BYTE*)pv ) / OSMemoryPageCommitGranularity();
            Assert( iPage == ( ( iIter * cpageQWSChunkSize ) + iWSInfo ) );

            BOOL fResident = g_residencemap.pmwsexinfo[ iWSInfo ].VirtualAttributes.Valid;

            
            //  BOOL fResident = g_residencemap.pmwsexinfo[ iWSInfo ].VirtualAttributes.Valid ||
            //                      ((MEMORY_WORKING_SET_EX_BLOCK*)&(g_residencemap.pmwsexinfo[ iWSInfo ].VirtualAttributes))->Invalid.Location == MemoryLocationResident;

            fResident = !FNegTest( fStrictIoPerfTesting ) && (BOOL)UlConfigOverrideInjection( 41680, fResident );

            errBM = g_residencemap.psbm->ErrSet( iPage, fResident );
            if ( errBM == IBitmapAPI::ERR::errOutOfMemory )
            {
                Call( ErrERRCheck( JET_errOutOfMemory ) );
            }
        }

    }   // for each QWS call

    //  disable future updates of the bitmap

    errBM = g_residencemap.psbm->ErrDisableUpdates();
    Assert( errBM == IBitmapAPI::ERR::errSuccess );

    //  return the resident memory bitmap

    *ppbmapi        = g_residencemap.psbm;

HandleError:

    if ( err < JET_errSuccess )
    {
        *ppbmapi        = NULL;
    }

    return err;
}

BOOL FOSMemoryPageResident( void* const pv, const size_t cb )
{
    BOOL                                    fResident   = fFalse;
    MEMORY_WORKING_SET_EX_INFORMATION*      pmwsexinfo  = NULL;
    void*                                   pvVMStart   = NULL;
    void*                                   pvVMEnd     = NULL;
    size_t                                  cbVM        = 0;
    size_t                                  cPageVM     = 0;
    size_t                                  iPageVM     = 0;

    //  compute the VM boundary of the buffer

    pvVMStart   = (BYTE*)pv - DWORD_PTR( pv ) % OSMemoryPageCommitGranularity();
    pvVMEnd     = (BYTE*)pv + cb - 1;
    pvVMEnd     = (BYTE*)pvVMEnd + ( OSMemoryPageCommitGranularity() - 1 - DWORD_PTR( pvVMEnd ) % OSMemoryPageCommitGranularity() );
    pvVMEnd     = (BYTE*)pvVMEnd + 1;
    cbVM        = (BYTE*)pvVMEnd - (BYTE*)pvVMStart;
    cPageVM     = cbVM / OSMemoryPageCommitGranularity();

    //  Protect from more than 512 bytes of stack usage
    //  cb is supposed to be one ESE page size, 128kb is the reasonable max size limit for now,
    //  then cPageVM limit would be around 32

    if ( cPageVM > 32 )
    {
        AssertSz( fFalse, "Passed in a too big (%d) buffer.", cb );
        goto HandleError;
    }

    //  query the status of these VM pages

    const size_t cbmwsexinfo = sizeof( MEMORY_WORKING_SET_EX_INFORMATION ) * cPageVM;
    pmwsexinfo = (MEMORY_WORKING_SET_EX_INFORMATION*)_alloca( cbmwsexinfo );

    for ( iPageVM = 0; iPageVM < cPageVM; iPageVM++ )
    {
        pmwsexinfo[ iPageVM ].VirtualAddress = (BYTE*)pvVMStart + iPageVM * OSMemoryPageCommitGranularity();
        memset( &pmwsexinfo[ iPageVM ].u1.VirtualAttributes, 0, sizeof( pmwsexinfo[ iPageVM ].u1.VirtualAttributes ) );
    }
    if ( g_pfnQueryWorkingSetEx.ErrIsPresent() < JET_errSuccess ||
         !g_pfnQueryWorkingSetEx( GetCurrentProcess(), (void*)pmwsexinfo, cbmwsexinfo ) )
    {
        goto HandleError;
    }

    //  this buffer is resident only if every VM page backing it is resident

    fResident = fTrue;
    for ( iPageVM = 0; fResident && iPageVM < cPageVM; iPageVM++ )
    {
        fResident =     pmwsexinfo[ iPageVM ].u1.VirtualAttributes.Valid ||
                        (   !pmwsexinfo[ iPageVM ].u1.VirtualAttributes.Valid &&
                            pmwsexinfo[ iPageVM ].u1.VirtualAttributes.Invalid.Location == MemoryLocationResident );
    }

HandleError:
    return fResident;
}


#if( defined(DEBUG) || !defined(OS_LAYER_VIOLATIONS) )
//  I put this in debug only because according to Pavel (OS MM person), we COULD but 
//  ARE VERY UNLIKELY to get a few lies ... so in using it for Assert()s it is valuable
//  until we start getting lies.  The lies are for things like if you dup a process, then
//  memory can show up as mapped-shared, when it wasn't really shared by you, and also
//  I think other other conditions.  So leave this debug only.  AND I put it in oslite.lib
//  so we can make liberal usage of it in unit testing as well.

//  Relevant types of memory state ESE is interested in.

enum OSMemoryState  //  ms
{
    msUnknown           = 0x0,

    //  Basic Types
    msPageAlloc         = 0x1,
    msMapping           = 0x2,

    //  Modifiers ...
    msCommitted         = 0x40,     //  only applies to msPageAlloc
    msReserved          = 0x80,     //  only applies to msPageAlloc

    msCOWed             = 0x100,    //  only applies to msMapping
};

/*
Just in case we need to indirect VirtualQueryEx() later, this would be the defn for this API.  It appears to be in XP and later though.
#define wszCoreMemory           L"api-ms-win-core-memory-l1-1-1.dll"            // win7
const wchar_t * const g_mwszzMemLibs            = wszCoreMemory L"\0" /* downlevel * / wszKernel32 L"\0";
NTOSFuncCount( g_pfnVirtualQueryEx, g_mwszzWorkingSetLibs, VirtualQueryEx, oslfExpectedOnWin5x|oslfRequired );
*/

ERR ErrOSMemoryIGetState( _In_ const void * const pvClient, _In_ const size_t cbClient, _Out_ OSMemoryState * const pms )
{
    Assert( cbClient != 0 );
    LONG_PTR cbCommitPage = OSMemoryPageCommitGranularity();
    LONG_PTR pv = (LONG_PTR)pvClient;
    LONG_PTR cb = cbClient;
    if ( ( pv % cbCommitPage ) != 0 )
    {
        //  Client gave us a mis-aligned pv ... let's square it up, by rounding down to the nearest
        //  OS memory granularity
        pv = rounddn( pv, cbCommitPage );
        Assert( ( (LONG_PTR)pvClient - pv ) > 0 );
        cb += ( (LONG_PTR)pvClient - pv );
        Assert( cb > 0 );   //   no overflow.
    }
    if ( ( cb % cbCommitPage ) != 0 )
    {
        //  Client gave us a mis-sized buffer (or our resizing adjusting the pv has mis-sized it)
        cb = roundup( cb, cbCommitPage );
        Assert( cb > 0 );   //  no overflow
    }
    Assert( ( cb % OSMemoryPageCommitGranularity() ) == 0 );
    Assert( ( ((ULONG_PTR)pv) % OSMemoryPageCommitGranularity() ) == 0 );

    OSMemoryState msCurrent = msUnknown;

    __int64 cbLeft = cb;
    while( cbLeft > 0 )
    {
        const void * const pvChunk = (BYTE*)pv + ( cb - cbLeft );
        MEMORY_BASIC_INFORMATION membasic = { 0 };
        //  Stunningly (and it makes me think this can't be efficient) this API does not require a cb, it just gives
        //  you everything in the next contiguously matching section.
        const size_t cbResult = VirtualQueryEx( GetCurrentProcess(), pvChunk, &membasic, sizeof(membasic) );
        if ( cbResult < sizeof(membasic) )
        {
            AssertSz( fFalse, "We failed to virtual query something.  Why?  cb=%d, GLE=%d", cbResult, GetLastError() );
            return ErrERRCheck( JET_errInternalError );
        }

        if ( msCurrent == msUnknown )
        {
            //  Seed the state
            Assert( membasic.Type == MEM_PRIVATE || membasic.Type == MEM_MAPPED || FNegTest( fInvalidUsage ) );
            msCurrent = ( membasic.Type == MEM_PRIVATE ) ? msPageAlloc : msMapping;
        }

        Assert( ( msCurrent & ( msPageAlloc | msMapping ) ) );  //  should be one or the other at this point

        if ( msCurrent & msPageAlloc )
        {
            if ( membasic.Type != MEM_PRIVATE )
            {
                AssertSz( FNegTest( fInvalidUsage ), "The memory type has flopped off MEM_PRIVATE on us, should be passsing in only one type of memory." );
                return ErrERRCheck( JET_errInvalidBufferSize );
            }

            //  Upgrade if the memory is reserved or committed ...
            if ( membasic.State == MEM_RESERVE )
            {
                msCurrent = OSMemoryState( msCurrent | msReserved );
            }
            if ( membasic.State == MEM_COMMIT )
            {
                msCurrent = OSMemoryState( msCurrent | msCommitted );
            }
        }
        else if ( msCurrent & msMapping )
        {
            if ( membasic.Type != MEM_MAPPED )
            {
                AssertSz( FNegTest( fInvalidUsage ), "The memory type has flopped off MEM_MAPPED on us, should be passsing in only one type of memory." );
                return ErrERRCheck( JET_errInvalidBufferSize );
            }

            //  Upgrade if someone has updated this memory ...
            //      .Protect typically can = PAGE_NOACCESS or PAGE_READONLY or PAGE_WRITECOPY or PAGE_READWRITE
            if ( membasic.Protect == PAGE_READWRITE )
            {
                Assert( membasic.Protect == PAGE_READWRITE );
                msCurrent = OSMemoryState( msCurrent | msCOWed );
            }
        }
        else
        {
            AssertSz( fFalse, "Unknown memory type!" );
        }

        //  This can underflow negative (but it is an integer so we're prepared for it) ...
        cbLeft -= membasic.RegionSize;
    }

    *pms = msCurrent;

    return JET_errSuccess;
}

BOOL FOSMemoryPageAllocated( const void * const pv, const size_t cb )
{
    OSMemoryState ms = msUnknown;
    return ( ErrOSMemoryIGetState( pv, cb, &ms ) >= JET_errSuccess ) &&
            ( ms & msPageAlloc ) &&
            ( ms & msCommitted ) &&
            ( !( ms & msReserved ) );
}

BOOL FOSMemoryFileMapped( const void * const pv, const size_t cb )
{
    OSMemoryState ms = msUnknown;
    return ( ErrOSMemoryIGetState( pv, cb, &ms ) >= JET_errSuccess ) &&
            ( ms & msMapping );
}

BOOL FOSMemoryFileMappedCowed( const void * const pv, const size_t cb )
{
    OSMemoryState ms = msUnknown;
    return ( ErrOSMemoryIGetState( pv, cb, &ms ) >= JET_errSuccess ) &&
            ( ms & msMapping ) &&
            ( ms & msCOWed );
}

#endif  //  DEBUG || OS_LAYER_VIOLATIONS

//  returns the total physical memory in the system taking any existing process
//  quotas into account

LOCAL QWORD g_cbQuotaTotalLast = 0;

DWORD_PTR OSMemoryQuotaTotal()
{
    const QWORD cbMemTotal = OSMemoryTotal();

    QUOTA_LIMITS_EX quota;
    ULONG cbStructActual = 0;
    NTSTATUS status = g_pfnNtQueryInformationProcess(   GetCurrentProcess(),
                                                        ProcessQuotaLimits,
                                                        &quota,
                                                        sizeof( quota ),
                                                        &cbStructActual );
    if (    NT_SUCCESS( status ) &&
            cbStructActual >= CbElementInStruct( quota, Flags ) &&
            ( quota.Flags & QUOTA_LIMITS_HARDWS_MAX_ENABLE ) )
    {
        g_cbQuotaTotalLast = quota.MaximumWorkingSetSize;
        return (DWORD_PTR)min( cbMemTotal, quota.MaximumWorkingSetSize );
    }

    const DWORD_PTR dwMemPageReserveTotal = OSMemoryPageReserveTotal();
    return (DWORD_PTR)min( cbMemTotal, dwMemPageReserveTotal );
}


#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY

//  Memory statistics

LONG LOSHeapAllocPerSecCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_cAllocHeap;

    return 0;
}

LONG LOSHeapFreePerSecCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_cFreeHeap;

    return 0;
}

LONG LOSHeapAllocCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_cAllocHeap - g_cFreeHeap;

    return 0;
}

LONG LOSHeapBytesAllocCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = g_cbAllocHeap;

    return 0;
}

LONG LOSPageBytesReservedCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = (ULONG)g_cbReservePage;

    return 0;
}

LONG LOSPageBytesCommittedCEFLPv( LONG iInstance, void* pvBuf )
{
    if ( pvBuf )
        *( (ULONG*) pvBuf ) = (ULONG)g_cbCommitPage;

    return 0;
}

#endif  //  MINIMAL_FUNCTIONALITY


//  Memory Subsystem Init / Term

//  our private heap

HANDLE  g_hHeap;

#ifdef MEM_CHECK

//  MAI hash table

DWORD_PTR g_cmai;
struct MAI;
MAI** g_rgpmai;

DWORD g_ccsmai;
BOOL* g_rgfcsmaiInit;
CRITICAL_SECTION* g_rgcsmai;

COSMemoryMap        *g_posmm;       //  list of COSMemoryMap objects
CRITICAL_SECTION    g_csosmm;       //  protection for COSMemoryMap list
BOOL                g_fcsosmmInit;  //  is the above critical section init

#endif  //  MEM_CHECK

//  post-terminates the memory subsystem

void OSMemoryPostterm()
{
    //  free page residence bitmap

    OSMemoryIPageResidenceMapPostterm();

    //  verify that there are no memory leaks

    const BOOL fReportMemLeak = !FRFSKnownResourceLeak() && !FUtilProcessAbort() && !FNegTest( fLeakStuff );

#ifdef MEM_CHECK
    if ( fReportMemLeak && g_posmm )
    {
        if ( g_fMemCheck )
        {
            COSMemoryMap::OSMMDumpAlloc( L"Assert.TXT" );
        }

        AssertSzRTL( fFalse, "Memory-Map Leak Detected" );
    }
#endif  //  MEM_CHECK

#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY
    if ( fReportMemLeak && ( g_cbAllocHeap != 0 || g_cbReservePage != 0 || g_cbCommitPage != 0 ) )
    {
#ifdef MEM_CHECK
        if ( g_fMemCheck )
        {
            OSMemoryIDumpAlloc( L"Assert.TXT" );
        }

        if ( g_cbAllocHeap != 0 )
        {
            printf( "Remaining memory allocations\n" );
            printf( "  g_rgpmai=0x%p\n", g_rgpmai );
            for ( DWORD_PTR i=0; i<g_cmai; i++ )
            {
                if ( g_rgpmai[i] )
                {
                    printf( "    +%I64d - 0x%p\n", (QWORD)i, g_rgpmai[i] );
                }
            }
        }
#endif  //  MEM_CHECK

        AssertSzRTL( fFalse, "Memory Leak Detected" );
    }
#endif  //  MINIMAL_FUNCTIONALITY

#ifdef MEM_CHECK

    //  free COSMemoryMap critical section

    if( g_fcsosmmInit )
    {
        DeleteCriticalSection( &g_csosmm );
        g_fcsosmmInit = fFalse;
    }

    //  free MAI hash table critical section pool

    if ( g_rgcsmai )
    {
        for ( size_t icsmai = 0; icsmai < g_ccsmai; icsmai++ )
        {
            if ( g_rgfcsmaiInit[ icsmai ] )
            {
                DeleteCriticalSection( g_rgcsmai + icsmai );
            }
        }
        BOOL fFreedCSPool = !LocalFree( g_rgcsmai );
        Assert( fFreedCSPool );
        g_rgcsmai = NULL;
    }
    if ( g_rgfcsmaiInit )
    {
        BOOL fFreedCSPool = !LocalFree( g_rgfcsmaiInit );
        Assert( fFreedCSPool );
        g_rgfcsmaiInit = NULL;
    }

    //  free MAI hash table

    if ( g_rgpmai )
    {
        BOOL fMAIHashFreed = VirtualFree( g_rgpmai, 0, MEM_RELEASE );
        Assert( fMAIHashFreed );
        g_rgpmai = NULL;
    }

#endif  //  MEM_CHECK

    //  terminate our private heap

    if ( g_hHeap )
    {
        if ( g_hHeap != GetProcessHeap() )
        {
            BOOL fHeapDestroyed = HeapDestroy( g_hHeap );
            Assert( fHeapDestroyed );
        }
        g_hHeap = NULL;
    }

    g_fMemCheck = fFalse;
}


typedef
WINBASEAPI
BOOL
WINAPI
PFNHeapSetInformation (
    IN HANDLE HeapHandle,
    IN HEAP_INFORMATION_CLASS HeapInformationClass,
    IN OPTIONAL PVOID HeapInformation,
    IN OPTIONAL SIZE_T HeapInformationLength
    );

//  pre-initialize the memory subsystem

BOOL FOSMemoryPreinit()
{
    const ULONG             InfoValue               = 2;

    //  initialize all pointers

    g_hHeap = NULL;

#ifdef MEM_CHECK

    g_cmai            = 0;
    g_rgpmai          = NULL;
    g_ccsmai          = 0;
    g_rgcsmai         = NULL;

    g_posmm = NULL;
    if ( !InitializeCriticalSectionAndSpinCount( &g_csosmm, 0 ) )
    {
        goto HandleError;
    }
    g_fcsosmmInit = fTrue;

#endif  //  MEM_CHECK

    OSMemoryIPageResidenceMapPreinit();

    //  initialize all counters

    g_fEvictStatsInit = fFalse;
    g_cAllocHeap      = 0;
    g_cFreeHeap       = 0;
    g_cbAllocHeap     = 0;
    g_cbReservePage   = 0;
    g_cbCommitPage    = 0;

    //  initialize our private heap

#ifdef MINIMAL_FUNCTIONALITY
    g_hHeap = GetProcessHeap();
#else  //  !MINIMAL_FUNCTIONALITY
#ifdef ESENT
    //  always use the global heap for ESENT to save working set
    g_hHeap = GetProcessHeap();
#else  //  !ESENT
    //  always use a private heap for ESE to save us from heap corruption
    //  by other stuff in the Store.EXE process
    g_hHeap = HeapCreate( 0, 0, 0 );
#endif  //  ESENT
#endif  //  MINIMAL_FUNCTIONALITY
    if ( !g_hHeap )
    {
        goto HandleError;
    }

    //  activate the Low Fragmentation Heap for our global heap because it
    //  has waaay better performance characteristics than the default heap
    //
    //  NOTE:  do not do this if we are using the process global heap
    //  NOTE:  ignore any errors because LFH may not be supported

    if ( g_hHeap != GetProcessHeap() )
    {
        (void)HeapSetInformation(   g_hHeap,
                                    HeapCompatibilityInformation,
                                    (void*)&InfoValue,
                                    sizeof( InfoValue ) );
    }

#ifdef MEM_CHECK

    WCHAR wsz[10];

    OnDebug( g_fMemCheck = fTrue );

    if ( FOSConfigGet( L"DEBUG", L"Mem Check", wsz, sizeof( wsz ) ) )
    {
        g_fMemCheck = !!_wtoi( wsz );
    }

    //  allocate MAI hash table

    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof( MEMORYSTATUSEX );
    GlobalMemoryStatusEx( &ms );
    for ( g_cmai = 0; ms.ullTotalPhys; ms.ullTotalPhys /= 2, g_cmai++ );
    g_cmai = ( 1 << max( 10, g_cmai - 12 ) ) - 1;

    g_rgpmai = (MAI**) VirtualAlloc( NULL, g_cmai * sizeof( MAI* ), MEM_COMMIT, PAGE_READWRITE );
    if ( !g_rgpmai )
    {
        goto HandleError;
    }

    //  initialize MAI hash table critical section pool

    g_ccsmai = 8 * CUtilProcessProcessor();
    g_rgfcsmaiInit = (BOOL*)LocalAlloc( LMEM_ZEROINIT, g_ccsmai * sizeof( BOOL ) );
    if ( !g_rgfcsmaiInit )
    {
        goto HandleError;
    }
    g_rgcsmai = (CRITICAL_SECTION*) LocalAlloc( 0, g_ccsmai * sizeof( CRITICAL_SECTION ) );
    if ( !g_rgcsmai )
    {
        goto HandleError;
    }
    for ( size_t icsmai = 0; icsmai < g_ccsmai; icsmai++ )
    {
        if ( !InitializeCriticalSectionAndSpinCount( g_rgcsmai + icsmai, 0 ) )
        {
            goto HandleError;
        }
        g_rgfcsmaiInit[ icsmai ] = fTrue;
    }

#endif  //  MEM_CHECK

    //  get page commit and reserve granularity

    SYSTEM_INFO sinf;
    SetLastError( ERROR_SUCCESS );
    GetNativeSystemInfo( &sinf );
    if ( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED )
    {
        GetSystemInfo( &sinf );
    }

    g_dwPageReserveGran = sinf.dwAllocationGranularity;
    g_dwPageCommitGran = sinf.dwPageSize;
    Expected( g_dwPageReserveGran == 64 * 1024 );
    Expected( g_dwPageCommitGran == 4 * 1024 /* x86, amd64, arm? */ || g_dwPageCommitGran == 16 * 1024 /* ia64? */ );

    //  initialize stats

    //  get total physical memory

    MEMORYSTATUSEX ms2;
    ms2.dwLength = sizeof( MEMORYSTATUSEX );
    GlobalMemoryStatusEx( &ms2 );
    g_cbMemoryTotal         = ms2.ullTotalPhys;
    g_cbPageReserveTotal    = (DWORD_PTR)ms2.ullTotalVirtual;
    g_cbPageFileTotal       = ms2.ullTotalPageFile;
    Assert( g_cbPageReserveTotal == ms2.ullTotalVirtual );  // 32-bit arch dataloss

    //  get last stats

    g_cbAvailPhysLast       = ms2.ullAvailPhys;
    g_dwMemoryLoadLast      = ms2.dwMemoryLoad;
    g_cbAvailVirtualLast    = ms2.ullAvailVirtual;

    return fTrue;

HandleError:
    OSMemoryPostterm();
    return fFalse;
}


//  terminate memory subsystem

void OSMemoryTerm()
{
    //  nop
}


//  init memory subsystem

ERR ErrOSMemoryInit()
{
    ERR err = JET_errSuccess;
    CallR( g_pfnGlobalMemoryStatusEx.ErrIsPresent() );
    CallR( g_pfnNtQuerySystemInformation.ErrIsPresent() );
    CallR( g_pfnNtQueryInformationProcess.ErrIsPresent() );
    return JET_errSuccess;
}


#ifdef MEM_CHECK

INLINE const CHAR * const SzNewFile()
{
    return Postls()->szNewFile;
}
INLINE ULONG UlNewLine()
{
    return Postls()->ulNewLine;
}

BOOL FOSMemoryNewMemCheck_( __in_z const CHAR* const szFileName, const ULONG ulLine )
{
    OSTLS *pTLS = Postls();
    if ( !pTLS )
    {
        return fFalse;
    }
    pTLS->szNewFile = szFileName;
    pTLS->ulNewLine = ulLine;
    return fTrue;
}

struct MAI
{
    MAI*            pmaiNext;

    void*           pv;
    size_t          cb;
    DWORD           lLine;
    const CHAR* szFile;
};

INLINE INT ImaiHashPv( void* const pv )
{
    return (INT)(( DWORD_PTR( pv ) / sizeof( INT ) ) % g_cmai);
}

void OSMemoryIInsertHeapAlloc( void* const pv, const size_t cb, const CHAR* szFile, LONG lLine )
{
    //  we should not see any failed allocations

    Assert( pv );

    //  use the first chunk of the memory for the MAI

    MAI* pmai = (MAI*)pv;

    //  initialize the MAI

    pmai->pv = pv;
    pmai->cb = cb;
    pmai->szFile = szFile;
    pmai->lLine = lLine;

    //  insert the MAI into the hash table

    const INT imai = ImaiHashPv( pv );
    CRITICAL_SECTION* pcsmai = g_rgcsmai + imai % g_ccsmai;

    EnterCriticalSection( pcsmai );

    pmai->pmaiNext = g_rgpmai[imai];
    g_rgpmai[imai] = pmai;

    LeaveCriticalSection( pcsmai );
}

void OSMemoryIDeleteHeapAlloc( void* const pv, size_t cb )
{
    //  we should not see any failed allocations

    Assert( pv );

    //  find the specified allocation in the hash table

    const INT imai = ImaiHashPv( pv );
    CRITICAL_SECTION* pcsmai = g_rgcsmai + imai % g_ccsmai;

    EnterCriticalSection( pcsmai );

    MAI** ppmai = &g_rgpmai[imai];
    while ( *ppmai && (*ppmai)->pv != pv )
    {
        ppmai = &(*ppmai)->pmaiNext;
    }

    EnforceSz( *ppmai, "MemCheckFreeingUnallocatedMemHeap" );

    //  remove the MAI from the hash table

    MAI* pmai = *ppmai;
    *ppmai = pmai->pmaiNext;

    //  Allocated and current size of heap chunk must match.
    AssertSz( pmai->cb == cb + sizeof( MAI ), "MemCheckCorruptedHeapBlock" );

    LeaveCriticalSection( pcsmai );
}

void OSMemoryIInsertPageAlloc( void* const pv, const size_t cb, const CHAR* szFile, LONG lLine )
{
    //  we should not see any failed allocations

    Assert( pv );

    //  allocate an MAI for this new allocation

    MAI* pmai = (MAI*)HeapAlloc( g_hHeap, 0, sizeof( MAI ) );
    EnforceSz( pmai, "MemCheckHeapBlockAllocFailure" );

    //  initialize the MAI

    pmai->pv = pv;
    pmai->cb = cb;
    pmai->szFile = szFile;
    pmai->lLine = lLine;

    //  insert the MAI into the hash table

    const INT imai = ImaiHashPv( pv );
    CRITICAL_SECTION* pcsmai = g_rgcsmai + imai % g_ccsmai;

    EnterCriticalSection( pcsmai );

    pmai->pmaiNext = g_rgpmai[imai];
    g_rgpmai[imai] = pmai;

    LeaveCriticalSection( pcsmai );
}

void OSMemoryIDeletePageAlloc( void* pv, const size_t cb )
{
    //  we should not see any failed allocations

    Assert( pv );

    //  find the specified allocation in the hash table

    const INT imai = ImaiHashPv( pv );
    CRITICAL_SECTION* pcsmai = g_rgcsmai + imai % g_ccsmai;

    EnterCriticalSection( pcsmai );

    MAI** ppmai = &g_rgpmai[imai];
    while ( *ppmai && (*ppmai)->pv != pv )
    {
        ppmai = &(*ppmai)->pmaiNext;
    }

    EnforceSz( *ppmai, "MemCheckFreeingUnallocatedMemPage" );

    //  remove the MAI from the hash table

    MAI* pmai = *ppmai;
    *ppmai = pmai->pmaiNext;

    AssertSz( pmai->cb == cb, "Difference between allocated 0x%016X and current size (pointer: 0x%p ) of page chunk" );

    LeaveCriticalSection( pcsmai );

    //  free allocation record

    HeapFree( g_hHeap, 0, pmai );
}

//  ================================================================
VOID SprintHex(
    __out_bcount_z(cbDest) PSTR const       szDest,
    __in const INT          cbDest,
    __in_bcount(cbSrc) const BYTE * const   rgbSrc,
    __in const INT          cbSrc,
    __in const INT          cbWidth     = 16,
    __in const INT          cbChunk     = 1,
    __in const INT          cbAddress   = 2 * sizeof( void* ),
    __in const INT          cbStart     = 0 )
//  ================================================================
{
    static const CHAR rgchConvert[] =   { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };

    const BYTE * const pbMax = rgbSrc + cbSrc;
    const INT cchHexWidth = ( cbWidth * 2 ) + (  cbWidth / cbChunk );

    const BYTE * pb = rgbSrc;
    CHAR * sz = szDest;
    while( pbMax != pb )
    {
        if ( 0 != cbAddress )
        {
            StringCbPrintfA( sz, cbDest-(sz-szDest), "%*.*lx    ", cbAddress, cbAddress, (INT)(pb - rgbSrc) + cbStart );
            sz += strlen( sz );
        }
        CHAR * szHex    = sz;
        CHAR * szText   = sz + cchHexWidth;
        do
        {
            for( INT cb = 0; cbChunk > cb && pbMax != pb; ++cb, ++pb )
            {
                AssertPREFIX( szHex < szDest + cbDest - 2 );
                AssertPREFIX( szText < szDest + cbDest );
                *szHex++    = rgchConvert[ *pb >> 4 ];
                *szHex++    = rgchConvert[ *pb & 0x0F ];
                *szText++   = isprint( *pb ) ? *pb : '.';
            }
            *szHex++ = ' ';
        } while( ( ( pb - rgbSrc ) % cbWidth ) && pbMax > pb );
        while( szHex != sz + cchHexWidth )
        {
            AssertPREFIX( szHex < szDest + cbDest );
            *szHex++ = ' ';
        }
        AssertPREFIX( szText < szDest + cbDest - 3 );
        *szText++ = '\r';
        *szText++ = '\n';
        *szText = '\0';
        sz = szText;
    }
}

void OSMemoryIDumpAlloc( const WCHAR* szDumpFile, const BOOL fMemDump )
{
    HANDLE hFile = CreateFileW(
        szDumpFile,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if ( INVALID_HANDLE_VALUE != hFile )
    {
        CHAR    szMessage[4096];
        DWORD cchActual;

        const LARGE_INTEGER ibOffset = { 0, 0 };
        (void)SetFilePointerEx( hFile, ibOffset, NULL, FILE_END );

        OSStrCbFormatA( szMessage, sizeof(szMessage), "\r\nMemory Leak Statistics\r\n\r\n" );
        Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
        (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );

        OSStrCbFormatA( szMessage, sizeof(szMessage), "g_cAllocHeap - g_cFreeHeap = 0x%016I64x\r\n", (QWORD)g_cAllocHeap - g_cFreeHeap );
        Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
        (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );

        OSStrCbFormatA( szMessage, sizeof(szMessage), "g_cbAllocHeap            = 0x%016I64x bytes\r\n", (QWORD)g_cbAllocHeap );
        Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
        (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );

        OSStrCbFormatA( szMessage, sizeof(szMessage), "g_cbReservePage          = 0x%016I64x pages (0x%016I64x bytes)\r\n", (QWORD)g_cbReservePage / OSMemoryPageCommitGranularity(), (QWORD)g_cbReservePage );
        Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
        (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );

        OSStrCbFormatA( szMessage, sizeof(szMessage), "g_cbCommitPage           = 0x%016I64x pages (0x%016I64x bytes)\r\n\r\n", (QWORD)g_cbCommitPage / OSMemoryPageCommitGranularity(), (QWORD)g_cbCommitPage );
        Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
        (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );

        OSStrCbFormatA( szMessage, sizeof(szMessage), "Address             Size                Type  File(Line)\r\n" );
        Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
        (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );

        OSStrCbFormatA( szMessage, sizeof(szMessage), "==================  ==================  ====  ==========================================\r\n" );
        Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
        (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );

        DWORD imai;
        SIZE_T cbAlloc;
        DWORD_PTR cbReserve;
        DWORD cAlloc, cPages;
        cbAlloc = 0;
        cbReserve = 0;
        cAlloc = 0;
        cPages = 0;
        for ( imai = 0; imai < g_cmai; imai++ )
        {
            MAI* pmai = g_rgpmai[imai];
            while ( pmai )
            {
                size_t cb;
                BYTE *pb;

                cb = pmai->cb;
                pb = (BYTE*)pmai->pv;
                if ( pmai->pv == (void*)pmai )
                {
                    pb += sizeof( MAI );    //  skip the MAI for heap allocations
                    cb = ( cb > sizeof( MAI ) ? cb - sizeof( MAI ) : 0 );   //  should never be 0
                }

                    OSStrCbFormatA( szMessage, sizeof(szMessage),
                            "0x%016I64X  0x%016I64X  %-4s  %s(%d)\r\n",
                            QWORD( pb ),
                            QWORD( cb ),
                            pmai->pv == (void*)pmai ? "Heap" : "Page",
                            pmai->szFile,
                            pmai->lLine );
                Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
                (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );
                if ( pmai->pv == pmai )
                {
                    cAlloc++;
                    cbAlloc += cb;
                }
                else
                {
                    cPages++;
                    cbReserve += cb;
                }

                if ( 512 < cb )
                {
                    cb = 512;
                }
                if ( fMemDump )
                {
                    //  dump up to 512 bytes

                    if ( pmai->pv != (void*)pmai )
                    {
                        BOOL                        fReadable   = fTrue;
                        MEMORY_BASIC_INFORMATION    mbi;
                        const size_t                iVQ         = VirtualQuery( pb, &mbi, sizeof( mbi ) );

                        if ( iVQ >= sizeof( mbi ) )
                        {
                            if ( mbi.RegionSize < cb )
                            {
                                fReadable = fFalse; //  should never happen -- we're too lazy to query again
                            }
                            if ( mbi.State != MEM_COMMIT )
                            {
                                fReadable = fFalse;
                            }
                            if ( !( mbi.AllocationProtect &
                                    ( PAGE_READONLY |
                                      PAGE_READWRITE |
                                      PAGE_EXECUTE_READ |
                                      PAGE_EXECUTE_READWRITE ) ) )
                            {
                                fReadable = fFalse;
                            }
                        }
                        else
                        {
                            fReadable = fFalse;
                        }
                        if ( !fReadable )
                        {
                            OSStrCbFormatA( szMessage, sizeof(szMessage), "\t<< block is not dumpable (not committed) >>\r\n" );
                            Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
                            (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );
                            goto NextMAI;
                        }
                    }

                    SprintHex( szMessage, sizeof(szMessage), pb, cb );
                    (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );
                }

NextMAI:
                pmai = pmai->pmaiNext;
            }
        }
        OSStrCbFormatA( szMessage, sizeof(szMessage), "Calculated mem stats\r\n====================\r\n" );
        Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
        (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );
        OSStrCbFormatA( szMessage, sizeof(szMessage), "g_cAllocHeap - g_cFreeHeap = 0x%016I64x\r\n", (QWORD)cAlloc );
        Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
        (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );
        OSStrCbFormatA( szMessage, sizeof(szMessage), "g_cbAllocHeap            = 0x%016I64x bytes\r\n", (QWORD)cbAlloc );
        Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
        (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );
        OSStrCbFormatA( szMessage, sizeof(szMessage), "g_cbReservePage          = 0x%016I64x pages (0x%016I64x bytes)\r\n", (QWORD)cbReserve / OSMemoryPageCommitGranularity(), (QWORD)cbReserve );
        Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
        (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );

        (void)CloseHandle( hFile );
    }
}

#endif  //  MEM_CHECK


//  Heap Memory Allocation

//  calculate the address of the aligned block and store its offset (for free)

INLINE void* PvOSMemoryHeapIAlign( void* const pv, const size_t cbAlign )
{

    //  round up to the nearest cache line
    //  NOTE: this formula always forces an offset of atleast 1 byte

    const ULONG_PTR ulp         = ULONG_PTR( pv );
    const ULONG_PTR ulpAligned  = ( ( ulp + cbAlign ) / cbAlign ) * cbAlign;
    const ULONG_PTR ulpOffset   = ulpAligned - ulp;

    Assert( ulpOffset > 0 );
    Assert( ulpOffset <= cbAlign );
    Assert( ulpOffset == BYTE( ulpOffset ) );   //  must fit into a single BYTE

    //  store the offset

    BYTE *const pbAligned   = (BYTE*)ulpAligned;
    pbAligned[ -1 ]         = BYTE( ulpOffset );

    //  return the aligned block

    return (void*)pbAligned;
}


//  retrieve the offset of the real block being freed

INLINE void* PvOSMemoryHeapIUnalign( void* const pv )
{

    //  read the offset of the real block

    BYTE *const pbAligned   = (BYTE*)pv;
    const BYTE bOffset      = pbAligned[ -1 ];

    Assert( bOffset > 0 );

    //  return the real unaligned block

    return (void*)( pbAligned - bOffset );
}


//  allocate a chunk of memory from the process heap of the specifed size,
//  returning NULL if there is insufficient heap memory available to satisfy
//  the request.  "aligned" memory will always start at the beginning of a
//  cache line.

#ifdef MEM_CHECK

void* PvOSMemoryHeapAlloc_( const size_t cbSize, __in_z const CHAR* szFile, LONG lLine )
{
    if ( !g_fMemCheck )
    {
        return PvOSMemoryHeapAlloc__( cbSize );
    }

    //  check for RFS

    if ( !RFSAlloc( OSMemoryHeap ) )
    {
        return NULL;
    }

    //  calculate the size of the block

    const size_t cbSizeT = sizeof( MAI ) + cbSize;

    // 5745: We need to ensure that cbSizeT = sizeof( MAI ) + cbSize; did not cause an overflow
    if (cbSizeT < cbSize)
    {
        return NULL;
    }

    //  allocate memory from the process global heap

    void* const pv = HeapAlloc( g_hHeap, 0, cbSizeT );
    if ( !pv )
    {
        return pv;
    }

    //  get the true size of the block (heap may allocate a few extra bytes)

    size_t cbAllocSize = HeapSize( g_hHeap, 0, pv );

#ifdef ENABLE_HEAP_MEM_COUNTERS

    AtomicIncrement( (LONG *)&g_cAllocHeap );
    AtomicExchangeAdd( (LONG *)&g_cbAllocHeap, DWORD( cbAllocSize - sizeof( MAI ) ) );

#endif  //  ENABLE_HEAP_MEM_COUNTERS

    //  insert allocation record for this memory

    OSMemoryIInsertHeapAlloc( pv, cbAllocSize, szFile, lLine );

    //  fill memory to detect illegal use of uninit data

    memset( (BYTE*)pv + sizeof( MAI ), bGlobalAllocFill, cbAllocSize - sizeof( MAI ) );

    //  return the block

    return (void*)( (BYTE*)pv + sizeof( MAI ) );
}


void* PvOSMemoryHeapAllocAlign_( const size_t cbSize, const size_t cbAlign, __in_z const CHAR* szFile, LONG lLine )
{
    if ( !g_fMemCheck )
    {
        return PvOSMemoryHeapAllocAlign__( cbSize, cbAlign );
    }

    // 5745: Ensure cbSize + cbAlign cannot overflow
    if (cbSize + cbAlign < cbSize)
    {
        return NULL;
    }

    void* const pv = PvOSMemoryHeapAlloc_( cbSize + cbAlign, szFile, lLine );
    if ( pv )
    {
        return PvOSMemoryHeapIAlign( pv, cbAlign );
    }
    return NULL;
}


#endif  //  !MEM_CHECK


void* PvOSMemoryHeapAlloc__( const size_t cbSize )
{
    //  check for RFS

    if ( !RFSAlloc( OSMemoryHeap ) )
    {
        return NULL;
    }

    //  allocate and return memory from the process global heap

    void* const pv = HeapAlloc( g_hHeap, 0, cbSize );
    if ( !pv )
    {
        //  indicate a low memory error (unfortunately, w/o an instance)
        //
        OSUHAEmitFailureTag( NULL, HaDbFailureTagMemory, L"e6af98a0-b3ed-4d19-b815-894f19fb5a13" );
        return pv;
    }

#ifdef ENABLE_HEAP_MEM_COUNTERS

    //  get the true size of the block (heap may allocate a few extra bytes)

    size_t cbAllocSize = HeapSize( g_hHeap, 0, pv );

    AtomicIncrement( (LONG *)&g_cAllocHeap );
    AtomicExchangeAdd( (LONG *)&g_cbAllocHeap, DWORD( cbAllocSize ) );

#endif  //  ENABLE_HEAP_MEM_COUNTERS

    //  return the block

    return pv;
}

void* PvOSMemoryHeapAllocAlign__( const size_t cbSize, const size_t cbAlign )
{
    // 5745: We need to prevent numeric wrapping in cbSize + cbAlign
    const size_t cbTotal = cbSize + cbAlign;
    // ironic, that lack of 2nd clause causes prefast overflow detection, b/c we were checking, and knew cbAlign was small enough.
    if ( cbTotal < cbSize || cbTotal < cbAlign )
    {
        return NULL;
    }

    void* const pv = PvOSMemoryHeapAlloc( cbTotal );
    if ( pv )
    {
        return PvOSMemoryHeapIAlign( pv, cbAlign );
    }
    return NULL;
}

//  free the specified chunk of memory back to the process heap

void OSMemoryHeapFree( void* const pv )
{
    if ( pv )
    {
        //  recover true allocation pointer

#ifdef MEM_CHECK
        void* pvTrue = (BYTE*)pv - ( g_fMemCheck ? sizeof( MAI ) : 0 );
#else // MEM_CHECK
        void* pvTrue = pv;
#endif  //  MEM_CHECK

#ifdef ENABLE_HEAP_MEM_COUNTERS

        //  recover the true allocation size

        size_t cbAllocSize = HeapSize( g_hHeap, 0, pvTrue );

#ifdef MEM_CHECK

        if ( g_fMemCheck )
        {

            //  fix true allocation size

            cbAllocSize -= sizeof( MAI );

            //  remove allocation record for this memory

            OSMemoryIDeleteHeapAlloc( pvTrue, cbAllocSize );

            //  fill memory to detect illegal use after free

            memset( pv, bGlobalFreeFill, cbAllocSize );
        }

#endif  //  MEM_CHECK

        //  free memory to the process global heap

        AtomicIncrement( (LONG *)&g_cFreeHeap );
        AtomicExchangeAdd( (LONG *)&g_cbAllocHeap, LONG( 0 - cbAllocSize ) );

#endif  //  ENABLE_HEAP_MEM_COUNTERS

        const BOOL fMemFreed = HeapFree( g_hHeap, 0, pvTrue );
        const DWORD dwGleMemFreed = fMemFreed ? ERROR_SUCCESS : GetLastError();
        AssertSz( fMemFreed || FUtilProcessAbort(), "Failed to free heap memory (last error is %d).", dwGleMemFreed );
    }
}


void OSMemoryHeapFreeAlign( void* const pv )
{
    if ( pv )
    {
        OSMemoryHeapFree( PvOSMemoryHeapIUnalign( pv ) );
    }
}


//  Test hooks

const void* PvOSMemoryHookExchangePfns_( void** const ppfnOld, const void* const pfnNew )
{
    const void* pfnOld = *ppfnOld;
    Assert( pfnOld != NULL );
    Assert( pfnNew != NULL );
    
    *ppfnOld = (void* const)pfnNew;
    return pfnOld;
}

const void* PvOSMemoryHookNtQueryInformationProcess( const void* const pfnNew )
{
    return g_pfnNtQueryInformationProcess.PfnLoadHook( pfnNew );
}

const void* PvOSMemoryHookNtQuerySystemInformation( const void* const pfnNew )
{
    return g_pfnNtQuerySystemInformation.PfnLoadHook( pfnNew );
}

const void* PvOSMemoryHookGlobalMemoryStatus( const void* const pfnNew )
{
    return g_pfnGlobalMemoryStatusEx.PfnLoadHook( pfnNew );
}


//  Page Memory Allocation Support

//  returns information about the page memory allocation unit containing the
//  specified pointer including the base address of the region, the size of the
//  region, and the amount of the region committed

void OSMemoryPageIGetAlloc( void* const pv, size_t* const pcbAllocReserve, size_t* const pcbAllocCommit )
{
    //  get pointer to the base of the allocation region

    MEMORY_BASIC_INFORMATION    mbi;
    const size_t                cbRet           = VirtualQuery( pv, &mbi, sizeof( mbi ) );
    Assert( cbRet >= sizeof( mbi ) );

    void*                       pvAllocBase     = mbi.AllocationBase;
    void*                       pvScan          = pvAllocBase;

    //  scan the entire allocation region to determine its size and committed bytes

    *pcbAllocReserve = 0;
    *pcbAllocCommit = 0;
    for ( ; ; )
    {
        //  get this region's attributes

        const size_t    cbT     = VirtualQuery( pvScan, &mbi, sizeof( mbi ) );
        Assert( cbT >= sizeof( mbi ) );

        //  this region is part of the allocation

        if ( mbi.AllocationBase == pvAllocBase )
        {
            Assert( mbi.State == MEM_COMMIT || mbi.State == MEM_RESERVE || mbi.State == MEM_FREE );

            if ( mbi.State != MEM_FREE )
            {
                //  add this region's size to our reserve total

                *pcbAllocReserve += mbi.RegionSize;
            }

            //  this region is committed

            if ( mbi.State == MEM_COMMIT )
            {
                //  add this region's size to our commit total

                *pcbAllocCommit += mbi.RegionSize;
            }

            //  advance to the next region

            pvScan = (void*)( (BYTE*)mbi.BaseAddress + mbi.RegionSize );
        }

        //  this region is not part of the allocation

        else
        {
            //  we're done

            break;
        }
    }
}

//  returns the total size and committed bytes of the specified region, rounding
//  to the nearest page commit granularity on the high and low end of the region

void OSMemoryPageIGetCommit( void* const pv, const size_t cbSize, size_t* const pcbCommit, size_t* const pcbTotal )
{
    //  compute region boundaries, accounting for page commit granularity

    void* pvStart = (BYTE*)pv - DWORD_PTR( pv ) % OSMemoryPageCommitGranularity();
    void* pvEnd = (BYTE*)pv + cbSize - 1;
    pvEnd = (BYTE*)pvEnd + ( OSMemoryPageCommitGranularity() - 1 - DWORD_PTR( pvEnd ) % OSMemoryPageCommitGranularity() );
    pvEnd = (BYTE*)pvEnd + 1;

    //  compute total region size

    *pcbTotal = (BYTE*)pvEnd - (BYTE*)pvStart;

    //  scan region of memory looking for committed memory

    void* pvScan = pvStart;
    *pcbCommit = 0;
    do
    {
        //  get the attributes of the current region

        MEMORY_BASIC_INFORMATION    mbi;
        const size_t                cbRet   = VirtualQuery( pvScan, &mbi, sizeof( mbi ) );
        Assert( cbRet >= sizeof( mbi ) );
        Assert( mbi.State == MEM_COMMIT || mbi.State == MEM_RESERVE || mbi.State == MEM_FREE );

        //  this region is committed

        if ( mbi.State == MEM_COMMIT )
        {
            //  add its size to the total committed bytes count

            *pcbCommit += min( mbi.RegionSize, size_t( (BYTE*)pvEnd - (BYTE*)mbi.BaseAddress ) );
        }

        //  advance to the next region

        pvScan = (void*)( (BYTE*)mbi.BaseAddress + mbi.RegionSize );
    }
    while ( pvScan < pvEnd );
}


//  Page Memory Control

//  reserves and commits a range of virtual addresses of the specifed size,
//  returning NULL if there is insufficient address space or backing store to
//  satisfy the request.  Note that the page reserve granularity applies to
//  this range

#ifdef MEM_CHECK

void* PvOSMemoryPageAlloc_(
    const size_t    cbSize,
    void * const    pv,
    const BOOL      fAllocTopDown,
    __in_z const CHAR * szFile,
    const LONG      lLine )
{
    if ( !g_fMemCheck )
    {
        return PvOSMemoryPageAlloc__( cbSize, pv, fAllocTopDown );
    }

    //  check for RFS

    if (    !RFSAlloc( OSMemoryPageAddressSpace ) ||
            !RFSAlloc( OSMemoryPageBackingStore ) )
    {
        return NULL;
    }

    //  allocate address space and backing store of the specified size

    const DWORD dwFlags = MEM_COMMIT | ( fAllocTopDown ? MEM_TOP_DOWN : 0 );
    void* const pvRet   = VirtualAlloc( pv, cbSize, dwFlags, PAGE_READWRITE );
    if ( !pvRet )
    {
        return pvRet;
    }
    Assert( !pv || pvRet == pv );

#ifdef ENABLE_VM_MEM_COUNTERS
    //  query the state of the address range

    size_t cbAllocReserve;
    size_t cbAllocCommit;
    OSMemoryPageIGetAlloc( pvRet, &cbAllocReserve, &cbAllocCommit );

    //  memory should be fully reserved and committed

    Enforce( cbAllocReserve >= cbSize );
    Enforce( cbAllocCommit >= cbSize );

    //  update the counters

    AtomicExchangeAddPointer( (void**)&g_cbReservePage, (void*)cbAllocReserve );
    AtomicExchangeAddPointer( (void**)&g_cbCommitPage, (void*)cbAllocCommit );

    //  insert allocation record for this memory

    OSMemoryIInsertPageAlloc( pvRet, cbAllocReserve, szFile, lLine );
#endif // ENABLE_VM_MEM_COUNTERS

    return pvRet;
}

#endif  //  !MEM_CHECK

void* PvOSMemoryPageAlloc__( const size_t cbSize, void* const pv, const BOOL fAllocTopDown )
{
    //  check for RFS

#pragma prefast(suppress: 6285, "logical-or of constants is by design")
    if (    !RFSAlloc( OSMemoryPageAddressSpace ) ||
            !RFSAlloc( OSMemoryPageBackingStore ) )
    {
        return NULL;
    }

    //  allocate address space and backing store of the specified size

    const DWORD dwFlags = MEM_COMMIT | ( fAllocTopDown ? MEM_TOP_DOWN : 0 );
    void* const pvRet   = VirtualAlloc( pv, cbSize, dwFlags, PAGE_READWRITE );
    if ( !pvRet )
    {
        //  indicate a low memory error (unfortunately, w/o an instance)
        //
        OSUHAEmitFailureTag( NULL, HaDbFailureTagMemory, L"cc95a8ab-7b96-45e2-bcad-9da4323e74b0" );
        return pvRet;
    }
    Assert( !pv || pvRet == pv );

#ifdef ENABLE_VM_MEM_COUNTERS
    //  query the state of the address range

    size_t cbAllocReserve;
    size_t cbAllocCommit;
    OSMemoryPageIGetAlloc( pvRet, &cbAllocReserve, &cbAllocCommit );

    //  memory should be fully reserved and committed

    Enforce( cbAllocReserve >= cbSize );
    Enforce( cbAllocCommit >= cbSize );

    //  update the counters

    AtomicExchangeAddPointer( (void**)&g_cbReservePage, (void*)cbAllocReserve );
    AtomicExchangeAddPointer( (void**)&g_cbCommitPage, (void*)cbAllocCommit );
#endif // ENABLE_VM_MEM_COUNTERS

    return pvRet;
}


//  free the reserved range of virtual addresses starting at the specified
//  address, freeing any backing store committed to this range

void OSMemoryPageFree( void* const pv )
{
    if ( pv )
    {
#ifdef ENABLE_VM_MEM_COUNTERS
        //  query the state of the address range

        size_t cbAllocReserve;
        size_t cbAllocCommit;
        OSMemoryPageIGetAlloc( pv, &cbAllocReserve, &cbAllocCommit );

        //  the state should be reflected in the counters

        Enforce( g_cbReservePage >= cbAllocReserve );
        Enforce( g_cbCommitPage >= cbAllocCommit );

        //  update the counters

        AtomicExchangeAddPointer( (void**)&g_cbReservePage, (void*)( 0 - cbAllocReserve ) );
        AtomicExchangeAddPointer( (void**)&g_cbCommitPage, (void*)( 0 - cbAllocCommit ) );

#ifdef MEM_CHECK

        if ( g_fMemCheck )
        {

            //  verify that the entire allocated range will be freed
            //  if this goes off, ot means an attempt to free mixed-attribute page memory has been made

            EnforceSz(  cbAllocCommit == 0 || cbAllocCommit == cbAllocReserve, "MemCheckFreeingPartiallyCommittedPage" );

            //  remove allocation record for this memory

            OSMemoryIDeletePageAlloc( pv, cbAllocReserve );
        }

#endif  //  MEM_CHECK
#endif // ENABLE_VM_MEM_COUNTERS

        const BOOL fMemFreed = VirtualFree( pv, 0, MEM_RELEASE );
        const DWORD dwGleMemFreed = fMemFreed ? ERROR_SUCCESS : GetLastError();
        AssertSz( fMemFreed || FUtilProcessAbort(), "Failed to free virtual memory (last error is %d).", dwGleMemFreed );
    }
}


//  reserve a range of virtual addresses of the specified size, returning NULL
//  if there is insufficient address space to satisfy the request.  Note that
//  the page reserve granularity applies to this range

#ifdef MEM_CHECK

void* PvOSMemoryPageReserve_( const size_t cbSize, void* const pv, const __in_z CHAR* szFile, LONG lLine )
{
    if ( !g_fMemCheck )
    {
        return PvOSMemoryPageReserve__( cbSize, pv );
    }

    //  check for RFS

    if ( !RFSAlloc( OSMemoryPageAddressSpace ) )
    {
        return NULL;
    }

    //  allocate address space of the specified size

    void* const pvRet = VirtualAlloc( pv, cbSize, MEM_RESERVE, PAGE_READWRITE );
    if ( !pvRet )
    {
        return pvRet;
    }
    Assert( !pv || pvRet == pv );

#ifdef ENABLE_VM_MEM_COUNTERS
    //  query the current state of the address range

    size_t cbAllocReserve;
    size_t cbAllocCommit;
    OSMemoryPageIGetAlloc( pvRet, &cbAllocReserve, &cbAllocCommit );

    //  memory should be reserved, not committed

    Enforce( cbAllocReserve >= cbSize );
    Enforce( 0 == cbAllocCommit );

    //  update the counters

    AtomicExchangeAddPointer( (void**)&g_cbReservePage, (void*)cbAllocReserve );

    //  insert allocation record for this memory

    OSMemoryIInsertPageAlloc( pvRet, cbAllocReserve, szFile, lLine );
#endif // ENABLE_VM_MEM_COUNTERS

    return pvRet;
}

#endif  //  !MEM_CHECK

void* PvOSMemoryPageReserve__( const size_t cbSize, void* const pv )
{
    //  check for RFS

    if ( !RFSAlloc( OSMemoryPageAddressSpace ) )
    {
        return NULL;
    }

    //  allocate address space of the specified size

    void* const pvRet = VirtualAlloc( pv, cbSize, MEM_RESERVE, PAGE_READWRITE );
    if ( !pvRet )
    {
        //  indicate a low memory error (unfortunately, w/o an instance)
        //
        OSUHAEmitFailureTag( NULL, HaDbFailureTagMemory, L"86e4aab6-b638-49b7-8940-6bdf277d082d" );
        return pvRet;
    }
    Assert( !pv || pvRet == pv );

#ifdef ENABLE_VM_MEM_COUNTERS
    //  query the current state of the address range

    size_t cbAllocReserve;
    size_t cbAllocCommit;
    OSMemoryPageIGetAlloc( pvRet, &cbAllocReserve, &cbAllocCommit );

    //  memory should be reserved, not committed

    Enforce( cbAllocReserve >= cbSize );
    Enforce( 0 == cbAllocCommit );

    //  update the counters

    AtomicExchangeAddPointer( (void**)&g_cbReservePage, (void*)cbAllocReserve );
#endif // ENABLE_VM_MEM_COUNTERS

    return pvRet;
}


//  reset the dirty bit for the specified range of virtual addresses.  this
//  results in the contents of the memory being thrown away instead of paged
//  to disk if the OS needs its physical memory for another process.  a value
//  of fTrue for fToss results in a hint to the OS to throw the specified
//  memory out of our working set.  Note that the page commit granularity
//  applies to this range

void OSMemoryPageReset( void* const pv, const size_t cbSize, const BOOL fToss )
{
    if ( pv )
    {
        //  reset the dirty bit for these vmem pages if set

        (void)VirtualAlloc( pv, cbSize, MEM_RESET, PAGE_READWRITE );

        //  this memory has been selected to be thrown out of our working set

        if ( fToss )
        {
            //  VirtualUnlock() hints the OS to put this memory on the avail list

            (void)VirtualUnlock( pv, cbSize );
        }
    }
}


//  set the specified range of virtual addresses as read only.  Note that the
//  page commit granularity applies to this range

void OSMemoryPageProtect( void* const pv, const size_t cbSize )
{
    if ( cbSize != 0 )
    {
        DWORD flOldProtect;
        BOOL fSetRO = VirtualProtect( pv, cbSize, PAGE_READONLY, &flOldProtect );
        Assert( fSetRO );
    }
}


//  set the specified range of virtual addresses as read / write.  Note that
//  the page commit granularity applies to this range

void OSMemoryPageUnprotect( void* const pv, const size_t cbSize )
{
    if ( cbSize != 0 )
    {
        DWORD flOldProtect;
        BOOL fSetRW = VirtualProtect( pv, cbSize, PAGE_READWRITE, &flOldProtect );
        Assert( fSetRW );
    }
}


//  commit the specified range of virtual addresses, returning fFalse if there
//  is insufficient backing store to satisfy the request.  Note that the page
//  commit granularity applies to this range

BOOL FOSMemoryPageCommit( void* const pv, const size_t cb )
{

    //  check for RFS

    if ( !RFSAlloc( OSMemoryPageBackingStore ) )
    {
        return fFalse;
    }

    //  verify input

    if ( !pv )
    {
        return fFalse;
    }

    //  query the state of the space we are committing

#ifdef ENABLE_VM_MEM_COUNTERS
    size_t cbCommitT;
    size_t cbTotalT;
    OSMemoryPageIGetCommit( pv, cb, &cbCommitT, &cbTotalT );

    //  calculate the amount of memory we will be committing

    const size_t cbToCommitT = cbTotalT - cbCommitT;

    //  we cannot commit more than we have reserved

    Enforce( g_cbCommitPage + cbToCommitT <= g_cbReservePage );
#endif // ENABLE_VM_MEM_COUNTERS

    //  commit the memory

    const BOOL fAllocOK = VirtualAlloc( pv, cb, MEM_COMMIT, PAGE_READWRITE ) != NULL;

    if ( !fAllocOK )
    {
        //  indicate a low memory error (unfortunately, w/o an instance)
        //
        OSUHAEmitFailureTag( NULL, HaDbFailureTagMemory, L"31ae440e-7f2a-4c74-bf11-027f2f5c3f47" );
    }

#ifdef ENABLE_VM_MEM_COUNTERS
    if ( fAllocOK )
    {

        //  update the global counters

        AtomicExchangeAddPointer( (void**)&g_cbCommitPage, (void*)cbToCommitT );
    }
#endif // ENABLE_VM_MEM_COUNTERS

    return fAllocOK;
}


//  decommit the specified range of virtual addresses, freeing any backing
//  store committed to this range.  Note that the page commit granularity
//  applies to this range

void OSMemoryPageDecommit( void* const pv, const size_t cb )
{

    //  verify input

    if ( !pv )
    {
        return;
    }

#ifdef ENABLE_VM_MEM_COUNTERS
    //  query the state of the space we are decommitting

    size_t cbCommitT;
    size_t cbTotalT;
    OSMemoryPageIGetCommit( pv, cb, &cbCommitT, &cbTotalT );

    //  we cannot decommit more than we have reserved or committed

    Enforce( cbCommitT <= g_cbCommitPage );
    Enforce( cbCommitT <= g_cbReservePage );

    //  update the global counter

    AtomicExchangeAddPointer( (void**)&g_cbCommitPage, (void*)( 0 - cbCommitT ) );
#endif // ENABLE_VM_MEM_COUNTERS

    //  free backing store for the specified range

#pragma prefast(suppress:6250, "We specifically are only decommitting, not releasing." )
    const BOOL fFreeOK = VirtualFree( pv, cb, MEM_DECOMMIT );
    Assert( fFreeOK );
}

#ifndef MINIMAL_FUNCTIONALITY

//  locks the specified range of virtual addresses, returning fFalse if there
//  is insufficient quota to satisfy the request.  Note that the page commit
//  granularity applies to this range

BOOL FOSMemoryPageLock( void* const pv, const size_t cb )
{
    //  try to lock the page until we succeed or we know we cannot succeed
    //
    //  NOTE:  just in case, we will also limit ourselves to 10 retries

    Expected( fFalse ); // written, used for testing occasionally, not used right now. compiled out of MINESE

    const INT cRetry = 10;
    for ( INT iRetry = 0; pv && iRetry < cRetry; iRetry++ )
    {
        //  try to lock the address range with existing process settings.  if we
        //  succeed then return success.  if we fail for any reason other than
        //  insufficient quota then return failure

#pragma prefast ( suppress: __WARNING_OVERFLOW_OR_UNDERFLOW_IN_ALLOC_SIZE, "suppessed due to a bug in prefast.  It thinks VirtualLock allocates memory and it does not (in the classical sense).  It also thinks that 'cb' could be smaller than 'cb + cbAlign' which is backwards." )
        if ( VirtualLock( pv, cb ) )
        {
            return fTrue;
        }
        else if ( GetLastError() != ERROR_WORKING_SET_QUOTA )
        {
            return fFalse;
        }

#ifndef ENABLE_WORKINGSET_ADJUST_ON_LOCK_OOM    // by default, not on

        AssertSz( fFalse, "This code is not called by anything else, so I'm disabling it to be able to link to CoreSystem." );

#else

        //  try to grow our process quota.  if we cannot do this then return
        //  failure because we will not be able to lock the memory

        SIZE_T  cbWSMin;
        SIZE_T  cbWSMax;

        if ( !GetProcessWorkingSetSize( GetCurrentProcess(), &cbWSMin, &cbWSMax ) )
        {
            return fFalse;
        }

        const SIZE_T    cbAlign         = OSMemoryPageCommitGranularity();
        const SIZE_T    cbWSGrow        = ( ( cb + cbAlign - 1 ) / cbAlign ) * cbAlign;
        const SIZE_T    cbWSMinNew      = max( cbWSMin + cbWSGrow, cbWSMin );
        const SIZE_T    cbWSMaxNew      = max( cbWSMax + cbWSGrow, cbWSMax );

        if ( !SetProcessWorkingSetSize( GetCurrentProcess(), cbWSMinNew, cbWSMaxNew ) )
        {
            return fFalse;
        }
#endif // ENABLE_WORKINGSET_ADJUST_ON_LOCK_OOM
    }

    //  we didn't lock the memory successfully

    return fFalse;
}

#endif

//  unlocks the specified range of virtual addresses.  Note that the page
//  commit granularity applies to this range

void OSMemoryPageUnlock( void* const pv, const size_t cb )
{
    //  unlock the specified virtual pages

    if ( pv )
    {
        const BOOL fUnlockOK = VirtualUnlock( pv, cb );
        Assert( fUnlockOK );
    }
}


//  Memory Mapping


//  COSMemoryMap -- the basic object used in all memory mapping

//  ctor

COSMemoryMap::COSMemoryMap()
{
    m_pvMap = NULL;
    m_cbMap = 0;
    m_cMap = 0;

    m_cbReserve = 0;
    m_cbCommit = 0;

#ifdef MEM_CHECK

    m_posmmNext = NULL;
    m_fInList = fFalse;
    m_szFile = NULL;
    m_lLine = 0;

#endif  //  MEM_CHECK
}


//  dtor

COSMemoryMap::~COSMemoryMap()
{
}


//  init

COSMemoryMap::ERR
COSMemoryMap::ErrOSMMInit()
{
    m_pvMap = NULL;
    m_cbMap = 0;
    m_cMap = 0;

    m_cbReserve = 0;
    m_cbCommit = 0;

#ifdef MEM_CHECK

    m_posmmNext = NULL;
    m_fInList = fFalse;
    m_szFile = NULL;
    m_lLine = 0;

    if ( g_fMemCheck )
    {

        //  insert this COSMemoryMap object into the global list

        EnterCriticalSection( &g_csosmm );
        m_posmmNext = g_posmm;
        g_posmm = this;
        m_fInList = fTrue;
        LeaveCriticalSection( &g_csosmm );
    }

#endif  //  MEM_CHECK

    return ERR::errSuccess;
}


//  term

VOID COSMemoryMap::OSMMTerm()
{
    //  make sure all resources have been released

    Enforce( 0 == m_cbReserve );
    Enforce( 0 == m_cbCommit );

#ifdef MEM_CHECK

    if ( g_fMemCheck && m_fInList )
    {

        //  remove this object from the list

        EnterCriticalSection( &g_csosmm );
        COSMemoryMap *posmmCur;
        COSMemoryMap *posmmPrev;

        posmmCur = g_posmm;
        posmmPrev = NULL;
        while ( posmmCur && posmmCur != this )
        {
            posmmPrev = posmmCur;
            posmmCur = posmmCur->m_posmmNext;
        }
        if ( posmmCur )
        {
            if ( posmmPrev )
            {
                posmmPrev->m_posmmNext = m_posmmNext;
            }
            else
            {
                g_posmm = m_posmmNext;
            }
        }
        else
        {
            EnforceSz( fFalse, "MemCheckMemMapListCorrupted" );
        }
        LeaveCriticalSection( &g_csosmm );

        m_fInList = fFalse;
    }

#endif  //  MEM_CHECK
}

//  returns fTrue if we can map the same chunk of backing store into more than
//  one address range

BOOL COSMemoryMap::FCanMultiMap()
{
    return fTrue;
}

//  reserve a chunk of backing store and map it to the given address(es).
//  if any of the given addresses in rgpvMap[] are NULL, a suitable address
//  will be chosen automatically.  on success, the location of each mapping
//  will be returned in rgpvMap[].  on failure, the state of rgpvMap[] is
//  undefined.
//
//  NOTE: backing store it not actually allocated until commit time
//  NOTE: page reserve granularity applies to this range


COSMemoryMap::ERR
COSMemoryMap::ErrOSMMReserve__( const size_t        cbMap,
                                const size_t        cMap,
                                __inout_ecount(cMap) void** const       rgpvMap,
                                const BOOL* const   rgfProtect )
{
    ERR err;

    //  check for RFS

    if ( !RFSAlloc( OSMemoryPageBackingStore ) )
    {
        return ERR::errOutOfBackingStore;
    }
    if ( !RFSAlloc( OSMemoryPageAddressSpace ) )
    {
        return ERR::errMappingFailed;
    }

    size_t  cbMapT;
    HANDLE  hBackingStore   = NULL;
    size_t  iMap            = 0;

#ifdef ENABLE_MM_MEM_COUNTERS
    //  verify our state (should not already have a mapping setup)

    Assert( !m_pvMap );
    Assert( 0 == m_cbMap );
    Assert( 0 == m_cMap );
#endif // ENABLE_MM_MEM_COUNTERS

    //  verify input

    Assert( cbMap > 0 );
    Assert( cMap > 0 );
    Assert( rgpvMap );
    Assert( rgfProtect );

    //  round to the nearest reservation granularity

    cbMapT =    (   ( cbMap + OSMemoryPageReserveGranularity() - 1 ) /
                    OSMemoryPageReserveGranularity() ) *
                OSMemoryPageReserveGranularity();

    //  allocate the backing store

    hBackingStore = CreateFileMappingW( INVALID_HANDLE_VALUE,
                                        NULL,
                                        PAGE_READWRITE | SEC_RESERVE,
                                        sizeof( cbMapT ) == sizeof( QWORD ) ? DWORD( QWORD( cbMapT ) >> ( sizeof( DWORD ) * 8 ) ) : 0,
                                        DWORD( cbMapT ),
                                        NULL );
    if ( !hBackingStore )
    {
        //  indicate a low memory error (unfortunately, w/o an instance)
        //
        OSUHAEmitFailureTag( NULL, HaDbFailureTagMemory, L"d76ba6e5-f9b9-44cd-b7e8-006acd7c2f35" );
        return ERR::errOutOfBackingStore;
    }

    //  map the backing store to given location(s)

    while ( iMap < cMap )
    {
        void* const pvMap = MapViewOfFileEx(    hBackingStore,
                                                rgfProtect[iMap] ? FILE_MAP_READ : FILE_MAP_WRITE,
                                                0,
                                                0,
                                                0,
                                                rgpvMap[iMap] );
        if ( !pvMap )
        {
            err = ERR::errMappingFailed;
            goto HandleError;
        }

        //  record the result of the mapping

        Assert( !rgpvMap[iMap] || pvMap == rgpvMap[iMap] );
        rgpvMap[iMap] = pvMap;

        //  move to the next mapping

        iMap++;
    }

    BOOL fCloseOK;
    fCloseOK = CloseHandle( hBackingStore );
    Assert( fCloseOK );

    //  record the state of the entire mapping

    m_pvMap = rgpvMap[0];
    m_cbMap = cbMapT;
    m_cMap = cMap;

#ifdef ENABLE_MM_MEM_COUNTERS
    //  update our counters

    m_cbReserve = cMap * cbMapT;
    m_cbCommit = 0;

    //  update the global counters

    AtomicExchangeAddPointer( (void**)&g_cbReservePage, (void*)m_cbReserve );
#endif // ENABLE_MM_MEM_COUNTERS

    return ERR::errSuccess;

HandleError:

    //  cleanup any leftover mappings

    while ( iMap-- > 0 )
    {
        const BOOL fUnmapOK = UnmapViewOfFile( rgpvMap[iMap] );
        Assert( fUnmapOK );
    }

    //  cleanup the backing store

    Assert( hBackingStore );
    fCloseOK = CloseHandle( hBackingStore );
    Assert( fCloseOK );

    return err;
}

#ifdef MEM_CHECK

COSMemoryMap::ERR
COSMemoryMap::ErrOSMMReserve_(  const size_t        cbMap,
                                const size_t        cMap,
                                void** const        rgpvMap,
                                const BOOL* const   rgfProtect,
                                __in_z const CHAR*      szFile,
                                LONG                lLine )
{

    //  do the reservation

    const ERR err = ErrOSMMReserve__( cbMap, cMap, rgpvMap, rgfProtect );

    if ( ERR::errSuccess == err && g_fMemCheck )
    {

        //  record the file/line responsible for this reservation

        Assert( !m_szFile );
        Assert( 0 == m_lLine );
        m_szFile = const_cast<CHAR*>( szFile );
        m_lLine = lLine;
    }

    return err;
}

#endif  //  MEM_CHECK


//  commit a chunk of the reservation.  this forces the backing store to be
//  allocated and assigned to the appropriate address range.  on success,
//  the specified region within each mapping will be committed.
//
//  NOTE: page commit granularity applies to this range

BOOL COSMemoryMap::FOSMMCommit( const size_t cbCommit )
{

    //  check for RFS

    if ( !RFSAlloc( OSMemoryPageBackingStore ) )
    {
        return fFalse;
    }

    //  verify our state (should have a mapping setup)

    Assert( m_pvMap );
    Assert( m_cbMap > 0 );
    Assert( m_cMap > 0 );

    //  verify input

    Assert( cbCommit <= m_cbMap );

    //  calculate the address we are committing

    void *const pvCommit = (BYTE*)m_pvMap;

    //  query the state of the space we are committing

#ifdef ENABLE_MM_MEM_COUNTERS
    size_t cbCommitT;
    size_t cbTotalT;
    OSMemoryPageIGetCommit( pvCommit, cbCommit, &cbCommitT, &cbTotalT );

    //  calculate the amount of memory we will be committing

    const size_t cbToCommitT = ( cbTotalT - cbCommitT ) * m_cMap;

    //  we cannot commit more than we have reserved

    Enforce( m_cbCommit + cbToCommitT <= m_cbReserve );
#endif // ENABLE_MM_MEM_COUNTERS

    //  commit the memory

    const BOOL fAllocOK = VirtualAlloc( pvCommit, cbCommit, MEM_COMMIT, PAGE_READWRITE ) != NULL;

    if ( !fAllocOK )
    {
        //  indicate a low memory error (unfortunately, w/o an instance)
        //
        OSUHAEmitFailureTag( NULL, HaDbFailureTagMemory, L"6cdb16d3-1c0f-44af-ba41-775044f62c76" );
    }

#ifdef ENABLE_MM_MEM_COUNTERS
    if ( fAllocOK )
    {

        //  update our counters

        AtomicExchangeAddPointer( (void**)&m_cbCommit, (void*)cbToCommitT );

        //  update the global counters

        AtomicExchangeAddPointer( (void**)&g_cbCommitPage, (void*)cbToCommitT );
    }
#endif // ENABLE_MM_MEM_COUNTERS

    //  HACK:  workaround WOW64 bug where multiply mapped memory is not committed properly

#ifndef MINIMAL_FUNCTIONALITY
    if ( FUtilIProcessIsWow64() )
    {
        for ( size_t iMap = 1; iMap < m_cMap; iMap++ )
        {
            VirtualAlloc( (BYTE*)m_pvMap + iMap * m_cbMap, m_cbMap, MEM_COMMIT, PAGE_READWRITE );
        }
    }
#endif

    return fAllocOK;
}


//  decommit and unreserve the address range

VOID COSMemoryMap::OSMMFree( void *const pv )
{

    //  verify our state (should have a mapping setup)

    Assert( m_pvMap );
    Assert( m_cbMap > 0 );
    Assert( m_cMap > 0 );

    //  make sure we have a valid address

    if ( !pv )
    {
        return;
    }

#ifdef ENABLE_MM_MEM_COUNTERS
    //  query the current state of the allocated block

    size_t cbAllocReserve;
    size_t cbAllocCommit;
    OSMemoryPageIGetAlloc( pv, &cbAllocReserve, &cbAllocCommit );

    //  verify the amount of reserved and committed memory

    Enforce( cbAllocReserve <= m_cbMap );
    Enforce( cbAllocCommit <= m_cbMap );

    //  verify that the counters reflect the reserved/committed sizes

    Enforce( g_cbReservePage >= cbAllocReserve );
    Enforce( g_cbCommitPage >= cbAllocCommit );
    Enforce( m_cbReserve >= cbAllocReserve );
    Enforce( m_cbCommit >= cbAllocCommit );

    //  update our counters

    AtomicExchangeAddPointer( (void**)&m_cbReserve, (void*)( 0 - cbAllocReserve ) );
    AtomicExchangeAddPointer( (void**)&m_cbCommit, (void*)( 0 - cbAllocCommit ) );

    //  update the global counters

    AtomicExchangeAddPointer( (void**)&g_cbReservePage, (void*)( 0 - cbAllocReserve ) );
    AtomicExchangeAddPointer( (void**)&g_cbCommitPage, (void*)( 0 - cbAllocCommit ) );

#ifdef DEBUG

    if ( 0 == m_cbReserve )
    {

        //  we are about to free that last portion of this mapping

        Assert( 0 == m_cbCommit );

        //  reset the rest of the object so it can be used again

        m_pvMap = NULL;
        m_cbMap = 0;
        m_cMap = 0;

#ifdef MEM_CHECK

        m_szFile = NULL;
        m_lLine = NULL;

#endif  //  MEM_CHECK
    }

#endif  //  DEBUG
#endif // ENABLE_MM_MEM_COUNTERS

    //  free the specified range

    const BOOL fUnmapOK = UnmapViewOfFile( pv );
    Assert( fUnmapOK );
}


//  reserve and commit a series of mappings to create a large contiguous
//  pattern (e.g. zero-filled block)
//
//  NOTE: page reserve granularity applies here

#ifdef MEM_CHECK

COSMemoryMap::ERR
COSMemoryMap::ErrOSMMPatternAlloc_( const size_t    cbPattern,
                                    const size_t    cbSize,
                                    void** const    ppvPattern,
                                    __in_z const CHAR*  szFile,
                                    LONG            lLine )
{
    if ( !g_fMemCheck )
    {
        return ErrOSMMPatternAlloc__( cbPattern, cbSize, ppvPattern );
    }

    ERR err;

#ifdef ENABLE_MM_MEM_COUNTERS
    //  verify our state (should not have a mapping setup)

    Assert( !m_pvMap );
    Assert( 0 == m_cbMap );
    Assert( 0 == m_cMap );
#endif // ENABLE_MM_MEM_COUNTERS

    //  verify input

    Assert( cbPattern > 0 );
    Assert( cbSize >= cbPattern );
    Assert( ppvPattern );

    //  reset output

    *ppvPattern = NULL;

    //  round to the nearest reservation granularity

    size_t cbPatternT;
    cbPatternT =    (   ( cbPattern + OSMemoryPageReserveGranularity() - 1 ) /
                        OSMemoryPageReserveGranularity() ) *
                    OSMemoryPageReserveGranularity();

    size_t cbSizeT;
    cbSizeT =       (   ( cbSize + OSMemoryPageReserveGranularity() - 1 ) /
                        OSMemoryPageReserveGranularity() ) *
                    OSMemoryPageReserveGranularity();

    Assert( cbSizeT >= cbPatternT );
    Assert( 0 == cbSizeT % cbPatternT );

    //  allocate space to describe the mappings (for addresses and protection)

    size_t  cMap        = cbSizeT / cbPatternT;
    void    **rgpvMap   = (void**)( _alloca( cMap * ( sizeof( void* ) + sizeof( BOOL ) ) ) );
    BOOL    *rgfProtect = (BOOL*)( (BYTE*)rgpvMap + cMap * sizeof( void* ) );

    Assert( cMap > 0 );

    while ( fTrue )
    {

        //  find a suitable address range

        BYTE* rgb;
        if ( !( rgb = (BYTE*)PvOSMemoryPageReserve_( size_t( cbSizeT ), NULL, szFile, lLine ) ) )
        {
            return ERR::errOutOfAddressSpace;
        }
        OSMemoryPageFree( rgb );

        //  setup each mapping

        size_t iMap;
        for ( iMap = 0; iMap < cMap; iMap++ )
        {
            rgpvMap[iMap]       = rgb + ( iMap * cbPatternT );
            rgfProtect[iMap]    = fFalse;
        }

        //  make the mapping

        err = ErrOSMMReserve_( cbPatternT, cMap, rgpvMap, rgfProtect, szFile, lLine );
        if ( ERR::errSuccess == err )
        {
            break;
        }
        else if ( ERR::errOutOfBackingStore == err )
        {
            return err;
        }
        else
        {
            Assert( ERR::errMappingFailed == err );
        }
    }

    //  we should have a valid pattern

    Assert( rgpvMap[0] );
    Assert( m_pvMap == rgpvMap[0] );
    Assert( m_cbMap == cbPatternT );
    Assert( m_cMap == cMap );

#ifdef ENABLE_MM_MEM_COUNTERS
    Assert( m_cbReserve == cbSizeT );
#endif // ENABLE_MM_MEM_COUNTERS
    Assert( m_cbCommit == 0 );

    //  commit the mapping

    if ( !FOSMMCommit( cbPatternT ) )
    {

        //  free each mapping

        size_t iMap;
        for ( iMap = 0; iMap < cMap; iMap++ )
        {
            Assert( (BYTE*)m_pvMap + ( iMap * cbPatternT ) == rgpvMap[iMap] );
            OSMMFree( rgpvMap[iMap] );
        }
        m_pvMap = NULL;

        return ERR::errOutOfMemory;
    }

#ifdef ENABLE_MM_MEM_COUNTERS
    Assert( m_cbCommit == cbSizeT );
#endif // ENABLE_MM_MEM_COUNTERS

    //  return the result

    *ppvPattern = rgpvMap[0];

    return ERR::errSuccess;
}

#endif  //  MEM_CHECK

COSMemoryMap::ERR
COSMemoryMap::ErrOSMMPatternAlloc__(    const size_t    cbPattern,
                                        const size_t    cbSize,
                                        void** const    ppvPattern )
{
    ERR err;

#ifdef ENABLE_MM_MEM_COUNTERS
    //  verify our state (should not have a mapping setup)

    Assert( !m_pvMap );
    Assert( 0 == m_cbMap );
    Assert( 0 == m_cMap );
#endif // ENABLE_MM_MEM_COUNTERS

    //  verify input

    Assert( cbPattern > 0 );
    Assert( cbSize >= cbPattern );
    Assert( ppvPattern );

    //  reset output

    *ppvPattern = NULL;

    //  round to the nearest reservation granularity

    size_t cbPatternT;
    cbPatternT =    (   ( cbPattern + OSMemoryPageReserveGranularity() - 1 ) /
                        OSMemoryPageReserveGranularity() ) *
                    OSMemoryPageReserveGranularity();

    size_t cbSizeT;
    cbSizeT =       (   ( cbSize + OSMemoryPageReserveGranularity() - 1 ) /
                        OSMemoryPageReserveGranularity() ) *
                    OSMemoryPageReserveGranularity();

    Assert( cbSizeT >= cbPatternT );
    Assert( 0 == cbSizeT % cbPatternT );

    // 5745: prevent numeric overflows.
    if (cbSizeT < cbSize)
    {
        return ERR::errOutOfAddressSpace;
    }
    if ( cbPatternT < cbPattern )
    {
        return ERR::errOutOfAddressSpace;
    }
    //  allocate space to describe the mappings (for addresses and protection)

    size_t  cMap        = cbSizeT / cbPatternT;
    //  Prevent overflow here as well.
    if ( cMap > ulMax/(sizeof(void*)+sizeof(BOOL)) )
    {
        return ERR::errOutOfAddressSpace;
    }
    void    **rgpvMap   = (void**)( _alloca( cMap * ( sizeof( void* ) + sizeof( BOOL ) ) ) );
    BOOL    *rgfProtect = (BOOL*)( (BYTE*)rgpvMap + cMap * sizeof( void* ) );

    Assert( cMap > 0 );

    while ( fTrue )
    {

        //  find a suitable address range

        BYTE* rgb;
        if ( !( rgb = (BYTE*)PvOSMemoryPageReserve__( size_t( cbSizeT ), NULL ) ) )
        {
            return ERR::errOutOfAddressSpace;
        }
        OSMemoryPageFree( rgb );

        //  setup each mapping

        size_t iMap;
        for ( iMap = 0; iMap < cMap; iMap++ )
        {
            rgpvMap[iMap]       = rgb + ( iMap * cbPatternT );
            rgfProtect[iMap]    = fFalse;
        }

        //  make the mapping

        err = ErrOSMMReserve__( cbPatternT, cMap, rgpvMap, rgfProtect );
        if ( ERR::errSuccess == err )
        {
            break;
        }
        else if ( ERR::errOutOfBackingStore == err )
        {
            return err;
        }
        else
        {
            Assert( ERR::errMappingFailed == err );
        }
    }

    //  we should have a valid pattern

    Assert( rgpvMap[0] );
    Assert( m_pvMap == rgpvMap[0] );
    Assert( m_cbMap == cbPatternT );
    Assert( m_cMap == cMap );

#ifdef ENABLE_MM_MEM_COUNTERS
    Assert( m_cbReserve == cbSizeT );
#endif // ENABLE_MM_MEM_COUNTERS
    Assert( m_cbCommit == 0 );

    //  commit the mapping

    if ( !FOSMMCommit( cbPatternT ) )
    {

        //  free each mapping

        size_t iMap;
        for ( iMap = 0; iMap < cMap; iMap++ )
        {
            Assert( (BYTE*)m_pvMap + ( iMap * cbPatternT ) == rgpvMap[iMap] );
            OSMMFree( rgpvMap[iMap] );
        }
        m_pvMap = NULL;

        return ERR::errOutOfMemory;
    }

#ifdef ENABLE_MM_MEM_COUNTERS
    Assert( m_cbCommit == cbSizeT );
#endif // ENABLE_MM_MEM_COUNTERS

    //  return the result

    *ppvPattern = rgpvMap[0];

    return ERR::errSuccess;
}


//  free the series of mappings used to make a pattern

VOID COSMemoryMap::OSMMPatternFree()
{

    //  verify our state (should have a mapping setup)

    if ( m_pvMap )
    {
        Assert( m_cbMap > 0 );
        Assert( m_cMap > 0 );

        //  free each mapping

        size_t iMap;
        for ( iMap = 0; iMap < m_cMap; iMap++ )
        {
            OSMMFree( (BYTE*)m_pvMap + ( iMap * m_cbMap ) );
        }
        m_pvMap = NULL;
    }

#ifdef ENABLE_MM_MEM_COUNTERS
    //  we should no longer have any mapping info

    Assert( !m_pvMap );
    Assert( 0 == m_cbMap );
    Assert( 0 == m_cMap );
#endif // ENABLE_MM_MEM_COUNTERS
}



#ifdef MEM_CHECK

//  dump stray COSMemoryMap objects

VOID COSMemoryMap::OSMMDumpAlloc( __in_z const WCHAR* szFile )
{
    HANDLE hFile = CreateFileW( szFile,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );

    if ( INVALID_HANDLE_VALUE == hFile )
    {
        return;
    }

    CHAR            szMessage[512];
    DWORD           cchActual;
    COSMemoryMap    *posmm;

    const LARGE_INTEGER ibOffset = { 0, 0 };
    (void)SetFilePointerEx( hFile, ibOffset, NULL, FILE_END );

    OSStrCbFormatA( szMessage, sizeof(szMessage), "\r\nCOSMemoryMap Leak Statistics\r\n\r\n" );
    Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
    (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );

    OSStrCbFormatA( szMessage, sizeof(szMessage), "g_cbReservePage = %I64d pages (0x%016I64x bytes)\r\n", g_cbReservePage / OSMemoryPageCommitGranularity(), g_cbReservePage );
    Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
    (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );

    OSStrCbFormatA( szMessage, sizeof(szMessage), "g_cbCommitPage  = %I64d pages (0x%016I64x bytes)\r\n\r\n", g_cbCommitPage / OSMemoryPageCommitGranularity(), g_cbCommitPage );
    Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
    (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );

    OSStrCbFormatA( szMessage, sizeof(szMessage), "First Mapping       Size                Count  Reserved            Committed           File(Line)\r\n" );
    Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
    (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );

    OSStrCbFormatA( szMessage, sizeof(szMessage), "==================  ==================  =====  ==================  ==================  ==========================================\r\n" );
    Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
    (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );

    EnterCriticalSection( &g_csosmm );

    //  dump each COSMemoryMap object in the list

    posmm = g_posmm;
    while ( posmm )
    {
        OSStrCbFormatA( szMessage, sizeof(szMessage),
                    "0x%016I64X  0x%016I64X  %-5d  0x%016I64X  0x%016I64X  %s(%d)\r\n",
                    QWORD( posmm->m_pvMap ),
                    QWORD( posmm->m_cbMap ),
                    DWORD( posmm->m_cMap ),
                    QWORD( posmm->m_cbReserve ),
                    QWORD( posmm->m_cbCommit ),
                    posmm->m_szFile,
                    posmm->m_lLine );
        Assert( strlen( szMessage ) < ARRAYSIZE( szMessage ) );
        (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );

        posmm = posmm->m_posmmNext;
    }
    if ( !g_posmm )
    {
        OSStrCbFormatA( szMessage, sizeof(szMessage), "<< no mappings >>\r\n" );
        Assert( strlen( szMessage ) < sizeof( szMessage ) / sizeof( CHAR ) );
        (void)WriteFile( hFile, szMessage, strlen( szMessage ), &cchActual, NULL );
    }

    LeaveCriticalSection( &g_csosmm );

    CloseHandle( hFile );
}

#endif  //  MEM_CHECK

BOOL FUtilZeroed( __in_bcount(cbData) const BYTE * pbData, __in const size_t cbData )
{
    Assert( pbData != NULL );

    if ( cbData == 0 )
    {
        return fTrue;
    }

    const size_t cbAlignment = sizeof( QWORD );
    const BYTE* const pbDataMax = pbData + cbData - 1;
    const BYTE* const pbDataAlignedFirst = (BYTE*)roundup( (DWORD_PTR)pbData, cbAlignment );
    const BYTE* const pbDataPostAligned = (BYTE*)rounddn( (DWORD_PTR)pbDataMax, cbAlignment );

    // First unaligned bytes.
    while ( ( pbData <= pbDataMax ) && ( pbData < pbDataAlignedFirst ) )
    {
        if ( *pbData != 0 )
        {
            return false;
        }
        pbData++;
    }

    // Aligned bytes.
    while ( pbData < pbDataPostAligned )
    {
        if ( *( (QWORD*)pbData ) != 0 )
        {
            return false;
        }
        pbData += cbAlignment;
    }

    // Last unaligned bytes.
    while ( pbData <= pbDataMax )
    {
        if ( *pbData != 0 )
        {
            return false;
        }
        pbData++;
    }

    return fTrue;
}


size_t IbUtilLastNonZeroed( __in_bcount(cbData) const BYTE * pbData, __in const size_t cbData )
{
    Assert( pbData != NULL );

    if ( cbData == 0 )
    {
        return 0;
    }

    size_t ibData = cbData;
    do
    {
        ibData--;
        if ( pbData[ ibData ] != 0 )
        {
            return ibData;
        }
    }
    while ( ibData > 0 );

    return cbData;
}

