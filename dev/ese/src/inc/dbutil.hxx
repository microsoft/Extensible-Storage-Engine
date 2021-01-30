// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

struct DBCCINFO
{
    PIB             *ppib;
    IFMP            ifmp;
    FUCB            *pfucb;
    DBUTIL_OP       op;
    JET_GRBIT       grbitOptions;

    
    JET_TABLEID     tableidPageInfo;
    JET_TABLEID     tableidSpaceInfo;

    
    WCHAR           wszDatabase[OSFSAPI_MAX_PATH + 1];
    WCHAR           wszTable[JET_cbNameMost + 1];
    WCHAR           wszIndex[JET_cbNameMost + 1];
};

struct LVSTATS
{
    QWORD   cbLVTotal;
    QWORD   cbLVMultiRefTotal;

    QWORD   lidMin;
    QWORD   lidMax;

    QWORD   clv;
    QWORD   clvMultiRef;
    QWORD   clvNoRef;
    QWORD   referenceMax;
    QWORD   referenceTotal;
    
    QWORD   cbLVMin;
    QWORD   cbLVMax;

    QWORD   clvFragmentedRoot;
    QWORD   clvUnfragmentedRoot;
};
    

struct BTSTATS
{
    QWORD   cbDataInternal;
    QWORD   cbDataLeaf;

    QWORD   cpageEmpty;
    QWORD   cpagePrefixUnused;
    QWORD   cpageDepth;

    QWORD   cpageInternal;
    QWORD   cpageLeaf;

    QWORD   rgckeyPrefixInternal[cbKeyAlloc];
    QWORD   rgckeyPrefixLeaf[cbKeyAlloc];

    QWORD   rgckeyInternal[cbKeyAlloc];
    QWORD   rgckeyLeaf[cbKeyAlloc];

    QWORD   rgckeySuffixInternal[cbKeyAlloc];
    QWORD   rgckeySuffixLeaf[cbKeyAlloc];
    
    QWORD   cnodeVersioned;
    QWORD   cnodeDeleted;
    QWORD   cnodeCompressed;

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

INLINE const CHAR * SzColumnType( const JET_COLTYP coltyp )
{
    CHAR * szType;

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

