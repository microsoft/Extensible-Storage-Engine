// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSU_HXX_INCLUDED
#define _OSU_HXX_INCLUDED

#ifndef OS_LAYER_VIOLATIONS
#define OS_LAYER_VIOLATIONS
#endif

#if !defined(MINIMAL_FUNCTIONALITY) && !defined(ARM)
#define ENABLE_LOG_V7_RECOVERY_COMPAT
#endif

#include "os.hxx"

#include "checksum.hxx"

#include "typesu.hxx"

#include "checksumu.hxx"
#include "configu.hxx"
#include "encryptu.hxx"
#include "eventu.hxx"
#include "fileu.hxx"
#include "normu.hxx"
#include "syncu.hxx"
#include "timeu.hxx"



const ERR ErrOSUInit();


void OSUTerm();


#endif



