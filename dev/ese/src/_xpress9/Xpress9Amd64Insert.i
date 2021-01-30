// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
            h0 = r0.m128i_u32[ROUND];
            h1 = r1.m128i_u32[ROUND];
            h2 = r2.m128i_u32[ROUND];
            h3 = r3.m128i_u32[ROUND];

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT
            uHashValue = * (__unaligned UInt32 *) (pData + 4*ROUND + 0);
            uHashValue = (UInt32) ((uHashValue ^ 0xDEADBEEFu) + (uHashValue >> 5));
            uHashValue ^= uHashValue >> 11;
            uHashValue ^= uHashValue >> 17;
            uHashValue &= uHashMask;
            ASSERT (uHashValue == h0, "uPosition=%Iu ROUND=%u", uPosition, ROUND);
#endif 

            pState->m_uNext[uPosition + 4*ROUND + 0] = pHashTable[h0];
            pHashTable[h0] = (LZ77_INDEX) (uPosition  + 4*ROUND + 0);

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT
            uHashValue = * (__unaligned UInt32 *) (pData + 4*ROUND + 1);
            uHashValue = (UInt32) ((uHashValue ^ 0xDEADBEEFu) + (uHashValue >> 5));
            uHashValue ^= uHashValue >> 11;
            uHashValue ^= uHashValue >> 17;
            uHashValue &= uHashMask;
            ASSERT (uHashValue == h1, "uPosition=%Iu ROUND=%u", uPosition, ROUND);
#endif 

            pState->m_uNext[uPosition + 4*ROUND + 1] = pHashTable[h1];
            pHashTable[h1] = (LZ77_INDEX) (uPosition + 4*ROUND + 1);

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT
            uHashValue = * (__unaligned UInt32 *) (pData + 4*ROUND + 2);
            uHashValue = (UInt32) ((uHashValue ^ 0xDEADBEEFu) + (uHashValue >> 5));
            uHashValue ^= uHashValue >> 11;
            uHashValue ^= uHashValue >> 17;
            uHashValue &= uHashMask;
            ASSERT (uHashValue == h2, "uPosition=%Iu ROUND=%u", uPosition, ROUND);
#endif 

            pState->m_uNext[uPosition + 4*ROUND + 2] = pHashTable[h2];
            pHashTable[h2] = (LZ77_INDEX) (uPosition + 4*ROUND + 2);

#if XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_ASSERT
            uHashValue = * (__unaligned UInt32 *) (pData + 4*ROUND + 3);
            uHashValue = (UInt32) ((uHashValue ^ 0xDEADBEEFu) + (uHashValue >> 5));
            uHashValue ^= uHashValue >> 11;
            uHashValue ^= uHashValue >> 17;
            uHashValue &= uHashMask;
            ASSERT (uHashValue == h3, "uPosition=%Iu ROUND=%u", uPosition, ROUND);
#endif 

            pState->m_uNext[uPosition + 4*ROUND + 3] = pHashTable[h3];
            pHashTable[h3] = (LZ77_INDEX) (uPosition + 4*ROUND + 3);

#undef ROUND
