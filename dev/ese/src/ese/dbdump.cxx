// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_dump.hxx"

LOCAL const char * const rgszDBState[] = {
                        "Illegal",
                        "Just Created",
                        "Dirty Shutdown",
                        "Clean Shutdown",
                        "Being Converted",
                        "Force Detach Replay Error",
                        "Incremental Reseed In Progress",
                        "Dirty and Patched Shutdown",
                        "Database Revert in Progress" };

const CHAR * SzFromState( ULONG dbstate )
{
    const CHAR * szState = NULL;
    switch ( dbstate )
    {
        case JET_dbstateJustCreated:
        case JET_dbstateDirtyShutdown:
        case JET_dbstateCleanShutdown:
        case JET_dbstateBeingConverted:
        case JET_dbstateIncrementalReseedInProgress:
        case JET_dbstateDirtyAndPatchedShutdown:
        case JET_dbstateRevertInProgress:
            szState = rgszDBState[dbstate];
            break;
        default:
            szState = rgszDBState[0];
            break;
    }
    Assert( szState );
    return szState;
}


#ifdef MINIMAL_FUNCTIONALITY
const BOOL g_fDisableDumpPrintF = fFalse;
#else
extern BOOL g_fDisableDumpPrintF;
#endif

VOID __cdecl DUMPPrintF(const CHAR * fmt, ...)
{
    if ( g_fDisableDumpPrintF )
    {
        return;
    }
    
    va_list arg_ptr;
    va_start( arg_ptr, fmt );
    vprintf( fmt, arg_ptr );
    fflush( stdout );
    va_end( arg_ptr );
}

VOID DUMPPrintLogTime( const LOGTIME * const plt )
{
    DUMPPrintF( "%02d/%02d/%04d %02d:%02d:%02d.%3.3d %s",
        ( SHORT )plt->bMonth, ( SHORT )plt->bDay, ( SHORT )plt->bYear + 1900,
        ( SHORT )plt->bHours, ( SHORT )plt->bMinutes, ( SHORT )plt->bSeconds,
        ( SHORT )plt->Milliseconds(), plt->fTimeIsUTC ? "UTC" : "LOC" );
}

VOID DUMPPrintSig( const SIGNATURE * const psig )
{
    CHAR rgSig[128];
    SigToSz( psig, rgSig, sizeof(rgSig) );
    DUMPPrintF( "%s\n", rgSig );
}

INLINE LOCAL VOID DUMPPrintBkinfo( BKINFO *pbkinfo, DBFILEHDR::BKINFOTYPE bkinfoType = DBFILEHDR::backupNormal)
{
    LONG    genLow, genHigh;
    LGPOS   lgpos;

    lgpos = pbkinfo->le_lgposMark;
    genLow = pbkinfo->le_genLow;
    genHigh = pbkinfo->le_genHigh;

    switch( bkinfoType )
    {
        case DBFILEHDR::backupNormal:
            DUMPPrintF( "        Log Gen: %u-%u (0x%x-0x%x)\n", genLow, genHigh, genLow, genHigh );
            break;

        case DBFILEHDR::backupOSSnapshot:
            DUMPPrintF( "        Log Gen: %u-%u (0x%x-0x%x) - OSSnapshot\n", genLow, genHigh, genLow, genHigh );
            break;

        case DBFILEHDR::backupSnapshot:
            DUMPPrintF( "        Log Gen: %u-%u (0x%x-0x%x) - Snapshot\n", genLow, genHigh, genLow, genHigh );
            break;

        default:
            AssertSz( FNegTest( fCorruptingDbHeaders ), "Invalid backup type" );
            DUMPPrintF( "        Log Gen: %u-%u (0x%x-0x%x) - UNKNOWN\n", genLow, genHigh, genLow, genHigh );
            break;
    }

    DUMPPrintF( "           Mark: (0x%X,%X,%X)\n", lgpos.lGeneration, lgpos.isec, lgpos.ib );

    DUMPPrintF( "           Mark: " );
    DUMPPrintLogTime( &pbkinfo->logtimeMark );
    DUMPPrintF( "\n" );
}

ERR LOCAL ErrDUMPHeaderStandard( INST *pinst, __in const DB_HEADER_READER* const pdbHdrReader )
{
    ERR                     err        = JET_errSuccess;
    LGPOS                   lgpos;
    DBFILEHDR_FIX* const    pdbfilehdr = reinterpret_cast<DBFILEHDR_FIX* const>( pdbHdrReader->pbHeader );

    DUMPPrintF( "\nFields:\n" );
    DUMPPrintF( "        File Type: %s\n", attribDb == pdbfilehdr->le_attrib ? "Database" : "UNKNOWN" );
    DUMPPrintF( "         Checksum: 0x%lx\n", LONG( pdbfilehdr->le_ulChecksum ) );
    DUMPPrintF( "   Format ulMagic: 0x%lx\n", LONG( pdbfilehdr->le_ulMagic ) );
    DUMPPrintF( "   Engine ulMagic: 0x%lx\n", ulDAEMagic );
    DUMPPrintF( " Format ulVersion: 0x%lx,%d,%d  (attached by %d)\n", LONG( pdbfilehdr->le_ulVersion ), ULONG( pdbfilehdr->le_ulDaeUpdateMajor ), ULONG( pdbfilehdr->le_ulDaeUpdateMinor ), ULONG( pdbfilehdr->le_efvMaxBinAttachDiagnostic ) );
    Assert( ulDAEVersionMax == PfmtversEngineMax()->dbv.ulDbMajorVersion );
    Assert( ulDAEUpdateMajorMax == PfmtversEngineMax()->dbv.ulDbUpdateMajor );
    DUMPPrintF( " Engine ulVersion: 0x%lx,%d,%d  (efvCurrent = %d)\n", PfmtversEngineMax()->dbv.ulDbMajorVersion, PfmtversEngineMax()->dbv.ulDbUpdateMajor, PfmtversEngineMax()->dbv.ulDbUpdateMinor, PfmtversEngineMax()->efv );
    DUMPPrintF( "Created ulVersion: 0x%lx,%d\n", LONG( pdbfilehdr->le_ulCreateVersion ), SHORT( pdbfilehdr->le_ulCreateUpdate ) );
    DUMPPrintF( "     DB Signature: " );
    DUMPPrintSig( &pdbfilehdr->signDb );
    DUMPPrintF( "         cbDbPage: %d\n", (ULONG)pdbfilehdr->le_cbPageSize );

    DBTIME dbtimeDirtied;
    dbtimeDirtied = pdbfilehdr->le_dbtimeDirtied;
    DUMPPrintF( "           dbtime: %I64u (0x%I64x)\n", dbtimeDirtied, dbtimeDirtied );

    DUMPPrintF( "            State: %s\n", SzFromState( pdbfilehdr->Dbstate() ) );

    DUMPPrintF(
                "     Log Required: %u-%u (0x%x-0x%x) [Min. Req. Pre-Redo: %u (0x%x)]\n",
                (ULONG) pdbfilehdr->le_lGenMinRequired,
                (ULONG) pdbfilehdr->le_lGenMaxRequired,
                (ULONG) pdbfilehdr->le_lGenMinRequired,
                (ULONG) pdbfilehdr->le_lGenMaxRequired,
                (ULONG) pdbfilehdr->le_lGenPreRedoMinRequired,
                (ULONG) pdbfilehdr->le_lGenPreRedoMinRequired );
    DUMPPrintF(
                "Gen. Max. Req. Creation Time: " );
    DUMPPrintLogTime( &pdbfilehdr->logtimeGenMaxRequired );
    DUMPPrintF( "\n" );
    DUMPPrintF(
                "    Log Committed: %u-%u (0x%x-0x%x)\n",
                (ULONG) 0,
                (ULONG) pdbfilehdr->le_lGenMaxCommitted,
                (ULONG) 0,
                (ULONG) pdbfilehdr->le_lGenMaxCommitted );
    DUMPPrintF(
                "Gen. Max. Com. Creation Time: " );
    DUMPPrintLogTime( &pdbfilehdr->logtimeGenMaxCreate );
    DUMPPrintF( "\n" );
    DUMPPrintF(
                "   Log Consistent: %u (0x%x) [Pre-Redo: %u (0x%x)]\n",
                (ULONG) pdbfilehdr->le_lGenMinConsistent,
                (ULONG) pdbfilehdr->le_lGenMinConsistent,
                (ULONG) pdbfilehdr->le_lGenPreRedoMinConsistent,
                (ULONG) pdbfilehdr->le_lGenPreRedoMinConsistent );
    DUMPPrintF(
                "   Log Recovering: %u (0x%x)\n",
                (ULONG) pdbfilehdr->le_lGenRecovering,
                (ULONG) pdbfilehdr->le_lGenRecovering );
    DUMPPrintF( "         Shadowed: %s\n", pdbfilehdr->FShadowingDisabled() ? "No" : "Yes" );
    DUMPPrintF( "       Last Objid: %u\n", (ULONG) pdbfilehdr->le_objidLast );
    DBTIME dbtimeLastScrub;
    dbtimeLastScrub = pdbfilehdr->le_dbtimeLastScrub;
    DUMPPrintF( "     Scrub Dbtime: %I64u (0x%I64x)\n", dbtimeLastScrub, dbtimeLastScrub );
    DUMPPrintF( "       Scrub Date: " );
    DUMPPrintLogTime( &pdbfilehdr->logtimeScrub );
    DUMPPrintF( "\n     Repair Count: %u\n", (ULONG) pdbfilehdr->le_ulRepairCount );
    DUMPPrintF( "      Repair Date: " );
    DUMPPrintLogTime( &pdbfilehdr->logtimeRepair );
    DUMPPrintF( "\n" );
    DUMPPrintF( " Old Repair Count: %u\n", (ULONG) pdbfilehdr->le_ulRepairCountOld );

    lgpos = pdbfilehdr->le_lgposConsistent;
    DUMPPrintF( "  Last Consistent: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );

    DUMPPrintLogTime( &pdbfilehdr->logtimeConsistent );
    DUMPPrintF( "\n" );

    lgpos = pdbfilehdr->le_lgposAttach;
    DUMPPrintF( "      Last Attach: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );

    DUMPPrintLogTime( &pdbfilehdr->logtimeAttach );
    DUMPPrintF( "\n" );

    lgpos = pdbfilehdr->le_lgposDetach;
    DUMPPrintF( "      Last Detach: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );

    DUMPPrintLogTime( &pdbfilehdr->logtimeDetach );
    DUMPPrintF( "\n" );

    lgpos = pdbfilehdr->le_lgposLastReAttach;
    DUMPPrintF( "    Last ReAttach: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );

    DUMPPrintLogTime( &pdbfilehdr->logtimeLastReAttach );
    DUMPPrintF( "\n" );

    DUMPPrintF( "             Dbid: %d\n", (SHORT) pdbfilehdr->le_dbid );

    DUMPPrintF( "    Log Signature: " );
    DUMPPrintSig( &pdbfilehdr->signLog );

    DUMPPrintF( "       OS Version: (%u.%u.%u SP %u NLS %x.%x)\n",
                (ULONG) pdbfilehdr->le_dwMajorVersion,
                (ULONG) pdbfilehdr->le_dwMinorVersion,
                (ULONG) pdbfilehdr->le_dwBuildNumber,
                (ULONG) pdbfilehdr->le_lSPNumber,
                (DWORD)( ( pdbfilehdr->le_qwSortVersion >> 32 ) & 0xFFFFFFFF ),
                (DWORD)( pdbfilehdr->le_qwSortVersion & 0xFFFFFFFF ) );

    if (    pdbfilehdr->le_ulIncrementalReseedCount ||
            pdbfilehdr->le_ulIncrementalReseedCountOld ||
            pdbfilehdr->le_ulPagePatchCount ||
            pdbfilehdr->le_ulPagePatchCountOld )
    {
        DUMPPrintF( "     Reseed Count: %u\n", (ULONG) pdbfilehdr->le_ulIncrementalReseedCount );
        DUMPPrintF( "      Reseed Date: " );
        DUMPPrintLogTime( &pdbfilehdr->logtimeIncrementalReseed );
        DUMPPrintF( "\n" );
        DUMPPrintF( " Old Reseed Count: %u\n", (ULONG) pdbfilehdr->le_ulIncrementalReseedCountOld );
        DUMPPrintF( "      Patch Count: %u\n", (ULONG) pdbfilehdr->le_ulPagePatchCount );
        DUMPPrintF( "       Patch Date: " );
        DUMPPrintLogTime( &pdbfilehdr->logtimePagePatch );
        DUMPPrintF( "\n" );
        DUMPPrintF( "  Old Patch Count: %u\n", (ULONG) pdbfilehdr->le_ulPagePatchCountOld );
    }

    DUMPPrintF( "\nPrevious Full Backup:\n" );
    DUMPPrintBkinfo( &pdbfilehdr->bkinfoFullPrev, pdbfilehdr->bkinfoTypeFullPrev );

    DUMPPrintF( "\nPrevious Incremental Backup:\n" );
    DUMPPrintBkinfo( &pdbfilehdr->bkinfoIncPrev, pdbfilehdr->bkinfoTypeIncPrev );

    DUMPPrintF( "\nPrevious Copy Backup:\n" );
    DUMPPrintBkinfo( &pdbfilehdr->bkinfoCopyPrev, pdbfilehdr->bkinfoTypeCopyPrev );

    DUMPPrintF( "\nPrevious Differential Backup:\n" );
    DUMPPrintBkinfo( &pdbfilehdr->bkinfoDiffPrev, pdbfilehdr->bkinfoTypeDiffPrev );

    DUMPPrintF( "\nCurrent Full Backup:\n" );
    DUMPPrintBkinfo( &pdbfilehdr->bkinfoFullCur );

    DUMPPrintF( "\nCurrent Shadow copy backup:\n" );
    DUMPPrintBkinfo( &pdbfilehdr->bkinfoSnapshotCur );

    DUMPPrintF( "\n" );
    DUMPPrintF( "     cpgUpgrade55Format: %d\n", (LONG)pdbfilehdr->le_cpgUpgrade55Format );
    DUMPPrintF( "    cpgUpgradeFreePages: %d\n", (LONG)pdbfilehdr->le_cpgUpgradeFreePages );
    DUMPPrintF( "cpgUpgradeSpaceMapPages: %d\n", (LONG)pdbfilehdr->le_cpgUpgradeSpaceMapPages );

    lgpos = pdbfilehdr->le_lgposLastResize;
    DUMPPrintF( "\n" );
    DUMPPrintF( "      Last Resize: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );
    if ( CmpLgpos( lgpos, pdbfilehdr->le_lgposAttach ) == 0 )
    {
        DUMPPrintF( "(Matches \"Last Attach\", no subsequent resize)" );
    }
    else if ( CmpLgpos( lgpos, pdbfilehdr->le_lgposConsistent ) == 0 )
    {
        DUMPPrintF( "(Matches \"Last Consistent\", no subsequent resize)" );
    }
    DUMPPrintF( "\n" );

    DUMPPrintF( "\n" );
    DUMPPrintF( "     Extend Count: %u\n", (ULONG) pdbfilehdr->le_ulExtendCount );
    DUMPPrintF( "      Extend Date: " );
    DUMPPrintLogTime( &pdbfilehdr->logtimeLastExtend );
    DUMPPrintF( "\n" );

    DUMPPrintF( "\n" );
    DUMPPrintF( "     Shrink Count: %u\n", (ULONG) pdbfilehdr->le_ulShrinkCount );
    DUMPPrintF( "      Shrink Date: " );
    DUMPPrintLogTime( &pdbfilehdr->logtimeLastShrink );
    DUMPPrintF( "\n" );

    DUMPPrintF( "\n" );
    DUMPPrintF( "       Trim Count: %u\n", (ULONG) pdbfilehdr->le_ulTrimCount );

    if (JET_filetypeDatabase == pdbfilehdr->le_filetype)
    {
        DUMPPrintF( "\n" );
        if ( pdbfilehdr->le_ulECCFixSuccess )
        {
            DUMPPrintF( "       ECC Fix Success Count: found (%u)\n", (ULONG) pdbfilehdr->le_ulECCFixSuccess );
            DUMPPrintF( "   Last ECC Fix Success Date: " );
            DUMPPrintLogTime( &pdbfilehdr->logtimeECCFixSuccess );
            DUMPPrintF( "\n" );
        }
        else
        {
            DUMPPrintF( "       ECC Fix Success Count: none\n" );
        }

        if ( pdbfilehdr->le_ulECCFixSuccessOld )
        {
            DUMPPrintF( "   Old ECC Fix Success Count: found (%u)\n", (ULONG) pdbfilehdr->le_ulECCFixSuccessOld );
        }
        else
        {
            DUMPPrintF( "   Old ECC Fix Success Count: none\n" );
        }
        
        if ( pdbfilehdr->le_ulECCFixFail )
        {
            DUMPPrintF( "         ECC Fix Error Count: found (%u)\n", (ULONG) pdbfilehdr->le_ulECCFixFail );
            DUMPPrintF( "     Last ECC Fix Error Date: " );
            DUMPPrintLogTime( &pdbfilehdr->logtimeECCFixFail );
            DUMPPrintF( "\n" );
        }
        else
        {
            DUMPPrintF( "         ECC Fix Error Count: none\n" );
        }

        if ( pdbfilehdr->le_ulECCFixFailOld )
        {
            DUMPPrintF( "     Old ECC Fix Error Count: found (%u)\n", (ULONG) pdbfilehdr->le_ulECCFixFailOld );
        }
        else
        {
            DUMPPrintF( "     Old ECC Fix Error Count: none\n" );
        }

        if ( pdbfilehdr->le_ulBadChecksum )
        {
            DUMPPrintF( "    Bad Checksum Error Count: found (%u)\n", (ULONG) pdbfilehdr->le_ulBadChecksum );
            DUMPPrintF( "Last Bad Checksum Error Date: " );
            DUMPPrintLogTime( &pdbfilehdr->logtimeBadChecksum );
            DUMPPrintF( "\n" );
        }
        else
        {
            DUMPPrintF( "    Bad Checksum Error Count: none\n" );
        }

        if ( pdbfilehdr->le_ulBadChecksumOld )
        {
            DUMPPrintF( "Old bad Checksum Error Count: found (%u)\n", (ULONG) pdbfilehdr->le_ulBadChecksumOld );
        }
        else
        {
            DUMPPrintF( "Old bad Checksum Error Count: none\n" );
        }

        DUMPPrintF( "\n");
        DUMPPrintF( "  Last Database Maintenance Finish Date: " );
        DUMPPrintLogTime( &pdbfilehdr->logtimeDbscanPrev );
        DUMPPrintF( "\n" );

        DUMPPrintF( "Current Database Maintenance Start Date: " );
        DUMPPrintLogTime( &pdbfilehdr->logtimeDbscanStart );
        DUMPPrintF( "\n" );

        DUMPPrintF( "      Highest Continuous Database Maintenance Page: %u\n", ( ULONG )( pdbfilehdr->le_pgnoDbscanHighestContinuous ) );
        DUMPPrintF( "      Highest Database Maintenance Page: %u\n", ( ULONG )( pdbfilehdr->le_pgnoDbscanHighest ) );
    }

    if ( pdbfilehdr->bkinfoFullCur.le_genLow && !pdbfilehdr->bkinfoFullCur.le_genHigh )
    {
        SIGNATURE signLog;
        SIGNATURE signDb;
        BKINFO bkinfoFullCur;

        PATCHHDR * ppatchHdr = (PATCHHDR *)pdbfilehdr;

        memcpy( &signLog, &pdbfilehdr->signLog, sizeof(signLog) );
        memcpy( &signDb, &pdbfilehdr->signDb, sizeof(signLog) );
        memcpy( &bkinfoFullCur, &pdbfilehdr->bkinfoFullCur, sizeof(BKINFO) );

        Call ( ErrDBReadAndCheckDBTrailer( pinst, pinst->m_pfsapi, pdbHdrReader->wszFileName, (BYTE *)ppatchHdr, g_cbPage ) );

        if ( memcmp( &signDb, &ppatchHdr->signDb, sizeof( SIGNATURE ) ) != 0 ||
             memcmp( &signLog, &ppatchHdr->signLog, sizeof( SIGNATURE ) ) != 0 ||
             CmpLgpos( &bkinfoFullCur.le_lgposMark, &ppatchHdr->bkinfo.le_lgposMark ) != 0 )
        {
            Error( ErrERRCheck( JET_errDatabasePatchFileMismatch ) );
        }

        DUMPPrintF( "\nPatch Current Full Backup:\n" );
        DUMPPrintBkinfo( &ppatchHdr->bkinfo );
        if ( ( ppatchHdr->lgenMinReq != 0 ) || ( ppatchHdr->lgenMaxReq != 0 ) || ( ppatchHdr->lgenMaxCommitted != 0 ) ||
             ppatchHdr->logtimeGenMaxRequired.FIsSet() || ppatchHdr->logtimeGenMaxCommitted.FIsSet() )
        {
            DUMPPrintF( "   Log Required: %u-%u (0x%x-0x%x)\n", (LONG)ppatchHdr->lgenMinReq, (LONG)ppatchHdr->lgenMaxReq, (LONG)ppatchHdr->lgenMinReq, (LONG)ppatchHdr->lgenMaxReq );
            DUMPPrintF(
                        "   Gen. Max. Req. Creation Time: " );
            DUMPPrintLogTime( &ppatchHdr->logtimeGenMaxRequired );
            DUMPPrintF( "  Log Committed: %u (0x%x)\n", (LONG)ppatchHdr->lgenMaxCommitted, (LONG)ppatchHdr->lgenMaxCommitted );
            DUMPPrintF(
                        "  Gen. Max. Com. Creation Time: " );
            DUMPPrintLogTime( &ppatchHdr->logtimeGenMaxCommitted );
            DUMPPrintF( "\n" );
        }
    }

    DUMPPrintF( "\n");
    DUMPPrintF( "  Database Header Flush Signature: " );
    DUMPPrintSig( &pdbfilehdr->signDbHdrFlush );
    DUMPPrintF( " Flush Map Header Flush Signature: " );
    DUMPPrintSig( &pdbfilehdr->signFlushMapHdrFlush );

    DUMPPrintF( "       RBS Header Flush Signature: " );
    DUMPPrintSig( &pdbfilehdr->signRBSHdrFlush );

    if ( pdbfilehdr->le_ulRevertCount > 0 || pdbfilehdr->le_ulRevertPageCount > 0 )
    {
        DUMPPrintF( "\n" );
        DUMPPrintF( "     Revert Count: %u\n", (ULONG) pdbfilehdr->le_ulRevertCount );
        DUMPPrintF( " Revert From Date: " );
        DUMPPrintLogTime( &pdbfilehdr->logtimeRevertFrom );
        DUMPPrintF( "\n" );
        DUMPPrintF( "   Revert To Date: " );
        DUMPPrintLogTime( &pdbfilehdr->logtimeRevertTo );
        DUMPPrintF( "\n" );
        DUMPPrintF( "Revert Page Count: %u\n", (ULONG) pdbfilehdr->le_ulRevertPageCount );

        lgpos = pdbfilehdr->le_lgposCommitBeforeRevert;
        DUMPPrintF( "Last Commit Before Revert: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );
        DUMPPrintF( "\n" );
    }

HandleError:
    return err;
}

#if defined( DEBUGGER_EXTENSION ) && defined ( DEBUG )

ERR LOCAL ErrDUMPHeaderStandardDebug( __in const DB_HEADER_READER* const pdbHdrReader )
{
    ERR err                           = JET_errSuccess;
    const DBFILEHDR* const pdbfilehdr = reinterpret_cast<DBFILEHDR* const>( pdbHdrReader->pbHeader );
    char* szBuf                       = NULL;

    DUMPPrintF( "\nFields:\n" );
    (VOID)( pdbfilehdr->Dump( CPRINTFSTDOUT::PcprintfInstance() ) );

    DUMPPrintF( "\nBinary Dump:\n" );
    const INT cbWidth = UtilCprintfStdoutWidth() >= 116 ? 32 : 16;

    Assert( pdbHdrReader->fNoAutoDetectPageSize );
    const INT cbBuffer = pdbHdrReader->cbHeader * 8;

    szBuf = new char[ cbBuffer ];
    if( !szBuf )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    DBUTLSprintHex( szBuf, cbBuffer, reinterpret_cast<BYTE *>( pdbHdrReader->pbHeader ), pdbHdrReader->cbHeader, cbWidth );
    DUMPPrintF( "%s\n", szBuf );

HandleError:
    delete[] szBuf;

    return err;
}

#endif

ERR LOCAL ErrDUMPHeaderHex( INST *pinst, __in const DB_HEADER_READER* const pdbHdrReader )
{
    ERR             err     = JET_errSuccess;

#if defined( DEBUGGER_EXTENSION ) && defined ( DEBUG )
    Call( ErrDUMPHeaderStandardDebug( pdbHdrReader ) );
#else
    Call( ErrDUMPHeaderStandard( pinst, pdbHdrReader ) );
#endif

HandleError:
    return err;
}

ERR LOCAL ErrDUMPHeaderFormat( INST *pinst, __in const DB_HEADER_READER* const pdbHdrReader, const BOOL fVerbose )
{
    ERR             err     = JET_errSuccess;

    DUMPPrintF( "Checksum Information:\n" );
    DUMPPrintF( "Expected Checksum: 0x%08I64x\n", pdbHdrReader->checksumExpected.rgChecksum[ 0 ] );
    DUMPPrintF( "  Actual Checksum: 0x%08I64x\n", pdbHdrReader->checksumActual.rgChecksum[ 0 ] );

    if ( fVerbose )
    {
        Call( ErrDUMPHeaderHex( pinst, pdbHdrReader ) );
    }
    else
    {
        Call( ErrDUMPHeaderStandard( pinst, pdbHdrReader ) );
    }

HandleError:
    return err;
}

ERR ErrDUMPHeader( INST *pinst, __in PCWSTR wszDatabase, const BOOL fVerbose )
{
    ERR             err             = JET_errSuccess;
    DBFILEHDR_FIX   *pdbfilehdrPrimary    = NULL;
    DBFILEHDR_FIX   *pdbfilehdrSecondary  = NULL;
    const DWORD     cbHeader        = g_cbPage;
    DB_HEADER_READER dbHeaderReaderPrimary =
    {
        headerRequestPrimaryOnly,
        wszDatabase,
        NULL,
        cbHeader,
        OffsetOf( DBFILEHDR_FIX, le_cbPageSize ),
        pinst->m_pfsapi,
        NULL,
        fTrue,
        0,
        0,
        0,
        shadowedHeaderCorrupt
    };
    DB_HEADER_READER dbHeaderReaderSecondary =
    {
        headerRequestSecondaryOnly,
        wszDatabase,
        NULL,
        cbHeader,
        OffsetOf( DBFILEHDR_FIX, le_cbPageSize ),
        pinst->m_pfsapi,
        NULL,
        fTrue,
        0,
        0,
        0,
        shadowedHeaderCorrupt
    };
    BOOL fIdentical             = fTrue;
    BOOL fMismatchPrimary       = fFalse;
    BOOL fZeroPrimary           = fFalse;
    BOOL fSizeMismatchPrimary   = fFalse;
    BOOL fMismatchSecondary     = fFalse;
    BOOL fZeroSecondary         = fFalse;
    BOOL fSizeMismatchSecondary = fFalse;
    BOOL fDatabaseUnusable      = fFalse;
    BOOL fCheckPageSize         = fTrue;

    Alloc( pdbfilehdrPrimary = ( DBFILEHDR_FIX* )PvOSMemoryPageAlloc( cbHeader, NULL ) );
    dbHeaderReaderPrimary.pbHeader = ( BYTE* )pdbfilehdrPrimary;
    Call( ErrUtilReadSpecificShadowedHeader( pinst, &dbHeaderReaderPrimary ) );
    if (dbHeaderReaderPrimary.shadowedHeaderStatus == shadowedHeaderCorrupt && FDefaultParam(JET_paramDatabasePageSize))
    {
        dbHeaderReaderPrimary.fNoAutoDetectPageSize = fFalse;
        Call( ErrUtilReadSpecificShadowedHeader( pinst, &dbHeaderReaderPrimary ) );
        if (dbHeaderReaderPrimary.cbHeaderActual == 2 * 1024)
        {
            fCheckPageSize = fFalse;
        }
    }
    fDatabaseUnusable       = ( dbHeaderReaderPrimary.shadowedHeaderStatus == shadowedHeaderCorrupt );
    fMismatchPrimary        = dbHeaderReaderPrimary.checksumActual != dbHeaderReaderPrimary.checksumExpected;
    fZeroPrimary            = dbHeaderReaderPrimary.checksumActual == 0;

    if ( fCheckPageSize )
    {
        Assert( dbHeaderReaderPrimary.fNoAutoDetectPageSize );
        fSizeMismatchPrimary    = dbHeaderReaderPrimary.cbHeaderActual != dbHeaderReaderPrimary.cbHeader;
    }
    
    if ( fVerbose ||
            dbHeaderReaderPrimary.shadowedHeaderStatus == shadowedHeaderOK ||
            dbHeaderReaderPrimary.shadowedHeaderStatus == shadowedHeaderSecondaryBad )
    {
        DUMPPrintF( "\nDATABASE HEADER:\n" );
        Call( ErrDUMPHeaderFormat( pinst, &dbHeaderReaderPrimary, fVerbose ) );
    }

    Alloc( pdbfilehdrSecondary = ( DBFILEHDR_FIX* )PvOSMemoryPageAlloc( cbHeader, NULL ) );
    dbHeaderReaderSecondary.pbHeader = ( BYTE* )pdbfilehdrSecondary;
    Call( ErrUtilReadSpecificShadowedHeader( pinst, &dbHeaderReaderSecondary ) );
    if (dbHeaderReaderSecondary.shadowedHeaderStatus == shadowedHeaderCorrupt && !fCheckPageSize)
    {
        dbHeaderReaderSecondary.fNoAutoDetectPageSize = fFalse;
        Call( ErrUtilReadSpecificShadowedHeader( pinst, &dbHeaderReaderSecondary ) );
    }
    fIdentical              = !memcmp( pdbfilehdrPrimary, pdbfilehdrSecondary, cbHeader ) && dbHeaderReaderPrimary.cbHeaderActual == dbHeaderReaderSecondary.cbHeaderActual;
    fMismatchSecondary      = dbHeaderReaderSecondary.checksumActual != dbHeaderReaderSecondary.checksumExpected;
    fZeroSecondary          = dbHeaderReaderSecondary.checksumActual == 0;

    if ( fCheckPageSize )
    {
        Assert( dbHeaderReaderSecondary.fNoAutoDetectPageSize );
        fSizeMismatchSecondary  = dbHeaderReaderSecondary.cbHeaderActual != dbHeaderReaderSecondary.cbHeader;
    }
    
    Assert( dbHeaderReaderPrimary.shadowedHeaderStatus == dbHeaderReaderSecondary.shadowedHeaderStatus );
    if ( fVerbose ||
            dbHeaderReaderSecondary.shadowedHeaderStatus == shadowedHeaderPrimaryBad )
    {
        DUMPPrintF( "\nDATABASE SHADOW HEADER:\n" );
        Call( ErrDUMPHeaderFormat( pinst, &dbHeaderReaderSecondary, fVerbose ) );
    }

HandleError:
    DUMPPrintF( "\n" );
    if ( !fIdentical )
    {
        DUMPPrintF( "WARNING: primary header and shadow header are not identical.\n" );
    }
    if ( fMismatchPrimary )
    {
        DUMPPrintF( "WARNING: primary header has a checksum error.\n" );
    }
    if ( fMismatchSecondary )
    {
        DUMPPrintF( "WARNING: shadow header has a checksum error.\n" );
    }
    if ( fZeroPrimary )
    {
        DUMPPrintF( "WARNING: primary header has a zeroed checksum.\n" );
    }
    if ( fZeroSecondary )
    {
        DUMPPrintF( "WARNING: shadow header has a zeroed checksum.\n" );
    }
    if ( fSizeMismatchPrimary && fCheckPageSize )
    {
        DUMPPrintF( "WARNING: primary header has an unexpected size of %lu bytes.\n", dbHeaderReaderPrimary.cbHeaderActual );
    }
    if ( fSizeMismatchSecondary && fCheckPageSize )
    {
        DUMPPrintF( "WARNING: shadow header has an unexpected size of %lu bytes.\n", dbHeaderReaderSecondary.cbHeaderActual );
    }
    if ( !fCheckPageSize && dbHeaderReaderPrimary.cbHeaderActual == 2048)
    {
        DUMPPrintF( "WARNING: this is a legacy database using a 2KB page size, which is no longer supported in this version of ESE.\n" );
    }
    if ( fDatabaseUnusable )
    {
        err = ErrERRCheck( JET_errDatabaseCorrupted );
    }

    OSMemoryPageFree( pdbfilehdrPrimary );
    OSMemoryPageFree( pdbfilehdrSecondary );

    return err;
}

ERR ErrDUMPFixupHeader( INST *pinst, __in PCWSTR wszDatabase, const BOOL fVerbose )
{
    ERR             err             = JET_errSuccess;
    DBFILEHDR_FIX   *pdbfilehdrPrimary    = NULL;
    const DWORD     cbHeader        = g_cbPage;
    DB_HEADER_READER dbHeaderReader =
    {
        headerRequestGoodOnly,
        wszDatabase,
        NULL,
        cbHeader,
        OffsetOf( DBFILEHDR_FIX, le_cbPageSize ),
        pinst->m_pfsapi,
        NULL,
        fFalse,
        0,
        0,
        0,
        shadowedHeaderCorrupt
    };

    Alloc( pdbfilehdrPrimary = ( DBFILEHDR_FIX* )PvOSMemoryPageAlloc( cbHeader, NULL ) );
    dbHeaderReader.pbHeader = ( BYTE* )pdbfilehdrPrimary;
    err = ErrUtilReadSpecificShadowedHeader( pinst, &dbHeaderReader );

    if ( err < 0 )
    {
        if ( JET_errReadVerifyFailure == err ||
            JET_errFileSystemCorruption == err ||
            JET_errDiskReadVerificationFailure == err )
        {
            err = ErrERRCheck( JET_errDatabaseCorrupted );
        }
        goto HandleError;
    }

    if( JET_filetypeUnknown != pdbfilehdrPrimary->le_filetype
        && JET_filetypeDatabase != pdbfilehdrPrimary->le_filetype
        && JET_filetypeTempDatabase != pdbfilehdrPrimary->le_filetype
        && JET_filetypeStreamingFile != pdbfilehdrPrimary->le_filetype )
    {
        Call( ErrERRCheck( JET_errFileInvalidType ) );
    }

    pdbfilehdrPrimary->SetDbstate( JET_dbstateCleanShutdown );
    memset( &pdbfilehdrPrimary->le_lgposConsistent, 0, sizeof( pdbfilehdrPrimary->le_lgposConsistent ) );
    memset( &pdbfilehdrPrimary->logtimeConsistent, 0, sizeof( pdbfilehdrPrimary->logtimeConsistent ) );
    memset( &pdbfilehdrPrimary->le_lgposAttach, 0, sizeof( pdbfilehdrPrimary->le_lgposAttach ) );
    memset( &pdbfilehdrPrimary->le_lgposLastResize, 0, sizeof( pdbfilehdrPrimary->le_lgposLastResize ) );
    pdbfilehdrPrimary->le_lGenMinRequired = 0;
    pdbfilehdrPrimary->le_lGenMaxRequired = 0;
    pdbfilehdrPrimary->le_lGenMinConsistent = 0;
    Call( ErrUtilWriteUnattachedDatabaseHeaders( pinst, pinst->m_pfsapi, wszDatabase, pdbfilehdrPrimary ) );

    Call( ErrDUMPHeader( pinst, wszDatabase, fVerbose ) );

HandleError:
    OSMemoryPageFree( pdbfilehdrPrimary );

    return err;
}


