// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "Xpress9Internal.h"
#include <stddef.h>     // for "offsetof"


#define GET_DECODER_STATE(pStatus,Failure,pDecoder,pXpress9Decoder) do {    \
    if (pXpress9Decoder == NULL)                                            \
    {                                                                       \
        SET_ERROR (Xpress9Status_BadArgument, pStatus, "pDecoderIsNULL");   \
        goto Failure;                                                       \
    }                                                                       \
    pDecoder = (LZ77_DECODER *) pXpress9Decoder;                            \
    if (pDecoder->m_uMagic != XPRESS9_DECODER_MAGIC)                        \
    {                                                                       \
        SET_ERROR (                                                         \
            Xpress9Status_BadArgument,                                      \
            pStatus,                                                        \
            "pDecoder->m_uMagic=0x%Ix expecting 0x%Ix",                     \
            pDecoder->m_uMagic,                                             \
            (uxint) XPRESS9_DECODER_MAGIC                                   \
        );                                                                  \
        goto Failure;                                                       \
    }                                                                       \
} while (0)


#define LZ77_MTF                    0
#define LZ77_PTR_MIN_MATCH_LENGTH   3
#define LZ77_MTF_MIN_MATCH_LENGTH   "ShallNotBeUsed"
#define Xpress9Lz77Dec              Xpress9Lz77Dec_MtfCount0_Ptr3
#include "Xpress9Lz77Dec.i"

#define LZ77_MTF                    0
#define LZ77_PTR_MIN_MATCH_LENGTH   4
#define LZ77_MTF_MIN_MATCH_LENGTH   "ShallNotBeUsed"
#define Xpress9Lz77Dec              Xpress9Lz77Dec_MtfCount0_Ptr4
#include "Xpress9Lz77Dec.i"

#define LZ77_MTF                    2
#define LZ77_PTR_MIN_MATCH_LENGTH   3
#define LZ77_MTF_MIN_MATCH_LENGTH   2
#define Xpress9Lz77Dec              Xpress9Lz77Dec_MtfCount2_Ptr3_Mtf2
#include "Xpress9Lz77Dec.i"


#define LZ77_MTF                    2
#define LZ77_PTR_MIN_MATCH_LENGTH   3
#define LZ77_MTF_MIN_MATCH_LENGTH   3
#define Xpress9Lz77Dec              Xpress9Lz77Dec_MtfCount2_Ptr3_Mtf3
#include "Xpress9Lz77Dec.i"


#define LZ77_MTF                    2
#define LZ77_PTR_MIN_MATCH_LENGTH   4
#define LZ77_MTF_MIN_MATCH_LENGTH   2
#define Xpress9Lz77Dec              Xpress9Lz77Dec_MtfCount2_Ptr4_Mtf2
#include "Xpress9Lz77Dec.i"


#define LZ77_MTF                    2
#define LZ77_PTR_MIN_MATCH_LENGTH   4
#define LZ77_MTF_MIN_MATCH_LENGTH   3
#define Xpress9Lz77Dec              Xpress9Lz77Dec_MtfCount2_Ptr4_Mtf3
#include "Xpress9Lz77Dec.i"


#define LZ77_MTF                    4
#define LZ77_PTR_MIN_MATCH_LENGTH   3
#define LZ77_MTF_MIN_MATCH_LENGTH   2
#define Xpress9Lz77Dec              Xpress9Lz77Dec_MtfCount4_Ptr3_Mtf2
#include "Xpress9Lz77Dec.i"


#define LZ77_MTF                    4
#define LZ77_PTR_MIN_MATCH_LENGTH   3
#define LZ77_MTF_MIN_MATCH_LENGTH   3
#define Xpress9Lz77Dec              Xpress9Lz77Dec_MtfCount4_Ptr3_Mtf3
#include "Xpress9Lz77Dec.i"


#define LZ77_MTF                    4
#define LZ77_PTR_MIN_MATCH_LENGTH   4
#define LZ77_MTF_MIN_MATCH_LENGTH   2
#define Xpress9Lz77Dec              Xpress9Lz77Dec_MtfCount4_Ptr4_Mtf2
#include "Xpress9Lz77Dec.i"


#define LZ77_MTF                    4
#define LZ77_PTR_MIN_MATCH_LENGTH   4
#define LZ77_MTF_MIN_MATCH_LENGTH   3
#define Xpress9Lz77Dec              Xpress9Lz77Dec_MtfCount4_Ptr4_Mtf3
#include "Xpress9Lz77Dec.i"


static
void
Xpress9DecoderGetProc (
    XPRESS9_STATUS      *pStatus,
    LZ77_DECODER        *pDecoder
)
{
    pDecoder->m_DecodeData.m_pLz77DecProc = NULL;
    if (pDecoder->m_DecodeData.m_uMtfEntryCount == 0)
    {
        if (pDecoder->m_DecodeData.m_uPtrMinMatchLength == 3)
        {
            pDecoder->m_DecodeData.m_pLz77DecProc = Xpress9Lz77Dec_MtfCount0_Ptr3;
        }
        else
        {
            RETAIL_ASSERT (pDecoder->m_DecodeData.m_uPtrMinMatchLength == 4, "uPtrMinMatchLength=%Iu", pDecoder->m_DecodeData.m_uPtrMinMatchLength);
            pDecoder->m_DecodeData.m_pLz77DecProc = Xpress9Lz77Dec_MtfCount0_Ptr4;
        }
    }
    else if (pDecoder->m_DecodeData.m_uMtfEntryCount == 2)
    {
        if (pDecoder->m_DecodeData.m_uPtrMinMatchLength == 3)
        {
            if (pDecoder->m_DecodeData.m_uMtfMinMatchLength == 2)
            {
                pDecoder->m_DecodeData.m_pLz77DecProc = Xpress9Lz77Dec_MtfCount2_Ptr3_Mtf2;
            }
            else
            {
                RETAIL_ASSERT (pDecoder->m_DecodeData.m_uMtfMinMatchLength == 3, "uMtfMinMatchLength=%Iu", pDecoder->m_DecodeData.m_uMtfMinMatchLength);
                pDecoder->m_DecodeData.m_pLz77DecProc = Xpress9Lz77Dec_MtfCount2_Ptr3_Mtf3;
            }
        }
        else
        {
            RETAIL_ASSERT (pDecoder->m_DecodeData.m_uPtrMinMatchLength == 4, "uPtrMinMatchLength=%Iu", pDecoder->m_DecodeData.m_uPtrMinMatchLength);
            if (pDecoder->m_DecodeData.m_uMtfMinMatchLength == 2)
            {
                pDecoder->m_DecodeData.m_pLz77DecProc = Xpress9Lz77Dec_MtfCount2_Ptr4_Mtf2;
            }
            else
            {
                RETAIL_ASSERT (pDecoder->m_DecodeData.m_uMtfMinMatchLength == 3, "uMtfMinMatchLength=%Iu", pDecoder->m_DecodeData.m_uMtfMinMatchLength);
                pDecoder->m_DecodeData.m_pLz77DecProc = Xpress9Lz77Dec_MtfCount2_Ptr4_Mtf3;
            }
        }
    }
    else
    {
        RETAIL_ASSERT (pDecoder->m_DecodeData.m_uMtfEntryCount == 4, "uMtfEntryCount=%Iu", pDecoder->m_DecodeData.m_uMtfEntryCount);
        if (pDecoder->m_DecodeData.m_uPtrMinMatchLength == 3)
        {
            if (pDecoder->m_DecodeData.m_uMtfMinMatchLength == 2)
            {
                pDecoder->m_DecodeData.m_pLz77DecProc = Xpress9Lz77Dec_MtfCount4_Ptr3_Mtf2;
            }
            else
            {
                RETAIL_ASSERT (pDecoder->m_DecodeData.m_uMtfMinMatchLength == 3, "uMtfMinMatchLength=%Iu", pDecoder->m_DecodeData.m_uMtfMinMatchLength);
                pDecoder->m_DecodeData.m_pLz77DecProc = Xpress9Lz77Dec_MtfCount4_Ptr3_Mtf3;
            }
        }
        else
        {
            RETAIL_ASSERT (pDecoder->m_DecodeData.m_uPtrMinMatchLength == 4, "uPtrMinMatchLength=%Iu", pDecoder->m_DecodeData.m_uPtrMinMatchLength);
            if (pDecoder->m_DecodeData.m_uMtfMinMatchLength == 2)
            {
                pDecoder->m_DecodeData.m_pLz77DecProc = Xpress9Lz77Dec_MtfCount4_Ptr4_Mtf2;
            }
            else
            {
                RETAIL_ASSERT (pDecoder->m_DecodeData.m_uMtfMinMatchLength == 3, "uMtfMinMatchLength=%Iu", pDecoder->m_DecodeData.m_uMtfMinMatchLength);
                pDecoder->m_DecodeData.m_pLz77DecProc = Xpress9Lz77Dec_MtfCount4_Ptr4_Mtf3;
            }
        }
    }

Failure:;
}


//
// Allocates memory, initializes, and returns the pointer to newly created decoder.
//
// It is recommended to create few decoders (1-2 decoders per CPU core) and reuse them:
// creation of new decoder is relatively expensive.
//
// Each instance of encoder will use about 64KB + 9 * 2**uMaxWindowSizeLog2 bytes; e.g.
// encoder with 1MB max window size (uMaxWindowSizeLog2 equal to 20) will use about 9MB + 64KB
//
#pragma warning (push)
#pragma warning (disable: 4311) // 'type cast' : pointer truncation from 'UInt8 *' to 'xint'
XPRESS9_EXPORT
XPRESS9_DECODER
XPRESS9_CALL
Xpress9DecoderCreate (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    void                   *pAllocContext,      // IN:  context for user-defined memory allocator
    XpressAllocFn          *pAllocFn,           // IN:  user-defined memory allocator (it will be called by Xpress9EncodeCreate only)
    unsigned                uMaxWindowSizeLog2, // IN:  log2 of max size of LZ77 window [XPRESS9_WINDOW_SIZE_LOG2_MIN..XPRESS9_WINDOW_SIZE_LOG2_MAX]
    unsigned                uFlags              // IN:  one or more of Xpress9Flag_* flags
)
{
    LZ77_DECODER   *pDecoder;               // decoder
    UInt8          *pAllocatedMemory;       // pointer to the beginning of allocated memory block
    UInt8          *pAlignedPtr;            // a pointer aligned on appropriate byte boundary
    size_t          uBufferSize;            // amount of temp buffer to store decoded data
    size_t          uReservedSize;          // amount of memory to reserve after the buffer to allow small overrun caused by fast copy
    size_t          uAllocationSize;        // total amount of allocated memory

    memset (pStatus, 0, sizeof (*pStatus));
    if (uMaxWindowSizeLog2 < XPRESS9_WINDOW_SIZE_LOG2_MIN || uMaxWindowSizeLog2 > XPRESS9_WINDOW_SIZE_LOG2_MAX)
    {
        SET_ERROR (
            Xpress9Status_BadArgument,
            pStatus,
            "uMaxWindowSizeLog2=%Iu is out of range [%u..%u]",
            uMaxWindowSizeLog2,
            XPRESS9_WINDOW_SIZE_LOG2_MIN,
            XPRESS9_WINDOW_SIZE_LOG2_MAX
        );

        goto Failure;
    }

    uReservedSize = 256;
    uBufferSize = POWER2 (uMaxWindowSizeLog2) + POWER2 (uMaxWindowSizeLog2 - 1);

    uAllocationSize = sizeof (*pDecoder) + uBufferSize + uReservedSize + MEMORY_ALIGNMENT;

    pAllocatedMemory = (UInt8 *) pAllocFn (pAllocContext, (int) uAllocationSize);
    if (pAllocatedMemory == NULL)
    {
        SET_ERROR (Xpress9Status_NotEnoughMemory, pStatus, "");
        goto Failure;
    }

    pAlignedPtr = pAllocatedMemory;
    pAlignedPtr += (- (xint) pAlignedPtr) & (MEMORY_ALIGNMENT - 1);
    pDecoder = (LZ77_DECODER *) pAlignedPtr;
    memset (pDecoder, 0, offsetof (LZ77_DECODER, m_Huffman));

    pDecoder->m_BufferData.m_pBufferData = (UInt8 *) (pDecoder + 1);
    pDecoder->m_BufferData.m_uBufferDataSize = (uxint) uBufferSize;

    pDecoder->m_uMaxWindowSizeLog2 = uMaxWindowSizeLog2;
    pDecoder->m_pAllocatedMemory   = pAllocatedMemory;

    pDecoder->m_uRuntimeFlags = uFlags;
    pDecoder->m_uState = LZ77_DECODER_STATE_READY;
    pDecoder->m_uMagic = XPRESS9_DECODER_MAGIC;

    return ((XPRESS9_DECODER) pDecoder);

Failure:
    return (NULL);
}
#pragma warning (pop)


//
// Release memory occupied by the decoder.
//
// Please notice that the order to destroy the decoder will be always obeyed irrespective of decoder state
//
XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9DecoderDestroy (
    XPRESS9_STATUS     *pStatus,                // OUT: status
    XPRESS9_DECODER     pXpress9Decoder,        // IN:  decoder
    void               *pFreeContext,           // IN:  context for user-defined memory release function
    XpressFreeFn       *pFreeFn                 // IN:  user-defined memory release function
)
{
    LZ77_DECODER       *pDecoder;
    void               *pAllocatedMemory;

    memset (pStatus, 0, sizeof (*pStatus));
    GET_DECODER_STATE (pStatus, Failure, pDecoder, pXpress9Decoder);
    
    pAllocatedMemory = pDecoder->m_pAllocatedMemory;
    memset (pDecoder, 0, offsetof (LZ77_DECODER, m_Huffman));
    pFreeFn (pFreeContext, pAllocatedMemory);

Failure:;
}


//
// Start new encoding session. if fForceReset is not set then decoder must be flushed.
//
XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9DecoderStartSession (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    XPRESS9_DECODER         pXpress9Decoder,    // IN:  decoder
    unsigned                fForceReset         // IN:  if TRUE then allows resetting of previous session even if it wasn't flushed
)
{
    LZ77_DECODER       *pDecoder;

    memset (pStatus, 0, sizeof (*pStatus));
    GET_DECODER_STATE (pStatus, Failure, pDecoder, pXpress9Decoder);

    if (!fForceReset)
    {
        if (pDecoder->m_UserData.m_pUserData != NULL)
        {
            SET_ERROR (Xpress9Status_DecoderNotDetached, pStatus, "");
            goto Failure;
        }

        if (
            pDecoder->m_uState != LZ77_DECODER_STATE_READY &&
            pDecoder->m_uState != LZ77_DECODER_STATE_WAIT_SESSION &&
            pDecoder->m_uState != LZ77_DECODER_STATE_WAIT_BLOCK
        )
        {
#if 0
            RETAIL_ASSERT (
                pDecoder->m_DecodeData.m_uScratchBytesStored == pDecoder->m_DecodeData.m_uScratchBytesProcessed,
                "DecoderIsDrainedButState=%Iu ScratchBytesStored=%Iu ScratchBytesProcessed=%Iu",
                pDecoder->m_uState,
                pDecoder->m_DecodeData.m_uScratchBytesStored,
                pDecoder->m_DecodeData.m_uScratchBytesProcessed
            );
            if (
            RETAIL_ASSERT (
                pDecoder->m_DecodeData.m_uDecodedSizeBits == pDecoder->m_DecodeData.m_uEncodedSizeBits,
                "DecoderIsDrainedButState=%Iu DecodedSizeBits=%Iu EncodedSizeBits=%Iu",
                pDecoder->m_uState,
                pDecoder->m_DecodeData.m_uDecodedSizeBits,
                pDecoder->m_DecodeData.m_uEncodedSizeBits
            );
#endif
            SET_ERROR (Xpress9Status_DecoderNotDrained, pStatus, "");
            goto Failure;
        }
    }

    memset (&pDecoder->m_DecodeData, 0, sizeof (pDecoder->m_DecodeData));
    memset (&pDecoder->m_UserData, 0, sizeof (pDecoder->m_UserData));
    pDecoder->m_uState = LZ77_DECODER_STATE_READY;

Failure:;
}

static
void
Xpress9DecodeState (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    LZ77_DECODER           *pDecoder
)
{
    UInt8   uShortSymbolLength[LZ77_SHORT_SYMBOL_ALPHABET_SIZE] = {0};
    UInt8   uLongLengthLength[LZ77_LONG_LENGTH_ALPHABET_SIZE] = {0};
    uxint   uMsb = 0;
    xint    iOffset = 0;
    uxint   i = 0;
    BIO_DECLARE();

    RETAIL_ASSERT (pStatus->m_uStatus == Xpress9Status_OK, "BadStatus");

    if (pDecoder->m_DecodeData.m_uMtfEntryCount > 0)
    {
        BIO_STATE_ENTER (pDecoder->m_DecodeData.m_BioState);
        iOffset = BIORD_LOOKUP_FIXED (1);
        BIORD_SKIP (1);
        pDecoder->m_DecodeData.m_Mtf.m_iMtfLastPtr = iOffset;
        for (i = 0; i < pDecoder->m_DecodeData.m_uMtfEntryCount; ++i)
        {
            uMsb = BIORD_LOOKUP_FIXED (5);
            BIORD_SKIP (5);
            if (uMsb >= pDecoder->m_DecodeData.m_uWindowSizeLog2)
            {
                SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "uMsb=%Iu MaxWindowSize=%Iu", uMsb, pDecoder->m_DecodeData.m_uWindowSizeLog2);
                goto Failure;
            }

            BIORD_LOAD (iOffset, uMsb);
            iOffset += POWER2 (uMsb);
            pDecoder->m_DecodeData.m_Mtf.m_iMtfOffset[i] = iOffset;
        }
        BIO_STATE_LEAVE (pDecoder->m_DecodeData.m_BioState);
    }

    // now decode Huffman tables
    HuffmanDecodeLengthTable (
        pStatus,
        &pDecoder->m_DecodeData.m_BioState,
        uShortSymbolLength,
        LZ77_SHORT_SYMBOL_ALPHABET_SIZE,
        LZ77_MAX_SHORT_LENGTH,
        HUFFMAN_MAX_CODEWORD_LENGTH
    );
    if (pStatus->m_uStatus != Xpress9Status_OK)
    {
        goto Failure;
    }
    pDecoder->m_DecodeData.m_piShortSymbolRoot = HuffmanCreateDecodeTables (
        pStatus,
        uShortSymbolLength,
        LZ77_SHORT_SYMBOL_ALPHABET_SIZE,
        pDecoder->m_Huffman.m_iShortSymbolDecodeTable,
        sizeof (pDecoder->m_Huffman.m_iShortSymbolDecodeTable) / sizeof (pDecoder->m_Huffman.m_iShortSymbolDecodeTable[0]),
        LZ77_SHORT_SYMBOL_DECODE_ROOT_BITS,
        LZ77_SHORT_SYMBOL_DECODE_TAIL_BITS,
        pDecoder->m_Scratch.m_HuffmanNodeTemp
    );
    if (pStatus->m_uStatus != Xpress9Status_OK)
    {
        goto Failure;
    }

    HuffmanDecodeLengthTable (
        pStatus,
        &pDecoder->m_DecodeData.m_BioState,
        uLongLengthLength, // for index i, contains codeword length of i-th symbol.
        LZ77_LONG_LENGTH_ALPHABET_SIZE,
        LZ77_MAX_SHORT_LENGTH,
        HUFFMAN_MAX_CODEWORD_LENGTH
    );
    if (pStatus->m_uStatus != Xpress9Status_OK)
    {
        goto Failure;
    }
    pDecoder->m_DecodeData.m_piLongLengthRoot = HuffmanCreateDecodeTables (
        pStatus,
        uLongLengthLength,
        LZ77_LONG_LENGTH_ALPHABET_SIZE,
        pDecoder->m_Huffman.m_iLongLengthDecodeTable,
        sizeof (pDecoder->m_Huffman.m_iLongLengthDecodeTable) / sizeof (pDecoder->m_Huffman.m_iLongLengthDecodeTable[0]),
        LZ77_LONG_LENGTH_DECODE_ROOT_BITS,
        LZ77_LONG_LENGTH_DECODE_TAIL_BITS,
        pDecoder->m_Scratch.m_HuffmanNodeTemp
    );
    if (pStatus->m_uStatus != Xpress9Status_OK)
    {
        goto Failure;
    }


Failure:;
}


// Attach next buffer with compressed data to the decoder
//
XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9DecoderAttach (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    XPRESS9_DECODER         pXpress9Decoder,    // IN:  decoder
    const void             *pCompData,          // IN:  buffer containing compressed data
    unsigned                uCompDataSize       // IN:  amount of data in compressed buffer
)
{
    LZ77_DECODER       *pDecoder;

    memset (pStatus, 0, sizeof (*pStatus));
    GET_DECODER_STATE (pStatus, Failure, pDecoder, pXpress9Decoder);

    if (pDecoder->m_UserData.m_uUserDataSize != pDecoder->m_UserData.m_uUserDataRead)
    {
        SET_ERROR (Xpress9Status_DecoderNotDrained, pStatus, "UserDataSize=%Iu UserDataRead=%Iu", pDecoder->m_UserData.m_uUserDataSize, pDecoder->m_UserData.m_uUserDataRead);
        goto Failure;
    }

    pDecoder->m_UserData.m_pUserData     = pCompData;
    pDecoder->m_UserData.m_uUserDataSize = uCompDataSize;
    pDecoder->m_UserData.m_uUserDataRead = 0;

Failure:;
}



//
// Detaches the buffer with compressed data
//
XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9DecoderDetach (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    XPRESS9_DECODER         pXpress9Decoder,    // IN:  decoder
    const void             *pCompData,          // IN:  buffer containing compressed data
    unsigned                uCompDataSize       // IN:  amount of data in compressed buffer
)
{
    LZ77_DECODER       *pDecoder;

    memset (pStatus, 0, sizeof (*pStatus));
    GET_DECODER_STATE (pStatus, Failure, pDecoder, pXpress9Decoder);

    if (pDecoder->m_UserData.m_pUserData != pCompData || pDecoder->m_UserData.m_uUserDataSize != uCompDataSize)
    {
        SET_ERROR (
            Xpress9Status_BadArgument,
            pStatus,
            "CompData=%p AttachedCompData=%p CompDataSize=%u AttachedCompDataSize=%Iu",
            pCompData,
            pDecoder->m_UserData.m_pUserData,
            uCompDataSize,
            pDecoder->m_UserData.m_uUserDataSize
        );
        goto Failure;
    }

    if (pDecoder->m_UserData.m_uUserDataSize != pDecoder->m_UserData.m_uUserDataRead)
    {
        SET_ERROR (Xpress9Status_DecoderNotDrained, pStatus, "UserDataSize=%Iu UserDataRead=%Iu", pDecoder->m_UserData.m_uUserDataSize, pDecoder->m_UserData.m_uUserDataRead);
        goto Failure;
    }

    memset (&pDecoder->m_UserData, 0, sizeof (pDecoder->m_UserData));

Failure:;
}


//
// Fetch decompressed data filling pOrigData buffer with up to uOrigDataSize bytes.
//
// Returns number of decopressed bytes that are available but did not fit into pOrigData buffer
//
XPRESS9_EXPORT
unsigned
XPRESS9_CALL
Xpress9DecoderFetchDecompressedData (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    XPRESS9_DECODER         pXpress9Decoder,    // IN:  decoder
    void                   *pOrigData,          // IN:  buffer for storing uncompressed data (may be NULL)
    unsigned                uOrigDataSize,      // IN:  size of pOrigDataBuffer (may be 0)
    unsigned               *puOrigDataWritten,  // OUT: number of bytes that were actually written into uOrigDataSize buffer
    unsigned               *puCompDataNeeded    // OUT: how much compressed bytes needed
)
{
    LZ77_DECODER       *pDecoder;
    UInt8              *pDst;
    uxint               uBytesToCopy;
    uxint               uBytesNeeded;
    uxint               uBytesCopied;
    uxint               uOrigDataWritten;
    uxint               uCompDataNeeded;
    uxint               uOrigDataAvailable;
    uxint               uFlags;
    BOOL                fEof;
    BIO_DECLARE();

    uCompDataNeeded     = 0;
    uOrigDataWritten    = 0;
    uOrigDataAvailable  = 0;

    memset (pStatus, 0, sizeof (*pStatus));
    GET_DECODER_STATE (pStatus, Failure, pDecoder, pXpress9Decoder);

#if 0
    if (pDecoder->m_UserData.m_uUserDataSize == pDecoder->m_UserData.m_uUserDataRead)
    {
        uCompDataNeeded = 1;
        goto Done;
    }
#endif

    //
    // retrieve session and/or next block header first
    // by copying header into scratch area and getting it from there
    //

    pDst = pDecoder->m_Scratch.m_uScratchArea;

    switch (pDecoder->m_uState)
    {
    case LZ77_DECODER_STATE_READY:
        pDecoder->m_uState = LZ77_DECODER_STATE_WAIT_SESSION;
        // fall through

    case LZ77_DECODER_STATE_WAIT_SESSION:
    case LZ77_DECODER_STATE_WAIT_BLOCK:
        //
        // retrieve the header first
        //
        uBytesNeeded = 8*4;
        RETAIL_ASSERT (uBytesNeeded > pDecoder->m_DecodeData.m_uScratchBytesStored, "uBytesNeeded=%Iu ScratchBytesStored=%Iu", uBytesNeeded, pDecoder->m_DecodeData.m_uScratchBytesStored);
        uBytesToCopy = uBytesNeeded - pDecoder->m_DecodeData.m_uScratchBytesStored;
        if (uBytesToCopy > pDecoder->m_UserData.m_uUserDataSize - pDecoder->m_UserData.m_uUserDataRead)
        {
            //
            // cannot decoder the header yet; just copy the data and return
            //
            uBytesToCopy = pDecoder->m_UserData.m_uUserDataSize - pDecoder->m_UserData.m_uUserDataRead;
            memcpy (pDst + pDecoder->m_DecodeData.m_uScratchBytesStored, pDecoder->m_UserData.m_pUserData + pDecoder->m_UserData.m_uUserDataRead, uBytesToCopy);
            pDecoder->m_UserData.m_uUserDataRead += uBytesToCopy;
            pDecoder->m_DecodeData.m_uScratchBytesStored += uBytesToCopy;
            
            uCompDataNeeded     = uBytesNeeded - pDecoder->m_DecodeData.m_uScratchBytesStored;
            goto Done;
        }

        memcpy (pDst + pDecoder->m_DecodeData.m_uScratchBytesStored, pDecoder->m_UserData.m_pUserData + pDecoder->m_UserData.m_uUserDataRead, uBytesToCopy);
        pDecoder->m_UserData.m_uUserDataRead += uBytesToCopy;
        pDecoder->m_DecodeData.m_uScratchBytesStored += uBytesToCopy;

        //
        // we got the header; decode it
        //
        if (((UInt32 *) pDst)[0] != XPRESS9_MAGIC)
        {
            SET_ERROR (Xpress9Status_DecoderCorruptedHeader, pStatus, "HeaderMagic=%u ExpectedMagic=%u", ((UInt32 *) pDst)[0], (UInt32) XPRESS9_MAGIC);
            goto Failure;
        }

        if (pDecoder->m_uState == LZ77_DECODER_STATE_WAIT_SESSION)
        {
            pDecoder->m_Stat.m_uBlockIndex = 0;
            pDecoder->m_Stat.m_uSessionSignature = ((UInt32 *) pDst)[5];
        }

        if (((UInt32 *) pDst)[6] != (UInt32) pDecoder->m_Stat.m_uBlockIndex)
        {
            SET_ERROR (Xpress9Status_DecoderCorruptedHeader, pStatus, "BlockIndex=%u ExpectedBlockIndex=%u", ((UInt32 *) pDst)[6], (UInt32) pDecoder->m_Stat.m_uBlockIndex);
            goto Failure;
        }

        if (((UInt32 *) pDst)[5] != (UInt32) pDecoder->m_Stat.m_uSessionSignature)
        {
            SET_ERROR (Xpress9Status_DecoderCorruptedHeader, pStatus, "SessionSignature=%u ExpectedSessionSignature=%u", ((UInt32 *) pDst)[5], (UInt32) pDecoder->m_Stat.m_uSessionSignature);
            goto Failure;
        }

        if (((UInt32 *) pDst)[4] != 0)
        {
            SET_ERROR (Xpress9Status_DecoderCorruptedHeader, pStatus, "ReservedValue=%u ExpectedValue=0", ((UInt32 *) pDst)[4]);
            goto Failure;
        }


        uFlags = Xpress9Crc32 (pDst, 7*4, ((UInt32 *) pDst)[4]);
        if (((UInt32 *) pDst)[7] != (UInt32) uFlags)
        {
            SET_ERROR (Xpress9Status_DecoderCorruptedHeader, pStatus, "HeaderCrc32=%u ExpectedHeaderCrc32=%Iu", (UInt32) uFlags, ((UInt32 *) pDst)[7]);
            goto Failure;
        }


        uFlags = ((UInt32 *) pDst)[3];

//     13 bits      length of encoded huffman tables in bits
//      3 bits      log2 of window size (16 .. 23)
//      2 bits      number of MTF entries   (0, 2, 4, reserved)
//      1 bit       min ptr match length (3 or 4)
//      1 bit       min mtf match length (2 or 3, shall be 0 if number of MTF entries is set to 0)
//
//     12 bits      reserved, must be 0

        pDecoder->m_DecodeData.m_uHuffmanTableBits = uFlags & 0x1fff;
        if (pDecoder->m_uState == LZ77_DECODER_STATE_WAIT_SESSION)
        {
            pDecoder->m_DecodeData.m_uMtfEntryCount = ((uFlags >> 16) & 3) << 1;
            if (pDecoder->m_DecodeData.m_uMtfEntryCount > 4)
            {
                SET_ERROR (Xpress9Status_DecoderCorruptedHeader, pStatus, "MtfEntryCount=%Iu", pDecoder->m_DecodeData.m_uMtfEntryCount);
                goto Failure;
            }
            pDecoder->m_DecodeData.m_uWindowSizeLog2 = ((uFlags >> 13) & 7) + 16; // window size - 16 is what is written by encoder.
            pDecoder->m_DecodeData.m_uPtrMinMatchLength = ((uFlags >> 18) & 1) + 3;
            pDecoder->m_DecodeData.m_uMtfMinMatchLength = ((uFlags >> 19) & 1) + 2;
            Xpress9DecoderGetProc (pStatus, pDecoder);
            if (pStatus->m_uStatus != Xpress9Status_OK)
            {
                goto Failure;
            }
        }
        else
        {
            if (pDecoder->m_DecodeData.m_uWindowSizeLog2 != (((uFlags >> 13) & 7) + 16))
            {
                SET_ERROR (Xpress9Status_DecoderCorruptedHeader, pStatus, "BlockWindowsSizeLog2=%Iu Expected=%Iu", (((uFlags >> 13) & 7) + 16), pDecoder->m_DecodeData.m_uWindowSizeLog2);
                goto Failure;
            }
            if (pDecoder->m_DecodeData.m_uMtfEntryCount != (((uFlags >> 16) & 3) << 1))
            {
                SET_ERROR (Xpress9Status_DecoderCorruptedHeader, pStatus, "MtfEntryCount=%Iu Expected=%Iu", (((uFlags >> 16) & 3) << 1), pDecoder->m_DecodeData.m_uMtfEntryCount);
                goto Failure;
            }
            if (pDecoder->m_DecodeData.m_uPtrMinMatchLength != (((uFlags >> 18) & 1) + 3))
            {
                SET_ERROR (Xpress9Status_DecoderCorruptedHeader, pStatus, "PtrMinMatchLength=%Iu Expected=%Iu", (((uFlags >> 18) & 1) + 3), pDecoder->m_DecodeData.m_uPtrMinMatchLength);
                goto Failure;
            }
            if (pDecoder->m_DecodeData.m_uMtfMinMatchLength != (((uFlags >> 19) & 1) + 2))
            {
                SET_ERROR (Xpress9Status_DecoderCorruptedHeader, pStatus, "MtfMinMatchLength=%Iu Expected=%Iu", (((uFlags >> 19) & 1) + 2), pDecoder->m_DecodeData.m_uMtfMinMatchLength);
                goto Failure;
            }
        }

        if ((uFlags >> 20) != 0)
        {
            SET_ERROR (Xpress9Status_DecoderCorruptedHeader, pStatus, "FlagsReservedIsNonZero=0x%Ix", uFlags);
            goto Failure;
        }

        pDecoder->m_DecodeData.m_uEncodedSizeBits = ((UInt32 *) pDst)[2];
        pDecoder->m_Stat.m_Block.m_uOrigSize = ((UInt32 *) pDst)[1];

        pDecoder->m_DecodeData.m_uBlockDecodedBytes = 0;
        pDecoder->m_DecodeData.m_uBlockCopiedBytes  = 0;
        pDecoder->m_DecodeData.m_uDecodedSizeBits   = 0;


        if (pDecoder->m_DecodeData.m_uEncodedSizeBits <= pDecoder->m_DecodeData.m_uHuffmanTableBits + 8*4*8)
        {
            SET_ERROR (Xpress9Status_DecoderCorruptedHeader, pStatus, "EncodedSizeBits=%Iu HuffmanTableBits=%Iu", pDecoder->m_DecodeData.m_uEncodedSizeBits, pDecoder->m_DecodeData.m_uHuffmanTableBits);
            goto Failure;
        }

        //
        // fast path for retrieving the uncompressed size 
        //
        if (pDecoder->m_uState == LZ77_DECODER_STATE_WAIT_SESSION && uOrigDataSize == 0)
        {
            pDecoder->m_uState = LZ77_DECODER_STATE_WAIT_HUFFMAN;
            uOrigDataAvailable = (unsigned) (pDecoder->m_Stat.m_Block.m_uOrigSize - pDecoder->m_DecodeData.m_uBlockCopiedBytes);
            goto Done;
        }

        pDecoder->m_uState = LZ77_DECODER_STATE_WAIT_HUFFMAN;
	}

    if (pDecoder->m_uState == LZ77_DECODER_STATE_WAIT_HUFFMAN)
    {
        uBytesNeeded = 8*4 + ((pDecoder->m_DecodeData.m_uHuffmanTableBits + 7) >> 3); //( + 7 >> 3, rounds off to the next byte)

        //
        // copy few bytes more so that we may have smooth transition from scratch buffer to user buffer and back
        // without the need to reload the shift register
        //
        uBytesNeeded += 64;
        uBytesToCopy = (pDecoder->m_DecodeData.m_uEncodedSizeBits + 7) >> 3;
        if (uBytesNeeded > uBytesToCopy)
            uBytesNeeded = uBytesToCopy;

        if (uBytesNeeded > pDecoder->m_DecodeData.m_uScratchBytesStored)
        {
            //
            // need to copy more data
            //
            uBytesToCopy = uBytesNeeded - pDecoder->m_DecodeData.m_uScratchBytesStored;
            
            // Check m_uUserDataRead overrun ( see defect 264966 )
            if( uBytesToCopy + pDecoder->m_UserData.m_uUserDataRead > pDecoder->m_UserData.m_uUserDataSize)
            {
                uBytesToCopy = pDecoder->m_UserData.m_uUserDataSize - pDecoder->m_UserData.m_uUserDataRead;
            
                //
                // do not have enough; just copy what we have
                //
                memcpy (    pDst + pDecoder->m_DecodeData.m_uScratchBytesStored, 
                            pDecoder->m_UserData.m_pUserData + pDecoder->m_UserData.m_uUserDataRead, 
                            uBytesToCopy
                            );

                pDecoder->m_UserData.m_uUserDataRead            += uBytesToCopy;
                pDecoder->m_DecodeData.m_uScratchBytesStored    += uBytesToCopy;
                
                uCompDataNeeded = uBytesNeeded - pDecoder->m_DecodeData.m_uScratchBytesStored;

                goto Done;
            }

            memcpy (pDst + pDecoder->m_DecodeData.m_uScratchBytesStored, pDecoder->m_UserData.m_pUserData + pDecoder->m_UserData.m_uUserDataRead, uBytesToCopy);
            pDecoder->m_UserData.m_uUserDataRead += uBytesToCopy;
            pDecoder->m_DecodeData.m_uScratchBytesStored += uBytesToCopy;

            RETAIL_ASSERT( pDecoder->m_UserData.m_uUserDataRead <= pDecoder->m_UserData.m_uUserDataSize, 
                "Buffer overrun! m_uUserDataRead = %d, m_uUserDataSize = %d", 
                pDecoder->m_UserData.m_uUserDataRead,
                pDecoder->m_UserData.m_uUserDataSize);

        }

        BIORD_SET_INPUT_PTR (pDst + 8*4);
        BIORD_PRELOAD_SHIFT_REGISTER ();
        BIO_STATE_LEAVE (pDecoder->m_DecodeData.m_BioState);

        DEBUG_PERF_START (pDecoder->m_DebugPerf.m_uHuffman);
        Xpress9DecodeState (pStatus, pDecoder);
        DEBUG_PERF_STOP (pDecoder->m_DebugPerf.m_uHuffman);

        if (pStatus->m_uStatus != Xpress9Status_OK)
        {
            goto Failure;
        }

        // In the following piece of the code, uBytesToCopy is used to keep count of number of bits ( and not bytes )
        uBytesToCopy = BIORD_TELL_CONSUMED_BITS_STATE (pDecoder->m_DecodeData.m_BioState, pDst + 8*4);
        if (uBytesToCopy != pDecoder->m_DecodeData.m_uHuffmanTableBits)
        {
            SET_ERROR (Xpress9Status_DecoderCorruptedData, pStatus, "HuffmanTableBits=%Iu DecodedHuffmanTableBits=%Iu", pDecoder->m_DecodeData.m_uHuffmanTableBits, uBytesToCopy);
            goto Failure;
        }

        pDecoder->m_DecodeData.m_uDecodedSizeBits = uBytesToCopy + 8*4*8;

        pDecoder->m_uState = LZ77_DECODER_STATE_DECODING;
    }

    fEof = FALSE;
    for (;;)
    {
        // first, copy already decompressed data into user buffer
        uBytesToCopy = pDecoder->m_DecodeData.m_uDecodePosition - pDecoder->m_DecodeData.m_uCopyPosition;
        if (uBytesToCopy > uOrigDataSize)
            uBytesToCopy = uOrigDataSize;

        if (pOrigData != NULL)
        {
            DEBUG_PERF_START (pDecoder->m_DebugPerf.m_uMemcpyOrig);
            memcpy (pOrigData, pDecoder->m_BufferData.m_pBufferData + pDecoder->m_DecodeData.m_uCopyPosition, uBytesToCopy);
            DEBUG_PERF_STOP (pDecoder->m_DebugPerf.m_uMemcpyOrig);
        }

        uOrigDataSize = (unsigned) (uOrigDataSize - uBytesToCopy);
        if (pOrigData != NULL)
        {
            pOrigData = (void *) (((UInt8 *) pOrigData) + uBytesToCopy);
        }
        pDecoder->m_DecodeData.m_uCopyPosition += uBytesToCopy;
        pDecoder->m_DecodeData.m_uBlockCopiedBytes += uBytesToCopy;
        uOrigDataWritten += uBytesToCopy;

        //
        // see whether we have anything left
        //
        if (
            pDecoder->m_DecodeData.m_uBlockCopiedBytes >= pDecoder->m_Stat.m_Block.m_uOrigSize ||
            pDecoder->m_DecodeData.m_uBlockDecodedBytes + pDecoder->m_DecodeData.m_Tail.m_uLength >= pDecoder->m_Stat.m_Block.m_uOrigSize ||
            pDecoder->m_DecodeData.m_uDecodedSizeBits >= pDecoder->m_DecodeData.m_uEncodedSizeBits
        )
        {
            // if we decoded everything, make sure we copied everything
            if (
                pDecoder->m_DecodeData.m_uBlockDecodedBytes + pDecoder->m_DecodeData.m_Tail.m_uLength != pDecoder->m_Stat.m_Block.m_uOrigSize ||
                pDecoder->m_DecodeData.m_uDecodedSizeBits != pDecoder->m_DecodeData.m_uEncodedSizeBits
            )
            {
                SET_ERROR (
                    Xpress9Status_DecoderCorruptedData,
                    pStatus,
                    "DecodedBytes=%Iu EncodedBytes=%I64u TailLength=%Iu DecodedBits=%Iu EncodedBits=%Iu CopiedBytes=%Iu BlockBytes=%I64u",
                    pDecoder->m_DecodeData.m_uBlockDecodedBytes,
                    pDecoder->m_Stat.m_Block.m_uOrigSize,
                    pDecoder->m_DecodeData.m_Tail.m_uLength,
                    pDecoder->m_DecodeData.m_uDecodedSizeBits,
                    pDecoder->m_DecodeData.m_uEncodedSizeBits,
                    pDecoder->m_DecodeData.m_uBlockCopiedBytes,
                    pDecoder->m_Stat.m_Block.m_uOrigSize
                );
                goto Failure;
            }

#if XPRESS9_DEBUG_PERF_STAT
            {
                UInt64 uTotal;
                uTotal =
                    pDecoder->m_DebugPerf.m_uHuffman +
                    pDecoder->m_DebugPerf.m_uShift +
                    pDecoder->m_DebugPerf.m_uDecoder +
                    pDecoder->m_DebugPerf.m_uMemcpyOrig +
                    pDecoder->m_DebugPerf.m_uMemcpyComp
                ;
                if (uTotal == 0)
                    uTotal = 1;
#define P(f) printf ("%-15s =%12I64u %6.2f%%\n", "Dec" #f, pDecoder->m_DebugPerf.m_u##f, (100.0 * (double) pDecoder->m_DebugPerf.m_u##f) / uTotal);
                P (Huffman);
                P (Decoder);
                P (MemcpyOrig);
                P (MemcpyComp);
                P (Shift);
                printf ("%-15s =%12I64u %6.2f%%\n\n", "DecTotal", uTotal, 100.0);
#undef P
            }
#endif /* XPRESS9_DEBUG_PERF_STAT */

            if (pDecoder->m_DecodeData.m_uBlockCopiedBytes == pDecoder->m_Stat.m_Block.m_uOrigSize)
            {
                //
                // decoded and copied everything without a glitch
                //
                pDecoder->m_uState = LZ77_DECODER_STATE_WAIT_BLOCK;

                pDecoder->m_Stat.m_uBlockIndex += 1;
                pDecoder->m_DecodeData.m_uScratchBytesStored = 0;

                uOrigDataAvailable = 0;
                goto Done;
            }
            else
            {
                //
                // we decoded everything but user didn't supply enough buffers to fetch all the data;
                // a little bit is still left it internal buffer
                //
                RETAIL_ASSERT (
                    pDecoder->m_DecodeData.m_uBlockCopiedBytes < pDecoder->m_Stat.m_Block.m_uOrigSize,
                    "uOrigDataSize=%Iu OrigSize=%I64u BlockCopiedBytes=%Iu",
                    uOrigDataSize,
                    pDecoder->m_Stat.m_Block.m_uOrigSize,
                    pDecoder->m_DecodeData.m_uBlockCopiedBytes
                );
            }
        }

        if (uOrigDataSize == 0)
        {
            uOrigDataAvailable = (unsigned) (pDecoder->m_Stat.m_Block.m_uOrigSize - pDecoder->m_DecodeData.m_uBlockCopiedBytes);
            goto Done;
        }

        RETAIL_ASSERT (
            pDecoder->m_DecodeData.m_uDecodePosition == pDecoder->m_DecodeData.m_uCopyPosition,
            "DecodePosition=%Iu CopyPosition=%Iu",
            pDecoder->m_DecodeData.m_uDecodePosition,
            pDecoder->m_DecodeData.m_uCopyPosition
        );

        //
        // see how many bytes we need to go through
        //
        uBytesNeeded = pDecoder->m_DecodeData.m_uDecodedSizeBits;
        uBytesNeeded += BIORD_TELL_SHIFT_REGISTER_BITS_STATE (pDecoder->m_DecodeData.m_BioState);
        RETAIL_ASSERT ((uBytesNeeded & 7) == 0, "NumberOfBitsFetched=%Iu", uBytesNeeded);
        if (fEof || uBytesNeeded > pDecoder->m_DecodeData.m_uEncodedSizeBits)
        {
            fEof = TRUE;
            uBytesNeeded = 0;
            uBytesToCopy = 1;

            //
            // when decoder comes close to end of buffer we allow small buffer overrun because in
            // shift register is populated by
            //
            RETAIL_ASSERT (
                BIORD_TELL_OUTPUT_PTR_STATE (pDecoder->m_DecodeData.m_BioState) <= pDst + pDecoder->m_DecodeData.m_uScratchBytesStored + sizeof (BIO_FULL) - 1,
                "BioStatePtr=%p ScratchStart=%p ScratchEnd=%p",
                BIORD_TELL_OUTPUT_PTR_STATE (pDecoder->m_DecodeData.m_BioState),
                pDst,
                pDst + pDecoder->m_DecodeData.m_uScratchBytesStored
            );
        }
        else
        {
            //
            // shift data in scratch buffer if necesary
            //
            uBytesCopied = BIORD_TELL_OUTPUT_PTR_STATE (pDecoder->m_DecodeData.m_BioState) - pDst;
            if (uBytesCopied > LZ77_DECODER_SCRATCH_AREA_SIZE * 7 / 8)
            {
                //
                // not too much left in scratch buffer; shift it
                //
                uBytesToCopy = pDecoder->m_DecodeData.m_uScratchBytesStored - uBytesCopied;

                DEBUG_PERF_START (pDecoder->m_DebugPerf.m_uMemcpyComp);
                memcpy (pDst, pDst + uBytesCopied, uBytesToCopy);
                DEBUG_PERF_STOP (pDecoder->m_DebugPerf.m_uMemcpyComp);

                pDecoder->m_DecodeData.m_uScratchBytesStored = uBytesToCopy;
                BIORD_SET_INPUT_PTR_STATE (pDecoder->m_DecodeData.m_BioState, pDst);
            }


            uBytesNeeded = (pDecoder->m_DecodeData.m_uEncodedSizeBits + 7 - uBytesNeeded) >> 3;

            //
            // see how much already copied
            //
            RETAIL_ASSERT (
                pDst + pDecoder->m_DecodeData.m_uScratchBytesStored >= BIORD_TELL_OUTPUT_PTR_STATE (pDecoder->m_DecodeData.m_BioState),
                "BioStatePtr=%p ScratchStart=%p ScratchEnd=%p",
                BIORD_TELL_OUTPUT_PTR_STATE (pDecoder->m_DecodeData.m_BioState),
                pDst,
                pDst + pDecoder->m_DecodeData.m_uScratchBytesStored
            );
            uBytesCopied = pDst + pDecoder->m_DecodeData.m_uScratchBytesStored - BIORD_TELL_OUTPUT_PTR_STATE (pDecoder->m_DecodeData.m_BioState);
            uBytesNeeded -= uBytesCopied;

            uBytesToCopy = pDecoder->m_UserData.m_uUserDataSize - pDecoder->m_UserData.m_uUserDataRead;
            if (uBytesNeeded > uBytesToCopy)
            {
                if (uBytesToCopy == 0 && uBytesNeeded != 0)
                {
                    uCompDataNeeded = uBytesNeeded;
                    uOrigDataAvailable = (unsigned) (pDecoder->m_Stat.m_Block.m_uOrigSize - pDecoder->m_DecodeData.m_uBlockCopiedBytes);
                    goto Done;
                }
            }
            else
            {
                uBytesToCopy = uBytesNeeded;
            }

            if (uBytesToCopy > sizeof (pDecoder->m_Scratch.m_uScratchArea) - pDecoder->m_DecodeData.m_uScratchBytesStored)
                uBytesToCopy = sizeof (pDecoder->m_Scratch.m_uScratchArea) - pDecoder->m_DecodeData.m_uScratchBytesStored;

            DEBUG_PERF_START (pDecoder->m_DebugPerf.m_uMemcpyComp);
            memcpy (pDst + pDecoder->m_DecodeData.m_uScratchBytesStored, pDecoder->m_UserData.m_pUserData + pDecoder->m_UserData.m_uUserDataRead, uBytesToCopy);
            DEBUG_PERF_STOP (pDecoder->m_DebugPerf.m_uMemcpyComp);

            pDecoder->m_UserData.m_uUserDataRead += uBytesToCopy;
            pDecoder->m_DecodeData.m_uScratchBytesStored += uBytesToCopy;
            uBytesNeeded -= uBytesToCopy;
            uBytesCopied += uBytesToCopy;

            //
            // Decoding 1 byte will read at most 27 bits, so we can read up to
            // Note the value 27 corresponds to MAX_CODEWORD_LENGTH -- we are generating length constrained Huffman code 
            // where the upper bound length is 27.
            //
            uBytesToCopy = uBytesCopied * 8 / 27;
            if (uBytesToCopy == 0)
                uBytesToCopy = 1;
        }


        //
        // clamp number of bytes to decompress by remaining uncompressed bytes
        //
        uBytesNeeded = ((uxint) pDecoder->m_Stat.m_Block.m_uOrigSize) - pDecoder->m_DecodeData.m_uBlockDecodedBytes;
        if (uBytesToCopy > uBytesNeeded)
            uBytesToCopy = uBytesNeeded;

        //
        // clamp number of bytes to decompress by number of bytes left in the the buffer
        //
        uBytesNeeded = pDecoder->m_BufferData.m_uBufferDataSize - pDecoder->m_DecodeData.m_uDecodePosition;
        if (uBytesNeeded < 256 && uBytesNeeded < uBytesToCopy)
        {
            if (pOrigData == NULL)
            {
                SET_ERROR (
                    Xpress9Status_DecoderCorruptedData,
                    pStatus,
                    "Buffer too small to DirectDecode, uBytesNeeded=%Iu, uBytesToCopy=%Iu, uDecodePosition=%Iu, uBufferDataSize=%Iu",
                    uBytesNeeded,
                    uBytesToCopy,
                    pDecoder->m_DecodeData.m_uDecodePosition
                    pDecoder->m_BufferData.m_uBufferDataSize,
                );
                goto Failure;
            }

            //
            // if we have less not much space left in the buffer and we'll need to decompress more than
            // amount of free space left in the buffer we need to shift the contents of the buffer
            //
            uBytesNeeded = pDecoder->m_DecodeData.m_uDecodePosition - POWER2 (pDecoder->m_DecodeData.m_uWindowSizeLog2);

            // shift out the left most uBytesNeeded bytes keeping windowsize bytes.
            DEBUG_PERF_START (pDecoder->m_DebugPerf.m_uShift);
            memcpy (
                pDecoder->m_BufferData.m_pBufferData,
                pDecoder->m_BufferData.m_pBufferData + uBytesNeeded,
                POWER2 (pDecoder->m_DecodeData.m_uWindowSizeLog2)
            );
            DEBUG_PERF_STOP (pDecoder->m_DebugPerf.m_uShift);

            pDecoder->m_DecodeData.m_uBufferOffset  += uBytesNeeded;
            pDecoder->m_DecodeData.m_uDecodePosition = POWER2 (pDecoder->m_DecodeData.m_uWindowSizeLog2);
            pDecoder->m_DecodeData.m_uCopyPosition   = POWER2 (pDecoder->m_DecodeData.m_uWindowSizeLog2);
            uBytesNeeded = pDecoder->m_BufferData.m_uBufferDataSize - pDecoder->m_DecodeData.m_uDecodePosition;
        }
        if (uBytesToCopy > uBytesNeeded)
            uBytesToCopy = uBytesNeeded;

        pDecoder->m_DecodeData.m_uStopPosition = pDecoder->m_DecodeData.m_uDecodePosition + uBytesToCopy;
        pDecoder->m_DecodeData.m_uEndOfBuffer = pDecoder->m_BufferData.m_uBufferDataSize;

        pDecoder->m_DecodeData.m_uBlockDecodedBytes -= pDecoder->m_DecodeData.m_uDecodePosition;        
        pDecoder->m_DecodeData.m_uDecodedSizeBits -= BIORD_TELL_CONSUMED_BITS_STATE (pDecoder->m_DecodeData.m_BioState, pDst);

        DEBUG_PERF_START (pDecoder->m_DebugPerf.m_uDecoder);
        (*pDecoder->m_DecodeData.m_pLz77DecProc) (pStatus, pDecoder);
        DEBUG_PERF_STOP (pDecoder->m_DebugPerf.m_uDecoder);

        pDecoder->m_DecodeData.m_uDecodedSizeBits += BIORD_TELL_CONSUMED_BITS_STATE (pDecoder->m_DecodeData.m_BioState, pDst);
        pDecoder->m_DecodeData.m_uBlockDecodedBytes += pDecoder->m_DecodeData.m_uDecodePosition;
    }



Done:
    *puCompDataNeeded = (unsigned) uCompDataNeeded;
    *puOrigDataWritten = (unsigned) uOrigDataWritten;
    return ((unsigned) (uOrigDataAvailable));

Failure:
    *puCompDataNeeded = (unsigned) uCompDataNeeded;
    *puOrigDataWritten = (unsigned) uOrigDataWritten;
    return (0);
}


XPRESS9_EXPORT
unsigned
XPRESS9_CALL
Xpress9DecoderQueryBuffer(
    XPRESS9_STATUS     *pStatus,                // OUT: status
    XPRESS9_DECODER     pXpress9Decoder,        // IN:  decoder
    void              **pBuffer,                // OUT
    size_t             *pBufferSize             // OUT
)
{
    LZ77_DECODER       *pDecoder;

    memset (pStatus, 0, sizeof (*pStatus));
    GET_DECODER_STATE (pStatus, Failure, pDecoder, pXpress9Decoder);

    *pBuffer = pDecoder->m_BufferData.m_pBufferData;
    *pBufferSize = pDecoder->m_BufferData.m_uBufferDataSize;

    return TRUE;

Failure:
    return FALSE;
}


XPRESS9_EXPORT
unsigned
XPRESS9_CALL
Xpress9DecoderSetBuffer(
    XPRESS9_STATUS     *pStatus,                // OUT: status
    XPRESS9_DECODER     pXpress9Decoder,        // IN:  decoder
    void              * pBuffer,                // IN
    size_t              pBufferSize             // IN
)
{
    LZ77_DECODER       *pDecoder;

    memset (pStatus, 0, sizeof (*pStatus));
    GET_DECODER_STATE (pStatus, Failure, pDecoder, pXpress9Decoder);

    pDecoder->m_BufferData.m_pBufferData = pBuffer;
    pDecoder->m_BufferData.m_uBufferDataSize = pBufferSize;

    return TRUE;

Failure:
    return FALSE;
}
