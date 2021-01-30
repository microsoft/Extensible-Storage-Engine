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


MSINTERNAL enum class MJET_EXCEPTION
{
    MsgBox = 0x0001,
    None = 0x0002,
    FailFast = 0x0004,
};

}
}
}
