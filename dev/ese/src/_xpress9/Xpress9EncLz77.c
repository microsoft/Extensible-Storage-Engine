// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "Xpress9Internal.h"

#pragma warning (push)
#pragma warning (disable: 4668) // intrin.h(82) : '_M_IX86' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#pragma warning (disable: 4820) // malloc.h(66) : '_heapinfo' : '4' bytes padding added after data member '_useflag'
#pragma warning (disable: 4255) // intrin.h(999) : '__getcallerseflags' : no function prototype given: converting '()' to '(void)'

#if XPRESS9_USE_SSE2
#include "emmintrin.h"
#endif /* XPRESS9_USE_SSE2 */

#if XPRESS9_USE_SSE4
#include "smmintrin.h"
#endif /* XPRESS9_USE_SSE4 */

#pragma warning (pop)

#define GET_ENCODER_STATE(pStatus,Failure,pState,pEncoder) do {             \
    if (pEncoder == NULL)                                                   \
    {                                                                       \
        SET_ERROR (Xpress9Status_BadArgument, pStatus, "pEncoderIsNULL");   \
        goto Failure;                                                       \
    }                                                                       \
    pState = (LZ77_PASS1_STATE *) pEncoder;                                 \
    if (pState->m_uMagic != XPRESS9_ENCODER_MAGIC)                          \
    {                                                                       \
        SET_ERROR (                                                         \
            Xpress9Status_BadArgument,                                      \
            pStatus,                                                        \
            "pState->m_uMagic=0x%Ix expecting 0x%Ix",                       \
            pState->m_uMagic,                                               \
            (uxint) XPRESS9_ENCODER_MAGIC                                   \
        );                                                                  \
        goto Failure;                                                       \
    };                                                                      \
    pState += 1;                                                            \
} while (0)


#define CHECK_MTF(uPosition,iOffset,iIndex,iEncodeIndex) do {                               \
    if (                                                                                    \
        ((xint) (uPosition)) + (iOffset) > 0 &&                                             \
        _pComp[iOffset] == _pComp[0] &&                                                     \
        _pComp[iOffset + 1] == _pComp[1] &&                                                 \
        (LZ77_MIN_MTF_MATCH_LENGTH <= 2 || _pComp[iOffset + 2] == _pComp[2]) &&             \
        (LZ77_MIN_MTF_MATCH_LENGTH <= 3 || _pComp[iOffset + 3] == _pComp[3])                \
    )                                                                                       \
    {                                                                                       \
        _pComp += LZ77_MIN_MTF_MATCH_LENGTH;                                                \
        while (_pComp < pEndData && _pComp[iOffset] == _pComp[0])                           \
        {                                                                                   \
            ++_pComp;                                                                       \
        }                                                                                   \
                                                                                            \
        uBestLength = (uxint) (_pComp - pData) - (uPosition);                               \
        ENCODE_MTF (uPosition, uBestLength, LZ77_MIN_MTF_MATCH_LENGTH, (iEncodeIndex));     \
        if (iIndex != 0)                                                                    \
        {                                                                                   \
            UPDATE_MTF (iIndex, iOffset);                                                   \
        }                                                                                   \
                                                                                            \
        goto EncodeMtfPtr;                                                                  \
    }                                                                                       \
} while (0)


// Like CHECK_MTF except we only take the match if it is >= uSavedBestLength-3
// If we take the match, first output the character at uPosition-1
#define LOOKAHEAD_CHECK_MTF(uPosition,iOffset,iIndex,iEncodeIndex) do {                     \
    if (                                                                                    \
        ((xint) (uPosition)) + (iOffset) > 0 &&                                             \
        _pComp[iOffset] == _pComp[0] &&                                                     \
        _pComp[iOffset + 1] == _pComp[1] &&                                                 \
        (LZ77_MIN_MTF_MATCH_LENGTH <= 2 || _pComp[iOffset + 2] == _pComp[2]) &&             \
        (LZ77_MIN_MTF_MATCH_LENGTH <= 3 || _pComp[iOffset + 3] == _pComp[3])                \
    )                                                                                       \
    {                                                                                       \
        uxint uMtfBestLength;                                                               \
        _pComp += LZ77_MIN_MTF_MATCH_LENGTH;                                                \
        while (_pComp < pEndData && _pComp[iOffset] == _pComp[0])                           \
        {                                                                                   \
            ++_pComp;                                                                       \
        }                                                                                   \
                                                                                            \
        uMtfBestLength = (uxint) (_pComp - pData) - (uPosition);                            \
        if (uMtfBestLength >= uSavedBestLength-3)                                           \
        {                                                                                   \
            uBestLength = uMtfBestLength;                                                   \
            ENCODE_LIT (uPosition-1);                                                       \
            ENCODE_MTF (uPosition, uBestLength, LZ77_MIN_MTF_MATCH_LENGTH, (iEncodeIndex)); \
            if (iIndex != 0)                                                                \
            {                                                                               \
                UPDATE_MTF (iIndex, iOffset);                                               \
            }                                                                               \
                                                                                            \
            goto EncodeMtfPtr;                                                              \
        }                                                                                   \
    }                                                                                       \
} while (0)


// Like LOOKAHEAD_CHECK_MTF except we output characters at uPosition-2 and uPosition-1 if we take the match
#define LOOKAHEAD2_CHECK_MTF(uPosition,iOffset,iIndex,iEncodeIndex) do {                    \
    if (                                                                                    \
        ((xint) (uPosition)) + (iOffset) > 0 &&                                             \
        _pComp[iOffset] == _pComp[0] &&                                                     \
        _pComp[iOffset + 1] == _pComp[1] &&                                                 \
        (LZ77_MIN_MTF_MATCH_LENGTH <= 2 || _pComp[iOffset + 2] == _pComp[2]) &&             \
        (LZ77_MIN_MTF_MATCH_LENGTH <= 3 || _pComp[iOffset + 3] == _pComp[3])                \
    )                                                                                       \
    {                                                                                       \
        uxint uMtfBestLength;                                                               \
        _pComp += LZ77_MIN_MTF_MATCH_LENGTH;                                                \
        while (_pComp < pEndData && _pComp[iOffset] == _pComp[0])                           \
        {                                                                                   \
            ++_pComp;                                                                       \
        }                                                                                   \
                                                                                            \
        uMtfBestLength = (uxint) (_pComp - pData) - (uPosition);                            \
        if (uMtfBestLength >= uSavedBestLength2-3)                                          \
        {                                                                                   \
            uBestLength = uMtfBestLength;                                                   \
            ENCODE_LIT (uPosition-2);                                                       \
            ENCODE_LIT (uPosition-1);                                                       \
            ENCODE_MTF (uPosition, uBestLength, LZ77_MIN_MTF_MATCH_LENGTH, (iEncodeIndex)); \
            if (iIndex != 0)                                                                \
            {                                                                               \
                UPDATE_MTF (iIndex, iOffset);                                               \
            }                                                                               \
                                                                                            \
            goto EncodeMtfPtr;                                                              \
        }                                                                                   \
    }                                                                                       \
} while (0)


#define ENCODE_PTR(uPosition,uBestLength,uMinMatchLength,iBestOffset) do {                  \
    uxint _uSymbol;                                                                         \
    uxint _uOffset;                                                                         \
    uxint _uLength;                                                                         \
    MSB_T _uMsbLength;                                                                      \
    MSB_T _uMsbOffset;                                                                      \
    TRACE ("ENC %Iu PTR %Iu %Id", (uPosition), (uBestLength), (iBestOffset));               \
                                                                                            \
    ASSERT ((iBestOffset) < 0, "iBestOffset=%Iu", (xint) (iBestOffset));                    \
    _uOffset = (uxint) (- (iBestOffset));                                                   \
    GET_MSB (_uMsbOffset, _uOffset);                                                        \
    _uOffset -= POWER2 (_uMsbOffset);                                                       \
    STATE.m_HuffmanStat.m_uStoredBitCount += _uMsbOffset;                                   \
    _uSymbol = (_uMsbOffset + (256 >> LZ77_MAX_SHORT_LENGTH_LOG) + LZ77_MTF) << LZ77_MAX_SHORT_LENGTH_LOG;    \
    ASSERT (                                                                                \
        (uBestLength) >= (uMinMatchLength),                                                 \
        "uBestLength=%Iu uMinMatchLength=%Iu",                                              \
        (uxint) (uBestLength),                                                              \
        (uxint) (uMinMatchLength)                                                           \
    );                                                                                      \
                                                                                            \
    _uLength = (uBestLength) - (uMinMatchLength);                                           \
    if (_uLength < LZ77_MAX_SHORT_LENGTH - 1)                                               \
    {                                                                                       \
        /* Note that the last LZ77_MAX_SHORT_LENGTH_LOG bits of _uSymbol are zeroes, */     \
        /* so all bits of _uLength can be fit in them */                                    \
        _uSymbol += _uLength;                                                               \
        TRACE ("P1 %p %Iu", pIrPtr, _uSymbol);                                              \
        /* The following cannot cause loss of precision because of the definition of LZ77_SHORT_SYMBOL_ALPHABET_SIZE */    \
        pIrPtr[0] = (UInt16) _uSymbol;                                                      \
        STATE.m_HuffmanStat.m_uShortSymbolCount[_uSymbol] += 1;                             \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        _uSymbol += LZ77_MAX_SHORT_LENGTH - 1;                                              \
        _uLength -= LZ77_MAX_SHORT_LENGTH - 1;                                              \
        TRACE ("P1 %p %Iu", pIrPtr, _uSymbol);                                              \
        pIrPtr[0] = (UInt16) _uSymbol;                                                      \
        STATE.m_HuffmanStat.m_uShortSymbolCount[_uSymbol] += 1;                             \
        if (_uLength <= LZ77_MAX_LONG_LENGTH - 1)                                           \
        {                                                                                   \
            pIrPtr[1] = (UInt16) _uLength;                                                  \
            pIrPtr += 1;                                                                    \
            STATE.m_HuffmanStat.m_uLongLengthCount[_uLength] += 1;                          \
        }                                                                                   \
        else                                                                                \
        {                                                                                   \
            _uLength -= LZ77_MAX_LONG_LENGTH - 1;                                           \
            GET_MSB (_uMsbLength, _uLength);                                                \
            _uLength -= POWER2 (_uMsbLength);                                               \
            STATE.m_HuffmanStat.m_uStoredBitCount += _uMsbLength;                           \
            _uSymbol = _uMsbLength + LZ77_MAX_LONG_LENGTH;                                  \
            pIrPtr[1] = (UInt16) _uSymbol;                                                  \
            STATE.m_HuffmanStat.m_uLongLengthCount[_uSymbol] += 1;                          \
                                                                                            \
            /* BUGBUG: Why 32, can we improve further by trying to fit in 24 bits? */       \
            if (_uMsbLength > 16)                                                           \
            { /* The remainder of the length cannot fit in 16 bits, we need more */         \
                * (UInt32 *) (pIrPtr + 2) = (UInt32) _uLength;                              \
                pIrPtr += 3;                                                                \
            }                                                                               \
            else                                                                            \
            {                                                                               \
                pIrPtr[2] = (UInt16) _uLength;                                              \
                pIrPtr += 2;                                                                \
            }                                                                               \
        }                                                                                   \
    }                                                                                       \
                                                                                            \
    if (_uMsbOffset > 16)                                                                   \
    {                                                                                       \
        * (UInt32 *) (pIrPtr + 1) = (UInt32) _uOffset;                                      \
        pIrPtr += 3;                                                                        \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        pIrPtr[1] = (UInt16) _uOffset;                                                      \
        pIrPtr += 2;                                                                        \
    }                                                                                       \
} while (0)


#define ENCODE_MTF(uPosition,uBestLength,uMinMatchLength,uMtfIndex) do {                    \
    uxint _uSymbol;                                                                         \
    uxint _uLength;                                                                         \
    MSB_T _uMsb;                                                                            \
                                                                                            \
    TRACE ("ENC %Iu MTF %Iu %Id", (uPosition), (uBestLength), (uMtfIndex));                 \
    _uSymbol = (uMtfIndex + (256 >> LZ77_MAX_SHORT_LENGTH_LOG)) << LZ77_MAX_SHORT_LENGTH_LOG;   \
    ASSERT (                                                                                \
        (uBestLength) >= (uMinMatchLength),                                                 \
        "uBestLength=%Iu uMinMatchLength=%Iu",                                              \
        (uxint) (uBestLength),                                                              \
        (uxint) (uMinMatchLength)                                                           \
    );                                                                                      \
                                                                                            \
    _uLength = (uBestLength) - (uMinMatchLength);                                           \
    if (_uLength < LZ77_MAX_SHORT_LENGTH - 1)                                               \
    {                                                                                       \
        _uSymbol += _uLength;                                                               \
        TRACE ("P1 %p %Iu", pIrPtr, _uSymbol);                                              \
        pIrPtr[0] = (UInt16) _uSymbol;                                                      \
        pIrPtr += 1;                                                                        \
        STATE.m_HuffmanStat.m_uShortSymbolCount[_uSymbol] += 1;                             \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        _uSymbol += LZ77_MAX_SHORT_LENGTH - 1;                                              \
        _uLength -= LZ77_MAX_SHORT_LENGTH - 1;                                              \
        TRACE ("P1 %p %Iu", pIrPtr, _uSymbol);                                              \
        pIrPtr[0] = (UInt16) _uSymbol;                                                      \
        pIrPtr += 1;                                                                        \
        STATE.m_HuffmanStat.m_uShortSymbolCount[_uSymbol] += 1;                             \
                                                                                            \
        if (_uLength <= LZ77_MAX_LONG_LENGTH - 1)                                           \
        {                                                                                   \
            pIrPtr[0] = (UInt16) _uLength;                                                  \
            pIrPtr += 1;                                                                    \
            STATE.m_HuffmanStat.m_uLongLengthCount[_uLength] += 1;                          \
        }                                                                                   \
        else                                                                                \
        {                                                                                   \
            _uLength -= LZ77_MAX_LONG_LENGTH - 1;                                           \
            GET_MSB (_uMsb, _uLength);                                                      \
            _uLength -= POWER2 (_uMsb);                                                     \
            STATE.m_HuffmanStat.m_uStoredBitCount += _uMsb;                                 \
            _uSymbol = _uMsb + LZ77_MAX_LONG_LENGTH;                                        \
            pIrPtr[0] = (UInt16) _uSymbol;                                                  \
            STATE.m_HuffmanStat.m_uLongLengthCount[_uSymbol] += 1;                          \
                                                                                            \
            if (_uMsb > 16)                                                                 \
            {                                                                               \
                * (UInt32 *) (pIrPtr + 1) = (UInt32) _uLength;                              \
                pIrPtr += 3;                                                                \
            }                                                                               \
            else                                                                            \
            {                                                                               \
                pIrPtr[1] = (UInt16) _uLength;                                              \
                pIrPtr += 2;                                                                \
            }                                                                               \
        }                                                                                   \
    }                                                                                       \
} while (0)


#define ENCODE_LIT(uPosition) do {                                                          \
    uxint _uSymbol;                                                                         \
                                                                                            \
    _uSymbol = pData[uPosition];                                                            \
    TRACE ("ENC %Iu LIT %Iu", (uPosition), _uSymbol);                                       \
    TRACE ("P1 %p %Iu", pIrPtr, _uSymbol);                                                  \
    pIrPtr[0] = (UInt16) _uSymbol;                                                          \
    pIrPtr += 1;                                                                            \
    STATE.m_HuffmanStat.m_uShortSymbolCount[_uSymbol] += 1;                                 \
} while (0)



#ifndef XRPESS9_LOOKUP_HAVE_MAX_OFFSET_BY_LENGTH
#define XRPESS9_LOOKUP_HAVE_MAX_OFFSET_BY_LENGTH 6
__declspec(align(64))
static
xint
s_iMaxOffsetByLength[XRPESS9_LOOKUP_HAVE_MAX_OFFSET_BY_LENGTH] =
{
    /* 0 */           0,
    /* 1 */           0,
    /* 2 */    -1 <<  6,
    /* 3 */    -1 << 10,
    /* 4 */    -1 << 13,
    /* 5 */    -1 << 16,
};
#endif /* XRPESS9_LOOKUP_HAVE_MAX_OFFSET_BY_LENGTH */


#define pNext pState[0].m_uNext


#define DEEP_LOOKUP                 0
#define LZ77_MTF                    0
#define LZ77_MIN_MTF_MATCH_LENGTH   2
#define LZ77_MIN_PTR_MATCH_LENGTH   3
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep0_Mtf0_MtfLen2_MatchLen3
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 1
#define LZ77_MTF                    0
#define LZ77_MIN_MTF_MATCH_LENGTH   2
#define LZ77_MIN_PTR_MATCH_LENGTH   3
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep1_Mtf0_MtfLen2_MatchLen3
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 0
#define LZ77_MTF                    0
#define LZ77_MIN_MTF_MATCH_LENGTH   3
#define LZ77_MIN_PTR_MATCH_LENGTH   3
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep0_Mtf0_MtfLen3_MatchLen3
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 1
#define LZ77_MTF                    0
#define LZ77_MIN_MTF_MATCH_LENGTH   3
#define LZ77_MIN_PTR_MATCH_LENGTH   3
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep1_Mtf0_MtfLen3_MatchLen3
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 0
#define LZ77_MTF                    2
#define LZ77_MIN_MTF_MATCH_LENGTH   2
#define LZ77_MIN_PTR_MATCH_LENGTH   3
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep0_Mtf2_MtfLen2_MatchLen3
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 1
#define LZ77_MTF                    2
#define LZ77_MIN_MTF_MATCH_LENGTH   2
#define LZ77_MIN_PTR_MATCH_LENGTH   3
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep1_Mtf2_MtfLen2_MatchLen3
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 0
#define LZ77_MTF                    2
#define LZ77_MIN_MTF_MATCH_LENGTH   3
#define LZ77_MIN_PTR_MATCH_LENGTH   3
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep0_Mtf2_MtfLen3_MatchLen3
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 1
#define LZ77_MTF                    2
#define LZ77_MIN_MTF_MATCH_LENGTH   3
#define LZ77_MIN_PTR_MATCH_LENGTH   3
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep1_Mtf2_MtfLen3_MatchLen3
#include "Xpress9Lz77EncPass1.i"


#define DEEP_LOOKUP                 0
#define LZ77_MTF                    4
#define LZ77_MIN_MTF_MATCH_LENGTH   2
#define LZ77_MIN_PTR_MATCH_LENGTH   3
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep0_Mtf4_MtfLen2_MatchLen3
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 1
#define LZ77_MTF                    4
#define LZ77_MIN_MTF_MATCH_LENGTH   2
#define LZ77_MIN_PTR_MATCH_LENGTH   3
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep1_Mtf4_MtfLen2_MatchLen3
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 0
#define LZ77_MTF                    4
#define LZ77_MIN_MTF_MATCH_LENGTH   3
#define LZ77_MIN_PTR_MATCH_LENGTH   3
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep0_Mtf4_MtfLen3_MatchLen3
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 1
#define LZ77_MTF                    4
#define LZ77_MIN_MTF_MATCH_LENGTH   3
#define LZ77_MIN_PTR_MATCH_LENGTH   3
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep1_Mtf4_MtfLen3_MatchLen3
#include "Xpress9Lz77EncPass1.i"


#define DEEP_LOOKUP                 0
#define LZ77_MTF                    0
#define LZ77_MIN_MTF_MATCH_LENGTH   2
#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep0_Mtf0_MtfLen2_MatchLen4
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 1
#define LZ77_MTF                    0
#define LZ77_MIN_MTF_MATCH_LENGTH   2
#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep1_Mtf0_MtfLen2_MatchLen4
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 0
#define LZ77_MTF                    0
#define LZ77_MIN_MTF_MATCH_LENGTH   3
#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep0_Mtf0_MtfLen3_MatchLen4
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 1
#define LZ77_MTF                    0
#define LZ77_MIN_MTF_MATCH_LENGTH   3
#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep1_Mtf0_MtfLen3_MatchLen4
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 0
#define LZ77_MTF                    2
#define LZ77_MIN_MTF_MATCH_LENGTH   2
#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep0_Mtf2_MtfLen2_MatchLen4
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 1
#define LZ77_MTF                    2
#define LZ77_MIN_MTF_MATCH_LENGTH   2
#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep1_Mtf2_MtfLen2_MatchLen4
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 0
#define LZ77_MTF                    2
#define LZ77_MIN_MTF_MATCH_LENGTH   3
#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep0_Mtf2_MtfLen3_MatchLen4
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 1
#define LZ77_MTF                    2
#define LZ77_MIN_MTF_MATCH_LENGTH   3
#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep1_Mtf2_MtfLen3_MatchLen4
#include "Xpress9Lz77EncPass1.i"


#define DEEP_LOOKUP                 0
#define LZ77_MTF                    4
#define LZ77_MIN_MTF_MATCH_LENGTH   2
#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep0_Mtf4_MtfLen2_MatchLen4
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 1
#define LZ77_MTF                    4
#define LZ77_MIN_MTF_MATCH_LENGTH   2
#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep1_Mtf4_MtfLen2_MatchLen4
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 0
#define LZ77_MTF                    4
#define LZ77_MIN_MTF_MATCH_LENGTH   3
#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep0_Mtf4_MtfLen3_MatchLen4
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 1
#define LZ77_MTF                    4
#define LZ77_MIN_MTF_MATCH_LENGTH   3
#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define TAIL_T                      UInt16
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep1_Mtf4_MtfLen3_MatchLen4
#include "Xpress9Lz77EncPass1.i"

#define DEEP_LOOKUP                 1
#define LZ77_MTF                    4
#define LZ77_MIN_MTF_MATCH_LENGTH   2
#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define TAIL_T                      UInt16
#define LAZY_MATCH_EVALUATION       1
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep1_Mtf4_MtfLen2_MatchLen4_LazyMatchEvaluation
#include "Xpress9Lz77EncPass1.i"
#undef  LAZY_MATCH_EVALUATION

#define DEEP_LOOKUP                 1
#define LZ77_MTF                    4
#define LZ77_MIN_MTF_MATCH_LENGTH   2
#define LZ77_MIN_PTR_MATCH_LENGTH   3
#define TAIL_T                      UInt16
#define LAZY_MATCH_EVALUATION       1
#define Xpress9Lz77EncPass1 Xpress9Lz77EncPass1_Fast1_Deep1_Mtf4_MtfLen2_MatchLen3_LazyMatchEvaluation
#include "Xpress9Lz77EncPass1.i"
#undef  LAZY_MATCH_EVALUATION

#include "Xpress9ZobristTable.h"


#define LZ77_MIN_PTR_MATCH_LENGTH   3
#define Xpress9Lz77EncInsert Xpress9Lz77EncInsert_MatchLen3
#include "Xpress9Lz77EncInsert.i"

#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define Xpress9Lz77EncInsert Xpress9Lz77EncInsert_MatchLen4
#include "Xpress9Lz77EncInsert.i"

#if XPRESS9_USE_SSE2
#define LZ77_MIN_PTR_MATCH_LENGTH   4
#define Xpress9Lz77EncInsert_SSE2 Xpress9Lz77EncInsert_MatchLen4_SSE2
#define Xpress9Lz77EncInsert "Xpress9Lz77EncInsert_ShallNotBeInstantiated"
#include "Xpress9Lz77EncInsert.i"
#endif /* XPRESS9_USE_SSE2 */

#define LZ77_MTF 0
#define Xpress9Lz77EncPass2 Xpress9Lz77EncPass2_Mtf0
#include "Xpress9Lz77EncPass2.i"

#define LZ77_MTF 2
#define Xpress9Lz77EncPass2 Xpress9Lz77EncPass2_Mtf2
#include "Xpress9Lz77EncPass2.i"


#define LZ77_MTF 4
#define Xpress9Lz77EncPass2 Xpress9Lz77EncPass2_Mtf4
#include "Xpress9Lz77EncPass2.i"


static
void
Xpress9EncoderVerifyParameters (
    XPRESS9_STATUS                 *pStatus,
    const XPRESS9_ENCODER_PARAMS   *pParams,
    uxint                           uMaxWindowSizeLog2
)
{
    RETAIL_ASSERT (pStatus->m_uStatus == Xpress9Status_OK, "IllegalStatus=%u", pStatus->m_uStatus);
    if (pParams->m_cbSize != sizeof (XPRESS9_ENCODER_PARAMS))
    {
        SET_ERROR (Xpress9Status_BadArgument, pStatus, "pParams->m_cbSize=%u", pParams->m_cbSize);
        goto Failure;
    }

    if (pParams->m_uMtfEntryCount != 0 && pParams->m_uMtfEntryCount != 2 && pParams->m_uMtfEntryCount != 4)
    {
        SET_ERROR (Xpress9Status_BadArgument, pStatus, "pParams->m_uMtfEntryCount=%u, expecting 0, 2, or 4", pParams->m_uMtfEntryCount);
        goto Failure;
    }

    if (pParams->m_uLookupDepth > 9 && pParams->m_uOptimizationLevel == 0)
    {
        SET_ERROR (Xpress9Status_BadArgument, pStatus, "pParams->m_uLookupDepth=%u, expecting value in [0..9] range", pParams->m_uLookupDepth);
        goto Failure;
    }

    if (pParams->m_uWindowSizeLog2 < XPRESS9_WINDOW_SIZE_LOG2_MIN || pParams->m_uWindowSizeLog2 > uMaxWindowSizeLog2)
    {
        SET_ERROR (Xpress9Status_BadArgument, pStatus, "pParams->m_uWindowSizeLog2=%u, expecting value in [%Iu..%Iu] range", pParams->m_uWindowSizeLog2, (uxint) XPRESS9_WINDOW_SIZE_LOG2_MIN, (uxint) uMaxWindowSizeLog2);
        goto Failure;
    }

    if (pParams->m_uOptimizationLevel > 1)
    {
        SET_ERROR (Xpress9Status_BadArgument, pStatus, "pParams->m_uOptimizationLevel=%u, expecting 0 or 1", pParams->m_uOptimizationLevel);
        goto Failure;
    }

    if (pParams->m_uPtrMinMatchLength < 3 || pParams->m_uPtrMinMatchLength > 4)
    {
        SET_ERROR (Xpress9Status_BadArgument, pStatus, "pParams->m_uPtrMinMatchLength=%u, expecting 3 or 4", pParams->m_uPtrMinMatchLength);
        goto Failure;
    }

    if (pParams->m_uMtfMinMatchLength < 2 || pParams->m_uMtfMinMatchLength > pParams->m_uPtrMinMatchLength)
    {
        SET_ERROR (Xpress9Status_BadArgument, pStatus, "pParams->m_uMtfMinMatchLength=%u, expecting value in [2..%u] range", pParams->m_uMtfMinMatchLength, pParams->m_uPtrMinMatchLength);
        goto Failure;
    }

Failure:;
}


static
void
Xpress9GetLz77Procs (
    XPRESS9_STATUS                  *pStatus,
    const XPRESS9_ENCODER_PARAMS    *pParams,
    LZ77_PASS1_STATE                *pState
)
{
    Lz77EncPass1_Proc      *pLz77EncPass1;    // pass 1 encoder
    Lz77EncPass2_Proc      *pLz77EncPass2;    // pass 2 encoder
    Lz77EncInsert_Proc     *pLz77EncInsert;   // hash insertion procedure

    pLz77EncPass1 = NULL;
    pLz77EncPass2 = NULL;
    pLz77EncInsert = NULL;

    RETAIL_ASSERT (pStatus->m_uStatus == Xpress9Status_OK, "IllegalStatus=%u", pStatus->m_uStatus);
    if (pParams->m_uPtrMinMatchLength == 3)
    {
        pLz77EncInsert = Xpress9Lz77EncInsert_MatchLen3;
        switch (pParams->m_uMtfEntryCount)
        {
        default:
            RETAIL_ASSERT (FALSE, "IllegalMtfEntryCount=%u", pParams->m_uMtfEntryCount);
            goto Failure;

        case 0:
            if (pParams->m_uLookupDepth == 0)
            {
                if (pParams->m_uMtfMinMatchLength == 2)
                {
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep0_Mtf0_MtfLen2_MatchLen3;
                }
                else
                {
                    RETAIL_ASSERT (pParams->m_uMtfMinMatchLength == 3, "IllegalMtfMinMatchLength=%u", pParams->m_uMtfMinMatchLength);
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep0_Mtf0_MtfLen3_MatchLen3;
                }
            }
            else
            {
                if (pParams->m_uMtfMinMatchLength == 2)
                {
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep1_Mtf0_MtfLen2_MatchLen3;
                }
                else
                {
                    RETAIL_ASSERT (pParams->m_uMtfMinMatchLength == 3, "IllegalMtfMinMatchLength=%u", pParams->m_uMtfMinMatchLength);
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep1_Mtf0_MtfLen3_MatchLen3;
                }
            }

            break;

        case 2:
            if (pParams->m_uLookupDepth == 0)
            {
                if (pParams->m_uMtfMinMatchLength == 2)
                {
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep0_Mtf2_MtfLen2_MatchLen3;
                }
                else
                {
                    RETAIL_ASSERT (pParams->m_uMtfMinMatchLength == 3, "IllegalMtfMinMatchLength=%u", pParams->m_uMtfMinMatchLength);
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep0_Mtf2_MtfLen3_MatchLen3;
                }
            }
            else
            {
                if (pParams->m_uMtfMinMatchLength == 2)
                {
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep1_Mtf2_MtfLen2_MatchLen3;
                }
                else
                {
                    RETAIL_ASSERT (pParams->m_uMtfMinMatchLength == 3, "IllegalMtfMinMatchLength=%u", pParams->m_uMtfMinMatchLength);
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep1_Mtf2_MtfLen3_MatchLen3;
                }
            }

            break;

        case 4:
            if (pParams->m_uLookupDepth == 0)
            {
                if (pParams->m_uMtfMinMatchLength == 2)
                {
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep0_Mtf4_MtfLen2_MatchLen3;
                }
                else
                {
                    RETAIL_ASSERT (pParams->m_uMtfMinMatchLength == 3, "IllegalMtfMinMatchLength=%u", pParams->m_uMtfMinMatchLength);
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep0_Mtf4_MtfLen3_MatchLen3;
                }
            }
            else
            {
                if (pParams->m_uMtfMinMatchLength == 2)
                {
                    if (pParams->m_uOptimizationLevel > 0)
                    {
                        pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep1_Mtf4_MtfLen2_MatchLen3_LazyMatchEvaluation;                         
                    }
                    else
                    {
                        pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep1_Mtf4_MtfLen2_MatchLen3;
                    }
                }
                else
                {
                    RETAIL_ASSERT (pParams->m_uMtfMinMatchLength == 3, "IllegalMtfMinMatchLength=%u", pParams->m_uMtfMinMatchLength);
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep1_Mtf4_MtfLen3_MatchLen3;
                }
            }

            break;
        }
    } 
    else
    {
        RETAIL_ASSERT (pParams->m_uPtrMinMatchLength == 4, "IllegalPtrMinMatchLength=%u", pParams->m_uPtrMinMatchLength);
#if XPRESS9_USE_SSE2
        if (STATE.m_uRuntimeFlags & Xpress9Flag_UseSSE2)
        {
            pLz77EncInsert = Xpress9Lz77EncInsert_MatchLen4_SSE2;
        }
        else
#endif /* XPRESS9_USE_SSE2 */
        {
            pLz77EncInsert = Xpress9Lz77EncInsert_MatchLen4;
        }
        switch (pParams->m_uMtfEntryCount)
        {
        default:
            RETAIL_ASSERT (FALSE, "IllegalMtfEntryCount=%u", pParams->m_uMtfEntryCount);
            goto Failure;

        case 0:
            if (pParams->m_uLookupDepth == 0)
            {
                if (pParams->m_uMtfMinMatchLength == 2)
                {
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep0_Mtf0_MtfLen2_MatchLen4;
                }
                else
                {
                    RETAIL_ASSERT (pParams->m_uMtfMinMatchLength == 3, "IllegalMtfMinMatchLength=%u", pParams->m_uMtfMinMatchLength);
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep0_Mtf0_MtfLen3_MatchLen4;
                }
            }
            else
            {
                if (pParams->m_uMtfMinMatchLength == 2)
                {
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep1_Mtf0_MtfLen2_MatchLen4;
                }
                else
                {
                    RETAIL_ASSERT (pParams->m_uMtfMinMatchLength == 3, "IllegalMtfMinMatchLength=%u", pParams->m_uMtfMinMatchLength);
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep1_Mtf0_MtfLen3_MatchLen4;
                }
            }

            break;

        case 2:
            if (pParams->m_uLookupDepth == 0)
            {
                if (pParams->m_uMtfMinMatchLength == 2)
                {
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep0_Mtf2_MtfLen2_MatchLen4;
                }
                else
                {
                    RETAIL_ASSERT (pParams->m_uMtfMinMatchLength == 3, "IllegalMtfMinMatchLength=%u", pParams->m_uMtfMinMatchLength);
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep0_Mtf2_MtfLen3_MatchLen4;
                }
            }
            else
            {
                if (pParams->m_uMtfMinMatchLength == 2)
                {
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep1_Mtf2_MtfLen2_MatchLen4;
                }
                else
                {
                    RETAIL_ASSERT (pParams->m_uMtfMinMatchLength == 3, "IllegalMtfMinMatchLength=%u", pParams->m_uMtfMinMatchLength);
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep1_Mtf2_MtfLen3_MatchLen4;
                }
            }

            break;

        case 4:
            if (pParams->m_uLookupDepth == 0)
            {
                if (pParams->m_uMtfMinMatchLength == 2)
                {
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep0_Mtf4_MtfLen2_MatchLen4;
                }
                else
                {
                    RETAIL_ASSERT (pParams->m_uMtfMinMatchLength == 3, "IllegalMtfMinMatchLength=%u", pParams->m_uMtfMinMatchLength);
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep0_Mtf4_MtfLen3_MatchLen4;
                }
            }
            else
            {
                if (pParams->m_uMtfMinMatchLength == 2)
                {
                    if (pParams->m_uOptimizationLevel > 0)
                    {
                        pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep1_Mtf4_MtfLen2_MatchLen4_LazyMatchEvaluation;                         
                    }
                    else
                    {
                        pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep1_Mtf4_MtfLen2_MatchLen4;                    
                    }
                }
                else
                {
                    RETAIL_ASSERT (pParams->m_uMtfMinMatchLength == 3, "IllegalMtfMinMatchLength=%u", pParams->m_uMtfMinMatchLength);
                    pLz77EncPass1 = Xpress9Lz77EncPass1_Fast1_Deep1_Mtf4_MtfLen3_MatchLen4;
                }
            }

            break;
        }
    }

    switch (pParams->m_uMtfEntryCount)
    {
    default:
        RETAIL_ASSERT (FALSE, "IllegalMtfEntryCount=%u", pParams->m_uMtfEntryCount);
        goto Failure;
    case 0:
        pLz77EncPass2 = Xpress9Lz77EncPass2_Mtf0;
        break;
    case 2:
        pLz77EncPass2 = Xpress9Lz77EncPass2_Mtf2;
        break;
    case 4:
        pLz77EncPass2 = Xpress9Lz77EncPass2_Mtf4;
        break;
    }

Failure:
    STATE.m_Params.m_pLz77EncPass1  = pLz77EncPass1; 
    STATE.m_Params.m_pLz77EncPass2  = pLz77EncPass2;
    STATE.m_Params.m_pLz77EncInsert = pLz77EncInsert;
}


//
// Allocates memory, initializes, and returns the pointer to newly created encoder.
//
// It is recommended to create few encoders (1-2 encoders per CPU core) and reuse them:
// creation of new encoder is relatively expensive.
//
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
)
{
    LZ77_PASS1_STATE    *pState = NULL;
    UInt8      *pAllocatedMemory = NULL;       // pointer to the beginning of allocated memory block
    UInt8      *pAlignedPtr = NULL;            // a pointer aligned on appropriate byte boundary
    uxint       uHashTableSize = 0;         // hash table max dimension
    uxint       uHashTableAllocSize = 0;    // amount of memory occupied by the hash table
    uxint       uIrBufferSize = 0;          // size of the buffer containing intermediate data
    uxint       uScratchAreaSize = 0;       // size of scratch area
    uxint       uDataBufferSize = 0;        // size of internal data buffer (must be exact power of 2)
    uxint       uNextTableSize = 0;         // size of real m_pNext[] array
    uxint       uAllocationSize = 0;        // total allocation size

    memset (pStatus, 0, sizeof (*pStatus));
    if (uMaxWindowSizeLog2 < XPRESS9_WINDOW_SIZE_LOG2_MIN || uMaxWindowSizeLog2 > XPRESS9_WINDOW_SIZE_LOG2_MAX)
    {
        SET_ERROR (
            Xpress9Status_BadArgument,
            pStatus,
            "uMaxWindowSizeLog2=%Iu is out of range [%u..%u]",
            uMaxWindowSizeLog2,
            XPRESS9_WINDOW_SIZE_LOG2_MIN,
            XPRESS9_WINDOW_SIZE_LOG2_MAX
        );

        goto Failure;
    }

    uDataBufferSize = POWER2 (uMaxWindowSizeLog2);

    uHashTableSize = uDataBufferSize >> 1;
    uHashTableAllocSize = uHashTableSize * sizeof (STATE.m_HashTable.m_pHashTable[0]);

    uIrBufferSize = uDataBufferSize * 2;

    uScratchAreaSize = 8 * 1024;

    uNextTableSize = sizeof (STATE.m_uNext[0]) * uDataBufferSize;

    uAllocationSize = sizeof (STATE) + MEMORY_ALIGNMENT * 2 + uDataBufferSize + uHashTableAllocSize + uIrBufferSize + uScratchAreaSize + uNextTableSize;

    pAllocatedMemory = (UInt8 *) pAllocFn (pAllocContext, (int) uAllocationSize);


    if (pAllocatedMemory == NULL)
    {
        SET_ERROR (Xpress9Status_NotEnoughMemory, pStatus, "");
        goto Failure;
    }

    pAlignedPtr = pAllocatedMemory + sizeof (STATE);
    pAlignedPtr += (- (xint) pAlignedPtr) & (MEMORY_ALIGNMENT - 1);
    pState = (LZ77_PASS1_STATE *) pAlignedPtr;
    memset (&STATE, 0, sizeof (STATE));
    pAlignedPtr += uNextTableSize;

    STATE.m_Scratch.m_pScratchArea = pAlignedPtr;
    STATE.m_Scratch.m_uScratchAreaSize = uScratchAreaSize;
    pAlignedPtr += uScratchAreaSize;
    pAlignedPtr += (- (xint) pAlignedPtr) & (MEMORY_ALIGNMENT - 1);

    STATE.m_BufferData.m_pBufferData = pAlignedPtr;
    STATE.m_BufferData.m_uBufferDataSize = uDataBufferSize;
    pAlignedPtr += uDataBufferSize;

    STATE.m_Pass2.m_Ir.m_pIrBuffer = pAlignedPtr;
    STATE.m_Pass2.m_Ir.m_pIrPtr    = STATE.m_Pass2.m_Ir.m_pIrBuffer;
    STATE.m_Pass2.m_Ir.m_uIrBufferSize = uIrBufferSize;
    pAlignedPtr += uIrBufferSize;

    STATE.m_HashTable.m_pHashTable = (LZ77_INDEX *) pAlignedPtr;
    STATE.m_HashTable.m_uHashTableSizeMax = uHashTableSize;

    STATE.m_pAllocatedMemory = pAllocatedMemory;
    STATE.m_uMaxWindowSizeLog2 = uMaxWindowSizeLog2;
    STATE.m_Pass2.m_pState = pState;
    STATE.m_uRuntimeFlags = uFlags;
    STATE.m_uState = LZ77_ENCODER_STATE_READY;
    STATE.m_uMagic = XPRESS9_ENCODER_MAGIC;


    return ((XPRESS9_ENCODER) &STATE);

Failure:
    return (NULL);
}


static
BOOL
Xpress9EncoderEmpty (
    LZ77_PASS1_STATE *pState
)
{
    return (
        STATE.m_Pass2.m_Ir.m_pIrPtr == STATE.m_Pass2.m_Ir.m_pIrBuffer &&
        STATE.m_UserData.m_uUserDataSize == STATE.m_UserData.m_uUserDataProcessed
    );
}



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
)
{
    XPRESS9_ENCODER_PARAMS  Params;
    MSB_T                   uMsb;
    uxint                   uMaxStreamLength;
    uxint                   i;
    LZ77_PASS1_STATE       *pState;

    memset (pStatus, 0, sizeof (*pStatus));
    GET_ENCODER_STATE (pStatus, Failure, pState, pEncoder);

    if (!fForceReset)
    {
        if (STATE.m_uState != LZ77_ENCODER_STATE_READY || !Xpress9EncoderEmpty (pState))
        {
            SET_ERROR (
                Xpress9Status_EncoderNotEmpty,
                pStatus,
                "pState->m_uState=%Iu EncoderEmpty=%u",
                pState->m_uState,
                Xpress9EncoderEmpty (pState)
            );
            goto Failure;
        }
    }

    //
    // get parameters for this encoding session
    //
    Params = *pParams;

    Xpress9EncoderVerifyParameters (pStatus, pParams, STATE.m_uMaxWindowSizeLog2);
    if (pStatus->m_uStatus != Xpress9Status_OK)
        goto Failure;

    Xpress9GetLz77Procs (pStatus, &Params, pState);
    if (pStatus->m_uStatus != Xpress9Status_OK)
        goto Failure;

    STATE.m_Pass2.m_Ir.m_pIrPtr = STATE.m_Pass2.m_Ir.m_pIrBuffer;
    memset (&STATE.m_EncodeData, 0, sizeof (STATE.m_EncodeData));
    memset (&STATE.m_UserData, 0, sizeof (STATE.m_UserData));

    STATE.m_Params.m_Current = Params;

    if (STATE.m_Params.m_Current.m_uMaxStreamLength != 0 && STATE.m_Params.m_Current.m_uMaxStreamLength < POWER2 (STATE.m_Params.m_Current.m_uWindowSizeLog2))
    {
        // If the stream length is less than the window size, constrain the window size to 512. 
        GET_MSB (uMsb, STATE.m_Params.m_Current.m_uMaxStreamLength);
        if (STATE.m_Params.m_Current.m_uMaxStreamLength > POWER2 (uMsb))
            uMsb += 1;
        if (uMsb < 9)
            uMsb = 9;
        STATE.m_Params.m_Current.m_uWindowSizeLog2 = uMsb;
    }

    // set size of hash table
    uMaxStreamLength = STATE.m_Params.m_Current.m_uMaxStreamLength;
    if (uMaxStreamLength == 0 || uMaxStreamLength >= POWER2 (STATE.m_Params.m_Current.m_uWindowSizeLog2))
    {
        uMsb = (MSB_T) STATE.m_Params.m_Current.m_uWindowSizeLog2;
    }
    else
    {
        GET_MSB (uMsb, uMaxStreamLength);
        if (STATE.m_Params.m_Current.m_uMaxStreamLength > POWER2 (uMsb))
            uMsb += 1;
        if (uMsb < 9)
            uMsb = 9;
    }
    STATE.m_EncodeData.m_uWindowSize = POWER2 (uMsb);

    for (i = 0; i < LZ77_MAX_MTF; ++i)
    {
        STATE.m_EncodeData.m_Mtf.m_iMtfOffset[i] = (xint) -1;
    }
    STATE.m_EncodeData.m_Mtf.m_iMtfLastPtr = 0;


    uMsb -= 1;
    switch (STATE.m_Params.m_Current.m_uLookupDepth)
    {
    case 0:
        if (uMsb > 12)
            uMsb = 12;
        break;
    case 1:
        if (uMsb > 12)
            uMsb = 12;
        break;
    case 2:
        if (uMsb > 18)
            uMsb = 18;
        break;
    }

    STATE.m_HashTable.m_uHashTableSizeCurrent = POWER2 (uMsb);
    ASSERT (STATE.m_HashTable.m_uHashTableSizeCurrent <= STATE.m_HashTable.m_uHashTableSizeMax, "");
    memset (STATE.m_HashTable.m_pHashTable, 0, sizeof (STATE.m_HashTable.m_pHashTable[0]) * STATE.m_HashTable.m_uHashTableSizeCurrent);

    STATE.m_Params.m_fDirty = FALSE;

    pState->m_uState = LZ77_ENCODER_STATE_READY;

    if (STATE.m_Stat.m_Block.m_uOrigSize != 0)
    {
        STATE.m_Stat.m_Session.m_uCount    += 1;
        STATE.m_Stat.m_Session.m_uOrigSize += STATE.m_Stat.m_Block.m_uOrigSize;
        STATE.m_Stat.m_Session.m_uCompSize += STATE.m_Stat.m_Block.m_uCompSize;
        memset (&STATE.m_Stat.m_Block, 0, sizeof (STATE.m_Stat.m_Block));
    }

    if (STATE.m_Stat.m_Session.m_uOrigSize != 0)
    {
        STATE.m_Stat.m_Encoder.m_uCount    += 1;
        STATE.m_Stat.m_Encoder.m_uOrigSize += STATE.m_Stat.m_Session.m_uOrigSize;
        STATE.m_Stat.m_Encoder.m_uCompSize += STATE.m_Stat.m_Session.m_uCompSize;
        memset (&STATE.m_Stat.m_Session, 0, sizeof (STATE.m_Stat.m_Session));
    }

    {
        UInt64  uRandomValue;
        uRandomValue = __rdtsc ();
        STATE.m_Stat.m_uSessionSignature = Xpress9Crc32 ((const UInt8 *) &uRandomValue, sizeof (uRandomValue), 0);
        STATE.m_Stat.m_uBlockIndex = 0;
    }

Failure:;
}



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
)
{
    LZ77_PASS1_STATE       *pState;

    memset (pStatus, 0, sizeof (*pStatus));
    GET_ENCODER_STATE (pStatus, Failure, pState, pEncoder);

    if (STATE.m_uState != LZ77_ENCODER_STATE_READY)
    {
        SET_ERROR (Xpress9Status_EncoderNotReady, pStatus, "Encoder->m_uState=%Iu", STATE.m_uState);
        goto Failure;
    }

    ASSERT (
        STATE.m_UserData.m_uUserDataSize == STATE.m_UserData.m_uUserDataProcessed,
        "uUserDataSize=%Iu m_uUserDataProcessed=%Iu",
        STATE.m_UserData.m_uUserDataSize,
        STATE.m_UserData.m_uUserDataProcessed
    );

    if (pOrigData == NULL || uOrigDataSize == 0)
    {
        if (!fFlush || pOrigData != NULL || uOrigDataSize != 0)
        {
            SET_ERROR (
                Xpress9Status_BadArgument,
                pStatus,
                "pOrigData=%p uOrigDataSize=%u fFlush=%u",
                pOrigData,
                uOrigDataSize,
                fFlush
            );
            goto Failure;
        }
    }

    STATE.m_uState = LZ77_ENCODER_STATE_ENCODING;

    STATE.m_UserData.m_fFlush               = fFlush;
    STATE.m_UserData.m_pUserData            = pOrigData;
    STATE.m_UserData.m_uUserDataSize        = uOrigDataSize;
    STATE.m_UserData.m_uUserDataProcessed   = 0;


Failure:;
}


static
void
ShiftValues (
    LZ77_INDEX *pDst,
    LZ77_INDEX *pSrc,
    uxint       uCount,
    uxint       uRemoveSize
)
{
    uxint       i;
    uxint       v0, v1, v2, v3;

    for (i = 0; i < uCount; i += 4)
    {
        v0 = pSrc[i];
        v1 = pSrc[i + 1];
        v2 = pSrc[i + 2];
        v3 = pSrc[i + 3];
        // The values in the memory area under consideration m_pNext
        // are indexes into itself, forming a singly linked list.
        // If the pointer value points to a cell outside the uRemoveSize,
        // it should now point to the new cell.
        //
        //    |<----removeSize--------->|<--------- uCount, keepSize ---->
        //    0                         10  
        //    --------------------------|---------------------------------|
        //    |                         |                                 |
        //    |                         |                                 |
        //    |-------------------------|---------------------------------|
        //    ^                         ^
        //    |                         |
        //    pDst                      pSrc
        if (((xint)(v0 -= uRemoveSize)) < 0)
            v0 = 0;
        if (((xint)(v1 -= uRemoveSize)) < 0)
            v1 = 0;
        if (((xint)(v2 -= uRemoveSize)) < 0)
            v2 = 0;
        if (((xint)(v3 -= uRemoveSize)) < 0)
            v3 = 0;
        pDst[i] = (LZ77_INDEX) v0;
        pDst[i + 1] = (LZ77_INDEX) v1;
        pDst[i + 2] = (LZ77_INDEX) v2;
        pDst[i + 3] = (LZ77_INDEX) v3;
    }
}


#if XPRESS9_USE_SSE2
#pragma prefast(push)
#pragma prefast(disable:6001, "Using uninitialized memory 's'") // prefast complains about initialization of s with 0 via s = s^s in SSE2
static
void
ShiftValues_SSE2 (
    LZ77_INDEX *pDst,
    LZ77_INDEX *pSrc,
    uxint       uCount,
    uxint       uRemoveSize
)
{
    uxint       i;
    __m128i     s;
    __m128i     z;

    s.m128i_u32[0] = (UInt32) uRemoveSize;
    s.m128i_u32[1] = (UInt32) uRemoveSize;
    s.m128i_u32[2] = (UInt32) uRemoveSize;
    s.m128i_u32[3] = (UInt32) uRemoveSize;

    z = _mm_xor_si128 (z, z);

    for (i = 0; i < uCount; i += 4*4)
    {
        __m128i v0, t0;
        __m128i v1, t1;
        __m128i v2, t2;
        __m128i v3, t3;

        v0 = _mm_load_si128 ((__m128i *) (pSrc + i));
        v1 = _mm_load_si128 ((__m128i *) (pSrc + i + 4*1));
        v2 = _mm_load_si128 ((__m128i *) (pSrc + i + 4*2));
        v3 = _mm_load_si128 ((__m128i *) (pSrc + i + 4*3));

        v0 = _mm_sub_epi32 (v0, s);
        v1 = _mm_sub_epi32 (v1, s);
        v2 = _mm_sub_epi32 (v2, s);
        v3 = _mm_sub_epi32 (v3, s);

        t0 = _mm_srai_epi32 (v0, 31);
        t1 = _mm_srai_epi32 (v1, 31);
        t2 = _mm_srai_epi32 (v2, 31);
        t3 = _mm_srai_epi32 (v3, 31);

        t0 = _mm_andnot_si128 (t0, v0);
        t1 = _mm_andnot_si128 (t1, v1);
        t2 = _mm_andnot_si128 (t2, v2);
        t3 = _mm_andnot_si128 (t3, v3);

        _mm_store_si128 ((__m128i *) (pDst + i), t0);
        _mm_store_si128 ((__m128i *) (pDst + i + 4*1), t1);
        _mm_store_si128 ((__m128i *) (pDst + i + 4*2), t2);
        _mm_store_si128 ((__m128i *) (pDst + i + 4*3), t3);
    }
}
#pragma prefast(pop)
#endif /* XPRESS9_USE_SSE2 */


static
void
ShiftWindow (
    LZ77_PASS1_STATE    *pState
)
{
    uxint   uWindowSize;
    uxint   uKeepSize;
    uxint   uRemoveSize;

    uWindowSize = STATE.m_EncodeData.m_uWindowSize;
    uKeepSize = uWindowSize >> 1; // Keep half of the previous window size, why?
    uRemoveSize = uWindowSize - uKeepSize; // Remove the other half.
#if XPRESS9_USE_SSE2
    if (STATE.m_uRuntimeFlags & Xpress9Flag_UseSSE2)
    {
        ShiftValues_SSE2 (pState->m_uNext, pState->m_uNext + uRemoveSize, uKeepSize, uRemoveSize);
        ShiftValues_SSE2 (STATE.m_HashTable.m_pHashTable, STATE.m_HashTable.m_pHashTable, STATE.m_HashTable.m_uHashTableSizeCurrent, uRemoveSize);
    }
    else
#endif /* XPRESS9_USE_SSE2 */
    {
        ShiftValues (pState->m_uNext, pState->m_uNext + uRemoveSize, uKeepSize, uRemoveSize);
        // Update the hash table pointers and remove obsolete pointers that dont belong in the new window.
        ShiftValues (STATE.m_HashTable.m_pHashTable, STATE.m_HashTable.m_pHashTable, STATE.m_HashTable.m_uHashTableSizeCurrent, uRemoveSize);
    }
    // Actually shift the window. The sliding window is maintained in the m_pData buffer which holds some part of the 
    // data in its original form.
    memcpy (STATE.m_EncodeData.m_pData, STATE.m_EncodeData.m_pData + uRemoveSize, uKeepSize);

    ASSERT (
        STATE.m_EncodeData.m_uEncodePosition >= uRemoveSize,
        "uEncodePosition=%Iu uWindowSize=%Iu uKeepSize=%Iu",
        STATE.m_EncodeData.m_uEncodePosition,
        uWindowSize,
        uKeepSize
    );
    // Update the encode position 
    STATE.m_EncodeData.m_uEncodePosition -= uRemoveSize;

    ASSERT (
        STATE.m_EncodeData.m_uHashInsertPosition >= uRemoveSize,
        "uHashInsertPosition=%Iu uWindowSize=%Iu uKeepSize=%Iu",
        STATE.m_EncodeData.m_uHashInsertPosition,
        uWindowSize,
        uKeepSize
    );
    STATE.m_EncodeData.m_uHashInsertPosition -= uRemoveSize;


    ASSERT (
        STATE.m_EncodeData.m_uDataSize >= uRemoveSize,
        "uHashInsertPosition=%Iu uWindowSize=%Iu uKeepSize=%Iu",
        STATE.m_EncodeData.m_uDataSize,
        uWindowSize,
        uKeepSize
    );
    STATE.m_EncodeData.m_uDataSize -= uRemoveSize;
}


static
void
StartFlushing (
    XPRESS9_STATUS     *pStatus,
    LZ77_PASS1_STATE   *pState
)
{
    uxint   uShortSymbolCost;
    uxint   uLongLengthCost;
    uxint   uHeaderCost;
    uxint   uEncodedHuffmanTablesBits;
    UInt8  *pDst;
    uxint   uFlags;
    uxint   i;
    MSB_T   uMsb;
    xint    iOffset;
    BIO_STATE BioState;
    BIO_DECLARE();

    ASSERT (STATE.m_uState == LZ77_ENCODER_STATE_ENCODING, "");
    ASSERT (STATE.m_Pass2.m_Ir.m_pIrPtr > STATE.m_Pass2.m_Ir.m_pIrBuffer, "");
    ASSERT (STATE.m_Pass2.m_Ir.m_pIrPtr <= STATE.m_Pass2.m_Ir.m_pIrBuffer + STATE.m_Pass2.m_Ir.m_uIrBufferSize, "");

    pDst = STATE.m_Scratch.m_pScratchArea;
    BIOWR_INITIALIZE_SHIFT_REGISTER ();
    BIOWR_SET_OUTPUT_PTR (pDst + 8*4); // Leave the first 32 bytes ( 8 32 bit numbers of the scratch area for the header information ).
    if (STATE.m_Params.m_Current.m_uMtfEntryCount > 0)
    {
        //
        // This one is just passed to the decoder. It is also read back by the decoder, but is not used.
        //
        iOffset = STATE.m_Pass2.m_Ir.m_Mtf.m_iMtfLastPtr & 1;
        BIOWR (iOffset, 1);
        for (i = 0; i < STATE.m_Params.m_Current.m_uMtfEntryCount; ++i)
        {
            iOffset = STATE.m_Pass2.m_Ir.m_Mtf.m_iMtfOffset[i];
            ASSERT (iOffset < 0, "");
            iOffset = -iOffset;
            GET_MSB (uMsb, iOffset);
            iOffset ^= POWER2 (uMsb);
            BIOWR (uMsb, 5);
            BIOWR (iOffset, uMsb);
        }
    }
    uHeaderCost = (BIOWR_TELL_OUTPUT_PTR () - pDst) * 8 + BIOWR_TELL_SHIFT_REGISTER_BITS ();
    BIO_STATE_LEAVE (BioState);

    DEBUG_PERF_START (STATE.m_DebugPerf.m_uHuffman);
    Xpress9HuffmanCreateAndEncodeTable (
        pStatus,
        &BioState,
        STATE.m_HuffmanStat.m_uShortSymbolCount,
        LZ77_SHORT_SYMBOL_ALPHABET_SIZE,
        HUFFMAN_MAX_CODEWORD_LENGTH,
        STATE.m_Scratch.m_HuffmanNodeTemp,
        STATE.m_Pass2.m_uShortSymbolMask,
        &uShortSymbolCost,
        LZ77_MAX_SHORT_LENGTH
    );
    if (pStatus->m_uStatus != Xpress9Status_OK)
    {
        DEBUG_PERF_STOP (STATE.m_DebugPerf.m_uHuffman);
        goto Failure;
    }

    Xpress9HuffmanCreateAndEncodeTable (
        pStatus,
        &BioState,
        STATE.m_HuffmanStat.m_uLongLengthCount,
        LZ77_LONG_LENGTH_ALPHABET_SIZE,
        HUFFMAN_MAX_CODEWORD_LENGTH,
        STATE.m_Scratch.m_HuffmanNodeTemp,
        STATE.m_Pass2.m_uLongLengthMask,
        &uLongLengthCost,
        LZ77_MAX_SHORT_LENGTH
    );
    DEBUG_PERF_STOP (STATE.m_DebugPerf.m_uHuffman);
    if (pStatus->m_uStatus != Xpress9Status_OK)
        goto Failure;

    uEncodedHuffmanTablesBits = (BIOWR_TELL_OUTPUT_PTR_STATE (BioState) - pDst - 8*4) * 8 + BIOWR_TELL_SHIFT_REGISTER_BITS_STATE (BioState);

    STATE.m_uState = LZ77_ENCODER_STATE_FLUSHING;
    STATE.m_Pass2.m_Ir.m_pIrSrc = STATE.m_Pass2.m_Ir.m_pIrBuffer;

    STATE.m_Pass2.m_uComputedEncodedSizeBits = uHeaderCost + uShortSymbolCost + uLongLengthCost + STATE.m_HuffmanStat.m_uStoredBitCount;

    STATE.m_Pass2.m_uActualEncodedSizeBits = 0;

    STATE.m_Pass2.m_BioState = BioState;
    STATE.m_Pass2.m_uBytesCopiedFromScratch = 0;

    ((UInt32 *) pDst)[0] = XPRESS9_MAGIC;
    ((UInt32 *) pDst)[1] = (UInt32) STATE.m_Stat.m_Block.m_uOrigSize;
    ((UInt32 *) pDst)[2] = (UInt32) STATE.m_Pass2.m_uComputedEncodedSizeBits;


//     13 bits      length of encoded huffman tables in bits
//      3 bits      log2 of window size (16 .. 23)
//      2 bits      number of MTF entries   (0, 2, 4, reserved)
//      1 bit       min ptr match length (3 or 4)
//      1 bit       min mtf match length (2 or 3, shall be 0 if number of MTF entries is set to 0)
//
//     12 bits      reserved, must be 0

    uFlags = uEncodedHuffmanTablesBits & 0x1fff;

    uFlags |= ((STATE.m_Params.m_Current.m_uWindowSizeLog2 - 16) & 7) << 13;

    i = STATE.m_Params.m_Current.m_uMtfEntryCount;
    RETAIL_ASSERT ((i & 1) == 0 && i <= 4, "BadMtfEntryCount=%Iu", i);
    i >>= 1;
    uFlags |= i << 16;

    uFlags |= ((STATE.m_Params.m_Current.m_uPtrMinMatchLength - 3) & 1) << 18;
    uFlags |= ((STATE.m_Params.m_Current.m_uMtfMinMatchLength - 2) & 1) << 19;

    ((UInt32 *) pDst)[3] = (UInt32) uFlags;
    ((UInt32 *) pDst)[4] = 0;
    ((UInt32 *) pDst)[5] = STATE.m_Stat.m_uSessionSignature;
    ((UInt32 *) pDst)[6] = (UInt32) STATE.m_Stat.m_uBlockIndex;
    STATE.m_Stat.m_uBlockIndex += 1;

    ((UInt32 *) pDst)[7] = Xpress9Crc32 (pDst, 7*4, ((UInt32 *) pDst)[4]);

#if 1 && XPRESS9_DEBUG_PERF_STAT && XPRESS9_MAX_TRACE_LEVEL >= TRACE_LEVEL_TRACE
    {
        uxint i;
        for (i = 0; i < LZ77_SHORT_SYMBOL_ALPHABET_SIZE;)
        {
            if ((i & (LZ77_MAX_SHORT_LENGTH - 1)) == 0)
                printf ("%5Iu:", i);
            printf (
                "  %2Iu",
                (uxint) HUFFMAN_GET_CODEWORD_LENGTH (STATE.m_Pass2.m_uShortSymbolMask[i]),
                (uxint) STATE.m_HuffmanStat.m_uShortSymbolCount[i]
            );
            ++i;
            if ((i & (LZ77_MAX_SHORT_LENGTH - 1)) == 0)
                printf ("\n");
        }
    }
#endif


Failure:;
}


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
)
{
    LZ77_PASS1_STATE       *pState;
    uxint   uUserBytesAvailable;
    uxint   uIrBytesAvailable;
    uxint   uWindowBytesAvailable;
    uxint   uBytesToCopy;


    memset (pStatus, 0, sizeof (*pStatus));
    GET_ENCODER_STATE (pStatus, Failure, pState, pEncoder);

    if (STATE.m_uState != LZ77_ENCODER_STATE_ENCODING)
    {
        SET_ERROR (Xpress9Status_EncoderNotCompressing, pStatus, "Encoder->m_uState=%Iu", STATE.m_uState);
        goto Failure;
    }

    STATE.m_EncodeData.m_pData = STATE.m_BufferData.m_pBufferData;

    while (STATE.m_UserData.m_uUserDataSize > STATE.m_UserData.m_uUserDataProcessed)
    { // While there is still data left to be processed.
        ASSERT (
            STATE.m_EncodeData.m_uEncodePosition >= STATE.m_EncodeData.m_uHashInsertPosition,
            "m_uEncodePosition=%Iu m_uHashInsertPosition=%Iu",
            STATE.m_EncodeData.m_uEncodePosition,
            STATE.m_EncodeData.m_uHashInsertPosition
        );
        ASSERT (
            STATE.m_EncodeData.m_uWindowSize >= STATE.m_EncodeData.m_uDataSize,
            "m_uWindowSize=%Iu m_uDataSize=%Iu",
            STATE.m_EncodeData.m_uWindowSize,
            STATE.m_EncodeData.m_uDataSize
        );

        //
        // Get the number of bytes yet available for processing
        //
        uUserBytesAvailable = STATE.m_UserData.m_uUserDataSize - STATE.m_UserData.m_uUserDataProcessed;


        // Compute remaining capacity of IrBuffer: it should be able to represent each incoming byte + room for last unhashed values
        uIrBytesAvailable = STATE.m_Pass2.m_Ir.m_uIrBufferSize - (uxint)(STATE.m_Pass2.m_Ir.m_pIrPtr - STATE.m_Pass2.m_Ir.m_pIrBuffer);
        uIrBytesAvailable = uIrBytesAvailable / 3;    // One input byte may generate 3 bytes in internal representation? 

        //
        // first see whether we need to flush internal representation
        //
        if ( uIrBytesAvailable < 512 )
        {
            // start flushing the data
            goto Flushing;
        }
        uIrBytesAvailable -= 256; // We still need room to push last un-hashed data 

        //
        // see whether we need to shift the window
        //
        uWindowBytesAvailable = STATE.m_EncodeData.m_uWindowSize - STATE.m_EncodeData.m_uDataSize;
        if (uWindowBytesAvailable < 256 && uWindowBytesAvailable < uUserBytesAvailable)
        {
            DEBUG_PERF_START (STATE.m_DebugPerf.m_uShift);
            ShiftWindow (pState); // BUGBUG: This changes the window, does not reduce windowSize.
            uWindowBytesAvailable = STATE.m_EncodeData.m_uWindowSize - STATE.m_EncodeData.m_uDataSize;
            ASSERT (uWindowBytesAvailable >= 256, "uWindowBytesAvailable=%Iu", uWindowBytesAvailable);
            DEBUG_PERF_STOP  (STATE.m_DebugPerf.m_uShift);
        }

        // At this point, windowBytesAvailable is at least 256.
        //
        // copy new bytes if necessary
        //
        uBytesToCopy = uIrBytesAvailable;
        if (uBytesToCopy > uWindowBytesAvailable)
            uBytesToCopy = uWindowBytesAvailable;
        if (uBytesToCopy > uUserBytesAvailable)
            uBytesToCopy = uUserBytesAvailable;

        ASSERT (uBytesToCopy != 0, "");

        if (pCallback != NULL)
        {
            DEBUG_PERF_START (STATE.m_DebugPerf.m_uCallback);
            pCallback (STATE.m_UserData.m_pUserData + STATE.m_UserData.m_uUserDataProcessed, (unsigned) uBytesToCopy, pCallbackContext);
            DEBUG_PERF_STOP  (STATE.m_DebugPerf.m_uCallback);
        }

        // (pavane) I think we need to have the following check
        //if ( STATE.m_EncodeData.m_pData == STATE.m_BufferData.m_pBufferData )
        //{
        //    ASSERT (STATE.m_EncodeData.m_uDataSize + uBytesToCopy <= STATE.m_BufferData.m_uBufferDataSize, "" );
        //}
        
        DEBUG_PERF_START (STATE.m_DebugPerf.m_uMemcpy);
        memcpy (
            STATE.m_EncodeData.m_pData + STATE.m_EncodeData.m_uDataSize,
            STATE.m_UserData.m_pUserData + STATE.m_UserData.m_uUserDataProcessed,
            uBytesToCopy
        );
        DEBUG_PERF_STOP  (STATE.m_DebugPerf.m_uMemcpy);

        STATE.m_EncodeData.m_uDataSize += uBytesToCopy;
        STATE.m_UserData.m_uUserDataProcessed += uBytesToCopy;

        //
        // At this point we have the sliding window data in the m_EncodeData.m_pData
        // For every byte in the sliding window : EncInsert inserts it into the
        // hash table.
        //
        DEBUG_PERF_START (STATE.m_DebugPerf.m_uInsert);
        (*STATE.m_Params.m_pLz77EncInsert) (pState);
        DEBUG_PERF_STOP  (STATE.m_DebugPerf.m_uInsert);

        if (STATE.m_Pass2.m_Ir.m_pIrPtr == STATE.m_Pass2.m_Ir.m_pIrBuffer)
        {
            // this is the first write to the IR buffer; so clear statistics
            memset (&STATE.m_HuffmanStat, 0, sizeof (STATE.m_HuffmanStat));
            STATE.m_Pass2.m_Ir.m_Mtf = STATE.m_EncodeData.m_Mtf;
        }

        STATE.m_Stat.m_Block.m_uOrigSize -= STATE.m_EncodeData.m_uEncodePosition;
        //
        // Encode pass1 populates the m_Pass2.m_Ir.m_pIrBuffer for use by pass2.
        //
        DEBUG_PERF_START (STATE.m_DebugPerf.m_uPass1);
        (*STATE.m_Params.m_pLz77EncPass1) (pState);
        DEBUG_PERF_STOP  (STATE.m_DebugPerf.m_uPass1);
        STATE.m_Stat.m_Block.m_uOrigSize += STATE.m_EncodeData.m_uEncodePosition;
    }

    if (STATE.m_UserData.m_fFlush)
    {
        UInt16         *pIrPtr;
        const UInt8    *pData;
        uxint           uPosition;

        //
        // encode few last symbols that were not hashed as literals (we should have enough space for them) 
        // see comment in EncInsert.i
        //
        pData     = STATE.m_EncodeData.m_pData;
        uPosition = STATE.m_EncodeData.m_uEncodePosition;
        pIrPtr    = (UInt16 *) STATE.m_Pass2.m_Ir.m_pIrPtr;

        // we should have 256 bytes available in IR, more than enough to store 8 literals or less
        RETAIL_ASSERT (
            STATE.m_EncodeData.m_uDataSize < 8 + uPosition ,
            "uDataSize=%Iu uPosition=%Iu",
            STATE.m_EncodeData.m_uDataSize,
            uPosition
            );

        while (uPosition < STATE.m_EncodeData.m_uDataSize)
        {
            ENCODE_LIT (uPosition);
            uPosition += 1;
            STATE.m_EncodeData.m_Mtf.m_iMtfLastPtr = 0;
            STATE.m_Stat.m_Block.m_uOrigSize += 1;
        }

        STATE.m_Pass2.m_Ir.m_pIrPtr = (UInt8 *) pIrPtr;
        STATE.m_EncodeData.m_uEncodePosition = uPosition;

#if XPRESS9_DEBUG_PERF_STAT
        {
            UInt64 uTotal;
            uTotal =
                STATE.m_DebugPerf.m_uCallback +
                STATE.m_DebugPerf.m_uMemcpy +
                STATE.m_DebugPerf.m_uInsert +
                STATE.m_DebugPerf.m_uShift +
                STATE.m_DebugPerf.m_uPass1 +
                STATE.m_DebugPerf.m_uHuffman +
                STATE.m_DebugPerf.m_uPass2
            ;
            if (uTotal == 0)
                uTotal = 1;
#define P(f) printf ("%-15s =%12I64u %6.2f%%\n", "Enc" #f, STATE.m_DebugPerf.m_u##f, (100.0 * (double) STATE.m_DebugPerf.m_u##f) / uTotal);
            P (Callback);
            P (Memcpy);
            P (Insert);
            P (Shift);
            P (Pass1);
            P (Huffman);
            P (Pass2);
            printf ("%-15s =%12I64u %6.2f%%\n\n", "EncTotal", uTotal, 100.0);
#undef P
        }
#endif /* XPRESS9_DEBUG_PERF_STAT */

        //
        // now orig 
        //
        if (STATE.m_Pass2.m_Ir.m_pIrPtr != STATE.m_Pass2.m_Ir.m_pIrBuffer)
        {
            //
            // If we do not have any more space in the IR buffer, flush whatever we have. To do this,
            // we first need to write the huffman tables to the scratch area. The huffman tables 
            // are then copied to the final output when FetchCompressedData is called. This also
            // causes the actual pass2 huffman encoding of data in the IR buffer and the encoded
            // data gets written to the output buffer. Each such block of data containing a header,
            // huffman tables and the encoded data is called a compresed block. A single data buffer 
            // can be compressed to multiple compression blocks in the final output.
            // 
            goto Flushing;
        }
    }

Failure:
    return (0);

Flushing:
    StartFlushing (pStatus, pState);
    return ((unsigned) ((STATE.m_Pass2.m_uComputedEncodedSizeBits + 7) >> 3));
}



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
)
{
    LZ77_PASS1_STATE   *pState;
    LZ77_PASS2_STATE   *pPass2;
    UInt8              *pDst;
    uxint               uDstSize;
    uxint               uCopyBytes;
    uxint               uMaxIrOutput;
    uxint               uCompBytesWritten;


    *puCompBytesWritten = 0;
    uCompBytesWritten = 0;

    pDst = (UInt8 *) pCompData;
    uDstSize = uCompDataBufferSize;

    memset (pStatus, 0, sizeof (*pStatus));
    GET_ENCODER_STATE (pStatus, Failure, pState, pEncoder);

    DEBUG_PERF_START (STATE.m_DebugPerf.m_uPass2);

    pPass2 = &STATE.m_Pass2;

    if (STATE.m_uState != LZ77_ENCODER_STATE_FLUSHING)
    {
        SET_ERROR (Xpress9Status_EncoderNotFlushing, pStatus, "Encoder->m_uState=%Iu", STATE.m_uState);
        goto Failure;
    }

    RETAIL_ASSERT (
        pPass2->m_Ir.m_pIrSrc < STATE.m_Pass2.m_Ir.m_pIrPtr || pPass2->m_uBytesCopiedFromScratch < (uxint) (BIOWR_TELL_OUTPUT_PTR_STATE (pPass2->m_BioState) - STATE.m_Scratch.m_pScratchArea),
        ""
    );

    //
    // first, copy data buffered in scratch area into user buffer
    //
    RETAIL_ASSERT (
        BIOWR_TELL_OUTPUT_PTR_STATE (pPass2->m_BioState) > STATE.m_Scratch.m_pScratchArea && 
        BIOWR_TELL_OUTPUT_PTR_STATE (pPass2->m_BioState) <= STATE.m_Scratch.m_pScratchArea + STATE.m_Scratch.m_uScratchAreaSize,
        ""
    );

    uCopyBytes = BIOWR_TELL_OUTPUT_PTR_STATE (pPass2->m_BioState) - STATE.m_Scratch.m_pScratchArea;
    RETAIL_ASSERT (uCopyBytes >= pPass2->m_uBytesCopiedFromScratch, "");
    uCopyBytes -= pPass2->m_uBytesCopiedFromScratch;
    if (uCopyBytes > uDstSize)
        uCopyBytes = uDstSize;

    memcpy (pDst, STATE.m_Scratch.m_pScratchArea + pPass2->m_uBytesCopiedFromScratch, uCopyBytes);
    pPass2->m_uBytesCopiedFromScratch += uCopyBytes;
    uDstSize                          -= uCopyBytes;
    pDst                              += uCopyBytes;
    uCompBytesWritten                 += uCopyBytes;
    pPass2->m_uActualEncodedSizeBits  += uCopyBytes << 3;

    if (uDstSize == 0)
        goto Done;

    // now do pass2 encoding until we fill the user buffer
    BIOWR_SET_OUTPUT_PTR_STATE (pPass2->m_BioState, pDst);

    while (uDstSize > 128)
    {
        //
        // in fact, at most 16 bits in IR can produce 27 bits out, but 1:2 ratio is acceptable approximation
        //
        uMaxIrOutput = (pPass2->m_Ir.m_pIrPtr - pPass2->m_Ir.m_pIrSrc) << 1;
        if (uMaxIrOutput == 0)
            break;

        if (uMaxIrOutput > uDstSize - 32)
            uMaxIrOutput = uDstSize - 32;

        uMaxIrOutput >>= 1;
        if (uMaxIrOutput > (uxint) (pPass2->m_Ir.m_pIrPtr - pPass2->m_Ir.m_pIrSrc))
            uMaxIrOutput = (uxint) (pPass2->m_Ir.m_pIrPtr - pPass2->m_Ir.m_pIrSrc);

        (*STATE.m_Params.m_pLz77EncPass2) (pPass2, pPass2->m_Ir.m_pIrSrc + uMaxIrOutput);

        uCopyBytes = BIOWR_TELL_OUTPUT_PTR_STATE (pPass2->m_BioState) - pDst;
        RETAIL_ASSERT (uCopyBytes < uDstSize, "uCopyBytes=%Iu uDstSize=%Iu", uCopyBytes, uDstSize);
        uDstSize          -= uCopyBytes;
        uCompBytesWritten += uCopyBytes;
        pDst      = BIOWR_TELL_OUTPUT_PTR_STATE (pPass2->m_BioState);
        pPass2->m_uActualEncodedSizeBits  += uCopyBytes << 3;

        if (pPass2->m_Ir.m_pIrSrc >= pPass2->m_Ir.m_pIrPtr)
        {
            // encoded everything; check size
            // The last byte may be incomplete, add correction for the unfilled bits in the last byte.
            pPass2->m_uActualEncodedSizeBits -= (- (xint) BIOWR_TELL_SHIFT_REGISTER_BITS_STATE (pPass2->m_BioState)) & 7;
            RETAIL_ASSERT (
                pPass2->m_uActualEncodedSizeBits == pPass2->m_uComputedEncodedSizeBits,
                "ComputedEncodedBits=%Iu ActualEncodedBits=%Iu",
                pPass2->m_uComputedEncodedSizeBits,
                pPass2->m_uActualEncodedSizeBits
            );
        }
    }


    // now, encode into scratch buffer
    pPass2->m_uBytesCopiedFromScratch = 0;
    BIOWR_SET_OUTPUT_PTR_STATE (pPass2->m_BioState, STATE.m_Scratch.m_pScratchArea);
    uMaxIrOutput = (pPass2->m_Ir.m_pIrPtr - pPass2->m_Ir.m_pIrSrc) << 1;
    if (uMaxIrOutput != 0)
    {
        if (uMaxIrOutput > STATE.m_Scratch.m_uScratchAreaSize - 32)
            uMaxIrOutput = STATE.m_Scratch.m_uScratchAreaSize - 32;

        uMaxIrOutput >>= 1;
        if (uMaxIrOutput > (uxint) (pPass2->m_Ir.m_pIrPtr - pPass2->m_Ir.m_pIrSrc))
            uMaxIrOutput = (uxint) (pPass2->m_Ir.m_pIrPtr - pPass2->m_Ir.m_pIrSrc);

        (*STATE.m_Params.m_pLz77EncPass2) (pPass2, pPass2->m_Ir.m_pIrSrc + uMaxIrOutput);

        //
        // copy data buffered in scratch area into user buffer
        //
        RETAIL_ASSERT (
            BIOWR_TELL_OUTPUT_PTR_STATE (pPass2->m_BioState) >= STATE.m_Scratch.m_pScratchArea && 
            BIOWR_TELL_OUTPUT_PTR_STATE (pPass2->m_BioState) < STATE.m_Scratch.m_pScratchArea + STATE.m_Scratch.m_uScratchAreaSize - 16,
            ""
        );

        uCopyBytes = BIOWR_TELL_OUTPUT_PTR_STATE (pPass2->m_BioState) - STATE.m_Scratch.m_pScratchArea;
        if (uCopyBytes > uDstSize)
            uCopyBytes = uDstSize;

        memcpy (pDst, STATE.m_Scratch.m_pScratchArea, uCopyBytes);
        pPass2->m_uBytesCopiedFromScratch  = uCopyBytes;
        uDstSize                          -= uCopyBytes;
        pDst                              += uCopyBytes;
        uCompBytesWritten                 += uCopyBytes;
        pPass2->m_uActualEncodedSizeBits  += uCopyBytes << 3;

        if (pPass2->m_Ir.m_pIrSrc >= pPass2->m_Ir.m_pIrPtr)
        {
            // encoded everything; check size
            pPass2->m_uActualEncodedSizeBits -= (- (xint) BIOWR_TELL_SHIFT_REGISTER_BITS_STATE (pPass2->m_BioState)) & 7;
            uCopyBytes = BIOWR_TELL_OUTPUT_PTR_STATE (pPass2->m_BioState) - STATE.m_Scratch.m_pScratchArea;
            uCopyBytes -= pPass2->m_uBytesCopiedFromScratch;
            RETAIL_ASSERT (
                pPass2->m_uActualEncodedSizeBits + (uCopyBytes << 3) == pPass2->m_uComputedEncodedSizeBits,
                "ComputedEncodedBits=%Iu ScratchBits=%Iu ActualEncodedBits=%Iu",
                pPass2->m_uComputedEncodedSizeBits,
                uCopyBytes << 3,
                pPass2->m_uActualEncodedSizeBits
            );
        }
    }

Done:
    // see whether we have something left
    if (
        pPass2->m_Ir.m_pIrSrc >= pPass2->m_Ir.m_pIrPtr && 
        pPass2->m_uBytesCopiedFromScratch >= (uxint) (BIOWR_TELL_OUTPUT_PTR_STATE (pPass2->m_BioState) - STATE.m_Scratch.m_pScratchArea)
    )
    {
        RETAIL_ASSERT (
            pPass2->m_uActualEncodedSizeBits >= pPass2->m_uComputedEncodedSizeBits,
            "ActualEncodedSizeBits=%Iu EncodedSizeBits=%Iu",
            pPass2->m_uActualEncodedSizeBits,
            pPass2->m_uComputedEncodedSizeBits
        );

        ASSERT (pPass2->m_Ir.m_pIrSrc == pPass2->m_Ir.m_pIrPtr, "");
        pPass2->m_Ir.m_pIrPtr = pPass2->m_Ir.m_pIrBuffer;

        STATE.m_uState = LZ77_ENCODER_STATE_ENCODING;

        STATE.m_Stat.m_Session.m_uCount    += 1;
        STATE.m_Stat.m_Session.m_uCompSize += (STATE.m_Pass2.m_uComputedEncodedSizeBits + 7) >> 3;
        STATE.m_Stat.m_Session.m_uOrigSize += STATE.m_Stat.m_Block.m_uOrigSize;
        memset (&STATE.m_Stat.m_Block, 0, sizeof (STATE.m_Stat.m_Block));
    }
    else
    {
        RETAIL_ASSERT (
            pPass2->m_uActualEncodedSizeBits < pPass2->m_uComputedEncodedSizeBits,
            "ActualEncodedSizeBits=%Iu EncodedSizeBits=%Iu",
            pPass2->m_uActualEncodedSizeBits,
            pPass2->m_uComputedEncodedSizeBits
        );
        RETAIL_ASSERT (
            pPass2->m_Ir.m_pIrSrc < pPass2->m_Ir.m_pIrPtr ||
            pPass2->m_uBytesCopiedFromScratch < (uxint) (BIOWR_TELL_OUTPUT_PTR_STATE (pPass2->m_BioState) - STATE.m_Scratch.m_pScratchArea),
            ""
        );
    }

    *puCompBytesWritten = (unsigned) uCompBytesWritten;

    DEBUG_PERF_STOP (STATE.m_DebugPerf.m_uPass2);

    return ((unsigned) (((pPass2->m_uComputedEncodedSizeBits - pPass2->m_uActualEncodedSizeBits) + 7) >> 3));

Failure:
    return (0);
}


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
)
{
    LZ77_PASS1_STATE   *pState;

    memset (pStatus, 0, sizeof (*pStatus));
    GET_ENCODER_STATE (pStatus, Failure, pState, pEncoder);

    if (STATE.m_uState != LZ77_ENCODER_STATE_ENCODING)
    {
        SET_ERROR (
            Xpress9Status_EncoderNotCompressing,
            pStatus,
            "EncoderState=%u",
            STATE.m_uState
        );
        goto Failure;
    }

    if (pOrigData != STATE.m_UserData.m_pUserData || uOrigDataSize != STATE.m_UserData.m_uUserDataSize)
    {
        SET_ERROR (
            Xpress9Status_BadArgument,
            pStatus,
            "AttachedOrigData=%p OrigData=%p AttachedOrigSize=%Iu OrigSize=%u",
            STATE.m_UserData.m_pUserData,
            pOrigData,
            STATE.m_UserData.m_uUserDataSize,
            uOrigDataSize
        );
        goto Failure;
    }

    if (STATE.m_UserData.m_uUserDataProcessed != uOrigDataSize)
    {
        SET_ERROR (
            Xpress9Status_EncoderCompressing,
            pStatus,
            "OrigSize=%u ProcessedOrigSize=%Iu",
            uOrigDataSize,
            STATE.m_UserData.m_uUserDataProcessed
        );
        goto Failure;
    }

    if (STATE.m_UserData.m_fFlush && STATE.m_Pass2.m_Ir.m_pIrPtr != STATE.m_Pass2.m_Ir.m_pIrBuffer)
    {
        SET_ERROR (
            Xpress9Status_EncoderCompressing,
            pStatus,
            "InternalBufferNotEmpty"
        );
        goto Failure;
    }

    memset (&STATE.m_UserData, 0, sizeof (STATE.m_UserData));

    STATE.m_uState = LZ77_ENCODER_STATE_READY;

Failure:;
}


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
)
{
    LZ77_PASS1_STATE   *pState;
    void               *pAllocatedMemory;

    memset (pStatus, 0, sizeof (*pStatus));
    GET_ENCODER_STATE (pStatus, Failure, pState, pEncoder);
    
    pAllocatedMemory = STATE.m_pAllocatedMemory;
    memset (&STATE, 0, sizeof (STATE));
    pFreeFn (pFreeContext, pAllocatedMemory);

Failure:;
}
