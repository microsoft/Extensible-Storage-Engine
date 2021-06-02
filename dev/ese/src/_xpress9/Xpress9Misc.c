// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "Xpress9Internal.h"


const char *
Xpress9GetErrorText (
    unsigned uErrorCode
)
{
    switch (uErrorCode)
    {
    case Xpress9Status_OK:
        return ("OK");
    case Xpress9Status_NotEnoughMemory:
        return ("NotEnoughMemory");
    case Xpress9Status_BadArgument:
        return ("BadArgument");
    case Xpress9Status_CorruptedMemory:
        return ("CorruptedMemory");

    case Xpress9Status_EncoderNotEmpty:
        return ("EncoderNotEmpty");
    case Xpress9Status_EncoderNotReady:
        return ("EncoderNotReady");
    case Xpress9Status_EncoderNotCompressing:
        return ("EncoderNotCompressing");
    case Xpress9Status_EncoderNotFlushing:
        return ("EncoderNotFlushing");
    case Xpress9Status_EncoderCompressing:
        return ("EncoderStillCompressing");

    case Xpress9Status_DecoderCorruptedData:
        return ("DecoderCorruptedData");
    case Xpress9Status_DecoderCorruptedHeader:
        return ("DecoderCorruptedHeader");
    case Xpress9Status_DecoderWindowTooLarge:
        return ("DecoderWindowTooLarge");
    case Xpress9Status_DecoderUnknownFormat:
        return ("DecoderUnknownFormat");
    case Xpress9Status_DecoderNotDetached:
        return ("DecoderNotDetached");
    case Xpress9Status_DecoderNotDrained:
        return ("DecoderNotDrained");
    }

    return ("UnknownErrorCode");
}

#define s_uHuffmanBitReverseTableLog 6
//for each index contains the value obtained by reversing the 6-bit representation of the index.
__declspec(align(64))
static
UInt8
s_uHuffmanBitReverseTable[64] =
{
    /*   0 */   0,
    /*   1 */  32,
    /*   2 */  16,
    /*   3 */  48,
    /*   4 */   8,
    /*   5 */  40,
    /*   6 */  24,
    /*   7 */  56,
    /*   8 */   4,
    /*   9 */  36,
    /*  10 */  20,
    /*  11 */  52,
    /*  12 */  12,
    /*  13 */  44,
    /*  14 */  28,
    /*  15 */  60,
    /*  16 */   2,
    /*  17 */  34,
    /*  18 */  18,
    /*  19 */  50,
    /*  20 */  10,
    /*  21 */  42,
    /*  22 */  26,
    /*  23 */  58,
    /*  24 */   6,
    /*  25 */  38,
    /*  26 */  22,
    /*  27 */  54,
    /*  28 */  14,
    /*  29 */  46,
    /*  30 */  30,
    /*  31 */  62,
    /*  32 */   1,
    /*  33 */  33,
    /*  34 */  17,
    /*  35 */  49,
    /*  36 */   9,
    /*  37 */  41,
    /*  38 */  25,
    /*  39 */  57,
    /*  40 */   5,
    /*  41 */  37,
    /*  42 */  21,
    /*  43 */  53,
    /*  44 */  13,
    /*  45 */  45,
    /*  46 */  29,
    /*  47 */  61,
    /*  48 */   3,
    /*  49 */  35,
    /*  50 */  19,
    /*  51 */  51,
    /*  52 */  11,
    /*  53 */  43,
    /*  54 */  27,
    /*  55 */  59,
    /*  56 */   7,
    /*  57 */  39,
    /*  58 */  23,
    /*  59 */  55,
    /*  60 */  15,
    /*  61 */  47,
    /*  62 */  31,
    /*  63 */  63,
};

XPRESS9_INTERNAL
uxint
HuffmanReverseMask (
    uxint   uMask,
    uxint   uBits
)
{
    uxint uNewMask;

    ASSERT (uMask < (((uxint) 1) << uBits), "uMask=0x%Ix uBits=%Iu", uMask, uBits);

    uNewMask = 0;
    while (uBits >= s_uHuffmanBitReverseTableLog)
    {
        uNewMask = (uNewMask << s_uHuffmanBitReverseTableLog) + s_uHuffmanBitReverseTable[uMask & ((1 << s_uHuffmanBitReverseTableLog) - 1)];
        uMask >>= s_uHuffmanBitReverseTableLog;
        uBits -= s_uHuffmanBitReverseTableLog;
    }

    if (uBits != 0)
    {
        uNewMask = (uNewMask << s_uHuffmanBitReverseTableLog) + s_uHuffmanBitReverseTable[uMask & ((1 << s_uHuffmanBitReverseTableLog) - 1)];
        uNewMask >>= s_uHuffmanBitReverseTableLog - uBits;
    }

    return (uNewMask);
}

//
// Generating polynomial for Crc32 is
// 0x82f63b78 =
//              x^32 + x^28 + x^27 + x^26 + x^25 + x^23 + x^22 + x^20 + x^19 + x^18 + x^14 + 
//                   + x^13 + x^11 + x^10 + x^9  + x^8  + x^6  + x^0 
//
__declspec(align(1024))
static
UInt32
Xpress9Crc32Table[256] =
{
   0x00000000u, 0xf26b8303u, 0xe13b70f7u, 0x1350f3f4u,
   0xc79a971fu, 0x35f1141cu, 0x26a1e7e8u, 0xd4ca64ebu,
   0x8ad958cfu, 0x78b2dbccu, 0x6be22838u, 0x9989ab3bu,
   0x4d43cfd0u, 0xbf284cd3u, 0xac78bf27u, 0x5e133c24u,
   0x105ec76fu, 0xe235446cu, 0xf165b798u, 0x030e349bu,
   0xd7c45070u, 0x25afd373u, 0x36ff2087u, 0xc494a384u,
   0x9a879fa0u, 0x68ec1ca3u, 0x7bbcef57u, 0x89d76c54u,
   0x5d1d08bfu, 0xaf768bbcu, 0xbc267848u, 0x4e4dfb4bu,
   0x20bd8edeu, 0xd2d60dddu, 0xc186fe29u, 0x33ed7d2au,
   0xe72719c1u, 0x154c9ac2u, 0x061c6936u, 0xf477ea35u,
   0xaa64d611u, 0x580f5512u, 0x4b5fa6e6u, 0xb93425e5u,
   0x6dfe410eu, 0x9f95c20du, 0x8cc531f9u, 0x7eaeb2fau,
   0x30e349b1u, 0xc288cab2u, 0xd1d83946u, 0x23b3ba45u,
   0xf779deaeu, 0x05125dadu, 0x1642ae59u, 0xe4292d5au,
   0xba3a117eu, 0x4851927du, 0x5b016189u, 0xa96ae28au,
   0x7da08661u, 0x8fcb0562u, 0x9c9bf696u, 0x6ef07595u,
   0x417b1dbcu, 0xb3109ebfu, 0xa0406d4bu, 0x522bee48u,
   0x86e18aa3u, 0x748a09a0u, 0x67dafa54u, 0x95b17957u,
   0xcba24573u, 0x39c9c670u, 0x2a993584u, 0xd8f2b687u,
   0x0c38d26cu, 0xfe53516fu, 0xed03a29bu, 0x1f682198u,
   0x5125dad3u, 0xa34e59d0u, 0xb01eaa24u, 0x42752927u,
   0x96bf4dccu, 0x64d4cecfu, 0x77843d3bu, 0x85efbe38u,
   0xdbfc821cu, 0x2997011fu, 0x3ac7f2ebu, 0xc8ac71e8u,
   0x1c661503u, 0xee0d9600u, 0xfd5d65f4u, 0x0f36e6f7u,
   0x61c69362u, 0x93ad1061u, 0x80fde395u, 0x72966096u,
   0xa65c047du, 0x5437877eu, 0x4767748au, 0xb50cf789u,
   0xeb1fcbadu, 0x197448aeu, 0x0a24bb5au, 0xf84f3859u,
   0x2c855cb2u, 0xdeeedfb1u, 0xcdbe2c45u, 0x3fd5af46u,
   0x7198540du, 0x83f3d70eu, 0x90a324fau, 0x62c8a7f9u,
   0xb602c312u, 0x44694011u, 0x5739b3e5u, 0xa55230e6u,
   0xfb410cc2u, 0x092a8fc1u, 0x1a7a7c35u, 0xe811ff36u,
   0x3cdb9bddu, 0xceb018deu, 0xdde0eb2au, 0x2f8b6829u,
   0x82f63b78u, 0x709db87bu, 0x63cd4b8fu, 0x91a6c88cu,
   0x456cac67u, 0xb7072f64u, 0xa457dc90u, 0x563c5f93u,
   0x082f63b7u, 0xfa44e0b4u, 0xe9141340u, 0x1b7f9043u,
   0xcfb5f4a8u, 0x3dde77abu, 0x2e8e845fu, 0xdce5075cu,
   0x92a8fc17u, 0x60c37f14u, 0x73938ce0u, 0x81f80fe3u,
   0x55326b08u, 0xa759e80bu, 0xb4091bffu, 0x466298fcu,
   0x1871a4d8u, 0xea1a27dbu, 0xf94ad42fu, 0x0b21572cu,
   0xdfeb33c7u, 0x2d80b0c4u, 0x3ed04330u, 0xccbbc033u,
   0xa24bb5a6u, 0x502036a5u, 0x4370c551u, 0xb11b4652u,
   0x65d122b9u, 0x97baa1bau, 0x84ea524eu, 0x7681d14du,
   0x2892ed69u, 0xdaf96e6au, 0xc9a99d9eu, 0x3bc21e9du,
   0xef087a76u, 0x1d63f975u, 0x0e330a81u, 0xfc588982u,
   0xb21572c9u, 0x407ef1cau, 0x532e023eu, 0xa145813du,
   0x758fe5d6u, 0x87e466d5u, 0x94b49521u, 0x66df1622u,
   0x38cc2a06u, 0xcaa7a905u, 0xd9f75af1u, 0x2b9cd9f2u,
   0xff56bd19u, 0x0d3d3e1au, 0x1e6dcdeeu, 0xec064eedu,
   0xc38d26c4u, 0x31e6a5c7u, 0x22b65633u, 0xd0ddd530u,
   0x0417b1dbu, 0xf67c32d8u, 0xe52cc12cu, 0x1747422fu,
   0x49547e0bu, 0xbb3ffd08u, 0xa86f0efcu, 0x5a048dffu,
   0x8ecee914u, 0x7ca56a17u, 0x6ff599e3u, 0x9d9e1ae0u,
   0xd3d3e1abu, 0x21b862a8u, 0x32e8915cu, 0xc083125fu,
   0x144976b4u, 0xe622f5b7u, 0xf5720643u, 0x07198540u,
   0x590ab964u, 0xab613a67u, 0xb831c993u, 0x4a5a4a90u,
   0x9e902e7bu, 0x6cfbad78u, 0x7fab5e8cu, 0x8dc0dd8fu,
   0xe330a81au, 0x115b2b19u, 0x020bd8edu, 0xf0605beeu,
   0x24aa3f05u, 0xd6c1bc06u, 0xc5914ff2u, 0x37faccf1u,
   0x69e9f0d5u, 0x9b8273d6u, 0x88d28022u, 0x7ab90321u,
   0xae7367cau, 0x5c18e4c9u, 0x4f48173du, 0xbd23943eu,
   0xf36e6f75u, 0x0105ec76u, 0x12551f82u, 0xe03e9c81u,
   0x34f4f86au, 0xc69f7b69u, 0xd5cf889du, 0x27a40b9eu,
   0x79b737bau, 0x8bdcb4b9u, 0x988c474du, 0x6ae7c44eu,
   0xbe2da0a5u, 0x4c4623a6u, 0x5f16d052u, 0xad7d5351u,
};


XPRESS9_INTERNAL
UInt32
Xpress9Crc32 (
    const UInt8    *pData,
    uxint           uSize,
    UInt32          uCrc
)
{
    uxint i;

    uCrc ^= 0xffffffffu;
    for (i = 0; i < uSize; ++i) 
    {
        uCrc = (uCrc>>8) ^ Xpress9Crc32Table[(uCrc ^ pData[i]) & 255];
    }
    uCrc ^= 0xffffffffu;

    return (uCrc);
}
