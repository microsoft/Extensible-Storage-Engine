// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#include "resmgrlrukeseif.hxx"



PageEvictionAlgorithmLRUKESE::PageEvictionAlgorithmLRUKESE() : 
    IPageEvictionAlgorithmImplementation(),
    m_lruk( rankBFLRUK )
{
}

PageEvictionAlgorithmLRUKESE::~PageEvictionAlgorithmLRUKESE()
{
}

ERR PageEvictionAlgorithmLRUKESE::ErrInit( const BFTRACE::BFSysResMgrInit_& bfinit )
{
    if ( bfinit.K > Kmax )
    {
        return ErrERRCheck( JET_errTestError );
    }
    
    const BFLRUK::ERR err = m_lruk.ErrInit( bfinit.K,
                                            bfinit.csecCorrelatedTouch,
                                            bfinit.csecTimeout,
                                            bfinit.csecUncertainty,
                                            bfinit.dblHashLoadFactor,
                                            bfinit.dblHashUniformity,
                                            bfinit.dblSpeedSizeTradeoff );

    if ( err != BFLRUK::ERR::errSuccess )
    {
        if ( err == BFLRUK::ERR::errOutOfMemory )
        {
            return ErrERRCheck( JET_errOutOfMemory );
        }
        else
        {
            return ErrERRCheck( JET_errTestError );
        }
    }

    return JET_errSuccess;
}

void PageEvictionAlgorithmLRUKESE::Term( const BFTRACE::BFSysResMgrTerm_& bfterm )
{
    m_lruk.Term();
}

ERR PageEvictionAlgorithmLRUKESE::ErrCachePage( PAGE* const ppage, const BFTRACE::BFCache_& bfcache )
{
    const BFLRUK::ERR err = m_lruk.ErrCacheResource( IFMPPGNO( bfcache.ifmp, bfcache.pgno ),
                                                        (BFRESMGRLRUKESE*)ppage,
                                                        TickRESMGRTimeCurrent(),
                                                        bfcache.pctPri,
                                                        bfcache.fUseHistory );

    if ( err != BFLRUK::ERR::errSuccess )
    {
        if ( err == BFLRUK::ERR::errOutOfMemory )
        {
            return ErrERRCheck( JET_errOutOfMemory );
        }
        else
        {
            return ErrERRCheck( JET_errTestError );
        }
    }

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmLRUKESE::ErrTouchPage( PAGE* const ppage, const BFTRACE::BFTouch_& bftouch )
{
    (void)m_lruk.RmtfTouchResource( (BFRESMGRLRUKESE*)ppage, bftouch.pctPri, TickRESMGRTimeCurrent() );

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmLRUKESE::ErrSuperColdPage( PAGE* const ppage, const BFTRACE::BFSuperCold_& bfsupercold )
{
    m_lruk.MarkAsSuperCold( (BFRESMGRLRUKESE*)ppage );

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmLRUKESE::ErrEvictSpecificPage( PAGE* const ppage, const BFTRACE::BFEvict_& bfevict )
{
    const BFLRUK::ERR err = m_lruk.ErrEvictResource( IFMPPGNO( bfevict.ifmp, bfevict.pgno ),
                                                        (BFRESMGRLRUKESE*)ppage,
                                                        bfevict.bfef & bfefKeepHistory );

    if ( err != BFLRUK::ERR::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmLRUKESE::ErrEvictNextPage( void* const pv, const BFTRACE::BFEvict_& bfevict, PageEvictionContextType* const ppectyp )
{
    return ErrEvictNextPage_( pv, bfevict, ppectyp, true );
}

ERR PageEvictionAlgorithmLRUKESE::ErrGetNextPageToEvict( void* const pv, PageEvictionContextType* const ppectyp )
{
    BFTRACE::BFEvict_ bfevictDummy = { 0 };
    return ErrEvictNextPage_( pv, bfevictDummy, ppectyp, false );
}

size_t PageEvictionAlgorithmLRUKESE::CbPageContext() const
{
    return sizeof(BFRESMGRLRUKESE);
}

void PageEvictionAlgorithmLRUKESE::InitPage( PAGE* const ppage )
{
    new ( (BFRESMGRLRUKESE*)ppage ) BFRESMGRLRUKESE;
}

void PageEvictionAlgorithmLRUKESE::DestroyPage( PAGE* const ppage )
{
    ( (BFRESMGRLRUKESE*)ppage )->~BFRESMGRLRUKESE();
}

ERR PageEvictionAlgorithmLRUKESE::ErrEvictNextPage_( void* const pv, const BFTRACE::BFEvict_& bfevict, PageEvictionContextType* const ppectyp, const bool fEvict )
{
    BFLRUK::ERR errLRUK;
    BFLRUK::CLock lockLRUK;
    PAGE** const pppage = (PAGE**)pv;
    *ppectyp = pectypPAGE;

    m_lruk.BeginResourceScan( &lockLRUK );
    errLRUK = m_lruk.ErrGetNextResource( &lockLRUK, (BFRESMGRLRUKESE**)pppage );


    if ( errLRUK == BFLRUK::ERR::errNoCurrentResource )
    {
        errLRUK = BFLRUK::ERR::errSuccess;
        *pppage = NULL;
    }
    else
    {
        BFRESMGRLRUKESE* ppageLocked;
        errLRUK = m_lruk.ErrGetCurrentResource( &lockLRUK, &ppageLocked );

        if ( errLRUK == BFLRUK::ERR::errSuccess )
        {
            Enforce( *pppage == ppageLocked );

            if ( fEvict )
            {
                errLRUK = m_lruk.ErrEvictCurrentResource( &lockLRUK, 
                                                          (*pppage)->ifmppgno,
                                                          bfevict.bfef & bfefKeepHistory );
            }
        }
    }

    m_lruk.EndResourceScan( &lockLRUK );

    if ( errLRUK != BFLRUK::ERR::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    return JET_errSuccess;
}

