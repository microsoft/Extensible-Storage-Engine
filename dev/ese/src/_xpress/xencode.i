// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef ALIGN
#if DEBUG
#define ALIGN(n)
#else
#define ALIGN(n) align n
#endif 
#endif 

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
#endif 
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
#endif 

#else 

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
#endif 
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
#endif 

#endif 

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
#endif 
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
#endif 
        {
          v.match.len = 3;
          v.match.pos = k;
#if CHAIN < 3
          return (1);
#endif 
        }
#endif 

        goto cont;
      }
#else 
      if (p1[0] != p2[0] || p1[1] != p2[1]
#if CHAIN >= 3
        || p1[2] != p2[2]
#endif 
      )
        goto cont;
      if (p1[3] != p2[3])
      {
#if MIN_MATCH <= 3
        assert (p1 + 3 <= v.orig.end);
#if CHAIN >= 3
    if (v.match.len <= 2)
#endif 
        {
          v.match.len = 3;
          v.match.pos = k;
#if CHAIN < 3
          return (1);
#endif 
        }
#endif 
        goto cont;
      }
#endif 

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
#endif 
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
#endif 
  }
}


static void encode_pass1 (prs *p)
{
  uchar *ptr = v.temp.ptr;
#if CHAIN < 3
  v.match.len = MIN_MATCH-1;
#endif 
  do
  {
    if (p->x.z_next[v.orig.pos] == 0)
      goto literal;
#if CHAIN >= 3
    v.match.len = MIN_MATCH-1;
#endif 
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
#endif 
    }
  }
  while (v.orig.pos < v.orig.stop);
  v.temp.ptr = ptr;
}

#endif 


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

#else 

#ifndef BITMASK_TABLE_PRESENT
#define BITMASK_TABLE_PRESENT
static const int32 bit_mask_table[] = {
  0x0001, 0x0002, 0x0004, 0x0008,
  0x0010, 0x0020, 0x0040, 0x0080,
  0x0100, 0x0200, 0x0400, 0x0800,
  0x1000, 0x2000, 0x4000, 0x8000,
  0x10000
};
#endif 


static void encode_pass1 (prs *PrsPtr)
{

#define PRS     edx
#define OUT ebx
#define TAG     ebp
#define TAGW    bp

#define V               [PRS - SIZE prs] prs.c

#if CODING == CODING_HUFF_ALL
#define INC_MASKS_ASM   __asm   inc dword ptr V.stat.masks
#else
#define INC_MASKS_ASM
#endif 

#define WRITE_TAG_MASK()                        \
        __asm   mov     ecx, V.temp.tag_ptr     \
        __asm   mov     V.temp.tag_ptr, OUT     \
        __asm   add     OUT, 4                  \
        __asm   mov     [ecx], TAG              \
        INC_MASKS_ASM                           \
        __asm   mov     TAG, 1

#if CHAIN <= 0

#define Q_HTABLE(idx)   dword ptr [PRS + idx*4] prs.x.q_last

#define Q_HASH_SUM_ASM()                                                    \
        __asm   movzx   ecx, byte ptr [esi]                                 \
        __asm   movzx   edi, byte ptr [esi+1]                               \
        __asm   movzx   eax, byte ptr [esi+2]                               \
        __asm   lea     eax, [eax + ecx * (1 << Q_HASH_SH1)]                \
        __asm   lea     eax, [eax + edi * (1 << Q_HASH_SH2)]                \

#else

#define Z_NEXT(idx) word ptr [PRS + idx*2] prs.x.z_next

#if CHAIN >= 2
#define Z_HTABLE(idx)   word ptr [PRS + idx*2 - SIZE prs] prs.x.z_hash
#else
#define Z_HTABLE(idx)   word ptr [PRS + idx*2] prs.x.z_next
#endif 

#define Z_HASH_SUM_ASM()                                                    \
        __asm   movzx   eax, byte ptr [esi]                                 \
        __asm   movzx   ecx, byte ptr [esi+1]                               \
        __asm   movzx   edi, byte ptr [esi+2]                               \
        __asm   movzx   eax, word ptr z_hash_map[eax*2]                     \
        __asm   movzx   ecx, word ptr z_hash_map[ecx*2][512]                \
        __asm   movzx   edi, word ptr z_hash_map[edi*2][1024]               \
        __asm   xor     eax, ecx                                            \
        __asm   xor     eax, edi

#endif 

__asm
{
        push    TAG

        mov     PRS, PrsPtr


        mov     esi, V.orig.ptr
        mov     eax, V.orig.stop
        add     eax, esi
        mov     V.orig.ptr_stop, eax
        add     esi, V.orig.pos
        mov     OUT, V.temp.ptr
        mov     TAG, V.temp.tag_mask

        cmp     esi, V.orig.ptr
        jne     write_literal_entry

#if CHAIN >= 2
    jmp write_literal
#endif 

        ALIGN (16)
#if CHAIN >= 2
write_literal_0:
    add esi, ecx
#endif 

write_literal:

#if CODING == CODING_HUFF_ALL
    movzx   eax, [esi]

#if CHAIN == 2
write_literal_1:
#endif 

        inc     OUT
        inc     esi
        inc dword ptr V.stat.freq[eax*4]
        mov     [OUT-1], al
#else
        mov     al, [esi]

#if CHAIN == 2
write_literal_1:
#endif 

        inc     OUT
        inc     esi
        mov     [OUT-1], al
#endif 

        add     TAG, TAG
        jc      write_literal_tag_new
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

        cmp     esi, V.orig.ptr_stop
        jae     pass1_stop


#if CHAIN <= 0
        Q_HASH_SUM_ASM ()
#if MAX_OFFSET < BUFF_SIZE_LOG
        lea     ecx, [esi - (1 << MAX_OFFSET) + 1]
#endif
        mov     edi, Q_HTABLE (eax)
        mov     Q_HTABLE (eax), esi
check_candidate:
#else

#if CHAIN >= 2
        mov     ecx,V.orig.ptr
        sub     esi, ecx
        movzx   edi, Z_NEXT (esi)
#else
        Z_HASH_SUM_ASM ()
        mov     ecx,V.orig.ptr
        movzx   edi, Z_HTABLE (eax)
        sub     esi, ecx
#endif 

#if CHAIN >= 2
    test    edi, edi
    jz  write_literal_0

    mov ax, [ecx + esi + 1]

#if CHAIN >= 3
    mov Z_NEXT (0), si
    mov V.match.len, MIN_MATCH-1
#endif 

    cmp ax, [ecx + edi + 1]
    je  CheckMatch_0

#if CHAIN < 3
    mov Z_NEXT (0), si
#endif 

    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi + 1]
    je  CheckMatch

    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi + 1]
    je  CheckMatch

    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi + 1]
    je  CheckMatch

    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi + 1]
    je  CheckMatch

    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi + 1]
    je  CheckMatch

    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi + 1]
    je  CheckMatch

#if CHAIN == 2
    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi + 1]
    je  CheckMatch
#endif 

    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi + 1]

#if CHAIN == 2
    jne write_literal_0
#else
    je  CheckMatch

        add esi, ecx
continue_search_0:
    mov eax, V.chain
        add     edi, ecx
        dec eax
        mov V.temp.chain, eax
    jmp continue_search
#endif 

    ALIGN (16)
CheckMatch:
    cmp edi, esi
    je  write_literal_0

CheckMatch_0:

#if CHAIN == 2 && CODING == CODING_HUFF_ALL
    movzx   eax, [esi + ecx]
#else
    mov al, [esi + ecx]
#endif 
    add esi, ecx
    cmp al, [edi + ecx]

#if CHAIN >= 3
    jne continue_search_0
    mov eax, V.chain
        add     edi, ecx
        dec eax
        mov V.temp.chain, eax
#else
    jne write_literal_1
        add     edi, ecx
#endif 

#else 

        add     edi, ecx
        mov     Z_HTABLE (eax), si
        add     esi, ecx

#endif 

#if MAX_OFFSET < BUFF_SIZE_LOG
        lea     ecx, [esi - (1 << MAX_OFFSET) + 1]
#endif
#endif 

#if MAX_OFFSET < BUFF_SIZE_LOG
        cmp     edi, ecx
#if CHAIN <= 2
        js      write_literal
#else
        js      continue_search_1
#endif 
#endif 

#if CHAIN <= 0 && FILL_NULL
        test    edi, edi
        jz      write_literal
#endif 

#if CHAIN >= 2
    mov al, [esi + 3]
    cmp al, [edi + 3]
    je  length_4
#else
        mov     eax, [esi]
        sub     eax, [edi]
        je      length_4

        test    eax, 0xffffff
        jne     write_literal
#endif 

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
#endif 
    mov ecx, V.match.len
    mov ax, [esi + ecx - 1]
    add ecx, V.orig.ptr
    sub edi, V.orig.ptr
    dec ecx

continue_search_loop:
    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    movzx   edi, Z_NEXT (edi)
    cmp ax, [ecx + edi]
    je  continue_CheckMatch

    movzx   edi, Z_NEXT (edi)
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
        lea     eax, [esi - (1 << MAX_OFFSET) + 1]
        cmp     edi, eax
        js      write_pointer
#endif 

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

#else 

        mov     ecx, 3
        sub     edi, esi

#endif 

write_small_ptr:
#if CODING == CODING_HUFF_ALL
    neg edi
    bsr eax, edi
    add V.stat.pointers, 3
    sub edi, bit_mask_table[eax*4]
    add V.stat.extra, eax
    shl eax, MAX_LENGTH_LOG
    add OUT, 3
    lea eax, [eax + ecx - MIN_MATCH]
    inc dword ptr V.stat.freq[eax*4+256*4]
    mov [OUT-3], al

write_offset:
    cmp eax, (9 << MAX_LENGTH_LOG)
    sbb eax, eax
    mov [OUT-2], di
    add OUT, eax
    add V.stat.pointers, eax
    stc
    lea eax, [esi+ecx]
#else
        lea     eax, [esi+ecx]
        not     edi
        add     OUT, 2
        shl     edi, DIRECT2_LEN_LOG
        inc     esi
        lea     edi, [edi + ecx - MIN_MATCH]
        stc
        mov     [OUT-2], di
#endif

        adc     TAG, TAG
        jc      write_pointer_tag_new
write_pointer_tag_done:

#if CHAIN < 2
        cmp     eax, V.orig.end_3
        ja      insert_tail
#endif 

#if CHAIN >= 2

    mov esi, eax
    jmp write_literal_entry

#elif CHAIN <= 0

#if 1
        push    TAG
        lea     TAG, [eax-3]

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

        cmp     esi, V.orig.ptr_stop
        jae     pass1_stop

        movzx   ecx, byte ptr [esi+2]
        lea eax, [eax + edi * (1 << (Q_HASH_SH1 - Q_HASH_SH2))]
        lea eax, [ecx + eax * (1 << Q_HASH_SH2)]

#if MAX_OFFSET < BUFF_SIZE_LOG
        lea     ecx, [esi - (1 << MAX_OFFSET) + 1]
#endif
        mov     edi, Q_HTABLE (eax)
        mov     Q_HTABLE (eax), esi

        jmp     check_candidate

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
        jmp     write_literal_entry


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
        jmp     write_literal_entry

    ALIGN (16)
#else

        push    OUT
        mov     OUT, eax
insert_all:
        Q_HASH_SUM_ASM ()
        mov     Q_HTABLE (eax), esi
        inc     esi
        cmp     esi, OUT
        jne     insert_all
        pop     OUT
        jmp     write_literal_entry

    ALIGN (16)
#endif

#else 

        push    OUT
        push    TAG
        mov     OUT, esi
        mov TAG, eax
        sub     OUT, V.orig.ptr
insert_all:
        Z_HASH_SUM_ASM ()
        inc     esi
        mov     Z_HTABLE (eax), bx
        inc     OUT
        cmp     esi, TAG
        jne     insert_all
        pop     TAG
        pop     OUT
        jmp     write_literal_entry

    ALIGN (16)
#endif 

length_4:
#define KNOWN_LENGTH    4

#if (CODING == CODING_DIRECT2 && DIRECT2_MAX_LEN + MIN_MATCH >= 8) || (CODING == CODING_HUFF_ALL && 4 - MIN_MATCH < MAX_LENGTH - 1)
        mov     eax, [esi+4]
        sub     eax, [edi+4]
        jz      length_8

        bsf     ecx, eax

#if CHAIN >= 3
        shr     ecx, 3

    add ecx, 4
    cmp ecx, V.match.len
    jbe continue_search

    mov V.match.len, ecx
    mov V.match.pos, edi
    jmp continue_search

#else 

        sub     edi, esi
        shr     ecx, 3

#if CODING == CODING_HUFF_ALL
    add ecx, 4
    jmp write_small_ptr
#else
        not     edi
        add     ecx, 4
        add     OUT, 2
        lea     eax, [esi+ecx]
        shl     edi, DIRECT2_LEN_LOG
        inc     esi
        lea     edi,[edi+ecx-MIN_MATCH]
        stc
        mov     [OUT-2], di

        adc     TAG, TAG
        jnc     write_pointer_tag_done

        WRITE_TAG_MASK ()
        jmp     write_pointer_tag_done
#endif 
#endif 

length_8:
#undef  KNOWN_LENGTH
#define KNOWN_LENGTH    8
#endif 

        mov     eax, esi
        mov     ecx, V.orig.end
        add     esi, KNOWN_LENGTH
        add     edi, KNOWN_LENGTH
        sub     ecx, esi

        rep     cmpsb
        je      match_complete
match_complete_done:

        lea     ecx, [esi-1]

#if CHAIN >= 3

    cmp esi, V.orig.end
    ja  stop_search

    mov esi, eax
    dec edi
    sub ecx, eax
    sub edi, ecx
    cmp ecx, V.match.len
    jbe continue_search

    mov V.match.len, ecx
    mov V.match.pos, edi
    jmp continue_search

stop_search:
    mov esi, eax
    dec edi
    sub ecx, eax
    sub edi, ecx
#if DEBUG
    cmp ecx, V.match.len
    ja  last_length_ok
    int 3
last_length_ok:
#endif 

    mov V.match.len, ecx
    mov V.match.pos, edi
    jmp write_pointer

    ALIGN (16)

write_pointer:
    cmp V.match.len, MIN_MATCH-1
    jbe write_literal

    mov edi, V.match.pos
    mov ecx, V.match.len
    sub edi, esi

#else 

        sub     edi, esi
        sub     ecx, eax
        mov     esi, eax

#endif 

#if CODING == CODING_HUFF_ALL

    cmp ecx, MAX_LENGTH + MIN_MATCH - 1
    jb  write_small_ptr

    neg edi
    sub ecx, MAX_LENGTH + MIN_MATCH - 1
    bsr eax, edi
    add V.stat.pointers, 3
    sub edi, bit_mask_table[eax*4]
    add V.stat.extra, eax
    shl eax, MAX_LENGTH_LOG

    add OUT, 4
    add eax, MAX_LENGTH-1
    inc dword ptr V.stat.freq[eax*4+256*4]
    mov [OUT-4], al

    mov [OUT-3], cl
    cmp ecx, 255
    jb  wrote_length

    add OUT, 2
    add ecx, MAX_LENGTH-1
    mov byte ptr [OUT-5], 255
    mov [OUT-4], cx
    sub ecx, MAX_LENGTH-1
wrote_length:

    add ecx, MAX_LENGTH + MIN_MATCH - 1

    jmp write_offset

write_pointer_tag_new:
        WRITE_TAG_MASK ()
        jmp     write_pointer_tag_done

#else 

        cmp     ecx, DIRECT2_MAX_LEN+MIN_MATCH
        jb      write_small_ptr

        not     edi
        lea     eax, [esi+ecx]
        shl     edi, DIRECT2_LEN_LOG
        sub     ecx, DIRECT2_MAX_LEN+MIN_MATCH
        add     edi, DIRECT2_MAX_LEN
        push    eax
        mov     [OUT], di

        mov     al, cl
        cmp     ecx, 15
        jbe     match_less_15
        mov     al, 15
match_less_15:

        mov     edi, V.stat.ptr
        add     OUT, 2

        test    edi, edi
        jne     match_have_ptr
        mov     V.stat.ptr, OUT
        mov     [OUT], al
        inc     OUT
        jmp     match_done_ptr

match_have_ptr:
        shl     al, 4
        mov     dword ptr V.stat.ptr, 0
        or      [edi], al
match_done_ptr:
        sub     ecx, 15
        jae     match_long_long_length
match_finish_2:
        inc     esi
        pop     eax
        stc
        adc     TAG, TAG
        jnc     write_pointer_tag_done

write_pointer_tag_new:
        WRITE_TAG_MASK ()
        jmp     write_pointer_tag_done

match_long_long_length:
        mov     [OUT], cl
        inc     OUT
        cmp     ecx, 255
        jb      match_finish_2

        add     ecx, DIRECT2_MAX_LEN+15
        mov     byte ptr [OUT-1], 255
        mov     [OUT], cx
        add     OUT, 2
        jmp     match_finish_2

#endif 

write_literal_tag_new:
        WRITE_TAG_MASK ()
        jmp     write_literal_tag_done

match_complete:
        inc     esi
        inc     edi
        jmp     match_complete_done

#if CHAIN < 2

    ALIGN (16)
insert_tail:
        push    OUT
        push    eax
        mov OUT, V.orig.end_3
        jmp     insert_tail_1

insert_tail_next:

#if CHAIN <= 0

        Q_HASH_SUM_ASM ()
        mov     Q_HTABLE (eax), esi

#else
        Z_HASH_SUM_ASM ()
        mov     ecx, esi
        sub     ecx, V.orig.ptr

        mov     Z_HTABLE (eax), cx

#endif 

        inc     esi
insert_tail_1:
        cmp     esi, OUT
        jb      insert_tail_next
        pop     esi
        pop OUT
        jmp     write_literal_entry

#endif 

pass1_stop:
        mov     V.temp.ptr, OUT
        mov     V.temp.tag_mask, TAG
        sub     esi, V.orig.ptr
        mov     V.orig.pos, esi

        pop     ebp
} 
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
#endif 
#endif 


#undef CHAIN
#undef find_match
#undef encode_pass1
