// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RESMGRLRUTESTIF_HXX_INCLUDED
#define _RESMGRLRUTESTIF_HXX_INCLUDED

//  Header files.
#include <tchar.h>
#include "os.hxx"
#include "bfreqs.hxx"
#include "_bfconst.hxx"
#include "bfftl.hxx"
#include "bfftldriver.hxx"
#include "collection.hxx"
#include "rmemulator.hxx"


//  Redefining macros.

#define RESMGRLRUTESTGetTickCount   PageEvictionEmulator::DwGetTickCount


//  Now, include the resource manager.

#include "resmgrlrutest.hxx"


//  Data structures.

//  ================================================================
struct BFRESMGRLRUTEST : public PAGE
//  ================================================================
{
    static SIZE_T OffsetOfLRUTestIC()       { return OffsetOf( BFRESMGRLRUTEST, lrutestic ); }      //  CLRUTestResourceUtilityManager glue.

    CLRUTestResourceUtilityManager<BFRESMGRLRUTEST, OffsetOfLRUTestIC>::CInvasiveContext lrutestic; //  CLRUTestResourceUtilityManager context.
};


//  IPageEvictionAlgorithmImplementation implementation.

//  ================================================================
class PageEvictionAlgorithmLRUTest : public IPageEvictionAlgorithmImplementation
//  ================================================================
{
public:
    //  Ctor./Dtor.

    PageEvictionAlgorithmLRUTest();
    ~PageEvictionAlgorithmLRUTest();

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
    //  Type definitions.

    typedef CLRUTestResourceUtilityManager<BFRESMGRLRUTEST, BFRESMGRLRUTEST::OffsetOfLRUTestIC> CLRUTestResourceUtilityManagerType;
    typedef CLRUTestResourceUtilityManager<BFRESMGRLRUTEST, BFRESMGRLRUTEST::OffsetOfLRUTestIC>::ERR CLRUTestResourceUtilityManagerERR;
    
    //  Private members.

    CLRUTestResourceUtilityManagerType m_lrutest;   //  Resource manager object.
};

#endif  //  _RESMGRLRUTESTIF_HXX_INCLUDED

