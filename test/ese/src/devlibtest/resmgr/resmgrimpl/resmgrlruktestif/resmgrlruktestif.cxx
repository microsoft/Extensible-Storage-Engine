// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  Header files.

#include "resmgrlruktestif.hxx"


//  class PageEvictionAlgorithmLRUKTest.

PageEvictionAlgorithmLRUKTest::PageEvictionAlgorithmLRUKTest( const bool fBest ) : 
    IPageEvictionAlgorithmImplementation(),
    m_lruktest()
{
}

PageEvictionAlgorithmLRUKTest::~PageEvictionAlgorithmLRUKTest()
{
}

ERR PageEvictionAlgorithmLRUKTest::ErrInit( const BFTRACE::BFSysResMgrInit_& bfinit )
{
    if ( m_lruktest.ErrInit( bfinit.K, bfinit.csecCorrelatedTouch ) != CLRUKTestResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    return JET_errSuccess;
}

void PageEvictionAlgorithmLRUKTest::Term( const BFTRACE::BFSysResMgrTerm_& bfterm )
{
    m_lruktest.Term();
}

ERR PageEvictionAlgorithmLRUKTest::ErrCachePage( PAGE* const ppage, const BFTRACE::BFCache_& bfcache )
{

    if ( m_lruktest.ErrCacheResource(
        QwGetCompactIFMPPGNOFromPAGE_( ppage ),
        RESMGRLRUKTESTGetTickCount(),
        bfcache.fUseHistory ) != CLRUKTestResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmLRUKTest::ErrTouchPage( PAGE* const ppage, const BFTRACE::BFTouch_& bftouch )
{
    if ( m_lruktest.ErrTouchResource( QwGetCompactIFMPPGNOFromPAGE_( ppage ), RESMGRLRUKTESTGetTickCount() ) != CLRUKTestResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }
    
    return JET_errSuccess;
}

ERR PageEvictionAlgorithmLRUKTest::ErrSuperColdPage( PAGE* const ppage, const BFTRACE::BFSuperCold_& bfsupercold )
{
    if ( m_lruktest.ErrMarkResourceAsSuperCold( QwGetCompactIFMPPGNOFromPAGE_( ppage ) ) != CLRUKTestResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }
    
    return JET_errSuccess;
}

ERR PageEvictionAlgorithmLRUKTest::ErrEvictSpecificPage( PAGE* const ppage, const BFTRACE::BFEvict_& bfevict )
{
    if ( m_lruktest.ErrEvictResource( QwGetCompactIFMPPGNOFromPAGE_( ppage ) ) != CLRUKTestResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmLRUKTest::ErrEvictNextPage( void* const pv, const BFTRACE::BFEvict_& bfevict, PageEvictionContextType* const ppectyp )
{
    *ppectyp = pectypIFMPPGNO;

    QWORD qwCompactIFMPPGNO = 0;
    CLRUKTestResourceUtilityManagerTypeERR err = m_lruktest.ErrEvictNextResource( &qwCompactIFMPPGNO, RESMGRLRUKTESTGetTickCount() );

    //  Force success if there is nothing to evict.

    if ( err == CLRUKTestResourceUtilityManagerType::errNoCurrentResource )
    {
        PAGE** const pppage = (PAGE**)pv;
        *pppage = NULL;
        *ppectyp = pectypPAGE;

        return JET_errSuccess;
    }

    if ( err != CLRUKTestResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    GetIFMPPGNOFromCompactIFMPPGNO_( (IFMPPGNO*)pv, qwCompactIFMPPGNO );

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmLRUKTest::ErrGetNextPageToEvict( void* const pv, PageEvictionContextType* const ppectyp )
{
    *ppectyp = pectypIFMPPGNO;

    QWORD qwCompactIFMPPGNO = 0;
    CLRUKTestResourceUtilityManagerTypeERR err = m_lruktest.ErrGetNextResource( &qwCompactIFMPPGNO, RESMGRLRUKTESTGetTickCount() );

    //  Force success if there is nothing to evict.

    if ( err == CLRUKTestResourceUtilityManagerType::errNoCurrentResource )
    {
        PAGE** const pppage = (PAGE**)pv;
        *pppage = NULL;
        *ppectyp = pectypPAGE;

        return JET_errSuccess;
    }

    if ( err != CLRUKTestResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    GetIFMPPGNOFromCompactIFMPPGNO_( (IFMPPGNO*)pv, qwCompactIFMPPGNO );

    return JET_errSuccess;
}

size_t PageEvictionAlgorithmLRUKTest::CbPageContext() const
{
    return sizeof(PAGE);
}

void PageEvictionAlgorithmLRUKTest::InitPage( PAGE* const ppage )
{
    new ( ppage ) PAGE;
}

void PageEvictionAlgorithmLRUKTest::DestroyPage( PAGE* const ppage )
{
    ppage->~PAGE();
}

QWORD PageEvictionAlgorithmLRUKTest::QwGetCompactIFMPPGNOFromPAGE_( const PAGE* const ppage )
{
    const IFMP ifmp = ppage->ifmppgno.ifmp;
    const PGNO pgno = ppage->ifmppgno.pgno;

    Enforce( ifmp <= ULONG_MAX );
    Enforce( pgno <= ULONG_MAX );

    return ( ( (QWORD)ifmp << ( 8 * sizeof(QWORD) / 2 ) ) | (DWORD)pgno );
}

void PageEvictionAlgorithmLRUKTest::GetIFMPPGNOFromCompactIFMPPGNO_( IFMPPGNO* const pifmppgno, const QWORD qwCompactIFMPPGNO )
{
    pifmppgno->ifmp = (IFMP)( qwCompactIFMPPGNO >> ( 8 * sizeof(QWORD) / 2 ) );
    pifmppgno->pgno = (PGNO)( qwCompactIFMPPGNO & 0xFFFFFFFF );
}

