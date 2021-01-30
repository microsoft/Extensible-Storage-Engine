// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef __REFTRACE_H_
#define __REFTRACE_H_

#include "time.hxx"

typedef struct _REF_TRACE_LOG
{
    LONG    Signature;
    LONG    cLogSize;
    LONG    NextEntry;
    LONG    cbExtraInfo;
    LONG    cbEntrySize;
    BYTE *  pLogBuffer;

} REF_TRACE_LOG;


#define REF_TRACE_LOG_SIGNATURE   ((DWORD)'GOLT')
#define REF_TRACE_LOG_SIGNATURE_X ((DWORD)'golX')

#define REF_TRACE_LOG_NO_ENTRY      -1


#define REF_TRACE_LOG_STACK_DEPTH   9


typedef struct _REF_TRACE_LOG_ENTRY
{
    DWORD       ThreadId;
    LONG        NewRefCount;
    VOID *      pContext;
    HRT         hrt;
    VOID *      Stack[REF_TRACE_LOG_STACK_DEPTH];
    BYTE        pExtraInformation[0];
} REF_TRACE_LOG_ENTRY, *PREF_TRACE_LOG_ENTRY;

#endif

