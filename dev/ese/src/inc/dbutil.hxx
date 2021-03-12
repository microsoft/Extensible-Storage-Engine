// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

struct DBCCINFO
{
    PIB             *ppib;
    IFMP            ifmp;
    FUCB            *pfucb;
    DBUTIL_OP       op;
    JET_GRBIT       grbitOptions;

    /*  consistency check information
    /**/
    JET_TABLEID     tableidPageInfo;
    JET_TABLEID     tableidSpaceInfo;

    /*  common information
    /**/
    WCHAR           wszDatabase[OSFSAPI_MAX_PATH + 1];
    WCHAR           wszTable[JET_cbNameMost + 1];
    WCHAR           wszIndex[JET_cbNameMost + 1];
};

//  ================================================================
struct LVSTATS
//  ================================================================
{
    QWORD   cbLVTotal;          //  total bytes stored in LV's
    QWORD   cbLVMultiRefTotal;  //  total bytes stored in multi-ref LV's

    QWORD   lidMin;             //  smallest LID in the table
    QWORD   lidMax;             //  largest LID in the table

    QWORD   clv;                //  total number of LVs in the table
    QWORD   clvMultiRef;        //  total number of LVs with refcount > 1
    QWORD   clvNoRef;           //  total number of LVs with refcount == 0
    QWORD   referenceMax;       //  largest refcount on a LV
    QWORD   referenceTotal;     //  total LV refcounts
    
    QWORD   cbLVMin;            //  smallest LV
    QWORD   cbLVMax;            //  largest LV

    QWORD   clvFragmentedRoot;  //  total number of LVs where the root is on a different page than the first chunk
    QWORD   clvUnfragmentedRoot;//  total number of LVs where the root is on the same page as the first chunk
};
    

//  ================================================================
struct BTSTATS
//  ================================================================
{
    QWORD   cbDataInternal;     //  total data stored in internal nodes
    QWORD   cbDataLeaf;         //  total data stored in leaf nodes

    QWORD   cpageEmpty;         //  number of empty pages we encountered (all nodes are FlagDeleted)
    QWORD   cpagePrefixUnused;  //  number of pages we encountered where the prefix is not used (no nodes are compressed)
    QWORD   cpageDepth;         //  depth of the tree

    QWORD   cpageInternal;  //  number of internal pages
    QWORD   cpageLeaf;      //  number of leaf pages

    QWORD   rgckeyPrefixInternal[cbKeyAlloc];   //  distribution of key lengths (prefix) each prefix is counted once per page
    QWORD   rgckeyPrefixLeaf[cbKeyAlloc];       //  distribution of key lengths (prefix) each prefix is counted once per page

    QWORD   rgckeyInternal[cbKeyAlloc];     //  distribution of key lengths (prefix and suffix)
    QWORD   rgckeyLeaf[cbKeyAlloc];         //  distribution of key lengths (prefix and suffix)

    QWORD   rgckeySuffixInternal[cbKeyAlloc];   //  distribution of key lengths (suffix)
    QWORD   rgckeySuffixLeaf[cbKeyAlloc];       //  distribution of key lengths (suffix)
    
    QWORD   cnodeVersioned;     //  number of Versioned nodes
    QWORD   cnodeDeleted;       //  number of FlagDeleted nodes
    QWORD   cnodeCompressed;    //  number of KeyCompressed nodes

    PGNO pgnoLastSeen;
    PGNO pgnoNextExpected;
    
};

VOID DBUTLSprintHex(
    __out_bcount(cbDest) CHAR * const       szDest,
    const INT           cbDest,
    const BYTE * const  rgbSrc,
    const INT           cbSrc,
    const INT           cbWidth     = 16,
    const INT           cbChunk     = 4,
    const INT           cbAddress   = 8,
    const INT           cbStart     = 0);

//  ================================================================
INLINE const CHAR * SzColumnType( const JET_COLTYP coltyp )
//  ================================================================
{
    const CHAR * szType;

    switch ( coltyp )
    {
        case JET_coltypBit:
            szType = "Bit";
            break;

        case JET_coltypUnsignedByte:
            szType = "UnsignedByte";
            break;

        case JET_coltypShort:
            szType = "Short";
            break;

        case JET_coltypUnsignedShort:
            szType = "UnsignedShort";
            break;

        case JET_coltypLong:
            szType = "Long";
            break;

        case JET_coltypUnsignedLong:
            szType = "UnsignedLong";
            break;

        case JET_coltypLongLong:
            szType = "LongLong";
            break;

        case JET_coltypUnsignedLongLong:
            szType = "UnsignedLongLong";
            break;

        case JET_coltypCurrency:
            szType = "Currency";
            break;

        case JET_coltypIEEESingle:
            szType = "IEEESingle";
            break;

        case JET_coltypIEEEDouble:
            szType = "IEEEDouble";
            break;

        case JET_coltypDateTime:
            szType = "DateTime";
            break;

        case JET_coltypGUID:
            szType = "GUID";
            break;

        case JET_coltypBinary:
            szType = "Binary";
            break;

        case JET_coltypText:
            szType = "Text";
            break;

        case JET_coltypLongBinary:
            szType = "LongBinary";
            break;

        case JET_coltypLongText:
            szType = "LongText";
            break;

        case JET_coltypNil:
            szType = "<deleted>";
            break;

        default:
            szType = "<unknown>";
            break;
    }

    return szType;
}

