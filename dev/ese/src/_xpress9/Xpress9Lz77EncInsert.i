// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#if LZ77_MIN_PTR_MATCH_LENGTH == 4 && XPRESS9_USE_SSE2 && defined (Xpress9Lz77EncInsert_SSE2)

static
void
Xpress9Lz77EncInsert_SSE2 (
    LZ77_PASS1_STATE   *pState
)
{
    LZ77_INDEX     *pHashTable;
    const UInt8    *pData;
    uxint           uPosition;
    uxint           uLastPosition;
    uxint           uHashValue;
    uxint           uHashMask;

    pHashTable = STATE.m_HashTable.m_pHashTable;
    pData = STATE.m_EncodeData.m_pData;
    uPosition = STATE.m_EncodeData.m_uHashInsertPosition;
    uLastPosition = STATE.m_EncodeData.m_uDataSize;

    ASSERT (uLastPosition >= uPosition, "uLastPosition=%Iu uPosition=%Iu", uLastPosition, uPosition);

    if (uLastPosition >= 4)
    {
        uHashMask = STATE.m_HashTable.m_uHashTableSizeCurrent - 1;

        pData += uPosition;
        uLastPosition -= 4;

        while (uPosition < uLastPosition && (((uxint) pData) & 15) != 0)
        {
            uHashValue = * (__unaligned UInt32 *) pData;

            uHashValue = (uHashValue ^ 0xDEADBEEFu) + (uHashValue >> 5);
            uHashValue ^= uHashValue >> 11;
            uHashValue ^= uHashValue >> 17;

            uHashValue &= uHashMask;
            pState->m_uNext[uPosition] = pHashTable[uHashValue];
            pHashTable[uHashValue] = (LZ77_INDEX) uPosition;

            uPosition += 1;
            pData += 1;
        }

        if (uLastPosition >= 32 && uLastPosition - 32 >= uPosition)
        {
            __m128i v0, v1, v2, v3, t0, t1, t2, t3, vt;
            __m128i r0, r1, r2, r3;
            uxint   h0, h1, h2, h3;
            __m128i s_DeadBeef =
            {
                0xEF, 0xBE, 0xAD, 0xDE,
                0xEF, 0xBE, 0xAD, 0xDE,
                0xEF, 0xBE, 0xAD, 0xDE,
                0xEF, 0xBE, 0xAD, 0xDE,
            };
            __m128i mt, mask;

            mt.m128i_u32[0] = (UInt32) uHashMask;
            mt.m128i_u32[1] = (UInt32) uHashMask;
            mt.m128i_u32[2] = (UInt32) uHashMask;
            mt.m128i_u32[3] = (UInt32) uHashMask;
            mask = mt;

            uLastPosition -= 32;
            do
            {
                v0 = _mm_load_si128 ((__m128i *) pData);
                vt = _mm_load_si128 ((__m128i *) (pData + 16));

                v1 = _mm_srli_si128 (v0, 1);
                t1 = _mm_slli_si128 (vt, 15);
                v1 = _mm_or_si128 (v1, t1);

                v2 = _mm_srli_si128 (v0, 2);
                t2 = _mm_slli_si128 (vt, 14);
                v2 = _mm_or_si128 (v2, t2);

                v3 = _mm_srli_si128 (v0, 3);
                t3 = _mm_slli_si128 (vt, 13);
                v3 = _mm_or_si128 (v3, t3);

                t0 = _mm_srli_epi32 (v0, 5);
                v0 = _mm_xor_si128 (v0, s_DeadBeef);
                v0 = _mm_add_epi32 (v0, t0);
                t0 = v0;
                v0 = _mm_srli_epi32 (v0, 11);
                v0 = _mm_xor_si128 (v0, t0);
                t0 = v0;
                v0 = _mm_srli_epi32 (v0, 17);
                v0 = _mm_xor_si128 (v0, t0);
                v0 = _mm_and_si128 (v0, mask);
                _mm_store_si128 (&r0, v0);

                t1 = _mm_srli_epi32 (v1, 5);
                v1 = _mm_xor_si128 (v1, s_DeadBeef);
                v1 = _mm_add_epi32 (v1, t1);
                t1 = v1;
                v1 = _mm_srli_epi32 (v1, 11);
                v1 = _mm_xor_si128 (v1, t1);
                t1 = v1;
                v1 = _mm_srli_epi32 (v1, 17);
                v1 = _mm_xor_si128 (v1, t1);
                v1 = _mm_and_si128 (v1, mask);
                _mm_store_si128 (&r1, v1);

                t2 = _mm_srli_epi32 (v2, 5);
                v2 = _mm_xor_si128 (v2, s_DeadBeef);
                v2 = _mm_add_epi32 (v2, t2);
                t2 = v2;
                v2 = _mm_srli_epi32 (v2, 11);
                v2 = _mm_xor_si128 (v2, t2);
                t2 = v2;
                v2 = _mm_srli_epi32 (v2, 17);
                v2 = _mm_xor_si128 (v2, t2);
                v2 = _mm_and_si128 (v2, mask);
                _mm_store_si128 (&r2, v2);

                t3 = _mm_srli_epi32 (v3, 5);
                v3 = _mm_xor_si128 (v3, s_DeadBeef);
                v3 = _mm_add_epi32 (v3, t3);
                t3 = v3;
                v3 = _mm_srli_epi32 (v3, 11);
                v3 = _mm_xor_si128 (v3, t3);
                t3 = v3;
                v3 = _mm_srli_epi32 (v3, 17);
                v3 = _mm_xor_si128 (v3, t3);
                v3 = _mm_and_si128 (v3, mask);
                _mm_store_si128 (&r3, v3);

                #define ROUND 0
                #include "Xpress9Amd64Insert.i"

                #define ROUND 1
                #include "Xpress9Amd64Insert.i"

                #define ROUND 2
                #include "Xpress9Amd64Insert.i"

                #define ROUND 3
                #include "Xpress9Amd64Insert.i"

                uPosition += 16;
                pData += 16;
            }
            while (uPosition < uLastPosition);

            uLastPosition += 32;
        }

        while (uPosition < uLastPosition)
        {
            uHashValue = * (__unaligned UInt32 *) pData;

            uHashValue = (uHashValue ^ 0xDEADBEEFu) + (uHashValue >> 5);
            uHashValue ^= uHashValue >> 11;
            uHashValue ^= uHashValue >> 17;

            uHashValue &= uHashMask;
            pState->m_uNext[uPosition] = pHashTable[uHashValue];
            pHashTable[uHashValue] = (LZ77_INDEX) uPosition;

            uPosition += 1;
            pData += 1;
        }

        uLastPosition += 4;

        STATE.m_EncodeData.m_uHashInsertPosition = uPosition;
    }

    while (uPosition < uLastPosition)
    {
        pState->m_uNext[uPosition] = 0;
        uPosition += 1;
    }
}

#else

static
void
Xpress9Lz77EncInsert (
    LZ77_PASS1_STATE   *pState
)
{
    LZ77_INDEX     *pHashTable;
    const UInt8    *pData;
    uxint           uPosition;
    uxint           uLastPosition;
    uxint           uHashValue;
    uxint           uHashMask;

    pHashTable = STATE.m_HashTable.m_pHashTable;
    pData = STATE.m_EncodeData.m_pData;
    uPosition = STATE.m_EncodeData.m_uHashInsertPosition;
    uLastPosition = STATE.m_EncodeData.m_uDataSize;

    ASSERT (uLastPosition >= uPosition, "uLastPosition=%Iu uPosition=%Iu", uLastPosition, uPosition);

    if (uLastPosition >= 4)
    {
        uHashMask = STATE.m_HashTable.m_uHashTableSizeCurrent - 1;

        pData += uPosition;
        uLastPosition -= 4;
        while (uPosition < uLastPosition)
        {
#if LZ77_MIN_PTR_MATCH_LENGTH == 4
            uHashValue = * (__unaligned UInt32 *) pData;
            uHashValue = (uHashValue ^ 0xDEADBEEFu) + (uHashValue >> 5);
            uHashValue ^= uHashValue >> 11;
#else

            uHashValue = ZobristTable[pData[0]][0] ^ ZobristTable[pData[1]][1] ^ ZobristTable[pData[2]][2]
#if LZ77_MIN_PTR_MATCH_LENGTH > 3
                            ^ ZobristTable[pData[3]][3]
#endif
            ;
#endif

            uHashValue &= uHashMask;
            // Link the position at the head of the list at this hash table position.
            pState->m_uNext[uPosition] = pHashTable[uHashValue];
            pHashTable[uHashValue] = (LZ77_INDEX) uPosition;

            uPosition += 1;
            pData += 1;
        }
        uLastPosition += 4;

        STATE.m_EncodeData.m_uHashInsertPosition = uPosition;
    }

    //
    // The last few( upto 3 ) bytes may have been left unhashed if uLastPosition is not a multiple of 4.
    //
    while (uPosition < uLastPosition)
    {
        pState->m_uNext[uPosition] = 0;
        uPosition += 1;
    }
}

#endif

#undef LZ77_MIN_PTR_MATCH_LENGTH
#undef Xpress9Lz77EncInsert
