// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

class TimeTestController
{
    public:
        static QWORD    s_cIterations;
        volatile QWORD  m_iIteration;

    private:
        static ULONGLONG TickFromFileTime_( const FILETIME ft )
        {
            ULARGE_INTEGER tick;
            
            tick.u.LowPart = ft.dwLowDateTime;
            tick.u.HighPart = ft.dwHighDateTime;
            
            return tick.QuadPart;
        }
            
        static ULONGLONG DTickFromFileTimes_( const FILETIME ft1, const FILETIME ft2 )
        {
            return TickFromFileTime_( ft2 ) - TickFromFileTime_( ft1 );
        }
            
    public:
        void ReportResults_( const char* const szTestName,
                                const FILETIME ftKernelTimeBegin, const FILETIME ftUserTimeBegin,
                                const FILETIME ftKernelTimeEnd, const FILETIME ftUserTimeEnd ) const
        {
            const ULONGLONG dtickPerLoopKernel = TimeTestController::DTickFromFileTimes_( ftKernelTimeBegin, ftKernelTimeEnd );
            const ULONGLONG dtickPerLoopUser = TimeTestController::DTickFromFileTimes_( ftUserTimeBegin, ftUserTimeEnd );
            
            const double dbltickPerLoopKernel = dtickPerLoopKernel * 100.0 / s_cIterations;
            const double dbltickPerLoopUser = dtickPerLoopUser * 100.0 / s_cIterations;
            const double dbltickPerLoopTotal = dbltickPerLoopKernel + dbltickPerLoopUser;
            
            wprintf( L"\t%hs: kernel time %.3lf nsec, user time %.3lf nsec, total time %.3lf nsec\n", szTestName, dbltickPerLoopKernel, dbltickPerLoopUser, dbltickPerLoopTotal );
        }
};

QWORD TimeTestController::s_cIterations = 100000000;
TimeTestController g_ttctrl;



CUnitTest( OslayerHrtCheckPerfTestLoopBaseline, 0, "Test for perf for  ." );
ERR OslayerHrtCheckPerfTestLoopBaseline::ErrTest()
{
    JET_ERR err = JET_errSuccess;
    
    FILETIME ftCreationTime, ftExitTime, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd;
    
    const HANDLE hThread = GetCurrentThread();
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeBegin, &ftUserTimeBegin ) != 0 );
    for ( g_ttctrl.m_iIteration = 0 ; g_ttctrl.m_iIteration < g_ttctrl.s_cIterations ; g_ttctrl.m_iIteration++ )
    {
    }
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeEnd, &ftUserTimeEnd ) != 0 );

    g_ttctrl.ReportResults_( __FUNCTION__, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd );
    
HandleError:
    return err;
}

CUnitTest( OslayerHrtCheckPerfErrTickOSTimeCurrentPerf, 0, "Loop to check ErrTickOSTimeCurrent() perf." );
ERR OslayerHrtCheckPerfErrTickOSTimeCurrentPerf::ErrTest()
{
    JET_ERR err = JET_errSuccess;
    volatile DWORD dwBuffer;
    
    FILETIME ftCreationTime, ftExitTime, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd;
    
    const HANDLE hThread = GetCurrentThread();
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeBegin, &ftUserTimeBegin ) != 0 );
    for ( g_ttctrl.m_iIteration = 0 ; g_ttctrl.m_iIteration < g_ttctrl.s_cIterations ; g_ttctrl.m_iIteration++ )
    {
        dwBuffer = TickOSTimeCurrent();
    }
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeEnd, &ftUserTimeEnd ) != 0 );

    g_ttctrl.ReportResults_( __FUNCTION__, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd );
    
HandleError:
    return err;
}

CUnitTest( OslayerHrtCheckPerfErrHrtHRTCountPerf, 0, "Loop to check ErrHrtHRTCount() perf." );
ERR OslayerHrtCheckPerfErrHrtHRTCountPerf::ErrTest()
{
    JET_ERR err = JET_errSuccess;
    volatile QWORD qwBuffer;
    
    FILETIME ftCreationTime, ftExitTime, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd;

    qwBuffer = HrtHRTCount();
    const HANDLE hThread = GetCurrentThread();
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeBegin, &ftUserTimeBegin ) != 0 );
    for ( g_ttctrl.m_iIteration = 0 ; g_ttctrl.m_iIteration < g_ttctrl.s_cIterations ; g_ttctrl.m_iIteration++ )
    {
        qwBuffer = HrtHRTCount();
    }
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeEnd, &ftUserTimeEnd ) != 0 );

    g_ttctrl.ReportResults_( __FUNCTION__, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd );
    
HandleError:
    return err;
}

CUnitTest( OslayerHrtCheckPerfErrHrtHRTFreqPerf, 0, "Loop to check ErrHrtHRTFreq() perf." );
ERR OslayerHrtCheckPerfErrHrtHRTFreqPerf::ErrTest()
{
    JET_ERR err = JET_errSuccess;
    volatile QWORD qwBuffer;

    FILETIME ftCreationTime, ftExitTime, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd;
    
    qwBuffer = HrtHRTFreq();
    const HANDLE hThread = GetCurrentThread();
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeBegin, &ftUserTimeBegin ) != 0 );
    for ( g_ttctrl.m_iIteration = 0 ; g_ttctrl.m_iIteration < g_ttctrl.s_cIterations ; g_ttctrl.m_iIteration++ )
    {
        qwBuffer = HrtHRTFreq();
    }
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeEnd, &ftUserTimeEnd ) != 0 );

    g_ttctrl.ReportResults_( __FUNCTION__, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd );
    
HandleError:
    return err;
}

CUnitTest( OslayerHrtDblHRTDeltaWorks, 0, "Test DblHRTDelta Works." );
ERR OslayerHrtDblHRTDeltaWorks::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    HRT hrt = HrtHRTCount();
    DWORD ms = 159;
    UtilSleep( ms );
    double dds = DblHRTDelta( hrt, HrtHRTCount() );
    TICK tick = TICK( dds * 1000.0 );
    OSTestCheck( tick > ( ms - 48 ) );
    OSTestCheck( tick < ( ms + 48 ) );

HandleError:
    return err;
}

CUnitTest( OslayerHrtTestHrtCusecTimeSeemsAccurate, 0, "Test CusecHRTFromDhrt Works." );
ERR OslayerHrtTestHrtCusecTimeSeemsAccurate::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    const HRT dhrtFrequence = HrtHRTFreq();

    const HRT hrtStart = HrtHRTCount();
    QWORD cusecTest = 80 * 1000;
    UtilSleep( (DWORD)cusecTest / 1000 );
    HRT dhrt = HrtHRTCount() - hrtStart;
    const QWORD cusecActual = CusecHRTFromDhrt( dhrt );
    OSTestCheck( cusecActual > ( cusecTest - 48 * 1000 ) );
    OSTestCheck( cusecActual < ( cusecTest + 48 * 1000 ) );
    wprintf( L"    TimeMissed = %I64d ( target %I64d, @ %I64d hrz )\n", cusecActual - cusecTest, cusecTest, dhrtFrequence );

    OSTestCheck( CusecHRTFromDhrt( 0 ) == 0 );

    OSTestCheck( CusecHRTFromDhrt( 1 ) == 0 );

    wprintf( L"    Checking frequence %I64d and HRT latency time tests successful.\n", dhrtFrequence );

HandleError:
    return err;
}

