// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osustd.hxx"


//  terminate sync subsystem

void OSUSyncTerm()
{
}

//  init sync subsystem

ERR ErrOSUSyncInit()
{
    return JET_errSuccess;
}

//  init statics

BYTE* PLS::s_pbPerfCounters = NULL;
ULONG PLS::s_cbPerfCounters = 0;


