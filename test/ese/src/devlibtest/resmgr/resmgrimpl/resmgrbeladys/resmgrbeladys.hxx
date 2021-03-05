// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RESMGRBELADYS_HXX_INCLUDED
#define _RESMGRBELADYS_HXX_INCLUDED



#ifndef RESMGRBELADYSAssert
#define RESMGRBELADYSAssert Assert
#endif

#include <unordered_map>
#include <vector>
#include <map>


template<class CKey, class CTimestamp>
class CBeladysResourceUtilityManager
{
    public:

        enum ERR
        {
            errSuccess,
            errOutOfMemory,
            errInvalidOperation,
            errResourceNotCached,
            errResourceAlreadyCached,
            errNoCurrentResource
        };



        CBeladysResourceUtilityManager();
        ~CBeladysResourceUtilityManager();



        ERR ErrInit();
        void Term();



        ERR ErrCacheResource( const CKey key, CTimestamp timestamp );
        ERR ErrTouchResource( const CKey key, CTimestamp timestamp );
        ERR ErrEvictResource( const CKey key );
        ERR ErrEvictBestNextResource( CKey* const pkey );
        ERR ErrEvictWorstNextResource( CKey* const pkey );
        ERR ErrGetBestNextResource( CKey* const pkey );
        ERR ErrGetWorstNextResource( CKey* const pkey );



        bool FProcessing() const;
        ERR ErrStartProcessing();
        ERR ErrResetProcessing();

    private:

        typedef std::multimap<CTimestamp, CKey> EvictionIndex;
        typedef typename EvictionIndex::iterator EvictionIndexIter;



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



        static const CTimestamp s_terminator = CTimestamp( 0 ) - 1;



        bool m_fProcessing;

        CTimestamp m_time0;

        bool m_fTime0init;

        ResourceRequestMap m_rrmap;

        EvictionIndex m_eindex;



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


    if ( iter == m_rrmap.end() )
    {
        return errInvalidOperation;
    }

    RESMGRBELADYSAssert( iter->first == key );

    RequestTimeSequence* const prts = iter->second;


    if ( prts->iterevict != m_eindex.end() )
    {
        return errResourceAlreadyCached;
    }


    timestamp = TSScaleTimestamp( timestamp );
    RESMGRBELADYSAssert( timestamp != s_terminator );

    const CTimestamp timestampNext = TSGetNextRequest( prts, timestamp );


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


    if ( iter == m_rrmap.end() )
    {
        return errInvalidOperation;
    }

    RESMGRBELADYSAssert( iter->first == key );

    RequestTimeSequence* const prts = iter->second;


    if ( prts->iterevict == m_eindex.end() )
    {
        return errResourceNotCached;
    }

    RESMGRBELADYSAssert( prts->iterevict->second == key );


    timestamp = TSScaleTimestamp( timestamp );
    RESMGRBELADYSAssert( timestamp != s_terminator );

    const CTimestamp timestampNext = TSGetNextRequest( prts, timestamp );


    m_eindex.erase( prts->iterevict );
    prts->iterevict = m_eindex.insert( std::pair<CTimestamp, CKey>( timestampNext, key ) );

    return errSuccess;
}

template<class CKey, class CTimestamp>
inline typename CBeladysResourceUtilityManager<CKey, CTimestamp>::ERR
CBeladysResourceUtilityManager<CKey, CTimestamp>::
ErrEvictResource( const CKey key )
{

    if ( !m_fProcessing )
    {
        return errSuccess;
    }
    
    ResourceRequestMapIter iter = m_rrmap.find( key );


    if ( iter == m_rrmap.end() )
    {
        return errInvalidOperation;
    }

    RESMGRBELADYSAssert( iter->first == key );

    RequestTimeSequence* const prts = iter->second;


    if ( prts->iterevict == m_eindex.end() )
    {
        return errResourceNotCached;
    }

    RESMGRBELADYSAssert( prts->iterevict->second == key );


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


    if ( !m_fProcessing )
    {
        return errSuccess;
    }


    if ( m_eindex.empty() )
    {
        return errNoCurrentResource;
    }


    EvictionIndexIter iterevict = fBest ? std::prev( m_eindex.end() ) : m_eindex.begin();

    const CKey key = iterevict->second;

    ResourceRequestMapIter iter = m_rrmap.find( key );


    RESMGRBELADYSAssert( iter != m_rrmap.end() );
    RESMGRBELADYSAssert( iter->first == key );

    RequestTimeSequence* const prts = iter->second;


    RESMGRBELADYSAssert( prts->iterevict == iterevict );

    if ( fEvict )
    {

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

    if ( !m_fProcessing )
    {
        return errInvalidOperation;
    }


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

#endif

