// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef __OSTLS_HXX_INCLUDED
#define __OSTLS_HXX_INCLUDED


#include "_osdisk.hxx"

class COSTimerTaskEntry;

struct OSTLS
{
    union
    {
        INT                     fFlags;
        struct
        {
            FLAG32              fIsTaskThread:1;
            FLAG32              fIOThread:1;
        };
    };

    CErrFrameSimple             m_efLastErr;

#ifdef DEBUG
    const CHAR *                szFileLastCall;
    ULONG               ulLineLastCall;
    INT                         errLastCall;

    BOOL                        fTrackErrors;
#endif

#ifdef RFS2
    LONG                        cRFSCountdown;
#endif

    const CHAR *                szNewFile;
    ULONG               ulNewLine;


    IOREQ*                      pioreqCache;

    COSDisk::CIoRunPool         iorunpool;


    const _TCHAR*               szCprintfPrefix;

    ULONG               dwCurPerfObj;

    TICK                        dtickTaskThreadIdle;
    CTaskManager::PfnCompletion pfnTaskThreadIdle;

    COSTimerTaskEntry *         posttExecuting;

    const UserTraceContext*     putc;
    TraceContext            etc;

#ifdef DEBUG
    BOOL                        fCleanupState;
#endif
};

OSTLS* const Postls();
OSTLS* PostlsFromTLS( TLS* ptls );


__forceinline CErrFrameSimple * PefLastThrow();
ULONG UlLineLastCall();

#endif

