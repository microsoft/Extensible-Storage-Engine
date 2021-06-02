// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifndef ENABLE_JET_UNIT_TEST
#error This file should only be compiled with the unit tests!
#endif // !ENABLE_JET_UNIT_TEST

#define CleanAndTermFlushMap( pfm )                         \
{                                                           \
    CHECK( JET_errSuccess == pfm->ErrCleanFlushMap() );     \
    pfm->TermFlushMap();                                    \
}

JETUNITTEST( CFlushMap, BasicSettingAndResettingToUnknown )
{
    CFlushMap* pfm = new CFlushMapForUnattachedDb();
    pfm->SetPersisted( fFalse );

    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 1 ) );

    pfm->SetPgnoFlushType( 1, CPAGE::pgftRockWrite );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 1 ) );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftPaperWrite );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 1 ) );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftScissorsWrite );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 1 ) );

    pfm->SetPgnoFlushType( 1, CPAGE::pgftUnknown );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 1 ) );

    delete pfm;
}

JETUNITTEST( CFlushMap, SettingHighPageNumbers )
{
    CFlushMap* pfm = new CFlushMapForUnattachedDb();
    pfm->SetPersisted( fFalse );
    const DWORD cpg = 10;
    DWORD pgnoMiddle = 0;
    CPAGE::PageFlushType pgft = CPAGE::pgftRockWrite;

    CHECK( 0 == pfm->m_cbFmPageInMemory );
    CHECK( 0 == pfm->m_cbDescriptorPage );
    CHECK( 0 == pfm->m_cfmdCommitted );
    CHECK( 0 == pfm->m_cfmpgAllocated );
    CHECK( 0 == pfm->m_cbfmdReserved );
    CHECK( 0 == pfm->m_cbfmdCommitted );
    CHECK( 0 == pfm->m_cbfmAllocated );

    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    CHECK( 0 < pfm->m_cbFmPageInMemory );
    CHECK( 0 < pfm->m_cbDescriptorPage );
    CHECK( 0 == pfm->m_cfmdCommitted );
    CHECK( 1 == pfm->m_cfmpgAllocated );
    CHECK( 0 == pfm->m_cbfmdReserved );
    CHECK( 0 == pfm->m_cbfmdCommitted );
    CHECK( ( 1 * pfm->m_cbFmPageInMemory ) == pfm->m_cbfmAllocated );

    // Crossing first flush map page.
    pgnoMiddle = pfm->m_cDbPagesPerFlushMapPage;

    for ( DWORD pgno = pgnoMiddle - cpg / 2; pgno < ( pgnoMiddle + cpg / 2 ); pgno++, pgft = CPAGE::PgftGetNextFlushType( pgft ) )
    {
        CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgno ) );
        CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
        pfm->SetPgnoFlushType( pgno, pgft );
        CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

        CHECK( ( pfm->m_cbfmdCommitted / sizeof( CFlushMap::FlushMapPageDescriptor ) ) == (DWORD)pfm->m_cfmdCommitted );
        CHECK( ( pgno / pfm->m_cDbPagesPerFlushMapPage + 1 ) == (DWORD)pfm->m_cfmpgAllocated );
        CHECK( pfm->m_cbfmAllocated  == ( pfm->m_cfmpgAllocated * pfm->m_cbFmPageInMemory ) );

        if ( pgno < pgnoMiddle )
        {
            CHECK( 0 == pfm->m_cbfmdReserved );
            CHECK( 0 == pfm->m_cbfmdCommitted );
        }
        else
        {
            CHECK( pfm->m_cbfmdReserved > pfm->m_cbfmdCommitted );
            CHECK( ( 1 * pfm->m_cbDescriptorPage ) == pfm->m_cbfmdCommitted );
        }
    }

    // Crossing first -> second flush map descriptor page.
    pgnoMiddle = pfm->m_cDbPagesPerFlushMapPage * ( ( 1 * pfm->m_cbDescriptorPage ) / sizeof( CFlushMap::FlushMapPageDescriptor ) );

    for ( DWORD pgno = pgnoMiddle - cpg / 2; pgno < ( pgnoMiddle + cpg / 2 ); pgno++, pgft = CPAGE::PgftGetNextFlushType( pgft ) )
    {
        CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgno ) );
        CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
        pfm->SetPgnoFlushType( pgno, pgft );
        CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

        CHECK( ( pfm->m_cbfmdCommitted / sizeof( CFlushMap::FlushMapPageDescriptor ) ) == (DWORD)pfm->m_cfmdCommitted );
        CHECK( ( pgno / pfm->m_cDbPagesPerFlushMapPage + 1 ) == (DWORD)pfm->m_cfmpgAllocated );
        CHECK( pfm->m_cbfmAllocated  == ( pfm->m_cfmpgAllocated * pfm->m_cbFmPageInMemory ) );

        if ( pgno < pgnoMiddle )
        {
            CHECK( ( 1 * pfm->m_cbDescriptorPage ) == pfm->m_cbfmdCommitted );
        }
        else
        {
            CHECK( ( 2 * pfm->m_cbDescriptorPage ) == pfm->m_cbfmdCommitted );
        }
    }

    // Crossing second -> third flush map descriptor page.
    pgnoMiddle = pfm->m_cDbPagesPerFlushMapPage * ( ( 2 * pfm->m_cbDescriptorPage ) / sizeof( CFlushMap::FlushMapPageDescriptor ) );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMiddle - cpg / 2 ) );

    for ( DWORD pgno = pgnoMiddle - cpg / 2; pgno < ( pgnoMiddle + cpg / 2 ); pgno++, pgft = CPAGE::PgftGetNextFlushType( pgft ) )
    {
        CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgno ) );
        CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
        pfm->SetPgnoFlushType( pgno, pgft );
        CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

        CHECK( ( pfm->m_cbfmdCommitted / sizeof( CFlushMap::FlushMapPageDescriptor ) ) == (DWORD)pfm->m_cfmdCommitted );
        CHECK( ( pgno / pfm->m_cDbPagesPerFlushMapPage + 1 ) == (DWORD)pfm->m_cfmpgAllocated );
        CHECK( pfm->m_cbfmAllocated  == ( pfm->m_cfmpgAllocated * pfm->m_cbFmPageInMemory ) );

        if ( pgno < pgnoMiddle )
        {
            CHECK( ( 2 * pfm->m_cbDescriptorPage ) == pfm->m_cbfmdCommitted );
        }
        else
        {
            CHECK( ( 3 * pfm->m_cbDescriptorPage ) == pfm->m_cbfmdCommitted );
        }
    }

    // Crossing fifth -> sixth flush map descriptor page.
    pgnoMiddle = pfm->m_cDbPagesPerFlushMapPage * ( ( 5 * pfm->m_cbDescriptorPage ) / sizeof( CFlushMap::FlushMapPageDescriptor ) );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMiddle - cpg / 2 ) );

    for ( DWORD pgno = pgnoMiddle - cpg / 2; pgno < ( pgnoMiddle + cpg / 2 ); pgno++, pgft = CPAGE::PgftGetNextFlushType( pgft ) )
    {
        CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgno ) );
        CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
        pfm->SetPgnoFlushType( pgno, pgft );
        CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

        CHECK( ( pfm->m_cbfmdCommitted / sizeof( CFlushMap::FlushMapPageDescriptor ) ) == (DWORD)pfm->m_cfmdCommitted );
        CHECK( ( pgno / pfm->m_cDbPagesPerFlushMapPage + 1 ) == (DWORD)pfm->m_cfmpgAllocated );
        CHECK( pfm->m_cbfmAllocated  == ( pfm->m_cfmpgAllocated * pfm->m_cbFmPageInMemory ) );

        if ( pgno < pgnoMiddle )
        {
            CHECK( ( 5 * pfm->m_cbDescriptorPage ) == pfm->m_cbfmdCommitted );
        }
        else
        {
            CHECK( ( 6 * pfm->m_cbDescriptorPage ) == pfm->m_cbfmdCommitted );
        }
    }

    delete pfm;
}

JETUNITTEST( CFlushMap, SettingAWholeFlushMapPage )
{
    CFlushMap* pfm = new CFlushMapForUnattachedDb();
    pfm->SetPersisted( fFalse );
    CPAGE::PageFlushType pgft = CPAGE::pgftRockWrite;

    CHECK( 0 == pfm->m_cbfmdReserved );
    CHECK( 0 == pfm->m_cbfmdCommitted );
    CHECK( 0 == pfm->m_cbfmAllocated );

    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    CHECK( 0 == pfm->m_cbfmdReserved );
    CHECK( 0 == pfm->m_cbfmdCommitted );
    CHECK( ( 1 * pfm->m_cbFmPageInMemory ) == pfm->m_cbfmAllocated );

    // Filling entire first page.
    DWORD pgno = 0;
    for ( ; pgno < pfm->m_cDbPagesPerFlushMapPage; pgno++, pgft = CPAGE::PgftGetNextFlushType( pgft ) )
    {
        CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
        pfm->SetPgnoFlushType( pgno, pgft );
        CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
        CHECK( 0 == pfm->m_cfmdCommitted );
        CHECK( 1 == pfm->m_cfmpgAllocated );
        CHECK( 0 == pfm->m_cbfmdReserved );
        CHECK( 0 == pfm->m_cbfmdCommitted );
        CHECK( ( 1 * pfm->m_cbFmPageInMemory ) == pfm->m_cbfmAllocated );
    }

    // Next one needs capacity growth.
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CHECK( 0 == pfm->m_cfmdCommitted );
    CHECK( 1 == pfm->m_cfmpgAllocated );
    CHECK( 0 == pfm->m_cbfmdReserved );
    CHECK( 0 == pfm->m_cbfmdCommitted );
    CHECK( ( 1 * pfm->m_cbFmPageInMemory ) == pfm->m_cbfmAllocated );

    // Grow capacity.
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgno ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    CHECK( ( pfm->m_cbfmdCommitted / sizeof( CFlushMap::FlushMapPageDescriptor ) ) == (DWORD)pfm->m_cfmdCommitted );
    CHECK( 2 == pfm->m_cfmpgAllocated );
    CHECK( pfm->m_cbfmdReserved > pfm->m_cbfmdCommitted );
    CHECK( ( 1 * pfm->m_cbDescriptorPage ) == pfm->m_cbfmdCommitted );
    CHECK( ( 2 * pfm->m_cbFmPageInMemory ) == pfm->m_cbfmAllocated );

    // Next one is two flush map pages away and needs capacity growth.
    pgno += ( 2 * pfm->m_cDbPagesPerFlushMapPage );
    pgft = CPAGE::PgftGetNextFlushType( pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CHECK( ( pfm->m_cbfmdCommitted / sizeof( CFlushMap::FlushMapPageDescriptor ) ) == (DWORD)pfm->m_cfmdCommitted );
    CHECK( 2 == pfm->m_cfmpgAllocated );
    CHECK( pfm->m_cbfmdReserved > pfm->m_cbfmdCommitted );
    CHECK( ( 1 * pfm->m_cbDescriptorPage ) == pfm->m_cbfmdCommitted );
    CHECK( ( 2 * pfm->m_cbFmPageInMemory ) == pfm->m_cbfmAllocated );

    // Grow capacity.
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgno ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    CHECK( ( pfm->m_cbfmdCommitted / sizeof( CFlushMap::FlushMapPageDescriptor ) ) == (DWORD)pfm->m_cfmdCommitted );
    CHECK( 4 == pfm->m_cfmpgAllocated );
    CHECK( pfm->m_cbfmdReserved > pfm->m_cbfmdCommitted );
    CHECK( ( 1 * pfm->m_cbDescriptorPage ) == pfm->m_cbfmdCommitted );
    CHECK( ( 4 * pfm->m_cbFmPageInMemory ) == pfm->m_cbfmAllocated );

    // Halfway-through page should be unknown.
    pgno -= pfm->m_cDbPagesPerFlushMapPage;
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );

    delete pfm;
}

JETUNITTEST( CFlushMap, BasicRangeSettingAndResetting )
{
    CFlushMap* pfm = new CFlushMapForUnattachedDb();
    pfm->SetPersisted( fFalse );

    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    CHECK( JET_errSuccess == pfm->ErrSetRangeFlushTypeAndWait( 2, 1, CPAGE::pgftRockWrite ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 0 ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 1 ) );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 2 ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 3 ) );

    CHECK( JET_errSuccess == pfm->ErrSetRangeFlushTypeAndWait( 1, 3, CPAGE::pgftPaperWrite ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 0 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 1 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 3 ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 4 ) );

    CHECK( JET_errSuccess == pfm->ErrSetRangeFlushTypeAndWait( 1, 4, CPAGE::pgftPaperWrite ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 0 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 1 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 3 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 4 ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 5 ) );

    CHECK( JET_errSuccess == pfm->ErrSetRangeFlushTypeAndWait( 0, 2, CPAGE::pgftScissorsWrite ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 0 ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 1 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 3 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 4 ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 5 ) );

    CHECK( JET_errSuccess == pfm->ErrSetRangeFlushTypeAndWait( 5, 1, CPAGE::pgftRockWrite ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 0 ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 1 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 3 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 4 ) );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 5 ) );

    CHECK( JET_errSuccess == pfm->ErrSetRangeFlushTypeAndWait( 2, 2, CPAGE::pgftUnknown ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 0 ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 1 ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 2 ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 3 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 4 ) );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 5 ) );

    delete pfm;
}

JETUNITTEST( CFlushMap, BasicPersistedFlushMap )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    PGNO pgno = 0;
    CPAGE::PageFlushType pgft = CPAGE::pgftRockWrite;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );

    // Make sure file doesn't exist.
    (void)pfsapi->ErrFileDelete( wszFmFilePath );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Initialize.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // Set the flush state on a few pages.

    pgno = 1;
    pgft = CPAGE::pgftRockWrite;
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    pgno += 100000;
    pgft = CPAGE::PgftGetNextFlushType( pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    pgno += 100000;
    pgft = CPAGE::PgftGetNextFlushType( pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    // Terminate cleanly.
    CleanAndTermFlushMap( pfm );

    // Re-initialize and read the states.

    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    pgno = 1;
    pgft = CPAGE::pgftRockWrite;
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    pgno += 100000;
    pgft = CPAGE::PgftGetNextFlushType( pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    pgno += 100000;
    pgft = CPAGE::PgftGetNextFlushType( pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    // Terminate cleanly.
    CleanAndTermFlushMap( pfm );

    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, RecoverabilityIsReportedCorrectly )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();

    // Unrecoverable.
    pfm->SetPersisted( fFalse );
    pfm->SetRecoverable( fFalse );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( !pfm->FRecoverable() );
    pfm->TermFlushMap();

    // Recoverable.
    pfm->SetPersisted( fFalse );
    pfm->SetRecoverable( fTrue );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( pfm->FRecoverable() );
    pfm->TermFlushMap();

    delete pfm;
}

JETUNITTEST( CFlushMap, InconsistentDbTimeMustInvalidateNonPersistedFlushMapPage )
{
#ifndef RTM
    const bool fPreviouslySet = FNegTestSet( fCorruptingWithLostFlush );
#endif

    CFlushMap* pfm = new CFlushMapForUnattachedDb();
    pfm->SetPersisted( fFalse );

    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    // No DBTIME established, even a very high one succeeds.
    pfm->SetPgnoFlushType( 1, CPAGE::pgftRockWrite );
    pfm->SetPgnoFlushType( 2, CPAGE::pgftPaperWrite );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 1 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2 ) );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 1, dbtimeNil, NULL ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2, dbtimeNil, NULL ) );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 1, 0xFFFFFFFFFF, NULL ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2, 0xFFFFFFFFFF, NULL ) );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftScissorsWrite, dbtimeNil );
    pfm->SetPgnoFlushType( 2, CPAGE::pgftRockWrite, dbtimeNil );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 1, 0xFFFFFFFFFF, NULL ) );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 2, 0xFFFFFFFFFF, NULL ) );

    // Establish DBTIME.
    pfm->SetPgnoFlushType( 1, CPAGE::pgftPaperWrite );
    pfm->SetPgnoFlushType( 2, CPAGE::pgftScissorsWrite, 10 );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 1 ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 2 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 1, dbtimeNil, NULL ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 2, dbtimeNil, NULL ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 1, 10, NULL ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 2, 10, NULL ) );

    // Violate DBTIME.
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 1, 11, NULL ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 2, 11, NULL ) );

    // Establish different DBTIMEs.
    pfm->SetPgnoFlushType( 1, CPAGE::pgftRockWrite, 10 );
    pfm->SetPgnoFlushType( 2, CPAGE::pgftPaperWrite, 100 );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftScissorsWrite, 50 );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 1 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2 ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 1, dbtimeNil, NULL ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2, dbtimeNil, NULL ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 1, 100, NULL ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2, 100, NULL ) );

    // Violate DBTIME.
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 1, 101, NULL ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 2, 101, NULL ) );

    delete pfm;

#ifndef RTM
    FNegTestSet( fPreviouslySet );
#endif
}

JETUNITTEST( CFlushMap, InconsistentDbTimeMustInvalidatePersistedFlushMapPage )
{
#ifndef RTM
    const bool fPreviouslySet = FNegTestSet( fCorruptingWithLostFlush );
#endif

    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetPersisted( fTrue );
    pfm->SetReadOnly( fFalse );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    // No DBTIME established, even a very high one succeeds.
    pfm->SetPgnoFlushType( 1, CPAGE::pgftRockWrite );
    pfm->SetPgnoFlushType( 2, CPAGE::pgftPaperWrite );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 1 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2 ) );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 1, dbtimeNil, NULL ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2, dbtimeNil, NULL ) );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 1, 0xFFFFFFFFFF, NULL ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2, 0xFFFFFFFFFF, NULL ) );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftScissorsWrite, dbtimeNil );
    pfm->SetPgnoFlushType( 2, CPAGE::pgftRockWrite, dbtimeNil );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 1, 0xFFFFFFFFFF, NULL ) );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 2, 0xFFFFFFFFFF, NULL ) );

    // Establish DBTIME.
    pfm->SetPgnoFlushType( 1, CPAGE::pgftPaperWrite );
    pfm->SetPgnoFlushType( 2, CPAGE::pgftScissorsWrite, 10 );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 1 ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 2 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 1, dbtimeNil, NULL ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 2, dbtimeNil, NULL ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 1, 10, NULL ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 2, 10, NULL ) );

    // Violate DBTIME.
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 1, 11, NULL ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 2, 11, NULL ) );

    // Establish different DBTIMEs.
    pfm->SetPgnoFlushType( 1, CPAGE::pgftRockWrite, 10 );
    pfm->SetPgnoFlushType( 2, CPAGE::pgftPaperWrite, 100 );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftScissorsWrite, 50 );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 1 ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2 ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 1, dbtimeNil, NULL ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2, dbtimeNil, NULL ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 1, 100, NULL ) );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2, 100, NULL ) );

    // Violate DBTIME.
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 1, 101, NULL ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 2, 101, NULL ) );

    // Re-establish DBTIMEs and terminate cleanly.
    pfm->SetPgnoFlushType( 1, CPAGE::pgftPaperWrite, 1000 );
    pfm->SetPgnoFlushType( 2, CPAGE::pgftScissorsWrite, 10000 );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftRockWrite, 500 );
    CleanAndTermFlushMap( pfm );

    // Re-attach and assert that we've correctly persisted DBTIMEs.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 1 ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 2 ) );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 1, dbtimeNil, NULL ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 2, dbtimeNil, NULL ) );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 1, 10000, NULL ) );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 2, 10000, NULL ) );

    // Violate DBTIME.
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 1, 10001, NULL ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 2, 10001, NULL ) );

    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    delete pfsapi;
    delete pfm;

#ifndef RTM
    FNegTestSet( fPreviouslySet );
#endif
}

JETUNITTEST( CFlushMap, NonPersistedFlushMapAlwaysReportsRuntime )
{
    BOOL fRuntime = fFalse;
    CFlushMap* pfm = new CFlushMapForUnattachedDb();
    pfm->SetPersisted( fFalse );

    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 1, dbtimeNil, &fRuntime ) );
    CHECK( fRuntime );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftRockWrite );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 1, dbtimeNil, &fRuntime ) );
    CHECK( fRuntime );

    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 2, dbtimeNil, &fRuntime ) );
    CHECK( fRuntime );
    pfm->SetPgnoFlushType( 2, CPAGE::pgftPaperWrite );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2, dbtimeNil, &fRuntime ) );
    CHECK( fRuntime );

    delete pfm;
}

JETUNITTEST( CFlushMap, PersistedFlushMapReportsRuntimeStateCorrectly )
{
    BOOL fRuntime = fFalse;
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetPersisted( fTrue );
    pfm->SetReadOnly( fFalse );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    // Initially, all states are considered 'runtime' because a new map does not
    // have anything persisted.

    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 1, dbtimeNil, &fRuntime ) );
    CHECK( fRuntime );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftRockWrite );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 1, dbtimeNil, &fRuntime ) );
    CHECK( fRuntime );

    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 2, dbtimeNil, &fRuntime ) );
    CHECK( fRuntime );
    pfm->SetPgnoFlushType( 2, CPAGE::pgftPaperWrite );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2, dbtimeNil, &fRuntime ) );
    CHECK( fRuntime );

    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 3, dbtimeNil, &fRuntime ) );
    CHECK( fRuntime );

    // Terminate cleanly and re-attach.
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    // Now the runtime state is supposed to be unset.

    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 1, dbtimeNil, &fRuntime ) );
    CHECK( !fRuntime );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftPaperWrite );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 1, dbtimeNil, &fRuntime ) );
    CHECK( fRuntime );

    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2, dbtimeNil, &fRuntime ) );
    CHECK( !fRuntime );
    pfm->SetPgnoFlushType( 2, CPAGE::pgftScissorsWrite );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( 2, dbtimeNil, &fRuntime ) );
    CHECK( fRuntime );

    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 3, dbtimeNil, &fRuntime ) );
    CHECK( !fRuntime );
    pfm->SetPgnoFlushType( 3, CPAGE::pgftRockWrite );
    CHECK( CPAGE::pgftRockWrite == pfm->PgftGetPgnoFlushType( 3, dbtimeNil, &fRuntime ) );
    CHECK( fRuntime );

    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, FragmentedFlushMapFileSizeIsConsistent )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    QWORD cbFileSize = 0;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetPersisted( fTrue );
    pfm->SetReadOnly( fFalse );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Only up to pgno 1.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    const CPG cpgPerFmPage = pfm->m_cDbPagesPerFlushMapPage;
    CHECK( pfm->m_cfmpgAllocated == 1 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 1 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // Fill the entire first FM page.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 1 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 1 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // Entire first page + 1.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 2 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 2 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // Two pages completely filled up.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 2 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 2 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 2 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // Two pages completely filled up + 1.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 2 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 3 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 3 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // Previous test did not delete file. Make sure size remains the same.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( pfm->m_cfmpgAllocated == 3 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 3 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Non-persisted, non-read-only.
    pfm->SetPersisted( fFalse );
    pfm->SetReadOnly( fFalse );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    pfm->SetPgnoFlushType( cpgPerFmPage, CPAGE::pgftRockWrite );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( cpgPerFmPage ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    pfm->SetPgnoFlushType( cpgPerFmPage, CPAGE::pgftPaperWrite );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 2 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // Non-persisted, read-only.
    pfm->SetPersisted( fFalse );
    pfm->SetReadOnly( fTrue );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    pfm->SetPgnoFlushType( cpgPerFmPage, CPAGE::pgftRockWrite );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( cpgPerFmPage ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    pfm->SetPgnoFlushType( cpgPerFmPage, CPAGE::pgftPaperWrite );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 2 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // Persisted, non-read-only.
    pfm->SetPersisted( fTrue );
    pfm->SetReadOnly( fFalse );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    pfm->SetPgnoFlushType( cpgPerFmPage, CPAGE::pgftRockWrite );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( cpgPerFmPage ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    pfm->SetPgnoFlushType( cpgPerFmPage, CPAGE::pgftPaperWrite );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 2 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 2 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    pfm->SetPgnoFlushType( cpgPerFmPage, CPAGE::pgftScissorsWrite );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 2 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 2 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    pfm->SetPgnoFlushType( cpgPerFmPage, CPAGE::pgftPaperWrite );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 2 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 2 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // Try opening previous map read-only. Should be able to grow and set new state,
    // but file size is not supposed to change.
    pfm->SetPersisted( fTrue );
    pfm->SetReadOnly( fTrue );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 2 );
    pfm->TermFlushMap();
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 2 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    pfm->SetPgnoFlushType( cpgPerFmPage, CPAGE::pgftScissorsWrite );
    CHECK( CPAGE::pgftScissorsWrite == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 2 * cpgPerFmPage ) );
    pfm->SetPgnoFlushType( 2 * cpgPerFmPage, CPAGE::pgftRockWrite );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 2 * cpgPerFmPage ) );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 2 * cpgPerFmPage ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 2 * cpgPerFmPage ) );
    pfm->SetPgnoFlushType( 2 * cpgPerFmPage, CPAGE::pgftPaperWrite );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( 2 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 3 );
    pfm->TermFlushMap();
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 2 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( 2 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 2 );
    pfm->TermFlushMap();

    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    
    // Persisted, read-only.
    pfm->SetPersisted( fTrue );
    pfm->SetReadOnly( fTrue );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    pfm->SetPgnoFlushType( cpgPerFmPage, CPAGE::pgftRockWrite );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( cpgPerFmPage ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    pfm->SetPgnoFlushType( cpgPerFmPage, CPAGE::pgftPaperWrite );
    CHECK( CPAGE::pgftPaperWrite == pfm->PgftGetPgnoFlushType( cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 2 );
    pfm->TermFlushMap();
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, NonFragmentedSmallClientFlushMapFileSizeIsConsistent )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    QWORD cbFileSize = 0;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    const ULONG cbWriteSizeMaxOriginal = (ULONG)UlParam( JET_paramMaxCoalesceWriteSize );
    const ULONG cbWriteSizeMaxNew = 48 * CFlushMap::s_cbFlushMapPageOnDisk; // 48 map pages per write.

    // Override system parameters.
    Param( pinstNil, JET_paramMaxCoalesceWriteSize )->Set( pinstNil, ppibNil, cbWriteSizeMaxNew, NULL );
    CHECK( cbWriteSizeMaxNew == UlParam( JET_paramMaxCoalesceWriteSize ) );
    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
    {
        public:
            CFileSystemConfiguration( ULONG cbMaxWriteSize )
            {
                m_cbMaxWriteSize = cbMaxWriteSize;
            }
    } fsconfig( cbWriteSizeMaxNew );

    CHECK( JET_errSuccess == ErrOSFSCreate( &fsconfig, &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFileSystemConfiguration( &fsconfig );
    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetPersisted( fTrue );
    pfm->SetReadOnly( fFalse );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );
    pfm->SetDbExtensionSize( 256 );

    // Init only. Header + 1 data page.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    const CPG cpgPerFmPage = pfm->m_cDbPagesPerFlushMapPage;
    CHECK( pfm->m_cfmpgAllocated == 1 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 1 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // Fill the entire first FM page. Size unchanged. Header + 1 data page.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 1 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 1 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // Entire first page + 1. Header + 3 data pages. +2 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 3 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 3 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 3 pages completely filled up. Size unchanged. Header + 3 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 3 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 3 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 3 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 3 pages completely filled up + 1. Header + 7 data pages. +4 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 3 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 7 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 7 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 7 pages completely filled up. Size unchanged. Header + 7 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 7 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 7 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 7 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 7 pages completely filled up + 1. Header + 15 data pages. +8 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 7 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 15 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 15 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 15 pages completely filled up. Size unchanged. Header + 15 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 15 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 15 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 15 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 15 pages completely filled up + 1. Header + 31 data pages. +16 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 15 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 31 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 31 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 31 pages completely filled up. Size unchanged. Header + 31 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 31 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 31 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 31 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 31 pages completely filled up + 1. Header + 48 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 31 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 48 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 48 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 48 pages completely filled up. Size unchanged. Header + 48 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 48 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 48 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 48 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 48 pages completely filled up + 1. Header + 96 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 48 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 96 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 96 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 96 pages completely filled up. Size unchanged. Header + 96 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 96 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 96 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 96 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 96 pages completely filled up + 1. Header + 192 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 96 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 192 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 192 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 192 pages completely filled up. Size unchanged. Header + 192 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 192 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 192 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 192 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 192 pages completely filled up + 1. Header + 384 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 192 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 384 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 384 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 384 pages completely filled up. Size unchanged. Header + 384 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 384 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 384 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 384 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 384 pages completely filled up + 1. Header + 576 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 384 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 576 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 576 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 576 pages completely filled up. Size unchanged. Header + 576 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 576 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 576 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 576 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 576 pages completely filled up + 1. Header + 768 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 576 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 768 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 768 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 768 pages completely filled up. Size unchanged. Header + 768 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 768 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 768 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 768 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 768 pages completely filled up + 1. Header + 960 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 768 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 960 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 960 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // Restore system parameters.
    Param( pinstNil, JET_paramMaxCoalesceWriteSize )->Set( pinstNil, ppibNil, cbWriteSizeMaxOriginal, NULL );
    CHECK( cbWriteSizeMaxOriginal == UlParam( JET_paramMaxCoalesceWriteSize ) );

    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, NonFragmentedLargeClientFlushMapFileSizeIsConsistent )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    QWORD cbFileSize = 0;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    const ULONG cbWriteSizeMaxOriginal = (ULONG)UlParam( JET_paramMaxCoalesceWriteSize );
    const ULONG cbWriteSizeMaxNew = 48 * CFlushMap::s_cbFlushMapPageOnDisk; // 48 map pages per write.

    // Override system parameters.
    Param( pinstNil, JET_paramMaxCoalesceWriteSize )->Set( pinstNil, ppibNil, cbWriteSizeMaxNew, NULL );
    CHECK( cbWriteSizeMaxNew == UlParam( JET_paramMaxCoalesceWriteSize ) );
    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
    {
        public:
            CFileSystemConfiguration( ULONG cbMaxWriteSize )
            {
                m_cbMaxWriteSize = cbMaxWriteSize;
            }
    } fsconfig( cbWriteSizeMaxNew );

    CHECK( JET_errSuccess == ErrOSFSCreate( &fsconfig, &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFileSystemConfiguration( &fsconfig );
    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetPersisted( fTrue );
    pfm->SetReadOnly( fFalse );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );
    pfm->SetDbExtensionSize( 4096 );

    // Init only. Header + 48 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    const CPG cpgPerFmPage = pfm->m_cDbPagesPerFlushMapPage;
    CHECK( pfm->m_cfmpgAllocated == 48 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 48 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 48 pages completely filled up. Size unchanged. Header + 48 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 48 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 48 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 48 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 48 pages completely filled up + 1. Header + 96 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 48 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 96 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 96 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 96 pages completely filled up. Size unchanged. Header + 96 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 96 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 96 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 96 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 96 pages completely filled up + 1. Header + 192 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 96 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 192 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 192 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 192 pages completely filled up. Size unchanged. Header + 192 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 192 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 192 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 192 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 192 pages completely filled up + 1. Header + 384 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 192 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 384 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 384 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 384 pages completely filled up. Size unchanged. Header + 384 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 384 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 384 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 384 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 384 pages completely filled up + 1. Header + 576 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 384 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 576 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 576 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 576 pages completely filled up. Size unchanged. Header + 576 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 576 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 576 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 576 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 576 pages completely filled up + 1. Header + 768 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 576 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 768 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 768 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 768 pages completely filled up. Size unchanged. Header + 768 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 768 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 768 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 768 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // 768 pages completely filled up + 1. Header + 960 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 768 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 960 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSize ) );
    CHECK( ( ( 1 + 960 ) * CFlushMap::s_cbFlushMapPageOnDisk ) == cbFileSize );

    // Restore system parameters.
    Param( pinstNil, JET_paramMaxCoalesceWriteSize )->Set( pinstNil, ppibNil, cbWriteSizeMaxOriginal, NULL );
    CHECK( cbWriteSizeMaxOriginal == UlParam( JET_paramMaxCoalesceWriteSize ) );

    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, NonFragmentedNonPersistedFlushMapAllocationIsConsistent )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    const ULONG cbWriteSizeMaxOriginal = (ULONG)UlParam( JET_paramMaxCoalesceWriteSize );
    const ULONG cbWriteSizeMaxNew = 48 * CFlushMap::s_cbFlushMapPageOnDisk; // 48 map pages per write.

    // Override system parameters.
    Param( pinstNil, JET_paramMaxCoalesceWriteSize )->Set( pinstNil, ppibNil, cbWriteSizeMaxNew, NULL );
    CHECK( cbWriteSizeMaxNew == UlParam( JET_paramMaxCoalesceWriteSize ) );
    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
    {
        public:
            CFileSystemConfiguration( ULONG cbMaxWriteSize )
            {
                m_cbMaxWriteSize = cbMaxWriteSize;
            }
    } fsconfig( cbWriteSizeMaxNew );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFileSystemConfiguration( &fsconfig );
    pfm->SetPersisted( fFalse );
    pfm->SetReadOnly( fFalse );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbExtensionSize( 4096 );

    // Init only. Header + 48 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    const CPG cpgPerFmPage = pfm->m_cDbPagesPerFlushMapPage;
    CHECK( pfm->m_cfmpgAllocated == 1 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // 1 page completely filled up. Size unchanged. Header + 1 data page.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 1 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 1 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // 1 page completely filled up + 1. Header + 2 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 1 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 2 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // 2 pages completely filled up. Size unchanged. Header + 2 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 2 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 2 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // 2 pages completely filled up + 1. Header + 3 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 2 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 3 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // 100 pages completely filled up. Header + 100 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 100 * cpgPerFmPage - 1 ) );
    CHECK( pfm->m_cfmpgAllocated == 100 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // 100 pages completely filled up + 1. Header + 101 data pages.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 100 * cpgPerFmPage ) );
    CHECK( pfm->m_cfmpgAllocated == 101 );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // Restore system parameters.
    Param( pinstNil, JET_paramMaxCoalesceWriteSize )->Set( pinstNil, ppibNil, cbWriteSizeMaxOriginal, NULL );
    CHECK( cbWriteSizeMaxOriginal == UlParam( JET_paramMaxCoalesceWriteSize ) );

    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, DirtyTermDoesNotPersistState )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    PGNO pgno = 0;
    CPAGE::PageFlushType pgft = CPAGE::pgftRockWrite;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );
    pfm->SetRecoverable( fTrue );

    // Set flush state.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    pgno = 1;
    pgft = CPAGE::pgftRockWrite;
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    // Terminate dirty.
    pfm->TermFlushMap();

    // Re-initialize and read the state.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->TermFlushMap();

    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, FlushMapFlushPersistsStateWhenCleaned )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    PGNO pgno = 0;
    CPAGE::PageFlushType pgft = CPAGE::pgftRockWrite;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );
    pfm->SetRecoverable( fTrue );

    // Set flush state.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    pgno = 1;
    pgft = CPAGE::pgftRockWrite;
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    // Flush and term clean.
    CHECK( JET_errSuccess == pfm->ErrCleanFlushMap() );
    pfm->TermFlushMap();

    // Re-initialize and read the state.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->TermFlushMap();

    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, FlushMapFlushPersistsStateWhenFlushedCompletely )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    PGNO pgno = 0;
    CPAGE::PageFlushType pgft = CPAGE::pgftRockWrite;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );
    pfm->SetRecoverable( fTrue );

    // Set flush state.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    pgno = 1;
    pgft = CPAGE::pgftRockWrite;
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    // Flush and term dirty.
    CHECK( JET_errSuccess == pfm->ErrFlushAllSections( OnDebug( fTrue ) ) );
    pfm->TermFlushMap();

    // Re-initialize and read the state.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->TermFlushMap();

    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, NonPersistedOrReadOnlyDoesNotCreateFlushMapFile )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Non-persisted, non-read-only.
    pfm->SetPersisted( fFalse );
    pfm->SetReadOnly( fFalse );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // Non-persisted, read-only.
    pfm->SetPersisted( fFalse );
    pfm->SetReadOnly( fTrue );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // Persisted, non-read-only.
    pfm->SetPersisted( fTrue );
    pfm->SetReadOnly( fFalse );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Persisted, read-only.
    pfm->SetPersisted( fTrue );
    pfm->SetReadOnly( fTrue );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, ReadOnlyReAttachDoesNotChangeFlushMapFile )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    PGNO pgno = 0;
    CPAGE::PageFlushType pgft = CPAGE::pgftRockWrite, pgftPrev = CPAGE::pgftUnknown;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Create flush map file.
    pfm->SetPersisted( fTrue );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    // Set flush state and terminate cleanly.
    pgno = 1;
    pgft = CPAGE::pgftRockWrite;
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );

    // Attach with persistence enabled. Must be able to see saved state.
    pfm->SetPersisted( fTrue );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );

    // Attach with persistence disabled. Must not be able to see saved state.
    // Change the state.
    pgftPrev = pgft;
    pgft = CPAGE::PgftGetNextFlushType( pgft );
    pfm->SetPersisted( fFalse );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );

    // Attach with persistence enabled. Must be able to see previous state.
    pfm->SetPersisted( fTrue );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( pgft != pgftPrev );
    CHECK( pgftPrev == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );

    // However, file must still exist.
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, CreateNewMapGetsThrownOut )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    PGNO pgno = 0;
    CPAGE::PageFlushType pgft = CPAGE::pgftRockWrite;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Create flush map file.
    pfm->SetCreateNew( fFalse );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    // Set flush state and terminate cleanly.
    pgno = 1;
    pgft = CPAGE::pgftRockWrite;
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // Attach with create-new enabled. Must not be able to see saved state.
    pfm->SetCreateNew( fTrue );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // Attach with create-new disabled. Must not be able to see saved state
    // because the previous attachment should have cleared the map.
    pfm->SetCreateNew( fFalse );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, CreateNewMapDoesNotGetThrownOutIfReadOnly )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    PGNO pgno = 0;
    CPAGE::PageFlushType pgft = CPAGE::pgftRockWrite;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Create flush map file.
    pfm->SetCreateNew( fFalse );
    pfm->SetReadOnly( fFalse );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    // Set flush state and terminate cleanly.
    pgno = 1;
    pgft = CPAGE::pgftRockWrite;
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // Attach with create-new enabled, read-only. Must not be able to see saved state.
    pfm->SetCreateNew( fTrue );
    pfm->SetReadOnly( fTrue );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // Attach with create-new disabled. Must be able to see saved state
    // because the previous attachment was read-only so the map wasn't thrown out.
    pfm->SetCreateNew( fFalse );
    pfm->SetReadOnly( fFalse );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, BadPathDoesNotFailAttach )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L"c:\\d\\e\\f\\g\\h\\i\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    PGNO pgno = 0;
    CPAGE::PageFlushType pgft = CPAGE::pgftRockWrite;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Initialize flush map (invalid path).
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    // Set flush state and terminate cleanly. Flush map must not exist on disk.
    pgno = 1;
    pgft = CPAGE::pgftRockWrite;
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // Attach again. Must not be able to see saved state.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 2000000 ) );  // Should not cause an issue.
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, SetCapacityWhileAttachedWorks )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    QWORD cbFileSizeBefore = 0, cbFileSizeAfterGrow = 0, cbFileSizeAfterShrink = 0;
    PGNO pgno = 0;
    CPAGE::PageFlushType pgft = CPAGE::pgftRockWrite;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Create small file.
    pgno = 1;
    pgft = CPAGE::pgftRockWrite;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSizeBefore ) );

    // Grow map while attached.
    pgno += 100000;
    pgft = CPAGE::PgftGetNextFlushType( pgft );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgno ) );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );

    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSizeAfterGrow ) );
    CHECK( cbFileSizeAfterGrow > cbFileSizeBefore );

    // Shrink capacity while attached. File must not shrink
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    pgft = CPAGE::PgftGetNextFlushType( pgft );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 1 ) );
    pgft = CPAGE::PgftGetNextFlushType( pgft );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );

    CHECK( JET_errSuccess == ErrUtilGetLogicalFileSize( pfsapi, wszFmFilePath, &cbFileSizeAfterShrink ) );
    CHECK( cbFileSizeAfterShrink == cbFileSizeAfterGrow );

    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, DbHeaderFlushesAreCorrectlySynchronizedWithNoFm )
{
    SIGNATURE signDbHdrFlushFromDbBefore, signFlushMapHdrFlushFromDbBefore;
    SIGNATURE signDbHdrFlushFromDbAfter, signFlushMapHdrFlushFromDbAfter;

    // Initialize.
    SIGGetSignature( &signDbHdrFlushFromDbAfter );
    SIGGetSignature( &signFlushMapHdrFlushFromDbAfter );
    CHECK( FSIGSignSet( &signDbHdrFlushFromDbAfter ) );
    CHECK( FSIGSignSet( &signFlushMapHdrFlushFromDbAfter ) );
    
    // Copy to compare later.
    UtilMemCpy( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( signDbHdrFlushFromDbBefore ) );
    UtilMemCpy( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( signFlushMapHdrFlushFromDbBefore ) );

    // Enter, check, leave.
    CFlushMap::EnterDbHeaderFlush( NULL, &signDbHdrFlushFromDbAfter, &signFlushMapHdrFlushFromDbAfter );
    CHECK( 0 != memcmp( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( 0 != memcmp( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( FSIGSignSet( &signDbHdrFlushFromDbAfter ) );
    CHECK( !FSIGSignSet( &signFlushMapHdrFlushFromDbAfter ) );
    CFlushMap::LeaveDbHeaderFlush( NULL );

    // One more time.
    UtilMemCpy( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( signDbHdrFlushFromDbBefore ) );
    UtilMemCpy( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( signFlushMapHdrFlushFromDbBefore ) );
    CFlushMap::EnterDbHeaderFlush( NULL, &signDbHdrFlushFromDbAfter, &signFlushMapHdrFlushFromDbAfter );
    CHECK( 0 != memcmp( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( 0 == memcmp( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( FSIGSignSet( &signDbHdrFlushFromDbAfter ) );
    CHECK( !FSIGSignSet( &signFlushMapHdrFlushFromDbAfter ) );
    CFlushMap::LeaveDbHeaderFlush( NULL );
}

JETUNITTEST( CFlushMap, DbAndFmHeaderFlushesAreCorrectlySynchronized )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDbBefore, signFlushMapHdrFlushFromDbBefore;
    SIGNATURE signDbHdrFlushFromDbAfter, signFlushMapHdrFlushFromDbAfter;

    // Initialize.
    SIGGetSignature( &signDbHdrFlushFromDbAfter );
    SIGGetSignature( &signFlushMapHdrFlushFromDbAfter );
    CHECK( FSIGSignSet( &signDbHdrFlushFromDbAfter ) );
    CHECK( FSIGSignSet( &signFlushMapHdrFlushFromDbAfter ) );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetRecoverable( fTrue );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDbBefore );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDbBefore );

    // Init must change flush map header.
    UtilMemCpy( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( signDbHdrFlushFromDbBefore ) );
    UtilMemCpy( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( signFlushMapHdrFlushFromDbBefore ) );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CFlushMap::EnterDbHeaderFlush( pfm, &signDbHdrFlushFromDbAfter, &signFlushMapHdrFlushFromDbAfter );
    CHECK( 0 != memcmp( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( 0 != memcmp( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( FSIGSignSet( &signDbHdrFlushFromDbAfter ) );
    CHECK( FSIGSignSet( &signFlushMapHdrFlushFromDbAfter ) );
    CFlushMap::LeaveDbHeaderFlush( pfm );

    // One more time. Only DB header signature must change as there were not FM header flushes in-between.
    UtilMemCpy( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( signDbHdrFlushFromDbBefore ) );
    UtilMemCpy( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( signFlushMapHdrFlushFromDbBefore ) );
    CFlushMap::EnterDbHeaderFlush( pfm, &signDbHdrFlushFromDbAfter, &signFlushMapHdrFlushFromDbAfter );
    CHECK( 0 != memcmp( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( 0 == memcmp( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( FSIGSignSet( &signDbHdrFlushFromDbAfter ) );
    CHECK( FSIGSignSet( &signFlushMapHdrFlushFromDbAfter ) );
    CFlushMap::LeaveDbHeaderFlush( pfm );

    // Clean termination must change map header.
    CleanAndTermFlushMap( pfm );

    // Init must change flush map header.
    UtilMemCpy( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( signDbHdrFlushFromDbBefore ) );
    UtilMemCpy( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( signFlushMapHdrFlushFromDbBefore ) );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CFlushMap::EnterDbHeaderFlush( pfm, &signDbHdrFlushFromDbAfter, &signFlushMapHdrFlushFromDbAfter );
    CHECK( 0 != memcmp( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( 0 != memcmp( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( FSIGSignSet( &signDbHdrFlushFromDbAfter ) );
    CHECK( FSIGSignSet( &signFlushMapHdrFlushFromDbAfter ) );
    CFlushMap::LeaveDbHeaderFlush( pfm );

    // Synchronously setting flush states must not change the map header.
    UtilMemCpy( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( signDbHdrFlushFromDbBefore ) );
    UtilMemCpy( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( signFlushMapHdrFlushFromDbBefore ) );
    CHECK( pfm->m_fmpgnoSectionFlushFirst == CFlushMap::s_fmpgnoUninit );
    CHECK( JET_errSuccess == pfm->ErrSetPgnoFlushTypeAndWait( 1, CPAGE::pgftRockWrite, dbtimeNil ) );
    CHECK( pfm->m_fmpgnoSectionFlushFirst == CFlushMap::s_fmpgnoUninit );
    CFlushMap::EnterDbHeaderFlush( pfm, &signDbHdrFlushFromDbAfter, &signFlushMapHdrFlushFromDbAfter );
    CHECK( 0 != memcmp( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( 0 == memcmp( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( FSIGSignSet( &signDbHdrFlushFromDbAfter ) );
    CHECK( FSIGSignSet( &signFlushMapHdrFlushFromDbAfter ) );
    CFlushMap::LeaveDbHeaderFlush( pfm );

    // Asynchronously setting flush states must not change the map header.
    UtilMemCpy( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( signDbHdrFlushFromDbBefore ) );
    UtilMemCpy( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( signFlushMapHdrFlushFromDbBefore ) );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftPaperWrite );
    CFlushMap::EnterDbHeaderFlush( pfm, &signDbHdrFlushFromDbAfter, &signFlushMapHdrFlushFromDbAfter );
    CHECK( 0 != memcmp( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( 0 == memcmp( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( FSIGSignSet( &signDbHdrFlushFromDbAfter ) );
    CHECK( FSIGSignSet( &signFlushMapHdrFlushFromDbAfter ) );
    CFlushMap::LeaveDbHeaderFlush( pfm );

    // Starting a section flush must flush the map header.
    UtilMemCpy( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( signDbHdrFlushFromDbBefore ) );
    UtilMemCpy( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( signFlushMapHdrFlushFromDbBefore ) );
    CHECK( pfm->m_fmpgnoSectionFlushFirst == CFlushMap::s_fmpgnoUninit );
    CHECK( pfm->m_fmpgnoSectionFlushLast == CFlushMap::s_fmpgnoUninit );
    CHECK( pfm->m_fmpgnoSectionFlushNext == CFlushMap::s_fmpgnoUninit );
    CHECK( !pfm->m_fSectionCheckpointHeaderWrite );
    pfm->FlushOneSection( 0 );
    CHECK( !pfm->m_fSectionCheckpointHeaderWrite );
    CFlushMap::EnterDbHeaderFlush( pfm, &signDbHdrFlushFromDbAfter, &signFlushMapHdrFlushFromDbAfter );
    CHECK( 0 != memcmp( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( 0 != memcmp( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( FSIGSignSet( &signDbHdrFlushFromDbAfter ) );
    CHECK( FSIGSignSet( &signFlushMapHdrFlushFromDbAfter ) );
    CFlushMap::LeaveDbHeaderFlush( pfm );

    // The previous flush only flushed the header.
    while ( !pfm->FRequestFmSectionWrite( 100, 0 ) );   // Wait until previous section completes, as FlushOneSection() returns immediately if a flush is happening.
    CHECK( pfm->m_fmpgnoSectionFlushFirst == CFlushMap::s_fmpgnoUninit );
    CHECK( pfm->m_fmpgnoSectionFlushLast == CFlushMap::s_fmpgnoUninit );
    CHECK( pfm->m_fmpgnoSectionFlushNext == 0 );
    UtilMemCpy( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( signDbHdrFlushFromDbBefore ) );
    UtilMemCpy( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( signFlushMapHdrFlushFromDbBefore ) );
    CHECK( !pfm->m_fSectionCheckpointHeaderWrite );
    pfm->FlushOneSection( 0 );
    CHECK( !pfm->m_fSectionCheckpointHeaderWrite );

    // The previous section must have been the last, so one more flush would flush the map header.
    while ( !pfm->FRequestFmSectionWrite( 100, 0 ) );   // Wait until previous section completes, as FlushOneSection() returns immediately if a flush is happening.
    CHECK( !pfm->m_fSectionCheckpointHeaderWrite );
    CHECK( pfm->m_fmpgnoSectionFlushFirst == CFlushMap::s_fmpgnoUninit );
    CHECK( pfm->m_fmpgnoSectionFlushLast == CFlushMap::s_fmpgnoUninit );
    CHECK( pfm->m_fmpgnoSectionFlushNext == 1 );
    pfm->FlushOneSection( 0 );
    CFlushMap::EnterDbHeaderFlush( pfm, &signDbHdrFlushFromDbAfter, &signFlushMapHdrFlushFromDbAfter );
    CHECK( 0 != memcmp( &signDbHdrFlushFromDbBefore, &signDbHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( 0 != memcmp( &signFlushMapHdrFlushFromDbBefore, &signFlushMapHdrFlushFromDbAfter, sizeof( SIGNATURE ) ) );
    CHECK( FSIGSignSet( &signDbHdrFlushFromDbAfter ) );
    CHECK( FSIGSignSet( &signFlushMapHdrFlushFromDbAfter ) );
    CFlushMap::LeaveDbHeaderFlush( pfm );

    // Check final async flush progress.
    pfm->SetDbGenMinConsistent( 2 );
    while ( !pfm->FRequestFmSectionWrite( 100, 0 ) );   // Wait until previous section completes, as FlushOneSection() returns immediately if a flush is happening.
    CHECK( pfm->m_fmpgnoSectionFlushFirst == CFlushMap::s_fmpgnoUninit );
    CHECK( pfm->m_fmpgnoSectionFlushLast == CFlushMap::s_fmpgnoUninit );
    CHECK( pfm->m_fmpgnoSectionFlushNext == CFlushMap::s_fmpgnoUninit );

    pfm->TermFlushMap();

    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, FRecentlyAsyncWrittenHeaderIsProperlyMaintained )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlush, signFlushMapHdrFlush;

    // Initialize.
    SIGGetSignature( &signDbHdrFlush );
    SIGGetSignature( &signFlushMapHdrFlush );
    CHECK( FSIGSignSet( &signDbHdrFlush ) );
    CHECK( FSIGSignSet( &signFlushMapHdrFlush ) );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetRecoverable( fTrue );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlush );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlush );

    // Init must not set the flag.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( !pfm->m_fmdFmHdr.FRecentlyAsyncWrittenHeader() );
    while ( !pfm->FRequestFmSectionWrite( 100, 0 ) );
    CHECK( !pfm->m_fmdFmHdr.FRecentlyAsyncWrittenHeader() );

    pfm->SetPgnoFlushType( 1, CPAGE::pgftPaperWrite );

    // First flush must set the flag.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 100, 0 ) );
    CHECK( pfm->m_fmdFmHdr.FRecentlyAsyncWrittenHeader() );

    // Reset by entering DB header flush.
    CFlushMap::EnterDbHeaderFlush( pfm, &signDbHdrFlush, &signFlushMapHdrFlush );
    CHECK( !pfm->m_fmdFmHdr.FRecentlyAsyncWrittenHeader() );
    CFlushMap::LeaveDbHeaderFlush( pfm );
    CHECK( FSIGSignSet( &signDbHdrFlush ) );
    CHECK( FSIGSignSet( &signFlushMapHdrFlush ) );

    // Second flush must not set the flag.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 100, 0 ) );
    CHECK( !pfm->m_fmdFmHdr.FRecentlyAsyncWrittenHeader() );

    // Third flush must set the flag.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 100, 0 ) );
    CHECK( pfm->m_fmdFmHdr.FRecentlyAsyncWrittenHeader() );

    // Reset by querying min required.
    (void)pfm->LGetFmGenMinRequired();
    CHECK( !pfm->m_fmdFmHdr.FRecentlyAsyncWrittenHeader() );

    pfm->TermFlushMap();

    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, AsyncMapFlushCorrectlySignalsFlush )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;

    // Initialize.
    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetRecoverable( fTrue );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 2 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Non-persisted flush map should never require a flush.
    pfm->SetPersisted( fFalse );
    CHECK( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->TermFlushMap();

    // Uninitialized flush map should never require a flush.
    pfm->SetPersisted( fTrue );
    CHECK( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // Pinning this value. If s_pctCheckpointDelay changes, please revise this test.
    CHECK( 20 == CFlushMap::s_pctCheckpointDelay );

    // Start with FM min required @ 1.
    CHECK( 1 == pfm->LGetFmGenMinRequired() );

    // Log tip @ 0, preferred depth is 105 (should flush completely by 20% of that, which is 21), start flushing (header).
    CHECK( pfm->FRequestFmSectionWrite( 0, 105 ) );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // Cross first threshold (7).
    CHECK( !pfm->FRequestFmSectionWrite( 6, 105 ) );
    CHECK( pfm->FRequestFmSectionWrite( 7, 105 ) );
    pfm->FlushOneSection( 7 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // Cross second threshold (14). Completes a full flush.
    CHECK( !pfm->FRequestFmSectionWrite( 13, 105 ) );
    CHECK( pfm->FRequestFmSectionWrite( 14, 105 ) );
    pfm->FlushOneSection( 14 );
    while ( 1 == pfm->LGetFmGenMinRequired() );
    CHECK( 2 == pfm->LGetFmGenMinRequired() );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // This is a new flush. We need to dirty the page and move the min consistent.
    pfm->SetDbGenMinConsistent( 3 );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftRockWrite );
    CHECK( !pfm->FRequestFmSectionWrite( 20, 105 ) );
    CHECK( pfm->FRequestFmSectionWrite( 21, 105 ) );
    pfm->FlushOneSection( 21 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // Cross first threshold (28).
    CHECK( !pfm->FRequestFmSectionWrite( 27, 105 ) );
    CHECK( pfm->FRequestFmSectionWrite( 28, 105 ) );
    pfm->FlushOneSection( 28 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // Cross second threshold (35). Completes a full flush.
    CHECK( !pfm->FRequestFmSectionWrite( 34, 105 ) );
    CHECK( pfm->FRequestFmSectionWrite( 35, 105 ) );
    pfm->FlushOneSection( 35 );
    while ( 2 == pfm->LGetFmGenMinRequired() );
    CHECK( 3 == pfm->LGetFmGenMinRequired() );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // This is a new flush. We need to dirty the page and move the min consistent. Restart at a different position.
    pfm->SetDbGenMinConsistent( 4 );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftRockWrite );
    CHECK( !pfm->FRequestFmSectionWrite( 41, 105 ) );
    CHECK( pfm->FRequestFmSectionWrite( 42, 105 ) );
    pfm->FlushOneSection( 100 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // Cross first threshold (107). Flush at a different position.
    CHECK( !pfm->FRequestFmSectionWrite( 106, 105 ) );
    CHECK( pfm->FRequestFmSectionWrite( 107, 105 ) );
    pfm->FlushOneSection( 200 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // We should follow the previously initialized position. Completes a full flush.
    CHECK( !pfm->FRequestFmSectionWrite( 113, 105 ) );
    CHECK( pfm->FRequestFmSectionWrite( 114, 105 ) );
    pfm->FlushOneSection( 201 );
    while ( 3 == pfm->LGetFmGenMinRequired() );
    CHECK( 4 == pfm->LGetFmGenMinRequired() );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, AsyncMapFlushCorrectlySignalsFlushWithZeroedCheckpoint )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;

    // Initialize.
    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetRecoverable( fTrue );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 2 );
    pfm->SetPersisted( fTrue );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    // Pinning this value. If s_pctCheckpointDelay changes, please revise this test.
    CHECK( 20 == CFlushMap::s_pctCheckpointDelay );

    // Flush is always required at the beginning.
    CHECK( 1 == pfm->LGetFmGenMinRequired() );
    CHECK( pfm->FRequestFmSectionWrite( 0, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 0, 600 ) );
    CHECK( pfm->FRequestFmSectionWrite( 39, 600 ) );
    CHECK( pfm->FRequestFmSectionWrite( 40, 600 ) );
    CHECK( pfm->FRequestFmSectionWrite( 41, 600 ) );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // Cross first threshold (40).
    CHECK( !pfm->FRequestFmSectionWrite( 0, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 1, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 39, 0 ) );
    CHECK( !pfm->FRequestFmSectionWrite( 39, 600 ) );
    CHECK( pfm->FRequestFmSectionWrite( 40, 600 ) );
    pfm->FlushOneSection( 40 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // Cross second threshold (80). Completes a full flush.
    CHECK( !pfm->FRequestFmSectionWrite( 39, 0 ) );
    CHECK( !pfm->FRequestFmSectionWrite( 40, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 41, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 79, 0 ) );
    CHECK( !pfm->FRequestFmSectionWrite( 79, 600 ) );
    CHECK( pfm->FRequestFmSectionWrite( 80, 600 ) );
    pfm->FlushOneSection( 80 );
    while ( 1 == pfm->LGetFmGenMinRequired() );
    CHECK( 2 == pfm->LGetFmGenMinRequired() );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // This is a new flush. We need to dirty the page and move the min consistent.
    pfm->SetDbGenMinConsistent( 3 );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftRockWrite );
    CHECK( !pfm->FRequestFmSectionWrite( 0, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 1, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 119, 0 ) );
    CHECK( !pfm->FRequestFmSectionWrite( 119, 600 ) );
    CHECK( pfm->FRequestFmSectionWrite( 120, 600 ) );
    pfm->FlushOneSection( 120 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // Cross first threshold (160).
    CHECK( !pfm->FRequestFmSectionWrite( 119, 0 ) );
    CHECK( !pfm->FRequestFmSectionWrite( 120, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 121, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 159, 0 ) );
    CHECK( !pfm->FRequestFmSectionWrite( 159, 600 ) );
    CHECK( pfm->FRequestFmSectionWrite( 160, 600 ) );
    pfm->FlushOneSection( 160 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // Cross second threshold (200). Completes a full flush.
    CHECK( !pfm->FRequestFmSectionWrite( 159, 0 ) );
    CHECK( !pfm->FRequestFmSectionWrite( 160, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 161, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 199, 0 ) );
    CHECK( !pfm->FRequestFmSectionWrite( 199, 600 ) );
    CHECK( pfm->FRequestFmSectionWrite( 200, 600 ) );
    pfm->FlushOneSection( 200 );
    while ( 2 == pfm->LGetFmGenMinRequired() );
    CHECK( 3 == pfm->LGetFmGenMinRequired() );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // This is a new flush. We need to dirty the page and move the min consistent.
    pfm->SetDbGenMinConsistent( 4 );
    pfm->SetPgnoFlushType( 1, CPAGE::pgftRockWrite );
    CHECK( !pfm->FRequestFmSectionWrite( 119, 0 ) );
    CHECK( !pfm->FRequestFmSectionWrite( 120, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 121, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 239, 0 ) );
    CHECK( !pfm->FRequestFmSectionWrite( 239, 600 ) );
    CHECK( pfm->FRequestFmSectionWrite( 240, 600 ) );
    pfm->FlushOneSection( 240 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // Cross first threshold (280).
    CHECK( !pfm->FRequestFmSectionWrite( 239, 0 ) );
    CHECK( !pfm->FRequestFmSectionWrite( 240, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 241, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 279, 0 ) );
    CHECK( !pfm->FRequestFmSectionWrite( 279, 600 ) );
    CHECK( pfm->FRequestFmSectionWrite( 280, 600 ) );
    pfm->FlushOneSection( 280 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // Cross second threshold (320). Completes a full flush.
    CHECK( !pfm->FRequestFmSectionWrite( 279, 0 ) );
    CHECK( !pfm->FRequestFmSectionWrite( 280, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 281, 0 ) );
    CHECK( pfm->FRequestFmSectionWrite( 319, 0 ) );
    CHECK( !pfm->FRequestFmSectionWrite( 319, 600 ) );
    CHECK( pfm->FRequestFmSectionWrite( 320, 600 ) );
    pfm->FlushOneSection( 320 );
    while ( 3 == pfm->LGetFmGenMinRequired() );
    CHECK( 4 == pfm->LGetFmGenMinRequired() );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, AsyncMapFlushCorrectlyCalculatesNextMinRequired )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;

    // Initialize.
    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetRecoverable( fTrue );
    pfm->SetPersisted( fTrue );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 2 );
    pfm->SetDbGenMinConsistent( 3 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    // Start with FM min required @ 2.
    CHECK( 2 == pfm->LGetFmGenMinRequired() );

    // Flush full map, expect next min required to be 10.
    pfm->SetPgnoFlushType( 1, CPAGE::pgftRockWrite );
    pfm->SetDbGenMinConsistent( 10 );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    CHECK( 10 == pfm->LGetFmGenMinRequired() );

    // Flush full map, expect next min required to be 20.
    pfm->SetPgnoFlushType( 1, CPAGE::pgftPaperWrite );
    pfm->SetDbGenMinConsistent( 20 );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    CHECK( 20 == pfm->LGetFmGenMinRequired() );

    // Flush full map, expect next min required to be 25 (simulate crash/recovery).
    pfm->SetPgnoFlushType( 1, CPAGE::pgftScissorsWrite );
    pfm->SetDbGenMinConsistent( 25 );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    CHECK( 25 == pfm->LGetFmGenMinRequired() );

    // Flush full map, make sure min required doesn't go back (min consistent goes back).
    pfm->SetPgnoFlushType( 1, CPAGE::pgftRockWrite );
    pfm->SetDbGenMinConsistent( 21 );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    CHECK( 25 == pfm->LGetFmGenMinRequired() );

    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, CleanFlushMapBlocksFutureAsyncWriteRequests )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;

    // Initialize.
    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetRecoverable( fTrue );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );
    pfm->SetPersisted( fTrue );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    // Sanity check. Full flush 1.
    pfm->SetDbGenMinConsistent( 2 );
    CHECK( pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 1 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 2 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 3 );
    while ( 1 == pfm->LGetFmGenMinRequired() );
    CHECK( 2 == pfm->LGetFmGenMinRequired() );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // Sanity check. Full flush 2.
    pfm->SetDbGenMinConsistent( 3 );
    CHECK( pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 4 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 5 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 6 );
    while ( 2 == pfm->LGetFmGenMinRequired() );
    CHECK( 3 == pfm->LGetFmGenMinRequired() );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    // Full flush 3 gets blocked.
    pfm->SetDbGenMinConsistent( 4 );
    CHECK( pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    pfm->FlushOneSection( 7 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );
    CHECK( JET_errSuccess == pfm->ErrCleanFlushMap() );
    while ( 3 == pfm->LGetFmGenMinRequired() );
    CHECK( 0 == pfm->LGetFmGenMinRequired() );
    CHECK( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );

    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, SyncSetFlushTypePersistsState )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    CFlushMapForDump* pfmDump = new CFlushMapForDump();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    PGNO pgno = 0;
    CPAGE::PageFlushType pgft = CPAGE::pgftRockWrite;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    // Set flush state in six different flush map pages. Some of them
    // are not synchrounously flushed.

    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 6 * pfm->m_cDbPagesPerFlushMapPage - 1 ) );
    pgno = pfm->m_cDbPagesPerFlushMapPage - 1;
    pgft = CPAGE::pgftRockWrite;
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CHECK( JET_errSuccess == pfm->ErrSetPgnoFlushTypeAndWait( pgno, pgft, dbtimeNil ) );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    pgno += pfm->m_cDbPagesPerFlushMapPage;
    pgft = CPAGE::PgftGetNextFlushType( pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    pgno += pfm->m_cDbPagesPerFlushMapPage;
    pgft = CPAGE::PgftGetNextFlushType( pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CHECK( JET_errSuccess == pfm->ErrSetRangeFlushTypeAndWait( pgno, 2, pgft ) );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    pgno += ( 2 * pfm->m_cDbPagesPerFlushMapPage );
    pgft = CPAGE::PgftGetNextFlushType( pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetRangeFlushType( pgno, 2, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    // Terminate dirty.
    pfm->TermFlushMap();

    // Use a dump-mode object to probe for the page states.
    pfmDump->SetFmFilePath( wszFmFilePath );
    CHECK( JET_errSuccess == pfmDump->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( 0 ) );
    CHECK( JET_errPageNotInitialized == pfmDump->ErrChecksumFmPage( 1 ) );
    CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( 2 ) );
    CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( 3 ) );
    CHECK( JET_errPageNotInitialized == pfmDump->ErrChecksumFmPage( 4 ) );
    CHECK( JET_errPageNotInitialized == pfmDump->ErrChecksumFmPage( 5 ) );
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrChecksumFmPage( 6 ) );
    pfmDump->TermFlushMap();

    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
    delete pfmDump;
}

JETUNITTEST( CFlushMap, FlushSectionPersistsState )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    CFlushMapForDump* pfmDump = new CFlushMapForDump();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    PGNO pgno = 0;
    CPAGE::PageFlushType pgft = CPAGE::pgftRockWrite;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetRecoverable( fTrue );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    // Set flush state in three different flush map pages.

    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 3 * pfm->m_cDbPagesPerFlushMapPage - 1 ) );
    pgno = pfm->m_cDbPagesPerFlushMapPage - 1;
    pgft = CPAGE::pgftRockWrite;
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    pgno += pfm->m_cDbPagesPerFlushMapPage;
    pgft = CPAGE::PgftGetNextFlushType( pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    pgno += pfm->m_cDbPagesPerFlushMapPage;
    pgft = CPAGE::PgftGetNextFlushType( pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );

    // Flush the header and one section, which should cover all three pages.
    CHECK( ( UlParam( JET_paramMaxCoalesceWriteSize ) / CFlushMap::s_cbFlushMapPageOnDisk ) >= 3 );
    pfm->FlushOneSection( 0 );  // Header.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->FlushOneSection( 0 );  // First section.

    // Terminate dirty.
    pfm->TermFlushMap();

    // Use a dump-mode object to probe for the page states.
    pfmDump->SetFmFilePath( wszFmFilePath );
    CHECK( JET_errSuccess == pfmDump->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( 0 ) );
    CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( 1 ) );
    CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( 2 ) );
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrChecksumFmPage( 3 ) );
    pfmDump->TermFlushMap();

    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );
    delete pfsapi;
    delete pfm;
    delete pfmDump;
}

JETUNITTEST( CFlushMap, AsyncMapFlushHonorsMaxWriteSize )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    CFlushMapForDump* pfmDump = new CFlushMapForDump();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    FMPGNO fmpgno = 0, fmpgnoMax = 0;
    PGNO pgnoMaxDb = 0;
    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    const ULONG cbWriteSizeMaxOriginal = (ULONG)UlParam( JET_paramMaxCoalesceWriteSize );
    const ULONG cbWriteSizeGapMaxOriginal = (ULONG)UlParam( JET_paramMaxCoalesceWriteGapSize );
    const ULONG cbWriteSizeMaxNew = 10 * CFlushMap::s_cbFlushMapPageOnDisk;
    const ULONG cbWriteSizeGapMaxNew = 3 * CFlushMap::s_cbFlushMapPageOnDisk;

    // Override system parameters.
    Param( pinstNil, JET_paramMaxCoalesceWriteSize )->Set( pinstNil, ppibNil, cbWriteSizeMaxNew, NULL );
    Param( pinstNil, JET_paramMaxCoalesceWriteGapSize )->Set( pinstNil, ppibNil, cbWriteSizeGapMaxNew, NULL );
    CHECK( cbWriteSizeMaxNew == UlParam( JET_paramMaxCoalesceWriteSize ) );
    CHECK( cbWriteSizeGapMaxNew == UlParam( JET_paramMaxCoalesceWriteGapSize ) );
    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
    {
        public:
            CFileSystemConfiguration( ULONG cbMaxWriteSize )
            {
                m_cbMaxWriteSize = cbMaxWriteSize;
            }
    } fsconfig( cbWriteSizeMaxNew );

    const CFMPG cfmpgPerWrite = ( (CFMPG)cbWriteSizeMaxNew / CFlushMap::s_cbFlushMapPageOnDisk );

    // Use a dump-mode object to probe for the page states.
    pfmDump->SetFmFilePath( wszFmFilePath );

    CHECK( JET_errSuccess == ErrOSFSCreate( &fsconfig, &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFileSystemConfiguration( &fsconfig );
    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetRecoverable( fTrue );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Flush map with 1 page.
    fmpgnoMax = 0;
    pgnoMaxDb = 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    const CPG cpgPerFmPage = pfm->m_cDbPagesPerFlushMapPage;
    pfm->SetDbGenMinConsistent( 2 );
    pfm->FlushOneSection( 0 );  // Header
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->FlushOneSection( 0 );  // Data page.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfmDump->ErrInitFlushMap() );
    for ( fmpgno = 0; fmpgno <= fmpgnoMax; fmpgno++ )
    {
        CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( fmpgno ) );
    }
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrChecksumFmPage( fmpgno ) );
    pfmDump->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Flush map with half of the max write size.
    fmpgnoMax = cfmpgPerWrite / 2 - 1;
    pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    pfm->SetDbGenMinConsistent( 2 );
    pfm->FlushOneSection( 0 );  // Header
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->FlushOneSection( 0 );  // Data page.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfmDump->ErrInitFlushMap() );
    for ( fmpgno = 0; fmpgno <= fmpgnoMax; fmpgno++ )
    {
        CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( fmpgno ) );
    }
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrChecksumFmPage( fmpgno ) );
    pfmDump->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Flush map with one shy of the max write size.
    fmpgnoMax = cfmpgPerWrite - 1 - 1;
    pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    pfm->SetDbGenMinConsistent( 2 );
    pfm->FlushOneSection( 0 );  // Header
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->FlushOneSection( 0 );  // Data page.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfmDump->ErrInitFlushMap() );
    for ( fmpgno = 0; fmpgno <= fmpgnoMax; fmpgno++ )
    {
        CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( fmpgno ) );
    }
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrChecksumFmPage( fmpgno ) );
    pfmDump->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Flush map with one max write size chunk.
    fmpgnoMax = cfmpgPerWrite - 1;
    pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    pfm->SetDbGenMinConsistent( 2 );
    pfm->FlushOneSection( 0 );  // Header
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->FlushOneSection( 0 );  // Data page.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfmDump->ErrInitFlushMap() );
    for ( fmpgno = 0; fmpgno <= fmpgnoMax; fmpgno++ )
    {
        CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( fmpgno ) );
    }
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrChecksumFmPage( fmpgno ) );
    pfmDump->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Flush map with one more than max write size chunk.
    fmpgnoMax = cfmpgPerWrite + 1 - 1;
    pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    pfm->SetDbGenMinConsistent( 2 );
    pfm->FlushOneSection( 0 );  // Header
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->FlushOneSection( 0 );  // Data page.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfmDump->ErrInitFlushMap() );
    for ( fmpgno = 0; fmpgno <= ( cfmpgPerWrite - 1 ); fmpgno++ )
    {
        CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( fmpgno ) );
    }
    for (; fmpgno <= fmpgnoMax; fmpgno++ )
    {
        CHECK( JET_errPageNotInitialized == pfmDump->ErrChecksumFmPage( fmpgno ) );
    }
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrChecksumFmPage( fmpgno ) );
    pfmDump->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Flush map with 1 1/2 of the max write size chunk.
    fmpgnoMax = ( 3 * cfmpgPerWrite ) / 2 - 1;
    pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    pfm->SetDbGenMinConsistent( 2 );
    pfm->FlushOneSection( 0 );  // Header
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->FlushOneSection( 0 );  // Data page.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfmDump->ErrInitFlushMap() );
    for ( fmpgno = 0; fmpgno <= ( cfmpgPerWrite - 1 ); fmpgno++ )
    {
        CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( fmpgno ) );
    }
    for (; fmpgno <= fmpgnoMax; fmpgno++ )
    {
        CHECK( JET_errPageNotInitialized == pfmDump->ErrChecksumFmPage( fmpgno ) );
    }
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrChecksumFmPage( fmpgno ) );
    pfmDump->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Flush map with 2 1/2 of the max write size chunk.
    fmpgnoMax = ( 5 * cfmpgPerWrite ) / 2 - 1;
    pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    pfm->SetDbGenMinConsistent( 2 );
    pfm->FlushOneSection( 0 );  // Header
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->FlushOneSection( 0 );  // Data page.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfmDump->ErrInitFlushMap() );
    for ( fmpgno = 0; fmpgno <= ( cfmpgPerWrite - 1 ); fmpgno++ )
    {
        CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( fmpgno ) );
    }
    for (; fmpgno <= fmpgnoMax; fmpgno++ )
    {
        CHECK( JET_errPageNotInitialized == pfmDump->ErrChecksumFmPage( fmpgno ) );
    }
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrChecksumFmPage( fmpgno ) );
    pfmDump->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Flush map with 2 1/2 of the max write size chunk, two flushes.
    fmpgnoMax = ( 5 * cfmpgPerWrite ) / 2 - 1;
    pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    pfm->SetDbGenMinConsistent( 2 );
    pfm->FlushOneSection( 0 );  // Header
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->FlushOneSection( 0 );  // Data page.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->FlushOneSection( 0 );  // Data page
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfmDump->ErrInitFlushMap() );
    for ( fmpgno = 0; fmpgno <= ( 2 * cfmpgPerWrite - 1 ); fmpgno++ )
    {
        CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( fmpgno ) );
    }
    for (; fmpgno <= fmpgnoMax; fmpgno++ )
    {
        CHECK( JET_errPageNotInitialized == pfmDump->ErrChecksumFmPage( fmpgno ) );
    }
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrChecksumFmPage( fmpgno ) );
    pfmDump->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Flush map with 2 1/2 of the max write size chunk, four flushes.
    fmpgnoMax = ( 5 * cfmpgPerWrite ) / 2 - 1;
    pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    pfm->SetDbGenMinConsistent( 2 );
    pfm->FlushOneSection( 0 );  // Header
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->FlushOneSection( 0 );  // Data page.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->FlushOneSection( 0 );  // Data page
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->FlushOneSection( 0 );  // Data page
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    CHECK( 1 == pfm->LGetFmGenMinRequired() );
    pfm->FlushOneSection( 0 );  // Header (final).
    pfm->SetDbGenMinConsistent( 3 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    CHECK( 2 == pfm->LGetFmGenMinRequired() );
    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfmDump->ErrInitFlushMap() );
    for ( fmpgno = 0; fmpgno <= fmpgnoMax; fmpgno++ )
    {
        CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( fmpgno ) );
    }
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrChecksumFmPage( fmpgno ) );
    pfmDump->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Restore system parameters.
    Param( pinstNil, JET_paramMaxCoalesceWriteSize )->Set( pinstNil, ppibNil, cbWriteSizeMaxOriginal, NULL );
    Param( pinstNil, JET_paramMaxCoalesceWriteGapSize )->Set( pinstNil, ppibNil, cbWriteSizeGapMaxOriginal, NULL );
    CHECK( cbWriteSizeMaxOriginal == UlParam( JET_paramMaxCoalesceWriteSize ) );
    CHECK( cbWriteSizeGapMaxOriginal == UlParam( JET_paramMaxCoalesceWriteGapSize ) );

    delete pfsapi;
    delete pfm;
    delete pfmDump;
}

JETUNITTEST( CFlushMap, AsyncMapFlushCorrectlySkipsCleanPages )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );
    CPAGE::PageFlushType pgft = CPAGE::pgftUnknown;

    const ULONG cbWriteSizeMaxOriginal = (ULONG)UlParam( JET_paramMaxCoalesceWriteSize );
    const ULONG cbWriteSizeGapMaxOriginal = (ULONG)UlParam( JET_paramMaxCoalesceWriteGapSize );
    const ULONG cbWriteSizeMaxNew = 10 * CFlushMap::s_cbFlushMapPageOnDisk;
    const ULONG cbWriteSizeGapMaxNew = 3 * CFlushMap::s_cbFlushMapPageOnDisk;

    // Override system parameters.
    Param( pinstNil, JET_paramMaxCoalesceWriteSize )->Set( pinstNil, ppibNil, cbWriteSizeMaxNew, NULL );
    Param( pinstNil, JET_paramMaxCoalesceWriteGapSize )->Set( pinstNil, ppibNil, cbWriteSizeGapMaxNew, NULL );
    CHECK( cbWriteSizeMaxNew == UlParam( JET_paramMaxCoalesceWriteSize ) );
    CHECK( cbWriteSizeGapMaxNew == UlParam( JET_paramMaxCoalesceWriteGapSize ) );
    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
    {
        public:
            CFileSystemConfiguration( ULONG cbMaxWriteSize )
            {
                m_cbMaxWriteSize = cbMaxWriteSize;
            }
    } fsconfig( cbWriteSizeMaxNew );

    const CFMPG cfmpgPerWrite = ( (CFMPG)cbWriteSizeMaxNew / CFlushMap::s_cbFlushMapPageOnDisk );

    CHECK( JET_errSuccess == ErrOSFSCreate( &fsconfig, &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFileSystemConfiguration( &fsconfig );
    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetRecoverable( fTrue );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Flush map with 9 chunks of 10 FM pages each.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    const CPG cpgPerFmPage = pfm->m_cDbPagesPerFlushMapPage;
    const FMPGNO fmpgnoMax = 9 * cfmpgPerWrite - 1;
    const PGNO pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    CleanAndTermFlushMap( pfm );

    // Map is fully clean at this point.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( ( fmpgnoMax + 1 ) == pfm->m_cfmpgAllocated );
    pfm->SetDbGenMinConsistent( 2 );
    CHECK( 1 == pfm->LGetFmGenMinRequired() );
    pfm->FlushOneSection( 0 );  // Header.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 2 );
    CHECK( 1 == pfm->LGetFmGenMinRequired() );
    pfm->FlushOneSection( 0 );  // Rest of the map.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 2 );
    CHECK( 2 == pfm->LGetFmGenMinRequired() );

    // One more time.
    pfm->SetDbGenMinConsistent( 3 );
    CHECK( 2 == pfm->LGetFmGenMinRequired() );
    pfm->FlushOneSection( 0 );  // Header.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 3 );
    CHECK( 2 == pfm->LGetFmGenMinRequired() );
    pfm->FlushOneSection( 0 );  // Rest of the map.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 3 );
    CHECK( 3 == pfm->LGetFmGenMinRequired() );

    // Map is fully clean at this point. Dirty the following pages:
    //   o From half of the first chunk through half of the second chunk.
    //   o One page past half of the second chunk.
    //   o From beginning of the third chunk through half of the third chunk.
    //   o One page at the beginning of the fifth chunk.
    pgft = CPAGE::pgftUnknown;
    BOOL* const rgbFlushTypeSet = new BOOL[ pgnoMaxDb + 1 ];
    memset( rgbFlushTypeSet, 0, sizeof( BOOL ) * ( pgnoMaxDb + 1 ) );
    for ( FMPGNO fmpgno = ( cfmpgPerWrite / 2 ); fmpgno < ( ( 3 * cfmpgPerWrite ) / 2 ); fmpgno++ )
    {
        const PGNO pgno = fmpgno * cpgPerFmPage;
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        pfm->SetPgnoFlushType( pgno, pgft );
        rgbFlushTypeSet[ pgno ] = fTrue;
    }
    for ( FMPGNO fmpgno = ( ( 3 * cfmpgPerWrite ) / 2 ); fmpgno <= ( ( 3 * cfmpgPerWrite ) / 2 ); fmpgno++ )
    {
        const PGNO pgno = fmpgno * cpgPerFmPage;
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        pfm->SetPgnoFlushType( pgno, pgft );
        rgbFlushTypeSet[ pgno ] = fTrue;
    }
    for ( FMPGNO fmpgno = ( 2 * cfmpgPerWrite ); fmpgno < ( ( 5 * cfmpgPerWrite ) / 2 ); fmpgno++ )
    {
        const PGNO pgno = fmpgno * cpgPerFmPage;
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        pfm->SetPgnoFlushType( pgno, pgft );
        rgbFlushTypeSet[ pgno ] = fTrue;
    }
    for ( FMPGNO fmpgno = ( 4 * cfmpgPerWrite ); fmpgno <= ( 4 * cfmpgPerWrite ); fmpgno++ )
    {
        const PGNO pgno = fmpgno * cpgPerFmPage;
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        pfm->SetPgnoFlushType( pgno, pgft );
        rgbFlushTypeSet[ pgno ] = fTrue;
    }

    // Flush header.
    CHECK( CFlushMap::s_fmpgnoUninit == pfm->m_fmpgnoSectionFlushNext );
    pfm->SetDbGenMinConsistent( 4 );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 4 );

    // First flush: from half of the first chunk through half of the second chunk.
    CHECK( 0 == pfm->m_fmpgnoSectionFlushNext );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 4 );
    CHECK( ( ( 3 * cfmpgPerWrite ) / 2 ) == pfm->m_fmpgnoSectionFlushNext );
    
    // Second flush: one page past half of the second chunk.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 4 );
    CHECK( ( ( 3 * cfmpgPerWrite ) / 2 + 1 ) == pfm->m_fmpgnoSectionFlushNext );

    // Third flush: from beginning of the third chunk through half of the third chunk.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 4 );
    CHECK( ( ( 5 * cfmpgPerWrite ) / 2 ) == pfm->m_fmpgnoSectionFlushNext );

    // Fourth flush: one page at the beginning of the fifth chunk.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 4 );
    CHECK( ( 4 * cfmpgPerWrite + 1 ) == pfm->m_fmpgnoSectionFlushNext );

    // Final flush.
    CHECK( 3 == pfm->LGetFmGenMinRequired() );
    pfm->FlushOneSection( 0 );  // Flushes the entire map.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 4 );
    CHECK( 4 == pfm->LGetFmGenMinRequired() );

    // One more full clean.
    pfm->SetDbGenMinConsistent( 5 );
    CHECK( 4 == pfm->LGetFmGenMinRequired() );
    pfm->FlushOneSection( 0 );  // Header.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 5 );
    CHECK( 4 == pfm->LGetFmGenMinRequired() );
    pfm->FlushOneSection( 0 );  // Rest of the map.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 5 );
    CHECK( 5 == pfm->LGetFmGenMinRequired() );

    // Terminate cleanly and check.
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    pgft = CPAGE::pgftUnknown;
    for ( PGNO pgno = 0; pgno <= pgnoMaxDb; pgno++ )
    {
        if ( rgbFlushTypeSet[ pgno ] )
        {
            pgft = CPAGE::PgftGetNextFlushType( pgft );
            CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
        }
        else
        {
            CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
        }
    }

    // Cleanup.
    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Restore system parameters.
    Param( pinstNil, JET_paramMaxCoalesceWriteSize )->Set( pinstNil, ppibNil, cbWriteSizeMaxOriginal, NULL );
    Param( pinstNil, JET_paramMaxCoalesceWriteGapSize )->Set( pinstNil, ppibNil, cbWriteSizeGapMaxOriginal, NULL );
    CHECK( cbWriteSizeMaxOriginal == UlParam( JET_paramMaxCoalesceWriteSize ) );
    CHECK( cbWriteSizeGapMaxOriginal == UlParam( JET_paramMaxCoalesceWriteGapSize ) );

    delete[] rgbFlushTypeSet;
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, AsyncMapFlushHonorsMaxWriteGapSize )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );
    CPAGE::PageFlushType pgft = CPAGE::pgftUnknown;

    const ULONG cbWriteSizeMaxOriginal = (ULONG)UlParam( JET_paramMaxCoalesceWriteSize );
    const ULONG cbWriteSizeGapMaxOriginal = (ULONG)UlParam( JET_paramMaxCoalesceWriteGapSize );
    const ULONG cbWriteSizeMaxNew = 10 * CFlushMap::s_cbFlushMapPageOnDisk;
    const ULONG cbWriteSizeGapMaxNew = 3 * CFlushMap::s_cbFlushMapPageOnDisk;

    // Override system parameters.
    Param( pinstNil, JET_paramMaxCoalesceWriteSize )->Set( pinstNil, ppibNil, cbWriteSizeMaxNew, NULL );
    Param( pinstNil, JET_paramMaxCoalesceWriteGapSize )->Set( pinstNil, ppibNil, cbWriteSizeGapMaxNew, NULL );
    CHECK( cbWriteSizeMaxNew == UlParam( JET_paramMaxCoalesceWriteSize ) );
    CHECK( cbWriteSizeGapMaxNew == UlParam( JET_paramMaxCoalesceWriteGapSize ) );
    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
    {
        public:
            CFileSystemConfiguration( ULONG cbMaxWriteSize )
            {
                m_cbMaxWriteSize = cbMaxWriteSize;
            }
    } fsconfig( cbWriteSizeMaxNew );

    const CFMPG cfmpgPerWrite = ( (CFMPG)cbWriteSizeMaxNew / CFlushMap::s_cbFlushMapPageOnDisk );

    CHECK( JET_errSuccess == ErrOSFSCreate( &fsconfig, &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFileSystemConfiguration( &fsconfig );
    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetRecoverable( fTrue );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Flush map with 9 chunks of 10 FM pages each.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    const CPG cpgPerFmPage = pfm->m_cDbPagesPerFlushMapPage;
    const FMPGNO fmpgnoMax = 9 * cfmpgPerWrite - 1;
    const PGNO pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    CleanAndTermFlushMap( pfm );

    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( ( fmpgnoMax + 1 ) == pfm->m_cfmpgAllocated );

    // Map is fully clean at this point. Dirty the following pages:
    //   o From half of the first chunk through the end of the first chunk.
    //   o Entire second chunk, except for its first page.
    //   o Alternate pages on third chunk.
    //   o Second half of the fourth chunk.
    //   o Pages 0 and 1 of the sixth chunk.
    //   o Pages 6 and 8 of the sixth chunk.
    //   o Pages 3 and 6 of the seventh chunk.
    //   o Pages 1 and 5 of the eighth chunk.
    //   o Pages 0 and 5 of the ninth chunk.
    pgft = CPAGE::pgftUnknown;
    BOOL* const rgbFlushTypeSet = new BOOL[ pgnoMaxDb + 1 ];
    memset( rgbFlushTypeSet, 0, sizeof( BOOL ) * ( pgnoMaxDb + 1 ) );
    for ( FMPGNO fmpgno = ( cfmpgPerWrite / 2 ); fmpgno < cfmpgPerWrite; fmpgno++ )
    {
        const PGNO pgno = fmpgno * cpgPerFmPage;
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        pfm->SetPgnoFlushType( pgno, pgft );
        rgbFlushTypeSet[ pgno ] = fTrue;
    }
    for ( FMPGNO fmpgno = ( cfmpgPerWrite + 1 ); fmpgno < ( 2 * cfmpgPerWrite ); fmpgno++ )
    {
        const PGNO pgno = fmpgno * cpgPerFmPage;
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        pfm->SetPgnoFlushType( pgno, pgft );
        rgbFlushTypeSet[ pgno ] = fTrue;
    }
    for ( FMPGNO fmpgno = ( 2 * cfmpgPerWrite ); fmpgno < ( 3 * cfmpgPerWrite ); fmpgno += 2 )
    {
        const PGNO pgno = fmpgno * cpgPerFmPage;
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        pfm->SetPgnoFlushType( pgno, pgft );
        rgbFlushTypeSet[ pgno ] = fTrue;
    }
    for ( FMPGNO fmpgno = ( ( 7 * cfmpgPerWrite ) / 2 ); fmpgno < ( 4 * cfmpgPerWrite ); fmpgno++ )
    {
        const PGNO pgno = fmpgno * cpgPerFmPage;
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        pfm->SetPgnoFlushType( pgno, pgft );
        rgbFlushTypeSet[ pgno ] = fTrue;
    }
    for ( FMPGNO fmpgno = 0; fmpgno <= 9; fmpgno++ )
    {
        if ( ( fmpgno == 0 ) || ( fmpgno == 1 ) || ( fmpgno == 6 ) || ( fmpgno == 8 ) )
        {
            const PGNO pgno = ( fmpgno + 5 * cfmpgPerWrite ) * cpgPerFmPage;
            pgft = CPAGE::PgftGetNextFlushType( pgft );
            pfm->SetPgnoFlushType( pgno, pgft );
            rgbFlushTypeSet[ pgno ] = fTrue;
        }
    }
    for ( FMPGNO fmpgno = 0; fmpgno <= 9; fmpgno++ )
    {
        if ( ( fmpgno == 3 ) || ( fmpgno == 6 ) )
        {
            const PGNO pgno = ( fmpgno + 6 * cfmpgPerWrite ) * cpgPerFmPage;
            pgft = CPAGE::PgftGetNextFlushType( pgft );
            pfm->SetPgnoFlushType( pgno, pgft );
            rgbFlushTypeSet[ pgno ] = fTrue;
        }
    }
    for ( FMPGNO fmpgno = 0; fmpgno <= 9; fmpgno++ )
    {
        if ( ( fmpgno == 1 ) || ( fmpgno == 5 ) )
        {
            const PGNO pgno = ( fmpgno + 7 * cfmpgPerWrite ) * cpgPerFmPage;
            pgft = CPAGE::PgftGetNextFlushType( pgft );
            pfm->SetPgnoFlushType( pgno, pgft );
            rgbFlushTypeSet[ pgno ] = fTrue;
        }
    }
    for ( FMPGNO fmpgno = 0; fmpgno <= 9; fmpgno++ )
    {
        if ( ( fmpgno == 0 ) || ( fmpgno == 5 ) )
        {
            const PGNO pgno = ( fmpgno + 8 * cfmpgPerWrite ) * cpgPerFmPage;
            pgft = CPAGE::PgftGetNextFlushType( pgft );
            pfm->SetPgnoFlushType( pgno, pgft );
            rgbFlushTypeSet[ pgno ] = fTrue;
        }
    }

    // Flush header.
    CHECK( CFlushMap::s_fmpgnoUninit == pfm->m_fmpgnoSectionFlushNext );
    pfm->SetDbGenMinConsistent( 2 );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 2 );

    // First flush: from half of the first chunk through half of the second chunk.
    CHECK( 0 == pfm->m_fmpgnoSectionFlushNext );
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 3 );
    CHECK( ( ( 3 * cfmpgPerWrite ) / 2 ) == pfm->m_fmpgnoSectionFlushNext );
    
    // Second flush: from half of the second chunk through half of the third chunk.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 4 );
    CHECK( ( ( 5 * cfmpgPerWrite ) / 2 ) == pfm->m_fmpgnoSectionFlushNext );

    // Third flush: second half of the third chunk.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 5 );
    CHECK( ( 3 * cfmpgPerWrite - 1 ) == pfm->m_fmpgnoSectionFlushNext );

    // Fourth flush: second half of the fourth chunk.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 6 );
    CHECK( ( 4 * cfmpgPerWrite ) == pfm->m_fmpgnoSectionFlushNext );

    // Fifth flush: pages 0 through 1 of the sixth chunk.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 7 );
    CHECK( ( 5 * cfmpgPerWrite + 2 ) == pfm->m_fmpgnoSectionFlushNext );

    // Sixth flush: pages 6 through 8 of the sixth chunk.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 8 );
    CHECK( ( 5 * cfmpgPerWrite + 9 ) == pfm->m_fmpgnoSectionFlushNext );

    // Seventh flush: pages 3 through 6 of the seventh chunk.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 9 );
    CHECK( ( 6 * cfmpgPerWrite + 7 ) == pfm->m_fmpgnoSectionFlushNext );

    // Eighth flush: pages 1 through 5 of the eighth chunk.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 10 );
    CHECK( ( 7 * cfmpgPerWrite + 6 ) == pfm->m_fmpgnoSectionFlushNext );

    // Ninth flush: page 0 of the ninth chunk.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 11 );
    CHECK( ( 8 * cfmpgPerWrite + 1 ) == pfm->m_fmpgnoSectionFlushNext );

    // Tenth flush: page 5 of the ninth chunk.
    pfm->FlushOneSection( 0 );
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 12 );
    CHECK( ( 8 * cfmpgPerWrite + 6 ) == pfm->m_fmpgnoSectionFlushNext );

    // Final flush.
    CHECK( 1 == pfm->LGetFmGenMinRequired() );
    pfm->FlushOneSection( 0 );  // Flushes the entire map.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 12 );
    CHECK( 2 == pfm->LGetFmGenMinRequired() );

    // One more full clean.
    pfm->SetDbGenMinConsistent( 13 );
    CHECK( 2 == pfm->LGetFmGenMinRequired() );
    pfm->FlushOneSection( 0 );  // Header.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 13 );
    CHECK( 2 == pfm->LGetFmGenMinRequired() );
    pfm->FlushOneSection( 0 );  // Rest of the map.
    while ( !pfm->FRequestFmSectionWrite( 1000000, 0 ) );   // Wait until previous section flush completes.
    pfm->SetDbGenMinConsistent( 13 );
    CHECK( 13 == pfm->LGetFmGenMinRequired() );

    // Terminate cleanly and check.
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    pgft = CPAGE::pgftUnknown;
    for ( PGNO pgno = 0; pgno <= pgnoMaxDb; pgno++ )
    {
        if ( rgbFlushTypeSet[ pgno ] )
        {
            pgft = CPAGE::PgftGetNextFlushType( pgft );
            CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
        }
        else
        {
            CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
        }
    }

    // Cleanup.
    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Restore system parameters.
    Param( pinstNil, JET_paramMaxCoalesceWriteSize )->Set( pinstNil, ppibNil, cbWriteSizeMaxOriginal, NULL );
    Param( pinstNil, JET_paramMaxCoalesceWriteGapSize )->Set( pinstNil, ppibNil, cbWriteSizeGapMaxOriginal, NULL );
    CHECK( cbWriteSizeMaxOriginal == UlParam( JET_paramMaxCoalesceWriteSize ) );
    CHECK( cbWriteSizeGapMaxOriginal == UlParam( JET_paramMaxCoalesceWriteGapSize ) );

    delete[] rgbFlushTypeSet;
    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, AttachFlushMapLargerThanMaxReadSizeWorks )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;
    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );
    CPAGE::PageFlushType pgft = CPAGE::pgftUnknown, pgftInitial = CPAGE::pgftUnknown;
    FMPGNO fmpgnoMax = 0;
    PGNO pgno = 0, pgnoMaxDb = 0;

    const ULONG cbReadSizeMaxOriginal = (ULONG)UlParam( JET_paramMaxCoalesceReadSize );
    const ULONG cbReadSizeGapMaxOriginal = (ULONG)UlParam( JET_paramMaxCoalesceReadGapSize );
    const ULONG cbReadSizeMaxNew = 10 * CFlushMap::s_cbFlushMapPageOnDisk;
    const ULONG cbReadSizeGapMaxNew = 3 * CFlushMap::s_cbFlushMapPageOnDisk;

    // Override system parameters.
    Param( pinstNil, JET_paramMaxCoalesceReadSize )->Set( pinstNil, ppibNil, cbReadSizeMaxNew, NULL );
    Param( pinstNil, JET_paramMaxCoalesceReadGapSize )->Set( pinstNil, ppibNil, cbReadSizeGapMaxNew, NULL );
    CHECK( cbReadSizeMaxNew == UlParam( JET_paramMaxCoalesceReadSize ) );
    CHECK( cbReadSizeGapMaxNew == UlParam( JET_paramMaxCoalesceReadGapSize ) );
    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
    {
        public:
            CFileSystemConfiguration( ULONG cbMaxReadSize, ULONG cbMaxReadGapSize )
            {
                m_cbMaxReadSize = cbMaxReadSize;
                m_cbMaxReadGapSize = cbMaxReadGapSize;
            }
    } fsconfig( cbReadSizeMaxNew, cbReadSizeGapMaxNew );

    const CFMPG cfmpgPerRead = ( (CFMPG)cbReadSizeMaxNew / CFlushMap::s_cbFlushMapPageOnDisk );

    CHECK( JET_errSuccess == ErrOSFSCreate( &fsconfig, &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFileSystemConfiguration( &fsconfig );
    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Flush map with 1 page.
    fmpgnoMax = 1 - 1;
    pgnoMaxDb = 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    const CPG cpgPerFmPage = pfm->m_cDbPagesPerFlushMapPage;
    pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    pgftInitial = pgft;
    for ( pgno = 1; pgno <= pgnoMaxDb; pgno++ )
    {
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        pfm->SetPgnoFlushType( pgno, pgft );
    }
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    pgft = pgftInitial;
    for ( pgno = 1; pgno <= pgnoMaxDb; pgno++ )
    {
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    }
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );

    // Flush map with 1 page shy of max read size.
    fmpgnoMax = cfmpgPerRead - 1 - 1;
    pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    pgftInitial = pgft;
    for ( pgno = 1; pgno <= pgnoMaxDb; pgno++ )
    {
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        pfm->SetPgnoFlushType( pgno, pgft );
    }
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    pgft = pgftInitial;
    for ( pgno = 1; pgno <= pgnoMaxDb; pgno++ )
    {
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    }
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );

    // Flush map with max read size.
    fmpgnoMax = cfmpgPerRead - 1;
    pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    pgftInitial = pgft;
    for ( pgno = 1; pgno <= pgnoMaxDb; pgno++ )
    {
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        pfm->SetPgnoFlushType( pgno, pgft );
    }
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    pgft = pgftInitial;
    for ( pgno = 1; pgno <= pgnoMaxDb; pgno++ )
    {
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    }
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );

    // Flush map with max read size + 1.
    fmpgnoMax = cfmpgPerRead + 1 - 1;
    pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    pgftInitial = pgft;
    for ( pgno = 1; pgno <= pgnoMaxDb; pgno++ )
    {
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        pfm->SetPgnoFlushType( pgno, pgft );
    }
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    pgft = pgftInitial;
    for ( pgno = 1; pgno <= pgnoMaxDb; pgno++ )
    {
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    }
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );

    // Flush map with 1 1/2 the max read size.
    fmpgnoMax = ( ( 3 * cfmpgPerRead ) / 2 ) - 1;
    pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    pgftInitial = pgft;
    for ( pgno = 1; pgno <= pgnoMaxDb; pgno++ )
    {
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        pfm->SetPgnoFlushType( pgno, pgft );
    }
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    pgft = pgftInitial;
    for ( pgno = 1; pgno <= pgnoMaxDb; pgno++ )
    {
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    }
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );

    // Flush map with 2x the max read size.
    fmpgnoMax = 2 * cfmpgPerRead - 1;
    pgnoMaxDb = ( fmpgnoMax + 1 ) * cpgPerFmPage - 1;
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( pgnoMaxDb ) );
    pgftInitial = pgft;
    for ( pgno = 1; pgno <= pgnoMaxDb; pgno++ )
    {
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        pfm->SetPgnoFlushType( pgno, pgft );
    }
    CleanAndTermFlushMap( pfm );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    pgft = pgftInitial;
    for ( pgno = 1; pgno <= pgnoMaxDb; pgno++ )
    {
        pgft = CPAGE::PgftGetNextFlushType( pgft );
        CHECK( pgft == pfm->PgftGetPgnoFlushType( pgno ) );
    }
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    pfm->SetPgnoFlushType( pgno, pgft );
    CHECK( CPAGE::pgftUnknown == pfm->PgftGetPgnoFlushType( pgno ) );
    CleanAndTermFlushMap( pfm );

    // Cleanup.
    pfm->TermFlushMap();
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Restore system parameters.
    Param( pinstNil, JET_paramMaxCoalesceReadSize )->Set( pinstNil, ppibNil, cbReadSizeMaxOriginal, NULL );
    Param( pinstNil, JET_paramMaxCoalesceReadGapSize )->Set( pinstNil, ppibNil, cbReadSizeGapMaxOriginal, NULL );
    CHECK( cbReadSizeMaxOriginal == UlParam( JET_paramMaxCoalesceReadSize ) );
    CHECK( cbReadSizeGapMaxOriginal == UlParam( JET_paramMaxCoalesceReadGapSize ) );

    delete pfsapi;
    delete pfm;
}

JETUNITTEST( CFlushMap, DumpAndChecksumOfNonExistentFlushMapReturnsError )
{
    CFlushMapForDump* pfmDump = new CFlushMapForDump();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    pfmDump->SetFmFilePath( wszFmFilePath );
    CHECK( JET_errFileNotFound == pfmDump->ErrInitFlushMap() );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    CHECK( JET_errFileNotFound == CFlushMapForDump::ErrChecksumFlushMapFile( pinstNil, wszFmFilePath ) );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    CHECK( JET_errFileNotFound == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, 0, fTrue ) );
    CHECK( JET_errSuccess != ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    delete pfsapi;
    delete pfmDump;
}

JETUNITTEST( CFlushMap, DumpAndChecksumOfUncorruptedFlushMapWorks )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    CFlushMapForDump* pfmDump = new CFlushMapForDump();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Flush map with 3 pages. First and third have data.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    const CPG cpgPerFmPage = pfm->m_cDbPagesPerFlushMapPage;
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 3 * cpgPerFmPage - 1 ) );
    CHECK( JET_errSuccess == pfm->ErrSetPgnoFlushTypeAndWait( 0 * cpgPerFmPage, CPAGE::pgftRockWrite, dbtimeNil ) );
    CHECK( JET_errSuccess == pfm->ErrSetPgnoFlushTypeAndWait( 2 * cpgPerFmPage, CPAGE::pgftScissorsWrite, dbtimeNil ) );
    pfm->TermFlushMap();

    // Initialize dump/checksum object.
    pfmDump->SetFmFilePath( wszFmFilePath );
    CHECK( JET_errSuccess == pfmDump->ErrInitFlushMap() );

    // Dump with object.
    CHECK( JET_errInvalidParameter == pfmDump->ErrDumpFmPage( CFlushMap::s_fmpgnoUninit, fFalse ) );
    CHECK( JET_errSuccess == pfmDump->ErrDumpFmPage( CFlushMap::s_fmpgnoHdr, fTrue ) );
    CHECK( JET_errSuccess == pfmDump->ErrDumpFmPage( 0, fFalse ) );
    CHECK( JET_errSuccess == pfmDump->ErrDumpFmPage( 1, fTrue ) );
    CHECK( JET_errSuccess == pfmDump->ErrDumpFmPage( 2, fFalse ) );
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrDumpFmPage( 3, fTrue ) );

    // Checksum with object.
    CHECK( JET_errInvalidParameter == pfmDump->ErrChecksumFmPage( CFlushMap::s_fmpgnoUninit ) );
    CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( 0 ) );
    CHECK( JET_errPageNotInitialized == pfmDump->ErrChecksumFmPage( 1 ) );
    CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( 2 ) );
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrChecksumFmPage( 3 ) );

    // Free up file.
    pfmDump->TermFlushMap();

    // Dump with helper.
    CHECK( JET_errInvalidParameter == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, CFlushMap::s_fmpgnoUninit, fFalse ) );
    CHECK( JET_errSuccess == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, CFlushMap::s_fmpgnoHdr, fTrue ) );
    CHECK( JET_errSuccess == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, 0, fFalse ) );
    CHECK( JET_errSuccess == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, 1, fFalse ) );
    CHECK( JET_errSuccess == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, 2, fFalse ) );
    CHECK( JET_errFileIOBeyondEOF == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, 3, fTrue ) );

    // Checksum with helper.
    CHECK( JET_errSuccess == CFlushMapForDump::ErrChecksumFlushMapFile( pinstNil, wszFmFilePath ) );

    // Cleanup.
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    delete pfsapi;
    delete pfmDump;
    delete pfm;
}

JETUNITTEST( CFlushMap, DumpAndChecksumOfCorruptedFlushMapWorks )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    CFlushMapForDump* pfmDump = new CFlushMapForDump();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetReadOnly( fFalse );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Flush map with 3 pages. Second one is corrupted.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    const CPG cpgPerFmPage = pfm->m_cDbPagesPerFlushMapPage;
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 3 * cpgPerFmPage - 1 ) );
    CleanAndTermFlushMap( pfm );

    // Corrupt flush map.
    IFileAPI* pfapi = NULL;
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    const BYTE rgbCorruption[ CFlushMap::s_cbFlushMapPageOnDisk ] = { 12, 34, 56, 78 };
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        2 * CFlushMap::s_cbFlushMapPageOnDisk,
        sizeof( rgbCorruption ),
        rgbCorruption,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;

    // Initialize dump/checksum object.
    pfmDump->SetFmFilePath( wszFmFilePath );
    CHECK( JET_errSuccess == pfmDump->ErrInitFlushMap() );

    // Dump with object.
    CHECK( JET_errInvalidParameter == pfmDump->ErrDumpFmPage( CFlushMap::s_fmpgnoUninit, fFalse ) );
    CHECK( JET_errSuccess == pfmDump->ErrDumpFmPage( CFlushMap::s_fmpgnoHdr, fTrue ) );
    CHECK( JET_errSuccess == pfmDump->ErrDumpFmPage( 0, fFalse ) );
    CHECK( JET_errSuccess == pfmDump->ErrDumpFmPage( 1, fTrue ) );
    CHECK( JET_errSuccess == pfmDump->ErrDumpFmPage( 2, fFalse ) );
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrDumpFmPage( 3, fTrue ) );

    // Checksum with object.
    CHECK( JET_errInvalidParameter == pfmDump->ErrChecksumFmPage( CFlushMap::s_fmpgnoUninit ) );
    CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( 0 ) );
    CHECK( JET_errReadVerifyFailure == pfmDump->ErrChecksumFmPage( 1 ) );
    CHECK( JET_errSuccess == pfmDump->ErrChecksumFmPage( 2 ) );
    CHECK( JET_errFileIOBeyondEOF == pfmDump->ErrChecksumFmPage( 3 ) );

    // Free up file.
    pfmDump->TermFlushMap();

    // Dump with helper.
    CHECK( JET_errInvalidParameter == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, CFlushMap::s_fmpgnoUninit, fFalse ) );
    CHECK( JET_errSuccess == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, CFlushMap::s_fmpgnoHdr, fTrue ) );
    CHECK( JET_errSuccess == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, 0, fFalse ) );
    CHECK( JET_errSuccess == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, 1, fFalse ) );
    CHECK( JET_errSuccess == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, 2, fFalse ) );
    CHECK( JET_errFileIOBeyondEOF == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, 3, fTrue ) );

    // Checksum with helper.
    CHECK( JET_errReadVerifyFailure == CFlushMapForDump::ErrChecksumFlushMapFile( pinstNil, wszFmFilePath ) );

    // Cleanup.
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    delete pfsapi;
    delete pfmDump;
    delete pfm;
}

JETUNITTEST( CFlushMap, DumpAndChecksumOfCorruptedFlushMapDoesNotWorkIfHeaderCorrupt )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    CFlushMapForDump* pfmDump = new CFlushMapForDump();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Flush map with 3 pages. Header is corrupted.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    const CPG cpgPerFmPage = pfm->m_cDbPagesPerFlushMapPage;
    CHECK( JET_errSuccess == pfm->ErrSetFlushMapCapacity( 3 * cpgPerFmPage - 1 ) );
    CleanAndTermFlushMap( pfm );

    // Corrupt flush map.
    IFileAPI* pfapi = NULL;
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    const BYTE rgbCorruption[ CFlushMap::s_cbFlushMapPageOnDisk ] = { 12, 34, 56, 78 };
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        sizeof( rgbCorruption ),
        rgbCorruption,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;

    pfmDump->SetFmFilePath( wszFmFilePath );
    CHECK( JET_errReadVerifyFailure == pfmDump->ErrInitFlushMap() );

    CHECK( JET_errReadVerifyFailure == CFlushMapForDump::ErrChecksumFlushMapFile( pinstNil, wszFmFilePath ) );

    CHECK( JET_errReadVerifyFailure == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, 0, fTrue ) );

    // Cleanup.
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    delete pfsapi;
    delete pfmDump;
    delete pfm;
}

JETUNITTEST( CFlushMap, DumpAndChecksumDoesNotWorkIfFileLocked )
{
    CFlushMapForUnattachedDb* pfm = new CFlushMapForUnattachedDb();
    CFlushMapForDump* pfmDump = new CFlushMapForDump();
    IFileSystemAPI* pfsapi = NULL;
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    SIGNATURE signDbHdrFlushFromDb, signFlushMapHdrFlushFromDb;

    SIGGetSignature( &signDbHdrFlushFromDb );
    SIGGetSignature( &signFlushMapHdrFlushFromDb );

    // Override system parameters.
    const TICK dtickRetryOriginal = (TICK)UlParam( JET_paramAccessDeniedRetryPeriod );
    const TICK dtickRetryNew = 0;
    Param( pinstNil, JET_paramAccessDeniedRetryPeriod )->Set( pinstNil, ppibNil, dtickRetryNew, NULL );
    CHECK( dtickRetryNew == UlParam( JET_paramAccessDeniedRetryPeriod ) );
    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
    {
        public:
            CFileSystemConfiguration( ULONG dtickAccessDeniedRetryPeriod )
            {
                m_dtickAccessDeniedRetryPeriod = dtickAccessDeniedRetryPeriod;
            }
    } fsconfig( dtickRetryNew );

    CHECK( JET_errSuccess == ErrOSFSCreate( &fsconfig, &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    pfm->SetFileSystemConfiguration( &fsconfig );
    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( 1 );
    pfm->SetDbGenMinConsistent( 1 );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &signDbHdrFlushFromDb );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &signFlushMapHdrFlushFromDb );

    // Initialize and keep flush map locked.
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );

    pfmDump->SetFileSystemConfiguration( &fsconfig );
    pfmDump->SetFmFilePath( wszFmFilePath );
    CHECK( JET_errFileAccessDenied == pfmDump->ErrInitFlushMap() );

    CHECK( JET_errFileAccessDenied == CFlushMapForDump::ErrChecksumFlushMapFile( pinstNil, wszFmFilePath, &fsconfig ) );

    CHECK( JET_errFileAccessDenied == CFlushMapForDump::ErrDumpFlushMapPage( pinstNil, wszFmFilePath, 0, fTrue, &fsconfig ) );

    // Cleanup.
    delete pfm;
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    // Restore system parameters.
    Param( pinstNil, JET_paramAccessDeniedRetryPeriod )->Set( pinstNil, ppibNil, dtickRetryOriginal, NULL );
    CHECK( dtickRetryOriginal == UlParam( JET_paramAccessDeniedRetryPeriod ) );

    delete pfsapi;
    delete pfmDump;
}

JETUNITTEST( CFlushMap, VariousInvalidFlushMapConditionsAreCorrectlyDetected )
{
    CFlushMapForUnattachedDb* pfm = NULL;
    IFileSystemAPI* pfsapi = NULL;
    IFileAPI* pfapi = NULL;
    const WCHAR* const wszDbFilePath = L".\\flushmap.edb";
    const WCHAR* const wszFmFilePath = L".\\flushmap.jfm";
    const WCHAR* const wszDbFileBadPath = L".\\database.edb";
    const WCHAR* const wszDbFileBigPath = L"C:\\0008001200160020002400280032003600400044004800520056006000640068007200760080008400880092009601000104010801120116012001240128013201360140014401480152015601600164016801720176018001840188019201960200020402080212021602200224022802320236024002440248025202560260";
    const size_t cbFmHdr = CFlushMap::s_cbFlushMapPageOnDisk;
    // Use alloca to ensure proper alignment. (checksummming requires 16-byte alignment.)
    BYTE* rgbFmHdr = (BYTE*) alloca( cbFmHdr );
    memset( rgbFmHdr, 0, cbFmHdr );

    SIGNATURE signDbHdrFlushFromDbInitial, signFlushMapHdrFlushFromDbInitial;
    SIGGetSignature( &signDbHdrFlushFromDbInitial );
    SIGGetSignature( &signFlushMapHdrFlushFromDbInitial );
    CHECK( 0 != memcmp( &signDbHdrFlushFromDbInitial, &signFlushMapHdrFlushFromDbInitial, sizeof( SIGNATURE ) ) );

    DBFILEHDR dbhdr;
    memset( &dbhdr, 0, sizeof( dbhdr ) );
    dbhdr.le_lGenMinRequired = 10;
    dbhdr.le_lGenMinConsistent = 20;
    UtilMemCpy( &dbhdr.signDbHdrFlush, &signDbHdrFlushFromDbInitial, sizeof( dbhdr.signDbHdrFlush ) );
    UtilMemCpy( &dbhdr.signFlushMapHdrFlush, &signFlushMapHdrFlushFromDbInitial, sizeof( dbhdr.signFlushMapHdrFlush ) );

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    (void)pfsapi->ErrFileDelete( wszFmFilePath );

    // Create valid and clean flush map.
    pfm = new CFlushMapForUnattachedDb();
    pfm->SetFmFilePath( wszFmFilePath );
    pfm->SetDbState( JET_dbstateDirtyShutdown );
    pfm->SetDbGenMinRequired( dbhdr.le_lGenMinRequired );
    pfm->SetDbGenMinConsistent( dbhdr.le_lGenMinConsistent );
    pfm->SetDbHdrFlushSignaturePointerFromDb( &dbhdr.signDbHdrFlush );
    pfm->SetFmHdrFlushSignaturePointerFromDb( &dbhdr.signFlushMapHdrFlush );
    CHECK( JET_errSuccess == pfm->ErrInitFlushMap() );
    CleanAndTermFlushMap( pfm );
    delete pfm;
    pfm = NULL;
    CHECK( JET_errSuccess == ErrUtilPathExists( pfsapi, wszFmFilePath ) );

    // Retrieve a valid object. Twice.
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL != pfm );
    CleanAndTermFlushMap( pfm );
    delete pfm;
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL != pfm );
    CleanAndTermFlushMap( pfm );
    delete pfm;
    pfm = NULL;
    CHECK( 0 == memcmp( &dbhdr.signDbHdrFlush, &signDbHdrFlushFromDbInitial, sizeof( SIGNATURE ) ) );
    CHECK( 0 == memcmp( &dbhdr.signFlushMapHdrFlush, &signFlushMapHdrFlushFromDbInitial, sizeof( SIGNATURE ) ) );

    // Flush the DB header twice in-between.
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL != pfm );
    CFlushMap::EnterDbHeaderFlush( pfm, &dbhdr.signDbHdrFlush, &dbhdr.signFlushMapHdrFlush );
    CFlushMap::LeaveDbHeaderFlush( pfm );
    CFlushMap::EnterDbHeaderFlush( pfm, &dbhdr.signDbHdrFlush, &dbhdr.signFlushMapHdrFlush );
    CFlushMap::LeaveDbHeaderFlush( pfm );
    CleanAndTermFlushMap( pfm );
    delete pfm;
    pfm = NULL;
    CHECK( 0 != memcmp( &dbhdr.signDbHdrFlush, &signDbHdrFlushFromDbInitial, sizeof( SIGNATURE ) ) );
    CHECK( 0 != memcmp( &dbhdr.signFlushMapHdrFlush, &signFlushMapHdrFlushFromDbInitial, sizeof( SIGNATURE ) ) );
    UtilMemCpy( &signDbHdrFlushFromDbInitial, &dbhdr.signDbHdrFlush, sizeof( dbhdr.signDbHdrFlush ) );
    UtilMemCpy( &signFlushMapHdrFlushFromDbInitial, &dbhdr.signFlushMapHdrFlush, sizeof( dbhdr.signFlushMapHdrFlush ) );

    // Retrieve a valid object again.
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL != pfm );
    CleanAndTermFlushMap( pfm );
    delete pfm;
    pfm = NULL;
    CHECK( 0 == memcmp( &dbhdr.signDbHdrFlush, &signDbHdrFlushFromDbInitial, sizeof( SIGNATURE ) ) );
    CHECK( 0 == memcmp( &dbhdr.signFlushMapHdrFlush, &signFlushMapHdrFlushFromDbInitial, sizeof( SIGNATURE ) ) );

    // Take a snapshot of the valid flush map header.
    CFlushMap::FMFILEHDR fmhdr;
    CFlushMapForDump* pfmDump = new CFlushMapForDump();
    pfmDump->SetFmFilePath( wszFmFilePath );
    CHECK( JET_errSuccess == pfmDump->ErrInitFlushMap() );
    UtilMemCpy( &fmhdr, pfmDump->m_fmdFmHdr.pv, sizeof( fmhdr ) );
    delete pfmDump;
    pfmDump = NULL;

    // Bad DB/FM path.
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFileBadPath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL == pfm );

    // Big DB path.
    CHECK( JET_errInvalidPath == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFileBigPath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL == pfm );

    // Corrupted header checksum.
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->checksum = 12345678;
    CHECK( JET_errReadVerifyFailure == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL == pfm );

    // Invalid file type.
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->filetype = 12345678;
    CHECK( JET_errReadVerifyFailure == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SetPageChecksum( rgbFmHdr, CFlushMap::s_cbFlushMapPageOnDisk, flushmapHeaderPage, (ULONG)CFlushMap::s_fmpgnoHdr );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL == pfm );

    // Minor version mismatch.
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->le_ulFmVersionUpdateMajor++;
    CHECK( JET_errReadVerifyFailure == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SetPageChecksum( rgbFmHdr, CFlushMap::s_cbFlushMapPageOnDisk, flushmapHeaderPage, (ULONG)CFlushMap::s_fmpgnoHdr );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL == pfm );

    // Major version mismatch.
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->le_ulFmVersionMajor--;
    CHECK( JET_errReadVerifyFailure == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SetPageChecksum( rgbFmHdr, CFlushMap::s_cbFlushMapPageOnDisk, flushmapHeaderPage, (ULONG)CFlushMap::s_fmpgnoHdr );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL == pfm );

    // DB headers must not have changed all along.
    CHECK( 0 == memcmp( &dbhdr.signDbHdrFlush, &signDbHdrFlushFromDbInitial, sizeof( SIGNATURE ) ) );
    CHECK( 0 == memcmp( &dbhdr.signFlushMapHdrFlush, &signFlushMapHdrFlushFromDbInitial, sizeof( SIGNATURE ) ) );
    CHECK( FSIGSignSet( &signDbHdrFlushFromDbInitial ) );
    CHECK( FSIGSignSet( &signFlushMapHdrFlushFromDbInitial ) );

    // Matching signatures, all zeroed.
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SIGResetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signDbHdrFlush ) );
    SIGResetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signFlushMapHdrFlush ) );
    CHECK( JET_errReadVerifyFailure == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SetPageChecksum( rgbFmHdr, CFlushMap::s_cbFlushMapPageOnDisk, flushmapHeaderPage, (ULONG)CFlushMap::s_fmpgnoHdr );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    SIGResetSignature( &dbhdr.signDbHdrFlush );
    SIGResetSignature( &dbhdr.signFlushMapHdrFlush );
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL == pfm );

    // All signatures mismatched.
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SIGGetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signDbHdrFlush ) );
    SIGGetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signFlushMapHdrFlush ) );
    CHECK( JET_errReadVerifyFailure == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SetPageChecksum( rgbFmHdr, CFlushMap::s_cbFlushMapPageOnDisk, flushmapHeaderPage, (ULONG)CFlushMap::s_fmpgnoHdr );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    SIGGetSignature( &dbhdr.signDbHdrFlush );
    SIGGetSignature( &dbhdr.signFlushMapHdrFlush );
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL == pfm );

    // Mismatched + zeroed.
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SIGGetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signDbHdrFlush ) );
    SIGResetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signFlushMapHdrFlush ) );
    CHECK( JET_errReadVerifyFailure == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SetPageChecksum( rgbFmHdr, CFlushMap::s_cbFlushMapPageOnDisk, flushmapHeaderPage, (ULONG)CFlushMap::s_fmpgnoHdr );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    SIGGetSignature( &dbhdr.signDbHdrFlush );
    SIGResetSignature( &dbhdr.signFlushMapHdrFlush );
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL == pfm );

    // Zeroed + mismatched.
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SIGResetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signDbHdrFlush ) );
    SIGGetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signFlushMapHdrFlush ) );
    CHECK( JET_errReadVerifyFailure == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SetPageChecksum( rgbFmHdr, CFlushMap::s_cbFlushMapPageOnDisk, flushmapHeaderPage, (ULONG)CFlushMap::s_fmpgnoHdr );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    SIGResetSignature( &dbhdr.signDbHdrFlush );
    SIGGetSignature( &dbhdr.signFlushMapHdrFlush );
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL == pfm );

    // Matched + zeroed.
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SIGGetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signDbHdrFlush ) );
    SIGResetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signFlushMapHdrFlush ) );
    CHECK( JET_errReadVerifyFailure == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SetPageChecksum( rgbFmHdr, CFlushMap::s_cbFlushMapPageOnDisk, flushmapHeaderPage, (ULONG)CFlushMap::s_fmpgnoHdr );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    UtilMemCpy( &dbhdr.signDbHdrFlush, &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signDbHdrFlush ), sizeof( dbhdr.signDbHdrFlush ) );
    SIGResetSignature( &dbhdr.signFlushMapHdrFlush );
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL != pfm );
    pfm->TermFlushMap();
    delete pfm;
    pfm = NULL;

    // Zeroed + matched.
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SIGResetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signDbHdrFlush ) );
    SIGGetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signFlushMapHdrFlush ) );
    CHECK( JET_errReadVerifyFailure == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SetPageChecksum( rgbFmHdr, CFlushMap::s_cbFlushMapPageOnDisk, flushmapHeaderPage, (ULONG)CFlushMap::s_fmpgnoHdr );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    SIGResetSignature( &dbhdr.signDbHdrFlush );
    UtilMemCpy( &dbhdr.signFlushMapHdrFlush, &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signFlushMapHdrFlush ), sizeof( dbhdr.signFlushMapHdrFlush ) );
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL != pfm );
    pfm->TermFlushMap();
    delete pfm;
    pfm = NULL;

    // Matched + mismatched.
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SIGGetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signDbHdrFlush ) );
    SIGGetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signFlushMapHdrFlush ) );
    CHECK( JET_errReadVerifyFailure == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SetPageChecksum( rgbFmHdr, CFlushMap::s_cbFlushMapPageOnDisk, flushmapHeaderPage, (ULONG)CFlushMap::s_fmpgnoHdr );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    UtilMemCpy( &dbhdr.signDbHdrFlush, &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signDbHdrFlush ), sizeof( dbhdr.signDbHdrFlush ) );
    SIGGetSignature( &dbhdr.signFlushMapHdrFlush );
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL != pfm );
    pfm->TermFlushMap();
    delete pfm;
    pfm = NULL;

    // Mismatched + matched.
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SIGGetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signDbHdrFlush ) );
    SIGGetSignature( &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signFlushMapHdrFlush ) );
    CHECK( JET_errReadVerifyFailure == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SetPageChecksum( rgbFmHdr, CFlushMap::s_cbFlushMapPageOnDisk, flushmapHeaderPage, (ULONG)CFlushMap::s_fmpgnoHdr );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    SIGGetSignature( &dbhdr.signDbHdrFlush );
    UtilMemCpy( &dbhdr.signFlushMapHdrFlush, &( ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->signFlushMapHdrFlush ), sizeof( dbhdr.signFlushMapHdrFlush ) );
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL != pfm );
    pfm->TermFlushMap();
    delete pfm;
    pfm = NULL;

    // Restore signatures.
    UtilMemCpy( &dbhdr.signDbHdrFlush, &signDbHdrFlushFromDbInitial, sizeof( dbhdr.signDbHdrFlush ) );
    UtilMemCpy( &dbhdr.signFlushMapHdrFlush, &signFlushMapHdrFlushFromDbInitial, sizeof( dbhdr.signFlushMapHdrFlush ) );

    // Restore good header and perform a successful attach.
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL != pfm );
    CleanAndTermFlushMap( pfm );
    delete pfm;
    pfm = NULL;

    // Generate dirty map.
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL != pfm );
    pfm->TermFlushMap();
    delete pfm;
    pfm = NULL;

    // Dirty recoverable: dbhdr.le_lGenMinRequired == fmhdr.lGenMinRequired.
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL != pfm );
    pfm->TermFlushMap();
    delete pfm;
    pfm = NULL;

    // Dirty recoverable: dbhdr.le_lGenMinRequired < fmhdr.lGenMinRequired.
    dbhdr.le_lGenMinRequired--;
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL != pfm );
    UtilMemCpy( &fmhdr, pfm->m_fmdFmHdr.pv, sizeof( fmhdr ) );  // Snapshot the header here.
    pfm->TermFlushMap();
    delete pfm;
    pfm = NULL;
    dbhdr.le_lGenMinRequired++;

    // Dirty unrecoverable: dbhdr.le_lGenMinRequired > fmhdr.lGenMinRequired.
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL == pfm );

    // Dirty unrecoverable: dbhdr.le_lGenMinRequired == 0.
    const LONG dbHdrlGenMinRequired = dbhdr.le_lGenMinRequired;
    dbhdr.le_lGenMinRequired = 0;
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL == pfm );
    dbhdr.le_lGenMinRequired = dbHdrlGenMinRequired;

    // Dirty unrecoverable: fmhdr.lGenMinRequired == 0.
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    ( (CFlushMap::FMFILEHDR*)rgbFmHdr )->lGenMinRequired = 0;
    CHECK( JET_errReadVerifyFailure == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    SetPageChecksum( rgbFmHdr, CFlushMap::s_cbFlushMapPageOnDisk, flushmapHeaderPage, (ULONG)CFlushMap::s_fmpgnoHdr );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL == pfm );

    // Dirty recoverable again.
    dbhdr.le_lGenMinRequired--;
    pfapi = NULL;
    memset( rgbFmHdr, 0, cbFmHdr );
    CHECK( JET_errSuccess == pfsapi->ErrFileOpen( wszFmFilePath, IFileAPI::fmfNone, &pfapi ) );
    UtilMemCpy( rgbFmHdr, &fmhdr, sizeof( fmhdr ) );
    CHECK( JET_errSuccess == CFlushMap::ErrChecksumFmPage_( rgbFmHdr, CFlushMap::s_fmpgnoHdr ) );
    CHECK( JET_errSuccess == pfapi->ErrIOWrite(
        *TraceContextScope( iorpFlushMap ),
        0,
        cbFmHdr,
        rgbFmHdr,
        qosIODispatchImmediate ) );
    pfapi->SetNoFlushNeeded();
    delete pfapi;
    pfapi = NULL;
    CHECK( JET_errSuccess == CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbFilePath, &dbhdr, pinstNil, &pfm ) );
    CHECK( NULL != pfm );
    CleanAndTermFlushMap( pfm );
    delete pfm;
    pfm = NULL;
    dbhdr.le_lGenMinRequired++;

    // DB headers must not have changed all along.
    CHECK( 0 == memcmp( &dbhdr.signDbHdrFlush, &signDbHdrFlushFromDbInitial, sizeof( SIGNATURE ) ) );
    CHECK( 0 == memcmp( &dbhdr.signFlushMapHdrFlush, &signFlushMapHdrFlushFromDbInitial, sizeof( SIGNATURE ) ) );

    // Cleanup.
    CHECK( JET_errSuccess == pfsapi->ErrFileDelete( wszFmFilePath ) );

    delete pfsapi;
}
