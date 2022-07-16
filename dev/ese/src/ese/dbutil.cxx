// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_bt.hxx"
#include "_dump.hxx"
#include "_space.hxx"

#include "stat.hxx"

LOCAL ERR ErrDBUTLDumpTables( DBCCINFO *pdbccinfo, PFNTABLE pfntable, VOID* pvCtx = NULL );


#if !defined( MINIMAL_FUNCTIONALITY ) || defined( DEBUGGER_EXTENSION )

/*
The output will be something like the one below:
000001a0    00326d32 6e383059 04000900 1d7f8002 7f800000 01098023 001d0000 00020001 \ .2m2n80Y...............#........
000001c0    00000009 00000004 00000000 00000000 0000002a 040080fe 1b005479 70686f6f \ ...................*......Typhoo
000001e0    6e205665 72696669 63617469 6f6e2043 6f6c756d 6e040009 001e7f80 017f8000 \ n Verification Column...........
00000200    001e0880 20001e00 00000100 1e000000 8e000000 38000000 00000000 03000000 \ .... ...............8...........
00000220    ff001900 35315545 486a6c36 44797051 6a544255 6b76557a 454d6a66 42040009 \ ....51UEHjl6DypQjTBUkvUzEMjfB...
00000240    001e7f80 027f8000 00010980 23001e00 00000200 01000000 09000000 04000000 \ ............#...................
00000260    00000000 00000000 2a040080 fe1b0054 7970686f 6f6e2056 65726966 69636174 \ ........*......Typhoon Verificat
00000280    696f6e20 436f6c75 6d6e0400 09001f7f 80017f80 00001f08 8020001f 00000001 \ ion Column............... ......
000002a0    001f0000 00910000 00350000 00000000 00090000 00ff003c 0064414e 67664135 \ .........5.............<.dANgfA5
000002c0    62705866 6e717479 426f756e 53663365 72484732 7945744b 386f4d4c 6e315a30 \ bpXfnqtyBounSf3erHG2yEtK8oMLn1Z0
000002e0    35567361 316f7a75 77645550 7557514b 384c744d 67040009 001f7f80 027f8000 \ 5Vsa1ozuwdUPuWQK8LtMg...........
00000300    00010980 23001f00 00000200 01000000 09000000 04000000 00000000 00000000 \ ....#...........................
00000320    2a040080 fe1b0054 7970686f 6f6e2056 65726966 69636174 696f6e20 436f6c75 \ *......Typhoon Verification Colu
00000340    6d6e0400 0900207f 80017f80 00002008 80200020 00000001 00200000 009a0000 \ mn.... ....... .. . ..... ......

*/
//  ================================================================
VOID DBUTLSprintHex(
    __out_bcount(cbDest) CHAR * const       szDest,
    const INT           cbDest,
    const BYTE * const  rgbSrc,
    const INT           cbSrc,
    const INT           cbWidth,
    const INT           cbChunk,
    const INT           cbAddress,
    const INT           cbStart)
//  ================================================================
{
    static const CHAR rgchConvert[] =   { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
    
    const BYTE * const pbMax = rgbSrc + cbSrc;
    const INT cchHexWidth = ( cbWidth * 2 ) + (  cbWidth / cbChunk );

    const BYTE * pb = rgbSrc;
    CHAR * szDestCurrent = szDest;

    Assert( cbDest >= sizeof(CHAR) );
    if ( cbDest < sizeof(CHAR) )
    {
        return;
    }

    Assert( cchHexWidth );
    if ( 0 == cchHexWidth )
    {
        return;
    }
    
    // this is the max position we can write to and
    // we will use it as a marker to stop
    //
    CHAR * const szDestMax = szDestCurrent + cbDest - 1;
    
    while( pbMax != pb && szDestCurrent < szDestMax )
    {
        
        if ( cbAddress )
        {
            StringCbPrintfA( szDestCurrent, szDestMax - szDestCurrent + 1, "%*.*lx    ", cbAddress, cbAddress, (INT)(pb - rgbSrc + cbStart) );
            (*szDestMax) = 0;
            szDestCurrent += strlen(szDestCurrent);

            if ( szDestMax <= szDestCurrent )
                break;
        }

        
        CHAR * szCurrentRightSide   = szDestCurrent + cchHexWidth;
        
        if ( szDestMax <= ( szCurrentRightSide + 2 /* for left/right separator */ ) )
            break;
            
        *szCurrentRightSide++ = '\\';
        *szCurrentRightSide++ = ' ';

        memset( szDestCurrent, ' ', cchHexWidth );
        do
        {
            Assert( szDestCurrent < szCurrentRightSide );
            
            for( INT cb = 0; cbChunk > cb && pbMax != pb && szCurrentRightSide <= szDestMax && szDestCurrent < szDestMax; ++cb, ++pb )
            {
                *szDestCurrent++    = rgchConvert[ *pb >> 4 ];
                *szDestCurrent++    = rgchConvert[ *pb & 0x0F ];
                *szCurrentRightSide++   = isprint( *pb ) ? *pb : '.';
            }
            
            szDestCurrent++;
            
        } while( ( ( pb - rgbSrc ) % cbWidth ) && pbMax > pb && szCurrentRightSide < szDestMax);
        
        if ( szCurrentRightSide < szDestMax )
        {
            *szCurrentRightSide++ = '\n';
            *szCurrentRightSide = '\0';
        }
        
        szDestCurrent = szCurrentRightSide;
    }

    Assert( szDestCurrent<= szDestMax ) ;
    (*szDestMax) = 0;
}

#pragma prefast(push)
#pragma prefast(disable:6262, "This function uses a lot of stack (33k) because of szBuff[g_cbPageMax].")
//  ================================================================
VOID DBUTLDumpRec( const LONG cbPage, const FUCB * const pfucbTable, const VOID * const pv, const INT cb, CPRINTF * pcprintf, const INT cbWidth )
//  ================================================================
{
    CHAR szBuf[g_cbPageMax];
    
    TDB* const ptdb = ( pfucbTable != pfucbNil ? pfucbTable->u.pfcb->Ptdb() : ptdbNil );
    TDB* const ptdbTemplate = ( ptdb != ptdbNil && ptdb->PfcbTemplateTable() != pfcbNil ?
                                    ptdb->PfcbTemplateTable()->Ptdb() :
                                    ptdbNil );

    //  dump the columns of the record
    const REC * const prec = reinterpret_cast<const REC *>( pv );

    const CHAR * szColNameSeparator = "  -  ";
    const CHAR * szNullValue = "<NULL>";
    const CHAR * szTemplate  = " (template)";
    const CHAR * szDerived   = " (derived)";
    #define SzTableColType( ptdbTemplateIn, fTemplateColumnIn )    ( ( ptdbTemplateIn == NULL ) ? "" : ( fTemplateColumnIn ? szTemplate : szDerived ) ) 

    FID fid;
    
    const FID fidFixedFirst = FID( fidtypFixed, fidlimLeast );
    const FID fidFixedLast  = prec->FidFixedLastInRec();
    const INT cColumnsFixed = max( 0, fidFixedLast - fidFixedFirst + 1 );
    
    (*pcprintf)( "   Fixed Columns:  %d\n", cColumnsFixed );
    (*pcprintf)( "=================\n" );
    for( fid = fidFixedFirst; fid <= fidFixedLast; ++fid )
    {
        const UINT  ifid                    = fid.IndexOf( fidtypFixed );
        const BYTE  * const prgbitNullity   = prec->PbFixedNullBitMap() + ifid/8;

        // Get the COLUMNID from FID.
        const BOOL fTemplateColumn = ptdbTemplate == ptdbNil ? fFalse : fid <= ptdbTemplate->FidFixedLast() ? fTrue : fFalse;
        const COLUMNID columnid = ColumnidOfFid( fid, fTemplateColumn );

        const FIELD * const     pfield      = ( ptdb != ptdbNil ) ? ptdb->Pfield( columnid ) : NULL;
        BOOL                    fDeleted    = fFalse;
        const CHAR *            szType      = "UnknownColType";
        const CHAR *            szColumn    = "UnknownColName ";
        if ( pfield )
        {
            fDeleted = ( 0 == pfield->itagFieldName );
            szType   = ( fDeleted ? "<deleted>" : SzColumnType( pfield->coltyp ) );
            szColumn = ( fDeleted ? "<deleted>" : ( fTemplateColumn ? ptdbTemplate : ptdb )->SzFieldName( pfield->itagFieldName, ptdbTemplate != NULL && !fTemplateColumn /* was: fFalse */ ) );
        }
        const CHAR * const      szHddlSrc   = SzTableColType( ptdbTemplate, fTemplateColumn );

        (*pcprintf)( "%u (0x%x):  [%-16hs] %hs%hs", columnid, columnid, szType, szColumn, szHddlSrc );

        if ( FFixedNullBit( prgbitNullity, ifid ) )
        {
            (*pcprintf)( "%hs%hs\n", szColNameSeparator, szNullValue );
        }
        else
        {
            (*pcprintf)( "\n" );
            if ( pfucbTable )
            {
                DATA dataRec;
                dataRec.SetPv( (void*)prec );
                dataRec.SetCb( cb );
                DATA dataCol;
            
                const ERR errRet = ErrRECRetrieveNonTaggedColumn(
                        pfucbTable->u.pfcb,
                        columnid,
                        dataRec,
                        &dataCol,
                        pfieldNil );
                if ( errRet >= JET_errSuccess )
                {
                    Expected( dataCol.Cb() < 257 ); // think it's even < 128 actually
                    szBuf[0] = 0;
                    DBUTLSprintHex( szBuf, sizeof(szBuf), (BYTE*)dataCol.Pv(), dataCol.Cb(), cbWidth );
                    (*pcprintf)( "%s", szBuf );
                }
                else
                {
                    (*pcprintf)( "<ERR: got %d trying retrieve column value>\n", errRet );
                }
            }
            else
            {
                (*pcprintf)( "<ERR: need at least copy of FCB to dump fixed cols>\n" );
            }
        }
    }

    (*pcprintf)( "\n" );
    
    const FID fidVariableFirst = FID( fidtypVar, fidlimLeast );
    const FID fidVariableLast  = prec->FidVarLastInRec();
    const INT cColumnsVariable = max( 0, fidVariableLast - fidVariableFirst + 1 );
    (*pcprintf)( "Variable Columns:  %d\n", cColumnsVariable );
    (*pcprintf)( "=================\n" );

    const UnalignedLittleEndian<REC::VAROFFSET> * const pibVarOffs      = ( const UnalignedLittleEndian<REC::VAROFFSET> * const )prec->PibVarOffsets();
    for( fid = fidVariableFirst; fid <= fidVariableLast; ++fid )
    {
        const UINT              ifid            = fid.IndexOf( fidtypVar );
        const REC::VAROFFSET    ibStartOfColumn = prec->IbVarOffsetStart( fid );
        const REC::VAROFFSET    ibEndOfColumn   = IbVarOffset( pibVarOffs[ifid] );

        // Get the COLUMNID from FID.
        const BOOL fTemplateColumn = ptdbTemplate == ptdbNil ? fFalse : fid <= ptdbTemplate->FidVarLast() ? fTrue : fFalse;
        const COLUMNID columnid = ColumnidOfFid( fid, fTemplateColumn );

        const FIELD * const     pfield      = ( ptdb != ptdbNil ) ? ptdb->Pfield( columnid ) : NULL;
        BOOL                    fDeleted    = fFalse;
        const CHAR *            szType      = "UnknownColType";
        const CHAR *            szColumn    = "UnknownColName ";
        if ( pfield )
        {
            fDeleted    = ( 0 == pfield->itagFieldName );
            szType      = ( fDeleted ? "<deleted>" : SzColumnType( pfield->coltyp ) );
            szColumn    = ( fDeleted ? "<deleted>" : ( fTemplateColumn ? ptdbTemplate : ptdb )->SzFieldName( pfield->itagFieldName, ptdbTemplate != NULL && !fTemplateColumn /* was: fFalse */ ) );
        }
        const CHAR * const      szHddlSrc   = SzTableColType( ptdbTemplate, fTemplateColumn );

        (*pcprintf)( "%u (0x%x):  [%-16hs] %hs%hs", columnid, columnid, szType, szColumn, szHddlSrc );
        if ( FVarNullBit( pibVarOffs[ifid] ) )
        {
            (*pcprintf)( "%hs%hs\n", szColNameSeparator, szNullValue );
        }
        else
        {
            const VOID * const pvColumn = prec->PbVarData() + ibStartOfColumn;
            const INT cbColumn          = ibEndOfColumn - ibStartOfColumn;
            (*pcprintf)( "%hs%d bytes\n", szColNameSeparator, cbColumn );
            DBUTLSprintHex( szBuf, sizeof(szBuf), (BYTE *)pvColumn, cbColumn, cbWidth );
            (*pcprintf)( "%s\n", szBuf );
        }
    }

    (*pcprintf)( "\n" );

    (*pcprintf)( "  Tagged Columns:\n" );
    (*pcprintf)( "=================\n" );

    DATA    dataRec;
    dataRec.SetPv( (VOID *)pv );
    dataRec.SetCb( cb );

    if ( !TAGFIELDS::FIsValidTagfields( cbPage, dataRec, pcprintf ) )
    {
        (*pcprintf)( "Tagged column corruption detected.\n" );
    }

    (*pcprintf)( "TAGFIELDS array begins at offset 0x%x from start of record.\n\n", prec->PbTaggedData() - (BYTE *)prec );

    TAGFIELDS_ITERATOR ti( dataRec );

    ti.MoveBeforeFirst();

    while( JET_errSuccess == ti.ErrMoveNext() )
    {
        //  we are now on an individual column

        const CHAR * szComma = " ";

        const BOOL fTemplateColumn = ptdbTemplate == ptdbNil ? fFalse : ti.Fid() <= ptdbTemplate->FidTaggedLast() ? fTrue : fFalse;
        const COLUMNID columnid = ColumnidOfFid( ti.Fid(), fTemplateColumn );

        const FIELD * const     pfield      = ( ptdb != ptdbNil ) ? ptdb->Pfield( columnid ) : NULL;
        BOOL                    fDeleted    = fFalse;
        const CHAR *            szType      = "UnknownColType";
        const CHAR *            szColumn    = "UnknownColName ";
        if ( pfield )
        {
            fDeleted    = ( 0 == pfield->itagFieldName );
            szType      = ( fDeleted ? "<deleted>" : SzColumnType( pfield->coltyp ) );
            szColumn    = ( fDeleted ? "<deleted>" : ( fTemplateColumn ? ptdbTemplate : ptdb )->SzFieldName( pfield->itagFieldName, ptdbTemplate != NULL && !fTemplateColumn /* was: fFalse */ ) );

            Assert( !!ti.FLV() == ( ( pfield->coltyp == JET_coltypLongText ) || ( pfield->coltyp == JET_coltypLongBinary ) ) );
        }
        //  So ti.FDerived() actually means the column "was derived" from template, so in this context
        //  it means that it is from the template schema.  More from SOMEONE on the subject:
        //      TAGFLD doesn't have an FTemplate bit.  It only has an FDerived bit.  So for a 
        //      TAGFLD, if FDerived==TRUE, it means the column is from the template table (in 
        //      other words, the column was derived from the template).  If FDerived==FALSE, 
        //      it means the column was defined just in the derived table (in other words the 
        //      column was not derived from the template).
        //  At any rate this: Assert( fTemplateColumn == !ti.FDerived() ); does not hold, so
        //  I am not sure which is conceptually right.  And/or if the compute of fTemplateColumn
        //  above for fixed and variable columns is right.
        const CHAR * const      szHddlSrc   = SzTableColType( ptdbTemplate, !ti.FDerived() );

        (*pcprintf)( "%u (0x%x):  [%-16hs] %hs%hs", ti.Fid(), ti.Fid(), szType, szColumn, szHddlSrc );
        if( ti.FNull() )
        {
            (*pcprintf)( "%hs%hs", szColNameSeparator, szNullValue );
            szComma = ", ";
        }
        (*pcprintf)( "\r\n" );

        ti.TagfldIterator().MoveBeforeFirst();

        INT itag = 1;
        
        while( JET_errSuccess == ti.TagfldIterator().ErrMoveNext() )
        {
            const BOOL  fSeparated  = ti.TagfldIterator().FSeparated();
            const BOOL  fCompressed = ti.TagfldIterator().FCompressed();
            const BOOL  fEncrypted  = ti.TagfldIterator().FEncrypted();

            (*pcprintf)( ">> itag %d: %d bytes (offset 0x%x): ", itag, ti.TagfldIterator().CbData(), ti.TagfldIterator().PbData() - (BYTE *)prec );
            if ( fSeparated )
            {
                (*pcprintf)( "separated" );
            }
            if ( fCompressed )
            {
                (*pcprintf)( " compressed" );
            }
            if ( fEncrypted )
            {
                (*pcprintf)( " encrypted" );
            }
            (*pcprintf)( "\r\n" );

            const INT cbPrintMax = 512;
            if( fCompressed && !fEncrypted && !fSeparated )
            {
                szBuf[0] = 0;
                DBUTLSprintHex(
                    szBuf,
                    sizeof(szBuf),
                    ti.TagfldIterator().PbData(),
                    min( ti.TagfldIterator().CbData(), 64 ),    //  only print 64b of compressed/encrypted data
                    cbWidth );
                (*pcprintf)( "%s%s\r\n", szBuf, ( ti.TagfldIterator().CbData() > 64 ? "...\r\n" : "" ) );

                BYTE rgbDecompressed[cbPrintMax];
                DATA dataCompressed;
                dataCompressed.SetPv( (void *)ti.TagfldIterator().PbData() );
                dataCompressed.SetCb( ti.TagfldIterator().CbData() );
                INT cbDecompressed;

                CallSx( ErrPKDecompressData(
                    dataCompressed,
                    NULL,  // The pfucb from debugger is half filled, passing NULL here only makes PKDecompress not update perf counters and report events (by deref PinstFromIfmp( pfucb-ifmp )).
                    rgbDecompressed,
                    sizeof(rgbDecompressed),
                    &cbDecompressed ),
                    JET_wrnBufferTruncated );
                size_t cbToPrint = min( sizeof(rgbDecompressed ), cbDecompressed );

                (*pcprintf)( ">> %d bytes uncompressed:\r\n", cbDecompressed );

                szBuf[0] = 0;
                DBUTLSprintHex(
                    szBuf,
                    sizeof(szBuf),
                    rgbDecompressed,
                    min( cbToPrint, cbPrintMax ),
                    cbWidth );
                (*pcprintf)( "%s%s\r\n", szBuf, ( cbDecompressed > cbPrintMax ? "...\r\n" : "" ) );
            }
            else
            {
                szBuf[0] = 0;
                DBUTLSprintHex(
                    szBuf,
                    sizeof(szBuf),
                    ti.TagfldIterator().PbData(),
                    min( ti.TagfldIterator().CbData(), cbPrintMax ),   //  only print cbPrintMax of data to ensure we don't overrun printf buffer
                    cbWidth );
                (*pcprintf)( "%s%s\r\n", szBuf, ( ti.TagfldIterator().CbData() > cbPrintMax ? "...\r\n" : "" ) );
            }

            ++itag;
        }
    }

/*
    if ( TAGFIELDS::FIsValidTagfields( cbPage, dataRec, pcprintf ) )
    {
        TAGFIELDS   tagfields( dataRec );
        tagfields.Dump( pcprintf, szBuf, cbWidth );
    }
    else
    {
        (*pcprintf)( "Tagged column corruption detected.\n" );
    }
*/

    (*pcprintf)( "\n" );
}
#pragma prefast(push)


#endif  //  !defined( MINIMAL_FUNCTIONALITY ) || defined( DEBUGGER_EXTENSION )


#ifdef MINIMAL_FUNCTIONALITY
#else

//  description of page_info table
const JET_COLUMNDEF rgcolumndefPageInfoTable[] =
{
    //  Pgno
    {sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed | JET_bitColumnTTKey},

    //  consistency checked
    {sizeof(JET_COLUMNDEF), 0, JET_coltypBit, 0, 0, 0, 0, 0, JET_bitColumnFixed | JET_bitColumnNotNULL},
    
    //  Avail
    {sizeof(JET_COLUMNDEF), 0, JET_coltypBit, 0, 0, 0, 0, 0, JET_bitColumnFixed },

    //  Space free
    {sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed},

    //  Pgno left
    {sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },

    //  Pgno right
    {sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed }
};

const INT icolumnidPageInfoPgno         = 0;
const INT icolumnidPageInfoFChecked     = 1;
const INT icolumnidPageInfoFAvail       = 2;
const INT icolumnidPageInfoFreeSpace    = 3;
const INT icolumnidPageInfoPgnoLeft     = 4;
const INT icolumnidPageInfoPgnoRight    = 5;

const INT ccolumndefPageInfoTable       = ( sizeof ( rgcolumndefPageInfoTable ) / sizeof(JET_COLUMNDEF) );
LOCAL JET_COLUMNID g_rgcolumnidPageInfoTable[ccolumndefPageInfoTable];


typedef ERR(*PFNDUMP)( PIB *ppib, FUCB *pfucbCatalog, VOID *pfnCallback, VOID *pvCallback );

#endif  // !MINIMAL_FUNCTIONALITY

LOCAL ERR ErrDBUTLDump( JET_SESID sesid, const JET_DBUTIL_W *pdbutil );


//  ================================================================
LOCAL VOID DBUTLPrintfIntN( INT iValue, INT ichMax )
//  ================================================================
{
    CHAR    rgchT[17]; /* C-runtime max bytes == 17 */
    INT     ichT;

    _itoa_s( iValue, rgchT, _countof(rgchT), 10 );
    for ( ichT = 0; ichT < sizeof(rgchT) && rgchT[ichT] != '\0' ; ichT++ )
        ;
    if ( ichT > ichMax ) //lint !e661
    {
        for ( ichT = 0; ichT < ichMax; ichT++ )
            printf( "#" );
    }
    else
    {
        for ( ichT = ichMax - ichT; ichT > 0; ichT-- )
            printf( " " );
        for ( ichT = 0; rgchT[ichT] != '\0'; ichT++ )
            printf( "%c", rgchT[ichT] );
    }
    return;
}
#ifdef MINIMAL_FUNCTIONALITY
#else

LOCAL_BROKEN VOID DBUTLPrintfStringN(
    __in_bcount(ichMax) const CHAR *sz, // not a PCSTR, b/c not nesc. NUL terminated
    INT ichMax )
{
    INT     ich;

    for ( ich = 0; ich < ichMax && sz[ich] != '\0' ; ich++ )
        printf( "%c", sz[ich] );
    for ( ; ich < ichMax; ich++ )
        printf( " " );
    printf( " " );
    return;
}


//  ================================================================
LOCAL_BROKEN ERR ErrDBUTLRegExt( DBCCINFO *pdbccinfo, PGNO pgnoFirst, CPG cpg, BOOL fAvailT )
//  ================================================================
{
    ERR             err = JET_errSuccess;
    PGNO            pgnoLast = (PGNO)( pgnoFirst + cpg - 1 );
    PGNO            pgno;
    PIB             *ppib = pdbccinfo->ppib;
    JET_SESID       sesid = (JET_SESID) pdbccinfo->ppib;
    JET_TABLEID     tableid = pdbccinfo->tableidPageInfo;
    BYTE            fAvail = (BYTE) fAvailT;
    
    Assert( tableid != JET_tableidNil );

    if ( pgnoFirst > pgnoLast )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    for ( pgno = pgnoFirst; pgno <= pgnoLast; pgno++ )
    {
        BOOL        fFound;
        BYTE        fChecked = fFalse;

        CallR( ErrIsamBeginTransaction( (JET_SESID) ppib, 41189, NO_GRBIT ) );

        /*  search for page in the table
        /**/
        CallS( ErrDispMakeKey( sesid, tableid, (BYTE *)&pgno, sizeof(pgno), JET_bitNewKey ) );
        err = ErrDispSeek( sesid, tableid, JET_bitSeekEQ );
        if ( err < 0 && err != JET_errRecordNotFound )
        {
            Assert( fFalse );
            Call( err );
        }
        
        fFound = ( err == JET_errRecordNotFound ) ? fFalse : fTrue;
        if ( fFound )
        {
            ULONG   cbActual;
            BYTE    fAvailT2;
            
            /*  is this in availext
            /**/
            Call( ErrDispRetrieveColumn( sesid,
                tableid,
                g_rgcolumnidPageInfoTable[icolumnidPageInfoFAvail],
                (BYTE *)&fAvailT2,
                sizeof(fAvailT2),
                &cbActual,
                0,
                NULL ) );

            Assert( err == JET_wrnColumnNull || cbActual == sizeof(fAvailT2) );
            if ( err != JET_wrnColumnNull )
            {
                Assert( !fAvail || fAvailT2 );
            }

            /*  if fAvail is false, no changes to record
            /**/
            if ( !fAvail )
                goto Commit;

            /*  get fChecked [for setting it later]
            /**/
            Call( ErrDispRetrieveColumn( sesid,
                tableid,
                g_rgcolumnidPageInfoTable[icolumnidPageInfoFChecked],
                (BYTE *)&fChecked,
                sizeof(fChecked),
                &cbActual,
                0,
                NULL ) );

            Assert( cbActual == sizeof(fChecked) );

            Call( ErrDispPrepareUpdate( sesid, tableid, JET_prepReplaceNoLock ) );
        }
        else
        {
            Call( ErrDispPrepareUpdate( sesid, tableid, JET_prepInsert ) );

            /*  pgno
            /**/
            Call( ErrDispSetColumn( sesid,
                tableid,
                g_rgcolumnidPageInfoTable[icolumnidPageInfoPgno],
                (BYTE *) &pgno,
                sizeof(pgno),
                0,
                NULL ) );
        }

        /*  set FChecked
        /**/
        Call( ErrDispSetColumn( sesid,
            tableid,
            g_rgcolumnidPageInfoTable[icolumnidPageInfoFChecked],
            (BYTE *)&fChecked,
            sizeof(fChecked),
            0,
            NULL ) );

        /*  fAvail set if in AvailExt node
        /**/
        if ( fAvail )
        {
            Call( ErrDispSetColumn( sesid,
                tableid,
                g_rgcolumnidPageInfoTable[icolumnidPageInfoFAvail],
                (BYTE *) &fAvail,
                sizeof(fAvail),
                0,
                NULL ) );
        }

        /*  update
        /**/
        Call( ErrDispUpdate( sesid, tableid, NULL, 0, NULL, 0 ) );
        
        /*  commit
        /**/
Commit:
        Assert( ppib->Level() == 1 );
        Call( ErrIsamCommitTransaction( ( JET_SESID ) ppib, 0 ) );
    }

    return JET_errSuccess;
    
HandleError:
    CallS( ErrIsamRollback( (JET_SESID) ppib, JET_bitRollbackAll ) );

    return err;
}


//  ****************************************************************
//  DBCC Info Routines
//  ****************************************************************


//  ================================================================
LOCAL_BROKEN ERR ErrDBUTLPrintPageDump( DBCCINFO *pdbccinfo )
//  ================================================================
{
    ERR                 err;
    const JET_SESID     sesid   = (JET_SESID) pdbccinfo->ppib;
    const JET_TABLEID   tableid = pdbccinfo->tableidPageInfo;
    ULONG               cbT;
    
    FUCBSetSequential( reinterpret_cast<FUCB *>( tableid ) );

    Assert( pdbccinfo->grbitOptions & JET_bitDBUtilOptionPageDump );

    //  move to first record
    err = ErrDispMove( sesid, tableid, JET_MoveFirst, 0 );
    if ( JET_errNoCurrentRecord != err )
    {
        err = JET_errSuccess;
        goto HandleError;
    }
    Call( err );
    
    printf( "\n\n ***************** PAGE DUMP *******************\n\n" );
    printf( "PGNO\tAVAIL\tCHECK\tLEFT\tRIGHT\tFREESPACE\n" );

    /*  while there are more records, print record
    /**/
    for( ;
        JET_errSuccess == err;
        err = ErrDispMove( sesid, tableid, JET_MoveNext, 0 ) )
    {
        PGNO    pgnoThis    = pgnoNull;
        PGNO    pgnoLeft    = pgnoNull;
        PGNO    pgnoRight   = pgnoNull;
        BYTE    fChecked    = fFalse;
        BYTE    fAvail      = fFalse;
        ULONG   cbFreeSpace = 0;
        
        //  pgno
        Call( ErrDispRetrieveColumn( sesid,
            tableid,
            g_rgcolumnidPageInfoTable[icolumnidPageInfoPgno],
            (BYTE *) &pgnoThis,
            sizeof(pgnoThis),
            &cbT,
            0,
            NULL ) );
        Assert( sizeof(pgnoThis) == cbT );
        
        //  FAvail
        Call( ErrDispRetrieveColumn( sesid,
            tableid,
            g_rgcolumnidPageInfoTable[icolumnidPageInfoFAvail],
            (BYTE *) &fAvail,
            sizeof(fAvail),
            &cbT,
            0,
            NULL ) );
        Assert( sizeof(fAvail) == cbT || JET_wrnColumnNull == err );
        
        //  FChecked
        Call( ErrDispRetrieveColumn( sesid,
            tableid,
            g_rgcolumnidPageInfoTable[icolumnidPageInfoFChecked],
            (BYTE *)&fChecked,
            sizeof(fChecked),
            &cbT,
            0,
            NULL ) );
        Assert( cbT == sizeof(fChecked) );
        Assert( fChecked || fAvail );
        
        //  left and right pgno
        Call( ErrDispRetrieveColumn( sesid,
            tableid,
            g_rgcolumnidPageInfoTable[icolumnidPageInfoPgnoLeft],
            (BYTE *)&pgnoLeft,
            sizeof(pgnoLeft),
            &cbT,
            0,
            NULL ) );
        Assert( cbT == sizeof(pgnoLeft) );
        
        Call( ErrDispRetrieveColumn( sesid,
             tableid,
             g_rgcolumnidPageInfoTable[icolumnidPageInfoPgnoRight],
             (BYTE *) &pgnoRight,
             sizeof(pgnoRight),
             &cbT,
             0,
             NULL ) );
        Assert( cbT == sizeof(pgnoRight) );
        
        //  free space
        Call( ErrDispRetrieveColumn( sesid,
            tableid,
            g_rgcolumnidPageInfoTable[icolumnidPageInfoFreeSpace],
            (BYTE *) &cbFreeSpace,
            sizeof(cbFreeSpace),
            &cbT,
            0,
            NULL ) );
        Assert( cbT == sizeof(cbFreeSpace) );

        //  print
        printf( "%u\t%s\t%s", pgnoThis, fAvail ? "FAvail" : "", fChecked ? "FCheck" : "" );
        if( fChecked )
        {
            printf( "\t%u\t%u\t%u", pgnoLeft, pgnoRight, cbFreeSpace );
        }
        printf( "\n" );
    }

    //  polymorph expected error to success
    if ( JET_errNoCurrentRecord == err )
        err = JET_errSuccess;

HandleError:
    return err;
}

#ifdef DEBUG

//  ================================================================
LOCAL ERR ErrDBUTLISzToData( const CHAR * const sz, DATA * const pdata )
//  ================================================================
{
    DATA& data = *pdata;
    
    const LONG cch = (LONG)strlen( sz );
    if( cch % 2 == 1
        || cch <= 0 )
    {
        //  no data to insert
        return ErrERRCheck( JET_errInvalidParameter );
    }
    const LONG cbInsert = cch / 2;
    BYTE * pbInsert = (BYTE *)PvOSMemoryHeapAlloc( cbInsert );
    if( NULL == pbInsert )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    for( INT ibInsert = 0; ibInsert < cbInsert; ++ibInsert )
    {
        CHAR szConvert[3];
        szConvert[0] = sz[ibInsert * 2 ];
        szConvert[1] = sz[ibInsert * 2 + 1];
        szConvert[2] = 0;
        CHAR * pchEnd;
        const ULONG lConvert = strtoul( szConvert, &pchEnd, 16 );
        if( lConvert > 0xff
            || 0 != *pchEnd )
        {
            OSMemoryHeapFree( pbInsert );
            return ErrERRCheck( JET_errInvalidParameter );
        }
        pbInsert[ibInsert] = (BYTE)lConvert;
    }
    
    data.SetCb( cbInsert );
    data.SetPv( pbInsert );

    return JET_errSuccess;
}


//  ================================================================
LOCAL ERR ErrDBUTLIInsertNode(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgno,
    const LONG iline,
    const DATA& data,
    CPRINTF * const pcprintf )
//  ================================================================
{
    ERR err = JET_errSuccess;

    CPAGE cpage;
    CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
    Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

    cpage.Dirty( bfdfFilthy );

    (*pcprintf)( "inserting data at %d:%d\r\n", pgno, iline );
    cpage.Insert( iline, &data, 1, 0 );

HandleError:
    cpage.ReleaseWriteLatch( fTrue );

    return err;
}


//  ================================================================
LOCAL ERR ErrDBUTLIReplaceNode(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgno,
    const LONG iline,
    const DATA& data,
    CPRINTF * const pcprintf )
//  ================================================================
{
    ERR err = JET_errSuccess;

    CPAGE cpage;
    CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
    Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

    cpage.Dirty( bfdfFilthy );

    (*pcprintf)( "replacing data at %lu:%d\r\n", pgno, iline );
    cpage.Replace( iline, &data, 1, 0 );

HandleError:
    cpage.ReleaseWriteLatch( fTrue );

    return err;
}


//  ================================================================
LOCAL ERR ErrDBUTLISetNodeFlags(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgno,
    const LONG iline,
    const INT fFlags,
    CPRINTF * const pcprintf )
//  ================================================================
{
    ERR err = JET_errSuccess;

    CPAGE cpage;
    CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
    Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

    cpage.Dirty( bfdfFilthy );

    (*pcprintf)( "settings flags at %lu:%d to 0x%x\r\n", pgno, iline, fFlags );
    cpage.ReplaceFlags( iline, fFlags );

HandleError:
    cpage.ReleaseWriteLatch( fTrue );

    return err;
}


//  ================================================================
LOCAL ERR ErrDBUTLIDeleteNode(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgno,
    const LONG iline,
    CPRINTF * const pcprintf )
//  ================================================================
{
    ERR err = JET_errSuccess;

    CPAGE cpage;
    CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
    Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

    cpage.Dirty( bfdfFilthy );

    (*pcprintf)( "deleting %lu:%d\r\n", pgno, iline );
    cpage.Delete( iline );

HandleError:
    cpage.ReleaseWriteLatch( fTrue );

    return err;
}


//  ================================================================
LOCAL ERR ErrDBUTLISetExternalHeader(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgno,
    const DATA& data,
    CPRINTF * const pcprintf )
//  ================================================================
{
    ERR err = JET_errSuccess;

    CPAGE cpage;
    CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
    Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

    cpage.Dirty( bfdfFilthy );

    (*pcprintf)( "setting external header of %lu\r\n", pgno );
    cpage.SetExternalHeader( &data, 1, 0 );

HandleError:
    cpage.ReleaseWriteLatch( fTrue );

    return err;
}


//  ================================================================
LOCAL ERR ErrDBUTLISetPgnoNext(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgno,
    const PGNO pgnoNext,
    CPRINTF * const pcprintf )
//  ================================================================
{
    ERR err = JET_errSuccess;

    CPAGE cpage;
    CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
    Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

    cpage.Dirty( bfdfFilthy );

    (*pcprintf)( "setting pgnoNext of %lu to %lu (was %lu)\r\n", pgno, pgnoNext, cpage.PgnoNext() );
    cpage.SetPgnoNext( pgnoNext );

HandleError:
    cpage.ReleaseWriteLatch( fTrue );

    return err;
}


//  ================================================================
LOCAL ERR ErrDBUTLISetPgnoPrev(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgno,
    const PGNO pgnoPrev,
    CPRINTF * const pcprintf )
//  ================================================================
{
    ERR err = JET_errSuccess;

    CPAGE cpage;
    CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
    Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

    cpage.Dirty( bfdfFilthy );

    (*pcprintf)( "setting pgnoPrev of %lu to %lu (was %lu)\r\n", pgno, pgnoPrev, cpage.PgnoPrev() );
    cpage.SetPgnoPrev( pgnoPrev );

HandleError:
    cpage.ReleaseWriteLatch( fTrue );

    return err;
}


//  ================================================================
LOCAL ERR ErrDBUTLISetPageFlags(
    PIB * const ppib,
    const IFMP ifmp,
    const PGNO pgno,
    const ULONG fFlags,
    CPRINTF * const pcprintf )
//  ================================================================
{
    ERR err = JET_errSuccess;

    CPAGE cpage;
    CallR( cpage.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );
    Call( cpage.ErrUpgradeReadLatchToWriteLatch() );

    cpage.Dirty( bfdfFilthy );

    (*pcprintf)( "setting flags of %lu to 0x%x (was 0x%x)\r\n", pgno, fFlags, cpage.FFlags() );
    cpage.SetFlags( fFlags );

HandleError:
    cpage.ReleaseWriteLatch( fTrue );

    return err;
}


//  ================================================================
LOCAL_BROKEN ERR ErrDBUTLMungeDatabase(
    PIB * const ppib,
    const IFMP ifmp,
    const CHAR * const rgszCommand[],
    const INT cszCommand,
    CPRINTF * const pcprintf )
//  ================================================================
{
    ERR err = JET_errSuccess;

    if( 3 == cszCommand
        && _stricmp( rgszCommand[0], "insert" ) == 0 )
    {
        PGNO    pgno;
        LONG    iline;
        DATA    data;
        
        if( 2 != sscanf_s( rgszCommand[1], "%lu:%d", &pgno, &iline ) )
        {
            //  we didn't get all the arguments we need
            return ErrERRCheck( JET_errInvalidParameter );
        }
        
        CallR( ErrDBUTLISzToData( rgszCommand[2], &data ) );
        err = ErrDBUTLIInsertNode( ppib, ifmp, pgno, iline, data, pcprintf );
        OSMemoryHeapFree( data.Pv() );
        return err;
    }
    if( 3 == cszCommand
        && _stricmp( rgszCommand[0], "replace" ) == 0 )
    {
        PGNO    pgno;
        LONG    iline;
        DATA    data;
        
        if( 2 != sscanf_s( rgszCommand[1], "%lu:%d", &pgno, &iline ) )
        {
            //  we didn't get all the arguments we need
            return ErrERRCheck( JET_errInvalidParameter );
        }
        
        CallR( ErrDBUTLISzToData( rgszCommand[2], &data ) );
        err = ErrDBUTLIReplaceNode( ppib, ifmp, pgno, iline, data, pcprintf );
        OSMemoryHeapFree( data.Pv() );
        
        return err;
    }
    if( 3 == cszCommand
        && _stricmp( rgszCommand[0], "setflags" ) == 0 )
    {
        PGNO    pgno;
        LONG    iline;
        if( 2 != sscanf_s( rgszCommand[1], "%lu:%d", &pgno, &iline ) )
        {
            //  we didn't get all the arguments we need
            return ErrERRCheck( JET_errInvalidParameter );
        }

        const ULONG fFlags = strtoul( rgszCommand[2], NULL, 0 );
        err = ErrDBUTLISetNodeFlags( ppib, ifmp, pgno, iline, fFlags, pcprintf );
        
        return err;
    }
    else if( 2 == cszCommand
        && _stricmp( rgszCommand[0], "delete" ) == 0 )
    {
        PGNO    pgno;
        LONG    iline;
        
        if( 2 != sscanf_s( rgszCommand[1], "%lu:%d", &pgno, &iline ) )
        {
            //  we didn't get all the arguments we need
            return ErrERRCheck( JET_errInvalidParameter );
        }
        
        err = ErrDBUTLIDeleteNode( ppib, ifmp, pgno, iline, pcprintf );
        return err;
    }
    if( 3 == cszCommand
        && _stricmp( rgszCommand[0], "exthdr" ) == 0 )
    {
        char * pchEnd;
        const PGNO pgno = strtoul( rgszCommand[1], &pchEnd, 0 );
        if( pgnoNull == pgno
            || 0 != *pchEnd )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
        
        DATA    data;
        CallR( ErrDBUTLISzToData( rgszCommand[2], &data ) );
        err = ErrDBUTLISetExternalHeader( ppib, ifmp, pgno, data, pcprintf );
        OSMemoryHeapFree( data.Pv() );
        
        return err;
    }
    if( 3 == cszCommand
        && _stricmp( rgszCommand[0], "pgnonext" ) == 0 )
    {
        char * pchEnd;
        const PGNO pgno = strtoul( rgszCommand[1], &pchEnd, 0 );
        if( pgnoNull == pgno
            || 0 != *pchEnd )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
        const PGNO pgnoNext = strtoul( rgszCommand[2], NULL, 0 );
        if( 0 != *pchEnd )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
        err = ErrDBUTLISetPgnoNext( ppib, ifmp, pgno, pgnoNext, pcprintf );
        return err;
    }
    if( 3 == cszCommand
        && _stricmp( rgszCommand[0], "pgnoprev" ) == 0 )
    {
        char * pchEnd;
        const PGNO pgno = strtoul( rgszCommand[1], &pchEnd, 0 );
        if( pgnoNull == pgno
            || 0 != *pchEnd )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
        const PGNO pgnoPrev = strtoul( rgszCommand[2], NULL, 0 );
        if( 0 != *pchEnd )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
        err = ErrDBUTLISetPgnoPrev( ppib, ifmp, pgno, pgnoPrev, pcprintf );
        return err;
    }
    if( 3 == cszCommand
        && _stricmp( rgszCommand[0], "pageflags" ) == 0 )
    {
        char * pchEnd;
        const PGNO pgno = strtoul( rgszCommand[1], &pchEnd, 0 );
        if( pgnoNull == pgno
            || 0 != *pchEnd )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
        const ULONG fFlags = strtoul( rgszCommand[2], NULL, 0 );
        if( 0 != *pchEnd )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
        err = ErrDBUTLISetPageFlags( ppib, ifmp, pgno, fFlags, pcprintf );
        return err;
    }
    if( 1 == cszCommand
        && _stricmp( rgszCommand[0], "help" ) == 0 )
    {
        (*pcprintf)( "insert <pgno>:<iline> <data>  -  insert a node\r\n" );
        (*pcprintf)( "replace <pgno>:<iline> <data>  -  replace a node\r\n" );
        (*pcprintf)( "delete <pgno>:<iline>  -  delete a node\r\n" );
        (*pcprintf)( "setflags <pgno>:<iline> <flags>  -  set flags on a node\r\n" );
        (*pcprintf)( "exthdr <pgno> <data>  -  set external header\r\n" );
        (*pcprintf)( "pgnonext <pgno> <pgnonext>  -  set pgnonext on a page\r\n" );
        (*pcprintf)( "pgnoprev <pgno> <pgnoprev>  -  set pgnoprev on a page\r\n" );
        (*pcprintf)( "pageflags <pgno> <flags>  -  set flags on a page\r\n" );
    }
    else
    {
        (*pcprintf)( "unknown command \"%s\"\r\n", rgszCommand[0] );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    return err;
}
#endif // DEBUG

//  ================================================================
LOCAL ERR ErrDBUTLDumpOneColumn( PIB * ppib, FUCB * pfucbCatalog, VOID * pfnCallback, VOID * pvCallback )
//  ================================================================
{
    JET_RETRIEVECOLUMN  rgretrievecolumn[10];
    COLUMNDEF           columndef;
    ERR                 err             = JET_errSuccess;
    INT                 iretrievecolumn = 0;

    memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );
    memset( &columndef, 0, sizeof( columndef ) );

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Name;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)columndef.szName;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( columndef.szName );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Id;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( columndef.columnid );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( columndef.columnid );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Coltyp;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( columndef.coltyp );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( columndef.coltyp );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Localization;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( columndef.cp );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( columndef.cp );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Flags;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( columndef.fFlags );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( columndef.fFlags );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_SpaceUsage;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( columndef.cbLength );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( columndef.cbLength );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_RecordOffset;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( columndef.ibRecordOffset );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( columndef.ibRecordOffset );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Callback;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( columndef.szCallback );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( columndef.szCallback );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_CallbackData;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( columndef.rgbCallbackData );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( columndef.rgbCallbackData );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_DefaultValue;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)columndef.rgbDefaultValue;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( columndef.rgbDefaultValue );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    CallR( ErrIsamRetrieveColumns(
                (JET_SESID)ppib,
                (JET_TABLEID)pfucbCatalog,
                rgretrievecolumn,
                iretrievecolumn ) );

    // WARNING: if the order of rgretrievecolumn initialization is changed above this must change too
    columndef.cbDefaultValue    = rgretrievecolumn[iretrievecolumn-1].cbActual;
    columndef.cbCallbackData    = rgretrievecolumn[iretrievecolumn-2].cbActual;

    columndef.fFixed            = !!FidOfColumnid( columndef.columnid ).FFixed();
    columndef.fVariable         = !!FidOfColumnid( columndef.columnid ).FVar();
    columndef.fTagged           = !!FidOfColumnid( columndef.columnid ).FTagged();

    const FIELDFLAG ffield      = FIELDFLAG( columndef.fFlags );
    columndef.fVersion          = !!FFIELDVersion( ffield );
    columndef.fNotNull          = !!FFIELDNotNull( ffield );
    columndef.fMultiValue       = !!FFIELDMultivalued( ffield );
    columndef.fAutoIncrement    = !!FFIELDAutoincrement( ffield );
    columndef.fDefaultValue     = !!FFIELDDefault( ffield );
    columndef.fEscrowUpdate     = !!FFIELDEscrowUpdate( ffield );
    columndef.fVersioned        = !!FFIELDVersioned( ffield );
    columndef.fDeleted          = !!FFIELDDeleted( ffield );
    columndef.fFinalize         = !!FFIELDFinalize( ffield );
    columndef.fDeleteOnZero     = !!FFIELDDeleteOnZero( ffield );
    columndef.fUserDefinedDefault       = !!FFIELDUserDefinedDefault( ffield );
    columndef.fTemplateColumnESE98      = !!FFIELDTemplateColumnESE98( ffield );
    columndef.fPrimaryIndexPlaceholder  = !!FFIELDPrimaryIndexPlaceholder( ffield );
    columndef.fCompressed               = !!FFIELDCompressed( ffield );
    columndef.fEncrypted                = !!FFIELDEncrypted( ffield );

    PFNCOLUMN const pfncolumn   = (PFNCOLUMN)pfnCallback;
    return (*pfncolumn)( &columndef, pvCallback );
}


//  ================================================================
LOCAL ERR ErrDBUTLDumpOneCallback( PIB * ppib, FUCB * pfucbCatalog, VOID * pfnCallback, VOID * pvCallback )
//  ================================================================
{
    JET_RETRIEVECOLUMN  rgretrievecolumn[3];
    CALLBACKDEF         callbackdef;
    ERR                 err             = JET_errSuccess;
    INT                 iretrievecolumn = 0;

    memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );
    memset( &callbackdef, 0, sizeof( callbackdef ) );

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Name;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)callbackdef.szName;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( callbackdef.szName );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Flags;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( callbackdef.cbtyp );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( callbackdef.cbtyp );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Callback;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)callbackdef.szCallback;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( callbackdef.szCallback );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    CallR( ErrIsamRetrieveColumns(
                (JET_SESID)ppib,
                (JET_TABLEID)pfucbCatalog,
                rgretrievecolumn,
                iretrievecolumn ) );

    PFNCALLBACKFN const pfncallback = (PFNCALLBACKFN)pfnCallback;
    return (*pfncallback)( &callbackdef, pvCallback );
}


//  ================================================================
LOCAL ERR ErrDBUTLDumpPage( PIB * ppib, IFMP ifmp, PGNO pgno, PFNPAGE pfnpage, VOID * pvCallback )
//  ================================================================
{
    ERR err = JET_errSuccess;
    CSR csr;
    Call( csr.ErrGetReadPage( ppib, ifmp, pgno, bflfNoTouch ) );

    PAGEDEF pagedef;

    pagedef.dbtime      = csr.Cpage().Dbtime();
    pagedef.pgno        = pgno;
    pagedef.objidFDP    = csr.Cpage().ObjidFDP();
    pagedef.pgnoNext    = csr.Cpage().PgnoNext();
    pagedef.pgnoPrev    = csr.Cpage().PgnoPrev();
    
    pagedef.pbRawPage   = reinterpret_cast<const BYTE *>( csr.Cpage().PvBuffer() );

    pagedef.cbFree      = csr.Cpage().CbPageFree();
    pagedef.cbUncommittedFree   = csr.Cpage().CbUncommittedFree();
    pagedef.clines      = SHORT( csr.Cpage().Clines() );

    pagedef.fFlags          = csr.Cpage().FFlags();

    pagedef.fLeafPage       = !!csr.Cpage().FLeafPage();
    pagedef.fInvisibleSons  = !!csr.Cpage().FInvisibleSons();
    pagedef.fRootPage       = !!csr.Cpage().FRootPage();
    pagedef.fPrimaryPage    = !!csr.Cpage().FPrimaryPage();
    pagedef.fParentOfLeaf   = !!csr.Cpage().FParentOfLeaf();

    if( pagedef.fInvisibleSons )
    {
        Assert( !pagedef.fLeafPage );
        
        INT iline;
        for( iline = 0; iline < pagedef.clines; iline++ )
        {
            KEYDATAFLAGS    kdf;
            
            csr.SetILine( iline );
            NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdf );

            Assert( kdf.data.Cb() == sizeof( PGNO ) );
            pagedef.rgpgnoChildren[iline] = *((UnalignedLittleEndian< PGNO > *)kdf.data.Pv() );
        }
        pagedef.rgpgnoChildren[pagedef.clines] = pgnoNull;
    }
    else
    {
        Assert( pagedef.fLeafPage );
    }

    Call( (*pfnpage)( &pagedef, pvCallback ) );

HandleError:
    csr.ReleasePage();
    return err;
}

//  ================================================================
LOCAL INT PrintCallback( const CALLBACKDEF * pcallbackdef, void * )
//  ================================================================
{
    //  this will only compile if the signatures match
    PFNCALLBACKFN pfncallback = PrintCallback;

    Unused( pfncallback );

    char szCbtyp[255];
    szCbtyp[0] = 0;

    if( JET_cbtypNull == pcallbackdef->cbtyp )
    {
        OSStrCbAppendA( szCbtyp, sizeof(szCbtyp), "NULL" );
    }
    if( pcallbackdef->cbtyp & JET_cbtypFinalize )
    {
        OSStrCbAppendA( szCbtyp, sizeof(szCbtyp), "Finalize|" );
    }
    if( pcallbackdef->cbtyp & JET_cbtypBeforeInsert )
    {
        OSStrCbAppendA( szCbtyp, sizeof(szCbtyp), "BeforeInsert|" );
    }
    if( pcallbackdef->cbtyp & JET_cbtypAfterInsert )
    {
        OSStrCbAppendA( szCbtyp, sizeof(szCbtyp), "AfterInsert|" );
    }
    if( pcallbackdef->cbtyp & JET_cbtypBeforeReplace )
    {
        OSStrCbAppendA( szCbtyp, sizeof(szCbtyp), "BeforeReplace|" );
    }
    if( pcallbackdef->cbtyp & JET_cbtypAfterReplace )
    {
        OSStrCbAppendA( szCbtyp, sizeof(szCbtyp), "AfterReplace|" );
    }
    if( pcallbackdef->cbtyp & JET_cbtypBeforeDelete )
    {
        OSStrCbAppendA( szCbtyp, sizeof(szCbtyp), "BeforeDelete|" );
    }
    if( pcallbackdef->cbtyp & JET_cbtypAfterDelete )
    {
        OSStrCbAppendA( szCbtyp, sizeof(szCbtyp), "AfterDelete|" );
    }
    szCbtyp[strlen( szCbtyp ) - 1] = 0;

    printf( "    %2.2d (%s)   %s\n", pcallbackdef->cbtyp, szCbtyp, pcallbackdef->szCallback );
    return 0;
}

//  ================================================================
LOCAL VOID PrintSpaceHintMetaData(
    _In_ const CHAR * const szIndent,
    _In_ const CHAR * const szObjectType,
    _In_ const JET_SPACEHINTS * const pSpaceHints )
//  ================================================================
{
    printf( "%s%s Space Hints:       cbStruct: %d, grbit=0x%x\n", szIndent, szObjectType,
                pSpaceHints->cbStruct, pSpaceHints->grbit );
    printf( "%s    Densities:     Initial=%u%%,  Maintenance=%u%%\n", szIndent,
                pSpaceHints->ulInitialDensity , pSpaceHints->ulMaintDensity );
    printf( "%s    Growth Hints:  cbInitial=%u, ulGrowth=%u%%,  cbMinExtent=%u, cbMaxExtent=%u\n", szIndent,
                pSpaceHints->cbInitial, pSpaceHints->ulGrowth,
                pSpaceHints->cbMinExtent, pSpaceHints->cbMaxExtent );
}


//  ================================================================
LOCAL INT PrintIndexMetaData( const INDEXDEF * pindexdef, void * )
//  ================================================================
{
    //  this will only compile if the signatures match
    PFNINDEX pfnindex = PrintIndexMetaData;

    Unused( pfnindex );
    
    Assert( pindexdef );
    
    printf( "    %-15.15s  ", pindexdef->szName );
    DBUTLPrintfIntN( pindexdef->pgnoFDP, 8 );
    printf( "  " );
    DBUTLPrintfIntN( pindexdef->objidFDP, 8 );
    printf( "  " );
    DBUTLPrintfIntN( pindexdef->density, 6 );
    printf( "%%\n" );

    if ( pindexdef->fUnique )
        printf( "        Unique=yes\n" );
    if ( pindexdef->fPrimary )
        printf( "        Primary=yes\n" );
    if ( pindexdef->fTemplateIndex )
        printf( "        Template=yes\n" );
    if ( pindexdef->fDerivedIndex )
        printf( "        Derived=yes\n" );
    if ( pindexdef->fNoNullSeg )
        printf( "        Disallow Null=yes\n" );
    if ( pindexdef->fAllowAllNulls )
        printf( "        Allow All Nulls=yes\n" );
    if ( pindexdef->fAllowFirstNull )
        printf( "        Allow First Null=yes\n" );
    if ( pindexdef->fAllowSomeNulls )
        printf( "        Allow Some Nulls=yes\n" );
    if ( pindexdef->fSortNullsHigh )
        printf( "        Sort Nulls High=yes\n" );
    if ( pindexdef->fMultivalued )
        printf( "        Multivalued=yes\n" );
    if ( pindexdef->fTuples )
    {
        printf( "        Tuples=yes\n" );
        printf( "            LengthMin=%d\n", (ULONG) pindexdef->le_tuplelimits.le_chLengthMin );
        printf( "            LengthMax=%d\n", (ULONG) pindexdef->le_tuplelimits.le_chLengthMax );
        printf( "            ToIndexMax=%d\n", (ULONG) pindexdef->le_tuplelimits.le_chToIndexMax );
        printf( "            CchIncrement=%d\n", (ULONG) pindexdef->le_tuplelimits.le_cchIncrement );
        printf( "            IchStart=%d\n", (ULONG) pindexdef->le_tuplelimits.le_ichStart );
    }
    if ( pindexdef->fLocalizedText )
    {
        WCHAR   wszLocaleName[NORM_LOCALE_NAME_MAX_LENGTH];
        QWORD   qwCurrSortVersion;
        SORTID  sortID;

        printf( "        Localized Text=yes\n" );
        printf( "            Locale Id=%d\n", pindexdef->lcid );
        printf( "            Locale Name=%ws\n", pindexdef->wszLocaleName );
        printf( "            LCMap flags=0x%08x\n", pindexdef->dwMapFlags );

        //  try to report current sort version. if it has an LCID, convert it to locale and get its sort version. Or
        //  try to use the indexdef's locale name.
        if ( ( ( 0 != pindexdef->lcid ) &&
                JET_errSuccess == ErrNORMLcidToLocale( pindexdef->lcid, wszLocaleName, _countof( wszLocaleName ) ) &&
                JET_errSuccess == ErrNORMGetSortVersion( wszLocaleName, &qwCurrSortVersion, &sortID ) ) ||
            ( JET_errSuccess == ErrNORMGetSortVersion( pindexdef->wszLocaleName, &qwCurrSortVersion, &sortID ) ) )
        {
            WCHAR wszSortID[ PERSISTED_SORTID_MAX_LENGTH ] = L"";
            WCHAR wszIndexDefSortID[ PERSISTED_SORTID_MAX_LENGTH ] = L"";
            WszCATFormatSortID( pindexdef->sortID, wszIndexDefSortID, _countof( wszIndexDefSortID ) );
            WszCATFormatSortID( sortID, wszSortID, _countof( wszSortID ) );

            printf( "            NLSVersion=%d (current OS: %d)\n", pindexdef->dwNLSVersion, DWORD( ( qwCurrSortVersion >> 32 ) & 0xFFFFFFFF ) );
            printf( "            DefinedVersion=%d (current OS: %d)\n", pindexdef->dwDefinedVersion, DWORD( qwCurrSortVersion & 0xFFFFFFFF ) );
            printf( "            SortID=%ws (current OS: %ws)\n", wszIndexDefSortID, wszSortID );
        }
        else
        {
            WCHAR wszIndexDefSortID[ PERSISTED_SORTID_MAX_LENGTH ] = L"";
            WszCATFormatSortID( pindexdef->sortID, wszIndexDefSortID, _countof( wszIndexDefSortID ) );

            printf( "            NLSVersion=%d (current OS: <unknown>)\n", pindexdef->dwNLSVersion );
            printf( "            DefinedVersion=%d (current OS: <unknown>)\n", pindexdef->dwDefinedVersion );
            printf( "            SortID=%ws (current OS: <unknown>)\n", wszIndexDefSortID );
        }
    }
    if ( pindexdef->fExtendedColumns )
        printf( "        Extended Columns=yes\n" );
    printf( "        Flags=0x%08x\n", pindexdef->fFlags );

    if ( pindexdef->cbVarSegMac )
    {
        printf( "        Index Key Segment Length Specified=yes\n" );
        printf( "            Maximum Key Segment Length=0x%08x\n", pindexdef->cbVarSegMac );
    }

    if ( pindexdef->cbKeyMost )
    {
        printf( "        Index Key Length Specified=yes\n" );
        printf( "            Maximum Key Length=0x%08x\n", pindexdef->cbKeyMost );
    }

    PrintSpaceHintMetaData( "        ", "Index", &(pindexdef->spacehints) );

    UINT isz;

    Assert( pindexdef->ccolumnidDef > 0 );
    if ( pindexdef->fExtendedColumns )
    {
        printf( "        Key Segments (%d)\n", pindexdef->ccolumnidDef );
        printf( "        -----------------\n" );
    }
    else
    {
        printf( "        Key Segments (%d - ESE97 format)\n", pindexdef->ccolumnidDef );
        printf( "        --------------------------------\n" );
    }
    for( isz = 0; isz < (ULONG)pindexdef->ccolumnidDef; isz++ )
    {
        printf( "            %-15.15s (0x%08x)\n", pindexdef->rgszIndexDef[isz], pindexdef->rgidxsegDef[isz].Columnid() );
    }

    if( pindexdef->ccolumnidConditional > 0 )
    {
        if ( pindexdef->fExtendedColumns )
        {
            printf( "        Conditional Columns (%d)\n", pindexdef->ccolumnidConditional );
            printf( "        ------------------------\n" );
        }
        else
        {
            printf( "        Conditional Columns (%d - ESE97 format)\n", pindexdef->ccolumnidConditional );
            printf( "        ---------------------------------------\n" );
        }
        for( isz = 0; isz < (ULONG)pindexdef->ccolumnidConditional; isz++ )
        {
            printf( "            %-15.15s (0x%08x,%s)\n",
                    ( pindexdef->rgszIndexConditional[isz] ) + 1,
                    pindexdef->rgidxsegConditional[isz].Columnid(),
                    ( pindexdef->rgidxsegConditional[isz] ).FMustBeNull() ? "JET_bitIndexColumnMustBeNull" : "JET_bitIndexColumnMustBeNonNull" );
        }
    }

    LOGTIME tmPgnoFDPLastSet = { 0 };
    ConvertFileTimeToLogTime( pindexdef->ftPgnoFDPLastSet, &tmPgnoFDPLastSet );

    printf("        IndexPgnoFDPLastSetTime=" );
    DUMPPrintLogTime( &tmPgnoFDPLastSet );
    printf( "\n" );

    return 0;
}

    
//  ================================================================
LOCAL INT PrintColumn( const COLUMNDEF * pcolumndef, void * )
//  ================================================================
{
    //  this will only compile if the signatures match
    PFNCOLUMN pfncolumn = PrintColumn;

    Unused( pfncolumn );
    
    Assert( pcolumndef );
    
    printf( "    %-15.15s ", pcolumndef->szName );
    DBUTLPrintfIntN( pcolumndef->columnid, 9 );

    const CHAR * szType;
    const CHAR * szFVT;
    CHAR szUnknown[50];

    if( pcolumndef->fFixed )
    {
        szFVT = "(F)";
    }
    else if( pcolumndef->fTagged )
    {
        szFVT = "(T)";
    }
    else if( pcolumndef->fVariable )
    {
        szFVT = "(V)";
    }
    else
    {
        AssertSz( fFalse, "Unknown column type is not fixed, tagged, or variable." );
        szFVT = "(?)";  // unknown?  What type of column is this?
    }
    
    switch ( pcolumndef->coltyp )
    {
        case JET_coltypBit:
            szType = "Bit";
            break;
            
        case JET_coltypUnsignedByte:
            szType = "UnsignedByte";
            break;

        case JET_coltypShort:
            szType = "Short";
            break;

        case JET_coltypUnsignedShort:
            szType = "UnsignedShort";
            break;
            
        case JET_coltypLong:
            szType = "Long";
            break;

        case JET_coltypUnsignedLong:
            szType = "UnsignedLong";
            break;
            
        case JET_coltypLongLong:
            szType = "LongLong";
            break;
            
        case JET_coltypUnsignedLongLong:
            szType = "UnsignedLongLong";
            break;

        case JET_coltypCurrency:
            szType = "Currency";
            break;

        case JET_coltypIEEESingle:
            szType = "IEEESingle";
            break;
            
        case JET_coltypIEEEDouble:
            szType = "IEEEDouble";
            break;
            
        case JET_coltypDateTime:
            szType = "DateTime";
            break;
            
        case JET_coltypGUID:
            szType = "GUID";
            break;

        case JET_coltypBinary:
            szType = "Binary";
            break;

        case JET_coltypText:
            szType = "Text";
            break;

        case JET_coltypLongBinary:
            szType = "LongBinary";
            break;

        case JET_coltypLongText:
            szType = "LongText";
            break;

        case JET_coltypNil:
            szType = "Deleted";
            break;
            
        default:
            OSStrCbFormatA( szUnknown, sizeof(szUnknown), "???(%d)", pcolumndef->coltyp );
            szType = szUnknown;
            break;
    }

        printf("  %-12.12s%3.3s ", szType, szFVT );

        //  only have 7 characters for column length,
        //  so if it's greater than 7 digits, express
        //  the length in MB instead of bytes
        //
        if ( pcolumndef->cbLength > 9999999 )
        {
            DBUTLPrintfIntN( pcolumndef->cbLength / ( 1024 * 1024 ), 5 );
            printf("MB");
        }
        else
        {
            DBUTLPrintfIntN( pcolumndef->cbLength, 7 );
        }

        if ( 0 != pcolumndef->cbDefaultValue )
        {
            printf( "  Yes" );
        }
        printf( "\n" );

        if ( pcolumndef->fFixed )
            printf( "        Offset=%d (0x%x)\n", pcolumndef->ibRecordOffset, pcolumndef->ibRecordOffset );

        if ( FRECTextColumn( pcolumndef->coltyp ) )
        {
            printf( "        Code Page=%d (0x%x)\n", pcolumndef->cp, pcolumndef->cp );
        }

        if ( pcolumndef->fVersion )
            printf( "        Version=yes\n" );
        if ( pcolumndef->fNotNull )
            printf( "        Disallow Null=yes\n" );
        if ( pcolumndef->fMultiValue )
            printf( "        Multi-value=yes\n" );
        if ( pcolumndef->fAutoIncrement )
            printf( "        Auto-increment=yes\n" );
        if ( pcolumndef->fEscrowUpdate )
            printf( "        EscrowUpdate=yes\n" );
        if ( pcolumndef->fFinalize )
            printf( "        Finalize=yes\n" );
        if ( pcolumndef->fDeleteOnZero )
            printf( "        DeleteOnZero=yes\n" );
        if ( pcolumndef->fDefaultValue )
        {
            printf( "        DefaultValue=yes\n" );
            printf( "                Length=%d bytes\n", pcolumndef->cbDefaultValue );
        }
        if ( pcolumndef->fUserDefinedDefault )
        {
            printf( "        User-defined Default=yes\n" );
            printf( "                Callback=%s\n", pcolumndef->szCallback );
            printf( "                CallbackData=%d bytes\n", pcolumndef->cbCallbackData );
        }
        if ( pcolumndef->fTemplateColumnESE98 )
            printf( "        TemplateColumnESE98=yes\n" );
        if ( pcolumndef->fPrimaryIndexPlaceholder )
            printf( "        PrimaryIndexPlaceholder=yes\n" );
        if ( pcolumndef->fCompressed )
            printf( "        Compressed=yes\n" );
        if ( pcolumndef->fEncrypted )
            printf( "        Encrypted=yes\n" );

        printf( "        Flags=0x%x\n", pcolumndef->fFlags );

    return 0;
}

//  ================================================================
LOCAL INT PrintTableMetaData( const TABLEDEF * ptabledef, void * pv )
//  ================================================================
{
    //  this will only compile if the signatures match
    PFNTABLE pfntable = PrintTableMetaData;

    Unused( pfntable );

    Assert( ptabledef );
    JET_DBUTIL_A * pdbutil = (JET_DBUTIL_A *)pv;

    printf( "Table Name        PgnoFDP  ObjidFDP    PgnoLV   ObjidLV     Pages  Density\n"
            "==========================================================================\n"
            "%hs\n", ptabledef->szName );
    printf( "                 " ); // spaces rest to match up with PgnoFDP, ObjidFDP, etc.
    DBUTLPrintfIntN( ptabledef->pgnoFDP, 8 );
    printf( "  " );
    DBUTLPrintfIntN( ptabledef->objidFDP, 8 );
    printf( "  " );
    DBUTLPrintfIntN( ptabledef->pgnoFDPLongValues, 8 );
    printf( "  " );
    DBUTLPrintfIntN( ptabledef->objidFDPLongValues, 8 );
    printf( "  " );
    DBUTLPrintfIntN( ptabledef->pages, 8 );
    printf( "  " );
    DBUTLPrintfIntN( ptabledef->density, 6 );
    printf( "%%\n" );

    LOGTIME tmPgnoFDPLastSet = { 0 };
    LOGTIME tmPgnoFDPLongValuesLastSet = { 0 };
    ConvertFileTimeToLogTime( ptabledef->ftPgnoFDPLastSet, &tmPgnoFDPLastSet );
    ConvertFileTimeToLogTime( ptabledef->ftPgnoFDPLongValuesLastSet, &tmPgnoFDPLongValuesLastSet );

    printf("    PgnoFDPLastSetTime=" );
    DUMPPrintLogTime( &tmPgnoFDPLastSet );
    printf( "\n" );
    printf("    PgnoFDPLVLastSetTime=" );
    DUMPPrintLogTime( &tmPgnoFDPLongValuesLastSet );
    printf( "\n" );
    printf( "    LV ChunkSize=%d\n", ptabledef->cbLVChunkMax );

    if ( ptabledef->fFlags & JET_bitObjectSystem )
        printf( "    System Table=yes\n" );
    if ( ptabledef->fFlags & JET_bitObjectTableFixedDDL )
        printf( "    FixedDDL=yes\n" );
    if ( ptabledef->fFlags & JET_bitObjectTableTemplate )
        printf( "    Template=yes\n" );

    if ( NULL != ptabledef->szTemplateTable
        && '\0' != ptabledef->szTemplateTable[0] )
    {
        Assert( ptabledef->fFlags & JET_bitObjectTableDerived );
        printf( "    Derived From: %5s\n", ptabledef->szTemplateTable );
    }
    else
    {
        Assert( !( ptabledef->fFlags & JET_bitObjectTableDerived ) );
    }

    PrintSpaceHintMetaData( "    ", "Table", &(ptabledef->spacehints) );
    if ( ptabledef->spacehintsDeferredLV.cbStruct == sizeof(ptabledef->spacehintsDeferredLV) )
    {
        PrintSpaceHintMetaData( "    ", "Deferred LV", &(ptabledef->spacehintsDeferredLV) );
    }

    JET_DBUTIL_W    dbutil;
    ERR         err     = JET_errSuccess;

    printf( "    Column Name     Column Id  Column Type      Length  Default\n"
            "    -----------------------------------------------------------\n" );

    memset( &dbutil, 0, sizeof( dbutil ) );
    
    dbutil.cbStruct     = sizeof( dbutil );
    dbutil.op           = opDBUTILEDBDump;
    dbutil.sesid        = pdbutil->sesid;
    dbutil.dbid         = pdbutil->dbid;
    dbutil.pgno         = ptabledef->objidFDP;
    dbutil.pfnCallback  = (void *)PrintColumn;
    dbutil.pvCallback   = &dbutil;
    dbutil.edbdump      = opEDBDumpColumns;

    err = ErrDBUTLDump( pdbutil->sesid, &dbutil );

    printf( "    Index Name        PgnoFDP  ObjidFDP  Density\n"
            "    --------------------------------------------\n" );

    memset( &dbutil, 0, sizeof( dbutil ) );
    
    dbutil.cbStruct     = sizeof( dbutil );
    dbutil.op           = opDBUTILEDBDump;
    dbutil.sesid        = pdbutil->sesid;
    dbutil.dbid         = pdbutil->dbid;
    dbutil.pgno         = ptabledef->objidFDP;
    dbutil.pfnCallback  = (void *)PrintIndexMetaData;
    dbutil.pvCallback   = &dbutil;
    dbutil.edbdump      = opEDBDumpIndexes;

    err = ErrDBUTLDump( pdbutil->sesid, &dbutil );

    printf( "    Callback Type        Callback\n"
            "    --------------------------------------------\n" );

    memset( &dbutil, 0, sizeof( dbutil ) );
    
    dbutil.cbStruct     = sizeof( dbutil );
    dbutil.op           = opDBUTILEDBDump;
    dbutil.sesid        = pdbutil->sesid;
    dbutil.dbid         = pdbutil->dbid;
    dbutil.pgno         = ptabledef->objidFDP;
    dbutil.pfnCallback  = (void *)PrintCallback;
    dbutil.pvCallback   = &dbutil;
    dbutil.edbdump      = opEDBDumpCallbacks;

    err = ErrDBUTLDump( pdbutil->sesid, &dbutil );

    return err;
}


//  ================================================================
ERR ErrDBUTLGetIfmpFucbOfPage(
    _Inout_ JET_SESID sesid,
    PCWSTR wszDatabase,
    _In_ const CPAGE& cpage,
    _Out_ IFMP * pifmp,
    _Out_ FUCB ** ppfucb,
    _Out_writes_bytes_opt_(cbObjName) WCHAR * wszObjName = NULL,
    _In_ ULONG cbObjName = 0 );
#define cbDbutlObjNameMost ( JET_cbNameMost + 40 + 1 )
ERR ErrDBUTLGetIfmpFucbOfPage( _Inout_ JET_SESID sesid, PCWSTR wszDatabase, _In_ const CPAGE& cpage, _Out_ IFMP * pifmp, _Out_ FUCB ** ppfucbObj, _Out_writes_bytes_opt_(cbObjName) WCHAR * wszObjName, _In_ ULONG cbObjName )
//  ================================================================
{
    ERR             err         = JET_errSuccess;
    JET_DBID        jdbid       = JET_dbidNil;

    *pifmp = ifmpNil;
    *ppfucbObj = pfucbNil;

    //  We really do not know what MIGHT be wrong with this datababase, so let's do all this under
    //  an exception handle so that node or page or whatever dump can probably continue.
    TRY
    {
        // Disable index checking as we won't be using any _unicode text_ indexes.
        (void)SetParam( reinterpret_cast<PIB*>( sesid )->m_pinst, NULL, JET_paramEnableIndexChecking, JET_IndexCheckingOff, NULL );

        //  Must open database RO because it is already opened above us to directly read the page data.
        Call( ErrIsamAttachDatabase( sesid, wszDatabase, fFalse, NULL, 0, JET_bitDbReadOnly ) );
        err = ErrIsamOpenDatabase( sesid, wszDatabase, NULL, &jdbid, JET_bitDbReadOnly );
        if ( err < JET_errSuccess )
        {
            CallS( ErrIsamDetachDatabase( sesid, NULL, wszDatabase ) );
            Assert( jdbid == JET_dbidNil );
            jdbid = JET_dbidNil;
        }
        Call( err );

        // Try to do without trx because we want the FUCB not auto-closed by the commit.
        // Also in DBUTL oper no one is competing - so should be no visibility issues.
        //Call( ErrDIRBeginTransaction( reinterpret_cast<PIB*>( sesid ), XXXX, JET_bitTransactionReadOnly ) );
        Expected( reinterpret_cast<PIB*>( sesid )->Level() == 0 );

        PGNO pgnoFDP = pgnoNull;
        CHAR szObjName[JET_cbNameMost+1];
        Call( ErrCATSeekTableByObjid(
                    reinterpret_cast<PIB*>( sesid ),
                    (IFMP)jdbid,
                    cpage.ObjidFDP(),
                    szObjName,
                    sizeof( szObjName ),
                    &pgnoFDP ) );
        Call( ErrFILEOpenTable( reinterpret_cast<PIB*>( sesid ), (IFMP)jdbid, ppfucbObj, szObjName ) );

        Assert( (*ppfucbObj)->u.pfcb->FTypeTable() );

        if ( wszObjName )
        {
            Assert( ( cbDbutlObjNameMost - JET_cbNameMost ) > ( LOSStrLengthW( L"::LV::[Space Tree]" /* worst case from below */ ) * 2 /* sizeof(WCHAR) */ ) );

            OSStrCbFormatW( wszObjName, cbObjName, L"%hs", szObjName );
            if ( (*ppfucbObj)->u.pfcb->FTypeLV() )
            {
                OSStrCbAppendW( wszObjName, cbObjName, L"::LV" );
            }
            if ( cpage.FFlags() & CPAGE::fPageSpaceTree )
            {
                OSStrCbAppendW( wszObjName, cbObjName, L"::[Space Tree]" );
            }
        }

        //CallS( ErrDIRCommitTransaction( reinterpret_cast<PIB*>( sesid ), NO_GRBIT ) );
    }
    EXCEPT( efaExecuteHandler )
    {
        //  Something has gone drastically wrong, but this is only eseutil, so just leak it (to avoid trying to close it)!
        wprintf( L"WARNING: Hit exception or AV trying to attach database or lookup object or table, continuing without detailed info/strings.\n" );
        *pifmp = ifmpNil;
        *ppfucbObj = pfucbNil;
        err = ErrERRCheck( JET_errDiskIO );
    }
    Call( err );

    *pifmp = (IFMP)jdbid;
    jdbid = JET_dbidNil;

    Assert( *pifmp != ifmpNil );
    Assert( *ppfucbObj != pfucbNil );

    return JET_errSuccess;

HandleError:

    wprintf( L"WARNING: Hit error %d trying to attach, open or lookup object or table, continuing without detailed info/strings.\n", err );
    Assert( *pifmp == ifmpNil );
    Assert( *ppfucbObj == pfucbNil );

    //CallS( ErrDIRCommitTransaction( reinterpret_cast<PIB*>( sesid ), NO_GRBIT ) );

    if ( jdbid != JET_dbidNil )
    {
        //  This shouldn't fail for a RO attach?
        CallS( ErrIsamCloseDatabase( sesid, jdbid, NO_GRBIT ) );
        CallS( ErrIsamDetachDatabase( sesid, NULL, wszDatabase ) );
    }

    return err;
}

//  ================================================================
void DBUTLCloseIfmpFucb( _Inout_ JET_SESID sesid, PCWSTR wszDatabase, _In_ IFMP ifmp, _Inout_ FUCB * pfucbObj )
//  ================================================================
{
    if ( pfucbObj != pfucbNil )
    {
        Assert( pfucbObj->u.pfcb->FTypeTable() ); // note like a FTypeLV() would be closed with like a DIRClose( pfucbTable );
        CallS( ErrFILECloseTable( reinterpret_cast<PIB*>( sesid ), pfucbObj ) );
    }
    if ( ifmp != ifmpNil )
    {
        CallS( ErrIsamCloseDatabase( sesid, ifmp, NO_GRBIT ) );
        CallS( ErrIsamDetachDatabase( sesid, NULL, wszDatabase ) );
    }
}

//  ================================================================
LOCAL VOID DBUTLDumpNode_( const KEYDATAFLAGS& kdf, CHAR* szBuf, const INT cbBuf, const INT cbWidth )
//  ================================================================
{
    printf( "     Flags:  0x%4.4x\n", kdf.fFlags );
    printf( "===========\n" );
    if ( FNDVersion( kdf ) )
    {
        printf( "            Versioned\n" );
    }
    if ( FNDDeleted( kdf ) )
    {
        printf( "            Deleted\n" );
    }
    if ( FNDCompressed( kdf ) )
    {
        printf( "            Compressed\n" );
    }
    printf( "\n" );

    printf( "Key Prefix:  %4d bytes\n", kdf.key.prefix.Cb() );
    printf( "===========\n" );
    szBuf[ 0 ] = 0;
    DBUTLSprintHex( szBuf, cbBuf, reinterpret_cast<BYTE*>( kdf.key.prefix.Pv() ), kdf.key.prefix.Cb(), cbWidth );
    printf( "%s\n", szBuf );
    printf( "Key Suffix:  %4d bytes\n", kdf.key.suffix.Cb() );
    printf( "===========\n" );
    szBuf[ 0 ] = 0;
    DBUTLSprintHex( szBuf, cbBuf, reinterpret_cast<BYTE*>( kdf.key.suffix.Pv() ), kdf.key.suffix.Cb(), cbWidth );
    printf( "%s\n", szBuf );
    printf( "      Data:  %4d bytes\n", kdf.data.Cb() );
    printf( "===========\n" );
    szBuf[ 0 ] = 0;
    DBUTLSprintHex( szBuf, cbBuf, reinterpret_cast<BYTE*>( kdf.data.Pv() ), kdf.data.Cb(), cbWidth );
    printf( "%s\n", szBuf );
}

//  ================================================================
LOCAL ERR ErrDBUTLDumpNode( JET_SESID sesid, IFileSystemAPI *const pfsapi, const WCHAR * const wszFile, const PGNO pgno, const INT iline, const  JET_GRBIT grbit )
//  ================================================================
{
    ERR             err         = JET_errSuccess;
    KEYDATAFLAGS    kdf;
    CPAGE           cpage;
    IFileAPI*       pfapi       = NULL;
    QWORD           ibOffset    = OffsetOfPgno( pgno );
    VOID*           pvPage      = NULL;
    CHAR*           szBuf       = NULL;
    const INT       cbWidth     = UtilCprintfStdoutWidth() >= 116 ? 32 : 16;
    INT             ilineCurrent;
    TraceContextScope   tcUtil  ( iorpDirectAccessUtil );
    IFMP            ifmp        = ifmpNil;
    FUCB *          pfucbTable  = pfucbNil;

    pvPage = PvOSMemoryPageAlloc( g_cbPage, NULL );
    if( NULL == pvPage )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    err = CIOFilePerf::ErrFileOpen( pfsapi,
        reinterpret_cast<PIB*>( sesid )->m_pinst,
        wszFile,
        IFileAPI::fmfReadOnly,
        iofileDbAttached,
        qwDumpingFileID,
        &pfapi );
    if ( err < 0 )
    {
        wprintf( L"Cannot open file %ws.\n\n", wszFile );
        Call( err );
    }
    Call( pfapi->ErrIORead( *tcUtil, ibOffset, g_cbPage, (BYTE* const)pvPage, qosIONormal ) );

    cpage.LoadPage( ifmpNil, pgno, pvPage, g_cbPage);

    if ( iline < -1 || iline >= cpage.Clines() )
    {
        printf( "Invalid iline: %d\n\n", iline );
        //  errNDNotFound would be a MUCH better error, but we have a general policy against returning
        //  internal errors out the JET API.  Perhaps we should relax that for utility \ dumping.
        Call( ErrERRCheck( cpage.FPageIsInitialized() ? JET_errInvalidParameter : JET_errPageNotInitialized ) );
    }

    if ( !cpage.FNewRecordFormat()
        && cpage.FPrimaryPage()
        && !cpage.FRepairedPage()
        && cpage.FLeafPage()
        && !cpage.FSpaceTree()
        && !cpage.FLongValuePage() )
    {
        if ( iline == -1 )
        {
            printf( "Cannot dump all nodes on old format page\n\n" );
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }

        VOID *  pvBuf   = PvOSMemoryPageAlloc( g_cbPage, NULL );

        if( NULL == pvBuf )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        err = ErrUPGRADEConvertNode( &cpage, iline, pvBuf );
        OSMemoryPageFree( pvBuf );
        Call( err );
    }

    szBuf = (CHAR *)PvOSMemoryPageAlloc( g_cbPage * 8, NULL );
    if( NULL == szBuf )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    ilineCurrent = ( iline == -1 ) ? 0 : iline;

    do
    {
        if ( ilineCurrent >= cpage.Clines() )
        {
            Assert( ilineCurrent == 0 );
            printf( "ERROR:  This page doesn't even have one valid iline.  Q: Blank?  A: %hs\n", cpage.FPageIsInitialized() ? "No" : "Yes" );
            Call( ErrERRCheck( cpage.FPageIsInitialized() ? JET_errInvalidParameter : JET_errPageNotInitialized ) );
        }

        NDIGetKeydataflags( cpage, ilineCurrent, &kdf );

        printf( "          Node: %d:%d\n\n", pgno, ilineCurrent );
        DBUTLDumpNode_( kdf, szBuf, g_cbPage * 8, cbWidth );
        printf( "\n\n" );

        if( !cpage.FLeafPage() )
        {
            if( sizeof( PGNO ) == kdf.data.Cb() )
            {
                const PGNO pgnoChild = *(reinterpret_cast<UnalignedLittleEndian< PGNO > *>( kdf.data.Pv() ) );
                printf( "pgnoChild = %lu (0x%x)\n", pgnoChild, pgnoChild );
            }
        }
        else if( cpage.FSpaceTree() )
        {
            PGNO pgnoLast;
            CPG cpg;
            SpacePool sppPool;

            Call( ErrSPIGetExtentInfo( &kdf, &pgnoLast, &cpg, &sppPool ) );
            // PGNO is usigned long. 
            printf( "%d (0x%x) pages ending at %lu (0x%x) in pool %lu (%ws)\n", cpg, cpg, pgnoLast, pgnoLast, (ULONG)sppPool, WszPoolName( sppPool ) );
        }
        else if( cpage.FPrimaryPage() )
        {
            if( cpage.FLongValuePage() )
            {
                //  UNDONE: dump this
            }
            else
            {
                if ( ifmp == ifmpNil && pfucbTable == pfucbNil )
                {
                    tcUtil->iorReason.SetIorp( iorpNone );
                    OnDebug( ERR errT = )ErrDBUTLGetIfmpFucbOfPage( sesid, wszFile, cpage, &ifmp, &pfucbTable );
                    tcUtil->iorReason.SetIorp( iorpDirectAccessUtil );
                    //  Both valid or both Nil depending on err value
                    Assert( errT < JET_errSuccess || ifmp != ifmpNil );
                    Assert( errT < JET_errSuccess || pfucbTable != pfucbNil );
                    Assert( errT >= JET_errSuccess || ifmp == ifmpNil );
                    Assert( errT >= JET_errSuccess || pfucbTable == pfucbNil );
                }
                DBUTLDumpRec( cpage.CbPage(), pfucbTable, kdf.data.Pv(), kdf.data.Cb(), CPRINTFSTDOUT::PcprintfInstance(), cbWidth );
            }
        }

        ilineCurrent++;
    }
    while ( iline == -1 && ilineCurrent < cpage.Clines() );
    
HandleError:
    DBUTLCloseIfmpFucb( sesid, wszFile, ifmp, pfucbTable );

    if ( cpage.FLoadedPage() )
    {
        cpage.UnloadPage();
    }
    OSMemoryPageFree( szBuf );
    OSMemoryPageFree( pvPage );
    delete pfapi;
    return err;
}

//  ================================================================
LOCAL ERR ErrDBUTLDumpTag( JET_SESID sesid, IFileSystemAPI* const pfsapi, const WCHAR* const wszFile, const PGNO pgno, const INT itag, const  JET_GRBIT grbit )
//  ================================================================
{
    ERR             err = JET_errSuccess;
    KEYDATAFLAGS    kdf;
    CPAGE           cpage;
    IFileAPI* pfapi = NULL;
    QWORD           ibOffset = OffsetOfPgno( pgno );
    VOID* pvPage = NULL;
    CHAR* szBuf = NULL;
    const INT       cbWidth = UtilCprintfStdoutWidth() >= 116 ? 32 : 16;
    INT             itagCurr;
    TraceContextScope   tcUtil( iorpDirectAccessUtil );

    pvPage = PvOSMemoryPageAlloc( g_cbPage, NULL );
    if ( NULL == pvPage )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    err = CIOFilePerf::ErrFileOpen( pfsapi,
        reinterpret_cast<PIB*>( sesid )->m_pinst,
        wszFile,
        IFileAPI::fmfReadOnly,
        iofileDbAttached,
        qwDumpingFileID,
        &pfapi );
    if ( err < 0 )
    {
        wprintf( L"Cannot open file %ws.\n\n", wszFile );
        Call( err );
    }
    Call( pfapi->ErrIORead( *tcUtil, ibOffset, g_cbPage, (BYTE* const) pvPage, qosIONormal ) );

    cpage.LoadPage( ifmpNil, pgno, pvPage, g_cbPage );

    const INT ctags = cpage.Clines() + cpage.CTagReserved();
    if ( itag < -1 || itag >= ctags )
    {
        printf( "Invalid itag: %d\n\n", itag );
        //  errNDNotFound would be a MUCH better error, but we have a general policy against returning
        //  internal errors out the JET API.  Perhaps we should relax that for utility \ dumping.
        Call( ErrERRCheck( cpage.FPageIsInitialized() ? JET_errInvalidParameter : JET_errPageNotInitialized ) );
    }

    if ( !cpage.FNewRecordFormat()
        && cpage.FPrimaryPage()
        && !cpage.FRepairedPage()
        && cpage.FLeafPage()
        && !cpage.FSpaceTree()
        && !cpage.FLongValuePage() )
    {
        if ( itag == -1 )
        {
            printf( "Cannot dump all tags on old format page\n\n" );
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }

        VOID* pvBuf = PvOSMemoryPageAlloc( g_cbPage, NULL );

        if ( NULL == pvBuf )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        err = ErrUPGRADEConvertNode( &cpage, itag, pvBuf );
        OSMemoryPageFree( pvBuf );
        Call( err );
    }

    szBuf = (CHAR*) PvOSMemoryPageAlloc( g_cbPage * 8, NULL );
    if ( NULL == szBuf )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    itagCurr = ( itag == -1 ) ? 0 : itag;

    do
    {
        if ( itagCurr >= ctags )
        {
            Assert( itagCurr == 0 );
            printf( "ERROR:  This page doesn't even have one valid tag.  Q: Blank?  A: %hs\n", cpage.FPageIsInitialized() ? "No" : "Yes" );
            Call( ErrERRCheck( cpage.FPageIsInitialized() ? JET_errInvalidParameter : JET_errPageNotInitialized ) );
        }

        printf( "          Tag: %d:%d\n", pgno, itagCurr );
        printf( "         Type: %s\n\n",
                    itagCurr == 0 ? "External Header" :
                    itagCurr < cpage.CTagReserved() ? "Reserved Tag" :
                    "Node" );

        if ( itagCurr < cpage.CTagReserved() )
        {
            LINE line;
            cpage.GetPtrReservedTag( itagCurr, &line );
            if ( itagCurr > 0 )
            {
                NodeResvTag* pResvTag = (NodeResvTag*) line.pv;
                printf( " ResvTagId:  %d\n", pResvTag->resvTagId );
            }

            printf( "      Data:  %4d bytes\n", line.cb );
            printf( "===========\n" );
            szBuf[ 0 ] = 0;
            DBUTLSprintHex( szBuf, g_cbPage * 8, reinterpret_cast<BYTE*>( line.pv ), line.cb, cbWidth );
            printf( "%s\n", szBuf );
        }
        else
        {
            NDIGetKeydataflags( cpage, itagCurr - cpage.CTagReserved(), &kdf);
            DBUTLDumpNode_( kdf, szBuf, g_cbPage * 8, cbWidth );
        }

        printf( "\n\n" );
        itagCurr++;
    } while ( itag == -1 && itagCurr < ctags );

HandleError:
    if ( cpage.FLoadedPage() )
    {
        cpage.UnloadPage();
    }
    OSMemoryPageFree( szBuf );
    OSMemoryPageFree( pvPage );
    delete pfapi;
    return err;
}


LOCAL BOOL FDBUTLConvertHexDigit_(
    const CHAR      cHexDigit,
    __out_bcount_full(1) CHAR * const   pcHexValue )
{
    if ( cHexDigit >= '0' && cHexDigit <= '9' )
    {
        *pcHexValue = cHexDigit - '0';
        return fTrue;
    }
    else if ( cHexDigit >= 'A' && cHexDigit <= 'F' )
    {
        *pcHexValue = cHexDigit - 'A' + 0xa;
        return fTrue;
    }
    else if ( cHexDigit >= 'a' && cHexDigit <= 'f' )
    {
        *pcHexValue = cHexDigit - 'a' + 0xa;
        return fTrue;
    }
    else
    {
        return fFalse;
    }
}

LOCAL BOOL FDBUTLPrintedKeyToRealKey(
    __in_bcount(2 * cbKey) const CHAR * const   szPrintedKey,
    __out_bcount_full(cbKey) CHAR * const       szKey,
    const ULONG         cbKey )
{
    for ( ULONG i = 0; i < cbKey; i++ )
    {
        const ULONG     j            = i * 2;
        CHAR            cHighNibble;
        CHAR            cLowNibble;

        if ( 0 == i )
        {
            printf( "%c%c", szPrintedKey[j], szPrintedKey[j+1] );
        }
        else
        {
            printf( " %c%c", szPrintedKey[j], szPrintedKey[j+1] );
        }

        if ( !FDBUTLConvertHexDigit_( szPrintedKey[j], &cHighNibble )
            || !FDBUTLConvertHexDigit_( szPrintedKey[j+1], &cLowNibble ) )
        {
            return fFalse;
        }

        szKey[i] = ( cHighNibble << 4 ) + cLowNibble;
    }

    return fTrue;
}

LOCAL ERR ErrDBUTLSeekToKey_(
    IFileAPI *          pfapi,
    PGNO                pgnoRoot,
    CPAGE&              cpage,
    VOID * const        pvPageBuf,
    const CHAR * const  szPrintedKey,
    const CHAR * const  szPrintedData )
{
    ERR                 err;
    CHAR                szKey[cbKeyAlloc];
    CHAR                szData[cbKeyAlloc];
    const ULONG         cbPrintedKey        = (ULONG)strlen( szPrintedKey );
    const ULONG         cbPrintedData       = ( NULL != szPrintedData ? (ULONG)strlen( szPrintedData ) : 0 );
    BOOKMARK            bm;
    INT                 compare;
    INT                 iline;

    PGNO                pgnoCurr = pgnoRoot;

    //  validate key length
    //
    if ( cbPrintedKey % 2 != 0
        || cbPrintedKey > ( cbKeyAlloc * 2 )
        || cbPrintedData % 2 != 0
        || cbPrintedData > ( cbKeyAlloc * 2 ) )
    {
        return ErrERRCheck( JET_errInvalidBookmark );
    }

    printf( " Seek bookmark: \"" );

    bm.Nullify();
    bm.key.suffix.SetPv( szKey );
    bm.key.suffix.SetCb( cbPrintedKey / 2 );

    //  only use data portion of bookmark if we're on a btree
    //  with non-unique keys
    //
    if ( cpage.FNonUniqueKeys() && 0 != cbPrintedData )
    {
        bm.data.SetPv( szData );
        bm.data.SetCb( cbPrintedData / 2 );
    }

    const INT cbSuffix = bm.key.suffix.Cb();

    AssertPREFIX( sizeof( szKey ) >= cbSuffix );

    //  first, convert the printed key to a real key
    //
    if ( !FDBUTLPrintedKeyToRealKey(
                szPrintedKey,
                szKey,
                cbSuffix ) )
    {
        printf( "...\"\n\n" );
        return ErrERRCheck( JET_errInvalidBookmark );
    }

    //  next, convert the printed data (if any) to a real data
    //
    const INT cbData = bm.data.Cb();
    
    if ( cbData > 0 )
    {
        //  print a key/data separator
        //
        printf( " | " );

        AssertPREFIX( sizeof( szData ) >= cbData );

        if ( !FDBUTLPrintedKeyToRealKey(
                    szPrintedData,
                    szData,
                    cbData ) )
        {
            printf( "...\"\n\n" );
            return ErrERRCheck( JET_errInvalidBookmark );
        }
    }

    printf( "\"\n" );

    //  now seek down the btree for the bookmark
    //
    while ( !cpage.FLeafPage() )
    {
        iline = IlineNDISeekGEQInternal( cpage, bm, &compare );

        if ( 0 == compare )
        {
            //  see ErrNDISeekInternalPage() for an
            //  explanation of why we increment by 1 here
            //
            iline++;
        }

        printf( "    pgno/iline: %lu-%d  (", pgnoCurr, iline );
        if ( cpage.FRootPage() )
            printf( "root," );
        if ( cpage.FParentOfLeaf() )
            printf( "parent-of-leaf" );
        else
            printf( "internal" );
        printf( ")\n" );

        KEYDATAFLAGS    kdf;
        NDIGetKeydataflags( cpage, iline, &kdf );

        if ( sizeof(PGNO) != kdf.data.Cb() )
        {
            printf( "\n" );
            return ErrERRCheck( JET_errBadPageLink );
        }

        const PGNO  pgnoChild   = *(UnalignedLittleEndian< PGNO > *)kdf.data.Pv();

        cpage.UnloadPage();

        err = pfapi->ErrIORead( *TraceContextScope( iorpDirectAccessUtil ), OffsetOfPgno( pgnoChild ), g_cbPage, (BYTE* const)pvPageBuf, qosIONormal );
        if ( err < 0 )
        {
            printf( "\n" );
            return err;
        }

        cpage.LoadPage( ifmpNil, pgnoChild, pvPageBuf, g_cbPage );

        pgnoCurr = pgnoChild;
    }

    //  find the first node in the leaf page that's
    //  greater than or equal to the specified bookmark
    //
    iline = IlineNDISeekGEQ( cpage, bm, !cpage.FNonUniqueKeys(), &compare );
    Assert( iline < cpage.Clines( ) );
    if ( iline < 0 )
    {
        //  all nodes in page are less than key
        //
        iline = cpage.Clines( ) - 1;
    }

    printf( "    pgno/iline: %lu-%d  (%sleaf)\n\n", pgnoCurr, iline, ( cpage.FRootPage() ? "root," : "" ) );

    return JET_errSuccess;
}

//  ================================================================
LOCAL ERR ErrDBUTLDumpPage(
    INST * const            pinst,
    const WCHAR *           wszFile,
    const PGNO              pgno,
    const WCHAR * const     wszPrintedKeyToSeek,
    const WCHAR * const     wszPrintedDataToSeek,
    const JET_GRBIT         grbit )
//  ================================================================
{
    ERR                     err         = JET_errSuccess;
    IFileSystemAPI * const  pfsapi      = pinst->m_pfsapi;
    IFileAPI *              pfapi       = NULL;
    CPAGE                   cpage;
    TraceContextScope       tcUtil      ( iorpDirectAccessUtil );

    VOID * const pvPage = PvOSMemoryPageAlloc( g_cbPage, NULL );
    if( NULL == pvPage )
    {
        CallR( ErrERRCheck( JET_errOutOfMemory ) );
    }

    err = CIOFilePerf::ErrFileOpen( pfsapi,
                                    pinst,
                                    wszFile,
                                    IFileAPI::fmfReadOnly,
                                    iofileDbAttached, 
                                    qwDumpingFileID, 
                                    &pfapi );
    if ( err < 0 )
    {
        printf( "Cannot open file %ws.\n\n", wszFile );
        Call( err );
    }
    Call( pfapi->ErrIORead( *tcUtil, OffsetOfPgno( pgno ), g_cbPage, (BYTE* const)pvPage, qosIONormal ) );

    cpage.LoadPage( ifmpNil, pgno, pvPage, g_cbPage );


    //  page integrity check.
    //
    if (  cpage.Clines() == -1 )
    {
        //  special pages with all 0 and 0 TAG. 
        //
        AssertSz( cpage.ObjidFDP() == 0 || cpage.FPreInitPage(), "Odd page (%d):0 TAG page should be all 0s\r\n", cpage.PgnoThis() );
    }
    else
    {
        Call( cpage.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance() ) );
    }
    
    if ( NULL != wszPrintedKeyToSeek && 0 != cpage.Clines() )
    {
        CAutoSZ szPrintedKeyToSeek;
        CAutoSZ szPrintedDataToSeek;

        Call( szPrintedKeyToSeek.ErrSet( wszPrintedKeyToSeek ) );
        Call( szPrintedDataToSeek.ErrSet( wszPrintedDataToSeek ) );
        
        Call( ErrDBUTLSeekToKey_( pfapi, pgno, cpage, pvPage, (CHAR*)szPrintedKeyToSeek, (CHAR*)szPrintedDataToSeek ) );
    }

#ifdef DEBUGGER_EXTENSION
    (VOID)cpage.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );
    printf( "\n" );
    (VOID)cpage.DumpTags( CPRINTFSTDOUT::PcprintfInstance() );
    printf( "\n" );
#endif

    if( grbit & JET_bitDBUtilOptionDumpVerbose )
    {
        (VOID)cpage.DumpAllocMap( CPRINTFSTDOUT::PcprintfInstance() );
        printf( "\n" );

        const INT cbWidth = UtilCprintfStdoutWidth() >= 116 ? 32 : 16;

        CHAR * szBuf = new CHAR[g_cbPage * 8];
        if( NULL == szBuf )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        printf( "Raw dump (cbPage = %d / cbBuffer = %d) in big-endian quartets of bytes (not little endian DWORDs):\n", g_cbPage, g_cbPage );
        DBUTLSprintHex( szBuf, g_cbPage * 8, reinterpret_cast<BYTE *>( pvPage ), g_cbPage, cbWidth );
        printf( "%s\n", szBuf );

        delete [] szBuf;
    }
    
HandleError:
    if ( cpage.FLoadedPage() )
    {
        cpage.UnloadPage();
    }
    delete pfapi;
    OSMemoryPageFree( pvPage );
    return err;
}

//  Helper code to keep track of where we are in a run, and push each run into
//  the stats structure.
class CDBUTLIRunCalculator {

    //  Where to accumulate run stats ....  
    CStats *        m_pStats;
    ULONG *         m_pcForwardScans;

    //  For keeping the state of the current run ...
    ULONG           m_cpgCurrRun;
    PGNO            m_pgnoPrevious;

public:

    //
    //  Order of calls:
    //      CDBUTLIRunCalculator( pStat )
    //      ErrProcessPage( pgno ) x N
    //      [ErrBreak() x M]
    //      ErrProcessPage( CDBUTLIRunCalculator::pgnoDoneSentinel )
    //          Note: As of here pStat is now populated with all the runs.
    //      ~CDBUTLIRunCalculator()
    //

    CDBUTLIRunCalculator( CStats * pStat, ULONG * pcHaltedForwardProgress ) :
        m_pStats( pStat ),
        m_pcForwardScans( pcHaltedForwardProgress )
    {
        m_cpgCurrRun = 0;
        m_pgnoPrevious = 0x0;
        Assert( m_pStats );
        Assert( m_pcForwardScans );
        Assert( *m_pcForwardScans == 0 );
    }

    ~CDBUTLIRunCalculator( )
    {
        //  These asserts means someone forgot to process the pgnoDoneSentinel, and the last run was not added to m_pStats.
        Assert( m_pgnoPrevious == pgnoDoneSentinel );
        Assert( m_cpgCurrRun ==  0 );
        m_pStats = NULL;
        m_pcForwardScans = NULL;
    }

    ERR ErrBreak( void )
    {
        ERR err = JET_errSuccess;

        //  We have a run, add the sample to the stats structure.
        if ( m_cpgCurrRun != 0 )
        {
            if ( m_pStats->ErrAddSample( m_cpgCurrRun ) == CStats::ERR::errOutOfMemory )
            {
                return ErrERRCheck( JET_errOutOfMemory );
            }
            m_cpgCurrRun = 0;
        }

        return err;
    }

    static const PGNO   pgnoDoneSentinel = 0xFFFFFFFF;

    ERR ErrProcessPage( PGNO pgno )
    {
        ERR err = JET_errSuccess;

        Assert( pgno != 0x0 );
        if ( m_pgnoPrevious == pgnoDoneSentinel )
        {
            Assert( m_pgnoPrevious != pgnoDoneSentinel );   // shouldn't happen
            return JET_errSuccess;  // but handle it anyway.
        }

        if ( pgnoDoneSentinel == pgno )
        {
            Call( ErrBreak() ); //  flush the last run.

            //  Account for the last run if the tree isn't completely empty.
            if ( m_pStats->C() )
            {
                (*m_pcForwardScans)++;
            }
        }
        else
        {

            //  If the current pgno is less than the previous, we have a halt in forward progress
            //  through the DB, so count a forward scan.  This depends upon m_pgnoPrevious being
            //  zero initially.
            if ( pgno < m_pgnoPrevious )
            {
                (*m_pcForwardScans)++;
            }

            if ( m_pgnoPrevious != 0x0 )
            {
                if ( m_pgnoPrevious + 1 != pgno )
                {
                    Call( ErrBreak() );
                }
            }

            m_cpgCurrRun++;
        }

        m_pgnoPrevious = pgno;
        return JET_errSuccess;

    HandleError:
        Assert( err == JET_errOutOfMemory );
        return err;
    }
    
};  // CDBUTLIRunCalculator


//  A helper class to manage the stack allocation, and initialization
//  of these complicated inter related structures for callback.
class CBTreeStatsManager {

    private:
        BTREE_STATS_BASIC_CATALOG   m_btsBasicCatalog;
        BTREE_STATS_SPACE_TREES     m_btsSpaceTrees;
        BTREE_STATS_PARENT_OF_LEAF  m_btsParentOfLeaf;
        BTREE_STATS_PAGE_SPACE      m_btsInternalPageSpace;
        BTREE_STATS_PAGE_SPACE      m_btsFullWalk;
        BTREE_STATS_LV              m_btsLvData;
        CPerfectHistogramStats      m_rgHistos[22];
        BTREE_STATS                 m_bts;

    public:
        CBTreeStatsManager( JET_GRBIT grbit, BTREE_STATS * pbtsParent )
        {

            //  Zero and initialize the cbStruct versions.
            memset( &m_btsBasicCatalog, 0, sizeof(m_btsBasicCatalog) );
            m_btsBasicCatalog.cbStruct = sizeof(m_btsBasicCatalog);
            memset( &m_btsSpaceTrees, 0, sizeof(m_btsSpaceTrees) );
            m_btsSpaceTrees.cbStruct = sizeof(m_btsSpaceTrees);
            memset( &m_btsParentOfLeaf, 0, sizeof(m_btsParentOfLeaf) );
            m_btsParentOfLeaf.cbStruct = sizeof(m_btsParentOfLeaf);
            memset( &m_btsInternalPageSpace, 0, sizeof(m_btsInternalPageSpace) );
            m_btsInternalPageSpace.cbStruct = sizeof(m_btsInternalPageSpace);
            memset( &m_btsFullWalk, 0, sizeof(m_btsFullWalk) );
            m_btsFullWalk.cbStruct = sizeof(m_btsFullWalk);
            memset( &m_btsLvData, 0, sizeof(m_btsLvData) );
            m_btsLvData.cbStruct = sizeof(m_btsLvData);

            //  Setup histograms.
            m_btsParentOfLeaf.phistoIOContiguousRuns    = (JET_HISTO*)&m_rgHistos[0];

            m_btsFullWalk.phistoFreeBytes       = (JET_HISTO*)&m_rgHistos[1];
            m_btsFullWalk.phistoNodeCounts      = (JET_HISTO*)&m_rgHistos[2];
            m_btsFullWalk.phistoKeySizes        = (JET_HISTO*)&m_rgHistos[3];
            m_btsFullWalk.phistoDataSizes       = (JET_HISTO*)&m_rgHistos[4];
            m_btsFullWalk.phistoKeyCompression  = (JET_HISTO*)&m_rgHistos[5];
            m_btsFullWalk.phistoResvTagSizes    = (JET_HISTO*)&m_rgHistos[6];
            m_btsFullWalk.phistoUnreclaimedBytes    = (JET_HISTO*)&m_rgHistos[7];
            m_btsFullWalk.cVersionedNodes       = 0;
            
            m_btsInternalPageSpace.phistoFreeBytes      = (JET_HISTO*)&m_rgHistos[8];
            m_btsInternalPageSpace.phistoNodeCounts     = (JET_HISTO*)&m_rgHistos[9];
            m_btsInternalPageSpace.phistoKeySizes       = (JET_HISTO*)&m_rgHistos[10];
            m_btsInternalPageSpace.phistoDataSizes      = (JET_HISTO*)&m_rgHistos[11];
            m_btsInternalPageSpace.phistoKeyCompression = (JET_HISTO*)&m_rgHistos[12];
            m_btsInternalPageSpace.phistoResvTagSizes   = (JET_HISTO*)&m_rgHistos[13];
            m_btsInternalPageSpace.phistoUnreclaimedBytes   = (JET_HISTO*)&m_rgHistos[14];

            m_btsLvData.phistoLVSize            = (JET_HISTO*)&m_rgHistos[15];
            m_btsLvData.phistoLVComp            = (JET_HISTO*)&m_rgHistos[16];
            m_btsLvData.phistoLVRatio           = (JET_HISTO*)&m_rgHistos[17];
            m_btsLvData.phistoLVSeeks           = (JET_HISTO*)&m_rgHistos[18];
            m_btsLvData.phistoLVBytes           = (JET_HISTO*)&m_rgHistos[19];
            m_btsLvData.phistoLVExtraSeeks      = (JET_HISTO*)&m_rgHistos[20];
            m_btsLvData.phistoLVExtraBytes      = (JET_HISTO*)&m_rgHistos[21];

            //  Zero the parent structure of all this.
            memset( &m_bts, 0, sizeof(m_bts) );
            m_bts.cbStruct = sizeof(m_bts);

            m_bts.grbitData = grbit;
            m_bts.pParent = pbtsParent;
            
            m_bts.pBasicCatalog = ( m_bts.grbitData & JET_bitDBUtilSpaceInfoBasicCatalog ) ?
                                    &m_btsBasicCatalog : NULL;
            m_bts.pSpaceTrees   = ( m_bts.grbitData & JET_bitDBUtilSpaceInfoSpaceTrees ) ?
                                    &m_btsSpaceTrees : NULL;
            m_bts.pParentOfLeaf = ( m_bts.grbitData & JET_bitDBUtilSpaceInfoParentOfLeaf ) ?
                                    &m_btsParentOfLeaf : NULL;
            if ( m_bts.pParentOfLeaf )
            {
                m_bts.pParentOfLeaf->pInternalPageStats = &m_btsInternalPageSpace;
            }
            m_bts.pFullWalk     = ( m_bts.grbitData & JET_bitDBUtilSpaceInfoFullWalk ) ?
                                    &m_btsFullWalk : NULL;
            m_bts.pLvData       = ( m_bts.grbitData & JET_bitDBUtilSpaceInfoBasicCatalog &&
                                    m_bts.grbitData & JET_bitDBUtilSpaceInfoFullWalk ) ?
                                    &m_btsLvData : NULL;
        }
        
        static void ResetParentOfLeaf( BTREE_STATS_PARENT_OF_LEAF * pPOL )
        {
            Assert( pPOL );
            Assert( pPOL->cbStruct == sizeof(*pPOL) );
            Assert( pPOL->phistoIOContiguousRuns );
            pPOL->fEmpty = fFalse;
            pPOL->cpgInternal = 0;
            pPOL->cpgData = 0;
            pPOL->cDepth = 0;

            Assert( pPOL->phistoIOContiguousRuns );
            CStatsFromPv(pPOL->phistoIOContiguousRuns)->Zero();
            pPOL->cForwardScans = 0;

            Assert( pPOL->pInternalPageStats );
            Assert( pPOL->pInternalPageStats->phistoFreeBytes );
            CStatsFromPv(pPOL->pInternalPageStats->phistoFreeBytes)->Zero();
            CStatsFromPv(pPOL->pInternalPageStats->phistoNodeCounts)->Zero();
            CStatsFromPv(pPOL->pInternalPageStats->phistoKeySizes)->Zero();
            CStatsFromPv(pPOL->pInternalPageStats->phistoDataSizes)->Zero();
            CStatsFromPv(pPOL->pInternalPageStats->phistoKeyCompression)->Zero();
            CStatsFromPv(pPOL->pInternalPageStats->phistoResvTagSizes)->Zero();
            CStatsFromPv(pPOL->pInternalPageStats->phistoUnreclaimedBytes)->Zero();
        }

        static void ResetFullWalk( BTREE_STATS_PAGE_SPACE * pFW )
        {
            Assert( pFW );
            Assert( pFW->cbStruct == sizeof(*pFW) );

            Assert( pFW->phistoFreeBytes );
            CStatsFromPv(pFW->phistoFreeBytes)->Zero();

            Assert( pFW->phistoNodeCounts );
            CStatsFromPv(pFW->phistoNodeCounts)->Zero();

            Assert( pFW->phistoKeySizes );
            CStatsFromPv(pFW->phistoKeySizes)->Zero();

            Assert( pFW->phistoDataSizes );
            CStatsFromPv(pFW->phistoDataSizes)->Zero();

            Assert( pFW->phistoKeyCompression );
            CStatsFromPv(pFW->phistoKeyCompression)->Zero();

            Assert( pFW->phistoResvTagSizes );
            CStatsFromPv(pFW->phistoResvTagSizes)->Zero();

            Assert( pFW->phistoUnreclaimedBytes );
            CStatsFromPv(pFW->phistoUnreclaimedBytes)->Zero();

            pFW->cVersionedNodes = 0;
        }

        static void ResetSpaceTreeInfo( BTREE_STATS_SPACE_TREES * pbtsST )
        {
            Assert( pbtsST );
            if ( pbtsST->prgOwnedExtents )
            {
                delete [] pbtsST->prgOwnedExtents;
                pbtsST->prgOwnedExtents = NULL;
            }
            if ( pbtsST->prgAvailExtents )
            {
                delete [] pbtsST->prgAvailExtents;
                pbtsST->prgAvailExtents = NULL;
            }
            Assert( pbtsST->prgOwnedExtents == NULL );  // don't leak
            Assert( pbtsST->prgAvailExtents == NULL );  // don't leak
            memset( pbtsST, 0, sizeof(*pbtsST) );
            pbtsST->cbStruct = sizeof(*pbtsST);
        }

        static void ResetLvData( BTREE_STATS_LV * pLvData )
        {
            if ( pLvData )
            {
                Assert( pLvData->cbStruct == sizeof(*pLvData) );

                pLvData->cLVRefs = 0;
                pLvData->cCorruptLVs = 0;
                pLvData->cSeparatedRootChunks = 0;
                pLvData->cPartiallyDeletedLVs = 0;
                pLvData->lidMax = 0;
                CStatsFromPv( pLvData->phistoLVSize )->Zero();
                CStatsFromPv( pLvData->phistoLVComp )->Zero();
                CStatsFromPv( pLvData->phistoLVRatio )->Zero();
                CStatsFromPv( pLvData->phistoLVSeeks )->Zero();
                CStatsFromPv( pLvData->phistoLVBytes )->Zero();
                CStatsFromPv( pLvData->phistoLVExtraSeeks )->Zero();
                CStatsFromPv( pLvData->phistoLVExtraBytes )->Zero();
            }
        }

        ~CBTreeStatsManager ()
        {
            // Do we want to validate anything?
            //  Note the most important part is we reset all parts, and also that the histos 
            //  destructor is called so we free allocated memory.
            if ( m_bts.pParentOfLeaf )
            {
                ResetParentOfLeaf( m_bts.pParentOfLeaf );
            }
            if ( m_bts.pFullWalk )
            {
                ResetFullWalk( m_bts.pFullWalk );
            }
            if ( m_bts.pSpaceTrees )
            {
                ResetSpaceTreeInfo( m_bts.pSpaceTrees );
            }
            if ( m_bts.pLvData )
            {
                ResetLvData( m_bts.pLvData );
            }
        }

        BTREE_STATS * Pbts( void )
        {
            return &m_bts;
        }
};


LOCAL ERR ErrEnumDataNodes(
    PIB *           ppib,
    const IFMP      ifmp,
    const PGNO      pgnoFDP,
    // fVistFlags not relevant, we can only visit the leaf
    //  const ULONG             fVisitFlags,
    PFNVISITPAGE            pfnErrVisitPage,
    void *                  pvVisitPageCtx,
    CPAGE::PFNVISITNODE     pfnErrVisitNode,
    void *                  pvVisitNodeCtx
    )
{
    ERR             err;
    FUCB            *pfucb          = pfucbNil;
    BOOL            fForceInit      = fFalse;
    DIB         dib;

    PGNO        pgnoLastSeen    = pgnoNull;
    CPG         cpgSeen         = 0;

    CallR( ErrBTOpen( ppib, pgnoFDP, ifmp, &pfucb ) );
    Assert( pfucbNil != pfucb );
    Assert( pfcbNil != pfucb->u.pfcb );

    //  This is shamelessly stolen from the space enumeration/printing code
    //  that also walks a B-Tree during eseutil /ms, but its not clear if how
    //  much of what we were doing here is goodness ... I read the lifecycle
    //  of a FCB/FUCB doc, but it was still not clear.  Haha, just kidding, I
    //  would've read such a doc if it existed.
    if ( !pfucb->u.pfcb->FInitialized() )
    {
        Assert( pgnoSystemRoot != pgnoFDP );
        Assert( pgnoFDPMSO != pgnoFDP );
        Assert( pgnoFDPMSO_NameIndex != pgnoFDP );
        Assert( pgnoFDPMSO_RootObjectIndex != pgnoFDP );
        Assert( pfucb->u.pfcb->WRefCount() == 1 );

        pfucb->u.pfcb->Lock();

        //  must force FCB to initialized state to allow SPGetInfo() to
        //  open more cursors on the FCB -- this is safe because no
        //  other thread should be opening this FCB
        pfucb->u.pfcb->CreateComplete();

        pfucb->u.pfcb->Unlock();
        fForceInit = fTrue;
    }

    else if ( pgnoSystemRoot == pgnoFDP
            || pgnoFDPMSO == pgnoFDP
            || pgnoFDPMSO_NameIndex == pgnoFDP
            || pgnoFDPMSO_RootObjectIndex == pgnoFDP )
    {
        //  fine!
    }

    BTUp( pfucb );

    if ( pfucb->u.pfcb->FPrimaryIndex() ||
            pfucb->u.pfcb->FTypeLV() ||
            FFUCBSpace( pfucb ) )
    {
        //  we will be traversing the entire tree in order, preread all the pages
        FUCBSetSequential( pfucb );
        FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
    }

    // what about fDIRAllNode? it seems like we should be using it...  ?
    dib.dirflag = fDIRNull;
    dib.pos = posFirst;
    err = ErrBTDown( pfucb, &dib, latchReadNoTouch );

    if ( err != JET_errRecordNotFound )
    {
        Call( err );

        forever
        {

            if( pgnoLastSeen != Pcsr( pfucb )->Pgno() )
            {

                pgnoLastSeen = Pcsr( pfucb )->Pgno();
                ++cpgSeen;

                if ( pfnErrVisitPage )
                {
                    Call( pfnErrVisitPage( pgnoLastSeen, 0xFFFFFFFF /* unknown level */,
                                            &(Pcsr( pfucb )->Cpage()), pvVisitPageCtx ) );
                }

                
                if ( pfnErrVisitNode )
                {
                    Call( Pcsr( pfucb )->Cpage().ErrEnumTags( pfnErrVisitNode, pvVisitNodeCtx ) );
                }

            }

            err = ErrBTNext( pfucb, fDIRNull );
            if ( err < 0 )
            {
                if ( err != JET_errNoCurrentRecord )
                {
                    goto HandleError;
                }
                break;
            }
        }
    }

    err = JET_errSuccess;

HandleError:
    Assert( pfucbNil != pfucb );

    if ( fForceInit )
    {
        Assert( pfucb->u.pfcb->WRefCount() == 1 );

        pfucb->u.pfcb->Lock();

        //  force the FCB to be uninitialized so it will be purged by BTClose

        pfucb->u.pfcb->CreateCompleteErr( errFCBUnusable );

        pfucb->u.pfcb->Unlock();
    }
    BTClose( pfucb );

    return err;
}

//  Forward decl from cpage.
ERR ErrAccumulatePageStats(
    const CPAGE::PGHDR * const  ppghdr,
    INT                         itag,
    DWORD                       fNodeFlags,
    const KEYDATAFLAGS * const  pkdf,
    void *                      pvCtx
    );

//  ================================================================
LOCAL ERR ErrDBUTLGetDataPageStats(
    PIB *                       ppib,
    IFMP                        ifmp,
    const PGNO                  pgnoFDP,
    BTREE_STATS_PAGE_SPACE *    pFullWalk,
    BTREE_STATS_LV *            pLvData
    )
//  ================================================================
{
    ERR                     err         = JET_errSuccess;

    CBTreeStatsManager::ResetFullWalk( pFullWalk );
    Assert( pFullWalk->phistoFreeBytes );   // just checking one.
    CBTreeStatsManager::ResetLvData( pLvData );

    if ( fFalse )
    {
        err = ErrEnumDataNodes( ppib,
                            ifmp, pgnoFDP,
                            // fVistFlags not relevant, we're visiting the leaf here.
                            //  const ULONG             fVisitFlags,
                            NULL, NULL,
                            ErrAccumulatePageStats, pFullWalk );
    }
    else
    {
        //  Even enumerating leaf nodes with this queue based breadth
        //  first function, uses only 10 MBs of memory for a B-Tree with
        //  20 GB worth of leaf pages.

        if ( pLvData )
        {
            EVAL_LV_PAGE_CTX ctx = { 0 };
            ctx.cpgAccumMax = 0;
            ctx.rgpgnoAccum = NULL;
            ctx.pLvData = pLvData;
    
            CPAGE::PFNVISITNODE     rgpfnzErrVisitNode[3] = { ErrAccumulatePageStats, ErrAccumulateLvNodeData, NULL };
            void *                  rgpvzVisitNodeCtx[3] = { pFullWalk, &ctx, NULL };

            err = ErrBTUTLAcross( ifmp, pgnoFDP,
                                CPAGE::fPageLeaf,
                                ErrAccumulateLvPageData, &ctx,
                                rgpfnzErrVisitNode, rgpvzVisitNodeCtx );

            if ( ctx.rgpgnoAccum != NULL )
            {
                delete[] ctx.rgpgnoAccum;
                ctx.cpgAccumMax = 0;
                ctx.rgpgnoAccum = NULL;
            }
        }
        else
        {
            err = ErrBTUTLAcross( ifmp, pgnoFDP,
                                CPAGE::fPageLeaf,
                                NULL, NULL,
                                ErrAccumulatePageStats, pFullWalk );
        }
    }

    return err;
}


//  Should all this internal knowledge be in ErrSPGetInfo maybe?
LOCAL ERR ErrDBUTLGetSpaceTreeInfo(
    PIB             *ppib,
    const IFMP      ifmp,
    const OBJID     objidFDP,
    const PGNO      pgnoFDP,
    BTREE_STATS_SPACE_TREES * pbtsSpaceTree,
    CPRINTF * const pcprintf )
{
    ERR             err;
    FUCB            *pfucb          = pfucbNil;
    BOOL            fForceInit      = fFalse;
    CPG             rgcpgExtent[4];

    CallR( ErrBTOpen( ppib, pgnoFDP, ifmp, &pfucb ) );
    Assert( pfucbNil != pfucb );
    Assert( pfcbNil != pfucb->u.pfcb );

    CBTreeStatsManager::ResetSpaceTreeInfo( pbtsSpaceTree );

    if ( !pfucb->u.pfcb->FInitialized() )
    {
        Assert( pgnoSystemRoot != pgnoFDP );
        Assert( pgnoFDPMSO != pgnoFDP );
        Assert( pgnoFDPMSO_NameIndex != pgnoFDP );
        Assert( pgnoFDPMSO_RootObjectIndex != pgnoFDP );
        Assert( pfucb->u.pfcb->WRefCount() == 1 );

        pfucb->u.pfcb->Lock();

        //  must force FCB to initialized state to allow SPGetInfo() to
        //  open more cursors on the FCB -- this is safe because no
        //  other thread should be opening this FCB
        pfucb->u.pfcb->CreateComplete();

        pfucb->u.pfcb->Unlock();
        fForceInit = fTrue;
    }

    else if ( pgnoSystemRoot == pgnoFDP
            || pgnoFDPMSO == pgnoFDP
            || pgnoFDPMSO_NameIndex == pgnoFDP
            || pgnoFDPMSO_RootObjectIndex == pgnoFDP )
    {
        //  fine!
    }
    
    Call( ErrBTIGotoRoot( pfucb, latchReadNoTouch ) );

    NDGetExternalHeader ( pfucb, noderfIsamAutoInc );
    if ( pfucb->kdfCurr.data.Cb() != 0 && pfucb->kdfCurr.data.Pv() != NULL )
    {
        pbtsSpaceTree->fAutoIncPresents = fTrue;
        const QWORD qwAutoInc = *(QWORD*)pfucb->kdfCurr.data.Pv();
        pbtsSpaceTree->qwAutoInc = qwAutoInc;
    }
    else
    {
        // Both should be true: Cb is 0 and Pv is NULL
        Assert( pfucb->kdfCurr.data.Cb() == 0 );
        Assert( pfucb->kdfCurr.data.Pv() == NULL );
        pbtsSpaceTree->fAutoIncPresents = fFalse;
    }

    NDGetExternalHeader ( pfucb, noderfSpaceHeader );
    
    Assert( sizeof( SPACE_HEADER ) == pfucb->kdfCurr.data.Cb() );
    const SPACE_HEADER  * const psph = reinterpret_cast <SPACE_HEADER *> ( pfucb->kdfCurr.data.Pv() );

    if ( psph->Fv1() )
    {
        pbtsSpaceTree->cpgPrimary = psph->CpgPrimary();
        pbtsSpaceTree->cpgLastAlloc = 0;
    }
    else
    {
        Assert( psph->Fv2() );
        pbtsSpaceTree->cpgPrimary = 0;
        pbtsSpaceTree->cpgLastAlloc = psph->CpgLastAlloc();
    }
    pbtsSpaceTree->fMultiExtent = psph->FMultipleExtent();
    if ( pbtsSpaceTree->fMultiExtent )
    {
        pbtsSpaceTree->pgnoOE = psph->PgnoOE();
        pbtsSpaceTree->pgnoAE = psph->PgnoAE();
        Assert( ( pbtsSpaceTree->pgnoOE + 1 ) == pbtsSpaceTree->pgnoAE );
    }
    else
    {
        pbtsSpaceTree->pgnoOE = pgnoNull;
        pbtsSpaceTree->pgnoAE = pgnoNull;
    }

    BTUp( pfucb );

    ULONG fSPExtents = fSPOwnedExtent | fSPAvailExtent | fSPSplitBuffers;
    if ( ObjidFDP( pfucb ) == pgnoSystemRoot )
    {
        // Shelved only for dbroot.
        fSPExtents |= fSPShelvedExtent;
    }
    else
    {
        // Reserved only for non-dbroot.
        fSPExtents |= fSPReservedExtent;
    }

    Call( ErrSPGetInfo(
              ppib,
              ifmp,
              pfucb,
              (BYTE *)rgcpgExtent,
              sizeof(rgcpgExtent),
              fSPExtents,
              gci::Forbid, // Get the real values, not the cached values.
              pcprintf ) );
    
    pbtsSpaceTree->cpgOwned = rgcpgExtent[0];
    pbtsSpaceTree->cpgAvailable = rgcpgExtent[1];
    pbtsSpaceTree->cpgSpaceTreeAvailable = rgcpgExtent[2];
    if ( ObjidFDP( pfucb ) == pgnoSystemRoot )
    {
        // We read shelved, not reserved.
        pbtsSpaceTree->cpgReserved = 0;
        pbtsSpaceTree->cpgShelved = rgcpgExtent[3];
    }
    else
    {
        // We read reserved, not shelved.
        pbtsSpaceTree->cpgReserved = rgcpgExtent[3];
        pbtsSpaceTree->cpgShelved = 0;
    }

    err = ErrSPGetInfo(
        ppib,
        ifmp,
        pfucb,
        (BYTE *)rgcpgExtent,
        sizeof(rgcpgExtent),
        fSPOwnedExtent | fSPAvailExtent,
        gci::Require,
        pcprintf );

    switch ( err )
    {
        case JET_errSuccess:
            pbtsSpaceTree->cpgOwnedCache = rgcpgExtent[0];
            pbtsSpaceTree->cpgAvailableCache = rgcpgExtent[1];
            break;
            
        case JET_errObjectNotFound:
            pbtsSpaceTree->cpgOwnedCache = (ULONG)-1;
            pbtsSpaceTree->cpgAvailableCache = (ULONG)-1;
            break;
            
        default:
            // If there was an error, return it.
            Call( err );
            
            ExpectedSz( fFalse, "Unexpected warning from ErrSPGetInfo" );
            Error( ErrERRCheck( JET_errInternalError ) );
    }
    
    Call( ErrSPGetExtentInfo(
                ppib,
                ifmp,
                pfucb,
                fSPOwnedExtent,
                &( pbtsSpaceTree->cOwnedExtents ),
                &( pbtsSpaceTree->prgOwnedExtents ) ) );

    Call( ErrSPGetExtentInfo(
                ppib,
                ifmp,
                pfucb,
                fSPAvailExtent,
                &( pbtsSpaceTree->cAvailExtents ),
                &( pbtsSpaceTree->prgAvailExtents ) ) );

HandleError:
    Assert( pfucbNil != pfucb );

    if ( fForceInit )
    {
        Assert( pfucb->u.pfcb->WRefCount() == 1 );

        pfucb->u.pfcb->Lock();

        //  force the FCB to be uninitialized so it will be purged by BTClose

        pfucb->u.pfcb->CreateCompleteErr( errFCBUnusable );

        pfucb->u.pfcb->Unlock();
    }
    BTClose( pfucb );

    return err;
}

typedef struct
{
    CDBUTLIRunCalculator *              pRC;
    BTREE_STATS_PARENT_OF_LEAF *    pPOL;
} EVAL_INT_PAGE_CTX;


ERR EvalInternalPages(
    const PGNO pgno, const ULONG iLevel, const CPAGE * pcpage, void * pvCtx
    )
{
    EVAL_INT_PAGE_CTX * pEval = (EVAL_INT_PAGE_CTX*)pvCtx;

    Assert( pEval );

    if ( pcpage->FLeafPage() )
    {
        //  This is a leaf page, this can only happen for a single page B-Tree, because
        //  we directed ErrBTUTLAcross() to only drive as deep as parent of leaf.
        Assert( pcpage->FRootPage() );
        Assert( pEval->pPOL->cpgInternal == 0 );
        pEval->pPOL->cpgData++;

        if ( pEval->pRC )
        {
            pEval->pRC->ErrProcessPage(pgno);
        }
    }
    else
    {
        //  Otherwise this is just an internal B-Tree page.
        pEval->pPOL->cpgInternal++;

        #ifdef SPACE_DUMP_ACOUNT_FOR_RUNS_BROKEN_BY_INTERNAL_PAGES
        //  LB says if a pre-read crosses pages on internal level that the preread
        //  is broken.  SO we may want to check runs that get broken by internal 
        //  pages too. :P
        if ( pEval->pRC )
        {
            pEval->pRC->ErrBreak();
        }
        #endif
    }

    if ( pEval->pPOL->cDepth == 0 &&
        ( pcpage->FParentOfLeaf() || pcpage->FLeafPage() ) )
    {
        //  We've gotten a leaf (root) or the parent of leaf page, we can figure out and set the depth of the b-tree...
        pEval->pPOL->cDepth = 1 + ( pcpage->FLeafPage() ? 0 : ( iLevel + 1 ) );
    }

    return JET_errSuccess;
}


ERR EvalInternalPageNodes(
    const CPAGE::PGHDR * const  ppghdr,
    INT                         itag,
    DWORD                       fNodeFlags,
    const KEYDATAFLAGS * const  pkdf,
    void *                      pvCtx
    )
{
    ERR err = JET_errSuccess;
    EVAL_INT_PAGE_CTX * pEval = (EVAL_INT_PAGE_CTX*)pvCtx;

    Assert( pEval );

    if( !( ppghdr->fFlags & CPAGE::fPageLeaf ) )
    {
        //  Call through to accumulate stats on internal pages.
        //
        if ( pEval->pPOL->pInternalPageStats )
        {
            CallR( ErrAccumulatePageStats(
                                ppghdr,
                                itag,
                                fNodeFlags,
                                pkdf,
                                pEval->pPOL->pInternalPageStats ) );
        }
    }

    //  Now compute b-tree based stats.
    //
    if ( pkdf == NULL )
    {
        Assert( itag == 0 );    // we don't process the external header as a regular node...
        return JET_errSuccess;
    }

    if ( pEval->pPOL->fEmpty )
    {
        //  Not 100% sure we can know this w/o a full walk... this case means that there
        //  is at least one node on a single page B-Tree or a child page of root.  Does
        //  this mean the tree is not empty?
        pEval->pPOL->fEmpty = fFalse;
    }

    if( ppghdr->fFlags & CPAGE::fPageParentOfLeaf )
    {
        PGNO pgnoLeaf = *((UnalignedLittleEndian<ULONG>*)pkdf->data.Pv());

        pEval->pPOL->cpgData++;

        if ( pEval->pRC )
        {
            pEval->pRC->ErrProcessPage(pgnoLeaf);
        }

    }

    return err;
}


ERR ErrDBUTLGetParentOfLeaf(
    _In_  const IFMP                    ifmp,
    _In_  const PGNO                    pgnoFDP,
    _Out_ BTREE_STATS_PARENT_OF_LEAF *  pParentOfLeaf
    )
{
    ERR err = JET_errSuccess;

    CBTreeStatsManager::ResetParentOfLeaf( pParentOfLeaf );
    Assert( pParentOfLeaf->phistoIOContiguousRuns );

    pParentOfLeaf->fEmpty = fTrue;

    CDBUTLIRunCalculator RC( (CStats*)(pParentOfLeaf->phistoIOContiguousRuns), &(pParentOfLeaf->cForwardScans) );
    EVAL_INT_PAGE_CTX ctx = { &RC, pParentOfLeaf };

    Call( ErrBTUTLAcross( ifmp, pgnoFDP,
                CPAGE::fPageParentOfLeaf,
                EvalInternalPages, &ctx,
                EvalInternalPageNodes, &ctx ) );
    RC.ErrProcessPage(CDBUTLIRunCalculator::pgnoDoneSentinel);

    Assert( pParentOfLeaf->cDepth );
    Assert( !pParentOfLeaf->fEmpty || ( pParentOfLeaf->cDepth == 1 ) );
    Assert( ( pParentOfLeaf->cpgInternal == 1 ) || ( pParentOfLeaf->cDepth != 2 ) );
    Assert( pParentOfLeaf->fEmpty || pParentOfLeaf->cForwardScans );

HandleError:
    
    return err;
}


ERR ErrDBUTLGetAdditionalSpaceData(
    PIB *               ppib,
    const IFMP          ifmp,
    const OBJID         objidFDP,
    const PGNO          pgnoFDP,
    BTREE_STATS * const pbts,
    CPRINTF * const     pcprintf )
{
    ERR err = JET_errSuccess;

    if( pbts->pSpaceTrees )
    {
        Call( ErrDBUTLGetSpaceTreeInfo(
                    ppib,
                    ifmp,
                    objidFDP,
                    pgnoFDP,
                    pbts->pSpaceTrees,
                    pcprintf ) );
    }

    if ( pbts->pParentOfLeaf )
    {
        Call( ErrDBUTLGetParentOfLeaf( ifmp, pgnoFDP, pbts->pParentOfLeaf ) );
    }

    if ( pbts->pFullWalk )
    {
        Expected( pbts->pBasicCatalog->eType != eBTreeTypeInternalLongValue ||
                    pbts->pLvData != NULL );

        Call( ErrDBUTLGetDataPageStats(
                    ppib,
                    ifmp,
                    pgnoFDP,
                    pbts->pFullWalk,
                    pbts->pBasicCatalog->eType == eBTreeTypeInternalLongValue ? pbts->pLvData : NULL ) );
    }

HandleError:
    
    return err;
}

typedef struct
{
    JET_SESID           ppib;
    IFMP                ifmp;
    JET_GRBIT           grbitDbUtilOptions;
    WCHAR *             wszSelectedTable;
    BTREE_STATS *       pbts;
    OBJID               objidCurrentTable;
    JET_PFNSPACEDATA    pfnBTreeStatsAnalysisFunc;
    JET_API_PTR         pvBTreeStatsAnalysisFuncCtx;
} DBUTIL_ENUM_SPACE_CTX;

ERR ErrDBUTLEnumSingleSpaceTree(
    PIB *                   ppib,
    const IFMP              ifmp,
    const JET_BTREETYPE     eBTType,
    BTREE_STATS * const     pbts,
    JET_PFNSPACEDATA        pfnBTreeStatsAnalysisFunc,
    JET_API_PTR             pvBTreeStatsAnalysisFuncCtx )
{
    ERR err = JET_errSuccess;

    Assert( pbts );
    Assert( pbts->pParent );    // OE/AE have a parent, we should be on a child pbts.
    Assert( pbts->pSpaceTrees );
    Assert( pbts->pParent->pSpaceTrees->fMultiExtent ); // parent should be multi-extent

    Assert( eBTType == eBTreeTypeInternalSpaceOE ||
            eBTType == eBTreeTypeInternalSpaceAE );

    //
    //  First retrieve the root pages of the two (OE & AE) space trees from the parent.
    //

    memset( pbts->pSpaceTrees, 0, sizeof(*(pbts->pSpaceTrees)) );
    pbts->pSpaceTrees->cbStruct = sizeof(*(pbts->pSpaceTrees));
    pbts->pSpaceTrees->pgnoOE = pbts->pParent->pSpaceTrees->pgnoOE;
    pbts->pSpaceTrees->pgnoAE = pbts->pParent->pSpaceTrees->pgnoAE;

    //  Now pick which we're doing based on B Tree type asked for.
    PGNO pgnoFDP = ( eBTType == eBTreeTypeInternalSpaceOE ) ?
                        pbts->pSpaceTrees->pgnoOE :
                        pbts->pSpaceTrees->pgnoAE;

    //  Setup catalog info.
    //
    if ( pbts->pBasicCatalog )
    {
        pbts->pBasicCatalog->eType = eBTType;
        pbts->pBasicCatalog->pgnoFDP = pgnoFDP; // parent has this.
        Assert( pbts->pBasicCatalog->objidFDP );
        Assert( pbts->pParent->pBasicCatalog->objidFDP == pbts->pBasicCatalog->objidFDP );  // should be true for both space trees
        if ( pbts->pBasicCatalog->eType == eBTreeTypeInternalSpaceOE )
        {
            OSStrCbCopyW( pbts->pBasicCatalog->rgName, sizeof(pbts->pBasicCatalog->rgName), L"[Owned Extents]" );
        }
        else
        {
            Assert( pbts->pBasicCatalog->eType == eBTreeTypeInternalSpaceAE );
            OSStrCbCopyW( pbts->pBasicCatalog->rgName, sizeof(pbts->pBasicCatalog->rgName), L"[Avail Extents]" );
        }
        pbts->pBasicCatalog->pSpaceHints = NULL;
    }

    //  Setup space tree info.
    //
    if ( pbts->pSpaceTrees )
    {
        Assert( pbts->pSpaceTrees->cbStruct == sizeof(*(pbts->pSpaceTrees)) );

        pbts->pSpaceTrees->cpgPrimary = 1;
        Assert( ( pbts->pSpaceTrees->pgnoOE + 1 ) == pbts->pSpaceTrees->pgnoAE );
        pbts->pSpaceTrees->cpgAvailable = 0;
        pbts->pSpaceTrees->cpgSpaceTreeAvailable = 0;

        BTREE_STATS_PARENT_OF_LEAF  btsSpaceTree = { 0 };
        EVAL_INT_PAGE_CTX ctx = { NULL, &btsSpaceTree };

        Call( ErrBTUTLAcross( ifmp,
                    pgnoFDP,
                    CPAGE::fPageParentOfLeaf,
                    EvalInternalPages, &ctx,
                    EvalInternalPageNodes, &ctx ) );
        pbts->pSpaceTrees->cpgOwned = btsSpaceTree.cpgInternal + btsSpaceTree.cpgData;
        pbts->pSpaceTrees->fMultiExtent = (btsSpaceTree.cpgInternal != 0);

    }

    //  Parent of leaf
    //
    if ( pbts->pParentOfLeaf )
    {
        Call( ErrDBUTLGetParentOfLeaf( ifmp, pgnoFDP, pbts->pParentOfLeaf ) );
    }

    //  Full Walk
    //
    if ( pbts->pFullWalk )
    {
        Call( ErrDBUTLGetDataPageStats(
                    ppib,
                    ifmp,
                    pgnoFDP,
                    pbts->pFullWalk,
                    NULL ) );
    }

    //  Initiate callback.
    //
    Call( pfnBTreeStatsAnalysisFunc( pbts, pvBTreeStatsAnalysisFuncCtx ) );

HandleError:

    return err;
}

ERR ErrDBUTLEnumSpaceTrees(
    PIB *                   ppib,
    const IFMP              ifmp,
    const OBJID             objidParent,
    BTREE_STATS * const     pbts,
    JET_PFNSPACEDATA        pfnBTreeStatsAnalysisFunc,
    JET_API_PTR             pvBTreeStatsAnalysisFuncCtx )
{
    ERR err = JET_errSuccess;

    Assert( pbts );
    Assert( pbts->pParent );    // OE/AE have a parent, we should be on a child pbts.
    Assert( pbts->pSpaceTrees );
    Assert( pbts->pParent->pSpaceTrees->fMultiExtent ); // parent should be multi-extent

    //  Generic pieces common to both OE/AE trees...
    //
    if ( pbts->pBasicCatalog )
    {
        //  We just did the table, so we should be able to setup this data by copying it
        //  down from the parent.
        memset( pbts->pBasicCatalog, 0, sizeof(*(pbts->pBasicCatalog)) );
        pbts->pBasicCatalog->cbStruct = sizeof(*(pbts->pBasicCatalog));
        pbts->pBasicCatalog->objidFDP = pbts->pParent->pBasicCatalog->objidFDP;
        Assert( pbts->pParent->pBasicCatalog->objidFDP == objidParent );
        Assert( pbts->pBasicCatalog->objidFDP == objidParent ); // lets make really sure.
        pbts->pBasicCatalog->pSpaceHints = NULL;
    }

    //
    //  Do OE Tree
    //

    Call( ErrDBUTLEnumSingleSpaceTree(
                ppib,
                ifmp,
                eBTreeTypeInternalSpaceOE,
                pbts,
                pfnBTreeStatsAnalysisFunc,
                pvBTreeStatsAnalysisFuncCtx ) );


    //
    //  Do AE Tree
    //

    Call( ErrDBUTLEnumSingleSpaceTree(
                ppib,
                ifmp,
                eBTreeTypeInternalSpaceAE,
                pbts,
                pfnBTreeStatsAnalysisFunc,
                pvBTreeStatsAnalysisFuncCtx ) );

HandleError:

    return err;
}


//  ================================================================
ERR ErrDBUTLEnumIndexSpace( const INDEXDEF * pindexdef, void * pv )
//  ================================================================
{
    //  this will only compile if the signatures match
    PFNINDEX    pfnindex        = ErrDBUTLEnumIndexSpace;
    ERR         err             = JET_errSuccess;
    DBUTIL_ENUM_SPACE_CTX   *pdbues = (DBUTIL_ENUM_SPACE_CTX *)pv;
    PIB         *ppib           = (PIB*)pdbues->ppib;
    const IFMP  ifmp            = pdbues->ifmp;

    BTREE_STATS * pbts          = pdbues->pbts;

    Unused( pfnindex );

    Assert( pbts );
    Assert( pbts->pParent );

    Assert( pindexdef );

    if ( pindexdef->fPrimary )
    {
        //  primary index dumped when table was dumped
        return JET_errSuccess;
    }

    //  Setup catalog data.
    //
    if ( pbts->pBasicCatalog )
    {
        BTREE_STATS_BASIC_CATALOG * pbtsBasicCatalog = pbts->pBasicCatalog;
        pbtsBasicCatalog->cbStruct = sizeof(*pbtsBasicCatalog);
        pbtsBasicCatalog->eType = eBTreeTypeUserSecondaryIndex;
        CAutoWSZDDL cwszDDL;
        CallR( cwszDDL.ErrSet(pindexdef->szName) );
        OSStrCbCopyW( pbtsBasicCatalog->rgName, sizeof(pbtsBasicCatalog->rgName), cwszDDL.Pv() );
        pbtsBasicCatalog->objidFDP = pindexdef->objidFDP;
        pbtsBasicCatalog->pgnoFDP = pindexdef->pgnoFDP;
        pbts->pBasicCatalog->pSpaceHints = (JET_SPACEHINTS*) &(pindexdef->spacehints);
    }

    CPRINTF * const pcprintf = ( pdbues->grbitDbUtilOptions & JET_bitDBUtilOptionDumpVerbose ) ?
        CPRINTFSTDOUT::PcprintfInstance() : NULL;

    //  Setup other BTree stats data.
    //
    CallR( ErrDBUTLGetAdditionalSpaceData(
                    ppib,
                    ifmp,
                    pindexdef->objidFDP,
                    pindexdef->pgnoFDP,
                    pbts,
                    pcprintf ) );

    //  Callback to client.
    //
    Call( pdbues->pfnBTreeStatsAnalysisFunc( pbts, pdbues->pvBTreeStatsAnalysisFuncCtx ) );


    //
    //  Now do space trees if appropriate.
    //
    if ( pbts->pSpaceTrees && pbts->pSpaceTrees->fMultiExtent )
    {
        CBTreeStatsManager  btsIdxSpaceTreesManager( pdbues->grbitDbUtilOptions, pbts );

        //  Note this function fills in all data and does the callbacks.
        //
        Call( ErrDBUTLEnumSpaceTrees(
                    ppib,
                    ifmp,
                    pindexdef->objidFDP,
                    btsIdxSpaceTreesManager.Pbts(),
                    pdbues->pfnBTreeStatsAnalysisFunc,
                    pdbues->pvBTreeStatsAnalysisFuncCtx ) );
    }

HandleError:

    return err;
}

//  ================================================================
ERR ErrDBUTLEnumTableSpace( const TABLEDEF * ptabledef, void * pv )
//  ================================================================
{
    //  this will only compile if the signatures match
    PFNTABLE pfntable = ErrDBUTLEnumTableSpace;

    ERR         err;
    DBUTIL_ENUM_SPACE_CTX   *pdbues = (DBUTIL_ENUM_SPACE_CTX *)pv;
    PIB         *ppib           = (PIB *)pdbues->ppib;
    const IFMP  ifmp            = (IFMP)pdbues->ifmp;

    BTREE_STATS *   pbts = pdbues->pbts;

    //  For children of tables.
    CBTreeStatsManager  btsTableChildrenManager( pdbues->grbitDbUtilOptions, pbts );

    Unused( pfntable );

    Assert( ptabledef );

    Assert( pbts );
    Assert( pbts->pParent );                        // all tables have a parent

    //
    //  [1] Setup data and do callback for this table.
    //

    //  Setup catalog data.
    //
    if ( pbts->pBasicCatalog )
    {
        //  User registered interest in this data.
        BTREE_STATS_BASIC_CATALOG * pbtsBasicCatalog = pbts->pBasicCatalog;

        pbtsBasicCatalog->cbStruct = sizeof(*pbtsBasicCatalog);
        pbtsBasicCatalog->eType = eBTreeTypeUserClusteredIndex;
        CAutoWSZDDL cwszDDL;
        CallR( cwszDDL.ErrSet( ptabledef->szName ) );
        OSStrCbCopyW( pbtsBasicCatalog->rgName, sizeof(pbtsBasicCatalog->rgName), cwszDDL.Pv() );
        pbtsBasicCatalog->objidFDP = ptabledef->objidFDP;
        pbtsBasicCatalog->pgnoFDP = ptabledef->pgnoFDP;
        pbts->pBasicCatalog->pSpaceHints = (JET_SPACEHINTS*) &(ptabledef->spacehints);
    }

    if ( pgnoNull != ptabledef->pgnoFDPLongValues )
    {
        (void)ErrBFPrereadPage( ifmp, ptabledef->pgnoFDPLongValues, bfprfDefault, ppib->BfpriPriority( ifmp ), TcCurr() );
    }

    CPRINTF * const pcprintf = ( pdbues->grbitDbUtilOptions & JET_bitDBUtilOptionDumpVerbose ) ?
        CPRINTFSTDOUT::PcprintfInstance() : NULL;

    //  Setup other BTree stats data.
    //
    err = ErrDBUTLGetAdditionalSpaceData(
                    ppib,
                    ifmp,
                    ptabledef->objidFDP,
                    ptabledef->pgnoFDP,
                    pbts,
                    pcprintf );
    if ( err == JET_errRBSFDPToBeDeleted )
    {
        pbts->fPgnoFDPRootDelete = fTrue;
        Call( pdbues->pfnBTreeStatsAnalysisFunc( pbts, pdbues->pvBTreeStatsAnalysisFuncCtx ) );
        return JET_errSuccess;
    }
    Call( err );

    pbts->fPgnoFDPRootDelete = fFalse;

    //  Callback to client.
    //
    Call( pdbues->pfnBTreeStatsAnalysisFunc( pbts, pdbues->pvBTreeStatsAnalysisFuncCtx ) );


    //
    //  Setup ourselves for doing callback on children of this table.
    //
    pbts = btsTableChildrenManager.Pbts();


    //
    //  [2,3]   The first children we do are the space trees (OE & AE).
    //
    if ( pbts->pSpaceTrees && pbts->pParent->pSpaceTrees->fMultiExtent )
    {

        //  Note this function fills in all data and does the callbacks.
        //
        Call( ErrDBUTLEnumSpaceTrees(
                    ppib,
                    ifmp,
                    ptabledef->objidFDP,
                    pbts,
                    pdbues->pfnBTreeStatsAnalysisFunc,
                    pdbues->pvBTreeStatsAnalysisFuncCtx ) );

    }

    //
    //  [4] If the LV exists, we do that child next.
    //
    if ( pgnoNull != ptabledef->pgnoFDPLongValues )
    {

        //  Fill basic catalog info.
        //
        if ( pbts->pBasicCatalog )
        {
            //  User registered interest in this data.
            BTREE_STATS_BASIC_CATALOG * pbtsBasicCatalog = pbts->pBasicCatalog;
            pbtsBasicCatalog->cbStruct = sizeof(*pbtsBasicCatalog);
            pbtsBasicCatalog->eType = eBTreeTypeInternalLongValue;
            OSStrCbCopyW( pbtsBasicCatalog->rgName, sizeof(pbtsBasicCatalog->rgName), L"[Long Values]" );
            pbtsBasicCatalog->objidFDP = ptabledef->objidFDPLongValues;
            pbtsBasicCatalog->pgnoFDP = ptabledef->pgnoFDPLongValues;
            //  Getting space hints for LV tree.
            pbtsBasicCatalog->pSpaceHints = (JET_SPACEHINTS*) &(ptabledef->spacehintsLV);
        }

        if ( pbts->pLvData )
        {
            pbts->pLvData->cbLVChunkMax = ptabledef->cbLVChunkMax;
        }

        //  Fill other data.
        //
        Call( ErrDBUTLGetAdditionalSpaceData(
                        ppib,
                        ifmp,
                        ptabledef->objidFDPLongValues,
                        ptabledef->pgnoFDPLongValues,
                        pbts,
                        pcprintf ) );
                        
        //  Callback to client.
        //
        Call( pdbues->pfnBTreeStatsAnalysisFunc( pbts, pdbues->pvBTreeStatsAnalysisFuncCtx ) );


        //
        // [4.1,4.2]    If the LV is a multi-extent, do space trees as well.
        //

        CBTreeStatsManager  btsLVSpaceTreesManager( pdbues->grbitDbUtilOptions, pbts );

        if ( pbts->pSpaceTrees && pbts->pSpaceTrees->fMultiExtent )
        {

            //  Note this function fills in all data and does the callbacks.
            //
            Call( ErrDBUTLEnumSpaceTrees(
                        ppib,
                        ifmp,
                        ptabledef->objidFDPLongValues,
                        btsLVSpaceTreesManager.Pbts(),
                        pdbues->pfnBTreeStatsAnalysisFunc,
                        pdbues->pvBTreeStatsAnalysisFuncCtx ) );

        }
    }

    //
    //  [5-n]   Finally enumerate all the secondary indices.
    //

    DBUTIL_ENUM_SPACE_CTX dbuesIdx;
    memcpy( &dbuesIdx, pdbues, sizeof(dbuesIdx) );

    dbuesIdx.pbts =  btsTableChildrenManager.Pbts();

    //  Everything is reuseable from the original handle, but we want to use 
    //  the new BTree stats structure set out for the children of tables.
    JET_DBUTIL_W    dbutil;
    memset( &dbutil, 0, sizeof( dbutil ) );
    
    dbutil.cbStruct     = sizeof( dbutil );
    dbutil.op           = opDBUTILEDBDump;
    dbutil.sesid            = pdbues->ppib;
    dbutil.dbid         = (JET_DBID)pdbues->ifmp;
    dbutil.pgno         = ptabledef->objidFDP;
    dbutil.pfnCallback  = (void *)ErrDBUTLEnumIndexSpace;
    dbutil.pvCallback   = &dbuesIdx;
    dbutil.edbdump      = opEDBDumpIndexes;
    dbutil.grbitOptions = pdbues->grbitDbUtilOptions;

    pdbues->objidCurrentTable = ptabledef->objidFDP;    // set this so ErrDBUTLEnumIndexSpace can know parent objid.

    err = ErrDBUTLDump( pdbues->ppib, &dbutil );

HandleError:
    
    return err;
}


//  ================================================================
LOCAL INT PrintIndexBareMetaData( const INDEXDEF * pindexdef, void * pv )
//  ================================================================
{
    //  this will only compile if the signatures match
    PFNINDEX    pfnindex        = PrintIndexBareMetaData;

    Unused( pfnindex );
    
    Assert( pindexdef );

    printf( "  %-49.49s %s  ", pindexdef->szName, ( pindexdef->fPrimary ? "Pri" : "Idx" ) );
    DBUTLPrintfIntN( pindexdef->objidFDP, 10 );
    printf( " " );
    DBUTLPrintfIntN( pindexdef->pgnoFDP, 10 );
    printf( "\n" );

    return JET_errSuccess;
}


//  ================================================================
LOCAL INT PrintTableBareMetaData( const TABLEDEF * ptabledef, void * pv )
//  ================================================================
{
    //  this will only compile if the signatures match
    PFNTABLE    pfntable        = PrintTableBareMetaData;
    ERR         err;
    JET_DBUTIL_A    *pdbutil    = (JET_DBUTIL_A *)pv;

    Unused( pfntable );

    Assert( ptabledef );
    
    printf( "%-51.51s Tbl  ", ptabledef->szName );
    DBUTLPrintfIntN( ptabledef->objidFDP, 10 );
    printf( " " );
    DBUTLPrintfIntN( ptabledef->pgnoFDP, 10 );
    printf( "    " );

    LOGTIME tmPgnoFDPLastSet = { 0 };
    ConvertFileTimeToLogTime( ptabledef->ftPgnoFDPLastSet, &tmPgnoFDPLastSet );
    DUMPPrintLogTime( &tmPgnoFDPLastSet );
    printf( "\n" );

    if ( pgnoNull != ptabledef->pgnoFDPLongValues )
    {
        printf( "  %-49.49s LV   ", "<Long Values>" );
        DBUTLPrintfIntN( ptabledef->objidFDPLongValues, 10 );
        printf( " " );
        DBUTLPrintfIntN( ptabledef->pgnoFDPLongValues, 10 );
        printf( "    " );

        LOGTIME tmPgnoFDPLongValuesLastSet = { 0 };
        ConvertFileTimeToLogTime( ptabledef->ftPgnoFDPLongValuesLastSet, &tmPgnoFDPLongValuesLastSet );
        DUMPPrintLogTime( &tmPgnoFDPLongValuesLastSet );
        printf( "\n" );
    }
    
    JET_DBUTIL_W    dbutil;
    memset( &dbutil, 0, sizeof( dbutil ) );
    
    dbutil.cbStruct     = sizeof( dbutil );
    dbutil.op           = opDBUTILEDBDump;
    dbutil.sesid        = pdbutil->sesid;
    dbutil.dbid         = pdbutil->dbid;
    dbutil.pgno         = ptabledef->objidFDP;
    dbutil.pfnCallback  = (void *)PrintIndexBareMetaData;
    dbutil.pvCallback   = &dbutil;
    dbutil.edbdump      = opEDBDumpIndexes;
    dbutil.grbitOptions = pdbutil->grbitOptions;

    err = ErrDBUTLDump( pdbutil->sesid, &dbutil );

    return err;
}

//  ================================================================
LOCAL VOID DBUTLDumpDefaultSpaceHints( __inout JET_SPACEHINTS * const pSpacehints, _In_ const CPG cpgInitial, _In_ const BOOL fTable )
//  ================================================================
{
    if ( 0 == pSpacehints->cbInitial )
    {
        pSpacehints->cbInitial = g_cbPage * cpgInitial;
    }

    if ( 0 == pSpacehints->ulMaintDensity )
    {
        pSpacehints->ulMaintDensity = ulFILEDefaultDensity;
    }

    if ( 0 == pSpacehints->cbMinExtent )
    {
        // If it's not a table, we must consider if JET_bitSpaceHintsUtilizeParentSpace is not set. This grbit will 
        // change the allocation behavior so that it will only allocate one page at a time for the index (although it gets 
        // multiple pages to the parent tables' space and then gives one page to the index).
        if ( fTable || ( 0 == ( pSpacehints->grbit & JET_bitSpaceHintsUtilizeParentSpace ) ) )
        {
            pSpacehints->cbMinExtent = cpgSmallGrow * g_cbPage;
        }
        else
        {
            pSpacehints->cbMinExtent = g_cbPage;
        }
    }

    if ( 0 == pSpacehints->cbMaxExtent )
    {
        // If it's not a table, we must consider if JET_bitSpaceHintsUtilizeParentSpace is not set. This grbit will 
        // change the allocation behavior so that it will only allocate one page at a time for the index (although it gets 
        // multiple pages to the parent tables' space and then gives one page to the index).
        if ( fTable || ( 0 == ( pSpacehints->grbit & JET_bitSpaceHintsUtilizeParentSpace ) ) )
        {
            pSpacehints->cbMaxExtent = cpageSEDefault * g_cbPage;
        }
        else
        {
            pSpacehints->cbMaxExtent = g_cbPage;
        }
    }

    if ( 0 == pSpacehints->ulGrowth )
    {
        pSpacehints->ulGrowth = 100;
    }
}

ERR ErrCATIUnmarshallExtendedSpaceHints(
    _In_ INST * const           pinst,
    _In_ const SYSOBJ           sysobj,
    _In_ const BOOL             fDeferredLongValueHints,
    _In_ const BYTE * const     pBuffer,
    _In_ const ULONG            cbBuffer,
    _In_ const LONG             cbPageSize,
    _Out_ JET_SPACEHINTS *      pSpacehints
    );


//  ================================================================
LOCAL ERR ErrDBUTLDumpOneIndex( PIB * ppib, FUCB * pfucbCatalog, VOID * pfnCallback, VOID * pvCallback )
//  ================================================================
{
    JET_RETRIEVECOLUMN  rgretrievecolumn[18];
    BYTE                pbufidxseg[JET_ccolKeyMost*sizeof(IDXSEG)];
    BYTE                pbufidxsegConditional[JET_ccolKeyMost*sizeof(IDXSEG)];
    BYTE                pbExtendedSpaceHints[cbExtendedSpaceHints];
    INDEXDEF            indexdef;
    ERR                 err             = JET_errSuccess;
    INT                 iretrievecolumn = 0;
    OBJID               objidTable;
    USHORT              cbVarSegMac     = 0;
    USHORT              cbKeyMost       = 0;

    QWORD qwSortVersion = 0;
    
    memset( &indexdef, 0, sizeof( indexdef ) );
    memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );

    //  objectId of owning table
    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_ObjidTable;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&objidTable;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(objidTable);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    //  pgnoFDP of index tree
    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_PgnoFDP;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&indexdef.pgnoFDP;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(indexdef.pgnoFDP);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    //  indexId (objidFDP of index tree)
    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Id;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&indexdef.objidFDP;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(indexdef.objidFDP);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Name;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)indexdef.szName;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(indexdef.szName);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;
    
    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_SpaceUsage;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&indexdef.density;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(indexdef.density);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Localization;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&indexdef.lcid;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(indexdef.lcid);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_LocaleName;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&indexdef.wszLocaleName;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(indexdef.wszLocaleName);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_SortID;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&indexdef.sortID;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(indexdef.sortID);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_LCMapFlags;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&indexdef.dwMapFlags;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(indexdef.dwMapFlags);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Flags;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&indexdef.fFlags;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(indexdef.fFlags);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    const INT iretcolTupleLimits                    = iretrievecolumn;
    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_TupleLimits;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&indexdef.le_tuplelimits;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(indexdef.le_tuplelimits);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    const INT iretcolIdxsegConditional              = iretrievecolumn;
    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_ConditionalColumns;
    rgretrievecolumn[iretrievecolumn].pvData        = pbufidxsegConditional;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(pbufidxsegConditional);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    const INT iretcolIdxseg                         = iretrievecolumn;
    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_KeyFldIDs;
    rgretrievecolumn[iretrievecolumn].pvData        = pbufidxseg;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(pbufidxseg);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_VarSegMac;
    rgretrievecolumn[iretrievecolumn].pvData        = &cbVarSegMac;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(cbVarSegMac);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_KeyMost;
    rgretrievecolumn[iretrievecolumn].pvData        = &cbKeyMost;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(cbKeyMost);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;


    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Version;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&qwSortVersion;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(qwSortVersion);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    const INT iretcolExtSpaceHints                      = iretrievecolumn;
    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_SpaceHints;
    rgretrievecolumn[iretrievecolumn].pvData        = pbExtendedSpaceHints;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(pbExtendedSpaceHints);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_PgnoFDPLastSetTime;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&indexdef.ftPgnoFDPLastSet;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(indexdef.ftPgnoFDPLastSet);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    Assert( iretrievecolumn <= sizeof( rgretrievecolumn ) / sizeof( rgretrievecolumn[0] ) );

    CallR( ErrIsamRetrieveColumns(
                (JET_SESID)ppib,
                (JET_TABLEID)pfucbCatalog,
                rgretrievecolumn,
                iretrievecolumn ) );

    const IDBFLAG   idbflag     = (IDBFLAG)indexdef.fFlags;
    const IDXFLAG   idxflag     = (IDXFLAG)( indexdef.fFlags >> sizeof(IDBFLAG) * 8 );

    indexdef.fUnique            = !!FIDBUnique( idbflag );
    indexdef.fPrimary           = !!FIDBPrimary( idbflag );
    indexdef.fAllowAllNulls     = !!FIDBAllowAllNulls( idbflag );
    indexdef.fAllowFirstNull    = !!FIDBAllowFirstNull( idbflag );
    indexdef.fAllowSomeNulls    = !!FIDBAllowSomeNulls( idbflag );
    indexdef.fNoNullSeg         = !!FIDBNoNullSeg( idbflag );
    indexdef.fSortNullsHigh     = !!FIDBSortNullsHigh( idbflag );
    indexdef.fMultivalued       = !!FIDBMultivalued( idbflag );
    indexdef.fTuples            = ( JET_wrnColumnNull == rgretrievecolumn[iretcolTupleLimits].err ? fFalse : fTrue );
    indexdef.fLocaleSet         = !!FIDBLocaleSet( idbflag );
    indexdef.fLocalizedText     = !!FIDBLocalizedText( idbflag );
    indexdef.fTemplateIndex     = !!FIDBTemplateIndex( idbflag );
    indexdef.fDerivedIndex      = !!FIDBDerivedIndex( idbflag );
    indexdef.fExtendedColumns   = !!FIDXExtendedColumns( idxflag );

    indexdef.cbVarSegMac        = cbVarSegMac;
    indexdef.cbKeyMost          = cbKeyMost;

    indexdef.dwDefinedVersion   = (DWORD)( ( qwSortVersion >> 32 ) & 0xFFFFFFFF );
    indexdef.dwNLSVersion       = (DWORD)( qwSortVersion & 0xFFFFFFFF );

    // If this is a secondary index and it is derived as well, we must look at the space hints for
    // the template index instead. Of course, we only do so when there *are* any space hints
    // set for this index in the first place - otherwise there is nothing that it could have inherited.
    if ( !indexdef.fPrimary && indexdef.fDerivedIndex )
    {
        const INT cchTableName = JET_cbNameMost + 1;
        CHAR szTableName[cchTableName];
        FUCB * pfucbDerivedTable;
        FCB * pfcbDerivedIndex;

        // First we find the table name for this index using its objid...
        CallR( ErrCATSeekTableByObjid(
                ppib,
                pfucbCatalog->ifmp,
                objidTable,
                szTableName,
                cchTableName,
                NULL ) );

        // ... then open it in read-only mode.
        CallR( ErrFILEOpenTable(
                ppib,
                pfucbCatalog->u.pfcb->Ifmp(),
                &pfucbDerivedTable,
                szTableName,
                JET_bitTableReadOnly ) );

        const TDB * ptdbDerivedTable = pfucbDerivedTable->u.pfcb->Ptdb();

        // Let's now find the derived index...
        for ( pfcbDerivedIndex = pfucbDerivedTable->u.pfcb;
            NULL != pfcbDerivedIndex;
            pfcbDerivedIndex = pfcbDerivedIndex->PfcbNextIndex() )
        {
            if ( NULL != pfcbDerivedIndex->Pidb() && ( UtilCmpName( indexdef.szName, ptdbDerivedTable->SzIndexName( pfcbDerivedIndex->Pidb()->ItagIndexName(), pfcbDerivedIndex->FDerivedIndex() ) ) == 0 ) )
            {
                break;
            }
        }

        Assert( pfcbDerivedIndex );
        if ( NULL == pfcbDerivedIndex )
        {
            CallR( JET_errCatalogCorrupted );
        }

        // ... and then get its space hints. This will contain the already stamped template's
        // space hints as the FCB above was instantiated already. 
        pfcbDerivedIndex->GetAPISpaceHints( &indexdef.spacehints );
        
        // Finally close the cursor.
        CallS( ErrFILECloseTable( ppib, pfucbDerivedTable ) );
    }
    else
    {
        indexdef.spacehints.cbStruct = sizeof(indexdef.spacehints);
        indexdef.spacehints.ulInitialDensity = indexdef.density;
        if( rgretrievecolumn[iretcolExtSpaceHints].cbActual )
        {
            CallR( ErrCATIUnmarshallExtendedSpaceHints( PinstFromPfucb( pfucbCatalog ), sysobjIndex, fFalse, pbExtendedSpaceHints,
                        rgretrievecolumn[iretcolExtSpaceHints].cbActual, g_rgfmp[ pfucbCatalog->ifmp ].CbPage(), &indexdef.spacehints ) );
        }
    }

    // We now must set the actual values in for the defaults set to zero.

    DBUTLDumpDefaultSpaceHints( &indexdef.spacehints, cpgInitialTreeDefault, fFalse );
    
    Assert( rgretrievecolumn[iretcolIdxseg].cbActual > 0 );

    if ( indexdef.fExtendedColumns )
    {
        INT iidxseg;

        Assert( sizeof(IDXSEG) == sizeof(JET_COLUMNID) );

        Assert( rgretrievecolumn[iretcolIdxseg].cbActual <= sizeof(JET_COLUMNID) * JET_ccolKeyMost );
        Assert( rgretrievecolumn[iretcolIdxseg].cbActual % sizeof(JET_COLUMNID) == 0 );
        Assert( rgretrievecolumn[iretcolIdxseg].cbActual / sizeof(JET_COLUMNID) <= JET_ccolKeyMost );
        indexdef.ccolumnidDef = rgretrievecolumn[iretcolIdxseg].cbActual / sizeof(JET_COLUMNID);

        for ( iidxseg = 0; iidxseg < indexdef.ccolumnidDef; iidxseg++ )
        {
            const LE_IDXSEG     * const ple_idxseg  = (LE_IDXSEG *)pbufidxseg + iidxseg;
            //  Endian conversion
            indexdef.rgidxsegDef[iidxseg] = *ple_idxseg;
            Assert( FCOLUMNIDValid( indexdef.rgidxsegDef[iidxseg].Columnid() ) );
        }

        Assert( rgretrievecolumn[iretcolIdxsegConditional].cbActual <= sizeof(JET_COLUMNID) * JET_ccolKeyMost );
        Assert( rgretrievecolumn[iretcolIdxsegConditional].cbActual % sizeof(JET_COLUMNID) == 0 );
        Assert( rgretrievecolumn[iretcolIdxsegConditional].cbActual / sizeof(JET_COLUMNID) <= JET_ccolKeyMost );
        indexdef.ccolumnidConditional = rgretrievecolumn[iretcolIdxsegConditional].cbActual / sizeof(JET_COLUMNID);

        for ( iidxseg = 0; iidxseg < indexdef.ccolumnidConditional; iidxseg++ )
        {
            const LE_IDXSEG     * const ple_idxsegConditional   = (LE_IDXSEG *)pbufidxsegConditional + iidxseg;
            //  Endian conversion
            indexdef.rgidxsegConditional[iidxseg] = *ple_idxsegConditional;
            Assert( FCOLUMNIDValid( indexdef.rgidxsegConditional[iidxseg].Columnid() ) );
        }

    }
    else
    {
        Assert( sizeof(IDXSEG_OLD) == sizeof(FID) );

        Assert( rgretrievecolumn[iretcolIdxseg].cbActual <= sizeof(FID) * JET_ccolKeyMost );
        Assert( rgretrievecolumn[iretcolIdxseg].cbActual % sizeof(FID) == 0);
        Assert( rgretrievecolumn[iretcolIdxseg].cbActual / sizeof(FID) <= JET_ccolKeyMost );
        indexdef.ccolumnidDef = rgretrievecolumn[iretcolIdxseg].cbActual / sizeof( FID );

        SetIdxSegFromOldFormat(
            (UnalignedLittleEndian< IDXSEG_OLD > *)pbufidxseg,
            indexdef.rgidxsegDef,
            indexdef.ccolumnidDef,
            fFalse,
            fFalse,
            NULL );

        Assert( rgretrievecolumn[iretcolIdxsegConditional].cbActual <= sizeof(FID) * JET_ccolKeyMost );
        Assert( rgretrievecolumn[iretcolIdxsegConditional].cbActual % sizeof(FID) == 0);
        Assert( rgretrievecolumn[iretcolIdxsegConditional].cbActual / sizeof(FID) <= JET_ccolKeyMost );
        indexdef.ccolumnidConditional = rgretrievecolumn[iretcolIdxsegConditional].cbActual / sizeof( FID );

        SetIdxSegFromOldFormat(
            (UnalignedLittleEndian< IDXSEG_OLD > *)pbufidxsegConditional,
            indexdef.rgidxsegConditional,
            indexdef.ccolumnidConditional,
            fTrue,
            fFalse,
            NULL );
    }

    CallR( ErrCATGetIndexSegments(
                    ppib,
                    pfucbCatalog->u.pfcb->Ifmp(),
                    objidTable,
                    indexdef.rgidxsegDef,
                    indexdef.ccolumnidDef,
                    fFalse,
                    !indexdef.fExtendedColumns,
                    indexdef.rgszIndexDef ) );
                    
    CallR( ErrCATGetIndexSegments(
                    ppib,
                    pfucbCatalog->u.pfcb->Ifmp(),
                    objidTable,
                    indexdef.rgidxsegConditional,
                    indexdef.ccolumnidConditional,
                    fTrue,
                    !indexdef.fExtendedColumns,
                    indexdef.rgszIndexConditional ) );

    PFNINDEX const pfnindex = (PFNINDEX)pfnCallback;
    return (*pfnindex)( &indexdef, pvCallback );
}

// Get LV space hints.
LOCAL ERR ErrGetSpaceHintsForLV( PIB * ppib, const IFMP ifmp, const OBJID objidTable, JET_SPACEHINTS * const pSpacehints )
{
    ERR err = JET_errSuccess;
    FUCB * pfucbTable = pfucbNil;
    FUCB * pfucbLV = pfucbNil;
    PGNO pgnoFDP;
    CHAR szTableName[JET_cbNameMost+1];
    
    // Get table name from objidTable.
    Call( ErrCATSeekTableByObjid(
            ppib,
            ifmp,
            objidTable,
            szTableName,
            sizeof( szTableName ),
            &pgnoFDP ) );

    // Open the cursor pfucbTable to open LV root and get space hint
    CallR( ErrFILEOpenTable( ppib, ifmp, &pfucbTable, szTableName ) );

    // Open the LV and get the pfucbLV.
    Call( ErrFILEOpenLVRoot( pfucbTable, &pfucbLV, fFalse ) );

    if( pfucbLV != pfucbNil )
    {
        // Get the LV space hint
        pfucbLV->u.pfcb->GetAPISpaceHints( pSpacehints );
    }
HandleError:
    
    if( pfucbNil != pfucbTable )
    {
        if( pfucbNil != pfucbLV )
        {
            DIRClose( pfucbLV );
            pfucbLV = pfucbNil;
        }
        ErrFILECloseTable( ppib, pfucbTable );
        pfucbTable = pfucbNil;
    }
    return err;
}

//  ================================================================
LOCAL ERR ErrDBUTLDumpOneTable( PIB * ppib, FUCB * pfucbCatalog, PFNTABLE pfntable, VOID * pvCallback )
//  ================================================================
{
    JET_RETRIEVECOLUMN  rgretrievecolumn[11];
    BYTE                pbExtendedSpaceHints[cbExtendedSpaceHints];
    BYTE                pbExtendedSpaceHintsDeferredLV[cbExtendedSpaceHints];
    TABLEDEF            tabledef;
    ERR                 err             = JET_errSuccess;
    INT                 iretrievecolumn = 0;

    memset( &tabledef, 0, sizeof( tabledef ) );
    memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Name;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)tabledef.szName;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( tabledef.szName );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_TemplateTable;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)tabledef.szTemplateTable;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( tabledef.szTemplateTable );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_PgnoFDP;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( tabledef.pgnoFDP );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( tabledef.pgnoFDP );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Id;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( tabledef.objidFDP );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( tabledef.objidFDP );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Pages;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( tabledef.pages );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( tabledef.pages );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_SpaceUsage;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( tabledef.density );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( tabledef.density );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Flags;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( tabledef.fFlags );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( tabledef.fFlags );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    const INT iretcolExtSpaceHints                      = iretrievecolumn;
    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_SpaceHints;
    rgretrievecolumn[iretrievecolumn].pvData        = pbExtendedSpaceHints;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(pbExtendedSpaceHints);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    const INT iretcolExtSpaceHintsDeferredLV            = iretrievecolumn;
    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_SpaceLVDeferredHints;
    rgretrievecolumn[iretrievecolumn].pvData        = pbExtendedSpaceHintsDeferredLV;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof(pbExtendedSpaceHintsDeferredLV);
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_LVChunkMax;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&( tabledef.cbLVChunkMax );
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( tabledef.cbLVChunkMax );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_PgnoFDPLastSetTime;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&tabledef.ftPgnoFDPLastSet;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( tabledef.ftPgnoFDPLastSet );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    Call( ErrIsamRetrieveColumns(
                (JET_SESID)ppib,
                (JET_TABLEID)pfucbCatalog,
                rgretrievecolumn,
                iretrievecolumn ) );

    if ( tabledef.cbLVChunkMax == 0 )
    {
        tabledef.cbLVChunkMax = (LONG)UlParam( JET_paramLVChunkSizeMost );
    }

    // If this is a derived table and it has no space hints of its own, we should look at the template
    // table for space hints it may be inheriting.
    if ( ( tabledef.fFlags & JET_bitObjectTableDerived ) && ( 0 == rgretrievecolumn[iretcolExtSpaceHints].cbActual ) )
    {
        FUCB    *pfucbTemplateTable;

        Assert( ( NULL != tabledef.szTemplateTable ) && ( '\0' != tabledef.szTemplateTable[0] ) );
        
        CallR( ErrFILEOpenTable(
                ppib,
                pfucbCatalog->u.pfcb->Ifmp(),
                &pfucbTemplateTable,
                tabledef.szTemplateTable,
                JET_bitTableReadOnly ) );

        Assert( pfcbNil != pfucbTemplateTable->u.pfcb );
        Assert( pfucbTemplateTable->u.pfcb->FTemplateTable() );
        Expected( pfucbTemplateTable->u.pfcb->FFixedDDL() );

        pfucbTemplateTable->u.pfcb->GetAPISpaceHints( &tabledef.spacehints );
        
        // Close the cursor.
        CallS( ErrFILECloseTable( ppib, pfucbTemplateTable ) );
    }
    else
    {
        tabledef.spacehints.cbStruct = sizeof(tabledef.spacehints);
        tabledef.spacehints.ulInitialDensity    =  tabledef.density;

        if( rgretrievecolumn[iretcolExtSpaceHints].cbActual )
        {
            Call( ErrCATIUnmarshallExtendedSpaceHints( PinstFromPfucb( pfucbCatalog ), sysobjTable, fFalse, pbExtendedSpaceHints,
                        rgretrievecolumn[iretcolExtSpaceHints].cbActual, g_rgfmp[ pfucbCatalog->ifmp ].CbPage(), &tabledef.spacehints ) );
        }

        if( rgretrievecolumn[iretcolExtSpaceHintsDeferredLV].cbActual )
        {
            memset( &tabledef.spacehintsDeferredLV, 0, sizeof(tabledef.spacehintsDeferredLV) );
            tabledef.spacehintsDeferredLV.cbStruct = sizeof(tabledef.spacehints);
            tabledef.spacehintsDeferredLV.ulInitialDensity  =  tabledef.density;
            Call( ErrCATIUnmarshallExtendedSpaceHints( PinstFromPfucb( pfucbCatalog ), sysobjTable, fTrue, pbExtendedSpaceHintsDeferredLV,
                        rgretrievecolumn[iretcolExtSpaceHintsDeferredLV].cbActual, g_rgfmp[ pfucbCatalog->ifmp ].CbPage(), &tabledef.spacehintsDeferredLV ) );
        }
    }

    // We now must set the actual values in for the defaults set to zero.


    DBUTLDumpDefaultSpaceHints( &tabledef.spacehints, tabledef.pages, fTrue );

    Call( ErrCATAccessTableLV(
                ppib,
                pfucbCatalog->u.pfcb->Ifmp(),
                tabledef.objidFDP,
                &tabledef.pgnoFDPLongValues,
                &tabledef.objidFDPLongValues,
                &tabledef.ftPgnoFDPLongValuesLastSet ) );
    
    // Try to dump the actual LV spacehints out here too.
    err = ErrGetSpaceHintsForLV( ppib, pfucbCatalog->u.pfcb->Ifmp(), tabledef.objidFDP, &tabledef.spacehintsLV );
    if ( err >= JET_errSuccess )
    {
        DBUTLDumpDefaultSpaceHints( &tabledef.spacehintsLV, 0, fFalse );
    }
    else
    {
        // We may fail to open a table (e.g. has a large number of indices; or has callback DLL not present).
        // We'll just skip dumping the space hints in that case.
        err = JET_errSuccess;
    }

    Call( (*pfntable)( &tabledef, pvCallback ) );

HandleError:
    return err;
}

//  ================================================================
LOCAL ERR ErrDBUTLDumpTables( PIB * ppib, IFMP ifmp, _In_ PCWSTR wszTableName, PFNTABLE pfntable, VOID * pvCallback )
//  ================================================================
{
    ERR     err;
    FUCB    *pfucbCatalog   = pfucbNil;
    
    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );
    
    Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, szMSORootObjectsIndex ) );

    if ( NULL != wszTableName )
    {
        //  find the table we want and dump it
        const BYTE  bTrue   = 0xff;
        CAutoSZDDL  szTableName;
        
        Call( szTableName.ErrSet( wszTableName ) );
        
        Call( ErrIsamMakeKey(
                    ppib,
                    pfucbCatalog,
                    &bTrue,
                    sizeof(bTrue),
                    JET_bitNewKey ) );
        Call( ErrIsamMakeKey(
                    ppib,
                    pfucbCatalog,
                    (CHAR*)szTableName,
                    (ULONG)strlen((CHAR*)szTableName),
                    NO_GRBIT ) );
        err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );
        if ( JET_errRecordNotFound == err )
            err = ErrERRCheck( JET_errObjectNotFound );
        Call( err );
        CallS( err );

        Call( ErrDBUTLDumpOneTable( ppib, pfucbCatalog, pfntable, pvCallback ) );
    }
    else
    {
        err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT );
        while ( JET_errNoCurrentRecord != err )
        {
            Call( err );
            
            Call( ErrDBUTLDumpOneTable( ppib, pfucbCatalog, pfntable, pvCallback ) );
            err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
        }
    }
    
    err = JET_errSuccess;

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}


//  ================================================================
LOCAL ERR ErrDBUTLDumpTableObjects(
    PIB             *ppib,
    const IFMP      ifmp,
    const OBJID     objidFDP,
    const SYSOBJ    sysobj,
    PFNDUMP         pfnDump,
    VOID            *pfnCallback,
    VOID            *pvCallback )
//  ================================================================
{
    ERR             err;
    FUCB            *pfucbCatalog   = pfucbNil;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );
    FUCBSetSequential( pfucbCatalog );

    Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, szMSONameIndex ) );
    
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&objidFDP,
                sizeof(objidFDP),
                JET_bitNewKey ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&sysobj,
                sizeof(sysobj),
                NO_GRBIT ) );

    err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekGT );
    if ( err < 0 )
    {
        if ( JET_errRecordNotFound != err )
            goto HandleError;
    }
    else
    {
        CallS( err );
        
        Call( ErrIsamMakeKey(
                    ppib,
                    pfucbCatalog,
                    (BYTE *)&objidFDP,
                    sizeof(objidFDP),
                    JET_bitNewKey ) );
        Call( ErrIsamMakeKey(
                    ppib,
                    pfucbCatalog,
                    (BYTE *)&sysobj,
                    sizeof(sysobj),
                    JET_bitStrLimit ) );
        err = ErrIsamSetIndexRange( ppib, pfucbCatalog, JET_bitRangeUpperLimit );
        Assert( err <= 0 );
        while ( JET_errNoCurrentRecord != err )
        {
            Call( err );
            
            Call( (*pfnDump)( ppib, pfucbCatalog, pfnCallback, pvCallback ) );
            err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
        }
    }

    err = JET_errSuccess;

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}

#endif // !MINIMAL_FUNCTIONALITY

//  ================================================================
LOCAL ERR ErrDBUTLDumpPageUsage( JET_SESID sesid, const JET_DBUTIL_W * pdbutil )
//  ================================================================
{
    ERR             err;
    PIB *           ppib        = (PIB *)sesid;
    const FUCB *    pfucb       = (FUCB *)pdbutil->tableid;
    PGNO            pgnoFDP;

    CallR( ErrPIBCheck( ppib ) );

    //  normally we'd dispatch API calls, so this check wouldn't
    //  be needed, but since this is just some hack functionality
    //  I'm throwing in, it's easier just to bypass the dispatch layer
    //
    if ( JET_tableidNil == pdbutil->tableid || pfucbNil == pfucb )
    {
        CallR( ErrERRCheck( JET_errInvalidTableId ) );
    }

    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );

    if ( pdbutil->grbitOptions & JET_bitDBUtilOptionDumpLVPageUsage )
    {
        //  LV FCB may not yet be linked into the TDB, so just go to the catalog
        //
        Call( ErrCATAccessTableLV( ppib, pfucb->ifmp, pfucb->u.pfcb->ObjidFDP(), &pgnoFDP ) );
    }
    else
    {
        if ( pfucbNil != pfucb->pfucbCurIndex )
            pfucb = pfucb->pfucbCurIndex;

        pgnoFDP = pfucb->u.pfcb->PgnoFDP();
    }

    Call( ErrBTDumpPageUsage( ppib, pfucb->ifmp, pgnoFDP ) );

HandleError:
    return err;
}
#ifdef MINIMAL_FUNCTIONALITY
#else

//  ================================================================
LOCAL ERR ErrDBUTLDump( JET_SESID sesid, const JET_DBUTIL_W *pdbutil )
//  ================================================================
{
    ERR     err;

    switch( pdbutil->edbdump )
    {
        case opEDBDumpTables:
        {
            PFNTABLE const pfntable = (PFNTABLE)( pdbutil->pfnCallback );
            err = ErrDBUTLDumpTables(
                        (PIB *)sesid,
                        IFMP( pdbutil->dbid ),
                        pdbutil->szTable,
                        pfntable,
                        pdbutil->pvCallback );
        }
            break;
        case opEDBDumpIndexes:
            err = ErrDBUTLDumpTableObjects(
                        (PIB *)sesid,
                        IFMP( pdbutil->dbid ),
                        (OBJID)pdbutil->pgno,
                        sysobjIndex,
                        &ErrDBUTLDumpOneIndex,
                        pdbutil->pfnCallback,
                        pdbutil->pvCallback );
            break;

        case opEDBDumpColumns:
            err = ErrDBUTLDumpTableObjects(
                        (PIB *)sesid,
                        IFMP( pdbutil->dbid ),
                        OBJID( pdbutil->pgno ),
                        sysobjColumn,
                        &ErrDBUTLDumpOneColumn,
                        pdbutil->pfnCallback,
                        pdbutil->pvCallback );
            break;
        case opEDBDumpCallbacks:
            err = ErrDBUTLDumpTableObjects(
                        (PIB *)sesid,
                        IFMP( pdbutil->dbid ),
                        OBJID( pdbutil->pgno ),
                        sysobjCallback,
                        &ErrDBUTLDumpOneCallback,
                        pdbutil->pfnCallback,
                        pdbutil->pvCallback );
            break;
        case opEDBDumpPage:
        {
            PFNPAGE const pfnpage = (PFNPAGE)( pdbutil->pfnCallback );
            err = ErrDBUTLDumpPage( (PIB *)sesid, (IFMP) pdbutil->dbid, pdbutil->pgno, pfnpage, pdbutil->pvCallback );
        }
            break;

        default:
            Assert( fFalse );
            err = ErrERRCheck( JET_errFeatureNotAvailable );
            break;
    }
    
    return err;
}


//  ================================================================
LOCAL ERR ErrDBUTLDumpTables( DBCCINFO *pdbccinfo, PFNTABLE pfntable, VOID * pvCtx )
//  ================================================================
{
    JET_SESID   sesid           = (JET_SESID)pdbccinfo->ppib;
    JET_DBID    dbid            = (JET_DBID)pdbccinfo->ifmp;
    JET_DBUTIL_W    dbutil;

    memset( &dbutil, 0, sizeof( dbutil ) );

    dbutil.cbStruct     = sizeof( dbutil );
    dbutil.op           = opDBUTILEDBDump;
    dbutil.sesid        = sesid;
    dbutil.dbid         = dbid;
    dbutil.pfnCallback  = (void *)pfntable;
    dbutil.pvCallback   = pvCtx ? pvCtx : &dbutil;
    dbutil.edbdump      = opEDBDumpTables;
    dbutil.grbitOptions = pdbccinfo->grbitOptions;
    
    dbutil.szTable      = ( NULL == pdbccinfo->wszTable || L'\0' == pdbccinfo->wszTable[0] ?
                                NULL :
                                pdbccinfo->wszTable );

    return ErrDBUTLDump( sesid, &dbutil );
}


#endif // !MINIMAL_FUNCTIONALITY

//  ================================================================
LOCAL ERR ErrDBUTLDumpFlushMap( INST* const pinst, const WCHAR* const wszFlushMapFilePath, const FMPGNO fmpgno, const JET_GRBIT grbit )
//  ================================================================
{
    if ( grbit & JET_bitDBUtilOptionVerify )
    {
        return CFlushMapForDump::ErrChecksumFlushMapFile( pinst, wszFlushMapFilePath );
    }
    else
    {
        return CFlushMapForDump::ErrDumpFlushMapPage( pinst, wszFlushMapFilePath, fmpgno, grbit & JET_bitDBUtilOptionDumpVerbose );
    }
}

//  ================================================================
LOCAL ERR ErrDBUTLDumpSpaceCat( JET_SESID sesid, JET_DBUTIL_W *pdbutil )
//  ================================================================
{
    ERR err = JET_errSuccess;
    JET_DBID dbid = JET_dbidNil;
    BOOL fDbAttached = fFalse;
    BOOL fDbOpen = fFalse;

    if ( ( pdbutil->spcatOptions.pgnoFirst < 1 ) ||
            ( ( pdbutil->spcatOptions.pgnoLast != pgnoMax ) && ( pdbutil->spcatOptions.pgnoFirst > pdbutil->spcatOptions.pgnoLast ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Call( ErrIsamAttachDatabase(
            sesid,
            pdbutil->spcatOptions.szDatabase,
            fFalse,
            NULL,
            0,
            JET_bitDbReadOnly ) );
    fDbAttached = fTrue;

    Call( ErrIsamOpenDatabase(
            sesid,
            pdbutil->spcatOptions.szDatabase,
            NULL,
            &dbid,
            JET_bitDbExclusive | JET_bitDbReadOnly ) );
    fDbOpen = fTrue;

    if ( pdbutil->spcatOptions.pgnoLast == pgnoMax )
    {
        Call( g_rgfmp[ (IFMP)dbid ].ErrPgnoLastFileSystem( &( pdbutil->spcatOptions.pgnoLast ) ) );
    }
    pdbutil->spcatOptions.pgnoFirst = UlFunctionalMin( pdbutil->spcatOptions.pgnoFirst, pdbutil->spcatOptions.pgnoLast );
    Assert( pdbutil->spcatOptions.pgnoFirst >= 1 );
    Assert( pdbutil->spcatOptions.pgnoLast >= 1 );
    Assert( pdbutil->spcatOptions.pgnoFirst <= pdbutil->spcatOptions.pgnoLast );

    Call( ErrSPGetSpaceCategoryRange(
            (PIB*)sesid,
            (IFMP)dbid,
            pdbutil->spcatOptions.pgnoFirst,
            pdbutil->spcatOptions.pgnoLast,
            !!( pdbutil->grbitOptions & JET_bitDBUtilFullCategorization ),
            (JET_SPCATCALLBACK)pdbutil->spcatOptions.pfnSpaceCatCallback,
            pdbutil->spcatOptions.pvContext ) );

    err = JET_errSuccess;

HandleError:
    if ( fDbOpen )
    {
        (void)ErrIsamCloseDatabase( sesid, dbid, NO_GRBIT );
        fDbOpen = fFalse;
    }

    if ( fDbAttached )
    {
        (void)ErrIsamDetachDatabase( sesid, NULL, pdbutil->spcatOptions.szDatabase );
        fDbAttached = fFalse;
    }

    return err;
}

//  ================================================================
LOCAL ERR ErrDBUTLDumpCachedFileHeader( const WCHAR* const wszFilePath, const JET_GRBIT grbit )
//  ================================================================
{
    ERR                 err     = JET_errSuccess;
    IBlockCacheFactory* pbcf    = NULL;

    Call( COSBlockCacheFactory::ErrCreate( &pbcf ) );

    Call( pbcf->ErrDumpCachedFileHeader( wszFilePath, grbit, CPRINTFSTDOUT::PcprintfInstance() ) );

HandleError:
    delete pbcf;
    return err;
}

//  ================================================================
LOCAL ERR ErrDBUTLDumpCacheFile( const WCHAR* const wszFilePath, const JET_GRBIT grbit )
//  ================================================================
{
    ERR                 err     = JET_errSuccess;
    IBlockCacheFactory* pbcf    = NULL;

    Call( COSBlockCacheFactory::ErrCreate( &pbcf ) );

    Call( pbcf->ErrDumpCacheFile( wszFilePath, grbit, CPRINTFSTDOUT::PcprintfInstance() ) );

HandleError:
    delete pbcf;
    return err;
}

//  ================================================================
LOCAL VOID DBUTLIReportSpaceLeakEstimationSucceeded(
    const FMP* const pfmp,
    const CPG cpgAvailable,
    const CPG cpgOwnedBelowEof,
    const CPG cpgOwnedBeyondEof,
    const CPG cpgOwnedPrimary,
    const CPG cpgUsedRoot,
    const CPG cpgUsedOe,
    const CPG cpgUsedAe,
    const CPG cpgSplitBuffers,
    const ULONG cCachedPrimary,
    const ULONG cUncachedPrimary,
    const JET_THREADSTATS& jts,
    const ULONG ulMinElapsed,
    const double dblSecElapsed )
//  ================================================================
{
    const CPG cpgOwned = cpgOwnedBelowEof + cpgOwnedBeyondEof;
    Assert( cpgOwned > 0 );

    const CPG cpgUsed = cpgOwnedPrimary + cpgUsedRoot + cpgUsedOe + cpgUsedAe;
    Assert( ( cpgUsed <= cpgOwned ) || ( !pfmp->FReadOnlyAttach() && !pfmp->FExclusiveOpen() ) );

    CPG cpgLeaked = cpgOwned - ( cpgAvailable + cpgSplitBuffers + cpgUsed );
    Assert( ( cpgLeaked >= 0 ) || ( !pfmp->FReadOnlyAttach() && !pfmp->FExclusiveOpen() ) );
    cpgLeaked = LFunctionalMax( cpgLeaked, 0 );

    OSTraceSuspendGC();
    const WCHAR* rgwsz[] =
    {
        pfmp->WszDatabaseName(),
        OSFormatW( L"%d", cpgLeaked ), OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgLeaked ) ), OSFormatW( L"%.3f", ( 100.0 * (double)cpgLeaked ) / (double)cpgOwned ),
        OSFormatW( L"%d", cpgOwned ), OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgOwned ) ),
        OSFormatW( L"%d", cpgAvailable ), OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgAvailable ) ), OSFormatW( L"%.3f", ( 100.0 * (double)cpgAvailable ) / (double)cpgOwned ),
        OSFormatW( L"%d", cpgUsed ), OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgUsed ) ), OSFormatW( L"%.3f", ( 100.0 * (double)cpgUsed ) / (double)cpgOwned ),
        OSFormatW( L"%d", cpgOwnedBelowEof ), OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgOwnedBelowEof ) ), OSFormatW( L"%.3f", ( 100.0 * (double)cpgOwnedBelowEof ) / (double)cpgOwned ),
        OSFormatW( L"%d", cpgOwnedBeyondEof ), OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgOwnedBeyondEof ) ), OSFormatW( L"%.3f", ( 100.0 * (double)cpgOwnedBeyondEof ) / (double)cpgOwned ),
        OSFormatW( L"%d", cpgOwnedPrimary ), OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgOwnedPrimary ) ), OSFormatW( L"%.3f", ( 100.0 * (double)cpgOwnedPrimary ) / (double)cpgOwned ),
        OSFormatW( L"%d", cpgUsedRoot ), OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgUsedRoot ) ), OSFormatW( L"%.3f", ( 100.0 * (double)cpgUsedRoot ) / (double)cpgOwned ),
        OSFormatW( L"%d", cpgUsedOe ), OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgUsedOe ) ), OSFormatW( L"%.3f", ( 100.0 * (double)cpgUsedOe ) / (double)cpgOwned ),
        OSFormatW( L"%d", cpgUsedAe ), OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgUsedAe ) ), OSFormatW( L"%.3f", ( 100.0 * (double)cpgUsedAe ) / (double)cpgOwned ),
        OSFormatW( L"%d", cpgSplitBuffers ), OSFormatW( L"%I64u", pfmp->CbOfCpg( cpgSplitBuffers ) ), OSFormatW( L"%.3f", ( 100.0 * (double)cpgSplitBuffers ) / (double)cpgOwned ),
        OSFormatW( L"%u", cCachedPrimary ),
        OSFormatW( L"%u", cUncachedPrimary ),
        OSFormatW( L"%u", jts.cPageRead ), OSFormatW( L"%u", jts.cPagePreread ), OSFormatW( L"%u", jts.cPageReferenced ), OSFormatW( L"%u", jts.cPageDirtied ), OSFormatW( L"%u", jts.cPageRedirtied ),
        OSFormatW( L"%u", ulMinElapsed ), OSFormatW( L"%.3f", dblSecElapsed )
    };
    UtilReportEvent(
        eventInformation,
        SPACE_MANAGER_CATEGORY,
        ROOT_SPACE_LEAK_ESTIMATION_SUCCEEDED_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        pfmp->Pinst() );
    OSTraceResumeGC();
}

//  ================================================================
LOCAL VOID DBUTLIReportSpaceLeakEstimationFailed(
    const FMP* const pfmp,
    const ERR err,
    const OBJID objidLast,
    const CHAR* const szContext,
    const ULONG ulMinElapsed,
    const double dblSecElapsed )
//  ================================================================
{
    OSTraceSuspendGC();
    const WCHAR* rgwsz[] =
    {
        pfmp->WszDatabaseName(),
        OSFormatW( L"%d", err ),
        OSFormatW( L"%u", objidLast ),
        OSFormatW( L"%hs", szContext ),
        OSFormatW( L"%u", ulMinElapsed ), OSFormatW( L"%.3f", dblSecElapsed )
    };
    UtilReportEvent(
        eventError,
        SPACE_MANAGER_CATEGORY,
        ROOT_SPACE_LEAK_ESTIMATION_FAILED_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        pfmp->Pinst() );
    OSTraceResumeGC();
}

//  ================================================================
LOCAL ERR ErrDBUTLIEstimateRootSpaceLeak( PIB* const ppib, const IFMP ifmp )
//  ================================================================
{
    ERR err = JET_errSuccess;
    FMP* const pfmp = g_rgfmp + ifmp;
    const CHAR* szContext = NULL;
    const HRT hrtStart = HrtHRTCount();
    BOOL fRunning = fFalse;
    JET_THREADSTATS jtsStart = { 0 }, jtsEnd = { 0 };
    OBJID objidLast = objidNil;
    CPG cpgOwnedPrimary = 0;
    ULONG cCachedPrimary = 0, cUncachedPrimary = 0;
    CPG cpgUsedRoot = 0, cpgUsedOe = 0, cpgUsedAe = 0;
    CPG rgcpgRootSpaceInfo[ 4 ] = { 0 };
    FUCB* pfucbCatalog = pfucbNil;
    FUCB* pfucbSpaceTree = pfucbNil;
    FUCB* pfucbTable = pfucbNil;
    FUCB* pfucb = pfucbNil;

    szContext = "CheckInTrx";
    if ( ppib->Level() > 0 )
    {
        // Being in a transaction means we might miss some catalog entries that get
        // inserted while we process all the tables.
        Error( ErrERRCheck( JET_errInTransaction ) );
    }

    szContext = "CheckAlreadyRunning";
    Call( pfmp->ErrStartRootSpaceLeakEstimation() );
    fRunning = fTrue;

    szContext = "ThreadStatsStart";
    jtsStart.cbStruct = sizeof( jtsStart );
    jtsEnd.cbStruct = sizeof( jtsStart );
    Call( JetGetThreadStats( &jtsStart, jtsStart.cbStruct ) );

    szContext = "PrimaryObjects";
    OnDebug( OBJID objidPrev = objidNil );

    ppib->SetFSessionLeakReport();

    CHAR szObjectName[ JET_cbNameMost + 1 ];
    for ( err = ErrCATGetNextRootObject( ppib, ifmp, fTrue, &pfucbCatalog, &objidLast, szObjectName );
        ( err >= JET_errSuccess ) && ( objidLast != objidNil );
        err = ErrCATGetNextRootObject( ppib, ifmp, fTrue, &pfucbCatalog, &objidLast, szObjectName ) )
    {
#ifdef DEBUG
        Assert( objidLast != objidSystemRoot );  // Root object is not supposed to be returned here.
        Assert( objidLast > objidPrev );
        objidPrev = objidLast;

        // Test injection.
        while ( objidLast >= (OBJID)UlConfigOverrideInjection( 35366, objidFDPOverMax ) );
        Call( ErrFaultInjection( 55190 ) );
#endif // DEBUG

        CPG cpgPrimaryObject = cpgNil;

        // Check if it is cached.
        err = ErrCATGetExtentPageCounts( ppib, ifmp, objidLast, &cpgPrimaryObject, NULL );
        if ( err >= JET_errSuccess )
        {
            cCachedPrimary++;
            cpgOwnedPrimary += cpgPrimaryObject;
        }
        else
        {
            if ( ( err == JET_errRecordNotFound ) || ( err == JET_errNotInitialized ) )
            {
                err = JET_errSuccess;
            }
            Call( err );

            // Test injection.
            OnDebug( while ( objidLast >= (OBJID)UlConfigOverrideInjection( 48550, objidFDPOverMax ) ) );

            err = ErrFILEOpenTable( ppib, ifmp, &pfucbTable, szObjectName, JET_bitTableReadOnly | JET_bitTableTryPurgeOnClose );
            if ( err == JET_errObjectNotFound )
            {
                err = JET_errSuccess;
            }
            else
            {
                Call( err );

                cUncachedPrimary++;
                
                // Test injection.
                OnDebug( while ( objidLast >= (OBJID)UlConfigOverrideInjection( 57894, objidFDPOverMax ) ) );

                Call( ErrSPGetInfo(
                    ppib,
                    ifmp,
                    pfucbTable,
                    (BYTE*)&cpgPrimaryObject,
                    sizeof( cpgPrimaryObject ),
                    fSPOwnedExtent,
                    gci::Allow ) );

                cpgOwnedPrimary += cpgPrimaryObject;

                Call( ErrFILECloseTable( ppib, pfucbTable ) );
                pfucbTable = pfucbNil;
            }

            Assert( pfucbTable == pfucbNil );
        }
    }
    Call( err );
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;
    objidLast = objidSystemRoot;

    // Root space
    //
    szContext = "RootSpace";

    // Open root.
    Call( ErrBTIOpen( ppib, ifmp, pgnoSystemRoot, objidNil, openNormal, &pfucb, fFalse ) );
    Call( ErrBTIGotoRoot( pfucb, latchReadNoTouch ) );
    pfucb->pcsrRoot = Pcsr( pfucb );
    Assert( pfucb->u.pfcb->FSpaceInitialized() );

    // Root object.
    CPG rgcpgRootInfo[ 4 ] = { cpgNil };
    Call( ErrSPGetInfo(
        ppib,
        ifmp,
        pfucb,
        (BYTE*)rgcpgRootInfo,
        sizeof( rgcpgRootInfo ),
        fSPOwnedExtent | fSPAvailExtent | fSPSplitBuffers | fSPShelvedExtent,
        gci::Allow ) );
    Call( ErrSPGetInfo(
        ppib,
        ifmp,
        pfucb,
        (BYTE*)&cpgUsedRoot,
        sizeof( cpgUsedRoot ),
        fSPReachablePages,
        gci::Allow ) );
    Expected( cpgUsedRoot == 1 );

    // Root OE.
    szContext = "RootOe";
    Call( ErrSPIOpenOwnExt( pfucb, &pfucbSpaceTree ) );
    Call( ErrBTIGotoRoot( pfucbSpaceTree, latchReadNoTouch ) );
    pfucbSpaceTree->pcsrRoot = Pcsr( pfucbSpaceTree );
    Call( ErrSPGetInfo(
        ppib,
        ifmp,
        pfucbSpaceTree,
        (BYTE*)&cpgUsedOe,
        sizeof( cpgUsedOe ),
        fSPReachablePages,
        gci::Allow ) );
    pfucbSpaceTree->pcsrRoot = pcsrNil;
    BTClose( pfucbSpaceTree );
    pfucbSpaceTree = pfucbNil;

    // Root AE.
    szContext = "RootAe";
    Call( ErrSPIOpenAvailExt( pfucb, &pfucbSpaceTree ) );
    Call( ErrBTIGotoRoot( pfucbSpaceTree, latchReadNoTouch ) );
    pfucbSpaceTree->pcsrRoot = Pcsr( pfucbSpaceTree );
    Call( ErrSPGetInfo(
        ppib,
        ifmp,
        pfucbSpaceTree,
        (BYTE*)&cpgUsedAe,
        sizeof( cpgUsedAe ),
        fSPReachablePages,
        gci::Allow ) );
    pfucbSpaceTree->pcsrRoot = pcsrNil;
    BTClose( pfucbSpaceTree );
    pfucbSpaceTree = pfucbNil;

    // Root space.
    szContext = "RootSpace";
    Call( ErrSPGetInfo(
        ppib,
        ifmp,
        pfucb,
        (BYTE*)rgcpgRootSpaceInfo,
        sizeof( rgcpgRootSpaceInfo ),
        fSPOwnedExtent | fSPAvailExtent | fSPSplitBuffers | fSPShelvedExtent,
        gci::Allow ) );

    // Close root.
    pfucb->pcsrRoot = pcsrNil;
    BTClose( pfucb );
    pfucb = pfucbNil;

    // We are done with the part that requires exclusivity.
    pfmp->StopRootSpaceLeakEstimation();
    fRunning = fFalse;

    szContext = "ThreadStatsEnd";
    Call( JetGetThreadStats( &jtsEnd, jtsEnd.cbStruct ) );

HandleError:
    if ( fRunning )
    {
        pfmp->StopRootSpaceLeakEstimation();
    }

    if ( pfucbCatalog != pfucbNil )
    {
        Assert( err < JET_errSuccess );
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;
    }

    if ( pfucbSpaceTree != pfucbNil )
    {
        Assert( err < JET_errSuccess );
        pfucbSpaceTree->pcsrRoot = pcsrNil;
        BTClose( pfucbSpaceTree );
        pfucbSpaceTree = pfucbNil;
    }

    if ( pfucbTable != pfucbNil )
    {
        Assert( err < JET_errSuccess );
        CallS( ErrFILECloseTable( ppib, pfucbTable ) );
        pfucbTable = pfucbNil;
    }

    if ( pfucb != pfucbNil )
    {
        Assert( err < JET_errSuccess );
        pfucb->pcsrRoot = pcsrNil;
        BTClose( pfucb );
        pfucb = pfucbNil;
    }

    ppib->ResetFSessionLeakReport();

    const double dblSecTotalElapsed = DblHRTSecondsElapsed( DhrtHRTElapsedFromHrtStart( hrtStart ) );
    const ULONG ulMinElapsed = (ULONG)( dblSecTotalElapsed / 60.0 );
    const double dblSecElapsed = dblSecTotalElapsed - (double)ulMinElapsed * 60.0;
    if ( err >= JET_errSuccess )
    {
        JET_THREADSTATS jts = { 0 };
        jts.cPageRead = jtsEnd.cPageRead - jtsStart.cPageRead;
        jts.cPagePreread = jtsEnd.cPagePreread - jtsStart.cPagePreread;
        jts.cPageReferenced = jtsEnd.cPageReferenced - jtsStart.cPageReferenced;
        jts.cPageDirtied = jtsEnd.cPageDirtied - jtsStart.cPageDirtied;
        jts.cPageRedirtied = jtsEnd.cPageRedirtied - jtsStart.cPageRedirtied;

        DBUTLIReportSpaceLeakEstimationSucceeded(
            pfmp,
            rgcpgRootSpaceInfo[ 1 ],    // cpgAvailable
            rgcpgRootSpaceInfo[ 0 ],    // cpgOwnedBelowEof
            rgcpgRootSpaceInfo[ 3 ],    // cpgOwnedBeyondEof (shelved)
            cpgOwnedPrimary,
            cpgUsedRoot,
            cpgUsedOe,
            cpgUsedAe,
            rgcpgRootSpaceInfo[ 2 ],    // cpgSplitBuffers
            cCachedPrimary,
            cUncachedPrimary,
            jts,
            ulMinElapsed,
            dblSecElapsed );

        err = JET_errSuccess;
    }
    else
    {
        Assert( err != JET_errObjectNotFound );
        Assert( err != JET_errRBSFDPToBeDeleted );

        DBUTLIReportSpaceLeakEstimationFailed(
            pfmp,
            err,
            objidLast,
            szContext,
            ulMinElapsed,
            dblSecElapsed );
    }

    return err;
}

BOOL g_fDisableDumpPrintF = fFalse;

//  ================================================================
ERR ISAMAPI ErrIsamDBUtilities( JET_SESID sesid, JET_DBUTIL_W *pdbutil )
//  ================================================================
{
    ERR     err     = JET_errSuccess;
    INST    *pinst  = PinstFromPpib( (PIB*)sesid );

    PIBTraceContextScope tcScope = ( (PIB*)sesid )->InitTraceContextScope();
    tcScope->nParentObjectClass = tceNone;

    //  verify input

    Assert( pdbutil );
    Assert( pdbutil->cbStruct == sizeof( JET_DBUTIL_W ) );


    if ( opDBUTILEDBDump               != pdbutil->op &&
         opDBUTILDumpPageUsage         != pdbutil->op &&
         opDBUTILChecksumLogFromMemory != pdbutil->op &&
         opDBUTILDumpSpaceCategory     != pdbutil->op && 
         opDBUTILDumpRBSPages          != pdbutil->op &&
         opDBUTILEstimateRootSpaceLeak != pdbutil->op )
    {
        //  the current operation requires szDatabase != NULL

        if ( NULL == pdbutil->szDatabase || L'\0' == pdbutil->szDatabase[0] )
        {
            return ErrERRCheck( JET_errDatabaseInvalidName );
        }
    }

    if ( opDBUTILDumpRBSPages == pdbutil->op )
    {
        //  the current operation requires szDatabase != NULL

        if ( NULL == pdbutil->rbsOptions.szDatabase || L'\0' == pdbutil->rbsOptions.szDatabase[0] )
        {
            return ErrERRCheck( JET_errDatabaseInvalidName );
        }
    }

    if ( opDBUTILChecksumLogFromMemory == pdbutil->op )
    {
        if ( pinst->m_plog->FLGFileOpened() || !pinst->FComputeLogDisabled() )
        {
            // calling this function when there is already a log attached
            // to the instance is not supported.
            return ErrERRCheck( JET_errInvalidParameter );
        }
    }

    if ( opDBUTILEstimateRootSpaceLeak == pdbutil->op )
    {
        // We need a JET_DBID, not a DB name.
        if ( ( pdbutil->szDatabase != NULL ) || ( pdbutil->dbid == JET_dbidNil ) )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
    }

    if ( ( pdbutil->grbitOptions & JET_bitDBUtilOptionSuppressConsoleOutput ) != 0 )
    {
        g_fDisableDumpPrintF = fTrue;
    }

    //  dispatch the work

    switch ( pdbutil->op )
    {
#ifdef MINIMAL_FUNCTIONALITY
#else

        case opDBUTILChecksumLogFromMemory:
            return ErrDUMPLogFromMemory( pinst,
                                         pdbutil->checksumlogfrommemory.szLog,
                                         pdbutil->checksumlogfrommemory.pvBuffer,
                                         pdbutil->checksumlogfrommemory.cbBuffer );

        case opDBUTILDumpLogfile:
            // szIntegPrefix being overloaded as the variable to put the CSV file name in.
            // lGeneration is the starting log generation to dump.
            // isec is being overloaded with the ending log generation to dump.
            return ErrDUMPLog( pinst, pdbutil->szDatabase, pdbutil->lGeneration, pdbutil->isec, pdbutil->grbitOptions, pdbutil->szIntegPrefix );

        case opDBUTILDumpFlushMapFile:
            return ErrDBUTLDumpFlushMap( pinst, pdbutil->szDatabase, pdbutil->pgno, pdbutil->grbitOptions );

        case opDBUTILDumpLogfileTrackNode:
            return JET_errSuccess;
            
        case opDBUTILEDBDump:
            return ErrDBUTLDump( sesid, pdbutil );
        case opDBUTILDumpNode:
            return ErrDBUTLDumpNode( sesid, pinst->m_pfsapi, pdbutil->szDatabase, pdbutil->pgno, pdbutil->iline, pdbutil->grbitOptions );
        case opDBUTILDumpTag:
            return ErrDBUTLDumpTag( sesid, pinst->m_pfsapi, pdbutil->szDatabase, pdbutil->pgno, pdbutil->iline, pdbutil->grbitOptions );
#ifdef DEBUG
        case opDBUTILSetHeaderState:
            return ErrDUMPFixupHeader( pinst, pdbutil->szDatabase, pdbutil->grbitOptions & JET_bitDBUtilOptionDumpVerbose );
#endif // DEBUG
        case opDBUTILDumpPage:
            return ErrDBUTLDumpPage( pinst, pdbutil->szDatabase, pdbutil->pgno, pdbutil->szIndex, pdbutil->szTable, pdbutil->grbitOptions );

        case opDBUTILDumpHeader:
            return ErrDUMPHeader( pinst, pdbutil->szDatabase, pdbutil->grbitOptions & JET_bitDBUtilOptionDumpVerbose );

        case opDBUTILDumpCheckpoint:
            return ErrDUMPCheckpoint( pinst, pdbutil->szDatabase );

        case opDBUTILDumpCachedFileHeader:
            return ErrDBUTLDumpCachedFileHeader( pdbutil->szDatabase, pdbutil->grbitOptions );

        case opDBUTILDumpCacheFile:
            return ErrDBUTLDumpCacheFile( pdbutil->szDatabase, pdbutil->grbitOptions );

#endif  // !MINIMAL_FUNCTIONALITY

        case opDBUTILEDBRepair:
            return ErrDBUTLRepair( sesid, pdbutil, CPRINTFSTDOUT::PcprintfInstance() );

#ifdef MINIMAL_FUNCTIONALITY
#else

        case opDBUTILEDBScrub:
            return ErrDBUTLScrub( sesid, pdbutil );

        case opDBUTILDumpData:
            return ErrESEDUMPData( sesid, pdbutil );

        case opDBUTILDumpPageUsage:
            return ErrDBUTLDumpPageUsage( sesid, pdbutil );

#endif  // !MINIMAL_FUNCTIONALITY

        case opDBUTILDBDefragment:
        {
            ERR errDetach;

            if ( (ULONG)UlParam( pinst, JET_paramEngineFormatVersion ) == JET_efvUsePersistedFormat )
            {
                DBFILEHDR * pdbfilehdr = NULL;
                JET_ENGINEFORMATVERSION efvSourceDb = JET_efvExchange55Rtm;

                AllocR( pdbfilehdr = (DBFILEHDR * )PvOSMemoryPageAlloc( g_cbPage, NULL ) );
                memset( pdbfilehdr, 0, g_cbPage );

                IFileAPI * pfapi = NULL;
                if ( CIOFilePerf::ErrFileOpen(
                                        pinst->m_pfsapi,
                                        pinst,
                                        pdbutil->szDatabase,
                                        (   IFileAPI::fmfReadOnly |
                                            ( BoolParam( JET_paramEnableFileCache ) ?
                                                IFileAPI::fmfCached :
                                                IFileAPI::fmfNone ) ),
                                        iofileDbAttached,
                                        qwDefragFileID,
                                        &pfapi ) >= JET_errSuccess )
                {
                    err = ErrUtilReadShadowedHeader(    pinst,
                                                        pinst->m_pfsapi,
                                                        pfapi,
                                                        (BYTE*)pdbfilehdr,
                                                        g_cbPage,
                                                        OffsetOf( DBFILEHDR, le_cbPageSize ),
                                                        urhfReadOnly );
                    if ( err >= JET_errSuccess )
                    {
                        const FormatVersions * pfmtvers = NULL;
                        err = ErrDBFindHighestMatchingDbMajors( pdbfilehdr->Dbv(), &pfmtvers, fTrue /* allow bad version, client may try to defrag too high version */ );
                        if ( err >= JET_errSuccess )
                        {
                            efvSourceDb = pfmtvers->efv;
                        }
                    }
                    delete pfapi;
                }
                
                CallR( err );

                Assert( efvSourceDb != JET_efvUsePersistedFormat );
                Assert( efvSourceDb != JET_efvExchange55Rtm );

                err = SetParam( pinst, NULL, JET_paramEngineFormatVersion, efvSourceDb, NULL );
            }
            Assert( JET_efvUsePersistedFormat != UlParam( pinst, JET_paramEngineFormatVersion ) );

            err = ErrIsamAttachDatabase(    sesid,
                                            pdbutil->szDatabase,
                                            fFalse,
                                            NULL,
                                            0,
                                            JET_bitDbReadOnly );
            if ( JET_errSuccess != err )
            {
                return err;
            }

            err = ErrIsamCompact(           sesid,
                                            pdbutil->szDatabase,
                                            pinst->m_pfsapi,
                                            pdbutil->szTable,
                                            JET_PFNSTATUS( pdbutil->pfnCallback ),
                                            NULL,
                                            pdbutil->grbitOptions );

            errDetach = ErrIsamDetachDatabase( sesid, NULL, pdbutil->szDatabase );

            if ( err >= JET_errSuccess && errDetach < JET_errSuccess )
            {
                err = errDetach;
            }

            return err;
        }

        case opDBUTILDBTrim:
        {
            ERR errDetach;

            err = ErrIsamAttachDatabase(    sesid,
                                            pdbutil->szDatabase,
                                            fFalse,
                                            NULL,
                                            0,
                                            JET_bitDbExclusive );
            if ( JET_errSuccess != err )
            {
                return err;
            }

            err = ErrIsamTrimDatabase( sesid,
                                       pdbutil->szDatabase,
                                       pinst->m_pfsapi,
                                       CPRINTFSTDOUT::PcprintfInstance(),
                                       pdbutil->grbitOptions );

            errDetach = ErrIsamDetachDatabase( sesid, NULL, pdbutil->szDatabase );

            if ( err >= JET_errSuccess && errDetach < JET_errSuccess )
            {
                err = errDetach;
            }

            return err;
        }

        case opDBUTILDumpSpaceCategory:
            return ErrDBUTLDumpSpaceCat( sesid, pdbutil );
                    
        case opDBUTILDumpRBSPages:
            return ErrDUMPRBSPage( pinst, pdbutil->rbsOptions.szDatabase, pdbutil->rbsOptions.pgnoFirst, pdbutil->rbsOptions.pgnoLast, pdbutil->grbitOptions & JET_bitDBUtilOptionDumpVerbose );

        case opDBUTILDumpRBSHeader:
            return ErrDUMPRBSHeader( pinst, pdbutil->szDatabase, pdbutil->grbitOptions & JET_bitDBUtilOptionDumpVerbose );

        case opDBUTILEstimateRootSpaceLeak:
            return ErrDBUTLIEstimateRootSpaceLeak( (PIB*)sesid, (IFMP)pdbutil->dbid );
    }


#ifdef MINIMAL_FUNCTIONALITY
#else

    DBCCINFO    dbccinfo;
    JET_DBID    dbid        = JET_dbidNil;
    JET_GRBIT   grbitAttach;

    //  setup DBCCINFO

    memset( &dbccinfo, 0, sizeof(DBCCINFO) );
    dbccinfo.tableidPageInfo    = JET_tableidNil;
    dbccinfo.tableidSpaceInfo   = JET_tableidNil;

    //  default to the consistency checker

    dbccinfo.op = opDBUTILConsistency;

    switch ( pdbutil->op )
    {
#ifdef DEBUG
        case opDBUTILMunge:
#endif  //  DEBUG
        case opDBUTILDumpMetaData:
        case opDBUTILDumpSpace:
            dbccinfo.op = pdbutil->op;
            break;
    }

    //  copy object names

    Assert( NULL != pdbutil->szDatabase );
    OSStrCbCopyW( dbccinfo.wszDatabase, sizeof(dbccinfo.wszDatabase), pdbutil->szDatabase );

    if ( NULL != pdbutil->szTable )
    {
        OSStrCbCopyW( dbccinfo.wszTable, sizeof(dbccinfo.wszTable), pdbutil->szTable );
    }
    if ( NULL != pdbutil->szIndex )
    {
        OSStrCbCopyW( dbccinfo.wszIndex, sizeof(dbccinfo.wszIndex), pdbutil->szIndex );
    }

    //  propagate the grbit

    if ( pdbutil->grbitOptions & JET_bitDBUtilOptionStats )
    {
        dbccinfo.grbitOptions |= JET_bitDBUtilOptionStats;
    }
    if ( pdbutil->grbitOptions & JET_bitDBUtilOptionDumpVerbose )
    {
        dbccinfo.grbitOptions |= JET_bitDBUtilOptionDumpVerbose;
    }
    
    //  attach/open database, table and index

    grbitAttach = ( opDBUTILMunge == dbccinfo.op ) ? 0 : JET_bitDbReadOnly;
    CallR( ErrIsamAttachDatabase(   sesid,
                                    dbccinfo.wszDatabase,
                                    fFalse,
                                    NULL,
                                    0,
                                    grbitAttach ) );
    Assert( JET_wrnDatabaseAttached != err );   // Since logging/recovery is disabled.

    Call( ErrIsamOpenDatabase(      sesid,
                                    dbccinfo.wszDatabase,
                                    NULL,
                                    &dbid,
                                    grbitAttach ) );

    //  copy the session and database handles

    dbccinfo.ppib = (PIB*)sesid;
    dbccinfo.ifmp = dbid;

    //  check database according to command line args/flags

    switch ( dbccinfo.op )
    {
        case opDBUTILConsistency:
            Call( ErrERRCheck( JET_errFeatureNotAvailable ) );
            break;

        case opDBUTILDumpSpace:
        {
            if ( NULL == pdbutil->pfnCallback )
            {
                //  We are causing incompatibility if anyone actually utilizes space 
                //  dump.  If we really want to preserve, we can move the legacy space
                //  dump callbacks from eseutil\dbspacedump.cxx to here.
                Call( ErrERRCheck( JET_errFeatureNotAvailable ) );
            }

            Call( ErrOLDDumpMSysDefrag( dbccinfo.ppib, dbccinfo.ifmp ) );
            Call( ErrSCANDumpMSysScan( dbccinfo.ppib, dbccinfo.ifmp ) );
            Call( MSysDBM::ErrDumpTable( dbccinfo.ifmp ) );

            CBTreeStatsManager btsDbRootManager( pdbutil->grbitOptions, NULL /* DbRoot has no parent */ );
            BTREE_STATS * pbts = btsDbRootManager.Pbts();

            if ( pbts->pBasicCatalog )
            {
                BTREE_STATS_BASIC_CATALOG * pbtsBasicCatalog = pbts->pBasicCatalog;
                memset( pbtsBasicCatalog, 0, sizeof(*pbtsBasicCatalog) );
                pbtsBasicCatalog->cbStruct = sizeof(*pbtsBasicCatalog);
                pbtsBasicCatalog->eType = eBTreeTypeInternalDbRootSpace;
                (void)ErrOSStrCbCopyW( pbtsBasicCatalog->rgName, sizeof(pbtsBasicCatalog->rgName), dbccinfo.wszDatabase );
                pbtsBasicCatalog->objidFDP = objidSystemRoot;
                pbtsBasicCatalog->pgnoFDP = pgnoSystemRoot;
            }

            if ( pbts->pSpaceTrees )
            {
                CPRINTF * const pcprintf = ( pdbutil->grbitOptions & JET_bitDBUtilOptionDumpVerbose ) ?
                                                CPRINTFSTDOUT::PcprintfInstance() :
                                                NULL;
                CallR( ErrDBUTLGetSpaceTreeInfo(
                            dbccinfo.ppib,
                            dbccinfo.ifmp,
                            objidSystemRoot,
                            pgnoSystemRoot,
                            pbts->pSpaceTrees,
                            pcprintf ) );
            }

            if ( pbts->pParentOfLeaf )
            {
                pbts->pParentOfLeaf->cpgData = 1;   //  CPG of 1 for pgnoSystemRoot (pgno=1) ...
            }

            //  Trigger callback to user to consume the DbRoot information.
            //
            Call( ((JET_PFNSPACEDATA)pdbutil->pfnCallback)( pbts, (JET_API_PTR)pdbutil->pvCallback ) );
            
            //
            //  Setup the BT stats context for all children of DB root (primarily tables)
            //
            CBTreeStatsManager btsTableManager( pdbutil->grbitOptions, pbts );


            //
            //  [1,2]   Do enumeration of the DbRoot space trees.
            //

            if( pbts->pSpaceTrees )
            {
                Call( ErrDBUTLEnumSpaceTrees(
                            dbccinfo.ppib,
                            dbccinfo.ifmp,
                            objidSystemRoot,
                            btsTableManager.Pbts(),
                            (JET_PFNSPACEDATA)pdbutil->pfnCallback,
                            (JET_API_PTR)pdbutil->pvCallback ) );
            }

            //
            //  [3-n]   Setup the enumeration context for all tables (children of DB root)
            //

            DBUTIL_ENUM_SPACE_CTX dbues = { 0 };
            dbues.ppib = (JET_SESID)dbccinfo.ppib;
            dbues.ifmp = dbccinfo.ifmp;
            dbues.grbitDbUtilOptions = pdbutil->grbitOptions;
            dbues.wszSelectedTable = ( NULL == dbccinfo.wszTable || L'\0' == dbccinfo.wszTable[0] ?
                                        NULL :
                                        dbccinfo.wszTable );
            dbues.pbts = btsTableManager.Pbts();
            dbues.pfnBTreeStatsAnalysisFunc = (JET_PFNSPACEDATA)pdbutil->pfnCallback;
            dbues.pvBTreeStatsAnalysisFuncCtx = (JET_API_PTR) pdbutil->pvCallback;

            Call( ErrDBUTLDumpTables( &dbccinfo, ErrDBUTLEnumTableSpace, (VOID*)&dbues ) );
        }
            break;

        case opDBUTILDumpMetaData:
            if ( dbccinfo.grbitOptions & JET_bitDBUtilOptionDumpVerbose )
            {
                printf( "******************************* MSysLocales **********************************\n" );
                err = ErrCATDumpMSLocales( NULL, dbccinfo.ifmp );
                if ( err != JET_errSuccess )
                {
                    printf( "Failed to dump %hs table with: %d (continuing on ...).\n", szMSLocales, err );
                    // continue on ...
                }
                printf( "******************************************************************************\n" );
            }
            printf( "******************************* META-DATA DUMP *******************************\n" );

            if ( dbccinfo.grbitOptions & JET_bitDBUtilOptionDumpVerbose )
            {
                Call( ErrDBUTLDumpTables( &dbccinfo, PrintTableMetaData ) );
            }
            else
            {
                printf( "Name                                               Type    ObjidFDP    PgnoFDP             PgnoFDPLastSetTime\n" );
                printf( "=============================================================================================================\n" );
                printf( "%-51.5ws Db   ", dbccinfo.wszDatabase );
                DBUTLPrintfIntN( objidSystemRoot, 10 );
                printf( " " );
                DBUTLPrintfIntN( pgnoSystemRoot, 10 );
                printf( "\n\n" );

                Call( ErrDBUTLDumpTables( &dbccinfo, PrintTableBareMetaData ) );
            }

            printf( "******************************************************************************\n" );
            break;

        default:
            err = ErrERRCheck( JET_errFeatureNotAvailable );
            Call( err );
            break;
    }

    //  terminate
HandleError:
    if ( JET_tableidNil != dbccinfo.tableidPageInfo )
    {
        Assert( dbccinfo.grbitOptions & JET_bitDBUtilOptionPageDump );
        CallS( ErrDispCloseTable( (JET_SESID)dbccinfo.ppib, dbccinfo.tableidPageInfo ) );
        dbccinfo.tableidPageInfo = JET_tableidNil;
    }
    
    if ( JET_dbidNil != dbid )
    {
        (VOID)ErrIsamCloseDatabase( sesid, dbid, 0 );
    }

    (VOID)ErrIsamDetachDatabase( sesid, NULL, dbccinfo.wszDatabase );

    fflush( stdout );
#endif // !MINIMAL_FUNCTIONALITY
    
    return err;
}

