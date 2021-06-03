// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#ifndef _RESMGR_HXX_INCLUDED
#define _RESMGR_HXX_INCLUDED


//  asserts
//
//  #define RESMGRAssert to point to your favorite assert function per #include

#ifdef RESMGRAssert
#else  //  !RESMGRAssert
#define RESMGRAssert    Assert
#endif  //  RESMGRAssert


#ifdef COLLAssert
#else  //  !COLLAssert
#define COLLAssert RESMGRAssert
#endif  //  COLLAssert

//  Build Options
//

#define RESSCAN_STATISTICS                      1
#define FASTEVICT_STATISTICS                    1

//  enables default automatic tick validation
//#define RESMGR_ENABLE_TICK_VALIDATION         1

#ifdef FASTEVICT_STATISTICS
#define FE_STATISTICS(x) x
#else   //  !FASTEVICT_STATISTICS
#define FE_STATISTICS(x)
#endif  //  FASTEVICT_STATISTICS

#ifdef RESSCAN_STATISTICS
#define RS_STATISTICS(x) x
#else   //  !RESSCAN_STATISTICS
#define RS_STATISTICS(x)
#endif  //  RESSCAN_STATISTICS

//  externalized time function

//  we allow test modules using us to override our time function (only for testing purposes).

#ifdef TickRESMGRTimeCurrent
#else  //  !TickRESMGRTimeCurrent
#define TickRESMGRTimeCurrent TickOSTimeCurrent
#endif  //  TickRESMGRTimeCurrent

#include "stat.hxx"
#include "collection.hxx"

#include <math.h>


NAMESPACE_BEGIN( RESMGR );


//////////////////////////////////////////////////////////////////////////////////////////
//  CDBAResourceAllocationManager
//
//  Implements a class used to manage how much memory a resource pool should use as a
//  function of the real-time usage characteristics of that pool and the entire system.
//  Naturally, the managed resource would have to be of a type where retention in memory
//  is optional, at least to some extent.
//
//  The resource pool's size is managed by the Dynamic Buffer Allocation (DBA) algorithm.
//  DBA was originally invented by SOMEONE in 1997 to manage the size of the
//  multiple database page caches in Exchange Server 5.5.  The resource pool currently
//  uses a new version of DBA that was reinvented in 2002 for Windows Server 2003 and
//  is patent pending.
//
//  The class is written to be platform independent.  As such, there are several pure
//  virtual functions that must be implemented to provide the data about the OS that the
//  resource manager needs to make decisions.  To provide these functions, derive the
//  class for the desired platform and then define these functions:
//
//    TotalPhysicalMemory()
//
//        Returns the total amount of pageable physical memory in the system.
//
//    AvailablePhysicalMemory()
//
//        Returns the amount of available pageable physical memory in the system.
//
//    TotalPhysicalMemoryEvicted()
//
//        Returns the total amount of physical memory evicted.
//
//  There are also other pure virtual functions that must be implemented to provide
//  resource specific data.  To provide these functions, derive the class for each
//  resource to manage and then define these functions:
//
//    TotalResources()
//
//        Returns the total amount of resources in the resource pool.
//
//    TotalResourcesEvicted()
//
//        Returns the total amount of resources evicted.
//
//  With the above implemented, the client must utilize these next couple functions to
//  update the statistics and retrieve the recommended pool adjustment size.
//
//    UpdateStatistics()
//
//        Must be called once per second to sample the above resource specific data and 
//        update the current resource pool size recommendation.  The statistics need to
//        be reset before use via ResetStatistics().  This function can be called at any
//        time to reset the manager's statistics for whatever reason.
//
//    ConsumeResourceAdjustments()
//
//        Returns (destructively) the amount the resource pool should be adjusted up 
//        or down.  Once called, the value is removed, so the user must consume it.
//        Subsequent calls will return 0, until UpdateStatistics() is called again.
//

template< size_t m_cAvgDepth >
class CDBAResourceAllocationManager
{
    public:

        CDBAResourceAllocationManager();
        virtual ~CDBAResourceAllocationManager();

        virtual void    ResetStatistics();
        virtual void    UpdateStatistics();
        virtual void    ConsumeResourceAdjustments( __out double * const pdcbTotalResource, __in const double cbResourceSize );

    protected:

        virtual size_t  TotalPhysicalMemory()                                   = 0;
        virtual size_t  AvailablePhysicalMemory()                               = 0;
        virtual size_t  TotalPhysicalMemoryEvicted()                            = 0;

        virtual QWORD   TotalResources()                                        = 0;
        virtual QWORD   TotalResourcesEvicted()                                 = 0;

    private:

        static const BYTE m_dpctResourceMax = 100;

        template <class T>
        void InitArray_( _Out_writes_(cElements) T* const rgArray, _In_ const size_t cElements, _In_ const T value )
        {
            for ( size_t iElement = 0 ; iElement < cElements ; iElement++ )
            {
#pragma warning(suppress: 22103) // Esp:1139 False positive from MSRC analyzer
                rgArray[ iElement ] = value;
            }
        };

        template <class T>
        T ComputeArrayAvg_( const T* const rgArray, const size_t cElements )
        {
            T remainders    = 0;
            T sum           = 0;
        
            for ( size_t iElement = 0 ; iElement < cElements ; iElement++ )
            {
                remainders += rgArray[ iElement ] % cElements;
                sum += rgArray[ iElement ] / cElements;
            }
        
            return sum + remainders / cElements;
        };

        size_t  m_cbTotalPhysicalMemoryEvictedLast;
        QWORD   m_cbTotalResourcesEvictedLast;

        size_t  m_icbOldest;

        size_t  m_rgcbTotalPhysicalMemory[ m_cAvgDepth ];
        size_t  m_rgcbAvailablePhysicalMemory[ m_cAvgDepth ];
        size_t  m_rgdcbEvictedPhysicalMemory[ m_cAvgDepth ];

        QWORD   m_rgcbTotalResources[ m_cAvgDepth ];
        QWORD   m_rgdcbEvictedResources[ m_cAvgDepth ];

        //  results of caculation

        double  m_dcbTotalResource;
};

//  ctor

template< size_t m_cAvgDepth >
inline CDBAResourceAllocationManager< m_cAvgDepth >::CDBAResourceAllocationManager()
{
    //  nothing to do
}

//  virtual dtor

template< size_t m_cAvgDepth >
inline CDBAResourceAllocationManager< m_cAvgDepth >::~CDBAResourceAllocationManager()
{
    //  nothing to do
}

//  resets the observed statistics for the resource pool and the system such
//  that the current state appears to have been the steady state for as long as
//  past behavioral data is retained

template< size_t m_cAvgDepth >
inline void CDBAResourceAllocationManager< m_cAvgDepth >::ResetStatistics()
{
    //  load all statistics

    m_cbTotalPhysicalMemoryEvictedLast = TotalPhysicalMemoryEvicted();
    m_cbTotalResourcesEvictedLast = TotalResourcesEvicted();
    
    m_icbOldest = 0;

    InitArray_( m_rgcbTotalPhysicalMemory, m_cAvgDepth, TotalPhysicalMemory() );
    InitArray_( m_rgcbAvailablePhysicalMemory, m_cAvgDepth, AvailablePhysicalMemory() );
    InitArray_( m_rgdcbEvictedPhysicalMemory, m_cAvgDepth, (size_t)0 );

    InitArray_( m_rgcbTotalResources, m_cAvgDepth, TotalResources() );
    InitArray_( m_rgdcbEvictedResources, m_cAvgDepth, (QWORD)0 );

    m_dcbTotalResource = 0.0;
}

//  updates the observed characteristics of the resource pool and the system as
//  a whole for the purpose of making real-time suggestions as to the size of
//  the resource pool.  this function should be called at approximately 1 Hz

template< size_t m_cAvgDepth >
inline void CDBAResourceAllocationManager< m_cAvgDepth >::UpdateStatistics()
{
    //  update all statistics
    
    const size_t cbTotalPhysicalMemoryEvicted = TotalPhysicalMemoryEvicted();
    const QWORD cbTotalResourcesEvicted = TotalResourcesEvicted();

    m_icbOldest = ( m_icbOldest + 1 ) % m_cAvgDepth;

    m_rgcbTotalPhysicalMemory[ m_icbOldest ] = TotalPhysicalMemory();
    m_rgcbAvailablePhysicalMemory[ m_icbOldest ] = AvailablePhysicalMemory();
    m_rgdcbEvictedPhysicalMemory[ m_icbOldest ] = cbTotalPhysicalMemoryEvicted - m_cbTotalPhysicalMemoryEvictedLast;
    m_cbTotalPhysicalMemoryEvictedLast = cbTotalPhysicalMemoryEvicted;
    
    m_rgcbTotalResources[ m_icbOldest ] = TotalResources();
    m_rgdcbEvictedResources[ m_icbOldest ] = cbTotalResourcesEvicted - m_cbTotalResourcesEvictedLast;
    m_cbTotalResourcesEvictedLast = cbTotalResourcesEvicted;

    const size_t cbTotalPhysicalMemory = ComputeArrayAvg_( m_rgcbTotalPhysicalMemory, m_cAvgDepth );
    const size_t cbAvailablePhysicalMemory = ComputeArrayAvg_( m_rgcbAvailablePhysicalMemory, m_cAvgDepth );
    const size_t dcbPhysicalMemoryEvicted = ComputeArrayAvg_( m_rgdcbEvictedPhysicalMemory, m_cAvgDepth );

    const QWORD cbTotalResources = ComputeArrayAvg_( m_rgcbTotalResources, m_cAvgDepth );
    const QWORD dcbResourcesEvicted = ComputeArrayAvg_( m_rgdcbEvictedResources, m_cAvgDepth );

    //  compute the change in the amount of memory that this resource pool
    //  should have

    //  our goal is to equalize the internal memory pressure in our resource
    //  pool with the external memory pressure from the rest of the system.  we
    //  do this using the following differential equation:
    //
    //    dRM/dt = k1 * AM/TM * dRE/dt - k2 * RM/TM * dPE/dt
    //
    //      RM = Total Resource Memory
    //      k1 = fudge factor 1 (1.0 by default)
    //      AM = Available System Memory
    //      TM = Total System Memory
    //      RE = Resource Evictions
    //      k2 = fudge factor 2 (1.0 by default)
    //      PE = System Page Evictions
    //
    //  this equation has two parts:
    //
    //    the first half tries to grow the resource pool proportional to the
    //    internal resource pool memory pressure and to the amount of memory
    //    available in the system
    //
    //    the second half tries to shrink the resource pool proportional to the
    //    external resource pool memory pressure and to the amount of resource
    //    memory we own
    //
    //  in other words, the more available memory and the faster resources are
    //  evicted, the faster the pool grows.  the larger the pool size and the
    //  faster memory pages are evicted, the faster the pool shrinks
    //
    //  the method behind the madness is an approximation of page victimization
    //  by the OS under memory pressure.  the more memory we have then the more
    //  likely it is that one of our pages will be evicted.  to pre-emptively
    //  avoid this, we'll voluntarily reduce our memory usage when we determine
    //  that memory pressure will cause our pages to get evicted.  note that
    //  the reverse of this is also true.  if we make it clear to the OS that
    //  we need this memory by slowly growing while under memory pressure, we
    //  force pages of memory owned by others to be victimized to be used by
    //  our pool.  the more memory any one of those others owns, the more
    //  likely it is that one of their pages will be stolen.  if any of the
    //  others is another resource allocation manager then they can communicate
    //  their memory needs indirectly via the memory pressure they each apply
    //  on the system.  the net result is an equilibrium where each pool gets
    //  the memory it "deserves"

    const double    k1                  = 1.0;
    const double    cbAvailMemory       = (double)cbAvailablePhysicalMemory;
    const double    dcbResourceEvict    = (double)dcbResourcesEvicted;
    const double    k2                  = 1.0;
    const double    cbResourcePool      = (double)cbTotalResources;
    const double    dcbMemoryEvict      = (double)dcbPhysicalMemoryEvicted;
    const double    cbTotalMemory       = (double)cbTotalPhysicalMemory;

    //  compute the optimal amount of memory that this resource pool should grow or shrink by

    const double dcbTotalResource       =   (
                                                k1 * cbAvailMemory * dcbResourceEvict -
                                                k2 * cbResourcePool * dcbMemoryEvict
                                            ) / cbTotalMemory;

    //  limit the rate that the resource pool size changes

    const double dcbTotalResourceMax = cbTotalMemory * (double)m_dpctResourceMax / 100.0;

    RESMGRAssert( dcbTotalResourceMax > 0.0 );

    if ( dcbTotalResource < 0.0 )
    {
        //  cache size reduction

        m_dcbTotalResource += max( dcbTotalResource, -dcbTotalResourceMax );
    }
    else
    {
        //  cache size increase (or same size)

        m_dcbTotalResource += min( dcbTotalResource, dcbTotalResourceMax );
    }
}

//  Retrieves the amount the resource pool should be adjusted by.  The returned 
//  value is a double, so may be positive (for pool growth), or negative (for pool
//  shrinkage).  Once called, the value is removed, so the caller must consume 
//  and materialize the value in the new pool size, or the adjustment will be 
//  lost. Subsequent calls will return 0, until UpdateStatistics() is called 
//  again.
//  Note that this is not thread-safe, so callers need to make sure that only
//  one thread will be calling into this function at any given time.

template< size_t m_cAvgDepth >
inline void CDBAResourceAllocationManager< m_cAvgDepth >::ConsumeResourceAdjustments( __out double * const pdcbTotalResource, __in const double cbResourceSize )
{
    const __int64 dcbTotalResource = (__int64)m_dcbTotalResource;
    *pdcbTotalResource = (double)rounddn( dcbTotalResource, (__int64)cbResourceSize );
    m_dcbTotalResource -= *pdcbTotalResource;
}


//////////////////////////////////////////////////////////////////////////////////////////
//  CLRUKResourceUtilityManager
//
//  Implements a class used to manage a resource via the LRUK Replacement Policy.  Each
//  resource can be cached, touched, and evicted.  The current pool of resources can
//  also be scanned in order by ascending utility to allow eviction of less useful
//  resouces by a clean procedure.
//
//  m_Kmax          = the maximum K-ness of the LRUK Replacement Policy (the actual
//                    K-ness can be set at init time)
//  CResource       = class representing the managed resource
//  OffsetOfIC      = inline function returning the offset of the CInvasiveContext
//                    contained in each CResource
//  CKey            = class representing the key which uniquely identifies each
//                    instance of a CResource.  CKey must implement a default ctor,
//                    operator=(), and operator==()
//
//  You must use the DECLARE_LRUK_RESOURCE_UTILITY_MANAGER macro to declare this class.
//
//  You must implement the following inline function and pass its hashing characteristics
//  into ErrInit():
//
//      int CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::CHistoryTable::CKeyEntry::Hash( const CKey& key );

//  CONSIDER:  add code to special case LRU

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
class CLRUKResourceUtilityManager
{
    public:

        //  timestamp used for resource touch times

        typedef DWORD TICK;

        enum { tickUnindexed        = ~TICK( 0 ) }; //  tick value used to indicate RES is not indexed

        enum { ftickResourceLocked  = 0x1 };        //  bit to indicate that resource ticks and related variables are locked (namely: m_tickLast, m_rgtick[], m_tickIndexTarget and m_pctCachePriority)
        enum { tickTickLastMask     = ~TICK( 0 ) ^ TICK( ftickResourceLocked ) };

        //  normally we would give a flag for "super-cold", but I want a normal touch to be
        //  numerically higher than a super-cold TICK ... so I make normal touch have the
        //  flag and straighten it out with accessors on the IC.
        //  this flag is only applicable to m_tickIndex and m_tickIndexTarget.
        enum { ftickResourceNormalTouch = 0x1 };        //  bit to indicate that the resource is touched normally (i.e., not supercolded).
        enum { tickTickIndexMask        = ~TICK( 0 ) ^ TICK( ftickResourceNormalTouch ) };

        static LONG _CmpTick( const TICK tick1, const TICK tick2 );
        static LONG _DtickDelta( const TICK tickEarlier, const TICK tickLater );
        static WORD _AdjustCachePriority( const ULONG_PTR pctCachePriorityExternal );
        static TICK _TickSanitizeTick( const TICK tick );
        static TICK _TickSanitizeTickForTickLast( TICK tick );
        static TICK _TickLastTouchTime( const TICK tick );
        static BOOL _FTickLocked( const TICK tick );
        static TICK _TickSanitizeTickForTickIndex( TICK tick );
        static TICK _TickIndexTime( const TICK tick );
        static BOOL _FTickSuperColded( const TICK tick );

        //  class containing context needed per CResource

        class CInvasiveContext
        {
            public:

                CInvasiveContext() {}
                ~CInvasiveContext() {}

                static SIZE_T OffsetOfILE() { return OffsetOfIC() + OffsetOf( CInvasiveContext, m_aiic ); }
                static SIZE_T OffsetOfAIIC() { return OffsetOfIC() + OffsetOf( CInvasiveContext, m_aiic ); }

                TICK TickKthTouch( const INT k ) const          { RESMGRAssert( k >= 1 ); return m_rgtick[ k - 1 ]; }
                TICK TickIndexTarget() const                    { return m_tickIndexTarget; }   //  gets the raw m_tickIndexTarget
                TICK TickLastTouchTime() const                  { return _TickLastTouchTime( m_tickLast ); }    //  extracts the time part of m_tickLast (i.e., lock bit masked off)
                TICK TickIndexTargetTime() const                { return _TickIndexTime( m_tickIndexTarget ); } //  extracts the "normal" time part of m_tickIndexTarget (i.e., supercold/normal bit masked off)
                BOOL FCorrelatedTouch() const                   { return TickLastTouchTime() != _TickSanitizeTickForTickLast( m_rgtick[0] ); }
                BOOL FSuperColded() const                       { return _FTickSuperColded( m_tickIndexTarget ); }
                BOOL FResourceLocked() const                    { return _FTickLocked( m_tickLast ); }

#ifdef DEBUGGER_EXTENSION
                TICK TickLastTouch() const                      { return m_tickLast; }
                TICK TickIndex() const                          { return m_tickIndex; }
                TICK TickIndexTime() const                      { return _TickIndexTime( m_tickIndex ); }   //  extracts the "normal" time part of m_tickIndex (i.e., supercold/normal bit masked off)
                BOOL FSuperColdedIndex() const                  { return _FTickSuperColded( m_tickIndex ); }
                WORD PctCachePriority() const                   { return m_pctCachePriority; }
#endif

#ifdef MINIMAL_FUNCTIONALITY    // required for BF perfmon counters
            private:    // avoids undue dependence on this function
#endif
                // SOMEONE said that to generalize k = # of distinct values in m_rgtick ...
                ULONG kLrukPool() const
                {
                    return _kLrukPool( m_rgtick );
                }

            private:

                friend class CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >;

                typename CApproximateIndex< TICK, CResource, OffsetOfAIIC >::CInvasiveContext   m_aiic;
                TICK                                                                            m_tickIndex;        //  Actual index of the resource in the LRU-K approximate index (cache priority scaled).
                TICK                                                                            m_tickIndexTarget;  //  Target tick to be used for indexing in the LRU-K approximate index (cache priority scaled).
                TICK                                                                            m_tickLast;         //  Last time the resource was requested (unscaled).
                TICK                                                                            m_rgtick[ m_Kmax ]; //  History of resource request timestamps (unscaled).
                WORD                                                                            m_pctCachePriority; //  Priority with which the resource is being handled.
        };

        //  API Error Codes

        enum class ERR
        {
            errSuccess,
            errInvalidParameter,
            errOutOfMemory,
            errResourceNotCached,
            errNoCurrentResource,
        };

        //  API Lock Context

        class CLock;

    public:

        //  ctor / dtor

        CLRUKResourceUtilityManager( const INT Rank );
        ~CLRUKResourceUtilityManager();

        //  API

        void SetTimeBar( const TICK dtickHighBar, const TICK dtickLifetimeBar );
#ifdef DEBUG
        void SetFaultInjection( DWORD grbit );
#endif
        ERR ErrInit(    const INT       K,
                        const double    csecCorrelatedTouch,
                        const double    csecTimeout,
                        const double    csecUncertainty,
                        const double    dblHashLoadFactor,
                        const double    dblHashUniformity,
                        const double    dblSpeedSizeTradeoff );
        void Term();

        enum ResMgrTouchFlags
        {
            kNoTouch = 0,
            k1Touch = 1,
            k2Touch = 2,
            k1CorrelatedTouch = 3,
            kMax = 4
        };

        // Note: These externalized APIs must be passed an externalized tick count as it is immediately adjusted, so the default value must be TickRESMGRTimeCurrent().
        ERR ErrCacheResource( const CKey& key, CResource* const pres, __in TICK tickNowExternal, const ULONG_PTR pctCachePriorityExternal, const BOOL fUseHistory = fTrue, __out_opt BOOL * pfInHistory = NULL, const CResource* const presHistoryProvided = NULL );
        ResMgrTouchFlags RmtfTouchResource( CResource* const pres, const ULONG_PTR pctCachePriorityExternal, const TICK tickNowExternal = TickRESMGRTimeCurrent() );
        void PuntResource( CResource* const pres, const TICK dtick );
        BOOL FRecentlyTouched( CResource* const pres, const TICK dtickRecent );
        BOOL FSuperHotResource( CResource* const pres );
        void MarkAsSuperCold( CResource* const pres );
        ERR ErrEvictResource( const CKey& key, CResource* const pres, const BOOL fKeepHistory = fTrue );

        void LockResourceForEvict( CResource* const pres, CLock* const plock );
        void UnlockResourceForEvict( CLock* const plock );

        void BeginResourceScan( CLock* const plock );
        ERR ErrGetCurrentResource( CLock* const plock, CResource** const ppres );
        ERR ErrGetNextResource( CLock* const plock, CResource** const ppres );
        ERR ErrEvictCurrentResource( CLock* const plock, const CKey& key, const BOOL fKeepHistory = fTrue );
        void EndResourceScan( CLock* const plock );

        DWORD CHistoryRecord();
        DWORD CHistoryHit();
        DWORD CHistoryRequest();

        DWORD CResourceScanned();
        DWORD CResourceScannedMoves();
        DWORD CResourceScannedOutOfOrder();

        LONG DtickScanFirstEvictedIndexTarget() const       { return LONG( _TickCurrentTime() - m_tickScanFirstEvictedIndexTarget ); }
        LONG DtickScanFirstEvictedIndexTargetHW() const     { return LONG( _TickCurrentTime() - m_tickScanFirstEvictedIndexTargetHW ); }
        LONG DtickScanFirstFoundNormal() const              { return LONG( _TickCurrentTime() - m_tickScanFirstFoundNormal ); }
        LONG DtickScanFirstEvictedIndexTargetVar() const    { return LONG( m_tickScanFirstEvictedIndexTargetHW - m_tickScanFirstEvictedIndexTarget ); }
        LONG DtickScanFirstEvictedTouchK1() const   { return LONG( _TickCurrentTime() - m_tickScanFirstEvictedTouchK1 ); }
        LONG DtickScanFirstEvictedTouchK2() const   { return LONG( _TickCurrentTime() - m_tickScanFirstEvictedTouchK2 ); }

        LONG DtickFoundToEvictDelta() const         { return LONG( m_tickScanFirstEvictedIndexTarget - m_tickScanFirstFoundAll ); }

        LONG CLastScanEnumeratedEntries() const     { return m_cLastScanEnumeratedEntries; }
        LONG CLastScanBucketsScanned() const        { return m_cLastScanBucketsScanned; }
        LONG CLastScanEmptyBucketsScanned() const   { return m_cLastScanEmptyBucketsScanned; }
        LONG CLastScanEnumeratedIDRange() const     { return m_cLastScanEnumeratedIDRange; }
        LONG DtickLastScanEnumeratedRange() const   { return m_dtickLastScanEnumeratedRange; }

        LONG CSuperColdedResources() const          { return m_cSuperCold; }

        LONG CSuperColdAttempts() const             { return (LONG)m_statsFastEvict.cMarkForEvictionAttempted; }
        LONG CSuperColdSuccesses() const            { return (LONG)m_statsFastEvict.cMarkForEvictionSucceeded; }

#ifdef DEBUGGER_EXTENSION
        VOID    Dump( CPRINTF * pcprintf, const DWORD_PTR dwOffset = 0 ) const;
#endif

    public:

        //  class containing the history of an evicted CResource instance

        class CHistory
        {
                public:

                CHistory() {}
                ~CHistory() {}

#ifdef CRESMGR_H_INCLUDED
//  this seems sucky but I don't know how to do this in a more elegant fashion
#pragma push_macro( "new" )
#undef new
            private:
                void* operator new[]( size_t );         //  not supported
                void operator delete[]( void* );        //  not supported
            public:
                void* operator new( size_t cbAlloc )
                {
                    RESMGRAssert( cbAlloc == sizeof( BFLRUK::CHistory ) ); // this is the size that RESLRUKHIST expects
                    return RESLRUKHIST.PvRESAlloc_( SzNewFile(), UlNewLine() );
                }
                void operator delete( void* pv )
                {
                    RESLRUKHIST.Free( pv );
                }
#pragma pop_macro( "new" )
#endif  //  CRESMGR_H_INCLUDED

                static SIZE_T OffsetOfAIIC() { return OffsetOf( CHistory, m_aiic ); }

            public:

                typename CApproximateIndex< TICK, CHistory, OffsetOfAIIC >::CInvasiveContext    m_aiic;
                CKey                                                                            m_key;
                TICK                                                                            m_tickLast;
                TICK                                                                            m_rgtick[ m_Kmax ];
                WORD                                                                            m_pctCachePriority;
        };

    private:

        //  Non-Obvious (and generally painful) things about CResourceLRUK
        //
        //  1) First when you lock a pointer at TICK = x for a given resource = p, the approximate index 
        //      is 4-way hashed on p, so you can succeed to insert for x for p1, and then fail for the same
        //      value of x w/ p2 (as out of range).
        //  2) While the uncertainty is listed as 1000 ms (from JET param) / ticks, in fact the App Idx 
        //      underneath I believe pushes it up to 1024 (power of 2), so jumping an uncertainty does not 
        //      move you necessarily to the next bucket.
        //  3) There are ~256,000 tick holes in the index in DEBUG ... once the index approaches its max
        //      range, you will get a gap that is = LNextPowerOf2( m_dtickLRUKPrecision ) / 2; in the entries
        //      that you can insert (any insert in this range will return errKeyRangeExceeded)
        //  ?? Anything else hairy ??

        //  index over CResources by LRUK

        typedef CApproximateIndex< TICK, CResource, CInvasiveContext::OffsetOfAIIC > CResourceLRUK;
        typedef typename CResourceLRUK::ERR ERR_RES_APPIDX;

        //  index over history by LRUK

        typedef CApproximateIndex< TICK, CHistory, CHistory::OffsetOfAIIC > CHistoryLRUK;

        //  list of resources that need to have their positions updated in the
        //  resource LRUK

        typedef CInvasiveList< CResource, CInvasiveContext::OffsetOfILE > CUpdateList;

        //  list of resources that couldn't be placed in their correct positions
        //  in the resource LRUK

        typedef CInvasiveList< CResource, CInvasiveContext::OffsetOfILE > CStuckList;

    public:

        //  API Lock Context

        class CLock
        {
            public:

                CLock() :
                    m_tickIndexCurrent( (TICK)tickUnindexed ),
                    m_fHasLrukLock( fFalse )
                {
                    memset( &m_icCurrentBI, 0, sizeof(m_icCurrentBI) );
#ifdef DEBUG
                    m_picCheckBI = NULL;
#endif
                    memset( &m_stats, 0, sizeof(m_stats) );
                }
                ~CLock() {}

                class CLockStats
                {
                    public:
#ifdef RESSCAN_STATISTICS
                        TICK                m_tickBeginResScan;

                        DWORD               m_cEntriesScanned;
                        DWORD               m_cUpdateListIndexChangeBreak;

                        DWORD               m_cEntriesFound;
                        DWORD               m_cEntriesFoundOutOfOrder;
#endif  // RESSCAN_STATISTICS

                        //  these next statistics are required for correctness

                        TICK                m_tickScanFirstFoundAll;        //  lowest m_tickIndex for any returned resource (evicted or not), always normalized as normal-touch
                        TICK                m_tickScanFirstFoundNormal;     //  lowest m_tickIndex for any normal/non-supercolded resource (evicted or not), always normalized as normal-touch
                        TICK                m_tickScanLastFound;            //  highest m_tickIndex for any returned resource (evicted or not), always normalized as normal-touch

                        friend class CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >;

                        //  updates the tick tracking variables for a resource that has been found to
                        //  be worthy of returning to the buffer manager for eviction.

                        VOID UpdateFoundTimes( CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey > * const presmgr, const CInvasiveContext * const pic )
                        {
                            const TICK tickNow = _TickCurrentTime();


                            // NOTE: run on a plock-local copy of real RES IC, snapped with resource lock, but copy we have should never be locked.
                            RESMGRAssert( !pic->FResourceLocked() );

                            //  Consider adding a _SanityCheckTick( x ) for each of these elements as well.
                            const TICK tickFoundUpdate = pic->TickIndexTargetTime();
                            const BOOL fSuperColded = pic->FSuperColded();

                            //  this is only really needed b/c we are not using absolute ticks for tickIndex b/c otherwise
                            //  we could just being using the tickIndex directly.
                            //  we have a res that is mis-indexed by a fair margin, this can happen because when 
                            //  we goto mark it for fast eviction (supercold) the LRUK buckets are locked.
                            //  we have also seen cases where this is misplaced in stress tests that touch resources with
                            //  random cache priorities, ranging from 0 up to 1000 so we've added protection to not let
                            //  m_tickIndexTarget go backwards.
                            //  yet a third case is when we hit conflicts inserting the resource into the approximate index
                            //  and we end up with tickYoungest.

                            const BOOL fOutOfPlace = presmgr->m_ResourceLRUK.CmpKey( pic->m_tickIndexTarget, pic->m_tickIndex - presmgr->m_dtickUncertainty * 2 ) < 0;

                            if ( fOutOfPlace )
                            {
                                return;
                            }

                            if ( ( m_tickScanFirstFoundAll == 0 ) || ( _CmpTick( tickFoundUpdate, m_tickScanFirstFoundAll ) < 0 ) )
                            {
                                //  this can walk back a little bit, but only within the granularity of the bucket(hopefully)
                                presmgr->_ValidateTickProgress( L"LOCK-SFFA", m_tickScanFirstFoundAll, tickFoundUpdate, vtpSuperColdBackwardCheck );
                                //  early warning against a bad update in scan end.
                                presmgr->_ValidateTickProgress( L"GLOB-SFFA", presmgr->m_tickScanFirstFoundAll, tickFoundUpdate, vtpSuperColdBackwardCheck );

                                m_tickScanFirstFoundAll = tickFoundUpdate;
                            }

                            if ( !fSuperColded )
                            {
                                if ( ( m_tickScanFirstFoundNormal == 0 ) || ( _CmpTick( tickFoundUpdate, m_tickScanFirstFoundNormal ) < 0 ) )
                                {
                                    //  OUCH!  Hit in accept forever.  Due to appx idx wrap this has to be commented out.
                                    //      We went from 450 secs (420 secs in future, thx to cache priority) all the way
                                    //      back to 26 secs.

                                    m_tickScanFirstFoundNormal = tickFoundUpdate;
                                    presmgr->m_tickLatestScanFirstFoundNormalUpdate = tickNow;
                                }
                            }

                            if ( ( m_tickScanLastFound == 0 ) || ( _CmpTick( tickFoundUpdate, m_tickScanLastFound ) > 0 ) )
                            {
                                m_tickScanLastFound = tickFoundUpdate;
                            }

                            RESMGRAssert( !( ( m_tickScanLastFound == 0 ) && ( m_tickScanFirstFoundAll != 0 ) ) );
                            RESMGRAssert( !( ( m_tickScanLastFound == 0 ) && ( m_tickScanFirstFoundNormal != 0 ) ) );
                            RESMGRAssert( !( ( m_tickScanFirstFoundAll == 0 ) && ( m_tickScanFirstFoundNormal != 0 ) ) );
                            RESMGRAssert( ( m_tickScanFirstFoundAll == 0 ) || ( _CmpTick( m_tickScanFirstFoundAll, m_tickScanLastFound ) <= 0 ) );
                            RESMGRAssert( ( m_tickScanFirstFoundNormal == 0 ) || ( _CmpTick( m_tickScanFirstFoundNormal, m_tickScanLastFound ) <= 0 ) );
                            RESMGRAssert( ( m_tickScanFirstFoundNormal == 0 ) || ( _CmpTick( m_tickScanFirstFoundAll, m_tickScanFirstFoundNormal ) <= 0 ) );
                        }

#ifdef RESSCAN_STATISTICS
                        DWORD               m_cEntriesEvicted;
#endif  // RESSCAN_STATISTICS

                        TICK                m_tickScanFirstEvictedTouchK1;      //  lowest K1 touch time of any evicted normal/non-supercolded resource
                        TICK                m_tickScanFirstEvictedTouchK2;      //  lowest K2 touch time of any evicted normal/non-supercolded resource
                        TICK                m_tickScanFirstEvictedIndexTarget;  //  lowest index-target of any evicted normal/non-supercolded resource

                        //  updates the tick tracking variables for a resource that is being evicted
                        //  from the resmgr by the buffer manager.

                        VOID UpdateEvictTimes( CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey > * const presmgr, const CInvasiveContext * const pic )
                        {

                            const BOOL fSuperColded = pic->FSuperColded();
                            const BOOL fOutOfPlace = presmgr->m_ResourceLRUK.CmpKey( pic->m_tickIndexTarget, pic->m_tickIndex - presmgr->m_dtickUncertainty * 2 ) < 0;

                            //  if the resource is supercolded or out of place, bail
                            if ( fSuperColded || fOutOfPlace )
                            {
                                return;
                            }

                            if ( pic->kLrukPool() == 1 || presmgr->m_K == 1 /* just to handle bloody unit test */ )
                            {
                                const TICK tickScanLastEvictedTouchK1 = pic->TickKthTouch( 1 );
                                if ( ( m_tickScanFirstEvictedTouchK1 == 0 ) || ( _CmpTick( tickScanLastEvictedTouchK1, m_tickScanFirstEvictedTouchK1 ) < 0 ) )
                                {
                                    m_tickScanFirstEvictedTouchK1 = tickScanLastEvictedTouchK1;
                                }
                            }
                            else if ( pic->kLrukPool() == presmgr->m_K )
                            {
                                const TICK tickScanLastEvictedTouchK2New = pic->TickKthTouch( presmgr->m_K ) - presmgr->m_ctickTimeout * ( presmgr->m_K - 1 );
                                if ( ( m_tickScanFirstEvictedTouchK2 == 0 ) || ( _CmpTick( tickScanLastEvictedTouchK2New, m_tickScanFirstEvictedTouchK2 ) < 0 ) )
                                {
                                    m_tickScanFirstEvictedTouchK2 = tickScanLastEvictedTouchK2New;
                                }
                            }
                            else
                            {
                                RESMGRAssert( fFalse ); //  not k = 1 or k = 2 ???
                            }

                            const TICK tickScanFirstEvictedIndexTargetNew = pic->TickIndexTargetTime();

                            if ( ( m_tickScanFirstEvictedIndexTarget == 0 ) || ( _CmpTick( tickScanFirstEvictedIndexTargetNew, m_tickScanFirstEvictedIndexTarget ) < 0 ) )
                            {
                                m_tickScanFirstEvictedIndexTarget = tickScanFirstEvictedIndexTargetNew;
                                presmgr->m_tickLatestScanFirstEvictedIndexTargetUpdate = _TickCurrentTime();
                            }
                        }

#ifdef RESSCAN_STATISTICS
                        DWORD               m_cMovedToUpdateList;
                        DWORD               m_cMovedOutOfUpdateList;
                        DWORD               m_cMovedToStuckList;
                        DWORD               m_cStuckListReturned;
#endif
                };

            private:

                friend class CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >;

                typename CResourceLRUK::CLock   m_lock;
                BOOL                            m_fHasLrukLock;

                TICK                    m_tickIndexCurrent;
                CInvasiveContext        m_icCurrentBI;
#ifdef DEBUG
                const CInvasiveContext* m_picCheckBI;
#endif

                CUpdateList             m_UpdateList;

                CStuckList              m_StuckList;
                CResource*              m_presStuckList;
                CResource*              m_presStuckListNext;

                CLockStats              m_stats;
        };

        typedef typename CLock::CLockStats  CResScanLockStats;

        //  entry used in the hash table used to map CKeys to CHistorys

        class CHistoryEntry
        {
            public:

                CHistoryEntry() {}
                ~CHistoryEntry() {}

                CHistoryEntry& operator=( const CHistoryEntry& he )
                {
                    m_KeySignature  = he.m_KeySignature;
                    m_phist         = he.m_phist;
                    return *this;
                }

            public:

                ULONG_PTR   m_KeySignature;
                CHistory*   m_phist;
        };

    private:

        //  table that maps CKeys to CHistoryEntrys

        typedef CDynamicHashTable< CKey, CHistoryEntry > CHistoryTable;

    private:

        BOOL _FResourceStaleForKeepingHistory( const TICK* const rgtick, const TICK tickLast, const WORD pctCachePriority, const TICK tickNow );
        ResMgrTouchFlags _RmtfTouchResource( CInvasiveContext* const pic, const WORD pctCachePriority, const TICK tickNow );
        ERR_RES_APPIDX _ErrInsertResource( CLock * const plock, CResource * const pres, const TICK * ptickReserved );
        void _MarkForFastEviction ( CInvasiveContext* const pic );
        void _RestoreHistory( const CKey& key, CInvasiveContext* const pic, const TICK tickNow, const WORD pctCachePriority, __out_opt BOOL * pfInHistory );
        void _CopyHistory( const CKey& key, CInvasiveContext* const picDst, const CInvasiveContext* const picSrc );
        void _StoreHistory( const CKey& key, CInvasiveContext* const pic );
        CHistory* _PhistAllocHistory( const CKey& key );
        BOOL _FInsertHistory( CHistory* const phist );

        CResource* _PresFromPic( CInvasiveContext* const pic ) const;
        CInvasiveContext* _PicFromPres( CResource* const pres ) const;
        static ULONG _kLrukPool( const TICK* const rgtick );
        static TICK _TickCurrentTime();
        TICK _TickOldestEstimate();
        TICK _ScaleTick( const TICK tickToScale, const WORD pctCachePriority );
        void _AdjustIndexTargetForMinimumLifetime( CInvasiveContext* const pic );
        const static TICK m_dtickSuperColdOffsetConcurrentMax = 5000;

        typedef enum
        {
            vtcNone                 = 0x0,
            vtcAllowFarForward      = 0x1,
            vtcAllowIndexKey        = 0x2,

            vtcDefault              = vtcNone,
        } ValidateTickCheck;

        typedef enum
        {
            vtpNone                     = 0,
#ifdef DEBUG
            vtpDisableAssert            = 0x1,
#endif
            vtpSmoothForward            = 0x2,  //  tracing only
            vtpBucketBackwardCheck      = 0x4,  //  allows for jumps backwards w/in 2 bucket granularities
            vtpConcurrentBackwardCheck  = 0x8,  //  allows for jumps backwards based upon concurrency / bucketing (3 x bucket) ...
            vtpSuperColdBackwardCheck   = 0x10, //  allows for jumps backwards based upon super colde + concurrency + bucketizing (~5 or 5x bucket)...
            vtpBucketForwardCheck       = 0x20, //  allows for jumps forwards w/in 2 bucket granularities

            vtpNoTracing                = 0x80000000,
        } ValidateTickProgress;

#ifdef RTM
        #define _SanityCheckTick( tick )
        #define _SanityCheckTickEx( tick, vtc )
#else   // !RTM
        #define _SanityCheckTick( tick )            _SanityCheckTick_( tick, vtcDefault, #tick )
        #define _SanityCheckTickEx( tick, vtc )     _SanityCheckTick_( tick, vtc, #tick )
        void _SanityCheckTick_( const TICK tick, const ValidateTickCheck vtc, const CHAR * szTickName ) const;
#endif  // RTM
        //  non-const b/c m_cTraces is modified by tracing
        void _ValidateTickProgress( __in_z const WCHAR * szStateVarName, const TICK tickStoredState, const TICK tickNew, const ValidateTickProgress vtp );

        //  problems including math.hxx everywhere ...
        template<class T>
        inline BOOL _FPowerOf2( T x ) const
        {
            return ( ( 0 < x ) && ( 0 == ( x & ( x - 1 ) ) ) );
        }
        inline LONG _LNextPowerOf2( LONG l ) const
        {
            LONG i = 0;
        
            if ( !_FPowerOf2( lMax / 2 + 1 ) || ( l > lMax / 2 + 1 ) )
            {
                return -1;
            }
            
            for ( i = 1; i < l; i += i )
            {
            }
        
            return i;
        }
        

    private:

#ifndef RTM
        enum RESMGRTraceTag     //  rmtt
        {
            rmttNone                        = 0x0,
#ifdef DEBUG
            rmttYes                         = 0x1,  //  only for debugging
#endif
            rmttInitTerm                    = 0x2,
            rmttScanStats                   = 0x4,
            rmttScanProcessing              = 0x8,
            rmttTickUpdateJumps             = 0x10,     //  should be small, but isn't
            rmttFastEvictContentions        = 0x20,     //  error conditions
            rmttHistoryRestore              = 0x40,     //  some ...
            rmttScaleTickAdjustments        = 0x80,     //  very verbose
            rmttResourceInsertStall         = 0x100,
        };
        const static ULONG m_cTraceAllOn    = 0xffffffff;

        #define RESMGRTrace( rmtt, ... )        \
            if ( ( m_rmttControl & rmtt && m_cTracesTTL == m_cTraceAllOn ) ||   \
                    ( m_rmttControl & rmtt && m_cTracesTTL )                    \
                    OnDebug( || ( rmtt & rmttYes /* hard coded on */ ) ) )      \
            {                                   \
                wprintf( __VA_ARGS__ );         \
                m_cTracesTTL--;                 \
            }
#else
        #define RESMGRTrace( rmtt, ... )
#endif  //  !RTM

    private:

        struct CFastEvictStatistics
        {
            __int64     cMarkForEvictionAttempted;
            __int64     cMarkForEvictionSucceeded;
#ifdef FASTEVICT_STATISTICS
            INT         cFailureToMarkForEvictionBecauseFailedTouchLock;
            INT         cFailureToMarkForEvictionBecauseItHasNoBucket;
            INT         cFailureToMarkForEvictionBecauseOfLockFailure;
            INT         cFailureToMarkForEvictionBecauseOfLockFailureOnDestination;
            INT         cFailureToMarkForEvictionBecauseItMoved;
            INT         cFailuseToInsertIntoTargetBucket;
            INT         cSuccessAlreadyInRightBucket;
            INT         cAlreadyInTargetTick;
            INT         cLockedSecondOldest;
#endif
        };

        //  never updated

        BOOL            m_fInitialized;
        DWORD           m_K;
        TICK            m_ctickCorrelatedTouch;
        TICK            m_ctickTimeout;
        TICK            m_dtickUncertainty;
        TICK            m_dtickLRUKPrecision;
        TICK            m_dtickSuperColdOffset;

        TICK            m_tickStart;            // not needed, can remove some day, for now helpful with current debugs
        TICK            m_tickLowBar;
        TICK            m_tickHighBar;
        TICK            m_dtickCacheLifetimeBar;
        BYTE            m_rgbReserved1[ 20 ];

        //  rarely updated

        volatile DWORD  m_cHistoryRecord;
        volatile DWORD  m_cHistoryHit;
        volatile DWORD  m_cHistoryReq;

        volatile DWORD  m_cResourceScanned;
        volatile DWORD  m_cResourceScannedMoves;
        volatile DWORD  m_cResourceScannedOutOfOrder;

        volatile LONG   m_cSuperCold;

#ifdef DEBUG
        enum RESMGRFaultInj     //  rmfi
        {
            rmfiNone                            = 0x0,
            rmfiScanTouchConcurrentFI           = 0x1,
            rmfiFastEvictSourceLockFI           = 0x2,
            rmfiFastEvictDestInitialLockFI      = 0x4,
            rmfiFastEvictDestSrcBucketMatchFI   = 0x8,
            rmfiFastEvictDestSecondaryLockFI    = 0x10,
            rmfiFastEvictIndexTickUnstableFI    = 0x20,
            rmfiFastEvictInsertEntryFailsFI     = 0x40, // most likely b/c of out of range

            rmfiAll                             = 0xFFFFFFFF
        };

        RESMGRFaultInj  m_grbitFI;

        BOOL    _FFaultInj( const RESMGRFaultInj rmfiTarget ) const     { return ( m_grbitFI & rmfiTarget ) && ( rand() % 20 == 3 ); }
#else
        #define _FFaultInj( rmfiTarget )                                fFalse
#endif

#ifndef RTM
        RESMGRTraceTag  m_rmttControl;
        ULONG           m_cTracesTTL;
#endif

        //  Scavenge-based eviction is handled through the resource scan, which looks from
        //  a RESMGR API perspective like this (lines preceeded by [BF] are purely decisions
        //  that are up a layer from us ... and thus can't be known here):
        //              BeginResourceScan( plock )
        //              while( ErrGetNextResource( plock ) == ::errSuccess )
        //      [BF]            if ( BF satisfied with # of RESs evicted so far )
        //      [BF]                break.
        //      [BF]            if ( BF can evict this RES /* not dirty and such */ )
        //                          ErrEvictCurrentResource( plock )
        //              EndResourceScan( plock )
        //
        //  The E14 model of calculating the estimated oldest tick (as the m_tickLast value from the last
        //  successful return of ErrGetNextResource()) had several drawbacks ...
        //      A) The m_tickLast can advance for several seconds beyond its tick1/tick2/tickIndex
        //          values due to the 125 ms correlation interval and being repeatedly touched.
        //      B) The m_tickLast will oscillate by a full m_ctickTimeout / 100,000 when evicting a
        //          k=1 vs. k=2 resource.
        //      C) Changes in the two if clauses in [BF] area could make a whole bunch of RESs non-
        //          evictable for a period, further messing with the last successful GetNext call.
        //          Such as a pinned dirty resource, seen from DBScan's status updates, or in the
        //          past a LLR pinned resource.
        //      D) In DEBUG, our wrapping on the tickIndex causes ALL sorts of nasty rescavenging of
        //          things that are NOT the oldest tick.
        //      E) If a super colded resource wasn't able to updated to the right bucket, it would
        //          be out of place and cause the tickLast touch to swing wildly when that resource
        //          was evicted.
        //      F) Finally, there were two competing needs for computing the estimated oldest tick,
        //          one for super-cold / fast eviction and another for cache priority ... this made
        //          the trade-offs you might want for one, not comensurate with the trade-offs you'd
        //          make for the other.
        //
        //  We implement these tracking variables from a given scan ...
        //      The first/lowest m_tickTargetIndex|Index value for any resource returned
        //      The first/lowest m_tickTargetIndex|Index value for any normal/non-supercolded resource returned
        //      The last/highest m_tickTargetIndex|Index value for any resource returned.
        //
        //      The first/lowest m_rgtick[1-1]/k1 and m_rgtick[m_K-1]/k2 (after subtracing timeout) that was in-order evicted.
        //      The first/lowest m_tickTargetIndex|Index value from an in-order evicted resource.
        //
        //      Note: The lowest values have the 'First' name, to imply they are the first resources
        //      we found in the scan, but are not necessarily from the first successful resource returned 
        //      because things are within a 1024 ms uncertainty bucket, and for these other factors:
        //          m_tickScanFirstFoundAll - Can bounce backwards due to fast evict putting resources,
        //              farther back in time.
        //          Note: Also similarly Last / highest values can jump around due to bucket granularity.
        //
        //  The tick to use for scaling cache priority is m_tickScanFirstEvictedIndexTargetHW.
        //  The tick to use for the basis for supercold is m_tickScanFirstFoundNormal.
        //
        //  Note: We also track when we update these last two, so that 
        TICK            m_tickScanFirstFoundAll;        //  lowest m_tickTargetIndex|Index for any returned resource (evicted or not), always normalized as normal-touch
        TICK            m_tickScanFirstFoundNormal;     //  lowest m_tickTargetIndex|Index for any normal/non-supercolded resource (evicted or not), always normalized as normal-touch (base - few secs used for _MarkForFastEviction / supercold)
        TICK            m_tickScanLastFound;            //  highest m_tickTargetIndex|Index for any returned resource (evicted or not)
        TICK            m_tickLatestScanFirstFoundNormalUpdate;

        TICK            m_tickScanFirstEvictedTouchK1;          //  lowest K1 touch time of any evicted normal/non-supercolded resource, always normalized as normal-touch
        TICK            m_tickScanFirstEvictedTouchK2;          //  lowest K2 touch time of any evicted normal/non-supercolded resource, always normalized as normal-touch
        TICK            m_tickScanFirstEvictedIndexTarget;      //  lowest index-target of any evicted normal/non-supercolded resource, always normalized as normal-touch
        TICK            m_tickScanFirstEvictedIndexTargetHW;    //  highest watermark m_tickScanFirstEvictedIndexTarget across multiple scans, always normalized as normal-touch (base used for _ScaleTick() / cache priority)
        TICK            m_tickScanFirstFoundAllIndexTargetHW;   //  value of m_tickScanFirstFoundAll upon updating m_tickScanFirstEvictedIndexTargetHW
        TICK            m_tickScanFirstFoundNormalIndexTargetHW;//  value of m_tickScanFirstFoundNormal upon updating m_tickScanFirstEvictedIndexTargetHW
        TICK            m_tickLatestScanFirstEvictedIndexTargetUpdate;
        TICK            m_tickLatestScanFirstEvictedIndexTargetHWUpdate;

        //  statistics from the last scan against the m_ResourceLRUK for resources

        LONG            m_cLastScanEnumeratedEntries;
        LONG            m_cLastScanBucketsScanned;
        LONG            m_cLastScanEmptyBucketsScanned;
        LONG            m_cLastScanEnumeratedIDRange;
        TICK            m_dtickLastScanEnumeratedRange;

#ifdef DEBUG
        DWORD           m_tidLockCheck;
#endif

        //  m_HistoryLRUK is a CApproximateIndex sorted by tick and stores history records.
        //  m_KeyHistory is a CDynamicHashTable hashed by a custom key (IFMP,PGNO in BF's case)
        //  and points to a history record.
        //  the contract is that m_HistoryLRUK may have orphaned entries (i.e., history records
        //  that don't belong to any entry in m_KeyHistory), but not the other way around.  this
        //  is acceptable because they will be reused at some point and leftovers will be freed
        //  upon RESMGR termination.
        //  allowing m_KeyHistory to have orphaned entries could cause any external code trying
        //  to retrieve history records to potentially dereference freed history records or use
        //  data pertaining to a different resource.
        //  to maintain this contract, all deletions and insertions from/to m_HistoryLRUK
        //  must be done with both locks held.  because different codepaths may want to lock
        //  either one first, the second lock will always be a try-lock.
        //  functions that read data only need to take one of the locks, since that will prevent
        //  deletions (as those would require both locks).

        CHistoryLRUK    m_HistoryLRUK;
        CHistoryTable   m_KeyHistory;

        //  commonly updated, this is a CApproximateIndex that stores the actual resources.

        CResourceLRUK   m_ResourceLRUK;

        //  statistics of interest

        CResScanLockStats       m_statsScanLast;
        CResScanLockStats       m_statsScanPrev;
        CFastEvictStatistics    m_statsFastEvict;
};

//  ctor

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CLRUKResourceUtilityManager( const INT Rank )
    :   m_fInitialized( fFalse ),
        m_HistoryLRUK( Rank - 2 ),
        m_KeyHistory( Rank - 1 ),
        m_ResourceLRUK( Rank ),
        m_tickLowBar( 0 ),
        m_tickHighBar( 0 ),
        m_dtickCacheLifetimeBar( 0 )
{
}

//  dtor

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
~CLRUKResourceUtilityManager()
{
}

//  sets the cache lifetime high water, and cache lifetime (dynamic range) bars
//  that the we should defend against.

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
SetTimeBar( const TICK dtickLifetimeBar, const TICK dtickHighBar )
{
    m_tickHighBar = dtickHighBar;
    m_dtickCacheLifetimeBar = dtickLifetimeBar;
}

#ifdef DEBUG
//  sets the fault injection options

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
SetFaultInjection( const DWORD grbit )
{
    m_grbitFI = (RESMGRFaultInj)grbit;
}
#endif

//  initializes the LRUK resource utility manager using the given parameters.
//  if the RUM cannot be initialized, errOutOfMemory is returned

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrInit(    const INT       K,
            const double    csecCorrelatedTouch,
            const double    csecTimeout,
            const double    csecUncertainty,
            const double    dblHashLoadFactor,
            const double    dblHashUniformity,
            const double    dblSpeedSizeTradeoff )
{
    //  validate all parameters

    if (    K < 1 || K > m_Kmax ||
            csecCorrelatedTouch < 0 ||
            csecTimeout < 0 ||
            dblHashLoadFactor < 0 ||
            dblHashUniformity < 1.0 )
    {
        return ERR::errInvalidParameter;
    }

    //  init our parameters

    m_K                     = DWORD( K );
    m_ctickCorrelatedTouch  = TICK( csecCorrelatedTouch * 1000.0 );
    m_ctickTimeout          = TICK( csecTimeout * 1000.0 );                     // default: 100,000
    m_dtickUncertainty      = TICK( max( csecUncertainty * 1000.0, 2.0 ) );

    m_dtickSuperColdOffset  = ( _LNextPowerOf2( m_dtickUncertainty ) * 3 + 1 ); // default: ~3000 ticks

    //RESMGRAssert( m_K != 1 );     // expected, broken by resmgremulatorunit

    //  init our counters

    m_cHistoryRecord    = 0;
    m_cHistoryHit       = 0;
    m_cHistoryReq       = 0;

    m_cResourceScanned              = 0;
    m_cResourceScannedMoves         = 0;
    m_cResourceScannedOutOfOrder    = 0;

#ifdef DEBUG
    //  diagnostics

    m_tidLockCheck      = 0;

    //  fault injection

    //  we are using m_ctickCorrelatedTouch < 3 to tell us we're running from resmgremulatorunit.exe and
    //  so we want to avoid fault injection as it can mess up the simulator.

    m_grbitFI           =  rmfiNone;
//  m_grbitFI           =  ( m_ctickCorrelatedTouch < 3 ) ? rmfiNone : rmfiAll;
#endif

    //  how to enable resmgr tracing ...
    //m_rmttControl     = RESMGRTraceTag( rmttInitTerm | rmttTickUpdateJumps | rmttResourceInsertStall );
#ifndef RTM
    m_rmttControl       = rmttNone;
    m_cTracesTTL        = m_cTraceAllOn;
#endif

    m_cSuperCold        = 0;

    const TICK tickStart = _TickCurrentTime();
    m_tickStart = tickStart;

    const TICK tickStartStats = _TickSanitizeTickForTickIndex( tickStart );
    m_tickScanFirstFoundAll = tickStartStats;
    m_tickScanFirstFoundNormal = tickStartStats;
    m_tickScanLastFound = tickStartStats;
    m_tickScanFirstEvictedTouchK1 = tickStartStats;
    m_tickScanFirstEvictedTouchK2 = tickStartStats;
    m_tickScanFirstEvictedIndexTarget = tickStartStats;
    m_tickScanFirstEvictedIndexTargetHW = tickStartStats;
    m_tickScanFirstFoundAllIndexTargetHW = tickStartStats;
    m_tickScanFirstFoundNormalIndexTargetHW = tickStartStats;
    m_tickLatestScanFirstEvictedIndexTargetUpdate = tickStartStats;
    m_tickLatestScanFirstEvictedIndexTargetHWUpdate = tickStartStats;

    RESMGRTrace( rmttInitTerm, L"\tInitial tickStart = %u (0x%x)\n", tickStart, tickStart );

    //  The following instrumentation is commented out because of users that run 
    //  for long periods of time with very low activity. It can be re-enabled for
    //  for testing purposes; so I'm leaving it here.
    //  for this range we choose a large delta to minimize false positives 
    //  Consider adding a test hook to set these *Bar validation variables ... 

    m_tickLowBar = 0;
    //m_tickHighBar will be zero, or set externally
    //m_dtickCacheLifetimeBar will be zero or set externally

#ifdef RESMGR_ENABLE_TICK_VALIDATION
    if ( m_tickHighBar == 0 && m_dtickCacheLifetimeBar == 0 )
    {
        //  if no one has overriden the tick validation parameters, then set some reasonable defaults
        SetTimeBar( 90 * 60 * 1000 /* lifetime = 90 min | 5,400,000 */, 10 * 60 * 60 * 1000 /* highbar "+=" 10 hrs */ );
    }
#endif  //  RESMGR_ENABLE_TICK_VALIDATION
    m_tickLowBar = tickStart - m_dtickSuperColdOffsetConcurrentMax;
    if ( m_tickHighBar )
    {
        m_tickHighBar += tickStart;
        RESMGRTrace( rmttInitTerm, L"\tBar Testing is Enabled: LowBar = %u (0x%x), HighBar = %u (0x%x), LifetimeBar/Range: %u (%u:%02u:%02u.%03u) \n",
                        m_tickLowBar, m_tickLowBar, m_tickHighBar, m_tickHighBar, m_dtickCacheLifetimeBar,
                        m_dtickCacheLifetimeBar / 1000 / 60 / 60, m_dtickCacheLifetimeBar / 1000 / 60 % 60, m_dtickCacheLifetimeBar / 1000 % 60, m_dtickCacheLifetimeBar % 1000 );
    }


    //  init our indexes
    //
    //  NOTE:  the precision for the history LRUK is intentionally low so that
    //  we are forced to purge history records older than twice the timeout
    //  in order to insert new history records.  we would normally only purge
    //  history records on demand which could allow a sudden burst of activity
    //  to permanently allocate an abnormally high number of history records

    if ( m_HistoryLRUK.ErrInit( 4 * m_K * m_ctickTimeout,
                                m_dtickUncertainty,
                                dblSpeedSizeTradeoff ) != CHistoryLRUK::ERR::errSuccess )
    {
        Term();
        return ERR::errOutOfMemory;
    }

    if ( m_KeyHistory.ErrInit(  dblHashLoadFactor,
                                dblHashUniformity ) != CHistoryTable::ERR::errSuccess )
    {
        Term();
        return ERR::errOutOfMemory;
    }

    //  note: The first artifacts this actually show up at around half this time I believe
#ifdef DEBUG
    m_dtickLRUKPrecision = (DWORD)( _LNextPowerOf2(
                                max( min( 2 * m_K * m_ctickTimeout + 2, //  min value that satisfies ( > 2 * m_K * m_ctickTimeout ) below
                                          INT_MAX / 2 ),                //  don't let it overflow
                                      8 * 60 * 1000 ) ) - 1 );          //  too small is too restrictive if the real cache lifetime is not tiny
#else  //  !DEBUG
#ifdef RTM
    m_dtickLRUKPrecision = ~TICK( 0 );      //  49.7 days
#else  //  !RTM
    m_dtickLRUKPrecision = 0x10000000 - 1;  //  3.107 days
#endif  //  RTM
#endif  //  DEBUG

    RESMGRAssert( m_dtickLRUKPrecision >= 2 * m_K * m_ctickTimeout );
    RESMGRAssert( (DWORD)_LNextPowerOf2( m_dtickLRUKPrecision ) == m_dtickLRUKPrecision + 1 );  //  expected: CApproximateIndex rounds to next power of 2, so make it match real value.

    const TICK dtickBucketUncer = _LNextPowerOf2( m_dtickUncertainty );
    RESMGRAssert( dtickBucketUncer == 1024 || /* unit test */ dtickBucketUncer == 128 );        //  expected: CApproximateIndex rounds to power of 2 anyways, so make it match real value.

    RESMGRTrace( rmttInitTerm, L"\tm_ResourceLRUK.ErrInit( %u, %u, %f )\n",
                    m_dtickLRUKPrecision, m_dtickUncertainty, dblSpeedSizeTradeoff );

    if ( m_ResourceLRUK.ErrInit(    m_dtickLRUKPrecision,
                                    m_dtickUncertainty,
                                    dblSpeedSizeTradeoff ) != CResourceLRUK::ERR::errSuccess )
    {
        Term();
        return ERR::errOutOfMemory;
    }

    memset( &m_statsScanLast, 0, sizeof(m_statsScanLast) );
    memset( &m_statsScanPrev, 0, sizeof(m_statsScanPrev) );
    memset( &m_statsFastEvict, 0, sizeof(m_statsFastEvict) );

    m_fInitialized = fTrue;
    return ERR::errSuccess;
}

//  terminates the LRUK RUM.  this function can be called even if the RUM has
//  never been initialized or is only partially initialized
//
//  NOTE:  any data stored in the RUM at this time will be lost!

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
Term()
{
    //  purge all history records from the history LRUK.  we must do this from
    //  the history LRUK and not the history table as it is possible for the
    //  history table to lose pointers to history records in the history LRUK

    if ( m_fInitialized )
    {
        CHistoryLRUK::CLock lockHLRUK;

        m_HistoryLRUK.MoveBeforeFirst( &lockHLRUK );
        while ( m_HistoryLRUK.ErrMoveNext( &lockHLRUK ) == CHistoryLRUK::ERR::errSuccess )
        {
            CHistory* phist;
            CHistoryLRUK::ERR errHLRUK = m_HistoryLRUK.ErrRetrieveEntry( &lockHLRUK, &phist );
            RESMGRAssert( errHLRUK == CHistoryLRUK::ERR::errSuccess );

            errHLRUK = m_HistoryLRUK.ErrDeleteEntry( &lockHLRUK );
            RESMGRAssert( errHLRUK == CHistoryLRUK::ERR::errSuccess );
            AtomicDecrement( (LONG*)&m_cHistoryRecord );

            delete phist;
        }
        m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );
        RESMGRAssert( m_cHistoryRecord == 0 );
    }

    //  terminate our indexes

    m_ResourceLRUK.Term();
    m_KeyHistory.Term();
    m_HistoryLRUK.Term();

    if ( m_tickHighBar )
    {
        m_tickHighBar -= m_tickStart;
    }

    m_fInitialized = fFalse;
}

//  starts management of the specified resource optionally using any previous
//  knowledge the RUM has about this resource.  the resource must currently be
//  evicted from the RUM.  the resource will be touched by this call, except 
//  if presHistoryProvided is provided.  if we cannot start management of this resource, 
//  errOutOfMemory will be returned

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrCacheResource( const CKey& key, CResource* const pres, __in TICK tickNowExternal, const ULONG_PTR pctCachePriorityExternal, const BOOL fUseHistory, __out_opt BOOL * pfInHistory, const CResource* const presHistoryProvided )
{
    CLock lock;
    CResourceLRUK::ERR errLRUK;
    BOOL fRecoveredFromHistory = fFalse;
    CInvasiveContext* const pic = _PicFromPres( pres );
    const WORD pctCachePriority = _AdjustCachePriority( pctCachePriorityExternal );
    const TICK tickNow = _TickSanitizeTickForTickLast( tickNowExternal );
    _SanityCheckTick( tickNow );

    //  we are supposed to use the history for this resource if available

    if ( presHistoryProvided )
    {
        RESMGRAssert( !fUseHistory );   // one version of history or the other, not both ...

        //  restore / copy the history from the provided resource

        //  note: cast un-const'ing, but ok, as _CopyHistory() is const for picSrc ..
        _CopyHistory( key, pic, _PicFromPres( (CResource* const)presHistoryProvided ) );
        fRecoveredFromHistory = fTrue;
    }
    else if ( fUseHistory )
    {
        RESMGRAssert( !presHistoryProvided );   // one version of history or the other, not both ...

        //  restore the history for this resource

        BOOL fInHistory = fFalse;
        _RestoreHistory( key, pic, tickNow, pctCachePriority, &fInHistory );
        fRecoveredFromHistory = fInHistory;

        if ( pfInHistory != NULL )
        {
            *pfInHistory = fInHistory;
        }
    }

    pic->m_tickIndex = TICK( tickUnindexed );

    const TICK tickScanFirstFoundAll = m_tickScanFirstFoundAllIndexTargetHW;
    const TICK tickScanFirstFoundNormal = m_tickScanFirstFoundNormalIndexTargetHW;
    ResMgrTouchFlags rmtf = kNoTouch;

    //  only initialize ticks if necessary

    if ( !fRecoveredFromHistory )
    {
        //  resource is locked at first
        pic->m_tickLast = _TickSanitizeTickForTickLast( tickNow ) | ftickResourceLocked;
        
        //  initialize this resource to be touched once by setting all of its
        //  touch times to the current time

        for ( INT K = 1; K <= m_Kmax; K++ )
        {
            pic->m_rgtick[ K - 1 ] = tickNow;
        }

        pic->m_pctCachePriority = pctCachePriority;
        pic->m_tickIndexTarget = _TickSanitizeTickForTickIndex( _ScaleTick( pic->m_rgtick[ m_K - 1 ], pic->m_pctCachePriority ) ) | ftickResourceNormalTouch;
        _AdjustIndexTargetForMinimumLifetime( pic );
    }
    else if ( !presHistoryProvided )
    {
        RESMGRAssert( pic->FResourceLocked() );
        const TICK tickLast = pic->TickLastTouchTime();
        rmtf = k1CorrelatedTouch;
        RESMGRAssert( _CmpTick( tickNow, tickLast ) >= 0 );
        if ( _CmpTick( tickNow, tickLast ) > 0 )
        {
            rmtf = _RmtfTouchResource( pic, pctCachePriority, tickNow );
            pic->m_tickLast = _TickSanitizeTickForTickLast( tickNow ) | ftickResourceLocked;
            _AdjustIndexTargetForMinimumLifetime( pic );
        }
    }

    RESMGRAssert( pic->FResourceLocked() );

    //  this is one of the best places to assert we are not entering trash into our eviction
    //  index / priority ... 

    _SanityCheckTick( pic->m_tickLast );
    _SanityCheckTick( pic->m_rgtick[ 1 - 1 ] );
    _SanityCheckTick( pic->m_rgtick[ m_K - 1 ] );
    _SanityCheckTickEx( pic->m_tickIndexTarget, ValidateTickCheck( vtcDefault | vtcAllowFarForward ) );


    //  check that we are did not insert it too far in the past
    //  we may have recovered a resource that looks a little old now, but we still decided to use its history
    //  to leverage K > 1 touches, which could turn into a higher-order touch soon
    //  also, when the history is provided, we can't assert anything

    if ( !presHistoryProvided && !( fRecoveredFromHistory && ( rmtf == k1CorrelatedTouch ) ) )
    {
        //  if the tickScanFirstFound* variables are in the future, we can't assert anything because the baseline for computing scaled ticks
        //  (and therefore the baseline for m_tickIndexTarget) is never in the future.
        RESMGRAssert( ( _CmpTick( tickScanFirstFoundAll, tickNow ) >= 0 ) || ( _CmpTick( pic->m_tickIndexTarget, tickScanFirstFoundAll ) >= -6000 ) );
        RESMGRAssert( ( _CmpTick( tickScanFirstFoundNormal, tickNow ) >= 0 ) || ( _CmpTick( pic->m_tickIndexTarget, tickScanFirstFoundNormal ) >= -9000 ) );
    }

    //  insert this resource into the resource LRUK 

    errLRUK = _ErrInsertResource( &lock, pres, NULL );

    RESMGRAssert(   errLRUK == CResourceLRUK::ERR::errSuccess ||
                    errLRUK == CResourceLRUK::ERR::errOutOfMemory );

    //  unlock the resource

    RESMGRAssert( pic->FResourceLocked() );
    const TICK tickLastRestore = _TickSanitizeTickForTickLast( pic->m_tickLast );
    OnDebug( const TICK tickCheck = ) AtomicExchange( (LONG*)&pic->m_tickLast, tickLastRestore );
    RESMGRAssert( _FTickLocked( tickCheck ) );

    return errLRUK == CResourceLRUK::ERR::errSuccess ? ERR::errSuccess : ERR::errOutOfMemory;
}

//  touches the specifed resource according to the LRUK Replacement Policy
//
//  NOTE:  only the touch times are updated here.  the actual update of this
//  resource's position in the LRUK index is amortized to ErrGetNextResource().
//  this scheme minimizes updates to the index and localizes those updates
//  to the thread(s) that perform cleanup of the resources.  this scheme also
//  makes it impossible to block while touching a resource

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ResMgrTouchFlags CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
RmtfTouchResource( CResource* const pres, const ULONG_PTR pctCachePriorityExternal, const TICK tickNowExternal )
{
    CInvasiveContext* const pic = _PicFromPres( pres );

    const WORD pctCachePriority = _AdjustCachePriority( pctCachePriorityExternal );
    const TICK tickNow = _TickSanitizeTickForTickLast( tickNowExternal );

    //  get the current last touch time of the resource.  we will use this time
    //  to lock the resource for touching.  if the last touch time ever changes
    //  to not match this touch time then we can no longer touch the resource

    const TICK tickLastBIExpected = pic->TickLastTouchTime();
    RESMGRAssert( !_FTickLocked( tickLastBIExpected ) );
    _SanityCheckTick( tickLastBIExpected );

    //  if the current time is equal to the last touch time then there is no
    //  need to touch the resource, unless we're raising its priority
    //
    //  NOTE:  also handle the degenerate case where the current time is less
    //  than the last touch time

    if ( ( _CmpTick( tickNow, tickLastBIExpected ) <= 0 ) && ( pctCachePriority <= pic->m_pctCachePriority ) )
    {
        //  do not touch the resource
        return kNoTouch;
    }

    //  if the current time is not equal to the last touch time then we need
    //  to update the touch times of the resource

    else
    {
        //  to determine who will get to actually touch the resource, we will do an
        //  AtomicCompareExchange to try and set the ftickResourceLocked on m_tickLast.
        //  the thread whose transaction succeeds wins the race and will touch the resource.
        //  all other threads will leave.  this is certainly acceptable as touches
        //  that collide are obviously too close together to be of any interest

        const TICK tickLastAI = _TickSanitizeTickForTickLast( tickLastBIExpected ) | ftickResourceLocked;
        RESMGRAssert( _FTickLocked( tickLastAI ) );

        const TICK tickLastBI = TICK( AtomicCompareExchange( (LONG*)&pic->m_tickLast, tickLastBIExpected, tickLastAI ) );

        if ( tickLastBI != tickLastBIExpected )
        {
            return kNoTouch;
        }

        RESMGRAssert( pic->FResourceLocked() );
        RESMGRAssert( !_FTickLocked( tickLastBI ) );
        RESMGRAssert( tickLastBI == _TickLastTouchTime( tickLastAI ) );

        const ResMgrTouchFlags rmtf = _RmtfTouchResource( pic, pctCachePriority, tickNow );

        RESMGRAssert( pic->FResourceLocked() );

        //  unlock the resource (note this does not restore the original m_tickLast, it replaces it with the new timestamp)

        const TICK tickLastRestore = _TickSanitizeTickForTickLast( tickNow );
        OnDebug( const TICK tickCheck = ) AtomicExchange( (LONG*)&pic->m_tickLast, tickLastRestore );
        RESMGRAssert( _FTickLocked( tickCheck ) );
        RESMGRAssert( tickCheck == tickLastAI );

        return rmtf;
    }
}

//  Used to migrate this resource away from the scavenging tail of the LRU-K.
//  Used so that we do not keep re-evaluating the resource without effect (such as if the resource is
//  hung in some form of unevictable state).
//
//  NOTE: it is important that this not actually eagerly moves the resource because we are in the middle
//  of a resource scan and could endlessly rotate resources to the end of the scan and never end our resource scan.

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
PuntResource( CResource* const pres, const TICK dtick )
{
#ifdef DEBUG
    //  assert that we must be calling this from within a resource scan loop (i.e., initiated
    //  by BeginResourceScan()) so that resources don't move while we punt

#ifdef _OS_HXX_INCLUDED
    RESMGRAssert( m_tidLockCheck == DwUtilThreadId() );
#else   //  !_OS_HXX_INCLUDED
    RESMGRAssert( m_tidLockCheck == 1 );
#endif  //  _OS_HXX_INCLUDED
#endif  //  DEBUG

    CInvasiveContext* const pic = _PicFromPres( pres );

    //  grab the lock first

    const TICK tickLastBIExpected = pic->TickLastTouchTime();
    const TICK tickLastAI = _TickSanitizeTickForTickLast( tickLastBIExpected ) | ftickResourceLocked;
    RESMGRAssert( _FTickLocked( tickLastAI ) );

    const TICK tickLastBI = TICK( AtomicCompareExchange( (LONG*)&pic->m_tickLast, tickLastBIExpected, tickLastAI ) );

    if ( tickLastBI != tickLastBIExpected )
    {
        return;
    }

    //  don't let it get too out of hand if we punt this repeatedly

    TICK tickIndexTarget = _TickSanitizeTickForTickIndex( pic->m_tickIndexTarget + dtick );
    const TICK tickIndexTargetMax = _TickCurrentTime() + ( m_K - 1 ) * m_ctickTimeout;
    if ( _CmpTick( tickIndexTarget, tickIndexTargetMax ) > 0 )
    {
        tickIndexTarget = tickIndexTargetMax;
    }

    tickIndexTarget = _TickSanitizeTickForTickIndex( tickIndexTarget );
    if ( !pic->FSuperColded() )
    {
        tickIndexTarget |= ftickResourceNormalTouch;
    }

    pic->m_tickIndexTarget = tickIndexTarget;

    //  release the lock

    RESMGRAssert( pic->FResourceLocked() );
    RESMGRAssert( !_FTickLocked( tickLastBI ) );
    OnDebug( const TICK tickCheck = ) AtomicExchange( (LONG*)&pic->m_tickLast, tickLastBI );
    RESMGRAssert( _FTickLocked( tickCheck ) );
    RESMGRAssert( tickCheck == tickLastAI );
}

//  returns fTrue if the specified resource was touched recently

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline BOOL CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
FRecentlyTouched( CResource* const pres, const TICK dtickRecent )
{
    CInvasiveContext* const pic = _PicFromPres( pres );

    return _CmpTick( pic->TickLastTouchTime(), _TickSanitizeTickForTickLast( TickRESMGRTimeCurrent() - dtickRecent ) ) > 0;
}

//  returns fTrue if the specified resource is touched as frequently and as
//  recently as possible (i.e. the resource appears to be as useful as
//  possible)

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline BOOL CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
FSuperHotResource( CResource* const pres )
{
    CInvasiveContext* const pic = _PicFromPres( pres );

    //  A resource is super hot if all touch times for the resource differ only
    //  by the timeout (plus correlation) and the last touch was a correlated touch and the last
    //  touch time equals the current time
    //  The reason we add m_ctickCorrelatedTouch here is because if a resource is super hot and gets touched continuously
    //  on every msec, at the first msec right after it becomes uncorrelated (i.e., becomes a double touch), the dtickCorrelationPeriod
    //  local var in _RmtfTouchResource() should be the whole correlation period so it gets added to the boost.
    //
    RESMGRAssert( fFalse );

    if ( ( pic->m_rgtick[ m_Kmax - 1 ] - pic->m_rgtick[ 1 - 1 ] ) == ( m_ctickTimeout + m_ctickCorrelatedTouch ) * ( m_Kmax - 1 ) )
    {
        const TICK tickLast = pic->TickLastTouchTime();

        if ( _CmpTick( tickLast, pic->m_rgtick[ 1 - 1 ] ) > 0 || !m_ctickCorrelatedTouch )
        {
            const TICK tickNow = _TickCurrentTime();

            if ( _CmpTick( tickLast, tickNow ) == 0 )
            {
                return fTrue;
            }
        }
    }

    return fFalse;
}

//  MarkAsSuperCold
//
// This function moves the resource back in time and helps ensure that it is
// among the first against the wall when scavenge comes. The timestamp is moved
// back in time as well and all tick times are reset.
//
 
template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
MarkAsSuperCold( CResource* const pres )
{
    CInvasiveContext* const pic = _PicFromPres( pres );

    _MarkForFastEviction( pic );
    return;
}

//  stops management of the specified resource optionally saving any current
//  knowledge the RUM has about this resource.  if the resource is not currently
//  cached by the RUM, errResourceNotCached will be returned

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrEvictResource( const CKey& key, CResource* const pres, const BOOL fKeepHistory )
{
    ERR     err = ERR::errSuccess;
    CLock   lock;

    //  lock and evict this resource from the resource LRUK by setting up our
    //  currency to look like we navigated to this resource normally and then
    //  calling our normal eviction routine

    LockResourceForEvict( pres, &lock );

    if ( ( err = ErrEvictCurrentResource( &lock, key, fKeepHistory ) ) != ERR::errSuccess )
    {
        RESMGRAssert( err == ERR::errNoCurrentResource );

        err = ERR::errResourceNotCached;
    }

    UnlockResourceForEvict( &lock );

    return err;
}

//  sets up the specified lock context in preparation for evicting the given
//  resource
//  NOTE: this is not a typical ftickResourceLocked lock, but an approximate index bucket lock used to evict the resource.

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
LockResourceForEvict( CResource* const pres, CLock* const plock )
{
    //  get the current index touch time for this resource

    CInvasiveContext* const pic = _PicFromPres( pres );

    const TICK tickIndex = pic->m_tickIndex;

    //  in order to lock this resource, we speculatively lock
    //  the bucket where we think the resource is located.
    //  after it is locked, we can then safely check if it is 
    //  the right place. the invariant is that m_tickIndex can
    //  should always equal its owning bucket (or tickUnindexed if
    //  no owner); the value is only guaranteed correct while the 
    //  matching bucket is locked. 

    RESMGRAssert( !plock->m_fHasLrukLock );
    m_ResourceLRUK.LockKeyPtr( tickIndex, pres, &plock->m_lock );
    plock->m_fHasLrukLock = fTrue;

    //  the index touch time has changed for this resource or it is tickUnindexed

    if ( pic->m_tickIndex != tickIndex || tickIndex == tickUnindexed )
    {
        //  release our lock and re-lock the bucket with our current entry
        //  explicitly set off any valid entry.  we do this so that we will
        //  have the desired errNoCurrentResource from ErrEvictCurrentResource

        RESMGRAssert( plock->m_fHasLrukLock );
        m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
        m_ResourceLRUK.MoveBeforeKeyPtr( tickIndex, pres, &plock->m_lock );
    }
#ifdef DEBUG
    else
    {
        CResource *presTmp;
        ERR errTmp = ErrGetCurrentResource( plock, &presTmp );
        RESMGRAssert( errTmp == ERR::errSuccess );
        RESMGRAssert( presTmp == pres );
    }
#endif // DEBUG

}

//  releases the lock acquired with LockResourceForEvict()

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
UnlockResourceForEvict( CLock* const plock )
{
    RESMGRAssert( plock->m_fHasLrukLock );
    m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
    plock->m_fHasLrukLock = fFalse;
}

//  sets up the specified lock context in preparation for scanning all resources
//  managed by the RUM by ascending utility
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via EndResourceScan()

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
BeginResourceScan( CLock* const plock )
{
    const TICK tickNow = _TickCurrentTime();

    RESMGRAssert( !plock->m_fHasLrukLock );
    memset( &plock->m_stats, 0, sizeof(plock->m_stats) );

    RS_STATISTICS( plock->m_stats.m_tickBeginResScan = tickNow );

    RESMGRAssert( plock->m_UpdateList.FEmpty() && plock->m_StuckList.FEmpty() );
    plock->m_presStuckList = NULL;
    plock->m_presStuckListNext = NULL;

    //  move before the first entry in the resource LRUK

    m_ResourceLRUK.MoveBeforeFirst( &plock->m_lock );
    plock->m_fHasLrukLock = fTrue;

    RESMGRTrace( rmttScanStats, L"\t\tBeginResScan:\n" );

#ifdef DEBUG
    RESMGRAssert( m_tidLockCheck == 0 /* very first first scan */ || m_tidLockCheck == (DWORD)-1 /* subsequent scans */ );
#ifdef _OS_HXX_INCLUDED
    m_tidLockCheck = DwUtilThreadId();
#else   //  !_OS_HXX_INCLUDED
    m_tidLockCheck = 1;
#endif  //  _OS_HXX_INCLUDED
#endif  //  DEBUG
}

//  retrieves the current resource managed by the RUM by ascending utility
//  locked by the specified lock context.  if there is no current resource,
//  errNoCurrentResource is returned

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrGetCurrentResource( CLock* const plock, CResource** const ppres )
{
    RESMGRTrace( rmttScanProcessing, L"\t\t\t\tErrGetCurrentRes( %d ) ... ", plock->m_StuckList.FEmpty() );

    //  we are currently walking the resource LRUK

    if ( plock->m_StuckList.FEmpty() )
    {
        CResourceLRUK::ERR errLRUK;

        //  get the current resource in the resource LRUK

        errLRUK = m_ResourceLRUK.ErrRetrieveEntry( &plock->m_lock, ppres );

        RESMGRTrace( rmttScanProcessing, L"pbf = %p:%p, err = %d\n", *ppres, _PicFromPres( *ppres ), errLRUK );

        //  return the status of our currency

        return errLRUK == CResourceLRUK::ERR::errSuccess ? ERR::errSuccess : ERR::errNoCurrentResource;
    }

    //  we are currently walking the list of resources that couldn't be
    //  moved

    else
    {
        //  return the status of our currency on the stuck list

        *ppres = plock->m_presStuckList;

        RESMGRTrace( rmttScanProcessing, L"pbf = %p:%p, err = %d\n", *ppres, _PicFromPres( *ppres ), *ppres ? ERR::errSuccess : ERR::errNoCurrentResource );

        return *ppres ? ERR::errSuccess : ERR::errNoCurrentResource;
    }
}

//  retrieves the next resource managed by the RUM by ascending utility locked
//  by the specified lock context.  if there are no more resources to be
//  scanned, errNoCurrentResource is returned

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrGetNextResource( CLock* const plock, CResource** const ppres )
{
    CResourceLRUK::ERR errLRUK;

#ifdef DEBUG
#ifdef _OS_HXX_INCLUDED
    RESMGRAssert( m_tidLockCheck == DwUtilThreadId() );
#else   //  !_OS_HXX_INCLUDED
    RESMGRAssert( m_tidLockCheck == 1 );
#endif  //  _OS_HXX_INCLUDED
#endif  //  DEBUG

    //  keep scanning until we establish currency on a resource or reach the
    //  end of the resource LRUK

    m_cResourceScanned++;

    RESMGRTrace( rmttScanProcessing, L"\t\t\tErrGetNextRes()\n" );

    *ppres  = NULL;
    memset( &plock->m_icCurrentBI, 0, sizeof(plock->m_icCurrentBI) );
#ifdef DEBUG
    plock->m_picCheckBI = NULL;
#endif

    errLRUK = CResourceLRUK::ERR::errSuccess;
    do
    {
        //  we are currently walking the resource LRUK

        BOOL fReestablishedCurrentIndex = fFalse;

        if ( plock->m_StuckList.FEmpty() )
        {
            //  scan forward in the index until we find the least useful
            //  resource that is in the correct place in the index or we reach
            //  the end of the index

            while ( ( errLRUK = m_ResourceLRUK.ErrMoveNext( &plock->m_lock ) ) == CResourceLRUK::ERR::errSuccess )
            {
                plock->m_fHasLrukLock = fTrue;
                CResourceLRUK::ERR errLRUK2 = CResourceLRUK::ERR::errSuccess;
                CResource* pres;

                RS_STATISTICS( plock->m_stats.m_cEntriesScanned++ );

                errLRUK2 = m_ResourceLRUK.ErrRetrieveEntry( &plock->m_lock, &pres );
                RESMGRAssert( errLRUK2 == CResourceLRUK::ERR::errSuccess );

                CInvasiveContext* const pic = _PicFromPres( pres );

                RESMGRTrace( rmttScanProcessing, L"\t\t\t\tProcessing pres:pic %p:%p, { %u, %u, [0] %u, [1] %u, %u [x], %hu }\n", pres, pic, pic->m_tickIndex, pic->m_tickIndexTarget, pic->m_rgtick[0], pic->m_rgtick[1], pic->m_tickLast, pic->m_pctCachePriority /* private: , plock->m_lock.m_bucket.m_id */ );

                _SanityCheckTickEx( pic->m_tickIndex, ValidateTickCheck( vtcAllowFarForward | vtcAllowIndexKey ) ); //  must allow far fwd b/c we don't know what inst / pct cache priority this came from

                //  try to validate that tickIndex matches the locked bucket. 
                //  b/c the matching depends on the hash function and as well as the
                //  state of the DHT, we simply do a try-lock which we expect to fail
                //  (since double-locks are not allowed). 

                RESMGRAssert( pic->m_tickIndex != TICK( tickUnindexed ) );
#ifdef DEBUG
                {
                CResourceLRUK::CLock lockValidate;

                RESMGRAssert( plock->m_fHasLrukLock );
                BOOL fLockSucceeded = m_ResourceLRUK.FTryLockKeyPtr( pic->m_tickIndex,  // current bucket
                                                                     pres,              // resource pointer
                                                                     &lockValidate);    // lock state

                RESMGRAssert( !fLockSucceeded );
                }
#endif

                BOOL fRightBucket = fFalse;

                //  if our update list contains resources and the tickIndex for
                //  those resources doesn't match the tickIndex for this
                //  resource then we need to stop and empty the update list

                if (    !plock->m_UpdateList.FEmpty() &&
                        m_ResourceLRUK.CmpKey( pic->m_tickIndex, plock->m_tickIndexCurrent ) )
                {
                    RESMGRAssert( pic->m_tickIndex != TICK( tickUnindexed ) );
                    RESMGRAssert( plock->m_tickIndexCurrent != TICK( tickUnindexed ) );
                    RS_STATISTICS( plock->m_stats.m_cUpdateListIndexChangeBreak++ );
                    RESMGRTrace( rmttScanProcessing, L"\t\t\t\t\tBreak to triage update list: %u != %u\n", pic->m_tickIndex, plock->m_tickIndexCurrent );
                    break;
                }

                //  We'll loop forever here waiting to acquire the lock. Not doing so may lead to unpredictable
                //  behavior because we'll read from the actual pic unsafely. It's acceptable to loop forever
                //  because this is the slow path and the other acquirers only perform fast memory operations.

                OSSYNC_FOREVER
                {
#ifdef DEBUG
                    // ~5% miss FI
                    const TICK tickLastBIExpected = _TickSanitizeTickForTickLast(
                                                        pic->TickLastTouchTime() +
                                                        ( _FFaultInj( rmfiScanTouchConcurrentFI ) ? ( rand() % 19 ) : 0 ) );
#else
                    const TICK tickLastBIExpected = pic->TickLastTouchTime();
#endif
                    RESMGRAssert( !_FTickLocked( tickLastBIExpected ) );

                    const TICK tickLastAI = _TickSanitizeTickForTickLast( tickLastBIExpected ) | ftickResourceLocked;
                    RESMGRAssert( _FTickLocked( tickLastAI ) );

                    const TICK tickLastBI = AtomicCompareExchange( (LONG*)&pic->m_tickLast, tickLastBIExpected, tickLastAI );

                    if ( tickLastBIExpected != tickLastBI )
                    {
                        continue;
                    }

                    RESMGRAssert( !_FTickLocked( tickLastBI ) );
                    RESMGRAssert( tickLastBI == _TickLastTouchTime( tickLastAI ) );
                    
                    //  we have grabbed the tick lock, so m_tickLast, m_rgtick[*] should now all be stable.

                    //  equivalent of: plock->m_icCurrentBI = *pic;
                    plock->m_icCurrentBI.m_tickLast = pic->m_tickLast;      //  pointless, see below
                    for ( INT K = m_Kmax; K >= 1; K-- )
                    {
                        plock->m_icCurrentBI.m_rgtick[ K - 1 ] = pic->m_rgtick[ K - 1 ];
                    }
                    plock->m_icCurrentBI.m_pctCachePriority = pic->m_pctCachePriority;
                    plock->m_icCurrentBI.m_tickIndex = pic->m_tickIndex;
                    plock->m_icCurrentBI.m_tickIndexTarget = pic->m_tickIndexTarget;
                    //  note: of course dropping the invasive list context piece of the IC

                    //  fix up the m_tickLast

                    RESMGRAssert( plock->m_icCurrentBI.m_tickLast == tickLastAI );
                    plock->m_icCurrentBI.m_tickLast = tickLastBI;

                    _SanityCheckTick( plock->m_icCurrentBI.m_tickLast );
                    _SanityCheckTick( plock->m_icCurrentBI.m_rgtick[ 1 - 1 ] );
                    _SanityCheckTick( plock->m_icCurrentBI.m_rgtick[ m_K - 1 ] );

                    //  note: since we're in scavenging, we can't know if these RESs are in a 
                    //  highly (i.e. >100%) prioritized context, so assume they are and pass
                    //  vtcAllowFarForward
                    _SanityCheckTickEx( plock->m_icCurrentBI.m_tickIndex, ValidateTickCheck( vtcAllowFarForward | vtcAllowIndexKey ) );
                    _SanityCheckTickEx( plock->m_icCurrentBI.m_tickIndexTarget, ValidateTickCheck( vtcAllowFarForward | vtcAllowIndexKey ) );

                    fRightBucket = m_ResourceLRUK.CmpKey( plock->m_icCurrentBI.m_tickIndex, plock->m_icCurrentBI.m_tickIndexTarget ) >= 0;

                    //  reset m_tickIndex so that other threads that take this lock cannot operate on the resource

                    if ( !fRightBucket )
                    {
                        pic->m_tickIndex = TICK( tickUnindexed );
                    }

                    //  release the touch lock

                    OnDebug( const TICK tickCheck = ) AtomicExchange( (LONG*)&pic->m_tickLast, tickLastBI );
                    RESMGRAssert( _FTickLocked( tickCheck ) );
                    RESMGRAssert( tickCheck == tickLastAI );

                    break;
                }

                RESMGRAssert( plock->m_icCurrentBI.m_tickIndex != tickUnindexed );
                RESMGRAssert( plock->m_fHasLrukLock );

                //  if the target index of this resource matches the tickIndex
                //  of the resource then we can return this resource as the
                //  current resource because it is in the correct place in the
                //  index. if it is further back, then it's ok to evict too.
                //
                //  we use CmpKey here becasue tickIndex may either be a key or tick (see
                //  how tickYoungest is used in this file)
                //  
                RESMGRAssert( !plock->m_icCurrentBI.FResourceLocked() );
                if ( fRightBucket )
                {
                    plock->m_stats.UpdateFoundTimes( this, &plock->m_icCurrentBI );

                    RS_STATISTICS( plock->m_stats.m_cEntriesFound++ );
                    
                    RESMGRTrace( rmttScanProcessing, L"\t\t\t\t\tReturning pres:pic %p:%p\n", pres, pic );  //  Success! We found a resource suitable for eviction! Return it.
                    RESMGRAssert( pic == _PicFromPres( pres ) );    // lot of code, loops, and iterative API model between pic assignment and here ...
#ifdef DEBUG
                    plock->m_picCheckBI = pic;
#endif
                    *ppres = pres;
                    break;
                }
                else
                {
                    //  if tickIndexTarget tick of this resource doesn't match the
                    //  tickIndex of the resource then add the resource to the
                    //  update list so that we can move it to the correct place
                    //  later.  leave space reserved for this resource here so
                    //  that we can always move it back in case of an error

                    errLRUK2 = m_ResourceLRUK.ErrReserveEntry( &plock->m_lock );
                    RESMGRAssert( errLRUK2 == CResourceLRUK::ERR::errSuccess );

                    RESMGRAssert( plock->m_fHasLrukLock );
                    errLRUK2 = m_ResourceLRUK.ErrDeleteEntry( &plock->m_lock );
                    RESMGRAssert( errLRUK2 == CResourceLRUK::ERR::errSuccess );

                    RESMGRAssert( plock->m_UpdateList.FEmpty() || ( m_ResourceLRUK.CmpKey( plock->m_icCurrentBI.m_tickIndex, plock->m_tickIndexCurrent ) == 0 ) ) ;

                    plock->m_tickIndexCurrent = plock->m_icCurrentBI.m_tickIndex;
                    RESMGRTrace( rmttScanProcessing, L"\t\t\t\t\tMoving to UpdateList pres:pic %p:%p ( m_tickIndexCurrent = %u )\n", pres, pic, plock->m_tickIndexCurrent );
                    plock->m_UpdateList.InsertAsNextMost( pres );
                    RS_STATISTICS( plock->m_stats.m_cMovedToUpdateList++ );
                }
            }   //  end while( m_ResourceLRUK.ErrMoveNext == errSuccess )

            RESMGRAssert(   errLRUK == CResourceLRUK::ERR::errSuccess ||
                            errLRUK == CResourceLRUK::ERR::errNoCurrentEntry );

            //  we still do not have a current resource

            if ( *ppres == NULL )
            {
                BOOL fNeedToMoveToCurrentIndex = fFalse;

                //  try to update the positions of any resources we have in our
                //  update list.  remove the reservation for any resource we
                //  can move and put any resource we can't move in the stuck
                //  list
                RESMGRTrace( rmttScanProcessing, L"\t\t\t\t\tProcessing deferred UpdateList ...\n" );

                while ( !plock->m_UpdateList.FEmpty() )
                {
                    CResource* const        pres    = plock->m_UpdateList.PrevMost();

                    plock->m_UpdateList.Remove( pres );

                    if ( plock->m_fHasLrukLock )
                    {
                        m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
                        plock->m_fHasLrukLock = fFalse;
                    }

                    const CResourceLRUK::ERR errLRUK2 = _ErrInsertResource( plock, pres, &plock->m_tickIndexCurrent );

                    if ( errLRUK2 == CResourceLRUK::ERR::errSuccess )
                    {
                        fNeedToMoveToCurrentIndex = fTrue;

                        RESMGRAssert( !plock->m_fHasLrukLock );
                        m_ResourceLRUK.LockKeyPtr( plock->m_tickIndexCurrent, pres, &plock->m_lock );
                        m_ResourceLRUK.UnreserveEntry( &plock->m_lock );
                        m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );

                        errLRUK = CResourceLRUK::ERR::errSuccess;
                        RS_STATISTICS( plock->m_stats.m_cMovedOutOfUpdateList++ );
                    }
                    else
                    {
                        plock->m_StuckList.InsertAsNextMost( pres );
                        RS_STATISTICS( plock->m_stats.m_cMovedToStuckList++ );
                    }
                }   //  while update list ! empty

                //  set our currency so that if we have any resources in the
                //  stuck list then we will walk them as if they are in the
                //  correct place in the resource LRUK

                if ( plock->m_StuckList.FEmpty() )
                {
                    if ( fNeedToMoveToCurrentIndex )
                    {
                        RESMGRAssert( !plock->m_fHasLrukLock );
                        //  this clause "(CResource*)DWORD_PTR( -LONG_PTR( sizeof( CResource ) ) )" deserves some explanation
                        //  basically since the approximate index is CPU hashed by pres pointer (4 buckets per actual / specific
                        //  TICK) then we need something that is ~0xfffffff ... the sizeof(CResource) is b/c the approximate 
                        //  index is dividing by pres / sizeof(CResource) to get perfectly even hashing.

                        RESMGRAssert( plock->m_tickIndexCurrent != TICK( tickUnindexed ) );
                        m_ResourceLRUK.MoveAfterKeyPtr( plock->m_tickIndexCurrent, (CResource*)DWORD_PTR( -LONG_PTR( sizeof( CResource ) ) ), &plock->m_lock );
                        plock->m_fHasLrukLock = fTrue;
                        fReestablishedCurrentIndex = ( plock->m_tickIndexCurrent != TICK( tickUnindexed ) );
                        plock->m_tickIndexCurrent = TICK( tickUnindexed );
                    }
                }
                else
                {
                    if ( plock->m_fHasLrukLock )
                    {
                        m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
                        plock->m_fHasLrukLock = fFalse;
                    }

                    plock->m_presStuckList      = NULL;
                    plock->m_presStuckListNext  = plock->m_StuckList.PrevMost();
                }
            }   //  end if ( *ppres == NULL )

            RESMGRTrace( rmttScanProcessing, L"\t\t\t\tEnd.\n" );
        }   //  end if ( stuck list empty )

        //  we are currently walking the list of resources that couldn't be
        //  moved

        if ( !plock->m_StuckList.FEmpty() )
        {
            //  move to the next resource in the list

            plock->m_presStuckList      = ( plock->m_presStuckList ?
                                                plock->m_StuckList.Next( plock->m_presStuckList ) :
                                                plock->m_presStuckListNext );
            plock->m_presStuckListNext  = NULL;

            //  we have a current resource

            if ( plock->m_presStuckList )
            {
                //  return the current resource

                m_cResourceScannedOutOfOrder++;
                RS_STATISTICS( plock->m_stats.m_cEntriesFoundOutOfOrder++ );
                //  Do not update the m_tickFirst/Last stats as this is out of order

                *ppres = plock->m_presStuckList;
                RESMGRTrace( rmttScanProcessing, L"\t\t\t\tOutOfOrderPres %p:%p { %u, [0] %u, [1] %u, %u, %u }\n",
                                *ppres, _PicFromPres( *ppres ), _PicFromPres( *ppres )->m_tickLast,
                                _PicFromPres( *ppres )->m_rgtick[0], _PicFromPres( *ppres )->m_rgtick[1],
                                _PicFromPres( *ppres )->m_tickIndex, _PicFromPres( *ppres )->m_tickIndexTarget );
            }

            //  we still do not have a current resource

            else
            {
                //  we have walked off of the end of the list of resources that
                //  couldn't be moved so we need to put them back into their
                //  source buckets before we move on to the next entry in the
                //  resource LRUK.  we are guaranteed to be able to put them
                //  back because we reserved space for them

                while ( !plock->m_StuckList.FEmpty() )
                {
                    CResource*          pres        = plock->m_StuckList.PrevMost();
                    CResourceLRUK::ERR  errLRUK2    = CResourceLRUK::ERR::errSuccess;

                    RESMGRTrace( rmttScanProcessing, L"\t\t\t\tStucklist, %p:%p (tickLast = %u) returning to original location = %u\n",
                                    pres, _PicFromPres( pres ), _PicFromPres( pres )->m_tickLast,
                                    plock->m_tickIndexCurrent );

                    plock->m_StuckList.Remove( pres );

                    RESMGRAssert( !plock->m_fHasLrukLock );
                    m_ResourceLRUK.LockKeyPtr( plock->m_tickIndexCurrent, pres, &plock->m_lock );
                    plock->m_fHasLrukLock = fTrue;
                    _SanityCheckTickEx( plock->m_tickIndexCurrent, vtcAllowIndexKey );

                    RESMGRAssert( plock->m_fHasLrukLock );
                    m_ResourceLRUK.UnreserveEntry( &plock->m_lock );

                    _PicFromPres( pres )->m_tickIndex = plock->m_tickIndexCurrent;
                    _SanityCheckTickEx( _PicFromPres( pres )->m_tickIndex, vtcAllowIndexKey );
                    RESMGRAssert( plock->m_fHasLrukLock );
                    errLRUK2 = m_ResourceLRUK.ErrInsertEntry( &plock->m_lock, pres, fTrue );
                    RESMGRAssert( errLRUK2 == CResourceLRUK::ERR::errSuccess );

                    RESMGRAssert( plock->m_fHasLrukLock );
                    m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
                    plock->m_fHasLrukLock = fFalse;
                }

                //  we are no longer walking the stuck list so restore our
                //  currency on the resource LRUK

                RESMGRAssert( plock->m_tickIndexCurrent != TICK( tickUnindexed ) );
                RESMGRAssert( !plock->m_fHasLrukLock );
                m_ResourceLRUK.MoveAfterKeyPtr( plock->m_tickIndexCurrent, (CResource*)DWORD_PTR( -LONG_PTR( sizeof( CResource ) ) ), &plock->m_lock );
                plock->m_fHasLrukLock = fTrue;
                fReestablishedCurrentIndex = ( plock->m_tickIndexCurrent != TICK( tickUnindexed ) );
                plock->m_tickIndexCurrent = TICK( tickUnindexed );
            }
        }   // end if ( stuck list ! empty )

        //  currency may have been restored above, set errLRUK to success if we got to the end of the index
        //  to force an extra iteration because we may have a resource to return

        if ( ( *ppres == NULL ) && ( errLRUK == CResourceLRUK::ERR::errNoCurrentEntry ) && fReestablishedCurrentIndex )
        {
            errLRUK = CResourceLRUK::ERR::errSuccess;
        }
    }
    while ( *ppres == NULL && errLRUK != CResourceLRUK::ERR::errNoCurrentEntry );

    if ( *ppres != NULL )
    {
        RESMGRTrace( rmttScanProcessing, L"\t\t\t\tReturning pres:pic = %p:%p { %u, [0] %u, [1] %u, %u, %u, %hu }\n",
                        *ppres, _PicFromPres( *ppres ), _PicFromPres( *ppres )->m_tickLast,
                        _PicFromPres( *ppres )->m_rgtick[0], _PicFromPres( *ppres )->m_rgtick[1],
                        _PicFromPres( *ppres )->m_tickIndex, _PicFromPres( *ppres )->m_tickIndexTarget,
                        _PicFromPres( *ppres )->m_pctCachePriority );
    }

    //  make sure we didn't leave anything behind to evaluate

    RESMGRAssert( *ppres || ( plock->m_UpdateList.FEmpty() && plock->m_StuckList.FEmpty() ) );

    //  If the function is returning something, we must be holding the bucket lock or must be returning something from the stuck list.
    //  If the function is not returning anything, the state of the lock can't be asserted.

    RESMGRAssert( ( *ppres == NULL ) || plock->m_fHasLrukLock || ( *ppres == plock->m_presStuckList ) );

    //  return the state of our currency

    return *ppres ? ERR::errSuccess : ERR::errNoCurrentResource;
}

//  stops management of the current resource optionally saving any current
//  knowledge the RUM has about this resource.  if there is no current
//  resource, errNoCurrentResource will be returned

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrEvictCurrentResource( CLock* const plock, const CKey& key, const BOOL fKeepHistory )
{
    ERR                     err;
    CResourceLRUK::ERR      errLRUK;
    CResource*              pres;

    //  get the current resource in the resource LRUK, if any

    if ( ( err = ErrGetCurrentResource( plock, &pres ) ) != ERR::errSuccess )
    {
        return err;
    }

#ifdef DEBUG
    if ( plock->m_picCheckBI )
    {
        //  this was gotten from a scan, ensure we have the scan lock ...
#ifdef _OS_HXX_INCLUDED
        RESMGRAssert( m_tidLockCheck == DwUtilThreadId() );
#else   //  !_OS_HXX_INCLUDED
        RESMGRAssert( m_tidLockCheck == 1 );
#endif  //  _OS_HXX_INCLUDED
    }
#endif  //  DEBUG

    CInvasiveContext * const pic = _PicFromPres( pres );
    const TICK tickLast = pic->TickLastTouchTime();
    RESMGRAssert( !plock->m_StuckList.FEmpty() || pic->m_tickIndex != tickUnindexed );

    //  we are currently walking the resource LRUK

    if ( plock->m_StuckList.FEmpty() )
    {
        //  delete the current resource from the resource LRUK

        RESMGRAssert( plock->m_fHasLrukLock );
        errLRUK = m_ResourceLRUK.ErrDeleteEntry( &plock->m_lock );
        RESMGRAssert( errLRUK == CResourceLRUK::ERR::errSuccess );

        //  update stats

        if ( ( plock->m_icCurrentBI.m_tickLast != 0 ) && ( plock->m_icCurrentBI.m_tickLast == tickLast ) )
        {
            RESMGRAssert( !plock->m_icCurrentBI.FResourceLocked() );

            //  nothing has changed, we can use these stats

            RESMGRAssert( plock->m_picCheckBI == pic );

            plock->m_stats.UpdateEvictTimes( this, &plock->m_icCurrentBI );
        }
    }

    //  we are currently walking the list of resources that couldn't be
    //  moved

    else
    {
        //  delete the current resource from the stuck list and remove its
        //  reservation from the resource LRUK

        plock->m_presStuckListNext = plock->m_StuckList.Next( pres );
        plock->m_StuckList.Remove( pres );
        plock->m_presStuckList = NULL;

        RESMGRAssert( !plock->m_fHasLrukLock );
        m_ResourceLRUK.LockKeyPtr( plock->m_tickIndexCurrent, pres, &plock->m_lock );
        m_ResourceLRUK.UnreserveEntry( &plock->m_lock );
        m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );

        //  if we are no longer walking the stuck list then restore our currency
        //  on the resource LRUK

        if ( plock->m_StuckList.FEmpty() )
        {
            RESMGRAssert( plock->m_tickIndexCurrent != TICK( tickUnindexed ) );
            RESMGRAssert( !plock->m_fHasLrukLock );
            m_ResourceLRUK.MoveAfterKeyPtr( plock->m_tickIndexCurrent, (CResource*)DWORD_PTR( -LONG_PTR( sizeof( CResource ) ) ), &plock->m_lock );
            plock->m_fHasLrukLock = fTrue;
            plock->m_tickIndexCurrent = TICK( tickUnindexed );
        }
    }

    //  updating statistics

    RS_STATISTICS( plock->m_stats.m_cEntriesEvicted++ );

    if ( pic->FSuperColded() )
    {
        //  this resource is super colded, decrement count

        AtomicDecrement( (LONG*)&m_cSuperCold );
    }
    else if ( fKeepHistory )
    {
        //  store the history for this resource only if requested and the resource has not been supercolded

        _StoreHistory( key, pic );
    }

    return ERR::errSuccess;
}

//  ends the scan of all resources managed by the RUM by ascending utility
//  associated with the specified lock context and releases all locks held

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
EndResourceScan( CLock* const plock )
{
#ifdef DEBUG
#ifdef _OS_HXX_INCLUDED
    RESMGRAssert( m_tidLockCheck == DwUtilThreadId() );
    m_tidLockCheck = (DWORD)-1;
#else   //  !_OS_HXX_INCLUDED
    RESMGRAssert( m_tidLockCheck == 1 );
    m_tidLockCheck = (DWORD)-1;
#endif  //  _OS_HXX_INCLUDED
#endif  //  DEBUG

    //  release our lock on the resource LRUK

    if ( plock->m_fHasLrukLock )
    {
        RESMGRAssert( plock->m_StuckList.FEmpty() );
        m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
        plock->m_fHasLrukLock = fFalse;
    }

    //  we are currently walking the resource LRUK

    if ( plock->m_StuckList.FEmpty() )
    {
        //  try to update the positions of any resources we have in our
        //  update list.  remove the reservation for any resource we
        //  can move and put any resource we can't move in the stuck
        //  list

        while ( !plock->m_UpdateList.FEmpty() )
        {
            CResource* const        pres    = plock->m_UpdateList.PrevMost();

            plock->m_UpdateList.Remove( pres );


            const CResourceLRUK::ERR errLRUK = _ErrInsertResource( plock, pres, NULL );

            if ( errLRUK == CResourceLRUK::ERR::errSuccess )
            {
                RESMGRAssert( !plock->m_fHasLrukLock );
                m_ResourceLRUK.LockKeyPtr( plock->m_tickIndexCurrent, pres, &plock->m_lock );
                m_ResourceLRUK.UnreserveEntry( &plock->m_lock );
                m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );

                m_cResourceScannedMoves++;
                RS_STATISTICS( plock->m_stats.m_cMovedOutOfUpdateList++ );
            }
            else
            {
                RESMGRAssert( errLRUK != CResourceLRUK::ERR::errKeyRangeExceeded );
                plock->m_StuckList.InsertAsNextMost( pres );
                RS_STATISTICS( plock->m_stats.m_cMovedToStuckList++ );
            }

        }   //  while more update list entries to process

    }   //  if stuck list empty

    //  My (SOMEONE) read on this code, this would be bad ... as it would 
    //  get "abandoned outside" of the m_ResourceLRUK index

    RESMGRAssert( plock->m_UpdateList.FEmpty() );

    //  put all the resources we were not able to move back into their
    //  source buckets.  we know that we will succeed because we reserved
    //  space for them

    while ( !plock->m_StuckList.FEmpty() )
    {
        CResource*          pres    = plock->m_StuckList.PrevMost();
        CResourceLRUK::ERR  errLRUK = CResourceLRUK::ERR::errSuccess;

        plock->m_StuckList.Remove( pres );

        RESMGRAssert( !plock->m_fHasLrukLock );
        m_ResourceLRUK.LockKeyPtr( plock->m_tickIndexCurrent, pres, &plock->m_lock );
        plock->m_fHasLrukLock = fTrue;
        _SanityCheckTickEx( plock->m_tickIndexCurrent, vtcAllowIndexKey );

        RESMGRAssert( plock->m_fHasLrukLock );
        m_ResourceLRUK.UnreserveEntry( &plock->m_lock );

        _PicFromPres( pres )->m_tickIndex = plock->m_tickIndexCurrent;
        _SanityCheckTickEx( _PicFromPres( pres )->m_tickIndex, vtcAllowIndexKey );
        RESMGRAssert( plock->m_fHasLrukLock );
        errLRUK = m_ResourceLRUK.ErrInsertEntry( &plock->m_lock, pres, fTrue );
        RESMGRAssert( errLRUK == CResourceLRUK::ERR::errSuccess );

        RESMGRAssert( plock->m_fHasLrukLock );
        m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
        plock->m_fHasLrukLock = fFalse;

        RS_STATISTICS( plock->m_stats.m_cStuckListReturned++ );
    }

    //  Finally update all the statistics about the completed resource 
    //  scan.  Some statistics are always kept (to managed super cold
    //  insertion point, and cache priority scaling), and others are
    //  just for tracking stats and perf counters.

    RS_STATISTICS( const TICK dtickScan = _TickCurrentTime() - plock->m_stats.m_tickBeginResScan );

    //  update statistics about the efficacy of our m_ResourceLRUK / approx index walk

    m_cLastScanEnumeratedEntries    = plock->m_lock.CEnumeratedEntries();
    m_cLastScanBucketsScanned       = plock->m_lock.CEnumeratedBuckets();
    m_cLastScanEmptyBucketsScanned  = plock->m_lock.CEnumeratedEmptyBuckets();
    m_cLastScanEnumeratedIDRange    = m_ResourceLRUK.CEnumeratedRangeVirtual( &plock->m_lock );
    m_dtickLastScanEnumeratedRange  = m_ResourceLRUK.CEnumeratedRangeReal( &plock->m_lock );

    if ( plock->m_stats.m_tickScanFirstFoundAll != 0 )
    {
        //  Note: The m_tickLast[Found|Evicted]* values can gyrate quite well, due to decisions about when BF
        //  decides to stop scanning, as things get cleaned, etc.
        _ValidateTickProgress( L"SFFA", m_tickScanFirstFoundAll, plock->m_stats.m_tickScanFirstFoundAll, vtpSuperColdBackwardCheck );
        m_tickScanFirstFoundAll = plock->m_stats.m_tickScanFirstFoundAll;
    }

#ifdef RESMGR_LOWBAR_CHECKING_FIXCREEP
    RESMGRAssert( _CmpTick( m_tickScanFirstFoundAll, m_tickLowBar ) >= 0 );
#endif

    if ( plock->m_stats.m_tickScanLastFound != 0 )
    {
        m_tickScanLastFound = plock->m_stats.m_tickScanLastFound;
    }

    if ( plock->m_stats.m_tickScanFirstFoundNormal != 0 )
    {
        _ValidateTickProgress( L"SFFN", m_tickScanFirstFoundNormal, plock->m_stats.m_tickScanFirstFoundNormal, vtpConcurrentBackwardCheck );
        m_tickScanFirstFoundNormal = plock->m_stats.m_tickScanFirstFoundNormal;
    }

    RESMGRAssert( !( ( plock->m_stats.m_tickScanLastFound == 0 ) && ( plock->m_stats.m_tickScanFirstFoundAll != 0 ) ) );
    RESMGRAssert( !( ( plock->m_stats.m_tickScanLastFound == 0 ) && ( plock->m_stats.m_tickScanFirstFoundNormal != 0 ) ) );
    RESMGRAssert( !( ( plock->m_stats.m_tickScanFirstFoundAll == 0 ) && ( plock->m_stats.m_tickScanFirstFoundNormal != 0 ) ) );
    RESMGRAssert( ( plock->m_stats.m_tickScanFirstFoundAll == 0 ) || ( _CmpTick( m_tickScanFirstFoundAll, m_tickScanLastFound ) <= 0 ) );
    RESMGRAssert( ( plock->m_stats.m_tickScanFirstFoundNormal == 0 ) || ( _CmpTick( m_tickScanFirstFoundNormal, m_tickScanLastFound ) <= 0 ) );
    RESMGRAssert( ( plock->m_stats.m_tickScanFirstFoundNormal == 0 ) || ( _CmpTick( m_tickScanFirstFoundAll, m_tickScanFirstFoundNormal ) <= 0 ) );

    if ( plock->m_stats.m_tickScanFirstEvictedTouchK1 != 0 )
    {
        _ValidateTickProgress( L"SFEK1", m_tickScanFirstEvictedTouchK1, plock->m_stats.m_tickScanFirstEvictedTouchK1, vtpConcurrentBackwardCheck );
        m_tickScanFirstEvictedTouchK1 = plock->m_stats.m_tickScanFirstEvictedTouchK1;
    }

    if ( plock->m_stats.m_tickScanFirstEvictedTouchK2 != 0 )
    {
        _ValidateTickProgress( L"SFEK2", m_tickScanFirstEvictedTouchK2, plock->m_stats.m_tickScanFirstEvictedTouchK2, vtpNone );
        m_tickScanFirstEvictedTouchK2 = plock->m_stats.m_tickScanFirstEvictedTouchK2;
    }

    // Impossible to _ValidateTickProgress() on this.
    if ( plock->m_stats.m_tickScanFirstEvictedIndexTarget != 0 )
    {
        m_tickScanFirstEvictedIndexTarget = plock->m_stats.m_tickScanFirstEvictedIndexTarget;
    }

    if ( _CmpTick( m_tickScanFirstEvictedIndexTarget, m_tickScanFirstEvictedIndexTargetHW ) > 0 )
    {
        _ValidateTickProgress( L"SFEITHW", m_tickScanFirstEvictedIndexTargetHW, m_tickScanFirstEvictedIndexTarget, vtpBucketBackwardCheck );
        m_tickScanFirstEvictedIndexTargetHW = m_tickScanFirstEvictedIndexTarget;
        m_tickLatestScanFirstEvictedIndexTargetHWUpdate = _TickCurrentTime();
        m_tickScanFirstFoundAllIndexTargetHW = m_tickScanFirstFoundAll;
        m_tickScanFirstFoundNormalIndexTargetHW = m_tickScanFirstFoundNormal;
    }

#ifdef RESSCAN_STATISTICS

    const TICK tickNow = _TickCurrentTime();

    //  just keep the last and the previous run

    memcpy( &m_statsScanPrev, &m_statsScanLast, sizeof(m_statsScanPrev) );
    memcpy( &m_statsScanLast, &plock->m_stats, sizeof(m_statsScanLast) );

    //  for debugging it is nice to trace what our scan looked like

    RESMGRTrace( rmttScanStats, L"\t\tEndResLock Stats: %4d {%4d, %2d, %1d} -> %4d {%2d, %2d, %2d, %2d};  FR:%u|%u-%u(%d,%d)  ER: Z-%u-%u|%u(%d,%d)  CL:S:%d, L:%d, N:%d Lo:%d 12:%d|%d ... FE-Sep: %d ... %d ms\n",
                    plock->m_stats.m_cEntriesScanned,
                    plock->m_stats.m_cEntriesFound,
                    plock->m_stats.m_cEntriesFoundOutOfOrder,
                    plock->m_stats.m_cUpdateListIndexChangeBreak,

                    plock->m_stats.m_cEntriesEvicted,
                    plock->m_stats.m_cMovedToUpdateList,
                    plock->m_stats.m_cMovedOutOfUpdateList,
                    plock->m_stats.m_cMovedToStuckList,
                    plock->m_stats.m_cStuckListReturned,
    
                    plock->m_stats.m_tickScanFirstFoundAll, plock->m_stats.m_tickScanFirstFoundNormal, plock->m_stats.m_tickScanLastFound,
                        INT( plock->m_stats.m_tickScanLastFound - plock->m_stats.m_tickScanFirstFoundAll ),
                        INT( plock->m_stats.m_tickScanLastFound - plock->m_stats.m_tickScanFirstFoundNormal ),
    
                    plock->m_stats.m_tickScanFirstFoundAll, plock->m_stats.m_tickScanFirstEvictedTouchK1, plock->m_stats.m_tickScanFirstEvictedTouchK2,
                        INT( plock->m_stats.m_tickScanFirstEvictedTouchK1 - plock->m_stats.m_tickScanFirstFoundAll ),
                        INT( plock->m_stats.m_tickScanFirstEvictedTouchK2 - plock->m_stats.m_tickScanFirstFoundAll ),
    
                    INT( tickNow - plock->m_stats.m_tickScanFirstFoundAll ),
                    INT( tickNow - plock->m_stats.m_tickScanFirstFoundNormal ),
                    INT( tickNow - plock->m_stats.m_tickScanFirstEvictedIndexTarget ),
                    INT( tickNow - m_tickScanFirstEvictedIndexTargetHW ),
                    INT( tickNow - plock->m_stats.m_tickScanFirstEvictedTouchK1 ),
                    INT( tickNow - plock->m_stats.m_tickScanFirstEvictedTouchK2 ),
    
                    INT( plock->m_stats.m_tickScanFirstEvictedIndexTarget - plock->m_stats.m_tickScanFirstFoundAll ),
    
                    dtickScan
                    );

    RESMGRTrace( rmttScanStats, L"\t\tEndResScan Stats:  %4d {%4d, %2d, %1d} -> %4d {%2d, %2d, %2d, %2d};  FR:%u|%u-%u(%d,%d)  ER: Z-%u-%u|%u(%d,%d)  CL:S:%d, L:%d, N:%d Lo:%d SLo:%d NLo:%d 12:%d|%d ... IndexTargetVar:%d  FE-Sep: %d  ... %d ms\n",
                plock->m_stats.m_cEntriesScanned,
                plock->m_stats.m_cEntriesFound,
                plock->m_stats.m_cEntriesFoundOutOfOrder,
                plock->m_stats.m_cUpdateListIndexChangeBreak,

                plock->m_stats.m_cEntriesEvicted,
                plock->m_stats.m_cMovedToUpdateList,
                plock->m_stats.m_cMovedOutOfUpdateList,
                plock->m_stats.m_cMovedToStuckList,
                plock->m_stats.m_cStuckListReturned,

                m_tickScanFirstFoundAll, m_tickScanFirstFoundNormal, m_tickScanLastFound,
                    INT( m_tickScanLastFound - m_tickScanFirstFoundAll ),
                    INT( m_tickScanLastFound - m_tickScanFirstFoundNormal ),

                m_tickScanFirstFoundAll, m_tickScanFirstEvictedTouchK1, m_tickScanFirstEvictedTouchK2,
                    INT( m_tickScanFirstEvictedTouchK1 - m_tickScanFirstFoundAll ),
                    INT( m_tickScanFirstEvictedTouchK2 - m_tickScanFirstFoundAll ),

                INT( tickNow - m_tickScanFirstFoundAll ),
                INT( tickNow - m_tickScanFirstFoundNormal ),
                INT( tickNow - m_tickScanFirstEvictedIndexTarget ),
                INT( tickNow - m_tickScanFirstEvictedIndexTargetHW ),
                INT( tickNow - m_tickScanFirstFoundAllIndexTargetHW ),
                INT( tickNow - m_tickScanFirstFoundNormalIndexTargetHW ),
    
                INT( tickNow - m_tickScanFirstEvictedTouchK1 ),
                INT( tickNow - m_tickScanFirstEvictedTouchK2 ),

                INT( m_tickScanFirstEvictedIndexTargetHW - m_tickScanFirstEvictedIndexTarget ),

                INT( m_tickScanFirstEvictedIndexTarget - m_tickScanFirstFoundAll ),

                dtickScan
                );
#endif  // RESSCAN_STATISTICS

    RESMGRAssert( !plock->m_fHasLrukLock );
}

//  returns the current number of resource history records retained for
//  supporting the LRUK replacement policy

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CHistoryRecord()
{
    return m_cHistoryRecord;
}

//  returns the cumulative number of successful resource history record loads
//  when caching a resource

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CHistoryHit()
{
    return m_cHistoryHit;
}

//  returns the cumulative number of attempted resource history record loads
//  when caching a resource

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CHistoryRequest()
{
    return m_cHistoryReq;
}

//  returns the cumulative number of resources scanned in the LRUK

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CResourceScanned()
{
    return m_cResourceScanned;
}

//  returns the cumulative number of resources scanned in the LRUK that were
//  moved because they were out of order

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CResourceScannedMoves()
{
    return m_cResourceScannedMoves;
}

//  returns the cumulative number of resources scanned in the LRUK that were
//  returned out of order

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CResourceScannedOutOfOrder()
{
    return m_cResourceScannedOutOfOrder;
}

//  updates the touch times of a given resource

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ResMgrTouchFlags CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_RmtfTouchResource( CInvasiveContext* const pic, const WORD pctCachePriority, const TICK tickNow )
{

    RESMGRAssert( pic->FResourceLocked() );
    RESMGRAssert( !_FTickLocked( tickNow ) );
    _SanityCheckTick( tickNow );
    _SanityCheckTick( pic->m_tickLast );

    ResMgrTouchFlags rmtf = kNoTouch;
    BOOL fRecomputeIndexTarget = fFalse;
    const TICK tickLast = pic->TickLastTouchTime();
    RESMGRAssert( !_FTickLocked( tickLast ) );

    //  the resulting cache priority is the max of the current priority of the resource and the
    //  new priority

    if ( pctCachePriority > pic->m_pctCachePriority )
    {
        pic->m_pctCachePriority = pctCachePriority;

        //  even though this may be a correlated touch, force re-calculation on the index target
        //  to make sure the new priority is captured right away

        fRecomputeIndexTarget = fTrue;
    }

    //  if the resource has been previously supercolded, update the global count and request recomputing
    //  the index, which will effectively revert the supercold state

    if ( pic->FSuperColded() )
    {
        AtomicDecrement( (LONG*)&m_cSuperCold );
        fRecomputeIndexTarget = fTrue;
    }

    //  if the current time and the last historical touch time are closer than
    //  the correlation interval then this is a correlated touch
    //
    //  the correlation interval is wall-clock interval that corresponds to the
    //  multiple touches that happen as part of single "logical" operation to distinguish 
    //  data that is used once from data that is repeatedly accessed.

    if ( _DtickDelta( pic->m_rgtick[ 1 - 1 ], tickNow ) <= (LONG)m_ctickCorrelatedTouch )
    {
        rmtf = k1CorrelatedTouch;
        goto CheckComputeIndexTarget;
    }

    //  if the current time and the last historical touch time are not closer
    //  than the correlation interval, then do not treat this as a correlated touch

    fRecomputeIndexTarget = fTrue;

    _SanityCheckTick( m_tickScanFirstFoundNormal );
    _SanityCheckTick( m_tickScanFirstEvictedIndexTarget );
    _SanityCheckTick( m_tickScanFirstEvictedIndexTargetHW );

    //  compute the correlation period of the last series of correlated
    //  touches by taking the difference between the last touch time and
    //  the most recent touch time in our history

    const LONG dtickCorrelationPeriod = _DtickDelta( pic->m_rgtick[ 1 - 1 ], tickLast );
    RESMGRAssert( dtickCorrelationPeriod >= 0 );

    //  move all touch times up one slot and advance them by the correlation
    //  period and the timeout.  this will collapse all correlated touches
    //  to a time interval of zero and will make resources that have more
    //  uncorrelated touches appear younger in the index

    for ( INT K = m_Kmax; K >= 2; K-- )
    {
        pic->m_rgtick[ K - 1 ] = _TickSanitizeTick( pic->m_rgtick[ ( K - 1 ) - 1 ] + (TICK)dtickCorrelationPeriod + m_ctickTimeout );
        _SanityCheckTick( pic->m_rgtick[ K - 1 ] );
    }

    //  set the most recent historical touch time to the current time

    pic->m_rgtick[ 1 - 1 ] = tickNow;
    
    rmtf = ( ( m_K == 1 ) ? k1Touch : k2Touch );

CheckComputeIndexTarget:

    //  recompute index target if necessary

    if ( fRecomputeIndexTarget )
    {
        //  Some highly concurrent stress tests that play with a wide range of cache priorities may and up with a new tickIndexTarget
        //  which is older than the current m_tickIndexTarget. This may happen if, for example, we're incrementing priority by, say, only 1
        //  and because of the highly concurrent nature of the test, the oldest estimate go backwards in-between and we may end up with
        //  an older new tickIndexTarget.
        
        const TICK tickIndexTarget = _TickSanitizeTickForTickIndex( _ScaleTick( pic->m_rgtick[ m_K - 1 ], pic->m_pctCachePriority ) );
        if ( _CmpTick( tickIndexTarget, pic->m_tickIndexTarget ) >= 0 )
        {
            pic->m_tickIndexTarget = tickIndexTarget;
        }

        pic->m_tickIndexTarget |= ftickResourceNormalTouch;
    }

    RESMGRAssert( pic->FResourceLocked() );
    RESMGRAssert( rmtf != kNoTouch );

    return rmtf;
}

//  insert this resource into the resource LRUK at its Kth touch time or as
//  close as we can get to that Kth touch time

//  in debug we have a very small approximate index, and so we overlay ticks 
//  on each other.  even in retail we have a 31 or 32 bit approximate index,
//  but we still have to overlay on the old ticks due to natural integer wrap
//  at 49 days.  this creates sort of a problem in that we can have ticks
//  spanning larger than the approximate index can address.  SOOO when that
//  happens we have to insert resources "out of order", not in thier natural
//  place.

//  the ptickReserved is because we are currently enumerating that particular
//  bucket ... I'm not sure if this is a consistency issue if we just ignored
//  ptickReserved, or if it would just be silly b/c you would be moving it
//  back to the exact same bucket.


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR_RES_APPIDX CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_ErrInsertResource( CLock * const plock, CResource * const pres, const TICK * ptickReserved )
{
    CInvasiveContext * const pic = _PicFromPres( pres );
    

    CResourceLRUK::ERR errLRUK;


    RESMGRAssert( !plock->m_fHasLrukLock );
    RESMGRAssert( pic->m_tickIndex == tickUnindexed );
    ULONG cInsertionAttempts = 0;

    do
    {
        //  Detect if we're trying to position the resource in a place that would wrap-around our approximate index range.
        TICK tickIndexTarget = pic->m_tickIndexTarget;

        //  If we're about to try an insertion at an older bucket where we pulled it from, return OOM and let the upper
        //  layers handle it. This is necessary to avoid this resource never being returned, since the cursor will be
        //  repositioned after the current bucket in ErrGetNextResource().
        //  Note that ErrCacheResource() does not hit this codepath because ptickReserved is NULL in that case, i.e.,
        //  we're not trying to move a resource, but rather insert it for the first time.
        if ( ( ptickReserved != NULL ) && ( m_ResourceLRUK.CmpKey( tickIndexTarget, *ptickReserved ) <= 0 ) )
        {
            errLRUK = CResourceLRUK::ERR::errOutOfMemory;
            break;
        }

        RESMGRAssert( !plock->m_fHasLrukLock );
        m_ResourceLRUK.LockKeyPtr( tickIndexTarget, pres, &plock->m_lock );
        plock->m_fHasLrukLock = fTrue;

        pic->m_tickIndex = tickIndexTarget;
        errLRUK = m_ResourceLRUK.ErrInsertEntry( &plock->m_lock, pres, fTrue );
        const TICK tickYoungest = m_ResourceLRUK.KeyInsertMost();

        if ( errLRUK != CResourceLRUK::ERR::errSuccess )
        {
            pic->m_tickIndex = TICK( tickUnindexed );
            RESMGRAssert( plock->m_fHasLrukLock );
            m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
            plock->m_fHasLrukLock = fFalse;

            tickIndexTarget = tickYoungest;

            //  We're about to try an insertion at an older bucket where we pulled it from. See comment a few lines above
            //  in this function.
            if ( ( ptickReserved != NULL ) && ( m_ResourceLRUK.CmpKey( tickIndexTarget, *ptickReserved ) <= 0 ) )
            {
                errLRUK = CResourceLRUK::ERR::errOutOfMemory;
                break;
            }

            RESMGRAssert( !plock->m_fHasLrukLock );
            m_ResourceLRUK.LockKeyPtr( tickIndexTarget, pres, &plock->m_lock );
            plock->m_fHasLrukLock = fTrue;

            pic->m_tickIndex = tickIndexTarget;
            errLRUK = m_ResourceLRUK.ErrInsertEntry( &plock->m_lock, pres, fTrue );

            if ( errLRUK != CResourceLRUK::ERR::errSuccess )
            {
                pic->m_tickIndex = TICK( tickUnindexed );
                cInsertionAttempts++;
                if ( cInsertionAttempts > 5 )
                {
                    RESMGRTrace( rmttResourceInsertStall, L"Insert Conflicts (%d) for %u @ %u, errLRUK = %d\n", cInsertionAttempts, tickIndexTarget, _TickCurrentTime(), errLRUK );
                }
            }
        }

        RESMGRAssert( plock->m_fHasLrukLock );
        m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
        plock->m_fHasLrukLock = fFalse;
    }
    while ( errLRUK == CResourceLRUK::ERR::errKeyRangeExceeded );

    RESMGRAssert( errLRUK == CResourceLRUK::ERR::errSuccess ||  errLRUK == CResourceLRUK::ERR::errOutOfMemory );  // expected, ErrCacheResource() originally and maintains an assert there as well
    RESMGRAssert( ( errLRUK == CResourceLRUK::ERR::errSuccess && pic->m_tickIndex != tickUnindexed ) ||
                    ( errLRUK != CResourceLRUK::ERR::errSuccess && pic->m_tickIndex == tickUnindexed ) );
    RESMGRAssert( !plock->m_fHasLrukLock );

    return errLRUK;
}

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_MarkForFastEviction( CInvasiveContext* const pic )
{
    CResourceLRUK::ERR   errLRUK;
    CResourceLRUK::CLock lockSource;
    CResourceLRUK::CLock lockDestination;
    CResource* const     pres           = _PresFromPic( pic );
    BOOL                 fLockedSrc     = fFalse;
    BOOL                 fLockedDst     = fFalse;

    m_statsFastEvict.cMarkForEvictionAttempted++;

    //
    //  1) Acquire the touch lock.
    //

    const TICK tickLastBIExpected = pic->TickLastTouchTime();
    RESMGRAssert( !_FTickLocked( tickLastBIExpected ) );

    const TICK tickLastAI = _TickSanitizeTickForTickLast( tickLastBIExpected ) | ftickResourceLocked;
    RESMGRAssert( _FTickLocked( tickLastAI ) );

    const TICK tickLastBI = AtomicCompareExchange( (LONG*)&pic->m_tickLast, tickLastBIExpected, tickLastAI );

    if ( tickLastBI != tickLastBIExpected )
    {
        //  we lost, that's fine (someone has saved this from a fast eviction death to be used).
        FE_STATISTICS( m_statsFastEvict.cFailureToMarkForEvictionBecauseFailedTouchLock++; );
        return;
    }

    RESMGRAssert( !_FTickLocked( tickLastBI ) );
    RESMGRAssert( tickLastBI == _TickLastTouchTime( tickLastAI ) );

    _SanityCheckTick( tickLastBI );     // successfully got touch lock, BI should be sane

    const BOOL fWasSuperColded = pic->FSuperColded();

    //
    //  2) Select target tick for super-cold.
    //

    //  now get the tickTarget for where we want to super-cold this page too
    //  we want a bucket that is 3 buckets below to keep this separate from regular touches.

    const TICK      tickInitial             = m_tickScanFirstFoundNormal;
    TICK            tickTarget              = _TickSanitizeTickForTickIndex( tickInitial - m_dtickSuperColdOffset );  //  note ftickResourceNormalTouch not OR'd in

    //  ok, we have the final target, now validate it matches all of our criteria

    _SanityCheckTick( tickTarget );
    RESMGRAssert( _FTickSuperColded( tickTarget ) );

    //
    //  3) check the current index to ensure we need to do something
    //

    //  now the pic should be stable ...
    //      (except that res scan / scavenging eviction could still grab it an evict it)

    const TICK      tickInitialIndex        = pic->m_tickIndex;

    //  ignore pics that are moving

    if ( tickInitialIndex == tickUnindexed )
    {
        FE_STATISTICS( m_statsFastEvict.cFailureToMarkForEvictionBecauseItHasNoBucket++; );
        goto HandleError;
    }

    //  ignore pics that are already in the right bucket. because of the wrap-around
    //  on index/tick values, we only check for the equality. 

    if ( m_ResourceLRUK.CmpKey( tickInitialIndex, tickTarget ) == 0 )
    {
        FE_STATISTICS( m_statsFastEvict.cAlreadyInTargetTick++; );
        m_statsFastEvict.cMarkForEvictionSucceeded++;
        goto HandleError;
    }

    RESMGRAssert( pic->m_tickLast == tickLastAI );  //  should still have the lock

    //
    //  4) update the index target, all other ticks are left intact
    //
    //  WARNING: Do not fail from here til after step 5
    //

    pic->m_tickIndexTarget = tickTarget;

    //
    //  5) update super cold stats
    //

    //  after this point, the resource is already marked as super cold with, no matter what ... so
    //  update the super colded resource count

    //  Note: we may not succeed at moving this resource between buckets, and in that case it will
    //  still be considered a super-colded resource, just "out of place".

    if ( !fWasSuperColded )
    {
        AtomicIncrement( (LONG*)&m_cSuperCold );
    }

    //
    //  6) get the various locks
    //


    //  try to lock the current bucket
    
    fLockedSrc = !_FFaultInj( rmfiFastEvictSourceLockFI ) &&
                 m_ResourceLRUK.FTryLockKeyPtr( tickInitialIndex,   // target bucket
                                                 pres,              // resource pointer
                                                &lockSource );      // lock state

    if ( !fLockedSrc )
    {
        FE_STATISTICS( m_statsFastEvict.cFailureToMarkForEvictionBecauseOfLockFailure++; );
        RESMGRTrace( rmttFastEvictContentions, L"Hit source bucket lock contention tickTarget = %u on source %p:%p{ [1] %u [2] %u, %u } ... @ %u!\n",
                        tickTarget, pres, pic, pic->m_rgtick[ 0 ], pic->m_rgtick[ 1 ], pic->m_tickIndex, _TickCurrentTime() );
        goto HandleError;
    }

    //  try to lock the destination bucket (note that by the magic of hashing)
    //  this may end up being the same as the original bucket; in which case; just fail.

    fLockedDst = !_FFaultInj( rmfiFastEvictDestInitialLockFI ) &&
                 m_ResourceLRUK.FTryLockKeyPtr( tickTarget,         // target bucket
                                                pres,               // resource pointer
                                                &lockDestination );     // lock state

    if ( !fLockedDst )
    {

        //  check if this is we are already in the right DHT bucket

        if ( lockDestination.IsSameBucketHead( &lockSource ) || _FFaultInj( rmfiFastEvictDestSrcBucketMatchFI ) )
        {
            FE_STATISTICS( m_statsFastEvict.cSuccessAlreadyInRightBucket++; );
            m_statsFastEvict.cMarkForEvictionSucceeded++;
            goto HandleError;
        }
        
        //  lock the next bucket. uncertainty is rounded up to nearest pow-of-two; 
        //  so (2 * dtick - 1) generates a value that will be at least one uncertainty
        //  range away whether dtick is pow-of-two or not.

        tickTarget     = _TickSanitizeTickForTickIndex( tickTarget + m_dtickUncertainty * 2 - 1 );  //  note ftickResourceNormalTouch not OR'd in
        _SanityCheckTick( tickTarget );
        RESMGRAssert( _FTickSuperColded( tickTarget ) );
#ifdef RESMGR_LOWBAR_CHECKING_FIXCREEP
        RESMGRAssert( _CmpTick( tickTarget, m_tickLowBar ) > 0 );
#endif

        fLockedDst = !_FFaultInj( rmfiFastEvictDestSecondaryLockFI ) &&
                     m_ResourceLRUK.FTryLockKeyPtr( tickTarget,         // target bucket
                                                        pres,               // resource pointer
                                                       &lockDestination);   // lock state
        if ( !fLockedDst )
        {
            RESMGRTrace( rmttFastEvictContentions, L"Hit double dst lock contention tickTarget = %u on %p:%p{ [1] %u [2] %u, %u } ... @ %u!\n",
                            tickTarget, pres, pic, pic->m_rgtick[ 0 ], pic->m_rgtick[ 1 ], pic->m_tickIndex, _TickCurrentTime() );

            FE_STATISTICS( m_statsFastEvict.cFailureToMarkForEvictionBecauseOfLockFailureOnDestination++; );
            goto HandleError;
        }

        FE_STATISTICS( m_statsFastEvict.cLockedSecondOldest++; );
    }

    if ( pic->m_tickIndex != tickInitialIndex || _FFaultInj( rmfiFastEvictIndexTickUnstableFI ) )
    {
        //  the resource has been moved while we were looking. 

        FE_STATISTICS(  m_statsFastEvict.cFailureToMarkForEvictionBecauseItMoved++; );
        goto HandleError;
    }
    
    RESMGRAssert( tickInitialIndex == pic->m_tickIndex );   // at this point, tickIndex should be guaranteed stable ...

    //
    //  7) move the resource to the super-cold bucket we locked
    //

    //  remove from current bucket

    RESMGRAssert( pic->m_tickIndex != tickUnindexed );
    errLRUK = m_ResourceLRUK.ErrDeleteEntry( &lockSource );
    RESMGRAssert( errLRUK == CResourceLRUK::ERR::errSuccess );

    //  try to move the resource to the target bucket. this should only fail
    //  if the bucket needs to be allocated and that allocation failed 
    //  b/c target tick is now outside of the valid range or due to an actual out of memory

    _SanityCheckTick( tickTarget );
    RESMGRAssert( _FTickSuperColded( tickTarget ) );
    RESMGRAssert( 0 == ( tickTarget & ftickResourceNormalTouch ) );     //  same, but double checking ...
    pic->m_tickIndex = tickTarget;
    _SanityCheckTickEx( pic->m_tickIndex, vtcAllowIndexKey );

    errLRUK = _FFaultInj( rmfiFastEvictInsertEntryFailsFI ) ?
              CResourceLRUK::ERR::errKeyRangeExceeded /* or could do OOM, but handled the same, so */ :
              m_ResourceLRUK.ErrInsertEntry( &lockDestination,  // Locked Bucket
                                              pres,             // Entry
                                              fFalse );         // Insert at End

    if ( errLRUK != CResourceLRUK::ERR::errSuccess )
    {
        //  put the resource back into its source bucket.  
        //  we know that we will succeed because the bucket existed before 
        //  and we haven't unlocked it

        RESMGRTrace( rmttFastEvictContentions, L"Hit bucket insert issue (%d) tickTarget = %u on %p:%p{ [1] %u [2] %u, %u } ... @ %u!\n",
                        errLRUK, tickTarget, pres, pic, pic->m_rgtick[ 0 ], pic->m_rgtick[ 1 ], pic->m_tickIndex, _TickCurrentTime() );
        pic->m_tickIndex = tickInitialIndex;
        _SanityCheckTickEx( pic->m_tickIndex, ValidateTickCheck( vtcAllowIndexKey | vtcAllowIndexKey ) );
        errLRUK = m_ResourceLRUK.ErrInsertEntry( &lockSource, pres, fTrue );
        RESMGRAssert( errLRUK == CResourceLRUK::ERR::errSuccess );
        RESMGRAssert( tickInitialIndex == pic->m_tickIndex );   // must have held.
        FE_STATISTICS(  m_statsFastEvict.cFailuseToInsertIntoTargetBucket++; );
    }
    else
    {
        m_statsFastEvict.cMarkForEvictionSucceeded++;
    }

HandleError:

    //  release the touch lock

#ifdef RESMGR_LOWBAR_CHECKING_FIXCREEP
    RESMGRAssert( _CmpTick( m_tickLowBar, tickLastBI ) <= 0 );
#endif
    RESMGRAssert( pic->FResourceLocked() );
    RESMGRAssert( !_FTickLocked( tickLastBI ) );
    OnDebug( const TICK tickCheck = ) AtomicExchange( (LONG*)&pic->m_tickLast, tickLastBI );
    RESMGRAssert( _FTickLocked( tickCheck ) );
    RESMGRAssert( tickCheck == tickLastAI );

    //  unlock the buckets

    if ( fLockedDst )
    {
        m_ResourceLRUK.UnlockKeyPtr( &lockDestination );
    }
    if ( fLockedSrc )
    {
        m_ResourceLRUK.UnlockKeyPtr( &lockSource );
    }
    return;
}

//  restores the touch history for the specified resource if known

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_RestoreHistory( const CKey& key, CInvasiveContext* const pic, const TICK tickNow, const WORD pctCachePriority, __out_opt BOOL * pfInHistory )
{
    //  _FResourceStaleForKeepingHistory() below already takes care of this case, but to avoid counting this as a request (when it shouldn't be, by design),
    //  we'll check only this condition here.

    if ( m_K < 2 )
    {
        return;
    }

    CHistoryTable::CLock    lockHist;
    CHistoryEntry           he;

    _SanityCheckTick( tickNow );

    //  this resource has history
    m_cHistoryReq++;

    m_KeyHistory.ReadLockKey( key, &lockHist );
    if ( m_KeyHistory.ErrRetrieveEntry( &lockHist, &he ) == CHistoryTable::ERR::errSuccess )
    {
        CHistory* const phist = he.m_phist;

        //  Often, very low cache priorities are used to effectively create a pool of resources used only transiently, like supercold, but
        //  less aggressive in terms of how quickly they can be disposed of. In these cases, they would potentially be promoted to double touch
        //  if they were to be re-cached with regular (100%) priority, which isn't what clients would expect. To help alleviate this effect, we'll
        //  test for staleness using the old priority too, which will tend to throw out history records for resources that were touched with very
        //  low priorities in the past.

        if ( !_FResourceStaleForKeepingHistory( phist->m_rgtick, phist->m_tickLast, phist->m_pctCachePriority, tickNow ) &&
            !_FResourceStaleForKeepingHistory( phist->m_rgtick, phist->m_tickLast, pctCachePriority, tickNow ) )
        {
            //  restore the history for this resource

            m_cHistoryHit++;
            *pfInHistory = fTrue;

            //  resource is locked at first
            pic->m_tickLast = _TickSanitizeTickForTickLast( phist->m_tickLast ) | ftickResourceLocked;
            _SanityCheckTick( pic->m_tickLast );
            RESMGRAssert( _FTickLocked( phist->m_tickLast ) );

            pic->m_tickIndex = TICK( tickUnindexed );
            for ( INT K = 1; K <= m_Kmax; K++ )
            {
                pic->m_rgtick[ K - 1 ] = phist->m_rgtick[ K - 1 ];
            }

            pic->m_pctCachePriority = pctCachePriority;
            pic->m_tickIndexTarget = _TickSanitizeTickForTickIndex( _ScaleTick( pic->m_rgtick[ m_K - 1 ], pic->m_pctCachePriority ) ) | ftickResourceNormalTouch;
            RESMGRTrace( rmttHistoryRestore, L"Restoring hist: %p { %u (0x%x), [] %u (0x%x) %u (0x%x), %u (0x%x), %hu }\n",
                            pic, pic->m_tickLast, pic->m_tickLast, pic->m_rgtick[0], pic->m_rgtick[0], pic->m_rgtick[1], pic->m_rgtick[1], pic->m_tickIndexTarget, pic->m_tickIndexTarget, pic->m_pctCachePriority );
        }
    }
    
    m_KeyHistory.ReadUnlockKey( &lockHist );
}

//  copies the touch history for the specified resource and locks the duplicated resource (picDst), which
//  will then get unlocked by the calling code once it is added to the LRU-K and released for consumption

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_CopyHistory( const CKey& key, CInvasiveContext* const picDst, const CInvasiveContext* const picSrc )
{
    _SanityCheckTick( picSrc->m_tickLast );

    // resource is locked at first  
    picDst->m_tickLast = _TickSanitizeTickForTickLast( picSrc->m_tickLast ) | ftickResourceLocked;
    _SanityCheckTick( picDst->m_tickLast );
    RESMGRAssert( picDst->FResourceLocked() );

    picDst->m_tickIndex = picSrc->m_tickIndex;
    picDst->m_tickIndexTarget = picSrc->m_tickIndexTarget;
    for ( INT K = 1; K <= m_Kmax; K++ )
    {
        picDst->m_rgtick[ K - 1 ] = picSrc->m_rgtick[ K - 1 ];
    }
    picDst->m_pctCachePriority = picSrc->m_pctCachePriority;

    if ( picDst->FSuperColded() )
    {
        //  we are making a copy of history of a super colded page, ensure we adjust our count.
        AtomicIncrement( (LONG*)&m_cSuperCold );
    }
}

//  stores the touch history for a resource

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_StoreHistory( const CKey& key, CInvasiveContext* const pic )
{
    if ( _FResourceStaleForKeepingHistory( pic->m_rgtick, pic->m_tickLast, pic->m_pctCachePriority, TickRESMGRTimeCurrent() ) )
    {
        return;
    }

    CHistory* const phist = _PhistAllocHistory( key );
    
    //  we allocated a history record

    if ( phist == NULL )
    {
        return;
    }

    //  we'll store it locked, just in case

    phist->m_tickLast = _TickSanitizeTickForTickLast( pic->m_tickLast ) | ftickResourceLocked;

    //  save the history for this resource

    phist->m_key = key;

    for ( INT K = 1; K <= m_Kmax; K++ )
    {
        phist->m_rgtick[ K - 1 ] = pic->m_rgtick[ K - 1 ];
    }

    phist->m_pctCachePriority = pic->m_pctCachePriority;

    //  we failed to insert the history in the history table

    if ( !_FInsertHistory( phist ) )
    {
        //  free the history record

        delete phist;

        //  we failed to allocate a history record; not the end of the world
    }
}

//  allocates a new history record to store the touch history for a resource.
//  this history record can either be newly allocated or recycled from the
//  history table

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::CHistory* CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_PhistAllocHistory( const CKey& key )
{
    CHistoryTable::ERR      errHist;
    CHistoryTable::CLock    lockHist;
    CHistoryEntry           he;

    CHistoryLRUK::ERR       errHLRUK;
    CHistoryLRUK::CLock     lockHLRUK;

    CHistory*               phist;

    //  remove the history for this resource from the history LRUK, if already
    //  present

    m_KeyHistory.WriteLockKey( key, &lockHist );
    errHist = m_KeyHistory.ErrRetrieveEntry( &lockHist, &he );
    if ( errHist == CHistoryTable::ERR::errSuccess )
    {
        //  remember the key and pointer for this history before we remove it
        //  from the history table

        const TICK      tickKey     = he.m_phist->m_rgtick[ m_K - 1 ];
        CHistory* const phistPtr    = he.m_phist;

        //  remove this history from the history table

        errHist = m_KeyHistory.ErrDeleteEntry( &lockHist );
        RESMGRAssert( errHist == CHistoryTable::ERR::errSuccess );
        
        //  try-lock the history LRUK.

        if ( ( errHist == CHistoryTable::ERR::errSuccess ) &&
                m_HistoryLRUK.FTryLockKeyPtr( tickKey, phistPtr, &lockHLRUK ) )
        {
            //  remove this history from the history LRUK, if present
            //
            //  NOTE:  we can only scavenge history from the history LRUK.  if this
            //  history is not present then we cannot return it!

            errHLRUK = m_HistoryLRUK.ErrDeleteEntry( &lockHLRUK );
            if ( errHLRUK == CHistoryLRUK::ERR::errSuccess )
            {
                AtomicDecrement( (LONG*)&m_cHistoryRecord );
                m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );
                m_KeyHistory.WriteUnlockKey( &lockHist );

                //  return this history

                return phistPtr;
            }

            m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );
        }
    }
    
    m_KeyHistory.WriteUnlockKey( &lockHist );

    //  get the first history in the history LRUK, if any

    m_HistoryLRUK.MoveBeforeFirst( &lockHLRUK );
    if ( ( errHLRUK = m_HistoryLRUK.ErrMoveNext( &lockHLRUK ) ) == CHistoryLRUK::ERR::errSuccess )
    {
        errHLRUK = m_HistoryLRUK.ErrRetrieveEntry( &lockHLRUK, &phist );
        RESMGRAssert( errHLRUK == CHistoryLRUK::ERR::errSuccess );

        if( errHLRUK == CHistoryLRUK::ERR::errSuccess )
        {
            //  if this history is no longer valid, recycle it
            
            if ( _FResourceStaleForKeepingHistory( phist->m_rgtick, phist->m_tickLast, phist->m_pctCachePriority, TickRESMGRTimeCurrent() ) &&
                m_KeyHistory.FTryWriteLockKey( phist->m_key, &lockHist ) )
            {
                //  remove this history from the history table, if present
                
                errHist = m_KeyHistory.ErrRetrieveEntry( &lockHist, &he );
                if ( errHist == CHistoryTable::ERR::errSuccess )
                {
                    //  only delete from the hash table if it points to this record
                    if ( he.m_phist == phist )
                    {
                        errHist = m_KeyHistory.ErrDeleteEntry( &lockHist );
                        RESMGRAssert( errHist == CHistoryTable::ERR::errSuccess );
                    }

                    if( errHist == CHistoryTable::ERR::errSuccess )
                    {
                        //  remove this history from the history LRUK

                        errHLRUK = m_HistoryLRUK.ErrDeleteEntry( &lockHLRUK );
                        RESMGRAssert( errHLRUK == CHistoryLRUK::ERR::errSuccess );

                        if( errHLRUK == CHistoryLRUK::ERR::errSuccess )
                        {
                            AtomicDecrement( (LONG*)&m_cHistoryRecord );
                            m_KeyHistory.WriteUnlockKey( &lockHist );
                            m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );

                            //  return this history

                            return phist;
                        }
                    }
                }
                
                m_KeyHistory.WriteUnlockKey( &lockHist );
            }
        }
    }
    
    m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );

    //  get the last history in the history LRUK, if any

    m_HistoryLRUK.MoveAfterLast( &lockHLRUK );
    if ( ( errHLRUK = m_HistoryLRUK.ErrMovePrev( &lockHLRUK ) ) == CHistoryLRUK::ERR::errSuccess )
    {
        errHLRUK = m_HistoryLRUK.ErrRetrieveEntry( &lockHLRUK, &phist );
        RESMGRAssert( errHLRUK == CHistoryLRUK::ERR::errSuccess );

        if( errHLRUK == CHistoryLRUK::ERR::errSuccess )
        {
            //  if this history is no longer valid, recycle it
            
            if ( _FResourceStaleForKeepingHistory( phist->m_rgtick, phist->m_tickLast, phist->m_pctCachePriority, TickRESMGRTimeCurrent() ) &&
                m_KeyHistory.FTryWriteLockKey( phist->m_key, &lockHist ) )
            {
                //  remove this history from the history table, if present
                
                errHist = m_KeyHistory.ErrRetrieveEntry( &lockHist, &he );
                if ( errHist == CHistoryTable::ERR::errSuccess )
                {
                    //  only delete from the hash table if it points to this record
                    if ( he.m_phist == phist )
                    {
                        errHist = m_KeyHistory.ErrDeleteEntry( &lockHist );
                        RESMGRAssert( errHist == CHistoryTable::ERR::errSuccess );
                    }

                    if( errHist == CHistoryTable::ERR::errSuccess )
                    {
                        //  remove this history from the history LRUK

                        errHLRUK = m_HistoryLRUK.ErrDeleteEntry( &lockHLRUK );
                        RESMGRAssert( errHLRUK == CHistoryLRUK::ERR::errSuccess );

                        if( errHLRUK == CHistoryLRUK::ERR::errSuccess )
                        {
                            AtomicDecrement( (LONG*)&m_cHistoryRecord );
                            m_KeyHistory.WriteUnlockKey( &lockHist );
                            m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );

                            //  return this history

                            return phist;
                        }
                    }
                }
                
                m_KeyHistory.WriteUnlockKey( &lockHist );
            }
        }
    }
    
    m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );

    //  allocate and return a new history because one couldn't be recycled

    return new CHistory;
}

//  inserts the history record containing the touch history for a resource into
//  the history table

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline BOOL CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_FInsertHistory( CHistory* const phist )
{
    CHistoryLRUK::ERR       errHLRUK;
    CHistoryLRUK::CLock     lockHLRUK;

    CHistoryEntry           he;

    CHistoryTable::ERR      errHist;
    CHistoryTable::CLock    lockHist;

    //  try to insert our history into the history LRUK
    //  we'll attempt to lock the history record in its hash table as well

    _SanityCheckTick( phist->m_rgtick[ m_K - 1 ] );

    m_HistoryLRUK.LockKeyPtr( phist->m_rgtick[ m_K - 1 ], phist, &lockHLRUK );

    if ( m_KeyHistory.FTryWriteLockKey( phist->m_key, &lockHist ) )
    {
        if ( ( errHLRUK = m_HistoryLRUK.ErrInsertEntry( &lockHLRUK, phist ) ) != CHistoryLRUK::ERR::errSuccess )
        {
            RESMGRAssert(   errHLRUK == CHistoryLRUK::ERR::errOutOfMemory ||
                            errHLRUK == CHistoryLRUK::ERR::errKeyRangeExceeded );

            //  we failed to insert our history into the history LRUK

            m_KeyHistory.WriteUnlockKey( &lockHist );
            m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );
            
            return fFalse;
        }
        
        AtomicIncrement( (LONG*)&m_cHistoryRecord );

        //  try to insert our history into the history table

        he.m_KeySignature   = CHistoryTable::CKeyEntry::Hash( phist->m_key );
        he.m_phist          = phist;

        if ( ( errHist = m_KeyHistory.ErrInsertEntry( &lockHist, he ) ) != CHistoryTable::ERR::errSuccess )
        {
            //  there is already an entry in the history table for this resource

            if ( errHist == CHistoryTable::ERR::errKeyDuplicate )
            {
                //  replace this entry with our entry.  note that this will make the
                //  old history that this entry pointed to invisible to the history
                //  table.  this is OK because we will eventually clean it up via the
                //  history LRUK

                errHist = m_KeyHistory.ErrReplaceEntry( &lockHist, he );
                RESMGRAssert( errHist == CHistoryTable::ERR::errSuccess );
            }

            //  some other error occurred while inserting this entry in the history table

            else
            {
                RESMGRAssert( errHist == CHistoryTable::ERR::errOutOfMemory );

                //  ignore the failure.  we don't want to bother removing the entry
                //  from the history LRUK.  don't return OOM because that will
                //  cause us to free the history record at a higher level.  this is
                //  allowed because we can have an entry in the history LRUK and
                //  not have an entry in the history table.  if this happens, we
                //  simply just cannot find the history by its key.  also, this is
                //  OK because we will eventually clean it up via the history LRUK
            }
        }
        
        m_KeyHistory.WriteUnlockKey( &lockHist );
    }
    else
    {
        m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );
        return fFalse;
    }
    
    m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );

    //  we have successfully stored our history

    return fTrue;
}

//  converts a pointer to the invasive context to a pointer to a resource

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CResource* CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_PresFromPic( CInvasiveContext* const pic ) const
{
    return (CResource*)( (BYTE*)pic - OffsetOfIC() );
}

//  converts a pointer to a resource to a pointer to the invasive context

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::CInvasiveContext* CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_PicFromPres( CResource* const pres ) const
{
    return (CInvasiveContext*)( (BYTE*)pres + OffsetOfIC() );
}

//  returns the current k-ness, given an array of ticks

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline ULONG CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_kLrukPool( const TICK* const rgtick )
{
    RESMGRAssert( 2 == m_Kmax );    //  expected, but should work for higher k

    ULONG K = m_Kmax;

    for ( ULONG k = 1; k < m_Kmax; k++ )
    {
        for ( ULONG l = 0; l < m_Kmax; l++ )
        {
            if ( k != l && rgtick[k] == rgtick[l] )
            {
                K--;
                break;
            }
        }
    }

    return K;
}

//  returns a valid current tick count

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TickCurrentTime()
{
    return _TickSanitizeTick( TickRESMGRTimeCurrent() );
}

//  returns the oldest estimate

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TickOldestEstimate()
{
    //  We could randomly alternate between different methods for estimating the oldest tick here,
    //  (for example, m_tickScanFirstEvictedIndexTarget), but that caused the side effect of some
    //  asserts going off because newer calculated index targets could be actually older depending on
    //  what was picked here. It is more important to keep those asserts than flipping the method.
    TICK tickOldestEstimate = *(volatile TICK*)&m_tickScanFirstEvictedIndexTargetHW;

    //  estimate should never be in the future
    RESMGRAssert( tickOldestEstimate != 0 );
    const TICK tickNow = _TickCurrentTime();
    tickOldestEstimate = ( _CmpTick( tickOldestEstimate, tickNow ) ) > 0 ? tickNow : tickOldestEstimate;

    return tickOldestEstimate;
}

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_ScaleTick( const TICK tickToScale, const WORD pctCachePriority )
{
    _SanityCheckTick( tickToScale );



    const TICK tickOldestEstimate = _TickOldestEstimate();

    //  dtick may be negative because tickOldestEstimateCaptured is only an estimate
    LONG dtick = tickToScale - tickOldestEstimate;

    //  clamp this to zero and treat as equal to oldest estimate.
    if ( dtick < 0 )
    {
        dtick = 0;
    }

    DWORD tickScaled = 0;

    if ( pctCachePriority == 100 )
    {
        //  We are already scaled
        tickScaled = tickOldestEstimate + (TICK)dtick;
        RESMGRAssert( ( _CmpTick( tickScaled, tickToScale ) == 0 ) || ( _CmpTick( tickToScale, tickOldestEstimate ) < 0 ) );
    }
    else
    {
        const __int64 dtickScaled = dtick * (__int64)pctCachePriority / (__int64)100;
        tickScaled = tickOldestEstimate + (TICK)dtickScaled;
    }

    tickScaled = _TickSanitizeTick( tickScaled );

    RESMGRTrace( rmttScaleTickAdjustments, L"_ScaleTick( %u, %hu ) -> %u - %u * pct + base -mod-> %u\n",
                    tickToScale, pctCachePriority, tickToScale, tickOldestEstimate, tickScaled );

    OnNonRTM( const ValidateTickCheck vtcAdjusted = pctCachePriority <= 100 ? vtcDefault : ValidateTickCheck( vtcDefault | vtcAllowIndexKey ) );
    _SanityCheckTickEx( tickScaled, vtcAdjusted );

    return tickScaled;
}

//  This function gives the resource a little boost if it detects that it's currently too close to the
//  eviction end of the LRU-K (within 10% of currelation interval). The boost sets the target resource
//  position to right after the "protected" window. This is a kind of soft pinning in that the actual
//  extra lifetime will not necessarily be exactly the interval the index was boosted by, since it'll
//  depend on the current resource churn. This was introduced to replace the harder kind of pinning
//  we had before, where we would test for the actual last tick timestamp and skip scavenging the resorce
//  for some period of time. That solution not only made the code complicated, but would also waste
//  CPU on small caches where we could scan pinned resources unnecessarily ver often.
//
//  The goal of the extra lifetime itself is to protect it from immediate eviction until the resource
//  gets a chance to be touched more times and eventually be promoted to doubly-touched if it's a hot
//  reource. It's particularly useful when we have a very small cache combined with a large correlation
//  interval.

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_AdjustIndexTargetForMinimumLifetime( CInvasiveContext* const pic )
{
    const TICK tickOldestEstimate = _TickOldestEstimate();
    const TICK tickIndexTargetCurrent = pic->TickIndexTargetTime();
    const LONG dtickLifetimeEstimate = _DtickDelta( tickOldestEstimate, tickIndexTargetCurrent );
    const TICK dtickMinimumLifetime = UlFunctionalMax( m_ctickCorrelatedTouch / 10, 1 );

    if ( dtickLifetimeEstimate < (LONG)dtickMinimumLifetime )
    {
        pic->m_tickIndexTarget = _TickSanitizeTickForTickIndex( tickOldestEstimate + dtickMinimumLifetime ) | ftickResourceNormalTouch;
    }
}


//  performs a wrap-around insensitive comparison of two TICKs

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline LONG CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_CmpTick( const TICK tick1, const TICK tick2 )
{
    return LONG( tick1 - tick2 );
}

//  performs a subtraction of two absolute tick counts

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline LONG CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_DtickDelta( const TICK tickEarlier, const TICK tickLater )
{
    return LONG( tickLater - tickEarlier );
}

//  Converts an externally provided cache priority (ULONG_PTR) into an internally
//  maintained cache priority with a lower range (WORD). Right now, no real scaling
//  is needed, but we might need it in the future if we ever need to use, for example,
//  an internal BYTE value to store cache priority (to save space in the invasive context).

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline WORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_AdjustCachePriority( const ULONG_PTR pctCachePriorityExternal )
{
    RESMGRAssert( FIsCachePriorityValid( pctCachePriorityExternal ) );

    //  Internally, we'll avoid accepting a cache priority of 0 to prevent the case where
    //  continuously caching/touching resources with priority 0 pins the oldest bucket in
    //  such a way that the LRU-K range grows indefinitely.
    C_ASSERT( FIsCachePriorityValid( 0 ) );
    C_ASSERT( FIsCachePriorityValid( 1 ) );
    if ( pctCachePriorityExternal == 0 )
    {
        return 1;
    }

    return (WORD)pctCachePriorityExternal;
}

//  Sanitizes external or internally calculated ticks to be stored in the ResMgr object.
//  The only value which is not allowed is zero.

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TickSanitizeTick( const TICK tick )
{
    return ( ( tick == 0 ) ? 1 : tick );
}

//  Sanitizes external or internally calculated ticks to be stored in tickLast-like variables.
//  The result has an unset ftickResourceLocked flag.
//  Additionally, the result must be such that, regardless of state of the flag we eventually set,
//  the result will not be 0.

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TickSanitizeTickForTickLast( TICK tick )
{
    C_ASSERT( ftickResourceLocked == 1 );
    
    tick = UlFunctionalMax( tick, ftickResourceLocked + 1 ) & tickTickLastMask;

    RESMGRAssert( tick > (TICK)ftickResourceLocked );
    RESMGRAssert( ( tick & tickTickLastMask ) != 0 );
    RESMGRAssert( ( tick & ~tickTickLastMask ) == 0 );
    RESMGRAssert( !_FTickLocked( tick ) );

    return tick;
}

//  Extracts the time portion of the tickLast-like variable, i.e., strips off ftickResourceLocked.

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TickLastTouchTime( const TICK tick )
{
    return ( tick & tickTickLastMask );
}

//  Whether or not the resource is locked.

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline BOOL CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_FTickLocked( const TICK tick )
{
    return ( ( tick & ftickResourceLocked ) != 0 );
}

//  Sanitizes external or internally calculated ticks to be stored in tickIndex-like variables.
//  The result has an unset ftickResourceNormalTouch flag.
//  Additionally, the result must be such that, regardless of state of the flag we eventually set,
//  the result will not be 0 or tickUnindexed.

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TickSanitizeTickForTickIndex( TICK tick )
{
    C_ASSERT( ftickResourceNormalTouch == 1 );
    C_ASSERT( tickUnindexed == 0xFFFFFFFF );

    //  The -2 in the upper bound calculation is to handle the case where setting the ftickResourceNormalTouch flag on the tick would result
    //  in tickUnindexed.

    tick = UlBound( tick, (TICK)ftickResourceNormalTouch + 1, (TICK)tickUnindexed - 2 ) & tickTickIndexMask;

    RESMGRAssert( tick > (TICK)ftickResourceNormalTouch );
    RESMGRAssert( tick < (TICK)tickUnindexed );
    RESMGRAssert( ( tick & tickTickIndexMask ) != 0 );
    RESMGRAssert( ( tick & ~tickTickIndexMask ) == 0 );
    RESMGRAssert( _FTickSuperColded( tick ) );  //  not really supercolded, but because we have sanitized the input without adding any flags, this tick appears supercolded

    return tick;
}

//  Extracts the time portion of the tickIndex-like variable, i.e., strips off ftickResourceNormalTouch.

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TickIndexTime( const TICK tick )
{
    return ( tick & tickTickIndexMask );
}

//  Whether or not the resource is supercolded (only applicable to tickIndex-like variables, e.g., m_tickIndex and m_tickIndexTarget).

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline BOOL CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_FTickSuperColded( const TICK tick )
{
    return ( ( tick & ftickResourceNormalTouch ) == 0 );
}

//  Implements a heuristic to determine whether or not a resource or resource history looks stale or otherwise invalid
//  to be kept in the ResMgr.

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline BOOL CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_FResourceStaleForKeepingHistory( const TICK* const rgtick, const TICK tickLast, const WORD pctCachePriority, const TICK tickNow )
{
    //  it doesn't make sense to keep history if we're using LRU-1 because the restored history will either
    //  be clobbered immediately by the upcoming touch or it'll be considered a correlated touch, in which case
    //  the resource will remain with an even older tick and be targeted for eviction sooner.

    if ( m_K < 2 )
    {
        return fTrue;
    }

    //  discard any history records for which the ticks to be evaluated later in this function are older than the oldest estimate
    //  because scaling such ticks is not reliable since we don't have the original oldest estimate when they were generated, so
    //  scaling them would essentially return the oldest estimate itself

    const TICK tickOldestEstimate = _TickOldestEstimate();

    //  the next timestamp to be evaluated (K - 1, e.g., 4th tick on an LRU-5 manager)

    const TICK tickIndexTargetNextEstimate = _TickSanitizeTick( rgtick[ ( m_K - 1 ) - 1 ] + m_ctickTimeout );
    if ( _CmpTick( tickIndexTargetNextEstimate, tickOldestEstimate ) < 0 )
    {
        return fTrue;
    }

    //  the most significant tick in the array

    const ULONG K = _kLrukPool( rgtick );
    const TICK tickMostSignificant = _TickSanitizeTick( rgtick[ K - 1 ] - ( m_ctickTimeout + m_ctickCorrelatedTouch ) * ( K - 1 ) );
    if ( _CmpTick( tickMostSignificant, tickOldestEstimate ) < 0 )
    {
        return fTrue;
    }

    //  consider stale a resource for which the next timestamp to be evaluated is older than the oldest normal/non-supercolded
    //  resource in the LRU-K, after undergoing an additional touch (i.e., a boost, which is likely to happen if the resource is recovered and touched).

    if ( _CmpTick( _ScaleTick( tickIndexTargetNextEstimate, pctCachePriority ), m_tickScanFirstFoundNormal ) < 0 )
    {
        return fTrue;
    }

    //  check for resources which could have ticks that appear to be in the future, which may happen if the resource
    //  has been evicted so long ago that the ticks have wrapped. we only need to check the oldest tick.

    if ( _CmpTick( _ScaleTick( tickMostSignificant, pctCachePriority ), tickNow ) > 0 )
    {
        return fTrue;
    }

    return fFalse;
}

#ifndef RTM

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_SanityCheckTick_( const TICK tick, const ValidateTickCheck vtc, const CHAR * szTickName ) const
{
    //  this may need a flag if we have a valid 0 variable
    RESMGRAssert( tick != 0 );

    if ( m_tickLowBar && !( vtc & vtcAllowIndexKey ) )
    {
        RESMGRAssert( _CmpTick( m_tickLowBar - m_ctickTimeout, tick ) <= 0 || ( vtc & vtcAllowIndexKey ) );
    }

    if ( m_tickHighBar )
    {
        RESMGRAssert( _CmpTick( tick, m_tickHighBar ) <= 0 || ( vtc & vtcAllowIndexKey ) );
    }

    if ( m_dtickCacheLifetimeBar && !( vtc & vtcAllowIndexKey ) )
    {
        //  ticks should be in the same numerical scale.
        RESMGRAssert( _DtickDelta( tick, _TickCurrentTime() ) < (LONG)m_dtickCacheLifetimeBar );
    }

}

#endif  // !RTM

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_ValidateTickProgress( __in_z const WCHAR * szStateVarName, const TICK tickStoredState, const TICK tickNew, const ValidateTickProgress vtp )
{
#ifndef RTM
    const RESMGRTraceTag rmttEff = ( vtpNoTracing & vtp ) ? rmttNone : rmttTickUpdateJumps;
#endif

    //  like DtickDelta(), 
    //      plus means counter is jumping fwd (usually fine), and
    //      negative means jumping backwards (usually, but not always bad).
    
    INT dtickUpdate = INT( tickNew - tickStoredState );

    const INT dtickTwoBuckets = _LNextPowerOf2( m_dtickUncertainty ) * 2 + 1;
    if ( vtpBucketBackwardCheck & vtp )
    {
        RESMGRAssert( dtickUpdate > -dtickTwoBuckets );
    }
    if ( vtpBucketForwardCheck & vtp )
    {
        RESMGRAssert( dtickUpdate < dtickTwoBuckets );
    }

#ifdef RESMGR_LOWBAR_CHECKING_FIXCREEP
    //  do we need a vtp flag to avoid?  Should not.

    if ( m_tickLowBar )
    {
        RESMGRAssert( _CmpTick( tickNew, m_tickLowBar ) >= 0 );
    }
#endif

    if ( tickStoredState != 0 )
    {
        if ( dtickUpdate > ( 2 * dtickTwoBuckets ) ||
                dtickUpdate < -( 2 * dtickTwoBuckets ) )
        {
            //  trace or assert something ...

            OnDebug( C_ASSERT( vtpDisableAssert == 0x1 ) ); //  fixing value, but want it private to debug.
            const BOOL fAssertable = !( vtp & ValidateTickProgress( 0x1 ) );

            if ( vtp & vtpSuperColdBackwardCheck && dtickUpdate < -( 2 * dtickTwoBuckets + (INT)m_dtickSuperColdOffsetConcurrentMax ) )
            {
                //  significant negative adjustment, this is the bad kind of update as some of the "oldest estimate" type
                //  variables should only be moving forwards
                RESMGRTrace( rmttEff, L"\t\tERRR: Significant %ws Movement from %u(0x%x) %hs%d to %u(0x%x) ... (ERRR: too far negative / backwards adjustment!)\n",
                                    szStateVarName, tickStoredState, tickStoredState, INT(dtickUpdate) >= 0 ? "+" : "", dtickUpdate, tickNew, tickNew );
                if ( fAssertable )
                {
                }
            }
            else if ( vtp & vtpConcurrentBackwardCheck && dtickUpdate < -( 2 * dtickTwoBuckets ) )
            {
                //  significant negative adjustment, this is the bad kind of update as some of the "oldest estimate" type
                //  variables should only be moving forwards
                RESMGRTrace( rmttEff, L"\t\tERRR: Significant %ws Movement from %u(0x%x) %hs%d to %u(0x%x) ... (ERRR: too far negative / backwards adjustment!)\n",
                                    szStateVarName, tickStoredState, tickStoredState, INT(dtickUpdate) >= 0 ? "+" : "", dtickUpdate, tickNew, tickNew );
                if ( fAssertable )
                {
                }
            }
            else if ( vtp & vtpSmoothForward && dtickUpdate > ( 4 * dtickTwoBuckets ) )
            {
                RESMGRTrace( rmttEff, L"\t\tWARN: Significant %ws Movement from %u(0x%x) %hs%d to %u(0x%x) ... (WARN: too far forward jump.)\n",
                                    szStateVarName, tickStoredState, tickStoredState, INT(dtickUpdate) >= 0 ? "+" : "", dtickUpdate, tickNew, tickNew );
                if ( fAssertable )
                {
                    //  Tthere is no case that we can actually assert here ... a client is always free to 
                    //  just stop calling ESE for awhile.  However, certain tests we know have a smooth
                    //  going forward pattern and we can Assert for those tests.  For now I just use this
                    //  manually, but consider test hooking the checks.
                    RESMGRTrace( rmttEff, L"BREAKING On %ws jump too far forwards.\n", szStateVarName );
                    //  useful for debugging, but ultimately never defendable as the client app can just not
                    //  touch things for awhile, then become active again ... this will always cause a large
                    //  unexpected shift forward.  All usage of vtpSmoothForward flag has been removed, but
                    //  leaving the capability.
                    RESMGRAssert( fFalse );
                }
            }
            else
            {
                RESMGRTrace( rmttEff, L"\t\tINFO: Significant %ws Movement from %u(0x%x) %hs%d to %u(0x%x) ... (%ws)\n",
                                    szStateVarName, tickStoredState, tickStoredState, INT(dtickUpdate) >= 0 ? "+" : "", dtickUpdate, tickNew, tickNew, dtickUpdate < 0 ? L"backwards" : L"forwards" );
            }
        }
    }
}


#define DECLARE_LRUK_RESOURCE_UTILITY_MANAGER( m_Kmax, CResource, OffsetOfIC, CKey, Typedef )                                   \
                                                                                                                                \
typedef CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey > Typedef;                                             \
                                                                                                                                \
DECLARE_APPROXIMATE_INDEX( Typedef::TICK, CResource, Typedef::CInvasiveContext::OffsetOfAIIC, Typedef##__m_aiResourceLRUK );    \
                                                                                                                                \
DECLARE_APPROXIMATE_INDEX( Typedef::TICK, Typedef::CHistory, Typedef::CHistory::OffsetOfAIIC, Typedef##__m_aiHistoryLRU );      \
                                                                                                                                \
inline ULONG_PTR Typedef::CHistoryTable::CKeyEntry::                                                                            \
Hash() const                                                                                                                    \
{                                                                                                                               \
    return ULONG_PTR( m_entry.m_KeySignature );                                                                                 \
}                                                                                                                               \
                                                                                                                                \
inline BOOL Typedef::CHistoryTable::CKeyEntry::                                                                                 \
FEntryMatchesKey( const CKey& key ) const                                                                                       \
{                                                                                                                               \
    return Hash() == Hash( key ) && m_entry.m_phist->m_key == key;                                                              \
}                                                                                                                               \
                                                                                                                                \
inline void Typedef::CHistoryTable::CKeyEntry::                                                                                 \
SetEntry( const Typedef::CHistoryEntry& he )                                                                                    \
{                                                                                                                               \
    m_entry = he;                                                                                                               \
}                                                                                                                               \
                                                                                                                                \
inline void Typedef::CHistoryTable::CKeyEntry::                                                                                 \
GetEntry( Typedef::CHistoryEntry* const phe ) const                                                                             \
{                                                                                                                               \
    *phe = m_entry;                                                                                                             \
}                                                                                                                               \
                                                                                                                                \
template<>                                                                                                                      \
inline ULONG_PTR Typedef::CHistoryTable::CKeyEntry::                                                                            \
Hash( const CKey& key )                                                                                                         \
{                                                                                                                               \
    return key.Hash();                                                                                                          \
}


NAMESPACE_END( RESMGR )


using namespace RESMGR;


#endif  //  _RESMGR_HXX_INCLUDED
