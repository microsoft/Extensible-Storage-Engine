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


C_ASSERT( 0x80000000 == INT_MIN );

MSINTERNAL enum class MJET_MOVE
{
    First = INT_MIN,
    Previous = (-1),
    Next = (+1),
    Last = (0x7fffffff),
};

}
}
}
