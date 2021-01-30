// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"



ERR CFixedColumnIter::
ErrMoveNext()
{
    ERR err = JET_errSuccess;
    FID fid;

    if ( !m_prec )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    fid = FidOfColumnid( m_columnidCurr + 1 );

    if ( fid > m_prec->FidFixedLastInRec() )
    {
        m_columnidCurr  = m_prec->FidFixedLastInRec() + 1;
        m_errCurr       = errRECNoCurrentColumnValue;
    }
    else
    {
        m_columnidCurr  = ColumnidOfFid( fid, m_pfcb->Ptdb()->FFixedTemplateColumn( fid ) );
        m_errCurr       = JET_errSuccess;

        if ( fid > m_pfcb->Ptdb()->FidFixedLastInitial() )
        {
            m_pfcb->EnterDML();
        }
        m_fieldCurr     = *m_pfcb->Ptdb()->PfieldFixed( m_columnidCurr );
        if ( fid > m_pfcb->Ptdb()->FidFixedLastInitial() )
        {
            m_pfcb->LeaveDML();
        }
    }

    Call( m_errCurr );

HandleError:
    return err;
}

ERR CFixedColumnIter::
ErrSeek( const COLUMNID columnid )
{
    ERR err = JET_errSuccess;
    FID fid;

    if ( !m_prec )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    fid = FidOfColumnid( columnid );

    if ( fid < fidFixedLeast )
    {
        m_columnidCurr  = fidFixedLeast - 1;
        m_errCurr       = errRECNoCurrentColumnValue;

        Error( errRECColumnNotFound );
    }
    else if ( fid > m_prec->FidFixedLastInRec() )
    {
        m_columnidCurr  = m_prec->FidFixedLastInRec() + 1;
        m_errCurr       = errRECNoCurrentColumnValue;

        Error( errRECColumnNotFound );
    }
    else
    {
        m_columnidCurr  = columnid;
        m_errCurr       = JET_errSuccess;

        if ( fid > m_pfcb->Ptdb()->FidFixedLastInitial() )
        {
            m_pfcb->EnterDML();
        }
        m_fieldCurr     = *m_pfcb->Ptdb()->PfieldFixed( m_columnidCurr );
        if ( fid > m_pfcb->Ptdb()->FidFixedLastInitial() )
        {
            m_pfcb->LeaveDML();
        }
    }

    Call( m_errCurr );

HandleError:
    return err;
}

ERR CFixedColumnIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
{
    ERR     err         = JET_errSuccess;
    size_t  ifid;
    BYTE*   prgbitNullity;

    Call( m_errCurr );

    ifid            = FidOfColumnid( m_columnidCurr ) - fidFixedLeast;
    prgbitNullity   = m_prec->PbFixedNullBitMap() + ifid / 8;

    *pcColumnValue = FFixedNullBit( prgbitNullity, ifid ) ? 0 : 1;

HandleError:
    return err;
}

ERR CFixedColumnIter::
ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const
{
    ERR err = JET_errSuccess;

    Call( m_errCurr );

    precsize->cbData            += m_fieldCurr.cbMaxLen;
    precsize->cbDataCompressed  += m_fieldCurr.cbMaxLen;
    precsize->cNonTaggedColumns++;

HandleError:
    return err;
}

ERR CFixedColumnIter::
ErrGetColumnValue(  const size_t    iColumnValue,
                    size_t* const   pcbColumnValue,
                    void** const    ppvColumnValue,
                    BOOL* const     pfColumnValueSeparated,
                    BOOL* const     pfColumnValueCompressed ) const
{
    ERR err = JET_errSuccess;

    Call( m_errCurr );

    if ( iColumnValue != 1 )
    {
        Error( ErrERRCheck( JET_errBadItagSequence ) );
    }

    *pcbColumnValue         = m_fieldCurr.cbMaxLen;
    *ppvColumnValue         = (BYTE*)m_prec + m_fieldCurr.ibRecordOffset;
    *pfColumnValueSeparated = fFalse;
    *pfColumnValueCompressed= fFalse;

HandleError:
    return err;
}




ERR CVariableColumnIter::
ErrMoveNext()
{
    ERR err = JET_errSuccess;
    FID fid;

    if ( !m_prec )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    fid = FidOfColumnid( m_columnidCurr + 1 );

    if ( fid > m_prec->FidVarLastInRec() )
    {
        m_columnidCurr  = m_prec->FidVarLastInRec() + 1;
        m_errCurr       = errRECNoCurrentColumnValue;
    }
    else
    {
        m_columnidCurr  = ColumnidOfFid( fid, m_pfcb->Ptdb()->FVarTemplateColumn( fid ) );
        m_errCurr       = JET_errSuccess;
    }

    Call( m_errCurr );

HandleError:
    return err;
}

ERR CVariableColumnIter::
ErrSeek( const COLUMNID columnid )
{
    ERR err = JET_errSuccess;
    FID fid;

    if ( !m_prec )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    fid = FidOfColumnid( columnid );

    if ( fid < fidVarLeast )
    {
        m_columnidCurr  = fidVarLeast - 1;
        m_errCurr       = errRECNoCurrentColumnValue;

        Error( errRECColumnNotFound );
    }
    else if ( fid > m_prec->FidVarLastInRec() )
    {
        m_columnidCurr  = m_prec->FidVarLastInRec() + 1;
        m_errCurr       = errRECNoCurrentColumnValue;

        Error( errRECColumnNotFound );
    }
    else
    {
        m_columnidCurr  = columnid;
        m_errCurr       = JET_errSuccess;
    }

    Call( m_errCurr );

HandleError:
    return err;
}

ERR CVariableColumnIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
{
    ERR                                         err         = JET_errSuccess;
    size_t                                      ifid;
    UnalignedLittleEndian< REC::VAROFFSET >*    pibVarOffs;

    Call( m_errCurr );

    ifid        = FidOfColumnid( m_columnidCurr ) - fidVarLeast;
    pibVarOffs  = m_prec->PibVarOffsets();

    *pcColumnValue = FVarNullBit( pibVarOffs[ ifid ] ) ? 0 : 1;

HandleError:
    return err;
}

ERR CVariableColumnIter::
ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const
{
    ERR                                         err         = JET_errSuccess;
    size_t                                      ifid;
    UnalignedLittleEndian< REC::VAROFFSET >*    pibVarOffs;
    REC::VAROFFSET                              ibStartOfColumn;

    Call( m_errCurr );

    ifid            = FidOfColumnid( m_columnidCurr ) - fidVarLeast;
    pibVarOffs      = m_prec->PibVarOffsets();
    ibStartOfColumn = m_prec->IbVarOffsetStart( FidOfColumnid( m_columnidCurr ) );

    precsize->cbData            += IbVarOffset( pibVarOffs[ ifid ] ) - ibStartOfColumn;
    precsize->cbDataCompressed  += IbVarOffset( pibVarOffs[ ifid ] ) - ibStartOfColumn;
    precsize->cbOverhead += sizeof(REC::VAROFFSET);
    precsize->cNonTaggedColumns++;

    Assert( !FVarNullBit( pibVarOffs[ ifid ] ) || 0 == precsize->cbData );

HandleError:
    return err;
}

ERR CVariableColumnIter::
ErrGetColumnValue(  const size_t    iColumnValue,
                    size_t* const   pcbColumnValue,
                    void** const    ppvColumnValue,
                    BOOL* const     pfColumnValueSeparated,
                    BOOL* const     pfColumnValueCompressed ) const
{
    ERR                                         err             = JET_errSuccess;
    size_t                                      ifid;
    UnalignedLittleEndian< REC::VAROFFSET >*    pibVarOffs;
    REC::VAROFFSET                              ibStartOfColumn;

    Call( m_errCurr );

    if ( iColumnValue != 1 )
    {
        Error( ErrERRCheck( JET_errBadItagSequence ) );
    }

    ifid            = FidOfColumnid( m_columnidCurr ) - fidVarLeast;
    pibVarOffs      = m_prec->PibVarOffsets();
    ibStartOfColumn = m_prec->IbVarOffsetStart( FidOfColumnid( m_columnidCurr ) );

    *pcbColumnValue = IbVarOffset( pibVarOffs[ ifid ] ) - ibStartOfColumn;
    if ( *pcbColumnValue == 0 )
    {
        *ppvColumnValue = NULL;
    }
    else
    {
        *ppvColumnValue = m_prec->PbVarData() + ibStartOfColumn;
    }
    *pfColumnValueSeparated = fFalse;
    *pfColumnValueCompressed= fFalse;

HandleError:
    return err;
}




ERR CSingleValuedTaggedColumnValueIter::
ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const
{
    ERR err = JET_errSuccess;
    BYTE *pbDataDecrypted = NULL;
    const BOOL fLocal = ( grbit & JET_bitRecordSizeLocal );

    if ( !m_rgbData )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    if ( m_fSeparated )
    {
        Assert( m_fSeparatable );
        Assert( sizeof( _LID64 ) == m_cbData || sizeof( _LID32 ) == m_cbData );

        if ( !fLocal )
        {
            Call( ErrRECGetLVSize(
                            pfucb,
                            LidOfSeparatedLV( m_rgbData, m_cbData ),
                            !!(grbit & JET_bitRecordSizeExtrinsicLogicalOnly),
                            &precsize->cbLongValueData,
                            &precsize->cbLongValueDataCompressed,
                            &precsize->cbLongValueOverhead ) );
        }
        precsize->cbOverhead += sizeof(TAGFLD) + sizeof(TAGFLD_HEADER) + m_cbData;
        precsize->cLongValues++;
    }
    else if ( grbit & JET_bitRecordSizeIntrinsicPhysicalOnly )
    {
        precsize->cbOverhead        += sizeof(TAGFLD) + ( m_fSeparatable ? sizeof(TAGFLD_HEADER) : 0 );
        precsize->cbData            += m_cbData;
        precsize->cbDataCompressed  += m_cbData;

        if ( m_fSeparatable )
        {
            Assert( !m_fSeparated );
            precsize->cbIntrinsicLongValueData += m_cbData;
            precsize->cbIntrinsicLongValueDataCompressed += m_cbData;
            precsize->cIntrinsicLongValues++;
        }
    }
    else
    {
        precsize->cbOverhead        += sizeof(TAGFLD) + ( m_fSeparatable ? sizeof(TAGFLD_HEADER) : 0 );
        INT cbDataActual = m_cbData;
        DATA data;
        data.SetPv( m_rgbData );
        data.SetCb( m_cbData );


        if ( m_fEncrypted &&
             data.Cb() > 0 )
        {
            if ( pfucb->pbEncryptionKey == NULL )
            {
                Error( ErrERRCheck( JET_errColumnNoEncryptionKey ) );
            }
            Alloc( pbDataDecrypted = new BYTE[ data.Cb() ] );
            Call( ErrOSUDecrypt(
                    (BYTE*)data.Pv(),
                    pbDataDecrypted,
                    (ULONG *)&cbDataActual,
                    pfucb->pbEncryptionKey,
                    pfucb->cbEncryptionKey,
                    PinstFromPfucb( pfucb )->m_iInstance,
                    pfucb->u.pfcb->TCE() ) );
            data.SetPv( pbDataDecrypted );
            data.SetCb( cbDataActual );
        }

        if ( m_fCompressed )
        {
            Call( ErrPKDecompressData( data, pfucb, NULL, 0, &cbDataActual ) );
            Assert( JET_wrnBufferTruncated == err );
            err = JET_errSuccess;
            Assert( m_fEncrypted || (ULONG)cbDataActual >= m_cbData );
        }

        precsize->cbData += cbDataActual;
        precsize->cbDataCompressed  += m_cbData;

        if ( m_fSeparatable )
        {
            Assert( !m_fSeparated );
            precsize->cbIntrinsicLongValueData += cbDataActual;
            precsize->cbIntrinsicLongValueDataCompressed += m_cbData;
            precsize->cIntrinsicLongValues++;
        }
    }

    precsize->cTaggedColumns++;

HandleError:
    if ( pbDataDecrypted )
    {
        delete[] pbDataDecrypted;
        pbDataDecrypted = NULL;
    }

    return err;
}

ERR CSingleValuedTaggedColumnValueIter::
ErrGetColumnValue(  const size_t    iColumnValue,
                    size_t* const   pcbColumnValue,
                    void** const    ppvColumnValue,
                    BOOL* const     pfColumnValueSeparated,
                    BOOL* const     pfColumnValueCompressed ) const
{
    ERR err = JET_errSuccess;

    if ( !m_rgbData )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    if ( iColumnValue != 1 )
    {
        Error( ErrERRCheck( JET_errBadItagSequence ) );
    }

    *pcbColumnValue         = m_cbData;
    *ppvColumnValue         = m_rgbData;
    *pfColumnValueSeparated = m_fSeparated;
    *pfColumnValueCompressed= m_fCompressed;

HandleError:
    return err;
}




ERR CDualValuedTaggedColumnValueIter::
ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const
{
    ERR err = JET_errSuccess;

    if ( !m_ptwovalues )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    precsize->cbData            += m_ptwovalues->CbFirstValue() + m_ptwovalues->CbSecondValue();
    precsize->cbDataCompressed  += m_ptwovalues->CbFirstValue() + m_ptwovalues->CbSecondValue();
    precsize->cbOverhead += sizeof(TAGFLD) + sizeof(TAGFLD_HEADER) + sizeof(TWOVALUES::TVLENGTH);
    precsize->cTaggedColumns++;
    precsize->cMultiValues++;

HandleError:
    return err;
}

ERR CDualValuedTaggedColumnValueIter::
ErrGetColumnValue(  const size_t    iColumnValue,
                    size_t* const   pcbColumnValue,
                    void** const    ppvColumnValue,
                    BOOL* const     pfColumnValueSeparated,
                    BOOL* const     pfColumnValueCompressed ) const
{
    ERR err = JET_errSuccess;

    if ( !m_ptwovalues )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    if ( iColumnValue < 1 || iColumnValue > 2 )
    {
        Error( ErrERRCheck( JET_errBadItagSequence ) );
    }

    if ( iColumnValue == 1 )
    {
        *pcbColumnValue = m_ptwovalues->CbFirstValue();
        *ppvColumnValue = m_ptwovalues->PbData();
    }
    else
    {
        *pcbColumnValue = m_ptwovalues->CbSecondValue();
        *ppvColumnValue = m_ptwovalues->PbData() + m_ptwovalues->CbFirstValue();
    }

    *pfColumnValueSeparated = fFalse;
    *pfColumnValueCompressed= fFalse;

HandleError:
    return err;
}




ERR CMultiValuedTaggedColumnValueIter::
ErrGetColumnSize( FUCB* const pfucb, JET_RECSIZE3* const precsize, const JET_GRBIT grbit ) const
{
    ERR err = JET_errSuccess;
    const BOOL fLocal = ( grbit & JET_bitRecordSizeLocal );

    if ( !m_pmultivalues )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    const TAGFLD_HEADER *   pheader         = m_pmultivalues->Pheader();
    const ULONG             cMultiValues    = m_pmultivalues->CMultiValues();

#ifdef UNLIMITED_MULTIVALUES
    UNDONE: unhandled case
#else
    Assert( !pheader->FSeparated() );
#endif

    if ( pheader->FColumnCanBeSeparated() )
    {
        precsize->cbOverhead += sizeof(TAGFLD) + sizeof(TAGFLD_HEADER) + m_pmultivalues->IbStartOfMVData();

        for ( ULONG imv = 0; imv < cMultiValues; imv++ )
        {
            if ( m_pmultivalues->FSeparatedInstance( imv ) )
            {
                ULONG cbData = m_pmultivalues->CbData( imv );
                Assert( sizeof( _LID64 ) == cbData || sizeof( _LID32 ) == cbData );

                if ( !fLocal )
                {
                    Call( ErrRECGetLVSize(
                                pfucb,
                                LidOfSeparatedLV( m_pmultivalues->PbData( imv ), cbData ),
                                !!(grbit & JET_bitRecordSizeExtrinsicLogicalOnly),
                                &precsize->cbLongValueData,
                                &precsize->cbLongValueDataCompressed,
                                &precsize->cbLongValueOverhead ) );

                }

                precsize->cbOverhead += cbData;
                precsize->cLongValues++;
            }
            else if( 0 == imv && m_fCompressed && !(grbit & JET_bitRecordSizeIntrinsicPhysicalOnly) )
            {
                DATA data;
                data.SetPv( m_pmultivalues->PbData( imv ) );
                data.SetCb( m_pmultivalues->CbData( imv ) );
                INT cbDataActual;
                Call( ErrPKDecompressData( data, pfucb, NULL, 0, &cbDataActual ) );
                Assert( JET_wrnBufferTruncated == err );
                err = JET_errSuccess;
                Assert( (ULONG)cbDataActual >= m_pmultivalues->CbData( imv ) );
                precsize->cbData += cbDataActual;
                
                precsize->cbDataCompressed += m_pmultivalues->CbData( imv );

                precsize->cbIntrinsicLongValueData += cbDataActual;
                precsize->cbIntrinsicLongValueDataCompressed += m_pmultivalues->CbData( imv );
                precsize->cIntrinsicLongValues++;
            }
            else
            {
                ULONG cbData = m_pmultivalues->CbData( imv );
                precsize->cbData += cbData;
                precsize->cbDataCompressed += cbData;
                precsize->cbIntrinsicLongValueData += cbData;
                precsize->cbIntrinsicLongValueDataCompressed += cbData;
                precsize->cIntrinsicLongValues++;
            }
        }
    }
    else
    {
        Assert( !pheader->FLongValue() );

        precsize->cbOverhead += sizeof(TAGFLD) + sizeof(TAGFLD_HEADER) + m_pmultivalues->IbStartOfMVData();
        precsize->cbData            += m_pmultivalues->CbMVData();
        precsize->cbDataCompressed  += m_pmultivalues->CbMVData();
    }

    precsize->cTaggedColumns++;

    Assert( cMultiValues > 1 );
    precsize->cMultiValues += ( cMultiValues - 1 );

HandleError:
    return err;
}

ERR CMultiValuedTaggedColumnValueIter::
ErrGetColumnValue(  const size_t    iColumnValue,
                    size_t* const   pcbColumnValue,
                    void** const    ppvColumnValue,
                    BOOL* const     pfColumnValueSeparated,
                    BOOL* const     pfColumnValueCompressed ) const
{
    ERR err = JET_errSuccess;

    if ( !m_pmultivalues )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    if ( iColumnValue < 1 || iColumnValue > m_pmultivalues->CMultiValues() )
    {
        Error( ErrERRCheck( JET_errBadItagSequence ) );
    }

    *pcbColumnValue         = m_pmultivalues->CbData( iColumnValue - 1 );
    *ppvColumnValue         = m_pmultivalues->PbData( iColumnValue - 1 );
    *pfColumnValueSeparated = m_pmultivalues->FSeparatedInstance( iColumnValue - 1 );
    *pfColumnValueCompressed= m_fCompressed && ( 1 == iColumnValue );

HandleError:
    return err;
}

size_t CMultiValuedTaggedColumnValueIter::
CbESE97Format() const
{
    const ULONG cMultiValues    = m_pmultivalues->CMultiValues();
    const BOOL  fSeparatable    = ( m_pmultivalues->Pheader()->FColumnCanBeSeparated() );
    const ULONG cbMVOverhead    = sizeof(TAGFLD) + ( fSeparatable ? sizeof(BYTE) : 0 );
    size_t      cbESE97Format   = cbMVOverhead * cMultiValues;

    for ( ULONG imv = 0; imv < cMultiValues; imv++ )
    {
        const ULONG     cbMV    = m_pmultivalues->CbData( imv );

        if ( fSeparatable )
        {
            cbESE97Format += min( cbMV, sizeof(_LID32) );
        }
        else
        {
            cbESE97Format += cbMV;
        }
    }

    return cbESE97Format;
}


ERR CTaggedColumnIter::
ErrMoveNext()
{
    ERR         err     = JET_errSuccess;
    TAGFLD*     ptagfld;

    if ( !m_ptagfields )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    ptagfld = m_ptagfldCurr + 1;

    if ( ptagfld > m_ptagfields->Rgtagfld() + m_ptagfields->CTaggedColumns() - 1 )
    {
        m_ptagfldCurr   = m_ptagfields->Rgtagfld() + m_ptagfields->CTaggedColumns();
        m_errCurr       = errRECNoCurrentColumnValue;
    }
    else
    {
        m_ptagfldCurr   = ptagfld;
        m_errCurr       = JET_errSuccess;

        Call( ErrCreateCVIter( &m_pcviterCurr ) );
    }

    Call( m_errCurr );

HandleError:
    m_errCurr = err;
    return err;
}

ERR CTaggedColumnIter::
ErrSeek( const COLUMNID columnid )
{
    ERR         err             = JET_errSuccess;
    BOOL        fUseDerivedBit  = fFalse;
    FCB*        pfcb            = m_pfcb;
    FID         fid;
    size_t      itagfld;
    TAGFLD*     ptagfld;

    if ( !m_ptagfields )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    if ( !FCOLUMNIDTagged( columnid ) )
    {
        m_ptagfldCurr = m_ptagfields->Rgtagfld() - 1;
        m_errCurr = errRECNoCurrentColumnValue;

        Error( errRECColumnNotFound );
    }

    if ( FCOLUMNIDTemplateColumn( columnid ) )
    {
        if ( !pfcb->FTemplateTable() )
        {
            pfcb->Ptdb()->AssertValidDerivedTable();

            fUseDerivedBit = FRECUseDerivedBitForTemplateColumnInDerivedTable( columnid, pfcb->Ptdb() );

            pfcb = pfcb->Ptdb()->PfcbTemplateTable();
        }
        else
        {
            pfcb->Ptdb()->AssertValidTemplateTable();
            Assert( !fUseDerivedBit );
        }
    }
    else
    {
        Assert( !pfcb->FTemplateTable() );
    }

    fid     = FidOfColumnid( columnid );
    itagfld = m_ptagfields->ItagfldFind( TAGFLD( fid, fUseDerivedBit ) );
    ptagfld = m_ptagfields->Rgtagfld() + itagfld;

    if (    itagfld < m_ptagfields->CTaggedColumns() &&
            ptagfld->FIsEqual( fid, fUseDerivedBit ) )
    {
        m_ptagfldCurr   = ptagfld;
        m_errCurr       = JET_errSuccess;

        Call( ErrCreateCVIter( &m_pcviterCurr ) );
    }
    else
    {
        m_ptagfldCurr   = ptagfld - 1;
        m_errCurr       = errRECNoCurrentColumnValue;

        Error( errRECColumnNotFound );
    }

    Call( m_errCurr );

HandleError:
    m_errCurr = err;
    return err;
}

ERR CTaggedColumnIter::
ErrCreateCVIter( IColumnValueIter** const ppcviter )
{
    ERR err = JET_errSuccess;

    if ( m_ptagfldCurr->FNull( m_ptagfields ) )
    {
        CNullValuedTaggedColumnValueIter* pcviterNV = new( m_rgbCVITER ) CNullValuedTaggedColumnValueIter();
        Call( pcviterNV->ErrInit() );
        *ppcviter = pcviterNV;
    }
    else
    {
        BYTE*   rgbData = m_ptagfields->PbTaggedColumns() + m_ptagfldCurr->Ib();
        size_t  cbData  = m_ptagfields->CbData( ULONG( m_ptagfldCurr - m_ptagfields->Rgtagfld() ) );

        if ( m_ptagfldCurr->FExtendedInfo() )
        {
            TAGFLD_HEADER* ptagfldhdr = (TAGFLD_HEADER*)rgbData;

            if ( ptagfldhdr->FSingleValue() )
            {
                CSingleValuedTaggedColumnValueIter* pcviterSV = new( m_rgbCVITER ) CSingleValuedTaggedColumnValueIter();
                Call( pcviterSV->ErrInit(   rgbData + sizeof( TAGFLD_HEADER ),
                                            cbData - sizeof( TAGFLD_HEADER ),
                                            ptagfldhdr->FColumnCanBeSeparated(),
                                            ptagfldhdr->FSeparated(),
                                            ptagfldhdr->FCompressed(),
                                            ptagfldhdr->FEncrypted() ) );
                *ppcviter = pcviterSV;
            }
            else if ( ptagfldhdr->FTwoValues() )
            {
                CDualValuedTaggedColumnValueIter* pcviterDV = new( m_rgbCVITER ) CDualValuedTaggedColumnValueIter();
                Call( pcviterDV->ErrInit( rgbData, cbData ) );
                *ppcviter = pcviterDV;
            }
            else
            {
                Assert( ptagfldhdr->FMultiValues() );

                CMultiValuedTaggedColumnValueIter* pcviterMV = new( m_rgbCVITER ) CMultiValuedTaggedColumnValueIter();
                Call( pcviterMV->ErrInit( rgbData, cbData, ptagfldhdr->FCompressed() ) );
                *ppcviter = pcviterMV;
            }
        }
        else
        {
            CSingleValuedTaggedColumnValueIter* pcviterSV = new( m_rgbCVITER ) CSingleValuedTaggedColumnValueIter();
            Call( pcviterSV->ErrInit( rgbData, cbData, fFalse, fFalse, fFalse, fFalse ) );
            *ppcviter = pcviterSV;
        }
    }

HandleError:
    return err;
}




ERR CUnionIter::
ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const
{
    ERR     err         = JET_errSuccess;
    size_t  cColumnLHS;
    size_t  cColumnRHS;

    if ( !m_pciterLHS )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    Call( m_pciterLHS->ErrGetWorstCaseColumnCount( &cColumnLHS ) );
    Call( m_pciterRHS->ErrGetWorstCaseColumnCount( &cColumnRHS ) );

    *pcColumn = cColumnLHS + cColumnRHS;

HandleError:
    return err;
}


ERR CUnionIter::
ErrMoveBeforeFirst()
{
    ERR err = JET_errSuccess;

    if ( !m_pciterLHS )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    Call( m_pciterLHS->ErrMoveBeforeFirst() );

    Call( m_pciterRHS->ErrMoveBeforeFirst() );

    m_pciterCurr = m_pciterLHS;
    m_fRHSBeforeFirst = fTrue;

HandleError:
    return err;
}

ERR CUnionIter::
ErrMoveNext()
{
    ERR         err         = JET_errSuccess;
    COLUMNID    columnidLHS;
    COLUMNID    columnidRHS;

    if ( !m_pciterLHS )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    if ( m_fRHSBeforeFirst )
    {
        m_fRHSBeforeFirst = fFalse;
        err = m_pciterRHS->ErrMoveNext();
        Call( err == errRECNoCurrentColumnValue ? JET_errSuccess : err );
    }

    err = m_pciterCurr->ErrMoveNext();
    Call( err == errRECNoCurrentColumnValue ? JET_errSuccess : err );

    err = m_pciterLHS->ErrGetColumnId( &columnidLHS );
    if ( err == errRECNoCurrentColumnValue )
    {
        err = JET_errSuccess;
        columnidLHS = ~fCOLUMNIDTemplate;
    }
    Call( err );

    err = m_pciterRHS->ErrGetColumnId( &columnidRHS );
    if ( err == errRECNoCurrentColumnValue )
    {
        err = JET_errSuccess;
        columnidRHS = ~fCOLUMNIDTemplate;
    }
    Call( err );

    if ( ( columnidLHS ^ fCOLUMNIDTemplate ) < ( columnidRHS ^ fCOLUMNIDTemplate ) )
    {
        m_pciterCurr = m_pciterLHS;
    }
    else if ( ( columnidLHS ^ fCOLUMNIDTemplate ) > ( columnidRHS ^ fCOLUMNIDTemplate ) )
    {
        m_pciterCurr = m_pciterRHS;
    }
    else
    {
        if ( columnidLHS != ~fCOLUMNIDTemplate )
        {
            err = m_pciterRHS->ErrMoveNext();
            Call( err == errRECNoCurrentColumnValue ? JET_errSuccess : err );

            m_pciterCurr = m_pciterLHS;
        }
        else
        {
            Error( errRECNoCurrentColumnValue );
        }
    }

HandleError:
    return err;
}

ERR CUnionIter::
ErrSeek( const COLUMNID columnid )
{
    ERR err = JET_errSuccess;

    if ( !m_pciterLHS )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    m_pciterCurr = m_pciterLHS;
    err = m_pciterCurr->ErrSeek( columnid );

    if ( err == errRECColumnNotFound )
    {
        m_pciterCurr = m_pciterRHS;
        err = m_pciterCurr->ErrSeek( columnid );
    }

HandleError:
    return err;
}




#include <pshpack1.h>

typedef BYTE ColumnReferenceFormatIdentifier;

PERSISTED
enum ColumnReferenceFormatIdentifiers : ColumnReferenceFormatIdentifier
{
    crfiInvalid         = 0x00,
    crfiV1              = 0x01,
    crfiV2              = 0x02,
    crfiVMaxSupported   = 0x02,
};

PERSISTED
class CColumnReferenceFormatCommon
{
    public:

        UnalignedLittleEndian<ColumnReferenceFormatIdentifier>  m_crfi;
};

PERSISTED
class CColumnReferenceFormatV1 : public CColumnReferenceFormatCommon
{
    public:

        UnalignedLittleEndian<USHORT>                           m_cbBookmark;
        BYTE                                                    m_cbColumnName;
        UnalignedLittleEndian<ULONG>                            m_itagSequence;
        UnalignedLittleEndian<_LID32>                           m_lid;
        BYTE                                                    m_rgbDatabaseSignature[OffsetOf( SIGNATURE, szComputerName )];
        BYTE                                                    m_rgbBookmark[0];
};

PERSISTED
class CColumnReferenceFormatV2 : public CColumnReferenceFormatCommon
{
    public:

        UnalignedLittleEndian<USHORT>                           m_cbBookmark;
        BYTE                                                    m_cbColumnName;
        UnalignedLittleEndian<ULONG>                            m_itagSequence;
        UnalignedLittleEndian<LvId>                             m_lid;
        BYTE                                                    m_rgbDatabaseSignature[ OffsetOf( SIGNATURE, szComputerName ) ];
        BYTE                                                    m_rgbBookmark[ 0 ];
};

#include <poppack.h>

LOCAL ERR ErrRECIParseColumnReference(
    FUCB* const         pfucb,
    const BYTE* const   rgbReference,
    const ULONG         cbReference,
    LvId* const     plid,
    const BYTE** const  prgbBookmark,
    ULONG* const        pcbBookmark,
    const CHAR** const  prgchColumnName,
    ULONG* const        pcchColumnName,
    ULONG* const        pitagSequence
    )
{
    ERR                                         err     = JET_errSuccess;
    const CColumnReferenceFormatCommon* const   pcrfc   = (const CColumnReferenceFormatCommon* const)rgbReference;
    const CColumnReferenceFormatV1* const       pcrfv1  = (const CColumnReferenceFormatV1* const)rgbReference;
    const CColumnReferenceFormatV2* const       pcrfv2  = (const CColumnReferenceFormatV2* const)rgbReference;

{

    *plid               = lidMin;
    *prgbBookmark       = NULL;
    *pcbBookmark        = 0;
    *prgchColumnName    = NULL;
    *pcchColumnName     = 0;
    *pitagSequence      = 0;


    if ( rgbReference == NULL )
    {
        Error( ErrERRCheck( JET_errInvalidColumnReference ) );
    }
    if ( cbReference < sizeof( CColumnReferenceFormatCommon ) )
    {
        Error( ErrERRCheck( JET_errInvalidColumnReference) );
    }
    if ( pcrfc->m_crfi > crfiVMaxSupported )
    {
        Error( ErrERRCheck( JET_errInvalidColumnReference ) );
    }

    ULONG   cbReferenceMinExpected;
    USHORT  cbBookmark;
    BYTE    cbColumnName;
    ULONG   itagSequence;
    LvId    lid;
    BYTE*   rgbDatabaseSignature;
    BYTE*   rgbBookmark;

    if ( pcrfc->m_crfi == crfiV1 )
    {
        cbReferenceMinExpected = sizeof( CColumnReferenceFormatV1 );
        cbBookmark = pcrfv1->m_cbBookmark;
        cbColumnName = pcrfv1->m_cbColumnName;
        itagSequence = pcrfv1->m_itagSequence;
        lid = (_LID32)pcrfv1->m_lid;
        rgbDatabaseSignature = const_cast<BYTE*>( pcrfv1->m_rgbDatabaseSignature );
        rgbBookmark = const_cast<BYTE*>( pcrfv1->m_rgbBookmark );
    }
    else if ( pcrfc->m_crfi == crfiV2 )
    {
        cbReferenceMinExpected = sizeof( CColumnReferenceFormatV2 );
        cbBookmark = pcrfv2->m_cbBookmark;
        cbColumnName = pcrfv2->m_cbColumnName;
        itagSequence = pcrfv2->m_itagSequence;
        lid = pcrfv2->m_lid;
        rgbDatabaseSignature = const_cast<BYTE*>( pcrfv2->m_rgbDatabaseSignature );
        rgbBookmark = const_cast<BYTE*>( pcrfv2->m_rgbBookmark );
    }
    else
    {
        Error( ErrERRCheck( JET_errInvalidColumnReference ) );
    }

    if ( cbReference < cbReferenceMinExpected )
    {
        Error( ErrERRCheck( JET_errInvalidColumnReference ) );
    }
    if ( cbBookmark <= 0 || cbBookmark > ( pfucb->u.pfcb->Pidb() ? pfucb->u.pfcb->Pidb()->CbKeyMost() : sizeof( DBK ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidColumnReference ) );
    }
    if ( cbColumnName <= 0 || cbColumnName > JET_cbNameMost )
    {
        Error( ErrERRCheck( JET_errInvalidColumnReference ) );
    }
    if ( cbReference != cbReferenceMinExpected + cbBookmark + cbColumnName )
    {
        Error( ErrERRCheck( JET_errInvalidColumnReference ) );
    }
    if ( itagSequence == 0 )
    {
        Error( ErrERRCheck( JET_errInvalidColumnReference ) );
    }
    _LID64 lidMost = pfucb->u.pfcb->Ptdb()->Ui64LidLast();
    if ( !lid.FValid() || ( lidMost != lidMin && lid > lidMost ) )
    {
        if ( !FNegTest( fInvalidAPIUsage ) )
        {
            FireWall( "InvalidLidInCrf" );
        }
        Error( ErrERRCheck( JET_errInvalidColumnReference ) );
    }


    *plid = lid;
    *prgbBookmark = rgbBookmark;
    *pcbBookmark = cbBookmark;
    *prgchColumnName = (CHAR*) &rgbBookmark[ cbBookmark ];
    *pcchColumnName = cbColumnName;
    *pitagSequence = itagSequence;


    static_assert( sizeof( pcrfv1->m_rgbDatabaseSignature ) == sizeof( pcrfv2->m_rgbDatabaseSignature ),
                    "Following code expects the crfv1 and crfv2 db signatures to be of the same size" );
    if ( memcmp(    &rgbDatabaseSignature[0],
                    &g_rgfmp[pfucb->ifmp].Pdbfilehdr()->signDb,
                    sizeof( pcrfv1->m_rgbDatabaseSignature ) ) )
    {
        *plid = lidMin;
    }
}

HandleError:
    return err;
}

ERR ErrRECCreateColumnReference(
    FUCB* const             pfucb,
    const JET_COLUMNID      columnid,
    const ULONG             itagSequence,
    const LvId              lid,
    ULONG* const            pcbReference,
    BYTE** const            prgbReference,
    const JET_PFNREALLOC    pfnRealloc,
    const void* const       pvReallocContext
    )
{
    ERR                         err         = JET_errSuccess;
    FCB*                        pfcb        = FCOLUMNIDTemplateColumn( columnid ) ?
                                                pfucb->u.pfcb->Ptdb()->PfcbTemplateTable() :
                                                pfucb->u.pfcb;
    BOOL                        fLeaveDML   = fFalse;
    FIELD*                      pfield      = pfieldNil;


    Assert( !FFUCBSort( pfucb ) );
    if ( FFMPIsTempDB( pfucb->ifmp ) )
    {
        Expected( fFalse );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    SIZE_T cbBookmark = pfucb->bmCurr.key.Cb();
    Assert( cbBookmark > 0 );
    Assert( cbBookmark <= ( pfucb->u.pfcb->Pidb() ? pfucb->u.pfcb->Pidb()->CbKeyMost() : sizeof( DBK ) ) );
    Assert( cbBookmark <= wMax );


    if ( !pfcb->FTemplateTable() )
    {
        pfcb->EnterDML();
        fLeaveDML = fTrue;
    }
    Call( ErrFILEGetPfieldFromColumnid( pfcb, columnid, &pfield ) );
    CHAR* szColumnName = pfcb->Ptdb()->SzFieldName( pfield->itagFieldName, fFalse );
    SIZE_T cbColumnName = LOSStrLengthA( szColumnName ) * sizeof( CHAR );


    Assert( cbColumnName > 0 );
    Assert( cbColumnName <= JET_cbNameMost );
    Assert( cbColumnName <= bMax );

    bool fUseCrfv2 = ( JET_errSuccess == PfmpFromIfmp( pfucb->ifmp )->ErrDBFormatFeatureEnabled( JET_efvLid64 ) );

    *pcbReference = ULONG( cbBookmark + cbColumnName +
                    ( fUseCrfv2 ? sizeof( CColumnReferenceFormatV2 ) : sizeof( CColumnReferenceFormatV1 ) ) );
    if ( pfnRealloc )
    {
        Alloc( *prgbReference = (BYTE*)pfnRealloc( (void*)pvReallocContext, (void*)*prgbReference, *pcbReference ) );
    }
    else
    {
        Alloc( *prgbReference = new BYTE[*pcbReference] );
    }

    if ( fUseCrfv2 )
    {
        CColumnReferenceFormatV2*   pcrfv2 = (CColumnReferenceFormatV2*) *prgbReference;
        pcrfv2->m_crfi = crfiV2;
        pcrfv2->m_cbBookmark = (USHORT) cbBookmark;
        pcrfv2->m_cbColumnName = (BYTE) cbColumnName;
        pcrfv2->m_itagSequence = itagSequence;
        pcrfv2->m_lid = lid;
        UtilMemCpy( &pcrfv2->m_rgbDatabaseSignature[ 0 ],
            &g_rgfmp[ pfucb->ifmp ].Pdbfilehdr()->signDb,
            sizeof( pcrfv2->m_rgbDatabaseSignature ) );
        pfucb->bmCurr.key.CopyIntoBuffer( &pcrfv2->m_rgbBookmark[ 0 ] );
        UtilMemCpy( &pcrfv2->m_rgbBookmark[ cbBookmark ], szColumnName, cbColumnName );
    }
    else
    {
        CColumnReferenceFormatV1*   pcrfv1 = (CColumnReferenceFormatV1*) *prgbReference;
        pcrfv1->m_crfi = crfiV1;
        pcrfv1->m_cbBookmark = (USHORT) cbBookmark;
        pcrfv1->m_cbColumnName = (BYTE) cbColumnName;
        pcrfv1->m_itagSequence = itagSequence;

        EnforceSz( lid.FIsLid32(), "UnexpectedLid64InCrfv1" );
        pcrfv1->m_lid = (ULONG) lid;

        UtilMemCpy( &pcrfv1->m_rgbDatabaseSignature[ 0 ],
            &g_rgfmp[ pfucb->ifmp ].Pdbfilehdr()->signDb,
            sizeof( pcrfv1->m_rgbDatabaseSignature ) );
        pfucb->bmCurr.key.CopyIntoBuffer( &pcrfv1->m_rgbBookmark[ 0 ] );
        UtilMemCpy( &pcrfv1->m_rgbBookmark[ cbBookmark ], szColumnName, cbColumnName );
    }

HandleError:
    if ( fLeaveDML )
    {
        pfcb->LeaveDML();
    }
    if ( err < JET_errSuccess )
    {
        if ( *prgbReference )
        {
            if ( pfnRealloc )
            {
                pfnRealloc( (void*)pvReallocContext, (void*)*prgbReference, 0 );
            }
            else
            {
                delete[] *prgbReference;
            }
        }
    }
    return err;
}

LOCAL ERR ErrIsamIResolveColumnFromColumnReference(
    _In_ const JET_SESID                                sesid,
    _In_ const JET_TABLEID                              tableid,
    _In_reads_bytes_( cchColumnName ) const CHAR* const rgchColumnName,
    _In_ const ULONG                                    cchColumnName,
    _Out_ JET_COLUMNID* const                           pcolumnid )
{
    ERR err = JET_errSuccess;


    *pcolumnid = JET_columnidNil;


    CHAR szColumn[JET_cbNameMost + 1];
    if ( cchColumnName > JET_cbNameMost )
    {
        Error( ErrERRCheck( JET_errInvalidColumnReference ) );
    }
    UtilMemCpy( szColumn, rgchColumnName, cchColumnName );
    szColumn[cchColumnName] = 0;


    JET_COLUMNDEF columndef;
    err = ErrIsamGetTableColumnInfo( sesid, tableid, szColumn, NULL, &columndef, sizeof( columndef ), JET_ColInfo, false );
    if ( err == JET_errColumnNotFound )
    {
        Error( ErrERRCheck( JET_errStaleColumnReference ) );
    }


    if ( columndef.coltyp != JET_coltypLongBinary && columndef.coltyp != JET_coltypLongText )
    {
        Error( ErrERRCheck( JET_errStaleColumnReference ) );
    }

    *pcolumnid = columndef.columnid;

HandleError:
    return err;
}

#ifdef DEBUG
LOCAL ERR ErrIsamIRetrieveColumnByReferenceSlowly(
    _In_ const JET_SESID                                            sesid,
    _In_ const JET_TABLEID                                          tableid,
    _In_reads_bytes_( cchColumnName ) const BYTE* const             rgbBookmark,
    _In_ const ULONG                                                cbBookmark,
    _In_reads_bytes_( cchColumnName ) const CHAR* const             rgchColumnName,
    _In_ const ULONG                                                cchColumnName,
    _In_ ULONG                                                      itagSequence,
    _In_ const ULONG                                        ibData,
    _Out_writes_bytes_to_opt_( cbData, *pcbActual ) void * const    pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit )
{
    ERR             err             = JET_errSuccess;
    JET_TABLEID     tableidRetrieve = JET_tableidNil;
    JET_COLUMNID    columnid        = JET_columnidNil;
    JET_RETINFO     retInfo         = { sizeof( JET_RETINFO ), ibData, itagSequence, 0 };
    LONG            cbKey           = 0;
    BYTE*           rgbKey          = NULL;

    Call( ErrIsamDupCursor( sesid, tableid, &tableidRetrieve, NO_GRBIT ) );

    cbKey = ((FUCB*)tableid)->cbEncryptionKey;
    if ( cbKey )
    {
        Alloc( rgbKey = new BYTE[cbKey] );
        Call( ErrIsamGetTableInfo( sesid, tableid, rgbKey, cbKey, JET_TblInfoEncryptionKey ) );
        Call( ErrIsamSetTableInfo( sesid, tableidRetrieve, rgbKey, cbKey, JET_TblInfoEncryptionKey ) );
    }

    Call( ErrIsamGotoBookmark( sesid, tableidRetrieve, rgbBookmark, cbBookmark ) );

    Call( ErrIsamIResolveColumnFromColumnReference( sesid, tableidRetrieve, rgchColumnName, cchColumnName, &columnid ) );

    Call( ErrIsamRetrieveColumn( sesid, tableidRetrieve, columnid, pvData, cbData, pcbActual, grbit, &retInfo ) );

HandleError:
    delete[] rgbKey;
    if ( tableidRetrieve != JET_tableidNil )
    {
        CallS( ErrIsamCloseTable( sesid, tableidRetrieve ) );
    }

    return err;
}
#endif

ERR VTAPI ErrIsamRetrieveColumnByReference(
    _In_ const JET_SESID                                            sesid,
    _In_ const JET_TABLEID                                          tableid,
    _In_reads_bytes_( cbReference ) const void * const              pvReference,
    _In_ const ULONG                                        cbReference,
    _In_ const ULONG                                        ibData,
    _Out_writes_bytes_to_opt_( cbData, min( cbData, *pcbActual ) ) void * const pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit )
{
    ERR         err                     = JET_errSuccess;
    PIB* const  ppib                    = (PIB * const)sesid;
    FUCB* const pfucb                   = (FUCB * const)tableid;
    BOOL        fTransactionStarted     = fFalse;
    BOOL        fRetrievedValue         = fFalse;
    LvId        lid                     = lidMin;
    const BYTE* rgbBookmark             = NULL;
    ULONG       cbBookmark              = NULL;
    const CHAR* rgchColumnName          = NULL;
    ULONG       cchColumnName           = NULL;
    ULONG       itagSequence            = 0;
#ifdef DEBUG
    BYTE*       rgbDataVerify           = NULL;
#endif DEBUG
    ULONG       cbActualT;
    ULONG&      cbActual                = pcbActual ? *pcbActual : cbActualT;
                cbActual                = 0;


    if ( ppib == ppibNil || pfucb == pfucbNil )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( cbData && !pvData )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( !pvData && !pcbActual )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( grbit & JET_bitRetrievePhysicalSize )
    {
        if ( ibData || pvData || cbData || !pcbActual )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }
    if ( FFUCBUpdatePrepared( pfucb ) )
    {
        Error( ErrERRCheck( JET_errInvalidOperation ) );
    }


    if ( grbit & ~( JET_bitRetrievePhysicalSize ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }


    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRNoLatch( ppib );
    Assert( FFUCBIndex( pfucb ) );
    Assert( !FFUCBSort( pfucb ) );
    
    if( FFMPIsTempDB( pfucb->ifmp ) )
    {
        Expected( fFalse );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    


    if ( 0 == ppib->Level() )
    {
        Call( ErrIsamBeginTransaction( sesid, 54276, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }


    Call( ErrRECIParseColumnReference(  pfucb,
                                        (const BYTE* const)pvReference,
                                        cbReference,
                                        &lid,
                                        &rgbBookmark,
                                        &cbBookmark,
                                        &rgchColumnName,
                                        &cchColumnName,
                                        &itagSequence ) );


    if ( lid != lidMin )
    {


        JET_COLUMNID columnid = JET_columnidNil;
        Call( ErrIsamIResolveColumnFromColumnReference( sesid, tableid, rgchColumnName, cchColumnName, &columnid ) );

        FIELD   fieldFixed;
        BOOL    fEncrypted  = fFalse;
        err = ErrRECIAccessColumn( pfucb, columnid, &fieldFixed, &fEncrypted );
        if ( err == JET_errColumnNotFound )
        {
            Error( ErrERRCheck( JET_errStaleColumnReference ) );
        }
        Call( err );
        if ( !( grbit & ( JET_bitRetrievePhysicalSize) ) && fEncrypted && pfucb->pbEncryptionKey == NULL )
        {
            Error( ErrERRCheck( JET_errColumnNoEncryptionKey ) );
        }


        Call( ErrLVPrereadLongValue( pfucb, lid, ibData, cbData ) );


        BOOL fExists = fFalse;
        Call( ErrRECDoesSLongFieldExist( pfucb, lid, &fExists ) );


        if ( fExists )
        {
            if ( grbit & JET_bitRetrievePhysicalSize )
            {
                QWORD   cbDataLogical   = 0;
                QWORD   cbDataPhysical  = 0;
                QWORD   cbOverhead      = 0;
                Call( ErrRECGetLVSize( pfucb, lid, fFalse, &cbDataLogical, &cbDataPhysical, &cbOverhead ) );
                cbActual = (ULONG)cbDataPhysical;
            }
            else
            {
                Call( ErrRECRetrieveSLongField( pfucb,
                                                lid,
                                                fTrue,
                                                ibData,
                                                fEncrypted,
                                                (BYTE*)pvData,
                                                cbData,
                                                &cbActual ) );
                if ( cbActual > cbData )
                {
                    err = ErrERRCheck( JET_wrnBufferTruncated );
                }

#ifdef DEBUG


                if ( !(BOOL)UlConfigOverrideInjection( 41988, fFalse ) )
                {
                    ERR errSave = err;
                    LvId lidCurrent = lidMin;
                    Call( ErrIsamIRetrieveColumnByReferenceSlowly(  sesid,
                                                                    tableid,
                                                                    rgbBookmark,
                                                                    cbBookmark,
                                                                    rgchColumnName,
                                                                    cchColumnName,
                                                                    itagSequence,
                                                                    0,
                                                                    &lidCurrent,
                                                                    sizeof( lidCurrent ),
                                                                    NULL,
                                                                    grbit | JET_bitRetrieveLongId ) );
                    Assert( err != JET_wrnColumnNull || lidCurrent == lidMin );

                    Alloc( rgbDataVerify = new BYTE[cbData] );
                    ULONG cbDataVerifyActual;
                    Call( ErrIsamIRetrieveColumnByReferenceSlowly(  sesid,
                                                                    tableid,
                                                                    rgbBookmark,
                                                                    cbBookmark,
                                                                    rgchColumnName,
                                                                    cchColumnName,
                                                                    itagSequence,
                                                                    ibData,
                                                                    rgbDataVerify,
                                                                    cbData,
                                                                    &cbDataVerifyActual,
                                                                    grbit ) );
                    Assert( cbActual == cbDataVerifyActual || lidCurrent != lid );
                    Assert( !pvData ||
                            !CmpData( pvData, min( cbData, cbActual ), rgbDataVerify, min( cbData, cbDataVerifyActual ) ) ||
                            lidCurrent != lid );
                    err = errSave;
                }

#endif
            }
            fRetrievedValue = fTrue;
        }
    }


    if ( !fRetrievedValue )
    {
        Error( ErrERRCheck( JET_errStaleColumnReference ) );
    }

HandleError:
    if ( fTransactionStarted )
    {
        CallS( ErrIsamCommitTransaction( sesid, NO_GRBIT, 0, NULL ) );
    }
    AssertDIRNoLatch( ppib );

#ifdef DEBUG
    delete[] rgbDataVerify;
#endif
    return err;
}

ERR VTAPI ErrIsamPrereadColumnsByReference(
    _In_ const JET_SESID                                    sesid,
    _In_ const JET_TABLEID                                  tableid,
    _In_reads_( cReferences ) const void * const * const    rgpvReferences,
    _In_reads_( cReferences ) const ULONG * const   rgcbReferences,
    _In_ const ULONG                                cReferences,
    _In_ const ULONG                                cPageCacheMin,
    _In_ const ULONG                                cPageCacheMax,
    _Out_opt_ ULONG * const                         pcReferencesPreread,
    _In_ const JET_GRBIT                                    grbit )
{
    ERR             err                     = JET_errSuccess;
    PIB* const      ppib                    = (PIB * const)sesid;
    FUCB* const     pfucb                   = (FUCB * const)tableid;
    ULONG           clid                    = 0;
    LvId*           rglid                   = NULL;
    ULONG           cPageCacheMinRemaining  = cPageCacheMin;
    ULONG           cPageCacheMaxRemaining  = cPageCacheMax;
    ULONG           cReferencesPrereadT;
    ULONG&          cReferencesPreread      = pcReferencesPreread ? *pcReferencesPreread : cReferencesPrereadT;
                    cReferencesPreread      = 0;


    if ( ppib == ppibNil || pfucb == pfucbNil || !rgpvReferences || !rgcbReferences )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( cPageCacheMin > cPageCacheMax )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( FFUCBUpdatePrepared( pfucb ) )
    {
        Error( ErrERRCheck( JET_errInvalidOperation ) );
    }


    if ( grbit & ~( JET_bitPrereadFirstPage ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }


    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRNoLatch( ppib );
    Assert( FFUCBIndex( pfucb ) );
    Assert( !FFUCBSort( pfucb ) );

    if( FFMPIsTempDB( pfucb->ifmp ) )
    {
        Expected( fFalse );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    

    for ( ULONG iReference = 0; iReference < cReferences; iReference++ )
    {
        LvId        lid             = lidMin;
        const BYTE* rgbBookmark     = NULL;
        ULONG       cbBookmark      = 0;
        const CHAR* rgchColumnName  = NULL;
        ULONG       cchColumnName   = 0;
        ULONG       itagSequence    = 0;
        Call( ErrRECIParseColumnReference(  pfucb,
                                            (const BYTE* const)rgpvReferences[iReference],
                                            rgcbReferences[iReference],
                                            &lid,
                                            &rgbBookmark,
                                            &cbBookmark,
                                            &rgchColumnName,
                                            &cchColumnName,
                                            &itagSequence ) );

        JET_COLUMNID columnid = JET_columnidNil;
        err = ErrIsamIResolveColumnFromColumnReference( sesid, tableid, rgchColumnName, cchColumnName, &columnid );
        if ( err == JET_errStaleColumnReference )
        {
            err = JET_errSuccess;
            continue;
        }
        Call( err );

        if ( lid != lidMin )
        {
            if ( !rglid )
            {
                Alloc( rglid = new LvId[cReferences] );
            }

            rglid[clid] = lid;

            clid++;
        }
    }


    for ( ULONG ilid = 0; ilid < clid && cPageCacheMaxRemaining; ilid += clidMostPreread )
    {
        LONG    clidPrereadActual   = 0;
        ULONG   cPageCacheActual    = 0;
        Call( ErrLVPrereadLongValues(   pfucb,
                                        rglid + ilid,
                                        min( clidMostPreread, clid - ilid ),
                                        cPageCacheMinRemaining,
                                        cPageCacheMaxRemaining,
                                        &clidPrereadActual,
                                        &cPageCacheActual,
                                        grbit & JET_bitPrereadFirstPage ) );

        cReferencesPreread += (ULONG)clidPrereadActual;
        cPageCacheMinRemaining = cPageCacheMinRemaining > cPageCacheActual ? cPageCacheMinRemaining - cPageCacheActual : 0;
        cPageCacheMaxRemaining = cPageCacheMaxRemaining > cPageCacheActual ? cPageCacheMaxRemaining - cPageCacheActual : 0;
    }

HandleError:
    AssertDIRNoLatch( ppib );

    delete[] rglid;
    return err;
}

LOCAL ERR ErrRECIFetchMissingLVs(
    FUCB*                   pfucb,
    ULONG*                  pcEnumColumn,
    JET_ENUMCOLUMN**        prgEnumColumn,
    CArray< LvId >&     rgLidStore,
    JET_PFNREALLOC          pfnRealloc,
    void*                   pvReallocContext,
    ULONG                   cbDataMost,
    BOOL                    fAfterImage,
    BOOL                    fRetrieveAsRefIfNotInRecord )
{
    ERR                     err                 = JET_errSuccess;
    ULONG                   cEnumColumnT        = 0;
    ULONG&                  cEnumColumn         = ( pcEnumColumn ? *pcEnumColumn : cEnumColumnT );
    JET_ENUMCOLUMN*         rgEnumColumnT       = NULL;
    JET_ENUMCOLUMN*&        rgEnumColumn        = ( prgEnumColumn ? *prgEnumColumn : rgEnumColumnT );
    size_t                  iEnumColumn;
    size_t                  iEnumColumnValue;


    if ( Pcsr( pfucb )->FLatched() )
    {
        CallS( ErrDIRRelease( pfucb ) );
    }
    AssertDIRNoLatch( pfucb->ppib );


    for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
    {
        JET_ENUMCOLUMN* const pEnumColumn = &rgEnumColumn[ iEnumColumn ];

        if ( pEnumColumn->err != JET_wrnColumnSingleValue )
        {
            for (   iEnumColumnValue = 0;
                    iEnumColumnValue < rgEnumColumn[ iEnumColumn ].cEnumColumnValue;
                    iEnumColumnValue++ )
            {
                Assert( NULL != pEnumColumn );
                JET_ENUMCOLUMNVALUE* const pEnumColumnValue = &pEnumColumn->rgEnumColumnValue[ iEnumColumnValue ];


                if ( pEnumColumnValue->err != wrnRECSeparatedLV && pEnumColumnValue->err != wrnRECSeparatedEncryptedLV )
                {
                    continue;
                }

                Assert( pEnumColumnValue->cbData < rgLidStore.Size() );
                const LvId lid = rgLidStore[ pEnumColumnValue->cbData ];
                const BOOL fEncrypted = ( pEnumColumnValue->err == wrnRECSeparatedEncryptedLV );

                pEnumColumnValue->err       = JET_errSuccess;
                pEnumColumnValue->cbData    = 0;


                if ( fRetrieveAsRefIfNotInRecord )
                {
                    pEnumColumnValue->err       = ErrERRCheck( JET_wrnColumnReference );
                    Call( ErrRECCreateColumnReference( pfucb,
                                                       pEnumColumn->columnid,
                                                       pEnumColumnValue->itagSequence,
                                                       lid,
                                                       &pEnumColumnValue->cbData,
                                                       (BYTE**)&pEnumColumnValue->pvData,
                                                       pfnRealloc,
                                                       pvReallocContext ) );
                }
                else
                {

                    Call( ErrRECRetrieveSLongField( pfucb,
                                                    lid,
                                                    fAfterImage,
                                                    0,
                                                    fEncrypted,
                                                    (BYTE*)&pEnumColumnValue->pvData,
                                                    cbDataMost,
                                                    &pEnumColumnValue->cbData,
                                                    pfnRealloc,
                                                    pvReallocContext,
                                                    prceNil ) );


                    if ( pEnumColumnValue->cbData > cbDataMost )
                    {
                        pEnumColumnValue->err       = JET_wrnColumnTruncated;
                        pEnumColumnValue->cbData    = cbDataMost;
                    }
                }
            }
        }
    }

HandleError:
    return err;
}

LOCAL ERR ErrRECEnumerateAllColumns(
    FUCB*                   pfucb,
    ULONG*                  pcEnumColumn,
    JET_ENUMCOLUMN**        prgEnumColumn,
    JET_PFNREALLOC          pfnRealloc,
    void*                   pvReallocContext,
    ULONG                   cbDataMost,
    JET_GRBIT               grbit )
{
    ERR                     err                         = JET_errSuccess;
    ULONG                   cEnumColumnT                = 0;
    ULONG&                  cEnumColumn                 = ( pcEnumColumn ? *pcEnumColumn : cEnumColumnT );
                            cEnumColumn                 = 0;
    JET_ENUMCOLUMN*         rgEnumColumnT               = NULL;
    JET_ENUMCOLUMN*&        rgEnumColumn                = ( prgEnumColumn ? *prgEnumColumn : rgEnumColumnT );
                            rgEnumColumn                = NULL;
    CArray<LvId>            rgEnumLidStore;
    const INT               iMinGrowthLidStore          = 8;


    TDB* const                  ptdb                    = pfucb->u.pfcb->Ptdb();
    TDB* const                  ptdbTemplate            = ( ptdb->PfcbTemplateTable() != pfcbNil ?
                                                                ptdb->PfcbTemplateTable()->Ptdb() :
                                                                ptdbNil );

    BOOL                    fSeparatedLV                = fFalse;
    BOOL                    fEncrypted                  = fFalse;
    BOOL                    fRecord                     = fFalse;
    BOOL                    fDefaultRecord              = fFalse;
    BOOL                    fNonEscrowDefault           = fFalse;
    BOOL                    fTaggedOnly                 = fFalse;
    BOOL                    fInRecordOnly               = fFalse;
    BOOL                    fRetrieveAsRefIfNotInRecord = fFalse;
    BOOL                    fHasFixed                   = fFalse;
    BOOL                    fHasVariable                = fFalse;
    BOOL                    fHasTagged                  = fFalse;

    CFixedColumnIter        rgciterFC[ 2 ];
    size_t                  iciterFC                    = 0;
    CFixedColumnIter*       pciterFC                    = NULL;
    CVariableColumnIter     rgciterVC[ 2 ];
    size_t                  iciterVC                    = 0;
    CVariableColumnIter*    pciterVC                    = NULL;
    CTaggedColumnIter       rgciterTC[ 2 ];
    size_t                  iciterTC                    = 0;
    CTaggedColumnIter*      pciterTC                    = NULL;
    CUnionIter              rgciterU[ 5 ];
    size_t                  iciterU                     = 0;
    CUnionIter*             pciterU                     = NULL;

    IColumnIter*            pciterRec                   = NULL;
    IColumnIter*            pciterDefaultRec            = NULL;
    IColumnIter*            pciterRoot                  = NULL;

    BOOL                    fUseCopyBuffer              = fFalse;
    DATA*                   pdataRec                    = NULL;
    DATA*                   pdataDefaultRec             = NULL;

    size_t                  cAlloc;
    size_t                  cbAlloc;
    size_t                  iEnumColumn;
    size_t                  iEnumColumnValue;
    FIELD                   fieldFC;
    size_t                  cColumnValue                = 0;
    BOOL                    fSeparated                  = fFalse;
    BOOL                    fCompressed                 = fFalse;
    size_t                  cbData                      = 0;
    void*                   pvData                      = NULL;
    BYTE *                  pbDataDecrypted             = NULL;


    if ( !pcEnumColumn || !prgEnumColumn || !pfnRealloc )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( grbit & ~( JET_bitEnumerateCopy |
                    JET_bitEnumerateIgnoreDefault |
                    JET_bitEnumerateLocal |
                    JET_bitEnumeratePresenceOnly |
                    JET_bitEnumerateTaggedOnly |
                    JET_bitEnumerateCompressOutput |
                    JET_bitEnumerateIgnoreUserDefinedDefault |
                    JET_bitEnumerateInRecordOnly |
                    JET_bitEnumerateAsRefIfNotInRecord ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }


    if ( grbit & JET_bitEnumerateLocal )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }


    if (    ptdb->FTableHasUserDefinedDefault() &&
            !( grbit & ( JET_bitEnumerateIgnoreDefault | JET_bitEnumerateIgnoreUserDefinedDefault ) ) )
    {
        Error( ErrERRCheck( JET_errCallbackFailed ) );
    }


    if ( !!( grbit & JET_bitEnumerateInRecordOnly ) && !!( grbit & JET_bitEnumerateAsRefIfNotInRecord ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }


    fRecord                     = fTrue;
    fDefaultRecord              = !( grbit & JET_bitEnumerateIgnoreDefault ) && ptdb->FTableHasDefault();
    fNonEscrowDefault           = ptdb->FTableHasNonEscrowDefault();
    fTaggedOnly                 = grbit & JET_bitEnumerateTaggedOnly;
    fInRecordOnly               = grbit & JET_bitEnumerateInRecordOnly;
    fRetrieveAsRefIfNotInRecord = grbit & JET_bitEnumerateAsRefIfNotInRecord;
    
    fHasFixed                   = ptdb->CFixedColumns() > 0 || ptdbTemplate != ptdbNil && ptdbTemplate->CFixedColumns() > 0;
    fHasVariable                = ptdb->CVarColumns() > 0 || ptdbTemplate != ptdbNil && ptdbTemplate->CVarColumns() > 0;
    fHasTagged                  = ptdb->CTaggedColumns() > 0 || ptdbTemplate != ptdbNil && ptdbTemplate->CTaggedColumns() > 0;

    if ( fRecord )
    {
        if ( fHasVariable && !fTaggedOnly )
        {
            pciterVC = &rgciterVC[iciterVC++];
            Call( pciterVC->ErrInit( pfucb->u.pfcb ) );
            if ( !pciterRec )
            {
                pciterRec = pciterVC;
            }
            else
            {
                pciterU = &rgciterU[iciterU++];
                Call( pciterU->ErrInit( pciterVC, pciterRec ) );
                pciterRec = pciterU;
            }
        }

        if ( fHasFixed && !fTaggedOnly )
        {
            pciterFC = &rgciterFC[iciterFC++];
            Call( pciterFC->ErrInit( pfucb->u.pfcb ) );
            if ( !pciterRec )
            {
                pciterRec = pciterFC;
            }
            else
            {
                pciterU = &rgciterU[iciterU++];
                Call( pciterU->ErrInit( pciterFC, pciterRec ) );
                pciterRec = pciterU;
            }
        }

        if ( fHasTagged )
        {
            pciterTC = &rgciterTC[iciterTC++];
            Call( pciterTC->ErrInit( pfucb->u.pfcb ) );
            if ( !pciterRec )
            {
                pciterRec = pciterTC;
            }
            else
            {
                pciterU = &rgciterU[iciterU++];
                Call( pciterU->ErrInit( pciterTC, pciterRec ) );
                pciterRec = pciterU;
            }
        }

        if ( !pciterRoot )
        {
            pciterRoot = pciterRec;
        }
        else
        {
            pciterU = &rgciterU[iciterU++];
            Call( pciterU->ErrInit( pciterRoot, pciterRec ) );
            pciterRoot = pciterU;
        }
    }

    if ( fDefaultRecord )
    {
        if ( fHasVariable && !fTaggedOnly && fNonEscrowDefault )
        {
            pciterVC = &rgciterVC[iciterVC++];
            Call( pciterVC->ErrInit( pfucb->u.pfcb ) );
            if ( !pciterDefaultRec )
            {
                pciterDefaultRec = pciterVC;
            }
            else
            {
                pciterU = &rgciterU[iciterU++];
                Call( pciterU->ErrInit( pciterVC, pciterDefaultRec ) );
                pciterDefaultRec = pciterU;
            }
        }

        if ( fHasFixed && !fTaggedOnly )
        {
            pciterFC = &rgciterFC[iciterFC++];
            Call( pciterFC->ErrInit( pfucb->u.pfcb ) );
            if ( !pciterDefaultRec )
            {
                pciterDefaultRec = pciterFC;
            }
            else
            {
                pciterU = &rgciterU[iciterU++];
                Call( pciterU->ErrInit( pciterFC, pciterDefaultRec ) );
                pciterDefaultRec = pciterU;
            }
        }

        if ( fHasTagged && fNonEscrowDefault )
        {
            pciterTC = &rgciterTC[iciterTC++];
            Call( pciterTC->ErrInit( pfucb->u.pfcb ) );
            if ( !pciterDefaultRec )
            {
                pciterDefaultRec = pciterTC;
            }
            else
            {
                pciterU = &rgciterU[iciterU++];
                Call( pciterU->ErrInit( pciterTC, pciterDefaultRec ) );
                pciterDefaultRec = pciterU;
            }
        }

        if ( pciterDefaultRec )
        {
            if ( !pciterRoot )
            {
                pciterRoot = pciterDefaultRec;
            }
            else
            {
                pciterU = &rgciterU[iciterU++];
                Call( pciterU->ErrInit( pciterRoot, pciterDefaultRec ) );
                pciterRoot = pciterU;
            }
        }
    }


    if ( fRecord )
    {
        fUseCopyBuffer = (  (   ( grbit & JET_bitEnumerateCopy ) &&
                                FFUCBUpdatePrepared( pfucb ) &&
                                !FFUCBNeverRetrieveCopy( pfucb ) ) ||
                            FFUCBAlwaysRetrieveCopy( pfucb ) );

        Call( ErrRECIGetRecord( pfucb, &pdataRec, fUseCopyBuffer ) );
        Call( pciterRec->ErrSetRecord( *pdataRec ) );
    }
    if ( fDefaultRecord && pciterDefaultRec )
    {
        pdataDefaultRec = ptdb->PdataDefaultRecord();
        Call( pciterDefaultRec->ErrSetRecord( *pdataDefaultRec ) );
    }


    if (    fRetrieveAsRefIfNotInRecord &&
            ( FFUCBUpdatePrepared( pfucb ) || FFUCBSort( pfucb ) || FFMPIsTempDB( pfucb->ifmp ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }


    Call( pciterRoot->ErrGetWorstCaseColumnCount( &cAlloc ) );
    cbAlloc = cAlloc * sizeof( JET_ENUMCOLUMN );

    Alloc( rgEnumColumn = (JET_ENUMCOLUMN*)pfnRealloc(
            pvReallocContext,
            NULL,
            cbAlloc ) );

    cEnumColumn = (ULONG)cAlloc;
    memset( rgEnumColumn, 0, cbAlloc );

    if ( rgEnumLidStore.ErrSetCapacity( cEnumColumn ) != CArray<LvId>::ERR::errSuccess )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }
    rgEnumLidStore.SetCapacityGrowth( iMinGrowthLidStore );


    Call( pciterRoot->ErrMoveBeforeFirst() );
    for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
    {
        JET_ENUMCOLUMN* const pEnumColumn = &rgEnumColumn[ iEnumColumn ];


        err = pciterRoot->ErrMoveNext();
        if ( err == errRECNoCurrentColumnValue )
        {
            err = JET_errSuccess;
            cEnumColumn = iEnumColumn;
            continue;
        }
        Call( err );


        Call( pciterRoot->ErrGetColumnId( &pEnumColumn->columnid ) );

        err = ErrRECIAccessColumn( pfucb, pEnumColumn->columnid, &fieldFC, &fEncrypted );
        if ( err == JET_errColumnNotFound )
        {
            err = JET_errSuccess;
            iEnumColumn--;
            continue;
        }
        Call( err );


        Call( pciterRoot->ErrGetColumnValueCount( &cColumnValue ) );


        if ( !cColumnValue )
        {
            pEnumColumn->err = ErrERRCheck( JET_wrnColumnNull );
            continue;
        }


        if ( grbit & JET_bitEnumeratePresenceOnly )
        {
            pEnumColumn->err = ErrERRCheck( JET_wrnColumnPresent );
            continue;
        }

        if ( fEncrypted && pfucb->pbEncryptionKey == NULL )
        {
            pEnumColumn->err = ErrERRCheck( JET_errColumnNoEncryptionKey );
            continue;
        }


        if (    cColumnValue == 1 &&
                !fEncrypted &&
                ( grbit & JET_bitEnumerateCompressOutput ) )
        {

            Call( pciterRoot->ErrGetColumnValue( 1, &cbData, &pvData, &fSeparated, &fCompressed ) );


            if ( !fSeparated )
            {

                if( fCompressed )
                {
                    DATA data;
                    data.SetPv( pvData );
                    data.SetCb( cbData );
                    INT cbDataActual;
                    Call( ErrPKDecompressData( data, pfucb, NULL, 0, &cbDataActual ) );
                    pEnumColumn->err = ErrERRCheck( JET_wrnColumnSingleValue );
                    Alloc( pEnumColumn->pvData = pfnRealloc( pvReallocContext, NULL, cbDataActual ) );
                    pEnumColumn->cbData = (ULONG)cbDataActual;
                    Call( ErrPKDecompressData( data, pfucb, (BYTE *) pEnumColumn->pvData, pEnumColumn->cbData, &cbDataActual ) );
                    Assert( JET_wrnBufferTruncated != err );
                }
                else
                {
                    pEnumColumn->err = ErrERRCheck( JET_wrnColumnSingleValue );
                    Alloc( pEnumColumn->pvData = pfnRealloc( pvReallocContext, NULL, cbData ) );
                    pEnumColumn->cbData = (ULONG)cbData;

                    if ( !FHostIsLittleEndian() && FCOLUMNIDFixed( pEnumColumn->columnid ) )
                    {
                        switch ( fieldFC.cbMaxLen )
                        {
                            case 1:
                                *((BYTE*)pEnumColumn->pvData) = *((UnalignedLittleEndian< BYTE >*)pvData);
                                break;

                            case 2:
                                *((USHORT*)pEnumColumn->pvData) = *((UnalignedLittleEndian< USHORT >*)pvData);
                                break;

                            case 4:
                                *((ULONG*)pEnumColumn->pvData) = *((UnalignedLittleEndian< ULONG >*)pvData);
                                break;

                            case 8:
                                *((QWORD*)pEnumColumn->pvData) = *((UnalignedLittleEndian< QWORD >*)pvData);
                                break;

                            default:
                                Assert( fFalse );
                                Error( ErrERRCheck( JET_errInternalError ) );
                                break;
                        }
                    }
                    else
                    {
                        memcpy( pEnumColumn->pvData,
                                pvData,
                                pEnumColumn->cbData );
                    }
                }


                if (    FCOLUMNIDFixed( pEnumColumn->columnid ) &&
                        FFIELDEscrowUpdate( fieldFC.ffield ) &&
                        !FFUCBInsertPrepared( pfucb ) )
                {
                    Call( ErrRECAdjustEscrowedColumn(   pfucb,
                                                        pEnumColumn->columnid,
                                                        fieldFC,
                                                        pEnumColumn->pvData,
                                                        pEnumColumn->cbData ) );
                }


                continue;
            }
        }


        cAlloc  = cColumnValue;
        cbAlloc = cAlloc * sizeof( JET_ENUMCOLUMNVALUE );

        Alloc( pEnumColumn->rgEnumColumnValue = (JET_ENUMCOLUMNVALUE*)pfnRealloc(
                pvReallocContext,
                NULL,
                cbAlloc ) );

        pEnumColumn->cEnumColumnValue = (ULONG)cAlloc;
        memset( pEnumColumn->rgEnumColumnValue, 0, cbAlloc );

        rgEnumLidStore.SetCapacityGrowth( max( iMinGrowthLidStore, cColumnValue ) );


        for (   iEnumColumnValue = 0;
                iEnumColumnValue < pEnumColumn->cEnumColumnValue;
                iEnumColumnValue++ )
        {
            JET_ENUMCOLUMNVALUE* const pEnumColumnValue = &pEnumColumn->rgEnumColumnValue[ iEnumColumnValue ];

            pEnumColumnValue->itagSequence = (ULONG)( iEnumColumnValue + 1 );


            Call( pciterRoot->ErrGetColumnValue( pEnumColumnValue->itagSequence, &cbData, &pvData, &fSeparated, &fCompressed ) );


            if ( fSeparated && fInRecordOnly )
            {
                pEnumColumnValue->err = ErrERRCheck( JET_wrnColumnNotInRecord );
                continue;
            }


            if ( fSeparated )
            {
                INT iLid = rgEnumLidStore.Size();
                if ( rgEnumLidStore.ErrSetEntry( iLid, LidOfSeparatedLV( (BYTE*)pvData, cbData ) ) != CArray<LvId>::ERR::errSuccess )
                {
                    Error( ErrERRCheck( JET_errOutOfMemory ) );
                }

                pEnumColumnValue->err = fEncrypted ? ErrERRCheck( wrnRECSeparatedEncryptedLV ) : ErrERRCheck( wrnRECSeparatedLV );
                pEnumColumnValue->cbData = iLid;

                fSeparatedLV = fTrue;
                continue;
            }


            if ( fEncrypted &&
                 cbData > 0 )
            {
                Alloc( pbDataDecrypted = new BYTE[ cbData ] );
                ULONG cbDataDecryptedActual = cbData;
                Call( ErrOSUDecrypt(
                            (BYTE*)pvData,
                            pbDataDecrypted,
                            (ULONG *)&cbDataDecryptedActual,
                            pfucb->pbEncryptionKey,
                            pfucb->cbEncryptionKey,
                            PinstFromPfucb( pfucb )->m_iInstance,
                            pfucb->u.pfcb->TCE() ) );
                pvData = pbDataDecrypted;
                cbData = cbDataDecryptedActual;
            }

            if( fCompressed )
            {
                DATA data;
                data.SetPv( pvData );
                data.SetCb( cbData );
                INT cbDataActual;
                Call( ErrPKDecompressData( data, pfucb, NULL, 0, &cbDataActual ) );
                Alloc( pEnumColumnValue->pvData = pfnRealloc( pvReallocContext, NULL, cbDataActual ) );
                pEnumColumnValue->cbData = (ULONG)cbDataActual;
                Call( ErrPKDecompressData( data, pfucb, (BYTE *) pEnumColumnValue->pvData, pEnumColumnValue->cbData, &cbDataActual ) );
                Assert( JET_wrnBufferTruncated != err );
            }
            else
            {
                Alloc( pEnumColumnValue->pvData = pfnRealloc( pvReallocContext, NULL, cbData ) );
                pEnumColumnValue->cbData = (ULONG)cbData;

                if ( !FHostIsLittleEndian() && FCOLUMNIDFixed( pEnumColumn->columnid ) )
                {
                    switch ( fieldFC.cbMaxLen )
                    {
                        case 1:
                            *((BYTE*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< BYTE >*)pvData);
                            break;

                        case 2:
                            *((USHORT*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< USHORT >*)pvData);
                            break;

                        case 4:
                            *((ULONG*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< ULONG >*)pvData);
                            break;

                        case 8:
                            *((QWORD*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< QWORD >*)pvData);
                            break;

                        default:
                            Assert( fFalse );
                            Error( ErrERRCheck( JET_errInternalError ) );
                            break;
                    }
                }
                else
                {
                    memcpy( pEnumColumnValue->pvData,
                            pvData,
                            pEnumColumnValue->cbData );
                }
            }

            if ( pbDataDecrypted )
            {
                delete[] pbDataDecrypted;
                pbDataDecrypted = NULL;
            }


            if (    FCOLUMNIDFixed( pEnumColumn->columnid ) &&
                    FFIELDEscrowUpdate( fieldFC.ffield ) &&
                    !FFUCBInsertPrepared( pfucb ) )
            {
                Call( ErrRECAdjustEscrowedColumn(   pfucb,
                                                    pEnumColumn->columnid,
                                                    fieldFC,
                                                    pEnumColumnValue->pvData,
                                                    pEnumColumnValue->cbData ) );
            }
        }
    }


    Assert( pciterRoot->ErrMoveNext() == errRECNoCurrentColumnValue );


    if ( fSeparatedLV )
    {
        const BOOL fAfterImage = fUseCopyBuffer
                                || !FFUCBUpdateSeparateLV( pfucb )
                                || !FFUCBReplacePrepared( pfucb );

        Call( ErrRECIFetchMissingLVs(   pfucb,
                                        pcEnumColumn,
                                        prgEnumColumn,
                                        rgEnumLidStore,
                                        pfnRealloc,
                                        pvReallocContext,
                                        cbDataMost,
                                        fAfterImage,
                                        fRetrieveAsRefIfNotInRecord ) );
    }


HandleError:
    if ( Pcsr( pfucb )->FLatched() )
    {
        CallS( ErrDIRRelease( pfucb ) );
    }
    AssertDIRNoLatch( pfucb->ppib );

    if ( pbDataDecrypted )
    {
        delete[] pbDataDecrypted;
        pbDataDecrypted = NULL;
    }

    if ( err < JET_errSuccess )
    {
        if ( prgEnumColumn )
        {
            for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
            {
                if ( rgEnumColumn[ iEnumColumn ].err != JET_wrnColumnSingleValue )
                {
                    for (   iEnumColumnValue = 0;
                            iEnumColumnValue < rgEnumColumn[ iEnumColumn ].cEnumColumnValue;
                            iEnumColumnValue++ )
                    {
                        if ( rgEnumColumn[ iEnumColumn ].rgEnumColumnValue[ iEnumColumnValue ].pvData )
                        {
                            pfnRealloc( pvReallocContext,
                                        rgEnumColumn[ iEnumColumn ].rgEnumColumnValue[ iEnumColumnValue ].pvData,
                                        0 );
                        }
                    }
                    if ( rgEnumColumn[ iEnumColumn ].rgEnumColumnValue )
                    {
                        pfnRealloc( pvReallocContext,
                                    rgEnumColumn[ iEnumColumn ].rgEnumColumnValue,
                                    0 );
                    }
                }
                else
                {
                    if ( rgEnumColumn[ iEnumColumn ].pvData )
                    {
                        pfnRealloc( pvReallocContext,
                                    rgEnumColumn[ iEnumColumn ].pvData,
                                    0 );
                    }
                }
            }
            if ( rgEnumColumn )
            {
                pfnRealloc( pvReallocContext, rgEnumColumn, 0 );
                rgEnumColumn = NULL;
            }
        }
        if ( pcEnumColumn )
        {
            cEnumColumn = 0;
        }
    }
    return err;
}

LOCAL ERR ErrRECEnumerateReqColumns(
    FUCB*                   pfucb,
    ULONG                   cEnumColumnId,
    JET_ENUMCOLUMNID*       rgEnumColumnId,
    ULONG*                  pcEnumColumn,
    JET_ENUMCOLUMN**        prgEnumColumn,
    JET_PFNREALLOC          pfnRealloc,
    void*                   pvReallocContext,
    ULONG                   cbDataMost,
    JET_GRBIT               grbit )
{
    ERR                     err                         = JET_errSuccess;
    ULONG                   cEnumColumnT                = 0;
    ULONG&                  cEnumColumn                 = ( pcEnumColumn ? *pcEnumColumn : cEnumColumnT );
                            cEnumColumn                 = 0;
    JET_ENUMCOLUMN*         rgEnumColumnT               = NULL;
    JET_ENUMCOLUMN*&        rgEnumColumn                = ( prgEnumColumn ? *prgEnumColumn : rgEnumColumnT );
                            rgEnumColumn                = NULL;
    CArray<LvId>            rgEnumLidStore;
    const INT               iMinGrowthLidStore          = 8;
    BOOL                    fColumnIdError = fFalse;
    BOOL                    fRetColumnIdError           = fFalse;
    BOOL                    fSeparatedLV                = fFalse;
    BOOL                    fEncrypted                  = fFalse;
    BOOL                    fInRecordOnly               = fFalse;
    BOOL                    fRetrieveAsRefIfNotInRecord = fFalse;
    size_t                  iEnumColumn                 = 0;
    size_t                  iEnumColumnValue            = 0;
    BOOL                    fUseCopyBuffer              = fFalse;
    DATA*                   pdataRec                    = NULL;
    DATA*                   pdataDefaultRec             = NULL;
    BOOL                    fNonEscrowDefault           = fFalse;
    size_t                  cAlloc;
    size_t                  cbAlloc;
    FIELD                   fieldFC;
    CTaggedColumnIter       rgciterTC[ 2 ];
    CTaggedColumnIter*      pciterTC                    = NULL;
    CTaggedColumnIter*      pciterTCDefault             = NULL;
    CFixedColumnIter        rgciterFC[ 2 ];
    CFixedColumnIter*       pciterFC                    = NULL;
    CFixedColumnIter*       pciterFCDefault             = NULL;
    CVariableColumnIter     rgciterVC[ 2 ];
    CVariableColumnIter*    pciterVC                    = NULL;
    CVariableColumnIter*    pciterVCDefault             = NULL;
    IColumnIter*            pciter                      = NULL;
    BOOL                    fColumnFound                = fFalse;
    size_t                  cColumnValue                = 0;
    BOOL                    fSeparated                  = fFalse;
    BOOL                    fCompressed                 = fFalse;
    size_t                  cbData                      = 0;
    void*                   pvData                      = NULL;
    BYTE *                  pbDataDecrypted             = NULL;


    if ( !pcEnumColumn || !prgEnumColumn || !pfnRealloc )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( grbit & ~( JET_bitEnumerateCopy |
                    JET_bitEnumerateIgnoreDefault |
                    JET_bitEnumerateLocal |
                    JET_bitEnumeratePresenceOnly |
                    JET_bitEnumerateTaggedOnly |
                    JET_bitEnumerateCompressOutput |
                    JET_bitEnumerateIgnoreUserDefinedDefault |
                    JET_bitEnumerateInRecordOnly |
                    JET_bitEnumerateAsRefIfNotInRecord ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }


    if ( grbit & JET_bitEnumerateLocal )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }


    if ( grbit & JET_bitEnumerateTaggedOnly )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }


    if (    pfucb->u.pfcb->Ptdb()->FTableHasUserDefinedDefault() &&
            !( grbit & ( JET_bitEnumerateIgnoreDefault | JET_bitEnumerateIgnoreUserDefinedDefault ) ) )
    {
        Error( ErrERRCheck( JET_errCallbackFailed ) );
    }


    if ( !!( grbit & JET_bitEnumerateInRecordOnly ) && !!( grbit & JET_bitEnumerateAsRefIfNotInRecord ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    fInRecordOnly               = grbit & JET_bitEnumerateInRecordOnly;
    fRetrieveAsRefIfNotInRecord = grbit & JET_bitEnumerateAsRefIfNotInRecord;


    fUseCopyBuffer = (  (   ( grbit & JET_bitEnumerateCopy ) &&
                            FFUCBUpdatePrepared( pfucb ) &&
                            !FFUCBNeverRetrieveCopy( pfucb ) ) ||
                        FFUCBAlwaysRetrieveCopy( pfucb ) );

    Call( ErrRECIGetRecord( pfucb, &pdataRec, fUseCopyBuffer ) );

    if ( !( grbit & JET_bitEnumerateIgnoreDefault ) && pfucb->u.pfcb->Ptdb()->FTableHasDefault() )
    {
        pdataDefaultRec = pfucb->u.pfcb->Ptdb()->PdataDefaultRecord();
        fNonEscrowDefault = pfucb->u.pfcb->Ptdb()->FTableHasNonEscrowDefault();
    }


    if (    fRetrieveAsRefIfNotInRecord &&
            ( FFUCBUpdatePrepared( pfucb ) || FFUCBSort( pfucb ) || FFMPIsTempDB( pfucb->ifmp ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }


    cAlloc  = cEnumColumnId;
    cbAlloc = cAlloc * sizeof( JET_ENUMCOLUMN );

    Alloc( rgEnumColumn = (JET_ENUMCOLUMN*)pfnRealloc(
            pvReallocContext,
            NULL,
            cbAlloc ) );

    cEnumColumn = (ULONG)cAlloc;
    memset( rgEnumColumn, 0, cbAlloc );

    if ( rgEnumLidStore.ErrSetCapacity( cEnumColumn ) != CArray<LvId>::ERR::errSuccess )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }
    rgEnumLidStore.SetCapacityGrowth( iMinGrowthLidStore );

    for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
    {
        JET_ENUMCOLUMN* const pEnumColumn = &rgEnumColumn[ iEnumColumn ];


        pEnumColumn->columnid = rgEnumColumnId[ iEnumColumn ].columnid;

        if ( !pEnumColumn->columnid )
        {
            pEnumColumn->err = ErrERRCheck( JET_wrnColumnSkipped );
            continue;
        }


        err = ErrRECIAccessColumn( pfucb, pEnumColumn->columnid, &fieldFC, &fEncrypted );
        if ( err == JET_errBadColumnId || err == JET_errColumnNotFound )
        {
            pEnumColumn->err = err;

            fColumnIdError = fTrue;

            err = JET_errSuccess;
            continue;
        }
        Call( err );


        if ( FCOLUMNIDTagged( pEnumColumn->columnid ) )
        {
            if ( !pciterTC )
            {
                pciterTC = &rgciterTC[ 0 ];
                Call( pciterTC->ErrInit( pfucb->u.pfcb ) );
                Call( pciterTC->ErrSetRecord( *pdataRec ) );
            }
            err = pciterTC->ErrSeek( pEnumColumn->columnid );
            pciter = pciterTC;
            if ( err == errRECColumnNotFound && fNonEscrowDefault )
            {
                if ( !pciterTCDefault )
                {
                    pciterTCDefault = &rgciterTC[ 1 ];
                    Call( pciterTCDefault->ErrInit( pfucb->u.pfcb ) );
                    Call( pciterTCDefault->ErrSetRecord( *pdataDefaultRec ) );
                }
                err = pciterTCDefault->ErrSeek( pEnumColumn->columnid );
                pciter = pciterTCDefault;
            }
        }
        else if ( FCOLUMNIDFixed( pEnumColumn->columnid ) )
        {
            if ( !pciterFC )
            {
                pciterFC = &rgciterFC[ 0 ];
                Call( pciterFC->ErrInit( pfucb->u.pfcb ) );
                Call( pciterFC->ErrSetRecord( *pdataRec ) );
            }
            err = pciterFC->ErrSeek( pEnumColumn->columnid );
            pciter = pciterFC;
            if ( err == errRECColumnNotFound && pdataDefaultRec )
            {
                if ( !pciterFCDefault )
                {
                    pciterFCDefault = &rgciterFC[ 1 ];
                    Call( pciterFCDefault->ErrInit( pfucb->u.pfcb ) );
                    Call( pciterFCDefault->ErrSetRecord( *pdataDefaultRec ) );
                }
                err = pciterFCDefault->ErrSeek( pEnumColumn->columnid );
                pciter = pciterFCDefault;
            }
        }
        else
        {
            if ( !pciterVC )
            {
                pciterVC = &rgciterVC[ 0 ];
                Call( pciterVC->ErrInit( pfucb->u.pfcb ) );
                Call( pciterVC->ErrSetRecord( *pdataRec ) );
            }
            err = pciterVC->ErrSeek( pEnumColumn->columnid );
            pciter = pciterVC;
            if ( err == errRECColumnNotFound && fNonEscrowDefault )
            {
                if ( !pciterVCDefault )
                {
                    pciterVCDefault = &rgciterVC[ 1 ];
                    Call( pciterVCDefault->ErrInit( pfucb->u.pfcb ) );
                    Call( pciterVCDefault->ErrSetRecord( *pdataDefaultRec ) );
                }
                err = pciterVCDefault->ErrSeek( pEnumColumn->columnid );
                pciter = pciterVCDefault;
            }
        }
        if ( err != errRECColumnNotFound )
        {
            Call( err );
        }


        if ( err == errRECColumnNotFound )
        {
            fColumnFound    = fFalse;
            cColumnValue    = 0;
            err             = JET_errSuccess;
        }
        else
        {
            fColumnFound    = fTrue;
            Call( pciter->ErrGetColumnValueCount( &cColumnValue ) );
        }


        if (    !fColumnFound && ( grbit & JET_bitEnumerateIgnoreDefault ) &&
                !rgEnumColumnId[ iEnumColumn ].ctagSequence )
        {
            pEnumColumn->err = ErrERRCheck( JET_wrnColumnDefault );
            continue;
        }


        if (    !cColumnValue &&
                !rgEnumColumnId[ iEnumColumn ].ctagSequence )
        {
            pEnumColumn->err = ErrERRCheck( JET_wrnColumnNull );
            continue;
        }


        if (    ( grbit & JET_bitEnumeratePresenceOnly ) &&
                !rgEnumColumnId[ iEnumColumn ].ctagSequence )
        {
            pEnumColumn->err = ErrERRCheck( JET_wrnColumnPresent );
            continue;
        }

        if ( fEncrypted && pfucb->pbEncryptionKey == NULL )
        {
            pEnumColumn->err = ErrERRCheck( JET_errColumnNoEncryptionKey );
            continue;
        }


        if (    cColumnValue == 1 &&
                !fEncrypted &&
                ( grbit & JET_bitEnumerateCompressOutput ) &&
                !rgEnumColumnId[ iEnumColumn ].ctagSequence )
        {

            Call( pciter->ErrGetColumnValue( 1, &cbData, &pvData, &fSeparated, &fCompressed ) );


            if ( !fSeparated )
            {

                if( fCompressed )
                {
                    DATA data;
                    data.SetPv( pvData );
                    data.SetCb( cbData );
                    INT cbDataActual;
                    Call( ErrPKDecompressData( data, pfucb, NULL, 0, &cbDataActual ) );
                    pEnumColumn->err = ErrERRCheck( JET_wrnColumnSingleValue );
                    Alloc( pEnumColumn->pvData = pfnRealloc( pvReallocContext, NULL, cbDataActual ) );
                    pEnumColumn->cbData = (ULONG)cbDataActual;
                    Call( ErrPKDecompressData( data, pfucb, (BYTE *) pEnumColumn->pvData, pEnumColumn->cbData, &cbDataActual ) );
                    Assert( JET_wrnBufferTruncated != err );
                }
                else
                {
                    pEnumColumn->err = ErrERRCheck( JET_wrnColumnSingleValue );
                    Alloc( pEnumColumn->pvData = pfnRealloc( pvReallocContext, NULL, cbData ) );
                    pEnumColumn->cbData = (ULONG)cbData;

                    if ( !FHostIsLittleEndian() && FCOLUMNIDFixed( pEnumColumn->columnid ) )
                    {
                        switch ( fieldFC.cbMaxLen )
                        {
                            case 1:
                                *((BYTE*)pEnumColumn->pvData) = *((UnalignedLittleEndian< BYTE >*)pvData);
                                break;

                            case 2:
                                *((USHORT*)pEnumColumn->pvData) = *((UnalignedLittleEndian< USHORT >*)pvData);
                                break;

                            case 4:
                                *((ULONG*)pEnumColumn->pvData) = *((UnalignedLittleEndian< ULONG >*)pvData);
                                break;

                            case 8:
                                *((QWORD*)pEnumColumn->pvData) = *((UnalignedLittleEndian< QWORD >*)pvData);
                                break;

                            default:
                                Assert( fFalse );
                                Error( ErrERRCheck( JET_errInternalError ) );
                                break;
                        }
                    }
                    else
                    {
                        memcpy( pEnumColumn->pvData,
                                pvData,
                                pEnumColumn->cbData );
                    }
                }


                if (    FCOLUMNIDFixed( pEnumColumn->columnid ) &&
                        FFIELDEscrowUpdate( fieldFC.ffield ) &&
                        !FFUCBInsertPrepared( pfucb ) )
                {
                    Call( ErrRECAdjustEscrowedColumn(   pfucb,
                                                        pEnumColumn->columnid,
                                                        fieldFC,
                                                        pEnumColumn->pvData,
                                                        pEnumColumn->cbData ) );
                }


                continue;
            }
        }


        cAlloc = rgEnumColumnId[ iEnumColumn ].ctagSequence;
        if ( !cAlloc )
        {
            cAlloc = cColumnValue;
        }
        cbAlloc = cAlloc * sizeof( JET_ENUMCOLUMNVALUE );

        Alloc( pEnumColumn->rgEnumColumnValue = (JET_ENUMCOLUMNVALUE*)pfnRealloc(
                pvReallocContext,
                NULL,
                cbAlloc ) );

        pEnumColumn->cEnumColumnValue = (ULONG)cAlloc;
        memset( pEnumColumn->rgEnumColumnValue, 0, cbAlloc );

        rgEnumLidStore.SetCapacityGrowth( max( iMinGrowthLidStore, cAlloc ) );


        for (   iEnumColumnValue = 0;
                iEnumColumnValue < pEnumColumn->cEnumColumnValue;
                iEnumColumnValue++ )
        {
            JET_ENUMCOLUMNVALUE* const pEnumColumnValue = &pEnumColumn->rgEnumColumnValue[ iEnumColumnValue ];


            if ( rgEnumColumnId[ iEnumColumn ].ctagSequence )
            {
                pEnumColumnValue->itagSequence = rgEnumColumnId[ iEnumColumn ].rgtagSequence[ iEnumColumnValue ];
            }
            else
            {
                pEnumColumnValue->itagSequence = (ULONG)( iEnumColumnValue + 1 );
            }

            if ( !pEnumColumnValue->itagSequence )
            {
                pEnumColumnValue->err = ErrERRCheck( JET_wrnColumnSkipped );
                continue;
            }


            if (    pEnumColumnValue->itagSequence > cColumnValue &&
                    !fColumnFound &&
                    ( grbit & JET_bitEnumerateIgnoreDefault ) )
            {
                pEnumColumnValue->err = ErrERRCheck( JET_wrnColumnDefault );
                continue;
            }


            if ( pEnumColumnValue->itagSequence > cColumnValue )
            {
                pEnumColumnValue->err = ErrERRCheck( JET_wrnColumnNull );
                continue;
            }


            if ( grbit & JET_bitEnumeratePresenceOnly )
            {
                pEnumColumnValue->err = ErrERRCheck( JET_wrnColumnPresent );
                continue;
            }


            Call( pciter->ErrGetColumnValue( pEnumColumnValue->itagSequence, &cbData, &pvData, &fSeparated, &fCompressed ) );


            if ( fSeparated && fInRecordOnly )
            {
                pEnumColumnValue->err = ErrERRCheck( JET_wrnColumnNotInRecord );
                continue;
            }


            if ( fSeparated )
            {
                INT iLid = rgEnumLidStore.Size();
                if ( rgEnumLidStore.ErrSetEntry( iLid, LidOfSeparatedLV( (BYTE*) pvData, cbData ) ) != CArray<LvId>::ERR::errSuccess )
                {
                    Error( ErrERRCheck( JET_errOutOfMemory ) );
                }

                pEnumColumnValue->err = fEncrypted ? ErrERRCheck( wrnRECSeparatedEncryptedLV ) : ErrERRCheck( wrnRECSeparatedLV );
                pEnumColumnValue->cbData = iLid;

                fSeparatedLV = fTrue;
                continue;
            }


            if ( fEncrypted &&
                 cbData > 0 )
            {
                Alloc( pbDataDecrypted = new BYTE[ cbData ] );
                ULONG cbDataDecryptedActual = cbData;
                Call( ErrOSUDecrypt(
                            (BYTE*)pvData,
                            pbDataDecrypted,
                            (ULONG *)&cbDataDecryptedActual,
                            pfucb->pbEncryptionKey,
                            pfucb->cbEncryptionKey,
                            PinstFromPfucb( pfucb )->m_iInstance,
                            pfucb->u.pfcb->TCE() ) );
                pvData = pbDataDecrypted;
                cbData = cbDataDecryptedActual;
            }

            if( fCompressed )
            {
                DATA data;
                data.SetPv( pvData );
                data.SetCb( cbData );
                INT cbDataActual;
                Call( ErrPKDecompressData( data, pfucb, NULL, 0, &cbDataActual ) );
                Alloc( pEnumColumnValue->pvData = pfnRealloc( pvReallocContext, NULL, cbDataActual ) );
                pEnumColumnValue->cbData = (ULONG)cbDataActual;
                Call( ErrPKDecompressData( data, pfucb, (BYTE *) pEnumColumnValue->pvData, pEnumColumnValue->cbData, &cbDataActual ) );
                Assert( JET_wrnBufferTruncated != err );
            }
            else
            {
                Alloc( pEnumColumnValue->pvData = pfnRealloc( pvReallocContext, NULL, cbData ) );
                pEnumColumnValue->cbData = (ULONG)cbData;

                if ( !FHostIsLittleEndian() && FCOLUMNIDFixed( pEnumColumn->columnid ) )
                {
                    switch ( fieldFC.cbMaxLen )
                    {
                        case 1:
                            *((BYTE*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< BYTE >*)pvData);
                            break;

                        case 2:
                            *((USHORT*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< USHORT >*)pvData);
                            break;

                        case 4:
                            *((ULONG*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< ULONG >*)pvData);
                            break;

                        case 8:
                            *((QWORD*)pEnumColumnValue->pvData) = *((UnalignedLittleEndian< QWORD >*)pvData);
                            break;

                        default:
                            Assert( fFalse );
                            Error( ErrERRCheck( JET_errInternalError ) );
                            break;
                    }
                }
                else
                {
                    memcpy( pEnumColumnValue->pvData,
                            pvData,
                            pEnumColumnValue->cbData );
                }
            }

            if ( pbDataDecrypted )
            {
                delete[] pbDataDecrypted;
                pbDataDecrypted = NULL;
            }


            if (    FCOLUMNIDFixed( pEnumColumn->columnid ) &&
                    FFIELDEscrowUpdate( fieldFC.ffield ) &&
                    !FFUCBInsertPrepared( pfucb ) )
            {
                Call( ErrRECAdjustEscrowedColumn(   pfucb,
                                                    pEnumColumn->columnid,
                                                    fieldFC,
                                                    pEnumColumnValue->pvData,
                                                    pEnumColumnValue->cbData ) );
            }
        }
    }


    if ( fSeparatedLV )
    {
        const BOOL fAfterImage = fUseCopyBuffer
                                || !FFUCBUpdateSeparateLV( pfucb )
                                || !FFUCBReplacePrepared( pfucb );

        Call( ErrRECIFetchMissingLVs(   pfucb,
                                        pcEnumColumn,
                                        prgEnumColumn,
                                        rgEnumLidStore,
                                        pfnRealloc,
                                        pvReallocContext,
                                        cbDataMost,
                                        fAfterImage,
                                        fRetrieveAsRefIfNotInRecord ) );
    }


    if ( fColumnIdError )
    {
        fRetColumnIdError = fTrue;
        for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
        {
            JET_ENUMCOLUMN* const pEnumColumn = &rgEnumColumn[ iEnumColumn ];
            Call( ErrERRCheck( pEnumColumn->err ) );
        }
        fRetColumnIdError = fFalse;
        err = JET_errSuccess;
    }


HandleError:
    if ( Pcsr( pfucb )->FLatched() )
    {
        CallS( ErrDIRRelease( pfucb ) );
    }
    AssertDIRNoLatch( pfucb->ppib );

    if ( pbDataDecrypted )
    {
        delete[] pbDataDecrypted;
        pbDataDecrypted = NULL;
    }

    if ( err < JET_errSuccess )
    {
        if ( !fRetColumnIdError )
        {
            if ( prgEnumColumn )
            {
                for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
                {
                    if ( rgEnumColumn[ iEnumColumn ].err != JET_wrnColumnSingleValue )
                    {
                        for (   iEnumColumnValue = 0;
                                iEnumColumnValue < rgEnumColumn[ iEnumColumn ].cEnumColumnValue;
                                iEnumColumnValue++ )
                        {
                            if ( rgEnumColumn[ iEnumColumn ].rgEnumColumnValue[ iEnumColumnValue ].pvData )
                            {
                                pfnRealloc( pvReallocContext,
                                            rgEnumColumn[ iEnumColumn ].rgEnumColumnValue[ iEnumColumnValue ].pvData,
                                            0 );
                            }
                        }
                        if ( rgEnumColumn[ iEnumColumn ].rgEnumColumnValue )
                        {
                            pfnRealloc( pvReallocContext,
                                        rgEnumColumn[ iEnumColumn ].rgEnumColumnValue,
                                        0 );
                        }
                    }
                    else
                    {
                        if ( rgEnumColumn[ iEnumColumn ].pvData )
                        {
                            pfnRealloc( pvReallocContext,
                                        rgEnumColumn[ iEnumColumn ].pvData,
                                        0 );
                        }
                    }
                }
                if ( rgEnumColumn )
                {
                    pfnRealloc( pvReallocContext, rgEnumColumn, 0 );
                    rgEnumColumn = NULL;
                }
            }
            if ( pcEnumColumn )
            {
                cEnumColumn = 0;
            }
        }
    }
    return err;
}

ERR VTAPI ErrIsamEnumerateColumns(
    JET_SESID               vsesid,
    JET_VTID                vtid,
    ULONG           cEnumColumnId,
    JET_ENUMCOLUMNID*       rgEnumColumnId,
    ULONG*          pcEnumColumn,
    JET_ENUMCOLUMN**        prgEnumColumn,
    JET_PFNREALLOC          pfnRealloc,
    void*                   pvReallocContext,
    ULONG           cbDataMost,
    JET_GRBIT               grbit )
{
    ERR     err;
    PIB*    ppib                = (PIB *)vsesid;
    FUCB*   pfucb               = (FUCB *)vtid;
    BOOL    fTransactionStarted = fFalse;

    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRNoLatch( ppib );

    if ( 0 == ppib->Level() )
    {
        Call( ErrDIRBeginTransaction( ppib, 51493, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }

    AssertDIRNoLatch( ppib );
    Assert( FFUCBSort( pfucb ) || FFUCBIndex( pfucb ) );


    if ( !cEnumColumnId )
    {
        Call( ErrRECEnumerateAllColumns(    pfucb,
                                            pcEnumColumn,
                                            prgEnumColumn,
                                            pfnRealloc,
                                            pvReallocContext,
                                            cbDataMost,
                                            grbit ) );
    }


    else
    {
        Call( ErrRECEnumerateReqColumns(    pfucb,
                                            cEnumColumnId,
                                            rgEnumColumnId,
                                            pcEnumColumn,
                                            prgEnumColumn,
                                            pfnRealloc,
                                            pvReallocContext,
                                            cbDataMost,
                                            grbit ) );
    }

HandleError:
    if ( fTransactionStarted )
    {
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }
    AssertDIRNoLatch( ppib );

    return err;
}


LOCAL ERR ErrRECIGetRecordSize(
    FUCB *                  pfucb,
    const DATA&             dataRec,
    JET_RECSIZE3 *          precsize,
    const JET_GRBIT         grbit )
{
    ERR                     err;
    FCB *                   pfcb                = pfucb->u.pfcb;
    const BOOL              fRunningTotal       = ( grbit & JET_bitRecordSizeRunningTotal );
    const REC *             prec                = (REC *)dataRec.Pv();
    CTaggedColumnIter       citerTC;

    Assert( NULL != precsize );

    if ( !fRunningTotal )
    {
        memset( precsize, 0, sizeof(*precsize) );
    }

    precsize->cbOverhead += CPAGE::cbInsertionOverhead + cbKeyCount + REC::cbRecordHeader;

    if ( Pcsr( pfucb )->FLatched() || FFUCBSort( pfucb ) )
    {
        Assert( locOnCurBM == pfucb->locLogical );
        precsize->cbOverhead += pfucb->kdfCurr.key.Cb();
        precsize->cbKey += pfucb->kdfCurr.key.Cb();
    }
    else if ( pfucb->cbstat == fCBSTATNull || FFUCBReplacePrepared( pfucb ) )
    {
        Assert( locOnCurBM == pfucb->locLogical );
        precsize->cbOverhead += pfucb->bmCurr.key.Cb();
        precsize->cbKey += pfucb->bmCurr.key.Cb();
    }
    else
    {
    }

    precsize->cbOverhead += prec->CbFixedRecordOverhead() + prec->CbVarRecordOverhead();
    precsize->cbData += prec->CbFixedUserData() + prec->CbVarUserData();
    precsize->cbDataCompressed += prec->CbFixedUserData() + prec->CbVarUserData();
    precsize->cNonTaggedColumns += prec->CFixedColumns() + prec->CVarColumns();

    Call( citerTC.ErrInit( pfcb ) );
    Call( citerTC.ErrSetRecord( dataRec ) );

    Call( citerTC.ErrMoveBeforeFirst() );
    while ( errRECNoCurrentColumnValue != ( err = citerTC.ErrMoveNext() ) )
    {
        Call( err );

        Call( citerTC.ErrGetColumnSize( pfucb, precsize, grbit ) );
    }

    Assert( errRECNoCurrentColumnValue == err );
    err = JET_errSuccess;

HandleError:
    return err;
}

ERR VTAPI ErrIsamGetRecordSize(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_RECSIZE3 *  precsize,
    const JET_GRBIT grbit )
{
    ERR             err;
    PIB *           ppib                = (PIB *)vsesid;
    FUCB *          pfucb               = (FUCB *)vtid;
    BOOL            fTransactionStarted = fFalse;
    BOOL            fUseCopyBuffer      = fFalse;
    DATA *          pdataRec            = NULL;
    BOOL            fAllocated          = fFalse;
    DATA            dataAllocated;

    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRNoLatch( ppib );
    Assert( FFUCBSort( pfucb ) || FFUCBIndex( pfucb ) );

    if ( NULL == precsize )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( 0 == ppib->Level() )
    {
        Call( ErrDIRBeginTransaction( ppib, 45349, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }

    fUseCopyBuffer = ( ( ( grbit & JET_bitRecordSizeInCopyBuffer ) && FFUCBUpdatePrepared( pfucb ) && !FFUCBNeverRetrieveCopy( pfucb ) )
                    || FFUCBAlwaysRetrieveCopy( pfucb ) );

    Call( ErrRECIGetRecord( pfucb, &pdataRec, fUseCopyBuffer ) );

    if ( !fUseCopyBuffer && !( grbit & JET_bitRecordSizeLocal ) )
    {
        VOID *pvAlloc;
        Alloc( pvAlloc = PvOSMemoryHeapAlloc( pdataRec->Cb() ) );
        fAllocated = fTrue;
        UtilMemCpy( pvAlloc, pdataRec->Pv(), pdataRec->Cb() );

        dataAllocated.SetPv( pvAlloc );
        dataAllocated.SetCb( pdataRec->Cb() );
        pdataRec = &dataAllocated;

        Call( ErrDIRRelease( pfucb ) );
    }

    Call( ErrRECIGetRecordSize( pfucb, *pdataRec, precsize, grbit ) );

HandleError:
    if ( Pcsr( pfucb )->FLatched() )
    {
        Assert( !fUseCopyBuffer );

        const ERR   errT    = ErrDIRRelease( pfucb );
        CallSx( errT, JET_errOutOfMemory );
        if ( errT < JET_errSuccess && err >= JET_errSuccess )
        {
            err = errT;
        }
    }

    if ( fTransactionStarted )
    {
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }
    AssertDIRNoLatch( ppib );

    if ( fAllocated )
    {
        Assert( !fUseCopyBuffer );
        Assert( !( grbit & JET_bitRecordSizeLocal ) );
        Assert( pdataRec == &dataAllocated );
        OSMemoryHeapFree( dataAllocated.Pv() );
    }

    return err;
}


#include <pshpack1.h>

typedef BYTE RecordBufferFormatIdentifier;

PERSISTED
enum RecordBufferFormatIdentifiers : RecordBufferFormatIdentifier
{
    rbfiInvalid = 0x00,
    rbfiV1      = 0x01,
};

PERSISTED
struct RECORD_BUFFER_HEADER_COMMON
{
    Unaligned<RecordBufferFormatIdentifier> rbfi;
};

PERSISTED
struct RECORD_BUFFER_HEADER_V1 : public RECORD_BUFFER_HEADER_COMMON
{
    Unaligned<ULONG>                        cbHeader;


    Unaligned<ULONG>                        ibRead;
    Unaligned<ULONG>                        iRecord;
    Unaligned<JET_COLUMNID>                 columnid;
    Unaligned<ULONG>                        itagSequence;
};

PERSISTED
struct RECORD_BUFFER_COLUMN_VALUE
{
    BYTE                                    iRecordMod2 : 1;
    BYTE                                    fNotAvailable : 1;
    BYTE                                    fReference : 1;
    BYTE                                    fNull : 1;
    BYTE                                    rgbitReservedMustBeZero : 4;
    Unaligned<JET_COLUMNID>                 columnid;
    Unaligned<ULONG>                        cbValue;
    BYTE                                    rgbValue[0];
};

#include <poppack.h>

struct STREAM_RECORDS_COMMON_FILTER_CONTEXT : public MOVE_FILTER_CONTEXT
{
    void *                  pvData;
    ULONG                   cbData;
    ULONG *                 pcbActual;
    JET_GRBIT               grbit;

    ULONG                   ccolumnid;
    const JET_COLUMNID *    rgcolumnid;

    ULONG                   cRecords;
    ULONG                   cbDataGenerated;
};

LOCAL ERR ErrRECIStreamRecordsICommonFilter(
    FUCB * const                                    pfucb,
    STREAM_RECORDS_COMMON_FILTER_CONTEXT * const    pcontext )
{
    ERR err = JET_errSuccess;


    Call( ErrRECCheckMoveFilter( pfucb, (MOVE_FILTER_CONTEXT* const)pcontext ) );
    if ( err > JET_errSuccess )
    {
        goto HandleError;
    }


    if ( FFUCBLimstat( pfucb ) )
    {
        if (    ( ( pcontext->grbit & JET_bitStreamForward ) && FFUCBUpper( pfucb ) ) ||
                ( ( pcontext->grbit & JET_bitStreamBackward ) && !FFUCBUpper( pfucb ) ) )
        {
            Call( ErrFUCBCheckIndexRange( pfucb, pfucb->kdfCurr.key ) );
        }
    }

HandleError:
    return err;
}

LOCAL ERR ErrRECIStreamRecordsIAppendColumnValue(
    STREAM_RECORDS_COMMON_FILTER_CONTEXT * const    pcontext,
    const JET_COLUMNID                              columnid,
    const ERR                                       errData,
    const size_t                                    cbData,
    const VOID * const                              pvData )
{
    ERR err = JET_errSuccess;


    if ( pcontext->cbDataGenerated + sizeof( RECORD_BUFFER_COLUMN_VALUE ) + cbData > pcontext->cbData )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }


    RECORD_BUFFER_COLUMN_VALUE* pcolumnValue = (RECORD_BUFFER_COLUMN_VALUE*)( (BYTE*)pcontext->pvData + pcontext->cbDataGenerated );
    pcontext->cbDataGenerated += sizeof( RECORD_BUFFER_COLUMN_VALUE ) + cbData;

    pcolumnValue->iRecordMod2               = pcontext->cRecords & 1;
    pcolumnValue->fNotAvailable             = errData == JET_wrnColumnNotInRecord;
    pcolumnValue->fReference                = errData == JET_wrnColumnReference;
    pcolumnValue->fNull                     = errData == JET_wrnColumnNull;
    pcolumnValue->rgbitReservedMustBeZero   = 0;
    pcolumnValue->columnid                  = columnid;
    pcolumnValue->cbValue                   = cbData;

    switch ( cbData )
    {
        case 1:
            *( (BYTE*)pcolumnValue->rgbValue ) = *( (BYTE*)pvData );
            break;

        case 2:
            *( ( UnalignedLittleEndian<WORD>* )pcolumnValue->rgbValue ) = *( ( UnalignedLittleEndian<WORD>* )pvData );
            break;

        case 4:
            *( ( UnalignedLittleEndian<DWORD>* )pcolumnValue->rgbValue ) = *( ( UnalignedLittleEndian<DWORD>* )pvData );
            break;

        case 8:
            *( ( UnalignedLittleEndian<QWORD>* )pcolumnValue->rgbValue ) = *( ( UnalignedLittleEndian<QWORD>* )pvData );
            break;

        default:
            memcpy( pcolumnValue->rgbValue, pvData, cbData );
            break;
    }

HandleError:
    return err;
}

struct COLUMN_METADATA
{
    JET_COLUMNID            columnid;
    ULONG                   fEscrow : 1;
    ULONG                   fVisible : 1;
    ULONG                   fEncrypted : 1;
    ULONG                   rgbitReserved : 29;
};

struct STREAM_RECORDS_PRIMARY_FILTER_CONTEXT : public STREAM_RECORDS_COMMON_FILTER_CONTEXT
{
    const BOOL *            rgfEncrypted;
    const BOOL *            rgfEscrow;

    IColumnIter *           pciterRoot;
    IColumnIter *           pciterRec;

    TDB *                   ptdb;
    TDB *                   ptdbTemplate;
    ULONG                   ccolumnmetadata;
    COLUMN_METADATA *       rgcolumnmetadata;
};

LOCAL NOINLINE ERR ErrRECIStreamRecordsOnPrimaryIndexIGetColumnMetaDataSlowly(
    COLUMN_METADATA * const                         pcolumnmetadata,
    FUCB * const                                    pfucb,
    const JET_COLUMNID                              columnid,
    BOOL * const                                    pfVisible,
    BOOL * const                                    pfEncrypted,
    BOOL * const                                    pfEscrow
    )
{
    ERR err = JET_errSuccess;


    FIELD fieldFC;
    err = ErrRECIAccessColumn( pfucb, columnid, &fieldFC, pfEncrypted );
    if ( err == JET_errColumnNotFound )
    {
        *pfVisible = fFalse;
        err = JET_errSuccess;
    }
    Call( err );

    if ( *pfEncrypted && !pfucb->pbEncryptionKey )
    {
        Error( ErrERRCheck( JET_errColumnNoEncryptionKey ) );
    }

    *pfVisible = fTrue;
    *pfEscrow = FCOLUMNIDFixed( columnid ) && FFIELDEscrowUpdate( fieldFC.ffield );


    pcolumnmetadata->columnid                   = columnid;
    pcolumnmetadata->fVisible                   = *pfVisible;
    pcolumnmetadata->fEncrypted                 = *pfEncrypted;
    pcolumnmetadata->fEscrow                    = *pfEscrow;

HandleError:
    return err;
}

LOCAL ERR ErrRECIStreamRecordsOnPrimaryIndexIGetColumnMetaData(
    STREAM_RECORDS_PRIMARY_FILTER_CONTEXT * const   pcontext,
    FUCB * const                                    pfucb,
    const JET_COLUMNID                              columnid,
    BOOL * const                                    pfVisible,
    BOOL * const                                    pfEncrypted,
    BOOL * const                                    pfEscrow
    )
{

    TDB * const ptdb = FCOLUMNIDTemplateColumn( columnid ) ? pcontext->ptdbTemplate : pcontext->ptdb;

    FID fid = FidOfColumnid( columnid );
    ULONG icolumnmetadata = fid;
    icolumnmetadata -= fid >= fidTaggedLeast ? fidVarMost - ptdb->FidVarLast() : 0;
    icolumnmetadata -= fid >= fidVarLeast ? fidFixedMost - ptdb->FidFixedLast() : 0;
    icolumnmetadata -= fid >= fidFixedLeast ? fidFixedLeast : 0;
    icolumnmetadata += FCOLUMNIDTemplateColumn( columnid ) ? pcontext->ptdb->CColumns() : 0;
    icolumnmetadata = icolumnmetadata & ( pcontext->ccolumnmetadata - 1 );

    COLUMN_METADATA * pcolumnmetadata = &pcontext->rgcolumnmetadata[icolumnmetadata];

    if ( pcolumnmetadata->columnid == columnid )
    {
        *pfVisible                  = pcolumnmetadata->fVisible;
        *pfEncrypted                = pcolumnmetadata->fEncrypted;
        *pfEscrow                   = pcolumnmetadata->fEscrow;
        return JET_errSuccess;
    }


    return ErrRECIStreamRecordsOnPrimaryIndexIGetColumnMetaDataSlowly(  pcolumnmetadata,
                                                                        pfucb,
                                                                        columnid,
                                                                        pfVisible,
                                                                        pfEncrypted,
                                                                        pfEscrow );
}

LOCAL ERR ErrRECIStreamRecordsOnPrimaryIndexIAppendColumnValues(
    FUCB * const                                    pfucb,
    STREAM_RECORDS_PRIMARY_FILTER_CONTEXT * const   pcontext,
    const JET_COLUMNID                              columnid,
    const BOOL                                      fEncrypted,
    const BOOL                                      fEscrow )
{
    ERR     err                 = JET_errSuccess;
    size_t  cColumnValue        = 0;
    BOOL    fAllocatedMemory    = fFalse;
    BYTE*   pbDataReference     = NULL;
    BYTE*   pbDataDecrypted     = NULL;
    BYTE*   pbDataDecompressed  = NULL;
    BYTE*   pbDataAdjusted      = NULL;


    Call( pcontext->pciterRoot->ErrGetColumnValueCount( &cColumnValue ) );

    for ( ULONG itagSequence = 1; itagSequence <= cColumnValue; itagSequence++ )
    {

        ERR errData = JET_errSuccess;
        size_t cbData;
        VOID* pvData;
        BOOL fSeparated;
        BOOL fCompressed;
        Call( pcontext->pciterRoot->ErrGetColumnValue( itagSequence, &cbData, &pvData, &fSeparated, &fCompressed ) );


        if ( fSeparated )
        {

            if ( pcontext->grbit & JET_bitStreamColumnReferences )
            {
                ULONG cbDataReference;
                Call( ErrRECCreateColumnReference(  pfucb,
                                                    columnid,
                                                    itagSequence,
                                                    LidOfSeparatedLV( (BYTE*)pvData, cbData ),
                                                    &cbDataReference,
                                                    &pbDataReference ) );
                fAllocatedMemory = fTrue;
                errData = JET_wrnColumnReference;
                pvData = pbDataReference;
                cbData = cbDataReference;
            }


            else
            {
                errData = JET_wrnColumnNotInRecord;
                pvData = NULL;
                cbData = 0;
            }
        }


        else
        {

            if ( fEncrypted && cbData )
            {
                Alloc( pbDataDecrypted = new BYTE[cbData] );
                fAllocatedMemory = fTrue;
                ULONG cbDataDecryptedActual = cbData;
                Call( ErrOSUDecrypt(    (BYTE*)pvData,
                                        pbDataDecrypted,
                                        (ULONG *)&cbDataDecryptedActual,
                                        pfucb->pbEncryptionKey,
                                        pfucb->cbEncryptionKey,
                                        PinstFromPfucb( pfucb )->m_iInstance,
                                        pfucb->u.pfcb->TCE() ) );
                pvData = pbDataDecrypted;
                cbData = cbDataDecryptedActual;
            }


            if ( fCompressed )
            {
                DATA data;
                data.SetPv( pvData );
                data.SetCb( cbData );
                INT cbDataDecompressed;
                Call( ErrPKAllocAndDecompressData( data, pfucb, &pbDataDecompressed, &cbDataDecompressed ) );
                fAllocatedMemory = fTrue;
                Assert( JET_wrnBufferTruncated != err );
                pvData = pbDataDecompressed;
                cbData = cbDataDecompressed;
            }


            if ( fEscrow )
            {
                Alloc( pbDataAdjusted = new BYTE[ cbData ] );
                fAllocatedMemory = fTrue;
                memcpy( pbDataAdjusted, pvData, cbData );
                const FIELD* const pfield = pcontext->pciterRoot->PField();
                Call( ErrRECAdjustEscrowedColumn(   pfucb,
                                                    columnid,
                                                    *pfield,
                                                    pbDataAdjusted,
                                                    cbData ) );
                pvData = pbDataAdjusted;
            }
        }


        Call( ErrRECIStreamRecordsIAppendColumnValue( pcontext, columnid, errData, cbData, pvData ) );
    }


    if ( cColumnValue == 0 )
    {
        Call( ErrRECIStreamRecordsIAppendColumnValue( pcontext, columnid, JET_wrnColumnNull, 0, NULL ) );
    }

HandleError:
    if ( fAllocatedMemory )
    {
        delete[] pbDataReference;
        delete[] pbDataDecrypted;
        delete[] pbDataDecompressed;
        delete[] pbDataAdjusted;
    }
    return err;
}

LOCAL ERR ErrRECIStreamRecordsOnPrimaryIndexIFilter(
    FUCB * const                                    pfucb,
    STREAM_RECORDS_PRIMARY_FILTER_CONTEXT * const   pcontext )
{
    ERR err = JET_errSuccess;


    Call( ErrRECIStreamRecordsICommonFilter( pfucb, (STREAM_RECORDS_COMMON_FILTER_CONTEXT* const)pcontext ) );
    if ( err > JET_errSuccess )
    {
        goto HandleError;
    }


    Call( pcontext->pciterRec->ErrSetRecord( pfucb->kdfCurr.data ) );


    if ( !pcontext->rgcolumnid )
    {

        Call( pcontext->pciterRoot->ErrMoveBeforeFirst() );
        while ( ( err = pcontext->pciterRoot->ErrMoveNext() ) >= JET_errSuccess )
        {

            JET_COLUMNID columnid;
            Call( pcontext->pciterRoot->ErrGetColumnId( &columnid ) );


            BOOL fVisible;
            BOOL fEncrypted;
            BOOL fEscrow;
            Call( ErrRECIStreamRecordsOnPrimaryIndexIGetColumnMetaData( pcontext,
                                                                        pfucb,
                                                                        columnid,
                                                                        &fVisible,
                                                                        &fEncrypted,
                                                                        &fEscrow ) );


            if ( !fVisible )
            {
                continue;
            }


            Call( ErrRECIStreamRecordsOnPrimaryIndexIAppendColumnValues(    pfucb,
                                                                            pcontext,
                                                                            columnid,
                                                                            fEncrypted,
                                                                            fEscrow ) );
        }
        if ( err == errRECNoCurrentColumnValue )
        {
            err = JET_errSuccess;
        }
        Call( err );
    }
    else
    {
        for ( ULONG icolumnid = 0; icolumnid < pcontext->ccolumnid; icolumnid++ )
        {

            JET_COLUMNID columnid = pcontext->rgcolumnid[icolumnid];


            if ( ( err = pcontext->pciterRoot->ErrSeek( columnid ) ) >= JET_errSuccess )
            {

                Call( ErrRECIStreamRecordsOnPrimaryIndexIAppendColumnValues(    pfucb,
                                                                                pcontext,
                                                                                columnid,
                                                                                pcontext->rgfEncrypted[icolumnid],
                                                                                pcontext->rgfEscrow[icolumnid] ) );
            }


            else if ( err == errRECColumnNotFound )
            {
                Call( ErrRECIStreamRecordsIAppendColumnValue( pcontext, columnid, JET_wrnColumnNull, 0, NULL ) );
            }
            Call( err );
        }
    }


    pcontext->cRecords++;
    *pcontext->pcbActual = pcontext->cbDataGenerated;


    err = wrnBTNotVisibleAccumulated;

HandleError:
    if ( err == JET_errBufferTooSmall )
    {
        err = JET_errSuccess;
    }
    return err;
}

LOCAL ERR ErrIsamStreamRecordsOnPrimaryIndex(
    _In_ PIB * const                                            ppib,
    _In_ FUCB * const                                           pfucb,
    _In_ const ULONG                                    ccolumnid,
    _In_reads_opt_( ccolumnid ) const JET_COLUMNID * const      rgcolumnid,
    _Out_writes_bytes_to_opt_( cbData, cbActual ) void * const  pvData,
    _In_ const ULONG                                    cbData,
    _Inout_ ULONG &                                     cbActual,
    _In_ const JET_GRBIT                                        grbit )
{
    ERR                                     err                         = JET_errSuccess;

    TDB* const                              ptdb                        = pfucb->u.pfcb->Ptdb();
    TDB* const                              ptdbTemplate                = ( ptdb->PfcbTemplateTable() != pfcbNil ?
                                                                                ptdb->PfcbTemplateTable()->Ptdb() :
                                                                                ptdbNil );

    BOOL                                    fDefaultRecord              = fFalse;
    BOOL                                    fNonEscrowDefault           = fFalse;
    BOOL                                    fUseColumnReferences        = fFalse;
    BOOL                                    fHasFixed                   = fFalse;
    BOOL                                    fHasVariable                = fFalse;
    BOOL                                    fHasTagged                  = fFalse;

    CFixedColumnIter                        rgciterFC[ 2 ];
    size_t                                  iciterFC                    = 0;
    CFixedColumnIter*                       pciterFC                    = NULL;
    CVariableColumnIter                     rgciterVC[ 2 ];
    size_t                                  iciterVC                    = 0;
    CVariableColumnIter*                    pciterVC                    = NULL;
    CTaggedColumnIter                       rgciterTC[ 2 ];
    size_t                                  iciterTC                    = 0;
    CTaggedColumnIter*                      pciterTC                    = NULL;
    CUnionIter                              rgciterU[ 5 ];
    size_t                                  iciterU                     = 0;
    CUnionIter*                             pciterU                     = NULL;

    IColumnIter*                            pciterRec                   = NULL;
    IColumnIter*                            pciterDefaultRec            = NULL;
    IColumnIter*                            pciterRoot                  = NULL;

    RECORD_BUFFER_HEADER_V1                 headerV1;
    STREAM_RECORDS_PRIMARY_FILTER_CONTEXT   context;
    ULONG                                   ccolumnidContext            = 0;
    JET_COLUMNID*                           rgcolumnidContext           = NULL;
    const ULONG                             ccolumnidPreprocessMax      = 100;
    BOOL*                                   rgfEncrypted                = NULL;
    BOOL*                                   rgfEscrow                   = NULL;
    ULONG                                   ccolumnmetadata             = 0;
    COLUMN_METADATA *                       rgcolumnmetadata            = NULL;


    Assert( pfucb->pfucbCurIndex == pfucbNil );


    if ( ptdb->FTableHasUserDefinedDefault() )
    {
        Error( ErrERRCheck( JET_errCallbackFailed ) );
    }


    fDefaultRecord          = ptdb->FTableHasDefault();
    fNonEscrowDefault       = ptdb->FTableHasNonEscrowDefault();
    fUseColumnReferences    = grbit & JET_bitStreamColumnReferences;

    fHasFixed               = ptdb->CFixedColumns() > 0 || ptdbTemplate != ptdbNil && ptdbTemplate->CFixedColumns() > 0;
    fHasVariable            = ptdb->CVarColumns() > 0 || ptdbTemplate != ptdbNil && ptdbTemplate->CVarColumns() > 0;
    fHasTagged              = ptdb->CTaggedColumns() > 0 || ptdbTemplate != ptdbNil && ptdbTemplate->CTaggedColumns() > 0;

    if ( fHasVariable )
    {
        pciterVC = &rgciterVC[iciterVC++];
        Call( pciterVC->ErrInit( pfucb->u.pfcb ) );
        if ( !pciterRec )
        {
            pciterRec = pciterVC;
        }
        else
        {
            pciterU = &rgciterU[iciterU++];
            Call( pciterU->ErrInit( pciterVC, pciterRec ) );
            pciterRec = pciterU;
        }
    }

    if ( fHasFixed )
    {
        pciterFC = &rgciterFC[iciterFC++];
        Call( pciterFC->ErrInit( pfucb->u.pfcb ) );
        if ( !pciterRec )
        {
            pciterRec = pciterFC;
        }
        else
        {
            pciterU = &rgciterU[iciterU++];
            Call( pciterU->ErrInit( pciterFC, pciterRec ) );
            pciterRec = pciterU;
        }
    }

    if ( fHasTagged )
    {
        pciterTC = &rgciterTC[iciterTC++];
        Call( pciterTC->ErrInit( pfucb->u.pfcb ) );
        if ( !pciterRec )
        {
            pciterRec = pciterTC;
        }
        else
        {
            pciterU = &rgciterU[iciterU++];
            Call( pciterU->ErrInit( pciterTC, pciterRec ) );
            pciterRec = pciterU;
        }
    }

    if ( !pciterRoot )
    {
        pciterRoot = pciterRec;
    }
    else
    {
        pciterU = &rgciterU[iciterU++];
        Call( pciterU->ErrInit( pciterRoot, pciterRec ) );
        pciterRoot = pciterU;
    }

    if ( fDefaultRecord )
    {
        if ( fHasVariable && fNonEscrowDefault )
        {
            pciterVC = &rgciterVC[iciterVC++];
            Call( pciterVC->ErrInit( pfucb->u.pfcb ) );
            if ( !pciterDefaultRec )
            {
                pciterDefaultRec = pciterVC;
            }
            else
            {
                pciterU = &rgciterU[iciterU++];
                Call( pciterU->ErrInit( pciterVC, pciterDefaultRec ) );
                pciterDefaultRec = pciterU;
            }
        }

        if ( fHasFixed )
        {
            pciterFC = &rgciterFC[iciterFC++];
            Call( pciterFC->ErrInit( pfucb->u.pfcb ) );
            if ( !pciterDefaultRec )
            {
                pciterDefaultRec = pciterFC;
            }
            else
            {
                pciterU = &rgciterU[iciterU++];
                Call( pciterU->ErrInit( pciterFC, pciterDefaultRec ) );
                pciterDefaultRec = pciterU;
            }
        }

        if ( fHasTagged && fNonEscrowDefault )
        {
            pciterTC = &rgciterTC[iciterTC++];
            Call( pciterTC->ErrInit( pfucb->u.pfcb ) );
            if ( !pciterDefaultRec )
            {
                pciterDefaultRec = pciterTC;
            }
            else
            {
                pciterU = &rgciterU[iciterU++];
                Call( pciterU->ErrInit( pciterTC, pciterDefaultRec ) );
                pciterDefaultRec = pciterU;
            }
        }

        if ( pciterDefaultRec )
        {
            if ( !pciterRoot )
            {
                pciterRoot = pciterDefaultRec;
            }
            else
            {
                pciterU = &rgciterU[iciterU++];
                Call( pciterU->ErrInit( pciterRoot, pciterDefaultRec ) );
                pciterRoot = pciterU;
            }
        }
    }


    if ( fUseColumnReferences && ( FFUCBSort( pfucb ) || FFMPIsTempDB( pfucb->ifmp ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }


    if ( fDefaultRecord && pciterDefaultRec )
    {
        DATA * const pdataDefaultRec = ptdb->PdataDefaultRecord();
        Call( pciterDefaultRec->ErrSetRecord( *pdataDefaultRec ) );
    }


    if ( rgcolumnid )
    {
        Alloc( rgfEncrypted = new BOOL[ccolumnid] );
        Alloc( rgfEscrow = new BOOL[ccolumnid] );
        for ( ULONG icolumnid = 0; icolumnid < ccolumnid; icolumnid++ )
        {
            FIELD fieldFC;
            Call( ErrRECIAccessColumn( pfucb, rgcolumnid[icolumnid], &fieldFC, &rgfEncrypted[icolumnid] ) );
            rgfEscrow[ icolumnid ] = FCOLUMNIDFixed( rgcolumnid[ icolumnid ] ) && FFIELDEscrowUpdate( fieldFC.ffield );
            if ( rgfEncrypted[icolumnid] && !pfucb->pbEncryptionKey )
            {
                Error( ErrERRCheck( JET_errColumnNoEncryptionKey ) );
            }
        }
        ccolumnidContext = ccolumnid;
        rgcolumnidContext = (JET_COLUMNID*)rgcolumnid;
    }


    if ( !rgcolumnid )
    {
        ULONG ccolumnidUsed = 0;
        for ( TDB* ptdbT = ptdb; ptdbT != ptdbNil; ptdbT = ptdbT == ptdbTemplate ? ptdbNil : ptdbTemplate )
        {
            ccolumnidUsed += ptdbT->FidFixedLast() - fidFixedLeast + 1;
            ccolumnidUsed += ptdbT->FidVarLast() - fidVarLeast + 1;
            ccolumnidUsed += ptdbT->FidTaggedLast() - fidTaggedLeast + 1;
        }

        if ( ccolumnidUsed < ccolumnidPreprocessMax )
        {
            Alloc( rgcolumnidContext = new JET_COLUMNID[ccolumnidUsed] );
            Alloc( rgfEncrypted = new BOOL[ccolumnidUsed] );
            Alloc( rgfEscrow = new BOOL[ccolumnidUsed] );

            for ( TDB* ptdbT = ptdb; ptdbT != ptdbNil; ptdbT = ptdbT == ptdbTemplate ? ptdbNil : ptdbTemplate )
            {
                for ( FID fid = ptdbT->FidFixedFirst(); fid <= ptdbT->FidTaggedLast(); fid++ )
                {
                    if ( fid == ptdbT->FidFixedLast() + 1 )
                    {
                        fid = ptdbT->FidVarFirst();
                    }
                    if ( fid == ptdbT->FidVarLast() + 1 )
                    {
                        fid = ptdbT->FidTaggedFirst();
                    }

                    JET_COLUMNID columnid = ColumnidOfFid( fid, ptdbT->FTemplateTable() );

                    FIELD fieldFC;
                    err = ErrRECIAccessColumn( pfucb, columnid, &fieldFC, &rgfEncrypted[ccolumnidContext] );
                    if ( err == JET_errColumnNotFound )
                    {
                        continue;
                    }
                    Call( err );
                    if ( rgfEncrypted[ccolumnidContext] && !pfucb->pbEncryptionKey )
                    {
                        Error( ErrERRCheck( JET_errColumnNoEncryptionKey ) );
                    }
                    rgcolumnidContext[ccolumnidContext] = columnid;
                    rgfEscrow[ ccolumnidContext ] = FCOLUMNIDFixed( columnid ) && FFIELDEscrowUpdate( fieldFC.ffield );
                    ccolumnidContext++;
                }
                Assert( ccolumnidContext <= ccolumnidUsed );
            }
        }
        else
        {
            for ( ccolumnmetadata = 1; ccolumnmetadata < ccolumnidUsed; ccolumnmetadata *= 2 );
            Alloc( rgcolumnmetadata = new COLUMN_METADATA[ccolumnmetadata] );
            memset( rgcolumnmetadata, 0, sizeof( rgcolumnmetadata[0] ) * ccolumnmetadata );
        }
    }


    headerV1.rbfi           = rbfiV1;
    headerV1.cbHeader       = sizeof( headerV1 );
    headerV1.ibRead         = sizeof( headerV1 );
    headerV1.iRecord        = ulMax;
    headerV1.columnid       = JET_columnidNil;
    headerV1.itagSequence   = 1;

    if ( cbData < sizeof( headerV1 ) )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    memcpy( pvData, &headerV1, sizeof( headerV1 ) );
    cbActual += sizeof( headerV1 );


    err = ErrIsamMove( ppib, pfucb, ( grbit & JET_bitStreamForward ) ? JET_MovePrevious : JET_MoveNext, NO_GRBIT );
    if ( err == JET_errNoCurrentRecord )
    {
        err = JET_errSuccess;
    }
    Call( err );


    context.pvData                      = pvData;
    context.cbData                      = cbData;
    context.pcbActual                   = &cbActual;
    context.grbit                       = grbit;

    context.ccolumnid                   = ccolumnidContext;
    context.rgcolumnid                  = rgcolumnidContext;

    context.cRecords                    = 0;
    context.cbDataGenerated             = cbActual;

    context.rgfEncrypted                = rgfEncrypted;
    context.rgfEscrow                   = rgfEscrow;

    context.pciterRoot                  = pciterRoot;
    context.pciterRec                   = pciterRec;

    context.ptdb                        = ptdb;
    context.ptdbTemplate                = ptdbTemplate;
    context.ccolumnmetadata             = ccolumnmetadata;
    context.rgcolumnmetadata            = rgcolumnmetadata;

    RECAddMoveFilter( pfucb, (PFN_MOVE_FILTER)ErrRECIStreamRecordsOnPrimaryIndexIFilter, (MOVE_FILTER_CONTEXT *)&context );


    err = ErrIsamMove( ppib, pfucb, ( grbit & JET_bitStreamForward ) ? JET_MoveNext : JET_MovePrevious, NO_GRBIT );
    if ( err == JET_errNoCurrentRecord )
    {
        err = ErrERRCheck( JET_wrnNoMoreRecords );
    }
    Call( err );

HandleError:
    RECRemoveMoveFilter( pfucb, (PFN_MOVE_FILTER)ErrRECIStreamRecordsOnPrimaryIndexIFilter );
    if ( rgcolumnidContext != rgcolumnid )
    {
        delete[] rgcolumnidContext;
    }
    delete[] rgfEncrypted;
    delete[] rgfEscrow;
    delete[] rgcolumnmetadata;
    return err;
}

struct STREAM_RECORDS_SECONDARY_FILTER_CONTEXT : public STREAM_RECORDS_COMMON_FILTER_CONTEXT
{
    const ULONG *           mpicolumnidiidxsegSecondary;
    ULONG                   iidxsegSecondaryMax;

    FCB *                   pfcbTableSecondary;
    TDB *                   ptdbSecondary;
    IDB *                   pidbSecondary;
    ULONG                   cidxsegSecondary;
    const IDXSEG *          rgidxsegSecondary;
    const FIELD *           rgfieldSecondary;
    ULONG                   cbKeyMostSecondary;
    ULONG                   cbKeyTruncatedSecondary;
    BYTE*                   rgbDenormSecondary;
    BYTE*                   rgbKeySecondary;

    const ULONG *           mpicolumnidiidxsegPrimary;
    ULONG                   iidxsegPrimaryMax;

    FCB *                   pfcbTablePrimary;
    TDB *                   ptdbPrimary;
    IDB *                   pidbPrimary;
    ULONG                   cidxsegPrimary;
    const IDXSEG *          rgidxsegPrimary;
    const FIELD *           rgfieldPrimary;
    ULONG                   cbKeyMostPrimary;
    ULONG                   cbKeyTruncatedPrimary;
    BYTE*                   rgbDenormPrimary;
};

LOCAL ERR ErrRECIStreamRecordsOnSecondaryIndexIFilter(
    FUCB * const                                    pfucbIndex,
    STREAM_RECORDS_SECONDARY_FILTER_CONTEXT * const pcontext )
{
    ERR             err                                 = JET_errSuccess;
    BOOL            fSecondaryKeyTruncated              = fFalse;
    ERR             rgerrSecondary[JET_ccolKeyMost];
    DATA            rgvalueSecondary[JET_ccolKeyMost];
    BOOL            fPrimaryKeyTruncated                = fFalse;
    ERR             rgerrPrimary[JET_ccolKeyMost];
    DATA            rgvaluePrimary[JET_ccolKeyMost];


    Call( ErrRECIStreamRecordsICommonFilter( pfucbIndex, (STREAM_RECORDS_COMMON_FILTER_CONTEXT* const)pcontext ) );
    if ( err > JET_errSuccess )
    {
        goto HandleError;
    }


    if ( pcontext->iidxsegSecondaryMax != ulMax )
    {
        const BYTE *        pbKeySecondary      = pcontext->rgbKeySecondary;
        const INT           cbKeySecondary      = pfucbIndex->kdfCurr.key.Cb();
        const BYTE * const  pbKeyMaxSecondary   = pbKeySecondary + cbKeySecondary;
        pfucbIndex->kdfCurr.key.CopyIntoBuffer( (void*)pbKeySecondary, pcontext->cbKeyMostSecondary );

        fSecondaryKeyTruncated = (ULONG)cbKeySecondary >= pcontext->cbKeyTruncatedSecondary;

        if ( !fSecondaryKeyTruncated )
        {
            ULONG ibDenormSecondary = 0;
            for ( ULONG iidxsegSecondary = 0; iidxsegSecondary <= pcontext->iidxsegSecondaryMax; iidxsegSecondary++ )
            {
                rgvalueSecondary[iidxsegSecondary].SetPv( pcontext->rgbDenormSecondary + ibDenormSecondary );
                rgvalueSecondary[iidxsegSecondary].SetCb( pcontext->cbKeyMostSecondary - ibDenormSecondary );
                BOOL fNull;
                RECIRetrieveColumnFromKey(  pcontext->ptdbSecondary,
                                            pcontext->pidbSecondary,
                                            pcontext->rgidxsegSecondary + iidxsegSecondary,
                                            pcontext->rgfieldSecondary + iidxsegSecondary,
                                            pbKeySecondary,
                                            pbKeyMaxSecondary,
                                            &rgvalueSecondary[iidxsegSecondary],
                                            &fNull );
                rgerrSecondary[iidxsegSecondary] = fNull ? JET_wrnColumnNull : JET_errSuccess;
                ibDenormSecondary += rgvalueSecondary[iidxsegSecondary].Cb();
            }
        }
    }


    if ( pcontext->iidxsegPrimaryMax != ulMax )
    {
        const BYTE *        pbKeyPrimary    = (const BYTE*)pfucbIndex->kdfCurr.data.Pv();
        const INT           cbKeyPrimary    = pfucbIndex->kdfCurr.data.Cb();
        const BYTE * const  pbKeyMaxPrimary = pbKeyPrimary + cbKeyPrimary;

        fPrimaryKeyTruncated = (ULONG)cbKeyPrimary >= pcontext->cbKeyTruncatedPrimary;

        if ( !fPrimaryKeyTruncated )
        {
            ULONG ibDenormPrimary = 0;
            for ( ULONG iidxsegPrimary = 0; iidxsegPrimary <= pcontext->iidxsegPrimaryMax; iidxsegPrimary++ )
            {
                rgvaluePrimary[iidxsegPrimary].SetPv( pcontext->rgbDenormPrimary + ibDenormPrimary );
                rgvaluePrimary[iidxsegPrimary].SetCb( pcontext->cbKeyMostPrimary - ibDenormPrimary );
                BOOL fNull;
                RECIRetrieveColumnFromKey(  pcontext->ptdbPrimary,
                                            pcontext->pidbPrimary,
                                            pcontext->rgidxsegPrimary + iidxsegPrimary,
                                            pcontext->rgfieldPrimary + iidxsegPrimary,
                                            pbKeyPrimary,
                                            pbKeyMaxPrimary,
                                            &rgvaluePrimary[iidxsegPrimary],
                                            &fNull );
                rgerrPrimary[iidxsegPrimary] = fNull ? JET_wrnColumnNull : JET_errSuccess;
                ibDenormPrimary += rgvaluePrimary[iidxsegPrimary].Cb();
            }
        }
    }


    for ( ULONG icolumnid = 0; icolumnid < pcontext->ccolumnid; icolumnid++ )
    {

        JET_COLUMNID columnid = pcontext->rgcolumnid[icolumnid];


        const ULONG iidxsegSecondary = pcontext->mpicolumnidiidxsegSecondary[icolumnid];
        if ( iidxsegSecondary != ulMax && !fSecondaryKeyTruncated )
        {
            Call( ErrRECIStreamRecordsIAppendColumnValue(
                pcontext,
                columnid,
                rgerrSecondary[iidxsegSecondary],
                rgvalueSecondary[iidxsegSecondary].Cb(),
                rgvalueSecondary[iidxsegSecondary].Pv() ) );
        }
        else
        {
            const ULONG iidxsegPrimary = pcontext->mpicolumnidiidxsegPrimary[icolumnid];
            if ( iidxsegPrimary != ulMax && !fPrimaryKeyTruncated )
            {
                Call( ErrRECIStreamRecordsIAppendColumnValue(
                    pcontext,
                    columnid,
                    rgerrPrimary[iidxsegPrimary],
                    rgvaluePrimary[iidxsegPrimary].Cb(),
                    rgvaluePrimary[iidxsegPrimary].Pv() ) );
            }
            else
            {
                Call( ErrRECIStreamRecordsIAppendColumnValue(
                    pcontext,
                    columnid,
                    JET_wrnColumnNotInRecord,
                    0,
                    NULL ) );
            }
        }
    }


    pcontext->cRecords++;
    *pcontext->pcbActual = pcontext->cbDataGenerated;


    err = wrnBTNotVisibleAccumulated;

HandleError:
    if ( err == JET_errBufferTooSmall )
    {
        err = JET_errSuccess;
    }
    return err;
}

LOCAL ERR ErrRECIStreamRecordsOnSecondaryIndexIGetKeyColumns(
    _In_ FUCB * const                                           pfucb,
    _In_ FUCB * const                                           pfucbIndex,
    _Out_ FCB * * const                                         ppfcbTable,
    _Out_ TDB * * const                                         pptdb,
    _Out_ IDB * * const                                         ppidb,
    _Out_ ULONG *                                       pcidxseg,
    _Outptr_result_buffer_( *pcidxseg ) IDXSEG * * const        prgidxseg,
    _Outptr_result_buffer_( *pcidxseg ) FIELD * * const         prgfield,
    _Outptr_result_buffer_( *pcidxseg ) BOOL * * const          prgfCanDenormalize,
    _Out_ ULONG *                                       pcbKeyMost,
    _Out_ ULONG *                                       pcbKeyTruncated
    )
{
    ERR                 err                     = JET_errSuccess;
    FCB*                pfcbTableT;
    FCB*&               pfcbTable               = ppfcbTable ? *ppfcbTable : pfcbTableT;
                        pfcbTable               = pfcbNil;
    TDB*                ptdbT;
    TDB*&               ptdb                    = pptdb ? *pptdb : ptdbT;
                        ptdb                    = ptdbNil;
    IDB*                pidbT;
    IDB*&               pidb                    = ppidb ? *ppidb : pidbT;
                        pidb                    = pidbNil;
    ULONG               cidxsegT;
    ULONG&              cidxseg                 = pcidxseg ? *pcidxseg : cidxsegT;
                        cidxseg                 = 0;
    IDXSEG*             rgidxsegT;
    IDXSEG*&            rgidxseg                = prgidxseg ? *prgidxseg : rgidxsegT;
                        rgidxseg                = NULL;
    FIELD*              rgfieldT;
    FIELD*&             rgfield                 = prgfield ? *prgfield : rgfieldT;
                        rgfield                 = NULL;
    BOOL*               rgfCanDenormalizeT;
    BOOL*&              rgfCanDenormalize       = prgfCanDenormalize ? *prgfCanDenormalize : rgfCanDenormalizeT;
                        rgfCanDenormalize       = NULL;
    ULONG               cbKeyMostT;
    ULONG&              cbKeyMost               = pcbKeyMost ? *pcbKeyMost : cbKeyMostT;
                        cbKeyMost               = 0;
    ULONG               cbKeyTruncatedT;
    ULONG&              cbKeyTruncated          = pcbKeyTruncated ? *pcbKeyTruncated : cbKeyTruncatedT;
                        cbKeyTruncated          = 0;

    const FCB * const   pfcbIndex               = pfucbIndex->u.pfcb;
    BOOL                fLeaveDMLLatch          = fFalse;
    bool                fSawUnicodeTextColumn   = fFalse;

    pfcbTable       = pfcbIndex->FDerivedIndex() ? pfucb->u.pfcb->Ptdb()->PfcbTemplateTable() : pfucb->u.pfcb;
    ptdb            = pfcbTable->Ptdb();
    pidb            = pfcbIndex->Pidb();
    cbKeyMost       = pidb == pidbNil ? sizeof( DBK ) : pidb->CbKeyMost();
    cbKeyTruncated  = ( ( pidb == pidbNil || pidb->CbKeyMost() < JET_cbKeyMostMin || pidb->FDisallowTruncation() ) ?
                            ulMax :
                            cbKeyMost );

    fLeaveDMLLatch = fTrue;
    pfcbTable->EnterDML();

    cidxseg = pidb == pidbNil ? 0 : pidb->Cidxseg();

    Alloc( rgidxseg = new IDXSEG[cidxseg] );
    Alloc( rgfield = new FIELD[cidxseg] );
    Alloc( rgfCanDenormalize = new BOOL[cidxseg] );

    if ( cidxseg )
    {
        memcpy( rgidxseg, PidxsegIDBGetIdxSeg( pidb, ptdb ), cidxseg * sizeof( IDXSEG ) );
    }

    for ( ULONG iidxseg = 0; iidxseg < cidxseg; iidxseg++ )
    {
        const JET_COLUMNID  columnid        = rgidxseg[iidxseg].Columnid();
        FIELD * const       pfield          = &rgfield[iidxseg];
        BOOL &              fCanDenormalize = rgfCanDenormalize[iidxseg];

        memcpy( pfield, ptdb->Pfield( columnid ), sizeof( FIELD ) );
        fCanDenormalize = fTrue;


        if ( FRECTextColumn( pfield->coltyp ) && usUniCodePage == pfield->cp )
        {
            fSawUnicodeTextColumn = fTrue;
        }

        if ( fSawUnicodeTextColumn )
        {
            fCanDenormalize = fFalse;
        }

        if ( pidb->FTuples() && !iidxseg )
        {
            fCanDenormalize = fFalse;
        }

        if ( FRECTextColumn( pfield->coltyp ) )
        {
            fCanDenormalize = fFalse;
        }

        if (    !FCOLUMNIDFixed( columnid )
                && FRECBinaryColumn( pfield->coltyp )
                && pidb->CbVarSegMac() < cbKeyMost
                && pfield->cbMaxLen
                && pidb->CbVarSegMac() < 1 + ( ( ( pfield->cbMaxLen + 7 ) / 8 ) * 9 ) )
        {
            fCanDenormalize = fFalse;
        }

        if ( FCOLUMNIDTagged( columnid ) && FFIELDMultivalued( pfield->ffield ) )
        {
            fCanDenormalize = fFalse;
        }
    }

HandleError:
    if ( fLeaveDMLLatch )
    {
        pfcbTable->LeaveDML();
    }
    if ( err < JET_errSuccess )
    {
        delete[] rgidxseg;
        delete[] rgfield;
        delete[] rgfCanDenormalize;
        pfcbTable = pfcbNil;
        ptdb = ptdbNil;
        pidb = pidbNil;
        cidxseg = 0;
        rgidxseg = NULL;
        rgfield = NULL;
        rgfCanDenormalize = NULL;
        cbKeyMost = 0;
        cbKeyTruncated = 0;
    }
    return err;
}

LOCAL ERR ErrIsamStreamRecordsOnSecondaryIndex(
    _In_ PIB * const                                            ppib,
    _In_ FUCB * const                                           pfucb,
    _In_ const ULONG                                    ccolumnid,
    _In_reads_opt_( ccolumnid ) const JET_COLUMNID * const      rgcolumnid,
    _Out_writes_bytes_to_opt_( cbData, cbActual ) void * const  pvData,
    _In_ const ULONG                                    cbData,
    _Inout_ ULONG &                                     cbActual,
    _In_ const JET_GRBIT                                        grbit )
{
    ERR                                     err                         = JET_errSuccess;
    RECORD_BUFFER_HEADER_V1                 headerV1;
    STREAM_RECORDS_SECONDARY_FILTER_CONTEXT context;
    ULONG*                                  mpicolumnidiidxsegSecondary = NULL;
    ULONG                                   iidxsegSecondaryMax         = ulMax;
    FCB*                                    pfcbTableSecondary          = pfcbNil;
    TDB*                                    ptdbSecondary               = ptdbNil;
    IDB*                                    pidbSecondary               = pidbNil;
    ULONG                                   cidxsegSecondary            = 0;
    IDXSEG*                                 rgidxsegSecondary           = NULL;
    FIELD*                                  rgfieldSecondary            = NULL;
    BOOL*                                   rgfCanDenormalizeSecondary  = NULL;
    ULONG                                   cbKeyMostSecondary          = 0;
    ULONG                                   cbKeyTruncatedSecondary     = 0;
    BYTE*                                   rgbDenormSecondary          = NULL;
    BYTE*                                   rgbKeySecondary             = NULL;
    ULONG*                                  mpicolumnidiidxsegPrimary   = NULL;
    ULONG                                   iidxsegPrimaryMax           = ulMax;
    FCB*                                    pfcbTablePrimary            = pfcbNil;
    TDB*                                    ptdbPrimary                 = ptdbNil;
    IDB*                                    pidbPrimary                 = pidbNil;
    ULONG                                   cidxsegPrimary              = 0;
    IDXSEG*                                 rgidxsegPrimary             = NULL;
    FIELD*                                  rgfieldPrimary              = NULL;
    BOOL*                                   rgfCanDenormalizePrimary    = NULL;
    ULONG                                   cbKeyMostPrimary            = 0;
    ULONG                                   cbKeyTruncatedPrimary       = 0;
    BYTE*                                   rgbDenormPrimary            = NULL;
    ULONG                                   ccolumnidContext            = 0;
    JET_COLUMNID*                           rgcolumnidContext           = NULL;


    Assert( pfucb->pfucbCurIndex != pfucbNil );


    Call( ErrRECIStreamRecordsOnSecondaryIndexIGetKeyColumns(   pfucb,
                                                                pfucb->pfucbCurIndex,
                                                                &pfcbTableSecondary,
                                                                &ptdbSecondary,
                                                                &pidbSecondary,
                                                                &cidxsegSecondary,
                                                                &rgidxsegSecondary,
                                                                &rgfieldSecondary,
                                                                &rgfCanDenormalizeSecondary,
                                                                &cbKeyMostSecondary,
                                                                &cbKeyTruncatedSecondary ) );
    Alloc( rgbDenormSecondary = new BYTE[cbKeyMostSecondary] );
    Alloc( rgbKeySecondary = new BYTE[cbKeyMostSecondary] );
    Call( ErrRECIStreamRecordsOnSecondaryIndexIGetKeyColumns(   pfucb,
                                                                pfucb,
                                                                &pfcbTablePrimary,
                                                                &ptdbPrimary,
                                                                &pidbPrimary,
                                                                &cidxsegPrimary,
                                                                &rgidxsegPrimary,
                                                                &rgfieldPrimary,
                                                                &rgfCanDenormalizePrimary,
                                                                &cbKeyMostPrimary,
                                                                &cbKeyTruncatedPrimary ) );
    Alloc( rgbDenormPrimary = new BYTE[cbKeyMostPrimary] );


    if ( rgcolumnid )
    {
        ccolumnidContext = ccolumnid;
        rgcolumnidContext = (JET_COLUMNID*)rgcolumnid;

        Alloc( mpicolumnidiidxsegSecondary = new ULONG[ccolumnidContext] );
        Alloc( mpicolumnidiidxsegPrimary = new ULONG[ccolumnidContext] );

        for ( ULONG icolumnid = 0; icolumnid < ccolumnidContext; icolumnid++ )
        {
            FIELD fieldFC;
            BOOL fEncrypted;
            Call( ErrRECIAccessColumn( pfucb, rgcolumnidContext[icolumnid], &fieldFC, &fEncrypted ) );

            mpicolumnidiidxsegSecondary[icolumnid] = ulMax;
            for ( ULONG iidxsegSecondary = 0; iidxsegSecondary < cidxsegSecondary; iidxsegSecondary++ )
            {
                if (    rgidxsegSecondary[iidxsegSecondary].Columnid() == rgcolumnidContext[icolumnid]
                        && rgfCanDenormalizeSecondary[iidxsegSecondary] )
                {
                    mpicolumnidiidxsegSecondary[icolumnid] = iidxsegSecondary;
                    iidxsegSecondaryMax = iidxsegSecondaryMax == ulMax ? iidxsegSecondary : max( iidxsegSecondaryMax, iidxsegSecondary );
                    break;
                }
            }

            mpicolumnidiidxsegPrimary[icolumnid] = ulMax;
            for ( ULONG iidxsegPrimary = 0; iidxsegPrimary < cidxsegPrimary; iidxsegPrimary++ )
            {
                if (    rgidxsegPrimary[iidxsegPrimary].Columnid() == rgcolumnidContext[icolumnid]
                        && rgfCanDenormalizePrimary[iidxsegPrimary] )
                {
                    mpicolumnidiidxsegPrimary[icolumnid] = iidxsegPrimary;
                    iidxsegPrimaryMax = iidxsegPrimaryMax == ulMax ? iidxsegPrimary : max( iidxsegPrimaryMax, iidxsegPrimary );
                    break;
                }
            }
        }
    }
    else
    {
        ccolumnidContext = cidxsegSecondary + cidxsegPrimary;
        Alloc( rgcolumnidContext = new JET_COLUMNID[ccolumnidContext] );
        Alloc( mpicolumnidiidxsegSecondary = new ULONG[ccolumnidContext] );
        Alloc( mpicolumnidiidxsegPrimary = new ULONG[ccolumnidContext] );

        for ( ULONG iidxsegSecondary = 0; iidxsegSecondary < cidxsegSecondary; iidxsegSecondary++ )
        {
            rgcolumnidContext[iidxsegSecondary] = rgidxsegSecondary[iidxsegSecondary].Columnid();
            mpicolumnidiidxsegSecondary[iidxsegSecondary] = ulMax;
            if ( rgfCanDenormalizeSecondary[iidxsegSecondary] )
            {
                mpicolumnidiidxsegSecondary[iidxsegSecondary] = iidxsegSecondary;
                iidxsegSecondaryMax = iidxsegSecondary;
            }
            mpicolumnidiidxsegPrimary[iidxsegSecondary] = ulMax;
        }

        for ( ULONG iidxsegPrimary = 0; iidxsegPrimary < cidxsegPrimary; iidxsegPrimary++ )
        {
            rgcolumnidContext[cidxsegSecondary + iidxsegPrimary] = rgidxsegPrimary[iidxsegPrimary].Columnid();
            mpicolumnidiidxsegSecondary[cidxsegSecondary + iidxsegPrimary] = ulMax;
            mpicolumnidiidxsegPrimary[cidxsegSecondary + iidxsegPrimary] = ulMax;
            if ( rgfCanDenormalizePrimary[iidxsegPrimary] )
            {
                mpicolumnidiidxsegPrimary[cidxsegSecondary + iidxsegPrimary] = iidxsegPrimary;
                iidxsegPrimaryMax = iidxsegPrimary;
            }
        }
    }


    headerV1.rbfi           = rbfiV1;
    headerV1.cbHeader       = sizeof( headerV1 );
    headerV1.ibRead         = sizeof( headerV1 );
    headerV1.iRecord        = ulMax;
    headerV1.columnid       = JET_columnidNil;
    headerV1.itagSequence   = 1;

    if ( cbData < sizeof( headerV1 ) )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    memcpy( pvData, &headerV1, sizeof( headerV1 ) );
    cbActual += sizeof( headerV1 );


    err = ErrIsamMove( ppib, pfucb, ( grbit & JET_bitStreamForward ) ? JET_MovePrevious : JET_MoveNext, NO_GRBIT );
    if ( err == JET_errNoCurrentRecord )
    {
        err = JET_errSuccess;
    }
    Call( err );


    context.pvData                      = pvData;
    context.cbData                      = cbData;
    context.pcbActual                   = &cbActual;
    context.grbit                       = grbit;

    context.ccolumnid                   = ccolumnidContext;
    context.rgcolumnid                  = rgcolumnidContext;

    context.cRecords                    = 0;
    context.cbDataGenerated             = cbActual;

    context.mpicolumnidiidxsegSecondary = mpicolumnidiidxsegSecondary;
    context.iidxsegSecondaryMax         = iidxsegSecondaryMax;
    context.pfcbTableSecondary          = pfcbTableSecondary;
    context.ptdbSecondary               = ptdbSecondary;
    context.pidbSecondary               = pidbSecondary;
    context.cidxsegSecondary            = cidxsegSecondary;
    context.rgidxsegSecondary           = rgidxsegSecondary;
    context.rgfieldSecondary            = rgfieldSecondary;
    context.cbKeyMostSecondary          = cbKeyMostSecondary;
    context.cbKeyTruncatedSecondary     = cbKeyTruncatedSecondary;
    context.rgbDenormSecondary          = rgbDenormSecondary;
    context.rgbKeySecondary             = rgbKeySecondary;

    context.mpicolumnidiidxsegPrimary   = mpicolumnidiidxsegPrimary;
    context.iidxsegPrimaryMax           = iidxsegPrimaryMax;
    context.pfcbTablePrimary            = pfcbTablePrimary;
    context.ptdbPrimary                 = ptdbPrimary;
    context.pidbPrimary                 = pidbPrimary;
    context.cidxsegPrimary              = cidxsegPrimary;
    context.rgidxsegPrimary             = rgidxsegPrimary;
    context.rgfieldPrimary              = rgfieldPrimary;
    context.cbKeyMostPrimary            = cbKeyMostPrimary;
    context.cbKeyTruncatedPrimary       = cbKeyTruncatedPrimary;
    context.rgbDenormPrimary            = rgbDenormPrimary;

    RECAddMoveFilter( pfucb->pfucbCurIndex, (PFN_MOVE_FILTER)ErrRECIStreamRecordsOnSecondaryIndexIFilter, (MOVE_FILTER_CONTEXT *)&context );


    err = ErrIsamMove( ppib, pfucb, ( grbit & JET_bitStreamForward ) ? JET_MoveNext : JET_MovePrevious, NO_GRBIT );
    if ( err == JET_errNoCurrentRecord )
    {
        err = ErrERRCheck( JET_wrnNoMoreRecords );
    }
    Call( err );

HandleError:
    RECRemoveMoveFilter( pfucb->pfucbCurIndex, (PFN_MOVE_FILTER)ErrRECIStreamRecordsOnSecondaryIndexIFilter );
    if ( rgcolumnidContext != rgcolumnid )
    {
        delete[] rgcolumnidContext;
    }
    delete[] mpicolumnidiidxsegSecondary;
    delete[] rgidxsegSecondary;
    delete[] rgfieldSecondary;
    delete[] rgfCanDenormalizeSecondary;
    delete[] rgbDenormSecondary;
    delete[] rgbKeySecondary;
    delete[] mpicolumnidiidxsegPrimary;
    delete[] rgidxsegPrimary;
    delete[] rgfieldPrimary;
    delete[] rgfCanDenormalizePrimary;
    delete[] rgbDenormPrimary;
    return err;
}

ERR VTAPI ErrIsamStreamRecords(
    _In_ JET_SESID                                                  sesid,
    _In_ JET_TABLEID                                                tableid,
    _In_ const ULONG                                        ccolumnid,
    _In_reads_opt_( ccolumnid ) const JET_COLUMNID * const          rgcolumnid,
    _Out_writes_bytes_to_opt_( cbData, *pcbActual ) void * const    pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit )
{
    ERR         err                 = JET_errSuccess;
    PIB* const  ppib                = (PIB * const)sesid;
    FUCB* const pfucb               = (FUCB * const)tableid;
    ULONG       cbActualT;
    ULONG&      cbActual            = pcbActual ? *pcbActual : cbActualT;
                cbActual            = 0;

    BOOL        fTransactionStarted = fFalse;


    if ( ppib == ppibNil || pfucb == pfucbNil )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( !pfucb->u.pfcb->FTypeTable() )
    {
        Error( ErrERRCheck( JET_errInvalidOperation ) );
    }
    if ( ccolumnid && !rgcolumnid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( !ccolumnid && rgcolumnid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( cbData && !pvData )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( !pvData && !pcbActual )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    if ( grbit & ~( JET_bitStreamColumnReferences |
                    JET_bitStreamForward |
                    JET_bitStreamBackward ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }



    if ( !!( grbit & JET_bitStreamForward ) == !!( grbit & JET_bitStreamBackward ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }


    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRNoLatch( ppib );
    Assert( FFUCBIndex( pfucb ) );
    Assert( !FFUCBSort( pfucb ) );

    if ( FFMPIsTempDB( pfucb->ifmp ) )
    {
        Expected( fFalse );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    if ( 0 == ppib->Level() )
    {
        Call( ErrIsamBeginTransaction( sesid, 33668, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }


    if ( pfucb->pfucbCurIndex != pfucbNil )
    {
        Call( ErrIsamStreamRecordsOnSecondaryIndex( ppib, pfucb, ccolumnid, rgcolumnid, pvData, cbData, cbActual, grbit ) );
    }
    else
    {
        Call( ErrIsamStreamRecordsOnPrimaryIndex( ppib, pfucb, ccolumnid, rgcolumnid, pvData, cbData, cbActual, grbit ) );
    }

HandleError:
    if ( fTransactionStarted )
    {
        CallS( ErrIsamCommitTransaction( sesid, NO_GRBIT, 0, NULL ) );
    }
    AssertDIRNoLatch( ppib );
    return err;
}

ERR VTAPI ErrIsamRetrieveColumnFromRecordStream(
    _Inout_updates_bytes_( cbData ) void * const    pvData,
    _In_ const ULONG                        cbData,
    _Out_ ULONG * const                     piRecord,
    _Out_ JET_COLUMNID * const                      pcolumnid,
    _Out_ ULONG * const                     pitagSequence,
    _Out_ ULONG * const                     pibValue,
    _Out_ ULONG * const                     pcbValue )
{
    ERR                             err         = JET_errSuccess;
    RECORD_BUFFER_HEADER_V1* const  pheaderV1   = (RECORD_BUFFER_HEADER_V1*)pvData;


    if ( !pvData )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( cbData < sizeof( RECORD_BUFFER_HEADER_COMMON ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( !piRecord )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( !pcolumnid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( !pitagSequence )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( !pibValue )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( !pcbValue )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    if ( pheaderV1->rbfi != rbfiV1 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( cbData < sizeof( RECORD_BUFFER_HEADER_V1 ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( pheaderV1->cbHeader != sizeof( RECORD_BUFFER_HEADER_V1 ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( pheaderV1->ibRead < sizeof( RECORD_BUFFER_HEADER_V1 ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( pheaderV1->ibRead > cbData )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    if ( pheaderV1->ibRead == cbData )
    {
        err = ErrERRCheck( JET_wrnNoMoreRecords );
        goto HandleError;
    }


    if ( cbData - pheaderV1->ibRead < sizeof( RECORD_BUFFER_COLUMN_VALUE ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    const RECORD_BUFFER_COLUMN_VALUE* const pcolumnValue = (const RECORD_BUFFER_COLUMN_VALUE* const)( (BYTE*)pvData + pheaderV1->ibRead );
    if ( sizeof( RECORD_BUFFER_COLUMN_VALUE ) + pcolumnValue->cbValue < pcolumnValue->cbValue )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( cbData - pheaderV1->ibRead < sizeof( RECORD_BUFFER_COLUMN_VALUE ) + pcolumnValue->cbValue )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    bool fNewRecord = ( pcolumnValue->iRecordMod2 != 0 ) ^ ( ( pheaderV1->iRecord & 1 ) != 0 );
    pheaderV1->iRecord += fNewRecord ? 1 : 0;
    pheaderV1->itagSequence = !fNewRecord && pheaderV1->columnid == pcolumnValue->columnid ? pheaderV1->itagSequence + 1 : 1;
    pheaderV1->columnid = pcolumnValue->columnid;
    pheaderV1->ibRead += (ULONG)( sizeof( RECORD_BUFFER_COLUMN_VALUE ) + pcolumnValue->cbValue );


    *piRecord = pheaderV1->iRecord;
    *pcolumnid = pheaderV1->columnid;
    *pitagSequence = pheaderV1->itagSequence;
    *pibValue = (ULONG)( &pcolumnValue->rgbValue[0] - (BYTE*)pvData );
    *pcbValue = pcolumnValue->cbValue;

    if ( pcolumnValue->fNotAvailable )
    {
        err = ErrERRCheck( JET_wrnColumnNotInRecord );
    }
    if ( pcolumnValue->fReference )
    {
        err = ErrERRCheck( JET_wrnColumnReference );
    }
    if ( pcolumnValue->fNull )
    {
        err = ErrERRCheck( JET_wrnColumnNull );
    }

HandleError:
    return err;
}
