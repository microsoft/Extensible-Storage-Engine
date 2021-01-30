// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


namespace Microsoft
{
#if (ESENT)
namespace Windows
#else
namespace Exchange
#endif
{
namespace Isam
{

MSINTERNAL enum class MJET_COLTYP
{

    Nil = 0,
    Bit = 1,
    UnsignedByte = 2,
    Short = 3,
    Long = 4,
    Currency = 5,
    IEEESingle = 6,
    IEEEDouble = 7,
    DateTime = 8,
    Binary = 9,
    Text = 10,
    LongBinary = 11,
    LongText = 12,
    UnsignedLong = 14,
    LongLong = 15,
    GUID = 16,
    UnsignedShort = 17,
    UnsignedLongLong = 18,
    Max = 19,

};

}
}
}
