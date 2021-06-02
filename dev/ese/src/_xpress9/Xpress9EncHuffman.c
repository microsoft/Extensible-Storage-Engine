// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "Xpress9Internal.h"


#define HUFFMAN_RADIX_MERGE_SORT_THRESHOLD  64      // use MergeSort if number of entries below HUFFMAN_RADIX_MERGE_SORT_THRESHOLD

/*
 Time distribution:
 40%    sorting symbols by frequency
 15%    evaluating codeword lengths
 15%    building the tree
 15%    storing symbol frequencies
  8%    sorting symbols by codeword lengths
  7%    everything else
*/


//
// Copies non-0 counts into the nodes.
// Returns number of nodes that have non-0 counts
//
static
uxint
HuffmanCreateTemp (
    HUFFMAN_NODE           *pTemp,
    const HUFFMAN_COUNT    *pCount,
    uxint                   uAlphabetSize
)
{
    uxint i;
    uxint n;

    n = 0;
    for (i = 0; i < uAlphabetSize; ++i)
    {
        if (pCount[i] == 0)
        {
            continue;
        }
        pTemp[n].m_pSon[0] = NULL;
        pTemp[n].m_pSon[1] = NULL;
        pTemp[n].m_uSymbol = (UInt16) i;
        pTemp[n].m_uCount  = pCount[i];
        pTemp[n].m_pNext = pTemp + n + 1;
        ++n;
    }
    
    if (n != 0)
    {
        pTemp[n-1].m_pNext = NULL;
    }

    return (n);
}


//
// Sort single-linked list of HUFFMAN_NODE's in ascending order using m_pNext node
// Returns pointer to the the node with minumum count value
//
static
HUFFMAN_NODE *
HuffmanMergeSort (
    HUFFMAN_NODE *pUnsorted
)
{
    HUFFMAN_NODE   *pRecord1;
    HUFFMAN_NODE   *pRecord2;
    HUFFMAN_NODE  **ppLink;

    struct
    {
        HUFFMAN_NODE   *m_pHead;
        uxint           m_uBits;
    } Stack[128], *pStack;

    pStack = Stack;
    pStack->m_pHead = NULL;
    pStack->m_uBits = 0;

    if (pUnsorted == NULL)
    {
        return (NULL);
    }

    for (;;)
    {
        //
        // get next 2 records
        //
        ++pStack;
        ppLink = NULL;

        pRecord1 = pUnsorted;
        pRecord2 = pRecord1->m_pNext;
        pStack->m_pHead = pRecord1;
        pStack->m_uBits = 1;
        if (pRecord2 == NULL)
        {
            //
            // only 1 record to go
            //
            pUnsorted = NULL;
            break;
        }
        else
        {
            pUnsorted = pRecord2->m_pNext;
            if (pRecord1->m_uCount > pRecord2->m_uCount)
            {
                //
                // inverse the order
                //
                pStack->m_pHead = pRecord2;
                pRecord2->m_pNext = pRecord1;
                pRecord2 = pRecord1;
            }
            pRecord2->m_pNext = NULL;
            if (pUnsorted == NULL)
                break;
        }

        //
        // go back and merge
        //
        while (pStack->m_uBits == pStack[-1].m_uBits)
        {
            // merge two lists
            pStack[-1].m_uBits += 1;

            pRecord1 = pStack[-1].m_pHead;
            pRecord2 = pStack->m_pHead;
            ppLink   = &pStack[-1].m_pHead;
            --pStack;

            if (pRecord1->m_uCount > pRecord2->m_uCount)
                goto Entry2a;

            for (;;)
            {
                *ppLink  = pRecord1;

                do
                {
                    ppLink  = &pRecord1->m_pNext;
                    pRecord1 = pRecord1->m_pNext;
                    if (pRecord1 == NULL)
                        goto Exit1a;
                }
                while (pRecord1->m_uCount <= pRecord2->m_uCount);

            Entry2a:
                *ppLink = pRecord2;

                do
                {
                    ppLink = &pRecord2->m_pNext;
                    pRecord2 = pRecord2->m_pNext;

                    if (pRecord2 == NULL)
                        goto Exit2a;
                }
                while (pRecord1->m_uCount > pRecord2->m_uCount);
            }

        Exit2a:
            pRecord2 = pRecord1;
        Exit1a:
            *ppLink = pRecord2;
        }
    }

    //
    // go back and merge
    //
    while (pStack[-1].m_uBits != 0)
    {
        // merge two lists
        pRecord1 = pStack[-1].m_pHead;
        pRecord2 = pStack->m_pHead;
        ppLink   = &pStack[-1].m_pHead;
        --pStack;

        if (pRecord1->m_uCount > pRecord2->m_uCount)
            goto Entry2b;

        for (;;)
        {
            *ppLink  = pRecord1;

            do
            {
                ppLink  = &pRecord1->m_pNext;
                pRecord1 = pRecord1->m_pNext;
                if (pRecord1 == NULL)
                    goto Exit1b;
            }
            while (pRecord1->m_uCount <= pRecord2->m_uCount);

        Entry2b:
            *ppLink = pRecord2;

            do
            {
                ppLink = &pRecord2->m_pNext;
                pRecord2 = pRecord2->m_pNext;

                if (pRecord2 == NULL)
                    goto Exit2b;
            }
            while (pRecord1->m_uCount > pRecord2->m_uCount);
        }

    Exit2b:
        pRecord2 = pRecord1;
    Exit1b:
        *ppLink = pRecord2;
    }

    return (pStack->m_pHead);
}


typedef struct HUFFMAN_RADIX_SORT_CHAIN_T HUFFMAN_RADIX_SORT_CHAIN;
struct HUFFMAN_RADIX_SORT_CHAIN_T
{
    HUFFMAN_NODE   *m_pHead;
    HUFFMAN_NODE  **m_ppLast;
};


//
// Sort single-linked list of HUFFMAN_NODE's in ascending order using m_pNext node
// Returns pointer to the the node with minumum count value
//
static
HUFFMAN_NODE *
HuffmanRadixSort (
    XPRESS9_STATUS *pStatus,
    HUFFMAN_NODE   *pNode
)
{
    HUFFMAN_RADIX_SORT_CHAIN    Chain[256];
    uxint   uShift;
    uxint   i;

    // initialize ppLast by setting it to pHead
    for (i = 0; i < 256; ++i)
    {
        Chain[i].m_ppLast = &Chain[i].m_pHead;
    }

    uShift = 0;
    do
    {
        RETAIL_ASSERT (pNode != NULL, "");

        // insert nodes into appropriate buckets
        do
        {
            i = (pNode->m_uCount >> uShift) & 255;
            *(Chain[i].m_ppLast) = pNode;
            Chain[i].m_ppLast = &pNode->m_pNext;
            pNode = pNode->m_pNext;
        }
        while (pNode != NULL);

        // merge all buckets into single list
        pNode = NULL;
        i = 256;
        do
        {
            --i;
            *(Chain[i].m_ppLast) = pNode;
            pNode = Chain[i].m_pHead;
            Chain[i].m_ppLast = &Chain[i].m_pHead;
        }
        while (i != 0);

        uShift += 8;
    }
    while (uShift != 32);

    return (pNode);

Failure:
    return (NULL);
}

//
// Given sorted [by count] list of symbols, create Huffman tree
// Returns pointer to the root of the tree, NULL in case of fatal failure (corrupted sorted list)
//
static
HUFFMAN_NODE *
HuffmanCreateTree (
    XPRESS9_STATUS  *pStatus,
    HUFFMAN_NODE    *pLeaf,
    HUFFMAN_NODE    *pTemp
)
{
    uxint n;
    HUFFMAN_NODE *pNode;

    //
    // at least 2 leaves shall be available; form first intermediate node by merging two leafs
    //
    RETAIL_ASSERT (pLeaf != NULL , "");
    pTemp->m_pSon[0] = pLeaf;
    pTemp->m_uCount = pLeaf->m_uCount;
    pLeaf = pLeaf->m_pNext;

    RETAIL_ASSERT (pLeaf != NULL, "");
    pTemp->m_pSon[1] = pLeaf;
    RETAIL_ASSERT (pTemp->m_uCount <= ~pLeaf->m_uCount, "Overflow %Iu + %Iu", pTemp->m_uCount, pLeaf->m_uCount);
    pTemp->m_uCount += pLeaf->m_uCount;
    pLeaf = pLeaf->m_pNext;

    pTemp->m_uSymbol = 0xffff;

    pNode = pTemp;

    n = 0;
    while (pLeaf != NULL)
    {
        ++pTemp;
        pTemp->m_uSymbol = 0xffff;

        if (pLeaf->m_uCount <= pNode->m_uCount)
        {
            pTemp->m_pSon[0] = pLeaf;
            pTemp->m_uCount = pLeaf->m_uCount;
            pLeaf = pLeaf->m_pNext;

            if (pLeaf == NULL)
            {
                RETAIL_ASSERT (pNode != pTemp, "");
                goto GetRightNode;
            }
        }
        else
        {
            pTemp->m_pSon[0] = pNode;
            pTemp->m_uCount = pNode->m_uCount;
            ++pNode;

            if (pNode == pTemp)
            {
                RETAIL_ASSERT (pLeaf != NULL, "");
                goto GetRightLeaf;
            }
        }

        if (pLeaf->m_uCount <= pNode->m_uCount)
        {
        GetRightLeaf:
            pTemp->m_pSon[1] = pLeaf;
            pTemp->m_uCount += pLeaf->m_uCount;
            pLeaf = pLeaf->m_pNext;
        }
        else
        {
        GetRightNode:
            pTemp->m_pSon[1] = pNode;
            pTemp->m_uCount += pNode->m_uCount;
            ++pNode;
        }
    }

    while (pNode < pTemp)
    {
        ++pTemp;
        pTemp->m_uSymbol = 0xffff;

        pTemp->m_pSon[0] = pNode;
        pTemp->m_uCount = pNode->m_uCount;
        ++pNode;
        RETAIL_ASSERT (pNode != pTemp, "");

        pTemp->m_pSon[1] = pNode;
        pTemp->m_uCount += pNode->m_uCount;
        ++pNode;
    }

    return (pNode);

Failure:
    return (NULL);
}


//
// Given Huffman tree, compute length of codewords
// Returns TRUE if frequencies are correct, FALSE otherwise
//
static
BOOL
HuffmanComputeBitLength (
    XPRESS9_STATUS *pStatus,
    HUFFMAN_NODE   *pRoot,
    uxint           puBitLengthCount[HUFFMAN_CODEWORD_LENGTH_LIMIT],
    uxint           uAlphabetSize
)
{
    HUFFMAN_NODE   *Stack[HUFFMAN_CODEWORD_LENGTH_LIMIT];
    HUFFMAN_NODE  **pStack;
    HUFFMAN_NODE   *pTemp;
    size_t          uBits;

    memset (puBitLengthCount, 0, sizeof (puBitLengthCount[0]) * HUFFMAN_CODEWORD_LENGTH_LIMIT);

    pRoot->m_uBits = 0;
    pStack = Stack;
    *pStack++ = pRoot;
    do
    {
        pRoot = *--pStack;
        uBits = pRoot->m_uBits;

        // traverse the tree (it should be right-aligned)
        while (pRoot->m_pSon[0] != NULL)
        {
            RETAIL_ASSERT (pRoot->m_pSon[1] != NULL, "LeftSonIsNotNullButRightIs");
            // Intermediate nodes all have symbol = 0xffff. Leaf nodes have the original symbols.
            RETAIL_ASSERT (pRoot->m_uSymbol > uAlphabetSize, "IntermediateNodeSymbol=%Iu AlphabetSize=%Iu", pRoot->m_uSymbol, uAlphabetSize);
            uBits += 1;
            pTemp = pRoot->m_pSon[1];
            pTemp->m_uBits = (UInt16) uBits;
            *pStack++ = pTemp;
            RETAIL_ASSERT (pStack < Stack + sizeof (Stack) / sizeof (Stack[0]), "HuffmanTreeIsWayTooDeep");
            pRoot = pRoot->m_pSon[0];
            pRoot->m_uBits = (UInt16) uBits;
        }

        // reached leaf node
        RETAIL_ASSERT (pRoot->m_pSon[1] == NULL, "LeftSonIsNullButRightIsNotNull");
        // All the nodes which contain the symbols are leaves.
        RETAIL_ASSERT (pRoot->m_uSymbol < uAlphabetSize, "LeafSymbol=%Iu AlphabetSize=%Iu", pRoot->m_uSymbol, uAlphabetSize);
        RETAIL_ASSERT (uBits < HUFFMAN_CODEWORD_LENGTH_LIMIT, "uBits=%Iu", uBits);

        puBitLengthCount[uBits] += 1;
    }
    while (pStack > Stack);

    return (TRUE);

Failure:
    return (FALSE);
}


//
// Ensure that maximum codeword length <= uMaxCodewordLength
// Returns TRUE if frequencies are correct, FALSE otherwise
//
//
static
BOOL
HuffmanTruncateTree (
    XPRESS9_STATUS *pStatus,
    uxint   puBitLengthCount[HUFFMAN_CODEWORD_LENGTH_LIMIT],
    uxint   uMaxCodewordLength
)
{
    uxint   i;
    uxint   j;

    j = uMaxCodewordLength;
    for (i = HUFFMAN_CODEWORD_LENGTH_LIMIT - 1; i > uMaxCodewordLength; --i)
    {
        while (puBitLengthCount[i] != 0)
        {
            if (j == uMaxCodewordLength)
            {
                // find deepest leaf with codeword length < uMaxCodewordLength
                do
                {
                    --j;
                    RETAIL_ASSERT (j > 0, "CorruptHuffmanFrequencyTable");
                }
                while (puBitLengthCount[j] == 0);
            }

            RETAIL_ASSERT (puBitLengthCount[j] > 0, "CorruptHuffmanFrequencyTableIndex");

            //
            // convert leaf at level j into node and attach to it one leaf from level i and one leaf from level j;
            // remaining leaf at level i moves one level up
            //
            puBitLengthCount[j]     -= 1; // The node at level j is no longer as leaf now, so decrement.
            puBitLengthCount[j + 1] += 2; // Two new leaf nodes are attached at level j+1.
            puBitLengthCount[i - 1] += 1; // The leaf node at level i+1 gets attached here.
            puBitLengthCount[i]     -= 2; // Two leaf nodes have been lost from level i.

            //
            // now deepest leaf with codeword length < uMaxCodeWordLength is at level (j+1) unless
            // (j+1) == uMaxCodewordLength
            //
            j += 1;
        }
    }

    return (TRUE);

Failure:
    return (FALSE);
}

//
// Canonize the tree
// Cannonical huffman code see: http://en.wikipedia.org/wiki/Canonical_Huffman_code
//
static
HUFFMAN_NODE *
HuffmanCanonizeTree (
    XPRESS9_STATUS *pStatus,
    HUFFMAN_NODE   *pSortedList,            // list of symbols sorted by m_uCount
    uxint           puBitLengthCount[HUFFMAN_CODEWORD_LENGTH_LIMIT],   // number of symbols having respective codeword length
    HUFFMAN_MASK   *puBits,                 // puBits[i] = number of bits for i-th symbol
    uxint           uAlphabetSize,          // size of the alphabet
    uxint          *puCost                  // cost of encoding (sum count[i] * bits[i])
)
{
    HUFFMAN_RADIX_SORT_CHAIN    Chain[256];
    HUFFMAN_NODE   *pNode;
    uxint           uCost[HUFFMAN_CODEWORD_LENGTH_LIMIT];
    uxint           i;
    uxint           n;

    memset (puBits, 0, sizeof (puBits[0]) * uAlphabetSize);
    memset (uCost, 0, sizeof (uCost));

    //
    // assign # of bits to symbols
    //
    i = HUFFMAN_CODEWORD_LENGTH_LIMIT; // Looks like this can be set to maxCodewordLength?
    n = 0;
    for (pNode = pSortedList; pNode != NULL; pNode = pNode->m_pNext)
    {
        if (n == 0)
        {
            do
            {
                RETAIL_ASSERT (i != 0, "");
                i -= 1;
                n = (uxint) puBitLengthCount[i]; // means n = Number of symbols with encoded-length i
            }
            while (n == 0);
        }
        puBits[pNode->m_uSymbol] = (HUFFMAN_MASK) i; // The next n symbols have encoded length i.
        uCost[i] += pNode->m_uCount; // Cost of encoding for symbols of length i = sum of counts of all symbols whose encoding length = i.
        n -= 1;
    }

    //
    // sort symbols by length of code word in stable manner
    //
    RETAIL_ASSERT (n == 0, "n=0x%Ix", n);

    for (i = 0; i < HUFFMAN_CODEWORD_LENGTH_LIMIT; ++i)
    {
        Chain[i].m_ppLast = &Chain[i].m_pHead;
    }
    // There is one chain for each codeword length.
    // chain n connects all nodes of codeword length n with head pointed to by m_pHead
    // and the nodes linked using their respective pNext pointers.

    pNode = pSortedList;
    for (i = 0; i < uAlphabetSize; ++i)
    {
        n = puBits[i];
        if (n != 0)
        {
            RETAIL_ASSERT (pNode != NULL, "");
            pNode->m_uSymbol = (UInt16) i ;
            pNode->m_uBits   = (UInt16) n;
            RETAIL_ASSERT (n < HUFFMAN_CODEWORD_LENGTH_LIMIT, "n=%Iu", n);
            *(Chain[n].m_ppLast) = pNode;
            Chain[n].m_ppLast = &pNode->m_pNext;
            pNode = pNode->m_pNext;
        }
    }
    RETAIL_ASSERT (pNode == NULL, "");

    // Terminate the chains.
    i = HUFFMAN_CODEWORD_LENGTH_LIMIT;
    pNode = NULL;
    do
    {
        --i;
        *(Chain[i].m_ppLast) = pNode;
        pNode = Chain[i].m_pHead;
//        Chain[i].m_ppLast = &Chain[i].m_pHead;
    }
    while (i != 0);

    if (puCost != NULL)
    {
        *puCost = 0;
        n = 0;
        i = HUFFMAN_CODEWORD_LENGTH_LIMIT - 1;
        do
        {
            n += uCost[i];
            //BUGBUG: According to this computation, cost for i'th symbol = sum of costs of (i.. HUFFMAN_CODEWORD_LENGTH - 1 ) symbols.
            *puCost += n;
            --i;
        }
        while (i != 0);
    }

    return (pNode);

Failure:
    return (NULL);
}

//
// Create canonical codewords
//
static
BOOL
HuffmanCreateCodewords (
    XPRESS9_STATUS *pStatus,
    HUFFMAN_NODE   *pNode,
    HUFFMAN_MASK   *puMask,
    uxint           uAlphabetSize
)
{
    uxint   uBits;
    uxint   uMask;

    uBits = 1;
    uMask = 0;
    for (;pNode != NULL; pNode = pNode->m_pNext)
    {
        // pNode->m_uBits >= uBits is to ascertain that the symbols are sorted in decreasing order of occurence count or increasing order of number of bits
        RETAIL_ASSERT (
            pNode->m_uBits < HUFFMAN_CODEWORD_LENGTH_LIMIT && pNode->m_uBits >= uBits && pNode->m_uSymbol <= uAlphabetSize,
            "pNode->m_uBits=%u uBits=%Iu pNode->m_uSymbol=%u uAlphabetSize=%Iu",
            pNode->m_uBits,
            uBits,
            pNode->m_uSymbol,
            uAlphabetSize
        );

        uMask <<= pNode->m_uBits - uBits;
        uBits = pNode->m_uBits;

        RETAIL_ASSERT (pNode->m_uSymbol < uAlphabetSize, "Symbol=%u AlphabetSize=%Iu", pNode->m_uSymbol, uAlphabetSize);
        RETAIL_ASSERT (uBits < (1 << HUFFMAN_CODEWORD_LENGTH_BITS), "uBits=%Iu", uBits);
        puMask[pNode->m_uSymbol] = (HUFFMAN_MASK) ((HuffmanReverseMask (uMask, uBits) << HUFFMAN_CODEWORD_LENGTH_BITS) + uBits);

        uMask += 1;
        RETAIL_ASSERT (uMask <= (((uxint) 1) << uBits), "uMask=%Iu uBits=%Iu", (uxint) uMask, uBits);
    }

    RETAIL_ASSERT (uMask == (((uxint) 1) << uBits), "uMask=%Iu uBits=%Iu", (uxint) uMask, uBits);

    return (TRUE);

Failure:
    return (FALSE);
}


//
// Create Huffman codewords
//
// Returns total number of symbols with non-0 counts (0 or >= 2), (uAlphabetSize+1) in case of memory corruption
//
XPRESS9_INTERNAL
uxint
Xpress9HuffmanCreateTree (
    XPRESS9_STATUS *pStatus,
    const HUFFMAN_COUNT *puCount,       // IN: uCount[i] = occurance count of i-th symbol
    uxint           uAlphabetSize,      // IN: number of symbols in the alphabet
    uxint           uMaxCodewordLength, // IN: max codeword length (inclusively)
    HUFFMAN_NODE   *pTemp,              // OUT: scratch area (ALPHABET*2)
    HUFFMAN_MASK   *puMask,             // OUT: encoded codeword + # of bits (least significant HUFFMAN_CODEWORD_LENGTH_BITS)
    uxint          *puCost              // OUT (optional, may be NULL): cost of encoding
)
{
    HUFFMAN_NODE   *pSortedList;
    HUFFMAN_NODE   *pTreeRoot;
    uxint           uBitLengthCount[HUFFMAN_CODEWORD_LENGTH_LIMIT];
    uxint           n;
    BOOL            fResult;

    RETAIL_ASSERT (pStatus->m_uStatus == Xpress9Status_OK, "BadStatus");

    RETAIL_ASSERT (uMaxCodewordLength <= HUFFMAN_MAX_CODEWORD_LENGTH, "uMaxCodewordLength=%Iu Limit=%Iu", uMaxCodewordLength, (uxint) HUFFMAN_MAX_CODEWORD_LENGTH);

    n = HuffmanCreateTemp (pTemp, puCount, uAlphabetSize);
    if (n <= 1)
    {
        if (n == 1)
        {
            uxint iSymbol;
            // Note that in this case the tree is full binary, since it is a single node tree.
            memset (puMask, 0, sizeof (puMask[0]) * uAlphabetSize);
            iSymbol = pTemp[0].m_uSymbol;
            puMask[iSymbol] = (0 << HUFFMAN_CODEWORD_LENGTH_BITS) + 1;
            if (NULL != puCost)
            {
                *puCost = pTemp[0].m_uCount;
            }
        }

        goto Success;
    }

    // At this point the "first half" of the scratch area (pointed to by pTemp) contains a list of nodes 
    // that contains non-zero node counts.
    if (n < HUFFMAN_RADIX_MERGE_SORT_THRESHOLD)
    {
        pSortedList = HuffmanMergeSort (pTemp);
        RETAIL_ASSERT (pSortedList != NULL, "HuffmanMergeSort");
    }
    else
    {
        pSortedList = HuffmanRadixSort (pStatus, pTemp);
        if (pSortedList == NULL)
        {
            RETAIL_ASSERT (pStatus->m_uStatus != Xpress9Status_OK, "HuffmanRadixSort failed");
            goto Failure;
        }
    }

    // At this point pSortedList can be used to traverse the "first half" of scratch area in sorted order of symbol counts.
#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ERROR_STATUS_AND_POSITION
    {
        HUFFMAN_NODE *p;
        for (p = pSortedList; p->m_pNext != NULL; p = p->m_pNext)
        {
            RETAIL_ASSERT (
                p->m_uCount < p->m_pNext->m_uCount || 
                (p->m_uCount == p->m_pNext->m_uCount && p->m_uSymbol < p->m_pNext->m_uSymbol),
                "Current=(%u 0x%Ix) Next=(%u 0x%Ix)",
                p->m_uSymbol,
                p->m_uCount,
                p->m_pNext->m_uSymbol,
                p->m_pNext->m_uCount
            );
        }
    }
#endif /* XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT */

    // Using the pSortedList, create the huffman tree in the "second half" of the scratch area.
    //  pTemp                           
    //   |------------------------------|----------------------------------------|
    //   |Space used for the leaf nodes | Space used for the intermediate nodes  |
    //   |                              | in huffman tree                        |
    //   |------------------------------|----------------------------------------|
    ///////////////////////////////////////////////////////////////////////////////
    pTreeRoot = HuffmanCreateTree (pStatus, pSortedList, pTemp + n);
    if (pTreeRoot == NULL)
    {
        RETAIL_ASSERT (pStatus->m_uStatus != Xpress9Status_OK, "HuffmanCreateTree failed");
        goto Failure;
    }

    fResult = HuffmanComputeBitLength (pStatus, pTreeRoot, uBitLengthCount, uAlphabetSize);
    if (!fResult)
    {
        RETAIL_ASSERT (pStatus->m_uStatus != Xpress9Status_OK, "HuffmanComputeBitLength failed");
        goto Failure;
    }

    fResult = HuffmanTruncateTree (pStatus, uBitLengthCount, uMaxCodewordLength);
    if (!fResult)
    {
        RETAIL_ASSERT (pStatus->m_uStatus != Xpress9Status_OK, "HuffmanTruncateTree failed");
        goto Failure;
    }

    pSortedList = HuffmanCanonizeTree (pStatus, pSortedList, uBitLengthCount, puMask, uAlphabetSize, puCost);
    if (pSortedList == NULL)
    {
        RETAIL_ASSERT (pStatus->m_uStatus != Xpress9Status_OK, "HuffmanCanonizeTree failed");
        goto Failure;
    }

    fResult = HuffmanCreateCodewords (pStatus, pSortedList, puMask, uAlphabetSize);
    if (!fResult)
    {
        RETAIL_ASSERT (pStatus->m_uStatus != Xpress9Status_OK, "HuffmanCreateCodewords failed");
        goto Failure;
    }

Success:
    return (n);

Failure:
    return (uAlphabetSize + 1);
}


#pragma warning (push)
#pragma warning (disable: 4127) // conditional expression is constant


//
// Write encoded Huffman table into output bitstream
//
// Returns TRUE on success, FALSE on failure (memory corruptions do happen)
//
XPRESS9_INTERNAL
BOOL
Xpress9HuffmanEncodeTable (
    XPRESS9_STATUS *pStatus,
    BIO_STATE      *pBioState,          // bitstream I/O state
    HUFFMAN_MASK   *pMask,              // array of packed (codeword value, codeword length) for all symbols
    uxint           uAlphabetSize,      // number of symbols in the alphabet
    uxint           uFillBoundary       // fill boundary
)
{
    HUFFMAN_NODE    HuffmanCreateTreeTemp[HUFFMAN_ENCODED_TABLE_SIZE*2];
    HUFFMAN_COUNT   uCount[HUFFMAN_ENCODED_TABLE_SIZE];
    HUFFMAN_MASK    uMask[HUFFMAN_ENCODED_TABLE_SIZE];
    uxint   uPrevSymbol;
    uxint   uSymbol;
    uxint   k;
    uxint   i;
    BIO_DECLARE();

    RETAIL_ASSERT (pStatus->m_uStatus == Xpress9Status_OK, "BadStatus");

    // Ensures that the uFillBoundary is a power of 2.
    RETAIL_ASSERT (
        uFillBoundary != 0 && 
        (uFillBoundary & (uFillBoundary - 1)) == 0 &&
        (uAlphabetSize & (uFillBoundary - 1)) == 0,
        "uFillBoundary=%Iu uAlphabetSize=%Iu",
        uFillBoundary,
        uAlphabetSize
    );
    // Encode the code word lengths using a separate huffman table.

    memset (uCount, 0, sizeof (uCount));

    uPrevSymbol = 8;
    for (i = 0; i < uAlphabetSize;)
    {
        k = HUFFMAN_GET_CODEWORD_LENGTH (pMask[i]);
        RETAIL_ASSERT (k <= HUFFMAN_MAX_CODEWORD_LENGTH, "k=%Iu", k);
        if (k != 0)
        {
            uSymbol = k;
            if (k == uPrevSymbol)
            {
                uSymbol = HUFFMAN_ENCODED_TABLE_PREV;
            }
            else
            {
                uPrevSymbol = k;
                if (i >= uFillBoundary)
                {
                    k = HUFFMAN_GET_CODEWORD_LENGTH (pMask[i - uFillBoundary]);
                    if (uSymbol == k)
                    {
                        uSymbol = HUFFMAN_ENCODED_TABLE_ROW_0;
                    }
                    else if (uSymbol == k + 1)
                    {
                        uSymbol = HUFFMAN_ENCODED_TABLE_ROW_1;
                    }
                }
            }
            ++uCount[uSymbol];
            ++i;
        }
        else
        { // These symbols are part of the alphabet but do not occur in the input.
            // zeroes get special treatment
            k = i;
            do
            {
                ++i;
            }
            while (i < uAlphabetSize && HUFFMAN_GET_CODEWORD_LENGTH (pMask[i]) == 0);

            ////////////////////////////////////////////////////////////////////////////////////////
            //  k = The largest multiple of uFillBoundary between k and i if there exists one.
            // There are two cases:
            // 1. There is at least a single multiple of uFillBoundary between k and i.
            //    
            //  -----------|-----------|----------|----------|-----------|----------|--------|
            //             ma           k          mb         mc          md         me      i
            //    => k = me
            //
            // 2. There is no multiple of uFillBoundary between k and i.
            //   -----------|-----------|-------|---|----------|-----------|----------|--------
            //             ma           k       i   mb         mc          md         me      
            //    => k = k                             
            ////////////////////////////////////////////////////////////////////////////////////////
            while ((k ^ i) >= uFillBoundary) 
            {// Loop is entered if there exists a multiple of uFillBoundary between k and i.
                ++uCount[HUFFMAN_ENCODED_TABLE_FILL];
                // the first time loop is entered, the last lg(uFillBoundary) bits are reset.
                // k is set to the closest multiple of uFillBoundary that is larger than k.
                k = (k & ~(uFillBoundary - 1)) + uFillBoundary; 
                RETAIL_ASSERT (k <= i, "k=%Iu i=%Iu uFillBoundary=%Iu", k, i, uFillBoundary);
            }

            k = i - k;
            if (k > 0)
            {
                if (k < HUFFMAN_ENCODED_TABLE_ZERO_REPT_MIN_COUNT)
                {
                    uCount[0] += (HUFFMAN_COUNT) k;
                }
                else
                {
                    uCount[HUFFMAN_ENCODED_TABLE_ZERO_REPT] += 1;
                }
            }
        }
    }

    BIO_STATE_ENTER ((*pBioState));
    k = Xpress9HuffmanCreateTree (
        pStatus,
        uCount, // occurrence count of each symbol
        sizeof (uCount) / sizeof (uCount[0]), // alphabet size
        8, // Maxcodeword length
        HuffmanCreateTreeTemp, // 
        uMask,
        NULL
    );
    if (k == 0 || k > uAlphabetSize)
    {
        RETAIL_ASSERT (pStatus->m_uStatus != Xpress9Status_OK, "Xpress9HuffmanCreateTree failed");
        goto Failure;
    }

    //
    // write small table
    //
    uPrevSymbol = 4;
    for (i = 0; i < HUFFMAN_ENCODED_TABLE_SIZE; ++i)
    {
        k = HUFFMAN_GET_CODEWORD_LENGTH (uMask[i]);
        if (k == uPrevSymbol)
        {
            BIOWR (0, 1); // save bits by writing just a 0 to indicate that this symbol is same as previous symbol.
        }
        else
        {
            BIOWR (1, 1); // write a '1' bit to indicate this symbol is different from the previous one.
            // BUGBUG: Why do we need this check, we are writing three bits anyway, why not write the value 
            // of k in both the following two cases? Need to look at decompressor code.
            if (k > uPrevSymbol)
            {
                BIOWR (k - 1, 3); // 3 -- because max code word length passed to Xpress9HuffmanCreateTree is 8, lg8 = 3.
            }
            else
            {
                BIOWR (k, 3);
            }
            uPrevSymbol = k;
        }
    }

#if 0
for (i = 0; i < uAlphabetSize;)
{
    k = HUFFMAN_GET_CODEWORD_LENGTH (pMask[i]);
    printf (" %2Iu", k);
    ++i;
    if ((i & 15) == 0)
        printf ("\n");
}
printf ("\n");
#endif

    // Write the actual table
    // With canonical huffman code, only lengths of the symbols need to be transmitted for decoding.
    // http://en.wikipedia.org/wiki/Canonical_Huffman_code

    uPrevSymbol = 8;
    for (i = 0; i < uAlphabetSize;)
    {
        k = HUFFMAN_GET_CODEWORD_LENGTH (pMask[i]);
        RETAIL_ASSERT (k <= HUFFMAN_MAX_CODEWORD_LENGTH, "k=%Iu", k);
        if (k != 0)
        {
            uSymbol = k;
            if (k == uPrevSymbol)
            {
                uSymbol = HUFFMAN_ENCODED_TABLE_PREV;
            }
            else
            {
                uPrevSymbol = k;
                if (i >= uFillBoundary)
                {
                    k = HUFFMAN_GET_CODEWORD_LENGTH (pMask[i - uFillBoundary]);
                    if (uSymbol == k)
                    {
                        uSymbol = HUFFMAN_ENCODED_TABLE_ROW_0;
                    }
                    else if (uSymbol == k + 1)
                    {
                        uSymbol = HUFFMAN_ENCODED_TABLE_ROW_1;
                    }
                }
            }
            HUFFMAN_WRITE_SYMBOL (uMask[uSymbol]);
            ++i;
        }
        else
        {
            // zeroes get special treatment
            k = i;
            do
            {
                ++i;
            }
            while (i < uAlphabetSize && HUFFMAN_GET_CODEWORD_LENGTH (pMask[i]) == 0);

            while ((k ^ i) >= uFillBoundary)
            {
                HUFFMAN_WRITE_SYMBOL (uMask[HUFFMAN_ENCODED_TABLE_FILL]);
                k = (k & ~(uFillBoundary - 1)) + uFillBoundary;
                RETAIL_ASSERT (k <= i, "k=%Iu i=%Iu uFillBoundary=%Iu", k, i, uFillBoundary);
            }

            k = i - k;
            if (k > 0)
            {
                if (k < HUFFMAN_ENCODED_TABLE_ZERO_REPT_MIN_COUNT)
                {
                    do
                    {
                        HUFFMAN_WRITE_SYMBOL (uMask[0]);
                        --k;
                    }
                    while (k != 0);
                }
                else
                {
                    HUFFMAN_WRITE_SYMBOL (uMask[HUFFMAN_ENCODED_TABLE_ZERO_REPT]);
                    k -= HUFFMAN_ENCODED_TABLE_ZERO_REPT_MIN_COUNT;

                    if (k < 3)
                    {
                        BIOWR (k, 2);
                    }
                    else
                    {
                        BIOWR (3, 2);
                        k -= 3;
                        while (k >= 7)
                        {
                            BIOWR (7, 3);
                            k -= 7;
                        }
                        BIOWR (k, 3);
                    }
                }
            }
        }
    }

    BIO_STATE_LEAVE ((*pBioState));
    return (TRUE);

Failure:
    return (FALSE);
}


//
// Create huffman tables and encode them into output bitstream
//
XPRESS9_INTERNAL
void
Xpress9HuffmanCreateAndEncodeTable (
    XPRESS9_STATUS *pStatus,
    BIO_STATE      *pBioState,       // out bitstream I/O state
    const HUFFMAN_COUNT *puCount,       // IN:  uCount[i] = occurance count of i-th symbol
    uxint           uAlphabetSize,      // IN:  number of symbols in the alphabet
    uxint           uMaxCodewordLength, // IN:  max codeword length (inclusively)
    HUFFMAN_NODE   *pTemp,              // OUT: scratch area (ALPHABET*2)
    HUFFMAN_MASK   *puMask,             // OUT: encoded codeword + # of bits (least significant HUFFMAN_CODEWORD_LENGTH_BITS)
    uxint          *puCost,             // OUT  (optional, may be NULL): cost of encoding
    uxint           uFillBoundary       // IN:  fill boundary
)
{
    BIO_STATE BioState[2];
    MSB_T   uBits;
    uxint   uThreshold;
    uxint   uCount0;
    uxint   uCount1;
    uxint   uCandidateCodeCost;
    uxint   uHuffmanCodeCost = 0;
    uxint   i;
    BIO_DECLARE();


    //
    // for given alphabet size |A| return value N <= |A| such that first N symbols of the alphabet
    // may be encoded with M=MSB(|A|) bits, remaining |A|-N symbols may be encoded with (M+1) bits
    //
    GET_MSB (uBits, uAlphabetSize);
    uThreshold = POWER2 (uBits + 1) - uAlphabetSize;

    uCount0 = 0;
    for (i = 0; i < uThreshold; ++i)
    {
        uCount0 += puCount[i];
    }

    uCount1 = 0;
    for (; i < uAlphabetSize; ++i)
    {
        uCount1 += puCount[i];
    }

    uCandidateCodeCost = (uCount0 + uCount1) * ((uxint) uBits) + uCount1 + 3;

    (void) Xpress9HuffmanCreateTree (pStatus, puCount, uAlphabetSize, uMaxCodewordLength, pTemp, puMask, &uHuffmanCodeCost);
    if (pStatus->m_uStatus != Xpress9Status_OK)
        goto Failure;


    BioState[0] = *pBioState;
    BIO_STATE_ENTER (BioState[0]);
    BIOWR (1, 3); //1 Differentiates from stored encoding.
    BIO_STATE_LEAVE (BioState[0]);
    (void) Xpress9HuffmanEncodeTable (pStatus, &BioState[0], puMask, uAlphabetSize, uFillBoundary);
    if (pStatus->m_uStatus != Xpress9Status_OK)
        goto Failure;

    uHuffmanCodeCost += (
        (BIOWR_TELL_OUTPUT_PTR_STATE (BioState[0]) - BIOWR_TELL_OUTPUT_PTR_STATE (*pBioState)) * 8
        + BIOWR_TELL_SHIFT_REGISTER_BITS_STATE (BioState[0])
        - BIOWR_TELL_SHIFT_REGISTER_BITS_STATE (*pBioState)
    );

    if (uCandidateCodeCost <= uHuffmanCodeCost)
    {
        BIO_STATE_ENTER (*pBioState);
        BIOWR (0, 3); // 0 is to differentiate from encoding done by Xpress9HuffmanEncodeTable above.
        BIO_STATE_LEAVE (*pBioState);

        for (i = 0; i < uThreshold; ++i)
        {
            puMask[i] = (HUFFMAN_MASK) ((HuffmanReverseMask (i, uBits) << HUFFMAN_CODEWORD_LENGTH_BITS) + uBits);
        }

        uCount0 = i << 1;
        uBits += 1;
        for (; i < uAlphabetSize; ++i)
        {
            puMask[i] = (HUFFMAN_MASK) ((HuffmanReverseMask (uCount0, uBits) << HUFFMAN_CODEWORD_LENGTH_BITS) + uBits);
            uCount0 += 1;
        }

        if (puCost != NULL)
            *puCost = uCandidateCodeCost;

        goto Done;
    }

#if 0
    if (fOptimizeTable)
    {
        HUFFMAN_MASK  *puTempMask;
        HUFFMAN_COUNT *puTempCount;
        uxint   uShift;

        for (uShift = 1; (uFillBoundary >> uShift) != 0 && uShift <= 6; ++uShift)
        {
            
            puTempCount = (HUFFMAN_COUNT *) (pTemp + uAlphabetSize);
            memset (puTempCount, 0, sizeof (puTempCount[0]) * (uAlphabetSize >> uShift));
            puTempMask = (HUFFMAN_MASK *) (puTempCount + (uAlphabetSize >> uShift));

            for (i = 0; i < uAlphabetSize; ++i)
                puTempCount[i >> uShift] += puCount[i];

            (void) Xpress9HuffmanCreateTree (pStatus, puTempCount, uAlphabetSize >> uShift, uMaxCodewordLength - uShift, pTemp, puTempMask, &uCandidateCodeCost);
            if (pStatus->m_uStatus != Xpress9Status_OK)
                goto Failure;

            BioState[1] = *pTmpBioState;
            BIO_STATE_ENTER (BioState[1]);
            BIOWR (i + 1, 3);
            BIO_STATE_LEAVE (BioState[1]);
            (void) Xpress9HuffmanEncodeTable (pStatus, &BioState[1], puMask, uAlphabetSize >> uShift, uFillBoundary >> uShift);
            if (pStatus->m_uStatus != Xpress9Status_OK)
                goto Failure;

            uCandidateCodeCost += ((char *) BioState[1].m_pBitStreamPtr - (char *) pTmpBioState->m_pBitStreamPtr) * 8 + BIOWR_TELL_SHIFT_REGISTER_BITS_STATE (BioState[1]);

            if (uHuffmanCodeCost <= uCandidateCodeCost)
                break;

            i = BioState[1].m_pBitStreamPtr - pTmpBioState->m_pBitStreamPtr;
            memcpy (pBioState->m_pBitStreamPtr, pTmpBioState->m_pBitStreamPtr, i);
            BioState[0] = BioState[1];
            BioState[0].m_pBitStreamPtr = pBioState->m_pBitStreamPtr + i;
            uHuffmanCodeCost = uCandidateCodeCost;

            for (i = 0; i < uAlphabetSize; ++i)
            {
                uCount0 = HUFFMAN_GET_CODEWORD_LENGTH (puMask[i >> uShift]);
                puMask[i] = (HUFFMAN_MASK) (puMask[i >> uShift] + uShift + (HuffmanReverseMask (i & MASK2 (uShift), uShift) << (uCount0 + HUFFMAN_CODEWORD_LENGTH_BITS)));
            }
        }
    }
#endif

    *pBioState = BioState[0];
    if (puCost != NULL)
        *puCost = uHuffmanCodeCost;
Done:;
Failure:;
}


#pragma warning (pop)
