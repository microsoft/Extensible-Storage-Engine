// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "resmgremulatorunit.hxx"


class ResMgrEmulatorNegativeTest : public UNITTEST
{
    private:
        static ResMgrEmulatorNegativeTest s_instance;

    protected:
        ResMgrEmulatorNegativeTest() {}
    public:
        ~ResMgrEmulatorNegativeTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();

    private:
        ERR ErrConfigSetInvalidParams_();
        ERR ErrInitContextNull_();
        ERR ErrInitAlgorithmNull_();
        ERR ErrInitTwice_();
        ERR ErrExecuteNoInit_();
        ERR ErrExecuteTwice_();
        ERR ErrDumpStatsNoInit_();
        ERR ErrDumpStatsNoExecute_();
        ERR ErrDumpStatsTerm_();
};

ResMgrEmulatorNegativeTest ResMgrEmulatorNegativeTest::s_instance;

const char * ResMgrEmulatorNegativeTest::SzName() const         { return "ResMgrEmulatorNegativeTest"; };
const char * ResMgrEmulatorNegativeTest::SzDescription() const  { return "Negative tests for PageEvictionEmulator."; }
bool ResMgrEmulatorNegativeTest::FRunUnderESE98() const         { return true; }
bool ResMgrEmulatorNegativeTest::FRunUnderESENT() const         { return true; }
bool ResMgrEmulatorNegativeTest::FRunUnderESE97() const         { return true; }


ERR ResMgrEmulatorNegativeTest::ErrTest()
{
    ERR err = JET_errSuccess;
    
    TestCall( ErrConfigSetInvalidParams_() );
    TestCall( ErrInitContextNull_() );
    TestCall( ErrInitAlgorithmNull_() );
    TestCall( ErrInitTwice_() );
    TestCall( ErrExecuteNoInit_() );
    TestCall( ErrExecuteTwice_() );
    TestCall( ErrDumpStatsNoInit_() );
    TestCall( ErrDumpStatsNoExecute_() );
    TestCall( ErrDumpStatsTerm_() );

HandleError:
    return err;
}

ERR ResMgrEmulatorNegativeTest::ErrConfigSetInvalidParams_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    BFTRACE bftrace = { 0 };
    bftrace.traceid = bftidInvalid;
    PageEvictionAlgorithmLRUTest algorithm;

    TestCall( ErrBFFTLInit( &bftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    
    TestCheck( emulator.ErrSetFaultsHistoRes( 0 ) == JET_errInvalidParameter );
    TestCheck( emulator.ErrSetLifetimeHistoRes( 0 ) == JET_errInvalidParameter );

HandleError:

    BFFTLTerm( pbfftlc );

    return err;
}

ERR ResMgrEmulatorNegativeTest::ErrInitContextNull_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    PageEvictionAlgorithmLRUTest algorithm;

    TestCheck( emulator.ErrInit( NULL, &algorithm ) == JET_errInvalidParameter );

HandleError:

    emulator.Term();

    return err;
}

ERR ResMgrEmulatorNegativeTest::ErrInitAlgorithmNull_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    BFTRACE bftrace = { 0 };
    bftrace.traceid = bftidInvalid;

    TestCall( ErrBFFTLInit( &bftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    TestCheck( emulator.ErrInit( pbfftlc, NULL ) == JET_errInvalidParameter );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

ERR ResMgrEmulatorNegativeTest::ErrInitTwice_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    BFTRACE bftrace = { 0 };
    bftrace.traceid = bftidInvalid;
    PageEvictionAlgorithmLRUTest algorithm;

    TestCall( ErrBFFTLInit( &bftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCheck( emulator.ErrInit( pbfftlc, &algorithm ) == JET_errTestError );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

ERR ResMgrEmulatorNegativeTest::ErrExecuteNoInit_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    TestCheck( emulator.ErrExecute() == JET_errTestError );

HandleError:

    return err;
}

ERR ResMgrEmulatorNegativeTest::ErrExecuteTwice_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    BFTRACE bftrace = { 0 };
    bftrace.traceid = bftidInvalid;
    PageEvictionAlgorithmLRUTest algorithm;

    TestCall( ErrBFFTLInit( &bftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );
    TestCheck( emulator.ErrExecute() == JET_errTestError );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

ERR ResMgrEmulatorNegativeTest::ErrDumpStatsNoInit_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    TestCheck( emulator.ErrDumpStats() == JET_errTestError );

HandleError:

    return err;
}

ERR ResMgrEmulatorNegativeTest::ErrDumpStatsNoExecute_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    BFTRACE bftrace = { 0 };
    bftrace.traceid = bftidInvalid;
    PageEvictionAlgorithmLRUTest algorithm;

    TestCall( ErrBFFTLInit( &bftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCheck( emulator.ErrDumpStats() == JET_errTestError );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

ERR ResMgrEmulatorNegativeTest::ErrDumpStatsTerm_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    BFTRACE bftrace = { 0 };
    bftrace.traceid = bftidInvalid;
    PageEvictionAlgorithmLRUTest algorithm;

    TestCall( ErrBFFTLInit( &bftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );
    emulator.Term();
    TestCheck( emulator.ErrDumpStats() == JET_errTestError );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

