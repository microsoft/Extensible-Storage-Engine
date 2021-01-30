// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _XPRS_H_
#define _XPRS_H_

#pragma warning (disable: 4820)

#pragma warning (push)
#pragma warning (disable: 4619)
#pragma warning (disable: 4255)

#ifndef DEBUG
#define DEBUG 0
#endif 

#include <stdlib.h>

#if DEBUG
#include <stdio.h>
#include <string.h>
#include <assert.h>
#else
#undef assert
#define assert(cond)
#endif


#pragma warning (pop)

#include "xpress.h"

#ifdef _MSC_VER

#ifdef _NTSYSTEM_
#pragma code_seg ("PAGELK")
#endif 

#endif 






#ifndef CODING_ALG
#define CODING_ALG      1
#endif

#define CODING_DIRECT2  (1 << 1)
#define CODING_DIRECT   (1 << 2)
#define CODING_BY_BIT   (1 << 3)
#define CODING_HUFF_LEN (1 << 4)
#define CODING_HUFF_PTR (1 << 5)
#define CODING_HUFF_ALL (1 << 6)

#define CODING          (1 << CODING_ALG)

#define SUPPORT_CRC     0

#define BUFF_SIZE_LOG    XPRESS_MAX_BLOCK_LOG
#define BUFF_SIZE        (1<<BUFF_SIZE_LOG)

#if 1
#define MAX_OFFSET      (BUFF_SIZE_LOG > 16 ? 16 : BUFF_SIZE_LOG)
#else
#define MAX_OFFSET      13
#endif

#if CODING == CODING_DIRECT2 && MAX_OFFSET > 13
#undef MAX_OFFSET
#define MAX_OFFSET 13
#define DIRECT2_LEN_LOG (16 - MAX_OFFSET)
#define DIRECT2_MAX_LEN ((1 << DIRECT2_LEN_LOG) - 1)
#endif

#if BUFF_SIZE < XPRESS_MAX_BLOCK
#error BUFF_SIZE should not be less than XPRESS_MAX_BLOCK
#endif

#if CODING == CODING_HUFF_LEN
#define MAX_LENGTH      32
#define HUFF_SIZE       (MAX_LENGTH * 2)
#elif CODING & (CODING_HUFF_PTR | CODING_HUFF_ALL)
#if (256 / MAX_OFFSET) >= 32
#define MAX_LENGTH_LOG  5
#else
#define MAX_LENGTH_LOG  4
#endif
#define MAX_LENGTH      (1 << MAX_LENGTH_LOG)
#if CODING == CODING_HUFF_PTR
#define HUFF_SIZE       ((MAX_LENGTH * MAX_OFFSET + 1) & ~1)
#elif CODING == CODING_HUFF_ALL
#define HUFF_SIZE       (256 + ((MAX_LENGTH * MAX_OFFSET + 1) & ~1))
#endif
#endif

#define MIN_MATCH       3       

#if CODING == CODING_HUFF_LEN
#define DECODE_BITS     8
#elif CODING & (CODING_HUFF_PTR | CODING_HUFF_ALL)
#define DECODE_BITS     10
#endif


#define MEM_ALIGN   256         
#define ALIGNED_PTR(b) \
  ((void *) (((char *) (b)) + MEM_ALIGN - (((size_t) (b)) & (MEM_ALIGN-1))))





#if defined (_M_IX86) && !defined (i386)
#define i386 1                  
#endif

#if defined (i386) && defined (NOASM)
#undef i386
#endif

#define uchar unsigned char     
#define schar signed char

#ifdef _M_IX86
#define __unaligned             
#endif

#define int4  int               
#define int2  short             
#define xint  int               
#define int32 int               
#define int16 short             

#define tag_t    int32

#ifdef i386
#define bitmask4 int32  
#define bitmask2 int16
#else
#define bitmask4 int4   
#define bitmask2 int2   
#endif


#define uint4     unsigned int4
#define uint2     unsigned int2
#define uxint     unsigned xint
#define uint32    unsigned int32
#define uint16    unsigned int16
#define utag_t    unsigned tag_t
#define ubitmask4 unsigned bitmask4
#define ubitmask2 unsigned bitmask2

#ifdef _MSC_VER
#if _MSC_VER >= 1200
#define INLINE  __forceinline
#else
#define INLINE __inline
#endif
#pragma warning(disable:4127)   
#pragma warning(disable:4711)   
#pragma warning(disable:4710)   
#pragma warning(disable:4100)   
#pragma warning(disable:4068)   
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

#if !defined (INLINE) || DEBUG
#undef  INLINE
#define INLINE static
#endif

#if !DEBUG
#undef assert
#define assert(x)
#endif

#if CODING & (CODING_DIRECT | CODING_DIRECT2)
#define MIN_SIZE0       8
#elif CODING == CODING_BY_BIT
#define MIN_SIZE0       7
#elif CODING == CODING_HUFF_LEN
#define MIN_SIZE0       44
#elif CODING == CODING_HUFF_PTR
#define MIN_SIZE0       139
#elif CODING == CODING_HUFF_ALL
#define MIN_SIZE0       261
#else
#error wrong CODING
#endif

#define MIN_SIZE        (MIN_SIZE0 + CRC_STAMP_SIZE)


#define CRC32_FIRST     0
#if SUPPORT_CRC
#define CRC_STAMP_SIZE  sizeof (uint32)
#else
#define CRC_STAMP_SIZE  0
#endif

#if DEBUG
extern long xxx[];
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4731) 
#endif

#if CODING_ALG != 1 && CODING_ALG != 6
#error CODING_ALGs different from 1 and 6 are not supported anymore
#endif

#endif 
