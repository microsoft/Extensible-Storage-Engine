// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

CUnitTest( CprintBufBasicAppend, 0, "Tests that OS Layer CPRINTFBUF can append data." );
ERR CprintBufBasicAppend::ErrTest()
{
    ERR err = JET_errSuccess;

    CPRINTINTRINBUF prtbuf;

    const CHAR * szTest1 = "ThisIsText.";
    const CHAR * szTest2 = "TestAsWell.";

    prtbuf( szTest1 );
    prtbuf( szTest2 );

    CPRINTINTRINBUF::BUFCURSOR csr( &prtbuf );
    const CHAR * szT;
    while( ( szT = csr.SzNext() ) != NULL )
    {
        printf( "SzNext: %hs\n", szT );
        OSTestCheck( !!_stricmp( szTest1, szT ) != !!_stricmp( szTest2, szT ) );
        OSTestCheck( _stricmp( szTest1, szT ) == 0 || _stricmp( szTest2, szT ) == 0 );
    }

    OSTestCheck( fTrue );

HandleError:

    return err;
}

CUnitTest( CprintBufFContainsTest, 0, "Tests that OS Layer CPRINTFBUF can append data." );
ERR CprintBufFContainsTest::ErrTest()
{
    ERR err = JET_errSuccess;

    CPRINTINTRINBUF prtbuf;

    CHAR * szTest1 = "ThisIsText.";
    CHAR * szTest2 = "TestAsWell.";
    CHAR * szTest3 = "LastText";

    prtbuf( szTest1 );
    prtbuf( szTest2 );
    prtbuf( szTest3 );

    OSTestCheck( prtbuf.FContains( szTest2 ) );
    OSTestCheck( prtbuf.FContains( szTest1 ) );
    OSTestCheck( !prtbuf.FContains( "NotText" ) );
    OSTestCheck( prtbuf.FContains( szTest3 ) );

HandleError:

    return err;
}

CUnitTest( CprintBufCContainsCountsProperlyTest, 0, "Tests that OS Layer CPRINTFBUF can append data and it is counted correctly." );
ERR CprintBufCContainsCountsProperlyTest::ErrTest()
{
    ERR err = JET_errSuccess;

    CPRINTINTRINBUF prtbuf;

    CHAR * szTest1 = "ThisIsText.";
    CHAR * szTest2 = "TestAsWell.";
    CHAR * szTest3 = "LastText";

    prtbuf( szTest1 );
    prtbuf( szTest2 );
    prtbuf( szTest3 );
    prtbuf( szTest2 );
    prtbuf( szTest3 );
    prtbuf( szTest3 );
    prtbuf( szTest1 );
    prtbuf( "LastTextLastTextLastText"  );
    prtbuf( szTest2 );

    OSTestCheck( 2 == prtbuf.CContains( szTest1 ) );
    OSTestCheck( 3 == prtbuf.CContains( szTest2 ) );
    OSTestCheck( 4 == prtbuf.CContains( szTest3 ) );

HandleError:

    return err;
}


CUnitTest( CprintBufCContainsDoubleCountsProperlyTest, 0, "Tests that OS Layer CPRINTFBUF can append data and it is counted correctly." );
ERR CprintBufCContainsDoubleCountsProperlyTest::ErrTest()
{
    ERR err = JET_errSuccess;

    CPRINTINTRINBUF prtbuf;

    CHAR * szTest1 = "ThisIsSomeText";
    CHAR * szTest2 = "TextAsWell.";
    CHAR * szTest3 = "LastText";

    prtbuf( szTest1 );
    prtbuf( szTest2 );
    prtbuf( szTest3 );

    prtbuf( "LastTextLastText"  );
    prtbuf( "ThisIsSomeTextAsWell." );

    OSTestCheck( 2 == prtbuf.CContains( szTest1 ) );
    OSTestCheck( 2 == prtbuf.CContains( szTest2 ) );
    OSTestCheck( 2 == prtbuf.CContains( szTest3 ) );

HandleError:

    return err;
}

CUnitTest( CprintBufSecondaryPrintsProperly, 0, "Tests that OS Layer CPRINTFBUF can collect and then print the buffers again." );
ERR CprintBufSecondaryPrintsProperly::ErrTest()
{
    ERR err = JET_errSuccess;

    CPRINTINTRINBUF prtbuf;

    CHAR * szTest1 = "ThisIsText.";
    CHAR * szTest2 = "TestAsWell.";
    CHAR * szTest3 = "LastText";

    prtbuf( szTest1 );
    prtbuf( szTest2 );
    prtbuf( szTest3 );


    prtbuf.Print( *CPRINTFSTDOUT::PcprintfInstance() );

    CPRINTINTRINBUF prtbuf2;

    prtbuf.Print( prtbuf2 );

    OSTestCheck( prtbuf.FContains( szTest2 ) );
    OSTestCheck( prtbuf.FContains( szTest1 ) );
    OSTestCheck( !prtbuf.FContains( "NotText" ) );
    OSTestCheck( prtbuf.FContains( szTest3 ) );
    
HandleError:

    return err;
}


