// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once






#ifndef BSTF_AVOID_WIN_DEPENDENCE
#include <windows.h>
#endif
#include <wchar.h>



#include "cc.hxx"


void BstfSetVerbosity( QWORD bvl );

#define bvlPrintTests       (50)

extern QWORD    g_bvl;



void TestReportErr( long err, unsigned long ulLine, const char *szFileName );
void TestReportErr( unsigned long ulLine, const char *szFileName );


#define TestCallJ( func, label )                            \
    {                                                       \
    const ERR errFuncT = (func);                            \
    if ( errFuncT < JET_errSuccess )                        \
        {                                                   \
        TestReportErr( errFuncT, __LINE__, __FILE__ );      \
        }                                                   \
    err = errFuncT;                                         \
    if ( err < JET_errSuccess )                             \
        {                                                   \
        TestAssertFail( #func );                            \
        goto label;                                         \
        }                                                   \
    }

#define TestCallSJ( func, label )                           \
    {                                                       \
    const ERR errFuncT = (func);                            \
    if ( errFuncT != JET_errSuccess )                       \
        {                                                   \
        TestReportErr( errFuncT, __LINE__, __FILE__ );      \
        }                                                   \
    err = errFuncT;                                         \
    if ( err != JET_errSuccess )                            \
        {                                                   \
        TestAssertFail( #func );                            \
        goto label;                                         \
        }                                                   \
    }

#define TestCall( func )                        TestCallJ( func, HandleError )
#define TestCallS( func )                       TestCallSJ( func, HandleError )


extern QWORD    g_cTests;
extern QWORD    g_cTestsSucceeded;
extern QWORD    g_cTestsFailed;

extern QWORD    g_cSuppressedTestSuccesses;
extern DWORD    g_tickLastSuppressStatus;
const DWORD     g_dtickSuppressStatusUpdateDelay = 333;
DWORD DWGetTickCount();

inline void PrintClearSuppressedChecks()
    {
    if ( g_cSuppressedTestSuccesses )
        {
#ifdef BSTF_AVOID_WIN_DEPENDENCE
        printf( ". (%lld checks) Passed.\n", g_cSuppressedTestSuccesses );
#else
        wprintf( L". (%lld checks) Passed.\n", g_cSuppressedTestSuccesses );
#endif
        g_cSuppressedTestSuccesses = 0;
        }
    }

inline void TestReportSuccess_( const char * const szTestType, const char * const szReason )
    {
    g_cTestsSucceeded++;
    if ( g_bvl < bvlPrintTests && g_bvl > bvlPrintTests - 10 )
        {
        if ( g_cSuppressedTestSuccesses == 0 )
            {
#ifdef BSTF_AVOID_WIN_DEPENDENCE
            printf( "\t\t\tSuppressing Check()s: " );
#else
            wprintf( L"\t\t\tSuppressing Check()s: " );
#endif
            }

#ifndef BSTF_AVOID_WIN_DEPENDENCE
        if ( ( DWGetTickCount() - g_tickLastSuppressStatus ) > g_dtickSuppressStatusUpdateDelay )
#else
        if ( ( g_cSuppressedTestSuccesses < 10 ) ||
                ( ( g_cSuppressedTestSuccesses < 100 ) && ( ( g_cSuppressedTestSuccesses % 10 ) == 0 ) ) ||
                ( ( g_cSuppressedTestSuccesses < 1000 ) && ( ( g_cSuppressedTestSuccesses % 100 ) == 0 ) ) || 
                ( ( g_cSuppressedTestSuccesses < 10000 ) && ( ( g_cSuppressedTestSuccesses % 1000 ) == 0 ) ) ||
                ( ( g_cSuppressedTestSuccesses < 100000 ) && ( ( g_cSuppressedTestSuccesses % 10000 ) == 0 ) ) ||
                ( ( g_cSuppressedTestSuccesses < 1000000 ) && ( ( g_cSuppressedTestSuccesses % 100000 ) == 0 ) ) ||
                ( ( g_cSuppressedTestSuccesses < 10000000 ) && ( ( g_cSuppressedTestSuccesses % 1000000 ) == 0 ) ) ||
                ( ( g_cSuppressedTestSuccesses < 100000000 ) && ( ( g_cSuppressedTestSuccesses % 10000000 ) == 0 ) ) ||
                ( ( g_cSuppressedTestSuccesses < 1000000000 ) && ( ( g_cSuppressedTestSuccesses % 100000000 ) == 0 ) ) ||
                ( ( g_cSuppressedTestSuccesses < 10000000000 ) && ( ( g_cSuppressedTestSuccesses % 1000000000 ) == 0 ) ) )
#endif
            {
#ifdef BSTF_AVOID_WIN_DEPENDENCE
            printf( "." );
#else
            printf( "." );
#endif
            g_tickLastSuppressStatus = DWGetTickCount();
            }
        g_cSuppressedTestSuccesses++;
        }
    else if ( g_bvl >= bvlPrintTests )
        {
        PrintClearSuppressedChecks();
#ifdef BSTF_AVOID_WIN_DEPENDENCE
        printf( "\t\t\t%hs( %hs ) ... Passed.\n", szTestType, szReason );
#else
        wprintf( L"\t\t\t%hs( %hs ) ... Passed.\n", szTestType, szReason );
#endif
        }
    }

inline void TestReportFail_( const char * const szTestType, const char * const szReason, const unsigned long ulLine )
    {
    PrintClearSuppressedChecks();
    g_cTestsFailed++;
#ifdef BSTF_AVOID_WIN_DEPENDENCE
    printf( "\t\t\t%hs( %hs ) ... Failed @ %d!\n", szTestType, szReason, ulLine );
#else
    wprintf( L"\t\t\t%hs( %hs ) ... Failed @ %d!\n", szTestType, szReason, ulLine );
#endif
#ifdef BSTF_AVOID_WIN_DEPENDENCE
    *(int*)NULL = 0x42;
#else
    DebugBreak();
#endif
    }

#define TestCheckSuccess( _sztestcase )     TestReportSuccess_( "Check", _sztestcase )

#define TestCheckFail( _szreason )                          \
    TestReportFail_( "Check", _szreason, __LINE__ );        \
    err = ( err < 0 ) ? err : JET_errTestFailure;           \
    goto HandleError;

#define TestAssertSuccess( _sztestcase )    TestReportSuccess_( "Assert", _sztestcase )

#define TestAssertFail( _szreason )         TestReportFail_( "Assert", _szreason, __LINE__ )

#define TestCheck( _condition )                             \
    g_cTests++;                                             \
    if ( _condition )                                       \
        {                                                   \
        TestCheckSuccess( #_condition );                    \
        NULL;                                               \
        }                                                   \
    else                                                    \
        {                                                   \
        err = JET_errInvalidParameter;                      \
        TestCheckFail( #_condition );                       \
        }

#define TestCheckErr( func )                                \
    g_cTests++;                                             \
    if ( ( err = (func) ) == JET_errSuccess )               \
        {                                                   \
        TestCheckSuccess( #func );                          \
        NULL;                                               \
        }                                                   \
    else                                                    \
        {                                                   \
        printf( "[Expected=%d, Actual=%d] ", 0, err );  \
        TestCheckFail( #func );                             \
        }

#define TestCheckExpectedErr( errExpected, func )           \
    g_cTests++;                                             \
    if ( ( err = (func) ) == errExpected )                  \
        {                                                   \
        TestCheckSuccess( #func );                          \
        err = JET_errSuccess;                               \
        NULL;                                               \
        }                                                   \
    else                                                    \
        {                                                   \
        printf( "[Expected=%d, Actual=%d] ", errExpected, err );    \
        TestCheckFail( #func );                             \
        }

#define TestAssert( _condition )                            \
    g_cTests++;                                             \
    if ( _condition )                                       \
        {                                                   \
        TestAssertSuccess( #_condition );                   \
        NULL;                                               \
        }                                                   \
    else                                                    \
        {                                                   \
        TestAssertFail( #_condition );                      \
        }



enum BstfTestCaseFlags : QWORD
    {

    btcfmskTestType                     = 0xFF00000000000000,
    btcfmskTestState                    = 0x00FF000000000000,
    btcfmskTestReqs                     = 0x0000FF0000000000,

    btcfmskTestRigsFlags                = 0x000000000000FF00,
    btcfmskTestRigsReqs                 = 0x0000000000FF0000,

    btcfmskTestsFlags                   = 0x00000000000000FF,

    btcfForceStackTrash                 = 0x0000010000000000,

    bitExplicitOnly                     =       0x1,
    };


class UNITTEST
    {
    public:
        UNITTEST *              m_punittestNext;
        BstfTestCaseFlags       m_btcf;

    public:
        static UNITTEST *       s_punittestHead;

    protected:
        UNITTEST();
        ~UNITTEST();

    public:
        virtual const char * SzName() const         = 0;
        virtual const char * SzDescription() const  = 0;

        virtual bool FRunByDefault() const  { return !( m_btcf & bitExplicitOnly ); }

        virtual bool FRunUnderESE98() const { return true; }
        virtual bool FRunUnderESENT() const { return true; }
        virtual bool FRunUnderESE97() const { return false; }

        virtual ERR ErrTest( ) = 0;
    };


#define CUnitTest( TestClassName, btcf, szTestDesc )            \
    class TestClassName : public UNITTEST                       \
        {                                                       \
        private:                                                \
            static TestClassName s_instance;                    \
        protected:                                              \
            TestClassName( BstfTestCaseFlags btcfCtor )         \
                { m_btcf = btcfCtor; }                          \
        public:                                                 \
            ~TestClassName() {}                                 \
        public:                                                 \
            const char * SzName() const;                        \
            const char * SzDescription() const;                 \
            bool FRunUnderESE98() const;                        \
            bool FRunUnderESENT() const;                        \
            bool FRunUnderESE97() const;                        \
            ERR ErrTest();                                      \
        };                                                      \
    TestClassName TestClassName::s_instance( (BstfTestCaseFlags)btcf );         \
    const char * TestClassName::SzName() const      { return #TestClassName; };     \
    const char * TestClassName::SzDescription() const   { return szTestDesc; };     \
    bool TestClassName::FRunUnderESE98() const      { return true; }                \
    bool TestClassName::FRunUnderESENT() const      { return true; }                \
    bool TestClassName::FRunUnderESE97() const      { return true; }                \




inline bool FBstfHelpArg( const char * const szArg )
    {
    if ( szArg &&
        ( ( szArg[0] == '-' ) || ( szArg[0] == '/' ) ) &&
        ( ( szArg[1] == '?' ) || ( szArg[1] == 'h' ) || ( szArg[1] == 'H' ) ) )
        {
        return true;
        }
    return false;
    }



void BstfPrintTests();
ERR ErrBstfRunTests( const int argc, const char * const argv[] );

