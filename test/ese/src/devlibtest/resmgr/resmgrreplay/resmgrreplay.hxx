// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RESMGRREPLAY_HXX_INCLUDED
#define _RESMGRREPLAY_HXX_INCLUDED


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



#include <tchar.h>
#include "os.hxx"


#include "bfreqs.hxx"
#include "_bfconst.hxx"
#include "bfftl.hxx"
#include "bfetldriver.hxx"
#include "bfftldriver.hxx"


#include "rmemulator.hxx"
#include "resmgrlrutestif\resmgrlrutestif.hxx"
#include "resmgrlruktestif\resmgrlruktestif.hxx"
#include "resmgrlrukeseif\resmgrlrukeseif.hxx"
#include "resmgrbeladysif\resmgrbeladysif.hxx"


typedef enum _ResMgrReplayAlgorithm
{
    rmralgInvalid,
    rmralgLruTest,
    rmralgLrukTest,
    rmralgLrukEse,
    rmralgBeladys,
    rmralgBeladysInverse
} ResMgrReplayAlgorithm;



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



const USHORT g_cCachedHistoResDefault           = 10;
const TICK g_dtickLifetimeHistoResDefault       = 10 * 1000;



ERR ErrResMgrAccumFtlStats( __in BFFTLContext * const pbfftlc, __in const BOOL fDump );
ERR ErrResMgrAccumEtlStats( __in BFETLContext * const pbfetlc );

#endif

