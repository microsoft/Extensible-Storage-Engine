// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#ifndef _BFETLDRIVER_HXX_INCLUDED
#define _BFETLDRIVER_HXX_INCLUDED

#include <tchar.h>
#include "os.hxx"
#include "bfftl.hxx"
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

        const void* pvTraceProvider;



        HANDLE hETW;



        BOOL fTestMode;
        BOOL fCollectBfStats;
        BOOL fEOF;



        DWORD cEventsBufferedMax;
        typedef std::list<EtwEvent*> ListEtwEvent;
        typedef ListEtwEvent::iterator ListEtwEventIter;
        ListEtwEvent* pListEtwEvent;
        TICK tickLast;
        TICK tickMin;
        TICK tickMax;



        typedef std::unordered_map<DWORD, BFTraceStats*> HashPidStats;
        typedef HashPidStats::iterator HashPidStatsIter;
        HashPidStats* pHashPidStats;

    public:

        std::set<DWORD>* pids;

        

        __int64 cTracesOutOfOrder;
        __int64 cTracesProcessed;
        BFTraceStats bftstats;


        CFastTraceLog::BFFTLFilePostProcessHeader* rgbfftlPostProcHdr;
        CFastTraceLog::BFFTLFilePostProcessHeader& bfftlPostProcHdr( const DWORD dwPID ) const;
        CFastTraceLog::BFFTLFilePostProcessHeader& bfftlPostProcHdr() const;

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



#define fBFETLDriverTestMode            (0x00000001)
#define fBFETLDriverCollectBFStats      (0x00000002)



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



void BFETLTerm( __in BFETLContext* const pbfetlc );



ERR ErrBFETLGetNext( __in BFETLContext* const pbfetlc, __out BFTRACE * const pbftrace );
ERR ErrBFETLGetNext( __in BFETLContext* const pbfetlc, __out BFTRACE * const pbftrace, __out DWORD* const pdwPID );



const BFTraceStats* PbftsBFETLStatsByPID( __in const BFETLContext* const pbfetlc, __inout DWORD* const pdwPID );



ERR ErrBFETLConvertToFTL(
    __in const WCHAR* const wszEtlFilePath,
    __in const WCHAR* const wszFtlFilePath,
    __in const std::set<DWORD>& pids );



ERR ErrBFETLFTLCmp(
    __in const void* pvTraceProviderEtl,
    __in const void* pvTraceProviderFtl,
    __in const DWORD dwPID,
    __in const BOOL fTestMode = fFalse,
    __out __int64* const pcTracesProcessed = NULL );



ERR ErrBFETLDumpStats( __in const BFETLContext* const pbfetlc, __in const DWORD grbit );

#endif _BFETLDRIVER_HXX_INCLUDED

