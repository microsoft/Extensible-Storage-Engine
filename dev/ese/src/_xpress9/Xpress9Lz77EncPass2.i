// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
static
void
Xpress9Lz77EncPass2 (
    LZ77_PASS2_STATE *pPass2,
    const void       *pIrStop
)
{
/* Encodes the data */
    uxint uBits;
    uxint uValue;
    uxint uSymbol;
    const UInt16 *pIrSrc = (const UInt16 *) pPass2->m_Ir.m_pIrSrc;
    BIO_DECLARE();

    BIO_STATE_ENTER (pPass2->m_BioState);


    do
    {
        TRACE ("P2 %p %u", pIrSrc, pIrSrc[0]);
        uSymbol = pIrSrc[0];
        ASSERT (uSymbol < LZ77_SHORT_SYMBOL_ALPHABET_SIZE, "uSymbol=%Iu", uSymbol);
        pIrSrc += 1;

        HUFFMAN_WRITE_SYMBOL (pPass2->m_uShortSymbolMask[uSymbol]);

        if (uSymbol >= 256)
        {
            //
            // it is a pointer (explicit or MTF)
            //

            //
            // encode length
            //
            uBits = uSymbol & (LZ77_MAX_SHORT_LENGTH - 1);
            if (uBits >= LZ77_MAX_SHORT_LENGTH - 1)
            {
                uBits = pIrSrc[0];
                ASSERT (uBits < LZ77_LONG_LENGTH_ALPHABET_SIZE, "uBits=%Iu", uBits);
                pIrSrc += 1;

                HUFFMAN_WRITE_SYMBOL (pPass2->m_uLongLengthMask[uBits]);

                if (uBits >= LZ77_MAX_LONG_LENGTH)
                {
                    uBits -= LZ77_MAX_LONG_LENGTH;

                    if (uBits > 16)
                    {
                        uValue = * (UInt32 *) pIrSrc;
                        pIrSrc += 2;
                    }
                    else
                    {
                        uValue = pIrSrc[0];
                        pIrSrc += 1;
                    }

                    BIOWR (uValue, uBits);
                }
            }

#if LZ77_MTF > 0
            if (uSymbol >= 256 + (LZ77_MTF << LZ77_MAX_SHORT_LENGTH_LOG))
#endif
            {
                //
                // encode offset
                //
                // uBits corresponds to uMsbOffset in ENCODE_PTR
                uBits = (uSymbol >> LZ77_MAX_SHORT_LENGTH_LOG) - ((256 >> LZ77_MAX_SHORT_LENGTH_LOG) + LZ77_MTF);

                if (uBits > 16)
                {
                    uValue = * (UInt32 *) pIrSrc;
                    pIrSrc += 2;
                }
                else
                {
                    uValue = pIrSrc[0];
                    pIrSrc += 1;
                }
                
                BIOWR (uValue, uBits);
            }
        }
    }
    while (pIrSrc < (const UInt16 *) pIrStop);

    if (pIrSrc >= (const UInt16 *) pPass2->m_Ir.m_pIrPtr)
    {
        ASSERT (pIrSrc == (const UInt16 *) pPass2->m_Ir.m_pIrPtr, "");
        BIOWR_FLUSH ();
    }


    BIO_STATE_LEAVE (pPass2->m_BioState);

    pPass2->m_Ir.m_pIrSrc = (const UInt8 *) pIrSrc;
}


#undef LZ77_MTF
#undef Xpress9Lz77EncPass2
