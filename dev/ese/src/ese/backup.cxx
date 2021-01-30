// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include <stdarg.h>

#define wszTemp L"temp"


extern VOID ITDBGSetConstants( INST * pinst = NULL);

BACKUP_CONTEXT::BACKUP_CONTEXT( INST * pinst )
    : CZeroInit( sizeof( BACKUP_CONTEXT ) ),
      m_pinst( pinst ),
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


LOCAL ERR ErrLGDeleteAllFiles( IFileSystemAPI *const pfsapi, __inout_bcount(cbDir) const PWSTR wszDir, ULONG cbDir )
{
    ERR             err     = JET_errSuccess;
    IFileFindAPI*   pffapi  = NULL;

    Assert( LOSStrLengthW( wszDir ) + 1 + 1 < IFileSystemAPI::cchPathMax );

    WCHAR wszPath[ IFileSystemAPI::cchPathMax ];
    Call( ErrOSStrCbCopyW( wszPath, sizeof( wszPath ), wszDir ) );
    Call( pfsapi->ErrPathFolderNorm( wszPath, sizeof( wszPath ) ) );
    Call( ErrOSStrCbAppendW( wszPath, sizeof( wszPath ), L"*" ) );

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

        
        if (    wcscmp( wszFileNameT, L"." ) &&
                wcscmp( wszFileNameT, L".." ) &&
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
    
    Assert( wszDir[LOSStrLengthW(wszDir)] != L'*' );

    delete pffapi;

    return err;
}



ERR ErrLGCheckDir( IFileSystemAPI *const pfsapi, __inout_bcount(cbDir) const PWSTR wszDir, ULONG cbDir, __in_opt PCWSTR wszSearch )
{
    ERR             err     = JET_errSuccess;
    IFileFindAPI*   pffapi  = NULL;

    Assert( LOSStrLengthW( wszDir ) + 1 + 1 < IFileSystemAPI::cchPathMax );

    WCHAR wszPath[ IFileSystemAPI::cchPathMax ];
    Call( ErrOSStrCbCopyW( wszPath, sizeof( wszPath ), wszDir ) );
    Call( pfsapi->ErrPathFolderNorm( wszPath, sizeof( wszPath ) ) );
    Call( ErrOSStrCbAppendW( wszPath, sizeof( wszPath ), L"*" ) );

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

        
        if (    wcscmp( wszFileNameT, L"." ) &&
                wcscmp( wszFileNameT, L".." ) &&
                ( !wszSearch || !UtilCmpFileName( wszFileNameT, wszSearch ) ) )
        {
            Call( ErrERRCheck( JET_errBackupDirectoryNotEmpty ) );
        }
    }
    Call( err == JET_errFileNotFound ? JET_errSuccess : err );

    err = JET_errSuccess;

HandleError:
    
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

    BOOL fRoomForFinalHeaderPage = fFalse;

#ifdef MINIMAL_FUNCTIONALITY
#else

    
    CONST BOOL fScrub = BoolParam( m_pinst, JET_paramZeroDatabaseDuringBackup );

    if ( fScrub && m_pinst->FRecovering() )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    
    if ( fScrub )
    {
        
        if ( g_rgfmp[ ifmp ].PgnoBackupCopyMost() == 0 )
        {
            
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

#endif

    Assert( cpage > 0 );

    

    
    Assert( pfmp->PgnoBackupMost() >= pfmp->PgnoBackupCopyMost() );
    cpageT = min( cpage, (INT)( pfmp->PgnoBackupMost() - pfmp->PgnoBackupCopyMost() + ( 0 == pfmp->PgnoBackupCopyMost() ? cpgDBReserved : 0 ) ) );
    *pcbActual = 0;
    ipageT = 0;

    fRoomForFinalHeaderPage =  ( cpageT < cpage );

    
    if ( cpageT == 0
        && ( !fRoomForFinalHeaderPage || pfmp->FCopiedPatchHeader() ) )
    {
        Assert( pfmp->PgnoBackupMost() == pfmp->PgnoBackupCopyMost() );
        return JET_errSuccess;
    }

    if ( pfmp->PgnoBackupCopyMost() == 0 )
    {
        Assert( cpage > cpgDBReserved );

        Assert( prhf->ib == 0 );
        UtilMemCpy( pvPageMin, prhf->pdbfilehdr, g_cbPage );
        UtilMemCpy( (BYTE*)pvPageMin + g_cbPage, prhf->pdbfilehdr, g_cbPage );

        
        *pcbActual += g_cbPage * 2;
        ipageT += 2;
        Assert( 2 == cpgDBReserved );
        Assert( cpage >= ipageT );
    }

    const PGNO  pgnoStart   = pfmp->PgnoBackupCopyMost() + 1;
    const PGNO  pgnoEnd     = pfmp->PgnoBackupCopyMost() + ( cpageT - ipageT );

    if ( pgnoStart <= pgnoEnd  )
    {
        CallR( pfmp->ErrRangeLock( pgnoStart, pgnoEnd ) );

        pfmp->SetPgnoBackupCopyMost( pgnoEnd );
    }
    else
    {
        Assert( fRoomForFinalHeaderPage );
    }


    const TICK  tickStart   = TickOSTimeCurrent();
    const TICK  tickBackoff = 100;
    const INT   cRetry      = 16;

    INT         iRetry      = 0;

    if ( pgnoStart > pgnoEnd )
    {
        Assert( fRoomForFinalHeaderPage );
        goto CopyFinalHeaderPage;
    }

    do
    {
        TraceContextScope tcBackupT( iorpBackup );

        err = ErrIOReadDbPages( ifmp, pfmp->Pfapi(), (BYTE *) pvPageMin + ipageT * g_cbPage, pgnoStart, pgnoEnd, fTrue, 0, *tcBackupT, !!( UlParam( m_pinst, JET_paramDisableVerifications ) & DISABLE_EXTENSIVE_CHECKS_DURING_STREAMING_BACKUP ) );

        if ( err < JET_errSuccess )
        {
            if ( iRetry < cRetry )
            {
                BOOL fIsPatchable = BoolParam( m_pinst, JET_paramEnableExternalAutoHealing ) && PagePatching::FIsPatchableError( err );
                if ( fIsPatchable )
                {
                    pfmp->RangeUnlock( pgnoStart, pgnoEnd );
                }

                UtilSleep( ( iRetry + 1 ) * tickBackoff );

                if ( fIsPatchable )
                {
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

    
    if ( fScrub && err >= JET_errSuccess )
    {
        
        PGNO pgnoScrubStart = 0xFFFFFFFF;
        PGNO pgnoScrubEnd   = 0x00000000;
        PIBTraceContextScope tcRef = m_ppibBackup->InitTraceContextScope();
        tcRef->nParentObjectClass = tceNone;
        tcRef->iorReason.SetIort( iortScrubbing );

        for ( PGNO pgno = pgnoStart; pgno <= pgnoEnd; pgno++ )
        {
            
            void* pv = (BYTE *)pvPageMin + ( pgno - pgnoStart + ipageT ) * g_cbPage;

            
            Assert( pfmp->CbPage() == g_cbPage );
            CPAGE cpageLoad;
            cpageLoad.LoadPage( pv, pfmp->CbPage() );
            const BOOL fPageIsInitialized = cpageLoad.FPageIsInitialized();
            cpageLoad.UnloadPage();

            
            if ( fPageIsInitialized )
            {
                
                BFLatch bfl;
                if ( ErrBFWriteLatchPage( &bfl, ifmp, pgno, BFLatchFlags( bflfNoCached | bflfNew ), m_ppibBackup->BfpriPriority( ifmp ), *tcRef ) >= JET_errSuccess )
                {
                    UtilMemCpy( bfl.pv, pv, g_cbPage );
                    BFWriteUnlatch( &bfl );
                }

                
                pgnoScrubStart  = min( pgnoScrubStart, pgno );
                pgnoScrubEnd    = max( pgnoScrubEnd, pgno );
            }

            
            if ( !fPageIsInitialized || pgno == pgnoEnd )
            {
                
                if ( pgnoScrubStart <= pgnoScrubEnd )
                {
                    
                    err = m_pscrubdb->ErrScrubPages( pgnoScrubStart, pgnoScrubEnd - pgnoScrubStart + 1 );

                    
                    pgnoScrubStart  = 0xFFFFFFFF;
                    pgnoScrubEnd    = 0x00000000;
                }
            }
        }

        
        Assert( pgnoScrubStart == 0xFFFFFFFF );
        Assert( pgnoScrubEnd == 0x00000000 );
    }

#endif

    if ( err < JET_errSuccess )
    {
        Assert ( pgnoStart > 0 );
        pfmp->SetPgnoBackupCopyMost( pgnoStart - 1 );
    }

    
    pfmp->RangeUnlock( pgnoStart, pgnoEnd );

    Call( err );

    
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
            Assert( pfmp->CbPage() == g_cbPage );
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

    
    if ( fScrub
        && ( NULL != m_pscrubdb )
        && ( g_rgfmp[ ifmp ].PgnoBackupCopyMost() == g_rgfmp[ ifmp ].PgnoBackupMost() ) )
    {
        
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

#endif

    return err;
}



ERR BACKUP_CONTEXT::ErrBKIPrepareLogFiles(
    JET_GRBIT                   grbit,
    __in PCWSTR                 wszLogFilePath,
    __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) PWSTR   wszPathJetChkLog,
    __in PCWSTR                  wszBackupPath )
{
    ERR         err;
    CHECKPOINT  *pcheckpointT = NULL;
    LGPOS       lgposRecT;
    LOG *       plog = m_pinst->m_plog;

    const BOOL  fFullBackup = ( 0 == grbit );
    const BOOL  fIncrementalBackup = ( JET_bitBackupIncremental == grbit );

    Assert ( 0 == grbit || JET_bitBackupIncremental == grbit );

    Assert( m_fBackupInProgressLocal );

    Enforce( !m_pinst->FRecovering() );

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

    
    Assert( m_lgenCopyMac == 0 );
    m_lgenCopyMac = plog->LGGetCurrentFileGenWithLock();
    Assert( m_lgenCopyMac != 0 );
    m_cGenCopyDone = 0;

    
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

    
    AllocR( pcheckpointT = (CHECKPOINT *) PvOSMemoryPageAlloc( sizeof(CHECKPOINT), NULL ) );

    plog->LGFullNameCheckpoint( wszPathJetChkLog );
    Call( plog->ErrLGReadCheckpoint( wszPathJetChkLog, pcheckpointT, fTrue ) );
    Assert( m_lgenDeleteMac == 0 );
    m_lgenDeleteMac = pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration;

    m_lgenDeleteMac = min( m_lgenDeleteMac, m_lgenCopyMac);
    Assert( m_lgenDeleteMic <= m_lgenDeleteMac );

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

                m_lgenDeleteMac = min( (ULONG)m_lgenDeleteMac, genLastFullBackupForDb );
            }
        }
        m_lgenDeleteMic = min( m_lgenDeleteMac, m_lgenDeleteMic );
    }


    
    if ( fFullBackup )
    {
        
        Assert( m_lgenCopyMic != 0 );
    }
    else
    {
        Assert ( fIncrementalBackup );
        
        Assert( m_lgenDeleteMic != 0 );
        m_lgenCopyMic = m_lgenDeleteMic;
    }

    Assert ( m_lgenDeleteMic <= m_lgenDeleteMac );
    Assert ( m_lgenCopyMic <= m_lgenCopyMac );

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

    OSMemoryPageFree( pcheckpointT );
    return err;
}


ERR ErrLGCheckIncrementalBackup( INST *pinst, DBFILEHDR::BKINFOTYPE backupType )
{
    DBID dbid;
    const BKINFO *pbkinfo;

    backupType = ( backupType == DBFILEHDR::backupSurrogate ) ? DBFILEHDR::backupOSSnapshot : backupType;

    for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP *pfmp = &g_rgfmp[ifmp];

        
        if ( pfmp->FAttached() && !pfmp->FAttachedForRecovery() && !pfmp->FAttachingDB() )
        {
            Assert( pfmp->Pdbfilehdr() );
            Assert( !pfmp->FSkippedAttach() );
            Assert( !pfmp->FDeferredAttach() );
            pbkinfo = &pfmp->Pdbfilehdr()->bkinfoFullPrev;
            if ( pbkinfo->le_genLow == 0 ||
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
    __in PWSTR wszBackupPath,
    const WCHAR *wszTempDirIn )
{
    ERR err;

    
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

    Assert( LOSStrLengthW( wszBackup ) < IFileSystemAPI::cchPathMax - 1 );
    OSStrCbCopyW( wszBackupPath, IFileSystemAPI::cchPathMax * sizeof( WCHAR ), wszBackup );
    if ( LOSStrLengthW( wszBackup ) >= IFileSystemAPI::cchPathMax - 1 )
    {
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
        qwEseFileID = qwPatchFileID;
    }
    
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
    __in PCWSTR wszBackup,
    __out_bcount(cbBackupPath) PWSTR wszBackupPath,
    const size_t cbBackupPath,
    JET_GRBIT grbit )
{
    ERR             err             = JET_errSuccess;
    const BOOL      fFullBackup     = !( grbit & JET_bitBackupIncremental );
    const BOOL      fBackupAtomic   = ( grbit & JET_bitBackupAtomic );

    WCHAR           wszT[IFileSystemAPI::cchPathMax];
    WCHAR           wszFrom[IFileSystemAPI::cchPathMax];

    
    Assert( LOSStrLengthW( wszBackup ) < IFileSystemAPI::cchPathMax - 1 );
    if ( ErrOSStrCbCopyW( wszBackupPath, cbBackupPath, wszBackup ) < JET_errSuccess )
    {
        return ErrERRCheck( JET_errInvalidPath );
    }
    if ( LOSStrLengthW( wszBackup ) < IFileSystemAPI::cchPathMax - 1 )
    {
        CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszBackupPath, cbBackupPath ) );
    }

    if ( BoolParam( m_pinst, JET_paramCreatePathIfNotExist ) )
    {
        Call( ErrUtilCreatePathIfNotExist( m_pinst->m_pfsapi, wszBackupPath, NULL, 0 ) );
    }

    
    Call( ErrLGIRemoveTempDir( m_pinst->m_pfsapi, wszT, sizeof(wszT), wszBackupPath, wszTempDir ) );

    if ( fBackupAtomic )
    {
        
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

        
        if ( !fFullBackup )
        {
            
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

            
            OSStrCbAppendW( wszBackupPath, cbBackupPath, wszTempDir );
        }
    }
    else
    {
        if ( !fFullBackup )
        {
            
            Call( ErrLGCheckDir( m_pinst->m_pfsapi, wszBackupPath, cbBackupPath, wszAtomicNew ) );
            Call( ErrLGCheckDir( m_pinst->m_pfsapi, wszBackupPath, cbBackupPath, wszAtomicOld ) );
        }
        else
        {
            
            Call( ErrLGCheckDir( m_pinst->m_pfsapi, wszBackupPath, cbBackupPath, NULL ) );
        }
    }

    HandleError:
        return err;
    }


ERR BACKUP_CONTEXT::ErrBKIPromoteDirectory(
    __in PCWSTR wszBackup,
    __out_bcount(OSFSAPI_MAX_PATH * sizeof(WCHAR)) PWSTR wszBackupPath,
    JET_GRBIT grbit )
{
    ERR             err             = JET_errSuccess;
    const BOOL      fFullBackup     = !( grbit & JET_bitBackupIncremental );
    const BOOL      fBackupAtomic   = ( grbit & JET_bitBackupAtomic );


    if ( !fBackupAtomic || !fFullBackup )
        return JET_errSuccess;

    WCHAR           wszFrom[IFileSystemAPI::cchPathMax];
    WCHAR           wszT[IFileSystemAPI::cchPathMax];

    Assert( LOSStrLengthW( wszBackupPath ) < IFileSystemAPI::cchPathMax );
    OSStrCbCopyW( wszFrom, sizeof(wszFrom) ,wszBackupPath );
    wszFrom[ IFileSystemAPI::cchPathMax - 1 ] = 0;

    

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
    __in PCWSTR wszBackup,
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

    
    if ( wszBackup == NULL || wszBackup[0] == L'\0' )
    {
        m_lgenDeleteMac = plog->LgposGetCheckpoint().le_lGeneration;

        m_lgenDeleteMic = 0;

        if ( !BoolParam( m_pinst, JET_paramCircularLog ) )
        {
            (void)plog->ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), &m_lgenDeleteMic, NULL );
        }

        if ( 0 == m_lgenDeleteMic )
        {
            m_lgenDeleteMic = m_lgenDeleteMac;
        }

        goto DeleteLogs;
    }

    if ( LOSStrLengthW( wszBackup ) >= IFileSystemAPI::cchPathMax - 1 )
    {
        Call( ErrERRCheck( JET_errInvalidPath ) );
    }

    Call ( ErrBKIPrepareDirectory( wszBackup, wszBackupPath, sizeof( wszBackupPath ), grbit ) );
    if ( !fFullBackup )
    {
        goto CopyLogFiles;
    }

    
    Assert( fFullBackup );

    
    fShowStatus = ( pfnStatus != NULL );
    if ( fShowStatus )
    {
        Assert( 0 == snprog.cunitDone );
        snprog.cbStruct = sizeof(JET_SNPROG);
        snprog.cunitTotal = 100;

        
        (*pfnStatus)( JET_snpBackup, JET_sntBegin, &snprog, pvStatusContext );
    }
    Call ( ErrIsamGetInstanceInfo( &cInstanceInfo, &aInstanceInfo, NULL ) );

    {
    pInstanceInfo = NULL;
    for ( ULONG iInstanceInfo = 0; iInstanceInfo < cInstanceInfo && !pInstanceInfo; iInstanceInfo++)
    {
        if ( aInstanceInfo[iInstanceInfo].hInstanceId == (JET_INSTANCE)m_pinst )
        {
            pInstanceInfo = aInstanceInfo + iInstanceInfo;
        }
    }
    AssertRTL ( pInstanceInfo );

    for ( ULONG_PTR iDatabase = 0; iDatabase < pInstanceInfo->cDatabases; iDatabase++ )
    {
        Assert ( pInstanceInfo->szDatabaseFileName );
        Assert ( pInstanceInfo->szDatabaseFileName[iDatabase] );
        Call ( ErrBKICopyFile( pInstanceInfo->szDatabaseFileName[iDatabase], wszBackupPath, pfnStatus, pvStatusContext ) );
    }
    }
    

CopyLogFiles:
    {
    ULONG           cbNames;

    Call ( ErrBKGetLogInfo( NULL, 0, &cbNames, NULL, fFullBackup ) );
    Alloc( wszNames = (WCHAR *)PvOSMemoryPageAlloc( cbNames, NULL ) );

    Call ( ErrBKGetLogInfo( wszNames, cbNames, NULL, NULL, fFullBackup ) );
    Assert ( wszNames );
    }

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
VOID DBGBRTrace( __in PCSTR sz )
{
    DBGprintf( "%s", sz );
}
#endif




ERR ISAMAPI ErrIsamBeginSurrogateBackup(
    __in        JET_INSTANCE    jinst,
    __in        ULONG       lgenFirst,
    __in        ULONG       lgenLast,
    __in        JET_GRBIT       grbit )
{
    INST *  pinst = (INST *)jinst;
    ERR     err;
    ULONG   lgenTip;


    if ( lgenFirst > lgenLast )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    lgenTip = pinst->m_plog->LGGetCurrentFileGenWithLock();

    if ( lgenTip < lgenLast )
    {
        AssertSz( fFalse, "Uh-oh!  The replica is ahead of us!" );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    CallR( pinst->m_pbackup->ErrBKBeginExternalBackup( JET_bitBackupSurrogate | grbit, lgenFirst, lgenLast ) );

    return( err );

}

ERR ISAMAPI ErrIsamEndSurrogateBackup(
    __inout JET_INSTANCE    jinst,
    __in        JET_GRBIT       grbit )
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
        pinst->m_pbackup->BKSetBackupStatus( BACKUP_CONTEXT::backupStateDone );
        err = JET_errSuccess;
    }

    err = pinst->m_pbackup->ErrBKExternalBackupCleanUp( err, JET_bitBackupSurrogate | grbit );
    return( err );
}





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

    if ( m_pinst->FRecovering() )
    {
        if ( !fInternalCopyBackup || !BoolParam( m_pinst, JET_paramFlight_EnableBackupDuringRecovery ) )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
    }

    
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
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    CallR( m_pinst->m_plog->ErrLGCheckState() );

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

#if DEBUG
    if ( fSurrogateBackup )
    {
        Assert( !BoolParam( m_pinst, JET_paramCircularLog ) );
        Assert( m_pinst->CNonLoggedIndexCreators() == 0 );
    }
#endif
    while ( m_pinst->CNonLoggedIndexCreators() > 0 )
    {
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


    m_pinst->WaitForDBAttachDetach();
 
    for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

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


    m_fBackupBeginNewLogFile = fFalse;


    
    if ( fIncrementalBackup )
    {
#ifdef DEBUG
        if ( m_fDBGTraceBR )
            DBGBRTrace( "Incremental Backup.\n" );
#endif
        UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY,
                START_INCREMENTAL_BACKUP_INSTANCE_ID, 0, NULL, 0, NULL, m_pinst );
        m_fBackupFull = fFalse;

        
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
            Call( errLGBackupBegin );
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

    
    if ( !m_fBackupFull )
    {
        return ErrERRCheck( JET_errInvalidBackupSequence );
    }

#ifdef DEBUG
    if ( m_fDBGTraceBR )
        DBGBRTrace( "** Begin GetAttachInfo.\n" );
#endif

    
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
    cbActual += sizeof(WCHAR);

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

    
    if ( pcbActual != NULL )
    {
        *pcbActual = cbActual;
    }

    
    if ( wszzDatabases != NULL )
        UtilMemCpy( wszzDatabases, pwch, min( cbMax, cbActual ) );

HandleError:
    
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
    PGNO    pgnoRead = 0;
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
        if ( !m_fBackupIsInternal || !BoolParam( m_pinst, JET_paramFlight_EnableBackupDuringRecovery ) )
        {
            return ErrERRCheck( JET_errBackupNotAllowedYet );
        }

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
        
        if ( pgnoRead < 2 )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
    }

    
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


    err = ErrDBOpenDatabase( m_ppibBackup, wszFileName, &ifmpT, 0 );
    Expected( err < JET_errSuccess || err == JET_errSuccess || err == JET_wrnFileOpenReadOnly );
    const IFMP ifmp = ( err >= JET_errSuccess ) ? ifmpT : ifmpNil;

    if ( err >= JET_errSuccess )
    {
        PATCH_HEADER_PAGE *ppatchhdr;

        fDbOpen = fTrue;

        
        if ( !m_fBackupFull )
        {
            Assert ( backupStateLogsAndPatchs == m_fBackupStatus );
            Error( ErrERRCheck( JET_errInvalidBackupSequence ) );
        }

        
        if ( backupStateDatabases != m_fBackupStatus )
        {
            Assert ( m_fBackupBeginNewLogFile );
            Error( ErrERRCheck( JET_errInvalidBackupSequence ) );
        }

        FMP* const pfmpT = &g_rgfmp[ifmp];

        
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

        
        Assert( pfmpT->FLogOn() );

        if ( fIncludePatch )
        {
            
            IFileAPI *  pfapiPatch      = NULL;
            WCHAR       wszPatch[IFileSystemAPI::cchPathMax];

            

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


        const PGNO pgnoWriteLatchedMax = pfmpT->PgnoHighestWriteLatched();
        const PGNO pgnoDirtiedMax = pfmpT->PgnoDirtiedMax();
        const PGNO pgnoWriteLatchedNonScanMax = pfmpT->PgnoWriteLatchedNonScanMax();
        const PGNO pgnoLatchedScanMax = pfmpT->PgnoLatchedScanMax();
        const PGNO pgnoPreReadScanMax = pfmpT->PgnoPreReadScanMax();
        const PGNO pgnoScanMax = pfmpT->PgnoScanMax();
        const PGNO pgnoLast = pfmpT->PgnoLast();

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
            pfmpT->SetPgnoBackupCopyMost( pgnoRead - 1 );
            Assert( pfmpT->PgnoBackupMost() >= pfmpT->PgnoBackupCopyMost() );
        }


        m_rgrhf[irhf].cb += g_cbPage * cpgDBReserved;

        m_rgrhf[irhf].cb += g_cbPage;

        pfmpT->CritLatch().Leave();


        if ( !pfmpT->Ppatchhdr() )
        {
            Alloc( ppatchhdr = (PATCH_HEADER_PAGE*)PvOSMemoryPageAlloc( g_cbPage, NULL ) );
            pfmpT->SetPpatchhdr( ppatchhdr );
        }

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

            Call( err );

            Call( ErrERRCheck( JET_wrnNyi ) );
        }
        else
        {
            Assert ( backupStateLogsAndPatchs == m_fBackupStatus );



            Assert( !m_rgrhf[irhf].fDatabase );


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
                qwEseFileID = qwPatchFileID;
            }


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
        if ( !m_fBackupIsInternal || !BoolParam( m_pinst, JET_paramFlight_EnableBackupDuringRecovery ) )
        {
            return ErrERRCheck( JET_errBackupNotAllowedYet );
        }

        if ( m_fCtxIsSuspended )
        {
            return ErrERRCheck( JET_errBackupNotAllowedYet );
        }

        fBackupDuringRecovery = fTrue;
    }

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
        }

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

        Assert ( pfmpT->FInBackupSession() );

        if ( cpage > 0 )
        {
            
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

            if ( fBackupDuringRecovery && m_fCtxIsSuspended )
            {
                return ErrERRCheck( JET_errBackupNotAllowedYet );
            }

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
        if( m_pscrubdb )
        {
            CallSRFS( m_pscrubdb->ErrTerm(), ( JET_errDiskIO, 0 ) );
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

    {
        WCHAR wszSizeRead[32];
        WCHAR wszSizeAll[32];

        Assert ( m_rgrhf[irhf].wszFileName );

        const WCHAR * rgszT[] = { m_rgrhf[irhf].wszFileName, wszSizeRead, wszSizeAll };

        if ( m_rgrhf[irhf].ib == m_rgrhf[irhf].cb )
        {
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

    
    for ( irhf = 0; irhf < m_crhfMac; irhf++ )
    {
        if ( m_rgrhf[irhf].fInUse )
        {
            return ErrERRCheck( JET_errInvalidBackupSequence );
        }
    }

    Assert ( backupStateDatabases == m_fBackupStatus || backupStateLogsAndPatchs == m_fBackupStatus );


    
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
    __in ULONG                  cbMax,
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

                

                Assert( pfmp->FLogOn() );
                BKIGetPatchName( wszT, pfmp->WszDatabaseName() );
                cbActual += (ULONG) (sizeof(WCHAR) * ( LOSStrLengthW( wszT ) + 1 ) );
            }
        }
    }
    cbActual += sizeof(WCHAR);

    Alloc( pch = static_cast<WCHAR *>( PvOSMemoryHeapAlloc( cbActual ) ) );

    
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

    if ( fIncludePatch )
    {
        
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

    
    if ( wszzLogs != NULL )
    {
        UtilMemCpy( wszzLogs, pch, min( cbMax, cbActual ) );

        
        if ( pcbActual != NULL )
        {
            *pcbActual = min( cbMax, cbActual );
        }
    }
    else
    {
        
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
        WCHAR *pwchT;

        if ( err >= 0 )
        {
            OSStrCbFormatA( sz, sizeof(sz), "   Log Info with cbActual = %d and cbMax = %d :\n", cbActual, cbMax );
            Assert( strlen( sz ) <= sizeof( sz ) - 1 );
            DBGBRTrace( sz );

            if ( wszzLogs != NULL )
            {
                pwchT = wszzLogs;

                do {
                    if ( wcslen( pwchT ) != 0 )
                    {
                        CAutoSZ lszTemp;
                        if ( lszTemp.ErrSet( pwchT ) )
                        {
                            OSStrCbFormatA( sz, sizeof(sz), "     <errInToAsciiConversion>\n" );
                        }
                        else
                        {
                            OSStrCbFormatA( sz, sizeof(sz), "     %s\n", (CHAR*)lszTemp );
                        }
                        Assert( strlen( sz ) <= sizeof( sz ) - 1 );
                        DBGBRTrace( sz );
                        pwchT += wcslen( pwchT );
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
    __in ULONG                  cbMax,
    __out ULONG                 *pcbActual,
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
    __in ULONG                  cbMax,
    __out ULONG                 *pcbActual )
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
        return ErrERRCheck( JET_errInvalidBackupSequence );
    }

    Assert( backupStateDatabases == m_fBackupStatus
        || backupStateLogsAndPatchs == m_fBackupStatus );
    m_fBackupStatus = backupStateLogsAndPatchs;

    Assert( m_lgenCopyMic );
    Assert( m_lgenCopyMac );
    Assert( m_lgenCopyMic <= (m_lgenCopyMac - 1) );
    const SIZE_T        cGenCopyMust = ( ( (SIZE_T)m_lgenCopyMic + (SIZE_T)(m_lgenCopyMac - 1) ) * ( (SIZE_T)(m_lgenCopyMac - 1) - (SIZE_T)m_lgenCopyMic + 1  ) ) / 2;

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
        Call ( m_pinst->m_plog->ErrLGTruncateLog( 0, 0, fTrue, m_fBackupFull ) );
    }

HandleError:


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


    BOOL    fSkippedAttachDetach;
    LOGTIME tmEmpty;
    memset( &tmEmpty, 0, sizeof(LOGTIME) );

    m_pinst->m_plog->LockCheckpoint();
    errUpdateHdr = m_pinst->m_plog->ErrLGUpdateGenRequired( m_pinst->m_pfsapi, 0, 0, 0, tmEmpty, &fSkippedAttachDetach );
    m_pinst->m_plog->UnlockCheckpoint();

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
    }

    if ( !fIncremental )
    {
        LGIGetDateTime( &m_logtimeFullBackup );

        Call ( ErrLGBackupPrepLogs(
                    m_pinst->m_plog,
                    DBFILEHDR::backupOSSnapshot, fIncremental,
                    &m_logtimeFullBackup,
                    grbitLogRec,
                    &m_lgposFullBackup ) );

        if ( fCreateAndStopOnNewGen )
        {
            m_pinst->m_plog->LGWaitLogPaused();
            Assert( m_pinst->m_plog->FLGLogPaused() || m_pinst->m_plog->FNoMoreLogWrite() );
        }

        fLogStopped = fTrue;
        
        Assert( fCircularLogging || m_lgposFullBackup.lGeneration > 1 );

        m_lgenCopyMic = 0;
        m_lgenCopyMac = m_lgposFullBackup.lGeneration;

        if ( !fCircularLogging )
        {
            Assert( 0 != CmpLgpos( &m_lgposFullBackup, &lgposMin ) );
            m_lgenCopyMac--;
        }
    }
    else
    {

        Call( ErrLGCheckIncrementalBackup( m_pinst, DBFILEHDR::backupOSSnapshot ) );

        if ( fCircularLogging )
        {
            Call( ErrERRCheck( JET_errInvalidBackup ) );
        }

        LGIGetDateTime( &m_logtimeIncBackup );

        Call ( ErrLGBackupPrepLogs(
                    m_pinst->m_plog,
                    DBFILEHDR::backupOSSnapshot, fIncremental,
                    &m_logtimeIncBackup,
                    grbitLogRec,
                    &m_lgposIncBackup ) );

        if ( fCreateAndStopOnNewGen )
        {
            m_pinst->m_plog->LGWaitLogPaused();
            Assert( m_pinst->m_plog->FLGLogPaused() || m_pinst->m_plog->FNoMoreLogWrite() );
        }

        fLogStopped = fTrue;
        
        Assert (m_lgposIncBackup.lGeneration > 1);
        m_lgenCopyMac = m_lgposIncBackup.lGeneration - 1;

        Call ( m_pinst->m_plog->ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), &m_lgenCopyMic, NULL ) );

        if ( 0 == m_lgenCopyMic )
        {
            Assert( 1 == m_lgenCopyMac );
            m_lgenCopyMic = 1;
        }

        Call( ErrBKICheckLogsForIncrementalBackup( m_lgenCopyMic ) );

    }


    if ( !fCircularLogging )
    {
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

        m_pinst->m_plog->LGLockWrite();



        m_pinst->m_plog->LGCreateAsynchCancel( fFalse );


        m_pinst->m_plog->LGMakeLogName( wszLogName, sizeof(wszLogName), eCurrentTmpLog );

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


        m_pinst->m_plog->LGMakeLogName( wszTempLogName, sizeof(wszTempLogName), eCurrentTmpLog );

        CallS( m_pinst->m_plog->ErrStartAsyncLogFileCreation( wszTempLogName, lGenSignalTempID ) );
    }
    else
    {
        m_pinst->m_plog->LGLockWrite();
        Assert ( m_pinst->m_plog->FLGLogPaused() );

        ERR err = JET_errSuccess;

        err = m_pinst->m_plog->ErrLGNewLogFile(
                                0 ,
                                fLGLogRenameOnly);

        Assert ( JET_errSuccess <= err || m_pinst->m_plog->FNoMoreLogWrite() );

        m_pinst->m_plog->LGSetLogPaused( fFalse );
    }
    m_pinst->m_plog->LGUnlockWrite();

    m_pinst->m_plog->FLGSignalWrite( );

    const INT cbFillBuffer = 128;
    char szTrace[cbFillBuffer + 1];
    LGPOS lgposFreezeLogRec = lgposMax;

    BKIOSSnapshotGetFreezeLogRec( &lgposFreezeLogRec );

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
    const BOOL              fLogOnly,
    const BOOL              fLogTruncated,
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
        fLoggedNow = fTrue;
    }

    bkinfoType = ( bkinfoType == DBFILEHDR::backupSurrogate ) ? DBFILEHDR::backupOSSnapshot : bkinfoType;

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

        if ( fLogTruncated )
        {
            if ( !fLogOnly )
            {
                pdbfilehdr->bkinfoTypeIncPrev = DBFILEHDR::backupNormal;
                memset( &pdbfilehdr->bkinfoIncPrev, 0, sizeof( BKINFO ) );

                pdbfilehdr->bkinfoTypeDiffPrev = DBFILEHDR::backupNormal;
                memset( &pdbfilehdr->bkinfoDiffPrev, 0, sizeof( pdbfilehdr->bkinfoDiffPrev ) );
            }

            switch( bkinfoType )
            {
                case DBFILEHDR::backupNormal:
                    memset( &(pdbfilehdr->bkinfoSnapshotCur), 0, sizeof( BKINFO ) );
                    pdbfilehdr->bkinfoTypeIncPrev = DBFILEHDR::backupNormal;
                    break;

                case DBFILEHDR::backupOSSnapshot:
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
        fNormal = fFalse;
        if ( m_fBackupInProgressLocal )
        {
            error = ErrERRCheck( JET_errBackupInProgress );
        }
        else
        {
            error = ErrERRCheck( JET_errSurrogateBackupInProgress );
        }
    }

    if ( fNormal )
    {
        if ( m_fStopBackup )
        {
            fNormal = fFalse;
            error = ErrERRCheck( JET_errBackupAbortByServer );
        }
        else if ( m_fBackupStatus == backupStateDatabases )
        {
            fNormal = fFalse;
            error = ErrERRCheck( errBackupAbortByCaller );
        }
    }


    
    for ( INT irhf = 0; irhf < crhfMax; ++irhf )
    {
        if ( m_rgrhf[irhf].fInUse && m_rgrhf[irhf].pfapi )
        {
            Assert ( !m_rgrhf[irhf].fDatabase );
            delete m_rgrhf[irhf].pfapi;
            m_rgrhf[irhf].pfapi = NULL;
        }
    }

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


                if ( fSurrogateBackup )
                {
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
                    Assert( pfmp->Ppatchhdr() );
                    Assert( pfmp->Pdbfilehdr()->bkinfoFullCur.le_genLow == 0 );
                    PATCH_HEADER_PAGE * ppatchHdr = pfmp->Ppatchhdr();

                    LGPOS lgposT = ppatchHdr->bkinfo.le_lgposMark;
                    BOOL fLoggedT = fFalse;
                    err = ErrBKUpdateHdrBackupInfo( ifmp,
                        DBFILEHDR::backupNormal, fFalse ,
                        JET_bitBackupTruncateDone & grbit || FBKLogsTruncated(),
                        ppatchHdr->bkinfo.le_genLow, m_lgenCopyMac - 1,
                        &(lgposT), &(ppatchHdr->bkinfo.logtimeMark),
                        &fLoggedT );
                }
                else if ( !m_fBackupFull )
                {
                    BOOL fLoggedT = fFalse;
                    err = ErrBKUpdateHdrBackupInfo( ifmp,
                        DBFILEHDR::backupNormal, fTrue ,
                        JET_bitBackupTruncateDone & grbit || FBKLogsTruncated(),
                        m_lgenCopyMic, m_lgenCopyMac - 1,
                        &m_lgposIncBackup, &m_logtimeIncBackup,
                        &fLoggedT );
                }
                else
                {
                    Assert( !pfmp->FBackupFileCopyDone() );
                    err = JET_errSuccess;
                }
                errUpdateLog = ( errUpdateLog < JET_errSuccess ) ? errUpdateLog : err;
                err = JET_errSuccess;

            }
        }

    }

    if ( !fNormal && !m_pinst->FRecovering() )
    {
        LGPOS       lgposRecT;
        ErrLGBackupAbort( m_pinst->m_plog, fSurrogateBackup ? DBFILEHDR::backupSurrogate : DBFILEHDR::backupNormal,
                    !m_fBackupFull, &lgposRecT );
    }

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


    

    for ( INT irhf = 0; irhf < crhfMax; ++irhf )
    {
        if ( m_rgrhf[irhf].fInUse )
        {
            
            Assert( !fSurrogateBackup );
            
            if ( m_rgrhf[irhf].fDatabase )
            {
                const IFMP ifmp = m_rgrhf[irhf].ifmp;
                FMP::AssertVALIDIFMP( ifmp );
                Assert( !g_rgfmp[ ifmp ].PfapiPatch() );
                CallS( ErrDBCloseDatabase( m_ppibBackup, ifmp, 0 ) );
            }

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

    
    if ( fNormal )
    {
        CallS( error );

        if ( m_fBackupIsInternal )
        {
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


        ERR     errUpdateHdr;
        BOOL    fSkippedAttachDetach;
        LOGTIME tmEmpty;
        memset( &tmEmpty, 0, sizeof(LOGTIME) );

        m_pinst->m_plog->LockCheckpoint();
        errUpdateHdr = m_pinst->m_plog->ErrLGUpdateGenRequired( m_pinst->m_pfsapi, 0, 0, 0, tmEmpty, &fSkippedAttachDetach );
        m_pinst->m_plog->UnlockCheckpoint();

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
        CallS( m_pscrubdb->ErrTerm() );
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


    if ( !m_pinst->FRecovering() )
    {
        ErrLGBackupStop( m_pinst->m_plog,
                         fSurrogateBackup ? DBFILEHDR::backupSurrogate : DBFILEHDR::backupNormal,
                         !m_fBackupFull,
                         BoolParam( m_pinst, JET_paramAggressiveLogRollover ) ? fLGCreateNewGen : 0,
                         NULL );
    }


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


VOID BACKUP_CONTEXT::BKIGetPatchName( __out_bcount(OSFSAPI_MAX_PATH * sizeof(WCHAR)) PWSTR wszPatch, PCWSTR wszDatabaseName, __in PCWSTR wszDirectory)
{
    Assert ( wszDatabaseName );

    WCHAR   wszFNameT[ IFileSystemAPI::cchPathMax ];
    WCHAR   wszDirT[ IFileSystemAPI::cchPathMax ];
    WCHAR   wszExtT[ IFileSystemAPI::cchPathMax ];

    CallS( m_pinst->m_pfsapi->ErrPathParse( wszDatabaseName, wszDirT, wszFNameT, wszExtT ) );

    if ( wszDirectory )
    {
        LGMakeName( m_pinst->m_pfsapi, wszPatch, wszDirectory, wszFNameT, wszPatExt );
    }
    else
    {
        LGMakeName( m_pinst->m_pfsapi, wszPatch, wszDirT, wszFNameT, wszPatExt );
    }
}


ERR ISAMAPI  ErrIsamSnapshotStart(  JET_INSTANCE        instance,
                                    JET_PCSTR           szDatabases,
                                    JET_GRBIT           grbit)
{
    return ErrERRCheck( JET_wrnNyi );
}

ERR ISAMAPI  ErrIsamSnapshotStop(   JET_INSTANCE        instance,
                                    JET_GRBIT           grbit)
{
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
#endif

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
        pcheckpoint->le_lgposFullBackup = m_lgposFullBackup;
        pcheckpoint->logtimeFullBackup = m_logtimeFullBackup;
    }
    if ( m_lgposIncBackup.lGeneration )
    {
        pcheckpoint->le_lgposIncBackup = m_lgposIncBackup;
        pcheckpoint->logtimeIncBackup = m_logtimeIncBackup;
    }
}

VOID BACKUP_CONTEXT::BKInitForSnapshot( BOOL fIncrementalSnapshot, LONG lgenCheckpoint )
{
    m_lgenDeleteMic = 0;
    if ( !fIncrementalSnapshot )
    {
        m_lgenCopyMic = lgenCheckpoint;
    }
    Assert( 1 <= m_lgenCopyMic );
    m_lgenDeleteMac = min( m_lgenCopyMac, lgenCheckpoint );
}

