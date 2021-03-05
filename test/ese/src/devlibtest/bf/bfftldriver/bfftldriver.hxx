// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#ifndef _BFFTLDRIVER_HXX_INCLUDED
#define _BFFTLDRIVER_HXX_INCLUDED

typedef struct
{
    DWORD           grbit;


    WCHAR **        wszTraceLogFiles;
    ULONG           cTraceLogFiles;
    ULONG           iTraceLogFile;


    BFTRACE *       rgFakeTraces;
    ULONG           iFake;


    CFastTraceLog *                 pftl;
    CFastTraceLog::CFTLReader *     pftlr;
    CFastTraceLog::FTLTrace         ftltraceCurrent;

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



    IFMP            cIFMP;
    CPG             rgpgnoMax[ifmpMax];

} BFFTLContext;



#define fBFFTLDriverResMgrTraces        (0x01)
#define fBFFTLDriverDirtyTraces         (0x02)
#define fBFFTLDriverCollectFTLStats     (0x0100)
#define fBFFTLDriverCollectBFStats      (0x0200)
#define fBFFTLDriverTestMode            (0x010000)


ERR ErrBFFTLInit( __in const void * const pvContext, __in const DWORD grbit, __out BFFTLContext ** ppbfftlc );


void BFFTLTerm( __out BFFTLContext * pbfftlc );


ERR ErrBFFTLDumpStats( __in const BFFTLContext * pbfftlc, __in const DWORD grbit );


ERR ErrBFFTLGetNext( __inout BFFTLContext * pbfftlc, __out BFTRACE * pbftrace );


QWORD IbBFFTLBookmark( __out const BFFTLContext * pbfftlc );


ERR ErrBFFTLPostProcess( __in const BFFTLContext * pbfftlc );


#endif _BFFTLDRIVER_HXX_INCLUDED

