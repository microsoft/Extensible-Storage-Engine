// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#include "resmgrlrutestif.hxx"



PageEvictionAlgorithmLRUTest::PageEvictionAlgorithmLRUTest() : 
    IPageEvictionAlgorithmImplementation(),
    m_lrutest()
{
}

PageEvictionAlgorithmLRUTest::~PageEvictionAlgorithmLRUTest()
{
}

ERR PageEvictionAlgorithmLRUTest::ErrInit( const BFTRACE::BFSysResMgrInit_& bfinit )
{
    const CLRUTestResourceUtilityManagerERR err = m_lrutest.ErrInit();

    if ( err != CLRUTestResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    return JET_errSuccess;
}

void PageEvictionAlgorithmLRUTest::Term( const BFTRACE::BFSysResMgrTerm_& bfterm )
{
    m_lrutest.Term();
}

ERR PageEvictionAlgorithmLRUTest::ErrCachePage( PAGE* const ppage, const BFTRACE::BFCache_& bfcache )
{
    const CLRUTestResourceUtilityManagerERR err = m_lrutest.ErrCacheResource( (BFRESMGRLRUTEST*)ppage, bfcache.pctPri );

    if ( err != CLRUTestResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmLRUTest::ErrTouchPage( PAGE* const ppage, const BFTRACE::BFTouch_& bftouch )
{
    const CLRUTestResourceUtilityManagerERR err = m_lrutest.ErrTouchResource( (BFRESMGRLRUTEST*)ppage, bftouch.pctPri );

    if ( err != CLRUTestResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmLRUTest::ErrSuperColdPage( PAGE* const ppage, const BFTRACE::BFSuperCold_& bfsupercold )
{
    const CLRUTestResourceUtilityManagerERR err = m_lrutest.ErrMarkAsSuperCold( (BFRESMGRLRUTEST*)ppage );

    if ( err != CLRUTestResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmLRUTest::ErrEvictSpecificPage( PAGE* const ppage, const BFTRACE::BFEvict_& bfevict )
{
    const CLRUTestResourceUtilityManagerERR err = m_lrutest.ErrEvictResource( (BFRESMGRLRUTEST*)ppage );

    if ( err != CLRUTestResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmLRUTest::ErrEvictNextPage( void* const pv, const BFTRACE::BFEvict_& bfevict, PageEvictionContextType* const ppectyp )
{
    PAGE** const pppage = (PAGE**)pv;
    *ppectyp = pectypPAGE;

    CLRUTestResourceUtilityManagerERR err = m_lrutest.ErrEvictNextResource( (BFRESMGRLRUTEST**)pppage );


    if ( err == CLRUTestResourceUtilityManagerType::errNoCurrentResource )
    {
        err = CLRUTestResourceUtilityManagerType::errSuccess;
        *pppage = NULL;
    }

    if ( err != CLRUTestResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmLRUTest::ErrGetNextPageToEvict( void* const pv, PageEvictionContextType* const ppectyp )
{
    PAGE** const pppage = (PAGE**)pv;
    *ppectyp = pectypPAGE;

    CLRUTestResourceUtilityManagerERR err = m_lrutest.ErrGetNextResource( (BFRESMGRLRUTEST**)pppage );


    if ( err == CLRUTestResourceUtilityManagerType::errNoCurrentResource )
    {
        err = CLRUTestResourceUtilityManagerType::errSuccess;
        *pppage = NULL;
    }

    if ( err != CLRUTestResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    return JET_errSuccess;
}

size_t PageEvictionAlgorithmLRUTest::CbPageContext() const
{
    return sizeof(BFRESMGRLRUTEST);
}

void PageEvictionAlgorithmLRUTest::InitPage( PAGE* const ppage )
{
    new ( (BFRESMGRLRUTEST*)ppage ) BFRESMGRLRUTEST;
}

void PageEvictionAlgorithmLRUTest::DestroyPage( PAGE* const ppage )
{
    ( (BFRESMGRLRUTEST*)ppage )->~BFRESMGRLRUTEST();
}

