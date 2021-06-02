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

// JET_IOPriority

MSINTERNAL enum class MJET_IOPRIORITY
{
    Normal = 0,
    Low = 0x1,
    LowForCheckpoint = 0x2,
    LowForScavenge = 0x4,
};

}
}
}
