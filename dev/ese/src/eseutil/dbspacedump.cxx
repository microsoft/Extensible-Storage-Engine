// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  Space Dump Utility
//
//

//  eseutil related headers.
//
#include "_edbutil.hxx"
#include "_dbspacedump.hxx"

// Get the definition of "enum class SpacePool" from _space.hxx.
#define SPACE_ONLY_DIAGNOSTIC_CONSTANTS 1
#include "_space.hxx"

//  Stat library.
//
#include "stat.hxx"

//  Prototypes.
//
void EseutilDbspacedumpUnitTest();

// SpacePool helpers.
// This routine is at the boundary between the code in space.cxx (which uses enum class SpacePool)
// and the published header definition of BTREE_SPACE_EXTENT_INFO (which uses unsigned long iPool).
// Create some #defined constants in order to avoid all the ULONG casts that would be necessary
// otherwise.

#define ulsppAvailExtLegacyGeneralPool ((ULONG)spp::AvailExtLegacyGeneralPool)
#define ulsppPrimaryExt                ((ULONG)spp::PrimaryExt)
#define ulsppContinuousPool            ((ULONG)spp::ContinuousPool)
#define ulsppShelvedPool               ((ULONG)spp::ShelvedPool)
#define ulsppOwnedTreeAvail            ((ULONG)spp::OwnedTreeAvail) 
#define ulsppAvailTreeAvail            ((ULONG)spp::AvailTreeAvail)

//  String helpers.
//

//  cheap form of fill buffer.
static WCHAR g_rgSpaces []  = L"                                                                                                                                                         ";
static WCHAR g_rgStars []   = L"*********************************************************************************************************************************************************";
static WCHAR g_rgEquals []  = L"=========================================================================================================================================================";
static WCHAR g_rgDashes []  = L"---------------------------------------------------------------------------------------------------------------------------------------------------------";
static WCHAR g_rgCommas []  = L",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,";

INLINE const WCHAR * WszFillBuffer_(
    __in_ecount_z( cchPatternBuffer ) const WCHAR * wszPatternBuffer,
    __in SIZE_T         cchPatternBuffer,
    __in SIZE_T         cchDesiredLength
    )
{
    assert( cchPatternBuffer >= 1 );
    const WCHAR * wszPatternBufferEnd = wszPatternBuffer + cchPatternBuffer - 1;
    assert( L'\0' == *wszPatternBufferEnd );
    if ( ( wszPatternBufferEnd - cchDesiredLength ) < wszPatternBuffer )
    {
        assertSz( fFalse, "Pattern buffer request is too big" );
        return wszPatternBuffer;        // filler to avoid AV / bad buffer math.
    }
    return wszPatternBufferEnd - cchDesiredLength;
}

INLINE const WCHAR * WszFillBuffer(
    __in const WCHAR    wchPattern,
    __in SIZE_T         cchDesiredLength
    )
{
    switch ( wchPattern )
    {
        case L' ':
            return WszFillBuffer_( g_rgSpaces, _countof(g_rgSpaces), cchDesiredLength );
            break;
        case L'*':
            return WszFillBuffer_( g_rgStars, _countof(g_rgStars), cchDesiredLength );
            break;
        case L'=':
            return WszFillBuffer_( g_rgEquals, _countof(g_rgEquals), cchDesiredLength );
            break;
        case L'-':
            return WszFillBuffer_( g_rgDashes, _countof(g_rgDashes), cchDesiredLength );
            break;
        case L',':
            return WszFillBuffer_( g_rgCommas, _countof(g_rgCommas), cchDesiredLength );
            break;
        default:
            break;  // no such buffer ...
    }
    assertSz( fFalse, "Unknown pattern" );
    return L" ";    // filler to avoid AV.
}

INLINE ULONG CchCountOfChar(
    __in_z const WCHAR * const  wszString,
    __in const WCHAR        wchChar
    )
{
    ULONG   cchChar = 0;
    const WCHAR * wszField = wszString;
    while ( wszField && wszField[0] != L'\0' )
    {
        wszField = wcschr( (WCHAR*)wszField, wchChar );
        if ( wszField )
        {
            wszField++;
            cchChar++;
        }
    }
    return cchChar;
}

const WCHAR * const WszTableOfStats( _In_ const BTREE_STATS * const pBTreeStats )
{
    switch( pBTreeStats->pBasicCatalog->eType )
    {
        case eBTreeTypeInternalDbRootSpace:
            return L"DbRoot";
        case eBTreeTypeUserClusteredIndex:
            return pBTreeStats->pBasicCatalog->rgName;
        case eBTreeTypeInternalLongValue:
        case eBTreeTypeUserSecondaryIndex:
            return pBTreeStats->pParent->pBasicCatalog->rgName;
        case eBTreeTypeInternalSpaceOE:
        case eBTreeTypeInternalSpaceAE:
            if ( pBTreeStats->pParent->pBasicCatalog->eType == eBTreeTypeInternalDbRootSpace )
            {
                return L"DbRoot";   //  Root OE and AE Trees (pgno 2, and 3)
            }
            else if ( pBTreeStats->pParent->pParent &&
                    pBTreeStats->pParent->pParent->pBasicCatalog->eType != eBTreeTypeInternalDbRootSpace )
            {
                //  This is a space tree of a secondary index or LV tree, so it is 2 levels removed from it's name.
                return pBTreeStats->pParent->pParent->pBasicCatalog->rgName;
            }
            else
            {
                return pBTreeStats->pParent->pBasicCatalog->rgName;
            }
        default:
            AssertSz( fFalse, "Unexpected type" );
    }
    return L"UNKNOWN";
}

ULONG PgnoFirstFromExt( _In_ const BTREE_SPACE_EXTENT_INFO * const pext )
{
    Assert( pext->cpgExtent > 0 );
    Assert( pext->cpgExtent < (ULONG)lMax ); // not irrationally high or unsigned overflowed.
    return pext->pgnoLast - pext->cpgExtent + 1;
}

BOOL FDBSPUTLContains( const BTREE_SPACE_EXTENT_INFO * const pextContaining, const ULONG pgnoPossibleSubPage )
{
    Assert( pextContaining->cpgExtent > 0 );
    if ( pgnoPossibleSubPage >= PgnoFirstFromExt( pextContaining ) &&
        pgnoPossibleSubPage <= pextContaining->pgnoLast )
    {
        return fTrue;
    }
    return fFalse;
}

BOOL FDBSPUTLContains( const BTREE_SPACE_EXTENT_INFO * const pextContaining, const BTREE_SPACE_EXTENT_INFO * const pextPossibleSubExtent )
{
    if ( FDBSPUTLContains( pextContaining, PgnoFirstFromExt( pextPossibleSubExtent ) ) &&
         FDBSPUTLContains( pextContaining, pextPossibleSubExtent->pgnoLast ) )
    {
        return fTrue;
    }

    // If one end of the extent is not in the containing extent, then it should be completely out, as ESE (at least today) does
    // not deal in overlapping available extents.
    Assert( !FDBSPUTLContains( pextContaining, PgnoFirstFromExt( pextPossibleSubExtent ) ) );
    Assert( !FDBSPUTLContains( pextContaining, pextPossibleSubExtent->pgnoLast ) );

    return fFalse;
}


//
//  Defining Space Dump Fields
//

//  To add a field:
//      1. Put an enum here.
//      2. Add an entry to rgSpaceFields representing the field.
//      3. Add a case to ErrPrintField().
//  Note this enum and the rgSpaceFields must be in lock step, but it will assert on first run
//  if any are out of order, so no big deal.
typedef enum
{
    eSPFieldNone = 0,
    eSPFieldNameFull,
    eSPFieldOwningTableName,
    eSPFieldType,
    eSPFieldObjid,
    eSPFieldPgnoFDP,
    eSPFieldPriExt,
    eSPFieldPriExtType,
    eSPFieldPriExtCpg,
    eSPFieldPgnoOE,
    eSPFieldPgnoAE,
    eSPFieldOwnedPgnoMin,
    eSPFieldOwnedPgnoMax,
    eSPFieldOwnedExts,
    eSPFieldOwnedCPG,
    eSPFieldOwnedMB,
    eSPFieldOwnedPctOfDb,
    eSPFieldOwnedPctOfTable,
    eSPFieldDataCPG,
    eSPFieldDataMB,
    eSPFieldDataPctOfDb,
    eSPFieldAvailExts,
    eSPFieldAvailCPG,
    eSPFieldAvailMB,
    eSPFieldAvailPctOfTable,
    eSPFieldSpaceTreeReservedCPG,
    eSPFieldAutoInc,
    eSPFieldReservedCPG,
    eSPFieldReservedMB,
    eSPFieldReservedPctOfTable,
    eSPFieldOwnedExtents,
    eSPFieldGeneralAvailExtents,
    eSPFieldContigAvailExtents,
    eSPFieldEmpty,
    eSPFieldInternalCPG,
    eSPFieldInternalMB,
    eSPFieldDepth,
    eSPFieldIORuns,
    eSPFieldIORunsHisto,
    eSPFieldForwardScans,
    eSPFieldIntFreeBytesMAM,
    eSPFieldIntNodeCountsMAM,
    eSPFieldIntKeySizesMAM,
    eSPFieldIntKeyCompMAM,
    eSPFieldFreeBytesMAM,
    eSPFieldNodeCountsMAM,
    eSPFieldNodePctOfTable,
    eSPFieldKeySizesMAM,
    eSPFieldDataSizesMAM,
    eSPFieldKeyCompMAM,
    eSPFieldUnreclaimedMAM,
    eSPFieldVersionedNodes,
    eSPFieldLVRefs,
    eSPFieldCorruptLVs,
    eSPFieldSeparatedRootChunks,
    eSPFieldPartiallyDeletedLVs,
    eSPFieldLidMax,
    eSPFieldLVChunkMax,
    eSPFieldLVSize,
    eSPFieldLVSizeHisto,
    eSPFieldLVComp,
    eSPFieldLVCompHisto,
    eSPFieldLVRatioHisto,
    eSPFieldLVSeeks,
    eSPFieldLVSeeksHisto,
    eSPFieldLVBytes,
    eSPFieldLVBytesHisto,
    eSPFieldLVExtraSeeks,
    eSPFieldLVExtraSeeksHisto,
    eSPFieldLVExtraBytes,
    eSPFieldLVExtraBytesHisto,
    eSPFieldSHInitialDensity,
    eSPFieldSHInitialSize,
    eSPFieldSHGrbit,
    eSPFieldSHMaintDensity,
    eSPFieldSHGrowthPct,
    eSPFieldSHMinExtent,
    eSPFieldSHMaxExtent,

    //  At end b/c in the /f#all - 
    eSPFieldName,

    eSPFieldMax

} E_SP_FIELD;


//
//  Space Dump CTX
//

typedef struct
{

    //
    //      Initial call information
    //
    JET_GRBIT       grbit;

    //
    //      Printing / output information
    //
    ULONG           cFields;
    E_SP_FIELD *    rgeFields;
    WCHAR           rgwchSep[4];
    BOOL            fPrintSmallTrees;
    BOOL            fPrintSpaceTrees;
    BOOL            fPrintSpaceNodes;
    BOOL            fPrintSpaceLeaks;
    BOOL            fSquelchedSmallTrees;
    BOOL            fSelectOneTable;
    ULONG           cbPageSize;

    //
    //      Accumulation Data.
    //

    //  Count of various B-Tree types [always updated] ...
    ULONG           cBTrees[eBTreeTypeMax];

    //  Updated only if space trees info was asked for...
    ULONG           cpgDbOwned;
    ULONG           cpgTotalAvailExt;
    ULONG           cpgTotalShelved;
    ULONG           pgnoDbEndUnusedBegin;   // The first pgno of the available region at the end of the database (if any).
    ULONG           pgnoDbEndUnusedEnd;     // The last pgno of the available region at the end of the database (if any).
    ULONG           pgnoUsedMin;            // used = in a tables, LVs, or indices owned extents (not space trees, though)
    ULONG           pgnoUsedMax;
    ULONG           pgnoLast;
    //  Count of Page stats ...
    ULONG           cpgTotalOEOverhead;
    ULONG           cpgTotalAEOverhead;
    //  Updates only if parent of leaf was asked for
    ULONG           cpgTotalInternal;
    ULONG           cpgTotalData;

    //  Data for current table
    ULONG           cpgCurrentTableOwned;
    

    //  Leaked page detection
    //      These are ints for a reason, its unfortunate for these values to come up
    //      positive (i.e. user is leaked space), but its critical if these values
    //      come up negative!!!  This would mean the space is probably used AND marked
    //      as available.
    //  Leaked page detection - working variables.
    //  Leaked page detection - working variables - DbRoot.
    INT             cpgDbRootUnAccountedFor;        // Includes cpgDbRootUnAccountedForEof
    INT             cpgDbRootUnAccountedForEof;
    ULONG           objidCurrTable;
    INT             cpgCurrTableUnAccountedFor;     // Includes cpgCurrTableUnAccountedForEof
    INT             cpgCurrTableUnAccountedForEof;
    ULONG           objidCurrIdxLv;
    JET_BTREETYPE   eBTTypeCurr;
    INT             cpgCurrIdxLvUnAccountedFor;     // Includes cpgCurrIdxLvUnAccountedFor
    INT             cpgCurrIdxLvUnAccountedForEof;
    //  Leaked page detection - final accumulator.
    INT             cpgTotalLeaked;
    INT             cpgTotalLeakedEof;
    INT             cpgTotalOverbooked;

    //
    //      Global (DB-root level) Shelved Extent List.
    //
    unsigned long                   cShelvedExtents;
    _Field_size_opt_(cShelvedExtents) BTREE_SPACE_EXTENT_INFO *       prgShelvedExtents;
    BOOL            fOwnedNonShelved;
} ESEUTIL_SPACE_DUMP_CTX;


//
//  Field printing support
//

double g_dblSmallTableThreshold = .005;

typedef struct
{
    E_SP_FIELD  eField;
    ULONG       cchFieldSize;
    WCHAR *     wszField;
    WCHAR *     wszSubFields;
    JET_GRBIT   grbitRequired;
} ESEUTIL_SPACE_FIELDS;

//  The sub-header for fields w/ Min,Ave,Max data (as well as count and total for completeness).
WCHAR * szMAMH = L"   Count,  Min,  Ave,  Max,       Total";
#define cchMAMH     39

WCHAR * szCAH = L"   Count,  Ave";
#define cchCAH      14

WCHAR szRunsH []= L"    1-Page,   2-Pages,   3-Pages,   4-Pages,   8-Pages,  16-Pages,  32-Pages,  64-Pages,  96-Pages, 128-Pages,  Over-128";
#define cchRunsH 120
SAMPLE rgIORunHistoDivisions[] =
{
    1, 2, 3, 4, 8, 16, 32, 64, 96, 128, (SAMPLE)-1 /* catch the rest */
};

WCHAR szLVB[] = L"       4KB,       8KB,      16KB,      32KB,      64KB,     128KB,     256KB,     512KB,       1MB,       2MB,  Over-2MB";
#define cchLVB 120
SAMPLE rgLVBytesDivisions[] =
{
    4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, (SAMPLE)-1 /* catch the rest */
};

WCHAR szLVR[] = L"       30%,       40%,       50%,       60%,       70%,       80%,       90%,       93%,       95%,       97%,      100%";
#define cchLVR 120
SAMPLE rgLVRatioDivisions[] =
{
    30, 40, 50, 60, 70, 80, 90, 93, 95, 97, 100
};

WCHAR szLVS[] = L"         1,         2,         3,         4,         5,         6,         7,         8,        16,        32,   Over-32";
#define cchLVS 120
SAMPLE rgLVSeeksDivisions[] =
{
    1, 2, 3, 4, 5, 6, 7, 8, 16, 32, (SAMPLE)-1 /* catch the rest */
};

WCHAR szLVES[] = L"         0,         1,         2,         3,         4,         5,         6,         7,         8,        16,   Over-16";
#define cchLVES 120
SAMPLE rgLVExtraSeeksDivisions[] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 16, (SAMPLE)-1 /* catch the rest */
};

WCHAR szLVEB[] = L"       0KB,       4KB,       8KB,      16KB,      32KB,      64KB,     128KB,     256KB,     512KB,       1MB,  Over-1MB";
#define cchLVEB 120
SAMPLE rgLVExtraBytesDivisions[] =
{
    0, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, (SAMPLE)-1 /* catch the rest */
};

ESEUTIL_SPACE_FIELDS rgSpaceFields [] =
{

    //  enum                            cchFieldSize,   wszFieldName,       wszSubField,    grbitRequired
    { eSPFieldNone,                     3,          L"N/A",                     NULL,   0x0                         },

    // Note: 2-levels of indenting is the worst case for a long name, b/c under an Idx/LV the OE/AE trees would have short names.
    { eSPFieldNameFull,                 64 + 4,     L"FullName",                NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },
    { eSPFieldOwningTableName,          64 + 4,     L"OwningTableName",         NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },
    { eSPFieldType,                     4,          L"Type",                    NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },  // legacy
    { eSPFieldObjid,                    10,         L"ObjidFDP",                NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },  // legacy
    { eSPFieldPgnoFDP,                  10,         L"PgnoFDP",                 NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },  // legacy

    { eSPFieldPriExt,                   7,          L"PriExt",                  NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },  // legacy
    { eSPFieldPriExtType,               10,         L"PriExtType",              NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldPriExtCpg,                9,          L"PriExtCpg",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldPgnoOE,                   10,         L"PgnoOE",                  NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldPgnoAE,                   10,         L"PgnoAE",                  NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldOwnedPgnoMin,             10,         L"OwnPgnoMin",              NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldOwnedPgnoMax,             10,         L"OwnPgnoMax",              NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldOwnedExts,                10,         L"OwnedExts",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldOwnedCPG,                 10,         L"Owned",                   NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },  // legacy
    { eSPFieldOwnedMB,                  8 + 4,      L"Owned(MB)",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldOwnedPctOfDb,             9,          L"O%OfDb",                  NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldOwnedPctOfTable,          9,          L"O%OfTable",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldDataCPG,                  10,         L"Data",                    NULL,   JET_bitDBUtilSpaceInfoParentOfLeaf  },
    { eSPFieldDataMB,                   8 + 4,      L"Data(MB)",                NULL,   JET_bitDBUtilSpaceInfoParentOfLeaf  },
    { eSPFieldDataPctOfDb,              9,          L"D%OfDb",                  NULL,   JET_bitDBUtilSpaceInfoParentOfLeaf  },

    { eSPFieldAvailExts,                10,         L"AvailExts",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldAvailCPG,                 10,         L"Available",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },  // legacy
    { eSPFieldAvailMB,                  8 + 4,      L"Avail(MB)",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldAvailPctOfTable,          9,          L"Avail%Tbl",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldSpaceTreeReservedCPG,     10,         L"SpcReserve",              NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldAutoInc,                  10,         L"AutoInc",                 NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldReservedCPG,              10,         L"Reserved",                NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },  // legacy
    { eSPFieldReservedMB,               8 + 4,      L"Reser(MB)",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldReservedPctOfTable,       9,          L"Reser%Tbl",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },

    { eSPFieldOwnedExtents,             cchMAMH,    L"OwnExt",                  szMAMH, JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldGeneralAvailExtents,      cchMAMH,    L"GenAvailExt",             szMAMH, JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldContigAvailExtents,       cchMAMH,    L"ContigAvailExt",          szMAMH, JET_bitDBUtilSpaceInfoSpaceTrees    },

    { eSPFieldEmpty,                    5,          L"Empty",                   NULL,   JET_bitDBUtilSpaceInfoParentOfLeaf  },
    { eSPFieldInternalCPG,              10,         L"Internal",                NULL,   JET_bitDBUtilSpaceInfoParentOfLeaf  },
    { eSPFieldInternalMB,               8 + 4,      L"Internal(MB)",            NULL,   JET_bitDBUtilSpaceInfoParentOfLeaf  },
    { eSPFieldDepth,                    5,          L"Depth",                   NULL,   JET_bitDBUtilSpaceInfoParentOfLeaf  },
    { eSPFieldIORuns,                   6,          L"IORuns",                  NULL,   JET_bitDBUtilSpaceInfoParentOfLeaf  },
    { eSPFieldIORunsHisto,              cchRunsH,   L"IORuns(histo)",           szRunsH,JET_bitDBUtilSpaceInfoParentOfLeaf  },
    { eSPFieldForwardScans,             8,          L"FwdScans",                NULL,   JET_bitDBUtilSpaceInfoParentOfLeaf  },

    { eSPFieldIntFreeBytesMAM,          cchMAMH,    L"Int:FreeBytes",           szMAMH, JET_bitDBUtilSpaceInfoParentOfLeaf  },
    { eSPFieldIntNodeCountsMAM,         cchMAMH,    L"Int:Nodes",               szMAMH, JET_bitDBUtilSpaceInfoParentOfLeaf  },
    { eSPFieldIntKeySizesMAM,           cchMAMH,    L"Int:KeySizes",            szMAMH, JET_bitDBUtilSpaceInfoParentOfLeaf  },
    // Avoided as its fundamentally not interesting because DataSize is almost always 4 (a pgno).
    //eSPFieldIntDataSizesMAM,  cchMAMH,    L"Int:DataSizes",   szMAMH, JET_bitDBUtilSpaceInfoParentOfLeaf  },
    { eSPFieldIntKeyCompMAM,            cchMAMH,    L"Int:KeyComp",             szMAMH, JET_bitDBUtilSpaceInfoParentOfLeaf  },
    // Don't think this case is valid, couldn't find any cases.
    //eSPFieldIntUnreclaimedMAM,cchMAMH,    L"Int:Unreclaim",   szMAMH, JET_bitDBUtilSpaceInfoParentOfLeaf  },

    { eSPFieldFreeBytesMAM,             cchMAMH,    L"Data:FreeBytes",          szMAMH, JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldNodeCountsMAM,            cchMAMH,    L"Data:Nodes",              szMAMH, JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldNodePctOfTable,           9,          L"Node%Tbl",                NULL,   JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldKeySizesMAM,              cchMAMH,    L"Data:KeySizes",           szMAMH, JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldDataSizesMAM,             cchMAMH,    L"Data:DataSizes",          szMAMH, JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldKeyCompMAM,               cchMAMH,    L"Data:KeyComp",            szMAMH, JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldUnreclaimedMAM,           cchMAMH,    L"Data:Unreclaim",          szMAMH, JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldVersionedNodes,           10,         L"VersndNode",              NULL,   JET_bitDBUtilSpaceInfoFullWalk      },

    { eSPFieldLVRefs,                   12,         L"cLVRefs",                 NULL,   JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldCorruptLVs,               12,         L"cCorrLVs",                NULL,   JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldSeparatedRootChunks,      12,         L"cSepRtChk",               NULL,   JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldPartiallyDeletedLVs,      12,         L"cPartDelLVs",             NULL,   JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLidMax,                   12,         L"lidMax",                  NULL,   JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLVChunkMax,               12,         L"LV:ChunkSize",            NULL,   JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLVSize,                   cchCAH,     L"LV:Size",                 szCAH,  JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLVSizeHisto,              cchLVB,     L"LV:Size(histo)",          szLVB,  JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLVComp,                   12,         L"LV:Comp",                 NULL,   JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLVCompHisto,              cchLVB,     L"LV:Comp(histo)",          szLVB,  JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLVRatioHisto,             cchLVR,     L"LV:Ratio(histo)",         szLVR,  JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLVSeeks,                  12,         L"LV:Seeks",                NULL,   JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLVSeeksHisto,             cchLVS,     L"LV:Seeks(histo)",         szLVS,  JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLVBytes,                  12,         L"LV:Bytes",                NULL,   JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLVBytesHisto,             cchLVB,     L"LV:Bytes(histo)",         szLVB,  JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLVExtraSeeks,             13,         L"LV:ExtraSeeks",           NULL,   JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLVExtraSeeksHisto,        cchLVES,    L"LV:ExtraSeeks(histo)",    szLVES, JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLVExtraBytes,             13,         L"LV:ExtraBytes",           NULL,   JET_bitDBUtilSpaceInfoFullWalk      },
    { eSPFieldLVExtraBytesHisto,        cchLVEB,    L"LV:ExtraBytes(histo)",    szLVEB, JET_bitDBUtilSpaceInfoFullWalk      },

    { eSPFieldSHInitialDensity,         11,         L"SH:IDensity",             NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },
    { eSPFieldSHInitialSize,            12,         L"SH:ISize(KB)",            NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },
    { eSPFieldSHGrbit,                  9,          L"SH:grbit",                NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },
    { eSPFieldSHMaintDensity,           11,         L"SH:MDensity",             NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },
    { eSPFieldSHGrowthPct,              9,          L"SH:Growth",               NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },
    { eSPFieldSHMinExtent,              13,         L"SH:MinExt(KB)",           NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },
    { eSPFieldSHMaxExtent,              13,         L"SH:MaxExt(KB)",           NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },

    { eSPFieldName,                     23,         L"Name",                    NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },  // legacy

};


BOOL FFindField( const WCHAR * const wszField, ULONG * peField )
{
    assert( peField );

    for( ULONG eField = 0; eField < _countof(rgSpaceFields); eField++ )
    {
        if ( 0 == _wcsicmp( rgSpaceFields[eField].wszField, wszField ) )
        {
            *peField = eField;
            return fTrue;
        }
    }
    return fFalse;
}

BOOL FLastField( ULONG eField )
{
    return ( eField == ( _countof(rgSpaceFields) - 1 ) );
}

    
//  Print min,ave,max ... as well as count and total for simplicity.
void PrintMAM( _In_ const JET_HISTO *   phisto )
{
    CStats * pStats = CStatsFromPv( phisto );

    if ( pStats && pStats->C() )
    {
        wprintf(L"%8I64u,%5I64u,%5I64u,%5I64u,%12I64u",
                pStats->C(),
                pStats->Min(),
                pStats->Ave(),
                pStats->Max(),
                pStats->Total() );
    }
    else
    {
        wprintf(L"       0,     ,     ,     ,            ");
    }
}

VOID PrintCA(
    _In_ const ULONG        eField,
    _In_ const JET_HISTO *  phisto
    )
{
    CStats * pStats = CStatsFromPv( phisto );

    if ( pStats && pStats->C() )
    {
        wprintf( L"%8I64u,%5I64u", pStats->C(), pStats->Ave() );
    }
    else
    {
        wprintf( L"       0,     " );
    }
}

VOID PrintA(
    _In_ const ULONG        eField,
    __in const JET_HISTO *  phisto
    )
{
    CStats * pStats = CStatsFromPv( phisto );
    if ( pStats )
    {
        wprintf(L"%*.*f", rgSpaceFields[eField].cchFieldSize, 2, pStats->DblAve() );
    }
    else
    {
        wprintf(L"%*s", rgSpaceFields[eField].cchFieldSize, L"N/A" );
    }
}

VOID PrintHisto(
    _In_ const ULONG            eField,
    _In_reads_(cDivisions) const STAT::SAMPLE * rgDivisions,
    __in const size_t           cDivisions,
    __in const JET_HISTO *      phisto
    )
{
    CStats * pStats = CStatsFromPv( phisto );
    pStats->ErrReset();
    for ( ULONG iDiv = 0; iDiv < cDivisions; iDiv++ )
    {
        CHITS hits = 0;
        CStats::ERR csErr;
        WCHAR * szSep = ( iDiv == ( cDivisions - 1 ) ) ? L"" : L",";
        if ( pStats &&
                ( CStats::ERR::errSuccess == ( csErr = pStats->ErrGetSampleHits( rgDivisions[iDiv], &hits ) ) )
            )
        {
            wprintf(L"%*I64u%ws", 10, hits, szSep);
        }
        else
        {
            wprintf(L"%*d%ws", 10, 0, szSep);
        }
    }
    pStats->ErrReset();
}
    
#define CbFromCpg( cpg )            (((__int64)cpg) * ((__int64)pespCtx->cbPageSize))
#define CmbFromCpg( cpg )           ((DWORD)( CbFromCpg(cpg) / ((__int64)1024) / ((__int64)1024) ))
#define CkbDecimalFromCpg( cpg )    ((DWORD)(       \
                                            ( CbFromCpg(cpg) / ((__int64)1024) ) % ((__int64)1024) * ((__int64)1000) / ((__int64)1024) + \
                                            ( ((( CbFromCpg(cpg) / ((__int64)1024) ) % ((__int64)1024) * ((__int64)10000) / ((__int64)1024)) >= 5 ) ? 1 : 0 )       \
                                            ))

void PrintFieldSep (
    __in const ESEUTIL_SPACE_DUMP_CTX * const   pespCtx,
    __in ULONG ieField
    )
{
    wprintf( L"%ws", ( ieField != (pespCtx->cFields - 1) ) ? pespCtx->rgwchSep : L"\n" );
}


void PrintNullField(
    __in const ESEUTIL_SPACE_DUMP_CTX * const   pespCtx,
    __in ULONG eField
    )
{
    ULONG cSep = CchCountOfChar( rgSpaceFields[eField].wszSubFields, L',' );
    wprintf(    L"%ws%ws",
                WszFillBuffer( L' ', rgSpaceFields[eField].cchFieldSize - cSep ),
                WszFillBuffer( pespCtx->rgwchSep[0], cSep ) );
}

JET_ERR ErrFromStatErr( CStats::ERR errStat )
{
    switch( errStat )
    {
        case CStats::ERR::errSuccess:           return JET_errSuccess;
        case CStats::ERR::errInvalidParameter:  return ErrERRCheck( JET_errInvalidParameter );
        case CStats::ERR::errOutOfMemory:       return ErrERRCheck( JET_errOutOfMemory );
        case CStats::ERR::wrnOutOfSamples:      return ErrERRCheck( errNotFound );
        case CStats::ERR::errFuncNotSupported:  return ErrERRCheck( JET_errDisabledFunctionality );
        default:
            AssertSz( fFalse, "Unknown stat error" );
    }
    return ErrERRCheck( errCodeInconsistency );
}

typedef struct _BTREE_POST_COMPUTED_SPACE_STATS
{
    CPerfectHistogramStats  histoOEs;
    CPerfectHistogramStats  histoGeneralAEs;
    CPerfectHistogramStats  histoContiguousAEs;
    ULONG                   cpgReservedForSpaceTrees;

    _BTREE_POST_COMPUTED_SPACE_STATS() :
        histoOEs(),
        histoGeneralAEs(),
        histoContiguousAEs()
    {
        cpgReservedForSpaceTrees = 0;
    }
}
BTREE_POST_COMPUTED_SPACE_STATS;

JET_ERR ErrCalculatePostComputedSpaceStats(
    _In_ const BTREE_STATS * const                      pBTreeStats,
    _Inout_ BTREE_POST_COMPUTED_SPACE_STATS * const     pComputedStats
    )
{
    JET_ERR err = JET_errSuccess;

    if ( pBTreeStats->pSpaceTrees )
    {
        for( ULONG iext = 0; iext < pBTreeStats->pSpaceTrees->cOwnedExtents; iext++ )
        {
            Call( ErrFromStatErr( pComputedStats->histoOEs.ErrAddSample( pBTreeStats->pSpaceTrees->prgOwnedExtents[iext].cpgExtent ) ) );
        }
        for( ULONG iext = 0; iext < pBTreeStats->pSpaceTrees->cAvailExtents; iext++ )
        {
            if ( pBTreeStats->pSpaceTrees->prgAvailExtents[iext].iPool == ulsppAvailExtLegacyGeneralPool ||
                    pBTreeStats->pSpaceTrees->prgAvailExtents[iext].iPool == ulsppPrimaryExt )
            {
                Call( ErrFromStatErr( pComputedStats->histoGeneralAEs.ErrAddSample( pBTreeStats->pSpaceTrees->prgAvailExtents[iext].cpgExtent ) ) );
            }
            else if ( pBTreeStats->pSpaceTrees->prgAvailExtents[iext].iPool == ulsppContinuousPool )
            {
                // Contiguous Marker "extents" come down now, and these aren't really available space, so we'll filter them.
                if ( pBTreeStats->pSpaceTrees->prgAvailExtents[iext].cpgExtent != 0 )
                {
                    Call( ErrFromStatErr( pComputedStats->histoContiguousAEs.ErrAddSample( pBTreeStats->pSpaceTrees->prgAvailExtents[iext].cpgExtent ) ) );
                }
            }
            else if ( pBTreeStats->pSpaceTrees->prgAvailExtents[iext].iPool == ulsppShelvedPool )
            {
                // Do nothing. We don't accumulate details stats on shelved pages.
            }
            else if ( pBTreeStats->pSpaceTrees->prgAvailExtents[iext].iPool == ulsppOwnedTreeAvail ||
                    pBTreeStats->pSpaceTrees->prgAvailExtents[iext].iPool == ulsppAvailTreeAvail )
            {
                pComputedStats->cpgReservedForSpaceTrees += pBTreeStats->pSpaceTrees->prgAvailExtents[iext].cpgExtent;
            }
            else
            {
                AssertSz( fFalse, "A pool type (%d) is unrecognized, update eseutil\\dbspacedump.cxx", pBTreeStats->pSpaceTrees->prgAvailExtents[iext].iPool );
            }
        }
    }

HandleError:

    return err;
}

double PctNormalized( const unsigned __int64 cNumerator, const unsigned __int64 cDenominator )
{
    double pct = ((double)cNumerator) / ((double)cDenominator) * 100;
    if ( cDenominator == 0 )
    {
        //  div by 0 is NaN ... 
        pct = 0.0;
    }
    else if ( cNumerator > 0 )
    {
        pct = max( pct, 0.1 );
    }
    else if ( cNumerator < cDenominator )
    {
        pct = min( pct, 99.9 );
    }
    return pct;
}


//  We're not returning any owned min/max for the _actual_ space trees themselves (this shows
//  as 0s in the report), but the owned min/max of the actual user / parent tree of those 
//  space tress here.  Just the space trees is where we're puling this information from.
//
//  The owned of the space trees themselves would be something like the min/max pgno actually
//  used in the space tree iteself plus the min/max of thier split buffers.  Do note the owned
//  of the space trees is actually still contained within the owned min/max of the actual user
//  / parent trees of those space trees, so all the usable pages are represented in the user /
//  parent tree line.

ULONG PgnoMinOwned( const BTREE_STATS_SPACE_TREES * const pSpaceTrees )
{
    Assert( pSpaceTrees );
    ULONG pgnoMinFound = ulMax;
    for( ULONG iext = 0; iext < pSpaceTrees->cOwnedExtents; iext++ )
    {
        BTREE_SPACE_EXTENT_INFO * pext = &pSpaceTrees->prgOwnedExtents[iext];
        pgnoMinFound = min( pgnoMinFound, PgnoFirstFromExt( pext ) );
    }
    if ( pgnoMinFound == ulMax )
    {
        pgnoMinFound = 0;
    }
    return pgnoMinFound;
}

ULONG PgnoMaxOwned( const BTREE_STATS_SPACE_TREES * const pSpaceTrees )
{
    Assert( pSpaceTrees );
    ULONG pgnoMaxFound = 0;
    for( ULONG iext = 0; iext < pSpaceTrees->cOwnedExtents; iext++ )
    {
        BTREE_SPACE_EXTENT_INFO * pext = &pSpaceTrees->prgOwnedExtents[iext];
        pgnoMaxFound = max( pgnoMaxFound, pext->pgnoLast );
    }
    return pgnoMaxFound;
}


JET_ERR ErrPrintField(
    __in const ESEUTIL_SPACE_DUMP_CTX * const           pespCtx,
    __in const BTREE_STATS * const                      pBTStats,
    _In_ const BTREE_POST_COMPUTED_SPACE_STATS * const  pComputedStats,
    _In_ const ULONG                                    eField
    )
{

    switch( eField )
    {
        case eSPFieldNameFull:
        case eSPFieldName:
        {
            assert( pBTStats->pBasicCatalog );

            const BTREE_STATS * pIndenting = pBTStats;  // I think this = pBTStats->pParent; would make it so tables weren't indented.
            ULONG cchFieldAdjust = 0;
            while( pIndenting->pParent )
            {
                pIndenting = pIndenting->pParent;
                wprintf( L"  " );
                cchFieldAdjust += 2;
            }
            if ( eField == eSPFieldNameFull )
            {
                // This is the whole point of eSPFieldNameFull, lets assert we're achieving our goal.
                assert( rgSpaceFields[eField].cchFieldSize >= ( cchFieldAdjust + wcslen(pBTStats->pBasicCatalog->rgName) ) );
            }

            wprintf(L"%-*.*ws", rgSpaceFields[eField].cchFieldSize-cchFieldAdjust,
                            rgSpaceFields[eField].cchFieldSize-cchFieldAdjust,
                            pBTStats->pBasicCatalog->rgName );
        }
            break;
        case eSPFieldOwningTableName:
        {
            assert( pBTStats->pBasicCatalog );
            // check for truncation
            assert( rgSpaceFields[eField].cchFieldSize >= wcslen(WszTableOfStats( pBTStats )) );

            wprintf(L"%-*.*ws", rgSpaceFields[eField].cchFieldSize,
                            rgSpaceFields[eField].cchFieldSize,
                            WszTableOfStats( pBTStats ) );
        }
            break;

        case eSPFieldType:
        {
            WCHAR * szType = NULL;
            assert( pBTStats->pBasicCatalog );
            switch( pBTStats->pBasicCatalog->eType )
            {
                case eBTreeTypeInternalDbRootSpace:
                    szType = L" Db ";
                    break;
                case eBTreeTypeInternalSpaceOE:
                    szType = L" OE ";
                    break;
                case eBTreeTypeInternalSpaceAE:
                    szType = L" AE ";
                    break;
                case eBTreeTypeUserClusteredIndex:
                    szType = L" Pri";
                    break;
                case eBTreeTypeInternalLongValue:
                    szType = L" LV ";
                    break;
                case eBTreeTypeUserSecondaryIndex:
                    szType = L" Idx";
                    break;
                default:
                    assertSz( fFalse, "Cool there is a new B-Tree type, please update dbspacedump.cxx." );
                    szType = L"???";
                    break;
            }
            wprintf( szType );
        }
            break;
        case eSPFieldObjid:
            assert( pBTStats->pBasicCatalog );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pBasicCatalog->objidFDP );
            break;
        case eSPFieldPgnoFDP:
            assert( pBTStats->pBasicCatalog );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pBasicCatalog->pgnoFDP );
            break;

        case eSPFieldPriExt:
            assert( pBTStats->pSpaceTrees );
            wprintf(L"%*d-%c", 5, pBTStats->pSpaceTrees->cpgPrimary,
                        pBTStats->pSpaceTrees->fMultiExtent ? L'm' : L's' );
            break;
        case eSPFieldPriExtType:
            assert( pBTStats->pSpaceTrees );
            wprintf(L"%*ws", rgSpaceFields[eField].cchFieldSize, pBTStats->pSpaceTrees->fMultiExtent ? L"multi" : L"single" );
            break;
        case eSPFieldPriExtCpg:
        {
            assert( pBTStats->pSpaceTrees );
            ULONG cpgFirstExt = 0;
            for( ULONG iext = 0; iext < pBTStats->pSpaceTrees->cOwnedExtents; iext++ )
            {
                BTREE_SPACE_EXTENT_INFO * pext = &pBTStats->pSpaceTrees->prgOwnedExtents[iext];
                if ( PgnoFirstFromExt( pext ) == pBTStats->pBasicCatalog->pgnoFDP )
                {
                    cpgFirstExt = pext->cpgExtent;
                }
            }
            Assert( cpgFirstExt );  // should be impossible now
            Assert( pBTStats->pSpaceTrees->cpgPrimary == 0 || pBTStats->pSpaceTrees->cpgPrimary == cpgFirstExt );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, cpgFirstExt );
        }
            break;
        case eSPFieldPgnoOE:
            assert( pBTStats->pSpaceTrees );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pSpaceTrees->pgnoOE );
            break;
        case eSPFieldPgnoAE:
            assert( pBTStats->pSpaceTrees );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pSpaceTrees->pgnoAE );
            break;
        case eSPFieldOwnedPgnoMin:
        {
            Assert( pBTStats->pSpaceTrees );
            const ULONG pgnoMinFound = PgnoMinOwned( pBTStats->pSpaceTrees );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pgnoMinFound );
        }
            break;
        case eSPFieldOwnedPgnoMax:
        {
            Assert( pBTStats->pSpaceTrees );
            const ULONG pgnoMaxFound = PgnoMaxOwned( pBTStats->pSpaceTrees );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pgnoMaxFound );
        }
            break;

        case eSPFieldOwnedExts:
            assert( pBTStats->pSpaceTrees );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pSpaceTrees->cOwnedExtents );
            break;
        case eSPFieldOwnedCPG:
            assert( pBTStats->pSpaceTrees );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pSpaceTrees->cpgOwned );
            break;
        case eSPFieldOwnedMB:
            assert( pBTStats->pSpaceTrees );
            wprintf(L"%*d.%03d", rgSpaceFields[eField].cchFieldSize-4,
                                CmbFromCpg(pBTStats->pSpaceTrees->cpgOwned),
                                CkbDecimalFromCpg(pBTStats->pSpaceTrees->cpgOwned) );
            break;
        case eSPFieldOwnedPctOfDb:
        {
            assert( pBTStats->pSpaceTrees );
            assert( pespCtx->cpgDbOwned );
            double pct = ((double)pBTStats->pSpaceTrees->cpgOwned) / ((double)pespCtx->cpgDbOwned) * 100;
            wprintf(L"%*.*f%%", rgSpaceFields[eField].cchFieldSize-1, 2, pct );
        }
            break;
        case eSPFieldOwnedPctOfTable:
        {
            assert( pBTStats->pSpaceTrees );
            if ( pBTStats->pBasicCatalog->eType != eBTreeTypeInternalDbRootSpace && pespCtx->cpgCurrentTableOwned )
            {
                //assert( pespCtx->cpgCurrentTableOwned );
                double pct = ((double)pBTStats->pSpaceTrees->cpgOwned) / ((double)pespCtx->cpgCurrentTableOwned) * 100;
                wprintf(L"%*.*f%%", rgSpaceFields[eField].cchFieldSize-1, 2, pct );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
        }
            break;
        case eSPFieldDataCPG:
            assert( pBTStats->pParentOfLeaf );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pParentOfLeaf->cpgData );
            break;
        case eSPFieldDataMB:
            assert( pBTStats->pParentOfLeaf );
            wprintf(L"%*d.%03d", rgSpaceFields[eField].cchFieldSize-4,
                                CmbFromCpg(pBTStats->pParentOfLeaf->cpgData),
                                CkbDecimalFromCpg(pBTStats->pParentOfLeaf->cpgData) );
            break;
        case eSPFieldDataPctOfDb:
        {
            assert( pBTStats->pParentOfLeaf );
            assert( pespCtx->cpgDbOwned );
            double pct = ((double)pBTStats->pParentOfLeaf->cpgData) / ((double)pespCtx->cpgDbOwned) * 100;
            wprintf(L"%*.*f%%", rgSpaceFields[eField].cchFieldSize-1, 2, pct );
        }
            break;

        case eSPFieldAvailExts:
            assert( pBTStats->pSpaceTrees );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pSpaceTrees->cAvailExtents );
            break;
        case eSPFieldAvailCPG:
            assert( pBTStats->pSpaceTrees );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pSpaceTrees->cpgAvailable );
            break;
        case eSPFieldAvailMB:
            assert( pBTStats->pSpaceTrees );
            wprintf(L"%*d.%03d", rgSpaceFields[eField].cchFieldSize-4,
                                CmbFromCpg(pBTStats->pSpaceTrees->cpgAvailable),
                                CkbDecimalFromCpg(pBTStats->pSpaceTrees->cpgAvailable) );
            break;
        case eSPFieldAvailPctOfTable:
            assert( pBTStats->pSpaceTrees );
            if ( pBTStats->pBasicCatalog->eType != eBTreeTypeInternalDbRootSpace && pespCtx->cpgCurrentTableOwned )
            {
                //assert( pespCtx->cpgCurrentTableOwned );
                double pct = ((double)pBTStats->pSpaceTrees->cpgAvailable) / ((double)pespCtx->cpgCurrentTableOwned) * 100;
                wprintf(L"%*.*f%%", rgSpaceFields[eField].cchFieldSize-1, 2, pct );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldSpaceTreeReservedCPG:
            assert( pBTStats->pSpaceTrees );
            assert( pComputedStats );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pComputedStats->cpgReservedForSpaceTrees );
            break;
        case eSPFieldAutoInc:
            assert( pBTStats->pSpaceTrees );
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeUserClusteredIndex && pBTStats->pSpaceTrees->fAutoIncPresents )
            {
                const QWORD qwAutoInc = pBTStats->pSpaceTrees->qwAutoInc;
                wprintf(L"%*I64d", rgSpaceFields[eField].cchFieldSize, qwAutoInc);
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldReservedCPG:
            assert( pBTStats->pSpaceTrees );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pSpaceTrees->cpgReserved );
            break;
        case eSPFieldReservedMB:
            assert( pBTStats->pSpaceTrees );
            wprintf(L"%*d.%03d", rgSpaceFields[eField].cchFieldSize-4,
                                CmbFromCpg(pBTStats->pSpaceTrees->cpgReserved),
                                CkbDecimalFromCpg(pBTStats->pSpaceTrees->cpgReserved) );
            break;
        case eSPFieldReservedPctOfTable:
            assert( pBTStats->pSpaceTrees );
            if ( pBTStats->pBasicCatalog->eType != eBTreeTypeInternalDbRootSpace && pespCtx->cpgCurrentTableOwned )
            {
                //assert( pespCtx->cpgCurrentTableOwned );
                double pct = ((double)pBTStats->pSpaceTrees->cpgReserved) / ((double)pespCtx->cpgCurrentTableOwned) * 100;
                wprintf(L"%*.*f%%", rgSpaceFields[eField].cchFieldSize-1, 2, pct );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldOwnedExtents:
            assert( pBTStats->pSpaceTrees );
            assert( pComputedStats );
            PrintMAM( (JET_HISTO*)&pComputedStats->histoOEs );
            break;
        case eSPFieldGeneralAvailExtents:
            assert( pBTStats->pSpaceTrees );
            assert( pComputedStats );
            PrintMAM( (JET_HISTO*)&pComputedStats->histoGeneralAEs );
            break;
        case eSPFieldContigAvailExtents:
            assert( pBTStats->pSpaceTrees );
            assert( pComputedStats );
            PrintMAM( (JET_HISTO*)&pComputedStats->histoContiguousAEs );
            break;

        case eSPFieldEmpty:
            assert( pBTStats->pParentOfLeaf );
            wprintf(pBTStats->pParentOfLeaf->fEmpty ? L" yes " : L"  no " );
            break;

        case eSPFieldInternalCPG:
            assert( pBTStats->pParentOfLeaf );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pParentOfLeaf->cpgInternal );
            break;
        case eSPFieldInternalMB:
            assert( pBTStats->pParentOfLeaf );
            wprintf(L"%*d.%03d", rgSpaceFields[eField].cchFieldSize-4,
                                CmbFromCpg(pBTStats->pParentOfLeaf->cpgInternal),
                                CkbDecimalFromCpg(pBTStats->pParentOfLeaf->cpgInternal) );
            break;

        case eSPFieldDepth:
            assert( pBTStats->pParentOfLeaf );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pParentOfLeaf->cDepth );
            break;

        case eSPFieldIORuns:
            PrintA( eField, pBTStats->pParentOfLeaf->phistoIOContiguousRuns );
            break;
        case eSPFieldIORunsHisto:
            PrintHisto( eField, rgIORunHistoDivisions, _countof( rgIORunHistoDivisions ), pBTStats->pParentOfLeaf->phistoIOContiguousRuns );
            break;

        case eSPFieldForwardScans:
            assert( pBTStats->pParentOfLeaf );
            wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pParentOfLeaf->cForwardScans );
            break;


        case eSPFieldIntFreeBytesMAM:
            assert( pBTStats->pParentOfLeaf );
            assert( pBTStats->pParentOfLeaf->pInternalPageStats );
            PrintMAM( pBTStats->pParentOfLeaf->pInternalPageStats->phistoFreeBytes );
            break;
        case eSPFieldIntNodeCountsMAM:
            assert( pBTStats->pParentOfLeaf );
            assert( pBTStats->pParentOfLeaf->pInternalPageStats );
            PrintMAM( pBTStats->pParentOfLeaf->pInternalPageStats->phistoNodeCounts );
            break;
        case eSPFieldIntKeySizesMAM:
            assert( pBTStats->pParentOfLeaf );
            assert( pBTStats->pParentOfLeaf->pInternalPageStats );
            PrintMAM( pBTStats->pParentOfLeaf->pInternalPageStats->phistoKeySizes );
            break;
        case eSPFieldIntKeyCompMAM:
            assert( pBTStats->pParentOfLeaf );
            assert( pBTStats->pParentOfLeaf->pInternalPageStats );
            PrintMAM( pBTStats->pParentOfLeaf->pInternalPageStats->phistoKeyCompression );
            break;

        case eSPFieldFreeBytesMAM:
            PrintMAM( pBTStats->pFullWalk->phistoFreeBytes );
            break;
        case eSPFieldNodeCountsMAM:
            PrintMAM( pBTStats->pFullWalk->phistoNodeCounts );
            break;
        case eSPFieldNodePctOfTable:
            assert( pBTStats->pFullWalk->phistoNodeCounts );
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeUserSecondaryIndex &&
                    CStatsFromPv(pBTStats->pParent->pFullWalk->phistoNodeCounts)->Total() )
            {
                const double pct = PctNormalized( CStatsFromPv(pBTStats->pFullWalk->phistoNodeCounts)->Total(),
                                                    CStatsFromPv(pBTStats->pParent->pFullWalk->phistoNodeCounts)->Total() );
                wprintf(L"%*.*f%%", rgSpaceFields[eField].cchFieldSize-1, 2, pct );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldKeySizesMAM:
            PrintMAM( pBTStats->pFullWalk->phistoKeySizes );
            break;
        case eSPFieldDataSizesMAM:
            PrintMAM( pBTStats->pFullWalk->phistoDataSizes );
            break;
        case eSPFieldKeyCompMAM:
            PrintMAM( pBTStats->pFullWalk->phistoKeyCompression );
            break;
        case eSPFieldUnreclaimedMAM:
            PrintMAM( pBTStats->pFullWalk->phistoUnreclaimedBytes );
            break;
        case eSPFieldVersionedNodes:
            wprintf(L"%*I64d", rgSpaceFields[eField].cchFieldSize, pBTStats->pFullWalk->cVersionedNodes );
            break;

        case eSPFieldLVRefs:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                assert( pBTStats->pLvData );
                wprintf( L"%*I64d", rgSpaceFields[eField].cchFieldSize, pBTStats->pLvData->cLVRefs );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldCorruptLVs:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                assert( pBTStats->pLvData );
                wprintf(L"%*I64d", rgSpaceFields[eField].cchFieldSize, pBTStats->pLvData->cCorruptLVs );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldPartiallyDeletedLVs:
            if( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                assert( pBTStats->pLvData );
                wprintf( L"%*I64d", rgSpaceFields[eField].cchFieldSize, pBTStats->pLvData->cPartiallyDeletedLVs );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldSeparatedRootChunks:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                assert( pBTStats->pLvData );
                wprintf(L"%*I64d", rgSpaceFields[eField].cchFieldSize, pBTStats->pLvData->cSeparatedRootChunks );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldLidMax:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                assert( pBTStats->pLvData );
                wprintf(L"0x%*I64x", rgSpaceFields[eField].cchFieldSize, pBTStats->pLvData->lidMax );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;

        case eSPFieldLVChunkMax:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                assert( pBTStats->pLvData );
                wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pLvData->cbLVChunkMax );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;

        case eSPFieldLVSize:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                PrintCA( eField, pBTStats->pLvData->phistoLVSize );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldLVSizeHisto:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                PrintHisto( eField, rgLVBytesDivisions, _countof( rgLVBytesDivisions ), pBTStats->pLvData->phistoLVSize );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;

        case eSPFieldLVComp:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                PrintA( eField, pBTStats->pLvData->phistoLVComp );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldLVCompHisto:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                PrintHisto( eField, rgLVBytesDivisions, _countof( rgLVBytesDivisions ), pBTStats->pLvData->phistoLVComp );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;

        case eSPFieldLVRatioHisto:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                PrintHisto( eField, rgLVRatioDivisions, _countof( rgLVRatioDivisions ), pBTStats->pLvData->phistoLVRatio );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;

        case eSPFieldLVSeeks:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                PrintA( eField, pBTStats->pLvData->phistoLVSeeks );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldLVSeeksHisto:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                PrintHisto( eField, rgLVSeeksDivisions, _countof( rgLVSeeksDivisions ), pBTStats->pLvData->phistoLVSeeks );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;

        case eSPFieldLVBytes:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                PrintA( eField, pBTStats->pLvData->phistoLVBytes );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldLVBytesHisto:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                PrintHisto( eField, rgLVBytesDivisions, _countof( rgLVBytesDivisions ), pBTStats->pLvData->phistoLVBytes );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;

        case eSPFieldLVExtraSeeks:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                PrintA( eField, pBTStats->pLvData->phistoLVExtraSeeks );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldLVExtraSeeksHisto:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                PrintHisto( eField, rgLVExtraSeeksDivisions, _countof( rgLVExtraSeeksDivisions ), pBTStats->pLvData->phistoLVExtraSeeks );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;

        case eSPFieldLVExtraBytes:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                PrintA( eField, pBTStats->pLvData->phistoLVExtraBytes );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldLVExtraBytesHisto:
            if ( pBTStats->pBasicCatalog->eType == eBTreeTypeInternalLongValue )
            {
                PrintHisto( eField, rgLVExtraBytesDivisions, _countof( rgLVExtraBytesDivisions ), pBTStats->pLvData->phistoLVExtraBytes );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;

        case eSPFieldSHInitialDensity:
            assert( pBTStats->pBasicCatalog );
            if ( pBTStats->pBasicCatalog->pSpaceHints )
            {
                wprintf(L"%*d%%", rgSpaceFields[eField].cchFieldSize-1, pBTStats->pBasicCatalog->pSpaceHints->ulInitialDensity );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;

        case eSPFieldSHInitialSize:
            if ( pBTStats->pBasicCatalog->pSpaceHints )
            {
                assert( pBTStats->pBasicCatalog->pSpaceHints->cbInitial % 1024 == 0 );
                wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pBasicCatalog->pSpaceHints->cbInitial / 1024 );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
            
        case eSPFieldSHGrbit:
            assert( pBTStats->pBasicCatalog );
            if ( pBTStats->pBasicCatalog->pSpaceHints )
            {
                WCHAR wszSpaceHintsGrbits[12];
                const JET_ERR err = ErrOSStrCbFormatW( wszSpaceHintsGrbits, sizeof(wszSpaceHintsGrbits), L"0x%x", pBTStats->pBasicCatalog->pSpaceHints->grbit );
                assert( JET_errSuccess == err );
                assert( rgSpaceFields[eField].cchFieldSize >= wcslen( wszSpaceHintsGrbits ) );
                wprintf( L"%ws%ws", WszFillBuffer( L' ', rgSpaceFields[eField].cchFieldSize - wcslen( wszSpaceHintsGrbits ) ), wszSpaceHintsGrbits );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;

        case eSPFieldSHMaintDensity:
            assert( pBTStats->pBasicCatalog );
            if ( pBTStats->pBasicCatalog->pSpaceHints )
            {
                wprintf(L"%*d%%", rgSpaceFields[eField].cchFieldSize-1, pBTStats->pBasicCatalog->pSpaceHints->ulMaintDensity );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldSHGrowthPct:
            assert( pBTStats->pBasicCatalog );
            if ( pBTStats->pBasicCatalog->pSpaceHints )
            {
                wprintf(L"%*d%%", rgSpaceFields[eField].cchFieldSize-1, pBTStats->pBasicCatalog->pSpaceHints->ulGrowth );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldSHMinExtent:
            assert( pBTStats->pBasicCatalog );
            if ( pBTStats->pBasicCatalog->pSpaceHints )
            {
                assert( pBTStats->pBasicCatalog->pSpaceHints->cbMinExtent % 1024 == 0 );
                wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pBasicCatalog->pSpaceHints->cbMinExtent / 1024 );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;
        case eSPFieldSHMaxExtent:
            assert( pBTStats->pBasicCatalog );
            if ( pBTStats->pBasicCatalog->pSpaceHints )
            {
                assert( pBTStats->pBasicCatalog->pSpaceHints->cbMaxExtent % 1024 == 0 );
                wprintf(L"%*d", rgSpaceFields[eField].cchFieldSize, pBTStats->pBasicCatalog->pSpaceHints->cbMaxExtent / 1024 );
            }
            else
            {
                PrintNullField( pespCtx, eField );
            }
            break;

        default:
            assertSz( fFalse, "Asked to print a field that doesn't exist." );
            break;
    }

    return JET_errSuccess;
}

//
//  Space dump context processing...
//

JET_ERR ErrSpaceDumpCtxInit( __out void ** ppvContext )
{
    JET_ERR err = JET_errSuccess;

    EseutilDbspacedumpUnitTest();

    if ( NULL == ppvContext )
    {
        assertSz( fFalse, "Huh?" );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    //
    //  Allocate and init the context
    //
    ESEUTIL_SPACE_DUMP_CTX * pespCtx = (ESEUTIL_SPACE_DUMP_CTX*)malloc( sizeof(ESEUTIL_SPACE_DUMP_CTX) );
    if ( pespCtx == NULL )
    {
        wprintf(L"Failed to allocate %d bytes, failing command.\n", (INT)sizeof(ESEUTIL_SPACE_DUMP_CTX) );
        return ErrERRCheck( JET_errOutOfMemory );
    }
    memset( pespCtx, 0, sizeof(ESEUTIL_SPACE_DUMP_CTX) );

    pespCtx->fPrintSpaceTrees = fFalse;
    pespCtx->fPrintSpaceNodes = fFalse;

    //  Set default separator.
    pespCtx->rgwchSep[0] = L' ';
    pespCtx->rgwchSep[1] = L'\0';

    *ppvContext = (void*)pespCtx;
    return err;
}

JET_ERR ErrSpaceDumpCtxGetGRBIT(
    __inout void *                      pvContext,
    __out JET_GRBIT *                   pgrbit )
{
    JET_ERR err = JET_errSuccess;
    ESEUTIL_SPACE_DUMP_CTX * pespCtx = (ESEUTIL_SPACE_DUMP_CTX*)pvContext;

    if ( NULL == pespCtx || NULL == pgrbit )
    {
        assertSz( fFalse, "Huh?" );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( NULL == pespCtx->rgeFields )
    {
        // User didn't request any specific fields, use default.
        Call( ErrSpaceDumpCtxSetFields( pvContext, L"#default" ) );
    }

    //  Success, set out param
    *pgrbit = pespCtx->grbit;

HandleError:

    return err;
}

//
//  "Field Sets" ...
//      consumed by PrintSpaceDumpHelp and ErrSpaceDumpCtxSetFields.
//

//      #default
//
WCHAR wszDefaultFieldsCmd [] = L"#default";
WCHAR wszDefaultFields [] = L"Name,Type,Owned(MB),O%OfDb,O%OfTable,Avail(MB),Avail%Tbl,AutoInc";

//      #legacy
//
WCHAR wszLegacyFieldsCmd [] = L"#legacy";
WCHAR wszLegacyFields [] = L"name,type,objidfdp,pgnofdp,priext,owned,available";

//      #leaked
//
WCHAR wszLeakedDataCmd [] = L"#leaked";
WCHAR wszLeakedDataFields [] = L"Name,Type,ObjidFDP,PriExt,PgnoFDP,Owned,Available,Internal,Data";

//      #spacehints
//
WCHAR wszSpaceHintsCmd [] = L"#spacehints";
WCHAR wszSpaceHintsFields [] = L"Name,Type,SH:IDensity,SH:ISize(KB),SH:grbit,SH:MDensity,SH:Growth,SH:MinExt(KB),SH:MaxExt(KB)";

//      #lvs
//
WCHAR wszLVsCmd[] = L"#lvs";
WCHAR wszLVsFields[] = L"FullName,Type,LV:Size,LV:Size(histo),LV:Comp,LV:Comp(histo),LV:Ratio(histo),LV:Seeks,LV:Seeks(histo),LV:ExtraSeeks,LV:ExtraSeeks(histo),LV:Bytes,LV:Bytes(histo),LV:ExtraBytes,LV:ExtraBytes(histo),Data:FreeBytes,Data:Nodes,Data:KeySizes,Data:DataSizes,Data:Unreclaim,VersndNode,cLVRefs,cCorrLVs,cSepRtChk,lidMax,LV:ChunkSize,OwnExt,GenAvailExt";

//          #all
//
WCHAR wszAllFieldsCmd [] = L"#all";
//  Actual fields calculated directly off the table of fields.

    
void PrintSpaceDumpHelp(
    __in const ULONG                    cchLineWidth,
    __in const WCHAR                    wchNewLine,
    __in_z const WCHAR *                wszTab1,
    __in_z const WCHAR *                wszTab2 )
{

    wprintf( L"%c", wchNewLine );
    wprintf( L"%wsSPACE USAGE OPTIONS:%c", wszTab1, wchNewLine );

#if DEBUG
    // 'Detailed' sounds a little bit vague...
    wprintf( L"%ws%ws     /d[<n>]       - Prints more detailed information on trees.%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws                      /d1 - Prints space trees (default).%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws                      /d2 - Prints space nodes.%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws     /l            - Prints leak information.%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%c", wchNewLine );
#endif

    wprintf( L"%ws%ws     /f<field[,field]>%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws     - Space info fields to print. Sets of fields%c", wszTab1, wszTab2, wchNewLine );

    wprintf( L"%ws%ws       Sets of fields:%c", wszTab1, wszTab2, wchNewLine );
    assert( 29 == wcslen(wszTab1) + wcslen(wszTab2) );  // text will need re-calibration
    wprintf( L"%ws%ws         /f#spacehints - Prints the spacehint settings%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws                         for the object.%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws         /f#default    - Produces default output.%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws         /f#legacy     - Print out the legacy set of%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws                         fields.%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws         /f#lvs        - Print out all information to%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws                         analyze LV trees.%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws         /f#all        - Print out all fields for%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws                         which we collect stats.%c", wszTab1, wszTab2, wchNewLine );
#ifdef DEBUG
    wprintf( L"%ws%ws         /f#leaked     - Print out all information to%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws                         analyze space leaks.%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws                         Note: Returns error on leaks%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws                         or space overbookings.%c", wszTab1, wszTab2, wchNewLine );
#endif


    wprintf( L"%ws%ws       Independent fields:%c", wszTab1, wszTab2, wchNewLine );

    ULONG cchUsed = 0;
    const ULONG cchTab = (ULONG) ( wcslen(wszTab1) + wcslen(wszTab2) + 4 );

    for( ULONG eField = 1; eField < _countof(rgSpaceFields); eField++ )
    {
        if ( cchUsed < cchTab )
        {
            assert( 0 == cchUsed );
            wprintf( L"%ws     ", WszFillBuffer( L' ', cchTab ) );
            cchUsed += cchTab;
        }

        const WCHAR * wszDelimator = FLastField(eField) ? L"" : L", ";
        if ( ( cchTab == cchUsed ) || // first check ensures 1 field printed ...
            ( ( wcslen(wszDelimator) + cchUsed + wcslen(rgSpaceFields[eField].wszField) ) < cchLineWidth ) )
        {
            wprintf( L"%ws%ws", rgSpaceFields[eField].wszField, wszDelimator );
            cchUsed += (ULONG) ( wcslen(rgSpaceFields[eField].wszField) + wcslen(wszDelimator) );
        }
        else
        {
            wprintf( L"%c", wchNewLine );
            cchUsed = 0;
        }
    }
    if ( cchUsed != 0 )
    {
        wprintf( L"%c", wchNewLine );
    }

    wprintf( L"%c", wchNewLine );
    wprintf( L"%ws%ws         /csv          - Print all fields CSV delimited.%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%c", wchNewLine );

}


JET_ERR ErrSpaceDumpCtxSetFields(
    __inout void *                      pvContext,
    __in_z const WCHAR *                wszFields  )
{
    JET_ERR err = JET_errSuccess;
    ESEUTIL_SPACE_DUMP_CTX * pespCtx = (ESEUTIL_SPACE_DUMP_CTX*)pvContext;

    if ( NULL == pespCtx || NULL == wszFields )
    {
        assertSz( fFalse, "Huh?" );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( pespCtx->rgeFields )
    {
        assertSz( fFalse, "Fields were already set?" );
        return ErrERRCheck( JET_errInvalidParameter );
    }

#ifdef DEBUG
    //
    //  Validating the rgSpaceFields and enum are in check.
    //
    for( ULONG eField = 0; eField < sizeof(rgSpaceFields)/sizeof(rgSpaceFields[0]); eField++ )
    {
        assert( eField == (ULONG)rgSpaceFields[eField].eField );
        assert( (ULONG)wcslen(rgSpaceFields[eField].wszField) <= rgSpaceFields[eField].cchFieldSize );  // field header larger than field width.
    }
#endif

    //
    //  Parse the field arguments.
    //

    //  Identify no or special arguments.
    assert( wszFields );
    if ( 0 == _wcsicmp( wszFields, wszDefaultFieldsCmd ) )
    {
        wszFields = wszDefaultFields;
    }
    else if ( 0 == _wcsicmp( wszFields, wszLegacyFieldsCmd ) )
    {
        wszFields = wszLegacyFields;
        pespCtx->fPrintSmallTrees = fTrue;
    }
    else if ( 0 == _wcsicmp( wszFields, wszLeakedDataCmd ) )
    {
        wszFields = wszLeakedDataFields;
        pespCtx->fPrintSmallTrees = fTrue;
        pespCtx->fPrintSpaceTrees = fTrue;
        pespCtx->fPrintSpaceLeaks = fTrue;
    }
    else if ( 0 == _wcsicmp( wszFields, wszSpaceHintsCmd ) )
    {
        wszFields = wszSpaceHintsFields;
    }
    else if ( 0 == _wcsicmp( wszFields, wszLVsCmd ) )
    {
        wszFields = wszLVsFields;
    }
    else if ( 0 == _wcsicmp( wszFields, wszAllFieldsCmd ) )
    {
        ULONG cb = sizeof(WCHAR);   // for NUL
        for( ULONG eField = 1; eField < sizeof(rgSpaceFields)/sizeof(rgSpaceFields[0]); eField++ )
        {
            cb += (ULONG)( sizeof(WCHAR) * ( 1 + wcslen(rgSpaceFields[eField].wszField) ) );
        }
        WCHAR * wszTemp;
        Alloc( wszTemp = (WCHAR*)malloc( cb ) );
        wszTemp[0] = L'\0';
        for( ULONG eField = 1; eField < sizeof(rgSpaceFields)/sizeof(rgSpaceFields[0]); eField++ )
        {
            ULONG ret = StringCbCatW( (WCHAR*)wszTemp, cb, rgSpaceFields[eField].wszField );
            assert( 0 == ret );
            ret = StringCbCatW( (WCHAR*)wszTemp, cb, L"," );
            assert( 0 == ret );
        }
        wszFields = wszTemp;
    }

    //  Parse the fields from the wszFields argument
    ULONG cb = sizeof(E_SP_FIELD);
    WCHAR * pchSep = (WCHAR*)wszFields;
    while ( pchSep = wcschr( (WCHAR*)pchSep, L',' ) )
    {
        pchSep++;
        cb += sizeof(E_SP_FIELD);
    }

    //  If the last character was a comma, we have over allocated.
    ULONG cchFields = (ULONG) ( wszFields ? wcslen(wszFields) : 0 );
    if ( cchFields && wszFields[cchFields-1] == L',' )
    {
        assert( cb >= sizeof(E_SP_FIELD) );
        cb -= sizeof(E_SP_FIELD);
    }
    
    pespCtx->rgeFields = (E_SP_FIELD*) malloc( cb );
    if ( NULL == pespCtx->rgeFields )
    {
        wprintf(L"Failed to allocate %d bytes, failing command.\n", cb );
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    pespCtx->grbit |= JET_bitDBUtilSpaceInfoBasicCatalog;
    ULONG ieFields = 0;
    const WCHAR * wszField = wszFields;
    while ( wszField && wszField[0] != L'\0' )
    {
        pchSep = wcschr( (WCHAR*)wszField, L',' );
        if ( pchSep )
        {
            *pchSep = L'\0';    // temporarily unset the comma.
        }
        ULONG eField;
        if ( !FFindField( wszField, &eField ) )
        {
            wprintf(L"Did not recognize the field: %ws\n", wszField );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        pespCtx->rgeFields[ieFields++] = (E_SP_FIELD)eField;
        pespCtx->grbit |= rgSpaceFields[eField].grbitRequired;

        if ( pchSep )
        {
            *pchSep = L',';
            wszField = pchSep+1;
        }
        else
        {
            wszField = wszField + wcslen(wszField);
        }
    }
    assert( wszField == NULL || wszField[0] == L'\0' );

    pespCtx->cFields = ieFields;

    assert( !( pespCtx->cFields && cb != (pespCtx->cFields * sizeof(E_SP_FIELD)) ) );

    if ( !pespCtx->cFields )
    {
        //  No fields, will use /f#default.
        free( pespCtx->rgeFields );
        pespCtx->rgeFields = NULL;
    }
    
    err = JET_errSuccess;

HandleError:

    return err;
}

JET_ERR ErrSpaceDumpCtxSetOptions(
    __inout void *                      pvContext,
    __in_opt const ULONG *              pcbPageSize,
    __in_opt const JET_GRBIT *          pgrbitAdditional,
    __in_z_opt const PWSTR              wszSeparator,
    __in const SPDUMPOPTS               fSPDumpOpts
    )
{
    JET_ERR err = JET_errSuccess;
    ESEUTIL_SPACE_DUMP_CTX * pespCtx = (ESEUTIL_SPACE_DUMP_CTX*)pvContext;

    if ( NULL == pespCtx )
    {
        assertSz( fFalse, "Huh?" );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( pcbPageSize )
    {
        pespCtx->cbPageSize = *pcbPageSize;
    }

    if ( pgrbitAdditional )
    {
        pespCtx->grbit |= *pgrbitAdditional;
    }

    if ( fSPDumpPrintSmallTrees & fSPDumpOpts )
    {
        pespCtx->fPrintSmallTrees = fTrue;
    }

    if ( fSPDumpPrintSpaceTrees & fSPDumpOpts )
    {
        pespCtx->fPrintSpaceTrees = fTrue;
    }

    if ( fSPDumpPrintSpaceNodes & fSPDumpOpts )
    {
        pespCtx->fPrintSpaceNodes = fTrue;
    }

    if ( fSPDumpPrintSpaceLeaks & fSPDumpOpts )
    {
        //  To process space leaks, we need parent of leaf info.
        pespCtx->grbit |= JET_bitDBUtilSpaceInfoParentOfLeaf;
        pespCtx->fPrintSpaceLeaks = fTrue;
    }

    if ( fSPDumpSelectOneTable & fSPDumpOpts )
    {
        pespCtx->fSelectOneTable = fTrue;
    }

    if( wszSeparator )
    {
        //  We only allow 1 char + NUL separators, likely it was wrong.
        assert( wszSeparator[0] != L'\0' );
        assert( wszSeparator[1] == L'\0' );
        if ( S_OK != StringCbCopyW( pespCtx->rgwchSep, sizeof(pespCtx->rgwchSep), wszSeparator ) )
        {
            assertSz( fFalse, "Huh?" );
            return ErrERRCheck( JET_errInvalidParameter );
        }
    }

    return err;
}

ULONG CpgSpaceDumpGetOwnedNonShelved(
    __in const ESEUTIL_SPACE_DUMP_CTX * const pespCtx,
    __in const BTREE_SPACE_EXTENT_INFO * const pextOwned )
{
    if ( pextOwned->pgnoLast <= pespCtx->pgnoLast )
    {
        return 0;
    }

    ULONG cpgNonShelved = 0;
    const ULONG pgnoFirstOwned = PgnoFirstFromExt( pextOwned );
    const ULONG pgnoLastOwned = pextOwned->pgnoLast;
    ULONG pgnoFirstShelved = ulMax;
    ULONG pgnoLastShelved = ulMax;
    Assert( pgnoFirstOwned > pespCtx->pgnoLast );

    for ( ULONG iextShelved = 0; iextShelved < pespCtx->cShelvedExtents; iextShelved++ )
    {
        const BTREE_SPACE_EXTENT_INFO * const pextShelved = &pespCtx->prgShelvedExtents[ iextShelved ];
        Assert( pextShelved->iPool == ulsppShelvedPool );
        Assert( pextShelved->cpgExtent > 0 );

        if ( FDBSPUTLContains( pextOwned, pextShelved ) )
        {
            if ( pgnoFirstShelved == ulMax )
            {
                pgnoFirstShelved = PgnoFirstFromExt( pextShelved );
                pgnoLastShelved = pextShelved->pgnoLast;
                if ( pgnoFirstShelved > pgnoFirstOwned  )
                {
                    cpgNonShelved += ( pgnoFirstShelved - pgnoFirstOwned );
                }
            }
            else
            {
                if ( ( PgnoFirstFromExt( pextShelved ) - 1 ) > pgnoLastShelved )
                {
                    cpgNonShelved += ( ( PgnoFirstFromExt( pextShelved ) - 1 ) - pgnoLastShelved );
                }
                pgnoLastShelved = pextShelved->pgnoLast;
            }
        }
    }

    if ( pgnoFirstShelved == ulMax )
    {
        cpgNonShelved = ( pgnoLastOwned - pgnoFirstOwned + 1 );
    }
    else if ( pgnoLastShelved < pgnoLastOwned )
    {
        cpgNonShelved += ( pgnoLastOwned - pgnoLastShelved );
    }

    return cpgNonShelved;
}

ULONG CpgSpaceDumpGetExtPagesEof(
    __in const ESEUTIL_SPACE_DUMP_CTX * const pespCtx,
    __in const BTREE_SPACE_EXTENT_INFO * const prgext,
    __in const ULONG cext )
{
    ULONG cpgEof = 0;

    for ( ULONG iext = cext; ( iext-- >= 1 ) && ( prgext[iext].pgnoLast > pespCtx->pgnoLast ) ; )
    {
        const BTREE_SPACE_EXTENT_INFO * const pext = prgext + iext;
        Assert( pext->iPool != ulsppShelvedPool );
        Assert( PgnoFirstFromExt( pext ) > pespCtx->pgnoLast );
        cpgEof += pext->cpgExtent;
    }

    return cpgEof;
}


ULONG /* cch */ EseutilSpaceDumpTableWidth(
    __in const ESEUTIL_SPACE_DUMP_CTX * const       pespCtx
    )
{
    ULONG cchTableWidth = 0;
    for( ULONG ieField = 0; ieField < pespCtx->cFields; ieField++ )
    {
        cchTableWidth += rgSpaceFields[pespCtx->rgeFields[ieField]].cchFieldSize + 1;
    }
    return cchTableWidth;
}

JET_ERR EseutilSpaceDumpPrintHeadersAndDbRoot(
    __in const BTREE_STATS * const      pDbRoot,
    __in ESEUTIL_SPACE_DUMP_CTX *       pespCtx )
{
    JET_ERR err = JET_errSuccess;

    //
    //  First, print the space dump and field header output.
    //

    // original...
    //wprintf( L"******************************** SPACE DUMP ***********************************\n" );
    //printf( "Name                   Type   ObjidFDP    PgnoFDP  PriExt      Owned  Available\n" );

    wprintf( L"        PageSize: %d\n\n", pespCtx->cbPageSize );

    WCHAR * wszSpaceDump = L"******************************** SPACE DUMP *******";
    ULONG cchTableWidth = (ULONG) max( wcslen( wszSpaceDump ), EseutilSpaceDumpTableWidth( pespCtx ) );
    ULONG cchTableLeft = cchTableWidth;
    wprintf( L"%ws", wszSpaceDump );
    cchTableLeft -= (ULONG) wcslen( wszSpaceDump );
    for( ULONG ieField = 0; cchTableLeft && ieField < pespCtx->cFields; ieField++ )
    {
        ULONG cchField = rgSpaceFields[pespCtx->rgeFields[ieField]].cchFieldSize + 1;
        cchField = min( cchTableLeft, cchField );
        wprintf( L"%ws", WszFillBuffer( L'*', cchField ) );
        cchTableLeft -= cchField;
    }
    wprintf( L"\n" );

    BOOL fSecondHeaders = fFalse;
    for( ULONG ieField = 0; ieField < pespCtx->cFields; ieField++ )
    {
        ULONG cExtraDelimitters = 0;
        if ( rgSpaceFields[pespCtx->rgeFields[ieField]].wszSubFields )
        {
            fSecondHeaders = fTrue;
            cExtraDelimitters = CchCountOfChar( rgSpaceFields[pespCtx->rgeFields[ieField]].wszSubFields, L',' );
            assert( cExtraDelimitters );
        }

        if ( eSPFieldName == pespCtx->rgeFields[ieField] ||
                eSPFieldNameFull == pespCtx->rgeFields[ieField] )
        {
            wprintf( L"%ws%ws",
                        rgSpaceFields[pespCtx->rgeFields[ieField]].wszField,
                        WszFillBuffer( L' ', rgSpaceFields[pespCtx->rgeFields[ieField]].cchFieldSize - wcslen( rgSpaceFields[pespCtx->rgeFields[ieField]].wszField ) ) );
        }
        else
        {
            if ( pespCtx->rgwchSep[0] == L',' &&
                pespCtx->rgwchSep[1] == L'\0' &&
                rgSpaceFields[pespCtx->rgeFields[ieField]].wszSubFields )
            {
                // In CSV mode, we must keep the right number of CSV fields, or we won't be "CSV compliant"
                wprintf( L"%ws%ws%ws",
                        WszFillBuffer( L' ', rgSpaceFields[pespCtx->rgeFields[ieField]].cchFieldSize - wcslen( rgSpaceFields[pespCtx->rgeFields[ieField]].wszField ) - cExtraDelimitters ),
                        WszFillBuffer( L',', cExtraDelimitters  ),
                        rgSpaceFields[pespCtx->rgeFields[ieField]].wszField );
            }
            else
            {
                wprintf( L"%ws%ws",
                        WszFillBuffer( L' ', rgSpaceFields[pespCtx->rgeFields[ieField]].cchFieldSize - wcslen( rgSpaceFields[pespCtx->rgeFields[ieField]].wszField ) ),
                        rgSpaceFields[pespCtx->rgeFields[ieField]].wszField );
            }
        }
        PrintFieldSep ( pespCtx, ieField );
    }
    if ( fSecondHeaders )
    {
        for( ULONG ieField = 0; ieField < pespCtx->cFields; ieField++ )
        {
            if ( rgSpaceFields[pespCtx->rgeFields[ieField]].wszSubFields )
            {
                wprintf( L"%ws", rgSpaceFields[pespCtx->rgeFields[ieField]].wszSubFields );
            }
            else
            {
                wprintf( L"%ws", WszFillBuffer( L' ', rgSpaceFields[pespCtx->rgeFields[ieField]].cchFieldSize ) );
            }
            PrintFieldSep ( pespCtx, ieField );
        }
    }

    cchTableLeft = cchTableWidth;
    for( ULONG ieField = 0; cchTableLeft && ieField < pespCtx->cFields; ieField++ )
    {
        ULONG cchField = rgSpaceFields[pespCtx->rgeFields[ieField]].cchFieldSize + 1;
        cchField = min( cchTableLeft, cchField );
        wprintf( L"%ws", WszFillBuffer( L'=', cchField ) );
        cchTableLeft -= cchField;
    }
    wprintf( L"\n" );

    //  Do any post-collection stats computations ...
    //
    BTREE_POST_COMPUTED_SPACE_STATS statsComputed;
    Call( ErrCalculatePostComputedSpaceStats( pDbRoot, &statsComputed ) );

    //
    //  Now print the fields for the DB Root itself.
    //
    for( ULONG ieField = 0; ieField < pespCtx->cFields; ieField++ )
    {
        Call( ErrPrintField( pespCtx, pDbRoot, &statsComputed, pespCtx->rgeFields[ieField] ) );
        PrintFieldSep ( pespCtx, ieField );
    }
    wprintf( L"\n" );

HandleError:

    return err;
}

JET_ERR EseutilPrintSpecifiedSpaceFields(
    __in const ESEUTIL_SPACE_DUMP_CTX * const   pespCtx,
    __in const BTREE_STATS * const              pBTreeStats
    )
{
    JET_ERR err = JET_errSuccess;

    //  Do any post-collection stats computations ...
    //
    BTREE_POST_COMPUTED_SPACE_STATS statsComputed;
    Call( ErrCalculatePostComputedSpaceStats( pBTreeStats, &statsComputed ) );

    //  Print out all fields
    //
    for( ULONG ieField = 0; ieField < pespCtx->cFields; ieField++ )
    {
        Call( ErrPrintField( pespCtx, pBTreeStats, &statsComputed, pespCtx->rgeFields[ieField] ) );
        PrintFieldSep ( pespCtx, ieField );
    }

HandleError:

    return err;
}

JET_ERR EseutilPrintSpaceTrees(
    _In_ const ESEUTIL_SPACE_DUMP_CTX * const   pespCtx,
    _In_ JET_PCWSTR                             wszTable,
    _In_ JET_PCWSTR                             wszTree,
    _In_ const BTREE_STATS_SPACE_TREES * const  pSpaceTrees
    )
{
    JET_ERR err = JET_errSuccess;

    bool * rgfAeHandled = new bool[ pSpaceTrees->cAvailExtents ];
    AllocR( rgfAeHandled );
    memset( rgfAeHandled, 0, sizeof( bool ) * pSpaceTrees->cAvailExtents );

    //  Iterate over / print out each OwnExtent - appending / print all AvailExtents contained within the OE.
    //
    for ( ULONG iextOwned = 0; iextOwned < pSpaceTrees->cOwnedExtents; iextOwned++ )
    {
        const BTREE_SPACE_EXTENT_INFO * const pextOwned = &pSpaceTrees->prgOwnedExtents[ iextOwned ];
        const ULONG cpgNonShelved = CpgSpaceDumpGetOwnedNonShelved( pespCtx, pextOwned );

        //  Start line with basic Owned Extent info...
        //
        Assert( ( pextOwned->pgnoLast <= pespCtx->pgnoLast ) || ( PgnoFirstFromExt( pextOwned ) > pespCtx->pgnoLast ) );
        wprintf( L"        space[%ws\\%ws]  OE[  %5I32u]: %6I32u - %6I32u (%4I32u, ",
                    wszTable, wszTree,
                    pextOwned->pgnoSpaceNode,
                    PgnoFirstFromExt( pextOwned ),
                    pextOwned->pgnoLast,
                    pextOwned->cpgExtent );

        // Is this extent beyond the EOF?
        if ( pextOwned->pgnoLast > pespCtx->pgnoLast )
        {
            // If this extent is beyond EOF, we expect matching shelved extents covering the entire range.
            if ( cpgNonShelved > 0 )
            {
                wprintf( L">EOF[ERROR:%I32u non-shelved]", cpgNonShelved );
            }
            else
            {
                wprintf( L">EOF[OK]" );
            }
        }
        else
        {
            Assert( cpgNonShelved == 0 );
        }
        wprintf( L"):  " );

        //  Count up avail extents for printing summary Avail Extent info ...
        //
        ULONG cextAvailInOwned = 0;
        ULONG cpgAvailInOwned = 0;
        ULONG cextContigMarkersInOwned = 0;
        for ( ULONG iextAvail = 0; iextAvail < pSpaceTrees->cAvailExtents; iextAvail++ )
        {
            const BTREE_SPACE_EXTENT_INFO * const pextAvail = &pSpaceTrees->prgAvailExtents[ iextAvail ];

            // If your Pool is not within one of these, make sure that we'll update all our ext and cpg avail tracking
            // correctly below, and then extend the asserts to cover your new pool.
            Expected( ( pextAvail->iPool >= ulsppAvailExtLegacyGeneralPool && pextAvail->iPool <= ulsppShelvedPool ) ||
                      pextAvail->iPool == ulsppPrimaryExt ||
                      pextAvail->iPool == ulsppOwnedTreeAvail ||
                      pextAvail->iPool == ulsppAvailTreeAvail );

            if ( pextAvail->cpgExtent != 0 /* suppress marker extents */ && FDBSPUTLContains( pextOwned, pextAvail ) )
            {
                cpgAvailInOwned += pextAvail->cpgExtent;
                cextAvailInOwned++;
            }
            else if ( pextAvail->cpgExtent == 0 && FDBSPUTLContains( pextOwned, pextAvail->pgnoLast ) )
            {
                Assert( pextAvail->iPool == ulsppContinuousPool );
                cextContigMarkersInOwned++;
            }
        }
        if ( cextAvailInOwned || cextContigMarkersInOwned )
        {
            wprintf( L"AEs(%I32u extents, %I32u markers, %I32u pages)  Extents: ", cextAvailInOwned, cextContigMarkersInOwned, cpgAvailInOwned );
        }

        //  Reset and print out detailed Avail Extent info ...
        //
        for ( ULONG iextAvail = 0; iextAvail < pSpaceTrees->cAvailExtents; iextAvail++ )
        {
            const BTREE_SPACE_EXTENT_INFO * const pextAvail = &pSpaceTrees->prgAvailExtents[iextAvail];

            // Similar to above - if your Pool is not within one of these, we'll print it out below possibly correctly
            // because of WszPoolName(), but you should make sure the printing is clear on the meaning of if it is actually
            // "available space" (for instance the -Marker means not really available).
            Expected( ( pextAvail->iPool >= ulsppAvailExtLegacyGeneralPool && pextAvail->iPool <= ulsppShelvedPool ) ||
                      pextAvail->iPool == ulsppPrimaryExt ||
                      pextAvail->iPool == ulsppOwnedTreeAvail ||
                      pextAvail->iPool == ulsppAvailTreeAvail );

            if ( pextAvail->cpgExtent != 0 ? FDBSPUTLContains( pextOwned, pextAvail ) : FDBSPUTLContains( pextOwned, pextAvail->pgnoLast ) )
            {
                wprintf( L"[%I32u]%I32u-%I32u (%I32u, Pool: %ws%ws), ",
                         pextAvail->pgnoSpaceNode,
                         pextAvail->cpgExtent ? PgnoFirstFromExt( pextAvail ) : pextAvail->pgnoLast,
                         pextAvail->pgnoLast,
                         pextAvail->cpgExtent,
                         WszPoolName( (SpacePool)pextAvail->iPool ),
                         ( pextAvail->iPool == ulsppContinuousPool && pextAvail->cpgExtent == 0 ) ?
                            L"-Marker" :
                            ( ( pextAvail->iPool == ulsppShelvedPool && pextAvail->pgnoLast <= pespCtx->pgnoLast ) ?
                                L"-<EOF" :
                                L"" ) );

                Assert( rgfAeHandled[ iextAvail ] == false ); // we should only visit once.  Perhaps overlapping OwnExtents?!?
                rgfAeHandled[ iextAvail ] = true;
            }
        }
        wprintf( L"\n" );
    }

    //  Finish by printing out any AEs we didn't cover within the OE trees!
    //  Shelved extents beyond EOF will be printed here.
    //
    ULONG iPool = ulMax, iextAvailFirst = ulMax, iextAvailLast = ulMax;
    for ( ULONG iextAvailScan = 0; iextAvailScan < pSpaceTrees->cAvailExtents; iextAvailScan++ )
    {
        BOOL fProcessed = rgfAeHandled[ iextAvailScan ];

        const BTREE_SPACE_EXTENT_INFO * const pextAvailScan = &pSpaceTrees->prgAvailExtents[iextAvailScan];
        const BOOL fShelved = ( pextAvailScan->iPool == ulsppShelvedPool );
        const BOOL fBeyondEof = ( pextAvailScan->pgnoLast > pespCtx->pgnoLast );
        Expected( fProcessed || ( fShelved && fBeyondEof && ( PgnoFirstFromExt( pextAvailScan ) > pespCtx->pgnoLast ) && ( pextAvailScan->cpgExtent == 1 ) ) );

        BOOL fEndOfRun = fFalse;

        Assert( ( ( iPool == ulMax ) && ( iextAvailFirst == ulMax ) && ( iextAvailLast == ulMax ) ) ||
                ( ( iPool != ulMax ) && ( iextAvailFirst != ulMax ) && ( iextAvailLast != ulMax ) && ( iextAvailFirst <= iextAvailLast ) ) );
        if ( !fProcessed )
        {
            if ( iPool == ulMax )
            {
                // We are initiating printing of a run.
                iextAvailFirst = iextAvailScan;
                iextAvailLast = iextAvailScan;
                iPool = pextAvailScan->iPool;
            }
            else if ( ( ( pextAvailScan->pgnoLast - pextAvailScan->cpgExtent ) == pSpaceTrees->prgAvailExtents[iextAvailLast].pgnoLast ) &&
                      ( iPool == pSpaceTrees->prgAvailExtents[iextAvailLast].iPool ) )
            {
                // We are continuing a run.
                iextAvailLast = iextAvailScan;
            }
            else
            {
                // We've reached the end of a run.
                fEndOfRun = fTrue;
            }
        }
        else if ( iPool != ulMax )
        {
            // We've reached the end of a run.
            fEndOfRun = fTrue;
        }

        Assert( ( ( iPool == ulMax ) && ( iextAvailFirst == ulMax ) && ( iextAvailLast == ulMax ) ) ||
                ( ( iPool != ulMax ) && ( iextAvailFirst != ulMax ) && ( iextAvailLast != ulMax ) && ( iextAvailFirst <= iextAvailLast ) ) );

        const BOOL fLastExtent = ( iextAvailScan == ( pSpaceTrees->cAvailExtents - 1 ) );

        //  Print range if we just concluded a run or if this is the last extent and there's a run being built.
        //
        if ( fEndOfRun || ( fLastExtent && ( iPool != ulMax ) ) )
        {
            Assert( ( ( iPool != ulMax ) && ( iextAvailFirst != ulMax ) && ( iextAvailLast != ulMax ) && ( iextAvailFirst <= iextAvailLast ) ) );

            const ULONG pgnoFirst = PgnoFirstFromExt( &pSpaceTrees->prgAvailExtents[iextAvailFirst] );
            const ULONG pgnoLast = pSpaceTrees->prgAvailExtents[iextAvailLast].pgnoLast;
            Assert( ( pgnoLast <= pespCtx->pgnoLast ) || ( iPool == ulsppShelvedPool ) );

            wprintf( L"        space[%ws\\%ws]   Pool: %ws: %6I32u - %6I32u (%4I32u%ws):  Extents:  ",
                        wszTable, wszTree,
                        WszPoolName( (SpacePool)iPool ),
                        pgnoFirst,
                        pgnoLast,
                        pgnoLast - pgnoFirst + 1,
                        ( ( pgnoLast > pespCtx->pgnoLast ) ?
                            ( ( iPool == ulsppShelvedPool ) ? L", >EOF[OK]" : L", >EOF[ERROR:non-shelved]" ) :
                            L"" ) );


            //  Print out detailed Avail Extent info ...
            //
            for ( ULONG iextAvailDetail = iextAvailFirst; iextAvailDetail <= iextAvailLast; iextAvailDetail++ )
            {
                const BTREE_SPACE_EXTENT_INFO * const pextAvailDetail = &pSpaceTrees->prgAvailExtents[iextAvailDetail];
                Assert( rgfAeHandled[ iextAvailDetail ] || ( iPool == ulsppShelvedPool ) );

                wprintf( L"[%I32u]%I32u-%I32u (%I32u, Pool: %ws%ws%ws), ",
                         pextAvailDetail->pgnoSpaceNode,
                         pextAvailDetail->cpgExtent ? PgnoFirstFromExt( pextAvailDetail ) : pextAvailDetail->pgnoLast,
                         pextAvailDetail->pgnoLast,
                         pextAvailDetail->cpgExtent,
                         WszPoolName( (SpacePool)pextAvailDetail->iPool ),
                         ( pextAvailDetail->iPool == ulsppContinuousPool && pextAvailDetail->cpgExtent == 0 ) ?
                            L"-Marker" :
                            ( ( pextAvailDetail->iPool == ulsppShelvedPool && pextAvailDetail->pgnoLast <= pespCtx->pgnoLast ) ?
                                L"-<EOF" :
                                L"" ),
                         ( !rgfAeHandled[ iextAvailDetail ] && ( iPool != ulsppShelvedPool ) ) ? L", ERROR:not-owned" : L"" );

                Assert( rgfAeHandled[ iextAvailDetail ] == false );
                rgfAeHandled[ iextAvailDetail ] = true;
            }
            wprintf( L"\n" );

            iextAvailFirst = ulMax;
            iextAvailLast = ulMax;
            iPool = ulMax;
        }

        //  Switch to the next run.
        //
        if ( fEndOfRun && !fProcessed )
        {
            // Rewind iextAvailScan.
            iextAvailScan--;
        }
    }

    Assert( ( iPool == ulMax ) && ( iextAvailFirst == ulMax ) && ( iextAvailLast == ulMax ) );

#ifdef DEBUG
    for ( ULONG iextAvailScan = 0; iextAvailScan < pSpaceTrees->cAvailExtents; iextAvailScan++ )
    {
        Assert( rgfAeHandled[ iextAvailScan ] );
    }
#endif

    delete [] rgfAeHandled;

    return err;
}



BOOL FChildOfDbRoot( __in const BTREE_STATS * const pBTreeStats )
{
    if ( pBTreeStats->pParent == NULL )
        return fFalse;
    if ( pBTreeStats->pParent->pBasicCatalog->objidFDP != 1 /* DbRoot */ )
        return fFalse;
    return fTrue;
}
BOOL FChildOfTable( __in const BTREE_STATS * const pBTreeStats )
{
    if ( pBTreeStats->pParent == NULL )
        return fFalse;
    if ( pBTreeStats->pParent->pBasicCatalog->eType != eBTreeTypeUserClusteredIndex )
        return fFalse;
    return fTrue;
}
BOOL FChildOfLV( __in const BTREE_STATS * const pBTreeStats )
{
    if ( pBTreeStats->pParent == NULL )
        return fFalse;
    if ( pBTreeStats->pParent->pBasicCatalog->eType != eBTreeTypeInternalLongValue )
        return fFalse;
    return fTrue;
}
BOOL FChildOfSecIdx( __in const BTREE_STATS * const pBTreeStats )
{
    if ( pBTreeStats->pParent == NULL )
        return fFalse;
    if ( pBTreeStats->pParent->pBasicCatalog->eType != eBTreeTypeUserSecondaryIndex )
        return fFalse;
    return fTrue;
}

// There are three cases:
// Database ends with avail pages: (Begin = x, End = pgnoEOF)
// Database ends with used pages: (Begin = pgnoEOF+1, End = lMax)
// Database is completely full: (Begin = pgnoEOF+1, End = lMax)
void EseutilCalculateLastAvailableDbLogicalExtentRange(
    _In_ const BTREE_STATS_SPACE_TREES* pSpaceTrees,
    _Out_ ULONG* ppgnoDbEndUnusedBegin,
    _Out_ ULONG* ppgnoDbEndUnusedEnd )
{
    const BTREE_SPACE_EXTENT_INFO* const rgAE = pSpaceTrees->prgAvailExtents;
    const size_t cAE = pSpaceTrees->cAvailExtents;

    const BTREE_SPACE_EXTENT_INFO* const rgOE = pSpaceTrees->prgOwnedExtents;
    const size_t cOE = pSpaceTrees->cOwnedExtents;

    AssertPREFIX( cOE >= 1 );

    // Did the caller pass in data for a table?
    AssertSz( PgnoFirstFromExt( &( rgOE[ 0 ] ) ) == 1, __FUNCTION__ " should be called on the root OE/AE only." );

    *ppgnoDbEndUnusedBegin = lMax;
    *ppgnoDbEndUnusedEnd = lMax;

    const ULONG pgnoOELast = rgOE[ cOE - 1 ].pgnoLast;

    // Look for last extent which isn't shelved.
    size_t cAvailAE = cAE;
    while ( ( cAvailAE > 0 ) && ( rgAE[ cAvailAE - 1 ].iPool == ulsppShelvedPool ) )
    {
        cAvailAE--;
    }

    if ( cAvailAE == 0 )
    {
        // No available space in the database!
        *ppgnoDbEndUnusedBegin = pgnoOELast + 1;
    }
    else
    {
        const ULONG pgnoAELast = rgAE[ cAvailAE - 1 ].pgnoLast;

        if ( pgnoOELast == pgnoAELast && rgAE[ cAE - 1 ].cpgExtent != 0 )
        {
            // If they are the same, then the database ends with free space. The End is easy to find,
            // but we have to manually walk the AE's and see if they are indeed all contiguous.
            *ppgnoDbEndUnusedBegin = PgnoFirstFromExt( &rgAE[ cAvailAE - 1 ] );
            *ppgnoDbEndUnusedEnd = pgnoAELast;

            for ( INT iCurrentAE = (INT) cAvailAE - 1; iCurrentAE > 0; --iCurrentAE )
            {
                // DB Space is currently all from the 'General' pool or split buffers.
                Assert( ( rgAE[ iCurrentAE ].iPool == ulsppAvailExtLegacyGeneralPool ) ||
                        ( rgAE[ iCurrentAE ].iPool == ulsppOwnedTreeAvail ) ||
                        ( rgAE[ iCurrentAE ].iPool == ulsppAvailTreeAvail ) );

                // Is the passed-in list in sorted order? If not, then we will get bad results.
                Assert( rgAE[ iCurrentAE - 1 ].pgnoLast < rgAE[ iCurrentAE ].pgnoLast );

                if ( rgAE[ iCurrentAE - 1 ].pgnoLast == PgnoFirstFromExt( &rgAE[ iCurrentAE ] ) - 1 )
                {
                    // It's a continuation of free pages. Update the 'Begin' page.
                    *ppgnoDbEndUnusedBegin = PgnoFirstFromExt( &rgAE[ iCurrentAE - 1 ] );
                }
                else
                {
                    // We found a discontinuity! The Begin page must be the previous AE we examined.
                    break;
                }
            }

            Assert( *ppgnoDbEndUnusedBegin != lMax );
        }
        else
        {
            // If the last OE and AE are different, then there are used pages at the end of the database.
            *ppgnoDbEndUnusedBegin = pgnoOELast + 1;
        }
    }

    Assert( ( *ppgnoDbEndUnusedBegin <= *ppgnoDbEndUnusedEnd && *ppgnoDbEndUnusedEnd < lMax ) ||
            ( *ppgnoDbEndUnusedBegin == rgOE[ cOE - 1 ].pgnoLast + 1 && *ppgnoDbEndUnusedEnd == lMax ));
}


void EseutilTrackSpace(
    __inout ESEUTIL_SPACE_DUMP_CTX * const  pespCtx,
    __in const BTREE_STATS * const          pBTreeStats,
    __in const JET_BTREETYPE                eBTType
    )
{
    if ( !( pBTreeStats->pSpaceTrees && pBTreeStats->pParentOfLeaf ) )
    {
        return;
    }

    ULONG cpgNonShelved = 0;

    if ( pBTreeStats->pSpaceTrees->prgOwnedExtents )
    {
        for ( ULONG iext = pBTreeStats->pSpaceTrees->cOwnedExtents; ( iext-- >= 1 ) && ( pBTreeStats->pSpaceTrees->prgOwnedExtents[iext].pgnoLast > pespCtx->pgnoLast ); )
        {
            cpgNonShelved += CpgSpaceDumpGetOwnedNonShelved( pespCtx, &pBTreeStats->pSpaceTrees->prgOwnedExtents[iext] );
        }

        if ( cpgNonShelved > 0 )
        {
            ULONG objid = 0;
            pespCtx->fOwnedNonShelved = fTrue;

            switch ( eBTType )
            {
                case eBTreeTypeInternalDbRootSpace:
                    objid = 1;  // objidSystemRoot
                    break;

                case eBTreeTypeUserClusteredIndex:
                    objid = pespCtx->objidCurrTable;
                    break;

                case eBTreeTypeInternalLongValue:
                case eBTreeTypeUserSecondaryIndex:
                    objid = pespCtx->objidCurrIdxLv;
                    break;

                case eBTreeTypeInternalSpaceOE:
                case eBTreeTypeInternalSpaceAE:
                default:
                    Assert( fFalse );
                    break;
            }

            wprintf( L"\n\t\tERROR: objid(%I32u) has %I32u untracked pages beyond the end of file.\n"
                     L"\t\tPlease contact PSS.\n\n",
                     objid, cpgNonShelved );
        }
    }

    if ( FChildOfDbRoot( pBTreeStats ) )
    {
        //  Don't know we 100% trust this calculation, might not account for last extension
        //  operation against the DB properly.
        pespCtx->cpgDbRootUnAccountedFor -= pBTreeStats->pSpaceTrees->cpgOwned;
        pespCtx->cpgDbRootUnAccountedForEof -= CpgSpaceDumpGetExtPagesEof( pespCtx, pBTreeStats->pSpaceTrees->prgOwnedExtents, pBTreeStats->pSpaceTrees->cOwnedExtents );
    }

    //  If we walked onto a new table.
    //
    if ( eBTType == eBTreeTypeUserClusteredIndex )
    {
        if ( pespCtx->cpgCurrTableUnAccountedFor ||
                pespCtx->cpgCurrIdxLvUnAccountedFor )
        {
            //
            //  WHOA LEAKAGE!!!
            //
            if ( pespCtx->cpgCurrTableUnAccountedFor )
            {
                //  Print any overbookings or leaks for the last table ...
                //
                if ( pespCtx->cpgCurrTableUnAccountedFor < 0 )
                {
                    //  I think this is a corrupt database, but lets make the message tame-ish.
                    wprintf(L"\n\t\tERROR: objid(%d) has %d pages overbooked.\n"
                            L"\t\tPlease contact PSS.\n\n",
                                pespCtx->objidCurrTable, -pespCtx->cpgCurrTableUnAccountedFor );
                    //  Accumulate global counts for any overbookings or leaks
                    pespCtx->cpgTotalOverbooked += -pespCtx->cpgCurrTableUnAccountedFor;
                }
                else
                {
                    if ( pespCtx->fPrintSpaceLeaks )
                    {
                        wprintf(L"\t\tTable Objid(%d) leaked %d pages.\n",
                                    pespCtx->objidCurrTable, pespCtx->cpgCurrTableUnAccountedFor );
                    }
                    //  Accumulate global counts for any overbookings or leaks
                    pespCtx->cpgTotalLeaked += pespCtx->cpgCurrTableUnAccountedFor;
                    pespCtx->cpgTotalLeakedEof += pespCtx->cpgCurrTableUnAccountedForEof;
                }

                //  Reset current counters.
                //
                pespCtx->objidCurrTable = 0;
                pespCtx->cpgCurrTableUnAccountedFor = 0;
                pespCtx->cpgCurrTableUnAccountedForEof = 0;
            }

            if ( pespCtx->cpgCurrIdxLvUnAccountedFor )
            {
                //  Print any overbookings or leaks for the last index or LV ...
                //
                assert( pespCtx->eBTTypeCurr != eBTreeTypeInvalid );
                if ( pespCtx->cpgCurrIdxLvUnAccountedFor < 0 )
                {
                    //  I think this is a corrupt database, but lets make the message tame-ish.
                    wprintf(L"\n\t\tERROR: objid(%d) has %d pages overbooked.\n"
                            L"\t\tPlease contact PSS.\n\n",
                                pespCtx->objidCurrIdxLv, -pespCtx->cpgCurrIdxLvUnAccountedFor );
                    //  Accumulate global counts for any overbookings or leaks
                    pespCtx->cpgTotalOverbooked += -pespCtx->cpgCurrIdxLvUnAccountedFor;
                }
                else
                {
                    if ( pespCtx->fPrintSpaceLeaks )
                    {
                        wprintf(L"\t\t%ws Objid(%d) leaked %d pages.\n",
                                    ( pespCtx->eBTTypeCurr == eBTreeTypeInternalLongValue ) ? L"LV" : L"Index",
                                    pespCtx->objidCurrIdxLv, pespCtx->cpgCurrIdxLvUnAccountedFor );
                    }
                    //  Accumulate global counts for any overbookings or leaks
                    pespCtx->cpgTotalLeaked += pespCtx->cpgCurrIdxLvUnAccountedFor;
                    pespCtx->cpgTotalLeakedEof += pespCtx->cpgCurrIdxLvUnAccountedForEof;
                }


                //  Reset current counters.
                //
                pespCtx->objidCurrIdxLv = 0;
                pespCtx->eBTTypeCurr = eBTreeTypeInvalid;
                pespCtx->cpgCurrIdxLvUnAccountedFor = 0;
                pespCtx->cpgCurrIdxLvUnAccountedForEof = 0;
            }
        }

        pespCtx->objidCurrTable = pBTreeStats->pBasicCatalog->objidFDP;
        pespCtx->cpgCurrTableUnAccountedFor = pBTreeStats->pSpaceTrees->cpgOwned;
        pespCtx->cpgCurrTableUnAccountedForEof = CpgSpaceDumpGetExtPagesEof( pespCtx, pBTreeStats->pSpaceTrees->prgOwnedExtents, pBTreeStats->pSpaceTrees->cOwnedExtents );
        //  Remove space we've correctly tracked in AE tree.
        pespCtx->cpgCurrTableUnAccountedFor -= pBTreeStats->pSpaceTrees->cpgAvailable;
        pespCtx->cpgCurrTableUnAccountedForEof -= CpgSpaceDumpGetExtPagesEof( pespCtx, pBTreeStats->pSpaceTrees->prgAvailExtents, pBTreeStats->pSpaceTrees->cAvailExtents );
        //  Subtract primary B-Tree's internal + data pages.
        pespCtx->cpgCurrTableUnAccountedFor -= pBTreeStats->pParentOfLeaf->cpgInternal;
        pespCtx->cpgCurrTableUnAccountedFor -= pBTreeStats->pParentOfLeaf->cpgData;
        //  After this its up to subsequent calls to remove their owned cpg counts...
    }
    else
    {
        if ( FChildOfTable( pBTreeStats ) )
        {
            pespCtx->cpgCurrTableUnAccountedFor -= pBTreeStats->pSpaceTrees->cpgOwned;
            pespCtx->cpgCurrTableUnAccountedForEof -= CpgSpaceDumpGetExtPagesEof( pespCtx, pBTreeStats->pSpaceTrees->prgOwnedExtents, pBTreeStats->pSpaceTrees->cOwnedExtents );
        }
    }

    //  If we walked onto a new Idx or LV tree ...
    //
    if ( eBTType == eBTreeTypeInternalLongValue ||
            eBTType == eBTreeTypeUserSecondaryIndex )
    {
        if ( pespCtx->cpgCurrIdxLvUnAccountedFor )
        {
            //
            //  WHOA LEAKAGE!!!
            //

            //  Print any overbookings or leaks for the last index or LV ...
            //
            if ( pespCtx->cpgCurrIdxLvUnAccountedFor < 0 )
            {
                //  I think this is a corrupt database, but lets make the message tame-ish.
                wprintf(L"\n\t\tERROR: objid(%d) has %d pages overbooked.\n"
                        L"\t\tPlease contact PSS.\n\n",
                            pespCtx->objidCurrIdxLv, -pespCtx->cpgCurrIdxLvUnAccountedFor );
                //  Accumulate global counts for any overbookings or leaks
                pespCtx->cpgTotalOverbooked += -pespCtx->cpgCurrIdxLvUnAccountedFor;
            }
            else
            {
                if ( pespCtx->fPrintSpaceLeaks )
                {
                    wprintf(L"\t\t%ws Objid(%d) leaked %d pages.\n",
                                ( pespCtx->eBTTypeCurr == eBTreeTypeInternalLongValue ) ? L"LV" : L"Index",
                                pespCtx->objidCurrIdxLv, pespCtx->cpgCurrIdxLvUnAccountedFor );
                }
                //  Accumulate global counts for any overbookings or leaks
                pespCtx->cpgTotalLeaked += pespCtx->cpgCurrIdxLvUnAccountedFor;
                pespCtx->cpgTotalLeakedEof += pespCtx->cpgCurrIdxLvUnAccountedForEof;
            }


            //  Reset current counters.
            //
            pespCtx->objidCurrIdxLv = 0;
            pespCtx->eBTTypeCurr = eBTreeTypeInvalid;
            pespCtx->cpgCurrIdxLvUnAccountedFor = 0;
            pespCtx->cpgCurrIdxLvUnAccountedForEof = 0;
        }

        pespCtx->objidCurrIdxLv = pBTreeStats->pBasicCatalog->objidFDP;
        pespCtx->eBTTypeCurr = eBTType;
        pespCtx->cpgCurrIdxLvUnAccountedFor = pBTreeStats->pSpaceTrees->cpgOwned;
        pespCtx->cpgCurrIdxLvUnAccountedForEof = CpgSpaceDumpGetExtPagesEof( pespCtx, pBTreeStats->pSpaceTrees->prgOwnedExtents, pBTreeStats->pSpaceTrees->cOwnedExtents );
        //  Remove space we've correctly tracked in AE tree.
        pespCtx->cpgCurrIdxLvUnAccountedFor -= pBTreeStats->pSpaceTrees->cpgAvailable;
        pespCtx->cpgCurrIdxLvUnAccountedForEof -= CpgSpaceDumpGetExtPagesEof( pespCtx, pBTreeStats->pSpaceTrees->prgAvailExtents, pBTreeStats->pSpaceTrees->cAvailExtents );
        //  Subtract primary B-Tree's internal + data pages.
        pespCtx->cpgCurrIdxLvUnAccountedFor -= pBTreeStats->pParentOfLeaf->cpgInternal;
        pespCtx->cpgCurrIdxLvUnAccountedFor -= pBTreeStats->pParentOfLeaf->cpgData;
        //  After this its up to subsequent calls to remove thier owned cpg counts...
    }
    else
    {
        if ( FChildOfLV( pBTreeStats ) || FChildOfSecIdx( pBTreeStats ) )
        {
            pespCtx->cpgCurrIdxLvUnAccountedFor -= pBTreeStats->pSpaceTrees->cpgOwned;
            pespCtx->cpgCurrIdxLvUnAccountedForEof -= CpgSpaceDumpGetExtPagesEof( pespCtx, pBTreeStats->pSpaceTrees->prgOwnedExtents, pBTreeStats->pSpaceTrees->cOwnedExtents );
        }
    }
}

JET_ERR EseutilEvalBTreeData(
    __in const BTREE_STATS * const      pBTreeStats,
    __in JET_API_PTR                    pvContext )
{
    ESEUTIL_SPACE_DUMP_CTX * pespCtx = (ESEUTIL_SPACE_DUMP_CTX *)pvContext;
    JET_ERR err = JET_errSuccess;


    //
    //          Validate data coming in.
    //
    if ( pBTreeStats->cbStruct != sizeof(*pBTreeStats) )
    {
        //  ESE and ESEUTIL do not agree on size of struct, bad versions.
        wprintf(L"The ESE engine did not return expected stat data size.\n");
        return ErrERRCheck( JET_errInvalidParameter );
    }

    assert( pBTreeStats->pBasicCatalog );   // silly to ask w/o catalog.
    if ( NULL == pBTreeStats->pBasicCatalog )
    {
        //  ESE and ESEUTIL do not agree about presence of basic catalog info, bad versions?
        wprintf(L"The ESE engine did not return expected catalog data.\n");
        return ErrERRCheck( JET_errInvalidParameter );
    }

    //  For simplicity
    JET_BTREETYPE eBTType = pBTreeStats->pBasicCatalog->eType;
    switch( eBTType )
    {
        case eBTreeTypeInternalDbRootSpace:
        case eBTreeTypeUserClusteredIndex:
        case eBTreeTypeInternalSpaceOE:
        case eBTreeTypeInternalSpaceAE:
        case eBTreeTypeInternalLongValue:
        case eBTreeTypeUserSecondaryIndex:
            break;

        default:
            //  ESE and ESEUTIL do not agree on the kinds of BTrees, bad versions?
            wprintf(L"Unknown BTree enum: %d\n", eBTType );
            return ErrERRCheck( JET_errInvalidParameter );
    }


    //
    //          Deal with DB Root and Headers.
    //
    if( eBTreeTypeInternalDbRootSpace == eBTType )
    {
        //  This always comes first.

        pespCtx->pgnoDbEndUnusedBegin = 0;
        pespCtx->pgnoDbEndUnusedEnd = 0;

        //  Accumulate data.
        //
        if ( pBTreeStats->pSpaceTrees )
        {
            //  Database owned pages.
            pespCtx->cpgTotalShelved = pBTreeStats->pSpaceTrees->cpgShelved;
            pespCtx->cpgDbOwned = pBTreeStats->pSpaceTrees->cpgOwned;
            pespCtx->pgnoLast = pBTreeStats->pSpaceTrees->cpgOwned;
            pespCtx->cpgDbRootUnAccountedFor = pespCtx->cpgDbOwned + pespCtx->cpgTotalShelved;
            pespCtx->cpgDbRootUnAccountedForEof = pespCtx->cpgTotalShelved;

            //  Remove avail from unaccounted for.
            pespCtx->cpgDbRootUnAccountedFor -= pBTreeStats->pSpaceTrees->cpgAvailable;
            //  Accumulate total avail
            pespCtx->cpgTotalAvailExt += pBTreeStats->pSpaceTrees->cpgAvailable;
            // Do not update pgnoMin/pgnoMax with the root OE tree data here, so it is only "in-use" pgnos at end ...
            pespCtx->pgnoUsedMin = ulMax;
            pespCtx->pgnoUsedMax = 0;

            EseutilCalculateLastAvailableDbLogicalExtentRange( pBTreeStats->pSpaceTrees, &pespCtx->pgnoDbEndUnusedBegin, &pespCtx->pgnoDbEndUnusedEnd );

            Assert( ( pespCtx->pgnoDbEndUnusedEnd <= pespCtx->pgnoLast ) ||
                // unless there is no available space ... in which case it reports a virtual extent off the EOF of DB
                ( ( pespCtx->pgnoDbEndUnusedBegin == ( pespCtx->pgnoLast + 1 ) ) && ( pespCtx->pgnoDbEndUnusedEnd == lMax ) ) );

            //  If we've got pParentOfLeaf stats, we can truly track leaked, the DB root
            //  has a cpgData of 1 to account for the pgnoSystemRoot (pgno=1).
            if ( pBTreeStats->pParentOfLeaf )
            {
                assert( pBTreeStats->pParentOfLeaf->cpgData );
                pespCtx->cpgDbRootUnAccountedFor -= pBTreeStats->pParentOfLeaf->cpgData;
            }

            //  Collect shelved extents into a separate list.
            ULONG cShelvedExtents = 0;
            for ( ULONG iextAvail = 0; iextAvail < pBTreeStats->pSpaceTrees->cAvailExtents; iextAvail++ )
            {
                const BTREE_SPACE_EXTENT_INFO * const pextAvail = &pBTreeStats->pSpaceTrees->prgAvailExtents[iextAvail];
                if ( pextAvail->iPool == ulsppShelvedPool )
                {
                    cShelvedExtents++;
                }
            }
            if ( cShelvedExtents > 0 )
            {
                pespCtx->prgShelvedExtents = (BTREE_SPACE_EXTENT_INFO *)malloc( sizeof(BTREE_SPACE_EXTENT_INFO) * cShelvedExtents );
                Alloc( pespCtx->prgShelvedExtents );
                for ( ULONG iextAvail = 0; iextAvail < pBTreeStats->pSpaceTrees->cAvailExtents; iextAvail++ )
                {
                    const BTREE_SPACE_EXTENT_INFO * const pextAvail = &pBTreeStats->pSpaceTrees->prgAvailExtents[iextAvail];
                    if ( pextAvail->iPool == ulsppShelvedPool )
                    {
                        pespCtx->prgShelvedExtents[pespCtx->cShelvedExtents++] = *pextAvail;
                    }
                }
            }
        }

        //  Print out header and DbRoot data...
        //
        Call( EseutilSpaceDumpPrintHeadersAndDbRoot( pBTreeStats, pespCtx ) );

        //  Print out DbRoot space tree data (if appropriate)...
        //
        if ( ( pespCtx->grbit & JET_bitDBUtilSpaceInfoSpaceTrees ) && pespCtx->fPrintSpaceNodes )
        {
            Assert( pBTreeStats->pSpaceTrees );
            Assert( pBTreeStats->pSpaceTrees->prgOwnedExtents );
            Assert( pBTreeStats->pSpaceTrees->cOwnedExtents );
            Call( EseutilPrintSpaceTrees( pespCtx, L"DbRoot", L"", pBTreeStats->pSpaceTrees ) );
        }

        return JET_errSuccess;
    }

    //  Every other entry after the DbRoot should have a parent ...
    assert( pBTreeStats->pParent );


    //
    //          Accumulate interesting data.
    //

    //  B-Tree counts...
    //
    assert( eBTType < eBTreeTypeMax );
    pespCtx->cBTrees[eBTType]++;

    //  
    if ( pBTreeStats->pParent->pBasicCatalog->objidFDP == 1 /* DbRoot */ &&
            ( eBTType == eBTreeTypeInternalSpaceOE || eBTType == eBTreeTypeInternalSpaceAE ) )
    {
        // DB Root Space Trees...
    }

    if ( pBTreeStats->pSpaceTrees )
    {
        //  Currently owned.
        if ( pBTreeStats->pBasicCatalog->eType == eBTreeTypeUserClusteredIndex )
        {
            pespCtx->cpgCurrentTableOwned = pBTreeStats->pSpaceTrees->cpgOwned;
        }
        //  We accumulate avail pages across whole DB ...
        pespCtx->cpgTotalAvailExt += pBTreeStats->pSpaceTrees->cpgAvailable;
        const ULONG pgnoMinFound = PgnoMinOwned( pBTreeStats->pSpaceTrees );
        if ( pgnoMinFound != 0 )
        {
            pespCtx->pgnoUsedMin = min( pespCtx->pgnoUsedMin, pgnoMinFound );
        }
        pespCtx->pgnoUsedMax = max( pespCtx->pgnoUsedMax, PgnoMaxOwned( pBTreeStats->pSpaceTrees ) );

        if ( eBTType == eBTreeTypeInternalSpaceOE )
        {
            pespCtx->cpgTotalOEOverhead += pBTreeStats->pSpaceTrees->cpgOwned;
        }
        if ( eBTType == eBTreeTypeInternalSpaceAE )
        {
            pespCtx->cpgTotalAEOverhead += pBTreeStats->pSpaceTrees->cpgOwned;
        }
    }

    if ( pBTreeStats->pParentOfLeaf )
    {
        pespCtx->cpgTotalInternal += pBTreeStats->pParentOfLeaf->cpgInternal;
        pespCtx->cpgTotalData += pBTreeStats->pParentOfLeaf->cpgData;
    }

    //
    //  Attemping to track leaking or overbookings of space.
    //
    if ( pBTreeStats->pSpaceTrees && pBTreeStats->pParentOfLeaf )
    {
        EseutilTrackSpace( pespCtx, pBTreeStats, eBTType );
    }

    //
    //  Print data about this B-Tree
    //
    switch( eBTType )
    {
        case eBTreeTypeInternalSpaceOE:
        case eBTreeTypeInternalSpaceAE:
            //  Do not print space trees unless specifically requested to.
            if ( !pespCtx->fPrintSpaceTrees )
            {
                break;
            }
            //  else fall through so wr print the OE/AE tree data.
        case eBTreeTypeUserClusteredIndex:
        case eBTreeTypeInternalLongValue:
        case eBTreeTypeUserSecondaryIndex:
            //  Do not print small trees unless specifically requested to.
            if ( !pespCtx->fPrintSmallTrees && pBTreeStats->pSpaceTrees )
            {
                //  All note this about the only way in which ESE's heiarchical space helps us, for this
                //  case I don't have to worry about accidentally printing an index for a table that wasn't
                //  printed, b/c we know the table had a larger cpgOwned.
                double dblPct = ((double)pBTreeStats->pSpaceTrees->cpgOwned) / ((double)pespCtx->cpgDbOwned);
                if ( dblPct < g_dblSmallTableThreshold )
                {
                    //  Track that we didn't print, so we can tell the client how to get it.
                    pespCtx->fSquelchedSmallTrees = fTrue;
                    break;
                }
            }

            Call( EseutilPrintSpecifiedSpaceFields( pespCtx, pBTreeStats ) );

            if ( ( pespCtx->grbit & JET_bitDBUtilSpaceInfoSpaceTrees ) &&
                    pespCtx->fPrintSpaceNodes &&
                    pBTreeStats->pBasicCatalog->eType != eBTreeTypeInternalSpaceOE &&
                    pBTreeStats->pBasicCatalog->eType != eBTreeTypeInternalSpaceAE )
            {
                Assert( pBTreeStats->pSpaceTrees );
                Assert( pBTreeStats->pSpaceTrees->prgOwnedExtents );
                Assert( pBTreeStats->pSpaceTrees->cOwnedExtents );
                Call( EseutilPrintSpaceTrees( pespCtx, WszTableOfStats( pBTreeStats ), pBTreeStats->pBasicCatalog->rgName, pBTreeStats->pSpaceTrees ) );
            }
            break;

        default:
            assertSz( fFalse, "No" );
    }

HandleError:

    return err;
}

void EseutilTrackSpaceComplete(
    __inout ESEUTIL_SPACE_DUMP_CTX * const  pespCtx
    )
{
    // Potentially a fragile solution, but a concise solution ... we make up a 0 page table
    // B+ Tree space entry, so that EseutilTrackSpace() will complete it's calculations on
    // the last tree we were working on.

    //  First make up a parent BTREE_STATS that will convince them we're a tree...
    //
    BTREE_STATS_BASIC_CATALOG   FakeBasicCatalog = { sizeof(BTREE_STATS_BASIC_CATALOG),
                                                        eBTreeTypeUserClusteredIndex,   // this is our fake B+Tree / table
                                                        L"",
                                                        0x0, 0x0, NULL
                                                    };
    BTREE_STATS_SPACE_TREES     EmptySpaceTrees = { sizeof(BTREE_STATS_SPACE_TREES),
                                                        0, 0, fFalse, 0, 0, // cpgPrimary, cpgLastAlloc, fMultiExtent, pgnoOE, pgnoAE
                                                        0,                  // cpgOwned, the relevant field ... this gets subtracted from unaccounted for.
                                                        0, 0, 0             // cpgAvailable, cpgReserved, cpgShelved
                                                    };
    BTREE_STATS_PARENT_OF_LEAF  EmptyParentOfLeaf = { sizeof(BTREE_STATS_PARENT_OF_LEAF),
                                                        fTrue,              // fEmptyTree
                                                        0,                  // cpgInternal
                                                        0,                  // cpgData
                                                        0, NULL, 0, NULL        // phistoIOContiguousRuns, cForwardScans, pInternalPageStats.
                                                    };
    BTREE_STATS                 EmptyBTreeStats = { sizeof(BTREE_STATS),
                                                        0x0,                    // grbitData ...
                                                        NULL,               // do not need parent ...
                                                        &FakeBasicCatalog,  // space trees that are empty
                                                        &EmptySpaceTrees,   // more space data that is empty
                                                        &EmptyParentOfLeaf,
                                                        NULL
                                                    };

    //  Finalize the space calculations by claiming one more table.
    //
    EseutilTrackSpace( pespCtx, &EmptyBTreeStats, eBTreeTypeUserClusteredIndex );

}
    
JET_ERR ErrSpaceDumpCtxComplete(
    __inout void *          pvContext,
    __in JET_ERR            err )
{
    ESEUTIL_SPACE_DUMP_CTX * pespCtx = (ESEUTIL_SPACE_DUMP_CTX *)pvContext;

    if ( pespCtx == NULL || err != JET_errSuccess )
    {
        //  Haha, just kidding ... only cleanup ...
        goto HandleError;
    }

    //  Finally, do any last calculations.
    //

    if ( pespCtx->objidCurrTable ||
            pespCtx->objidCurrIdxLv ||
            pespCtx->fPrintSpaceLeaks )
    {
        assert( pespCtx->objidCurrTable );

        EseutilTrackSpaceComplete( pespCtx );
    }

    //  Accumulate or print any DB Root leakage or overbookings.

    if ( !pespCtx->fSelectOneTable )
    {
        if ( pespCtx->cpgDbRootUnAccountedFor < 0 )
        {
            //  I think this is a horrible inconsistency, but lets make the message tame-ish.
            wprintf(L"\n\t\tERROR: database has %d pages overbooked.\n"
                L"\t\tPlease contact PSS.\n\n",
                -pespCtx->cpgDbRootUnAccountedFor );
            pespCtx->cpgTotalOverbooked += -pespCtx->cpgDbRootUnAccountedFor;
        }
        else if ( pespCtx->cpgDbRootUnAccountedFor > 0 )
        {
            if ( pespCtx->fPrintSpaceLeaks )
            {
                wprintf(L"\t\tDatabase root leaked %d pages.\n",
                         pespCtx->cpgDbRootUnAccountedFor );
            }
            pespCtx->cpgTotalLeaked += pespCtx->cpgDbRootUnAccountedFor;
            pespCtx->cpgTotalLeakedEof += pespCtx->cpgDbRootUnAccountedForEof;
        }
    }

    if ( pespCtx->fSquelchedSmallTrees )
    {
        wprintf( L"Note: Some small tables/indices were not printed (use /v option to see those smaller than %.1f%% of the database).\n",
                        ( g_dblSmallTableThreshold * 100 ) );
    }

    ULONG cchTableLeft = EseutilSpaceDumpTableWidth( pespCtx );
    for( ULONG ieField = 0; cchTableLeft && ieField < pespCtx->cFields; ieField++ )
    {
        ULONG cchField = rgSpaceFields[pespCtx->rgeFields[ieField]].cchFieldSize + 1;
        cchField = min( cchTableLeft, cchField );
        wprintf( L"%ws", WszFillBuffer( L'-', cchField ) );
        cchTableLeft -= cchField;
    }
    wprintf( L"\n" );

    //  Print out the accumulation of selected data.
    //

    wprintf( L"\n" );

    //
    //  Print out # of B-Tree stats.
    //
    wprintf( L"    Enumerated %d Tables (",
                    pespCtx->cBTrees[eBTreeTypeUserClusteredIndex] );
    if ( pespCtx->grbit & JET_bitDBUtilSpaceInfoSpaceTrees )
    {
#ifdef DEBUG
        wprintf( L" %d OE Trees, %d AE Trees,",
                        pespCtx->cBTrees[eBTreeTypeInternalSpaceOE],
                        pespCtx->cBTrees[eBTreeTypeInternalSpaceAE] );
#else
        wprintf( L" %d Internal Trees,",
                        pespCtx->cBTrees[eBTreeTypeInternalSpaceOE] + pespCtx->cBTrees[eBTreeTypeInternalSpaceAE] );
#endif
    }
    wprintf( L" %d Long Value Trees, %d Secondary Indices )\n",
                    pespCtx->cBTrees[eBTreeTypeInternalLongValue],
                    pespCtx->cBTrees[eBTreeTypeUserSecondaryIndex] );
    assert( 0 == pespCtx->cBTrees[eBTreeTypeInvalid] );
    assert( 0 == pespCtx->cBTrees[eBTreeTypeInternalDbRootSpace] );

    wprintf( L"\n" );
    
    //  
    //  Print page purpose stats.
    //

    if ( pespCtx->grbit & JET_bitDBUtilSpaceInfoSpaceTrees )
    {

        //  Retail w/ only space trees:
        //    Total Pages 34234 ( 34 Used (x%), 234 Available (x%) ) + 4 Shelved
        //
        //  Retail w/ parent of leaf:
        //    Total Pages 34234 ( 34 Data (x%), 34 Overhead (x%), 1 Leaked (x%), 234 Available (x%) ) + 4 Shelved
        //
        //  Debug w/ parent of leaf:
        //    Total Pages 34234 ( 34 Data (x%), 29 Internal (x%), 4 OE (x%), 1 AE (x%), 1 Leaked (x%), 234 Available (x%) ) + 4 Shelved

        wprintf( L"    Pages %d (", pespCtx->cpgDbOwned );


        double dblPctAvail = ((double)pespCtx->cpgTotalAvailExt) / ((double)pespCtx->cpgDbOwned) * 100;

        ULONG pgnoShelvedMin = ulMax, pgnoShelvedMax = 0;

        if ( !(pespCtx->grbit & JET_bitDBUtilSpaceInfoParentOfLeaf) )
        {
            //  W/o parent of leaf, we can only tell used vs. available, and used is just !available ;)
            ULONG cpgUsed = pespCtx->cpgDbOwned - pespCtx->cpgTotalAvailExt;
            double dblPctUsed = ((double)cpgUsed) / ((double)pespCtx->cpgDbOwned) * 100;
            wprintf( L" %lu Used (%.1f%%), %lu Available (%.1f%%) ) + %lu Shelved\n",
                     cpgUsed, dblPctUsed,
                     pespCtx->cpgTotalAvailExt, dblPctAvail,
                     pespCtx->cpgTotalShelved );
        }
        else
        {

            double dblPctData = ((double)pespCtx->cpgTotalData) / ((double)pespCtx->cpgDbOwned) * 100;
            wprintf( L" %lu Data (%.1f%%),", pespCtx->cpgTotalData, dblPctData );

            #ifndef DEBUG
            //  Retail, but we have parent of leaf, print slightly more detail...
            //      We'll just call it all "overhead".
            ULONG cpgOverhead = pespCtx->cpgTotalOEOverhead + pespCtx->cpgTotalAEOverhead + pespCtx->cpgTotalInternal;
            double dblPctOverhead = ((double)cpgOverhead) / ((double)pespCtx->cpgDbOwned) * 100;
            wprintf( L" %lu Overhead (%.1f%%),", cpgOverhead, dblPctOverhead );
            #else
            //  Debug we'll give all the detail... 
            double dblPctInternal = ((double)pespCtx->cpgTotalInternal) / ((double)pespCtx->cpgDbOwned) * 100;
            double dblPctOE = ((double)pespCtx->cpgTotalOEOverhead) / ((double)pespCtx->cpgDbOwned) * 100;
            double dblPctAE = ((double)pespCtx->cpgTotalAEOverhead) / ((double)pespCtx->cpgDbOwned) * 100;
            wprintf( L" Internal %lu (%.1f%%), %lu OE (%.1f%%), %lu AE (%.1f%%),",
                     pespCtx->cpgTotalInternal, dblPctInternal,
                     pespCtx->cpgTotalOEOverhead, dblPctOE,
                     pespCtx->cpgTotalAEOverhead, dblPctAE );
            #endif

            //  Don't really feel we should draw attention to this, so it doesn't print by default
            //  Also we can't have used /tSpecificTable arg, otherwise the counts will be all wacked.
            if( pespCtx->fPrintSpaceLeaks && !pespCtx->fSelectOneTable )
            {
                Assert( pespCtx->cpgTotalLeaked >= pespCtx->cpgTotalLeakedEof );
                const INT cpgTotalLeakedNonEof = pespCtx->cpgTotalLeaked - pespCtx->cpgTotalLeakedEof;
                double dblPctLeaked = ((double)cpgTotalLeakedNonEof) / ((double)pespCtx->cpgDbOwned) * 100;
                wprintf( L" %lu Leaked (%.1f%%),",
                         cpgTotalLeakedNonEof, dblPctLeaked );
            }

            wprintf( L" %lu Available (%.1f%%) ) + %lu Shelved",
                     pespCtx->cpgTotalAvailExt, dblPctAvail,
                     pespCtx->cpgTotalShelved );

            Assert( pespCtx->cpgTotalLeakedEof <= (INT)pespCtx->cpgTotalShelved );
            if( pespCtx->cpgTotalShelved > 0 && pespCtx->fPrintSpaceLeaks && !pespCtx->fSelectOneTable )
            {
                double dblPctLeaked = ((double)pespCtx->cpgTotalLeakedEof) / ((double)pespCtx->cpgTotalShelved) * 100;
                wprintf( L" (%lu Leaked (%.1f%%))",
                         pespCtx->cpgTotalLeakedEof, dblPctLeaked );
            }

            wprintf( L"\n\n" );

            if ( pespCtx->cpgTotalOverbooked )
            {
                //  How should we position this?  DB is probably corrupt.
                wprintf( L" There are %lu overbooked pages.\n", pespCtx->cpgTotalOverbooked );
            }
        }
        if ( pespCtx->pgnoUsedMin == ulMax )
        {
            pespCtx->pgnoUsedMin = 0;
        }

        // Compute shelved min/max.
        for ( ULONG iextShelved = 0; iextShelved < pespCtx->cShelvedExtents; iextShelved++ )
        {
            const BTREE_SPACE_EXTENT_INFO * const pextShelved = &pespCtx->prgShelvedExtents[ iextShelved ];
            Assert( pextShelved->iPool == ulsppShelvedPool );
            Assert( pextShelved->cpgExtent > 0 );

            pgnoShelvedMin = min( pgnoShelvedMin, PgnoFirstFromExt( pextShelved ) );
            pgnoShelvedMax = max( pgnoShelvedMax, pextShelved->pgnoLast );
        }
        if ( pgnoShelvedMin == ulMax )
        {
            pgnoShelvedMin = 0;
        }

        wprintf( L"\n    Pgno Ranges (User min-max: %I32u - %I32u; Last: %I32u; DB End Contiguous Unused: %I32u - %I32u; Shelved min-max: %I32u - %I32u; Total: %I32u)\n",
                 pespCtx->pgnoUsedMin, pespCtx->pgnoUsedMax,
                 pespCtx->pgnoLast,
                 pespCtx->pgnoDbEndUnusedBegin, pespCtx->pgnoDbEndUnusedEnd,
                 pgnoShelvedMin, pgnoShelvedMax,
                 pespCtx->cpgDbOwned );

        //  If database is "pretty" big (100 MB) and over 1/5th empty, suggest defrag.
        //  Is 20% a good number?  Maybe 25%? 33%? 40%?
        if ( CbFromCpg( pespCtx->cpgDbOwned ) > 100*1024*1024 && dblPctAvail > 20.0 )
        {
            wprintf( L"\n    Note: This database is over 20%% empty, an offline defragmentation can be used to shrink the file.\n");
        }

        wprintf( L"\n" );
    }

HandleError:

    if ( pespCtx->fPrintSpaceLeaks && JET_errSuccess == err )
    {
        if ( pespCtx->cpgTotalOverbooked || pespCtx->fOwnedNonShelved )
        {
            // One might be tempted to chose JET_errSPAvailExtCorrupted or JET_errSPOwnExtCorrupted, but 
            //  the space APIs might actually return that if things are very bad, so rather than conflict
            //  we would like to punt back the more generic corruption error.
            err = ErrERRCheck( JET_errDatabaseCorrupted );
        }
        else if ( pespCtx->cpgTotalLeaked )
        {
            // This is less serious, it CAN happen on crash ...
            err = ErrERRCheck( JET_errDatabaseLeakInSpace );
        }
        // else err == JET_errSuccess
    }

    if ( pespCtx )
    {
        if ( pespCtx->prgShelvedExtents )
        {
            free( pespCtx->prgShelvedExtents );
        }

        if ( pespCtx->rgeFields )
        {
            free( pespCtx->rgeFields );
        }
        free( pespCtx );
    }

    return err;
}

//  -------------------------------------
//
//  Legacy Formatting.
//

//  This code is made to be lifted.  When we integrate to Windows we'll have to find all people
//  who call JetDBUtilities with the opDBUTILDumpSpace operation expecting it to print out a list
//  of space information, and just push this code to them.

// temporarily and shamelessly copied here from dbutil.cxx
//  ================================================================
LOCAL VOID DBUTLPrintfIntN( INT iValue, INT ichMax )
//  ================================================================
{
    CHAR    rgchT[17]; /* C-runtime max bytes == 17 */
    INT     ichT;

    _itoa_s( iValue, rgchT, _countof(rgchT), 10 );
    for ( ichT = 0; ichT < sizeof(rgchT) && rgchT[ichT] != '\0' ; ichT++ )
        ;
    if ( ichT > ichMax ) //lint !e661
    {
        for ( ichT = 0; ichT < ichMax; ichT++ )
            printf( "#" );
    }
    else
    {
        for ( ichT = ichMax - ichT; ichT > 0; ichT-- )
            printf( " " );
        for ( ichT = 0; rgchT[ichT] != '\0'; ichT++ )
            printf( "%c", rgchT[ichT] );
    }
    return;
}

JET_ERR ErrLegacySpaceDumpEvalBTreeData(
    __in const BTREE_STATS * const      pBTreeStats,
    __in JET_API_PTR                    pvContext )
{
    ULONG * pcpgAvailTotal = (ULONG*)pvContext;

    if ( pBTreeStats->cbStruct != sizeof(*pBTreeStats) )
    {
        //  ESE and ESEUTIL do not agree on size of struct, bad versions.
        wprintf(L"The ESE engine did not return expected stat data size.\n");
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( NULL == pBTreeStats->pBasicCatalog || NULL == pBTreeStats->pSpaceTrees )
    {
        //  ESE and ESEUTIL do not agree about presence of basic catalog info, bad versions?
        wprintf(L"The ESE engine did not return expected catalog data.\n");
        return ErrERRCheck( JET_errInvalidParameter );
    }

    //  Accumulate stats.
    //
    *pcpgAvailTotal += pBTreeStats->pSpaceTrees->cpgAvailable;


    //  Print out other B-Tree space info.
    //
    switch( pBTreeStats->pBasicCatalog->eType )
    {

        case eBTreeTypeInternalDbRootSpace:
            //  This always comes first.

            //  Print out header and DbRoot space info.
            //

            printf( "******************************** SPACE DUMP ***********************************\n" );
            printf( "Name                   Type   ObjidFDP    PgnoFDP  PriExt      Owned  Available\n" );
            printf( "===============================================================================\n" );
            printf( "%-23.23ws Db  ", pBTreeStats->pBasicCatalog->rgName );
            DBUTLPrintfIntN( pBTreeStats->pBasicCatalog->objidFDP, 10 );
            printf( " " );
            DBUTLPrintfIntN( pBTreeStats->pBasicCatalog->pgnoFDP, 10 );
            printf( " " );
            DBUTLPrintfIntN( pBTreeStats->pSpaceTrees->cpgPrimary, 5 );
            printf( "-%c ", 'm' );
            
            DBUTLPrintfIntN( pBTreeStats->pSpaceTrees->cpgOwned, 10 );
            printf( " " );
            DBUTLPrintfIntN( pBTreeStats->pSpaceTrees->cpgAvailable, 10 );
            printf( "\n" );
            printf( "\n" );
            break;
    
        case eBTreeTypeInternalSpaceOE:
        case eBTreeTypeInternalSpaceAE:
            //  Do nothing.
            break;

        case eBTreeTypeUserSecondaryIndex:
        case eBTreeTypeUserClusteredIndex:
        case eBTreeTypeInternalLongValue:

            switch ( pBTreeStats->pBasicCatalog->eType )
            {
                case eBTreeTypeUserClusteredIndex:
                    wprintf( L"%-23.23ws Tbl ", pBTreeStats->pBasicCatalog->rgName );
                    break;
                case eBTreeTypeInternalLongValue:
                    wprintf( L"  %-21.21ws LV  ", L"<Long Values>" );
                    break;
                case eBTreeTypeUserSecondaryIndex:
                    wprintf( L"  %-21.21ws Idx ", pBTreeStats->pBasicCatalog->rgName );
                    break;
            }
            DBUTLPrintfIntN( pBTreeStats->pBasicCatalog->objidFDP, 10 );
            printf( " " );
            DBUTLPrintfIntN( pBTreeStats->pBasicCatalog->pgnoFDP, 10 );
            printf( " " );
            DBUTLPrintfIntN( pBTreeStats->pSpaceTrees->cpgPrimary, 5 );
            printf( "-%c ", pBTreeStats->pSpaceTrees->fMultiExtent ? 'm' : 's' );
            
            DBUTLPrintfIntN( pBTreeStats->pSpaceTrees->cpgOwned, 10 );
            printf( " " );
            DBUTLPrintfIntN( pBTreeStats->pSpaceTrees->cpgAvailable, 10 );
            printf( "\n" );
            break;

        default:
            //  ESE and ESEUTIL do not agree on the kinds of BTrees, bad versions?
            wprintf(L"Eseutil did not recognize the B-Tree type, incompatible engine and utility versions?\n");
            return ErrERRCheck( JET_errInvalidParameter );

    }

    return JET_errSuccess;
}

//  This basically converts an older JetDBUtilities() call into something that will perform
//  nearly exactly like the older Exch2k7/Win2k8 code.
JET_ERR JetLegacyDBSpaceDump( JET_DBUTIL_W * pdbutilW )
{
    ULONG cpgAvailable = 0;
    JET_ERR err = JET_errSuccess;

    if ( pdbutilW->op != opDBUTILDumpSpace ||
        pdbutilW->pfnCallback != NULL )
    {
        printf("Does not look like a proper util argument\n");
        return ErrERRCheck( JET_errInvalidParameter );
    }
    //  else this looks like a space dump dbutil operations...

    //  convert the dbutil op to use the above externalized callback code that performs the
    //  same as legacy Exch2k7/Win2k8 code.
    pdbutilW->grbitOptions |= ( JET_bitDBUtilSpaceInfoBasicCatalog | JET_bitDBUtilSpaceInfoSpaceTrees );
    pdbutilW->pfnCallback = ErrLegacySpaceDumpEvalBTreeData;
    pdbutilW->pvCallback = &cpgAvailable;

    err = JetDBUtilitiesW( pdbutilW );

    printf( "-------------------------------------------------------------------------------\n" );
    printf( "                                                                     " );
    DBUTLPrintfIntN( cpgAvailable, 10 );
    printf( "\n" );

    return err;
}

#if DEBUG
// Calls EseutilCalculateLastAvailableDbLogicalExtentRange and verifies that the returned
// result is correct.
void EseutilITestDbspacedump(
    _In_ const ULONG pgnoExpectedStart,
    _In_ const ULONG pgnoExpectedEnd,
    _In_ const ULONG cOE,
    _In_reads_( cOE ) BTREE_SPACE_EXTENT_INFO* rgOE,
    _In_ const ULONG cAE,
    _In_reads_( cAE ) BTREE_SPACE_EXTENT_INFO* rgAE
    )
{
        BTREE_STATS_SPACE_TREES bspt = { 0 };
        bspt.cOwnedExtents = cOE;
        bspt.prgOwnedExtents = rgOE;
        bspt.cAvailExtents = cAE;
        bspt.prgAvailExtents = rgAE;

        ULONG pgnoActualStart = 1;
        ULONG pgnoActualEnd = 1;

        EseutilCalculateLastAvailableDbLogicalExtentRange( &bspt, &pgnoActualStart, &pgnoActualEnd );

        AssertSz( pgnoExpectedStart == pgnoActualStart, "Failed for Start Range. Expected (%u) != Actual (%u).", pgnoExpectedStart, pgnoActualStart );
        AssertSz( pgnoExpectedEnd == pgnoActualEnd, "Failed for End Range. Expected (%u) != Actual (%u).", pgnoExpectedEnd, pgnoActualEnd );
}
#endif


// Set up some fake AE/OE arrays, and call EseutilITestDbspacedump.
void EseutilDbspacedumpUnitTest()
{
#if DEBUG
    BTREE_SPACE_EXTENT_INFO rgOETwoExtent[] =
    {
        { ulsppAvailExtLegacyGeneralPool, 8, 8, 1 },
        { ulsppAvailExtLegacyGeneralPool, 16, 8, 1 },
    };

    // no free extent space at EOF
    BTREE_SPACE_EXTENT_INFO rgAETwoExtentUsedEnd[] =
    {
        { ulsppAvailExtLegacyGeneralPool, 6, 4, 1 },
        { ulsppAvailExtLegacyGeneralPool, 15, 5, 1 },
    };
    BTREE_SPACE_EXTENT_INFO rgAETwoExtentUsedEndWithShelved[] =
    {
        { ulsppAvailExtLegacyGeneralPool, 6, 4, 1 },
        { ulsppAvailExtLegacyGeneralPool, 15, 5, 1 },
        { ulsppShelvedPool, 16, 1, 1 },
        { ulsppShelvedPool, 18, 1, 1 },
    };
    EseutilITestDbspacedump( 17, lMax, _countof( rgOETwoExtent ), rgOETwoExtent, _countof( rgAETwoExtentUsedEnd ), rgAETwoExtentUsedEnd );
    EseutilITestDbspacedump( 17, lMax, _countof( rgOETwoExtent ), rgOETwoExtent, _countof( rgAETwoExtentUsedEndWithShelved ), rgAETwoExtentUsedEndWithShelved );

    // 1 free extent at EOF.
    BTREE_SPACE_EXTENT_INFO rgAETwoExtentLastExtentPartiallyFree[] =
    {
        { ulsppAvailExtLegacyGeneralPool, 8, 4, 1 },
        { ulsppAvailExtLegacyGeneralPool, 16, 3, 1 },
    };
    BTREE_SPACE_EXTENT_INFO rgAETwoExtentLastExtentPartiallyFreeWithShelved[] =
    {
        { ulsppAvailExtLegacyGeneralPool, 8, 4, 1 },
        { ulsppAvailExtLegacyGeneralPool, 16, 3, 1 },
        { ulsppShelvedPool, 18, 1, 1 },
        { ulsppShelvedPool, 19, 1, 1 },
    };
    EseutilITestDbspacedump( 14, 16, _countof( rgOETwoExtent ), rgOETwoExtent, _countof( rgAETwoExtentLastExtentPartiallyFree ), rgAETwoExtentLastExtentPartiallyFree );
    EseutilITestDbspacedump( 14, 16, _countof( rgOETwoExtent ), rgOETwoExtent, _countof( rgAETwoExtentLastExtentPartiallyFreeWithShelved ), rgAETwoExtentLastExtentPartiallyFreeWithShelved );

    BTREE_SPACE_EXTENT_INFO rgOESingle[] =
    {
        { ulsppAvailExtLegacyGeneralPool, 16, 16, 1 },
    };

    // 1 free extent at EOF. 1 OE total.
    BTREE_SPACE_EXTENT_INFO rgAESingle[] =
    {
        { ulsppAvailExtLegacyGeneralPool, 16, 3, 1 },
    };
    BTREE_SPACE_EXTENT_INFO rgAESingleWithShelved[] =
    {
        { ulsppAvailExtLegacyGeneralPool, 16, 3, 1 },
        { ulsppShelvedPool, 18, 1, 1 },
        { ulsppShelvedPool, 19, 1, 1 },
    };
    EseutilITestDbspacedump( 14, 16, _countof( rgOESingle ), rgOESingle, _countof( rgAESingle ), rgAESingle );
    EseutilITestDbspacedump( 14, 16, _countof( rgOESingle ), rgOESingle, _countof( rgAESingleWithShelved ), rgAESingleWithShelved );

    // 1 free extent at EOF, multiple OE.
    EseutilITestDbspacedump( 14, 16, _countof( rgOETwoExtent ), rgOETwoExtent, _countof( rgAESingle ), rgAESingle );
    EseutilITestDbspacedump( 14, 16, _countof( rgOETwoExtent ), rgOETwoExtent, _countof( rgAESingleWithShelved ), rgAESingleWithShelved );

    // 1.5 free extents at EOF.
    BTREE_SPACE_EXTENT_INFO rgAETwoExtentLastExtentFree[] =
    {
        { ulsppAvailExtLegacyGeneralPool, 8, 3, 1 },
        { ulsppAvailExtLegacyGeneralPool, 16, 8, 1 },
    };
    BTREE_SPACE_EXTENT_INFO rgAETwoExtentLastExtentFreeWithShelved[] =
    {
        { ulsppAvailExtLegacyGeneralPool, 8, 3, 1 },
        { ulsppAvailExtLegacyGeneralPool, 16, 8, 1 },
        { ulsppShelvedPool, 22, 1, 1 },
        { ulsppShelvedPool, 33, 1, 1 },
    };
    EseutilITestDbspacedump( 6, 16, _countof( rgOETwoExtent ), rgOETwoExtent, _countof( rgAETwoExtentLastExtentFree ), rgAETwoExtentLastExtentFree );
    EseutilITestDbspacedump( 6, 16, _countof( rgOETwoExtent ), rgOETwoExtent, _countof( rgAETwoExtentLastExtentFreeWithShelved ), rgAETwoExtentLastExtentFreeWithShelved );

    // 2.5 free extents at EOF.
    BTREE_SPACE_EXTENT_INFO rgAEThreeExtentLastTwoExtentsFree[] =
    {
        { ulsppAvailExtLegacyGeneralPool, 8, 3, 1 },
        { ulsppAvailExtLegacyGeneralPool, 12, 4, 1 },
        { ulsppAvailExtLegacyGeneralPool, 16, 4, 1 },
    };
    BTREE_SPACE_EXTENT_INFO rgAEThreeExtentLastTwoExtentsFreeWithShelved[] =
    {
        { ulsppAvailExtLegacyGeneralPool, 8, 3, 1 },
        { ulsppAvailExtLegacyGeneralPool, 12, 4, 1 },
        { ulsppAvailExtLegacyGeneralPool, 16, 4, 1 },
        { ulsppShelvedPool, 22, 1, 1 },
        { ulsppShelvedPool, 33, 1, 1 },
    };
    EseutilITestDbspacedump( 6, 16, _countof( rgOETwoExtent ), rgOETwoExtent, _countof( rgAEThreeExtentLastTwoExtentsFree ), rgAEThreeExtentLastTwoExtentsFree );
    EseutilITestDbspacedump( 6, 16, _countof( rgOETwoExtent ), rgOETwoExtent, _countof( rgAEThreeExtentLastTwoExtentsFreeWithShelved ), rgAEThreeExtentLastTwoExtentsFreeWithShelved );

    // No free extents.
    BTREE_SPACE_EXTENT_INFO rgAEOnlyShelved[] =
    {
        { ulsppShelvedPool, 22, 1, 1 },
        { ulsppShelvedPool, 33, 1, 1 },
    };
    EseutilITestDbspacedump( 17, lMax, _countof( rgOETwoExtent ), rgOETwoExtent, 0, NULL );
    EseutilITestDbspacedump( 17, lMax, _countof( rgOETwoExtent ), rgOETwoExtent, _countof( rgAEOnlyShelved ), rgAEOnlyShelved );

#endif // DEBUG
}

