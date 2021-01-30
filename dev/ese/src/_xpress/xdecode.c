// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "xprs.h"

#define ALLOCATE_ON_STACK       1

#define MAGIC_DECODE 0x35DEC0DE

typedef struct
{
  struct
  {
    uchar *end, *beg, *careful, *stop, *last;
  } dst;
  struct
  {
    const uchar *end, *beg, *careful, *end_1, *end_tag, *end_bitmask2, *last;
  }
  src;
  int result;
  int eof;
  int magic;
#if CODING & (CODING_HUFF_LEN | CODING_HUFF_PTR | CODING_HUFF_ALL)
  uint16 table[(1 << DECODE_BITS) + (HUFF_SIZE << 1)];
#endif
  void *memory;
  const void *signature;
} decode_info;

#if CODING == CODING_BY_BIT
static int bit_to_len_initialized = 0;
static uchar bit_to_len1[1 << (9 - MIN_MATCH)];
static uchar bit_to_len2[1 << (9 - MIN_MATCH)];
static void bit_to_len_init (void)
{
  int i, k;
  if (bit_to_len_initialized) return;
  bit_to_len1[0] = 0;
  bit_to_len2[0] = 9 - MIN_MATCH;
  for (k = 1, i = 1 << (8 - MIN_MATCH); i != 0; i >>= 1, ++k)
  {
    memset (bit_to_len1 + i, k, i);
    memset (bit_to_len2 + i, k - 1, i);
  }
  bit_to_len_initialized = 1;
}
#endif

#if CODING & (CODING_HUFF_LEN | CODING_HUFF_PTR | CODING_HUFF_ALL)


static int huffman_decode_create (uint16 *table, const uchar *length)
{
  xint i, j, k, last, freq[16], sum[16];

  
  memset (freq, 0, sizeof (freq));
  i = HUFF_SIZE >> 1;
  do
  {
    k = length[--i];
    ++freq[k & 0xf];
    ++freq[k >> 4];
  }
  while (i != 0);

  
  if (freq[0] >= HUFF_SIZE - 1)
    goto bad;
#if 0
  {

    if (freq[1] != 1)
      goto bad;
    i = -1; do ++i; while (length[i] == 0);
    k = i << 1;
    if (length[i] != 1) ++k;
    i = 1 << DECODE_BITS;
    do
      *table++ = (uint16) k;
    while (--i > 0);
    goto ok;
  }
#endif

  
  memcpy_s (sum, sizeof(sum), freq, sizeof (freq));

  
  k = 0;
  i = 15;
  do
  {
    if ((k += freq[i]) & 1)
      goto bad;
    k >>= 1;
  }
  while (--i != 0);
  if (k != 1)
    goto bad;

  
  k = 0;
  for (i = 1; i < 16; ++i)
    freq[i] = (k += freq[i]);
  last = freq[15];      
  i = HUFF_SIZE << 4;
  do
  {
    i -= 1 << 4;
    k = length[i >> 5] >> 4;
    if (k != 0)
      table[--freq[k]] = (uint16) (k | i);
    i -= 1 << 4;
    k = length[i >> 5] & 0xf;
    if (k != 0)
      table[--freq[k]] = (uint16) (k | i);
  }
  while (i != 0);

  
  k = i = (1 << DECODE_BITS) + (HUFF_SIZE << 1);

  {
    xint n;
    for (n = 15; n > DECODE_BITS; --n)
    {
      j = i;
      while (k > j)
        table[--i] = (uint16) ((k -= 2) | 0x8000);
      for (k = sum[n]; --k >= 0;)
        table[--i] = table[--last];
      k = j;
    }
  }

  j = i;
  i = 1 << DECODE_BITS;
  while (k > j)
    table[--i] = (uint16) ((k -= 2) | 0x8000);

  while (last > 0)
  {
    k = table[--last];
    j = i - ((1 << DECODE_BITS) >> (k & 15));
    do
      table[--i] = (uint16) k;
    while (i != j);
  }
  assert ((i | last) == 0);

  return (1);

bad:
  return (0);
}


#endif 

#if DEBUG > 1
#define RET_OK do {printf ("OK @ %d\n", __LINE__); goto ret_ok;} while (0)
#define RET_ERR do {printf ("ERR @ %d\n", __LINE__); goto ret_err;} while (0)
#else
#define RET_OK goto ret_ok
#define RET_ERR goto ret_err
#endif


#define GET_UINT16(x,p) x = *(__unaligned uint16 *)(p); p += 2


#define COPY_8_BYTES(dst,src) \
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3]; \
  dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7]



#define COPY_BLOCK_SLOW(dst,src,len) \
  if (len > 8) do \
  { \
    COPY_8_BYTES (dst, src); \
    len -= 8; dst += 8; src += 8; \
  } \
  while (len > 8); \
  do \
    *dst++ = *src++, --len; \
  while (len)


#ifndef i386
#define COPY_BLOCK_FAST_8(dst,src) \
  COPY_8_BYTES (dst, src)
#else
#if 0
#define COPY_BLOCK_FAST_8(dst,src) \
  ((__unaligned uint32 *) dst)[0] = ((__unaligned uint32 *) src)[0]; \
  ((__unaligned uint32 *) dst)[1] = ((__unaligned uint32 *) src)[1]
#else
#define COPY_BLOCK_FAST_8(dst,src) \
  ((__unaligned __int64 *) dst)[0] = ((__unaligned __int64 *) src)[0]
#endif
#endif 


#define BIORD(bits) \
  (Mask >> (sizeof (Mask) * 8 - (bits)))

#define CONCAT2(x,y) x##y
#define CONCAT(x,y) CONCAT2(x,y)

#define bits_t signed char

#define BIORD_MORE0(bits)                       \
  if ((Bits = (bits_t) (Bits - (bits))) < 0)    \
  {                                             \
    CAREFUL_ERR_IF (src >= info->src.end_1);    \
    Mask += ((ubitmask4) *(__unaligned ubitmask2 *)src) << (-Bits); \
    src += sizeof (ubitmask2);                      \
    Bits += (bits_t) (sizeof (ubitmask2) * 8);      \
  }


#define BIORD_MORE(bits)                        \
  Mask <<= (bits_t)(bits);                      \
  BIORD_MORE0 (bits)


#define BIORD_WORD(result,bits)         \
  result = 1 << (bits);                 \
  if (bits)                             \
  {                                     \
    result += BIORD (bits);             \
    BIORD_MORE (bits);                  \
  }


#define BIORD_DECODE(result,table) {     \
  bits_t __bits;                         \
  result = ((int16 *)(table))[BIORD (DECODE_BITS)]; \
  if (result < 0)                        \
  {                                      \
    Mask <<= DECODE_BITS;                \
    do                                   \
    {                                    \
      result &= 0x7fff;                  \
      if ((bitmask4) Mask < 0) ++result; \
      result = ((int16 *)(table))[result];          \
      Mask <<= 1;                        \
    }                                    \
    while (result < 0);                  \
    __bits = (bits_t)(result & 15);      \
  }                                      \
  else                                   \
  {                                      \
    __bits = (bits_t)(result & 15);      \
    Mask <<= __bits;                     \
  }                                      \
  result >>= 4;                          \
  Bits = (bits_t) (Bits - __bits);       \
}                                        \
if (Bits < 0)                            \
{                                                         \
  CAREFUL_ERR_IF (src >= info->src.end_1);                \
  if (CODING == CODING_HUFF_ALL)                          \
    {CAREFUL_IF (src >= info->src.careful, rdmore);}      \
  Mask += ((ubitmask4) *(__unaligned ubitmask2 *)src) << (-Bits); \
  src += sizeof (ubitmask2);                              \
  Bits += (bits_t) (sizeof (ubitmask2) * 8);              \
}

#define CAREFUL 0
#include "xdecode.i"
#define CAREFUL 1
#include "xdecode.i"



#undef DECODER_SIGNATURE
#if (defined (_M_IX86) || defined (_M_AMD64) || defined (_M_IA64)) && (CODING & (CODING_HUFF_ALL | CODING_DIRECT2)) != 0
#define DECODER_SIGNATURE 1
__declspec(align(256))
static const uchar XpressDecoderSignature[256 + 2] = {
    0x2f, 0x19, 0xf6, 0x23, 0x3e, 0xbb, 0xb1, 0xfb, 0xad, 0xb6, 0x3a, 0x5d, 0x7a, 0xed, 0x91, 0xff,
    0xc6, 0x6c, 0x52, 0xc6, 0x4f, 0x30, 0xb4, 0xb6, 0xda, 0xad, 0xe2, 0x17, 0x61, 0xe8, 0x13, 0xd4,
    0x02, 0x48, 0xa4, 0x39, 0x5c, 0xfc, 0x29, 0x5f, 0xd5, 0xf0, 0x24, 0x92, 0x3f, 0x1f, 0x6c, 0x31,
    0xd4, 0xff, 0x5d, 0x2b, 0xe1, 0x00, 0x39, 0x19, 0xd2, 0x94, 0x92, 0xfe, 0xf4, 0xb6, 0xed, 0xdc,
    0xfc, 0xc5, 0x05, 0x07, 0xc3, 0x4d, 0xcc, 0x02, 0x26, 0xc3, 0x46, 0x3d, 0xda, 0xa3, 0x14, 0x7d,
    0x4b, 0x00, 0xab, 0xf9, 0x1b, 0xb0, 0xa0, 0xfd, 0x64, 0xc9, 0x9d, 0xd0, 0xf2, 0xa0, 0x54, 0xa8,
    0xad, 0x88, 0x81, 0x4e, 0xec, 0xa8, 0xc0, 0x4f, 0x4e, 0xa8, 0x70, 0xa6, 0x9d, 0x65, 0xbe, 0x61,
    0x35, 0x0a, 0x7b, 0xf9, 0xa6, 0x14, 0x79, 0x8b, 0x7a, 0xa1, 0xaf, 0xdd, 0x55, 0xb2, 0xf8, 0xa4,
    0x0b, 0x53, 0x53, 0xdf, 0x05, 0xcd, 0xd9, 0x85, 0x77, 0x2d, 0x77, 0x7b, 0x0a, 0xa9, 0xe2, 0x4f,
    0x0a, 0x14, 0xcc, 0xe4, 0xed, 0x18, 0x1f, 0x78, 0x2f, 0x8e, 0x01, 0x5d, 0xb3, 0xdf, 0x81, 0xbe,
    0x7b, 0x61, 0xac, 0x04, 0xcf, 0xa2, 0x18, 0x8c, 0x06, 0x71, 0x1d, 0x1b, 0xfa, 0x45, 0x95, 0x5e,
    0xa6, 0x49, 0x39, 0x66, 0x4b, 0xe5, 0x55, 0x66, 0x74, 0xeb, 0xf4, 0x93, 0xcb, 0x21, 0xb4, 0x74,
    0x88, 0x59, 0x24, 0xaf, 0x24, 0x95, 0x20, 0x13, 0x05, 0x9b, 0x1f, 0x56, 0xa6, 0x7e, 0x92, 0x44,
    0x64, 0x41, 0xe5, 0x15, 0x5c, 0x4a, 0x46, 0x4f, 0xb1, 0x1c, 0xa8, 0x41, 0xfd, 0xc0, 0xc5, 0x31,
    0xa9, 0xca, 0x5e, 0x2b, 0x62, 0x34, 0xa2, 0xe6, 0x9a, 0x44, 0x0c, 0x40, 0xc8, 0xc1, 0x25, 0x95,
    0x14, 0x6e, 0x38, 0xb8, 0x49, 0xf9, 0x6e, 0x4f, 0x61, 0x2d, 0x51, 0xd3, 0x2f, 0xeb, 0x01, 0xbe,
    0x81, CODING
};
#endif



XPRESS_EXPORT
int
XPRESS_CALL
XpressDecode
(
  XpressDecodeStream stream,
  __in_bcount(decode_size) void              *orig,
  int                orig_size,
	int 		  decode_size,
  __in_ecount(comp_size) const void        *comp,
  int                comp_size
)
{
  decode_info *info;
  const uchar *src;

#if ALLOCATE_ON_STACK
  char stack_buffer [sizeof (*info) + MEM_ALIGN];
  info = ALIGNED_PTR (stack_buffer);

  __analysis_assume(((char*)info + sizeof (*info)) < (stack_buffer + MEM_ALIGN));

  info->src.beg = (void *) stream;
#else
  if (stream == 0 || (info = (decode_info *) stream)->magic != MAGIC_DECODE || (uxint) decode_size > (uxint) orig_size)
    return (-1);
#endif

  if (decode_size > orig_size)
    decode_size = orig_size;

  if (comp_size == orig_size || decode_size == 0)
    return (decode_size);

  if (orig_size < comp_size
    || comp_size < 0
    || orig_size <= MIN_SIZE
    || comp_size < MIN_SIZE
    || orig_size > BUFF_SIZE
    || decode_size < 0
  )
  {
    return (-1);
  }

  src = comp;
  info->dst.beg = orig;
  info->dst.end = (uchar *) orig + orig_size;
  info->dst.stop = (uchar *) orig + decode_size;
  info->src.end = src + comp_size;
  info->src.end_1 = info->src.end - 1;
  info->src.end_tag = info->src.end - (sizeof (tag_t) - 1);
  info->src.end_bitmask2 = info->src.end - (sizeof (bitmask2) - 1);

#ifdef DECODER_SIGNATURE
  info->signature = XpressDecoderSignature;
#endif 


  #define RESERVE_DST ((8 * 8 + 2) * sizeof (tag_t))
  info->dst.careful = info->dst.beg;
  if (info->dst.stop - info->dst.beg > RESERVE_DST)
    info->dst.careful = info->dst.stop - RESERVE_DST;

  #define RESERVE_SRC ((7 * 8 + 2) * sizeof (tag_t))
  info->src.careful = info->src.beg;
  if (info->src.end - info->src.beg > RESERVE_SRC)
    info->src.careful = info->src.end - RESERVE_SRC;

#if CODING & (CODING_HUFF_LEN | CODING_HUFF_PTR | CODING_HUFF_ALL)
  if (!huffman_decode_create (info->table, src))
    return (-1);
  src += HUFF_SIZE >> 1;
#endif
#if CODING == CODING_BY_BIT
  bit_to_len_init ();
#endif

  info->src.beg = src;
  info->result = 0;
  info->eof = 0;

  do_decode (info);

  if (!info->result || info->dst.last > info->dst.stop || info->src.last > info->src.end
    || (info->dst.stop == info->dst.end && !info->eof)
  )
    return (-1);

  return (decode_size);
}

XPRESS_EXPORT
XpressDecodeStream
XPRESS_CALL
XpressDecodeCreate (
  void          *context,
  XpressAllocFn *AllocFn
)
{
#if ALLOCATE_ON_STACK
#if !defined(_WIN64)
  return ((XpressDecodeStream) 1);
#else
  return ((XpressDecodeStream) (__int64) 1);
#endif
#else
  char *b;
  decode_info *info;
  if (AllocFn == 0 || (b = AllocFn (context, sizeof (*info) + MEM_ALIGN)) == 0)
    return (0);
  info = ALIGNED_PTR (b);
  info->memory = b;
  info->magic = MAGIC_DECODE;
  return ((XpressDecodeStream) info);
#endif
}

XPRESS_EXPORT
void
XPRESS_CALL
XpressDecodeClose (
  XpressDecodeStream stream,
  void              *context,
  XpressFreeFn      *FreeFn
)
{
#if ALLOCATE_ON_STACK
  
#else
  decode_info *info = (decode_info *) stream;
  if (FreeFn != 0 && info != 0 && info->magic == MAGIC_DECODE)
  {
    info->magic = 0;
    FreeFn (context, info->memory);
  }
#endif
}
