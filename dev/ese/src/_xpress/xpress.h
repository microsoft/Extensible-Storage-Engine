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




#define XPRESS_MAX_BLOCK_LOG    16
#define XPRESS_MAX_BLOCK        (1 << XPRESS_MAX_BLOCK_LOG)


#define XPRESS_ALIGNMENT        8

#if !defined (UNIX) && !defined (XPRESS_CALL)
#define XPRESS_CALL __stdcall
#endif

#if !defined (XPRESS_EXPORT)
#define XPRESS_EXPORT
#endif


typedef
void *
XPRESS_CALL
XpressAllocFn (
  void *Context,
  int   AllocSize
);

typedef
void
XPRESS_CALL
XpressFreeFn (
  void *Context,
  void *Address
);





typedef struct XpressEncodeDummyStruct {int XpressEncodeDummy;} *XpressEncodeStream;

XPRESS_EXPORT
XpressEncodeStream
XPRESS_CALL
XpressEncodeCreate (
  int            MaxOrigSize,
  void          *Context,
  XpressAllocFn *AllocFn,
  int            CompressionLevel
);


typedef
void
XPRESS_CALL
XpressProgressFn (
  void *Context,
  int   EncodedSize
);


XPRESS_EXPORT
int
XPRESS_CALL
XpressEncode (
  __in_opt XpressEncodeStream EncodeStream,
  __in_bcount(CompSize) void              *CompAdr,
  int                CompSize,
  __in_bcount(OrigSize) const void        *OrigAdr,
  int                OrigSize,
  XpressProgressFn  *ProgressFn,
  void              *ProgressContext,
  int                ProgressSize
);

XPRESS_EXPORT
int
XPRESS_CALL
XpressEncodeEx (
  __in_opt XpressEncodeStream EncodeStream,
  __in_bcount(CompSize) void              *CompAdr,
  int                CompSize,
  __in_bcount(OrigSize) const void        *OrigAdr,
  int                OrigSize,
  XpressProgressFn  *ProgressFn,
  void              *ProgressContext,
  int                ProgressSize,
  int                CompressionLevel
);

XPRESS_EXPORT
int
XPRESS_CALL
XpressEncodeEx2 (
  __in_opt XpressEncodeStream EncodeStream,
  __in_bcount(CompSize) void              *CompAdr,
  int                CompSize,
  __in_bcount(OrigSize) const void        *OrigAdr,
  int                OrigSize,
  __out_opt int               *pEncodedSize,
  XpressProgressFn  *ProgressFn,
  void              *ProgressContext,
  int                ProgressSize,
  int                CompressionLevel
);

XPRESS_EXPORT
int
XPRESS_CALL
XpressEncodeGetMaxCompressionLevel (
  __in_opt XpressEncodeStream EncodeStream
);


XPRESS_EXPORT
void
XPRESS_CALL
XpressEncodeClose (
  __inout_opt XpressEncodeStream EncodeStream,
  void              *Context,
  XpressFreeFn      *FreeFn
);





typedef struct XpressDecodeDummyStruct {int XpressDecodeDummy;} *XpressDecodeStream;

XPRESS_EXPORT
XpressDecodeStream
XPRESS_CALL
XpressDecodeCreate (
  void          *Context,
  XpressAllocFn *AllocFn
);

XPRESS_EXPORT
int
XPRESS_CALL
XpressDecode (
  XpressDecodeStream DecodeStream,
  __in_bcount(DecodeSize) void              *OrigAdr,
  int                OrigSize,
  int                DecodeSize,
  __in_ecount(CompSize) const void        *CompAdr,
  int                CompSize
);

XPRESS_EXPORT
void
XPRESS_CALL
XpressDecodeClose (
  XpressDecodeStream DecodeStream,
  void              *Context,
  XpressFreeFn      *FreeFn
);


#ifdef __cplusplus
};
#endif

#endif 
