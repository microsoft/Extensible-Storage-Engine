// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef __OSTLS_HXX_INCLUDED
#define __OSTLS_HXX_INCLUDED

//  The OS TLS for the OS Layer / Library to use.
//
//  Note: This is subtly different than _tls.hxx, which defines the internal
//  TLS structure for the thread.cxx / TLS sub-component of the OS library, and 
//  different from the User TLS.
//

#include "_osdisk.hxx"  // needs access to COSDisk::IORun ...

class COSTimerTaskEntry;

struct OSTLS
{
    union
    {
        INT                     fFlags;
        struct
        {
            FLAG32              fIsTaskThread:1;        //  Is this an internal task thread
            FLAG32              fIOThread:1;            //  this thread is in the I/O thread pool
        };
    };

    CErrFrameSimple             m_efLastErr;            //  Error frame that ErrERRCheck was called in

#ifdef DEBUG
    const CHAR *                szFileLastCall;         //  file that last failed call was in
    ULONG               ulLineLastCall;         //  line number that last failed call was on
    INT                         errLastCall;            //  error that call failed on

    BOOL                        fTrackErrors;           //  start ref tracing errors
#endif  //  DEBUG

#ifdef RFS2
    LONG                        cRFSCountdown;          //  failure count for RFS violators before disabling RFS
#endif  //  RFS2

    const CHAR *                szNewFile;
    ULONG               ulNewLine;

    //      I/O Manager properties

    IOREQ*                      pioreqCache;            //  cached IOREQ for use by I/O completion

    COSDisk::CIoRunPool         iorunpool;

    //      other properties

    const _TCHAR*               szCprintfPrefix;

    ULONG               dwCurPerfObj;           //  perf object for which data is currently being collected

    TICK                        dtickTaskThreadIdle;    //  task manager thread idle timeout
    CTaskManager::PfnCompletion pfnTaskThreadIdle;      //  task manager thread idle callback

    COSTimerTaskEntry *         posttExecuting;         //  timer task that is executing on this thread

    const UserTraceContext*     putc;                   //  user supplied trace context for this thread
    TraceContext            etc;                    //  engine trace context for this thread

#ifdef DEBUG
    BOOL                        fCleanupState;          //  indicates we are in a "cleanup path" (such as trx rollback), controls checking for no new allocations
#endif  //  DEBUG
};

OSTLS* const Postls();
OSTLS* PostlsFromTLS( TLS* ptls );

//
//  OSTLS element accessors
//

__forceinline CErrFrameSimple * PefLastThrow();
ULONG UlLineLastCall();

#endif  //  __OSTLS_HXX_INCLUDED

