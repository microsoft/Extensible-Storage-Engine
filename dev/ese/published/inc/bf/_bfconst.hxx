// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _BFCONST_HXX_INCLUDED
#define _BFCONST_HXX_INCLUDED




const LONG      Kmax                    = 2;


const LONG      cDepChainLengthMax      = 16;

const LONG_PTR  cbfCacheMinMin          = 2;

const LONG      pctOverReserve          = 200;

const LONG      pctCommitDefenseRAM     = 101;

const LONG      pctCommitDefenseTarget  = 120;

const __int64   cbCommitDefenseMin      = 50 * 1024 * 1024;

const TICK      dtickCommitDefenseTarget = 30 * 60 * 1000;


#ifdef DEBUG
const LONG      cCacheChunkMax          = 4096 * pctOverReserve / 100;
#else
const LONG      cCacheChunkMax          = 128 * pctOverReserve / 100;
#endif
const double    fracVAAvailMin          = 0.10;


#ifdef _WIN64
const double    fracVAMax               = 1.00;
#else
const double    fracVAMax               = 0.50;
#endif


const TICK      dtickFastRetry                      = 2;

const TICK      dtickMaintImmediateFuzzy            = 45;

const TICK      dtickMaintAvailPoolRequestUrgent    = 0;
const TICK      dtickMaintAvailPoolRequest          = 10;
const TICK      dtickMaintAvailPoolOomRetry         = 16;

const TICK      dtickMaintScavengeShrinkMax         = 30 * 1000;
C_ASSERT( dtickMaintScavengeShrinkMax <= ( dtickCommitDefenseTarget / 2 ) );
const ULONG     ulMaintScavengeSevMin               = 0;
const ULONG     ulMaintScavengeSevMax               = 1000;
C_ASSERT( ( ulMaintScavengeSevMax - 1 ) > ( ulMaintScavengeSevMin + 1 ) );
const TICK      dtickMaintScavengeTimeSeqDelta      = 500;

const TICK      dtickMaintCheckpointDepthDelay      = 1000;
const TICK      dtickMaintCheckpointDepthRetry      = 10;

const TICK      dtickMaintCheckpointDelay           = 1000;
const TICK      dtickMaintCheckpointFuzz            = 1000;

const TICK      dtickMaintHashedLatchesPeriod       = 1000;
const double    pctProcAffined                      = 0.90;
const TICK      dtickPromotionDenied                = 60000;
const TICK      dtickLosingNominee                  = 10000;

const size_t    cMaintCacheSamplesAvg               = 3;
const TICK      dtickMaintCacheStatsPeriod          = 1000;
const TICK      dtickMaintCacheStatsSlop            = dtickMaintCacheStatsPeriod / 2;
const TICK      dtickMaintCacheStatsTooOld          = 2 * dtickMaintCacheStatsPeriod;
C_ASSERT( dtickMaintCacheStatsTooOld >= dtickMaintCacheStatsPeriod + dtickMaintCacheStatsSlop );
const TICK      dtickMaintCacheStatsQuiesce         = dtickMaintCacheStatsPeriod;
#define         dtickIdleCacheStatsPeriod           ( (TICK)UlConfigOverrideInjection( 51192, 10 * 60 * 1000 ) )
#define         dtickIdleCacheStatsSlop             ( (TICK)UlConfigOverrideInjection( 49212, 1 * 60 * 1000 ) )
const ULONG_PTR dcbIdleCacheLastWithdrawalThreshold = 512 * 1024;
const LONG_PTR  cbfIdleCacheUncleanThreshold        = 20;
const double    fracMaintCacheSensitivity           = 0.005;

const TICK      dtickMaintCacheSizeRequest          = 0;
#define         dtickMaintCacheSizeRetry            ( (TICK)UlConfigOverrideInjection( 51191, 10 ) )
const LONG_PTR  cbfMainCacheSizeStartedLastInactive = -1;

const TICK      dtickMaintIdleDatabaseDelay         = 10000;
const TICK      dtickMaintIdleDatabaseClearLLR      = 15 * 60 * 1000;
C_ASSERT( ( dtickMaintIdleDatabaseClearLLR + 1 * 60 * 1000 ) <= dtickCommitDefenseTarget );

const TICK      dtickMaintCacheResidencyPeriod      = 10 * 1000;
const TICK      dtickMaintCacheResidencyQuiesce     = min( 30 * 1000, 10 * dtickMaintCacheResidencyPeriod );
const TICK      dtickMaintCacheResidencyTooOld      = 2 * dtickMaintCacheResidencyPeriod;

const TICK      dtickMaintRefreshViewMappedThreshold = 500;

#define         dtickTelemetryPeriod                ( (TICK)UlConfigOverrideInjection( 62210, OnDebugOrRetail( 10 * 1000, 1 * 60 * 60 * 1000 ) ) )



enum ICBPage {
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

template<>
inline ICBPage operator++< ICBPage >( ICBPage& t, const INT not_used )
{
    ICBPage tOld = t;
    Assert( t >= icbPageSmallest && t <= icbPageBiggest );
    t = (ICBPage)(t + 1);
    Assert( t >= icbPageSmallest && t <= icbPageBiggest );
    return tOld;
}

enum BFLatchType
{
    bfltMin         = 0,
    bfltShared      = 0,
    bfltExclusive   = 1,
    bfltWrite       = 2,
    bfltMax         = 3,
    bfltNone        = bfltMax,
};

enum BFLatchState
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

enum BFAllocType
{
    bfatNone            = 0,
    bfatFracCommit      = 1,
    bfatViewMapped      = 2,
    bfatPageAlloc       = 3,
};

enum BFResidenceState
{
    bfrsMin             = 0,
    bfrsNotCommitted    = 0,
    bfrsNewlyCommitted  = 1,
    bfrsNotResident     = 2,
    bfrsResident        = 3,
    bfrsMax             = 4,
};

enum BFOB0MaintOperations
{
    bfob0moFlushing     = 0x00000001,
    bfob0moVersioning   = 0x00000002,
    bfob0moCleaning     = 0x00000004,
    bfob0moQuiescing    = 0x00000008,
};

enum BFEvictFlags
{
    bfefMin                 = 0,
    bfefNone                = 0,

    bfefReasonMin           = 0x1,
    bfefReasonAvailPool     = 0x1,
    bfefReasonShrink        = 0x2,
    bfefReasonPurgeContext  = 0x3,
    bfefReasonPatch         = 0x4,
    bfefReasonTest          = 0x5,
    bfefReasonPurgePage     = 0x6,
    bfefReasonMax           = 0x7,
    bfefReasonMask          = ( bfefReasonAvailPool | bfefReasonShrink | bfefReasonPurgeContext | bfefReasonPurgePage | bfefReasonPatch | bfefReasonTest ),

    bfefEvictDirty      = 0x100,
#ifdef DEBUG
    bfefNukePageImage   = 0x400,
#endif
    bfefKeepHistory         = 0x800,
    bfefQuiesce             = 0x1000,
    bfefAllowTearDownClean  = 0x2000,

    bfefTraceUntouched      = 0x01000000,
    bfefTraceFlushComplete  = 0x02000000,
    bfefTraceSuperColded    = 0x04000000,
    bfefTraceNonResident    = 0x08000000,
    bfefTraceK2             = 0x10000000,

    bfefTraceMask           = ( bfefTraceUntouched | bfefTraceFlushComplete | bfefTraceSuperColded | bfefTraceNonResident | bfefTraceK2 ),
};
DEFINE_ENUM_FLAG_OPERATORS_BASIC( BFEvictFlags );

enum BFRequestTraceFlags
{
    bfrtfNone               = 0x00000000,

    bfrtfNoTouch            = 0x00000001,
    bfrtfNewPage            = 0x00000002,
    bfrtfUseHistory         = 0x00000004,
    bfrtfFileCacheEnabled   = 0x00000008,
    bfrtfDBScan             = 0x00000010,
};

enum BFCleanFlags
{
    bfcfNone      = 0x0,
    bfcfAllowTearDownClean  = 0x1,
    bfcfAllowReorganization = 0x2,
};

enum BFFreePageFlags
{
    bffpfMin            = 0,
    bffpfNone           = bffpfMin,
    bffpfQuiesce        = 0x1,
};

enum BFTraceCacheMissReason : BYTE
{
    bftcmrInvalid               = 0x0,

    bftcmrReasonPrereadTooSlow  = 0x1,
    bftcmrReasonSyncRead        = 0x2,
    bftcmrReasonPagingFaultDb   = 0x3,
    bftcmrReasonPagingFaultOs   = 0x4,
    bftcmrReasonMapViewRead     = 0x5,
    bftcmrReasonMapViewRefresh  = 0x6,
    bftcmrReasonMask            = ( bftcmrReasonPrereadTooSlow | bftcmrReasonSyncRead | bftcmrReasonPagingFaultDb | bftcmrReasonPagingFaultOs | bftcmrReasonMapViewRead | bftcmrReasonMapViewRefresh )
};

#endif

