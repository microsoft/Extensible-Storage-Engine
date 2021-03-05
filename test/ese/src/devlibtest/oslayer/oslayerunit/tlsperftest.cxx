// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

struct TLS {
    ULONG   i;
};

class TLSPERFTEST : public UNITTEST
{
    private:
        static TLSPERFTEST s_instance;
        static QWORD s_cIterations;
        volatile QWORD m_iIteration;
        volatile TLS* m_ptlsBuffer;
        volatile DWORD m_dwGle;

    protected:
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
            
        void ReportResults_( const char* const szTestName,
                                const FILETIME ftKernelTimeBegin, const FILETIME ftUserTimeBegin,
                                const FILETIME ftKernelTimeEnd, const FILETIME ftUserTimeEnd ) const
        {
            const ULONGLONG dtickPerLoopKernel = TLSPERFTEST::DTickFromFileTimes_( ftKernelTimeBegin, ftKernelTimeEnd );
            const ULONGLONG dtickPerLoopUser = TLSPERFTEST::DTickFromFileTimes_( ftUserTimeBegin, ftUserTimeEnd );
            
            const double dbltickPerLoopKernel = dtickPerLoopKernel * 100.0 / s_cIterations;
            const double dbltickPerLoopUser = dtickPerLoopUser * 100.0 / s_cIterations;
            const double dbltickPerLoopTotal = dbltickPerLoopKernel + dbltickPerLoopUser;
            
            wprintf( L"\t%hs: kernel time %.3lf nsec, user time %.3lf nsec, total time %.3lf nsec\n", szTestName, dbltickPerLoopKernel, dbltickPerLoopUser, dbltickPerLoopTotal );
        }

        TLSPERFTEST()
        {
        }

    public:
        ~TLSPERFTEST()
        {
        }

    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunByDefault() const;
        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        JET_ERR ErrTestLoopPerf();
        JET_ERR ErrPTlsPerf();
        JET_ERR ErrGlePerf();
        JET_ERR ErrSlePerf();

        ERR ErrTest();
};

TLSPERFTEST TLSPERFTEST::s_instance;
QWORD TLSPERFTEST::s_cIterations = 1000000000;

const char * TLSPERFTEST::SzName() const        { return "TLS Performance Test"; };
const char * TLSPERFTEST::SzDescription() const { return 
            "Tests the functions in the OS layer that deal with the TLS.";
        }
bool TLSPERFTEST::FRunByDefault() const         { return false; }
bool TLSPERFTEST::FRunUnderESE98() const        { return true; }
bool TLSPERFTEST::FRunUnderESENT() const        { return true; }
bool TLSPERFTEST::FRunUnderESE97() const        { return true; }

JET_ERR TLSPERFTEST::ErrTestLoopPerf()
{
    JET_ERR err = JET_errSuccess;
    
    FILETIME ftCreationTime, ftExitTime, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd;
    
    const HANDLE hThread = GetCurrentThread();
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeBegin, &ftUserTimeBegin ) != 0 );
    for ( m_iIteration = 0 ; m_iIteration < s_cIterations ; m_iIteration++ )
    {
    }
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeEnd, &ftUserTimeEnd ) != 0 );

    this->ReportResults_( __FUNCTION__, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd );
    
HandleError:
    return err;
}

JET_ERR TLSPERFTEST::ErrPTlsPerf()
{
    JET_ERR err = JET_errSuccess;
    
    FILETIME ftCreationTime, ftExitTime, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd;

    m_ptlsBuffer = Ptls();
    const HANDLE hThread = GetCurrentThread();
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeBegin, &ftUserTimeBegin ) != 0 );
    for ( m_iIteration = 0 ; m_iIteration < s_cIterations ; m_iIteration++ )
    {
        m_ptlsBuffer = Ptls();
    }
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeEnd, &ftUserTimeEnd ) != 0 );

    this->ReportResults_( __FUNCTION__, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd );
    
HandleError:
    return err;
}

JET_ERR TLSPERFTEST::ErrGlePerf()
{
    JET_ERR err = JET_errSuccess;
    
    FILETIME ftCreationTime, ftExitTime, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd;

    m_dwGle = GetLastError();
    const HANDLE hThread = GetCurrentThread();
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeBegin, &ftUserTimeBegin ) != 0 );
    for ( m_iIteration = 0 ; m_iIteration < s_cIterations ; m_iIteration++ )
    {
        m_dwGle = GetLastError();
    }
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeEnd, &ftUserTimeEnd ) != 0 );

    this->ReportResults_( __FUNCTION__, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd );
    
HandleError:
    return err;
}

JET_ERR TLSPERFTEST::ErrSlePerf()
{
    JET_ERR err = JET_errSuccess;
    
    FILETIME ftCreationTime, ftExitTime, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd;

    SetLastError( 0 );
    const HANDLE hThread = GetCurrentThread();
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeBegin, &ftUserTimeBegin ) != 0 );
    for ( m_iIteration = 0 ; m_iIteration < s_cIterations ; m_iIteration++ )
    {
        SetLastError( 0 );
    }
    OSTestCheck( GetThreadTimes( hThread, &ftCreationTime, &ftExitTime, &ftKernelTimeEnd, &ftUserTimeEnd ) != 0 );

    this->ReportResults_( __FUNCTION__, ftKernelTimeBegin, ftUserTimeBegin, ftKernelTimeEnd, ftUserTimeEnd );
    
HandleError:
    return err;
}

ERR TLSPERFTEST::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    wprintf( L"\tTesting time functions...\n");

    COSLayerPreInit     oslayer;

    OSTestCheck( oslayer.FInitd() );

    OSTestCall( this->ErrTestLoopPerf() );
    OSTestCall( this->ErrPTlsPerf() );
    OSTestCall( this->ErrGlePerf() );
    OSTestCall( this->ErrSlePerf() );

HandleError:
    if ( err )
    {
        wprintf( L"\tDone(error).\n");
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    return err;
}

