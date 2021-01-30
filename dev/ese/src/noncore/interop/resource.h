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
MSINTERNAL enum class MJET_RESID
{
    Null = JET_residNull,
    FCB = JET_residFCB,
    FUCB = JET_residFUCB,
    TDB = JET_residTDB,
    IDB = JET_residIDB,
    PIB = JET_residPIB,
    SCB = JET_residSCB,
    VERBUCKET = JET_residVERBUCKET,
    PAGE = JET_residPAGE,
    SPLIT = JET_residSPLIT,
    SPLITPATH = JET_residSPLITPATH,
    MERGE = JET_residMERGE,
    MERGEPATH = JET_residMERGEPATH,
    INST = JET_residINST,
    LOG = JET_residLOG,
    KEY = JET_residKEY,
    BOOKMARK = JET_residBOOKMARK,
    LRUKHIST = JET_residLRUKHIST,
    Max = JET_residMax,
    All = JET_residAll
};

MSINTERNAL enum class MJET_RESOPER
{
    MinUse = JET_resoperMinUse,
    MaxUse = JET_resoperMaxUse,
    CurrentUse = JET_resoperCurrentUse,
};
#endif
#endif


}
}
}

