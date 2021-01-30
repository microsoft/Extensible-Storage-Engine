// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "logstd.hxx"


extern VOID       ITDBGSetConstants( INST * pinst = NULL);

ERR ErrLGCheckDBFiles(
    INST *pinst,
    RSTMAP * pDbMapEntry,
    INT genLow,
    INT genHigh,
    LGPOS *plgposSnapshotRestore = NULL )
{
    ERR             err;
    DBFILEHDR_FIX * pdbfilehdrDb    = NULL;
    PATCHHDR *      ppatchHdr       = NULL;;
    SIGNATURE       signLog = pinst->m_plog->SignLog();
    CFlushMapForUnattachedDb* pfm   = NULL;

    Assert ( pDbMapEntry );
    const WCHAR * wszDatabase = pDbMapEntry->wszNewDatabaseName;

    BOOL fFromSnapshotBackup = fFalse;
    ULONG ulGenHighFound = 0;
    ULONG ulGenLowFound = 0;

    
    Alloc( pdbfilehdrDb = (DBFILEHDR_FIX *)PvOSMemoryPageAlloc( g_cbPage, NULL ) );
    err = ErrUtilReadShadowedHeader(
            pinst,
            pinst->m_pfsapi,
            wszDatabase,
            (BYTE*)pdbfilehdrDb,
            g_cbPage,
            OffsetOf( DBFILEHDR_FIX, le_cbPageSize ),
            urhfReadOnly );

    if ( err < 0 )
    {
        if ( FErrIsDbHeaderCorruption( err ) )
        {
            OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"e6fe84fe-9d11-4323-a13f-414f97044e75" );
            err = ErrERRCheck( JET_errDatabaseCorrupted );
        }
        Error( err );
    }

    memcpy ( &pDbMapEntry->signDatabase, &pdbfilehdrDb->signDb , sizeof(SIGNATURE) );

    if ( attribDb != pdbfilehdrDb->le_attrib )
    {
        Assert ( fFalse );
        OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"1e4825d4-5021-4667-9131-aefabce3abe5" );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    fFromSnapshotBackup = ( CmpLgpos ( &(pdbfilehdrDb->bkinfoSnapshotCur.le_lgposMark), &lgposMin ) > 0 );

    if ( plgposSnapshotRestore )
    {
        *plgposSnapshotRestore = pdbfilehdrDb->bkinfoSnapshotCur.le_lgposMark;
    }

    if ( fFromSnapshotBackup )
    {
        Assert ( plgposSnapshotRestore );
        err = JET_errSuccess;

        if ( memcmp( &pdbfilehdrDb->signLog, &signLog, sizeof( SIGNATURE ) ) != 0 )
        {
            const UINT csz = 1;
            const WCHAR *rgszT[csz];

            rgszT[0] = wszDatabase;

            UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                DATABASE_PATCH_FILE_MISMATCH_ERROR_ID, csz, rgszT, 0, NULL, pinst);
            Error( ErrERRCheck( JET_errDatabasePatchFileMismatch ) );
        }

        ulGenHighFound = pdbfilehdrDb->bkinfoSnapshotCur.le_genHigh;
        ulGenLowFound = pdbfilehdrDb->bkinfoSnapshotCur.le_genLow;

        Assert( err == JET_errSuccess );
        goto EndOfCheckLogRange;
    }

    Alloc( ppatchHdr = (PATCHHDR *)PvOSMemoryPageAlloc( g_cbPage, NULL ) );

    if ( 0 == pdbfilehdrDb->bkinfoFullCur.le_genLow )
    {
        Error( ErrERRCheck( JET_errRestoreOfNonBackupDatabase ) );
    }

    if ( pdbfilehdrDb->bkinfoFullCur.le_genHigh )
    {
        ulGenHighFound = pdbfilehdrDb->bkinfoFullCur.le_genHigh;
        ulGenLowFound = pdbfilehdrDb->bkinfoFullCur.le_genLow;
        Assert( err == JET_errSuccess );
        goto EndOfCheckLogRange;
    }

    Call( ErrDBReadAndCheckDBTrailer( pinst, pinst->m_pfsapi, wszDatabase, (BYTE*)ppatchHdr, g_cbPage ) );

    if ( memcmp( &pdbfilehdrDb->signDb, &ppatchHdr->signDb, sizeof( SIGNATURE ) ) != 0 ||
         memcmp( &pdbfilehdrDb->signLog, &signLog, sizeof( SIGNATURE ) ) != 0 ||
         memcmp( &ppatchHdr->signLog, &signLog, sizeof( SIGNATURE ) ) != 0 ||
         CmpLgpos( &pdbfilehdrDb->bkinfoFullCur.le_lgposMark,
                   &ppatchHdr->bkinfo.le_lgposMark ) != 0 )
    {
        const UINT csz = 1;
        const WCHAR *rgszT[csz];

        rgszT[0] = wszDatabase;
        UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                    DATABASE_PATCH_FILE_MISMATCH_ERROR_ID, csz, rgszT, 0, NULL, pinst );
        Error( ErrERRCheck( JET_errDatabasePatchFileMismatch ) );
    }
    else
    {
        ulGenHighFound = ppatchHdr->bkinfo.le_genHigh;
        ulGenLowFound = ppatchHdr->bkinfo.le_genLow;
    }


EndOfCheckLogRange:
    if ( ulGenLowFound < (ULONG) genLow )
    {
        
        WCHAR   wszT1[IFileSystemAPI::cchPathMax];
        WCHAR   wszT2[IFileSystemAPI::cchPathMax];
        const WCHAR *rgszT[] = { wszT1, wszT2 };

        pinst->m_plog->LGFullLogNameFromLogId( wszT1, genLow, NULL );
        pinst->m_plog->LGFullLogNameFromLogId( wszT2, ulGenLowFound, NULL );

        UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                    STARTING_RESTORE_LOG_TOO_HIGH_ERROR_ID, 2, rgszT, 0, NULL, pinst );
        
        OSUHAPublishEvent(  HaDbFailureTagCorruption,
                            pinst,
                            HA_LOGGING_RECOVERY_CATEGORY,
                            HaDbIoErrorNone, NULL, 0, 0,
                            HA_STARTING_RESTORE_LOG_TOO_HIGH_ERROR_ID,
                            2,
                            rgszT );

        err = ErrERRCheck( JET_errStartingRestoreLogTooHigh );
    }

    else if ( ulGenHighFound > (ULONG) genHigh )
    {
        
        WCHAR   szT1[IFileSystemAPI::cchPathMax];
        WCHAR   szT2[IFileSystemAPI::cchPathMax];
        const WCHAR *rgszT[] = { szT1, szT2 };

        pinst->m_plog->LGFullLogNameFromLogId( szT1, genHigh, NULL );
        pinst->m_plog->LGFullLogNameFromLogId( szT2, ulGenHighFound, NULL );

        UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                    ENDING_RESTORE_LOG_TOO_LOW_ERROR_ID, 2, rgszT, 0, NULL, pinst );
        
        OSUHAPublishEvent(  HaDbFailureTagCorruption,
                            pinst,
                            HA_LOGGING_RECOVERY_CATEGORY,
                            HaDbIoErrorNone, NULL, 0, 0,
                            HA_ENDING_RESTORE_LOG_TOO_LOW_ERROR_ID,
                            2,
                            rgszT );

        err = ErrERRCheck( JET_errEndingRestoreLogTooLow );
    }

    Assert ( ulGenLowFound == pdbfilehdrDb->bkinfoFullCur.le_genLow);
    Assert ( ulGenHighFound <= (ULONG)genHigh);

    pdbfilehdrDb->bkinfoFullCur.le_genHigh = genHigh;

    if ( genHigh > pdbfilehdrDb->le_lGenMaxCommitted )
    {
        memset( &pdbfilehdrDb->logtimeGenMaxCreate, 0, sizeof(LOGTIME) );
        pdbfilehdrDb->le_lGenMaxCommitted = genHigh;
    }

    if ( genHigh > pdbfilehdrDb->le_lGenMaxRequired )
    {
        memset( &pdbfilehdrDb->logtimeGenMaxRequired, 0, sizeof(LOGTIME) );
        pdbfilehdrDb->le_lGenMaxRequired = genHigh;
    }

    Call( CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDatabase, pdbfilehdrDb, pinst, &pfm ) );

    Call( ErrUtilWriteUnattachedDatabaseHeaders( pinst, pinst->m_pfsapi, wszDatabase, pdbfilehdrDb, NULL, pfm ) );
    CallS( err );

    if ( pfm != NULL )
    {
        if ( pdbfilehdrDb->Dbstate() == JET_dbstateCleanShutdown )
        {
            Call( pfm->ErrCleanFlushMap() );
        }

        pfm->TermFlushMap();
    }

    Call( ErrDBTryToZeroDBTrailer(  pinst,
                                    pinst->m_pfsapi,
                                    wszDatabase ) );
    CallS( err );

HandleError:

    delete pfm;
    OSMemoryPageFree( ppatchHdr );
    OSMemoryPageFree( pdbfilehdrDb );

    return err;
}


ERR LOG::ErrLGIRSTCheckSignaturesLogSequence(
    __in PCWSTR   wszRestorePath,
    __in PCWSTR  wszLogFilePath,
    INT genLow,
    INT genHigh,
    __in_opt PCWSTR  wszTargetInstanceFilePath,
    INT genHighTarget )
{
    ERR             err = JET_errSuccess;
    LONG            gen;
    LONG            genLowT;
    LONG            genHighT;
    IFileAPI    *pfapiT = NULL;
    LGFILEHDR       *plgfilehdrT = NULL;
    LGFILEHDR       *plgfilehdrCur[2] = { NULL, NULL };
    LGFILEHDR       *plgfilehdrLow = NULL;
    LGFILEHDR       *plgfilehdrHigh = NULL;
    INT             ilgfilehdrAvail = 0;
    INT             ilgfilehdrCur;
    INT             ilgfilehdrPrv;
    BOOL            fReadyToCheckContiguity;
    PCWSTR          wszLogExt = NULL;

    BOOL fTargetInstanceCheck = fFalse;
    const WCHAR * wszLogFilePathCheck = NULL;

    AllocR( plgfilehdrT = (LGFILEHDR *)PvOSMemoryPageAlloc( sizeof(LGFILEHDR) * 4, NULL ) );

    plgfilehdrCur[0] = plgfilehdrT;
    plgfilehdrCur[1] = plgfilehdrT + 1;
    plgfilehdrLow = plgfilehdrT + 2;
    plgfilehdrHigh = plgfilehdrT + 3;

    
    for ( gen = genLow; gen <= genHigh; gen++ )
    {
        Expected( gen > 0 );
        ilgfilehdrCur = ilgfilehdrAvail++ % 2;
        ilgfilehdrPrv = ilgfilehdrAvail % 2;

        if ( NULL == m_pLogStream->LogExt() )
        {
            Assert( gen == genLow && wszLogExt == NULL );
            wszLogExt = WszLGGetDefaultExt( fFalse );
        }
        err = m_pLogStream->ErrLGRSTOpenLogFile( wszRestorePath, gen, &pfapiT, wszLogExt );
        if ( NULL == m_pLogStream->LogExt() )
        {
            Assert( gen == genLow );

            if ( err == JET_errFileNotFound )
            {
                if ( pfapiT != NULL )
                {
                    delete pfapiT;
                    pfapiT = NULL;
                }
                wszLogExt = WszLGGetOtherExt( fFalse );
                err = m_pLogStream->ErrLGRSTOpenLogFile( wszRestorePath, gen, &pfapiT, wszLogExt );
                if ( err )
                {
                    err = ErrERRCheck( JET_errFileNotFound );
                }
            }
            if ( err >= JET_errSuccess )
            {
                LGSetChkExts( FLGIsLegacyExt( fFalse, wszLogExt), fFalse );
                m_pLogStream->LGSetLogExt( FLGIsLegacyExt( fFalse, wszLogExt), fFalse );
            }
        }
        Call( err );
        Call( m_pLogStream->ErrLGReadFileHdr( pfapiT, iorpRestore, plgfilehdrCur[ ilgfilehdrCur ], fCheckLogID ) );
        delete pfapiT;
        pfapiT = NULL;

        if ( gen == genLow )
        {
            UtilMemCpy( plgfilehdrLow, plgfilehdrCur[ ilgfilehdrCur ], sizeof( LGFILEHDR ) );
        }

        if ( gen == genHigh )
        {
            UtilMemCpy( plgfilehdrHigh, plgfilehdrCur[ ilgfilehdrCur ], sizeof( LGFILEHDR ) );
        }

        if ( gen > genLow )
        {
            if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.signLog,
                         &plgfilehdrCur[ ilgfilehdrPrv ]->lgfilehdr.signLog,
                         sizeof( SIGNATURE ) ) != 0 )
            {
                WCHAR wszT[IFileSystemAPI::cchPathMax];
                const UINT  csz = 1;
                const WCHAR *rgszT[csz];
                rgszT[0] = wszT;

                m_pLogStream->LGFullLogNameFromLogId( wszT, gen, wszRestorePath );

                UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                    RESTORE_LOG_FILE_HAS_BAD_SIGNATURE_ERROR_ID, csz, rgszT, 0, NULL, m_pinst );
                Call( ErrERRCheck( JET_errGivenLogFileHasBadSignature ) );
            }
            if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.tmPrevGen,
                         &plgfilehdrCur[ ilgfilehdrPrv ]->lgfilehdr.tmCreate,
                         sizeof( LOGTIME ) ) != 0 )
            {
                WCHAR wszT[20];
                const UINT  csz = 1;
                const WCHAR *rgszT[csz];

                OSStrCbFormatW( wszT, sizeof( wszT ), L"%d", gen );
                rgszT[0] = wszT;
                UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                    RESTORE_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID, csz, rgszT, 0, NULL, m_pinst );
                Call( ErrERRCheck( JET_errGivenLogFileIsNotContiguous ) );
            }
        }
    }

    if ( gen <= genHigh )
    {
        Assert ( 0 );

        WCHAR wszT[IFileSystemAPI::cchPathMax];
        const UINT  csz = 1;
        const WCHAR *rgszT[csz];
        rgszT[0] = wszT;

        m_pLogStream->LGFullLogNameFromLogId( wszT, gen, wszRestorePath );

        UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                RESTORE_LOG_FILE_MISSING_ERROR_ID, csz, rgszT, 0, NULL, m_pinst );
        Call( ErrERRCheck( JET_errMissingRestoreLogFiles ) );
    }


    if ( wszTargetInstanceFilePath )
    {
        Assert ( genHighTarget );
        fTargetInstanceCheck = fTrue;
        wszLogFilePathCheck = wszTargetInstanceFilePath;
    }
    else
    {
        Assert ( ! fTargetInstanceCheck );
        wszLogFilePathCheck = wszLogFilePath;
    }

    Assert ( wszLogFilePathCheck );

    
{
    WCHAR   wszFullLogPath[IFileSystemAPI::cchPathMax];
    WCHAR   wszFullLogFilePathCheck[IFileSystemAPI::cchPathMax];

    Call( m_pinst->m_pfsapi->ErrPathComplete( wszRestorePath, wszFullLogPath ) );
    Call( m_pinst->m_pfsapi->ErrPathComplete( wszLogFilePathCheck, wszFullLogFilePathCheck ) );

#ifdef DEBUG
    if ( wszTargetInstanceFilePath )
    {
        Assert ( wszLogFilePathCheck == wszTargetInstanceFilePath );

        CallS ( m_pinst->m_pfsapi->ErrPathComplete( wszLogFilePath, wszFullLogPath ) );
        Assert ( UtilCmpFileName( wszFullLogPath, wszFullLogFilePathCheck ) );
    }
#endif

    Call( m_pinst->m_pfsapi->ErrPathComplete( wszRestorePath, wszFullLogPath ) );

    if ( UtilCmpFileName( wszFullLogPath, wszFullLogFilePathCheck ) != 0 &&
        ( JET_errSuccess == m_pLogStream->ErrLGGetGenerationRange( wszRestorePath, &genLowT, &genHighT ) ) )
    {
        m_pLogStream->LGRSTDeleteLogs( wszRestorePath, genLowT, genLow - 1, fFalse );
        m_pLogStream->LGRSTDeleteLogs( wszRestorePath, genHigh + 1, genHighT, fTrue );
    }
}

    
    Assert( m_pLogStream->LogExt() );
    Call ( m_pLogStream->ErrLGGetGenerationRange( wszLogFilePathCheck, &genLowT, &genHighT ) );

    if ( fTargetInstanceCheck )
    {
        Assert ( genHighTarget );
        Assert ( genHighT >= genHighTarget );
        Assert ( genLowT <= genHighTarget );


        if ( genHighT < genHighTarget || genLowT > genHighTarget || 0 == genLowT )
        {

            Assert(0);
            WCHAR wszT[IFileSystemAPI::cchPathMax];
            const UINT  csz = 1;
            const WCHAR *rgszT[csz];
            rgszT[0] = wszT;

            m_pLogStream->LGFullLogNameFromLogId( wszT, genHighTarget, wszLogFilePathCheck );

            UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                    RESTORE_LOG_FILE_MISSING_ERROR_ID, csz, rgszT, 0, NULL, m_pinst );
            Call( ErrERRCheck( JET_errMissingRestoreLogFiles ) );
        }

        genHighT = genHighTarget;
    }

    
    if ( genLowT > genHigh )
        fReadyToCheckContiguity = fTrue;
    else
        fReadyToCheckContiguity = fFalse;

    for ( gen = genLowT; gen <= ( fTargetInstanceCheck ? genHighT : genHighT + 1 ); gen++ )
    {
        if ( gen == 0 )
        {

            Assert ( !fTargetInstanceCheck );

            
            if ( m_pLogStream->ErrLGRSTOpenLogFile( wszLogFilePathCheck, lGenSignalCurrentID, &pfapiT ) < 0 )
                break;

            
            gen = genHigh + 1;
            genHighT = genHigh;
            Assert( gen == genHighT + 1 );
        }
        else
        {
            if ( gen == genHighT + 1 )
            {
                Assert ( !fTargetInstanceCheck );

                
                if ( m_pLogStream->ErrLGRSTOpenLogFile( wszLogFilePathCheck, lGenSignalCurrentID, &pfapiT ) < 0 )
                    break;
            }
            else
            {
                Assert( gen > 0 );
                err = m_pLogStream->ErrLGRSTOpenLogFile( wszLogFilePathCheck, gen, &pfapiT );
                if ( err == JET_errFileNotFound )
                {
                    WCHAR wszT[IFileSystemAPI::cchPathMax];
                    const UINT  csz = 1;
                    const WCHAR *rgszT[csz];

                    m_pLogStream->LGFullLogNameFromLogId( wszT, gen, wszLogFilePathCheck );

                    rgszT[0] = wszT;
                    if ( gen <= genHigh )
                    {
                        if ( _wcsicmp( wszRestorePath, wszLogFilePathCheck ) != 0 )
                        {
                            gen = genHigh;
                            continue;
                        }


                        Assert(0);
                        UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                                RESTORE_LOG_FILE_MISSING_ERROR_ID, csz, rgszT, 0, NULL, m_pinst );
                        Call( ErrERRCheck( JET_errMissingRestoreLogFiles ) );
                    }
                    else
                    {
                        UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
                            CURRENT_LOG_FILE_MISSING_ERROR_ID, csz, rgszT, 0, NULL, m_pinst );
                        err = ErrERRCheck( JET_errMissingCurrentLogFiles );
                    }
                }
                Call( err );
            }
        }


        ilgfilehdrCur = ilgfilehdrAvail++ % 2;
        ilgfilehdrPrv = ilgfilehdrAvail % 2;

        Call( m_pLogStream->ErrLGReadFileHdr( pfapiT, iorpRestore, plgfilehdrCur[ ilgfilehdrCur ], fNoCheckLogID ) );
        delete pfapiT;
        pfapiT = NULL;

        if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.signLog,
                     &plgfilehdrHigh->lgfilehdr.signLog,
                     sizeof( SIGNATURE ) ) != 0 )
        {
            INT genCurrent;
            WCHAR wszT1[20];
            const UINT  csz = 1;
            const WCHAR *rgszT[csz];
            rgszT[0] = wszT1;

            if ( gen < genLow )
            {
                genCurrent = genLow - 1;
            }
            else if ( gen <= genHigh )
            {
                genCurrent = gen;
            }
            else
            {
                genCurrent = genHighT + 1;
            }

            CallS( m_pLogStream->ErrLGMakeLogNameBaseless( wszT1, sizeof(wszT1), NULL,
                                            ( gen == genHighT + 1 || 0 == gen ) ? eCurrentLog : eArchiveLog,
                                            ( gen == genHighT + 1 || 0 == gen ) ? 0                 : gen ) );

            UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY,
                    EXISTING_LOG_FILE_HAS_BAD_SIGNATURE_ERROR_ID_2, csz, rgszT, 0, NULL, m_pinst );

            err = ErrERRCheck( JET_errExistingLogFileHasBadSignature );
            gen = genCurrent;
            fReadyToCheckContiguity = fFalse;
            continue;
        }

        if ( fReadyToCheckContiguity )
        {
            if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.tmPrevGen,
                         &plgfilehdrCur[ ilgfilehdrPrv ]->lgfilehdr.tmCreate,
                         sizeof( LOGTIME ) ) != 0 )
            {
                WCHAR wszT1[20];
                WCHAR wszT2[20];
                const UINT  csz = 2;
                const WCHAR *rgszT[csz];
                LONG genCur = plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.le_lGeneration;
                LONG genPrv = plgfilehdrCur[ ilgfilehdrPrv ]->lgfilehdr.le_lGeneration;

                err = ErrERRCheck( JET_errExistingLogFileIsNotContiguous );

                CallS( m_pLogStream->ErrLGMakeLogNameBaseless( wszT1, sizeof(wszT1), NULL,
                                                ( genPrv == genHighT + 1 || 0 == genPrv ) ? eCurrentLog : eArchiveLog,
                                                ( genPrv == genHighT + 1 || 0 == genPrv ) ? 0                 : genPrv ) );
                CallS( m_pLogStream->ErrLGMakeLogNameBaseless( wszT2, sizeof(wszT2), NULL,
                                                ( genCur == genHighT + 1 || 0 == genCur ) ? eCurrentLog : eArchiveLog,
                                                ( genCur == genHighT + 1 || 0 == genCur ) ? 0                 : genCur ) );
                rgszT[0] = wszT1;
                rgszT[1] = wszT2;
                UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY,
                    EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID_2, csz, rgszT, 0, NULL, m_pinst );

                if ( gen < genLow )
                {
                    continue;
                }
                else if ( gen <= genHigh )
                {
                    gen = genHigh;
                    continue;
                }
                else
                {
                    break;
                }
            }
        }

        if ( gen == genLow - 1 )
        {
            
            if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.tmCreate,
                         &plgfilehdrLow->lgfilehdr.tmPrevGen,
                         sizeof( LOGTIME ) ) != 0 )
            {
                WCHAR wszT1[20];
                WCHAR wszT2[20];
                const UINT csz = 2;
                const WCHAR *rgszT[csz];
                LONG genCur = plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.le_lGeneration;
                LONG genPrv = plgfilehdrLow->lgfilehdr.le_lGeneration;

                CallS( m_pLogStream->ErrLGMakeLogNameBaseless( wszT1, sizeof(wszT1), NULL,
                                                ( genPrv == genHighT + 1 || 0 == genPrv ) ? eCurrentLog : eArchiveLog,
                                                ( genPrv == genHighT + 1 || 0 == genPrv ) ? 0                 : genPrv ) );
                CallS( m_pLogStream->ErrLGMakeLogNameBaseless( wszT2, sizeof(wszT2), NULL,
                                                ( genCur == genHighT + 1 || 0 == genCur ) ? eCurrentLog : eArchiveLog,
                                                ( genCur == genHighT + 1 || 0 == genCur ) ? 0                 : genCur ) );

                rgszT[0] = wszT1;
                rgszT[1] = wszT2;
                UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY,
                    EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID_2, csz, rgszT, 0, NULL, m_pinst );

                err = ErrERRCheck( JET_errExistingLogFileIsNotContiguous );
                fReadyToCheckContiguity = fFalse;
                continue;
            }
        }

        if ( gen == genLow )
        {
            
            if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.tmCreate,
                         &plgfilehdrLow->lgfilehdr.tmCreate,
                         sizeof( LOGTIME ) ) != 0 )
            {
                WCHAR wszT1[20];
                WCHAR wszT2[20];
                const UINT csz = 2;
                const WCHAR *rgszT[csz];
                LONG genCur = plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.le_lGeneration;
                LONG genPrv = plgfilehdrLow->lgfilehdr.le_lGeneration;

                CallS( m_pLogStream->ErrLGMakeLogNameBaseless( wszT1, sizeof(wszT1), NULL,
                                                ( genPrv == genHighT + 1 || 0 == genPrv ) ? eCurrentLog : eArchiveLog,
                                                ( genPrv == genHighT + 1 || 0 == genPrv ) ? 0                 : genPrv ) );
                CallS( m_pLogStream->ErrLGMakeLogNameBaseless( wszT2, sizeof(wszT2), NULL,
                                                ( genCur == genHighT + 1 || 0 == genCur ) ? eCurrentLog : eArchiveLog,
                                                ( genCur == genHighT + 1 || 0 == genCur ) ? 0                 : genCur ) );

                rgszT[0] = wszT1;
                rgszT[1] = wszT2;
                UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY,
                    EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID_2, csz, rgszT, 0, NULL, m_pinst );

                err = ErrERRCheck( JET_errExistingLogFileIsNotContiguous );

                Assert( _wcsicmp( wszRestorePath, wszLogFilePathCheck ) != 0 );
                gen = genHigh;
                continue;
            }
        }

        if ( gen == genHigh + 1 )
        {
            
            if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.tmPrevGen,
                         &plgfilehdrHigh->lgfilehdr.tmCreate,
                         sizeof( LOGTIME ) ) != 0 )
            {
                WCHAR wszT1[20];
                WCHAR wszT2[20];
                const UINT csz = 2;
                const WCHAR *rgszT[csz];
                LONG genCur = plgfilehdrCur[ ilgfilehdrCur ]->lgfilehdr.le_lGeneration;
                LONG genPrv = plgfilehdrHigh->lgfilehdr.le_lGeneration;

                CallS( m_pLogStream->ErrLGMakeLogNameBaseless( wszT1, sizeof(wszT1), NULL,
                                                ( genPrv == genHighT + 1 || 0 == genPrv ) ? eCurrentLog : eArchiveLog,
                                                ( genPrv == genHighT + 1 || 0 == genPrv ) ? 0                 : genPrv ) );
                CallS( m_pLogStream->ErrLGMakeLogNameBaseless( wszT2, sizeof(wszT2), NULL,
                                                ( genCur == genHighT + 1 || 0 == genCur ) ? eCurrentLog : eArchiveLog,
                                                ( genCur == genHighT + 1 || 0 == genCur ) ? 0                 : genCur ) );

                rgszT[0] = wszT1;
                rgszT[1] = wszT2;
                UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY,
                    EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID_2, csz, rgszT, 0, NULL, m_pinst );

                err = ErrERRCheck( JET_errExistingLogFileIsNotContiguous );
                break;
            }
        }

        fReadyToCheckContiguity = fTrue;
    }

HandleError:

    delete pfapiT;

    OSMemoryPageFree( plgfilehdrT );

    return err;
}




ERR LOG::ErrLGIRSTInitPath(
    __in PCWSTR wszBackupPath,
    __in PCWSTR wszNewLogPath,
    __out_bcount(cbOSFSAPI_MAX_PATHW) PWSTR wszRestorePath,
    __in_range(sizeof(WCHAR), cbOSFSAPI_MAX_PATHW) const ULONG cbRestorePath,
    __out_bcount(cbOSFSAPI_MAX_PATHW) PWSTR wszLogDirPath,
    __in_range(sizeof(WCHAR), cbOSFSAPI_MAX_PATHW) const ULONG cbLogDirPath )

{
    ERR err;

    CallR( m_pinst->m_pfsapi->ErrPathComplete( wszBackupPath == NULL ? L"." : wszBackupPath, wszRestorePath ) );
    CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszRestorePath, cbRestorePath ) );

    m_wszLogCurrent = wszRestorePath;

    CallR( m_pinst->m_pfsapi->ErrPathComplete( wszNewLogPath, wszLogDirPath ) );
    CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszLogDirPath, cbLogDirPath ) );

    return JET_errSuccess;
}



ERR LOG::ErrLGIRSTSetupCheckpoint(
    LONG lgenLow,
    LONG lgenHigh,
    __in PCWSTR wszCurCheckpoint )
{
    ERR         err;
    WCHAR       wszT[IFileSystemAPI::cchPathMax];
    LGPOS       lgposCheckpoint;


    

    
    Call( m_pLogStream->ErrLGMakeLogNameBaseless( wszT, sizeof(wszT), m_wszRestorePath, eArchiveLog, lgenLow ) );
    Assert( LOSStrLengthW( wszT ) * sizeof( WCHAR ) <= sizeof( wszT ) - 1 );
    Call( m_pLogStream->ErrLGOpenFile( wszT ) );

    
    Call( m_pLogStream->ErrLGReadFileHdr( NULL, iorpRestore, NULL, fCheckLogID ) );
    m_pcheckpoint->checkpoint.dbms_param = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.dbms_param;

    lgposCheckpoint.lGeneration = lgenLow;
    lgposCheckpoint.isec = (WORD) m_pLogStream->CSecHeader();
    lgposCheckpoint.ib = 0;
    m_pcheckpoint->checkpoint.le_lgposCheckpoint = lgposCheckpoint;
    m_pcheckpoint->checkpoint.le_lgposDbConsistency = lgposCheckpoint;

    Assert( sizeof( m_pcheckpoint->rgbAttach ) == cbAttach );
    UtilMemCpy( m_pcheckpoint->rgbAttach, m_pLogStream->GetCurrentFileHdr()->rgbAttach, cbAttach );

    
    if ( wszCurCheckpoint )
    {
        Assert( LOSStrLengthW( wszCurCheckpoint ) + 1 + LOSStrLengthW( SzParam( m_pinst, JET_paramBaseName ) ) + LOSStrLengthW( m_wszChkExt ) < IFileSystemAPI::cchPathMax - 1 );
        if ( LOSStrLengthW( wszCurCheckpoint ) + 1 + LOSStrLengthW( SzParam( m_pinst, JET_paramBaseName ) ) + LOSStrLengthW( m_wszChkExt ) >= IFileSystemAPI::cchPathMax - 1 )
        {
            Error( ErrERRCheck( JET_errInvalidPath ) );
        }
        OSStrCbCopyW( wszT, sizeof(wszT), wszCurCheckpoint );
        CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszT, sizeof(wszT) ) );
        OSStrCbAppendW( wszT, sizeof(wszT), SzParam( m_pinst, JET_paramBaseName ) );
        OSStrCbAppendW( wszT, sizeof(wszT), m_wszChkExt );

        CallSx( m_pinst->m_pfsapi->ErrFileDelete( wszT ), JET_errFileNotFound );

        Assert( LOSStrLengthW( wszCurCheckpoint ) < IFileSystemAPI::cchPathMax - 1 );
        Param( m_pinst, JET_paramSystemPath )->Set( m_pinst, ppibNil, 0, wszCurCheckpoint );
    }

HandleError:
    m_pLogStream->LGCloseFile();

    return err;
}

#define cRestoreStatusPadding   0.10

VOID LOG::LGIRSTPrepareCallback(
    LGSTATUSINFO *              plgstat,
    LONG                        lgenHigh,
    LONG                        lgenLow,
    const LONG                  lgenHighStop
    )
{
    
    if ( SzParam( m_pinst, JET_paramLogFilePath ) && *SzParam( m_pinst, JET_paramLogFilePath ) != '\0' )
    {
        LONG    lgenHighT;
        WCHAR   wszFNameT[IFileSystemAPI::cchPathMax];

        
        (void)m_pLogStream->ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), NULL, &lgenHighT );

        

        Assert( LOSStrLengthW( SzParam( m_pinst, JET_paramLogFilePath ) ) + LOSStrLengthW( SzParam( m_pinst, JET_paramBaseName ) ) + LOSStrLengthW( m_pLogStream->LogExt() ) < IFileSystemAPI::cchPathMax - 1 );
        OSStrCbCopyW( wszFNameT, sizeof(wszFNameT), SzParam( m_pinst, JET_paramLogFilePath ) );
        OSStrCbAppendW( wszFNameT, sizeof(wszFNameT), SzParam( m_pinst, JET_paramBaseName ) );
        OSStrCbAppendW( wszFNameT, sizeof(wszFNameT), m_pLogStream->LogExt() );

        if ( ErrUtilPathExists( m_pinst->m_pfsapi, wszFNameT ) == JET_errSuccess )
        {
            lgenHighT++;
        }

        lgenHigh = max( lgenHigh, lgenHighT );

        Assert( lgenHigh >= m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration );
    }

    if ( lgenHighStop )
    {
        lgenHigh = min( lgenHigh, lgenHighStop );
        lgenLow = min( lgenLow, lgenHighStop );
    }

    plgstat->cGensSoFar = 0;
    plgstat->cGensExpected = lgenHigh - lgenLow + 1;

    
    plgstat->fCountingSectors = (plgstat->cGensExpected <
            (ULONG)((100 - ( m_fHardRestore ? (cRestoreStatusPadding * 100) : 0 ) ) * 2/3));

    
    plgstat->pfnCallback = m_pinst->m_pfnInitCallback;
    plgstat->pvCallbackContext = m_pinst->m_pvInitCallbackContext;
    plgstat->snprog.cbStruct = sizeof(JET_SNPROG);
    plgstat->snprog.cunitDone = 0;
    plgstat->snprog.cunitTotal = 100;

    (plgstat->pfnCallback)( JET_snpRestore, JET_sntBegin, &plgstat->snprog, plgstat->pvCallbackContext );
}

ERR ErrLGCheckDir( IFileSystemAPI *const pfsapi, __inout_bcount(cbDir) const PWSTR wszDir, ULONG cbDir, __in_opt PCWSTR wszSearch );

ERR LOG::ErrLGRestore(
    __in PCWSTR wszBackup,
    __in PCWSTR wszDest,
    JET_PFNINITCALLBACK pfn,
    void * pvContext )
{
    ERR                 err;
    WCHAR               wszBackupPath[IFileSystemAPI::cchPathMax] = {0};
    WCHAR               wszLogDirPath[cbFilenameMost];
    BOOL                fLogDisabledSav;
    LONG                lgenLow;
    LONG                lgenHigh;
    LGSTATUSINFO        lgstat = { 0 };
    LGSTATUSINFO        *plgstat = NULL;
    const WCHAR         *rgszT[2];
    BOOL                fNewCheckpointFile;
    ULONG               cbSecVolumeSave;
    BOOL                fGlobalRepairSave = g_fRepair;

    TraceContextScope tcScope( iortBackupRestore );
    tcScope->nParentObjectClass = tceNone;

    Assert( !g_fRepair );

    if ( _wcsicmp( SzParam( m_pinst, JET_paramRecovery ), L"repair" ) == 0 )
    {
        g_fRepair = fTrue;
    }

    if ( LOSStrLengthW( wszBackup ) >= IFileSystemAPI::cchPathMax - 1 )
    {
        err = ErrERRCheck( JET_errInvalidPath );
        goto ResetGlobalRepair;
    }
    Assert( LOSStrLengthW( wszBackup ) < IFileSystemAPI::cchPathMax - 1 );
    OSStrCbCopyW( wszBackupPath, IFileSystemAPI::cchPathMax * sizeof(WCHAR), wszBackup );

    Assert( m_pinst->m_fSTInit == fSTInitDone || m_pinst->m_fSTInit == fSTInitNotDone );
    if ( m_pinst->m_fSTInit == fSTInitDone )
    {
        err = ErrERRCheck( JET_errAfterInitialization );
        goto ResetGlobalRepair;
    }

    if ( wszDest )
    {
        if ( BoolParam( m_pinst, JET_paramCreatePathIfNotExist ) )
        {
            WCHAR wszT[IFileSystemAPI::cchPathMax];

            Assert( LOSStrLengthW( wszDest ) + 1 < IFileSystemAPI::cchPathMax - 1 );
            if ( LOSStrLengthW( wszDest ) + 1 >= IFileSystemAPI::cchPathMax - 1 )
            {
                err = ErrERRCheck( JET_errInvalidPath );
                goto ResetGlobalRepair;
            }
            OSStrCbCopyW( wszT, sizeof(wszT), wszDest );
            CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszT, sizeof(wszT) ) );
            CallJ( ErrUtilCreatePathIfNotExist( m_pinst->m_pfsapi, wszT, m_wszNewDestination, sizeof(m_wszNewDestination) ), ResetGlobalRepair );
        }
        else
        {
            CallJ( m_pinst->m_pfsapi->ErrPathComplete( wszDest, m_wszNewDestination ), ResetGlobalRepair );
        }
        CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( m_wszNewDestination, sizeof(m_wszNewDestination) ) );
    }
    else
    {
        m_wszNewDestination[0] = L'\0';
    }

    m_fSignLogSet = fFalse;


{
    ULONG cbSecVolumeNew;
    WCHAR wszFullName[IFileSystemAPI::cchPathMax];
    if ( m_pinst->m_pfsapi->ErrPathComplete( SzParam( m_pinst, JET_paramLogFilePath ), wszFullName ) == JET_errInvalidPath )
    {
        const WCHAR *szPathT[1] = { SzParam( m_pinst, JET_paramLogFilePath ) };
        UtilReportEvent(
                eventError,
                LOGGING_RECOVERY_CATEGORY,
                FILE_NOT_FOUND_ERROR_ID,
                1,
                szPathT,
                0,
                NULL,
                m_pinst );
        CallJ( ErrERRCheck( JET_errFileNotFound ), ResetGlobalRepair );
    }

    CallJ( m_pinst->m_pfsapi->ErrFileAtomicWriteSize( wszFullName, &cbSecVolumeNew ), ResetGlobalRepair );
    m_pLogStream->SetCbSecVolume( cbSecVolumeNew );
}

    cbSecVolumeSave = m_pLogStream->CbSecVolume();


    Assert( m_fUseRecoveryLogFileSize == fFalse );
    m_fUseRecoveryLogFileSize = fTrue;

    CallJ( ErrLGIRSTInitPath( wszBackupPath, SzParam( m_pinst, JET_paramLogFilePath ), m_wszRestorePath, sizeof(m_wszRestorePath), wszLogDirPath, sizeof(wszLogDirPath) ), ReturnError );
    CallJ ( m_pLogStream->ErrLGGetGenerationRange( m_wszRestorePath, &lgenLow, &lgenHigh, fTrue ), ReturnError );
    Assert( ( lgenLow > 0 && lgenHigh >= lgenLow ) || ( 0 == lgenLow && 0 == lgenHigh ) );

    if ( 0 == lgenLow || 0 == lgenHigh )
    {


        CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszBackupPath, sizeof(wszBackupPath) ) );
        err = ErrLGCheckDir( m_pinst->m_pfsapi, wszBackupPath, sizeof(wszBackupPath), wszAtomicNew );
        if ( err == JET_errBackupDirectoryNotEmpty )
        {
            OSStrCbAppendW( wszBackupPath, sizeof(wszBackupPath), wszAtomicNew );
            CallJ( ErrLGIRSTInitPath( wszBackupPath, SzParam( m_pinst, JET_paramLogFilePath ), m_wszRestorePath, sizeof(m_wszRestorePath), wszLogDirPath, sizeof(wszLogDirPath) ), ReturnError );
            CallJ ( m_pLogStream->ErrLGGetGenerationRange( m_wszRestorePath, &lgenLow, &lgenHigh, fTrue ), ReturnError );
            Assert( ( lgenLow > 0 && lgenHigh >= lgenLow ) || ( 0 == lgenLow && 0 == lgenHigh ) );
        }
        CallJ( err, ReturnError );
    }

    if ( 0 == lgenLow || 0 == lgenHigh )
    {


        CallS( m_pinst->m_pfsapi->ErrPathFolderNorm( wszBackupPath, sizeof(wszBackupPath) ) );
        err = ErrLGCheckDir( m_pinst->m_pfsapi, wszBackupPath, sizeof(wszBackupPath), wszAtomicOld );
        if ( err == JET_errBackupDirectoryNotEmpty )
        {
            OSStrCbAppendW( wszBackupPath, sizeof(wszBackupPath), wszAtomicOld );
            CallJ( ErrLGIRSTInitPath( wszBackupPath, SzParam( m_pinst, JET_paramLogFilePath ), m_wszRestorePath, sizeof(m_wszRestorePath), wszLogDirPath, sizeof(wszLogDirPath) ), ReturnError );
            CallJ ( m_pLogStream->ErrLGGetGenerationRange( m_wszRestorePath, &lgenLow, &lgenHigh, fTrue ), ReturnError );
            Assert( ( lgenLow > 0 && lgenHigh >= lgenLow ) || ( 0 == lgenLow && 0 == lgenHigh ) );
        }
        CallJ( err, ReturnError );
    }

    if ( 0 == lgenLow || 0 == lgenHigh )
    {


        err = ErrERRCheck( JET_errFileNotFound );
        goto ReturnError;
    }

    Assert( lgenLow > 0 );
    Assert( lgenHigh >= lgenLow );
    CallJ( ErrLGIRSTCheckSignaturesLogSequence( m_wszRestorePath, wszLogDirPath, lgenLow, lgenHigh, NULL, 0 ), ReturnError );

    Assert( ( LOSStrLengthW( m_wszRestorePath ) + 1 ) * sizeof( WCHAR ) < sizeof( m_wszRestorePath ) );
    Assert( ( LOSStrLengthW( wszLogDirPath ) + 1 ) * sizeof( WCHAR ) < sizeof( wszLogDirPath ) );
    Assert( m_wszLogCurrent == m_wszRestorePath );


    fLogDisabledSav = m_fLogDisabled;
    m_fHardRestore = fTrue;
    m_fLogDisabled = fFalse;
    m_fAbruptEnd = fFalse;

    
    CallJ( ErrLGInit( &fNewCheckpointFile ), TermFMP );

    rgszT[0] = m_wszRestorePath;
    rgszT[1] = wszLogDirPath;
    UtilReportEvent(
            eventInformation,
            LOGGING_RECOVERY_CATEGORY,
            START_RESTORE_ID,
            2,
            rgszT,
            0,
            NULL,
            m_pinst );

    m_lgenInitial = m_lgenInitial > lgenLow ? lgenLow : m_lgenInitial;

    
    Call( ErrLGIRSTSetupCheckpoint( lgenLow, lgenHigh, NULL ) );

    m_lGenLowRestore = lgenLow;
    m_lGenHighRestore = lgenHigh;

    
    m_pinst->m_pfnInitCallback = pfn;
    m_pinst->m_pvInitCallbackContext = pvContext;
    if ( m_pinst->m_pfnInitCallback != NULL )
    {
        plgstat = &lgstat;
        LGIRSTPrepareCallback( plgstat, lgenHigh, lgenLow, 0 );
    }
    else
    {
        Assert( NULL == plgstat );
        memset( &lgstat, 0, sizeof(lgstat) );
    }
    Assert( m_wszLogCurrent == m_wszRestorePath );

    Call( ErrBuildRstmapForRestore( m_wszRestorePath ) );

    
    for ( INT irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
    {
        
        WCHAR *wszNewDatabaseName = m_rgrstmap[irstmap].wszNewDatabaseName;

        if ( !wszNewDatabaseName )
            continue;

        Assert( m_fSignLogSet );
        Call( ErrLGCheckDBFiles( m_pinst, m_rgrstmap + irstmap, lgenLow, lgenHigh ) );
    }

    Assert ( FRstmapCheckDuplicateSignature() );

    
    Assert( m_fExternalRestore == fFalse );

    
    Assert( m_wszLogCurrent == m_wszRestorePath );
    m_errGlobalRedoError = JET_errSuccess;
    Call( ErrLGRRedo( fFalse, m_pcheckpoint, plgstat ) );


    Call( ErrLGMoveToRunningState( err, NULL ) );


    Assert( m_fUseRecoveryLogFileSize == fFalse );


    Assert( m_pLogStream->CbSecVolume() != ~(ULONG)0 );


    cbSecVolumeSave = m_pLogStream->CbSecVolume();

    if ( plgstat )
    {
        lgstat.snprog.cunitDone = lgstat.snprog.cunitTotal;
        (lgstat.pfnCallback)( JET_snpRestore, JET_sntComplete, &lgstat.snprog, lgstat.pvCallbackContext );
    }

HandleError:

    
    FreeRstmap( );

    if ( err < 0  &&  m_pinst->m_fSTInit != fSTInitNotDone )
    {
        Assert( m_pinst->m_fSTInit == fSTInitDone );
        const ERR errT = m_pinst->ErrINSTTerm( termtypeError );
        Assert( errT == JET_errSuccess || m_pinst->m_fTermAbruptly );
    }

    CallS( ErrLGTerm( fFalse ) );

TermFMP:

    m_fHardRestore = fFalse;

    
    m_pinst->m_fSTInit = err >= JET_errSuccess ? fSTInitNotDone : fSTInitFailed;

    if ( err != JET_errSuccess && !FErrIsLogCorruption( err ) )
    {
        UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, RESTORE_DATABASE_FAIL_ID, err, m_pinst );
    }
    else
    {
        if ( g_fRepair && m_errGlobalRedoError != JET_errSuccess )
            err = ErrERRCheck( JET_errRecoveredWithErrors );
    }
    UtilReportEvent(
            eventInformation,
            LOGGING_RECOVERY_CATEGORY,
            STOP_RESTORE_ID,
            0,
            NULL,
            0,
            NULL,
            m_pinst );

    m_fSignLogSet = fFalse;

    m_fLogDisabled = fLogDisabledSav;

ReturnError:
    m_pLogStream->SetCbSecVolume( cbSecVolumeSave );
    m_fUseRecoveryLogFileSize = fFalse;

ResetGlobalRepair:
    g_fRepair = fGlobalRepairSave;
    return err;
}

ERR ISAMAPI ErrIsamRestore(
    JET_INSTANCE jinst,
    __in JET_PCWSTR wszBackup,
    __in JET_PCWSTR wszDest,
    JET_PFNINITCALLBACK pfnStatus,
    void * pvStatusContext )
{
    ERR         err;
    INST        *pinst = (INST *)jinst;

    err = pinst->m_plog->ErrLGRestore( wszBackup, wszDest, pfnStatus, pvStatusContext );

    Assert( pinst );

    return err;
}

ERR ISAMAPI ErrIsamExternalRestore(
    JET_INSTANCE jinst,
    __in PCWSTR wszCheckpointFilePath,
    __in PCWSTR wszNewLogPath,
    __in_ecount_opt(cjrstmap) JET_RSTMAP_W *rgjrstmap,
    INT cjrstmap,
    __in PCWSTR wszBackupLogPath,
    LONG lgenLow,
    LONG lgenHigh,
    __in PCWSTR wszTargetLogPath,
    LONG lGenHighTarget,
    JET_PFNINITCALLBACK pfn,
    void * pvContext )
{
    ERR err;
    INST *pinst = (INST *)jinst;

    Assert( wszBackupLogPath );

#ifdef DEBUG
    ITDBGSetConstants( pinst );
#endif

    Assert( pinst->m_fSTInit == fSTInitDone || pinst->m_fSTInit == fSTInitNotDone );
    if ( pinst->m_fSTInit == fSTInitDone )
    {
        return ErrERRCheck( JET_errAfterInitialization );
    }

    LOG *plog = pinst->m_plog;

    err = plog->ErrLGRSTExternalRestore(
            wszCheckpointFilePath,
            wszNewLogPath,
            rgjrstmap,
            cjrstmap,
            wszBackupLogPath,
            lgenLow,
            lgenHigh,
            wszTargetLogPath,
            lGenHighTarget,
            pfn,
            pvContext );

    return err;
}

VOID DBGBRTrace( __in PCSTR sz );

ERR LOG::ErrLGRSTExternalRestore(
    __in PCWSTR                 wszCheckpointFilePath,
    __in PCWSTR                 wszNewLogPath,
    JET_RSTMAP_W                *rgjrstmap,
    INT                         cjrstmap,
    __in PCWSTR                 wszBackupLogPath,
    LONG                        lgenLow,
    LONG                        lgenHigh,
    __in PCWSTR                 wszTargetLogPath,
    LONG                        lGenHighTarget,
    JET_PFNINITCALLBACK         pfn,
    void *                      pvContext )
{
    ERR                 err;
    BOOL                fLogDisabledSav;
    DBMS_PARAM          dbms_param;
    LGSTATUSINFO        lgstat = { 0 };
    LGSTATUSINFO        *plgstat = NULL;
    const WCHAR         *rgszT[2];
    INT                 irstmap;
    BOOL                fNewCheckpointFile;
    ULONG               cbSecVolumeSave;

    TraceContextScope tcScope( iortBackupRestore );
    tcScope->nParentObjectClass = tceNone;

    OSStrCbCopyW( m_wszTargetInstanceLogPath , sizeof(m_wszTargetInstanceLogPath), wszTargetLogPath );
    m_lGenHighTargetInstance = lGenHighTarget;

    {
        INST * pinst = m_pinst;


        Assert( !FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramTempPath ) ) );
        Assert( FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramSystemPath ) ) );
        Assert( FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramLogFilePath ) ) );


        CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, SzParam( pinst, JET_paramTempPath ), NULL, 0 ) );
        CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, SzParam( pinst, JET_paramSystemPath ), NULL, 0 ) );
        CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, SzParam( pinst, JET_paramLogFilePath ), NULL, 0 ) );
    }


    m_fSignLogSet = fFalse;

    
    WCHAR wszFullName[IFileSystemAPI::cchPathMax];
    CallR( ErrLGIRSTInitPath( wszBackupLogPath, wszNewLogPath, m_wszRestorePath, sizeof(m_wszRestorePath), wszFullName, sizeof(wszFullName) ) );
    CallR( Param( m_pinst, JET_paramLogFilePath )->Set( m_pinst, ppibNil, 0, wszFullName ) );

    Assert( sizeof( WCHAR ) * ( LOSStrLengthW( m_wszRestorePath ) + 1 ) < sizeof( m_wszRestorePath ) );
    Assert( LOSStrLengthW( SzParam( m_pinst, JET_paramLogFilePath ) ) < IFileSystemAPI::cchPathMax );
    Assert( m_wszLogCurrent == m_wszRestorePath );


{
    ULONG cbSecVolumeNew;
    if ( m_pinst->m_pfsapi->ErrPathComplete( SzParam( m_pinst, JET_paramLogFilePath ), wszFullName ) == JET_errInvalidPath )
    {
        const WCHAR *szPathT[1] = { SzParam( m_pinst, JET_paramLogFilePath ) };
        UtilReportEvent(
                eventError,
                LOGGING_RECOVERY_CATEGORY,
                FILE_NOT_FOUND_ERROR_ID,
                1,
                szPathT,
                0,
                NULL,
                m_pinst );
        return ErrERRCheck( JET_errFileNotFound );
    }

    CallR( m_pinst->m_pfsapi->ErrFileAtomicWriteSize( wszFullName, &cbSecVolumeNew ) );
    m_pLogStream->SetCbSecVolume( cbSecVolumeNew );
}
    cbSecVolumeSave = m_pLogStream->CbSecVolume();


    Assert( m_fUseRecoveryLogFileSize == fFalse );
    m_fUseRecoveryLogFileSize = fTrue;

    

    Assert ( 0 == m_lGenHighTargetInstance || m_wszTargetInstanceLogPath[0] );

    if ( m_lGenHighTargetInstance )
    {
        CallJ( ErrLGIRSTCheckSignaturesLogSequence(
            m_wszRestorePath, SzParam( m_pinst, JET_paramLogFilePath ), lgenLow, lgenHigh, m_wszTargetInstanceLogPath, m_lGenHighTargetInstance ), ReturnError );
    }
    else
    {
        CallJ( ErrLGIRSTCheckSignaturesLogSequence(
            m_wszRestorePath, SzParam( m_pinst, JET_paramLogFilePath ), lgenLow, lgenHigh, NULL, 0 ), ReturnError );
    }

    fLogDisabledSav = m_fLogDisabled;
    m_pinst->SaveDBMSParams( &dbms_param );

    m_fHardRestore = fTrue;
    m_fLogDisabled = fFalse;
    m_fAbruptEnd = fFalse;

    CallJ( ErrBuildRstmapForExternalRestore( rgjrstmap, cjrstmap ), TermResetGlobals );

    

    for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
    {
        
        LGPOS lgposSnapshotDb = lgposMin;
        Assert( m_fSignLogSet );

        CallJ( ErrLGCheckDBFiles( m_pinst, m_rgrstmap + irstmap, lgenLow, lgenHigh, &lgposSnapshotDb ), TermFreeRstmap );
    }

    Assert ( FRstmapCheckDuplicateSignature() );



    CallJ( ErrLGInit( &fNewCheckpointFile ), TermFMP );

    rgszT[0] = m_wszRestorePath;
    rgszT[1] = SzParam( m_pinst, JET_paramLogFilePath );
    UtilReportEvent(
            eventInformation,
            LOGGING_RECOVERY_CATEGORY,
            START_RESTORE_ID,
            2,
            rgszT,
            0,
            NULL,
            m_pinst );

#ifdef DEBUG
    if ( m_fDBGTraceBR )
    {
        CHAR sz[256];

        OSStrCbFormatA( sz, sizeof(sz), "** Begin ExternalRestore:\n" );
        Assert( strlen( sz ) <= sizeof( sz ) - 1 );
        DBGBRTrace( sz );

        if ( wszCheckpointFilePath )
        {
            OSStrCbFormatA( sz, sizeof(sz), "     CheckpointFilePath: %ws\n", wszCheckpointFilePath );
            Assert( strlen( sz ) <= sizeof( sz ) - 1 );
            DBGBRTrace( sz );
        }
        if ( wszNewLogPath )
        {
            OSStrCbFormatA( sz, sizeof(sz), "     LogPath: %ws\n", wszNewLogPath );
            Assert( strlen( sz ) <= sizeof( sz ) - 1 );
            DBGBRTrace( sz );
        }
        if ( wszBackupLogPath )
        {
            OSStrCbFormatA( sz, sizeof(sz), "     BackupLogPath: %ws\n", wszBackupLogPath );
            Assert( strlen( sz ) <= sizeof( sz ) - 1 );
            DBGBRTrace( sz );
        }
        OSStrCbFormatA( sz, sizeof(sz), "     Generation number: %d - %d\n", lgenLow, lgenHigh );
        Assert( strlen( sz ) <= sizeof( sz ) - 1 );
        DBGBRTrace( sz );

        if ( m_irstmapMac )
        {
            for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
            {
                RSTMAP *prstmap = m_rgrstmap + irstmap;

                OSStrCbFormatA( sz, sizeof(sz), "     %ws --> %ws\n", prstmap->wszDatabaseName, prstmap->wszNewDatabaseName );
                Assert( strlen( sz ) <= sizeof( sz ) - 1 );
                DBGBRTrace( sz );
            }
        }
    }
#endif

    
    Call ( m_pLogStream->ErrLGGetGenerationRange( m_wszRestorePath,
            !lgenLow?&lgenLow:NULL,
            !lgenHigh?&lgenHigh:NULL ) );

    m_lgenInitial = m_lgenInitial > lgenLow ? lgenLow : m_lgenInitial;

    Call( ErrLGIRSTSetupCheckpoint( lgenLow, lgenHigh, wszCheckpointFilePath ) );

    m_lGenLowRestore = lgenLow;
    m_lGenHighRestore = lgenHigh;

    
    m_pinst->m_pfnInitCallback = pfn;
    m_pinst->m_pvInitCallbackContext = pvContext;
    if ( m_pinst->m_pfnInitCallback != NULL )
    {
        plgstat = &lgstat;
        LGIRSTPrepareCallback( plgstat, lgenHigh, lgenLow, 0 );
    }
    else
    {
        Assert( NULL == plgstat );
        memset( &lgstat, 0, sizeof(lgstat) );
    }

    
    m_fExternalRestore = fTrue;

    m_fReplayMissingMapEntryDB = fFalse;
    m_fReplayingIgnoreMissingDB = fTrue;

    
    Assert( m_wszLogCurrent == m_wszRestorePath );
    m_errGlobalRedoError = JET_errSuccess;
    Call( ErrLGRRedo( fFalse, m_pcheckpoint, plgstat ) );


    Call( ErrLGMoveToRunningState( err, NULL ) );


    Assert( m_fUseRecoveryLogFileSize == fFalse );


    Assert( m_pLogStream->CbSecVolume() != ~(ULONG)0 );


    cbSecVolumeSave = m_pLogStream->CbSecVolume();

    
    if ( plgstat )
    {
        
        lgstat.snprog.cunitDone = lgstat.snprog.cunitTotal;
        (lgstat.pfnCallback)( JET_snpRestore, JET_sntComplete, &lgstat.snprog, lgstat.pvCallbackContext );
    }

HandleError:
    
    Assert( err < 0 || m_pinst->m_fSTInit == fSTInitNotDone );
    if ( err < 0  &&  m_pinst->m_fSTInit != fSTInitNotDone )
    {
        Assert( m_pinst->m_fSTInit == fSTInitDone );
        const ERR errT = m_pinst->ErrINSTTerm( termtypeError );
        Assert( errT == JET_errSuccess || m_pinst->m_fTermAbruptly );
    }

    CallS( ErrLGTerm( fFalse ) );

    if ( err >= 0 )
    {
        WCHAR   wszFullLogPath[IFileSystemAPI::cchPathMax];
        WCHAR   wszFullTargetPath[IFileSystemAPI::cchPathMax];

        Assert ( 0 == m_lGenHighTargetInstance || m_wszTargetInstanceLogPath[0] );
        Assert ( SzParam( m_pinst, JET_paramLogFilePath ) );

        CallS( m_pinst->m_pfsapi->ErrPathComplete( m_wszTargetInstanceLogPath, wszFullTargetPath ) );
        CallS( m_pinst->m_pfsapi->ErrPathComplete( SzParam( m_pinst, JET_paramLogFilePath ), wszFullLogPath ) );

        Assert ( 0 == m_lGenHighTargetInstance || (0 != UtilCmpFileName( wszFullTargetPath, wszFullLogPath ) ) );
        if ( m_lGenHighTargetInstance && ( 0 != UtilCmpFileName( wszFullTargetPath, wszFullLogPath ) ) )
        {

            CallSx( m_pLogStream->ErrLGDeleteLegacyReserveLogFiles(), JET_errFileNotFound );
        }
    }

TermFMP:

TermFreeRstmap:
    FreeRstmap( );

TermResetGlobals:
    m_fHardRestore = fFalse;

    m_wszTargetInstanceLogPath[0] = L'\0';

    
    m_pinst->m_fSTInit = ( err >= 0 ) ? fSTInitNotDone : fSTInitFailed;

    if ( err != JET_errSuccess && !FErrIsLogCorruption( err ) )
    {
        UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, RESTORE_DATABASE_FAIL_ID, err, m_pinst );
    }
    else
    {
        if ( g_fRepair && m_errGlobalRedoError != JET_errSuccess )
            err = ErrERRCheck( JET_errRecoveredWithErrors );
    }
    UtilReportEvent(
            eventInformation,
            LOGGING_RECOVERY_CATEGORY,
            STOP_RESTORE_ID,
            0,
            NULL,
            0,
            NULL,
            m_pinst );

    if ( m_lGenHighTargetInstance && JET_errSuccess <= err )
    {
        err = ErrERRCheck( JET_wrnTargetInstanceRunning );
    }
    m_lGenHighTargetInstance = 0;

    m_fSignLogSet = fFalse;

    m_fLogDisabled = fLogDisabledSav;
    m_pinst->RestoreDBMSParams( &dbms_param );

    m_fExternalRestore = fFalse;

ReturnError:
    m_pLogStream->SetCbSecVolume( cbSecVolumeSave );
    m_fUseRecoveryLogFileSize = fFalse;
    return err;
}

ERR LOG::ErrCompareLogs( PCWSTR wszLog1, PCWSTR wszLog2, const BOOL fAllowSubsetIfEmpty, BOOL* const pfLogsDiverged )
{
    ERR         err         = JET_errSuccess;
    IFileAPI*   pfapiLog1   = NULL;
    IFileAPI*   pfapiLog2   = NULL;

    TraceContextScope tcScope( iorpLog );

    void* pvLog1 = NULL;
    void* pvLog2 = NULL;

    QWORD cbLog1        = 0;
    QWORD cbLog2        = 0;
    QWORD cbRemaining   = 0;
    QWORD cbRead        = 0;
    QWORD cbMaxRead     = 2 * 1024 * 1024;

    BOOL fCloseNormally     = fFalse;    
    const BOOL fRecoveringSaved   = m_fRecovering;
    const RECOVERING_MODE rmSaved = m_fRecoveringMode;

    *pfLogsDiverged = fFalse;

    Call( CIOFilePerf::ErrFileOpen( m_pinst->m_pfsapi, m_pinst, wszLog1, IFileAPI::fmfReadOnly, iofileLog, qwDumpingFileID, &pfapiLog1 ) );
    Call( CIOFilePerf::ErrFileOpen( m_pinst->m_pfsapi, m_pinst, wszLog2, IFileAPI::fmfReadOnly, iofileLog, qwDumpingFileID, &pfapiLog2 ) );
    Call( pfapiLog1->ErrSize( &cbLog1, IFileAPI::filesizeLogical ) );
    Call( pfapiLog2->ErrSize( &cbLog2, IFileAPI::filesizeLogical ) );

    if ( cbLog1 != cbLog2 )
    {
        *pfLogsDiverged = fTrue;
    }    

    if ( fAllowSubsetIfEmpty )
    {
        m_fRecovering = fTrue;
        m_fRecoveringMode = fRecoveringRedo;

        Call( m_pLogStream->ErrLGInit() );

        m_pLogStream->SetPfapi( pfapiLog1 );
        Call( m_pLogStream->ErrLGReadFileHdr( NULL, iorpDirectAccessUtil, NULL, fNoCheckLogID, fTrue ) );
        Call( m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( &fCloseNormally, fTrue, fTrue ) );

        cbLog1 = m_pLogReadBuffer->LgposFileEnd().isec * m_pLogStream->CbSec();

        m_pLogStream->ResetPfapi();
        m_pLogStream->SetPfapi( pfapiLog2 );
        Call( m_pLogStream->ErrLGReadFileHdr( NULL, iorpDirectAccessUtil, NULL, fNoCheckLogID, fTrue ) );
        Call( m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( &fCloseNormally, fTrue, fTrue ) );

        cbLog2 = m_pLogReadBuffer->LgposFileEnd().isec * m_pLogStream->CbSec();
    }

    cbLog1      = min( cbLog1, cbLog2 );
    cbRemaining = cbLog1;

    cbRead = min( cbMaxRead, cbRemaining );
    pvLog1 = PvOSMemoryPageAlloc( cbRead, NULL );
    pvLog2 = PvOSMemoryPageAlloc( cbRead, NULL );
        
    while ( cbRemaining > 0 )
    {
        cbRead = min( cbMaxRead, cbRemaining );
        Call( pfapiLog1->ErrIORead( *tcScope, cbLog1 - cbRemaining, (DWORD) cbRead, (BYTE*) pvLog1, QosSyncDefault( pinstNil ) ) );
        Call( pfapiLog2->ErrIORead( *tcScope, cbLog1 - cbRemaining, (DWORD) cbRead, (BYTE*) pvLog2, QosSyncDefault( pinstNil ) ) );

        if ( memcmp( pvLog1, pvLog2, cbRead ) != 0 )
        {
            *pfLogsDiverged = fTrue;
            break;
        }

        cbRemaining -= cbRead;
    }    

HandleError:
    m_pLogStream->ResetPfapi();
    m_fRecovering = fRecoveringSaved;
    m_fRecoveringMode = rmSaved;

    if ( pfapiLog1 )
    {
        delete pfapiLog1;
    }

    if ( pfapiLog2 )
    {
        delete pfapiLog2;
    }

    if ( pvLog1 )
    {
        OSMemoryPageFree( pvLog1 );
    }

    if ( pvLog2 )
    {
        OSMemoryPageFree( pvLog2 );
    }

    return err;
}

ERR LGFileHelper::ErrLGGetGeneration( IFileSystemAPI* const pfsapi, __in PCWSTR wszFileName, __in PCWSTR wszBaseName, LONG* plgen, __in PCWSTR const wszLogExt, ULONG * pcchLogDigits )
{
    ERR     err = JET_errSuccess;
    WCHAR   wszFNameT[IFileSystemAPI::cchPathMax];
    WCHAR   wszExtT[IFileSystemAPI::cchPathMax];

    Assert( pfsapi );
    Assert( wszFileName );
    Assert( plgen );
    Assert( pcchLogDigits );

    *plgen = 0;

    
    err = pfsapi->ErrPathParse( wszFileName, NULL, wszFNameT, wszExtT );
    CallS( err );

    
    if ( UtilCmpFileName( wszExtT, wszLogExt ) )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }

    
    if ( _wcsnicmp( wszFNameT, wszBaseName, wcslen( wszBaseName ) ) )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }

    const ULONG cLogDigitsSmall = 5;
    const ULONG cLogDigitsBig = 8;

    ULONG cLogDigits = LOSStrLengthW( wszFNameT );
    if ( cLogDigits < wcslen( wszBaseName ) )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }
    cLogDigits -= wcslen( wszBaseName );
    if ( ( cLogDigits != cLogDigitsSmall &&
          cLogDigits != cLogDigitsBig ) )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }
    const INT   ibMax = (cLogDigits + wcslen( wszBaseName ));

    INT         ib;
    LONG        lGen = 0;

    for (ib = wcslen( wszBaseName ); ib < ibMax; ib++ )
    {
        WCHAR   b = wszFNameT[ib];

        if ( b >= L'0' && b <= L'9' )
            lGen = lGen * 16 + b - L'0';
        else if ( b >= L'A' && b <= L'F' )
            lGen = lGen * 16 + b - L'A' + 10;
        else if ( b >= L'a' && b <= L'f' )
            lGen = lGen * 16 + b - L'a' + 10;
        else
            break;
    }

    if ( ib != ibMax )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }

    *plgen = lGen;
    *pcchLogDigits = cLogDigits;

Assert( !err );

HandleError:

    return(err);

}


