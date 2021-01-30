// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifndef ENABLE_JET_UNIT_TEST
#error This file should only be compiled with the unit tests!
#endif

JETUNITTEST( LOG, FormatVersionTests )
{



    #define fmtLGVersion_Test1      4,8000,1,333

    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8000,1,333 ) == 0 );


    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 5,0,0,0 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 5,8000,1,333 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 3,0xFFFF,0xFFFF,0xFFFF ) > 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 3,8000,1,333 ) > 0 );

    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 5,7999,1,333 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 5,8001,1,333 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 5,8000,0,333 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 5,8000,2,333 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 5,8000,1,332 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 5,8001,1,334 ) < 0 );
 
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 3,7999,1,333 ) > 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 3,8001,1,333 ) > 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 3,8000,0,333 ) > 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 3,8000,2,333 ) > 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 3,8000,1,332 ) > 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 3,8001,1,334 ) > 0 );


    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8001,0,0 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8001,1,333 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,7999,0xFFFF,0xFFFF ) > 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,7999,1,333 ) > 0 );

    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8001,0,333 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8001,2,333 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8001,1,332 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8001,1,334 ) < 0 );

    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,7999,0,333 ) > 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,7999,2,333 ) > 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,7999,1,332 ) > 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,7999,1,334 ) > 0 );


    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8000,2,0 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8000,2,333 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8000,0,0xFFFF ) > 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8000,0,333 ) > 0 );

    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8000,2,332 ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8000,2,334 ) < 0 );

    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8000,0,332 ) > 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8000,0,334 ) > 0 );


    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8000,1,0 ) > 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8000,1,332 ) > 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8000,1,0xFFFF ) < 0 );
    CHECK( CmpLogFormatVersion( fmtLGVersion_Test1, 4,8000,1,334 ) < 0 );
}


#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT

bool FLGIsLVChunkSizeCompatible(
    const ULONG cbPage,
    const ULONG ulVersionMajor,
    const ULONG ulVersionMinor,
    const ULONG ulVersionUpdate );

JETUNITTEST( LOG, FLGIsLVChunkSizeCompatible  )
{
    for( INT i = 2; i <= 32; i *= 2 )
    {
        CHECK( true == FLGIsLVChunkSizeCompatible( i*1024, ulLGVersionMajorMax, ulLGVersionMinorFinalDeprecatedValue, ulLGVersionUpdateMajorMax ) );
        CHECK( true == FLGIsLVChunkSizeCompatible( i*1024, ulLGVersionMajorNewLVChunk, ulLGVersionMinorNewLVChunk, ulLGVersionUpdateNewLVChunk ) );
    }

    CHECK( true == FLGIsLVChunkSizeCompatible( 2*1024, ulLGVersionMajorNewLVChunk, ulLGVersionMinorNewLVChunk, ulLGVersionUpdateNewLVChunk-1 ) );
    CHECK( true == FLGIsLVChunkSizeCompatible( 4*1024, ulLGVersionMajorNewLVChunk, ulLGVersionMinorNewLVChunk, ulLGVersionUpdateNewLVChunk-1 ) );
    CHECK( true == FLGIsLVChunkSizeCompatible( 8*1024, ulLGVersionMajorNewLVChunk, ulLGVersionMinorNewLVChunk, ulLGVersionUpdateNewLVChunk-1 ) );
    CHECK( false == FLGIsLVChunkSizeCompatible( 16*1024, ulLGVersionMajorNewLVChunk, ulLGVersionMinorNewLVChunk, ulLGVersionUpdateNewLVChunk-1 ) );
    CHECK( false == FLGIsLVChunkSizeCompatible( 32*1024, ulLGVersionMajorNewLVChunk, ulLGVersionMinorNewLVChunk, ulLGVersionUpdateNewLVChunk-1 ) );
}

#endif

JETUNITTEST( LOGWRITE, SIGGetSignatureReturnsUniqueSignatures )
{
    SIGNATURE signature[2] = { 0 };

    for ( INT i = 0; i < 1000; i++ )
    {
        SIGGetSignature( &signature[i & 1] );
        CHECK( 0 != memcmp( &signature[i & 1], &signature[(i + 1) & 1], sizeof(SIGNATURE) ) );
    }
}


JETUNITTEST( LGFileHelper, LGGetGenerationTests )
{
    ERR err;
    LONG lgen;
    ULONG cLogDigits;
    IFileSystemAPI *pfsapi = NULL;

    err = ErrOSFSCreate( &pfsapi );
    CHECK( err == JET_errSuccess );
    CHECK( pfsapi != NULL );

    err = LGFileHelper::ErrLGGetGeneration( pfsapi, L"C:\\foo\\edb00001.log", L"edb", &lgen, L".jtx", &cLogDigits );
    CHECK( err == JET_errInvalidParameter );

    err = LGFileHelper::ErrLGGetGeneration( pfsapi, L"C:\\foo\\edb00001.log", L"e00", &lgen, L".log", &cLogDigits );
    CHECK( err == JET_errInvalidParameter );

    err = LGFileHelper::ErrLGGetGeneration( pfsapi, L"C:\\foo\\edb00001.log", L"edbres", &lgen, L".log", &cLogDigits );
    CHECK( err == JET_errInvalidParameter );

    err = LGFileHelper::ErrLGGetGeneration( pfsapi, L"C:\\foo\\edb0000.log", L"edb", &lgen, L".log", &cLogDigits );
    CHECK( err == JET_errInvalidParameter );

    err = LGFileHelper::ErrLGGetGeneration( pfsapi, L"C:\\foo\\edbres0001.log", L"edbres", &lgen, L".log", &cLogDigits );
    CHECK( err == JET_errInvalidParameter );

    err = LGFileHelper::ErrLGGetGeneration( pfsapi, L"C:\\foo\\edbres000001.log", L"edbres", &lgen, L".log", &cLogDigits );
    CHECK( err == JET_errInvalidParameter );

    err = LGFileHelper::ErrLGGetGeneration( pfsapi, L"C:\\foo\\edbres000000001.log", L"edbres", &lgen, L".log", &cLogDigits );
    CHECK( err == JET_errInvalidParameter );

    err = LGFileHelper::ErrLGGetGeneration( pfsapi, L"C:\\foo\\edbres00001.log", L"edb", &lgen, L".log", &cLogDigits );
    CHECK( err == JET_errInvalidParameter );

    err = LGFileHelper::ErrLGGetGeneration( pfsapi, L"C:\\foo\\edb123AB.log", L"edb", &lgen, L".log", &cLogDigits );
    CHECK( err == JET_errSuccess );
    CHECK( lgen == 0x123AB );
    CHECK( cLogDigits == 5 );

    err = LGFileHelper::ErrLGGetGeneration( pfsapi, L"C:\\foo\\eDb234BC.log", L"edb", &lgen, L".log", &cLogDigits );
    CHECK( err == JET_errSuccess );
    CHECK( lgen == 0x234BC );
    CHECK( cLogDigits == 5 );

    err = LGFileHelper::ErrLGGetGeneration( pfsapi, L"C:\\foo\\edbresAB123.log", L"edbres", &lgen, L".log", &cLogDigits );
    CHECK( err == JET_errSuccess );
    CHECK( lgen == 0xAB123 );
    CHECK( cLogDigits == 5 );

    err = LGFileHelper::ErrLGGetGeneration( pfsapi, L"C:\\foo\\eDbrEsBC234.log", L"edbres", &lgen, L".log", &cLogDigits );
    CHECK( err == JET_errSuccess );
    CHECK( lgen == 0xBC234 );
    CHECK( cLogDigits == 5 );

    err = LGFileHelper::ErrLGGetGeneration( pfsapi, L"C:\\foo\\edb123456AB.log", L"edb", &lgen, L".log", &cLogDigits );
    CHECK( err == JET_errSuccess );
    CHECK( lgen == 0x123456AB );
    CHECK( cLogDigits == 8 );

    err = LGFileHelper::ErrLGGetGeneration( pfsapi, L"C:\\foo\\edbres654321BA.log", L"edbres", &lgen, L".log", &cLogDigits );
    CHECK( err == JET_errSuccess );
    CHECK( lgen == 0x654321BA );
    CHECK( cLogDigits == 8 );

    delete pfsapi;
}


JETUNITTEST( LOG, CheckRedoConditionForDatabaseTests )
{
    BOOL        fReplayIgnoreLogRecordsBeforeMinRequiredLog;
    DBFILEHDR   dbfilehdr;
    DBFILEHDR*  pdbfilehdr;
    LGPOS       lgpos;
    BOOL        fExpected;

    dbfilehdr.le_lgposAttach.le_lGeneration = 10;
    dbfilehdr.le_lgposAttach.le_isec = 0;
    dbfilehdr.le_lgposAttach.le_ib = 0;
    dbfilehdr.le_lGenMinRequired = 100;

    lgpos = lgposMin;

    fReplayIgnoreLogRecordsBeforeMinRequiredLog = fFalse;
    pdbfilehdr = NULL;
    lgpos.lGeneration = dbfilehdr.le_lgposAttach.le_lGeneration + 1;
    fExpected = fFalse;
    CHECK( fExpected == LOG::FLGRICheckRedoConditionForAttachedDb( fReplayIgnoreLogRecordsBeforeMinRequiredLog, pdbfilehdr, lgpos ) );

    fReplayIgnoreLogRecordsBeforeMinRequiredLog = fFalse;
    pdbfilehdr = &dbfilehdr;
    lgpos = lgposMin;
    fExpected = fFalse;
    CHECK( fExpected == LOG::FLGRICheckRedoConditionForAttachedDb( fReplayIgnoreLogRecordsBeforeMinRequiredLog, pdbfilehdr, lgpos ) );

#ifndef DEBUG

    fReplayIgnoreLogRecordsBeforeMinRequiredLog = fFalse;
    pdbfilehdr = &dbfilehdr;
    lgpos = dbfilehdr.le_lgposAttach;
    fExpected = fFalse;
    CHECK( fExpected == LOG::FLGRICheckRedoConditionForAttachedDb( fReplayIgnoreLogRecordsBeforeMinRequiredLog, pdbfilehdr, lgpos ) );

    fReplayIgnoreLogRecordsBeforeMinRequiredLog = fFalse;
    pdbfilehdr = &dbfilehdr;
    lgpos.lGeneration = dbfilehdr.le_lgposAttach.le_lGeneration - 1;
    fExpected = fFalse;
    CHECK( fExpected == LOG::FLGRICheckRedoConditionForAttachedDb( fReplayIgnoreLogRecordsBeforeMinRequiredLog, pdbfilehdr, lgpos ) );
#endif

    fReplayIgnoreLogRecordsBeforeMinRequiredLog = fTrue;
    pdbfilehdr = &dbfilehdr;
    lgpos.lGeneration = dbfilehdr.le_lGenMinRequired - 1;
    fExpected = fFalse;
    CHECK( fExpected == LOG::FLGRICheckRedoConditionForAttachedDb( fReplayIgnoreLogRecordsBeforeMinRequiredLog, pdbfilehdr, lgpos ) );

    fReplayIgnoreLogRecordsBeforeMinRequiredLog = fFalse;
    pdbfilehdr = &dbfilehdr;
    lgpos.lGeneration = dbfilehdr.le_lgposAttach.le_lGeneration + 1;
    fExpected = fTrue;
    CHECK( fExpected == LOG::FLGRICheckRedoConditionForAttachedDb( fReplayIgnoreLogRecordsBeforeMinRequiredLog, pdbfilehdr, lgpos ) );

    fReplayIgnoreLogRecordsBeforeMinRequiredLog = fFalse;
    pdbfilehdr = &dbfilehdr;
    lgpos.lGeneration = dbfilehdr.le_lGenMinRequired - 1;
    fExpected = fTrue;
    CHECK( fExpected == LOG::FLGRICheckRedoConditionForAttachedDb( fReplayIgnoreLogRecordsBeforeMinRequiredLog, pdbfilehdr, lgpos ) );

    fReplayIgnoreLogRecordsBeforeMinRequiredLog = fTrue;
    pdbfilehdr = &dbfilehdr;
    lgpos.lGeneration = dbfilehdr.le_lGenMinRequired;
    fExpected = fTrue;
    CHECK( fExpected == LOG::FLGRICheckRedoConditionForAttachedDb( fReplayIgnoreLogRecordsBeforeMinRequiredLog, pdbfilehdr, lgpos ) );

    fReplayIgnoreLogRecordsBeforeMinRequiredLog = fFalse;
    pdbfilehdr = &dbfilehdr;
    lgpos.lGeneration = dbfilehdr.le_lGenMinRequired;
    fExpected = fTrue;
    CHECK( fExpected == LOG::FLGRICheckRedoConditionForAttachedDb( fReplayIgnoreLogRecordsBeforeMinRequiredLog, pdbfilehdr, lgpos ) );

    fReplayIgnoreLogRecordsBeforeMinRequiredLog = fTrue;
    pdbfilehdr = &dbfilehdr;
    lgpos.lGeneration = dbfilehdr.le_lGenMinRequired + 1;
    fExpected = fTrue;
    CHECK( fExpected == LOG::FLGRICheckRedoConditionForAttachedDb( fReplayIgnoreLogRecordsBeforeMinRequiredLog, pdbfilehdr, lgpos ) );
}

JETUNITTEST( LGEN_LOGTIME_MAP, CheckLgenLogtimeMap )
{
    LGEN_LOGTIME_MAP map;
    LOGTIME tm1, tm2, tm3, tmEmpty, tmOut;

    LGIGetDateTime( &tm1 );
    memcpy( &tm2, &tm1, sizeof(tm1) );
    tm2.bSeconds = 59 - tm2.bSeconds;
    memcpy( &tm3, &tm1, sizeof(tm1) );
    tm3.bMinutes = 59 - tm3.bMinutes;
    memset( &tmEmpty, 0, sizeof(tmEmpty) );

    LONG lGenStart = 42;

    CHECK( JET_errSuccess == map.ErrAddLogtimeMapping( lGenStart, &tm1 ) );
    map.LookupLogtimeMapping( lGenStart, &tmOut );
    CHECK( memcmp( &tmOut, &tm1, sizeof(tm1) ) == 0 );

    map.LookupLogtimeMapping( lGenStart+1, &tmOut );
    CHECK( memcmp( &tmOut, &tmEmpty, sizeof(tmEmpty) ) == 0 );

    CHECK( JET_errSuccess == map.ErrAddLogtimeMapping( lGenStart+1, &tm2 ) );
    UINT wAssertActionSaved = COSLayerPreInit::SetAssertAction( JET_AssertSkipAll );
    CHECK( JET_errSuccess == map.ErrAddLogtimeMapping( lGenStart+4, &tm3 ) );
    (void)COSLayerPreInit::SetAssertAction( wAssertActionSaved );

    map.LookupLogtimeMapping( lGenStart, &tmOut );
    CHECK( memcmp( &tmOut, &tm1, sizeof(tm1) ) == 0 );
    map.LookupLogtimeMapping( lGenStart+1, &tmOut );
    CHECK( memcmp( &tmOut, &tm2, sizeof(tm2) ) == 0 );
    map.LookupLogtimeMapping( lGenStart+2, &tmOut );
    CHECK( memcmp( &tmOut, &tmEmpty, sizeof(tmEmpty) ) == 0 );
    map.LookupLogtimeMapping( lGenStart+3, &tmOut );
    CHECK( memcmp( &tmOut, &tmEmpty, sizeof(tmEmpty) ) == 0 );
    map.LookupLogtimeMapping( lGenStart+4, &tmOut );
    CHECK( memcmp( &tmOut, &tm3, sizeof(tm3) ) == 0 );
    map.LookupLogtimeMapping( lGenStart+5, &tmOut );
    CHECK( memcmp( &tmOut, &tmEmpty, sizeof(tmEmpty) ) == 0 );

    map.TrimLogtimeMapping( lGenStart+3);
    map.LookupLogtimeMapping( lGenStart+4, &tmOut );
    CHECK( memcmp( &tmOut, &tm3, sizeof(tm3) ) == 0 );
    map.LookupLogtimeMapping( lGenStart+5, &tmOut );
    CHECK( memcmp( &tmOut, &tmEmpty, sizeof(tmEmpty) ) == 0 );

    wAssertActionSaved = COSLayerPreInit::SetAssertAction( JET_AssertSkipAll );
    CHECK( JET_errSuccess == map.ErrAddLogtimeMapping( lGenStart+6, &tm1 ) );
    CHECK( JET_errSuccess == map.ErrAddLogtimeMapping( lGenStart+9, &tm2 ) );
    (void)COSLayerPreInit::SetAssertAction( wAssertActionSaved );

    map.LookupLogtimeMapping( lGenStart+4, &tmOut );
    CHECK( memcmp( &tmOut, &tm3, sizeof(tm3) ) == 0 );
    map.LookupLogtimeMapping( lGenStart+5, &tmOut );
    CHECK( memcmp( &tmOut, &tmEmpty, sizeof(tmEmpty) ) == 0 );
    map.LookupLogtimeMapping( lGenStart+6, &tmOut );
    CHECK( memcmp( &tmOut, &tm1, sizeof(tm1) ) == 0 );
    map.LookupLogtimeMapping( lGenStart+7, &tmOut );
    CHECK( memcmp( &tmOut, &tmEmpty, sizeof(tmEmpty) ) == 0 );
    map.LookupLogtimeMapping( lGenStart+8, &tmOut );
    CHECK( memcmp( &tmOut, &tmEmpty, sizeof(tmEmpty) ) == 0 );
    map.LookupLogtimeMapping( lGenStart+9, &tmOut );
    CHECK( memcmp( &tmOut, &tm2, sizeof(tm2) ) == 0 );
    map.LookupLogtimeMapping( lGenStart+10, &tmOut );
    CHECK( memcmp( &tmOut, &tmEmpty, sizeof(tmEmpty) ) == 0 );

    map.TrimLogtimeMapping( lGenStart+4);
    wAssertActionSaved = COSLayerPreInit::SetAssertAction( JET_AssertSkipAll );
    CHECK( JET_errSuccess == map.ErrAddLogtimeMapping( lGenStart+7, &tm3 ) );
    (void)COSLayerPreInit::SetAssertAction( wAssertActionSaved );
    map.LookupLogtimeMapping( lGenStart+7, &tmOut );
    CHECK( memcmp( &tmOut, &tm3, sizeof(tm3) ) == 0 );
}

