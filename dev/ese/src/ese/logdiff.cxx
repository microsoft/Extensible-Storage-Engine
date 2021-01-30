// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "PageSizeClean.hxx"

class TAGFIELDS_DUMP : public TAGFIELDS
{
    ULONG m_itagfld;

    enum { iInvalidState = -1 };

    const DATA* m_pData;
public:
    TAGFIELDS_DUMP( const DATA& dataRec ) : TAGFIELDS( dataRec ) { m_itagfld = (ULONG)iInvalidState; m_pData = &dataRec; }
    virtual ~TAGFIELDS_DUMP() {}

    BOOL FIsValidTagField() const { return ( m_itagfld != iInvalidState )? fTrue: fFalse; }
    const BYTE *PbTagColumnsHeader() const { return (const BYTE *)TAGFIELDS::PbTaggedColumns(); }
    ULONG CbTagColumnsHeaderSize() const { return TAGFIELDS::CTaggedColumns()*sizeof( TAGFLD ); }
    FID GetFID() const { Assert( FIsValidTagField() ); return TAGFIELDS::Fid( m_itagfld ); }
    const BYTE *PbData() const { Assert( FIsValidTagField() ); return ( const BYTE *)TAGFIELDS::PbData( m_itagfld ); }
    ULONG CbData() const { Assert( FIsValidTagField() ); return TAGFIELDS::CbData( m_itagfld ); }
    VOID FindFirstTagField() { m_itagfld = TAGFIELDS::CTaggedColumns() != 0? 0: iInvalidState; }
    VOID FindNextTagField() { Assert( FIsValidTagField() ); m_itagfld = ( m_itagfld+1 < TAGFIELDS::CTaggedColumns() )? m_itagfld+1: iInvalidState; }
    SIZE_T IbDataOffset() const { Assert( FIsValidTagField() ); return PbData() - (const BYTE *)m_pData->Pv(); }
    SIZE_T CbRestData() const { Assert( FIsValidTagField() ); return m_pData->Cb() - IbDataOffset(); }
    INT Compare( const TAGFIELDS_DUMP &tfd ) const { Assert( FIsValidTagField() ); Assert( tfd.FIsValidTagField() ); return TAGFIELDS::TagFldCmp( Ptagfld( m_itagfld ), tfd.Ptagfld( tfd.m_itagfld ) ); }
};



VOID LGDumpDiff( const LOG* const plog, const LR * const plr, CPRINTF * const pcprintf, const ULONG cbIndent )
{
    const LRREPLACE * const plrReplaceD = (LRREPLACE *) plr;
    const BYTE * const      pbDiff      = (BYTE *) plrReplaceD + sizeof( LRREPLACE );
    const BYTE *            pbDiffCur   = pbDiff;
    const BYTE *            pbDiffMax   = pbDiff + plrReplaceD->Cb();
    const BOOL              fDiff2      = plrReplaceD->FDiff2();
    const UINT              iVerbosityLevel = ( plog != NULL ) ? plog->IDumpVerbosityLevel() : LOG::ldvlMax;

#ifndef DEBUGGER_EXTENSION
    Assert( plog != NULL );
#endif

    while ( pbDiffCur < pbDiffMax )
    {
        INT         ibOffsetOld;
        INT         cbDataNew;
        DIFFHDR2    diffhdr2    = fDiff2 ? *( ( DIFFHDR2* )pbDiffCur ) : DIFFHDR2( *( ( DIFFHDR* )pbDiffCur ) );

        (*pcprintf)(
            "%*sReplDiff  %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],objid:[%u:%lu],",
            cbIndent,
            "",
            (DBTIME) plrReplaceD->le_dbtime,
            (DBTIME) plrReplaceD->le_dbtimeBefore,
            (TRX) plrReplaceD->le_trxBegin0,
            (USHORT) plrReplaceD->level,
            (PROCID) plrReplaceD->le_procid,
            (USHORT) plrReplaceD->dbid,
            (PGNO) plrReplaceD->le_pgno,
            plrReplaceD->ILine(),
            (USHORT) plrReplaceD->dbid,
            (OBJID) plrReplaceD->le_objidFDP );

        pbDiffCur += fDiff2 ? sizeof( diffhdr2 ) : sizeof( DIFFHDR );

        ibOffsetOld = diffhdr2.Ib();
        (*pcprintf)( "[ Offs:%u ", ibOffsetOld );

        if ( diffhdr2.FUseTwoBytesForDataLength() )
        {
            (*pcprintf)( "2B " );
            cbDataNew = *( (UnalignedLittleEndian< WORD >*) pbDiffCur );
            pbDiffCur += sizeof( WORD );
        }
        else
        {
            (*pcprintf)( "1B " );
            cbDataNew = *( (UnalignedLittleEndian< BYTE >*) pbDiffCur );
            pbDiffCur += sizeof( BYTE );
        }

        if ( diffhdr2.FInsert() )
        {
            (*pcprintf)( "InsertWithValue (%u): ", cbDataNew );
            ShowData( pbDiffCur, cbDataNew, iVerbosityLevel, pcprintf );
            pbDiffCur += cbDataNew;
        }
        else
        {
            if ( diffhdr2.FReplaceWithSameLength() )
            {
                (*pcprintf)( "ReplaceWithSameLength (%u): ", cbDataNew );
                ShowData( pbDiffCur, cbDataNew, iVerbosityLevel, pcprintf );
                pbDiffCur += cbDataNew;
            }
            else
            {
                INT cbDataOld;

                if ( diffhdr2.FUseTwoBytesForDataLength() )
                {
                    cbDataOld = *( (UnalignedLittleEndian< WORD >*) pbDiffCur );
                    pbDiffCur += sizeof( WORD );
                }
                else
                {
                    cbDataOld = *( (UnalignedLittleEndian< BYTE >*) pbDiffCur );
                    pbDiffCur += sizeof( BYTE );
                }

                (*pcprintf)( "ReplaceWithNewValue (%u=>%u): ", cbDataOld, cbDataNew );
                ShowData( pbDiffCur, cbDataNew, iVerbosityLevel, pcprintf );
                pbDiffCur += cbDataNew;
            }
        }

        (*pcprintf)( "])\n" );

        Assert( pbDiffCur <= pbDiffMax );
    }
}



BOOL FLGAppendDiff(
    const IFMP      ifmp,
    __deref BYTE    **ppbCur,
    BYTE            *pbMax,
    const SIZE_T    ibOffsetOld,
    const SIZE_T    cbDataOld,
    const SIZE_T    cbDataNew,
    const BYTE      *pbDataNew )
{

    Assert( pbNil != pbDataNew || cbDataNew == 0 );
    Assert( ibOffsetOld + cbDataOld >= ibOffsetOld );
    Assert( ibOffsetOld + cbDataOld <= (ULONG)g_rgfmp[ ifmp ].CbPage() );

    DIFFHDR     diffhdr;
    const WORD  wBitField   = WORD( ( ( ibOffsetOld << DIFFHDR::ibOffsetShf ) & DIFFHDR::ibOffsetMask ) |
                                (   cbDataOld != 0 && cbDataOld == cbDataNew ?
                                        DIFFHDR::fReplaceWithSameLength :
                                        0 ) |
                                (   cbDataOld == 0 ?
                                        DIFFHDR::fInsert :
                                        0 ) |
                                (   cbDataOld > 255 || cbDataNew > 255 ?
                                        DIFFHDR::fUseTwoBytesForDataLength :
                                        0 ) );

    diffhdr.m_bitfield = wBitField;


    Assert( !g_rgfmp[ ifmp ].FSmallPageDb() || SIZE_T( ( wBitField & DIFFHDR::ibOffsetMask ) >> DIFFHDR::ibOffsetShf ) == ibOffsetOld );


    const BOOL  fReplaceWithDiffLength  = !( wBitField & DIFFHDR::fReplaceWithSameLength )
                                            && !( wBitField & DIFFHDR::fInsert );
    const ULONG cbBytesForDataLength    = ( wBitField & DIFFHDR::fUseTwoBytesForDataLength ? 2 : 1 )
                                            * ( fReplaceWithDiffLength ? 2 : 1 );


    BYTE    * pbCur = *ppbCur;
    const size_t cbDiffHdr = g_rgfmp[ ifmp ].FSmallPageDb() ? sizeof( DIFFHDR ) : sizeof( DIFFHDR2 );
    if ( ( pbCur + cbDiffHdr  + cbBytesForDataLength + cbDataNew ) > pbMax )
    {
        return fFalse;
    }


    if ( sizeof( DIFFHDR ) == cbDiffHdr )
    {
        Assert( sizeof( diffhdr ) == cbDiffHdr );
        *( ( DIFFHDR* )pbCur ) = diffhdr;
    }
    else
    {
        DIFFHDR2 diffhdr2( diffhdr );
        diffhdr2.SetIb( ( WORD )ibOffsetOld );
        Assert( diffhdr2.Ib() == ibOffsetOld );

        Assert( sizeof( diffhdr2 ) == cbDiffHdr );
        *( ( DIFFHDR2* )pbCur ) = diffhdr2;
    }
    pbCur += cbDiffHdr;


    if ( wBitField & DIFFHDR::fUseTwoBytesForDataLength )
    {
        *( (UnalignedLittleEndian< WORD >*) pbCur ) = (WORD)cbDataNew;
        pbCur += sizeof( WORD );

        if ( fReplaceWithDiffLength )
        {
            *( (UnalignedLittleEndian< WORD >*) pbCur ) = (WORD)cbDataOld;
            pbCur += sizeof( WORD );
        }
    }
    else
    {
        *( (UnalignedLittleEndian< BYTE >*) pbCur ) = (BYTE)cbDataNew;
        pbCur += sizeof( BYTE );

        if ( fReplaceWithDiffLength )
        {
            *( (UnalignedLittleEndian< BYTE >*) pbCur ) = (BYTE)cbDataOld;
            pbCur += sizeof( BYTE );
        }
    }


    UtilMemCpy( pbCur, pbDataNew, cbDataNew );
    pbCur += cbDataNew;


    *ppbCur = pbCur;
    return fTrue;
}

BOOL FLGSetArrayDiff(
    const IFMP          ifmp,
    const BYTE *        pbDataBegin,
    const BYTE * const  pbOld,
    const BYTE * const  pbNew,
    const ULONG         cbOld,
    const ULONG         cbNew,
    BYTE **             ppbDiff,
    BYTE *              pbDiffMaxTotal )
{
    Assert( pbOld != NULL || cbOld == 0 );
    Assert( pbNew != NULL || cbNew == 0 );
    Assert( ppbDiff != NULL && *ppbDiff != NULL );
    Assert( pbDiffMaxTotal != NULL );
    Assert( (cbOld % sizeof( DWORD )) == 0 );
    Assert( (cbNew % sizeof( DWORD )) == 0 );
    Assert( (sizeof( TAGFLD ) %sizeof( DWORD )) == 0 );

    ULONG cbMinSize = cbOld;
    BOOL fTruncate, fEndIsReached;
    fTruncate = fFalse;
    fEndIsReached = fFalse;
    if ( cbMinSize > cbNew )
    {
        cbMinSize = cbNew;
        fTruncate = fTrue;
    }
    else if ( cbMinSize == cbNew )
    {
        fEndIsReached = fTrue;
    }

    Unaligned< DWORD > *pdwOldCur   = ( Unaligned< DWORD > * )pbOld;
    Unaligned< DWORD > *pdwOldMax   = pdwOldCur + ( cbMinSize / sizeof( DWORD ) );
    Unaligned< DWORD > *pdwNewCur   = ( Unaligned< DWORD > * )pbNew;
    Assert ( (BYTE *)(pdwOldCur + 1) - (BYTE *)pdwOldCur == 4 );

    BYTE    *pbDiffCur = *ppbDiff;
    BYTE    *pbDiffMax = pbDiffCur + cbNew;

    while ( pdwOldCur < pdwOldMax )
    {
        if ( *pdwOldCur == *pdwNewCur )
        {
            pdwOldCur++;
            pdwNewCur++;
        }
        else
        {
            Assert( (const BYTE *)pdwOldCur >= pbOld );
            const SIZE_T ibOld      = (BYTE *)pdwOldCur - pbDataBegin;
            const BYTE  *pbNewData  = (BYTE *)pdwNewCur;
            SIZE_T      cbNewData   = sizeof( DWORD );
            pdwOldCur++;
            pdwNewCur++;
            while ( pdwOldCur < pdwOldMax && *pdwOldCur != *pdwNewCur )
            {
                cbNewData += sizeof( DWORD );
                pdwOldCur ++;
                pdwNewCur ++;
            }

            SIZE_T cbOldData = cbNewData;
            if ( pdwOldCur == pdwOldMax )
            {
                if ( fTruncate )
                {
                    cbOldData += cbOld - cbMinSize;
                }
                else
                {
                    cbNewData += cbNew - cbMinSize;
                }
                fEndIsReached = fTrue;
            }
            else
            {
                pdwOldCur++;
                pdwNewCur++;
            }

            if ( !FLGAppendDiff(
                    ifmp,
                    &pbDiffCur,
                    pbDiffMax,
                    ibOld,
                    cbOldData,
                    cbNewData,
                    pbNewData
                    ) )
            {
                goto LogAll;
            }
        }
    }

    Assert( pdwOldMax == pdwOldCur );

    if ( !fEndIsReached )
    {
        if ( !FLGAppendDiff(
                    ifmp,
                    &pbDiffCur,
                    pbDiffMax,
                    cbMinSize + pbOld - pbDataBegin,
                    cbOld - cbMinSize,
                    cbNew - cbMinSize,
                    (const BYTE *)pdwNewCur ) )
        {
            goto LogAll;
        }
    }
    *ppbDiff = pbDiffCur;
    return fTrue;

    LogAll:
    return FLGAppendDiff(
            ifmp,
            ppbDiff,
            pbDiffMaxTotal,
            pbOld - pbDataBegin,
            cbOld,
            cbNew,
            pbNew );
}



VOID LGSetColumnDiffs(
    FUCB        *pfucb,
    const DATA& dataNew,
    const DATA& dataOld,
    BYTE        *pbDiff,
    BOOL        *pfOverflow,
    SIZE_T      *pcbDiff )
{
    FID         fid;

    Assert( NULL != pcbDiff );
    *pfOverflow = fTrue;
    *pcbDiff = 0;

    Assert( pfucbNil != pfucb );
    Assert( FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );

    FCB     *pfcb = pfucb->u.pfcb;
    Assert( pfcbNil != pfcb );

    TDB     *ptdb = pfcb->Ptdb();
    Assert( ptdbNil != ptdb );


#ifdef DISABLE_LOGDIFF
    return;
#endif

    const REC   *precOld    = (REC *)dataOld.Pv();
    const ULONG cbRecOld    = dataOld.Cb();

    Assert( precNil != precOld );
    Assert( cbRecOld >= REC::cbRecordMin );
    Assert( cbRecOld <= (SIZE_T)REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
    Assert( cbRecOld <= (SIZE_T)REC::CbRecordMost( pfucb ) );

    const REC   *precNew    = (REC *)dataNew.Pv();
    const ULONG cbRecNew    = dataNew.Cb();

    Assert( precNil != precNew );
    Assert( cbRecNew >= REC::cbRecordMin );
    Assert( cbRecNew <= (SIZE_T)REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
    Assert( cbRecNew <= (SIZE_T)REC::CbRecordMost( pfucb ) );

    const FID   fidFixedLastInRecOld    = precOld->FidFixedLastInRec();
    const FID   fidFixedLastInRecNew    = precNew->FidFixedLastInRec();
    Assert( fidFixedLastInRecOld <= fidFixedLastInRecNew );

    const FID   fidVarLastInRecOld      = precOld->FidVarLastInRec();
    const FID   fidVarLastInRecNew      = precNew->FidVarLastInRec();
    Assert( fidVarLastInRecOld <= fidVarLastInRecNew );


    BYTE            *pbDiffCur              = pbDiff;
    BYTE            *pbDiffMax              = pbDiffCur + cbRecNew;

    if ( memcmp( (BYTE *)precOld, (BYTE *)precNew, REC::cbRecordHeader ) != 0 )
    {
        if ( !FLGAppendDiff(
                pfucb->ifmp,
                &pbDiffCur,
                pbDiffMax,
                0,
                REC::cbRecordHeader,
                REC::cbRecordHeader,
                (BYTE *)precNew ) )
        {
            return;
        }
    }


    BOOL        fLogFixedNullBitMap     = fFalse;

    const BYTE  *pbFixedNullBitMapOld   = precOld->PbFixedNullBitMap();
    const BYTE  *pbFixedNullBitMapNew   = precNew->PbFixedNullBitMap();

    Assert( pbFixedNullBitMapOld > (BYTE *)precOld );
    Assert( pbFixedNullBitMapOld <= (BYTE *)precOld + cbRecOld );
    Assert( pbFixedNullBitMapNew > (BYTE *)precNew );
    Assert( pbFixedNullBitMapNew <= (BYTE *)precNew + cbRecNew );

    pfcb->EnterDML();

    Assert( fidFixedLastInRecOld <= fidFixedLastInRecNew );
    for ( fid = fidFixedLeast; fid <= fidFixedLastInRecOld; fid++ )
    {
        if ( !FFUCBColumnSet( pfucb, fid ) )
        {
            continue;
        }



        const UINT  ifid            = fid - fidFixedLeast;
        const BOOL  fFieldNullOld   = FFixedNullBit( pbFixedNullBitMapOld + ifid/8, ifid );
        const BOOL  fFieldNullNew   = FFixedNullBit( pbFixedNullBitMapNew + ifid/8, ifid );

        if ( fFieldNullOld ^ fFieldNullNew )
        {
            fLogFixedNullBitMap = fTrue;
        }

        if ( !fFieldNullNew )
        {
            const BOOL  fTemplateColumn = ptdb->FFixedTemplateColumn( fid );
            const FIELD *pfield         = ptdb->PfieldFixed( ColumnidOfFid( fid, fTemplateColumn ) );

            if ( JET_coltypNil != pfield->coltyp )
            {
                if ( !FLGAppendDiff(
                        pfucb->ifmp,
                        &pbDiffCur,
                        pbDiffMax,
                        pfield->ibRecordOffset,
                        pfield->cbMaxLen,
                        pfield->cbMaxLen,
                        (BYTE *)precNew + pfield->ibRecordOffset ) )
                {
                    pfcb->LeaveDML();
                    return;
                }
            }
        }
    }

    pfcb->LeaveDML();

    if ( fidFixedLastInRecOld < fidFixedLastInRecNew )
    {
        Assert( fidFixedLastInRecNew >= fidFixedLeast );

        fLogFixedNullBitMap = fTrue;

        const REC::RECOFFSET    ibBitMapOffsetOld   = (REC::RECOFFSET)( pbFixedNullBitMapOld - (BYTE *)precOld );
        const REC::RECOFFSET    ibBitMapOffsetNew   = (REC::RECOFFSET)( pbFixedNullBitMapNew - (BYTE *)precNew );

        Assert( ibBitMapOffsetNew > ibBitMapOffsetOld );
        const SIZE_T cbToAppend = ibBitMapOffsetNew - ibBitMapOffsetOld;

        const BYTE  *pbToAppend = (BYTE *)precNew + ibBitMapOffsetOld;

        if ( !FLGAppendDiff(
                pfucb->ifmp,
                &pbDiffCur,
                pbDiffMax,
                ibBitMapOffsetOld,
                0,
                cbToAppend,
                pbToAppend
                ) )
        {
            return;
        }
    }


    if ( fLogFixedNullBitMap )
    {
        if ( !FLGAppendDiff(
            pfucb->ifmp,
            &pbDiffCur,
            pbDiffMax,
            pbFixedNullBitMapOld - (BYTE *)precOld,
            ( fidFixedLastInRecOld + 7 ) / 8,
            ( fidFixedLastInRecNew + 7 ) / 8,
            pbFixedNullBitMapNew ) )
        {
            return;
        }
    }

    const UnalignedLittleEndian<REC::VAROFFSET> *pibVarOffsOld  = precOld->PibVarOffsets();
    const UnalignedLittleEndian<REC::VAROFFSET> *pibVarOffsNew  = precNew->PibVarOffsets();
    const REC::RECOFFSET    ibVarDataOld    = (REC::RECOFFSET)( precOld->PbVarData() - (BYTE *)precOld );

    Assert( fidVarLastInRecOld <= fidVarLastInRecNew );
    for ( fid = fidVarLeast; fid <= fidVarLastInRecOld; fid++ )
    {
        const UINT  ifid    = fid - fidVarLeast;
        if ( pibVarOffsOld[ifid] != pibVarOffsNew[ifid] )
        {
            break;
        }
    }

    Assert( fid <= fidVarLastInRecNew || fidVarLastInRecOld == fidVarLastInRecNew );
    if ( fid <= fidVarLastInRecNew )
    {
        const UINT  ifid    = fid - fidVarLeast;

        if ( !FLGAppendDiff(
                pfucb->ifmp,
                &pbDiffCur,
                pbDiffMax,
                (BYTE*)( pibVarOffsOld + ifid ) - (BYTE *)precOld,
                ( fidVarLastInRecOld + 1 - fid ) * sizeof(REC::VAROFFSET),
                ( fidVarLastInRecNew + 1 - fid ) * sizeof(REC::VAROFFSET),
                (BYTE *)( pibVarOffsNew + ifid ) ) )
        {
            return;
        }
    }


    for ( fid = fidVarLeast; fid <= fidVarLastInRecOld; fid++ )
    {
        if ( !FFUCBColumnSet( pfucb, fid ) )
            continue;



        const REC::VAROFFSET    ibStartOfColumnOld  = precOld->IbVarOffsetStart( fid );
        const REC::VAROFFSET    ibEndOfColumnOld    = precOld->IbVarOffsetEnd( fid );
        const REC::VAROFFSET    ibStartOfColumnNew  = precNew->IbVarOffsetStart( fid );
        const REC::VAROFFSET    ibEndOfColumnNew    = precNew->IbVarOffsetEnd( fid );

        if ( !FLGAppendDiff(
                    pfucb->ifmp,
                    &pbDiffCur,
                    pbDiffMax,
                    ibVarDataOld + ibStartOfColumnOld,
                    ibEndOfColumnOld - ibStartOfColumnOld,
                    ibEndOfColumnNew - ibStartOfColumnNew,
                    precNew->PbVarData() + ibStartOfColumnNew ) )
        {
            return;
        }
    }

    Assert( fid == fidVarLastInRecOld + 1 );

    if ( fid <= fidVarLastInRecNew )
    {
        const REC::VAROFFSET    ibStartOfColumn = precNew->IbVarOffsetStart( fid );

        if ( !FLGAppendDiff(
                pfucb->ifmp,
                &pbDiffCur,
                pbDiffMax,
                ibVarDataOld + precOld->IbEndOfVarData(),
                0,
                precNew->IbEndOfVarData() - ibStartOfColumn,
                precNew->PbVarData() + ibStartOfColumn ) )
        {
            return;
        }

    }

    if ( FFUCBTagImplicitOp( pfucb ) )
    {
        SIZE_T ibTagOffsetOld = precOld->PbTaggedData() - (BYTE *)precOld;
        SIZE_T ibTagOffsetNew = precNew->PbTaggedData() - (BYTE *)precNew;
        if ( !FLGAppendDiff(
                    pfucb->ifmp,
                    &pbDiffCur,
                    pbDiffMax,
                    ibTagOffsetOld,
                    cbRecOld - ibTagOffsetOld,
                    cbRecNew - ibTagOffsetNew,
                    precNew->PbTaggedData() ) )
        {
            return;
        }
    }

    else if ( FFUCBTaggedColumnSet( pfucb ) )
    {
        TAGFIELDS_DUMP tfdOld( dataOld );
        TAGFIELDS_DUMP tfdNew( dataNew );

        if ( !FLGSetArrayDiff(
            pfucb->ifmp,
            (const BYTE *)precOld,
            tfdOld.PbTagColumnsHeader(),
            tfdNew.PbTagColumnsHeader(),
            tfdOld.CbTagColumnsHeaderSize(),
            tfdNew.CbTagColumnsHeaderSize(),
            &pbDiffCur,
            pbDiffMax ) )
        {
            return;
        }

        tfdOld.FindFirstTagField();
        tfdNew.FindFirstTagField();
        for ( ; tfdOld.FIsValidTagField() && tfdNew.FIsValidTagField(); )
        {
            const INT iFidOrder = tfdOld.Compare( tfdNew );
            if ( iFidOrder == 0 )
            {
                if ( FFUCBColumnSet( pfucb, tfdOld.GetFID() ) )
                {
                    const SIZE_T    cbTagFieldOld   = tfdOld.CbData();
                    const SIZE_T    cbTagFieldNew   = tfdNew.CbData();

                    if ( cbTagFieldNew != cbTagFieldOld
                        || memcmp( tfdOld.PbData(), tfdNew.PbData(), cbTagFieldNew ) != 0 )
                    {
                        if ( !FLGAppendDiff(
                                    pfucb->ifmp,
                                    &pbDiffCur,
                                    pbDiffMax,
                                    tfdOld.IbDataOffset(),
                                    cbTagFieldOld,
                                    cbTagFieldNew,
                                    tfdNew.PbData() ) )
                        {
                            return;
                        }
                    }
                }
                tfdOld.FindNextTagField();
                tfdNew.FindNextTagField();
            }
            else if ( iFidOrder > 0 )
            {
                if ( !FLGAppendDiff(
                            pfucb->ifmp,
                            &pbDiffCur,
                            pbDiffMax,
                            tfdOld.IbDataOffset(),
                            0,
                            tfdNew.CbData(),
                            tfdNew.PbData() ) )
                {
                    return;
                }

                tfdNew.FindNextTagField();
            }

            else
            {
                if ( !FLGAppendDiff(
                            pfucb->ifmp,
                            &pbDiffCur,
                            pbDiffMax,
                            tfdOld.IbDataOffset(),
                            tfdOld.CbData(),
                            0,
                            pbNil ) )
                {
                    return;
                }

                tfdOld.FindNextTagField();
            }
        }

        if ( tfdNew.FIsValidTagField() )
        {
            Assert( !tfdOld.FIsValidTagField() );

            if ( !FLGAppendDiff(
                        pfucb->ifmp,
                        &pbDiffCur,
                        pbDiffMax,
                        cbRecOld,
                        0,
                        tfdNew.CbRestData(),
                        tfdNew.PbData() ) )
            {
                return;
            }
        }

        else if ( tfdOld.FIsValidTagField() )
        {
            Assert( !tfdNew.FIsValidTagField() );

            if ( !FLGAppendDiff(
                        pfucb->ifmp,
                        &pbDiffCur,
                        pbDiffMax,
                        tfdOld.IbDataOffset(),
                        tfdOld.CbRestData(),
                        0,
                        pbNil ) )
            {
                return;
            }
        }

        else
        {
            Assert( !tfdOld.FIsValidTagField() );
            Assert( !tfdNew.FIsValidTagField() );
        }
    }

    if ( pbDiffCur == pbDiff )
    {
        Assert( *pcbDiff == 0 );

        *pfOverflow = fFalse;
        return;
    }

    *pfOverflow = fFalse;
    *pcbDiff = pbDiffCur - pbDiff;

#ifdef DEBUG
    Assert( *pcbDiff != 0 );

    BYTE *  pbAfterImage;
    BFAlloc( bfasTemporary, (VOID **)&pbAfterImage );

    SIZE_T  cbAfterImage;
    DATA    dataAfterImage;

    Assert( dataNew.Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
    Assert( dataNew.Cb() <= REC::CbRecordMost( pfucb ) );

    ERR errDebug = ErrLGGetAfterImage(
                                    pfucb->ifmp,
                                    pbDiff,
                                    *pcbDiff,
                                    (BYTE *)dataOld.Pv(),
                                    dataOld.Cb(),
                                    !g_rgfmp[ pfucb->ifmp ].FSmallPageDb(),
                                    pbAfterImage,
                                    &cbAfterImage );
    Assert( errDebug == JET_errSuccess );
    Assert( (SIZE_T)dataNew.Cb() == cbAfterImage );

    Assert( dataNew.Pv() == (BYTE *)precNew );

    dataAfterImage.SetPv( pbAfterImage );
    dataAfterImage.SetCb( cbAfterImage );

    if ( !FDataEqual( dataNew, dataAfterImage ) )
    {
        REC     *precAfterImage     = (REC *)pbAfterImage;
        DATA    dataNewT;


        Assert( memcmp( precAfterImage, precNew, REC::cbRecordHeader ) == 0 );

        dataNewT.SetPv( (BYTE *)precNew );
        dataNewT.SetCb( precNew->IbEndOfFixedData() - precNew->CbFixedNullBitMap() );
        dataAfterImage.SetPv( precAfterImage );
        dataAfterImage.SetCb( precAfterImage->IbEndOfFixedData() - precAfterImage->CbFixedNullBitMap() );
        Assert( FDataEqual( dataNewT, dataAfterImage ) );

        dataNewT.SetPv( (BYTE *)precNew + precNew->IbEndOfFixedData() - precNew->CbFixedNullBitMap() );
        dataNewT.SetCb( precNew->CbFixedNullBitMap() );
        dataAfterImage.SetPv( (BYTE *)precAfterImage + precAfterImage->IbEndOfFixedData() - precAfterImage->CbFixedNullBitMap() );
        dataAfterImage.SetCb( precAfterImage->CbFixedNullBitMap() );
        Assert( FDataEqual( dataNewT, dataAfterImage ) );

        dataNewT.SetPv( (BYTE *)precNew->PibVarOffsets() );
        dataNewT.SetCb( precNew->PbVarData() - (BYTE *)precNew->PibVarOffsets() );
        dataAfterImage.SetPv( (BYTE *)precAfterImage->PibVarOffsets() );
        dataAfterImage.SetCb( precAfterImage->PbVarData() - (BYTE *)precAfterImage->PibVarOffsets() );
        Assert( FDataEqual( dataNewT, dataAfterImage ) );

        dataNewT.SetPv( precNew->PbVarData() );
        dataNewT.SetCb( precNew->PbTaggedData() - precNew->PbVarData() );
        dataAfterImage.SetPv( precAfterImage->PbVarData() );
        dataAfterImage.SetCb( precAfterImage->PbTaggedData() - precAfterImage->PbVarData() );
        Assert( FDataEqual( dataNewT, dataAfterImage ) );

        dataNewT.SetPv( precNew->PbTaggedData() );
        dataNewT.SetCb( (BYTE *)precNew + dataNew.Cb() - precNew->PbTaggedData() );
        dataAfterImage.SetPv( precAfterImage->PbTaggedData() );
        dataAfterImage.SetCb( pbAfterImage + cbAfterImage - precAfterImage->PbTaggedData() );
        Assert( FDataEqual( dataNewT, dataAfterImage ) );

        Assert( fFalse );
    }

    BFFree( pbAfterImage );

#endif
}



VOID LGSetLVDiffs(
    FUCB        *pfucb,
    const DATA& dataNew,
    const DATA& dataOld,
    BYTE        *pbDiff,
    BOOL        *pfOverflow,
    SIZE_T      *pcbDiff )
{
    Assert( NULL != pcbDiff );
    *pcbDiff = 0;
    *pfOverflow = fTrue;

    Assert( pfucbNil != pfucb );
    Assert( pfcbNil != pfucb->u.pfcb );
    Assert( pfucb->u.pfcb->FTypeLV() );

#ifdef DISABLE_LOGDIFF
    return;
#endif

    INT cbMinSize = dataOld.Cb();
    BOOL fTruncate, fEndIsReached;
    fTruncate = fFalse;
    fEndIsReached = fFalse;
    if ( cbMinSize > dataNew.Cb() )
    {
        cbMinSize = dataNew.Cb();
        fTruncate = fTrue;
    }
    else if ( cbMinSize == dataNew.Cb() )
    {
        fEndIsReached = fTrue;
    }

    BYTE    *pbOldCur   = (BYTE *)dataOld.Pv();
    BYTE    *pbOldMax   = pbOldCur + ( cbMinSize - cbMinSize % sizeof( LONG ) );
    BYTE    *pbNewCur   = (BYTE *)dataNew.Pv();

    while ( pbOldCur < pbOldMax && *( Unaligned< LONG > * )pbOldCur == *( Unaligned< LONG > * )pbNewCur )
    {
        pbOldCur += sizeof( LONG );
        pbNewCur += sizeof( LONG );
    }

    pbOldMax += cbMinSize % sizeof( LONG );

    BYTE    *pbDiffCur = pbDiff;
    BYTE    *pbDiffMax = pbDiffCur + dataNew.Cb();

    while ( pbOldCur < pbOldMax )
    {
        if ( *pbOldCur == *pbNewCur )
        {
            pbOldCur++;
            pbNewCur++;
        }
        else
        {
            Assert( pbOldCur >= dataOld.Pv() );
            const SIZE_T ibOld      = pbOldCur - (BYTE *)dataOld.Pv();
            const BYTE  *pbNewData  = pbNewCur;
            SIZE_T      cbNewData   = 0;
            SIZE_T      cbT;
            do
            {
                if ( pbOldCur + sizeof( SIZE_T ) >= pbOldMax )
                    cbT = pbOldMax - pbOldCur;
                else if ( *(Unaligned< SIZE_T > *)pbOldCur == *(Unaligned< SIZE_T > *)pbNewCur )
                {
                    pbOldCur += sizeof( SIZE_T );
                    pbNewCur += sizeof( SIZE_T );
                    break;
                }
                else
                    cbT = sizeof( SIZE_T );

                cbNewData += cbT;
                pbOldCur += cbT;
                pbNewCur += cbT;
            }
            while ( pbOldCur < pbOldMax );

            SIZE_T cbOldData = cbNewData;
            if ( pbOldCur == pbOldMax )
            {
                if ( fTruncate )
                {
                    cbOldData += dataOld.Cb() - cbMinSize;
                }
                else
                {
                    cbNewData += dataNew.Cb() - cbMinSize;
                }
                fEndIsReached = fTrue;
            }

            if ( !FLGAppendDiff(
                    pfucb->ifmp,
                    &pbDiffCur,
                    pbDiffMax,
                    ibOld,
                    cbOldData,
                    cbNewData,
                    pbNewData
                    ) )
            {
                return;
            }
        }
    }

    Assert( pbOldMax == pbOldCur );

    if ( pbDiffCur == pbDiff || !fEndIsReached )
    {
        if ( pbDiffCur == pbDiff && dataNew.Cb() == dataOld.Cb() )
        {
            Assert( *pcbDiff == 0 );
            *pfOverflow = fFalse;
            return;
        }

        if ( !FLGAppendDiff(
                    pfucb->ifmp,
                    &pbDiffCur,
                    pbDiffMax,
                    cbMinSize,
                    dataOld.Cb() - cbMinSize,
                    dataNew.Cb() - cbMinSize,
                    pbNewCur ) )
        {
            return;
        }

    }

    *pfOverflow = fFalse;
    *pcbDiff = pbDiffCur - pbDiff;

#ifdef DEBUG
    Assert ( *pcbDiff != 0 );

    BYTE *  pbAfterImage;
    BFAlloc( bfasTemporary, (VOID **)&pbAfterImage );
    SIZE_T  cbAfterImage;

    Assert( dataNew.Cb() <= g_rgfmp[ pfucb->ifmp ].CbPage() );

    ERR errDebug = ErrLGGetAfterImage(
                                    pfucb->ifmp,
                                    pbDiff,
                                    *pcbDiff,
                                    (BYTE *)dataOld.Pv(),
                                    dataOld.Cb(),
                                    !g_rgfmp[ pfucb->ifmp ].FSmallPageDb(),
                                    pbAfterImage,
                                    &cbAfterImage );
    Assert( errDebug == JET_errSuccess );
    Assert( (SIZE_T)dataNew.Cb() == cbAfterImage );

    Assert( memcmp( pbAfterImage, dataNew.Pv(), cbAfterImage ) == 0 );

    BFFree( pbAfterImage );
#endif
}

ERR ErrLGGetAfterImage(
    const IFMP      ifmp,
    BYTE *          pbDiff,
    const SIZE_T    cbDiff,
    BYTE *          pbOld,
    const SIZE_T    cbOld,
    const BOOL      fDiff2,
    BYTE *          pbNew,
    SIZE_T *        pcbNew )
{
    BYTE        *pbOldCur   = pbOld;
    BYTE        *pbNewCur   = pbNew;
    BYTE        *pbDiffCur  = pbDiff;
    BYTE        *pbDiffMax  = pbDiff + cbDiff;
    ERR          err        = JET_errSuccess;
    SIZE_T       cbT;

    while ( pbDiffCur < pbDiffMax )
    {
        INT     cbDataNew;
        INT     ibOffsetOld;
        SIZE_T  cbSkip;

        DIFFHDR2 diffhdr2 = fDiff2 ? *( ( DIFFHDR2* )pbDiffCur ) : DIFFHDR2( *( ( DIFFHDR* )pbDiffCur ) );
        pbDiffCur += fDiff2 ? sizeof( diffhdr2 ) : sizeof( DIFFHDR );

        ibOffsetOld = diffhdr2.Ib();

        if ( ibOffsetOld < 0 )
        {
            AssertSz( fFalse, "Buffer underrun detected in ErrLGGetAfterImage: ibOffsetOld < 0" );
            Call( ErrERRCheck( JET_errLogFileCorrupt ) );
        }
        
        if ( pbOld + ibOffsetOld < pbOldCur )
        {
            AssertSz( fFalse, "Buffer underrun detected in ErrLGGetAfterImage, pbOld + ibOffsetOld < pbOldCur" );
            Call( ErrERRCheck( JET_errLogFileCorrupt ) );
        }
        
        cbSkip = pbOld + ibOffsetOld - pbOldCur;
        if ( pbNewCur + cbSkip - pbNew > g_rgfmp[ ifmp ].CbPage() ||
             (INT_PTR)cbSkip < 0 )
        {
            AssertSz( fFalse, "Buffer corruption detected in ErrLGGetAfterImage" );
            Call( ErrERRCheck( JET_errLogFileCorrupt ) );
        }
        
        UtilMemCpy( pbNewCur, pbOldCur, cbSkip );
        pbNewCur += cbSkip;
        pbOldCur += cbSkip;

        if ( diffhdr2.FUseTwoBytesForDataLength() )
        {
            cbDataNew = *( (UnalignedLittleEndian< WORD >*) pbDiffCur );
            pbDiffCur += sizeof( WORD );
        }
        else
        {
            cbDataNew = *( (UnalignedLittleEndian< BYTE >*) pbDiffCur );
            pbDiffCur += sizeof( BYTE );
        }

        if ( pbNewCur + cbDataNew - pbNew > g_rgfmp[ ifmp ].CbPage() ||
             cbDataNew < 0)
        {
            AssertSz( fFalse, "Stack corruption detected in ErrLGGetAfterImage after UtilMemCpy called" );
            Call( ErrERRCheck( JET_errLogFileCorrupt ) );
        }
        
        if ( diffhdr2.FInsert() )
        {
            UtilMemCpy( pbNewCur, pbDiffCur, cbDataNew );
            pbDiffCur += cbDataNew;
        }
        else
        {
            INT cbDataOld;

            if ( diffhdr2.FReplaceWithSameLength() )
            {
                cbDataOld = cbDataNew;
            }
            else if ( diffhdr2.FUseTwoBytesForDataLength() )
            {
                cbDataOld = *( (UnalignedLittleEndian< WORD >*) pbDiffCur );
                pbDiffCur += sizeof( WORD );
            }
            else
            {
                cbDataOld = *( (UnalignedLittleEndian< BYTE >*) pbDiffCur );
                pbDiffCur += sizeof( BYTE );
            }

            UtilMemCpy( pbNewCur, pbDiffCur, cbDataNew );
            pbDiffCur += cbDataNew;

            pbOldCur += cbDataOld;
        }

        pbNewCur += cbDataNew;

        if ( pbDiffCur > pbDiffMax )
        {
            AssertSz( fFalse, "Buffer overrun detected in ErrLGGetAfterImage, pbDiffCur > pbDiffMax" );
            Call( ErrERRCheck( JET_errLogFileCorrupt ) );
        }
        if ( pbOldCur > pbOld + cbOld )
        {
            AssertSz( fFalse, "Buffer overrun detected in ErrLGGetAfterImage, pbOldCur > pbOld + cbOld" );
            Call( ErrERRCheck( JET_errLogFileCorrupt ) );
        }
    }

    cbT = pbOld + cbOld - pbOldCur;

    if ( pbNewCur + cbT - pbNew > g_rgfmp[ ifmp ].CbPage() )
    {
        AssertSz( fFalse, "Buffer overrun detected in ErrLGGetAfterImage: data generated is larger than a page" );
        Call( ErrERRCheck( JET_errLogFileCorrupt ) );
    }
    
    UtilMemCpy( pbNewCur, pbOldCur, cbT );
    pbNewCur += cbT;

    Assert( pbNewCur >= pbNew );
    *pcbNew = pbNewCur - pbNew;

    return JET_errSuccess;
    
HandleError:

    
    Assert( err < JET_errSuccess );
    return err;
}

