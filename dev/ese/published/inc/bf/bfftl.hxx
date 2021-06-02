// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _BFFTL_HXX_INCLUDED
#define _BFFTL_HXX_INCLUDED

// BFFTL Enable/Disable Macro
// This macros will wrap BFFTL operations to empower us 
// to enable/disable BFFTL funcationalities

#ifdef MINIMAL_FUNCTIONALITY
#define BFFTLOpt( x ) JET_errSuccess
#else  //  !MINIMAL_FUNCTIONALITY
#define BFFTLOpt( x ) x
#endif  //  MINIMAL_FUNCTIONALITY

enum BFTraceID  // bftid
{
    bftidInvalid        = 0,    //  also used as a sentinel for the test mode of the driver.

    //                  = eFTLFirstShortTrace /* 1 */,
    bftidTouch          = 1,

    // reserved for other short traces

    //                  = eFTLLastShortTrace /* 14 */
    bftidFTLReserved    = 15,   //  the FTL trace logging system reserves this trace ID from all clients

    bftidCache          = 21,
    bftidTouchLong      = 22,   //  see also bftidTouch
    bftidVerifyInfo     = 23,
    bftidEvict          = 24,
    bftidSuperCold      = 25,
    bftidDirty          = 26,
    bftidWrite          = 27,
    bftidSetLgposModify = 28,

    // reserved for other medium traces

    //                  = eFTLLastMedTrace /* 127 */
    bftidSysResMgrInit  = 128,
    bftidSysResMgrTerm  = 129,
    bftidIfmpAtt        = 130,
    bftidIfmpPurge      = 131,

    bftidMax    //  must be at end
};



typedef struct BFTRACE_
{
public:
    USHORT      traceid;    //  a BFTraceID identifying what event / trace occurred
    TICK        tick;       //  tick (ms) time of the event / trace

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
        QWORD   ftTerm;         //  want absolute time of the term
    };
    struct BFTouch_
    {
        BYTE    ifmp;           //  "dbid" of the touch
        PGNO    pgno;           //  pgno of the touch
        BYTE    bflt;           //  BFLatchType of the touch
        BYTE    clientType;     //  Type of client touching the page
        ULONG   pctPri;         //  Percent Cache Priority used for touch
        union
        {
            USHORT      flags;
            struct
            {
                USHORT  fUseHistory:1;      //  If the history should be used
                USHORT  fNewPage:1;         //  If this is a new page
                USHORT  fNoTouch:1;         //  If the resource manager should capture this touch/request operation
                USHORT  fDBScan:1;          //  If this operation is being originated by DBM
            };
        };
    };
    struct BFSuperCold_
    {
        BYTE    ifmp;       //  "dbid" of the supercold touch
        PGNO    pgno;       //  pgno of the supercold touch
    };
    struct BFCache_
    {
        BYTE    ifmp;           //  "dbid" of the cache page event
        PGNO    pgno;           //  pgno of the cache page event
        BYTE    bflt;           //  BFLatchType of the touch
        BYTE    clientType;     //  Type of client caching the page
        ULONG   pctPri;         //  Percent Cache Priority used for cache
        union
        {
            USHORT      flags;
            struct
            {
                USHORT  fUseHistory:1;  //  If the history should be used
                USHORT  fNewPage:1;     //  If this is a new page
                USHORT  fDBScan:1;      //  If this operation is being originated by DBM
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
        ULONG   pctPri;         //  Percent Cache Priority used for evict (OBSOLETE, always 0)
    };
    struct BFDirty_
    {
        BYTE    ifmp;       //  "dbid" of the page being dirtied
        PGNO    pgno;       //  pgno of the page being dirtied
        BYTE    bfdf;       //  dirty level
        ULONG   lgenModify; //  log generation of the lgposModify
        USHORT  isecModify; //  sector of the lgposModify
        USHORT  ibModify;   //  offset within sector of the lgposModify
    };
    struct BFWrite_
    {
        BYTE    ifmp;       //  "dbid" of the page being written
        PGNO    pgno;       //  pgno of the page being written
        BYTE    bfdf;       //  dirty level
        BYTE    iorp;       //  primary reason for the write operation
    };
    struct BFSetLgposModify_
    {
        BYTE    ifmp;       //  "dbid" of the page being modified
        PGNO    pgno;       //  pgno of the page being modified
        ULONG   lgenModify; //  log generation of the lgposModify
        USHORT  isecModify; //  sector of the lgposModify
        USHORT  ibModify;   //  offset within sector of the lgposModify
    };

    union
    {
        BFSysResMgrInit_    bfinit;             //  bftidSysResMgrInit
        BFSysResMgrTerm_    bfterm;             //  bftidSysResMgrTerm
        BFTouch_            bftouch;            //  bftidTouch
        BFSuperCold_        bfsupercold;        //  bftidSuperCold
        BFCache_            bfcache;            //  bftidCache
        BFVerify_           bfverify;           //  bftidVerify
        BFEvict_            bfevict;            //  bftidEvict
        BFDirty_            bfdirty;            //  bftidDirty
        BFWrite_            bfwrite;            //  bftidWrite
        BFSetLgposModify_   bfsetlgposmodify;   //  bftidSetLgposModify
    };
}
BFTRACE;

//
//  Tracing Support
//
#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY
extern CFastTraceLog* g_pbfftl;
#endif  //  MINIMAL_FUNCTIONALITY
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

    Assert( bflt < 256 );   // stuffed into a byte
    Assert( pctPriority < 0x80000000 ); // stuffed into a ULONG

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

    Assert( bflt < 256 );   // stuffed into a byte
    Assert( pctPriority < 0x80000000 ); // stuffed into a ULONG

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

#endif  //  _BFFTL_HXX_INCLUDED

