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

// JET_objtyp* appears as #defines in jet.h, we rename them slightly (dropping JET_objtyp) to
// avoid a preprocessor nightmare

MSINTERNAL enum class MJET_OBJTYP
{

    Nil = 0,
    Table = 1,
};

}
}
}

