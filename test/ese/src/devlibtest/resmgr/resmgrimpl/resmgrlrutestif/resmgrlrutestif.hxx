// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RESMGRLRUTESTIF_HXX_INCLUDED
#define _RESMGRLRUTESTIF_HXX_INCLUDED

#include <tchar.h>
#include "os.hxx"
#include "bfreqs.hxx"
#include "_bfconst.hxx"
#include "bfftl.hxx"
#include "bfftldriver.hxx"
#include "collection.hxx"
#include "rmemulator.hxx"



#define RESMGRLRUTESTGetTickCount   PageEvictionEmulator::DwGetTickCount



#include "resmgrlrutest.hxx"



struct BFRESMGRLRUTEST : public PAGE
{
    static SIZE_T OffsetOfLRUTestIC()       { return OffsetOf( BFRESMGRLRUTEST, lrutestic ); }

    CLRUTestResourceUtilityManager<BFRESMGRLRUTEST, OffsetOfLRUTestIC>::CInvasiveContext lrutestic;
};



class PageEvictionAlgorithmLRUTest : public IPageEvictionAlgorithmImplementation
{
public:

    PageEvictionAlgorithmLRUTest();
    ~PageEvictionAlgorithmLRUTest();


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

    typedef CLRUTestResourceUtilityManager<BFRESMGRLRUTEST, BFRESMGRLRUTEST::OffsetOfLRUTestIC> CLRUTestResourceUtilityManagerType;
    typedef CLRUTestResourceUtilityManager<BFRESMGRLRUTEST, BFRESMGRLRUTEST::OffsetOfLRUTestIC>::ERR CLRUTestResourceUtilityManagerERR;
    

    CLRUTestResourceUtilityManagerType m_lrutest;
};

#endif

