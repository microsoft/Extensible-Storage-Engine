// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  This file declares the interface for the BF FTL driver, which allows
//  a component to convert a file (or test traces) to an enumerator model
//  for easy trace processing.

#ifndef _BFFTLDRIVER_HXX_INCLUDED
#define _BFFTLDRIVER_HXX_INCLUDED

typedef struct
{
    DWORD           grbit;

    //  BF FTL Trace File

    WCHAR **        wszTraceLogFiles;
    ULONG           cTraceLogFiles;
    ULONG           iTraceLogFile;

    //  Test Tracing - tracks the trace we're on when test traces are loaded.

    BFTRACE *       rgFakeTraces;
    ULONG           iFake;

    //  Trace File

    CFastTraceLog *                 pftl;
    CFastTraceLog::CFTLReader *     pftlr;
    CFastTraceLog::FTLTrace         ftltraceCurrent;

    //  BF Trace Stats
    __int64         cTracesProcessed;
    __int64         cSysResMgrInit;
    __int64         cSysResMgrTerm;
    __int64         cCache;
    __int64         cVerify;
    __int64         cTouch;
    __int64         cDirty;
    __int64         cWrite;
    __int64         cSetLgposModify;
    __int64         cEvict;
    __int64         cSuperCold;


    //  Trace File Summary Data (populated by trace file post processing)

    // consider rename to more std cifmp & mpifmpcpgMax[ifmpMax];
    IFMP            cIFMP;
    CPG             rgpgnoMax[ifmpMax];

} BFFTLContext;


//  Flags / options to ErrBFFTLInit( and ErrBFFTLDumpStats() )

#define fBFFTLDriverResMgrTraces        (0x01)
#define fBFFTLDriverDirtyTraces         (0x02)
#define fBFFTLDriverCollectFTLStats     (0x0100)
#define fBFFTLDriverCollectBFStats      (0x0200)
#define fBFFTLDriverTestMode            (0x010000)

//  Inits BFFTL driver of a trace file and driver options.

ERR ErrBFFTLInit( __in const void * const pvContext, __in const DWORD grbit, __out BFFTLContext ** ppbfftlc );

//  Terms and cleanups up the driver handle / allocations.

void BFFTLTerm( __out BFFTLContext * pbfftlc );

//  Dumps any stats specified by the flags

ERR ErrBFFTLDumpStats( __in const BFFTLContext * pbfftlc, __in const DWORD grbit );

//  Gets the next trace in the trace file (or in the test traces)

ERR ErrBFFTLGetNext( __inout BFFTLContext * pbfftlc, __out BFTRACE * pbftrace );

//  Gets the bookmark of the last trace retrieved

QWORD IbBFFTLBookmark( __out const BFFTLContext * pbfftlc );

//  After enumerating all the traces, can call this to save # of IFMPs and cpgs for each IFMP

ERR ErrBFFTLPostProcess( __in const BFFTLContext * pbfftlc );


#endif _BFFTLDRIVER_HXX_INCLUDED

