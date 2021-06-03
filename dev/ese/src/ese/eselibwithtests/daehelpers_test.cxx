// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifndef ENABLE_JET_UNIT_TEST
#error This file should only be compiled with the unit tests!
#endif // ENABLE_JET_UNIT_TEST


JETUNITTEST( CIsamSequenceDiagLog, Basic )
{
    CIsamSequenceDiagLog isdl( isdltypeTest );

    isdl.InitSequence( isdltypeTest, 4 );

    isdl.Trigger( 0 );
    isdl.Trigger( 1 );
    isdl.Trigger( 2 );
    isdl.Trigger( 3 );

    WCHAR * wszT = (WCHAR*)_alloca( isdl.CbSprintTimings() );
    isdl.SprintTimings( wszT, isdl.CbSprintTimings() );
    wprintf( L"wszT = %ws\n", wszT );

    isdl.TermSequence();
}

JETUNITTEST( CIsamSequenceDiagLog, CanTermSeqTwice )
{
    CIsamSequenceDiagLog isdl( isdltypeTest );

    isdl.InitSequence( isdltypeTest, 4 );

    isdl.Trigger( 0 );
    isdl.Trigger( 1 );
    isdl.Trigger( 2 );
    isdl.Trigger( 3 );

    WCHAR * wszT = (WCHAR*)_alloca( isdl.CbSprintTimings() );
    isdl.SprintTimings( wszT, isdl.CbSprintTimings() );
    wprintf( L"wszT = %ws\n", wszT );

    isdl.TermSequence();
    isdl.TermSequence();    // Test - should not assert or anything bad.
}

JETUNITTEST( CIsamSequenceDiagLog, SingleLogSeqStats )
{
    CIsamSequenceDiagLog isdl( isdltypeTest );

    wprintf( L" CIsamSequenceDiagLog::cbSingleStepPreAlloc = %d\n", CIsamSequenceDiagLog::cbSingleStepPreAlloc );
    BYTE rgb[CIsamSequenceDiagLog::cbSingleStepPreAlloc];

    isdl.InitSingleStep( isdltypeTest, rgb, sizeof(rgb) );

    isdl.AddCallbackTime( 1.1 );
    isdl.AddThrottleTime( 2.2 );
    Ptls()->threadstats.cPagePreread++;
    UtilSleep( 1 );
    isdl.Trigger( 1 );

    WCHAR * wszT = (WCHAR*)_alloca( isdl.CbSprintTimings() );
    isdl.SprintTimings( wszT, isdl.CbSprintTimings() );
    wprintf( L"wszT = %ws\n", wszT );

    isdl.TermSequence();
}

JETUNITTEST( CIsamSequenceDiagLog, SingleLogSeqStatsObjectReuse )
{
    CIsamSequenceDiagLog isdl( isdltypeTest );

    BYTE rgb[CIsamSequenceDiagLog::cbSingleStepPreAlloc];

    isdl.InitSingleStep( isdltypeTest, rgb, sizeof(rgb) );

    isdl.AddCallbackTime( 1.1 );
    isdl.AddThrottleTime( 2.2 );
    Ptls()->threadstats.cPagePreread++;
    UtilSleep( 20 );
    isdl.Trigger( 1 );

    WCHAR * wszT = (WCHAR*)_alloca( isdl.CbSprintTimings() );
    isdl.SprintTimings( wszT, isdl.CbSprintTimings() );
    wprintf( L"wszT1 = %ws\n", wszT );

    isdl.TermSequence();
    isdl.InitSingleStep( isdltypeTest, rgb, sizeof(rgb) );

    isdl.AddCallbackTime( 0.1 );
    isdl.AddThrottleTime( 0.2 );
    Ptls()->threadstats.cPagePreread++;
    UtilSleep( 20 );
    isdl.Trigger( 1 );

    isdl.SprintTimings( wszT, isdl.CbSprintTimings() );
    wprintf( L"wszT2 = %ws\n", wszT );

    isdl.TermSequence();
}

