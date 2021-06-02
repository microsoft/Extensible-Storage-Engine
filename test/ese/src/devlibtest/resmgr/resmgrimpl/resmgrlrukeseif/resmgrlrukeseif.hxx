// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RESMGRLRUKESEIF_HXX_INCLUDED
#define _RESMGRLRUKESEIF_HXX_INCLUDED

//  Header files.
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

//  Constants.

const INT rankBFLRUK                    = 70;   //  Rank for deadlock detection purposes.
const LONG g_pctCachePriorityMin        = 0;
const LONG g_pctCachePriorityMax        = 1000;
const LONG g_pctCachePriorityMaxMax     = (LONG)wMax;
const LONG g_pctCachePriorityDefault    = 100;
#define FIsCachePriorityValid( pctCachePriority )   ( ( (LONG)(pctCachePriority) >= g_pctCachePriorityMin ) && ( (LONG)(pctCachePriority) <= g_pctCachePriorityMax ) )

//  Redefining macros.

#define TickRESMGRTimeCurrent       PageEvictionEmulator::DwGetTickCount


//  Now, include the resource manager.

#include "resmgr.hxx"


//  Data structures.

//  ================================================================
struct BFRESMGRLRUKESE : public PAGE
//  ================================================================
{
    static SIZE_T OffsetOfLRUKIC()      { return OffsetOf( BFRESMGRLRUKESE, lrukic ); }                     //  CLRUKResourceUtilityManager glue.

    CLRUKResourceUtilityManager<Kmax, BFRESMGRLRUKESE, OffsetOfLRUKIC, IFMPPGNO>::CInvasiveContext lrukic;  //  CLRUKResourceUtilityManager context.
};

//  ================================================================
DECLARE_LRUK_RESOURCE_UTILITY_MANAGER( Kmax, BFRESMGRLRUKESE, BFRESMGRLRUKESE::OffsetOfLRUKIC, IFMPPGNO, BFLRUK );
//  ================================================================

//  IPageEvictionAlgorithmImplementation implementation.

//  ================================================================
class PageEvictionAlgorithmLRUKESE : public IPageEvictionAlgorithmImplementation
//  ================================================================
{
public:
    //  Ctor./Dtor.

    PageEvictionAlgorithmLRUKESE();
    ~PageEvictionAlgorithmLRUKESE();

    //  Init./term.

    ERR ErrInit( const BFTRACE::BFSysResMgrInit_& bfinit );
    void Term( const BFTRACE::BFSysResMgrTerm_& bfterm );

    //  Cache/touch/evict.

    ERR ErrCachePage( PAGE* const ppage, const BFTRACE::BFCache_& bfcache );
    ERR ErrTouchPage( PAGE* const ppage, const BFTRACE::BFTouch_& bftouch );
    ERR ErrSuperColdPage( PAGE* const ppage, const BFTRACE::BFSuperCold_& bfsupercold );
    ERR ErrEvictSpecificPage( PAGE* const ppage, const BFTRACE::BFEvict_& bfevict );
    ERR ErrEvictNextPage( void* const pv, const BFTRACE::BFEvict_& bfevict, PageEvictionContextType* const ppectyp );
    ERR ErrGetNextPageToEvict( void* const pv, PageEvictionContextType* const ppectyp );

    //  Helpers.

    size_t CbPageContext() const;
    void InitPage( PAGE* const ppage );
    void DestroyPage( PAGE* const ppage );

private:
    //  Private helpers.

    ERR ErrEvictNextPage_( void* const pv, const BFTRACE::BFEvict_& bfevict, PageEvictionContextType* const ppectyp, const bool fEvict );

    //  Private members.

    BFLRUK m_lruk;  //  Resource manager object.
};

#endif  //  _RESMGRLRUKESEIF_HXX_INCLUDED

