// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_comp.hxx"

#include "PageSizeClean.hxx"

// UNDONE:  Do these need to be localised?
#define wszCompactStatsFile     L"DFRGINFO.TXT"
#define szCompactAction         "Defragmentation"
#define szCMPSTATSTableName     "Table Name"
#define szCMPSTATSFixedVarCols  "# Non-Derived Fixed/Variable Columns"
#define szCMPSTATSTaggedCols    "# Non-Derived Tagged Columns"
#define szCMPSTATSColumns       "# Columns"
#define szCMPSTATSSecondaryIdx  "# Secondary Indexes"
#define szCMPSTATSPagesOwned    "Pages Owned (Source DB)"
#define szCMPSTATSPagesAvail    "Pages Avail. (Source DB)"
#define szCMPSTATSInitTime      "Table Create/Init. Time"
#define szCMPSTATSRecordsCopied "# Records Copied"
#define szCMPSTATSRawData       "Raw Data Bytes Copied"
#define szCMPSTATSRawDataLV     "Raw Data LV Bytes Copied"
#define szCMPSTATSLeafPages     "Leaf Pages Traversed"
#define szCMPSTATSMinLVPages    "Min. LV Pages Traversed"
#define szCMPSTATSRecordsTime   "Copy Records Time"
#define szCMPSTATSTableTime     "Copy Table Time"


//  ================================================================
class CMEMLIST
//  ================================================================
//
//  Allocate a chunk of memory, keep allocating more. All memory
//  can be destroyed.
//
//  Memory is stored in a singly-linked list. The first 4 bytes
//  of memory allocated points to the next chunk
//
//-
{
    private:
        VOID * m_pvHead;
        ULONG  m_cbAllocated;   //  includes list overhead

    public:
        CMEMLIST();
        ~CMEMLIST();

        VOID * PvAlloc( const ULONG cb );
        VOID FreeAllMemory();

    public:
#ifdef ENABLE_JET_UNIT_TEST
        static void UnitTest();
#endif  //  ENABLE_JET_UNIT_TEST

#ifdef DEBUG
        VOID AssertValid() const ;
#endif  //  DEBUG

    private:
        CMEMLIST( const CMEMLIST& );
        CMEMLIST& operator=( const CMEMLIST& );
};


//  ================================================================
CMEMLIST::CMEMLIST() :
    m_pvHead( 0 ),
    m_cbAllocated( 0 )
//  ================================================================
{
}


//  ================================================================
CMEMLIST::~CMEMLIST()
//  ================================================================
{
    //  Have people call FreeAllMemory to make the code clearer
    Assert( NULL == m_pvHead ); //  CMEMLIST::FreeAllMemory() not called?
    Assert( 0 == m_cbAllocated ); //    CMEMLIST::FreeAllMemory() not called?
    FreeAllMemory();
}


//  ================================================================
VOID * CMEMLIST::PvAlloc( const ULONG cb )
//  ================================================================
//
//  Get a new chunk of memory, put it at the head of the list
//
//-
{
    const ULONG cbActualAllocate = cb + sizeof( VOID* );

    //  if allocation too near numeric limit then fail as though OOM
    //
    if ( cbActualAllocate < cb )
    {
        return NULL;
    }

    VOID * const pvNew = PvOSMemoryHeapAlloc( cbActualAllocate );
    if( NULL == pvNew )
    {
        return NULL;
    }

    VOID * const pvReturn = reinterpret_cast<BYTE *>( pvNew ) + sizeof( VOID* );
    *(reinterpret_cast<VOID **>( pvNew ) ) = m_pvHead;
    m_pvHead = pvNew;

    m_cbAllocated += cbActualAllocate;

    return pvReturn;
}


//  ================================================================
VOID CMEMLIST::FreeAllMemory()
//  ================================================================
{
    VOID * pv = m_pvHead;
    while( pv )
    {
        VOID * const pvNext = *(reinterpret_cast<VOID **>( pv ) );
        OSMemoryHeapFree( pv );
        pv = pvNext;
    }

    m_cbAllocated   = 0;
    m_pvHead        = NULL;
}


#ifdef DEBUG
//  ================================================================
VOID CMEMLIST::AssertValid() const
//  ================================================================
{
    const VOID * pv = m_pvHead;
    while( pv )
    {
        const VOID * const pvNext = *((VOID **)( pv ) );
        pv = pvNext;
    }

    if( 0 == m_cbAllocated )
    {
        Assert( NULL == m_pvHead );
    }
    else
    {
        Assert( NULL != m_pvHead );
    }
}
#endif  //  DEBUG


#ifdef ENABLE_JET_UNIT_TEST
//  ================================================================
VOID CMEMLIST::UnitTest()
//  ================================================================
//
//  STATIC function
//
//-
{
    CMEMLIST cmemlist;

    ULONG   cbAllocated = 0;
    INT     i;
    //  void    *pvT = NULL;

    for( i = 0; i < 64; ++i )
    {
        VOID * const pv = cmemlist.PvAlloc( i );
        AssertRTL( NULL != pv );    //  Out-of-memory is not acceptable during a unit test :-)
        cbAllocated += i + sizeof( VOID* );
        AssertRTL( cbAllocated == cmemlist.m_cbAllocated );
        ASSERT_VALID( &cmemlist );
    }

    cmemlist.FreeAllMemory();
    ASSERT_VALID( &cmemlist );

    (VOID)cmemlist.PvAlloc( 1024 * 1024 );
    ASSERT_VALID( &cmemlist );

    //  pvT = cmemlist.PvAlloc( 0x7fffffff );   //  try an allocation that fails
    //  Assert( NULL == pvT );
    //  ASSERT_VALID( &cmemlist );

    cmemlist.FreeAllMemory();
    ASSERT_VALID( &cmemlist );
}
#endif  //  ENABLE_JET_UNIT_TEST

struct COMPACTINFO
{
    PIB             *ppib;
    IFMP            ifmpSrc;
    IFMP            ifmpDest;
    COLUMNIDINFO    rgcolumnids[ ccolCMPFixedVar ];
    ULONG           ccolSingleValue;
    STATUSINFO      *pstatus;
    BYTE            rgbBuf[ 64 * 1024 ];        // Buffer for copying LV and other misc. usage
};

//  Giving compact / offline defrag access to this private function, as it's good testing.
CPG CpgDBDatabaseMinMin();

INLINE ERR ErrCMPOpenDB(
    COMPACTINFO     *pcompactinfo,
    const WCHAR     *wszDatabaseSrc,
    IFileSystemAPI  *pfsapiDest,
    const WCHAR     *wszDatabaseDest )
{
    ERR         err;
    JET_GRBIT   grbitCreateForDefrag    = JET_bitDbRecoveryOff|JET_bitDbVersioningOff;

    //  open the source DB Exclusive and ReadOnly
    //  UNDONE: JET_bitDbReadOnly currently unsupported
    //  by OpenDatabase (must be specified with AttachDb)
    CallR( ErrDBOpenDatabase(
                pcompactinfo->ppib,
                wszDatabaseSrc,
                &pcompactinfo->ifmpSrc,
                JET_bitDbExclusive|JET_bitDbReadOnly ) );

    if ( g_rgfmp[pcompactinfo->ifmpSrc].FShadowingOff() )
    {
        grbitCreateForDefrag |= JET_bitDbShadowingOff;
    }

    //  Create and then open the destination database.
    //  CONSIDER: Should the destination database be deleted
    //  if it already exists?
    Assert( NULL != pfsapiDest );
    err = ErrDBCreateDatabase(
                pcompactinfo->ppib,
                pfsapiDest,
                wszDatabaseDest,
                &pcompactinfo->ifmpDest,
                dbidMax,
                CpgDBDatabaseMinMin(),  //  using min-min minimizes DB size, and provides good testing.
                fFalse, // fSparseEnabledFile
                NULL,
                NULL,
                0,
                grbitCreateForDefrag );

    Assert( err <= 0 );     // No warnings.
    if ( err < 0 )
    {
        (VOID)ErrDBCloseDatabase(
                        pcompactinfo->ppib,
                        pcompactinfo->ifmpSrc,
                        NO_GRBIT );
    }

    return err;
}


LOCAL VOID CMPCopyOneIndex(
    FCB                     * const pfcbSrc,
    FCB                     *pfcbIndex,
    JET_INDEXCREATE3_A      *pidxcreate,
    size_t                  cbIdxCreateIndexName,
    size_t                  cbIdxCreateKeys,
    JET_TUPLELIMITS         *ptuplelimits,
    JET_CONDITIONALCOLUMN_A *pconditionalcolumn,
    __out JET_SPACEHINTS *  pjsph )
{
    TDB                     *ptdb = pfcbSrc->Ptdb();
    IDB                     *pidb = pfcbIndex->Pidb();

    Assert( pfcbSrc->FTypeTable() );
    Assert( pfcbSrc->FPrimaryIndex() );

    Assert( ptdbNil != ptdb );
    Assert( pidbNil != pidb );

    // Derived indexes are inherited at table creation time.
    Assert( !pfcbIndex->FDerivedIndex() );

    Assert( sizeof(JET_INDEXCREATE3_A) == pidxcreate->cbStruct );

    pfcbSrc->EnterDML();    // Strictly speaking, not needed because defrag is single-threaded

    OSStrCbCopyA( pidxcreate->szIndexName, cbIdxCreateIndexName, ptdb->SzIndexName( pidb->ItagIndexName() ) );

    ULONG   cb = (ULONG)strlen( pidxcreate->szIndexName );
    Assert( cb > 0 );
    Assert( cb <= JET_cbNameMost );
    Assert( pidxcreate->szIndexName[cb] == '\0' );

    CHAR    * const szKey = pidxcreate->szKey;
    ULONG   ichKey = 0;
    const IDXSEG* rgidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb );

    Assert( pidb->Cidxseg() > 0 );
    Assert( pidb->Cidxseg() <= JET_ccolKeyMost );
    ULONG iidxseg;
    for ( iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
    {
        const COLUMNID  columnid        = rgidxseg[iidxseg].Columnid();
        const FIELD     * const pfield  = ptdb->Pfield( columnid );

        Assert( pfieldNil != pfield );

        if ( !FFIELDPrimaryIndexPlaceholder( pfield->ffield ) )
        {
            Enforce( cbIdxCreateKeys - ichKey <= cbIdxCreateKeys );
            const BOOL      fDerived    = ( FCOLUMNIDTemplateColumn( columnid ) && !ptdb->FTemplateTable() );
            szKey[ichKey++] = ( rgidxseg[iidxseg].FDescending() ? '-' : '+' );
            OSStrCbCopyA( szKey+ichKey, cbIdxCreateKeys-ichKey, ptdb->SzFieldName( pfield->itagFieldName, fDerived ) );
            cb = (ULONG)strlen( szKey+ichKey );
            Assert( cb > 0 );
            Assert( cb <= JET_cbNameMost );

            Assert( szKey[ichKey+cb] == '\0' );
            ichKey += cb + 1;       // +1 for segment's null terminator.
        }
        else
        {
            //  must be first column in primary index
            Assert( pidb->FPrimary() );
            Assert( 0 == iidxseg );
            Assert( 0 == ichKey );
        }
    }

    szKey[ichKey++] = '\0'; // double-null termination

    Assert( ichKey > 2 );

    Assert( pidb->CbVarSegMac() > 0 );
    Assert( pidb->CbVarSegMac() <= pidb->CbKeyMost() );

    pidxcreate->cbKey       = ichKey;
    pidxcreate->grbit       = pidb->GrbitFromFlags() | JET_bitIndexUnicode;
    pidxcreate->cbKeyMost   = pidb->CbKeyMost();

    if ( pidb->FTuples() )
    {
        pidxcreate->grbit |= JET_bitIndexTupleLimits;
        ptuplelimits->chLengthMin = pidb->CchTuplesLengthMin();
        ptuplelimits->chLengthMax = pidb->CchTuplesLengthMax();
        ptuplelimits->chToIndexMax = pidb->IchTuplesToIndexMax();
        ptuplelimits->cchIncrement = pidb->CchTuplesIncrement();
        ptuplelimits->ichStart = pidb->IchTuplesStart();
        pidxcreate->ptuplelimits = ptuplelimits;
    }
    else
    {
        pidxcreate->cbVarSegMac = pidb->CbVarSegMac();
    }

    //  API requires that key size be no lower than 255
    //
    if ( pidxcreate->cbKeyMost < JET_cbKeyMost_OLD )
    {
        if ( pidxcreate->cbVarSegMac == pidxcreate->cbKeyMost )
        {
            pidxcreate->cbVarSegMac = JET_cbKeyMost_OLD;
        }
        pidxcreate->cbKeyMost = JET_cbKeyMost_OLD;
    }

    Assert( NULL != pidxcreate->pidxunicode );

    if ( NULL != pidxcreate->pidxunicode->szLocaleName )
    {
        CallS( ErrOSStrCbCopyW( pidxcreate->pidxunicode->szLocaleName, sizeof( WCHAR ) * NORM_LOCALE_NAME_MAX_LENGTH, pidb->WszLocaleName() ) );
    }
    pidxcreate->pidxunicode->dwMapFlags = pidb->DwLCMapFlags();

    pidxcreate->rgconditionalcolumn = pconditionalcolumn;
    pidxcreate->cConditionalColumn  = pidb->CidxsegConditional();

    rgidxseg = PidxsegIDBGetIdxSegConditional( pidb, ptdb );
    for( iidxseg = 0; iidxseg < pidxcreate->cConditionalColumn; ++iidxseg )
    {
        CHAR * const szConditionalKey = szKey + ichKey;

        const COLUMNID  columnid    = rgidxseg[iidxseg].Columnid();
        const FIELD     *pfield     = ptdb->Pfield( columnid );
        const BOOL      fDerived    = FCOLUMNIDTemplateColumn( columnid ) && !ptdb->FTemplateTable();

        Assert( pfieldNil != pfield );

        OSStrCbCopyA( szConditionalKey, cbIdxCreateKeys-(szConditionalKey-szKey), ptdb->SzFieldName( pfield->itagFieldName, fDerived ) );
        ichKey += (ULONG)strlen( szConditionalKey ) + 1;

        pidxcreate->rgconditionalcolumn[iidxseg].cbStruct       = sizeof( JET_CONDITIONALCOLUMN_A );
        pidxcreate->rgconditionalcolumn[iidxseg].szColumnName   = szConditionalKey;
        pidxcreate->rgconditionalcolumn[iidxseg].grbit          = ( rgidxseg[iidxseg].FMustBeNull()
                                                ? JET_bitIndexColumnMustBeNull
                                                : JET_bitIndexColumnMustBeNonNull );
    }

    pfcbSrc->LeaveDML();

    pfcbIndex->GetAPISpaceHints( pjsph );
    Assert( pidxcreate->pSpacehints == pjsph ); // just to make sure.

}

LOCAL ERR ErrCMPCreateTableColumnIndex(
    COMPACTINFO     *pcompactinfo,
    FCB             * const pfcbSrc,
    JET_TABLECREATE5_A  *ptablecreate,
    JET_COLUMNLIST  *pcolumnList,
    COLUMNIDINFO    *columnidInfo,
    JET_COLUMNID    **pmpcolumnidcolumnidTagged )
{
    ERR             err;
    PIB             *ppib = pcompactinfo->ppib;
    FCB             *pfcbIndex;
    ULONG           cColumns;
    ULONG           cbColumnids;
    ULONG           cbAllocate;
    ULONG           cSecondaryIndexes;
    ULONG           cIndexesToCreate;
    ULONG           cConditionalColumns;
    ULONG           ccolSingleValue = 0;
    ULONG           cbActual;
    JET_COLUMNID    *mpcolumnidcolumnidTagged = NULL;
    FID             fidTaggedHighest = 0;
    ULONG           cTagged = 0;
    BOOL            fLocalAlloc = fFalse;
    ULONG           cbRecordMost = REC::CbRecordMost( pfcbSrc );
    ULONG           cbDefaultRecRemaining = cbRecordMost;

    //  all memory allocated from this will be freed at the end of the function
    //
    CMEMLIST        cmemlist;

    const INT       cbName          = JET_cbNameMost+1; // index/column name plus terminator
    const INT       cbLangid        = sizeof(LANGID)+2; // langid plus double-null terminator
    const INT       cbCbVarSegMac   = sizeof(BYTE)+2;   // cbVarSegMac plus double-null terminator
    const INT       cbKeySegment    = 1+cbName;         // +/- prefix plus name
    const INT       cbKey           = ( JET_ccolKeyMost * cbKeySegment ) + 1;   // plus 1 for double-null terminator
    const INT       cbKeyExtended   = cbKey + cbLangid + cbCbVarSegMac;

    Assert( ptablecreate->cCreated == 0 );

    
    // Allocate a pool of memory for:
    //      1) list of source table columnids
    //      2) the JET_COLUMNCREATE_A structures
    //      3) buffer for column names
    //      4) buffer for default values
    //      5) the JET_INDEXCREATE structures
    //      6) buffer for index names
    //      7) buffer for index keys.

    cColumns = pcolumnList->cRecord;

    //  start by allocating space for the source table columnids, adjusted for alignment
    cbColumnids = cColumns * sizeof(JET_COLUMNID);
    cbColumnids = ( ( cbColumnids + sizeof(SIZE_T) - 1 ) / sizeof(SIZE_T) ) * sizeof(SIZE_T);

    cbAllocate = cbColumnids +
                    ( cColumns *
                        ( sizeof(JET_COLUMNCREATE_A) +  // JET_COLUMNCREATE_A structures
                        cbName ) );                     // column names

    // Derived indexes will get inherited from template -- don't count
    // them as ones that need to be created.
    Assert( ( pfcbSrc->FSequentialIndex() && pfcbSrc->Pidb() == pidbNil )
        || ( !pfcbSrc->FSequentialIndex() && pfcbSrc->Pidb() != pidbNil ) );
    cIndexesToCreate = ( pfcbSrc->Pidb() != pidbNil && !pfcbSrc->FDerivedIndex() ? 1 : 0 );
    cConditionalColumns = 0;
    cSecondaryIndexes = 0;
    for ( pfcbIndex = pfcbSrc->PfcbNextIndex();
        pfcbIndex != pfcbNil;
        pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        cConditionalColumns += pfcbIndex->Pidb()->CidxsegConditional();
        cSecondaryIndexes++;
        Assert( pfcbIndex->FTypeSecondaryIndex() );
        Assert( pfcbIndex->Pidb() != pidbNil );
        if ( !pfcbIndex->FDerivedIndex() )
            cIndexesToCreate++;
    }

    //  ensure primary extent is large enough to at least accommodate the primary index
    //  and each secondary index
    ptablecreate->ulPages = max( cSecondaryIndexes+1, ptablecreate->ulPages );

    cbAllocate +=
        cIndexesToCreate *
            (
            sizeof( JET_INDEXCREATE3_A )    // JET_INDEXCREATE3
            + sizeof( JET_SPACEHINTS )
            + cbName                    // index name
            + cbKeyExtended             // index key, plus langid and cbVarSegmac
            + sizeof( JET_UNICODEINDEX2 )
            + sizeof( JET_TUPLELIMITS )
            );

    cbAllocate += cConditionalColumns * ( sizeof( JET_CONDITIONALCOLUMN_A ) + cbKeySegment );

    cbAllocate += cbDefaultRecRemaining;        // all default values must fit in an intrinsic record

    const ULONG cbLocaleName = sizeof( WCHAR ) * NORM_LOCALE_NAME_MAX_LENGTH;

    cbAllocate += ( cIndexesToCreate * cbLocaleName );

    // WARNING: To ensure that columnids and JET_COLUMN/INDEXCREATE
    // structs are 4-byte aligned, arrange everything in the following
    // order:
    //      1) list of source table columnids
    //      2) JET_COLUMNCREATE_A structures
    //      3) JET_INDEXCREATE structures
    //      4) JET_UNICODEINDEX structures
    //      5) JET_TUPLELIMIT structures
    //      6) JET_CONDITIONALCOLUMN structures
    //      7) buffer for column names
    //      8) buffer for index names
    //      9) buffer for index keys
    //      10) buffer for default values
    //      11) buffer for locale names


    // Can we use the buffer hanging off pcompactinfo?
    JET_COLUMNID    *pcolumnidSrc;
    if ( cbAllocate <= sizeof( pcompactinfo->rgbBuf ) )
    {
        pcolumnidSrc = (JET_COLUMNID *)pcompactinfo->rgbBuf;
    }
    else
    {
        AllocR( pcolumnidSrc = static_cast<JET_COLUMNID *>( PvOSMemoryHeapAlloc( cbAllocate ) ) );
        fLocalAlloc = fTrue;
    }

    JET_COLUMNID    * const rgcolumnidSrc = pcolumnidSrc;
    BYTE            * const pbMax = (BYTE *)rgcolumnidSrc + cbAllocate;
    memset( (BYTE *)pcolumnidSrc, 0, cbAllocate );

    // JET_COLUMNCREATE_A structures follow the tagged columnid map.
    JET_COLUMNCREATE_A *pcolcreateCurr      = (JET_COLUMNCREATE_A *)( (BYTE *)rgcolumnidSrc + cbColumnids );
    JET_COLUMNCREATE_A * const rgcolcreate  = pcolcreateCurr;
    Assert( (BYTE *)rgcolcreate < pbMax );

    // JET_INDEXCREATE structures follow the JET_COLUMNCREATE_A structures
    JET_INDEXCREATE3_A  *pidxcreateCurr = (JET_INDEXCREATE3_A *)( rgcolcreate + cColumns );
    JET_INDEXCREATE3_A  * const rgidxcreate = pidxcreateCurr;
    Assert( (BYTE *)rgidxcreate < pbMax );

    // JET_SPACEHINTS structures follow the JET_INDEXCREATE structures
    JET_SPACEHINTS * psphintsCurr = (JET_SPACEHINTS *)( rgidxcreate + cIndexesToCreate );
    JET_SPACEHINTS * const rgsphints = psphintsCurr;
    Assert( (BYTE *)psphintsCurr < pbMax );

    //  JET_UNICODEINDEX structures follow the JET_SPACEHINTS structures
    JET_UNICODEINDEX2   *pidxunicodeCurr        = (JET_UNICODEINDEX2 *)( rgsphints + cIndexesToCreate );
    JET_UNICODEINDEX2   * const rgidxunicode    = pidxunicodeCurr;
    Assert( (BYTE *)rgidxunicode < pbMax );

    //  JET_TUPLELIMITS structures follow the JET_UNICODEINDEX structures
    JET_TUPLELIMITS     *ptuplelimitsCurr       = (JET_TUPLELIMITS *)( rgidxunicode + cIndexesToCreate );
    JET_TUPLELIMITS     * const rgtuplelimits   = ptuplelimitsCurr;
    Assert( (BYTE *)rgtuplelimits < pbMax );

    // JET_CONDITIONALCOLUMN structures follow the JET_TUPLELIMITS structures
    JET_CONDITIONALCOLUMN_A *pconditionalcolumnCurr     = (JET_CONDITIONALCOLUMN_A *)( rgtuplelimits + cIndexesToCreate  );
    JET_CONDITIONALCOLUMN_A * const rgconditionalcolumn = pconditionalcolumnCurr;
    Assert( (BYTE *)rgconditionalcolumn < pbMax );

    // Column names follow the JET_CONDITIONALCOLUMN structures.
    CHAR    *szCurrColumn = (CHAR *)( rgconditionalcolumn + cConditionalColumns );
    CHAR    * const rgszColumns = szCurrColumn;
    Assert( (BYTE *)rgszColumns < pbMax );

    // Index names follow the column names.
    CHAR    *szCurrIndex = (CHAR *)( rgszColumns + ( cColumns * cbName ) );
    CHAR    * const rgszIndexes = szCurrIndex;
    Assert( (BYTE *)rgszIndexes < pbMax );
    ULONG cbIndexNamesLeft = cIndexesToCreate * cbName;

    // Index/Conditional Column keys follow the index names.
    CHAR    *szCurrKey = ( CHAR *)( rgszIndexes + ( cbIndexNamesLeft ) );
    CHAR    * const rgszKeys = szCurrKey;
    Assert( (BYTE *)rgszKeys < pbMax );
    ULONG cbKeysLeft = ( cIndexesToCreate * cbKeyExtended) + ( cConditionalColumns * cbKeySegment );

    // Default values follow the keys.
    BYTE    *pbCurrDefault = (BYTE *)( rgszKeys + cbKeysLeft );
    BYTE    * const rgbDefaultValues = pbCurrDefault;
    Assert( rgbDefaultValues < pbMax );
    ULONG cbDefaultValuesLeft = cbDefaultRecRemaining;

    // Locale Names follow the default values.
    WCHAR   *wszCurLocaleName = ( WCHAR *)( rgbDefaultValues + cbDefaultValuesLeft );
    WCHAR   * const rgwszLocaleName = wszCurLocaleName;
    Assert( (BYTE *)rgwszLocaleName <= pbMax );
    ULONG cbLocaleNames = ( cIndexesToCreate * cbLocaleName );

    Assert( (BYTE *)rgwszLocaleName + cbLocaleNames == pbMax );

    err = ErrDispMove(
                reinterpret_cast<JET_SESID>( ppib ),
                pcolumnList->tableid,
                JET_MoveFirst,
                NO_GRBIT );

    /* loop though all the columns in the table for the src tbl and
    /* copy the information in the destination database
    /**/
    cColumns = 0;
    while ( err >= 0 )
    {
        memset( pcolcreateCurr, 0, sizeof( JET_COLUMNCREATE_A ) );
        pcolcreateCurr->cbStruct = sizeof(JET_COLUMNCREATE_A);

        /* retrieve info from table and create all the columns
        /**/
        Assert( (BYTE *)szCurrColumn + JET_cbNameMost + 1 <= (BYTE *)rgszIndexes );
        Call( ErrDispRetrieveColumn(
                    reinterpret_cast<JET_SESID>( ppib ),
                    pcolumnList->tableid,
                    pcolumnList->columnidcolumnname,
                    szCurrColumn,
                    JET_cbNameMost,
                    &cbActual,
                    NO_GRBIT,
                    NULL ) );

        Assert( cbActual <= JET_cbNameMost );
        szCurrColumn[cbActual] = '\0';
        pcolcreateCurr->szColumnName = szCurrColumn;

        szCurrColumn += cbActual + 1;
        Assert( (BYTE *)szCurrColumn <= (BYTE *)rgszIndexes );

#ifdef DEBUG
        // Assert Presentation order no longer supported.
        ULONG   ulPOrder;
        Call( ErrDispRetrieveColumn(
                    reinterpret_cast<JET_SESID>( ppib ),
                    pcolumnList->tableid,
                    pcolumnList->columnidPresentationOrder,
                    &ulPOrder,
                    sizeof(ulPOrder),
                    &cbActual,
                    NO_GRBIT,
                    NULL ) );
        Assert( JET_wrnColumnNull == err );
#endif

        Call( ErrDispRetrieveColumn(
                    reinterpret_cast<JET_SESID>( ppib ),
                    pcolumnList->tableid,
                    pcolumnList->columnidcoltyp,
                    &pcolcreateCurr->coltyp,
                    sizeof( pcolcreateCurr->coltyp ),
                    &cbActual,
                    NO_GRBIT,
                    NULL ) );
        Assert( cbActual == sizeof( JET_COLTYP ) );
        Assert( JET_coltypNil != pcolcreateCurr->coltyp );

        Call( ErrDispRetrieveColumn(
                    reinterpret_cast<JET_SESID>( ppib ),
                    pcolumnList->tableid,
                    pcolumnList->columnidcbMax,
                    &pcolcreateCurr->cbMax,
                    sizeof( pcolcreateCurr->cbMax ),
                    &cbActual,
                    NO_GRBIT,
                    NULL ) );
        Assert( cbActual == sizeof( ULONG ) );

        Call( ErrDispRetrieveColumn(
                    reinterpret_cast<JET_SESID>( ppib ),
                    pcolumnList->tableid,
                    pcolumnList->columnidgrbit,
                    &pcolcreateCurr->grbit,
                    sizeof( pcolcreateCurr->grbit ),
                    &cbActual,
                    NO_GRBIT,
                    NULL ) );
        Assert( cbActual == sizeof( JET_GRBIT ) );

        Call( ErrDispRetrieveColumn(
                    reinterpret_cast<JET_SESID>( ppib ),
                    pcolumnList->tableid,
                    pcolumnList->columnidCp,
                    &pcolcreateCurr->cp,
                    sizeof( pcolcreateCurr->cp ),
                    &cbActual,
                    NO_GRBIT,
                    NULL ) );
        Assert( cbActual == sizeof( USHORT ) );

        /*  retrieve default value.
        /**/
        if( pcolcreateCurr->grbit & JET_bitColumnUserDefinedDefault )
        {
            JET_USERDEFINEDDEFAULT_A * pudd = NULL;

            //  don't want to pass in NULL
            BYTE b;
            Call( ErrDispRetrieveColumn(
                        reinterpret_cast<JET_SESID>( ppib ),
                        pcolumnList->tableid,
                        pcolumnList->columnidDefault,
                        &b,
                        sizeof( b ),
                        &pcolcreateCurr->cbDefault,
                        NO_GRBIT,
                        NULL ) );

            Alloc( pcolcreateCurr->pvDefault = cmemlist.PvAlloc( pcolcreateCurr->cbDefault ) );

            Call( ErrDispRetrieveColumn(
                        reinterpret_cast<JET_SESID>( ppib ),
                        pcolumnList->tableid,
                        pcolumnList->columnidDefault,
                        pcolcreateCurr->pvDefault,
                        pcolcreateCurr->cbDefault,
                        &pcolcreateCurr->cbDefault,
                        NO_GRBIT,
                        NULL ) );
            Assert( JET_wrnBufferTruncated != err );
            Assert( JET_wrnColumnNull != err );
            Assert( pcolcreateCurr->cbDefault > 0 );

            //  All of the information about a user-defined default is in the default buffer
            //  ErrINFOGetTableColumnInfo lays it out like this:
            //
            //  JET_USERDEFINEDDEFAULT | szCallback | pbUserData | szDependantColumns
            //
            //  The pointers in the JET_USERDEFINEDDEFAULT are no longer usable so they have
            //  to be fixed up. The cbDefault has to be reduced to sizeof( JET_USERDEFINEDDEFAULT )
            //  because that is what the JET APIs are expecting

            pudd = (JET_USERDEFINEDDEFAULT_A *)pcolcreateCurr->pvDefault;
            pudd->szCallback = ((CHAR*)(pcolcreateCurr->pvDefault)) + sizeof( JET_USERDEFINEDDEFAULT_A );
            pudd->pbUserData = ((BYTE*)(pudd->szCallback)) + strlen( pudd->szCallback ) + 1;
            if( NULL != pudd->szDependantColumns )
            {
                pudd->szDependantColumns = (CHAR *)pudd->pbUserData + pudd->cbUserData;
            }

            //  in order to create the column the pvDefault should point to the JET_USERDEFINEDDEFAULT structure
            Assert( pcolcreateCurr->cbDefault > sizeof( JET_USERDEFINEDDEFAULT_A ) );
            pcolcreateCurr->cbDefault = sizeof( JET_USERDEFINEDDEFAULT_A );
        }
        else
        {
            Assert( cbDefaultRecRemaining > 0 );        // can never reach cbDefaultRecRemaining, because of record overhead
            Assert( pbCurrDefault + cbDefaultRecRemaining == (BYTE *)rgwszLocaleName );
            Call( ErrDispRetrieveColumn(
                        reinterpret_cast<JET_SESID>( ppib ),
                        pcolumnList->tableid,
                        pcolumnList->columnidDefault,
                        pbCurrDefault,
                        cbDefaultRecRemaining,
                        &pcolcreateCurr->cbDefault,
                        NO_GRBIT,
                        NULL ) );
            Assert( JET_wrnBufferTruncated != err );
            Assert( pcolcreateCurr->cbDefault < cbDefaultRecRemaining );    // can never reach cbDefaultRecRemaining, because of record overhead
            pcolcreateCurr->pvDefault = pbCurrDefault;
            pbCurrDefault += pcolcreateCurr->cbDefault;
            cbDefaultRecRemaining -= pcolcreateCurr->cbDefault;
            Assert( cbDefaultRecRemaining > 0 );        // can never reach cbDefaultRecRemaining, because of record overhead
            Assert( pbCurrDefault + cbDefaultRecRemaining == (BYTE *)rgwszLocaleName );
        }

        // Save the source columnid.
        /* CONSIDER: Should the column id be checked? */
        Call( ErrDispRetrieveColumn(
                    reinterpret_cast<JET_SESID>( ppib ),
                    pcolumnList->tableid,
                    pcolumnList->columnidcolumnid,
                    pcolumnidSrc,
                    sizeof( JET_COLUMNID ),
                    &cbActual,
                    NO_GRBIT,
                    NULL ) );
        Assert( cbActual == sizeof( JET_COLUMNID ) );

        if ( pcolcreateCurr->grbit & JET_bitColumnTagged )
        {
            cTagged++;
            fidTaggedHighest = max( fidTaggedHighest, FidOfColumnid( *pcolumnidSrc ) );
        }

        pcolumnidSrc++;
        Assert( (BYTE *)pcolumnidSrc <= (BYTE *)rgcolcreate );

        pcolcreateCurr++;
        Assert( (BYTE *)pcolcreateCurr <= (BYTE *)rgidxcreate );
        cColumns++;

        err = ErrDispMove(
                    reinterpret_cast<JET_SESID>( ppib ),
                    pcolumnList->tableid,
                    JET_MoveNext,
                    NO_GRBIT );
    }

    Assert( cColumns == pcolumnList->cRecord );

    Assert( ptablecreate->rgcolumncreate == NULL );
    Assert( ptablecreate->cColumns == 0 );
    ptablecreate->rgcolumncreate = rgcolcreate;
    ptablecreate->cColumns = cColumns;


    Assert( ptablecreate->rgindexcreate == NULL );
    Assert( ptablecreate->cIndexes == 0 );

    for ( pfcbIndex = pfcbSrc;
        pfcbIndex != pfcbNil;
        pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        if ( pfcbIndex->Pidb() != pidbNil )
        {
            // Derived indexes will get inherited from template.
            if ( !pfcbIndex->FDerivedIndex() )
            {
                Assert( (BYTE *)pidxcreateCurr < (BYTE *)rgsphints );
                Assert( (BYTE *)psphintsCurr < (BYTE *)rgidxunicode );
                Assert( (BYTE *)pidxunicodeCurr < (BYTE *)rgtuplelimits );
                Assert( (BYTE *)ptuplelimitsCurr < (BYTE *)rgconditionalcolumn );
                Assert( ( cConditionalColumns > 0 && (BYTE *)pconditionalcolumnCurr <= (BYTE *)rgszColumns )
                    || ( 0 == cConditionalColumns && (BYTE *)pconditionalcolumnCurr == (BYTE *)rgszColumns ) );
                Assert( (BYTE *)szCurrIndex < (BYTE *)rgszKeys );
                Assert( (BYTE *)szCurrKey < (BYTE *)rgbDefaultValues );

                memset( pidxcreateCurr, 0, sizeof( JET_INDEXCREATE3_A ) );

                pidxcreateCurr->cbStruct    = sizeof(JET_INDEXCREATE3_A);
                pidxcreateCurr->szIndexName = szCurrIndex;
                pidxcreateCurr->szKey       = szCurrKey;
                pidxcreateCurr->pidxunicode = pidxunicodeCurr;
                pidxunicodeCurr->szLocaleName = wszCurLocaleName;
                pidxcreateCurr->pSpacehints = psphintsCurr;

                CMPCopyOneIndex(
                    pfcbSrc,
                    pfcbIndex,
                    pidxcreateCurr,
                    min( cbName, cbIndexNamesLeft ),
                    cbKeysLeft, // better is min() something, but can't work that out
                    ptuplelimitsCurr,
                    pconditionalcolumnCurr,
                    psphintsCurr );

                ptablecreate->cIndexes++;

                szCurrIndex += strlen( pidxcreateCurr->szIndexName ) + 1;
                cbIndexNamesLeft -= strlen( pidxcreateCurr->szIndexName ) + 1;
                Assert( (BYTE *)szCurrIndex <= (BYTE *)rgszKeys );
                Assert( ((BYTE *)szCurrIndex+cbIndexNamesLeft) <= (BYTE *)rgszKeys );

                szCurrKey += pidxcreateCurr->cbKey;
                cbKeysLeft -= pidxcreateCurr->cbKey;
                Assert( (BYTE *)szCurrKey <= (BYTE *)rgbDefaultValues );
                Assert( ((BYTE *)szCurrKey+cbKeysLeft) <= (BYTE *)rgbDefaultValues );

                if ( 0 != pidxcreateCurr->cConditionalColumn )
                {
                    Assert( pidxcreateCurr->rgconditionalcolumn == pconditionalcolumnCurr );
                    Assert( NULL != pconditionalcolumnCurr->szColumnName );
                    Assert( pidxcreateCurr->cConditionalColumn > 0 );

                    for( size_t iConditionalColumn = 0; iConditionalColumn < pidxcreateCurr->cConditionalColumn; ++iConditionalColumn )
                    {
                        szCurrKey += strlen( pidxcreateCurr->rgconditionalcolumn[iConditionalColumn].szColumnName ) + 1;
                        cbKeysLeft -= strlen( pidxcreateCurr->rgconditionalcolumn[iConditionalColumn].szColumnName ) + 1;
                    }
                    pconditionalcolumnCurr += pidxcreateCurr->cConditionalColumn;
                }

                pidxcreateCurr++;
                psphintsCurr++;
                pidxunicodeCurr++;
                ptuplelimitsCurr++;
                wszCurLocaleName += ( cbLocaleName / sizeof( WCHAR ) );
                Assert( (BYTE *)pidxcreateCurr <= (BYTE *)rgsphints );
                Assert( (BYTE *)psphintsCurr <= (BYTE *)rgidxunicode );
                Assert( (BYTE *)pidxunicodeCurr <= (BYTE *)rgtuplelimits );
                Assert( (BYTE *)ptuplelimitsCurr <= (BYTE *)rgconditionalcolumn );
            }
        }
        else
        {
            // If IDB is null, must be sequential index.
            Assert( pfcbIndex == pfcbSrc );
            Assert( pfcbIndex->FSequentialIndex() );
        }
    }

    Assert( (BYTE *)pidxcreateCurr == (BYTE *)rgsphints );
    Assert( (BYTE *)psphintsCurr == (BYTE *)rgidxunicode );
    Assert( (BYTE *)pidxunicodeCurr == (BYTE *)rgtuplelimits );
    Assert( (BYTE *)ptuplelimitsCurr == (BYTE *)rgconditionalcolumn );
    Assert( ptablecreate->cIndexes == cIndexesToCreate );

    ptablecreate->rgindexcreate = rgidxcreate;

    Call( ErrFILECreateTable(
                ppib,
                pcompactinfo->ifmpDest,
                ptablecreate ) );
    Assert( ptablecreate->cCreated == 1 + cColumns + cIndexesToCreate );


    // If there's at least one tagged column, create an array for the
    // tagged columnid map.
    if ( cTagged > 0 )
    {
        Assert( FTaggedFid( fidTaggedHighest ) );
        cbAllocate = sizeof(JET_COLUMNID) * ( fidTaggedHighest + 1 - fidTaggedLeast );
        Alloc( mpcolumnidcolumnidTagged = static_cast<JET_COLUMNID *>( PvOSMemoryHeapAlloc( cbAllocate ) ) );
        memset( (BYTE *)mpcolumnidcolumnidTagged, 0, cbAllocate );
    }


    // Update columnid maps.
    for ( pcolcreateCurr = rgcolcreate, pcolumnidSrc = rgcolumnidSrc, cColumns = 0;
        cColumns < pcolumnList->cRecord;
        pcolcreateCurr++, pcolumnidSrc++, cColumns++ )
    {
        Assert( (BYTE *)pcolcreateCurr <= (BYTE *)rgidxcreate );
        Assert( (BYTE *)pcolumnidSrc <= (BYTE *)rgcolcreate );

        if ( FCOLUMNIDTemplateColumn( *pcolumnidSrc ) )
        {
            Assert( ptablecreate->grbit & JET_bitTableCreateTemplateTable );
            Assert( FCOLUMNIDTemplateColumn( pcolcreateCurr->columnid ) );
        }
        else
        {
            Assert( !( ptablecreate->grbit & JET_bitTableCreateTemplateTable ) );
            Assert( !( ptablecreate->grbit & JET_bitTableCreateNoFixedVarColumnsInDerivedTables ) );
            Assert( !FCOLUMNIDTemplateColumn( pcolcreateCurr->columnid ) );
        }

        if ( pcolcreateCurr->grbit & JET_bitColumnTagged )
        {
            Assert( FCOLUMNIDTagged( *pcolumnidSrc ) );
            Assert( FCOLUMNIDTagged( pcolcreateCurr->columnid ) );
            Assert( FidOfColumnid( *pcolumnidSrc ) <= fidTaggedHighest );
            Assert( mpcolumnidcolumnidTagged != NULL );
            Assert( mpcolumnidcolumnidTagged[FidOfColumnid( *pcolumnidSrc ) - fidTaggedLeast] == 0 );
            mpcolumnidcolumnidTagged[FidOfColumnid( *pcolumnidSrc ) - fidTaggedLeast] = pcolcreateCurr->columnid;
        }
        else
        {
            /*  else add the columnids to the columnid array
            /**/
            columnidInfo[ccolSingleValue].columnidDest = pcolcreateCurr->columnid;
            columnidInfo[ccolSingleValue].columnidSrc  = *pcolumnidSrc;
            ccolSingleValue++;
        }   // if ( columndef.grbit & JET_bitColumnTagged )
    }

    /*  set count of fixed and variable columns to copy
    /**/
    pcompactinfo->ccolSingleValue = ccolSingleValue;

    if ( err == JET_errNoCurrentRecord )
        err = JET_errSuccess;

HandleError:
    if ( err < 0  &&  mpcolumnidcolumnidTagged )
    {
        OSMemoryHeapFree( mpcolumnidcolumnidTagged );
        mpcolumnidcolumnidTagged = NULL;
    }

    if ( fLocalAlloc )
    {
        OSMemoryHeapFree( rgcolumnidSrc );
    }

    cmemlist.FreeAllMemory();

    // Set return value.
    *pmpcolumnidcolumnidTagged = mpcolumnidcolumnidTagged;

    return err;
}

LOCAL ERR ErrCMPCopyTable(
    COMPACTINFO     *pcompactinfo,
    const CHAR      *szObjectName,
    ULONG           ulFlags )
{
    ERR             err;
    PIB             *ppib = pcompactinfo->ppib;
    INST            *pinst = PinstFromPpib( ppib );
    IFMP            ifmpSrc = pcompactinfo->ifmpSrc;
    FUCB            *pfucbSrc = pfucbNil;
    FUCB            *pfucbDest = pfucbNil;
    FCB             *pfcbSrc;
    CPG             cpgTableSrc;
    const CPG       cpgDbExtensionSizeSave = (CPG)UlParam( pinst, JET_paramDbExtensionSize );
    JET_COLUMNLIST  columnList;
    JET_COLUMNID    *mpcolumnidcolumnidTagged = NULL;
    STATUSINFO      *pstatus = pcompactinfo->pstatus;
    ULONG           crowCopied = 0;
    QWORD           qwAutoIncRecMax;
    ULONG           rgulAllocInfo[] = { ulCMPDefaultPages, ulCMPDefaultDensity };
    CHAR            *szTemplateTableName = NULL;
    BOOL            fCorruption = fFalse;
    JET_TABLECREATE5_A  tablecreate = {
                        sizeof(JET_TABLECREATE5_A),
                        (CHAR *)szObjectName,
                        NULL,                   // Template table
                        ulCMPDefaultPages,
                        ulCMPDefaultDensity,
                        NULL, 0,                // Columns
                        NULL, 0,                // Indexes
                        NULL, 0,                // Callbacks
                        NO_GRBIT,
                        NULL,
                        NULL,
                        0,
                        0,
                        JET_TABLEID( pfucbNil ),
                        0
                    };

    if ( pstatus  &&  pstatus->fDumpStats )
    {
        Assert( pstatus->hfCompactStats );
        fprintf( pstatus->hfCompactStats, "%s\t", szObjectName );
        fflush( pstatus->hfCompactStats );
        CMPSetTime( &pstatus->timerCopyTable );
        CMPSetTime( &pstatus->timerInitTable );
    }

    CallR( ErrFILEOpenTable(
                ppib,
                ifmpSrc,
                &pfucbSrc,
                szObjectName,
                JET_bitTableSequential ) );
    Assert( pfucbNil != pfucbSrc );

    err = ErrIsamGetTableInfo(
                reinterpret_cast<JET_SESID>( ppib ),
                reinterpret_cast<JET_TABLEID>( pfucbSrc ),
                rgulAllocInfo,
                sizeof(rgulAllocInfo),
                JET_TblInfoSpaceAlloc );
    if ( err < 0  &&  !g_fRepair )
    {
        goto HandleError;
    }

    // On error, just use the default values of rgulAllocInfo.
    tablecreate.ulPages = rgulAllocInfo[0];

    // Set the LV chunk size to match original table
    tablecreate.cbLVChunkMax = pfucbSrc->u.pfcb->Ptdb()->CbLVChunkMost();

    /*  if a table create the columns in the Dest Db the same as in
    /*  the src Db.
    /**/
    Assert( !( ulFlags & JET_bitObjectSystem ) );
    if ( ulFlags & JET_bitObjectTableTemplate )
    {
        Assert( ulFlags & JET_bitObjectTableFixedDDL );
        Assert( !( ulFlags & JET_bitObjectTableDerived ) );
        tablecreate.grbit = ( JET_bitTableCreateTemplateTable | JET_bitTableCreateFixedDDL );

        if ( ulFlags & JET_bitObjectTableNoFixedVarColumnsInDerivedTables )
            tablecreate.grbit |= JET_bitTableCreateNoFixedVarColumnsInDerivedTables;
    }
    else
    {
        Assert( !( ulFlags & JET_bitObjectTableNoFixedVarColumnsInDerivedTables ) );
        if ( ulFlags & JET_bitObjectTableFixedDDL )
        {
            tablecreate.grbit = JET_bitTableCreateFixedDDL;
        }
        if ( ulFlags & JET_bitObjectTableDerived )
        {
            Alloc( szTemplateTableName = reinterpret_cast<CHAR *>( PvOSMemoryHeapAlloc( JET_cbNameMost + 1 ) ) );

            // extract name.
            Call( ErrIsamGetTableInfo(
                        reinterpret_cast<JET_SESID>( ppib ),
                        reinterpret_cast<JET_TABLEID>( pfucbSrc ),
                        szTemplateTableName,
                        JET_cbNameMost+1,
                        JET_TblInfoTemplateTableName ) );

            Assert( strlen( szTemplateTableName ) > 0 );
            tablecreate.szTemplateTableName = szTemplateTableName;
        }
    }


    pfcbSrc = pfucbSrc->u.pfcb;
    Assert( pfcbNil != pfcbSrc );
    Assert( pfcbSrc->FTypeTable() );

    /*  get a table with the column information for the query in it
    /**/
    Call( ErrIsamGetTableColumnInfo(
                reinterpret_cast<JET_SESID>( ppib ),
                reinterpret_cast<JET_TABLEID>( pfucbSrc ),
                NULL,
                NULL,
                &columnList,
                sizeof(columnList),
                JET_ColInfoList|JET_ColInfoGrbitCompacting,
                fFalse ) );

    QWORD qwAutoIncExtHdrMax = 0;
    if ( pfucbSrc->u.pfcb->Ptdb()->FidAutoincrement() )
    {
        Call( ErrCMPRECGetAutoInc( pfucbSrc, &qwAutoIncExtHdrMax ) );
    }

    err = ErrCMPCreateTableColumnIndex(
                pcompactinfo,
                pfcbSrc,
                &tablecreate,
                &columnList,
                pcompactinfo->rgcolumnids,
                &mpcolumnidcolumnidTagged );

    // Must use dispatch layer for temp/sort table.
    CallS( ErrDispCloseTable(
                    reinterpret_cast<JET_SESID>( ppib ),
                    columnList.tableid ) );

    // Act on error code returned from CreateTableColumnIndex().
    Call( err );

    pfucbDest = reinterpret_cast<FUCB *>( tablecreate.tableid );
    Assert( tablecreate.cCreated == 1 + tablecreate.cColumns + tablecreate.cIndexes + ( tablecreate.szCallback ? 1 : 0 ) );
    Assert( tablecreate.cColumns >= pcompactinfo->ccolSingleValue );

    if ( pstatus )
    {
        ULONG   rgcpgExtent[2];     // OwnExt and AvailExt

        Assert( pstatus->pfnStatus );
        Assert( pstatus->snt == JET_sntProgress );

        // tablecreate.cIndexes is only a count of the indexes that were created.  We
        // also need the number of indexes inherited.
        FCB *pfcbIndex = pfucbDest->u.pfcb;
        Assert( pfcbIndex->FPrimaryIndex() );
        Assert( ( pfcbIndex->FSequentialIndex() && pfcbIndex->Pidb() == pidbNil )
            || ( !pfcbIndex->FSequentialIndex() && pfcbIndex->Pidb() != pidbNil ) );
        INT cSecondaryIndexes = 0;
        for ( pfcbIndex = pfcbIndex->PfcbNextIndex();
            pfcbIndex != pfcbNil;
            pfcbIndex = pfcbIndex->PfcbNextIndex() )
        {
            Assert( pfcbIndex->FTypeSecondaryIndex() );
            Assert( pfcbIndex->Pidb() != pidbNil );
            cSecondaryIndexes++;
        }

        pstatus->szTableName = (char *)szObjectName;
        pstatus->cTableFixedVarColumns = pcompactinfo->ccolSingleValue;
        pstatus->cTableTaggedColumns = tablecreate.cColumns - pcompactinfo->ccolSingleValue;
        pstatus->cTableInitialPages = rgulAllocInfo[0];
        pstatus->cSecondaryIndexes = cSecondaryIndexes;

        err = ErrIsamGetTableInfo(
                    reinterpret_cast<JET_SESID>( ppib ),
                    reinterpret_cast<JET_TABLEID>( pfucbSrc ),
                    rgcpgExtent,
                    sizeof(rgcpgExtent),
                    JET_TblInfoSpaceUsage );
        if ( err < 0 )
        {
            if ( g_fRepair )
            {
                //  if failure in space query then default to
                //  one page owned and no pages available.
                fCorruption = fTrue;
                rgcpgExtent[0] = 1;
                rgcpgExtent[1] = 0;
            }
            else
            {
                goto HandleError;
            }
        }

        // AvailExt always less than OwnExt.
        Assert( rgcpgExtent[1] < rgcpgExtent[0] );

        // cunitProjected is the projected total pages completed once
        // this table has been copied.
        pstatus->cunitProjected = pstatus->cunitDone + rgcpgExtent[0];
        if ( pstatus->cunitProjected > pstatus->cunitTotal )
        {
            Assert( g_fRepair );
            fCorruption = fTrue;
            pstatus->cunitProjected = pstatus->cunitTotal;
        }

        const ULONG cpgUsed = rgcpgExtent[0] - rgcpgExtent[1];
        Assert( cpgUsed > 0 );

        pstatus->cbRawData = 0;
        pstatus->cbRawDataLV = 0;
        pstatus->cLeafPagesTraversed = 0;
        pstatus->cLVPagesTraversed = 0;

        // If corrupt, suppress progression of meter.
        pstatus->cunitPerProgression =
            ( fCorruption ? 0 : 1 + ( rgcpgExtent[1] / cpgUsed ) );
        pstatus->cTablePagesOwned = rgcpgExtent[0];
        pstatus->cTablePagesAvail = rgcpgExtent[1];

        if ( pstatus->fDumpStats )
        {
            Assert( pstatus->hfCompactStats );

            const TDB   * const ptdbT = pfucbDest->u.pfcb->Ptdb();
            Assert( ptdbNil != ptdbT );
            const INT   cColumns = ( ptdbT->FidFixedLast() + 1 - fidFixedLeast )
                                    + ( ptdbT->FidVarLast() + 1 - fidVarLeast )
                                    + ( ptdbT->FidTaggedLast() + 1 - fidTaggedLeast );

            if ( cColumns > INT( pstatus->cTableFixedVarColumns + pstatus->cTableTaggedColumns ) )
            {
                Assert( pfucbDest->u.pfcb->FDerivedTable() );
            }
            else
            {
                Assert( cColumns == INT( pstatus->cTableFixedVarColumns + pstatus->cTableTaggedColumns ) );
            }

            INT iSec, iMSec;
            CMPGetTime( pstatus->timerInitTable, &iSec, &iMSec );
            fprintf(
                pstatus->hfCompactStats,
                "%d\t%d\t%d\t%d\t%d\t%d\t%d.%d\t",
                pstatus->cTableFixedVarColumns,
                pstatus->cTableTaggedColumns,
                cColumns,
                pstatus->cSecondaryIndexes,
                pstatus->cTablePagesOwned,
                pstatus->cTablePagesAvail,
                iSec, iMSec );
            fflush( pstatus->hfCompactStats );
            CMPSetTime( &pstatus->timerCopyRecords );
        }

        cpgTableSrc = rgcpgExtent[0];
    }
    else
    {
        Call( ErrIsamGetTableInfo(
                    reinterpret_cast<JET_SESID>( ppib ),
                    reinterpret_cast<JET_TABLEID>( pfucbSrc ),
                    &cpgTableSrc,
                    sizeof(cpgTableSrc),
                    JET_TblInfoSpaceOwned ) );
    }

    //  set DB extension size to 1% of the current size of the table, up to
    //  a limit (I somewhat arbitrarily chose the limit to be a number of pages
    //  equal to the page size)
    //
    Call( Param( pinst, JET_paramDbExtensionSize )->Set( pinst, ppibNil, max( cpgDbExtensionSizeSave, (CPG)min( g_rgfmp[ pcompactinfo->ifmpSrc ].CbPage(), cpgTableSrc / 100 ) ), NULL ) );

    Call( ErrSORTCopyRecords(
                ppib,
                pfucbSrc,
                pfucbDest,
                (CPCOL *)pcompactinfo->rgcolumnids,
                pcompactinfo->ccolSingleValue,
                0,
                &crowCopied,
                &qwAutoIncRecMax,
                pcompactinfo->rgbBuf,
                sizeof( pcompactinfo->rgbBuf ),
                mpcolumnidcolumnidTagged,
                pstatus ) );

    if ( pfucbSrc->u.pfcb->Ptdb()->FidAutoincrement() )
    {
        Assert( pfucbDest->u.pfcb->Ptdb()->FidAutoincrement() );

        //  Update the auto-inc max of the table, so that new records will be guranteed to not
        //  conflict (duplicate key exception) with any previous record.
        //
        QWORD qwAutoIncExtHdrMax2 = 0; // pulled from destination after all records are moved.
        Call( ErrCMPRECGetAutoInc( pfucbDest, &qwAutoIncExtHdrMax2 ) );
        CallSx( err, JET_wrnColumnNull ); // null indicates legacy (non-external header) auto-inc storage.
        if ( err == JET_errSuccess )
        {
            //  By maxing in the source's qwAutoIncExtHdrMax we maintain the (new w/ External Header 
            //  based Auto-Inc) contract that any inc value we gave out, we do not re-give out.
            //  This contract is much nicer to clients who maintain replication systems, and often 
            //  get confused if they delete or rollback (and restart) the last auto-inc values and 
            //  then suddenly ESE starts re-using previously given out values.
            QWORD qwAutoIncSet = max( qwAutoIncRecMax, qwAutoIncExtHdrMax );
            //  Note even this could be larger because of the batches of AutoIncs we request, depending
            //  upon how many values we migrated ... 
            Expected( qwAutoIncExtHdrMax == 0 || qwAutoIncExtHdrMax2 <= qwAutoIncExtHdrMax + g_ulAutoIncBatchSize );  // this is just in case we have messed up somehow ... should not happen, but handled.
            Expected( qwAutoIncExtHdrMax2 <= qwAutoIncRecMax + g_ulAutoIncBatchSize );
            //  ... but to be safe, we'll max it in as well.
            qwAutoIncSet = max( qwAutoIncSet, qwAutoIncExtHdrMax2 );


            Call( ErrCMPRECSetAutoInc( pfucbDest, qwAutoIncSet ) );
        }
    }

    if ( err >= 0 )
    {
        //  copy the callbacks from one database to another
        err = ErrCATCopyCallbacks(
                ppib,
                pcompactinfo->ifmpSrc,
                pcompactinfo->ifmpDest,
                pfucbSrc->u.pfcb->ObjidFDP(),
                pfucbDest->u.pfcb->ObjidFDP()
                );
    }

    if ( pstatus )
    {
        if ( pstatus->fDumpStats )
        {
            INT iSec, iMSec;
            Assert( pstatus->hfCompactStats );
            CMPGetTime( pstatus->timerCopyRecords, &iSec, &iMSec );
            fprintf( pstatus->hfCompactStats,
                    "%d\t%I64d\t%I64d\t%d\t%d\t%d.%d\t",
                    crowCopied,
                    pstatus->cbRawData,
                    pstatus->cbRawDataLV,
                    pstatus->cLeafPagesTraversed,
                    pstatus->cLVPagesTraversed,
                    iSec, iMSec );
            fflush( pstatus->hfCompactStats );
        }

        if ( err >= 0 || g_fRepair )
        {
            // Top off progress meter for this table.
            Assert( pstatus->cunitDone <= pstatus->cunitProjected );
            pstatus->cunitDone = pstatus->cunitProjected;
            ERR errT = ErrCMPReportProgress( pstatus );
            if ( err >= 0 )
                err = errT;
        }
    }

HandleError:
    CallS( Param( pinst, JET_paramDbExtensionSize )->Set( pinst, ppibNil, cpgDbExtensionSizeSave, NULL ) );

    if ( pfucbNil != pfucbDest )
    {
        Assert( (JET_TABLEID)pfucbDest == tablecreate.tableid );
        CallS( ErrFILECloseTable( ppib, pfucbDest ) );
    }

    if ( mpcolumnidcolumnidTagged != NULL )
    {
        OSMemoryHeapFree( mpcolumnidcolumnidTagged );
    }

    if ( szTemplateTableName != NULL )
    {
        OSMemoryHeapFree( szTemplateTableName );
    }

    Assert( pfucbNil != pfucbSrc );
    CallS( ErrFILECloseTable( ppib, pfucbSrc ) );

    if ( pstatus  &&  pstatus->fDumpStats )
    {
        INT iSec, iMSec;
        Assert( pstatus->hfCompactStats );
        CMPGetTime( pstatus->timerCopyTable, &iSec, &iMSec );
        fprintf( pstatus->hfCompactStats, "%d.%d\n", iSec, iMSec );
        fflush( pstatus->hfCompactStats );
    }

    return err;
}

LOCAL BOOL FCMPLegacySystemTable( PCSTR szTableName )
{
    return FCATUnicodeFixupTable( szTableName );
}


LOCAL ERR ErrCMPCopySelectedTables(
    COMPACTINFO *pcompactinfo,
    FUCB        *pfucbCatalog,
    const BOOL  fCopyDerivedTablesOnly,
    BOOL        *pfEncounteredDerivedTable )
{
    ERR         err;
    FCB         *pfcbCatalog    = pfucbCatalog->u.pfcb;
    DATA        dataField;
    BOOL        fLatched        = fFalse;
    CHAR        szTableName[JET_cbNameMost+1];

    Assert( pfcbNil != pfcbCatalog );
    Assert( pfcbCatalog->FTypeTable() );
    Assert( pfcbCatalog->FFixedDDL() );
    Assert( pfcbCatalog->PgnoFDP() == pgnoFDPMSO );

    err = ErrIsamMove( pcompactinfo->ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT );
    if ( err < 0 )
    {
        Assert( JET_errRecordNotFound != err );
        if ( JET_errNoCurrentRecord == err )
            err = ErrERRCheck( JET_errDatabaseCorrupted );  // MSysObjects shouldn't be empty.

        return err;
    }

    do
    {
        Assert( !Pcsr( pfucbCatalog )->FLatched() );
        Call( ErrDIRGet( pfucbCatalog ) );
        fLatched = fTrue;

        const DATA& dataRec             = pfucbCatalog->kdfCurr.data;
        BOOL        fProceedWithCopy    = fTrue;
        ULONG       ulFlags;

        Assert( FFixedFid( fidMSO_Flags ) );
        Call( ErrRECIRetrieveFixedColumn(
                        pfcbNil,
                        pfucbCatalog->u.pfcb->Ptdb(),
                        fidMSO_Flags,
                        dataRec,
                        &dataField ) );
        Assert( JET_errSuccess == err );
        Assert( dataField.Cb() == sizeof(ULONG) );
        UtilMemCpy( &ulFlags, dataField.Pv(), sizeof(ULONG) );

        if ( ulFlags & JET_bitObjectTableDerived )
        {
            if ( !fCopyDerivedTablesOnly )
            {
                //  Must defer derived tables to a second pass
                //  (in order to ensure that the base tables are
                //  created first).
                *pfEncounteredDerivedTable = fTrue;
                fProceedWithCopy = fFalse;
            }
        }
        else if ( fCopyDerivedTablesOnly )
        {
            // Only want derived tables.  If this isn't one, skip it.
            fProceedWithCopy = fFalse;
        }


        if ( fProceedWithCopy )
        {

#ifdef DEBUG
            //  verify this is a column
            Assert( FFixedFid( fidMSO_Type ) );
            Call( ErrRECIRetrieveFixedColumn(
                        pfcbNil,
                        pfucbCatalog->u.pfcb->Ptdb(),
                        fidMSO_Type,
                        dataRec,
                        &dataField ) );
            Assert( JET_errSuccess == err );
            Assert( dataField.Cb() == sizeof(SYSOBJ) );
            Assert( sysobjTable == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );
#endif

            Assert( FVarFid( fidMSO_Name ) );
            Call( ErrRECIRetrieveVarColumn(
                            pfcbNil,
                            pfucbCatalog->u.pfcb->Ptdb(),
                            fidMSO_Name,
                            dataRec,
                            &dataField ) );
            Assert( JET_errSuccess == err );

            const INT cbDataField = dataField.Cb();

            if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
            {
                AssertSz( fFalse, "Invalid fidMSO_Name column in catalog." );
                Error( ErrERRCheck( JET_errCatalogCorrupted ) );
            }
            Assert( sizeof( szTableName ) > cbDataField );
            UtilMemCpy( szTableName, dataField.Pv(), cbDataField );
            szTableName[cbDataField] = '\0';

            Assert( Pcsr( pfucbCatalog )->FLatched() );
            CallS( ErrDIRRelease( pfucbCatalog ) );
            fLatched = fFalse;

            if (
                !FCATSystemTable( szTableName )
                && !FCMPLegacySystemTable( szTableName )
                && !FOLDSystemTable( szTableName )
                && !FSCANSystemTable( szTableName )
                && !FCATObjidsTable( szTableName )
                && !FCATLocalesTable( szTableName ) // rebuilt by catalog updates
                && !MSysDBM::FIsSystemTable( szTableName ) )
            {
                err = ErrCMPCopyTable( pcompactinfo, szTableName, ulFlags );
                if ( err < 0 && g_fRepair )
                {
                    CAutoWSZDDL lszTableName;

                    CallS( lszTableName.ErrSet( szTableName ) );
                    const WCHAR *szName     = (WCHAR*)lszTableName;
                    
                    err = JET_errSuccess;
                    UtilReportEvent(
                        eventWarning,
                        REPAIR_CATEGORY,
                        REPAIR_BAD_TABLE,
                        1,
                        &szName );
                }
                Call( err );
            }
        }
        else
        {
            Assert( Pcsr( pfucbCatalog )->FLatched() );
            CallS( ErrDIRRelease( pfucbCatalog ) );
            fLatched = fFalse;
        }

        err = ErrIsamMove( pcompactinfo->ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
    }
    while ( err >= 0 );

    if ( JET_errNoCurrentRecord == err )
        err = JET_errSuccess;

HandleError:
    if ( fLatched )
    {
        //  if still latched at this point, an error must have occurred
        Assert( err < 0 );
        CallS( ErrDIRRelease( pfucbCatalog ) );
    }

    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    return err;
}


INLINE ERR ErrCMPCopyTables( COMPACTINFO *pcompactinfo )
{
    ERR     err;
    FUCB    *pfucbCatalog   = pfucbNil;
    BOOL    fDerivedTables  = fFalse;

    CallR( ErrCATOpen( pcompactinfo->ppib, pcompactinfo->ifmpSrc, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrIsamSetCurrentIndex( pcompactinfo->ppib, pfucbCatalog, szMSORootObjectsIndex ) );

    fDerivedTables = fFalse;
    Call( ErrCMPCopySelectedTables(
                pcompactinfo,
                pfucbCatalog,
                fFalse,
                &fDerivedTables ) );

    if ( fDerivedTables )       // Process derived tables on second pass.
    {
        Call( ErrCMPCopySelectedTables(
                    pcompactinfo,
                    pfucbCatalog,
                    fTrue,
                    NULL ) );
    }

    Call( ErrCATClose( pcompactinfo->ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

HandleError:
    if (pfucbNil != pfucbCatalog)
    {
        CallS( ErrCATClose( pcompactinfo->ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;
    }

    return err;
}


INLINE ERR ErrCMPCloseDB( COMPACTINFO *pcompactinfo, ERR err )
{
    ERR     errCloseSrc;
    ERR     errCloseDest;
    PIB     *ppib = pcompactinfo->ppib;

    errCloseSrc = ErrDBCloseDatabase( ppib, pcompactinfo->ifmpSrc, NO_GRBIT );
    errCloseDest = ErrDBCloseDatabase( ppib, pcompactinfo->ifmpDest, NO_GRBIT );

    if ( err >= 0 )
    {
        if ( errCloseSrc < 0 )
            err = errCloseSrc;
        else if ( errCloseDest < 0 )
            err = errCloseDest;
    }

    return err;
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( CMEMLIST, BasicUnitTest )
{
    CMEMLIST::UnitTest();
}

#endif

CCriticalSection g_critCompact( CLockBasicInfo( CSyncBasicInfo( szCompact ), rankCompact, 0 ) );

ERR ISAMAPI ErrIsamCompact(
    JET_SESID       sesid,
    const WCHAR     *wszDatabaseSrc,
    IFileSystemAPI  *pfsapiDest,
    const WCHAR     *wszDatabaseDest,
    JET_PFNSTATUS   pfnStatus,
    JET_CONVERT_W   *pconvert,
    JET_GRBIT       grbit )
{
    ERR             err                 = JET_errSuccess;
    COMPACTINFO     *pcompactinfo       = NULL;
    BOOL            fDatabasesOpened    = fFalse;
    STATUSINFO      statusinfo          = { 0 };

    ENTERCRITICALSECTION critCompact( &g_critCompact );

    if ( pconvert )
    {
        //  convert was ripped out in ESE98
        return ErrERRCheck( JET_errFeatureNotAvailable );
    }

    AllocR( pcompactinfo = (COMPACTINFO *)PvOSMemoryHeapAlloc( sizeof( COMPACTINFO ) ) );
    memset( pcompactinfo, 0, sizeof( COMPACTINFO ) );

    pcompactinfo->ppib = reinterpret_cast<PIB *>( sesid );
    if ( pcompactinfo->ppib->Level() > 0 )
    {
        err = ErrERRCheck( JET_errInTransaction );
        goto Cleanup;
    }

    if ( NULL != pfnStatus )
    {
        pcompactinfo->pstatus = &statusinfo;
        memset( pcompactinfo->pstatus, 0, sizeof(STATUSINFO) ); //  just in case = { 0 } doesn't work

        pcompactinfo->pstatus->sesid = sesid;
        pcompactinfo->pstatus->pfnStatus = pfnStatus;

        pcompactinfo->pstatus->snp = JET_snpCompact;

        Call( ErrCMPInitProgress(
                    pcompactinfo->pstatus,
                    wszDatabaseSrc,
                    szCompactAction,
                    ( grbit & JET_bitCompactStats ) ? wszCompactStatsFile : NULL ) );
    }

    else
    {
        pcompactinfo->pstatus = NULL;
    }

    /* Open and create the databases */
    Call( ErrCMPOpenDB( pcompactinfo, wszDatabaseSrc, pfsapiDest, wszDatabaseDest ) );
    if ( grbit & JET_bitCompactPreserveOriginal )
    {
        g_rgfmp[ pcompactinfo->ifmpDest ].SetDefragPreserveOriginal();
    }
    else
    {
        g_rgfmp[ pcompactinfo->ifmpDest ].ResetDefragPreserveOriginal();
    }
    fDatabasesOpened = fTrue;

    if ( NULL != pfnStatus )
    {
        Assert( pcompactinfo->pstatus );

        /* Init status meter.  We'll be tracking status by pages processed, */
        err = ErrIsamGetDatabaseInfo(
                    sesid,
                    (JET_DBID) pcompactinfo->ifmpSrc,
                    &pcompactinfo->pstatus->cDBPagesOwned,
                    sizeof(pcompactinfo->pstatus->cDBPagesOwned),
                    JET_DbInfoSpaceOwned );
        if ( err < 0 )
        {
            goto HandleError;
        }

        ULONG cDBPagesShelved = 0;
        err = ErrIsamGetDatabaseInfo(
                    sesid,
                    (JET_DBID) pcompactinfo->ifmpSrc,
                    &cDBPagesShelved,
                    sizeof(cDBPagesShelved),
                    dbInfoSpaceShelved );
        if ( err < 0 )
        {
            goto HandleError;
        }

        err = ErrIsamGetDatabaseInfo(
                    sesid,
                    (JET_DBID) pcompactinfo->ifmpSrc,
                    &pcompactinfo->pstatus->cDBPagesAvail,
                    sizeof(pcompactinfo->pstatus->cDBPagesAvail),
                    JET_DbInfoSpaceAvailable | JET_DbInfoUseCachedResult );
        if ( err < 0 )
        {
            goto HandleError;
        }

        // Don't count unused space in the database;
        Assert( pcompactinfo->pstatus->cDBPagesOwned >= (ULONG)CpgDBDatabaseMinMin() );
        Assert( pcompactinfo->pstatus->cDBPagesAvail < pcompactinfo->pstatus->cDBPagesOwned );

        // Shelved space is not really owned space, but we'll add it to the expected total because
        // there are asserts and other checks that may fail if we are compacting a database with shelved
        // space from table-level leaks.
        pcompactinfo->pstatus->cunitTotal =
            pcompactinfo->pstatus->cDBPagesOwned - pcompactinfo->pstatus->cDBPagesAvail + cDBPagesShelved;

        // Approximate the number of pages used by MSysObjects
        pcompactinfo->pstatus->cunitDone = cpgMSOInitial;
        Assert( pcompactinfo->pstatus->cunitDone <= pcompactinfo->pstatus->cunitTotal );
        pcompactinfo->pstatus->cunitProjected = pcompactinfo->pstatus->cunitTotal;

        Call( ErrCMPReportProgress( pcompactinfo->pstatus ) );

        if ( pcompactinfo->pstatus->fDumpStats )
        {
            INT iSec, iMSec;

            Assert( pcompactinfo->pstatus->hfCompactStats );
            CMPGetTime( pcompactinfo->pstatus->timerInitDB, &iSec, &iMSec );
            fprintf(
                pcompactinfo->pstatus->hfCompactStats,
                "\nNew database created and initialized in %d.%d seconds.\n",
                iSec, iMSec );

            fprintf(
                pcompactinfo->pstatus->hfCompactStats,
                "    (Source database is %I64d bytes and it owns %d pages, of which %d are available.)\n",
                QWORD( pcompactinfo->pstatus->cDBPagesOwned + cpgDBReserved ) * g_rgfmp[ pcompactinfo->ifmpSrc ].CbPage(),
                pcompactinfo->pstatus->cDBPagesOwned,
                pcompactinfo->pstatus->cDBPagesAvail );
            fprintf(
                pcompactinfo->pstatus->hfCompactStats,
                "\n\n%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n\n",
                szCMPSTATSTableName,
                szCMPSTATSFixedVarCols,
                szCMPSTATSTaggedCols,
                szCMPSTATSColumns,
                szCMPSTATSSecondaryIdx,
                szCMPSTATSPagesOwned,
                szCMPSTATSPagesAvail,
                szCMPSTATSInitTime,
                szCMPSTATSRecordsCopied,
                szCMPSTATSRawData,
                szCMPSTATSRawDataLV,
                szCMPSTATSLeafPages,
                szCMPSTATSMinLVPages,
                szCMPSTATSRecordsTime,
                szCMPSTATSTableTime );
            fflush( pcompactinfo->pstatus->hfCompactStats );
        }
    }

    // Temp is needed here: ErrCMPCopyTables->ErrCMPCopySelectedTables->ErrCMPCopyTable->ErrSORTCopyRecords
    Call( ErrPIBOpenTempDatabase ( pcompactinfo->ppib ) );
    
    /* Create and copy all non-container objects */

    Call( ErrCMPCopyTables( pcompactinfo ) );

    if ( pfnStatus )
    {
        Assert( pcompactinfo->pstatus );
        Assert( pcompactinfo->pstatus->cunitDone <= pcompactinfo->pstatus->cunitTotal );
        if ( pcompactinfo->pstatus->fDumpStats )
        {
            fprintf(
                pcompactinfo->pstatus->hfCompactStats,
                "\nDatabase defragmented to %I64d bytes.\n",
                g_rgfmp[pcompactinfo->ifmpDest].CbOwnedFileSize() );

            INST *pinst = PinstFromPpib( (PIB *) sesid );
            fprintf(
                pcompactinfo->pstatus->hfCompactStats,
                "Temp. database used %I64d bytes during defragmentation.\n",
                g_rgfmp[pinst->m_mpdbidifmp[dbidTemp]].CbOwnedFileSize() );
            fflush( pcompactinfo->pstatus->hfCompactStats );
        }
    }



HandleError:
    if ( fDatabasesOpened )
    {
        ERR errT = ErrCMPCloseDB( pcompactinfo, err );
        if ( errT < JET_errSuccess && err >= JET_errSuccess )
            err = errT;

        //  we can't lock both database headers at the same time so we copy the information out of the old before setting the new

        //  update destination header to preserve repair stats for the database
        if ( err >= JET_errSuccess )
        {
            const ULONG ulRepairCount = g_rgfmp[ pcompactinfo->ifmpSrc ].Pdbfilehdr()->le_ulRepairCount;
            g_rgfmp[ pcompactinfo->ifmpDest ].PdbfilehdrUpdateable()->le_ulRepairCount    = ulRepairCount;
            g_rgfmp[ pcompactinfo->ifmpDest ].PdbfilehdrUpdateable()->le_ulRepairCountOld = ulRepairCount;
            const LOGTIME logtimeRepair = g_rgfmp[ pcompactinfo->ifmpSrc ].Pdbfilehdr()->logtimeRepair;
            g_rgfmp[ pcompactinfo->ifmpDest ].PdbfilehdrUpdateable()->logtimeRepair       = logtimeRepair;
        }

        //  update destination header to preserve the incremental reseed stats for the database
        if ( err >= JET_errSuccess )
        {
            const ULONG ulIncrementalReseedCount = g_rgfmp[ pcompactinfo->ifmpSrc ].Pdbfilehdr()->le_ulIncrementalReseedCount;
            g_rgfmp[ pcompactinfo->ifmpDest ].PdbfilehdrUpdateable()->le_ulIncrementalReseedCount     = ulIncrementalReseedCount;
            g_rgfmp[ pcompactinfo->ifmpDest ].PdbfilehdrUpdateable()->le_ulIncrementalReseedCountOld  = ulIncrementalReseedCount;
            const LOGTIME logtimeIncrementalReseed = g_rgfmp[ pcompactinfo->ifmpSrc ].Pdbfilehdr()->logtimeIncrementalReseed;
            g_rgfmp[ pcompactinfo->ifmpDest ].PdbfilehdrUpdateable()->logtimeIncrementalReseed        = logtimeIncrementalReseed;

            const ULONG ulPagePatchCount = g_rgfmp[ pcompactinfo->ifmpSrc ].Pdbfilehdr()->le_ulPagePatchCount;
            g_rgfmp[ pcompactinfo->ifmpDest ].PdbfilehdrUpdateable()->le_ulPagePatchCount             = ulPagePatchCount;
            g_rgfmp[ pcompactinfo->ifmpDest ].PdbfilehdrUpdateable()->le_ulPagePatchCountOld          = ulPagePatchCount;
            const LOGTIME logtimePagePatch = g_rgfmp[ pcompactinfo->ifmpSrc ].Pdbfilehdr()->logtimePagePatch;
            g_rgfmp[ pcompactinfo->ifmpDest ].PdbfilehdrUpdateable()->logtimePagePatch                = logtimePagePatch;
        }
        
        errT = ErrIsamDetachDatabase( sesid, pfsapiDest, wszDatabaseDest );
        if ( errT < JET_errSuccess && err >= JET_errSuccess )
            err = errT;
    }

    if ( NULL != pfnStatus )        // top off status meter
    {
        Assert( pcompactinfo->pstatus );

        ERR errT = ErrCMPTermProgress( pcompactinfo->pstatus, szCompactAction, err );
        if ( errT < JET_errSuccess && err >= JET_errSuccess )
            err = errT;
    }

Cleanup:
    OSMemoryHeapFree( pcompactinfo );
    return err;
}

ERR ErrIsamTrimDatabase(
    _In_ JET_SESID sesid,
    _In_ JET_PCWSTR wszDatabaseName,
    _In_ IFileSystemAPI* pfsapi,
    _In_ CPRINTF * pcprintf,
    _In_ JET_GRBIT grbit )
{
    ERR err = JET_errSuccess;
    BOOL fDatabaseOpened = fFalse;
    IFMP ifmp = ifmpNil;

    PIB* ppib = reinterpret_cast<PIB *>( sesid );

    // Open and create the databases.
    Call( ErrDBOpenDatabase( ppib, wszDatabaseName, &ifmp, JET_bitNil ) );
    fDatabaseOpened = fTrue;

    Call( ErrSPTrimRootAvail( ppib, ifmp, pcprintf ) );

HandleError:
    if ( fDatabaseOpened )
    {
        ERR errT = ErrDBCloseDatabase( ppib, ifmp, JET_bitNil );
        if ( errT < 0 && err >= 0 )
        {
            err = errT;
        }
    }

    return err;
}


