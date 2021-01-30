// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "Xpress9Internal.h"


#define HUFFMAN_RADIX_MERGE_SORT_THRESHOLD  64




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
        ++pStack;
        ppLink = NULL;

        pRecord1 = pUnsorted;
        pRecord2 = pRecord1->m_pNext;
        pStack->m_pHead = pRecord1;
        pStack->m_uBits = 1;
        if (pRecord2 == NULL)
        {
            pUnsorted = NULL;
            break;
        }
        else
        {
            pUnsorted = pRecord2->m_pNext;
            if (pRecord1->m_uCount > pRecord2->m_uCount)
            {
                pStack->m_pHead = pRecord2;
                pRecord2->m_pNext = pRecord1;
                pRecord2 = pRecord1;
            }
            pRecord2->m_pNext = NULL;
            if (pUnsorted == NULL)
                break;
        }

        while (pStack->m_uBits == pStack[-1].m_uBits)
        {
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

    while (pStack[-1].m_uBits != 0)
    {
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

    for (i = 0; i < 256; ++i)
    {
        Chain[i].m_ppLast = &Chain[i].m_pHead;
    }

    uShift = 0;
    do
    {
        RETAIL_ASSERT (pNode != NULL, "");

        do
        {
            i = (pNode->m_uCount >> uShift) & 255;
            *(Chain[i].m_ppLast) = pNode;
            Chain[i].m_ppLast = &pNode->m_pNext;
            pNode = pNode->m_pNext;
        }
        while (pNode != NULL);

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

        while (pRoot->m_pSon[0] != NULL)
        {
            RETAIL_ASSERT (pRoot->m_pSon[1] != NULL, "LeftSonIsNotNullButRightIs");
            RETAIL_ASSERT (pRoot->m_uSymbol > uAlphabetSize, "IntermediateNodeSymbol=%Iu AlphabetSize=%Iu", pRoot->m_uSymbol, uAlphabetSize);
            uBits += 1;
            pTemp = pRoot->m_pSon[1];
            pTemp->m_uBits = (UInt16) uBits;
            *pStack++ = pTemp;
            RETAIL_ASSERT (pStack < Stack + sizeof (Stack) / sizeof (Stack[0]), "HuffmanTreeIsWayTooDeep");
            pRoot = pRoot->m_pSon[0];
            pRoot->m_uBits = (UInt16) uBits;
        }

        RETAIL_ASSERT (pRoot->m_pSon[1] == NULL, "LeftSonIsNullButRightIsNotNull");
        RETAIL_ASSERT (pRoot->m_uSymbol < uAlphabetSize, "LeafSymbol=%Iu AlphabetSize=%Iu", pRoot->m_uSymbol, uAlphabetSize);
        RETAIL_ASSERT (uBits < HUFFMAN_CODEWORD_LENGTH_LIMIT, "uBits=%Iu", uBits);

        puBitLengthCount[uBits] += 1;
    }
    while (pStack > Stack);

    return (TRUE);

Failure:
    return (FALSE);
}


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
                do
                {
                    --j;
                    RETAIL_ASSERT (j > 0, "CorruptHuffmanFrequencyTable");
                }
                while (puBitLengthCount[j] == 0);
            }

            RETAIL_ASSERT (puBitLengthCount[j] > 0, "CorruptHuffmanFrequencyTableIndex");

            puBitLengthCount[j]     -= 1;
            puBitLengthCount[j + 1] += 2;
            puBitLengthCount[i - 1] += 1;
            puBitLengthCount[i]     -= 2;

            j += 1;
        }
    }

    return (TRUE);

Failure:
    return (FALSE);
}

static
HUFFMAN_NODE *
HuffmanCanonizeTree (
    XPRESS9_STATUS *pStatus,
    HUFFMAN_NODE   *pSortedList,
    uxint           puBitLengthCount[HUFFMAN_CODEWORD_LENGTH_LIMIT],
    HUFFMAN_MASK   *puBits,
    uxint           uAlphabetSize,
    uxint          *puCost
)
{
    HUFFMAN_RADIX_SORT_CHAIN    Chain[256];
    HUFFMAN_NODE   *pNode;
    uxint           uCost[HUFFMAN_CODEWORD_LENGTH_LIMIT];
    uxint           i;
    uxint           n;

    memset (puBits, 0, sizeof (puBits[0]) * uAlphabetSize);
    memset (uCost, 0, sizeof (uCost));

    i = HUFFMAN_CODEWORD_LENGTH_LIMIT;
    n = 0;
    for (pNode = pSortedList; pNode != NULL; pNode = pNode->m_pNext)
    {
        if (n == 0)
        {
            do
            {
                RETAIL_ASSERT (i != 0, "");
                i -= 1;
                n = (uxint) puBitLengthCount[i];
            }
            while (n == 0);
        }
        puBits[pNode->m_uSymbol] = (HUFFMAN_MASK) i;
        uCost[i] += pNode->m_uCount;
        n -= 1;
    }

    RETAIL_ASSERT (n == 0, "n=0x%Ix", n);

    for (i = 0; i < HUFFMAN_CODEWORD_LENGTH_LIMIT; ++i)
    {
        Chain[i].m_ppLast = &Chain[i].m_pHead;
    }

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

    i = HUFFMAN_CODEWORD_LENGTH_LIMIT;
    pNode = NULL;
    do
    {
        --i;
        *(Chain[i].m_ppLast) = pNode;
        pNode = Chain[i].m_pHead;
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
            *puCost += n;
            --i;
        }
        while (i != 0);
    }

    return (pNode);

Failure:
    return (NULL);
}

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


XPRESS9_INTERNAL
uxint
Xpress9HuffmanCreateTree (
    XPRESS9_STATUS *pStatus,
    const HUFFMAN_COUNT *puCount,
    uxint           uAlphabetSize,
    uxint           uMaxCodewordLength,
    HUFFMAN_NODE   *pTemp,
    HUFFMAN_MASK   *puMask,
    uxint          *puCost
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
#endif 

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
#pragma warning (disable: 4127)


XPRESS9_INTERNAL
BOOL
Xpress9HuffmanEncodeTable (
    XPRESS9_STATUS *pStatus,
    BIO_STATE      *pBioState,
    HUFFMAN_MASK   *pMask,
    uxint           uAlphabetSize,
    uxint           uFillBoundary
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

    RETAIL_ASSERT (
        uFillBoundary != 0 && 
        (uFillBoundary & (uFillBoundary - 1)) == 0 &&
        (uAlphabetSize & (uFillBoundary - 1)) == 0,
        "uFillBoundary=%Iu uAlphabetSize=%Iu",
        uFillBoundary,
        uAlphabetSize
    );

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
        {
            k = i;
            do
            {
                ++i;
            }
            while (i < uAlphabetSize && HUFFMAN_GET_CODEWORD_LENGTH (pMask[i]) == 0);

            while ((k ^ i) >= uFillBoundary) 
            {// Loop is entered if there exists a multiple of uFillBoundary between k and i.
                ++uCount[HUFFMAN_ENCODED_TABLE_FILL];
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
        uCount,
        sizeof (uCount) / sizeof (uCount[0]),
        8,
        HuffmanCreateTreeTemp,
        uMask,
        NULL
    );
    if (k == 0 || k > uAlphabetSize)
    {
        RETAIL_ASSERT (pStatus->m_uStatus != Xpress9Status_OK, "Xpress9HuffmanCreateTree failed");
        goto Failure;
    }

    uPrevSymbol = 4;
    for (i = 0; i < HUFFMAN_ENCODED_TABLE_SIZE; ++i)
    {
        k = HUFFMAN_GET_CODEWORD_LENGTH (uMask[i]);
        if (k == uPrevSymbol)
        {
            BIOWR (0, 1);
        }
        else
        {
            BIOWR (1, 1);
            if (k > uPrevSymbol)
            {
                BIOWR (k - 1, 3);
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


XPRESS9_INTERNAL
void
Xpress9HuffmanCreateAndEncodeTable (
    XPRESS9_STATUS *pStatus,
    BIO_STATE      *pBioState,
    const HUFFMAN_COUNT *puCount,
    uxint           uAlphabetSize,
    uxint           uMaxCodewordLength,
    HUFFMAN_NODE   *pTemp,
    HUFFMAN_MASK   *puMask,
    uxint          *puCost,
    uxint           uFillBoundary
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
    BIOWR (1, 3);
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
        BIOWR (0, 3);
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
