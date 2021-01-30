// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _TLS_HXX_INCLUDED
#define _TLS_HXX_INCLUDED




class CBFIssueList;

struct TLS
{
public:
    union
    {
        INT         fFlags;
        struct
        {
            FLAG32  fIsRCECleanup:1;
            FLAG32  fAddColumn:1;
            FLAG32  fInCallback:1;
            FLAG32  fCheckpoint:1;
            FLAG32  fInSoftStart:1;
            FLAG32  fInJetAPI:1;
#ifdef DEBUG
            FLAG32  fNoExtendingDuringCreateDB:1;
            FLAG32  fFfbFreePath:1;
#endif
        };
    };
    INT             ccallbacksNested;

    CBFIssueList*   pbfil;
    ULONG           cbfAsyncReadIOs;

    HRT             hrtWaitBegin;

    INLINE BOOL FIsTaskThread( void )
    {
        return FOSTaskIsTaskThread();
    }

private:
    TICK    tickThreadStatsLast;

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
    JET_THREADSTATS4    threadstats;

    void*           pvfmp;
    void*           pvfmpOld;

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

#endif


