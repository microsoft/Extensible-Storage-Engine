// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osustd.hxx"



void OSUSyncTerm()
{
}


ERR ErrOSUSyncInit()
{
    return JET_errSuccess;
}


BYTE* PLS::s_pbPerfCounters = NULL;
ULONG PLS::s_cbPerfCounters = 0;


