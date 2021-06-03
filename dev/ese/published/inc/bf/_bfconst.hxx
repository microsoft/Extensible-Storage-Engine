// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _BFCONST_HXX_INCLUDED
#define _BFCONST_HXX_INCLUDED


//  Constants

    //  LRU-K

const LONG      Kmax                    = 2;        //  maximum K for LRUK

    //  Dependencies

const LONG      cDepChainLengthMax      = 16;       //  threshold at which we start breaking
                                                    //    dependency chains
    //  Cache Sizing

const LONG_PTR  cbfCacheMinMin          = 2;        //  due to avail pool sizing requirements, our min cache size is 2

const LONG      pctOverReserve          = 200;      //  over-reserve factor (100% means "do not over-reserve")

const LONG      pctCommitDefenseRAM     = 101;      //  RAM commit-defense factor (100% means "enforce if commit size reaches 100% of the physical RAM")

const LONG      pctCommitDefenseTarget  = 120;      //  target commit-defense factor (100% means "enforce if commit size reaches 100% of the target")

const __int64   cbCommitDefenseMin      = 50 * 1024 * 1024; //  commit target threshold at which we start honoring over-commit defense

const TICK      dtickCommitDefenseTarget = 30 * 60 * 1000;  //  cache resizing duration threshold at which we start honoring target over-commit defense

    //  Cache Segmentation

#ifdef DEBUG
const LONG      cCacheChunkMax          = 4096 * pctOverReserve / 100;      //  maximum number of cache chunks
#else  //  !DEBUG
const LONG      cCacheChunkMax          = 128 * pctOverReserve / 100;       //  maximum number of cache chunks
#endif  //  DEBUG
const double    fracVAAvailMin          = 0.10;                             //  minimum fraction of VA left unused by the cache

//  In 32-bit, limit the cache size to half of VA to avoid VA exhaustion due to memory fragmentation caused by shrink

#ifdef _WIN64
const double    fracVAMax               = 1.00; //  maximum fraction of VA that can be used by the cache
#else
const double    fracVAMax               = 0.50; //  maximum fraction of VA that can be used by the cache
#endif

    //  Maintenance

const TICK      dtickFastRetry                      = 2;

const TICK      dtickMaintImmediateFuzzy            = 45;

const TICK      dtickMaintAvailPoolRequestUrgent    = 0;
const TICK      dtickMaintAvailPoolRequest          = 10;
const TICK      dtickMaintAvailPoolOomRetry         = 16;

const TICK      dtickMaintScavengeShrinkMax         = 30 * 1000;    //  target shrink duration above which shrink severity starts to ramp up more aggressively
C_ASSERT( dtickMaintScavengeShrinkMax <= ( dtickCommitDefenseTarget / 2 ) );
const ULONG     ulMaintScavengeSevMin               = 0;
const ULONG     ulMaintScavengeSevMax               = 1000;
C_ASSERT( ( ulMaintScavengeSevMax - 1 ) > ( ulMaintScavengeSevMin + 1 ) );
const TICK      dtickMaintScavengeTimeSeqDelta      = 500;      //  time delta between tracked scavenging runs

const TICK      dtickMaintCheckpointDepthDelay      = 1000;
const TICK      dtickMaintCheckpointDepthRetry      = 10;

const TICK      dtickMaintCheckpointDelay           = 1000;     //  this should be significant but small enough that we can go truly idle quickly
const TICK      dtickMaintCheckpointFuzz            = 1000;     //  fuzzy extra delay to allow our task to be scheduled / coalesced with other OS tasks

const TICK      dtickMaintHashedLatchesPeriod       = 1000;
const double    pctProcAffined                      = 0.90;     //  % latch count per proc where proc affinity is assumed
const TICK      dtickPromotionDenied                = 60000;    //  eligibility delay for a BF that has been denied promotion
const TICK      dtickLosingNominee                  = 10000;    //  eligibility delay for a BF that has lost an election

const size_t    cMaintCacheSamplesAvg               = 3;
const TICK      dtickMaintCacheStatsPeriod          = 1000;
const TICK      dtickMaintCacheStatsSlop            = dtickMaintCacheStatsPeriod / 2;
const TICK      dtickMaintCacheStatsTooOld          = 2 * dtickMaintCacheStatsPeriod;
C_ASSERT( dtickMaintCacheStatsTooOld >= dtickMaintCacheStatsPeriod + dtickMaintCacheStatsSlop );
const TICK      dtickMaintCacheStatsQuiesce         = dtickMaintCacheStatsPeriod;       // quiesce Cache Stats task 2nd run after last cache activity
#define         dtickIdleCacheStatsPeriod           ( (TICK)UlConfigOverrideInjection( 51192, 10 * 60 * 1000 ) )    // 10 minutes
#define         dtickIdleCacheStatsSlop             ( (TICK)UlConfigOverrideInjection( 49212, 1 * 60 * 1000 ) )     // 1 minute
const ULONG_PTR dcbIdleCacheLastWithdrawalThreshold = 512 * 1024;   // Shrink cache all the way down to min when idle and this threshold is hit (above min).
const LONG_PTR  cbfIdleCacheUncleanThreshold        = 20;           // Idle-cache shrinkage will be limited to this amount of unclean pages within the shrink range.
const double    fracMaintCacheSensitivity           = 0.005;

const TICK      dtickMaintCacheSizeRequest          = 0;
#define         dtickMaintCacheSizeRetry            ( (TICK)UlConfigOverrideInjection( 51191, 10 ) )
const LONG_PTR  cbfMainCacheSizeStartedLastInactive = -1;

const TICK      dtickMaintIdleDatabaseDelay         = 10000;    //  max rate at which we check for idle databases
const TICK      dtickMaintIdleDatabaseClearLLR      = 15 * 60 * 1000;   //  15 minute delay before we clear out the whole LLR range
C_ASSERT( ( dtickMaintIdleDatabaseClearLLR + 1 * 60 * 1000 ) <= dtickCommitDefenseTarget );

const TICK      dtickMaintCacheResidencyPeriod      = 10 * 1000;                                //  rate at which we update the BF residency map
const TICK      dtickMaintCacheResidencyQuiesce     = min( 30 * 1000, 10 * dtickMaintCacheResidencyPeriod );    //  we should quiesce the residency map update if this amount of time elapsed since the last interested consumer
const TICK      dtickMaintCacheResidencyTooOld      = 2 * dtickMaintCacheResidencyPeriod;   //  if we disable the scheduled task, we should re-enable it if this amount of time elapses and there is an interested consumer

const TICK      dtickMaintRefreshViewMappedThreshold = 500;     // time between forcible refreshes of the entire page

#define         dtickTelemetryPeriod                ( (TICK)UlConfigOverrideInjection( 62210, OnDebugOrRetail( 10 * 1000, 1 * 60 * 60 * 1000 ) ) )    // 1 hour

//  Internal Types

    //  Page size constant ...

enum ICBPage {  //  icb
    icbPageInvalid  = 0,
    icbPageSmallest = 1,
    icbPage0        = icbPageSmallest,
    icbPage64       = 2,
    icbPage128      = 3,
    icbPage256      = 4,
    icbPage512      = 5,
    icbPage1KB      = 6,
    icbPage2KB      = 7,
    icbPage4KB      = 8,
    icbPage8KB      = 9,
    icbPage12KB     = 10,
    icbPage16KB     = 11,
    icbPage20KB     = 12,
    icbPage24KB     = 13,
    icbPage28KB     = 14,
    icbPage32KB     = 15,
    icbPageBiggest  = icbPage32KB,
    icbPageMax
};

//  Some crazy-shtuff that LB showed me to make the ++ operator work on an ICBPage enum ...
template<>
inline ICBPage operator++< ICBPage >( ICBPage& t, const INT not_used )
{
    ICBPage tOld = t;
    Assert( t >= icbPageSmallest && t <= icbPageBiggest );
    t = (ICBPage)(t + 1);
    Assert( t >= icbPageSmallest && t <= icbPageBiggest );
    return tOld;
}

enum BFLatchType  //  bflt
{
    bfltMin         = 0,
    //  must match sync constants ... declared in C_ASSERT()s above ErrBFILatchPage()
    bfltShared      = 0,
    bfltExclusive   = 1,
    bfltWrite       = 2,
    bfltMax         = 3,
    bfltNone        = bfltMax,
};

enum BFLatchState  //  bfls
{
    bflsMin         = 0,
    bflsNormal      = 0,
    bflsNominee1    = 1,
    bflsNominee2    = 2,
    bflsNominee3    = 3,
    bflsElect       = 4,
    bflsHashed      = 5,
    bflsMax         = 6,
};

enum BFAllocType    //  bfat
{
    bfatNone            = 0,
    bfatFracCommit      = 1,        //  Fractional commit is classic ESE memory management where we reserve large chunks, and then commit to the cache size we want (or part in case of Hyper-Cache)
    bfatViewMapped      = 2,        //  Mapped from the file.
    bfatPageAlloc       = 3,        //  Diff between FracCommit and PageAlloc is whether it was commited as part of a large chunk of cache, or independently allocated and committed (respectively).
};

enum BFResidenceState  //  bfrs
{
    bfrsMin             = 0,
    bfrsNotCommitted    = 0,
    bfrsNewlyCommitted  = 1,
    bfrsNotResident     = 2,
    bfrsResident        = 3,
    bfrsMax             = 4,
};

enum BFOB0MaintOperations   // bfob0mo
{
    bfob0moFlushing     = 0x00000001,
    bfob0moVersioning   = 0x00000002,
    bfob0moCleaning     = 0x00000004,
    bfob0moQuiescing    = 0x00000008,
};

enum BFEvictFlags   // bfef
{
    bfefMin                 = 0,
    bfefNone                = 0,

    //  eviction reasons (note: must be first)
    bfefReasonMin           = 0x1,
    bfefReasonAvailPool     = 0x1,
    bfefReasonShrink        = 0x2,
    bfefReasonPurgeContext  = 0x3,
    bfefReasonPatch         = 0x4,
    bfefReasonTest          = 0x5,
    bfefReasonPurgePage     = 0x6,
    bfefReasonMax           = 0x7,
    bfefReasonMask          = ( bfefReasonAvailPool | bfefReasonShrink | bfefReasonPurgeContext | bfefReasonPurgePage | bfefReasonPatch | bfefReasonTest ),

    //  eviction options
    bfefEvictDirty      = 0x100,
    //bfefHasHashLock       = 0x200,    // DEPRECATED: may be reused. Numberspace was kept due to trace processing.
#ifdef DEBUG
    bfefNukePageImage   = 0x400,
#endif
    bfefKeepHistory         = 0x800,
    bfefQuiesce             = 0x1000,
    bfefAllowTearDownClean  = 0x2000,

    //  trace only info - about page at time of eviction
    bfefTraceUntouched      = 0x01000000,
    bfefTraceFlushComplete  = 0x02000000,
    bfefTraceSuperColded    = 0x04000000,
    bfefTraceNonResident    = 0x08000000,
    bfefTraceK2             = 0x10000000,  // else it is K1.
    //bfefTraceUnattached     = 0x20000000,

    bfefTraceMask           = ( bfefTraceUntouched | bfefTraceFlushComplete | bfefTraceSuperColded | bfefTraceNonResident | bfefTraceK2 ),
};
DEFINE_ENUM_FLAG_OPERATORS_BASIC( BFEvictFlags );

enum BFRequestTraceFlags    //  bfrtf
{
    bfrtfNone               = 0x00000000,

    bfrtfNoTouch            = 0x00000001,
    bfrtfNewPage            = 0x00000002,
    bfrtfUseHistory         = 0x00000004,
    bfrtfFileCacheEnabled   = 0x00000008,
    bfrtfDBScan             = 0x00000010,
};

enum BFCleanFlags   // bfcf
{
    bfcfNone /* safe */     = 0x0,
    bfcfAllowTearDownClean  = 0x1,
    bfcfAllowReorganization = 0x2,
};

enum BFFreePageFlags    // bffpf
{
    bffpfMin            = 0,
    bffpfNone           = bffpfMin,
    bffpfQuiesce        = 0x1,  //  Page should be quiesced upon freeing.
};

enum BFTraceCacheMissReason : BYTE  // bftcmr
{
    bftcmrInvalid               = 0x0,  //  <you must at least specify a unique reason>

    bftcmrReasonPrereadTooSlow  = 0x1,  //  The page was cached, but read IO not completed, was pre-read not early enough.
    bftcmrReasonSyncRead        = 0x2,  //  Typical sync read, are we missing a pre-read.
    bftcmrReasonPagingFaultDb   = 0x3,  //  A faulted out page, that we paged back in via redirecting the IO to DB file.
    bftcmrReasonPagingFaultOs   = 0x4,  //  Probably a hard (though may be a soft) fault from the actual OS paging file.
    bftcmrReasonMapViewRead     = 0x5,  //  View mapped files do IO on first deref, so this is main deref during validate/checksum path.
    bftcmrReasonMapViewRefresh  = 0x6,  //  View mapped files can be reclaimed, this is for a potential soft fault "re-read".
    bftcmrReasonMask            = ( bftcmrReasonPrereadTooSlow | bftcmrReasonSyncRead | bftcmrReasonPagingFaultDb | bftcmrReasonPagingFaultOs | bftcmrReasonMapViewRead | bftcmrReasonMapViewRefresh )
};

#endif  //  _BFCONST_HXX_INCLUDED

