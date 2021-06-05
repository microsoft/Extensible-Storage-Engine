// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSU_HXX_INCLUDED
#define _OSU_HXX_INCLUDED

// When this is included in osfile.cxx and building oswinnt.lib (i.e. with
// OS Layer violations), this is already defined, so need to protect 
// against that.
#ifndef OS_LAYER_VIOLATIONS
#define OS_LAYER_VIOLATIONS
#endif

// Only build v7 recovery compat for full functionality
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


//  init OSU subsystem

const ERR ErrOSUInit();

//  terminate OSU subsystem

void OSUTerm();


#endif  //  _OSU_HXX_INCLUDED



