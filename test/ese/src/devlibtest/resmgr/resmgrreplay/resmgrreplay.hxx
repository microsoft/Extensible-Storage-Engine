// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RESMGRREPLAY_HXX_INCLUDED
#define _RESMGRREPLAY_HXX_INCLUDED

//  needed for JET errors

#ifdef BUILD_ENV_IS_NT
#include <esent_x.h>
#endif
#ifdef BUILD_ENV_IS_EX
#include <jet.h>
#endif

#include <windows.h>
#include <cstdio>
#include <stdlib.h>
#include <stddef.h>
#include <set>


//  Include OS Layer
//
// Note: Not required for resmgr emulation, but because the FTL trace log reader will need it.

#include <tchar.h>
#include "os.hxx"


//  Get the BF FTL tracing driver and requirements
//
#include "bfreqs.hxx"
#include "_bfconst.hxx"
#include "bfftl.hxx"
#include "bfetldriver.hxx"
#include "bfftldriver.hxx"


//  ResMgr Emulator required headers
//
#include "rmemulator.hxx"
#include "resmgrlrutestif\resmgrlrutestif.hxx"
#include "resmgrlruktestif\resmgrlruktestif.hxx"
#include "resmgrlrukeseif\resmgrlrukeseif.hxx"
#include "resmgrbeladysif\resmgrbeladysif.hxx"

//  Supported algorithms
//

typedef enum _ResMgrReplayAlgorithm
{
    rmralgInvalid,
    rmralgLruTest,
    rmralgLrukTest,
    rmralgLrukEse,
    rmralgBeladys,
    rmralgBeladysInverse
} ResMgrReplayAlgorithm;


//  Supported emulation modes
//

typedef enum _ResMgrReplayEmulationMode
{
    rmemInvalid,
    rmemNormal,
    rmemCacheSizeFixed,
    rmemCacheSizeAgeBased,
    rmemCacheSizeFixedIteration,
    rmemCacheSizeIteration,
    rmemCacheSizeIterationAvoidable,
    rmemCacheFaultIteration,
    rmemCacheFaultIterationAvoidable,
    rmemCorrelatedTouchIteration,
    rmemCorrelatedTouchIterationAvoidable,
    rmemChkptDepthIteration,
} ResMgrReplayEmulationMode;


//  Constants and defaults.
//

const USHORT g_cCachedHistoResDefault           = 10;
const TICK g_dtickLifetimeHistoResDefault       = 10 * 1000;


//  Prototypes.
//

ERR ErrResMgrAccumFtlStats( _In_ BFFTLContext * const pbfftlc, _In_ const BOOL fDump );
ERR ErrResMgrAccumEtlStats( _In_ BFETLContext * const pbfetlc );

#endif  //  _RESMGRREPLAY_HXX_INCLUDED

