// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "Xpress9Internal.h"

//
// Produces sorted list of symbols in descending order by codeword length in stable manner
//
// Returns head of the list on success, NULL otherwise
//
static
HUFFMAN_DECODE_NODE *
HuffmanDecodeSortSymbols (
    XPRESS9_STATUS         *pStatus,
    const UInt8            *puBitLengthTable, // puBitLengthTable[i] = codeword length of i-th symbol
    uxint                   uAlphabetSize,
    HUFFMAN_DECODE_NODE   **ppTemp,
    uxint                   uBitLengthCount[HUFFMAN_CODEWORD_LENGTH_LIMIT],
    uxint                   *puSymbolCount // Number of symbols from the alphabet that are actually present in the input ( note this is not the input to the compressor )
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

    //
    // sort symbols by length of codeword
    //
    // As in encoder Chain[i] stores a linked list of all nodes containing symbols with codeword length i.
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
            // i-th symbol is present
            n = puBitLengthTable[i];
            RETAIL_ASSERT (n < HUFFMAN_CODEWORD_LENGTH_LIMIT, "ExcessivelyLargeBitLength: symbol=%Iu bits=%Iu", i, n);

            pTemp->m_uSymbol = (UInt16) i;
            pTemp->m_uBits   = (UInt16) n;
            uBitLengthCount[n] += 1; //Increment number of symbols with codeword length n.

            *(Chain[n].m_ppLast) = pTemp;
            Chain[n].m_ppLast = &pTemp->m_pNext;
            pTemp += 1;
            (*puSymbolCount) += 1;
        }
    }
    while (i != 0);
    // build the list of symbols sorted in descending order by bit length and then by symbol code
    pLeaf = NULL;
    for (i = 0; i < HUFFMAN_CODEWORD_LENGTH_LIMIT; ++i)
    {
        *(Chain[i].m_ppLast) = pLeaf;
        pLeaf = Chain[i].m_pHead;
    }

    RETAIL_ASSERT (pLeaf != NULL, "");

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT
    // verify sort order
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
#endif /* XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT */

    *ppTemp = pTemp;
    return (pLeaf);

Failure:
    return (NULL);
}


//
// Verify that tree is full binary tree (i.e. every node has either 0 or 2 children but never 1)
//
// Returns TRUE if tree is full, FALSE otherwise
//
static
BOOL
HuffmanDecodeVerifyTree (
    XPRESS9_STATUS         *pStatus,
    uxint                   uBitLengthCount[HUFFMAN_CODEWORD_LENGTH_LIMIT] // uBitLengthCount[i] = number of symbols with codeword length i.
)
{
    uxint i;
    uxint n;

    n = 0;
    i = HUFFMAN_CODEWORD_LENGTH_LIMIT - 1;
    do
    {
        n += uBitLengthCount[i];
        if ((n & 1) != 0) // n is odd
        {
            SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "i=%Iu n=%Iu", i, n);
            goto Failure;
        }
        n >>= 1;
        --i;
    }
    while (i != 0);

    // n != 1 checks for -> there is at least one symbol with non-zero codeword length
    // uBitLengthCount[0] -> number of symbols with codeword length 0 = 0.
    if (n != 1 || uBitLengthCount[0] != 0)
    {
        SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "n=%Iu Count[0]=%Iu", n, uBitLengthCount[0]);
        goto Failure;
    }

    return (TRUE);

Failure:
    return (FALSE);
}


//
// Given the list of symbols sorted by codeword length in descending order in stable manner,
// fill decoding tables
//
// Returns number of entries used in pDecodeTable on success, 0 on failure
//
static
uxint
HuffmanDecodeFillTables (
    XPRESS9_STATUS         *pStatus,
    HUFFMAN_DECODE_TABLE_ENTRY *pDecodeTable, // The table that holds the decode entry
    uxint                   uDecodeTableSize, // The max size of the decode table
    HUFFMAN_DECODE_NODE    *pLeaf, // Pointer to the leaf with the longest codeword length
    uxint                   uRootBits, // Number of bits to be used for root table (top-level table) lookup
    uxint                   uTailBits, // Number of bits used for each of the subsequent lookups.
    HUFFMAN_DECODE_NODE    *pTemp // Temporary area to store intermediate nodes, the root nodes at the levels at which we create the tables.
)
{
    HUFFMAN_DECODE_NODE  *pNode;
    HUFFMAN_DECODE_NODE **ppLink;
    uxint   uTableOffset;
    uxint   uRootLevel; // Level of the current root -- current root is the node that has an entry for each 
    uxint   uLeafLevel;
    uxint   uSymbol;
    uxint   uBits;
    uxint   i;
    uxint   j;

    // Lets assume the following scenario:
    // The length of the longest code word = n, n > uRootBits
    // This is how the algorithm works:
    // Expand the tree of height n rooted at the node at level 0 to be full-binary by adding fictitious nodes.
    // For each leaf node that is actually present in the subtree, create an interval.
    // Each level-n node belongs to the subtree of a codeword.
    // Divide the range [0, 2^n] into N intervals. All nodes in the subtree for a codeword will 
    // have the same value.
    //

    //
    // HUFFMAN_DECODE_SYMBOL relies on the fact that offset to parent table is at least multiple of 16,
    // using 4 LSB to encode number of bits to remove from input stream
    //
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
        //
        // start next table
        //
        // There is one table at the root level of size 1 << uRootBits, and multiple other tables of size 1 << uTailBits 
        // at locations such that the leaves at all levels are part of some table.
        // If a leaf has a codeword length longer than uRootBits, we construct the path for the search such that after
        // multiple lookups we are able to end up at that node.
        // Note that the tables are filled in reverse codeword length order, i.e bottom up.
        // To elaborate, we fill the lowest level table first, and then proceed upwards.
        // If length of longest codeword = n > uRootBits, the first table is constructed for level n.

        uLeafLevel = pLeaf->m_uBits; // The level at which this leaf is.
        // If there are more leaves at levels higher than the previous rootlevel (  pNode->m_uBits ) then construct tables
        // for those nodes. Otherwise, construct the table at this level.
        // (pavane) should we add an assert that ( uLeafLevel  == pNode->m_uBits || uLeafLevel == pNode->m_uBits + 1 )
        // since cannonical huffman code cannot have any node with only the right child at any level.
        if (pNode != NULL && uLeafLevel < pNode->m_uBits)
        {
            uLeafLevel = pNode->m_uBits;
        }

        if (uLeafLevel <= uRootBits)
        { // This leaf is at a level where we can find the symbol using a single lookup just using the root bits.
            uRootLevel = 0; // Thus, the fictitious full-binary subtree is rooted at the root of the tree, i.e level 0.
            uLeafLevel = uRootBits; 
        }
        else
        {
            // This leaf is at a level where we need more than one look up.
            uRootLevel = uLeafLevel - uTailBits;
        }

        i = ((uxint) 1) << (uLeafLevel - uRootLevel); // The total size of this table.
        // Verify that there is enough space in the whole decode table to add one more table.
        RETAIL_ASSERT (
            uTableOffset + i <= uDecodeTableSize,
            "uTableOffset=%Iu i=%Iu uDecodeTableSize=%Iu",
            uTableOffset,
            i,
            uDecodeTableSize
        );

        // The following loop populates one table; either the root table or one of the next level tables.
        do
        {
            if (pNode != NULL && pNode->m_uBits > uRootLevel)
            {
                // uSymbol generated out of this if will be < 0.
                // This corresponds to the next table lookup in HUFFMAN_DECODE_SYMBOL.
                uSymbol = ((uxint) pNode->m_uSymbol) << uTailBits;
                RETAIL_ASSERT (uSymbol < uTableOffset, "uSymbol=%Iu uTableOffset=%Iu", uSymbol, uTableOffset);
                uSymbol = uSymbol - uTableOffset;
                uBits = pNode->m_uBits;
                pNode = pNode->m_pNext;
                uSymbol += uBits - uRootLevel; // To indicate that the decoder should skip this many bits from the stream.
                RETAIL_ASSERT ((xint) uSymbol == (HUFFMAN_DECODE_TABLE_ENTRY) uSymbol, "uSymbol=%Iu", uSymbol);
            }
            else
            {
                uSymbol = pLeaf->m_uSymbol;
                uBits = pLeaf->m_uBits;
                pLeaf = pLeaf->m_pNext;
                uSymbol <<= 4;
                uSymbol += uBits - uRootLevel; // (uBits - uRootLevel) = distance from current root, i.e. number of bits in the codeword from root to this leaf.
                RETAIL_ASSERT ((xint) uSymbol == (HUFFMAN_DECODE_TABLE_ENTRY) uSymbol, "uSymbol=%Iu", uSymbol);
            }

            RETAIL_ASSERT (i >= (((uxint) 1) << (uLeafLevel - uBits)), "i=%Iu entryCount=%Iu", i, (((uxint) 1) << (uLeafLevel - uBits)));
            j = i - (((uxint) 1) << (uLeafLevel - uBits)); // 1 << ( uLeafLevel - uBits ) represents the number of nodes in the subtree of the codeword that ends in the current pLeaf.

            do
            {
                --i;

                // The need for HuffmanReverseMask : 
                // Note that the counterpart encode was: puMask[pNode->m_uSymbol] = (HUFFMAN_MASK) ((HuffmanReverseMask (uMask, uBits) << HUFFMAN_CODEWORD_LENGTH_BITS) + uBits);
                // HuffmanReverseMask ensures that if there is a need for two lookups, the lookup is such that we first lookup the bit pattern
                // from root a certain level and then more bits from that level.
                // Consider the following case:
                // The codeword to look up is of  13 bits length:  1 111111 111110
                // The RootLookup bits = 12. When the encoder wrote the mask, we would have written: 011111 111111 100000
                // We first lookup the right most 12 bits (least significant 12 bits). They are 111111 100000 ( which when HuffmanReversed correspond to 1 111111 -- 
                // the MSB i.e. path from root to some intermediate level node where we have another level of lookup ).
                // Since in the following code, we already create the decoded table such that the entry at i's bits HuffmanReversed;
                // this 12 bit pattern can used to further drive the next level lookup, using the bits 011111 for tail lookup.
                // With this method, the order in which the bits were used for the lookup is such that we looked up using the bits in the 
                // order in which they appear in the mask.

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


//
// Build decoding tables compatible with HUFFMAN_DECODE_SYMBOL macro
//
// Returns NULL in case of failure (sets (*pStatus) appropriately), non-NULL value in case of success
//
XPRESS9_INTERNAL
HUFFMAN_DECODE_TABLE_ENTRY *
HuffmanCreateDecodeTables (
    XPRESS9_STATUS *pStatus,                    // OUT: error status
    const UInt8    *puBitLengthTable,           // IN:  puBitLengthTable[i] = codeword length of i-th symbol
    uxint           uAlphabetSize,              // IN:  size of the alphabet
    HUFFMAN_DECODE_TABLE_ENTRY *pDecodeTable,   // OUT: area for decode tables (1<<uRootBits) + k*(1<<uTailBits) entries
    uxint           uDecodeTableSize,           // IN:  size of decode tables
    uxint           uRootBits,                  // IN:  # of bits that index root table
    uxint           uTailBits,                  // IN:  # of bits that index other tables
    HUFFMAN_DECODE_NODE *pTemp                  // IN:  scratch area (uAlphabetSize + k) entries
)
{
    uxint uBitLengthCount[HUFFMAN_CODEWORD_LENGTH_LIMIT] = {0}; // intended to store at index i, the number of symbols with bit-length i.
    HUFFMAN_DECODE_NODE  *pLeaf = NULL;
    uxint                  n =0;
    uxint        uNumSymbols = 0;

    RETAIL_ASSERT (pStatus->m_uStatus == Xpress9Status_OK, "BadStatus");

    RETAIL_ASSERT (uRootBits >= uTailBits, "uRootBits=%Iu uTailBits=%Iu", uRootBits, uTailBits);

    pLeaf = HuffmanDecodeSortSymbols (pStatus, puBitLengthTable, uAlphabetSize, &pTemp, uBitLengthCount, &uNumSymbols );
    RETAIL_ASSERT (pLeaf != NULL, "");

    if ( uNumSymbols == 1 )
    {
        // The rest of the code in this function (outside this if) assumes that the tree has 
        // either 2 or 0 nodes at any level where there is a leaf.
        // This is not satisfied in the case of a tree with a single symbol. Handle it specially here.
        // Fill the decode tables using the same logic as done in HuffmanDecodeFillTables.
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
                                  uBitLengthCount // uBitLengthCount[i] = number of symbols with code word length i.
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


//
// Restore sequence of codeword lengths
//
XPRESS9_INTERNAL
void
HuffmanDecodeLengthTable (
    XPRESS9_STATUS *pStatus,                    // OUT: error status
    BIO_STATE      *pBioState,                  // INOUT: bitstream I/O state
    UInt8          *puBitLengthTable,           // IN:  puBitLengthTable[i] = codeword length of i-th symbol
    uxint           uAlphabetSize,              // IN:  size of the alphabet
    uxint           uFillBoundary,              // IN:  0 fill boundary (must be exact power of two)
    uxint           uMaxCodewordLength          // IN:  max allowed codeword length
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
        //
        // "stored" encoding
        //
        // Corresponds to an encoding where the first N symbols of the alphabet
        // were encoded with M=MSB(|A|) bits, remaining |A|-N symbols were encoded with (M+1) bits
        //
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

    // The three bits that we read above should be 0 -- stored encoding; or 1 - Xpress9HuffmanEncodeTable

    if (k != 1)
    {
        SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "UnknownHuffmanTableEncoding=%Iu", k);
        goto Failure;
    }

    //
    // read small table
    //
    uPrevSymbol = 4; // should probably be #defined since we use the same value while encoding the small table.
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
        uSmallTableLength, // For each symbol contains number of bits.
        HUFFMAN_ENCODED_TABLE_SIZE, // should be made sizeof (uCount) / sizeof (uCount[0]) for better readability, alphabet size
        DecodeTable,
        sizeof (DecodeTable) / sizeof (DecodeTable[0]),
        8, // Root bits
        4, // tail bits
        DecodeNode
    );
    if (pStatus->m_uStatus != Xpress9Status_OK)
    {
        // bad length table
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
//printf (" %2u", puBitLengthTable[i-1]); if ((i & 15) == 0) printf ("\n");
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
//printf (" %2u", puBitLengthTable[i-1]); if ((i & 15) == 0) printf ("\n");
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
//printf (" %2u", puBitLengthTable[i-1]); if ((i & 15) == 0) printf ("\n");
                --iSymbol;
            }
            while (i < uAlphabetSize && iSymbol != 0);
            break;

        case HUFFMAN_ENCODED_TABLE_PREV:
            puBitLengthTable[i] = (UInt8) uPrevSymbol;
            ++i;
//printf (" %2u", puBitLengthTable[i-1]); if ((i & 15) == 0) printf ("\n");
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
//printf (" %2u", puBitLengthTable[i-1]); if ((i & 15) == 0) printf ("\n");
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
//printf (" %2u", puBitLengthTable[i-1]); if ((i & 15) == 0) printf ("\n");
            break;
        }
    }
//printf ("\n");

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
