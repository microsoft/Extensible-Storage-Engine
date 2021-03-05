// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RESMGRLRUTEST_HXX_INCLUDED
#define _RESMGRLRUTEST_HXX_INCLUDED


#ifndef RESMGRLRUTESTGetTickCount
#define RESMGRLRUTESTGetTickCount   GetTickCount
#endif



#ifndef RESMGRLRUTESTAssert
#define RESMGRLRUTESTAssert Assert
#endif



template<class CResource, PfnOffsetOf OffsetOfIC>
class CLRUTestResourceUtilityManager
{
    public:
        typedef DWORD TICK;


        class CInvasiveContext
        {
            public:
                static SIZE_T OffsetOfILE() { return OffsetOfIC() + OffsetOf( CInvasiveContext, m_ile ); }

                typename CInvasiveList<CResource, OffsetOfILE>::CElement m_ile;

                TICK m_tick;
        };


        enum ERR
        {
            errSuccess,
            errAlreadyInitialized,
            errResourceNotCached,
            errResourceAlreadyCached,
            errNoCurrentResource
        };


        CLRUTestResourceUtilityManager();
        ~CLRUTestResourceUtilityManager();


        ERR ErrInit();
        void Term();


        ERR ErrCacheResource( CResource* const pres, const ULONG pctCachePriority );
        ERR ErrTouchResource( CResource* const pres, const ULONG pctCachePriority );
        ERR ErrMarkAsSuperCold( CResource* const pres );
        ERR ErrEvictResource( CResource* const pres );
        ERR ErrEvictNextResource( CResource** const ppres );
        ERR ErrGetNextResource( CResource** const ppres );

    private:

        CInvasiveList<CResource, CInvasiveContext::OffsetOfILE> m_ilCachedResources;

        CInvasiveContext* PicFromRes_( const CResource* const pres ) const;

        TICK TickScale_( const ULONG pctCachePriority ) const;

        CResource* CResGetNextToInsert_( const TICK tick ) const;
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

    RESMGRLRUTESTAssert( m_ilCachedResources.FEmpty() );
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline typename CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ERR
CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ErrInit()
{

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

    RESMGRLRUTESTAssert( m_ilCachedResources.FEmpty() );
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline typename CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ERR
CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ErrCacheResource( CResource* const pres, const ULONG pctCachePriority )
{

    if ( m_ilCachedResources.FMember( pres ) )
    {
        return errResourceAlreadyCached;
    }


    const TICK tickScaled = TickScale_( pctCachePriority );


    CResource* const presNext = CResGetNextToInsert_( tickScaled );


    CInvasiveContext* const pic = PicFromRes_( pres );
    pic->m_tick = tickScaled;
    m_ilCachedResources.Insert( pres, presNext );

    return errSuccess;
}

template<class CResource, PfnOffsetOf OffsetOfIC>
inline typename CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ERR
CLRUTestResourceUtilityManager<CResource, OffsetOfIC>::ErrTouchResource( CResource* const pres, const ULONG pctCachePriority )
{

    if ( !m_ilCachedResources.FMember( pres ) )
    {
        return errResourceNotCached;
    }


    m_ilCachedResources.Remove( pres );


    const TICK tickScaled = TickScale_( pctCachePriority );


    CResource* const presNext = CResGetNextToInsert_( tickScaled );


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

    if ( !m_ilCachedResources.FMember( pres ) )
    {
        return errResourceNotCached;
    }


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
    

    if ( m_ilCachedResources.FEmpty() )
    {
        return errNoCurrentResource;
    }


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
    

    if ( m_ilCachedResources.FEmpty() )
    {
        return tickNow;
    }


    const CResource* const presOldest = m_ilCachedResources.PrevMost();
    const CInvasiveContext* const picOldest = PicFromRes_( presOldest );
    const TICK tickOldest = picOldest->m_tick;


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


    while ( ( presOldest != NULL ) && ( presNewest != NULL ) )
    {

        const CInvasiveContext* const picNewest = PicFromRes_( presNewest );
        if ( (LONG)( tick - picNewest->m_tick ) >= 0 )
        {
            pres = m_ilCachedResources.Next( presNewest );
            break;
        }
        presNewest = m_ilCachedResources.Prev( presNewest );


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

#endif

