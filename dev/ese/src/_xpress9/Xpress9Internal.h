// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#ifdef _XPRESS9INTERNAL_H_
#error Xpress9Internal.h was included from two different directories
#else

#ifdef _MSC_VER
#pragma once
#endif 

#define _XPRESS9INTERNAL_H_


#pragma warning (push)

#pragma warning (disable: 4820)
#pragma warning (disable: 4255)

#include <stdlib.h>
#include <string.h>

#pragma warning (pop)

#include "Xpress9.h"


#pragma warning (disable: 4127)
#pragma warning (disable: 4711)


#ifndef XPRESS9_INTERNAL
#define XPRESS9_INTERNAL
#endif 

#ifndef XPRESS9_3264BIT_COMPAT
#define XPRESS9_3264BIT_COMPAT 1
#endif 

#ifndef XPRESS9_USE_SSE2
#define XPRESS9_USE_SSE2    (defined (_M_IX86) || defined (_M_AMD64) || defined (_M_IA64))
#endif 

#ifndef XPRESS9_USE_SSE4
#define XPRESS9_USE_SSE4    (defined (_M_IX86) || defined (_M_AMD64) || defined (_M_IA64))
#endif 

#ifndef XPRESS9_HAVE_BARREL_SHIFTER
#define XPRESS9_HAVE_BARREL_SHIFTER (defined (_M_IX86) || defined (_M_AMD64) || defined (_M_IA64))
#endif 

#ifndef XPRESS9_DEBUG_PERF_STAT
#define XPRESS9_DEBUG_PERF_STAT 0
#endif 

#if XPRESS9_DEBUG_PERF_STAT
#pragma warning (push)
#pragma warning (disable: 4820)
#pragma warning (disable: 4255)
#include <stdio.h>
#pragma warning (pop)
#define DEBUG_PERF_START(timer)     ((timer) -= __rdtsc ())
#define DEBUG_PERF_STOP(timer)      ((timer) += __rdtsc ())
#else
#define DEBUG_PERF_START(timer)     (0)
#define DEBUG_PERF_STOP(timer)      (0)
#endif 


#ifndef BOOL
typedef unsigned BOOL;
#endif 

#ifndef TRUE
#define TRUE 1
#endif 

#ifndef FALSE
#define FALSE 0
#endif 

#ifndef NULL
#define NULL 0
#endif 


#define MAX(a,b)    ((a) > (b) ? (a) : (b))



#pragma pack (push, 8)






#ifdef _M_IX86
#define __unaligned             
#endif 

typedef unsigned char       UInt8;
typedef   signed char        Int8;
typedef unsigned short      UInt16;
typedef          short       Int16;
typedef unsigned int        UInt32;
typedef          int         Int32;
typedef unsigned __int64    UInt64;
typedef          __int64     Int64;


typedef size_t      uxint;
typedef ptrdiff_t    xint;

#if defined(_M_AMD64) || defined(_M_IA64)
#define BITS_PER_XINT   64
typedef UInt32 uxint_half;
typedef  Int32  xint_half;
#else
#define BITS_PER_XINT   32
typedef UInt16 uxint_half;
typedef  Int16  xint_half;
#endif 





#ifndef XPRESS9_MAX_TRACE_LEVEL
#define XPRESS9_MAX_TRACE_LEVEL     10
#endif 

#define TRACE_LEVEL_ERROR_STATUS_ONLY                       0
#define TRACE_LEVEL_ERROR_STATUS_AND_POSITION               1
#define TRACE_LEVEL_ERROR_STATUS_POSITION_AND_DESCRIPTION   2
#define TRACE_LEVEL_ASSERT                                  3
#define TRACE_LEVEL_DEBUG                                   4
#define TRACE_LEVEL_TRACE                                   5


#define DEBUG_PRINT(pFormat,...) (printf (pFormat "\n", __VA_ARGS__))

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ERROR_STATUS_POSITION_AND_DESCRIPTION
#include <string.h>
#endif 

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT
#pragma warning (push)
#pragma warning (disable: 4820)
#pragma warning (disable: 4255)
#include <stdio.h>
#pragma warning (pop)
#endif 

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT
#define DEBUGBREAK() __debugbreak()
#else
#define DEBUGBREAK() (0)
#endif 


#if XPRESS9_MAX_TRACE_LEVEL == TRACE_LEVEL_ERROR_STATUS_ONLY
#define SET_ERROR(uErrorCode,pStatus,pFormat,...) do {                      \
    pStatus->m_uStatus = uErrorCode;                                        \
} while (0)
#elif XPRESS9_MAX_TRACE_LEVEL == TRACE_LEVEL_ERROR_STATUS_AND_POSITION
#define SET_ERROR(uErrorCode,pStatus,pFormat,...) do {                      \
    pStatus->m_uStatus     = uErrorCode;                                    \
    pStatus->m_uLineNumber = __LINE__;                                      \
    pStatus->m_pFilename   = __FILE__;                                      \
    pStatus->m_pFunction   = __FUNCTION__;                                  \
} while (0)
#else
#define SET_ERROR(uErrorCode,pStatus,pFormat,...) do {                      \
    _snprintf_s (                                                           \
        pStatus->m_ErrorDescription,                                        \
        sizeof (pStatus->m_ErrorDescription) / sizeof (pStatus->m_ErrorDescription[0]) - 1,   \
        _TRUNCATE,                                                          \
        "Error %s: " pFormat,                                               \
        Xpress9GetErrorText (uErrorCode),                                   \
        __VA_ARGS__                                                         \
    );                                                                      \
    DEBUG_PRINT ("%s", pStatus->m_ErrorDescription);                        \
    DEBUGBREAK ();                                                          \
    pStatus->m_uStatus     = uErrorCode;                                    \
    pStatus->m_uLineNumber = __LINE__;                                      \
    pStatus->m_pFilename   = __FILE__;                                      \
    pStatus->m_pFunction   = __FUNCTION__;                                  \
} while (0)
#endif 

#define RETAIL_ASSERT(condition,pFormat,...) do {                           \
    if (!(condition))                                                       \
    {                                                                       \
        SET_ERROR (                                                         \
            Xpress9Status_CorruptedMemory,                                  \
            pStatus,                                                        \
            "Assert %s failed in %s (%s %u), " pFormat,                     \
            #condition,                                                     \
            __FUNCTION__,                                                   \
            __FILE__,                                                       \
            __LINE__,                                                       \
            __VA_ARGS__                                                     \
        );                                                                  \
        goto Failure;                                                       \
    }                                                                       \
} while (0)

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT
#define ASSERT(condition,pFormat,...) do {                                  \
    if (!(condition))                                                       \
    {                                                                       \
        DEBUG_PRINT (                                                       \
            "Assert %s failed in %s (%s %u), " pFormat,                     \
            #condition,                                                     \
            __FUNCTION__,                                                   \
            __FILE__,                                                       \
            __LINE__,                                                       \
            __VA_ARGS__                                                     \
        );                                                                  \
        DEBUGBREAK ();                                                      \
    }                                                                       \
} while (0)
#else
#define ASSERT(condition,pFormat,...) (0)
#endif 

#undef DEBUG
#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_DEBUG
#define DEBUG(pFormat,...) DEBUG_PRINT(pFormat, __VA_ARGS__)
#else
#define DEBUG(pFormat,...) (0)
#endif 

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_TRACE
#define TRACE(pFormat,...) DEBUG_PRINT(pFormat, __VA_ARGS__)
#else
#define TRACE(pFormat,...) (0)
#endif 






typedef uxint       BIO_FULL;
typedef uxint_half  BIO_HALF;

#define BIO_FLUSH_BITS  (sizeof (BIO_HALF) * 8)


#define BIO_DECLARE()                                                                   \
    BIO_FULL    _BitIo_uShiftRegister   = 0;                                            \
    xint        _BitIo_iBitsAvailable   = 0;                                            \
    BIO_HALF   *_BitIo_pBitStreamPtr    = 0

typedef struct BIO_STATE_T BIO_STATE;
struct BIO_STATE_T
{
    BIO_FULL    m_uShiftRegister;
    xint        m_iBitsAvailable;
    BIO_HALF   *m_pBitStreamPtr;
};

#define BIO_STATE_ENTER(state) do {                                                     \
    _BitIo_uShiftRegister = (state).m_uShiftRegister;                                   \
    _BitIo_iBitsAvailable = (state).m_iBitsAvailable;                                   \
    _BitIo_pBitStreamPtr  = (state).m_pBitStreamPtr;                                    \
} while (0)
    
#define BIO_STATE_LEAVE(state) do {                                                     \
    (state).m_uShiftRegister = _BitIo_uShiftRegister;                                   \
    (state).m_iBitsAvailable = _BitIo_iBitsAvailable;                                   \
    (state).m_pBitStreamPtr  = _BitIo_pBitStreamPtr;                                    \
} while (0)

#define BIOWR_INITIALIZE_SHIFT_REGISTER() do {                                          \
    _BitIo_uShiftRegister = 0;                                                          \
    _BitIo_iBitsAvailable = 0;                                                          \
} while (0)

#define BIOWR_SET_OUTPUT_PTR(pBitStreamPtr) (_BitIo_pBitStreamPtr  = (BIO_HALF *) (pBitStreamPtr))
#define BIOWR_SET_OUTPUT_PTR_STATE(state,pBitStreamPtr) ((state).m_pBitStreamPtr  = (BIO_HALF *) (pBitStreamPtr))

#if defined (_M_AMD64) || defined (_M_IA64)
#define BIOWR(uMask,uBitCount) do {                                                     \
    ASSERT(                                                                             \
        (uBitCount) <= BIO_FLUSH_BITS && ((uMask) >> (uBitCount)) == 0,                 \
        "uBitCount=%u uMask=0x%x",                                                      \
        (UInt32) (uBitCount),                                                           \
        (UInt32) (uMask)                                                                \
    );                                                                                  \
    _BitIo_uShiftRegister += ((BIO_FULL) (uMask)) << _BitIo_iBitsAvailable;             \
    _BitIo_iBitsAvailable += (uBitCount);                                               \
    if (((uxint) _BitIo_iBitsAvailable) >= BIO_FLUSH_BITS)                              \
    {                                                                                   \
        _BitIo_iBitsAvailable -= BIO_FLUSH_BITS;                                        \
        *_BitIo_pBitStreamPtr++ = (BIO_HALF) _BitIo_uShiftRegister;                     \
        _BitIo_uShiftRegister >>= BIO_FLUSH_BITS;                                       \
    }                                                                                   \
} while (0)
#else
#define BIOWR(uMask,uBitCount) do {                                                     \
    ASSERT(                                                                             \
        ((uMask) >> (uBitCount)) == 0,                                                  \
        "uBitCount=%u uMask=0x%x",                                                      \
        (UInt32) (uBitCount),                                                           \
        (UInt32) (uMask)                                                                \
    );                                                                                  \
    _BitIo_uShiftRegister += ((BIO_FULL) (uMask)) << _BitIo_iBitsAvailable;             \
    _BitIo_iBitsAvailable += (uBitCount);                                               \
    if (((uxint) _BitIo_iBitsAvailable) >= BIO_FLUSH_BITS)                              \
    {                                                                                   \
        _BitIo_iBitsAvailable -= BIO_FLUSH_BITS;                                        \
        *_BitIo_pBitStreamPtr++ = (BIO_HALF) _BitIo_uShiftRegister;                     \
        _BitIo_uShiftRegister >>= BIO_FLUSH_BITS;                                       \
        if (((uxint) _BitIo_iBitsAvailable) >= BIO_FLUSH_BITS)                          \
        {                                                                               \
            ASSERT (                                                                    \
                (uBitCount) >= (uxint) _BitIo_iBitsAvailable,                           \
                "BitCount=%u BitsAvailable=%u",                                         \
                (UInt32) uBitCount,                                                     \
                (UInt32) _BitIo_iBitsAvailable                                          \
            );                                                                          \
 \
            _BitIo_uShiftRegister = ((BIO_FULL) (uMask)) >> ((uBitCount) - _BitIo_iBitsAvailable); \
            _BitIo_iBitsAvailable -= BIO_FLUSH_BITS;                                    \
            *_BitIo_pBitStreamPtr++ = (BIO_HALF) _BitIo_uShiftRegister;                 \
            _BitIo_uShiftRegister >>= BIO_FLUSH_BITS;                                   \
        }                                                                               \
    }                                                                                   \
} while (0)
#endif 

#define BIOWR_FLUSH() do {                                                              \
    if (_BitIo_iBitsAvailable != 0)                                                     \
    {                                                                                   \
        *_BitIo_pBitStreamPtr = (BIO_HALF) _BitIo_uShiftRegister;                       \
        _BitIo_pBitStreamPtr = (BIO_HALF *) ((char *) _BitIo_pBitStreamPtr + ((_BitIo_iBitsAvailable + 7) >> 3)); \
    }                                                                                   \
} while (0)

#define BIOWR_TELL_SHIFT_REGISTER_BITS() ((uxint) _BitIo_iBitsAvailable)
#define BIOWR_TELL_SHIFT_REGISTER_BITS_STATE(state) ((uxint) (state).m_iBitsAvailable)

#define BIOWR_TELL_OUTPUT_PTR() ((UInt8 *) _BitIo_pBitStreamPtr)
#define BIOWR_TELL_OUTPUT_PTR_STATE(state) ((UInt8 *) (state).m_pBitStreamPtr)


#define BIORD_SET_INPUT_PTR(pBitStreamPtr) do {                                         \
    _BitIo_pBitStreamPtr  = (BIO_HALF *) (pBitStreamPtr);                               \
} while (0)
#define BIORD_SET_INPUT_PTR_STATE(state,pBitStreamPtr) do {                             \
    (state).m_pBitStreamPtr  = (BIO_HALF *) (pBitStreamPtr);                            \
} while (0)

#define BIORD_PRELOAD_SHIFT_REGISTER() do {                                             \
    _BitIo_iBitsAvailable = BIO_FLUSH_BITS;                                             \
    _BitIo_uShiftRegister = * (BIO_FULL *) _BitIo_pBitStreamPtr;                        \
    _BitIo_pBitStreamPtr += 2;                                                          \
} while (0)


#define BIORD_LOOKUP_FIXED(uBitCount) (_BitIo_uShiftRegister & ((((uxint) 1) << (uBitCount)) - 1))

#define BIORD_LOOKUP(uBitCount) (_BitIo_uShiftRegister & MASK2 (uBitCount))

#define BIORD_SKIP(uBitCount) do {                                                      \
    ASSERT (                                                                            \
        (uBitCount) <= BIO_FLUSH_BITS,                                                  \
        "uBitCount=%u",                                                                 \
        (UInt32) (uBitCount)                                                            \
    );                                                                                  \
    _BitIo_uShiftRegister >>= (uBitCount);                                              \
    if ((_BitIo_iBitsAvailable -= (xint) (uBitCount)) < 0)                              \
    {                                                                                   \
        _BitIo_iBitsAvailable += BIO_FLUSH_BITS;                                        \
        _BitIo_uShiftRegister += ((BIO_FULL) *_BitIo_pBitStreamPtr++) << _BitIo_iBitsAvailable; \
    }                                                                                   \
} while (0)


#if defined (_M_AMD64) || defined (_M_IA64)
#define BIORD_LOAD(uValue,uBitCount) do {                                               \
    uValue = BIORD_LOOKUP (uBitCount);                                                  \
    BIORD_SKIP (uBitCount);                                                             \
} while (0)
#else
#define BIORD_LOAD(uValue,uBitCount) do {                                               \
    if ((uBitCount) <= BIO_FLUSH_BITS)                                                  \
    {                                                                                   \
        uValue = BIORD_LOOKUP (uBitCount);                                              \
        BIORD_SKIP (uBitCount);                                                         \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
        uValue = BIORD_LOOKUP (BIO_FLUSH_BITS);                                         \
        BIORD_SKIP (BIO_FLUSH_BITS);                                                    \
        uValue += BIORD_LOOKUP ((uBitCount) - BIO_FLUSH_BITS) << BIO_FLUSH_BITS;        \
        BIORD_SKIP ((uBitCount) - BIO_FLUSH_BITS);                                      \
    }                                                                                   \
} while (0)
#endif 

#define BIORD_TELL_SHIFT_REGISTER_BITS() (BIO_FLUSH_BITS + (uxint) _BitIo_iBitsAvailable)
#define BIORD_TELL_SHIFT_REGISTER_BITS_STATE(state) (BIO_FLUSH_BITS + (uxint) (state).m_iBitsAvailable)
    
#define BIORD_TELL_OUTPUT_PTR() ((UInt8 *) _BitIo_pBitStreamPtr)
#define BIORD_TELL_OUTPUT_PTR_STATE(state) ((UInt8 *) (state).m_pBitStreamPtr)

#define BIORD_TELL_CONSUMED_BITS_STATE(state,ptr) \
    (((BIORD_TELL_OUTPUT_PTR_STATE (state) - (UInt8 *) (ptr)) << 3) - BIORD_TELL_SHIFT_REGISTER_BITS_STATE (state))








#define HUFFMAN_CODEWORD_LENGTH_LIMIT   64


typedef UInt32 HUFFMAN_COUNT;
typedef UInt32 HUFFMAN_MASK;

#define HUFFMAN_CODEWORD_LENGTH_BITS     5
#define HUFFMAN_MAX_CODEWORD_LENGTH     27

#if HUFFMAN_CODEWORD_LENGTH_BITS + HUFFMAN_MAX_CODEWORD_LENGTH > 32
#error HUFFMAN_CODEWORD_LENGTH_BITS + HUFFMAN_MAX_CODEWORD_LENGTH > 32
#endif 

#define HUFFMAN_GET_CODEWORD_VALUE(x)   ((x) >> HUFFMAN_CODEWORD_LENGTH_BITS)
#define HUFFMAN_GET_CODEWORD_LENGTH(x)  ((x) & ((1 << HUFFMAN_CODEWORD_LENGTH_BITS) - 1))

typedef struct HUFFMAN_NODE_T HUFFMAN_NODE;
struct HUFFMAN_NODE_T
{
    HUFFMAN_NODE   *m_pSon[2];
    HUFFMAN_NODE   *m_pNext;
    HUFFMAN_COUNT   m_uCount;
    UInt16          m_uSymbol;
    UInt16          m_uBits;
};


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
);


#define HUFFMAN_WRITE_SYMBOL(uPackedCodeword) do {                          \
    uxint _uSymbolCodeValue;                                                \
    uxint _uSymbolCodeLength;                                               \
    _uSymbolCodeValue  = (uPackedCodeword);                                 \
    _uSymbolCodeLength = HUFFMAN_GET_CODEWORD_LENGTH (_uSymbolCodeValue);   \
    _uSymbolCodeValue  = HUFFMAN_GET_CODEWORD_VALUE  (_uSymbolCodeValue);   \
    BIOWR (_uSymbolCodeValue, _uSymbolCodeLength);                          \
} while (0)


#pragma warning (push)
#pragma warning (disable: 4820)
typedef struct HUFFMAN_DECODE_NODE_T HUFFMAN_DECODE_NODE;
struct HUFFMAN_DECODE_NODE_T
{
    HUFFMAN_DECODE_NODE *m_pNext;
    UInt16  m_uSymbol;
    UInt16  m_uBits;
};
#pragma warning (pop)

typedef Int16 HUFFMAN_DECODE_TABLE_ENTRY;


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
);


#define HUFFMAN_DECODE_SYMBOL(iSymbol,pDecodeTable,ROOT_LOOKUP_BITS,TAIL_LOOKUP_BITS) do {  \
    uxint _uBits;                                                                           \
    iSymbol = BIORD_LOOKUP_FIXED (ROOT_LOOKUP_BITS);                                        \
    iSymbol = (pDecodeTable)[iSymbol];                                                      \
    if (((xint) iSymbol) < 0)                                                               \
    {                                                                                       \
        HUFFMAN_DECODE_TABLE_ENTRY *_pNextTable;                                            \
        _uBits = (uxint) (iSymbol & 15);                                                    \
        BIORD_SKIP (_uBits);                                                                \
        _pNextTable = pDecodeTable + (((xint) iSymbol) & ~ (xint) 15);                      \
        while ((xint) (iSymbol = _pNextTable[BIORD_LOOKUP_FIXED (TAIL_LOOKUP_BITS)]) < 0)   \
        {                                                                                   \
            _uBits = (uxint) (iSymbol & 15);                                                \
            BIORD_SKIP (_uBits);                                                            \
            _pNextTable += ((xint) iSymbol) & ~ (xint) 15;                                  \
        }                                                                                   \
    }                                                                                       \
    _uBits = (uxint) (iSymbol & 15);                                                        \
    BIORD_SKIP (_uBits);                                                                    \
    iSymbol >>= 4;                                                                          \
} while (0)


#define HUFFMAN_ENCODED_TABLE_FILL                  (HUFFMAN_MAX_CODEWORD_LENGTH + 1)
#define HUFFMAN_ENCODED_TABLE_ZERO_REPT             (HUFFMAN_MAX_CODEWORD_LENGTH + 2)
#define HUFFMAN_ENCODED_TABLE_PREV                  (HUFFMAN_MAX_CODEWORD_LENGTH + 3)
#define HUFFMAN_ENCODED_TABLE_ROW_0                 (HUFFMAN_MAX_CODEWORD_LENGTH + 4)
#define HUFFMAN_ENCODED_TABLE_ROW_1                 (HUFFMAN_MAX_CODEWORD_LENGTH + 5)
#define HUFFMAN_ENCODED_TABLE_SIZE                  (HUFFMAN_MAX_CODEWORD_LENGTH + 6)
#define HUFFMAN_ENCODED_TABLE_ZERO_REPT_MIN_COUNT   5
#define HUFFMAN_ENCODED_TABLE_FILL_BOUNDARY         16


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
);


XPRESS9_INTERNAL
void
HuffmanDecodeLengthTable (
    XPRESS9_STATUS *pStatus,
    BIO_STATE      *pBioState,
    UInt8          *puBitLengthTable,
    uxint           uAlphabetSize,
    uxint           uFillBoundary,
    uxint           uMaxCodewordLength
);






#define MEMORY_ALIGNMENT   256

typedef unsigned long MSB_T;
#define GET_MSB(uMsb,uValue)    ((void) _BitScanReverse (&uMsb, (unsigned) (uValue)))

#if XPRESS9_HAVE_BARREL_SHIFTER

#define POWER2(uMsb)            (((uxint) 1) << (uMsb))
#define MASK2(uMsb)             (POWER2 (uMsb) - 1)

#else

__declspec(align(64))
static
uxint
s_uPower2Table[32] = {
    0x00000001u,
    0x00000002u,
    0x00000004u,
    0x00000008u,
    0x00000010u,
    0x00000020u,
    0x00000040u,
    0x00000080u,
    0x00000100u,
    0x00000200u,
    0x00000400u,
    0x00000800u,
    0x00001000u,
    0x00002000u,
    0x00004000u,
    0x00008000u,
    0x00010000u,
    0x00020000u,
    0x00040000u,
    0x00080000u,
    0x00100000u,
    0x00200000u,
    0x00400000u,
    0x00800000u,
    0x01000000u,
    0x02000000u,
    0x04000000u,
    0x08000000u,
    0x10000000u,
    0x20000000u,
    0x40000000u,
    0x80000000u,
};

__declspec(align(64))
static
uxint
s_uMask2Table[32] = {
    0x00000000u,
    0x00000001u,
    0x00000003u,
    0x00000007u,
    0x0000000fu,
    0x0000001fu,
    0x0000003fu,
    0x0000007fu,
    0x000000ffu,
    0x000001ffu,
    0x000003ffu,
    0x000007ffu,
    0x00000fffu,
    0x00001fffu,
    0x00003fffu,
    0x00007fffu,
    0x0000ffffu,
    0x0001ffffu,
    0x0003ffffu,
    0x0007ffffu,
    0x000fffffu,
    0x001fffffu,
    0x003fffffu,
    0x007fffffu,
    0x00ffffffu,
    0x01ffffffu,
    0x03ffffffu,
    0x07ffffffu,
    0x0fffffffu,
    0x1fffffffu,
    0x3fffffffu,
    0x7fffffffu,
};

#define POWER2(uMsb)    s_uPower2Table[(uMsb)]
#define MASK2(uMsb)     s_uMask2Table[(uMsb)]

#endif 


XPRESS9_INTERNAL
uxint
HuffmanReverseMask (
    uxint   uMask,
    uxint   uBits
);


XPRESS9_INTERNAL
UInt32
Xpress9Crc32 (
    const UInt8    *pData,
    uxint           uSize,
    UInt32          uCrc
);





typedef UInt32  LZ77_INDEX;

#define LZ77_MAX_MTF                    4

#define LZ77_MAX_SHORT_LENGTH_LOG       4
#define LZ77_MAX_SHORT_LENGTH           (1 << LZ77_MAX_SHORT_LENGTH_LOG)

#define LZ77_MAX_WINDOW_SIZE_LOG        24


#define LZ77_LONG_LENGTH_ALPHABET_SIZE_LOG  9
#define LZ77_LONG_LENGTH_ALPHABET_SIZE  (1 << (LZ77_LONG_LENGTH_ALPHABET_SIZE_LOG - 1))
#define LZ77_MAX_LONG_LENGTH            (LZ77_LONG_LENGTH_ALPHABET_SIZE - LZ77_MAX_WINDOW_SIZE_LOG)

#define LZ77_SHORT_SYMBOL_ALPHABET_SIZE (256 + ((LZ77_MAX_WINDOW_SIZE_LOG + LZ77_MAX_MTF) << LZ77_MAX_SHORT_LENGTH_LOG))

#define LZ77_SHORT_SYMBOL_ALPHABET_SIZE_LOG 10

#if LZ77_SHORT_SYMBOL_ALPHABET_SIZE >= (1 << LZ77_SHORT_SYMBOL_ALPHABET_SIZE_LOG)
#error LZ77_SHORT_SYMBOL_ALPHABET_SIZE exceeds (1 << LZ77_SHORT_SYMBOL_ALPHABET_SIZE_LOG)
#endif

#if LZ77_SHORT_SYMBOL_ALPHABET_SIZE <= (1 << (LZ77_SHORT_SYMBOL_ALPHABET_SIZE_LOG - 1))
#error LZ77_SHORT_SYMBOL_ALPHABET_SIZE is not greater than (1 << LZ77_SHORT_SYMBOL_ALPHABET_SIZE_LOG)
#endif


#define LZ77_ENCODER_STATE_READY                0
#define LZ77_ENCODER_STATE_ENCODING             1
#define LZ77_ENCODER_STATE_FLUSHING             2


typedef struct LZ77_IR_STATE_T        LZ77_IR_STATE;
typedef struct LZ77_PASS2_STATE_T     LZ77_PASS2_STATE;
typedef struct LZ77_PASS1_STATE_T     LZ77_PASS1_STATE;

typedef
void
Lz77EncPass1_Proc (
    LZ77_PASS1_STATE *pState
);

typedef
void
Lz77EncPass2_Proc (
    LZ77_PASS2_STATE *pPass2,
    const void       *pIrStop
);

typedef
void
Lz77EncInsert_Proc (
    LZ77_PASS1_STATE   *pState
);


typedef struct LZ77_MTF_STATE_T LZ77_MTF_STATE;
struct LZ77_MTF_STATE_T
{
    xint            m_iMtfLastPtr;
    xint            m_iMtfOffset[LZ77_MAX_MTF];
};


struct LZ77_IR_STATE_T
{
    UInt8          *m_pIrBuffer;
    uxint           m_uIrBufferSize;
    UInt8          *m_pIrPtr;
    const UInt8    *m_pIrSrc;
    LZ77_MTF_STATE  m_Mtf;
};

struct LZ77_PASS2_STATE_T
{
    HUFFMAN_MASK    m_uShortSymbolMask[LZ77_SHORT_SYMBOL_ALPHABET_SIZE];

    HUFFMAN_MASK    m_uLongLengthMask[LZ77_LONG_LENGTH_ALPHABET_SIZE];

    LZ77_IR_STATE   m_Ir;

    uxint           m_uComputedEncodedSizeBits;
    uxint           m_uActualEncodedSizeBits;

    uxint           m_uBytesCopiedFromScratch;

    BIO_STATE       m_BioState;

    LZ77_PASS1_STATE *m_pState;
};


#define XPRESS9_ENCODER_MAGIC 'exp9'

typedef struct LZ77_STAT_T LZ77_STAT;
struct LZ77_STAT_T
{
    UInt64          m_uCount;
    UInt64          m_uOrigSize;
    UInt64          m_uCompSize;
};

#pragma warning (push)
#pragma warning (disable: 4820)
struct LZ77_PASS1_STATE_T
{
    LZ77_INDEX      m_uNext[8];

    struct
    {
        HUFFMAN_NODE    m_HuffmanNodeTemp[2 * MAX (LZ77_SHORT_SYMBOL_ALPHABET_SIZE, LZ77_LONG_LENGTH_ALPHABET_SIZE)];
    
        UInt8          *m_pScratchArea;
        uxint           m_uScratchAreaSize;
    } m_Scratch;

    LZ77_PASS2_STATE m_Pass2;

    struct
    {
        LZ77_INDEX     *m_pHashTable;
        uxint           m_uHashTableSizeMax;
        uxint           m_uHashTableSizeCurrent;
    } m_HashTable;

    UInt8          *m_pAllocatedMemory;
    uxint           m_uMagic;
    uxint           m_uState;
    uxint           m_uMaxWindowSizeLog2;
    uxint           m_uRuntimeFlags;

    struct
    {
        UInt8          *m_pBufferData;
        uxint           m_uBufferDataSize;
    } m_BufferData;

    struct
    {
        const UInt8    *m_pUserData;
        uxint           m_uUserDataSize;
        uxint           m_uUserDataProcessed;
        uxint           m_fFlush;
    } m_UserData;

    struct
    {
        UInt8          *m_pData;
        uxint           m_uDataSize;
        uxint           m_uWindowSize;
        uxint           m_uEncodePosition;
        uxint           m_uHashInsertPosition;
        LZ77_MTF_STATE  m_Mtf;
    } m_EncodeData;

    struct
    {
        Lz77EncPass1_Proc      *m_pLz77EncPass1;
        Lz77EncPass2_Proc      *m_pLz77EncPass2;
        Lz77EncInsert_Proc     *m_pLz77EncInsert;
        uxint                   m_fDirty;
        XPRESS9_ENCODER_PARAMS  m_Delayed;
        XPRESS9_ENCODER_PARAMS  m_Current;
    } m_Params;

#if XPRESS9_DEBUG_PERF_STAT
    struct
    {
        UInt64      m_uCallback;
        UInt64      m_uMemcpy;
        UInt64      m_uInsert;
        UInt64      m_uShift;
        UInt64      m_uPass1;
        UInt64      m_uHuffman;
        UInt64      m_uPass2;
    } m_DebugPerf;
#endif 

    struct
    {
        LZ77_STAT   m_Encoder;
        LZ77_STAT   m_Session;
        LZ77_STAT   m_Block;
        UInt32      m_uSessionSignature;
        UInt32      m_uBlockIndex;
    } m_Stat;

    struct
    {
        HUFFMAN_COUNT   m_uLongLengthCount[LZ77_LONG_LENGTH_ALPHABET_SIZE];
        
        HUFFMAN_COUNT   m_uShortSymbolCount[LZ77_SHORT_SYMBOL_ALPHABET_SIZE];

        MSB_T           m_uStoredBitCount;
    } m_HuffmanStat;
};
#pragma warning (pop)

#define STATE (pState[-1])






#define XPRESS9_MAGIC       0x4e86d72a

typedef struct LZ77_BLOCK_HEADER_T LZ77_BLOCK_HEADER;

struct LZ77_BLOCK_HEADER_T
{
    UInt32      m_uMagic;
    UInt32      m_uOrigSizeBytes;
    UInt32      m_uCompSizeBits;
    UInt32      m_uFlags;
    UInt32      m_uReserved;
    UInt32      m_uSessionSignature;
    UInt32      m_uBlockSignature;
    UInt32      m_uHeaderSignature;
};






#define XPRESS9_DECODER_MAGIC 'd9xp'


#define LZ77_SHORT_SYMBOL_DECODE_ROOT_BITS          12
#define LZ77_SHORT_SYMBOL_DECODE_TAIL_BITS          6
#define LZ77_SHORT_SYMBOL_DECODE_TAIL_TABLE_COUNT   64

#define LZ77_LONG_LENGTH_DECODE_ROOT_BITS           12
#define LZ77_LONG_LENGTH_DECODE_TAIL_BITS           6
#define LZ77_LONG_LENGTH_DECODE_TAIL_TABLE_COUNT    64

#define LZ77_DECODER_SCRATCH_AREA_SIZE              (8*1024)


#define LZ77_DECODER_STATE_READY                0
#define LZ77_DECODER_STATE_WAIT_SESSION         1
#define LZ77_DECODER_STATE_WAIT_BLOCK           2
#define LZ77_DECODER_STATE_WAIT_HUFFMAN         3
#define LZ77_DECODER_STATE_DECODING             4

typedef struct LZ77_DECODER_T LZ77_DECODER;

typedef
void
Xpress9Lz77DecProc (
    XPRESS9_STATUS  *pStatus,
    LZ77_DECODER    *pDecoder
);


#pragma warning (push)
#pragma warning (disable: 4820)
struct LZ77_DECODER_T
{
    uxint           m_uMagic;
    uxint           m_uState;
    UInt8          *m_pAllocatedMemory;
    uxint           m_uMaxWindowSizeLog2;
    uxint           m_uRuntimeFlags;

    struct
    {
        UInt64          m_uBufferOffset;

        uxint           m_uDecodePosition;
        uxint           m_uStopPosition;
        uxint           m_uEndOfBuffer;
        uxint           m_uCopyPosition;

        uxint           m_uDecodedSizeBits;
        uxint           m_uEncodedSizeBits;

        uxint           m_uBlockDecodedBytes;
        uxint           m_uBlockCopiedBytes;

        uxint           m_uScratchBytesStored;
        uxint           m_uScratchBytesProcessed;

        uxint           m_uWindowSizeLog2;
        uxint           m_uHuffmanTableBits;
        uxint           m_uMtfEntryCount;
        uxint           m_uPtrMinMatchLength;
        uxint           m_uMtfMinMatchLength;

        Xpress9Lz77DecProc *m_pLz77DecProc;

        HUFFMAN_DECODE_TABLE_ENTRY *m_piShortSymbolRoot;
        HUFFMAN_DECODE_TABLE_ENTRY *m_piLongLengthRoot;

        BIO_STATE       m_BioState;

        LZ77_MTF_STATE  m_Mtf;

        struct
        {
            uxint       m_uLength;
            xint        m_iOffset;
        } m_Tail;
    } m_DecodeData;


    struct
    {
        UInt8          *m_pBufferData;
        uxint           m_uBufferDataSize;
    } m_BufferData;

    struct
    {
        const UInt8    *m_pUserData;
        uxint           m_uUserDataSize;
        uxint           m_uUserDataRead;
    } m_UserData;

#if XPRESS9_DEBUG_PERF_STAT
    struct
    {
        UInt64      m_uHuffman;
        UInt64      m_uShift;
        UInt64      m_uDecoder;
        UInt64      m_uMemcpyOrig;
        UInt64      m_uMemcpyComp;
    } m_DebugPerf;
#endif 

    struct
    {
        LZ77_STAT   m_Encoder;
        LZ77_STAT   m_Session;
        LZ77_STAT   m_Block;
        UInt32      m_uSessionSignature;
        UInt32      m_uBlockIndex;
    } m_Stat;


    struct
    {
        HUFFMAN_DECODE_TABLE_ENTRY  m_iShortSymbolDecodeTable[(1 << LZ77_SHORT_SYMBOL_DECODE_ROOT_BITS) + (1 << LZ77_SHORT_SYMBOL_DECODE_TAIL_BITS) * LZ77_SHORT_SYMBOL_DECODE_TAIL_TABLE_COUNT];
        HUFFMAN_DECODE_TABLE_ENTRY  m_iLongLengthDecodeTable[(1 << LZ77_LONG_LENGTH_DECODE_ROOT_BITS) + (1 << LZ77_LONG_LENGTH_DECODE_TAIL_BITS) * LZ77_LONG_LENGTH_DECODE_TAIL_TABLE_COUNT];
    } m_Huffman;

    struct
    {
        UInt8               m_uScratchArea[LZ77_DECODER_SCRATCH_AREA_SIZE + 256];
        HUFFMAN_DECODE_NODE m_HuffmanNodeTemp[MAX (LZ77_SHORT_SYMBOL_ALPHABET_SIZE + LZ77_SHORT_SYMBOL_DECODE_TAIL_TABLE_COUNT, LZ77_LONG_LENGTH_ALPHABET_SIZE + LZ77_LONG_LENGTH_DECODE_TAIL_TABLE_COUNT)];
    } m_Scratch;
};

#pragma warning (pop)



#pragma pack (pop)


#endif 
