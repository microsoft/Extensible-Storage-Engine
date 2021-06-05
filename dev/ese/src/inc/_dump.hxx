// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  ================================================================
struct TABLEDEF
//  ================================================================
{
    ULONG   pgnoFDP;
    ULONG   objidFDP;
    
    ULONG   pgnoFDPLongValues;
    ULONG   objidFDPLongValues;
    
    LONG            pages;
    LONG            density;
    LONG            fFlags;
    LONG            cbLVChunkMax;

    JET_SPACEHINTS  spacehints;
    JET_SPACEHINTS  spacehintsDeferredLV;
    JET_SPACEHINTS  spacehintsLV;

    char            szName[JET_cbNameMost + 1];
    char            szTemplateTable[JET_cbNameMost + 1];
};
    

//  ================================================================
struct INDEXDEF
//  ================================================================
{
    ULONG   pgnoFDP;
    ULONG   objidFDP;

    LONG            density;
    ULONG   lcid;
    WCHAR           wszLocaleName[NORM_LOCALE_NAME_MAX_LENGTH];
    SORTID          sortID;
    ULONG   dwMapFlags;
    
    ULONG   dwNLSVersion;
    ULONG   dwDefinedVersion;
    
    LONG            ccolumnidDef;
    LONG            ccolumnidConditional;
    
    ULONG   fFlags;         //  the raw flags for the index

    union
    {
        ULONG ul;
        struct
        {
            UINT fUnique:1;
            UINT fPrimary:1;
            UINT fAllowAllNulls:1;
            UINT fAllowFirstNull:1;
            UINT fAllowSomeNulls:1;
            UINT fNoNullSeg:1;
            UINT fSortNullsHigh:1;
            UINT fMultivalued:1;
            UINT fTuples:1;
            UINT fLocaleSet:1;
            UINT fLocalizedText:1;
            UINT fTemplateIndex:1;
            UINT fDerivedIndex:1;
            UINT fExtendedColumns:1;
        };
    };

    LE_TUPLELIMITS  le_tuplelimits;

    USHORT          cbVarSegMac;
    USHORT          cbKeyMost;

    JET_SPACEHINTS  spacehints;

    IDXSEG          rgidxsegDef[JET_ccolKeyMost];
    IDXSEG          rgidxsegConditional[JET_ccolKeyMost];
    
    char            szName[JET_cbNameMost + 1];
    
    char            rgszIndexDef[JET_ccolKeyMost][JET_cbNameMost + 1 + 1];
    char            rgszIndexConditional[JET_ccolKeyMost][JET_cbNameMost + 1 + 1];
};


//  ================================================================
struct COLUMNDEF
//  ================================================================
{
    JET_COLUMNID    columnid;
    JET_COLTYP      coltyp;

    LONG            cbLength;           //  length of column
    LONG            cp;                 //  code page (for text columns only)
    LONG            ibRecordOffset;     //  offset of record in column
    LONG            presentationOrder;
    ULONG   cbDefaultValue;
    ULONG   cbCallbackData;

    LONG            fFlags;

    union
    {
        ULONG   ul;
        struct
        {
            UINT fFixed:1;
            UINT fVariable:1;
            UINT fTagged:1;
            UINT fVersion:1;
            UINT fNotNull:1;
            UINT fMultiValue:1;
            UINT fAutoIncrement:1;
            UINT fEscrowUpdate:1;
            UINT fDefaultValue:1;
            UINT fVersioned:1;
            UINT fDeleted:1;
            UINT fFinalize:1;
            UINT fDeleteOnZero:1;
            UINT fUserDefinedDefault:1;
            UINT fTemplateColumnESE98:1;
            UINT fPrimaryIndexPlaceholder:1;
            UINT fCompressed:1;
            UINT fEncrypted:1;
        };
    };

    char            szName[JET_cbNameMost + 1];
    char            szCallback[JET_cbNameMost + 1];
    unsigned char   rgbDefaultValue[cbDefaultValueMost];
    unsigned char   rgbCallbackData[JET_cbCallbackUserDataMost];
};


//  ================================================================
struct CALLBACKDEF
//  ================================================================
{
    char            szName[JET_cbNameMost + 1];
    JET_CBTYP       cbtyp;
    char            szCallback[JET_cbColumnMost + 1];
};


//  ================================================================
struct PAGEDEF
//  ================================================================
{
    __int64         dbtime;
    
    ULONG   pgno;
    ULONG   objidFDP;
    ULONG   pgnoNext;
    ULONG   pgnoPrev;

    ULONG   rgpgnoChildren[365];
    
    const unsigned char *   pbRawPage;

    ULONG   fFlags;

    SHORT           cbFree;
    SHORT           cbUncommittedFree;
    SHORT           clines;

    union
    {
        unsigned char   uch;
        struct
        {
            UINT    fLeafPage:1;
            UINT    fInvisibleSons:1;
            UINT    fRootPage:1;
            UINT    fPrimaryPage:1;
            UINT    fEmptyPage:1;
            UINT    fParentOfLeaf:1;
        };
    };
};

typedef INT(*PFNTABLE)( const TABLEDEF * ptable, void * pv );
typedef INT(*PFNINDEX)( const INDEXDEF * pidx, void * pv );
typedef INT(*PFNCOLUMN)( const COLUMNDEF * pcol, void * pv );
typedef INT(*PFNCALLBACKFN)( const CALLBACKDEF * pcol, void * pv );
typedef INT(*PFNPAGE)( const PAGEDEF * ppage, void * pv );


VOID __cdecl DUMPPrintF( const CHAR * fmt, ... );
VOID DUMPPrintLogTime( const LOGTIME * const plt );
VOID DUMPPrintSig( const SIGNATURE * const psig );

ERR ErrESEDUMPData( JET_SESID, JET_DBUTIL_W *pdbutil );

ERR ErrDUMPHeader( INST *pinst, __in PCWSTR wszDatabase, const BOOL fVerbose );
ERR ErrDUMPFixupHeader( INST *pinst, __in PCWSTR wszDatabase, const BOOL fVerbose );

ERR ErrDUMPCheckpoint( INST *pinst, __in PCWSTR wszCheckpoint );
ERR ErrDUMPLog( INST *pinst, __in PCWSTR wszLog, const LONG lgenStart, const LONG lgenEnd, JET_GRBIT grbit, __in PCWSTR wszCsvDataFile );
ERR ErrDUMPLogFromMemory( INST *pinst, __in PCWSTR wszLog, __in_bcount( cbBuffer ) void *pvBuffer, ULONG cbBuffer );

ERR ErrDUMPRBSHeader( INST *pinst, __in PCWSTR wszRBS, const BOOL fVerbose );
ERR ErrDUMPRBSPage( INST *pinst,  __in PCWSTR wszRBS, PGNO pgnoFirst, PGNO pgnoLast, const BOOL fVerbose );
