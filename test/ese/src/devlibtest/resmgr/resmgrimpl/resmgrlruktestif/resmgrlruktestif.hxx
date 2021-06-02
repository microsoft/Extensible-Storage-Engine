// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RESMGRLRUKTESTIF_HXX_INCLUDED
#define _RESMGRLRUKTESTIF_HXX_INCLUDED

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

#define RESMGRLRUKTESTGetTickCount  PageEvictionEmulator::DwGetTickCount

//  Now, include the resource manager.

#include "resmgrlruktest.hxx"

//  IPageEvictionAlgorithmImplementation implementation.

//  ================================================================
class PageEvictionAlgorithmLRUKTest : public IPageEvictionAlgorithmImplementation
//  ================================================================
{
public:
    //  Ctor./Dtor.

    PageEvictionAlgorithmLRUKTest( const bool fBest = true );
    ~PageEvictionAlgorithmLRUKTest();

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

    typedef CLRUKTestResourceUtilityManager<QWORD, TICK> CLRUKTestResourceUtilityManagerType;
    typedef CLRUKTestResourceUtilityManager<QWORD, TICK>::ERR CLRUKTestResourceUtilityManagerTypeERR;

    //  Private members.

    CLRUKTestResourceUtilityManagerType m_lruktest; //  //  Resource manager object.

    //  Helpers.

    static QWORD QwGetCompactIFMPPGNOFromPAGE_( const PAGE* const ppage );
    static void GetIFMPPGNOFromCompactIFMPPGNO_( IFMPPGNO* const pifmppgno, const QWORD qwCompactIFMPPGNO );
};

#endif  //  _RESMGRLRUKTESTIF_HXX_INCLUDED

