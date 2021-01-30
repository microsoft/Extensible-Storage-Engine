// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifndef ENABLE_JET_UNIT_TEST
#error This file should only be compiled with the unit tests!
#endif

#undef g_cbPage
#define g_cbPage g_cbPage_CPAGE_NOT_ALLOWED_TO_USE_THIS_VARIABLE

inline INT CmpFormatVer( const GenVersion& ver1, const GenVersion& ver2 )
{
    return CmpFormatVer( &ver1, &ver2 );
}

JETUNITTEST( GenericVersions, GenericVersionsTestEqualCompare )
{
    GenVersion verCase1 = { 5, 3, 34221 };
    GenVersion verCase2 = { 5, 3, 34220 };

    CHECK( 0 != CmpFormatVer( verCase1, verCase2 ) );

    verCase2.ulVersionUpdateMinor = verCase1.ulVersionUpdateMinor;
    CHECK( 0 == CmpFormatVer( verCase1, verCase2 ) );

    verCase1.ulVersionUpdateMajor++;
    CHECK( 0 != CmpFormatVer( verCase1, verCase2 ) );
    verCase2.ulVersionUpdateMajor++;
    CHECK( 0 == CmpFormatVer( verCase1, verCase2 ) );

    verCase2.ulVersionMajor++;
    CHECK( 0 != CmpFormatVer( verCase1, verCase2 ) );
}

JETUNITTEST( GenericVersions, GenericVersionsTestGreaterThanCompare )
{
    GenVersion verCase1 = { 6, 10, 200 };
    GenVersion verCase2 = { 6, 10, 200 };

    CHECK( CmpFormatVer( verCase1, verCase2 ) == 0 );

    verCase1.ulVersionMajor++;
    CHECK( CmpFormatVer( verCase1, verCase2 ) > 0 );
    CHECK( FMajorVersionMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( !FUpdateMajorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( !FUpdateMinorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    verCase1.ulVersionMajor--; CHECK( CmpFormatVer( verCase1, verCase2 ) == 0 );

    verCase1.ulVersionUpdateMajor++;
    CHECK( CmpFormatVer( verCase1, verCase2 ) > 0 );
    CHECK( !FMajorVersionMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( FUpdateMajorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( !FUpdateMinorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    verCase1.ulVersionUpdateMajor--; CHECK( CmpFormatVer( verCase1, verCase2 ) == 0 );

    verCase1.ulVersionUpdateMinor++;
    CHECK( CmpFormatVer( verCase1, verCase2 ) > 0 );
    CHECK( !FMajorVersionMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( !FUpdateMajorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( FUpdateMinorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    verCase1.ulVersionUpdateMinor--; CHECK( CmpFormatVer( verCase1, verCase2 ) == 0 );


    GenVersion verCaseA = { 10, 5, 2 };
    GenVersion verCaseB = { 10, 5, 2 };

    CHECK( CmpFormatVer( verCaseA, verCaseB ) == 0 );

    verCaseA.ulVersionMajor++;
    CHECK( CmpFormatVer( verCaseA, verCaseB ) > 0 );
    verCaseA.ulVersionMajor--; CHECK( CmpFormatVer( verCaseA, verCaseB ) == 0 );

    verCaseA.ulVersionUpdateMajor++;
    CHECK( CmpFormatVer( verCaseA, verCaseB ) > 0 );
    verCaseA.ulVersionUpdateMajor--; CHECK( CmpFormatVer( verCaseA, verCaseB ) == 0 );

    verCaseA.ulVersionUpdateMinor++;
    CHECK( CmpFormatVer( verCaseA, verCaseB ) > 0 );
    verCaseA.ulVersionUpdateMinor--; CHECK( CmpFormatVer( verCaseA, verCaseB ) == 0 );
}

JETUNITTEST( GenericVersions, GenericVersionsTestLessThanCompare )
{
    GenVersion verCase1 = { 7, 20, 100 };
    GenVersion verCase2 = { 7, 20, 100 };

    CHECK( CmpFormatVer( verCase1, verCase2 ) == 0 );

    verCase1.ulVersionMajor--;
    CHECK( CmpFormatVer( verCase1, verCase2 ) < 0 );
    CHECK( FMajorVersionMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( !FUpdateMajorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( !FUpdateMinorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    verCase1.ulVersionMajor++; CHECK( CmpFormatVer( verCase1, verCase2 ) == 0 );

    verCase1.ulVersionUpdateMajor--;
    CHECK( CmpFormatVer( verCase1, verCase2 ) < 0 );
    CHECK( !FMajorVersionMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( FUpdateMajorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( !FUpdateMinorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    verCase1.ulVersionUpdateMajor++; CHECK( CmpFormatVer( verCase1, verCase2 ) == 0 );

    verCase1.ulVersionUpdateMinor--;
    CHECK( CmpFormatVer( verCase1, verCase2 ) < 0 );
    CHECK( !FMajorVersionMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( !FUpdateMajorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( FUpdateMinorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    verCase1.ulVersionUpdateMinor++; CHECK( CmpFormatVer( verCase1, verCase2 ) == 0 );


    GenVersion verCaseA = { 10, 5, 2 };
    GenVersion verCaseB = { 10, 5, 2 };

    CHECK( CmpFormatVer( verCaseA, verCaseB ) == 0 );

    verCaseA.ulVersionMajor--;
    CHECK( CmpFormatVer( verCaseA, verCaseB ) < 0 );
    verCaseA.ulVersionMajor++; CHECK( CmpFormatVer( verCaseA, verCaseB ) == 0 );

    verCaseA.ulVersionUpdateMajor--;
    CHECK( CmpFormatVer( verCaseA, verCaseB ) < 0 );
    verCaseA.ulVersionUpdateMajor++; CHECK( CmpFormatVer( verCaseA, verCaseB ) == 0 );

    verCaseA.ulVersionUpdateMinor--;
    CHECK( CmpFormatVer( verCaseA, verCaseB ) < 0 );
    verCaseA.ulVersionUpdateMinor++; CHECK( CmpFormatVer( verCaseA, verCaseB ) == 0 );
}

JETUNITTEST( GenericVersions, GenericVersionsCheckMismatchPriorities )
{
    GenVersion verTemplate = { 6, 10, 200 };
    GenVersion verCase1 = verTemplate;
    GenVersion verCase2 = verTemplate;

    CHECK( CmpFormatVer( verCase1, verCase2 ) == 0 );

    verCase1.ulVersionMajor += ( rand() % 2 ) ? (1) : (-1);
    verCase1.ulVersionUpdateMajor += ( rand() % 3 ) - 1;
    verCase1.ulVersionUpdateMinor += ( rand() % 3 ) - 1;
    CHECK( CmpFormatVer( verCase1, verCase2 ) != 0 );
    CHECK( CmpFormatVer( verCase2, verCase1 ) != 0 );
    CHECK( FMajorVersionMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( FMajorVersionMismatch( CmpFormatVer( verCase2, verCase1 ) ) );
    CHECK( !FUpdateMajorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( !FUpdateMajorMismatch( CmpFormatVer( verCase2, verCase1 ) ) );
    CHECK( !FUpdateMinorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( !FUpdateMinorMismatch( CmpFormatVer( verCase2, verCase1 ) ) );
    verCase1 = verTemplate; CHECK( CmpFormatVer( verCase1, verCase2 ) == 0 );

    verCase1.ulVersionUpdateMajor += ( rand() % 2 ) ? (1) : (-1);
    verCase1.ulVersionUpdateMinor += ( rand() % 3 ) - 1;
    CHECK( CmpFormatVer( verCase1, verCase2 ) != 0 );
    CHECK( CmpFormatVer( verCase2, verCase1 ) != 0 );
    CHECK( !FMajorVersionMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( !FMajorVersionMismatch( CmpFormatVer( verCase2, verCase1 ) ) );
    CHECK( FUpdateMajorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( FUpdateMajorMismatch( CmpFormatVer( verCase2, verCase1 ) ) );
    CHECK( !FUpdateMinorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( !FUpdateMinorMismatch( CmpFormatVer( verCase2, verCase1 ) ) );
    verCase1 = verTemplate; CHECK( CmpFormatVer( verCase1, verCase2 ) == 0 );

    verCase1.ulVersionUpdateMinor += ( rand() % 2 ) ? (1) : (-1);
    CHECK( CmpFormatVer( verCase1, verCase2 ) != 0 );
    CHECK( CmpFormatVer( verCase2, verCase1 ) != 0 );
    CHECK( !FMajorVersionMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( !FMajorVersionMismatch( CmpFormatVer( verCase2, verCase1 ) ) );
    CHECK( !FUpdateMajorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( !FUpdateMajorMismatch( CmpFormatVer( verCase2, verCase1 ) ) );
    CHECK( FUpdateMinorMismatch( CmpFormatVer( verCase1, verCase2 ) ) );
    CHECK( FUpdateMinorMismatch( CmpFormatVer( verCase2, verCase1 ) ) );
    verCase1 = verTemplate; CHECK( CmpFormatVer( verCase1, verCase2 ) == 0 );

}


JETUNITTEST( EngineFormatVersionsTable, TestLookupOfDbMajors )
{
    DbVersion dbvTest1A = { 0x620, 30, 50 };
    DbVersion dbvTest1B = { 0x620, 30, 30 };
    DbVersion dbvTest1C = { 0x620, 30, 40 };
    DbVersion dbvTest2A = { 0x620, 20, 50 };
    DbVersion dbvTest2B = { 0x620, 20, 20 };
    DbVersion dbvTest2C = { 0x620, 20, 10 };
    DbVersion dbvTest2D = { 0x620, 20, 0 };
    DbVersion dbvTest3A = { 0x620, 100000000, 0 };
    DbVersion dbvTest3B = { 0x10000, 0, 0 };

    const FormatVersions * pfmtversMatching = NULL;

    CallS( ErrDBFindHighestMatchingDbMajors( dbvTest1A, &pfmtversMatching, fFalse ) );
    CHECK( pfmtversMatching->efv == JET_efvExtHdrRootFieldAutoIncStorageReleased );
    CallS( ErrDBFindHighestMatchingDbMajors( dbvTest1B, &pfmtversMatching, fFalse ) );
    CHECK( pfmtversMatching->efv == JET_efvExtHdrRootFieldAutoIncStorageReleased );
    CallS( ErrDBFindHighestMatchingDbMajors( dbvTest1C, &pfmtversMatching, fFalse ) );
    CHECK( pfmtversMatching->efv == JET_efvExtHdrRootFieldAutoIncStorageReleased );

    CallS( ErrDBFindHighestMatchingDbMajors( dbvTest2A, &pfmtversMatching, fFalse ) );
    CHECK( pfmtversMatching->efv == JET_efvSetDbVersion );
    CallS( ErrDBFindHighestMatchingDbMajors( dbvTest2B, &pfmtversMatching, fFalse ) );
    CHECK( pfmtversMatching->efv == JET_efvSetDbVersion );
    CallS( ErrDBFindHighestMatchingDbMajors( dbvTest2C, &pfmtversMatching, fFalse ) );
    CHECK( pfmtversMatching->efv == JET_efvSetDbVersion );
    CallS( ErrDBFindHighestMatchingDbMajors( dbvTest2D, &pfmtversMatching, fFalse ) );
    CHECK( pfmtversMatching->efv == JET_efvSetDbVersion );

    CHECK( JET_errInvalidDatabaseVersion == ErrDBFindHighestMatchingDbMajors( dbvTest3A, &pfmtversMatching, fTrue ) );
    CHECK( JET_errInvalidDatabaseVersion == ErrDBFindHighestMatchingDbMajors( dbvTest3B, &pfmtversMatching, fTrue ) );
}

