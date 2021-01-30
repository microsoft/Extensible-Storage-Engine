// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#ifndef _RESMGR_HXX_INCLUDED
#define _RESMGR_HXX_INCLUDED



#ifdef RESMGRAssert
#else
#define RESMGRAssert    Assert
#endif


#ifdef COLLAssert
#else
#define COLLAssert RESMGRAssert
#endif


#define RESSCAN_STATISTICS                      1
#define FASTEVICT_STATISTICS                    1


#ifdef FASTEVICT_STATISTICS
#define FE_STATISTICS(x) x
#else
#define FE_STATISTICS(x)
#endif

#ifdef RESSCAN_STATISTICS
#define RS_STATISTICS(x) x
#else
#define RS_STATISTICS(x)
#endif



#ifdef TickRESMGRTimeCurrent
#else
#define TickRESMGRTimeCurrent TickOSTimeCurrent
#endif

#include "stat.hxx"
#include "collection.hxx"

#include <math.h>


NAMESPACE_BEGIN( RESMGR );



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
#pragma warning(suppress: 22103)
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


        double  m_dcbTotalResource;
};


template< size_t m_cAvgDepth >
inline CDBAResourceAllocationManager< m_cAvgDepth >::CDBAResourceAllocationManager()
{
}


template< size_t m_cAvgDepth >
inline CDBAResourceAllocationManager< m_cAvgDepth >::~CDBAResourceAllocationManager()
{
}


template< size_t m_cAvgDepth >
inline void CDBAResourceAllocationManager< m_cAvgDepth >::ResetStatistics()
{

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


template< size_t m_cAvgDepth >
inline void CDBAResourceAllocationManager< m_cAvgDepth >::UpdateStatistics()
{
    
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



    const double    k1                  = 1.0;
    const double    cbAvailMemory       = (double)cbAvailablePhysicalMemory;
    const double    dcbResourceEvict    = (double)dcbResourcesEvicted;
    const double    k2                  = 1.0;
    const double    cbResourcePool      = (double)cbTotalResources;
    const double    dcbMemoryEvict      = (double)dcbPhysicalMemoryEvicted;
    const double    cbTotalMemory       = (double)cbTotalPhysicalMemory;


    const double dcbTotalResource       =   (
                                                k1 * cbAvailMemory * dcbResourceEvict -
                                                k2 * cbResourcePool * dcbMemoryEvict
                                            ) / cbTotalMemory;


    const double dcbTotalResourceMax = cbTotalMemory * (double)m_dpctResourceMax / 100.0;

    RESMGRAssert( dcbTotalResourceMax > 0.0 );

    if ( dcbTotalResource < 0.0 )
    {

        m_dcbTotalResource += max( dcbTotalResource, -dcbTotalResourceMax );
    }
    else
    {

        m_dcbTotalResource += min( dcbTotalResource, dcbTotalResourceMax );
    }
}


template< size_t m_cAvgDepth >
inline void CDBAResourceAllocationManager< m_cAvgDepth >::ConsumeResourceAdjustments( __out double * const pdcbTotalResource, __in const double cbResourceSize )
{
    const __int64 dcbTotalResource = (__int64)m_dcbTotalResource;
    *pdcbTotalResource = (double)rounddn( dcbTotalResource, (__int64)cbResourceSize );
    m_dcbTotalResource -= *pdcbTotalResource;
}




template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
class CLRUKResourceUtilityManager
{
    public:


        typedef DWORD TICK;

        enum { tickUnindexed        = ~TICK( 0 ) };

        enum { ftickResourceLocked  = 0x1 };
        enum { tickTickLastMask     = ~TICK( 0 ) ^ TICK( ftickResourceLocked ) };

        enum { ftickResourceNormalTouch = 0x1 };
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


        class CInvasiveContext
        {
            public:

                CInvasiveContext() {}
                ~CInvasiveContext() {}

                static SIZE_T OffsetOfILE() { return OffsetOfIC() + OffsetOf( CInvasiveContext, m_aiic ); }
                static SIZE_T OffsetOfAIIC() { return OffsetOfIC() + OffsetOf( CInvasiveContext, m_aiic ); }

                TICK TickKthTouch( const INT k ) const          { RESMGRAssert( k >= 1 ); return m_rgtick[ k - 1 ]; }
                TICK TickIndexTarget() const                    { return m_tickIndexTarget; }
                TICK TickLastTouchTime() const                  { return _TickLastTouchTime( m_tickLast ); }
                TICK TickIndexTargetTime() const                { return _TickIndexTime( m_tickIndexTarget ); }
                BOOL FCorrelatedTouch() const                   { return TickLastTouchTime() != _TickSanitizeTickForTickLast( m_rgtick[0] ); }
                BOOL FSuperColded() const                       { return _FTickSuperColded( m_tickIndexTarget ); }
                BOOL FResourceLocked() const                    { return _FTickLocked( m_tickLast ); }

#ifdef DEBUGGER_EXTENSION
                TICK TickLastTouch() const                      { return m_tickLast; }
                TICK TickIndex() const                          { return m_tickIndex; }
                TICK TickIndexTime() const                      { return _TickIndexTime( m_tickIndex ); }
                BOOL FSuperColdedIndex() const                  { return _FTickSuperColded( m_tickIndex ); }
                WORD PctCachePriority() const                   { return m_pctCachePriority; }
#endif

#ifdef MINIMAL_FUNCTIONALITY
            private:
#endif
                ULONG kLrukPool() const
                {
                    return _kLrukPool( m_rgtick );
                }

            private:

                friend class CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >;

                typename CApproximateIndex< TICK, CResource, OffsetOfAIIC >::CInvasiveContext   m_aiic;
                TICK                                                                            m_tickIndex;
                TICK                                                                            m_tickIndexTarget;
                TICK                                                                            m_tickLast;
                TICK                                                                            m_rgtick[ m_Kmax ];
                WORD                                                                            m_pctCachePriority;
        };


        enum class ERR
        {
            errSuccess,
            errInvalidParameter,
            errOutOfMemory,
            errResourceNotCached,
            errNoCurrentResource,
        };


        class CLock;

    public:


        CLRUKResourceUtilityManager( const INT Rank );
        ~CLRUKResourceUtilityManager();


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


        class CHistory
        {
                public:

                CHistory() {}
                ~CHistory() {}

#ifdef CRESMGR_H_INCLUDED
#pragma push_macro( "new" )
#undef new
            private:
                void* operator new[]( size_t );
                void operator delete[]( void* );
            public:
                void* operator new( size_t cbAlloc )
                {
                    RESMGRAssert( cbAlloc == sizeof( BFLRUK::CHistory ) );
                    return RESLRUKHIST.PvRESAlloc_( SzNewFile(), UlNewLine() );
                }
                void operator delete( void* pv )
                {
                    RESLRUKHIST.Free( pv );
                }
#pragma pop_macro( "new" )
#endif

                static SIZE_T OffsetOfAIIC() { return OffsetOf( CHistory, m_aiic ); }

            public:

                typename CApproximateIndex< TICK, CHistory, OffsetOfAIIC >::CInvasiveContext    m_aiic;
                CKey                                                                            m_key;
                TICK                                                                            m_tickLast;
                TICK                                                                            m_rgtick[ m_Kmax ];
                WORD                                                                            m_pctCachePriority;
        };

    private:



        typedef CApproximateIndex< TICK, CResource, CInvasiveContext::OffsetOfAIIC > CResourceLRUK;
        typedef typename CResourceLRUK::ERR ERR_RES_APPIDX;


        typedef CApproximateIndex< TICK, CHistory, CHistory::OffsetOfAIIC > CHistoryLRUK;


        typedef CInvasiveList< CResource, CInvasiveContext::OffsetOfILE > CUpdateList;


        typedef CInvasiveList< CResource, CInvasiveContext::OffsetOfILE > CStuckList;

    public:


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
#endif


                        TICK                m_tickScanFirstFoundAll;
                        TICK                m_tickScanFirstFoundNormal;
                        TICK                m_tickScanLastFound;

                        friend class CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >;


                        VOID UpdateFoundTimes( CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey > * const presmgr, const CInvasiveContext * const pic )
                        {
                            const TICK tickNow = _TickCurrentTime();


                            RESMGRAssert( !pic->FResourceLocked() );

                            const TICK tickFoundUpdate = pic->TickIndexTargetTime();
                            const BOOL fSuperColded = pic->FSuperColded();


                            const BOOL fOutOfPlace = presmgr->m_ResourceLRUK.CmpKey( pic->m_tickIndexTarget, pic->m_tickIndex - presmgr->m_dtickUncertainty * 2 ) < 0;

                            if ( fOutOfPlace )
                            {
                                return;
                            }

                            if ( ( m_tickScanFirstFoundAll == 0 ) || ( _CmpTick( tickFoundUpdate, m_tickScanFirstFoundAll ) < 0 ) )
                            {
                                presmgr->_ValidateTickProgress( L"LOCK-SFFA", m_tickScanFirstFoundAll, tickFoundUpdate, vtpSuperColdBackwardCheck );
                                presmgr->_ValidateTickProgress( L"GLOB-SFFA", presmgr->m_tickScanFirstFoundAll, tickFoundUpdate, vtpSuperColdBackwardCheck );

                                m_tickScanFirstFoundAll = tickFoundUpdate;
                            }

                            if ( !fSuperColded )
                            {
                                if ( ( m_tickScanFirstFoundNormal == 0 ) || ( _CmpTick( tickFoundUpdate, m_tickScanFirstFoundNormal ) < 0 ) )
                                {

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
#endif

                        TICK                m_tickScanFirstEvictedTouchK1;
                        TICK                m_tickScanFirstEvictedTouchK2;
                        TICK                m_tickScanFirstEvictedIndexTarget;


                        VOID UpdateEvictTimes( CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey > * const presmgr, const CInvasiveContext * const pic )
                        {

                            const BOOL fSuperColded = pic->FSuperColded();
                            const BOOL fOutOfPlace = presmgr->m_ResourceLRUK.CmpKey( pic->m_tickIndexTarget, pic->m_tickIndex - presmgr->m_dtickUncertainty * 2 ) < 0;

                            if ( fSuperColded || fOutOfPlace )
                            {
                                return;
                            }

                            if ( pic->kLrukPool() == 1 || presmgr->m_K == 1  )
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
                                RESMGRAssert( fFalse );
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
            vtpSmoothForward            = 0x2,
            vtpBucketBackwardCheck      = 0x4,
            vtpConcurrentBackwardCheck  = 0x8,
            vtpSuperColdBackwardCheck   = 0x10,
            vtpBucketForwardCheck       = 0x20,

            vtpNoTracing                = 0x80000000,
        } ValidateTickProgress;

#ifdef RTM
        #define _SanityCheckTick( tick )
        #define _SanityCheckTickEx( tick, vtc )
#else
        #define _SanityCheckTick( tick )            _SanityCheckTick_( tick, vtcDefault, #tick )
        #define _SanityCheckTickEx( tick, vtc )     _SanityCheckTick_( tick, vtc, #tick )
        void _SanityCheckTick_( const TICK tick, const ValidateTickCheck vtc, const CHAR * szTickName ) const;
#endif
        void _ValidateTickProgress( __in_z const WCHAR * szStateVarName, const TICK tickStoredState, const TICK tickNew, const ValidateTickProgress vtp );

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
        enum RESMGRTraceTag
        {
            rmttNone                        = 0x0,
#ifdef DEBUG
            rmttYes                         = 0x1,
#endif
            rmttInitTerm                    = 0x2,
            rmttScanStats                   = 0x4,
            rmttScanProcessing              = 0x8,
            rmttTickUpdateJumps             = 0x10,
            rmttFastEvictContentions        = 0x20,
            rmttHistoryRestore              = 0x40,
            rmttScaleTickAdjustments        = 0x80,
            rmttResourceInsertStall         = 0x100,
        };
        const static ULONG m_cTraceAllOn    = 0xffffffff;

        #define RESMGRTrace( rmtt, ... )        \
            if ( ( m_rmttControl & rmtt && m_cTracesTTL == m_cTraceAllOn ) ||   \
                    ( m_rmttControl & rmtt && m_cTracesTTL )                    \
                    OnDebug( || ( rmtt & rmttYes  ) ) )      \
            {                                   \
                wprintf( __VA_ARGS__ );         \
                m_cTracesTTL--;                 \
            }
#else
        #define RESMGRTrace( rmtt, ... )
#endif

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


        BOOL            m_fInitialized;
        DWORD           m_K;
        TICK            m_ctickCorrelatedTouch;
        TICK            m_ctickTimeout;
        TICK            m_dtickUncertainty;
        TICK            m_dtickLRUKPrecision;
        TICK            m_dtickSuperColdOffset;

        TICK            m_tickStart;
        TICK            m_tickLowBar;
        TICK            m_tickHighBar;
        TICK            m_dtickCacheLifetimeBar;
        BYTE            m_rgbReserved1[ 20 ];


        volatile DWORD  m_cHistoryRecord;
        volatile DWORD  m_cHistoryHit;
        volatile DWORD  m_cHistoryReq;

        volatile DWORD  m_cResourceScanned;
        volatile DWORD  m_cResourceScannedMoves;
        volatile DWORD  m_cResourceScannedOutOfOrder;

        volatile LONG   m_cSuperCold;

#ifdef DEBUG
        enum RESMGRFaultInj
        {
            rmfiNone                            = 0x0,
            rmfiScanTouchConcurrentFI           = 0x1,
            rmfiFastEvictSourceLockFI           = 0x2,
            rmfiFastEvictDestInitialLockFI      = 0x4,
            rmfiFastEvictDestSrcBucketMatchFI   = 0x8,
            rmfiFastEvictDestSecondaryLockFI    = 0x10,
            rmfiFastEvictIndexTickUnstableFI    = 0x20,
            rmfiFastEvictInsertEntryFailsFI     = 0x40,

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

        TICK            m_tickScanFirstFoundAll;
        TICK            m_tickScanFirstFoundNormal;
        TICK            m_tickScanLastFound;
        TICK            m_tickLatestScanFirstFoundNormalUpdate;

        TICK            m_tickScanFirstEvictedTouchK1;
        TICK            m_tickScanFirstEvictedTouchK2;
        TICK            m_tickScanFirstEvictedIndexTarget;
        TICK            m_tickScanFirstEvictedIndexTargetHW;
        TICK            m_tickScanFirstFoundAllIndexTargetHW;
        TICK            m_tickScanFirstFoundNormalIndexTargetHW;
        TICK            m_tickLatestScanFirstEvictedIndexTargetUpdate;
        TICK            m_tickLatestScanFirstEvictedIndexTargetHWUpdate;


        LONG            m_cLastScanEnumeratedEntries;
        LONG            m_cLastScanBucketsScanned;
        LONG            m_cLastScanEmptyBucketsScanned;
        LONG            m_cLastScanEnumeratedIDRange;
        TICK            m_dtickLastScanEnumeratedRange;

#ifdef DEBUG
        DWORD           m_tidLockCheck;
#endif


        CHistoryLRUK    m_HistoryLRUK;
        CHistoryTable   m_KeyHistory;


        CResourceLRUK   m_ResourceLRUK;


        CResScanLockStats       m_statsScanLast;
        CResScanLockStats       m_statsScanPrev;
        CFastEvictStatistics    m_statsFastEvict;
};


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


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
~CLRUKResourceUtilityManager()
{
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
SetTimeBar( const TICK dtickLifetimeBar, const TICK dtickHighBar )
{
    m_tickHighBar = dtickHighBar;
    m_dtickCacheLifetimeBar = dtickLifetimeBar;
}

#ifdef DEBUG

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
SetFaultInjection( const DWORD grbit )
{
    m_grbitFI = (RESMGRFaultInj)grbit;
}
#endif


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

    if (    K < 1 || K > m_Kmax ||
            csecCorrelatedTouch < 0 ||
            csecTimeout < 0 ||
            dblHashLoadFactor < 0 ||
            dblHashUniformity < 1.0 )
    {
        return ERR::errInvalidParameter;
    }


    m_K                     = DWORD( K );
    m_ctickCorrelatedTouch  = TICK( csecCorrelatedTouch * 1000.0 );
    m_ctickTimeout          = TICK( csecTimeout * 1000.0 );
    m_dtickUncertainty      = TICK( max( csecUncertainty * 1000.0, 2.0 ) );

    m_dtickSuperColdOffset  = ( _LNextPowerOf2( m_dtickUncertainty ) * 3 + 1 );



    m_cHistoryRecord    = 0;
    m_cHistoryHit       = 0;
    m_cHistoryReq       = 0;

    m_cResourceScanned              = 0;
    m_cResourceScannedMoves         = 0;
    m_cResourceScannedOutOfOrder    = 0;

#ifdef DEBUG

    m_tidLockCheck      = 0;



    m_grbitFI           =  rmfiNone;
#endif

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


    m_tickLowBar = 0;

#ifdef RESMGR_ENABLE_TICK_VALIDATION
    if ( m_tickHighBar == 0 && m_dtickCacheLifetimeBar == 0 )
    {
        SetTimeBar( 90 * 60 * 1000 , 10 * 60 * 60 * 1000  );
    }
#endif
    m_tickLowBar = tickStart - m_dtickSuperColdOffsetConcurrentMax;
    if ( m_tickHighBar )
    {
        m_tickHighBar += tickStart;
        RESMGRTrace( rmttInitTerm, L"\tBar Testing is Enabled: LowBar = %u (0x%x), HighBar = %u (0x%x), LifetimeBar/Range: %u (%u:%02u:%02u.%03u) \n",
                        m_tickLowBar, m_tickLowBar, m_tickHighBar, m_tickHighBar, m_dtickCacheLifetimeBar,
                        m_dtickCacheLifetimeBar / 1000 / 60 / 60, m_dtickCacheLifetimeBar / 1000 / 60 % 60, m_dtickCacheLifetimeBar / 1000 % 60, m_dtickCacheLifetimeBar % 1000 );
    }



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

#ifdef DEBUG
    m_dtickLRUKPrecision = (DWORD)( _LNextPowerOf2(
                                max( min( 2 * m_K * m_ctickTimeout + 2,
                                          INT_MAX / 2 ),
                                      8 * 60 * 1000 ) ) - 1 );
#else
#ifdef RTM
    m_dtickLRUKPrecision = ~TICK( 0 );
#else
    m_dtickLRUKPrecision = 0x10000000 - 1;
#endif
#endif

    RESMGRAssert( m_dtickLRUKPrecision >= 2 * m_K * m_ctickTimeout );
    RESMGRAssert( (DWORD)_LNextPowerOf2( m_dtickLRUKPrecision ) == m_dtickLRUKPrecision + 1 );

    const TICK dtickBucketUncer = _LNextPowerOf2( m_dtickUncertainty );
    RESMGRAssert( dtickBucketUncer == 1024 ||  dtickBucketUncer == 128 );

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


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
Term()
{

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


    m_ResourceLRUK.Term();
    m_KeyHistory.Term();
    m_HistoryLRUK.Term();

    if ( m_tickHighBar )
    {
        m_tickHighBar -= m_tickStart;
    }

    m_fInitialized = fFalse;
}


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


    if ( presHistoryProvided )
    {
        RESMGRAssert( !fUseHistory );


        _CopyHistory( key, pic, _PicFromPres( (CResource* const)presHistoryProvided ) );
        fRecoveredFromHistory = fTrue;
    }
    else if ( fUseHistory )
    {
        RESMGRAssert( !presHistoryProvided );


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


    if ( !fRecoveredFromHistory )
    {
        pic->m_tickLast = _TickSanitizeTickForTickLast( tickNow ) | ftickResourceLocked;
        

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


    _SanityCheckTick( pic->m_tickLast );
    _SanityCheckTick( pic->m_rgtick[ 1 - 1 ] );
    _SanityCheckTick( pic->m_rgtick[ m_K - 1 ] );
    _SanityCheckTickEx( pic->m_tickIndexTarget, ValidateTickCheck( vtcDefault | vtcAllowFarForward ) );



    if ( !presHistoryProvided && !( fRecoveredFromHistory && ( rmtf == k1CorrelatedTouch ) ) )
    {
        RESMGRAssert( ( _CmpTick( tickScanFirstFoundAll, tickNow ) >= 0 ) || ( _CmpTick( pic->m_tickIndexTarget, tickScanFirstFoundAll ) >= -6000 ) );
        RESMGRAssert( ( _CmpTick( tickScanFirstFoundNormal, tickNow ) >= 0 ) || ( _CmpTick( pic->m_tickIndexTarget, tickScanFirstFoundNormal ) >= -9000 ) );
    }


    errLRUK = _ErrInsertResource( &lock, pres, NULL );

    RESMGRAssert(   errLRUK == CResourceLRUK::ERR::errSuccess ||
                    errLRUK == CResourceLRUK::ERR::errOutOfMemory );


    RESMGRAssert( pic->FResourceLocked() );
    const TICK tickLastRestore = _TickSanitizeTickForTickLast( pic->m_tickLast );
    OnDebug( const TICK tickCheck = ) AtomicExchange( (LONG*)&pic->m_tickLast, tickLastRestore );
    RESMGRAssert( _FTickLocked( tickCheck ) );

    return errLRUK == CResourceLRUK::ERR::errSuccess ? ERR::errSuccess : ERR::errOutOfMemory;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ResMgrTouchFlags CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
RmtfTouchResource( CResource* const pres, const ULONG_PTR pctCachePriorityExternal, const TICK tickNowExternal )
{
    CInvasiveContext* const pic = _PicFromPres( pres );

    const WORD pctCachePriority = _AdjustCachePriority( pctCachePriorityExternal );
    const TICK tickNow = _TickSanitizeTickForTickLast( tickNowExternal );


    const TICK tickLastBIExpected = pic->TickLastTouchTime();
    RESMGRAssert( !_FTickLocked( tickLastBIExpected ) );
    _SanityCheckTick( tickLastBIExpected );


    if ( ( _CmpTick( tickNow, tickLastBIExpected ) <= 0 ) && ( pctCachePriority <= pic->m_pctCachePriority ) )
    {
        return kNoTouch;
    }


    else
    {

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


        const TICK tickLastRestore = _TickSanitizeTickForTickLast( tickNow );
        OnDebug( const TICK tickCheck = ) AtomicExchange( (LONG*)&pic->m_tickLast, tickLastRestore );
        RESMGRAssert( _FTickLocked( tickCheck ) );
        RESMGRAssert( tickCheck == tickLastAI );

        return rmtf;
    }
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
PuntResource( CResource* const pres, const TICK dtick )
{
#ifdef DEBUG

#ifdef _OS_HXX_INCLUDED
    RESMGRAssert( m_tidLockCheck == DwUtilThreadId() );
#else
    RESMGRAssert( m_tidLockCheck == 1 );
#endif
#endif

    CInvasiveContext* const pic = _PicFromPres( pres );


    const TICK tickLastBIExpected = pic->TickLastTouchTime();
    const TICK tickLastAI = _TickSanitizeTickForTickLast( tickLastBIExpected ) | ftickResourceLocked;
    RESMGRAssert( _FTickLocked( tickLastAI ) );

    const TICK tickLastBI = TICK( AtomicCompareExchange( (LONG*)&pic->m_tickLast, tickLastBIExpected, tickLastAI ) );

    if ( tickLastBI != tickLastBIExpected )
    {
        return;
    }


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


    RESMGRAssert( pic->FResourceLocked() );
    RESMGRAssert( !_FTickLocked( tickLastBI ) );
    OnDebug( const TICK tickCheck = ) AtomicExchange( (LONG*)&pic->m_tickLast, tickLastBI );
    RESMGRAssert( _FTickLocked( tickCheck ) );
    RESMGRAssert( tickCheck == tickLastAI );
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline BOOL CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
FRecentlyTouched( CResource* const pres, const TICK dtickRecent )
{
    CInvasiveContext* const pic = _PicFromPres( pres );

    return _CmpTick( pic->TickLastTouchTime(), _TickSanitizeTickForTickLast( TickRESMGRTimeCurrent() - dtickRecent ) ) > 0;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline BOOL CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
FSuperHotResource( CResource* const pres )
{
    CInvasiveContext* const pic = _PicFromPres( pres );

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

 
template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
MarkAsSuperCold( CResource* const pres )
{
    CInvasiveContext* const pic = _PicFromPres( pres );

    _MarkForFastEviction( pic );
    return;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrEvictResource( const CKey& key, CResource* const pres, const BOOL fKeepHistory )
{
    ERR     err = ERR::errSuccess;
    CLock   lock;


    LockResourceForEvict( pres, &lock );

    if ( ( err = ErrEvictCurrentResource( &lock, key, fKeepHistory ) ) != ERR::errSuccess )
    {
        RESMGRAssert( err == ERR::errNoCurrentResource );

        err = ERR::errResourceNotCached;
    }

    UnlockResourceForEvict( &lock );

    return err;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
LockResourceForEvict( CResource* const pres, CLock* const plock )
{

    CInvasiveContext* const pic = _PicFromPres( pres );

    const TICK tickIndex = pic->m_tickIndex;


    RESMGRAssert( !plock->m_fHasLrukLock );
    m_ResourceLRUK.LockKeyPtr( tickIndex, pres, &plock->m_lock );
    plock->m_fHasLrukLock = fTrue;


    if ( pic->m_tickIndex != tickIndex || tickIndex == tickUnindexed )
    {

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
#endif

}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
UnlockResourceForEvict( CLock* const plock )
{
    RESMGRAssert( plock->m_fHasLrukLock );
    m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
    plock->m_fHasLrukLock = fFalse;
}


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


    m_ResourceLRUK.MoveBeforeFirst( &plock->m_lock );
    plock->m_fHasLrukLock = fTrue;

    RESMGRTrace( rmttScanStats, L"\t\tBeginResScan:\n" );

#ifdef DEBUG
    RESMGRAssert( m_tidLockCheck == 0  || m_tidLockCheck == (DWORD)-1  );
#ifdef _OS_HXX_INCLUDED
    m_tidLockCheck = DwUtilThreadId();
#else
    m_tidLockCheck = 1;
#endif
#endif
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrGetCurrentResource( CLock* const plock, CResource** const ppres )
{
    RESMGRTrace( rmttScanProcessing, L"\t\t\t\tErrGetCurrentRes( %d ) ... ", plock->m_StuckList.FEmpty() );


    if ( plock->m_StuckList.FEmpty() )
    {
        CResourceLRUK::ERR errLRUK;


        errLRUK = m_ResourceLRUK.ErrRetrieveEntry( &plock->m_lock, ppres );

        RESMGRTrace( rmttScanProcessing, L"pbf = %p:%p, err = %d\n", *ppres, _PicFromPres( *ppres ), errLRUK );


        return errLRUK == CResourceLRUK::ERR::errSuccess ? ERR::errSuccess : ERR::errNoCurrentResource;
    }


    else
    {

        *ppres = plock->m_presStuckList;

        RESMGRTrace( rmttScanProcessing, L"pbf = %p:%p, err = %d\n", *ppres, _PicFromPres( *ppres ), *ppres ? ERR::errSuccess : ERR::errNoCurrentResource );

        return *ppres ? ERR::errSuccess : ERR::errNoCurrentResource;
    }
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrGetNextResource( CLock* const plock, CResource** const ppres )
{
    CResourceLRUK::ERR errLRUK;

#ifdef DEBUG
#ifdef _OS_HXX_INCLUDED
    RESMGRAssert( m_tidLockCheck == DwUtilThreadId() );
#else
    RESMGRAssert( m_tidLockCheck == 1 );
#endif
#endif


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

        BOOL fReestablishedCurrentIndex = fFalse;

        if ( plock->m_StuckList.FEmpty() )
        {

            while ( ( errLRUK = m_ResourceLRUK.ErrMoveNext( &plock->m_lock ) ) == CResourceLRUK::ERR::errSuccess )
            {
                plock->m_fHasLrukLock = fTrue;
                CResourceLRUK::ERR errLRUK2 = CResourceLRUK::ERR::errSuccess;
                CResource* pres;

                RS_STATISTICS( plock->m_stats.m_cEntriesScanned++ );

                errLRUK2 = m_ResourceLRUK.ErrRetrieveEntry( &plock->m_lock, &pres );
                RESMGRAssert( errLRUK2 == CResourceLRUK::ERR::errSuccess );

                CInvasiveContext* const pic = _PicFromPres( pres );

                RESMGRTrace( rmttScanProcessing, L"\t\t\t\tProcessing pres:pic %p:%p, { %u, %u, [0] %u, [1] %u, %u [x], %hu }\n", pres, pic, pic->m_tickIndex, pic->m_tickIndexTarget, pic->m_rgtick[0], pic->m_rgtick[1], pic->m_tickLast, pic->m_pctCachePriority  );

                _SanityCheckTickEx( pic->m_tickIndex, ValidateTickCheck( vtcAllowFarForward | vtcAllowIndexKey ) );


                RESMGRAssert( pic->m_tickIndex != TICK( tickUnindexed ) );
#ifdef DEBUG
                {
                CResourceLRUK::CLock lockValidate;

                RESMGRAssert( plock->m_fHasLrukLock );
                BOOL fLockSucceeded = m_ResourceLRUK.FTryLockKeyPtr( pic->m_tickIndex,
                                                                     pres,
                                                                     &lockValidate);

                RESMGRAssert( !fLockSucceeded );
                }
#endif

                BOOL fRightBucket = fFalse;


                if (    !plock->m_UpdateList.FEmpty() &&
                        m_ResourceLRUK.CmpKey( pic->m_tickIndex, plock->m_tickIndexCurrent ) )
                {
                    RESMGRAssert( pic->m_tickIndex != TICK( tickUnindexed ) );
                    RESMGRAssert( plock->m_tickIndexCurrent != TICK( tickUnindexed ) );
                    RS_STATISTICS( plock->m_stats.m_cUpdateListIndexChangeBreak++ );
                    RESMGRTrace( rmttScanProcessing, L"\t\t\t\t\tBreak to triage update list: %u != %u\n", pic->m_tickIndex, plock->m_tickIndexCurrent );
                    break;
                }


                OSSYNC_FOREVER
                {
#ifdef DEBUG
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
                    

                    plock->m_icCurrentBI.m_tickLast = pic->m_tickLast;
                    for ( INT K = m_Kmax; K >= 1; K-- )
                    {
                        plock->m_icCurrentBI.m_rgtick[ K - 1 ] = pic->m_rgtick[ K - 1 ];
                    }
                    plock->m_icCurrentBI.m_pctCachePriority = pic->m_pctCachePriority;
                    plock->m_icCurrentBI.m_tickIndex = pic->m_tickIndex;
                    plock->m_icCurrentBI.m_tickIndexTarget = pic->m_tickIndexTarget;


                    RESMGRAssert( plock->m_icCurrentBI.m_tickLast == tickLastAI );
                    plock->m_icCurrentBI.m_tickLast = tickLastBI;

                    _SanityCheckTick( plock->m_icCurrentBI.m_tickLast );
                    _SanityCheckTick( plock->m_icCurrentBI.m_rgtick[ 1 - 1 ] );
                    _SanityCheckTick( plock->m_icCurrentBI.m_rgtick[ m_K - 1 ] );

                    _SanityCheckTickEx( plock->m_icCurrentBI.m_tickIndex, ValidateTickCheck( vtcAllowFarForward | vtcAllowIndexKey ) );
                    _SanityCheckTickEx( plock->m_icCurrentBI.m_tickIndexTarget, ValidateTickCheck( vtcAllowFarForward | vtcAllowIndexKey ) );

                    fRightBucket = m_ResourceLRUK.CmpKey( plock->m_icCurrentBI.m_tickIndex, plock->m_icCurrentBI.m_tickIndexTarget ) >= 0;


                    if ( !fRightBucket )
                    {
                        pic->m_tickIndex = TICK( tickUnindexed );
                    }


                    OnDebug( const TICK tickCheck = ) AtomicExchange( (LONG*)&pic->m_tickLast, tickLastBI );
                    RESMGRAssert( _FTickLocked( tickCheck ) );
                    RESMGRAssert( tickCheck == tickLastAI );

                    break;
                }

                RESMGRAssert( plock->m_icCurrentBI.m_tickIndex != tickUnindexed );
                RESMGRAssert( plock->m_fHasLrukLock );

                RESMGRAssert( !plock->m_icCurrentBI.FResourceLocked() );
                if ( fRightBucket )
                {
                    plock->m_stats.UpdateFoundTimes( this, &plock->m_icCurrentBI );

                    RS_STATISTICS( plock->m_stats.m_cEntriesFound++ );
                    
                    RESMGRTrace( rmttScanProcessing, L"\t\t\t\t\tReturning pres:pic %p:%p\n", pres, pic );
                    RESMGRAssert( pic == _PicFromPres( pres ) );
#ifdef DEBUG
                    plock->m_picCheckBI = pic;
#endif
                    *ppres = pres;
                    break;
                }
                else
                {

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
            }

            RESMGRAssert(   errLRUK == CResourceLRUK::ERR::errSuccess ||
                            errLRUK == CResourceLRUK::ERR::errNoCurrentEntry );


            if ( *ppres == NULL )
            {
                BOOL fNeedToMoveToCurrentIndex = fFalse;

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
                }


                if ( plock->m_StuckList.FEmpty() )
                {
                    if ( fNeedToMoveToCurrentIndex )
                    {
                        RESMGRAssert( !plock->m_fHasLrukLock );

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
            }

            RESMGRTrace( rmttScanProcessing, L"\t\t\t\tEnd.\n" );
        }


        if ( !plock->m_StuckList.FEmpty() )
        {

            plock->m_presStuckList      = ( plock->m_presStuckList ?
                                                plock->m_StuckList.Next( plock->m_presStuckList ) :
                                                plock->m_presStuckListNext );
            plock->m_presStuckListNext  = NULL;


            if ( plock->m_presStuckList )
            {

                m_cResourceScannedOutOfOrder++;
                RS_STATISTICS( plock->m_stats.m_cEntriesFoundOutOfOrder++ );

                *ppres = plock->m_presStuckList;
                RESMGRTrace( rmttScanProcessing, L"\t\t\t\tOutOfOrderPres %p:%p { %u, [0] %u, [1] %u, %u, %u }\n",
                                *ppres, _PicFromPres( *ppres ), _PicFromPres( *ppres )->m_tickLast,
                                _PicFromPres( *ppres )->m_rgtick[0], _PicFromPres( *ppres )->m_rgtick[1],
                                _PicFromPres( *ppres )->m_tickIndex, _PicFromPres( *ppres )->m_tickIndexTarget );
            }


            else
            {

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


                RESMGRAssert( plock->m_tickIndexCurrent != TICK( tickUnindexed ) );
                RESMGRAssert( !plock->m_fHasLrukLock );
                m_ResourceLRUK.MoveAfterKeyPtr( plock->m_tickIndexCurrent, (CResource*)DWORD_PTR( -LONG_PTR( sizeof( CResource ) ) ), &plock->m_lock );
                plock->m_fHasLrukLock = fTrue;
                fReestablishedCurrentIndex = ( plock->m_tickIndexCurrent != TICK( tickUnindexed ) );
                plock->m_tickIndexCurrent = TICK( tickUnindexed );
            }
        }


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


    RESMGRAssert( *ppres || ( plock->m_UpdateList.FEmpty() && plock->m_StuckList.FEmpty() ) );


    RESMGRAssert( ( *ppres == NULL ) || plock->m_fHasLrukLock || ( *ppres == plock->m_presStuckList ) );


    return *ppres ? ERR::errSuccess : ERR::errNoCurrentResource;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::ERR CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
ErrEvictCurrentResource( CLock* const plock, const CKey& key, const BOOL fKeepHistory )
{
    ERR                     err;
    CResourceLRUK::ERR      errLRUK;
    CResource*              pres;


    if ( ( err = ErrGetCurrentResource( plock, &pres ) ) != ERR::errSuccess )
    {
        return err;
    }

#ifdef DEBUG
    if ( plock->m_picCheckBI )
    {
#ifdef _OS_HXX_INCLUDED
        RESMGRAssert( m_tidLockCheck == DwUtilThreadId() );
#else
        RESMGRAssert( m_tidLockCheck == 1 );
#endif
    }
#endif

    CInvasiveContext * const pic = _PicFromPres( pres );
    const TICK tickLast = pic->TickLastTouchTime();
    RESMGRAssert( !plock->m_StuckList.FEmpty() || pic->m_tickIndex != tickUnindexed );


    if ( plock->m_StuckList.FEmpty() )
    {

        RESMGRAssert( plock->m_fHasLrukLock );
        errLRUK = m_ResourceLRUK.ErrDeleteEntry( &plock->m_lock );
        RESMGRAssert( errLRUK == CResourceLRUK::ERR::errSuccess );


        if ( ( plock->m_icCurrentBI.m_tickLast != 0 ) && ( plock->m_icCurrentBI.m_tickLast == tickLast ) )
        {
            RESMGRAssert( !plock->m_icCurrentBI.FResourceLocked() );


            RESMGRAssert( plock->m_picCheckBI == pic );

            plock->m_stats.UpdateEvictTimes( this, &plock->m_icCurrentBI );
        }
    }


    else
    {

        plock->m_presStuckListNext = plock->m_StuckList.Next( pres );
        plock->m_StuckList.Remove( pres );
        plock->m_presStuckList = NULL;

        RESMGRAssert( !plock->m_fHasLrukLock );
        m_ResourceLRUK.LockKeyPtr( plock->m_tickIndexCurrent, pres, &plock->m_lock );
        m_ResourceLRUK.UnreserveEntry( &plock->m_lock );
        m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );


        if ( plock->m_StuckList.FEmpty() )
        {
            RESMGRAssert( plock->m_tickIndexCurrent != TICK( tickUnindexed ) );
            RESMGRAssert( !plock->m_fHasLrukLock );
            m_ResourceLRUK.MoveAfterKeyPtr( plock->m_tickIndexCurrent, (CResource*)DWORD_PTR( -LONG_PTR( sizeof( CResource ) ) ), &plock->m_lock );
            plock->m_fHasLrukLock = fTrue;
            plock->m_tickIndexCurrent = TICK( tickUnindexed );
        }
    }


    RS_STATISTICS( plock->m_stats.m_cEntriesEvicted++ );

    if ( pic->FSuperColded() )
    {

        AtomicDecrement( (LONG*)&m_cSuperCold );
    }
    else if ( fKeepHistory )
    {

        _StoreHistory( key, pic );
    }

    return ERR::errSuccess;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
EndResourceScan( CLock* const plock )
{
#ifdef DEBUG
#ifdef _OS_HXX_INCLUDED
    RESMGRAssert( m_tidLockCheck == DwUtilThreadId() );
    m_tidLockCheck = (DWORD)-1;
#else
    RESMGRAssert( m_tidLockCheck == 1 );
    m_tidLockCheck = (DWORD)-1;
#endif
#endif


    if ( plock->m_fHasLrukLock )
    {
        RESMGRAssert( plock->m_StuckList.FEmpty() );
        m_ResourceLRUK.UnlockKeyPtr( &plock->m_lock );
        plock->m_fHasLrukLock = fFalse;
    }


    if ( plock->m_StuckList.FEmpty() )
    {

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

        }

    }


    RESMGRAssert( plock->m_UpdateList.FEmpty() );


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


    RS_STATISTICS( const TICK dtickScan = _TickCurrentTime() - plock->m_stats.m_tickBeginResScan );


    m_cLastScanEnumeratedEntries    = plock->m_lock.CEnumeratedEntries();
    m_cLastScanBucketsScanned       = plock->m_lock.CEnumeratedBuckets();
    m_cLastScanEmptyBucketsScanned  = plock->m_lock.CEnumeratedEmptyBuckets();
    m_cLastScanEnumeratedIDRange    = m_ResourceLRUK.CEnumeratedRangeVirtual( &plock->m_lock );
    m_dtickLastScanEnumeratedRange  = m_ResourceLRUK.CEnumeratedRangeReal( &plock->m_lock );

    if ( plock->m_stats.m_tickScanFirstFoundAll != 0 )
    {
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


    memcpy( &m_statsScanPrev, &m_statsScanLast, sizeof(m_statsScanPrev) );
    memcpy( &m_statsScanLast, &plock->m_stats, sizeof(m_statsScanLast) );


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
#endif

    RESMGRAssert( !plock->m_fHasLrukLock );
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CHistoryRecord()
{
    return m_cHistoryRecord;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CHistoryHit()
{
    return m_cHistoryHit;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CHistoryRequest()
{
    return m_cHistoryReq;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CResourceScanned()
{
    return m_cResourceScanned;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CResourceScannedMoves()
{
    return m_cResourceScannedMoves;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline DWORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
CResourceScannedOutOfOrder()
{
    return m_cResourceScannedOutOfOrder;
}


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


    if ( pctCachePriority > pic->m_pctCachePriority )
    {
        pic->m_pctCachePriority = pctCachePriority;


        fRecomputeIndexTarget = fTrue;
    }


    if ( pic->FSuperColded() )
    {
        AtomicDecrement( (LONG*)&m_cSuperCold );
        fRecomputeIndexTarget = fTrue;
    }


    if ( _DtickDelta( pic->m_rgtick[ 1 - 1 ], tickNow ) <= (LONG)m_ctickCorrelatedTouch )
    {
        rmtf = k1CorrelatedTouch;
        goto CheckComputeIndexTarget;
    }


    fRecomputeIndexTarget = fTrue;

    _SanityCheckTick( m_tickScanFirstFoundNormal );
    _SanityCheckTick( m_tickScanFirstEvictedIndexTarget );
    _SanityCheckTick( m_tickScanFirstEvictedIndexTargetHW );


    const LONG dtickCorrelationPeriod = _DtickDelta( pic->m_rgtick[ 1 - 1 ], tickLast );
    RESMGRAssert( dtickCorrelationPeriod >= 0 );


    for ( INT K = m_Kmax; K >= 2; K-- )
    {
        pic->m_rgtick[ K - 1 ] = _TickSanitizeTick( pic->m_rgtick[ ( K - 1 ) - 1 ] + (TICK)dtickCorrelationPeriod + m_ctickTimeout );
        _SanityCheckTick( pic->m_rgtick[ K - 1 ] );
    }


    pic->m_rgtick[ 1 - 1 ] = tickNow;
    
    rmtf = ( ( m_K == 1 ) ? k1Touch : k2Touch );

CheckComputeIndexTarget:


    if ( fRecomputeIndexTarget )
    {
        
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
        TICK tickIndexTarget = pic->m_tickIndexTarget;

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

    RESMGRAssert( errLRUK == CResourceLRUK::ERR::errSuccess ||  errLRUK == CResourceLRUK::ERR::errOutOfMemory );
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


    const TICK tickLastBIExpected = pic->TickLastTouchTime();
    RESMGRAssert( !_FTickLocked( tickLastBIExpected ) );

    const TICK tickLastAI = _TickSanitizeTickForTickLast( tickLastBIExpected ) | ftickResourceLocked;
    RESMGRAssert( _FTickLocked( tickLastAI ) );

    const TICK tickLastBI = AtomicCompareExchange( (LONG*)&pic->m_tickLast, tickLastBIExpected, tickLastAI );

    if ( tickLastBI != tickLastBIExpected )
    {
        FE_STATISTICS( m_statsFastEvict.cFailureToMarkForEvictionBecauseFailedTouchLock++; );
        return;
    }

    RESMGRAssert( !_FTickLocked( tickLastBI ) );
    RESMGRAssert( tickLastBI == _TickLastTouchTime( tickLastAI ) );

    _SanityCheckTick( tickLastBI );

    const BOOL fWasSuperColded = pic->FSuperColded();



    const TICK      tickInitial             = m_tickScanFirstFoundNormal;
    TICK            tickTarget              = _TickSanitizeTickForTickIndex( tickInitial - m_dtickSuperColdOffset );


    _SanityCheckTick( tickTarget );
    RESMGRAssert( _FTickSuperColded( tickTarget ) );



    const TICK      tickInitialIndex        = pic->m_tickIndex;


    if ( tickInitialIndex == tickUnindexed )
    {
        FE_STATISTICS( m_statsFastEvict.cFailureToMarkForEvictionBecauseItHasNoBucket++; );
        goto HandleError;
    }


    if ( m_ResourceLRUK.CmpKey( tickInitialIndex, tickTarget ) == 0 )
    {
        FE_STATISTICS( m_statsFastEvict.cAlreadyInTargetTick++; );
        m_statsFastEvict.cMarkForEvictionSucceeded++;
        goto HandleError;
    }

    RESMGRAssert( pic->m_tickLast == tickLastAI );


    pic->m_tickIndexTarget = tickTarget;




    if ( !fWasSuperColded )
    {
        AtomicIncrement( (LONG*)&m_cSuperCold );
    }



    
    fLockedSrc = !_FFaultInj( rmfiFastEvictSourceLockFI ) &&
                 m_ResourceLRUK.FTryLockKeyPtr( tickInitialIndex,
                                                 pres,
                                                &lockSource );

    if ( !fLockedSrc )
    {
        FE_STATISTICS( m_statsFastEvict.cFailureToMarkForEvictionBecauseOfLockFailure++; );
        RESMGRTrace( rmttFastEvictContentions, L"Hit source bucket lock contention tickTarget = %u on source %p:%p{ [1] %u [2] %u, %u } ... @ %u!\n",
                        tickTarget, pres, pic, pic->m_rgtick[ 0 ], pic->m_rgtick[ 1 ], pic->m_tickIndex, _TickCurrentTime() );
        goto HandleError;
    }


    fLockedDst = !_FFaultInj( rmfiFastEvictDestInitialLockFI ) &&
                 m_ResourceLRUK.FTryLockKeyPtr( tickTarget,
                                                pres,
                                                &lockDestination );

    if ( !fLockedDst )
    {


        if ( lockDestination.IsSameBucketHead( &lockSource ) || _FFaultInj( rmfiFastEvictDestSrcBucketMatchFI ) )
        {
            FE_STATISTICS( m_statsFastEvict.cSuccessAlreadyInRightBucket++; );
            m_statsFastEvict.cMarkForEvictionSucceeded++;
            goto HandleError;
        }
        

        tickTarget     = _TickSanitizeTickForTickIndex( tickTarget + m_dtickUncertainty * 2 - 1 );
        _SanityCheckTick( tickTarget );
        RESMGRAssert( _FTickSuperColded( tickTarget ) );
#ifdef RESMGR_LOWBAR_CHECKING_FIXCREEP
        RESMGRAssert( _CmpTick( tickTarget, m_tickLowBar ) > 0 );
#endif

        fLockedDst = !_FFaultInj( rmfiFastEvictDestSecondaryLockFI ) &&
                     m_ResourceLRUK.FTryLockKeyPtr( tickTarget,
                                                        pres,
                                                       &lockDestination);
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

        FE_STATISTICS(  m_statsFastEvict.cFailureToMarkForEvictionBecauseItMoved++; );
        goto HandleError;
    }
    
    RESMGRAssert( tickInitialIndex == pic->m_tickIndex );



    RESMGRAssert( pic->m_tickIndex != tickUnindexed );
    errLRUK = m_ResourceLRUK.ErrDeleteEntry( &lockSource );
    RESMGRAssert( errLRUK == CResourceLRUK::ERR::errSuccess );


    _SanityCheckTick( tickTarget );
    RESMGRAssert( _FTickSuperColded( tickTarget ) );
    RESMGRAssert( 0 == ( tickTarget & ftickResourceNormalTouch ) );
    pic->m_tickIndex = tickTarget;
    _SanityCheckTickEx( pic->m_tickIndex, vtcAllowIndexKey );

    errLRUK = _FFaultInj( rmfiFastEvictInsertEntryFailsFI ) ?
              CResourceLRUK::ERR::errKeyRangeExceeded  :
              m_ResourceLRUK.ErrInsertEntry( &lockDestination,
                                              pres,
                                              fFalse );

    if ( errLRUK != CResourceLRUK::ERR::errSuccess )
    {

        RESMGRTrace( rmttFastEvictContentions, L"Hit bucket insert issue (%d) tickTarget = %u on %p:%p{ [1] %u [2] %u, %u } ... @ %u!\n",
                        errLRUK, tickTarget, pres, pic, pic->m_rgtick[ 0 ], pic->m_rgtick[ 1 ], pic->m_tickIndex, _TickCurrentTime() );
        pic->m_tickIndex = tickInitialIndex;
        _SanityCheckTickEx( pic->m_tickIndex, ValidateTickCheck( vtcAllowIndexKey | vtcAllowIndexKey ) );
        errLRUK = m_ResourceLRUK.ErrInsertEntry( &lockSource, pres, fTrue );
        RESMGRAssert( errLRUK == CResourceLRUK::ERR::errSuccess );
        RESMGRAssert( tickInitialIndex == pic->m_tickIndex );
        FE_STATISTICS(  m_statsFastEvict.cFailuseToInsertIntoTargetBucket++; );
    }
    else
    {
        m_statsFastEvict.cMarkForEvictionSucceeded++;
    }

HandleError:


#ifdef RESMGR_LOWBAR_CHECKING_FIXCREEP
    RESMGRAssert( _CmpTick( m_tickLowBar, tickLastBI ) <= 0 );
#endif
    RESMGRAssert( pic->FResourceLocked() );
    RESMGRAssert( !_FTickLocked( tickLastBI ) );
    OnDebug( const TICK tickCheck = ) AtomicExchange( (LONG*)&pic->m_tickLast, tickLastBI );
    RESMGRAssert( _FTickLocked( tickCheck ) );
    RESMGRAssert( tickCheck == tickLastAI );


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


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_RestoreHistory( const CKey& key, CInvasiveContext* const pic, const TICK tickNow, const WORD pctCachePriority, __out_opt BOOL * pfInHistory )
{

    if ( m_K < 2 )
    {
        return;
    }

    CHistoryTable::CLock    lockHist;
    CHistoryEntry           he;

    _SanityCheckTick( tickNow );

    m_cHistoryReq++;

    m_KeyHistory.ReadLockKey( key, &lockHist );
    if ( m_KeyHistory.ErrRetrieveEntry( &lockHist, &he ) == CHistoryTable::ERR::errSuccess )
    {
        CHistory* const phist = he.m_phist;


        if ( !_FResourceStaleForKeepingHistory( phist->m_rgtick, phist->m_tickLast, phist->m_pctCachePriority, tickNow ) &&
            !_FResourceStaleForKeepingHistory( phist->m_rgtick, phist->m_tickLast, pctCachePriority, tickNow ) )
        {

            m_cHistoryHit++;
            *pfInHistory = fTrue;

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


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_CopyHistory( const CKey& key, CInvasiveContext* const picDst, const CInvasiveContext* const picSrc )
{
    _SanityCheckTick( picSrc->m_tickLast );

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
        AtomicIncrement( (LONG*)&m_cSuperCold );
    }
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_StoreHistory( const CKey& key, CInvasiveContext* const pic )
{
    if ( _FResourceStaleForKeepingHistory( pic->m_rgtick, pic->m_tickLast, pic->m_pctCachePriority, TickRESMGRTimeCurrent() ) )
    {
        return;
    }

    CHistory* const phist = _PhistAllocHistory( key );
    

    if ( phist == NULL )
    {
        return;
    }


    phist->m_tickLast = _TickSanitizeTickForTickLast( pic->m_tickLast ) | ftickResourceLocked;


    phist->m_key = key;

    for ( INT K = 1; K <= m_Kmax; K++ )
    {
        phist->m_rgtick[ K - 1 ] = pic->m_rgtick[ K - 1 ];
    }

    phist->m_pctCachePriority = pic->m_pctCachePriority;


    if ( !_FInsertHistory( phist ) )
    {

        delete phist;

    }
}


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


    m_KeyHistory.WriteLockKey( key, &lockHist );
    errHist = m_KeyHistory.ErrRetrieveEntry( &lockHist, &he );
    if ( errHist == CHistoryTable::ERR::errSuccess )
    {

        const TICK      tickKey     = he.m_phist->m_rgtick[ m_K - 1 ];
        CHistory* const phistPtr    = he.m_phist;


        errHist = m_KeyHistory.ErrDeleteEntry( &lockHist );
        RESMGRAssert( errHist == CHistoryTable::ERR::errSuccess );
        

        if ( ( errHist == CHistoryTable::ERR::errSuccess ) &&
                m_HistoryLRUK.FTryLockKeyPtr( tickKey, phistPtr, &lockHLRUK ) )
        {

            errHLRUK = m_HistoryLRUK.ErrDeleteEntry( &lockHLRUK );
            if ( errHLRUK == CHistoryLRUK::ERR::errSuccess )
            {
                AtomicDecrement( (LONG*)&m_cHistoryRecord );
                m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );
                m_KeyHistory.WriteUnlockKey( &lockHist );


                return phistPtr;
            }

            m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );
        }
    }
    
    m_KeyHistory.WriteUnlockKey( &lockHist );


    m_HistoryLRUK.MoveBeforeFirst( &lockHLRUK );
    if ( ( errHLRUK = m_HistoryLRUK.ErrMoveNext( &lockHLRUK ) ) == CHistoryLRUK::ERR::errSuccess )
    {
        errHLRUK = m_HistoryLRUK.ErrRetrieveEntry( &lockHLRUK, &phist );
        RESMGRAssert( errHLRUK == CHistoryLRUK::ERR::errSuccess );

        if( errHLRUK == CHistoryLRUK::ERR::errSuccess )
        {
            
            if ( _FResourceStaleForKeepingHistory( phist->m_rgtick, phist->m_tickLast, phist->m_pctCachePriority, TickRESMGRTimeCurrent() ) &&
                m_KeyHistory.FTryWriteLockKey( phist->m_key, &lockHist ) )
            {
                
                errHist = m_KeyHistory.ErrRetrieveEntry( &lockHist, &he );
                if ( errHist == CHistoryTable::ERR::errSuccess )
                {
                    if ( he.m_phist == phist )
                    {
                        errHist = m_KeyHistory.ErrDeleteEntry( &lockHist );
                        RESMGRAssert( errHist == CHistoryTable::ERR::errSuccess );
                    }

                    if( errHist == CHistoryTable::ERR::errSuccess )
                    {

                        errHLRUK = m_HistoryLRUK.ErrDeleteEntry( &lockHLRUK );
                        RESMGRAssert( errHLRUK == CHistoryLRUK::ERR::errSuccess );

                        if( errHLRUK == CHistoryLRUK::ERR::errSuccess )
                        {
                            AtomicDecrement( (LONG*)&m_cHistoryRecord );
                            m_KeyHistory.WriteUnlockKey( &lockHist );
                            m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );


                            return phist;
                        }
                    }
                }
                
                m_KeyHistory.WriteUnlockKey( &lockHist );
            }
        }
    }
    
    m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );


    m_HistoryLRUK.MoveAfterLast( &lockHLRUK );
    if ( ( errHLRUK = m_HistoryLRUK.ErrMovePrev( &lockHLRUK ) ) == CHistoryLRUK::ERR::errSuccess )
    {
        errHLRUK = m_HistoryLRUK.ErrRetrieveEntry( &lockHLRUK, &phist );
        RESMGRAssert( errHLRUK == CHistoryLRUK::ERR::errSuccess );

        if( errHLRUK == CHistoryLRUK::ERR::errSuccess )
        {
            
            if ( _FResourceStaleForKeepingHistory( phist->m_rgtick, phist->m_tickLast, phist->m_pctCachePriority, TickRESMGRTimeCurrent() ) &&
                m_KeyHistory.FTryWriteLockKey( phist->m_key, &lockHist ) )
            {
                
                errHist = m_KeyHistory.ErrRetrieveEntry( &lockHist, &he );
                if ( errHist == CHistoryTable::ERR::errSuccess )
                {
                    if ( he.m_phist == phist )
                    {
                        errHist = m_KeyHistory.ErrDeleteEntry( &lockHist );
                        RESMGRAssert( errHist == CHistoryTable::ERR::errSuccess );
                    }

                    if( errHist == CHistoryTable::ERR::errSuccess )
                    {

                        errHLRUK = m_HistoryLRUK.ErrDeleteEntry( &lockHLRUK );
                        RESMGRAssert( errHLRUK == CHistoryLRUK::ERR::errSuccess );

                        if( errHLRUK == CHistoryLRUK::ERR::errSuccess )
                        {
                            AtomicDecrement( (LONG*)&m_cHistoryRecord );
                            m_KeyHistory.WriteUnlockKey( &lockHist );
                            m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );


                            return phist;
                        }
                    }
                }
                
                m_KeyHistory.WriteUnlockKey( &lockHist );
            }
        }
    }
    
    m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );


    return new CHistory;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline BOOL CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_FInsertHistory( CHistory* const phist )
{
    CHistoryLRUK::ERR       errHLRUK;
    CHistoryLRUK::CLock     lockHLRUK;

    CHistoryEntry           he;

    CHistoryTable::ERR      errHist;
    CHistoryTable::CLock    lockHist;


    _SanityCheckTick( phist->m_rgtick[ m_K - 1 ] );

    m_HistoryLRUK.LockKeyPtr( phist->m_rgtick[ m_K - 1 ], phist, &lockHLRUK );

    if ( m_KeyHistory.FTryWriteLockKey( phist->m_key, &lockHist ) )
    {
        if ( ( errHLRUK = m_HistoryLRUK.ErrInsertEntry( &lockHLRUK, phist ) ) != CHistoryLRUK::ERR::errSuccess )
        {
            RESMGRAssert(   errHLRUK == CHistoryLRUK::ERR::errOutOfMemory ||
                            errHLRUK == CHistoryLRUK::ERR::errKeyRangeExceeded );


            m_KeyHistory.WriteUnlockKey( &lockHist );
            m_HistoryLRUK.UnlockKeyPtr( &lockHLRUK );
            
            return fFalse;
        }
        
        AtomicIncrement( (LONG*)&m_cHistoryRecord );


        he.m_KeySignature   = CHistoryTable::CKeyEntry::Hash( phist->m_key );
        he.m_phist          = phist;

        if ( ( errHist = m_KeyHistory.ErrInsertEntry( &lockHist, he ) ) != CHistoryTable::ERR::errSuccess )
        {

            if ( errHist == CHistoryTable::ERR::errKeyDuplicate )
            {

                errHist = m_KeyHistory.ErrReplaceEntry( &lockHist, he );
                RESMGRAssert( errHist == CHistoryTable::ERR::errSuccess );
            }


            else
            {
                RESMGRAssert( errHist == CHistoryTable::ERR::errOutOfMemory );

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


    return fTrue;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline CResource* CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_PresFromPic( CInvasiveContext* const pic ) const
{
    return (CResource*)( (BYTE*)pic - OffsetOfIC() );
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::CInvasiveContext* CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_PicFromPres( CResource* const pres ) const
{
    return (CInvasiveContext*)( (BYTE*)pres + OffsetOfIC() );
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline ULONG CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_kLrukPool( const TICK* const rgtick )
{
    RESMGRAssert( 2 == m_Kmax );

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


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TickCurrentTime()
{
    return _TickSanitizeTick( TickRESMGRTimeCurrent() );
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TickOldestEstimate()
{
    TICK tickOldestEstimate = *(volatile TICK*)&m_tickScanFirstEvictedIndexTargetHW;

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

    LONG dtick = tickToScale - tickOldestEstimate;

    if ( dtick < 0 )
    {
        dtick = 0;
    }

    DWORD tickScaled = 0;

    if ( pctCachePriority == 100 )
    {
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



template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline LONG CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_CmpTick( const TICK tick1, const TICK tick2 )
{
    return LONG( tick1 - tick2 );
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline LONG CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_DtickDelta( const TICK tickEarlier, const TICK tickLater )
{
    return LONG( tickLater - tickEarlier );
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline WORD CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_AdjustCachePriority( const ULONG_PTR pctCachePriorityExternal )
{
    RESMGRAssert( FIsCachePriorityValid( pctCachePriorityExternal ) );

    C_ASSERT( FIsCachePriorityValid( 0 ) );
    C_ASSERT( FIsCachePriorityValid( 1 ) );
    if ( pctCachePriorityExternal == 0 )
    {
        return 1;
    }

    return (WORD)pctCachePriorityExternal;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TickSanitizeTick( const TICK tick )
{
    return ( ( tick == 0 ) ? 1 : tick );
}


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


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TickLastTouchTime( const TICK tick )
{
    return ( tick & tickTickLastMask );
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline BOOL CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_FTickLocked( const TICK tick )
{
    return ( ( tick & ftickResourceLocked ) != 0 );
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TickSanitizeTickForTickIndex( TICK tick )
{
    C_ASSERT( ftickResourceNormalTouch == 1 );
    C_ASSERT( tickUnindexed == 0xFFFFFFFF );


    tick = UlBound( tick, (TICK)ftickResourceNormalTouch + 1, (TICK)tickUnindexed - 2 ) & tickTickIndexMask;

    RESMGRAssert( tick > (TICK)ftickResourceNormalTouch );
    RESMGRAssert( tick < (TICK)tickUnindexed );
    RESMGRAssert( ( tick & tickTickIndexMask ) != 0 );
    RESMGRAssert( ( tick & ~tickTickIndexMask ) == 0 );
    RESMGRAssert( _FTickSuperColded( tick ) );

    return tick;
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
typename CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::TICK CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_TickIndexTime( const TICK tick )
{
    return ( tick & tickTickIndexMask );
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline BOOL CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_FTickSuperColded( const TICK tick )
{
    return ( ( tick & ftickResourceNormalTouch ) == 0 );
}


template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline BOOL CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_FResourceStaleForKeepingHistory( const TICK* const rgtick, const TICK tickLast, const WORD pctCachePriority, const TICK tickNow )
{

    if ( m_K < 2 )
    {
        return fTrue;
    }


    const TICK tickOldestEstimate = _TickOldestEstimate();


    const TICK tickIndexTargetNextEstimate = _TickSanitizeTick( rgtick[ ( m_K - 1 ) - 1 ] + m_ctickTimeout );
    if ( _CmpTick( tickIndexTargetNextEstimate, tickOldestEstimate ) < 0 )
    {
        return fTrue;
    }


    const ULONG K = _kLrukPool( rgtick );
    const TICK tickMostSignificant = _TickSanitizeTick( rgtick[ K - 1 ] - ( m_ctickTimeout + m_ctickCorrelatedTouch ) * ( K - 1 ) );
    if ( _CmpTick( tickMostSignificant, tickOldestEstimate ) < 0 )
    {
        return fTrue;
    }


    if ( _CmpTick( _ScaleTick( tickIndexTargetNextEstimate, pctCachePriority ), m_tickScanFirstFoundNormal ) < 0 )
    {
        return fTrue;
    }


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
        RESMGRAssert( _DtickDelta( tick, _TickCurrentTime() ) < (LONG)m_dtickCacheLifetimeBar );
    }

}

#endif

template< INT m_Kmax, class CResource, PfnOffsetOf OffsetOfIC, class CKey >
inline void CLRUKResourceUtilityManager< m_Kmax, CResource, OffsetOfIC, CKey >::
_ValidateTickProgress( __in_z const WCHAR * szStateVarName, const TICK tickStoredState, const TICK tickNew, const ValidateTickProgress vtp )
{
#ifndef RTM
    const RESMGRTraceTag rmttEff = ( vtpNoTracing & vtp ) ? rmttNone : rmttTickUpdateJumps;
#endif

    
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

            OnDebug( C_ASSERT( vtpDisableAssert == 0x1 ) );
            const BOOL fAssertable = !( vtp & ValidateTickProgress( 0x1 ) );

            if ( vtp & vtpSuperColdBackwardCheck && dtickUpdate < -( 2 * dtickTwoBuckets + (INT)m_dtickSuperColdOffsetConcurrentMax ) )
            {
                RESMGRTrace( rmttEff, L"\t\tERRR: Significant %ws Movement from %u(0x%x) %hs%d to %u(0x%x) ... (ERRR: too far negative / backwards adjustment!)\n",
                                    szStateVarName, tickStoredState, tickStoredState, INT(dtickUpdate) >= 0 ? "+" : "", dtickUpdate, tickNew, tickNew );
                if ( fAssertable )
                {
                }
            }
            else if ( vtp & vtpConcurrentBackwardCheck && dtickUpdate < -( 2 * dtickTwoBuckets ) )
            {
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
                    RESMGRTrace( rmttEff, L"BREAKING On %ws jump too far forwards.\n", szStateVarName );
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


#endif
