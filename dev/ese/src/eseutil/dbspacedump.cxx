// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#include "_edbutil.hxx"
#include "_dbspacedump.hxx"

#define SPACE_ONLY_DIAGNOSTIC_CONSTANTS 1
#include "_space.hxx"

#include "stat.hxx"

void EseutilDbspacedumpUnitTest();


#define ulsppAvailExtLegacyGeneralPool ((ULONG)spp::AvailExtLegacyGeneralPool)
#define ulsppPrimaryExt                ((ULONG)spp::PrimaryExt)
#define ulsppContinuousPool            ((ULONG)spp::ContinuousPool)
#define ulsppShelvedPool               ((ULONG)spp::ShelvedPool)
#define ulsppOwnedTreeAvail            ((ULONG)spp::OwnedTreeAvail) 
#define ulsppAvailTreeAvail            ((ULONG)spp::AvailTreeAvail)


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
        return wszPatternBuffer;
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
            break;
    }
    assertSz( fFalse, "Unknown pattern" );
    return L" ";
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
                return L"DbRoot";
            }
            else if ( pBTreeStats->pParent->pParent &&
                    pBTreeStats->pParent->pParent->pBasicCatalog->eType != eBTreeTypeInternalDbRootSpace )
            {
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
    Assert( pext->cpgExtent < (ULONG)lMax );
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

    Assert( !FDBSPUTLContains( pextContaining, PgnoFirstFromExt( pextPossibleSubExtent ) ) );
    Assert( !FDBSPUTLContains( pextContaining, pextPossibleSubExtent->pgnoLast ) );

    return fFalse;
}



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

    eSPFieldName,

    eSPFieldMax

} E_SP_FIELD;



typedef struct
{

    JET_GRBIT       grbit;

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


    ULONG           cBTrees[eBTreeTypeMax];

    ULONG           cpgDbOwned;
    ULONG           cpgTotalAvailExt;
    ULONG           cpgTotalShelved;
    ULONG           pgnoDbEndUnusedBegin;
    ULONG           pgnoDbEndUnusedEnd;
    ULONG           pgnoUsedMin;
    ULONG           pgnoUsedMax;
    ULONG           pgnoLast;
    ULONG           cpgTotalOEOverhead;
    ULONG           cpgTotalAEOverhead;
    ULONG           cpgTotalInternal;
    ULONG           cpgTotalData;

    ULONG           cpgCurrentTableOwned;
    

    INT             cpgDbRootUnAccountedFor;
    INT             cpgDbRootUnAccountedForEof;
    ULONG           objidCurrTable;
    INT             cpgCurrTableUnAccountedFor;
    INT             cpgCurrTableUnAccountedForEof;
    ULONG           objidCurrIdxLv;
    JET_BTREETYPE   eBTTypeCurr;
    INT             cpgCurrIdxLvUnAccountedFor;
    INT             cpgCurrIdxLvUnAccountedForEof;
    INT             cpgTotalLeaked;
    INT             cpgTotalLeakedEof;
    INT             cpgTotalOverbooked;

    unsigned long                   cShelvedExtents;
    _Field_size_opt_(cShelvedExtents) BTREE_SPACE_EXTENT_INFO *       prgShelvedExtents;
    BOOL            fOwnedNonShelved;
} ESEUTIL_SPACE_DUMP_CTX;



double g_dblSmallTableThreshold = .005;

typedef struct
{
    E_SP_FIELD  eField;
    ULONG       cchFieldSize;
    WCHAR *     wszField;
    WCHAR *     wszSubFields;
    JET_GRBIT   grbitRequired;
} ESEUTIL_SPACE_FIELDS;

WCHAR * szMAMH = L"   Count,  Min,  Ave,  Max,       Total";
#define cchMAMH     39

WCHAR * szCAH = L"   Count,  Ave";
#define cchCAH      14

WCHAR szRunsH []= L"    1-Page,   2-Pages,   3-Pages,   4-Pages,   8-Pages,  16-Pages,  32-Pages,  64-Pages,  96-Pages, 128-Pages,  Over-128";
#define cchRunsH 120
SAMPLE rgIORunHistoDivisions[] =
{
    1, 2, 3, 4, 8, 16, 32, 64, 96, 128, (SAMPLE)-1 
};

WCHAR szLVB[] = L"       4KB,       8KB,      16KB,      32KB,      64KB,     128KB,     256KB,     512KB,       1MB,       2MB,  Over-2MB";
#define cchLVB 120
SAMPLE rgLVBytesDivisions[] =
{
    4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, (SAMPLE)-1 
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
    1, 2, 3, 4, 5, 6, 7, 8, 16, 32, (SAMPLE)-1 
};

WCHAR szLVES[] = L"         0,         1,         2,         3,         4,         5,         6,         7,         8,        16,   Over-16";
#define cchLVES 120
SAMPLE rgLVExtraSeeksDivisions[] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 16, (SAMPLE)-1 
};

WCHAR szLVEB[] = L"       0KB,       4KB,       8KB,      16KB,      32KB,      64KB,     128KB,     256KB,     512KB,       1MB,  Over-1MB";
#define cchLVEB 120
SAMPLE rgLVExtraBytesDivisions[] =
{
    0, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, (SAMPLE)-1 
};

ESEUTIL_SPACE_FIELDS rgSpaceFields [] =
{

    { eSPFieldNone,                     3,          L"N/A",                     NULL,   0x0                         },

    { eSPFieldNameFull,                 64 + 4,     L"FullName",                NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },
    { eSPFieldOwningTableName,          64 + 4,     L"OwningTableName",         NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },
    { eSPFieldType,                     4,          L"Type",                    NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },
    { eSPFieldObjid,                    10,         L"ObjidFDP",                NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },
    { eSPFieldPgnoFDP,                  10,         L"PgnoFDP",                 NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },

    { eSPFieldPriExt,                   7,          L"PriExt",                  NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldPriExtType,               10,         L"PriExtType",              NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldPriExtCpg,                9,          L"PriExtCpg",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldPgnoOE,                   10,         L"PgnoOE",                  NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldPgnoAE,                   10,         L"PgnoAE",                  NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldOwnedPgnoMin,             10,         L"OwnPgnoMin",              NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldOwnedPgnoMax,             10,         L"OwnPgnoMax",              NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldOwnedExts,                10,         L"OwnedExts",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldOwnedCPG,                 10,         L"Owned",                   NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldOwnedMB,                  8 + 4,      L"Owned(MB)",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldOwnedPctOfDb,             9,          L"O%OfDb",                  NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldOwnedPctOfTable,          9,          L"O%OfTable",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldDataCPG,                  10,         L"Data",                    NULL,   JET_bitDBUtilSpaceInfoParentOfLeaf  },
    { eSPFieldDataMB,                   8 + 4,      L"Data(MB)",                NULL,   JET_bitDBUtilSpaceInfoParentOfLeaf  },
    { eSPFieldDataPctOfDb,              9,          L"D%OfDb",                  NULL,   JET_bitDBUtilSpaceInfoParentOfLeaf  },

    { eSPFieldAvailExts,                10,         L"AvailExts",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldAvailCPG,                 10,         L"Available",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldAvailMB,                  8 + 4,      L"Avail(MB)",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldAvailPctOfTable,          9,          L"Avail%Tbl",               NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldSpaceTreeReservedCPG,     10,         L"SpcReserve",              NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldAutoInc,                  10,         L"AutoInc",                 NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
    { eSPFieldReservedCPG,              10,         L"Reserved",                NULL,   JET_bitDBUtilSpaceInfoSpaceTrees    },
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
    { eSPFieldIntKeyCompMAM,            cchMAMH,    L"Int:KeyComp",             szMAMH, JET_bitDBUtilSpaceInfoParentOfLeaf  },

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

    { eSPFieldName,                     23,         L"Name",                    NULL,   JET_bitDBUtilSpaceInfoBasicCatalog  },

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
                if ( pBTreeStats->pSpaceTrees->prgAvailExtents[iext].cpgExtent != 0 )
                {
                    Call( ErrFromStatErr( pComputedStats->histoContiguousAEs.ErrAddSample( pBTreeStats->pSpaceTrees->prgAvailExtents[iext].cpgExtent ) ) );
                }
            }
            else if ( pBTreeStats->pSpaceTrees->prgAvailExtents[iext].iPool == ulsppShelvedPool )
            {
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

            const BTREE_STATS * pIndenting = pBTStats;
            ULONG cchFieldAdjust = 0;
            while( pIndenting->pParent )
            {
                pIndenting = pIndenting->pParent;
                wprintf( L"  " );
                cchFieldAdjust += 2;
            }
            if ( eField == eSPFieldNameFull )
            {
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
            Assert( cpgFirstExt );
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


JET_ERR ErrSpaceDumpCtxInit( __out void ** ppvContext )
{
    JET_ERR err = JET_errSuccess;

    EseutilDbspacedumpUnitTest();

    if ( NULL == ppvContext )
    {
        assertSz( fFalse, "Huh?" );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    ESEUTIL_SPACE_DUMP_CTX * pespCtx = (ESEUTIL_SPACE_DUMP_CTX*)malloc( sizeof(ESEUTIL_SPACE_DUMP_CTX) );
    if ( pespCtx == NULL )
    {
        wprintf(L"Failed to allocate %d bytes, failing command.\n", (INT)sizeof(ESEUTIL_SPACE_DUMP_CTX) );
        return ErrERRCheck( JET_errOutOfMemory );
    }
    memset( pespCtx, 0, sizeof(ESEUTIL_SPACE_DUMP_CTX) );

    pespCtx->fPrintSpaceTrees = fFalse;
    pespCtx->fPrintSpaceNodes = fFalse;

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
        Call( ErrSpaceDumpCtxSetFields( pvContext, L"#default" ) );
    }

    *pgrbit = pespCtx->grbit;

HandleError:

    return err;
}


WCHAR wszDefaultFieldsCmd [] = L"#default";
WCHAR wszDefaultFields [] = L"Name,Type,Owned(MB),O%OfDb,O%OfTable,Avail(MB),Avail%Tbl,AutoInc";

WCHAR wszLegacyFieldsCmd [] = L"#legacy";
WCHAR wszLegacyFields [] = L"name,type,objidfdp,pgnofdp,priext,owned,available";

WCHAR wszLeakedDataCmd [] = L"#leaked";
WCHAR wszLeakedDataFields [] = L"Name,Type,ObjidFDP,PriExt,PgnoFDP,Owned,Available,Internal,Data";

WCHAR wszSpaceHintsCmd [] = L"#spacehints";
WCHAR wszSpaceHintsFields [] = L"Name,Type,SH:IDensity,SH:ISize(KB),SH:grbit,SH:MDensity,SH:Growth,SH:MinExt(KB),SH:MaxExt(KB)";

WCHAR wszLVsCmd[] = L"#lvs";
WCHAR wszLVsFields[] = L"FullName,Type,LV:Size,LV:Size(histo),LV:Comp,LV:Comp(histo),LV:Ratio(histo),LV:Seeks,LV:Seeks(histo),LV:ExtraSeeks,LV:ExtraSeeks(histo),LV:Bytes,LV:Bytes(histo),LV:ExtraBytes,LV:ExtraBytes(histo),Data:FreeBytes,Data:Nodes,Data:KeySizes,Data:DataSizes,Data:Unreclaim,VersndNode,cLVRefs,cCorrLVs,cSepRtChk,lidMax,LV:ChunkSize,OwnExt,GenAvailExt";

WCHAR wszAllFieldsCmd [] = L"#all";

    
void PrintSpaceDumpHelp(
    __in const ULONG                    cchLineWidth,
    __in const WCHAR                    wchNewLine,
    __in_z const WCHAR *                wszTab1,
    __in_z const WCHAR *                wszTab2 )
{

    wprintf( L"%c", wchNewLine );
    wprintf( L"%wsSPACE USAGE OPTIONS:%c", wszTab1, wchNewLine );

#if DEBUG
    wprintf( L"%ws%ws     /d[<n>]       - Prints more detailed information on trees.%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws                      /d1 - Prints space trees (default).%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws                      /d2 - Prints space nodes.%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws     /l            - Prints leak information.%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%c", wchNewLine );
#endif

    wprintf( L"%ws%ws     /f<field[,field]>%c", wszTab1, wszTab2, wchNewLine );
    wprintf( L"%ws%ws     - Space info fields to print. Sets of fields%c", wszTab1, wszTab2, wchNewLine );

    wprintf( L"%ws%ws       Sets of fields:%c", wszTab1, wszTab2, wchNewLine );
    assert( 29 == wcslen(wszTab1) + wcslen(wszTab2) );
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
        if ( ( cchTab == cchUsed ) ||
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
    for( ULONG eField = 0; eField < sizeof(rgSpaceFields)/sizeof(rgSpaceFields[0]); eField++ )
    {
        assert( eField == (ULONG)rgSpaceFields[eField].eField );
        assert( (ULONG)wcslen(rgSpaceFields[eField].wszField) <= rgSpaceFields[eField].cchFieldSize );
    }
#endif


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
        ULONG cb = sizeof(WCHAR);
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

    ULONG cb = sizeof(E_SP_FIELD);
    WCHAR * pchSep = (WCHAR*)wszFields;
    while ( pchSep = wcschr( (WCHAR*)pchSep, L',' ) )
    {
        pchSep++;
        cb += sizeof(E_SP_FIELD);
    }

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
            *pchSep = L'\0';
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
        pespCtx->grbit |= JET_bitDBUtilSpaceInfoParentOfLeaf;
        pespCtx->fPrintSpaceLeaks = fTrue;
    }

    if ( fSPDumpSelectOneTable & fSPDumpOpts )
    {
        pespCtx->fSelectOneTable = fTrue;
    }

    if( wszSeparator )
    {
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


ULONG  EseutilSpaceDumpTableWidth(
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

    BTREE_POST_COMPUTED_SPACE_STATS statsComputed;
    Call( ErrCalculatePostComputedSpaceStats( pDbRoot, &statsComputed ) );

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

    BTREE_POST_COMPUTED_SPACE_STATS statsComputed;
    Call( ErrCalculatePostComputedSpaceStats( pBTreeStats, &statsComputed ) );

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

    for ( ULONG iextOwned = 0; iextOwned < pSpaceTrees->cOwnedExtents; iextOwned++ )
    {
        const BTREE_SPACE_EXTENT_INFO * const pextOwned = &pSpaceTrees->prgOwnedExtents[ iextOwned ];
        const ULONG cpgNonShelved = CpgSpaceDumpGetOwnedNonShelved( pespCtx, pextOwned );

        Assert( ( pextOwned->pgnoLast <= pespCtx->pgnoLast ) || ( PgnoFirstFromExt( pextOwned ) > pespCtx->pgnoLast ) );
        wprintf( L"        space[%ws\\%ws]  OE[  %5I32u]: %6I32u - %6I32u (%4I32u, ",
                    wszTable, wszTree,
                    pextOwned->pgnoSpaceNode,
                    PgnoFirstFromExt( pextOwned ),
                    pextOwned->pgnoLast,
                    pextOwned->cpgExtent );

        if ( pextOwned->pgnoLast > pespCtx->pgnoLast )
        {
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

        ULONG cextAvailInOwned = 0;
        ULONG cpgAvailInOwned = 0;
        ULONG cextContigMarkersInOwned = 0;
        for ( ULONG iextAvail = 0; iextAvail < pSpaceTrees->cAvailExtents; iextAvail++ )
        {
            const BTREE_SPACE_EXTENT_INFO * const pextAvail = &pSpaceTrees->prgAvailExtents[ iextAvail ];

            Expected( ( pextAvail->iPool >= ulsppAvailExtLegacyGeneralPool && pextAvail->iPool <= ulsppShelvedPool ) ||
                      pextAvail->iPool == ulsppPrimaryExt ||
                      pextAvail->iPool == ulsppOwnedTreeAvail ||
                      pextAvail->iPool == ulsppAvailTreeAvail );

            if ( pextAvail->cpgExtent != 0  && FDBSPUTLContains( pextOwned, pextAvail ) )
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

        for ( ULONG iextAvail = 0; iextAvail < pSpaceTrees->cAvailExtents; iextAvail++ )
        {
            const BTREE_SPACE_EXTENT_INFO * const pextAvail = &pSpaceTrees->prgAvailExtents[iextAvail];

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

                Assert( rgfAeHandled[ iextAvail ] == false );
                rgfAeHandled[ iextAvail ] = true;
            }
        }
        wprintf( L"\n" );
    }

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
                iextAvailFirst = iextAvailScan;
                iextAvailLast = iextAvailScan;
                iPool = pextAvailScan->iPool;
            }
            else if ( ( ( pextAvailScan->pgnoLast - pextAvailScan->cpgExtent ) == pSpaceTrees->prgAvailExtents[iextAvailLast].pgnoLast ) &&
                      ( iPool == pSpaceTrees->prgAvailExtents[iextAvailLast].iPool ) )
            {
                iextAvailLast = iextAvailScan;
            }
            else
            {
                fEndOfRun = fTrue;
            }
        }
        else if ( iPool != ulMax )
        {
            fEndOfRun = fTrue;
        }

        Assert( ( ( iPool == ulMax ) && ( iextAvailFirst == ulMax ) && ( iextAvailLast == ulMax ) ) ||
                ( ( iPool != ulMax ) && ( iextAvailFirst != ulMax ) && ( iextAvailLast != ulMax ) && ( iextAvailFirst <= iextAvailLast ) ) );

        const BOOL fLastExtent = ( iextAvailScan == ( pSpaceTrees->cAvailExtents - 1 ) );

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

        if ( fEndOfRun && !fProcessed )
        {
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
    if ( pBTreeStats->pParent->pBasicCatalog->objidFDP != 1  )
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

    AssertSz( PgnoFirstFromExt( &( rgOE[ 0 ] ) ) == 1, __FUNCTION__ " should be called on the root OE/AE only." );

    *ppgnoDbEndUnusedBegin = lMax;
    *ppgnoDbEndUnusedEnd = lMax;

    const ULONG pgnoOELast = rgOE[ cOE - 1 ].pgnoLast;

    size_t cAvailAE = cAE;
    while ( ( cAvailAE > 0 ) && ( rgAE[ cAvailAE - 1 ].iPool == ulsppShelvedPool ) )
    {
        cAvailAE--;
    }

    if ( cAvailAE == 0 )
    {
        *ppgnoDbEndUnusedBegin = pgnoOELast + 1;
    }
    else
    {
        const ULONG pgnoAELast = rgAE[ cAvailAE - 1 ].pgnoLast;

        if ( pgnoOELast == pgnoAELast && rgAE[ cAE - 1 ].cpgExtent != 0 )
        {
            *ppgnoDbEndUnusedBegin = PgnoFirstFromExt( &rgAE[ cAvailAE - 1 ] );
            *ppgnoDbEndUnusedEnd = pgnoAELast;

            for ( INT iCurrentAE = (INT) cAvailAE - 1; iCurrentAE > 0; --iCurrentAE )
            {
                Assert( ( rgAE[ iCurrentAE ].iPool == ulsppAvailExtLegacyGeneralPool ) ||
                        ( rgAE[ iCurrentAE ].iPool == ulsppOwnedTreeAvail ) ||
                        ( rgAE[ iCurrentAE ].iPool == ulsppAvailTreeAvail ) );

                Assert( rgAE[ iCurrentAE - 1 ].pgnoLast < rgAE[ iCurrentAE ].pgnoLast );

                if ( rgAE[ iCurrentAE - 1 ].pgnoLast == PgnoFirstFromExt( &rgAE[ iCurrentAE ] ) - 1 )
                {
                    *ppgnoDbEndUnusedBegin = PgnoFirstFromExt( &rgAE[ iCurrentAE - 1 ] );
                }
                else
                {
                    break;
                }
            }

            Assert( *ppgnoDbEndUnusedBegin != lMax );
        }
        else
        {
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
                    objid = 1;
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
        pespCtx->cpgDbRootUnAccountedFor -= pBTreeStats->pSpaceTrees->cpgOwned;
        pespCtx->cpgDbRootUnAccountedForEof -= CpgSpaceDumpGetExtPagesEof( pespCtx, pBTreeStats->pSpaceTrees->prgOwnedExtents, pBTreeStats->pSpaceTrees->cOwnedExtents );
    }

    if ( eBTType == eBTreeTypeUserClusteredIndex )
    {
        if ( pespCtx->cpgCurrTableUnAccountedFor ||
                pespCtx->cpgCurrIdxLvUnAccountedFor )
        {
            if ( pespCtx->cpgCurrTableUnAccountedFor )
            {
                if ( pespCtx->cpgCurrTableUnAccountedFor < 0 )
                {
                    wprintf(L"\n\t\tERROR: objid(%d) has %d pages overbooked.\n"
                            L"\t\tPlease contact PSS.\n\n",
                                pespCtx->objidCurrTable, -pespCtx->cpgCurrTableUnAccountedFor );
                    pespCtx->cpgTotalOverbooked += -pespCtx->cpgCurrTableUnAccountedFor;
                }
                else
                {
                    if ( pespCtx->fPrintSpaceLeaks )
                    {
                        wprintf(L"\t\tTable Objid(%d) leaked %d pages.\n",
                                    pespCtx->objidCurrTable, pespCtx->cpgCurrTableUnAccountedFor );
                    }
                    pespCtx->cpgTotalLeaked += pespCtx->cpgCurrTableUnAccountedFor;
                    pespCtx->cpgTotalLeakedEof += pespCtx->cpgCurrTableUnAccountedForEof;
                }

                pespCtx->objidCurrTable = 0;
                pespCtx->cpgCurrTableUnAccountedFor = 0;
                pespCtx->cpgCurrTableUnAccountedForEof = 0;
            }

            if ( pespCtx->cpgCurrIdxLvUnAccountedFor )
            {
                assert( pespCtx->eBTTypeCurr != eBTreeTypeInvalid );
                if ( pespCtx->cpgCurrIdxLvUnAccountedFor < 0 )
                {
                    wprintf(L"\n\t\tERROR: objid(%d) has %d pages overbooked.\n"
                            L"\t\tPlease contact PSS.\n\n",
                                pespCtx->objidCurrIdxLv, -pespCtx->cpgCurrIdxLvUnAccountedFor );
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
                    pespCtx->cpgTotalLeaked += pespCtx->cpgCurrIdxLvUnAccountedFor;
                    pespCtx->cpgTotalLeakedEof += pespCtx->cpgCurrIdxLvUnAccountedForEof;
                }


                pespCtx->objidCurrIdxLv = 0;
                pespCtx->eBTTypeCurr = eBTreeTypeInvalid;
                pespCtx->cpgCurrIdxLvUnAccountedFor = 0;
                pespCtx->cpgCurrIdxLvUnAccountedForEof = 0;
            }
        }

        pespCtx->objidCurrTable = pBTreeStats->pBasicCatalog->objidFDP;
        pespCtx->cpgCurrTableUnAccountedFor = pBTreeStats->pSpaceTrees->cpgOwned;
        pespCtx->cpgCurrTableUnAccountedForEof = CpgSpaceDumpGetExtPagesEof( pespCtx, pBTreeStats->pSpaceTrees->prgOwnedExtents, pBTreeStats->pSpaceTrees->cOwnedExtents );
        pespCtx->cpgCurrTableUnAccountedFor -= pBTreeStats->pSpaceTrees->cpgAvailable;
        pespCtx->cpgCurrTableUnAccountedForEof -= CpgSpaceDumpGetExtPagesEof( pespCtx, pBTreeStats->pSpaceTrees->prgAvailExtents, pBTreeStats->pSpaceTrees->cAvailExtents );
        pespCtx->cpgCurrTableUnAccountedFor -= pBTreeStats->pParentOfLeaf->cpgInternal;
        pespCtx->cpgCurrTableUnAccountedFor -= pBTreeStats->pParentOfLeaf->cpgData;
    }
    else
    {
        if ( FChildOfTable( pBTreeStats ) )
        {
            pespCtx->cpgCurrTableUnAccountedFor -= pBTreeStats->pSpaceTrees->cpgOwned;
            pespCtx->cpgCurrTableUnAccountedForEof -= CpgSpaceDumpGetExtPagesEof( pespCtx, pBTreeStats->pSpaceTrees->prgOwnedExtents, pBTreeStats->pSpaceTrees->cOwnedExtents );
        }
    }

    if ( eBTType == eBTreeTypeInternalLongValue ||
            eBTType == eBTreeTypeUserSecondaryIndex )
    {
        if ( pespCtx->cpgCurrIdxLvUnAccountedFor )
        {

            if ( pespCtx->cpgCurrIdxLvUnAccountedFor < 0 )
            {
                wprintf(L"\n\t\tERROR: objid(%d) has %d pages overbooked.\n"
                        L"\t\tPlease contact PSS.\n\n",
                            pespCtx->objidCurrIdxLv, -pespCtx->cpgCurrIdxLvUnAccountedFor );
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
                pespCtx->cpgTotalLeaked += pespCtx->cpgCurrIdxLvUnAccountedFor;
                pespCtx->cpgTotalLeakedEof += pespCtx->cpgCurrIdxLvUnAccountedForEof;
            }


            pespCtx->objidCurrIdxLv = 0;
            pespCtx->eBTTypeCurr = eBTreeTypeInvalid;
            pespCtx->cpgCurrIdxLvUnAccountedFor = 0;
            pespCtx->cpgCurrIdxLvUnAccountedForEof = 0;
        }

        pespCtx->objidCurrIdxLv = pBTreeStats->pBasicCatalog->objidFDP;
        pespCtx->eBTTypeCurr = eBTType;
        pespCtx->cpgCurrIdxLvUnAccountedFor = pBTreeStats->pSpaceTrees->cpgOwned;
        pespCtx->cpgCurrIdxLvUnAccountedForEof = CpgSpaceDumpGetExtPagesEof( pespCtx, pBTreeStats->pSpaceTrees->prgOwnedExtents, pBTreeStats->pSpaceTrees->cOwnedExtents );
        pespCtx->cpgCurrIdxLvUnAccountedFor -= pBTreeStats->pSpaceTrees->cpgAvailable;
        pespCtx->cpgCurrIdxLvUnAccountedForEof -= CpgSpaceDumpGetExtPagesEof( pespCtx, pBTreeStats->pSpaceTrees->prgAvailExtents, pBTreeStats->pSpaceTrees->cAvailExtents );
        pespCtx->cpgCurrIdxLvUnAccountedFor -= pBTreeStats->pParentOfLeaf->cpgInternal;
        pespCtx->cpgCurrIdxLvUnAccountedFor -= pBTreeStats->pParentOfLeaf->cpgData;
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


    if ( pBTreeStats->cbStruct != sizeof(*pBTreeStats) )
    {
        wprintf(L"The ESE engine did not return expected stat data size.\n");
        return ErrERRCheck( JET_errInvalidParameter );
    }

    assert( pBTreeStats->pBasicCatalog );
    if ( NULL == pBTreeStats->pBasicCatalog )
    {
        wprintf(L"The ESE engine did not return expected catalog data.\n");
        return ErrERRCheck( JET_errInvalidParameter );
    }

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
            wprintf(L"Unknown BTree enum: %d\n", eBTType );
            return ErrERRCheck( JET_errInvalidParameter );
    }


    if( eBTreeTypeInternalDbRootSpace == eBTType )
    {

        pespCtx->pgnoDbEndUnusedBegin = 0;
        pespCtx->pgnoDbEndUnusedEnd = 0;

        if ( pBTreeStats->pSpaceTrees )
        {
            pespCtx->cpgTotalShelved = pBTreeStats->pSpaceTrees->cpgShelved;
            pespCtx->cpgDbOwned = pBTreeStats->pSpaceTrees->cpgOwned;
            pespCtx->pgnoLast = pBTreeStats->pSpaceTrees->cpgOwned;
            pespCtx->cpgDbRootUnAccountedFor = pespCtx->cpgDbOwned + pespCtx->cpgTotalShelved;
            pespCtx->cpgDbRootUnAccountedForEof = pespCtx->cpgTotalShelved;

            pespCtx->cpgDbRootUnAccountedFor -= pBTreeStats->pSpaceTrees->cpgAvailable;
            pespCtx->cpgTotalAvailExt += pBTreeStats->pSpaceTrees->cpgAvailable;
            pespCtx->pgnoUsedMin = ulMax;
            pespCtx->pgnoUsedMax = 0;

            EseutilCalculateLastAvailableDbLogicalExtentRange( pBTreeStats->pSpaceTrees, &pespCtx->pgnoDbEndUnusedBegin, &pespCtx->pgnoDbEndUnusedEnd );

            Assert( ( pespCtx->pgnoDbEndUnusedEnd <= pespCtx->pgnoLast ) ||
                ( ( pespCtx->pgnoDbEndUnusedBegin == ( pespCtx->pgnoLast + 1 ) ) && ( pespCtx->pgnoDbEndUnusedEnd == lMax ) ) );

            if ( pBTreeStats->pParentOfLeaf )
            {
                assert( pBTreeStats->pParentOfLeaf->cpgData );
                pespCtx->cpgDbRootUnAccountedFor -= pBTreeStats->pParentOfLeaf->cpgData;
            }

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

        Call( EseutilSpaceDumpPrintHeadersAndDbRoot( pBTreeStats, pespCtx ) );

        if ( ( pespCtx->grbit & JET_bitDBUtilSpaceInfoSpaceTrees ) && pespCtx->fPrintSpaceNodes )
        {
            Assert( pBTreeStats->pSpaceTrees );
            Assert( pBTreeStats->pSpaceTrees->prgOwnedExtents );
            Assert( pBTreeStats->pSpaceTrees->cOwnedExtents );
            Call( EseutilPrintSpaceTrees( pespCtx, L"DbRoot", L"", pBTreeStats->pSpaceTrees ) );
        }

        return JET_errSuccess;
    }

    assert( pBTreeStats->pParent );



    assert( eBTType < eBTreeTypeMax );
    pespCtx->cBTrees[eBTType]++;

    if ( pBTreeStats->pParent->pBasicCatalog->objidFDP == 1  &&
            ( eBTType == eBTreeTypeInternalSpaceOE || eBTType == eBTreeTypeInternalSpaceAE ) )
    {
    }

    if ( pBTreeStats->pSpaceTrees )
    {
        if ( pBTreeStats->pBasicCatalog->eType == eBTreeTypeUserClusteredIndex )
        {
            pespCtx->cpgCurrentTableOwned = pBTreeStats->pSpaceTrees->cpgOwned;
        }
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

    if ( pBTreeStats->pSpaceTrees && pBTreeStats->pParentOfLeaf )
    {
        EseutilTrackSpace( pespCtx, pBTreeStats, eBTType );
    }

    switch( eBTType )
    {
        case eBTreeTypeInternalSpaceOE:
        case eBTreeTypeInternalSpaceAE:
            if ( !pespCtx->fPrintSpaceTrees )
            {
                break;
            }
        case eBTreeTypeUserClusteredIndex:
        case eBTreeTypeInternalLongValue:
        case eBTreeTypeUserSecondaryIndex:
            if ( !pespCtx->fPrintSmallTrees && pBTreeStats->pSpaceTrees )
            {
                double dblPct = ((double)pBTreeStats->pSpaceTrees->cpgOwned) / ((double)pespCtx->cpgDbOwned);
                if ( dblPct < g_dblSmallTableThreshold )
                {
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

    BTREE_STATS_BASIC_CATALOG   FakeBasicCatalog = { sizeof(BTREE_STATS_BASIC_CATALOG),
                                                        eBTreeTypeUserClusteredIndex,
                                                        L"",
                                                        0x0, 0x0, NULL
                                                    };
    BTREE_STATS_SPACE_TREES     EmptySpaceTrees = { sizeof(BTREE_STATS_SPACE_TREES),
                                                        0, 0, fFalse, 0, 0,
                                                        0,
                                                        0, 0, 0
                                                    };
    BTREE_STATS_PARENT_OF_LEAF  EmptyParentOfLeaf = { sizeof(BTREE_STATS_PARENT_OF_LEAF),
                                                        fTrue,
                                                        0,
                                                        0,
                                                        0, NULL, 0, NULL
                                                    };
    BTREE_STATS                 EmptyBTreeStats = { sizeof(BTREE_STATS),
                                                        0x0,
                                                        NULL,
                                                        &FakeBasicCatalog,
                                                        &EmptySpaceTrees,
                                                        &EmptyParentOfLeaf,
                                                        NULL
                                                    };

    EseutilTrackSpace( pespCtx, &EmptyBTreeStats, eBTreeTypeUserClusteredIndex );

}
    
JET_ERR ErrSpaceDumpCtxComplete(
    __inout void *          pvContext,
    __in JET_ERR            err )
{
    ESEUTIL_SPACE_DUMP_CTX * pespCtx = (ESEUTIL_SPACE_DUMP_CTX *)pvContext;

    if ( pespCtx == NULL || err != JET_errSuccess )
    {
        goto HandleError;
    }


    if ( pespCtx->objidCurrTable ||
            pespCtx->objidCurrIdxLv ||
            pespCtx->fPrintSpaceLeaks )
    {
        assert( pespCtx->objidCurrTable );

        EseutilTrackSpaceComplete( pespCtx );
    }


    if ( !pespCtx->fSelectOneTable )
    {
        if ( pespCtx->cpgDbRootUnAccountedFor < 0 )
        {
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


    wprintf( L"\n" );

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
    

    if ( pespCtx->grbit & JET_bitDBUtilSpaceInfoSpaceTrees )
    {


        wprintf( L"    Pages %d (", pespCtx->cpgDbOwned );


        double dblPctAvail = ((double)pespCtx->cpgTotalAvailExt) / ((double)pespCtx->cpgDbOwned) * 100;

        ULONG pgnoShelvedMin = ulMax, pgnoShelvedMax = 0;

        if ( !(pespCtx->grbit & JET_bitDBUtilSpaceInfoParentOfLeaf) )
        {
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
            ULONG cpgOverhead = pespCtx->cpgTotalOEOverhead + pespCtx->cpgTotalAEOverhead + pespCtx->cpgTotalInternal;
            double dblPctOverhead = ((double)cpgOverhead) / ((double)pespCtx->cpgDbOwned) * 100;
            wprintf( L" %lu Overhead (%.1f%%),", cpgOverhead, dblPctOverhead );
            #else
            double dblPctInternal = ((double)pespCtx->cpgTotalInternal) / ((double)pespCtx->cpgDbOwned) * 100;
            double dblPctOE = ((double)pespCtx->cpgTotalOEOverhead) / ((double)pespCtx->cpgDbOwned) * 100;
            double dblPctAE = ((double)pespCtx->cpgTotalAEOverhead) / ((double)pespCtx->cpgDbOwned) * 100;
            wprintf( L" Internal %lu (%.1f%%), %lu OE (%.1f%%), %lu AE (%.1f%%),",
                     pespCtx->cpgTotalInternal, dblPctInternal,
                     pespCtx->cpgTotalOEOverhead, dblPctOE,
                     pespCtx->cpgTotalAEOverhead, dblPctAE );
            #endif

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
                wprintf( L" There are %lu overbooked pages.\n", pespCtx->cpgTotalOverbooked );
            }
        }
        if ( pespCtx->pgnoUsedMin == ulMax )
        {
            pespCtx->pgnoUsedMin = 0;
        }

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
            err = ErrERRCheck( JET_errDatabaseCorrupted );
        }
        else if ( pespCtx->cpgTotalLeaked )
        {
            err = ErrERRCheck( JET_errDatabaseLeakInSpace );
        }
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



LOCAL VOID DBUTLPrintfIntN( INT iValue, INT ichMax )
{
    CHAR    rgchT[17]; 
    INT     ichT;

    _itoa_s( iValue, rgchT, _countof(rgchT), 10 );
    for ( ichT = 0; ichT < sizeof(rgchT) && rgchT[ichT] != '\0' ; ichT++ )
        ;
    if ( ichT > ichMax )
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
        wprintf(L"The ESE engine did not return expected stat data size.\n");
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( NULL == pBTreeStats->pBasicCatalog || NULL == pBTreeStats->pSpaceTrees )
    {
        wprintf(L"The ESE engine did not return expected catalog data.\n");
        return ErrERRCheck( JET_errInvalidParameter );
    }

    *pcpgAvailTotal += pBTreeStats->pSpaceTrees->cpgAvailable;


    switch( pBTreeStats->pBasicCatalog->eType )
    {

        case eBTreeTypeInternalDbRootSpace:


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
            wprintf(L"Eseutil did not recognize the B-Tree type, incompatible engine and utility versions?\n");
            return ErrERRCheck( JET_errInvalidParameter );

    }

    return JET_errSuccess;
}

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


void EseutilDbspacedumpUnitTest()
{
#if DEBUG
    BTREE_SPACE_EXTENT_INFO rgOETwoExtent[] =
    {
        { ulsppAvailExtLegacyGeneralPool, 8, 8, 1 },
        { ulsppAvailExtLegacyGeneralPool, 16, 8, 1 },
    };

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

    EseutilITestDbspacedump( 14, 16, _countof( rgOETwoExtent ), rgOETwoExtent, _countof( rgAESingle ), rgAESingle );
    EseutilITestDbspacedump( 14, 16, _countof( rgOETwoExtent ), rgOETwoExtent, _countof( rgAESingleWithShelved ), rgAESingleWithShelved );

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

    BTREE_SPACE_EXTENT_INFO rgAEOnlyShelved[] =
    {
        { ulsppShelvedPool, 22, 1, 1 },
        { ulsppShelvedPool, 33, 1, 1 },
    };
    EseutilITestDbspacedump( 17, lMax, _countof( rgOETwoExtent ), rgOETwoExtent, 0, NULL );
    EseutilITestDbspacedump( 17, lMax, _countof( rgOETwoExtent ), rgOETwoExtent, _countof( rgAEOnlyShelved ), rgAEOnlyShelved );

#endif
}

