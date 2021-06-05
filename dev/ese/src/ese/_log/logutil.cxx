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

    /*  check if dbfilehdr of database and patchfile are the same.
    /**/
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

    // fill in the RSTMAP the found database signature
    memcpy ( &pDbMapEntry->signDatabase, &pdbfilehdrDb->signDb , sizeof(SIGNATURE) );

    // should be a db header or patch file header
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

    // with snapshot backup, we don't use a patch file
    // exit with no error
    if ( fFromSnapshotBackup )
    {
        Assert ( plgposSnapshotRestore );
        err = JET_errSuccess;

        if ( memcmp( &pdbfilehdrDb->signLog, &signLog, sizeof( SIGNATURE ) ) != 0 )
        {
            const UINT csz = 1;
            const WCHAR *rgszT[csz];

            rgszT[0] = wszDatabase;

            // UNDONE: change event message and error
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
        //  only way this is possible is if the database is not
        //  a backup database (eg. it's a clean-shutdown or
        //  dirty-shutdown database)
        //
        Error( ErrERRCheck( JET_errRestoreOfNonBackupDatabase ) );
    }

    // if we have already the trailer moved to the header
    // we are good to go
    if ( pdbfilehdrDb->bkinfoFullCur.le_genHigh )
    {
        ulGenHighFound = pdbfilehdrDb->bkinfoFullCur.le_genHigh;
        ulGenLowFound = pdbfilehdrDb->bkinfoFullCur.le_genLow;
        Assert( err == JET_errSuccess );
        goto EndOfCheckLogRange;
    }

    //  the patch file is always on the OS file-system
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
        /*  It should start at most from bkinfoFullCur.genLow
         */
        WCHAR   wszT1[IFileSystemAPI::cchPathMax];
        WCHAR   wszT2[IFileSystemAPI::cchPathMax];
        const WCHAR *rgszT[] = { wszT1, wszT2 };

        // use szPatch as this should be the path of the log files as well
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
        /*  It should be at least from bkinfoFullCur.genHigh
         */
        WCHAR   szT1[IFileSystemAPI::cchPathMax];
        WCHAR   szT2[IFileSystemAPI::cchPathMax];
        const WCHAR *rgszT[] = { szT1, szT2 };

        // use szPatch as this should be the path of the log files as well
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

    // we can update the database header and we don't need the trailer
    Assert ( ulGenLowFound == pdbfilehdrDb->bkinfoFullCur.le_genLow);
    Assert ( ulGenHighFound <= (ULONG)genHigh);

    // we update genMax with the max that we restored,
    // not just with was in the header file
    pdbfilehdrDb->bkinfoFullCur.le_genHigh = genHigh;

    if ( genHigh > pdbfilehdrDb->le_lGenMaxCommitted )
    {
        // We don't know what the logtime of the new required max is.
        memset( &pdbfilehdrDb->logtimeGenMaxCreate, 0, sizeof(LOGTIME) );
        pdbfilehdrDb->le_lGenMaxCommitted = genHigh;
    }

    if ( genHigh > pdbfilehdrDb->le_lGenMaxRequired )
    {
        // We don't know what the logtime of the new required max is.
        memset( &pdbfilehdrDb->logtimeGenMaxRequired, 0, sizeof(LOGTIME) );
        pdbfilehdrDb->le_lGenMaxRequired = genHigh;
    }

    //  initialize persisted flush map
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
//  ERR             wrn = JET_errSuccess;
    PCWSTR          wszLogExt = NULL;

    BOOL fTargetInstanceCheck = fFalse;
    const WCHAR * wszLogFilePathCheck = NULL;

    AllocR( plgfilehdrT = (LGFILEHDR *)PvOSMemoryPageAlloc( sizeof(LGFILEHDR) * 4, NULL ) );

    plgfilehdrCur[0] = plgfilehdrT;
    plgfilehdrCur[1] = plgfilehdrT + 1;
    plgfilehdrLow = plgfilehdrT + 2;
    plgfilehdrHigh = plgfilehdrT + 3;

    /*  starting from lowest generation of the restored path.
     *  Check the given logs are all correct and contiguous
     */
    for ( gen = genLow; gen <= genHigh; gen++ )
    {
        Expected( gen > 0 );
        ilgfilehdrCur = ilgfilehdrAvail++ % 2;
        ilgfilehdrPrv = ilgfilehdrAvail % 2;

        // Restore needs to auto-switch to the appropriate log files extension ...
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
                // Lets fall back and try the other log extension ...
                if ( pfapiT != NULL ) // just in case ?
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
                // Found an acceptable log extension ...
                LGSetChkExts( FLGIsLegacyExt( fFalse, wszLogExt), fFalse );
                m_pLogStream->LGSetLogExt( FLGIsLegacyExt( fFalse, wszLogExt), fFalse );
            }
        }
        Call( err ); // return error if any ...
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
        // I can't see how this path can be taken after the loop ...
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

    // we are going to check the log sequence from the backup set ()
    // and szLogFilePath or szTargetInstanceFilePath (if used)

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

    /*  if Restore path and log path is different, delete all the unrelated log files
     *  in the restore log path.
     */
{
    WCHAR   wszFullLogPath[IFileSystemAPI::cchPathMax];
    WCHAR   wszFullLogFilePathCheck[IFileSystemAPI::cchPathMax];

    Call( m_pinst->m_pfsapi->ErrPathComplete( wszRestorePath, wszFullLogPath ) );
    Call( m_pinst->m_pfsapi->ErrPathComplete( wszLogFilePathCheck, wszFullLogFilePathCheck ) );

#ifdef DEBUG
    // TargetInstance, the TargetInstance should be different the recover instance directory
    if ( wszTargetInstanceFilePath )
    {
        Assert ( wszLogFilePathCheck == wszTargetInstanceFilePath );

        CallS ( m_pinst->m_pfsapi->ErrPathComplete( wszLogFilePath, wszFullLogPath ) );
        Assert ( UtilCmpFileName( wszFullLogPath, wszFullLogFilePathCheck ) );
    }
#endif // DEBUG

    Call( m_pinst->m_pfsapi->ErrPathComplete( wszRestorePath, wszFullLogPath ) );

    if ( UtilCmpFileName( wszFullLogPath, wszFullLogFilePathCheck ) != 0 &&
        ( JET_errSuccess == m_pLogStream->ErrLGGetGenerationRange( wszRestorePath, &genLowT, &genHighT ) ) )
    {
        m_pLogStream->LGRSTDeleteLogs( wszRestorePath, genLowT, genLow - 1, fFalse );
        m_pLogStream->LGRSTDeleteLogs( wszRestorePath, genHigh + 1, genHighT, fTrue );
    }
}

    /*  Check the log directory. Make sure all the log files has the same signature.
     */
    Assert( m_pLogStream->LogExt() );
    Call ( m_pLogStream->ErrLGGetGenerationRange( wszLogFilePathCheck, &genLowT, &genHighT ) );

    if ( fTargetInstanceCheck )
    {
        Assert ( genHighTarget );
        Assert ( genHighT >= genHighTarget );
        Assert ( genLowT <= genHighTarget );


        if ( genHighT < genHighTarget || genLowT > genHighTarget || 0 == genLowT )
        {
            //  Someone delete the logs since last time we checked the
            //  restore log files.

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

        // set the max log to check to genHighTarget
        genHighT = genHighTarget;
    }

    /*  genHighT + 1 implies JetLog file (edb.jtx/log).
     */
    if ( genLowT > genHigh )
        fReadyToCheckContiguity = fTrue;
    else
        fReadyToCheckContiguity = fFalse;

    for ( gen = genLowT; gen <= ( fTargetInstanceCheck ? genHighT : genHighT + 1 ); gen++ )
    {
        if ( gen == 0 )
        {

            Assert ( !fTargetInstanceCheck );

            /*  A special case. Check if JETLog(edb.jtx/log) exist?
             */
            if ( m_pLogStream->ErrLGRSTOpenLogFile( wszLogFilePathCheck, lGenSignalCurrentID, &pfapiT ) < 0 )
                break;

            /*  Set break condition. Also set condition to check if
             *  the log is contiguous from the restore logs ( genHigh + 1 )
             */
            gen = genHigh + 1;
            genHighT = genHigh;
            Assert( gen == genHighT + 1 );
        }
        else
        {
            if ( gen == genHighT + 1 )
            {
                Assert ( !fTargetInstanceCheck );

                /*  A special case. Check if JETLog(edb.jtx/log) exist?
                 */
                if ( m_pLogStream->ErrLGRSTOpenLogFile( wszLogFilePathCheck, lGenSignalCurrentID, &pfapiT ) < 0 )
                    break;
            }
            else
            {
                Assert( gen > 0 );
                err = m_pLogStream->ErrLGRSTOpenLogFile( wszLogFilePathCheck, gen, &pfapiT );
                if ( err == JET_errFileNotFound )
                {
                    // The expected log generation is missing...
                    WCHAR wszT[IFileSystemAPI::cchPathMax];
                    const UINT  csz = 1;
                    const WCHAR *rgszT[csz];

                    m_pLogStream->LGFullLogNameFromLogId( wszT, gen, wszLogFilePathCheck );

                    rgszT[0] = wszT;
                    if ( gen <= genHigh )
                    {
                        if ( _wcsicmp( wszRestorePath, wszLogFilePathCheck ) != 0 )
                        {
                            //  skip all the logs that can be found in
                            //  szRestorePath. Continue from genHigh+1
                            gen = genHigh;
                            continue;
                        }

                        //  Someone delete the logs since last time we checked the
                        //  restore log files.

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
                genCurrent = genHighT + 1;  // to break out the loop
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

//              wrn = ErrERRCheck( JET_wrnExistingLogFileIsNotContiguous );
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
            /*  make sure it and the restore log are contiguous. If not, then delete
             *  all the logs up to genLow - 1.
             */
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
            /*  make sure it and the restore log are the same. If not, then delete
             *  all the logs up to genHigh.
             */
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
            /*  make sure it and the restore log are contiguous. If not, then delete
             *  all the logs higher than genHigh.
             */
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
//  if ( err == JET_errSuccess )
//      err = wrn;

    delete pfapiT;

    OSMemoryPageFree( plgfilehdrT );

    return err;
}


/*
 *  Restores databases from database backups and log generations.  Redoes
 *  log from latest checkpoint record. After the backed-up logfile is
 *  Restored, the initialization process continues with Redo of the current
 *  logfile as long as the generation numbers are contiguous. There must be a
 *  log file szJetLog in the backup directory, else the Restore process fails.
 *
 *  GLOBAL PARAMETERS
 *      m_wszRestorePath (IN)   pathname of the directory with backed-up files.
 *      lgposRedoFrom(OUT)  is the position (generation, logsec, displacement)
 *                          of the last saved log record; Redo of the
 *                          current logfile will continue from this point.
 *
 *  RETURNS
 *      JET_errSuccess, or error code from failing routine, or one
 *              of the following "local" errors:
 *              -AfterInitialization
 *              -errFailRestoreDatabase
 *              -errNoRestoredDatabases
 *              -errMissingJetLog
 *  FAILS ON
 *      missing szJetLog or System.mdb on backup directory
 *      noncontiguous log generation
 *
 *  SIDE EFFECTS:
 *      All databases may be changed.
 *
 *  COMMENTS
 *      this call is executed during the normal first JetInit call,
 *      if the environment variable RESTORE is set. Subsequent to
 *      the successful execution of Restore,
 *      system operation continues normally.
 */
/*  initialize log path, restore log path, and check its continuity
/**/
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


/*  log restore checkpoint setup
/**/
ERR LOG::ErrLGIRSTSetupCheckpoint(
    LONG lgenLow,
    LONG lgenHigh,
    __in PCWSTR wszCurCheckpoint )
{
    ERR         err;
    WCHAR       wszT[IFileSystemAPI::cchPathMax];
    LGPOS       lgposCheckpoint;

    //  UNDONE: optimize to start at backup checkpoint

    /*  Set up *checkpoint* and related *system parameters*.
     *  Read checkpoint file in backup directory. If does not exist, make checkpoint
     *  as the oldest log files. Also set dbms_paramT as the parameter for the redo
     *  point.
     */

    /*  redo backeup logfiles beginning with first gen log file.
    /**/
    Call( m_pLogStream->ErrLGMakeLogNameBaseless( wszT, sizeof(wszT), m_wszRestorePath, eArchiveLog, lgenLow ) );
    Assert( LOSStrLengthW( wszT ) * sizeof( WCHAR ) <= sizeof( wszT ) - 1 );
    Call( m_pLogStream->ErrLGOpenFile( wszT ) );

    /*  read log file header
    /**/
    Call( m_pLogStream->ErrLGReadFileHdr( NULL, iorpRestore, NULL, fCheckLogID ) );
    m_pcheckpoint->checkpoint.dbms_param = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.dbms_param;

    lgposCheckpoint.lGeneration = lgenLow;
    lgposCheckpoint.isec = (WORD) m_pLogStream->CSecHeader();
    lgposCheckpoint.ib = 0;
    m_pcheckpoint->checkpoint.le_lgposCheckpoint = lgposCheckpoint;
    m_pcheckpoint->checkpoint.le_lgposDbConsistency = lgposCheckpoint;

    Assert( sizeof( m_pcheckpoint->rgbAttach ) == cbAttach );
    UtilMemCpy( m_pcheckpoint->rgbAttach, m_pLogStream->GetCurrentFileHdr()->rgbAttach, cbAttach );

    /*  delete the old checkpoint file
    /**/
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

#define cRestoreStatusPadding   0.10    // Padding to add to account for DB copy.

VOID LOG::LGIRSTPrepareCallback(
    LGSTATUSINFO *              plgstat,
    LONG                        lgenHigh,
    LONG                        lgenLow,
    const LONG                  lgenHighStop
    )
{
    /*  get last generation in log dir directory.  Compare with last generation
    /*  in restore directory.  Take the higher.
    /**/
    //  UNDONE:  I'm pretty sure that the log path always has a value so we should remove this test
    if ( SzParam( m_pinst, JET_paramLogFilePath ) && *SzParam( m_pinst, JET_paramLogFilePath ) != '\0' )
    {
        LONG    lgenHighT;
        WCHAR   wszFNameT[IFileSystemAPI::cchPathMax];

        /*  check if it is needed to continue the log files in current
        /*  log working directory.
        /**/
        (void)m_pLogStream->ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), NULL, &lgenHighT );

        /*  check if edb.jtx/log exist, if it is, then add one more generation.
        /**/

        Assert( LOSStrLengthW( SzParam( m_pinst, JET_paramLogFilePath ) ) + LOSStrLengthW( SzParam( m_pinst, JET_paramBaseName ) ) + LOSStrLengthW( m_pLogStream->LogExt() ) < IFileSystemAPI::cchPathMax - 1 );
        // hmmm, interesting making relative name, do we need to do that?
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

    /*  If the number of generations is less than about 67%, then count sectors,
    /*  otherwise, just count generations.  We set the threshold at 67% because
    /*  this equates to about 1.5% per generation.  Any percentage higher than
    /*  this (meaning fewer generations) and we count sectors.  Any percentage
    /*  lower than this (meaning more generations) and we just count generations.
    /**/
    plgstat->fCountingSectors = (plgstat->cGensExpected <
            // SOFT_HARD: this is "JetRestore" check
            (ULONG)((100 - ( m_fHardRestore ? (cRestoreStatusPadding * 100) : 0 ) ) * 2/3));

    /*  Granularity of status callback is 1%.
    /*  Assume we callback after every generation.  If there are 67
    /*  callbacks, this equates to 1.5% per generation.  This seems like a
    /*  good cutoff value.  So, if there are 67 callbacks or more, count
    /*  generations.  Otherwise, count bytes per generation.
    /**/
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
    // LOG doesn't know TCE during replay
    tcScope->nParentObjectClass = tceNone;

    Assert( !g_fRepair );

    if ( _wcsicmp( SzParam( m_pinst, JET_paramRecovery ), L"repair" ) == 0 )
    {
        // If JET_paramRecovery is exactly "repair", then enable logging.  If anything
        // follows "repair", then disable logging.
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

    //  disable the sector-size checks

//
//  SEARCH-STRING: SecSizeMismatchFixMe
//
//  ---** TEMPORARY FIX **---
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
//
//  SEARCH-STRING: SecSizeMismatchFixMe
//
//  m_pLogStream->CbSecVolume() = ~(ULONG)0;

    //  use the right log file size for restore

    Assert( m_fUseRecoveryLogFileSize == fFalse );
    m_fUseRecoveryLogFileSize = fTrue;

    CallJ( ErrLGIRSTInitPath( wszBackupPath, SzParam( m_pinst, JET_paramLogFilePath ), m_wszRestorePath, sizeof(m_wszRestorePath), wszLogDirPath, sizeof(wszLogDirPath) ), ReturnError );
    // Recover from the case someone set the wrong extension ...
    CallJ ( m_pLogStream->ErrLGGetGenerationRange( m_wszRestorePath, &lgenLow, &lgenHigh, fTrue ), ReturnError );
    Assert( ( lgenLow > 0 && lgenHigh >= lgenLow ) || ( 0 == lgenLow && 0 == lgenHigh ) );

    if ( 0 == lgenLow || 0 == lgenHigh )
    {

        //  we didn't find anything in the given restore path
        //  check the wszAtomicNew subdirectory (what is this???)

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

        //  we didn't find anything in the NEW restore path
        //  check the szAtomicOld subdirectory (what is this???)

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

        //  we didn't find any log file anywhere -- error out

        err = ErrERRCheck( JET_errFileNotFound );
        goto ReturnError;
    }

    Assert( lgenLow > 0 );
    Assert( lgenHigh >= lgenLow );
    CallJ( ErrLGIRSTCheckSignaturesLogSequence( m_wszRestorePath, wszLogDirPath, lgenLow, lgenHigh, NULL, 0 ), ReturnError );

    Assert( ( LOSStrLengthW( m_wszRestorePath ) + 1 ) * sizeof( WCHAR ) < sizeof( m_wszRestorePath ) );
    Assert( ( LOSStrLengthW( wszLogDirPath ) + 1 ) * sizeof( WCHAR ) < sizeof( wszLogDirPath ) );
    Assert( m_wszLogCurrent == m_wszRestorePath );

//  CallR( FMP::ErrFMPInit() );

    fLogDisabledSav = m_fLogDisabled;
    // SOFT_HARD: we might want to set a different "JetRestore" flag in the future
    m_fHardRestore = fTrue;
    m_fLogDisabled = fFalse;
    m_fAbruptEnd = fFalse;

    /*  initialize log manager and set working log file path
    /**/
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

    // We could be restoring from a backup that started before the test initial log generation.
    m_lgenInitial = m_lgenInitial > lgenLow ? lgenLow : m_lgenInitial;

    /*  all saved log generation files, database backups
    /*  must be in m_wszRestorePath.
    /**/
    Call( ErrLGIRSTSetupCheckpoint( lgenLow, lgenHigh, NULL ) );

    m_lGenLowRestore = lgenLow;
    m_lGenHighRestore = lgenHigh;

    /*  prepare for callbacks
    /**/
    m_pinst->m_pfnInitCallback = pfn;
    m_pinst->m_pvInitCallbackContext = pvContext;
    if ( m_pinst->m_pfnInitCallback != NULL )
    {
        plgstat = &lgstat;
        LGIRSTPrepareCallback( plgstat, lgenHigh, lgenLow, 0 );
    }
    else
    {
        //  zero out to silence the compiler
        //
        Assert( NULL == plgstat );
        memset( &lgstat, 0, sizeof(lgstat) );
    }
    Assert( m_wszLogCurrent == m_wszRestorePath );

    Call( ErrBuildRstmapForRestore( m_wszRestorePath ) );

    /*  make sure all the patch files have enough logs to replay
    /**/
    for ( INT irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
    {
        /*  Open patch file and check its minimum requirement for full backup.
         */
        WCHAR *wszNewDatabaseName = m_rgrstmap[irstmap].wszNewDatabaseName;

        if ( !wszNewDatabaseName )
            continue;

        Assert( m_fSignLogSet );
        Call( ErrLGCheckDBFiles( m_pinst, m_rgrstmap + irstmap, lgenLow, lgenHigh ) );
    }

    // check that there are no databases with same signature
    Assert ( FRstmapCheckDuplicateSignature() );

    /*  adjust fmp according to the restore map.
    /**/
    Assert( m_fExternalRestore == fFalse );

    /*  do redo according to the checkpoint, dbms_params, and rgbAttach
    /*  set in checkpointGlobal.
    /**/
    Assert( m_wszLogCurrent == m_wszRestorePath );
    m_errGlobalRedoError = JET_errSuccess;
    Call( ErrLGRRedo( fFalse, m_pcheckpoint, plgstat ) );

    //  ErrLGRRedo() gets through the logs, but doesn't set the LOG() to run w/ the newly desired
    //  parameters ( log file size, log ext, etc )

    Call( ErrLGMoveToRunningState( err, NULL ) );

    //  we should be using the right log file size now

    Assert( m_fUseRecoveryLogFileSize == fFalse );

    //  sector-size checking should now be on

    Assert( m_pLogStream->CbSecVolume() != ~(ULONG)0 );

    //  update saved copy

    cbSecVolumeSave = m_pLogStream->CbSecVolume();

    if ( plgstat )
    {
        lgstat.snprog.cunitDone = lgstat.snprog.cunitTotal;     //lint !e644
        (lgstat.pfnCallback)( JET_snpRestore, JET_sntComplete, &lgstat.snprog, lgstat.pvCallbackContext );
    }

HandleError:

    /*  delete restore map
    /**/
    FreeRstmap( );

    if ( err < 0  &&  m_pinst->m_fSTInit != fSTInitNotDone )
    {
        Assert( m_pinst->m_fSTInit == fSTInitDone );
        const ERR errT = m_pinst->ErrINSTTerm( termtypeError );
        Assert( errT == JET_errSuccess || m_pinst->m_fTermAbruptly );
    }

//  CallS( ErrLGTerm( err >= JET_errSuccess ) );
    CallS( ErrLGTerm( fFalse ) );

TermFMP:
//  FMP::Term( );

    // SOFT_HARD: mirror the start of the function
    m_fHardRestore = fFalse;

    /*  reset initialization state
    /**/
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

//  Assert( szNewLogPath );
//  Assert( (rgjrstmap != NULL && cjrstmap > 0) || (rgjrstmap == NULL && cjrstmap == 0) );
    Assert( wszBackupLogPath );
//  Assert( lgenLow );
//  Assert( lgenHigh );

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
    // LOG doesn't know TCE during replay
    tcScope->nParentObjectClass = tceNone;

    OSStrCbCopyW( m_wszTargetInstanceLogPath , sizeof(m_wszTargetInstanceLogPath), wszTargetLogPath );
    m_lGenHighTargetInstance = lGenHighTarget;

    //  create paths now if they do not exist
    {
        INST * pinst = m_pinst;

        //  make sure the temp path does NOT have a trailing '\' and the log/sys paths do

        Assert( !FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramTempPath ) ) );
        Assert( FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramSystemPath ) ) );
        Assert( FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramLogFilePath ) ) );

        //  create paths

        CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, SzParam( pinst, JET_paramTempPath ), NULL, 0 ) );
        CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, SzParam( pinst, JET_paramSystemPath ), NULL, 0 ) );
        CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, SzParam( pinst, JET_paramLogFilePath ), NULL, 0 ) );
    }

    //  Start the restore work

    m_fSignLogSet = fFalse;

    /*  set restore path
    /**/
    WCHAR wszFullName[IFileSystemAPI::cchPathMax];
    CallR( ErrLGIRSTInitPath( wszBackupLogPath, wszNewLogPath, m_wszRestorePath, sizeof(m_wszRestorePath), wszFullName, sizeof(wszFullName) ) );
    CallR( Param( m_pinst, JET_paramLogFilePath )->Set( m_pinst, ppibNil, 0, wszFullName ) );

    Assert( sizeof( WCHAR ) * ( LOSStrLengthW( m_wszRestorePath ) + 1 ) < sizeof( m_wszRestorePath ) );
    Assert( LOSStrLengthW( SzParam( m_pinst, JET_paramLogFilePath ) ) < IFileSystemAPI::cchPathMax );
    Assert( m_wszLogCurrent == m_wszRestorePath );

    //  disable the sector-size checks

//
//  SEARCH-STRING: SecSizeMismatchFixMe
//
//  ---** TEMPORARY FIX **---
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
//
//  SEARCH-STRING: SecSizeMismatchFixMe
//
//  m_pLogStream->CbSecVolume() = ~(ULONG)0;

    //  use the proper log file size for recovery

    Assert( m_fUseRecoveryLogFileSize == fFalse );
    m_fUseRecoveryLogFileSize = fTrue;

    /*  check log signature and database signatures
    /**/

    Assert ( 0 == m_lGenHighTargetInstance || m_wszTargetInstanceLogPath[0] );

    //  check log signature and database signatures
    //  also auto configures the log ext in use
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

    // SOFT_HARD: leave like this most likely. Set a flag in the RSMAP maybe below
    m_fHardRestore = fTrue;
    m_fLogDisabled = fFalse;
    m_fAbruptEnd = fFalse;

    CallJ( ErrBuildRstmapForExternalRestore( rgjrstmap, cjrstmap ), TermResetGlobals );

    /*  make sure all the patch files have enough logs to replay
    /**/

    for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
    {
        /*  open patch file and check its minimum requirement for full backup,
        /**/
        LGPOS lgposSnapshotDb = lgposMin;
        Assert( m_fSignLogSet );

        CallJ( ErrLGCheckDBFiles( m_pinst, m_rgrstmap + irstmap, lgenLow, lgenHigh, &lgposSnapshotDb ), TermFreeRstmap );
    }

    // check that there are no databases with same signature
    Assert ( FRstmapCheckDuplicateSignature() );

//  CallJ( FMP::ErrFMPInit(), TermFreeRstmap );

    //  initialize log manager and set working log file path

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

    /*  set up checkpoint file for restore
    /**/
    Call ( m_pLogStream->ErrLGGetGenerationRange( m_wszRestorePath,
            !lgenLow?&lgenLow:NULL,
            !lgenHigh?&lgenHigh:NULL ) );

    // We could be restoring from a backup that started before the test initial log generation.
    m_lgenInitial = m_lgenInitial > lgenLow ? lgenLow : m_lgenInitial;

    Call( ErrLGIRSTSetupCheckpoint( lgenLow, lgenHigh, wszCheckpointFilePath ) );

    m_lGenLowRestore = lgenLow;
    m_lGenHighRestore = lgenHigh;

    /*  prepare for callbacks
    /**/
    m_pinst->m_pfnInitCallback = pfn;
    m_pinst->m_pvInitCallbackContext = pvContext;
    if ( m_pinst->m_pfnInitCallback != NULL )
    {
        plgstat = &lgstat;
        LGIRSTPrepareCallback( plgstat, lgenHigh, lgenLow, 0 );
    }
    else
    {
        //  zero out to silence the compiler
        //
        Assert( NULL == plgstat );
        memset( &lgstat, 0, sizeof(lgstat) );
    }

    /*  adjust fmp according to the restore map
    /**/
    m_fExternalRestore = fTrue;

    // on hard recovery set it such that we ignore all dbs not in the map
    m_fReplayMissingMapEntryDB = fFalse;
    m_fReplayingIgnoreMissingDB = fTrue;

    /*  do redo according to the checkpoint, dbms_params, and rgbAttach
    /*  set in checkpointGlobal.
    /**/
    Assert( m_wszLogCurrent == m_wszRestorePath );
    m_errGlobalRedoError = JET_errSuccess;
    Call( ErrLGRRedo( fFalse, m_pcheckpoint, plgstat ) );

    //  ErrLGRRedo() gets through the logs, but doesn't set the LOG() to run w/ the newly desired
    //  parameters ( log file size, log ext, etc )

    Call( ErrLGMoveToRunningState( err, NULL ) );

    //  we should be using the right log file size by now

    Assert( m_fUseRecoveryLogFileSize == fFalse );

    //  sector-size checking should now be on

    Assert( m_pLogStream->CbSecVolume() != ~(ULONG)0 );

    //  update saved copy

    cbSecVolumeSave = m_pLogStream->CbSecVolume();

    /*  same as going to shut down, Make all attached databases consistent
    /**/
    if ( plgstat )
    {
        /*  top off the progress metre and wrap it up
        /**/
        lgstat.snprog.cunitDone = lgstat.snprog.cunitTotal;     //lint !e644
        (lgstat.pfnCallback)( JET_snpRestore, JET_sntComplete, &lgstat.snprog, lgstat.pvCallbackContext );
    }

HandleError:
    /*  either error or terminated
    /**/
    Assert( err < 0 || m_pinst->m_fSTInit == fSTInitNotDone );
    if ( err < 0  &&  m_pinst->m_fSTInit != fSTInitNotDone )
    {
        Assert( m_pinst->m_fSTInit == fSTInitDone );
        const ERR errT = m_pinst->ErrINSTTerm( termtypeError );
        Assert( errT == JET_errSuccess || m_pinst->m_fTermAbruptly );
    }

//  CallS( ErrLGTerm( pfsapi, err >= JET_errSuccess ) );
    CallS( ErrLGTerm( fFalse ) );

    // on success, delete the files generated by this instance
    // (logs, chk, reserve log pool)
    if ( err >= 0 )
    {
        WCHAR   wszFullLogPath[IFileSystemAPI::cchPathMax];
        WCHAR   wszFullTargetPath[IFileSystemAPI::cchPathMax];

        //  delete the log files generated if those files are not in
        //  the instance that is runnign.
        //
        Assert ( 0 == m_lGenHighTargetInstance || m_wszTargetInstanceLogPath[0] );
        //  UNDONE:  I'm pretty sure this assert is obsolete.  consider removing it
        Assert ( SzParam( m_pinst, JET_paramLogFilePath ) );

        CallS( m_pinst->m_pfsapi->ErrPathComplete( m_wszTargetInstanceLogPath, wszFullTargetPath ) );
        CallS( m_pinst->m_pfsapi->ErrPathComplete( SzParam( m_pinst, JET_paramLogFilePath ), wszFullLogPath ) );

        Assert ( 0 == m_lGenHighTargetInstance || (0 != UtilCmpFileName( wszFullTargetPath, wszFullLogPath ) ) );
        if ( m_lGenHighTargetInstance && ( 0 != UtilCmpFileName( wszFullTargetPath, wszFullLogPath ) ) )
        {
/*          LONG        genLowT;
            LONG        genHighT;

            Assert ( m_lGenHighTargetInstance );

            genLowT = m_lGenHighTargetInstance + 1;
            LGILastGeneration( pfsapi, SzParam( m_pinst, JET_paramLogFilePath ), &genHighT );
            // if only edb.jtx/log new generated, genHighT will be the max one from the
            // backup set (if in that directory)
            genHighT = max ( genHighT, genLowT);
            m_pLogStream->LGRSTDeleteLogs( pfsapi, SzParam( m_pinst, JET_paramLogFilePath ), genLowT, genHighT, fLGRSTIncludeJetLog );

            LGFullNameCheckpoint( pfsapi, szFileName );
            CallSx( pfsapi->ErrFileDelete( szFileName ), JET_errFileNotFound );
*/
            CallSx( m_pLogStream->ErrLGDeleteLegacyReserveLogFiles(), JET_errFileNotFound );
        }
    }

TermFMP:
//  FMP::Term();

TermFreeRstmap:
    FreeRstmap( );

TermResetGlobals:
    // SOFT_HARD: this is fine as long as we have the flag
    m_fHardRestore = fFalse;

    m_wszTargetInstanceLogPath[0] = L'\0';

    /*  reset initialization state
    /**/
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

    // signal the caller that we found a running instance
    // the caller (eseback2) will deal with the resulting logs
    // generated by the restore instance in  szNewLogPath
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

// Compares the given two logs for divergence.
// If fAllowSubsetIfEmpty is set, logs will not be considered diverged if one is a subset of the other, provided reminder of that log is empty.
//
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
    QWORD cbMaxRead     = 2 * 1024 * 1024; // 2MB max read size. Logs are usually 1MB and we will take min of the two.

    BOOL fCloseNormally     = fFalse;    
    const BOOL fRecoveringSaved   = m_fRecovering;
    const RECOVERING_MODE rmSaved = m_fRecoveringMode;

    *pfLogsDiverged = fFalse;

    Call( CIOFilePerf::ErrFileOpen( m_pinst->m_pfsapi, m_pinst, wszLog1, IFileAPI::fmfReadOnly, iofileLog, qwDumpingFileID, &pfapiLog1 ) );
    Call( CIOFilePerf::ErrFileOpen( m_pinst->m_pfsapi, m_pinst, wszLog2, IFileAPI::fmfReadOnly, iofileLog, qwDumpingFileID, &pfapiLog2 ) );
    Call( pfapiLog1->ErrSize( &cbLog1, IFileAPI::filesizeLogical ) );
    Call( pfapiLog2->ErrSize( &cbLog2, IFileAPI::filesizeLogical ) );

    // Logs should be of same size.
    if ( cbLog1 != cbLog2 )
    {
        *pfLogsDiverged = fTrue;
    }    

    if ( fAllowSubsetIfEmpty )
    {
        m_fRecovering = fTrue;
        m_fRecoveringMode = fRecoveringRedo;

        Call( m_pLogStream->ErrLGInit() );

        // Get last log pos for log1
        m_pLogStream->SetPfapi( pfapiLog1 );
        Call( m_pLogStream->ErrLGReadFileHdr( NULL, iorpDirectAccessUtil, NULL, fNoCheckLogID, fTrue ) );
        Call( m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( &fCloseNormally, fTrue, fTrue ) );

        cbLog1 = m_pLogReadBuffer->LgposFileEnd().isec * m_pLogStream->CbSec();

        // Get last log pos for log2
        m_pLogStream->ResetPfapi();
        m_pLogStream->SetPfapi( pfapiLog2 );
        Call( m_pLogStream->ErrLGReadFileHdr( NULL, iorpDirectAccessUtil, NULL, fNoCheckLogID, fTrue ) );
        Call( m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( &fCloseNormally, fTrue, fTrue ) );

        cbLog2 = m_pLogReadBuffer->LgposFileEnd().isec * m_pLogStream->CbSec();
    }

    // cbLog1 and cbLog2 will be different if one is only partially filled.
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

//  Retrieves the log generation from a file name.
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

    /*  get file name and extension
    /**/
    err = pfsapi->ErrPathParse( wszFileName, NULL, wszFNameT, wszExtT );
    CallS( err );

    /* if has log file extension
    /**/
    if ( UtilCmpFileName( wszExtT, wszLogExt ) )
    {
        // Engh, only a little bit bad ...
        // happens from esentutl /k edb
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }

    /* if has not the current base name
    /**/
    if ( _wcsnicmp( wszFNameT, wszBaseName, wcslen( wszBaseName ) ) )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }

    const ULONG cLogDigitsSmall = 5;
    const ULONG cLogDigitsBig = 8;

    // if length of a numbered log file name
    ULONG cLogDigits = LOSStrLengthW( wszFNameT );
    if ( cLogDigits < wcslen( wszBaseName ) )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }
    cLogDigits -= wcslen( wszBaseName ); // subtract base name digits
    if ( ( cLogDigits != cLogDigitsSmall &&
          cLogDigits != cLogDigitsBig ) )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }
    const INT   ibMax = (cLogDigits + wcslen( wszBaseName ));

    // finally try to pull out the log generation ...
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

    // Yeah, there was much rejoicing ...
    *plgen = lGen;
    *pcchLogDigits = cLogDigits;

Assert( !err );

HandleError:

    return(err);

}


