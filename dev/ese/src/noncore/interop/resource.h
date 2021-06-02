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

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
// Managed version of JET_RESID that appears in Jet.h. The Prefix JET_resid removed since its redundant in the Enum name.
MSINTERNAL enum class MJET_RESID
{
    Null = JET_residNull,                   //  invalid (null) resource id
    FCB = JET_residFCB,                 //  tables
    FUCB = JET_residFUCB,                   //  cursors
    TDB = JET_residTDB,                 //  table descriptors
    IDB = JET_residIDB,                 //  index descriptors
    PIB = JET_residPIB,                 //  sessions
    SCB = JET_residSCB,                 //  sorted tables
    VERBUCKET = JET_residVERBUCKET,     //  buckets used by Vesrion store
    PAGE = JET_residPAGE,                   //  general purposes page usage (instanceless)
    SPLIT = JET_residSPLIT,             //  split struct used by BTree page spilt (instanceless)
    SPLITPATH = JET_residSPLITPATH,     //  another split struct used by BTree page split (instanceless)
    MERGE = JET_residMERGE,             //  merge sruct used by BTree pages merge (instanceless)
    MERGEPATH = JET_residMERGEPATH,     //  another merge struct used by BTree page merge (instanceless)
    INST = JET_residINST,                   //  instances (instanceless)
    LOG = JET_residLOG,                 //  log instances (instanceless)
    KEY = JET_residKEY,                 //  key buffers (instanceless)
    BOOKMARK = JET_residBOOKMARK,       //  bookmark buffers (instanceless)
    LRUKHIST = JET_residLRUKHIST,           //  LRUK history records (instanceless)
    Max = JET_residMax,
    All = JET_residAll                      //  special value to be used when a setting is to be applied to all known resources
};

// Managed version of JET_RESOPER that appears in Jet.h. The Prefix JET_resoper removed since its redundant in the Enum name.
// NOTE - Only the Min, Max, and Current operations are exposed for now, per SOMEONE's feedback.
MSINTERNAL enum class MJET_RESOPER
{
    MinUse = JET_resoperMinUse,     //  Min number of preallocated objects, Set before jetinit
    MaxUse = JET_resoperMaxUse,     //  Max number of allocated object, After jetinit is set per instance
                                    //  Befre jetinit is set for the whole process
                                    //      and is rounded up to the full chunk
                                    //      the count is object based
    CurrentUse = JET_resoperCurrentUse, //  Get only, with nonnull instance param the count is object based
                                    //  with null instance param the count is chunks based
};
#endif
#endif


}
}
}

