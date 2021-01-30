// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "PageSizeClean.hxx"

const BYTE  bPrefixNull         = 0x00;
const BYTE  bPrefixZeroLength   = 0x40;
const BYTE  bPrefixNullHigh     = 0xc0;
const BYTE  bPrefixData         = 0x7f;
const BYTE  bSentinel           = 0xff;

const BYTE  bUnicodeSortKeySentinel     = 0x01;
const BYTE  bUnicodeSortKeyTerminator   = 0x00;

const BYTE  bASCIISortKeyTerminator     = 0x00;


LOCAL ERR ErrFLDNormalizeTextSegment(
    BYTE *                      pbField,
    const ULONG                 cbField,
    BYTE *                      rgbSeg,
    INT *                       pcbSeg,
    const ULONG                 cbKeyAvail,
    const ULONG                 cbKeyMost,
    const ULONG                 cbVarSegMac,
    const USHORT                cp,
    _In_ const NORM_LOCALE_VER* pnlv )
{
    ERR     err     = JET_errSuccess;

    Assert( NULL != pbField );
    Assert( cbField > 0 );

    Assert( cbKeyAvail > 0 );
    Assert( cbVarSegMac > 0 );

    Assert( cbVarSegMac <= cbKeyMost+1 );
    Assert( cbKeyMost != cbVarSegMac );

    Assert( cbVarSegMac < cbKeyMost || cbKeyAvail < cbVarSegMac );
    INT cbT = min( cbKeyAvail, cbVarSegMac );

    cbT--;

    ULONG cbChar = cp == usUniCodePage ? sizeof( wchar_t ) : sizeof( char );

    ULONG cbFieldT = ( cbField / cbChar ) * cbChar;

    if ( cbKeyMost == JET_cbKeyMost_OLD )
    {
        cbFieldT = min( cbFieldT, ( ( cbKeyMost + cbChar - 1 ) / cbChar ) * cbChar );
    }

    if ( cp == usUniCodePage )
    {
        err = ErrNORMMapString(
                    pnlv,
                    pbField,
                    cbFieldT,
                    rgbSeg + 1,
                    cbT,
                    pcbSeg );
        if ( err < 0 )
        {
#ifdef DEBUG
            switch ( err )
            {
                case JET_errInvalidLanguageId:
                case JET_errOutOfMemory:
                case JET_errUnicodeNormalizationNotSupported:
                    break;
                default:
                    CallS( err );
            }
#endif
            return err;
        }
    }
    else
    {
        err = ErrUtilNormText(
                    (CHAR *)pbField,
                    cbFieldT,
                    cbT,
                    (CHAR *)rgbSeg + 1,
                    pcbSeg );
    }

    Assert( JET_errSuccess == err || wrnFLDKeyTooBig == err );
    if ( wrnFLDKeyTooBig == err )
    {
        const INT   cbVarSegMacNoHeader = cbVarSegMac - 1;

        Assert( cbKeyMost != cbVarSegMac );
        Assert( *pcbSeg <= cbVarSegMacNoHeader );
        if ( cbVarSegMac > cbKeyMost || *pcbSeg < cbVarSegMacNoHeader )
        {
            Assert( ULONG( *pcbSeg + 1 ) == cbKeyAvail );
        }
        else
        {
            Assert( cbVarSegMac < cbKeyMost );
            Assert( cbVarSegMacNoHeader == *pcbSeg );
            err = JET_errSuccess;
        }
    }

    Assert( *pcbSeg >= 0 );
    Assert( *pcbSeg <= cbT );

    *rgbSeg = bPrefixData;
    (*pcbSeg)++;

    return err;
}


#ifndef RTM
VOID FLDNormUnitTest()
{
    const wchar_t wszUndefined[]    = L"\xe001\xfdd2\xe0e0\x8c0";
    const INT cbUndefined           = sizeof( wszUndefined );
    const wchar_t wszDefined[]      = L"abcDEF123$%^;,.";
    const INT cbDefined             = sizeof( wszDefined );

    BYTE rgbNormalized[JET_cbKeyMost_OLD];
    INT cbNormalized = sizeof( rgbNormalized );

    NORM_LOCALE_VER nlv =
    {
        SORTIDNil,
        dwLCMapFlagsDefault,
        0,
        0,
        L'\0',
    };
    OSStrCbCopyW( &nlv.m_wszLocaleName[0], sizeof(nlv.m_wszLocaleName), wszLocaleNameDefault );

    ERR err;

    err = ErrFLDNormalizeTextSegment(
            (BYTE *)wszDefined,
            cbDefined,
            rgbNormalized,
            &cbNormalized,
            JET_cbKeyMost_OLD,
            JET_cbKeyMost_OLD,
            JET_cbKeyMost_OLD+1,
            usUniCodePage,
            &nlv );

    Enforce( JET_errSuccess == err );
    Enforce( cbNormalized >= cbDefined );

    cbNormalized = sizeof( rgbNormalized );
    err = ErrFLDNormalizeTextSegment(
            (BYTE *)wszUndefined,
            cbUndefined,
            rgbNormalized,
            &cbNormalized,
            JET_cbKeyMost_OLD,
            JET_cbKeyMost_OLD,
            JET_cbKeyMost_OLD+1,
            usUniCodePage,
            &nlv );

    Enforce( JET_errSuccess == err );
    Enforce( cbNormalized <= cbUndefined );
}
#endif

LOCAL VOID FLDNormalizeBinarySegment(
    const BYTE      * const pbField,
    const ULONG     cbField,
    __inout_bcount_part(cbKeyAvail, *pcbSeg) BYTE           * const rgbSeg,
    INT             * const pcbSeg,
    const ULONG     cbKeyAvail,
    const ULONG     cbKeyMost,
    const ULONG     cbVarSegMac,
    const BOOL      fFixedField,
    BOOL            * const pfColumnTruncated,
    ULONG           * pibBinaryColumnDelimiter )
{
    ULONG           cbSeg;

    Assert( NULL == pibBinaryColumnDelimiter
        || 0 == *pibBinaryColumnDelimiter );

    Assert( NULL != pbField );
    Assert( cbField > 0 );

    Assert( cbKeyAvail > 0 );
    Assert( cbVarSegMac > 0 );

    Assert( cbVarSegMac <= cbKeyMost+1 );
    Assert( cbKeyMost != cbVarSegMac );

    Assert( cbVarSegMac < cbKeyMost || cbKeyAvail < cbVarSegMac );

    rgbSeg[0] = bPrefixData;

    if ( fFixedField )
    {
        cbSeg = cbField + 1;

        if ( cbVarSegMac < cbKeyMost && cbSeg > cbVarSegMac )
        {
            cbSeg = cbVarSegMac;
        }

        if ( cbSeg > cbKeyAvail )
        {
            cbSeg = cbKeyAvail;
            *pfColumnTruncated = fTrue;
        }

        Assert( cbSeg > 0 );
        UtilMemCpy( rgbSeg+1, pbField, cbSeg-1 );
    }
    else
    {
        BOOL            fNormalisedEntireColumn     = fTrue;
        const ULONG     cChunks                     = ( cbField + ( cbFLDBinaryChunk-1 ) ) / cbFLDBinaryChunk;

        cbSeg = ( cChunks * cbFLDBinaryChunkNormalized ) + 1;

        if ( cbVarSegMac < cbKeyMost && cbSeg > cbVarSegMac )
        {
            cbSeg = cbVarSegMac;
            fNormalisedEntireColumn = fFalse;
        }

        if ( cbSeg > cbKeyAvail )
        {
            cbSeg = cbKeyAvail;
            fNormalisedEntireColumn = fFalse;
            *pfColumnTruncated = fTrue;
        }

        Assert( cbSeg > 0 );
        Assert( cbSeg > cbFLDBinaryChunkNormalized || !fNormalisedEntireColumn );

        ULONG       cbSegRemaining = cbSeg - 1;
        ULONG       cbFieldRemaining = cbField;
        BYTE        *pbSeg = rgbSeg + 1;
        const BYTE  *pbNextChunk = pbField;
        while ( cbSegRemaining >= cbFLDBinaryChunkNormalized )
        {
            Assert( cbFieldRemaining > 0 );
            Assert( pbNextChunk + cbFieldRemaining == pbField + cbField );

            if ( cbFieldRemaining <= cbFLDBinaryChunk )
            {
                Assert( cbSegRemaining - cbFLDBinaryChunkNormalized == 0 );
                UtilMemCpy( pbSeg, pbNextChunk, cbFieldRemaining );
                pbSeg += cbFieldRemaining;

                if ( NULL != pibBinaryColumnDelimiter )
                {
                    Assert( 0 == *pibBinaryColumnDelimiter );
                    *pibBinaryColumnDelimiter = ULONG( pbSeg - rgbSeg );
                }

                if ( cbFieldRemaining == cbFLDBinaryChunk )
                {
                    if ( fNormalisedEntireColumn )
                        *pbSeg++ = cbFLDBinaryChunkNormalized-1;
                    else
                        *pbSeg++ = cbFLDBinaryChunkNormalized;
                }
                else
                {
                    memset( pbSeg, 0, cbFLDBinaryChunk - cbFieldRemaining );
                    pbSeg += ( cbFLDBinaryChunk - cbFieldRemaining );
                    *pbSeg++ = (BYTE)cbFieldRemaining;

                }

#ifdef DEBUG
                cbFieldRemaining = 0;
#endif
            }
            else
            {
                UtilMemCpy( pbSeg, pbNextChunk, cbFLDBinaryChunk );

                pbNextChunk += cbFLDBinaryChunk;
                pbSeg += cbFLDBinaryChunk;
                *pbSeg++ = cbFLDBinaryChunkNormalized;

                cbFieldRemaining -= cbFLDBinaryChunk;
            }

            cbSegRemaining -= cbFLDBinaryChunkNormalized;
        }


        if ( cbSeg >= 1 + cbFLDBinaryChunkNormalized )
        {
            Assert( pbSeg >= rgbSeg + 1 + cbFLDBinaryChunkNormalized );
            Assert( pbSeg[-1] > 0 );
            Assert( pbSeg[-1] <= cbFLDBinaryChunkNormalized );

            Assert( ( pbSeg - ( rgbSeg + 1 ) ) % cbFLDBinaryChunkNormalized == 0 );
        }
        else
        {
            Assert( pbSeg == rgbSeg + 1 );
        }

        Assert( cbSegRemaining >= 0 );
        Assert( cbSegRemaining < cbFLDBinaryChunkNormalized );
        if ( cbSegRemaining > 0 )
        {
            Assert( !fNormalisedEntireColumn );
            Assert( cbFieldRemaining > 0 );

            if ( cbFieldRemaining >= cbSegRemaining )
            {
                UtilMemCpy( pbSeg, pbNextChunk, cbSegRemaining );
            }
            else
            {
                if ( NULL != pibBinaryColumnDelimiter )
                {
                    Assert( 0 == *pibBinaryColumnDelimiter );
                    *pibBinaryColumnDelimiter = ULONG( pbSeg + cbFieldRemaining - rgbSeg );
                }

                UtilMemCpy( pbSeg, pbNextChunk, cbFieldRemaining );
                memset( pbSeg+cbFieldRemaining, 0, cbSegRemaining - cbFieldRemaining );
            }
        }
    }

    Assert( cbSeg > 0 );
    *pcbSeg = cbSeg;
}


INLINE VOID FLDNormalizeFixedSegment(
    const BYTE          *pbField,
    const ULONG         cbField,
    BYTE                *rgbSeg,
    INT                 *pcbSeg,
    const JET_COLTYP    coltyp,
    BOOL                fDotNetGuid,
    BOOL                fDataPassedFromUser )
{
    WORD                wSrc;
    DWORD               dwSrc;
    QWORD               qwSrc;

    rgbSeg[0] = bPrefixData;
    switch ( coltyp )
    {
        case JET_coltypBit:
            Assert( 1 == cbField );
            *pcbSeg = 2;

            rgbSeg[1] = BYTE( pbField[0] == 0 ? 0x00 : 0xff );
            break;

        case JET_coltypUnsignedByte:
            Assert( 1 == cbField );
            *pcbSeg = 2;

            rgbSeg[1] = pbField[0];
            break;

        case JET_coltypShort:
            Assert( 2 == cbField );
            *pcbSeg = 3;

            if ( fDataPassedFromUser )
            {
                wSrc = *(Unaligned< WORD >*)pbField;
            }
            else
            {
                wSrc = *(UnalignedLittleEndian< WORD >*)pbField;
            }

            *( (UnalignedBigEndian< WORD >*) &rgbSeg[ 1 ] ) = wFlipHighBit( wSrc );
            break;

        case JET_coltypUnsignedShort:
            Assert( 2 == cbField );
            *pcbSeg = 3;

            if ( fDataPassedFromUser )
            {
                wSrc = *(Unaligned< WORD >*)pbField;
            }
            else
            {
                wSrc = *(UnalignedLittleEndian< WORD >*)pbField;
            }
            *( (UnalignedBigEndian< WORD >*) &rgbSeg[ 1 ] ) = wSrc;
            break;

        case JET_coltypLong:
            Assert( 4 == cbField );
            *pcbSeg = 5;

            if ( fDataPassedFromUser )
            {
                dwSrc = *(Unaligned< DWORD >*)pbField;
            }
            else
            {
                dwSrc = *(UnalignedLittleEndian< DWORD >*)pbField;
            }

            *( (UnalignedBigEndian< DWORD >*) &rgbSeg[ 1 ] ) = dwFlipHighBit( dwSrc );
            break;

        case JET_coltypUnsignedLong:
            Assert( 4 == cbField );
            *pcbSeg = 5;

            if ( fDataPassedFromUser )
            {
                dwSrc = *(Unaligned< DWORD >*)pbField;
            }
            else
            {
                dwSrc = *(UnalignedLittleEndian< DWORD >*)pbField;
            }

            *( (UnalignedBigEndian< DWORD >*) &rgbSeg[ 1 ] ) = dwSrc;
            break;

        case JET_coltypLongLong:
        case JET_coltypCurrency:
            Assert( 8 == cbField );
            *pcbSeg = 9;

            if ( fDataPassedFromUser )
            {
                qwSrc = *(Unaligned< QWORD >*)pbField;
            }
            else
            {
                qwSrc = *(UnalignedLittleEndian< QWORD >*)pbField;
            }

            *( (UnalignedBigEndian< QWORD >*) &rgbSeg[ 1 ] ) = qwFlipHighBit( qwSrc );
            break;

        case JET_coltypUnsignedLongLong:
            Assert( 8 == cbField );
            *pcbSeg = 9;

            if ( fDataPassedFromUser )
            {
                qwSrc = *(Unaligned< QWORD >*)pbField;
            }
            else
            {
                qwSrc = *(UnalignedLittleEndian< QWORD >*)pbField;
            }

            *( (UnalignedBigEndian< QWORD >*) &rgbSeg[ 1 ] ) = qwSrc;
            break;

        case JET_coltypIEEESingle:
            Assert( 4 == cbField );
            *pcbSeg = 5;

            if ( fDataPassedFromUser )
            {
                dwSrc = *(Unaligned< DWORD >*)pbField;
            }
            else
            {
                dwSrc = *(UnalignedLittleEndian< DWORD >*)pbField;
            }

            if ( dwSrc & maskDWordHighBit )
            {
                *( (UnalignedBigEndian< DWORD >*) &rgbSeg[ 1 ] ) = ~dwSrc;
            }
            else
            {
                *( (UnalignedBigEndian< DWORD >*) &rgbSeg[ 1 ] ) = dwFlipHighBit( dwSrc );
            }
            break;

        case JET_coltypIEEEDouble:
        case JET_coltypDateTime:
            Assert( 8 == cbField );
            *pcbSeg = 9;

            if ( fDataPassedFromUser )
            {
                qwSrc = *(Unaligned< QWORD >*)pbField;
            }
            else
            {
                qwSrc = *(UnalignedLittleEndian< QWORD >*)pbField;
            }

            if ( qwSrc & maskQWordHighBit )
            {
                *( (UnalignedBigEndian< QWORD >*) &rgbSeg[ 1 ] ) = ~qwSrc;
            }
            else
            {
                *( (UnalignedBigEndian< QWORD >*) &rgbSeg[ 1 ] ) = qwFlipHighBit( qwSrc );
            }
            break;
        case JET_coltypGUID:
            if ( fDotNetGuid )
            {
                Assert( 16 == cbField );
                *pcbSeg = 17;
                rgbSeg[1] = pbField[3];
                rgbSeg[2] = pbField[2];
                rgbSeg[3] = pbField[1];
                rgbSeg[4] = pbField[0];
                rgbSeg[5] = pbField[5];
                rgbSeg[6] = pbField[4];
                rgbSeg[7] = pbField[7];
                rgbSeg[8] = pbField[6];
                rgbSeg[9] = pbField[8];
                rgbSeg[10] = pbField[9];
                rgbSeg[11] = pbField[10];
                rgbSeg[12] = pbField[11];
                rgbSeg[13] = pbField[12];
                rgbSeg[14] = pbField[13];
                rgbSeg[15] = pbField[14];
                rgbSeg[16] = pbField[15];
            }
            else
            {
                Assert( 16 == cbField );
                *pcbSeg = 17;
                rgbSeg[1] = pbField[10];
                rgbSeg[2] = pbField[11];
                rgbSeg[3] = pbField[12];
                rgbSeg[4] = pbField[13];
                rgbSeg[5] = pbField[14];
                rgbSeg[6] = pbField[15];
                rgbSeg[7] = pbField[8];
                rgbSeg[8] = pbField[9];
                rgbSeg[9] = pbField[6];
                rgbSeg[10] = pbField[7];
                rgbSeg[11] = pbField[4];
                rgbSeg[12] = pbField[5];
                rgbSeg[13] = pbField[0];
                rgbSeg[14] = pbField[1];
                rgbSeg[15] = pbField[2];
                rgbSeg[16] = pbField[3];
            }
            break;

        default:
            Assert( !FRECTextColumn( coltyp ) );
            Assert( !FRECBinaryColumn( coltyp ) );
            Assert( fFalse );
            break;
    }
}


INLINE VOID FLDNormalizeNullSegment(
    BYTE                *rgbSeg,
    const JET_COLTYP    coltyp,
    const BOOL          fZeroLength,
    const BOOL          fSortNullsHigh )
{
    const BYTE          bPrefixNullT    = ( fSortNullsHigh ? bPrefixNullHigh : bPrefixNull );

    switch ( coltyp )
    {
        case JET_coltypBit:
        case JET_coltypUnsignedByte:
        case JET_coltypShort:
        case JET_coltypUnsignedShort:
        case JET_coltypLong:
        case JET_coltypUnsignedLong:
        case JET_coltypLongLong:
        case JET_coltypUnsignedLongLong:
        case JET_coltypCurrency:
        case JET_coltypIEEESingle:
        case JET_coltypIEEEDouble:
        case JET_coltypDateTime:
        case JET_coltypGUID:
            rgbSeg[0] = bPrefixNullT;
            break;

        default:
            Assert( FRECTextColumn( coltyp ) || FRECBinaryColumn( coltyp ) );
            rgbSeg[0] = ( fZeroLength ? bPrefixZeroLength : bPrefixNullT );
            break;
    }
}


ERR ErrFLDNormalizeTaggedData(
    _In_ const FIELD *          pfield,
    _In_ const DATA&            dataRaw,
    _Out_ DATA&                 dataNorm,
    _In_ const NORM_LOCALE_VER* const pnlv,
    _In_ const BOOL             fDotNetGuid,
    _Out_ BOOL *                pfDataTruncated)
{
    ERR                         err         = JET_errSuccess;

    *pfDataTruncated = fFalse;

    if ( 0 == dataRaw.Cb() )
    {
        FLDNormalizeNullSegment(
                (BYTE *)dataNorm.Pv(),
                pfield->coltyp,
                fTrue,
                fFalse );
        dataNorm.SetCb( 1 );
    }
    else
    {
        INT         cb      = 0;
        switch ( pfield->coltyp )
        {
            case JET_coltypText:
            case JET_coltypLongText:
                CallR( ErrFLDNormalizeTextSegment(
                            (BYTE *)dataRaw.Pv(),
                            dataRaw.Cb(),
                            (BYTE *)dataNorm.Pv(),
                            &cb,
                            JET_cbKeyMost_OLD,
                            JET_cbKeyMost_OLD,
                            JET_cbKeyMost_OLD+1,
                            pfield->cp,
                            pnlv ) );
                dataNorm.SetCb( cb );
                if ( wrnFLDKeyTooBig == err )
                {
                    *pfDataTruncated = fTrue;
                    err = JET_errSuccess;
                }
                CallS( err );
                Assert( dataNorm.Cb() > 0 );
                break;

            case JET_coltypBinary:
            case JET_coltypLongBinary:
                FLDNormalizeBinarySegment(
                        (BYTE *)dataRaw.Pv(),
                        dataRaw.Cb(),
                        (BYTE *)dataNorm.Pv(),
                        &cb,
                        JET_cbKeyMost_OLD,
                        JET_cbKeyMost_OLD,
                        JET_cbKeyMost_OLD+1,
                        fFalse,
                        pfDataTruncated,
                        NULL );
                dataNorm.SetCb( cb );
                break;

            default:
                FLDNormalizeFixedSegment(
                        (BYTE *)dataRaw.Pv(),
                        dataRaw.Cb(),
                        (BYTE *)dataNorm.Pv(),
                        &cb,
                        pfield->coltyp,
                        fDotNetGuid );
                dataNorm.SetCb( cb );
                break;
        }
    }

    return err;
}


LOCAL ERR ErrRECIIRetrieveKey(
    FUCB                *pfucb,
    const IDB           * const pidb,
    DATA&               lineRec,
    KEY                 *pkey,
    const ULONG         *rgitag,
    const ULONG         ichOffset,
    const BOOL          fRetrieveLVBeforeImage,
    RCE                 *prce,
    ULONG               *piidxseg,
    ULONG               cbDataMost,
    BOOL                *pfNeedMoreData );

ERR ErrRECIRetrieveKey(
    FUCB                *pfucb,
    const IDB           * const pidb,
    DATA&               lineRec,
    KEY                 *pkey,
    const ULONG         *rgitag,
    const ULONG         ichOffset,
    const BOOL          fRetrieveLVBeforeImage,
    RCE                 *prce,
    ULONG               *piidxseg )
{
    ERR     err             = JET_errSuccess;
    ULONG   cbKeyMost       = 0;
    ULONG   cbDataMost      = 0;
    BOOL    fNeedMoreData   = fFalse;

    cbKeyMost   = pidb->CbKeyMost();

    cbDataMost  = cbKeyMost;

    while ( ( err = ErrRECIIRetrieveKey(    pfucb,
                                            pidb,
                                            lineRec,
                                            pkey,
                                            rgitag,
                                            ichOffset,
                                            fRetrieveLVBeforeImage,
                                            prce,
                                            piidxseg,
                                            cbDataMost,
                                            &fNeedMoreData ) ) >= JET_errSuccess &&
            fNeedMoreData )
    {
        cbDataMost *= 2;
    }

    return err;
}

LOCAL ERR ErrRECIIRetrieveKey(
    FUCB                *pfucb,
    const IDB           * const pidb,
    DATA&               lineRec,
    KEY                 *pkey,
    const ULONG         *rgitag,
    const ULONG         ichOffset,
    const BOOL          fRetrieveLVBeforeImage,
    RCE                 *prce,
    ULONG               *piidxseg,
    ULONG               cbDataMost,
    BOOL                *pfNeedMoreData )
{
    ERR                 err                         = JET_errSuccess;
    FCB                 * const pfcbTable           = pfucb->u.pfcb;
    FCB                 * pfcb                      = pfcbTable;
    BOOL                fAllNulls                   = fTrue;
    BOOL                fNullFirstSeg               = fFalse;
    BOOL                fNullSeg                    = fFalse;
    BOOL                fColumnTruncated            = fFalse;
    BOOL                fKeyTruncated               = fFalse;
    const BOOL          fPageInitiallyLatched       = Pcsr( pfucb )->FLatched();
    const BOOL          fTupleIndex                 = pidb->FTuples();
    const BOOL          fNestedTable                = pidb->FNestedTable();
    const BOOL          fDotNetGuid                 = pidb->FDotNetGuid();
    BOOL                fAllMVOutOfValues           = fTrue;
    BOOL                fPageLatched                = fPageInitiallyLatched;

    BYTE                *pbSegCur;
    ULONG               cbKeyAvail;
    const IDXSEG        *pidxseg;
    const IDXSEG        *pidxsegMac;
    ULONG               iidxsegT                    = 0;
    IDXSEG              rgidxseg[JET_ccolKeyMost];
    IDXSEG              rgidxsegConditional[JET_ccolKeyMost];
    const BOOL          fOnRecord                   = ( lineRec == pfucb->kdfCurr.data );
    BOOL                fTransactionStarted         = fFalse;
    const BOOL          fRetrieveBasedOnRCE         = ( prceNil != prce );
    const size_t        cbLVStack                   = 256;
    BYTE                rgbLVStack[ cbLVStack ];
    BYTE                *pbAllocated                    = NULL;
    BYTE                *pbLV                       = NULL;
    const size_t        cbSegStack                  = 256;
    BYTE                rgbSegStack[ cbSegStack ];
    BYTE                *pbSegRes                   = NULL;
    BYTE                *pbSeg                      = NULL;
    const BOOL          fDisallowTruncation         = pidb->FDisallowTruncation();

    Assert( pkey != NULL );
    Assert( !pkey->FNull() );
    Assert( pfcb->Ptdb() != ptdbNil );
    Assert( pidb != pidbNil );
    Assert( pidb->Cidxseg() > 0 );
    Assert( pfNeedMoreData );

    *pfNeedMoreData = fFalse;

    if ( fPageInitiallyLatched )
    {
        Assert( fOnRecord );
        Assert( locOnCurBM == pfucb->locLogical );
        Assert( !fRetrieveLVBeforeImage );
        Assert( !fRetrieveBasedOnRCE );
        Assert( pfucb->ppib->Level() > 0 );
    }

    const ULONG cbKeyMost       = pidb->CbKeyMost();

    Assert( pidb->CbVarSegMac() > 0 );
    Assert( pidb->CbVarSegMac() <= cbKeyMost );

    const ULONG cbVarSegMac     = ( cbKeyMost == pidb->CbVarSegMac() ?
                                        cbKeyMost+1 :
                                        pidb->CbVarSegMac() );
    Assert( cbVarSegMac > 0 );
    Assert( cbVarSegMac < cbKeyMost || cbVarSegMac == cbKeyMost+1 );

    const BOOL  fSortNullsHigh  = pidb->FSortNullsHigh();

    if ( cbDataMost > cbLVStack )
    {
        Alloc( pbAllocated = new BYTE[cbDataMost] );
        pbLV = pbAllocated;
    }
    else
    {
        pbLV = rgbLVStack;
    }
    if ( cbKeyMost > cbSegStack )
    {
        Alloc( pbSegRes = (BYTE *)RESKEY.PvRESAlloc() );
        pbSeg = pbSegRes;
    }
    else
    {
        pbSeg = rgbSegStack;
    }

    Assert( pkey->prefix.FNull() );
    pbSegCur = (BYTE *)pkey->suffix.Pv();
    cbKeyAvail = cbKeyMost;

    Assert( fRetrieveBasedOnRCE
        || !fRetrieveLVBeforeImage
        || FFUCBReplacePrepared( pfucb ) );

    if ( pidb->FTemplateIndex() )
    {
        Assert( pfcb->FDerivedTable() || pfcb->FTemplateTable() );
        if ( pfcb->FDerivedTable() )
        {
            pfcb = pfcb->Ptdb()->PfcbTemplateTable();
            Assert( pfcbNil != pfcb );
            Assert( pfcb->FTemplateTable() );
        }
        else
        {
            Assert( pfcb->Ptdb()->PfcbTemplateTable() == pfcbNil );
        }
    }

    pfcb->EnterDML();
    UtilMemCpy( rgidxseg, PidxsegIDBGetIdxSeg( pidb, pfcb->Ptdb() ), pidb->Cidxseg() * sizeof(IDXSEG) );
    UtilMemCpy( rgidxsegConditional, PidxsegIDBGetIdxSegConditional( pidb, pfcb->Ptdb() ), pidb->CidxsegConditional() * sizeof(IDXSEG) );
    pfcb->LeaveDML();

    if ( fOnRecord && 0 == pfucb->ppib->Level() )
    {
        Assert( !fPageInitiallyLatched );
        Assert( !Pcsr( pfucb )->FLatched() );

        Call( ErrDIRBeginTransaction( pfucb->ppib, 57637, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }

    pidxseg = rgidxsegConditional;
    pidxsegMac = pidxseg + pidb->CidxsegConditional();
    for ( ; pidxseg < pidxsegMac; pidxseg++ )
    {
        const COLUMNID  columnidConditional     = pidxseg->Columnid();
        const BOOL      fMustBeNull             = pidxseg->FMustBeNull();
        BOOL            fColumnIsNull           = fFalse;
        DATA            dataField;

        if ( fOnRecord && !fPageLatched )
        {
            Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );
            Call( ErrDIRGet( pfucb ) );
            fPageLatched = fTrue;
        }

#ifdef DEBUG
        if ( fOnRecord )
        {
            Assert( Pcsr( pfucb )->FLatched() );
        }
        else
        {
            Assert( !Pcsr( pfucb )->FLatched() );
        }
#endif

        Assert( !lineRec.FNull() );
        if ( FCOLUMNIDTagged( columnidConditional ) )
        {
            err = ErrRECRetrieveTaggedColumn(
                        pfcbTable,
                        columnidConditional,
                        1,
                        lineRec,
                        &dataField );


            if( wrnRECUserDefinedDefault == err )
            {

                if ( fPageLatched )
                {
                    Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );
                    CallS( ErrDIRRelease( pfucb ) );
                    fPageLatched = fFalse;
                }

                DATA    dataSaved   = pfucb->dataWorkBuf;

                const BOOL fAlwaysRetrieveCopy  = FFUCBAlwaysRetrieveCopy( pfucb );
                const BOOL fNeverRetrieveCopy   = FFUCBNeverRetrieveCopy( pfucb );

                Assert( !( fAlwaysRetrieveCopy && fNeverRetrieveCopy ) );

                if( fOnRecord )
                {
                    FUCBSetNeverRetrieveCopy( pfucb );
                }
                else
                {
                    pfucb->dataWorkBuf = lineRec;
                    FUCBSetAlwaysRetrieveCopy( pfucb );
                }

#ifdef SYNC_DEADLOCK_DETECTION
                COwner * pownerSaved;
                UtilAssertCriticalSectionCanDoIO();
                pownerSaved = Pcls()->pownerLockHead;
#endif

                ULONG   cbActual    = 0;
                err = ErrRECCallback(
                    pfucb->ppib,
                    pfucb,
                    JET_cbtypUserDefinedDefaultValue,
                    columnidConditional,
                    NULL,
                    &cbActual,
                    columnidConditional );

#ifdef SYNC_DEADLOCK_DETECTION
                Assert( Pcls()->pownerLockHead == pownerSaved );
#endif

                pfucb->dataWorkBuf = dataSaved;
                FUCBResetAlwaysRetrieveCopy( pfucb );
                FUCBResetNeverRetrieveCopy( pfucb );

                if( fAlwaysRetrieveCopy )
                {
                    FUCBSetAlwaysRetrieveCopy( pfucb );
                }
                else if ( fNeverRetrieveCopy )
                {
                    FUCBSetNeverRetrieveCopy( pfucb );
                }

                Call( err );
            }
        }
        else
        {
            err = ErrRECRetrieveNonTaggedColumn(
                        pfcbTable,
                        columnidConditional,
                        lineRec,
                        &dataField,
                        pfieldNil );
        }
        Assert( err >= 0 );

        fColumnIsNull = ( JET_wrnColumnNull == err );

        if( fMustBeNull && !fColumnIsNull )
        {
            err = ErrERRCheck( wrnFLDNotPresentInIndex );
            goto HandleError;
        }
        else if( !fMustBeNull && fColumnIsNull )
        {
            err = ErrERRCheck( wrnFLDNotPresentInIndex );
            goto HandleError;
        }
    }

    pidxseg = rgidxseg;
    pidxsegMac = pidxseg + pidb->Cidxseg();
    for ( iidxsegT = 0; pidxseg < pidxsegMac; pidxseg++, iidxsegT++ )
    {
        FIELD           field;
        BYTE            *pbField = 0;
        ULONG           cbField = 0xffffffff;
        DATA            dataField;
        INT             cbSeg       = 0;
        const COLUMNID  columnid    = pidxseg->Columnid();
        const BOOL      fDescending = pidxseg->FDescending();
        const BOOL      fFixedField = FCOLUMNIDFixed( columnid );

        pfcb->EnterDML();
        field = *( pfcb->Ptdb()->Pfield( columnid ) );
        pfcb->LeaveDML();

        Assert( !FFIELDEncrypted( field.ffield ) );


        ULONG       ibTupleOffset       = 0;
        BOOL        fTupleAdjusted      = fFalse;

        if ( fTupleIndex && ( pidxseg == rgidxseg ) )
        {
            Assert( FRECTextColumn( field.coltyp ) || FRECBinaryColumn( field.coltyp) );


            ibTupleOffset       = ( usUniCodePage == field.cp ? ichOffset * 2 : ichOffset );
        }

        Assert( !FFIELDDeleted( field.ffield ) );
        Assert( JET_coltypNil != field.coltyp );
        Assert( !FCOLUMNIDVar( columnid ) || field.coltyp == JET_coltypBinary || field.coltyp == JET_coltypText );
        Assert( !FFIELDMultivalued( field.ffield ) || FCOLUMNIDTagged( columnid ) );

        if ( fOnRecord && !fPageLatched )
        {
            Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );
            Call( ErrDIRGet( pfucb ) );
            fPageLatched = fTrue;
        }

        if ( fOnRecord )
        {
            Assert( Pcsr( pfucb )->FLatched() );
        }
        else
        {
            Assert( !Pcsr( pfucb )->FLatched() );
        }

        Assert( !lineRec.FNull() );
        if ( FCOLUMNIDTagged( columnid ) )
        {
            err = ErrRECRetrieveTaggedColumn(
                        pfcbTable,
                        columnid,
                        rgitag[iidxsegT],
                        lineRec,
                        &dataField );

            fAllMVOutOfValues = ( fAllMVOutOfValues && ( rgitag[iidxsegT] > 1 && ( JET_wrnColumnNull == err ) ) );
        }
        else
        {
            Assert( 1 == rgitag[iidxsegT] );
            err = ErrRECRetrieveNonTaggedColumn(
                        pfcbTable,
                        columnid,
                        lineRec,
                        &dataField,
                        &field );
        }
        Assert( err >= 0 );

        if ( cbKeyAvail == 0 )
        {
            fKeyTruncated = fTrue;
            if ( fDisallowTruncation )
            {
                Call( ErrERRCheck( JET_errKeyTruncated ) );
            }

            if ( JET_wrnColumnNull == err )
            {
                Assert( rgitag[iidxsegT] >= 1 );
                if ( rgitag[iidxsegT] > 1 && !fNestedTable )
                {
                    Assert( FFIELDMultivalued( field.ffield ) );
                    err = ErrERRCheck( wrnFLDOutOfKeys );
                    goto HandleError;
                }
                else
                {
                    if ( pidxseg == rgidxseg )
                        fNullFirstSeg = fTrue;
                    fNullSeg = fTrue;
                }
            }

            Assert( JET_errSuccess == err
                || wrnRECSeparatedLV == err
                || wrnRECCompressed == err
                || wrnRECIntrinsicLV == err
                || wrnRECUserDefinedDefault == err
                || JET_wrnColumnNull == err );
            err = JET_errSuccess;

            continue;
        }

        Assert( cbKeyAvail > 0 );
        if ( wrnRECUserDefinedDefault == err )
        {
            if ( fPageLatched )
            {
                Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );
                CallS( ErrDIRRelease( pfucb ) );
                fPageLatched = fFalse;
            }

            DATA dataSaved = pfucb->dataWorkBuf;

            const BOOL fAlwaysRetrieveCopy  = FFUCBAlwaysRetrieveCopy( pfucb );
            const BOOL fNeverRetrieveCopy   = FFUCBNeverRetrieveCopy( pfucb );

            Assert( !( fAlwaysRetrieveCopy && fNeverRetrieveCopy ) );

            if( fOnRecord )
            {
                FUCBSetNeverRetrieveCopy( pfucb );
            }
            else
            {
                pfucb->dataWorkBuf = lineRec;
                FUCBSetAlwaysRetrieveCopy( pfucb );
            }

#ifdef SYNC_DEADLOCK_DETECTION
            COwner * pownerSaved;
            UtilAssertCriticalSectionCanDoIO();
            pownerSaved = Pcls()->pownerLockHead;
#endif

            ULONG cbActual;
            cbActual = cbDataMost;
            err = ErrRECCallback(
                pfucb->ppib,
                pfucb,
                JET_cbtypUserDefinedDefaultValue,
                columnid,
                pbLV,
                &cbActual,
                columnid );

#ifdef SYNC_DEADLOCK_DETECTION
            Assert( Pcls()->pownerLockHead == pownerSaved );
#endif

            pfucb->dataWorkBuf = dataSaved;
            FUCBResetAlwaysRetrieveCopy( pfucb );
            FUCBResetNeverRetrieveCopy( pfucb );

            if( fAlwaysRetrieveCopy )
            {
                FUCBSetAlwaysRetrieveCopy( pfucb );
            }
            else if ( fNeverRetrieveCopy )
            {
                FUCBSetNeverRetrieveCopy( pfucb );
            }

            Call( err );

            dataField.SetPv( pbLV );
            dataField.SetCb( min( cbActual, cbDataMost ) );

            if ( JET_wrnBufferTruncated == err )
                err = JET_errSuccess;
        }
        else if ( wrnRECCompressed == err )
        {
            Assert( FCOLUMNIDTagged( columnid ) );
            Assert( dataField.Cb() > 0 );

            if( ibTupleOffset > 0)
            {
                INT cbActual = 0;
                BYTE * pbT = NULL;
                Call( ErrPKAllocAndDecompressData(
                        dataField,
                        pfucb,
                        &pbT,
                        &cbActual ) );
                INT cbToCopy;
                if( ibTupleOffset > (ULONG)cbActual )
                {
                    cbToCopy = 0;
                }
                else
                {
                    cbToCopy = min( cbDataMost, ( cbActual - ibTupleOffset ) );
                }
                UtilMemCpy( pbLV, pbT+ibTupleOffset, cbToCopy );
                delete [] pbT;

                dataField.SetPv( pbLV );
                dataField.SetCb( cbToCopy );

                if( fTupleIndex && ( pidxseg == rgidxseg ) )
                {
                    fTupleAdjusted = fTrue;
                }
            }
            else
            {
                INT cbActual = 0;
                Call( ErrPKDecompressData(
                        dataField,
                        pfucb,
                        pbLV,
                        cbDataMost,
                        &cbActual ) );

                dataField.SetPv( pbLV );
                dataField.SetCb( min( (ULONG)cbActual, cbDataMost ) );
            }

            if ( JET_wrnBufferTruncated == err )
                err = JET_errSuccess;
        }
        else if ( wrnRECSeparatedLV == err )
        {
            const LvId  lid             = LidOfSeparatedLV( dataField );
            const BOOL  fAfterImage     = !fRetrieveLVBeforeImage;
            ULONG       cbActual;

            if ( fRetrieveBasedOnRCE )
            {
                Assert( !fOnRecord );
                Assert( !fPageLatched );
                Assert( !fPageInitiallyLatched );
                Assert( !Pcsr( pfucb )->FLatched() );

                Assert( prceNil != prce );
                Assert( prce->TrxCommitted() != trxMax
                        || ppibNil != prce->Pfucb()->ppib );

                Call( ErrRECRetrieveSLongField(
                            pfucb,
                            lid,
                            fAfterImage,
                            ibTupleOffset,
                            fFalse,
                            pbLV,
                            cbDataMost,
                            &cbActual,
                            NULL,
                            NULL,
                            prce ) );

                Assert( !Pcsr( pfucb )->FLatched() );

                dataField.SetPv( pbLV );
                dataField.SetCb( cbActual );
            }

            else
            {
                Assert( prceNil == prce );

                if ( fPageLatched )
                {
                    Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );
                    CallS( ErrDIRRelease( pfucb ) );
                    fPageLatched = fFalse;
                }

                Call( ErrRECRetrieveSLongField(
                            pfucb,
                            lid,
                            fAfterImage,
                            ibTupleOffset,
                            fFalse,
                            pbLV,
                            cbDataMost,
                            &cbActual,
                            NULL,
                            NULL ) );

                Assert( !Pcsr( pfucb )->FLatched() );

                dataField.SetPv( pbLV );
                dataField.SetCb( cbActual );
            }

            if( fTupleIndex && ( pidxseg == rgidxseg ) )
            {
                fTupleAdjusted = fTrue;
            }

            if ( (ULONG)dataField.Cb() > cbDataMost )
            {
                dataField.SetCb( cbDataMost );
            }

            Assert( JET_wrnColumnNull != err );
            err = JET_errSuccess;
        }
        else if ( wrnRECIntrinsicLV == err )
        {
            if ( fTupleIndex && ( pidxseg == rgidxseg ) )
            {
                const INT cbDelta = min( dataField.Cb(), (INT)ibTupleOffset );
                dataField.DeltaPv( cbDelta );
                dataField.DeltaCb( -cbDelta );

                fTupleAdjusted = fTrue;
            }

            if ( (ULONG)dataField.Cb() > cbDataMost )
            {
                dataField.SetCb( cbDataMost );
            }

            err = JET_errSuccess;
        }
        else
        {
            CallSx( err, JET_wrnColumnNull );

            if ( (ULONG)dataField.Cb() > cbDataMost )
            {
                dataField.SetCb( cbDataMost );
            }
        }

        if ( rgitag[iidxsegT] > 1 && JET_wrnColumnNull == err && !fNestedTable )
        {
            Assert( FFIELDMultivalued( field.ffield ) );
            err = ErrERRCheck( wrnFLDOutOfKeys );
            goto HandleError;
        }

        CallSx( err, JET_wrnColumnNull );
        Assert( (ULONG)dataField.Cb() <= cbDataMost );

        if ( fTupleIndex && ( pidxseg == rgidxseg ) )
        {

            const BYTE *    pbFieldT    = (BYTE *)dataField.Pv();
            const ULONG     cbFieldT    = dataField.Cb();
            const ULONG     cbFieldMax  = ( usUniCodePage == field.cp ? pidb->CchTuplesLengthMax() * 2 : pidb->CchTuplesLengthMax() );
            const ULONG     cbFieldMin  = ( usUniCodePage == field.cp ? pidb->CchTuplesLengthMin() * 2 : pidb->CchTuplesLengthMin() );

            if ( JET_wrnColumnNull == err )
            {

                dataField.SetPv( NULL );
                dataField.SetCb( 0 );
            }
            else if( fTupleAdjusted )
            {
            }
            else if( pbFieldT + ibTupleOffset <= pbFieldT + cbFieldT )
            {

                dataField.DeltaPv( ibTupleOffset );
                dataField.DeltaCb( 0 - ibTupleOffset );
            }
            else
            {

                dataField.SetPv( NULL );
                dataField.SetCb( 0 );
            }

            dataField.SetCb( min( dataField.Cb(), (INT)cbFieldMax ) );
            if( dataField.Cb() < (INT)cbFieldMin )
            {
                err = ErrERRCheck( wrnFLDOutOfTuples );
                goto HandleError;
            }
        }

        cbField = dataField.Cb();
        pbField = (BYTE *)dataField.Pv();

        Assert( cbKeyAvail > 0 );
        if ( JET_wrnColumnNull == err || pbField == NULL || cbField == 0 )
        {
            if ( JET_wrnColumnNull == err )
            {
                if ( pidxseg == rgidxseg )
                    fNullFirstSeg = fTrue;
                fNullSeg = fTrue;
            }
            else
            {
                Assert( !fFixedField );
                Assert( FRECTextColumn( field.coltyp ) || FRECBinaryColumn( field.coltyp ) );
                fAllNulls = fFalse;
            }

            FLDNormalizeNullSegment(
                    pbSeg,
                    field.coltyp,
                    JET_wrnColumnNull != err,
                    fSortNullsHigh );
            cbSeg = 1;
        }

        else
        {
            fAllNulls = fFalse;

            switch ( field.coltyp )
            {
                case JET_coltypText:
                case JET_coltypLongText:

                    Call( ErrFLDNormalizeTextSegment(
                                pbField,
                                cbField,
                                pbSeg,
                                &cbSeg,
                                cbKeyAvail,
                                cbKeyMost,
                                cbVarSegMac,
                                field.cp,
                                pidb->PnlvGetNormalizationStruct() ) );
                    Assert( JET_errSuccess == err || wrnFLDKeyTooBig == err );
                    if ( wrnFLDKeyTooBig == err )
                    {
                        fColumnTruncated = fTrue;
                        err = JET_errSuccess;
                    }
                    Assert( cbSeg > 0 );
                    break;

                case JET_coltypBinary:
                case JET_coltypLongBinary:
                    FLDNormalizeBinarySegment(
                            pbField,
                            cbField,
                            pbSeg,
                            &cbSeg,
                            cbKeyAvail,
                            cbKeyMost,
                            cbVarSegMac,
                            fFixedField,
                            &fColumnTruncated,
                            NULL );
                    break;

                default:
                    FLDNormalizeFixedSegment(
                            pbField,
                            cbField,
                            pbSeg,
                            &cbSeg,
                            field.coltyp,
                            fDotNetGuid );
                    break;
            }
        }

        if (    cbField == cbDataMost &&
                (ULONG) cbSeg < cbKeyMost &&
                (ULONG) cbSeg < cbKeyAvail &&
                (ULONG) cbSeg < pidb->CbVarSegMac() &&
                cbKeyMost != JET_cbKeyMost_OLD )
        {
            *pfNeedMoreData = fTrue;
        }

        if ( !fKeyTruncated )
        {
            if ( fColumnTruncated )
            {
                fKeyTruncated = fTrue;
                if ( fDisallowTruncation )
                {
                    Call( ErrERRCheck( JET_errKeyTruncated ) );
                }

                Assert( FRECTextColumn( field.coltyp ) || FRECBinaryColumn( field.coltyp ) );

                if ( (ULONG)cbSeg != cbKeyAvail )
                {
                    Assert( (ULONG)cbSeg < cbKeyAvail );
                    Assert( !fFixedField );
                    Assert( FRECBinaryColumn( field.coltyp ) );
                    Assert( cbKeyAvail - cbSeg < cbFLDBinaryChunkNormalized );
                }
            }
            else if ( (ULONG)cbSeg > cbKeyAvail )
            {
                fKeyTruncated = fTrue;
                if ( fDisallowTruncation )
                {
                    Call( ErrERRCheck( JET_errKeyTruncated ) );
                }

                cbSeg = cbKeyAvail;
            }

            if ( fDescending && cbSeg > 0 )
            {
                BYTE *pb;

                for ( pb = pbSeg + cbSeg - 1; pb >= (BYTE *)pbSeg; pb-- )
                    *pb ^= 0xff;
            }

            Assert( cbKeyAvail >= (ULONG)cbSeg );
            UtilMemCpy( pbSegCur, pbSeg, cbSeg );
            pbSegCur += cbSeg;
            cbKeyAvail -= cbSeg;
        }

    }

    if ( fNestedTable && fAllMVOutOfValues )
    {
        iidxsegT = 0;
        err = ErrERRCheck( wrnFLDOutOfKeys );
        goto HandleError;
    }
    
    Assert( pkey->prefix.FNull() );
    pkey->suffix.SetCb( pbSegCur - (BYTE *) pkey->suffix.Pv() );
    if ( fAllNulls )
        err = ErrERRCheck( wrnFLDNullKey );
    else if ( fNullFirstSeg )
        err = ErrERRCheck( wrnFLDNullFirstSeg );
    else if ( fNullSeg )
        err = ErrERRCheck( wrnFLDNullSeg );

#ifdef DEBUG
    switch ( err )
    {
        case wrnFLDNullKey:
        case wrnFLDNullFirstSeg:
        case wrnFLDNullSeg:
            break;
        default:
            CallS( err );
    }
#endif

HandleError:
    if ( wrnFLDOutOfKeys == err )
    {
        *piidxseg = iidxsegT;
    }
    else
    {
        *piidxseg = iidxsegNoRollover;
    }

    if ( fPageLatched )
    {
        if ( !fPageInitiallyLatched )
        {
            CallS( ErrDIRRelease( pfucb ) );
        }
    }
    else if ( fPageInitiallyLatched && err >= 0 )
    {
        const ERR   errT    = ErrBTGet( pfucb );
        if ( errT < 0 )
            err = errT;
    }
    Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );

    if ( fTransactionStarted )
    {
        Assert( fOnRecord );
        Assert( !fPageInitiallyLatched );
        CallS( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );
    }

    RESKEY.Free( pbSegRes );
    delete [] pbAllocated;

    return err;
}


INLINE VOID FLDISetFullColumnLimit(
    DATA        * const plineNorm,
    const ULONG cbAvailWithSuffix,
    const BOOL  fNeedSentinel )
{
    if ( fNeedSentinel )
    {
        Assert( cbAvailWithSuffix > 0 );
        Assert( cbAvailWithSuffix <= cbLimitKeyMostMost );
        Assert( (ULONG)plineNorm->Cb() < cbAvailWithSuffix );

        BYTE    * const pbNorm              = (BYTE *)plineNorm->Pv() + plineNorm->Cb();
        const   ULONG   cbSentinelPadding   = cbAvailWithSuffix - plineNorm->Cb();

        memset( pbNorm, bSentinel, cbSentinelPadding );
        plineNorm->DeltaCb( cbSentinelPadding );
    }
}

INLINE VOID FLDISetPartialColumnLimitOnTextColumn(
    DATA            *plineNorm,
    const ULONG     cbAvailWithSuffix,
    const BOOL      fDescending,
    const BOOL      fNeedSentinel,
    const USHORT    cp )
{
    ULONG           ibT;
    const ULONG     ibSuffix            = cbAvailWithSuffix - 1;
    BYTE            * const pbNorm      = (BYTE *)plineNorm->Pv();

    Assert( plineNorm->Cb() > 0 );
    Assert( cbAvailWithSuffix > 0 );
    Assert( cbAvailWithSuffix <= cbLimitKeyMostMost );
    Assert( (ULONG)plineNorm->Cb() < cbAvailWithSuffix );

    if ( plineNorm->Cb() < 2 )
    {
        FLDISetFullColumnLimit( plineNorm, cbAvailWithSuffix, fNeedSentinel );
        return;
    }

    if ( usUniCodePage == cp )
    {
        const BYTE  bUnicodeDelimiter   = BYTE( bUnicodeSortKeySentinel ^ ( fDescending ? 0xff : 0x00 ) );

        for ( ibT = 1;
              pbNorm[ibT] != bUnicodeDelimiter && ibT < ibSuffix;
              ibT++ )
            ;
    }
    else
    {
        const BYTE  bTerminator         = BYTE( bASCIISortKeyTerminator ^ ( fDescending ? 0xff : 0x00 ) );

        ibT = plineNorm->Cb();

        if ( bTerminator == pbNorm[ibT-1] )
        {
            ibT--;
            Assert( plineNorm->Cb() >= 1 );
        }
        else
        {
            Assert( (ULONG)plineNorm->Cb() == ibSuffix );
        }

        Assert( ibT <= ibSuffix );
    }

    ibT = min( ibT, ibSuffix );
    if ( fNeedSentinel )
    {
        memset(
            pbNorm + ibT,
            bSentinel,
            ibSuffix - ibT + 1 );
        plineNorm->SetCb( cbAvailWithSuffix );
    }
    else
    {
        plineNorm->SetCb( ibT );
    }
}

JETUNITTEST( NORM, PartialLimitTest )
{
    BYTE buf[12];
    DATA data;
    INT i;
    const BYTE sortkey[12] = { 0x7f, 0xce, 0x93, 0x16, 0x1, 0x1, 0x1, 0x1, 0x0 };

    memcpy( buf, sortkey, 9 );
    data.SetPv( buf );
    data.SetCb( 9 );
    FLDISetPartialColumnLimitOnTextColumn( &data, sizeof(buf), fFalse, fFalse, usUniCodePage );
    CHECK( data.Cb() == 4 );
    CHECK( memcmp( data.Pv(), sortkey, data.Cb() ) == 0 );

    memcpy( buf, sortkey, 9 );
    data.SetPv( buf );
    data.SetCb( 9 );
    FLDISetPartialColumnLimitOnTextColumn( &data, sizeof(buf), fFalse, fTrue, usUniCodePage );
    CHECK( data.Cb() == sizeof(buf) );
    CHECK( memcmp( data.Pv(), sortkey, 4 ) == 0 );
    for ( i=4; i<sizeof(buf); i++ )
    {
        CHECK( buf[i] == 0xff );
    }

    memcpy( buf, sortkey, 9 );
    for ( i=0; i<sizeof(buf); i++ )
    {
        buf[i] ^= 0xff;
    }
    data.SetPv( buf );
    data.SetCb( 9 );
    FLDISetPartialColumnLimitOnTextColumn( &data, sizeof(buf), fTrue, fFalse, usUniCodePage );
    CHECK( data.Cb() == 4 );
    for ( i=0; i<4; i++ )
    {
        CHECK( ( buf[i] ^ sortkey[i] ) == 0xff );
    }

    memcpy( buf, sortkey, 9 );
    for ( i=0; i<sizeof(buf); i++ )
    {
        buf[i] ^= 0xff;
    }
    data.SetPv( buf );
    data.SetCb( 9 );
    FLDISetPartialColumnLimitOnTextColumn( &data, sizeof(buf), fTrue, fTrue, usUniCodePage );
    CHECK( data.Cb() == sizeof(buf) );
    for ( i=0; i<4; i++ )
    {
        CHECK( ( buf[i] ^ sortkey[i] ) == 0xff );
    }
    for ( i=4; i<sizeof(buf); i++ )
    {
        CHECK( buf[i] == 0xff );
    }
}

INLINE VOID FLDITrySetPartialColumnLimitOnBinaryColumn(
    DATA            * const plineNorm,
    const ULONG     cbAvailWithSuffix,
    const ULONG     ibBinaryColumnDelimiter,
    const BOOL      fNeedSentinel )
{
    Assert( plineNorm->Cb() > 0 );
    Assert( cbAvailWithSuffix > 0 );
    Assert( cbAvailWithSuffix <= cbLimitKeyMostMost );
    Assert( (ULONG)plineNorm->Cb() < cbAvailWithSuffix );

#ifdef DEBUG
    if ( 0 != ibBinaryColumnDelimiter )
    {
        Assert( plineNorm->Cb() > 1 );
        Assert( ibBinaryColumnDelimiter > 1 );
        Assert( ibBinaryColumnDelimiter < (ULONG)plineNorm->Cb() );
    }
#endif

    if ( 0 == ibBinaryColumnDelimiter )
    {
        FLDISetFullColumnLimit( plineNorm, cbAvailWithSuffix, fNeedSentinel );
    }
    else if ( fNeedSentinel )
    {
        plineNorm->DeltaCb( 1 );
        memset(
            (BYTE *)plineNorm->Pv() + ibBinaryColumnDelimiter,
            bSentinel,
            plineNorm->Cb() - ibBinaryColumnDelimiter );
    }
    else
    {
        plineNorm->SetCb( ibBinaryColumnDelimiter );
    }
}


LOCAL ERR ErrFLDNormalizeSegment(
    FUCB                * const pfucb,
    IDB                 * const pidb,
    DATA                * const plineColumn,
    DATA                * const plineNorm,
    const JET_COLTYP    coltyp,
    const USHORT        cp,
    const ULONG         cbAvail,
    const BOOL          fDescending,
    const BOOL          fFixedField,
    const BOOL          fTupleField,
    const JET_GRBIT     grbit )
{
    INT                 cbColumn;
    BYTE                * pbColumn;
    BYTE                * const pbNorm              = (BYTE *)plineNorm->Pv();
    ULONG               ibBinaryColumnDelimiter     = 0;
    const BOOL          fDotNetGuid = pidb->FDotNetGuid();

    const ULONG cbKeyMost       = pidb->CbKeyMost();

    Assert( pidb->CbVarSegMac() > 0 );
    Assert( pidb->CbVarSegMac() <= cbKeyMost );

    const ULONG cbVarSegMac     = ( cbKeyMost == pidb->CbVarSegMac() ?
                                        cbKeyMost+1 :
                                        pidb->CbVarSegMac() );
    Assert( cbVarSegMac > 0 );
    Assert( cbVarSegMac < cbKeyMost || cbVarSegMac == cbKeyMost+1 );

    const BOOL  fSortNullsHigh  = pidb->FSortNullsHigh();


    Assert( !FKSTooBig( pfucb ) );
    Assert( cbAvail > 0 );
    if ( NULL == plineColumn || NULL == plineColumn->Pv() || 0 == plineColumn->Cb() )
    {
        if ( fTupleField )
        {
            Assert( FRECTextColumn( coltyp ) || FRECBinaryColumn( coltyp ) );
            Assert( pidb->CchTuplesLengthMin() > 0 );
            return ErrERRCheck( JET_errIndexTuplesKeyTooSmall );
        }
        else
        {
            const BOOL  fKeyDataZeroLengthGrbit = grbit & JET_bitKeyDataZeroLength;

            if ( fKeyDataZeroLengthGrbit )
            {
                if ( fFixedField )
                {
                    return ErrERRCheck( JET_errInvalidBufferSize );
                }
                if ( !FRECTextColumn( coltyp ) && !FRECBinaryColumn( coltyp ) )
                {
                    return ErrERRCheck( JET_errInvalidBufferSize );
                }
            }

            FLDNormalizeNullSegment(
                    pbNorm,
                    coltyp,
                    fKeyDataZeroLengthGrbit,
                    fSortNullsHigh );
            plineNorm->SetCb( 1 );
        }
    }

    else
    {
        INT     cbSeg   = 0;

        cbColumn = plineColumn->Cb();
        pbColumn = (BYTE *)plineColumn->Pv();

        switch ( coltyp )
        {
            case JET_coltypText:
            case JET_coltypLongText:
            {
                if ( fTupleField )
                {
                    Assert( FRECTextColumn( coltyp ) || FRECBinaryColumn( coltyp ) );

                    Assert( usUniCodePage != cp || cbColumn % 2 == 0 );
                    const ULONG     chColumn    = ( usUniCodePage == cp ? cbColumn / 2 : cbColumn );
                    const ULONG     cbColumnMax = ( usUniCodePage == cp ? pidb->CchTuplesLengthMax() * 2 : pidb->CchTuplesLengthMax() );

                    if ( chColumn < pidb->CchTuplesLengthMin() )
                    {
                        return ErrERRCheck( JET_errIndexTuplesKeyTooSmall );
                    }
                    else
                    {
                        cbColumn = min( cbColumn, (INT)cbColumnMax );
                    }
                }

                const ERR   errNorm     = ErrFLDNormalizeTextSegment(
                                                pbColumn,
                                                cbColumn,
                                                pbNorm,
                                                &cbSeg,
                                                cbAvail,
                                                cbKeyMost,
                                                cbVarSegMac,
                                                cp,
                                                pidb->PnlvGetNormalizationStruct() );

                if ( errNorm < 0 )
                    return errNorm;
                else if ( wrnFLDKeyTooBig == errNorm )
                    KSSetTooBig( pfucb );
                else
                    CallS( errNorm );

                Assert( cbSeg > 0 );
                break;
            }

            case JET_coltypBinary:
            case JET_coltypLongBinary:
            {
                BOOL    fColumnTruncated = fFalse;
                Assert( FRECBinaryColumn( coltyp ) );
                FLDNormalizeBinarySegment(
                        pbColumn,
                        cbColumn,
                        pbNorm,
                        &cbSeg,
                        cbAvail,
                        cbKeyMost,
                        cbVarSegMac,
                        fFixedField,
                        &fColumnTruncated,
                        &ibBinaryColumnDelimiter );
                if ( fColumnTruncated )
                {
                    KSSetTooBig( pfucb );
                }
                break;
            }

            default:
                FLDNormalizeFixedSegment(
                        pbColumn,
                        cbColumn,
                        pbNorm,
                        &cbSeg,
                        coltyp,
                        fDotNetGuid,
                        fTrue );
                if ( (ULONG)cbSeg > cbAvail )
                {
                    cbSeg = cbAvail;
                    KSSetTooBig( pfucb );
                }
                break;
        }

        Assert( (ULONG)cbSeg <= cbAvail );
        plineNorm->SetCb( cbSeg );
    }


    if ( fDescending )
    {
        BYTE *pbMin = (BYTE *)plineNorm->Pv();
        BYTE *pb = pbMin + plineNorm->Cb() - 1;
        while ( pb >= pbMin )
            *pb-- ^= 0xff;
    }

    Assert( (ULONG)plineNorm->Cb() < cbAvail + 1 );
    switch ( grbit & JET_maskLimitOptions )
    {
        case JET_bitFullColumnStartLimit:
        case JET_bitFullColumnEndLimit:
            FLDISetFullColumnLimit( plineNorm, cbAvail + 1, grbit & JET_bitFullColumnEndLimit );
            KSSetLimit( pfucb );
            break;
        case JET_bitPartialColumnStartLimit:
        case JET_bitPartialColumnEndLimit:
            if ( FRECTextColumn( coltyp ) )
            {
                FLDISetPartialColumnLimitOnTextColumn(
                        plineNorm,
                        cbAvail + 1,
                        fDescending,
                        grbit & JET_bitPartialColumnEndLimit,
                        cp );
            }
            else
            {
                FLDITrySetPartialColumnLimitOnBinaryColumn(
                        plineNorm,
                        cbAvail + 1,
                        ibBinaryColumnDelimiter,
                        grbit & JET_bitPartialColumnEndLimit );
            }
            KSSetLimit( pfucb );
            break;
        default:
        {
            Assert( !( grbit & JET_maskLimitOptions ) );
            if ( ( grbit & JET_bitSubStrLimit )
                && FRECTextColumn( coltyp ) )
            {
                FLDISetPartialColumnLimitOnTextColumn(
                        plineNorm,
                        cbAvail + 1,
                        fDescending,
                        fTrue,
                        cp );
                KSSetLimit( pfucb );
            }
            else if ( grbit & JET_bitStrLimit )
            {
                FLDITrySetPartialColumnLimitOnBinaryColumn(
                        plineNorm,
                        cbAvail + 1,
                        ibBinaryColumnDelimiter,
                        fTrue );
                KSSetLimit( pfucb );
            }
            break;
        }
    }

    return JET_errSuccess;
}


LOCAL VOID RECINormalisePlaceholder(
    FUCB    * const pfucb,
    FCB     * const pfcbTable,
    IDB     * const pidb )
{
    Assert( !FKSPrepared( pfucb ) );

    Assert( pidb->FHasPlaceholderColumn() );
    Assert( pidb->Cidxseg() > 1 );
    Assert( pidb->FPrimary() );

    pfcbTable->EnterDML();

    const TDB       * const ptdb    = pfcbTable->Ptdb();
    const IDXSEG    idxseg          = PidxsegIDBGetIdxSeg( pidb, ptdb )[0];
    const BOOL      fDescending     = idxseg.FDescending();

#ifdef DEBUG
    Assert( FCOLUMNIDFixed( idxseg.Columnid() ) );
    const FIELD     * pfield        = ptdb->PfieldFixed( idxseg.Columnid() );

    Assert( FFIELDPrimaryIndexPlaceholder( pfield->ffield ) );
    Assert( JET_coltypBit == pfield->coltyp );
#endif

    pfcbTable->LeaveDML();


    const BYTE      bPrefix         = BYTE( fDescending ? ~bPrefixData : bPrefixData );
    const BYTE      bData           = BYTE( fDescending ? 0xff : 0x00 );
    BYTE            * pbSeg         = (BYTE *)pfucb->dataSearchKey.Pv();

    pbSeg[0] = bPrefix;
    pbSeg[1] = bData;
    pfucb->dataSearchKey.SetCb( 2 );
    pfucb->cColumnsInSearchKey = 1;
}

ERR VTAPI ErrIsamMakeKey(
    JET_SESID       sesid,
    JET_VTID        vtid,
    __in_bcount( cbKeySegT ) const VOID*        pvKeySeg,
    const ULONG     cbKeySegT,
    JET_GRBIT       grbit )
{
    ERR             err;
    PIB*            ppib            = reinterpret_cast<PIB *>( sesid );
    FUCB*           pfucbTable      = reinterpret_cast<FUCB *>( vtid );
    FUCB*           pfucb;
    FCB*            pfcbTable;
    BYTE*           pbKeySeg        = const_cast<BYTE *>( reinterpret_cast<const BYTE *>( pvKeySeg ) );
    IDB*            pidb            = pidbNil;
    BOOL            fInitialIndex   = fFalse;
    INT             iidxsegCur;
    const size_t    cbSegStack      = 256;
    BYTE            rgbSegStack[ cbSegStack ];
    BYTE            *pbSegRes       = NULL;
    DATA            lineNormSeg;
    BYTE            rgbFixedColumnKeyPadded[ JET_cbColumnMost ];
    BOOL            fFixedField;
    DATA            lineKeySeg;
    ULONG           cbKeyMost;
    TDB             *ptdb = ptdbNil;
    IDXSEG          idxseg;
    BOOL            fDescending;
    COLUMNID        columnid;
    FIELD           *pfield;

    ULONG       cbKeySeg = cbKeySegT;

    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucbTable );
    AssertDIRNoLatch( ppib );

    if ( pfucbNil != pfucbTable->pfucbCurIndex )
    {
        pfucb = pfucbTable->pfucbCurIndex;
        Assert( pfucb->u.pfcb->FTypeSecondaryIndex() );
        Assert( pfucb->u.pfcb->Pidb() != pidbNil );
        Assert( !pfucb->u.pfcb->Pidb()->FPrimary() );
    }
    else
    {
        pfucb = pfucbTable;
        Assert( pfucb->u.pfcb->FPrimaryIndex() );
        Assert( pfucb->u.pfcb->Pidb() == pidbNil
            || pfucb->u.pfcb->Pidb()->FPrimary() );
    }

    pfcbTable = pfucbTable->u.pfcb;
    if ( FFUCBIndex( pfucbTable ) )
    {
        if ( pfucbTable->pfucbCurIndex != pfucbNil )
        {
            Assert( pfucb == pfucbTable->pfucbCurIndex );
            Assert( pfucb->u.pfcb->FTypeSecondaryIndex() );
            pidb = pfucb->u.pfcb->Pidb();
            fInitialIndex = pfucb->u.pfcb->FInitialIndex();
            if ( pfucb->u.pfcb->FDerivedIndex() &&  !pidb->FIDBOwnedByFCB() )
            {
                Assert( pidb->FTemplateIndex() );
                Assert( pfcbTable->Ptdb() != ptdbNil );
                pfcbTable = pfcbTable->Ptdb()->PfcbTemplateTable();
                Assert( pfcbNil != pfcbTable );
            }
        }
        else
        {
            BOOL    fPrimaryIndexTemplate   = fFalse;

            Assert( pfcbTable->FPrimaryIndex() );
            if ( pfcbTable->FDerivedTable() )
            {
                Assert( pfcbTable->Ptdb() != ptdbNil );
                Assert( pfcbTable->Ptdb()->PfcbTemplateTable() != pfcbNil );
                if ( !pfcbTable->Ptdb()->PfcbTemplateTable()->FSequentialIndex() )
                {
                    fPrimaryIndexTemplate = fTrue;
                    pfcbTable = pfcbTable->Ptdb()->PfcbTemplateTable();
                    pidb = pfcbTable->Pidb();
                    Assert( pidbNil != pidb );
                    Assert( pfcbTable->Pidb()->FTemplateIndex() );
                    fInitialIndex = fTrue;
                }
                else
                {
                    Assert( pfcbTable->Ptdb()->PfcbTemplateTable()->Pidb() == pidbNil );
                }
            }

            if ( !fPrimaryIndexTemplate )
            {
                if ( pfcbTable->FInitialIndex() )
                {
                    pidb = pfcbTable->Pidb();
                    fInitialIndex = fTrue;
                }
                else
                {
                    pfcbTable->EnterDML();

                    pidb = pfcbTable->Pidb();

                    const ERR   errT    = ( pidbNil != pidb ?
                                                ErrFILEIAccessIndex( pfucbTable->ppib, pfcbTable, pfcbTable ) :
                                                ErrERRCheck( JET_errNoCurrentIndex ) );

                    pfcbTable->LeaveDML();

                    if ( errT < JET_errSuccess )
                    {
                        if ( pidb || !( grbit & JET_bitNormalizedKey ) )
                        {
                            return ( JET_errIndexNotFound == errT ?
                                        ErrERRCheck( JET_errNoCurrentIndex ) :
                                        errT );
                        }
                    }
                }
            }
        }
    }
    else
    {
        pidb = pfucbTable->u.pscb->fcb.Pidb();
        Assert( pfcbTable == &pfucbTable->u.pscb->fcb );
    }

    Assert( pidb != pidbNil || ( grbit & JET_bitNormalizedKey ) );
    Assert( pidb == pidbNil || pidb->Cidxseg() > 0 );

    if ( pidb == pidbNil )
    {
        cbKeyMost = JET_cbKeyMost_OLD;
    }
    else
    {
        cbKeyMost = pidb->CbKeyMost();
    }
    if ( cbKeyMost > cbSegStack )
    {
        Alloc( pbSegRes = (BYTE *)RESKEY.PvRESAlloc() );
        lineNormSeg.SetPv( pbSegRes );
    }
    else
    {
        lineNormSeg.SetPv( rgbSegStack );
    }

    lineKeySeg.SetPv( pbKeySeg );
    if ( cbKeyMost == JET_cbKeyMost_OLD )
    {
        lineKeySeg.SetCb( min( cbKeySeg, JET_cbKeyMost_OLD ) );
    }
    else
    {
        lineKeySeg.SetCb( cbKeySeg );
    }

    if ( NULL == pfucbTable->dataSearchKey.Pv() )
    {
        Assert( !FKSPrepared( pfucbTable ) );
        Assert( !FKSPrepared( pfucb ) );

        pfucbTable->dataSearchKey.SetPv( RESKEY.PvRESAlloc() );
        if ( NULL == pfucbTable->dataSearchKey.Pv() )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        pfucbTable->dataSearchKey.SetCb( 0 );
        pfucbTable->cColumnsInSearchKey = 0;
        KSReset( pfucbTable );
    }


    if ( NULL == pfucb->dataSearchKey.Pv() )
    {
        pfucbTable->cColumnsInSearchKey = 0;
        KSReset( pfucbTable );

        Assert( !FKSPrepared( pfucb ) );
        pfucb->dataSearchKey.SetPv( pfucbTable->dataSearchKey.Pv() );
        pfucb->dataSearchKey.SetCb( 0 );
        pfucb->cColumnsInSearchKey = 0;
        FUCBSetUsingTableSearchKeyBuffer( pfucb );

        KSReset( pfucb );
    }

    Assert( !( grbit & JET_bitKeyDataZeroLength ) || 0 == cbKeySeg );

    if ( grbit & JET_bitNormalizedKey )
    {
        const ULONG cbLimitKeyMost = KEY::CbLimitKeyMost( (USHORT)cbKeyMost );
        if ( cbKeySeg > cbLimitKeyMost &&
            cbKeySeg > JET_cbKeyMost )
        {
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }
        cbKeySeg = min( cbLimitKeyMost, cbKeySeg );

        KSReset( pfucb );

        pfucb->cColumnsInSearchKey = 1;
        UtilMemCpy( (BYTE *)pfucb->dataSearchKey.Pv(), pbKeySeg, cbKeySeg );
        pfucb->dataSearchKey.SetCb( cbKeySeg );

        KSSetPrepare( pfucb );
        if ( cbKeySeg > cbKeyMost )
        {
            KSSetLimit( pfucb );
        }

        err = JET_errSuccess;
        goto HandleError;
    }

    else if ( grbit & JET_bitNewKey )
    {
        KSReset( pfucb );

        pfucb->dataSearchKey.SetCb( 0 );
        pfucb->cColumnsInSearchKey = 0;
    }
    else if ( FKSLimit( pfucb ) )
    {
        Call( ErrERRCheck( JET_errKeyIsMade ) );
    }
    else if ( !FKSPrepared( pfucb ) )
    {
        Call( ErrERRCheck( JET_errKeyNotMade ) );
    }
    if ( !FKSPrepared( pfucb )
        && pidb->FHasPlaceholderColumn()
        && !( grbit & JET_bitKeyOverridePrimaryIndexPlaceholder ) )
    {
        RECINormalisePlaceholder( pfucb, pfcbTable, pidb );
    }

    iidxsegCur = pfucb->cColumnsInSearchKey;
    if ( iidxsegCur >= pidb->Cidxseg() )
    {
        Call( ErrERRCheck( JET_errKeyIsMade ) );
    }

    const BOOL      fUseDMLLatch    = ( !fInitialIndex
                                        || pidb->FIsRgidxsegInMempool() );

    if ( fUseDMLLatch )
        pfcbTable->EnterDML();

    ptdb = pfcbTable->Ptdb();
    idxseg = PidxsegIDBGetIdxSeg( pidb, ptdb )[iidxsegCur];
    fDescending = idxseg.FDescending();
    columnid = idxseg.Columnid();

    if ( fFixedField = FCOLUMNIDFixed( columnid ) )
    {
        Assert( fUseDMLLatch
                || idxseg.FTemplateColumn()
                || FidOfColumnid( columnid ) <= ptdb->FidFixedLastInitial() );
        pfield = ptdb->PfieldFixed( columnid );

        Assert( pfield->cbMaxLen <= JET_cbColumnMost );
        if ( cbKeySeg > 0 && cbKeySeg != pfield->cbMaxLen )
        {
            Assert( pfield->coltyp != JET_coltypLongText );
            if ( pfield->coltyp == JET_coltypText
                 && cbKeySeg < pfield->cbMaxLen
                 && cbKeySeg < sizeof( rgbFixedColumnKeyPadded ) )
            {
                const INT cbLineKeySeg = lineKeySeg.Cb();
                
                AssertPREFIX( cbKeySeg == (ULONG)cbLineKeySeg );
                UtilMemCpy( rgbFixedColumnKeyPadded, lineKeySeg.Pv(), cbLineKeySeg );

                AssertPREFIX( pfield->cbMaxLen < sizeof( rgbFixedColumnKeyPadded ) );
                RECTextColumnPadding( rgbFixedColumnKeyPadded, pfield->cbMaxLen, cbLineKeySeg, pfield->cp );
                
                lineKeySeg.SetPv( rgbFixedColumnKeyPadded );
                lineKeySeg.SetCb( pfield->cbMaxLen );
            }
            else
            {
                if ( fUseDMLLatch )
                    pfcbTable->LeaveDML();
                Call( ErrERRCheck( JET_errInvalidBufferSize ) );
            }
        }
    }
    else if ( FCOLUMNIDTagged( columnid ) )
    {
        Assert( fUseDMLLatch
                || idxseg.FTemplateColumn()
                || FidOfColumnid( columnid ) <= ptdb->FidTaggedLastInitial() );
        pfield = ptdb->PfieldTagged( columnid );
    }
    else
    {
        Assert( FCOLUMNIDVar( columnid ) );
        Assert( fUseDMLLatch
                || idxseg.FTemplateColumn()
                || FidOfColumnid( columnid ) <= ptdb->FidVarLastInitial() );
        pfield = ptdb->PfieldVar( columnid );
    }

    const JET_COLTYP    coltyp      = pfield->coltyp;
    const USHORT        cp          = pfield->cp;

    if ( fUseDMLLatch )
        pfcbTable->LeaveDML();

    switch ( grbit & JET_maskLimitOptions )
    {
        case JET_bitPartialColumnStartLimit:
        case JET_bitPartialColumnEndLimit:
            if ( !FRECTextColumn( coltyp )
                && ( fFixedField || !FRECBinaryColumn( coltyp ) ) )
            {
                Call( ErrERRCheck( JET_errInvalidGrbit ) );
            }
        case 0:
        case JET_bitFullColumnStartLimit:
        case JET_bitFullColumnEndLimit:
            break;

        default:
            Call( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    Assert( pidb->CbKeyMost() == cbKeyMost );
    Assert( pfucb->dataSearchKey.Cb() < cbLimitKeyMostMost );

    if ( !FKSTooBig( pfucb )
        && (ULONG)pfucb->dataSearchKey.Cb() < cbKeyMost )
    {
        const ERR   errNorm     = ErrFLDNormalizeSegment(
                                        pfucb,
                                        pidb,
                                        ( cbKeySeg != 0 || ( grbit & JET_bitKeyDataZeroLength ) ) ? &lineKeySeg : NULL,
                                        &lineNormSeg,
                                        coltyp,
                                        cp,
                                        cbKeyMost - pfucb->dataSearchKey.Cb(),
                                        fDescending,
                                        fFixedField,
                                        ( ( 0 == iidxsegCur ) && pidb->FTuples() ),
                                        grbit );
        if ( errNorm < 0 )
        {
#ifdef DEBUG
            if ( JET_errInvalidBufferSize != errNorm )
            {
                Assert( FRECTextColumn( coltyp ) );
                switch ( errNorm )
                {
                    case JET_errInvalidLanguageId:
                    case JET_errOutOfMemory:
                    case JET_errUnicodeNormalizationNotSupported:
                        Assert( usUniCodePage == cp );
                        break;
                    case JET_errIndexTuplesKeyTooSmall:
                        Assert( pidb->FTuples() );
                        break;
                    default:
                        CallS( errNorm );
                }
            }
#endif
            err = errNorm;
            goto HandleError;
        }
        CallS( errNorm );
    }
    else
    {
        lineNormSeg.SetCb( 0 );
    }

    pfucb->cColumnsInSearchKey++;

    Assert( pfucb->dataSearchKey.Cb() + lineNormSeg.Cb() <= KEY::CbLimitKeyMost( pidb->CbKeyMost() ) );
    if ( pfucb->dataSearchKey.Cb() + lineNormSeg.Cb() > KEY::CbLimitKeyMost( pidb->CbKeyMost() ) )
    {
        lineNormSeg.SetCb( KEY::CbLimitKeyMost( pidb->CbKeyMost() ) - pfucb->dataSearchKey.Cb() );
    }

    UtilMemCpy(
        (BYTE *)pfucb->dataSearchKey.Pv() + pfucb->dataSearchKey.Cb(),
        lineNormSeg.Pv(),
        lineNormSeg.Cb() );
    pfucb->dataSearchKey.DeltaCb( lineNormSeg.Cb() );
    KSSetPrepare( pfucb );
    AssertDIRNoLatch( ppib );

    CallS( err );

HandleError:
    RESKEY.Free( pbSegRes );
    return err;
}

VOID RECIRetrieveColumnFromKey(
    TDB * const             ptdb,
    IDB * const             pidb,
    const IDXSEG * const    pidxseg,
    const FIELD * const     pfield,
    const BYTE *&           pbKey,
    const BYTE * const      pbKeyMax,
    DATA * const            plineColumn,
    BOOL * const            pfNull )
{
    const ULONG     cbKeyMost       = pidb->CbKeyMost();
    const BOOL      fDotNetGuid     = pidb->FDotNetGuid();
    const BYTE      bPrefixNullT    = ( pidb->FSortNullsHigh() ? bPrefixNullHigh : bPrefixNull );
    JET_COLTYP      coltyp          = pfield->coltyp;
    BOOL            fFixedField     = FCOLUMNIDFixed( pidxseg->Columnid() );
    ULONG           cbField;

    const BOOL      fDescending     = pidxseg->FDescending();
    const BYTE      bMask           = BYTE( fDescending ? ~BYTE( 0 ) : BYTE( 0 ) );
    const WORD      wMask           = WORD( fDescending ? ~WORD( 0 ) : WORD( 0 ) );
    const DWORD     dwMask          = DWORD( fDescending ? ~DWORD( 0 ) : DWORD( 0 ) );
    const QWORD     qwMask          = QWORD( fDescending ? ~QWORD( 0 ) : QWORD( 0 ) );

    *pfNull = fFalse;

    Assert( coltyp != JET_coltypNil );

    BYTE        * const pbDataColumn = (BYTE *)plineColumn->Pv();

    switch ( coltyp )
    {
        default:
            Assert( coltyp == JET_coltypBit );
            if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
            {
                plineColumn->SetCb( 0 );
                *pfNull = fTrue;
            }
            else
            {
                Assert( pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
                plineColumn->SetCb( 1 );

                *( (BYTE *)plineColumn->Pv() ) = BYTE( ( ( bMask ^ pbKey[ 0 ] ) == 0 ) ? 0x00 : 0xff );

                pbKey++;
            }
            break;

        case JET_coltypUnsignedByte:
            if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
            {
                plineColumn->SetCb( 0 );
                *pfNull = fTrue;
            }
            else
            {
                Assert( pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
                plineColumn->SetCb( 1 );

                *( (BYTE *)plineColumn->Pv() ) = BYTE( bMask ^ pbKey[ 0 ] );

                pbKey++;
            }
            break;

        case JET_coltypShort:
            if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
            {
                plineColumn->SetCb( 0 );
                *pfNull = fTrue;
            }
            else
            {
                Assert( pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
                plineColumn->SetCb( 2 );

                *( (Unaligned< WORD > *)plineColumn->Pv() ) =
                        WORD( wMask ^ wFlipHighBit( *( (UnalignedBigEndian< WORD >*) &pbKey[ 0 ] ) ) );

                pbKey += 2;
            }
            break;

        case JET_coltypUnsignedShort:
            if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
            {
                plineColumn->SetCb( 0 );
                *pfNull = fTrue;
            }
            else
            {
                Assert( pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
                plineColumn->SetCb( 2 );

                *( (Unaligned< WORD > *)plineColumn->Pv() ) =
                        WORD( wMask ^ *( (UnalignedBigEndian< WORD >*) &pbKey[ 0 ] ) );

                pbKey += 2;
            }
            break;

        case JET_coltypLong:
            if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
            {
                plineColumn->SetCb( 0 );
                *pfNull = fTrue;
            }
            else
            {
                Assert(pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
                plineColumn->SetCb( 4 );

                *( (Unaligned< DWORD >*) plineColumn->Pv() ) = dwMask ^ dwFlipHighBit( *( (UnalignedBigEndian< DWORD >*) &pbKey[ 0 ] ) );

                pbKey += 4;
            }
            break;

        case JET_coltypUnsignedLong:
            if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
            {
                plineColumn->SetCb( 0 );
                *pfNull = fTrue;
            }
            else
            {
                Assert(pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
                plineColumn->SetCb( 4 );

                *( (Unaligned< DWORD >*) plineColumn->Pv() ) = dwMask ^ *( (UnalignedBigEndian< DWORD >*) &pbKey[ 0 ] );

                pbKey += 4;
            }
            break;

        case JET_coltypLongLong:
        case JET_coltypCurrency:
            if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
            {
                plineColumn->SetCb( 0 );
                *pfNull = fTrue;
            }
            else
            {
                Assert(pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
                plineColumn->SetCb( 8 );

                *( (Unaligned< QWORD >*) plineColumn->Pv() ) = qwMask ^ qwFlipHighBit( *( (UnalignedBigEndian< QWORD >*) &pbKey[ 0 ] ) );

                pbKey += 8;
            }
            break;

        case JET_coltypUnsignedLongLong:
            if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
            {
                plineColumn->SetCb( 0 );
                *pfNull = fTrue;
            }
            else
            {
                Assert(pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
                plineColumn->SetCb( 8 );

                *( (Unaligned< QWORD >*) plineColumn->Pv() ) = qwMask ^ *( (UnalignedBigEndian< QWORD >*) &pbKey[ 0 ] );

                pbKey += 8;
            }
            break;

        case JET_coltypIEEESingle:
            if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
            {
                plineColumn->SetCb( 0 );
                *pfNull = fTrue;
            }
            else
            {
                Assert(pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
                plineColumn->SetCb( 4 );

                DWORD dwT = dwMask ^ *( (UnalignedBigEndian< DWORD >*) &pbKey[ 0 ] );

                if ( dwT & maskDWordHighBit )
                {
                    dwT = dwFlipHighBit( dwT );
                }
                else
                {
                    dwT = ~dwT;
                }

                *( (Unaligned< DWORD >*) plineColumn->Pv() ) = dwT;

                pbKey += 4;
            }
            break;

        case JET_coltypIEEEDouble:
        case JET_coltypDateTime:
            if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
            {
                plineColumn->SetCb( 0 );
                *pfNull = fTrue;
            }
            else
            {
                Assert(pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
                plineColumn->SetCb( 8 );

                QWORD qwT = qwMask ^ *( (UnalignedBigEndian< QWORD >*) &pbKey[ 0 ] );

                if ( qwT & maskQWordHighBit )
                {
                    qwT = qwFlipHighBit( qwT );
                }
                else
                {
                    qwT = ~qwT;
                }

                *( (Unaligned< QWORD >*) plineColumn->Pv() ) = qwT;

                pbKey += 8;
            }
            break;

        case JET_coltypGUID:
            if ( fDotNetGuid )
            {
                if ( *pbKey++ == bPrefixData )
                {
                    cbField = 16;
                    Assert( cbField <= SIZE_T( pbKeyMax - pbKey ) );
                    plineColumn->SetCb( cbField );
                    ((unsigned char *)plineColumn->Pv())[3] = pbKey[0];
                    ((unsigned char *)plineColumn->Pv())[2] = pbKey[1];
                    ((unsigned char *)plineColumn->Pv())[1] = pbKey[2];
                    ((unsigned char *)plineColumn->Pv())[0] = pbKey[3];
                    ((unsigned char *)plineColumn->Pv())[5] = pbKey[4];
                    ((unsigned char *)plineColumn->Pv())[4] = pbKey[5];
                    ((unsigned char *)plineColumn->Pv())[7] = pbKey[6];
                    ((unsigned char *)plineColumn->Pv())[6] = pbKey[7];
                    ((unsigned char *)plineColumn->Pv())[8] = pbKey[8];
                    ((unsigned char *)plineColumn->Pv())[9] = pbKey[9];
                    ((unsigned char *)plineColumn->Pv())[10] = pbKey[10];
                    ((unsigned char *)plineColumn->Pv())[11] = pbKey[11];
                    ((unsigned char *)plineColumn->Pv())[12] = pbKey[12];
                    ((unsigned char *)plineColumn->Pv())[13] = pbKey[13];
                    ((unsigned char *)plineColumn->Pv())[14] = pbKey[14];
                    ((unsigned char *)plineColumn->Pv())[15] = pbKey[15];
                    pbKey += 16;
                }
                else
                {
                    Assert( pbKey[-1] == bPrefixNullT );
                    plineColumn->SetCb( 0 );
                    *pfNull = fTrue;
                }
            }
            else
            {
                if ( *pbKey++ == bPrefixData )
                {
                    cbField = 16;
                    Assert( cbField <= SIZE_T( pbKeyMax - pbKey ) );
                    plineColumn->SetCb( cbField );
                    ((unsigned char *)plineColumn->Pv())[10] = pbKey[0];
                    ((unsigned char *)plineColumn->Pv())[11] = pbKey[1];
                    ((unsigned char *)plineColumn->Pv())[12] = pbKey[2];
                    ((unsigned char *)plineColumn->Pv())[13] = pbKey[3];
                    ((unsigned char *)plineColumn->Pv())[14] = pbKey[4];
                    ((unsigned char *)plineColumn->Pv())[15] = pbKey[5];
                    ((unsigned char *)plineColumn->Pv())[8] = pbKey[6];
                    ((unsigned char *)plineColumn->Pv())[9] = pbKey[7];
                    ((unsigned char *)plineColumn->Pv())[6] = pbKey[8];
                    ((unsigned char *)plineColumn->Pv())[7] = pbKey[9];
                    ((unsigned char *)plineColumn->Pv())[4] = pbKey[10];
                    ((unsigned char *)plineColumn->Pv())[5] = pbKey[11];
                    ((unsigned char *)plineColumn->Pv())[0] = pbKey[12];
                    ((unsigned char *)plineColumn->Pv())[1] = pbKey[13];
                    ((unsigned char *)plineColumn->Pv())[2] = pbKey[14];
                    ((unsigned char *)plineColumn->Pv())[3] = pbKey[15];
                    pbKey += 16;
                }
                else
                {
                    Assert( pbKey[-1] == bPrefixNullT );
                    plineColumn->SetCb( 0 );
                    *pfNull = fTrue;
                }
            }
            break;

        case JET_coltypText:
        case JET_coltypLongText:


        case JET_coltypBinary:
        case JET_coltypLongBinary:
            if ( fDescending )
            {
                if ( fFixedField )
                {
                    if ( *pbKey++ == (BYTE)~bPrefixData )
                    {
                        cbField = pfield->cbMaxLen;
                        Assert( cbField <= SIZE_T( pbKeyMax - pbKey ) );
                        plineColumn->SetCb( cbField );
                        for ( ULONG ibT = 0; ibT < cbField; ibT++ )
                        {
                            Assert( pbDataColumn == (BYTE *)plineColumn->Pv() );
                            pbDataColumn[ibT] = (BYTE)~*pbKey++;
                        }
                    }
                    else
                    {
                        Assert( pbKey[-1] == (BYTE)~bPrefixNullT );
                        plineColumn->SetCb( 0 );
                        *pfNull = fTrue;
                    }
                }
                else
                {
                    cbField = 0;
                    if ( *pbKey++ == (BYTE)~bPrefixData )
                    {
                        Assert( pbDataColumn == (BYTE *)plineColumn->Pv() );
                        if ( FRECBinaryColumn( coltyp ) )
                        {
                            BYTE    *pbColumn = pbDataColumn;
                            do {
                                Assert( pbKey + cbFLDBinaryChunkNormalized <= pbKeyMax );

                                BYTE    cbChunk = (BYTE)~pbKey[cbFLDBinaryChunkNormalized-1];
                                if ( cbFLDBinaryChunkNormalized == cbChunk )
                                    cbChunk = cbFLDBinaryChunkNormalized-1;

                                for ( BYTE ib = 0; ib < cbChunk; ib++ )
                                    pbColumn[ib] = (BYTE)~pbKey[ib];

                                cbField += cbChunk;
                                pbKey += cbFLDBinaryChunkNormalized;
                                pbColumn += cbChunk;
                            }
                            while ( pbKey[-1] == (BYTE)~cbFLDBinaryChunkNormalized );
                        }

                        else
                        {
                            Assert( FRECTextColumn( coltyp ) );
                            if ( pfield->cp == usUniCodePage )
                            {
                                for ( ; *pbKey != (BYTE)~bUnicodeSortKeySentinel; cbField++)
                                {
                                    Assert( pbKey < pbKeyMax );
                                    pbDataColumn[cbField] = (BYTE)~*pbKey++;
                                }
                                for ( ; *pbKey != (BYTE)~bUnicodeSortKeyTerminator; cbField++)
                                {
                                    Assert( pbKey < pbKeyMax );
                                    pbDataColumn[cbField] = (BYTE)~*pbKey++;
                                }
                                pbKey++;
                            }
                            else
                            {
                                for ( ; *pbKey != (BYTE)~bASCIISortKeyTerminator; cbField++)
                                {
                                    Assert( pbKey < pbKeyMax );
                                    pbDataColumn[cbField] = (BYTE)~*pbKey++;
                                }
                                pbKey++;
                            }
                        }
                    }
                    else if ( pbKey[-1] == (BYTE)~bPrefixNullT )
                    {
                        *pfNull = fTrue;
                    }
                    else
                    {
                        Assert( pbKey[-1] == (BYTE)~bPrefixZeroLength );
                    }
                    Assert( cbField <= cbKeyMost );
                    plineColumn->SetCb( cbField );
                }
            }
            else
            {
                if ( fFixedField )
                {
                    if ( *pbKey++ == bPrefixData )
                    {
                        cbField = pfield->cbMaxLen;
                        Assert( cbField <= SIZE_T( pbKeyMax - pbKey ) );
                        plineColumn->SetCb( cbField );
                        UtilMemCpy( plineColumn->Pv(), pbKey, cbField );
                        pbKey += cbField;
                    }
                    else
                    {
                        Assert( pbKey[-1] == bPrefixNullT );
                        plineColumn->SetCb( 0 );
                        *pfNull = fTrue;
                    }
                }
                else
                {
                    cbField = 0;
                    if ( *pbKey++ == bPrefixData )
                    {
                        Assert( pbDataColumn == (BYTE *)plineColumn->Pv() );
                        if ( FRECBinaryColumn( coltyp ) )
                        {
                            BYTE    *pbColumn = pbDataColumn;
                            do {
                                Assert( pbKey + cbFLDBinaryChunkNormalized <= pbKeyMax );

                                BYTE    cbChunk = pbKey[cbFLDBinaryChunkNormalized-1];
                                if ( cbFLDBinaryChunkNormalized == cbChunk )
                                    cbChunk = cbFLDBinaryChunkNormalized-1;

                                UtilMemCpy( pbColumn, pbKey, cbChunk );

                                cbField += cbChunk;
                                pbKey += cbFLDBinaryChunkNormalized;
                                pbColumn += cbChunk;
                            }
                            while ( pbKey[-1] == cbFLDBinaryChunkNormalized );
                        }

                        else
                        {
                            Assert( FRECTextColumn( coltyp ) );
                            if ( pfield->cp == usUniCodePage )
                            {
                                for ( ; *pbKey != bUnicodeSortKeySentinel; cbField++ )
                                {
                                    Assert( pbKey < pbKeyMax );
                                    pbDataColumn[cbField] = (BYTE)*pbKey++;
                                }
                                for ( ; *pbKey != bUnicodeSortKeyTerminator; cbField++ )
                                {
                                    Assert( pbKey < pbKeyMax );
                                    pbDataColumn[cbField] = (BYTE)*pbKey++;
                                }
                                pbKey++;
                            }
                            else
                            {
                                for ( ; *pbKey != bASCIISortKeyTerminator; cbField++ )
                                {
                                    Assert( pbKey < pbKeyMax );
                                    pbDataColumn[cbField] = (BYTE)*pbKey++;
                                }
                                pbKey++;
                            }
                        }
                    }
                    else if ( pbKey[-1] == bPrefixNullT )
                    {
                        *pfNull = fTrue;
                    }
                    else
                    {
                        Assert( pbKey[-1] == bPrefixZeroLength );
                    }
                    Assert( cbField <= cbKeyMost );
                    plineColumn->SetCb( cbField );
                }
            }
            break;
    }
}

ERR ErrRECIRetrieveColumnFromKey(
    TDB                     * ptdb,
    IDB                     * pidb,
    const KEY               * pkey,
    const COLUMNID          columnid,
    DATA                    * plineColumn )
{
    ERR                     err         = JET_errSuccess;
    const IDXSEG*           pidxseg;
    const IDXSEG*           pidxsegMac;

    Assert( ptdb != ptdbNil );
    Assert( pidb != pidbNil );
    Assert( pidb->Cidxseg() > 0 );
    Assert( !pkey->FNull() );
    Assert( plineColumn != NULL );
    Assert( plineColumn->Pv() != NULL );

    const ULONG         cbKeyMost       = pidb->CbKeyMost();
    const size_t        cbKeyStack      = 256;
    BYTE                rgbKeyStack[ cbKeyStack ];
    BYTE                *pbKeyRes       = NULL;
    BYTE                *pbKeyAlloc     = NULL;
    BYTE                *pbKey          = NULL;
    const BYTE          *pbKeyMax       = NULL;
    
    if ( cbKeyMost > cbKeyStack )
    {
        Alloc( pbKeyRes = (BYTE *)RESKEY.PvRESAlloc() );
        pbKeyAlloc = pbKeyRes;
    }
    else
    {
        pbKeyAlloc = rgbKeyStack;
    }
    pbKey = pbKeyAlloc;
    pbKeyMax = pbKey + cbKeyMost;
    pkey->CopyIntoBuffer( pbKey, cbKeyMost );

    pidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb );
    pidxsegMac = pidxseg + pidb->Cidxseg();

    for ( ; pidxseg < pidxsegMac && pbKey < pbKeyMax; pidxseg++ )
    {
        FIELD* pfield;

        if ( FCOLUMNIDTagged( pidxseg->Columnid() ) )
        {
            pfield = ptdb->PfieldTagged( pidxseg->Columnid() );
        }
        else if ( FCOLUMNIDFixed( pidxseg->Columnid() ) )
        {
            pfield = ptdb->PfieldFixed( pidxseg->Columnid() );
        }
        else
        {
            Assert( FCOLUMNIDVar( pidxseg->Columnid() ) );
            pfield = ptdb->PfieldVar( pidxseg->Columnid() );
            Assert( pfield->coltyp == JET_coltypBinary || pfield->coltyp == JET_coltypText );
        }

        BOOL fNull;
        RECIRetrieveColumnFromKey( ptdb, pidb, pidxseg, pfield, (const BYTE*&)pbKey, pbKeyMax, plineColumn, &fNull );

        if ( columnid == pidxseg->Columnid() )
        {
#ifdef DEBUG
            const FIELD * const pfieldT = ptdb->Pfield( columnid );
            AssertSz( !FRECTextColumn( pfieldT->coltyp ), "Can't de-normalise text strings (due to data loss during normalization)." );
#endif

            err = fNull ? ErrERRCheck( JET_wrnColumnNull ) : JET_errSuccess;
            break;
        }
    }

    CallSx( err, JET_wrnColumnNull );

HandleError:
    RESKEY.Free( pbKeyRes );
    return err;
}


ERR VTAPI ErrIsamRetrieveKey(
    JET_SESID       sesid,
    JET_VTID        vtid,
    VOID*           pv,
    const ULONG     cbMax,
    ULONG*          pcbActual,
    JET_GRBIT       grbit )
{
    ERR             err;
    PIB*            ppib        = reinterpret_cast<PIB *>( sesid );
    FUCB*           pfucb       = reinterpret_cast<FUCB *>( vtid );
    FUCB*           pfucbIdx;
    FCB*            pfcbIdx;
    ULONG           cbKeyReturned;

    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRNoLatch( ppib );

    if ( grbit & JET_bitRetrieveCopy )
    {
        if ( pfucb->pfucbCurIndex != pfucbNil )
            pfucb = pfucb->pfucbCurIndex;

        if ( pfucb->dataSearchKey.FNull()
            || NULL == pfucb->dataSearchKey.Pv() )
        {
            return ErrERRCheck( JET_errKeyNotMade );
        }
        if ( pv != NULL )
        {
            UtilMemCpy( pv,
                    pfucb->dataSearchKey.Pv(),
                    min( (ULONG)pfucb->dataSearchKey.Cb(), cbMax ) );
        }
        if ( pcbActual )
            *pcbActual = pfucb->dataSearchKey.Cb();

        return JET_errSuccess;
    }

    if ( FFUCBIndex( pfucb ) )
    {
        pfucbIdx = pfucb->pfucbCurIndex != pfucbNil ? pfucb->pfucbCurIndex : pfucb;
        Assert( pfucbIdx != pfucbNil );
        pfcbIdx = pfucbIdx->u.pfcb;
        Assert( pfcbIdx != pfcbNil );
        CallR( ErrDIRGet( pfucbIdx ) );
    }
    else
    {
        pfucbIdx = pfucb;
        pfcbIdx = (FCB *)pfucb->u.pscb;
        Assert( pfcbIdx != pfcbNil );
    }

    err = JET_errSuccess;

    cbKeyReturned = pfucbIdx->kdfCurr.key.Cb();
    if ( pcbActual )
        *pcbActual = cbKeyReturned;
    if ( cbKeyReturned > cbMax )
    {
        err = ErrERRCheck( JET_wrnBufferTruncated );
        cbKeyReturned = cbMax;
    }

    if ( pv != NULL )
    {
        UtilMemCpy( pv, pfucbIdx->kdfCurr.key.prefix.Pv(),
                min( (ULONG)pfucbIdx->kdfCurr.key.prefix.Cb(), cbKeyReturned ) );
        Assert( pfucbIdx->kdfCurr.key.prefix.Cb() >= 0 );
        UtilMemCpy( (BYTE *)pv+pfucbIdx->kdfCurr.key.prefix.Cb(),
                pfucbIdx->kdfCurr.key.suffix.Pv(),
                min( (ULONG)pfucbIdx->kdfCurr.key.suffix.Cb(),
                     ( ( cbKeyReturned < (ULONG) pfucbIdx->kdfCurr.key.prefix.Cb() ) ?
                           0:
                           cbKeyReturned - pfucbIdx->kdfCurr.key.prefix.Cb()
                     ) ) );
    }

    if ( FFUCBIndex( pfucb ) )
    {
        Assert( Pcsr( pfucbIdx )->FLatched( ) );
        CallS( ErrDIRRelease( pfucbIdx ) );
    }

    AssertDIRNoLatch( ppib );
    return err;
}


ERR VTAPI ErrIsamGetBookmark(
    JET_SESID       sesid,
    JET_VTID        vtid,
    VOID * const    pvBookmark,
    const ULONG     cbMax,
    ULONG * const   pcbActual )
{
    ERR             err;
    PIB *           ppib        = reinterpret_cast<PIB *>( sesid );
    FUCB *          pfucb       = reinterpret_cast<FUCB *>( vtid );
    ULONG           cb;
    ULONG           cbReturned;
    BOOKMARK *      pbm;

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucb );
    AssertDIRNoLatch( ppib );
    Assert( NULL != pvBookmark || 0 == cbMax );

    CallR( ErrDIRGetBookmark( pfucb, &pbm ) );
    Assert( !Pcsr( pfucb )->FLatched() );

    Assert( pbm->key.prefix.FNull() );
    Assert( pbm->data.FNull() );

    cb = pbm->key.Cb();

    if ( pcbActual )
        *pcbActual = cb;

    if ( cb <= cbMax )
    {
        cbReturned = cb;
    }
    else
    {
        cbReturned = cbMax;
        err = ErrERRCheck( JET_errBufferTooSmall );
    }

    pbm->key.CopyIntoBuffer( pvBookmark, cbReturned );

    AssertDIRNoLatch( ppib );
    return err;
}

ERR VTAPI ErrIsamGetIndexBookmark(
    JET_SESID       sesid,
    JET_VTID        vtid,
    VOID * const    pvSecondaryKey,
    const ULONG     cbSecondaryKeyMax,
    ULONG * const   pcbSecondaryKeyActual,
    VOID * const    pvPrimaryBookmark,
    const ULONG     cbPrimaryBookmarkMax,
    ULONG * const   pcbPrimaryBookmarkActual )
{
    ERR             err;
    PIB *           ppib            = reinterpret_cast<PIB *>( sesid );
    FUCB *          pfucb           = reinterpret_cast<FUCB *>( vtid );
    FUCB * const    pfucbIdx        = pfucb->pfucbCurIndex;
    BOOL            fBufferTooSmall = fFalse;
    ULONG           cb;
    ULONG           cbReturned;

    CallR( ErrPIBCheck( ppib ) );
    AssertDIRNoLatch( ppib );
    CheckTable( ppib, pfucb );
    Assert( FFUCBIndex( pfucb ) );
    Assert( FFUCBPrimary( pfucb ) );
    Assert( pfucb->u.pfcb->FPrimaryIndex() );

    Assert( NULL != pvSecondaryKey || 0 == cbSecondaryKeyMax );
    Assert( NULL != pvPrimaryBookmark || 0 == cbPrimaryBookmarkMax );

    if ( pfucbNil == pfucbIdx )
    {
        return ErrERRCheck( JET_errNoCurrentIndex );
    }

    Assert( FFUCBSecondary( pfucbIdx ) );
    Assert( pfucbIdx->u.pfcb->FTypeSecondaryIndex() );

    CallR( ErrDIRGet( pfucbIdx ) );
    Assert( Pcsr( pfucbIdx )->FLatched() );
    Assert( !pfucbIdx->kdfCurr.key.FNull() );

    cb = pfucbIdx->kdfCurr.key.Cb();
    if ( NULL != pcbSecondaryKeyActual )
        *pcbSecondaryKeyActual = cb;

    if ( cb <= cbSecondaryKeyMax )
    {
        cbReturned = cb;
    }
    else
    {
        cbReturned = cbSecondaryKeyMax;
        fBufferTooSmall = fTrue;
    }

    pfucbIdx->kdfCurr.key.CopyIntoBuffer( pvSecondaryKey, cbReturned );

    cb = pfucbIdx->kdfCurr.data.Cb();
    if ( NULL != pcbPrimaryBookmarkActual )
        *pcbPrimaryBookmarkActual = cb;

    if ( cb <= cbPrimaryBookmarkMax )
    {
        cbReturned = cb;
    }
    else
    {
        cbReturned = cbPrimaryBookmarkMax;
        fBufferTooSmall = fTrue;
    }

    UtilMemCpy( pvPrimaryBookmark, pfucbIdx->kdfCurr.data.Pv(), cbReturned );

    CallR( ErrDIRRelease( pfucbIdx ) );
    AssertDIRNoLatch( ppib );

    return ( fBufferTooSmall ? ErrERRCheck( JET_errBufferTooSmall ) : err );
}

