// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#ifdef _XPRESS9INTERNAL_H_
#error Xpress9Internal.h was included from two different directories
#else

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

#define _XPRESS9INTERNAL_H_


#pragma warning (push)

#pragma warning (disable: 4820) // include\codeanalysis\sourceannotations.h 'PreAttribute' : '4' bytes padding added after data member 'Access'
#pragma warning (disable: 4255) // stdlib.h(215) : '_get_purecall_handler' : no function prototype given: converting '()' to '(void)'

#include <stdlib.h>             // size_t
#include <string.h>             // memcpy, memset

#pragma warning (pop)

#include "Xpress9.h"


#pragma warning (disable: 4127) // conditional expression is constant
#pragma warning (disable: 4711) // function 'ShiftValues' selected for automatic inline expansion


#ifndef XPRESS9_INTERNAL
#define XPRESS9_INTERNAL
#endif /* XPRESS9_INTERNAL */

// generate identical results on both 32 and 64 bit platforms
#ifndef XPRESS9_3264BIT_COMPAT
#define XPRESS9_3264BIT_COMPAT 1
#endif /* XPRESS9_3264BIT_COMPAT */

// do not use SSE on X86, AMD64, IA64 platforms
#ifndef XPRESS9_USE_SSE2
#define XPRESS9_USE_SSE2    (defined (_M_IX86) || defined (_M_AMD64) || defined (_M_IA64))
#endif /* XPRESS9_USE_SSE2 */

#ifndef XPRESS9_USE_SSE4
#define XPRESS9_USE_SSE4    (defined (_M_IX86) || defined (_M_AMD64) || defined (_M_IA64))
#endif /* XPRESS9_USE_SSE4 */

// if have barrel shifter use shifts to generate power of 2, otherwise use table
#ifndef XPRESS9_HAVE_BARREL_SHIFTER
#define XPRESS9_HAVE_BARREL_SHIFTER (defined (_M_IX86) || defined (_M_AMD64) || defined (_M_IA64))
#endif /* XPRESS9_HAVE_BARREL_SHIFTER */

// measure per-stage performance using __rdtsc()
#ifndef XPRESS9_DEBUG_PERF_STAT
#define XPRESS9_DEBUG_PERF_STAT 0
#endif /* XPRESS9_DEBUG_PERF_STAT */

#if XPRESS9_DEBUG_PERF_STAT
#pragma warning (push)
#pragma warning (disable: 4820) // stdio.h(62) : _iobuf' : '4' bytes padding added after data member '_cnt'
#pragma warning (disable: 4255) // stdio.h(381) : '_get_printf_count_output' : no function prototype given: converting '()' to '(void)'
#include <stdio.h>
#pragma warning (pop)
#define DEBUG_PERF_START(timer)     ((timer) -= __rdtsc ())
#define DEBUG_PERF_STOP(timer)      ((timer) += __rdtsc ())
#else
#define DEBUG_PERF_START(timer)     (0)
#define DEBUG_PERF_STOP(timer)      (0)
#endif /* XPRESS9_DEBUG_PERF_STAT */


// few useful defines in case they are omitted in standard headers
#ifndef BOOL
typedef unsigned BOOL;
#endif /* BOOL */

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

#ifndef NULL
#define NULL 0
#endif /* NULL */


#define MAX(a,b)    ((a) > (b) ? (a) : (b))



#pragma pack (push, 8)



/* ------------------------------------------ Basic types ------------------------------------------- */
/*                                            -----------                                             */

#ifdef _M_IX86
#define __unaligned             /* x86 does not have __unaligned keyword    */
#endif /* _M_IX86 */

typedef unsigned char       UInt8;
typedef   signed char        Int8;
typedef unsigned short      UInt16;
typedef          short       Int16;
typedef unsigned int        UInt32;
typedef          int         Int32;
typedef unsigned __int64    UInt64;
typedef          __int64     Int64;


//
// declare "fastest" integer type (native register type). Shall be at least 32-bit wide.
//
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
#endif /* defined(_M_AMD64) || defined(_M_IA64) */


/* -------------------------------- Debugging/tracing/error handling -------------------------------- */
/*                                  --------------------------------                                  */

#ifndef XPRESS9_MAX_TRACE_LEVEL
#define XPRESS9_MAX_TRACE_LEVEL     10          // set max trace level (for debugging purposes only)
#endif /* XPRESS9_MAX_TRACE_LEVEL */

#define TRACE_LEVEL_ERROR_STATUS_ONLY                       0
#define TRACE_LEVEL_ERROR_STATUS_AND_POSITION               1
#define TRACE_LEVEL_ERROR_STATUS_POSITION_AND_DESCRIPTION   2
#define TRACE_LEVEL_ASSERT                                  3
#define TRACE_LEVEL_DEBUG                                   4
#define TRACE_LEVEL_TRACE                                   5


#define DEBUG_PRINT(pFormat,...) (printf (pFormat "\n", __VA_ARGS__))

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ERROR_STATUS_POSITION_AND_DESCRIPTION
#include <string.h>
#endif /* XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ERROR_STATUS_POSITION_AND_DESCRIPTION */

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT
#pragma warning (push)
#pragma warning (disable: 4820) // stdio.h(62) : _iobuf' : '4' bytes padding added after data member '_cnt'
#pragma warning (disable: 4255) // stdio.h(381) : '_get_printf_count_output' : no function prototype given: converting '()' to '(void)'
#include <stdio.h>
#pragma warning (pop)
#endif /* XPRESS9_MAX_TRACE_LEVEL >= XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT */

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT
#define DEBUGBREAK() __debugbreak()
#else
#define DEBUGBREAK() (0)
#endif /* XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT */


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
#endif /* XPRESS9_MAX_TRACE_LEVEL == TRACE_LEVEL_ERROR_STATUS_ONLY */

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
#endif /* XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT */

#undef DEBUG
#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_DEBUG
#define DEBUG(pFormat,...) DEBUG_PRINT(pFormat, __VA_ARGS__)
#else
#define DEBUG(pFormat,...) (0)
#endif /* XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_DEBUG */

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_TRACE
#define TRACE(pFormat,...) DEBUG_PRINT(pFormat, __VA_ARGS__)
#else
#define TRACE(pFormat,...) (0)
#endif /* XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_TRACE */



/* ---------------------------------------- Bitstream I/O ------------------------------------------- */
/*                                          -------------                                             */

typedef uxint       BIO_FULL;
typedef uxint_half  BIO_HALF;

#define BIO_FLUSH_BITS  (sizeof (BIO_HALF) * 8)


//
// declare appropriate local variable and initialize shift register for writing
//
#define BIO_DECLARE()                                                                   \
    BIO_FULL    _BitIo_uShiftRegister   = 0;                                            \
    xint        _BitIo_iBitsAvailable   = 0;                                            \
    BIO_HALF   *_BitIo_pBitStreamPtr    = 0

//
// bitstream IO state
//
typedef struct BIO_STATE_T BIO_STATE;
struct BIO_STATE_T
{
    BIO_FULL    m_uShiftRegister;
    xint        m_iBitsAvailable;
    BIO_HALF   *m_pBitStreamPtr;
};

//
// create appropriate local variables and copy in-memory state there
//
#define BIO_STATE_ENTER(state) do {                                                     \
    _BitIo_uShiftRegister = (state).m_uShiftRegister;                                   \
    _BitIo_iBitsAvailable = (state).m_iBitsAvailable;                                   \
    _BitIo_pBitStreamPtr  = (state).m_pBitStreamPtr;                                    \
} while (0)
    
//
// save local variables into in-memory state
//
#define BIO_STATE_LEAVE(state) do {                                                     \
    (state).m_uShiftRegister = _BitIo_uShiftRegister;                                   \
    (state).m_iBitsAvailable = _BitIo_iBitsAvailable;                                   \
    (state).m_pBitStreamPtr  = _BitIo_pBitStreamPtr;                                    \
} while (0)

//
// initialize shift register for writing
//
#define BIOWR_INITIALIZE_SHIFT_REGISTER() do {                                          \
    _BitIo_uShiftRegister = 0;                                                          \
    _BitIo_iBitsAvailable = 0;                                                          \
} while (0)

//
// set output memory area
//
#define BIOWR_SET_OUTPUT_PTR(pBitStreamPtr) (_BitIo_pBitStreamPtr  = (BIO_HALF *) (pBitStreamPtr))
#define BIOWR_SET_OUTPUT_PTR_STATE(state,pBitStreamPtr) ((state).m_pBitStreamPtr  = (BIO_HALF *) (pBitStreamPtr))

//
// write into output bitstream uBitCount bits of uMask value
//
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
/* Of the bits that we are trying to write, the previous write (above if ) already wrote Least significant uBitCount - _BitIo_iBitsAvailable bits */ \
            _BitIo_uShiftRegister = ((BIO_FULL) (uMask)) >> ((uBitCount) - _BitIo_iBitsAvailable); \
            _BitIo_iBitsAvailable -= BIO_FLUSH_BITS;                                    \
            *_BitIo_pBitStreamPtr++ = (BIO_HALF) _BitIo_uShiftRegister;                 \
            _BitIo_uShiftRegister >>= BIO_FLUSH_BITS;                                   \
        }                                                                               \
    }                                                                                   \
} while (0)
#endif /* defined (_M_AMD64) || defined (_M_IA64) */

//
// flushes remaining bits in shift register buffer into memory
//
#define BIOWR_FLUSH() do {                                                              \
    if (_BitIo_iBitsAvailable != 0)                                                     \
    {                                                                                   \
        *_BitIo_pBitStreamPtr = (BIO_HALF) _BitIo_uShiftRegister;                       \
        _BitIo_pBitStreamPtr = (BIO_HALF *) ((char *) _BitIo_pBitStreamPtr + ((_BitIo_iBitsAvailable + 7) >> 3)); \
    }                                                                                   \
} while (0)

//
// returns number of bits stored in shift register
//
#define BIOWR_TELL_SHIFT_REGISTER_BITS() ((uxint) _BitIo_iBitsAvailable)
#define BIOWR_TELL_SHIFT_REGISTER_BITS_STATE(state) ((uxint) (state).m_iBitsAvailable)

//
// returns output pointer
//
#define BIOWR_TELL_OUTPUT_PTR() ((UInt8 *) _BitIo_pBitStreamPtr)
#define BIOWR_TELL_OUTPUT_PTR_STATE(state) ((UInt8 *) (state).m_pBitStreamPtr)


//
// set input memory area
//
#define BIORD_SET_INPUT_PTR(pBitStreamPtr) do {                                         \
    _BitIo_pBitStreamPtr  = (BIO_HALF *) (pBitStreamPtr);                               \
} while (0)
#define BIORD_SET_INPUT_PTR_STATE(state,pBitStreamPtr) do {                             \
    (state).m_pBitStreamPtr  = (BIO_HALF *) (pBitStreamPtr);                            \
} while (0)

//
// preload shift register with data
//
#define BIORD_PRELOAD_SHIFT_REGISTER() do {                                             \
    _BitIo_iBitsAvailable = BIO_FLUSH_BITS;                                             \
    _BitIo_uShiftRegister = * (BIO_FULL *) _BitIo_pBitStreamPtr;                        \
    _BitIo_pBitStreamPtr += 2;                                                          \
} while (0)


//
// Lookup at uBitCount least significant bits from shift register which is guaranteed to
// have at least BIO_FLUSH_BITS bits available
//
#define BIORD_LOOKUP_FIXED(uBitCount) (_BitIo_uShiftRegister & ((((uxint) 1) << (uBitCount)) - 1))

#define BIORD_LOOKUP(uBitCount) (_BitIo_uShiftRegister & MASK2 (uBitCount))

//
// remove uBitCount bits from shift register loading more bits from memory
// if number of bits in shift register drops below BIO_FLUSH_BITS
//
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
#endif /* defined (_M_AMD64) || defined (_M_IA64) */

//
// returns number of bits preloaded in the shift register
//
#define BIORD_TELL_SHIFT_REGISTER_BITS() (BIO_FLUSH_BITS + (uxint) _BitIo_iBitsAvailable)
#define BIORD_TELL_SHIFT_REGISTER_BITS_STATE(state) (BIO_FLUSH_BITS + (uxint) (state).m_iBitsAvailable)
    
//
// returns pointer to first byte containing data that is not loaded into shift register yet
//
#define BIORD_TELL_OUTPUT_PTR() ((UInt8 *) _BitIo_pBitStreamPtr)
#define BIORD_TELL_OUTPUT_PTR_STATE(state) ((UInt8 *) (state).m_pBitStreamPtr)

//
// tell number of bits used in a buffer
//
#define BIORD_TELL_CONSUMED_BITS_STATE(state,ptr) \
    (((BIORD_TELL_OUTPUT_PTR_STATE (state) - (UInt8 *) (ptr)) << 3) - BIORD_TELL_SHIFT_REGISTER_BITS_STATE (state))


/* ----------------------------------------- Huffman code ------------------------------------------- */
/*                                           ------------                                             */


/*
    Huffman tree construction algorithm implemented further prefers leaves whenever possible, therefore it
    will create tallest tree with smallest leaf weights when leaf counts are (1, 1, 1, 3, 4, 7, 11, ...)

    Therefore, if we observed N symbols max codeword length will not exceed D bits where
             N          D
          439,203       26
          710,646       27
        4,870,846       31
        7,881,195       32
        ...             ..
    4,106,118,242       45 (2**32 = 4,294,967,296)
*/

#define HUFFMAN_CODEWORD_LENGTH_LIMIT   64      // guaranteed upper threshold on codeword length during tree construction


typedef UInt32 HUFFMAN_COUNT;                   // type to store frequency counts (>= 32 bits)
typedef UInt32 HUFFMAN_MASK;                    // type to store (codeword bit mask, codeword length in bits) pairs

#define HUFFMAN_CODEWORD_LENGTH_BITS     5      // number of bits needed to record HUFFMAN_MAX_CODEWORD_LENGTH value
#define HUFFMAN_MAX_CODEWORD_LENGTH     27      // max codeword length for 32- and 64-bit platoforms

#if HUFFMAN_CODEWORD_LENGTH_BITS + HUFFMAN_MAX_CODEWORD_LENGTH > 32
#error HUFFMAN_CODEWORD_LENGTH_BITS + HUFFMAN_MAX_CODEWORD_LENGTH > 32
#endif /* HUFFMAN_CODEWORD_LENGTH_BITS + HUFFMAN_MAX_CODEWORD_LENGTH > 32 */

#define HUFFMAN_GET_CODEWORD_VALUE(x)   ((x) >> HUFFMAN_CODEWORD_LENGTH_BITS)
#define HUFFMAN_GET_CODEWORD_LENGTH(x)  ((x) & ((1 << HUFFMAN_CODEWORD_LENGTH_BITS) - 1))

typedef struct HUFFMAN_NODE_T HUFFMAN_NODE;
struct HUFFMAN_NODE_T
{
    HUFFMAN_NODE   *m_pSon[2];      // ptr to left and right child
    HUFFMAN_NODE   *m_pNext;        // next node in ascending order by m_uCount
    HUFFMAN_COUNT   m_uCount;       // symbol's occurance count
    UInt16          m_uSymbol;      // symbol
    UInt16          m_uBits;        // number of bits per symbol
};


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
#pragma warning (disable: 4820)     // 'HUFFMAN_DECODE_NODE_T' : '4' bytes padding added after data member 'm_uBits'
typedef struct HUFFMAN_DECODE_NODE_T HUFFMAN_DECODE_NODE;
struct HUFFMAN_DECODE_NODE_T
{
    HUFFMAN_DECODE_NODE *m_pNext;
    UInt16  m_uSymbol;
    UInt16  m_uBits;
};
#pragma warning (pop)

typedef Int16 HUFFMAN_DECODE_TABLE_ENTRY;


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


#define HUFFMAN_ENCODED_TABLE_FILL                  (HUFFMAN_MAX_CODEWORD_LENGTH + 1)   // length[i] == 0 up to uFillBoundary bounds
#define HUFFMAN_ENCODED_TABLE_ZERO_REPT             (HUFFMAN_MAX_CODEWORD_LENGTH + 2)   // length[i] == 0 k times
#define HUFFMAN_ENCODED_TABLE_PREV                  (HUFFMAN_MAX_CODEWORD_LENGTH + 3)   // length[i] == length[i-1]
#define HUFFMAN_ENCODED_TABLE_ROW_0                 (HUFFMAN_MAX_CODEWORD_LENGTH + 4)   // length[i] == length[i-uFillBoundary]
#define HUFFMAN_ENCODED_TABLE_ROW_1                 (HUFFMAN_MAX_CODEWORD_LENGTH + 5)   // length[i] == length[i-uFillBoundary] + 1
#define HUFFMAN_ENCODED_TABLE_SIZE                  (HUFFMAN_MAX_CODEWORD_LENGTH + 6)
#define HUFFMAN_ENCODED_TABLE_ZERO_REPT_MIN_COUNT   5
#define HUFFMAN_ENCODED_TABLE_FILL_BOUNDARY         16


//
// Create huffman tables and encode them into output bitstream
//
XPRESS9_INTERNAL
void
Xpress9HuffmanCreateAndEncodeTable (
    XPRESS9_STATUS *pStatus,
    BIO_STATE      *pBioState,          // out bitstream I/O state
    const HUFFMAN_COUNT *puCount,       // IN:  uCount[i] = occurance count of i-th symbol
    uxint           uAlphabetSize,      // IN:  number of symbols in the alphabet
    uxint           uMaxCodewordLength, // IN:  max codeword length (inclusively)
    HUFFMAN_NODE   *pTemp,              // OUT: scratch area (ALPHABET*2)
    HUFFMAN_MASK   *puMask,             // OUT: encoded codeword + # of bits (least significant HUFFMAN_CODEWORD_LENGTH_BITS)
    uxint          *puCost,             // OUT  (optional, may be NULL): cost of encoding
    uxint           uFillBoundary       // IN:  fill boundary
);


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
);



/* -------------------------------------------- Misc ------------------------------------------ */
/*                                              ----                                            */

#define MEMORY_ALIGNMENT   256      // preferred memory alignment

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

#endif /* XPRESS9_HAVE_BARREL_SHIFTER */


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


/* ------------------------------------------- LZ 77 --------------------------------------- */
/*                                             -----                                         */

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


#define LZ77_ENCODER_STATE_READY                0   // ready to receive new data
#define LZ77_ENCODER_STATE_ENCODING             1   // encoding
#define LZ77_ENCODER_STATE_FLUSHING             2   // flushing


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
    UInt8          *m_pIrBuffer;            // buffer for intermediate representation
    uxint           m_uIrBufferSize;        // size of the buffer containing intermediate data
    UInt8          *m_pIrPtr;               // output pointer (m_pIrBuffer <= m_pIrPtr <= m_pIrBuffer + m_uIrBufferSize)
    const UInt8    *m_pIrSrc;               // used by 2nd pass
    LZ77_MTF_STATE  m_Mtf;                  // starting MTF state
};

struct LZ77_PASS2_STATE_T
{
    HUFFMAN_MASK    m_uShortSymbolMask[LZ77_SHORT_SYMBOL_ALPHABET_SIZE];

    HUFFMAN_MASK    m_uLongLengthMask[LZ77_LONG_LENGTH_ALPHABET_SIZE];

    LZ77_IR_STATE   m_Ir;

    uxint           m_uComputedEncodedSizeBits; // expected number of output bytes
    uxint           m_uActualEncodedSizeBits;   // actual number of bits written out

    uxint           m_uBytesCopiedFromScratch;  // number of bytes copied from scratch area into user buffer

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
#pragma warning (disable: 4820) // 'LZ77_PASS1_STATE_T' : '4' bytes padding added after data member 'm_HuffmanStat'
struct LZ77_PASS1_STATE_T
{
    LZ77_INDEX      m_uNext[8];

    struct
    {
        HUFFMAN_NODE    m_HuffmanNodeTemp[2 * MAX (LZ77_SHORT_SYMBOL_ALPHABET_SIZE, LZ77_LONG_LENGTH_ALPHABET_SIZE)];
    
        UInt8          *m_pScratchArea;         // scratch area used to handle I/O buffer boundaries efficiently
        uxint           m_uScratchAreaSize;
    } m_Scratch;

    LZ77_PASS2_STATE m_Pass2;

    struct
    {
        LZ77_INDEX     *m_pHashTable;               // pointer to the hash table
        uxint           m_uHashTableSizeMax;        // max size of the hash table (must be exact power of 2)
        uxint           m_uHashTableSizeCurrent;    // current size of the hash table (must be exact power of 2)
    } m_HashTable;

    UInt8          *m_pAllocatedMemory;     // pointer to the beginning of allocated memory block
    uxint           m_uMagic;               // magic
    uxint           m_uState;               // one of states
    uxint           m_uMaxWindowSizeLog2;   // log2 of max window size
    uxint           m_uRuntimeFlags;        // combination of Xpress9Flag_* flags

    struct
    {
        UInt8          *m_pBufferData;          // internal data buffer ( allocated on CompressionWriter initialization )
        uxint           m_uBufferDataSize;      // size of internal data buffer (must be exact power of 2)
    } m_BufferData;

    struct
    {
        const UInt8    *m_pUserData;            // user data buffer ( holds the pointer to the original data the user supplied )
        uxint           m_uUserDataSize;        // amount of data available in the user buffer ( the original data supplied )
        uxint           m_uUserDataProcessed;   // amount of data processed in the user buffer
        uxint           m_fFlush;               // user requested flush of internal buffers
    } m_UserData;

    struct
    {
        UInt8          *m_pData;                // current data buffer (m_pDataBuffer most of the time)
        uxint           m_uDataSize;            // amount of data available in data buffer
        uxint           m_uWindowSize;          // size of LZ77 window
        uxint           m_uEncodePosition;      // encode position
        uxint           m_uHashInsertPosition;  // next hash insertion position
        LZ77_MTF_STATE  m_Mtf;                  // current MTF state
    } m_EncodeData;

    struct
    {
        Lz77EncPass1_Proc      *m_pLz77EncPass1;    // pass 1 encoder
        Lz77EncPass2_Proc      *m_pLz77EncPass2;    // pass 2 encoder
        Lz77EncInsert_Proc     *m_pLz77EncInsert;   // hash insertion procedure
        uxint                   m_fDirty;
        XPRESS9_ENCODER_PARAMS  m_Delayed;      // parameters that couldn't be reset now and are delayed
        XPRESS9_ENCODER_PARAMS  m_Current;      // current set of parameters
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
#endif /* XPRESS9_DEBUG_PERF_STAT */

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
        
        HUFFMAN_COUNT   m_uShortSymbolCount[LZ77_SHORT_SYMBOL_ALPHABET_SIZE]; // number of short symbols for each symbol type.

        MSB_T           m_uStoredBitCount;      // count of bits that will be stored bypassing Huffman encoding
    } m_HuffmanStat;
};
#pragma warning (pop)

#define STATE (pState[-1])



/* ------------------------------------- Block header ------------------------------------- */
/*                                       ------------                                       */

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

//
// Flags format:
//     13 bits      length of encoded huffman tables in bits
//      3 bits      log2 of window size (16 .. 23)
//      2 bits      number of MTF entries   (0, 2, 4, reserved)
//      1 bit       min ptr match length (3 or 4)
//      1 bit       min mtf match length (2 or 3, shall be 0 if number of MTF entries is set to 0)
//
//     12 bits      reserved, must be 0
//

/* ---------------------------------------- Decoder ---------------------------------------- */
/*                                          -------                                          */


#define XPRESS9_DECODER_MAGIC 'd9xp'


#define LZ77_SHORT_SYMBOL_DECODE_ROOT_BITS          12
#define LZ77_SHORT_SYMBOL_DECODE_TAIL_BITS          6
#define LZ77_SHORT_SYMBOL_DECODE_TAIL_TABLE_COUNT   64

#define LZ77_LONG_LENGTH_DECODE_ROOT_BITS           12
#define LZ77_LONG_LENGTH_DECODE_TAIL_BITS           6
#define LZ77_LONG_LENGTH_DECODE_TAIL_TABLE_COUNT    64

#define LZ77_DECODER_SCRATCH_AREA_SIZE              (8*1024)


#define LZ77_DECODER_STATE_READY                0   // ready to start new session
#define LZ77_DECODER_STATE_WAIT_SESSION         1   // waiting for new session
#define LZ77_DECODER_STATE_WAIT_BLOCK           2   // waiting for new block
#define LZ77_DECODER_STATE_WAIT_HUFFMAN         3   // started decoding, waiting for huffman tables
#define LZ77_DECODER_STATE_DECODING             4   // in process of decoding a new block

typedef struct LZ77_DECODER_T LZ77_DECODER;

typedef
void
Xpress9Lz77DecProc (
    XPRESS9_STATUS  *pStatus,
    LZ77_DECODER    *pDecoder
);


#pragma warning (push)
#pragma warning (disable: 4820)     // 'LZ77_DECODER_T' : '4' bytes padding added after data member 'm_uRuntimeFlags'
struct LZ77_DECODER_T
{
    uxint           m_uMagic;               // magic
    uxint           m_uState;               // one of states
    UInt8          *m_pAllocatedMemory;     // pointer to the beginning of allocated memory block
    uxint           m_uMaxWindowSizeLog2;   // log2 of max window size
    uxint           m_uRuntimeFlags;        // combination of Xpress9Flag_* flags

    struct
    {
        UInt64          m_uBufferOffset;        // beginning of offset of the buffer in current session; decode position = BufferOffset + DecodePosition

        uxint           m_uDecodePosition;      // decode position in data buffer -- the location at which the next decoded bit will be written.
        uxint           m_uStopPosition;        // position where to stop
        uxint           m_uEndOfBuffer;         // end of buffer position
        uxint           m_uCopyPosition;        // offset in the internal buffer up to which everything was copied to user -- the location from which the next decoded bit will be copied to user buffers.

        uxint           m_uDecodedSizeBits;     // number of bytes that were decoded from current data block
        uxint           m_uEncodedSizeBits;     // total number of bits in compressed data block

        uxint           m_uBlockDecodedBytes;   // number of bytes decoded in current block
        uxint           m_uBlockCopiedBytes;    // number of bytes copied to user buffers in current block

        uxint           m_uScratchBytesStored;      // number of bytes stored in scratch area
        uxint           m_uScratchBytesProcessed;   // number of bytes in scratch area that were already processed

        uxint           m_uWindowSizeLog2;      // log2 of LZ77 window size
        uxint           m_uHuffmanTableBits;    // number of bits in encoded Huffman tables
        uxint           m_uMtfEntryCount;       // MTF tentry count
        uxint           m_uPtrMinMatchLength;   // min ptr match length
        uxint           m_uMtfMinMatchLength;   // min MTF entry length

        Xpress9Lz77DecProc *m_pLz77DecProc;     // decoder proc

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
        UInt8          *m_pBufferData;          // internal data buffer
        uxint           m_uBufferDataSize;      // size of internal data buffer
    } m_BufferData;

    struct
    {
        const UInt8    *m_pUserData;            // user data buffer
        uxint           m_uUserDataSize;        // amount of data available in the user buffer
        uxint           m_uUserDataRead;        // amount of data read the user buffer
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
#endif /* XPRESS9_DEBUG_PERF_STAT */

    struct
    {
        LZ77_STAT   m_Encoder;
        LZ77_STAT   m_Session;
        LZ77_STAT   m_Block;
        UInt32      m_uSessionSignature;
        UInt32      m_uBlockIndex;
    } m_Stat;


    //
    // large fields that do not require initialization shall follow m_Huffman
    //
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


#endif /* _XPRESS9INTERNAL_H_ */
