// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _TLS_HXX_INCLUDED
#define _TLS_HXX_INCLUDED


//  Thread Local Storage

//  TLS structure

class CBFIssueList;

struct TLS
{
public:
    union
    {
        INT         fFlags;
        struct
        {
            FLAG32  fIsRCECleanup:1;    //  VER:  is this thread currently in RCEClean?
            FLAG32  fAddColumn:1;       //  VER:  are we currently creating an AddColumn RCE?
            FLAG32  fInCallback:1;      //  CALLBACKS: are we currently in a callback
            FLAG32  fCheckpoint:1;      //  this thread is performing checkpoint advancement
            FLAG32  fInSoftStart:1;     //  are we currently running recovery / JetInit-style (note off for JetRestore())
            FLAG32  fInJetAPI:1;        //  is this thread in a call to a Jet API? (to prevent recursion except in specific cases)
#ifdef DEBUG
            FLAG32  fNoExtendingDuringCreateDB:1;   //  just to check that we don't extend the DB while creating the DB (implying we need to increase constants in CpgDBDatabaseMinMin())
            FLAG32  fFfbFreePath:1;     //  for paths that should stay clear of FFB calls for performance reasons (does not suppress FFB, just asserts there are no such calls).
#endif
        };
    };
    INT             ccallbacksNested;   //  CALLBACKS: how many callbacks are we currently nested?

    CBFIssueList*   pbfil;              //  per thread I/O issue list for BF
    ULONG           cbfAsyncReadIOs;    //  per thread I/O read (un-issued) count for BF

    HRT             hrtWaitBegin;       //  last time the thread began a wait (for thread stats)

    INLINE BOOL FIsTaskThread( void )
    {
        return FOSTaskIsTaskThread();
    }

private:
    TICK    tickThreadStatsLast;    //  last time the user retrieved stats for this thread

public:
    void RefreshTickThreadStatsLast()
    {
        const TICK tickNow = TickOSTimeCurrent();
        tickThreadStatsLast = ( tickNow == 0 ) ? 1 : tickNow;
    }
    INLINE TICK TickThreadStatsLast()
    {
        if ( tickThreadStatsLast == 0 )
        {
            RefreshTickThreadStatsLast();
        }

        return tickThreadStatsLast;
    }
    JET_THREADSTATS4    threadstats;    //  perf counters for this thread

    void*           pvfmp;          // thread currently has pfmp->m_rwlBFContext locked
    void*           pvfmpOld;       // history to support one level recursion

    void SetPFMP( void* pv )
    {
        Assert( !pvfmpOld && ( !pvfmp || pvfmp == pv ) );
        pvfmpOld = pvfmp;
        pvfmp = pv;
    }
    void ResetPFMP( void* pv )  {
        Assert( pvfmp == pv );
        pvfmp = pvfmpOld;
        pvfmpOld = NULL;
    }
    void* PFMP() const
    {
        return pvfmp;
    }
};

static_assert( sizeof( TLS ) == ESE_USER_TLS_SIZE, "The OS layer is configured early in DLLEntryPoint() with this size. If you're changing the TLS size, please update ESE_USER_TLS_SIZE accordingly." );

#endif  //  _TLS_HXX_INCLUDED


