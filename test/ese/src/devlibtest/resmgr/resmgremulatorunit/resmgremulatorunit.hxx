// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

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



#include "testerr.h"
#include "bstf.hxx"



#include <tchar.h>
#include "os.hxx"


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

