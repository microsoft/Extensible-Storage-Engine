// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RESMGRLRUTEST_HXX_INCLUDED
#define _RESMGRLRUTEST_HXX_INCLUDED

//  We allow modules using us to override our time function.

#ifndef RESMGRLRUTESTGetTickCount
#define RESMGRLRUTESTGetTickCount   GetTickCount
#endif  //  RESMGRLRUTESTGetTickCount


//  We allow modules using us to override our assert function.

#ifndef RESMGRLRUTESTAssert
#define RESMGRLRUTESTAssert Assert
#endif  //  RESMGRLRUTESTAssert


//  Implementation of an inefficient LRU algorithm for testing purposes.

//  ================================================================
template<class CResource, PfnOffsetOf OffsetOfIC>
class CLRUTestResourceUtilityManager
//  ================================================================
{
    public:
        typedef DWORD TICK; //  Timestamp used for resource touch times.

        //  Class containing context needed by CResource.

        class CInvasiveContext
        {
            public:
                static SIZE_T OffsetOfILE() { return OffsetOfIC() + OffsetOf( CInvasiveContext, m_ile ); }  //  CInvasiveList glue.

                typename CInvasiveList<CResource, OffsetOfILE>::CElement m_ile;                             //  CInvasiveList glue.

                TICK m_tick;                                                                                //  Tick when it was last touched (scaled).
        };

        //  API Error Codes

        enum ERR
        {
            errSuccess,
            errAlreadyInitialized,
            errResourceNotCached,
            errResourceAlreadyCached,
            errNoCurrentResource
        };

        //  Ctor./Dtor.

        CLRUTestResourceUtilityManager();
        ~CLRUTestResourceUtilityManager();

        //  Init./term.

        ERR ErrInit();
        void Term();

        //  Cache/touch/evict.

        ERR ErrCacheResource( CResource* const pres, const ULONG pctCachePriority );
        ERR ErrTouchResource( CResource* const pres, const ULONG pctCachePriority );
        ERR ErrMarkAsSuperCold( CResource* const pres );
        ERR ErrEvictResource( CResource* const pres );
        ERR ErrEvictNextResource( CResource** const ppres );
        ERR ErrGetNextResource( CResource** const ppres );

    private:

        CInvasiveList<CResource, CInvasiveContext::OffsetOfILE> m_ilCachedResources;    //  List of cached resources.

        CInvasiveContext* PicFromRes_( const CResource* const pres ) const;                 //  Returns a pointer to the invasive context, given the resource.

        TICK TickScale_( const ULONG pctCachePriority ) const;                          //  Returns the current tick scaled.

        CResource* CResGetNextToInsert_( const TICK tick ) const;                       //  Returns the resource that should follow the one we want
};

template<class CResource, PfnOffsetOf OffsetOfIC>
inline CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::
CLRUTestResourceUtilityManager()
    :   m_ilCachedResources()
{
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::
~CLRUTestResourceUtilityManager()
{
    //  The client must have evicted all the resources.

    RESMGRLRUTESTAssert( m_ilCachedResources.FEmpty() );
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline typename CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ERR
CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ErrInit()
{
    //  If the list is not empty, it means we are already initialized.

    if ( !m_ilCachedResources.FEmpty() )
    {
        return errAlreadyInitialized;
    }
    
    return errSuccess;
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline void
CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::Term()
{
    //  The client must have evicted all the resources.

    RESMGRLRUTESTAssert( m_ilCachedResources.FEmpty() );
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline typename CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ERR
CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ErrCacheResource( CResource* const pres, const ULONG pctCachePriority )
{
    //  Fail out if the resource is already cached.

    if ( m_ilCachedResources.FMember( pres ) )
    {
        return errResourceAlreadyCached;
    }

    //  Scale the tick.

    const TICK tickScaled = TickScale_( pctCachePriority );

    //  Where should it be inserted?

    CResource* const presNext = CResGetNextToInsert_( tickScaled );

    //  Insert it.

    CInvasiveContext* const pic = PicFromRes_( pres );
    pic->m_tick = tickScaled;
    m_ilCachedResources.Insert( pres, presNext );

    return errSuccess;
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline typename CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ERR
CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ErrTouchResource( CResource* const pres, const ULONG pctCachePriority )
{
    //  Fail out if the resource is not cached yet.

    if ( !m_ilCachedResources.FMember( pres ) )
    {
        return errResourceNotCached;
    }

    //  Remove it.

    m_ilCachedResources.Remove( pres );

    //  Scale the tick.

    const TICK tickScaled = TickScale_( pctCachePriority );

    //  Where should it be inserted?

    CResource* const presNext = CResGetNextToInsert_( tickScaled );

    //  Insert it.

    CInvasiveContext* const pic = PicFromRes_( pres );
    pic->m_tick = tickScaled;
    m_ilCachedResources.Insert( pres, presNext );

    return errSuccess;
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline typename CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ERR
CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ErrMarkAsSuperCold( CResource* const pres )
{
    return errSuccess;
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline typename CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ERR
CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ErrEvictResource( CResource* const pres )
{
    //  Fail out if the resource is not cached yet.

    if ( !m_ilCachedResources.FMember( pres ) )
    {
        return errResourceNotCached;
    }

    //  Remove it.

    m_ilCachedResources.Remove( pres );

    return errSuccess;
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline typename CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ERR
CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ErrEvictNextResource( CResource** const ppres )
{
    const ERR err = ErrGetNextResource( ppres );

    if ( err != errSuccess )
    {
        return err;
    }

    m_ilCachedResources.Remove( *ppres );

    return errSuccess;
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline typename CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ERR
CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ErrGetNextResource( CResource** const ppres )
{
    *ppres = NULL;
    
    //  Fail out if the list is empty.

    if ( m_ilCachedResources.FEmpty() )
    {
        return errNoCurrentResource;
    }

    //  The next is the oldest.

    *ppres = m_ilCachedResources.PrevMost();

    return errSuccess;
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline typename CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::CInvasiveContext*
CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::PicFromRes_( const CResource* const pres ) const
{
    return (CInvasiveContext*)( (BYTE*)pres + OffsetOfIC() );
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline typename CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::TICK
CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::TickScale_( const ULONG pctCachePriority ) const
{
    const TICK tickNow = RESMGRLRUTESTGetTickCount();
    
    //  If the list is empty, return the current tick.

    if ( m_ilCachedResources.FEmpty() )
    {
        return tickNow;
    }

    //  Range.

    const CResource* const presOldest = m_ilCachedResources.PrevMost();
    const CInvasiveContext* const picOldest = PicFromRes_( presOldest );
    const TICK tickOldest = picOldest->m_tick;

    // Short-circuit common case.

    if ( pctCachePriority == 100 )
    {
        return tickNow;
    }

    return ( tickOldest + (TICK)( ( (QWORD)( tickNow - tickOldest ) * pctCachePriority ) / 100 ) );
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline CResource*
CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::CResGetNextToInsert_( const TICK tick ) const
{
    CResource* presOldest = m_ilCachedResources.PrevMost();
    CResource* presNewest = m_ilCachedResources.NextMost();
    CResource* pres = NULL;

    //  Iterate through until we find an entry with a newer tick, from both sides.

    while ( ( presOldest != NULL ) && ( presNewest != NULL ) )
    {
        //  Newest first.

        const CInvasiveContext* const picNewest = PicFromRes_( presNewest );
        if ( (LONG)( tick - picNewest->m_tick ) >= 0 )
        {
            pres = m_ilCachedResources.Next( presNewest );
            break;
        }
        presNewest = m_ilCachedResources.Prev( presNewest );

        //  Oldest now.

        const CInvasiveContext* const picOldest = PicFromRes_( presOldest );
        if ( (LONG)( tick - picOldest->m_tick ) < 0 )
        {
            pres = presOldest;
            break;
        }
        presOldest = m_ilCachedResources.Next( presOldest );
    }

    return pres;
}

#endif  //  _RESMGRLRUTEST_HXX_INCLUDED

