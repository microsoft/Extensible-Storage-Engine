// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "PageSizeClean.hxx"

const BYTE  bPrefixNull         = 0x00;
const BYTE  bPrefixZeroLength   = 0x40;
const BYTE  bPrefixNullHigh     = 0xc0;
const BYTE  bPrefixData         = 0x7f;
const BYTE  bSentinel           = 0xff;

//  UNDONE:  this knowledge is really OS specific so shouldn't we be using
//  these values in functions underneath the OS layer?
//
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

    // Null column handled elsewhere.
    Assert( NULL != pbField );
    Assert( cbField > 0 );

    Assert( cbKeyAvail > 0 );
    Assert( cbVarSegMac > 0 );

    // If cbVarSegMac == cbKeyMost, that implies that it
    // was never set.  However, to detect this, the caller
    // artificially increments it to cbKeyMost+1.
    Assert( cbVarSegMac <= cbKeyMost+1 );
    Assert( cbKeyMost != cbVarSegMac );

    // if cbVarSegMac was not set (ie. it is cbKeyMost+1),
    // then cbKeyAvail will always be less than it.
    Assert( cbVarSegMac < cbKeyMost || cbKeyAvail < cbVarSegMac );
    INT cbT = min( cbKeyAvail, cbVarSegMac );

    //  cbT is the max size of the key segment data,
    //  and does not include the header byte which indicates
    //  NULL key, zero length key, or non-NULL key.
    cbT--;

    //  cbChar is the size of the characters in the source data
    //
    ULONG cbChar = cp == usUniCodePage ? sizeof( wchar_t ) : sizeof( char );

    //  cbField may have been truncated without respect to char boundaries.
    //  if this has happened, chop off the remaining partial char data
    //
    //  either we are at cbFieldMax or we are using tuple indexing
    //
    //  an example:
    //      insert a unicode column of > 255 bytes
    //      ErrRECIRetrieveKey truncates the column to 255 bytes
    //      the tuple index tries to index 10 characters, the offset is 118 characters
    //      cbOffset = 236, to there are only 19 bytes left when we actually want 20.
    //
    ULONG cbFieldT = ( cbField / cbChar ) * cbChar;

    //  cbField may also need to be truncated to support legacy index
    //  formats.  we used to have a fixed max size for our key length at
    //  255 bytes.  when that was the case, we would always truncate the
    //  source data to 256 bytes prior to normalization.  we did this
    //  because we believed that no data beyond 256 bytes could possibly
    //  affect the outcome of the normalization.  we have since learned
    //  that this is false.  there is no guarantee that one Unicode char
    //  will result in exactly one sort weight in the output key.  as a
    //  result, we must forever continue to truncate the source data in
    //  this manner for indices that could have been created when this old
    //  code was in effect.  we can't know this easily, so we will claim
    //  that any index that uses a max key length of 255 bytes must follow
    //  this truncation rule
    //
    //  NOTE:  the effective behavior of the old code always resulted in
    //  only 256 bytes of data being passed to both the function that tests
    //  for undefined characters (later removed in 2012) and the function that normalizes the
    //  source data
    //
    if ( cbKeyMost == JET_cbKeyMost_OLD )
    {
        cbFieldT = min( cbFieldT, ( ( cbKeyMost + cbChar - 1 ) / cbChar ) * cbChar );
    }

    //  unicode support
    //
    if ( cp == usUniCodePage )
    {
        //  convert the source data into a key that can be compared as a simple
        //  binary string
        //
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
                    //  report unexpected error
                    CallS( err );
            }
#endif
            return err;
        }
    }
    else
    {
        //  convert the source data into a key that can be compared as a simple
        //  binary string
        //
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
        const INT   cbVarSegMacNoHeader = cbVarSegMac - 1;  // account for header byte

        // If not truncated on purpose, set flag
        Assert( cbKeyMost != cbVarSegMac );
        Assert( *pcbSeg <= cbVarSegMacNoHeader );
        if ( cbVarSegMac > cbKeyMost || *pcbSeg < cbVarSegMacNoHeader )
        {
            Assert( ULONG( *pcbSeg + 1 ) == cbKeyAvail );
        }
        else
        {
            // Truncated on purpose, so suppress warning.
            Assert( cbVarSegMac < cbKeyMost );
            Assert( cbVarSegMacNoHeader == *pcbSeg );
            err = JET_errSuccess;
        }
    }

    Assert( *pcbSeg >= 0 );
    Assert( *pcbSeg <= cbT );

    //  put the prefix there
    //
    *rgbSeg = bPrefixData;
    (*pcbSeg)++;

    return err;
}


#ifndef RTM
//  ================================================================
VOID FLDNormUnitTest()
//  ================================================================
//
//  Call ErrFLDNormalizeTextSegment with defined and undefined strings
//  to make sure the correct results are returned. We assume that
//  undefined code points are dropped completely when normalized.
//
//-
{
    // 08c0 is a currently-undefined character in the Arabic region.
    // e000-f8ff is a 'Private Use Area'.
    // fdd0-fdef 'are intended for process-internal uses, but are not
    //            permitted for interchange.'
    const wchar_t wszUndefined[]    = L"\xe001\xfdd2\xe0e0\x8c0";
    const INT cbUndefined           = sizeof( wszUndefined );
    const wchar_t wszDefined[]      = L"abcDEF123$%^;,.";
    const INT cbDefined             = sizeof( wszDefined );

    BYTE rgbNormalized[JET_cbKeyMost_OLD];
    INT cbNormalized = sizeof( rgbNormalized );

    NORM_LOCALE_VER nlv =
    {
        SORTIDNil, // Sort GUID
        dwLCMapFlagsDefault,
        0, // NLS Version
        0, // NLS Defined Version
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
#endif  //  !RTM

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
        || 0 == *pibBinaryColumnDelimiter );        //  either NULL or initialised to 0

    // Null column handled elsewhere.
    Assert( NULL != pbField );
    Assert( cbField > 0 );

    Assert( cbKeyAvail > 0 );
    Assert( cbVarSegMac > 0 );

    // If cbVarSegMac == cbKeyMost, that implies that it
    // was never set.  However, to detect this, the caller
    // artificially increments it to cbKeyMost+1.
    Assert( cbVarSegMac <= cbKeyMost+1 );
    Assert( cbKeyMost != cbVarSegMac );

    // if cbVarSegMac was not set (ie. it is cbKeyMost+1),
    // then cbKeyAvail will always be less than it.
    Assert( cbVarSegMac < cbKeyMost || cbKeyAvail < cbVarSegMac );

    rgbSeg[0] = bPrefixData;

    if ( fFixedField )
    {
        // calculate size of the normalized column, including header byte
        cbSeg = cbField + 1;

        // First check if we exceeded the segment maximum.
        if ( cbVarSegMac < cbKeyMost && cbSeg > cbVarSegMac )
        {
            cbSeg = cbVarSegMac;
        }

        // Once we've fitted the field into the segment
        // maximum, may need to resize further to fit
        // into the total key space.
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
        // The difference between fNormalisedEntireColumn and
        // fColumnTruncated is that fNormaliseEntiredColumn is
        // set to FALSE if we had to truncate the column because
        // of either limited key space or we exceeded the
        // limitation imposed by cbVarSegMac.  fColumnTruncated
        // is only set to TRUE if the column was truncated due
        // to limited key space.
        BOOL            fNormalisedEntireColumn     = fTrue;
        const ULONG     cChunks                     = ( cbField + ( cbFLDBinaryChunk-1 ) ) / cbFLDBinaryChunk;

        cbSeg = ( cChunks * cbFLDBinaryChunkNormalized ) + 1;       // +1 for header byte

        // First check if we exceeded the segment maximum.
        if ( cbVarSegMac < cbKeyMost && cbSeg > cbVarSegMac )
        {
            cbSeg = cbVarSegMac;
            fNormalisedEntireColumn = fFalse;
        }

        // Once we've fitted the field into the segment
        // maximum, may need to resize further to fit
        // into the total key space.
        if ( cbSeg > cbKeyAvail )
        {
            cbSeg = cbKeyAvail;
            fNormalisedEntireColumn = fFalse;
            *pfColumnTruncated = fTrue;
        }

        // At least one chunk, unless truncated.
        Assert( cbSeg > 0 );
        Assert( cbSeg > cbFLDBinaryChunkNormalized || !fNormalisedEntireColumn );

        ULONG       cbSegRemaining = cbSeg - 1; // account for header byte
        ULONG       cbFieldRemaining = cbField;
        BYTE        *pbSeg = rgbSeg + 1;    // skip header byte
        const BYTE  *pbNextChunk = pbField;
        while ( cbSegRemaining >= cbFLDBinaryChunkNormalized )
        {
            Assert( cbFieldRemaining > 0 );
            Assert( pbNextChunk + cbFieldRemaining == pbField + cbField );

            if ( cbFieldRemaining <= cbFLDBinaryChunk )
            {
                // This is the last chunk.
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
                    // This allows us to differentiate between a
                    // a column that is entirely normalised and ends
                    // at the end of a chunk and one that is truncated
                    // at the end of a chunk.
                    if ( fNormalisedEntireColumn )
                        *pbSeg++ = cbFLDBinaryChunkNormalized-1;
                    else
                        *pbSeg++ = cbFLDBinaryChunkNormalized;
                }
                else
                {
                    // Zero out rest of chunk.
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
            // Able to fit at least one chunk in.
            Assert( pbSeg >= rgbSeg + 1 + cbFLDBinaryChunkNormalized );
            Assert( pbSeg[-1] > 0 );
            Assert( pbSeg[-1] <= cbFLDBinaryChunkNormalized );

            // Must have ended up at the end of a chunk.
            Assert( ( pbSeg - ( rgbSeg + 1 ) ) % cbFLDBinaryChunkNormalized == 0 );
        }
        else
        {
            // Couldn't accommodate any chunks.
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
                // Fill out remaining key space.
                UtilMemCpy( pbSeg, pbNextChunk, cbSegRemaining );
            }
            else
            {
                if ( NULL != pibBinaryColumnDelimiter )
                {
                    Assert( 0 == *pibBinaryColumnDelimiter );
                    *pibBinaryColumnDelimiter = ULONG( pbSeg + cbFieldRemaining - rgbSeg );
                }

                // Entire column will fit, but last bytes don't form
                // a complete chunk.  Pad with zeroes to end of available
                // key space.
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
    BOOL                fDataPassedFromUser ) // data in machine format, not necessary little endian format
{
    WORD                wSrc;
    DWORD               dwSrc;
    QWORD               qwSrc;

    rgbSeg[0] = bPrefixData;
    switch ( coltyp )
    {
        //  BIT: prefix with 0x7f, flip high bit
        //
        case JET_coltypBit:
            Assert( 1 == cbField );
            *pcbSeg = 2;

            rgbSeg[1] = BYTE( pbField[0] == 0 ? 0x00 : 0xff );
            break;

        //  UBYTE: prefix with 0x7f
        //
        case JET_coltypUnsignedByte:
            Assert( 1 == cbField );
            *pcbSeg = 2;

            rgbSeg[1] = pbField[0];
            break;

        //  SHORT: prefix with 0x7f, flip high bit
        //
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

        //* LONG: prefix with 0x7f, flip high bit
        //
        //  works because of 2's complement *
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

        //* UNSIGNED LONG: prefix with 0x7f
        //
        //  works because of 2's complement *
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

        //  CURRENCY: First swap bytes.  Then, flip the sign bit
        //
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

        //  REAL: First swap bytes.  Then, if positive:
        //  flip sign bit, else negative: flip whole thing.
        //  Then prefix with 0x7f.
        //
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

        //  LONGREAL: First swap bytes.  Then, if positive:
        //  flip sign bit, else negative: flip whole thing.
        //  Then prefix with 0x7f.
        //
        //  Same for DATETIME
        //
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
                //  .Net Guid order should match string order of standard GUID structure comparison:
                //  DWORD, WORD, WORD, BYTE ARRAY[8]
                //  order[1..16] = input[3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15]  
                //
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
                //  GUID order should match string order of standard GUID to string conversion:
                //  order[1..16] = input[10, 11, 12, 13, 14, 15, 8, 9, 6, 7, 4, 5, 0, 1, 2, 3]
                //
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
            Assert( !FRECTextColumn( coltyp ) );    // These types handled elsewhere.
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
        //  most nulls are represented by 0x00
        //  zero-length columns are represented by 0x40
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
                fTrue,              //  cannot normalize NULL via this function
                fFalse );
        dataNorm.SetCb( 1 );
    }
    else
    {
        INT         cb      = 0;
        switch ( pfield->coltyp )
        {
            //  case-insensitive TEXT: convert to uppercase.
            //  If fixed, prefix with 0x7f;  else affix with 0x00
            //
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

            //  BINARY data: if fixed, prefix with 0x7f;
            //  else break into chunks of 8 bytes, affixing each
            //  with 0x09, except for the last chunk, which is
            //  affixed with the number of bytes in the last chunk.
            //
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
                        fFalse,                 //  only called for tagged columns
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


//+API
//  ErrRECIRetrieveKey
//  ========================================================
//  ErrRECIRetrieveKey( FUCB *pfucb, TDB *ptdb, IDB *pidb, DATA *plineRec, KEY *pkey, ULONG itagSequence )
//
//  Retrieves the normalized key from a record, based on an index descriptor.
//
//  PARAMETERS
//      pfucb           cursor for record
//      ptdb            column info for index
//      pidb            index key descriptor
//      plineRec        data record to retrieve key from
//      pkey            buffer to put retrieve key in; pkey->pv must
//                      point to a large enough buffer, cbKeyMost bytes.
//      itagSequence    A secondary index whose key contains a tagged
//                      column segment will have an index entry made for
//                      each value of the tagged column, each refering to
//                      the same record.  This parameter specifies which
//                      occurance of the tagged column should be included
//                      in the retrieve key.
//
//  RETURNS Error code, one of:
//      JET_errSuccess      success
//      +wrnFLDNullKey      key has all NULL segments
//      +wrnFLDNullSeg      key has NULL segment
//
//  COMMENTS
//      Key formation is as follows:  each key segment is retrieved
//      from the record, transformed into a normalized form, and
//      complemented if it is "descending" in the key.  The key is
//      formed by concatenating each such transformed segment.
//-
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

    //  fetch the max key size for this index
    //
    cbKeyMost   = pidb->CbKeyMost();

    //  we will initially try to use enough source data to saturate the max key
    //  size for this index under normal conditions
    //
    cbDataMost  = cbKeyMost;

    //  try to retrieve the key using the selected amount of source data
    //
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
        //  we used all the source data and we still did not reach the max key
        //  length.  let's double the source data size and try again
        //
        //  NOTE:  it is possible that there will be a degenerate case that
        //  will cause us to try and fetch so much data that we will fail with
        //  an out-of-memory error.  we currently view that as an acceptable
        //  failure mode for such an esoteric case.  for example, you can get
        //  into that scenario if you try and make a key for a 2GB LV that is
        //  composed almost entirely of invalid Unicode characters
        //
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
    BOOL                fAllNulls                   = fTrue;    // Assume all null, until proven otherwise
    BOOL                fNullFirstSeg               = fFalse;   // Assume no null first segment
    BOOL                fNullSeg                    = fFalse;   // Assume no null segments
    BOOL                fColumnTruncated            = fFalse;
    BOOL                fKeyTruncated               = fFalse;
    const BOOL          fPageInitiallyLatched       = Pcsr( pfucb )->FLatched();
    const BOOL          fTupleIndex                 = pidb->FTuples();
    const BOOL          fNestedTable                = pidb->FNestedTable();
    const BOOL          fDotNetGuid                 = pidb->FDotNetGuid();
    BOOL                fAllMVOutOfValues           = fTrue;
    BOOL                fPageLatched                = fPageInitiallyLatched;

    BYTE                *pbSegCur;                              // Pointer to current segment
    ULONG               cbKeyAvail;                             // Space remaining in key buffer
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

    //  page is only latched if we're coming from CreateIndex
    if ( fPageInitiallyLatched )
    {
        Assert( fOnRecord );
        Assert( locOnCurBM == pfucb->locLogical );
        Assert( !fRetrieveLVBeforeImage );
        Assert( !fRetrieveBasedOnRCE );
        Assert( pfucb->ppib->Level() > 0 );
    }

    //  check cbVarSegMac and set to key most plus one if no column
    //  truncation enabled.  This must be done for subsequent truncation
    //  checks.
    //
    const ULONG cbKeyMost       = pidb->CbKeyMost();

    Assert( pidb->CbVarSegMac() > 0 );
    Assert( pidb->CbVarSegMac() <= cbKeyMost );

    const ULONG cbVarSegMac     = ( cbKeyMost == pidb->CbVarSegMac() ?
                                        cbKeyMost+1 :
                                        pidb->CbVarSegMac() );
    Assert( cbVarSegMac > 0 );
    Assert( cbVarSegMac < cbKeyMost || cbVarSegMac == cbKeyMost+1 );

    const BOOL  fSortNullsHigh  = pidb->FSortNullsHigh();

    //  use stack buffers for the common case and allocate memory for the
    //  uncommon (long key) case
    //
    if ( cbDataMost > cbLVStack )
    {
        // This memory can become arbitrarily large (e.g. normalizing 
        // a key that ends in 1GB of trailing spaces when ignore space
        // is set. In that case we will keep retrieving bigger buffers
        // until we run out of memory.
        // FUTURE- set an absolute limit on the amount of text that
        // we will try to normalize (2x the key limit?).
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

    //  start at beginning of buffer, with max size remaining.
    //
    Assert( pkey->prefix.FNull() );
    pbSegCur = (BYTE *)pkey->suffix.Pv();
    cbKeyAvail = cbKeyMost;

    //  fRetrieveFromLVBuf flags whether or not we have to check in the LV buffer.
    //  We only check in the LV buffer if one exists, and if we are looking for the
    //  before-image (as specified by the parameter passed in).  Assert that this
    //  only occurs during a Replace.
    //
    Assert( fRetrieveBasedOnRCE
        || !fRetrieveLVBeforeImage
        || FFUCBReplacePrepared( pfucb ) );

    //  retrieve each segment in key description
    //
    if ( pidb->FTemplateIndex() )
    {
        Assert( pfcb->FDerivedTable() || pfcb->FTemplateTable() );
        if ( pfcb->FDerivedTable() )
        {
            // switch to template table
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

    //  if we're looking at a record, then make sure we're in
    //  a transaction to ensure read consistency
    if ( fOnRecord && 0 == pfucb->ppib->Level() )
    {
        Assert( !fPageInitiallyLatched );
        Assert( !Pcsr( pfucb )->FLatched() );

        //  UNDONE: only time we're not in a transaction is if we got
        //  here via ErrRECIGotoBookmark() -- can it be eliminated??
        Call( ErrDIRBeginTransaction( pfucb->ppib, 57637, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }

    //  if the idxsegConditional doesn't match return wrnFLDRecordNotPresentInIndex
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
        //  page should be latched iff key is retrieved from record
        if ( fOnRecord )
        {
            Assert( Pcsr( pfucb )->FLatched() );
        }
        else
        {
            Assert( !Pcsr( pfucb )->FLatched() );
        }
#endif  //  DEBUG

        //  get segment value:
        Assert( !lineRec.FNull() );
        if ( FCOLUMNIDTagged( columnidConditional ) )
        {
            err = ErrRECRetrieveTaggedColumn(
                        pfcbTable,
                        columnidConditional,
                        1,
                        lineRec,
                        &dataField );

            // no need to decompress LVs as we are just checking for null/non-null

            if( wrnRECUserDefinedDefault == err )
            {
                //  if this is a user-defined default we should execute the callback
                //  and let the callback possibly return JET_wrnColumnNull

                //  release the page and retrieve the appropriate copy of the user-defined-default
                if ( fPageLatched )
                {
                    Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );
                    CallS( ErrDIRRelease( pfucb ) );
                    fPageLatched = fFalse;
                }

                //  if we aren't on the record we'll point the copy buffer to the record and
                //  retrieve from there. save off the old value
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
                //  At this point we have the Indexing/Updating latch
                //  turn off the checks to avoid asserts
                COwner * pownerSaved;
                UtilAssertCriticalSectionCanDoIO();
                pownerSaved = Pcls()->pownerLockHead;
#endif  //  SYNC_DEADLOCK_DETECTION

                ULONG   cbActual    = 0;
                err = ErrRECCallback(
                    pfucb->ppib,
                    pfucb,
                    JET_cbtypUserDefinedDefaultValue,
                    columnidConditional,
                    NULL,   //  not actually interested in the data at this point, only interested in whether column is NULL
                    &cbActual,
                    columnidConditional );

#ifdef SYNC_DEADLOCK_DETECTION
                Assert( Pcls()->pownerLockHead == pownerSaved );
#endif  //  SYNC_DEADLOCK_DETECTION

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
        BYTE            *pbField = 0;                   // pointer to column data.
        ULONG           cbField = 0xffffffff;           // length of column data.
        DATA            dataField;
        INT             cbSeg       = 0;                // length of segment.
        const COLUMNID  columnid    = pidxseg->Columnid();
        const BOOL      fDescending = pidxseg->FDescending();
        const BOOL      fFixedField = FCOLUMNIDFixed( columnid );

        // No need to check column access -- since the column belongs
        // to an index, it MUST be available.  It can't be deleted, or
        // even versioned deleted.
        pfcb->EnterDML();
        field = *( pfcb->Ptdb()->Pfield( columnid ) );
        pfcb->LeaveDML();

        // No column which is part of an index can be encrypted
        Assert( !FFIELDEncrypted( field.ffield ) );

        //  Determine the offsets for the tuple indexing
        //  When retrieveing a long-value, we retrieve data starting at the offset,
        //  otherwise we have to index into the data. fTupleAdjusted keeps track of
        //  whether the adjustment has been done
        //  If fTupleAdjust is true then dataField.Pv() points to the ibTupleOffset
        //  offset in the data

        ULONG       ibTupleOffset       = 0;
        BOOL        fTupleAdjusted      = fFalse;

        //  only apply tuple transformation to first column of tuple index
        //
        if ( fTupleIndex && ( pidxseg == rgidxseg ) )
        {
            Assert( FRECTextColumn( field.coltyp ) || FRECBinaryColumn( field.coltyp) );

            //  assert no longer true now that we support configurable step
            //
            //  caller should have verified whether we've
            //  exceeded maximum allowable characters to
            //  index in this string
            //  Assert( ichOffset < pidb->IchTuplesToIndexMax() );

            //  normalise counts to account for Unicode
            ibTupleOffset       = ( usUniCodePage == field.cp ? ichOffset * 2 : ichOffset );
        }

        Assert( !FFIELDDeleted( field.ffield ) );
        Assert( JET_coltypNil != field.coltyp );
        Assert( !FCOLUMNIDVar( columnid ) || field.coltyp == JET_coltypBinary || field.coltyp == JET_coltypText );
        Assert( !FFIELDMultivalued( field.ffield ) || FCOLUMNIDTagged( columnid ) );

        if ( fOnRecord && !fPageLatched )
        {
            // Obtain latch only if we're retrieving from the record and
            // this is the first time through, or if we had to give
            // up the latch on a previous iteration.
            Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );
            Call( ErrDIRGet( pfucb ) );
            fPageLatched = fTrue;
        }

        //  page should be latched iff key is retrieved from record
        //
        if ( fOnRecord )
        {
            Assert( Pcsr( pfucb )->FLatched() );
        }
        else
        {
            Assert( !Pcsr( pfucb )->FLatched() );
        }

        //  get segment value:
        Assert( !lineRec.FNull() );
        if ( FCOLUMNIDTagged( columnid ) )
        {
            err = ErrRECRetrieveTaggedColumn(
                        pfcbTable,
                        columnid,
                        rgitag[iidxsegT],
                        lineRec,
                        &dataField );

            //  if all MV columns are NULL then fAllMVNulls is fTrue
            //
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

        //  with no space left in the key buffer we cannot insert any more
        //  normalised keys
        //
        if ( cbKeyAvail == 0 )
        {
            fKeyTruncated = fTrue;
            if ( fDisallowTruncation )
            {
                Call( ErrERRCheck( JET_errKeyTruncated ) );
            }

            //  check if column is NULL for tagged column support
            //
            if ( JET_wrnColumnNull == err )
            {
                //  cannot be all NULL and cannot be first NULL
                //  since key truncated.
                //
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
            //  release the page and retrieve the appropriate copy of the user-defined-default
            if ( fPageLatched )
            {
                Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );
                CallS( ErrDIRRelease( pfucb ) );
                fPageLatched = fFalse;
            }

            //  if we aren't on the record we'll point the copy buffer to the record and
            //  retrieve from there. save off the old value
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
            //  At this point we have the Indexing/Updating latch
            //  turn off the checks to avoid asserts
            COwner * pownerSaved;
            UtilAssertCriticalSectionCanDoIO();
            pownerSaved = Pcls()->pownerLockHead;
#endif  //  SYNC_DEADLOCK_DETECTION

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
#endif  //  SYNC_DEADLOCK_DETECTION

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
                    //  dataField now points to the correct offset in the long-value
                    //
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
                            fFalse, // columns which are part of index cannot be encrypted
                            pbLV,
                            cbDataMost,
                            &cbActual,
                            NULL,
                            NULL,
                            prce ) );

                // Verify all latches released after LV call.
                Assert( !Pcsr( pfucb )->FLatched() );

                dataField.SetPv( pbLV );
                dataField.SetCb( cbActual );
            }

            // First check in the LV buffer (if allowed).
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
                            fFalse, // columns which are part of index cannot be encrypted
                            pbLV,
                            cbDataMost,
                            &cbActual,
                            NULL,
                            NULL ) );

                // Verify all latches released after LV call.
                Assert( !Pcsr( pfucb )->FLatched() );

                dataField.SetPv( pbLV );
                dataField.SetCb( cbActual );
            }

            if( fTupleIndex && ( pidxseg == rgidxseg ) )
            {
                //  dataField now points to the correct offset in the long-value
                //
                fTupleAdjusted = fTrue;
            }

            // Trim returned value if necessary.
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
                //  fixup the data to take the tupleOffset into account
                const INT cbDelta = min( dataField.Cb(), (INT)ibTupleOffset );
                dataField.DeltaPv( cbDelta );
                dataField.DeltaCb( -cbDelta );

                // dataField now points to the correct offset in the long-value
                fTupleAdjusted = fTrue;
            }

            // Trim returned value if necessary.
            if ( (ULONG)dataField.Cb() > cbDataMost )
            {
                dataField.SetCb( cbDataMost );
            }

            err = JET_errSuccess;
        }
        else
        {
            CallSx( err, JET_wrnColumnNull );

            // Trim returned value if necessary.
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
            //  normalise counts to account for Unicode
            //  return wrnFLDOutOfTuples if we don't have enough data

            const BYTE *    pbFieldT    = (BYTE *)dataField.Pv();
            const ULONG     cbFieldT    = dataField.Cb();
            const ULONG     cbFieldMax  = ( usUniCodePage == field.cp ? pidb->CchTuplesLengthMax() * 2 : pidb->CchTuplesLengthMax() );
            const ULONG     cbFieldMin  = ( usUniCodePage == field.cp ? pidb->CchTuplesLengthMin() * 2 : pidb->CchTuplesLengthMin() );

            if ( JET_wrnColumnNull == err )
            {
                //  we didn't get any data

                dataField.SetPv( NULL );
                dataField.SetCb( 0 );
            }
            else if( fTupleAdjusted )
            {
                //  dataField.Pv() is already offset to the appropriate position
                //  dataField.Cb() is min( dataRemaining, cbDataMost )
            }
            else if( pbFieldT + ibTupleOffset <= pbFieldT + cbFieldT )
            {
                //  we have at least some of the data we want to index

                dataField.DeltaPv( ibTupleOffset );
                dataField.DeltaCb( 0 - ibTupleOffset );
            }
            else
            {
                //  the data we got is all beyond the end of the (non-adjusted) buffer

                dataField.SetPv( NULL );
                dataField.SetCb( 0 );
            }

            dataField.SetCb( min( dataField.Cb(), (INT)cbFieldMax ) );
            if( dataField.Cb() < (INT)cbFieldMin )
            {
                //  this means that we are at the end of the data and are trying to index a tuple that is too small (possibly 0 bytes)
                err = ErrERRCheck( wrnFLDOutOfTuples );
                goto HandleError;
            }
        }

        cbField = dataField.Cb();
        pbField = (BYTE *)dataField.Pv();

        //  segment transformation: check for null column or zero-length columns first
        //  err == JET_wrnColumnNull => Null column
        //  zero-length column otherwise,
        //
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
                // Only variable-length binary and text columns
                // can be zero-length.
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
            //  column is not null-valued: perform transformation
            //
            fAllNulls = fFalse;

            switch ( field.coltyp )
            {
                //  case-insensitive TEXT: convert to uppercase.
                //  If fixed, prefix with 0x7f;  else affix with 0x00
                //
                case JET_coltypText:
                case JET_coltypLongText:

                    // If not a template table, should not use template index for localization
                    Assert( pfucb->u.pfcb->FTemplateTable() || !pidb->FTemplateIndex() );
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

                //  BINARY data: if fixed, prefix with 0x7f;
                //  else break into chunks of 8 bytes, affixing each
                //  with 0x09, except for the last chunk, which is
                //  affixed with the number of bytes in the last chunk.
                //
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

        //  if we gave the normalization function the max amount of source data
        //  allowed AND the resulting key segment was smaller than the max key
        //  segment length AND we did not limit the size of the key segment due
        //  to available key space or cbVarSegMac or due to legacy source data
        //  truncation THEN we will ask for more data to fill out the rest of
        //  the key segment
        //
        //  NOTE:  if this index has a legacy max key size then the net effect
        //  of this check is to always truncate the source data used to compute
        //  the key to 255 bytes.  this is critical to maintaining backwards
        //  compatibility with indexes that were created with this max key size
        //
        if (    cbField == cbDataMost &&
                (ULONG) cbSeg < cbKeyMost &&
                (ULONG) cbSeg < cbKeyAvail &&
                (ULONG) cbSeg < pidb->CbVarSegMac() &&
                cbKeyMost != JET_cbKeyMost_OLD )
        {
            *pfNeedMoreData = fTrue;
        }

        //  if key has not already been truncated, then append
        //  normalized key segment.  If insufficient room in key
        //  for key segment, then set key truncated to fTrue.  No
        //  additional key data will be appended after this append.
        //
        if ( !fKeyTruncated )
        {
            //  if column truncated or insufficient room in key
            //  for key segment, then set key truncated to fTrue.
            //  Append variable size column keys only.
            //
            if ( fColumnTruncated )
            {
                fKeyTruncated = fTrue;
                if ( fDisallowTruncation )
                {
                    Call( ErrERRCheck( JET_errKeyTruncated ) );
                }

                Assert( FRECTextColumn( field.coltyp ) || FRECBinaryColumn( field.coltyp ) );

                // If truncating, in most cases, we fill up as much
                // key space as possible.  The only exception is
                // for non-fixed binary columns, which are
                // broken up into chunks.
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

                // Put as much as possible into the key space.
                cbSeg = cbKeyAvail;
            }

            //  if descending, flip all bits of transformed segment
            //
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

    }   // for

    //  if nested table index and all multi-value columns are NULL
    //  for an itagSequence > 1 then return wrnFLDOutOfKeys
    //
    if ( fNestedTable && fAllMVOutOfValues )
    {
        //  nested table always rolls over all columns together
        //  so if we are out of keys it is for the most significant column
        iidxsegT = 0;
        err = ErrERRCheck( wrnFLDOutOfKeys );
        goto HandleError;
    }
    
    //  compute length of key and return error code
    //
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
    //  return index into key of NULL multi-valued column instance
    //  to allow correct sequencing to next key.
    //
    if ( wrnFLDOutOfKeys == err )
    {
        *piidxseg = iidxsegT;
    }
    else
    {
        //  return iidxsegNoRollover to indicate no overflow of multi-valued column has occurred
        //
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
        //  No updates performed, so force success.
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

        //  pad rest of key space with 0xff
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

    Assert( plineNorm->Cb() > 0 );      //  must be at least a prefix byte
    Assert( cbAvailWithSuffix > 0 );                    //  Always have a suffix byte reserved for limit purposes
    Assert( cbAvailWithSuffix <= cbLimitKeyMostMost );
    Assert( (ULONG)plineNorm->Cb() < cbAvailWithSuffix );

    if ( plineNorm->Cb() < 2 )
    {
        //  cannot effect partial column limit,
        //  so set full column limit instead
        FLDISetFullColumnLimit( plineNorm, cbAvailWithSuffix, fNeedSentinel );
        return;
    }

    if ( usUniCodePage == cp )
    {
        const BYTE  bUnicodeDelimiter   = BYTE( bUnicodeSortKeySentinel ^ ( fDescending ? 0xff : 0x00 ) );

        //  find end of base char weight and truncate key
        //  Append 0xff as first byte of next character as maximum
        //  possible value.
        //
        for ( ibT = 1;      //  skip header byte
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
            // Strip off null-terminator.
            ibT--;
            Assert( plineNorm->Cb() >= 1 );
        }
        else
        {
            //  must be at the end of key space
            Assert( (ULONG)plineNorm->Cb() == ibSuffix );
        }

        Assert( ibT <= ibSuffix );
    }

    ibT = min( ibT, ibSuffix );
    if ( fNeedSentinel )
    {
        //  starting at the delimiter, fill the rest of key
        //  space with the sentinel
        memset(
            pbNorm + ibT,
            bSentinel,
            ibSuffix - ibT + 1 );
        plineNorm->SetCb( cbAvailWithSuffix );
    }
    else
    {
        //  just strip off delimeter (or suffix byte if we spilled over it)
        plineNorm->SetCb( ibT );
    }
}

JETUNITTEST( NORM, PartialLimitTest )
{
    BYTE buf[12];
    DATA data;
    INT i;
    // output for "8b 73", locale zh-CN
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

//  try to set partial column limit, but set full column limit if can't
INLINE VOID FLDITrySetPartialColumnLimitOnBinaryColumn(
    DATA            * const plineNorm,
    const ULONG     cbAvailWithSuffix,
    const ULONG     ibBinaryColumnDelimiter,
    const BOOL      fNeedSentinel )
{
    Assert( plineNorm->Cb() > 0 );      //  must be at least a prefix byte
    Assert( cbAvailWithSuffix > 0 );                    //  Always have a suffix byte reserved for limit purposes
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
        //  cannot effect partial column limit,
        //  so set full column limit instead
        FLDISetFullColumnLimit( plineNorm, cbAvailWithSuffix, fNeedSentinel );
    }
    else if ( fNeedSentinel )
    {
        //  starting at the delimiter, fill up to the end
        //  of the chunk with the sentinel
        //  UNDONE (SOMEONE): go one past just to be safe, but I
        //  couldn't prove whether or not it is really needed
        plineNorm->DeltaCb( 1 );
        memset(
            (BYTE *)plineNorm->Pv() + ibBinaryColumnDelimiter,
            bSentinel,
            plineNorm->Cb() - ibBinaryColumnDelimiter );
    }
    else
    {
        //  just strip off delimiting bytes
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

    //  check cbVarSegMac and set to key most plus one if no column
    //  truncation enabled.  This must be done for subsequent truncation
    //  checks.
    //
    const ULONG cbKeyMost       = pidb->CbKeyMost();

    Assert( pidb->CbVarSegMac() > 0 );
    Assert( pidb->CbVarSegMac() <= cbKeyMost );

    const ULONG cbVarSegMac     = ( cbKeyMost == pidb->CbVarSegMac() ?
                                        cbKeyMost+1 :
                                        pidb->CbVarSegMac() );
    Assert( cbVarSegMac > 0 );
    Assert( cbVarSegMac < cbKeyMost || cbVarSegMac == cbKeyMost+1 );

    const BOOL  fSortNullsHigh  = pidb->FSortNullsHigh();


    //  check for null or zero-length column first
    //  plineColumn == NULL implies null-column,
    //  zero-length otherwise
    //
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

            // Only variable binary and text columns can be zero-length.
            //
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
            //  case-insensetive TEXT:  convert to uppercase.
            //  If fixed, prefix with 0x7f;  else affix with 0x00
            //
            case JET_coltypText:
            case JET_coltypLongText:
            {
                if ( fTupleField )
                {
                    Assert( FRECTextColumn( coltyp ) || FRECBinaryColumn( coltyp ) );

                    //  normalise counts to account for Unicode
                    Assert( usUniCodePage != cp || cbColumn % 2 == 0 );
                    const ULONG     chColumn    = ( usUniCodePage == cp ? cbColumn / 2 : cbColumn );
                    const ULONG     cbColumnMax = ( usUniCodePage == cp ? pidb->CchTuplesLengthMax() * 2 : pidb->CchTuplesLengthMax() );

                    //  if data is not large enough, bail out
                    if ( chColumn < pidb->CchTuplesLengthMin() )
                    {
                        return ErrERRCheck( JET_errIndexTuplesKeyTooSmall );
                    }
                    else
                    {
                        cbColumn = min( cbColumn, (INT)cbColumnMax );
                    }
                }

                // If not a template table, should not use template index for localization
                Assert( pfucb->u.pfcb->FTemplateTable() || !pidb->FTemplateIndex() );
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

            //  BINARY data: if fixed, prefix with 0x7f;
            //  else break into chunks of 8 bytes, affixing each
            //  with 0x09, except for the last chunk, which is
            //  affixed with the number of bytes in the last chunk.
            //
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
                        fTrue /* data is passed by user, in machine format */);
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

    //  string and substring limit key support
    //  NOTE:  The difference between the two is that StrLimit appends 0xff to the end of
    //  key space for any column type, while SubStrLimit only works on text columns and
    //  will strip the trailing null terminator of the string before appending 0xff to the
    //  end of key space.
    //
    Assert( (ULONG)plineNorm->Cb() < cbAvail + 1 ); //  should always be room for suffix byte
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
                        cbAvail + 1,        //  +1 for suffix byte
                        fDescending,
                        grbit & JET_bitPartialColumnEndLimit,
                        cp );
            }
            else
            {
                FLDITrySetPartialColumnLimitOnBinaryColumn(
                        plineNorm,
                        cbAvail + 1,        //  +1 for suffix byte
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
                        cbAvail + 1,        //  +1 for suffix byte
                        fDescending,
                        fTrue,
                        cp );
                KSSetLimit( pfucb );
            }
            else if ( grbit & JET_bitStrLimit )
            {
                FLDITrySetPartialColumnLimitOnBinaryColumn(
                        plineNorm,
                        cbAvail + 1,        //  +1 for suffix byte
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

    //  there must be more than just this column in the index
    Assert( pidb->FHasPlaceholderColumn() );
    Assert( pidb->Cidxseg() > 1 );
    Assert( pidb->FPrimary() );

    pfcbTable->EnterDML();

    const TDB       * const ptdb    = pfcbTable->Ptdb();
    const IDXSEG    idxseg          = PidxsegIDBGetIdxSeg( pidb, ptdb )[0];
    const BOOL      fDescending     = idxseg.FDescending();

#ifdef DEBUG
    //  HACK: placeholder column MUST be fixed bitfield
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

    //  get pidb
    //
    pfcbTable = pfucbTable->u.pfcb;
    if ( FFUCBIndex( pfucbTable ) )
    {
        if ( pfucbTable->pfucbCurIndex != pfucbNil )
        {
            Assert( pfucb == pfucbTable->pfucbCurIndex );
            Assert( pfucb->u.pfcb->FTypeSecondaryIndex() );
            pidb = pfucb->u.pfcb->Pidb();
            fInitialIndex = pfucb->u.pfcb->FInitialIndex();
            if ( pfucb->u.pfcb->FDerivedIndex() && !pidb->FIDBOwnedByFCB() )
            {
                // If secondary index is inherited, use FCB of template table.
                Assert( pidb->FTemplateIndex() );
                Assert( pfcbTable->Ptdb() != ptdbNil );
                pfcbTable = pfcbTable->Ptdb()->PfcbTemplateTable();
                Assert( pfcbNil != pfcbTable );
            }
        }
        else
        {
            BOOL    fPrimaryIndexTemplate   = fFalse;
            pidb = pfcbTable->Pidb();

            Assert( pfcbTable->FPrimaryIndex() );
            if ( pfcbTable->FDerivedTable() && ( pidb == NULL || !pidb->FIDBOwnedByFCB() ) )
            {
                Assert( pfcbTable->Ptdb() != ptdbNil );
                Assert( pfcbTable->Ptdb()->PfcbTemplateTable() != pfcbNil );
                if ( !pfcbTable->Ptdb()->PfcbTemplateTable()->FSequentialIndex() )
                {
                    //  if template table has a primary index, we must have inherited it,
                    //  so use FCB of template table instead.
                    //
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
                    //  since the primary index can't be deleted,
                    //  no need to check visibility
                    //
                    pidb = pfcbTable->Pidb();
                    fInitialIndex = fTrue;
                }
                else
                {
                    pfcbTable->EnterDML();

                    pidb = pfcbTable->Pidb();

                    //  must check whether we have a primary or sequential index and
                    //  whether we have visibility on it
                    //
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

    //  allocate our normalized segment buffer
    //
    if ( pidb == pidbNil )  //  sequential index
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

    //  the amount of source data used to compute the normalized key segment
    //  may need to be truncated to support legacy index formats.  we used to
    //  have a fixed max size for our key length at 255 bytes.  when that was
    //  the case, we would always truncate the source data to 255 bytes prior
    //  to normalization.  we did this because we believed that no data beyond
    //  255 bytes could possibly affect the outcome of the normalization.  we
    //  have since learned that this is false.  there is no guarantee that one
    //  Unicode char will result in exactly one sort weight in the output key.
    //  as a result, we must forever continue to truncate the source data in
    //  this manner for indices that could have been created when this old code
    //  was in effect.  we can't know this easily, so we will claim that any
    //  index that uses a max key length of 255 bytes must follow this
    //  truncation rule
    //
    lineKeySeg.SetPv( pbKeySeg );
    if ( cbKeyMost == JET_cbKeyMost_OLD )
    {
        lineKeySeg.SetCb( min( cbKeySeg, JET_cbKeyMost_OLD ) );
    }
    else
    {
        lineKeySeg.SetCb( cbKeySeg );
    }

    //  allocate key buffer if needed
    //
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

    //  hijack table's search key buffer

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

    //  if key is already normalized, then copy directly to
    //  key buffer and return.
    //
    if ( grbit & JET_bitNormalizedKey )
    {
        //  if given information is longer than largest key for index then given data invalid
        //
        const ULONG cbLimitKeyMost = KEY::CbLimitKeyMost( (USHORT)cbKeyMost );
        if ( cbKeySeg > cbLimitKeyMost &&
            cbKeySeg > JET_cbKeyMost )
        {
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }
        cbKeySeg = min( cbLimitKeyMost, cbKeySeg );

        //  ensure previous key is wiped out
        //
        KSReset( pfucb );

        //  set key segment counter to any value
        //  regardless of the number of key segments.
        //
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

    //  start new key if requested
    //
    else if ( grbit & JET_bitNewKey )
    {
        //  ensure previous key is wiped out
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
        //  HACK: first column is placeholder
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

        //  check that length of key segment matches fixed column length
        //
        Assert( pfield->cbMaxLen <= JET_cbColumnMost );
        if ( cbKeySeg > 0 && cbKeySeg != pfield->cbMaxLen )
        {
            //  if column is fixed text and buffer size is less
            //  than fixed size then pad with spaces.
            //
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
                //  partial column limits can only be done
                //  on text columns and non-fixed binary columns
                //  (because they are the only ones that have
                //  delimiters that need to be stripped off)
                //
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
                        //  report unexpected error
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

    //  increment segment counter
    //
    pfucb->cColumnsInSearchKey++;

    //  UNDONE: normalized segment should already be sized properly to ensure we
    //  don't overrun key buffer.  Assert this, but leave the check in for now
    //  just in case.
    Assert( pfucb->dataSearchKey.Cb() + lineNormSeg.Cb() <= KEY::CbLimitKeyMost( pidb->CbKeyMost() ) );
    if ( pfucb->dataSearchKey.Cb() + lineNormSeg.Cb() > KEY::CbLimitKeyMost( pidb->CbKeyMost() ) )
    {
        lineNormSeg.SetCb( KEY::CbLimitKeyMost( pidb->CbKeyMost() ) - pfucb->dataSearchKey.Cb() );
        //  no warning returned when key exceeds most size
        //
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
    ULONG           cbField;                    // Length of column data.

    //  negative column id means descending in the key
    //
    const BOOL      fDescending     = pidxseg->FDescending();
    const BYTE      bMask           = BYTE( fDescending ? ~BYTE( 0 ) : BYTE( 0 ) );
    const WORD      wMask           = WORD( fDescending ? ~WORD( 0 ) : WORD( 0 ) );
    const DWORD     dwMask          = DWORD( fDescending ? ~DWORD( 0 ) : DWORD( 0 ) );
    const QWORD     qwMask          = QWORD( fDescending ? ~QWORD( 0 ) : QWORD( 0 ) );

    *pfNull = fFalse;

    Assert( coltyp != JET_coltypNil );

    BYTE        * const pbDataColumn = (BYTE *)plineColumn->Pv();       //  efficiency variable

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
                    //  .Net Guid order should match string order of standard GUID structure comparison:
                    //  DWORD, WORD, WORD, BYTE ARRAY[8]
                    //  order[0..15] = input[3, 2, 1, 0, 5, 4, 7, 6, 10, 11, 12, 13, 14, 15]
                    //
                    cbField = 16;
                    Assert( cbField <= SIZE_T( pbKeyMax - pbKey ) );    // wouldn't call this function if key possibly truncated
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
                    //  GUID order should match string order of standard GUID to string conversion:
                    //  order[0..15] = input[10, 11, 12, 13, 14, 15, 8, 9, 6, 7, 4, 5, 0, 1, 2, 3]
                    //
                    cbField = 16;
                    Assert( cbField <= SIZE_T( pbKeyMax - pbKey ) );    // wouldn't call this function if key possibly truncated
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
            //  Can only de-normalise text column for the purpose of skipping
            //  over it (since normalisation doesn't alter the length of
            //  the string).  Can't return the string to the caller because
            //  we have no way of restoring the original case.

            //  FALL THROUGH (fixed text handled the same as fixed binary,
            //  non-fixed text special-cased below):

        case JET_coltypBinary:
        case JET_coltypLongBinary:
            if ( fDescending )
            {
                if ( fFixedField )
                {
                    if ( *pbKey++ == (BYTE)~bPrefixData )
                    {
                        cbField = pfield->cbMaxLen;
                        Assert( cbField <= SIZE_T( pbKeyMax - pbKey ) );    // wouldn't call this function if key possibly truncated
                        plineColumn->SetCb( cbField );
                        for ( ULONG ibT = 0; ibT < cbField; ibT++ )
                        {
                            Assert( pbDataColumn == (BYTE *)plineColumn->Pv() );
                            pbDataColumn[ibT] = (BYTE)~*pbKey++;
                        }
                    }
//                      // zero-length strings -- only for non-fixed columns
//                      //
//                      else if ( pbKey[-1] == (BYTE)~bPrefixZeroLength )
//                          {
//                          plineColumn->cb = 0;
//                          Assert( FRECTextColumn( coltyp ) );
//                          }
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
                                //  we are guaranteed to hit the NULL terminator, because
                                //  we never call this function if we hit the end
                                //  of key space
                                //
                                //  NOTE:  there is a bug in Win2k3 where embedded NULL terminators can appear
                                //  in Unicode sort keys.  These will be fixed in
                                //  Vista as part of a major revision change that will rebuild all Unicode indices.
                                //  Until then, we can work around their existence by first scanning for the
                                //  sort key sentinel and then scanning for the sort key terminator
                                //
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
                                pbKey++;    // skip null-terminator
                            }
                            else
                            {
                                //  we are guaranteed to hit the NULL terminator, because
                                //  we never call this function if we hit the end
                                //  of key space
                                //
                                for ( ; *pbKey != (BYTE)~bASCIISortKeyTerminator; cbField++)
                                {
                                    Assert( pbKey < pbKeyMax );
                                    pbDataColumn[cbField] = (BYTE)~*pbKey++;
                                }
                                pbKey++;    // skip null-terminator
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
                        Assert( cbField <= SIZE_T( pbKeyMax - pbKey ) );    // wouldn't call this function if key possibly truncated
                        plineColumn->SetCb( cbField );
                        UtilMemCpy( plineColumn->Pv(), pbKey, cbField );
                        pbKey += cbField;
                    }
//                      // zero-length strings -- only for non-fixed columns
//                      //
//                      else if ( pbKey[-1] == bPrefixZeroLength )
//                          {
//                          Assert( FRECTextColumn( coltyp ) );
//                          plineColumn->SetCb( 0 );
//                          }
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
                                //  we are guaranteed to hit the NULL terminator, because
                                //  we never call this function if we hit the end
                                //  of key space
                                //
                                //  NOTE:  there is a bug in Win2k3 where embedded NULL terminators can appear
                                //  in Unicode sort keys.  These will be fixed in
                                //  Vista as part of a major revision change that will rebuild all Unicode indices.
                                //  Until then, we can work around their existence by first scanning for the
                                //  sort key sentinel and then scanning for the sort key terminator
                                //
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
                                pbKey++;    // skip null-terminator
                            }
                            else
                            {
                                //  we are guaranteed to hit the NULL terminator, because
                                //  we never call this function if we hit the end
                                //  of key space
                                //
                                for ( ; *pbKey != bASCIISortKeyTerminator; cbField++ )
                                {
                                    Assert( pbKey < pbKeyMax );
                                    pbDataColumn[cbField] = (BYTE)*pbKey++;
                                }
                                pbKey++;    // skip null-terminator
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

//+API
//  ErrRECIRetrieveColumnFromKey
//  ========================================================================
//  ErrRECIRetrieveColumnFromKey(
//      TDB *ptdb,              // IN    column info for index
//      IDB *pidb,              // IN    IDB of index defining key
//      KEY *pkey,              // IN    key in normalized form
//      DATA *plineColumn );    // OUT   receives value list
//
//  PARAMETERS
//      ptdb            column info for index
//      pidb            IDB of index defining key
//      pkey            key in normalized form
//      plineColumn     plineColumn->pv must point to a buffer large
//                      enough to hold the denormalized column.  A buffer
//                      of cbKeyMost bytes is sufficient.
//
//  RETURNS     JET_errSuccess
//
//-
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
    BYTE                *pbKeyAlloc     = NULL;     // key allocation
    BYTE                *pbKey          = NULL;     // runs through key bytes
    const BYTE          *pbKeyMax       = NULL;     // end of key
    
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

        //  get the field for this index segment
        //
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
#endif  //  DEBUG

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

    //  retrieve key from key buffer
    //
    if ( grbit & JET_bitRetrieveCopy )
    {
        if ( pfucb->pfucbCurIndex != pfucbNil )
            pfucb = pfucb->pfucbCurIndex;

        //  UNDONE: support JET_bitRetrieveCopy for inserted record
        //          by creating key on the fly.
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

        //  BUGBUG:  why on earth can't we return JET_wrnBufferTruncated from
        //  BUGBUG:  JetRetrieveKey when used with JET_bitRetrieveCopy?
        //
        //  I would fix this, but I am scared of app compat issues
        return JET_errSuccess;
    }

    //  retrieve current index value
    //
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
        pfcbIdx = (FCB *)pfucb->u.pscb; // first element of an SCB is an FCB
        Assert( pfcbIdx != pfcbNil );
    }

    //  set err to JET_errSuccess.
    //
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
                           0: // no key left already, after copying the prefix
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

    //  retrieve bookmark
    //
    CallR( ErrDIRGetBookmark( pfucb, &pbm ) );
    Assert( !Pcsr( pfucb )->FLatched() );

    Assert( pbm->key.prefix.FNull() );
    Assert( pbm->data.FNull() );

    cb = pbm->key.Cb();

    //  set return values
    //
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

    //  retrieve bookmark
    //
    CallR( ErrDIRGet( pfucbIdx ) );
    Assert( Pcsr( pfucbIdx )->FLatched() );
    Assert( !pfucbIdx->kdfCurr.key.FNull() );

    //  set secondary index key return value
    //
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

    //  set primary bookmark return value
    //
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

