// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  This file declares the interface for the BF ETL driver, which allows
//  a component to convert a file (or test traces) to an enumerator model
//  for easy trace processing.

#ifndef _BFETLDRIVER_HXX_INCLUDED
#define _BFETLDRIVER_HXX_INCLUDED

#include <tchar.h>
#include "os.hxx"
#include "bfftl.hxx"        // BF trace data and IDs
#include "EtwCollection.hxx"

#include <set>
#include <list>
#include <unordered_map>

typedef struct _BFTraceStats
{
    __int64 cSysResMgrInit;
    __int64 cSysResMgrTerm;
    __int64 cCache;
    __int64 cRequest;
    __int64 cDirty;
    __int64 cWrite;
    __int64 cSetLgposModify;
    __int64 cEvict;
    __int64 cSuperCold;

    void Print( const ULONG cIndentation ) const
    {
        printf( "%*cResource manager init. traces: %I64d\r\n", cIndentation, ' ', cSysResMgrInit );
        printf( "%*cResource manager term. traces: %I64d\r\n", cIndentation, ' ', cSysResMgrTerm );
        printf( "%*cCache-page traces: %I64d\r\n", cIndentation, ' ', cCache );
        printf( "%*cRequest-page traces: %I64d\r\n", cIndentation, ' ', cRequest );
        printf( "%*cDirty-page traces: %I64d\r\n", cIndentation, ' ', cDirty );
        printf( "%*cWrite-page traces: %I64d\r\n", cIndentation, ' ', cWrite );
        printf( "%*cSet-lgpos-modify traces: %I64d\r\n", cIndentation, ' ', cSetLgposModify );
        printf( "%*cEvict-page traces: %I64d\r\n", cIndentation, ' ', cEvict );
        printf( "%*cSupercold-page traces: %I64d\r\n", cIndentation, ' ', cSuperCold );
        printf( "%*cTOTAL traces: %I64d\r\n", cIndentation, ' ', cSysResMgrInit + cSysResMgrTerm + cCache + cRequest + cDirty + cWrite + cSetLgposModify + cEvict + cSuperCold );
    }
} BFTraceStats;


typedef struct _BFETLContext BFETLContext;
typedef struct _BFETLContext
{
    private:
        //  ETW trace provider:
        //    o A pointer to an array of trace pointers if in test mode (NULL is the sentinel).
        //    o A pointer to a WCHAR string with the name of the .etl file if in normal mode.

        const void* pvTraceProvider;


        //  Trace file handle

        HANDLE hETW;


        //  Special flags / options

        BOOL fTestMode;
        BOOL fCollectBfStats;
        BOOL fEOF;


        //  Tick tracking.

        DWORD cEventsBufferedMax;   //  Number of entries that we're going to keep around in order to make sure
                                    //  we always return traces in ascending order of timestamp (tick).
        typedef std::list<EtwEvent*> ListEtwEvent;
        typedef ListEtwEvent::iterator ListEtwEventIter;
        ListEtwEvent* pListEtwEvent;        //  Buffer of last N entries, sorted by tick, oldest at front.
        TICK tickLast;                      //  Last tick returned.
        TICK tickMin;                       //  Min. tick currently buffered.
        TICK tickMax;                       //  Max. tick currently buffered.


        //  Stats by PID

        typedef std::unordered_map<DWORD, BFTraceStats*> HashPidStats;
        typedef HashPidStats::iterator HashPidStatsIter;
        HashPidStats* pHashPidStats;

    public:
        //  Process IDs to collect traces from. ZERO is a special value and means "collect-all".
        //  It should be used for unit-testing only or when you're guaranteed to have only
        //  one ESE process running at a time, or traces will be overlapped.

        std::set<DWORD>* pids;

        
        //  BF trace stats

        __int64 cTracesOutOfOrder;
        __int64 cTracesProcessed;
        BFTraceStats bftstats;

        //  Trace file summary data array (one element for each PID collected).

        CFastTraceLog::BFFTLFilePostProcessHeader* rgbfftlPostProcHdr;
        CFastTraceLog::BFFTLFilePostProcessHeader& bfftlPostProcHdr( const DWORD dwPID ) const;
        CFastTraceLog::BFFTLFilePostProcessHeader& bfftlPostProcHdr() const;

    //  Friend functions.
    friend ERR ErrBFETLInit(
        __in const void * const pvTraceProvider,
        __in const std::set<DWORD>& pids,
        __in DWORD cEventsBufferedMax,
        __in const DWORD grbit,
        __out BFETLContext** const ppbfetlc );
    friend void BFETLTerm( __in BFETLContext* const pbfetlc );
    friend ERR ErrBFETLGetNext( __in BFETLContext* const pbfetlc, __out BFTRACE* const pbftrace, __out DWORD* const pdwPID );
    friend const BFTraceStats* PbftsBFETLStatsByPID( __in const BFETLContext* const pbfetlc, __inout DWORD* const pdwPID );
} BFETLContext;


//  Flags / options to ErrBFETLInit (and ErrBFETLDumpStats).

#define fBFETLDriverTestMode            (0x00000001)
#define fBFETLDriverCollectBFStats      (0x00000002)


//  Inits BFETL driver of a trace file and driver options.

ERR ErrBFETLInit(
    __in const void * const pvTraceProvider,
    __in const std::set<DWORD>& pids,
    __in DWORD cEventsBufferedMax,
    __in const DWORD grbit,
    __out BFETLContext** const ppbfetlc );
ERR ErrBFETLInit(
    __in const void * const pvTraceProvider,
    __in const DWORD dwPID,
    __in DWORD cEventsBufferedMax,
    __in const DWORD grbit,
    __out BFETLContext** const ppbfetlc );


//  Terms and cleanups up the driver handle / allocations.

void BFETLTerm( __in BFETLContext* const pbfetlc );


//  Gets the next trace in the trace file (or in the test traces).

ERR ErrBFETLGetNext( __in BFETLContext* const pbfetlc, __out BFTRACE * const pbftrace );
ERR ErrBFETLGetNext( __in BFETLContext* const pbfetlc, __out BFTRACE * const pbftrace, __out DWORD* const pdwPID );


//  Gets BFTraceStats specific to a process ID. pdwPID is an in/out parameter: the input is an iterator
//  "i" (meaning "ith" process); the output is the actual process ID, for printing purposes.
//  If "i" is beyond the number of processes for which we have traces for, the output PID will be zero
//  and the returned pointer will be NULL.

const BFTraceStats* PbftsBFETLStatsByPID( __in const BFETLContext* const pbfetlc, __inout DWORD* const pdwPID );


//  Converts an ETL file into an FTL post-processed file.

ERR ErrBFETLConvertToFTL(
    __in const WCHAR* const wszEtlFilePath,
    __in const WCHAR* const wszFtlFilePath,
    __in const std::set<DWORD>& pids );


//  Compares an ETL against an FTL file. Returns JET_errDatabaseCorrupted if they are not the same.

ERR ErrBFETLFTLCmp(
    __in const void* pvTraceProviderEtl,
    __in const void* pvTraceProviderFtl,
    __in const DWORD dwPID,
    __in const BOOL fTestMode = fFalse,
    __out __int64* const pcTracesProcessed = NULL );


//  Dump stats related to the ETL file.

ERR ErrBFETLDumpStats( __in const BFETLContext* const pbfetlc, __in const DWORD grbit );

#endif _BFETLDRIVER_HXX_INCLUDED

