// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

LOCAL const CHAR szMSysScan[] = "MSysScan1";

//  ================================================================
BOOL FSCANSystemTable( const CHAR * const szTableName )
//  ================================================================
{
    return ( 0 == UtilCmpName( szTableName, szMSysScan ) );
}


#ifdef MINIMAL_FUNCTIONALITY

//  ================================================================
ERR ErrIsamDatabaseScan(
    const JET_SESID         vsesid,
    const JET_DBID          vdbid,
    __inout_opt ULONG * const   pcsec,
    const ULONG             cmsecSleep,
    const JET_CALLBACK      pfnCallback,
    const JET_GRBIT         grbit )
//  ================================================================
{
    return ErrERRCheck( JET_errDisabledFunctionality );
}

#else

LOCAL ERR ErrSCRUBGetObjidsFromCatalog(
        PIB * const ppib,
        const IFMP ifmp,
        const OBJIDINFO ** ppobjidinfo,
        LONG * pcobjidinfo );

#ifdef PERFMON_SUPPORT
extern PERFInstanceDelayedTotal<> perfctrDBMPagesZeroed;    // borrowed from DBMaint ...

PERFInstanceDelayedTotal<> cSCANPagesRead;

LONG LSCANPagesReadCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cSCANPagesRead.PassTo( iInstance, pvBuf );
    return 0;
}
#endif // PERFMON_SUPPORT

// All columns in MSysScan are JET_coltypLongLong

// Every column name in the next section should be placed in rgszMSysScanColumns
// We assert that the number of lines in the next section is equal to the number
// of entries in rgszMSysScanColumns so don't put any extra lines in.

static const INT clineFirstMSysScanColumnName = __LINE__;
LOCAL const CHAR szMsysScanPassUpdateDateTime[]         = "LastUpdateDateTime";
LOCAL const CHAR szMsysScanPassActive[]                 = "PassActive";
LOCAL const CHAR szMsysScanPassStartDateTime[]          = "PassStartDateTime";
LOCAL const CHAR szMsysScanPassElapsedSeconds[]         = "ElapsedSeconds";
LOCAL const CHAR szMsysScanPassInvocations[]            = "PassInvocations";
LOCAL const CHAR szMsysScanPassZeroingEnabled[]         = "PassZeroingEnabled";
LOCAL const CHAR szMsysScanPassPagesRead[]              = "PassPagesRead";
LOCAL const CHAR szMsysScanPassHighestDbtime[]          = "PassHighestDbtime";
LOCAL const CHAR szMsysScanPassBadChecksums[]           = "PassBadChecksums";
LOCAL const CHAR szMsysScanPassBlankPages[]             = "PassBlankPages";
LOCAL const CHAR szMsysScanPassZeroUnchangedPages[]     = "PassZeroUnchangedPages";
LOCAL const CHAR szMsysScanPassZeroUnusedPages[]        = "PassZeroUnusedPages";
LOCAL const CHAR szMsysScanPassZeroDeletedRecords[]     = "PassZeroDeletedRecords";
LOCAL const CHAR szMsysScanPassZeroOrphanedLVs[]        = "PassZeroOrphanedLVs";
LOCAL const CHAR szMsysScanPassZeroOrphanedLVChunks[]   = "PassZeroOrphanedLVChunks";
LOCAL const CHAR szMsysScanTotalScanDays[]              = "TotalScanDays";
LOCAL const CHAR szMsysScanTotalChecksumPasses[]        = "TotalChecksumPasses";
LOCAL const CHAR szMsysScanTotalZeroingPasses[]         = "TotalZeroingPasses";
LOCAL const CHAR szMsysScanTotalInvocations[]           = "TotalInvocations";
LOCAL const CHAR szMsysScanTotalPagesRead[]             = "TotalPagesRead";
LOCAL const CHAR szMsysScanTotalBadChecksums[]          = "TotalBadChecksums";
LOCAL const CHAR szMsysScanTotalBlankPages[]            = "TotalBlankPages";
LOCAL const CHAR szMsysScanTotalZeroUnchangedPages[]    = "TotalZeroUnchangedPages";
LOCAL const CHAR szMsysScanTotalZeroUnusedPages[]       = "TotalZeroUnusedPages";
LOCAL const CHAR szMsysScanTotalZeroDeletedRecords[]    = "TotalZeroDeletedRecords";
LOCAL const CHAR szMsysScanTotalZeroOrphanedLVs[]       = "TotalZeroOrphanedLVs";
LOCAL const CHAR szMsysScanTotalZeroOrphanedLVChunks[]  = "TotalZeroOrphanedLVChunks";
LOCAL const CHAR szMsysScanHighestDbtimeLastZero[]      = "HighestDbtimeLastZero";
LOCAL const CHAR szMsysScanHighestDbtimeLastChecksum[]  = "HighestDbtimeLastChecksum";
LOCAL const CHAR szMsysScanEndDateTimeLastZero[]        = "EndDateTimeLastZero";
LOCAL const CHAR szMsysScanEndDateTimeLastChecksum[]    = "EndDateTimeLastChecksum";
static const INT clineLastMSysScanColumnName = __LINE__;
static const INT clinesMSysScanColumnNames = clineLastMSysScanColumnName - ( clineFirstMSysScanColumnName + 1 );

LOCAL const CHAR * const rgszMSysScanColumns[] =
{
    szMsysScanPassUpdateDateTime,
    szMsysScanPassActive,
    szMsysScanPassStartDateTime,
    szMsysScanPassElapsedSeconds,
    szMsysScanPassInvocations,
    szMsysScanPassZeroingEnabled,
    szMsysScanPassPagesRead,
    szMsysScanPassHighestDbtime,
    szMsysScanPassBadChecksums,
    szMsysScanPassBlankPages,
    szMsysScanPassZeroUnchangedPages,
    szMsysScanPassZeroUnusedPages,
    szMsysScanPassZeroDeletedRecords,
    szMsysScanPassZeroOrphanedLVs,
    szMsysScanPassZeroOrphanedLVChunks,
    szMsysScanTotalScanDays,
    szMsysScanTotalChecksumPasses,
    szMsysScanTotalZeroingPasses,
    szMsysScanTotalInvocations,
    szMsysScanTotalPagesRead,
    szMsysScanTotalBadChecksums,
    szMsysScanTotalBlankPages,
    szMsysScanTotalZeroUnchangedPages,
    szMsysScanTotalZeroUnusedPages,
    szMsysScanTotalZeroDeletedRecords,
    szMsysScanTotalZeroOrphanedLVs,
    szMsysScanTotalZeroOrphanedLVChunks,
    szMsysScanHighestDbtimeLastZero,
    szMsysScanHighestDbtimeLastChecksum,
    szMsysScanEndDateTimeLastZero,
    szMsysScanEndDateTimeLastChecksum,
};
LOCAL const INT ccolMsysScan = _countof(rgszMSysScanColumns);


//  ================================================================
void DBMLogStartFailure( const IFMP ifmp, const ERR err )
//  ================================================================
{
    const INT       cszMax = 2;
    const WCHAR *   rgcwszT[cszMax];
    INT             isz = 0;
    
    rgcwszT[isz++] = g_rgfmp[ifmp].WszDatabaseName();

    WCHAR wszErr[32];
    OSStrCbFormatW ( wszErr, sizeof(wszErr), L"%d", err );
    rgcwszT[isz++] = wszErr;
    
    Assert( cszMax == isz );
    
    UtilReportEvent(
        eventError,
        ONLINE_DEFRAG_CATEGORY,
        DB_MAINTENANCE_START_FAILURE_ID,
        isz,
        rgcwszT,
        0,
        NULL,
        PinstFromIfmp( ifmp ));
}

JET_ERR JET_API NullCallback(
    _In_ JET_SESID,
    _In_ JET_DBID,
    _In_ JET_TABLEID,
    _In_ JET_CBTYP,
    __inout_opt void *,
    __inout_opt void *,
    __in_opt void *,
    _In_ JET_API_PTR)
{
    return JET_errSuccess;
}

//  ================================================================
ERR ErrIsamDatabaseScan(
    const JET_SESID         vsesid,
    const JET_DBID          vdbid,
    __inout_opt ULONG * const   pcsecMax,
    const ULONG             cmsecSleep,
    const JET_CALLBACK      pfnCallback,
    const JET_GRBIT         grbit )
//  ================================================================
{
    ERR             err     = JET_errSuccess;
    PIB * const     ppib    = (PIB *)vsesid;
    const IFMP      ifmp    = (IFMP)vdbid;

    const bool      fStartSinglePassScan    = !!(grbit & JET_bitDatabaseScanBatchStart);
    const bool      fStartContinuousScan    = !!(grbit & JET_bitDatabaseScanBatchStartContinuous);
    const bool      fStartScan              = fStartSinglePassScan || fStartContinuousScan;
    const bool      fStopScan               = !!(grbit & JET_bitDatabaseScanBatchStop);

    if( ( !fStartScan && !fStopScan ) ||
        ( fStartScan && fStopScan ) ||
        ( fStartScan && fStartSinglePassScan && fStartContinuousScan ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    // When starting continous DBM directly (as opposed to of implicitly during
    // log replay or database attach), the contract is that it will behave just
    // like continuous DBM started implicitly, i.e., callbacks and other fine-tune
    // parameters (such as throttling) will not be honored. Instead, the DBM engine
    // will consume the respective DBM-related JET_params.
    //
    // To try and catch cases of the user trying to set those, we'll fail this call
    // in case non-zeroed values are passed in.

    if ( fStartContinuousScan &&
        ( ( ( pcsecMax != NULL ) && ( *pcsecMax != 0 ) ) || ( cmsecSleep != 0 ) || ( pfnCallback != NULL ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Call( ErrPIBCheck( ppib ) );
    Call( ErrPIBCheckIfmp( ppib, ifmp ) );
    Call( ErrPIBCheckUpdatable( ppib ) );
    Call( ErrFaultInjection( 41109 ) );

    if ( fStartSinglePassScan )
    {
        const INT csec = (pcsecMax && *pcsecMax) ? *pcsecMax : INT_MAX;
        Call( g_rgfmp[ifmp].ErrStartDBMScanSinglePass(
            vsesid,
            csec,
            cmsecSleep,
            pfnCallback ? pfnCallback : NullCallback) );
    }
    else if ( fStartContinuousScan )
    {
        Call( g_rgfmp[ifmp].ErrStartDBMScan() );
    }
    else if( fStopScan )
    {
        g_rgfmp[ifmp].StopDBMScan();
    }
    
HandleError:
    if( ( err < JET_errSuccess ) && fStartScan )
    {
        DBMLogStartFailure( ifmp, err );
    }

    return err;
}


//  ================================================================
LOCAL COLUMNID ColumnidFromMSysScanColumnName( const CHAR * const szColumn )
//  ================================================================
{
    for( INT iColumn = 0; iColumn < ccolMsysScan; ++iColumn )
    {
        if( szColumn == rgszMSysScanColumns[iColumn] ||
            0 == UtilCmpName( szColumn, rgszMSysScanColumns[iColumn] ) )
        {
            return FID( fidtypFixed, (WORD)iColumn );
        }
    }
    AssertSz( fFalse, "Didn't find MSysScanColumn" );
    return FID( fidtypTagged, fidlimMost );
}


//  ================================================================
LOCAL ERR ErrSCANDumpOneMSysDefragColumn(
    _In_ PIB * const ppib,
    _In_ FUCB * const pfucb,
    const CHAR * const szColumn)
//  ================================================================
{
    ERR err;
    __int64 qw = 12345678;

    WCHAR * wszAlloc = NULL;

    const INT cchColumnName = 25;
    const COLUMNID columnid = ColumnidFromMSysScanColumnName( szColumn );

    wprintf( L"%*.*S: ", cchColumnName, cchColumnName, szColumn );
    
    ULONG cbActual;
    Call( ErrIsamRetrieveColumn(
        ppib,
        pfucb,
        columnid,
        &qw,
        sizeof( qw ),
        &cbActual,
        NO_GRBIT,
        NULL ) );
    CallS( err );
    Assert( sizeof( qw ) == cbActual );

    if( 0 != qw && NULL != strstr( szColumn, "DateTime" ) )
    {
        // Column name contains 'DateTime', format it as such
        size_t cchRequired;
        
        Call( ErrUtilFormatFileTimeAsDate(
            qw,
            0,
            0,
            &cchRequired) );
        Alloc( wszAlloc = new WCHAR[cchRequired] );
        Call( ErrUtilFormatFileTimeAsDate(
            qw,
            wszAlloc,
            cchRequired,
            &cchRequired) );
        wprintf( L"%.*s ", (ULONG)cchRequired, wszAlloc );
        delete [] wszAlloc;
        wszAlloc = NULL;

        Call( ErrUtilFormatFileTimeAsTime(
            qw,
            0,
            0,
            &cchRequired) );
        Alloc( wszAlloc = new WCHAR[cchRequired] );
        Call( ErrUtilFormatFileTimeAsTime(
            qw,
            wszAlloc,
            cchRequired,
            &cchRequired) );
        wprintf( L"%.*s (0x%I64x)\n", (ULONG)cchRequired, wszAlloc, qw );
        delete [] wszAlloc;
        wszAlloc = NULL;
    }
    else
    {
        wprintf( L"%I64d\n", qw );
    }

HandleError:
    delete [] wszAlloc;
    return err;
}


//  ================================================================
LOCAL ERR ErrSCANDumpMSysDefragColumns(
    _In_ PIB * const ppib,
    _In_ FUCB * const pfucb )
//  ================================================================
{
    ERR err = JET_errSuccess;

    for( INT iColumn = 0; iColumn < ccolMsysScan; ++iColumn )
    {
        Call( ErrSCANDumpOneMSysDefragColumn( ppib, pfucb, rgszMSysScanColumns[iColumn] ) );
    }

HandleError:
    return err;
}


//  ================================================================
ERR ErrSCANDumpMSysScan( _In_ PIB * const ppib, const IFMP ifmp )
//  ================================================================
{
    ERR err;
    FUCB * pfucbScan = pfucbNil;
    
    err = ErrFILEOpenTable( ppib, ifmp, &pfucbScan, szMSysScan );
    if ( JET_errObjectNotFound == err )
    {
        // no table to dump
        err = JET_errSuccess;
        goto HandleError;
    }
    Call( err );

    wprintf( L"********************************* MSysScan DUMP ************************************\n" );

    // move to the first (and only) record. the record should always exist as
    // it is created in the same transaction that creates the table
    
    err = ErrIsamMove( ppib, pfucbScan, JET_MoveFirst, NO_GRBIT );
    Assert( JET_errNoCurrentRecord != err );
    Assert( JET_errRecordNotFound != err );
    Call( err );
    Call( ErrSCANDumpMSysDefragColumns( ppib, pfucbScan ) );
    
    wprintf( L"\n" );
    
HandleError:
    if( pfucbNil != pfucbScan )
    {
        CallS( ErrFILECloseTable( ppib, pfucbScan ) );
    }
    return err;
}


//  ================================================================
LOCAL_BROKEN ERR ErrSCANCreateMSysScan( _In_ PIB * const ppib, const IFMP ifmp )
//  ================================================================
{
    C_ASSERT( ccolMsysScan == clinesMSysScanColumnNames );
    
    ERR err;
    FUCB *pfucb = pfucbNil;
    BOOL fInTransaction = fFalse;
    
    const __int64 zero = 0;
    
    JET_COLUMNCREATE_A rgjccMSysScan[ccolMsysScan];
    for( INT iColumn = 0; iColumn < ccolMsysScan; ++iColumn )
    {
        rgjccMSysScan[iColumn].cbStruct         = sizeof(JET_COLUMNCREATE_A);
        rgjccMSysScan[iColumn].szColumnName     = const_cast<CHAR *>(rgszMSysScanColumns[iColumn]);
        rgjccMSysScan[iColumn].coltyp           = JET_coltypLongLong;
        rgjccMSysScan[iColumn].cbMax            = 0;
        rgjccMSysScan[iColumn].grbit            = JET_bitColumnFixed;
        rgjccMSysScan[iColumn].pvDefault        = const_cast<__int64 *>( &zero );
        rgjccMSysScan[iColumn].cbDefault        = sizeof( zero );
        rgjccMSysScan[iColumn].cp               = 0;
        rgjccMSysScan[iColumn].columnid         = 0;
        rgjccMSysScan[iColumn].err              = JET_errSuccess;
    }
    
    JET_TABLECREATE5_A jtcMSysScan =
    {
        sizeof(JET_TABLECREATE5_A),
        (CHAR *)szMSysScan,
        NULL,
        1,  // only 1 record so only 1 page is needed
        100,
        rgjccMSysScan,
        ccolMsysScan,
        NULL,
        0,
        NULL,
        0,
        JET_bitTableCreateFixedDDL|JET_bitTableCreateSystemTable,
        NULL,
        NULL,
        0,
        0,
        JET_TABLEID( pfucbNil ),
        0
    };

    Call( ErrDIRBeginTransaction( ppib, 35877, NO_GRBIT ) );
    fInTransaction = fTrue;
    
    Call( ErrFILECreateTable( ppib, ifmp, &jtcMSysScan ) );

#ifdef DEBUG
    // make sure the columnids were assigned correctly
    for( INT iColumn = 0; iColumn < ccolMsysScan; ++iColumn )
    {
        const COLUMNID columnid = FID( fidtypFixed, (WORD)iColumn );
        Assert( columnid == rgjccMSysScan[iColumn].columnid );
        Assert( columnid == ColumnidFromMSysScanColumnName( rgszMSysScanColumns[iColumn] ) );
    }
#endif
    
    pfucb = (FUCB *)jtcMSysScan.tableid;

    Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepInsert ) )
    Call( ErrIsamUpdate( ppib, pfucb, NULL, 0, NULL, NO_GRBIT ) );
    Call( ErrFILECloseTable( ppib, pfucb ) );
    pfucb = pfucbNil;
    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
    fInTransaction = fFalse;

HandleError:
    if( pfucbNil != pfucb )
    {
        CallS( ErrFILECloseTable( ppib, pfucb ) );
    }
    if( fInTransaction )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    return err;
}


//  ================================================================
BOOL OBJIDINFO::CmpObjid( const OBJIDINFO& left, const OBJIDINFO& right )
//  ================================================================
{
    return left.objidFDP < right.objidFDP;
}


//  ================================================================
ERR ErrDBUTLScrub( JET_SESID sesid, const JET_DBUTIL_W *pdbutil )
//  ================================================================
{
    ERR err = JET_errSuccess;
    PIB * const ppib = reinterpret_cast<PIB *>( sesid );
    SCRUBDB * pscrubdb = NULL;

    LOGTIME logtimeScrub;

    PIBTraceContextScope tcScope = ((PIB*)sesid)->InitTraceContextScope();
    tcScope->nParentObjectClass = tceNone;
    tcScope->iorReason.SetIort( iortScrubbing );

    Call( ErrIsamAttachDatabase( sesid, pdbutil->szDatabase, fFalse, NULL, 0, NO_GRBIT ) );

    //  WARNING: must set ifmp to 0 to ensure high-dword is
    //  initialised on 64-bit, because we'll be casting this
    //  to a JET_DBID, which is a dword
    IFMP ifmp;
    ifmp = 0;
    Call( ErrIsamOpenDatabase(
        sesid,
        pdbutil->szDatabase,
        NULL,
        reinterpret_cast<JET_DBID *>( &ifmp ),
        NO_GRBIT
        ) );

    PGNO pgnoLast;
    pgnoLast = g_rgfmp[ifmp].PgnoLast();

    DBTIME dbtimeLastScrubNew;
    dbtimeLastScrubNew = g_rgfmp[ifmp].DbtimeLast();
    
    pscrubdb = new SCRUBDB( ifmp );
    if( NULL == pscrubdb )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    Call( pscrubdb->ErrInit( ppib, CUtilProcessProcessor() ) );

    ULONG_PTR cpgBuffersInit = 0;
    CallS( ErrBFGetCacheSize( &cpgBuffersInit ) );

    JET_SNPROG snprog;
    memset( &snprog, 0, sizeof( snprog ) );
    snprog.cunitTotal   = pgnoLast;
    snprog.cunitDone    = 0;

    JET_PFNSTATUS pfnStatus;
    pfnStatus = reinterpret_cast<JET_PFNSTATUS>( pdbutil->pfnCallback );
    
    if ( NULL != pfnStatus )
    {
        (VOID)pfnStatus( sesid, JET_snpScrub, JET_sntBegin, NULL );
    }

    PGNO pgno;
    const PGNO pgnoInit = 1;
    pgno = pgnoInit;

    while( pgnoLast >= pgno )
    {
        const CPG cpgChunk = 256;
        const CPG cpgPreread = min( cpgChunk, pgnoLast - pgno + 1 );

        BFPrereadPageRange( ifmp, pgno, cpgPreread, bfprfDefault, BfpriBackgroundRead( ifmp, ppib ), *tcScope );

        Call( pscrubdb->ErrScrubPages( pgno, cpgPreread ) );
        pgno += cpgPreread;

        snprog.cunitDone = pscrubdb->Scrubstats().cpgSeen;
        if ( NULL != pfnStatus )
        {
            (VOID)pfnStatus( sesid, JET_snpScrub, JET_sntProgress, &snprog );
        }

        ULONG_PTR cpgBuffers = 0;
        CallS( ErrBFGetCacheSize( &cpgBuffers ) );

        cpgBuffers = ( cpgBuffers > cpgBuffersInit ) ? ( cpgBuffers - cpgBuffersInit ) : 0;

        while( ( pscrubdb->Scrubstats().cpgSeen + ( cpgBuffers / 4 ) ) < ( pgno - pgnoInit )
            && pscrubdb->Scrubstats().err >= JET_errSuccess )
        {
            snprog.cunitDone = pscrubdb->Scrubstats().cpgSeen;
            if ( NULL != pfnStatus )
            {
                (VOID)pfnStatus( sesid, JET_snpScrub, JET_sntProgress, &snprog );
            }
            UtilSleep( cmsecWaitGeneric );
        }
            if ( pscrubdb->Scrubstats().err < JET_errSuccess )
            {
                break;
            }
    }

    snprog.cunitDone = pscrubdb->Scrubstats().cpgSeen;
    if ( NULL != pfnStatus )
    {
        (VOID)pfnStatus( sesid, JET_snpScrub, JET_sntProgress, &snprog );
    }

    Call( pscrubdb->ErrTerm() );
    
    if ( NULL != pfnStatus )
    {
        (VOID)pfnStatus( sesid, JET_snpRepair, JET_sntComplete, NULL );
    }

    g_rgfmp[ifmp].SetDbtimeLastScrub( dbtimeLastScrubNew );
    LGIGetDateTime( &logtimeScrub );
    g_rgfmp[ifmp].SetLogtimeScrub( logtimeScrub );
    
    (*CPRINTFSTDOUT::PcprintfInstance())( "%d pages seen\n", pscrubdb->Scrubstats().cpgSeen );
    (*CPRINTFSTDOUT::PcprintfInstance())( "%d blank pages seen\n", pscrubdb->Scrubstats().cpgUnused );
    (*CPRINTFSTDOUT::PcprintfInstance())( "%d unchanged pages seen\n", pscrubdb->Scrubstats().cpgUnchanged );
    (*CPRINTFSTDOUT::PcprintfInstance())( "%d unused pages zeroed\n", pscrubdb->Scrubstats().cpgZeroed );
    (*CPRINTFSTDOUT::PcprintfInstance())( "%d used pages seen\n", pscrubdb->Scrubstats().cpgUsed );
    (*CPRINTFSTDOUT::PcprintfInstance())( "%d pages with unknown objid\n", pscrubdb->Scrubstats().cpgUnknownObjid );
    (*CPRINTFSTDOUT::PcprintfInstance())( "%d nodes seen\n", pscrubdb->Scrubstats().cNodes );
    (*CPRINTFSTDOUT::PcprintfInstance())( "%d flag-deleted nodes zeroed\n", pscrubdb->Scrubstats().cFlagDeletedNodesZeroed );
    (*CPRINTFSTDOUT::PcprintfInstance())( "%d flag-deleted nodes not zeroed\n", pscrubdb->Scrubstats().cFlagDeletedNodesNotZeroed );
    (*CPRINTFSTDOUT::PcprintfInstance())( "%d version bits reset seen\n", pscrubdb->Scrubstats().cVersionBitsReset );
    (*CPRINTFSTDOUT::PcprintfInstance())( "%d orphaned LVs\n", pscrubdb->Scrubstats().cOrphanedLV );
    (*CPRINTFSTDOUT::PcprintfInstance())( "%d error code returned from scrub task\n", pscrubdb->Scrubstats().err );

    err = pscrubdb->Scrubstats().err;
    
    delete pscrubdb;
    pscrubdb = NULL;

    Call( err );

HandleError:
    if( NULL != pscrubdb )
    {
        const ERR errT = pscrubdb->ErrTerm();
        if( err >= 0 && errT < 0 )
        {
            err = errT;
        }
        delete pscrubdb;
    }
    return err;
}


//  ================================================================
SCRUBDB::SCRUBDB( const IFMP ifmp ) :
    m_ifmp( ifmp )
//  ================================================================
{
    m_constants.dbtimeLastScrub = 0;
    m_constants.pcprintfVerbose = CPRINTFSTDOUT::PcprintfInstance();
    m_constants.pcprintfDebug   = CPRINTFSTDOUT::PcprintfInstance();
    m_constants.objidMax        = 0;
    m_constants.pobjidinfo      = 0;
    m_constants.cobjidinfo      = 0;

    m_stats.err                 = JET_errSuccess;
    m_stats.cpgSeen             = 0;
    m_stats.cpgUnused           = 0;
    m_stats.cpgUnchanged        = 0;
    m_stats.cpgZeroed           = 0;
    m_stats.cpgUsed             = 0;
    m_stats.cpgUnknownObjid     = 0;
    m_stats.cNodes              = 0;
    m_stats.cFlagDeletedNodesZeroed     = 0;
    m_stats.cFlagDeletedNodesNotZeroed  = 0;
    m_stats.cOrphanedLV         = 0;
    m_stats.cVersionBitsReset   = 0;

    m_context.pconstants    = &m_constants;
    m_context.pstats        = &m_stats;
}


//  ================================================================
SCRUBDB::~SCRUBDB()
//  ================================================================
{
    delete [] m_constants.pobjidinfo;
}


//  ================================================================
volatile const SCRUBSTATS& SCRUBDB::Scrubstats() const
//  ================================================================
{
    return m_stats;
}


//  ================================================================
ERR SCRUBDB::ErrInit( PIB * const ppib, const INT cThreads )
//  ================================================================
{
    ERR err;
    
    INST * const pinst = PinstFromIfmp( m_ifmp );

#ifdef DEBUG
    INT cLoop = 0;
#endif  //  DEBUG

    Call( m_taskmgr.ErrInit( pinst, cThreads ) );

    CallS( ErrDIRBeginTransaction( ppib, 33829, JET_bitTransactionReadOnly ) );
    //  get an objidMax
    //  wait until this is the oldest transaction in the system
    //  at which point all objids < objidMax have been committed to
    //  the catalog. then commit the transaction and read the catalog
    m_constants.objidMax = g_rgfmp[m_ifmp].ObjidLast();
    while( TrxOldest( pinst ) != ppib->trxBegin0 )
    {
        UtilSleep( cmsecWaitGeneric );
        AssertSz( ++cLoop < 36000, "Waiting a long time for all transactions to commit (Deadlocked?)" );
    }
    CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    
    m_constants.dbtimeLastScrub = g_rgfmp[m_ifmp].DbtimeLastScrub();

    Call( ErrSCRUBGetObjidsFromCatalog(
            ppib,
            m_ifmp,
            &m_constants.pobjidinfo,
            &m_constants.cobjidinfo ) );

    return err;
    
HandleError:
    CallS( ErrTerm() );
    return err;
}


//  ================================================================
ERR SCRUBDB::ErrTerm()
//  ================================================================
{
    ERR err;
    
    CallS( m_taskmgr.ErrTerm() );

    delete [] m_constants.pobjidinfo;
    m_constants.pobjidinfo = NULL;

    err = m_stats.err;
    
    return err;
}


//  ================================================================
ERR SCRUBDB::ErrScrubPages( const PGNO pgnoFirst, const CPG cpg )
//  ================================================================
{
    SCRUBTASK * ptask = new SCRUBTASK( m_ifmp, pgnoFirst, cpg, &m_context );
    if( NULL == ptask )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    const ERR err = m_taskmgr.ErrPostTask( TASK::Dispatch, (ULONG_PTR)ptask );
    if( err < JET_errSuccess )
    {
        //  The task was not enqued successfully.
        delete ptask;
    }
    return err;
}


//  ================================================================
SCRUBTASK::SCRUBTASK( const IFMP ifmp, const PGNO pgnoFirst, const CPG cpg, const SCRUBCONTEXT * const pcontext ) :
    DBTASK( ifmp ),
    m_pgnoFirst( pgnoFirst ),
    m_cpg( cpg ),
    m_pcontext( pcontext ),
    m_pobjidinfoCached( NULL ),
    m_objidNotFoundCached( objidNil )
//  ================================================================
{
    //  don't fire off async tasks on the temp. database because the
    //  temp. database is simply not equipped to deal with concurrent access
    AssertRTL( !FFMPIsTempDB( ifmp ) );
}

    
//  ================================================================
SCRUBTASK::~SCRUBTASK()
//  ================================================================
{
}

        
//  ================================================================
ERR SCRUBTASK::ErrExecuteDbTask( PIB * const ppib )
//  ================================================================
{
    ERR err = JET_errSuccess;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = tceNone;
    tcScope->iorReason.SetIort( iortScrubbing );

    for ( PGNO pgno = m_pgnoFirst; pgno < m_pgnoFirst + m_cpg; ++pgno )
    {
        CSR csr;
        err = csr.ErrGetRIWPage( ppib, m_ifmp, pgno, bflfUninitPageOk );
        if ( JET_errPageNotInitialized == err )
        {
            //  this page is already zeroed out
            AtomicIncrement( &( m_pcontext->pstats->cpgUnused ) );
            err = JET_errSuccess;
        }
        else if ( err >= 0 )
        {
            err = ErrProcessPage_( ppib, &csr );
        }

        AtomicIncrement( &( m_pcontext->pstats->cpgSeen ) );
        csr.ReleasePage( fTrue );
        Call( err );
    }

HandleError:
    if ( err < 0 )
    {
        AtomicCompareExchange( &m_pcontext->pstats->err, JET_errSuccess, err );
    }
    return err;
}


//  ================================================================
VOID SCRUBTASK::HandleError( const ERR err )
//  ================================================================
{
}


//  ================================================================
ERR SCRUBTASK::ErrProcessPage_(
        PIB * const ppib,
        CSR * const pcsr )
//  ================================================================
{
    const OBJID objid = pcsr->Cpage().ObjidFDP();

    // do not scrub space tree pages
    if ( pcsr->Cpage().FSpaceTree() )
    {
        return JET_errSuccess;
    }

    if( objid > m_pcontext->pconstants->objidMax )
    {
        AtomicIncrement( &( m_pcontext->pstats->cpgUnknownObjid ) );
        return JET_errSuccess;
    }
    
    if( objid == m_objidNotFoundCached )
    {
        return ErrProcessUnusedPage_( ppib, pcsr );
    }
    else if( m_pobjidinfoCached && objid == m_pobjidinfoCached->objidFDP )
    {
    }
    else if( objidSystemRoot == objid )
    {
        //  this is not in the catalog
        return JET_errSuccess;
    }
    else
    {
        OBJIDINFO objidinfoSearch;
        objidinfoSearch.objidFDP = objid;
        
        const OBJIDINFO * pobjidinfoT = m_pcontext->pconstants->pobjidinfo;
        pobjidinfoT = lower_bound(
                        pobjidinfoT,
                        m_pcontext->pconstants->pobjidinfo + m_pcontext->pconstants->cobjidinfo,
                        objidinfoSearch,
                        OBJIDINFO::CmpObjid );
                        
        if( pobjidinfoT == m_pcontext->pconstants->pobjidinfo + m_pcontext->pconstants->cobjidinfo
            || pobjidinfoT->objidFDP != objid )
        {
            m_objidNotFoundCached = objid;
            return ErrProcessUnusedPage_( ppib, pcsr );
        }
        else
        {
            m_pobjidinfoCached = pobjidinfoT;
        }
    }

    if( pcsr->Cpage().Dbtime() < m_pcontext->pconstants->dbtimeLastScrub )
    {
        //  this page should not have been empty the last time we saw it
        //  if so we would have zeroed it out and never should have ended up here
        AtomicIncrement( &( m_pcontext->pstats->cpgUnchanged ) );
        return JET_errSuccess;
    }

    if( pcsr->Cpage().FPreInitPage() )
    {
        return JET_errSuccess;
    }

    if( pcsr->Cpage().FEmptyPage() )
    {
        return ErrProcessUnusedPage_( ppib, pcsr );
    }

    return ErrProcessUsedPage_( ppib, pcsr );
}


//  ================================================================
ERR SCRUBTASK::ErrProcessUnusedPage_(
        PIB * const ppib,
        CSR * const pcsr )
//  ================================================================
//
//  Can't call CPAGE::ResetHeader because of concurrency issues
//  we don't want to lose our latch on the page, let someone else
//  use the page and then zero it out
//
//-
{
    pcsr->UpgradeFromRIWLatch();

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = tceNone;
    tcScope->iorReason.SetIort( iortScrubbing );

    //  Don't call CSR::Dirty() or CPAGE::Dirty() because we don't
    //  want the dbtime on the page changed
    BFDirty( pcsr->Cpage().PBFLatch(), bfdfDirty, *tcScope );

    //  Delete all the data on the page
    const INT clines = pcsr->Cpage().Clines();
    INT iline;
    for( iline = 0; iline < clines; ++iline )
    {
        pcsr->Cpage().Delete( 0 );
    }
    
    //  reorganize the page and zero all unused data
    pcsr->Cpage().OverwriteUnusedSpace( chSCRUBLegacyUnusedPageFill );
    
    pcsr->Downgrade( latchReadNoTouch );

    AtomicIncrement( &( m_pcontext->pstats->cpgZeroed ) );

    return JET_errSuccess;
}


//  ================================================================
BOOL SCRUBTASK::FNodeHasVersions_( const OBJID objidFDP, const KEYDATAFLAGS& kdf, const TRX trxOldest ) const
//  ================================================================
{
    Assert( m_pobjidinfoCached->objidFDP == objidFDP );

    BOOKMARK bm;
    bm.key = kdf.key;
    if( m_pobjidinfoCached->fUnique )
    {
        bm.data.Nullify();
    }
    else
    {
        bm.data = kdf.data;
    }

    const BOOL fActiveVersion = FVERActive( m_ifmp, m_pobjidinfoCached->pgnoFDP, bm, trxOldest );
    return fActiveVersion;
}


//  ================================================================
ERR SCRUBTASK::ErrProcessUsedPage_(
        PIB * const ppib,
        CSR * const pcsr )
//  ================================================================
//
//  for each node on the page
//    if it is flag-deleted zero out the data
//    check for an orphaned LV
//
//-
{
    Assert( m_pobjidinfoCached->objidFDP == pcsr->Cpage().ObjidFDP() );
    
    ERR err = JET_errSuccess;
    
    AtomicIncrement( &( m_pcontext->pstats->cpgUsed ) );

    const TRX trxOldest = TrxOldest( PinstFromPpib( ppib ) );
    
    pcsr->UpgradeFromRIWLatch();

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = tceNone;
    tcScope->iorReason.SetIort( iortScrubbing );

    //  Don't call CSR::Dirty() or CPAGE::Dirty() because we don't
    //  want the dbtime on the page changed
    BFDirty( pcsr->Cpage().PBFLatch(), bfdfDirty, *tcScope );

    if( pcsr->Cpage().FLeafPage() )
    {
        LONG cVersionBitsReset          = 0;
        LONG cFlagDeletedNodesZeroed    = 0;
        LONG cFlagDeletedNodesNotZeroed = 0;
        
        KEYDATAFLAGS kdf;
        INT iline;
        for( iline = 0; iline < pcsr->Cpage().Clines(); ++iline )
        {
            NDIGetKeydataflags( pcsr->Cpage(), iline, &kdf );
            
            if( FNDDeleted( kdf ) )
            {
                if( !FNDVersion( kdf )
                    || !FNodeHasVersions_( pcsr->Cpage().ObjidFDP(), kdf, trxOldest ) )
                {
                    //  don't zero out the data for non-unique indexes
                    //  the data is part of the bookmark and can't be removed or the sort order
                    //  will be wrong
                    if( m_pobjidinfoCached->fUnique )
                    {
                        ++cFlagDeletedNodesZeroed;
                        //  the kdf is pointing directly into the page
                        memset( kdf.data.Pv(), chSCRUBLegacyDeletedDataFill, kdf.data.Cb() );
                    }

                    if( FNDVersion( kdf ) )
                    {
                        ++cVersionBitsReset;
                        pcsr->Cpage().ReplaceFlags( iline, kdf.fFlags & ~fNDVersion );
                    }
                }
                else
                {
                    ++cFlagDeletedNodesNotZeroed;
                }
            }
            else if( pcsr->Cpage().FLongValuePage()
                    && !pcsr->Cpage().FSpaceTree() )
            {
                if( FIsLVRootKey( kdf.key ) )
                {
                    if( sizeof( LVROOT ) != kdf.data.Cb() && sizeof( LVROOT2 ) != kdf.data.Cb() )
                    {
                        AssertSz( fFalse, "Corrupted LV: corrupted LVROOT" );
                        LvId lidT;
                        LidFromKey( &lidT, kdf.key );
                        LVReportAndTrapCorruptedLV2( PinstFromPpib( ppib ), L"", "", pcsr->Pgno(), lidT, L"c7aa6c1c-3c53-4fd1-985e-df5cf9e0c028" );
                        Call( ErrERRCheck( JET_errLVCorrupted ) );
                    }
                    const LVROOT * const plvroot = reinterpret_cast<LVROOT *>( kdf.data.Pv() );
                    if( 0 == plvroot->ulReference
                        && !FNodeHasVersions_( pcsr->Cpage().ObjidFDP(), kdf, trxOldest ) )
                    {
                        AtomicIncrement( &( m_pcontext->pstats->cOrphanedLV ) );
                        
                        Call( ErrSCRUBZeroLV( ppib, m_ifmp, pcsr, iline ) );
                    }
                }
            }
        }

        //  report stats for all nodes at one time to reduce contention
        AtomicExchangeAdd( &( m_pcontext->pstats->cNodes ), pcsr->Cpage().Clines() );
        AtomicExchangeAdd( &( m_pcontext->pstats->cVersionBitsReset ), cVersionBitsReset );
        AtomicExchangeAdd( &( m_pcontext->pstats->cFlagDeletedNodesZeroed ), cFlagDeletedNodesZeroed );
        AtomicExchangeAdd( &( m_pcontext->pstats->cFlagDeletedNodesNotZeroed ), cFlagDeletedNodesNotZeroed );
    }
    
    //  reorganize the page and zero all unused data
    pcsr->Cpage().OverwriteUnusedSpace( chSCRUBLegacyUsedPageFreeFill );
    
HandleError:
    pcsr->Downgrade( latchReadNoTouch );
    CallS( err );
    return err;
}


//  ================================================================
LOCAL ERR ErrSCRUBGetObjidsFromCatalog(
        PIB * const ppib,
        const IFMP ifmp,
        const OBJIDINFO ** ppobjidinfo,
        LONG * pcobjidinfo )
//  ================================================================
{
    FUCB * pfucbCatalog = pfucbNil;
    ERR err;
    DATA data;

    *ppobjidinfo    = NULL;
    *pcobjidinfo    = 0;

    const LONG      cobjidinfoChunk     = 64;
    LONG            iobjidinfoNext      = 0;
    LONG            cobjidinfoAllocated = cobjidinfoChunk;
    OBJIDINFO   *   pobjidinfo          = new OBJIDINFO[cobjidinfoChunk];

    if( NULL == pobjidinfo )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    
    SYSOBJ  sysobj;

    OBJID   objidFDP    = objidNil;
    PGNO    pgnoFDP     = pgnoNull;
    BOOL    fUnique;
    BOOL    fPrimaryIndex;

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    FUCBSetSequential( pfucbCatalog );
    FUCBSetPrereadForward( pfucbCatalog, cpgPrereadSequential );

    err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, 0 );
    while ( err >= 0 )
    {
        BOOL fBtree = fFalse;
        
        Call( ErrDIRGet( pfucbCatalog ) );

        Assert( fidMSO_Type.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Type,
                    pfucbCatalog->kdfCurr.data,
                    &data ) );
        CallS( err );

        sysobj = (SYSOBJ)(*( reinterpret_cast<UnalignedLittleEndian< SYSOBJ > *>( data.Pv() ) ) );
        switch( sysobj )
        {
            case sysobjNil:
            case sysobjColumn:
            case sysobjCallback:
                //  These are not B-trees
                break;
                
            case sysobjTable:
            case sysobjIndex:
            case sysobjLongValue:
                //  These are B-trees
                //  Obtain their objids and pgnoFDPs
                fBtree = fTrue;
                
                Assert( fidMSO_Id.FFixed() );
                Call( ErrRECIRetrieveFixedColumn(
                            pfcbNil,
                            pfucbCatalog->u.pfcb->Ptdb(),
                            fidMSO_Id,
                            pfucbCatalog->kdfCurr.data,
                            &data ) );
                CallS( err );

                objidFDP = (OBJID)(*( reinterpret_cast<UnalignedLittleEndian< OBJID > *>( data.Pv() ) ));

                Assert( fidMSO_PgnoFDP.FFixed() );
                Call( ErrRECIRetrieveFixedColumn(
                            pfcbNil,
                            pfucbCatalog->u.pfcb->Ptdb(),
                            fidMSO_PgnoFDP,
                            pfucbCatalog->kdfCurr.data,
                            &data ) );
                CallS( err );

                pgnoFDP = (PGNO)(*( reinterpret_cast<UnalignedLittleEndian< PGNO > *>( data.Pv() ) ));

                break;

            default:
                Assert( fFalse );
                break;
        }

        //  we need a list of unique objids for version store lookups
        fUnique         = fTrue;
        fPrimaryIndex   = fFalse;
        if( sysobjIndex == sysobj )
        {
            IDBFLAG idbflag;
            Assert( fidMSO_Flags.FFixed() );
            Call( ErrRECIRetrieveFixedColumn(
                        pfcbNil,
                        pfucbCatalog->u.pfcb->Ptdb(),
                        fidMSO_Flags,
                        pfucbCatalog->kdfCurr.data,
                        &data ) );
            CallS( err );
            Assert( data.Cb() == sizeof(ULONG) );
            UtilMemCpy( &idbflag, data.Pv(), sizeof(IDBFLAG) );

            fUnique = FIDBUnique( idbflag );
            fPrimaryIndex = FIDBPrimary( idbflag );
        }

        Call( ErrDIRRelease( pfucbCatalog ) );

        //  the primary index is already recorded as the B-Tree
        if( fBtree && !fPrimaryIndex )
        {
            if( iobjidinfoNext >= cobjidinfoAllocated )
            {
                const LONG cobjidinfoAllocatedOld = cobjidinfoAllocated;
                const LONG cobjidinfoAllocatedNew = cobjidinfoAllocated + cobjidinfoChunk;
                OBJIDINFO * const pobjidinfoOld = pobjidinfo;
                OBJIDINFO * const pobjidinfoNew = new OBJIDINFO[cobjidinfoAllocatedNew];
                
                if( NULL == pobjidinfoNew )
                {
                    Call( ErrERRCheck( JET_errOutOfMemory ) );
                }
                
                memcpy( pobjidinfoNew, pobjidinfoOld, sizeof( OBJIDINFO ) * cobjidinfoAllocatedOld );

                pobjidinfo      = pobjidinfoNew;
                cobjidinfoAllocated = cobjidinfoAllocatedNew;

                delete [] pobjidinfoOld;
            }
            pobjidinfo[iobjidinfoNext].objidFDP = objidFDP;
            pobjidinfo[iobjidinfoNext].pgnoFDP  = pgnoFDP;
            pobjidinfo[iobjidinfoNext].fUnique  = fUnique;

            ++iobjidinfoNext;
        }
        
        err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, 0 );
    }
    
    if ( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }
    
HandleError:
    if( pfucbNil != pfucbCatalog )
    {
        DIRUp( pfucbCatalog );
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }

    if( err >= 0 )
    {
        sort( pobjidinfo, pobjidinfo + iobjidinfoNext, OBJIDINFO::CmpObjid );
        *ppobjidinfo    = pobjidinfo;
        *pcobjidinfo    = iobjidinfoNext;
    }
    else
    {
        delete [] pobjidinfo;
        *ppobjidinfo    = NULL;
        *pcobjidinfo        = 0;
    }
    return err;
}

#endif  //  MINIMAL_FUNCTIONALITY

