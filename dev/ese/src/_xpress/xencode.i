// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef ALIGN
#if DEBUG
#define ALIGN(n)        // a lot of bugs in early versions of VC caused by align
#else
#define ALIGN(n) align n
#endif /* DEBUG */
#endif /* ALIGN */

#if CHAIN >= 2 && !defined (i386)

INLINE int find_match (prs *p)
{
  const uchar *p1;
  xint k, n, m;
#if CHAIN >= 3
  xint chain = v.chain;
#endif

  p->x.z_next[0] = (z_index_t) (k = n = v.orig.pos);
  for (;;)
  {
    m = p->x.z_next[k];
#ifdef i386compat
    {
#if CHAIN < 3
      uint16 c = *(__unaligned uint16 *)((p1 = v.orig.ptr + (MIN_MATCH - 2)) + n);
#else
      uint16 c = *(__unaligned uint16 *)((p1 = v.orig.ptr + v.match.len - 1) + n);
      do
      {
        if (--chain < 0)
          return (v.match.len >= MIN_MATCH);
#endif /* CHAIN >= 3 */
        k = p->x.z_next[m]; if (*(__unaligned uint16 *) (p1 + m) == c) goto same_m;
        m = p->x.z_next[k]; if (*(__unaligned uint16 *) (p1 + k) == c) goto same_k;
        k = p->x.z_next[m]; if (*(__unaligned uint16 *) (p1 + m) == c) goto same_m;
        m = p->x.z_next[k]; if (*(__unaligned uint16 *) (p1 + k) == c) goto same_k;
        k = p->x.z_next[m]; if (*(__unaligned uint16 *) (p1 + m) == c) goto same_m;
        m = p->x.z_next[k]; if (*(__unaligned uint16 *) (p1 + k) == c) goto same_k;
        k = p->x.z_next[m]; if (*(__unaligned uint16 *) (p1 + m) == c) goto same_m;
        m = p->x.z_next[k]; if (*(__unaligned uint16 *) (p1 + k) == c) goto same_k;

#if CHAIN < 3
        if (*(__unaligned uint16 *) (p1 + m) == c) goto same_m;
        return (0);
#else
      }
      while (1);
#endif /* CHAIN < 3 */

#else /* !i386compat */

    {
#if CHAIN < 3
      const uchar *p0 = v.orig.ptr + (MIN_MATCH - 1);
#else
      const uchar *p0 = v.orig.ptr + v.match.len;
#endif
      const uchar  c0 = p0[n];
      const uchar  c1 = (p1 = p0 - 1)[n];

#if CHAIN >= 3
      do
      {
        if (--chain < 0)
          return (v.match.len >= MIN_MATCH);
#endif /* CHAIN >= 3 */
        k = p->x.z_next[m]; if (p0[m] == c0 && p1[m] == c1) goto same_m;
        m = p->x.z_next[k]; if (p0[k] == c0 && p1[k] == c1) goto same_k;
        k = p->x.z_next[m]; if (p0[m] == c0 && p1[m] == c1) goto same_m;
        m = p->x.z_next[k]; if (p0[k] == c0 && p1[k] == c1) goto same_k;
        k = p->x.z_next[m]; if (p0[m] == c0 && p1[m] == c1) goto same_m;
        m = p->x.z_next[k]; if (p0[k] == c0 && p1[k] == c1) goto same_k;
        k = p->x.z_next[m]; if (p0[m] == c0 && p1[m] == c1) goto same_m;
        m = p->x.z_next[k]; if (p0[k] == c0 && p1[k] == c1) goto same_k;

#if CHAIN < 3
        if (p0[m] == c0 && p1[m] == c1) goto same_m;
        return (v.match.len >= MIN_MATCH);
#else
      }
      while (1);
#endif /* CHAIN < 3 */

#endif /* i386compat */

    same_m:
      k = m;
    same_k:
#if MAX_OFFSET < BUFF_SIZE_LOG
      if ((uxint) (n - k  - 1)>= (1 << MAX_OFFSET) - 1)
#else
      if (k == n)
#endif
#if CHAIN < 3
        return (0);
#else
        return (v.match.len >= MIN_MATCH);
#endif /* CHAIN < 3 */
    }
    {
      const uchar *p2;
      p1 = v.orig.ptr;
      p2 = p1 + k;
      p1 += n;

#ifdef i386compat
      if ((m = *(__unaligned uint32 *)p2 ^ *(__unaligned uint32 *)p1) != 0)
      {
#if MIN_MATCH <= 3
        assert (p1 + 3 <= v.orig.end);
#if CHAIN < 3
    assert (v.match.len <= MIN_MATCH-1);
        if ((m & 0xffffff) == 0)
#else
        if ((m & 0xffffff) == 0 && v.match.len <= 3)
#endif /* CHAIN < 3 */
        {
          v.match.len = 3;
          v.match.pos = k;
#if CHAIN < 3
          return (1);
#endif /* CHAIN < 3 */
        }
#endif /* MIN_MATCH <= 3 */

        goto cont;
      }
#else /* !i386compat */
      if (p1[0] != p2[0] || p1[1] != p2[1]
#if CHAIN >= 3
        || p1[2] != p2[2]
#endif /* CHAIN >= 3 */
      )
        goto cont;
      if (p1[3] != p2[3])
      {
#if MIN_MATCH <= 3
        assert (p1 + 3 <= v.orig.end);
        // if (v.match.len <= 2 && p1 + 3 <= v.orig.end)
#if CHAIN >= 3
    if (v.match.len <= 2)
#endif /* CHAIN >= 3 */
        {
          v.match.len = 3;
          v.match.pos = k;
#if CHAIN < 3
          return (1);
#endif /* CHAIN < 3 */
        }
#endif /* MIN_MATCH <= 3 */
        goto cont;
      }
#endif /* i386compat */

      if (p1 <= v.orig.end_16)
      {
        goto entry4;
        do
        {
#define X(i) if (p1[i] != p2[i]) {p1 += i; goto chk;}
          X(0); X(1); X(2); X(3);
        entry4:
          X(4); X(5); X(6); X(7); X(8);
          X(9); X(10); X(11); X(12); X(13); X(14); X(15);
#undef  X
          p1 += 16; p2 += 16;
        }
        while (p1 <= v.orig.end_16);
      }
      while (p1 != v.orig.end)
      {
        if (*p1 != *p2)
          goto chk;
        ++p1;
        ++p2;
      }
#define SET_LENGTH() \
      n = -n; \
      n += (xint) (p1 - v.orig.ptr); \
      if (CHAIN < 3 || n > v.match.len) \
      { \
        v.match.len = n; \
        v.match.pos = k; \
      }
      SET_LENGTH ();
#if CHAIN < 3
      return (1);
#else
      return (v.match.len >= MIN_MATCH);
#endif /* CHAIN < 3 */
    }
  chk:
    SET_LENGTH ();
#if CHAIN < 3
    return (1);
  cont:
    return (0);
#else
  cont:
    n = v.orig.pos;
#endif /* CHAIN < 3 */
  }
}


static void encode_pass1 (prs *p)
{
  uchar *ptr = v.temp.ptr;
#if CHAIN < 3
  v.match.len = MIN_MATCH-1;
#endif /* CHAIN < 3 */
  do
  {
    if (p->x.z_next[v.orig.pos] == 0)
      goto literal;
#if CHAIN >= 3
    v.match.len = MIN_MATCH-1;
#endif /* CHAIN >= 3 */
#if 0
{
  if (ptr - v.temp.beg == 0x746b)
  {
    int i, k;
    for (i = 0, k = v.orig.pos; k > 0; k = p->x.z_next[k], ++i)
    {
      printf ("%2d: 0x%04x (offset = 0x%04x)\n", i, k, v.orig.pos - k);
    }
  }
}
#endif
    if (!find_match (p))
    {
      assert (v.match.len <= MIN_MATCH-1);
    literal:
      write_lit (p, ptr, v.orig.ptr[v.orig.pos]);
      v.orig.pos += 1;
    }
    else
    {
      assert (v.match.len > MIN_MATCH-1);
#if 0
{
  int k = ptr - v.temp.beg;
  if (k >= 0x7460 && k <= 0xffff)
  {
    printf ("%d %x: 0x%x 0x%x\n", k, k, v.orig.pos - v.match.pos, v.match.len);
    printf ("last = 0x%x, end = 0x%x\n", v.orig.pos + v.match.len, v.orig.end - v.orig.ptr);
  }
}
#endif
      ptr = write_ptr (p, ptr, v.orig.pos - v.match.pos, v.match.len);
      v.orig.pos += v.match.len;
#if CHAIN < 3
      v.match.len = MIN_MATCH-1;
#endif /* CHAIN < 3 */
    }
  }
  while (v.orig.pos < v.orig.stop);
  v.temp.ptr = ptr;
}

#endif /* CHAIN >= 2 */


#if CHAIN < 2 || defined (i386)
#if (CODING != CODING_DIRECT2 && CODING != CODING_HUFF_ALL) || !defined (i386)

static void encode_pass1 (prs *p)
{
  const uchar *b, *b1, *stop;
  uchar *ptr;
#if CHAIN > 0
  xint pos = v.orig.pos;
#endif

  b = v.orig.ptr;
  v.orig.ptr_stop = stop = b + v.orig.stop;
  b += v.orig.pos;
  ptr = v.temp.ptr;

  if (b == v.orig.ptr)
  {
    write_lit (p, ptr, *b);
    ++b;
#if CHAIN > 0
      ++pos;
#endif
  }
  goto literal_entry;

  for (;;)
  {
    do
    {
#if MAX_OFFSET < BUFF_SIZE_LOG
    next:
#endif
      write_lit (p, ptr, *b);
      ++b;
#if CHAIN > 0
      ++pos;
#endif

    literal_entry:
      if (b >= stop)
        goto ret;

      {
        uxint h;
#if CHAIN <= 0
        h = Q_HASH_SUM (b);
        b1 = p->x.q_last[h];
        p->x.q_last[h] = b;
#else
        assert (pos == (xint) (b - v.orig.ptr));
        h = Z_HASH_SUM (b);
        b1 = v.orig.ptr + p->x.z_next[h];
        p->x.z_next[h] = (z_index_t) pos;
#endif
      }
#if MAX_OFFSET < BUFF_SIZE_LOG
      if (b1 <= b - (1 << MAX_OFFSET))
        goto next;
#endif
    }
    while (
#if FILL_NULL && CHAIN <= 0
      b1 == 0 ||
#endif
      b1[0] != b[0] || b1[1] != b[1] || b1[2] != b[2]
    );

    assert ((xint) (v.orig.ptr + v.orig.size - b) > 7);

    {
      const uchar *b0 = b;

      if (b <= v.orig.end_16)
        goto match_entry_3;
      goto match_careful;

      do
      {
#define X(i) if (b1[i] != b[i]) {b += i; b1 += i; goto eval_len;}
        X(0); X(1); X(2);
      match_entry_3:
        X(3); X(4); X(5); X(6); X(7);
        X(8); X(9); X(10); X(11);
        X(12); X(13); X(14); X(15);
#undef  X
        b += 16; b1 += 16;
      }
      while (b <= v.orig.end_16);

    match_careful:
      while (b != v.orig.end && *b1 == *b)
      {
        ++b;
        ++b1;
      }

    eval_len:
#if BUFF_SIZE_LOG > 16
#error
#endif
      ptr = write_ptr (p, ptr, (xint) (b - b1), (xint) (b - b0));
      b1 = b0;
    }

    ++b1;
#if CHAIN > 0
    ++pos;
#endif

    if (b > v.orig.end_3)
    {
      while (b1 < v.orig.end_3)
      {
#if CHAIN <= 0
        p->x.q_last[Q_HASH_SUM (b1)] = b1;
#else
        assert (pos == (xint) (b1 - v.orig.ptr));
        p->x.z_next[Z_HASH_SUM (b1)] = (z_index_t) pos;
        ++pos;
#endif
        ++b1;
      }
      goto literal_entry;
    }

    do
    {
#if CHAIN <= 0
      p->x.q_last[Q_HASH_SUM (b1)] = b1;
#else
      assert (pos == (xint) (b1 - v.orig.ptr));
      p->x.z_next[Z_HASH_SUM (b1)] = (z_index_t) pos;
      ++pos;
#endif
      ++b1;
    }
    while (b1 != b);

    goto literal_entry;
  }

ret:
  v.orig.pos = (xint) (b - v.orig.ptr);
  v.temp.ptr = ptr;
}

#else /* CODING != CODING_DIRECT2 && CODING != CODING_HUFF_ALL */

#ifndef BITMASK_TABLE_PRESENT
#define BITMASK_TABLE_PRESENT
static const int32 bit_mask_table[] = {
  0x0001, 0x0002, 0x0004, 0x0008,
  0x0010, 0x0020, 0x0040, 0x0080,
  0x0100, 0x0200, 0x0400, 0x0800,
  0x1000, 0x2000, 0x4000, 0x8000,
  0x10000
};
#endif /* BITMASK_TABLE_PRESENT */


static void encode_pass1 (prs *PrsPtr)
{

#define PRS     edx
#define OUT     ebx
#define TAG     ebp
#define TAGW    bp

// access to prs structure fields
#define V               [PRS - SIZE prs] prs.c

#if CODING == CODING_HUFF_ALL
#define INC_MASKS_ASM   __asm   inc dword ptr V.stat.masks
#else
#define INC_MASKS_ASM
#endif /* CODING == CODING_HUFF_ALL */

// TAG = tag_mask; adjusts TAG (tag_mask), V.temp.tag_ptr, and OUT (output pointer)
#define WRITE_TAG_MASK()                        \
        __asm   mov     ecx, V.temp.tag_ptr     \
        __asm   mov     V.temp.tag_ptr, OUT     \
        __asm   add     OUT, 4                  \
        __asm   mov     [ecx], TAG              \
        INC_MASKS_ASM                           \
        __asm   mov     TAG, 1

#if CHAIN <= 0

// access to respective hash table entry
#define Q_HTABLE(idx)   dword ptr [PRS + idx*4] prs.x.q_last

// evaluate hash sum of [esi] on eax; spoils eax, ecx, TAG
#define Q_HASH_SUM_ASM()                                                    \
        __asm   movzx   ecx, byte ptr [esi]                                 \
        __asm   movzx   edi, byte ptr [esi+1]                               \
        __asm   movzx   eax, byte ptr [esi+2]                               \
        __asm   lea     eax, [eax + ecx * (1 << Q_HASH_SH1)]                \
        __asm   lea     eax, [eax + edi * (1 << Q_HASH_SH2)]                \

#else

// access to respective hash table entry
// access to respective single-linked list entry
#define Z_NEXT(idx) word ptr [PRS + idx*2] prs.x.z_next

#if CHAIN >= 2
#define Z_HTABLE(idx)   word ptr [PRS + idx*2 - SIZE prs] prs.x.z_hash
#else
#define Z_HTABLE(idx)   word ptr [PRS + idx*2] prs.x.z_next
#endif /* CHAIN >= 2  */

// evaluate hash sum of [esi] on eax; spoils eax, ecx, edi
#define Z_HASH_SUM_ASM()                                                    \
        __asm   movzx   eax, byte ptr [esi]                                 \
        __asm   movzx   ecx, byte ptr [esi+1]                               \
        __asm   movzx   edi, byte ptr [esi+2]                               \
        __asm   movzx   eax, word ptr z_hash_map[eax*2]                     \
        __asm   movzx   ecx, word ptr z_hash_map[ecx*2][512]                \
        __asm   movzx   edi, word ptr z_hash_map[edi*2][1024]               \
        __asm   xor     eax, ecx                                            \
        __asm   xor     eax, edi

#endif /* CHAIN <= 0 */

__asm
{
        push    TAG                     // save TAG

        mov     PRS, PrsPtr             // PRS = PrsPtr (globally)

        // esi = b
        // edi = b1
        // OUT = V.prs.temp.ptr
        // TAG = V.temp.tag_mask

        mov     esi, V.orig.ptr         // obtain b, b1, temp.ptr, and temp.mask
        mov     eax, V.orig.stop
        add     eax, esi
        mov     V.orig.ptr_stop, eax    // and set orig.ptr_stop by orig.stop
        add     esi, V.orig.pos
        mov     OUT, V.temp.ptr
        mov     TAG, V.temp.tag_mask

        cmp     esi, V.orig.ptr         // if beginning of buffer
        jne     write_literal_entry     // then write literal immediately

#if CHAIN >= 2
    jmp write_literal
#endif /* CHAIN >= 2 */

        ALIGN (16)
#if CHAIN >= 2
write_literal_0:
    add esi, ecx
#endif /* CHAIN >= 2 */

write_literal:

#if CODING == CODING_HUFF_ALL
    movzx   eax, [esi]      // read the literal

#if CHAIN == 2
write_literal_1:
#endif /* CHAIN == 2 */

        inc     OUT                     // shift dst ptr in advance
        inc     esi                     // shift src ptr to next character
        inc dword ptr V.stat.freq[eax*4]
        mov     [OUT-1], al             // emit literal
#else
        mov     al, [esi]               // read the literal

#if CHAIN == 2
write_literal_1:
#endif /* CHAIN == 2 */

        inc     OUT                     // shift dst ptr in advance
        inc     esi                     // shift src ptr to next character
        mov     [OUT-1], al             // emit literal
#endif /* CODING == CODING_HUFF_ALL */

        add     TAG, TAG                // write tag bit 0
        jc      write_literal_tag_new   // save tag word if it is full
write_literal_tag_done:


write_literal_entry:

#if 0
    sub OUT, V.temp.beg
    cmp OUT, 0x746B
    jb  L1
    int 3
L1:
    add OUT, V.temp.beg
#endif

        cmp     esi, V.orig.ptr_stop    // processed everything?
        jae     pass1_stop              // yes, stop


#if CHAIN <= 0
        Q_HASH_SUM_ASM ()               // evaluate hash sum
#if MAX_OFFSET < BUFF_SIZE_LOG
        lea     ecx, [esi - (1 << MAX_OFFSET) + 1] // min. allowed left bound
#endif
        mov     edi, Q_HTABLE (eax)     // edi = candidate ptr
        mov     Q_HTABLE (eax), esi     // save current ptr
check_candidate:
#else

#if CHAIN >= 2
        mov     ecx,V.orig.ptr
        sub     esi, ecx                // esi = offset to current ptr
        movzx   edi, Z_NEXT (esi)     // edi = offset to candidate ptr
#else
        Z_HASH_SUM_ASM ()               // evaluate hash sum
        mov     ecx,V.orig.ptr
        movzx   edi, Z_HTABLE (eax)     // edi = offset to candidate ptr
        sub     esi, ecx                // esi = offset to current ptr
#endif /* CHAIN >= 2 */

#if CHAIN >= 2
    test    edi, edi
    jz  write_literal_0

    mov ax, [ecx + esi + 1]

#if CHAIN >= 3
    mov Z_NEXT (0), si      // cycle list in advance
    mov V.match.len, MIN_MATCH-1
#endif /* CHAIN >= 3 */

    cmp ax, [ecx + edi + 1] // 1
    je  CheckMatch_0

#if CHAIN < 3
    mov Z_NEXT (0), si      // don't have to do it in advance
#endif /* CHAIN < 3 */

    movzx   edi, Z_NEXT (edi)   // 2
    cmp ax, [ecx + edi + 1]
    je  CheckMatch

    movzx   edi, Z_NEXT (edi)   // 3
    cmp ax, [ecx + edi + 1]
    je  CheckMatch

    movzx   edi, Z_NEXT (edi)   // 4
    cmp ax, [ecx + edi + 1]
    je  CheckMatch

    movzx   edi, Z_NEXT (edi)   // 5
    cmp ax, [ecx + edi + 1]
    je  CheckMatch

    movzx   edi, Z_NEXT (edi)   // 6
    cmp ax, [ecx + edi + 1]
    je  CheckMatch

    movzx   edi, Z_NEXT (edi)   // 7
    cmp ax, [ecx + edi + 1]
    je  CheckMatch

#if CHAIN == 2
    movzx   edi, Z_NEXT (edi)   // 8
    cmp ax, [ecx + edi + 1]
    je  CheckMatch
#endif /* CHAIN == 2 */

    movzx   edi, Z_NEXT (edi)   // 9
    cmp ax, [ecx + edi + 1]

#if CHAIN == 2
    jne write_literal_0
#else
    je  CheckMatch

        add esi, ecx        // esi = current ptr
continue_search_0:
    mov eax, V.chain
        add     edi, ecx                // edi = candidate ptr
        dec eax
        mov V.temp.chain, eax
    jmp continue_search
#endif /* CHAIN == 2 */

    ALIGN (16)
CheckMatch:
    cmp edi, esi
    je  write_literal_0

CheckMatch_0:

#if CHAIN == 2 && CODING == CODING_HUFF_ALL
    movzx   eax, [esi + ecx]    // read first character (literal)
#else
    mov al, [esi + ecx]     // read first character (literal)
#endif /* CHAIN == 2 && CODING_HUFF_ALL */
    add esi, ecx
    cmp al, [edi + ecx]

#if CHAIN >= 3
    jne continue_search_0
    mov eax, V.chain
        add     edi, ecx                // edi = candidate ptr
        dec eax
        mov V.temp.chain, eax
#else
    jne write_literal_1
        add     edi, ecx                // edi = candidate ptr
#endif /* CHAIN >= 3 */

#else /* CHAIN < 2 */

        add     edi, ecx                // edi = candidate ptr
        mov     Z_HTABLE (eax), si      // store current ptr offset
        add     esi, ecx                // restore current ptr

#endif /* CHAIN >= 2 */

#if MAX_OFFSET < BUFF_SIZE_LOG
        lea     ecx, [esi - (1 << MAX_OFFSET) + 1] // min. allowed left bound
#endif
#endif /* CHAIN <= 0 */

#if MAX_OFFSET < BUFF_SIZE_LOG
        cmp     edi, ecx
#if CHAIN <= 2                          // candidate is in window?
        js      write_literal           // no, then emit literal
#else
        js      continue_search_1       // no, then emit literal
#endif /* CHAIN <= 2 */
#endif /* MAX_OFFSET < BUFF_SIZE_LOG */

#if CHAIN <= 0 && FILL_NULL
        test    edi, edi                // is it NULL?
        jz      write_literal           // emit literal if so
#endif /* CHAIN <= 0 && FILL_NULL */

#if CHAIN >= 2
    mov al, [esi + 3]
    cmp al, [edi + 3]
    je  length_4
#else
        mov     eax, [esi]              // get first 4 src bytes
        sub     eax, [edi]              // diff them with first 4 candidate bytes
        je      length_4                // if no diff then match is at least 4 bytes

        test    eax, 0xffffff           // is there any difference in first 3 bytes?
        jne     write_literal           // if yes emit literal
#endif /* CHAIN >= 2 */

#if CHAIN >= 3
    cmp V.match.len, 3
    jae continue_search

    mov V.match.len, 3
    mov V.match.pos, edi

    ALIGN (16)
continue_search:
    dec V.temp.chain
    jl  write_pointer

#if MAX_OFFSET < BUFF_SIZE_LOG && CHAIN >= 3
continue_search_1:
#endif /* MAX_OFFSET < BUFF_SIZE_LOG && CHAIN >= 3 */
    mov ecx, V.match.len
    mov ax, [esi + ecx - 1]
    add ecx, V.orig.ptr
    sub edi, V.orig.ptr
    dec ecx

continue_search_loop:
    movzx   edi, Z_NEXT (edi)   // 1
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    movzx   edi, Z_NEXT (edi)   // 2
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    movzx   edi, Z_NEXT (edi)   // 3
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    movzx   edi, Z_NEXT (edi)   // 4
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    movzx   edi, Z_NEXT (edi)   // 5
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    movzx   edi, Z_NEXT (edi)   // 6
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    movzx   edi, Z_NEXT (edi)   // 7
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    movzx   edi, Z_NEXT (edi)   // 8
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    dec V.temp.chain
    jge continue_search_loop
    jmp write_pointer

    ALIGN (16)

continue_CheckMatch:
    mov eax, V.orig.ptr
    inc ecx
    add edi, eax
    sub ecx, eax

    cmp esi, edi
    jz  write_pointer

#if MAX_OFFSET < BUFF_SIZE_LOG
        lea     eax, [esi - (1 << MAX_OFFSET) + 1] // min. allowed left bound
        cmp     edi, eax                // candidate is in window?
        js      write_pointer           // no, then emit literal
#endif /* MAX_OFFSET < BUFF_SIZE_LOG */

    mov eax, [esi]
    sub eax, [edi]
    jz  length_4

    test    eax, 0xffffff
    jnz continue_search

    cmp ecx, 3
    jae continue_search

    mov ecx, 3
    mov V.match.len, 3
    mov V.match.pos, edi
    jmp continue_search

    ALIGN (16)

#else /* CHAIN >= 3 */

        mov     ecx, 3                  // save match ptr of length ECX
        sub     edi, esi                // edi = -offset

#endif /* CHAIN >= 3 */

write_small_ptr:
#if CODING == CODING_HUFF_ALL
    neg edi         // edi = offset
    bsr eax, edi        // eax = most significant bit number of offset
    add V.stat.pointers, 3  // have 3 more pointer bytes
    sub edi, bit_mask_table[eax*4]      // edi = remaining offset bits
    add V.stat.extra, eax   // have "eax" more extra bits to encode
    shl eax, MAX_LENGTH_LOG // prepare output byte mask
    add OUT, 3          // adjust output pointer
    lea eax, [eax + ecx - MIN_MATCH]    // eax = (log_offset : length) packed
    inc dword ptr V.stat.freq[eax*4+256*4]  // increment respective frequency count
    mov [OUT-3], al     // write packed (log_offset : length)

write_offset:
    cmp eax, (9 << MAX_LENGTH_LOG) // offset is less than 9 bits?
    sbb eax, eax        // eax = -1 if offset is less than 9 bits, 0 otherwise
    mov [OUT-2], di     // write offset
    add OUT, eax        // shift output pointer back if offset is less than 9 bits
    add V.stat.pointers, eax    // have one less pointer bytes
    stc
    lea eax, [esi+ecx]      // eax = end of src match
#else
        lea     eax, [esi+ecx]          // eax = end of src match
        not     edi                     // edi = offset-1
        add     OUT, 2                  // adjust output ptr in advance
        shl     edi, DIRECT2_LEN_LOG    // make room for length
        inc     esi                     // esi = next substring (current already inserted)
        lea     edi, [edi + ecx - MIN_MATCH]    // combine offset and shoft length
        stc                             // set carry bit
        mov     [OUT-2], di             // save packed pointer
#endif

        adc     TAG, TAG                // write tag bit 1
        jc      write_pointer_tag_new   // write tag word when it is full
write_pointer_tag_done:

#if CHAIN < 2
        cmp     eax, V.orig.end_3       // is it too close to end of buffer?
        ja      insert_tail             // if yes process is specially avoiding read overrun
#endif /* CHAIN < 2 */

#if CHAIN >= 2

    mov esi, eax
    jmp write_literal_entry

#elif CHAIN <= 0

#if 1
        push    TAG
        lea     TAG, [eax-3]            // eax = end-of-match

        movzx   eax, byte ptr [esi]
        movzx   ecx, byte ptr [esi+1]
    movzx   edi, byte ptr [esi+2]

    cmp esi, TAG
    jbe insert_all_3_entry

    lea eax, [ecx + eax * (1 << (Q_HASH_SH1 - Q_HASH_SH2))]
    lea eax, [edi + eax * (1 << Q_HASH_SH2)]
    mov Q_HTABLE (eax), esi

    movzx   eax, byte ptr [esi+3]
    lea ecx, [edi + ecx * (1 << (Q_HASH_SH1 - Q_HASH_SH2))]
    inc esi
    lea ecx, [eax + ecx * (1 << Q_HASH_SH2)]
    mov Q_HTABLE (ecx), esi
    inc esi

        pop     TAG

        cmp     esi, V.orig.ptr_stop    // processed everything?
        jae     pass1_stop              // yes, stop

        movzx   ecx, byte ptr [esi+2]
        lea eax, [eax + edi * (1 << (Q_HASH_SH1 - Q_HASH_SH2))]
        lea eax, [ecx + eax * (1 << Q_HASH_SH2)]

#if MAX_OFFSET < BUFF_SIZE_LOG
        lea     ecx, [esi - (1 << MAX_OFFSET) + 1] // min. allowed left bound
#endif
        mov     edi, Q_HTABLE (eax)     // edi = candidate ptr
        mov     Q_HTABLE (eax), esi     // save current ptr

        jmp     check_candidate         // process next substring

insert_all_rest:
    lea eax, [ecx + eax * (1 << (Q_HASH_SH1 - Q_HASH_SH2))]
    lea eax, [edi + eax * (1 << Q_HASH_SH2)]
    mov Q_HTABLE (eax), esi
    inc esi
    cmp esi, TAG
    je  inserted_all

    movzx   eax, byte ptr [esi+2]
    lea ecx, [edi + ecx * (1 << (Q_HASH_SH1 - Q_HASH_SH2))]
    lea ecx, [eax + ecx * (1 << Q_HASH_SH2)]
    mov Q_HTABLE (ecx), esi
    inc esi

inserted_all:
        pop     TAG
        jmp     write_literal_entry     // process next substring


    ALIGN (16)
insert_all_3_entry:
    push    OUT
insert_all_3:
    lea OUT, [ecx + eax * (1 << (Q_HASH_SH1 - Q_HASH_SH2))]
    movzx   eax, byte ptr [esi+3]
    lea OUT, [edi + OUT * (1 << Q_HASH_SH2)]
    mov Q_HTABLE (OUT), esi
    inc esi

    lea OUT, [edi + ecx * (1 << (Q_HASH_SH1 - Q_HASH_SH2))]
    movzx   ecx, byte ptr [esi+3]
    lea OUT, [eax + OUT * (1 << Q_HASH_SH2)]
    mov Q_HTABLE (OUT), esi
    inc esi

    lea OUT, [eax + edi * (1 << (Q_HASH_SH1 - Q_HASH_SH2))]
    movzx   edi, byte ptr [esi+3]
    lea OUT, [ecx + OUT * (1 << Q_HASH_SH2)]
    mov Q_HTABLE (OUT), esi
    inc esi

    cmp esi, TAG
    jbe insert_all_3

    add TAG, 3
    pop OUT
    cmp esi, TAG
    jne insert_all_rest

        pop     TAG
        jmp     write_literal_entry     // process next substring

    ALIGN (16)
#else

        push    OUT
        mov     OUT, eax                // eax = end-of-match
insert_all:
        Q_HASH_SUM_ASM ()               // evaluate hash sum
        mov     Q_HTABLE (eax), esi     // save current ptr
        inc     esi                     // shift to next position
        cmp     esi, OUT                // inserted all substrings in the match?
        jne     insert_all              // continue until finished
        pop     OUT
        jmp     write_literal_entry     // process next substring

    ALIGN (16)
#endif

#else /* CHAIN == 1 */

        push    OUT
        push    TAG
        mov     OUT, esi                // OUT = current ptr
        mov TAG, eax                 // save end-of-match
        sub     OUT, V.orig.ptr         // OUT = current ptr offset
insert_all:
        Z_HASH_SUM_ASM ()               // evaluate hash sum
        inc     esi                     // shift to next position
        mov     Z_HTABLE (eax), bx      // save current offset
        inc     OUT                     // increase offset
        cmp     esi, TAG                // inserted all substrings in the match?
        jne     insert_all              // continue until finished
        pop     TAG
        pop     OUT
        jmp     write_literal_entry     // process next substring

    ALIGN (16)
#endif /* CHAIN */

length_4:
#define KNOWN_LENGTH    4               // we know that first 4 bytes match

#if (CODING == CODING_DIRECT2 && DIRECT2_MAX_LEN + MIN_MATCH >= 8) || (CODING == CODING_HUFF_ALL && 4 - MIN_MATCH < MAX_LENGTH - 1)
        mov     eax, [esi+4]            // fetch next 4 bytes
        sub     eax, [edi+4]            // get the diff between src and candidate
        jz      length_8                // do long compare if 8+ bytes match

        bsf     ecx, eax                // ecx = # of first non-zero bit

#if CHAIN >= 3
        shr     ecx, 3                  // ecx = # of first non-zero byte

    add ecx, 4
    cmp ecx, V.match.len
    jbe continue_search

    mov V.match.len, ecx
    mov V.match.pos, edi
    jmp continue_search

#else /* CHAIN >= 3 */

        sub     edi, esi                // edi = -offset
        shr     ecx, 3                  // ecx = # of first non-zero byte

#if CODING == CODING_HUFF_ALL
    add ecx, 4
    jmp write_small_ptr
#else
        not     edi                     // edi = offset-1
        add     ecx, 4                  // plus previous 4 matching bytes = match length
        add     OUT, 2                  // adjust output ptr in advance
        lea     eax, [esi+ecx]          // eax = end of src match
        shl     edi, DIRECT2_LEN_LOG    // make room for length
        inc     esi                     // esi = next substring (current already inserted)
        lea     edi,[edi+ecx-MIN_MATCH] // combine offset and shoft length
        stc                             // set carry bit
        mov     [OUT-2], di             // save packed pointer

        adc     TAG, TAG                // write tag bit 1
        jnc     write_pointer_tag_done  // write tag word when it is full

        WRITE_TAG_MASK ()
        jmp     write_pointer_tag_done
#endif /* CODING == CODING_HUFF_ALL */
#endif /* CHAIN >= 3 */

length_8:
#undef  KNOWN_LENGTH
#define KNOWN_LENGTH    8               // we know that first 8 bytes match
#endif /* DIRECT2_MAX_LEN + MIN_MATCH >= 8 */

        mov     eax, esi                // eax = beginning of the string
        mov     ecx, V.orig.end         // ecx = end of buffer
        add     esi, KNOWN_LENGTH       // shift to first untested src byte
        add     edi, KNOWN_LENGTH       // shift to first untested candidate
        sub     ecx, esi                // ecx = max compare length

        rep     cmpsb                   // compare src and candidate
        je      match_complete          // if eq then match till end of buffer
match_complete_done:

        lea     ecx, [esi-1]            // ecx = end of match

#if CHAIN >= 3

    cmp esi, V.orig.end     // if no data left in buffer, stop search
    ja  stop_search

    mov esi, eax        // esi = src ptr
    dec edi         // adjust for rep cmpsb behaviour
    sub ecx, eax        // ecx = match length
    sub edi, ecx        // edi = candidate
    cmp ecx, V.match.len
    jbe continue_search

    mov V.match.len, ecx
    mov V.match.pos, edi
    jmp continue_search

stop_search:
    mov esi, eax        // esi = src ptr
    dec edi         // adjust for rep cmpsb behaviour
    sub ecx, eax        // ecx = match length
    sub edi, ecx        // edi = candidate
#if DEBUG
    cmp ecx, V.match.len    // it should be longest match
    ja  last_length_ok
    int 3
last_length_ok:
#endif /* DEBUG */

    mov V.match.len, ecx
    mov V.match.pos, edi
    jmp write_pointer

    ALIGN (16)

write_pointer:
    cmp V.match.len, MIN_MATCH-1
    jbe write_literal

    mov edi, V.match.pos
    mov ecx, V.match.len
    sub edi, esi        // edi = -offset

#else /* CHAIN >= 3 */

        sub     edi, esi                // edi = -offset
        sub     ecx, eax                // ecx = match length
        mov     esi, eax                // esi = src ptr

#endif /* CHAIN >= 3 */

#if CODING == CODING_HUFF_ALL

    cmp ecx, MAX_LENGTH + MIN_MATCH - 1
    jb  write_small_ptr

    neg edi         // edi = offset
    sub ecx, MAX_LENGTH + MIN_MATCH - 1 // ecx = output length
    bsr eax, edi        // eax = most significant bit number of offset
    add V.stat.pointers, 3  // have 2 more pointer bytes
    sub edi, bit_mask_table[eax*4]      // edi = remaining offset bits
    add V.stat.extra, eax   // have "eax" more extra bits to encode
    shl eax, MAX_LENGTH_LOG // prepare output byte mask

    add OUT, 4          // adjust output pointer
    add eax, MAX_LENGTH-1   // eax = (log_offset : MAX_LENGTH-1) packed
    inc dword ptr V.stat.freq[eax*4+256*4]  // increment respective frequency count
    mov [OUT-4], al     // write packed (log_offset : MAX_LENGTH-1)

    mov [OUT-3], cl     // write actual length
    cmp ecx, 255
    jb  wrote_length

    add OUT, 2          // adjust output pointer in advance
    add ecx, MAX_LENGTH-1
    mov byte ptr [OUT-5], 255   // mark it as long length
    mov [OUT-4], cx     // write actual length
    sub ecx, MAX_LENGTH-1
wrote_length:

    add ecx, MAX_LENGTH + MIN_MATCH - 1 // restore length

    jmp write_offset

write_pointer_tag_new:                  // write tag word and return to pointers
        WRITE_TAG_MASK ()
        jmp     write_pointer_tag_done

#else /* CODING == CODING_DIRECT2 */

        cmp     ecx, DIRECT2_MAX_LEN+MIN_MATCH  // small length?
        jb      write_small_ptr         // write ptr if so

        not     edi                     // edi = offset-1
        lea     eax, [esi+ecx]          // eax = end of match
        shl     edi, DIRECT2_LEN_LOG    // make room for length
        sub     ecx, DIRECT2_MAX_LEN+MIN_MATCH  // decrease the length
        add     edi, DIRECT2_MAX_LEN    // mark length as long
        push    eax                     // save end of match
        mov     [OUT], di               // write packed pointer

        mov     al, cl                  // al = (ecx <= 15 ? cl : 15)
        cmp     ecx, 15
        jbe     match_less_15
        mov     al, 15
match_less_15:

        mov     edi, V.stat.ptr         // edi = quad_ptr
        add     OUT, 2                  // wrote 2 bytes, move output ptr

        test    edi, edi                // if quad_ptr != NULL write upper 4 bits
        jne     match_have_ptr
        mov     V.stat.ptr, OUT         // make new tag_ptr
        mov     [OUT], al               // write lower 4 bits
        inc     OUT                     // wrote 1 byte, move output ptr
        jmp     match_done_ptr          // continue execution

match_have_ptr:
        shl     al, 4                   // will write into upper 4 bits
        mov     dword ptr V.stat.ptr, 0 // no more space in this quad_bit[0]
        or      [edi], al               // write upper 4 bits
match_done_ptr:
        sub     ecx, 15                 // adjusted length < 15?
        jae     match_long_long_length  // if not continue encoding
match_finish_2:
        inc     esi                     // shift to next output position
        pop     eax                     // restore eax = end-of-match
        stc                             // set carry flag
        adc     TAG, TAG                // write tag bit 1
        jnc     write_pointer_tag_done  // continue execution if do not need to flush

write_pointer_tag_new:                  // write tag word and return to pointers
        WRITE_TAG_MASK ()
        jmp     write_pointer_tag_done

match_long_long_length:
        mov     [OUT], cl               // write the length as a byte
        inc     OUT                     // move output ptr
        cmp     ecx, 255                // adjusted length fits in byte?
        jb      match_finish_2          // if so ptr is written

        add     ecx, DIRECT2_MAX_LEN+15 // restore full length - MIN_MATCH
        mov     byte ptr [OUT-1], 255   // mark byte length as "to be continued"
        mov     [OUT], cx               // write full length
        add     OUT, 2                  // move output ptr
        jmp     match_finish_2

#endif /* CODING */

write_literal_tag_new:                  // write tag word and return to literals
        WRITE_TAG_MASK ()
        jmp     write_literal_tag_done

match_complete:                         // cmpsb compared till end of buffer
        inc     esi                     // increase esi
        inc     edi                     // increase edi
        jmp     match_complete_done     // resume execution

#if CHAIN < 2

    ALIGN (16)
insert_tail:
        push    OUT
        push    eax                     // save end-of-match
        mov OUT, V.orig.end_3
        jmp     insert_tail_1

insert_tail_next:

#if CHAIN <= 0

        Q_HASH_SUM_ASM ()               // evaluate hash sum
        mov     Q_HTABLE (eax), esi     // insert current src pointer

#else
        Z_HASH_SUM_ASM ()               // evaluate hash sum
        mov     ecx, esi
        sub     ecx, V.orig.ptr         // ecx = current ptr offset

        mov     Z_HTABLE (eax), cx      // save offset in hash table

#endif /* CHAIN <= 0 */

        inc     esi                     // and move it to next substring
insert_tail_1:                          // end of match exceeds end_3 -- be careful
        cmp     esi, OUT                // inserted up to end_3?
        jb      insert_tail_next        // if not continue
        pop     esi                     // esi = end of match
        pop OUT         // restore OUT
        jmp     write_literal_entry

#endif /* CHAIN < 2 */

pass1_stop:
        mov     V.temp.ptr, OUT         // save register variables
        mov     V.temp.tag_mask, TAG
        sub     esi, V.orig.ptr
        mov     V.orig.pos, esi

        pop     ebp                     // restore ebp
} /* __asm */
}

#undef V
#undef PRS
#undef TAG
#undef TAGW
#undef Q_HTABLE
#undef Q_HASH_SUM_ASM
#undef Z_HTABLE
#undef Z_NEXT
#undef Z_HASH_SUM_ASM
#undef KNOWN_LENGTH
#endif /* CODING != CODING_DIRECT2 */
#endif /* CHAIN < 2 */


#undef CHAIN
#undef find_match
#undef encode_pass1
