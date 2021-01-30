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


MSINTERNAL enum class MJET_ASSERT
{
    Exit = 0x0000,
    Break = 0x0001,
    MsgBox = 0x0002,
    Stop = 0x0004,
    SkippableMsgBox = 0x0008,
    SkipAll = 0x0010,
    Crash = 0x0020,
    FailFast = 0x0040,
};

}
}
}
