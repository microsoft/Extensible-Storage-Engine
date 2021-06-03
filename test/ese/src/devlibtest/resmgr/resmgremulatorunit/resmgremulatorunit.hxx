// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// needed for JET errors
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


//  Include Native Unit Test Layer (BSTF)
//

#include "testerr.h"
#include "bstf.hxx"


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
#include "rmemulator.hxx"
#include "resmgrbeladys.hxx"
#include "resmgrlrutestif.hxx"
#include "resmgrlruktestif.hxx"
#include "resmgrlrukeseif.hxx"
#include "resmgrbeladysif.hxx"

