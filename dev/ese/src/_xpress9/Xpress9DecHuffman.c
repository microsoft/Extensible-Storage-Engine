// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "Xpress9Internal.h"

static
HUFFMAN_DECODE_NODE *
HuffmanDecodeSortSymbols (
    XPRESS9_STATUS         *pStatus,
    const UInt8            *puBitLengthTable,
    uxint                   uAlphabetSize,
    HUFFMAN_DECODE_NODE   **ppTemp,
    uxint                   uBitLengthCount[HUFFMAN_CODEWORD_LENGTH_LIMIT],
    uxint                   *puSymbolCount
)
{
    struct
    {
        HUFFMAN_DECODE_NODE   *m_pHead;
        HUFFMAN_DECODE_NODE  **m_ppLast;
    }
    Chain[HUFFMAN_CODEWORD_LENGTH_LIMIT];
    HUFFMAN_DECODE_NODE *pTemp = *ppTemp;
    HUFFMAN_DECODE_NODE *pLeaf;
    uxint i;
    uxint n;

    *puSymbolCount = 0;

    for (i = 0; i < HUFFMAN_CODEWORD_LENGTH_LIMIT; ++i)
    {
        Chain[i].m_ppLast = &Chain[i].m_pHead;
    }
    memset (uBitLengthCount, 0, sizeof (uBitLengthCount[0]) * HUFFMAN_CODEWORD_LENGTH_LIMIT);

    i = uAlphabetSize;
    do
    {
        --i;
        if (puBitLengthTable[i] != 0)
        {
            n = puBitLengthTable[i];
            RETAIL_ASSERT (n < HUFFMAN_CODEWORD_LENGTH_LIMIT, "ExcessivelyLargeBitLength: symbol=%Iu bits=%Iu", i, n);

            pTemp->m_uSymbol = (UInt16) i;
            pTemp->m_uBits   = (UInt16) n;
            uBitLengthCount[n] += 1;

            *(Chain[n].m_ppLast) = pTemp;
            Chain[n].m_ppLast = &pTemp->m_pNext;
            pTemp += 1;
            (*puSymbolCount) += 1;
        }
    }
    while (i != 0);
    pLeaf = NULL;
    for (i = 0; i < HUFFMAN_CODEWORD_LENGTH_LIMIT; ++i)
    {
        *(Chain[i].m_ppLast) = pLeaf;
        pLeaf = Chain[i].m_pHead;
    }

    RETAIL_ASSERT (pLeaf != NULL, "");

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT
    {
        HUFFMAN_DECODE_NODE *pNode;
        for (pNode = pLeaf; pNode->m_pNext != NULL; pNode = pNode->m_pNext)
        {
            RETAIL_ASSERT (
                pNode->m_uBits > pNode->m_pNext->m_uBits || (pNode->m_uBits == pNode->m_pNext->m_uBits && pNode->m_uSymbol > pNode->m_pNext->m_uSymbol),
                "SortOrderViolation: Current=(%u %u) Next=(%u %u)",
                pNode->m_uBits,
                pNode->m_uSymbol,
                pNode->m_pNext->m_uBits,
                pNode->m_pNext->m_uSymbol
            );
        }
    }
#endif 

    *ppTemp = pTemp;
    return (pLeaf);

Failure:
    return (NULL);
}


static
BOOL
HuffmanDecodeVerifyTree (
    XPRESS9_STATUS         *pStatus,
    uxint                   uBitLengthCount[HUFFMAN_CODEWORD_LENGTH_LIMIT]
)
{
    uxint i;
    uxint n;

    n = 0;
    i = HUFFMAN_CODEWORD_LENGTH_LIMIT - 1;
    do
    {
        n += uBitLengthCount[i];
        if ((n & 1) != 0)
        {
            SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "i=%Iu n=%Iu", i, n);
            goto Failure;
        }
        n >>= 1;
        --i;
    }
    while (i != 0);

    if (n != 1 || uBitLengthCount[0] != 0)
    {
        SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "n=%Iu Count[0]=%Iu", n, uBitLengthCount[0]);
        goto Failure;
    }

    return (TRUE);

Failure:
    return (FALSE);
}


static
uxint
HuffmanDecodeFillTables (
    XPRESS9_STATUS         *pStatus,
    HUFFMAN_DECODE_TABLE_ENTRY *pDecodeTable,
    uxint                   uDecodeTableSize,
    HUFFMAN_DECODE_NODE    *pLeaf,
    uxint                   uRootBits,
    uxint                   uTailBits,
    HUFFMAN_DECODE_NODE    *pTemp
)
{
    HUFFMAN_DECODE_NODE  *pNode;
    HUFFMAN_DECODE_NODE **ppLink;
    uxint   uTableOffset;
    uxint   uRootLevel;
    uxint   uLeafLevel;
    uxint   uSymbol;
    uxint   uBits;
    uxint   i;
    uxint   j;


    RETAIL_ASSERT (
        uRootBits >= 4 && uTailBits >= 4 &&
        uRootBits <= 15 && uTailBits <= 15 &&
        uRootBits >= uTailBits,
        "uRootBits=%Iu uTailBits=%Iu",
        uRootBits,
        uTailBits
    );

    uTableOffset = 0;
    pNode = NULL;
    ppLink = NULL;
    do
    {

        uLeafLevel = pLeaf->m_uBits;
        if (pNode != NULL && uLeafLevel < pNode->m_uBits)
        {
            uLeafLevel = pNode->m_uBits;
        }

        if (uLeafLevel <= uRootBits)
        {
            uRootLevel = 0;
            uLeafLevel = uRootBits; 
        }
        else
        {
            uRootLevel = uLeafLevel - uTailBits;
        }

        i = ((uxint) 1) << (uLeafLevel - uRootLevel);
        RETAIL_ASSERT (
            uTableOffset + i <= uDecodeTableSize,
            "uTableOffset=%Iu i=%Iu uDecodeTableSize=%Iu",
            uTableOffset,
            i,
            uDecodeTableSize
        );

        do
        {
            if (pNode != NULL && pNode->m_uBits > uRootLevel)
            {
                uSymbol = ((uxint) pNode->m_uSymbol) << uTailBits;
                RETAIL_ASSERT (uSymbol < uTableOffset, "uSymbol=%Iu uTableOffset=%Iu", uSymbol, uTableOffset);
                uSymbol = uSymbol - uTableOffset;
                uBits = pNode->m_uBits;
                pNode = pNode->m_pNext;
                uSymbol += uBits - uRootLevel;
                RETAIL_ASSERT ((xint) uSymbol == (HUFFMAN_DECODE_TABLE_ENTRY) uSymbol, "uSymbol=%Iu", uSymbol);
            }
            else
            {
                uSymbol = pLeaf->m_uSymbol;
                uBits = pLeaf->m_uBits;
                pLeaf = pLeaf->m_pNext;
                uSymbol <<= 4;
                uSymbol += uBits - uRootLevel;
                RETAIL_ASSERT ((xint) uSymbol == (HUFFMAN_DECODE_TABLE_ENTRY) uSymbol, "uSymbol=%Iu", uSymbol);
            }

            RETAIL_ASSERT (i >= (((uxint) 1) << (uLeafLevel - uBits)), "i=%Iu entryCount=%Iu", i, (((uxint) 1) << (uLeafLevel - uBits)));
            j = i - (((uxint) 1) << (uLeafLevel - uBits));

            do
            {
                --i;


                pDecodeTable[uTableOffset + HuffmanReverseMask (i, uLeafLevel - uRootLevel)] = (HUFFMAN_DECODE_TABLE_ENTRY) uSymbol;
            }
            while (i != j);
        }
        while (i != 0);

        pTemp->m_uSymbol = (UInt16) (uTableOffset >> uTailBits);
        pTemp->m_uBits   = (UInt16) uRootLevel;
        pTemp->m_pNext = NULL;
        if (pNode == NULL)
        {
            pNode = pTemp;
        }
        else
        {
            ASSERT (ppLink != NULL, "");
            *ppLink = pTemp;
        }
        ppLink = &pTemp->m_pNext;

        pTemp += 1;
        uTableOffset += ((uxint) 1) << (uLeafLevel - uRootLevel);
    }
    while (uRootLevel != 0);

    RETAIL_ASSERT (pNode->m_pNext == NULL, "pNode->m_uBits=%u", pNode->m_uBits);

    return (uTableOffset);

Failure:
    return (0);
}


XPRESS9_INTERNAL
HUFFMAN_DECODE_TABLE_ENTRY *
HuffmanCreateDecodeTables (
    XPRESS9_STATUS *pStatus,
    const UInt8    *puBitLengthTable,
    uxint           uAlphabetSize,
    HUFFMAN_DECODE_TABLE_ENTRY *pDecodeTable,
    uxint           uDecodeTableSize,
    uxint           uRootBits,
    uxint           uTailBits,
    HUFFMAN_DECODE_NODE *pTemp
)
{
    uxint uBitLengthCount[HUFFMAN_CODEWORD_LENGTH_LIMIT] = {0};
    HUFFMAN_DECODE_NODE  *pLeaf = NULL;
    uxint                  n =0;
    uxint        uNumSymbols = 0;

    RETAIL_ASSERT (pStatus->m_uStatus == Xpress9Status_OK, "BadStatus");

    RETAIL_ASSERT (uRootBits >= uTailBits, "uRootBits=%Iu uTailBits=%Iu", uRootBits, uTailBits);

    pLeaf = HuffmanDecodeSortSymbols (pStatus, puBitLengthTable, uAlphabetSize, &pTemp, uBitLengthCount, &uNumSymbols );
    RETAIL_ASSERT (pLeaf != NULL, "");

    if ( uNumSymbols == 1 )
    {
        uxint uSymbol = pLeaf->m_uSymbol;
        uxint i = ((uxint) 1) << (uRootBits - 0);
        uxint uTableOffset = 0;
        uSymbol <<= 4;
        uSymbol += pLeaf->m_uBits - 0;
        RETAIL_ASSERT ((xint) uSymbol == (HUFFMAN_DECODE_TABLE_ENTRY) uSymbol, "uSymbol=%Iu", uSymbol);

        do
        {
            --i;
            pDecodeTable[uTableOffset + HuffmanReverseMask (i, uRootBits - 0)] = (HUFFMAN_DECODE_TABLE_ENTRY) uSymbol;
        }
        while ( i != 0 );

        return pDecodeTable;
    }

    if (!HuffmanDecodeVerifyTree (pStatus, 
                                  uBitLengthCount
                                  ))
    {
        RETAIL_ASSERT (pStatus->m_uStatus != Xpress9Status_OK, "HuffmanDecodeVerifyTree failed");
        goto Failure;
    }

    n = HuffmanDecodeFillTables (pStatus, pDecodeTable, uDecodeTableSize, pLeaf, uRootBits, uTailBits, pTemp);
    if (n < (((uxint) 1) << uRootBits))
    {
        RETAIL_ASSERT (pStatus->m_uStatus != Xpress9Status_OK, "HuffmanDecodeFillTables failed");
        goto Failure;
    }

    return (pDecodeTable + n - (((uxint) 1) << uRootBits));

Failure:
    return (NULL);

}


XPRESS9_INTERNAL
void
HuffmanDecodeLengthTable (
    XPRESS9_STATUS *pStatus,
    BIO_STATE      *pBioState,
    UInt8          *puBitLengthTable,
    uxint           uAlphabetSize,
    uxint           uFillBoundary,
    uxint           uMaxCodewordLength
)
{
    UInt8 uSmallTableLength[HUFFMAN_ENCODED_TABLE_SIZE] = {0};
    HUFFMAN_DECODE_NODE DecodeNode[HUFFMAN_ENCODED_TABLE_SIZE + 2] = {0};
    HUFFMAN_DECODE_TABLE_ENTRY DecodeTable[(1 << 8) + (2 << 4)] = {0};
    HUFFMAN_DECODE_TABLE_ENTRY *pDecodeTable = NULL;
    MSB_T uBits = 0;
    uxint uPrevSymbol = 0;
    uxint i = 0;
    uxint k = 0;
    xint  iSymbol = 0;
    BIO_DECLARE();

    RETAIL_ASSERT (pStatus->m_uStatus == Xpress9Status_OK, "BadStatus");

    BIO_STATE_ENTER ((*pBioState));

    k = BIORD_LOOKUP_FIXED (3);
    BIORD_SKIP (3);
    if (k == 0)
    {
        GET_MSB (uBits, uAlphabetSize);
        iSymbol = uBits;
        k = POWER2 (uBits + 1) - uAlphabetSize;
        for (i = 0; i < k; ++i)
        {
            puBitLengthTable[i] = (UInt8) iSymbol;
        }
        iSymbol += 1;
        for (; i < uAlphabetSize; ++i)
        {
            puBitLengthTable[i] = (UInt8) iSymbol;
        }
        goto Done;
    }


    if (k != 1)
    {
        SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "UnknownHuffmanTableEncoding=%Iu", k);
        goto Failure;
    }

    uPrevSymbol = 4;
    for (i = 0; i < HUFFMAN_ENCODED_TABLE_SIZE; ++i)
    {
        k = BIORD_LOOKUP_FIXED (1);
        BIORD_SKIP (1);
        if (k == 0)
        {
            uSmallTableLength[i] = (UInt8) uPrevSymbol;
        }
        else
        {
            k = BIORD_LOOKUP_FIXED (3);
            BIORD_SKIP (3);
            if (k >= uPrevSymbol)
            {
                k += 1;
                if (k > 8)
                {
                    SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "PrevSymbol=%Iu CurrentSymbol=%Iu", uPrevSymbol, k);
                    goto Failure;
                }
            }

            uSmallTableLength[i] = (UInt8) k;
            uPrevSymbol = k;
        }
    }


    pDecodeTable = HuffmanCreateDecodeTables (
        pStatus,
        uSmallTableLength,
        HUFFMAN_ENCODED_TABLE_SIZE,
        DecodeTable,
        sizeof (DecodeTable) / sizeof (DecodeTable[0]),
        8,
        4,
        DecodeNode
    );
    if (pStatus->m_uStatus != Xpress9Status_OK)
    {
        goto Failure;
    }

    uPrevSymbol = 8;
    for (i = 0; i < uAlphabetSize;)
    {
        HUFFMAN_DECODE_SYMBOL (iSymbol, pDecodeTable, 8, 1);
        switch (iSymbol)
        {
        default:
            if (((uxint) iSymbol) > uMaxCodewordLength)
            {
                SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "i=%Iu iSymbol=%Iu uMaxCodewordLength=%Iu", i, iSymbol, uMaxCodewordLength);
                goto Failure;
            }
            puBitLengthTable[i] = (UInt8) iSymbol;
            ++i;
            if (iSymbol != 0)
            {
                uPrevSymbol = iSymbol;
            }
            break;

        case HUFFMAN_ENCODED_TABLE_FILL:
            do
            {
                puBitLengthTable[i] = 0;
                ++i;
            }
            while ((i & (uFillBoundary - 1)) != 0 && i < uAlphabetSize);
            break;

        case HUFFMAN_ENCODED_TABLE_ZERO_REPT:
            k = BIORD_LOOKUP_FIXED (2);
            BIORD_SKIP (2);
            iSymbol = k + HUFFMAN_ENCODED_TABLE_ZERO_REPT_MIN_COUNT;
            if (k == 3)
            {
                do
                {
                    k = BIORD_LOOKUP_FIXED (3);
                    BIORD_SKIP (3);
                    iSymbol += k;
                    if (i + iSymbol > uAlphabetSize || (i ^ (i + iSymbol)) >= uFillBoundary)
                    {
                        SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "i=%Iu iSymbol=%Iu uAlphabetSize=%Iu uFillBoundary=%Iu", i, iSymbol, uAlphabetSize, uFillBoundary);
                        goto Failure;
                    }
                }
                while (k == 7);
            }
            if (i + iSymbol > uAlphabetSize || (i ^ (i + iSymbol)) >= uFillBoundary)
            {
                SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "i=%Iu iSymbol=%Iu uAlphabetSize=%Iu uFillBoundary=%Iu", i, iSymbol, uAlphabetSize, uFillBoundary);
                goto Failure;
            }
            do
            {
                puBitLengthTable[i] = 0;
                ++i;
                --iSymbol;
            }
            while (i < uAlphabetSize && iSymbol != 0);
            break;

        case HUFFMAN_ENCODED_TABLE_PREV:
            puBitLengthTable[i] = (UInt8) uPrevSymbol;
            ++i;
            break;

        case HUFFMAN_ENCODED_TABLE_ROW_0:
            if (i < uFillBoundary)
            {
                SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "i=%Iu uFillBoundary=%Iu iSymbol=HUFFMAN_ENCODED_TABLE_ROW_0", i, uFillBoundary);
                goto Failure;
            }
            puBitLengthTable[i] = puBitLengthTable[i - uFillBoundary];
            if (puBitLengthTable[i] == 0)
            {
                SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "i=%Iu iSymbol=HUFFMAN_ENCODED_TABLE_ROW_0 Length=0", i);
                goto Failure;
            }
            uPrevSymbol = puBitLengthTable[i];
            ++i;
            break;

        case HUFFMAN_ENCODED_TABLE_ROW_1:
            if (i < uFillBoundary)
            {
                SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "i=%Iu uFillBoundary=%Iu iSymbol=HUFFMAN_ENCODED_TABLE_ROW_1", i, uFillBoundary);
                goto Failure;
            }
            puBitLengthTable[i] = (UInt8) (puBitLengthTable[i - uFillBoundary] + 1);
            if (puBitLengthTable[i] > uMaxCodewordLength || puBitLengthTable[i] == 0)
            {
                SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "i=%Iu iSymbol=HUFFMAN_ENCODED_TABLE_ROW_1 Length=%u", i, puBitLengthTable[i]);
                goto Failure;
            }
            uPrevSymbol = puBitLengthTable[i];
            ++i;
            break;
        }
    }

Done:
    BIO_STATE_LEAVE ((*pBioState));

Failure:;
}


#if 0


#define HUFFMAN_DECODE_SYMBOL(iSymbol,pTable,ROOT_LOOKUP_BITS,TAIL_LOOKUP_BITS) do {    \
    uxint _uBits;                                                                       \
    iSymbol = BIORD_LOOKUP_FIXED (ROOT_LOOKUP_BITS);                                    \
    iSymbol = (pTable)[iSymbol];                                                        \
    if (((xint) iSymbol) < 0)                                                           \
    {                                                                                   \
        HUFFMAN_DECODE_TABLE_ENTRY *pNextTable;                                         \
        _uBits = iSymbol & 15;                                                          \
        BIORD_SKIP (_uBits);                                                            \
        pNextTable = pTable + (iSymbol & ~ (xint) 15);                                  \
        while ((xint) (iSymbol = BIORD_LOOKUP_FIXED (TAIL_LOOKUP_BITS)) < 0)            \
        {                                                                               \
            _uBits = iSymbol & 15;                                                      \
            BIORD_SKIP (_uBits);                                                        \
            pNextTable += iSymbol & ~ (xint) 15;                                        \
        }                                                                               \
    }                                                                                   \
    iSymbol >>= 4;                                                                      \
} while (0)

HUFFMAN_DECODE DecodeTable[65536];
UInt8 LengthTable[256];
HUFFMAN_DECODE_NODE Scratch[256*2];

int main (void)
{
    XPRESS9_STATUS Status;
    uxint i;
    uxint n;


    LengthTable[0] = 0;
    for (i = 1; i < 20; ++i)
        LengthTable[i] = (UInt8) i;
    LengthTable[i] = (UInt8) (i - 1);

    n = HuffmanCreateDecodeTables (
        &Status,
        LengthTable,
        i + 1,
        DecodeTable,
        sizeof (DecodeTable) / sizeof (DecodeTable[0]),
        8,
        8,
        Scratch
    );

    printf ("n=%Iu\n", n);

    return (0);
}


#endif
