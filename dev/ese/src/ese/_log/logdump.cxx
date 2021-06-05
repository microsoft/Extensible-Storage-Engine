// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "logstd.hxx"
#include "_dump.hxx"

/* in-memory log buffer. */
#define         csecLGBufSize 100

VOID DUMPIAttachInfo( const ATCHCHK * const patchchk )
{
    const DBTIME    dbtime  = patchchk->Dbtime();
    DUMPPrintF( "        dbtime: %I64u (%u-%u)\n",
            dbtime,
            ((QWORDX *)&dbtime)->DwHigh(),
            ((QWORDX *)&dbtime)->DwLow() );
    DUMPPrintF( "        objidLast: %u\n", patchchk->ObjidLast() );
    DUMPPrintF( "        Signature: " );
    DUMPPrintSig( &patchchk->signDb );
    DUMPPrintF( "        MaxDbSize: %u pages\n", patchchk->CpgDatabaseSizeMax() );
    DUMPPrintF( "        Last Attach: (0x%X,%X,%X)\n",
            patchchk->lgposAttach.lGeneration,
            patchchk->lgposAttach.isec,
            patchchk->lgposAttach.ib );
    DUMPPrintF( "        Last Consistent: (0x%X,%X,%X)\n",
            patchchk->lgposConsistent.lGeneration,
            patchchk->lgposConsistent.isec,
            patchchk->lgposConsistent.ib );
}


// Dump the ATTACHINFO from DbList
//  ================================================================
VOID DUMPIAttachInfo( const ATTACHINFO * const pattachinfo )
//  ================================================================
{
    const DBTIME    dbtime  = pattachinfo->Dbtime();
    DUMPPrintF( "		 dbtime: %I64u (%u-%u)\n",
            dbtime,
            ((QWORDX *)&dbtime)->DwHigh(),
            ((QWORDX *)&dbtime)->DwLow() );
    DUMPPrintF( "		 objidLast: %u\n", pattachinfo->ObjidLast() );
    DUMPPrintF( "		 Signature: " );
    DUMPPrintSig( &pattachinfo->signDb );
    DUMPPrintF( "		 MaxDbSize: %u pages\n", pattachinfo->CpgDatabaseSizeMax() );
    DUMPPrintF( "		 Last Attach: (0x%X,%X,%X)\n",
            (LONG)pattachinfo->le_lgposAttach.le_lGeneration,
            (USHORT)pattachinfo->le_lgposAttach.le_isec,
            (USHORT)pattachinfo->le_lgposAttach.le_ib );
    DUMPPrintF( "		 Last Consistent: (0x%X,%X,%X)\n",
            (LONG)pattachinfo->le_lgposConsistent.le_lGeneration,
            (USHORT)pattachinfo->le_lgposConsistent.le_isec,
            (USHORT)pattachinfo->le_lgposConsistent.le_ib );
}


ERR ErrDUMPCheckpoint( INST *pinst, __in PCWSTR wszCheckpoint )
{
    Assert( pinst->m_plog );
    Assert( pinst->m_pver );

    ERR err = pinst->m_plog->ErrLGDumpCheckpoint( wszCheckpoint );

    return err;
}

ERR LOG::ErrLGDumpCheckpoint( __in PCWSTR wszCheckpoint )
{
    ERR         err;
    LGPOS       lgpos;
    LOGTIME     tm;
    CHECKPOINT  *pcheckpoint = NULL;
    DBMS_PARAM  dbms_param;
    IFMP        ifmp;

    AllocR( pcheckpoint = (CHECKPOINT *) PvOSMemoryPageAlloc( sizeof( CHECKPOINT ), NULL ) );

    err = ErrLGReadCheckpoint( wszCheckpoint, pcheckpoint, fTrue );
    if ( err == JET_errSuccess )
    {
        lgpos = pcheckpoint->checkpoint.le_lgposLastFullBackupCheckpoint;
        DUMPPrintF( "      LastFullBackupCheckpoint: (0x%X,%X,%X)\n",
                lgpos.lGeneration, lgpos.isec, lgpos.ib );

        lgpos = pcheckpoint->checkpoint.le_lgposCheckpoint;
        DUMPPrintF( "      Checkpoint: (0x%X,%X,%X)\n",
                lgpos.lGeneration, lgpos.isec, lgpos.ib );

        lgpos = pcheckpoint->checkpoint.le_lgposDbConsistency;
        DUMPPrintF( "      DbConsistency: (0x%X,%X,%X)\n",
                lgpos.lGeneration, lgpos.isec, lgpos.ib );

        lgpos = pcheckpoint->checkpoint.le_lgposFullBackup;
        DUMPPrintF( "      FullBackup: (0x%X,%X,%X)\n",
                lgpos.lGeneration, lgpos.isec, lgpos.ib );

        tm = pcheckpoint->checkpoint.logtimeFullBackup;
        DUMPPrintF( "      FullBackup time: ");
        DUMPPrintLogTime(&tm );
        DUMPPrintF( "\n" );

        lgpos = pcheckpoint->checkpoint.le_lgposIncBackup;
        DUMPPrintF( "      IncBackup: (0x%X,%X,%X)\n",
                lgpos.lGeneration, lgpos.isec, lgpos.ib );

        tm = pcheckpoint->checkpoint.logtimeIncBackup;
        DUMPPrintF( "      IncBackup time: ");
        DUMPPrintLogTime(&tm );
        DUMPPrintF( "\n" );

        DUMPPrintF( "      Signature: " );
        DUMPPrintSig( &pcheckpoint->checkpoint.signLog );

        dbms_param = pcheckpoint->checkpoint.dbms_param;
        DUMPPrintF( "      Env (CircLog,Session,Opentbl,VerPage,Cursors,LogBufs,LogFile,Buffers)\n");
        DUMPPrintF( "          (%s,%7lu,%7lu,%7lu,%7lu,%7lu,%7lu,%7lu)\n",
                ( dbms_param.fDBMSPARAMFlags & fDBMSPARAMCircularLogging ? "     on" : "    off" ),
                ULONG( dbms_param.le_lSessionsMax ),
                ULONG( dbms_param.le_lOpenTablesMax ),
                ULONG( dbms_param.le_lVerPagesMax ),
                ULONG( dbms_param.le_lCursorsMax ),
                ULONG( dbms_param.le_lLogBuffers ),
                ULONG( dbms_param.le_lcsecLGFile ),
                ULONG( dbms_param.le_ulCacheSizeMax ) );

        Call( m_pinst->m_plog->ErrLGLoadFMPFromAttachments( pcheckpoint->rgbAttach ) );
        for ( ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++)
        {
            FMP *pfmp = &g_rgfmp[ifmp];
            if ( pfmp->FInUse() && dbidTemp != pfmp->Dbid() )
            {
                const ATCHCHK * const   patchchk    = pfmp->Patchchk();

                if ( NULL != patchchk )
                {
                    DUMPPrintF(
                            "%7d %ws %s %s %s\n",
                            pfmp->Dbid(),
                            pfmp->WszDatabaseName(),
                            pfmp->FLogOn() ? "LogOn" : "LogOff",
                            pfmp->FVersioningOff() ? "VerOff" : "VerOn",
                            pfmp->FReadOnlyAttach() ? "R" : "RW" );
                    DUMPIAttachInfo( patchchk );
                }
                else
                {
                    //  should never be NULL
                    //
                    Assert( fFalse );
                }
            }
        }
    }

HandleError:
    //  if ( NULL != g_rgfmp )
    //      FMP::Term();
    OSMemoryPageFree( pcheckpoint );

    return err;
}

struct DIRLOGGENERATIONINFO
{
    ULONG   ulgenMax;           // Maximum archive generation id
    ULONG   ulgenMin;           // Minimum archive generation id
    ULONG   cMaxLogDigits;      // Maximum digits (either 0, 5, or 8) for log files
    BOOL    fLogDigitsChanged;  // Does the number of digits in log file change?
    BOOL    fCurrentLog;            // Does the current log exist?
};

/*
 * Get the log generation information from the specified directory
 */
LOCAL ERR ErrGetDirLogGenerationInfo(
        INST *pinst,
        _In_z_count_c_( OSFSAPI_MAX_PATH /*IFileSystemAPI::cchPathMax*/) PCWSTR wszFind,
        LONG lgenStart,
        DIRLOGGENERATIONINFO *pDirLogGenInfo )
{
    Assert( pinst );
    Assert( pinst->m_pfsapi );
    Assert( wszFind );
    Assert( 0 <= lgenStart );
    Assert( lGenerationInvalidDuringDump > lgenStart );
    Assert( pDirLogGenInfo );

    ERR             err;
    IFileFindAPI        *pffapi = NULL;
    IFileSystemAPI  *pfsapi = pinst->m_pfsapi;
    ULONG           cCurrentLogDigits = 0;

    pDirLogGenInfo->ulgenMax = 0;
    pDirLogGenInfo->ulgenMin = lGenerationEXXDuringDump;
    pDirLogGenInfo->cMaxLogDigits = 0;
    pDirLogGenInfo->fLogDigitsChanged = fFalse;
    pDirLogGenInfo->fCurrentLog = fFalse;

    CallR( pfsapi->ErrFileFind( wszFind, &pffapi ) );
    forever
    {
        WCHAR   wszFileName[IFileSystemAPI::cchPathMax];
        WCHAR   wszDirT[IFileSystemAPI::cchPathMax];
        WCHAR   wszFNameT[IFileSystemAPI::cchPathMax];
        WCHAR   wszExtT[IFileSystemAPI::cchPathMax];
        LONG    lGen = 0;

        //  get file name and extension
        Call( pffapi->ErrNext() );
        Call( pffapi->ErrPath( wszFileName ) );
        Call( pfsapi->ErrPathParse( wszFileName, wszDirT, wszFNameT, wszExtT ) );

        if  ( 0 == UtilCmpFileName( wszExtT, wszNewLogExt ) || 0 == UtilCmpFileName( wszExtT, wszOldLogExt ) )
        {
            err = LGFileHelper::ErrLGGetGeneration( pfsapi, wszFileName, SzParam( pinst, JET_paramBaseName ), &lGen, wszExtT, &cCurrentLogDigits);
            if ( JET_errSuccess == err && lGen > 0 )
            {
                if ( lGen < ( lGenerationEXXDuringDump ) && lGen > lgenStart )
                {
                    pDirLogGenInfo->ulgenMax = max( pDirLogGenInfo->ulgenMax, (ULONG)lGen );
                    pDirLogGenInfo->ulgenMin = min( pDirLogGenInfo->ulgenMin, (ULONG)lGen );
                    if ( 0 != pDirLogGenInfo->cMaxLogDigits && cCurrentLogDigits != pDirLogGenInfo->cMaxLogDigits )
                    {
                        // log digits changed
                        Assert( 5 == cCurrentLogDigits || 8 == cCurrentLogDigits );
                        pDirLogGenInfo->fLogDigitsChanged = fTrue;
                    }
                    pDirLogGenInfo->cMaxLogDigits = max( pDirLogGenInfo->cMaxLogDigits, cCurrentLogDigits );
                }
            }
            else if ( 3 == LOSStrLengthW( wszFNameT ) && 0 == _wcsicmp( wszFNameT, SzParam( pinst, JET_paramBaseName ) ) )
            {
                //  found <inst>.[log|.jtx]
                pDirLogGenInfo->fCurrentLog = fTrue;
            }
        }
    }

HandleError:
    if ( pffapi )
    {
        delete pffapi;
    }

    if ( JET_errFileNotFound == err )
    {
        err = JET_errSuccess;
    }

    if ( lGenerationEXXDuringDump == pDirLogGenInfo->ulgenMin )
    {
        Assert( 0 == pDirLogGenInfo->ulgenMax );
        pDirLogGenInfo->ulgenMin = 0;
    }
    return err;
}

/*
 * Giving a generation number, this function will return the next one which has corresponding log file
 * Exchange12 146373: Use the obtained maximum/minimum generation to speed up nexgen
 * with complexity O(N)
 */
LOCAL ERR ErrFindNextGen(
        INST *pinst,
        const DIRLOGGENERATIONINFO * const pDirLogGenInfo,
        __in PCWSTR wszLogDir,
        __in PCWSTR wszLogExt,
        LONG* plgen,
        ULONG *pcLogDigits,
        __out_bcount(cbLogFile) PWSTR wszLogFile,
        __in_range(sizeof(WCHAR), cbOSFSAPI_MAX_PATHW) size_t cbLogFile)
{
    Assert( pinst );
    Assert( pinst->m_plog );
    Assert( pinst->m_pver );
    Assert( pDirLogGenInfo );
    Assert( plgen );
    Assert( pcLogDigits );
    Assert( wszLogFile && cbLogFile > 0 );
    Assert( pDirLogGenInfo->ulgenMin <= pDirLogGenInfo->ulgenMax );
    Assert( lGenerationInvalidDuringDump > *plgen );

    ERR             err = JET_errSuccess;
    WCHAR           wszFind[ IFileSystemAPI::cchPathMax ];
    WCHAR           wszFileName[ IFileSystemAPI::cchPathMax ];
    IFileFindAPI*       pffapi = NULL;
    ULONG           cLogDigits = 0;

    wszLogFile[0] = L'\0';

    // Set the initial next generation number to exactly 1 ahead the current generation number
    ULONG ulgenNext = (ULONG)max( (ULONG)( *plgen + 1 ), pDirLogGenInfo->ulgenMin );

    forever
    {
        if ( ulgenNext > ( lGenerationEXXDuringDump ) )
        {
            // we have run past the allowable limit, did not find log file
            // caller expect file to be empty string, so set here.
            ulgenNext = lGenerationInvalidDuringDump;
            cLogDigits = 0;
            wszLogFile[0] = L'\0';
            err = JET_errSuccess;
            break;
        }
        else if ( ulgenNext > pDirLogGenInfo->ulgenMax )
        {
            // Next gen falls into (pDirLogGenInfo->ulgenMax, lGenerationEXXDuringDump]
            // Caller expects (lGenerationEXXDuringDump) as the generation number to be returned for the current log
            ulgenNext = lGenerationEXXDuringDump;
            if ( pDirLogGenInfo->fCurrentLog )
            {
                Call( LGFileHelper::ErrLGMakeLogNameBaselessEx( wszFind,
                            sizeof(wszFind),
                            pinst->m_pfsapi,
                            wszLogDir,
                            SzParam( pinst, JET_paramBaseName ),
                            eCurrentLog,
                            0,
                            wszLogExt, 0 ) );

                Call( pinst->m_pfsapi->ErrFileFind( wszFind, &pffapi ) );
                err = pffapi->ErrNext( );
                if ( JET_errSuccess == err )
                {
                    Call( pffapi->ErrPath( wszFileName ) );
                    cLogDigits = 0;
                    OSStrCbCopyW( wszLogFile, cbLogFile, wszFileName );
                    break;
                }
            }
            else if ( JET_errFileNotFound != err )
            {
                Call( err );
            }
        }
        else
        {
            // Next gen falls into (pDirLogGenInfo->ulgenMin, pDirLogGenInfo->ulgenMax], must be archived log file

            cLogDigits = pDirLogGenInfo->cMaxLogDigits;
            Assert( 5 == cLogDigits || 8 == cLogDigits );

            Call( LGFileHelper::ErrLGMakeLogNameBaselessEx( wszFind,
                        sizeof(wszFind),
                        pinst->m_pfsapi,
                        wszLogDir,
                        SzParam( pinst, JET_paramBaseName ),
                        eArchiveLog,
                        ulgenNext,
                        wszLogExt, cLogDigits ) );

            Call( pinst->m_pfsapi->ErrFileFind( wszFind, &pffapi ) );
            err = pffapi->ErrNext();

            if ( JET_errFileNotFound == err && fTrue == pDirLogGenInfo->fLogDigitsChanged )
            {
                // There has log digits change. We have tried with maximum log digits, if we still have digits changes,
                // the former turn must used 8 for the digits length
                Assert ( 8 == cLogDigits );

                cLogDigits = 5;

                if ( pffapi )
                {
                    delete pffapi;
                    pffapi = NULL;
                }

                Call( LGFileHelper::ErrLGMakeLogNameBaselessEx( wszFind,
                            sizeof( wszFind ),
                            pinst->m_pfsapi,
                            wszLogDir,
                            SzParam( pinst, JET_paramBaseName ),
                            eArchiveLog,
                            ulgenNext,
                            wszLogExt, cLogDigits ) );

                Call( pinst->m_pfsapi->ErrFileFind( wszFind, &pffapi ) );
                err = pffapi->ErrNext();
            }

            if ( JET_errSuccess == err )
            {
                //  get file name
                Call( pffapi->ErrPath( wszFileName ) );
                OSStrCbCopyW( wszLogFile, cbLogFile, wszFileName );
                break;
            }
            else if ( JET_errFileNotFound != err )
            {
                Call( err );
            }

            if ( pffapi )
            {
                delete pffapi;
                pffapi = NULL;
            }
        }
        ulgenNext++;
    }
HandleError:
    if ( pffapi )
    {
        delete pffapi;
    }
    if ( err == JET_errFileNotFound )
    {
        err = JET_errSuccess;
    }
    if ( err == JET_errSuccess )
    {
        *plgen = ulgenNext;
        *pcLogDigits = cLogDigits;
    }
    return err;
}

//  report a corrupt or missing logfile
//
LOCAL VOID LGIReportLogfileCorrupt(
        const WCHAR * const wszLog,
        const ERR           err,
        const INST * const  pinst )
{
    WCHAR               wszErr[16];
    const WCHAR *       rgwsz[] = { wszLog, wszErr, };
    const BOOL          fMissing        = ( JET_errFileNotFound == err || JET_errMissingLogFile == err );

    Assert( NULL != wszLog );
    Assert( JET_errSuccess != err );
    Assert( NULL != pinst );

    OSStrCbFormatW( wszErr, sizeof(wszErr), L"%d", err );

    UtilReportEvent(
            eventError,
            LOGGING_RECOVERY_CATEGORY,
            ( fMissing ? LOG_FILE_MISSING_ID : LOG_FILE_CORRUPTED_ID ),
            _countof( rgwsz ),
            rgwsz,
            0,
            NULL,
            pinst );
}

//  report a missing logfile or range of missing logfiles
//
LOCAL VOID LGIReportLogfilesMissing(
        const WCHAR * const wszPathedBaseName,
        const WCHAR * const wszGenBegin,
        const WCHAR * const wszGenEnd,  //  pass NULL if only one logfile is missing
        const WCHAR * const wszLogExt,
        const ERR           err,
        const INST * const  pinst )
{
    const WCHAR *       rgwsz[3];
    WCHAR               wszErr[16];
    ULONG               irgsz       = 0;
    WCHAR               wszPathedLogfileBegin[IFileSystemAPI::cchPathMax];
    WCHAR               wszPathedLogfileEnd[IFileSystemAPI::cchPathMax];

    Assert( NULL != wszPathedBaseName );
    Assert( NULL != wszGenBegin );
    Assert( NULL != wszLogExt );
    Assert( NULL != pinst );

    //  compose fully-pathed logfile name
    //
    Assert( wcslen( wszPathedBaseName ) + wcslen( wszGenBegin ) + wcslen( wszLogExt ) < sizeof(wszPathedLogfileBegin) );
    OSStrCbCopyW( wszPathedLogfileBegin, sizeof(wszPathedLogfileBegin), wszPathedBaseName );
    OSStrCbAppendW( wszPathedLogfileBegin, sizeof(wszPathedLogfileBegin), wszGenBegin );
    OSStrCbAppendW( wszPathedLogfileBegin, sizeof(wszPathedLogfileBegin), wszLogExt );
    rgwsz[irgsz++] = wszPathedLogfileBegin;

    if ( NULL != wszGenEnd )
    {
        //  compose fully-pathed logfile name
        //
        Assert( wcslen( wszPathedBaseName ) + wcslen( wszGenEnd ) + wcslen( wszLogExt ) < sizeof(wszPathedLogfileEnd) );
        OSStrCbCopyW( wszPathedLogfileEnd, sizeof(wszPathedLogfileEnd), wszPathedBaseName );
        OSStrCbAppendW( wszPathedLogfileEnd, sizeof(wszPathedLogfileEnd), wszGenEnd );
        OSStrCbAppendW( wszPathedLogfileEnd, sizeof(wszPathedLogfileEnd), wszLogExt );
        rgwsz[irgsz++] = wszPathedLogfileEnd;
    }

    OSStrCbFormatW( wszErr, sizeof(wszErr), L"%d", err );
    rgwsz[irgsz++] = wszErr;

    Assert( irgsz <= ( sizeof(rgwsz) / sizeof(rgwsz[0]) ) );
    UtilReportEvent(
            eventError,
            LOGGING_RECOVERY_CATEGORY,
            ( NULL != wszGenEnd ? LOG_RANGE_MISSING_ID : LOG_FILE_MISSING_ID ),
            irgsz,
            rgwsz,
            0,
            NULL,
            pinst );
}


ERR ErrDUMPLogFromMemory( INST *pinst, __in PCWSTR wszLog, __in_bcount( cbBuffer ) void *pvBuffer, ULONG cbBuffer )
{
    ERR err;
    LOG::LOGDUMP_OP logdumpOp = { 0 };

    IFileAPI *pfapi = new CFileFromMemory( (BYTE *)pvBuffer, cbBuffer, wszLog );

    if ( NULL == pfapi )
    {
        CallR( ErrERRCheck( JET_errOutOfMemory ) );
    }

    // Set up some global state to make it possible to call this logging function

    err = pinst->m_plog->ErrLGDumpLog( pfapi, &logdumpOp );

    pinst->m_plog->LGCloseFile();

    return err;
}


ERR ErrDUMPLog( INST *pinst, __in PCWSTR wszLog, const LONG lgenStart, const LONG lgenEnd, JET_GRBIT grbit, __in PCWSTR wszCsvDataFile )
{
    ERR             err             = JET_errSuccess;
    ERR             errDump         = JET_errSuccess;
    WCHAR           wszLogFileDir[IFileSystemAPI::cchPathMax];
    WCHAR           wszLogFileName[IFileSystemAPI::cchPathMax];
    WCHAR           wszLogFileExt[IFileSystemAPI::cchPathMax];
    WCHAR           wszLogFindTemplate[IFileSystemAPI::cchPathMax];
    LGFILEHDR *     plgfilehdr      = NULL;
    XECHECKSUM      checksumLastSegment = 0;
    LOG::LOGDUMP_OP logdumpOp = { 0 };
    const ULONG     cchLogPath      = LOSStrLengthW( wszLog );
    const WCHAR *   wszLogExt       = NULL;
    ULONG           cLogDigits = 0;
    CWPRINTFFILE cpfCsvOut( wszCsvDataFile );
    DIRLOGGENERATIONINFO dirloginfo = { 0 };
    LONG            lgenStartSanitised = 0;
    LONG            lgenEndSanitised = 0;

    logdumpOp.m_opts = 0;
    Assert( logdumpOp.m_pcwpfCsvOut == NULL );

    if ( cchLogPath >= IFileSystemAPI::cchPathMax )
    {
        return ErrERRCheck( JET_errInvalidPath );
    }

    Assert( !( grbit & JET_bitDBUtilOptionDumpVerboseLevel2 ) || ( grbit & JET_bitDBUtilOptionDumpVerboseLevel1 ) );

    Call( pinst->m_pfsapi->ErrPathParse( wszLog, wszLogFileDir, wszLogFileName, wszLogFileExt ) );
    if ( 3 == LOSStrLengthW( wszLogFileName ) && 0 == LOSStrLengthW( wszLogFileExt ) )
    {

        //
        // This is dumping a series of log, by the specified log extension.
        //

        LONG    lgen = 0;
        LONG    lgenLast = 0;

        Alloc( plgfilehdr = (LGFILEHDR *) PvOSMemoryPageAlloc( sizeof( LGFILEHDR ), NULL ) );
        wszLogExt = ( 0 != wszLogFileExt[0] ) ? wszLogFileExt : wszNewLogExt;

        logdumpOp.m_loghdr = LOG::LOGDUMP_LOGHDR_INVALID;
        if ( grbit & JET_bitDBUtilOptionDumpLogInfoCSV )
        {
            Assert( wszCsvDataFile ); // otherwise we won't be printing to any file.
            logdumpOp.m_pcwpfCsvOut = &cpfCsvOut;

            DUMPPrintF( "\n      Csv file: %ws\n", wszCsvDataFile );
            if (JET_errSuccess != logdumpOp.m_pcwpfCsvOut->m_errLast)
            {
                DUMPPrintF( "\n      Cannot open csv file (%ws). Error %d.\n",
                        wszCsvDataFile, logdumpOp.m_pcwpfCsvOut->m_errLast);
                Call( ErrERRCheck( logdumpOp.m_pcwpfCsvOut->m_errLast ) );
            }
        }
        else if ( grbit & JET_bitDBUtilOptionDumpVerboseLevel1 )
        {
            logdumpOp.m_iVerbosityLevel = LOG::ldvlStructure;
            logdumpOp.m_fPrint = 1;

            if ( grbit & JET_bitDBUtilOptionDumpVerboseLevel2 )
            {
                logdumpOp.m_iVerbosityLevel = LOG::ldvlData;
            }
        }

        DUMPPrintF( "Verifying log files...\n" );
        DUMPPrintF( "     %sBase name: %ws\n", ( logdumpOp.m_iVerbosityLevel != LOG::ldvlBasic ) ? " " : "", SzParam( pinst, JET_paramBaseName ) );

        //  sanitise starting generation
        //
        if ( lgenStart > 0 && lgenStart <= lGenerationMaxDuringRecovery )
        {
            lgenStartSanitised = lgenStart;
        }
        else
        {
            //  illegal starting generation, so just start from the first generation
            //
            lgenStartSanitised = 1;
        }

        //  sanitise ending generating
        //
        if ( lgenEnd > 0 && lgenEnd <= lGenerationMaxDuringRecovery )
        {
            //  ensure ending generation is at least as much as the
            //  starting generation
            //
            lgenEndSanitised = max( lgenStartSanitised, lgenEnd );
        }
        else
        {
            //  illegal ending generation, so just set to the max
            //
            lgenEndSanitised = lGenerationEXXDuringDump;
        }

        //  if both starting and ending generation are 0, then the caller
        //  didn't care to specify a log generation range, so don't bother
        //  reporting it
        //
        if ( lgenStart != 0 || lgenEnd != 0 )
        {
            DUMPPrintF( "     %sLog generation range: 0x%X-0x%X\n", ( logdumpOp.m_iVerbosityLevel != LOG::ldvlBasic ) ? " " : "", lgenStartSanitised, lgenEndSanitised );
        }

TryAnotherExtension:
        //  -1 because ErrFindNextGen() below will look for lgen+1
        //
        lgen = lgenStartSanitised - 1;

        //  form template for FileFind
        //
        LGFileHelper::ErrLGMakeLogNameBaselessEx( wszLogFindTemplate, sizeof(wszLogFindTemplate), pinst->m_pfsapi,
                wszLogFileDir, SzParam( pinst, JET_paramBaseName ), eArchiveLog, 0, // <-- 0 means wildcard
                wszLogExt, 0 );
        Call( ErrGetDirLogGenerationInfo( pinst, wszLogFindTemplate, lgen, &dirloginfo ) );

        Call( ErrFindNextGen( pinst, &dirloginfo, wszLogFileDir, wszLogExt, &lgen, &cLogDigits, wszLogFileName, sizeof(wszLogFileName) ) );

        // If lgen is this high, it means ErrFindNextGen() found no items ...
        if ( 0 == _wcsicmp( wszLogExt, wszNewLogExt ) && lGenerationInvalidDuringDump == lgen )
        {
            // We found no logs, sooo try the other extension ... 
            wszLogExt = wszOldLogExt;
            goto TryAnotherExtension;
        }

        // we still haven't found any logs even with the old extension
        // we should error out at this point
        //
        if ( 0 ==_wcsicmp( wszLogExt, wszOldLogExt ) && lGenerationInvalidDuringDump == lgen )
        {
            Error( ErrERRCheck( JET_errFileNotFound ) );
        }

        if ( ( logdumpOp.m_iVerbosityLevel == LOG::ldvlBasic ) && 0 != wszLogFileName[0] )
            DUMPPrintF( "\n" );

        while ( 0 != wszLogFileName[0] && lgen <= lgenEndSanitised )
        {
            ULONG cLogDigitsCurr = 0;
            lgenLast = lgen;
            if ( logdumpOp.m_iVerbosityLevel != LOG::ldvlBasic )
            {
                DUMPPrintF( "\n      Log file: %ws", wszLogFileName );
            }
            else
            {
                DUMPPrintF( "      Log file: %ws", wszLogFileName );
            }
            err = pinst->m_plog->ErrLGDumpLog( wszLogFileName, &logdumpOp, plgfilehdr, &checksumLastSegment );
            if ( err < JET_errSuccess )
            {
                errDump = ( errDump >= JET_errSuccess ? err : errDump );
            }
            else if ( LOG::LOGDUMP_LOGHDR_VALIDADJACENT == logdumpOp.m_loghdr
                    && lgen != plgfilehdr->lgfilehdr.le_lGeneration
                    && lgen < lGenerationEXXDuringDump )
            {
                DUMPPrintF( "\n      %sERROR: Mismatched log generation %lu (0x%lX) with log file name. Expected generation %lu (0x%lX).\n",
                        ( ( logdumpOp.m_iVerbosityLevel != LOG::ldvlBasic ) ? "" : "          " ),
                        (ULONG)plgfilehdr->lgfilehdr.le_lGeneration, (LONG)plgfilehdr->lgfilehdr.le_lGeneration,
                        lgen, lgen );
                logdumpOp.m_loghdr = LOG::LOGDUMP_LOGHDR_VALID;
                LGIReportLogfileCorrupt( wszLogFileName, JET_errLogGenerationMismatch, pinst );
                errDump = ( errDump >= JET_errSuccess ? ErrERRCheck( JET_errLogGenerationMismatch ) : errDump );
            }
            else if ( logdumpOp.m_iVerbosityLevel == LOG::ldvlBasic )
            {
                //  just verifying, so report that logfile is OK
                DUMPPrintF( " - OK\n" );
            }

            Call( ErrFindNextGen( pinst, &dirloginfo, wszLogFileDir, wszLogExt, &lgen, &cLogDigitsCurr, wszLogFileName, sizeof(wszLogFileName) ) );

            if ( ( cLogDigits == 0 || cLogDigits == 5 || cLogDigits == 8 ) &&
                    (cLogDigitsCurr == 8 || cLogDigitsCurr == 5) )
            {
                // The first log returns zero digits.
                cLogDigits = cLogDigitsCurr;
            }
            // If we changed log digits (between 8 and 5), notice it.  Ignore it for other digitnesses.
            Assert( (cLogDigitsCurr != 8 && cLogDigitsCurr != 5) || (cLogDigitsCurr == cLogDigits) );

            //  if it is not jet.jtx/log file
            if ( lgen < lGenerationEXXDuringDump )
            {
                if ( lgen != lgenLast+1 )
                {
                    Assert( lgenLast < lgen );
                    if ( LOG::LOGDUMP_LOGHDR_VALIDADJACENT == logdumpOp.m_loghdr )
                    {
                        logdumpOp.m_loghdr = LOG::LOGDUMP_LOGHDR_VALID;
                    }
                    if ( logdumpOp.m_iVerbosityLevel != LOG::ldvlBasic )
                        DUMPPrintF( "\n\n" );
                    if ( lgen == lgenLast + 2 )
                    {
                        WCHAR  wszT[12] = L"tla"; // next func requires base name, though we won't use it below ...
                        LGFileHelper::LGSzLogIdAppend( wszT, sizeof(wszT), lgenLast+1, cLogDigits);
                        DUMPPrintF( "      Missing log file: %ws%ws%ws\n", wszLog, &(wszT[3]), wszLogExt );
                        LGIReportLogfilesMissing( wszLog, wszT + 3, NULL, wszLogExt, JET_errMissingLogFile, pinst );
                    }
                    else
                    {
                        WCHAR  wszT[12] = L"tla"; // next func requires base name, though we won't use it below ...
                        WCHAR  wszTT[12] = L"tla"; // next func requires base name, though we won't use it below ...
                        LGFileHelper::LGSzLogIdAppend( wszT, sizeof(wszT), lgenLast+1, cLogDigits);
                        LGFileHelper::LGSzLogIdAppend( wszTT, sizeof(wszTT), lgen-1, cLogDigits);
                        DUMPPrintF( "      Missing log files: %ws{%ws - %ws}%ws\n", wszLog, &(wszT[3]), &(wszTT[3]), wszLogExt );
                        LGIReportLogfilesMissing( wszLog, wszT + 3, wszTT + 3, wszLogExt, JET_errMissingLogFile, pinst );
                    }
                    errDump = ( errDump >= JET_errSuccess ? ErrERRCheck( JET_errMissingLogFile ) : errDump );
                }
            }

            if ( logdumpOp.m_iVerbosityLevel != LOG::ldvlBasic )
                DUMPPrintF( "\n" );
        }

        if ( errDump >= JET_errSuccess )
        {
            DUMPPrintF( "\nNo damaged log files were found.\n" );
        }
        else if ( !( grbit & JET_bitDBUtilOptionVerify ) )
        {
            DUMPPrintF( "\n" );
        }
        else
        {
            //  for checksum mode, eseutil will generate the whitespace on error
        }

        if (logdumpOp.m_pcwpfCsvOut
                && (JET_errSuccess != logdumpOp.m_pcwpfCsvOut->m_errLast))
        {
            DUMPPrintF( "\n      Cannot write csv file (%ws). Error %d.\n",
                    wszCsvDataFile, logdumpOp.m_pcwpfCsvOut->m_errLast);
            Call( ErrERRCheck( logdumpOp.m_pcwpfCsvOut->m_errLast ) );
        }
    }
    else
    {

        //
        // This is dumping a single specified log.
        //

        logdumpOp.m_loghdr = LOG::LOGDUMP_LOGHDR_NOHDR;

        // if verify, we don't print any output.
        // also verbose will be ignored
        if ( grbit & JET_bitDBUtilOptionVerify )
        {
            Assert( !( grbit & JET_bitDBUtilOptionDumpVerbose ) );
            Assert( !logdumpOp.m_fPrint );
            Assert( logdumpOp.m_iVerbosityLevel == LOG::ldvlBasic );
            logdumpOp.m_fVerifyOnly = 1;
        }
        else if ( grbit & JET_bitDBUtilOptionDumpLogInfoCSV )
        {
            Assert( wszCsvDataFile ); // otherwise we won't be printing to any file.
            logdumpOp.m_pcwpfCsvOut = &cpfCsvOut;
        }
        else
        {
            logdumpOp.m_fPrint = 1;

            if ( grbit & JET_bitDBUtilOptionDumpVerboseLevel1 )
            {
                logdumpOp.m_iVerbosityLevel = LOG::ldvlStructure;

                if ( grbit & JET_bitDBUtilOptionDumpVerboseLevel2 )
                {
                    logdumpOp.m_iVerbosityLevel = LOG::ldvlData;
                }
            }

            // Dump IO summary table
            if ( grbit & JET_bitDBUtilOptionDumpLogSummary )
            {
                logdumpOp.m_fSummary = 1;
            }

        }

        if ( grbit & JET_bitDBUtilOptionDumpLogPermitPatching )
        {
            logdumpOp.m_fPermitPatching = 1;
        }

        if ( !logdumpOp.m_fVerifyOnly )
        {
            DUMPPrintF( "      Base name: %ws\n", SzParam( pinst, JET_paramBaseName ) );
            DUMPPrintF( "      Log file: %ws", wszLog );
            if (logdumpOp.m_pcwpfCsvOut)
            {
                DUMPPrintF( "\n      Csv file: %ws", wszCsvDataFile );
                if (JET_errSuccess != logdumpOp.m_pcwpfCsvOut->m_errLast)
                {
                    DUMPPrintF( "\n      Cannot open csv file (%ws). Error %d.\n",
                            wszCsvDataFile, logdumpOp.m_pcwpfCsvOut->m_errLast);
                    Call( ErrERRCheck( logdumpOp.m_pcwpfCsvOut->m_errLast ) );
                }
            }
        }

        errDump = pinst->m_plog->ErrLGDumpLog( wszLog, &logdumpOp );

        if ( !logdumpOp.m_fVerifyOnly )
        {
            if (logdumpOp.m_pcwpfCsvOut
                    && (JET_errSuccess != logdumpOp.m_pcwpfCsvOut->m_errLast))
            {
                DUMPPrintF( "\n      Cannot write csv file (%ws). Error %d.\n",
                        wszCsvDataFile, logdumpOp.m_pcwpfCsvOut->m_errLast);
                Call( ErrERRCheck( logdumpOp.m_pcwpfCsvOut->m_errLast ) );
            }
            DUMPPrintF( "\n" );
        }
        else
        {
            //  for checksum mode, eseutil will generate the necessary whitespace
        }
    }

HandleError:
    logdumpOp.m_pcwpfCsvOut = NULL; // stack var will be freed ...
    if ( NULL != plgfilehdr )
    {
        OSMemoryPageFree( plgfilehdr );
    }
    if ( err >= JET_errSuccess && errDump < JET_errSuccess )
    {
        err = errDump;
    }

    return err;
}

ERR LOG::ErrLGIDumpOneAttachment( const ATTACHINFO * const pattachinfo, const LOGDUMP_OP * const plogdumpOp ) const
{
    Assert( NULL != pattachinfo );
    Assert( NULL != plogdumpOp );

    ERR err = JET_errSuccess;

    CAutoWSZ wszDbName;
    if ( pattachinfo->FUnicodeNames() )
    {
        Call( wszDbName.ErrSet( (UnalignedLittleEndian<WCHAR>*)pattachinfo->szNames ) );
    }
    else
    {
        Call( wszDbName.ErrSet( (CHAR*)(pattachinfo->szNames) ) );
    }

    if ( NULL == plogdumpOp->m_pcwpfCsvOut )
    {
        DUMPPrintF( "      %d %ws%ws\n",
                pattachinfo->Dbid(), (WCHAR *)wszDbName, pattachinfo->FSparseEnabledFile() ? L" (sparse)" : L"" );
        DUMPIAttachInfo( pattachinfo );
    }
    else
    {
        const WCHAR * const wszLogHeaderAttachInfo = L"LHAI";
        WCHAR rgwchSignBuf[3 * sizeof( pattachinfo->signDb ) + 1];
        for ( INT i = 0; i < sizeof( pattachinfo->signDb ); i++ )
        {
            OSStrCbFormatW( rgwchSignBuf + i * 3, sizeof( rgwchSignBuf ) - ( i * 3 * sizeof(rgwchSignBuf[0]) ), L"%02X ", ( (BYTE*)&pattachinfo->signDb )[i] );
        }
        const size_t cchHexDumped = 3 * sizeof( pattachinfo->signDb );
        rgwchSignBuf[ cchHexDumped ] = L'\0';
        (*(plogdumpOp->m_pcwpfCsvOut))( L"%s, %d, \"%s\", %s\r\n", wszLogHeaderAttachInfo, pattachinfo->Dbid(), (WCHAR *)wszDbName, rgwchSignBuf );
    }

HandleError:
    return err;
}

ERR LOG::ErrLGIDumpAttachments( const LOGDUMP_OP * const plogdumpOp ) const
{
    Assert( NULL != plogdumpOp );
    Assert( NULL != m_pLogStream->GetCurrentFileHdr() );
    Assert( NULL != m_pLogStream->GetCurrentFileHdr()->rgbAttach );

    ERR                 err         = JET_errSuccess;
    const ATTACHINFO *  pattachinfo = NULL;
    const BYTE *        pbT         = NULL;

    for ( pbT = m_pLogStream->GetCurrentFileHdr()->rgbAttach; 0 != *pbT; pbT += sizeof(ATTACHINFO) + pattachinfo->CbNames() )
    {
        if( pbT >= ( m_pLogStream->GetCurrentFileHdr()->rgbAttach + cbAttach ) )
        {
            Assert( fFalse );
            Error( ErrERRCheck( JET_errInternalError ) );
        }
        pattachinfo = (ATTACHINFO*)pbT;
        Call( ErrLGIDumpOneAttachment( pattachinfo, plogdumpOp ) );
    }

HandleError:
    return err;
}

extern const char * szUnknown;

ERR LOG::ErrLGDumpLog( __in PCWSTR wszLog, LOGDUMP_OP * const plogdumpOp, LGFILEHDR * const plgfilehdr, XECHECKSUM *pLastSegChecksum )
{
    ERR                 err;
    WCHAR               wszPathJetChkLog[ IFileSystemAPI::cchPathMax ];
    CHECKPOINT         *pcheckpoint     = NULL;
    LGPOS               lgposCheckpoint;
    LGPOS              *plgposCheckpoint = NULL;
    DWORD               cbSecVolume;

    if ( m_pinst->m_pfsapi->ErrPathComplete( wszLog, wszPathJetChkLog ) == JET_errInvalidPath )
    {
        DUMPPrintF( "\n                ERROR: File not found.\n" );
        err = ErrERRCheck( JET_errFileNotFound );
        LGIReportLogfileCorrupt( wszLog, err, m_pinst );
        return err;
    }

    err = m_pinst->m_pfsapi->ErrFileAtomicWriteSize( wszPathJetChkLog, &cbSecVolume );
    if ( err < 0 )
    {
        DUMPPrintF( "\n                ERROR: File error %d.\n", err );
        LGIReportLogfileCorrupt( wszPathJetChkLog, err, m_pinst );
        return err;
    }
    m_pLogStream->SetCbSecVolume( cbSecVolume );

    IFileAPI *pfapiLog;
    err = CIOFilePerf::ErrFileOpen( m_pinst->m_pfsapi, m_pinst, wszLog, IFileAPI::fmfReadOnly, iofileLog, qwDumpingFileID, &pfapiLog );
    if ( err < 0 )
    {
        DUMPPrintF( "\n                ERROR: Cannot open log file (%ws). Error %d.\n", wszLog, err );
        return err;
    }

    pcheckpoint = (CHECKPOINT *)PvOSMemoryPageAlloc( sizeof( CHECKPOINT ), NULL );
    if ( NULL != pcheckpoint )
    {
        WCHAR wszJetCheckpointFile[IFileSystemAPI::cchPathMax];
        WCHAR wszJetCheckpointFileName[IFileSystemAPI::cchPathMax];
        WCHAR wszJetCheckpointFileExt[IFileSystemAPI::cchPathMax];

        CallS( m_pinst->m_pfsapi->ErrPathParse( wszLog, wszJetCheckpointFile, wszJetCheckpointFileName, wszJetCheckpointFileExt ) );
        wszJetCheckpointFileName[JET_BASE_NAME_LENGTH] = 0;

        //  first try new checkpoint extension
        //
        CallS( m_pinst->m_pfsapi->ErrPathBuild( SzParam( m_pinst, JET_paramSystemPath ), wszJetCheckpointFileName, wszNewChkExt, wszJetCheckpointFile ) );
        err = ErrLGReadCheckpoint( wszJetCheckpointFile, pcheckpoint, fTrue );
        if ( JET_errCheckpointFileNotFound == err )
        {
            //  now try old checkpoint extension
            //
            CallS( m_pinst->m_pfsapi->ErrPathBuild( SzParam( m_pinst, JET_paramSystemPath ), wszJetCheckpointFileName, wszOldChkExt, wszJetCheckpointFile ) );
            err = ErrLGReadCheckpoint( wszJetCheckpointFile, pcheckpoint, fTrue );
        }

        if ( JET_errSuccess == err )
        {
            lgposCheckpoint = pcheckpoint->checkpoint.le_lgposCheckpoint;
            plgposCheckpoint = &lgposCheckpoint;
        }
        else
        {
            err = JET_errSuccess;
        }
        OSMemoryPageFree( pcheckpoint );
        pcheckpoint = NULL;
    }

    err = ErrLGDumpLog( pfapiLog, plogdumpOp, plgfilehdr, pLastSegChecksum, plgposCheckpoint );

    if ( err < JET_errSuccess )
    {
        LGIReportLogfileCorrupt( wszPathJetChkLog, err, m_pinst );
    }
    else
    {
        const BOOL  fShowIntegMsg   = ( plogdumpOp->m_fPrint || plogdumpOp->m_fVerifyOnly );

        if ( fShowIntegMsg )
        {
            if ( plogdumpOp->m_fPrint )
                DUMPPrintF( "\n" );
            DUMPPrintF( "Integrity check passed for log file: %ws\n", wszLog );
        }
    }

    return err;
}

ERR LOG::ErrLGDumpLog( IFileAPI *const pfapi, LOGDUMP_OP * const plogdumpOp, LGFILEHDR * const plgfilehdr, XECHECKSUM *pLastSegChecksum, const LGPOS * const plgposCheckpoint )
{
    Assert( NULL != plogdumpOp );
    Assert( !( ( LOGDUMP_LOGHDR_NOHDR == plogdumpOp->m_loghdr ) ^ (NULL == plgfilehdr ) ) );

    m_fDumpingLogs = fTrue;
    m_iDumpVerbosityLevel = plogdumpOp->m_iVerbosityLevel;

    ERR                 err;
    BOOL                fCloseNormally;
    BOOL                fCorrupt            = fFalse;
    ERR                 errCorrupt          = JET_errSuccess;
    BOOL                fIsPatchable        = fFalse;
    LE_LGPOS            le_lgposFirstT;
    BYTE                *pbNextLR;
    const BOOL          fPrint              = plogdumpOp->m_fPrint;
    const BOOL          fVerifyOnly         = plogdumpOp->m_fVerifyOnly;
    const INT           loghdr              = plogdumpOp->m_loghdr;
    INT                 logRecPosCurr       = 0;
    const BOOL          fRecoveringSaved    = m_fRecovering;
    // WCHAR const *        szLogTrailerCorruptLog      = L"LTCL\r\n";
    WCHAR const *       szLogTrailerEndOfLog        = L"LTEL\r\n";

    ULONG               rgclrtyp[ lrtypMax ];
    ULONG               rgcb[ lrtypMax ];
    ULONG               cPageRef            = 0;
    ULONG               cPageRefAlloc       = 0;
    PageRef*            rgPageRef           = NULL;

    if ( LOGDUMP_LOGHDR_VALIDADJACENT == loghdr )
    {
        //  if we fail reading next header will keep the current one
        plogdumpOp->m_loghdr = LOGDUMP_LOGHDR_VALID;
    }

    //  initialize the sector sizes

    //  NOTE: m_cbSec, m_csecHeader, and m_csecLGFile will be setup
    //        when we call LOG::ErrLGReadFileHdr()
    m_fRecovering = fTrue;      /* behave like doing recovery */

    // point ourselves to the new log file and remember the name
    // which is necessary for distintinguishing between the last active
    // log and archived logs
    m_pLogStream->SetPfapi( pfapi );

    /* dump file header */
    Call( m_pLogStream->ErrLGInit() );

    // HACK:  pretend we are not init so we can auto-set logfile size after init
    m_pinst->m_fSTInit = fSTInitInProgress;
    err = m_pLogStream->ErrLGReadFileHdr( NULL, iorpDirectAccessUtil, NULL, fNoCheckLogID, fTrue );
    m_pinst->m_fSTInit = fSTInitDone;
    if ( err < 0 )
    {
        DUMPPrintF( "\n                ERROR: Cannot read log file header. Error %d.\n", err );
        goto HandleError;
    }

    if ( LOGDUMP_LOGHDR_VALIDADJACENT == loghdr && m_pLogStream->GetCurrentFileGen() != plgfilehdr->lgfilehdr.le_lGeneration + 1 )
    {
        //  edb.jtx/log is not the correct generation number (all other missing log file cases are caught in ErrDUMPLog())
        DUMPPrintF( "\n                ERROR: Missing log file(s). Log file is generation %lu (0x%lX), but expected generation is %u (0x%lX).\n",
                (ULONG)m_pLogStream->GetCurrentFileGen(), (LONG)m_pLogStream->GetCurrentFileGen(),
                (ULONG)(plgfilehdr->lgfilehdr.le_lGeneration + 1), (LONG)(plgfilehdr->lgfilehdr.le_lGeneration + 1) );
        Call( ErrERRCheck( JET_errMissingLogFile ) );
    }

    // dump checkpoint file lgpos information
    if ( fPrint )
    {
        DUMPPrintF( "\n      lGeneration: %u (0x%X)\n", (ULONG)m_pLogStream->GetCurrentFileGen(), (LONG)m_pLogStream->GetCurrentFileGen() );

        if ( plgposCheckpoint )
        {
            DUMPPrintF( "      Checkpoint: (0x%X,%X,%X)\n",
                    plgposCheckpoint->lGeneration,
                    plgposCheckpoint->isec,
                    plgposCheckpoint->ib );
        }
        else
        {
            DUMPPrintF( "      Checkpoint: NOT AVAILABLE\n" );
        }

        DUMPPrintF( "      creation time: " );
        DUMPPrintLogTime( &m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmCreate );
        DUMPPrintF( "\n" );
    }

    if ( LOGDUMP_LOGHDR_VALIDADJACENT == loghdr
            && 0 != memcmp( &m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmPrevGen, &plgfilehdr->lgfilehdr.tmCreate, sizeof( LOGTIME ) ) )
    {
        DUMPPrintF( "%sERROR: Invalid log sequence. Previous generation time is [",
                ( fPrint || fVerifyOnly ? "      " : "\n                " ) );
        DUMPPrintLogTime( &m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmPrevGen );
        DUMPPrintF( "], but expected [" );
        DUMPPrintLogTime( &plgfilehdr->lgfilehdr.tmCreate );
        DUMPPrintF( "].\n" );

        Call( ErrERRCheck( JET_errInvalidLogSequence ) );
    }

    if ( fPrint )
    {
        DUMPPrintF( "      prev gen time: " );
        DUMPPrintLogTime( &m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmPrevGen );
        DUMPPrintF( "\n" );
    }

    if ( LOGDUMP_LOGHDR_VALIDADJACENT == loghdr
            && 0 != m_pLogStream->GetCurrentFileHdr()->lgfilehdr.checksumPrevLogAllSegments
            && *pLastSegChecksum != m_pLogStream->GetCurrentFileHdr()->lgfilehdr.checksumPrevLogAllSegments )
    {
        DUMPPrintF( "%sERROR: Invalid log sequence. Previous generation accumulated segment checksum is 0x%016I64x, but expected 0x%016I64x.\n",
                ( fPrint || fVerifyOnly ? "      " : "\n                " ),
                (XECHECKSUM)m_pLogStream->GetCurrentFileHdr()->lgfilehdr.checksumPrevLogAllSegments,
                *pLastSegChecksum );

        Call( ErrERRCheck( JET_errLogSequenceChecksumMismatch ) );
    }

    if ( fPrint )
    {
        DUMPPrintF( "      prev gen accumulated segment checksum: 0x%016I64x\n",
                (XECHECKSUM)m_pLogStream->GetCurrentFileHdr()->lgfilehdr.checksumPrevLogAllSegments );
    }

    if ( LOGDUMP_LOGHDR_VALIDADJACENT == loghdr &&
            ( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor < plgfilehdr->lgfilehdr.le_ulMajor ||
              ( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor == plgfilehdr->lgfilehdr.le_ulMajor &&
                m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMinor < plgfilehdr->lgfilehdr.le_ulMinor ) ||
              ( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor == plgfilehdr->lgfilehdr.le_ulMajor &&
                m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMinor == plgfilehdr->lgfilehdr.le_ulMinor &&
                m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulUpdateMajor < plgfilehdr->lgfilehdr.le_ulUpdateMajor ) ||
              ( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulUpdateMinor < plgfilehdr->lgfilehdr.le_ulUpdateMinor ) ) )
    {
        DUMPPrintF( "%sWARNING: Log version walked backwards. Last LGVersion: (%d.%d.%d.%d). Prev LGVersion: (%d.%d.%d.%d).\n",
                ( fPrint || fVerifyOnly ? "      " : "\n                " ),
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMinor,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulUpdateMajor,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulUpdateMinor,
                (SHORT) plgfilehdr->lgfilehdr.le_ulMajor,
                (SHORT) plgfilehdr->lgfilehdr.le_ulMinor,
                (SHORT) plgfilehdr->lgfilehdr.le_ulUpdateMajor,
                (SHORT) plgfilehdr->lgfilehdr.le_ulUpdateMinor );
    }

    if ( fPrint )
    {
        DUMPPrintF( "      Format LGVersion: (%d.%d.%d.%d) (generated by %d)\n",
                (ULONG) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor,
                (ULONG) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMinor,
                (ULONG) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulUpdateMajor,
                (ULONG) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulUpdateMinor,
                (ULONG) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_efvEngineCurrentDiagnostic );
        DUMPPrintF( "      Engine LGVersion: (%d.%d.%d.%d)\n",
                (SHORT) ulLGVersionMajorMax,
                (SHORT) ulLGVersionMinorFinalDeprecatedValue,
                (SHORT) ulLGVersionUpdateMajorMax,
                (SHORT) ulLGVersionUpdateMinorMax );
    }

    if ( LOGDUMP_LOGHDR_VALIDADJACENT == loghdr
            && 0 != memcmp( &m_pLogStream->GetCurrentFileHdr()->lgfilehdr.signLog, &plgfilehdr->lgfilehdr.signLog, sizeof( SIGNATURE ) ) )
    {
        DUMPPrintF( "%sERROR: Invalid log signature: ", ( fPrint || fVerifyOnly ? "      " : "\n                " ) );
        DUMPPrintSig( &m_pLogStream->GetCurrentFileHdr()->lgfilehdr.signLog );
        DUMPPrintF( "             %s   Expected signature: ", ( fPrint || fVerifyOnly ? "" : "          " ) );
        DUMPPrintSig( &plgfilehdr->lgfilehdr.signLog );

        Call( ErrERRCheck( JET_errBadLogSignature ) );
    }

    WCHAR const * wszLogHeaderGeneralInfo   = L"LHGI";

    if( plogdumpOp->m_pcwpfCsvOut )
    {
        CHAR szLogSig[128]; // plenty of space
        WCHAR wszLogCreate[128]; // plenty of space
        WCHAR wszPrevLogCreate[128]; // plenty of space

        SigToSz( &m_pLogStream->GetCurrentFileHdr()->lgfilehdr.signLog, szLogSig, sizeof(szLogSig) );

        OSStrCbFormatW( wszLogCreate, sizeof(wszLogCreate),
                L"%02d/%02d/%04d %02d:%02d:%02d.%3.3d",
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmCreate.bMonth,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmCreate.bDay,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmCreate.bYear + 1900,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmCreate.bHours,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmCreate.bMinutes,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmCreate.bSeconds,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmCreate.Milliseconds());
        OSStrCbFormatW( wszPrevLogCreate, sizeof(wszPrevLogCreate),
                L"%02d/%02d/%04d %02d:%02d:%02d.%3.3d",
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmPrevGen.bMonth,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmPrevGen.bDay,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmPrevGen.bYear + 1900,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmPrevGen.bHours,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmPrevGen.bMinutes,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmPrevGen.bSeconds,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmPrevGen.Milliseconds());

        (*(plogdumpOp->m_pcwpfCsvOut))(L"%ws, %hs, %08.08X, %08.08X, %ws, %ws, %d.%d.%d.%d, %ws, %d\r\n",
                wszLogHeaderGeneralInfo, szLogSig, (ULONG)m_pLogStream->GetCurrentFileGen(),
                (ULONG)m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulChecksum,
                wszLogCreate, wszPrevLogCreate,
                (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor, (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMinor, (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulUpdateMajor, (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulUpdateMinor,
                (m_pLogStream->GetCurrentFileHdr()->lgfilehdr.fLGFlags & fLGCircularLoggingCurrent) ? L"0x1" : L"0x0",
                (ULONG)(SHORT)m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec
                );

    }

    if ( fPrint )
    {
        DBMS_PARAM dbms_param = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.dbms_param;

        DUMPPrintF( "      Signature: " );
        DUMPPrintSig( &m_pLogStream->GetCurrentFileHdr()->lgfilehdr.signLog );
        DUMPPrintF( "      Env SystemPath: %s\n",
                m_pLogStream->GetCurrentFileHdr()->lgfilehdr.dbms_param.szSystemPathDebugOnly);
        DUMPPrintF( "      Env LogFilePath: %s\n",
                m_pLogStream->GetCurrentFileHdr()->lgfilehdr.dbms_param.szLogFilePathDebugOnly);
        char * szDiskSecSizeMismatch = NULL;
        switch( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.bDiskSecSizeMismatch )
        {
            case bTrueDiskSizeUnknown:
                szDiskSecSizeMismatch = " (legacy, unknown actual)";
                break;
            case bTrueDiskSizeMatches:
                szDiskSecSizeMismatch = " (matches)";
                break;
            case bTrueDiskSize512:
                szDiskSecSizeMismatch = " (actual @ log gen 512)";
                break;
            case bTrueDiskSize1024:
                szDiskSecSizeMismatch = " (actual @ log gen 1024)";
                break;
            case bTrueDiskSize2048:
                szDiskSecSizeMismatch = " (actual @ log gen 2048)";
                break;
            case bTrueDiskSize4096:
                szDiskSecSizeMismatch = " (actual @ log gen 4096)";
                break;
            case bTrueDiskSizeDiff:
                szDiskSecSizeMismatch = " (actual @ log gen NON-STANDARD)";
                break;
            default:
                szDiskSecSizeMismatch = " (Unknown ENUM!)";
        }
        QWORD qwSize = 0;
        CallS( m_pLogStream->ErrFileSize( &qwSize ) );
        Assert( dbms_param.le_lcsecLGFile == qwSize / m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec );
        Assert( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_csecLGFile == qwSize / m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec );
        DUMPPrintF( "      Env Log Sec size: %d%hs\n",  (SHORT) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec, szDiskSecSizeMismatch );
        DUMPPrintF( "      Env (CircLog,Session,Opentbl,VerPage,Cursors,LogBufs,LogFile,Buffers)\n");
        DUMPPrintF( "          (%s,%7lu,%7lu,%7lu,%7lu,%7lu,%7lu,%7lu)\n",
                ( dbms_param.fDBMSPARAMFlags & fDBMSPARAMCircularLogging ? "     on" : "    off" ),
                ULONG( dbms_param.le_lSessionsMax ),
                ULONG( dbms_param.le_lOpenTablesMax ),
                ULONG( dbms_param.le_lVerPagesMax ),
                ULONG( dbms_param.le_lCursorsMax ),
                ULONG( dbms_param.le_lLogBuffers ),
                ULONG( dbms_param.le_lcsecLGFile ),
                ULONG( dbms_param.le_ulCacheSizeMax ) );
        DUMPPrintF( "      Using Reserved Log File: %s\n",
                ( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.fLGFlags & fLGReservedLog ) ? "true" : "false" );
        DUMPPrintF( "      Circular Logging Flag (current file): %s\n",
                ( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.fLGFlags & fLGCircularLoggingCurrent ) ? "on":"off" );
        DUMPPrintF( "      Circular Logging Flag (past files): %s\n",
                ( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.fLGFlags & fLGCircularLoggingHistory ) ? "on": "off" );
        DUMPPrintF( "      Checkpoint at log creation time: (0x%X,%X,%X)\n",
                (LONG)m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_lgposCheckpoint.le_lGeneration,
                (USHORT)m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_lgposCheckpoint.le_isec,
                (USHORT)m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_lgposCheckpoint.le_ib );
    }

    if ( fPrint || plogdumpOp->m_pcwpfCsvOut )
    {
        err = ErrLGIDumpAttachments( plogdumpOp );
        if ( err < 0 )
        {
            DUMPPrintF( "      ERROR: Cannot read database attachment list. Error %d.\n", err );
            goto HandleError;
        }
        DUMPPrintF( "\n" );
    }

    if ( LOGDUMP_LOGHDR_NOHDR != plogdumpOp->m_loghdr )
    {
        //  we can set new current header for sure
        Assert( NULL != plgfilehdr );
        Assert( NULL != pLastSegChecksum );
        plogdumpOp->m_loghdr = LOGDUMP_LOGHDR_VALIDADJACENT;
    }

    /*  set buffer
    /**/
    m_pLogStream->LGSetSectorGeometry( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec, m_pLogStream->CSecLGFile() );

    m_pLogWriteBuffer->SetFNewRecordAdded( fFalse );

    // doing what we're doing here is similar to recovery in redo mode.
    // Quell UlComputeChecksum()'s checking to see if we're in m_critLGBuf
    m_fRecoveringMode = fRecoveringRedo;

    Call( m_pLogReadBuffer->ErrLReaderInit( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_csecLGFile ) );
    Call( m_pLogReadBuffer->ErrReaderEnsureLogFile() );
    err = m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF(
            &fCloseNormally,
            fTrue,                                                          //  compute accumulated segment checksum
            !plogdumpOp->m_fPermitPatching,                                 //  open read-only unless patching permitted
            ( plogdumpOp->m_fPermitPatching ? NULL : &fIsPatchable ) );     //  if patching permitted, we don't care if it's patchable (we'll just go ahead and patch it)

    m_fRecoveringMode = fRecoveringNone;

    if ( FErrIsLogCorruption( err ) )
    {
        Assert( !fIsPatchable );
        DUMPPrintF( "%sERROR: Log damaged (unusable). Last Lgpos: (0x%x,%X,%X). Error %d.\n",
                ( fPrint || fVerifyOnly ? "      " : "\n                " ),
                m_pLogReadBuffer->LgposFileEnd().lGeneration,
                m_pLogReadBuffer->LgposFileEnd().isec,
                m_pLogReadBuffer->LgposFileEnd().ib,
                err );

        fCorrupt = fTrue;
        errCorrupt = err;

        //  keep going to try to print out
        //  as many log records as possible
        err = JET_errSuccess;
    }

    else if ( JET_errSuccess != err )
    {
        DUMPPrintF( "%sERROR: Log file header is OK but integrity could not be verified. Error %d.\n",
                ( fPrint || fVerifyOnly ? "      " : "\n                " ),
                err );
        goto HandleError;
    }

    else if ( fPrint )
    {
        DUMPPrintF( "      Last Lgpos: (0x%x,%X,%X)\n",
                m_pLogReadBuffer->LgposFileEnd().lGeneration,
                m_pLogReadBuffer->LgposFileEnd().isec,
                m_pLogReadBuffer->LgposFileEnd().ib );
        DUMPPrintF( "      Accumulated segment checksum: 0x%016I64x\n", m_pLogStream->GetAccumulatedSectorChecksum());
    }

    // initialize other variables
    memset( rgclrtyp, 0, sizeof( rgclrtyp ) );
    memset( rgcb, 0, sizeof( rgcb ) );

#ifdef DEBUG
    m_fDBGTraceRedo = fTrue;
    if ( plogdumpOp->m_iVerbosityLevel != ldvlBasic )
        DUMPPrintF( "\n" );
#endif  //  DEBUG

    le_lgposFirstT.le_lGeneration = m_pLogStream->GetCurrentFileGen();
    le_lgposFirstT.le_isec = (WORD) m_pLogStream->CSecHeader();
    le_lgposFirstT.le_ib = 0;

    err = m_pLogReadBuffer->ErrLGLocateFirstRedoLogRecFF( &le_lgposFirstT, &pbNextLR );
    if ( err == errLGNoMoreRecords )
    {
        fCorrupt = fTrue;
        errCorrupt = ErrERRCheck( JET_errLogFileCorrupt );
    }

    while ( JET_errSuccess == err )
    {
        LR *plr = (LR *)pbNextLR;

        LRTYP lrtyp = plr->lrtyp;

        if ( lrtyp >= lrtypMax )
        {
            fCorrupt = fTrue;
            err = errCorrupt = ErrERRCheck( JET_errLogFileCorrupt );
            break;
        }

        AssertPREFIX( lrtyp < _countof( rgclrtyp ) );
        rgclrtyp[ lrtyp ]++;
        AssertPREFIX( lrtyp < _countof( rgcb ) );
        rgcb[ lrtyp ] += CbLGSizeOfRec( plr );

        Call( ErrLrToPageRef( m_pinst, plr, &cPageRef, &cPageRefAlloc, &rgPageRef ) );

        if ( plogdumpOp->m_iVerbosityLevel != ldvlBasic )
        {
            if ( GetNOP() > 0 )
            {
                CheckEndOfNOPList( plr, this );
            }

            if ( 0 == GetNOP() || plr->lrtyp != lrtypNOP )
            {
                PrintLgposReadLR();
                ShowLR( plr, this );
            }
        }

        if( plogdumpOp->m_pcwpfCsvOut )
        {
            LGPOS lgpos;
            m_pLogReadBuffer->GetLgposOfPbNext(&lgpos);
            Assert( lgpos.lGeneration == LONG(m_pLogStream->GetCurrentFileGen()) );
            lgpos.lGeneration = LONG(m_pLogStream->GetCurrentFileGen());
            Call( ErrLrToLogCsvSimple( plogdumpOp->m_pcwpfCsvOut, lgpos, plr, this ) );
        }

        logRecPosCurr++;

        err = m_pLogReadBuffer->ErrLGGetNextRecFF( &pbNextLR );
    }

    if ( err == errLGNoMoreRecords )
    {
        err = JET_errSuccess;
    }
    Call( err );
    CallS( err );

    if (plogdumpOp->m_pcwpfCsvOut)
    {
        // SOMEONE doesn't want the added dev/test cost of parsing LTCL along with LTEL and <nothing>.
        // So, a csv dump will end with LTEL (good log) or <nothing> (bad log or problems with csv file).
        // SOMEONE believes LTCL is needed because the corruption may have occcured after
        // the DB was updated with info past the corruption in the log. But SOMEONE's current design
        // doesn't need this refinement and so I am disabling LTCL at his request.
        // (*plogdumpOp->m_pcwpfCsvOut)((fCorrupt) ? szLogTrailerCorruptLog : szLogTrailerEndOfLog);
        if (!fCorrupt)
        {
            (*plogdumpOp->m_pcwpfCsvOut)(szLogTrailerEndOfLog);
        }
    }
    //  verbose dump
    if ( plogdumpOp->m_iVerbosityLevel != ldvlBasic )
    {
        if ( fCorrupt )
        {
            Assert( !fIsPatchable );
            Assert( errCorrupt < JET_errSuccess );
            DUMPPrintF( "      Last Lgpos: (0x%x,%X,%X)\n",
                    m_pLogReadBuffer->LgposFileEnd().lGeneration,
                    m_pLogReadBuffer->LgposFileEnd().isec,
                    m_pLogReadBuffer->LgposFileEnd().ib );
            DUMPPrintF( "\nLog Damaged (unusable): %ws\n\n", m_pLogStream->LogName() );
        }
#ifdef DEBUG
        else if ( fIsPatchable )
        {
            DUMPPrintF( ">%08.8X,%04.4X,%04.4X Log Damaged (PATCHABLE) -- soft recovery will fix this\n",
                    m_pLogReadBuffer->LgposFileEnd().lGeneration,
                    m_pLogReadBuffer->LgposFileEnd().isec,
                    m_pLogReadBuffer->LgposFileEnd().ib );
        }
        else if ( GetNOP() > 0 )
        {
            CheckEndOfNOPList( NULL, this );
        }
#endif // DEBUG

        DUMPPrintF( "\n" );

        float clrTotal = 0;
        float cbTotal = 0;
        for ( INT i = 0; i < lrtypMax; i++ )
        {
            clrTotal += (float)rgclrtyp[i];
            cbTotal += (float)rgcb[i];
        }
        DUMPPrintF( "clrTotal    =  %0.0f\n", clrTotal );
        DUMPPrintF( "cbTotal     =  %0.0f\n", cbTotal );
        DUMPPrintF( "\n" );

        DUMPPrintF( "======================================================""\n" );
        DUMPPrintF( "Op         # Records     Avg. Size      clr / cb Pct  ""\n" );
        DUMPPrintF( "------------------------------------------------------""\n" );

        for ( INT i = 0; i < lrtypMax; i++ )
        {
            //  Temporary hack
            //  Do not print replaced lrtyps
            if ( lrtypInit == i || lrtypTerm == i || lrtypRecoveryUndo == i || lrtypRecoveryQuit == i )
            {
                continue;
            }
#ifndef DEBUG
            //  Do not print the LRs with zero occurrence in retail build
            if ( rgclrtyp[i] == 0 )
            {
                continue;
            }
#endif // DEBUG
            const char *szLrTyp = SzLrtyp( (LRTYP)i );
            const BOOL fUnknown = !_stricmp( szLrTyp, szUnknown );
            if ( rgclrtyp[i] || !fUnknown )
            {
                const ULONG cbAvgSize   = ( rgclrtyp[i] > 0 ? rgcb[i]/rgclrtyp[i] : 0 );
                DUMPPrintF( "%s  %7lu       %7lu      %5.1f%% /%5.1f%%", szLrTyp, rgclrtyp[i], cbAvgSize,
                        ( (float)rgclrtyp[i] / clrTotal * 100.0 ), ( rgcb[i] / cbTotal * 100.0 ) );
                if ( fUnknown )
                {
                    DUMPPrintF( " (type %d)\n", i );
                }
                else
                {
                    DUMPPrintF( "\n", i );
                }
            }
        }

        DUMPPrintF( "======================================================""\n" );
    }

    if ( !fCorrupt && fPrint )
    {
        DUMPPrintF( "\n" );
        DUMPPrintF( "Number of database page references:  %u\n", cPageRef );

        if ( ( plogdumpOp->m_iVerbosityLevel != ldvlBasic ) || plogdumpOp->m_fSummary )
        {
            ULONG cbPage = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbPageSize;
            cbPage = cbPage == 0 ? g_cbPageDefault : cbPage;
            ULONG rgcPageRef[dbidMax];
            memset( rgcPageRef, 0, sizeof( rgcPageRef ) );
            ULONG cPageRefCache = 0;
            ULONG rgcPageRefCache[dbidMax];
            memset( rgcPageRefCache, 0, sizeof( rgcPageRefCache ) );
            ULONG cRead = 0;
            ULONG rgcRead[dbidMax];
            memset( rgcRead, 0, sizeof( rgcRead ) );
            ULONG cpgRead = 0;
            ULONG rgcpgRead[dbidMax];
            memset( rgcpgRead, 0, sizeof( rgcpgRead ) );
            ULONG cWrite = 0;
            ULONG rgcWrite[dbidMax];
            memset( rgcWrite, 0, sizeof( rgcWrite ) );
            ULONG cpgWrite = 0;
            ULONG rgcpgWrite[dbidMax];
            memset( rgcpgWrite, 0, sizeof( rgcpgWrite ) );
            sort( rgPageRef, rgPageRef + cPageRef );
            PageRef pagerefLastRead = PageRef( dbidMax, pgnoMax );
            PageRef pagerefRunStartRead = PageRef( dbidMax, pgnoMax );
            PageRef pagerefLastWrite = PageRef( dbidMax, pgnoMax );
            PageRef pagerefRunStartWrite = PageRef( dbidMax, pgnoMax );
            for ( ULONG iPageRef = 0; iPageRef < cPageRef; iPageRef++ )
            {
                const PageRef pagerefLast = iPageRef == 0 ? PageRef( dbidMax, pgnoMax ) : rgPageRef[iPageRef - 1];
                const PageRef pagerefCurr = rgPageRef[iPageRef];

                rgcPageRef[pagerefCurr.dbid]++;
                if ( pagerefCurr != pagerefLast )
                {
                    cPageRefCache++;
                    rgcPageRefCache[pagerefCurr.dbid]++;

                    if ( pagerefCurr.fRead )
                    {
                        if ( pagerefCurr.dbid != pagerefLastRead.dbid || pagerefCurr.pgno - pagerefLastRead.pgno != 1 )
                        {
                            if ( pagerefCurr.dbid != pagerefLastRead.dbid
                                    || (ULONG64)( pagerefCurr.pgno - pagerefLastRead.pgno - 1 ) * cbPage > UlParam( JET_paramMaxCoalesceReadGapSize )
                                    || (ULONG64)( pagerefCurr.pgno - pagerefRunStartRead.pgno + 1 ) * cbPage > UlParam( JET_paramMaxCoalesceReadSize ) )
                            {
                                // this page is not contiguous and is either too far away to combine into one IO via over-read
                                // or exceeds our max read length so this counts as a new read
                                pagerefRunStartRead = pagerefCurr;
                                cRead++;
                                rgcRead[pagerefCurr.dbid]++;
                                cpgRead++;
                                rgcpgRead[pagerefCurr.dbid]++;
                            }
                            else
                            {
                                // this page is not contiguous and is near enough to combine into one IO via over-read
                                cpgRead += pagerefCurr.pgno - pagerefLastRead.pgno;
                                rgcpgRead[pagerefCurr.dbid] += pagerefCurr.pgno - pagerefLastRead.pgno;
                            }
                        }
                        else
                        {
                            if ( (ULONG64)( pagerefCurr.pgno - pagerefRunStartRead.pgno + 1 ) * cbPage > UlParam( JET_paramMaxCoalesceReadSize ) )
                            {
                                // this page is contiguous but exceeds our max read length so this counts as a new read
                                pagerefRunStartRead = pagerefCurr;
                                cRead++;
                                rgcRead[pagerefCurr.dbid]++;
                            }

                            // this page is contiguous
                            cpgRead++;
                            rgcpgRead[pagerefCurr.dbid]++;
                        }

                        pagerefLastRead = pagerefCurr;
                    }
                    if ( pagerefCurr.fWrite )
                    {
                        if ( pagerefCurr.dbid != pagerefLastWrite.dbid || pagerefCurr.pgno - pagerefLastWrite.pgno != 1 )
                        {
                            if ( pagerefCurr.dbid != pagerefLastWrite.dbid
                                    || pagerefCurr.pgno != pagerefLastWrite.pgno
                                    || (ULONG64)( pagerefCurr.pgno - pagerefRunStartWrite.pgno + 1 ) * cbPage > UlParam( JET_paramMaxCoalesceWriteSize ) )
                            {
                                // this page is not contiguous and exceeds our max write length so this counts as a new write
                                // NOTE:  we do not model write gaps because that requires us to model the cache to fill the gap
                                pagerefRunStartWrite = pagerefCurr;
                                cWrite++;
                                rgcWrite[pagerefCurr.dbid]++;
                                cpgWrite++;
                                rgcpgWrite[pagerefCurr.dbid]++;
                            }
                            else
                            {
                                // this page is not contiguous and is near enough to combine into one IO via over-write
                                cpgWrite += pagerefCurr.pgno - pagerefLastWrite.pgno;
                                rgcpgWrite[pagerefCurr.dbid] += pagerefCurr.pgno - pagerefLastWrite.pgno;
                            }
                        }
                        else
                        {
                            if ( (ULONG64)( pagerefCurr.pgno - pagerefRunStartWrite.pgno + 1 ) * cbPage > UlParam( JET_paramMaxCoalesceWriteSize ) )
                            {
                                // this page is contiguous but exceeds our max write length so this counts as a new write
                                pagerefRunStartWrite = pagerefCurr;
                                cWrite++;
                                rgcWrite[pagerefCurr.dbid]++;
                            }

                            // this page is contiguous
                            cpgWrite++;
                            rgcpgWrite[pagerefCurr.dbid]++;
                        }

                        pagerefLastWrite = pagerefCurr;
                    }
                }
            }

            DUMPPrintF( "\n" );
            DUMPPrintF( "======================================================================================================""\n" );
            DUMPPrintF( "DBID   # Page Refs  # Cache Pages  # IOs  # Pages IO  # Reads  # Pages Read  # Writes  # Pages Written""\n" );
            DUMPPrintF( "------------------------------------------------------------------------------------------------------""\n" );
            for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
            {
                if ( rgcPageRef[dbid] > 0 )
                {
                    DUMPPrintF( "%5u  %11u  %13u  %5u  %10u  %7u  %12u  %8u  %15u\n",
                            dbid,
                            rgcPageRef[dbid],
                            rgcPageRefCache[dbid],
                            rgcRead[dbid] + rgcWrite[dbid],
                            rgcpgRead[dbid] + rgcpgWrite[dbid],
                            rgcRead[dbid],
                            rgcpgRead[dbid],
                            rgcWrite[dbid],
                            rgcpgWrite[dbid] );
                }
            }
            DUMPPrintF( "%5s  %11u  %13u  %5u  %10u  %7u  %12u  %8u  %15u\n",
                    "Total",
                    cPageRef,
                    cPageRefCache,
                    cRead + cWrite,
                    cpgRead + cpgWrite,
                    cRead,
                    cpgRead,
                    cWrite,
                    cpgWrite );
            DUMPPrintF( "======================================================================================================""\n" );
        }
    }


HandleError:
    m_pLogWriteBuffer->SetFNewRecordAdded( fTrue );

    if ( NULL != m_pLogStream->GetCurrentFileHdr() && LOGDUMP_LOGHDR_VALIDADJACENT == plogdumpOp->m_loghdr )
    {
        Assert( NULL != plgfilehdr );
        memcpy( plgfilehdr, m_pLogStream->GetCurrentFileHdr(), sizeof( LGFILEHDR ) );
        *pLastSegChecksum = m_pLogStream->GetAccumulatedSectorChecksum();
    }

    ERR errT = m_pLogReadBuffer->ErrLReaderTerm();
    if ( err == JET_errSuccess )
    {
        err = errT;
    }

    m_pLogStream->LGTerm();

    m_pLogStream->LGCloseFile();

    m_fRecovering = fRecoveringSaved;

    if ( err >= JET_errSuccess && errCorrupt < JET_errSuccess )
    {
        Assert( fCorrupt );
        err = errCorrupt;
    }

    m_fDumpingLogs = fFalse;

    delete[] rgPageRef;

    return err;
}


