// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include <stdarg.h>

#define wszTemp L"temp"


extern VOID ITDBGSetConstants( INST * pinst = NULL);

BACKUP_CONTEXT::BACKUP_CONTEXT( INST * pinst )
    : CZeroInit( sizeof( BACKUP_CONTEXT ) ),
      m_pinst( pinst ),
      //    Note: That m_iInstance is 1 higher than the corresponding ipinst value.  I am adding 1 so 
      //    no INST has a subrank of 0.
      m_critBackupInProgress( CLockBasicInfo( CSyncBasicInfo( "BackupInProcess" ), rankBackupInProcess, 1 + g_cpinstMax - pinst->m_iInstance ) )
{
#ifdef DEBUG
    WCHAR * wsz;
    if ( ( wsz = GetDebugEnvValue ( L"TRACEBR" ) ) != NULL )
    {
        m_fDBGTraceBR = _wtoi( wsz );
        delete[] wsz;
    }
    else
        m_fDBGTraceBR = 0;
#endif
}

/*  caller has to make sure szDir has enough space for appending "*"
/**/
LOCAL ERR ErrLGDeleteAllFiles( IFileSystemAPI *const pfsapi, __inout_bcount(cbDir) const PWSTR wszDir, ULONG cbDir )
{
    ERR             err     = JET_errSuccess;
    IFileFindAPI*   pffapi  = NULL;

    Assert( LOSStrLengthW( wszDir ) + 1 + 1 < IFileSystemAPI::cchPathMax );

    WCHAR wszPath[ IFileSystemAPI::cchPathMax ];
    Call( ErrOSStrCbCopyW( wszPath, sizeof( wszPath ), wszDir ) );
    Call( pfsapi->ErrPathFolderNorm( wszPath, sizeof( wszPath ) ) );
    Call( ErrOSStrCbAppendW( wszPath, sizeof( wszPath ), L"*" ) );

    //  iterate over all files in this directory
    Call( pfsapi->ErrFileFind( wszPath, &pffapi ) );
    while ( ( err = pffapi->ErrNext() ) == JET_errSuccess )
    {
        WCHAR wszFileName[IFileSystemAPI::cchPathMax];
        WCHAR wszDirT[IFileSystemAPI::cchPathMax];
        WCHAR wszFileT[IFileSystemAPI::cchPathMax];
        WCHAR wszExtT[IFileSystemAPI::cchPathMax];
        WCHAR wszFileNameT[IFileSystemAPI::cchPathMax];

        Call( pffapi->ErrPath( wszFileName ) );
        Call( pfsapi->ErrPathParse( wszFileName, wszDirT, wszFileT, wszExtT ) );
        wszDirT[0] = 0;
        Call( pfsapi->ErrPathBuild( wszDirT, wszFileT, wszExtT, wszFileNameT ) );

        /* not . , and .. and not temp
        /**/
        if (    LOSStrCompareW( wszFileNameT, L"." ) &&
                LOSStrCompareW( wszFileNameT, L".." ) &&
                UtilCmpFileName( wszFileNameT, wszTemp ) )
        {
            err = pfsapi->ErrFileDelete( wszFileName );
            if ( err != JET_errSuccess )
            {
                Call( ErrERRCheck( JET_errDeleteBackupFileFail ) );
            }
        }
    }
    Call( err == JET_errFileNotFound ? JET_errSuccess : err );

    err = JET_errSuccess;

HandleError:
    /*  assert restored szDir
    /**/
    Assert( wszDir[LOSStrLengthW(wszDir)] != L'*' );

    delete pffapi;

    return err;
}


/*  caller has to make sure szDir has enough space for appending "*"
/**/
ERR ErrLGCheckDir( IFileSystemAPI *const pfsapi, __inout_bcount(cbDir) const PWSTR wszDir, ULONG cbDir, __in_opt PCWSTR wszSearch )
{
    ERR             err     = JET_errSuccess;
    IFileFindAPI*   pffapi  = NULL;

    Assert( LOSStrLengthW( wszDir ) + 1 + 1 < IFileSystemAPI::cchPathMax );

    WCHAR wszPath[ IFileSystemAPI::cchPathMax ];
    Call( ErrOSStrCbCopyW( wszPath, sizeof( wszPath ), wszDir ) );
    Call( pfsapi->ErrPathFolderNorm( wszPath, sizeof( wszPath ) ) );
    Call( ErrOSStrCbAppendW( wszPath, sizeof( wszPath ), L"*" ) );

    //  iterate over all files in this directory
    Call( pfsapi->ErrFileFind( wszPath, &pffapi ) );
    while ( ( err = pffapi->ErrNext() ) == JET_errSuccess )
    {
        WCHAR wszFileName[IFileSystemAPI::cchPathMax];
        WCHAR wszDirT[IFileSystemAPI::cchPathMax];
        WCHAR wszFileT[IFileSystemAPI::cchPathMax];
        WCHAR wszExtT[IFileSystemAPI::cchPathMax];
        WCHAR wszFileNameT[IFileSystemAPI::cchPathMax];

        Call( pffapi->ErrPath( wszFileName ) );
        Call( pfsapi->ErrPathParse( wszFileName, wszDirT, wszFileT, wszExtT ) );
        wszDirT[0] = 0;
        Call( pfsapi->ErrPathBuild( wszDirT, wszFileT, wszExtT, wszFileNameT ) );

        /* not . , and .. and not szSearch
        /**/
        if (    LOSStrCompareW( wszFileNameT, L"." ) &&
                LOSStrCompareW( wszFileNameT, L".." ) &&
                ( !wszSearch || !UtilCmpFileName( wszFileNameT, wszSearch ) ) )
        {
            Call( ErrERRCheck( JET_errBackupDirectoryNotEmpty ) );
        }
    }
    Call( err == JET_errFileNotFound ? JET_errSuccess : err );

    err = JET_errSuccess;

HandleError:
    /*  assert restored szDir
    /**/
    Assert( wszDir[LOSStrLengthW(wszDir)] != L'*' );

    delete pffapi;

    return err;
}


#define DISABLE_EXTENSIVE_CHECKS_DURING_STREAMING_BACKUP 1

ERR BACKUP_CONTEXT::ErrBKIReadPages(
    RHF *prhf,
    VOID *pvPageMin,
    INT cpage,
    INT *pcbActual
#ifdef DEBUG
    , __out_bcount(cbLGDBGPageList) PSTR szLGDBGPageList,
    ULONG cbLGDBGPageList
#endif
    )
{
    ERR     err = JET_errSuccess;
    INT     cpageT;
    INT     ipageT;
    IFMP    ifmp = prhf->ifmp;
    FMP     *pfmp = &g_rgfmp[ifmp];
    TraceContextScope tcBackup( iortBackupRestore );

    // we add a final page, the former patch file header
    BOOL fRoomForFinalHeaderPage = fFalse;

#ifdef MINIMAL_FUNCTIONALITY
#else

    /*  determine if we are scrubbing the database
    /**/
    CONST BOOL fScrub = BoolParam( m_pinst, JET_paramZeroDatabaseDuringBackup );

    if ( fScrub && m_pinst->FRecovering() )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    /*  we are scrubbing the database
    /**/
    if ( fScrub )
    {
        /*  we are scrubbing the first page of the database
        /**/
        if ( g_rgfmp[ ifmp ].PgnoBackupCopyMost() == 0 )
        {
            /*  init the scrub
            /**/
            const WCHAR * rgszT[1];
            INT isz = 0;

            Assert( NULL == m_pscrubdb );
            AllocR( m_pscrubdb = new SCRUBDB( ifmp ) );

            CallR( m_pscrubdb->ErrInit( m_ppibBackup, CUtilProcessProcessor() ) );

            m_ulSecsStartScrub = UlUtilGetSeconds();
            m_dbtimeLastScrubNew = g_rgfmp[ifmp].DbtimeLast();

            rgszT[isz++] = g_rgfmp[ifmp].WszDatabaseName();
            Assert( isz <= sizeof( rgszT ) / sizeof( rgszT[0] ) );

            UtilReportEvent(
                    eventInformation,
                    DATABASE_ZEROING_CATEGORY,
                    DATABASE_ZEROING_STARTED_ID,
                    isz,
                    rgszT,
                    0,
                    NULL,
                    m_pinst );
        }
    }

#endif  //  MINIMAL_FUNCTIONALITY

    //  caller should have verified that buffer is not empty
    //
    Assert( cpage > 0 );

    /*  assume that database will be read in sets of cpage
    /*  pages.  Preread next cpage pages while the current
    /*  cpage pages are being read, and copied to caller
    /*  buffer.
    /*
    /*  preread next next cpage pages.  These pages should
    /*  be read while the next cpage pages are written to
    /*  the backup datababase file.
    /**/

    /*  read pages, which may have been preread, up to cpage but
    /*  not beyond last page at time of initiating backup.
    /**/
    Assert( pfmp->PgnoBackupMost() >= pfmp->PgnoBackupCopyMost() );
    cpageT = min( cpage, (INT)( pfmp->PgnoBackupMost() - pfmp->PgnoBackupCopyMost() + ( 0 == pfmp->PgnoBackupCopyMost() ? cpgDBReserved : 0 ) ) );
    *pcbActual = 0;
    ipageT = 0;

    // we check if we have space for the last page
    fRoomForFinalHeaderPage =  ( cpageT < cpage );

    /*  if we have no more pages to read, we're done
    /**/
    if ( cpageT == 0
        && ( !fRoomForFinalHeaderPage || pfmp->FCopiedPatchHeader() ) )
    {
        Assert( pfmp->PgnoBackupMost() == pfmp->PgnoBackupCopyMost() );
        return JET_errSuccess;
    }

    if ( pfmp->PgnoBackupCopyMost() == 0 )
    {
        //  caller should have verified that buffer has enough
        //  space for the reserved pages, plus at least one
        //  more page (to ensure that PgnoBackupCopyMost advances
        //  beyond 0)
        //
        Assert( cpage > cpgDBReserved );

        // Patch the db header (and shadow) with the one saved off at OpenFile
        Assert( prhf->ib == 0 );
        UtilMemCpy( pvPageMin, prhf->pdbfilehdr, g_cbPage );
        UtilMemCpy( (BYTE*)pvPageMin + g_cbPage, prhf->pdbfilehdr, g_cbPage );

        /*  we use first 2 pages buffer
         */
        *pcbActual += g_cbPage * 2;
        ipageT += 2;
        Assert( 2 == cpgDBReserved );
        Assert( cpage >= ipageT );
    }

    const PGNO  pgnoStart   = pfmp->PgnoBackupCopyMost() + 1;
    const PGNO  pgnoEnd     = pfmp->PgnoBackupCopyMost() + ( cpageT - ipageT );

    if ( pgnoStart <= pgnoEnd  )
    {
        //  engage range lock for the region to copy
        CallR( pfmp->ErrRangeLock( pgnoStart, pgnoEnd ) );

        // we will revert this on error after the RangeUnlock
        pfmp->SetPgnoBackupCopyMost( pgnoEnd );
    }
    else
    {
        Assert( fRoomForFinalHeaderPage );
    }

    //  we will retry failed reads during backup in the hope of saving the
    //  backup set

    const TICK  tickStart   = TickOSTimeCurrent();
    const TICK  tickBackoff = 100;
    const INT   cRetry      = 16;

    INT         iRetry      = 0;

    // if we have just the last page, avoid the loop
    // and the range locking part
    if ( pgnoStart > pgnoEnd )
    {
        Assert( fRoomForFinalHeaderPage );
        goto CopyFinalHeaderPage;
    }

    do
    {
        TraceContextScope tcBackupT( iorpBackup );   // iortBackupRestore is already setup in the current scope above

        err = ErrIOReadDbPages( ifmp, pfmp->Pfapi(), (BYTE *) pvPageMin + ipageT * g_cbPage, pgnoStart, pgnoEnd, fTrue, 0, *tcBackupT, !!( UlParam( m_pinst, JET_paramDisableVerifications ) & DISABLE_EXTENSIVE_CHECKS_DURING_STREAMING_BACKUP ) );

        if ( err < JET_errSuccess )
        {
            if ( iRetry < cRetry )
            {
                // For patchable errors, release the range lock so that page patching has a chance to patch the page
                BOOL fIsPatchable = BoolParam( m_pinst, JET_paramEnableExternalAutoHealing ) && PagePatching::FIsPatchableError( err );
                if ( fIsPatchable )
                {
                    // disengage range lock for the region copied
                    pfmp->RangeUnlock( pgnoStart, pgnoEnd );
                }

                UtilSleep( ( iRetry + 1 ) * tickBackoff );

                if ( fIsPatchable )
                {
                    // engage range lock for the region to copy
                    ERR errLock = pfmp->ErrRangeLock( pgnoStart, pgnoEnd );
                    if ( errLock < JET_errSuccess )
                    {
                        return errLock;
                    }
                }
            }
        }
        else
        {
            if ( iRetry > 0 )
            {
                const WCHAR*    rgpsz[ 5 ];
                DWORD           irgpsz      = 0;
                WCHAR           szAbsPath[ IFileSystemAPI::cchPathMax ];
                WCHAR           szOffset[ 64 ];
                WCHAR           szLength[ 64 ];
                WCHAR           szFailures[ 64 ];
                WCHAR           szElapsed[ 64 ];

                CallS( pfmp->Pfapi()->ErrPath( szAbsPath ) );
                OSStrCbFormatW( szOffset, sizeof( szOffset ), L"%I64i (0x%016I64x)", OffsetOfPgno( pgnoStart ), OffsetOfPgno( pgnoStart ) );
                OSStrCbFormatW( szLength, sizeof( szLength ), L"%u (0x%08x)", ( pgnoEnd - pgnoStart + 1 ) * g_cbPage, ( pgnoEnd - pgnoStart + 1 ) * g_cbPage );
                OSStrCbFormatW( szFailures, sizeof( szFailures ), L"%i", iRetry );
                OSStrCbFormatW( szElapsed, sizeof( szElapsed ), L"%g", ( TickOSTimeCurrent() - tickStart ) / 1000.0 );

                rgpsz[ irgpsz++ ]   = szAbsPath;
                rgpsz[ irgpsz++ ]   = szOffset;
                rgpsz[ irgpsz++ ]   = szLength;
                rgpsz[ irgpsz++ ]   = szFailures;
                rgpsz[ irgpsz++ ]   = szElapsed;

                UtilReportEvent(    eventError,
                                    LOGGING_RECOVERY_CATEGORY,
                                    TRANSIENT_READ_ERROR_DETECTED_ID,
                                    irgpsz,
                                    rgpsz,
                                    0,
                                    NULL,
                                    m_pinst );
            }
        }
    }
    while ( iRetry++ < cRetry && err < JET_errSuccess );

#ifdef MINIMAL_FUNCTIONALITY
#else

    /*  we are scrubbing the database and there was no error reading the pages
    /**/
    if ( fScrub && err >= JET_errSuccess )
    {
        /*  load all pages from the backup region into the cache so that they
        /*  can be scrubbed, skipping any uninitialized pages
        /**/
        PGNO pgnoScrubStart = 0xFFFFFFFF;
        PGNO pgnoScrubEnd   = 0x00000000;
        PIBTraceContextScope tcRef = m_ppibBackup->InitTraceContextScope();
        tcRef->nParentObjectClass = tceNone;
        tcRef->iorReason.SetIort( iortScrubbing );

        for ( PGNO pgno = pgnoStart; pgno <= pgnoEnd; pgno++ )
        {
            /*  get the raw page data
            /**/
            void* pv = (BYTE *)pvPageMin + ( pgno - pgnoStart + ipageT ) * g_cbPage;

            /*  see if this page is initialized
            /**/
            Assert( pfmp->CbPage() == g_cbPage ); // for now
            CPAGE cpageLoad;
            cpageLoad.LoadPage( pv, pfmp->CbPage() );
            const BOOL fPageIsInitialized = cpageLoad.FPageIsInitialized();
            cpageLoad.UnloadPage();

            /*  this page is initialized
            /**/
            if ( fPageIsInitialized )
            {
                /*  load this page data into the cache if not already cached
                /**/
                BFLatch bfl;
                if ( ErrBFWriteLatchPage( &bfl, ifmp, pgno, BFLatchFlags( bflfNoCached | bflfNew ), m_ppibBackup->BfpriPriority( ifmp ), *tcRef ) >= JET_errSuccess )
                {
                    UtilMemCpy( bfl.pv, pv, g_cbPage );
                    BFWriteUnlatch( &bfl );
                }

                /*  we need to scrub this page eventually
                /**/
                pgnoScrubStart  = min( pgnoScrubStart, pgno );
                pgnoScrubEnd    = max( pgnoScrubEnd, pgno );
            }

            /*  this page is not initialized or we are on the last page
            /**/
            if ( !fPageIsInitialized || pgno == pgnoEnd )
            {
                /*  we have pages to scrub
                /**/
                if ( pgnoScrubStart <= pgnoScrubEnd )
                {
                    /*  scrub these pages
                    /**/
                    err = m_pscrubdb->ErrScrubPages( pgnoScrubStart, pgnoScrubEnd - pgnoScrubStart + 1 );

                    /*  reset scrub range
                    /**/
                    pgnoScrubStart  = 0xFFFFFFFF;
                    pgnoScrubEnd    = 0x00000000;
                }
            }
        }

        /*  all pages had better be scrubbed!
        /**/
        Assert( pgnoScrubStart == 0xFFFFFFFF );
        Assert( pgnoScrubEnd == 0x00000000 );
    }

#endif  //  MINIMAL_FUNCTIONALITY

    if ( err < JET_errSuccess )
    {
        Assert ( pgnoStart > 0 );
        // we need to revert CopyMost on error
        pfmp->SetPgnoBackupCopyMost( pgnoStart - 1 );
    }

    /*  disengage range lock for the region copied
    /**/
    pfmp->RangeUnlock( pgnoStart, pgnoEnd );

    Call( err );

    /*  update the read data count
    /**/
    *pcbActual += g_cbPage * ( cpageT - ipageT );

CopyFinalHeaderPage:
    if ( fRoomForFinalHeaderPage )
    {
        BKIMakeDbTrailer( ifmp, (BYTE *)pvPageMin + *pcbActual);
        *pcbActual += g_cbPage;

        if ( BoolParam( m_pinst, JET_paramAggressiveLogRollover ) )
        {
            PIB pibFake;
            pibFake.m_pinst = m_pinst;
            LGPOS lgposRoll;

            // Make best effort to roll the log so that the newly reseeded copy is not dependent on edb.log
            if ( ErrLGForceLogRollover( &pibFake, __FUNCTION__, &lgposRoll ) >= JET_errSuccess )
            {
                (VOID)ErrLGWaitForWrite( &pibFake, &lgposRoll );
            }
        }
    }

#ifdef DEBUG
    for ( PGNO pgnoCur = pfmp->PgnoBackupCopyMost() - ( cpageT - ipageT ) + 1;
        ipageT < cpageT;
        ipageT++, pgnoCur++ )
    {
        if ( m_fDBGTraceBR > 1 && szLGDBGPageList )
        {
            Assert( pfmp->CbPage() == g_cbPage ); // for now
            CPAGE cpageLoad;
            cpageLoad.LoadPage( (BYTE*) pvPageMin + g_cbPage * ipageT, pfmp->CbPage() );
            OSStrCbFormatA( szLGDBGPageList,
                        cbLGDBGPageList,
                        "(%ld, %I64d) ",
                        pgnoCur,
                        cpageLoad.Dbtime() );
            cpageLoad.UnloadPage();
            cbLGDBGPageList -= strlen( szLGDBGPageList );
            szLGDBGPageList += strlen( szLGDBGPageList );
        }
    }
#endif

HandleError:
    if ( err < JET_errSuccess )
    {
        *pcbActual = 0;
    }

#ifdef MINIMAL_FUNCTIONALITY
#else

    /*  we just scrubbed the last page in the database
    /**/
    if ( fScrub
        && ( NULL != m_pscrubdb )
        && ( g_rgfmp[ ifmp ].PgnoBackupCopyMost() == g_rgfmp[ ifmp ].PgnoBackupMost() ) )
    {
        /*  term the scrub
        /**/
        err = m_pscrubdb->ErrTerm();

        const ULONG_PTR ulSecFinished   = UlUtilGetSeconds();
        const ULONG_PTR ulSecs          = ulSecFinished - m_ulSecsStartScrub;

        const WCHAR * rgszT[16];
        INT isz = 0;

        WCHAR   szSeconds[16];
        WCHAR   szErr[16];
        WCHAR   szPages[16];
        WCHAR   szBlankPages[16];
        WCHAR   szUnchangedPages[16];
        WCHAR   szUnusedPages[16];
        WCHAR   szUsedPages[16];
        WCHAR   szDeletedRecordsZeroed[16];
        WCHAR   szOrphanedLV[16];


        OSStrCbFormatW( szSeconds, sizeof(szSeconds), L"%" FMTSZ3264 L"d", ulSecs );
        OSStrCbFormatW( szErr, sizeof(szErr), L"%d", err );
        OSStrCbFormatW( szPages, sizeof(szPages), L"%d", m_pscrubdb->Scrubstats().cpgSeen );
        OSStrCbFormatW( szBlankPages, sizeof(szBlankPages), L"%d", m_pscrubdb->Scrubstats().cpgUnused );
        OSStrCbFormatW( szUnchangedPages, sizeof(szUnchangedPages), L"%d", m_pscrubdb->Scrubstats().cpgUnchanged );
        OSStrCbFormatW( szUnusedPages, sizeof(szUnusedPages), L"%d", m_pscrubdb->Scrubstats().cpgZeroed );
        OSStrCbFormatW( szUsedPages, sizeof(szUsedPages), L"%d", m_pscrubdb->Scrubstats().cpgUsed );
        OSStrCbFormatW( szDeletedRecordsZeroed, sizeof(szDeletedRecordsZeroed), L"%d", m_pscrubdb->Scrubstats().cFlagDeletedNodesZeroed );
        OSStrCbFormatW( szOrphanedLV, sizeof(szOrphanedLV), L"%d", m_pscrubdb->Scrubstats().cOrphanedLV );

        rgszT[isz++] = g_rgfmp[ifmp].WszDatabaseName();
        rgszT[isz++] = szSeconds;
        rgszT[isz++] = szErr;
        rgszT[isz++] = szPages;
        rgszT[isz++] = szBlankPages;
        rgszT[isz++] = szUnchangedPages;
        rgszT[isz++] = szUnusedPages;
        rgszT[isz++] = szUsedPages;
        rgszT[isz++] = szDeletedRecordsZeroed;
        rgszT[isz++] = szOrphanedLV;

        Assert( isz <= _countof( rgszT ) );
        UtilReportEvent(
                eventInformation,
                DATABASE_ZEROING_CATEGORY,
                DATABASE_ZEROING_STOPPED_ID,
                isz,
                rgszT,
                0,
                NULL,
                m_pinst );

        delete m_pscrubdb;
        m_pscrubdb = NULL;

        g_rgfmp[ifmp].SetDbtimeLastScrub( m_dbtimeLastScrubNew );
        LOGTIME logtimeScrub;
        LGIGetDateTime( &logtimeScrub );
        g_rgfmp[ifmp].SetLogtimeScrub( logtimeScrub );
    }

#endif  //  MINIMAL_FUNCTIONALITY

    return err;
}


/*  begin new log file and compute log backup parameters:
 *      m_lgenCopyMac = m_plgfilehdr->lGeneration;
 *      m_lgenCopyMic = fFullBackup ? set befor database copy : m_lgenDeleteMic.
 *      m_lgenDeleteMic = first generation in the current log file path
 *      m_lgenDeleteMac = current checkpoint, which may be several gen less than m_lgenCopyMac
 */
ERR BACKUP_CONTEXT::ErrBKIPrepareLogFiles(
    JET_GRBIT                   grbit,
    _In_ PCWSTR                 wszLogFilePath,
    __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) PWSTR   wszPathJetChkLog,
    _In_ PCWSTR                 wszBackupPath )
{
    ERR         err;
    CHECKPOINT  *pcheckpointT = NULL;
    LGPOS       lgposRecT;
    LOG *       plog = m_pinst->m_plog;

    const BOOL  fFullBackup = ( 0 == grbit );
    const BOOL  fIncrementalBackup = ( JET_bitBackupIncremental == grbit );

    Assert ( 0 == grbit || JET_bitBackupIncremental == grbit );

    Assert( m_fBackupInProgressLocal );

    // We now allow backups during recovery (aka "seed from passive"), but we should not get this far in that mode.
    // However if we did, we would need to avoid logging anything here, and possibly wait for a log to roll on to roll on it's own.
    Enforce( !m_pinst->FRecovering() );

    // Note we don't actually know m_lgenCopyMic / m_lgenCopyMac here, b/c we need to 
    // rotate the log first, and this log op is what does that. 
    if ( fFullBackup )
    {
        LGIGetDateTime( &m_logtimeFullBackup );
        CallR( ErrLGBackupPrepLogs( plog, DBFILEHDR::backupNormal, !fFullBackup, &m_logtimeFullBackup, fLGCreateNewGen, &lgposRecT ) );
        m_lgposFullBackup = lgposRecT;
    }
    else
    {
        Assert( fIncrementalBackup );
        LGIGetDateTime( &m_logtimeIncBackup );
        CallR( ErrLGBackupPrepLogs( plog, DBFILEHDR::backupNormal, !fFullBackup, &m_logtimeIncBackup, fLGCreateNewGen, &lgposRecT ) );
        m_lgposIncBackup = lgposRecT;
    }

    Call( plog->ErrLGWaitForLogGen( lgposRecT.lGeneration ) );

    m_fBackupBeginNewLogFile = fTrue;

    /*  compute m_lgenCopyMac:
    /*  copy all log files up to but not including current log file
    /**/
    Assert( m_lgenCopyMac == 0 );
    m_lgenCopyMac = plog->LGGetCurrentFileGenWithLock();
    Assert( m_lgenCopyMac != 0 );
    m_cGenCopyDone = 0;

    /*  set m_lgenDeleteMic
    /*  to first log file generation number.
    /**/
    Assert( m_lgenDeleteMic == 0 );
    Call ( plog->ErrLGGetGenerationRange( wszLogFilePath, &m_lgenDeleteMic, NULL ) );
    if ( 0 == m_lgenDeleteMic )
    {
        Error( ErrERRCheck( JET_errFileNotFound ) );
    }
    Assert( m_lgenDeleteMic != 0 );

    if ( fIncrementalBackup && wszBackupPath )
    {
        LONG lgenT;
        /*  validate incremental backup against previous
        /*  full and incremenal backup.
        /**/
        Call ( plog->ErrLGGetGenerationRange( wszBackupPath, NULL, &lgenT ) );
        if ( m_lgenDeleteMic > lgenT + 1 )
        {
            Call( ErrERRCheck( JET_errInvalidLogSequence ) );
        }
    }

    if ( fIncrementalBackup )
    {
        Call ( ErrBKICheckLogsForIncrementalBackup( m_lgenDeleteMic ) );
    }

    /*  set m_lgenDeleteMac to checkpoint log file
    /**/
    AllocR( pcheckpointT = (CHECKPOINT *) PvOSMemoryPageAlloc( sizeof(CHECKPOINT), NULL ) );

    plog->LGFullNameCheckpoint( wszPathJetChkLog );
    Call( plog->ErrLGReadCheckpoint( wszPathJetChkLog, pcheckpointT, fTrue ) );
    Assert( m_lgenDeleteMac == 0 );
    m_lgenDeleteMac = pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration;

    // if the checkpoint just advanced after the m_lgenCopyMac computation above
    // we need to keep the logs around from the truncation point (m_lgenCopyMac)
    // as we might need them for an incremental backup
    m_lgenDeleteMac = min( m_lgenDeleteMac, m_lgenCopyMac);
    Assert( m_lgenDeleteMic <= m_lgenDeleteMac );

    // calculate delete range the databases considering the
    // databases that were not involved in the backup process
    //
    if ( fFullBackup )
    {
        for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            IFMP    ifmp = m_pinst->m_mpdbidifmp[ dbid ];
            if ( ifmp >= g_ifmpMax )
                continue;

            FMP *pfmp = &g_rgfmp[ifmp];


            if ( pfmp->FInUse()
                && pfmp->FLogOn()
                && pfmp->FAttached()
                && !pfmp->FAttachedForRecovery()
                && !pfmp->FAttachingDB()
                && !(pfmp->FInBackupSession() &&
                        (!fFullBackup || pfmp->FBackupFileCopyDone())))
            {
                Assert( !pfmp->FSkippedAttach() );
                Assert( !pfmp->FDeferredAttach() );

                ULONG genLastFullBackupForDb = (ULONG) pfmp->Pdbfilehdr()->bkinfoFullPrev.le_genHigh;
//              ULONG genLastIncBackupForDb = (ULONG) pfmp->Pdbfilehdr()->bkinfoIncPrev.le_genHigh;

//              m_lgenDeleteMac = min ( m_lgenDeleteMac, max( genLastFullBackupForDb, genLastIncBackupForDb ) );
                // if one database not in this full backup has a differential backup
                // (which shows as an incremental for JET), we will truncate the logs. We don't want this
                // (the log truncation must be done using a full or incremental backup)
                // In other words: don't consider Copy backups, those aren't supposed to affect the log sequence
                m_lgenDeleteMac = min( (ULONG)m_lgenDeleteMac, genLastFullBackupForDb );
            }
        }
        m_lgenDeleteMic = min( m_lgenDeleteMac, m_lgenDeleteMic );
    }


    /*  compute m_lgenCopyMic
    /**/
    if ( fFullBackup )
    {
        /*  m_lgenCopyMic set before database copy
        /**/
        Assert( m_lgenCopyMic != 0 );
    }
    else
    {
        Assert ( fIncrementalBackup );
        /*  copy all files that are deleted for incremental backup
        /**/
        Assert( m_lgenDeleteMic != 0 );
        m_lgenCopyMic = m_lgenDeleteMic;
    }

    Assert ( m_lgenDeleteMic <= m_lgenDeleteMac );
    Assert ( m_lgenCopyMic <= m_lgenCopyMac );

    // report start backup of log file
    {
        WCHAR           wszFullLogNameCopyMic[IFileSystemAPI::cchPathMax];
        WCHAR           wszFullLogNameCopyMac[IFileSystemAPI::cchPathMax];
        const WCHAR *   rgszT[]         = { wszFullLogNameCopyMic, wszFullLogNameCopyMac };
        const INT       cbFillBuffer    = 128;
        CHAR            szTrace[cbFillBuffer + 1];

        plog->LGMakeLogName( wszFullLogNameCopyMic,
                             sizeof( wszFullLogNameCopyMic ),
                             eArchiveLog,
                             m_lgenCopyMic );

        Assert ( m_lgenCopyMic < m_lgenCopyMac );
        plog->LGMakeLogName( wszFullLogNameCopyMac,
                             sizeof( wszFullLogNameCopyMac ),
                             eArchiveLog,
                             m_lgenCopyMac - 1 );

        UtilReportEvent(
            eventInformation,
            LOGGING_RECOVERY_CATEGORY,
            BACKUP_LOG_FILES_START,
            2,
            rgszT,
            0,
            NULL,
            m_pinst );

        OSStrCbFormatA(
            szTrace,
            cbFillBuffer,
            "BACKUP PREPARE LOGS (copy 0x%05X-0x%05X, delete 0x%05X-0x%05X)",
            m_lgenCopyMic,
            m_lgenCopyMac - 1,
            m_lgenDeleteMic,
            m_lgenDeleteMac );
        CallS( plog->ErrLGTrace( m_ppibBackup, szTrace ) );
    }

HandleError:
    // UNDONE: if this function failed, we should stop the backup
    // not relay on the backup client behaviour and on or on the
    // fact that the code in log copy/truncate part will deal with
    // the state in which the backup is.

    OSMemoryPageFree( pcheckpointT );
    return err;
}


ERR ErrLGCheckIncrementalBackup( INST *pinst, DBFILEHDR::BKINFOTYPE backupType )
{
    DBID dbid;
    const BKINFO *pbkinfo;

    // We treat surrogate backup like OSSnapshot backup, because it is a OS snapshot backup, but on
    // the passive node of a log shipping cluster.
    backupType = ( backupType == DBFILEHDR::backupSurrogate ) ? DBFILEHDR::backupOSSnapshot : backupType;

    for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP *pfmp = &g_rgfmp[ifmp];

        /*  make sure all the attached DB are qualified for incremental backup.
         */
        if ( pfmp->FAttached() && !pfmp->FAttachedForRecovery() && !pfmp->FAttachingDB() )
        {
            Assert( pfmp->Pdbfilehdr() );
            Assert( !pfmp->FSkippedAttach() );
            Assert( !pfmp->FDeferredAttach() );
            pbkinfo = &pfmp->Pdbfilehdr()->bkinfoFullPrev;
            if ( pbkinfo->le_genLow == 0 ||
                // we have a full backup but it is not matching the backup type we want to do
                (pbkinfo->le_genLow != 0 && backupType != pfmp->Pdbfilehdr()->bkinfoTypeFullPrev ) )
            {
                const UINT  csz = 1;
                const WCHAR *rgszT[csz];
                rgszT[0] = pfmp->WszDatabaseName();
                UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                    DATABASE_MISS_FULL_BACKUP_ERROR_ID, csz, rgszT, 0, NULL, pinst );
                return ErrERRCheck( JET_errMissingFullBackup );
            }
        }
    }
    return JET_errSuccess;
}


ERR BACKUP_CONTEXT::ErrBKICheckLogsForIncrementalBackup( LONG lGenMinExisting )
{
    DBID dbid;

    for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP *pfmp = &g_rgfmp[ifmp];

        if ( pfmp->FAttached() && !pfmp->FAttachedForRecovery() && !pfmp->FAttachingDB() )
        {
            Assert( pfmp->Pdbfilehdr() );
            Assert( !pfmp->FSkippedAttach() );
            Assert( !pfmp->FDeferredAttach() );

            PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr();

            // if we have both backup (full and incremental), then those should be of the
            // same type (insured by the backup logic)
            Assert( 0 == pdbfilehdr->bkinfoFullPrev.le_genHigh ||
                   0 == pdbfilehdr->bkinfoIncPrev.le_genHigh ||
                   pdbfilehdr->bkinfoTypeFullPrev == pdbfilehdr->bkinfoTypeIncPrev );

            LONG lGenMaxBackup = max (  pdbfilehdr->bkinfoFullPrev.le_genHigh,
                                        pdbfilehdr->bkinfoIncPrev.le_genHigh );

            if ( lGenMinExisting > lGenMaxBackup  + 1 )
            {
                const UINT      csz = 1;
                const WCHAR *   rgszT[csz];
                WCHAR           wszFullLogName[IFileSystemAPI::cchPathMax];

                rgszT[0] = wszFullLogName;

                m_pinst->m_plog->LGMakeLogName(
                        wszFullLogName,
                        sizeof( wszFullLogName ),
                        eArchiveLog,
                        lGenMaxBackup + 1 );

                UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                        BACKUP_LOG_FILE_MISSING_ERROR_ID, csz, rgszT, 0, NULL, m_pinst );

                return ErrERRCheck( JET_errMissingFileToBackup );
            }
        }
    }
    return JET_errSuccess;
}


/*  copies database files and logfile generations starting at checkpoint
 *  record to directory specified by the environment variable BACKUP.
 *  No writing or switching of log generations is involved.
 *  The Backup call may be issued at any time, and does not interfere
 *  with the normal functioning of the system - nothing gets locked.
 *
 *  The database page is copied page by page in page sequence number. If
 *  a copied page is dirtied after it is copied, the page has to be
 *  recopied again. A flag is indicated if a database is being copied. If
 *  BufMan is writing a dirtied page and the page is copied, then BufMan
 *  has to copy the dirtied page to both the backup copy and the current
 *  database.
 *
 *  If the copy is later used to Restore without a subsequent log file, the
 *  restored database will be consistent and will include any transaction
 *  committed prior to backing up the very last log record; if there is a
 *  subsequent log file, that file will be used during Restore as a
 *  continuation of the backed-up log file.
 *
 *  PARAMETERS
 *
 *  RETURNS
 *      JET_errSuccess, or the following error codes:
 *          JET_errNoBackupDirectory
 *          JET_errFailCopyDatabase
 *          JET_errFailCopyLogFile
 *
 */
ERR ISAMAPI ErrIsamBackup(
    JET_INSTANCE jinst,
    const WCHAR* wszPathName,
    JET_GRBIT grbit,
    JET_PFNINITCALLBACK pfnStatus,
    void * pvStatusContext )
{
    ERR             err = JET_errSuccess;
    INST * const    pinst   = (INST *)jinst;
    WCHAR           wszBackup[IFileSystemAPI::cchPathMax];

    CallR( pinst->m_pfsapi->ErrPathComplete( wszPathName, wszBackup ) );
    return pinst->m_pbackup->ErrBKBackup( wszBackup, grbit, pfnStatus, pvStatusContext );
}

ERR ErrLGIRemoveTempDir(
    IFileSystemAPI *const pfsapi,
    __out_bcount(cbResultPath) PWSTR wszResultPath,
    ULONG cbResultPath,
    _In_ PWSTR wszBackupPath,
    const WCHAR *wszTempDirIn )
{
    ERR err;

    /*  backup directory
    /**/
    Assert( LOSStrLengthW( wszBackupPath ) + LOSStrLengthW( wszTempDirIn ) < IFileSystemAPI::cchPathMax - 1 );
    if ( LOSStrLengthW( wszBackupPath ) + LOSStrLengthW( wszTempDirIn ) >= IFileSystemAPI::cchPathMax - 1 )
    {
        return ErrERRCheck( JET_errInvalidPath );
    }

    OSStrCbCopyW( wszResultPath, cbResultPath, wszBackupPath );
    OSStrCbAppendW( wszResultPath, cbResultPath, wszTempDirIn );
    CallR( ErrLGDeleteAllFiles( pfsapi, wszResultPath, cbResultPath ) );
    OSStrCbCopyW( wszResultPath, cbResultPath, wszBackupPath );
    OSStrCbAppendW( wszResultPath, cbResultPath, wszTempDirIn );
    err = pfsapi->ErrFolderRemove( wszResultPath );
    return err == JET_errInvalidPath ? JET_errSuccess : err;
}

#define cpageBackupBufferMost   256

#define JET_INVALID_HANDLE  JET_HANDLE(-1)
ERR BACKUP_CONTEXT::ErrBKICopyFile(
    const WCHAR *wszFileName,
    const WCHAR *wszBackup,
    JET_PFNINITCALLBACK pfnStatus,
    void * pvStatusContext,
    const BOOL fOverwriteExisting    )
{
    ERR         err                 = JET_errSuccess;
    JET_HANDLE  hfFile              = JET_INVALID_HANDLE;
    QWORD       qwFileSize          = 0;
    QWORD       ibOffset            = 0;

    DWORD       cbBuffer            = cpageBackupBufferMost * g_cbPage;
    VOID *      pvBuffer            = NULL;
    IFileAPI *  pfapiBackupDest     = NULL;

    if ( ((0xFFFFFFFF / cpageBackupBufferMost) <= g_cbPage) ||
         (cbBuffer < (DWORD)max(cpageBackupBufferMost, g_cbPage)) )
    {
        return( JET_errOutOfMemory );
    }

    // UNDONE: keep status

    {
    ULONG                       ulFileSizeLow       = 0;
    ULONG                       ulFileSizeHigh      = 0;
    CallR( ErrBKOpenFile( wszFileName, &hfFile, 0, &ulFileSizeLow, &ulFileSizeHigh, m_fBackupFull ) );
    qwFileSize = ( QWORD(ulFileSizeHigh) << 32) + QWORD(ulFileSizeLow);
    if ( 0 == ulFileSizeHigh )
    {
        cbBuffer = min( cbBuffer, ulFileSizeLow);
    }
    }

    if ( cbBuffer != 0 )
    {
        Alloc( pvBuffer = PvOSMemoryPageAlloc( cbBuffer, NULL ) );
    }

    {
    WCHAR           wszDirT[ IFileSystemAPI::cchPathMax ];
    WCHAR           wszExtT[ IFileSystemAPI::cchPathMax ];
    WCHAR           wszFNameT[ IFileSystemAPI::cchPathMax ];
    WCHAR           wszBackupDest[ IFileSystemAPI::cchPathMax ];
    WCHAR           wszBackupPath[IFileSystemAPI::cchPathMax];

    CallS( m_pinst->m_pfsapi->ErrPathParse( wszFileName, wszDirT, wszFNameT, wszExtT ) );

    //  backup directory (should be pre-validated, but be defensive
    //  and handle it just in case)
    //
    Assert( LOSStrLengthW( wszBackup ) < IFileSystemAPI::cchPathMax - 1 );
    OSStrCbCopyW( wszBackupPath, IFileSystemAPI::cchPathMax * sizeof( WCHAR ), wszBackup );
    if ( LOSStrLengthW( wszBackup ) >= IFileSystemAPI::cchPathMax - 1 )
    {
        //  no room for trailing path delimiter, so just
        //  ensure null termination
        //
        wszBackupPath[ IFileSystemAPI::cchPathMax - 1 ] = 0;
    }
    else
    {
        CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszBackupPath, sizeof(wszBackupPath) ) );
    }

    LGMakeName( m_pinst->m_pfsapi, wszBackupDest, wszBackupPath, wszFNameT, wszExtT );

    Assert( hfFile != JET_INVALID_HANDLE );
    const INT irhf = (INT)hfFile;
    QWORD qwEseFileID = 0;

    if ( m_rgrhf[irhf].fDatabase )
    {
        Assert( !m_rgrhf[irhf].fIsLog );
        Assert( m_rgrhf[irhf].ifmp != 0 && m_rgrhf[irhf].ifmp != ifmpNil );
        qwEseFileID = qwBackupDbID | m_rgrhf[irhf].ifmp;
    }
    else if ( m_rgrhf[irhf].fIsLog )
    {
        Assert( !m_rgrhf[irhf].fDatabase );
        qwEseFileID = QwInstFileID( qwBackupLogID, m_pinst->m_iInstance, m_rgrhf[irhf].lGeneration );
    }
    else
    {
        // thunk patch file.
        qwEseFileID = qwPatchFileID;
    }
    // Where is checkpoint file?  As fIsLog?  or else case?
    
    Call( CIOFilePerf::ErrFileCreate(
                            m_pinst->m_pfsapi,
                            m_pinst,
                            wszBackupDest,
                            fOverwriteExisting ? IFileAPI::fmfOverwriteExisting : IFileAPI::fmfNone,
                            iofileOther,
                            qwEseFileID,
                            &pfapiBackupDest ) );
    }

    Assert ( pfapiBackupDest );

    while ( qwFileSize )
    {
        ULONG cbActual;

        if ( m_pinst->m_fTermInProgress )
        {
            Error( ErrERRCheck( JET_errTermInProgress ) );
        }

        Call ( ErrBKReadFile( hfFile, pvBuffer, cbBuffer, &cbActual ));

        Call( pfapiBackupDest->ErrIOWrite( *TraceContextScope( iorpBackup ), ibOffset, cbActual, (BYTE*)pvBuffer, QosSyncDefault( m_pinst ) ) );

        ibOffset += QWORD(cbActual);
        qwFileSize -= QWORD(cbActual);
    }

    Assert ( JET_INVALID_HANDLE != hfFile );
    Call ( ErrBKCloseFile( hfFile ) );
    hfFile = JET_INVALID_HANDLE;
    Assert ( JET_errSuccess == err );

HandleError:

    if ( NULL != pvBuffer )
    {
        OSMemoryPageFree( pvBuffer );
        pvBuffer = NULL;
    }

    if ( pfapiBackupDest )
    {
        pfapiBackupDest->SetNoFlushNeeded();
    }
    delete pfapiBackupDest;

    if ( JET_INVALID_HANDLE != hfFile )
    {
        (void) ErrBKCloseFile( hfFile );
        hfFile = JET_INVALID_HANDLE;
    }
    return err;
}

ERR BACKUP_CONTEXT::ErrBKIPrepareDirectory(
    _In_ PCWSTR wszBackup,
    __out_bcount(cbBackupPath) PWSTR wszBackupPath,
    const size_t cbBackupPath,
    JET_GRBIT grbit )
{
    ERR             err             = JET_errSuccess;
    const BOOL      fFullBackup     = !( grbit & JET_bitBackupIncremental );
    const BOOL      fBackupAtomic   = ( grbit & JET_bitBackupAtomic );

    WCHAR           wszT[IFileSystemAPI::cchPathMax];
    WCHAR           wszFrom[IFileSystemAPI::cchPathMax];

    /*  backup directory
    /**/
    Assert( LOSStrLengthW( wszBackup ) < IFileSystemAPI::cchPathMax - 1 );
    if ( ErrOSStrCbCopyW( wszBackupPath, cbBackupPath, wszBackup ) < JET_errSuccess )
    {
        return ErrERRCheck( JET_errInvalidPath );
    }
    if ( LOSStrLengthW( wszBackup ) < IFileSystemAPI::cchPathMax - 1 )
    {
        // there is room for the trailing path delimiter
        CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszBackupPath, cbBackupPath ) );
    }

    if ( BoolParam( m_pinst, JET_paramCreatePathIfNotExist ) )
    {
        Call( ErrUtilCreatePathIfNotExist( m_pinst->m_pfsapi, wszBackupPath, NULL, 0 ) );
    }

    /*  reconsist atomic backup directory
    /*  1)  if temp directory, delete temp directory
    /**/
    Call( ErrLGIRemoveTempDir( m_pinst->m_pfsapi, wszT, sizeof(wszT), wszBackupPath, wszTempDir ) );

    if ( fBackupAtomic )
    {
        /*  2)  if old and new directories, delete old directory
        /*  3)  if new directory, move new to old
        /*
        /*  Now we should have an empty direcotry, or a directory with
        /*  an old subdirectory with a valid backup.
        /*
        /*  4) make a temporary directory for the current backup.
        /**/
        err = ErrLGCheckDir( m_pinst->m_pfsapi, wszBackupPath, cbBackupPath, wszAtomicNew );
        if ( err == JET_errBackupDirectoryNotEmpty )
        {
            if ( LOSStrLengthW( wszBackupPath ) + LOSStrLengthW( wszAtomicOld ) + 1 >= IFileSystemAPI::cchPathMax - 1 )
            {
                Error( ErrERRCheck( JET_errInvalidPath ) );
            }
            OSStrCbCopyW( wszT, sizeof(wszT), wszBackupPath );
            OSStrCbAppendW( wszT, sizeof(wszT), wszAtomicOld );
            CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszT, sizeof(wszT) ) );
            Call( ErrLGDeleteAllFiles( m_pinst->m_pfsapi, wszT, sizeof(wszT) ) );

            Assert( LOSStrLengthW( wszBackupPath ) + LOSStrLengthW( wszAtomicOld ) < IFileSystemAPI::cchPathMax - 1 );
            OSStrCbCopyW( wszT, sizeof(wszT), wszBackupPath );
            OSStrCbAppendW( wszT, sizeof(wszT), wszAtomicOld );
            err = m_pinst->m_pfsapi->ErrFolderRemove( wszT );
            Call( err == JET_errInvalidPath ? JET_errSuccess : err );

            if ( LOSStrLengthW( wszBackupPath ) + LOSStrLengthW( wszAtomicNew ) + 1 >= IFileSystemAPI::cchPathMax - 1 )
            {
                Error( ErrERRCheck( JET_errInvalidPath ) );
            }
            OSStrCbCopyW( wszFrom, sizeof(wszFrom), wszBackupPath );
            OSStrCbAppendW( wszFrom, sizeof(wszFrom), wszAtomicNew );
            CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszT, sizeof(wszT) ) );
            Call( m_pinst->m_pfsapi->ErrFileMove( wszFrom, wszT ) );
        }

        /*  if incremental, set backup directory to szAtomicOld
        /*  else create and set to szTempDir
        /**/
        if ( !fFullBackup )
        {
            /*  backup to old directory
            /**/
            if ( LOSStrLengthW( wszBackupPath ) + LOSStrLengthW( wszAtomicOld ) + 1 >= IFileSystemAPI::cchPathMax - 1 )
            {
                Call( ErrERRCheck( JET_errInvalidPath ) );
            }
            OSStrCbCopyW( wszBackupPath, cbBackupPath, wszAtomicOld );
            CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszBackupPath, cbBackupPath ) );
        }
        else
        {
            if ( LOSStrLengthW( wszBackupPath ) + LOSStrLengthW( wszTempDir ) + 1 >= IFileSystemAPI::cchPathMax - 1 )
            {
                Call( ErrERRCheck( JET_errInvalidPath ) );
            }
            OSStrCbCopyW( wszT, sizeof(wszT), wszBackupPath );
            OSStrCbAppendW( wszT, sizeof(wszT), wszTempDir );
            CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszT, sizeof(wszT) ) );
            err = m_pinst->m_pfsapi->ErrFolderCreate( wszT );
            if ( err < 0 )
            {
                Call( ErrERRCheck( JET_errMakeBackupDirectoryFail ) );
            }

            /*  backup to temp directory
            /**/
            OSStrCbAppendW( wszBackupPath, cbBackupPath, wszTempDir );
        }
    }
    else
    {
        if ( !fFullBackup )
        {
            /*  check for non-atomic backup directory empty
            /**/
            Call( ErrLGCheckDir( m_pinst->m_pfsapi, wszBackupPath, cbBackupPath, wszAtomicNew ) );
            Call( ErrLGCheckDir( m_pinst->m_pfsapi, wszBackupPath, cbBackupPath, wszAtomicOld ) );
        }
        else
        {
            /*  check for backup directory empty
            /**/
            Call( ErrLGCheckDir( m_pinst->m_pfsapi, wszBackupPath, cbBackupPath, NULL ) );
        }
    }

    HandleError:
        return err;
    }


ERR BACKUP_CONTEXT::ErrBKIPromoteDirectory(
    _In_ PCWSTR wszBackup,
    __out_bcount(OSFSAPI_MAX_PATH * sizeof(WCHAR)) PWSTR wszBackupPath,
    JET_GRBIT grbit )
{
    ERR             err             = JET_errSuccess;
    const BOOL      fFullBackup     = !( grbit & JET_bitBackupIncremental );
    const BOOL      fBackupAtomic   = ( grbit & JET_bitBackupAtomic );

    //  for full backup, graduate temp backup to new backup and delete old backup.

    if ( !fBackupAtomic || !fFullBackup )
        return JET_errSuccess;

    WCHAR           wszFrom[IFileSystemAPI::cchPathMax];
    WCHAR           wszT[IFileSystemAPI::cchPathMax];

    //  backup directory (should be pre-validated, but be defensive
    //  and handle it just in case)
    //
    Assert( LOSStrLengthW( wszBackupPath ) < IFileSystemAPI::cchPathMax );
    OSStrCbCopyW( wszFrom, sizeof(wszFrom) ,wszBackupPath );
    wszFrom[ IFileSystemAPI::cchPathMax - 1 ] = 0;

    /*  reset backup path
    /**/

    if ( LOSStrLengthW(wszBackupPath) < LOSStrLengthW(wszTempDir) )
    {
        Error( ErrERRCheck( JET_errInvalidPath ) );
    }
    wszBackupPath[LOSStrLengthW(wszBackupPath) - LOSStrLengthW(wszTempDir)] = L'\0';


    Assert( LOSStrLengthW( wszBackupPath ) + LOSStrLengthW( wszAtomicNew ) + 1 < IFileSystemAPI::cchPathMax - 1 );
    if ( LOSStrLengthW( wszBackupPath ) + LOSStrLengthW( wszAtomicNew ) + 1 >= IFileSystemAPI::cchPathMax - 1 )
    {
        Error( ErrERRCheck( JET_errInvalidPath ) );
    }
    OSStrCbCopyW( wszT, sizeof(wszT), wszBackupPath );
    OSStrCbAppendW( wszT, sizeof(wszT), wszAtomicNew );
    err = m_pinst->m_pfsapi->ErrFileMove( wszFrom, wszT );
    if ( JET_errFileNotFound == err )
    {
        err = JET_errSuccess;
    }
    Call( err );

    Assert( LOSStrLengthW( wszBackupPath ) + LOSStrLengthW( wszAtomicOld ) + 1 < IFileSystemAPI::cchPathMax - 1 );
    if ( LOSStrLengthW( wszBackupPath ) + LOSStrLengthW( wszAtomicOld ) + 1 >= IFileSystemAPI::cchPathMax - 1 )
    {
        Error( ErrERRCheck( JET_errInvalidPath ) );
    }
    OSStrCbCopyW( wszT, sizeof(wszT), wszBackupPath );
    OSStrCbAppendW( wszT, sizeof(wszT), wszAtomicOld );
    CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszT, sizeof(wszT) ) );
    Call( ErrLGDeleteAllFiles( m_pinst->m_pfsapi, wszT, sizeof(wszT) ) );

    Assert( LOSStrLengthW( wszBackupPath ) + LOSStrLengthW( wszAtomicOld ) + 1 < IFileSystemAPI::cchPathMax - 1 );
    if ( LOSStrLengthW( wszBackupPath ) + LOSStrLengthW( wszAtomicOld ) + 1 >= IFileSystemAPI::cchPathMax - 1 )
    {
        Error( ErrERRCheck( JET_errInvalidPath ) );
    }
    OSStrCbCopyW( wszT, sizeof(wszT), wszBackupPath );
    OSStrCbAppendW( wszT, sizeof(wszT), wszAtomicOld );
    err = m_pinst->m_pfsapi->ErrFolderRemove( wszT );
    if ( JET_errInvalidPath == err )
    {
        err = JET_errSuccess;
    }

    HandleError:
        return err;
}

ERR BACKUP_CONTEXT::ErrBKICleanupDirectory(
    _In_ PCWSTR wszBackup,
    __out_bcount(cbBackupPath) PWSTR wszBackupPath,
    size_t cbBackupPath )
{
    ERR         err = JET_errSuccess;

    if ( wszBackup && wszBackup[0] )
    {
        WCHAR   wszT[IFileSystemAPI::cchPathMax];

        Assert( (size_t)LOSStrLengthW( wszBackup ) < cbBackupPath - 1 );
        OSStrCbCopyW( wszBackupPath, cbBackupPath, wszBackup );

        CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszBackupPath, cbBackupPath ) );
        //  apparently, it is ok if there is no room for trailing path delimiter?
        //  not sure why, but didn't bother changing it, when I moved to non-banned APIs.

        CallS( ErrLGIRemoveTempDir( m_pinst->m_pfsapi, wszT, sizeof(wszT), wszBackupPath, wszTempDir ) );
    }

    return err;
}

ERR BACKUP_CONTEXT::ErrBKBackup(
    const WCHAR *wszBackup,
    JET_GRBIT grbit,
    JET_PFNINITCALLBACK pfnStatus,
    void * pvStatusContext )
{
    ERR         err = JET_errSuccess;

    BOOL        fFullBackup = !( grbit & JET_bitBackupIncremental );

    JET_SNPROG  snprog = { 0 };
    BOOL        fShowStatus = fFalse;
    LOG *       plog = m_pinst->m_plog;

    WCHAR       wszBackupPath[IFileSystemAPI::cchPathMax];

    ULONG           cInstanceInfo   = 0;
    JET_INSTANCE_INFO_W *   aInstanceInfo   = NULL;
    JET_INSTANCE_INFO_W *   pInstanceInfo   = NULL;
    WCHAR *                 wszNames        = NULL;

    if ( plog->FLogDisabled() )
    {
        return ErrERRCheck( JET_errLoggingDisabled );
    }

    if ( plog->FNoMoreLogWrite() )
    {
        return ErrERRCheck( JET_errLogWriteFail );
    }

    if ( !fFullBackup && BoolParam( m_pinst, JET_paramCircularLog ) )
    {
        return ErrERRCheck( JET_errInvalidBackup );
    }

    CallR( ErrBKBeginExternalBackup( ( fFullBackup ? 0 : JET_bitBackupIncremental ) ) );
    Assert ( m_fBackupInProgressLocal );

    /*  if NULL backup directory then just delete log files
    /**/
    if ( wszBackup == NULL || wszBackup[0] == L'\0' )
    {
        //  set lgenDeleteMac to current checkpoint
        m_lgenDeleteMac = plog->LgposGetCheckpoint().le_lGeneration;

        //  if circular logging is not enabled, try to set lgenDeleteMic
        //  to first log file generation number
        //  if circular logging is enabled, no point in truncating
        //  logs (it will just happen naturally)
        m_lgenDeleteMic = 0;

        if ( !BoolParam( m_pinst, JET_paramCircularLog ) )
        {
            //  ignore any errors (we will just force TruncateLog
            //  not to do anything)
            (void)plog->ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), &m_lgenDeleteMic, NULL );
        }

        if ( 0 == m_lgenDeleteMic )
        {
            //  this will force TruncateLog not to do anything
            m_lgenDeleteMic = m_lgenDeleteMac;
        }

        goto DeleteLogs;
    }

    //  ensure path is not too long (-1 because we have subsequent
    //  code all over the place that will be appending a trailing
    //  path delimiter to this, but even if a trailing path delimiter
    //  is already present, the check is still valid because we will
    //  also be appending various filenames)
    //
    if ( LOSStrLengthW( wszBackup ) >= IFileSystemAPI::cchPathMax - 1 )
    {
        Call( ErrERRCheck( JET_errInvalidPath ) );
    }

    Call ( ErrBKIPrepareDirectory( wszBackup, wszBackupPath, sizeof( wszBackupPath ), grbit ) );
    if ( !fFullBackup )
    {
        goto CopyLogFiles;
    }

    /*  full backup
    /**/
    Assert( fFullBackup );

    /*  initialize status
    /**/
    fShowStatus = ( pfnStatus != NULL );
    if ( fShowStatus )
    {
        Assert( 0 == snprog.cunitDone );
        snprog.cbStruct = sizeof(JET_SNPROG);
        snprog.cunitTotal = 100;

        /*  status callback
        /**/
        (*pfnStatus)( JET_snpBackup, JET_sntBegin, &snprog, pvStatusContext );
    }
    Call ( ErrIsamGetInstanceInfo( &cInstanceInfo, &aInstanceInfo, NULL ) );

    // find the instance and backup all database file: edb
    {
    pInstanceInfo = NULL;
    for ( ULONG iInstanceInfo = 0; iInstanceInfo < cInstanceInfo && !pInstanceInfo; iInstanceInfo++)
    {
        if ( aInstanceInfo[iInstanceInfo].hInstanceId == (JET_INSTANCE)m_pinst )
        {
            pInstanceInfo = aInstanceInfo + iInstanceInfo;
        }
    }
    // we should find at least the instance in which we are running
    AssertRTL ( pInstanceInfo );

    for ( ULONG_PTR iDatabase = 0; iDatabase < pInstanceInfo->cDatabases; iDatabase++ )
    {
        Assert ( pInstanceInfo->szDatabaseFileName );
        Assert ( pInstanceInfo->szDatabaseFileName[iDatabase] );
        Call ( ErrBKICopyFile( pInstanceInfo->szDatabaseFileName[iDatabase], wszBackupPath, pfnStatus, pvStatusContext ) );
    }
    }
    /*  successful copy of all the databases */

CopyLogFiles:
    {
    ULONG           cbNames;

    Call ( ErrBKGetLogInfo( NULL, 0, &cbNames, NULL, fFullBackup ) );
    Alloc( wszNames = (WCHAR *)PvOSMemoryPageAlloc( cbNames, NULL ) );

    Call ( ErrBKGetLogInfo( wszNames, cbNames, NULL, NULL, fFullBackup ) );
    Assert ( wszNames );
    }

    // now backup all files (patch and logs) in szNames
    WCHAR *wszNamesWalking;
    wszNamesWalking = wszNames;
    while ( *wszNamesWalking )
    {
        Call ( ErrBKICopyFile( wszNamesWalking, wszBackupPath, pfnStatus, pvStatusContext, fTrue ) );
        wszNamesWalking += LOSStrLengthW( wszNamesWalking ) + 1;
    }

    Call ( ErrBKIPromoteDirectory( wszBackup, wszBackupPath, grbit ) );

DeleteLogs:
    Assert( err == JET_errSuccess );
    Call ( ErrBKTruncateLog() );

    /*  complete status update
    /**/
    if ( fShowStatus )
    {
        Assert( snprog.cbStruct == sizeof(snprog) );
        Assert( snprog.cunitTotal == 100 );
        snprog.cunitDone = 100;
        (*pfnStatus)( JET_snpBackup, JET_sntComplete, &snprog, pvStatusContext );
    }

HandleError:

{
    ERR errT;
    errT = ErrIsamEndExternalBackup( (JET_INSTANCE)m_pinst, ( JET_errSuccess <= err )?JET_bitBackupEndNormal:JET_bitBackupEndAbort );
    if ( JET_errSuccess <= err )
    {
        err = errT;
    }
}

    if ( aInstanceInfo )
    {
        JetFreeBuffer( (char *)aInstanceInfo );
        aInstanceInfo = NULL;
    }

    if ( wszNames )
    {
        OSMemoryPageFree( wszNames );
        wszNames = NULL;
    }

    CallS ( ErrBKICleanupDirectory( wszBackup, wszBackupPath, sizeof(wszBackupPath) ) );

    return err;
}


#ifdef DEBUG
VOID DBGBRTrace( _In_ PCSTR sz )
{
    DBGprintf( "%s", sz );
}
#endif


/***********************************************************
/******************* SURROGATE BACKUP *********************/

ERR ISAMAPI ErrIsamBeginSurrogateBackup(
    _In_        JET_INSTANCE    jinst,
    _In_        ULONG           lgenFirst,
    _In_        ULONG           lgenLast,
    _In_        JET_GRBIT       grbit )
{
    INST *  pinst = (INST *)jinst;
    ERR     err;
    ULONG   lgenTip;

    // Validate the log range the client thinks they want to backup.

    if ( lgenFirst > lgenLast )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    lgenTip = pinst->m_plog->LGGetCurrentFileGenWithLock();

    if ( lgenTip < lgenLast )
    {
        // we'll assert() in case it is useful to block here.
        AssertSz( fFalse, "Uh-oh!  The replica is ahead of us!" );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    CallR( pinst->m_pbackup->ErrBKBeginExternalBackup( JET_bitBackupSurrogate | grbit, lgenFirst, lgenLast ) );

    return( err );

}

ERR ISAMAPI ErrIsamEndSurrogateBackup(
    __inout JET_INSTANCE    jinst,
    _In_        JET_GRBIT       grbit )
{
    INST *  pinst = (INST *)jinst;
    BOOL    fNormalTermination = grbit & JET_bitBackupEndNormal;
    ERR         err = JET_errSuccess;

    if ( fNormalTermination && (grbit & JET_bitBackupEndAbort) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( !fNormalTermination && !(grbit & JET_bitBackupEndAbort) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    Assert ( (grbit & JET_bitBackupEndNormal) || (grbit & JET_bitBackupEndAbort) &&
            !( (grbit & JET_bitBackupEndNormal) && (grbit & JET_bitBackupEndAbort) ) );

    if ( !fNormalTermination )
    {
        err = ErrERRCheck( errBackupAbortByCaller );
    }
    else
    {
        // We skip to done in surrogate backup, because the real backup happened offline, and
        // calling this function with JET_bitBackupEndNormal means everything went smoothly, somewhere.
        pinst->m_pbackup->BKSetBackupStatus( BACKUP_CONTEXT::backupStateDone );
        err = JET_errSuccess; // explicit is nice.
    }

    err = pinst->m_pbackup->ErrBKExternalBackupCleanUp( err, JET_bitBackupSurrogate | grbit );
    return( err );
}



/***********************************************************
/********************* EXTERNAL BACKUP *********************/

ERR ISAMAPI ErrIsamBeginExternalBackup( JET_INSTANCE jinst, JET_GRBIT grbit )
{
    INST *pinst = (INST *)jinst;

    return pinst->m_pbackup->ErrBKBeginExternalBackup( grbit );
}

ERR BACKUP_CONTEXT::ErrBKBeginExternalBackup( JET_GRBIT grbit, ULONG lgenFirst, ULONG lgenLast )
{
    ERR         err = JET_errSuccess;
    CHECKPOINT  *pcheckpointT = NULL;
    WCHAR       wszPathJetChkLog[IFileSystemAPI::cchPathMax];
    DBID dbid;
    
    const BOOL  fIncrementalBackup = ( JET_bitBackupIncremental == ( JET_bitBackupIncremental & grbit ) );
    const BOOL  fSurrogateBackup = ( JET_bitBackupSurrogate == ( JET_bitBackupSurrogate & grbit ) );
    const BOOL  fInternalCopyBackup = ( JET_bitInternalCopy == ( JET_bitInternalCopy & grbit ) );

#ifdef DEBUG
    if ( m_fDBGTraceBR )
        DBGBRTrace("** Begin BeginExternalBackup - ");
#endif

    if ( m_pinst->m_plog->FLogDisabled() )
    {
        return ErrERRCheck( JET_errLoggingDisabled );
    }

    if ( m_pinst->m_plog->FNoMoreLogWrite() )
    {
        return ErrERRCheck( JET_errLogWriteFail );
    }

    // Allow opening backup during recovering only for internal copy
    // 
    if ( m_pinst->FRecovering() )
    {
        if ( !fInternalCopyBackup || !BoolParam( m_pinst, JET_paramFlight_EnableBackupDuringRecovery ) )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
    }

    /*  grbit may be 0 or combination of JET_bitBackupIncremental or JET_bitBackupSurrogate or JET_bitInternalCopy.
    /**/
    if ( ( grbit & ~(JET_bitBackupSurrogate | JET_bitBackupIncremental | JET_bitInternalCopy) ) != 0 )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    Assert( m_ppibBackup != ppibNil );

    if ( fIncrementalBackup && BoolParam( m_pinst, JET_paramCircularLog ) )
    {
        return ErrERRCheck( JET_errInvalidBackup );
    }

    if ( fSurrogateBackup && BoolParam( m_pinst, JET_paramCircularLog ) )
    {
        return ErrERRCheck( JET_errInvalidBackup );
    }

    if ( ( grbit & JET_bitInternalCopy ) &&
            ( grbit & (JET_bitBackupSurrogate | JET_bitBackupIncremental) ) )
    {
        //  we wouldn't expect surrogate or incremental backups for internal copy
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    //  don't permit backups if low on log disk space or
    //  if approaching the end of the log sequence, because
    //  backups are just going to make the situation worse
    //  by forcing a new log generation
    //
    CallR( m_pinst->m_plog->ErrLGCheckState() );

    //  bail if we already have a backup in progress
    //
    m_critBackupInProgress.Enter();

    if ( m_fBackupInProgressAny )
    {
        BOOL fLocal = m_fBackupInProgressLocal;
        m_critBackupInProgress.Leave();
        if ( fLocal )
        {
            return ErrERRCheck( JET_errBackupInProgress );
        }
        else
        {
            return ErrERRCheck( JET_errSurrogateBackupInProgress );
        }
    }

    Assert ( backupStateNotStarted == m_fBackupStatus);

    // Not allow backup to start if the context is marked as suspended.
    if ( m_pinst->FRecovering() && m_fCtxIsSuspended )
    {
        m_critBackupInProgress.Leave();
        return ErrERRCheck( JET_errBackupNotAllowedYet );
    }

    m_lgenCopyMic = 0;
    m_lgenCopyMac = 0;
    m_lgenDeleteMic = 0;
    m_lgenDeleteMac = 0;
    BKSetLogsTruncated( fFalse );

    m_fStopBackup = fFalse;
    m_fBackupInProgressAny = fTrue;
    m_fBackupInProgressLocal = !fSurrogateBackup;
    m_fBackupIsInternal = fInternalCopyBackup;
    m_critBackupInProgress.Leave();

#ifdef DEBUG
    if ( fSurrogateBackup )
    {
        Assert( !BoolParam( m_pinst, JET_paramCircularLog ) );
        Assert( m_pinst->CNonLoggedIndexCreators() == 0 ); // since we're not circular, this is true.
    }
#endif
    // we technically don't need to do this for surrogate backup, unless we ever the circular flag to 
    // toggle while online.
    while ( m_pinst->CNonLoggedIndexCreators() > 0 )
    {
        //  wait for non-logged index creators
        //  to begin logging again
        //
        UtilSleep( cmsecWaitGeneric );
    }

    if ( fIncrementalBackup )
    {
        m_fBackupStatus = backupStateLogsAndPatchs;
    }
    else
    {
        m_fBackupStatus = backupStateDatabases;
    }

    //  make sure no detach/attach going. If there are, let them continue and finish.

    m_pinst->WaitForDBAttachDetach();
 
    // FULL: clean the "involved" in backup flag for all databases in a full backup. We mark the
    // databases as involved in the backup process during JetOpenFile for the database file
    // INCREMENTAL: set the "involved" in backup flag for all databases in a incremental backup
    // because in this case the database file is not used and, in fact, all the daatbases are
    // contained in the log files.
    for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        // on Incremental, all the databases are part of the backup
        // on Surrogate, all databases are part of the backup
        //      SUBTLE NOTE:  Note we are assuming ALL databases attached to the 
        //      instance (storage group for exch) are being backed up here, no
        //      matter what technology is used on the replica.  This is because
        //      the surrogate backup provides no guidance on which databases it
        //      is backing up.
        // on Full, we mark the Db as part of the backup only on JetOpenFile on it
        // on Snapshot we mark the Db as part of the backup on SnapshotStart
        if ( fIncrementalBackup || fSurrogateBackup )
        {
            g_rgfmp[ifmp].SetInBackupSession();
        }
        else
        {
            g_rgfmp[ifmp].ResetInBackupSession();
        }
    }

    if ( fIncrementalBackup )
    {

//  UNDONE: need to do tight checking to make sure there was a full backup before and
//  UNDONE: the logs after that full backup are still available.
        //  The UNDONE it is not done by ErrLGCheckLogsForIncrementalBackup later on.

        Call( ErrLGCheckIncrementalBackup( m_pinst,
                            fSurrogateBackup ?
                                DBFILEHDR::backupSurrogate :
                                DBFILEHDR::backupNormal ) );
        if ( fSurrogateBackup )
        {
            LGIGetDateTime( &m_logtimeIncBackup );
        }
    }
    else
    {
        m_pinst->m_plog->LGLogTipWithLock( fSurrogateBackup ? &m_lgposFullBackup : &m_lgposFullBackupMark );
        LGIGetDateTime( ( fSurrogateBackup ? &m_logtimeFullBackup : &m_logtimeFullBackupMark ) );
    }

    Assert( m_ppibBackup != ppibNil );

    //  reset global copy/delete generation variables

    m_fBackupBeginNewLogFile = fFalse;


    /*  if incremental backup set copy/delete mic and mac variables,
    /*  else backup is full and set copy/delete mic and delete mac.
    /*  Copy mac will be computed after database copy is complete.
    /**/
    if ( fIncrementalBackup )
    {
#ifdef DEBUG
        if ( m_fDBGTraceBR )
            DBGBRTrace( "Incremental Backup.\n" );
#endif
        UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY,
                START_INCREMENTAL_BACKUP_INSTANCE_ID, 0, NULL, 0, NULL, m_pinst );
        m_fBackupFull = fFalse;

        /*  if all database are allowed to do incremental backup? Check bkinfo prev.
         */
    }
    else
    {
#ifdef DEBUG
        if ( m_fDBGTraceBR )
            DBGBRTrace( "Full Backup.\n" );
#endif

        if ( m_fBackupIsInternal )
        {
            UtilReportEvent(
                eventInformation,
                LOGGING_RECOVERY_CATEGORY,
                START_INTERNAL_COPY_INSTANCE_ID,
                0,
                NULL,
                0,
                NULL,
                m_pinst );
        }
        else
        {
            UtilReportEvent(
                eventInformation,
                LOGGING_RECOVERY_CATEGORY,
                START_FULL_BACKUP_INSTANCE_ID,
                0,
                NULL,
                0,
                NULL,
                m_pinst );
        }
        m_fBackupFull = fTrue;

        Alloc( pcheckpointT = (CHECKPOINT *) PvOSMemoryPageAlloc( sizeof(CHECKPOINT), NULL ) );

        m_pinst->m_plog->LGFullNameCheckpoint( wszPathJetChkLog );

        // This call should only return an error on hardware failure.
        Call( m_pinst->m_plog->ErrLGReadCheckpoint( wszPathJetChkLog, pcheckpointT, fTrue ) );

        m_lgenCopyMic = pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration;
        Assert( m_lgenCopyMic != 0 );
    }

    if ( !m_pinst->FRecovering() )
    {
        LGPOS lgposT;
        const ERR errLGBackupBegin = ErrLGBackupBegin( m_pinst->m_plog, fSurrogateBackup ? DBFILEHDR::backupSurrogate : DBFILEHDR::backupNormal, fIncrementalBackup, &lgposT );
        if ( fSurrogateBackup && fIncrementalBackup )
        {
            Call( errLGBackupBegin ); // critical for surrogate incremental backups.
            // full is set above for surrogate backup, but incremental is not ...
            m_lgposIncBackup = lgposT;

        }
    }

    if ( fSurrogateBackup )
    {
        m_lgenCopyMic = lgenFirst;
        m_lgenCopyMac = lgenLast;
    }
HandleError:
    if ( pcheckpointT != NULL )
    {
        OSMemoryPageFree( pcheckpointT );
    }

#ifdef DEBUG
    if ( m_fDBGTraceBR )
    {
        CHAR sz[256];

        OSStrCbFormatA( sz, sizeof(sz), "   End BeginExternalBackup (%d).\n", err );
        Assert( strlen( sz ) <= sizeof( sz ) - 1 );
        DBGBRTrace( sz );
    }
#endif

    if ( err < 0 )
    {
        m_fBackupInProgressAny = fFalse;
        m_fBackupInProgressLocal = fFalse;
        m_fStopBackup = fFalse;
        m_fBackupStatus = backupStateNotStarted;
    }

    return err;
}


ERR ISAMAPI ErrIsamGetAttachInfo(
    JET_INSTANCE jinst,
    __out_bcount_part_opt(cbMax, *pcbActual) PWSTR wszzDatabases,
    ULONG cbMax,
    ULONG *pcbActual )
{
    INST * pinst = (INST *)jinst;
    return pinst->m_pbackup->ErrBKGetAttachInfo( wszzDatabases, cbMax, pcbActual );
}

ERR BACKUP_CONTEXT::ErrBKGetAttachInfo(
    _Out_opt_z_bytecap_post_bytecount_(cbMax, *pcbActual) PWSTR wszzDatabases,
    ULONG cbMax,
    ULONG *pcbActual )
{
    ERR     err = JET_errSuccess;
    DBID    dbid;
    ULONG   cbActual;
    WCHAR   *pwch = NULL;
    WCHAR   *pwchT;

    if ( !m_fBackupInProgressLocal )
    {
        return ErrERRCheck( JET_errNoBackup );
    }
    else if ( m_fStopBackup )
    {
        return ErrERRCheck( JET_errBackupAbortByServer );
    }

    /*  should not get attach info if not performing full backup
    /**/
    if ( !m_fBackupFull )
    {
        return ErrERRCheck( JET_errInvalidBackupSequence );
    }

#ifdef DEBUG
    if ( m_fDBGTraceBR )
        DBGBRTrace( "** Begin GetAttachInfo.\n" );
#endif

    /*  compute cbActual, for each database name with NULL terminator
    /*  and with terminator of super string.
    /**/
    cbActual = 0;
    for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP    ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP     *pfmp = &g_rgfmp[ifmp];
        if ( pfmp->FInUse()
            && pfmp->FLogOn()
            && pfmp->FAttached()
            && !pfmp->FAttachedForRecovery()
            && !pfmp->FAttachingDB() )
        {
            Assert( !pfmp->FSkippedAttach() );
            Assert( !pfmp->FDeferredAttach() );

            cbActual += sizeof( WCHAR ) * ( (ULONG) LOSStrLengthW( pfmp->WszDatabaseName() ) + 1 );
        }
    }
    cbActual += sizeof(WCHAR); // double NUL terminator for "super string" / multisz type string

    Alloc( pwch = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbActual ) ) );

    pwchT = pwch;
    ULONG cchT = cbActual / sizeof(WCHAR);
    for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP    ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP     *pfmp = &g_rgfmp[ifmp];

        if ( pfmp->FInUse()
            && pfmp->FLogOn()
            && pfmp->FAttached()
            && !pfmp->FAttachedForRecovery()
            && !pfmp->FAttachingDB() )
        {
            Assert( !pfmp->FSkippedAttach() );
            Assert( !pfmp->FDeferredAttach() );

            Assert( pwchT + LOSStrLengthW( pfmp->WszDatabaseName() ) + 1 < pwchT + ( cbActual / sizeof(WCHAR) ) );
            OSStrCbCopyW( pwchT, cchT * sizeof(WCHAR), pfmp->WszDatabaseName() );
            cchT -= LOSStrLengthW( pfmp->WszDatabaseName() );
            pwchT += LOSStrLengthW( pfmp->WszDatabaseName() );
            Assert( *pwchT == 0 );
            cchT--;
            pwchT++;
        }
    }
    Assert( pwchT == pwch + ( cbActual / sizeof(WCHAR) ) - 1 );
    Assert( cchT >= 1 );
    *pwchT = 0;

    /*  return cbActual
    /**/
    if ( pcbActual != NULL )
    {
        *pcbActual = cbActual;
    }

    /*  return data
    /**/
    if ( wszzDatabases != NULL )
        UtilMemCpy( wszzDatabases, pwch, min( cbMax, cbActual ) );

HandleError:
    /*  free buffer
    /**/
    if ( pwch != NULL )
    {
        OSMemoryHeapFree( pwch );
        pwch = NULL;
    }

#ifdef DEBUG
    if ( m_fDBGTraceBR )
    {
        CHAR sz[256];
        WCHAR *pwchTrace;

        if ( err >= 0 )
        {
            OSStrCbFormatA( sz, sizeof(sz), "   Attach Info with cbActual = %d and cbMax = %d :\n", cbActual, cbMax );
            Assert( strlen( sz ) <= sizeof( sz ) - 1 );
            DBGBRTrace( sz );

            if ( wszzDatabases != NULL )
            {
                pwchTrace = wszzDatabases;

                do {
                    if ( LOSStrLengthW( pwchTrace ) != 0 )
                    {
                        OSStrCbFormatA( sz, sizeof(sz), "     %ws\n", pwchTrace );
                        Assert( strlen( sz ) <= sizeof( sz ) - 1 );
                        DBGBRTrace( sz );
                        pwchTrace += LOSStrLengthW( pwchTrace );
                    }
                    pwchTrace++;
                } while ( *pwchTrace );
            }
        }

        OSStrCbFormatA( sz, sizeof(sz), "   End GetAttachInfo (%d).\n", err );
        Assert( strlen( sz ) <= sizeof( sz ) - 1 );
        DBGBRTrace( sz );
    }
#endif

    if ( err < 0 )
    {
        CallS( ErrBKExternalBackupCleanUp( err ) );
        Assert( !m_fBackupInProgressLocal && !m_fBackupInProgressAny );
    }

#pragma prefast(suppress : 26030, "We may fill in cbMax in wszzDatabases and say cbActual, but it is null terminated.")
    return err;
}


//  ====================================================
//  Spin off IOs of optimal size to fill buffer.
//  As IOs complete, if its chunk in the buffer is the next to checksum,
//  checksum that along with any other chunks forward in the buffer that
//  have also completed. If it's not the chunk's turn to checksum, simply
//  make a note that it should be checksummed later (it will be checksummed
//  by the chunk who's turn comes up next).
//  ====================================================


// *********************************************************** END log verification stuff

ERR ISAMAPI ErrIsamOpenFile(
    JET_INSTANCE jinst,
    const WCHAR *wszFileName,
    JET_HANDLE      *phfFile,
    const QWORD     ibRead,
    ULONG           *pulFileSizeLow,
    ULONG           *pulFileSizeHigh )
{
    INST *pinst = (INST *)jinst;
    return pinst->m_pbackup->ErrBKOpenFile( wszFileName, phfFile, ibRead, pulFileSizeLow, pulFileSizeHigh, fFalse );
}

ERR BACKUP_CONTEXT::ErrBKOpenFile(
    const WCHAR                 *wszFileName,
    JET_HANDLE                  *phfFile,
    const QWORD                 ibRead,
    ULONG                       *pulFileSizeLow,
    ULONG                       *pulFileSizeHigh,
    const BOOL                  fIncludePatch )
{
    ERR     err;
    INT     irhf;
    WCHAR   wszDirT[IFileSystemAPI::cchPathMax];
    WCHAR   wszFNameT[IFileSystemAPI::cchPathMax];
    WCHAR   wszExtT[IFileSystemAPI::cchPathMax];
    PGNO    pgnoRead = 0; //page number of the read offset passed
    BOOL    fDbOpen = fFalse;

    if ( !m_fBackupInProgressLocal )
    {
        return ErrERRCheck( JET_errNoBackup );
    }
    else if ( m_fStopBackup )
    {
        return ErrERRCheck( JET_errBackupAbortByServer );
    }

    if ( m_pinst->FRecovering() )
    {
        // Currently backup during recovery in only allowed for internal copy
        if ( !m_fBackupIsInternal || !BoolParam( m_pinst, JET_paramFlight_EnableBackupDuringRecovery ) )
        {
            return ErrERRCheck( JET_errBackupNotAllowedYet );
        }

        // The backup has been suspended by ErrLGDbDetachingCallback. 
        if ( m_fCtxIsSuspended )
        {
            return ErrERRCheck( JET_errBackupNotAllowedYet );
        }
    }

    if ( NULL == wszFileName || 0 == *wszFileName )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( ( ibRead % g_cbPage ) != 0 )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( ibRead > 0 )
    {
        pgnoRead = PgnoOfOffset( ibRead );
        
        // 29/08/2017 - SOMEONE
        // If we aren't reading from offset 0 we should start reading from atleast page 2
        // This is because ErrBKIReadPages checks that when we read from offset 0, we read the db header pages plus atleast one db page
        // I don't want to modify the logic in ErrBKIReadPages for this
        // So if someone wants to read from 1st page they may as well read the headers too (offset 0) 
        if ( pgnoRead < 2 )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
    }

    /*  allocate rhf from rhf array.
    /**/
    if ( m_crhfMac < crhfMax )
    {
        irhf = m_crhfMac;
        m_crhfMac++;
    }
    else
    {
        Assert( m_crhfMac == crhfMax );
        for ( irhf = 0; irhf < crhfMax; irhf++ )
        {
            if ( !m_rgrhf[irhf].fInUse )
            {
                break;
            }
        }
    }
    if ( irhf == crhfMax )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    Assert( irhf < m_crhfMac );

    CallR ( m_pinst->m_pfsapi->ErrPathComplete( wszFileName, wszFNameT ) );

    m_rgrhf[irhf].fInUse            = fTrue;
    m_rgrhf[irhf].fDatabase         = fFalse;
    m_rgrhf[irhf].fIsLog            = fFalse;
    m_rgrhf[irhf].pLogVerifyState   = pNil;
    m_rgrhf[irhf].pfapi             = NULL;
    m_rgrhf[irhf].ifmp              = g_ifmpMax;
    m_rgrhf[irhf].ib                = ibRead;
    m_rgrhf[irhf].cb                = 0;
    m_rgrhf[irhf].wszFileName       = NULL;
    m_rgrhf[irhf].lGeneration       = 0;
    m_rgrhf[irhf].pdbfilehdr        = NULL;

    Assert ( LOSStrLengthW(wszFNameT) > 0 );
    const ULONG cbFNameT = sizeof(WCHAR ) * ( 1 + LOSStrLengthW(wszFNameT) );
    m_rgrhf[irhf].wszFileName = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbFNameT ) );
    if ( NULL == m_rgrhf[irhf].wszFileName )
    {
        m_rgrhf[irhf].fInUse = fFalse;
        return ErrERRCheck( JET_errOutOfMemory );
    }
    OSStrCbCopyW( m_rgrhf[irhf].wszFileName, cbFNameT, wszFNameT );

    Assert ( backupStateLogsAndPatchs == m_fBackupStatus || backupStateDatabases == m_fBackupStatus );

    IFMP ifmpT;

    // Blocks backup when database is still in required range.

    err = ErrDBOpenDatabase( m_ppibBackup, wszFileName, &ifmpT, 0 );
    Expected( err < JET_errSuccess || err == JET_errSuccess || err == JET_wrnFileOpenReadOnly );
    const IFMP ifmp = ( err >= JET_errSuccess ) ? ifmpT : ifmpNil;

    if ( err >= JET_errSuccess )
    {
        PATCH_HEADER_PAGE *ppatchhdr;

        fDbOpen = fTrue;

        /*  should not open database if not performing full backup
        /**/
        if ( !m_fBackupFull )
        {
            Assert ( backupStateLogsAndPatchs == m_fBackupStatus );
            Error( ErrERRCheck( JET_errInvalidBackupSequence ) );
        }

        /*  should not open database if we are during log copy phase
        /**/
        if ( backupStateDatabases != m_fBackupStatus )
        {
            // it looks like it is called after ErrBKGetLogInfo
            Assert ( m_fBackupBeginNewLogFile );
            Error( ErrERRCheck( JET_errInvalidBackupSequence ) );
        }

        FMP* const pfmpT = &g_rgfmp[ifmp];

        /*  we fire ErrLGDbDetachingCallback for every shrink, so FAttachedForRecovery
        /*  can be backed up only when it's internal client and is turned on.
        /*  previous, databases undergoing recovery cannot be backed up at this time
        /*  because of currently unsupported interactions with database shrinkage from EOF.
        /**/
        if ( pfmpT->FAttachedForRecovery() )
        {
            if ( !m_fBackupIsInternal || !BoolParam( m_pinst, JET_paramFlight_EnableBackupDuringRecovery ) )
            {
                Expected( fFalse );
                Error( ErrERRCheck( JET_errDatabaseAttachedForRecovery ) );
            }
        }

        Assert( pfmpT->FAttached() && !pfmpT->FAttachingDB() );

        Assert( !m_rgrhf[irhf].pfapi );
        m_rgrhf[irhf].fDatabase = fTrue;
        m_rgrhf[irhf].ifmp = ifmp;

        /*  database should be loggable or would not have been
        /*  given out for backup purposes.
        /**/
        Assert( pfmpT->FLogOn() );

        if ( fIncludePatch )
        {
            /*  create a local patch file
            /**/
            IFileAPI *  pfapiPatch      = NULL;
            WCHAR       wszPatch[IFileSystemAPI::cchPathMax];

            /*  patch file should be in database directory during backup. In log directory during
             *  restore.
             */

            BKIGetPatchName( wszPatch, pfmpT->WszDatabaseName() );

            Call( CIOFilePerf::ErrFileCreate(
                                    m_pinst->m_pfsapi,
                                    m_pinst,
                                    wszPatch,
                                    IFileAPI::fmfOverwriteExisting,
                                    iofileOther,
                                    qwPatchFileID,
                &pfapiPatch ) );
            delete pfapiPatch;
        }

        Assert( pfmpT->PgnoBackupCopyMost() == 0 );

        // Snapshot the db header before getting the file size. This is so that the minRequired in the backed up
        // database reflects the size of the database copied and we are not missing updates to pages extended between
        // now and when we start reading the database.
        //
        Alloc( m_rgrhf[irhf].pdbfilehdr = (DBFILEHDR *)PvOSMemoryPageAlloc( g_cbPage, NULL ) );
        UtilMemCpy( m_rgrhf[irhf].pdbfilehdr, pfmpT->Pdbfilehdr(), g_cbPage );
        BKINFO *pbkinfo = &m_rgrhf[irhf].pdbfilehdr->bkinfoFullCur;
        pbkinfo->le_lgposMark = m_lgposFullBackupMark;
        pbkinfo->logtimeMark = m_logtimeFullBackupMark;
        pbkinfo->le_genLow = m_lgenCopyMic;
        pbkinfo->le_genHigh = 0;
        Assert( pbkinfo->le_genLow != 0 );
        Assert( pbkinfo->le_genHigh == 0 );
        SetPageChecksum( m_rgrhf[irhf].pdbfilehdr, g_cbPage, databaseHeader, 0 );

        //  set backup database file size to current database OE size

        const PGNO pgnoWriteLatchedMax = pfmpT->PgnoHighestWriteLatched();
        const PGNO pgnoDirtiedMax = pfmpT->PgnoDirtiedMax();
        const PGNO pgnoWriteLatchedNonScanMax = pfmpT->PgnoWriteLatchedNonScanMax();
        const PGNO pgnoLatchedScanMax = pfmpT->PgnoLatchedScanMax();
        const PGNO pgnoPreReadScanMax = pfmpT->PgnoPreReadScanMax();
        const PGNO pgnoScanMax = pfmpT->PgnoScanMax();
        const PGNO pgnoLast = pfmpT->PgnoLast();

        // We need to use the logical size here (instead of the file system size) to avoid resuming backups
        // to incorrectly treat the database as having undergone shrinkage in case the physical size is reduced
        // after an in-between attach (due to truncation down to logical size), despite the logical size having
        // remained the same.
        AssertTrack( pgnoDirtiedMax <= pgnoLast, "BackupPgnoLastTooLowv3Dirtied" );
        AssertTrack( pgnoWriteLatchedNonScanMax <= pgnoLast, "BackupPgnoLastTooLowv3NonScan" );
        AssertTrack( pgnoWriteLatchedMax <= pgnoLast, "BackupPgnoLastTooLowv2Original" );
        AssertTrack( pgnoLatchedScanMax <= pgnoScanMax, "BackupV3TheDbScanMaxNotAheadOfLatchedScanMax" );
        AssertTrack( pgnoLatchedScanMax <= pgnoPreReadScanMax, "BackupV3ThePreReadDbScanMaxNotAheadOfLatchedScanMax" );
        m_rgrhf[irhf].cb = ((QWORD)pgnoLast) * g_cbPage;

        pfmpT->CritLatch().Enter();

        pfmpT->SetPgnoBackupMost( pgnoLast );
        pfmpT->ResetFCopiedPatchHeader();
        Assert( pgnoRead != 1 );

        if ( pgnoRead > 1 )
        {
            // PgnoBackupCopyMost indicates the last/most page copied
            // And copy starts at 1 higher than the value of PgnoBackupCopyMost
            // Here we set PgnoBackupCopyMost to pgnoRead - 1 so that we start copying from pgnoRead
            pfmpT->SetPgnoBackupCopyMost( pgnoRead - 1 );
            Assert( pfmpT->PgnoBackupMost() >= pfmpT->PgnoBackupCopyMost() );
        }

        //  set the returned file size.
        // Must add on cpgDBReserved to accurately inform backup of file size

        m_rgrhf[irhf].cb += g_cbPage * cpgDBReserved;

        // we add the additional header added at the end
        // to replace information stored in the patch file header
        m_rgrhf[irhf].cb += g_cbPage;

        pfmpT->CritLatch().Leave();

        //  setup patch file header for copy.

        if ( !pfmpT->Ppatchhdr() )
        {
            Alloc( ppatchhdr = (PATCH_HEADER_PAGE*)PvOSMemoryPageAlloc( g_cbPage, NULL ) );
            pfmpT->SetPpatchhdr( ppatchhdr );
        }

        // mark database as involved in an external backup
        pfmpT->SetInBackupSession();

#ifdef DEBUG
        if ( m_fDBGTraceBR )
        {
            const INT cbFillBuffer = 128;
            char szTrace[cbFillBuffer + 1];
            OSStrCbFormatA( szTrace, sizeof(szTrace), "START COPY DB %ld", pfmpT->PgnoBackupMost() );
            CallS( m_pinst->m_plog->ErrLGTrace( ppibNil, szTrace ) );

            m_cbDBGCopied = pfmpT->PgnoBackupMost() * g_cbPage;
        }
#endif
    }
    else if ( err == JET_errDatabaseNotFound )
    {
        Assert( !m_rgrhf[irhf].pfapi );
        m_rgrhf[irhf].fDatabase = fFalse;

        if ( backupStateDatabases == m_fBackupStatus )
        {
            // we are in backup databases mode but database file (edb)
            // is not found in the FMP's.

            // UNDONE: we return JET_errDatabaseNotFound at this point
            // maybe we shell return something like "database unmounted" ...
            Call( err );

            Call( ErrERRCheck( JET_wrnNyi ) );
        }
        else
        {
            Assert ( backupStateLogsAndPatchs == m_fBackupStatus );

            // we backup just patch and log files at this point

            // UNDONE: check the format of the file (extension, etc.)

            Assert( !m_rgrhf[irhf].fDatabase );

            //  CONSIDER: Instead of checking extension, read in header to see if it's
            //  a valid log file header, else try to see if it's a valid patch file,
            //  else die because it's not a correct file that we recognize.
            //  Better yet, check if it's a log file first, since we open more log files
            //  than anything.

            CallS( m_pinst->m_pfsapi->ErrPathParse( wszFileName, wszDirT, wszFNameT, wszExtT ) );
            m_rgrhf[irhf].fIsLog = ( UtilCmpFileName( wszExtT, m_pinst->m_plog->LogExt() ) == 0 );

            LONG lGen = 0;
            QWORD qwEseFileID = 0;
            
            if ( m_rgrhf[irhf].fIsLog )
            {
                ULONG   cchLogDigits;

                Call( LGFileHelper::ErrLGGetGeneration(
                            m_pinst->m_pfsapi,
                            m_rgrhf[irhf].wszFileName,
                            SzParam( m_pinst, JET_paramBaseName ),
                            &lGen,
                            m_pinst->m_plog->LogExt(),
                            &cchLogDigits ) );
                
                qwEseFileID = QwInstFileID( qwLogFileID, m_pinst->m_iInstance, lGen );
            }
            else
            {
                // only thing left is the thunk patch file
                qwEseFileID = qwPatchFileID;
            }

            //  first try opening the file from the regular file-system

            err = CIOFilePerf::ErrFileOpen(
                                    m_pinst->m_pfsapi,
                                    m_pinst,
                                    wszFileName,
                                    (   IFileAPI::fmfReadOnly |
                                        ( BoolParam( JET_paramEnableFileCache ) ?
                                            IFileAPI::fmfCached :
                                            IFileAPI::fmfNone ) ),
                                    iofileOther,
                                    qwEseFileID,
                                    &m_rgrhf[irhf].pfapi );

            if ( JET_errFileNotFound == err )
            {
                const WCHAR * rgszT[] = { wszFileName };

                UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                        BACKUP_LOG_FILE_MISSING_ERROR_ID, 1, rgszT, 0, NULL, m_pinst );

                err = ErrERRCheck( JET_errMissingFileToBackup );
            }
            Call( err );

            if ( m_rgrhf[irhf].fIsLog )
            {
                m_rgrhf[irhf].lGeneration = lGen;
                Alloc( m_rgrhf[irhf].pLogVerifyState = new LOG_VERIFY_STATE() );
            }

            /*  get file size
            /**/
            // just opened the file, so the file size must be correctly buffered
            Call( m_rgrhf[irhf].pfapi->ErrSize( &m_rgrhf[irhf].cb, IFileAPI::filesizeLogical ) );
            Assert( m_rgrhf[irhf].cb > 0 || !m_rgrhf[irhf].fIsLog );
            Assert( m_rgrhf[irhf].cb == 0 || m_rgrhf[irhf].fIsLog );
        }

#ifdef DEBUG
        if ( m_fDBGTraceBR )
            m_cbDBGCopied = DWORD( m_rgrhf[irhf].cb );
#endif
    }
    else
    {
        Assert( err < JET_errSuccess );
        Call( err );
    }

    *phfFile = (JET_HANDLE)irhf;
    {
    QWORDX cbFile;
    cbFile.SetQw( m_rgrhf[irhf].cb );
    *pulFileSizeLow = cbFile.DwLow();
    *pulFileSizeHigh = cbFile.DwHigh();
    }
    err = JET_errSuccess;

    // report start backup of file (report only EDB)
    if ( m_rgrhf[irhf].fDatabase )
    {
        WCHAR wszSize[32];
        char szFileNameA[IFileSystemAPI::cchPathMax];

        Assert ( m_rgrhf[irhf].wszFileName );

        const WCHAR * rgszT[] = { m_rgrhf[irhf].wszFileName, wszSize };

        if ( m_rgrhf[irhf].cb > QWORD(1024 * 1024) )
        {
            OSStrCbFormatW( wszSize, sizeof(wszSize), L"%I64u Mb", m_rgrhf[irhf].cb / QWORD(1024 * 1024) );
        }
        else
        {
            OSStrCbFormatW( wszSize, sizeof(wszSize), L"%I64u Kb", m_rgrhf[irhf].cb / QWORD(1024) );
        }

        UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, BACKUP_FILE_START, 2, rgszT, 0, NULL, m_pinst );

        CallS( ErrOSSTRUnicodeToAscii( m_rgrhf[irhf].wszFileName,
                                       szFileNameA,
                                       sizeof( szFileNameA ),
                                       NULL,
                                       OSSTR_ALLOW_LOSSY ) );
        const INT cbFillBuffer = 64 + IFileSystemAPI::cchPathMax;
        char szTrace[cbFillBuffer + 1];
        OSStrCbFormatA( szTrace, sizeof(szTrace), "Start backup file %s, size %ws", szFileNameA, wszSize );
        CallS( m_pinst->m_plog->ErrLGTrace( m_ppibBackup, szTrace ) );

    }

HandleError:

#ifdef DEBUG
    if ( m_fDBGTraceBR )
    {
        char szFileNameA[IFileSystemAPI::cchPathMax];
        CallS( ErrOSSTRUnicodeToAscii( wszFileName,
                                       szFileNameA,
                                       sizeof( szFileNameA ),
                                       NULL,
                                       OSSTR_ALLOW_LOSSY ) );
        CHAR sz[256];

        OSStrCbFormatA( sz, sizeof(sz), "** OpenFile (%d) %s of size %ld.\n", err, szFileNameA, m_cbDBGCopied );
        Assert( strlen( sz ) <= sizeof( sz ) - 1 );
        DBGBRTrace( sz );
        m_cbDBGCopied = 0;
    }
#endif

    if ( err < JET_errSuccess )
    {
        if ( fDbOpen )
        {
            CallS( ErrDBCloseDatabase( m_ppibBackup, ifmp, 0 ) );
            fDbOpen = fFalse;
        }

        // if they try to backup a unmounted database or we get an error on one database
        // we will not stop the backup
        // on all other errors (logs, patch files) we stop the backup of the instance
        if ( backupStateDatabases == m_fBackupStatus )
        {
            WCHAR wszError[32];
            Assert ( m_rgrhf[irhf].wszFileName );
            const WCHAR * rgszT[] = { wszError, m_rgrhf[irhf].wszFileName };

            OSStrCbFormatW( wszError, sizeof(wszError), L"%d", err );

            if ( m_rgrhf[irhf].ifmp < g_ifmpMax )
            {
                FMP * pfmpT = g_rgfmp + m_rgrhf[irhf].ifmp;
                Assert ( pfmpT );
                pfmpT->ResetInBackupSession();

                if ( pfmpT->Ppatchhdr() )
                {
                    OSMemoryPageFree( pfmpT->Ppatchhdr() );
                    pfmpT->SetPpatchhdr( NULL );
                }

                if ( fIncludePatch )
                {
                    WCHAR   wszPatch[IFileSystemAPI::cchPathMax];

                    // delete the created patch file

                    //  UNDONE: Does this work?? If patch file was created, there might
                    //  be a handle open on the file, in which case deletion will fail

                    BKIGetPatchName( wszPatch, g_rgfmp[ m_rgrhf[irhf].ifmp ].WszDatabaseName()  );
                    ERR errAux;
                    errAux = m_pinst->m_pfsapi->ErrFileDelete( wszPatch );
                    if ( JET_errFileNotFound == errAux )
                    {
                        errAux = JET_errSuccess;
                    }
                    CallSRFS( errAux, ( JET_errFileAccessDenied, 0 ) );
                }
            }

            UtilReportEvent(
                    eventError,
                    LOGGING_RECOVERY_CATEGORY,
                    BACKUP_ERROR_FOR_ONE_DATABASE,
                    2,
                    rgszT,
                    0,
                    NULL,
                    m_pinst );

            Assert( m_fBackupInProgressLocal );

            /*  release file handle resource on error
            /**/
            Assert ( m_rgrhf[irhf].wszFileName );
            OSMemoryHeapFree( m_rgrhf[irhf].wszFileName );
            m_rgrhf[irhf].wszFileName = NULL;

            if ( m_rgrhf[irhf].pdbfilehdr != NULL )
            {
                OSMemoryPageFree( m_rgrhf[irhf].pdbfilehdr );
                m_rgrhf[irhf].pdbfilehdr = NULL;
            }

            m_rgrhf[irhf].fInUse = fFalse;
        }
        else
        {

            /*  release file handle resource on error
            /**/
            Assert ( m_rgrhf[irhf].wszFileName );
            OSMemoryHeapFree( m_rgrhf[irhf].wszFileName );
            m_rgrhf[irhf].wszFileName = NULL;

            m_rgrhf[irhf].fInUse = fFalse;

            CallS( ErrBKExternalBackupCleanUp( err ) );
            Assert( !m_fBackupInProgressLocal && !m_fBackupInProgressAny );
        }
    }

    return err;
}


ERR ISAMAPI ErrIsamReadFile( JET_INSTANCE jinst, JET_HANDLE hfFile, VOID *pv, ULONG cbMax, ULONG *pcbActual )
{
    INST *pinst = (INST *)jinst;
    return pinst->m_pbackup->ErrBKReadFile( hfFile, pv, cbMax, pcbActual );
}

ERR BACKUP_CONTEXT::ErrBKReadFile(
    JET_HANDLE                  hfFile,
    VOID                        *pv,
    ULONG                       cbMax,
    ULONG                       *pcbActual )
{
    ERR     err = JET_errSuccess;
    INT     irhf = (INT)hfFile;
    INT     cpage = 0;
    FMP     *pfmpT = NULL;
    INT     cbActual = 0;

#ifdef DEBUG
    CHAR*   szLGDBGPageList = NULL;
    ULONG   cbLGDBGPageList = 0;
#endif

    BOOL    fInTransaction  = fFalse;

    if ( !m_fBackupInProgressLocal )
    {
        return ErrERRCheck( JET_errNoBackup );
    }
    else if ( m_fStopBackup )
    {
        return ErrERRCheck( JET_errBackupAbortByServer );
    }
    
    BOOL fBackupDuringRecovery = fFalse;
    if ( m_pinst->FRecovering() )
    {
        // Currently backup during recovery in only allowed for internal copy
        if ( !m_fBackupIsInternal || !BoolParam( m_pinst, JET_paramFlight_EnableBackupDuringRecovery ) )
        {
            return ErrERRCheck( JET_errBackupNotAllowedYet );
        }

        // The backup has been suspended by ErrLGDbDetachingCallback. 
        if ( m_fCtxIsSuspended )
        {
            return ErrERRCheck( JET_errBackupNotAllowedYet );
        }

        fBackupDuringRecovery = fTrue;
    }

    // we pottentialy make an async IO on the pv so check here
    // for alignment rather then let the IO fail and get some
    // weird error back. One convenine way to check it (per MSDN)
    // is to check memory page size allocation. It is much easier
    // then to get the volume sector size at this point becuase
    // in theory that is dependent of where the file we are
    // reading is located and we should also cache it becuase
    // we should not call pfsapi->ErrFileAtomicWriteSize() each time.
    //
    if ( UINT_PTR(0) != ( UINT_PTR(pv) & UINT_PTR( OSMemoryPageCommitGranularity( ) - 1 ) ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( !m_rgrhf[irhf].fDatabase )
    {
        Assert ( ( backupStateLogsAndPatchs == m_fBackupStatus ) );

        if ( ( cbMax % g_cbPage ) != 0 )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }

        if ( cbMax / g_cbPage < cpgDBReserved && 0 == m_rgrhf[irhf].ib )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }

        DWORD       cbToRead        = DWORD( min( m_rgrhf[irhf].cb - m_rgrhf[irhf].ib, cbMax ) );

        Assert ( 0 == ( cbToRead % g_cbPage ) );

        Assert ( 0 == m_ppibBackup->Level() );
        Call ( ErrDIRBeginTransaction( m_ppibBackup, 45093, NO_GRBIT ) );
        fInTransaction = fTrue;
        Assert ( 1 == m_ppibBackup->Level() );

        if ( cbToRead > 0 )
        {
            AssertRTL( m_rgrhf[irhf].fIsLog );

            BYTE* const pb = reinterpret_cast< BYTE* const >( pv );
            
            Call( ErrLGReadAndVerifyFile( m_pinst,
                                          m_rgrhf[irhf].pfapi,
                                          (DWORD)m_rgrhf[irhf].ib,
                                          (DWORD)m_rgrhf[irhf].cb,
                                          m_rgrhf[irhf].pLogVerifyState,
                                          *TraceContextScope( iorpBackup ),
                                          pb,
                                          cbToRead ) );
        }   // cbToRead > 0

        Assert ( 1 == m_ppibBackup->Level() );

        Call( ErrDIRCommitTransaction( m_ppibBackup, NO_GRBIT ) );
        fInTransaction = fFalse;
        Assert ( 0 == m_ppibBackup->Level() );

        m_rgrhf[irhf].ib += cbToRead;
        if ( pcbActual )
        {
            *pcbActual = cbToRead;
        }

#ifdef DEBUG
        if ( m_fDBGTraceBR )
            m_cbDBGCopied += min( cbMax, *pcbActual );
#endif
    }
    else
    {
        Assert ( backupStateDatabases == m_fBackupStatus );
        FMP::AssertVALIDIFMP( m_rgrhf[irhf].ifmp );

        pfmpT = &g_rgfmp[ m_rgrhf[irhf].ifmp ];

        if ( ( cbMax % g_cbPage ) != 0 )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }

        cpage = cbMax / g_cbPage;

        //  we need to read at least 2 pages for the database header,
        //  plus one more to ensure PgnoBackupCopyMost advances beyond 0
        //
        if ( 0 == cpage || ( cpage <= cpgDBReserved && 0 == pfmpT->PgnoBackupCopyMost() ) )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }

#ifdef DEBUG
        if ( m_fDBGTraceBR > 1 )
        {
            cbLGDBGPageList = cpage * 20;
            szLGDBGPageList = static_cast<CHAR *>( PvOSMemoryHeapAlloc( cbLGDBGPageList ) );

            if ( szLGDBGPageList )
            {
                szLGDBGPageList[0] = '\0';
            }
        }
#endif

        // check database as involved in an external backup
        Assert ( pfmpT->FInBackupSession() );

        if ( cpage > 0 )
        {
            /*  read next cpageBackupBuffer pages
            /**/
            err = ErrBKIReadPages(
                &m_rgrhf[irhf],
                pv,
                cpage,
                &cbActual
#ifdef DEBUG
                ,szLGDBGPageList
                ,cbLGDBGPageList
#endif
                );
            Call( err );

#ifdef DEBUG
            if ( m_fDBGTraceBR )
                m_cbDBGCopied += cbActual;
#endif

            // The backup context has been suspended by ErrLGDbDetachingCallback. 
            // Currently it's only for backup during recovery opened by internal copy
            // 
            if ( fBackupDuringRecovery && m_fCtxIsSuspended )
            {
                return ErrERRCheck( JET_errBackupNotAllowedYet );
            }

            // set the data read (used just to check at the end if all was read.
            m_rgrhf[irhf].ib += cbActual;
            Assert( (m_rgrhf[irhf].ib / g_cbPage) == (g_rgfmp[ m_rgrhf[irhf].ifmp ].PgnoBackupCopyMost() + cpgDBReserved )
                    || ( (g_rgfmp[ m_rgrhf[irhf].ifmp ].PgnoBackupCopyMost() == g_rgfmp[ m_rgrhf[irhf].ifmp ].PgnoBackupMost() )
                        && (m_rgrhf[irhf].ib / g_cbPage) == (g_rgfmp[ m_rgrhf[irhf].ifmp ].PgnoBackupCopyMost() + cpgDBReserved + 1) ) );
        }

        if ( pcbActual )
        {
            *pcbActual = cbActual;
        }
    }

HandleError:

    if ( fInTransaction )
    {
        CallSx ( ErrDIRRollback( m_ppibBackup ), JET_errRollbackError );
        fInTransaction = fFalse;
    }

#ifdef DEBUG
    if ( m_fDBGTraceBR )
    {
        CHAR sz[256];

        OSStrCbFormatA( sz, sizeof(sz), "** ReadFile (%d) ", err );
        Assert( strlen( sz ) <= sizeof( sz ) - 1 );
        DBGBRTrace( sz );
        if ( m_fDBGTraceBR > 1 )
            DBGBRTrace( szLGDBGPageList );
        DBGBRTrace( "\n" );
    }
    if (szLGDBGPageList)
    {
        OSMemoryHeapFree( (void *)szLGDBGPageList );
        szLGDBGPageList = NULL;
    }
#endif

    if ( err < 0 )
    {
        WCHAR wszError[32];
        Assert ( m_rgrhf[irhf].wszFileName );

        const WCHAR * rgszT[] = { wszError, m_rgrhf[irhf].wszFileName };

        OSStrCbFormatW( wszError, sizeof(wszError), L"%d", err );

        // if they try to backup a unmounted database or we get an error on one database
        // we will not stop the backup
        // on all other errors (logs, patch files) we stop the backup of the instance
        if ( backupStateDatabases == m_fBackupStatus )
        {
            FMP::AssertVALIDIFMP( m_rgrhf[irhf].ifmp );

            pfmpT = g_rgfmp + m_rgrhf[irhf].ifmp;
            Assert ( pfmpT );
            Assert ( pfmpT->FInBackupSession() );
            pfmpT->ResetInBackupSession();

            if ( pfmpT->Ppatchhdr() )
            {
                OSMemoryPageFree( pfmpT->Ppatchhdr() );
                pfmpT->SetPpatchhdr( NULL );
            }

            UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                BACKUP_ERROR_FOR_ONE_DATABASE, 2, rgszT, 0, NULL, m_pinst );

            CallS( ErrBKCloseFile( hfFile ) );
            Assert( m_fBackupInProgressLocal );
        }
        else
        {
            UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                BACKUP_ERROR_READ_FILE, 2, rgszT, 0, NULL, m_pinst );

            CallS( ErrBKExternalBackupCleanUp( err ) );
            Assert( !m_fBackupInProgressLocal && !m_fBackupInProgressAny );
        }
    }

    return err;
}


ERR ISAMAPI ErrIsamCloseFile( JET_INSTANCE jinst,  JET_HANDLE hfFile )
{
    INST *pinst = (INST*) jinst;
    return pinst->m_pbackup->ErrBKCloseFile( hfFile );
}

ERR BACKUP_CONTEXT::ErrBKCloseFile( JET_HANDLE hfFile )
{
    INT     irhf = (INT)hfFile;
    IFMP    ifmpT = g_ifmpMax;

    if ( !m_fBackupInProgressLocal )
    {
        return ErrERRCheck( JET_errNoBackup );
    }

    if ( irhf < 0 ||
        irhf >= m_crhfMac ||
        !m_rgrhf[irhf].fInUse )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    /*  check if handle if for database file or non-database file.
    /*  if handle is for database file, then terminate patch file
    /*  support and release recovery handle for file.
    /*
    /*  if handle is for non-database file, then close file handle
    /*  and release recovery handle for file.
    /**/
    if ( m_rgrhf[irhf].fDatabase )
    {
        Assert( backupStateDatabases == m_fBackupStatus );
        Assert( !m_rgrhf[irhf].pfapi );
        FMP::AssertVALIDIFMP( m_rgrhf[irhf].ifmp );

        ifmpT = m_rgrhf[irhf].ifmp;

        g_rgfmp[ifmpT].CritLatch().Enter();
        g_rgfmp[ifmpT].SetPgnoBackupMost( 0 );
        g_rgfmp[ifmpT].SetPgnoBackupCopyMost( 0 );
        g_rgfmp[ifmpT].ResetFCopiedPatchHeader();
        g_rgfmp[ifmpT].CritLatch().Leave();

        if ( m_rgrhf[irhf].pdbfilehdr != NULL )
        {
            OSMemoryPageFree( m_rgrhf[irhf].pdbfilehdr );
            m_rgrhf[irhf].pdbfilehdr = NULL;
        }

        // Assert no longer valid as we reset this flag defore
        // calling CloseFile in ReadFile on error

        // check database as involved in an external backup
        // Assert ( g_rgfmp[ifmpT].FInBackupSession() );

#ifdef DEBUG
        if ( m_fDBGTraceBR )
        {
            const INT cbFillBuffer = 64;
            char szTrace[cbFillBuffer + 1];
            OSStrCbFormatA( szTrace, sizeof(szTrace), "STOP COPY DB" );
            CallS( m_pinst->m_plog->ErrLGTrace( ppibNil, szTrace ) );
        }
#endif

#ifdef MINIMAL_FUNCTIONALITY
#else
        // if the scrub object is left because they haven't read
        // all the pages from the db, hence we haven't stopped
        // the scrubbing at the end of the ReadFile phase
        if( m_pscrubdb )
        {
            // We get the operation's error here, and we can fail with an error from the disk ... should
            // be ignoreable in this cleanup.
            CallSRFS( m_pscrubdb->ErrTerm(), ( JET_errDiskIO, 0 ) ); // may fail with JET_errOutOfMemory. what to do?
            delete m_pscrubdb;
            m_pscrubdb = NULL;
        }
#endif

        CallS( ErrDBCloseDatabase( m_ppibBackup, ifmpT, 0 ) );
    }
    else
    {
        Assert ( ( backupStateLogsAndPatchs == m_fBackupStatus ) );

        delete m_rgrhf[irhf].pfapi;
        m_rgrhf[irhf].pfapi = NULL;
        if ( m_rgrhf[irhf].fIsLog )
        {
            delete m_rgrhf[irhf].pLogVerifyState;
            m_rgrhf[irhf].pLogVerifyState = pNil;
        }
    }

    // report end backup of file
    {
        WCHAR wszSizeRead[32];
        WCHAR wszSizeAll[32];

        Assert ( m_rgrhf[irhf].wszFileName );

        const WCHAR * rgszT[] = { m_rgrhf[irhf].wszFileName, wszSizeRead, wszSizeAll };

        if ( m_rgrhf[irhf].ib == m_rgrhf[irhf].cb )
        {
            // if all file read, report just EDB files
            if ( m_rgrhf[irhf].fDatabase )
            {
                UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, BACKUP_FILE_STOP_OK, 1, rgszT, 0, NULL, m_pinst );

                if (m_rgrhf[irhf].fDatabase)
                {
                    g_rgfmp[ m_rgrhf[irhf].ifmp ].SetEDBBackupDone( );
                }
                else
                {
                    Assert( fFalse );
                }

            }
            else if ( m_rgrhf[irhf].fIsLog )
            {
                // we can count this log as read if it is a required log
                // Note: we will add each generation number
                // and at the end of backup, they should have
                // the sum of all values between copyMic and copyMac
                //
                Assert( m_lgenCopyMic );
                Assert( m_lgenCopyMac );
                Assert( m_lgenCopyMic <= (m_lgenCopyMac - 1) );
                if ( m_rgrhf[irhf].lGeneration >= m_lgenCopyMic && m_rgrhf[irhf].lGeneration <= (m_lgenCopyMac - 1) )
                {
                    m_cGenCopyDone += m_rgrhf[irhf].lGeneration;
                }
            }

        }
        else
        // if not all file was read, issue a warning
        {
            OSStrCbFormatW( wszSizeRead, sizeof(wszSizeRead), L"%I64u", m_rgrhf[irhf].ib );
            OSStrCbFormatW( wszSizeAll, sizeof(wszSizeAll),  L"%I64u", m_rgrhf[irhf].cb );

            if ( m_fBackupIsInternal )
            {
                UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, INTERNAL_COPY_FILE_STOP_BEFORE_END, 3, rgszT, 0, NULL, m_pinst );
            }
            else
            {
                UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, BACKUP_FILE_STOP_BEFORE_END, 3, rgszT, 0, NULL, m_pinst );
            }
        }

        if ( m_rgrhf[irhf].fDatabase && !m_pinst->FRecovering() )
        {
            char szFileNameA[IFileSystemAPI::cchPathMax];
            CallS( ErrOSSTRUnicodeToAscii( m_rgrhf[irhf].wszFileName,
                                           szFileNameA,
                                           sizeof( szFileNameA ),
                                           NULL,
                                           OSSTR_ALLOW_LOSSY ) );
            const INT cbFillBuffer = 64 + IFileSystemAPI::cchPathMax;
            char szTrace[cbFillBuffer + 1];
            OSStrCbFormatA( szTrace, sizeof(szTrace), "Stop backup file %s", szFileNameA );

            CallS( m_pinst->m_plog->ErrLGTrace( m_ppibBackup, szTrace ) );
        }

    }

    OSMemoryHeapFree( m_rgrhf[irhf].wszFileName );

    /*  reset backup file handle and free
    /**/
    Assert( m_rgrhf[irhf].fInUse );

    m_rgrhf[irhf].fInUse            = fFalse;
    m_rgrhf[irhf].fDatabase         = fFalse;
    m_rgrhf[irhf].fIsLog            = fFalse;
    m_rgrhf[irhf].pLogVerifyState   = pNil;
    m_rgrhf[irhf].pfapi             = NULL;
    m_rgrhf[irhf].ifmp              = g_ifmpMax;
    m_rgrhf[irhf].ib                = 0;
    m_rgrhf[irhf].cb                = 0;
    m_rgrhf[irhf].wszFileName       = NULL;

#ifdef DEBUG
    if ( m_fDBGTraceBR )
    {
        CHAR sz[256];

        OSStrCbFormatA( sz, sizeof(sz), "** CloseFile (%d) - %ld Bytes.\n", 0, m_cbDBGCopied );
        Assert( strlen( sz ) <= sizeof( sz ) - 1 );
        DBGBRTrace( sz );
    }
#endif

    return JET_errSuccess;
}

ERR BACKUP_CONTEXT::ErrBKIPrepareLogInfo()
{
    ERR     err = JET_errSuccess;
    INT     irhf;
    WCHAR   wszPathJetChkLog[IFileSystemAPI::cchPathMax];

    if ( !m_fBackupInProgressLocal )
    {
        return ErrERRCheck( JET_errNoBackup );
    }
    else if ( m_fStopBackup )
    {
        return ErrERRCheck( JET_errBackupAbortByServer );
    }

    /*  all backup files must be closed
    /**/
    for ( irhf = 0; irhf < m_crhfMac; irhf++ )
    {
        if ( m_rgrhf[irhf].fInUse )
        {
            return ErrERRCheck( JET_errInvalidBackupSequence );
        }
    }

    Assert ( backupStateDatabases == m_fBackupStatus || backupStateLogsAndPatchs == m_fBackupStatus );


    /*  begin new log file and compute log backup parameters
    /**/
    if ( !m_fBackupBeginNewLogFile )
    {
        Call( ErrBKIPrepareLogFiles(
            m_fBackupFull ? 0 : JET_bitBackupIncremental ,
            SzParam( m_pinst, JET_paramLogFilePath ),
            wszPathJetChkLog,
            NULL ) );
    }

HandleError:
    return err;
}

ERR BACKUP_CONTEXT::ErrBKIGetLogInfo(
    const ULONG                 ulGenLow,
    const ULONG                 ulGenHigh,
    const BOOL                  fIncludePatch,
    __out_bcount_part_opt(cbMax, *pcbActual) PWSTR wszzLogs,
    _In_ ULONG                  cbMax,
    __out_opt ULONG             *pcbActual,
    JET_LOGINFO_W               *pLogInfo )
{
    ERR         err = JET_errSuccess;
    LONG        lT;
    WCHAR       *pch = NULL;
    WCHAR       *pchT;
    ULONG       cbActual;
    WCHAR       wszDirT[IFileSystemAPI::cchPathMax];
    WCHAR       wszFNameT[IFileSystemAPI::cchPathMax];
    WCHAR       wszExtT[IFileSystemAPI::cchPathMax];
    WCHAR       wszT[IFileSystemAPI::cchPathMax];
    WCHAR       wszFullLogFilePath[IFileSystemAPI::cchPathMax];

    /*  make full path from log file path, including trailing back slash
    /**/
    CallR( m_pinst->m_pfsapi->ErrPathComplete( SzParam( m_pinst, JET_paramLogFilePath ), wszFullLogFilePath ) );

#ifdef DEBUG
    if ( m_fDBGTraceBR )
        DBGBRTrace("** Begin GetLogInfo.\n" );
#endif

    if ( !FOSSTRTrailingPathDelimiterW( wszFullLogFilePath ) &&
         ( sizeof(WCHAR) * ( LOSStrLengthW( wszFullLogFilePath ) + 1 + 1 ) > sizeof( wszFullLogFilePath )) )
    {
        AssertSz( fFalse, "The path is invalid because the containing folder plus log file path come out to be more than MAX_PATH" );
        CallR( ErrERRCheck( JET_errInvalidPath ) );
    }
    CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszFullLogFilePath, sizeof(wszFullLogFilePath) ) );

    Assert ( m_fBackupBeginNewLogFile );

    /*  get cbActual for log file and patch files.
    /**/
    cbActual = 0;

    CallS( m_pinst->m_pfsapi->ErrPathParse( wszFullLogFilePath, wszDirT, wszFNameT, wszExtT ) );
    for ( lT = ulGenLow; (ULONG)lT < ulGenHigh; lT++ )
    {
        CallS( m_pinst->m_plog->ErrLGMakeLogNameBaseless(
                    wszT,
                    sizeof(wszT),
                    wszDirT,
                    eArchiveLog,
                    lT ) );
        cbActual += (ULONG) (sizeof(WCHAR) * ( LOSStrLengthW( wszT ) + 1 ) );
    }

    if ( fIncludePatch )
    {
        /*  put all the patch file info
        /**/
        for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            IFMP    ifmp = m_pinst->m_mpdbidifmp[ dbid ];
            if ( ifmp >= g_ifmpMax )
                continue;

            FMP *   pfmp = &g_rgfmp[ifmp];

            if ( pfmp->FInUse()
//              && pfmp->CpagePatch() > 0
                && pfmp->FAttached()
                && !pfmp->FAttachedForRecovery()
                && !pfmp->FAttachingDB()
                && pfmp->FInBackupSession()
                && pfmp->FBackupFileCopyDone())
            {
                Assert( !pfmp->FSkippedAttach() );
                Assert( !pfmp->FDeferredAttach() );

                /*  database with patch file must be loggable
                /**/

                Assert( pfmp->FLogOn() );
                BKIGetPatchName( wszT, pfmp->WszDatabaseName() );
                cbActual += (ULONG) (sizeof(WCHAR) * ( LOSStrLengthW( wszT ) + 1 ) );
            }
        }
    }
    cbActual += sizeof(WCHAR);

    Alloc( pch = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbActual ) ) );

    /*  return list of log files and patch files
    /**/
    pchT = pch;
    ULONG cchT = cbActual / sizeof(WCHAR);

    CallS( m_pinst->m_pfsapi->ErrPathParse( wszFullLogFilePath, wszDirT, wszFNameT, wszExtT ) );
    for ( lT = ulGenLow; (ULONG)lT < ulGenHigh; lT++ )
    {
        CallS( m_pinst->m_plog->ErrLGMakeLogNameBaseless(
                    wszT,
                    sizeof(wszT),
                    wszDirT,
                    eArchiveLog,
                    lT ) );
        Assert( pchT + LOSStrLengthW( wszT ) + 1 < pchT + ( cbActual / sizeof(WCHAR) ));
        OSStrCbCopyW( pchT, cchT * sizeof(WCHAR), wszT );
        cchT -= LOSStrLengthW( wszT );
        pchT += LOSStrLengthW( wszT );
        Assert( *pchT == 0 );
        cchT--;
        pchT++;
    }

    // on snapshot we don't have patch file
    if ( fIncludePatch )
    {
        /*  copy all the patch file info
        /**/
        for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            IFMP    ifmp = m_pinst->m_mpdbidifmp[ dbid ];
            if ( ifmp >= g_ifmpMax )
                continue;

            FMP *   pfmp = &g_rgfmp[ifmp];

            if ( pfmp->FInUse()
                && pfmp->FAttached()
                && !pfmp->FAttachedForRecovery()
                && !pfmp->FAttachingDB()
                && pfmp->FInBackupSession()
                && pfmp->FBackupFileCopyDone())
            {
                Assert( !pfmp->FSkippedAttach() );
                Assert( !pfmp->FDeferredAttach() );

                BKIGetPatchName( wszT, pfmp->WszDatabaseName() );
                Assert( pchT + LOSStrLengthW( wszT ) + 1 < pchT + ( cbActual / sizeof( WCHAR ) ) );
                OSStrCbCopyW( pchT, cchT * sizeof(WCHAR), wszT );
                cchT -= LOSStrLengthW( wszT );
                pchT += LOSStrLengthW( wszT );
                Assert( *pchT == 0 );
                cchT--;
                pchT++;
            }
        }
    }
    Assert( pchT == pch + cbActual/sizeof(WCHAR) - 1 );
    *pchT = L'\0';

    /*  return data
    /**/
    if ( wszzLogs != NULL )
    {
        UtilMemCpy( wszzLogs, pch, min( cbMax, cbActual ) );

        /*  return cbActual
        /**/
        if ( pcbActual != NULL )
        {
            *pcbActual = min( cbMax, cbActual );
        }
    }
    else
    {
        /*  return cbActual
        /**/
        if ( pcbActual != NULL )
        {
            *pcbActual = cbActual;
        }
    }


HandleError:

    if ( pch != NULL )
    {
        OSMemoryHeapFree( pch );
        pch = NULL;
    }

#ifdef DEBUG
    if ( m_fDBGTraceBR )
    {
        CHAR sz[256];
        WCHAR *pwchT; // added T to not be confused pch, which should be pwch anyway

        if ( err >= 0 )
        {
            OSStrCbFormatA( sz, sizeof(sz), "   Log Info with cbActual = %d and cbMax = %d :\n", cbActual, cbMax );
            Assert( strlen( sz ) <= sizeof( sz ) - 1 );
            DBGBRTrace( sz );

            if ( wszzLogs != NULL )
            {
                pwchT = wszzLogs;

                do {
                    if ( LOSStrLengthW( pwchT ) != 0 )
                    {
                        CAutoSZ lszTemp;
                        if ( lszTemp.ErrSet( pwchT ) )
                        {
                            OSStrCbFormatA( sz, sizeof(sz), "     <errInToAsciiConversion>\n" );
                        }
                        else
                        {
                            // success.
                            OSStrCbFormatA( sz, sizeof(sz), "     %s\n", (CHAR*)lszTemp );
                        }
                        Assert( strlen( sz ) <= sizeof( sz ) - 1 );
                        DBGBRTrace( sz );
                        pwchT += LOSStrLengthW( pwchT );
                    }
                    pwchT++;
                } while ( *pwchT );
            }
        }

        OSStrCbFormatA( sz, sizeof(sz), "   End GetLogInfo (%d).\n", err );
        Assert( strlen( sz ) <= sizeof( sz ) - 1 );
        DBGBRTrace( sz );
    }
#endif

    if ( err < 0 )
    {
        CallS( ErrBKExternalBackupCleanUp( err ) );
        Assert( !m_fBackupInProgressLocal && !m_fBackupInProgressAny );
    }
    else
    {
        if ( NULL != pLogInfo )
        {
            Assert ( pLogInfo->cbSize == sizeof( JET_LOGINFO_W ) );

            pLogInfo->ulGenLow = ulGenLow;
            pLogInfo->ulGenHigh = ulGenHigh - 1;
            Assert ( JET_BASE_NAME_LENGTH == LOSStrLengthW( SzParam( m_pinst, JET_paramBaseName ) ) );
            OSStrCbCopyW( pLogInfo->szBaseName, sizeof( pLogInfo->szBaseName ), SzParam( m_pinst, JET_paramBaseName ) );
            pLogInfo->szBaseName[JET_BASE_NAME_LENGTH] = 0;
        }

        Assert ( backupStateDatabases == m_fBackupStatus || backupStateLogsAndPatchs == m_fBackupStatus );
        // switch to the LogAndPatch status
        m_fBackupStatus = backupStateLogsAndPatchs;
    }
    return err;
}


ERR ISAMAPI ErrIsamGetLogInfo( JET_INSTANCE jinst,  __out_bcount_part_opt(cbMax, *pcbActual) PWSTR wszzLogs, ULONG cbMax, ULONG *pcbActual, JET_LOGINFO_W *pLogInfo )
{
    INST *pinst = (INST *)jinst;
    return pinst->m_pbackup->ErrBKGetLogInfo( wszzLogs, cbMax, pcbActual, pLogInfo, fFalse );
}

ERR BACKUP_CONTEXT::ErrBKGetLogInfo(
    __out_bcount_part_opt(cbMax, *pcbActual) PWSTR wszzLogs,
    _In_ ULONG                  cbMax,
    _Out_ ULONG                 *pcbActual,
    JET_LOGINFO_W               *pLogInfo,
    const BOOL                  fIncludePatch )
{
    ERR     err = JET_errSuccess;

    CallR ( ErrBKIPrepareLogInfo() );

    Assert ( m_lgenCopyMic );
    Assert ( m_lgenCopyMac );
    Assert ( m_lgenCopyMic <= m_lgenCopyMac );
    return ErrBKIGetLogInfo( m_lgenCopyMic, m_lgenCopyMac, fIncludePatch, wszzLogs, cbMax, pcbActual, pLogInfo );
}


ERR ISAMAPI ErrIsamGetTruncateLogInfo( JET_INSTANCE jinst,  __out_bcount_part_opt(cbMax, *pcbActual) PWSTR wszzLogs, ULONG cbMax, ULONG *pcbActual )
{
    INST *pinst = (INST *)jinst;
    return pinst->m_pbackup->ErrBKGetTruncateLogInfo( wszzLogs, cbMax, pcbActual );
}

ERR BACKUP_CONTEXT::ErrBKGetTruncateLogInfo(
    __out_bcount_part_opt(cbMax, *pcbActual) PWSTR wszzLogs,
    _In_ ULONG                  cbMax,
    _Out_ ULONG                 *pcbActual )
{
    ERR     err = JET_errSuccess;

    CallR ( ErrBKIPrepareLogInfo() );

    Assert ( m_lgenDeleteMic <= m_lgenDeleteMac );

    return ErrBKIGetLogInfo( m_lgenDeleteMic, m_lgenDeleteMac, fFalse, wszzLogs, cbMax, pcbActual, NULL);
}


ERR ISAMAPI ErrIsamTruncateLog( JET_INSTANCE jinst )
{
    INST *pinst = (INST *)jinst;
    return pinst->m_pbackup->ErrBKTruncateLog();
}

ERR BACKUP_CONTEXT::ErrBKTruncateLog()
{
    if ( !m_fBackupInProgressLocal )
    {
        return ErrERRCheck( JET_errNoBackup );
    }
    else if ( m_fStopBackup )
    {
        return ErrERRCheck( JET_errBackupAbortByServer );
    }

    //  all backup files must be closed
    for ( INT irhf = 0; irhf < m_crhfMac; irhf++ )
    {
        if ( m_rgrhf[irhf].fInUse )
        {
            return ErrERRCheck( JET_errInvalidBackupSequence );
        }
    }

    if ( 0 == m_lgenCopyMic ||
        0 == m_lgenCopyMac )
    {
        // The copy mic/mac are set during prepare log files, if we have a 0 here
        // it is an invalid backup sequence.
        return ErrERRCheck( JET_errInvalidBackupSequence );
    }

    // switch to the LogAndPatch status if not already there
    Assert( backupStateDatabases == m_fBackupStatus         //  this state is possible via JetBackup( NULL )
        || backupStateLogsAndPatchs == m_fBackupStatus );
    m_fBackupStatus = backupStateLogsAndPatchs;

    // we need to check if they got all the log files they need
    // The files they need are from m_lgenCopyMic to (m_lgenCopyMac - 1) inclusive.
    // The formula for the sum of all numbers from A to B inclusive is
    // S = ( (A+B)(B-A+1) / 2 )
    //
    Assert( m_lgenCopyMic );
    Assert( m_lgenCopyMac );
    Assert( m_lgenCopyMic <= (m_lgenCopyMac - 1) );
    const SIZE_T        cGenCopyMust = ( ( (SIZE_T)m_lgenCopyMic + (SIZE_T)(m_lgenCopyMac - 1) ) * ( (SIZE_T)(m_lgenCopyMac - 1) - (SIZE_T)m_lgenCopyMic + 1  ) ) / 2;

    // we don't check with == becuase there is a chance
    // that they copy one file twice
    // ( like it didn't fit in the destination the first time )
    // It is ok as it should cover the big majority of code bugs
    // in which they fail to copy some file
    //
    if ( m_cGenCopyDone < cGenCopyMust )
    {
        UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY, BACKUP_LOG_FILE_NOT_COPIED_ID, 0, NULL, 0 , NULL, m_pinst );

        return ErrERRCheck( JET_errLogFileNotCopied );
    }

    BKSetLogsTruncated( fTrue );

    return m_pinst->m_plog->ErrLGTruncateLog( m_lgenDeleteMic, m_lgenDeleteMac, fFalse, m_fBackupFull );
}


ERR BACKUP_CONTEXT::ErrBKOSSnapshotTruncateLog( const JET_GRBIT grbit )
{
    ERR     err = JET_errSuccess;

    m_lgenDeleteMic = 0;

    // if m_lgenDeleteMac is 0 at this point
    // (like the instance started after the snapshot thaw but before Truncate is called)
    // then the instance is not part of the snaphot
    if ( 0 == m_lgenDeleteMac )
    {
        return JET_errSuccess;
    }


    if ( JET_bitAllDatabasesSnapshot == grbit && !BoolParam( m_pinst, JET_paramCircularLog ) )
    {
        Call ( m_pinst->m_plog->ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), &m_lgenDeleteMic, NULL ) );

        if ( m_lgenDeleteMic > m_lgenDeleteMac )
        {
            AssertSz( fFalse, "Someone has probably tampered with the on-disk files before we got them recorded in the backup." );
            Error( ErrERRCheck( JET_errRequiredLogFilesMissing ) );
        }

        Call ( m_pinst->m_plog->ErrLGTruncateLog( m_lgenDeleteMic, m_lgenDeleteMac, fTrue, m_fBackupFull ) );
    }
    else
    {
        // If not all the databaseses are attached, we won't truncate
        // but we make the call with 0-0 range just to display the EventLog
        // Also, if circular logging is on, we won't truncate as in this case
        // snapshot is not forcing a new log generation and we should avoid
        // coliding with the logging thread on old log deletion
        Call ( m_pinst->m_plog->ErrLGTruncateLog( 0, 0, fTrue, m_fBackupFull ) );
    }

HandleError:


    // reset the range to avoid further truncate attempts
    m_lgenDeleteMic = 0;
    m_lgenDeleteMac = 0;
    return err;
}


void BACKUP_CONTEXT::BKOSSnapshotSaveInfo( const BOOL fIncremental, const BOOL fCopy )
{
    ERR     errUpdateLog = JET_errSuccess;
    ERR     errUpdateHdr = JET_errSuccess;
    DBID    dbid;
    BOOL    fLogged = fFalse;

    for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP    ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP *pfmp = &g_rgfmp[ifmp];

        if ( !pfmp->FInUse() || !pfmp->FLogOn() || !pfmp->FAttached() || pfmp->FAttachedForRecovery() || pfmp->FAttachingDB() )
        {
            continue;
        }

        Assert( !pfmp->FSkippedAttach() );
        Assert( !pfmp->FDeferredAttach() );

        ERR errT;
        errT = ErrBKUpdateHdrBackupInfo( ifmp,
                                DBFILEHDR::backupOSSnapshot,
                                fIncremental,
                                !fCopy,
                                m_lgenCopyMic,
                                m_lgenCopyMac,
                                fIncremental ? &m_lgposIncBackup : &m_lgposFullBackup,
                                fIncremental ? &m_logtimeIncBackup : &m_logtimeFullBackup,
                                &fLogged );
        errUpdateLog = ( errUpdateLog < JET_errSuccess ) ? errUpdateLog : errT;
        
    }

    // write the db header so that the info we updated is written.
    // otherwise a crash before other db header updates
    // may result in backup information lost.

    BOOL    fSkippedAttachDetach;
    LOGTIME tmEmpty;
    memset( &tmEmpty, 0, sizeof(LOGTIME) );

    m_pinst->m_plog->LockCheckpoint();
    errUpdateHdr = m_pinst->m_plog->ErrLGUpdateGenRequired( m_pinst->m_pfsapi, 0, 0, 0, tmEmpty, &fSkippedAttachDetach );
    m_pinst->m_plog->UnlockCheckpoint();

    // on error, report a Warning and continue as the backup is OK from it's point of view.
    if ( errUpdateLog < JET_errSuccess || errUpdateHdr < JET_errSuccess )
    {
        WCHAR   sz1T[32];
        const WCHAR *rgszT[1];

        OSStrCbFormatW( sz1T, sizeof(sz1T), L"%d", ( errUpdateLog < JET_errSuccess ) ? errUpdateLog : errUpdateHdr );
        rgszT[0] = sz1T;

        UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY, BACKUP_ERROR_INFO_UPDATE, 1, rgszT, 0 , NULL, m_pinst );
    }

    return;
}


ERR BACKUP_CONTEXT::ErrBKOSSnapshotStopLogging( const BOOL fIncremental )
{
    ERR err                             = JET_errSuccess;
    BOOL fLogStopped                    = fFalse;
    const BOOL fCircularLogging         = BoolParam( m_pinst, JET_paramCircularLog );
    const BOOL fCreateAndStopOnNewGen   = fIncremental || !fCircularLogging;
    const JET_GRBIT grbitLogRec         = fCreateAndStopOnNewGen ? ( fLGCreateNewGen | fLGStopOnNewGen ) : NO_GRBIT;

    if ( !fCircularLogging )
    {
        m_pinst->m_plog->LGSignalLogPaused( fFalse );
    }
    else
    {
        // nothing, we will just enter m_critLGFlush once we log the snapshot start
    }

    if ( !fIncremental )
    {
        LGIGetDateTime( &m_logtimeFullBackup );

        //  fLGStopOnNewGen forces the log flush thread to set m_sigLogPaused
        //  when the new gen has been created, and we will wait on that signal below
        //
        Call ( ErrLGBackupPrepLogs(
                    m_pinst->m_plog,
                    DBFILEHDR::backupOSSnapshot, fIncremental,
                    &m_logtimeFullBackup,
                    grbitLogRec,
                    &m_lgposFullBackup ) );

        if ( fCreateAndStopOnNewGen )
        {
            //  wait for log flush thread to create
            //  the new gen (we used the fLGStopOnNewGen
            //  flag above to indicate to the log flush
            //  thread that upon creation of the new
            //  gen, set this signal and bail)
            //
            m_pinst->m_plog->LGWaitLogPaused();
            Assert( m_pinst->m_plog->FLGLogPaused() || m_pinst->m_plog->FNoMoreLogWrite() );
        }

        fLogStopped = fTrue;
        
        Assert( fCircularLogging || m_lgposFullBackup.lGeneration > 1 );

        // this will be set later after getting the checkpoint critical section.
        m_lgenCopyMic = 0;
        m_lgenCopyMac = m_lgposFullBackup.lGeneration;

        // if no circular logging, we do switch to the new log with
        // the log record so the last log generation is less then
        // the one where the Backup log record is as that one is the
        // first in the next log which is not generated yet.
        // Note: if we didn't get a lgpos because we couldn't log
        // the new log record (m_lgenCopyMac will be 0), then
        // we will get the new log generation number later
        // after we wait for it it finish the switch
        if ( !fCircularLogging )
        {
            Assert( 0 != CmpLgpos( &m_lgposFullBackup, &lgposMin ) );
            m_lgenCopyMac--;
        }
    }
    else
    {

        // we need to check if all dbs do have a full backup
        Call( ErrLGCheckIncrementalBackup( m_pinst, DBFILEHDR::backupOSSnapshot ) );

        if ( fCircularLogging )
        {
            Call( ErrERRCheck( JET_errInvalidBackup ) );
        }

        LGIGetDateTime( &m_logtimeIncBackup );

        //  fLGStopOnNewGen forces the log flush thread to set m_sigLogPaused
        //  when the new gen has been created, and we will wait on that signal below
        //
        Call ( ErrLGBackupPrepLogs(
                    m_pinst->m_plog,
                    DBFILEHDR::backupOSSnapshot, fIncremental,
                    &m_logtimeIncBackup,
                    grbitLogRec,
                    &m_lgposIncBackup ) );

        if ( fCreateAndStopOnNewGen )
        {
            //  wait for log flush thread to create
            //  the new gen (we used the fLGStopOnNewGen
            //  flag above to indicate to the log flush
            //  thread that upon creation of the new
            //  gen, set this signal and bail)
            //
            m_pinst->m_plog->LGWaitLogPaused();
            Assert( m_pinst->m_plog->FLGLogPaused() || m_pinst->m_plog->FNoMoreLogWrite() );
        }

        fLogStopped = fTrue;
        
        Assert (m_lgposIncBackup.lGeneration > 1);
        m_lgenCopyMac = m_lgposIncBackup.lGeneration - 1;

        // on incrementals, we need to copy from the first existing log
        Call ( m_pinst->m_plog->ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), &m_lgenCopyMic, NULL ) );

        // we were just during the first log
        if ( 0 == m_lgenCopyMic )
        {
            Assert( 1 == m_lgenCopyMac );
            m_lgenCopyMic = 1;
        }

        // we need to check if the first existing log is
        // enough for all databases since the previous backup
        Call( ErrBKICheckLogsForIncrementalBackup( m_lgenCopyMic ) );

    }


    if ( !fCircularLogging )
    {
        //  if we don't have a valid m_lgposFullBackup then let's set it to a
        //  safe value so that the rest of the code functions properly
        //
        //  NOTE:  we could try to scan all attached FMPs and get the min
        //  of the max of the prev full backup and prev incr backup for each
        //  FMP and use that number, but there is always a case where we could
        //  take a snap with no databases attached so we need this safe value
        //  case anyway.  also, we used something like this safe value for
        //  the entire history of snapshot backup in ESE anyway
        //
        if ( 0 == CmpLgpos( &m_lgposFullBackup, &lgposMin ) )
        {
            m_lgposFullBackup.lGeneration = m_lgenCopyMac + 1;
            m_lgposFullBackup.isec = 0;
            m_lgposFullBackup.ib = 0;
        }

    }
    else
    {
        WCHAR   wszLogName[ IFileSystemAPI::cchPathMax ];

        //  will be released by CESESnapshotSession::ThawInstance()
        //
        m_pinst->m_plog->LGLockWrite();

        // we need to delete the incomplete edbtmp.jtx/log because
        // otherwise the snapshot image will have it and it will not checksum
        // This is the case only for the circular logging case in which we just
        // stop logging, for the normal case where will be an edbtmp.jtx/log ready
        // to be used as edb.jtx/log (i.e. will checksum fine)

        //  If edbtmp.jtx/log is being written to, cancel the operation and close file.

        m_pinst->m_plog->LGCreateAsynchCancel( fFalse );

        //  Delete any existing edbtmp.jtx/log since we would need to recreate
        //  it at next startup anyway (we never trust any prepared log files,
        //  not edbtmp.jtx/log, not edb00001.jrs, nor edb00002.jrs).

        m_pinst->m_plog->LGMakeLogName( wszLogName, sizeof(wszLogName), eCurrentTmpLog );

        // If the deletion fails, we keep going, no point in failing the snapshot:
        // If the backup app will checksum the image and this file will fail the checksum
        // then it will fail anyway. If they don't checksum (or they know it is a tmp log)
        // we will recover fine anyway.
        (void) m_pinst->m_pfsapi->ErrFileDelete( wszLogName );
    }


HandleError:
    if ( JET_errSuccess > err )
    {
        m_lgenCopyMic = 0;
        m_lgenCopyMac = 0;

        if ( fLogStopped )
        {
            BKOSSnapshotResumeLogging();
        }
    }
    return err;
}

VOID BACKUP_CONTEXT::BKOSSnapshotResumeLogging()
{
    INST * const pinst                              = m_pinst;
    CESESnapshotSession * const pOSSnapshotSession  = pinst->m_pOSSnapshotSession;

    Assert( NULL != pOSSnapshotSession );

    if ( BoolParam( pinst, JET_paramCircularLog ) )
    {
        WCHAR   wszTempLogName[ IFileSystemAPI::cchPathMax ];

        //  m_critLGWrite obtained by BACKUP_CONTEXT::ErrBKOSSnapshotStopLogging()
        //

        // we did stop the async log file creation on freeze for circular logging
        // so we need to restart it now.
        m_pinst->m_plog->LGMakeLogName( wszTempLogName, sizeof(wszTempLogName), eCurrentTmpLog );

        CallS( m_pinst->m_plog->ErrStartAsyncLogFileCreation( wszTempLogName, lGenSignalTempID ) );
    }
    else
    {
        m_pinst->m_plog->LGLockWrite();
        Assert ( m_pinst->m_plog->FLGLogPaused() );

        ERR err = JET_errSuccess;

        //  finish creating the logfile we started in Freeze
        //  We don't want to return an error from Thaw and
        //  the instance will be "m_fLGNoMoreLogWrite" anyway
        err = m_pinst->m_plog->ErrLGNewLogFile(
                                0 /* not used for rename only */,
                                fLGLogRenameOnly);

        // on error, we continue with the other instances.
        // The error for renaming the logs was logged in the EventLog
        // and the log was made unavailable.
        Assert ( JET_errSuccess <= err || m_pinst->m_plog->FNoMoreLogWrite() );

        m_pinst->m_plog->LGSetLogPaused( fFalse );
    }
    m_pinst->m_plog->LGUnlockWrite();

    // there might be waiters on log to write (like Commit0)
    // which were blocked by the log write freezing
    // so try to write to make those wake up
    //
    //  WARNING: a log write is not guaranteed,
    //  because a log write won't be signalled if
    //  we can't acquire the right to signal a log
    //  write (typically because a log write is
    //  already under way, but note that it could
    //  be at the very end of that process)
    //
    m_pinst->m_plog->FLGSignalWrite( );

    // we add a trace for the end of the snapshot. This will contain also
    // the snapshot start LGPOS which will be invalid if the freeze wasn't
    // able to log the trace for the start of the snapshot
    const INT cbFillBuffer = 128;
    char szTrace[cbFillBuffer + 1];
    LGPOS lgposFreezeLogRec = lgposMax;

    BKIOSSnapshotGetFreezeLogRec( &lgposFreezeLogRec );

    //  we don't expect this method to be called if BACKUP_CONTEXT::ErrBKOSSnapshotStopLogging() did not succeed
    Assert( 0 != CmpLgpos( &lgposFreezeLogRec, &lgposMax ) );

    if ( 0 != CmpLgpos( &lgposFreezeLogRec, &lgposMax ) )
    {
        OSStrCbFormatA( szTrace, cbFillBuffer, "OSSnapshot Resume (Stop at (%X,%X,%X)%s%s)",
            lgposFreezeLogRec.lGeneration,
            lgposFreezeLogRec.isec,
            lgposFreezeLogRec.ib,
            pOSSnapshotSession->IsIncrementalSnapshot() ? " incremental" : "",
            pOSSnapshotSession->IsCopySnapshot() ? " copy" : "" );
    }
    else
    {
        OSStrCbFormatA( szTrace, cbFillBuffer, "OSSnapshot Resume (Stop at (failed)%s%s)",
            pOSSnapshotSession->IsIncrementalSnapshot() ? " incremental" : "",
            pOSSnapshotSession->IsCopySnapshot() ? " copy" : "" );
    }

    (void)m_pinst->m_plog->ErrLGTrace( ppibNil, szTrace );
}

VOID BACKUP_CONTEXT::BKIOSSnapshotGetFreezeLogRec( LGPOS* const plgposFreezeLogRec )
{
    INST * const pinst                              = m_pinst;
    CESESnapshotSession * const pOSSnapshotSession  = pinst->m_pOSSnapshotSession;

    Assert( NULL != pOSSnapshotSession );

    *plgposFreezeLogRec = lgposMax;
    if ( NULL != pOSSnapshotSession )
    {
        if ( pOSSnapshotSession->IsIncrementalSnapshot( ) )
        {
            *plgposFreezeLogRec = m_lgposIncBackup;
        }
        else
        {
            *plgposFreezeLogRec = m_lgposFullBackup;
        }
    }
}

ERR ISAMAPI ErrIsamEndExternalBackup(  JET_INSTANCE jinst, JET_GRBIT grbit )
{
    INST *pinst = (INST *)jinst;
    BOOL    fNormalTermination = grbit & JET_bitBackupEndNormal;
    ERR     err = JET_errSuccess;

    if ( fNormalTermination && (grbit & JET_bitBackupEndAbort) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( !fNormalTermination && !(grbit & JET_bitBackupEndAbort) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    Assert ( (grbit & JET_bitBackupEndNormal) || (grbit & JET_bitBackupEndAbort) &&
            !( (grbit & JET_bitBackupEndNormal) && (grbit & JET_bitBackupEndAbort) ) );

    if ( !fNormalTermination )
    {
        err = ErrERRCheck( errBackupAbortByCaller );
    }

    return pinst->m_pbackup->ErrBKExternalBackupCleanUp( err, grbit );
}

ERR BACKUP_CONTEXT::ErrBKUpdateHdrBackupInfo(
    const IFMP              ifmp,
    DBFILEHDR::BKINFOTYPE   bkinfoType,
    const BOOL              fLogOnly,       // eg1: fLogOnly && fLogTruncated == Incremental
    const BOOL              fLogTruncated,  // eg2: !fLogOnly && !fLogTruncated == Copy
    const INT                   lgenLow,
    const INT                   lgenHigh,
    const LGPOS *           plgposMark,
    const LOGTIME *             plogtimeMark,
    BOOL *                  pfLogged
    )
{
    ERR         err = JET_errSuccess;

    FMP *       pfmp = &g_rgfmp[ifmp];
    LGPOS       lgposRecT;

    BOOL        fLoggedNow = fFalse;

    Assert( pfLogged );
    Assert( plgposMark );
    Assert( plogtimeMark );

    Assert( lgenLow && lgenHigh && lgenLow <= lgenHigh );

    Assert( pfmp
                && pfmp->FInUse()
                && pfmp->FLogOn()
                && pfmp->FAttached() );

    Assert( ( *pfLogged == fTrue ) ||
            ( bkinfoType == DBFILEHDR::backupOSSnapshot ) ||
            ( pfmp->FInBackupSession() ) );

    if ( !(*pfLogged) )
    {
        Enforce( !m_pinst->FRecovering() );

        CallR( ErrLGBackupUpdate( m_pinst->m_plog, bkinfoType, fLogOnly, fLogTruncated,
                                    lgenLow, lgenHigh, plgposMark, plogtimeMark,
                                    pfmp->Dbid(),
                                    &lgposRecT ) );
        fLoggedNow = fTrue; // just used to help us check an assert below.
    }

    // Surrogate backups are actually snapshot backups on another machine, so we put them
    // as snapshot backups for the header, but note we loged the bkinfoType with full fidelity.
    bkinfoType = ( bkinfoType == DBFILEHDR::backupSurrogate ) ? DBFILEHDR::backupOSSnapshot : bkinfoType;

    //  Grab the checkpoint
    m_pinst->m_plog->LockCheckpoint();

    Assert( pfmp->Pdbfilehdr() );
    struct BKINFO2
    {
        LittleEndian<DBFILEHDR_FIX::BKINFOTYPE>* m_pbkType;
        BKINFO* m_pbkInfo;
    };

    {
        PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
        BKINFO2 rgbkInfo2[2][2] =
        {
                { { &pdbfilehdr->bkinfoTypeCopyPrev, &pdbfilehdr->bkinfoCopyPrev, }, {&pdbfilehdr->bkinfoTypeFullPrev, &pdbfilehdr->bkinfoFullPrev, }, },
                { { &pdbfilehdr->bkinfoTypeDiffPrev, &pdbfilehdr->bkinfoDiffPrev, }, {&pdbfilehdr->bkinfoTypeIncPrev, &pdbfilehdr->bkinfoIncPrev, }, },
        };

        // select out proper field for future reference
        BKINFO2 bkInfo2 = rgbkInfo2[ !!fLogOnly ][ !!fLogTruncated ];

        if ( CmpLgpos( &bkInfo2.m_pbkInfo->le_lgposMark, plgposMark ) <= 0 )
        {
            Assert( fLoggedNow || *pfLogged == fTrue );
        }
        else
        {
            goto NoHdrUpdate;
        }

        *bkInfo2.m_pbkType = bkinfoType;
        bkInfo2.m_pbkInfo->le_lgposMark = *plgposMark;
        bkInfo2.m_pbkInfo->logtimeMark = *plogtimeMark;
        bkInfo2.m_pbkInfo->le_genLow = lgenLow;
        bkInfo2.m_pbkInfo->le_genHigh = lgenHigh;

        //
        //  Clear out and invalidate appropriate backup varieties
        //
        if ( fLogTruncated )
        {
            if ( !fLogOnly )
            {
                // new full, nullifies prev incremental
                pdbfilehdr->bkinfoTypeIncPrev = DBFILEHDR::backupNormal;
                memset( &pdbfilehdr->bkinfoIncPrev, 0, sizeof( BKINFO ) );

                // new full, nullifies prev diff as well
                pdbfilehdr->bkinfoTypeDiffPrev = DBFILEHDR::backupNormal;
                memset( &pdbfilehdr->bkinfoDiffPrev, 0, sizeof( pdbfilehdr->bkinfoDiffPrev ) );
            }

            switch( bkinfoType )
            {
                case DBFILEHDR::backupNormal:
                    // clean the snapshot info after a normal full backup
                    memset( &(pdbfilehdr->bkinfoSnapshotCur), 0, sizeof( BKINFO ) );
                    pdbfilehdr->bkinfoTypeIncPrev = DBFILEHDR::backupNormal;
                    break;

                case DBFILEHDR::backupOSSnapshot:
                    // no need to clear anything.
                    break;

                case DBFILEHDR::backupSnapshot:
                case DBFILEHDR::backupSurrogate:
                default:
                    Assert( fFalse );
                    break;
            }
        }
    
    }

NoHdrUpdate:
    
    m_pinst->m_plog->UnlockCheckpoint();
    *pfLogged = ( *pfLogged ) ? fTrue : fLoggedNow;

    return err;
}

ERR BACKUP_CONTEXT::ErrBKExternalBackupCleanUp( ERR error, const JET_GRBIT grbit )
{
    BOOL    fNormal     = ( JET_errSuccess == error );
    ERR     err;
    ERR     errUpdateLog = JET_errSuccess;
    DBID    dbid;
    const BOOL  fSurrogateBackup = ( JET_bitBackupSurrogate == ( JET_bitBackupSurrogate & grbit ) );

    if ( !m_fBackupInProgressAny )
    {
        m_fStopBackup = fFalse;
        m_fBackupStatus = backupStateNotStarted;
        return ErrERRCheck( JET_errNoBackup );
    }

    if ( ( fSurrogateBackup && m_fBackupInProgressLocal ) ||
        ( !fSurrogateBackup && !m_fBackupInProgressLocal ) )
    {
        // They've mismatched the "destinations" of the backups from begin to end, fail
        // them out.
        fNormal = fFalse;
        if ( m_fBackupInProgressLocal )
        {
            // tried to end local backup w/ surrogate backup bit.
            error = ErrERRCheck( JET_errBackupInProgress );
        }
        else
        {
            // tried to end surrogate backup w/ external backup.
            error = ErrERRCheck( JET_errSurrogateBackupInProgress );
        }
    }

    if ( fNormal )
    {
        if ( m_fStopBackup )
        {
            //  premature termination specifically requested
            //
            fNormal = fFalse;
            error = ErrERRCheck( JET_errBackupAbortByServer );
        }
        else if ( m_fBackupStatus == backupStateDatabases )
        {
            // if backup client calls BackupEnd without error
            // before logs are read, force the backup as "with error"
            //
            fNormal = fFalse;
            error = ErrERRCheck( errBackupAbortByCaller );
        }
    }


    /*  delete patch files, if present, for all databases.
    /**/
    //  first close the patch file if needed
    for ( INT irhf = 0; irhf < crhfMax; ++irhf )
    {
        if ( m_rgrhf[irhf].fInUse && m_rgrhf[irhf].pfapi )
        {
            Assert ( !m_rgrhf[irhf].fDatabase );
            delete m_rgrhf[irhf].pfapi;
            m_rgrhf[irhf].pfapi = NULL;
        }
    }

    // clean up patch file state
    for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP    ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP *pfmp = &g_rgfmp[ifmp];

        if ( pfmp->FInUse()
            && pfmp->FLogOn()
            && pfmp->FAttached()
            && !pfmp->FAttachingDB()
            && pfmp->FInBackupSession() )
        {
            Assert( !pfmp->FSkippedAttach() );
            Assert( !pfmp->FDeferredAttach() );

            // only full backup is using patch files
            if ( m_fBackupFull )
            {
                pfmp->CritLatch().Enter();
                pfmp->SetPgnoBackupMost( 0 );
                pfmp->SetPgnoBackupCopyMost( 0 );
                pfmp->ResetFCopiedPatchHeader();
                pfmp->CritLatch().Leave();
            }
        }
    }

    //
    //  Log and update Database headers, or log abort.
    //
    if ( fNormal &&
        !( JET_bitBackupNoDbHeaderUpdate & grbit ) )
    {

        for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            IFMP    ifmp = m_pinst->m_mpdbidifmp[ dbid ];
            if ( ifmp >= g_ifmpMax )
                continue;

            FMP *pfmp = &g_rgfmp[ifmp];

            if ( pfmp->FInUse()
                && pfmp->FLogOn()
                && pfmp->FAttached()
                && !pfmp->FAttachedForRecovery()
                && !pfmp->FAttachingDB()
                && pfmp->FInBackupSession() )
            {
                Assert( !pfmp->FSkippedAttach() );
                Assert( !pfmp->FDeferredAttach() );

                // log each one, b/c params may vary from FMP to FMP...

                if ( fSurrogateBackup )
                {
                    // surrogate backup
                    BOOL fLoggedT = fFalse;
                    err = ErrBKUpdateHdrBackupInfo( ifmp,
                        DBFILEHDR::backupSurrogate, !m_fBackupFull, grbit & JET_bitBackupTruncateDone,
                        m_lgenCopyMic, m_lgenCopyMac,
                        m_fBackupFull ? &m_lgposFullBackup : &m_lgposIncBackup,
                        m_fBackupFull ? &m_logtimeFullBackup : &m_logtimeIncBackup,
                        &fLoggedT );
                }
                else if ( m_fBackupFull && pfmp->FBackupFileCopyDone() )
                {
                    // full backup
                    Assert( pfmp->Ppatchhdr() );
                    Assert( pfmp->Pdbfilehdr()->bkinfoFullCur.le_genLow == 0 );
                    PATCH_HEADER_PAGE * ppatchHdr = pfmp->Ppatchhdr();

                    LGPOS lgposT = ppatchHdr->bkinfo.le_lgposMark; // needs help converting from & le_lgpos to const * lgpos
                    BOOL fLoggedT = fFalse;
                    err = ErrBKUpdateHdrBackupInfo( ifmp,
                        DBFILEHDR::backupNormal, fFalse /* !m_fBackupFull */,
                        JET_bitBackupTruncateDone & grbit || FBKLogsTruncated(),
                        ppatchHdr->bkinfo.le_genLow, m_lgenCopyMac - 1,
                        &(lgposT), &(ppatchHdr->bkinfo.logtimeMark),
                        &fLoggedT );
                }
                else if ( !m_fBackupFull )
                {
                    // incremental backup
                    BOOL fLoggedT = fFalse;
                    err = ErrBKUpdateHdrBackupInfo( ifmp,
                        DBFILEHDR::backupNormal, fTrue /* !m_fBackupFull */,
                        JET_bitBackupTruncateDone & grbit || FBKLogsTruncated(),
                        m_lgenCopyMic, m_lgenCopyMac - 1,
                        &m_lgposIncBackup, &m_logtimeIncBackup,
                        &fLoggedT );
                }
                else
                {
                    // must've been a full backup, but the copy wasn't done....
                    Assert( !pfmp->FBackupFileCopyDone() );
                    err = JET_errSuccess; // N/A but make sure it causes no problems.
                }
                errUpdateLog = ( errUpdateLog < JET_errSuccess ) ? errUpdateLog : err;
                err = JET_errSuccess;

            }
        } // end for each db

    }

    if ( !fNormal && !m_pinst->FRecovering() )
    {
        LGPOS       lgposRecT;
        ErrLGBackupAbort( m_pinst->m_plog, fSurrogateBackup ? DBFILEHDR::backupSurrogate : DBFILEHDR::backupNormal,
                    !m_fBackupFull, &lgposRecT );
    }

    // Reset in backup session and cleanup FMPs for all relevant dbs...
    for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP    ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP *pfmp = &g_rgfmp[ifmp];

        if ( pfmp->FInUse()
            && pfmp->FLogOn()
            && pfmp->FAttached()
            && !pfmp->FAttachingDB()
            && pfmp->FInBackupSession() )
        {
            Assert( !pfmp->FSkippedAttach() );
            Assert( !pfmp->FDeferredAttach() );

            if ( pfmp->Ppatchhdr() )
            {
                OSMemoryPageFree( pfmp->Ppatchhdr() );
                pfmp->SetPpatchhdr( NULL );
            }

            pfmp->ResetInBackupSession();
        }
        Assert( ! pfmp->FInBackupSession() );
    }


    /*  clean up rhf entries.
     */

    for ( INT irhf = 0; irhf < crhfMax; ++irhf )
    {
        if ( m_rgrhf[irhf].fInUse )
        {
            
            Assert( !fSurrogateBackup );
            
            if ( m_rgrhf[irhf].fDatabase )
            {
                const IFMP ifmp = m_rgrhf[irhf].ifmp;
                FMP::AssertVALIDIFMP( ifmp );
                // patch file already been closed
                Assert( !g_rgfmp[ ifmp ].PfapiPatch() );
                CallS( ErrDBCloseDatabase( m_ppibBackup, ifmp, 0 ) );
            }

            // we close those files defore deleting the patch files
            Assert( NULL == m_rgrhf[irhf].pfapi );

            if ( m_rgrhf[irhf].fIsLog )
            {
                delete m_rgrhf[irhf].pLogVerifyState;
                m_rgrhf[irhf].pLogVerifyState = pNil;
            }
            m_rgrhf[irhf].fInUse = fFalse;

            OSMemoryHeapFree ( m_rgrhf[irhf].wszFileName );
            m_rgrhf[irhf].wszFileName = NULL;

        }
    }

    /*  Log error event
     */
    if ( fNormal )
    {
        CallS( error );

        if ( m_fBackupIsInternal )
        {
            // impossible today as HA does not actually complete a backup, it stops before
            // it backups up logs, so we should not be here.  IF we get here, we have a
            // problem because we're updating the backup state for full backup.  SO we'd
            // have to fix it.
            AssertSz( fFalse, "Impossible today." );
            UtilReportEvent(
                    eventInformation,
                    LOGGING_RECOVERY_CATEGORY,
                    STOP_INTERNAL_COPY_INSTANCE_ID,
                    0,
                    NULL,
                    0,
                    NULL,
                    m_pinst );
        }
        else
        {
            UtilReportEvent(
                    eventInformation,
                    LOGGING_RECOVERY_CATEGORY,
                    STOP_BACKUP_INSTANCE_ID,
                    0,
                    NULL,
                    0,
                    NULL,
                    m_pinst );

        }

        // write the db header so that the info we updated is written.
        // otherwise a crash before other db header updates
        // may result in backup information lost.

        ERR     errUpdateHdr;
        BOOL    fSkippedAttachDetach;
        LOGTIME tmEmpty;
        memset( &tmEmpty, 0, sizeof(LOGTIME) );

        m_pinst->m_plog->LockCheckpoint();
        errUpdateHdr = m_pinst->m_plog->ErrLGUpdateGenRequired( m_pinst->m_pfsapi, 0, 0, 0, tmEmpty, &fSkippedAttachDetach );
        m_pinst->m_plog->UnlockCheckpoint();

        // on error, report a Warning and continue as the backup is OK from it's point of view.
        if ( errUpdateLog < JET_errSuccess || errUpdateHdr < JET_errSuccess )
        {
            WCHAR   sz1T[32];
            const WCHAR *rgszT[1];

            OSStrCbFormatW( sz1T, sizeof(sz1T), L"%d", ( errUpdateLog < JET_errSuccess ) ? errUpdateLog : errUpdateHdr );
            rgszT[0] = sz1T;
            UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY, BACKUP_ERROR_INFO_UPDATE, 1, rgszT, 0 , NULL, m_pinst );
        }
        else
        {
            Assert( !fSkippedAttachDetach );
        }

    }
    // special messages for frequent error cases
    else if ( errBackupAbortByCaller == error )
    {
        if ( m_fBackupIsInternal )
        {
            UtilReportEvent(
                    eventError,
                    LOGGING_RECOVERY_CATEGORY,
                    STOP_INTERNAL_COPY_ERROR_ABORT_BY_CALLER_INSTANCE_ID,
                    0,
                    NULL,
                    0,
                    NULL,
                    m_pinst );
        }
        else
        {
            UtilReportEvent(
                    eventError,
                    LOGGING_RECOVERY_CATEGORY,
                    STOP_BACKUP_ERROR_ABORT_BY_CALLER_INSTANCE_ID,
                    0,
                    NULL,
                    0,
                    NULL,
                    m_pinst );
        }
    }
    else if ( JET_errBackupAbortByServer == error )
    {
        if ( m_fBackupIsInternal )
        {
            UtilReportEvent(
                    eventError,
                    LOGGING_RECOVERY_CATEGORY,
                    STOP_INTERNAL_COPY_ERROR_ABORT_BY_SERVER_INSTANCE_ID,
                    0,
                    NULL,
                    0,
                    NULL,
                    m_pinst );
        }
        else
        {
            UtilReportEvent(
                    eventError,
                    LOGGING_RECOVERY_CATEGORY,
                    STOP_BACKUP_ERROR_ABORT_BY_SERVER_INSTANCE_ID,
                    0,
                    NULL,
                    0,
                    NULL,
                    m_pinst );
        }
    }
    else
    {
        WCHAR       sz1T[32];
        const UINT  csz         = 1;
        const WCHAR *rgszT[csz];

        OSStrCbFormatW( sz1T, sizeof(sz1T), L"%d", error );
        rgszT[0] = sz1T;

        if ( m_fBackupIsInternal )
        {
            UtilReportEvent(
                    eventError,
                    LOGGING_RECOVERY_CATEGORY,
                    STOP_INTERNAL_COPY_ERROR_INSTANCE_ID,
                    csz,
                    rgszT,
                    0,
                    NULL,
                    m_pinst );
        }
        else
        {
            UtilReportEvent(
                    eventError,
                    LOGGING_RECOVERY_CATEGORY,
                    STOP_BACKUP_ERROR_INSTANCE_ID,
                    csz,
                    rgszT,
                    0,
                    NULL,
                    m_pinst );
        }
    }

#ifdef MINIMAL_FUNCTIONALITY
#else
    if( BoolParam( m_pinst, JET_paramZeroDatabaseDuringBackup ) && m_pscrubdb )
    {
        CallS( m_pscrubdb->ErrTerm() ); // may fail with JET_errOutOfMemory. what to do?
        delete m_pscrubdb;
        m_pscrubdb = NULL;
    }
#endif

    BKSetLogsTruncated( fFalse );
    m_fBackupInProgressAny = fFalse;
    m_fBackupInProgressLocal = fFalse;
    m_fBackupIsInternal = fFalse;
    m_fStopBackup = fFalse;
    m_fBackupStatus = backupStateNotStarted;

    err = JET_errSuccess;

    // Note this rolls the log, and terminates the backup LR sequence.

    if ( !m_pinst->FRecovering() )
    {
        ErrLGBackupStop( m_pinst->m_plog,
                         fSurrogateBackup ? DBFILEHDR::backupSurrogate : DBFILEHDR::backupNormal,
                         !m_fBackupFull,
                         BoolParam( m_pinst, JET_paramAggressiveLogRollover ) ? fLGCreateNewGen : 0,
                         NULL );
    }
    // we ignore the error.


#ifdef DEBUG
    if ( m_fDBGTraceBR )
    {
        CHAR sz[256];

        OSStrCbFormatA( sz, sizeof(sz), "** EndExternalBackup (%d).\n", err );
        Assert( strlen( sz ) <= sizeof( sz ) - 1 );
        DBGBRTrace( sz );
    }
#endif

    return err;
}


// build in szFindPath the patch file full name for a database
// in a certain directory. If directory is NULL, build in the
// database directory (patch file during backup)
VOID BACKUP_CONTEXT::BKIGetPatchName( __out_bcount(OSFSAPI_MAX_PATH * sizeof(WCHAR)) PWSTR wszPatch, PCWSTR wszDatabaseName, _In_ PCWSTR wszDirectory)
{
    Assert ( wszDatabaseName );

    WCHAR   wszFNameT[ IFileSystemAPI::cchPathMax ];
    WCHAR   wszDirT[ IFileSystemAPI::cchPathMax ];
    WCHAR   wszExtT[ IFileSystemAPI::cchPathMax ];

    CallS( m_pinst->m_pfsapi->ErrPathParse( wszDatabaseName, wszDirT, wszFNameT, wszExtT ) );

    //  patch file is always on the OS file-system
    if ( wszDirectory )
        // patch file in the specified directory
        // (m_wszRestorePath during restore)
    {
        LGMakeName( m_pinst->m_pfsapi, wszPatch, wszDirectory, wszFNameT, wszPatExt );
    }
    else
        // patch file in the same directory with the database
    {
        LGMakeName( m_pinst->m_pfsapi, wszPatch, wszDirT, wszFNameT, wszPatExt );
    }
}


ERR ISAMAPI  ErrIsamSnapshotStart(  JET_INSTANCE        instance,
                                    JET_PCSTR           szDatabases,
                                    JET_GRBIT           grbit)
{
    // we never supported this non-VSS snapshot in a shipped product
    //
    return ErrERRCheck( JET_wrnNyi );
}

ERR ISAMAPI  ErrIsamSnapshotStop(   JET_INSTANCE        instance,
                                    JET_GRBIT           grbit)
{
    // we never supported this non-VSS snapshot in a shipped product
    //
    return ErrERRCheck( JET_wrnNyi );
}

BEGIN_PRAGMA_OPTIMIZE_DISABLE("", 6189075, "JetBackupInstance will create a corrupted database with the incorrectly optimized code");

VOID BACKUP_CONTEXT::BKIMakeDbTrailer(const IFMP ifmp, BYTE *pvPage)
{
    FMP     *pfmp = &g_rgfmp[ifmp];

    PATCHHDR * const ppatchHdr = pfmp->Ppatchhdr();
    BKINFO * pbkinfo;

    Assert( pfmp->PgnoBackupCopyMost() == pfmp->PgnoBackupMost() );
    Assert( !pfmp->FCopiedPatchHeader() );

    memset( (void *)ppatchHdr, '\0', g_cbPage );

    // Capture these before the backup marks so we avoid advancing them too much and capturing a max.
    // required which is ahead of the max. committed in busy clients.
    const BOOL fLowerMinReqLogGenOnRedo = ( pfmp->ErrDBFormatFeatureEnabled( JET_efvLowerMinReqLogGenOnRedo ) == JET_errSuccess );
    if ( fLowerMinReqLogGenOnRedo )
    {
        PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr();
        ppatchHdr->lgenMinReq = pdbfilehdr->le_lGenMinRequired;
        ppatchHdr->lgenMaxReq = pdbfilehdr->le_lGenMaxRequired;
        ppatchHdr->lgenMaxCommitted = pdbfilehdr->le_lGenMaxCommitted;
        ppatchHdr->logtimeGenMaxRequired = pdbfilehdr->logtimeGenMaxRequired;
        ppatchHdr->logtimeGenMaxCommitted = pdbfilehdr->logtimeGenMaxCreate;

        Assert( ppatchHdr->lgenMinReq != 0 );
        Assert( ppatchHdr->lgenMaxReq != 0 );
        Assert( ppatchHdr->lgenMaxCommitted != 0 );

        Assert( !!ppatchHdr->logtimeGenMaxRequired.FIsSet() == !!ppatchHdr->logtimeGenMaxCommitted.FIsSet() );
        Assert( ppatchHdr->lgenMinReq <= ppatchHdr->lgenMaxReq );
        Assert( ppatchHdr->lgenMaxReq <= ppatchHdr->lgenMaxCommitted );
        Assert( LOGTIME::CmpLogTime( ppatchHdr->logtimeGenMaxRequired, ppatchHdr->logtimeGenMaxCommitted ) <= 0 );
    }

    Assert( (LONG)ppatchHdr->lgenMinReq >= 0 );
    Assert( (LONG)ppatchHdr->lgenMaxReq >= 0 );
    Assert( (LONG)ppatchHdr->lgenMaxCommitted >= 0 );

    pbkinfo = &ppatchHdr->bkinfo;
    pbkinfo->le_lgposMark = m_lgposFullBackupMark;
    pbkinfo->logtimeMark = m_logtimeFullBackupMark;
    pbkinfo->le_genLow = m_lgenCopyMic;
    Assert( (LONG)pbkinfo->le_genLow > 0 );

    UtilMemCpy( (BYTE*)&ppatchHdr->signDb, &pfmp->Pdbfilehdr()->signDb, sizeof(SIGNATURE) );
    UtilMemCpy( (BYTE*)&ppatchHdr->signLog, &pfmp->Pdbfilehdr()->signLog, sizeof(SIGNATURE) );

    pbkinfo->le_genHigh = m_pinst->m_plog->LGGetCurrentFileGenWithLock();
    Assert( (LONG)pbkinfo->le_genHigh > 0 );

    // Unfortunately, there is a race condition here: the DB header may get updated
    // before the log stream object gets updated with a recently-rolled-over log file. Therefore,
    // take the max of both as the lgenHigh.
    if ( ( ppatchHdr->lgenMaxCommitted != 0 ) && ( ppatchHdr->lgenMaxCommitted > pbkinfo->le_genHigh ) )
    {
        Assert( ppatchHdr->lgenMaxCommitted <= ( pbkinfo->le_genHigh + 1 ) );
        pbkinfo->le_genHigh = ppatchHdr->lgenMaxCommitted;
    }

#ifdef DEBUG
    Assert( (LONG)pbkinfo->le_genLow > 0 );
    Assert( (LONG)pbkinfo->le_genHigh > 0 );
    Assert( pbkinfo->le_genHigh >= pbkinfo->le_genLow );
    if ( fLowerMinReqLogGenOnRedo )
    {
        Assert( pbkinfo->le_genLow <= ppatchHdr->lgenMinReq );
        Assert( pbkinfo->le_genHigh >= ppatchHdr->lgenMaxReq );
    }
#endif // DEBUG

    SetPageChecksum( ppatchHdr, g_cbPage, databaseHeader, 0 );

    UtilMemCpy( (BYTE *)pvPage, ppatchHdr, g_cbPage );

    pfmp->SetFCopiedPatchHeader();
}

END_PRAGMA_OPTIMIZE()

ERR BACKUP_CONTEXT::ErrBKBackupPibInit()
{
    ERR err = JET_errSuccess;
    Assert( m_ppibBackup == ppibNil );

    if ( !m_pinst->FRecovering() || BoolParam( m_pinst, JET_paramFlight_EnableBackupDuringRecovery ) )
    {
        Call( ErrPIBBeginSession( m_pinst, &m_ppibBackup, procidNil, fFalse ) );
        OPERATION_CONTEXT opContext = { OCUSER_BACKUP, 0, 0, 0, 0 };
        Call( m_ppibBackup->ErrSetOperationContext( &opContext, sizeof( opContext ) ) );
    }

HandleError:
    return JET_errSuccess;
}

VOID BACKUP_CONTEXT::BKBackupPibTerm()
{
    if ( m_ppibBackup != ppibNil )
    {
        PIBEndSession( m_ppibBackup );
        m_ppibBackup = ppibNil;
    }
}


VOID BACKUP_CONTEXT::BKCopyLastBackupStateFromCheckpoint( const CHECKPOINT_FIXED * pcheckpoint )
{
    m_logtimeFullBackup = pcheckpoint->logtimeFullBackup;
    m_lgposFullBackup   = pcheckpoint->le_lgposFullBackup;
    m_logtimeIncBackup  = pcheckpoint->logtimeIncBackup;
    m_lgposIncBackup    = pcheckpoint->le_lgposIncBackup;
}

VOID BACKUP_CONTEXT::BKCopyLastBackupStateToCheckpoint( CHECKPOINT_FIXED * pcheckpoint ) const
{
    if ( m_lgposFullBackup.lGeneration )
    {
        //  full backup in progress
        pcheckpoint->le_lgposFullBackup = m_lgposFullBackup;
        pcheckpoint->logtimeFullBackup = m_logtimeFullBackup;
    }
    if ( m_lgposIncBackup.lGeneration )
    {
        //  incremental backup in progress
        pcheckpoint->le_lgposIncBackup = m_lgposIncBackup;
        pcheckpoint->logtimeIncBackup = m_logtimeIncBackup;
    }
}

VOID BACKUP_CONTEXT::BKInitForSnapshot( BOOL fIncrementalSnapshot, LONG lgenCheckpoint )
{
    m_lgenDeleteMic = 0;
    // For incremental backup m_lgenCopyMic is set ErrOSSnapshotStopLogging().
    if ( !fIncrementalSnapshot )
    {
        m_lgenCopyMic = lgenCheckpoint;
    }
    Assert( 1 <= m_lgenCopyMic );
    m_lgenDeleteMac = min( m_lgenCopyMac, lgenCheckpoint );
}

