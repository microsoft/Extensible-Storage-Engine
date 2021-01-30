// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifdef MINIMAL_FUNCTIONALITY
#else

#include "_dump.hxx"
#include "_bt.hxx"
#include "_space.hxx"


extern CPRINTFSTDOUT g_cprintfStdout;

CPRINTFINDENT g_cprintfIndent( &g_cprintfStdout);

const BYTE mpbb[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8',
                     '9', 'A', 'B', 'C', 'D', 'E', 'F' };

#define addPBI              g_cprintfIndent
#define flushPB( )
#define addIndent()         g_cprintfIndent.Indent()
#define delIndent()         g_cprintfIndent.Unindent()


#define MAX_KEY_SIZE_DISPLAY 8


typedef char * (*PfnFormatNodeInfo)( KEYDATAFLAGS * pNode );

LOCAL ERR ErrESEDUMPOneTable(FUCB *, __in PCSTR, JET_GRBIT);
LOCAL ERR ErrESEDUMPNodesForOneTree(FUCB *pfucbTable, JET_GRBIT grbitESEDUMPMode, PfnFormatNodeInfo pfnFormat = NULL);
LOCAL ERR ErrESEDUMPTables( JET_SESID , JET_DBID , __in PCWSTR, JET_GRBIT);
LOCAL ERR ErrESEDUMPIndexForOneTable(FUCB *, JET_GRBIT);
LOCAL ERR ErrESEDUMPCheckAndDumpSpaceInfo(FUCB *, JET_GRBIT);
LOCAL ERR ErrESEDUMPDatabaseInfo(PIB *, IFMP , JET_GRBIT);
LOCAL ERR ErrESEDUMPLVForOneTable(FUCB *, JET_GRBIT);
LOCAL VOID ESEDUMPSplitBufferDump(SPLIT_BUFFER & , FUCB * , JET_GRBIT);
LOCAL VOID ESEDUMPSingleSpaceHeaderDump(SPACE_HEADER &, FUCB * , JET_GRBIT);
LOCAL ERR ErrESEDUMPMultipleSpaceHeaderDump(SPACE_HEADER & , FUCB *, JET_GRBIT );


ERR ErrESEDUMPData( JET_SESID sesid, JET_DBUTIL_W *pdbutil )
{
    ERR         err;
    JET_DBID    ifmp                = JET_dbidNil;
    JET_GRBIT   grbitESEDUMPMode    = 0;
    
    Assert( NULL != pdbutil);
    Assert( sizeof(JET_DBUTIL_W) == pdbutil->cbStruct );
    Assert( NULL != pdbutil->szDatabase );

    CallR( ErrIsamAttachDatabase(
                sesid,
                pdbutil->szDatabase,
                fFalse,
                NULL,
                0,
                JET_bitDbReadOnly
                ) );
    Call( ErrIsamOpenDatabase(
                sesid,
                pdbutil->szDatabase,
                NULL,
                &ifmp,
                JET_bitDbExclusive | JET_bitDbReadOnly
                ) );
    Assert( JET_dbidNil != ifmp );
    
    grbitESEDUMPMode = pdbutil->grbitOptions;

    if (!pdbutil->szTable)
        Call ( ErrESEDUMPDatabaseInfo( (PIB *)sesid, ifmp, grbitESEDUMPMode ) );

    addIndent();

    if (!pdbutil->szTable || ( pdbutil->szTable && pdbutil->szTable[0] ) )
    {
        Call( ErrESEDUMPTables( sesid, ifmp, pdbutil->szTable, grbitESEDUMPMode ) );
    }

HandleError:
    if ( JET_dbidNil != ifmp )
    {
        (VOID)ErrIsamCloseDatabase( sesid, ifmp, NO_GRBIT );
    }
    
    (VOID)ErrIsamDetachDatabase( sesid, NULL, pdbutil->szDatabase );

    return err;
}


LOCAL ERR ErrESEDUMPTables( JET_SESID sesid, JET_DBID ifmp, __in PCWSTR wszTable, JET_GRBIT grbitESEDUMPMode )
{
    ERR         err;
    FUCB        *pfucbCatalog =     pfucbNil;
    const BYTE  bTrue =             0xff;

    CallR( ErrCATOpen( (PIB *)sesid, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call ( ErrIsamSetCurrentIndex(
                (PIB *)sesid,
                pfucbCatalog,
                szMSORootObjectsIndex ) );

    Assert( pfcbNil != pfucbCatalog->u.pfcb );
    Assert( pfucbCatalog->u.pfcb->FTypeTable() );
    Assert( pfucbCatalog->u.pfcb->FFixedDDL() );
    Assert( pfucbCatalog->u.pfcb->PgnoFDP() == pgnoFDPMSO );

    if (wszTable)
    {
        CAutoSZDDL  szTableName;
        
        Call( szTableName.ErrSet( wszTable ) );
        
        Call( ErrIsamMakeKey(
                    pfucbCatalog->ppib,
                    pfucbCatalog,
                    &bTrue,
                    sizeof(bTrue),
                    JET_bitNewKey ) );
        Call( ErrIsamMakeKey(
                    pfucbCatalog->ppib,
                    pfucbCatalog,
                    (BYTE *)(CHAR*)szTableName,
                    (ULONG)strlen((CHAR*)szTableName),
                    NO_GRBIT ) );
        err = ErrIsamSeek( pfucbCatalog->ppib, pfucbCatalog, JET_bitSeekEQ );
        if ( JET_errRecordNotFound == err )
        {
            addPBI("\n Table %s not found.\n", (CHAR*)szTableName);
            flushPB();
        }
        Call( err );

        Call (ErrESEDUMPOneTable(pfucbCatalog, (CHAR*)szTableName, grbitESEDUMPMode));
        
    }
    else
    {
        Call ( ErrIsamMove( pfucbCatalog->ppib,
                    pfucbCatalog,
                    JET_MoveFirst,
                    NO_GRBIT ));
                    
        do
        {
            DATA    dataField;
            CHAR    szTableName[JET_cbNameMost+1];
            
            Assert( !Pcsr( pfucbCatalog )->FLatched() );
            Call( ErrDIRGet( pfucbCatalog ) );
            Assert( Pcsr( pfucbCatalog )->FLatched() );
            
            const DATA& dataRec = pfucbCatalog->kdfCurr.data;
            

            Call( ErrRECIRetrieveVarColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Name,
                    dataRec,
                    &dataField ) );
            Assert( JET_errSuccess == err );

            const INT cbDataField = dataField.Cb();

            if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
            {
                AssertSz( fFalse, "Invalid fidMSO_Name column in catalog." );
                Error( ErrERRCheck( JET_errCatalogCorrupted ) );
            }
            Assert( sizeof( szTableName ) > cbDataField );
            UtilMemCpy( szTableName, dataField.Pv(), cbDataField );
            szTableName[cbDataField] = '\0';

            Assert( Pcsr( pfucbCatalog )->FLatched() );
            Call ( ErrDIRRelease( pfucbCatalog ) );
            Assert( !Pcsr( pfucbCatalog )->FLatched() );

            Call (ErrESEDUMPOneTable(pfucbCatalog, szTableName, grbitESEDUMPMode));

            err = ErrIsamMove(
                        pfucbCatalog->ppib,
                        pfucbCatalog,
                        JET_MoveNext,
                        NO_GRBIT );
        }
        while ( err >= 0);
        
        if ( JET_errNoCurrentRecord == err )
            err = JET_errSuccess;
    }
    
HandleError:
    Assert(pfucbNil != pfucbCatalog);
    CallS(ErrCATClose( (PIB *)sesid, pfucbCatalog ));
    return err;
}


LOCAL ERR ErrESEDUMPOneTable(FUCB *pfucbCatalog, __in PCSTR szTableName, JET_GRBIT grbitESEDUMPMode)
{
    ERR err =           JET_errSuccess;
    FUCB *pfucbTable =  pfucbNil;
    
    CallR( ErrFILEOpenTable(
                pfucbCatalog->ppib,
                pfucbCatalog->ifmp,
                &pfucbTable,
                szTableName,
                JET_bitTableSequential ) );
    Assert( pfucbNil != pfucbTable );
    Assert( pfcbNil != pfucbTable->u.pfcb );
    Assert( pfucbTable->u.pfcb->FTypeTable() );


    addPBI("\n");
    addPBI("[%d , %d] %s (Table)\n",
                pfucbCatalog->ifmp,
                pfucbTable->u.pfcb->PgnoFDP(),
                szTableName);
    flushPB();

    Call (ErrESEDUMPCheckAndDumpSpaceInfo(pfucbTable, grbitESEDUMPMode));


    Call (ErrESEDUMPNodesForOneTree(pfucbTable, grbitESEDUMPMode));
    
    Call (ErrESEDUMPIndexForOneTable(pfucbTable, grbitESEDUMPMode));
    
    Call (ErrESEDUMPLVForOneTable(pfucbTable, grbitESEDUMPMode));
    
HandleError:
    Assert( pfucbNil != pfucbTable );
    CallS( ErrFILECloseTable( pfucbCatalog->ppib, pfucbTable ) );
    return err;
}

LOCAL ERR ErrESEDUMPLVForOneTable(FUCB *pfucbTable, JET_GRBIT grbitESEDUMPMode)
{
    ERR err =       JET_errSuccess;
    FUCB *pfucbLV = pfucbNil;

    Assert(pfucbTable);
    Assert( Pcsr(pfucbTable ));
    Assert( !Pcsr(pfucbTable )->FLatched() );
    
    addIndent();
    
    err = ErrFILEOpenLVRoot( pfucbTable, &pfucbLV, fFalse );
    if ( JET_errSuccess == err )
    {
        Assert(pfucbLV);
        Assert( !Pcsr(pfucbLV )->FLatched() );

        addPBI("\n");
        addPBI("[%d , %d] LONG VALUES:\n",
                    pfucbLV->ifmp,
                    pfucbLV->u.pfcb->PgnoFDP());
        
        flushPB();
        FUCBSetIndex(pfucbLV);

        Call (ErrESEDUMPCheckAndDumpSpaceInfo(pfucbLV, grbitESEDUMPMode));

        Call (ErrESEDUMPNodesForOneTree(pfucbLV, grbitESEDUMPMode));
        
        DIRClose(pfucbLV);
        pfucbLV = pfucbNil;
    }
    else
        if ( wrnLVNoLongValues != err)
        {
            Call (err);
        }
        else
            err = JET_errSuccess;
    
HandleError:
    delIndent();
    
    if( pfucbNil != pfucbLV )
        DIRClose( pfucbLV );

    Assert(pfucbTable);
    Assert( Pcsr(pfucbTable ));
    Assert( !Pcsr(pfucbTable )->FLatched() );

    return err;
}


LOCAL ERR ErrESEDUMPIndexForOneTable(FUCB *pfucbTable, JET_GRBIT grbitESEDUMPMode)
{
    
    ERR err =           JET_errSuccess;
    FUCB *pfucbIndex =  pfucbNil;
    FCB * pfcb =        pfcbNil;
    
    
    Assert(pfucbTable);
    Assert( Pcsr(pfucbTable ));
    Assert( !Pcsr(pfucbTable )->FLatched() );

    pfcb = pfucbTable->u.pfcb->PfcbNextIndex();
    
    addIndent();
    
    while (pfcb)
    {
        Call (ErrDIROpen(
                    pfucbTable->ppib,
                    pfcb->PgnoFDP(),
                    pfucbTable->ifmp,
                    &pfucbIndex ));
                    
        Assert(pfucbIndex != pfucbNil );
        
        FUCBSetIndex( pfucbIndex );

        USHORT indexNameTag = 0;
        char * szIndexName = (char *)0;
        FCB *pfcbT = pfucbTable->u.pfcb;
        
        Assert(pfcbT);
        Assert(pfucbIndex->u.pfcb->Pidb());
        indexNameTag = pfucbIndex->u.pfcb->Pidb()->ItagIndexName();

        Assert( pfcbT->Ptdb() );
        szIndexName = pfcbT->Ptdb()->SzIndexName(indexNameTag,
                                                    pfucbIndex->u.pfcb->FDerivedIndex());
        Assert(szIndexName);
        
        
        addPBI("\n");
        addPBI("[%d , %d] INDEX %s:\n",
                    pfucbIndex->ifmp,
                    pfucbIndex->u.pfcb->PgnoFDP(),
                    szIndexName);
        flushPB();
        Assert( !Pcsr( pfucbIndex )->FLatched() );

        Call (ErrESEDUMPCheckAndDumpSpaceInfo(pfucbIndex, grbitESEDUMPMode));
        
        Call (ErrESEDUMPNodesForOneTree(pfucbIndex, grbitESEDUMPMode));

        DIRClose(pfucbIndex);
        pfucbIndex = pfucbNil;

        pfcb = pfcb->PfcbNextIndex();
    }
    
HandleError:
    delIndent();
    if ( pfucbNil != pfucbIndex)
    {
        DIRClose(pfucbIndex);
    }

    Assert(pfucbTable);
    Assert( Pcsr(pfucbTable ));
    Assert( !Pcsr(pfucbTable )->FLatched() );
    return err;
}

LOCAL ERR ErrESEDUMPNodesForOneTree(FUCB *pfucbTable, JET_GRBIT grbitESEDUMPMode, PfnFormatNodeInfo pfnFormat)
{
    ERR             err         = JET_errSuccess;
    DIB             dib;
    const DIRFLAG   dirflag     = ( grbitESEDUMPMode & JET_bitDBUtilOptionAllNodes ) ? fDIRAllNode : fDIRNull;

    Assert( pfucbTable );
    Assert( Pcsr( pfucbTable ) );
    Assert( !Pcsr( pfucbTable )->FLatched() );

    
    const BOOL      fSpaceTree  = FFUCBSpace( pfucbTable );
    
    dib.pos     = posFirst;
    dib.dirflag = (grbitESEDUMPMode & JET_bitDBUtilOptionAllNodes)?
                        fDIRAllNode:fDIRNull;

    if ( fSpaceTree )
    {
        err = ErrBTDown( pfucbTable, &dib, latchReadTouch );
    }
    else
    {
        err = ErrDIRDown( pfucbTable, &dib );
    }

    if ( JET_errRecordNotFound == err )
    {
        addIndent();
        addPBI("NODES: none \n");
        delIndent();
        flushPB();
        
        Assert(pfucbTable);
        Assert( Pcsr(pfucbTable ));
        Assert( !Pcsr(pfucbTable )->FLatched() );
        return JET_errSuccess;
    }

    if ( JET_errSuccess != err )
    {
        Assert(pfucbTable);
        Assert( Pcsr(pfucbTable ));
        Assert( !Pcsr(pfucbTable )->FLatched() );
        CallR (err);
    }

    Assert (JET_errSuccess == err );
    Assert( Pcsr(pfucbTable )->FLatched() );

    addIndent();
    addPBI("NODES:\n");
    
    addIndent();
    addPBI("********************************"
                "**********************************\n");

    if (pfnFormat)
    {
        addPBI("Page    | Line | Flags | KSize | DSize | Node info \n");
    }
    else if (fSpaceTree)
    {
        addPBI("Page    | Line | Flags | KSize | DSize | Pages\n");
    }
    else
    {
        addPBI("Page    | Line | Flags | KSize | DSize | Key value\n");
    }
    
    addPBI("********************************"
                "**********************************\n");

    flushPB();

    while ( JET_errSuccess == err )
    {

        char *pCustomFormat = NULL;
        char keyDataPrintBuffer[ max ( MAX_KEY_SIZE_DISPLAY * 3  + 1,
                                    3 * 12 + 6  + 1 ) ];
        
        keyDataPrintBuffer[0] = 0;

        Assert( Pcsr( pfucbTable )->FLatched() );
        
        if (pfnFormat)
        {
            pCustomFormat = pfnFormat( &pfucbTable->kdfCurr );
        }
        else if (fSpaceTree)
        {
            PGNO pgnoLast;
            CPG cpg;
            SpacePool sppPool;

            Call( ErrSPIGetExtentInfo( pfucbTable, &pgnoLast, &cpg, &sppPool ) );

            Assert(pgnoLast <= pgnoSysMax);
            Assert(pgnoLast >= (ULONG)cpg);
            OSStrCbFormatA(keyDataPrintBuffer, sizeof(keyDataPrintBuffer), "%lu - %lu (%lu)", pgnoLast - cpg + 1, pgnoLast, (ULONG)sppPool);
        }
        else
        {
            char keyBuffer[MAX_KEY_SIZE_DISPLAY];

            const INT cbToRetrieve = min( MAX_KEY_SIZE_DISPLAY, pfucbTable->kdfCurr.key.Cb() );
            pfucbTable->kdfCurr.key.CopyIntoBuffer( keyBuffer, cbToRetrieve );

            BYTE * pbT  = (BYTE *)keyDataPrintBuffer;

            for( INT i = 0 ;
                i < min(MAX_KEY_SIZE_DISPLAY, pfucbTable->kdfCurr.key.Cb());
                i++)
            {
                AssumePREFAST( i < _countof( keyBuffer ) );
                const BYTE  b   = keyBuffer[i];

                AssumePREFAST( i < _countof( keyDataPrintBuffer ) );
                *pbT++ = mpbb[b >> 4];
                *pbT++ = mpbb[b & 0x0f];
                *pbT++ = ' ';
            }
            *pbT = 0;
        }

        addPBI( "%7d | %4d |  %c%c%c  | %5ld | %5ld | %s\n",
                    pfucbTable->csr.Pgno(),
                    pfucbTable->csr.ILine(),
                    pfucbTable->kdfCurr.fFlags & fNDCompressed ? 'c':' ',
                    pfucbTable->kdfCurr.fFlags & fNDDeleted ? 'd':' ',
                    pfucbTable->kdfCurr.fFlags & fNDVersion ? 'v':' ',
                    pfucbTable->kdfCurr.key.Cb(),
                    pfucbTable->kdfCurr.data.Cb(),
                    pfnFormat ? pCustomFormat : keyDataPrintBuffer );

        Assert( Pcsr( pfucbTable )->FLatched() );
        if ( fSpaceTree )
        {
            err = ErrBTNext( pfucbTable, dirflag );
        }
        else
        {
            Call (ErrDIRRelease( pfucbTable ));
            Assert( !Pcsr( pfucbTable )->FLatched() );

            err = ErrDIRNext(pfucbTable, dirflag);
        }
    }

    if (JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }

HandleError:
    flushPB();
    delIndent();
    delIndent();

    if ( Pcsr( pfucbTable )->FLatched() )
    {
        if ( fSpaceTree )
        {
            BTUp( pfucbTable );
        }
        else
        {
            (VOID)ErrDIRRelease( pfucbTable );
        }
    }
    
    Assert(pfucbTable);
    Assert( Pcsr(pfucbTable ));
    Assert( !Pcsr(pfucbTable )->FLatched() );
    return err;
}


LOCAL ERR ErrESEDUMPCheckAndDumpSpaceInfo(FUCB *pfucb, JET_GRBIT grbitESEDUMPMode)
{
    ERR err =           JET_errSuccess;
    INT fIsSpaceTree =  fFalse;
    INT fIsRoot =       fFalse;
    LINE line;
    
    Assert(pfucb);
    Assert( Pcsr(pfucb ));
    Assert( !Pcsr(pfucb )->FLatched() );

    if ( !pfucb->u.pfcb->FSpaceInitialized() )
    {
        Call( ErrSPDeferredInitFCB( pfucb ) );
    }

    CallR (ErrBTIGotoRoot( pfucb, latchReadNoTouch ));
    Assert( Pcsr(pfucb )->FLatched() );

    fIsSpaceTree = Pcsr(pfucb )->Cpage().FSpaceTree();
    fIsRoot =  Pcsr(pfucb )->Cpage().FRootPage();

    if (!fIsSpaceTree && !fIsRoot)
    {
        goto HandleError;
    }
    
    if (fIsSpaceTree)
    {
        NDGetPtrExternalHeader( Pcsr( pfucb )->Cpage(), &line, noderfWhole );
        Assert ( sizeof( SPLIT_BUFFER ) == line.cb );

        if (sizeof( SPLIT_BUFFER ) == line.cb)
        {
            SPLIT_BUFFER spb;
            UtilMemCpy( &spb, line.pv, sizeof( SPLIT_BUFFER ) );
            ESEDUMPSplitBufferDump(spb, pfucb, grbitESEDUMPMode);
        }
        else
        {
            addIndent();
            addPBI("SPLIT BUFFER not found in tree.\n");
            delIndent();
        }
    }
    else
    {
        Assert( fIsRoot );
        NDGetPtrExternalHeader( Pcsr( pfucb )->Cpage(), &line, noderfSpaceHeader );
        Assert( sizeof( SPACE_HEADER ) == line.cb );

        SPACE_HEADER sph;
        
        UtilMemCpy( &sph, line.pv, sizeof( SPACE_HEADER ) );

        addIndent();
        addPBI("\n");
        addPBI("SPACE INFO:\n");
        addIndent();

        if ( sph.Fv1() )
        {
            addPBI("Primary extent size: %ld page%s\n", sph.CpgPrimary(), sph.CpgPrimary() == 1 ? "" : "s" );
        }
        else
        {
            addPBI("Primary extent size: <unavailable>\n");
            addPBI("Last allocation size: %ld page%s\n", sph.CpgLastAlloc(), sph.CpgLastAlloc() == 1 ? "" : "s");
        }

        if ( sph.FSingleExtent())
        {
            ESEDUMPSingleSpaceHeaderDump(sph, pfucb, grbitESEDUMPMode);
        }
        else
        {
            err = ErrESEDUMPMultipleSpaceHeaderDump(sph, pfucb, grbitESEDUMPMode);
        }
        
        delIndent();
        delIndent();
    }

HandleError:
    flushPB();
    Assert(pfucb);
    Assert( Pcsr(pfucb ));
    Assert( Pcsr(pfucb )->FLatched() );
    BTUp(pfucb);
    Assert( !Pcsr(pfucb )->FLatched() );
    return err;
}


LOCAL ERR ErrESEDUMPDatabaseInfo(PIB *ppib, IFMP ifmp, JET_GRBIT grbitESEDUMPMode)
{
    FUCB *pfucbDb =     pfucbNil;
    ERR err =           JET_errSuccess;
    
    CallR (ErrDIROpen(
                ppib,
                pgnoSystemRoot,
                ifmp,
                &pfucbDb ));
    Assert( pfucbNil != pfucbDb );
    Assert( pfcbNil != pfucbDb->u.pfcb );
    Assert( pgnoSystemRoot == pfucbDb->u.pfcb->PgnoFDP());

    addPBI("\n");
    addPBI("[%d , %d] DATABASE ROOT\n", ifmp, pgnoSystemRoot);
    flushPB();

    Call (ErrESEDUMPCheckAndDumpSpaceInfo(pfucbDb, grbitESEDUMPMode));

HandleError:
    DIRClose(pfucbDb);
    return err;
}


LOCAL VOID ESEDUMPSplitBufferDump(SPLIT_BUFFER & spb, FUCB * pfucb, JET_GRBIT grbitESEDUMPMode)
{
    Assert(pfucb);
    Assert(Pcsr(pfucb));
    Assert(Pcsr(pfucb)->Cpage().FSpaceTree());
    
    addIndent();
    addPBI("SPLIT BUFFER:\n");
    addIndent();
    if (spb.CpgBuffer1() > 0)
    {
        addPBI("Buffer 1: %ld - %ld (%ld pages)\n",
                    spb.PgnoLastBuffer1() - spb.CpgBuffer1() + 1,
                    spb.PgnoLastBuffer1(),
                    spb.CpgBuffer1());
    }
    else
    {
        addPBI("Buffer 1: No pages\n");
    }
    if (spb.CpgBuffer1() > 0)
    {
        addPBI("Buffer 2: %ld - %ld (%ld pages)\n",
                    spb.PgnoLastBuffer2() - spb.CpgBuffer2() + 1,
                    spb.PgnoLastBuffer2(),
                    spb.CpgBuffer2());
    }
    else
    {
        addPBI("Buffer 2: No pages\n");
    }
    delIndent();
    delIndent();
    flushPB();
}

LOCAL VOID ESEDUMPSingleSpaceHeaderDump(SPACE_HEADER & sph, FUCB * pfucb, JET_GRBIT grbitESEDUMPMode)
{
    Assert( sph.FSingleExtent());
    Assert(pfucb);
    
    addPBI("Single extent covering pages %d to %d\n",
                PgnoFDP(pfucb),
                PgnoFDP(pfucb) + sph.CpgPrimary() - 1 );

    
    UINT rgbAvailBitMask = sph.RgbitAvail();
    UINT rgbCurrentPageMask = 0x1;
    
    PGNO pageNo = PgnoFDP(pfucb) + 1;

    char pagesPrintBuffer[ 8*sizeof(UINT) * 12  + 1];
    pagesPrintBuffer[0] = 0;
    
    CPG cpgAvail = 0;
    while(rgbCurrentPageMask)
    {
        if (rgbAvailBitMask & rgbCurrentPageMask)
        {
            OSStrCbFormatA(pagesPrintBuffer + strlen(pagesPrintBuffer), sizeof(pagesPrintBuffer)-strlen(pagesPrintBuffer), " %lu", pageNo);
            cpgAvail++;
        }

        rgbCurrentPageMask = rgbCurrentPageMask << 1;
        pageNo++;
    }
    addPBI("Avail pages (%ld): %s\n", cpgAvail, pagesPrintBuffer);
    flushPB();
}


LOCAL ERR ErrESEDUMPMultipleSpaceHeaderDump(SPACE_HEADER & sph, FUCB * pfucb, JET_GRBIT grbitESEDUMPMode)
{
    FUCB *pfucbExtent =     pfucbNil;
    ERR err =               JET_errSuccess;
    
    Assert(pfucb);
    Assert( Pcsr(pfucb ));
    Assert( Pcsr(pfucb )->FLatched() );
    
    
    Assert (sph.FMultipleExtent());
    addPBI("Multiple extents\n");
    addPBI("Own extent page number   : %d\n", sph.PgnoOE());
    addPBI("Avail extent page number : %d\n", sph.PgnoAE());
    flushPB();

    for(INT step = 0; step < 2; step++)
    {
        addPBI(step?"[%d , %d] AVAIL EXTENT:\n":"[%d , %d] OWN EXTENT:\n",
                    pfucb->ifmp,
                    step?sph.PgnoAE():sph.PgnoOE());
        flushPB();

        if ( step )
        {
            Call( ErrSPIOpenAvailExt( pfucb->ppib, pfucb->u.pfcb, &pfucbExtent ) );
        }
        else
        {
            Call( ErrSPIOpenOwnExt( pfucb->ppib, pfucb->u.pfcb, &pfucbExtent ) );
        }
        Assert( pfucbExtent != pfucbNil );

        
        FUCBSetSequential( pfucbExtent );
        FUCBSetPrereadForward( pfucbExtent, cpgPrereadSequential );
        
        Call (ErrESEDUMPCheckAndDumpSpaceInfo(pfucbExtent, grbitESEDUMPMode));

        Call (ErrESEDUMPNodesForOneTree(pfucbExtent, grbitESEDUMPMode));

        BTClose(pfucbExtent);
        pfucbExtent = pfucbNil;
    }

HandleError:
    if ( pfucbNil != pfucbExtent)
    {
        BTClose(pfucbExtent);
    }
    Assert( pfucb );
    Assert( Pcsr(pfucb ) );
    Assert( Pcsr(pfucb )->FLatched() );
    flushPB();
    return err;
}

#endif

