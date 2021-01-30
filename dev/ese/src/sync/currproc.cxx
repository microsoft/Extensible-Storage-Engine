// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma prefast(push)
#pragma prefast(disable:26006, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:26007, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28718, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28726, "Dont bother us with tchar, someone else owns that.")
#include <tchar.h>
#pragma prefast(pop)

#include "sync.hxx"

#include <windows.h>

namespace OSSYNC {

INT IprocOSSyncIGetCurrentProcessor()
{
    PROCESSOR_NUMBER Proc;
    GetThreadIdealProcessorEx( GetCurrentThread(), &Proc );
    return Proc.Number;
}

};
