// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef __REFTRACE_H_
#define __REFTRACE_H_

#include "time.hxx"

typedef struct _REF_TRACE_LOG
{
    LONG    Signature;
    LONG    cLogSize;       //  The total number of entries available in the log.
    LONG    NextEntry;      //  The index of the next entry to use.
    LONG    cbExtraInfo;    //  The extra info the client wants associated with each entry
    LONG    cbEntrySize;    //  The byte size of each entry.
    BYTE *  pLogBuffer;     //  Pointer to the start of the circular buffer.

    //
    // The extra header bytes and actual log entries go here.
    //
    // BYTE ExtraHeaderBytes[ExtraBytesInHeader];
    // BYTE Entries[LogSize][EntrySize];
    //
} REF_TRACE_LOG;

//
// Log header signature.
//

#define REF_TRACE_LOG_SIGNATURE   ((DWORD)'GOLT')
#define REF_TRACE_LOG_SIGNATURE_X ((DWORD)'golX')

#define REF_TRACE_LOG_NO_ENTRY      -1

//
// This is the number of stack backtrace values captured in each
// trace log entry.
//

#define REF_TRACE_LOG_STACK_DEPTH   9

//
// This defines the entry written to the trace log.
//

typedef struct _REF_TRACE_LOG_ENTRY
{
    DWORD       ThreadId;
    LONG        NewRefCount;
    VOID *      pContext;
    HRT         hrt;
    VOID *      Stack[REF_TRACE_LOG_STACK_DEPTH];
    BYTE        pExtraInformation[0];
} REF_TRACE_LOG_ENTRY, *PREF_TRACE_LOG_ENTRY;

#endif // __REFTRACE_H_

