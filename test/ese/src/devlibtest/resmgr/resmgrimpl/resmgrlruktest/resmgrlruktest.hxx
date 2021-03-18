// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RESMGRLRUKTEST_HXX_INCLUDED
#define _RESMGRLRUKTEST_HXX_INCLUDED


//  We allow modules using us to override our assert function.

#ifndef RESMGRLRUKTESTAssert
#define RESMGRLRUKTESTAssert    Assert
#endif  //  RESMGRLRUKTESTAssert

#pragma push_macro("wcslen")
#undef wcslen
#include <unordered_map>
#include <map>
#pragma pop_macro("wcslen")

//  Implementation of the classic LRU-K algorithm described in
//  http://www.cs.cmu.edu/~christos/courses/721-resources/p297-o_neil.pdf

//  ================================================================
template<class CKey, class CTimestamp>
class CLRUKTestResourceUtilityManager
//  ================================================================
{
    public:
        static const BYTE KMax = 5;

        //  API error codes.

        enum ERR
        {
            errSuccess,
            errInvalidParameter,
            errOutOfMemory,
            errResourceNotCached,
            errResourceAlreadyCached,
            errNoCurrentResource
        };

        //  Ctor./Dtor.

        CLRUKTestResourceUtilityManager();
        ~CLRUKTestResourceUtilityManager();


        //  Init./term.

        ERR ErrInit( const INT K, const double csecCorrelatedTouch );
        void Term();

        //  Cache/touch/evict.

        ERR ErrCacheResource( const CKey key, CTimestamp timestamp, const bool fUseHistory );
        ERR ErrTouchResource( const CKey key, CTimestamp timestamp );
        ERR ErrMarkResourceAsSuperCold( const CKey key );
        ERR ErrEvictResource( const CKey key );
        ERR ErrEvictNextResource( CKey* const pkey, CTimestamp timestamp );
        ERR ErrGetNextResource( CKey* const pkey, CTimestamp timestamp );

    private:
        //  Custom types (eviction index).

        typedef struct
        {
            CKey key;
            CTimestamp tsLast;
            BYTE kness;
            BYTE itsTouch;
            CTimestamp rgtsTouch[ KMax ];
        } ResourceEntry;
        typedef std::multimap<CTimestamp, ResourceEntry*> EvictionIndex;
        typedef typename EvictionIndex::iterator EvictionIndexIter;

        //  Custom types (resource hash map).

        typedef struct
        {
            EvictionIndexIter iterevict;
            BYTE iEvictIndex;
        } ResourceHashEntry;
        typedef std::unordered_map<CKey, ResourceHashEntry*> ResourceHashMap;
        typedef typename ResourceHashMap::iterator ResourceHashMapIter;

        //  Custom types (resource history hash map).

        typedef std::unordered_map<CKey, ResourceEntry*> ResourceHistoryHashMap;
        typedef typename ResourceHistoryHashMap::iterator ResourceHistoryHashMapIter;


        //  State.

        BYTE m_K;                                   //  K-ness of the algorithm.

        CTimestamp m_dtsCorrelatedTouch;            //  Correlation interval.

        CTimestamp m_time0;                         //  First timestamp reported, used as a baseline.
                                                    //  All timestamps stored will be deltas.

        bool m_fTime0init;                          //  Whether or not m_time0 is initialized.

        ResourceHashMap m_rhmCached;                //  Maps resources and their hash entries (cached entries).

        ResourceHistoryHashMap m_rhhmHistory;       //  Maps resources and their entries (history entries).

        EvictionIndex* m_rgeiCached[ KMax + 1 ];    //  Eviction indexes for cached pages (the 0th entry is the super-cold list).

        //  Helpers.

        CTimestamp TSScaleTimestamp( const CTimestamp timestamp );

        void UpdateEntryAndCache_(
            ResourceHashEntry* presHashEntry,
            ResourceEntry* const presEntry,
            const CTimestamp timestamp,
            const bool fCorrelatedTouch,
            const bool fSupercold );

        void EvictResource_( ResourceHashEntry* presHashEntry, ResourceEntry* presEntry, const bool fEvictExplicit );

        ERR ErrEvictNextResource_( CKey* const pkey, CTimestamp timestamp, const bool fEvict );

        static inline BYTE RRAdd_( const BYTE x, const BYTE y, const BYTE wrap )
        {
            return ( ( x + y ) % ( wrap + 1 ) );
        }

        static inline BYTE RRSub_( const BYTE x, const BYTE y, const BYTE wrap )
        {
            if ( x >= y )
            {
                return ( x - y );
            }
            else
            {
                return ( wrap - ( ( y - x - 1 ) % ( wrap + 1 ) ) );
            }
        }
};

template<class CKey, class CTimestamp>
inline CLRUKTestResourceUtilityManager<CKey, CTimestamp>::
CLRUKTestResourceUtilityManager():
    m_K( 0 ),
    m_time0( 0 ),
    m_fTime0init( false ),
    m_rhmCached(),
    m_rhhmHistory()
{
    memset( m_rgeiCached, 0, sizeof(m_rgeiCached) );
}

template<class CKey, class CTimestamp>
inline CLRUKTestResourceUtilityManager<CKey, CTimestamp>::
~CLRUKTestResourceUtilityManager()
{
    Term();
}

template<class CKey, class CTimestamp>
inline typename CLRUKTestResourceUtilityManager<CKey, CTimestamp>::ERR
CLRUKTestResourceUtilityManager<CKey, CTimestamp>::
ErrInit( const INT K, const double csecCorrelatedTouch )
{
    //  Input validation.

    if ( K < 1 || K > KMax )
    {
        return errInvalidParameter;
    }

    if ( ( csecCorrelatedTouch < 0.001 ) && ( csecCorrelatedTouch != 0.0 ) )
    {
        return errInvalidParameter;
    }

    //  Real initialization.

    RESMGRLRUKTESTAssert( m_K == 0 );
    m_K = (BYTE)K;
    m_dtsCorrelatedTouch = CTimestamp( csecCorrelatedTouch * 1000.0 );

    RESMGRLRUKTESTAssert( !m_fTime0init );
    m_fTime0init = false;

    RESMGRLRUKTESTAssert( m_rhmCached.empty() );
    m_rhmCached.clear();

    RESMGRLRUKTESTAssert( m_rhhmHistory.empty() );
    m_rhhmHistory.clear();

    for ( size_t i = 0; i <= m_K; i++ )
    {
        RESMGRLRUKTESTAssert( m_rgeiCached[ i ] == NULL );

        EvictionIndex* const peindex = new EvictionIndex;

        if ( peindex == NULL )
        {
            for ( size_t j = 0; j < i; j++ )
            {
                delete m_rgeiCached[ j ];
                m_rgeiCached[ j ] = NULL;
            }
            
            return errOutOfMemory;
        }

        m_rgeiCached[ i ] = peindex;
    }

    return errSuccess;
}

template<class CKey, class CTimestamp>
inline void CLRUKTestResourceUtilityManager<CKey, CTimestamp>::
Term()
{
    //  Free up all the stored entries.

    for ( size_t i = 0; i <= m_K; i++ )
    {
        EvictionIndex* const peindex = m_rgeiCached[ i ];

        if ( peindex != NULL )
        {
            RESMGRLRUKTESTAssert( m_K != 0 );
            
            for ( EvictionIndexIter iter = peindex->begin(); iter != peindex->end(); iter++ )
            {
                delete iter->second;
            }

            delete peindex;
            m_rgeiCached[ i ] = NULL;
        }
        else
        {
            RESMGRLRUKTESTAssert( m_K == 0 );
        }
    }

    //  Free up all the stored hash entries.

    for ( ResourceHashMapIter iter = m_rhmCached.begin(); iter != m_rhmCached.end(); iter++ )
    {
        delete iter->second;
    }

    //  Free up all the stored history entries.

    for ( ResourceHistoryHashMapIter iter = m_rhhmHistory.begin(); iter != m_rhhmHistory.end(); iter++ )
    {
        delete iter->second;
    }

    //  Empty containters.

    m_rhmCached.clear();
    m_rhhmHistory.clear();

    m_K = 0;
    m_fTime0init = false;
}

template<class CKey, class CTimestamp>
inline typename CLRUKTestResourceUtilityManager<CKey, CTimestamp>::ERR
CLRUKTestResourceUtilityManager<CKey, CTimestamp>::
ErrCacheResource( const CKey key, CTimestamp timestamp, const bool fUseHistory )
{
    ResourceHashMapIter iterCached = m_rhmCached.find( key );

    //  It must not be already cached.

    if ( iterCached != m_rhmCached.end() )
    {
        RESMGRLRUKTESTAssert( iterCached->second->iterevict->second->key == key );
        RESMGRLRUKTESTAssert( m_rhhmHistory.find( key ) == m_rhhmHistory.end() );
        return errResourceAlreadyCached;
    }

    ResourceHashEntry* const presHashEntry = new ResourceHashEntry;

    if ( presHashEntry == NULL )
    {
        return errOutOfMemory;
    }

    timestamp = TSScaleTimestamp( timestamp );

    //  Try to use the history record.

    ResourceEntry* presEntry = NULL;

    bool fResetEntry = true;
    ResourceHistoryHashMapIter iterHistory = m_rhhmHistory.find( key );

    if ( iterHistory != m_rhhmHistory.end() )
    {
        //  Recover from history.

        presEntry = iterHistory->second;
        RESMGRLRUKTESTAssert( presEntry != NULL );
        RESMGRLRUKTESTAssert( presEntry->key == key );
        
        m_rhhmHistory.erase( iterHistory );

        fResetEntry = !fUseHistory;
    }
    else
    {
        //  Allocate a new entry.
        
        presEntry = new ResourceEntry;

        if ( presEntry == NULL )
        {
            delete presHashEntry;
            return errOutOfMemory;
        }       
    }

    if ( fResetEntry )
    {
        presEntry->key = key;
        presEntry->tsLast = CTimestamp( 0 );
        presEntry->kness = 0;
        presEntry->itsTouch = 0;
        memset( presEntry->rgtsTouch, 0, sizeof(presEntry->rgtsTouch) );
    }

    m_rhmCached.insert( std::pair<CKey, ResourceHashEntry*>( key, presHashEntry ) );

    UpdateEntryAndCache_( presHashEntry, presEntry, timestamp, false, false );

    return errSuccess;
}

template<class CKey, class CTimestamp>
inline typename CLRUKTestResourceUtilityManager<CKey, CTimestamp>::ERR
CLRUKTestResourceUtilityManager<CKey, CTimestamp>::
ErrTouchResource( const CKey key, CTimestamp timestamp )
{
    ResourceHashMapIter iterCached = m_rhmCached.find( key );

    //  It must be already cached.

    if ( iterCached == m_rhmCached.end() )
    {
        return errResourceNotCached;
    }

    RESMGRLRUKTESTAssert( m_rhhmHistory.find( key ) == m_rhhmHistory.end() );

    ResourceHashEntry* const presHashEntry = iterCached->second;
    RESMGRLRUKTESTAssert( presHashEntry != NULL );

    ResourceEntry* const presEntry = presHashEntry->iterevict->second;
    RESMGRLRUKTESTAssert( presEntry != NULL );
    RESMGRLRUKTESTAssert( presEntry->key == key );

    timestamp = TSScaleTimestamp( timestamp );
    const bool fCorrelatedTouch = ( ( presEntry->tsLast + m_dtsCorrelatedTouch ) > timestamp );

    m_rgeiCached[ presHashEntry->iEvictIndex ]->erase( presHashEntry->iterevict );
    UpdateEntryAndCache_( presHashEntry, presEntry, timestamp, fCorrelatedTouch, false );

    return errSuccess;
}

template<class CKey, class CTimestamp>
inline typename CLRUKTestResourceUtilityManager<CKey, CTimestamp>::ERR
CLRUKTestResourceUtilityManager<CKey, CTimestamp>::
ErrMarkResourceAsSuperCold( const CKey key )
{
    ResourceHashMapIter iterCached = m_rhmCached.find( key );

    //  It must be already cached.

    if ( iterCached == m_rhmCached.end() )
    {
        return errResourceNotCached;
    }

    RESMGRLRUKTESTAssert( m_rhhmHistory.find( key ) == m_rhhmHistory.end() );

    ResourceHashEntry* const presHashEntry = iterCached->second;
    RESMGRLRUKTESTAssert( presHashEntry != NULL );

    //  Perhaps the page is already super-colded.

    RESMGRLRUKTESTAssert( presHashEntry->iEvictIndex <= m_K );
    if ( presHashEntry->iEvictIndex == 0 )
    {
        return errSuccess;
    }

    //  We'll need to move across indexes.

    ResourceEntry* const presEntry = presHashEntry->iterevict->second;
    RESMGRLRUKTESTAssert( presEntry != NULL );
    RESMGRLRUKTESTAssert( presEntry->key == key );
    
    m_rgeiCached[ presHashEntry->iEvictIndex ]->erase( presHashEntry->iterevict );
    UpdateEntryAndCache_( presHashEntry, presEntry, CTimestamp( 0 ), false, true );

    return errSuccess;
}

template<class CKey, class CTimestamp>
inline typename CLRUKTestResourceUtilityManager<CKey, CTimestamp>::ERR
CLRUKTestResourceUtilityManager<CKey, CTimestamp>::
ErrEvictResource( const CKey key )
{
    ResourceHashMapIter iterCached = m_rhmCached.find( key );

    //  Check if the resource is cached.

    if ( iterCached == m_rhmCached.end() )
    {
        return errResourceNotCached;
    }

    RESMGRLRUKTESTAssert( m_rhhmHistory.find( key ) == m_rhhmHistory.end() );

    ResourceHashEntry* const presHashEntry = iterCached->second;
    RESMGRLRUKTESTAssert( presHashEntry != NULL );

    m_rhmCached.erase( iterCached );
    EvictResource_( presHashEntry, NULL, true );

    return errSuccess;
}

template<class CKey, class CTimestamp>
inline typename CLRUKTestResourceUtilityManager<CKey, CTimestamp>::ERR
CLRUKTestResourceUtilityManager<CKey, CTimestamp>::
ErrEvictNextResource( CKey* const pkey, CTimestamp timestamp )
{
    return ErrEvictNextResource_( pkey, timestamp, true );
}

template<class CKey, class CTimestamp>
inline typename CLRUKTestResourceUtilityManager<CKey, CTimestamp>::ERR
CLRUKTestResourceUtilityManager<CKey, CTimestamp>::
ErrGetNextResource( CKey* const pkey, CTimestamp timestamp )
{
    return ErrEvictNextResource_( pkey, timestamp, false );
}

template<class CKey, class CTimestamp>
inline typename CTimestamp
CLRUKTestResourceUtilityManager<CKey, CTimestamp>::TSScaleTimestamp( const CTimestamp timestamp )
{
    if ( !m_fTime0init )
    {
        m_time0 = timestamp;
        m_fTime0init = true;
    }

    return timestamp - m_time0;
}

template<class CKey, class CTimestamp>
inline void
CLRUKTestResourceUtilityManager<CKey, CTimestamp>::
UpdateEntryAndCache_(
    ResourceHashEntry* presHashEntry,
    ResourceEntry* const presEntry,
    const CTimestamp timestamp,
    const bool fCorrelatedTouch,
    const bool fSupercold )
{
    RESMGRLRUKTESTAssert( presHashEntry != NULL );
    RESMGRLRUKTESTAssert( presEntry != NULL );
    RESMGRLRUKTESTAssert( !( fCorrelatedTouch && fSupercold ) );

    //  Update the entry if applicable.

    if ( !fSupercold )
    {
        if ( !fCorrelatedTouch )
        {
            //  Add any additional deltas originated from previous correlated touches.

            if ( ( presEntry->kness > 0 ) && ( m_K > 1 ) )
            {
                const CTimestamp tsLastStale = presEntry->rgtsTouch[ RRSub_( presEntry->itsTouch, 1, m_K - 1 ) ];
                RESMGRLRUKTESTAssert( tsLastStale <= presEntry->tsLast );
                const CTimestamp dtsCorrelation = presEntry->tsLast - tsLastStale;

                if ( dtsCorrelation > 0 )
                {
                    BYTE itsTouch = presEntry->itsTouch;
                    
                    for ( BYTE i = 0; i < min( presEntry->kness, m_K - 1 ); i++ )
                    {
                        itsTouch = RRSub_( itsTouch, 1, m_K - 1 );
                        presEntry->rgtsTouch[ itsTouch ] += dtsCorrelation;
                    }
                }
            }
            
            presEntry->rgtsTouch[ presEntry->itsTouch ] = timestamp;

            presEntry->itsTouch = RRAdd_( presEntry->itsTouch, 1, m_K - 1 );
            presEntry->kness = min( presEntry->kness + 1, m_K );
        }
        else
        {
            RESMGRLRUKTESTAssert( ( presEntry->tsLast + m_dtsCorrelatedTouch ) > timestamp );
        }

        RESMGRLRUKTESTAssert( timestamp >= presEntry->tsLast );
        presEntry->tsLast = timestamp;
    }

    //  Determine which index will receive the entry.

    presHashEntry->iEvictIndex = fSupercold ? 0 : presEntry->kness;

    //  Insert it into the appropriate index.

    presHashEntry->iterevict = m_rgeiCached[ presHashEntry->iEvictIndex ]->insert(
        std::pair<CTimestamp, ResourceEntry*>(
            presEntry->rgtsTouch[ RRSub_( presEntry->itsTouch, presEntry->kness, m_K - 1 ) ],
            presEntry ) );
}

template<class CKey, class CTimestamp>
inline void
CLRUKTestResourceUtilityManager<CKey, CTimestamp>::
EvictResource_( ResourceHashEntry* presHashEntry, ResourceEntry* presEntry, const bool fEvictExplicit )
{
    //  Either we've already handled the hash or the indexes.

    if ( presHashEntry != NULL )
    {
        RESMGRLRUKTESTAssert( presEntry == NULL );

        presEntry = presHashEntry->iterevict->second;
        m_rgeiCached[ presHashEntry->iEvictIndex ]->erase( presHashEntry->iterevict );

        RESMGRLRUKTESTAssert( m_rhmCached.find( presEntry->key ) == m_rhmCached.end() );
    }
    else if ( presEntry != NULL )
    {
        RESMGRLRUKTESTAssert( presHashEntry == NULL );

        ResourceHashMapIter iter = m_rhmCached.find( presEntry->key );
        RESMGRLRUKTESTAssert( iter != m_rhmCached.end() );

        presHashEntry = iter->second;
        RESMGRLRUKTESTAssert( iter->first == presEntry->key );
        m_rhmCached.erase( iter );
    }

    RESMGRLRUKTESTAssert( presHashEntry != NULL );
    RESMGRLRUKTESTAssert( presEntry != NULL );
    RESMGRLRUKTESTAssert( presEntry->kness > 0 );

    //  Do not add to history if:
    //    o It's coming from the supercold queue;
    //    o We are evicting explicitly;
    //    o K is 1 (i.e., nothing useful about history).

    if ( ( m_K  == 1 ) || ( presHashEntry->iEvictIndex == 0 ) || fEvictExplicit )
    {
        delete presEntry;
    }
    else
    {
        RESMGRLRUKTESTAssert( presHashEntry->iEvictIndex <= m_K );
        RESMGRLRUKTESTAssert( presHashEntry->iEvictIndex == presEntry->kness );
        m_rhhmHistory.insert( std::pair<CKey, ResourceEntry*>( presEntry->key, presEntry ) );
    }

    //  Always delete the hash entry.

    delete presHashEntry;
}

template<class CKey, class CTimestamp>
inline typename CLRUKTestResourceUtilityManager<CKey, CTimestamp>::ERR
CLRUKTestResourceUtilityManager<CKey, CTimestamp>::
ErrEvictNextResource_( CKey* const pkey, CTimestamp timestamp, const bool fEvict )
{
    //  We may not have anything to evict.

    if ( m_rhmCached.empty() )
    {
        return errNoCurrentResource;
    }

    *pkey = CKey( 0 );

    timestamp = TSScaleTimestamp( timestamp );

    //  Pages touched within the correlation interval are not evicted,
    //  except if they are in the super-cold queue. The second pass will
    //  drop this requirement if the first pass can't find anything.

    bool fEvictCorrelated = false;
    ResourceEntry* presEntry = NULL;

    do
    {
        for ( size_t i = 0; ( ( i <= m_K ) && ( presEntry == NULL ) ); i++ )
        {
            //  If we've already gone through the correlated cycle, we've already checked super-cold.

            if ( ( i == 0 ) && fEvictCorrelated )
            {
                continue;
            }

            //  Supercold queue is an exception.

            const bool fEvictCorrelatedT = fEvictCorrelated || ( i == 0 );

            EvictionIndex* const peindex = m_rgeiCached[ i ];
            EvictionIndexIter iter = peindex->begin();

            while ( iter != peindex->end() )
            {
                if ( fEvictCorrelatedT || ( ( iter->second->tsLast + m_dtsCorrelatedTouch ) <= timestamp ) )
                {
                    presEntry = iter->second;
                    if ( fEvict )
                    {
                        peindex->erase( iter );
                    }

                    break;
                }

                iter++;
            }
        }

        fEvictCorrelated = !fEvictCorrelated;
    } while ( fEvictCorrelated && ( presEntry == NULL ) );

    RESMGRLRUKTESTAssert( presEntry != NULL );

    *pkey = presEntry->key;

    if ( fEvict )
    {
        EvictResource_( NULL, presEntry, false );
    }

    return errSuccess;
}

#endif  //  _RESMGRLRUKTEST_HXX_INCLUDED

