// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "collectionunittest.hxx"

//  ================================================================
class RRNextPrevTest : public UNITTEST
//  ================================================================
{
    private:
        static RRNextPrevTest s_instance;

    protected:
        RRNextPrevTest() {}
    public:
        ~RRNextPrevTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

RRNextPrevTest RRNextPrevTest::s_instance;

const char * RRNextPrevTest::SzName() const         { return "Round-Robin Next/Prev Test"; };
const char * RRNextPrevTest::SzDescription() const  { return "Test Raw Round-Robin Next/Prev helper functions."; }
bool RRNextPrevTest::FRunUnderESE98() const         { return true; }
bool RRNextPrevTest::FRunUnderESENT() const         { return true; }
bool RRNextPrevTest::FRunUnderESE97() const         { return true; }

//  ================================================================
ERR RRNextPrevTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;

    wprintf( L"\tTesting round-robin next/prev helper functions ...\n");

    const DWORD cbFake1 = 3;
    const DWORD cbFake2 = 5;

    //  some explicit tests

    TestCheck( IrrNext( 0, cbFake2 ) == 1 );
    TestCheck( IrrNext( 2, cbFake2 ) == 3 );
    TestCheck( IrrNext( 4, cbFake2 ) == 0 );

    //  defines to explore full gamut

    #define TestIrrNextAllTypes( input, rrsize, result )                    \
        TestCheck( IrrNext( input, rrsize ) == result );                        \
        TestCheck( IrrNext( (__int64)input, (__int64)rrsize ) == result );      \
        TestCheck( IrrNext( (QWORD)input, (QWORD)rrsize ) == result );          \
        TestCheck( IrrNext( (INT)input, (INT)rrsize ) == result );              \
        TestCheck( IrrNext( (DWORD)input, (DWORD)rrsize ) == result );          \
        TestCheck( IrrNext( (SHORT)input, (SHORT)rrsize ) == result );          \
        TestCheck( IrrNext( (USHORT)input, (USHORT)rrsize ) == result );        \
        TestCheck( IrrNext( input, (size_t)rrsize ) == result );                \
        TestCheck( IrrNext( (__int64)input, (size_t)rrsize ) == result );       \
        TestCheck( IrrNext( (QWORD)input, (size_t)rrsize ) == result );         \
        TestCheck( IrrNext( (INT)input, (size_t)rrsize ) == result );           \
        TestCheck( IrrNext( (DWORD)input, (size_t)rrsize ) == result );         \
        TestCheck( IrrNext( (SHORT)input, (size_t)rrsize ) == result );         \
        TestCheck( IrrNext( (USHORT)input, (size_t)rrsize ) == result );

    #define TestIrrNextOnArray( input, result )                         \
        TestCheck( IrrNext( input, cbFake1 ) == result );               \
        TestCheck( IrrNext( (__int64)input, cbFake1 ) == result );  \
        TestCheck( IrrNext( (QWORD)input, cbFake1 ) == result );        \
        TestCheck( IrrNext( (INT)input, cbFake1 ) == result );      \
        TestCheck( IrrNext( (DWORD)input, cbFake1 ) == result );        \
        TestCheck( IrrNext( (SHORT)input, cbFake1 ) == result );        \
        TestCheck( IrrNext( (USHORT)input, cbFake1 ) == result );

    #define TestIrrPrevAllTypes( input, rrsize, result )                    \
        TestCheck( IrrPrev( input, rrsize ) == result );                        \
        TestCheck( IrrPrev( (__int64)input, (__int64)rrsize ) == result );      \
        TestCheck( IrrPrev( (QWORD)input, (QWORD)rrsize ) == result );          \
        TestCheck( IrrPrev( (INT)input, (INT)rrsize ) == result );              \
        TestCheck( IrrPrev( (DWORD)input, (DWORD)rrsize ) == result );          \
        TestCheck( IrrPrev( (SHORT)input, (SHORT)rrsize ) == result );          \
        TestCheck( IrrPrev( (USHORT)input, (USHORT)rrsize ) == result );        \
        TestCheck( IrrPrev( input, (size_t)rrsize ) == result );                \
        TestCheck( IrrPrev( (__int64)input, (size_t)rrsize ) == result );       \
        TestCheck( IrrPrev( (QWORD)input, (size_t)rrsize ) == result );         \
        TestCheck( IrrPrev( (INT)input, (size_t)rrsize ) == result );           \
        TestCheck( IrrPrev( (DWORD)input, (size_t)rrsize ) == result );         \
        TestCheck( IrrPrev( (SHORT)input, (size_t)rrsize ) == result );         \
        TestCheck( IrrPrev( (USHORT)input, (size_t)rrsize ) == result );

    #define TestIrrPrevOnArray( input, result )                         \
        TestCheck( IrrPrev( input, cbFake1 ) == result );               \
        TestCheck( IrrPrev( (__int64)input, cbFake1 ) == result );  \
        TestCheck( IrrPrev( (QWORD)input, cbFake1 ) == result );        \
        TestCheck( IrrPrev( (INT)input, cbFake1 ) == result );      \
        TestCheck( IrrPrev( (DWORD)input, cbFake1 ) == result );        \
        TestCheck( IrrPrev( (SHORT)input, cbFake1 ) == result );        \
        TestCheck( IrrPrev( (USHORT)input, cbFake1 ) == result );


    TestIrrNextOnArray( 0, 1 );
    TestIrrNextAllTypes( 0, 3, 1 );
    TestIrrNextOnArray( 1, 2 );
    TestIrrNextAllTypes( 1, 3, 2 );
    TestIrrNextOnArray( 2, 0 );
    TestIrrNextAllTypes( 2, 3, 0 );

    TestIrrPrevOnArray( 0, 2 );
    TestIrrPrevAllTypes( 0, 3, 2 );
    TestIrrPrevOnArray( 1, 0 );
    TestIrrPrevAllTypes( 1, 3, 0 );
    TestIrrPrevOnArray( 2, 1 );
    TestIrrPrevAllTypes( 2, 3, 1 );

HandleError:

    return err;
}


