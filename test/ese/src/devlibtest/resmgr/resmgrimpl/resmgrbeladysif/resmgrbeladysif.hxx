// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RESMGRBELADYSIF_HXX_INCLUDED
#define _RESMGRBELADYSIF_HXX_INCLUDED

#include <tchar.h>
#include "os.hxx"
#include "bfreqs.hxx"
#include "_bfconst.hxx"
#include "bfftl.hxx"
#include "bfftldriver.hxx"
#include "collection.hxx"
#include "rmemulator.hxx"
#include "resmgrlrutestif.hxx"


#define RESMGRBELADYSGetTickCount   PageEvictionEmulator::DwGetTickCount


#include "resmgrbeladys.hxx"


class PageEvictionAlgorithmBeladys : public IPageEvictionAlgorithmImplementation
{
public:

    PageEvictionAlgorithmBeladys( const bool fBest = true );
    ~PageEvictionAlgorithmBeladys();


    ERR ErrInit( const BFTRACE::BFSysResMgrInit_& bfinit );
    void Term( const BFTRACE::BFSysResMgrTerm_& bfterm );


    ERR ErrCachePage( PAGE* const ppage, const BFTRACE::BFCache_& bfcache );
    ERR ErrTouchPage( PAGE* const ppage, const BFTRACE::BFTouch_& bftouch );
    ERR ErrSuperColdPage( PAGE* const ppage, const BFTRACE::BFSuperCold_& bfsupercold );
    ERR ErrEvictSpecificPage( PAGE* const ppage, const BFTRACE::BFEvict_& bfevict );
    ERR ErrEvictNextPage( void* const pv, const BFTRACE::BFEvict_& bfevict, PageEvictionContextType* const ppectyp );
    ERR ErrGetNextPageToEvict( void* const pv, PageEvictionContextType* const ppectyp );


    bool FNeedsPreProcessing() const;
    ERR ErrStartProcessing();
    ERR ErrResetProcessing();


    size_t CbPageContext() const;
    void InitPage( PAGE* const ppage );
    void DestroyPage( PAGE* const ppage );

private:

    typedef CBeladysResourceUtilityManager<QWORD, TICK> CBeladysResourceUtilityManagerType;
    typedef CBeladysResourceUtilityManager<QWORD, TICK>::ERR CBeladysResourceUtilityManagerTypeERR;


    PageEvictionAlgorithmLRUTest m_lrutest;
    CBeladysResourceUtilityManagerType m_beladys;
    bool m_fBest;


    static QWORD QwGetCompactIFMPPGNOFromPAGE_( const PAGE* const ppage );
    static void GetIFMPPGNOFromCompactIFMPPGNO_( IFMPPGNO* const pifmppgno, const QWORD qwCompactIFMPPGNO );
};

#endif

