// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// begin_PubEsent
#pragma once

#ifndef JET_VERSION
//
//  By default the ESE header will configure itself off the WINVER.  However, in
//  Exchange we select the latest and greatest ESE version, inspite of any specific
//  WINVER being defined, because Exchange ships ESE itself.
//
#define JET_VERSION 0xFFFF
#endif

#include "eseex_x.h"
