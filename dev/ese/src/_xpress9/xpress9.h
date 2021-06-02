// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#ifdef _XPRESS9_H_
#error xpress9.h was included from two different directories
#else

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

#define _XPRESS9_H_

// declare default calling convention used in xpress
#if !defined (UNIX) && !defined (XPRESS9_CALL)
#define XPRESS9_CALL __stdcall
#endif /* !defined (UNIX) && !defined (XPRESS9_CALL) */

#if !defined (XPRESS9_EXPORT)
#define XPRESS9_EXPORT
#endif /* !defined (XPRESS9_EXPORT) */

#pragma pack (push, 8)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* ------------------------------- Memory allocation/deallocation -------------------------------- */
/*                                 ------------------------------                                  */

//
// use the same function as XPRESS8 to allow reuse of existing callbacks
//

#ifndef _XPRESS_H_

// user-supplied callback function that allocates memory
// if there is no memory available it shall return NULL
typedef
void *
XPRESS9_CALL
XpressAllocFn (
  void *Context,        // user-defined context (as passed to XpressEncodeCreate)
  int   AllocSize       // size of memory block to allocate (bytes)
);

// user-supplied callback function that releases memory
typedef
void
XPRESS9_CALL
XpressFreeFn (
  void *Context,        // user-defined context (as passed to XpressEncodeClose)
  void *Address         // pointer to the block to be freed
);


#endif /* _XPRESS_H_ */


/* ---------------------------------------- SSE support ----------------------------------------- */
/*                                          -----------                                           */


#define Xpress9Flag_UseSSE2     1       // IX86/AMD64/IA64: use SSE2    // (IsProcessorFeaturePresent (PF_XMMI_INSTRUCTIONS_AVAILABLE) && IsProcessorFeaturePresent (PF_XMMI64_INSTRUCTIONS_AVAILABLE))
#define Xpress9Flag_UseSSE4     2       // IX86/AMD64/IA64: use SSE4    // not available in current PSDK yet



/* ------------------------------------- Error information -------------------------------------- */
/*                                       -----------------                                        */


#define Xpress9Status_OK                        0       // everything is fine
#define Xpress9Status_NotEnoughMemory           1       // allocator returned 0 (only *Create() functions may return it)
#define Xpress9Status_BadArgument               2       // bad argument
#define Xpress9Status_CorruptedMemory           3       // memory corruption

// encoder-specific error codes
#define Xpress9Status_EncoderNotEmpty           4       // attempt to reset an encoder which has data in internal buffers
#define Xpress9Status_EncoderNotReady           5       // attempt to attach new buffers even though previously attached buffers were not fully processed
#define Xpress9Status_EncoderNotCompressing     6       // attempt to compress the data when encoder is not expecting it
#define Xpress9Status_EncoderNotFlushing        7       // attempt to flush data when encoder is not flushing it
#define Xpress9Status_EncoderCompressing        8       // attempt to detach buffers that were not fully processed yet

// decoder-specific error codes
#define Xpress9Status_DecoderCorruptedData      9       // compressed data is corrupted
#define Xpress9Status_DecoderCorruptedHeader   10       // header signature does not match expected one
#define Xpress9Status_DecoderWindowTooLarge    11       // window size exceed max window size supported by decoder
#define Xpress9Status_DecoderUnknownFormat     12       // decoder version does not support the format
#define Xpress9Status_DecoderNotDetached       13       // user buffers were not detached from the decoder
#define Xpress9Status_DecoderNotDrained        14       // user did not fetch all of encoded data

typedef struct XPRESS9_STATUS_T XPRESS9_STATUS;
struct XPRESS9_STATUS_T
{
    unsigned    m_uStatus;                  // one of Xpress9Status_* codes
    unsigned    m_uLineNumber;              // line number where the error was detected
    const char *m_pFilename;                // name of the file where the error was detected
    const char *m_pFunction;                // name of the function which detected the error
    char        m_ErrorDescription[1024];   // verbose description of error (primarily for debugging purposes)
};


const char *
Xpress9GetErrorText (
    unsigned uErrorCode
);


/* ------------------------------------------ Encoder ------------------------------------------- */
/*                                            -------                                             */


//
// min/max log2 of LZ77 window size (defines memory usage)
//
#define XPRESS9_WINDOW_SIZE_LOG2_MIN    16
#define XPRESS9_WINDOW_SIZE_LOG2_MAX    22


typedef const struct XPRESS9_ENCODER_T *XPRESS9_ENCODER;

typedef struct XPRESS9_ENCODER_PARAMS_T XPRESS9_ENCODER_PARAMS;

struct XPRESS9_ENCODER_PARAMS_T
{
    unsigned    m_cbSize;                   // size of the structure; must be set by the caller
    unsigned    m_uMaxStreamLength;         // the value of 0 means "no limit"
    unsigned    m_uMtfEntryCount;           // number of MTF entries (0, 2, or 4)
    unsigned    m_uLookupDepth;             // depth of lookup (0 .. 9)
    unsigned    m_uWindowSizeLog2;          // log2 of size of LZ77 window [XPRESS9_WINDOW_SIZE_LOG2_MIN..XPRESS9_WINDOW_SIZE_LOG2_MAX]
    unsigned    m_uOptimizationLevel;       // optimization level (0=no optimization, 1=greedy compression)
    unsigned    m_uPtrMinMatchLength;       // min match length for regular pointer (3 or 4)
    unsigned    m_uMtfMinMatchLength;       // min match length for MTF pointer (2..m_uMinMatch)
};

//
// Allocates memory, initializes, and returns the pointer to newly created encoder.
//
// It is recommended to create few encoders (1-2 encoders per CPU core) and reuse them:
// creation of new encoder is relatively expensive.
//
XPRESS9_EXPORT
XPRESS9_ENCODER
XPRESS9_CALL
Xpress9EncoderCreate (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    void                   *pAllocContext,      // IN:  context for user-defined memory allocator
    XpressAllocFn          *pAllocFn,           // IN:  user-defined memory allocator (it will be called by Xpress9EncodeCreate only)
    unsigned                uMaxWindowSizeLog2, // IN:  log2 of max size of LZ77 window [XPRESS9_WINDOW_SIZE_LOG2_MIN..XPRESS9_WINDOW_SIZE_LOG2_MAX]
    unsigned                uFlags              // IN:  one or more of Xpress9Flag_* flags
);


//
// Start new encoding session. if fForceReset is not set then encoder must be flushed.
//
XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9EncoderStartSession (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    XPRESS9_ENCODER         pEncoder,           // IN:  encoder
    const XPRESS9_ENCODER_PARAMS *pParams,      // IN:  parameter for this encoding session
    unsigned                fForceReset         // IN:  if TRUE then allows resetting of previous session even if it wasn't flushed
);

//
// Attach buffer with user data to the encoder
//
XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9EncoderAttach (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    XPRESS9_ENCODER         pEncoder,           // IN:  encoder
    const void             *pOrigData,          // IN:  original (uncompressed) data buffer (optinal, may be NULL)
    unsigned                uOrigDataSize,      // IN:  amount of data in "pData" buffer (optional, may be 0)
    unsigned                fFlush              // IN:  if TRUE then flush and compress data buffered internally
);


//
// User callback which gets called when encoder compresses uDataSize bytes stored in buffer at pData address
//
typedef
void
XPRESS9_CALL
Xpress9EncodeCallback (
    const void     *pData,
    size_t          uDataSize,
    void           *pCallbackContext
);


//
// Compress data in locked buffer. Returns number of compressed bytes that needs to be fetched by the caller.
// This function shall be called repeatedly until returned value is non-zero
//
XPRESS9_EXPORT
unsigned
XPRESS9_CALL
Xpress9EncoderCompress (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    XPRESS9_ENCODER         pEncoder,           // IN:  encoder    
    Xpress9EncodeCallback  *pCallback,          // IN:  encoder callback (optinal, may be NULL)
    void                   *pCallbackContext    // IN:  context for the callback
);

//
// Copies compressed data into user buffer and returns remaining number of bytes that should be fetched
//
// Buffer provided by the user is always filled, i.e. if return value is not zero then
// (*puCompBytesWritten) is equal to uCompDataBufferSize.
// 
//
XPRESS9_EXPORT
unsigned
XPRESS9_CALL
Xpress9EncoderFetchCompressedData (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    XPRESS9_ENCODER         pEncoder,           // IN:  encoder    
    void                   *pCompData,          // IN:  address of a buffer to receive compressed data (may be NULL)
    unsigned                uCompDataBufferSize,// IN:  size of pCompData buffer
    unsigned               *puCompBytesWritten  // OUT: actual number of bytes written to pCompData buffer
);


//
// Detach data buffer with user data that was attached to the encoder before
// Values of pOrigData and uOrigDataSize must match parameters passed to Xpress9EncoderDetach
//
XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9EncoderDetach (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    XPRESS9_ENCODER         pEncoder,           // IN:  encoder
    const void             *pOrigData,          // IN:  original (uncompressed) data buffer (optinal, may be NULL)
    unsigned                uOrigDataSize       // IN:  amount of data in "pData" buffer (optional, may be 0)
);


//
// Release memory occupied by the encoder.
//
// Please notice that the order to destroy the decoder will be always obeyed irrespective of encoder state
//
XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9EncoderDestroy (
    XPRESS9_STATUS     *pStatus,                // OUT: status
    XPRESS9_ENCODER     pEncoder,               // IN:  encoder
    void               *pFreeContext,           // IN:  context for user-defined memory release function
    XpressFreeFn       *pFreeFn                 // IN:  user-defined memory release function
);


/* ------------------------------------------ Decoder ------------------------------------------- */
/*                                            -------                                             */

typedef const struct XPRESS9_DECODER_T *XPRESS9_DECODER;

//
// Allocates memory, initializes, and returns the pointer to newly created decoder.
//
// It is recommended to create few decoders (1-2 decoders per CPU core) and reuse them:
// creation of new decoder is relatively expensive.
//
// Each instance of encoder will use about 64KB + 9 * 2**uMaxWindowSizeLog2 bytes; e.g.
// encoder with 1MB max window size (uMaxWindowSizeLog2 equal to 20) will use about 9MB + 64KB
//
XPRESS9_EXPORT
XPRESS9_DECODER
XPRESS9_CALL
Xpress9DecoderCreate (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    void                   *pAllocContext,      // IN:  context for user-defined memory allocator
    XpressAllocFn          *pAllocFn,           // IN:  user-defined memory allocator (it will be called by Xpress9EncodeCreate only)
    unsigned                uMaxWindowSizeLog2, // IN:  log2 of max size of LZ77 window [XPRESS9_WINDOW_SIZE_LOG2_MIN..XPRESS9_WINDOW_SIZE_LOG2_MAX]
    unsigned                uFlags              // IN:  one or more of Xpress9Flag_* flags
);


//
// Start new encoding session. if fForceReset is not set then decoder must be flushed.
//
XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9DecoderStartSession (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    XPRESS9_DECODER         pDecoder,           // IN:  decoder
    unsigned                fForceReset         // IN:  if TRUE then allows resetting of previous session even if it wasn't flushed
);


//
// Attach next buffer with compressed data to the decoder
//
XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9DecoderAttach (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    XPRESS9_DECODER         pDecoder,           // IN:  decoder
    const void             *pCompData,          // IN:  buffer containing compressed data
    unsigned                uCompDataSize       // IN:  amount of data in compressed buffer
);


//
// Fetch decompressed data filling pOrigData buffer with up to uOrigDataSize bytes.
//
// Returns number of decompressed bytes that are available but did not fit into pOrigData buffer
//
XPRESS9_EXPORT
unsigned
XPRESS9_CALL
Xpress9DecoderFetchDecompressedData (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    XPRESS9_DECODER         pDecoder,           // IN:  decoder
    void                   *pOrigData,          // IN:  buffer for storing uncompressed data (may be NULL)
    unsigned                uOrigDataSize,      // IN:  size of pOrigDataBuffer (may be 0)
    unsigned               *puOrigDataWritten,  // OUT: number of bytes that were actually written into uOrigDataSize buffer
    unsigned               *puCompDataNeeded    // OUT: how much compressed bytes needed
);


//
// Detaches the buffer with compressed data
//
XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9DecoderDetach (
    XPRESS9_STATUS         *pStatus,            // OUT: status
    XPRESS9_DECODER         pDecoder,           // IN:  decoder
    const void             *pCompData,          // IN:  buffer containing compressed data
    unsigned                uCompDataSize       // IN:  amount of data in compressed buffer
);


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
    XPRESS9_DECODER     pDecoder,               // IN:  decoder
    void               *pFreeContext,           // IN:  context for user-defined memory release function
    XpressFreeFn       *pFreeFn                 // IN:  user-defined memory release function
);


//
// Get access to xpress9's internal memory buffers
// Used to share memory with the lzma decoder
//
XPRESS9_EXPORT
unsigned
XPRESS9_CALL
Xpress9DecoderQueryBuffer(
    XPRESS9_STATUS     *pStatus,                // OUT: status
    XPRESS9_DECODER     pXpress9Decoder,        // IN:  decoder
    void              **pBuffer,                // OUT
    size_t             *pBufferSize             // OUT
);


//
// Set xpress9's internal lz77 window to another memory location
// Used by Cosmos to decode directly into the client buffer
// pBuffer needs to have 256 bytes allocated beyond uBufferSize
//
XPRESS9_EXPORT
unsigned
XPRESS9_CALL
Xpress9DecoderSetBuffer(
    XPRESS9_STATUS     *pStatus,                // OUT: status
    XPRESS9_DECODER     pXpress9Decoder,        // IN:  decoder
    void              * pBuffer,                // IN
    size_t              uBufferSize             // IN
);

#ifdef __cplusplus
}; // extern "C"
#endif /* __cplusplus */

#pragma pack (pop)

#endif /* _XPRESS9_H_ */
