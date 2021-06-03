// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"


//  =======================================================================
//  class CFixedColumnIter
//  -----------------------------------------------------------------------
//

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

//  =======================================================================


//  =======================================================================
//  class CVariableColumnIter
//  -----------------------------------------------------------------------
//

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

//  =======================================================================


//  =======================================================================
//  class CSingleValuedTaggedColumnValueIter
//  -----------------------------------------------------------------------
//

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
            // get the logical size of the data
            Call( ErrPKDecompressData( data, pfucb, NULL, 0, &cbDataActual ) );
            Assert( JET_wrnBufferTruncated == err );
            err = JET_errSuccess;
            Assert( m_fEncrypted || (ULONG)cbDataActual >= m_cbData );
        }

        precsize->cbData += cbDataActual;
        // this is the physical size
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

//  =======================================================================


//  =======================================================================
//  class CDualValuedTaggedColumnValueIter
//  -----------------------------------------------------------------------
//

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
    precsize->cMultiValues++;   //  for the purposes of JET_RECSIZE, we only count multi-values beyond the first

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

//  =======================================================================


//  =======================================================================
//  class CMultiValuedTaggedColumnValueIter
//  -----------------------------------------------------------------------
//

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
        //  initialize counts, then increment them as we iterate
        //  through the multi-values
        //
        precsize->cbOverhead += sizeof(TAGFLD) + sizeof(TAGFLD_HEADER) + m_pmultivalues->IbStartOfMVData();

        //  must iterate through and check which ones are separated
        //
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
    precsize->cMultiValues += ( cMultiValues - 1 );     //  for the purposes of JET_RECSIZE, we only count multi-values beyond the first

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
    const ULONG cbMVOverhead    = sizeof(TAGFLD) + ( fSeparatable ? sizeof(BYTE) : 0 );     //  if separatable, long-value overhead required
    size_t      cbESE97Format   = cbMVOverhead * cMultiValues;  //  initialise with overhead for each multi-value

    for ( ULONG imv = 0; imv < cMultiValues; imv++ )
    {
        const ULONG     cbMV    = m_pmultivalues->CbData( imv );

        if ( fSeparatable )
        {
            //  if the column is separatable and the data is greater than sizeof(LID),
            //  we may force the data to an LV
            //
            cbESE97Format += min( cbMV, sizeof(_LID32) );
        }
        else
        {
            cbESE97Format += cbMV;
        }
    }

    return cbESE97Format;
}

//  =======================================================================

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

            //  HACK: treat derived columns in original-format derived table as
            //  non-derived, because they don't have the fDerived bit set in the TAGFLD
            fUseDerivedBit = FRECUseDerivedBitForTemplateColumnInDerivedTable( columnid, pfcb->Ptdb() );

            // switch to template table
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

//  =======================================================================


//  =======================================================================
//  class CUnionIter
//  -----------------------------------------------------------------------
//

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

//  =======================================================================


//  =======================================================================

#include <pshpack1.h>

typedef BYTE ColumnReferenceFormatIdentifier;

PERSISTED
enum ColumnReferenceFormatIdentifiers : ColumnReferenceFormatIdentifier
{
    crfiInvalid         = 0x00,
    crfiV1              = 0x01,  // CColumnReferenceFormatV1
    crfiV2              = 0x02,  // CColumnReferenceFormatV2
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

        UnalignedLittleEndian<USHORT>                           m_cbBookmark;                                                   // 1 - JET_cbBookmarkMostMost
        BYTE                                                    m_cbColumnName;                                                 // 1 - JET_cbNameMost
        UnalignedLittleEndian<ULONG>                            m_itagSequence;
        UnalignedLittleEndian<_LID32>                           m_lid;                                                          // unstable identifier / hint, changes with signature change
        BYTE                                                    m_rgbDatabaseSignature[OffsetOf( SIGNATURE, szComputerName )];  // szComputeName elided as it is never set
        BYTE                                                    m_rgbBookmark[0];                                               // m_cbBookmark, normalized key
        //BYTE                                                  m_rgbColumnName[0];                                             // m_cbColumnName, ASCII chars
};

// V2 structure is the same except it stores a 64-bit lid.
PERSISTED
class CColumnReferenceFormatV2 : public CColumnReferenceFormatCommon
{
    public:

        UnalignedLittleEndian<USHORT>                           m_cbBookmark;                                                   // 1 - JET_cbBookmarkMostMost
        BYTE                                                    m_cbColumnName;                                                 // 1 - JET_cbNameMost
        UnalignedLittleEndian<ULONG>                            m_itagSequence;
        UnalignedLittleEndian<LvId>                             m_lid;                                                          // unstable identifier / hint, changes with signature change
        BYTE                                                    m_rgbDatabaseSignature[ OffsetOf( SIGNATURE, szComputerName ) ];    // szComputeName elided as it is never set
        BYTE                                                    m_rgbBookmark[ 0 ];                                             // m_cbBookmark, normalized key
        //BYTE                                                  m_rgbColumnName[0];                                             // m_cbColumnName, ASCII chars
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
    //  init out params for failure

    *plid               = lidMin;
    *prgbBookmark       = NULL;
    *pcbBookmark        = 0;
    *prgchColumnName    = NULL;
    *pcchColumnName     = 0;
    *pitagSequence      = 0;

    //  validate the column reference

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

    //  unpack all the values from the column reference

    *plid = lid;
    *prgbBookmark = rgbBookmark;
    *pcbBookmark = cbBookmark;
    *prgchColumnName = (CHAR*) &rgbBookmark[ cbBookmark ];
    *pcchColumnName = cbColumnName;
    *pitagSequence = itagSequence;

    //  invalidate the LID if we cannot prove that it isn't stale

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

    //  we do not support column references on temp tables

    Assert( !FFUCBSort( pfucb ) );
    if ( FFMPIsTempDB( pfucb->ifmp ) )
    {
        Expected( fFalse ); // API never run on temp DB.
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  validate the bookmark length

    SIZE_T cbBookmark = pfucb->bmCurr.key.Cb();
    Assert( cbBookmark > 0 );
    Assert( cbBookmark <= ( pfucb->u.pfcb->Pidb() ? pfucb->u.pfcb->Pidb()->CbKeyMost() : sizeof( DBK ) ) );
    Assert( cbBookmark <= wMax );

    //  determine the name of this column

    if ( !pfcb->FTemplateTable() )
    {
        pfcb->EnterDML();
        fLeaveDML = fTrue;
    }
    Call( ErrFILEGetPfieldFromColumnid( pfcb, columnid, &pfield ) );
    CHAR* szColumnName = pfcb->Ptdb()->SzFieldName( pfield->itagFieldName, fFalse );
    SIZE_T cbColumnName = LOSStrLengthA( szColumnName ) * sizeof( CHAR );

    //  validate the column name length

    Assert( cbColumnName > 0 );
    Assert( cbColumnName <= JET_cbNameMost );
    Assert( cbColumnName <= bMax );

    // Select the column reference version to use based on efv.
    // We don't use Ptdb()->FLid64() here deliberately.
    // Column reference format is independent of the properties of the current table.
    // This means that we will return crfv2 even for tables with 32-bit LIDs, if the efv is enabled.
    // This is purely theoretical in the current implementation, because Ptdb()->FLid64() and efv mirror each other.
    // But that could change easily.
    bool fUseCrfv2 = ( JET_errSuccess == PfmpFromIfmp( pfucb->ifmp )->ErrDBFormatFeatureEnabled( JET_efvLid64 ) );

    //  allocate enough memory to hold the ref
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

    //  generate the ref
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

    //  init out params for failure

    *pcolumnid = JET_columnidNil;

    //  get the column name from the reference as a null terminated string

    CHAR szColumn[JET_cbNameMost + 1];
    if ( cchColumnName > JET_cbNameMost )
    {
        Error( ErrERRCheck( JET_errInvalidColumnReference ) );
    }
    UtilMemCpy( szColumn, rgchColumnName, cchColumnName );
    szColumn[cchColumnName] = 0;

    //  lookup the column id

    JET_COLUMNDEF columndef;
    err = ErrIsamGetTableColumnInfo( sesid, tableid, szColumn, NULL, &columndef, sizeof( columndef ), JET_ColInfo, false );
    if ( err == JET_errColumnNotFound )
    {
        Error( ErrERRCheck( JET_errStaleColumnReference ) );
    }

    //  this column had better be for a long value

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
#endif  //  DEBUG

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

    // validate parameters

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

    //  only these grbits are supported at this time

    if ( grbit & ~( JET_bitRetrievePhysicalSize ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  check our session and table

    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRNoLatch( ppib );
    Assert( FFUCBIndex( pfucb ) );
    Assert( !FFUCBSort( pfucb ) );
    
    if( FFMPIsTempDB( pfucb->ifmp ) )
    {
        Expected( fFalse ); // API never run on temp DB.
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    

    // begin transaction for read consistency

    if ( 0 == ppib->Level() )
    {
        Call( ErrIsamBeginTransaction( sesid, 54276, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }

    //  parse the column reference

    Call( ErrRECIParseColumnReference(  pfucb,
                                        (const BYTE* const)pvReference,
                                        cbReference,
                                        &lid,
                                        &rgbBookmark,
                                        &cbBookmark,
                                        &rgchColumnName,
                                        &cchColumnName,
                                        &itagSequence ) );

    //  if the column reference is current then use its physical identifiers to fetch the column value from its LV

    if ( lid != lidMin )
    {
        //  UNDONE:  the encryption state is in the LV but the code is structured so that we must know this beforehand.
        //  this seems to be an unnecessary complication that could be handled internally to ErrRECRetrieveSLongField

        //  establish the encryption state of the column

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

        //  try to preread the LV before we check its existence.  otherwise we will wait twice:  once for the LV Root
        //  and once for the specific data we are going to read

        Call( ErrLVPrereadLongValue( pfucb, lid, ibData, cbData ) );

        //  verify that the LV still exists without declaring corruption if it doesn't

        BOOL fExists = fFalse;
        Call( ErrRECDoesSLongFieldExist( pfucb, lid, &fExists ) );

        //  if the LV still exists then retrieve the requested portion of the requested column value directly from the LV
        //  tree.  if it doesn't then we will fallback to retrieving it via the record

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

                //  verify the value we just fetched

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

#endif  //  DEBUG
            }
            fRetrievedValue = fTrue;
        }
    }

    //  if we still don't have the column value then the column reference is stale

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
#endif  //  DEBUG
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

    //  validate parameters

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

    //  only these grbits are supported at this time

    if ( grbit & ~( JET_bitPrereadFirstPage ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  check our session and table

    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRNoLatch( ppib );
    Assert( FFUCBIndex( pfucb ) );
    Assert( !FFUCBSort( pfucb ) );

    if( FFMPIsTempDB( pfucb->ifmp ) )
    {
        Expected( fFalse ); // API never run on temp DB.
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    
    //  parse the column references

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

    //  for any column references that were current, directly preread the LVs by LID

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

    //  release our page latch, if any

    if ( Pcsr( pfucb )->FLatched() )
    {
        CallS( ErrDIRRelease( pfucb ) );
    }
    AssertDIRNoLatch( pfucb->ppib );

    //  walk all column values looking for missing separated LVs

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

                //  if this isn't a missing separated LV then skip it

                if ( pEnumColumnValue->err != wrnRECSeparatedLV && pEnumColumnValue->err != wrnRECSeparatedEncryptedLV )
                {
                    continue;
                }

                //  retrieve the LID/fEncrypted from cbData
                //  cbData is used as an index into rgLidStore (if this column value was a separated LV)
                Assert( pEnumColumnValue->cbData < rgLidStore.Size() );
                const LvId lid = rgLidStore[ pEnumColumnValue->cbData ];
                const BOOL fEncrypted = ( pEnumColumnValue->err == wrnRECSeparatedEncryptedLV );

                pEnumColumnValue->err       = JET_errSuccess;
                pEnumColumnValue->cbData    = 0;

                // if the caller requested a reference then return it instead of the data

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
                    //  fetch up to the requested maximum column value size of this
                    //  LV.  note that in this mode, we are retrieving a pointer to
                    //  a newly allocated buffer containing the LV data NOT the LV
                    //  data itself

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

                    //  if the returned cbActual is larger than cbDataMost then we
                    //  obviously only got cbDataMost bytes of data.  warn the caller
                    //  that they didn't get all the data

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
    const INT               iMinGrowthLidStore          = 8;    // magic number, grow by 64-bytes (atleast)


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

    //  validate parameters

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

    //  NOCODE:  JET_bitEnumerateLocal is NYI

    if ( grbit & JET_bitEnumerateLocal )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  NOCODE:  integration with user-defined callbacks is NYI

    if (    ptdb->FTableHasUserDefinedDefault() &&
            !( grbit & ( JET_bitEnumerateIgnoreDefault | JET_bitEnumerateIgnoreUserDefinedDefault ) ) )
    {
        Error( ErrERRCheck( JET_errCallbackFailed ) );
    }

    //  these options are mutually exclusive

    if ( !!( grbit & JET_bitEnumerateInRecordOnly ) && !!( grbit & JET_bitEnumerateAsRefIfNotInRecord ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  build an iterator tree over all our input data
    //
    //  NOTE:  if no input data is needed then the root iterator will be NULL!
    //  NOTE:  make sure we have enough iterator storage to hold these iterators

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

    //  get access to the data we need
    //
    //  NOTE:  do not unlatch the page until we are done with the iterators!

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

    //  do not allow retrieve as ref if the storage isn't stable

    if (    fRetrieveAsRefIfNotInRecord &&
            ( FFUCBUpdatePrepared( pfucb ) || FFUCBSort( pfucb ) || FFMPIsTempDB( pfucb->ifmp ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  get worst case storage for the column array

    Call( pciterRoot->ErrGetWorstCaseColumnCount( &cAlloc ) );
    cbAlloc = cAlloc * sizeof( JET_ENUMCOLUMN );

    Alloc( rgEnumColumn = (JET_ENUMCOLUMN*)pfnRealloc(
            pvReallocContext,
            NULL,
            cbAlloc ) );

    cEnumColumn = (ULONG)cAlloc;
    memset( rgEnumColumn, 0, cbAlloc );

    // Improve the worst possible scenario:
    // if every column has exactly 1 value which is a lid,
    // we would end up with a realloc per lid.
    if ( rgEnumLidStore.ErrSetCapacity( cEnumColumn ) != CArray<LvId>::ERR::errSuccess )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }
    rgEnumLidStore.SetCapacityGrowth( iMinGrowthLidStore );

    //  walk all columns in the input data

    Call( pciterRoot->ErrMoveBeforeFirst() );
    for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
    {
        JET_ENUMCOLUMN* const pEnumColumn = &rgEnumColumn[ iEnumColumn ];

        //  if we have walked off the end of the input array then we are done

        err = pciterRoot->ErrMoveNext();
        if ( err == errRECNoCurrentColumnValue )
        {
            err = JET_errSuccess;
            cEnumColumn = iEnumColumn;
            continue;
        }
        Call( err );

        //  if we do not have access to this column then do not include it
        //  in the output array

        Call( pciterRoot->ErrGetColumnId( &pEnumColumn->columnid ) );

        err = ErrRECIAccessColumn( pfucb, pEnumColumn->columnid, &fieldFC, &fEncrypted );
        if ( err == JET_errColumnNotFound )
        {
            err = JET_errSuccess;
            iEnumColumn--;
            continue;
        }
        Call( err );

        //  get the properties for this column

        Call( pciterRoot->ErrGetColumnValueCount( &cColumnValue ) );

        //  if the column is NULL then include it in the output array

        if ( !cColumnValue )
        {
            pEnumColumn->err = ErrERRCheck( JET_wrnColumnNull );
            continue;
        }

        //  if we are testing for the presence of a column value only then
        //  return that it is present but do not return any column values

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

        //  if there is only one column value and output compression was
        //  requested and the caller asked for all column values then we may be
        //  able to put the column value directly in the JET_ENUMCOLUMN struct

        if (    cColumnValue == 1 &&
                !fEncrypted &&
                ( grbit & JET_bitEnumerateCompressOutput ) )
        {
            //  get the properties for this column value

            Call( pciterRoot->ErrGetColumnValue( 1, &cbData, &pvData, &fSeparated, &fCompressed ) );

            //  this column value is suitable for compression
            //
            //  NOTE:  currently, this criteria equals zero chance of needing
            //  to return a warning for this column

            if ( !fSeparated )
            {
                //  store the column value in the JET_ENUMCOLUMN struct

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

                //  if this is an escrow update column then adjust it

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

                //  we're done with this column

                continue;
            }
        }

        //  get storage for the column value array

        cAlloc  = cColumnValue;
        cbAlloc = cAlloc * sizeof( JET_ENUMCOLUMNVALUE );

        Alloc( pEnumColumn->rgEnumColumnValue = (JET_ENUMCOLUMNVALUE*)pfnRealloc(
                pvReallocContext,
                NULL,
                cbAlloc ) );

        pEnumColumn->cEnumColumnValue = (ULONG)cAlloc;
        memset( pEnumColumn->rgEnumColumnValue, 0, cbAlloc );

        rgEnumLidStore.SetCapacityGrowth( max( iMinGrowthLidStore, cColumnValue ) );

        //  walk all column values for this column in the input data

        for (   iEnumColumnValue = 0;
                iEnumColumnValue < pEnumColumn->cEnumColumnValue;
                iEnumColumnValue++ )
        {
            JET_ENUMCOLUMNVALUE* const pEnumColumnValue = &pEnumColumn->rgEnumColumnValue[ iEnumColumnValue ];

            pEnumColumnValue->itagSequence = (ULONG)( iEnumColumnValue + 1 );

            //  get the properties for this column value

            Call( pciterRoot->ErrGetColumnValue( pEnumColumnValue->itagSequence, &cbData, &pvData, &fSeparated, &fCompressed ) );

            //  if this column value is a separated long value and we are only
            //  enumerating data in the record then return that there is a value
            //  but don't actually fetch it

            if ( fSeparated && fInRecordOnly )
            {
                pEnumColumnValue->err = ErrERRCheck( JET_wrnColumnNotInRecord );
                continue;
            }

            //  if this column value is a separated long value, then we must
            //  defer fetching it until after we have iterated the record.
            //  we will do this by flagging the column value with wrnRECSeparatedLV
            //  and pointing cbData to the LID stored in rgEnumLidStore.

            if ( fSeparated )
            {
                // Really hacky way to pass on LID/fEncrypted to ErrRECIFetchMissingLVs
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

            //  store the column value

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

            //  if this is an escrow update column then adjust it

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

    //  we should have reached the end of the input data or else the "worst
    //  case" column count wasn't actually the worst case now was it?

    Assert( pciterRoot->ErrMoveNext() == errRECNoCurrentColumnValue );

    //  we need to fixup some missing separated LVs
    //
    //  NOTE:  as soon as we do this our iterators are invalid

    if ( fSeparatedLV )
    {
        //  If we are retrieving an after-image or
        //  haven't replaced a LV we can simply go
        //  to the LV tree. Otherwise we have to
        //  perform a more detailed consultation of
        //  the version store with ErrRECGetLVImage
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

    //  cleanup

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
    const INT               iMinGrowthLidStore          = 8;    // magic number, grow by 64-bytes (atleast)
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

    //  validate parameters

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

    //  NOCODE:  JET_bitEnumerateLocal is NYI

    if ( grbit & JET_bitEnumerateLocal )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  JET_bitEnumerateTaggedOnly is not supported when requesting specific columns

    if ( grbit & JET_bitEnumerateTaggedOnly )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  NOCODE:  integration with user-defined callbacks is NYI

    if (    pfucb->u.pfcb->Ptdb()->FTableHasUserDefinedDefault() &&
            !( grbit & ( JET_bitEnumerateIgnoreDefault | JET_bitEnumerateIgnoreUserDefinedDefault ) ) )
    {
        Error( ErrERRCheck( JET_errCallbackFailed ) );
    }

    //  these options are mutually exclusive

    if ( !!( grbit & JET_bitEnumerateInRecordOnly ) && !!( grbit & JET_bitEnumerateAsRefIfNotInRecord ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    fInRecordOnly               = grbit & JET_bitEnumerateInRecordOnly;
    fRetrieveAsRefIfNotInRecord = grbit & JET_bitEnumerateAsRefIfNotInRecord;

    //  get access to the data we need

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

    //  do not allow retrieve as ref if the storage isn't stable

    if (    fRetrieveAsRefIfNotInRecord &&
            ( FFUCBUpdatePrepared( pfucb ) || FFUCBSort( pfucb ) || FFMPIsTempDB( pfucb->ifmp ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  allocate storage for the column array

    cAlloc  = cEnumColumnId;
    cbAlloc = cAlloc * sizeof( JET_ENUMCOLUMN );

    Alloc( rgEnumColumn = (JET_ENUMCOLUMN*)pfnRealloc(
            pvReallocContext,
            NULL,
            cbAlloc ) );

    cEnumColumn = (ULONG)cAlloc;
    memset( rgEnumColumn, 0, cbAlloc );

    // Improve the worst possible scenario:
    // if every column has exactly 1 value which is a lid,
    // we would end up with a realloc per lid.
    if ( rgEnumLidStore.ErrSetCapacity( cEnumColumn ) != CArray<LvId>::ERR::errSuccess )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }
    rgEnumLidStore.SetCapacityGrowth( iMinGrowthLidStore );
    //  walk all requested columns

    for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
    {
        JET_ENUMCOLUMN* const pEnumColumn = &rgEnumColumn[ iEnumColumn ];

        //  if the requested column id is zero then we should skip this column

        pEnumColumn->columnid = rgEnumColumnId[ iEnumColumn ].columnid;

        if ( !pEnumColumn->columnid )
        {
            pEnumColumn->err = ErrERRCheck( JET_wrnColumnSkipped );
            continue;
        }

        //  if we do not have access to the requested column then set the
        //  appropriate error for this column so that the caller can see
        //  which column ids were bad

        err = ErrRECIAccessColumn( pfucb, pEnumColumn->columnid, &fieldFC, &fEncrypted );
        if ( err == JET_errBadColumnId || err == JET_errColumnNotFound )
        {
            pEnumColumn->err = err;

            fColumnIdError = fTrue;

            err = JET_errSuccess;
            continue;
        }
        Call( err );

        //  lookup the value for this column in the record.  if we cannot find
        //  a value there and we are returning default values then lookup the
        //  value for this column in the default record

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

        //  get the properties for this column

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

        //  if the column wasn't found and JET_bitEnumerateIgnoreDefault was
        //  specified and the caller asked for all column values then return
        //  that the column is set to the default

        if (    !fColumnFound && ( grbit & JET_bitEnumerateIgnoreDefault ) &&
                !rgEnumColumnId[ iEnumColumn ].ctagSequence )
        {
            pEnumColumn->err = ErrERRCheck( JET_wrnColumnDefault );
            continue;
        }

        //  if the column is explicitly set to NULL and the caller asked for
        //  all column values then return that the column is NULL

        if (    !cColumnValue &&
                !rgEnumColumnId[ iEnumColumn ].ctagSequence )
        {
            pEnumColumn->err = ErrERRCheck( JET_wrnColumnNull );
            continue;
        }

        //  if we are testing for the presence of a column value only and
        //  the caller asked for all column values then return that the column
        //  is present but do not return any column values

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

        //  if there is only one column value and output compression was
        //  requested and the caller asked for all column values then we may be
        //  able to put the column value directly in the JET_ENUMCOLUMN struct

        if (    cColumnValue == 1 &&
                !fEncrypted &&
                ( grbit & JET_bitEnumerateCompressOutput ) &&
                !rgEnumColumnId[ iEnumColumn ].ctagSequence )
        {
            //  get the properties for this column value

            Call( pciter->ErrGetColumnValue( 1, &cbData, &pvData, &fSeparated, &fCompressed ) );

            //  this column value is suitable for compression
            //
            //  NOTE:  currently, this criteria equals zero chance of needing
            //  to return a warning for this column

            if ( !fSeparated )
            {
                //  store the column value in the JET_ENUMCOLUMN struct

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

                //  if this is an escrow update column then adjust it

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

                //  we're done with this column

                continue;
            }
        }

        //  get storage for the column value array

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

        //  walk all requested column values
        //  NOCODE:  JET_wrnColumnMoreTags is NYI

        for (   iEnumColumnValue = 0;
                iEnumColumnValue < pEnumColumn->cEnumColumnValue;
                iEnumColumnValue++ )
        {
            JET_ENUMCOLUMNVALUE* const pEnumColumnValue = &pEnumColumn->rgEnumColumnValue[ iEnumColumnValue ];

            //  if the requested itagSequence is zero then we should skip
            //  this column value

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

            //  if there is no corresponding column value for the requested
            //  itagSequence and the column was not found and
            //  JET_bitEnumerateIgnoreDefault was specified then the column
            //  value is the default

            if (    pEnumColumnValue->itagSequence > cColumnValue &&
                    !fColumnFound &&
                    ( grbit & JET_bitEnumerateIgnoreDefault ) )
            {
                pEnumColumnValue->err = ErrERRCheck( JET_wrnColumnDefault );
                continue;
            }

            //  if there is no corresponding column value for the requested
            //  itagSequence then the column value is NULL

            if ( pEnumColumnValue->itagSequence > cColumnValue )
            {
                pEnumColumnValue->err = ErrERRCheck( JET_wrnColumnNull );
                continue;
            }

            //  if we are testing for the presence of a column value only then
            //  return that it is present but do not return the column value

            if ( grbit & JET_bitEnumeratePresenceOnly )
            {
                pEnumColumnValue->err = ErrERRCheck( JET_wrnColumnPresent );
                continue;
            }

            //  get the properties for this column value

            Call( pciter->ErrGetColumnValue( pEnumColumnValue->itagSequence, &cbData, &pvData, &fSeparated, &fCompressed ) );

            //  if this column value is a separated long value and we are only
            //  enumerating data in the record then return that there is a value
            //  but don't actually fetch it

            if ( fSeparated && fInRecordOnly )
            {
                pEnumColumnValue->err = ErrERRCheck( JET_wrnColumnNotInRecord );
                continue;
            }

            //  if this column value is a separated long value, then we must
            //  defer fetching it until after we have iterated the record.
            //  we will do this by flagging the column value with wrnRECSeparatedLV
            //  and pointing cbData to the LID stored in rgEnumLidStore.

            if ( fSeparated )
            {
                // Really hacky way to pass on LID/fEncrypted to ErrRECIFetchMissingLVs
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

            //  store the column value

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

            //  if this is an escrow update column then adjust it

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

    //  we need to fixup some missing separated LVs

    if ( fSeparatedLV )
    {
        //  If we are retrieving an after-image or
        //  haven't replaced a LV we can simply go
        //  to the LV tree. Otherwise we have to
        //  perform a more detailed consultation of
        //  the version store with ErrRECGetLVImage
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

    //  if there was a column id error then return the first one we see

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

    //  cleanup

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
        // Begin transaction for read consistency (in case page
        // latch must be released in order to validate column).
        Call( ErrDIRBeginTransaction( ppib, 51493, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }

    AssertDIRNoLatch( ppib );
    Assert( FFUCBSort( pfucb ) || FFUCBIndex( pfucb ) );

    //  if no column list is given then enumerate all columns

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

    //  if an explicit column list is provided then enumerate those columns

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
        //  No updates performed, so force success.
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

    //  Compute key size.
    //  For compatibility with JET_RECISZE2/JET_RECSIZE, cbKey is counted into cbOverhead.
    //  Key sizes are calculated using the full key (not the compressed key).
    //  So the actual key size on the page may differ from the reported size.
    //  JetGetRecordSize() aims for the contract: report the total number of bytes needed to store
    //  the record. That means we should report uncompressed key size.
    //
    if ( Pcsr( pfucb )->FLatched() || FFUCBSort( pfucb ) )
    {
        Assert( locOnCurBM == pfucb->locLogical );
        precsize->cbOverhead += pfucb->kdfCurr.key.Cb();
        precsize->cbKey += pfucb->kdfCurr.key.Cb();
    }
    else if ( pfucb->cbstat == fCBSTATNull || FFUCBReplacePrepared( pfucb ) )
    {
        // Currency should be valid and on the current key, if we aren't doing any updates or OR doing a replace.
        // Current key isn't valid if we are doing an insert (or variants of InsertCopy).
        Assert( locOnCurBM == pfucb->locLogical );
        precsize->cbOverhead += pfucb->bmCurr.key.Cb();
        precsize->cbKey += pfucb->bmCurr.key.Cb();
    }
    else
    {
        //  key hasn't been formulated yet, so there's no way to know how big it is
        //
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
        //  process the result of column navigation
        //
        Call( err );

        Call( citerTC.ErrGetColumnSize( pfucb, precsize, grbit ) );
    }

    //  if we have walked off the end of the input array then we are done
    //
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
        //  begin transaction for read consistency
        //
        Call( ErrDIRBeginTransaction( ppib, 45349, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }

    fUseCopyBuffer = ( ( ( grbit & JET_bitRecordSizeInCopyBuffer ) && FFUCBUpdatePrepared( pfucb ) && !FFUCBNeverRetrieveCopy( pfucb ) )
                    || FFUCBAlwaysRetrieveCopy( pfucb ) );

    Call( ErrRECIGetRecord( pfucb, &pdataRec, fUseCopyBuffer ) );

    // If we are fetching LV sizes, we may need to populate the LV FUCB and that requires taking a lock
    // with lower rank than a BF latch. So, make a copy of the data and release the latch.
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
        //  no updates performed, so force success
        //
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

//  JetStreamRecords record buffer format
//
//  We have to presume that someone may persist the stream to read later.

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

    //  this space is reserved as working memory for processing the buffer

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

    //  check any higher priority filter

    Call( ErrRECCheckMoveFilter( pfucb, (MOVE_FILTER_CONTEXT* const)pcontext ) );
    if ( err > JET_errSuccess )
    {
        goto HandleError;
    }

    //  check the index range before we accumulate any records

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

    //  check for enough room to copy the column value to the output.  if there isn't enough room then just fail.
    //  we will correct our output to be on a record boundary later

    if ( pcontext->cbDataGenerated + sizeof( RECORD_BUFFER_COLUMN_VALUE ) + cbData > pcontext->cbData )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    //  copy the column value to the output

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

    //  query the in memory schema for the column meta-data

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

    //  save this column metadata in our cache

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
    //  try to find the column metadata in our cache

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

    //  we didn't find the column metadata in our cache so fetch it and cache it

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

    //  iterate over each column value

    Call( pcontext->pciterRoot->ErrGetColumnValueCount( &cColumnValue ) );

    for ( ULONG itagSequence = 1; itagSequence <= cColumnValue; itagSequence++ )
    {
        //  get the current column value and its properties

        ERR errData = JET_errSuccess;
        size_t cbData;
        VOID* pvData;
        BOOL fSeparated;
        BOOL fCompressed;
        Call( pcontext->pciterRoot->ErrGetColumnValue( itagSequence, &cbData, &pvData, &fSeparated, &fCompressed ) );

        //  handle separated long values

        if ( fSeparated )
        {
            //  emit a column reference for this separated long value.  we cannot fetch the data because we are
            //  currently scanning a B+ Tree using a move filter.  we cannot access another B+ Tree when doing this

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

            //  otherwise we will return an indicator that the column value is not available

            else
            {
                errData = JET_wrnColumnNotInRecord;
                pvData = NULL;
                cbData = 0;
            }
        }

        //  handle all other column values

        else
        {
            //  decrypt the column value if required

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

            //  decompress the column value if required

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

            //  if this is an escrow update column then adjust it to the correct value

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

        //  emit this column value

        Call( ErrRECIStreamRecordsIAppendColumnValue( pcontext, columnid, errData, cbData, pvData ) );
    }

    //  if there were no column values then emit an explicit null column value

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

    //  perform the common filter check

    Call( ErrRECIStreamRecordsICommonFilter( pfucb, (STREAM_RECORDS_COMMON_FILTER_CONTEXT* const)pcontext ) );
    if ( err > JET_errSuccess )
    {
        goto HandleError;
    }

    //  load the current record into our iterator tree

    Call( pcontext->pciterRec->ErrSetRecord( pfucb->kdfCurr.data ) );

    //  append the requested column values from this record

    if ( !pcontext->rgcolumnid )
    {
        //  iterate over all column values in the record

        Call( pcontext->pciterRoot->ErrMoveBeforeFirst() );
        while ( ( err = pcontext->pciterRoot->ErrMoveNext() ) >= JET_errSuccess )
        {
            //  get the columnid of the column value we just found

            JET_COLUMNID columnid;
            Call( pcontext->pciterRoot->ErrGetColumnId( &columnid ) );

            //  get the meta-data for this column

            BOOL fVisible;
            BOOL fEncrypted;
            BOOL fEscrow;
            Call( ErrRECIStreamRecordsOnPrimaryIndexIGetColumnMetaData( pcontext,
                                                                        pfucb,
                                                                        columnid,
                                                                        &fVisible,
                                                                        &fEncrypted,
                                                                        &fEscrow ) );

            //  if this column is not visible to this transaction then do not emit it

            if ( !fVisible )
            {
                continue;
            }

            //  append the column values for this column to the output

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
            //  get the columnid of the column values we want to find

            JET_COLUMNID columnid = pcontext->rgcolumnid[icolumnid];

            //  look for this columnid in the record

            if ( ( err = pcontext->pciterRoot->ErrSeek( columnid ) ) >= JET_errSuccess )
            {
                //  append the column values for this column to the output

                Call( ErrRECIStreamRecordsOnPrimaryIndexIAppendColumnValues(    pfucb,
                                                                                pcontext,
                                                                                columnid,
                                                                                pcontext->rgfEncrypted[icolumnid],
                                                                                pcontext->rgfEscrow[icolumnid] ) );
            }

            //  if there were no column values then emit an explicit null column value

            else if ( err == errRECColumnNotFound )
            {
                Call( ErrRECIStreamRecordsIAppendColumnValue( pcontext, columnid, JET_wrnColumnNull, 0, NULL ) );
            }
            Call( err );
        }
    }

    //  we have successfully processed this record.  make it visible in the output buffer

    pcontext->cRecords++;
    *pcontext->pcbActual = pcontext->cbDataGenerated;

    //  we have successfully processed this record.  *ignore it* so that we can process the next record

    err = wrnBTNotVisibleAccumulated;

HandleError:
    //  if we ran out of record or buffer quota then we should *not ignore* the record so processing will stop
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

    //  this should be called on a primary or sequential index

    Assert( pfucb->pfucbCurIndex == pfucbNil );

    //  NOCODE:  integration with user-defined callbacks is NYI

    if ( ptdb->FTableHasUserDefinedDefault() )
    {
        Error( ErrERRCheck( JET_errCallbackFailed ) );
    }

    //  build an iterator tree over all our input data
    //
    //  NOTE:  if no input data is needed then the root iterator will be NULL!
    //  NOTE:  make sure we have enough iterator storage to hold these iterators

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

    //  do not allow retrieve as ref if the storage isn't stable

    if ( fUseColumnReferences && ( FFUCBSort( pfucb ) || FFMPIsTempDB( pfucb->ifmp ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  setup access to the default record if needed

    if ( fDefaultRecord && pciterDefaultRec )
    {
        DATA * const pdataDefaultRec = ptdb->PdataDefaultRecord();
        Call( pciterDefaultRec->ErrSetRecord( *pdataDefaultRec ) );
    }

    //  preprocess any explicitly requested columns

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

    //  if no column restriction is specified and the total number of columns is small then we will preprocess all
    //  columns.  otherwise, we will fallback to a column meta-data cache

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

    //  add the buffer format header

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

    //  move by one in the opposite direction so that we include the current record in our output

    err = ErrIsamMove( ppib, pfucb, ( grbit & JET_bitStreamForward ) ? JET_MovePrevious : JET_MoveNext, NO_GRBIT );
    if ( err == JET_errNoCurrentRecord )
    {
        err = JET_errSuccess;
    }
    Call( err );

    //  setup and install our move filter context

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

    //  move by one.  this will cause us to build up the output via our move filter.  this will result in the cursor
    //  being placed on the next row to process not the last row we processed

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

    //  perform the common filter check

    Call( ErrRECIStreamRecordsICommonFilter( pfucbIndex, (STREAM_RECORDS_COMMON_FILTER_CONTEXT* const)pcontext ) );
    if ( err > JET_errSuccess )
    {
        goto HandleError;
    }

    //  denormalize all the key column values that we need from the secondary index key

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

    //  denormalize all the key column values that we need from the primary index key / bookmark

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

    //  append the requested column values from this record

    for ( ULONG icolumnid = 0; icolumnid < pcontext->ccolumnid; icolumnid++ )
    {
        //  get the columnid of the column values we want to find

        JET_COLUMNID columnid = pcontext->rgcolumnid[icolumnid];

        //  look for this columnid in the index key or primary bookmark

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

    //  we have successfully processed this record.  make it visible in the output buffer

    pcontext->cRecords++;
    *pcontext->pcbActual = pcontext->cbDataGenerated;

    //  we have successfully processed this record.  *ignore it* so that we can processed the next record

    err = wrnBTNotVisibleAccumulated;

HandleError:
    //  if we ran out of record or buffer quota then we should *not ignore* the record so processing will stop
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

        //  record any reason why we cannot denormalize this key column
        //
        //  NOTE:  these rules are analogous to those in ErrRECIRetrieveFromIndex

        if ( FRECTextColumn( pfield->coltyp ) && usUniCodePage == pfield->cp )
        {
            fSawUnicodeTextColumn = fTrue;
        }

        //  any previous key column contains unicode text (embedded nulls derail denormalization)
        if ( fSawUnicodeTextColumn )
        {
            fCanDenormalize = fFalse;
        }

        //  this is a tuple index and this is the tuplized key column (partial data)
        if ( pidb->FTuples() && !iidxseg )
        {
            fCanDenormalize = fFalse;
        }

        //  this is a text key column (data loss on normalization)
        if ( FRECTextColumn( pfield->coltyp ) )
        {
            fCanDenormalize = fFalse;
        }

        //  this is a variable/tagged binary column truncated by VarSegMac and not by the max key length (data loss on
        //  normalization)
        if (    !FCOLUMNIDFixed( columnid )
                && FRECBinaryColumn( pfield->coltyp )
                && pidb->CbVarSegMac() < cbKeyMost
                && pfield->cbMaxLen
                && pidb->CbVarSegMac() < 1 + ( ( ( pfield->cbMaxLen + 7 ) / 8 ) * 9 ) )
        {
            fCanDenormalize = fFalse;
        }

        //  this is a multi-valued column (cannot compute the itagSequence w/o the record)
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

    //  this should be called on a secondary index

    Assert( pfucb->pfucbCurIndex != pfucbNil );

    //  get all columns that could be available from the secondary index key or the primary bookmark

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

    //  preprocess column access.  if no columns were requested then we will default to the set of key columns that are
    //  in the secondary index and the primary index.  we don't have to access check those because they must exist by
    //  definition

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

    //  add the buffer format header

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

    //  move by one in the opposite direction so that we include the current record in our output

    err = ErrIsamMove( ppib, pfucb, ( grbit & JET_bitStreamForward ) ? JET_MovePrevious : JET_MoveNext, NO_GRBIT );
    if ( err == JET_errNoCurrentRecord )
    {
        err = JET_errSuccess;
    }
    Call( err );

    //  setup and install our move filter context

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

    //  move by one.  this will cause us to build up the output via our move filter.  this will result in the cursor
    //  being placed on the next row to process not the last row we processed

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

    //  validate parameters

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

    //  only these grbits are supported at this time

    if ( grbit & ~( JET_bitStreamColumnReferences |
                    JET_bitStreamForward |
                    JET_bitStreamBackward ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  validate grbits

    //  the direction must be specified

    if ( !!( grbit & JET_bitStreamForward ) == !!( grbit & JET_bitStreamBackward ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  check our session and table

    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRNoLatch( ppib );
    Assert( FFUCBIndex( pfucb ) );
    Assert( !FFUCBSort( pfucb ) );

    if ( FFMPIsTempDB( pfucb->ifmp ) )
    {
        Expected( fFalse ); // API never run on temp DB.
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  begin transaction for read consistency

    if ( 0 == ppib->Level() )
    {
        Call( ErrIsamBeginTransaction( sesid, 33668, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }

    //  dispatch to the correct implementation

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

    //  validate parameters

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

    //  validate buffer header

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

    //  if we are already at the end of the buffer then we are done

    if ( pheaderV1->ibRead == cbData )
    {
        err = ErrERRCheck( JET_wrnNoMoreRecords );
        goto HandleError;
    }

    //  validate the next column value to read

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

    //  read the next column value from the buffer

    bool fNewRecord = ( pcolumnValue->iRecordMod2 != 0 ) ^ ( ( pheaderV1->iRecord & 1 ) != 0 );
    pheaderV1->iRecord += fNewRecord ? 1 : 0;
    pheaderV1->itagSequence = !fNewRecord && pheaderV1->columnid == pcolumnValue->columnid ? pheaderV1->itagSequence + 1 : 1;
    pheaderV1->columnid = pcolumnValue->columnid;
    pheaderV1->ibRead += (ULONG)( sizeof( RECORD_BUFFER_COLUMN_VALUE ) + pcolumnValue->cbValue );

    //  return the column value we read

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
