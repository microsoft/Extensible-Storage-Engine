// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef JETTEST_HXX_INCLUDED
#define JETTEST_HXX_INCLUDED

#ifdef ENABLE_JET_UNIT_TEST

//  ================================================================
class JetUnitTestFailure
//  ================================================================
//
//  Represents one failed assertion
//
//-
{
    public:
        JetUnitTestFailure(
            const char * const szFile,
            const INT          line,
            const char * const szCondition );
        ~JetUnitTestFailure();

        const char * SzFile() const;
        const char * SzCondition() const;
        INT Line() const;

    private:
        const char * const m_szFile;
        const char * const m_szCondition;
        const INT m_line;

    private:    // not implemented
        JetUnitTestFailure();
        JetUnitTestFailure( const JetUnitTestFailure& );
        JetUnitTestFailure& operator=( const JetUnitTestFailure& );
};

//  ================================================================
class JetUnitTestResult
//  ================================================================
//
//  This object is used to track the results of running one test.
//  Right now it just prints out the test results, but could be
//  used to keep a list of them instead.
//
//-
{
    public:
        JetUnitTestResult();
        ~JetUnitTestResult();

        void Start( const char * const szTest );
        void Finish();
        void AddFailure( const JetUnitTestFailure& failure );

        INT Failures() const;

    private:
        const char * m_szTest;
        INT m_failures;

    private:    // not implemented
        JetUnitTestResult( const JetUnitTestResult& );
        JetUnitTestResult& operator=( const JetUnitTestResult& );
};

//  ================================================================
class JetUnitTest
//  ================================================================
//
//  Each object that represents one JetTest should inherit from this
//  class. It keeps a list of tests and provides the ability to
//  run them.
//
//-
{
    public:
        static INT RunTests( const char * const szTest, const IFMP ifmpTest );
        static void PrintTests();

    private:
        static JetUnitTest * s_ptestHead;
        
    public:
        static const char chWildCard = '*';

    protected:
        JetUnitTest( const char * const szName );
        virtual ~JetUnitTest();

        virtual bool FNeedsBF();
        virtual bool FNeedsDB();
        virtual bool FRunByDefault();
        virtual void SetTestIfmp( const IFMP ifmpTest );

        virtual void Run( JetUnitTestResult * const presult ) = 0;
        
    private:
        JetUnitTest * m_ptestNext;
        const char * const m_szName;
};

//  ================================================================
class JetSimpleUnitTest : public JetUnitTest
//  ================================================================
//
//  Each basic test should inherit from this. There are two parts to this
//  class. The static part keeps a list of tests and can run them.
//  The dynamic part provides the abstract testing class.
//
//-
{
    public:
        static const DWORD dwBufferManager = 0x1;
        static const DWORD dwOpenDatabase = 0x2;
        static const DWORD dwDontRunByDefault = 0x4;

    protected:
        JetSimpleUnitTest( const char * const szName );
        JetSimpleUnitTest( const char * const szName, const DWORD dwFacilities );
        virtual ~JetSimpleUnitTest();

        bool FNeedsBF() { return m_dwFacilities & dwBufferManager; }
        bool FNeedsDB() { return false; }
        bool FRunByDefault() { return !(m_dwFacilities & dwDontRunByDefault); }

        void Run( JetUnitTestResult * const presult );

        virtual void Run_() = 0;
        void Fail_( const char * const szFile, const INT line, const char * const szCondition );

    private:
        DWORD               m_dwFacilities;

    protected:
        JetUnitTestResult * m_presult;
};

//  ================================================================
template<class FIXTURE>
class JetTestCaller : public JetSimpleUnitTest
//  ================================================================
{
public:
    typedef void (FIXTURE::*PfnTestMethod)();
    JetTestCaller( const char * const szName, PfnTestMethod pfnTest );
    JetTestCaller( const char * const szName, const DWORD dwFacilities, PfnTestMethod pfnTest );

protected:
    void Run_();

private:
    const PfnTestMethod m_pfnTest;
    const char * const m_szName;
};

//  ================================================================
template<class FIXTURE>
JetTestCaller<FIXTURE>::JetTestCaller( const char * const szName, PfnTestMethod pfnTest ) :
//  ================================================================
    JetSimpleUnitTest( szName ),
    m_pfnTest( pfnTest ),
    m_szName( szName )
{
}

//  ================================================================
template<class FIXTURE>
JetTestCaller<FIXTURE>::JetTestCaller( const char * const szName, const DWORD dwFacilities, PfnTestMethod pfnTest ) :
//  ================================================================
    JetSimpleUnitTest( szName, dwFacilities ),
    m_pfnTest( pfnTest ),
    m_szName( szName )
{
}

//  ================================================================
template<class FIXTURE>
void JetTestCaller<FIXTURE>::Run_()
//  ================================================================
{
    FIXTURE fixture;
    if( fixture.SetUp(m_presult) )
    {
        (fixture.*m_pfnTest)();
        fixture.TearDown();
    }
    else
    {
        JetUnitTestFailure failure( __FILE__, __LINE__, "Test fixture setup failed" );
        m_presult->AddFailure( failure );
    }
}

//  ================================================================
class JetSimpleDbUnitTest : public JetSimpleUnitTest
//  ================================================================
//
//  Each basic test should inherit from this. There are two parts to this
//  class. The static part keeps a list of tests and can run them.
//  The dynamic part provides the abstract testing class.
//
//-
{
    protected:
        JetSimpleDbUnitTest( const char * const szName );
        JetSimpleDbUnitTest( const char * const szName, const DWORD dwFacilities );
        virtual ~JetSimpleDbUnitTest();

        bool FNeedsDB() { return dwOpenDatabase == ( m_dwFacilities & dwOpenDatabase ); }
        void SetTestIfmp( const IFMP ifmpTest );
        IFMP IfmpTest();

        void Run( JetUnitTestResult * const presult );

        virtual void Run_() = 0;
        void Fail_( const char * const szFile, const INT line, const char * const szCondition );

    private:
        IFMP                m_ifmp;
        DWORD               m_dwFacilities;
        JetUnitTestResult * m_presult;
};

//  ================================================================
class JetTestFixture
//  ================================================================
//
//  A set of tests that require a common setup/teardown method should
//  inherit from this.
//
//-
{
    public:
        bool SetUp( JetUnitTestResult * const presult );
        void TearDown();
        
    protected:
        JetTestFixture();
        virtual ~JetTestFixture();

        void Fail_( const char * const szFile, const INT line, const char * const szCondition );

        virtual bool SetUp_() = 0;
        virtual void TearDown_() = 0;

    private:
        JetUnitTestResult * m_presult;
};

const DWORD JETTEST_EXCEP_ENFORCE = 0xE0E5E001;    // E0 = custom exception, E5E = ESE, XXX = error code

//  ================================================================
class JetTestEnforceSEHException
//  ================================================================
//
//  Jet unit tests throw an SEH exception with the following
//  information to react to code enforces.
//-
{
    static const int cchMax = 128;
    static thread_local JetTestEnforceSEHException* s_pThreadExcep;  // currently thrown exception on the current thread

public:
    WCHAR   wszContext[ cchMax ];
    CHAR    szMessage[ cchMax ];
    WCHAR   wszIssueSource[ cchMax ];

    // Filter function to catch JetTestEnforceSEHException.
    static DWORD Filter( _EXCEPTION_POINTERS* lpExcepPtrs );

    // Call from within an __except handler to cleanup memory allocated for the exception object.
    static void Cleanup();

    static JetTestEnforceSEHException* GetException()
    {
        Assert( s_pThreadExcep != NULL );
        return s_pThreadExcep;
    }
};


//** JETUNITTEST *****************************************************

#define JETUNITTEST(component,test) \
class Test##component##test : public JetSimpleUnitTest                  \
{                                                                       \
protected:                                                              \
    void Run_();                                                        \
private:                                                                \
    Test##component##test() : JetSimpleUnitTest(#component "." #test) {}\
    static Test##component##test s_instance;                            \
};                                                                      \
Test##component##test Test##component##test::s_instance;                \
void Test##component##test::Run_()

// facilities is something like dwBufferManager
#define JETUNITTESTEX(component,test,facilities) \
class Test##component##test : public JetSimpleUnitTest                  \
{                                                                       \
protected:                                                              \
    void Run_();                                                        \
private:                                                                \
    Test##component##test() : JetSimpleUnitTest(#component "." #test, facilities) {}\
    static Test##component##test s_instance;                            \
};                                                                      \
Test##component##test Test##component##test::s_instance;                \
void Test##component##test::Run_()



#define JETUNITTESTDB(component,test,facilities) \
class Test##component##test : public JetSimpleDbUnitTest                \
{                                                                       \
protected:                                                              \
    void Run_();                                                        \
private:                                                                \
    Test##component##test() : JetSimpleDbUnitTest(#component "." #test, facilities) {}\
    static Test##component##test s_instance;                            \
};                                                                      \
Test##component##test Test##component##test::s_instance;                \
void Test##component##test::Run_()

//** CHECK   *********************************************************

#define FAIL(_reason)          \
    Fail_(__FILE__, __LINE__, _reason)

#define CHECK(_condition)       \
    if (!(_condition))          \
    {                           \
        FAIL(#_condition);      \
    }

#define CHECKCALLS( _err )          \
    if ( _err != JET_errSuccess )   \
    {                               \
        wprintf( L"Last throw: %hs : %d err = %d\n", PefLastThrow()->SzFile(), PefLastThrow()->UlLine(), PefLastThrow()->Err() );   \
        FAIL( #_err " Failed with above throw site." );     \
    }

#else // ENABLE_JET_UNIT_TEST

// When the Unit Tests are disabled, then the following macros should do nothing
// The JETUNITTEST-* macros will still evaluate to functions and be compiled, but
// the linker will optimize them away.
#define FAIL(_reason)
#define CHECK(_condition)
#define CHECKCALLS( _err )

#define JETUNITTEST(component,test)         VOID DisabledTest##component##test( VOID )
#define JETUNITTESTEX(component,test,facilities)    VOID DisabledTest##component##test( VOID )
#define JETUNITTESTDB(component,test,facilities)    VOID DisabledTest##component##test( const IFMP ifmpTest, const WCHAR * const wszTable, JET_SESID * const psesid, JET_DBID * const pdbid, JET_TABLEID * pcursor )
#define IfmpTest()                  ((IFMP)ifmpNil)

#endif // !ENABLE_JET_UNIT_TEST

#endif // JETTEST_HXX_INCLUDED

