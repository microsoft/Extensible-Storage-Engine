// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RESMGRBELADYS_HXX_INCLUDED
#define _RESMGRBELADYS_HXX_INCLUDED


//  We allow modules using us to override our assert function.

#ifndef RESMGRBELADYSAssert
#define RESMGRBELADYSAssert Assert
#endif  //  RESMGRBELADYSAssert

#include <unordered_map>
#include <vector>
#include <map>

//  Implementation of the Bélády's (a.k.a. Clairvoyant, a.k.a. OPT) algorithm.
//
//  The algorithm requires a pre-processing phase, where all the page requests
//  will be partitioned according to their resource IDs (ifmp/pgno in ESE). A hash
//  map will be built where the key will be the res. ID and the data will be a
//  sorted list of request timestamps.
//
//  Once such a list is built, the CBeladysResourceUtilityManager object then is
//  switched to the processing phase. Upon each page request (cache/touch), the
//  resource is looked up in the hash map and is added to a list sorted by tick,
//  with the tick of the next request. The request iterator is then advanced. If
//  the resource is already on the list, it gets re-added with the updated tick.
//
//  At eviction time, the resource suggested for eviction is the one with the
//  highest tick in the sorted list of resources.
//
//  The algorithm assumes resource request timestamps in ascending order.

//  ================================================================
template<class CKey, class CTimestamp>
class CBeladysResourceUtilityManager
//  ================================================================
{
    public:
        //  API error codes.

        enum ERR
        {
            errSuccess,
            errOutOfMemory,
            errInvalidOperation,
            errResourceNotCached,
            errResourceAlreadyCached,
            errNoCurrentResource
        };


        //  Ctor./Dtor.

        CBeladysResourceUtilityManager();
        ~CBeladysResourceUtilityManager();


        //  Init./term.

        ERR ErrInit();
        void Term();


        //  Cache/touch/evict.

        ERR ErrCacheResource( const CKey key, CTimestamp timestamp );
        ERR ErrTouchResource( const CKey key, CTimestamp timestamp );
        ERR ErrEvictResource( const CKey key );
        ERR ErrEvictBestNextResource( CKey* const pkey );
        ERR ErrEvictWorstNextResource( CKey* const pkey );
        ERR ErrGetBestNextResource( CKey* const pkey );
        ERR ErrGetWorstNextResource( CKey* const pkey );


        //  Transition functions.

        bool FProcessing() const;
        ERR ErrStartProcessing();
        ERR ErrResetProcessing();

    private:
        //  Custom types (eviction index).

        typedef std::multimap<CTimestamp, CKey> EvictionIndex;
        typedef typename EvictionIndex::iterator EvictionIndexIter;


        //  Custom types (resource-timestamp map).

        typedef std::vector<CTimestamp> RequestSequence;
        typedef typename RequestSequence::iterator RequestSequenceIter;
        typedef struct
        {
            RequestSequence rseq;
            RequestSequenceIter iterseq;
            EvictionIndexIter iterevict;
        } RequestTimeSequence;
        typedef std::unordered_map<CKey, RequestTimeSequence*> ResourceRequestMap;
        typedef typename ResourceRequestMap::iterator ResourceRequestMapIter;


        //  Constants.

        static const CTimestamp s_terminator = CTimestamp( 0 ) - 1;


        //  State.

        bool m_fProcessing;                 //  Whether we are processing traces (as opposed to only pre-processing).

        CTimestamp m_time0;                 //  First timestamp reported, used as a baseline.
                                            //  All timestamps stored will be deltas.

        bool m_fTime0init;                  //  Whether or not m_time0 is initialized.

        ResourceRequestMap m_rrmap;         //  Maps resources and their list of timestamps.

        EvictionIndex m_eindex;             //  Eviction index.


        //  Helpers.

        ERR ErrRequestResource( const CKey key, CTimestamp timestamp );
        ERR ErrEvictNextResource( CKey* const pkey, const bool fBest );
        ERR ErrEvictNextResource( CKey* const pkey, const bool fBest, const bool fEvict );
        ERR ErrGetNextResource( CKey* const pkey, const bool fBest );
        ERR ErrResetProcessing( const bool fResetTimestampSequence );
        CTimestamp TSGetNextRequest( RequestTimeSequence* const prts, const CTimestamp timestamp );
        CTimestamp TSScaleTimestamp( const CTimestamp ts );
};

template<class CKey, class CTimestamp>
inline CBeladysResourceUtilityManager<CKey, CTimestamp>::
CBeladysResourceUtilityManager():
    m_fProcessing( false ),
    m_time0( 0 ),
    m_fTime0init( false ),
    m_rrmap(),
    m_eindex()
{
}

template<class CKey, class CTimestamp>
inline CBeladysResourceUtilityManager<CKey, CTimestamp>::
~CBeladysResourceUtilityManager()
{
    //  Free up all the stored time sequences.

    for ( ResourceRequestMapIter iter = m_rrmap.begin(); iter != m_rrmap.end(); iter++ )
    {
        delete iter->second;
    }
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrInit()
{
    //  Either we're not processing yet or the client should have evicted.

    RESMGRBELADYSAssert( m_eindex.empty() );

    if ( m_fProcessing )
    {
        return ErrResetProcessing( false );
    }

    return errSuccess;
}

template<class CKey, class CTimestamp>
inline void
CBeladysResourceUtilityManager<CKey, CTimestamp>::Term()
{
    if ( m_fProcessing )
    {
        const ERR err = ErrResetProcessing( false );

        RESMGRBELADYSAssert( err == errSuccess );
        
        return;
    }

    //  If collecting traces, stamp a terminator to everyone's list.

    for ( ResourceRequestMapIter iter = m_rrmap.begin(); iter != m_rrmap.end(); iter++ )
    {
        iter->second->rseq.push_back( s_terminator );
    }
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrCacheResource( const CKey key, CTimestamp timestamp )
{
    if ( !m_fProcessing )
    {
        return ErrRequestResource( key, timestamp );
    }

    ResourceRequestMapIter iter = m_rrmap.find( key );

    //  It must have shown up previously during pre-processing.

    if ( iter == m_rrmap.end() )
    {
        return errInvalidOperation;
    }

    RESMGRBELADYSAssert( iter->first == key );

    RequestTimeSequence* const prts = iter->second;

    //  Must not be pointing to an entry in the eviction index.

    if ( prts->iterevict != m_eindex.end() )
    {
        return errResourceAlreadyCached;
    }

    //  Search for a matching timestamp.

    timestamp = TSScaleTimestamp( timestamp );
    RESMGRBELADYSAssert( timestamp != s_terminator );

    const CTimestamp timestampNext = TSGetNextRequest( prts, timestamp );

    //  Now, insert into the index.

    prts->iterevict = m_eindex.insert( std::pair<CTimestamp, CKey>( timestampNext, key ) );

    return errSuccess;
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrTouchResource( const CKey key, CTimestamp timestamp )
{
    if ( !m_fProcessing )
    {
        return ErrRequestResource( key, timestamp );
    }
    
    ResourceRequestMapIter iter = m_rrmap.find( key );

    //  It must have shown up previously during pre-processing.

    if ( iter == m_rrmap.end() )
    {
        return errInvalidOperation;
    }

    RESMGRBELADYSAssert( iter->first == key );

    RequestTimeSequence* const prts = iter->second;

    //  Must be pointing to an entry in the eviction index.

    if ( prts->iterevict == m_eindex.end() )
    {
        return errResourceNotCached;
    }

    RESMGRBELADYSAssert( prts->iterevict->second == key );

    //  Search for a matching timestamp.

    timestamp = TSScaleTimestamp( timestamp );
    RESMGRBELADYSAssert( timestamp != s_terminator );

    const CTimestamp timestampNext = TSGetNextRequest( prts, timestamp );

    //  Now, update the index.

    m_eindex.erase( prts->iterevict );
    prts->iterevict = m_eindex.insert( std::pair<CTimestamp, CKey>( timestampNext, key ) );

    return errSuccess;
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrEvictResource( const CKey key )
{
    //  Do nothing if just accumulating traces.

    if ( !m_fProcessing )
    {
        return errSuccess;
    }
    
    ResourceRequestMapIter iter = m_rrmap.find( key );

    //  It must have shown up previously during pre-processing.

    if ( iter == m_rrmap.end() )
    {
        return errInvalidOperation;
    }

    RESMGRBELADYSAssert( iter->first == key );

    RequestTimeSequence* const prts = iter->second;

    //  Must be pointing to an entry in the eviction index.

    if ( prts->iterevict == m_eindex.end() )
    {
        return errResourceNotCached;
    }

    RESMGRBELADYSAssert( prts->iterevict->second == key );

    //  Now, delete from the index.

    m_eindex.erase( prts->iterevict );
    prts->iterevict = m_eindex.end();

    return errSuccess;
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrEvictBestNextResource( CKey* const pkey )
{
    return ErrEvictNextResource( pkey, true );
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrEvictWorstNextResource( CKey* const pkey )
{
    return ErrEvictNextResource( pkey, false );
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrGetBestNextResource( CKey* const pkey )
{
    return ErrGetNextResource( pkey, true );
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrGetWorstNextResource( CKey* const pkey )
{
    return ErrGetNextResource( pkey, false );
}

template<class CKey, class CTimestamp>
inline typename bool
CBeladysResourceUtilityManager<CKey, CTimestamp>::
FProcessing() const
{
    return m_fProcessing;
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrStartProcessing()
{
    if ( m_fProcessing )
    {
        return errInvalidOperation;
    }
    
    //  We're not processing yet.

    RESMGRBELADYSAssert( m_eindex.empty() );    
    
    m_fProcessing = true;

    return ErrResetProcessing();
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrResetProcessing()
{
    return ErrResetProcessing( true );
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrRequestResource( const CKey key, CTimestamp timestamp )
{
    RESMGRBELADYSAssert( !m_fProcessing );

    timestamp = TSScaleTimestamp( timestamp );

    //  Check if we already have this resource in our map.

    ResourceRequestMapIter iter = m_rrmap.find( key );
    RequestTimeSequence* prts = NULL;
    if ( iter == m_rrmap.end() )
    {
        prts = new RequestTimeSequence;
        if ( !prts )
        {
            return errOutOfMemory;
        }

        prts->rseq.push_back( timestamp );
        prts->iterseq = prts->rseq.begin();
        prts->iterevict = m_eindex.end();
        m_rrmap.insert( std::pair<CKey, RequestTimeSequence*>( key, prts ) );
    }
    else
    {
        prts = iter->second;
        prts->rseq.push_back( timestamp );
    }

    return errSuccess;
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrEvictNextResource( CKey* const pkey, const bool fBest )
{
    return ErrEvictNextResource( pkey, fBest, true );
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrGetNextResource( CKey* const pkey, const bool fBest )
{
    return ErrEvictNextResource( pkey, fBest, false );
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrEvictNextResource( CKey* const pkey, const bool fBest, const bool fEvict )
{
    *pkey = CKey( 0 );

    //  Do nothing if just accumulating traces.

    if ( !m_fProcessing )
    {
        return errSuccess;
    }

    //  Nothing to return if it's empty.

    if ( m_eindex.empty() )
    {
        return errNoCurrentResource;
    }

    //  Evict last element.

    EvictionIndexIter iterevict = fBest ? std::prev( m_eindex.end() ) : m_eindex.begin();

    const CKey key = iterevict->second;

    ResourceRequestMapIter iter = m_rrmap.find( key );

    //  It must have shown up previously during pre-processing.

    RESMGRBELADYSAssert( iter != m_rrmap.end() );
    RESMGRBELADYSAssert( iter->first == key );

    RequestTimeSequence* const prts = iter->second;

    //  Must be pointing to the right entry in the eviction index.

    RESMGRBELADYSAssert( prts->iterevict == iterevict );

    if ( fEvict )
    {
        //  Now, delete from the index.

        m_eindex.erase( iterevict );
        prts->iterevict = m_eindex.end();
    }

    *pkey = key;

    return errSuccess;
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrResetProcessing( const bool fResetTimestampSequence )
{
    //  Only valid to call this if currently processing.

    if ( !m_fProcessing )
    {
        return errInvalidOperation;
    }

    //  Reset state if processing.

    m_eindex.clear();

    for ( ResourceRequestMapIter iter = m_rrmap.begin(); iter != m_rrmap.end(); iter++ )
    {
        RequestTimeSequence* const prts = iter->second;
        RESMGRBELADYSAssert( !prts->rseq.empty() );
        prts->iterevict = m_eindex.end();
        if ( fResetTimestampSequence )
        {
            prts->iterseq = prts->rseq.begin();
        }
    }

    return errSuccess;
}

template<class CKey, class CTimestamp>
inline typename CTimestamp
CBeladysResourceUtilityManager<CKey, CTimestamp>::TSGetNextRequest( RequestTimeSequence* const prts, const CTimestamp timestamp )
{
    CTimestamp timestampNext = s_terminator;

    RESMGRBELADYSAssert( timestamp != s_terminator );

    //  There are a few cases (no-touch flag, for example) where a request in the processing phase may
    //  not have shown up during pre-processing. In that case, stop the lookup once we move to a timestamp
    //  that is higher than what we're looking for and return that as the next.

    while ( ( prts->iterseq != prts->rseq.end() ) &&
        ( ( timestampNext = *( prts->iterseq ) ) != timestamp ) )
    {
        if ( ( timestampNext == s_terminator ) || ( timestampNext < timestamp ) )
        {
            prts->iterseq++;
        }
        else
        {
            return timestampNext;
        }
    }

    //  Finally, move to next.

    if ( prts->iterseq != prts->rseq.end() )
    {
        prts->iterseq++;
    }

    timestampNext = ( prts->iterseq != prts->rseq.end() ) ? *( prts->iterseq ) : s_terminator;

    return timestampNext;
}

template<class CKey, class CTimestamp>
inline typename CTimestamp
CBeladysResourceUtilityManager<CKey, CTimestamp>::TSScaleTimestamp( const CTimestamp ts )
{
    if ( !m_fTime0init )
    {
        m_time0 = ts;
        m_fTime0init = true;
    }

    return ts - m_time0;
}

#endif  //  _RESMGRBELADYS_HXX_INCLUDED

