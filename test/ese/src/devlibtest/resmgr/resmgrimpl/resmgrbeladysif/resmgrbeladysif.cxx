// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  Header files.

#include "resmgrbeladysif.hxx"


//  class PageEvictionAlgorithmBeladys.

PageEvictionAlgorithmBeladys::PageEvictionAlgorithmBeladys( const bool fBest ) : 
    IPageEvictionAlgorithmImplementation(),
    m_lrutest(),
    m_beladys(),
    m_fBest( fBest )
{
}

PageEvictionAlgorithmBeladys::~PageEvictionAlgorithmBeladys()
{
}

ERR PageEvictionAlgorithmBeladys::ErrInit( const BFTRACE::BFSysResMgrInit_& bfinit )
{
    if ( m_beladys.ErrInit() != CBeladysResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    if ( !m_beladys.FProcessing() )
    {
        return m_lrutest.ErrInit( bfinit );
    }

    return JET_errSuccess;
}

void PageEvictionAlgorithmBeladys::Term( const BFTRACE::BFSysResMgrTerm_& bfterm )
{
    m_beladys.Term();

    if ( !m_beladys.FProcessing() )
    {
        m_lrutest.Term( bfterm );
    }
}

ERR PageEvictionAlgorithmBeladys::ErrCachePage( PAGE* const ppage, const BFTRACE::BFCache_& bfcache )
{

    if ( m_beladys.ErrCacheResource( QwGetCompactIFMPPGNOFromPAGE_( ppage ), RESMGRBELADYSGetTickCount() ) != CBeladysResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    if ( !m_beladys.FProcessing() )
    {
        return m_lrutest.ErrCachePage( ppage, bfcache );
    }

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmBeladys::ErrTouchPage( PAGE* const ppage, const BFTRACE::BFTouch_& bftouch )
{
    if ( m_beladys.ErrTouchResource( QwGetCompactIFMPPGNOFromPAGE_( ppage ), RESMGRBELADYSGetTickCount() ) != CBeladysResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }
    
    if ( !m_beladys.FProcessing() )
    {
        return m_lrutest.ErrTouchPage( ppage, bftouch );
    }

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmBeladys::ErrSuperColdPage( PAGE* const ppage, const BFTRACE::BFSuperCold_& bfsupercold )
{
    if ( !m_beladys.FProcessing() )
    {
        return m_lrutest.ErrSuperColdPage( ppage, bfsupercold );
    }

    //  No such concept in Bélády's implementation.

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmBeladys::ErrEvictSpecificPage( PAGE* const ppage, const BFTRACE::BFEvict_& bfevict )
{
    if ( m_beladys.ErrEvictResource( QwGetCompactIFMPPGNOFromPAGE_( ppage ) ) != CBeladysResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    if ( !m_beladys.FProcessing() )
    {
        return m_lrutest.ErrEvictSpecificPage( ppage, bfevict );
    }

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmBeladys::ErrEvictNextPage( void* const pv, const BFTRACE::BFEvict_& bfevict, PageEvictionContextType* const ppectyp )
{
    *ppectyp = pectypIFMPPGNO;

    CBeladysResourceUtilityManagerTypeERR err = CBeladysResourceUtilityManagerType::errSuccess;
    QWORD qwCompactIFMPPGNO = 0;

    if ( m_fBest )
    {
        err = m_beladys.ErrEvictBestNextResource( &qwCompactIFMPPGNO );
    }
    else
    {
        err = m_beladys.ErrEvictWorstNextResource( &qwCompactIFMPPGNO );
    }

    //  Force success if there is nothing to evict.

    if ( err == CBeladysResourceUtilityManagerType::errNoCurrentResource )
    {
        Enforce( m_beladys.FProcessing() );
        err = CBeladysResourceUtilityManagerType::errSuccess;
        PAGE** const pppage = (PAGE**)pv;
        *pppage = NULL;
        *ppectyp = pectypPAGE;
    }

    if ( err != CBeladysResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    if ( !m_beladys.FProcessing() )
    {
        return m_lrutest.ErrEvictNextPage( pv, bfevict, ppectyp );
    }
    else if ( *ppectyp == pectypIFMPPGNO )
    {
        GetIFMPPGNOFromCompactIFMPPGNO_( (IFMPPGNO*)pv, qwCompactIFMPPGNO );
    }

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmBeladys::ErrGetNextPageToEvict( void* const pv, PageEvictionContextType* const ppectyp )
{
    *ppectyp = pectypIFMPPGNO;

    CBeladysResourceUtilityManagerTypeERR err = CBeladysResourceUtilityManagerType::errSuccess;
    QWORD qwCompactIFMPPGNO = 0;

    if ( m_fBest )
    {
        err = m_beladys.ErrGetBestNextResource( &qwCompactIFMPPGNO );
    }
    else
    {
        err = m_beladys.ErrGetWorstNextResource( &qwCompactIFMPPGNO );
    }

    //  Force success if there is nothing to evict.

    if ( err == CBeladysResourceUtilityManagerType::errNoCurrentResource )
    {
        Enforce( m_beladys.FProcessing() );
        err = CBeladysResourceUtilityManagerType::errSuccess;
        PAGE** const pppage = (PAGE**)pv;
        *pppage = NULL;
        *ppectyp = pectypPAGE;
    }

    if ( err != CBeladysResourceUtilityManagerType::errSuccess )
    {
        return ErrERRCheck( JET_errTestError );
    }

    if ( *ppectyp == pectypIFMPPGNO )
    {
        GetIFMPPGNOFromCompactIFMPPGNO_( (IFMPPGNO*)pv, qwCompactIFMPPGNO );
    }

    return JET_errSuccess;
}

bool PageEvictionAlgorithmBeladys::FNeedsPreProcessing() const
{
    return true;
}

ERR PageEvictionAlgorithmBeladys::ErrStartProcessing()
{
    if ( !m_beladys.FProcessing() )
    {
        return m_beladys.ErrStartProcessing();
    }

    return JET_errSuccess;
}

ERR PageEvictionAlgorithmBeladys::ErrResetProcessing()
{
    if ( m_beladys.FProcessing() )
    {
        return m_beladys.ErrResetProcessing();
    }

    return JET_errSuccess;
}

size_t PageEvictionAlgorithmBeladys::CbPageContext() const
{
    if ( !m_beladys.FProcessing() )
    {
        return m_lrutest.CbPageContext();
    }

    return sizeof(PAGE);
}

void PageEvictionAlgorithmBeladys::InitPage( PAGE* const ppage )
{
    if ( !m_beladys.FProcessing() )
    {
        m_lrutest.InitPage( ppage );
    }
    else
    {
        new ( ppage ) PAGE;
    }
}

void PageEvictionAlgorithmBeladys::DestroyPage( PAGE* const ppage )
{
    if ( !m_beladys.FProcessing() )
    {
        m_lrutest.DestroyPage( ppage );
    }
    else
    {
        ppage->~PAGE();
    }
}

QWORD PageEvictionAlgorithmBeladys::QwGetCompactIFMPPGNOFromPAGE_( const PAGE* const ppage )
{
    const IFMP ifmp = ppage->ifmppgno.ifmp;
    const PGNO pgno = ppage->ifmppgno.pgno;

    Enforce( ifmp <= ULONG_MAX );
    Enforce( pgno <= ULONG_MAX );

    return ( ( (QWORD)ifmp << ( 8 * sizeof(QWORD) / 2 ) ) | (DWORD)pgno );
}

void PageEvictionAlgorithmBeladys::GetIFMPPGNOFromCompactIFMPPGNO_( IFMPPGNO* const pifmppgno, const QWORD qwCompactIFMPPGNO )
{
    pifmppgno->ifmp = (IFMP)( qwCompactIFMPPGNO >> ( 8 * sizeof(QWORD) / 2 ) );
    pifmppgno->pgno = (PGNO)( qwCompactIFMPPGNO & 0xFFFFFFFF );
}

