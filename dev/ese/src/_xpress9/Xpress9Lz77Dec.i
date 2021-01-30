// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
static
void
Xpress9Lz77Dec (
    XPRESS9_STATUS  *pStatus,
    LZ77_DECODER    *pDecoder
)
{

#if LZ77_MTF >= 2
    xint            iMtfLastPtr     = pDecoder->m_DecodeData.m_Mtf.m_iMtfLastPtr;
    xint            iMtfOffset0     = pDecoder->m_DecodeData.m_Mtf.m_iMtfOffset[0]; 
    xint            iMtfOffset1     = pDecoder->m_DecodeData.m_Mtf.m_iMtfOffset[1];
#endif 
#if LZ77_MTF >= 4
    xint            iMtfOffset2     = pDecoder->m_DecodeData.m_Mtf.m_iMtfOffset[2];
    xint            iMtfOffset3     = pDecoder->m_DecodeData.m_Mtf.m_iMtfOffset[3];
#endif 
    HUFFMAN_DECODE_TABLE_ENTRY *piShortSymbolRoot   = pDecoder->m_DecodeData.m_piShortSymbolRoot;
    HUFFMAN_DECODE_TABLE_ENTRY *piLongLengthRoot    = pDecoder->m_DecodeData.m_piLongLengthRoot;
    uxint   uDecodePosition         = pDecoder->m_DecodeData.m_uDecodePosition;
    uxint   uStopPosition           = pDecoder->m_DecodeData.m_uStopPosition;
    uxint   uEndOfBuffer            = pDecoder->m_DecodeData.m_uEndOfBuffer;
    UInt8  *pDst                    = pDecoder->m_BufferData.m_pBufferData;
    UInt8  *pSrc;
    uxint   uSymbol;
    uxint   uLength;
    uxint   uPositionLast;
    uxint   uBits;
    xint    iOffset;
    BIO_DECLARE();


    RETAIL_ASSERT (
        uDecodePosition < uStopPosition &&
        uStopPosition <= uEndOfBuffer,
        "uDecodePosition=%Iu uStopPosition=%Iu uEndOfBuffer=%Iu",
        uDecodePosition,
        uStopPosition,
        uEndOfBuffer
    );

    RETAIL_ASSERT (
        pDecoder->m_DecodeData.m_uMtfEntryCount == LZ77_MTF,
        "m_uMtfEntryCount=%Iu expected=%u",
        pDecoder->m_DecodeData.m_uMtfEntryCount,
        LZ77_MTF
    );

#if LZ77_MTF > 0
    RETAIL_ASSERT (
        pDecoder->m_DecodeData.m_uMtfMinMatchLength == LZ77_MTF_MIN_MATCH_LENGTH,
        "m_uMtfMinMatchLength=%Iu expected=%u",
        pDecoder->m_DecodeData.m_uMtfMinMatchLength,
        LZ77_MTF_MIN_MATCH_LENGTH
    );
#endif 

    RETAIL_ASSERT (
        pDecoder->m_DecodeData.m_uPtrMinMatchLength == LZ77_PTR_MIN_MATCH_LENGTH,
        "m_uPtrMinMatchLength=%Iu expected=%u",
        pDecoder->m_DecodeData.m_uPtrMinMatchLength,
        LZ77_PTR_MIN_MATCH_LENGTH
    );

    BIO_STATE_ENTER (pDecoder->m_DecodeData.m_BioState);

    if (pDecoder->m_DecodeData.m_Tail.m_uLength != 0)
    {
        uLength = pDecoder->m_DecodeData.m_Tail.m_uLength;
        iOffset = pDecoder->m_DecodeData.m_Tail.m_iOffset;

        if ((uxint) (-iOffset) > uDecodePosition)
        {
            SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "uDecodePosition=%Iu uBufferOffset=%I64u iOffset=%Iu", uDecodePosition, pDecoder->m_DecodeData.m_uBufferOffset, -iOffset);
            goto Failure;
        }

        uPositionLast = uDecodePosition + uLength;
        if (uPositionLast > uEndOfBuffer)
        {
            goto EndOfBuffer;
        }

        pSrc = pDst + iOffset;
        do
        {
            pDst[uDecodePosition] = pSrc[uDecodePosition];
            uDecodePosition += 1;
        }
        while (uDecodePosition < uPositionLast);

        if (uDecodePosition >= uStopPosition)
        {
            goto DoneNoTail;
        }
    }

    do
    {
        for (;;)
        {
            HUFFMAN_DECODE_SYMBOL (uSymbol, piShortSymbolRoot, LZ77_SHORT_SYMBOL_DECODE_ROOT_BITS, LZ77_SHORT_SYMBOL_DECODE_TAIL_BITS);
            if (((xint) (uSymbol -= 256)) >= 0)
            {
                break;
            }
            TRACE ("DEC %Iu LIT %Iu", uDecodePosition, uSymbol + 256);
            pDst[uDecodePosition] = (UInt8) uSymbol;
            uDecodePosition += 1;
#if LZ77_MTF > 0
            iMtfLastPtr = 0;
#endif 
            if (uDecodePosition >= uStopPosition)
            {
                goto DoneNoTail;
            }
        }

        uLength = (uxint) (uSymbol & (LZ77_MAX_SHORT_LENGTH - 1));
        uSymbol >>= LZ77_MAX_SHORT_LENGTH_LOG;
        if (uLength == LZ77_MAX_SHORT_LENGTH - 1)
        {
            HUFFMAN_DECODE_SYMBOL (uLength, piLongLengthRoot, LZ77_LONG_LENGTH_DECODE_ROOT_BITS, LZ77_LONG_LENGTH_DECODE_TAIL_BITS);

            if (uLength >= LZ77_MAX_LONG_LENGTH)
            {
                uBits = uLength - LZ77_MAX_LONG_LENGTH;
                BIORD_LOAD (uLength, uBits);
                uLength += POWER2 (uBits);
                uLength += LZ77_MAX_LONG_LENGTH - 1;
            }

            uLength += LZ77_MAX_SHORT_LENGTH - 1;
        }

#if LZ77_MTF > 0
        if (uSymbol < LZ77_MTF)
        {
            uLength += LZ77_MTF_MIN_MATCH_LENGTH;

            TRACE ("DEC %Iu MTF %Iu %Id", uDecodePosition, uLength, uSymbol);

            if (iMtfLastPtr)
            {
#if LZ77_MTF <= 2
                if (uSymbol != 0)
                {
                    SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "uDecodePosition=%Iu", uDecodePosition);
                    goto Failure;
                }
                iOffset     = iMtfOffset1;
#else
                if (uSymbol == 0)
                {
                    iOffset = iMtfOffset1;
                }
                else if (uSymbol == 1)
                {
                    iOffset     = iMtfOffset2;
                    iMtfOffset2 = iMtfOffset1;
                }
                else if (uSymbol == 2)
                {
                    iOffset     = iMtfOffset3;
                    iMtfOffset3 = iMtfOffset2;
                    iMtfOffset2 = iMtfOffset1;
                }
                else
                {
                    SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "uDecodePosition=%Iu", uDecodePosition);
                    goto Failure;
                }
#endif 
                iMtfOffset1 = iMtfOffset0;
                iMtfOffset0 = iOffset;
            }
            else
            {
                if (uSymbol == 0)
                {
                    iOffset = iMtfOffset0;
                }
                else
#if LZ77_MTF > 2
                if (uSymbol == 1)
#endif 
                {
                    iOffset     = iMtfOffset1;
                    iMtfOffset1 = iMtfOffset0;
                    iMtfOffset0 = iOffset;
                }
#if LZ77_MTF > 2
                else if (uSymbol == 2)
                {
                    iOffset     = iMtfOffset2;
                    iMtfOffset2 = iMtfOffset1;
                    iMtfOffset1 = iMtfOffset0;
                    iMtfOffset0 = iOffset;
                }
                else
                {
                    iOffset     = iMtfOffset3;
                    iMtfOffset3 = iMtfOffset2;
                    iMtfOffset2 = iMtfOffset1;
                    iMtfOffset1 = iMtfOffset0;
                    iMtfOffset0 = iOffset;
                }
#endif 
            }
        }
        else
#endif 
        {
            uLength += LZ77_PTR_MIN_MATCH_LENGTH;

            uSymbol -= LZ77_MTF;
            BIORD_LOAD (iOffset, uSymbol);
            iOffset += POWER2 (uSymbol);

            TRACE ("DEC %Iu PTR %Iu %Id", uDecodePosition, uLength, -iOffset);

#if LZ77_MTF > 2
            iMtfOffset3 = iMtfOffset2;
            iMtfOffset2 = iMtfOffset1;
#endif 
#if LZ77_MTF > 0
            iMtfOffset1 = iMtfOffset0;
            iMtfOffset0 = iOffset;
#endif 
        }

        if (((uxint) iOffset) > uDecodePosition)
        {
            SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "uDecodePosition=%Iu uBufferOffset=%I64u iOffset=%Iu", uDecodePosition, pDecoder->m_DecodeData.m_uBufferOffset, iOffset);
            goto Failure;
        }
        iOffset = -iOffset;

#if LZ77_MTF > 0
        iMtfLastPtr = 1;
#endif 

        uPositionLast = uDecodePosition + uLength;
        if (uPositionLast > uEndOfBuffer)
        {
            goto EndOfBuffer;
        }

        pSrc = pDst + iOffset;

#if LZ77_PTR_MIN_MATCH_LENGTH < 2
#error Invalid LZ77_PTR_MIN_MATCH_LENGTH
#endif
#if LZ77_MTF > 0
#if LZ77_MTF_MIN_MATCH_LENGTH < 2
#error Invalid LZ77_MTF_MIN_MATCH_LENGTH
#endif
#endif

        pDst[uDecodePosition] = pSrc[uDecodePosition];
        pDst[uDecodePosition+1] = pSrc[uDecodePosition+1];
        uDecodePosition += 2;

        while (uDecodePosition < uPositionLast)
        {
            pDst[uDecodePosition] = pSrc[uDecodePosition];
            uDecodePosition += 1;
        }
    }
    while (uDecodePosition < uStopPosition);

DoneNoTail:
    pDecoder->m_DecodeData.m_Tail.m_uLength = 0;

Done:
    pDecoder->m_DecodeData.m_uDecodePosition = uDecodePosition;

    BIO_STATE_LEAVE (pDecoder->m_DecodeData.m_BioState);

#if LZ77_MTF >= 2
    pDecoder->m_DecodeData.m_Mtf.m_iMtfLastPtr   = iMtfLastPtr;
    pDecoder->m_DecodeData.m_Mtf.m_iMtfOffset[0] = iMtfOffset0;
    pDecoder->m_DecodeData.m_Mtf.m_iMtfOffset[1] = iMtfOffset1;
#endif 
#if LZ77_MTF >= 4
    pDecoder->m_DecodeData.m_Mtf.m_iMtfOffset[2] = iMtfOffset2;
    pDecoder->m_DecodeData.m_Mtf.m_iMtfOffset[3] = iMtfOffset3;
#endif 

    return;



EndOfBuffer:
    pSrc = pDst + iOffset;
    do
    {
        pDst[uDecodePosition] = pSrc[uDecodePosition];
        uDecodePosition += 1;
    }
    while (uDecodePosition < uEndOfBuffer);

    pDecoder->m_DecodeData.m_Tail.m_uLength = uPositionLast - uEndOfBuffer;
    pDecoder->m_DecodeData.m_Tail.m_iOffset = iOffset;

    goto Done;


Failure:;
    return;
}

#undef Xpress9Lz77Dec
#undef LZ77_MTF
#undef LZ77_MTF_MIN_MATCH_LENGTH
#undef LZ77_PTR_MIN_MATCH_LENGTH
