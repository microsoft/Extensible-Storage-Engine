// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _BFFTL_HXX_INCLUDED
#define _BFFTL_HXX_INCLUDED


#ifdef MINIMAL_FUNCTIONALITY
#define BFFTLOpt( x ) JET_errSuccess
#else
#define BFFTLOpt( x ) x
#endif

enum BFTraceID
{
    bftidInvalid        = 0,

    bftidTouch          = 1,


    bftidFTLReserved    = 15,

    bftidCache          = 21,
    bftidTouchLong      = 22,
    bftidVerifyInfo     = 23,
    bftidEvict          = 24,
    bftidSuperCold      = 25,
    bftidDirty          = 26,
    bftidWrite          = 27,
    bftidSetLgposModify = 28,


    bftidSysResMgrInit  = 128,
    bftidSysResMgrTerm  = 129,
    bftidIfmpAtt        = 130,
    bftidIfmpPurge      = 131,

    bftidMax
};



typedef struct BFTRACE_
{
public:
    USHORT      traceid;
    TICK        tick;

    struct BFSysResMgrInit_
    {
        INT     K;
        double  csecCorrelatedTouch;
        double  csecTimeout;
        double  csecUncertainty;
        double  dblHashLoadFactor;
        double  dblHashUniformity;
        double  dblSpeedSizeTradeoff;
    };
    struct BFSysResMgrTerm_
    {
        QWORD   ftTerm;
    };
    struct BFTouch_
    {
        BYTE    ifmp;
        PGNO    pgno;
        BYTE    bflt;
        BYTE    clientType;
        ULONG   pctPri;
        union
        {
            USHORT      flags;
            struct
            {
                USHORT  fUseHistory:1;
                USHORT  fNewPage:1;
                USHORT  fNoTouch:1;
                USHORT  fDBScan:1;
            };
        };
    };
    struct BFSuperCold_
    {
        BYTE    ifmp;
        PGNO    pgno;
    };
    struct BFCache_
    {
        BYTE    ifmp;
        PGNO    pgno;
        BYTE    bflt;
        BYTE    clientType;
        ULONG   pctPri;
        union
        {
            USHORT      flags;
            struct
            {
                USHORT  fUseHistory:1;
                USHORT  fNewPage:1;
                USHORT  fDBScan:1;
            };
        };
    };
    struct BFVerify_
    {
        BYTE    ifmp;
        PGNO    pgno;
    };
    struct BFEvict_
    {
        BYTE    ifmp;
        PGNO    pgno;
        BOOL    fCurrentVersion;
        ERR     errBF;
        ULONG   bfef;
        ULONG   pctPri;
    };
    struct BFDirty_
    {
        BYTE    ifmp;
        PGNO    pgno;
        BYTE    bfdf;
        ULONG   lgenModify;
        USHORT  isecModify;
        USHORT  ibModify;
    };
    struct BFWrite_
    {
        BYTE    ifmp;
        PGNO    pgno;
        BYTE    bfdf;
        BYTE    iorp;
    };
    struct BFSetLgposModify_
    {
        BYTE    ifmp;
        PGNO    pgno;
        ULONG   lgenModify;
        USHORT  isecModify;
        USHORT  ibModify;
    };

    union
    {
        BFSysResMgrInit_    bfinit;
        BFSysResMgrTerm_    bfterm;
        BFTouch_            bftouch;
        BFSuperCold_        bfsupercold;
        BFCache_            bfcache;
        BFVerify_           bfverify;
        BFEvict_            bfevict;
        BFDirty_            bfdirty;
        BFWrite_            bfwrite;
        BFSetLgposModify_   bfsetlgposmodify;
    };
}
BFTRACE;

#ifdef MINIMAL_FUNCTIONALITY
#else
extern CFastTraceLog* g_pbfftl;
#endif
extern IOREASON g_iorBFTraceFile;

INLINE ERR ErrBFIFTLSysResMgrInit(
    const INT       K,
    const double    csecCorrelatedTouch,
    const double    csecTimeout,
    const double    csecUncertainty,
    const double    dblHashLoadFactor,
    const double    dblHashUniformity,
    const double    dblSpeedSizeTradeoff )
{
    BFTRACE::BFSysResMgrInit_ resmgrinit;

    resmgrinit.K = K;
    resmgrinit.csecCorrelatedTouch = csecCorrelatedTouch;
    resmgrinit.csecTimeout = csecTimeout;
    resmgrinit.csecUncertainty = csecUncertainty;
    resmgrinit.dblHashLoadFactor = dblHashLoadFactor;
    resmgrinit.dblHashUniformity = dblHashUniformity;
    resmgrinit.dblSpeedSizeTradeoff = dblSpeedSizeTradeoff;
    return BFFTLOpt( g_pbfftl->ErrFTLTrace( bftidSysResMgrInit, (BYTE*)&resmgrinit, sizeof(resmgrinit) ) );
}

INLINE ERR ErrBFIFTLSysResMgrTerm()
{
    BFTRACE_::BFSysResMgrTerm_  resmgrterm;
    resmgrterm.ftTerm = UtilGetCurrentFileTime();
    return BFFTLOpt( g_pbfftl->ErrFTLTrace( bftidSysResMgrTerm, (BYTE*)&resmgrterm, sizeof(resmgrterm) ) );
}

INLINE ERR ErrBFIFTLTouch(
    const TICK          tickTouch,
    const IFMP          ifmp,
    const PGNO          pgno,
    const BFLatchType   bflt,
    const BYTE          clientType,
    const ULONG_PTR     pctPriority,
    const BOOL          fUseHistory,
    const BOOL          fNewPage,
    const BOOL          fNoTouch,
    const BOOL          fDBScan )
{
    BFTRACE_::BFTouch_ bftouchlong;

    Assert( bflt < 256 );
    Assert( pctPriority < 0x80000000 );

    bftouchlong.ifmp = (BYTE)ifmp;
    bftouchlong.pgno = pgno;
    bftouchlong.clientType = clientType;
    bftouchlong.pctPri = (ULONG)pctPriority;
    bftouchlong.bflt = (BYTE)bflt;
    bftouchlong.fUseHistory = fUseHistory;
    bftouchlong.fNewPage = fNewPage;
    bftouchlong.fNoTouch = fNoTouch;
    bftouchlong.fDBScan = fDBScan;

    return BFFTLOpt( g_pbfftl->ErrFTLTrace( bftidTouchLong, (BYTE*)&bftouchlong, sizeof(bftouchlong), tickTouch ) );
}

INLINE ERR ErrBFIFTLCache(
    const TICK          tickCache,
    const IFMP          ifmp,
    const PGNO          pgno,
    const BFLatchType   bflt,
    const BYTE          clientType,
    const ULONG_PTR     pctPriority,
    const BOOL          fUseHistory,
    const BOOL          fNewPage,
    const BOOL          fDBScan )
{
    BFTRACE_::BFCache_ bfcache;

    Assert( bflt < 256 );
    Assert( pctPriority < 0x80000000 );

    bfcache.ifmp = (BYTE)ifmp;
    bfcache.pgno = pgno;
    bfcache.clientType = clientType;
    bfcache.pctPri = (ULONG)pctPriority;
    bfcache.bflt = (BYTE)bflt;
    bfcache.fUseHistory = fUseHistory;
    bfcache.fNewPage = fNewPage;
    bfcache.fDBScan = fDBScan;

    return BFFTLOpt( g_pbfftl->ErrFTLTrace( bftidCache, (BYTE*)&bfcache, sizeof(bfcache), tickCache ) );
}

INLINE ERR ErrBFIFTLEvict(
    const IFMP  ifmp,
    const PGNO  pgno,
    const BOOL  fCurrentVersion,
    const ERR   errBF,
    const ULONG bfef,
    const ULONG pctPriority
    )
{
    BFTRACE_::BFEvict_ bfevict;

    bfevict.ifmp = (BYTE)ifmp;
    bfevict.pgno = pgno;
    bfevict.fCurrentVersion = fCurrentVersion;
    bfevict.errBF = errBF;
    bfevict.bfef = bfef;
    bfevict.pctPri = pctPriority;
    return BFFTLOpt( g_pbfftl->ErrFTLTrace( bftidEvict, (BYTE*)&bfevict, sizeof(bfevict) ) );
}

INLINE ERR ErrBFIFTLMarkAsSuperCold(
    const IFMP  ifmp,
    const PGNO  pgno
    )
{
    BFTRACE_::BFSuperCold_  bfsupercold;
    bfsupercold.ifmp = (BYTE)ifmp;
    bfsupercold.pgno = pgno;
    return BFFTLOpt( g_pbfftl->ErrFTLTrace( bftidSuperCold, (BYTE*)&bfsupercold, sizeof(bfsupercold) ) );
}

INLINE ERR ErrBFIFTLDirty(
    const IFMP          ifmp,
    const PGNO          pgno,
    const BFDirtyFlags  bfdf,
    const ULONG         lgposModifyLGen,
    const USHORT        lgposModifyISec,
    const USHORT        lgposModifyIb
    )
{
    Assert( bfdf < 256 );
    BFTRACE_::BFDirty_ bfdirty;
    bfdirty.ifmp = (BYTE)ifmp;
    bfdirty.pgno = pgno;
    bfdirty.bfdf = (BYTE)bfdf;
    bfdirty.lgenModify = lgposModifyLGen;
    bfdirty.isecModify = lgposModifyISec;
    bfdirty.ibModify = lgposModifyIb;
    return BFFTLOpt( g_pbfftl->ErrFTLTrace( bftidDirty, (BYTE*)&bfdirty, sizeof(bfdirty) ) );
}

INLINE ERR ErrBFIFTLWrite(
    const IFMP              ifmp,
    const PGNO              pgno,
    const BFDirtyFlags      bfdf,
    const IOREASONPRIMARY   iorp
    )
{
    Assert( bfdf < 256 );
    Assert( iorp < 256 );
    BFTRACE_::BFWrite_ bfwrite;
    bfwrite.ifmp = (BYTE)ifmp;
    bfwrite.pgno = pgno;
    bfwrite.bfdf = (BYTE)bfdf;
    bfwrite.iorp = iorp;
    return BFFTLOpt( g_pbfftl->ErrFTLTrace( bftidWrite, (BYTE*)&bfwrite, sizeof(bfwrite) ) );
}

INLINE ERR ErrBFIFTLSetLgposModify(
    const IFMP          ifmp,
    const PGNO          pgno,
    const ULONG         lgposModifyLGen,
    const USHORT        lgposModifyISec,
    const USHORT        lgposModifyIb
    )
{
    BFTRACE_::BFSetLgposModify_ bfsetlgposmodify;
    bfsetlgposmodify.ifmp = (BYTE)ifmp;
    bfsetlgposmodify.pgno = pgno;
    bfsetlgposmodify.lgenModify = lgposModifyLGen;
    bfsetlgposmodify.isecModify = lgposModifyISec;
    bfsetlgposmodify.ibModify = lgposModifyIb;
    return BFFTLOpt( g_pbfftl->ErrFTLTrace( bftidSetLgposModify, (BYTE*)&bfsetlgposmodify, sizeof(bfsetlgposmodify) ) );
}

#endif

