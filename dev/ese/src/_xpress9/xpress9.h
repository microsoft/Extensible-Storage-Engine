// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#ifdef _XPRESS9_H_
#error xpress9.h was included from two different directories
#else

#ifdef _MSC_VER
#pragma once
#endif 

#define _XPRESS9_H_

#if !defined (UNIX) && !defined (XPRESS9_CALL)
#define XPRESS9_CALL __stdcall
#endif 

#if !defined (XPRESS9_EXPORT)
#define XPRESS9_EXPORT
#endif 

#pragma pack (push, 8)

#ifdef __cplusplus
extern "C" {
#endif 







#ifndef _XPRESS_H_

typedef
void *
XPRESS9_CALL
XpressAllocFn (
  void *Context,
  int   AllocSize
);

typedef
void
XPRESS9_CALL
XpressFreeFn (
  void *Context,
  void *Address
);


#endif 






#define Xpress9Flag_UseSSE2     1       // IX86/AMD64/IA64: use SSE2
#define Xpress9Flag_UseSSE4     2       // IX86/AMD64/IA64: use SSE4







#define Xpress9Status_OK                        0
#define Xpress9Status_NotEnoughMemory           1
#define Xpress9Status_BadArgument               2
#define Xpress9Status_CorruptedMemory           3

#define Xpress9Status_EncoderNotEmpty           4
#define Xpress9Status_EncoderNotReady           5
#define Xpress9Status_EncoderNotCompressing     6
#define Xpress9Status_EncoderNotFlushing        7
#define Xpress9Status_EncoderCompressing        8

#define Xpress9Status_DecoderCorruptedData      9
#define Xpress9Status_DecoderCorruptedHeader   10
#define Xpress9Status_DecoderWindowTooLarge    11
#define Xpress9Status_DecoderUnknownFormat     12
#define Xpress9Status_DecoderNotDetached       13
#define Xpress9Status_DecoderNotDrained        14

typedef struct XPRESS9_STATUS_T XPRESS9_STATUS;
struct XPRESS9_STATUS_T
{
    unsigned    m_uStatus;
    unsigned    m_uLineNumber;
    const char *m_pFilename;
    const char *m_pFunction;
    char        m_ErrorDescription[1024];
};


const char *
Xpress9GetErrorText (
    unsigned uErrorCode
);






#define XPRESS9_WINDOW_SIZE_LOG2_MIN    16
#define XPRESS9_WINDOW_SIZE_LOG2_MAX    22


typedef const struct XPRESS9_ENCODER_T *XPRESS9_ENCODER;

typedef struct XPRESS9_ENCODER_PARAMS_T XPRESS9_ENCODER_PARAMS;

struct XPRESS9_ENCODER_PARAMS_T
{
    unsigned    m_cbSize;
    unsigned    m_uMaxStreamLength;
    unsigned    m_uMtfEntryCount;
    unsigned    m_uLookupDepth;
    unsigned    m_uWindowSizeLog2;
    unsigned    m_uOptimizationLevel;
    unsigned    m_uPtrMinMatchLength;
    unsigned    m_uMtfMinMatchLength;
};

XPRESS9_EXPORT
XPRESS9_ENCODER
XPRESS9_CALL
Xpress9EncoderCreate (
    XPRESS9_STATUS         *pStatus,
    void                   *pAllocContext,
    XpressAllocFn          *pAllocFn,
    unsigned                uMaxWindowSizeLog2,
    unsigned                uFlags
);


XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9EncoderStartSession (
    XPRESS9_STATUS         *pStatus,
    XPRESS9_ENCODER         pEncoder,
    const XPRESS9_ENCODER_PARAMS *pParams,
    unsigned                fForceReset
);

XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9EncoderAttach (
    XPRESS9_STATUS         *pStatus,
    XPRESS9_ENCODER         pEncoder,
    const void             *pOrigData,
    unsigned                uOrigDataSize,
    unsigned                fFlush
);


typedef
void
XPRESS9_CALL
Xpress9EncodeCallback (
    const void     *pData,
    size_t          uDataSize,
    void           *pCallbackContext
);


XPRESS9_EXPORT
unsigned
XPRESS9_CALL
Xpress9EncoderCompress (
    XPRESS9_STATUS         *pStatus,
    XPRESS9_ENCODER         pEncoder,
    Xpress9EncodeCallback  *pCallback,
    void                   *pCallbackContext
);

XPRESS9_EXPORT
unsigned
XPRESS9_CALL
Xpress9EncoderFetchCompressedData (
    XPRESS9_STATUS         *pStatus,
    XPRESS9_ENCODER         pEncoder,
    void                   *pCompData,
    unsigned                uCompDataBufferSize,
    unsigned               *puCompBytesWritten
);


XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9EncoderDetach (
    XPRESS9_STATUS         *pStatus,
    XPRESS9_ENCODER         pEncoder,
    const void             *pOrigData,
    unsigned                uOrigDataSize
);


XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9EncoderDestroy (
    XPRESS9_STATUS     *pStatus,
    XPRESS9_ENCODER     pEncoder,
    void               *pFreeContext,
    XpressFreeFn       *pFreeFn
);





typedef const struct XPRESS9_DECODER_T *XPRESS9_DECODER;

XPRESS9_EXPORT
XPRESS9_DECODER
XPRESS9_CALL
Xpress9DecoderCreate (
    XPRESS9_STATUS         *pStatus,
    void                   *pAllocContext,
    XpressAllocFn          *pAllocFn,
    unsigned                uMaxWindowSizeLog2,
    unsigned                uFlags
);


XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9DecoderStartSession (
    XPRESS9_STATUS         *pStatus,
    XPRESS9_DECODER         pDecoder,
    unsigned                fForceReset
);


XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9DecoderAttach (
    XPRESS9_STATUS         *pStatus,
    XPRESS9_DECODER         pDecoder,
    const void             *pCompData,
    unsigned                uCompDataSize
);


XPRESS9_EXPORT
unsigned
XPRESS9_CALL
Xpress9DecoderFetchDecompressedData (
    XPRESS9_STATUS         *pStatus,
    XPRESS9_DECODER         pDecoder,
    void                   *pOrigData,
    unsigned                uOrigDataSize,
    unsigned               *puOrigDataWritten,
    unsigned               *puCompDataNeeded
);


XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9DecoderDetach (
    XPRESS9_STATUS         *pStatus,
    XPRESS9_DECODER         pDecoder,
    const void             *pCompData,
    unsigned                uCompDataSize
);


XPRESS9_EXPORT
void
XPRESS9_CALL
Xpress9DecoderDestroy (
    XPRESS9_STATUS     *pStatus,
    XPRESS9_DECODER     pDecoder,
    void               *pFreeContext,
    XpressFreeFn       *pFreeFn
);


XPRESS9_EXPORT
unsigned
XPRESS9_CALL
Xpress9DecoderQueryBuffer(
    XPRESS9_STATUS     *pStatus,
    XPRESS9_DECODER     pXpress9Decoder,
    void              **pBuffer,
    size_t             *pBufferSize
);


XPRESS9_EXPORT
unsigned
XPRESS9_CALL
Xpress9DecoderSetBuffer(
    XPRESS9_STATUS     *pStatus,
    XPRESS9_DECODER     pXpress9Decoder,
    void              * pBuffer,
    size_t              uBufferSize
);

#ifdef __cplusplus
};
#endif 

#pragma pack (pop)

#endif 
