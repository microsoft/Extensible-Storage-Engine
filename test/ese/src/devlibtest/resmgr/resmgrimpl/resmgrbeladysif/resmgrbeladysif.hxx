// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RESMGRBELADYSIF_HXX_INCLUDED
#define _RESMGRBELADYSIF_HXX_INCLUDED

//  Header files.
#include <tchar.h>
#include "os.hxx"
#include "bfreqs.hxx"
#include "_bfconst.hxx"
#include "bfftl.hxx"
#include "bfftldriver.hxx"
#include "collection.hxx"
#include "rmemulator.hxx"
#include "resmgrlrutestif.hxx"

//  Redefining macros.

#define RESMGRBELADYSGetTickCount   PageEvictionEmulator::DwGetTickCount

//  Now, include the resource manager.

#include "resmgrbeladys.hxx"

//  IPageEvictionAlgorithmImplementation implementation.

//  ================================================================
class PageEvictionAlgorithmBeladys : public IPageEvictionAlgorithmImplementation
//  ================================================================
{
public:
    //  Ctor./Dtor.

    PageEvictionAlgorithmBeladys( const bool fBest = true );
    ~PageEvictionAlgorithmBeladys();

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

    //  Other funtions.

    bool FNeedsPreProcessing() const;
    ERR ErrStartProcessing();
    ERR ErrResetProcessing();

    //  Helpers.

    size_t CbPageContext() const;
    void InitPage( PAGE* const ppage );
    void DestroyPage( PAGE* const ppage );

private:
    //  Type definitions.

    typedef CBeladysResourceUtilityManager<QWORD, TICK> CBeladysResourceUtilityManagerType;
    typedef CBeladysResourceUtilityManager<QWORD, TICK>::ERR CBeladysResourceUtilityManagerTypeERR;

    //  Private members.

    PageEvictionAlgorithmLRUTest m_lrutest;         //  Dummy LRU-test object, to be used at pre-processing-time.
    CBeladysResourceUtilityManagerType m_beladys;   //  Bélády's algorithm object.
    bool m_fBest;                                   //  Whether Bélády's algorith will evict the next-best.

    //  Helpers.

    static QWORD QwGetCompactIFMPPGNOFromPAGE_( const PAGE* const ppage );
    static void GetIFMPPGNOFromCompactIFMPPGNO_( IFMPPGNO* const pifmppgno, const QWORD qwCompactIFMPPGNO );
};

#endif  //  _RESMGRBELADYSIF_HXX_INCLUDED

