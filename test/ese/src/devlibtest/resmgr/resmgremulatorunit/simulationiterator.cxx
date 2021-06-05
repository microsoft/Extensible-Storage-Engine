// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "resmgremulatorunit.hxx"

// Unit test class

//  ================================================================
class FixedCacheSimulationIteratorTest : public UNITTEST
//  ================================================================
{
    private:
        static FixedCacheSimulationIteratorTest s_instance;

    protected:
        FixedCacheSimulationIteratorTest() {}
    public:
        ~FixedCacheSimulationIteratorTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();

    private:
        ERR ErrEmpty_();
        ERR ErrSingleSample_();
        ERR ErrConstantFunction_();
        ERR ErrIncreasingFunction_();
        ERR ErrDecreasingFunction_();
};

FixedCacheSimulationIteratorTest FixedCacheSimulationIteratorTest::s_instance;

const char * FixedCacheSimulationIteratorTest::SzName() const           { return "FixedCacheSimulationIteratorTest"; };
const char * FixedCacheSimulationIteratorTest::SzDescription() const    { return "Tests for FixedCacheSimulationIterator."; }
bool FixedCacheSimulationIteratorTest::FRunUnderESE98() const           { return true; }
bool FixedCacheSimulationIteratorTest::FRunUnderESENT() const           { return true; }
bool FixedCacheSimulationIteratorTest::FRunUnderESE97() const           { return true; }


//  ================================================================
ERR FixedCacheSimulationIteratorTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;
    
    TestCall( ErrEmpty_() );
    TestCall( ErrSingleSample_() );
    TestCall( ErrConstantFunction_() );
    TestCall( ErrIncreasingFunction_() );
    TestCall( ErrDecreasingFunction_() );

HandleError:
    return err;
}

//  ================================================================
ERR FixedCacheSimulationIteratorTest::ErrEmpty_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    FixedCacheSimulationIterator iter;
    DWORD cbfCache;
    QWORD cFaults;

    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( !iter.FGetSimulationSample( 0, &cbfCache, &cFaults ) );
    TestCheck( !iter.FGetSimulationSample( 1, &cbfCache, &cFaults ) );

HandleError:

    return err;
}

//  ================================================================
ERR FixedCacheSimulationIteratorTest::ErrSingleSample_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    FixedCacheSimulationIterator iter;
    DWORD cbfCache;
    QWORD cFaults;

    TestCheck( !iter.FSimulationSampleExists( 1 ) );
    TestCheck( !iter.FSimulationSampleExists( 2 ) );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );
    iter.AddSimulationSample( 1, 2 );
    TestCheck( iter.FSimulationSampleExists( 1 ) );
    TestCheck( !iter.FSimulationSampleExists( 2 ) );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( iter.FGetSimulationSample( 0, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 1 );
    TestCheck( cFaults == 2 );

    TestCheck( !iter.FGetSimulationSample( 1, &cbfCache, &cFaults ) );
    TestCheck( !iter.FGetSimulationSample( 2, &cbfCache, &cFaults ) );

HandleError:

    return err;
}

//  ================================================================
ERR FixedCacheSimulationIteratorTest::ErrConstantFunction_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    FixedCacheSimulationIterator iter;
    DWORD cbfCache;
    QWORD cFaults;

    iter.AddSimulationSample( 10, 10 );
    iter.AddSimulationSample( 5, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 7 );

    iter.AddSimulationSample( 7, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 8 );

    iter.AddSimulationSample( 8, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 6 );

    iter.AddSimulationSample( 6, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 9 );

    iter.AddSimulationSample( 9, 10 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    iter.AddSimulationSample( 1, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 3 );

    iter.AddSimulationSample( 3, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 2 );

    iter.AddSimulationSample( 2, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 4 );

    iter.AddSimulationSample( 4, 10 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( iter.FGetSimulationSample( 0, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 1 );
    TestCheck( cFaults == 10 );
    TestCheck( iter.FGetSimulationSample( 1, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 2 );
    TestCheck( cFaults == 10 );
    TestCheck( iter.FGetSimulationSample( 2, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 3 );
    TestCheck( cFaults == 10 );
    TestCheck( iter.FGetSimulationSample( 3, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 4 );
    TestCheck( cFaults == 10 );
    TestCheck( iter.FGetSimulationSample( 4, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 5 );
    TestCheck( cFaults == 10 );
    TestCheck( iter.FGetSimulationSample( 5, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 6 );
    TestCheck( cFaults == 10 );
    TestCheck( iter.FGetSimulationSample( 6, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 7 );
    TestCheck( cFaults == 10 );
    TestCheck( iter.FGetSimulationSample( 7, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 8 );
    TestCheck( cFaults == 10 );
    TestCheck( iter.FGetSimulationSample( 8, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 9 );
    TestCheck( cFaults == 10 );
    TestCheck( iter.FGetSimulationSample( 9, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 10 );
    TestCheck( cFaults == 10 );

    TestCheck( !iter.FGetSimulationSample( 10, &cbfCache, &cFaults ) );
    TestCheck( !iter.FGetSimulationSample( 11, &cbfCache, &cFaults ) );

HandleError:

    return err;
}

//  ================================================================
ERR FixedCacheSimulationIteratorTest::ErrIncreasingFunction_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    FixedCacheSimulationIterator iter;
    DWORD cbfCache;
    QWORD cFaults;

    iter.AddSimulationSample( 1, 1 );
    iter.AddSimulationSample( 10, 15 );
    TestCheck( iter.DwGetNextIterationValue() == 5 );

    iter.AddSimulationSample( 5, 5 );
    TestCheck( iter.DwGetNextIterationValue() == 7 );

    iter.AddSimulationSample( 7, 9 );
    TestCheck( iter.DwGetNextIterationValue() == 8 );

    iter.AddSimulationSample( 8, 11 );
    TestCheck( iter.DwGetNextIterationValue() == 3 );

    iter.AddSimulationSample( 3, 3 );
    TestCheck( iter.DwGetNextIterationValue() == 6 );

    iter.AddSimulationSample( 6, 7 );
    TestCheck( iter.DwGetNextIterationValue() == 9 );

    iter.AddSimulationSample( 9, 13 );
    TestCheck( iter.DwGetNextIterationValue() == 2 );

    iter.AddSimulationSample( 2, 2 );
    TestCheck( iter.DwGetNextIterationValue() == 4 );

    iter.AddSimulationSample( 4, 4 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( iter.FGetSimulationSample( 0, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 1 );
    TestCheck( cFaults == 1 );
    TestCheck( iter.FGetSimulationSample( 1, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 2 );
    TestCheck( cFaults == 2 );
    TestCheck( iter.FGetSimulationSample( 2, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 3 );
    TestCheck( cFaults == 3 );
    TestCheck( iter.FGetSimulationSample( 3, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 4 );
    TestCheck( cFaults == 4 );
    TestCheck( iter.FGetSimulationSample( 4, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 5 );
    TestCheck( cFaults == 5 );
    TestCheck( iter.FGetSimulationSample( 5, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 6 );
    TestCheck( cFaults == 7 );
    TestCheck( iter.FGetSimulationSample( 6, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 7 );
    TestCheck( cFaults == 9 );
    TestCheck( iter.FGetSimulationSample( 7, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 8 );
    TestCheck( cFaults == 11 );
    TestCheck( iter.FGetSimulationSample( 8, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 9 );
    TestCheck( cFaults == 13 );
    TestCheck( iter.FGetSimulationSample( 9, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 10 );
    TestCheck( cFaults == 15 );

    TestCheck( !iter.FGetSimulationSample( 10, &cbfCache, &cFaults ) );
    TestCheck( !iter.FGetSimulationSample( 11, &cbfCache, &cFaults ) );

HandleError:

    return err;
}

//  ================================================================
ERR FixedCacheSimulationIteratorTest::ErrDecreasingFunction_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    FixedCacheSimulationIterator iter;
    DWORD cbfCache;
    QWORD cFaults;

    iter.AddSimulationSample( 1, 15 );
    iter.AddSimulationSample( 10, 1 );
    TestCheck( iter.DwGetNextIterationValue() == 5 );

    iter.AddSimulationSample( 5, 7 );
    TestCheck( iter.DwGetNextIterationValue() == 3 );

    iter.AddSimulationSample( 3, 11 );
    TestCheck( iter.DwGetNextIterationValue() == 7 );

    iter.AddSimulationSample( 7, 5 );
    TestCheck( iter.DwGetNextIterationValue() == 8 );

    iter.AddSimulationSample( 8, 4 );
    TestCheck( iter.DwGetNextIterationValue() == 2 );

    iter.AddSimulationSample( 2, 13 );
    TestCheck( iter.DwGetNextIterationValue() == 4 );

    iter.AddSimulationSample( 4, 9 );
    TestCheck( iter.DwGetNextIterationValue() == 9 );

    iter.AddSimulationSample( 9, 2 );
    TestCheck( iter.DwGetNextIterationValue() == 6 );

    iter.AddSimulationSample( 6, 6 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( iter.FGetSimulationSample( 0, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 1 );
    TestCheck( cFaults == 15 );
    TestCheck( iter.FGetSimulationSample( 1, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 2 );
    TestCheck( cFaults == 13 );
    TestCheck( iter.FGetSimulationSample( 2, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 3 );
    TestCheck( cFaults == 11 );
    TestCheck( iter.FGetSimulationSample( 3, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 4 );
    TestCheck( cFaults == 9 );
    TestCheck( iter.FGetSimulationSample( 4, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 5 );
    TestCheck( cFaults == 7 );
    TestCheck( iter.FGetSimulationSample( 5, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 6 );
    TestCheck( cFaults == 6 );
    TestCheck( iter.FGetSimulationSample( 6, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 7 );
    TestCheck( cFaults == 5 );
    TestCheck( iter.FGetSimulationSample( 7, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 8 );
    TestCheck( cFaults == 4 );
    TestCheck( iter.FGetSimulationSample( 8, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 9 );
    TestCheck( cFaults == 2 );
    TestCheck( iter.FGetSimulationSample( 9, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 10 );
    TestCheck( cFaults == 1 );

    TestCheck( !iter.FGetSimulationSample( 10, &cbfCache, &cFaults ) );
    TestCheck( !iter.FGetSimulationSample( 11, &cbfCache, &cFaults ) );

HandleError:

    return err;
}


//  ================================================================
class CacheFaultLookupSimulationIteratorTest : public UNITTEST
//  ================================================================
{
    private:
        static CacheFaultLookupSimulationIteratorTest s_instance;

    protected:
        CacheFaultLookupSimulationIteratorTest() {}
    public:
        ~CacheFaultLookupSimulationIteratorTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();

    private:
        ERR ErrEmpty_();
        ERR ErrSingleSample_();
        ERR ErrConstantFunction_();
        ERR ErrIncreasingFunction_();
        ERR ErrDecreasingFunction_();
};

CacheFaultLookupSimulationIteratorTest CacheFaultLookupSimulationIteratorTest::s_instance;

const char * CacheFaultLookupSimulationIteratorTest::SzName() const         { return "CacheFaultLookupSimulationIteratorTest"; };
const char * CacheFaultLookupSimulationIteratorTest::SzDescription() const  { return "Tests for CacheFaultLookupSimulationIterator."; }
bool CacheFaultLookupSimulationIteratorTest::FRunUnderESE98() const         { return true; }
bool CacheFaultLookupSimulationIteratorTest::FRunUnderESENT() const         { return true; }
bool CacheFaultLookupSimulationIteratorTest::FRunUnderESE97() const         { return true; }


//  ================================================================
ERR CacheFaultLookupSimulationIteratorTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;
    
    TestCall( ErrEmpty_() );
    TestCall( ErrSingleSample_() );
    TestCall( ErrConstantFunction_() );
    TestCall( ErrIncreasingFunction_() );
    TestCall( ErrDecreasingFunction_() );

HandleError:
    return err;
}

//  ================================================================
ERR CacheFaultLookupSimulationIteratorTest::ErrEmpty_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CacheFaultLookupSimulationIterator iter( 2 );
    DWORD cbfCache;
    QWORD cFaults;

    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( !iter.FGetSimulationSample( 0, &cbfCache, &cFaults ) );
    TestCheck( !iter.FGetSimulationSample( 1, &cbfCache, &cFaults ) );

HandleError:

    return err;
}

//  ================================================================
ERR CacheFaultLookupSimulationIteratorTest::ErrSingleSample_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CacheFaultLookupSimulationIterator iter( 2 );
    DWORD cbfCache;
    QWORD cFaults;

    TestCheck( !iter.FSimulationSampleExists( 1 ) );
    TestCheck( !iter.FSimulationSampleExists( 2 ) );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );
    iter.AddSimulationSample( 1, 2 );
    TestCheck( iter.FSimulationSampleExists( 1 ) );
    TestCheck( !iter.FSimulationSampleExists( 2 ) );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( iter.FGetSimulationSample( 0, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 1 );
    TestCheck( cFaults == 2 );

    TestCheck( !iter.FGetSimulationSample( 1, &cbfCache, &cFaults ) );
    TestCheck( !iter.FGetSimulationSample( 2, &cbfCache, &cFaults ) );

HandleError:

    return err;
}

//  ================================================================
ERR CacheFaultLookupSimulationIteratorTest::ErrConstantFunction_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CacheFaultLookupSimulationIterator iter( 10 );
    DWORD cbfCache;
    QWORD cFaults;

    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    iter.AddSimulationSample( 2, 10 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );
    
    iter.AddSimulationSample( 5, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 2 );

    iter.AddSimulationSample( 3, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 2 );

    TestCheck( iter.FGetSimulationSample( 0, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 2 );
    TestCheck( cFaults == 10 );
    TestCheck( iter.FGetSimulationSample( 1, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 3 );
    TestCheck( cFaults == 10 );
    TestCheck( iter.FGetSimulationSample( 2, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 5 );
    TestCheck( cFaults == 10 );

    TestCheck( !iter.FGetSimulationSample( 3, &cbfCache, &cFaults ) );
    TestCheck( !iter.FGetSimulationSample( 4, &cbfCache, &cFaults ) );

HandleError:

    return err;
}

//  ================================================================
ERR CacheFaultLookupSimulationIteratorTest::ErrIncreasingFunction_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CacheFaultLookupSimulationIterator iter( 10 );
    DWORD cbfCache;
    QWORD cFaults;

    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    iter.AddSimulationSample( 100, 20 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );
    
    iter.AddSimulationSample( 200, 40 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    iter.AddSimulationSample( 300, 60 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    iter.AddSimulationSample( 20, 4 );
    TestCheck( iter.DwGetNextIterationValue() == 60 );

    iter.AddSimulationSample( 60, 12 );
    TestCheck( iter.DwGetNextIterationValue() == 40 );

    iter.AddSimulationSample( 50, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 50 );

    TestCheck( iter.FGetSimulationSample( 0, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 20 );
    TestCheck( cFaults == 4 );
    TestCheck( iter.FGetSimulationSample( 1, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 50 );
    TestCheck( cFaults == 10 );
    TestCheck( iter.FGetSimulationSample( 2, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 60 );
    TestCheck( cFaults == 12 );
    TestCheck( iter.FGetSimulationSample( 3, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 100 );
    TestCheck( cFaults == 20 );
    TestCheck( iter.FGetSimulationSample( 4, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 200 );
    TestCheck( cFaults == 40 );
    TestCheck( iter.FGetSimulationSample( 5, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 300 );
    TestCheck( cFaults == 60 );

    TestCheck( !iter.FGetSimulationSample( 7, &cbfCache, &cFaults ) );
    TestCheck( !iter.FGetSimulationSample( 8, &cbfCache, &cFaults ) );

HandleError:

    return err;
}

//  ================================================================
ERR CacheFaultLookupSimulationIteratorTest::ErrDecreasingFunction_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CacheFaultLookupSimulationIterator iter( 100 );
    DWORD cbfCache;
    QWORD cFaults;

    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    iter.AddSimulationSample( 100, 10 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );
    
    iter.AddSimulationSample( 50, 60 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    iter.AddSimulationSample( 70, 40 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    iter.AddSimulationSample( 4, 104 );
    TestCheck( iter.DwGetNextIterationValue() == 27 );

    iter.AddSimulationSample( 27, 83 );
    TestCheck( iter.DwGetNextIterationValue() == 15 );

    iter.AddSimulationSample( 1, 100 );
    TestCheck( iter.DwGetNextIterationValue() == 1 );

    TestCheck( iter.FGetSimulationSample( 0, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 1 );
    TestCheck( cFaults == 100 );
    TestCheck( iter.FGetSimulationSample( 1, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 4 );
    TestCheck( cFaults == 104 );
    TestCheck( iter.FGetSimulationSample( 2, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 27 );
    TestCheck( cFaults == 83 );
    TestCheck( iter.FGetSimulationSample( 3, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 50 );
    TestCheck( cFaults == 60 );
    TestCheck( iter.FGetSimulationSample( 4, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 70 );
    TestCheck( cFaults == 40 );
    TestCheck( iter.FGetSimulationSample( 5, &cbfCache, &cFaults ) );
    TestCheck( cbfCache == 100 );
    TestCheck( cFaults == 10 );

    TestCheck( !iter.FGetSimulationSample( 7, &cbfCache, &cFaults ) );
    TestCheck( !iter.FGetSimulationSample( 8, &cbfCache, &cFaults ) );

HandleError:

    return err;
}


//  ================================================================
class CacheFaultMinSimulationIteratorTest : public UNITTEST
//  ================================================================
{
    private:
        static CacheFaultMinSimulationIteratorTest s_instance;

    protected:
        CacheFaultMinSimulationIteratorTest() {}
    public:
        ~CacheFaultMinSimulationIteratorTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();

    private:
        ERR ErrEmpty_();
        ERR ErrSingleSampleFirstZero_();
        ERR ErrSingleSampleFirstNonZero_();
        ERR ErrTwoSamplesFirstZero_();
        ERR ErrTwoSamplesFirstNonZero_();
        ERR ErrSampleDiffBelowThreshold_();
        ERR ErrMaxSamples_();
};

CacheFaultMinSimulationIteratorTest CacheFaultMinSimulationIteratorTest::s_instance;

const char * CacheFaultMinSimulationIteratorTest::SzName() const            { return "CacheFaultMinSimulationIteratorTest"; };
const char * CacheFaultMinSimulationIteratorTest::SzDescription() const { return "Tests for CacheFaultMinSimulationIterator."; }
bool CacheFaultMinSimulationIteratorTest::FRunUnderESE98() const            { return true; }
bool CacheFaultMinSimulationIteratorTest::FRunUnderESENT() const            { return true; }
bool CacheFaultMinSimulationIteratorTest::FRunUnderESE97() const            { return true; }


//  ================================================================
ERR CacheFaultMinSimulationIteratorTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;
    
    TestCall( ErrEmpty_() );
    TestCall( ErrSingleSampleFirstZero_() );
    TestCall( ErrSingleSampleFirstNonZero_() );
    TestCall( ErrTwoSamplesFirstZero_() );
    TestCall( ErrTwoSamplesFirstNonZero_() );
    TestCall( ErrSampleDiffBelowThreshold_() );
    TestCall( ErrMaxSamples_() );

HandleError:
    return err;
}

//  ================================================================
ERR CacheFaultMinSimulationIteratorTest::ErrEmpty_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CacheFaultMinSimulationIterator iter( 5, 32768 );
    DWORD dtick;
    QWORD cFaults;

    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( !iter.FGetSimulationSample( 0, &dtick, &cFaults ) );
    TestCheck( !iter.FGetSimulationSample( 1, &dtick, &cFaults ) );

HandleError:

    return err;
}

//  ================================================================
ERR CacheFaultMinSimulationIteratorTest::ErrSingleSampleFirstZero_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CacheFaultMinSimulationIterator iter( 5, 32768 );
    DWORD dtick;
    QWORD cFaults;

    iter.AddSimulationSample( 0, 10 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( iter.FGetSimulationSample( 0, &dtick, &cFaults ) );
    TestCheck( dtick == 0 );
    TestCheck( cFaults == 10 );

    TestCheck( !iter.FGetSimulationSample( 1, &dtick, &cFaults ) );

HandleError:

    return err;
}

//  ================================================================
ERR CacheFaultMinSimulationIteratorTest::ErrSingleSampleFirstNonZero_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CacheFaultMinSimulationIterator iter( 5, 32768 );
    DWORD dtick;
    QWORD cFaults;

    iter.AddSimulationSample( 8, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 64 );

    TestCheck( iter.FGetSimulationSample( 0, &dtick, &cFaults ) );
    TestCheck( dtick == 8 );
    TestCheck( cFaults == 10 );

    TestCheck( !iter.FGetSimulationSample( 1, &dtick, &cFaults ) );

HandleError:

    return err;
}

//  ================================================================
ERR CacheFaultMinSimulationIteratorTest::ErrTwoSamplesFirstZero_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CacheFaultMinSimulationIterator iter( 5, 32768 );
    DWORD dtick;
    QWORD cFaults;

    iter.AddSimulationSample( 0, 10 );
    iter.AddSimulationSample( 64, 20 );
    TestCheck( iter.DwGetNextIterationValue() == 512 );

    TestCheck( iter.FGetSimulationSample( 0, &dtick, &cFaults ) );
    TestCheck( dtick == 0 );
    TestCheck( cFaults == 10 );

    TestCheck( iter.FGetSimulationSample( 1, &dtick, &cFaults ) );
    TestCheck( dtick == 64 );
    TestCheck( cFaults == 20 );

    TestCheck( !iter.FGetSimulationSample( 2, &dtick, &cFaults ) );

HandleError:

    return err;
}

//  ================================================================
ERR CacheFaultMinSimulationIteratorTest::ErrTwoSamplesFirstNonZero_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CacheFaultMinSimulationIterator iter( 5, 32768 );
    DWORD dtick;
    QWORD cFaults;

    iter.AddSimulationSample( 8, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 64 );
    
    iter.AddSimulationSample( 64, 20 );
    TestCheck( iter.DwGetNextIterationValue() == 512 );

    TestCheck( iter.FGetSimulationSample( 0, &dtick, &cFaults ) );
    TestCheck( dtick == 8 );
    TestCheck( cFaults == 10 );

    TestCheck( iter.FGetSimulationSample( 1, &dtick, &cFaults ) );
    TestCheck( dtick == 64 );
    TestCheck( cFaults == 20 );

    TestCheck( !iter.FGetSimulationSample( 2, &dtick, &cFaults ) );

HandleError:

    return err;
}

//  ================================================================
ERR CacheFaultMinSimulationIteratorTest::ErrSampleDiffBelowThreshold_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CacheFaultMinSimulationIterator iter( 5, 1024 );
    DWORD dtick;
    QWORD cFaults;

    iter.AddSimulationSample( 4, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 20 );
    
    iter.AddSimulationSample( 20, 20 );
    TestCheck( iter.DwGetNextIterationValue() == 80 );

    iter.AddSimulationSample( 80, 30 );
    TestCheck( iter.DwGetNextIterationValue() == 320 );

    iter.AddSimulationSample( 320, 40 );
    TestCheck( iter.DwGetNextIterationValue() == 1280 );

    iter.AddSimulationSample( 1280, 50 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( iter.FGetSimulationSample( 0, &dtick, &cFaults ) );
    TestCheck( dtick == 4 );
    TestCheck( cFaults == 10 );

    TestCheck( iter.FGetSimulationSample( 1, &dtick, &cFaults ) );
    TestCheck( dtick == 20 );
    TestCheck( cFaults == 20 );

    TestCheck( iter.FGetSimulationSample( 2, &dtick, &cFaults ) );
    TestCheck( dtick == 80 );
    TestCheck( cFaults == 30 );

    TestCheck( iter.FGetSimulationSample( 3, &dtick, &cFaults ) );
    TestCheck( dtick == 320 );
    TestCheck( cFaults == 40 );

    TestCheck( iter.FGetSimulationSample( 4, &dtick, &cFaults ) );
    TestCheck( dtick == 1280 );
    TestCheck( cFaults == 50 );

    TestCheck( !iter.FGetSimulationSample( 5, &dtick, &cFaults ) );

HandleError:

    return err;
}

//  ================================================================
ERR CacheFaultMinSimulationIteratorTest::ErrMaxSamples_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CacheFaultMinSimulationIterator iter( 5, 32768 );
    DWORD dtick;
    QWORD cFaults;

    iter.AddSimulationSample( 8, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 64 );
    
    iter.AddSimulationSample( 64, 20 );
    TestCheck( iter.DwGetNextIterationValue() == 512 );

    iter.AddSimulationSample( 512, 30 );
    TestCheck( iter.DwGetNextIterationValue() == 4096 );

    iter.AddSimulationSample( 4096, 40 );
    TestCheck( iter.DwGetNextIterationValue() == 32768 );

    iter.AddSimulationSample( 32768, 50 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( iter.FGetSimulationSample( 0, &dtick, &cFaults ) );
    TestCheck( dtick == 8 );
    TestCheck( cFaults == 10 );

    TestCheck( iter.FGetSimulationSample( 1, &dtick, &cFaults ) );
    TestCheck( dtick == 64 );
    TestCheck( cFaults == 20 );

    TestCheck( iter.FGetSimulationSample( 2, &dtick, &cFaults ) );
    TestCheck( dtick == 512 );
    TestCheck( cFaults == 30 );

    TestCheck( iter.FGetSimulationSample( 3, &dtick, &cFaults ) );
    TestCheck( dtick == 4096 );
    TestCheck( cFaults == 40 );

    TestCheck( iter.FGetSimulationSample( 4, &dtick, &cFaults ) );
    TestCheck( dtick == 32768 );
    TestCheck( cFaults == 50 );

    TestCheck( !iter.FGetSimulationSample( 5, &dtick, &cFaults ) );

HandleError:

    return err;
}


//  ================================================================
class ChkptDepthSimulationIteratorTest : public UNITTEST
//  ================================================================
{
    private:
        static ChkptDepthSimulationIteratorTest s_instance;

    protected:
        ChkptDepthSimulationIteratorTest() {}
    public:
        ~ChkptDepthSimulationIteratorTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();

    private:
        ERR ErrEmpty_();
        ERR ErrSingleSample_();
        ERR ErrConstantFunction_();
        ERR ErrIncreasingFunction_();
        ERR ErrDecreasingFunction_();
};

ChkptDepthSimulationIteratorTest ChkptDepthSimulationIteratorTest::s_instance;

const char * ChkptDepthSimulationIteratorTest::SzName() const           { return "ChkptDepthSimulationIteratorTest"; };
const char * ChkptDepthSimulationIteratorTest::SzDescription() const    { return "Tests for ChkptDepthSimulationIterator."; }
bool ChkptDepthSimulationIteratorTest::FRunUnderESE98() const           { return true; }
bool ChkptDepthSimulationIteratorTest::FRunUnderESENT() const           { return true; }
bool ChkptDepthSimulationIteratorTest::FRunUnderESE97() const           { return true; }


//  ================================================================
ERR ChkptDepthSimulationIteratorTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;
    
    TestCall( ErrEmpty_() );
    TestCall( ErrSingleSample_() );
    TestCall( ErrConstantFunction_() );
    TestCall( ErrIncreasingFunction_() );
    TestCall( ErrDecreasingFunction_() );

HandleError:
    return err;
}

//  ================================================================
ERR ChkptDepthSimulationIteratorTest::ErrEmpty_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    ChkptDepthSimulationIterator iter;
    DWORD clogsChkptDepth;
    QWORD cWrites;

    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( !iter.FGetSimulationSample( 0, &clogsChkptDepth, &cWrites ) );
    TestCheck( !iter.FGetSimulationSample( 1, &clogsChkptDepth, &cWrites ) );

HandleError:

    return err;
}

//  ================================================================
ERR ChkptDepthSimulationIteratorTest::ErrSingleSample_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    ChkptDepthSimulationIterator iter;
    DWORD clogsChkptDepth;
    QWORD cWrites;

    TestCheck( !iter.FSimulationSampleExists( 1 ) );
    TestCheck( !iter.FSimulationSampleExists( 2 ) );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );
    iter.AddSimulationSample( 1, 2 );
    TestCheck( iter.FSimulationSampleExists( 1 ) );
    TestCheck( !iter.FSimulationSampleExists( 2 ) );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( iter.FGetSimulationSample( 0, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 1 );
    TestCheck( cWrites == 2 );

    TestCheck( !iter.FGetSimulationSample( 1, &clogsChkptDepth, &cWrites ) );
    TestCheck( !iter.FGetSimulationSample( 2, &clogsChkptDepth, &cWrites ) );

HandleError:

    return err;
}

//  ================================================================
ERR ChkptDepthSimulationIteratorTest::ErrConstantFunction_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    ChkptDepthSimulationIterator iter;
    DWORD clogsChkptDepth;
    QWORD cWrites;

    iter.AddSimulationSample( 10, 10 );
    iter.AddSimulationSample( 5, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 7 );

    iter.AddSimulationSample( 7, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 8 );

    iter.AddSimulationSample( 8, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 6 );

    iter.AddSimulationSample( 6, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 9 );

    iter.AddSimulationSample( 9, 10 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    iter.AddSimulationSample( 1, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 3 );

    iter.AddSimulationSample( 3, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 2 );

    iter.AddSimulationSample( 2, 10 );
    TestCheck( iter.DwGetNextIterationValue() == 4 );

    iter.AddSimulationSample( 4, 10 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( iter.FGetSimulationSample( 0, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 1 );
    TestCheck( cWrites == 10 );
    TestCheck( iter.FGetSimulationSample( 1, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 2 );
    TestCheck( cWrites == 10 );
    TestCheck( iter.FGetSimulationSample( 2, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 3 );
    TestCheck( cWrites == 10 );
    TestCheck( iter.FGetSimulationSample( 3, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 4 );
    TestCheck( cWrites == 10 );
    TestCheck( iter.FGetSimulationSample( 4, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 5 );
    TestCheck( cWrites == 10 );
    TestCheck( iter.FGetSimulationSample( 5, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 6 );
    TestCheck( cWrites == 10 );
    TestCheck( iter.FGetSimulationSample( 6, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 7 );
    TestCheck( cWrites == 10 );
    TestCheck( iter.FGetSimulationSample( 7, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 8 );
    TestCheck( cWrites == 10 );
    TestCheck( iter.FGetSimulationSample( 8, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 9 );
    TestCheck( cWrites == 10 );
    TestCheck( iter.FGetSimulationSample( 9, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 10 );
    TestCheck( cWrites == 10 );

    TestCheck( !iter.FGetSimulationSample( 10, &clogsChkptDepth, &cWrites ) );
    TestCheck( !iter.FGetSimulationSample( 11, &clogsChkptDepth, &cWrites ) );

HandleError:

    return err;
}

//  ================================================================
ERR ChkptDepthSimulationIteratorTest::ErrIncreasingFunction_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    ChkptDepthSimulationIterator iter;
    DWORD clogsChkptDepth;
    QWORD cWrites;

    iter.AddSimulationSample( 1, 1 );
    iter.AddSimulationSample( 10, 15 );
    TestCheck( iter.DwGetNextIterationValue() == 5 );

    iter.AddSimulationSample( 5, 5 );
    TestCheck( iter.DwGetNextIterationValue() == 7 );

    iter.AddSimulationSample( 7, 9 );
    TestCheck( iter.DwGetNextIterationValue() == 8 );

    iter.AddSimulationSample( 8, 11 );
    TestCheck( iter.DwGetNextIterationValue() == 3 );

    iter.AddSimulationSample( 3, 3 );
    TestCheck( iter.DwGetNextIterationValue() == 6 );

    iter.AddSimulationSample( 6, 7 );
    TestCheck( iter.DwGetNextIterationValue() == 9 );

    iter.AddSimulationSample( 9, 13 );
    TestCheck( iter.DwGetNextIterationValue() == 2 );

    iter.AddSimulationSample( 2, 2 );
    TestCheck( iter.DwGetNextIterationValue() == 4 );

    iter.AddSimulationSample( 4, 4 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );

    TestCheck( iter.FGetSimulationSample( 0, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 1 );
    TestCheck( cWrites == 1 );
    TestCheck( iter.FGetSimulationSample( 1, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 2 );
    TestCheck( cWrites == 2 );
    TestCheck( iter.FGetSimulationSample( 2, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 3 );
    TestCheck( cWrites == 3 );
    TestCheck( iter.FGetSimulationSample( 3, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 4 );
    TestCheck( cWrites == 4 );
    TestCheck( iter.FGetSimulationSample( 4, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 5 );
    TestCheck( cWrites == 5 );
    TestCheck( iter.FGetSimulationSample( 5, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 6 );
    TestCheck( cWrites == 7 );
    TestCheck( iter.FGetSimulationSample( 6, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 7 );
    TestCheck( cWrites == 9 );
    TestCheck( iter.FGetSimulationSample( 7, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 8 );
    TestCheck( cWrites == 11 );
    TestCheck( iter.FGetSimulationSample( 8, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 9 );
    TestCheck( cWrites == 13 );
    TestCheck( iter.FGetSimulationSample( 9, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 10 );
    TestCheck( cWrites == 15 );

    TestCheck( !iter.FGetSimulationSample( 10, &clogsChkptDepth, &cWrites ) );
    TestCheck( !iter.FGetSimulationSample( 11, &clogsChkptDepth, &cWrites ) );

HandleError:

    return err;
}

//  ================================================================
ERR ChkptDepthSimulationIteratorTest::ErrDecreasingFunction_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    ChkptDepthSimulationIterator iter;
    DWORD clogsChkptDepth;
    QWORD cWrites;

    iter.AddSimulationSample( 1, 15 );
    iter.AddSimulationSample( 10, 1 );
    TestCheck( iter.DwGetNextIterationValue() == 5 );

    iter.AddSimulationSample( 5, 7 );
    TestCheck( iter.DwGetNextIterationValue() == 3 );

    iter.AddSimulationSample( 3, 11 );
    TestCheck( iter.DwGetNextIterationValue() == 7 );

    iter.AddSimulationSample( 7, 5 );
    TestCheck( iter.DwGetNextIterationValue() == 8 );

    iter.AddSimulationSample( 8, 4 );
    TestCheck( iter.DwGetNextIterationValue() == 2 );

    iter.AddSimulationSample( 2, 13 );
    TestCheck( iter.DwGetNextIterationValue() == 4 );

    iter.AddSimulationSample( 4, 9 );
    TestCheck( iter.DwGetNextIterationValue() == 9 );

    iter.AddSimulationSample( 9, 2 );
    TestCheck( iter.DwGetNextIterationValue() == 6 );

    iter.AddSimulationSample( 6, 6 );
    TestCheck( iter.DwGetNextIterationValue() == ulMax );   

    TestCheck( iter.FGetSimulationSample( 0, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 1 );
    TestCheck( cWrites == 15 );
    TestCheck( iter.FGetSimulationSample( 1, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 2 );
    TestCheck( cWrites == 13 );
    TestCheck( iter.FGetSimulationSample( 2, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 3 );
    TestCheck( cWrites == 11 );
    TestCheck( iter.FGetSimulationSample( 3, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 4 );
    TestCheck( cWrites == 9 );
    TestCheck( iter.FGetSimulationSample( 4, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 5 );
    TestCheck( cWrites == 7 );
    TestCheck( iter.FGetSimulationSample( 5, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 6 );
    TestCheck( cWrites == 6 );
    TestCheck( iter.FGetSimulationSample( 6, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 7 );
    TestCheck( cWrites == 5 );
    TestCheck( iter.FGetSimulationSample( 7, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 8 );
    TestCheck( cWrites == 4 );
    TestCheck( iter.FGetSimulationSample( 8, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 9 );
    TestCheck( cWrites == 2 );
    TestCheck( iter.FGetSimulationSample( 9, &clogsChkptDepth, &cWrites ) );
    TestCheck( clogsChkptDepth == 10 );
    TestCheck( cWrites == 1 );

    TestCheck( !iter.FGetSimulationSample( 10, &clogsChkptDepth, &cWrites ) );
    TestCheck( !iter.FGetSimulationSample( 11, &clogsChkptDepth, &cWrites ) );

HandleError:

    return err;
}

