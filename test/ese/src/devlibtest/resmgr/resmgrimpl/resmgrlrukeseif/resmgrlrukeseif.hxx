// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RESMGRLRUKESEIF_HXX_INCLUDED
#define _RESMGRLRUKESEIF_HXX_INCLUDED

#ifndef max
#define max(n1,n2) (n1 > n2 ? n1 : n2)
#endif
#include <tchar.h>
#include "os.hxx"
#include "bfreqs.hxx"
#include "_bfconst.hxx"
#include "bfftl.hxx"
#include "bfftldriver.hxx"
#include "collection.hxx"
#include "rmemulator.hxx"


const INT rankBFLRUK                    = 70;
const LONG g_pctCachePriorityMin        = 0;
const LONG g_pctCachePriorityMax        = 1000;
const LONG g_pctCachePriorityMaxMax     = (LONG)wMax;
const LONG g_pctCachePriorityDefault    = 100;
#define FIsCachePriorityValid( pctCachePriority )   ( ( (LONG)(pctCachePriority) >= g_pctCachePriorityMin ) && ( (LONG)(pctCachePriority) <= g_pctCachePriorityMax ) )


#define TickRESMGRTimeCurrent       PageEvictionEmulator::DwGetTickCount



#include "resmgr.hxx"



struct BFRESMGRLRUKESE : public PAGE
{
    static SIZE_T OffsetOfLRUKIC()      { return OffsetOf( BFRESMGRLRUKESE, lrukic ); }

    CLRUKResourceUtilityManager<Kmax, BFRESMGRLRUKESE, OffsetOfLRUKIC, IFMPPGNO>::CInvasiveContext lrukic;
};

DECLARE_LRUK_RESOURCE_UTILITY_MANAGER( Kmax, BFRESMGRLRUKESE, BFRESMGRLRUKESE::OffsetOfLRUKIC, IFMPPGNO, BFLRUK );


class PageEvictionAlgorithmLRUKESE : public IPageEvictionAlgorithmImplementation
{
public:

    PageEvictionAlgorithmLRUKESE();
    ~PageEvictionAlgorithmLRUKESE();


    ERR ErrInit( const BFTRACE::BFSysResMgrInit_& bfinit );
    void Term( const BFTRACE::BFSysResMgrTerm_& bfterm );


    ERR ErrCachePage( PAGE* const ppage, const BFTRACE::BFCache_& bfcache );
    ERR ErrTouchPage( PAGE* const ppage, const BFTRACE::BFTouch_& bftouch );
    ERR ErrSuperColdPage( PAGE* const ppage, const BFTRACE::BFSuperCold_& bfsupercold );
    ERR ErrEvictSpecificPage( PAGE* const ppage, const BFTRACE::BFEvict_& bfevict );
    ERR ErrEvictNextPage( void* const pv, const BFTRACE::BFEvict_& bfevict, PageEvictionContextType* const ppectyp );
    ERR ErrGetNextPageToEvict( void* const pv, PageEvictionContextType* const ppectyp );


    size_t CbPageContext() const;
    void InitPage( PAGE* const ppage );
    void DestroyPage( PAGE* const ppage );

private:

    ERR ErrEvictNextPage_( void* const pv, const BFTRACE::BFEvict_& bfevict, PageEvictionContextType* const ppectyp, const bool fEvict );


    BFLRUK m_lruk;
};

#endif

