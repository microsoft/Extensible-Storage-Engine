// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_dump.hxx"

VOID DUMPIRBSAttachInfo( const RBSATTACHINFO * const prbsattachinfo )
{
    Assert( prbsattachinfo );

    CAutoWSZPATH wszPath;
    (VOID)wszPath.ErrSet( prbsattachinfo->wszDatabaseName );
    DUMPPrintF( "%ws\n", (WCHAR *)wszPath );

    const DBTIME    dbtime  = prbsattachinfo->DbtimeDirtied();
    DUMPPrintF( "                dbtime: %I64u (0x%I64x)\n",
            dbtime, dbtime );

    const DBTIME dbtimePrev = prbsattachinfo->DbtimePrevDirtied();
    DUMPPrintF( "            dbtimePrev: %I64u (0x%I64x)\n",
            dbtimePrev, dbtimePrev );

    DUMPPrintF(
            "          Log Required: %u-%u (0x%x-0x%x)\n",
            (ULONG) prbsattachinfo->LGenMinRequired(),
            (ULONG) prbsattachinfo->LGenMaxRequired(),
            (ULONG) prbsattachinfo->LGenMinRequired(),
            (ULONG) prbsattachinfo->LGenMaxRequired() );

    DUMPPrintF( "           DbSignature: " );
    DUMPPrintSig( &prbsattachinfo->signDb );

    DUMPPrintF( "            DbHdrFlush: " );
    DUMPPrintSig( &prbsattachinfo->signDbHdrFlush );
}

LOCAL USHORT g_cbPageFromSnapshot = 0;

VOID LOCAL DUMPRBSHeaderStandard( INST *pinst, __in const DB_HEADER_READER* const pdbHdrReader )
{
    RBSFILEHDR* const    prbsfilehdr    = reinterpret_cast<RBSFILEHDR* const>( pdbHdrReader->pbHeader );

    DUMPPrintF( "\nFields:\n" );
    DUMPPrintF( "        File Type: %s\n", prbsfilehdr->rbsfilehdr.le_filetype == JET_filetypeSnapshot ? "Snapshot" : "UNKNOWN" );
    DUMPPrintF( "         Checksum: 0x%lx\n", LONG( prbsfilehdr->rbsfilehdr.le_ulChecksum ) );
    DUMPPrintF( "     Snapshot Gen: 0x%lx\n", LONG( prbsfilehdr->rbsfilehdr.le_lGeneration ) );

    DUMPPrintF("Snapshot Creation Time: " );
    DUMPPrintLogTime( &prbsfilehdr->rbsfilehdr.tmCreate );
    DUMPPrintF( "\n" );

    DUMPPrintF("Prev Snapshot Creation Time: " );
    DUMPPrintLogTime( &prbsfilehdr->rbsfilehdr.tmPrevGen );
    DUMPPrintF( "\n" );

    DUMPPrintF( "Format ulVersion: %d,%d\n", 
        ULONG( prbsfilehdr->rbsfilehdr.le_ulMajor ), 
        ULONG( prbsfilehdr->rbsfilehdr.le_ulMinor ) );

    DUMPPrintF( "cbLogicalFileSize: %I64u (0x%I64x)\n", (QWORD)prbsfilehdr->rbsfilehdr.le_cbLogicalFileSize, (QWORD)prbsfilehdr->rbsfilehdr.le_cbLogicalFileSize );

    DUMPPrintF( "Snapshot Header Flush Signature: " );
    DUMPPrintSig( &prbsfilehdr->rbsfilehdr.signRBSHdrFlush );

    g_cbPageFromSnapshot = prbsfilehdr->rbsfilehdr.le_cbDbPageSize;
    DUMPPrintF( "Database page size %u\n", (USHORT)prbsfilehdr->rbsfilehdr.le_cbDbPageSize );

    DUMPPrintF( "Logs copied: %s\n", prbsfilehdr->rbsfilehdr.bLogsCopied ? "true" : "false" );
    DUMPPrintF( "Logs copied Min gen: 0x%x, Max gen: 0x%x\n", (LONG)prbsfilehdr->rbsfilehdr.le_lGenMinLogCopied, (LONG)prbsfilehdr->rbsfilehdr.le_lGenMaxLogCopied );

    DUMPPrintF( "Snapshot Attach Infos:\n" );

    for( const BYTE * pbT = prbsfilehdr->rgbAttach; 0 != *pbT; pbT += sizeof( RBSATTACHINFO ) )
    {
        RBSATTACHINFO* prbsattachinfo = (RBSATTACHINFO*) pbT;

        if ( prbsattachinfo->FPresent() == 0 )
        {
            break;
        }

        DUMPIRBSAttachInfo( prbsattachinfo );
    }
}

#if defined( DEBUGGER_EXTENSION ) && defined ( DEBUG )

ERR LOCAL ErrDUMPRBSHeaderStandardDebug( __in const DB_HEADER_READER* const pdbHdrReader )
{
    ERR err                           = JET_errSuccess;
    const RBSFILEHDR* const prbsfilehdr = reinterpret_cast<RBSFILEHDR* const>( pdbHdrReader->pbHeader );
    char* szBuf                       = NULL;

    DUMPPrintF( "\nFields:\n" );
    (VOID)( prbsfilehdr->Dump( CPRINTFSTDOUT::PcprintfInstance() ) );

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

ERR LOCAL ErrDUMPRBSHeaderHex( INST *pinst, __in const DB_HEADER_READER* const pdbHdrReader )
{
#if defined( DEBUGGER_EXTENSION ) && defined ( DEBUG )
    return ErrDUMPRBSHeaderStandardDebug( pdbHdrReader );
#else
    DUMPRBSHeaderStandard( pinst, pdbHdrReader );
    return JET_errSuccess;
#endif
}

ERR LOCAL ErrDUMPRBSHeaderFormat( INST *pinst, __in const DB_HEADER_READER* const pdbHdrReader, const BOOL fVerbose )
{
    DUMPPrintF( "Checksum Information:\n" );
    DUMPPrintF( "Expected Checksum: 0x%08I64x\n", pdbHdrReader->checksumExpected.rgChecksum[ 0 ] );
    DUMPPrintF( "  Actual Checksum: 0x%08I64x\n", pdbHdrReader->checksumActual.rgChecksum[ 0 ] );

    if ( fVerbose )
    {
        return ErrDUMPRBSHeaderHex( pinst, pdbHdrReader );
    }
    else
    {
        DUMPRBSHeaderStandard( pinst, pdbHdrReader );
    }

    return JET_errSuccess;
}

const char * const szNOP                = "NOP";

const char * const szDbHdr              = "DbHdr";

const char * const szDbAttach           = "DbAttach";

const char * const szDbPage             = "DbPage";

const char * const szNewPage            = "NewPage";

const char * szRBSRecUnknown            = "*UNKNOWN*";

const INT   cbRBSRecBuf = 1024 + cbFormattedDataMax;

const char * SzRBSRec( BYTE bRBSRecType )
{
    switch ( bRBSRecType )
    {
        case rbsrectypeNOP:             return szNOP;
        case rbsrectypeDbHdr:           return szDbHdr;
        case rbsrectypeDbAttach:        return szDbAttach;
        case rbsrectypeDbPage:          return szDbPage;
        case rbsrectypeDbNewPage:       return szNewPage;
        default:                        return szRBSRecUnknown;
    }
}

VOID RBSRecToSz( const RBSRecord *prbsrec, __out_bcount(cbRBSRec) PSTR szRBSRec, ULONG cbRBSRec)
{
    Assert( prbsrec );

    BYTE    bRecType = prbsrec->m_bRecType;
    CHAR    rgchBuf[cbRBSRecBuf];

    if ( bRecType != rbsrectypeNOP )
    {
        OSStrCbFormatA( szRBSRec, cbRBSRec, " %s, %hu", SzRBSRec( bRecType ), (USHORT) prbsrec->m_usRecLength );
    }

    switch ( bRecType )
    {
        case rbsrectypeDbHdr:
        {
            RBSDbHdrRecord* prbsdbhdrrec = ( RBSDbHdrRecord* ) prbsrec;
                
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " [%u]",
                (DBID) prbsdbhdrrec->m_dbid );
            OSStrCbAppendA( szRBSRec, cbRBSRec, rgchBuf );
            break;
        }
        case rbsrectypeDbAttach:
        {
            RBSDbAttachRecord* prbsdbatchrec = ( RBSDbAttachRecord* ) prbsrec;
            CAutoWSZPATH wszPath;
            (VOID)wszPath.ErrSet( prbsdbatchrec->m_wszDbName );
                
            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " [%u], %ws",
                (DBID)prbsdbatchrec->m_dbid, (WCHAR *)wszPath );
            OSStrCbAppendA( szRBSRec, cbRBSRec, rgchBuf );
            break;
        }
        case rbsrectypeDbPage:
        {
            BYTE *pbDataDecompressed = NULL;
            DATA dataImage;
            BOOL fValidPage = fFalse;
            OBJID objid = 0;
            DBTIME dbtime = 0;

            RBSDbPageRecord* prbsdbpgrec = ( RBSDbPageRecord* ) prbsrec;
            dataImage.SetPv( prbsdbpgrec->m_rgbData );
            dataImage.SetCb( prbsrec->m_usRecLength - sizeof(RBSDbPageRecord) );

            if ( prbsdbpgrec->m_fFlags )
            {
                pbDataDecompressed = (BYTE *)PvOSMemoryPageAlloc( g_cbPageFromSnapshot, NULL );
                if ( pbDataDecompressed &&
                     ErrRBSDecompressPreimage( dataImage, g_cbPageFromSnapshot, pbDataDecompressed, prbsdbpgrec->m_pgno, prbsdbpgrec->m_fFlags ) >= JET_errSuccess )
                {
                    fValidPage = fTrue;
                }
            }
            else
            {
                fValidPage = fTrue;
            }
            if ( fValidPage )
            {
                CPAGE::PGHDR *pHdr = (CPAGE::PGHDR *)dataImage.Pv();
                objid = pHdr->objidFDP;
                dbtime = pHdr->dbtimeDirtied;
            }

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " [%u:%lu],[%s%s],objid:%d,dbtime:%I64x",
                (DBID)  prbsdbpgrec->m_dbid,
                (ULONG) prbsdbpgrec->m_pgno,
                ( prbsdbpgrec->m_fFlags & fRBSPreimageXpress ) ? "X" : "",
                ( prbsdbpgrec->m_fFlags & fRBSPreimageDehydrated ) ? "D" : "",
                objid, dbtime );
            OSStrCbAppendA( szRBSRec, cbRBSRec, rgchBuf );
            OSMemoryPageFree( pbDataDecompressed );
            break;
        }
        case rbsrectypeDbNewPage:
        {
            RBSDbNewPageRecord* prbsdbpgrec = ( RBSDbNewPageRecord* ) prbsrec;

            OSStrCbFormatA( rgchBuf, sizeof(rgchBuf), " [%u:%lu]",
                (DBID)  prbsdbpgrec->m_dbid,
                (ULONG) prbsdbpgrec->m_pgno );
            OSStrCbAppendA( szRBSRec, cbRBSRec, rgchBuf );
            break;
        }
        default:
        {
            EnforceSz( fFalse, OSFormat( "RBSRecToSzUnknownLr:%d", (INT)bRecType ) );
            break;
        }
    }
}

VOID ShowRBSRecord( const RBSRecord *prbsrec )
{
    CHAR    rgchBuf[cbRBSRecBuf];

    RBSRecToSz( prbsrec, rgchBuf, sizeof(rgchBuf) );
    (*CPRINTFSTDOUT::PcprintfInstance())( "%s\n", rgchBuf );
}

ERR LOCAL ErrDUMPRBSPageHex( _In_ IFileAPI* pfapirbs, _In_ const PGNO pgnoFirst, const PGNO pgnoLast )
{
    ERR err                           = JET_errSuccess;

    Assert( pfapirbs );

    #if defined( DEBUGGER_EXTENSION ) && defined ( DEBUG )
    BYTE* pbRBSBuffer                   = (BYTE *)PvOSMemoryPageAlloc( cbRBSBufferSize, NULL );
    QWORD qwStartOffset                 = IbRBSFileOffsetOfSegment( pgnoFirst );
    QWORD cbRemaining                   = IbRBSFileOffsetOfSegment( pgnoLast + 1) - qwStartOffset;
    
    TraceContextScope   tcUtil      ( iorpDirectAccessUtil );
    LONG  cbRead;

    DUMPPrintF( "\nBinary Dump:\n" );
    const INT cbWidth = UtilCprintfStdoutWidth() >= 116 ? 32 : 16;
    const INT cbBuffer = cbRBSBufferSize * 8;
    char* szBuf         = new char[ cbBuffer ];

    if( !szBuf )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    while ( cbRemaining > 0 )
    {
        cbRead = (LONG) min ( cbRemaining, (QWORD) cbRBSBufferSize );
        Call( pfapirbs->ErrIORead( *tcUtil, qwStartOffset, cbRead, pbRBSBuffer, qosIONormal ) );
        cbRemaining -= cbRead;
        qwStartOffset += cbRead;
        DBUTLSprintHex( szBuf, cbBuffer, pbRBSBuffer, cbRead, cbWidth );
        DUMPPrintF( "%s\n", szBuf );
    }

HandleError:
    if ( szBuf != NULL )
    {
        delete[] szBuf;
    }

    if ( pbRBSBuffer != NULL )
    {
        OSMemoryPageFree( pbRBSBuffer );
    }
    #endif

    return err;
}

ERR LOCAL ErrDUMPRBSPageFormat( CRevertSnapshot* prbs, PGNO pgnoLast )
{
    RBS_POS rbsposRecStart = rbsposMin;
    WCHAR wszErrorReason[ cbOSFSAPI_MAX_PATHW ];
    RBSRecord *prbsRecord;
    ERR err = JET_errSuccess;

    err = prbs->ErrGetNextRecord( &prbsRecord, &rbsposRecStart, wszErrorReason );

    while ( err != JET_wrnNoMoreRecords && rbsposRecStart.iSegment <= (INT)pgnoLast )
    {
        (*CPRINTFSTDOUT::PcprintfInstance())( ">%08X,%08X", rbsposRecStart.lGeneration, rbsposRecStart.iSegment);
        ShowRBSRecord( prbsRecord );
        
        err = prbs->ErrGetNextRecord( &prbsRecord, &rbsposRecStart, wszErrorReason );
    }
    
    if ( err == JET_errSuccess || err == JET_wrnNoMoreRecords )
    {
        return JET_errSuccess;
    }

    DUMPPrintF( "\nERROR: Cannot read next record %ws. Error %d\n", wszErrorReason, err );
    return err;
}

ERR ErrDUMPRBSHeader( INST *pinst, __in PCWSTR wszRBS, const BOOL fVerbose )
{
    ERR              err                    = JET_errSuccess;
    RBSFILEHDR      *prbsfilehdrPrimary      = NULL;
    RBSFILEHDR      *prbsfilehdrSecondary    = NULL;
    const DWORD     cbHeader                = sizeof( RBSFILEHDR );

    DB_HEADER_READER dbHeaderReaderPrimary  =
    {
        headerRequestPrimaryOnly,
        wszRBS,
        NULL,
        cbHeader,
        -1,
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
        wszRBS,
        NULL,
        cbHeader,
        -1,
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
    BOOL fRBSUnusable           = fFalse;
    BOOL fCheckPageSize         = fTrue;

    Alloc( prbsfilehdrPrimary = ( RBSFILEHDR* )PvOSMemoryPageAlloc( cbHeader, NULL ) );
    dbHeaderReaderPrimary.pbHeader = ( BYTE* )prbsfilehdrPrimary;
    Call( ErrUtilReadSpecificShadowedHeader( pinst, &dbHeaderReaderPrimary ) );

    fRBSUnusable        = ( dbHeaderReaderPrimary.shadowedHeaderStatus == shadowedHeaderCorrupt );
    fMismatchPrimary    = dbHeaderReaderPrimary.checksumActual != dbHeaderReaderPrimary.checksumExpected;
    fZeroPrimary        = dbHeaderReaderPrimary.checksumActual == 0;

    Assert( dbHeaderReaderPrimary.fNoAutoDetectPageSize );
    fSizeMismatchPrimary = dbHeaderReaderPrimary.cbHeaderActual != dbHeaderReaderPrimary.cbHeader;
    
    if ( fVerbose ||
            dbHeaderReaderPrimary.shadowedHeaderStatus == shadowedHeaderOK ||
            dbHeaderReaderPrimary.shadowedHeaderStatus == shadowedHeaderSecondaryBad )
    {
        DUMPPrintF( "\nSNAPSHOT HEADER:\n" );
        Call( ErrDUMPRBSHeaderFormat( pinst, &dbHeaderReaderPrimary, fVerbose ) );
    }

    Alloc( prbsfilehdrSecondary = ( RBSFILEHDR* )PvOSMemoryPageAlloc( cbHeader, NULL ) );
    dbHeaderReaderSecondary.pbHeader = ( BYTE* )prbsfilehdrSecondary;
    Call( ErrUtilReadSpecificShadowedHeader( pinst, &dbHeaderReaderSecondary ) );

    fIdentical              = !memcmp( prbsfilehdrPrimary, prbsfilehdrSecondary, cbHeader ) && dbHeaderReaderPrimary.cbHeaderActual == dbHeaderReaderSecondary.cbHeaderActual;
    fMismatchSecondary      = dbHeaderReaderSecondary.checksumActual != dbHeaderReaderSecondary.checksumExpected;
    fZeroSecondary          = dbHeaderReaderSecondary.checksumActual == 0;

    Assert( dbHeaderReaderSecondary.fNoAutoDetectPageSize );
    fSizeMismatchSecondary  = dbHeaderReaderSecondary.cbHeaderActual != dbHeaderReaderSecondary.cbHeader;
    
    Assert( dbHeaderReaderPrimary.shadowedHeaderStatus == dbHeaderReaderSecondary.shadowedHeaderStatus );
    if ( fVerbose ||
            dbHeaderReaderSecondary.shadowedHeaderStatus == shadowedHeaderPrimaryBad )
    {
        DUMPPrintF( "\nSNAPSHOT SHADOW HEADER:\n" );
        Call( ErrDUMPRBSHeaderFormat( pinst, &dbHeaderReaderSecondary, fVerbose ) );
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
    if ( fSizeMismatchPrimary )
    {
        DUMPPrintF( "WARNING: primary header has an unexpected size of %lu bytes.\n", dbHeaderReaderPrimary.cbHeaderActual );
    }
    if ( fSizeMismatchSecondary )
    {
        DUMPPrintF( "WARNING: shadow header has an unexpected size of %lu bytes.\n", dbHeaderReaderSecondary.cbHeaderActual );
    }
    if ( !fCheckPageSize && dbHeaderReaderPrimary.cbHeaderActual == 2048)
    {
        DUMPPrintF( "WARNING: this is a legacy database using a 2KB page size, which is no longer supported in this version of ESE.\n" );
    }
    if ( fRBSUnusable )
    {
        err = ErrERRCheck( JET_errRBSHeaderCorrupt );
    }

    OSMemoryPageFree( prbsfilehdrPrimary );
    OSMemoryPageFree( prbsfilehdrSecondary );

    return err;
}

ERR ErrDUMPRBSPage( INST *pinst, __in PCWSTR wszRBS, PGNO pgnoFirst, PGNO pgnoLast, const BOOL fVerbose )
{
    WCHAR wszRBSAbs[ cbOSFSAPI_MAX_PATHW ];
    QWORD pgStartOffset         = IbRBSFileOffsetOfSegment( pgnoFirst );
    QWORD pgEndOffset           = IbRBSFileOffsetOfSegment( pgnoFirst + 1) - 1;
    CRevertSnapshot* prbs       = NULL;
    IFileAPI *pfapirbs          = NULL;
    ERR err                     = JET_errSuccess;

    Call( ErrDUMPRBSHeader( pinst, wszRBS, fVerbose ) );

    if ( ( pgnoFirst < 1 ) ||
        ( ( pgnoLast != pgnoMax ) && ( pgnoFirst > pgnoLast ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( pinst->m_pfsapi->ErrPathComplete( wszRBS, wszRBSAbs ) == JET_errInvalidPath )
    {
        DUMPPrintF( "\n                ERROR: File not found.\n" );
        err = ErrERRCheck( JET_errFileNotFound );
        return err;
    }

    err = CIOFilePerf::ErrFileOpen( pinst->m_pfsapi, pinst, wszRBS, IFileAPI::fmfReadOnly, iofileRBS, qwDumpingFileID, &pfapirbs );
    if ( err < 0 )
    {
        DUMPPrintF( "\nERROR: Cannot open rbs file (%ws). Error %d.\n", wszRBS, err );
        return err;
    }

    prbs = new CRevertSnapshot( pinst );
    Call ( prbs->ErrSetRBSFileApi( pfapirbs ) );
    Call ( prbs->ErrSetReadBuffer( pgnoFirst ) );
    prbs->SetIsDumping();

    if( pgEndOffset > prbs->RBSFileHdr( )->rbsfilehdr.le_cbLogicalFileSize )
    {
        DUMPPrintF( "\nERROR: First page %lu(%I64u) is outside the logical size %I64u of revert snapshot file (%ws). \n", pgnoFirst, pgStartOffset, (QWORD) prbs->RBSFileHdr( )->rbsfilehdr.le_cbLogicalFileSize, wszRBS );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    pgnoLast = min( pgnoLast, IsegRBSSegmentOfFileOffset( prbs->RBSFileHdr( )->rbsfilehdr.le_cbLogicalFileSize - 1) );

    Call ( prbs->ErrSetReadBuffer( pgnoFirst ) );
    Call( ErrDUMPRBSPageFormat( prbs, pgnoLast ) );

    if ( fVerbose )
    {
        Call( ErrDUMPRBSPageHex( pfapirbs, pgnoFirst, pgnoLast ) );
    }

HandleError:
    if ( prbs != NULL )
    {
        delete prbs;
    }

    return err;
}
