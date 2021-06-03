// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _XPRESS_H_
#define _XPRESS_H_

#ifdef _MSC_VER
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------- Common declarations ------------------------- */
/*                        -------------------                           */

// max. size of input block
#define XPRESS_MAX_BLOCK_LOG    16
#define XPRESS_MAX_BLOCK        (1 << XPRESS_MAX_BLOCK_LOG)


// preferred data alignment to avoid misaligned accesses
#define XPRESS_ALIGNMENT        8

// declare default calling convention used in xpress
#if !defined (UNIX) && !defined (XPRESS_CALL)
#define XPRESS_CALL __stdcall
#endif

#if !defined (XPRESS_EXPORT)
#define XPRESS_EXPORT
#endif


// user-supplied callback function that allocates memory
// if there is no memory available it shall return NULL
typedef
void *
XPRESS_CALL
XpressAllocFn (
  void *Context,        // user-defined context (as passed to XpressEncodeCreate)
  int   AllocSize       // size of memory block to allocate (bytes)
);

// user-supplied callback function that releases memory
typedef
void
XPRESS_CALL
XpressFreeFn (
  void *Context,        // user-defined context (as passed to XpressEncodeClose)
  void *Address         // pointer to the block to be freed
);


/* ----------------------------- Encoder ------------------------------ */
/*                               -------                                */

// declare unique anonymous types for type safety
typedef struct XpressEncodeDummyStruct {int XpressEncodeDummy;} *XpressEncodeStream;

// allocate and initialize encoder's data structures
// returns NULL if callback returned NULL (not enough memory)
XPRESS_EXPORT
XpressEncodeStream
XPRESS_CALL
XpressEncodeCreate (
  int            MaxOrigSize,           // max size of original data block (bytes)
  void          *Context,               // user-defined context info (will  be passed to AllocFn)
  XpressAllocFn *AllocFn,               // memory allocation callback
  int            CompressionLevel       // use 0 for speed, 9 for quality
);


// callback function called by XpressEncode to indicate compression progress
typedef
void
XPRESS_CALL
XpressProgressFn (
  void *Context,                        // user-defined context
  int   EncodedSize                     // size of processed original data (bytes)
);


// returns size of compressed data
// if compression failed then compressed buffer is left as is, and
// original data should be saved instead
XPRESS_EXPORT
int
XPRESS_CALL
XpressEncode (
  __in_opt XpressEncodeStream EncodeStream,      // encoder's workspace
  __in_bcount(CompSize) void              *CompAdr,           // address of beggining of output memory region
  int                CompSize,          // size of output memory region (bytes)
  __in_bcount(OrigSize) const void        *OrigAdr,           // address of beggining of input data block
  int                OrigSize,          // input data block size (bytes)
  XpressProgressFn  *ProgressFn,        // NULL or progress callback
  void              *ProgressContext,   // user-defined context that will be passed to ProgressFn
  int                ProgressSize       // call ProgressFn each time ProgressSize bytes processed
);

// returns size of compressed data
// if compression failed then compressed buffer is left as is, and
// original data should be saved instead
XPRESS_EXPORT
int
XPRESS_CALL
XpressEncodeEx (
  __in_opt XpressEncodeStream EncodeStream,      // encoder's workspace
  __in_bcount(CompSize) void              *CompAdr,           // address of beggining of output memory region
  int                CompSize,          // size of output memory region (bytes)
  __in_bcount(OrigSize) const void        *OrigAdr,           // address of beggining of input data block
  int                OrigSize,          // input data block size (bytes)
  XpressProgressFn  *ProgressFn,        // NULL or progress callback
  void              *ProgressContext,   // user-defined context that will be passed to ProgressFn
  int                ProgressSize,      // call ProgressFn each time ProgressSize bytes processed
  int                CompressionLevel   // CompressionLevel should not exceed MaxCompressionLevel
);

//
// returns size of compressed data _and_ number of bytes actually encoded (may be less that OrigSize)
// if size of compressed data exceeds size of original, size of original data is returned and in this
// case contents of (*CompAdr) buffer is undefined; caller shall copy (*OrigAdr) to output itself
// NB: XpressEncodeEx2 is available only if CODING_ALG=1 (i.e. CODING_DIRECT2 encoding is used)
//
XPRESS_EXPORT
int
XPRESS_CALL
XpressEncodeEx2 (
  __in_opt XpressEncodeStream EncodeStream,      // encoder's workspace
  __in_bcount(CompSize) void              *CompAdr,           // address of beggining of output memory region
  int                CompSize,          // size of output memory region (bytes)
  __in_bcount(OrigSize) const void        *OrigAdr,           // address of beggining of input data block
  int                OrigSize,          // input data block size (bytes)
  __out_opt int               *pEncodedSize,      // ptr to location which receives actual # of encoded bytes
  XpressProgressFn  *ProgressFn,        // NULL or progress callback
  void              *ProgressContext,   // user-defined context that will be passed to ProgressFn
  int                ProgressSize,      // call ProgressFn each time ProgressSize bytes processed
  int                CompressionLevel   // CompressionLevel should not exceed MaxCompressionLevel
);

// returns MaxCompressionLevel or (-1) if stream was not initialized properly
XPRESS_EXPORT
int
XPRESS_CALL
XpressEncodeGetMaxCompressionLevel (
  __in_opt XpressEncodeStream EncodeStream       // encoder's workspace
);


// invalidate encoding stream and release workspace memory
XPRESS_EXPORT
void
XPRESS_CALL
XpressEncodeClose (
  __inout_opt XpressEncodeStream EncodeStream,      // encoder's workspace
  void              *Context,           // user-defined context for FreeFn
  XpressFreeFn      *FreeFn             // memory release callback
);


/* ----------------------------- Decoder ------------------------------ */
/*                               -------                                */

// declare unique anonymous types for type safety
typedef struct XpressDecodeDummyStruct {int XpressDecodeDummy;} *XpressDecodeStream;

// allocate memory for decoder. Returns NULL if not enough memory.
XPRESS_EXPORT
XpressDecodeStream
XPRESS_CALL
XpressDecodeCreate (
  void          *Context,               // user-defined context info (will  be passed to AllocFn)
  XpressAllocFn *AllocFn                // memory allocation callback
);

// decode compressed block. Returns # of decoded bytes or -1 otherwise
XPRESS_EXPORT
int
XPRESS_CALL
XpressDecode (
  XpressDecodeStream DecodeStream,      // decoder's workspace
  __in_bcount(DecodeSize) void              *OrigAdr,           // address of beginning out output memory region
  int                OrigSize,          // size of output memory region (bytes)
  int                DecodeSize,        // # of bytes to decode ( <= OrigSize)
  __in_ecount(CompSize) const void        *CompAdr,           // address of beginning of compressed data block
  int                CompSize           // size of compressed data block (bytes)
);

// invalidate decoding stream and release workspace memory
XPRESS_EXPORT
void
XPRESS_CALL
XpressDecodeClose (
  XpressDecodeStream DecodeStream,      // encoder's workspace
  void              *Context,           // user-defined context info (will  be passed to FreeFn)
  XpressFreeFn      *FreeFn             // callback that releases the memory
);


#ifdef __cplusplus
};
#endif

#endif /* _XPRESS_H_ */
