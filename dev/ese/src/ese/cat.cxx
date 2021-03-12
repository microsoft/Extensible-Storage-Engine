// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_cat.cxx"

const OBJID objidFDPMSO                     = 2;
const OBJID objidFDPMSOShadow               = 3;

const ULONG cbCATNormalizedObjid            = 5;    //  header byte + 4-byte objid

//  In WinBlue+1/WpBlue c 2013 we decided to shrink the DB to a semi-minimalistic
//  size and at that time we trimmed the shadow catalog's initial page allocation.
//  But we need the old value for Win7 Create DBs (which were done before we logged
//  all of create DB) b/c we have to reconstruct the exact state that Win7 code
//  would've created for a single CreateDB.
const CPG   cpgMSOShadowInitialLegacy       = 9;    //  original legacy value

const JET_SPACEHINTS g_jsphSystemDefaults [ eJSPHDefaultMax ] =
{
    //              cbStruct,            ulDensity, "cpgInit",    grbit, Maint, pctGro, "cpgMin", "cpgMax"
    { sizeof(JET_SPACEHINTS), ulFILEDefaultDensity,         0, NO_GRBIT,     0,      0,         0,      0 },
    { sizeof(JET_SPACEHINTS), ulFILEDensityMost,            0, NO_GRBIT,     0,      0,         0,      0 },
};

C_ASSERT( eJSPHDefaultMax == _countof( g_jsphSystemDefaults ) );

#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY
#pragma data_seg( "cacheline_aware_data" )
#endif  //  MINIMAL_FUNCTIONALITY
CATHash g_cathash( rankCATHash );
#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY
#pragma data_seg()
#endif  //  MINIMAL_FUNCTIONALITY

LOCAL ERR ErrCATIClearUnicodeFixupFlags(
        IN PIB * const ppib,
        IN const IFMP ifmp );

//  cat.hxx:iMSO_ list + cat.hxx:idataMSOMax and _cat.cxx:rgcdescMSO array
//  are out of sync.
C_ASSERT( idataMSOMax == cColumnsMSO );

//  ================================================================
LOCAL INLINE BOOL FSortIDEquals(
    __in const SORTID * psortID1,
    __in const SORTID * psortID2 )
//  ================================================================
// bg-*, ky-*, mn-*, ru-*, tg-*, uz-Cyrl-*, are all currently {0000004A-57EE-1E5C-00B4-D0000BB1E11E}
// zh-CN_phoneb and zh-SG_phoneb are currently {0000004B-57EE-1E5C-00B4-D0000BB1E11E} - That's new, so no backward compat version
// {00000049-57EE-1E5C-00B4-D0000BB1E11E} used to be ru-* and the others (and is a duplicate of 4A now).
{
    // 00000049-57EE-1E5C-00B4-D0000BB1E11E
    const SORTID sortIDRuOld = { 0x00000049, 0x57EE, 0x1E5C, { 0x00, 0xB4, 0xD0, 0x00, 0x0B, 0xB1, 0xE1, 0x1E } };

    // 0000004A-57EE-1E5C-00B4-D0000BB1E11E
    const SORTID sortIDRuNew = { 0x0000004A, 0x57EE, 0x1E5C, { 0x00, 0xB4, 0xD0, 0x00, 0x0B, 0xB1, 0xE1, 0x1E } };

    if ( 0 == memcmp( psortID1, psortID2, sizeof( SORTID ) ) )
    {
        return fTrue;
    }

    else if ( 0 == memcmp( psortID1, &sortIDRuOld, sizeof( SORTID ) )
        || 0 == memcmp( psortID1, &sortIDRuNew, sizeof( SORTID ) ) )
{
        return ( 0 == memcmp( psortID2, &sortIDRuOld, sizeof( SORTID ) )
            || 0 == memcmp( psortID2, &sortIDRuNew, sizeof( SORTID ) ) );
}

    return fFalse;
}

//  ================================================================
//  ================================================================
INLINE VOID WszCATFormatSortID(
    __in const SORTID & sortID,
    __out_ecount( cch ) WCHAR * wsz,
    __in INT cch )
//  ================================================================
//
//  Encodes a SORTID as a string.
//
//-
{
    Assert( cch >= PERSISTED_SORTID_MAX_LENGTH );

    OSStrCbFormatW(
        wsz,
        cch * sizeof( wsz[ 0 ] ),
        L"%08lx-%04hx-%04hx-%02hx%02hx-%02hx%02hx%02hx%02hx%02hx%02hx",
        sortID.Data1,
        sortID.Data2,
        sortID.Data3,
        (USHORT)sortID.Data4[0],
        (USHORT)sortID.Data4[1],
        (USHORT)sortID.Data4[2],
        (USHORT)sortID.Data4[3],
        (USHORT)sortID.Data4[4],
        (USHORT)sortID.Data4[5],
        (USHORT)sortID.Data4[6],
        (USHORT)sortID.Data4[7] );

        Assert( LOSStrLengthW( wsz ) < PERSISTED_SORTID_MAX_LENGTH );
}

//  ================================================================
LOCAL INLINE VOID SortIDWsz(
    __in_z WCHAR * wsz,
    __out SORTID * psortID )
//  ================================================================
//
//  Decodes a SORTID from a string.
//
//-
{
    ULONG p0;
    USHORT p1, p2;
    USHORT p3, p4, p5, p6, p7, p8, p9, p10;

    swscanf_s( wsz, L"%08lx-%04hx-%04hx-%02hx%02hx-%02hx%02hx%02hx%02hx%02hx%02hx",
        &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);

    Assert( wsz[36] == L'\0' );

    psortID->Data1 = p0;
    psortID->Data2 = p1;
    psortID->Data3 = p2;
    psortID->Data4[0] = static_cast<unsigned char>(p3);
    psortID->Data4[1] = static_cast<unsigned char>(p4);
    psortID->Data4[2] = static_cast<unsigned char>(p5);
    psortID->Data4[3] = static_cast<unsigned char>(p6);
    psortID->Data4[4] = static_cast<unsigned char>(p7);
    psortID->Data4[5] = static_cast<unsigned char>(p8);
    psortID->Data4[6] = static_cast<unsigned char>(p9);
    psortID->Data4[7] = static_cast<unsigned char>(p10);
}

// These two functions used to be in norm.hxx and used throughout the code. But
// when we got rid of the MSU, the differentiation of NLS Version and Defined
// Version largely disappeared -- we just treat the 64bit number as a single
// version number.
//
// But PfnCmpLocaleNameInfo() has a persisted sort order, so that order must
// be preserved.
LOCAL INLINE DWORD DwNLSVersionFromSortVersion( const QWORD qwVersion )
{
    return (DWORD)( ( qwVersion >> 32 ) & 0xFFFFFFFF );
}

LOCAL INLINE DWORD DwDefinedVersionFromSortVersion( const QWORD qwVersion )
{
    return (DWORD)( qwVersion & 0xFFFFFFFF );
}

//
//  initialization for the CATALOG layer
//

ERR ErrCATInit()
{
    //  if this assert goes off, you probably messed up adding a new column to the catalog
    //  remember to update idataMSOMax

    Assert( idataMSOMax == cColumnsMSO );

    ERR err = JET_errSuccess;

    //  initialize the catalog hash-table
    //      load factor     5.0
    //      uniformity      1.0

    CATHash::ERR errCATHash = g_cathash.ErrInit( 5.0, 1.0 );
    Assert( CATHash::ERR::errSuccess == errCATHash || CATHash::ERR::errOutOfMemory == errCATHash );
    if ( errCATHash == CATHash::ERR::errOutOfMemory )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    Assert( 0 != g_cbPage );

    Assert( PSystemSpaceHints( 0 ) );
    Assert( PSystemSpaceHints( eJSPHDefaultMax-1 ) );
    Assert( FIsSystemSpaceHint( PSystemSpaceHints( 0 ) ) );
    Assert( eJSPHDefaultMax-1 < _countof( g_jsphSystemDefaults ) );
    Assert( FIsSystemSpaceHint( PSystemSpaceHints( eJSPHDefaultMax-1 ) ) );

    return JET_errSuccess;

HandleError:

    //  term the catalog hash-table

    g_cathash.Term();

    return err;
}


//
//  termination of the CATALOG layer
//

void CATTerm()
{
#ifdef DEBUG

    //  catalog hash should be empty

    CATHashAssertClean();

#endif  //  DEBUG

    //  term the catalog hash-table

    g_cathash.Term();
}


//  lookup an entry in the catalog hash-table
//  if lookup succeeds, returns FCB from entryCATHash

BOOL FCATHashILookup(
    IFMP            ifmp,
    __in PCSTR const    szTableName,
    PGNO* const     ppgnoTableFDP,
    OBJID* const    pobjidTable )
{
    Assert( NULL != ppgnoTableFDP );
    Assert( NULL != pobjidTable );

    //  attempt to short-circuit the catalog seek by
    //  looking for it in the global ifmp/table-name
    //  hash table first

    CATHashKey      keyCATHash( ifmp, szTableName );
    CATHashEntry    entryCATHash;
    CATHash::CLock  lockCATHash;

    //  lock the key
    g_cathash.ReadLockKey( keyCATHash, &lockCATHash );

    //  see if we the entry exists
    const CATHash::ERR  errCATHash  = g_cathash.ErrRetrieveEntry( &lockCATHash, &entryCATHash );
    const BOOL          fFound      = ( CATHash::ERR::errSuccess == errCATHash );
    if ( fFound )
    {
        Assert( pfcbNil != entryCATHash.m_pfcb );
        *ppgnoTableFDP = entryCATHash.m_pfcb->PgnoFDP();
        *pobjidTable = entryCATHash.m_pfcb->ObjidFDP();
    }
    else
    {
        Assert( CATHash::ERR::errEntryNotFound == errCATHash );
    }

    //  unlock the key
    g_cathash.ReadUnlockKey( &lockCATHash );

    return fFound;
}


//  try to insert an entry into the catalog hash-table
//  ignore the result -- at best, we will get it in there or it will already be there
//                       at worst, we will get out-of-memory and the user will just have
//                           to incur the penalty for seeking every time

VOID CATHashIInsert( FCB *pfcb, __in PCSTR const szTable )
{

    //  try to add the new pgnoFDP / objidFDP to the
    //  catalog hash-table; if we fail, it means that
    //  someone else was racing with us to insert
    //  the entry and won (assert that the entry matches)

    CATHashKey      keyCATHash( pfcb->Ifmp(), szTable );
    CATHashEntry    entryCATHash( pfcb );
    CATHash::CLock  lockCATHash;
    BOOL            fProceedWithInsert = fTrue;
    BOOL            fInitialized;

    Assert( pfcb != NULL );
    // we never put temp tables in catalog hash
    Assert( pfcb->FTypeTable() );

    fInitialized = pfcb->FInitialized();

    //  lock the key

    g_cathash.WriteLockKey( keyCATHash, &lockCATHash );

    //  insert the entry

    if ( fInitialized )
    {
        entryCATHash.m_pfcb->EnterDML();
        fProceedWithInsert = !entryCATHash.m_pfcb->FDeletePending();
    }

    if ( fProceedWithInsert )
    {
        (void)g_cathash.ErrInsertEntry( &lockCATHash, entryCATHash );
    }

    if ( fInitialized )
        entryCATHash.m_pfcb->LeaveDML();

    //  unlock the key

    g_cathash.WriteUnlockKey( &lockCATHash );
}



//  delete an entry from the catalog hash-table
//  ignore the results -- at best, we will delete it
//                        at worst, it will already be gone

VOID CATHashIDelete( FCB *pfcb, __in PCSTR const szTable )
{

    //  try to add the new pgnoFDP / objidFDP to the
    //  catalog hash-table; if we fail, it means that
    //  someone else was racing with us to insert
    //  the entry and won (assert that the entry matches)

    CATHashKey      keyCATHash( pfcb->Ifmp(), szTable );
    CATHash::CLock  lockCATHash;

    //  lock the key

    g_cathash.WriteLockKey( keyCATHash, &lockCATHash );

    //  delete the entry (ignore the result)

    (VOID)g_cathash.ErrDeleteEntry( &lockCATHash );

    //  unlock the key

    g_cathash.WriteUnlockKey( &lockCATHash );
}


#ifdef DEBUG

//  verifies that the catalog hash does not contain any entries for the given IFMP/pgnoFDP pair

VOID CATHashAssertCleanIfmpPgnoFDP( const IFMP ifmp, const PGNO pgnoFDP )
{
    CATHash::CLock  lockCATHash;
    CATHash::ERR    errCATHash;
    CATHashEntry    entryCATHash;

    Assert( ifmp != ifmpNil );

    //  start a scan

    g_cathash.BeginHashScan( &lockCATHash );

    while ( ( errCATHash = g_cathash.ErrMoveNext( &lockCATHash ) ) != CATHash::ERR::errNoCurrentEntry )
    {

        //  fetch the current entry

        errCATHash = g_cathash.ErrRetrieveEntry( &lockCATHash, &entryCATHash );
        Assert( errCATHash == CATHash::ERR::errSuccess );

        //  ifmp should not match

        Assert( pfcbNil != entryCATHash.m_pfcb );
        if ( pgnoFDP != pgnoNull )
        {
            Assert( !( ( entryCATHash.m_pfcb->Ifmp() == ifmp ) && ( entryCATHash.m_pfcb->PgnoFDP() == pgnoFDP ) ) );
        }
        else
        {
            Assert( entryCATHash.m_pfcb->Ifmp() != ifmp );
        }
    }

    //  complete the scan

    g_cathash.EndHashScan( &lockCATHash );
}


//  make sure all entries in the catalog hash pertaining to the given IFMP are gone

VOID CATHashAssertCleanIfmp( IFMP ifmp )
{
    CATHashAssertCleanIfmpPgnoFDP( ifmp, pgnoNull );
}


//  make sure all entries in the catalog hash are gone

VOID CATHashAssertClean()
{
    CATHash::CLock  lockCATHash;
    CATHash::ERR    errCATHash;
    CATHashEntry    entryCATHash;

    //  start a scan

    g_cathash.BeginHashScan( &lockCATHash );
    errCATHash = g_cathash.ErrMoveNext( &lockCATHash );
    AssertSz( errCATHash == CATHash::ERR::errNoCurrentEntry, "Catalog hash-table was not empty during shutdown!" );
    while ( errCATHash != CATHash::ERR::errNoCurrentEntry )
    {

        //  fetch the current entry

        errCATHash = g_cathash.ErrRetrieveEntry( &lockCATHash, &entryCATHash );
        Assert( errCATHash == CATHash::ERR::errSuccess );

        //  move to the next entry

        errCATHash = g_cathash.ErrMoveNext( &lockCATHash );
    }

    //  complete the scan

    g_cathash.EndHashScan( &lockCATHash );
}

#endif  //  DEBUG


/*=================================================================
ErrCATCreate

Description:

    Called from ErrIsamCreateDatabase; creates all system tables

Parameters:

    PIB     *ppib       ; PIB of user
    IFMP    ifmp        ; ifmp of database that needs tables

Return Value:

    whatever error it encounters along the way

=================================================================*/

INLINE ERR ErrCATICreateCatalogIndexes(
    PIB         *ppib,
    const IFMP  ifmp,
    OBJID       *pobjidNameIndex,
    OBJID       *pobjidRootObjectsIndex )
{
    ERR         err;
    FUCB        *pfucbTableExtent;
    PGNO        pgnoIndexFDP;
    FCB         *pfcb = pfcbNil;

    //  don't maintain secondary indexes on the shadow catalog.

    // Open cursor for space navigation
    CallR( ErrDIROpen( ppib, pgnoFDPMSO, ifmp, &pfucbTableExtent ) );

    pfcb = pfucbTableExtent->u.pfcb;

    Assert( pfucbTableExtent != pfucbNil );
    Assert( !FFUCBVersioned( pfucbTableExtent ) );  // Verify won't be deferred closed.
    Assert( pfcb != pfcbNil );
    Assert( !pfcb->FInitialized() );
    Assert( pfcb->Pidb() == pidbNil );

    //  complete the initialization of the FCB

    pfcb->Lock();
    pfcb->CreateComplete();
    pfcb->Unlock();

    Call( ErrDIRCreateDirectory(
                pfucbTableExtent,
                (CPG)0,
                &pgnoIndexFDP,
                pobjidNameIndex,
                CPAGE::fPageIndex,
                fSPMultipleExtent|fSPUnversionedExtent ) );
    Assert( pgnoIndexFDP == pgnoFDPMSO_NameIndex );

    Call( ErrDIRCreateDirectory(
                pfucbTableExtent,
                (CPG)0,
                &pgnoIndexFDP,
                pobjidRootObjectsIndex,
                CPAGE::fPageIndex,
                fSPMultipleExtent|fSPUnversionedExtent ) );
    Assert( pgnoIndexFDP == pgnoFDPMSO_RootObjectIndex );

HandleError:
    Assert( pfcb->FInitialized() );
    Assert( pfcb->WRefCount() == 1 );

    //  force the FCB to be uninitialized so it will be purged by DIRClose

    pfcb->Lock();
    pfcb->CreateCompleteErr( errFCBUnusable );
    pfcb->Unlock();

    //  verify that this FUCB won't be defer-closed

    Assert( !FFUCBVersioned( pfucbTableExtent ) );

    //  close the FUCB

    DIRClose( pfucbTableExtent );

    return err;
}


// Returns the FID of a column in a catalog table.
INLINE FID ColumnidCATColumn( const CHAR *szColumnName )
{
    COLUMNID    columnid    = 0;
    UINT        i;

    for ( i = 0; i < cColumnsMSO; i++ )
    {
        if ( 0 == UtilCmpName( rgcdescMSO[i].szColName, szColumnName ) )
        {
            Assert( !FCOLUMNIDTemplateColumn( rgcdescMSO[i].columnid ) );
            columnid = rgcdescMSO[i].columnid;
            break;
        }
    }

    Assert( i < cColumnsMSO );
    Assert( columnid > 0 );

    return FID( (WORD)columnid );
}


//  ================================================================
ERR ErrCATIRetrieveTaggedColumn(
    FUCB                * const pfucb,
    const FID           fid,
    const ULONG         itagSequence,
    const DATA&         dataRec,
    BYTE                * const pbRet,
    const ULONG         cbRetMax,
    ULONG               * const pcbRetActual )
//  ================================================================
//
//  Retrieve a tagged column, possibly going to the LV tree
//  Takes a latched page and leaves the page latched. Because the
//  page is unlatched and relatched any pointers held into the page
//  may be invalidated
//
//-
{
    ERR err;

    Assert( fid.FTagged() );
    Assert( 1 == itagSequence );
    Assert( Pcsr( pfucb )->FLatched() );

    DATA dataRetrieved;
    Call( ErrRECIRetrieveTaggedColumn(
                pfucb->u.pfcb,
                ColumnidOfFid( fid, fFalse ),
                itagSequence,
                dataRec,
                &dataRetrieved ) );
    Assert( Pcsr( pfucb )->FLatched() );
    Assert( wrnRECUserDefinedDefault != err );
    Assert( wrnRECLongField != err );
    if ( wrnRECSeparatedLV == err )
    {
        Assert( REC::FValidLidRef( dataRetrieved ) );

        Call( ErrRECIRetrieveSeparatedLongValue(
                    pfucb,
                    dataRetrieved,
                    fTrue,
                    0,
                    fFalse,
                    pbRet,
                    cbRetMax,
                    pcbRetActual,
                    NO_GRBIT ) );
        Assert( JET_wrnColumnNull != err );

        //  must re-latch record
        Assert( !Pcsr( pfucb )->FLatched() );
        const ERR errT = ErrDIRGet( pfucb );
        err = ( errT < 0 ) ? errT : err;
    }
    else if( wrnRECCompressed == err )
    {
        INT cbActual;
        err = ErrPKDecompressData(
                dataRetrieved,
                pfucb,
                pbRet,
                cbRetMax,
                &cbActual );
        *pcbRetActual = (ULONG)cbActual;
    }
    else if( wrnRECIntrinsicLV == err )
    {
        *pcbRetActual = dataRetrieved.Cb();
        UtilMemCpy( pbRet, dataRetrieved.Pv(), min( cbRetMax, (ULONG)dataRetrieved.Cb() ) );
        err = ( cbRetMax >= (ULONG)dataRetrieved.Cb() ) ? JET_errSuccess : ErrERRCheck( JET_wrnBufferTruncated );
    }
    else
    {
        Assert( JET_wrnColumnNull == err );
        *pcbRetActual = 0;
    }

HandleError:
    Assert( err < 0 || Pcsr( pfucb )->FLatched() );

    return err;
}

//  ================================================================
LOCAL ERR ErrCATIRetrieveLocaleInformation(
    _In_ FUCB *                                 pfucbCatalog,
    _Out_writes_opt_z_( cchLocaleName ) WCHAR * wszLocaleName,
    _In_ ULONG                                  cchLocaleName,
    _Out_opt_ LCID *                            plcid,
    _Out_opt_ SORTID *                          psortidCustomSortVersion,
    _Out_opt_ QWORD *                           pqwSortVersion,
    _Out_opt_ DWORD *                           pdwNormalizationFlags )
//  ================================================================
//
//  Retrieves locale information (locale name and/or LCID) based on the LCID and
//  Locale Name columns. Only Locale Name should be present in new tables, while
//  LCID should be set for older tables.
//
//  When run on a non-Unicode index, wszLocaleName/plcid may still be populated, but
//  the version/sortid will be zero.
//  An index is considered to be a 'Unicode' index if the locale is set (fidbLocaleSet).
//
//  If neither the locale name nor the LCID is present in the catalog:
//      - wszLocaleName will be set to ""
//      - *plcid will be set to lcidInvariant (prior to Aug 2015, lcidNone).
//
//  Return values:
//      - JET_wrnColumnNull. No locale name present, and no LCID present. wszLocaleName will still be set to L"",
//              even though that's not the same thing. The empty string is actually the Invariant
//              Locale (LCID 127).
//              Reference: https://msdn.microsoft.com/en-us/library/system.globalization.cultureinfo(VS.71).aspx
//      - JET_errSuccess. The locale name exists. No guarantee about LCID/SORTID/Sort Version
//              existing or not. No way to differentiate between 0 and not-set.
//      - <Other Errors>
//-
{
    ERR err             = JET_errSuccess;
    ERR errLocaleAndLcidPresent = JET_errSuccess;
    ULONG cbActual;
    DATA dataField;
    WCHAR wszLocaleNameT[NORM_LOCALE_NAME_MAX_LENGTH];
    WCHAR * const wszOutputLocaleName = ( NULL != wszLocaleName ) ? wszLocaleName : wszLocaleNameT;
    const ULONG cchLocaleNameT = ( NULL != wszLocaleName ) ? cchLocaleName : _countof( wszLocaleNameT );
    BOOL fUserSpecifiedLocale = fFalse; // Was JET_bitIndexUnicode specified?
    BOOL fLocalizedText = fFalse; // Is there a cp==1200 column present?

    LCID lcidDummy;
    SORTID sortidDummyVersion;
    QWORD qwDummyVersion;
    DWORD dwNormalizationFlagsDummy;

    LCID * const plcidOutput = ( NULL != plcid ) ? plcid : &lcidDummy;
    SORTID * const psortidOutput = ( NULL != psortidCustomSortVersion ) ? psortidCustomSortVersion : &sortidDummyVersion;
    QWORD * const pqwOutputSortVersion = ( NULL != pqwSortVersion ) ? pqwSortVersion : &qwDummyVersion;
    DWORD * const pdwOutputNormalizationFlags = ( NULL != pdwNormalizationFlags ) ? pdwNormalizationFlags : &dwNormalizationFlagsDummy;

    *plcidOutput = 0;
    *pqwOutputSortVersion = 0;
    memset( psortidOutput, 0, sizeof( *psortidOutput ) );

    // At least one of the output parameters should be non-NULL.
    Assert( ( ( NULL != wszLocaleName ) && ( cchLocaleName > 0 ) )
            || ( NULL != plcid )
            || ( NULL != psortidCustomSortVersion )
            || ( NULL != pqwSortVersion ) );

    DBEnforce( pfucbCatalog->ifmp, cchLocaleNameT >= 1 );

    // First retrieve the status of FLocaleSet (whether the caller specified JET_bitIndexUnicode),
    // and fLocalizedText (whether any of the text columns are the Unicode code page).
    Assert( fidMSO_Flags.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Flags,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );
    Assert( dataField.Cb() == sizeof(LE_IDXFLAG) );
    const LE_IDXFLAG * const    ple_idxflag = (LE_IDXFLAG*)dataField.Pv();
    const IDBFLAG idbflag = ple_idxflag->fidb;
    fUserSpecifiedLocale = !!( fidbLocaleSet & idbflag );
    fLocalizedText = !!( fidbLocalizedText & idbflag );

    //  We will try to load the locale name property and see if there's anything set.
    Call( ErrCATIRetrieveTaggedColumn(
                pfucbCatalog,
                fidMSO_LocaleName,
                1,
                pfucbCatalog->kdfCurr.data,
                (BYTE *)wszOutputLocaleName,
                sizeof( WCHAR ) * cchLocaleNameT,
                &cbActual ) );
    errLocaleAndLcidPresent = err;

    Assert( err == JET_errSuccess || err == JET_wrnColumnNull || err == JET_wrnColumnNotInRecord );

    wszOutputLocaleName[min( cbActual / sizeof(WCHAR), cchLocaleNameT - 1 )] = 0;

    const BOOL fHasLocaleName = ( err != JET_wrnColumnNull && err != JET_wrnColumnNotInRecord );

    if ( fHasLocaleName )
    {
        Assert( JET_errSuccess == errLocaleAndLcidPresent );

        Call( ErrNORMLocaleToLcid( wszOutputLocaleName, plcidOutput ) );
        errLocaleAndLcidPresent = err;
    }
    else
    {
        //  If there was no locale name set, we will attempt to read in the LCID.
        Assert( fidMSO_Localization.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Localization,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(LCID) );
        errLocaleAndLcidPresent = err;

        *plcidOutput = *(UnalignedLittleEndian< LCID > *) dataField.Pv();

        if ( lcidNone == *plcidOutput )
        {
            // If there is neither a locale, nor an LCID stored in the catalog, default
            // to the invariant LCID.
            Assert( L'\0' == wszOutputLocaleName[0] );
            *plcidOutput = lcidInvariant;
        }
        else
        {
            Call( ErrNORMLcidToLocale( *plcidOutput, wszOutputLocaleName, cchLocaleNameT ) );
        }
    }

    // Look up the stored Versioning information, but only if there's a Unicode column.
    if ( fLocalizedText )
    {
        Assert( fidMSO_Version.FVar() );
        Call( ErrRECIRetrieveVarColumn(
            pfcbNil,
            pfucbCatalog->u.pfcb->Ptdb(),
            fidMSO_Version,
            pfucbCatalog->kdfCurr.data,
            &dataField ) );

        if( JET_wrnColumnNull == err )
        {
            // Do nothing further.
            Expected( dataField.Cb() == 0 );

            // Note: When originally implemented the sortid was hidden under the else
            // clause (i.e. version being not null), but we do not know for a fact that
            // NLS insists for new user custom sorts that they do not use a 0 version
            // field.
            // So it may be correct to have a version of zero and a non-zero
            // sort id. I (SOMEONE) suspect (but did not verify) a version of 0 would
            // be stored anyways in ESE, so the column won't actually be NULL, just 0.
            // If a version of 0 was stored as NULL and this is a valid usage case
            // (sort id != 0, version does == 0), then ESE will not run correctly,
            // failing to load this index. However there is an assert at the end of the
            // function today that will go off if a user uses this form of custom sort.
        }
        else
        {
            CallS( err );
            Assert( dataField.Cb() == sizeof( QWORD ) );
            *pqwOutputSortVersion = *(UnalignedLittleEndian< QWORD > * ) dataField.Pv();

            Assert( fidMSO_SortID.FVar() );
            Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_SortID,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );

            if ( JET_wrnColumnNull != err )
            {
                CallS( err );   // no other warnings expected
                if ( dataField.Cb() != sizeof( SORTID ) )
                {
                    AssertSz( fFalse, "Invalid fidMSO_SortID column in catalog." );
                    Error( ErrERRCheck( JET_errCatalogCorrupted ) );
                }

                *psortidOutput = *(UnalignedLittleEndian< SORTID > *) dataField.Pv();
            }
            else
            {
                Expected( dataField.Cb() == 0 );
            }

            Assert( fidMSO_LCMapFlags.FFixed() );
            Call( ErrRECIRetrieveFixedColumn(
                        pfcbNil,
                        pfucbCatalog->u.pfcb->Ptdb(),
                        fidMSO_LCMapFlags,
                        pfucbCatalog->kdfCurr.data,
                        &dataField ) );
            if ( dataField.Cb() == 0 )
            {
                Assert( JET_wrnColumnNull == err );

                //  old format: fLocaleSet is FALSE and locale name is not set
                //  (forced above to default value)
                *pdwOutputNormalizationFlags = dwLCMapFlagsDefaultOBSOLETE;
            }
            else
            {
                CallS( err );
                Assert( dataField.Cb() == sizeof(DWORD) );
                *pdwOutputNormalizationFlags = *(UnalignedLittleEndian< DWORD > *) dataField.Pv();

                OnDebug( const BOOL fUppercaseTextNormalization =
                        g_rgfmp[pfucbCatalog->ifmp].ErrDBFormatFeatureEnabled( JET_efvUppercaseTextNormalization ) >= JET_errSuccess );
                Assert( JET_errSuccess == ErrNORMCheckLCMapFlags(  PinstFromPfucb( pfucbCatalog ), *pdwOutputNormalizationFlags, fUppercaseTextNormalization ) );
            }
        }
    }

HandleError:
    if ( err >= JET_errSuccess )
    {
        // If neither locale name and LCID were present, return JET_wrnColumnNull.
        err = errLocaleAndLcidPresent;
        err = ( err == JET_wrnColumnNotInRecord ) ? JET_wrnColumnNull : err;

#ifdef DEBUG
        // At this point, it should be one of two possible return values.
        Expected( JET_errSuccess == err || JET_wrnColumnNull == err );

        // The invariant locale should have an empty locale name.
        // Other locales should have a locale name.
        Expected( ( L'\0' == wszOutputLocaleName[ 0 ] ) == ( lcidInvariant == *plcidOutput ) );

        if ( JET_wrnColumnNull == err )
        {
            // Locale not in the Catalog. Should default to "", 127.
            Assert( L'\0' == wszOutputLocaleName[ 0 ] );
            Assert( lcidInvariant == *plcidOutput );

            if ( fLocalizedText )
            {
                Expected( !FNORMSortidIsZeroes( psortidOutput ) );
                Expected( !DwNLSVersionFromSortVersion( *pqwOutputSortVersion ) == 0 );
            }
            else
            {
                Expected( FNORMSortidIsZeroes( psortidOutput ) );
                Expected( DwNLSVersionFromSortVersion( *pqwOutputSortVersion ) == 0 );
            }
        }
        else if ( JET_errSuccess == err )
        {
            // Locale (or LCID) IS not in the Catalog.
            // Check that wszOutputLocaleName is set (or "" for invariant culture).
            Expected( ( L'\0' != wszOutputLocaleName[ 0 ] && lcidInvariant != *plcidOutput ) ||
                      ( L'\0' == wszOutputLocaleName[ 0 ] && lcidInvariant == *plcidOutput ) );
            Assert( lcidNone != *plcidOutput );

            // fUserSpecifiedLocale may not hold. We sometimes default to the 'default locale' (en-us).
            // Expected( fUserSpecifiedLocale );

            // The locale name is set for all indices, even ones with no Unicode text!

            // fLocalizedText means at least one column has Unicode data (cp==1200).
            if ( fLocalizedText )
            {
                // Sort Version (fidMSO_Version) is not guaranteed to be set.
                Expected( 0 != *pqwOutputSortVersion );

                // psortidOutput == zero's is valid on Win7.
            }
            else
            {
                // This operation was done on a non-localized index.
                Expected( 0 == *pqwOutputSortVersion );
                Expected( FNORMSortidIsZeroes( psortidOutput ) );
            }
        }

        if ( !FNORMSortidIsZeroes( psortidOutput ) )
        {
            Assert( *pqwOutputSortVersion != 0 );
            Assert( DwNLSVersionFromSortVersion( *pqwOutputSortVersion ) != 0 );
        }
#endif
    }

    return err;
}

//
//  Space Hints and Extended Space Hint support
//
//

#if defined(_M_IX86)
#pragma optimize("", off)
#endif

LOCAL ERR ErrCheckNormalizeSpaceHintCb(
    _In_ const LONG             cbPageSize,
    _Inout_ ULONG *             pcb,
    _In_ BOOL                   fAllowCorrection )
{
    if( (*pcb) != ( g_cbPage * (ULONG)CpgEnclosingCb( cbPageSize, *pcb ) ) )
    {
        if( !fAllowCorrection )
        {
            //  Client expect the cb variable is correct.
            AssertSz( fFalse, "Space hints should be all corrected" );
            return ErrERRCheck( JET_errSpaceHintsInvalid );
        }
        *pcb = ( g_cbPage * CpgEnclosingCb( cbPageSize, *pcb ) );
    }
    Assert( (*pcb) % g_cbPage == 0 );
    return JET_errSuccess;
}

#if defined(_M_IX86)
#pragma optimize("", on)
#endif

ERR ErrCATCheckJetSpaceHints(
    _In_ const LONG             cbPageSize,
    _Inout_ JET_SPACEHINTS *    pSpaceHints,
    _In_ BOOL                   fAllowCorrection
    )
{
    ERR             err = JET_errSuccess;

#define SPACEHINT_VALIDATION_DOESNT_UNDERSTAND_IDX_VS_TBL
#ifdef SPACEHINT_VALIDATION_DOESNT_UNDERSTAND_IDX_VS_TBL
    BOOL                fExCatalogBug = fFalse;
#endif

    //  Check "version" and bail right off.
    //
    if ( NULL == pSpaceHints || pSpaceHints->cbStruct != sizeof(*pSpaceHints) )
    {
        Assert( fAllowCorrection ); // probably an error.
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  First normalize the spacehints cb type fields.
    //
    Call( ErrCheckNormalizeSpaceHintCb( cbPageSize, &(pSpaceHints->cbInitial), fAllowCorrection ) );
    Call( ErrCheckNormalizeSpaceHintCb( cbPageSize, &(pSpaceHints->cbMinExtent), fAllowCorrection ) );
    Call( ErrCheckNormalizeSpaceHintCb( cbPageSize, &(pSpaceHints->cbMaxExtent), fAllowCorrection ) );

    //  Further cbInitial should be at least 1 page.
    //
    if ( ( (ULONG)g_cbPage * cpgInitialTreeDefault ) == pSpaceHints->cbInitial )
    {
        if ( fAllowCorrection )
        {
            pSpaceHints->cbInitial = 0x0;   // set to default
        }
        else
        {
#ifdef SPACEHINT_VALIDATION_DOESNT_UNDERSTAND_IDX_VS_TBL
            fExCatalogBug = fTrue;
#else
            Error( ErrERRCheck( JET_errSpaceHintsInvalid ) );
#endif
        }
    }

    //  Validate the density
    //
    if ( 0 == pSpaceHints->ulInitialDensity )
    {
        if ( fAllowCorrection )
        {
            pSpaceHints->ulInitialDensity = ulFILEDefaultDensity;
        }
        else
        {
            Error( ErrERRCheck( JET_errSpaceHintsInvalid ) );
        }
    }

    if ( pSpaceHints->ulInitialDensity < ulFILEDensityLeast ||
            pSpaceHints->ulInitialDensity > ulFILEDensityMost )
    {
        Error( ErrERRCheck( JET_errDensityInvalid ) );
    }

    //  Maintenance density must be between 20% and 100% ... ( or 0 indicating "default" )
    //
    if ( pSpaceHints->ulMaintDensity != 0 &&
            pSpaceHints->ulMaintDensity != UlBound( pSpaceHints->ulMaintDensity , ulFILEDensityLeast, ulFILEDensityMost ) )
    {
        Error( ErrERRCheck( JET_errDensityInvalid ) );
    }

    //  Validate there are no unknown grbits.
    //
    if ( pSpaceHints->grbit & ~( JET_bitSpaceHintsUtilizeParentSpace |
                                            JET_bitCreateHintAppendSequential |
                                            JET_bitCreateHintHotpointSequential |
                                            JET_bitRetrieveHintReserve1 |
                                            JET_bitRetrieveHintTableScanForward |
                                            JET_bitRetrieveHintTableScanBackward |
                                            JET_bitRetrieveHintReserve2 |
                                            JET_bitRetrieveHintReserve3 |
                                            JET_bitDeleteHintTableSequential |
                                            JET_bitSpaceHintsUtilizeExactExtents
                                            )
        )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  Validate there are no incompatible with each other grbits.
    //
    if ( ( pSpaceHints->grbit & JET_bitRetrieveHintTableScanForward ) &&
            ( pSpaceHints->grbit & JET_bitRetrieveHintTableScanBackward ) )
    {
        Error( ErrERRCheck( JET_errSpaceHintsInvalid ) );
    }

    //  Validate we can push this into the FCB and get it out again.
    //
    C_ASSERT( 20 == sizeof(FCB_SPACE_HINTS) );
    FCB_SPACE_HINTS fcbshCheck;
    fcbshCheck.SetSpaceHints( pSpaceHints, cbPageSize );
    JET_SPACEHINTS  SpaceHintsCheck;
    fcbshCheck.GetAPISpaceHints( &SpaceHintsCheck, cbPageSize );
#ifdef SPACEHINT_VALIDATION_DOESNT_UNDERSTAND_IDX_VS_TBL
    BOOL fFCBAndAPISame;
    if ( fExCatalogBug )
    {
        fFCBAndAPISame = ( 0 == memcmp( pSpaceHints, &SpaceHintsCheck, 2 * sizeof(ULONG) ) );
        Assert( ( (ULONG) g_cbPage * cpgInitialTreeDefault ) == pSpaceHints->cbInitial &&
            0 == SpaceHintsCheck.cbInitial );
        fFCBAndAPISame = fFCBAndAPISame && ( 0 == memcmp( &(pSpaceHints->grbit), &(SpaceHintsCheck.grbit), sizeof(SpaceHintsCheck) - ( 3 * sizeof(ULONG) ) ) );
    }
    else
    {
        fFCBAndAPISame = ( 0 == memcmp( pSpaceHints, &SpaceHintsCheck, sizeof(SpaceHintsCheck) ) );
    }
#else
    const BOOL fFCBAndAPISame = ( 0 == memcmp( pSpaceHints, &SpaceHintsCheck, sizeof(SpaceHintsCheck) ) );
#endif
    if ( !fFCBAndAPISame )
    {
        AssertSz( fFalse, "We'd like the above checks to validate this well enough, that this doesn't happen." );
        Error( ErrERRCheck( JET_errSpaceHintsInvalid ) );
    }

    Assert( CpgInitial( pSpaceHints, cbPageSize ) != 0 );
    Assert( fcbshCheck.m_cpgInitial );
    fcbshCheck.m_cpgInitial = fcbshCheck.m_cpgInitial ? fcbshCheck.m_cpgInitial : 1;


    //  Validate the growth rate.
    //
    //      For now we only allow growth (could do shrinkage if someone wants?)
    //      And we only allow up to 500-fold growth in allocation size.
    if ( ( pSpaceHints->ulGrowth != 0 && pSpaceHints->ulGrowth < 100 ) ||
            pSpaceHints->ulGrowth >= ( 500 * 100) )
    {
        Error( ErrERRCheck( JET_errSpaceHintsInvalid ) );
    }

    //  Validate min / max extent size
    //
    //      For now we only allow cbSecondaryExtentMost (see space.hxx).
    if ( pSpaceHints->cbMinExtent >= cbSecondaryExtentMost ||
            pSpaceHints->cbMaxExtent >= cbSecondaryExtentMost ||
            pSpaceHints->cbMinExtent > pSpaceHints->cbMaxExtent )
    {
        //  For now we only allow 16 MBs as the min or max extent
        Error( ErrERRCheck( JET_errSpaceHintsInvalid ) );
    }

    //  We validate that the growth factor triggers growth.
    //
    if ( fcbshCheck.m_cpgMinExtent != fcbshCheck.m_cpgMaxExtent &&
            fcbshCheck.m_cpgMaxExtent != CpgSPIGetNextAlloc( &fcbshCheck, fcbshCheck.m_cpgInitial ) &&
            fcbshCheck.m_cpgMinExtent == CpgSPIGetNextAlloc( &fcbshCheck, fcbshCheck.m_cpgMinExtent ) )
    {
        Error( ErrERRCheck( JET_errSpaceHintsInvalid ) );
    }

    #ifdef SPACECHECK
    CPG cpgLast = fcbshCheck.m_cpgInitial;
    //  The way this # of Allocs is bounded to: cpgInitial * 1.01^cAllocs >= cpgMaxExtent, solve for cAllocs
    //      cpgInitial = 100, pctGrowth = 101%, cbMaxExtent = 16 MB --> 359 allocations.
    //      cpgInitial = 100, pctGrowth = 101%, cbMaxExtent = 1 GB  --> 780 allocations.
    for( ULONG iAlloc = 1; iAlloc < 781 /* worst case */ ; iAlloc++)
    {
        iAlloc++;
        cpgLast = CpgSPIGetNextAlloc( &fcbshCheck, cpgLast );
        if ( cpgLast >= fcbshCheck.m_cpgMaxExtent )
        {
            break;
        }
    }
    Assert( cpgLast >= fcbshCheck.m_cpgMaxExtent );
    #endif

HandleError:

    return err;
}

ULONG * PulCATIGetExtendedHintsStart(
    __in const SYSOBJ                   sysobj,
    __in const BOOL                     fDeferredLongValueHints,
    __in const JET_SPACEHINTS * const   pSpacehints,
    __out ULONG *                       pcTotalHints
    )
{
    Assert( pcTotalHints );
    ULONG * pulHints = NULL;
    if ( sysobj == sysobjTable && !fDeferredLongValueHints )
    {
        //  The grbit is the start of the extended hints
        //
        C_ASSERT( OffsetOf( JET_SPACEHINTS, grbit ) == 12 );
        pulHints = (ULONG*)&(pSpacehints->grbit);

        *pcTotalHints = sizeof(*pSpacehints) - OffsetOf( JET_SPACEHINTS, grbit );
        Assert( *pcTotalHints % 4 == 0 );
        *pcTotalHints = *pcTotalHints / 4;
        Assert( *pcTotalHints <= (sizeof(JET_SPACEHINTS)/sizeof(ULONG)-2) );
    }
    else if ( !fDeferredLongValueHints )
    {
        //  The cbInitial is the start of the extended hints
        //
        Assert( sysobj == sysobjIndex || sysobj == sysobjLongValue );
        C_ASSERT( OffsetOf( JET_SPACEHINTS, cbInitial ) == 8 );
        pulHints = (ULONG*)&(pSpacehints->cbInitial);

        *pcTotalHints = sizeof(*pSpacehints) - OffsetOf( JET_SPACEHINTS, cbInitial );
        Assert( *pcTotalHints % 4 == 0 );
        *pcTotalHints = *pcTotalHints / 4;
        Assert( *pcTotalHints <= (sizeof(JET_SPACEHINTS)/sizeof(ULONG)-2) );
    }
    else
    {
        //  For the deferred LV hints, we don't have any other place
        //  to store ulInitialDensity and cbInitial, so we start the
        //  extended hints at ulInitialDensity.  Note that in the LV's
        //  actual catalog entry we use the other branch.
        //
        Assert( fDeferredLongValueHints && sysobj == sysobjTable );
        C_ASSERT( OffsetOf( JET_SPACEHINTS, ulInitialDensity ) == 4 );
        pulHints = (ULONG*)&(pSpacehints->ulInitialDensity);

        *pcTotalHints = sizeof(*pSpacehints) - OffsetOf( JET_SPACEHINTS, ulInitialDensity );
        Assert( *pcTotalHints % 4 == 0 );
        *pcTotalHints = *pcTotalHints / 4;
        Assert( *pcTotalHints <= (sizeof(JET_SPACEHINTS)/sizeof(ULONG)-1) );
    }

    return pulHints;
}

LOCAL ERR ErrCATIMarshallExtendedSpaceHints(
    __in const IFMP                     ifmp,
    __in const SYSOBJ                   sysobj,
    __in const BOOL                     fDeferredLongValueHints,
    __in const JET_SPACEHINTS * const   pSpacehints,
    __out BYTE * const                  pBuffer,
    __in const ULONG                    cbBuffer,
    __out ULONG *                       pcbSetBuffer
    )
{
    ERR err = JET_errSuccess;
    ULONG cbSetBuffer = 0;

    if ( cbBuffer < cbExtendedSpaceHints ||
            pSpacehints->cbStruct != sizeof(JET_SPACEHINTS) )
    {
        AssertSz( fFalse, "Should have provided bigger buffer, or correct struct type." );
        Error( ErrERRCheck( JET_errInternalError ) );
    }

    //  Get the extended space hints portion of the space hints.
    //
    ULONG cTotalHints;
    ULONG * pulHints =  PulCATIGetExtendedHintsStart( sysobj, fDeferredLongValueHints, pSpacehints, &cTotalHints );

    //  Now "compress" everything that is non-zero.
    while( cTotalHints > 0 && pulHints[cTotalHints-1] == 0 )
    {
        cTotalHints--;
    }

    if ( cTotalHints == 0 )
    {
        *pcbSetBuffer = 0;  // indicate nothing to set
        err = JET_errSuccess;
        goto HandleError;
    }

    //  We have some amount of extended space hints to buffer.
    //
    Assert( cbSetBuffer == 0 );
    pBuffer[cbSetBuffer] = bCATExtendedSpaceHintsVersion;
    cbSetBuffer += sizeof(bCATExtendedSpaceHintsVersion);

    UnalignedLittleEndian< ULONG > * puleArray = NULL;
    puleArray = (UnalignedLittleEndian< ULONG > *) &(pBuffer[cbSetBuffer]);

    ULONG iHint = 0;
    for ( iHint = 0; ( iHint < cTotalHints ) && ( ( cbSetBuffer + 4 ) <= cbBuffer ) ; iHint++ )
    {
        DBEnforce( ifmp, (BYTE*)&(puleArray[iHint]) < pBuffer + cbBuffer );
        puleArray[iHint] = pulHints[iHint];
        cbSetBuffer += sizeof(ULONG);
    }
    if ( iHint != cTotalHints )
    {
        AssertSz( fFalse, "Could not marshall all of extended hints, odd" );
        Error( ErrERRCheck( JET_errInternalError ) );
    }

    // Success.
    *pcbSetBuffer = cbSetBuffer;
    err = JET_errSuccess;

HandleError:

    return err;
}

const LONG cbSpaceHintsTestPageSize = 4096;

ERR ErrCATIUnmarshallExtendedSpaceHints(
    __in INST * const           pinst,
    __in const SYSOBJ           sysobj,
    __in const BOOL             fDeferredLongValueHints,
    __in const BYTE * const     pBuffer,
    __in const ULONG            cbBuffer,
    __in const LONG             cbPageSize,
    __out JET_SPACEHINTS *      pSpacehints
    )
{
    ERR err = JET_errSuccess;

    if ( pSpacehints->cbStruct != sizeof(JET_SPACEHINTS) )
    {
        AssertSz( fFalse, "Should have correct struct type." );
        Error( ErrERRCheck( JET_errInternalError ) );
    }

    if ( cbBuffer < sizeof(bCATExtendedSpaceHintsVersion) ||
            pBuffer[0] != bCATExtendedSpaceHintsVersion )
    {
        AssertSz( fFalse, "Catalog extended space hints version bad or missized." );
        OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"7c12e126-7b09-4c3d-a504-1430f86a9f12" );
        Error( ErrERRCheck( JET_errCatalogCorrupted ) );
    }

    C_ASSERT( sizeof(ULONG) == sizeof(UnalignedLittleEndian< ULONG >) );
    Assert( sizeof(ULONG) == sizeof(UnalignedLittleEndian< ULONG >) );

    if ( ( cbBuffer - sizeof(bCATExtendedSpaceHintsVersion) ) % sizeof(ULONG) )
    {
        OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"0ef7c844-5385-492b-93bd-502153b1a8c3" );
        Error( ErrERRCheck( JET_errCatalogCorrupted ) );
    }

    UnalignedLittleEndian< ULONG > * puleArray = NULL;
    puleArray = (UnalignedLittleEndian< ULONG > *)&(pBuffer[sizeof(bCATExtendedSpaceHintsVersion)]);
    ULONG cArray = ( cbBuffer - sizeof(bCATExtendedSpaceHintsVersion) ) / sizeof(ULONG);

    ULONG cTotalHints;
    ULONG * pulHints = PulCATIGetExtendedHintsStart( sysobj, fDeferredLongValueHints, pSpacehints, &cTotalHints );

    ULONG iHint = 0;
    for( iHint = 0; ( iHint < cArray ) && ( iHint < cTotalHints ); iHint++ )
    {
        Assert( (BYTE*)(&(pulHints[iHint])) < (((BYTE*)pSpacehints) + pSpacehints->cbStruct) );
        Assert( (BYTE*)(&(pulHints[iHint])+1) <= (((BYTE*)pSpacehints) + pSpacehints->cbStruct) );
        pulHints[iHint] = puleArray[iHint];
    }
    if ( iHint != cArray )
    {
        AssertSz( fFalse, "Space hints was not large enough for extended hints stored in catalog!" );
        OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"ad4eadb4-2233-4c36-88b3-30b1e0fc06ee" );
        Error( ErrERRCheck( JET_errCatalogCorrupted ) );
    }
#ifdef DEBUG
    for ( ; iHint < cTotalHints; iHint++ )
    {
        Assert( (BYTE*)(&(pulHints[iHint])) < (((BYTE*)pSpacehints) + pSpacehints->cbStruct) );
        Assert( (BYTE*)(&(pulHints[iHint])+1) <= (((BYTE*)pSpacehints) + pSpacehints->cbStruct) );
        Assert( pulHints[iHint] == 0 );
    }
#endif

    CallS( ErrCATCheckJetSpaceHints( cbPageSize, pSpacehints ) );
    err = JET_errSuccess;

HandleError:

    return err;
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( CATSpaceHints, SpaceHintsMarshallUnmarshallBase )
{
    JET_SPACEHINTS  jsph1 = { sizeof(JET_SPACEHINTS), 0 };
    BYTE            rgBuffer[cbExtendedSpaceHints];
    ULONG           cbBufferSet;
    JET_SPACEHINTS  jsphCheck = { sizeof(JET_SPACEHINTS), 0 };

    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTable, fFalse /* fDefLV */, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( 0 == cbBufferSet );

    //  At this layer, every valid spacehints must have an initial density
    jsph1.ulInitialDensity = 80;
    jsphCheck.ulInitialDensity = jsph1.ulInitialDensity;

    //  Initial density in the table is stored in a catalog column
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTable, fFalse /* fDefLV */, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( 0 == cbBufferSet );

    //  Test maintenance density
    jsph1.ulMaintDensity = 90;
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // should still be true
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTable, fFalse /* fDefLV */, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTable, fFalse /* fDefLV */, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.ulMaintDensity = 0;
}

#endif // ENABLE_JET_UNIT_TEST

VOID ResetSPH( JET_SPACEHINTS * const pjsphReset, const JET_SPACEHINTS * const pjsphTemplate, SYSOBJ sysobj )
{
    memset( pjsphReset, 0, sizeof(*pjsphReset) );
    pjsphReset->cbStruct = sizeof(*pjsphReset);

    if ( sysobj == sysobjTable || sysobj == sysobjIndex || sysobj == sysobjLongValue )
    {
        if ( pjsphTemplate )
        {
            pjsphReset->ulInitialDensity = pjsphTemplate->ulInitialDensity;
        }
    }
    if ( sysobj == sysobjTable )
    {
        if ( pjsphTemplate )
        {
            pjsphReset->cbInitial = pjsphTemplate->cbInitial;
        }
    }
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( CATSpaceHints, BasicTableSpaceHintsMarshallUnmarshall )
{
    JET_SPACEHINTS  jsph1 = { sizeof(JET_SPACEHINTS), 0 };
    BYTE            rgBuffer[cbExtendedSpaceHints];
    ULONG           cbBufferSet;
    JET_SPACEHINTS  jsphCheck = { sizeof(JET_SPACEHINTS), 0 };

    const SYSOBJ    sysobjTest = sysobjTable;
    const BOOL      fDefLV = fFalse;

    ResetSPH( &jsphCheck, &jsph1, sysobjTest );

    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( 0 == cbBufferSet );

    //  At this layer, every valid spacehints must have an initial density
    jsph1.ulInitialDensity = 80;
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // set to true b/c marshal/unmarshal doesn't handle this element

    //  Initial density in the table is stored in a catalog column
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( 0 == cbBufferSet );

    //  Test initial extent
    jsph1.cbInitial = 256 * 1024;
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // should still be true
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( 0 == cbBufferSet );
    jsph1.cbInitial = 0;

    //  Test grbit
    jsph1.grbit = 0x1C;
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( cbBufferSet > 0 );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.grbit = 0;

    //  Test maintenance density
    jsph1.ulMaintDensity = 90;
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // should still be true
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( cbBufferSet > 0 );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.ulMaintDensity = 0;

    //  Test growth factor
    jsph1.ulGrowth = 200;
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // should still be true
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.ulGrowth = 0;

    //  Test min extent
    jsph1.cbMinExtent = 64 * 1024;
    jsph1.cbMaxExtent = 64 * 1024;  // cbMaxExtent must >= cbMinExtent
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // should still be true
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.cbMinExtent = 0;
    jsph1.cbMaxExtent = 0;

    //  Test max extent
    jsph1.cbMaxExtent = 192 * 1024;
    jsph1.cbMinExtent = 192 * 1024; // avoid triggering growth grows check
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // should still be true
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.cbMaxExtent = 0;
    jsph1.cbMinExtent = 0;
}


JETUNITTEST( CATSpaceHints, BasicIndexSpaceHintsMarshallUnmarshall )
{
    JET_SPACEHINTS  jsph1 = { sizeof(JET_SPACEHINTS), 0 };
    BYTE            rgBuffer[cbExtendedSpaceHints];
    ULONG           cbBufferSet;
    JET_SPACEHINTS  jsphCheck = { sizeof(JET_SPACEHINTS), 0 };

    const SYSOBJ    sysobjTest = sysobjIndex;
    const BOOL      fDefLV = fFalse;

    ResetSPH( &jsphCheck, &jsph1, sysobjTest );

    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( 0 == cbBufferSet );

    //  At this layer, every valid spacehints must have an initial density
    jsph1.ulInitialDensity = 80;
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // set to true b/c marshal/unmarshal doesn't handle this element

    //  Initial density in the table is stored in a catalog column
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( 0 == cbBufferSet );

    //  Test initial extent (unlike table ends up in extended hints)
    jsph1.cbInitial = 256 * 1024;
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( cbBufferSet > 0 );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.cbInitial = 0;

    //  Test grbit
    jsph1.grbit = 0x1C;
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( cbBufferSet > 0 );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.grbit = 0;

    //  Test maintenance density
    jsph1.ulMaintDensity = 90;
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // should still be true
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.ulMaintDensity = 0;

    //  Test growth factor
    jsph1.ulGrowth = 200;
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // should still be true
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.ulGrowth = 0;

    //  Test min extent
    jsph1.cbMinExtent = 64 * 1024;
    jsph1.cbMaxExtent = 64 * 1024;  // cbMaxExtent must >= cbMinExtent
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // should still be true
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.cbMinExtent = 0;
    jsph1.cbMaxExtent = 0;

    //  Test max extent
    jsph1.cbMaxExtent = 192 * 1024;
    jsph1.cbMinExtent = 192 * 1024; // avoid triggering growth grows check
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // should still be true
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.cbMaxExtent = 0;
    jsph1.cbMinExtent = 0;
}

JETUNITTEST( CATSpaceHints, BasicLvSpaceHintsMarshallUnmarshall )
{
    JET_SPACEHINTS  jsph1 = { sizeof(JET_SPACEHINTS), 0 };
    BYTE            rgBuffer[cbExtendedSpaceHints];
    ULONG           cbBufferSet;
    JET_SPACEHINTS  jsphCheck = { sizeof(JET_SPACEHINTS), 0 };

    const SYSOBJ    sysobjTest = sysobjLongValue;
    const BOOL      fDefLV = fFalse;

    ResetSPH( &jsphCheck, &jsph1, sysobjTest );

    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( 0 == cbBufferSet );

    //  At this layer, every valid spacehints must have an initial density
    jsph1.ulInitialDensity = 80;
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // set to true b/c marshal/unmarshal doesn't handle this element

    //  Initial density in the table is stored in a catalog column
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( 0 == cbBufferSet );

    //  Test initial extent (unlike table ends up in extended hints)
    jsph1.cbInitial = 256 * 1024;
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( cbBufferSet > 0 );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.cbInitial = 0;

    //  Test grbit
    jsph1.grbit = 0x1C;
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( cbBufferSet > 0 );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.grbit = 0;

    //  Test maintenance density
    jsph1.ulMaintDensity = 90;
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // should still be true
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.ulMaintDensity = 0;

    //  Test growth factor
    jsph1.ulGrowth = 200;
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // should still be true
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.ulGrowth = 0;

    //  Test min extent
    jsph1.cbMinExtent = 64 * 1024;
    jsph1.cbMaxExtent = 64 * 1024;  // cbMaxExtent must >= cbMinExtent
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // should still be true
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.cbMinExtent = 0;
    jsph1.cbMaxExtent = 0;

    //  Test max extent
    jsph1.cbMaxExtent = 192 * 1024;
    jsph1.cbMinExtent = 192 * 1024; // avoid triggering growth grows check
    ResetSPH( &jsphCheck, &jsph1, sysobjTest );
    CHECK( jsph1.ulInitialDensity == jsphCheck.ulInitialDensity );  // should still be true
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.cbMaxExtent = 0;
    jsph1.cbMinExtent = 0;
}

JETUNITTEST( CATSpaceHints, BasicDefLvSpaceHintsMarshallUnmarshall )
{
    JET_SPACEHINTS  jsph1 = { sizeof(JET_SPACEHINTS), 0 };
    BYTE            rgBuffer[cbExtendedSpaceHints];
    ULONG           cbBufferSet;
    JET_SPACEHINTS  jsphCheck = { sizeof(JET_SPACEHINTS), 0 };

    const SYSOBJ    sysobjTest = sysobjTable;
    const BOOL      fDefLV = fTrue;

    ResetSPH( &jsphCheck, &jsph1, sysobjTest );

    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( 0 == cbBufferSet );

    //  At this layer, every valid spacehints must have an initial density
    jsph1.ulInitialDensity = 80;

    ResetSPH( &jsphCheck, NULL, sysobjTest );

    //  Initial density in the table is stored in a catalog column
    CHECK( jsph1.ulInitialDensity == 80 );  // set above, b/c can't be 0
    ResetSPH( &jsphCheck, NULL, sysobjTest );
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( cbBufferSet > 0 );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    // no: jsph1.ulInitialDensity = 0;

    //  Test initial extent (unlike table ends up in extended hints)
    jsph1.cbInitial = 256 * 1024;
    ResetSPH( &jsphCheck, NULL, sysobjTest );
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECK( cbBufferSet > 0 );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.cbInitial = 0;

    //  Test grbit
    jsph1.grbit = 0x1C;
    ResetSPH( &jsphCheck, NULL, sysobjTest );
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.grbit = 0;

    //  Test maintenance density
    jsph1.ulMaintDensity = 90;
    ResetSPH( &jsphCheck, NULL, sysobjTest );
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.ulMaintDensity = 0;

    //  Test growth factor
    jsph1.ulGrowth = 200;
    ResetSPH( &jsphCheck, NULL, sysobjTest );
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.ulGrowth = 0;

    //  Test min extent
    jsph1.cbMinExtent = 64 * 1024;
    jsph1.cbMaxExtent = 64 * 1024;  // cbMaxExtent must >= cbMinExtent
    ResetSPH( &jsphCheck, NULL, sysobjTest );
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.cbMinExtent = 0;
    jsph1.cbMaxExtent = 0;

    //  Test max extent
    jsph1.cbMaxExtent = 192 * 1024;
    jsph1.cbMinExtent = 192 * 1024; // avoid triggering growth grows check
    ResetSPH( &jsphCheck, NULL, sysobjTest );
    CHECKCALLS( ErrCATIMarshallExtendedSpaceHints( ifmpNil, sysobjTest, fDefLV, &jsph1, rgBuffer, sizeof(rgBuffer), &cbBufferSet ) );
    CHECKCALLS( ErrCATIUnmarshallExtendedSpaceHints( pinstNil, sysobjTest, fDefLV, rgBuffer, cbBufferSet, cbSpaceHintsTestPageSize, &jsphCheck ) );
    CHECK( 0 == memcmp( &jsph1, &jsphCheck, sizeof(jsph1) ) );
    jsph1.cbMaxExtent = 0;
    jsph1.cbMinExtent = 0;
}

#endif // ENABLE_JET_UNIT_TEST

ERR ErrCATISetSpaceHints(
    __in FUCB* const                    pfucbCatalog,
    __in const SYSOBJ                   sysobj,
    __in const BOOL                     fDeferredLongValueHints,
    __in const JET_SPACEHINTS * const   pSpacehints,
    __in const CPG * const              pcpgInitialInPages,
    __out BYTE *                        pbBuffer,
    __in const ULONG                    cbBuffer,
    __out DATA                          rgdata[],
    __in const ULONG                    cdata
    )
{
    ERR     err = JET_errSuccess;

    if ( NULL == pSpacehints )
    {
        AssertSz( fFalse, "This should not happen anymore." );
        Assert( rgdata[ iMSO_SpaceUsage ].Cb() == 0 );
        Assert( rgdata[ iMSO_Pages ].Cb() == 0 || sysobj != sysobjTable );
        Assert( rgdata[ iMSO_SpaceHint ].Cb() == 0 );
        Assert( rgdata[ iMSO_SpaceLVDeferredHints ].Cb() == 0 );
        return JET_errSuccess;
    }

    //  By now the space hints should be valid, or else we are corrupting the catalog.
    CallS( ErrCATCheckJetSpaceHints( g_rgfmp[ pfucbCatalog->ifmp ].CbPage(), (JET_SPACEHINTS*)pSpacehints ) );

    if( !fDeferredLongValueHints )
    {
        //  First set the density.
        //
        rgdata[ iMSO_SpaceUsage ].SetPv( (BYTE*) &(pSpacehints->ulInitialDensity) );
        rgdata[ iMSO_SpaceUsage ].SetCb( sizeof(pSpacehints->ulInitialDensity) );

        if( sysobj == sysobjTable )
        {
            //  Next set the initial space allocation
            //
            Assert( pSpacehints->cbInitial % g_cbPage == 0 );
            Assert( *pcpgInitialInPages == (CPG)( pSpacehints->cbInitial / g_cbPage ) );
            rgdata[ iMSO_Pages ].SetPv( (BYTE*) pcpgInitialInPages );
            rgdata[ iMSO_Pages ].SetCb( sizeof(*pcpgInitialInPages) );
        }
        else
        {
            Assert( pcpgInitialInPages == NULL );
            Assert( ( (ULONG)g_cbPage * cpgInitialTreeDefault ) != pSpacehints->cbInitial );    // valid, but should not be stored as such...
        }
    }

    //  Process the extended space hints
    //
    ULONG iMSO = fDeferredLongValueHints ? iMSO_SpaceLVDeferredHints : iMSO_SpaceHint;

    ULONG cbSet;
    Call( ErrCATIMarshallExtendedSpaceHints( pfucbCatalog->ifmp, sysobj,
                    fDeferredLongValueHints, pSpacehints, pbBuffer, cbBuffer,
                    &cbSet ) );
    CallS( err );

    if ( cbSet > 0 )
    {
        Assert( cbBuffer >= sizeof(bCATExtendedSpaceHintsVersion) + sizeof(ULONG) );
        rgdata[ iMSO ].SetPv( pbBuffer );
        rgdata[ iMSO ].SetCb( cbSet );

        //  Validate that we'll be able to recover the space hints with full
        //  fidelity later.
        //
        JET_SPACEHINTS jsphCheck = { 0 };
        jsphCheck.cbStruct = sizeof(jsphCheck);
        jsphCheck.cbInitial = pSpacehints->cbInitial;
        jsphCheck.ulInitialDensity = pSpacehints->ulInitialDensity;
        ResetSPH( &jsphCheck, fDeferredLongValueHints ? NULL : pSpacehints, sysobj );
        // Consider making this handle NULL or zero length pbBuffer, and put after if...
        OnDebug( const ERR errT = )
        ErrCATIUnmarshallExtendedSpaceHints( 
            PinstFromPfucb( pfucbCatalog ), 
            sysobj, 
            fDeferredLongValueHints, 
            pbBuffer, 
            cbSet, 
            g_rgfmp[ pfucbCatalog->ifmp ].CbPage(), 
            &jsphCheck );
        Assert( errT >= JET_errSuccess );
        DBEnforce( pfucbCatalog->ifmp, 0 == memcmp( pSpacehints, &jsphCheck, sizeof(jsphCheck) ) );
    }
    else
    {
        Assert( rgdata[ iMSO ].Cb() == 0 );
    }

    err = JET_errSuccess;

HandleError:

    return err;
}

ERR ErrCATIRetrieveSpaceHints(
    __inout FUCB * const            pfucbCatalog,
    __in const TDB * const          ptdbCatalog,
    __in const DATA&                dataRec,
    __in const BOOL                 fDeferredLongValueHints,
    __in const JET_SPACEHINTS * const   pTemplateSpaceHints,
    __out JET_SPACEHINTS * const    pSpacehints
    )
{
    ERR     err = JET_errSuccess;
    ERR     wrn = JET_errSuccess;
    DATA    dataField;

    memset( pSpacehints, 0, sizeof(*pSpacehints) );
    pSpacehints->cbStruct = sizeof(*pSpacehints);

    Assert( fidMSO_Type.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_Type,
                dataRec,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(SYSOBJ) );
    const SYSOBJ    sysobj = (SYSOBJ) (*(UnalignedLittleEndian< ULONG > *) dataField.Pv());
    Assert( sysobj == sysobjTable || sysobj == sysobjIndex || sysobj == sysobjLongValue );

    if( !fDeferredLongValueHints )
    {
        //  First get the density.
        //
        Assert( fidMSO_SpaceUsage.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    ptdbCatalog,
                    fidMSO_SpaceUsage,
                    dataRec,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(ULONG) );
        pSpacehints->ulInitialDensity = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
        Assert( pSpacehints->ulInitialDensity >= ulFILEDensityLeast );
        Assert( pSpacehints->ulInitialDensity <= ulFILEDensityMost );

        if ( sysobj == sysobjTable )
        {
            Assert( fidMSO_Pages.FFixed() );
            Call( ErrRECIRetrieveFixedColumn(
                        pfcbNil,
                        ptdbCatalog,
                        fidMSO_Pages,
                        dataRec,
                        &dataField ) );
            CallS( err );
            Assert( dataField.Cb() == sizeof(CPG) );
            pSpacehints->cbInitial = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
            pSpacehints->cbInitial *= g_cbPage;
        }
    }

    BYTE rgBuffer[cbExtendedSpaceHints];
    ULONG cbActual = 0;
    Call( ErrCATIRetrieveTaggedColumn(
                    pfucbCatalog,
                    fDeferredLongValueHints ? fidMSO_SpaceLVDeferredHints : fidMSO_SpaceHints,
                    1,
                    dataRec,
                    (BYTE*)&rgBuffer,
                    sizeof(rgBuffer),
                    &cbActual ) );
    Assert( JET_errSuccess == err
            || JET_wrnColumnNull == err );

    if( JET_errSuccess == err )
    {
        Call( ErrCATIUnmarshallExtendedSpaceHints(
                    PinstFromPfucb( pfucbCatalog ), 
                    sysobj, 
                    fDeferredLongValueHints, 
                    rgBuffer, 
                    cbActual, 
                    g_rgfmp[ pfucbCatalog->ifmp ].CbPage(),
                    pSpacehints ) );
    }
    else if ( JET_wrnColumnNull == err )
    {
        if ( fDeferredLongValueHints )
        {
            //  Indicate to the client there were no explicit space hints set
            wrn = JET_wrnColumnNull;
        }

        //  There are no extended space hints, which means
        //  that we should pick up the template space hints.
        if ( NULL != pTemplateSpaceHints )
        {
            UtilMemCpy( pSpacehints, pTemplateSpaceHints, sizeof(JET_SPACEHINTS) );
        }
        if( fDeferredLongValueHints )
        {
            pSpacehints->ulInitialDensity = ulFILEDensityMost;
        }
    }
    else
    {
        Assert( err != JET_errSuccess );
        CallS( err );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"902fc489-520c-48a4-88e9-b43caff5ede8" );
        Error( ErrERRCheck( JET_errCatalogCorrupted ) );
    }

    Call( ErrCATCheckJetSpaceHints( g_rgfmp[ pfucbCatalog->ifmp ].CbPage(), pSpacehints ) );
    err = wrn;

HandleError:

    return err;
}


/*  Catalog-retrieval routines for system tables.
/*
/*  This code was lifted from the key parsing code in ErrIsamCreateIndex(), with
/*  optimisations for assumptions made for key strings of system table indexes.
/**/
LOCAL BYTE CfieldCATKeyString( _In_ _NullNull_terminated_ PCSTR szKey, IDXSEG* rgidxseg )
{
    const CHAR  *pch;
    ULONG   cfield = 0;

    for ( pch = szKey; *pch != '\0'; pch += strlen( pch ) + 1 )
    {
        /*  Assume the first character of each component is a '+' (this is
        /*  specific to system table index keys).  In general, this may also
        /*  be a '-' or nothing at all (in which case '+' is assumed), but we
        /*  don't use descending indexes for system tables and we'll assume we
        /*  know enough to put '+' characters in our key string.
        /**/
        Assert( *pch == '+' );
        pch++;

        rgidxseg[cfield].ResetFlags();
        rgidxseg[cfield].SetFid( FidOfColumnid( ColumnidCATColumn( pch ) ) );
        Assert( !rgidxseg[cfield].FTemplateColumn() );
        Assert( !rgidxseg[cfield].FDescending() );
        Assert( !rgidxseg[cfield].FMustBeNull() );
        cfield++;
    }


    // Verify the key-field array will fit in the IDB.
    Assert( cfield > 0 );
    Assert( cfield <= cIDBIdxSegMax );

    return (BYTE)cfield;
}

//  This function checks if the column ID is part of MSO before the CreateDB V1 log record.
LOCAL INLINE BOOL FColumnIDOldMSO( __in const COLUMNID columnID )
{
    for ( INT i = 0; i < _countof( rgcolumnidOldMSO ); ++i )
    {
        if ( columnID == rgcolumnidOldMSO[i] )
        {
            return fTrue;
        }
    }

    return fFalse;
}

LOCAL ERR ErrCATPopulateCatalog(
    PIB         *ppib,
    FUCB        *pfucbCatalog,
    const PGNO  pgnoFDP,
    const OBJID objidTable,
    const CHAR  *szTableName,
    const CPG   cpgInitial,
    const BOOL  fExpectKeyDuplicateErrors,
    const BOOL  fReplayCreateDbImplicitly )
{
    ERR         err;
    UINT        i;
    FIELD       field;
    JET_SPACEHINTS jsphCatalog = {
            sizeof(JET_SPACEHINTS),
            ulFILEDefaultDensity,
            (ULONG)g_rgfmp[ pfucbCatalog->ifmp ].CbOfCpg( cpgInitial ),
            /* rest will be zero'd */ };
    Assert( jsphCatalog.grbit == 0 && jsphCatalog.cbMaxExtent == 0 ); // spot check.
    // Can not set extended space hints on catalog table, no LV updates
    // during DB creation, as DB creation is done outside of a transaction.
    Assert( !FCATHasExtendedHints( sysobjTable, &jsphCatalog ) ); // real check.

    field.ibRecordOffset    = ibRECStartFixedColumns;   // record offset will be calculated at catalog open time
    field.cp                = usEnglishCodePage;

    //  must insert MSysObjects record to prevent clients from
    //  creating a table named "MSysObjects".
    err = ErrCATAddTable(
                ppib,
                pfucbCatalog,
                pgnoFDP,
                objidTable,
                szTableName,
                NULL,
                JET_bitObjectSystem|JET_bitObjectTableFixedDDL,
                &jsphCatalog,
                0,
                NULL,
                0 );
    if( JET_errTableDuplicate == err && fExpectKeyDuplicateErrors )
    {
        err = JET_errSuccess;
    }
    Call( err );

    for ( i = 0; i < cColumnsMSO; i++ )
    {
        //  when replaying a CreateDB record implicitly, we will not create the columns in MSO
        //  that were not present at the time of the CreateDB V1 log record. Only columns added
        //  to MSO after such (e.g.: SortID, LocaleName) should be skipped here.
        if ( fReplayCreateDbImplicitly &&
            !FColumnIDOldMSO( rgcdescMSO[i].columnid ) )
        {
            continue;
        }

        field.coltyp    = FIELD_COLTYP( rgcdescMSO[i].coltyp );
        field.cbMaxLen  = UlCATColumnSize( field.coltyp, 0, NULL );

        //  only supported flag for system table columns is JET_bitColumnNotNULL
        field.ffield    = 0;
        Assert( 0 == rgcdescMSO[i].grbit
                || JET_bitColumnNotNULL == rgcdescMSO[i].grbit
                || JET_bitColumnTagged == rgcdescMSO[i].grbit );
        if ( JET_bitColumnNotNULL == rgcdescMSO[i].grbit )
            FIELDSetNotNull( field.ffield );

        Assert( ibRECStartFixedColumns == field.ibRecordOffset );   //  offset will be fixed up at catalog open time
        Assert( usEnglishCodePage == field.cp );

        err = ErrCATAddTableColumn(
                ppib,
                pfucbCatalog,
                objidTable,
                rgcdescMSO[i].szColName,
                rgcdescMSO[i].columnid,
                &field,
                NULL,
                0,
                NULL,
                NULL,
                0 );
        if( JET_errColumnDuplicate == err && fExpectKeyDuplicateErrors )
        {
            err = JET_errSuccess;
        }
        Call( err );
    }

HandleError:
    return err;
}

ERR ErrREPAIRCATCreate(
    PIB *ppib,
    const IFMP ifmp,
    const OBJID objidNameIndex,
    const OBJID objidRootObjectsIndex,
    const BOOL  fInRepair,
    const BOOL  fReplayCreateDbImplicitly )
{
    ERR     err;
    FUCB    *pfucb              = pfucbNil;

    IDB     idb( PinstFromIfmp( ifmp ) );

    //  open table in exclusive mode, for output parameter
    //
    CallR( ErrFILEOpenTable(
            ppib,
            ifmp,
            &pfucb,
            szMSO,
            JET_bitTableDenyRead | bitTableUpdatableDuringRecovery ) );
    Assert( pfucbNil != pfucb );

    Call( ErrCATPopulateCatalog(
                ppib,
                pfucb,
                pgnoFDPMSO,
                objidFDPMSO,
                szMSO,
                cpgMSOInitial,
                fInRepair, //fFalse by default (non-repair situation)
                fReplayCreateDbImplicitly ) );

    Call( ErrCATPopulateCatalog(
                ppib,
                pfucb,
                pgnoFDPMSOShadow,
                objidFDPMSOShadow,
                szMSOShadow,
                fReplayCreateDbImplicitly ? cpgMSOShadowInitialLegacy : CpgCATShadowInitial( g_cbPage ),
                fInRepair, //fFalse by default (non-repair situation)
                fReplayCreateDbImplicitly ) );

    idb.SetWszLocaleName( wszLocaleNameDefault );
    idb.SetDwLCMapFlags( dwLCMapFlagsDefault );
    idb.SetCidxsegConditional( 0 );     //  UNDONE: catalog indexes cannot be conditional
    idb.SetCbKeyMost( JET_cbKeyMost_OLD );
    idb.SetCbVarSegMac( JET_cbKeyMost_OLD );
    idb.SetCidxseg( CfieldCATKeyString( rgidescMSO[0].szIdxKeys, idb.rgidxseg ) );
    idb.SetFlagsFromGrbit( rgidescMSO[0].grbit );
    err = ErrCATAddTableIndex(
                    ppib,
                    pfucb,
                    objidFDPMSO,
                    szMSOIdIndex,
                    pgnoFDPMSO,
                    objidFDPMSO,
                    &idb,
                    idb.rgidxseg,
                    idb.rgidxsegConditional,
                    PSystemSpaceHints(eJSPHDefaultCatalog) );
    if( fInRepair && JET_errIndexDuplicate == err )
    {
        err = JET_errSuccess;
    }
    else
    {
        Call( err );
    }

    err = ErrCATAddTableIndex(
                    ppib,
                    pfucb,
                    objidFDPMSOShadow,
                    szMSOIdIndex,
                    pgnoFDPMSOShadow,
                    objidFDPMSOShadow,
                    &idb,
                    idb.rgidxseg,
                    idb.rgidxsegConditional,
                    PSystemSpaceHints(eJSPHDefaultCatalog) );
    if( fInRepair && JET_errIndexDuplicate == err )
    {
        err = JET_errSuccess;
    }
    else
    {
        Call( err );
    }

    Assert( FNORMLocaleNameDefault( idb.WszLocaleName() ) );
    Assert( dwLCMapFlagsDefault == idb.DwLCMapFlags() );
    Assert( 0 == idb.CidxsegConditional() );
    Assert( JET_cbKeyMost_OLD == idb.CbKeyMost() );
    Assert( JET_cbKeyMost_OLD == idb.CbVarSegMac() );
    idb.SetCidxseg( CfieldCATKeyString( rgidescMSO[1].szIdxKeys, idb.rgidxseg ) );
    idb.SetFlagsFromGrbit( rgidescMSO[1].grbit );
    err = ErrCATAddTableIndex(
                    ppib,
                    pfucb,
                    objidFDPMSO,
                    szMSONameIndex,
                    pgnoFDPMSO_NameIndex,
                    objidNameIndex,
                    &idb,
                    idb.rgidxseg,
                    idb.rgidxsegConditional,
                    PSystemSpaceHints(eJSPHDefaultCatalog) );
    if( fInRepair && JET_errIndexDuplicate == err )
    {
        err = JET_errSuccess;
    }
    else
    {
        Call( err );
    }

    Assert( FNORMLocaleNameDefault( idb.WszLocaleName() ) );
    Assert( dwLCMapFlagsDefault == idb.DwLCMapFlags() );
    Assert( 0 == idb.CidxsegConditional() );
    Assert( JET_cbKeyMost_OLD == idb.CbKeyMost() );
    Assert( JET_cbKeyMost_OLD == idb.CbVarSegMac() );
    idb.SetCidxseg( CfieldCATKeyString( rgidescMSO[2].szIdxKeys, idb.rgidxseg ) );
    idb.SetFlagsFromGrbit( rgidescMSO[2].grbit );
    err = ErrCATAddTableIndex(
                    ppib,
                    pfucb,
                    objidFDPMSO,
                    szMSORootObjectsIndex,
                    pgnoFDPMSO_RootObjectIndex,
                    objidRootObjectsIndex,
                    &idb,
                    idb.rgidxseg,
                    idb.rgidxsegConditional,
                    PSystemSpaceHints(eJSPHDefaultCatalog) );
    if( fInRepair && JET_errIndexDuplicate == err )
    {
        err = JET_errSuccess;
    }
    else
    {
        Call( err );
    }

    Assert( pfucb->u.pfcb->FInitialized() );
    Assert( pfucb->u.pfcb->FTypeTable() );
    Assert( pfucb->u.pfcb->FPrimaryIndex() );

    err = JET_errSuccess;

HandleError:
    if ( pfucbNil != pfucb )
    {
        CallS( ErrFILECloseTable( ppib, pfucb ) );
    }

    return err;
}


ERR ErrCATCreate( PIB *ppib, const IFMP ifmp, const BOOL fReplayCreateDbImplicitly )
{
    ERR     err;
    FUCB    *pfucb              = pfucbNil;
    PGNO    pgnoFDP;
    PGNO    pgnoFDPShadow;
    OBJID   objidFDP;
    OBJID   objidFDPShadow;
    OBJID   objidNameIndex;
    OBJID   objidRootObjectsIndex;

    FMP::AssertVALIDIFMP( ifmp );

    if ( FFMPIsTempDB( ifmp ) )
    {
        AssertSz( fFalse, "Temp DBs do not have Catalog/Schema tables." );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    //  If this fails, the entire createdb will fail
    Assert( 0 == ppib->Level() );
    Assert( g_rgfmp[ifmp].FCreatingDB() );

    CheckPIB( ppib );
    CheckDBID( ppib, ifmp );

    CallR( ErrDIRBeginTransaction( ppib, 63025, bitTransactionWritableDuringRecovery ) );

    //  allocate cursor
    //
    CallR( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucb ) );
    Assert( pfucbNil != pfucb );
    Assert( cpgMSOInitial > cpgTableMin );
    Call( ErrDIRCreateDirectory(
                pfucb,
                cpgMSOInitial,
                &pgnoFDP,
                &objidFDP,
                CPAGE::fPagePrimary,
                fSPMultipleExtent|fSPUnversionedExtent ) );
    Call( ErrDIRCreateDirectory(
                pfucb,
                fReplayCreateDbImplicitly ? cpgMSOShadowInitialLegacy : CpgCATShadowInitial( g_cbPage ),
                &pgnoFDPShadow,
                &objidFDPShadow,
                CPAGE::fPagePrimary,
                fSPMultipleExtent|fSPUnversionedExtent ) );
    DIRClose( pfucb );
    pfucb = pfucbNil;

    Assert( FCATSystemTable( pgnoFDP ) );
    Assert( PgnoCATTableFDP( szMSO ) == pgnoFDP );
    Assert( pgnoFDPMSO == pgnoFDP );
    Assert( objidFDPMSO == objidFDP );

    Assert( FCATSystemTable( pgnoFDPShadow ) );
    Assert( PgnoCATTableFDP( szMSOShadow ) == pgnoFDPShadow );
    Assert( pgnoFDPMSOShadow == pgnoFDPShadow );
    Assert( objidFDPMSOShadow == objidFDPShadow );

    CallR( ErrCATICreateCatalogIndexes( ppib, ifmp, &objidNameIndex, &objidRootObjectsIndex ) );

    Call( ErrREPAIRCATCreate( ppib, ifmp, objidNameIndex, objidRootObjectsIndex, fFalse /* fRepair */, fReplayCreateDbImplicitly ) );

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

    err = JET_errSuccess;

HandleError:
    if ( pfucbNil != pfucb )
    {
        DIRClose( pfucb );
    }

    if( err < 0 )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    return err;
}


/*=================================================================
ErrCATInsert

Description:

    Inserts a record into a system table when new tables, indexes,
    or columns are added to the database.

Parameters:

    PIB     *ppib;
    IFMP    ifmp;
    INT     itable;
    DATA    rgdata[];

Return Value:

    whatever error it encounters along the way

=================================================================*/

INLINE ERR ErrCATIInsert(
    PIB         *ppib,
    FUCB        *pfucbCatalog,
    DATA        rgdata[],
    const UINT  iHighestFixedToSet )
{
    ERR         err;
    TDB         * const ptdbCatalog     = pfucbCatalog->u.pfcb->Ptdb();

    CallR( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepInsert ) );

    //  set highest fixed column first, to eliminate calls to memmove()
    Enforce( iHighestFixedToSet > 0 );
    Enforce( iHighestFixedToSet < _countof( rgcdescMSO ) );
    Assert( FCOLUMNIDFixed( rgcdescMSO[iHighestFixedToSet].columnid ) );
    Assert( rgdata[iHighestFixedToSet].Cb() > 0 );
    CallS( ErrRECISetFixedColumn(
                pfucbCatalog,
                ptdbCatalog,
                rgcdescMSO[iHighestFixedToSet].columnid,
                rgdata+iHighestFixedToSet ) );

    for ( UINT i = 0; i < cColumnsMSO; i++ )
    {
        if ( rgdata[i].Cb() != 0 )
        {
            Assert( rgdata[i].Cb() > 0 );

            if ( FCOLUMNIDFixed( rgcdescMSO[i].columnid ) )
            {
                Assert( i <= iHighestFixedToSet );
                if ( iHighestFixedToSet != i )
                {
                    CallS( ErrRECISetFixedColumn(
                                pfucbCatalog,
                                ptdbCatalog,
                                rgcdescMSO[i].columnid,
                                rgdata+i ) );
                }
            }
            else if( FCOLUMNIDVar( rgcdescMSO[i].columnid ) )
            {
                Assert( i != iHighestFixedToSet );
                CallS( ErrRECISetVarColumn(
                            pfucbCatalog,
                            ptdbCatalog,
                            rgcdescMSO[i].columnid,
                            rgdata+i ) );
            }
            else
            {
                Assert( FCOLUMNIDTagged( rgcdescMSO[i].columnid ) );
                Assert( i != iHighestFixedToSet );
                //  currently all tagged fields must be Long-Values
                //  call ErrRECISetTaggedColumn if the column isn't
                Assert( rgcdescMSO[i].coltyp == JET_coltypLongText
                        || rgcdescMSO[i].coltyp == JET_coltypLongBinary );
                CallS( ErrRECSetLongField(
                            pfucbCatalog,
                            rgcdescMSO[i].columnid,
                            0,
                            rgdata+i,
                            compressNone ) );
            }
        }
    }

    /*  insert record into system table
    /**/
    err = ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT );
    if( err < 0 )
    {
        CallS( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepCancel ) );
    }
    return err;
}

LOCAL ERR ErrCATInsert(
    PIB         *ppib,
    FUCB        *pfucbCatalog,
    DATA        rgdata[],
    const UINT  iHighestFixedToSet )
{
    ERR         err;

    //  ensure we're in a transaction, so that on failure, updates to both
    //  catalog and its shadow are rolled back
    Assert( ppib->Level() > 0
        || g_rgfmp[pfucbCatalog->ifmp].FCreatingDB() );

    CallR( ErrCATIInsert( ppib, pfucbCatalog, rgdata, iHighestFixedToSet ) );

    if ( !g_rgfmp[pfucbCatalog->u.pfcb->Ifmp()].FShadowingOff() )
    {
        FUCB    *pfucbShadow;
        CallR( ErrCATOpen( ppib, pfucbCatalog->u.pfcb->Ifmp(), &pfucbShadow, fTrue ) );
        Assert( pfucbNil != pfucbShadow );

        err = ErrCATIInsert( ppib, pfucbShadow, rgdata, iHighestFixedToSet );

        CallS( ErrCATClose( ppib, pfucbShadow ) );
    }

    return err;
}


ERR ErrCATAddTable(
    PIB             *ppib,
    FUCB            *pfucbCatalog,
    const PGNO      pgnoTableFDP,
    const OBJID     objidTable,
    const CHAR      *szTableName,
    const CHAR      *szTemplateTableName,
    const ULONG     ulFlags,
    const JET_SPACEHINTS * const pSpacehints,
    const ULONG     cbSeparateLV,
    const JET_SPACEHINTS * const pSpacehintsLV,
    const ULONG     cbLVChunkMax )
{
    DATA            rgdata[idataMSOMax];
    BYTE            rgExtendeSpaceHintBuffer[cbExtendedSpaceHints];
    BYTE            rgDefLvExtendeSpaceHintBuffer[cbExtendedSpaceHints];
    CPG             cpgInitial;
    const SYSOBJ    sysobj              = sysobjTable;
    const BYTE      bTrue               = 0xff;

    Assert( objidTable > objidSystemRoot );
    Assert( pSpacehints );

    //  must zero out to ensure unused fields are ignored
    memset( rgdata, 0, sizeof(rgdata) );

    rgdata[iMSO_ObjidTable].SetPv(      (BYTE *)&objidTable );
    rgdata[iMSO_ObjidTable].SetCb(      sizeof(objidTable) );

    rgdata[iMSO_Type].SetPv(            (BYTE *)&sysobj );
    rgdata[iMSO_Type].SetCb(            sizeof(sysobj) );

    rgdata[iMSO_Id].SetPv(              (BYTE *)&objidTable );
    rgdata[iMSO_Id].SetCb(              sizeof(objidTable) );

    rgdata[iMSO_Name].SetPv(            (BYTE *)szTableName );
    rgdata[iMSO_Name].SetCb(            strlen(szTableName) );

    rgdata[iMSO_PgnoFDP].SetPv(         (BYTE *)&pgnoTableFDP );
    rgdata[iMSO_PgnoFDP].SetCb(         sizeof(pgnoTableFDP) );

    rgdata[iMSO_Flags].SetPv(           (BYTE *)&ulFlags );
    rgdata[iMSO_Flags].SetCb(           sizeof(ulFlags) );

    Assert( 0 == rgdata[iMSO_Stats].Cb() );

    rgdata[iMSO_RootFlag].SetPv(        (BYTE *)&bTrue );
    rgdata[iMSO_RootFlag].SetCb(        sizeof(bTrue) );

    if ( NULL != szTemplateTableName )
    {
        rgdata[iMSO_TemplateTable].SetPv( (BYTE *)szTemplateTableName );
        rgdata[iMSO_TemplateTable].SetCb( strlen(szTemplateTableName) );
    }
    else
    {
        Assert( 0 == rgdata[iMSO_TemplateTable ].Cb() );
    }

    //  Primary index space hints.
    //
    //  Note: these are all handled by the ErrCATISetSpaceHints()
    //rgdata[iMSO_SpaceUsage].SetPv(        (BYTE *)&ulDensity );
    //rgdata[iMSO_SpaceUsage].SetCb(        sizeof(ulDensity) );
    //rgdata[iMSO_Pages].SetPv(         (BYTE *)&ulPages );
    //rgdata[iMSO_Pages].SetCb(         sizeof(ulPages) );
    Assert( pSpacehints->cbInitial % g_cbPage == 0 ); // should've been normalized by now.
    cpgInitial = pSpacehints->cbInitial / g_cbPage;
    Assert( pSpacehints->ulInitialDensity == UlBound( pSpacehints->ulInitialDensity, ulFILEDensityLeast, ulFILEDensityMost ) );
    ERR err = ErrCATISetSpaceHints( pfucbCatalog, sysobjTable, fFalse, pSpacehints, &cpgInitial,
                            rgExtendeSpaceHintBuffer, sizeof(rgExtendeSpaceHintBuffer),
                            rgdata, _countof(rgdata) );
    Assert( !FCATHasExtendedHints( sysobjTable, pSpacehints ) ||
            !FCATISystemObjid( objidTable ) );
    CallS( err );
    CallR( err );

    if ( pSpacehintsLV )
    {
        //  Deferred index space hints
        Assert( pSpacehints->cbInitial % g_cbPage == 0 ); // should've been normalized by now.
        err = ErrCATISetSpaceHints( pfucbCatalog, sysobjTable, fTrue, pSpacehintsLV, &cpgInitial,
                                rgDefLvExtendeSpaceHintBuffer, sizeof(rgDefLvExtendeSpaceHintBuffer),
                                rgdata, _countof(rgdata) );
        Assert( !FCATHasExtendedHints( sysobjTable, pSpacehints ) ||
                !FCATISystemObjid( objidTable ) );
        CallS( err );
        CallR( err );
    }

    //  Heuristic LV burst threshold
    //
    if ( cbSeparateLV )
    {
        Assert( !FCATISystemObjid( objidTable ) );
        rgdata[ iMSO_SeparateLVThreshold ].SetPv(   (BYTE*)&cbSeparateLV );
        rgdata[ iMSO_SeparateLVThreshold ].SetCb(   sizeof(cbSeparateLV) );
    }
    else
    {
        Assert( 0 == rgdata[ iMSO_SeparateLVThreshold ].Cb() );
    }

    // We do not allow arbitrary large chunk size right now
    if ( cbLVChunkMax > (ULONG)UlParam( JET_paramLVChunkSizeMost ) )
    {
        return ErrERRCheck( JET_errInvalidLVChunkSize );
    }
    if ( cbLVChunkMax )
    {
        Assert( !FCATISystemObjid( objidTable ) );
        rgdata[ iMSO_LVChunkMax ].SetPv( (BYTE*)&cbLVChunkMax );
        rgdata[ iMSO_LVChunkMax ].SetCb( sizeof(cbLVChunkMax) );
    }
    else
    {
        Assert( 0 == rgdata[ iMSO_LVChunkMax ].Cb() );
    }

    err = ErrCATInsert( ppib, pfucbCatalog, rgdata, cbLVChunkMax ? iMSO_LVChunkMax : iMSO_RootFlag );
    if ( JET_errKeyDuplicate == err )
    {
        if ( 0 == strcmp( szTableName, "MSysDefrag1" ) )
        {
            FireWall( "CATAddTableKeyDuplicate" );
        }

        err = ErrERRCheck( JET_errTableDuplicate );
    }

    if ( err >= JET_errSuccess )
    {
        err = ErrCATInsertMSObjidsRecord( ppib, pfucbCatalog->ifmp, objidTable, objidTable, sysobj );
    }

    return err;
}


ERR ErrCATAddTableColumn(
    PIB                 *ppib,
    FUCB                *pfucbCatalog,
    const OBJID         objidTable,
    const CHAR          *szColumnName,
    COLUMNID            columnid,
    const FIELD         *pfield,
    const VOID          *pvDefault,
    const ULONG         cbDefault,
    const CHAR          * const szCallback,
    const VOID          * const pvUserData,
    const ULONG         cbUserData )
{
    DATA                rgdata[idataMSOMax];
    UINT                iHighestFixedToSet;
    const SYSOBJ        sysobj              = sysobjColumn;
    const JET_COLTYP    coltyp              = pfield->coltyp;
    const ULONG         ulCodePage          = pfield->cp;

    //  filter out flags that shouldn't be persisted
    Assert( !FFIELDDeleted( pfield->ffield ) );
    const ULONG         ulFlags             = ( pfield->ffield & ffieldPersistedMask );

    Assert( objidTable > objidSystemRoot );


    //  must zero out to ensure unused fields are ignored
    memset( rgdata, 0, sizeof(rgdata) );

    rgdata[iMSO_ObjidTable].SetPv(      (BYTE *)&objidTable );
    rgdata[iMSO_ObjidTable].SetCb(      sizeof(objidTable) );

    rgdata[iMSO_Type].SetPv(            (BYTE *)&sysobj );
    rgdata[iMSO_Type].SetCb(            sizeof(sysobj) );

    Assert( FCOLUMNIDValid( columnid ) );
    Assert( !FCOLUMNIDTemplateColumn( columnid )
        || FFIELDTemplateColumnESE98( pfield->ffield ) );
    COLUMNIDResetFTemplateColumn( columnid );
    rgdata[iMSO_Id].SetPv(              (BYTE *)&columnid );
    rgdata[iMSO_Id].SetCb(              sizeof(columnid) );

    rgdata[iMSO_Name].SetPv(            (BYTE *)szColumnName );
    rgdata[iMSO_Name].SetCb(            strlen(szColumnName) );

    rgdata[iMSO_Coltyp].SetPv(          (BYTE *)&coltyp );
    rgdata[iMSO_Coltyp].SetCb(          sizeof(coltyp) );

    rgdata[iMSO_SpaceUsage].SetPv(      (BYTE *)&pfield->cbMaxLen );
    rgdata[iMSO_SpaceUsage].SetCb(      sizeof(pfield->cbMaxLen) );

    rgdata[iMSO_Flags].SetPv(           (BYTE *)&ulFlags );
    rgdata[iMSO_Flags].SetCb(           sizeof(ulFlags) );

    rgdata[iMSO_Localization].SetPv(    (BYTE *)&ulCodePage );
    rgdata[iMSO_Localization].SetCb(    sizeof(ulCodePage) );

    rgdata[iMSO_Callback].SetPv(        (BYTE*)szCallback );
    rgdata[iMSO_Callback].SetCb(        ( szCallback ? strlen(szCallback) : 0 ) );

    rgdata[iMSO_DefaultValue].SetPv(    (BYTE *)pvDefault );
    rgdata[iMSO_DefaultValue].SetCb(    cbDefault );

    rgdata[iMSO_CallbackData].SetPv(    (BYTE*)pvUserData );
    rgdata[iMSO_CallbackData].SetCb(    cbUserData );

    if ( FCOLUMNIDFixed( columnid ) )
    {
        Assert( pfield->ibRecordOffset >= ibRECStartFixedColumns );
        rgdata[iMSO_RecordOffset].SetPv( (BYTE *)&pfield->ibRecordOffset );
        rgdata[iMSO_RecordOffset].SetCb( sizeof(pfield->ibRecordOffset) );
        iHighestFixedToSet = iMSO_RecordOffset;
    }
    else
    {
        // Don't need to persist record offsets for var/tagged columns.
        Assert( FCOLUMNIDVar( columnid ) || FCOLUMNIDTagged( columnid ) );
        Assert( 0 == rgdata[iMSO_RecordOffset].Cb() );
        iHighestFixedToSet = iMSO_Localization;
    }

    ERR err = ErrCATInsert( ppib, pfucbCatalog, rgdata, iHighestFixedToSet );
    if ( JET_errKeyDuplicate == err )
        err = ErrERRCheck( JET_errColumnDuplicate );

    return err;
}


ERR ErrCATAddTableIndex(
    PIB                 *ppib,
    FUCB                *pfucbCatalog,
    const OBJID         objidTable,
    const CHAR          *szIndexName,
    const PGNO          pgnoIndexFDP,
    const OBJID         objidIndex,
    const IDB           *pidb,
    const IDXSEG* const rgidxseg,
    const IDXSEG* const rgidxsegConditional,
    const JET_SPACEHINTS * const pspacehints )
{
    ERR                         err = JET_errSuccess;
    DATA                        rgdata[idataMSOMax];
    BYTE                        rgExtendeSpaceHintBuffer[cbExtendedSpaceHints];
    LE_IDXFLAG                  le_idxflag;
    const SYSOBJ                    sysobj = sysobjIndex;
    INT                         idataT = 0;
    UnalignedLittleEndian<USHORT>   le_cbKeyMost;

    Assert( objidTable > objidSystemRoot );
    Assert( objidIndex > objidSystemRoot );

    //  objids are monotonically increasing, so an index should
    //  always have higher objid than its table
    Assert( ( pidb->FPrimary() && objidIndex == objidTable )
        || ( !pidb->FPrimary() && objidIndex > objidTable ) );


    //  must zero out to ensure unused fields are ignored
    memset( rgdata, 0, sizeof(rgdata) );

    rgdata[iMSO_ObjidTable].SetPv(      (BYTE *)&objidTable );
    rgdata[iMSO_ObjidTable].SetCb(      sizeof(objidTable) );

    rgdata[iMSO_Type].SetPv(            (BYTE *)&sysobj );
    rgdata[iMSO_Type].SetCb(            sizeof(sysobj) );

    rgdata[iMSO_Id].SetPv(              (BYTE *)&objidIndex );
    rgdata[iMSO_Id].SetCb(              sizeof(objidIndex) );

    rgdata[iMSO_Name].SetPv(            (BYTE *)szIndexName );
    rgdata[iMSO_Name].SetCb(            strlen(szIndexName) );

    rgdata[iMSO_PgnoFDP].SetPv(         (BYTE *)&pgnoIndexFDP );
    rgdata[iMSO_PgnoFDP].SetCb(         sizeof(pgnoIndexFDP) );

    Assert( pspacehints );
    Assert( pspacehints->cbInitial % g_cbPage == 0 );
    err = ErrCATISetSpaceHints( pfucbCatalog, sysobjIndex, fFalse, pspacehints, NULL,
                rgExtendeSpaceHintBuffer, sizeof(rgExtendeSpaceHintBuffer),
                rgdata, _countof(rgdata) );
    if ( err < JET_errSuccess )
    {
        return err; // game over.
    }
    Assert( rgdata[iMSO_SpaceUsage].Cb() == 4 );
    Assert( 0 == memcmp( rgdata[iMSO_SpaceUsage].Pv(), &(pspacehints->ulInitialDensity), 4 ) );

    le_idxflag.fidb = pidb->FPersistedFlags();
    le_idxflag.fIDXFlags = ( pidb->FPersistedFlagsX() | fIDXExtendedColumns );

    //  Hack on this field: SetColumn() will convert the fixed
    //  columns. So convert it here so that later it can be
    //  converted back to current value.
    LONG        l           = *(LONG *)&le_idxflag;
    l = ReverseBytesOnBE( l );

    rgdata[iMSO_Flags].SetPv(           (BYTE *)&l );
    rgdata[iMSO_Flags].SetCb(           sizeof(l) );

    const PCWSTR wszLocaleName = pidb->WszLocaleName();

    // Pre E15/Win8 we stored the LCID in iMSO_Localization, now we use iMSO_LocaleName
    Assert( rgdata[iMSO_LocaleName].Pv() == NULL && rgdata[iMSO_LocaleName].Cb() == 0 );
    rgdata[iMSO_LocaleName].SetPv( (BYTE *)wszLocaleName );
    rgdata[iMSO_LocaleName].SetCb( LOSStrLengthW( wszLocaleName ) * sizeof(WCHAR) );

    // iMSO_Localization is no longer used, but rgcdescMSO defines it as JET_bitColumnNotNull,
    // so we'll honour that.
    LCID lcidZero = 0;
    rgdata[iMSO_Localization].SetPv(    (BYTE *)&lcidZero );
    rgdata[iMSO_Localization].SetCb(    sizeof(lcidZero) );

    DWORD       dwMapFlags  = pidb->DwLCMapFlags();
    rgdata[iMSO_LCMapFlags].SetPv(      (BYTE *)&dwMapFlags );
    rgdata[iMSO_LCMapFlags].SetCb(      sizeof(dwMapFlags) );

    BYTE        *pbidxseg;
    LE_IDXSEG   le_rgidxseg[ JET_ccolKeyMost ];
    LE_IDXSEG   le_rgidxsegConditional[ JET_ccolKeyMost ];

    if ( FHostIsLittleEndian() )
    {
        pbidxseg = (BYTE *)rgidxseg;

#ifdef DEBUG
        for ( UINT iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
        {
            Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );
        }
#endif
    }
    else
    {
        for ( UINT iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
        {
            Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );

            //  Endian conversion
            le_rgidxseg[ iidxseg ] = rgidxseg[iidxseg];
        }

        pbidxseg = (BYTE *)le_rgidxseg;
    }

    rgdata[iMSO_KeyFldIDs].SetPv( pbidxseg );
    rgdata[iMSO_KeyFldIDs].SetCb( pidb->Cidxseg() * sizeof(IDXSEG) );

    if ( FHostIsLittleEndian() )
    {
        pbidxseg = (BYTE *)rgidxsegConditional;

#ifdef DEBUG
        for ( UINT iidxseg = 0; iidxseg < pidb->CidxsegConditional(); iidxseg++ )
        {
            //  verify no longer persisting old format
            Assert( FCOLUMNIDValid( rgidxsegConditional[iidxseg].Columnid() ) );
        }
#endif
    }
    else
    {
        for ( UINT iidxseg = 0; iidxseg < pidb->CidxsegConditional(); iidxseg++ )
        {
            Assert( FCOLUMNIDValid( rgidxsegConditional[iidxseg].Columnid() ) );

            //  Endian conversion
            le_rgidxsegConditional[ iidxseg ] = rgidxsegConditional[iidxseg];
        }
        pbidxseg = (BYTE *)le_rgidxsegConditional;
    }

    rgdata[iMSO_ConditionalColumns].SetPv(  pbidxseg );
    rgdata[iMSO_ConditionalColumns].SetCb(  pidb->CidxsegConditional() * sizeof(IDXSEG) );

    UnalignedLittleEndian<USHORT>   le_cbVarSegMac = pidb->CbVarSegMac();
    if ( pidb->CbVarSegMac() != pidb->CbKeyMost() )
    {
        rgdata[iMSO_VarSegMac].SetPv( (BYTE *)&le_cbVarSegMac );
        rgdata[iMSO_VarSegMac].SetCb( sizeof(le_cbVarSegMac) );
    }
    else
    {
        Assert( pidb->CbVarSegMac() == pidb->CbKeyMost() );
        Assert( 0 == rgdata[iMSO_VarSegMac].Cb() );
    }

    LE_TUPLELIMITS  le_tuplelimits;
    if ( pidb->FTuples() )
    {
        le_tuplelimits.le_chLengthMin = pidb->CchTuplesLengthMin();
        le_tuplelimits.le_chLengthMax = pidb->CchTuplesLengthMax();
        le_tuplelimits.le_chToIndexMax = pidb->IchTuplesToIndexMax();
        le_tuplelimits.le_cchIncrement = pidb->CchTuplesIncrement();
        le_tuplelimits.le_ichStart = pidb->IchTuplesStart();

        rgdata[iMSO_TupleLimits].SetPv( (BYTE *)&le_tuplelimits );
        rgdata[iMSO_TupleLimits].SetCb( sizeof(le_tuplelimits) );
    }
    else
    {
        Assert( 0 == rgdata[iMSO_TupleLimits].Cb() );
    }

    QWORD qwSortVersion = 0;
    if( pidb->FLocalizedText() )
    {
        SORTID sortID;

        Call( ErrNORMGetSortVersion( pidb->WszLocaleName(), &qwSortVersion, &sortID ) );

        rgdata[iMSO_Version].SetPv( &qwSortVersion );
        rgdata[iMSO_Version].SetCb( sizeof( qwSortVersion ) );

        rgdata[iMSO_SortID].SetPv( &sortID );
        rgdata[iMSO_SortID].SetCb( sizeof( sortID ) );

        Call( ErrCATIAddLocale( ppib, pfucbCatalog->ifmp, pidb->WszLocaleName(), sortID, qwSortVersion ) );
    }

    le_cbKeyMost = pidb->CbKeyMost();
    if ( pidb->CbKeyMost() != JET_cbKeyMost_OLD )
    {
        rgdata[iMSO_KeyMost].SetPv( (BYTE *)&le_cbKeyMost );
        rgdata[iMSO_KeyMost].SetCb( sizeof(le_cbKeyMost) );
    }
    else
    {
        Assert( 0 == rgdata[iMSO_KeyMost].Cb() );
    }

    //  only set fixed columns if higher fixed column is set to non-NULL value
    //
    for ( idataT = iMSO_KeyMost; ( idataT > 0 && ( 0 == rgdata[idataT].Cb() ) ); idataT-- )
        ;
    Assert( idataT > 0 );
    err = ErrCATInsert( ppib, pfucbCatalog, rgdata, idataT );
    if ( JET_errKeyDuplicate == err )
        err = ErrERRCheck( JET_errIndexDuplicate );

    // We don't need to create an entry for the primary index
    if ( err >= JET_errSuccess && objidTable != objidIndex )
    {
        err = ErrCATInsertMSObjidsRecord( ppib, pfucbCatalog->ifmp, objidIndex, objidTable, sysobj );
    }

HandleError:
    return err;
}

ERR ErrCATIAddTableLV(
    PIB             *ppib,
    FUCB            *pfucbCatalog,
    const OBJID     objidTable,
    const PGNO      pgnoLVFDP,
    const OBJID     objidLV )
{
    DATA            rgdata[idataMSOMax];
    const SYSOBJ    sysobj              = sysobjLongValue;
    const ULONG     ulPages             = cpgLVTree;
    const ULONG     ulDensity           = ulFILEDensityMost;
    const ULONG     ulFlagsNil          = 0;

    //  objids are monotonically increasing, so LV should
    //  always have higher objid than its table
    Assert( objidLV > objidTable );

    //  must zero out to ensure unused fields are ignored
    memset( rgdata, 0, sizeof(rgdata) );

    rgdata[iMSO_ObjidTable].SetPv(      (BYTE *)&objidTable );
    rgdata[iMSO_ObjidTable].SetCb(      sizeof(objidTable) );

    rgdata[iMSO_Type].SetPv(            (BYTE *)&sysobj );
    rgdata[iMSO_Type].SetCb(            sizeof(sysobj) );

    rgdata[iMSO_Id].SetPv(              (BYTE *)&objidLV );
    rgdata[iMSO_Id].SetCb(              sizeof(objidLV) );

    rgdata[iMSO_Name].SetPv(            (BYTE *)szLVRoot );
    rgdata[iMSO_Name].SetCb(            cbLVRoot );

    rgdata[iMSO_PgnoFDP].SetPv(         (BYTE *)&pgnoLVFDP );
    rgdata[iMSO_PgnoFDP].SetCb(         sizeof(pgnoLVFDP) );

    rgdata[iMSO_SpaceUsage].SetPv(      (BYTE *)&ulDensity );
    rgdata[iMSO_SpaceUsage].SetCb(      sizeof(ulDensity) );

    rgdata[iMSO_Flags].SetPv(           (BYTE *)&ulFlagsNil );
    rgdata[iMSO_Flags].SetCb(           sizeof(ulFlagsNil) );

    rgdata[iMSO_Pages].SetPv(           (BYTE *)&ulPages );
    rgdata[iMSO_Pages].SetCb(           sizeof(ulPages) );

    ERR err = ErrCATInsert( ppib, pfucbCatalog, rgdata, iMSO_Pages );

    if ( err >= JET_errSuccess )
    {
        err = ErrCATInsertMSObjidsRecord( ppib, pfucbCatalog->ifmp, objidLV, objidTable, sysobj );
    }

    return err;
}


//  ================================================================
ERR ErrCATAddTableCallback(
    PIB                 *ppib,
    FUCB                *pfucbCatalog,
    const OBJID         objidTable,
    const JET_CBTYP     cbtyp,
    const CHAR * const  szCallback )
//  ================================================================
{
    Assert( objidNil != objidTable );
    Assert( NULL != szCallback );
    Assert( cbtyp > JET_cbtypNull );

    DATA            rgdata[idataMSOMax];
    const SYSOBJ    sysobj              = sysobjCallback;
    const ULONG     ulId                = sysobjCallback;
    const ULONG     ulFlags             = cbtyp;
    const ULONG     ulNil               = 0;

    //  must zero out to ensure unused fields are ignored
    memset( rgdata, 0, sizeof(rgdata) );

    rgdata[iMSO_ObjidTable].SetPv(      (BYTE *)&objidTable );
    rgdata[iMSO_ObjidTable].SetCb(      sizeof(objidTable) );

    rgdata[iMSO_Type].SetPv(            (BYTE *)&sysobj );
    rgdata[iMSO_Type].SetCb(            sizeof(sysobj) );

    rgdata[iMSO_Id].SetPv(              (BYTE *)&ulId );
    rgdata[iMSO_Id].SetCb(              sizeof(ulId) );

    rgdata[iMSO_Name].SetPv(            (BYTE *)szTableCallback );
    rgdata[iMSO_Name].SetCb(            cbTableCallback );

    rgdata[iMSO_PgnoFDP].SetPv(         (BYTE *)&ulNil );
    rgdata[iMSO_PgnoFDP].SetCb(         sizeof(ulNil) );

    rgdata[iMSO_SpaceUsage].SetPv(      (BYTE *)&ulNil );
    rgdata[iMSO_SpaceUsage].SetCb(      sizeof(ulNil) );

    rgdata[iMSO_Flags].SetPv(           (BYTE *)&ulFlags );
    rgdata[iMSO_Flags].SetCb(           sizeof(ulFlags) );

    rgdata[iMSO_Pages].SetPv(           (BYTE *)&ulNil );
    rgdata[iMSO_Pages].SetCb(           sizeof(ulNil) );

    rgdata[iMSO_Callback].SetPv(        (BYTE *)szCallback );
    rgdata[iMSO_Callback].SetCb(        strlen( szCallback ) );

    return ErrCATInsert( ppib, pfucbCatalog, rgdata, iMSO_Pages );
}


LOCAL ERR ErrCATISeekTable(
    PIB         *ppib,
    FUCB        *pfucbCatalog,
    const CHAR  *szTableName )
{
    ERR         err;
    const BYTE  bTrue       = 0xff;

    //  should be on the primary index
    Assert( pfucbNil != pfucbCatalog );
    Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    //  switch to secondary index
    Call( ErrIsamSetCurrentIndex(
                    (JET_SESID)ppib,
                    (JET_TABLEID)pfucbCatalog,
                    szMSORootObjectsIndex ) );

    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                &bTrue,
                sizeof(bTrue),
                JET_bitNewKey ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)szTableName,
                (ULONG)strlen(szTableName),
                NO_GRBIT ) );
    err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );
    if ( JET_errRecordNotFound == err )
        err = ErrERRCheck( JET_errObjectNotFound );
    Call( err );
    CallS( err );

    Assert( !Pcsr( pfucbCatalog )->FLatched() );
    Call( ErrDIRGet( pfucbCatalog ) );

#ifdef DEBUG
{
    //  verify this is a table
    DATA    dataField;
    Assert( fidMSO_Type.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Type,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(SYSOBJ) );
    Assert( sysobjTable == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );
}
#endif

HandleError:
    return err;
}


LOCAL ERR ErrCATISeekTableObject(
    PIB             *ppib,
    FUCB            *pfucbCatalog,
    const OBJID     objidTable,
    const SYSOBJ    sysobj,
    const OBJID     objid )
{
    ERR             err;

    Assert( sysobjColumn == sysobj
        || sysobjIndex == sysobj
        || sysobjTable == sysobj
        || sysobjLongValue == sysobj );

    //  should be on the primary index, which is the Id index
    Assert( pfucbNil != pfucbCatalog );
    Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&objidTable,
                sizeof(objidTable),
                JET_bitNewKey ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&sysobj,
                sizeof(sysobj),
                NO_GRBIT ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&objid,
                sizeof(objid),
                NO_GRBIT ) );

    err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );
    Assert( err <= JET_errSuccess );    //  SeekEQ shouldn't return warnings

    if ( JET_errRecordNotFound == err )
    {
        switch ( sysobj )
        {
            case sysobjColumn:
                err = ErrERRCheck( JET_errColumnNotFound );
                break;
            case sysobjIndex:
                err = ErrERRCheck( JET_errIndexNotFound );
                break;

            default:
                AssertSz( fFalse, "Invalid CATALOG object" );
                //  FALL THROUGH:
            case sysobjTable:
            case sysobjLongValue:
                err = ErrERRCheck( JET_errObjectNotFound );
                break;
        }
    }
    else if ( JET_errSuccess == err )
    {
        Assert( !Pcsr( pfucbCatalog )->FLatched() );
        Call( ErrDIRGet( pfucbCatalog ) );
    }

HandleError:
    return err;
}


LOCAL ERR ErrCATISeekTable(
    PIB         *ppib,
    FUCB        *pfucbCatalog,
    const OBJID objidTable )
{
    return ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjTable, objidTable );
}

//  ================================================================
ERR ErrCATSeekObjectByObjid(
    _In_ PIB* const     ppib,
    _In_ const IFMP     ifmp,
    _In_ const OBJID    objidTable,
    _In_ const SYSOBJ   sysobj,
    _In_ const OBJID    objid,
    _Out_writes_( cchName ) PSTR const szName,
    _In_ const INT      cchName,
    _Out_ PGNO* const   ppgnoFDP )
//  ================================================================
{
    ERR err = JET_errSuccess;
    FUCB* pfucbCatalog = pfucbNil;
    DATA dataField;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobj, objid ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    if ( NULL != ppgnoFDP )
    {
        Assert( fidMSO_PgnoFDP.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_PgnoFDP,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(PGNO) );
        *ppgnoFDP = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
    }

    if ( ( NULL != szName ) && ( 0 != cchName ) )
    {
        Assert( fidMSO_Name.FVar() );
        Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Name,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
        CallS( err );

        const INT cbName = cchName * sizeof( szName[0] );
        const INT cbDataField = dataField.Cb();

        if ( cbDataField < 0 || cbDataField > cbName )
        {
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"15a0c968-5598-4498-b3ba-50a861afe852" );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }

        memset( szName, '\0', cbName );
        memcpy( szName, dataField.Pv(), cbDataField );
    }

HandleError:
    if ( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }

    return err;
}

//  ================================================================
ERR ErrCATSeekTable(
    _In_ PIB                *ppib,
    _In_ const IFMP         ifmp,
    _In_z_ const CHAR       *szTableName,
    _Out_opt_ PGNO          *ppgnoTableFDP,
    _Out_opt_ OBJID         *pobjidTable )
//  ================================================================
{
    ERR     err;
    FUCB    *pfucbCatalog = pfucbNil;
    DATA    dataField;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTable( ppib, pfucbCatalog, szTableName ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    if ( NULL != ppgnoTableFDP )
    {
        Assert( fidMSO_PgnoFDP.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_PgnoFDP,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(PGNO) );
        *ppgnoTableFDP = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
//      UtilMemCpy( ppgnoTableFDP, dataField.Pv(), sizeof(PGNO) );
    }

    if ( NULL != pobjidTable )
    {
        Assert( fidMSO_Id.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Id,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(OBJID) );
        *pobjidTable = *(UnalignedLittleEndian< OBJID > *) dataField.Pv();
//      UtilMemCpy( pobjidTable, dataField.Pv(), sizeof(OBJID) );
    }

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    return err;
}


//  ================================================================
ERR ErrCATSeekTableByObjid(
    IN PIB          * const ppib,
    IN const IFMP   ifmp,
    IN const OBJID  objidTable,
    __out_ecount_z( cchTableName ) PSTR const szTableName,
    IN const INT    cchTableName,
    OUT PGNO        * const ppgnoTableFDP )
//  ================================================================
{
    ERR err = JET_errSuccess;
    const BOOL fOpeningSys = FCATSystemTableByObjid( objidTable );

    if ( fOpeningSys )
    {
        // If we're opening a system table, then we don't need to do a seek.
        // (And potentially cause a deadlock, particularly with dbscan. If we are
        // scanning the catalog, and trying to open the table, then the Seek can
        // try to latch a page that's already latched by dbscan.)
        const PGNO pgnoFDPSys = PgnoCATTableFDP( objidTable );
        *ppgnoTableFDP = pgnoFDPSys;

        if ( NULL != szTableName && 0 != cchTableName )
        {
            switch ( objidTable )
            {
                case objidFDPMSO:
                    OSStrCbCopyA( szTableName, cchTableName, szMSO );
                    break;
                case objidFDPMSOShadow:
                    OSStrCbCopyA( szTableName, cchTableName, szMSOShadow );
                    break;
                default:
                    AssertSz( fFalse, "Unexpected system table!" );
                    err = ErrERRCheck( JET_errInternalError );
                    break;
            }
        }

        goto HandleError;
    }

    Call( ErrCATSeekObjectByObjid( ppib, ifmp, objidTable, sysobjTable, objidTable, szTableName, cchTableName, ppgnoTableFDP ) );

HandleError:
    return err;
}

LOCAL ERR ErrCATISeekTableObject(
    PIB             *ppib,
    FUCB            *pfucbCatalog,
    const OBJID     objidTable,
    const SYSOBJ    sysobj,
    const CHAR      *szName )
{
    ERR             err;

    Assert( sysobjColumn == sysobj
        || sysobjIndex == sysobj
        || sysobjLongValue == sysobj );

    //  should be on the primary index
    Assert( pfucbNil != pfucbCatalog );
    Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    //  switch to secondary index
    Call( ErrIsamSetCurrentIndex(
                (JET_SESID)ppib,
                (JET_TABLEID)pfucbCatalog,
                szMSONameIndex ) );

    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&objidTable,
                sizeof(objidTable),
                JET_bitNewKey ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&sysobj,
                sizeof(sysobj),
                NO_GRBIT ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)szName,
                (ULONG)strlen(szName),
                NO_GRBIT ) );
    err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );
    Assert( err <= JET_errSuccess );    //  SeekEQ shouldn't return warnings
    if ( JET_errRecordNotFound == err )
    {
        switch ( sysobj )
        {
            case sysobjColumn:
                err = ErrERRCheck( JET_errColumnNotFound );
                break;
            case sysobjIndex:
                err = ErrERRCheck( JET_errIndexNotFound );
                break;

            default:
                AssertSz( fFalse, "Invalid CATALOG object" );
                //  FALL THROUGH:
            case sysobjLongValue:
                err = ErrERRCheck( JET_errObjectNotFound );
                break;
        }
    }
    else if ( JET_errSuccess == err )
    {
        Assert( !Pcsr( pfucbCatalog )->FLatched() );
        Call( ErrDIRGet( pfucbCatalog ) );
    }

HandleError:
    return err;
}


LOCAL ERR ErrCATIDeleteTable( PIB *ppib, const IFMP ifmp, const OBJID objidTable, const BOOL fShadow )
{
    ERR     err;
    FUCB    *pfucbCatalog   = pfucbNil;

    Assert( ppib->Level() > 0 );

    Assert( objidTable > objidSystemRoot );

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fShadow ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTable(
                ppib,
                pfucbCatalog,
                objidTable ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );
    CallS( ErrDIRRelease( pfucbCatalog ) );

    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&objidTable,
                sizeof(objidTable),
                JET_bitNewKey | JET_bitStrLimit ) );

    err = ErrIsamSetIndexRange( ppib, pfucbCatalog, JET_bitRangeUpperLimit );
    Assert( JET_errNoCurrentRecord != err );

    do
    {
        Call( err );

        if ( !fShadow )
        {
            Call( ErrCATPossiblyDeleteMSObjidsRecord( ppib, pfucbCatalog ) );
        }

        Call( ErrIsamDelete( ppib, pfucbCatalog ) );
        err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
    }
    while ( JET_errNoCurrentRecord != err );

    err = JET_errSuccess;

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    return err;
}


ERR ErrCATDeleteTable( PIB *ppib, const IFMP ifmp, const OBJID objidTable )
{
    ERR         err;

    //  ensure we're in a transaction, so that on failure, updates to both
    //  catalog and its shadow are rolled back
    Assert( ppib->Level() > 0 );

    CallR( ErrCATIDeleteTable( ppib, ifmp, objidTable, fFalse ) );

    if ( !g_rgfmp[ifmp].FShadowingOff() )
    {
#ifdef DEBUG
        const ERR   errSave     = err;
#endif

        err = ErrCATIDeleteTable( ppib, ifmp, objidTable, fTrue );

        Assert( JET_errObjectNotFound != err );     // would have been detected in regular catalog
        Assert( err < 0 || errSave == err );
    }

    return err;
}


/*  replaces the value in a column of a record of a system table.
/**/
LOCAL ERR ErrCATIDeleteTableColumn(
    PIB             *ppib,
    const IFMP      ifmp,
    const OBJID     objidTable,
    const CHAR      *szColumnName,
    COLUMNID        *pcolumnid,
    const BOOL      fShadow )
{
    ERR             err;
    FUCB            *pfucbCatalog   = pfucbNil;
    DATA            dataField;
    JET_COLUMNID    columnid;
    JET_COLTYP      coltyp;
    FIELDFLAG       ffield;
    CHAR            szStubName[cbMaxDeletedColumnStubName];

    Assert( objidTable > objidSystemRoot );

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fShadow ) );
    Assert( pfucbNil != pfucbCatalog );

    if ( fShadow )
    {
        Assert( !FCOLUMNIDTemplateColumn( *pcolumnid ) );   //  Template bit is not persisted
        Call( ErrCATISeekTableObject(
                    ppib,
                    pfucbCatalog,
                    objidTable,
                    sysobjColumn,
                    *pcolumnid ) );
    }
    else
    {
        Call( ErrCATISeekTableObject(
                    ppib,
                    pfucbCatalog,
                    objidTable,
                    sysobjColumn,
                    szColumnName ) );
    }
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Assert( fidMSO_Id.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Id,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(JET_COLUMNID) );
    columnid = *(UnalignedLittleEndian< JET_COLUMNID > *) dataField.Pv();
//  UtilMemCpy( &columnid, dataField.Pv(), sizeof(JET_COLUMNID) );

    Assert( !FCOLUMNIDTemplateColumn( columnid ) ); //  Template bit is not persisted

#ifdef DEBUG
    Assert( fidMSO_Coltyp.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Coltyp,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(JET_COLTYP) );
    Assert( JET_coltypNil != *( UnalignedLittleEndian< JET_COLTYP > *)dataField.Pv() );
#endif

    Assert( fidMSO_Flags.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Flags,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );
    ffield = *(UnalignedLittleEndian< FIELDFLAG > *)dataField.Pv();

    Call( ErrDIRRelease( pfucbCatalog ) );

    Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplace ) );

    //  Set column type to Nil to flag column as deleted.
    //  If possible, the record will be deleted the next time the table
    //  is opened.
    coltyp = JET_coltypNil;
    Call( ErrIsamSetColumn(
            ppib,
            pfucbCatalog,
            fidMSO_Coltyp,
            (BYTE *)&coltyp,
            sizeof(coltyp),
            0,
            NULL ) );

    // Replace column name with bogus name of the form "JetStub_<objidFDP>_<fid>".
    OSStrCbCopyA( szStubName, sizeof(szStubName), szDeletedColumnStubPrefix );
    const size_t cbPrefix = strlen( szDeletedColumnStubPrefix );
    _ultoa_s( objidTable,
              szStubName + cbPrefix,
              CbRemainingInObject( szStubName, szStubName + cbPrefix, sizeof(szStubName) / sizeof(*szStubName) ),
              10 );

    Assert( strlen( szStubName ) < cbMaxDeletedColumnStubName );
    OSStrCbAppendA( szStubName, sizeof(szStubName), "_" );

    const size_t cbStubName = strlen( szStubName );
    _ultoa_s( columnid,
              szStubName + cbStubName,
              CbRemainingInObject( szStubName, szStubName + cbStubName, sizeof(szStubName) / sizeof(*szStubName) ),
              10 );

    Assert( strlen( szStubName ) < cbMaxDeletedColumnStubName );
    Call( ErrIsamSetColumn(
            ppib,
            pfucbCatalog,
            fidMSO_Name,
            (BYTE *)szStubName,
            (ULONG)strlen( szStubName ),
            NO_GRBIT,
            NULL ) );

    // Reset the user defined default bit so that we don't try to resolve the
    // callback on this column in the future
    if ( ffield & ffieldUserDefinedDefault )
    {
        ffield = ffield & ~( ffieldUserDefinedDefault | ffieldDefault );
        Call( ErrIsamSetColumn(
                ppib,
                pfucbCatalog,
                fidMSO_Flags,
                (BYTE *)&ffield,
                sizeof(ULONG),
                0,
                NULL ) );
    }


    Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );

    //  Set return value.
    *pcolumnid = columnid;

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    return err;
}


ERR ErrCATDeleteTableColumn(
    PIB         *ppib,
    const IFMP  ifmp,
    const OBJID objidTable,
    const CHAR  *szColumnName,
    COLUMNID    *pcolumnid )
{
    ERR         err;

    //  ensure we're in a transaction, so that on failure, updates to both
    //  catalog and its shadow are rolled back
    Assert( ppib->Level() > 0 );

    CallR( ErrCATIDeleteTableColumn(
                ppib,
                ifmp,
                objidTable,
                szColumnName,
                pcolumnid,
                fFalse ) );


    if ( !g_rgfmp[ifmp].FShadowingOff() )
    {
#ifdef DEBUG
        const ERR   errSave         = err;
#endif

        COLUMNID    columnidShadow  = *pcolumnid;

        err = ErrCATIDeleteTableColumn(
                    ppib,
                    ifmp,
                    objidTable,
                    szColumnName,
                    &columnidShadow,
                    fTrue );

        Assert( JET_errColumnNotFound != err );     // would have been detected in regular catalog
        Assert( err < 0
            || ( errSave == err && columnidShadow == *pcolumnid ) );
    }

    return err;
}


LOCAL ERR ErrCATIDeleteTableIndex(
    PIB         *ppib,
    const IFMP  ifmp,
    const OBJID objidTable,
    const CHAR  *szIndexName,
    PGNO        *ppgnoIndexFDP,
    OBJID       *pobjidIndex,
    const BOOL  fShadow )
{
    ERR         err;
    FUCB        *pfucbCatalog       = pfucbNil;
    DATA        dataField;

    Assert( objidTable > objidSystemRoot );
    Assert( NULL != ppgnoIndexFDP );
    Assert( NULL != pobjidIndex );

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fShadow ) );
    Assert( pfucbNil != pfucbCatalog );

    if ( fShadow )
    {
        //  if shadow, must search by objidIndex because there's
        //  no index on name
        Assert( *pobjidIndex > objidSystemRoot );

        //  objids are monotonically increasing, so an index should
        //  always have higher objid than its table
        Assert( *pobjidIndex > objidTable );

        Call( ErrCATISeekTableObject(
                    ppib,
                    pfucbCatalog,
                    objidTable,
                    sysobjIndex,
                    *pobjidIndex ) );
    }
    else
    {
        //  if not shadow, must search by name because we dont'
        //  know objidIndex yet
        Call( ErrCATISeekTableObject(
                    ppib,
                    pfucbCatalog,
                    objidTable,
                    sysobjIndex,
                    szIndexName ) );
    }
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Assert( fidMSO_PgnoFDP.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_PgnoFDP,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(PGNO) );
    *ppgnoIndexFDP = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
//  UtilMemCpy( ppgnoIndexFDP, dataField.Pv(), sizeof(PGNO) );

    Assert( fidMSO_Id.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Id,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(OBJID) );
    *pobjidIndex = *(UnalignedLittleEndian< OBJID > *) dataField.Pv();
//  UtilMemCpy( pobjidIndex, dataField.Pv(), sizeof(OBJID) );

#ifdef DEBUG
    LE_IDXFLAG      *ple_idxflag;
    IDBFLAG         fidb;
    Assert( fidMSO_Flags.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Flags,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );
    ple_idxflag = (LE_IDXFLAG*)dataField.Pv();
    fidb = ple_idxflag->fidb;

    if ( FIDBPrimary( fidb ) )
    {
        Assert( objidTable == *pobjidIndex );
    }
    else
    {
        //  objids are monotonically increasing, so an index should
        //  always have higher objid than its table
        Assert( *pobjidIndex > objidTable );
    }
#endif

    CallS( ErrDIRRelease( pfucbCatalog ) );

    if ( objidTable == *pobjidIndex )
        err = ErrERRCheck( JET_errIndexMustStay );
    else
        err = ErrIsamDelete( ppib, pfucbCatalog );

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    return err;
}


LOCAL ERR ErrCATIDeleteDbObject(
    PIB             * const ppib,
    const IFMP      ifmp,
    const CHAR      * const szName,
    const SYSOBJ    sysobj )
{
    ERR             err;
    FUCB *          pfucbCatalog        = pfucbNil;
    BOOKMARK        bm;
    BYTE            *pbBookmark         = NULL;
    ULONG           cbBookmark;

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTableObject(
                ppib,
                pfucbCatalog,
                objidSystemRoot,
                sysobj,
                szName ) );

    Call( ErrDIRRelease( pfucbCatalog ) );

    Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
    Assert( pfucbCatalog->bmCurr.data.FNull() );
    Alloc( pbBookmark = (BYTE *)RESBOOKMARK.PvRESAlloc() );
    cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), cbBookmarkAlloc );
    Assert( cbBookmark <= cbBookmarkMost );
    pfucbCatalog->bmCurr.key.CopyIntoBuffer( pbBookmark, cbBookmark );

    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( pbBookmark );
    bm.key.suffix.SetCb( cbBookmark );
    bm.data.Nullify();

    Call( ErrIsamDelete( ppib, pfucbCatalog ) );
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    if( !g_rgfmp[ifmp].FShadowingOff() )
    {
        Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );
        Assert( pfucbNil != pfucbCatalog );

        Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );
        Call( ErrIsamDelete( ppib, pfucbCatalog ) );
    }

HandleError:
    RESBOOKMARK.Free( pbBookmark );

    if( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }
    return err;
}

ERR ErrCATDeleteDbObject(
    PIB             * const ppib,
    const IFMP      ifmp,
    const CHAR      * const szName,
    const SYSOBJ    sysobj)
{
    ERR err = JET_errSuccess;

    CallR( ErrDIRBeginTransaction( ppib, 63205, NO_GRBIT ) );

    Call( ErrCATIDeleteDbObject(
                ppib,
                ifmp,
                szName,
                sysobj ) );

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
    if( err < 0 )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    return err;
}


ERR ErrCATDeleteTableIndex(
    PIB         *ppib,
    const IFMP  ifmp,
    const OBJID objidTable,
    const CHAR  *szIndexName,
    PGNO        *ppgnoIndexFDP )
{
    ERR         err;
    OBJID       objidIndex;

    //  ensure we're in a transaction, so that on failure, updates to both
    //  catalog and its shadow are rolled back
    Assert( ppib->Level() > 0 );

    CallR( ErrCATIDeleteTableIndex(
                ppib,
                ifmp,
                objidTable,
                szIndexName,
                ppgnoIndexFDP,
                &objidIndex,
                fFalse ) );

    if ( !g_rgfmp[ifmp].FShadowingOff() )
    {
#ifdef DEBUG
        const ERR   errSave     = err;
#endif

        PGNO        pgnoIndexShadow;

        err = ErrCATIDeleteTableIndex(
                    ppib,
                    ifmp,
                    objidTable,
                    szIndexName,
                    &pgnoIndexShadow,
                    &objidIndex,
                    fTrue );

        Assert( JET_errIndexNotFound != err );      // would have been detected in regular catalog
        CallR( err );
        Assert( errSave == err && pgnoIndexShadow == *ppgnoIndexFDP );
    }

    CallR( ErrCATDeleteMSObjidsRecord( ppib, ifmp, objidIndex ) );

    CATResetExtentPageCounts( ppib, ifmp, objidIndex );

    return JET_errSuccess;
}



ERR ErrCATAccessTableColumn(
    PIB             *ppib,
    const IFMP      ifmp,
    const OBJID     objidTable,
    const CHAR      *szColumnName,
    COLUMNID        *pcolumnid,
    const BOOL      fLockColumn )
{
    ERR             err;
    FUCB            *pfucbCatalog   = pfucbNil;
    const BOOL      fSearchByName   = ( szColumnName != NULL );
    DATA            dataField;
    JET_COLTYP      coltyp;

    Assert( NULL != pcolumnid );

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    if ( fSearchByName )
    {
        Call( ErrCATISeekTableObject(
                ppib,
                pfucbCatalog,
                objidTable,
                sysobjColumn,
                szColumnName ) );
    }
    else
    {
        COLUMNID    columnidT   = *pcolumnid;

        Assert( !FCOLUMNIDTemplateColumn( columnidT ) );
        Call( ErrCATISeekTableObject(
                ppib,
                pfucbCatalog,
                objidTable,
                sysobjColumn,
                (OBJID)columnidT ) );
    }
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Assert( fidMSO_Coltyp.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Coltyp,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(JET_COLTYP) );
    coltyp = *(UnalignedLittleEndian< JET_COLTYP > *) dataField.Pv();
//  UtilMemCpy( &coltyp, dataField.Pv(), sizeof(JET_COLTYP) );

    if ( JET_coltypNil == coltyp )
    {
        // Column has been deleted.
        err = ErrERRCheck( JET_errColumnNotFound );
        goto HandleError;
    }

    Assert( !fLockColumn || fSearchByName );        // Locking column only done by name.
    if ( fSearchByName )
    {
        Assert( fidMSO_Id.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Id,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(JET_COLUMNID) );
        *pcolumnid = *(UnalignedLittleEndian< JET_COLUMNID > *) dataField.Pv();
//      UtilMemCpy( pcolumnid, dataField.Pv(), sizeof(JET_COLUMNID) );
        Assert( 0 != *pcolumnid );

        if ( fLockColumn )
        {
            // UNDONE: Lock must be obtained in a transaction.  Further, since
            // CreateIndex is currently the only function to lock a table column,
            // we can assert that we are in a transaction.
            Assert( pfucbCatalog->ppib->Level() > 0 );

            Call( ErrDIRRelease( pfucbCatalog ) );
            Call( ErrDIRGetLock( pfucbCatalog, readLock ) );
        }
    }

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}


ERR ErrCATAccessTableIndex(
    PIB             *ppib,
    const IFMP      ifmp,
    const OBJID     objidTable,
    const OBJID     objidIndex )
{
    ERR             err;
    FUCB            *pfucbCatalog       = pfucbNil;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTableObject(
            ppib,
            pfucbCatalog,
            objidTable,
            sysobjIndex,
            objidIndex ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}


ERR ErrCATAccessTableLV(
    PIB             *ppib,
    const IFMP      ifmp,
    const OBJID     objidTable,
    PGNO            *ppgnoLVFDP,
    OBJID           *pobjidLV )
{
    ERR             err;
    FUCB            *pfucbCatalog   = pfucbNil;
    BOOL            fInTransaction  = fFalse;

    Call( ErrDIRBeginTransaction( ppib, 34324, NO_GRBIT ) );
    fInTransaction = fTrue;

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    err = ErrCATISeekTableObject(
                ppib,
                pfucbCatalog,
                objidTable,
                sysobjLongValue,
                szLVRoot );

    if ( err < 0 )
    {
        if ( JET_errObjectNotFound == err )
        {
            // LV tree has yet to be created.
            err = JET_errSuccess;
            *ppgnoLVFDP = pgnoNull;
            if( NULL != pobjidLV )
            {
                *pobjidLV   = objidNil;
            }
        }
    }
    else
    {
        DATA    dataField;

        Assert( Pcsr( pfucbCatalog )->FLatched() );

        Assert( fidMSO_PgnoFDP.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_PgnoFDP,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(PGNO) );

        *ppgnoLVFDP = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
//      UtilMemCpy( ppgnoLVFDP, dataField.Pv(), sizeof(PGNO) );

        if( NULL != pobjidLV )
        {
            Assert( fidMSO_Id.FFixed() );
            Call( ErrRECIRetrieveFixedColumn(
                        pfcbNil,
                        pfucbCatalog->u.pfcb->Ptdb(),
                        fidMSO_Id,
                        pfucbCatalog->kdfCurr.data,
                        &dataField ) );
            CallS( err );
            Assert( dataField.Cb() == sizeof(OBJID) );

            *pobjidLV = *(UnalignedLittleEndian< OBJID > *) dataField.Pv();
//          UtilMemCpy( pobjidLV, dataField.Pv(), sizeof(OBJID) );
        }
    }

HandleError:
    if ( pfucbCatalog != pfucbNil )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }

    if ( fInTransaction )
    {
        if( err >= 0 )
        {
            err = ErrDIRCommitTransaction( ppib, NO_GRBIT );
        }
        else
        {
            CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
        }
    }

    return err;
}


ERR ErrCATGetTableInfoCursor(
    PIB             *ppib,
    const IFMP      ifmp,
    const CHAR      *szTableName,
    FUCB            **ppfucbInfo )
{
    ERR             err;
    FUCB            *pfucbCatalog;

    Assert( NULL != ppfucbInfo );

    //  Can only open a system table cursor on a specific record.
    if ( NULL == szTableName || '\0' == *szTableName )
    {
        err = ErrERRCheck( JET_errObjectNotFound );
        return err;
    }

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTable( ppib, pfucbCatalog, szTableName ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    CallS( ErrDIRRelease( pfucbCatalog ) );

    *ppfucbInfo = pfucbCatalog;

    return err;

HandleError:
    Assert( err < 0 );
    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}

ERR ErrCATGetObjectNameFromObjid(
    PIB         *ppib,
    const IFMP  ifmp,
    const OBJID     objidTable,
    const SYSOBJ    sysobj,
    const OBJID objid,
    __out_bcount( cbName ) PSTR         szName,
    ULONG       cbName )
{
    ERR         err;
    FUCB        *pfucbCatalog       = pfucbNil;
    DATA        dataField;

    Assert( NULL != szName);
    Assert( cbName >= JET_cbNameMost + 1 );

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call (ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobj, objid) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Assert( fidMSO_Name.FVar() );
    Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Name,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );

    memset( szName, '\0', cbName);

    const INT cbDataField = dataField.Cb();

    memcpy( szName, dataField.Pv(), min( cbName, (ULONG)cbDataField ) );

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    return err;
}


ERR ErrCATGetTableAllocInfo(
    PIB         *ppib,
    const IFMP  ifmp,
    const OBJID objidTable,
    ULONG       *pulPages,
    ULONG       *pulDensity,
    PGNO        *ppgnoFDP)
{
    ERR         err;
    FUCB        *pfucbCatalog       = pfucbNil;
    DATA        dataField;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTable( ppib, pfucbCatalog, objidTable ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    //  pages are optional, density is not
    if ( NULL != pulPages )
    {
        Assert( fidMSO_Pages.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Pages,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(ULONG) );
        *pulPages = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
//      UtilMemCpy( pulPages, dataField.Pv(), sizeof(ULONG) );
    }

    Assert( NULL != pulDensity );

    Assert( fidMSO_SpaceUsage.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_SpaceUsage,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );
    *pulDensity = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
//  UtilMemCpy( pulDensity, dataField.Pv(), sizeof(ULONG) );

    if ( NULL != ppgnoFDP )
    {
        Assert( fidMSO_PgnoFDP.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_PgnoFDP,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(PGNO) );
        *ppgnoFDP = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
//      UtilMemCpy( ppgnoFDP, dataField.Pv(), sizeof(PGNO) );
        Expected( fFalse ); // not called from anywhere w/ ppgnoFDP passed in - so this code has not recently been run or tested - double check your result.
    }

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    return err;
}

ERR ErrCATGetIndexAllocInfo(
    PIB         *ppib,
    const IFMP  ifmp,
    const OBJID objidTable,
    const CHAR  *szIndexName,
    ULONG       *pulDensity )
{
    ERR         err;
    FUCB        *pfucbCatalog       = pfucbNil;
    DATA        dataField;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTableObject(
                ppib,
                pfucbCatalog,
                objidTable,
                sysobjIndex,
                szIndexName ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Assert( NULL != pulDensity );

    Assert( fidMSO_SpaceUsage.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_SpaceUsage,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );
    *pulDensity = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
//  UtilMemCpy( pulDensity, dataField.Pv(), sizeof(ULONG) );

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    return err;
}

ERR ErrCATGetIndexLcid(
    PIB         *ppib,
    const IFMP  ifmp,
    const OBJID objidTable,
    const CHAR  *szIndexName,
    LCID        *plcid )
{
    ERR         err;
    FUCB        *pfucbCatalog       = pfucbNil;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTableObject(
                ppib,
                pfucbCatalog,
                objidTable,
                sysobjIndex,
                szIndexName ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Assert( NULL != plcid );

    Call( ErrCATIRetrieveLocaleInformation(
        pfucbCatalog,
        NULL,
        0,
        plcid,
        NULL,
        NULL,
        NULL ) );

HandleError:

    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}

ERR ErrCATGetIndexLocaleName(
    _In_ PIB            *ppib,
    _In_ const IFMP     ifmp,
    _In_ const OBJID    objidTable,
    _In_ const CHAR     *szIndexName,
    _Out_writes_bytes_( cbLocaleName ) PWSTR    wszLocaleName,
    _In_ ULONG          cbLocaleName )
{
    ERR         err;
    FUCB        *pfucbCatalog       = pfucbNil;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTableObject(
                ppib,
                pfucbCatalog,
                objidTable,
                sysobjIndex,
                szIndexName ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Assert( NULL != wszLocaleName );

    const ULONG cchLocaleName = cbLocaleName / sizeof(WCHAR);
    Call( ErrCATIRetrieveLocaleInformation(
        pfucbCatalog,
        wszLocaleName,
        cchLocaleName,
        NULL,
        NULL,
        NULL,
        NULL ) );

HandleError:

    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}

// Retrieves the Sort Version for the specified index name.
ERR ErrCATGetIndexSortVersion(
    _In_ PIB            *ppib,
    _In_ const IFMP     ifmp,
    _In_ const OBJID    objidTable,
    _In_ const CHAR     *szIndexName,
    _Out_ DWORD         *pdwOutputSortVersion )
{
    ERR         err;
    FUCB        *pfucbCatalog       = pfucbNil;

    *pdwOutputSortVersion = 0;

    QWORD qwVersion;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTableObject(
                ppib,
                pfucbCatalog,
                objidTable,
                sysobjIndex,
                szIndexName ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Call( ErrCATIRetrieveLocaleInformation(
        pfucbCatalog,
        NULL,
        0,
        NULL,
        NULL,
        &qwVersion,
        NULL ) );

    *pdwOutputSortVersion = DwNLSVersionFromSortVersion( qwVersion );

HandleError:

    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}

// Retrieves the Sort Version for the specified index name.
ERR ErrCATGetIndexDefinedSortVersion(
    _In_ PIB            *ppib,
    _In_ const IFMP     ifmp,
    _In_ const OBJID    objidTable,
    _In_ const CHAR     *szIndexName,
    _Out_ DWORD         *pdwOutputDefinedSortVersion )
{
    ERR         err;
    FUCB        *pfucbCatalog       = pfucbNil;

    *pdwOutputDefinedSortVersion = 0;

    QWORD qwVersion;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTableObject(
                ppib,
                pfucbCatalog,
                objidTable,
                sysobjIndex,
                szIndexName ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Call( ErrCATIRetrieveLocaleInformation(
        pfucbCatalog,
        NULL,
        0,
        NULL,
        NULL,
        &qwVersion,
        NULL ) );

    *pdwOutputDefinedSortVersion = DwDefinedVersionFromSortVersion( qwVersion );

HandleError:

    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}

// Retrieves the sort GUID for a specific index.
ERR ErrCATGetIndexSortid(

    _In_ PIB            *ppib,
    _In_ const IFMP     ifmp,
    _In_ const OBJID    objidTable,
    _In_ const CHAR     *szIndexName,
    _Out_ SORTID        *psortid )
{
    ERR         err;
    FUCB        *pfucbCatalog       = pfucbNil;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTableObject(
                ppib,
                pfucbCatalog,
                objidTable,
                sysobjIndex,
                szIndexName ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Call( ErrCATIRetrieveLocaleInformation(
        pfucbCatalog,
        NULL,
        0,
        NULL,
        psortid,
        NULL,
        NULL ) );

HandleError:

    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}


ERR ErrCATGetIndexVarSegMac(
    PIB         *ppib,
    const IFMP  ifmp,
    const OBJID objidTable,
    const CHAR  *szIndexName,
    USHORT      *pusVarSegMac )
{
    ERR         err;
    FUCB        *pfucbCatalog   = pfucbNil;
    DATA        dataField;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTableObject(
                ppib,
                pfucbCatalog,
                objidTable,
                sysobjIndex,
                szIndexName ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Assert( NULL != pusVarSegMac );

    Assert( fidMSO_VarSegMac.FVar() );
    Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_VarSegMac,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    Assert( JET_errSuccess == err || JET_wrnColumnNull == err );

    if ( JET_wrnColumnNull == err )
    {
        Assert( dataField.Cb() == 0 );

        Assert( fidMSO_KeyMost.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_KeyMost,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        Assert( JET_errSuccess == err || JET_wrnColumnNull == err );

        if ( JET_wrnColumnNull == err )
        {
            Assert( dataField.Cb() == 0 );

            *pusVarSegMac = JET_cbKeyMost_OLD;
        }
        else
        {
            Assert( dataField.Cb() == sizeof(USHORT) );
            *pusVarSegMac = *(UnalignedLittleEndian< USHORT > *) dataField.Pv();
//          UtilMemCpy( pusVarSegMac, dataField.Pv(), sizeof(USHORT) );
            Assert( *pusVarSegMac > 0 );
            Assert( *pusVarSegMac <= cbKeyMostMost );
        }
    }
    else
    {
        Assert( dataField.Cb() == sizeof(USHORT) );
        *pusVarSegMac = *(UnalignedLittleEndian< USHORT > *) dataField.Pv();
//      UtilMemCpy( pusVarSegMac, dataField.Pv(), sizeof(USHORT) );
        Assert( *pusVarSegMac > 0 );
        Assert( *pusVarSegMac <= cbKeyMostMost );
    }

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    return err;
}


ERR ErrCATGetIndexKeyMost(
    PIB         *ppib,
    const IFMP  ifmp,
    const OBJID objidTable,
    const CHAR  *szIndexName,
    USHORT      *pusKeyMost )
{
    ERR         err;
    FUCB        *pfucbCatalog   = pfucbNil;
    DATA        dataField;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTableObject(
                ppib,
                pfucbCatalog,
                objidTable,
                sysobjIndex,
                szIndexName ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Assert( NULL != pusKeyMost );

    Assert( fidMSO_KeyMost.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_KeyMost,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    Assert( JET_errSuccess == err || JET_wrnColumnNull == err );

    if ( JET_wrnColumnNull == err )
    {
        Assert( dataField.Cb() == 0 );

        *pusKeyMost = JET_cbKeyMost_OLD;
    }
    else
    {
        Assert( dataField.Cb() == sizeof(SHORT) );
        *pusKeyMost = *(UnalignedLittleEndian< USHORT > *) dataField.Pv();
//      UtilMemCpy( pusVarSegMac, dataField.Pv(), sizeof(SHORT) );
        Assert( *pusKeyMost > 0 );
        Assert( *pusKeyMost <= cbKeyMostMost );
    }

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    return err;
}



ERR ErrCATGetIndexSegments(
    PIB             *ppib,
    const IFMP      ifmp,
    const OBJID     objidTable,
    const IDXSEG*   rgidxseg,
    const ULONG     cidxseg,
    const BOOL      fConditional,
    const BOOL      fOldFormat,
    CHAR            rgszSegments[JET_ccolKeyMost][JET_cbNameMost+1+1] )     // extra +1 for ascending/descending byte
{
    ERR             err;
    FUCB            *pfucbCatalog           = pfucbNil;
    DATA            dataField;
    ULONG           ulFlags;
    OBJID           objidTemplateTable      = objidNil;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATISeekTable( ppib, pfucbCatalog, objidTable ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    //  should be on primary index
    Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );

    Assert( fidMSO_Flags.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Flags,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );
    ulFlags = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();

    if ( ulFlags & JET_bitObjectTableDerived )
    {
        CHAR    szTemplateTable[JET_cbNameMost+1];

        Assert( fidMSO_TemplateTable.FVar() );
        Call( ErrRECIRetrieveVarColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_TemplateTable,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );

        const INT cbDataField = dataField.Cb();

        if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
        {
            AssertSz( fFalse, "Invalid fidMSO_TemplateTable column in catalog." );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"1757912b-ed15-447a-b6fb-94173a3b87a9" );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }

        Assert( sizeof( szTemplateTable ) > cbDataField );
        UtilMemCpy( szTemplateTable, dataField.Pv(), cbDataField );
        szTemplateTable[cbDataField] = '\0';

        Assert( Pcsr( pfucbCatalog )->FLatched() );
        CallS( ErrDIRRelease( pfucbCatalog ) );

        Call( ErrCATSeekTable( ppib, ifmp, szTemplateTable, NULL, &objidTemplateTable ) );
        Assert( objidNil != objidTemplateTable );
    }
    else
    {
        Assert( Pcsr( pfucbCatalog )->FLatched() );
        CallS( ErrDIRRelease( pfucbCatalog ) );
    }

    //  should still be on the primary index, which is the Id index
    Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    UINT    i;
    for ( i = 0; i < cidxseg; i++ )
    {
        CHAR                szColumnName[JET_cbNameMost+1];
        OBJID               objidT          = objidTable;
        BYTE                bPrefix;

        if ( rgidxseg[i].FTemplateColumn() )
        {
            Assert( !fOldFormat );

            if ( ulFlags & JET_bitObjectTableDerived )
            {
                Assert( objidNil != objidTemplateTable );
                objidT = objidTemplateTable;
            }
            else
            {
                Assert( ulFlags & JET_bitObjectTableTemplate );
                Assert( objidNil == objidTemplateTable );
            }
        }

        err = ErrCATISeekTableObject(
                    ppib,
                    pfucbCatalog,
                    objidT,
                    sysobjColumn,
                    ColumnidOfFid( rgidxseg[i].Fid(), fFalse ) );   //  Template bit never persisted in catalog ) );
        if ( err < 0 )
        {
            //  SPECIAL-CASE: We never stored template bit in old-format indexes, so it's
            //  possible that the column was actually derived from the template table
            if ( JET_errColumnNotFound == err
                && fOldFormat
                && objidNil != objidTemplateTable )
            {
                err = ErrCATISeekTableObject(
                            ppib,
                            pfucbCatalog,
                            objidTemplateTable,
                            sysobjColumn,
                            ColumnidOfFid( rgidxseg[i].Fid(), fFalse ) );
            }

            Call( err );
        }
        Assert( Pcsr( pfucbCatalog )->FLatched() );

        //  should be on primary index
        Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );

        Assert( fidMSO_Name.FVar() );
        Call( ErrRECIRetrieveVarColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Name,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );

        const INT cbDataField = dataField.Cb();

        if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
        {
            AssertSz( fFalse, "Invalid fidMSO_Name column in catalog." );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"a075ae77-b1fc-42bb-b416-3b1ea8a7fa13" );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }

        Assert( sizeof( szColumnName ) > cbDataField );
        UtilMemCpy( szColumnName, dataField.Pv(), cbDataField );
        szColumnName[cbDataField] = '\0';

        Assert( Pcsr( pfucbCatalog )->FLatched() );
        CallS( ErrDIRRelease( pfucbCatalog ) );

        if ( fConditional )
        {
            bPrefix = ( rgidxseg[i].FMustBeNull() ? '?' : '*' );
        }
        else
        {
            bPrefix = ( rgidxseg[i].FDescending() ? '-' : '+' );

        }
        C_ASSERT(JET_cbNameMost+1+1 == sizeof(rgszSegments[0]));
        OSStrCbFormatA(
            rgszSegments[i],
            JET_cbNameMost+1+1,
            "%c%-s",
            bPrefix,
            szColumnName );
    }

    err = JET_errSuccess;

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}


//  ================================================================
ERR ErrCATGetColumnCallbackInfo(
    PIB * const ppib,
    const IFMP ifmp,
    const OBJID objidTable,
    const OBJID objidTemplateTable,
    const COLUMNID columnid,
    __out_bcount_part(cchCallbackMax, *pcchCallback) PSTR const szCallback,
    const ULONG cchCallbackMax,
    ULONG * const pcchCallback,
    __out_bcount_part(cbUserDataMax, *pcbUserData) BYTE * const pbUserData,
    const ULONG cbUserDataMax,
    ULONG * const pcbUserData,
    __out_bcount_part(cchDependantColumnsMax, *pchDependantColumns) PSTR const szDependantColumns,
    const ULONG cchDependantColumnsMax,
    ULONG * const pchDependantColumns )
//  ================================================================
{
    ERR             err;
    FUCB            *pfucbCatalog           = pfucbNil;
    DATA            dataField;

    FID             rgfidDependencies[JET_ccolKeyMost];
    ULONG           cbActual;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    //  Template bit is not persisted
    Assert( !FCOLUMNIDTemplateColumn( columnid ) );

    Call( ErrCATISeekTableObject(
            ppib,
            pfucbCatalog,
            objidTable,
            sysobjColumn,
            columnid ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Assert( fidMSO_Callback.FVar() );
    Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Callback,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );

    INT cbDataField = dataField.Cb();

    Assert( cchCallbackMax > JET_cbNameMost );
    if ( cchCallbackMax < JET_cbNameMost + 1 )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  ErrCATGetColumnCallbackInfo should only be called for columns that have callbacks
    if ( cbDataField < 0 || (ULONG)cbDataField >= cchCallbackMax || cbDataField > JET_cbNameMost )
    {
        AssertSz( fFalse, "Invalid fidMSO_Callback column in catalog." );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"33a883ac-04a1-45ab-9300-2297a453e7da" );
        Error( ErrERRCheck( JET_errCatalogCorrupted ) );
    }

    UtilMemCpy( szCallback, dataField.Pv(), cbDataField );
    *pcchCallback = cbDataField + 1;
    szCallback[cbDataField] = '\0';

    Assert( fidMSO_CallbackData.FTagged() );
    Call( ErrCATIRetrieveTaggedColumn(
            pfucbCatalog,
            fidMSO_CallbackData,
            1,
            pfucbCatalog->kdfCurr.data,
            pbUserData,
            cbUserDataMax,
            pcbUserData ) );
    Assert( JET_errSuccess == err || JET_wrnColumnNull == err );

    Assert( fidMSO_CallbackDependencies.FTagged() );
    Call( ErrCATIRetrieveTaggedColumn(
            pfucbCatalog,
            fidMSO_CallbackDependencies,
            1,
            pfucbCatalog->kdfCurr.data,
            (BYTE *)rgfidDependencies,
            sizeof(rgfidDependencies),
            &cbActual ) );
    Assert( JET_errSuccess == err || JET_wrnColumnNull == err );
    Assert( ( cbActual % sizeof(FID) ) == 0 );

    *pchDependantColumns = 0;

    if( cbActual > 0 )
    {
        //  dependencies not currently supported
        //
        Assert( fFalse );

        //  loop through the FIDs in the dependency list and convert them into column names
        //  Put the column names into szDependantColumns in "column\0column\0column\0\0" form.
        //
        UINT iFid;
        for ( iFid = 0; iFid < ( cbActual / sizeof( FID ) ); ++iFid )
        {
            CHAR            szColumnName[JET_cbNameMost+1];
            const COLUMNID  columnidT = rgfidDependencies[iFid];
            Assert( fidtypUnknown != FidOfColumnid( columnidT ).Fidtyp() );
            Assert( Pcsr( pfucbCatalog )->FLatched() );

            CallS( ErrDIRRelease( pfucbCatalog ) );
            err = ErrCATISeekTableObject(
                        ppib,
                        pfucbCatalog,
                        objidTable,
                        sysobjColumn,
                        columnidT );
            if ( JET_errColumnNotFound == err
                && objidNil != objidTemplateTable )
            {
                err = ErrCATISeekTableObject(
                            ppib,
                            pfucbCatalog,
                            objidTemplateTable,
                            sysobjColumn,
                            columnidT );
            }
            Call( err );

            Assert( Pcsr( pfucbCatalog )->FLatched() );

            //  should be on primary index
            Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );

            Assert( fidMSO_Name.FVar() );
            Call( ErrRECIRetrieveVarColumn(
                        pfcbNil,
                        pfucbCatalog->u.pfcb->Ptdb(),
                        fidMSO_Name,
                        pfucbCatalog->kdfCurr.data,
                        &dataField ) );
            CallS( err );

            cbDataField = dataField.Cb();

            if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
            {
                AssertSz( fFalse, "Invalid fidMSO_Name column in catalog." );
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"ff66671e-0cbb-45df-aae2-106e1a5d916b" );
                Error( ErrERRCheck( JET_errCatalogCorrupted ) );
            }
            Assert( sizeof( szColumnName ) > cbDataField );
            UtilMemCpy( szColumnName, dataField.Pv(), cbDataField );
            szColumnName[cbDataField] = '\0';

            //  remember to leave space for the terminating NULL
            //
            if( *pchDependantColumns + dataField.Cb() + 2 >= cchDependantColumnsMax )
            {
                //  buffer should always have enough space
                //
                Assert( fFalse );
                err = ErrERRCheck( JET_wrnBufferTruncated );
                break;
            }
            OSStrCbCopyA( szDependantColumns + *pchDependantColumns,
                cchDependantColumnsMax - *pchDependantColumns,
                szColumnName );
            *pchDependantColumns += dataField.Cb() + 1;
        }

        if( *pchDependantColumns >= cchDependantColumnsMax )
        {
            //  buffer should always have enough space
            //
            Assert( fFalse );
            err = ErrERRCheck( JET_wrnBufferTruncated );
        }
        else
        {
            szDependantColumns[*pchDependantColumns] = '\0';
            ++(*pchDependantColumns);
        }
    }

HandleError:
    Assert( pfucbNil != pfucbCatalog );
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    return err;
}


/*  construct the catalog TDB using the static data structures
/*  defined in _cat.c.
/**/
INLINE ERR ErrCATIInitCatalogTDB( INST *pinst, IFMP ifmp, TDB **pptdbNew )
{
    ERR                 err;
    UINT                i;
    FIELD               field;
    const CDESC         *pcdesc;
    TDB                 *ptdb                       = ptdbNil;
    TCIB                tcib;
    REC::RECOFFSET      ibRec                       = ibRECStartFixedColumns;
    COLUMNID            columnidT;


    //  UNDONE:     international support.  Set these values from create database connect string.
    //
    field.cp = usEnglishCodePage;

    /*  first, determine how many columns there are
    /**/
    pcdesc  = rgcdescMSO;
    for ( i = 0; i < cColumnsMSO; i++, pcdesc++ )
    {
        CallS( ErrFILEGetNextColumnid( pcdesc->coltyp, pcdesc->grbit, fFalse, &tcib, &columnidT ) );

        //  verify generated columnid matches hard-coded columnid
        //
        Assert( pcdesc->columnid == columnidT );
    }

    CallR( ErrTDBCreate( pinst, ifmp, &ptdb, &tcib, fTrue, pfcbNil, fTrue ) );

    /*  check initialisations
    /**/
    Assert( ptdb->FidVersion() == 0 );
    Assert( ptdb->FidAutoincrement() == 0 );
    Assert( NULL == ptdb->PdataDefaultRecord() );
    Assert( tcib.fidFixedLast == ptdb->FidFixedLast() );
    Assert( tcib.fidVarLast == ptdb->FidVarLast() );
    Assert( tcib.fidTaggedLast == ptdb->FidTaggedLast() );
    Assert( ptdb->DbkMost() == 0 );
    Assert( ptdb->Ui64LidLast() == 0 );

    //  add table name
    //
    Assert( ptdb->ItagTableName() == 0 );
    MEMPOOL::ITAG   itagNew;
    Call( ptdb->MemPool().ErrAddEntry( (BYTE *)szMSO, (ULONG)strlen(szMSO) + 1, &itagNew ) );
    Assert( itagNew == itagTDBTableName );
    ptdb->SetItagTableName( itagNew );

    /*  fill in the column info
    /**/
    pcdesc = rgcdescMSO;
    for ( i = 0; i < cColumnsMSO; i++, pcdesc++ )
    {
        Call( ptdb->MemPool().ErrAddEntry(
                (BYTE *)pcdesc->szColName,
                (ULONG)strlen( pcdesc->szColName ) + 1,
                &field.itagFieldName ) );
        field.strhashFieldName = StrHashValue( pcdesc->szColName );
        field.coltyp = FIELD_COLTYP( pcdesc->coltyp );
        Assert( field.coltyp != JET_coltypNil );
        field.cbMaxLen = UlCATColumnSize( pcdesc->coltyp, 0, NULL );

        /*  flag for system table columns is JET_bitColumnNotNULL
        /**/
        field.ffield = 0;

        Assert( 0 == pcdesc->grbit || JET_bitColumnNotNULL == pcdesc->grbit || JET_bitColumnTagged == pcdesc->grbit );
        if ( pcdesc->grbit == JET_bitColumnNotNULL )
            FIELDSetNotNull( field.ffield );

        // ibRecordOffset is only relevant for fixed fields (it will be ignored by
        // RECAddFieldDef(), so don't even bother setting it).
        if ( FCOLUMNIDFixed( pcdesc->columnid ) )
        {
            field.ibRecordOffset = ibRec;
            ibRec = REC::RECOFFSET( ibRec + field.cbMaxLen );
        }
        else
        {
            field.ibRecordOffset = 0;
        }

        Call( ErrRECAddFieldDef(
                ptdb,
                ColumnidOfFid( FID( (WORD)(pcdesc->columnid) ), fFalse ),
                &field ) );
    }
    Assert( ibRec <= REC::CbRecordMost( g_rgfmp[ ifmp ].CbPage(), JET_cbKeyMost_OLD ) );
    ptdb->SetIbEndFixedColumns( ibRec, ptdb->FidFixedLast() );

    *pptdbNew = ptdb;

    return JET_errSuccess;

HandleError:
    delete ptdb;
    return err;
}


LOCAL VOID CATIFreeSecondaryIndexes( FCB *pfcbSecondaryIndexes )
{
    FCB     * pfcbT     = pfcbSecondaryIndexes;
    FCB     * pfcbKill;
    INST    * pinst;

    Assert( pfcbNil != pfcbSecondaryIndexes );
    pinst = PinstFromIfmp( pfcbSecondaryIndexes->Ifmp() );

    // Must clean up secondary index FCBs and primary index FCB separately,
    // because the secondary index FCBs may not be linked to the primary index FCB.
    while ( pfcbT != pfcbNil )
    {
        if ( pfcbT->Pidb() != pidbNil )
        {
            // No need to explicitly free index name or idxseg array, since
            // memory pool will be freed when TDB is deleted.
            pfcbT->ReleasePidb();
        }
        pfcbKill = pfcbT;
        pfcbT = pfcbT->PfcbNextIndex();

        //  synchronously purge the FCB

        pfcbKill->PrepareForPurge( fFalse );
        pfcbKill->Purge();
    }
}


/*  get index info of a system table index
/**/
ERR ErrCATInitCatalogFCB( FUCB *pfucbTable )
{
    ERR         err;
    PIB         *ppib                   = pfucbTable->ppib;
    const IFMP  ifmp                    = pfucbTable->ifmp;
    FCB         *pfcb                   = pfucbTable->u.pfcb;
    TDB         *ptdb                   = ptdbNil;
    IDB         idb( PinstFromIfmp( ifmp ) );
    UINT        iIndex;
    FCB         *pfcbSecondaryIndexes   = pfcbNil;
    const PGNO  pgnoTableFDP            = pfcb->PgnoFDP();
    const BOOL  fShadow                 = ( pgnoFDPMSOShadow == pgnoTableFDP );
    BOOL        fProcessedPrimary       = fFalse;

    INST        *pinst = PinstFromIfmp( ifmp );

    Assert( !pfcb->FInitialized() || pfcb->FInitedForRecovery() );
    Assert( pfcb->Ptdb() == ptdbNil );

    Assert( FCATSystemTable( pgnoTableFDP ) );

    CallR( ErrCATIInitCatalogTDB( pinst, ifmp, &ptdb ) );

    idb.SetWszLocaleName( wszLocaleNameDefault );
    idb.SetDwLCMapFlags( dwLCMapFlagsDefault );
    idb.SetCidxsegConditional( 0 );
    idb.SetCbKeyMost( JET_cbKeyMost_OLD );
    idb.SetCbVarSegMac( JET_cbKeyMost_OLD );

    Assert( pfcbSecondaryIndexes == pfcbNil );
    const IDESC         *pidesc     = rgidescMSO;
    for( iIndex = 0; iIndex < cIndexesMSO; iIndex++, pidesc++ )
    {
        USHORT      itagIndexName;

        // Add index name to table's byte pool.  On error, the entire TDB
        // and its byte pool is cleaned up, so we don't have to worry about
        // freeing the memory for the index name.
        Call( ptdb->MemPool().ErrAddEntry(
                    (BYTE *)pidesc->szIdxName,
                    (ULONG)strlen( pidesc->szIdxName ) + 1,
                    &itagIndexName ) );
        idb.SetItagIndexName( itagIndexName );
        idb.SetCidxseg( CfieldCATKeyString( pidesc->szIdxKeys, idb.rgidxseg ) );
        idb.SetFlagsFromGrbit( pidesc->grbit );

        //  catalog indexes are always unique -- if this changes,
        //  ErrCATICreateCatalogIndexes() will have to be modified
        Assert( idb.FUnique() );

        //  catalog indexes are not over Unicode columns
        Assert( !idb.FLocaleSet() );

        const BOOL  fPrimary = idb.FPrimary();
        if ( fPrimary )
        {
            Assert( !fProcessedPrimary );

            Assert( JET_cbKeyMost_OLD == idb.CbVarSegMac() );
            Assert( JET_cbKeyMost_OLD == idb.CbKeyMost() );

            //  initialize FCB for this index
            //
            Call( ErrFILEIInitializeFCB(
                    ppib,
                    ifmp,
                    ptdb,
                    pfcb,
                    &idb,
                    fTrue,
                    pgnoTableFDP,
                    PSystemSpaceHints(eJSPHDefaultUserTable),
                    NULL ) );

            pfcb->Lock();
            pfcb->SetInitialIndex();
            pfcb->Unlock();
            fProcessedPrimary = fTrue;
        }
        else if ( !fShadow )
        {
            FUCB    *pfucbSecondaryIndex;
            PGNO    pgnoIndexFDP;
            OBJID   objidIndexFDP;

            idb.SetCbKeyMost( JET_cbKeyMost_OLD );
            idb.SetCbVarSegMac( JET_cbKeyMost_OLD );

            // UNDONE:  This is a real hack to get around the problem of
            // determining pgnoFDPs of secondary system table indexes.
            // These values are currently hard-coded.
            Assert( pidesc == rgidescMSO + iIndex );
            if ( 1 == iIndex )
            {
                Assert( strcmp( pidesc->szIdxName, szMSONameIndex ) == 0 );
                pgnoIndexFDP = pgnoFDPMSO_NameIndex;
                objidIndexFDP = objidFDPMSO_NameIndex;
            }
            else
            {
                Assert( 2 == iIndex );
                Assert( strcmp( pidesc->szIdxName, szMSORootObjectsIndex ) == 0 );
                pgnoIndexFDP = pgnoFDPMSO_RootObjectIndex;
                objidIndexFDP = objidFDPMSO_RootObjectIndex;
            }

            Assert( idb.FUnique() );    //  all catalog indexes are unique  - note redundant w/ above assert.
            Call( ErrDIROpenNoTouch(
                        ppib,
                        ifmp,
                        pgnoIndexFDP,
                        objidIndexFDP,
                        fTrue,                  //  all catalog indexes are unique
                        &pfucbSecondaryIndex,
                        fTrue ) );              //  will initialize FCB

            Assert( !pfucbSecondaryIndex->u.pfcb->FInitialized() || pfucbSecondaryIndex->u.pfcb->FInitedForRecovery() );

            err = ErrFILEIInitializeFCB(
                    ppib,
                    ifmp,
                    ptdb,
                    pfucbSecondaryIndex->u.pfcb,
                    &idb,
                    fFalse,
                    pgnoIndexFDP,
                    PSystemSpaceHints(eJSPHDefaultUserTable),
                    NULL );
            if ( err < 0 )
            {
                DIRClose( pfucbSecondaryIndex );
                goto HandleError;
            }

            pfucbSecondaryIndex->u.pfcb->SetPfcbNextIndex( pfcbSecondaryIndexes );
            pfcbSecondaryIndexes = pfucbSecondaryIndex->u.pfcb;

            Assert( !pfucbSecondaryIndex->u.pfcb->FInList() );

            //  mark the secondary index as being initialized successfully

            pfucbSecondaryIndex->u.pfcb->Lock();
            pfucbSecondaryIndex->u.pfcb->CreateComplete();
            pfucbSecondaryIndex->u.pfcb->ResetInitedForRecovery();
            pfucbSecondaryIndex->u.pfcb->Unlock();

            DIRClose( pfucbSecondaryIndex );
        }
    }

    // All system tables have a primary index.
    Assert( fProcessedPrimary );

    // Try to compact byte pool, but if it fails, don't worry.  It just means
    // that the byte pool will have some unused space.
    ptdb->MemPool().FCompact();

    /*  link up sequential/primary index with the rest of the indexes
    /**/
    pfcb->SetPfcbNextIndex( pfcbSecondaryIndexes );

    /*  link up pfcbTable of secondary indexes
    /**/
    pfcb->LinkPrimaryIndex();

    FILESetAllIndexMask( pfcb );
    pfcb->Lock();
    pfcb->SetFixedDDL();
    pfcb->Unlock();
    pfcb->SetTypeTable();

    return JET_errSuccess;

    /*  error handling
    /**/
HandleError:
    // Must clean up secondary index FCBs and primary index FCB separately,
    // because the secondary index FCBs never got linked to the primary
    // index FCB.
    if ( pfcbNil != pfcbSecondaryIndexes )
        CATIFreeSecondaryIndexes( pfcbSecondaryIndexes );

    // No need to explicitly free index name or idxseg array, since
    // memory pool will be freed when TDB is deleted below.
    if ( pfcb->Pidb() != pidbNil )
    {
        pfcb->ReleasePidb();
        pfcb->SetPidb( pidbNil );
    }

    Assert( pfcb->Ptdb() == ptdbNil || pfcb->Ptdb() == ptdb );
    delete ptdb;
    pfcb->SetPtdb( ptdbNil );

    return err;
}

LOCAL ERR ErrCATIFindHighestColumnid(
    PIB                 *ppib,
    FUCB                *pfucbCatalog,
    const OBJID         objidTable,
    const JET_COLUMNID  columnidLeast,
    JET_COLUMNID        *pcolumnidMost )
{
    ERR                 err;
    SYSOBJ              sysobj      = sysobjColumn;
    DATA                dataField;
    BOOL                fLatched    = fFalse;

    Assert( *pcolumnidMost == fidFixedMost
        || *pcolumnidMost == fidVarMost
        || *pcolumnidMost == fidTaggedMost );

    //  should be on primary index
    Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&objidTable,
                sizeof(objidTable),
                JET_bitNewKey ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&sysobj,
                sizeof(sysobj),
                NO_GRBIT ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)pcolumnidMost,
                sizeof(*pcolumnidMost),
                NO_GRBIT ) );

    //  should never return RecordNotFound, because in the worst
    //  case (ie. no columns), we should seek back to the Table record.
    err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekLE );
    Assert( JET_errRecordNotFound != err );
    Call( err );

    Assert( JET_errSuccess == err || JET_wrnSeekNotEqual == err );
    if ( JET_wrnSeekNotEqual == err )
    {
        Assert( !Pcsr( pfucbCatalog )->FLatched() );
        Call( ErrDIRGet( pfucbCatalog ) );
        fLatched = fTrue;

        Assert( fidMSO_Type.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Type,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(SYSOBJ) );

        if ( sysobjColumn == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) )
        {
            Assert( fidMSO_Id.FFixed() );
            Call( ErrRECIRetrieveFixedColumn(
                        pfcbNil,
                        pfucbCatalog->u.pfcb->Ptdb(),
                        fidMSO_Id,
                        pfucbCatalog->kdfCurr.data,
                        &dataField ) );
            CallS( err );
            Assert( dataField.Cb() == sizeof(JET_COLUMNID) );

            Assert( *( (UnalignedLittleEndian< JET_COLUMNID > *)dataField.Pv() ) < *pcolumnidMost );
            Assert( *( (UnalignedLittleEndian< JET_COLUMNID > *)dataField.Pv() ) > 0 );

            JET_COLUMNID colidCur = *( (UnalignedLittleEndian< JET_COLUMNID >  *)dataField.Pv() );
            JET_COLUMNID colidLeast = columnidLeast - 1;
            //  might have found a column of a different type
            *pcolumnidMost = max( colidCur, colidLeast );
        }
        else
        {
            //  if no columns, must have seeked to table record
            Assert( sysobjTable == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );
            *pcolumnidMost = columnidLeast - 1;
        }
    }

#ifdef DEBUG
    else
    {
        Assert( !Pcsr( pfucbCatalog )->FLatched() );
        Call( ErrDIRGet( pfucbCatalog ) );
        fLatched = fTrue;

        Assert( fidMSO_Id.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Id,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(JET_COLUMNID) );

        //  the highest possible FID actually exists in this table
        Assert( *( (UnalignedLittleEndian< JET_COLUMNID > *)dataField.Pv() ) == *pcolumnidMost );
    }
#endif

HandleError:
    if ( fLatched )
    {
        Assert( Pcsr( pfucbCatalog )->FLatched() );
        CallS( ErrDIRRelease( pfucbCatalog ) );
    }

    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    return err;
}

LOCAL ERR ErrCATIFindAllHighestColumnids(
    PIB             *ppib,
    FUCB            *pfucbCatalog,
    const OBJID     objidTable,
    TCIB            *ptcib )
{
    ERR             err;
    FUCB            * pfucbCatalogDupe  = pfucbNil;
    COLUMNID        columnidMost;

    //  use dupe cursor to perform columnid seeks, so we won't
    //  lose our place
    CallR( ErrIsamDupCursor( ppib, pfucbCatalog, &pfucbCatalogDupe, NO_GRBIT ) );
    Assert( pfucbNil != pfucbCatalogDupe );

    //  this is an internal cursor - no dispatch needed
    Assert( pfucbCatalogDupe->pvtfndef == &vtfndefIsam );
    pfucbCatalogDupe->pvtfndef = &vtfndefInvalidTableid;

    columnidMost = fidFixedMost;
    Call( ErrCATIFindHighestColumnid(
                ppib,
                pfucbCatalogDupe,
                objidTable,
                fidFixedLeast,
                &columnidMost ) );
    ptcib->fidFixedLast = FidOfColumnid( columnidMost );
    Assert( ptcib->fidFixedLast == fidFixedLeast-1
        || ptcib->fidFixedLast.FFixed() );

    columnidMost = fidVarMost;
    Call( ErrCATIFindHighestColumnid(
                ppib,
                pfucbCatalogDupe,
                objidTable,
                fidVarLeast,
                &columnidMost ) );
    ptcib->fidVarLast = FidOfColumnid( columnidMost );
    Assert( ptcib->fidVarLast == fidVarLeast-1
        || ptcib->fidVarLast.FVar() );

    columnidMost = fidTaggedMost;
    Call( ErrCATIFindHighestColumnid(
                ppib,
                pfucbCatalogDupe,
                objidTable,
                fidTaggedLeast,
                &columnidMost ) );
    ptcib->fidTaggedLast = FidOfColumnid( columnidMost );
    Assert( ptcib->fidTaggedLast == fidTaggedLeast-1
        || ptcib->fidTaggedLast.FTagged() );


HandleError:
    Assert( pfucbNil != pfucbCatalogDupe );
    CallS( ErrFILECloseTable( ppib, pfucbCatalogDupe ) );

    return err;
}


LOCAL ERR ErrCATIFindLowestColumnid(
    PIB                 *ppib,
    FUCB                *pfucbCatalog,
    const OBJID         objidTable,
    JET_COLUMNID        *pcolumnidLeast )
{
    ERR                 err;
    SYSOBJ              sysobj      = sysobjColumn;
    DATA                dataField;
    BOOL                fLatched    = fFalse;

    Assert( *pcolumnidLeast == fidFixedLeast
        || *pcolumnidLeast == fidVarLeast
        || *pcolumnidLeast == fidTaggedLeast );

    //  should be on primary index
    Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&objidTable,
                sizeof(objidTable),
                JET_bitNewKey ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&sysobj,
                sizeof(sysobj),
                NO_GRBIT ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)pcolumnidLeast,
                sizeof(*pcolumnidLeast),
                NO_GRBIT ) );

    err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekGE );

    if ( JET_wrnSeekNotEqual == err )
    {
        Assert( !Pcsr( pfucbCatalog )->FLatched() );
        Call( ErrDIRGet( pfucbCatalog ) );
        fLatched = fTrue;

        Assert( fidMSO_Type.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Type,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(SYSOBJ) );

        if ( sysobjColumn == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) )
        {
            Assert( fidMSO_Id.FFixed() );
            Call( ErrRECIRetrieveFixedColumn(
                        pfcbNil,
                        pfucbCatalog->u.pfcb->Ptdb(),
                        fidMSO_Id,
                        pfucbCatalog->kdfCurr.data,
                        &dataField ) );
            CallS( err );
            Assert( dataField.Cb() == sizeof(COLUMNID) );

            //  note that we might have found a column of a different type
            *pcolumnidLeast = *( (UnalignedLittleEndian< COLUMNID > *)dataField.Pv() );

            Assert( fidtypUnknown != FidOfColumnid( *pcolumnidLeast ).Fidtyp() );
            
        }
        else
        {
            //  if no columns, must have seeked on some other object
            Assert( sysobjNil != *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );
            *pcolumnidLeast -= 1;
        }
    }

    else if ( JET_errSuccess == err )
    {
#ifdef DEBUG
        Assert( !Pcsr( pfucbCatalog )->FLatched() );
        Call( ErrDIRGet( pfucbCatalog ) );
        fLatched = fTrue;

        Assert( fidMSO_Id.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Id,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(COLUMNID) );

        //  the lowest possible FID actually exists in this table
        Assert( *( (UnalignedLittleEndian< COLUMNID > *)dataField.Pv() ) == *pcolumnidLeast );
#endif
    }

    else
    {
        Assert( err < 0 );
        if ( JET_errRecordNotFound == err )
            err = JET_errSuccess;

        *pcolumnidLeast -= 1;
    }

HandleError:
    if ( fLatched )
    {
        Assert( Pcsr( pfucbCatalog )->FLatched() );
        CallS( ErrDIRRelease( pfucbCatalog ) );
    }

    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    return err;
}


/*  Populate a FIELD structure with column info.  Called by ErrCATConstructTDB()
/*  once the proper column has been located.
/**/
LOCAL ERR ErrCATIInitFIELD(
    PIB         *ppib,
    FUCB        *pfucbCatalog,
#ifdef DEBUG
    OBJID       objidTable,
#endif
    TDB         *ptdb,
    COLUMNID    *pcolumnid,
    FIELD       *pfield,
    DATA        &dataDefaultValue )
{
    ERR         err;
    TDB         *ptdbCatalog;
    DATA        dataField;
    DATA&       dataRec         = pfucbCatalog->kdfCurr.data;

    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Assert( pfcbNil != pfucbCatalog->u.pfcb );
    Assert( pfucbCatalog->u.pfcb->FTypeTable() );
    Assert( pfucbCatalog->u.pfcb->FFixedDDL() );
    Assert( pfucbCatalog->u.pfcb->Ptdb() != ptdbNil );
    ptdbCatalog = pfucbCatalog->u.pfcb->Ptdb();

#ifdef DEBUG
    //  verify still on same table
    Assert( fidMSO_ObjidTable.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_ObjidTable,
                dataRec,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(OBJID) );
    Assert( objidTable == *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) );

    //  verify this is a column
    Assert( fidMSO_Type.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_Type,
                dataRec,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(SYSOBJ) );
    Assert( sysobjColumn == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );
#endif


    Assert( fidMSO_Id.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_Id,
                dataRec,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(COLUMNID) );
    *pcolumnid = *(UnalignedLittleEndian< COLUMNID > *) dataField.Pv();
//  UtilMemCpy( pfid, dataField.Pv(), sizeof(FID) );
    Assert( FCOLUMNIDValid( *pcolumnid ) );
    Assert( !FCOLUMNIDTemplateColumn( *pcolumnid ) );

    if ( ptdb->FTemplateTable() )
        COLUMNIDSetFTemplateColumn( *pcolumnid );

    Assert( fidMSO_Coltyp.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_Coltyp,
                dataRec,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(JET_COLTYP) );
    pfield->coltyp = FIELD_COLTYP( *(UnalignedLittleEndian< JET_COLTYP > *)dataField.Pv() );
//  UtilMemCpy( &pfield->coltyp, dataField.Pv(), sizeof(JET_COLTYP) );
    Assert( pfield->coltyp >= JET_coltypNil );  // May be Nil if column deleted.
    Assert( pfield->coltyp < JET_coltypMax );
    Assert( pfield->coltyp != JET_coltypSLV );

    Assert( fidMSO_SpaceUsage.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_SpaceUsage,
                dataRec,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );
    pfield->cbMaxLen = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
    // FUTURE (as of 9/07/2006): Reject values large enough to cause integer overflows on byte arithmetic.
    // Use a weak bound to avoid blocking any legitimate values.
    //if ( (pfield->cbMaxLen > (ULONG) lMax)&&(pfield->cbMaxLen != (ULONG)-1) )
    //  {
    //  AssertSz( fFalse, "Invalid cbMaxLen value for column in catalog." );
    //  Error( ErrERRCheck( JET_errCatalogCorrupted ) );
    //  }
//  UtilMemCpy( &pfield->cbMaxLen, dataField.Pv(), sizeof(ULONG) );

    Assert( fidMSO_Localization.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_Localization,
                dataRec,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );
    pfield->cp = *(UnalignedLittleEndian< USHORT > *) dataField.Pv();
//  UtilMemCpy( &pfield->cp, dataField.Pv(), sizeof(USHORT) );

    if ( 0 != pfield->cp )
    {
        Assert( FRECTextColumn( pfield->coltyp ) || JET_coltypNil == pfield->coltyp );
        Assert( pfield->cp == usEnglishCodePage || pfield->cp == usUniCodePage );
    }

    Assert( fidMSO_Flags.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_Flags,
                dataRec,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );
    pfield->ffield = *(UnalignedLittleEndian< FIELDFLAG > *) dataField.Pv();
//  UtilMemCpy( &pfield->ffield, dataField.Pv(), sizeof(FIELDFLAG) );
    // None of these flags should be persisted.
    Assert( !FFIELDVersioned( pfield->ffield ) );
    Assert( !FFIELDDeleted( pfield->ffield ) );
    Assert( !FFIELDVersionedAdd( pfield->ffield ) );

    Assert( fidMSO_RecordOffset.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_RecordOffset,
                dataRec,
                &dataField ) );
    if ( JET_wrnColumnNull == err )
    {
        Assert( dataField.Cb() == 0 );
        pfield->ibRecordOffset = 0;     // Set to a dummy value.
    }
    else
    {
        CallS( err );
        Assert( dataField.Cb() == sizeof(REC::RECOFFSET) );
        pfield->ibRecordOffset = *(UnalignedLittleEndian< REC::RECOFFSET > *) dataField.Pv();
//      UtilMemCpy( &pfield->ibRecordOffset, dataField.Pv(), sizeof(REC::RECOFFSET) );
        Assert( pfield->ibRecordOffset >= ibRECStartFixedColumns );
    }

    CHAR    szColumnName[JET_cbNameMost+1];
    Assert( fidMSO_Name.FVar() );
    Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_Name,
                dataRec,
                &dataField ) );
    CallS( err );

    const INT cbDataField = dataField.Cb();

    if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
    {
        AssertSz( fFalse, "Invalid fidMSO_Name column in catalog." );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"061492a1-756d-49fa-ad2c-3338b9d381d3" );
        Error( ErrERRCheck( JET_errCatalogCorrupted ) );
    }
    Assert( sizeof( szColumnName ) > cbDataField );
    UtilMemCpy( szColumnName, dataField.Pv(), cbDataField );
    szColumnName[cbDataField] = 0;      // Add null-termination.

    // Add column name to table's byte pool.  On error, the entire TDB
    // and its byte pool is cleaned up, so we don't have to worry about
    // freeing the memory for the column name.
    Call( ptdb->MemPool().ErrAddEntry(
                (BYTE *)szColumnName,
                dataField.Cb() + 1,
                &pfield->itagFieldName ) );
        pfield->strhashFieldName = StrHashValue( szColumnName );

    if( FFIELDUserDefinedDefault( pfield->ffield ) )
    {
        //  extract the callback information about this column
        //  add a CBDESC to the TDB

        Assert( fidMSO_Callback.FVar() );
        Call( ErrRECIRetrieveVarColumn(
                    pfcbNil,
                    ptdbCatalog,
                    fidMSO_Callback,
                    dataRec,
                    &dataField ) );

        const INT cbDataFieldMSO = dataField.Cb();

        if ( cbDataFieldMSO < 0 || cbDataFieldMSO > JET_cbColumnMost )
        {
            AssertSz( fFalse, "Invalid fidMSO_Callback column in catalog." );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"7900efbb-949e-47be-b488-8ddad49fd3a5" );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }
        CHAR szCallback[JET_cbColumnMost+1];
        AssertPREFIX( sizeof( szCallback ) > cbDataFieldMSO );
        UtilMemCpy( szCallback, dataField.Pv(), cbDataFieldMSO );
        szCallback[cbDataFieldMSO] = '\0';

        JET_CALLBACK callback = NULL;
        if ( BoolParam( PinstFromPpib( ppib ), JET_paramEnablePersistedCallbacks ) )
        {
            Call( ErrCALLBACKResolve( szCallback, &callback ) );
        }

        Assert( fidMSO_CallbackData.FTagged() );
        ULONG cbUserData;
        cbUserData = 0;
        Call( ErrCATIRetrieveTaggedColumn(
                pfucbCatalog,
                fidMSO_CallbackData,
                1,
                dataRec,
                NULL,
                0,
                &cbUserData ) );
        Assert( JET_errSuccess == err
                || JET_wrnColumnNull == err
                || JET_wrnBufferTruncated == err );

        CBDESC * pcbdesc;
        pcbdesc = new CBDESC;
        if( NULL == pcbdesc )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        if( 0 == cbUserData )
        {
            pcbdesc->pvContext  = NULL;
            pcbdesc->cbContext  = 0;
        }
        else
        {
            pcbdesc->pvContext = (BYTE *)PvOSMemoryHeapAlloc( cbUserData );
            pcbdesc->cbContext  = cbUserData;

            if( NULL == pcbdesc->pvContext )
            {
                delete pcbdesc;
                Call( ErrERRCheck( JET_errOutOfMemory ) );
            }

            Call( ErrCATIRetrieveTaggedColumn(
                    pfucbCatalog,
                    fidMSO_CallbackData,
                    1,
                    dataRec,
                    reinterpret_cast<BYTE *>( pcbdesc->pvContext ),
                    cbUserData,
                    &cbUserData ) );
            Assert( JET_wrnBufferTruncated != err );
        }

        pcbdesc->pcallback  = callback;
        pcbdesc->cbtyp      = JET_cbtypUserDefinedDefaultValue;
        pcbdesc->ulId       = *pcolumnid;
        pcbdesc->fPermanent = fTrue;
        pcbdesc->fVersioned = fFalse;

        ptdb->RegisterPcbdesc( pcbdesc );

        ptdb->SetFTableHasUserDefinedDefault();
        ptdb->SetFTableHasNonEscrowDefault();
        ptdb->SetFTableHasDefault();
    }
    else if ( FFIELDDefault( pfield->ffield ) )
    {
        Assert( fidMSO_DefaultValue.FVar() );
        Call( ErrRECIRetrieveVarColumn(
                    pfcbNil,
                    ptdbCatalog,
                    fidMSO_DefaultValue,
                    dataRec,
                    &dataField ) );
        Assert( dataField.Cb() <= cbDefaultValueMost );
        if ( dataField.Cb() > 0 )
        {
            dataField.CopyInto( dataDefaultValue );
        }
        else
        {
            AssertSz( fFalse, "Null default value detected in catalog." );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"dd4e587d-7410-4795-a5e9-33e6e1f5fd83" );
            Call( ErrERRCheck( JET_errCatalogCorrupted ) );
        }

        if ( !FFIELDEscrowUpdate( pfield->ffield ) )
        {
            ptdb->SetFTableHasNonEscrowDefault();
        }
        ptdb->SetFTableHasDefault();
    }
    else
    {
#ifdef DEBUG
        Assert( fidMSO_DefaultValue.FVar() );
        Call( ErrRECIRetrieveVarColumn(
                    pfcbNil,
                    ptdbCatalog,
                    fidMSO_DefaultValue,
                    dataRec,
                    &dataField ) );
        Assert( 0 == dataField.Cb() );
#endif

        if ( FFIELDNotNull( pfield->ffield ) )
        {
            if ( FCOLUMNIDTagged( *pcolumnid ) )
            {
                ptdb->SetFTableHasNonNullTaggedColumn();
            }
            else if ( FCOLUMNIDFixed( *pcolumnid ) )
            {
                ptdb->SetFTableHasNonNullFixedColumn();
            }
            else
            {
                Assert( FCOLUMNIDVar( *pcolumnid ) );
                ptdb->SetFTableHasNonNullVarColumn();
            }
        }
    }

HandleError:
    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( Pcsr( pfucbCatalog )->FLatched() );
    return err;
}


LOCAL VOID CATPatchFixedOffsets(
    TDB *           ptdb,
    const COLUMNID  columnidLastKnownGood,
    const COLUMNID  columnidCurr )
{
    Assert( FidOfColumnid( columnidLastKnownGood ) < FidOfColumnid( columnidCurr ) );
    Assert( FidOfColumnid( columnidLastKnownGood ) >= ptdb->FidFixedFirst() - 1 );
    Assert( FCOLUMNIDFixed( columnidCurr ) );
    Assert( FidOfColumnid( columnidCurr ) > ptdb->FidFixedFirst() );
    Assert( FidOfColumnid( columnidCurr ) <= ptdb->FidFixedLastInitial() );

    FIELD * pfield      = ptdb->PfieldFixed( columnidLastKnownGood + 1 );
    FIELD * pfieldCurr  = ptdb->PfieldFixed( columnidCurr );

    //  this function is only called if there are columns between the
    //  last known good one and the current one
    Assert( pfield < pfieldCurr );

    if ( FidOfColumnid( columnidLastKnownGood ) == ptdb->FidFixedFirst() - 1 )
    {
        pfield->ibRecordOffset = ptdb->IbEndFixedColumns();
        pfield++;
    }

    for ( ; pfield < pfieldCurr; pfield++ )
    {
        FIELD   * pfieldPrev    = pfield - 1;

        //  we're processing columns that have been removed from the catalog
        //  (either because an AddColumn rolled back or because they got
        //  deleted), so their FIELD structures should be zeroed out
        Assert( 0 == pfield->ibRecordOffset );
        Assert( 0 == pfield->cbMaxLen );
        Assert( JET_coltypNil == pfield->coltyp );
        Assert( pfieldPrev->ibRecordOffset >= ibRECStartFixedColumns );
        Assert( pfieldPrev->ibRecordOffset < pfieldCurr->ibRecordOffset );

        //  set offset to a bogus value (it doesn't matter because since
        //  the column is marked as deleted, we'll never retrieve it)
        //  note that previous column may have also been deleted, in
        //  which case its cbMaxLen will be 0, which is why we have
        //  the max() calculation
        pfield->ibRecordOffset = WORD( pfieldPrev->ibRecordOffset
                                        + max( 1, pfieldPrev->cbMaxLen ) );
        Assert( pfield->ibRecordOffset < pfieldCurr->ibRecordOffset );
    }

    Assert( (pfieldCurr-1)->ibRecordOffset + max( 1, (pfieldCurr-1)->cbMaxLen )
        <= pfieldCurr->ibRecordOffset );
}


INLINE VOID CATSetDeletedColumns( TDB *ptdb )
{
    FIELD *             pfield      = ptdb->PfieldsInitial();
    const ULONG         cfields     = ptdb->CInitialColumns();
    const FIELD * const pfieldMax   = pfield + cfields;

    for ( ; pfield < pfieldMax; pfield++ )
    {
        ptdb->AssertFIELDValid( pfield );
        Assert( !FFIELDVersioned( pfield->ffield ) );
        Assert( !FFIELDDeleted( pfield->ffield ) );
        if ( JET_coltypNil == pfield->coltyp )
        {
            FIELDSetDeleted( pfield->ffield );
        }
    }
}

LOCAL ERR ErrCATIBuildFIELDArray(
    PIB             *ppib,
    FUCB            *pfucbCatalog,
    const OBJID     objidTable,
    TDB             *ptdb )
{
    ERR             err;
    const IFMP      ifmp                    = pfucbCatalog->ifmp;
    FUCB            fucbFake( ppib, ifmp );
    FCB             fcbFake( ifmp, objidTable );    // actually expects a pgnoFDP, but it doesn't matter because it never gets referenced
    COLUMNID        columnid                = 0;
    COLUMNID        columnidPrevFixed;
    FIELD           field;
    DATA            dataField;
    BYTE            rgbDefaultValue[cbDefaultValueMost];
    DATA            dataDefaultValue;
    static BOOL     s_fLimitFinalizeFfieldNyi = fFalse;

    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    //  move to columns
    err = ErrBTNext( pfucbCatalog, fDIRNull );
    if ( err < 0 )
    {
        Assert( JET_errRecordDeleted != err );
        Assert( locOnCurBM == pfucbCatalog->locLogical );
        Assert( Pcsr( pfucbCatalog )->FLatched() );
        if ( JET_errNoCurrentRecord != err )
            return err;
    }

    //  even if we got NoCurrentRecord, we are actually
    //  still left on the last node (usually, the DIR
    //  level corrects the currency)
    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    dataDefaultValue.SetPv( rgbDefaultValue );
    FILEPrepareDefaultRecord( &fucbFake, &fcbFake, ptdb );
    Assert( fucbFake.pvWorkBuf != NULL );

    columnidPrevFixed = ColumnidOfFid( ptdb->FidFixedFirst(), ptdb->FTemplateTable() ) - 1;

    //  UPDATE: use new FInitialisingDefaultRecord() flag instead of
    //  fudging FidFixedLast()
/*  //  must reset FidFixedLast to silence DEBUG checks in
    //  SetIbEndFixedColumns() and IbOffsetOfNextColumn()
    const FID   fidFixedLastSave    = ptdb->FidFixedLast();
    ptdb->SetFidFixedLast( FID( ptdb->FidFixedFirst() - 1 ) );
*/

    while ( JET_errNoCurrentRecord != err )
    {
        BOOL    fAddDefaultValue    = fFalse;

        Call( err );

        Assert( locOnCurBM == pfucbCatalog->locLogical );
        Assert( Pcsr( pfucbCatalog )->FLatched() );

        Assert( fidMSO_ObjidTable.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_ObjidTable,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(OBJID) );
        if ( objidTable != *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) )
        {
            goto DoneFields;
        }

        //  verify this is a column
        Assert( fidMSO_Type.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Type,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(SYSOBJ) );
        switch( *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) )
        {
            case sysobjColumn:
                break;

            case sysobjTable:
                AssertSz( fFalse, "Catalog corrupted - sysobj in invalid order." );
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"d6219c39-6388-4192-bac9-c6fa6907a9d1" );
                err = ErrERRCheck( JET_errCatalogCorrupted );
                goto HandleError;

            default:
                goto DoneFields;
        }

        err = ErrCATIInitFIELD(
                    ppib,
                    pfucbCatalog,
#ifdef DEBUG
                    objidTable,
#endif
                    ptdb,
                    &columnid,
                    &field,
                    dataDefaultValue );
        Call( err );

        Assert( FCOLUMNIDValid( columnid ) );
        const BOOL  fFixedColumn    = FCOLUMNIDFixed( columnid );
        if ( fFixedColumn )
        {
            Assert( FidOfColumnid( columnid ) >= ptdb->FidFixedFirst() );
            Assert( FidOfColumnid( columnid ) <= ptdb->FidTaggedLast() );
        }

        if ( !FNegTest( fInvalidAPIUsage ) && FFIELDFinalize( field.ffield ) && !s_fLimitFinalizeFfieldNyi )
        {
            //  This warns if anyone has a previously stored field from JET_bitColumnFinalize grbit usage, that
            //  we've discouraged usage of.  This would be very hard to get rid of.
            AssertTrack( fFalse, "NyiFinalizeBehaviorInPersistedCatFfield" );
            s_fLimitFinalizeFfieldNyi = fTrue;
        }

        // If field is deleted, the coltyp for the FIELD entry should
        // already be JET_coltypNil (initialised that way).
        Assert( field.coltyp != JET_coltypNil
            || ptdb->Pfield( columnid )->coltyp == JET_coltypNil );

        Assert( ( FCOLUMNIDTemplateColumn( columnid ) && ptdb->FTemplateTable() )
            || ( !FCOLUMNIDTemplateColumn( columnid ) && !ptdb->FTemplateTable() ) );
        if ( ptdb->FTemplateTable() )
        {
            if ( !FFIELDTemplateColumnESE98( field.ffield ) )
            {
                ptdb->SetFESE97TemplateTable();

                if ( FCOLUMNIDTagged( columnid )
                    && FidOfColumnid( columnid ) > ptdb->FidTaggedLastOfESE97Template() )
                {
                    ptdb->SetFidTaggedLastOfESE97Template( FidOfColumnid( columnid ) );
                }
            }
        }
        else if ( ptdb->FDerivedTable() )
        {
            FID     fidFirst;

            if ( FCOLUMNIDTagged( columnid ) )
            {
                fidFirst = ptdb->FidTaggedFirst();
            }
            else if ( FCOLUMNIDFixed( columnid ) )
            {
                fidFirst = ptdb->FidFixedFirst();
            }
            else
            {
                Assert( FCOLUMNIDVar( columnid ) );
                fidFirst = ptdb->FidVarFirst();
            }
            if ( FidOfColumnid( columnid ) < fidFirst )
            {
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"60cd374e-ff91-493e-83b5-52ff87d7025f" );
                Call( ErrERRCheck( JET_errDerivedColumnCorruption ) );
            }
        }

        Assert( locOnCurBM == pfucbCatalog->locLogical );
        Assert( Pcsr( pfucbCatalog )->FLatched() );

        if ( field.coltyp != JET_coltypNil )
        {
            Call( ErrRECAddFieldDef( ptdb, columnid, &field ) );

            /*  set version and auto increment field ids (these are mutually
            /*  exclusive (ie. a field can't be both version and autoinc).
            /**/
            Assert( ptdb->FidVersion() != ptdb->FidAutoincrement()
                || ptdb->FidVersion() == 0 );
            if ( FFIELDVersion( field.ffield ) )
            {
                ptdb->SetFidVersion( FidOfColumnid( columnid ) );
            }
            if ( FFIELDAutoincrement( field.ffield ) )
            {
                ptdb->SetFidAutoincrement( FidOfColumnid( columnid ), ( field.coltyp == JET_coltypLongLong || field.coltyp == JET_coltypUnsignedLongLong || field.coltyp == JET_coltypCurrency ) );
            }

            if ( FFIELDFinalize( field.ffield ) )
            {
                Assert( FFIELDEscrowUpdate( field.ffield ) );
                ptdb->SetFTableHasFinalizeColumn();
            }
            if ( FFIELDDeleteOnZero( field.ffield ) )
            {
                Assert( FFIELDEscrowUpdate( field.ffield ) );
                ptdb->SetFTableHasDeleteOnZeroColumn();
            }

            if ( fFixedColumn )
            {
                Assert( field.ibRecordOffset >= ibRECStartFixedColumns );
                Assert( field.cbMaxLen > 0 );

                Assert( FidOfColumnid( columnidPrevFixed ) <= FidOfColumnid( columnid ) - 1 );
                if ( FidOfColumnid( columnidPrevFixed ) < FidOfColumnid( columnid ) - 1 )
                {
                    CATPatchFixedOffsets( ptdb, columnidPrevFixed, columnid );
                }
                columnidPrevFixed = columnid;

                //  Set the last offset.
                ptdb->SetIbEndFixedColumns(
                            WORD( field.ibRecordOffset + field.cbMaxLen ),
                            FidOfColumnid( columnid ) );
            }

            fAddDefaultValue = ( FFIELDDefault( field.ffield )
                                && !FFIELDUserDefinedDefault( field.ffield ) );
        }

        else if ( FCOLUMNIDTagged( columnid ) )
        {
            Assert( FidOfColumnid( columnid ) >= ptdb->FidTaggedFirst() );
            Assert( FidOfColumnid( columnid ) <= ptdb->FidTaggedLast() );
            if ( FidOfColumnid( columnid ) < ptdb->FidTaggedLast()
                && !g_fRepair
                && !g_rgfmp[ifmp].FReadOnlyAttach()
                && !g_rgfmp[ifmp].FShrinkIsRunning()
                && !g_rgfmp[ifmp].FLeakReclaimerIsRunning()
                && JET_errSuccess == ErrPIBCheckUpdatable( ppib ) )
            {
                // Clean up unnecessary MSysColumn entries.
                Call( ErrBTRelease( pfucbCatalog ) );
                Call( ErrIsamDelete( ppib, pfucbCatalog ) );
            }
        }
        else if ( fFixedColumn )
        {
            // For deleted fixed columns, we still need its fixed offset.
            // In addition, we also need its length if it is the last fixed
            // column.  We use this length in order to calculate the
            // offset to the rest of the record (past the fixed data).
            Assert( ptdb->Pfield( columnid )->coltyp == JET_coltypNil );
            Assert( ptdb->Pfield( columnid )->cbMaxLen == 0 );
            Assert( field.ibRecordOffset >= ibRECStartFixedColumns );
            Assert( field.cbMaxLen > 0 );

            ptdb->PfieldFixed( columnid )->ibRecordOffset = field.ibRecordOffset;
            ptdb->PfieldFixed( columnid )->cbMaxLen = field.cbMaxLen;

            Assert( FidOfColumnid( columnidPrevFixed ) <= FidOfColumnid( columnid ) - 1 );
            if ( FidOfColumnid( columnidPrevFixed ) < FidOfColumnid( columnid ) - 1 )
            {
                CATPatchFixedOffsets( ptdb, columnidPrevFixed, columnid );
            }
            columnidPrevFixed = columnid;

            //  Set the last offset.
            ptdb->SetIbEndFixedColumns(
                        WORD( field.ibRecordOffset + field.cbMaxLen ),
                        FidOfColumnid( columnid ) );
        }
        else
        {
            Assert( FCOLUMNIDVar( columnid ) );
            Assert( FidOfColumnid( columnid ) >= ptdb->FidVarFirst() );
            Assert( FidOfColumnid( columnid ) <= ptdb->FidVarLast() );
            if ( FidOfColumnid( columnid ) < ptdb->FidVarLast()
                && !g_fRepair
                && !g_rgfmp[ifmp].FReadOnlyAttach()
                && !g_rgfmp[ifmp].FShrinkIsRunning()
                && !g_rgfmp[ifmp].FLeakReclaimerIsRunning()
                && JET_errSuccess == ErrPIBCheckUpdatable( ppib ) )
            {
                // Clean up unnecessary MSysColumn entries.
                Call( ErrBTRelease( pfucbCatalog ) );
                Call( ErrIsamDelete( ppib, pfucbCatalog ) );
            }
        }

        if ( fAddDefaultValue )
        {
            Assert( FFIELDDefault( field.ffield ) );
            Assert( !FFIELDUserDefinedDefault( field.ffield ) );
            Assert( JET_coltypNil != field.coltyp );
            Assert( FCOLUMNIDValid( columnid ) );

            // Only long values are allowed to be greater than cbColumnMost.
            Assert( dataDefaultValue.Cb() > 0 );
            Assert( FRECLongValue( field.coltyp ) ?
                        dataDefaultValue.Cb() <= JET_cbLVDefaultValueMost :
                        dataDefaultValue.Cb() <= JET_cbColumnMost );

            ptdb->SetFInitialisingDefaultRecord();

            err = ErrRECSetDefaultValue( &fucbFake, columnid, dataDefaultValue.Pv(), dataDefaultValue.Cb() );
            CallS( err );

            ptdb->ResetFInitialisingDefaultRecord();

            Call( err );
        }

        err = ErrBTNext( pfucbCatalog, fDIRNull );

        //  we may have just deleted the record for a
        //  deleted column, and we may be at level 0,
        //  so the record may have gotten expunged
        if ( JET_errRecordDeleted == err )
        {
            BTSetupOnSeekBM( pfucbCatalog );
            err = ErrBTPerformOnSeekBM( pfucbCatalog, fDIRFavourNext );
            Assert( JET_errNoCurrentRecord != err );
            Assert( JET_errRecordDeleted != err );

            //  we never delete the highest columnid,
            //  so there must be at least one left
            Assert( JET_errRecordNotFound != err );
            Call( err );

            Assert( wrnNDFoundLess == err
                || wrnNDFoundGreater == err );
            Assert( Pcsr( pfucbCatalog )->FLatched() );

            if ( wrnNDFoundGreater == err )
            {
                err = JET_errSuccess;
            }
            else
            {
                Assert( wrnNDFoundLess == err );
                err = ErrBTNext( pfucbCatalog, fDIRNull );
                Assert( JET_errNoCurrentRecord != err );
                Assert( JET_errRecordNotFound != err );
                Assert( JET_errRecordDeleted != err );
            }

            pfucbCatalog->locLogical = locOnCurBM;
        }
    }   //  while ( JET_errNoCurrentRecord != err )

    //  verify we didn't unexpectedly break out
    Assert( JET_errNoCurrentRecord == err );

    //  only way to get here is to hit the end of the catalog
    err = ErrERRCheck( wrnCATNoMoreRecords );


DoneFields:

    // Set Deleted bit for all deleted columns.
    CATSetDeletedColumns( ptdb );

    //  in case we have to chain together the buffers (to keep
    //  around copies of previous of old default records
    //  because other threads may have stale pointers),
    //  allocate a RECDANGLING buffer to preface the actual
    //  default record
    //
    RECDANGLING *   precdangling;

    Assert( fucbFake.dataWorkBuf.Cb() >= REC::cbRecordMin );
    Assert( fucbFake.dataWorkBuf.Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ ifmp ].CbPage() ) );
    Assert( fucbFake.dataWorkBuf.Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ ifmp ].CbPage() ) );
    if ( fucbFake.dataWorkBuf.Cb() > REC::CbRecordMost( &fucbFake ) )
    {
        FireWall( "CATIBuildFIELDArrayRecTooBig17.2" );
    }
    if ( fucbFake.dataWorkBuf.Cb() < REC::cbRecordMin || fucbFake.dataWorkBuf.Cb() > REC::CbRecordMostCHECK( g_rgfmp[ ifmp ].CbPage() ) )
    {
        FireWall( "CATIBuildFIELDArrayRecTooBig17.1" );
        err = ErrERRCheck( JET_errDatabaseCorrupted );
        goto HandleError;
    }

    precdangling = (RECDANGLING *)PvOSMemoryHeapAlloc( sizeof(RECDANGLING) + fucbFake.dataWorkBuf.Cb() );
    if ( NULL == precdangling )
    {
        err = ErrERRCheck( JET_errOutOfMemory );
        goto HandleError;
    }

    precdangling->precdanglingNext = NULL;
    precdangling->data.SetPv( (BYTE *)precdangling + sizeof(RECDANGLING) );
    fucbFake.dataWorkBuf.CopyInto( precdangling->data );
    ptdb->SetPdataDefaultRecord( &( precdangling->data ) );

HandleError:
    Assert( JET_errRecordDeleted != err );

    FILEFreeDefaultRecord( &fucbFake );

    //  even if we got NoCurrentRecord, we are actually
    //  still left on the last node (usually, the DIR
    //  level corrects the currency)
    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( Pcsr( pfucbCatalog )->FLatched() || err < 0 );

    return err;
}


/*  construct a table TDB from the column info in the catalog
/**/
INLINE ERR ErrCATIInitTDB(
    PIB                     *ppib,
    FUCB                    *pfucbCatalog,
    const OBJID             objidTable,
    const CHAR              *szTableName,
    const BOOL              fTemplateTable,
    FCB                     *pfcbTemplateTable,
    const JET_SPACEHINTS    *pjsphDeferredLV,
    TDB                     **pptdbNew )
{
    ERR             err;
    TDB             *ptdb           = ptdbNil;
    TCIB            tcib;
    INST            *pinst          = PinstFromPpib( ppib );
    BOOL            fSystemTable    = fFalse;

    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    Call( ErrCATIFindAllHighestColumnids(
                ppib,
                pfucbCatalog,
                objidTable,
                &tcib ) );

    fSystemTable = (    FOLDSystemTable( szTableName ) ||
                        MSysDBM::FIsSystemTable( szTableName ) ||
                        FSCANSystemTable( szTableName ) ||
                        FCATObjidsTable( szTableName ) ||
                        FCATLocalesTable( szTableName ) );

    Call( ErrTDBCreate( pinst, pfucbCatalog->ifmp, &ptdb, &tcib, fSystemTable, pfcbTemplateTable, fTrue ) );

    Assert( ptdb->FidVersion() == 0 );
    Assert( ptdb->FidAutoincrement() == 0 );
    Assert( tcib.fidFixedLast == ptdb->FidFixedLast() );
    Assert( tcib.fidVarLast == ptdb->FidVarLast() );
    Assert( tcib.fidTaggedLast == ptdb->FidTaggedLast() );
    Assert( ptdb->DbkMost() == 0 );
    Assert( ptdb->PfcbLV() == pfcbNil );
    Assert( ptdb->Ui64LidLast() == 0 );

    // Add table name.
    Assert( ptdb->ItagTableName() == 0 );
    MEMPOOL::ITAG   itagNew;
    Call( ptdb->MemPool().ErrAddEntry(
                (BYTE *)szTableName,
                (ULONG)strlen( szTableName ) + 1,
                &itagNew ) );
    Assert( itagNew == itagTDBTableName );
    ptdb->SetItagTableName( itagNew );

    if ( pjsphDeferredLV )
    {
        // Add deferred LV spacehints
        MEMPOOL::ITAG   itagDeferredLVSpacehints;
        Call( ptdb->MemPool().ErrAddEntry(
                    (BYTE*)pjsphDeferredLV,
                    sizeof(*pjsphDeferredLV),
                    &itagDeferredLVSpacehints ) );
        ptdb->SetItagDeferredLVSpacehints( itagDeferredLVSpacehints );
    }
    else
    {
        ptdb->SetItagDeferredLVSpacehints( 0 );
    }

    // Inherit fidAutoinc and/or fidVersion from base table.
    Assert( ptdb->PfcbTemplateTable() == pfcbTemplateTable );

    if ( fTemplateTable )
    {
        ptdb->SetFTemplateTable();
        Assert( pfcbNil == pfcbTemplateTable );
    }
    else if ( pfcbNil != pfcbTemplateTable )
    {
        FID     fidT;

        Assert( ptdb->PfcbTemplateTable() == pfcbTemplateTable );
        ptdb->SetFDerivedTable();
        if ( ptdb->PfcbTemplateTable()->Ptdb()->FESE97TemplateTable() )
            ptdb->SetFESE97DerivedTable();

        // Can't have same fidAutoInc and fidVersion.
        fidT = pfcbTemplateTable->Ptdb()->FidAutoincrement();
        if ( fidT != 0 )
        {
            Assert( fidT != pfcbTemplateTable->Ptdb()->FidVersion() );
            ptdb->SetFidAutoincrement( FidOfColumnid( fidT ), pfcbTemplateTable->Ptdb()->F8BytesAutoInc() );
        }

        fidT = pfcbTemplateTable->Ptdb()->FidVersion();
        if ( fidT != 0 )
        {
            Assert( fidT != pfcbTemplateTable->Ptdb()->FidAutoincrement() );
            ptdb->SetFidVersion( fidT );
        }
    }

    Call( ErrCATIBuildFIELDArray( ppib, pfucbCatalog, objidTable, ptdb ) );
    CallSx( err, wrnCATNoMoreRecords );

    //  even if we got NoCurrentRecord, we are actually
    //  still left on the last node (usually, the DIR
    //  level corrects the currency)
    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    *pptdbNew   = ptdb;

    return err;


HandleError:
    Assert( err < 0 );

    // Delete TDB on error.
    delete ptdb;

    Assert( locOnCurBM == pfucbCatalog->locLogical );

    return err;
}


enum INDEX_UNICODE_STATE
{
    INDEX_UNICODE_GOOD,
    INDEX_UNICODE_DELETE,
    INDEX_UNICODE_OUTOFDATE,
};

//  ================================================================
LOCAL ERR ErrIndexUnicodeState(
    IN const PCWSTR wszLocaleName,
    IN const QWORD  qwVersionCreated,
    IN const SORTID * psortID,
    IN const DWORD dwNormalizationFlags,
    OUT INDEX_UNICODE_STATE * const pState )
//  ================================================================
{
    ERR err = JET_errSuccess;
    SORTID sortID;

    if( !FNORMGetNLSExIsSupported() )
    {
        //  if we don't have the needed API's we have to delete all the indexes

        AssertTrack( fFalse, "GetNLSExNotSupported" );

        OSTrace( JET_tracetagCatalog, "Setting to INDEX_UNICODE_DELETE because NLSEx is not supported.\n" );
        *pState = INDEX_UNICODE_DELETE;
    }
    else if( 0 == qwVersionCreated )
    {
        OSTrace( JET_tracetagCatalog, "Setting to INDEX_UNICODE_DELETE because qwVersionCreated is 0.\n" );
        *pState = INDEX_UNICODE_DELETE;
    }
    else
    {
        QWORD qwVersionCurrent;
        Call( ErrNORMGetSortVersion( wszLocaleName, &qwVersionCurrent, &sortID ) );

        if ( ( NULL != psortID ) && !FSortIDEquals( psortID, &sortID ) )
        {
            WCHAR wszSortIDPerssted[PERSISTED_SORTID_MAX_LENGTH] = L"";
            WCHAR wszSortIDCurrent[PERSISTED_SORTID_MAX_LENGTH] = L"";
            WszCATFormatSortID( *psortID, wszSortIDPerssted, _countof( wszSortIDPerssted ) );
            WszCATFormatSortID( sortID, wszSortIDCurrent, _countof( wszSortIDCurrent ) );
            //  the actual sort ID has changed. this index must be deleted
            OSTrace( JET_tracetagCatalog,
                OSFormat( "Setting to INDEX_UNICODE_DELETE because sort ID for locale=%ws has changed from catalog=%ws to current=%ws.\n",
                          wszLocaleName, wszSortIDPerssted, wszSortIDCurrent ) );
            *pState = INDEX_UNICODE_DELETE;
        }
        else if( !FNORMNLSVersionEquals( qwVersionCreated, qwVersionCurrent ) )
        {
            NORM_LOCALE_VER nlv;
            OSStrCbCopyW( nlv.m_wszLocaleName, sizeof( nlv.m_wszLocaleName ), wszLocaleName );
            nlv.m_sortidCustomSortVersion = *psortID;
            QwSortVersionToNLSDefined( qwVersionCreated, &nlv.m_dwNlsVersion, &nlv.m_dwDefinedNlsVersion );
            nlv.m_dwNormalizationFlags = dwNormalizationFlags;

            if ( ErrNORMCheckLocaleVersion( &nlv ) == JET_errSuccess )
            {
                //  the actual sort order has changed, but this sort order is still valid. This index could either be deleted or used with out-of-date sort order.
                OSTrace( JET_tracetagCatalog,
                    OSFormat( "Setting to INDEX_UNICODE_OUTOFDATE because sort version for locale=%ws has changed from catalog=%#I64x to current=%#I64x.\n",
                              wszLocaleName, qwVersionCreated, qwVersionCurrent ) );
                *pState = INDEX_UNICODE_OUTOFDATE;
            }
            else
            {
                //  the actual sort order has changed. this index must be deleted
                OSTrace( JET_tracetagCatalog,
                    OSFormat( "Setting to INDEX_UNICODE_DELETE because sort version for locale=%ws has changed from catalog=%#I64x to current=%#I64x.\n",
                              wszLocaleName, qwVersionCreated, qwVersionCurrent ) );
                *pState = INDEX_UNICODE_DELETE;
            }
        }
        else
        {
            //  nothing has changed
            Assert( FNORMNLSVersionEquals( qwVersionCreated, qwVersionCurrent ) );
            *pState = INDEX_UNICODE_GOOD;
        }
    }

HandleError:
    return err;
}


/*  Populate an IDB structure with index info.
/**/
LOCAL ERR ErrCATIInitIDB(
    PIB         *ppib,
    FUCB        *pfucbCatalog,
#ifdef DEBUG
    OBJID       objidTable,
#endif
    TDB         * const ptdb,
    IDB         * const pidb,
    PGNO        *ppgnoIndexFDP,
    OBJID       *ppgnoObjidFDP,
    FCB         **ppfcbTemplate,
    __out_bcount(sizeof(JET_SPACEHINTS)) JET_SPACEHINTS * pjsph )
{
    ERR     err;
    TDB     *ptdbCatalog;
    DATA    dataField;
    CHAR    szIndexName[ JET_cbNameMost+1 ];
    ULONG   ul;
    WCHAR   wszIndexLocale[NORM_LOCALE_NAME_MAX_LENGTH];
    FCB     *pfcbTemplateIndex = pfcbNil;

    Assert( ptdbNil != ptdb );
    Assert( pidbNil != pidb );
    Assert( NULL != ppfcbTemplate );
    Assert( NULL != pjsph );

    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( Pcsr( pfucbCatalog )->FLatched() );
    const DATA& dataRec = pfucbCatalog->kdfCurr.data;

    Assert( pfcbNil != pfucbCatalog->u.pfcb );
    Assert( pfucbCatalog->u.pfcb->FTypeTable() );
    Assert( pfucbCatalog->u.pfcb->FFixedDDL() );
    Assert( pfucbCatalog->u.pfcb->Ptdb() != ptdbNil );
    ptdbCatalog = pfucbCatalog->u.pfcb->Ptdb();


#ifdef DEBUG
    //  verify still on same table
    Assert( fidMSO_ObjidTable.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_ObjidTable,
                dataRec,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(OBJID) );
    Assert( objidTable == *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) );

    //  verify this is an index
    Assert( fidMSO_Type.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_Type,
                dataRec,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(SYSOBJ) );
    Assert( sysobjIndex == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );
#endif

    Assert( fidMSO_Name.FVar() );
    Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_Name,
                dataRec,
                &dataField ) );
    CallS( err );

    const INT cbDataField = dataField.Cb();

    if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
    {
        AssertSz( fFalse, "Invalid fidMSO_Name column in catalog." );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"01083d96-fbf5-421e-aee6-f87b2284c43a" );
        Error( ErrERRCheck( JET_errCatalogCorrupted ) );
    }
    Assert( sizeof( szIndexName ) > cbDataField );
    UtilMemCpy( szIndexName, dataField.Pv(), cbDataField );
    szIndexName[cbDataField] = 0;

    Assert( fidMSO_PgnoFDP.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_PgnoFDP,
                dataRec,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(PGNO) );
    *ppgnoIndexFDP = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
//  UtilMemCpy( ppgnoIndexFDP, dataField.Pv(), sizeof(PGNO) );

    Assert( fidMSO_Id.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_Id,
                dataRec,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(OBJID) );
    *ppgnoObjidFDP = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();
//  UtilMemCpy( ppgnoObjidFDP, dataField.Pv(), sizeof(PGNO) );

    Assert( fidMSO_Flags.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_Flags,
                dataRec,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );
    Assert( dataField.Cb() == sizeof(LE_IDXFLAG) );
    const LE_IDXFLAG * const    ple_idxflag = (LE_IDXFLAG*)dataField.Pv();
    pidb->SetPersistedFlags( ple_idxflag->fidb );
    pidb->SetPersistedFlagsX( ple_idxflag->fIDXFlags );
    const IDXFLAG idxflag = ple_idxflag->fIDXFlags;
    // Now that the MSU is deleted, the idxflag should have been cleared at startup time (except if we are under shrink, which runs prior
    // to the fixup).
    Expected( g_fRepair ||
              g_rgfmp[pfucbCatalog->u.pfcb->Ifmp()].FReadOnlyAttach() ||
              g_rgfmp[pfucbCatalog->u.pfcb->Ifmp()].FShrinkIsRunning() ||
              g_rgfmp[pfucbCatalog->u.pfcb->Ifmp()].FLeakReclaimerIsRunning() ||
              !FIDBUnicodeFixupOn_Deprecated( pidb->FPersistedFlags() ) );

    //  verify flags that shouldn't be persisted, aren't
    Assert( !pidb->FVersioned() );
    Assert( !pidb->FVersionedCreate() );
    Assert( !pidb->FDeleted() );

    if ( pidb->FDerivedIndex() )
    {
        pfcbTemplateIndex = ptdb->PfcbTemplateTable();
        Assert( pfcbNil != pfcbTemplateIndex );
        TDB *ptdbTemplate = pfcbTemplateIndex->Ptdb();
        Assert( ptdbNil != ptdbTemplate );

        forever
        {
            Assert( pfcbNil != pfcbTemplateIndex );
            if ( pfcbTemplateIndex->Pidb() == pidbNil )
            {
                Assert( pfcbTemplateIndex == ptdb->PfcbTemplateTable() );
                Assert( pfcbTemplateIndex->FPrimaryIndex() );
                Assert( pfcbTemplateIndex->FSequentialIndex() );
                Assert( pfcbTemplateIndex->FTypeTable() );
            }
            else
            {
                if ( pfcbTemplateIndex == ptdb->PfcbTemplateTable() )
                {
                    Assert( pfcbTemplateIndex->FPrimaryIndex() );
                    Assert( !pfcbTemplateIndex->FSequentialIndex() );
                    Assert( pfcbTemplateIndex->FTypeTable() );
                }
                else
                {
                    Assert( pfcbTemplateIndex->FTypeSecondaryIndex() );
                }
                const USHORT    itagTemplateIndex = pfcbTemplateIndex->Pidb()->ItagIndexName();
                if ( UtilCmpName( szIndexName, ptdbTemplate->SzIndexName( itagTemplateIndex ) ) == 0 )
                    break;
            }

            pfcbTemplateIndex = pfcbTemplateIndex->PfcbNextIndex();
        }

        pfcbTemplateIndex->GetAPISpaceHints( pjsph );
        *ppfcbTemplate = pfcbTemplateIndex;

        // Skip the rest of the initialization steps for non-localized index as the IDB of template table
        // will be linked by the FCB
        if ( pfcbTemplateIndex->Pidb() == pidbNil || !pfcbTemplateIndex->Pidb()->FLocalizedText() )
        {
            return JET_errSuccess;
        }
    }
    else
    {
        // For derived index, the space hint has been retrieved from the template index above
        Call( ErrCATIRetrieveSpaceHints( pfucbCatalog,
                                    ptdbCatalog,
                                    dataRec,
                                    fFalse,
                                    NULL,
                                    pjsph ) );
    }

    //
    // LCID/Locale stuff
    // Over the years, we added support for locale names (2011), kept track of
    // locales in the MSysLocales table, tracked the undefined code points
    // in the MSysUnicodeFixup[2] table, and discovered that locale="" is valid.
    //
    // And we will still assign a 'default' locale even if the user doesn't pass
    // one in.
    // This forces us to support many different formats in the catalog.
    //
    // The first column refers to the input LCID.
    // The other columns reflect what is stored in MSysObjects.
    //
    //
    // LCID Ex2003 (tirtm)      Pre-2011 (e14rtm)   2011+ (e15rtm)      mid-2015            2015
    // ---- -------             --------            ------              ----                ----
    // 0    <default LCID>      <default LCID>      LCID=0              LCID=0              LCID=0
    //      n/a                 n/a                 <default Locale>    <default Locale>    <default Locale>
    //      Flags=0x0001040f    Flags=0x0001140f    Flags=0x0001040f    0x0001040f          0x0001040f
    //
    // 127  LCID=127            LCID=127            LCID=0              LCID=0              LCID=0
    //      n/a                 n/a                 Locale=""           Locale=""           Locale=""
    //      Flags=0x0001044f    Flags=0x0001144f    Flags=0x0001040f    0x0001044f          0x0001044f
    //                          Mountable           Unmountable         Unmountable         Mountable
    //
    // 1033 LCID=1033           LCID=1033           LCID=0              LCID=0              LCID=0
    //      n/a                 n/a                 Locale="en-US"      Locale="en-US       Locale="en-US"
    //      Flags=0x0001044f    Flags=0x0001144f    Flags=0x0001044f    0x0001044f          0x0001044f
    //
    // Notes:
    // -e12rtm behaves the same as e14rtm.
    // -The 'default' LCID refers to the default locale set in the PINST,
    //  via JET_paramUnicodeIndexDefault. It is persisted at that point, and changing
    //  the param later does not affect existing indices.
    // -Flag 0x40 is fidbLocaleSet. If using the default locale, the locale is still
    //  stored in the catalog, but this flag isn't set!
    // -2011 moved away from the LCID, so we have LCID=0, which means 'use the Locale Name'.
    // -Flag 0x1000 is fidbUnicodeFixupOn_Deprecated. i.e. related to the MSU.
    //  old format (pre-Exchange2000): fLocaleSet is FALSE and LCID/locale is not set ("" or 0).
    //

    LCID lcidCatalog = lcidNone;
    SORTID sortid;
    QWORD qwVersionCreated;
    DWORD dwNormalizationFlags;

    Call( ErrCATIRetrieveLocaleInformation(
        pfucbCatalog,
        wszIndexLocale,
        _countof( wszIndexLocale ),
        &lcidCatalog,
        &sortid,
        &qwVersionCreated,
        &dwNormalizationFlags ) );

    // JET_wrnColumnNull indicates that locale name/LCID were not in the catalog.
    Expected( JET_errSuccess == err || JET_wrnColumnNull == err );

    // Set the locale name. The locale name may be "" at this point (for non-text
    // indices and invariant locale).
    // The locale name is *always* set, even for indices containing no text columns.
    pidb->SetWszLocaleName( wszIndexLocale );

    if ( !pidb->FLocalizedText() )
    {
        // This is an index over non-Unicode columns. Even those have the locale name
        // set, but FLocalizedText is only true if a Unicode text
        // column is present.
        // Note that ANSI columns don't show up as FLocalizedText.

        // Set the localized versions to zero. Don't touch the locale name.
        pidb->SetQwSortVersion( 0 );
        pidb->ResetSortidCustomSortVersion();
    }
    else
    {
        Assert( lcidNone != lcidCatalog );

        // There are two possibilities:
        // pidb->FLocaleSet()
        // This branch is hit when the index is initially created with JET_bitIndexUnicode,
        // or an LCID was passed in.
        // The LCID may have been set to 0 (lcidNone) or the locale to "" (invariant).
        // Those are both legal, even though prior to mid-2015 we assumed that an empty
        // locale string wasn't valid.
        //
        // !pidb->FLocaleSet()
        // This branch may still be hit with localized indices. If the user didn't specify
        // a locale, we'd have fallen back to the default, and that's still persisted in
        // the catalog. But FLocaleSet() isn't set.
        //
        // (Actually there was some point pre-2000 when the locale wasn't stored in the
        // catalog, but that is pre-Source Depot, and I don't know how to look up the SLM logs.)

        Assert( pidb->FLocalizedText() );
#ifdef DEBUG
        // Not sure if this is a valid assumption or not: Maybe a zeroed-out version
        // and sort ID are valid.
        Assert( 0 != qwVersionCreated );

        // This assumption is not valid on Win7. Sort IDs were not available until Win8.
        // Assert( !FNORMSortidIsZeroes( &sortid ) );
#endif
        // Verify that the locale name is valid (Shortcut: If it's already a verified
        // locale name, then don't bother verification).
        if ( !FNORMLocaleNameDefault( pidb->WszLocaleName() ) &&
            ( !FNORMEqualsLocaleName( PinstFromPpib( ppib )->m_wszLocaleNameDefault, pidb->WszLocaleName() ) ) )
        {
            Call( ErrNORMCheckLocaleName( PinstFromPpib( ppib ), pidb->WszLocaleName() ) );
        }
        else
        {
            //  workaround Win7:305:  do not assert that the U.S. English locale is present
            //
            //Assert( JET_errSuccess == ErrNORMCheckLocaleName( PinstFromPpib( ppib ), pidb->WszLocaleName() ) );
        }

        pidb->SetQwSortVersion( qwVersionCreated );
        pidb->SetSortidCustomSortVersion( &sortid );
        pidb->SetDwLCMapFlags( dwNormalizationFlags );

        if ( 0 == qwVersionCreated )
        {
            //  this index was created without any sort-order versioning
            //  we'll assume the worst and say the sort-order version is bad
            //  this flag should only be used by the routine that deletes localized
            //  indexes -- i.e. don't Assert( !FBadSortVersion()) as if the
            //  OS version hasn't changed the flag wil lbe set for all legacy indexes
            pidb->SetFBadSortVersion();

            Assert( FNORMSortidIsZeroes( &sortid ) );

            // Ensure we don't have a non-zero SORTID with a zeroed-out version.
            pidb->ResetSortidCustomSortVersion();
        }

        //  if the index includes a unicode column we need to see if the
        //  sort-order for that column has changed

        if ( !pidb->FBadSortVersion() )
        {
            INDEX_UNICODE_STATE state;

            Call( ErrIndexUnicodeState( pidb->WszLocaleName(), qwVersionCreated, &sortid, dwNormalizationFlags, &state ) );

            switch( state )
            {
                case INDEX_UNICODE_DELETE:
                    pidb->SetFBadSortVersion();
                    break;
                case INDEX_UNICODE_OUTOFDATE:
                    pidb->SetFOutOfDateSortVersion();
                    break;
                case INDEX_UNICODE_GOOD:
                    Assert( !pidb->FBadSortVersion() );
                    break;
                default:
                    AssertTrack( fFalse, "InvalidIndexUnicodeStateInitIdb" );
                    break;
            }
        }
    }

    // Done with Locale information!

    Assert( fidMSO_KeyMost.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_KeyMost,
                dataRec,
                &dataField ) );
    if ( dataField.Cb() == 0 )
    {
        Assert( JET_wrnColumnNull == err );
        pidb->SetCbKeyMost( USHORT( JET_cbKeyMost_OLD ) );
    }
    else
    {
        CallS( err );
        Assert( dataField.Cb() == sizeof(SHORT) );
        Assert( *( (UnalignedLittleEndian<SHORT> *)dataField.Pv()) <= cbKeyMostMost );
        pidb->SetCbKeyMost( USHORT( *(UnalignedLittleEndian< USHORT > *)dataField.Pv() ) );
//      UtilMemCpy( &pidb->cbKeyMost, dataField.Pv(), sizeof(pidb->cbKeyMost) );
        Assert( pidb->CbKeyMost() > 0 );
        Assert( pidb->CbKeyMost() <= cbKeyMostMost );
    }

    //  UNDONE: make VarSegMac an array parallel to
    //  KeyFldIDs so cbVarSegMac can be specified
    //  on a per-field basis
    //
    Assert( fidMSO_VarSegMac.FVar() );
    Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_VarSegMac,
                dataRec,
                &dataField ) );
    if ( dataField.Cb() == 0 )
    {
        Assert( JET_wrnColumnNull == err );
        pidb->SetCbVarSegMac( pidb->CbKeyMost() );
    }
    else
    {
        CallS( err );
        Assert( dataField.Cb() == sizeof(USHORT) );
        Assert( *( (UnalignedLittleEndian<USHORT> *)dataField.Pv()) <= cbKeyMostMost );
        pidb->SetCbVarSegMac( USHORT( *(UnalignedLittleEndian< USHORT > *)dataField.Pv() ) );
//      UtilMemCpy( &pidb->cbVarSegMac, dataField.Pv(), sizeof(pidb->cbVarSegMac) );
        Assert( pidb->CbVarSegMac() > 0 );
        Assert( pidb->CbVarSegMac() <= cbKeyMostMost );
    }

    Assert( fidMSO_KeyFldIDs.FVar() );
    Call( ErrRECIRetrieveVarColumn(
            pfcbNil,
            ptdbCatalog,
            fidMSO_KeyFldIDs,
            dataRec,
            &dataField ) );
    CallS( err );
    Assert( dataField.Cb() > 0 );

    // the length of the list of key fields should be a multiple
    // of the length of one field.
    if ( FIDXExtendedColumns( idxflag ) )
    {
        Assert( sizeof(IDXSEG) == sizeof(JET_COLUMNID) );
        Assert( dataField.Cb() <= sizeof(JET_COLUMNID) * JET_ccolKeyMost );
        Assert( dataField.Cb() % sizeof(JET_COLUMNID) == 0 );
        Assert( dataField.Cb() / sizeof(JET_COLUMNID) <= JET_ccolKeyMost );
        pidb->SetCidxseg( (BYTE)( dataField.Cb() / sizeof(JET_COLUMNID) ) );
        Call( ErrIDBSetIdxSeg( pidb, ptdb, fFalse, ( const LE_IDXSEG* const )dataField.Pv() ) );
    }
    else
    {
        Assert( sizeof(IDXSEG_OLD) == sizeof(FID) );
        Assert( dataField.Cb() <= sizeof(FID) * JET_ccolKeyMost );
        Assert( dataField.Cb() % sizeof(FID) == 0);
        Assert( dataField.Cb() / sizeof(FID) <= JET_ccolKeyMost );
        pidb->SetCidxseg( (BYTE)( dataField.Cb() / sizeof(FID) ) );
        Call( ErrIDBSetIdxSegFromOldFormat(
                    pidb,
                    ptdb,
                    fFalse,
                    (UnalignedLittleEndian< IDXSEG_OLD > *)dataField.Pv() ) );
    }


    Assert( fidMSO_ConditionalColumns.FVar() );
    Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_ConditionalColumns,
                dataRec,
                &dataField ) );

    // the length of the list of key fields should be a multiple
    // of the length of one field.
    if ( FIDXExtendedColumns( idxflag ) )
    {
        Assert( sizeof(IDXSEG) == sizeof(JET_COLUMNID) );
        Assert( dataField.Cb() <= sizeof(JET_COLUMNID) * JET_ccolKeyMost );
        Assert( dataField.Cb() % sizeof(JET_COLUMNID) == 0 );
        Assert( dataField.Cb() / sizeof(JET_COLUMNID) <= JET_ccolKeyMost );
        pidb->SetCidxsegConditional( (BYTE)( dataField.Cb() / sizeof(JET_COLUMNID) ) );
        Call( ErrIDBSetIdxSeg( pidb, ptdb, fTrue, (LE_IDXSEG*)dataField.Pv() ) );
    }
    else
    {
        Assert( sizeof(IDXSEG_OLD) == sizeof(FID) );
        Assert( dataField.Cb() <= sizeof(FID) * JET_ccolKeyMost );
        Assert( dataField.Cb() % sizeof(FID) == 0);
        Assert( dataField.Cb() / sizeof(FID) <= JET_ccolKeyMost );
        pidb->SetCidxsegConditional( (BYTE)( dataField.Cb() / sizeof(FID) ) );
        Call( ErrIDBSetIdxSegFromOldFormat(
                    pidb,
                    ptdb,
                    fTrue,
                    (UnalignedLittleEndian< IDXSEG_OLD > *)dataField.Pv() ) );
    }


    Assert( fidMSO_TupleLimits.FVar() );
    Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_TupleLimits,
                dataRec,
                &dataField ) );
    if ( dataField.Cb() == 0 )
    {
        Assert( JET_wrnColumnNull == err );
    }
    else
    {
        CallS( err );
        Assert( dataField.Cb() == sizeof(LE_TUPLELIMITS_V1)
            || dataField.Cb() == sizeof(LE_TUPLELIMITS) );

        if ( dataField.Cb() == sizeof(LE_TUPLELIMITS) )
        {
            const LE_TUPLELIMITS * const    ple_tuplelimits     = (LE_TUPLELIMITS *)dataField.Pv();

            pidb->SetFTuples();

            ul = ple_tuplelimits->le_chLengthMin;
            pidb->SetCchTuplesLengthMin( (USHORT)ul );

            ul = ple_tuplelimits->le_chLengthMax;
            pidb->SetCchTuplesLengthMax( (USHORT)ul );

            ul = ple_tuplelimits->le_chToIndexMax;
            pidb->SetIchTuplesToIndexMax( (USHORT)ul );

            ul = ple_tuplelimits->le_cchIncrement;
            pidb->SetCchTuplesIncrement( (USHORT)ul );

            ul = ple_tuplelimits->le_ichStart;
            pidb->SetIchTuplesStart( (USHORT)ul );
        }
        else
        {
            const LE_TUPLELIMITS_V1 * const ple_tuplelimits     = (LE_TUPLELIMITS_V1 *)dataField.Pv();

            pidb->SetFTuples();

            ul = ple_tuplelimits->le_chLengthMin;
            pidb->SetCchTuplesLengthMin( (USHORT)ul );

            ul = ple_tuplelimits->le_chLengthMax;
            pidb->SetCchTuplesLengthMax( (USHORT)ul );

            ul = ple_tuplelimits->le_chToIndexMax;
            pidb->SetIchTuplesToIndexMax( (USHORT)ul );

            //  fill in columns not present in previous tuple limits version
            //
            pidb->SetCchTuplesIncrement( (USHORT)1 );

            pidb->SetIchTuplesStart( (USHORT)0 );
        }
    }


    USHORT  itagIndexName;

    if ( !pidb->FDerivedIndex() )
    {
        // Add index name to table's byte pool.  On error, the entire TDB
        // and its byte pool is cleaned up, so we don't have to worry about
        // freeing the memory for the index name.
        Call( ptdb->MemPool().ErrAddEntry(
                        (BYTE *)szIndexName,
                        (ULONG)strlen( szIndexName ) + 1,
                        &itagIndexName ) );

        pidb->SetItagIndexName( itagIndexName );
    }
    else
    {
        Assert( pfcbNil != pfcbTemplateIndex );
        Assert( pidbNil != pfcbTemplateIndex->Pidb() );
        Assert( pfcbTemplateIndex->Pidb()->FLocalizedText() );
        itagIndexName = pfcbTemplateIndex->Pidb()->ItagIndexName();

        pidb->SetItagIndexName( itagIndexName );

        // Points the itag for sort columns to the template table's mempool if they are backed by mempool.
        if ( pidb->FIsRgidxsegInMempool() )
        {
            pidb->SetItagRgidxseg( pfcbTemplateIndex->Pidb()->ItagRgidxseg() );
        }

        // Points the itag for conditional columns to the template table's mempool if they are backed by mempool.
        if ( pidb->FIsRgidxsegConditionalInMempool() )
        {
            pidb->SetItagRgidxsegConditional( pfcbTemplateIndex->Pidb()->ItagRgidxsegConditional() );
        }
    }

    Assert( pidb->CbVarSegMac() <= pidb->CbKeyMost() );

HandleError:
    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( Pcsr( pfucbCatalog )->FLatched() );
    return err;
}

LOCAL ERR ErrCATIInitIndexFCBs(
    PIB             *ppib,
    FUCB            *pfucbCatalog,
    const OBJID     objidTable,
    FCB             * const pfcb,
    TDB             * const ptdb,
    const JET_SPACEHINTS * const pjsphPrimary )
{
    ERR             err                     = JET_errSuccess;
    const IFMP      ifmp                    = pfcb->Ifmp();
    FCB             *pfcbSecondaryIndexes   = pfcbNil;
    BOOL            fFoundPrimary           = fFalse;

    Assert( pfcb->Pidb() == pidbNil );
    Assert( ptdbNil != ptdb );

    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    do
    {
        DATA        dataField;
        IDB         idb( PinstFromIfmp( ifmp ) );
        PGNO        pgnoIndexFDP;
        OBJID       objidIndexFDP;

        Call( err );

        Assert( locOnCurBM == pfucbCatalog->locLogical );
        Assert( Pcsr( pfucbCatalog )->FLatched() );

        Assert( fidMSO_ObjidTable.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_ObjidTable,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(OBJID) );
        if ( objidTable != *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) )
        {
            goto CheckPrimary;
        }

        //  verify this is an index
        Assert( fidMSO_Type.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Type,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(SYSOBJ) );
        switch ( *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) )
        {
            case sysobjIndex:
                break;

            case sysobjLongValue:
            case sysobjCallback:
                goto CheckPrimary;

            case sysobjTable:
            case sysobjColumn:
            default:
                AssertSz( fFalse, "Catalog corrupted - sysobj in invalid order." );
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"81691705-19db-4f9e-8c94-2a0631119c7c" );
                err = ErrERRCheck( JET_errCatalogCorrupted );
                goto HandleError;
        }


        /*  read the data
        /**/

        FCB * pfcbTemplate = NULL;
        JET_SPACEHINTS jsph = { 0 };

        err = ErrCATIInitIDB(
                    ppib,
                    pfucbCatalog,
#ifdef DEBUG
                    objidTable,
#endif
                    ptdb,
                    &idb,
                    &pgnoIndexFDP,
                    &objidIndexFDP,
                    &pfcbTemplate,
                    &jsph );
        Call( err );

        if ( idb.FPrimary() )
        {
            Assert( !fFoundPrimary );
            fFoundPrimary = fTrue;
            Assert( pgnoIndexFDP == pfcb->PgnoFDP() || g_fRepair );
            Call( ErrFILEIInitializeFCB(
                ppib,
                ifmp,
                ptdb,
                pfcb,
                &idb,
                fTrue,
                pfcb->PgnoFDP(),
                pjsphPrimary,
                pfcbTemplate ) );
            pfcb->Lock();
            pfcb->SetInitialIndex();
            pfcb->Unlock();
        }
        else
        {
            FUCB    *pfucbSecondaryIndex;

            Assert( pgnoIndexFDP != pfcb->PgnoFDP() || g_fRepair );
            Call( ErrDIROpenNoTouch(
                        ppib,
                        ifmp,
                        pgnoIndexFDP,
                        objidIndexFDP,
                        idb.FUnique(),
                        &pfucbSecondaryIndex,
                        fTrue ) );              // Will initialize FCB
            Assert( !pfucbSecondaryIndex->u.pfcb->FInitialized() || pfucbSecondaryIndex->u.pfcb->FInitedForRecovery() );

            err = ErrFILEIInitializeFCB(
                    ppib,
                    ifmp,
                    ptdb,
                    pfucbSecondaryIndex->u.pfcb,
                    &idb,
                    fFalse,
                    pgnoIndexFDP,
                    &jsph,
                    pfcbTemplate );
            if ( err < 0 )
            {
                DIRClose( pfucbSecondaryIndex );
                goto HandleError;
            }
            Assert( pfucbSecondaryIndex->u.pfcb->ObjidFDP() == objidIndexFDP );

            pfucbSecondaryIndex->u.pfcb->SetPfcbNextIndex( pfcbSecondaryIndexes );
            pfcbSecondaryIndexes = pfucbSecondaryIndex->u.pfcb;

            Assert( !pfucbSecondaryIndex->u.pfcb->FInList() );

            //  mark the secondary index as being initialized successfully

            pfucbSecondaryIndex->u.pfcb->Lock();
            pfucbSecondaryIndex->u.pfcb->SetInitialIndex();
            pfucbSecondaryIndex->u.pfcb->CreateComplete();
            pfucbSecondaryIndex->u.pfcb->ResetInitedForRecovery();
            pfucbSecondaryIndex->u.pfcb->Unlock();

            DIRClose( pfucbSecondaryIndex );
        }

        Assert( locOnCurBM == pfucbCatalog->locLogical );
        Assert( Pcsr( pfucbCatalog )->FLatched() );

        err = ErrBTNext( pfucbCatalog, fDIRNull );
        Assert( JET_errRecordDeleted != err );
    }
    while ( JET_errNoCurrentRecord != err );

    //  verify we didn't unexpectedly break out
    Assert( JET_errNoCurrentRecord == err );

    //  only way to get here is to hit the end of the catalog
    err = ErrERRCheck( wrnCATNoMoreRecords );

CheckPrimary:
    CallSx( err, wrnCATNoMoreRecords );

    if ( !fFoundPrimary )
    {
        const ERR   errInit = ErrFILEIInitializeFCB(
                                        ppib,
                                        ifmp,
                                        ptdb,
                                        pfcb,
                                        pidbNil,
                                        fTrue,
                                        pfcb->PgnoFDP(),
                                        pjsphPrimary, // actually default sequential
                                        NULL );
        if ( errInit < 0 )
        {
            err = errInit;
            goto HandleError;
        }

        //  should be success, so just ignore it (to preserve wrnCATNoMoreRecords)
        CallS( errInit );
    }

    /*  link up sequential/primary index with the rest of the indexes
    /**/
    pfcb->SetPfcbNextIndex( pfcbSecondaryIndexes );

    /*  link up pfcbTable of secondary indexes
    /**/
    pfcb->LinkPrimaryIndex();

    FILESetAllIndexMask( pfcb );

    //  even if we got NoCurrentRecord, we are actually
    //  still left on the last node (usually, the DIR
    //  level corrects the currency)
    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    return err;

HandleError:
    Assert( err < 0 );

    if ( pfcbNil != pfcbSecondaryIndexes )
    {
        CATIFreeSecondaryIndexes( pfcbSecondaryIndexes );
    }

    // No need to explicitly free index name or idxseg array, since
    // memory pool will be freed when TDB is deleted below.
    if ( pfcb->Pidb() != pidbNil )
    {
        pfcb->ReleasePidb();
        pfcb->SetPidb( pidbNil );
    }

    //  even if we got NoCurrentRecord, we are actually
    //  still left on the last node (usually, the DIR
    //  level corrects the currency)
    Assert( ( err != JET_errNoCurrentRecord ) || ( locOnCurBM == pfucbCatalog->locLogical && Pcsr( pfucbCatalog )->FLatched() ) );

    return err;
}


//  ================================================================
ERR ErrCATCopyCallbacks(
    PIB * const ppib,
    const IFMP ifmpSrc,
    const IFMP ifmpDest,
    const OBJID objidSrc,
    const OBJID objidDest )
//  ================================================================
//
//  Used by defrag to transport callbacks from one table to another
//
//-
{
    const SYSOBJ    sysobj = sysobjCallback;

    ERR err = JET_errSuccess;

    FUCB * pfucbSrc = pfucbNil;
    FUCB * pfucbDest = pfucbNil;

    CallR( ErrDIRBeginTransaction( ppib, 38629, NO_GRBIT ) );

    Call( ErrCATOpen( ppib, ifmpSrc, &pfucbSrc ) );
    Call( ErrCATOpen( ppib, ifmpDest, &pfucbDest ) );

    Call( ErrIsamMakeKey(
                ppib,
                pfucbSrc,
                (BYTE *)&objidSrc,
                sizeof(objidSrc),
                JET_bitNewKey ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbSrc,
                (BYTE *)&sysobj,
                sizeof(sysobj),
                NO_GRBIT ) );

    err = ErrIsamSeek( ppib, pfucbSrc, JET_bitSeekGT );
    if ( err < 0 )
    {
        if ( JET_errRecordNotFound != err )
            goto HandleError;
    }
    else
    {
        CallS( err );

        Call( ErrIsamMakeKey(
                    ppib,
                    pfucbSrc,
                    (BYTE *)&objidSrc,
                    sizeof(objidSrc),
                    JET_bitNewKey ) );
        Call( ErrIsamMakeKey(
                    ppib,
                    pfucbSrc,
                    (BYTE *)&sysobj,
                    sizeof(sysobj),
                    JET_bitStrLimit ) );
        err = ErrIsamSetIndexRange( ppib, pfucbSrc, JET_bitRangeUpperLimit );
        Assert( err <= 0 );

        while ( JET_errNoCurrentRecord != err )
        {
            Call( err );

            JET_RETRIEVECOLUMN  rgretrievecolumn[3];
            INT                 iretrievecolumn     = 0;

            ULONG               cbtyp;
            CHAR                szCallback[JET_cbColumnMost+1];

            memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );

            rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Flags;
            rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&cbtyp;
            rgretrievecolumn[iretrievecolumn].cbData        = sizeof( cbtyp );
            rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
            ++iretrievecolumn;

            rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Callback;
            rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)szCallback;
            rgretrievecolumn[iretrievecolumn].cbData        = sizeof( szCallback );
            rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
            ULONG& cbCallback = rgretrievecolumn[iretrievecolumn].cbActual;
            ++iretrievecolumn;

            Call( ErrIsamRetrieveColumns(
                    (JET_SESID)ppib,
                    (JET_TABLEID)pfucbSrc,
                    rgretrievecolumn,
                    iretrievecolumn ) );


            if ( cbCallback >= sizeof( szCallback ) )
            {
                AssertSz( fFalse, "Invalid fidMSO_Callback column in catalog." );
                Error( ErrERRCheck( JET_errCatalogCorrupted ) );
            }
            szCallback[cbCallback] = 0;

            Assert( cbtyp != JET_cbtypNull );

            Call( ErrCATAddTableCallback( ppib, pfucbDest, objidDest, (JET_CBTYP)cbtyp, szCallback ) );
            err = ErrIsamMove( ppib, pfucbSrc, JET_MoveNext, NO_GRBIT );
        }

        Assert( JET_errNoCurrentRecord == err );
    }

    err = JET_errSuccess;

HandleError:
    if( pfucbNil != pfucbSrc )
    {
        CallS( ErrCATClose( ppib, pfucbSrc ) );
    }
    if( pfucbNil != pfucbDest )
    {
        CallS( ErrCATClose( ppib, pfucbDest ) );
    }

    if( err >= 0 )
    {
        err = ErrDIRCommitTransaction( ppib, NO_GRBIT );
    }
    else
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    return err;
}


//  ================================================================
LOCAL ERR ErrCATIInitCallbacks(
    PIB             *ppib,
    FUCB            *pfucbCatalog,
    const OBJID     objidTable,
    FCB             * const pfcb,
    TDB             * const ptdb )
//  ================================================================
{
    ERR             err                         = JET_errSuccess;
    BOOL            fMovedOffCurrentRecord      = fFalse;
    JET_CALLBACK    callback                    = NULL;
    ULONG           cbtyp;
    CHAR            szCallback[JET_cbColumnMost+1];
    DATA            dataField;

    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Assert( !BoolParam( PinstFromPpib( ppib ), JET_paramDisableCallbacks ) );

    do
    {
        Call( err );

        Assert( locOnCurBM == pfucbCatalog->locLogical );
        Assert( Pcsr( pfucbCatalog )->FLatched() );

        Assert( fidMSO_ObjidTable.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_ObjidTable,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(OBJID) );
        if ( objidTable != *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) )
        {
            goto DoneCallbacks;
        }

        //  verify this is a callback
        Assert( fidMSO_Type.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Type,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(SYSOBJ) );
        switch( *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) )
        {
            case sysobjCallback:
            {
                Assert( fidMSO_Flags.FFixed() );
                Call( ErrRECIRetrieveFixedColumn(
                            pfcbNil,
                            pfucbCatalog->u.pfcb->Ptdb(),
                            fidMSO_Flags,
                            pfucbCatalog->kdfCurr.data,
                            &dataField ) );
                CallS( err );
                Assert( dataField.Cb() == sizeof(ULONG) );
                cbtyp = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
                Assert( JET_cbtypNull != cbtyp );

                Assert( fidMSO_Callback.FVar() );
                Call( ErrRECIRetrieveVarColumn(
                            pfcbNil,
                            pfucbCatalog->u.pfcb->Ptdb(),
                            fidMSO_Callback,
                            pfucbCatalog->kdfCurr.data,
                            &dataField ) );
                CallS( err );

                const INT cbDataField = dataField.Cb();

                if ( cbDataField < 0 || cbDataField >= sizeof( szCallback ) )
                {
                    AssertSz( fFalse, "Invalid fidMSO_Callback column in catalog." );
                    OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"27d5acf2-2cde-49b1-a205-6e95cbe93484" );
                    Error( ErrERRCheck( JET_errCatalogCorrupted ) );
                }
                UtilMemCpy( szCallback, dataField.Pv(), cbDataField );
                szCallback[cbDataField] = 0;        // Add null-termination.

                if ( BoolParam( PinstFromPpib( ppib ), JET_paramEnablePersistedCallbacks ) )
                {
                    Call( ErrCALLBACKResolve( szCallback, &callback ) );
                }

                CBDESC * const pcbdescInsert = new CBDESC;  //  freed in TDB::Delete
                if( NULL == pcbdescInsert )
                {
                    Call( ErrERRCheck( JET_errOutOfMemory ) );
                }

                pcbdescInsert->pcbdescNext  = NULL;
                pcbdescInsert->ppcbdescPrev = NULL;
                pcbdescInsert->pcallback    = callback;
                pcbdescInsert->cbtyp        = cbtyp;
                pcbdescInsert->pvContext    = NULL;
                pcbdescInsert->cbContext    = 0;
                pcbdescInsert->ulId         = 0;
                pcbdescInsert->fPermanent   = fTrue;
                pcbdescInsert->fVersioned   = fFalse;
                ptdb->RegisterPcbdesc( pcbdescInsert );
                break;
            }

            case sysobjLongValue:
                //  skip LV's and continue moving to next record
                if ( !fMovedOffCurrentRecord )
                {
                    break;
                }

                //  LV's MUST come before callbacks, else
                //  FALL THROUGH to corruption case

            case sysobjTable:
            case sysobjColumn:
            case sysobjIndex:
            default:
                //  callbacks are currently the last type of table-level sysobjs
                AssertSz( fFalse, "Catalog corrupted - sysobj in invalid order." );
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"f1f3f1f8-1928-4d24-aa5e-b3865adf481d" );
                err = ErrERRCheck( JET_errCatalogCorrupted );
                goto HandleError;
        }

        Assert( locOnCurBM == pfucbCatalog->locLogical );
        Assert( Pcsr( pfucbCatalog )->FLatched() );

        fMovedOffCurrentRecord = fTrue;
        err = ErrBTNext( pfucbCatalog, fDIRNull );
        Assert( JET_errRecordDeleted != err );
    }
    while ( JET_errNoCurrentRecord != err );

    //  verify we didn't unexpectedly break out
    Assert( JET_errNoCurrentRecord == err );

    //  only way to get here is to hit the end of the catalog
    err = ErrERRCheck( wrnCATNoMoreRecords );


DoneCallbacks:
    CallSx( err, wrnCATNoMoreRecords );

HandleError:
    //  even if we got NoCurrentRecord, we are actually
    //  still left on the last node (usually, the DIR
    //  level corrects the currency)
    Assert( ( err != JET_errNoCurrentRecord && err != JET_errSuccess ) || ( locOnCurBM == pfucbCatalog->locLogical && Pcsr( pfucbCatalog )->FLatched() ) );

    return err;
}


ERR ErrCATInitFCB( FUCB *pfucbTable, OBJID objidTable )
{
    ERR         err;
    PIB         *ppib                   = pfucbTable->ppib;
    INST        *pinst                  = PinstFromPpib( ppib );
    const IFMP  ifmp                    = pfucbTable->ifmp;
    FUCB        *pfucbCatalog           = pfucbNil;
    FCB         *pfcb                   = pfucbTable->u.pfcb;
    TDB         *ptdb                   = ptdbNil;
    FCB         *pfcbTemplateTable      = pfcbNil;
    DATA        dataField;
    ULONG       ulFlags;
    BOOL        fHitEOF                 = fFalse;
    CHAR        szTableName[ JET_cbNameMost + 1 ];
    UnalignedLittleEndian<ULONG> le_ulSeparateLV = 0;
    JET_SPACEHINTS jsphPrimary;
    JET_SPACEHINTS jsphTemplate;
    JET_SPACEHINTS jsphPrimaryDeferredLV = { 0 };
    BOOL        fSetDeferredLVSpacehints = fFalse;

    if ( FFMPIsTempDB( ifmp ) )
    {
        // Temporary table FCBs are initialised by CATInitTempFCB.
        AssertSz( fFalse, "API never called on temp table." );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    Assert( !pfcb->FInitialized() || pfcb->FInitedForRecovery() );
    Assert( objidTable == pfucbTable->u.pfcb->ObjidFDP()
            || objidNil == pfucbTable->u.pfcb->ObjidFDP() && g_fRepair );

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    FUCBSetPrereadForward( pfucbCatalog, 3 );

    Call( ErrCATISeekTable( ppib, pfucbCatalog, objidTable ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    //  should be on primary index
    Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
    Assert( locOnCurBM == pfucbCatalog->locLogical );

    Assert( fidMSO_Name.FVar() );
    Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Name,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );

    INT cbDataField = dataField.Cb();

    if ( cbDataField < 0 || cbDataField  > JET_cbNameMost )
    {
        AssertSz( fFalse, "Invalid fidMSO_Name column in catalog." );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"d6250c09-4c3c-47b6-9fc8-f556a29ffce5" );
        Error( ErrERRCheck( JET_errCatalogCorrupted ) );
    }
    Assert( sizeof( szTableName ) > cbDataField );
    UtilMemCpy( szTableName, dataField.Pv(), cbDataField );
    szTableName[ cbDataField ] = 0;

    OSTraceFMP(
        ifmp,
        JET_tracetagCatalog,
        OSFormat(
            "Session=[0x%p:0x%x] is faulting in schema for table '%s' of [0x%x:0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            szTableName,
            (ULONG)ifmp,
            objidTable ) );

    Assert( fidMSO_SeparateLVThreshold.FTagged() );
    ULONG cbActual = 0;
    Call( ErrCATIRetrieveTaggedColumn(
                    pfucbCatalog,
                    fidMSO_SeparateLVThreshold,
                    1,
                    pfucbCatalog->kdfCurr.data,
                    (BYTE*)&le_ulSeparateLV,
                    sizeof(le_ulSeparateLV),
                    &cbActual ) );
    Assert( JET_errSuccess == err
            || JET_wrnColumnNull == err );
    Assert( JET_wrnColumnNull == err ||
            cbActual == sizeof(ULONG) );
    if ( JET_errSuccess != err ||
        cbActual != sizeof(ULONG) )
    {
        if ( JET_wrnColumnNull != err )
        {
            AssertSz( fFalse, "Invalid fidMSO_SeparateLVThreshold column in catalog, size not right." );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"8f6c6cd5-ff7c-4079-ae01-2cdb1b38500e" );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }
        le_ulSeparateLV = 0;
    }
    ULONG cbSeparateLV = le_ulSeparateLV;   // if we leave this at zero, we get the defautl later ...

    Assert( fidMSO_Flags.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Flags,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );
    ulFlags = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();

    if ( ulFlags & JET_bitObjectTableTemplate )
    {
        Assert( ulFlags & JET_bitObjectTableFixedDDL );
        Assert( !( ulFlags & JET_bitObjectTableDerived ) );

        pfcb->Lock();
        pfcb->SetFixedDDL();
        pfcb->SetTemplateTable();
        pfcb->Unlock();
    }
    else
    {
        if ( ulFlags & JET_bitObjectTableFixedDDL )
        {
            pfcb->Lock();
            pfcb->SetFixedDDL();
            pfcb->Unlock();
        }

        if ( ulFlags & JET_bitObjectTableDerived )
        {
            CHAR    szTemplateTable[JET_cbNameMost+1];
            FUCB    *pfucbTemplateTable;

            Assert( fidMSO_TemplateTable.FVar() );
            Call( ErrRECIRetrieveVarColumn(
                        pfcbNil,
                        pfucbCatalog->u.pfcb->Ptdb(),
                        fidMSO_TemplateTable,
                        pfucbCatalog->kdfCurr.data,
                        &dataField ) );
            CallS( err );

            cbDataField = dataField.Cb();

            if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
            {
                AssertSz( fFalse, "Invalid fidMSO_TemplateTable column in catalog." );
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"ddb15f29-ca94-4d4c-88fb-3c6538544ba9" );
                Error( ErrERRCheck( JET_errCatalogCorrupted ) );
            }
            Assert( sizeof( szTemplateTable ) > cbDataField );
            UtilMemCpy( szTemplateTable, dataField.Pv(), cbDataField );
            szTemplateTable[cbDataField] = '\0';

            //  release latch once we're done retrieving data from Table record
            Call( ErrBTRelease( pfucbCatalog ) );

            Call( ErrFILEOpenTable(
                        ppib,
                        ifmp,
                        &pfucbTemplateTable,
                        szTemplateTable,
                        JET_bitTableReadOnly ) );
            Assert( pfucbNil != pfucbTemplateTable );

            pfcbTemplateTable = pfucbTemplateTable->u.pfcb;
            Assert( pfcbNil != pfcbTemplateTable );
            Assert( pfcbTemplateTable->FTemplateTable() );
            Assert( pfcbTemplateTable->FFixedDDL() );

            pfcbTemplateTable->GetAPISpaceHints( &jsphTemplate );

            //  flag the template as static, i.e., it can't be opened for modification anymore
            //  need the lock because it's a bit-field, so read/write must be atomic
            pfcbTemplateTable->Lock();
            pfcbTemplateTable->SetTemplateStatic();
            pfcbTemplateTable->Unlock();

            //  Increment ref count so that template table does not get purged
            pfcbTemplateTable->IncrementRefCount();

            //  Close cursor.
            CallS( ErrFILECloseTable( ppib, pfucbTemplateTable ) );

            pfcb->Lock();
            pfcb->SetDerivedTable();
            pfcb->Unlock();

            //  re-obtain latch on catalog
            Call( ErrBTGet( pfucbCatalog ) );

#ifdef DEBUG
            //  verify still on same record
            Assert( fidMSO_ObjidTable.FFixed() );
            CallS( ErrRECIRetrieveFixedColumn(
                        pfcbNil,
                        pfucbCatalog->u.pfcb->Ptdb(),
                        fidMSO_ObjidTable,
                        pfucbCatalog->kdfCurr.data,
                        &dataField ) );
            Assert( dataField.Cb() == sizeof(OBJID) );
            Assert( objidTable == *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) );

            Assert( fidMSO_Type.FFixed() );
            CallS( ErrRECIRetrieveFixedColumn(
                        pfcbNil,
                        pfucbCatalog->u.pfcb->Ptdb(),
                        fidMSO_Type,
                        pfucbCatalog->kdfCurr.data,
                        &dataField ) );
            Assert( dataField.Cb() == sizeof(SYSOBJ) );
            Assert( sysobjTable == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );
#endif
        }
    }

    Call( ErrCATIRetrieveSpaceHints(
                    pfucbCatalog,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    pfucbCatalog->kdfCurr.data,
                    fFalse,
                    pfcb->FDerivedTable() ? &jsphTemplate : NULL,
                    &jsphPrimary ) );
    CallS( err );   // no warnings in this case ...

    memset( &jsphPrimaryDeferredLV, 0, sizeof(jsphPrimaryDeferredLV) );

    Call( ErrCATIRetrieveSpaceHints(
                    pfucbCatalog,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    pfucbCatalog->kdfCurr.data,
                    fTrue,
                    NULL,
                    &jsphPrimaryDeferredLV ) );
    CallSx( err, JET_wrnColumnNull );
    fSetDeferredLVSpacehints = ( err != JET_wrnColumnNull );

    LONG cbLVChunkMost = 0;
    Assert( fidMSO_LVChunkMax.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_LVChunkMax,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    if ( dataField.Cb() == sizeof(ULONG) )
    {
        cbLVChunkMost = *(UnalignedLittleEndian<ULONG> *)dataField.Pv();
    }
    else
    {
        Assert( err == JET_wrnColumnNull );
        Assert( dataField.Cb() == 0 );
        cbLVChunkMost = (LONG)UlParam( JET_paramLVChunkSizeMost );
    }

    //  must release page latch at this point for highest columnid
    //  search in ErrCATIInitTDB()
    //  latch will be reobtained in ErrCATIBuildFIELDArray()
    //  and will not be relinquished from that point forth
    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( Pcsr( pfucbCatalog )->FLatched() );
    Call( ErrBTRelease( pfucbCatalog ) );

    //  verify key no longer in prepared state.
    Assert( !FKSPrepared( pfucbCatalog ) );

    //  force index range on this table
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&objidTable,
                sizeof(objidTable),
                JET_bitNewKey | JET_bitFullColumnEndLimit ) );
    Call( ErrIsamSetIndexRange( ppib, pfucbCatalog, JET_bitRangeInclusive | JET_bitRangeUpperLimit ) );

    Call( ErrCATIInitTDB(
                ppib,
                pfucbCatalog,
                objidTable,
                szTableName,
                pfcb->FTemplateTable(),
                pfcbTemplateTable,
                fSetDeferredLVSpacehints ? &jsphPrimaryDeferredLV :  NULL,
                &ptdb ) );
    CallSx( err, wrnCATNoMoreRecords );
    fHitEOF = ( wrnCATNoMoreRecords == err );

    Assert( ptdbNil != ptdb );
    Assert( ( pfcb->FTemplateTable() && ptdb->FTemplateTable() )
            || ( !pfcb->FTemplateTable() && !ptdb->FTemplateTable() ) );

    //  even if we got NoCurrentRecord, we are actually
    //  still left on the last node (usually, the DIR
    //  level corrects the currency)
    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    if ( cbSeparateLV )
    {
        ptdb->SetPreferredIntrinsicLV( cbSeparateLV );
    }

    // We do not allow arbitrary large chunk size right now
    Assert( cbLVChunkMost <= (LONG)UlParam( JET_paramLVChunkSizeMost ) );
    ptdb->SetLVChunkMost( cbLVChunkMost );

    if ( fHitEOF )
    {
        //  no primary/secondary indexes, only a sequential index
        Call( ErrFILEIInitializeFCB(
                    ppib,
                    ifmp,
                    ptdb,
                    pfcb,
                    pidbNil,
                    fTrue,
                    pfcb->PgnoFDP(),
                    &jsphPrimary,   // sequential space hints.
                    NULL ) );
        ptdb->ResetRgbitAllIndex();
        Assert( pfcbNil == pfcb->PfcbNextIndex() );
    }
    else
    {
        Call( ErrCATIInitIndexFCBs(
                    ppib,
                    pfucbCatalog,
                    objidTable,
                    pfcb,
                    ptdb,
                    &jsphPrimary ) );
        CallSx( err, wrnCATNoMoreRecords );
        fHitEOF = ( wrnCATNoMoreRecords == err );
    }

    //  even if we got NoCurrentRecord, we are actually
    //  still left on the last node (usually, the DIR
    //  level corrects the currency)
    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    if ( !BoolParam( pinst, JET_paramDisableCallbacks ) && !fHitEOF )
    {
        Call( ErrCATIInitCallbacks(
                    ppib,
                    pfucbCatalog,
                    objidTable,
                    pfcb,
                    ptdb ) );
        CallSx( err, wrnCATNoMoreRecords );
    }

    //  even if we got NoCurrentRecord, we are actually
    //  still left on the last node (usually, the DIR
    //  level corrects the currency)
    Assert( locOnCurBM == pfucbCatalog->locLogical );
    Assert( Pcsr( pfucbCatalog )->FLatched() );
    BTUp( pfucbCatalog );

    // Try to compact byte pool, but if it fails, don't worry.  It just means
    // that the byte pool will have some unused space.
    ptdb->MemPool().FCompact();

    pfcb->SetTypeTable();

    err = JET_errSuccess;

HandleError:
    if ( err < 0 )
    {
        Assert( pfcb->Ptdb() == ptdbNil || pfcb->Ptdb() == ptdb );

        delete ptdb;
        pfcb->SetPtdb( ptdbNil );

        if ( pfcbNil != pfcb->PfcbNextIndex() )
        {
            CATIFreeSecondaryIndexes( pfcb->PfcbNextIndex() );
            pfcb->SetPfcbNextIndex( pfcbNil );
        }

        if ( pidbNil != pfcb->Pidb() )
        {
            pfcb->ReleasePidb();
            pfcb->SetPidb( pidbNil );
        }
    }

    if ( pfcbTemplateTable != pfcbNil )
    {
        pfcbTemplateTable->Release();
        pfcbTemplateTable = pfcbNil;
    }

    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}



ERR ErrCATInitTempFCB( FUCB *pfucbTable )
{
    ERR     err;
    PIB     *ppib = pfucbTable->ppib;
    FCB     *pfcb = pfucbTable->u.pfcb;
    TDB     *ptdb = ptdbNil;
    TCIB    tcib;
    INST    *pinst = PinstFromPpib( ppib );

    Assert( !pfcb->FInitialized() || pfcb->FInitedForRecovery() );

    /*  the only way a temporary table could get to this point is if it was being
    /*  created, in which case there are no primary or secondary indexes yet.
    /**/

    CallR( ErrTDBCreate( pinst, pfucbTable->ifmp, &ptdb, &tcib ) );

    ptdb->SetLVChunkMost( (LONG)UlParam( JET_paramLVChunkSizeMost ) );

    Assert( ptdb->FidVersion() == 0 );
    Assert( ptdb->FidAutoincrement() == 0 );
    Assert( tcib.fidFixedLast == ptdb->FidFixedLast() );
    Assert( tcib.fidVarLast == ptdb->FidVarLast() );
    Assert( tcib.fidTaggedLast == ptdb->FidTaggedLast() );

    /*  for temporary tables, could only get here from
    /*  create table which means table should currently be empty
    /**/
    Assert( ptdb->FidFixedLast() == fidFixedLeast - 1 );
    Assert( ptdb->FidVarLast() == fidVarLeast - 1 );
    Assert( ptdb->FidTaggedLast() == fidTaggedLeast - 1 );
    Assert( ptdb->DbkMost() == 0 );
    Assert( ptdb->Ui64LidLast() == 0 );

    // NOTE: Table name will be added when sort is materialized.

    Call( ErrFILEIInitializeFCB(
        ppib,
        pinst->m_mpdbidifmp[ dbidTemp ],
        ptdb,
        pfcb,
        pidbNil,
        fTrue,
        pfcb->PgnoFDP(),
        PSystemSpaceHints(eJSPHDefaultUserIndex),
        NULL ) );

    Assert( pfcb->PfcbNextIndex() == pfcbNil );

    pfcb->SetPtdb( ptdb );

    FILESetAllIndexMask( pfcb );
    pfcb->Lock();
    pfcb->SetFixedDDL();
    pfcb->SetTypeTemporaryTable();
    pfcb->Unlock();

    return JET_errSuccess;

HandleError:
    Assert( pfcb->Ptdb() == ptdbNil );      // Verify TDB never got linked in.
    delete ptdb;

    return err;
}


//  UNDONE: is there another function in JET which does this?
ULONG UlCATColumnSize( JET_COLTYP coltyp, INT cbMax, BOOL *pfMaxTruncated )
{
    ULONG   ulLength;
    BOOL    fTruncated = fFalse;

    switch( coltyp )
    {
        case JET_coltypBit:
        case JET_coltypUnsignedByte:
            ulLength = 1;
            Assert( ulLength == sizeof(BYTE) );
            break;

        case JET_coltypShort:
        case JET_coltypUnsignedShort:
            ulLength = 2;
            Assert( ulLength == sizeof(SHORT) );
            break;

        case JET_coltypLong:
        case JET_coltypUnsignedLong:
        case JET_coltypIEEESingle:
            ulLength = 4;
            Assert( ulLength == sizeof(LONG) );
            break;

        case JET_coltypLongLong:
        case JET_coltypCurrency:
        case JET_coltypIEEEDouble:
        case JET_coltypDateTime:
        case JET_coltypUnsignedLongLong:
            ulLength = 8;
            break;

        case JET_coltypGUID:
            ulLength = 16;
            break;

        case JET_coltypBinary:
        case JET_coltypText:
            if ( cbMax == 0 )
            {
                ulLength = JET_cbColumnMost;
            }
            else
            {
                ulLength = cbMax;
                if ( ulLength > JET_cbColumnMost )
                {
                    ulLength = JET_cbColumnMost;
                    fTruncated = fTrue;
                }
            }
            break;

        default:
            // Just pass back what was given.
            Assert( FRECLongValue( coltyp ) );
            Assert( JET_coltypNil != coltyp );
            ulLength = cbMax;
            break;
    }

    if ( pfMaxTruncated != NULL )
    {
        *pfMaxTruncated = fTruncated;
    }

    return ulLength;
}


/*  This routines sets/gets table and index stats.
/*  Pass NULL for szSecondaryIndexName if looking for sequential or primary index.
/**/
ERR ErrCATStats(
    PIB         *ppib,
    const IFMP  ifmp,
    const OBJID objidTable,
    const CHAR  *szSecondaryIndexName,
    SR          *psr,
    const BOOL  fWrite )

{
    ERR         err;
    FUCB        *pfucbCatalog       = pfucbNil;

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    /*  stats for sequential and primary indexes are in MSysObjects, while
    /*  stats for secondary indexes are in MSysIndexes.
    /**/
    if ( szSecondaryIndexName )
    {
        Call( ErrCATISeekTableObject(
                    ppib,
                    pfucbCatalog,
                    objidTable,
                    sysobjIndex,
                    szSecondaryIndexName ) );
    }
    else
    {
        Call( ErrCATISeekTable(
                    ppib,
                    pfucbCatalog,
                    objidTable ) );
    }

    Assert( Pcsr( pfucbCatalog )->FLatched() );

    /*  set/retrieve value
    /**/
    if ( fWrite )
    {
        LE_SR le_sr;

        le_sr = *psr;

        Call( ErrDIRRelease( pfucbCatalog ) );

        Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );
        Call( ErrIsamSetColumn(
                    ppib,
                    pfucbCatalog,
                    fidMSO_Stats,
                    (BYTE *)&le_sr,
                    sizeof(LE_SR),
                    NO_GRBIT,
                    NULL ) );
        Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );
    }
    else
    {
        DATA    dataField;

        Assert( fidMSO_Stats.FVar() );
        Call( ErrRECIRetrieveVarColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Stats,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        if ( dataField.Cb() == 0 )
        {
            Assert( JET_wrnColumnNull == err );
            memset( (BYTE *)psr, '\0', sizeof(SR) );
            err = JET_errSuccess;
        }
        else
        {
            Assert( dataField.Cb() == sizeof(LE_SR) );
            CallS( err );
            LE_SR le_sr;
            le_sr = *(LE_SR*)dataField.Pv();
            *psr = le_sr;
//          UtilMemCpy( psr, dataField.Pv(), sizeof(SR) );
        }
    }

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    return err;
}


//  ================================================================
ERR ErrCATChangePgnoFDP( PIB * ppib, IFMP ifmp, OBJID objidTable, OBJID objid, SYSOBJ sysobj, PGNO pgnoFDPNew )
//  ================================================================
{
    ERR         err             = JET_errSuccess;
    FUCB *      pfucbCatalog    = pfucbNil;
    BOOKMARK    bm;
    BYTE        *pbBookmark     = NULL;
    ULONG       cbBookmark;

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );
    Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobj, objid ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Call( ErrDIRRelease( pfucbCatalog ) );

    Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
    Assert( pfucbCatalog->bmCurr.data.FNull() );
    Alloc( pbBookmark = (BYTE *)RESBOOKMARK.PvRESAlloc() );
    cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), cbBookmarkAlloc );
    Assert( cbBookmark <= cbBookmarkMost );
    pfucbCatalog->bmCurr.key.CopyIntoBuffer( pbBookmark, cbBookmark );

    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( pbBookmark );
    bm.key.suffix.SetCb( cbBookmark );
    bm.data.Nullify();

    Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );
    Call( ErrIsamSetColumn(
                ppib,
                pfucbCatalog,
                fidMSO_PgnoFDP,
                (BYTE *)&pgnoFDPNew,
                sizeof(PGNO),
                NO_GRBIT,
                NULL ) );
    Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );

    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );     //  shadow catalog

    Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );

    Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );
    Call( ErrIsamSetColumn(
                ppib,
                pfucbCatalog,
                fidMSO_PgnoFDP,
                (BYTE *)&pgnoFDPNew,
                sizeof(PGNO),
                NO_GRBIT,
                NULL ) );
    Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );

HandleError:
    RESBOOKMARK.Free( pbBookmark );

    if( pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }
    return err;
}


//  ================================================================
LOCAL ERR ErrCATIChangeColumnDefaultValue(
    PIB         * const ppib,
    FUCB        * const pfucbCatalog,
    const DATA& dataDefault )
//  ================================================================
{
    ERR         err;
    DATA        dataField;
    FIELDFLAG   ffield      = 0;
    ULONG       ulFlags;

    CallR( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );

    // get the exsiting flags
    Assert( fidMSO_Flags.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Flags,
                pfucbCatalog->dataWorkBuf,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );
    ffield = *(UnalignedLittleEndian< FIELDFLAG > *)dataField.Pv();

    //  force DefaultValue flag in case it's not already set
    FIELDSetDefault( ffield );

    ulFlags = ffield;
    Call( ErrIsamSetColumn(
                ppib,
                pfucbCatalog,
                fidMSO_Flags,
                &ulFlags,
                sizeof(ulFlags),
                NO_GRBIT,
                NULL ) );

    Call( ErrIsamSetColumn(
                ppib,
                pfucbCatalog,
                fidMSO_DefaultValue,
                dataDefault.Pv(),
                dataDefault.Cb(),
                NO_GRBIT,
                NULL ) );
    Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );

HandleError:
    if( err < 0 )
    {
        CallS( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepCancel ) );
    }
    return err;
}


//  ================================================================
ERR ErrCATChangeColumnDefaultValue(
    PIB         *ppib,
    const IFMP  ifmp,
    const OBJID objidTable,
    const CHAR  *szColumn,
    const DATA& dataDefault )
//  ================================================================
{
    ERR         err;
    FUCB *      pfucbCatalog        = pfucbNil;
    BOOKMARK    bm;
    BYTE        *pbBookmark         = NULL;
    ULONG       cbBookmark;

    CallR( ErrDIRBeginTransaction( ppib, 55013, NO_GRBIT ) );

    Alloc( pbBookmark = (BYTE *)RESBOOKMARK.PvRESAlloc() );

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );    //  catalog
    Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjColumn, szColumn ) );

    Call( ErrDIRRelease( pfucbCatalog ) );

    Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
    Assert( pfucbCatalog->bmCurr.data.FNull() );
    Assert( pfucbCatalog->bmCurr.key.Cb() <= cbBookmarkAlloc );
    cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), cbBookmarkAlloc );
    pfucbCatalog->bmCurr.key.CopyIntoBuffer( pbBookmark, cbBookmark );

    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( pbBookmark );
    bm.key.suffix.SetCb( cbBookmark );
    bm.data.Nullify();

    Call( ErrCATIChangeColumnDefaultValue( ppib, pfucbCatalog, dataDefault ) );
    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );     //  shadow catalog
    Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );
    Call( ErrCATIChangeColumnDefaultValue( ppib, pfucbCatalog, dataDefault ) );
    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
    if( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }

    RESBOOKMARK.Free( pbBookmark );

    if( err < 0 )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    return err;
}

LOCAL ERR ErrCATIUpgradeLocaleForOneIndex(
    _In_ PIB * const ppib,
    _In_ IDB * const pidb,
    _In_ const OBJID objidTable,
    _In_ const OBJID objidIndex,
    _In_ FUCB * const pfucbCatalog,
    _In_ const BOOL fShadow
    )
{
    ERR err = JET_errSuccess;
    SORTID  sortidCatalog;
    SORTID  sortidCurrentOS;
    BOOL fHasSortID = fFalse;
    BOOL fInUpdate = fFalse;
    BOOL fNeedToRelease = fFalse;
    QWORD qwSortVersionCurrentOS;
#ifdef DEBUG
    QWORD qwSortVersionCatalog = 0;
#endif
    DATA dataField;

    Assert( pidb->FLocalizedText() );

    // Need to be on primary index:
    Assert( pfucbCatalog->pfucbCurIndex == pfucbNil );

    Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjIndex, objidIndex ) );
    fNeedToRelease = fTrue;

#ifdef DEBUG
{
    // This should only be modified if there are no entries in the index.
    // If there are multiple entries in the index, then it should have been
    // deleted by the calling function!
    CSR     csrIndexRootPage;
    ULONG   centries;
    PGNO    pgnoFDPIndex;
    WCHAR wszLocaleNameT[NORM_LOCALE_NAME_MAX_LENGTH];

    Assert( fidMSO_PgnoFDP.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_PgnoFDP,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(PGNO) );

    pgnoFDPIndex = *(UnalignedLittleEndian< PGNO > *) dataField.Pv();

    Call( csrIndexRootPage.ErrGetPage(
        ppib,
        pfucbCatalog->u.pfcb->Ifmp(),
        pgnoFDPIndex,
        latchReadTouch ) );
    centries = csrIndexRootPage.Cpage().Clines();
    csrIndexRootPage.ReleasePage();

    AssertSz( 0 == centries, "This index seems to have %d entries.\n", centries );

    Call( ErrRECIRetrieveTaggedColumn(
        pfucbCatalog->u.pfcb,
        fidMSO_LocaleName,
        1,
        pfucbCatalog->kdfCurr.data,
        &dataField,
        JET_bitNil
        ) );

    if ( JET_wrnColumnNull == err )
    {
        // Unfortunately, there aren't many guarantees we can tell at his point:
        // -It might be so old it only has an LCID.
        LCID lcid;
        WCHAR wszLocaleNameFromLcid[ NORM_LOCALE_NAME_MAX_LENGTH ];

        Assert( fidMSO_Localization.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
            pfcbNil,
            pfucbCatalog->u.pfcb->Ptdb(),
            fidMSO_Localization,
            pfucbCatalog->kdfCurr.data,
            &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(LCID) );
        lcid = *(UnalignedLittleEndian<LCID> *) dataField.Pv();

        Call( ErrNORMLcidToLocale( lcid, wszLocaleNameFromLcid, _countof( wszLocaleNameFromLcid ) ) );

        if (0 == lcid)
        {
            Assert( ( NULL != pidb->WszLocaleName() ) && ( 0 == wcslen( pidb->WszLocaleName() ) ) );
        }
        else
        {
            Assert( 0 == wcscmp( pidb->WszLocaleName(), wszLocaleNameFromLcid ) );
        }
    }
    else
    {
        const INT cbDataField = dataField.Cb();

        if ( cbDataField < 0 || cbDataField > NORM_LOCALE_NAME_MAX_LENGTH * sizeof(WCHAR) )
        {
            AssertSz( fFalse, "Invalid fidMSO_Localization column in catalog." );

            // Should this emit a failure tag?
            // OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"570400a0-d61a-43f6-b576-9e6c00a31c18" );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }
        Assert( sizeof( wszLocaleNameT ) > cbDataField );
        UtilMemCpy( wszLocaleNameT, dataField.Pv(), cbDataField );
        wszLocaleNameT[min( cbDataField / sizeof(WCHAR), _countof(wszLocaleNameT) - 1 )] = 0;

        Assert( 0 == wcscmp( pidb->WszLocaleName(), wszLocaleNameT ) );
    }

    // The Sort Version should be the same as in the IDB.
    Assert( fidMSO_Version.FVar() );
    Call( ErrRECIRetrieveVarColumn(
        pfcbNil,
        pfucbCatalog->u.pfcb->Ptdb(),
        fidMSO_Version,
        pfucbCatalog->kdfCurr.data,
        &dataField ) );

    if ( JET_wrnColumnNull != err )
    {
        CallS( err );   // no other warnings expected
        if ( dataField.Cb() != sizeof( qwSortVersionCatalog ) )
        {
            AssertSz( fFalse, "Invalid fidMSO_Version column in catalog." );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }

        qwSortVersionCatalog = *(UnalignedLittleEndian< QWORD > *) dataField.Pv();
    }
    else
    {
        Expected( dataField.Cb() == 0 );
    }

    // The IDB should not be updated to the current sort version yet. That happens
    // later in the function.
    Expected( qwSortVersionCatalog == pidb->QwSortVersion() );

}
#endif

    Assert( fidMSO_SortID.FVar() );
    Call( ErrRECIRetrieveVarColumn(
        pfcbNil,
        pfucbCatalog->u.pfcb->Ptdb(),
        fidMSO_SortID,
        pfucbCatalog->kdfCurr.data,
        &dataField ) );

    if ( JET_wrnColumnNull != err )
    {
        CallS( err );   // no other warnings expected
        if ( dataField.Cb() != sizeof( SORTID ) )
        {
            AssertSz( fFalse, "Invalid fidMSO_SortID column in catalog." );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }

        sortidCatalog = *(UnalignedLittleEndian< SORTID > *) dataField.Pv();

        fHasSortID = fTrue;
    }
    else
    {
        Expected( dataField.Cb() == 0 );
    }

    // We're done with retrieving from the record. The next operation will be from the Isam layer.
    Call( ErrDIRRelease( pfucbCatalog ) );
    fNeedToRelease = fFalse;

    Call( ErrNORMGetSortVersion( pidb->WszLocaleName(), &qwSortVersionCurrentOS, &sortidCurrentOS ) );

    // The IDB has still not been updated at this point. That comes later in the function.
    Expected( ( fHasSortID && !FSortIDEquals( &sortidCurrentOS, &sortidCatalog ) )
         || qwSortVersionCurrentOS != pidb->QwSortVersion() );

{
    LittleEndian<QWORD> qwSortVersionToStore( qwSortVersionCurrentOS );

    Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplace ) );
    fInUpdate = fTrue;

    // There may or may not be a locale name. Store it just in case! Also stamp the new Version and Sort ID (GUID).
    Call( ErrIsamSetColumn( ppib, pfucbCatalog, fidMSO_LocaleName, pidb->WszLocaleName(), wcslen( pidb->WszLocaleName() ) * sizeof( *pidb->WszLocaleName() ), JET_bitNil, NULL ) );
    Call( ErrIsamSetColumn( ppib, pfucbCatalog, fidMSO_Version, &qwSortVersionToStore, sizeof( qwSortVersionToStore ), NO_GRBIT, NULL ) );
    Call( ErrIsamSetColumn( ppib, pfucbCatalog, fidMSO_SortID, &sortidCurrentOS, sizeof( sortidCurrentOS ), NO_GRBIT, NULL ) );

    if ( !fShadow )
    {
        Call( ErrCATIAddLocale( ppib, pfucbCatalog->ifmp, pidb->WszLocaleName(), sortidCurrentOS, qwSortVersionCurrentOS ) );
    }

    Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, JET_bitNil ) );
    fInUpdate = fFalse;
}

    // And need to update the pidb, but only when finally updating the Shadow Catalog.
    Assert( pidb->FLocalizedText() );

    if ( fShadow )
    {
        pidb->SetQwSortVersion( qwSortVersionCurrentOS );
        pidb->SetSortidCustomSortVersion( &sortidCurrentOS );
        pidb->ResetFBadSortVersion();
        pidb->ResetFOutOfDateSortVersion();
    }

HandleError:
    if ( fNeedToRelease )
    {
        (void) ErrDIRRelease( pfucbCatalog );
    }
    if ( fInUpdate )
    {
        (void)ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepCancel );
    }
    return err;
}

LOCAL ERR ErrCATIUpgradeLocaleForTableIndex(
    _In_ PIB        * const ppib,
    _In_ FMP        * const pfmp,
    _In_z_ const CHAR * const szTableName,
    _In_ IDB    * const pidb,
    _In_z_ const CHAR * const szIndexName
    )
{
    ERR err;
    BOOL fInTrx = fFalse;
    FUCB* pfucbCatalog = pfucbNil;
    OBJID objidTable = objidNil;
    OBJID objidTableShadow = objidNil;
    OBJID objidIndex = objidNil;
    DATA dataField;

    Call( ErrDIRBeginTransaction( ppib, 0, JET_bitNil ) );
    fInTrx = fTrue;

    // First do everything with the Catalog.
    Call( ErrCATOpen( ppib, pfmp->Ifmp(), &pfucbCatalog, fFalse ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATSeekTable( ppib, pfmp->Ifmp(), szTableName, NULL, &objidTable ) );

    // The shadow catalog doesn't have the right indices to look up the Index by Name,
    // so let's grab it from the Real catalog
    Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjIndex, szIndexName ) );

    Call( ErrRECIRetrieveFixedColumn(
        pfcbNil,
        pfucbCatalog->u.pfcb->Ptdb(),
        fidMSO_Id,
        pfucbCatalog->kdfCurr.data,
        &dataField ) );
    Assert( dataField.Cb() == sizeof(OBJID) );
    UtilMemCpy( &objidIndex, dataField.Pv(), sizeof(OBJID) );

    Call( ErrDIRRelease( pfucbCatalog ) );

    Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, NULL ) );
    Call( ErrCATIUpgradeLocaleForOneIndex( ppib, pidb, objidTable, objidIndex, pfucbCatalog, fFalse ) );
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    // Now repeat it with the Shadow Catalog.
    Call( ErrCATOpen( ppib, pfmp->Ifmp(), &pfucbCatalog, fTrue ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATSeekTable( ppib, pfmp->Ifmp(), szTableName, NULL, &objidTableShadow ) );
    Assert( objidTable == objidTableShadow );

    Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, NULL ) );
    Call( ErrCATIUpgradeLocaleForOneIndex( ppib, pidb, objidTableShadow, objidIndex, pfucbCatalog, fTrue ) );
    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    if ( fInTrx )
    {
        Call( ErrDIRCommitTransaction( ppib, JET_bitNil, 0, NULL ) );
        fInTrx = fFalse;
    }

HandleError:
    if ( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }

    if ( fInTrx )
    {
        (void) ErrDIRRollback( ppib, JET_bitNil );
    }

    return err;
}

//  ================================================================
LOCAL ERR ErrCATIDeleteOrUpdateLocalizedIndexesInTable(
    _In_ PIB        * const ppib,
    _In_ const IFMP ifmp,
    _In_ FCB        * const pfcbTable,
    _In_ FUCB       * const pfucbTable,
    _In_z_ const CHAR   * const szTableName,
    _In_ const CATCheckIndicesFlags catcifFlags,
    _Out_ BOOL      * const pfIndexesUpdated,
    _Out_ BOOL      * const pfIndexesDeleted )
//  ================================================================
//
// Delete localized index, the basic logic is:
// => (Force deletion mode)
//      Delete the localized index unconditionally.
// => (Read only mode)
//      Return either JET_errPrimaryIndexCorrupted or JET_errSecondaryIndexCorrupted
//      For out of date unicode index.
// => (Otherwise)
// =>   Returns JET_errPrimaryIndexCorrupted or deletes the secondary index
//      for out of date unicode index.
//
// For any mode, if an index deleted, *pfIndexesDeleted will be set to fTrue.
//
//-
{
    ERR         err;
    FCB         *pfcbIndex;
    CHAR        szIndexName[JET_cbNameMost+1];
    const WCHAR *rgsz[3];
    BOOL        fInTrx          = fFalse;
    const BOOL fReadOnly = !!( catcifFlags & catcifReadOnly );
    const BOOL fUpdateEmptyIndex = !!( catcifFlags & catcifUpdateEmptyIndices );
    const BOOL fDeleteOutOfDateSecondaryIndex = !!( catcifFlags & catcifDeleteOutOfDateSecondaryIndices );
    const BOOL fForceDelete = !!(catcifFlags & catcifForceDeleteIndices );
    const BOOL fAllowOutOfDateSecondaryIndicesToBeOpened = !!(catcifFlags & catcifReportOnlyOutOfDateSecondaryIndices );
    const BOOL fAllowValidOutOfDateIndices = !!( catcifFlags & catcifAllowValidOutOfDateVersions );

    *pfIndexesUpdated = fFalse;
    *pfIndexesDeleted = fFalse;

    if ( !fReadOnly )
    {
        Assert( pfucbNil != pfucbTable );
    }
    Assert( pfcbNil != pfcbTable );
    Assert( objidNil != pfcbTable->ObjidFDP() );
    Assert( pfcbTable->FTypeTable() );
    Assert( ptdbNil != pfcbTable->Ptdb() );

    AssertSz( fReadOnly || !fAllowOutOfDateSecondaryIndicesToBeOpened,
              "Specifying fAllowOutOfDateSecondaryIndicesToBeOpened implies fReadOnly." );

    if ( pfcbTable->ObjidFDP() == PfmpFromIfmp( ifmp )->ObjidExtentPageCountCacheFDP() )
    {
        // The ExtentPageCountCache table has no localized indices.  However, the normal code path
        // that would discover this triggers Asserts at the very least.  Just short
        // circuit out of this.
        Assert( FCATExtentPageCountCacheTable( szTableName ) );
        Assert( objidNil != PfmpFromIfmp( ifmp )->ObjidExtentPageCountCacheFDP() );
        err = JET_errSuccess;
        goto HandleError;
    }

    //  If we already verified opening this table, then bail out early.
    if ( pfcbTable->FValidatedCurrentLocales() ||
         fAllowValidOutOfDateIndices && pfcbTable->FValidatedValidLocales() )
    {
        err = JET_errSuccess;
        goto HandleError;
    }

    //  Wrap in a transaction to ensure one index doesn't disappear
    //  (due to RCE cleanup) while we're working on another.
    //  It may be called by any OpenTable call, so we have no guarantee
    //  over what transaction level we're at.
    Call( ErrDIRBeginTransaction( ppib, 42725, NO_GRBIT ) );
    fInTrx = fTrue;

    //  This used to be only called at AttachDatabase() time,
    //  but this is no longer the case in E15 with the
    //  deferring-to-table-open checking.
    // ENTERCRITICALSECTION enterCritFCB( &pfcbTable->Crit() );

    for ( pfcbIndex = pfcbTable;
        pfcbIndex != pfcbNil;
        pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        const IDB   *pidb   = pfcbIndex->Pidb();

        //  only clustered index may be sequential
        //
        Assert( pidbNil != pidb || pfcbIndex == pfcbTable );

        if ( pidbNil != pidb )
        {
            OSTraceFMP(
                ifmp,
                JET_tracetagCatalog,
                OSFormat( __FUNCTION__ ": Examining %s:%s, LocalizedText = %d, FBadSortVersion = %d. fForceDelete = %d.\n",
                          szTableName,
                          pfcbTable->Ptdb()->SzIndexName( pidb->ItagIndexName(), pfcbIndex->FDerivedIndex() ),
                          pidb->FLocalizedText(),
                          pidb->FBadSortVersion(),
                          fForceDelete ) );
        }

        //  we need to rebuild the index if the sort version has changed
        //
        if ( pidbNil != pidb
            && pidb->FLocalizedText()
            && ( pidb->FBadSortVersion() || fForceDelete || pidb->FOutOfDateSortVersion() && !fAllowValidOutOfDateIndices ) )
        {
            CSR     csrIndexRootPage;
            ULONG   centries;

            PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
            tcScope->nParentObjectClass = pfcbIndex->Tableclass();
            tcScope->SetDwEngineObjid( pfcbIndex->ObjidFDP() );

#ifdef DEBUG
            BOOL                fFoundUnicodeTextColumn = fFalse;
            const IDXSEG*       rgidxseg                = PidxsegIDBGetIdxSeg( pidb, pfcbTable->Ptdb() );
            Assert( pidb->Cidxseg() <= JET_ccolKeyMost );

            for ( ULONG iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
            {
                const COLUMNID  columnid    = rgidxseg[iidxseg].Columnid();
                const FIELD     *pfield     = pfcbTable->Ptdb()->Pfield( columnid );
                if ( FRECTextColumn( pfield->coltyp ) && usUniCodePage == pfield->cp )
                {
                    fFoundUnicodeTextColumn = fTrue;
                    break;
                }
            }
            Assert( fFoundUnicodeTextColumn );
#endif

            Assert( strlen( pfcbTable->Ptdb()->SzIndexName( pidb->ItagIndexName(), pfcbIndex->FDerivedIndex() ) ) <= JET_cbNameMost );
            OSStrCbCopyA( szIndexName, sizeof(szIndexName), pfcbTable->Ptdb()->SzIndexName( pidb->ItagIndexName(), pfcbIndex->FDerivedIndex() ) );

            Call( csrIndexRootPage.ErrGetPage(
                                        ppib,
                                        pfcbIndex->Ifmp(),
                                        pfcbIndex->PgnoFDP(),
                                        latchReadTouch ) );
            centries = csrIndexRootPage.Cpage().Clines();
            csrIndexRootPage.ReleasePage();

            if ( fForceDelete )
            {
                *pfIndexesDeleted = fTrue;

                //  Ensure that we can always delete the index
                //  This means that for the life of this FCB, DDL will be
                //  possible and there will be a perf hit because we
                //  will now enter critFCB for many operations.
                //  Note that we cannot simply re-enable the flag after
                //  the deletion because version cleanup does some stuff
                //  with the FCB.
                pfcbTable->Lock();
                pfcbTable->ResetFixedDDL();
                pfcbTable->Unlock();

                err = ErrIsamDeleteIndex( ppib, pfucbTable, szIndexName );
                OSTraceFMP( ifmp, JET_tracetagCatalog,
                            OSFormat( "%d: ErrIsamDeleteIndex(%s) returned %d.\n", __LINE__, szIndexName, err ) );
            }
            else
            {
                CAutoWSZDDL     lszIndexName;
                CAutoWSZDDL     lszTableName;

                rgsz[0] = g_rgfmp[ifmp].WszDatabaseName();

                // should not fail as the allocation is made
                // and the name to convert is checked
                CallS( lszIndexName.ErrSet( szIndexName ) );
                CallS( lszTableName.ErrSet( szTableName ) );
                rgsz[1] = (WCHAR*)lszIndexName;
                rgsz[2] = (WCHAR*)lszTableName;

                // -------------------------------------------------------------
                // No entries in index
                //   This covers the template index case as the template table
                //   does not have records
                // -------------------------------------------------------------
                if ( 0 == centries )
                {
                    if ( fReadOnly || !fUpdateEmptyIndex )
                    {
                        OSTraceFMP( ifmp, JET_tracetagCatalog, OSFormat( "No entries in index (%s:%s)! Would fix up the catalog instead, but fReadOnly=%d.\n", szTableName, szIndexName, fReadOnly ) );
                        err = JET_errSuccess;
                    }
                    else
                    {
                        // Allow the IDB to be modified/upgraded, including template table, which is safe now
                        // since we have separate IDBs for template index and derived index.
                        IDB * const pidbMutable = const_cast<IDB* const>( pidb );

                        OSTraceFMP( ifmp, JET_tracetagCatalog, OSFormat( "No entries in index (%s:%s)! Will fix up the catalog instead. fReadOnly=%d.\n", szTableName, szIndexName, fReadOnly ) );
                        err = ErrCATIUpgradeLocaleForTableIndex( ppib, &g_rgfmp[ifmp], szTableName, pidbMutable, szIndexName );

                        // Force the MSysLocales table to be rebuilt. It's still cheaper than rebuilding
                        // an entire index.
                        *pfIndexesUpdated = fTrue;
                    }
                }

                // -------------------------------------------------------------
                // non-empty index could be corrupt if it contains a unicode text column
                // -------------------------------------------------------------
                else if ( pfcbIndex == pfcbTable )
                {
                    // Oops, primary index is localized text index and out of date.
                    // Report and done.
                    UtilReportEvent(
                            eventWarning,
                            DATA_DEFINITION_CATEGORY,
                            PRIMARY_INDEX_OUT_OF_DATE_ERROR_ID, 3, rgsz, 0, NULL, PinstFromPpib( ppib ) );

                    OSUHAPublishEvent(
                        HaDbFailureTagAlertOnly, PinstFromPpib( ppib ), HA_DATA_DEFINITION_CATEGORY,
                        HaDbIoErrorNone, g_rgfmp[ifmp].WszDatabaseName(), 0, 0,
                        HA_PRIMARY_INDEX_OUT_OF_DATE_ERROR_ID, _countof( rgsz ), rgsz );

                    err = ErrERRCheck( JET_errPrimaryIndexCorrupted );
                }
                else if ( fReadOnly || !fDeleteOutOfDateSecondaryIndex )
                {
                    // Secondary index in read only mode, report and done.
                    UtilReportEvent(
                            eventWarning,
                            DATA_DEFINITION_CATEGORY,
                            SECONDARY_INDEX_OUT_OF_DATE_ERROR_ID, 3, rgsz, 0, NULL, PinstFromPpib( ppib ) );

                    OSUHAPublishEvent(
                        HaDbFailureTagAlertOnly, PinstFromPpib( ppib ), HA_DATA_DEFINITION_CATEGORY,
                        HaDbIoErrorNone, g_rgfmp[ifmp].WszDatabaseName(), 0, 0,
                        HA_SECONDARY_INDEX_OUT_OF_DATE_ERROR_ID, _countof( rgsz ), rgsz );

                    err = ErrERRCheck( JET_errSecondaryIndexCorrupted );
                }
                else
                {
                    UtilReportEvent(
                            eventInformation,
                            DATA_DEFINITION_CATEGORY,
                            DO_SECONDARY_INDEX_CLEANUP_ID, 3, rgsz, 0, NULL, PinstFromPpib( ppib ) );
                    *pfIndexesDeleted = fTrue;

                    //  Ensure that we can always delete the index - same reason as above case.
                    pfcbTable->Lock();
                    pfcbTable->ResetFixedDDL();
                    pfcbTable->Unlock();

                    err = ErrIsamDeleteIndex( ppib, pfucbTable, szIndexName );
                        OSTraceFMP( ifmp, JET_tracetagCatalog,
                            OSFormat( "%d: ErrIsamDeleteIndex %s returned %d (centries was %d).\n", __LINE__, szIndexName, err, centries ) );
                }
            }
            Call( err );
        }
    }

    Call( ErrDIRCommitTransaction( ppib, 0 ) );
    fInTrx = fFalse;

    if ( JET_errSuccess == err )
    {
        //  Set the flag in the FCB so that we can avoid superfluous checking
        //  in the future.
        pfcbTable->Lock();
        if ( fAllowValidOutOfDateIndices )
        {
            pfcbTable->SetValidatedValidLocales();
        }
        else
        {
            pfcbTable->SetValidatedCurrentLocales();
        }
        pfcbTable->Unlock();
    }

HandleError:
    if ( fInTrx )
    {
        Assert( err < 0 );
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    return err;
}

//  ================================================================
LOCAL ERR ErrCATIDeleteOrUpdateLocalizedIndexesInTableByName(
    IN PIB          * const ppib,
    IN const IFMP   ifmp,
    IN const CHAR   * const szTableName,
    IN CATCheckIndicesFlags catcifFlags,
    OUT BOOL        * const pfIndexesUpdated,
    OUT BOOL        * const pfIndexesDeleted )
//  ================================================================
// This function is intended to be used at Attach Time, when
// iterating over every table in the database.
{
    ERR         err;
    FUCB        *pfucbTable     = pfucbNil;
    const BOOL fReadOnly = !!( catcifFlags & catcifReadOnly );

    Assert( 0 == ppib->Level() );

    CallR( ErrFILEOpenTable(
                ppib,
                ifmp,
                &pfucbTable,
                szTableName,
                ( fReadOnly ? 0 : JET_bitTableDenyRead | JET_bitTablePermitDDL ) ) );
    Assert( pfucbNil != pfucbTable );

    Call( ErrCATIDeleteOrUpdateLocalizedIndexesInTable(
       ppib,
       ifmp,
       pfucbTable->u.pfcb,
       pfucbTable,
       szTableName,
       catcifFlags,
       pfIndexesUpdated,
       pfIndexesDeleted ) );

HandleError:

    Assert( pfucbNil != pfucbTable );
    CallS( ErrFILECloseTable( ppib, pfucbTable ) );
    return err;
}

//  ================================================================
ERR ErrCATCheckLocalizedIndexesInTable(
    _In_ PIB            * const ppib,
    _In_ const IFMP ifmp,
    _In_ FCB        * const pfcbTable,
    _In_opt_ FUCB   * const pfucbTable,
    _In_z_ const CHAR   *szTableName,
    _In_ CATCheckIndicesFlags   catcifFlags,
    _Out_ BOOL      *pfIndexesUpdated,
    _Out_ BOOL      *pfIndicesDeleted )
//  ================================================================
// This function is called by ErrIsamOpenTable if index-checking
// is deferred to open-table.
//
// Parameters:
// pfcbTable is required.
// pfucbTable is only needed if the out of date indices are to be deleted.
{
    ERR         err;

    Expected( UlParam( PinstFromIfmp( ifmp ), JET_paramEnableIndexChecking ) == JET_IndexCheckingDeferToOpenTable );

    const BOOL fReadOnly = !!( catcifFlags & catcifReadOnly );

    Assert( pfcbNil != pfcbTable );
    if ( !fReadOnly )
    {
        Assert( pfucbNil != pfucbTable );

        // If both the FCB and FUCB are passed in, they MUST be consistent.
        EnforceSz( pfucbTable->u.pfcb == pfcbTable, "InconsistentFcbAndFcbFromFucb" );
    }

    // Remove the force deletion flag
    Expected( 0 == ( catcifFlags & catcifForceDeleteIndices ) );

    Call( ErrCATIDeleteOrUpdateLocalizedIndexesInTable(
       ppib,
       ifmp,
       pfcbTable,
       pfucbTable,
       szTableName,
       catcifFlags,
       pfIndexesUpdated,
       pfIndicesDeleted ) );

HandleError:

    return err;
}

//  ================================================================
ERR ErrCATDeleteOrUpdateOutOfDateLocalizedIndexes(
        IN PIB * const ppib,
        IN const IFMP ifmp,
        IN CATCheckIndicesFlags catcifFlags,
        OUT BOOL * const pfIndexesUpdated,
        OUT BOOL * const pfIndexesDeleted)
//  ================================================================
//
//  Check all the localized indexes in the catalog
//
//  For each index:
//
//  if ( index sort order matches OS sort order )
//      success
//  else if ( fReadOnly )
//      fCorruptedIndexes = TRUE
//  else
//      delete index
//      *pfIndexesDeleted = TRUE
//
//  if ( fCorruptedIndexes )
//      return error
//
//  the real implementation is more complex...
//
//-
{
    ERR     err;
    FUCB    *pfucbCatalog               = pfucbNil;     //  cursor to sequentially scan catalog
    OBJID   objidTable                  = objidNil;
    OBJID   objidTableLastWithLocalized = objidNil;
    SYSOBJ  sysobj;
    DATA    dataField;
    CHAR    szTableName[JET_cbNameMost+1];
    CHAR    szIndexName[JET_cbNameMost+1];
    BOOL    fLatched                    = fFalse;
    WCHAR   wszLocaleName[NORM_LOCALE_NAME_MAX_LENGTH];
    const BOOL fForceDelete = !!( catcifFlags & catcifForceDeleteIndices );
    *pfIndexesDeleted       = fFalse;

    if ( FFMPIsTempDB( ifmp ) )
    {
        AssertSz( fFalse, "Temp DBs don't have MSysLocales or persisted localized indices." );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    //  we will be sequentially scanning the catalog for index records
    FUCBSetSequential( pfucbCatalog );

    err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT );
    Assert( JET_errRecordNotFound != err );
    if ( JET_errNoCurrentRecord == err )
    {
        // catalog should not be empty
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"1e195179-2da2-4fa6-88eb-873afda166ca" );
        err = ErrERRCheck( JET_errCatalogCorrupted );
    }

    do
    {
        Call( err );

        Assert( !fLatched );
        Assert( !Pcsr( pfucbCatalog )->FLatched() );
        Call( ErrDIRGet( pfucbCatalog ) );
        fLatched = fTrue;

        Assert( fidMSO_ObjidTable.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
            pfcbNil,
            pfucbCatalog->u.pfcb->Ptdb(),
            fidMSO_ObjidTable,
            pfucbCatalog->kdfCurr.data,
            &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(OBJID) );
        objidTable = *(UnalignedLittleEndian< OBJID > *) dataField.Pv();

        Assert( objidTable >= objidTableLastWithLocalized );
        if ( objidTable > objidTableLastWithLocalized )
        {
            Assert( fidMSO_Type.FFixed() );
            Call( ErrRECIRetrieveFixedColumn(
                        pfcbNil,
                        pfucbCatalog->u.pfcb->Ptdb(),
                        fidMSO_Type,
                        pfucbCatalog->kdfCurr.data,
                        &dataField ) );
            CallS( err );
            Assert( dataField.Cb() == sizeof(SYSOBJ) );
            sysobj = *(UnalignedLittleEndian< SYSOBJ > *) dataField.Pv();

            if ( sysobjTable == sysobj )
            {
                Assert( fidMSO_Name.FVar() );
                Call( ErrRECIRetrieveVarColumn(
                            pfcbNil,
                            pfucbCatalog->u.pfcb->Ptdb(),
                            fidMSO_Name,
                            pfucbCatalog->kdfCurr.data,
                            &dataField ) );
                CallS( err );

                const INT cbDataField = dataField.Cb();

                if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
                {
                    AssertSz( fFalse, "Invalid fidMSO_Name column in catalog." );
                    OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"570400a0-d61a-43f6-b576-9e6c00a31c18" );
                    Error( ErrERRCheck( JET_errCatalogCorrupted ) );
                }
                Assert( sizeof( szTableName ) > cbDataField );
                UtilMemCpy( szTableName, dataField.Pv(), cbDataField );
                szTableName[cbDataField] = 0;
            }
            else if ( sysobjIndex == sysobj )
            {
                IDBFLAG     fidb;

                Assert( fidMSO_Flags.FFixed() );
                Call( ErrRECIRetrieveFixedColumn(
                            pfcbNil,
                            pfucbCatalog->u.pfcb->Ptdb(),
                            fidMSO_Flags,
                            pfucbCatalog->kdfCurr.data,
                            &dataField ) );
                CallS( err );
                Assert( dataField.Cb() == sizeof(ULONG) );
                fidb = *(UnalignedLittleEndian< IDBFLAG > *) dataField.Pv();

                Assert( fidMSO_Name.FVar() );
                Call( ErrRECIRetrieveVarColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Name,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
                CallS( err );

                const INT cbDataField = dataField.Cb();

                if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
                {
                    AssertSz( fFalse, "Invalid fidMSO_Name column in catalog." );
                    OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"570400a0-d61a-43f6-b576-9e6c00a31c18" );
                    Error( ErrERRCheck( JET_errCatalogCorrupted ) );
                }
                Assert( sizeof( szIndexName ) > cbDataField );
                UtilMemCpy( szIndexName, dataField.Pv(), cbDataField );
                szIndexName[cbDataField] = 0;
                OSTraceFMP(
                    ifmp,
                    JET_tracetagCatalog,
                    OSFormat( __FUNCTION__ " is checking table=%s, index=%s, flags=%#x...\n", szTableName, szIndexName, fidb ) );

                if ( FIDBLocalizedText( fidb ) )
                {
                    if ( fForceDelete )
                    {
                        Assert( Pcsr( pfucbCatalog )->FLatched() );
                        Call( ErrDIRRelease( pfucbCatalog ) );
                        fLatched = fFalse;
                        Assert( !FCATSystemTable( szTableName ) );
                        Call( ErrCATIDeleteOrUpdateLocalizedIndexesInTableByName(
                            ppib,
                            ifmp,
                            szTableName,
                            catcifFlags,
                            pfIndexesUpdated,
                            pfIndexesDeleted ) );

                        objidTableLastWithLocalized = objidTable;
                    }
                    else
                    {
                        QWORD   qwSortVersion           = 0;
                        SORTID sortID;
                        DWORD dwNormalizationFlags;

                        Call( ErrCATIRetrieveLocaleInformation(
                            pfucbCatalog,
                            wszLocaleName,
                            _countof( wszLocaleName ),
                            NULL,
                            &sortID,
                            &qwSortVersion,
                            &dwNormalizationFlags ) );

                        INDEX_UNICODE_STATE state;
                        Call( ErrIndexUnicodeState( wszLocaleName, qwSortVersion, &sortID, dwNormalizationFlags, &state ) );

                        switch( state )
                        {
                            case INDEX_UNICODE_GOOD:
                                //  don't need to do anything
                                OSTraceFMP(
                                    ifmp,
                                    JET_tracetagCatalog,
                                    OSFormat( " ... INDEX_UNICODE_GOOD!\n" ) );
                                break;

                            default:
                                //  missed case?!
                                AssertTrack( fFalse, "InvalidIndexUnicodeStateDelOrUpdIdx" );
                                break;

                            case INDEX_UNICODE_DELETE:
                            case INDEX_UNICODE_OUTOFDATE:
                                //  If we found one localized index in the table, it's
                                //  easiest to open the table and delete them all.

                                OSTraceFMP(
                                    ifmp,
                                    JET_tracetagCatalog,
                                    OSFormat( " ... INDEX_UNICODE_DELETE, fForceDelete=%d!\n", fForceDelete ) );

                                Assert( Pcsr( pfucbCatalog )->FLatched() );
                                Call( ErrDIRRelease( pfucbCatalog ) );
                                fLatched = fFalse;
                                Assert( !FCATSystemTable( szTableName ) );
                                Call( ErrCATIDeleteOrUpdateLocalizedIndexesInTableByName(
                                            ppib,
                                            ifmp,
                                            szTableName,
                                            catcifFlags,
                                            pfIndexesUpdated,
                                            pfIndexesDeleted ) );

                                OSTraceFMP(
                                    ifmp,
                                    JET_tracetagCatalog,
                                    OSFormat( " ... And it deleted indices: %d.\n", *pfIndexesDeleted ) )

                                objidTableLastWithLocalized = objidTable;
                                break;
                        }
                    }
                }
                else
                {
                    OSTraceFMP(
                        ifmp,
                        JET_tracetagCatalog,
                        OSFormat( " ... Not localized text.\n" ) );
                }
            }
            else
            {
                Assert( sysobjColumn == sysobj
                    || sysobjLongValue == sysobj
                    || sysobjCallback == sysobj );
            }
        }

        if ( fLatched )
        {
            Assert( Pcsr( pfucbCatalog )->FLatched() );
            Call( ErrDIRRelease( pfucbCatalog ) );
            fLatched = fFalse;
        }

        Assert( !Pcsr( pfucbCatalog )->FLatched() );
        err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
    }
    while ( JET_errNoCurrentRecord != err );

    err = JET_errSuccess;

HandleError:

    Assert( pfucbNil != pfucbCatalog );

    if ( fLatched )
    {
        Assert( Pcsr( pfucbCatalog )->FLatched() );
        Call( ErrDIRRelease( pfucbCatalog ) );
        fLatched = fFalse;
    }

    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}

//  ================================================================
LOCAL ERR ErrCATIRenameTable(
    PIB         * const ppib,
    const IFMP          ifmp,
    const OBJID         objidTable,
    const CHAR * const  szNameNew,
    const BOOL fShadow )
//  ================================================================
{
    ERR err = JET_errSuccess;
    FUCB * pfucbCatalog = pfucbNil;
    BOOL fUpdatePrepared = fFalse;

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fShadow ) );

    Call( ErrCATISeekTable( ppib, pfucbCatalog, objidTable ) );
    Call( ErrDIRRelease( pfucbCatalog ) );

    Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplace ) );
    fUpdatePrepared = fTrue;

    ULONG ulFlags;
    ULONG cbActual;
    Call( ErrIsamRetrieveColumn(
            ppib,
            pfucbCatalog,
            fidMSO_Flags,
            &ulFlags,
            sizeof( ulFlags ),
            &cbActual,
            JET_bitRetrieveCopy,
            NULL ) );

    if( ulFlags & JET_bitObjectTableTemplate
        || ulFlags & JET_bitObjectTableFixedDDL )
    {
        //  we can't rename a template table becuase the derived tables won't be updated
        Call( ErrERRCheck( JET_errFixedDDL ) );
    }

    Call( ErrIsamSetColumn(
            ppib,
            pfucbCatalog,
            fidMSO_Name,
            szNameNew,
            (ULONG)strlen( szNameNew ),
            NO_GRBIT,
            NULL ) );
    Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );
    fUpdatePrepared = fFalse;

HandleError:
    if( fUpdatePrepared )
    {
        CallS( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepCancel ) );
    }
    if( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }
    return err;
}


//  ================================================================
ERR ErrCATRenameTable(
    PIB             * const ppib,
    const IFMP      ifmp,
    const CHAR      * const szNameOld,
    const CHAR      * const szNameNew )
//  ================================================================
{
    ERR             err;
    INT             fState              = fFCBStateNull;
    FCB             * pfcbTable         = pfcbNil;
    OBJID           objidTable;
    PGNO            pgnoFDPTable;
    MEMPOOL::ITAG   itagTableNameNew    = 0;

    CallR( ErrDIRBeginTransaction( ppib, 59109, NO_GRBIT ) );

    Call( ErrCATSeekTable( ppib, ifmp, szNameOld, &pgnoFDPTable, &objidTable ) );

    //  check to see if the FCB is present and initialized
    //  if its not present we can just update the catalog

    pfcbTable = FCB::PfcbFCBGet( ifmp, pgnoFDPTable, &fState );
    if( pfcbNil != pfcbTable )
    {
        if( fFCBStateInitialized != fState )
        {

            //  this should only happen if this is called in a multi-threaded scenario
            //  (which it shouldn't be)

            Assert( fFalse );
            Call( ErrERRCheck( JET_errTableInUse ) );
        }

        //  place the new table name in memory, to avoid possibly running out of memory later

        TDB * const ptdb = pfcbTable->Ptdb();
        MEMPOOL& mempool = ptdb->MemPool();

        pfcbTable->EnterDDL();

        err = mempool.ErrAddEntry( (BYTE *)szNameNew, (ULONG)strlen( szNameNew ) + 1, &itagTableNameNew );
        Assert( 0 != itagTableNameNew || err < JET_errSuccess );    //  0 is used as "uninit"

        pfcbTable->LeaveDDL();

        Call( err );
    }

    Call( ErrCATIRenameTable( ppib, ifmp, objidTable, szNameNew, fFalse ) );
    Call( ErrCATIRenameTable( ppib, ifmp, objidTable, szNameNew, fTrue ) );

    //  once the commit succeeds, no errors can be generated

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

    if( pfcbNil != pfcbTable )
    {
        //  delete the old name from the catalog hash

        CATHashDelete( pfcbTable, const_cast< CHAR * >( szNameOld ) );

        pfcbTable->EnterDDL();

        TDB * const ptdb = pfcbTable->Ptdb();
        MEMPOOL& mempool = ptdb->MemPool();
        const MEMPOOL::ITAG itagTableNameOld = ptdb->ItagTableName();

        ptdb->SetItagTableName( itagTableNameNew );
        itagTableNameNew = 0;
        mempool.DeleteEntry( itagTableNameOld );

        pfcbTable->LeaveDDL();

        //  insert the new name into to the catalog hash

//  UNDONE: Can't reinsert into hash now because we may be renaming to a table that
//  has just been deleted, but that table's name does not get removed from the
//  hash table until the DeleteTable commits
///     CATHashInsert( pfcbTable );
    }

HandleError:

    if ( 0 != itagTableNameNew )
    {
        if ( pfcbNil != pfcbTable )
        {
            TDB * const ptdb = pfcbTable->Ptdb();
            MEMPOOL& mempool = ptdb->MemPool();

            pfcbTable->EnterDDL();
            mempool.DeleteEntry( itagTableNameNew );
            pfcbTable->LeaveDDL();
        }
    }

    if( err < 0 )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    if( pfcbNil != pfcbTable )
    {
        pfcbTable->Release();
    }
    return err;
}


//  ================================================================
LOCAL ERR ErrCATIRenameTableObject(
    PIB             * const ppib,
    const IFMP      ifmp,
    const OBJID     objidTable,
    const SYSOBJ    sysobj,
    const OBJID     objid,
    const CHAR      * const szNameNew,
    ULONG           * pulFlags,
    const BOOL      fShadow )
//  ================================================================
{
    ERR             err             = JET_errSuccess;
    FUCB            * pfucbCatalog  = pfucbNil;
    BOOL            fUpdatePrepared = fFalse;

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fShadow ) );

    Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobj, objid ) );
    Call( ErrDIRRelease( pfucbCatalog ) );

    Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplace ) );
    fUpdatePrepared = fTrue;

    Call( ErrIsamSetColumn(
            ppib,
            pfucbCatalog,
            fidMSO_Name,
            szNameNew,
            (ULONG)strlen( szNameNew ),
            NO_GRBIT,
            NULL ) );

    if ( NULL != pulFlags )
    {
        Call( ErrIsamSetColumn(
                ppib,
                pfucbCatalog,
                fidMSO_Flags,
                pulFlags,
                sizeof(ULONG),
                NO_GRBIT,
                NULL ) );
    }

    Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );
    fUpdatePrepared = fFalse;

HandleError:
    if( fUpdatePrepared )
    {
        CallS( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepCancel ) );
    }
    if( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }
    return err;
}


//  ================================================================
ERR ErrCATRenameColumn(
    PIB             * const ppib,
    const FUCB      * const pfucbTable,
    const CHAR      * const szNameOld,
    const CHAR      * const szNameNew,
    const JET_GRBIT grbit )
//  ================================================================
{
    ERR             err                 = JET_errSuccess;
    const INT       cbSzNameNew         = (ULONG)strlen( szNameNew ) + 1;
    Assert( cbSzNameNew > 1 );

    MEMPOOL::ITAG   itagColumnNameNew   = 0;
    MEMPOOL::ITAG   itagColumnNameOld   = 0;

    FCB             * const pfcbTable   = pfucbTable->u.pfcb;
    TDB             * const ptdbTable   = pfcbTable->Ptdb();
    FIELD           * pfield            = NULL;
    OBJID           objidTable;
    COLUMNID        columnid;
    ULONG           ulFlags;
    BOOL            fPrimaryIndexPlaceholder    = fFalse;

    Assert( 0 == ppib->Level() );
    CallR( ErrDIRBeginTransaction( ppib, 34533, NO_GRBIT ) );

    objidTable  = pfcbTable->ObjidFDP();

    pfcbTable->EnterDML();

    //  Get a pointer to the FIELD of the column
    //  WARNING: The following function does a LeaveDML() on error

    Call( ErrFILEPfieldFromColumnName(
            ppib,
            pfcbTable,
            szNameOld,
            &pfield,
            &columnid ) );

    pfcbTable->LeaveDML();

    if ( pfieldNil == pfield )
    {
        Call( ErrERRCheck( JET_errColumnNotFound ) );
    }

    pfcbTable->EnterDDL();

    //  put the new column name in the mempool
    //  do this before getting the FIELD in case we re-arrange the mempool

    err = ptdbTable->MemPool().ErrAddEntry( (BYTE *)szNameNew, cbSzNameNew, &itagColumnNameNew );
    if( err < 0 )
    {
        pfcbTable->LeaveDDL();
        Call( err );
    }
    Assert( 0 != itagColumnNameNew );   //  0 is used as "uninit"

    //  must refresh pfield pointer in case mempool got rearranged adding the new name
    pfield = ptdbTable->Pfield( columnid );
    Assert( pfieldNil != pfield );
    Assert( 0 == UtilCmpName( szNameOld, ptdbTable->SzFieldName( pfield->itagFieldName, fFalse ) ) );

    if ( grbit & JET_bitColumnRenameConvertToPrimaryIndexPlaceholder )
    {
        IDB     * const pidb    = pfcbTable->Pidb();

        //  placeholder column must be fixed bitfield and first column of primary index
        if ( JET_coltypBit != pfield->coltyp
            || !FCOLUMNIDFixed( columnid )
            || pidbNil == pidb
            || pidb->Cidxseg() < 2
            || PidxsegIDBGetIdxSeg( pidb, ptdbTable )[0].Columnid() != columnid )
        {
            AssertSz( fFalse, "Column cannot be converted to a placeholder." );
            pfcbTable->LeaveDDL();
            Call( ErrERRCheck( JET_errInvalidPlaceholderColumn ) );
        }

        Assert( pidb->FPrimary() );
        fPrimaryIndexPlaceholder = fTrue;

        ulFlags = FIELDFLAGPersisted(
                        FIELDFLAG( pfield->ffield | ffieldPrimaryIndexPlaceholder ) );
    }

    pfcbTable->LeaveDDL();


    //  Template bit is not persisted
    Assert( !FCOLUMNIDTemplateColumn( columnid ) || pfcbTable->FTemplateTable() );
    COLUMNIDResetFTemplateColumn( columnid );

    //  rename in the catalog and shadow catalog

    Call( ErrCATIRenameTableObject(
                ppib,
                pfucbTable->ifmp,
                objidTable,
                sysobjColumn,
                columnid,
                szNameNew,
                fPrimaryIndexPlaceholder ? &ulFlags : NULL,
                fFalse ) );
    Call( ErrCATIRenameTableObject(
                ppib,
                pfucbTable->ifmp,
                objidTable,
                sysobjColumn,
                columnid,
                szNameNew,
                fPrimaryIndexPlaceholder ? &ulFlags : NULL,
                fTrue ) );

    //  once the commit succeeds, no errors can be generated

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

    pfcbTable->EnterDML();

    Call( ErrFILEPfieldFromColumnName(
            ppib,
            pfcbTable,
            szNameOld,
            &pfield,
            &columnid ) );
    Assert( pfieldNil != pfield );

    itagColumnNameOld = pfield->itagFieldName;
    Assert( itagColumnNameOld != itagColumnNameNew );

    pfield->itagFieldName = itagColumnNameNew;
    pfield->strhashFieldName = StrHashValue( szNameNew );
    itagColumnNameNew = 0;

    pfcbTable->LeaveDML();
    pfcbTable->EnterDDL();

    ptdbTable->MemPool().DeleteEntry( itagColumnNameOld );

    if ( fPrimaryIndexPlaceholder )
    {
        FIELDSetPrimaryIndexPlaceholder( pfield->ffield );

        Assert( pidbNil != pfcbTable->Pidb() );
        Assert( pfcbTable->Pidb()->FPrimary() );
        pfcbTable->Pidb()->SetFHasPlaceholderColumn();
    }

    pfcbTable->LeaveDDL();

HandleError:

    if( 0 != itagColumnNameNew )
    {
        pfcbTable->EnterDDL();
        ptdbTable->MemPool().DeleteEntry( itagColumnNameNew );
        pfcbTable->LeaveDDL();
    }

    if( err < 0 )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    return err;
}


//  ================================================================
ERR ErrCATAddCallbackToTable(
    PIB * const ppib,
    const IFMP ifmp,
    const CHAR * const szTable,
    const JET_CBTYP cbtyp,
    const CHAR * const szCallback )
//  ================================================================
//
//  Used during upgrade. This does not add the callback to the TDB
//  and doesn't version it properly
//
//-
{
    ERR     err             = JET_errSuccess;
    OBJID   objidTable      = objidNil;
    PGNO    pgnoFDPTable    = pgnoNull;

    CallR( ErrDIRBeginTransaction( ppib, 50917, NO_GRBIT ) );
    Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDPTable, &objidTable ) );
    Call( ErrCATAddTableCallback( ppib, ifmp, objidTable, cbtyp, szCallback ) );
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
    if( err < 0 )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    return err;
}


//  ================================================================
ERR ErrCATIConvert(
    PIB * const ppib,
    FUCB * const pfucbCatalog,
    JET_SETCOLUMN * const psetcols,
    const ULONG csetcols )
//  ================================================================
{
    ERR     err             = JET_errSuccess;

    CallR( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );
    Call( ErrIsamSetColumns( (JET_SESID)ppib, (JET_VTID)pfucbCatalog, psetcols, csetcols ) );
    Call( ErrIsamUpdate( ppib, pfucbCatalog, NULL, 0, NULL, NO_GRBIT ) );

HandleError:
    if( err < 0 )
    {
        CallS( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepCancel ) );
    }
    return err;
}

//  ================================================================
ERR ErrCATAddColumnCallback(
    PIB * const ppib,
    const IFMP ifmp,
    const CHAR * const szTable,
    const CHAR * const szColumn,
    const CHAR * const szCallback,
    const VOID * const pvCallbackData,
    const ULONG cbCallbackData
    )
//  ================================================================
{
    ERR         err             = JET_errSuccess;
    OBJID       objidTable      = objidNil;
    PGNO        pgnoFDPTable    = pgnoNull;
    FUCB *      pfucbCatalog    = pfucbNil;
    BOOKMARK    bm;
    BYTE        *pbBookmark     = NULL;
    ULONG       cbBookmark;
    DATA        dataField;
    FIELDFLAG   ffield      = 0;
    ULONG       ulFlags;

    JET_SETCOLUMN   rgsetcolumn[3];
    memset( rgsetcolumn, 0, sizeof( rgsetcolumn ) );

    rgsetcolumn[0].columnid = fidMSO_Callback;
    rgsetcolumn[0].pvData   = szCallback;
    rgsetcolumn[0].cbData   = (ULONG)strlen( szCallback );
    rgsetcolumn[0].grbit    = NO_GRBIT;
    rgsetcolumn[0].ibLongValue  = 0;
    rgsetcolumn[0].itagSequence = 1;
    rgsetcolumn[0].err      = JET_errSuccess;

    rgsetcolumn[1].columnid = fidMSO_CallbackData;
    rgsetcolumn[1].pvData   = pvCallbackData;
    rgsetcolumn[1].cbData   = cbCallbackData;
    rgsetcolumn[1].grbit    = NO_GRBIT;
    rgsetcolumn[1].ibLongValue  = 0;
    rgsetcolumn[1].itagSequence = 1;
    rgsetcolumn[1].err      = JET_errSuccess;

    rgsetcolumn[2].columnid = fidMSO_Flags;
    rgsetcolumn[2].pvData   = &ulFlags;
    rgsetcolumn[2].cbData   = sizeof( ulFlags );
    rgsetcolumn[2].grbit    = NO_GRBIT;
    rgsetcolumn[2].ibLongValue  = 0;
    rgsetcolumn[2].itagSequence = 1;
    rgsetcolumn[2].err      = JET_errSuccess;

    CallR( ErrDIRBeginTransaction( ppib, 47845, NO_GRBIT ) );
    Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDPTable, &objidTable ) );

    //  We have to modify both the catalog and the shadow catalog
    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );    //  catalog
    Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjColumn, szColumn ) );

    // get the exsiting flags
    Assert( fidMSO_Flags.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Flags,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );
    ffield = *(UnalignedLittleEndian< FIELDFLAG > *)dataField.Pv();

    FIELDSetUserDefinedDefault( ffield );
    FIELDSetDefault( ffield );

    ulFlags = ffield;

    Call( ErrDIRRelease( pfucbCatalog ) );

    Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
    Assert( pfucbCatalog->bmCurr.data.FNull() );
    Alloc( pbBookmark = (BYTE *)RESBOOKMARK.PvRESAlloc() );
    cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), cbBookmarkAlloc );
    Assert( cbBookmark <= cbBookmarkMost );
    pfucbCatalog->bmCurr.key.CopyIntoBuffer( pbBookmark, cbBookmark );

    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( pbBookmark );
    bm.key.suffix.SetCb( cbBookmark );
    bm.data.Nullify();

    Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );     //  shadow catalog
    Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );
    Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
    RESBOOKMARK.Free( pbBookmark );

    if( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }
    if( err < 0 )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    return err;
}

//  ================================================================
ERR ErrCATConvertColumn(
    PIB * const ppib,
    const IFMP ifmp,
    const CHAR * const szTable,
    const CHAR * const szColumn,
    const JET_COLTYP coltyp,
    const JET_GRBIT grbit )
//  ================================================================
{
    ERR         err             = JET_errSuccess;
    OBJID       objidTable      = objidNil;
    PGNO        pgnoFDPTable    = pgnoNull;
    FUCB *      pfucbCatalog    = pfucbNil;
    BOOKMARK    bm;
    BYTE        *pbBookmark     = NULL;
    ULONG       cbBookmark;
    FIELDFLAG   ffield = 0;

    //  this does no error checking. its for specialized use
    if ( grbit & JET_bitColumnEscrowUpdate )
    {
        FIELDSetEscrowUpdate( ffield );
        FIELDSetDefault( ffield );      //  escrow update fields must have a default valie
    }
    if ( grbit & JET_bitColumnFinalize )
    {
        FIELDSetFinalize( ffield );
    }
    if ( grbit & JET_bitColumnDeleteOnZero )
    {
        FIELDSetDeleteOnZero( ffield );
    }
    if ( grbit & JET_bitColumnVersion )
    {
        FIELDSetVersion( ffield );
    }
    if ( grbit & JET_bitColumnAutoincrement )
    {
        FIELDSetAutoincrement( ffield );
    }
    if ( grbit & JET_bitColumnNotNULL )
    {
        FIELDSetNotNull( ffield );
    }
    if ( grbit & JET_bitColumnMultiValued )
    {
        FIELDSetMultivalued( ffield );
    }
    if ( grbit & JET_bitColumnCompressed )
    {
        FIELDSetCompressed( ffield );
    }
    if ( grbit & JET_bitColumnEncrypted )
    {
        FIELDSetEncrypted( ffield );
    }

    const ULONG ulFlags = ffield;

    JET_SETCOLUMN   rgsetcolumn[2];
    memset( rgsetcolumn, 0, sizeof( rgsetcolumn ) );

    rgsetcolumn[0].columnid = fidMSO_Coltyp;
    rgsetcolumn[0].pvData   = &coltyp;
    rgsetcolumn[0].cbData   = sizeof( coltyp );
    rgsetcolumn[0].grbit    = NO_GRBIT;
    rgsetcolumn[0].ibLongValue  = 0;
    rgsetcolumn[0].itagSequence = 1;
    rgsetcolumn[0].err      = JET_errSuccess;

    rgsetcolumn[1].columnid = fidMSO_Flags;
    rgsetcolumn[1].pvData   = &ulFlags;
    rgsetcolumn[1].cbData   = sizeof( ulFlags );
    rgsetcolumn[1].grbit    = NO_GRBIT;
    rgsetcolumn[1].ibLongValue  = 0;
    rgsetcolumn[1].itagSequence = 1;
    rgsetcolumn[1].err      = JET_errSuccess;

    CallR( ErrDIRBeginTransaction( ppib, 64229, NO_GRBIT ) );
    Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDPTable, &objidTable ) );

    //  We have to modify both the catalog and the shadow catalog
    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );    //  catalog
    Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjColumn, szColumn ) );

    Call( ErrDIRRelease( pfucbCatalog ) );

    Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
    Assert( pfucbCatalog->bmCurr.data.FNull() );
    Alloc( pbBookmark = (BYTE *)RESBOOKMARK.PvRESAlloc() );
    cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), cbBookmarkAlloc );
    Assert( cbBookmark <= cbBookmarkMost );
    pfucbCatalog->bmCurr.key.CopyIntoBuffer( pbBookmark, cbBookmark );

    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( pbBookmark );
    bm.key.suffix.SetCb( cbBookmark );
    bm.data.Nullify();

    Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );     //  shadow catalog
    Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );
    Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
    RESBOOKMARK.Free( pbBookmark );

    if( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }
    if( err < 0 )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    return err;
}


//  ================================================================
ERR ErrCATIncreaseMaxColumnSize(
    PIB * const         ppib,
    const IFMP          ifmp,
    const CHAR * const  szTable,
    const CHAR * const  szColumn,
    const ULONG         cbMaxLen )
//  ================================================================
{
    ERR                 err             = JET_errSuccess;
    OBJID               objidTable      = objidNil;
    PGNO                pgnoFDPTable    = pgnoNull;
    FUCB *              pfucbCatalog    = pfucbNil;
    DATA                dataField;
    BOOKMARK            bm;
    BYTE                *pbBookmark     = NULL;
    ULONG               cbBookmark;
    JET_SETCOLUMN       rgsetcolumn[1];

    memset( rgsetcolumn, 0, sizeof( rgsetcolumn ) );

    rgsetcolumn[0].columnid = fidMSO_SpaceUsage;
    rgsetcolumn[0].pvData   = &cbMaxLen;
    rgsetcolumn[0].cbData   = sizeof( cbMaxLen );
    rgsetcolumn[0].grbit    = NO_GRBIT;
    rgsetcolumn[0].ibLongValue  = 0;
    rgsetcolumn[0].itagSequence = 1;
    rgsetcolumn[0].err      = JET_errSuccess;

    CallR( ErrDIRBeginTransaction( ppib, 39653, NO_GRBIT ) );
    Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDPTable, &objidTable ) );

    //  We have to modify both the catalog and the shadow catalog
    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );    //  catalog
    Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjColumn, szColumn ) );

    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Assert( fidMSO_Coltyp.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
        pfcbNil,
        pfucbCatalog->u.pfcb->Ptdb(),
        fidMSO_Coltyp,
        pfucbCatalog->kdfCurr.data,
        &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(JET_COLTYP) );
    const JET_COLTYP coltyp = *( UnalignedLittleEndian< JET_COLTYP > *)dataField.Pv();

    JET_COLUMNID columnid = objidNil;
    switch ( coltyp )
    {
        case JET_coltypLongBinary:
        case JET_coltypLongText:
            break;

        case JET_coltypBinary:
        case JET_coltypText:
            Assert( fidMSO_Id.FFixed() );
            Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Id,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
            CallS( err );
            Assert( dataField.Cb() == sizeof(JET_COLUMNID) );
            columnid = *( UnalignedLittleEndian< JET_COLUMNID > *)dataField.Pv();

            // Fixed columns can not be resized, even if they are a normally supported type
            if ( FCOLUMNIDFixed( columnid ) || cbMaxLen > JET_cbColumnMost )
            {
                Call( ErrERRCheck( JET_errInvalidParameter ) );
            }
            break;

        default:
            Call( ErrERRCheck( JET_errInvalidParameter ) );
            break;
    }

    if ( cbMaxLen > 0 )
    {
        Assert( fidMSO_SpaceUsage.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_SpaceUsage,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(ULONG) );
        if ( *(UnalignedLittleEndian< ULONG > *)dataField.Pv() > cbMaxLen
            || 0 == *(UnalignedLittleEndian< ULONG > *)dataField.Pv() )
        {
            //  cannot decrease max. column size
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    Call( ErrDIRRelease( pfucbCatalog ) );

    Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
    Assert( pfucbCatalog->bmCurr.data.FNull() );
    Alloc( pbBookmark = (BYTE *)RESBOOKMARK.PvRESAlloc() );
    cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), cbBookmarkAlloc );
    Assert( cbBookmark <= cbBookmarkMost );
    pfucbCatalog->bmCurr.key.CopyIntoBuffer( pbBookmark, cbBookmark );

    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( pbBookmark );
    bm.key.suffix.SetCb( cbBookmark );
    bm.data.Nullify();

    Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );     //  shadow catalog
    Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );
    Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
    RESBOOKMARK.Free( pbBookmark );

    if( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }
    if( err < 0 )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    return err;
}


//  ================================================================
ERR ErrCATChangeIndexDensity(
    PIB * const         ppib,
    const IFMP          ifmp,
    const CHAR * const  szTable,
    const CHAR * const  szIndex,
    const ULONG         ulDensity )
//  ================================================================
{
    ERR                 err             = JET_errSuccess;
    OBJID               objidTable      = objidNil;
    PGNO                pgnoFDPTable    = pgnoNull;
    FUCB *              pfucbCatalog    = pfucbNil;
    BOOKMARK            bm;
    BYTE                *pbBookmark     = NULL;
    ULONG               cbBookmark;
    JET_SETCOLUMN       rgsetcolumn[1];

    if ( ulDensity < ulFILEDensityLeast || ulDensity > ulFILEDensityMost )
    {
        return ErrERRCheck( JET_errDensityInvalid );
    }

    memset( rgsetcolumn, 0, sizeof( rgsetcolumn ) );

    rgsetcolumn[0].columnid = fidMSO_SpaceUsage;
    rgsetcolumn[0].pvData   = &ulDensity;
    rgsetcolumn[0].cbData   = sizeof( ulDensity );
    rgsetcolumn[0].grbit    = NO_GRBIT;
    rgsetcolumn[0].ibLongValue  = 0;
    rgsetcolumn[0].itagSequence = 1;
    rgsetcolumn[0].err      = JET_errSuccess;

    CallR( ErrDIRBeginTransaction( ppib, 56037, NO_GRBIT ) );
    Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDPTable, &objidTable ) );

    //  We have to modify both the catalog and the shadow catalog,
    //  but start with the regular catalog first
    //
    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );

    if ( NULL != szIndex )
    {
        //  index specified, so go to that index
        //
        Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjIndex, szIndex ) );
    }
    else
    {
        //  index not specified, so go to the primary index
        //
        err = ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjIndex, objidTable );

        if ( JET_errIndexNotFound == err )
        {
            //  no primary index, so go to the sequential index
            //
            err = ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjTable, objidTable );
        }

        Call( err );
    }

    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Call( ErrDIRRelease( pfucbCatalog ) );

    //  save the bookmark so we can use it to update
    //  the shadow catalog
    //
    Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
    Assert( pfucbCatalog->bmCurr.data.FNull() );
    Alloc( pbBookmark = (BYTE *)RESBOOKMARK.PvRESAlloc() );
    cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), cbBookmarkAlloc );
    Assert( cbBookmark <= cbBookmarkMost );
    pfucbCatalog->bmCurr.key.CopyIntoBuffer( pbBookmark, cbBookmark );

    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( pbBookmark );
    bm.key.suffix.SetCb( cbBookmark );
    bm.data.Nullify();

    //  update the catalog record
    //
    Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    //  now update the corresponding record
    //  in the shadow catalog
    //
    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );     //  shadow catalog
    Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );
    Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
    RESBOOKMARK.Free( pbBookmark );

    if( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }
    if( err < 0 )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    return err;
}


//  ================================================================
ERR ErrCATChangeIndexFlags(
    PIB * const         ppib,
    const IFMP          ifmp,
    const OBJID         objidTable,
    const CHAR * const  szIndex,
    const IDBFLAG       idbflagNew,
    const IDXFLAG       idxflagNew )
//  ================================================================
{
    ERR                 err             = JET_errSuccess;
    FUCB *              pfucbCatalog    = pfucbNil;
    BOOKMARK            bm;
    BYTE                *pbBookmark     = NULL;
    ULONG               cbBookmark;
    LE_IDXFLAG          le_idxflag;
    JET_SETCOLUMN       rgsetcolumn[1];

    memset( rgsetcolumn, 0, sizeof( rgsetcolumn ) );

    le_idxflag.fidb         = idbflagNew;
    le_idxflag.fIDXFlags    = ( idxflagNew | fIDXExtendedColumns );

    //  Hack on this field: SetColumn() will convert the fixed
    //  columns. So convert it here so that later it can be
    //  converted back to current value.
    LONG        l           = *(LONG *)&le_idxflag;
    l = ReverseBytesOnBE( l );

    rgsetcolumn[0].columnid = fidMSO_Flags;
    rgsetcolumn[0].pvData   = &l;
    rgsetcolumn[0].cbData   = sizeof( l );
    rgsetcolumn[0].grbit    = NO_GRBIT;
    rgsetcolumn[0].ibLongValue  = 0;
    rgsetcolumn[0].itagSequence = 1;
    rgsetcolumn[0].err      = JET_errSuccess;

    CallR( ErrDIRBeginTransaction( ppib, 43749, NO_GRBIT ) );

    //  We have to modify both the catalog and the shadow catalog,
    //  but start with the regular catalog first
    //
    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );

    if ( NULL != szIndex )
    {
        //  index specified, so go to that index
        //
        Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjIndex, szIndex ) );
    }
    else
    {
        //  index not specified, so go to the primary index
        //
        Call( ErrCATISeekTableObject( ppib, pfucbCatalog, objidTable, sysobjIndex, objidTable ) );
    }

    Assert( Pcsr( pfucbCatalog )->FLatched() );

    Call( ErrDIRRelease( pfucbCatalog ) );

    //  save the bookmark so we can use it to update
    //  the shadow catalog
    //
    Assert( pfucbCatalog->bmCurr.key.prefix.FNull() );
    Assert( pfucbCatalog->bmCurr.data.FNull() );
    Alloc( pbBookmark = (BYTE *)RESBOOKMARK.PvRESAlloc() );
    cbBookmark = min( pfucbCatalog->bmCurr.key.Cb(), cbBookmarkAlloc );
    Assert( cbBookmark <= cbBookmarkMost );
    pfucbCatalog->bmCurr.key.CopyIntoBuffer( pbBookmark, cbBookmark );

    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( pbBookmark );
    bm.key.suffix.SetCb( cbBookmark );
    bm.data.Nullify();

    //  update the catalog record
    //
    Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    //  now update the corresponding record
    //  in the shadow catalog
    //
    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );     //  shadow catalog
    Call( ErrDIRGotoBookmark( pfucbCatalog, bm ) );
    Call( ErrCATIConvert( ppib, pfucbCatalog, rgsetcolumn, sizeof( rgsetcolumn ) / sizeof( rgsetcolumn[0] ) ) );
    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
    RESBOOKMARK.Free( pbBookmark );

    if( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }
    if( err < 0 )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    return err;
}


//  ================================================================
LOCAL ERR ErrCATIChangeOneCallbackDLL(
    PIB * const         ppib,
    FUCB * const        pfucbCatalog,
    const CHAR * const  szCallbackNew )
//  ================================================================
//
//  Given the new callback, change the column in the catalog and its
//  shadow
//
//-
{
    const size_t cbCallbackNew = strlen( szCallbackNew );

    ERR err;

    BOOKMARK bm;
    BYTE * pbBookmark = NULL;
    ULONG cbBookmark;

    FUCB * pfucbShadowCatalog = pfucbNil;

    Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );
    Call( ErrIsamSetColumn( ppib, pfucbCatalog, fidMSO_Callback, szCallbackNew, cbCallbackNew, NO_GRBIT, NULL ) );
    Alloc( pbBookmark = (BYTE *)RESBOOKMARK.PvRESAlloc() );
    Call( ErrIsamUpdate( ppib, pfucbCatalog, pbBookmark, cbBookmarkAlloc, &cbBookmark, NO_GRBIT ) );
    Assert( cbBookmark <= cbBookmarkAlloc );

    bm.key.Nullify();
    bm.data.Nullify();
    bm.key.suffix.SetPv( pbBookmark );
    bm.key.suffix.SetCb( cbBookmark );

    Call( ErrCATOpen( ppib, pfucbCatalog->ifmp, &pfucbShadowCatalog, fTrue ) );
    Call( ErrDIRGotoBookmark( pfucbShadowCatalog, bm ) );
    Call( ErrIsamPrepareUpdate( ppib, pfucbShadowCatalog, JET_prepReplaceNoLock ) );
    Call( ErrIsamSetColumn( ppib, pfucbShadowCatalog, fidMSO_Callback, szCallbackNew, cbCallbackNew, NO_GRBIT, NULL ) );
    Call( ErrIsamUpdate( ppib, pfucbShadowCatalog, NULL, 0, NULL, NO_GRBIT ) );

HandleError:
    if( pfucbNil != pfucbShadowCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbShadowCatalog ) );
    }
    RESBOOKMARK.Free( pbBookmark );
    return err;
}


//  ================================================================
LOCAL ERR ErrCATIPossiblyChangeOneCallbackDLL(
    PIB * const         ppib,
    FUCB * const        pfucbCatalog,
    const CHAR * const  szOldDLL,
    const CHAR * const  szNewDLL )
//  ================================================================
{
    ERR err;

    const size_t cchOldDLL = strlen( szOldDLL );
    const size_t cchNewDLL = strlen( szNewDLL );

    DATA dataField;

    bool fLatched = false;

    Assert( !fLatched );
    Call( ErrDIRGet( pfucbCatalog ) );
    fLatched = true;

    // retrieve the name of the current callback

    Assert( fidMSO_Callback.FVar() );
    Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Callback,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );

    const INT cbDataField = dataField.Cb();

    if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
    {
        AssertSz( fFalse, "Invalid fidMSO_Callback column in catalog." );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"9df4cd28-bc51-4754-b3cc-5dc2ff9083de" );
        Error( ErrERRCheck( JET_errCatalogCorrupted ) );
    }

    CHAR szCallbackCurrent[JET_cbNameMost+1];
    UtilMemCpy( szCallbackCurrent, dataField.Pv(), cbDataField );
    szCallbackCurrent[cbDataField] = '\0';

    Assert( fLatched );
    Assert( Pcsr( pfucbCatalog )->FLatched() );
    CallS( ErrDIRRelease( pfucbCatalog ) );
    fLatched = false;

    // now we have the callback. see if it has the old name
    if( 0 == _strnicmp( szOldDLL, szCallbackCurrent, cchOldDLL ) )
    {
        const CHAR * const pchSep = strchr( szCallbackCurrent, chCallbackSep );
        if( NULL == pchSep )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        const CHAR * const szFunction = pchSep + 1;
        const size_t cchFunction = strlen( szFunction );

        // the callback is "dll name + separator char + function"
        if( ( cchNewDLL + 1 + cchFunction ) > JET_cbNameMost )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }

        CHAR szCallbackNew[JET_cbNameMost+1];
        _snprintf_s(    szCallbackNew,
                        sizeof(szCallbackNew),
                        sizeof(szCallbackNew),
                        "%s%c%s",
                        szNewDLL,
                        chCallbackSep,
                        szFunction );

        // update the catalog with the new name
        Call( ErrCATIChangeOneCallbackDLL(
                ppib,
                pfucbCatalog,
                szCallbackNew ) );
    }

HandleError:
    if( fLatched )
    {
        Assert( Pcsr( pfucbCatalog )->FLatched() );
        CallS( ErrDIRRelease( pfucbCatalog ) );
    }
    return err;
}


//  ================================================================
ERR ErrCATChangeCallbackDLL(
    PIB * const         ppib,
    const IFMP          ifmp,
    const CHAR * const  szOldDLL,
    const CHAR * const  szNewDLL )
//  ================================================================
//
//  Given an old DLL and a new DLL find all callbacks that are using
//  the old DLL and change them to use the new DLL.
//
//-
{
    ERR err;

    FUCB * pfucbCatalog = pfucbNil;

    bool fInTransaction = false;
    bool fLatched = false;

    Assert( !fInTransaction );
    Call( ErrDIRBeginTransaction( ppib, 60133, NO_GRBIT ) );
    fInTransaction = true;

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    FUCBSetSequential( pfucbCatalog );

    err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT );
    if ( JET_errNoCurrentRecord == err )
    {
        // catalog should not be empty
        AssertSz( fFalse, "JET_errCatalogCorrupted" );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"004666d3-b28d-4b34-a09b-35f238046b59" );
        err = ErrERRCheck( JET_errCatalogCorrupted );
    }

    do
    {
        DATA dataField;

        Call( err );

        Assert( !fLatched );
        Call( ErrDIRGet( pfucbCatalog ) );
        fLatched = true;

        Assert( fidMSO_Type.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Type,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(SYSOBJ) );

        const SYSOBJ sysobj = *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() );

        Assert( fLatched );
        Assert( Pcsr( pfucbCatalog )->FLatched() );
        CallS( ErrDIRRelease( pfucbCatalog ) );
        fLatched = false;

        if( sysobjCallback == sysobj || sysobjColumn == sysobj )
        {
            Call( ErrCATIPossiblyChangeOneCallbackDLL(
                    ppib,
                    pfucbCatalog,
                    szOldDLL,
                    szNewDLL ) );
        }

        Assert( !Pcsr( pfucbCatalog )->FLatched() );
        err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
    }
    while ( JET_errNoCurrentRecord != err );

    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    Assert( fInTransaction );
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTransaction = false;

HandleError:
    if( fLatched )
    {
        Assert( Pcsr( pfucbCatalog )->FLatched() );
        CallS( ErrDIRRelease( pfucbCatalog ) );
    }
    if( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }
    if( fInTransaction )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    return err;
}



//  ================================================================
LOCAL ERR ErrCATIGetColumnsOfIndex(
    PIB         * const ppib,
    FUCB        * const pfucbCatalog,
    const OBJID objidTable,
    const BOOL  fTemplateTable,
    const TCIB  * const ptcibTemplateTable,
    LE_IDXFLAG  * const ple_idxflag,
    IDXSEG      * const rgidxseg,
    ULONG       * const pcidxseg,
    IDXSEG      * const rgidxsegConditional,
    ULONG       * const pcidxsegConditional,
    BOOL        * const pfPrimaryIndex
    )
//  ================================================================
{
    ERR     err;
    DATA    dataField;

    Assert( !Pcsr( pfucbCatalog )->FLatched() );
    CallR( ErrDIRGet( pfucbCatalog ) );

    DATA&   dataRec = pfucbCatalog->kdfCurr.data;

    Assert( pfcbNil != pfucbCatalog->u.pfcb );
    Assert( pfucbCatalog->u.pfcb->FTypeTable() );
    Assert( pfucbCatalog->u.pfcb->FFixedDDL() );
    Assert( pfucbCatalog->u.pfcb->Ptdb() != ptdbNil );
    TDB * const ptdbCatalog = pfucbCatalog->u.pfcb->Ptdb();

    //  verify still on same table
    Assert( fidMSO_ObjidTable.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_ObjidTable,
                dataRec,
                &dataField ) );
    CallS( err );
    if( dataField.Cb() != sizeof(OBJID)
        || objidTable != *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) )
    {
        AssertSz( fFalse, "Catalog corruption" );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"6d23e411-1282-4c23-b020-07f82d6ac0a4" );
        Call( ErrERRCheck( JET_errCatalogCorrupted ) );
    }

    //  verify this is an index
    Assert( fidMSO_Type.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_Type,
                dataRec,
                &dataField ) );
    CallS( err );
    if( dataField.Cb() != sizeof(SYSOBJ)
        || sysobjIndex != *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) )
    {
        AssertSz( fFalse, "Catalog corruption" );
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"ed01a5b3-ae30-41ac-a0fd-30f79df6f528" );
        Call( ErrERRCheck( JET_errCatalogCorrupted ) );
    }

    Assert( fidMSO_Flags.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_Flags,
                dataRec,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );

    *ple_idxflag = *(LE_IDXFLAG*)dataField.Pv();

    if( FIDBPrimary( ple_idxflag->fidb ) )
    {
        *pfPrimaryIndex = fTrue;
        *pcidxseg = 0;
        *pcidxsegConditional = 0;
        goto HandleError;
    }

    *pfPrimaryIndex = fFalse;

    Assert( fidMSO_KeyFldIDs.FVar() );
    Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_KeyFldIDs,
                dataRec,
                &dataField ) );

    // the length of the list of key fields should be a multiple
    // of the length of one field.
    if ( FIDXExtendedColumns( ple_idxflag->fIDXFlags ) )
    {
        Assert( sizeof(IDXSEG) == sizeof(JET_COLUMNID) );
        if (    dataField.Cb() / sizeof(JET_COLUMNID) > JET_ccolKeyMost ||
                dataField.Cb() % sizeof(JET_COLUMNID) != 0 )
        {
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"db03f721-efb2-456f-8889-d76edb952056" );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }
        *pcidxseg = dataField.Cb() / sizeof(JET_COLUMNID);

        if ( FHostIsLittleEndian() )
        {
            UtilMemCpy( rgidxseg, dataField.Pv(), dataField.Cb() );
#ifdef DEBUG
            for ( ULONG iidxseg = 0; iidxseg < *pcidxseg; iidxseg++ )
            {
                Assert( !fTemplateTable || rgidxseg[iidxseg].FTemplateColumn() );
                Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );
            }
#endif
        }
        else
        {
            LE_IDXSEG       *le_rgidxseg    = (LE_IDXSEG*)dataField.Pv();
            for ( ULONG iidxseg = 0; iidxseg < *pcidxseg; iidxseg++ )
            {
                LE_IDXSEG le_idxseg = ((LE_IDXSEG*)le_rgidxseg)[iidxseg]; // see if this will work for CISCO UNIX
                rgidxseg[iidxseg] = le_idxseg;
                Assert( !fTemplateTable || rgidxseg[iidxseg].FTemplateColumn() );
                Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );
            }
        }
    }
    else
    {
        Assert( sizeof(IDXSEG_OLD) == sizeof(FID) );
        if (    dataField.Cb() / sizeof(FID) > JET_ccolKeyMost ||
                dataField.Cb() % sizeof(FID) != 0 )
        {
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"a3389c23-5b87-4dcb-8584-d8fb6cfc2949" );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }
        *pcidxseg = dataField.Cb() / sizeof(FID);
        SetIdxSegFromOldFormat(
                (UnalignedLittleEndian< IDXSEG_OLD > *)dataField.Pv(),
                rgidxseg,
                *pcidxseg,
                fFalse,
                fTemplateTable,
                ptcibTemplateTable );
    }

    Assert( fidMSO_ConditionalColumns.FVar() );
    Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                ptdbCatalog,
                fidMSO_ConditionalColumns,
                dataRec,
                &dataField ) );

    // the length of the list of key fields should be a multiple
    // of the length of one field.
    if ( FIDXExtendedColumns( ple_idxflag->fIDXFlags ) )
    {
        Assert( sizeof(IDXSEG) == sizeof(JET_COLUMNID) );
        if (    dataField.Cb() / sizeof(JET_COLUMNID) > JET_ccolKeyMost ||
                dataField.Cb() % sizeof(JET_COLUMNID) != 0 )
        {
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"7ada2c07-4ee5-4195-be6f-f2c5774ccd3a" );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }
        *pcidxsegConditional = dataField.Cb() / sizeof(JET_COLUMNID);

        if ( FHostIsLittleEndian() )
        {
            UtilMemCpy( rgidxsegConditional, dataField.Pv(), dataField.Cb() );
#ifdef DEBUG
            for ( ULONG iidxseg = 0; iidxseg < *pcidxsegConditional; iidxseg++ )
            {
                Assert( !fTemplateTable || rgidxsegConditional[iidxseg].FTemplateColumn() );
                Assert( FCOLUMNIDValid( rgidxsegConditional[iidxseg].Columnid() ) );
            }
#endif
        }
        else
        {
            LE_IDXSEG       *le_rgidxseg    = (LE_IDXSEG*)dataField.Pv();
            for ( ULONG iidxseg = 0; iidxseg < *pcidxsegConditional; iidxseg++ )
            {
                rgidxsegConditional[iidxseg] = le_rgidxseg[iidxseg];
                Assert( !fTemplateTable || rgidxsegConditional[iidxseg].FTemplateColumn() );
                Assert( FCOLUMNIDValid( rgidxsegConditional[iidxseg].Columnid() ) );
            }
        }
    }
    else
    {
        Assert( sizeof(IDXSEG_OLD) == sizeof(FID) );
        if (    dataField.Cb() / sizeof(FID) > JET_ccolKeyMost ||
                dataField.Cb() % sizeof(FID) != 0 )
        {
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"a302a54d-cb51-4e44-af4a-4f165955ce9b" );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }
        *pcidxsegConditional = dataField.Cb() / sizeof(FID);
        SetIdxSegFromOldFormat(
                (UnalignedLittleEndian< IDXSEG_OLD > *)dataField.Pv(),
                rgidxsegConditional,
                *pcidxsegConditional,
                fTrue,
                fTemplateTable,
                ptcibTemplateTable );
    }

HandleError:
    Assert( Pcsr( pfucbCatalog )->FLatched() );
    CallS( ErrDIRRelease( pfucbCatalog ) );

    return err;
}


//  ================================================================
LOCAL ERR ErrCATIAddConditionalColumnsToIndex(
    PIB * const ppib,
    FUCB * const pfucbCatalog,
    FUCB * const pfucbShadowCatalog,
    LE_IDXFLAG * ple_idxflag,
    const IDXSEG * const rgidxseg,
    const ULONG cidxseg,
    const IDXSEG * const rgidxsegExisting,
    const ULONG cidxsegExisting,
    const IDXSEG * const rgidxsegToAdd,
    const ULONG cidxsegToAdd )
//  ================================================================
//
//  pfucbCatalog should be on the record to be modified
//
//-
{
    ERR         err;
    BYTE        *pbBookmark = NULL;
    ULONG       cbBookmark;
    BOOKMARK    bm;
    UINT        iidxseg;
    IDXSEG      rgidxsegConditional[JET_ccolKeyMost];
    BYTE        * pbidxsegConditional;
    ULONG       cidxsegConditional;
    LE_IDXSEG   le_rgidxsegConditional[JET_ccolKeyMost];
    LE_IDXSEG   le_rgidxseg[JET_ccolKeyMost];

    if ( cidxsegExisting > JET_ccolKeyMost )
    {
        Assert( cidxsegExisting <= JET_ccolKeyMost );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    memcpy( rgidxsegConditional, rgidxsegExisting, sizeof(IDXSEG) * cidxsegExisting );

    //  start with the existing columns and add the new ones
    cidxsegConditional = cidxsegExisting;

    for ( iidxseg = 0; iidxseg < cidxsegToAdd; iidxseg++ )
    {
        //  verify column doesn't already exist in the list
        UINT    iidxsegT;
        for ( iidxsegT = 0; iidxsegT < cidxsegConditional; iidxsegT++ )
        {
            if ( rgidxsegConditional[iidxsegT].Columnid() == rgidxsegToAdd[iidxseg].Columnid() )
            {
                break;
            }
        }

        if ( iidxsegT == cidxsegConditional )
        {
            if ( cidxsegConditional >= JET_ccolKeyMost )
            {
                AssertSz( fFalse, "Too many conditional columns" );
                return ErrERRCheck( JET_errInvalidParameter );
            }
            memcpy( rgidxsegConditional+cidxsegConditional, rgidxsegToAdd+iidxseg, sizeof(IDXSEG) );
            cidxsegConditional++;
        }
    }
    Assert( cidxsegConditional <= cidxsegExisting + cidxsegToAdd );
    Assert( cidxsegConditional <= JET_ccolKeyMost );

    if ( cidxsegConditional == cidxsegExisting )
    {
        //  nothing to do, just get out
        return JET_errSuccess;
    }

    LONG            l;
    JET_SETCOLUMN   rgsetcolumn[3];
    ULONG           csetcols = 0;
    memset( rgsetcolumn, 0, sizeof( rgsetcolumn ) );

    if ( FHostIsLittleEndian() )
    {
#ifdef DEBUG
        for ( iidxseg = 0; iidxseg < cidxsegConditional; iidxseg++ )
        {
            Assert( FCOLUMNIDValid( rgidxsegConditional[iidxseg].Columnid() ) );
        }
#endif

        pbidxsegConditional = (BYTE *)rgidxsegConditional;
    }
    else
    {
        for ( iidxseg = 0; iidxseg < cidxsegConditional; iidxseg++ )
        {
            Assert( FCOLUMNIDValid( rgidxsegConditional[iidxseg].Columnid() ) );

            //  Endian conversion
            le_rgidxsegConditional[iidxseg] = rgidxsegConditional[iidxseg];
        }

        pbidxsegConditional = (BYTE *)le_rgidxsegConditional;
    }

    //  see if we also have to update normal index columns to new format
    if ( !FIDXExtendedColumns( ple_idxflag->fIDXFlags ) )
    {
        BYTE    *pbidxseg;

        if ( FHostIsLittleEndian() )
        {
#ifdef DEBUG
            for ( iidxseg = 0; iidxseg < cidxseg; iidxseg++ )
            {
                Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );
            }
#endif

            pbidxseg = (BYTE *)rgidxseg;
        }
        else
        {
            for ( iidxseg = 0; iidxseg < cidxseg; iidxseg++ )
            {
                Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );

                //  Endian conversion
                le_rgidxseg[iidxseg] = rgidxseg[iidxseg];
            }

            pbidxseg = (BYTE *)le_rgidxseg;
        }

        rgsetcolumn[csetcols].columnid      = fidMSO_KeyFldIDs;
        rgsetcolumn[csetcols].pvData        = pbidxseg;
        rgsetcolumn[csetcols].cbData        = sizeof(IDXSEG) * cidxseg;
        rgsetcolumn[csetcols].grbit         = NO_GRBIT;
        rgsetcolumn[csetcols].ibLongValue   = 0;
        rgsetcolumn[csetcols].itagSequence  = 1;
        rgsetcolumn[csetcols].err           = JET_errSuccess;
        ++csetcols;

        //  force to new format
        ple_idxflag->fIDXFlags = fIDXExtendedColumns;

        //  Hack on this field: SetColumn() will convert the fixed
        //  columns. So convert it here so that later it can be
        //  converted back to current value.
        l = ReverseBytesOnBE( *(LONG *)ple_idxflag );

        rgsetcolumn[csetcols].columnid      = fidMSO_Flags;
        rgsetcolumn[csetcols].pvData        = &l;
        rgsetcolumn[csetcols].cbData        = sizeof(l);
        rgsetcolumn[csetcols].grbit         = NO_GRBIT;
        rgsetcolumn[csetcols].ibLongValue   = 0;
        rgsetcolumn[csetcols].itagSequence  = 1;
        rgsetcolumn[csetcols].err           = JET_errSuccess;
        ++csetcols;
    }

    rgsetcolumn[csetcols].columnid      = fidMSO_ConditionalColumns;
    rgsetcolumn[csetcols].pvData        = pbidxsegConditional;
    rgsetcolumn[csetcols].cbData        = sizeof(IDXSEG) * cidxsegConditional;
    rgsetcolumn[csetcols].grbit         = NO_GRBIT;
    rgsetcolumn[csetcols].ibLongValue   = 0;
    rgsetcolumn[csetcols].itagSequence  = 1;
    rgsetcolumn[csetcols].err           = JET_errSuccess;
    ++csetcols;

    Call( ErrIsamPrepareUpdate( ppib, pfucbCatalog, JET_prepReplaceNoLock ) );
    Call( ErrIsamSetColumns( (JET_SESID)ppib, (JET_VTID)pfucbCatalog, rgsetcolumn, csetcols ) );
    Alloc( pbBookmark = (BYTE *)RESBOOKMARK.PvRESAlloc() );
    Call( ErrIsamUpdate( ppib, pfucbCatalog, pbBookmark, cbBookmarkAlloc, &cbBookmark, NO_GRBIT ) );
    Assert( cbBookmark <= cbBookmarkAlloc );

    bm.key.Nullify();
    bm.data.Nullify();
    bm.key.suffix.SetPv( pbBookmark );
    bm.key.suffix.SetCb( cbBookmark );

    Call( ErrDIRGotoBookmark( pfucbShadowCatalog, bm ) );
    Call( ErrIsamPrepareUpdate( ppib, pfucbShadowCatalog, JET_prepReplaceNoLock ) );
    Call( ErrIsamSetColumns( (JET_SESID)ppib, (JET_VTID)pfucbShadowCatalog, rgsetcolumn, csetcols ) );
    Call( ErrIsamUpdate( ppib, pfucbShadowCatalog, NULL, 0, NULL, NO_GRBIT ) );

HandleError:

    RESBOOKMARK.Free( pbBookmark );
    return err;
}

//  ================================================================
ERR ErrCATAddConditionalColumnsToAllIndexes(
    PIB             * const ppib,
    const IFMP      ifmp,
    const CHAR      * const szTable,
    const JET_CONDITIONALCOLUMN_A   * rgconditionalcolumn,
    const ULONG     cConditionalColumn )
//  ================================================================
{
    ERR             err;
    OBJID           objidTable;
    PGNO            pgnoFDPTable;
    FUCB            * pfucbCatalog          = pfucbNil;
    FUCB            * pfucbShadowCatalog    = pfucbNil;
    IDXSEG          rgidxsegToAdd[JET_ccolKeyMost];
    ULONG           iidxseg;
    ULONG           ulFlags;
    DATA            dataField;
    BOOL            fTemplateTable          = fFalse;
    OBJID           objidTemplateTable      = objidNil;
    TCIB            tcibTemplateTable;
    LE_IDXFLAG      le_idxflag;
    const SYSOBJ    sysobj                  = sysobjIndex;

    CallR( ErrDIRBeginTransaction( ppib, 35557, NO_GRBIT ) );

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDPTable, &objidTable ) );

    Call( ErrCATISeekTable( ppib, pfucbCatalog, objidTable ) );
    Assert( Pcsr( pfucbCatalog )->FLatched() );

    //  should be on primary index
    Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );

    Assert( fidMSO_Flags.FFixed() );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Flags,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(ULONG) );
    ulFlags = *(UnalignedLittleEndian< ULONG > *) dataField.Pv();
//  UtilMemCpy( &ulFlags, dataField.Pv(), sizeof(ULONG) );

    if ( ulFlags & JET_bitObjectTableDerived )
    {
        CHAR        szTemplateTable[JET_cbNameMost+1];
        COLUMNID    columnidLeast;

        Assert( fidMSO_TemplateTable.FVar() );
        Call( ErrRECIRetrieveVarColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_TemplateTable,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );

        const INT cbDataField = dataField.Cb();

        if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
        {
            AssertSz( fFalse, "Invalid fidMSO_TemplateTable column in catalog." );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"d15ec547-6903-470e-810e-52a97ca77df6" );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }
        Assert( sizeof( szTemplateTable ) > cbDataField );
        UtilMemCpy( szTemplateTable, dataField.Pv(), cbDataField );
        szTemplateTable[cbDataField] = '\0';

        Assert( Pcsr( pfucbCatalog )->FLatched() );
        CallS( ErrDIRRelease( pfucbCatalog ) );

        Call( ErrCATSeekTable( ppib, ifmp, szTemplateTable, NULL, &objidTemplateTable ) );
        Assert( objidNil != objidTemplateTable );

        columnidLeast = fidFixedLeast;
        CallR( ErrCATIFindLowestColumnid(
                    ppib,
                    pfucbCatalog,
                    objidTable,
                    &columnidLeast ) );
        tcibTemplateTable.fidFixedLast = FID( FidOfColumnid( columnidLeast ).FFixed() ? //  use FID:FFixed() to avoid valid columnid check
                                            FidOfColumnid( columnidLeast ) - 1 :
                                            fidFixedLeast - 1 );

        columnidLeast = fidVarLeast;
        CallR( ErrCATIFindLowestColumnid(
                    ppib,
                    pfucbCatalog,
                    objidTable,
                    &columnidLeast ) );
        tcibTemplateTable.fidVarLast = FID( FCOLUMNIDVar( columnidLeast ) ?
                                            FidOfColumnid( columnidLeast ) - 1 :
                                            fidVarLeast - 1 );

        columnidLeast = fidTaggedLeast;
        CallR( ErrCATIFindLowestColumnid(
                    ppib,
                    pfucbCatalog,
                    objidTable,
                    &columnidLeast ) );
        tcibTemplateTable.fidTaggedLast = FID( FCOLUMNIDTagged( columnidLeast ) ?
                                            FidOfColumnid( columnidLeast ) - 1 :
                                            fidTaggedLeast - 1 );
    }
    else
    {
        if ( ulFlags & JET_bitObjectTableTemplate )
            fTemplateTable = fTrue;

        Assert( Pcsr( pfucbCatalog )->FLatched() );
        CallS( ErrDIRRelease( pfucbCatalog ) );
    }

    //  should still be on the primary index, which is the Id index
    Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    for ( iidxseg = 0; iidxseg < cConditionalColumn; iidxseg++ )
    {
        const CHAR      * const szColumnName    = rgconditionalcolumn[iidxseg].szColumnName;
        const JET_GRBIT grbit                   = rgconditionalcolumn[iidxseg].grbit;
        BOOL            fColumnWasDerived       = fFalse;
        COLUMNID        columnidT;

        if( sizeof( JET_CONDITIONALCOLUMN_A ) != rgconditionalcolumn[iidxseg].cbStruct )
        {
            err = ErrERRCheck( JET_errIndexInvalidDef );
            Call( err );
        }

        if( JET_bitIndexColumnMustBeNonNull != grbit
            && JET_bitIndexColumnMustBeNull != grbit )
        {
            err = ErrERRCheck( JET_errInvalidGrbit );
            Call( err );
        }

        err = ErrCATAccessTableColumn(
                ppib,
                ifmp,
                objidTable,
                szColumnName,
                &columnidT,
                !fTemplateTable );  //  only need to lock column if it's possible it might be deleted
        if ( JET_errColumnNotFound == err )
        {
            if ( objidNil != objidTemplateTable )
            {
                CallR( ErrCATAccessTableColumn(
                        ppib,
                        ifmp,
                        objidTemplateTable,
                        szColumnName,
                        &columnidT ) );
                fColumnWasDerived = fTrue;
            }
        }
        else
        {
            CallR( err );
        }

        rgidxsegToAdd[iidxseg].ResetFlags();
        if ( JET_bitIndexColumnMustBeNull == grbit )
            rgidxsegToAdd[iidxseg].SetFMustBeNull();

        //  columnid's template bit is never persisted
        Assert( !FCOLUMNIDTemplateColumn( columnidT ) );
        if ( fColumnWasDerived || fTemplateTable )
            rgidxsegToAdd[iidxseg].SetFTemplateColumn();

        rgidxsegToAdd[iidxseg].SetFid( FidOfColumnid( columnidT ) );
    }

    Call( ErrCATOpen( ppib, ifmp, &pfucbShadowCatalog, fTrue ) );

    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&objidTable,
                sizeof(objidTable),
                JET_bitNewKey ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *)&sysobj,
                sizeof(sysobj),
                NO_GRBIT ) );

    err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekGT );
    if ( err < 0 )
    {
        if ( JET_errRecordNotFound != err )
        {
            //  No indexes?!
            Assert( fFalse );
            goto HandleError;
        }
    }
    else
    {
        CallS( err );

        Call( ErrIsamMakeKey(
                    ppib,
                    pfucbCatalog,
                    (BYTE *)&objidTable,
                    sizeof(objidTable),
                    JET_bitNewKey ) );
        Call( ErrIsamMakeKey(
                    ppib,
                    pfucbCatalog,
                    (BYTE *)&sysobj,
                    sizeof(sysobj),
                    JET_bitStrLimit ) );
        err = ErrIsamSetIndexRange( ppib, pfucbCatalog, JET_bitRangeUpperLimit );
        Assert( err <= 0 );
        while ( JET_errNoCurrentRecord != err )
        {
            IDXSEG  rgidxseg[JET_ccolKeyMost];
            IDXSEG  rgidxsegConditional[JET_ccolKeyMost];
            ULONG   cidxseg;
            ULONG   cidxsegConditional;
            BOOL    fPrimaryIndex;

            Call( err );

            Call( ErrCATIGetColumnsOfIndex(
                    ppib,
                    pfucbCatalog,
                    objidTable,
                    fTemplateTable,
                    ( objidNil != objidTemplateTable ? &tcibTemplateTable : NULL ),
                    &le_idxflag,
                    rgidxseg,
                    &cidxseg,
                    rgidxsegConditional,
                    &cidxsegConditional,
                    &fPrimaryIndex ) );

            if( !fPrimaryIndex )
            {
                Call( ErrCATIAddConditionalColumnsToIndex(
                    ppib,
                    pfucbCatalog,
                    pfucbShadowCatalog,
                    &le_idxflag,
                    rgidxseg,
                    cidxseg,
                    rgidxsegConditional,
                    cidxsegConditional,
                    rgidxsegToAdd,
                    cConditionalColumn ) );
            }

            err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
        }

        Assert( JET_errNoCurrentRecord == err );
        err = JET_errSuccess;
    }

    err = ErrDIRCommitTransaction( ppib, NO_GRBIT );

HandleError:
    if( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }
    if( pfucbNil != pfucbShadowCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbShadowCatalog ) );
    }
    if( err < 0 )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    return err;
}

//  ================================================================
LOCAL ERR ErrCATIDeleteMSUTable(
        IN PIB * const ppib,
        IN const IFMP ifmp )
//  ================================================================
{
    ERR     err             = JET_errSuccess;
    BOOL    fInTransaction  = fFalse;

    Assert( ppibNil != ppib );
    Assert( ifmpNil != ifmp );

    const JET_SESID sesid   = (JET_SESID)ppib;
    const JET_DBID  dbid    = (JET_DBID)ifmp;

    Assert( !fInTransaction );
    Call( ErrDIRBeginTransaction( ppib, 45797, NO_GRBIT ) );
    fInTransaction = fTrue;

    Call( ErrIsamDeleteTable( sesid, dbid, szMSU ) );

    Assert( fInTransaction );
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTransaction = fFalse;

HandleError:

    if( fInTransaction )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
        fInTransaction = fFalse;
    }

    OSTraceFMP(
        ifmp,
        JET_tracetagCatalog,
        OSFormat(
            "Session=[0x%p:0x%x] finished deleting MSU table '%s' on database=['%ws':0x%x] with error %d (0x%x)",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            szMSU,
            g_rgfmp[ifmp].WszDatabaseName(),
            (ULONG)ifmp,
            err,
            err ) );

    Assert( !fInTransaction );
    return err;
}

//  ================================================================
LOCAL ERR ErrCATIClearUnicodeFixupFlagsOnOneRecord(
        IN PIB * const ppib,
        IN FUCB * const pfucbCatalog,
        OUT BOOL * const pfReset )
//  ================================================================
{
    Assert( FHostIsLittleEndian() );

    ERR     err         = JET_errSuccess;
    BOOL    fInUpdate   = fFalse;

    *pfReset = fFalse;

    const JET_SESID     sesid       = (JET_SESID)ppib;
    const JET_TABLEID   tableid     = (JET_TABLEID)pfucbCatalog;
    const INT cretrievecolumn   = 2;
    INT iretrievecolumn         = 0;
    JET_RETRIEVECOLUMN  rgretrievecolumn[cretrievecolumn];

    LE_IDXFLAG  idxflag;
    SYSOBJ      sysobj;
    memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Type;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&sysobj;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( sysobj );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    rgretrievecolumn[iretrievecolumn].columnid      = fidMSO_Flags;
    rgretrievecolumn[iretrievecolumn].pvData        = (BYTE *)&idxflag;
    rgretrievecolumn[iretrievecolumn].cbData        = sizeof( idxflag );
    rgretrievecolumn[iretrievecolumn].itagSequence  = 1;
    ++iretrievecolumn;

    Assert( cretrievecolumn == iretrievecolumn );

    Call( ErrIsamRetrieveColumns(
        sesid,
        tableid,
        rgretrievecolumn,
        iretrievecolumn ) );
    CallS( err );   //  no null columns allowed

    if( sysobjIndex != sysobj
        || !FIDBUnicodeFixupOn_Deprecated( idxflag.fidb ) )
    {
        return JET_errSuccess;
    }

    //  this is a fixup-enabled index

    Assert( !fInUpdate );
    Call( ErrIsamPrepareUpdate( sesid, tableid, JET_prepReplace ) );
    fInUpdate = fTrue;

    idxflag.fidb = (IDBFLAG)(idxflag.fidb & ~fidbUnicodeFixupOn_Deprecated);
    Call( ErrIsamSetColumn( sesid, tableid, fidMSO_Flags, &idxflag, sizeof( idxflag ), NO_GRBIT, NULL ) );

    Assert( fInUpdate );
    Call( ErrIsamUpdate( sesid, tableid, NULL, 0, NULL, NO_GRBIT ) );
    fInUpdate = fFalse;
    *pfReset = fTrue;

HandleError:
    if( fInUpdate )
    {
        CallS( ErrIsamPrepareUpdate( sesid, tableid, JET_prepCancel ) );
        fInUpdate = fFalse;
    }

    Assert( !fInUpdate );
    return err;
}


//  ================================================================
LOCAL ERR ErrCATIClearUnicodeFixupFlagsOnAllIndexes(
        IN PIB * const ppib,
        IN FUCB * const pfucbCatalog,
        OUT QWORD * const pqwIndexesChanged )
//  ================================================================
{
    ERR err = JET_errSuccess;

    *pqwIndexesChanged = 0;

    for( err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT );
         JET_errSuccess == err;
         err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT ) )
    {
        BOOL fChanged = fFalse;

        const ERR errFixup = ErrCATIClearUnicodeFixupFlagsOnOneRecord( ppib, pfucbCatalog, &fChanged );

        //  There is no point in continuing.
        if ( errFixup < JET_errSuccess )
        {
            Assert( !fChanged );
            return errFixup;
        }

        if( fChanged )
        {
            ++(*pqwIndexesChanged);
        }
    }

    if( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }

    return err;
}



//  ================================================================
LOCAL ERR ErrCATIClearUnicodeFixupFlags(
        IN PIB * const ppib,
        IN const IFMP ifmp )
//  ================================================================
{
    ERR     err             = JET_errSuccess;
    BOOL    fInTransaction  = fFalse;
    FUCB *  pfucbCatalog    = pfucbNil;

    QWORD   qwIndexesChanged        =   0;
    QWORD   qwIndexesChangedShadow  =   0;

    Assert( !fInTransaction );
    Call( ErrDIRBeginTransaction( ppib, 62181, NO_GRBIT ) );
    fInTransaction = fTrue;

    Assert( pfucbNil == pfucbCatalog );
    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );

    Call( ErrCATIClearUnicodeFixupFlagsOnAllIndexes( ppib, pfucbCatalog, &qwIndexesChanged ) );

    Assert( pfucbNil != pfucbCatalog );
    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    Assert( pfucbNil == pfucbCatalog );
    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fTrue ) );

    Call( ErrCATIClearUnicodeFixupFlagsOnAllIndexes( ppib, pfucbCatalog, &qwIndexesChangedShadow ) );
    Assert( qwIndexesChanged == qwIndexesChangedShadow );

    Assert( pfucbNil != pfucbCatalog );
    Call( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;

    Assert( fInTransaction );
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTransaction = fFalse;

HandleError:
    if( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        pfucbCatalog = pfucbNil;
    }

    if( fInTransaction )
    {
        (void)ErrDIRRollback( ppib );
        fInTransaction = fFalse;
    }

    Assert( !fInTransaction );
    Assert( pfucbNil == pfucbCatalog );
    return err;
}


//  ================================================================
ERR ErrCATDeleteMSU(
        IN PIB * const ppib,
        IN const IFMP ifmp )
//  ================================================================
//
//  This removes the MSU table and resets the unicode-fixup-enabled
//  flag on all the indexes in the catalog (and shadow catalog)
//
//  o   Open the catalog
//  o   Reset the flags
//  o   Open the shadow catalog
//  o   Reset the flags
//  o   Delete the MSU table
//
//
//-
{
    ERR     err             = JET_errSuccess;
    BOOL    fDatabaseOpen   = fFalse;
    BOOL    fInTransaction  = fFalse;

    IFMP        ifmpT;

    Call( ErrDBOpenDatabase( ppib, g_rgfmp[ifmp].WszDatabaseName(), &ifmpT, NO_GRBIT ) );
    Assert( ifmp == ifmpT );
    fDatabaseOpen = fTrue;

    Assert( !fInTransaction );
    Call( ErrDIRBeginTransaction( ppib, 37605, NO_GRBIT ) );
    fInTransaction = fTrue;

    err = ErrCATIDeleteMSUTable( ppib, ifmp);
    if( JET_errObjectNotFound == err )
    {
        err = JET_errSuccess;
    }
    else
    {
        Call( err );

        // we are going through the catalog only if we have actually
        // found a MSU table and we have deleted it
        //
        Call( ErrCATIClearUnicodeFixupFlags( ppib, ifmp ) );
    }

    Assert( fInTransaction );
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTransaction = fFalse;

HandleError:
    if( fInTransaction )
    {
        (void)ErrDIRRollback( ppib );
        fInTransaction = fFalse;
    }
    if( fDatabaseOpen )
    {
        CallS( ErrDBCloseDatabase( ppib, ifmpT, NO_GRBIT ) );
    }

    Assert( !fInTransaction );
    return err;
}

// Enumerates all indices for a given table, including the primary.
ERR ErrCATScanTableIndices(
    _In_ PIB * const ppib,
    _In_ FUCB * const pfucbCatalog,
    _In_ OBJID  objidTable,
    _In_ ERR (* pfnVisitIndex) (VOID * const, char *),
    _In_ VOID * pvparam)
{
    ERR err = JET_errSuccess;
    OBJID objidNext = 0;
    OBJID objidCurrent = 0;
    DATA dataField;
    CHAR szIndex[ JET_cbNameMost + 1 ];
    BOOL fCatalogLatched = fFalse;

    while ( fTrue )
    {
        Call( ErrIsamMakeKey(
            ppib,
            pfucbCatalog,
            (BYTE *) &objidTable,
            sizeof( objidTable ),
            JET_bitNewKey ) );
        Call( ErrIsamMakeKey(
            ppib,
            pfucbCatalog,
            (BYTE *) &sysobjIndex,
            sizeof( sysobjIndex ),
            NO_GRBIT ) );
        Call( ErrIsamMakeKey(
            ppib,
            pfucbCatalog,
            (BYTE *) &objidNext,
            sizeof( objidNext ),
            JET_bitNil ) );

        err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekGE );
        OnDebug( const ERR errSeek = err );

        if ( JET_errRecordNotFound == err )
        {
            err = JET_errSuccess;
            break;
        }
        else
        {
            Call( err );
            Assert( JET_wrnSeekNotEqual == err || JET_errSuccess == err );
        }

        Call( ErrDIRGet( pfucbCatalog ) );
        fCatalogLatched = fTrue;

        // It may have been a JET_wrnSeekNotEqual. Must verify that the objid/sysobj matches.

        OBJID objidTableFound;
        Call( ErrRECIRetrieveFixedColumn(
            pfcbNil,
            pfucbCatalog->u.pfcb->Ptdb(),
            fidMSO_ObjidTable,
            pfucbCatalog->kdfCurr.data,
            &dataField ) );
        Assert( dataField.Cb() == sizeof( OBJID ) );
        UtilMemCpy( &objidTableFound, dataField.Pv(), sizeof( OBJID ) );

        SYSOBJ sysobjFound;
        Call( ErrRECIRetrieveFixedColumn(
            pfcbNil,
            pfucbCatalog->u.pfcb->Ptdb(),
            fidMSO_Type,
            pfucbCatalog->kdfCurr.data,
            &dataField ) );
        Assert( dataField.Cb() == sizeof( sysobjFound ) );
        UtilMemCpy( &sysobjFound, dataField.Pv(), sizeof( sysobjFound ) );

#ifdef DEBUG
        if ( JET_errSuccess == errSeek )
        {
            Assert( objidTable == objidTableFound );
            Assert( sysobjIndex == sysobjFound );
        }

#endif
        if ( objidTable != objidTableFound ||
             sysobjIndex != sysobjFound )
        {
            Assert( JET_wrnSeekNotEqual == errSeek );
            // This is not an index. We are not interested in it.
            break;
        }

        Call( ErrRECIRetrieveFixedColumn(
            pfcbNil,
            pfucbCatalog->u.pfcb->Ptdb(),
            fidMSO_Id,
            pfucbCatalog->kdfCurr.data,
            &dataField ) );
        Assert( dataField.Cb() == sizeof(OBJID) );
        UtilMemCpy( &objidCurrent, dataField.Pv(), sizeof(OBJID) );

        if ( objidCurrent < objidNext )
        {
            break;
        }
        else
        {
            objidNext = objidCurrent + 1;
        }

        Assert( fidMSO_Name.FVar() );
        Call( ErrRECIRetrieveVarColumn(
            pfcbNil,
            pfucbCatalog->u.pfcb->Ptdb(),
            fidMSO_Name,
            pfucbCatalog->kdfCurr.data,
            &dataField ) );

        const INT cbDataField = dataField.Cb();
        if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
        {
            AssertSz( fFalse, "Invalid fidMSO_Name column in catalog. cbDataField = %d.", cbDataField );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }

        Assert( sizeof( szIndex ) > cbDataField );
        UtilMemCpy( szIndex, dataField.Pv(), min( sizeof( szIndex ), cbDataField ) );
        szIndex[ cbDataField ] = '\0';

        CallS( ErrDIRRelease( pfucbCatalog ) );
        fCatalogLatched = fFalse;

        Call( pfnVisitIndex( pvparam, szIndex ) );
    }
HandleError:
    if ( fCatalogLatched )
    {
        CallS( ErrDIRRelease( pfucbCatalog ) );
    }

    return err;
}

// Try seeking in the catalog for the specified objid at the specified FDP,
// trying with different sysobj-types.
// If it's found, set *psysobjType.
// If it's an index, also set szIndex.
ERR ErrCATISeekTableType(
    _In_ PIB            *ppib,
    _In_ FUCB           *pfucbCatalog,
    _In_ const OBJID    objidTable,
    _In_ const OBJID    objidFDP,
    _Out_ SYSOBJ        *psysobjType,
    _Out_writes_z_( cbIndex ) CHAR  *szIndex,
    _In_ const ULONG    cbIndex)
{
    ERR err;
    DATA dataField;
    BOOL fCatalogLatched = fFalse;

    szIndex[ 0 ] = '\0';

    //  should be on the primary index
    Assert( pfucbNil != pfucbCatalog );
    Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    // Try sysobjTable first
    *psysobjType = sysobjTable;
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *) &objidTable,
                sizeof( objidTable ),
                JET_bitNewKey ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *) psysobjType,
                sizeof( SYSOBJ ),
                JET_bitNil ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *) &objidFDP,
                sizeof( objidFDP ),
                JET_bitNil ) );

    err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );
    if ( JET_errSuccess == err )
    {
        goto HandleError;
    }

#ifdef ENABLE_OLD2_DEFRAG_LV
    // Try sysobjLongValue
    *psysobjType = sysobjLongValue;
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *) &objidTable,
                sizeof( objidTable ),
                JET_bitNewKey ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *) psysobjType,
                sizeof( SYSOBJ ),
                JET_bitNil ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *) &objidFDP,
                sizeof( objidFDP ),
                JET_bitNil ) );

    err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );
    if ( JET_errSuccess == err )
    {
        goto HandleError;
    }
#endif

    // Try sysobjIndex
    *psysobjType = sysobjIndex;
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *) &objidTable,
                sizeof( objidTable ),
                JET_bitNewKey ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *) psysobjType,
                sizeof( SYSOBJ ),
                JET_bitNil ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbCatalog,
                (BYTE *) &objidFDP,
                sizeof( objidFDP ),
                JET_bitNil ) );
    err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );

    if ( JET_errSuccess == err )
    {
        Call( ErrDIRGet( pfucbCatalog ) );
        fCatalogLatched = fTrue;

        Assert( fidMSO_Name.FVar() );
        Call( ErrRECIRetrieveVarColumn(
            pfcbNil,
            pfucbCatalog->u.pfcb->Ptdb(),
            fidMSO_Name,
            pfucbCatalog->kdfCurr.data,
            &dataField ) );

        const ULONG cbDataField = dataField.Cb();
        if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
        {
            AssertSz( fFalse, "Invalid fidMSO_Name column in catalog." );
            Error( ErrERRCheck( JET_errCatalogCorrupted ) );
        }

        Assert( cbIndex > cbDataField );
        if ( cbIndex <= cbDataField )
        {
            Error( ErrERRCheck( JET_errBufferTooSmall ) );
        }

        UtilMemCpy( szIndex, dataField.Pv(), cbDataField );
        szIndex[ cbDataField ] = '\0';

        CallS( ErrDIRRelease( pfucbCatalog ) );
        fCatalogLatched = fFalse;
    }

HandleError:
    if ( JET_errRecordNotFound == err )
    {
        err = ErrERRCheck( JET_errObjectNotFound );
    }
    if ( fCatalogLatched )
    {
        CallS( ErrDIRRelease( pfucbCatalog ) );
    }

    return err;
}

static const JET_COLUMNID columnidMSObjids_objid        = fidTaggedLeast;
static const JET_COLUMNID columnidMSObjids_objidTable   = fidTaggedLeast + 1;
static const JET_COLUMNID columnidMSObjids_type         = fidTaggedLeast + 2;

static const CHAR szMSObjidIndex[]      = "primary";
static const CHAR szMSObjidIndexKey[]   = "+objid\0";

//  ================================================================
struct MSObjidInfo
//  ================================================================
{
    MSObjidInfo() { tableid = JET_tableidNil; }
    ~MSObjidInfo() { Assert( JET_tableidNil == tableid ); }

    JET_TABLEID tableid;
    JET_COLUMNID columnidObjid;
    JET_COLUMNID columnidObjidTable;
    JET_COLUMNID columnidType;
};

//  ================================================================
ERR ErrCATIOpenMSObjids(
        const JET_SESID sesid,
        const JET_DBID dbid,
        __out MSObjidInfo * const pmsoInfo )
//  ================================================================
{
    ERR err;

    pmsoInfo->tableid = JET_tableidNil;
    Call( ErrIsamOpenTable( sesid, dbid, &(pmsoInfo->tableid), szMSObjids, NO_GRBIT ) );

    pmsoInfo->columnidObjid = columnidMSObjids_objid;
    pmsoInfo->columnidObjidTable = columnidMSObjids_objidTable;
    pmsoInfo->columnidType = columnidMSObjids_type;

HandleError:
    return err;
}

//  ================================================================
void CATICloseMSObjids(
        const JET_SESID sesid,
        __inout MSObjidInfo * const pmsoInfo )
//  ================================================================
{
    if ( JET_tableidNil != pmsoInfo->tableid )
    {
        CallS( ErrDispCloseTable( sesid, pmsoInfo->tableid ) );
        pmsoInfo->tableid = JET_tableidNil;
    }
}

//  ================================================================
template<class T>
ERR ErrCATIRetrieveColumn(
        const JET_SESID sesid,
        const JET_TABLEID tableid,
        const JET_COLUMNID columnid,
        T * const pValue )
//  ================================================================
{
    ULONG cbActual;
    *pValue = T();
    const ERR err = ErrDispRetrieveColumn(
            sesid,
            tableid,
            columnid,
            pValue,
            sizeof( T ),
            &cbActual,
            NO_GRBIT,
            NULL );
    Assert( cbActual == sizeof( T ) || JET_errSuccess != err );
    return err;
}

//  ================================================================
template<class T>
ERR ErrCATISetColumn(
        const JET_SESID sesid,
        const JET_TABLEID tableid,
        const JET_COLUMNID columnid,
        const T value )
//  ================================================================
{
    return ErrDispSetColumn(
            sesid,
            tableid,
            columnid,
            &value,
            sizeof( T ),
            NO_GRBIT,
            NULL );
}

//  ================================================================
template<class T>
ERR ErrCATISeek(
        const JET_SESID sesid,
        const JET_TABLEID tableid,
        const T key )
//  ================================================================
{
    ERR err;
    Call( ErrDispMakeKey( sesid, tableid, &key, sizeof( T ), JET_bitNewKey ) );
    Call( ErrDispSeek( sesid, tableid, JET_bitSeekEQ ) );

HandleError:
    return err;
}

//  ================================================================
ERR ErrCATIInsertMSObjidsRecord(
        const JET_SESID sesid,
        const MSObjidInfo& msoInfo,
        const OBJID objid,
        const OBJID objidTable,
        const SYSOBJ sysobj )
//  ================================================================
{
    Assert( objidNil != objid );
    Assert( objidNil != objidTable );
    Assert( sysobjColumn != sysobj );
    Assert( sysobjCallback != sysobj );
    Assert( objid >= objidTable );

    ERR err;
    bool fInUpdate = fFalse;

    // The record is a b-tree. Create an MSObjids entry
    Call( ErrDispPrepareUpdate( sesid, (JET_TABLEID) msoInfo.tableid, JET_prepInsert ) );
    fInUpdate = fTrue;
    Call( ErrCATISetColumn( sesid, msoInfo.tableid, msoInfo.columnidType, sysobj ) );
    Call( ErrCATISetColumn( sesid, msoInfo.tableid, msoInfo.columnidObjid, objid ) );
    Call( ErrCATISetColumn( sesid, msoInfo.tableid, msoInfo.columnidObjidTable, objidTable ) );
    Call( ErrDispUpdate( sesid, msoInfo.tableid, NULL, 0, NULL, NO_GRBIT ) );
    fInUpdate = fFalse;

HandleError:
    if( fInUpdate )
    {
        (void) ErrDispPrepareUpdate( sesid, msoInfo.tableid, JET_prepCancel );
    }

    return err;
}

//  ================================================================
ERR ErrCATIIsBTree(
        const JET_SESID sesid,
        const JET_TABLEID tableidCatalog,
        __out bool * const pfIsBTree )
//  ================================================================
//
//  Determine if the current catalog record is a b-tree
//
//-
{
    ERR err;

    SYSOBJ sysobj;
    Call( ErrCATIRetrieveColumn( sesid, tableidCatalog, fidMSO_Type, &sysobj ) );
    *pfIsBTree = ( sysobjTable == sysobj || sysobjIndex == sysobj || sysobjLongValue == sysobj );

HandleError:
    return err;
}

//  ================================================================
ERR ErrCATITryInsertMSObjidsRecord(
        const JET_SESID sesid,
        const JET_TABLEID tableidCatalog,
        const MSObjidInfo& msoInfo)
//  ================================================================
{
    ERR err;

    // Retrieve and check the type

    bool fIsBTree;
    Call( ErrCATIIsBTree( sesid, tableidCatalog, &fIsBTree ) );
    if ( !fIsBTree )
    {
        goto HandleError;
    }

    OBJID objid;
    OBJID objidTable;
    SYSOBJ sysobj;
    Call( ErrCATIRetrieveColumn( sesid, tableidCatalog, fidMSO_Id, &objid ) );
    Call( ErrCATIRetrieveColumn( sesid, tableidCatalog, fidMSO_ObjidTable, &objidTable ) );
    Call( ErrCATIRetrieveColumn( sesid, tableidCatalog, fidMSO_Type, &sysobj ) );

    Call( ErrCATIInsertMSObjidsRecord( sesid, msoInfo, objid, objidTable, sysobj ) );

HandleError:
    if ( JET_errKeyDuplicate == err )
    {
        // This error is expected as we might be resuming a build of MSObjids
        err = JET_errSuccess;
    }

    return err;
}

//  ================================================================
ERR ErrCATIInsertMSObjidsRecordsForOneTable(
        const JET_SESID sesid,
        const JET_TABLEID tableidCatalog,
        const MSObjidInfo& msoInfo )
//  ================================================================
//
//  All records for one table are inserted in the same transaction.
//
//-
{
    ERR err;
    bool fInTransaction = false;

    OBJID objidTable;
    Call( ErrCATIRetrieveColumn( sesid, tableidCatalog, fidMSO_ObjidTable, &objidTable ) );

    Assert( !fInTransaction );
    Call( ErrIsamBeginTransaction( sesid, 38299, NO_GRBIT ) );
    fInTransaction = fTrue;

    while( true )
    {
        OBJID objidTableCurr;
        Call( ErrCATIRetrieveColumn( sesid, tableidCatalog, fidMSO_ObjidTable, &objidTableCurr ) );

        Assert( objidTableCurr >= objidTable );
        if ( objidTable != objidTableCurr )
        {
            // We are on the first record of a new table
            break;
        }

        Call( ErrCATITryInsertMSObjidsRecord( sesid, tableidCatalog, msoInfo ) );

        err = ErrIsamMove( sesid, tableidCatalog, JET_MoveNext, NO_GRBIT );
        if ( JET_errNoCurrentRecord == err )
        {
            // End of the catalog
            break;
        }
        Call( err );
    }

    Call( ErrIsamCommitTransaction( sesid, JET_bitCommitLazyFlush ) );
    fInTransaction = fFalse;

HandleError:
    if( fInTransaction )
    {
        (void) ErrIsamRollback( sesid, NO_GRBIT );
    }

    return err;
}

//  ================================================================
ERR ErrCATIPopulateMSObjids(
        const JET_SESID sesid,
        const JET_TABLEID tableidCatalog,
        const MSObjidInfo& msoInfo )
//  ================================================================
{
    ERR err;

    Call( ErrIsamMove( sesid, tableidCatalog, JET_MoveFirst, NO_GRBIT ) );

    // Insert records one table at a time
    // To avoid a super-long transaction we insert records for a table at a time
    while( JET_errNoCurrentRecord != ( err = ErrCATIInsertMSObjidsRecordsForOneTable( sesid, tableidCatalog, msoInfo ) ) )
    {
        Call( err );
        Assert( ((PIB*) sesid)->Level() == 0 );
    }

    Call( ErrIsamCommitTransaction( sesid, JET_bitWaitLastLevel0Commit ) );

    err = JET_errSuccess;

HandleError:
    return err;
}

//  ================================================================
ERR ErrCATIRetrieveMSObjidRecord(
        const JET_SESID sesid,
        const MSObjidInfo& msoInfo,
        __out OBJID * pobjid,
        __out OBJID * pobjidTable,
        __out SYSOBJ * psysobj )
//  ================================================================
{
    ERR err;

    Call( ErrCATIRetrieveColumn( sesid, msoInfo.tableid, msoInfo.columnidObjid, pobjid ) );
    Call( ErrCATIRetrieveColumn( sesid, msoInfo.tableid, msoInfo.columnidObjidTable, pobjidTable ) );
    Call( ErrCATIRetrieveColumn( sesid, msoInfo.tableid, msoInfo.columnidType, psysobj ) );

HandleError:
    return err;
}

//  ================================================================
ERR ErrCATIDumpMSObjidRecord(
        const JET_SESID sesid,
        const MSObjidInfo& msoInfo,
              CPRINTF * const pcprintf )
//  ================================================================
{
    ERR err;

    OBJID objid;
    OBJID objidTable;
    SYSOBJ sysobj;
    Call( ErrCATIRetrieveMSObjidRecord( sesid, msoInfo, &objid, &objidTable, &sysobj ) );
    (*pcprintf)( "objid: %d, objidTable: %d, type: 0x%x\n", objid, objidTable, sysobj );

HandleError:
    return err;
}

//  ================================================================
ERR ErrCATICompareMSObjidRecords(
        const JET_SESID sesid,
        const MSObjidInfo& msoInfo1,
        const MSObjidInfo& msoInfo2,
              CPRINTF * const pcprintfError )
//  ================================================================
{
    ERR err;

    OBJID objid1, objid2;
    OBJID objidTable1, objidTable2;
    SYSOBJ sysobj1, sysobj2;

    Call( ErrCATIRetrieveMSObjidRecord( sesid, msoInfo1, &objid1, &objidTable1, &sysobj1 ) );
    Call( ErrCATIRetrieveMSObjidRecord( sesid, msoInfo2, &objid2, &objidTable2, &sysobj2 ) );

    if ( objid1 != objid2
         || objidTable1 != objidTable2
         || sysobj1 != sysobj2 )
    {
        (*pcprintfError)( "Record mismatch:\n" );
        (*pcprintfError)( "\t" );
        Call( ErrCATIDumpMSObjidRecord( sesid, msoInfo1, pcprintfError ) );
        (*pcprintfError)( "\t" );
        Call( ErrCATIDumpMSObjidRecord( sesid, msoInfo2, pcprintfError ) );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

HandleError:
    return err;
}

//  ================================================================
ERR ErrCATICompareMSObjids(
        const JET_SESID sesid,
        const MSObjidInfo& msoInfoOriginal,
        const MSObjidInfo& msoInfoTemp,
              CPRINTF * const pcprintfError )
//  ================================================================
{
    ERR err;

    Call( ErrDispMove( sesid, msoInfoOriginal.tableid, JET_MoveFirst, NO_GRBIT ) );
    Call( ErrDispMove( sesid, msoInfoTemp.tableid, JET_MoveFirst, NO_GRBIT ) );

    while( true )
    {
        Call( ErrCATICompareMSObjidRecords( sesid, msoInfoOriginal, msoInfoTemp, pcprintfError ) );

        const ERR errOriginal = ErrDispMove( sesid, (JET_TABLEID) msoInfoOriginal.tableid, JET_MoveNext, NO_GRBIT );
        const ERR err2 = ErrDispMove( sesid, (JET_TABLEID) msoInfoTemp.tableid, JET_MoveNext, NO_GRBIT );

        if ( JET_errNoCurrentRecord == errOriginal && JET_errSuccess == err2 )
        {
            (*pcprintfError)( "Not enough records. First missing record: " );
            Call( ErrCATIDumpMSObjidRecord( sesid, msoInfoTemp, pcprintfError ) );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        else if ( JET_errNoCurrentRecord == err2 && JET_errSuccess == errOriginal )
        {
            (*pcprintfError)( "Too many records. First extra record: " );
            Call( ErrCATIDumpMSObjidRecord( sesid, msoInfoOriginal, pcprintfError ) );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        else if( errOriginal != err2 )
        {
            (*pcprintfError)("ErrCATICompareMSObjids failed on move next: %d, %d\n", errOriginal, err2);
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        else if (JET_errNoCurrentRecord == errOriginal)
        {
            break;
        }
        Call( err );
    }

    err = JET_errSuccess;

HandleError:
    return err;
}

//  ================================================================
ERR ErrCATCheckMSObjidsReady(
        __in PIB * const ppib,
        const IFMP ifmp,
        __out BOOL * const pfReady )
//  ================================================================
{
    Assert( !g_fRepair );
    Assert( ppib != ppibNil );
    Assert( ifmp != ifmpNil );
    Assert( pfReady != NULL );

    ERR err = JET_errSuccess;
    JET_DBID dbid = (JET_DBID)ifmp;
    JET_SESID sesid = (JET_SESID)ppib;
    JET_TABLEID tableidCatalog = JET_tableidNil;
    MSObjidInfo msoInfo;
    OBJID objidTable = objidNil;

    *pfReady = fFalse;

    // Open catalog.
    Call( ErrIsamOpenTable( sesid, dbid, &tableidCatalog, szMSO, NO_GRBIT ) );

    // Try to open MSysObjids.
    err = ErrCATIOpenMSObjids( sesid, dbid, &msoInfo );
    if ( err == JET_errObjectNotFound )
    {
        err = JET_errSuccess;
        goto HandleError;
    }
    Call( err );

    // Check if the greatest objid in the catalog is present in MSysObjids.
    Call( ErrDispMove( sesid, tableidCatalog, JET_MoveLast, NO_GRBIT ) );
    Call( ErrCATIRetrieveColumn( sesid, tableidCatalog, fidMSO_ObjidTable, &objidTable ) );
    err = ErrCATISeek( sesid, msoInfo.tableid, objidTable );
    if ( err == JET_errSuccess )
    {
        *pfReady = fTrue;
        goto HandleError;
    }
    else if ( err == JET_errRecordNotFound )
    {
        err = JET_errSuccess;
        goto HandleError;
    }
    Call( err );

    err = JET_errSuccess;

HandleError:
    if ( tableidCatalog != JET_tableidNil )
    {
        CallS( ErrDispCloseTable( sesid, tableidCatalog ) );
        tableidCatalog = JET_tableidNil;
    }

    CATICloseMSObjids( sesid, &msoInfo );

    return err;
}

//  ================================================================
ERR ErrCATCreateMSObjids(
        __in PIB * const ppib,
        const IFMP ifmp,
        PGNO * const ppgnoFDP,
        OBJID * const pobjidFDP )
//  ================================================================
{
    Assert( !g_fRepair );
    Assert( ppibNil != ppib );
    Assert( ifmpNil != ifmp );

    ERR                 err             = JET_errSuccess;
    BOOL                fInTransaction  = fFalse;

    JET_COLUMNCREATE_A  rgcolumncreateMSObjids[] = {
//      { cbStruct,                   szColumn,     coltyp,               cbMax, grbit,               pvDefault, cbDefault, cp, columnid, err            }
        { sizeof(JET_COLUMNCREATE_A), "objid",      JET_coltypLong,       4,     JET_bitColumnTagged, NULL,      0,         0,  0,        JET_errSuccess },
        { sizeof(JET_COLUMNCREATE_A), "objidTable", JET_coltypLong,       4,     JET_bitColumnTagged, NULL,      0,         0,  0,        JET_errSuccess },
        { sizeof(JET_COLUMNCREATE_A), "type",       JET_coltypShort,      2,     JET_bitColumnTagged, NULL,      0,         0,  0,        JET_errSuccess },
    };

    JET_INDEXCREATE3_A  rgindexcreateMSObjids[] = {
        {
            sizeof( JET_INDEXCREATE3_A ),                   // size of this structure
            const_cast<char *>( szMSObjidIndex ),           // index name
            const_cast<char *>( szMSObjidIndexKey ),        // index key
            sizeof( szMSObjidIndexKey ),                    // length of key
            JET_bitIndexPrimary,                            // index options
            100,                                            // index density
            NULL,                                           // pidxunicode2 for the index
            0,                                              // maximum length of variable length columns in index key
            NULL,                                           // pointer to conditional column structure
            0,                                              // number of conditional columns
            JET_errSuccess,                                 // returned error code,
            255,                                            // maximum key size
            NULL                                            // space hints
        },
    };

    JET_TABLECREATE5_A  tablecreateMSObjids = {
        sizeof( JET_TABLECREATE5_A ),       // size of this structure
        const_cast<char *>( szMSObjids ),   // name of table
        NULL,                               // name of base table
        1,                                  // initial pages
        100,                                // density
        rgcolumncreateMSObjids,             // columns to create
        _countof(rgcolumncreateMSObjids),   // number of columns to create
        rgindexcreateMSObjids,              // array of index creation info
        _countof(rgindexcreateMSObjids),    // number of indexes to create
        NULL,                               // callback to use for this table
        JET_cbtypNull,                      // when the callback should be called
        JET_bitTableCreateSystemTable | JET_bitTableCreateFixedDDL,     // grbit
        NULL,                               // Sequential index space hints.
        NULL,                               // LV index space hints
        0,                                  // cbSeparateLV threshold
        0,                                  // cbLVChunkMax
        JET_TABLEID( pfucbNil ),            // returned tableid
        0,                                  // returned count of objects created
    };

    const JET_DBID  dbid    = (JET_DBID)ifmp;

    Assert( !fInTransaction );
    Call( ErrDIRBeginTransaction( ppib, 51941, NO_GRBIT ) );
    fInTransaction = fTrue;

    Call( ErrFILECreateTable( ppib, dbid, &tablecreateMSObjids ) );

    Assert( columnidMSObjids_objid      == rgcolumncreateMSObjids[0].columnid );
    Assert( columnidMSObjids_objidTable == rgcolumncreateMSObjids[1].columnid );
    Assert( columnidMSObjids_type       == rgcolumncreateMSObjids[2].columnid );

    if ( NULL != ppgnoFDP )
    {
        *ppgnoFDP = PgnoFDP( (FUCB *)tablecreateMSObjids.tableid );
    }

    if ( NULL != pobjidFDP )
    {
        *pobjidFDP = ObjidFDP( (FUCB *)tablecreateMSObjids.tableid );
    }

    Call( ErrFILECloseTable( ppib, (FUCB *)tablecreateMSObjids.tableid ) );

    Assert( fInTransaction );
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTransaction = fFalse;

HandleError:

    if( fInTransaction )
    {
        const ERR errRollback = ErrDIRRollback( ppib );
        CallSx( errRollback, JET_errRollbackError );
        Assert( errRollback >= JET_errSuccess || PinstFromPpib( ppib )->FInstanceUnavailable( ) );
        fInTransaction = fFalse;
    }

    OSTraceFMP(
        ifmp,
        JET_tracetagCatalog,
        OSFormat(
            "Session=[0x%p:0x%x] finished creating MSObjids table '%s' on database=['%ws':0x%x] with error %d (0x%x)",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            szMSObjids,
            g_rgfmp[ifmp].WszDatabaseName(),
            (ULONG)ifmp,
            err,
            err ) );

    Assert( !fInTransaction );
    return err;
}

//  ================================================================
ERR ErrCATDeleteMSObjids(
        _In_ PIB * const ppib,
        _In_ const IFMP ifmp )
//  ================================================================
{
    ERR     err             = JET_errSuccess;
    BOOL    fDatabaseOpen   = fFalse;
    BOOL    fInTransaction  = fFalse;

    IFMP        ifmpT;

    Call( ErrDBOpenDatabase( ppib, g_rgfmp[ifmp].WszDatabaseName(), &ifmpT, NO_GRBIT ) );
    Assert( ifmp == ifmpT );
    fDatabaseOpen = fTrue;

    Assert( !fInTransaction );
    Call( ErrDIRBeginTransaction( ppib, 58779, NO_GRBIT ) );
    fInTransaction = fTrue;

    err = ErrIsamDeleteTable( (JET_SESID)ppib, (JET_DBID)ifmp, szMSObjids, fTrue );
    if( JET_errObjectNotFound == err )
    {
        err = JET_errSuccess;
    }
    else
    {
        Call( err );
    }

    Assert( fInTransaction );
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTransaction = fFalse;

HandleError:
    if( fInTransaction )
    {
        (void)ErrDIRRollback( ppib );
        fInTransaction = fFalse;
    }
    if( fDatabaseOpen )
    {
        CallS( ErrDBCloseDatabase( ppib, ifmpT, NO_GRBIT ) );
    }

    Assert( !fInTransaction );
    return err;
}

//  ================================================================
ERR ErrCATPopulateMSObjids(
        __in PIB * const ppib,
        const IFMP ifmp )
//  ================================================================
//
//  (This function must be called *after* MSObjids is created)
//
//  Populates MSObjids with information from the catalog. After this
//  function completes it is safe to set FMaintainMSObjids in the FMP.
//
//  To avoid version store problems we add information for each table
//  in a separate transaction. That means that this function may be
//  called when MSObjids is partially populated so we check to see if
//  the last table in the catalog is present in MSObjids.
//
//-
{
    ERR err;
    IFMP ifmpT = ifmpNil;
    const JET_SESID sesid = (JET_SESID) ppib;
    JET_TABLEID tableidCatalog = JET_tableidNil;

    MSObjidInfo msoInfo;

    Call( ErrDBOpenDatabase( ppib, g_rgfmp[ ifmp ].WszDatabaseName(), &ifmpT, NO_GRBIT ) );
    Assert( ifmp == ifmpT );

    if( JET_wrnFileOpenReadOnly == err )
    {
        //  we have attached to a read-only file, but JET_bitDbReadOnly was not specified
        //  no-one will be modifying the database so it doesn't matter if MSObjids exists or not
        err = JET_errSuccess;
    }
    else
    {
        Call( err );

        BOOL fMSysObjidsReady = fFalse;
        Call( ErrCATCheckMSObjidsReady( ppib, ifmp, &fMSysObjidsReady ) );
        if ( fMSysObjidsReady )
        {
            // MSObjids is fully populated
            err = JET_errSuccess;
            goto HandleError;
        }

        Call( ErrIsamOpenTable( sesid, (JET_DBID) ifmp, &tableidCatalog, szMSO, NO_GRBIT ) );
        Call( ErrCATIOpenMSObjids( sesid, (JET_DBID) ifmp, &msoInfo ) );

        Call( ErrCATIPopulateMSObjids( sesid, tableidCatalog, msoInfo ) );
    }

HandleError:
    if ( JET_tableidNil != tableidCatalog )
    {
        CallS( ErrDispCloseTable( sesid, tableidCatalog ) );
    }
    CATICloseMSObjids( sesid, &msoInfo );

    if ( ifmpNil != ifmpT )
    {
        CallS( ErrDBCloseDatabase( ppib, ifmpT, 0 ) );
    }

    OSTraceFMP(
        ifmp,
        JET_tracetagCatalog,
        OSFormat(
            "Session=[0x%p:0x%x] populated MSObjids table '%s' on database=['%ws':0x%x] with error %d (0x%x)",
            ppib,
            ppib->trxBegin0,
            szMSObjids,
            g_rgfmp[ifmp].WszDatabaseName(),
            (ULONG)ifmp,
            err,
            err ) );

    return err;
}

//  ================================================================
ERR ErrCATInsertMSObjidsRecord(
        __in PIB * const ppib,
        const IFMP ifmp,
        const OBJID objid,
        const OBJID objidTable,
        const SYSOBJ sysobj )
//  ================================================================
{
    ERR err;
    const JET_SESID sesid = (JET_SESID) ppib;
    const JET_DBID dbid = (JET_DBID) ifmp;

    if ( !g_rgfmp[ ifmp ].FMaintainMSObjids() )
    {
        return JET_errSuccess;
    }

    MSObjidInfo msoInfo;
    Call( ErrCATIOpenMSObjids( sesid, dbid, &msoInfo ) );
    Call( ErrCATIInsertMSObjidsRecord( sesid, msoInfo, objid, objidTable, sysobj ) );

HandleError:
    CATICloseMSObjids( sesid, &msoInfo );
    return err;
}

//  ================================================================
ERR ErrCATDeleteMSObjidsRecord(
    __in PIB * const ppib,
    const IFMP ifmp,
    const OBJID objid )
//  ================================================================
{
    ERR err;
    const JET_SESID sesid = (JET_SESID) ppib;
    const JET_DBID dbid = (JET_DBID) ifmp;
    MSObjidInfo msoInfo;

    if ( !g_rgfmp[ ifmp ].FMaintainMSObjids() )
    {
        return JET_errSuccess;
    }

    Call( ErrCATIOpenMSObjids( sesid, dbid, &msoInfo ) );
    Call( ErrCATISeek( sesid, msoInfo.tableid, objid ) );
    Call( ErrDispDelete( sesid, msoInfo.tableid ) );

HandleError:
    CATICloseMSObjids( sesid, &msoInfo );
    return err;
}

//  ================================================================
ERR ErrCATPossiblyDeleteMSObjidsRecord(
        __in PIB * const ppib,
        FUCB * const pfucbCatalog )
//  ================================================================
//
//  Determine if the current catalog record is a b-tree and if so
//  delete the corresponding entry in MSObjids
//
//-
{
    Assert( ppib->Level() > 0 );

    ERR err;
    const JET_SESID sesid = (JET_SESID) ppib;
    const JET_TABLEID tableidCatalog = (JET_TABLEID) pfucbCatalog;

    SYSOBJ sysobj;
    ULONG cbActual;
    Call( ErrIsamRetrieveColumn(
            sesid,
            tableidCatalog,
            fidMSO_Type,
            &sysobj,
            sizeof( sysobj ),
            &cbActual,
            NO_GRBIT,
            NULL ) );
    Assert( sizeof( sysobj ) == cbActual );

    OBJID objid;
    Call( ErrIsamRetrieveColumn(
            sesid,
            tableidCatalog,
            fidMSO_Id,
            &objid,
            sizeof( objid ),
            &cbActual,
            NO_GRBIT,
            NULL ) );
    Assert( sizeof( objid ) == cbActual );

    OBJID objidTable;
    Call( ErrIsamRetrieveColumn(
            sesid,
            tableidCatalog,
            fidMSO_ObjidTable,
            &objidTable,
            sizeof( objidTable ),
            &cbActual,
            NO_GRBIT,
            NULL ) );
    Assert( sizeof( objid ) == cbActual );

    const bool fIsBTree = ( sysobjTable == sysobj || sysobjIndex == sysobj || sysobjLongValue == sysobj );
    const bool fIsPrimaryIndex = ( sysobjIndex == sysobj ) && ( objidTable == objid );

    // Don't delete the primary index, the MSObjids index will be removed when the table record is deleted
    if ( fIsBTree && !fIsPrimaryIndex )
    {
        Call( ErrCATDeleteMSObjidsRecord( ppib, pfucbCatalog->ifmp, objid ) );
        CATResetExtentPageCounts( ppib, pfucbCatalog->ifmp, objid );
    }

HandleError:
    return err;
}

//  ================================================================
//  Given an arbitrary objid, retrieve the objid of the parent table
//  (objidTable) from the MSysObjid table, as well as its type.
//  If objid is already a table, then objidTable will be the same.
//  This function returns JET_errSuccess even if the object has not been
//  found, in which case the returned *psysobj will be sysobjNil.
ERR ErrCATGetObjidMetadata(
    _In_ PIB* const     ppib,
    _In_ const IFMP     ifmp,
    _In_ const OBJID    objid,
    _Out_opt_ OBJID     *pobjidTable,
    _Out_opt_ SYSOBJ    *psysobj )
//  ================================================================
{
    ERR err = JET_errSuccess;
    JET_SESID sesid = (JET_SESID)ppib;
    MSObjidInfo msoInfo;

    Assert( pobjidTable != NULL );
    Assert( psysobj != NULL );

    *pobjidTable = objidNil;
    *psysobj = sysobjNil;

    err = ErrCATIOpenMSObjids( sesid, (JET_DBID)ifmp, &msoInfo );
    Expected( err != JET_errObjectNotFound );
    Call( err );

    err = ErrCATISeek( sesid, msoInfo.tableid, objid );
    if ( ( err == JET_errNoCurrentRecord ) || ( err == JET_errRecordNotFound ) )
    {
        err = JET_errSuccess;
        goto HandleError;
    }
    Call( err );

    // Get parent's objid and the object's type.
    Call( ErrCATIRetrieveColumn( sesid, msoInfo.tableid, msoInfo.columnidObjidTable, pobjidTable ) );

    // Get object's type.
    Call( ErrCATIRetrieveColumn( sesid, msoInfo.tableid, msoInfo.columnidType, psysobj ) );
    Assert( ( *psysobj == sysobjTable ) || ( *psysobj == sysobjIndex ) || ( *psysobj == sysobjLongValue ) );

    Assert( ( *psysobj == sysobjTable ) || ( objid != *pobjidTable ) );
    Assert( ( *psysobj != sysobjTable ) || ( objid == *pobjidTable ) );

HandleError:
    // Just in case.
    if ( err < JET_errSuccess )
    {
        *pobjidTable = objidNil;
        *psysobj = sysobjNil;
    }

    CATICloseMSObjids( sesid, &msoInfo );

    return err;
}

static const JET_COLUMNID columnidMSExtentPageCountCache_objid      = fidFixedLeast;
static const JET_COLUMNID columnidMSExtentPageCountCache_cpgAE      = fidFixedLeast + 1;
static const JET_COLUMNID columnidMSExtentPageCountCache_cpgOE      = fidFixedLeast + 2;
static const JET_COLUMNID columnidMSExtentPageCountCache_bFlag      = fidFixedLeast + 3;

static const CHAR szMSExtentPageCountCacheIndex[]      = "primary";
static const CHAR szMSExtentPageCountCacheIndexKey[]   = "+objid\0";

BOOL FCATIExtentPageCountCacheCacheableObject(
    const PIB * const ppib,
    const IFMP ifmp,
    const OBJID objid,
    PCWSTR *ppReasonNotUpdatable
    )
{
    *ppReasonNotUpdatable = NULL;

    if ( g_fRepair )
    {
        // We don't track space in the normal way during repair.
        *ppReasonNotUpdatable = L"REPAIR";
        return fFalse;
    }

    if ( PfmpFromIfmp( ifmp )->Dbid() == dbidTemp )
    {
        // The ExtentPageCountCache table doesn't get created on a temp database.
        *ppReasonNotUpdatable = L"TEMP";
        return fFalse;
    }

    if ( PinstFromPpib( ppib )->m_plog->FRecovering() )
    {
        // We don't update during recovery, we're relying on replaying logs to get
        // the correct value.
        *ppReasonNotUpdatable = L"RECOVERING";
        return fFalse;
    }

    switch ( objid )
    {
        case objidSystemRoot:
            // We don't track the System Root in the ExtentPageCountCache.  This is because
            // when we need to split in the ExtentPageCountCache table, we need to get a
            // secondary extent from the system root and add it to the ExtentPageCountCache table.
            // This would cause a recursive call back into the ExtentPageCountCache table to track
            // those extents while we already have various pages latched in the ExtentPageCountCache
            // table. This is not supported.
            // However, we have special casing for the root in that we cache values in the FMP
            // for the database.  It's not persisted, solely volatile in memory.  It's maintained
            // in the same code that maintains the ExtentPageCountCache.
            return fTrue;

        case objidFDPMSO:
            // We don't track the catalog because we need to have the
            // catalog unlocked and available for ErrFILEOpenTable( szMSExtentPageCountCache ).
            *ppReasonNotUpdatable = L"CATALOG";
            return fFalse;

        case objidFDPMSOShadow:
            // We don't track this because it's not very interesting to know the number of pages it
            // uses.  Also, in a fresh DB create, we create this before we have the ExtentPageCountCache
            // table created, so we can't easily track space for this table from the beginning.
            *ppReasonNotUpdatable = L"SHADOW";
            return fFalse;

        case objidFDPMSO_NameIndex:
            // We don't track this because it's not very interesting to know the number of pages it
            // uses.  Also, in a fresh DB create, we create this before we have the ExtentPageCountCache
            // table created, so we can't easily track space for this table from the beginning.
            *ppReasonNotUpdatable = L"NAME_INDEX";
            return fFalse;

        case objidFDPMSO_RootObjectIndex:
            // We don't track this because it's not very interesting to know the number of pages it
            // uses.  Also, in a fresh DB create, we create this before we have the ExtentPageCountCache
            // table created, so we can't easily track space for this table from the beginning.
            *ppReasonNotUpdatable = L"OBJECT_INDEX";
            return fFalse;

        default:
            // Check for the dynamic objids we don't track.

            if ( objidNil == PfmpFromIfmp( ifmp )->ObjidExtentPageCountCacheFDP() )
            {
                // No ExtentPageCountCache is available yet.

                if ( PfmpFromIfmp( ifmp )->FAttached() )
                {
                    // Looks like we're not going to have a ExtentPageCountCache table ever.
                    *ppReasonNotUpdatable = L"NOT_ENABLED";
                }
                else
                {
                    // We are still attaching/creating.  The only reason we expect
                    // to be here in that case is if we're creating the MSObjids
                    // table or the ExtentPageCountCache table itself, and we don't track
                    // those tables in the ExtentPageCountCache.

                    // Hardcoded transaction for creating a table in ErrFILECreateTable()
                    Assert( ppib->TrxidStack().Peek() == 42277 );

                    // Hardcoded transaction for creating MSObjids in ErrCATCreateMSObjids
                    // or transaction for creating ExtentPageCountCache in ErrCATCreateMSExtentPageCountCache(
                    Assert( ppib->TrxidStack().Peek0() == 51941 || ppib->TrxidStack().Peek0() == 40670 );

                    *ppReasonNotUpdatable = L"NOT_INITIALIZED";
                }
                return fFalse;
            }

            if ( objid == PfmpFromIfmp( ifmp )->ObjidExtentPageCountCacheFDP() )
            {
                // We don't track the ExtentPageCountCache table itself for much the same
                // reason we don't track the system root; if we need to split the
                // table while we're updating in the table, we run afoul of
                // existing latches.
                //
                *ppReasonNotUpdatable = L"CACHE";
                return fFalse;
            }

            break;
    }

    return fTrue;
}


//  ================================================================
VOID CATIPossiblySetUpdatingExtentPageCountCacheFlag(
    PIB * const ppib
    )
//  ================================================================
{
#ifdef DEBUG
    Assert( !ppib->FUpdatingExtentPageCountCache() );

    // Set a flag saying we're messing with the ExtentPageCountCache.
    if ( ppib->FBatchIndexCreation() )
    {
        // We don't set this in BatchIndexCreation since that has multiple threads using
        // the same PIB.  We adjust the necessary asserts elsewhere to check for
        // BatchIndexCreation in addition to FUpdatingExtentPageCountCache  (see AssertDIRNoLatch() )
        return;
    }

    ppib->SetFUpdatingExtentPageCountCache();
#endif
}

//  ================================================================
VOID CATIPossiblyResetUpdatingExtentPageCountCacheFlag(
    PIB * const ppib
    )
//  ================================================================
{
#ifdef DEBUG
    if ( !ppib->FUpdatingExtentPageCountCache() )
    {
        // Didn't set the bit; happens in a number of code paths, generally when
        // we got an answer (usually that we don't need to update the cache) without
        // needing to do DB operations.
        return;
    }
    
    ppib->ResetFUpdatingExtentPageCountCache();
    Assert( !ppib->FUpdatingExtentPageCountCache() );
#endif
}

VOID CATIExtentPageCountsCacheReportError(
    const PIB * const ppib,
    const IFMP ifmp,
    const PCWSTR wszOperation,
    const PCWSTR wszNote,
    const PGNO pgnoFDP,
    const OBJID objidFDP,
    const ERR err
    )
{
    WCHAR rgwObjidFDP[16];
    WCHAR rgwPgnoFDP[16];
    WCHAR rgwError[16];
    OSStrCbFormatW( rgwError, sizeof(rgwError), L"%d", err );
    OSStrCbFormatW( rgwPgnoFDP, sizeof(rgwPgnoFDP), L"0x%x", pgnoFDP );
    OSStrCbFormatW( rgwObjidFDP, sizeof(rgwObjidFDP), L"0x%x", objidFDP );

    const WCHAR * rgwsz[] = {
        PfmpFromIfmp( ifmp )->WszDatabaseName(),
        wszOperation,
        wszNote,
        rgwError,
        rgwObjidFDP,
        rgwPgnoFDP,
    };

    UtilReportEvent(
        eventError,
        SPACE_MANAGER_CATEGORY,
        EXTENT_PAGE_COUNT_CACHE_OPERATION_FAILED_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        PinstFromPpib( ppib ) );
}

//
// A note about the catalog Extent Page Count Cache:
//
// The Extent Page Count Cache stores the results of operations on space tables that change the number
// of pages assigned to a user-B+ tree. It's very important that any values held in the cache be correct.
// Consider a rarely read table.  If we don't trust the cache and we have some validity-checker task
// that runs more often than we read the table, we end up with a useless cache that costs IOs rather
// than saves them.
//  
// When we say "in sync" here, we mean that either the stored value in the cache is correct with respect
// to the related space tree, there is no stored value, or there is a stored value that is marked as invalid.
// When a caller asks for a value from the cache, we check whether any stored value is marked invalid
// and treat any invalid cache entries as if there were no stored value.
//  
// What we'd really like to do is to bundle changes to space trees and the related changes to the cache
// into an atomic unit so that both happen or neither happen.  Unfortunately, that just can't be done;
// changes to space trees take place immediately with no versioning and are not done in a way
// that can be rolled back.  In order to match the behavior of the space tree, updates to the cache table
// are also done immediately with no versioning.  If an error occurs during a space tree operation
// with multiple steps, a new set of space operations and related cache operations is done to logically
// undo the change rather than "rolling back" the change.
//  
// Note that it doesn't really matter if the process crashes during the actual cache or space tree update
// operations.  What truly matters is if the process crashes when not all of the relevant log operations
// are in the same log file and some but not all of the logs are made durable.  If they're in the same log
// file and both are done without versioning, then either both will be done or neither. In the rest of this
// discussion, "crash" means crash during log file flush.
//  
// Remember that logs are always flushed to disk in order.  Given that only a single thread will be updating
// a given space tree and cache entry at a time, we are guaranteed that the order of changes here is the
// same order that the changes are made durable, regardless of which log files the changes end up in.
//  
// CATAdjustExtentPageCountsPrepare and ErrCATAdjustExtentPageCountsPrepare:
//
//   A simplistic version of updating a valid value in the cache where we just replace an existing value
//   won't work.  Regardless of whether we update the cache to the correct value before or after we
//   modify the space tree, an untimely crash will lead to a divergence.
//  
//   So, if you assume the cache entry and space tree are in sync and that you get pessimal behavior and all log
//   events end up in different log files, we use the following solution to keep the cache entry and space
//   tree in sync:
//  
//     1) If there is an existing cache entry (valid or not), mark it as invalid with a log entry to match.
//          If the process crashes after 1) but before 2), if a cache entry exists it is marked as invalid
//          and we'll get the value from the space tree rather than the cache (and incidentally update the cache).
//          If an existing cache entry was not already marked as invalid, the value is actually still correct,
//          but there isn't any way to know this.  The cache entry and the space tree stay in sync.
//     2) Modify the space tree with one or more log entries to match.
//          If the process crashes after 2) but before 3), if a cache entry exists it is marked as invalid.
//          and we'll get the value from the space tree rather than the cache (and incidentally update the cache).
//          Any value in the cache is actually incorrect, but there isn't any way to know this.  The cache
//          entry and the space tree stay in sync.
//     3) Update or insert the value in the cache and remove any existing invalid mark with a log entry to match.
//          If the process crashes after 3), we've made all the relevant changes durable and the cache
//          entry and the space tree stay in sync and the value in the cache is correct.
//  
//   As an aside, during development of the cache, an early version of the cache did the simplistic version, 
//   simply updating the cache after the space tree was updated.  Fairly simple stress tests that used JET_bitTermDirty
//   would consistently result in the space tree update being made durable while the cache update was
//   not and divergence was common.
//  
// CATSetExtentPageCounts
//   CATSetExtentPageCounts is only called when either there is no value stored in the cache or the
//   existing stored value is marked as invalid and is always called after the related space tree operations.
//   Therefore, after any crash the cache entry and the space tree are still in sync.
//  
// CATResetExtentPageCounts
//   CatResetExtentPageCounts is only called when a user-B+ tree has been removed.  Therefore, it's possible
//   that the cache entry and the space tree are no longer in sync because there is a value stored, but
//   the space tree no longer exists and so is trivially empty.  However, given that the key in the
//   cache is objid, the object with that objid no longer exists, and we don't reuse objids, the worst that
//   will happen is that it doesn't get reset in the cache, but we never look up the cache entry again 
//   (although it leaves a row in the cache table forever).
//

//  ================================================================
VOID CATSetExtentPageCounts(
    PIB * const ppib,
    const IFMP ifmp,
    const OBJID objid,
    const CPG cpgOE,
    const CPG cpgAE )
//  ================================================================
{
    ERR             err;
    FMP             *pfmp = PfmpFromIfmp( ifmp );
    FUCB            *pfucbExtentPageCountCache = pfucbNil;
    PCWSTR          wszNote;
    FDPINFO         fdpinfo;
    BOOL            fReplace = fFalse;
    BYTE            bFlag;
    JET_SETCOLUMN   rgsetcolumn[] =
        {
            { columnidMSExtentPageCountCache_cpgAE, &cpgAE, sizeof( cpgAE ), 0, 1, JET_errSuccess },
            { columnidMSExtentPageCountCache_cpgOE, &cpgOE, sizeof( cpgOE ), 0, 1, JET_errSuccess },
            { columnidMSExtentPageCountCache_bFlag, &bFlag, sizeof( bFlag ), 0, 1, JET_errSuccess },
            { columnidMSExtentPageCountCache_objid, &objid, sizeof( objid ), 0, 1, JET_errSuccess },
        };
    ULONG csetcolumn;
    OnDebug( RCE *prceNewest = ppib->prceNewest );

    Assert( objidNil != objid );

    Assert( cpgOE >= cpgAE );

    if ( !FCATIExtentPageCountCacheCacheableObject(ppib, ifmp, objid, &wszNote ) )
    {
        Assert( NULL != wszNote );
        return;
    }

    // Special case, we cache the DBRoot in memory only.
    if ( objidSystemRoot == objid )
    {
        Assert( !pfmp->FCacheAvail() );
        Assert( (CPG)pfmp->PgnoLast() == cpgOE );
        pfmp->SetCpgAvail( cpgAE );
        pfmp->SetFCacheAvail();
        wszNote = L"DB_ROOT";
        err = JET_errSuccess;
        goto HandleError;
    }

    // If we're actually going to do something to the DB, we should be in a transaction.
    Assert( 0 != ppib->Level() );

    // Can't have the system root latched because we may need to split for the insert here
    // and that may require getting an extent from the system root.
    Assert ( FBFNotLatched( ifmp, pgnoSystemRoot ) );
    Assert ( FBFNotLatched( ifmp, pgnoFDPMSO ) );
    Assert ( pgnoNull != pfmp->PgnoExtentPageCountCacheFDP() );
    Assert ( FBFNotLatched( ifmp, pfmp->PgnoExtentPageCountCacheFDP() ) );

    // Set a flag saying we're messing with the ExtentPageCountCache.
    CATIPossiblySetUpdatingExtentPageCountCacheFlag( ppib );

    wszNote = L"LOOKING";

    fdpinfo.pgnoFDP  = pfmp->PgnoExtentPageCountCacheFDP();
    fdpinfo.objidFDP = pfmp->ObjidExtentPageCountCacheFDP();
    Call( ErrFILEOpenTable( ppib, ifmp, &pfucbExtentPageCountCache, szMSExtentPageCountCache, 0, &fdpinfo) );
    Assert( pfucbExtentPageCountCache->u.pfcb->FInitialized() );

    // This only ever gets set on the FCB for the ExtentPageCountCache and it never gets unset,
    // although it needs to be set here if the FCB wasn't already cached from a prior
    // ExtentPageCountCache call.
    pfucbExtentPageCountCache->u.pfcb->SetVersioningOffForExtentPageCountCache();

    Call( ErrIsamMakeKey(
              ppib,
              pfucbExtentPageCountCache,
              &objid,
              sizeof( objid ),
              JET_bitNewKey ) );

    err = ErrIsamSeek(
        ppib,
        pfucbExtentPageCountCache,
        JET_bitSeekEQ );

    switch ( err )
    {
        case JET_errSuccess:
            Call( ErrIsamRetrieveColumn(
                      ppib,
                      pfucbExtentPageCountCache,
                      columnidMSExtentPageCountCache_bFlag,
                      &bFlag,
                      sizeof( bFlag ),
                      NULL,
                      NO_GRBIT,
                      NULL ) );
            Assert( JET_errSuccess == err );
            if ( bFlag )
            {
                wszNote = L"PREPARED";
                fReplace = fTrue;
            }
            else
            {
                wszNote = L"DUPLICATE";
                Error( ErrERRCheck( JET_errObjectDuplicate ) );
            }
            break;

        case JET_errRecordNotFound:
            // Expected case.
            err = JET_errSuccess;
            break;

        default:
            AssertSz( fFalse, "Unexpected case in switch." );
            wszNote = L"UNEXPECTED";
            if ( err > JET_errSuccess )
            {
                err = ErrERRCheck( JET_errInternalError );
            }
            Call( err );
    }

    bFlag = 0;

    if ( fReplace )
    {
        wszNote = L"REPLACE";
        Call( ErrIsamPrepareUpdate( ppib, pfucbExtentPageCountCache, JET_prepReplaceNoLock ) );
        csetcolumn = _countof( rgsetcolumn ) - 1; // Don't reset objid.
    }
    else
    {
        wszNote = L"INSERT";
        Call( ErrIsamPrepareUpdate( ppib, pfucbExtentPageCountCache, JET_prepInsert ) );
        csetcolumn = _countof( rgsetcolumn );
    }

    Call( ErrIsamSetColumns(
              (JET_SESID)ppib,
              (JET_VTID)pfucbExtentPageCountCache,
              rgsetcolumn,
              _countof( rgsetcolumn ) ) );
    Assert( JET_errSuccess == err );

    Call( ErrIsamUpdate(
              ppib,
              pfucbExtentPageCountCache,
              NULL,
              0,
              NULL,
              JET_bitUpdateNoVersion ) );

HandleError:
    CATIPossiblyResetUpdatingExtentPageCountCacheFlag( ppib );

    if ( pfucbNil != pfucbExtentPageCountCache )
    {
        ERR errT;

        errT = ErrFILECloseTable( ppib, pfucbExtentPageCountCache );
        Assert( JET_errSuccess <= errT );

        Assert ( FBFNotLatched( ifmp, pfmp->PgnoExtentPageCountCacheFDP() ) );
    }

    // No RCEs where harmed in the filming of this feature.
    Assert( prceNewest == ppib->prceNewest );

    OSTraceFMP(
        ifmp,
        JET_tracetagCatalog,
        OSFormat(
            "%hs: note=%S BC{0:0} AC{%d:%ul} [0x%x:0x%x:?] err=(%d:0x%08X)",
            __FUNCTION__,
            wszNote,
            cpgOE,
            cpgAE,
            (ULONG)ifmp,
            objid,
            err,
            err ) );

    if ( JET_errSuccess > err )
    {
        CATIExtentPageCountsCacheReportError( ppib, ifmp, L"SET_VALUE", wszNote, pgnoNull, objid, err );
    }

    // This is a best-effort attempt.  The assumption is that setting is only done to a cache
    // entry that is either invalid or not yet in the cache, so an error to set a value leaves
    // the cache consistent.  Empty, but consistent.
    return;
}

//  ================================================================
VOID CATResetExtentPageCounts(
    PIB * const ppib,
    const IFMP ifmp,
    const OBJID objid )
//  ================================================================
{
    ERR                 err;
    FMP                 *pfmp = PfmpFromIfmp( ifmp );
    FUCB                *pfucbExtentPageCountCache = pfucbNil;
    CPG                 cpgOEBefore    = -1;
    CPG                 cpgAEBefore    = -1;
    PCWSTR              wszNote;
    FDPINFO             fdpinfo;
    JET_RETRIEVECOLUMN  rgretrievecolumn[] =
        {
            { columnidMSExtentPageCountCache_cpgAE, &cpgAEBefore, sizeof( cpgAEBefore ), 0, NO_GRBIT, 0, 1, 0, JET_errSuccess },
            { columnidMSExtentPageCountCache_cpgOE, &cpgOEBefore, sizeof( cpgOEBefore ), 0, NO_GRBIT, 0, 1, 0, JET_errSuccess },
        };

    OnDebug( RCE *prceNewest = ppib->prceNewest );

    Assert( objidNil != objid );

    if ( !FCATIExtentPageCountCacheCacheableObject(ppib, ifmp, objid, &wszNote ) )
    {
        Assert( NULL != wszNote );
        return;
    }

    // Special case.  This is cached in memory.
    if ( objidSystemRoot == objid )
    {
        AssertSz( fFalse, "Why are you trying to reset the DBRoot value?" );
        pfmp->ResetFCacheAvail();
        wszNote = L"DELETE_ROOT";
        err = JET_errSuccess;
        goto HandleError;
    }

    // If we're actually going to do something to the DB, we should be in a transaction.
    Assert( 0 != ppib->Level() );

    // Can't have the system root latched because we may need to split for the insert here
    // and that may require getting an extent from the system root.
    Assert ( FBFNotLatched( ifmp, pgnoSystemRoot ) );
    Assert ( FBFNotLatched( ifmp, pgnoFDPMSO ) );
    Assert ( pgnoNull != pfmp->PgnoExtentPageCountCacheFDP() );
    Assert ( FBFNotLatched( ifmp, pfmp->PgnoExtentPageCountCacheFDP() ) );

    // Set a flag saying we're messing with the ExtentPageCountCache.
    CATIPossiblySetUpdatingExtentPageCountCacheFlag( ppib );

    wszNote = L"LOOKING";

    fdpinfo.pgnoFDP  = pfmp->PgnoExtentPageCountCacheFDP();
    fdpinfo.objidFDP = pfmp->ObjidExtentPageCountCacheFDP();
    Call( ErrFILEOpenTable( ppib, ifmp, &pfucbExtentPageCountCache, szMSExtentPageCountCache, 0, &fdpinfo) );
    Assert( pfucbExtentPageCountCache->u.pfcb->FInitialized() );

    // This only ever gets set on the FCB for the ExtentPageCountCache and it never gets unset,
    // although it needs to be set here if the FCB wasn't already cached from a prior
    // ExtentPageCountCache call.
    pfucbExtentPageCountCache->u.pfcb->SetVersioningOffForExtentPageCountCache();

    Call( ErrIsamMakeKey(
              ppib,
              pfucbExtentPageCountCache,
              &objid,
              sizeof( objid ),
              JET_bitNewKey ) );

    err = ErrIsamSeek(
        ppib,
        pfucbExtentPageCountCache,
        JET_bitSeekEQ );

    switch ( err )
    {
        case JET_errSuccess:
            pfucbExtentPageCountCache->locLogical = locOnCurBM;
            // Found it to delete.
            break;

        case JET_errRecordNotFound:
            err = JET_errSuccess;
            wszNote = L"NOT_FOUND";
            goto HandleError;

        default:
            AssertSz( fFalse, "Unexpected case in switch." );
            wszNote = L"UNEXPECTED";
            if ( err > JET_errSuccess )
            {
                err = ErrERRCheck( JET_errInternalError );
            }
            Call( err );
    }

    Call( ErrIsamRetrieveColumns(
              (JET_SESID)ppib,
              (JET_VTID)pfucbExtentPageCountCache,
              rgretrievecolumn,
              _countof( rgretrievecolumn ) ) );

    wszNote = L"DELETE";

    Call( ErrIsamDelete( ppib, pfucbExtentPageCountCache ) );

HandleError:
    CATIPossiblyResetUpdatingExtentPageCountCacheFlag( ppib );

    if ( pfucbNil != pfucbExtentPageCountCache )
    {
        ERR errT;

        errT = ErrFILECloseTable( ppib, pfucbExtentPageCountCache );
        Assert( JET_errSuccess <= errT );

        Assert ( FBFNotLatched( ifmp, pfmp->PgnoExtentPageCountCacheFDP() ) );
    }

    // No RCEs where harmed in the filming of this feature.
    Assert( prceNewest == ppib->prceNewest );

    OSTraceFMP(
        ifmp,
        JET_tracetagCatalog,
        OSFormat(
            "%hs: note=%S BC{%d:%d} AC{0:0} [0x%x:0x%x:?] err=(%d:0x%08X)",
            __FUNCTION__,
            wszNote,
            cpgOEBefore,
            cpgAEBefore,
            (ULONG)ifmp,
            objid,
            err,
            err ) );

    if ( JET_errSuccess > err )
    {
        CATIExtentPageCountsCacheReportError( ppib, ifmp, L"RESET_VALUE", wszNote, pgnoNull, objid, err );
    }

    // This is a best-effort attempt.  The assumption is that resetting is only done to a cache
    // entry for which the objid is no longer valid.  In that case, while the cache is technically
    // inconsistent, the objid used as a key will never come up again, so it doesn't matter that
    // there is still a value for it in this table.  The row will never go away short of dropping
    // the entire table, but at least it's consistent.

}

//  ================================================================
ERR _ErrCATAdjustExtentPageCountsPrepare(
    const FUCB * const pfucb,
    ULONG ulLine
    )
{
    ERR             err;
    FMP             *pfmp = PfmpFromIfmp( pfucb->ifmp );
    FUCB            *pfucbExtentPageCountCache    = pfucbNil;
    PGNO            pgno              = PgnoFDP( pfucb );
    OBJID           objid             = ObjidFDP( pfucb );
    PIB             *ppib             = pfucb->ppib;
    IFMP            ifmp              = pfucb->ifmp;
    BYTE            bFlag;
    PCWSTR          wszNote;
    FDPINFO         fdpinfo;

    OnDebug( RCE *prceNewest = ppib->prceNewest );

    // Called before modifying a space tree.  Marks the cached value as invalid.
    // Value becomes valid again when it's updated in CATAdjustCpgValues().
    // Lets us deal with log redo that includes the update to a space tree without
    // including the matching adjustment of the ExtentPageCountCache.

    if ( !FCATIExtentPageCountCacheCacheableObject( ppib, ifmp, objid, &wszNote ) )
    {
        Assert( NULL != wszNote );
        return JET_errSuccess;
    }

    // We expect to only be called from the Space Tree code, and it has to have a write lock
    // on the FDP of the pfucb for the update to be safe (in the Space Tree code).
    // Note that Pcsr( pfucb ) may or may not be current on pgno.  That's why we
    // go directly to FBFWriteLatched rather than Pcsr( pfucb )->CheckLatch().
    Assert( FBFWriteLatched( ifmp, pgno ) || FBFRDWLatched( ifmp, pgno ) );

    // Special case.  This is cached in memory.
    if ( objidSystemRoot == objid )
    {
        if ( pfmp->FCacheAvail() )
        {
            // We don't actually have to mark this as invalid, since the invalid mark is
            // to maintain consistency in the face of a crash and a log replay.  Since
            // this is only in-memory, that's not an issue.
            wszNote = L"NOP_ROOT";
        }
        else
        {
            wszNote = L"NOT_FOUND_ROOT";
        }
        err = JET_errSuccess;
        goto HandleError;
    }

    // If we're actually going to do something to the DB, we should be in a transaction.
    Assert( 0 != ppib->Level() );

    // and the FDP of the ExtentPageCountCache needs to not be latched, since we'll be taking latches here.
    // Again, we don't yet have a CSR that's current for that page so we go directly to
    // FBFNotLatched().
    Assert ( pgnoNull != pfmp->PgnoExtentPageCountCacheFDP() );
    Assert ( FBFNotLatched( ifmp, pfmp->PgnoExtentPageCountCacheFDP() ) );

    // Set a flag saying we're messing with the ExtentPageCountCache.
    CATIPossiblySetUpdatingExtentPageCountCacheFlag( ppib );

    wszNote = L"LOOKING";

    fdpinfo.pgnoFDP  = pfmp->PgnoExtentPageCountCacheFDP();
    fdpinfo.objidFDP = pfmp->ObjidExtentPageCountCacheFDP();
    Call( ErrFILEOpenTable( ppib, ifmp, &pfucbExtentPageCountCache, szMSExtentPageCountCache, 0, &fdpinfo) );
    Assert( pfucbExtentPageCountCache->u.pfcb->FInitialized() );

    // This only ever gets set on the FCB for the ExtentPageCountCache and it never gets unset,
    // although it needs to be set here if the FCB wasn't already cached from a prior
    // ExtentPageCountCache call.  This lets us do IsamUpdate( JET_bitNoVersion ), which would
    // normally not be supported.
    pfucbExtentPageCountCache->u.pfcb->SetVersioningOffForExtentPageCountCache();

    Call( ErrIsamMakeKey(
              ppib,
              pfucbExtentPageCountCache,
              &objid,
              sizeof( objid ),
              JET_bitNewKey ) );

    err = ErrIsamSeek(
        ppib,
        pfucbExtentPageCountCache,
        JET_bitSeekEQ );


    switch ( err )
    {
        case JET_errSuccess:
            break;

        case JET_errRecordNotFound:
            // Didn't find the value to adjust.
            err = JET_errSuccess;
            wszNote = L"NOT_FOUND";
            goto HandleError;

        default:
            AssertSz( fFalse, "Unexpected case in switch.");
            wszNote = L"UNEXPECTED";
            if ( err > JET_errSuccess )
            {
                err = ErrERRCheck( JET_errInternalError );
            }
            Call( err );
    }

    Call( ErrIsamRetrieveColumn(
              ppib,
              pfucbExtentPageCountCache,
              columnidMSExtentPageCountCache_bFlag,
              &bFlag,
              sizeof( bFlag ),
              NULL,
              NO_GRBIT,
              NULL ) );

    if ( bFlag )
    {
        // We're being asked to prepare, which usually means to mark as prepared,
        // and then when we update, we re-mark as not-prepared.  Since this is ALREADY
        // prepared, something bad happened.
        // Delete the existing value and let it get refilled naturally later.
        wszNote = L"DELETE";
        Call( ErrIsamDelete( ppib, pfucbExtentPageCountCache ) );
    }
    else
    {
        wszNote = L"REPLACE";
        bFlag = 1;

        Call( ErrIsamPrepareUpdate(
                  ppib,
                  pfucbExtentPageCountCache,
                  JET_prepReplaceNoLock ) );
        Assert( JET_errSuccess == err );

        Call( ErrIsamSetColumn(
                  ppib,
                  pfucbExtentPageCountCache,
                  columnidMSExtentPageCountCache_bFlag,
                  &bFlag,
                  sizeof( bFlag ),
                  NO_GRBIT,
                  NULL ) );
        Assert( JET_errSuccess == err );

        Call( ErrIsamUpdate(
                  ppib,
                  pfucbExtentPageCountCache,
                  NULL,
                  0,
                  NULL,
                  JET_bitUpdateNoVersion ) );
        Assert( JET_errSuccess == err );

    }

HandleError:
    CATIPossiblyResetUpdatingExtentPageCountCacheFlag( ppib );

    if ( pfucbNil != pfucbExtentPageCountCache )
    {
        ERR errT;

        errT = ErrFILECloseTable( ppib, pfucbExtentPageCountCache );
        Assert( JET_errSuccess == errT );
        pfucbExtentPageCountCache = pfucbNil;

        // If we opened and closed a table, these pages need to still not be latched.
        Assert ( pgnoNull != pfmp->PgnoExtentPageCountCacheFDP() );

        Assert ( FBFNotLatched( ifmp, pfmp->PgnoExtentPageCountCacheFDP() ) );
    }

    Assert( pfucbNil == pfucbExtentPageCountCache );

    // No RCEs where harmed in the filming of this feature.
    Assert( prceNewest == ppib->prceNewest );

    OSTraceFMP(
        ifmp,
        JET_tracetagCatalog,
        OSFormat(
            "%hs: note=%S [0x%x:0x%x:%lu] err=(%d:0x%08X)",
            __FUNCTION__,
            wszNote,
            (ULONG)ifmp,
            objid,
            PgnoFDP( pfucb ),
            err,
            err ) );

    if ( JET_errSuccess > err )
    {
        CATIExtentPageCountsCacheReportError( ppib, ifmp, L"UPDATE_VALUE_PREPARE", wszNote, pgnoNull, objid, err );
    }

    // This is NOT a best-effort attempt.  It has to succeed for us to be able to make guarantees
    // about the consistency of the cache.
    return err;
}

//  ================================================================
VOID CATAdjustExtentPageCounts(
    const FUCB * const pfucb,
    const CPG lAddCpgOE,
    const CPG lAddCpgAE )
//  ================================================================
{
    ERR                 err;
    FMP                 *pfmp = PfmpFromIfmp( pfucb->ifmp );
    FUCB                *pfucbExtentPageCountCache = pfucbNil;
    PGNO                pgno           = PgnoFDP( pfucb );
    OBJID               objid          = ObjidFDP( pfucb );
    PIB                 *ppib          = pfucb->ppib;
    IFMP                ifmp           = pfucb->ifmp;
    CPG                 cpgAEBefore    = -1;
    CPG                 cpgAEAfter     = -1;
    CPG                 cpgOEBefore    = -1;
    CPG                 cpgOEAfter     = -1;
    PCWSTR              wszNote;
    FDPINFO             fdpinfo;
    BYTE                bFlag;
    JET_RETRIEVECOLUMN  rgretrievecolumn[] =
        {
            { columnidMSExtentPageCountCache_cpgAE, &cpgAEBefore, sizeof( cpgAEBefore ), 0, NO_GRBIT, 0, 1, 0, JET_errSuccess },
            { columnidMSExtentPageCountCache_cpgOE, &cpgOEBefore, sizeof( cpgOEBefore ), 0, NO_GRBIT, 0, 1, 0, JET_errSuccess },
            { columnidMSExtentPageCountCache_bFlag, &bFlag,       sizeof( bFlag ),       0, NO_GRBIT, 0, 1, 0, JET_errSuccess },
        };
    JET_SETCOLUMN   rgsetcolumn[] =
        {
            { columnidMSExtentPageCountCache_cpgAE, &cpgAEAfter, sizeof( cpgAEAfter ), 0, 1, JET_errSuccess },
            { columnidMSExtentPageCountCache_cpgOE, &cpgOEAfter, sizeof( cpgOEAfter ), 0, 1, JET_errSuccess },
            { columnidMSExtentPageCountCache_bFlag, &bFlag,      sizeof( bFlag ),      0, 1, JET_errSuccess },
        };

    OnDebug( RCE *prceNewest = ppib->prceNewest );

    if ( !FCATIExtentPageCountCacheCacheableObject( ppib, ifmp, objid, &wszNote ) )
    {
        Assert( NULL != wszNote );
        return;
    }

    // We expect to only be called from the Space Tree code, and it has to have a write lock
    // on the FDP of the pfucb for the update to be safe (in the Space Tree code).
    // Note that Pcsr( pfucb ) may or may not be current on pgno.  That's why we
    // go directly to FBFWriteLatched rather than Pcsr( pfucb )->CheckLatch().
    Assert( FBFWriteLatched( ifmp, pgno ) || FBFRDWLatched( ifmp, pgno ) );

    // Special case.  This is cached in memory.
    if ( objidSystemRoot == objid )
    {
        if ( pfmp->FCacheAvail() )
        {
            // There's nothing to do with lAddCpgOE.  We don't directly cache that,
            // we return pfmp->PgnoLast() when someone asks.
            pfmp->AdjustCpgAvail( lAddCpgAE );
            wszNote = L"REPLACE_ROOT";
        }
        else
        {
            wszNote = L"NOT_FOUND_ROOT";
        }
        err = JET_errSuccess;
        goto HandleError;
    }

    // If we're actually going to do something to the DB, we should be in a transaction.
    Assert( 0 != ppib->Level() );

    // The ExtentPageCountCache needs to be in use (shown by the FDP value being there in pfmp)
    // and the FDP of the ExtentPageCountCache needs to not be latched, since we'll be taking latches here.
    // Again, we don't yet have a CSR that's current for that page so we go directly to
    // FBFNotLatched().
    Assert ( pgnoNull != pfmp->PgnoExtentPageCountCacheFDP() );
    Assert ( FBFNotLatched( ifmp, pfmp->PgnoExtentPageCountCacheFDP() ) );

    // Set a flag saying we're messing with the ExtentPageCountCache.
    CATIPossiblySetUpdatingExtentPageCountCacheFlag( ppib );

    wszNote = L"LOOKING";

    fdpinfo.pgnoFDP  = pfmp->PgnoExtentPageCountCacheFDP();
    fdpinfo.objidFDP = pfmp->ObjidExtentPageCountCacheFDP();
    Call( ErrFILEOpenTable( ppib, ifmp, &pfucbExtentPageCountCache, szMSExtentPageCountCache, 0, &fdpinfo) );
    Assert( pfucbExtentPageCountCache->u.pfcb->FInitialized() );

    // This only ever gets set on the FCB for the ExtentPageCountCache and it never gets unset,
    // although it needs to be set here if the FCB wasn't already cached from a prior
    // ExtentPageCountCache call.
    pfucbExtentPageCountCache->u.pfcb->SetVersioningOffForExtentPageCountCache();

    Call( ErrIsamMakeKey(
              ppib,
              pfucbExtentPageCountCache,
              &objid,
              sizeof( objid ),
              JET_bitNewKey ) );

    err = ErrIsamSeek(
        ppib,
        pfucbExtentPageCountCache,
        JET_bitSeekEQ );

    switch ( err )
    {
        case JET_errSuccess:
            break;

        case JET_errRecordNotFound:
            // Didn't find the value to adjust.
            err = JET_errSuccess;
            wszNote = L"NOT_FOUND";
            goto HandleError;

        default:
            AssertSz( fFalse, "Unexpected case in switch.");
            wszNote = L"UNEXPECTED";
            if ( err > JET_errSuccess )
            {
                err = ErrERRCheck( JET_errInternalError );
            }
            Call( err );
    }

    Call( ErrIsamRetrieveColumns(
              (JET_SESID)ppib,
              (JET_VTID)pfucbExtentPageCountCache,
              rgretrievecolumn,
              _countof( rgretrievecolumn ) ) );

    Assert( bFlag );

    Assert( cpgOEBefore >= cpgAEBefore );

    cpgOEAfter = cpgOEBefore + lAddCpgOE;
    cpgAEAfter = cpgAEBefore + lAddCpgAE;

    Assert( cpgOEAfter >= cpgAEAfter );

    bFlag = 0;

    wszNote = L"REPLACE";
    Call( ErrIsamPrepareUpdate( ppib, pfucbExtentPageCountCache, JET_prepReplaceNoLock ) );

    Call( ErrIsamSetColumns(
              (JET_SESID)ppib,
              (JET_VTID)pfucbExtentPageCountCache,
              rgsetcolumn,
              _countof( rgsetcolumn ) ) );
    Assert( JET_errSuccess == err );

    Call( ErrIsamUpdate(
              ppib,
              pfucbExtentPageCountCache,
              NULL,
              0,
              NULL,
              JET_bitUpdateNoVersion ) );

HandleError:
    CATIPossiblyResetUpdatingExtentPageCountCacheFlag( ppib );

    if ( pfucbNil != pfucbExtentPageCountCache )
    {
        ERR errT;

        errT = ErrFILECloseTable( ppib, pfucbExtentPageCountCache );
        Assert( JET_errSuccess == errT );
        pfucbExtentPageCountCache = pfucbNil;

        // If we opened and closed a table, these pages need to still not be latched.
        Assert ( pgnoNull != pfmp->PgnoExtentPageCountCacheFDP() );

        Assert ( FBFNotLatched( ifmp, pfmp->PgnoExtentPageCountCacheFDP() ) );
    }

    Assert( pfucbNil == pfucbExtentPageCountCache );

    // No RCEs where harmed in the filming of this feature.
    Assert( prceNewest == ppib->prceNewest );

    OSTraceFMP(
        ifmp,
        JET_tracetagCatalog,
        OSFormat(
            "%hs: note=%S BC{%d:%d} AC{%d:%d} DC{%d:%d} [0x%x:0x%x:%lu] err=(%d:0x%08X)",
            __FUNCTION__,
            wszNote,
            cpgOEBefore,
            cpgAEBefore,
            cpgOEAfter,
            cpgAEAfter,
            lAddCpgOE,
            lAddCpgAE,
            (ULONG)ifmp,
            objid,
            pgno,
            err,
            err ) );

    if ( JET_errSuccess > err )
    {
        CATIExtentPageCountsCacheReportError( ppib, ifmp, L"UPDATE_VALUE", wszNote, pgno, objid, err );
    }

    // This is a best-effort attempt.  The assumption is that updating is only done to a cache
    // entry that is already present and marked as invalid.  If we fail, it's still marked as
    // invalid, so the cache is still consistent.

    return;
}

//  ================================================================
ERR ErrCATGetExtentPageCounts(
    PIB * const ppib,
    const IFMP ifmp,
    const OBJID objid,
    CPG * const pcpgOE,
    CPG * const pcpgAE )
//  ================================================================
{
    ERR                 err;
    FMP                 *pfmp = PfmpFromIfmp( ifmp );
    FUCB                *pfucbExtentPageCountCache = pfucbNil;
    PCWSTR              wszNote;
    FDPINFO             fdpinfo;
    CPG                 cpgAE;
    CPG                 cpgOE;
    BYTE                bFlag;
    JET_RETRIEVECOLUMN  rgretrievecolumn[] =
        {
            { columnidMSExtentPageCountCache_cpgAE, &cpgAE, sizeof( cpgAE ), 0, NO_GRBIT, 0, 1, 0, JET_errSuccess },
            { columnidMSExtentPageCountCache_cpgOE, &cpgOE, sizeof( cpgOE ), 0, NO_GRBIT, 0, 1, 0, JET_errSuccess },
            { columnidMSExtentPageCountCache_bFlag, &bFlag, sizeof( bFlag ), 0, NO_GRBIT, 0, 1, 0, JET_errSuccess },
        };

    if( pgnoNull == pfmp->PgnoExtentPageCountCacheFDP() )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    if( !FCATIExtentPageCountCacheCacheableObject( ppib, ifmp, objid, &wszNote ) )
    {
        Error( ErrERRCheck( JET_errNotInitialized ) );
    }

    // Special case.  This is cached in memory.
    if ( objidSystemRoot == objid )
    {
        if ( pfmp->FCacheAvail() )
        {
            *pcpgOE = pfmp->PgnoLast();
            *pcpgAE = pfmp->CpgAvail();
            err = JET_errSuccess;
            goto HandleError;
        }
        else
        {
            Error( ErrERRCheck( JET_errRecordNotFound ) );
        }
    }

    // Set a flag saying we're messing with the ExtentPageCountCache.
    CATIPossiblySetUpdatingExtentPageCountCacheFlag( ppib );

    Assert ( pgnoNull != pfmp->PgnoExtentPageCountCacheFDP() );
    Assert ( FBFNotLatched( ifmp, pfmp->PgnoExtentPageCountCacheFDP() ) );

    fdpinfo.pgnoFDP  = pfmp->PgnoExtentPageCountCacheFDP();
    fdpinfo.objidFDP = pfmp->ObjidExtentPageCountCacheFDP();
    Call( ErrFILEOpenTable( ppib, ifmp, &pfucbExtentPageCountCache, szMSExtentPageCountCache, 0, &fdpinfo) );
    Assert( pfucbExtentPageCountCache->u.pfcb->FInitialized() );

    // This only ever gets set on the FCB for the ExtentPageCountCache and it never gets unset,
    // although it needs to be set here if the FCB wasn't already cached from a prior
    // ExtentPageCountCache call.
    pfucbExtentPageCountCache->u.pfcb->SetVersioningOffForExtentPageCountCache();

    Call( ErrIsamMakeKey(
              ppib,
              pfucbExtentPageCountCache,
              &objid,
              sizeof( objid ),
              JET_bitNewKey ) );

    err = ErrIsamSeek(
        ppib,
        pfucbExtentPageCountCache,
        JET_bitSeekEQ );

    switch ( err )
    {
        case JET_errSuccess:
            break;

        case JET_errRecordNotFound:
            break;

        default:
            AssertSz( fFalse, "Unexpected case in switch.");
            if ( err > JET_errSuccess )
            {
                err = ErrERRCheck( JET_errInternalError );
            }
            break;
    }
    Call( err );

    Call( ErrIsamRetrieveColumns(
              (JET_SESID)ppib,
              (JET_VTID)pfucbExtentPageCountCache,
              rgretrievecolumn,
              _countof( rgretrievecolumn ) ) );
    Assert( JET_errSuccess == err );

    if ( bFlag )
    {
        OSTraceFMP(
            ifmp,
            JET_tracetagCatalog,
            OSFormat(
                "%hs: note=Reading uninitialized [0x%x:0x%x:?].",
                __FUNCTION__,
                (ULONG)ifmp,
                objid) );
        Error( ErrERRCheck( JET_errRecordNotFound ) );
    }

    Assert( cpgOE >= cpgAE );

    if ( NULL != pcpgOE )
    {
        *pcpgOE = cpgOE;
    }

    if ( NULL != pcpgAE )
    {
        *pcpgAE = cpgAE;
    }

HandleError:
    CATIPossiblyResetUpdatingExtentPageCountCacheFlag( ppib );

    if ( pfucbNil != pfucbExtentPageCountCache )
    {
        ERR errT = ErrFILECloseTable( ppib, pfucbExtentPageCountCache );
        Assert( JET_errSuccess == errT );

        // If we opened and closed a table, these pages need to still not be latched.
        Assert ( pgnoNull != pfmp->PgnoExtentPageCountCacheFDP() );
        Assert ( FBFNotLatched( ifmp, pfmp->PgnoExtentPageCountCacheFDP() ) );
    }

    return err;
}

//  ================================================================
ERR ErrCATCreateMSExtentPageCountCache(
        __in PIB * const ppib,
        const IFMP ifmp,
        PGNO *ppgnoFDP = NULL,
        OBJID *pobjidFDP = NULL
    )
//  ================================================================
{
    Assert( !g_fRepair );
    Assert( ppibNil != ppib );
    Assert( ifmpNil != ifmp );

    ERR                 err             = JET_errSuccess;
    FMP                 *pfmp           = PfmpFromIfmp( ifmp );
    BOOL                fInTransaction  = fFalse;

    JET_COLUMNCREATE_A  rgcolumncreateMSExtentPageCountCache[] = {
//      { cbStruct,                   szColumn,     coltyp,                 cbMax, grbit,               pvDefault, cbDefault, cp, columnid, err            }
        { sizeof(JET_COLUMNCREATE_A), "objid",      JET_coltypLong,         4,     JET_bitColumnFixed,  NULL,      0,         0,  0,        JET_errSuccess },
        { sizeof(JET_COLUMNCREATE_A), "cpgAE",      JET_coltypLong,         4,     JET_bitColumnFixed,  NULL,      0,         0,  0,        JET_errSuccess },
        { sizeof(JET_COLUMNCREATE_A), "cpgOE",      JET_coltypLong,         4,     JET_bitColumnFixed,  NULL,      0,         0,  0,        JET_errSuccess },
        { sizeof(JET_COLUMNCREATE_A), "flag",       JET_coltypUnsignedByte, 1,     JET_bitColumnFixed,  NULL,      0,         0,  0,        JET_errSuccess },
    };

    JET_INDEXCREATE3_A  rgindexcreateMSExtentPageCountCache[] = {
        {
            sizeof( JET_INDEXCREATE3_A ),                   // size of this structure
            const_cast<char *>( szMSExtentPageCountCacheIndex ),        // index name
            const_cast<char *>( szMSExtentPageCountCacheIndexKey ),     // index key
            sizeof( szMSExtentPageCountCacheIndexKey ),                 // length of key
            JET_bitIndexPrimary,                            // index options
            100,                                            // index density
            NULL,                                           // pidxunicode2 for the index
            0,                                              // maximum length of variable length columns in index key
            NULL,                                           // pointer to conditional column structure
            0,                                              // number of conditional columns
            JET_errSuccess,                                 // returned error code,
            255,                                            // maximum key size
            NULL                                            // space hints
        },
    };

    JET_TABLECREATE5_A  tablecreateMSExtentPageCountCache = {
        sizeof( JET_TABLECREATE5_A ),       // size of this structure
        const_cast<char *>( szMSExtentPageCountCache ), // name of table
        NULL,                               // name of base table
        1,                                  // initial pages
        100,                                // density
        rgcolumncreateMSExtentPageCountCache,           // columns to create
        _countof(rgcolumncreateMSExtentPageCountCache), // number of columns to create
        rgindexcreateMSExtentPageCountCache,            // array of index creation info
        _countof(rgindexcreateMSExtentPageCountCache),  // number of indexes to create
        NULL,                               // callback to use for this table
        JET_cbtypNull,                      // when the callback should be called
        JET_bitTableCreateSystemTable | JET_bitTableCreateFixedDDL,     // grbit
        NULL,                               // Sequential index space hints.
        NULL,                               // LV index space hints
        0,                                  // cbSeparateLV threshold
        0,                                  // cbLVChunkMax
        JET_TABLEID( pfucbNil ),            // returned tableid
        0,                                  // returned count of objects created
    };

    const JET_DBID  dbid    = (JET_DBID)ifmp;

    *ppgnoFDP = pgnoNull;
    *pobjidFDP = objidNil;

    Call( pfmp->ErrDBFormatFeatureEnabled( JET_efvExtentPageCountCache ) );

    Assert( !fInTransaction );
    Call( ErrDIRBeginTransaction( ppib, 40670, NO_GRBIT ) );
    fInTransaction = fTrue;

    Call( ErrFILECreateTable( ppib, dbid, &tablecreateMSExtentPageCountCache, fSPMultipleExtent ) );

    Assert( columnidMSExtentPageCountCache_objid == rgcolumncreateMSExtentPageCountCache[0].columnid );
    Assert( columnidMSExtentPageCountCache_cpgAE == rgcolumncreateMSExtentPageCountCache[1].columnid );
    Assert( columnidMSExtentPageCountCache_cpgOE == rgcolumncreateMSExtentPageCountCache[2].columnid );
    Assert( columnidMSExtentPageCountCache_bFlag == rgcolumncreateMSExtentPageCountCache[3].columnid );

    if ( NULL != ppgnoFDP )
    {
        *ppgnoFDP = PgnoFDP( (FUCB *)tablecreateMSExtentPageCountCache.tableid );
    }
    if ( NULL != pobjidFDP )
    {
        *pobjidFDP = ObjidFDP( (FUCB *)tablecreateMSExtentPageCountCache.tableid );
    }

    Call( ErrFILECloseTable( ppib, (FUCB *)tablecreateMSExtentPageCountCache.tableid ) );

    const WCHAR * rgwsz[] = { pfmp->WszDatabaseName(), L"FEATURE_ON" };

    UtilReportEvent(
        eventInformation,
        SPACE_MANAGER_CATEGORY,
        EXTENT_PAGE_COUNT_CACHE_CREATED_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        PinstFromPpib( ppib ) );

    Assert( fInTransaction );
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTransaction = fFalse;

HandleError:

    if( fInTransaction )
    {
        const ERR errRollback = ErrDIRRollback( ppib );
        CallSx( errRollback, JET_errRollbackError );
        Assert( errRollback >= JET_errSuccess || PinstFromPpib( ppib )->FInstanceUnavailable( ) );
        fInTransaction = fFalse;
    }

    OSTraceFMP(
        ifmp,
        JET_tracetagCatalog,
        OSFormat(
            "Session=[0x%p:0x%x] finished creating MSExtentPageCountCache table '%s' on database=['%ws':0x%x] with error %d (0x%x)",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            szMSExtentPageCountCache,
            pfmp->WszDatabaseName(),
            (ULONG)ifmp,
            err,
            err ) );

    Assert( !fInTransaction );
    return err;
}

//  ================================================================
ERR ErrCATDeleteMSExtentPageCountCache(
        _In_ PIB * const ppib,
        _In_ const IFMP ifmp,
        _In_ const EXTENT_CACHE_DELETE_REASON ecdrReason,
        _Out_ BOOL *pfTableExisted )
//  ================================================================
{
    ERR     err             = JET_errSuccess;
    FMP     *pfmp           = PfmpFromIfmp( ifmp );
    BOOL    fDatabaseOpen   = fFalse;
    BOOL    fInTransaction  = fFalse;
    IFMP    ifmpT;

    if ( NULL != pfTableExisted )
    {
        *pfTableExisted = fFalse;
    }

    pfmp->ResetFCacheAvail();

    Call( ErrDBOpenDatabase( ppib, pfmp->WszDatabaseName(), &ifmpT, NO_GRBIT ) );
    Assert( ifmp == ifmpT );
    fDatabaseOpen = fTrue;

    Assert( !fInTransaction );
    err = ErrCATSeekTable( ppib, ifmp, szMSExtentPageCountCache, NULL, NULL );

    switch ( err )
    {
        case JET_errSuccess:
            break;

        case JET_errObjectNotFound:
            err = JET_errSuccess;
            goto HandleError;

        default:
            AssertSz( fFalse, "Unexpected case in switch");
            Call( err );
            // Got a warning.  Didn't expect one, not sure what happened, but presumably the
            // table is there.
            break;
    }

    // Table exists
    if ( NULL != pfTableExisted )
    {
        *pfTableExisted = fTrue;
    }

    Call( ErrDIRBeginTransaction( ppib, 57054, NO_GRBIT ) );
    fInTransaction = fTrue;

    // Set a flag saying we're messing with the ExtentPageCountCache.
    Call( ErrIsamDeleteTable( (JET_SESID)ppib, (JET_DBID)ifmp, szMSExtentPageCountCache, fTrue ) );

    Assert( fInTransaction );
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTransaction = fFalse;

    WCHAR rgwReason[32];
    const WCHAR *pwszReason;

    switch ( ecdrReason )
    {
        case EXTENT_CACHE_DELETE_REASON::FeatureOff:
            pwszReason = L"FEATURE_OFF";
            break;

        case EXTENT_CACHE_DELETE_REASON::Repair:
            pwszReason = L"DATABASE_REPAIR";
            break;

        default:
            OSStrCbFormatW( rgwReason, sizeof(rgwReason), L"REASON_%d", ecdrReason );
            pwszReason = rgwReason;
            break;
    }

    const WCHAR * rgwsz[] = { pfmp->WszDatabaseName(), pwszReason };

    UtilReportEvent(
        eventWarning,
        SPACE_MANAGER_CATEGORY,
        EXTENT_PAGE_COUNT_CACHE_DELETED_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        PinstFromPpib( ppib ) );

HandleError:
    if( fInTransaction )
    {
        (void)ErrDIRRollback( ppib );
        fInTransaction = fFalse;
    }
    if( fDatabaseOpen )
    {
        CallS( ErrDBCloseDatabase( ppib, ifmpT, NO_GRBIT ) );
    }

    OSTraceFMP(
        ifmp,
        JET_tracetagCatalog,
        OSFormat(
            "Session=[0x%p:0x%x] finished deleting MSExtentPageCountCache table '%s' on database=['%ws':0x%x] with error %d (0x%x)",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            szMSExtentPageCountCache,
            pfmp->WszDatabaseName(),
            (ULONG)ifmp,
            err,
            err ) );
    Assert( !fInTransaction );
    return err;
}

//  ================================================================
//  Iterates through root objects in the catalog. If *ppfucbCatalog
//  is pfucbNil, we'll initialize the iteration (i.e., we'll return
//  the first root object in the catalog) and the FUCB of the catalog
//  will be returned so we can find the other root objects in subsequent
//  calls. When there are no more objects, the function will still
//  succeed, but the returned objid will be objidNil. The user must call
//  ErrCATClose() on the returned catalog FUCB once the first call
//  (i.e., the initialization call) has succeeded, even after a
//  subsequent failure. The user must NOT call ErrCATClose() on the
//  returned catalog FUCB if the initialization call fails.
ERR ErrCATGetNextRootObject(
    _In_ PIB* const         ppib,
    _In_ const IFMP         ifmp,
    _Inout_ FUCB** const    ppfucbCatalog,
    _Out_ OBJID* const      pobjid )
//  ================================================================
{
    Assert( ppib != NULL );
    Assert( ifmp != ifmpNil );
    Assert( ppfucbCatalog != NULL );
    Assert( pobjid != NULL );

    ERR err = JET_errSuccess;
    FUCB* pfucbCatalog = *ppfucbCatalog;
    BOOL fInitilializedFucb = fFalse;
    BOOL fCatalogLatched = fFalse;
    DATA dataField;

    *pobjid = objidNil;

    // Should we open the catalog or continue from where we left off?
    if ( pfucbCatalog == pfucbNil )
    {
        Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
        fInitilializedFucb = fTrue;

        // Set the "RootObjects" secondary index and sets the search to root objects.
        Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, szMSORootObjectsIndex ) );

        const BYTE bTrue = 0xff;

        // Preread records.
        JET_INDEX_COLUMN indexColumn;
        indexColumn.columnid = fidMSO_RootFlag;
        indexColumn.relop = JET_relopEquals;
        indexColumn.pv = (void*)&bTrue;
        indexColumn.cb = sizeof( bTrue );
        indexColumn.grbit = NO_GRBIT;
        JET_INDEX_RANGE indexRange;
        indexRange.rgStartColumns = &indexColumn;
        indexRange.cStartColumns = 1;
        indexRange.rgEndColumns = NULL;
        indexRange.cEndColumns = 0;
        Call( ErrIsamPrereadIndexRange( (JET_SESID)ppib, (JET_TABLEID)pfucbCatalog, &indexRange, 0, lMax, JET_bitPrereadForward, NULL ) );

        // Go to the first record.
        Call( ErrIsamMakeKey( ppib, pfucbCatalog, &bTrue, sizeof( bTrue ), JET_bitNewKey | JET_bitFullColumnStartLimit ) );
        err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekGE );
        if ( err == JET_errRecordNotFound )
        {
            Assert( fFalse );   //  We should have at least the catalog itself.
            Error( ErrERRCheck( JET_errObjectNotFound ) );
        }
        Call( err );
        err = JET_errSuccess;

        // Set index range (just to be safe, since we expect bTrue to be at the end of the index anyways).
        Call( ErrIsamMakeKey( ppib, pfucbCatalog, &bTrue, sizeof( bTrue ), JET_bitNewKey | JET_bitFullColumnEndLimit ) );
        err = ErrIsamSetIndexRange( ppib, pfucbCatalog, JET_bitRangeInclusive | JET_bitRangeUpperLimit );
        if ( err == JET_errNoCurrentRecord )
        {
            Assert( fFalse );   //  We should have at least the catalog itself.
            Error( ErrERRCheck( JET_errObjectNotFound ) );
        }
        Call( err );
        err = JET_errSuccess;
    }
    else
    {
        err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
        if ( err == JET_errNoCurrentRecord )
        {
            err = JET_errSuccess;
            goto HandleError;
        }
        Call( err );
        err = JET_errSuccess;
    }

    Call( ErrDIRGet( pfucbCatalog ) );
    fCatalogLatched = fTrue;

    // We're correctly positioned at the record, retrieve the objid.
    Call( ErrRECIRetrieveFixedColumn( pfcbNil, pfucbCatalog->u.pfcb->Ptdb(), fidMSO_Id, pfucbCatalog->kdfCurr.data, &dataField ) );
    *pobjid = *( (UnalignedLittleEndian<OBJID>*)dataField.Pv() );

HandleError:
    Assert( ( err < JET_errSuccess ) || ( pfucbCatalog != pfucbNil ) );

    if ( fCatalogLatched )
    {
        Assert( pfucbCatalog != pfucbNil );
        CallS( ErrDIRRelease( pfucbCatalog ) );
    }

    if ( ( err < JET_errSuccess ) && fInitilializedFucb )
    {
        Assert( pfucbCatalog != pfucbNil );
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        fInitilializedFucb = fFalse;
        pfucbCatalog = pfucbNil;
    }

    *ppfucbCatalog = pfucbCatalog;

    return err;
}

//  ================================================================
//  Iterates through non-root objects associated with a given root
//  object in the catalog. If *ppfucbCatalog is pfucbNil, we'll
//  initialize the iteration (i.e., we'll return the first non-root
//  object owned by the given root object in the catalog) and the
//  FUCB of the catalog will be returned so we can find the other
//  non-root objects in subsequent calls. When there are no more
//  objects, the function will still succeed, but the returned objid
//  will be objidNil. The user must call ErrCATClose() on the returned
//  catalog FUCB once the first call (i.e., the initialization call)
//  has succeeded, even after a subsequent failure. The user must NOT
//  call ErrCATClose() on the returned catalog FUCB if the initialization
//  call fails. Note that objidTable is only consumed in the
//  initialization call, which means that to enumerate objects owned
//  by a different root object, we need to initiate another iteration.
ERR ErrCATGetNextNonRootObject(
    _In_ PIB* const         ppib,
    _In_ const IFMP         ifmp,
    _In_ const OBJID        objidTable,
    _Inout_ FUCB** const    ppfucbCatalog,
    _Out_ OBJID* const      pobjid,
    _Out_ SYSOBJ* const     psysobj )
//  ================================================================
{
    Assert( ppib != NULL );
    Assert( ifmp != ifmpNil );
    Assert( ppfucbCatalog != NULL );
    Assert( pobjid != NULL );
    Assert( psysobj != NULL );

    ERR err = JET_errSuccess;
    FUCB* pfucbCatalog = *ppfucbCatalog;
    BOOL fInitilializedFucb = fFalse;
    BOOL fCatalogLatched = fFalse;

    *pobjid = objidNil;
    *psysobj = sysobjNil;

    // Should we open the catalog or continue from where we left off?
    if ( pfucbCatalog == pfucbNil )
    {
        Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
        fInitilializedFucb = fTrue;

        // Go to the first record.
        Call( ErrIsamMakeKey( ppib, pfucbCatalog, &objidTable, sizeof( objidTable ), JET_bitNewKey | JET_bitFullColumnStartLimit ) );
        err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekGE );
        if ( err == JET_errRecordNotFound )
        {
            Error( ErrERRCheck( JET_errObjectNotFound ) );
        }
        Call( err );
        err = JET_errSuccess;

        // Set index range.
        Call( ErrIsamMakeKey( ppib, pfucbCatalog, &objidTable, sizeof( objidTable ), JET_bitNewKey | JET_bitFullColumnEndLimit ) );
        err = ErrIsamSetIndexRange( ppib, pfucbCatalog, JET_bitRangeInclusive | JET_bitRangeUpperLimit );
        if ( err == JET_errNoCurrentRecord )
        {
            Error( ErrERRCheck( JET_errObjectNotFound ) );
        }
        Call( err );
        err = JET_errSuccess;
    }
    else
    {
        err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
        if ( err == JET_errNoCurrentRecord )
        {
            err = JET_errSuccess;
            goto HandleError;
        }
        Call( err );
        err = JET_errSuccess;
    }

    // We're correctly positioned at the record, retrieve the type.
    SYSOBJ sysobj = sysobjNil;
    while ( fTrue )
    {
        DATA dataField;

        Call( ErrDIRGet( pfucbCatalog ) );
        fCatalogLatched = fTrue;

        Call( ErrRECIRetrieveFixedColumn( pfcbNil, pfucbCatalog->u.pfcb->Ptdb(), fidMSO_Type, pfucbCatalog->kdfCurr.data, &dataField ) );
        sysobj = *( (UnalignedLittleEndian<SYSOBJ>*)dataField.Pv() );

        // If it's an index or an LV, return it.
        if ( ( sysobj == sysobjIndex ) || ( sysobj == sysobjLongValue ) )
        {
            // If it's an index, make sure it is not the clustered index.
            if ( sysobj == sysobjIndex )
            {
                Call( ErrRECIRetrieveFixedColumn( pfcbNil, pfucbCatalog->u.pfcb->Ptdb(), fidMSO_Flags, pfucbCatalog->kdfCurr.data, &dataField ) );
                const LE_IDXFLAG* const ple_idxflag = (LE_IDXFLAG*)dataField.Pv();
                const IDBFLAG idbflag = ple_idxflag->fidb;
                if ( ( idbflag & fidbPrimary ) != 0 )
                {
                    sysobj = sysobjNil;
                }
            }

            if ( sysobj != sysobjNil )
            {
                Call( ErrRECIRetrieveFixedColumn( pfcbNil, pfucbCatalog->u.pfcb->Ptdb(), fidMSO_Id, pfucbCatalog->kdfCurr.data, &dataField ) );
                *pobjid = *( (UnalignedLittleEndian<OBJID>*)dataField.Pv() );
                Call( ErrRECIRetrieveFixedColumn( pfcbNil, pfucbCatalog->u.pfcb->Ptdb(), fidMSO_Type, pfucbCatalog->kdfCurr.data, &dataField ) );
                *psysobj = *( (UnalignedLittleEndian<SYSOBJ>*)dataField.Pv() );
                break;
            }
        }

        CallS( ErrDIRRelease( pfucbCatalog ) );
        fCatalogLatched = fFalse;

        err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
        if ( err == JET_errNoCurrentRecord )
        {
            err = JET_errSuccess;
            break;
        }
        Call( err );
        err = JET_errSuccess;
    }

HandleError:
    Assert( ( err < JET_errSuccess ) || ( pfucbCatalog != pfucbNil ) );

    if ( fCatalogLatched )
    {
        Assert( pfucbCatalog != pfucbNil );
        CallS( ErrDIRRelease( pfucbCatalog ) );
    }

    if ( ( err < JET_errSuccess ) && fInitilializedFucb )
    {
        Assert( pfucbCatalog != pfucbNil );
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        fInitilializedFucb = fFalse;
        pfucbCatalog = pfucbNil;
    }

    *ppfucbCatalog = pfucbCatalog;

    return err;
}

//  ================================================================
//  Iterates through all objects associated with a given root
//  object in the catalog that have a given pgnoFDP as its root page.
//  If *ppfucbCatalog is pfucbNil, we'll initialize the iteration
//  (i.e., we'll return the first object owned by the given root
//  object in the catalog) and the FUCB of the catalog will be
//  returned so we can find the other objects in subsequent calls.
//  When there are no more objects, the function will still succeed,
//  but the returned objid will be objidNil. The user must call
//  ErrCATClose() on the returned catalog FUCB once the first call
//  (i.e., the initialization call) has succeeded, even after a
//  subsequent failure. The user must NOT call ErrCATClose() on the
//  returned catalog FUCB if the initialization call fails. Note that
//  objidTable is only consumed in the initialization call, which means
//  that to enumerate objects owned by a different root object, we need
//  to initiate another iteration.
ERR ErrCATGetNextObjectByPgnoFDP(
    _In_ PIB* const         ppib,
    _In_ const IFMP         ifmp,
    _In_ const OBJID        objidTable,
    _In_ const PGNO         pgnoFDP,
    _In_ const BOOL         fShadow,
    _Inout_ FUCB** const    ppfucbCatalog,
    _Out_ OBJID* const      pobjid,
    _Out_ SYSOBJ* const     psysobj )
//  ================================================================
{
    Assert( ppib != NULL );
    Assert( ifmp != ifmpNil );
    Assert( ppfucbCatalog != NULL );
    Assert( pobjid != NULL );
    Assert( psysobj != NULL );

    ERR err = JET_errSuccess;
    FUCB* pfucbCatalog = *ppfucbCatalog;
    BOOL fInitilializedFucb = fFalse;
    BOOL fCatalogLatched = fFalse;

    *pobjid = objidNil;
    *psysobj = sysobjNil;

    // Should we open the catalog or continue from where we left off?
    if ( pfucbCatalog == pfucbNil )
    {
        Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fShadow ) );
        fInitilializedFucb = fTrue;

        // Go to the first record.
        Call( ErrIsamMakeKey( ppib, pfucbCatalog, &objidTable, sizeof( objidTable ), JET_bitNewKey | JET_bitFullColumnStartLimit ) );
        err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekGE );
        if ( err == JET_errRecordNotFound )
        {
            Error( ErrERRCheck( JET_errObjectNotFound ) );
        }
        Call( err );
        err = JET_errSuccess;

        // Set index range.
        Call( ErrIsamMakeKey( ppib, pfucbCatalog, &objidTable, sizeof( objidTable ), JET_bitNewKey | JET_bitFullColumnEndLimit ) );
        err = ErrIsamSetIndexRange( ppib, pfucbCatalog, JET_bitRangeInclusive | JET_bitRangeUpperLimit );
        if ( err == JET_errNoCurrentRecord )
        {
            Error( ErrERRCheck( JET_errObjectNotFound ) );
        }
        Call( err );
        err = JET_errSuccess;
    }
    else
    {
        err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
        if ( err == JET_errNoCurrentRecord )
        {
            err = JET_errSuccess;
            goto HandleError;
        }
        Call( err );
        err = JET_errSuccess;
    }

    // We're correctly positioned at the record, retrieve the type.
    SYSOBJ sysobj = sysobjNil;
    while ( fTrue )
    {
        DATA dataField;

        Call( ErrDIRGet( pfucbCatalog ) );
        fCatalogLatched = fTrue;

        Call( ErrRECIRetrieveFixedColumn( pfcbNil, pfucbCatalog->u.pfcb->Ptdb(), fidMSO_Type, pfucbCatalog->kdfCurr.data, &dataField ) );
        sysobj = *( (UnalignedLittleEndian<SYSOBJ>*)dataField.Pv() );

        // The concept of pgnoFDP only applies to tables, indexes or long value trees.
        if ( ( sysobj == sysobjTable ) || ( sysobj == sysobjIndex ) || ( sysobj == sysobjLongValue ) )
        {
            Call( ErrRECIRetrieveFixedColumn( pfcbNil, pfucbCatalog->u.pfcb->Ptdb(), fidMSO_PgnoFDP, pfucbCatalog->kdfCurr.data, &dataField ) );
            if ( *( (UnalignedLittleEndian<PGNO>*)dataField.Pv() ) == pgnoFDP )
            {
                Call( ErrRECIRetrieveFixedColumn( pfcbNil, pfucbCatalog->u.pfcb->Ptdb(), fidMSO_Id, pfucbCatalog->kdfCurr.data, &dataField ) );
                *pobjid = *( (UnalignedLittleEndian<OBJID>*)dataField.Pv() );
                Call( ErrRECIRetrieveFixedColumn( pfcbNil, pfucbCatalog->u.pfcb->Ptdb(), fidMSO_Type, pfucbCatalog->kdfCurr.data, &dataField ) );
                *psysobj = *( (UnalignedLittleEndian<SYSOBJ>*)dataField.Pv() );
                break;
            }
        }

        CallS( ErrDIRRelease( pfucbCatalog ) );
        fCatalogLatched = fFalse;

        err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
        if ( err == JET_errNoCurrentRecord )
        {
            err = JET_errSuccess;
            break;
        }
        Call( err );
        err = JET_errSuccess;
    }

HandleError:
    Assert( ( err < JET_errSuccess ) || ( pfucbCatalog != pfucbNil ) );

    if ( fCatalogLatched )
    {
        Assert( pfucbCatalog != pfucbNil );
        CallS( ErrDIRRelease( pfucbCatalog ) );
    }

    if ( ( err < JET_errSuccess ) && fInitilializedFucb )
    {
        Assert( pfucbCatalog != pfucbNil );
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
        fInitilializedFucb = fFalse;
        pfucbCatalog = pfucbNil;
    }

    *ppfucbCatalog = pfucbCatalog;

    return err;
}

//  ================================================================
ERR ErrCATGetCursorsFromObjid(
    _In_ PIB* const ppib,
    _In_ const IFMP ifmp,
    _In_ const OBJID objid,
    _In_ const OBJID objidParent,
    _In_ const SYSOBJ sysobj,
    _Out_ PGNO* const ppgnoFDP,
    _Out_ PGNO* const ppgnoFDPParent,
    _Out_ FUCB** const ppfucb,
    _Out_ FUCB** const ppfucbParent )
//  ================================================================
{
    ERR err = JET_errSuccess;
    PGNO pgnoFDPParent = pgnoNull;
    PGNO pgnoFDP = pgnoNull;
    FUCB* pfucb = pfucbNil;
    FUCB* pfucbParent = pfucbNil;

    // The root's pgnoFDP is known.
    if ( objid == objidSystemRoot )
    {
        Assert( objidParent == objidNil );
        Assert( sysobj == sysobjNil );
        pgnoFDP = pgnoSystemRoot;
        Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucb ) );
        pgnoFDPParent = pgnoNull;
        pfucbParent = pfucbNil;
    }
    else
    {
        Assert( objidParent != objidNil );
        Assert( sysobj != sysobjNil );

        // Handle objects which are not the DB root.
        //

        CHAR szName[ JET_cbNameMost + 1 ];
        const OBJID objidTable = ( sysobj == sysobjTable ) ? objid : objidParent;
        PGNO pgnoFDPTable = pgnoNull;
        Call( ErrCATSeekObjectByObjid( ppib, ifmp, objidTable, sysobjTable, objidTable, szName, _countof( szName ), &pgnoFDPTable ) );
        Assert( ( pgnoFDPTable != pgnoNull ) && ( pgnoFDPTable != pgnoSystemRoot ) );

        FDPINFO fdpinfo;
        fdpinfo.objidFDP = objidTable;
        fdpinfo.pgnoFDP = pgnoFDPTable;

        FUCB* pfucbTable = pfucbNil;
        Call( ErrFILEOpenTable( ppib, ifmp, &pfucbTable, szName, JET_bitTableDenyRead, &fdpinfo ) );

        if ( sysobj == sysobjTable )
        {
            pgnoFDPParent = pgnoSystemRoot;
            Call( ErrDIROpen( ppib, pgnoFDPParent, ifmp, &pfucbParent ) );

            pgnoFDP = pgnoFDPTable;
            pfucb = pfucbTable;
        }
        else if ( sysobj == sysobjIndex )
        {
            Call( ErrCATSeekObjectByObjid( ppib, ifmp, objidTable, sysobj, objid, szName, _countof( szName ), &pgnoFDP ) );
            Assert( ( pgnoFDP != pgnoNull ) && ( pgnoFDP != pgnoSystemRoot ) && ( pgnoFDP != pgnoFDPTable ) );

            Call( ErrIsamSetCurrentIndex( ppib, pfucbTable, szName ) );
            Assert( pfucbTable->pfucbCurIndex != pfucbNil );

            pfucb = pfucbTable->pfucbCurIndex;
            pgnoFDPParent = pgnoFDPTable;
            pfucbParent = pfucbTable;
        }
        else if ( sysobj == sysobjLongValue )
        {
            Call( ErrFILEOpenLVRoot( pfucbTable, &pfucbTable->pfucbLV, fFalse ) );

            Assert( pfucbTable->pfucbLV != pfucbNil );
            pfucb = pfucbTable->pfucbLV;
            pgnoFDP = PgnoFDP( pfucb );
            Assert( ( pgnoFDP != pgnoNull ) && ( pgnoFDP != pgnoSystemRoot ) && ( pgnoFDP != pgnoFDPTable ) );
            pgnoFDPParent = pgnoFDPTable;
            pfucbParent = pfucbTable;
        }
        else
        {
            Assert( fFalse );
        }

        Assert( pgnoFDP != pgnoSystemRoot );
        Assert( pgnoFDPParent != pgnoNull );
        Assert( pgnoFDPParent == pfucbParent->u.pfcb->PgnoFDP() );
    }

    Assert( pfucb != pfucbNil );
    Assert( ( pgnoFDP == pfucb->u.pfcb->PgnoFDP() ) );
    Assert( pfucb->u.pfcb->FInitialized() );

    // Initialize space metadata in the FCB if necessary.
    if ( !pfucb->u.pfcb->FSpaceInitialized() )
    {
        Call( ErrSPDeferredInitFCB( pfucb ) );
    }

    *ppgnoFDPParent = pgnoFDPParent;
    *ppgnoFDP = pgnoFDP;
    *ppfucb = pfucb;
    *ppfucbParent = pfucbParent;

HandleError:
    if ( err < JET_errSuccess )
    {
        CATFreeCursorsFromObjid( pfucb, pfucbParent );
        *ppgnoFDPParent = pgnoNull;
        *ppgnoFDP = pgnoNull;
        *ppfucb = pfucbNil;
        *ppfucbParent = pfucbNil;
    }

    return err;
}

//  ================================================================
VOID CATFreeCursorsFromObjid( _In_ FUCB* const pfucb, _In_ FUCB* const pfucbParent )
//  ================================================================
{
    Assert( ( pfucb != pfucbNil ) || ( pfucbParent == pfucbNil ) );

    if ( pfucb == pfucbNil )
    {
        return;
    }

    if ( pfucbParent == pfucbNil )
    {
        Assert( pfucb->u.pfcb->PgnoFDP() == pgnoSystemRoot );

        BTUp( pfucb );
        DIRClose( pfucb );
    }
    else
    {
        Assert( pfucbParent->u.pfcb->PgnoFDP() != pgnoNull );

        // The parent might be the DB root, in which case, we need to close it differently.
        if ( pfucbParent->u.pfcb->PgnoFDP() == pgnoSystemRoot )
        {
            BTUp( pfucbParent );
            DIRClose( pfucbParent );

            BTUp( pfucb );
            CallS( ErrFILECloseTable( pfucb->ppib, pfucb ) );
        }
        else
        {
            BTUp( pfucbParent );
            BTUp( pfucb );
            CallS( ErrFILECloseTable( pfucbParent->ppib, pfucbParent ) );

            // We don't need to close pfucb here because it is always either
            // a cursor one of its secondary indexes or the LV tree,  which means
            // ErrFILECloseTable() above closes them for us.
        }
    }
}

//  ================================================================
ERR ErrCATMSObjidsRecordExists(
        __in PIB * const ppib,
        const IFMP ifmp,
        const OBJID objid,
        __out bool * const pfExists)
//  ================================================================
{
    ERR err;
    const JET_SESID sesid = (JET_SESID) ppib;
    MSObjidInfo msoInfo;

    Call( ErrCATIOpenMSObjids( sesid, (JET_DBID) ifmp, &msoInfo ) );
    err = ErrCATISeek( sesid, msoInfo.tableid, objid );
    if ( JET_errSuccess == err )
    {
        *pfExists = true;
    }
    else if ( JET_errRecordNotFound == err )
    {
        *pfExists = false;
        err = JET_errSuccess;
    }
    Call( err );

HandleError:
    CATICloseMSObjids( sesid, &msoInfo );
    return err;
}

//  ================================================================
ERR ErrCATVerifyMSObjids(
        __in PIB * const ppib,
        const IFMP ifmp,
              CPRINTF * const pcprintfError )
//  ================================================================
{
    ERR err;
    const JET_SESID sesid = (JET_SESID) ppib;
    JET_TABLEID tableidCatalog = JET_tableidNil;
    MSObjidInfo msoInfo;
    MSObjidInfo msoInfoTemp;

    err = ErrCATIOpenMSObjids( sesid, (JET_DBID) ifmp, &msoInfo );
    if ( JET_errObjectNotFound == err )
    {
        // No MSObjids? No problem.
        return JET_errSuccess;
    }
    Call( err );

    Call( ErrIsamOpenTable( sesid, (JET_DBID) ifmp, &tableidCatalog, szMSO, NO_GRBIT ) );

    JET_COLUMNDEF rgcolumndef[] =
    {
        {   // objid
            sizeof( JET_COLUMNDEF ),
            JET_columnidNil,
            JET_coltypLong,
            0,
            0,
            0,
            0,
            4,
            JET_bitColumnTTKey,
        },
        {   // objidTable
            sizeof( JET_COLUMNDEF ),
            JET_columnidNil,
            JET_coltypLong,
            0,
            0,
            0,
            0,
            4,
            NO_GRBIT,
        },
        {   // type
            sizeof( JET_COLUMNDEF ),
            JET_columnidNil,
            JET_coltypShort,
            0,
            0,
            0,
            0,
            2,
            NO_GRBIT,
        },
    };
    JET_COLUMNID rgcolumnid[_countof( rgcolumndef )];
    Call( ErrIsamOpenTempTable(
            sesid,
            rgcolumndef,
            _countof( rgcolumndef ),
            NULL,
            JET_bitTTErrorOnDuplicateInsertion,
            &msoInfoTemp.tableid,
            rgcolumnid,
            0,
            0) );

    msoInfoTemp.columnidObjid = rgcolumnid[0];
    msoInfoTemp.columnidObjidTable = rgcolumnid[1];
    msoInfoTemp.columnidType = rgcolumnid[2];

    Call( ErrCATIPopulateMSObjids( sesid, tableidCatalog, msoInfoTemp ) );
    Call( ErrCATICompareMSObjids( sesid, msoInfo, msoInfoTemp, pcprintfError ) );

HandleError:
    CATICloseMSObjids( sesid, &msoInfo );
    CATICloseMSObjids( sesid, &msoInfoTemp );
    if ( JET_tableidNil != tableidCatalog )
    {
        CallS( ErrDispCloseTable( sesid, tableidCatalog ) );
    }

    return err;
}

//
//  The MSysLocales Table / KVPStore
//
//  All the locales + sort-versions that are referenced in defined indexes in this
//  database are also tracked in the MSysLocales KVP-Store.  This MSysLocales store
//  basically ref counts for each index there is for a given LCID + sort version.
//
//  On creation (or cleanup that changes version) of an index will will increment
//  the index count for that locale's LCID + sort-version.  We do not decrement this
//  count, as it is difficult to ensure we're after a commit to level 0.
//

//  This is the value stored in the DBFILEHDR::le_qwSortVersion _after_ the database
//  has been upgraded to contain an MSysLocales table containing a ref count for all
//  the localized indices in the database / catalog.
//

//  When the MSysLocales feature was implemented, there was a bug that could make
//  a database logically corrupted by allowing localized indices to be present
//  without the proper representation in the MSysLocales table. This was the bug,
//  step-by-step:
//    1. Server X is the active and creates the DB, creates MSysLocales during DB creation;
//    2. Server X creates localized indices, MSysLocales gets updated;
//    3. Server Y is the passive and replays #1 and #2;
//    4. Active moves to server Y. During the first attach of this DB, the header is still uninitialized and so ErrIsamAttachDatabase()...
//      a. Deletes MSysLocales on trx A;
//      b. Creates an empty MSysLocales on trx B;
//      c. Re-populates the new MSysLocales on multiple separate transactions (one per row of MSysLocales).
//    5. Active moves back to server X. Log records generated by Y (when active) only made to X up to 4.b (4.c somehow
//       didn't make: lost a log file, or granular replication didn't fill an entire buffer, for example);
//    6. Server X replays up to 4.b, ends up with an empty MSysLocales (note 4.a, 4.b and 4.c use separate transactions);
//    7. Server X attaches the DB: BOOM! The localized indices are there, but MSysLocales is empty!
//
//  The fix was to insert a marker record to the MSysLocales table once the creation/population was done so that, on attach,
//  the absence of such a marker would signal the need to completely rebuild the table. The price to pay here was that the
//  first attach of every existing database triggered a full rebuild of the table.
//

//  This value is used for indicating that the MSLocales table schema is completely created
//  and consistent. Currently used for debugging purposes only, so no functional effect.

PERSISTED const QWORD g_qwUpgradedLocalesTable = 0xFFFFFFFFFFFFFFFF;

//  Major Version of the MSysLocales KVP-Store ...

PERSISTED const ULONG g_dwMSLocalesMajorVersions = 1;   // endianness taken care of by KVPStore

//  The key format we use for the MSysLocales KVP-Store ... old lcid based format "LCID=%d,Ver=%I64x";

PERSISTED const WCHAR g_wszLocaleNameSortIDSortVersionLocaleKey [] = L"LocaleName=%s,SortID=%s,Ver=%I64x";
const ULONG g_cchLocaleNameSortIDSortVersionLocaleKeyMax = _countof(g_wszLocaleNameSortIDSortVersionLocaleKey) + NORM_LOCALE_NAME_MAX_LENGTH /* LocaleName */ + 32 /* SortID */ + 16 /* Ver */ + 1 /* NULL */;

//  Consistency markers.

PERSISTED const WCHAR g_wszMSLocalesConsistencyMarkerKey[] = L"MSysLocalesConsistent";
PERSISTED const INT g_cMSLocalesConsistencyMarkerValue = 1;
C_ASSERT( _countof( g_wszMSLocalesConsistencyMarkerKey ) <= g_cchLocaleNameSortIDSortVersionLocaleKeyMax ); // expected

//  Internal function prototypes.

ERR ErrCATIPopulateMSLocales( __in PIB * const ppib, const IFMP ifmp );

//  Functions for MSLocales / MSysLocales support ...

//  Determines whether or not a given key-value pair represents the consistency marker.

INLINE BOOL FCATIIsMSLocalesConsistencyMarker( const WCHAR * const wszMSLocalesKey, const INT cMSLocalesValue )
{
    if ( ( wszMSLocalesKey != NULL ) &&
            ( wcscmp( wszMSLocalesKey, g_wszMSLocalesConsistencyMarkerKey ) == 0 )&&
            ( cMSLocalesValue == g_cMSLocalesConsistencyMarkerValue ) )
    {
        return fTrue;
    }

    return fFalse;
}

JETUNITTEST( CATMSysLocales, TestFCATIsMSLocalesConsistencyMarker )
{
    CHECK( fTrue == FCATIIsMSLocalesConsistencyMarker( g_wszMSLocalesConsistencyMarkerKey, g_cMSLocalesConsistencyMarkerValue ) );
    CHECK( fFalse == FCATIIsMSLocalesConsistencyMarker( g_wszMSLocalesConsistencyMarkerKey, g_cMSLocalesConsistencyMarkerValue + 1 ) );
    CHECK( fFalse == FCATIIsMSLocalesConsistencyMarker( NULL, g_cMSLocalesConsistencyMarkerValue ) );
    CHECK( fFalse == FCATIIsMSLocalesConsistencyMarker( L"NotAGoodMarker", g_cMSLocalesConsistencyMarkerValue ) );
}

//  Init's MSLocales facility (and creates the table if necessary).

ERR ErrCATICreateMSLocales(
    __in PIB * const ppib,
    const IFMP ifmp )
{
    ERR err = JET_errSuccess;
    CKVPStore * pkvps = NULL;
    IFMP ifmpOpen = ifmpNil;

    Assert( ppib != ppibNil );  //  This function assumes a valid session.

    Call( ErrDBOpenDatabase( ppib, g_rgfmp[ifmp].WszDatabaseName(), &ifmpOpen, NO_GRBIT ) );
    Assert( ifmpOpen == ifmp );
    Assert( JET_wrnFileOpenReadOnly != err );   // shouldn't have tried to populate if DB is RO.

    Alloc( pkvps = new CKVPStore( ifmp, wszMSLocales ) );

    Call( pkvps->ErrKVPInitStore( ppib, CKVPStore::eReadWrite, g_dwMSLocalesMajorVersions ) );

    //  set the MSysLocales store value in the FMP

    g_rgfmp[ ifmp ].SetKVPMSysLocales( pkvps );
    pkvps = NULL;   // owned by FMP now ...
    Assert( g_rgfmp[ ifmp ].PkvpsMSysLocales() );

HandleError:

    Assert( g_rgfmp[ ifmp ].PkvpsMSysLocales() != NULL || err < JET_errSuccess );

    //  do not need to KVPTermStore() because we used an external PIB / IFMP ...

    delete pkvps;

    if ( ifmpOpen != ifmpNil )
    {
        CallS( ErrDBCloseDatabase( ppib, ifmpOpen, 0 ) );
    }

    return err;
}

//  Deletes the MSysLocales table.

//  ================================================================
ERR ErrCATDeleteMSLocales(
        __in PIB * const ppibProvided,
        __in const IFMP ifmp )
//  ================================================================
{
    ERR     err             = JET_errSuccess;
    BOOL    fDatabaseOpen   = fFalse;
    BOOL    fInTransaction  = fFalse;
    PIB *   ppib            = NULL;
    IFMP    ifmpT           = ifmpNil;

    if ( NULL == ppibProvided )
    {
        Call( ErrPIBBeginSession( PinstFromIfmp( ifmp ), &ppib, procidNil, fFalse ) );
    }
    else
    {
        ppib = ppibProvided;
    }

    Call( ErrDBOpenDatabase( ppib, g_rgfmp[ifmp].WszDatabaseName(), &ifmpT, NO_GRBIT ) );
    Assert( ifmp == ifmpT );
    fDatabaseOpen = fTrue;

    Assert( !fInTransaction );
    Call( ErrDIRBeginTransaction( ppib, 33607, NO_GRBIT ) );
    fInTransaction = fTrue;

    //  Now delete the actual table.

    err = ErrIsamDeleteTable( (JET_SESID)ppib, (JET_DBID)ifmp, szMSLocales );
    if( JET_errObjectNotFound == err )
    {
        err = JET_errSuccess;
    }
    else
    {
        Call( err );
    }

    Assert( fInTransaction );
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTransaction = fFalse;

HandleError:
    if( fInTransaction )
    {
        (void)ErrDIRRollback( ppib );
        fInTransaction = fFalse;
    }

    if( fDatabaseOpen )
    {
        Assert( ifmpNil != ifmpT );
        CallS( ErrDBCloseDatabase( ppib, ifmpT, NO_GRBIT ) );
    }

    if ( NULL == ppibProvided )
    {
        PIBEndSession( ppib );
    }

    Assert( !fInTransaction );
    return err;
}

//  Stamps the consistency marker to the MSysLocales table.

//  ================================================================
ERR ErrCATIFlagMSLocalesConsistent(
    __in PIB * const ppib,
    const IFMP ifmp
//  ================================================================
    )
{
    ERR err = JET_errSuccess;
    IFMP ifmpOpen = ifmpNil;
    Call( ErrDBOpenDatabase( ppib, g_rgfmp[ifmp].WszDatabaseName(), &ifmpOpen, NO_GRBIT ) );
    Assert( ifmpOpen == ifmp );
    Assert( JET_wrnFileOpenReadOnly != err );   // shouldn't have tried to change MSysLocales if DB is RO.

    Call( g_rgfmp[ifmp].PkvpsMSysLocales()->ErrKVPSetValue( g_wszMSLocalesConsistencyMarkerKey, g_cMSLocalesConsistencyMarkerValue ) );

    OSTrace( JET_tracetagUpgrade, OSFormat( "Inserted MSysLocales consistency marker = %ws (%d)\n", g_wszMSLocalesConsistencyMarkerKey, g_cMSLocalesConsistencyMarkerValue ) );

HandleError:

    if ( ifmpOpen != ifmpNil )
    {
        CallS( ErrDBCloseDatabase( ppib, ifmpOpen, 0 ) );
    }

    return err;
}

//  This opens the MSysLocales KVPStore and searches for the consistency marker.
//  Does not open/close the database. Takes a CKVPStore * instead.

//  ================================================================
ERR ErrCATICheckForMSLocalesConsistencyMarker(
    __in CKVPStore * const pkvpMSysLocales,
    __out BOOL * const pfMSLocalesConsistencyMarker )
//  ================================================================
{
    ERR err = JET_errSuccess;
    INT iMSLocalesValue;

    *pfMSLocalesConsistencyMarker = fFalse;

    err = pkvpMSysLocales->ErrKVPGetValue( g_wszMSLocalesConsistencyMarkerKey, &iMSLocalesValue );

    //  Not finding the record is a valid case, so clobber the error.

    if ( err == JET_errRecordNotFound )
    {
        err = JET_errSuccess;
    }
    else
    {
        Call( err );
        Assert( err == JET_errSuccess );

        *pfMSLocalesConsistencyMarker = FCATIIsMSLocalesConsistencyMarker( g_wszMSLocalesConsistencyMarkerKey, iMSLocalesValue );

        //  There's no special value that could be set for that specific key, other than g_cMSLocalesConsistencyMarkerValue.

        Expected( *pfMSLocalesConsistencyMarker );
    }

HandleError:

    return err;
}

//  This opens the MSysLocales KVPStore and searches for the consistency marker.
//  Opens/closes the database.

//  ================================================================
ERR ErrCATICheckForMSLocalesConsistencyMarker(
    __in PIB * const ppib,
    const IFMP ifmp,
    __out BOOL * const pfMSLocalesConsistencyMarker )
//  ================================================================
{
    ERR err = JET_errSuccess;
    IFMP ifmpOpen = ifmpNil;

    *pfMSLocalesConsistencyMarker = fFalse;

    Call( ErrDBOpenDatabase( ppib, g_rgfmp[ifmp].WszDatabaseName(), &ifmpOpen, NO_GRBIT ) );
    Assert( ifmpOpen == ifmp );

    Call( ErrCATICheckForMSLocalesConsistencyMarker( g_rgfmp[ifmp].PkvpsMSysLocales(), pfMSLocalesConsistencyMarker ) );

HandleError:

    if ( ifmpOpen != ifmpNil )
    {
        CallS( ErrDBCloseDatabase( ppib, ifmpOpen, 0 ) );
    }

    return err;
}

//  Terms's MSLocales facility (must be done before tearing down PIBs and FUCBs).

VOID CATTermMSLocales( FMP * const pfmp )
{
    if ( pfmp->PkvpsMSysLocales() )
    {
        CKVPStore * pkvps = pfmp->PkvpsMSysLocales();
        pfmp->SetKVPMSysLocales( NULL );
        pkvps->KVPTermStore();
        delete pkvps;
    }
}

//  Data structures for MSLocales / MSysLocales support ...

typedef struct _LOCALEINFO
{
    QWORD       m_qwVersion;
    LCID        m_lcid;
    INT         m_cIndices;
} LOCALEINFO;

typedef struct _LOCALENAMEINFO
{
    QWORD       m_qwVersion;
    WCHAR       m_wszLocaleName[NORM_LOCALE_NAME_MAX_LENGTH];
    SORTID      m_sortID;
    INT         m_cIndices;
} LOCALENAMEINFO;

typedef CArray< LOCALEINFO > CLocaleInfoArray;
typedef CArray< LOCALENAMEINFO > CLocaleNameInfoArray;

// IMPORTANT: This function only works when used for straight up comparison in a linear search, not for binary search as greater/smaller relationships
// will not be properly translated with the string comparison below.
INT __cdecl PfnCmpLocaleNameInfo( const LOCALENAMEINFO * pentry1, const LOCALENAMEINFO * pentry2 )
{
    const LONG lComparison = NORMCompareLocaleName( pentry1->m_wszLocaleName, pentry2->m_wszLocaleName );
    if ( 0 != lComparison )
    {
        return lComparison;
    }

    if ( DwNLSVersionFromSortVersion( pentry1->m_qwVersion ) != DwNLSVersionFromSortVersion( pentry2->m_qwVersion ) )
    {
        return DwNLSVersionFromSortVersion( pentry1->m_qwVersion ) - DwNLSVersionFromSortVersion( pentry2->m_qwVersion );
    }

    if ( DwDefinedVersionFromSortVersion( pentry1->m_qwVersion ) != DwDefinedVersionFromSortVersion( pentry2->m_qwVersion ) )
    {
        return DwDefinedVersionFromSortVersion( pentry1->m_qwVersion ) - DwDefinedVersionFromSortVersion( pentry2->m_qwVersion );
    }

    return memcmp( &pentry1->m_sortID, &pentry2->m_sortID, sizeof( SORTID ) );
}


// Note: This code largely stolen from ErrCATDeleteOutOfDateLocalizedIndexes.  I was worried about breaking defaults, and the
// code was a little complex and non-obvious to change.

ERR ErrCATIAccumulateIndexLocales(
    __in PIB * const ppib,
    __in const IFMP ifmp,
    __inout CLocaleNameInfoArray * const parrayLocales
    )
{
    ERR     err;
    FUCB    *pfucbCatalog               = pfucbNil;     //  cursor to sequentially scan catalog
    OBJID   objidTable                  = objidNil;
    SYSOBJ  sysobj;
    DATA    dataField;
    CHAR    szTableName[JET_cbNameMost+1];
    BOOL    fLatched                    = fFalse;

    if ( FFMPIsTempDB( ifmp ) )
    {
        AssertSz( fFalse, "Temp DBs don't have MSysLocales or persisted localized indices." );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    //  we will be sequentially scanning the catalog for index records
    FUCBSetSequential( pfucbCatalog );

    err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT );
    Assert( JET_errRecordNotFound != err );
    if ( JET_errNoCurrentRecord == err )
    {
        // catalog should not be empty
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"f2a2844c-c270-4398-8d0b-bdfdd4e4934a" );
        err = ErrERRCheck( JET_errCatalogCorrupted );
    }

    do
    {
        Call( err );

        Assert( !fLatched );
        Assert( !Pcsr( pfucbCatalog )->FLatched() );
        Call( ErrDIRGet( pfucbCatalog ) );
        fLatched = fTrue;

        Assert( fidMSO_ObjidTable.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_ObjidTable,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(OBJID) );
        objidTable = *(UnalignedLittleEndian< OBJID > *) dataField.Pv();


        Assert( fidMSO_Type.FFixed() );
        Call( ErrRECIRetrieveFixedColumn(
                    pfcbNil,
                    pfucbCatalog->u.pfcb->Ptdb(),
                    fidMSO_Type,
                    pfucbCatalog->kdfCurr.data,
                    &dataField ) );
        CallS( err );
        Assert( dataField.Cb() == sizeof(SYSOBJ) );
        sysobj = *(UnalignedLittleEndian< SYSOBJ > *) dataField.Pv();

        if ( sysobjTable == sysobj )
        {
            // Don't really need anymore ... UNLESS the primary unicode index slips through here ... leaving
            // here for similarity with above code.
            Assert( fidMSO_Name.FVar() );
            Call( ErrRECIRetrieveVarColumn(
                        pfcbNil,
                        pfucbCatalog->u.pfcb->Ptdb(),
                        fidMSO_Name,
                        pfucbCatalog->kdfCurr.data,
                        &dataField ) );
            CallS( err );
            const INT cbDataField = dataField.Cb();
            if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
            {
                AssertSz( fFalse, "Invalid fidMSO_Name column in catalog." );
                OSUHAEmitFailureTag( PinstFromPfucb( pfucbCatalog ), HaDbFailureTagCorruption, L"30dd5f19-c9ae-4e96-b57e-4fb6f2f13863" );
                Error( ErrERRCheck( JET_errCatalogCorrupted ) );
            }
            AssertPREFIX( sizeof( szTableName ) > cbDataField );
            UtilMemCpy( szTableName, dataField.Pv(), cbDataField );
            szTableName[cbDataField] = 0;
        }
        else if ( sysobjIndex == sysobj )
        {
            IDBFLAG     fidb;

            Assert( fidMSO_Flags.FFixed() );
            Call( ErrRECIRetrieveFixedColumn(
                        pfcbNil,
                        pfucbCatalog->u.pfcb->Ptdb(),
                        fidMSO_Flags,
                        pfucbCatalog->kdfCurr.data,
                        &dataField ) );
            CallS( err );
            Assert( dataField.Cb() == sizeof(ULONG) );
            fidb = *(UnalignedLittleEndian< IDBFLAG > *) dataField.Pv();

            if ( FIDBLocalizedText( fidb ) )
            {
                SORTID  sortID = g_sortIDNull;
                QWORD   qwSortVersion           = 0;
                WCHAR   wszLocaleName[NORM_LOCALE_NAME_MAX_LENGTH];
                Call( ErrCATIRetrieveLocaleInformation(
                    pfucbCatalog,
                    wszLocaleName,
                    _countof( wszLocaleName ),
                    NULL,
                    &sortID,
                    &qwSortVersion,
                    NULL ) );

                LOCALENAMEINFO li;
                li.m_qwVersion = qwSortVersion;
                li.m_sortID = sortID;
                Call( ErrOSStrCbCopyW( li.m_wszLocaleName, sizeof( li.m_wszLocaleName ), wszLocaleName ) );

                size_t iEntry = parrayLocales->SearchLinear( li, PfnCmpLocaleNameInfo );
                if ( CLocaleInfoArray::iEntryNotFound == iEntry )
                {
                    //  no entry for this LocaleName + version, add one ...
                    li.m_cIndices = 1;
                    CLocaleNameInfoArray::ERR errArray = parrayLocales->ErrSetEntry( parrayLocales->Size(), li );
                    if ( CLocaleNameInfoArray::ERR::errSuccess != errArray )
                    {
                        Assert( CLocaleNameInfoArray::ERR::errOutOfMemory == errArray );
                        Call( ErrERRCheck( JET_errOutOfMemory ) );
                    }
                }
                else
                {
                    //  entry already exists for this index, increment the existing entries m_cIndices ...
                    li = parrayLocales->Entry( iEntry );
                    li.m_cIndices++;
                    CLocaleNameInfoArray::ERR errT = parrayLocales->ErrSetEntry( iEntry, li );
                    Assert( errT == CLocaleNameInfoArray::ERR::errSuccess );
                }
            }
        }
        else
        {
            Assert( sysobjColumn == sysobj
                || sysobjLongValue == sysobj
                || sysobjCallback == sysobj );
        }

        if ( fLatched )
        {
            Assert( Pcsr( pfucbCatalog )->FLatched() );
            Call( ErrDIRRelease( pfucbCatalog ) );
            fLatched = fFalse;
        }

        Assert( !Pcsr( pfucbCatalog )->FLatched() );
        err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
    }
    while ( JET_errNoCurrentRecord != err );

    err = JET_errSuccess;

HandleError:

    Assert( pfucbNil != pfucbCatalog );

    if ( fLatched )
    {
        Assert( Pcsr( pfucbCatalog )->FLatched() );
        Call( ErrDIRRelease( pfucbCatalog ) );
        fLatched = fFalse;
    }

    Assert( !Pcsr( pfucbCatalog )->FLatched() );

    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}

//  Creates the MSysLocales table and stamps the consistency marker.

//  ================================================================
ERR ErrCATCreateMSLocales(
        __in PIB * const ppib,
        const IFMP ifmp )
//  ================================================================
{
    ERR err = JET_errSuccess;
    FMP * const pfmp = &g_rgfmp[ifmp];

    Call( ErrCATICreateMSLocales( ppib, ifmp ) );
    Assert( pfmp->PkvpsMSysLocales() );
    OnDebug( BOOL fMSLocalesConsistencyMarkerT );
    Assert( ErrCATICheckForMSLocalesConsistencyMarker( ppib, ifmp, &fMSLocalesConsistencyMarkerT ) < JET_errSuccess || !fMSLocalesConsistencyMarkerT );

    //  stamp the consistency marker.

    Call( ErrCATIFlagMSLocalesConsistent( ppib, ifmp ) );
    Assert( ErrCATICheckForMSLocalesConsistencyMarker( ppib, ifmp, &fMSLocalesConsistencyMarkerT ) < JET_errSuccess || fMSLocalesConsistencyMarkerT );

HandleError:

    return err;
}

//  Creates and potentially upgrades the MSysLocales table, if the consistency marker is not present.

//  ================================================================
ERR ErrCATCreateOrUpgradeMSLocales(
        __in PIB * const ppib,
        const IFMP ifmp )
//  ================================================================
{
    ERR err = JET_errSuccess;
    FMP * const pfmp = &g_rgfmp[ifmp];

    Call( ErrCATICreateMSLocales( ppib, ifmp ) );
    Assert( pfmp->PkvpsMSysLocales() );

    //  check if the consistency marker can be found in the table.

    BOOL fMSLocalesConsistencyMarker;
    Call( ErrCATICheckForMSLocalesConsistencyMarker( ppib, ifmp, &fMSLocalesConsistencyMarker ) );

    if ( !fMSLocalesConsistencyMarker )
    {
        OnDebug( BOOL fMSLocalesConsistencyMarkerT );

        CATTermMSLocales( pfmp );
        Assert( !pfmp->PkvpsMSysLocales() );

        Call( ErrCATDeleteMSLocales( ppib, ifmp ) );

        Call( ErrFaultInjection( 40304 ) );

        Call( ErrCATICreateMSLocales( ppib, ifmp ) );
        Assert( pfmp->PkvpsMSysLocales() );
        Assert( ErrCATICheckForMSLocalesConsistencyMarker( ppib, ifmp, &fMSLocalesConsistencyMarkerT ) < JET_errSuccess || !fMSLocalesConsistencyMarkerT );

        //  enumerate all indices and fill out MSysLocales.

        Call( ErrCATIPopulateMSLocales( ppib, ifmp ) );
        Assert( ErrCATICheckForMSLocalesConsistencyMarker( ppib, ifmp, &fMSLocalesConsistencyMarkerT ) < JET_errSuccess || !fMSLocalesConsistencyMarkerT );

        Call( ErrFaultInjection( 56688 ) );

        //  stamp the consistency marker.

        Call( ErrCATIFlagMSLocalesConsistent( ppib, ifmp ) );
        Assert( ErrCATICheckForMSLocalesConsistencyMarker( ppib, ifmp, &fMSLocalesConsistencyMarkerT ) < JET_errSuccess || fMSLocalesConsistencyMarkerT );
    }

HandleError:

    return err;
}

//  PfnCmpLocaleInfo is now only used in unit tests.
INT __cdecl PfnCmpLocaleInfo( const LOCALEINFO * pentry1, const LOCALEINFO * pentry2 )
{
    if ( pentry1->m_lcid == pentry2->m_lcid )
    {
        QWORD qwResult = pentry1->m_qwVersion - pentry2->m_qwVersion;
#ifdef DEBUGGER_EXTENSION
        // without DEBUGGER_EXTENSION, it complains that this clause is always false
        if ( qwResult < 0 )
        {
            return -1;
        }
#endif
        return ( qwResult > 0 ) ? 1 : 0;
    }
    return pentry1->m_lcid - pentry2->m_lcid;
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( CATMSysLocales, TestCLocaleInfoArrayWillWorkAsRequiredForMSysLocalesUsage )
{
    LOCALEINFO li = { 0x34, 0x409, 0x2 };

    CLocaleInfoArray        localesarray;
    CLocaleInfoArray::ERR   err;

    //  insert 4 imaginary records

    CHECK( 0 == localesarray.Size() );
    err = localesarray.ErrSetEntry( localesarray.Size(), li );
    CHECK( err == CLocaleInfoArray::ERR::errSuccess );
    li.m_lcid = 1040;
    err = localesarray.ErrSetEntry( localesarray.Size(), li );
    CHECK( err == CLocaleInfoArray::ERR::errSuccess );
    li.m_qwVersion = 0x45;
    err = localesarray.ErrSetEntry( localesarray.Size(), li );
    CHECK( err == CLocaleInfoArray::ERR::errSuccess );
    li.m_lcid = 1046;
    err = localesarray.ErrSetEntry( localesarray.Size(), li );
    CHECK( err == CLocaleInfoArray::ERR::errSuccess );

    //  check that the by offset/iEntry all work to retrieve expected results.

    CHECK( 0x409 == localesarray.Entry( 0 ).m_lcid );
    CHECK( 0x34 == localesarray.Entry( 0 ).m_qwVersion );
    CHECK( 0x2 == localesarray.Entry( 0 ).m_cIndices );
    CHECK( 1040 == localesarray.Entry( 1 ).m_lcid );
    CHECK( 0x34 == localesarray.Entry( 1 ).m_qwVersion );
    CHECK( 1040 == localesarray.Entry( 2 ).m_lcid );
    CHECK( 0x45 == localesarray.Entry( 2 ).m_qwVersion );
    CHECK( 1046 == localesarray.Entry( 3 ).m_lcid );
    CHECK( 0x45 == localesarray.Entry( 3 ).m_qwVersion );

    //  Search for all LCID + version combos we inserted ...
    ULONG i;

    li.m_lcid = 1040;
    li.m_qwVersion = 0x34;

    i = localesarray.SearchLinear( li, PfnCmpLocaleInfo );
    CHECK( i == 1 );

    li.m_lcid = 1040;
    li.m_qwVersion = 0x45;

    i = localesarray.SearchLinear( li, PfnCmpLocaleInfo );
    CHECK( i == 2 );

    li.m_lcid = 0x409;
    li.m_qwVersion = 0x34;

    i = localesarray.SearchLinear( li, PfnCmpLocaleInfo );
    CHECK( i == 0 );

    li.m_lcid = 1046;
    li.m_qwVersion = 0x45;

    i = localesarray.SearchLinear( li, PfnCmpLocaleInfo );
    CHECK( i == 3 );

    //  search for non-existant LCID

    li.m_lcid = 1047;
    li.m_qwVersion = 0x34;

    i = localesarray.SearchLinear( li, PfnCmpLocaleInfo );
    size_t it = localesarray.SearchLinear( li, PfnCmpLocaleInfo );
    CHECK( it == localesarray.iEntryNotFound );
}

JETUNITTEST( CATMSysLocales, TestCLocaleNameInfoArrayWillWorkAsRequiredForMSysLocalesUsage )
{
    SORTID sortID;
    UINT wAssertActionSaved = COSLayerPreInit::SetAssertAction( JET_AssertSkipAll );    // GUID is too long.
    SortIDWsz( L"5d573fd4d76e4d55aaf7296bf42ac89f", &sortID );
    (void)COSLayerPreInit::SetAssertAction( wAssertActionSaved );   // reset

    LOCALENAMEINFO li;

    li.m_cIndices = 0x2;
    li.m_qwVersion = 0x34;
    li.m_sortID = sortID;
    StringCchCopyW( li.m_wszLocaleName, _countof( li.m_wszLocaleName ), L"en-us" );

    CLocaleNameInfoArray        localesarray;
    CLocaleNameInfoArray::ERR   err;

    //  insert 5 imaginary records

    CHECK( 0 == localesarray.Size() );
    err = localesarray.ErrSetEntry( localesarray.Size(), li );
    CHECK( err == CLocaleNameInfoArray::ERR::errSuccess );

    StringCchCopyW( li.m_wszLocaleName, _countof( li.m_wszLocaleName ), L"pt-br" );
    err = localesarray.ErrSetEntry( localesarray.Size(), li );
    CHECK( err == CLocaleNameInfoArray::ERR::errSuccess );

    li.m_qwVersion = 0x45;
    err = localesarray.ErrSetEntry( localesarray.Size(), li );
    CHECK( err == CLocaleNameInfoArray::ERR::errSuccess );

    StringCchCopyW( li.m_wszLocaleName, _countof( li.m_wszLocaleName ), L"pt-pt" );
    err = localesarray.ErrSetEntry( localesarray.Size(), li );
    CHECK( err == CLocaleNameInfoArray::ERR::errSuccess );

    li.m_sortID.Data1++;
    err = localesarray.ErrSetEntry( localesarray.Size(), li );
    CHECK( err == CLocaleNameInfoArray::ERR::errSuccess );

    li.m_sortID.Data1--;

    //  check that the by offset/iEntry all work to retrieve expected results.

    CHECK( 0 == LOSStrCompareW( localesarray.Entry( 0 ).m_wszLocaleName, L"en-us", _countof( li.m_wszLocaleName ) ) );
    CHECK( 0x34 == localesarray.Entry( 0 ).m_qwVersion );
    CHECK( 0x2 == localesarray.Entry( 0 ).m_cIndices );

    CHECK( 0 == LOSStrCompareW( localesarray.Entry( 1 ).m_wszLocaleName, L"pt-br", _countof( li.m_wszLocaleName ) ) );
    CHECK( 0x34 == localesarray.Entry( 1 ).m_qwVersion );

    CHECK( 0 == LOSStrCompareW( localesarray.Entry( 2 ).m_wszLocaleName, L"pt-br", _countof( li.m_wszLocaleName ) ) );
    CHECK( 0x45 == localesarray.Entry( 2 ).m_qwVersion );

    CHECK( 0 == LOSStrCompareW( localesarray.Entry( 3 ).m_wszLocaleName, L"pt-pt", _countof( li.m_wszLocaleName ) ) );
    CHECK( 0x45 == localesarray.Entry( 3 ).m_qwVersion );

    //  Search for all LocaleName + version combos we inserted ...
    ULONG i;

    StringCchCopyW( li.m_wszLocaleName, _countof( li.m_wszLocaleName ), L"pt-br" );
    li.m_qwVersion = 0x34;

    i = localesarray.SearchLinear( li, PfnCmpLocaleNameInfo );
    CHECK( i == 1 );

    StringCchCopyW( li.m_wszLocaleName, _countof( li.m_wszLocaleName ), L"pt-br" );
    li.m_qwVersion = 0x45;

    i = localesarray.SearchLinear( li, PfnCmpLocaleNameInfo );
    CHECK( i == 2 );

    StringCchCopyW( li.m_wszLocaleName, _countof( li.m_wszLocaleName ), L"en-us" );
    li.m_qwVersion = 0x34;

    i = localesarray.SearchLinear( li, PfnCmpLocaleNameInfo );
    CHECK( i == 0 );

    StringCchCopyW( li.m_wszLocaleName, _countof( li.m_wszLocaleName ), L"pt-pt" );
    li.m_qwVersion = 0x45;

    i = localesarray.SearchLinear( li, PfnCmpLocaleNameInfo );
    CHECK( i == 3 );

    li.m_sortID.Data1++;

    i = localesarray.SearchLinear( li, PfnCmpLocaleNameInfo );
    CHECK( i == 4 );

    //  search for non-existant LocaleName/NLSVersion/DefinedVersion/SortID

    li.m_sortID.Data1++;

    size_t it = localesarray.SearchLinear( li, PfnCmpLocaleNameInfo );
    CHECK( it == localesarray.iEntryNotFound );

    li.m_sortID.Data1 -= 2;
    li.m_qwVersion++;

    it = localesarray.SearchLinear( li, PfnCmpLocaleNameInfo );
    CHECK( it == localesarray.iEntryNotFound );

    li.m_qwVersion += (QWORD(1) << 32) - 1;

    it = localesarray.SearchLinear( li, PfnCmpLocaleNameInfo );
    CHECK( it == localesarray.iEntryNotFound );

    StringCchCopyW( li.m_wszLocaleName, _countof( li.m_wszLocaleName ), L"st-kg" );
    li.m_qwVersion = 0x34;

    it = localesarray.SearchLinear( li, PfnCmpLocaleNameInfo );
    CHECK( it == localesarray.iEntryNotFound );
}

#endif // ENABLE_JET_UNIT_TEST

INLINE ERR ErrCATIParseLocaleNameInfo(
    __in const PCWSTR wszLocaleEntryKey,
    __out_ecount(NORM_LOCALE_NAME_MAX_LENGTH) WCHAR * wszLocaleName,
    __out QWORD * pqwSortedVersion,
    __out SORTID * psortID )
{
    PCWSTR wszCurr;
    WCHAR wszSortID[PERSISTED_SORTID_MAX_LENGTH];
    ULONG cchLocaleName;

    Assert( ( NULL != wszLocaleName ) && ( NULL != pqwSortedVersion ) && ( NULL != psortID ) );

    //
    //  First, grab the LocaleName out of the key
    //
    wszCurr = wcschr( wszLocaleEntryKey, L'=' );
    if ( wszCurr == NULL ||
         *wszCurr == L'\0' ||
         wszCurr == wszLocaleEntryKey )
    {
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }
    else if ( *( wszCurr + 1 ) == L'\0' ||
              *( wszCurr + 1 ) == L',' )
    {
        // Empty locale name is valid.
        wszCurr++;
        cchLocaleName = 0;
    }
    else
    {
        wszCurr++;
        cchLocaleName = wcscspn( wszCurr, L"," );

        if ( ( cchLocaleName == 0 ) || ( cchLocaleName >= NORM_LOCALE_NAME_MAX_LENGTH ) )
        {
            return ErrERRCheck( JET_errDatabaseCorrupted );
        }
    }

#ifdef DEBUG
    const WCHAR * wszExpectedLocaleName = L"LocaleName=";
    Assert( 0 == LOSStrCompareW( wszLocaleEntryKey, wszExpectedLocaleName, LOSStrLengthW( wszExpectedLocaleName ) ) );
#endif // DEBUG

    StringCchCopyW( wszLocaleName, cchLocaleName + 1, wszCurr );
    wszLocaleName[cchLocaleName] = L'\0';

    //
    //  second, grab the Sort Version out of the key
    //
    wszCurr = wcschr( wszCurr, L',' );

    if ( wszCurr == NULL ||
         *wszCurr == L'\0' ||
         *( wszCurr + 1 ) == L'\0' )
    {
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }

#ifdef DEBUG
    const WCHAR * wszExpectedSortID = L",SortID=";
    Assert( 0 == LOSStrCompareW( wszCurr, wszExpectedSortID, LOSStrLengthW( wszExpectedSortID ) ) );
#endif // DEBUG

    wszCurr = wcschr( wszCurr, L'=' );
    if ( wszCurr == NULL ||
         *wszCurr == L'\0' ||
         *( wszCurr + 1 ) == L'\0' )
    {
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }

    wszCurr++;
    ULONG cchSortID = wcscspn( wszCurr, L"," );

    C_ASSERT( _countof( wszSortID ) == PERSISTED_SORTID_MAX_LENGTH );
    if ( ( cchSortID == 0 ) || ( cchSortID != PERSISTED_SORTID_MAX_LENGTH - 1 ) )
    {
        AssertSz( fFalse, "The sort ID was not of the right size.  Should be exactly the size we put in." );
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }

    StringCchCopyW( wszSortID, cchSortID + 1, wszCurr );
    wszSortID[cchSortID] = L'\0';
    SortIDWsz( wszSortID, psortID );

    //
    //  Third, grab the Sort Version out of the key
    //
    wszCurr = wcschr( wszCurr, L',' );

    if ( wszCurr == NULL ||
         *wszCurr == L'\0' ||
         *( wszCurr + 1 ) == L'\0' )
    {
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }

#ifdef DEBUG
    const WCHAR * wszExpectedVer = L",Ver=";
    Assert( 0 == LOSStrCompareW( wszCurr, wszExpectedVer, LOSStrLengthW( wszExpectedVer ) ) );
#endif // DEBUG

    wszCurr = wcschr( wszCurr, L'=' );
    if ( wszCurr == NULL ||
         *wszCurr == L'\0' ||
         * (wszCurr + 1 ) == L'\0' )
    {
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }

    wszCurr++;

    WCHAR * pwszNull = NULL;
    *pqwSortedVersion = _wcstoui64( wszCurr, &pwszNull, 16 );

    if ( *pwszNull != L'\0' ) // expect that's the end of the key.
    {
        AssertSz( fFalse, "More key beyond what was expected in the MSysLocales Locale= type entry." );
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }

    return JET_errSuccess;
}

//  Enumerates all the indices and populates MSysLocales.

ERR ErrCATIPopulateMSLocales(
    __in PIB * const ppib,
    const IFMP ifmp
    )
{
    ERR err = JET_errSuccess;

    CLocaleNameInfoArray    localesarray;

    IFMP ifmpOpen = ifmpNil;
    Call( ErrDBOpenDatabase( ppib, g_rgfmp[ifmp].WszDatabaseName(), &ifmpOpen, NO_GRBIT ) );
    Assert( ifmpOpen == ifmp );
    Assert( JET_wrnFileOpenReadOnly != err );   // shouldn't have tried to populate if DB is RO.

    Call( ErrCATIAccumulateIndexLocales( ppib, ifmp, &localesarray ) );

    for ( ULONG i = 0; i < localesarray.Size(); i++ )
    {
        const LOCALENAMEINFO liFromMSysObjects = localesarray.Entry( i );

        WCHAR wszLocaleNameSortVerKey[g_cchLocaleNameSortIDSortVersionLocaleKeyMax];

        WCHAR wszSortID[PERSISTED_SORTID_MAX_LENGTH] = L"";
        WszCATFormatSortID( liFromMSysObjects.m_sortID, wszSortID, _countof( wszSortID ) );
        OSStrCbFormatW( wszLocaleNameSortVerKey, sizeof( wszLocaleNameSortVerKey ), g_wszLocaleNameSortIDSortVersionLocaleKey, liFromMSysObjects.m_wszLocaleName, wszSortID, liFromMSysObjects.m_qwVersion );

#ifdef DEBUG
    {
        WCHAR wszLocaleNameT[NORM_LOCALE_NAME_MAX_LENGTH];
        QWORD qwSortVersionT;
        SORTID sortIDT;
        Assert( JET_errSuccess == ErrCATIParseLocaleNameInfo( wszLocaleNameSortVerKey, wszLocaleNameT, &qwSortVersionT, &sortIDT ) );
        Assert( 0 == _wcsicmp( liFromMSysObjects.m_wszLocaleName, wszLocaleNameT ) );
        Assert( liFromMSysObjects.m_qwVersion == qwSortVersionT );
        Assert( FSortIDEquals( &liFromMSysObjects.m_sortID, &sortIDT ) );
    }
#endif

        Call( g_rgfmp[ifmp].PkvpsMSysLocales()->ErrKVPSetValue( wszLocaleNameSortVerKey, liFromMSysObjects.m_cIndices ) );

        OSTrace( JET_tracetagUpgrade, OSFormat( "Inserted MSysLocales[%d] = %ws, cCount = %d\n", i, wszLocaleNameSortVerKey, liFromMSysObjects.m_cIndices ) );
    }

HandleError:

    if ( ifmpOpen != ifmpNil )
    {
        CallS( ErrDBCloseDatabase( ppib, ifmpOpen, 0 ) );
    }

    return err;
}

//  Note: this function only loads LCID information. ErrCATIGetLocaleNameInfo will load
//  the locale name information or punt to this function in case it's a legacy LCID entry.
LOCAL ERR ErrCATIGetLocaleInfo(
    __in CKVPStore::CKVPSCursor * const pkvpscursor,
    __inout LOCALEINFO * plocaleinfo
    )
{
    ERR err             = JET_errSuccess;
    PWSTR       pwszLCID    = NULL;

    Assert( NULL != pkvpscursor );
    Assert( NULL != plocaleinfo );

    //
    //  First, grab the LCID out of the key
    //
    OSStrCharFindW( pkvpscursor->WszKVPSCursorCurrKey(), L'=', &pwszLCID );
    if ( pwszLCID == NULL ||
         *pwszLCID == L'\0' ||
         *( pwszLCID + 1 ) == L'\0' )
    {
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    pwszLCID++;
    WCHAR * pwszVer = NULL;
    DWORD lcid = wcstol( pwszLCID, &pwszVer, 10 );
    if ( lcid == 0 )
    {
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    //
    //  Second, grab the Sort Version out of the key
    //
    OSStrCharFindW( pwszVer, L'=', &pwszVer );
    if ( pwszVer == NULL ||
         *pwszVer == L'\0' ||
         *( pwszVer + 1 ) == L'\0' )
    {
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    pwszVer++;
    WCHAR * pwszNull = NULL;
    QWORD qwSortedVersion = _wcstoui64( pwszVer, &pwszNull, 16 );
    Expected( *pwszNull == L'\0' ); // expect that's the end of the key.

    //
    //  Last, grab number of indices this applies to
    //
    INT cIndices = 0;
    err = pkvpscursor->ErrKVPSCursorGetValue( &cIndices );
    if ( err != JET_errSuccess )
    {
        Assert( ErrcatERRLeastSpecific( err ) != JET_errcatApi );
        if ( ErrcatERRLeastSpecific( err ) != JET_errcatOperation )
        {
            err = ErrERRCheck( JET_errDatabaseCorrupted );
        }
    }

    //
    //  Set out parameters
    //
    plocaleinfo->m_lcid = lcid;
    plocaleinfo->m_qwVersion = qwSortedVersion;
    plocaleinfo->m_cIndices = cIndices;

HandleError:

    return err;
}

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( CATMSysLocales, SORTIDConversion )
{
    SORTID sortIDAfter;
    SORTID sortID = { 0x1234567, 0x2345, 0xa987, { 0x20, 0x12, 0x03, 0x22, 0xa4, 0xfd, 0xe8, 0x95 } };

    WCHAR wsz[PERSISTED_SORTID_MAX_LENGTH];
    WszCATFormatSortID( sortID, wsz, _countof( wsz ) );

    WCHAR wszFormat[PERSISTED_SORTID_MAX_LENGTH];
    WszCATFormatSortID( sortID, wszFormat, _countof( wszFormat ) );

    wprintf( L"wsz from SortIDWsz() is %ws, and wsz from WszCATFormatSortID() is %ws\n", wsz, wszFormat );

    SortIDWsz( wsz, &sortIDAfter );
    CHECK( FSortIDEquals( &sortID, &sortIDAfter ) );

    SortIDWsz( wszFormat, &sortIDAfter );
    CHECK( FSortIDEquals( &sortID, &sortIDAfter ) );
}

JETUNITTEST( CATMSysLocales, SORTIDConversionAdv )
{
    SORTID sortIDAfter;
    WCHAR wsz[PERSISTED_SORTID_MAX_LENGTH];
    SORTID sortID = { 0x1234567, 0x2345, 0xa987, { 0x20, 0x12, 0x03, 0x22, 0xa4, 0xfd, 0xe8, 0x95 } };

    for ( ULONG i = 0; i < 100; i++ )
    {
        for ( ULONG ib = 0; ib < sizeof(sortID); ib++ )
        {
            ((BYTE*)(&sortID))[ib] = rand() % 255;
        }
        WszCATFormatSortID( sortID, wsz, _countof( wsz ) );
        SortIDWsz( wsz, &sortIDAfter );
        CHECK( FSortIDEquals( &sortID, &sortIDAfter ) );
    }
}

JETUNITTEST( CATMSysLocales, TestLocaleNameInfoParsing )
{
    WCHAR wszLocaleName[NORM_LOCALE_NAME_MAX_LENGTH];
    QWORD qwSortedVersion;
    SORTID sortID;

    UINT wAssertActionSaved = COSLayerPreInit::SetAssertAction( JET_AssertSkipAll );
    (void)COSLayerPreInit::SetAssertAction( wAssertActionSaved );   // reset

    SORTID sortIDComparison;
    SortIDWsz( L"00000048-30a8-0000-86AE-00000000A82A", &sortIDComparison );
    const WCHAR * wsz1 = L"LocaleName=en-us,SortID=00000048-30a8-0000-86AE-00000000A82A,Ver=409";
    CHECKCALLS( ErrCATIParseLocaleNameInfo( wsz1, wszLocaleName, &qwSortedVersion, &sortID ) );
    CHECK( FSortIDEquals( &sortID, &sortIDComparison ) );
    CHECK( 0 == LOSStrCompareW( L"en-us", wszLocaleName, _countof( wszLocaleName ) ) );
    CHECK( 1033 == qwSortedVersion );

    const WCHAR * wsz2 = L"";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz2, wszLocaleName, &qwSortedVersion, &sortID ) );

    const WCHAR * wsz3 = L"ugh?";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz3, wszLocaleName, &qwSortedVersion, &sortID ) );

    const WCHAR * wsz4 = L"LocaleName=en-us";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz4, wszLocaleName, &qwSortedVersion, &sortID ) );

    const WCHAR * wsz5 = L"LocaleName=";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz5, wszLocaleName, &qwSortedVersion, &sortID ) );

    const WCHAR * wsz5dot1 = L"LocaleName=,";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz5dot1, wszLocaleName, &qwSortedVersion, &sortID ) );

    // Check that we don't go past the end of string (embedded NULL).
    const WCHAR * wsz5dot2 = L"LocaleName=\0,SortID=00000000-0000-0000-0000-000000000000,Ver=6010100060101";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz5dot2, wszLocaleName, &qwSortedVersion, &sortID ) );

    // Check that we don't go past the end of string (embedded NULL).
    const WCHAR * wsz5dot3 = L"LocaleName=\0SortID=00000000-0000-0000-0000-000000000000,Ver=6010100060101";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz5dot3, wszLocaleName, &qwSortedVersion, &sortID ) );

    const WCHAR * wsz6 = L"LocaleName=1234";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz6, wszLocaleName, &qwSortedVersion, &sortID ) );

    const WCHAR * wsz7 = L"LocaleName=hemanrejgrekgfjrgjerjgierhjgirehgirehigherighiehgihrighirehgierhgihreihgirehgiherighrgih";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz7, wszLocaleName, &qwSortedVersion, &sortID ) );

    const WCHAR * wsz8 = L"LocaleName=en-us,";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz8, wszLocaleName, &qwSortedVersion, &sortID ) );

    const WCHAR * wsz9 = L"LocaleName=en-us,SortID=00000048-30a8-0000-86AE-00000000A82A";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz9, wszLocaleName, &qwSortedVersion, &sortID ) );

    (void)COSLayerPreInit::SetAssertAction( JET_AssertSkipAll );    // sortID is mis-persisted too long
    const WCHAR * wsz10 = L"LocaleName=en-us,SortID=00000048-30a8-0000-86AE-00000000A82A123";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz10, wszLocaleName, &qwSortedVersion, &sortID ) );
    (void)COSLayerPreInit::SetAssertAction( wAssertActionSaved );

    (void)COSLayerPreInit::SetAssertAction( JET_AssertSkipAll );    // sortID is mis-persisted too long
    const WCHAR * wsz11 = L"LocaleName=en-us,SortID=00000048-30a8-0000-86AE-00000000A82A12311";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz11, wszLocaleName, &qwSortedVersion, &sortID ) );
    (void)COSLayerPreInit::SetAssertAction( wAssertActionSaved );

    const WCHAR * wsz12 = L"LocaleName=en-us,SortID=";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz12, wszLocaleName, &qwSortedVersion, &sortID ) );

    (void)COSLayerPreInit::SetAssertAction( JET_AssertSkipAll );    // sortID is mis-persisted too short
    const WCHAR * wsz13 = L"LocaleName=en-us,SortID=aba";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz13, wszLocaleName, &qwSortedVersion, &sortID ) );
    (void)COSLayerPreInit::SetAssertAction( wAssertActionSaved );

    const WCHAR * wsz14 = L"LocaleName=en-us,SortID=00000048-30a8-0000-86AE-00000000A82A,";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz14, wszLocaleName, &qwSortedVersion, &sortID ) );

    (void)COSLayerPreInit::SetAssertAction( JET_AssertSkipAll );    // trailing comma, not properly terminated
    const WCHAR * wsz15 = L"LocaleName=en-us,SortID=00000048-30a8-0000-86AE-00000000A82A,Ver=1234,";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz15, wszLocaleName, &qwSortedVersion, &sortID ) );
    (void)COSLayerPreInit::SetAssertAction( wAssertActionSaved );

    const WCHAR * wsz16 = L"LocaleName=en-us,SortID=00000048-30a8-0000-86AE-00000000A82A,Ver=";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz16, wszLocaleName, &qwSortedVersion, &sortID ) );

    (void)COSLayerPreInit::SetAssertAction( JET_AssertSkipAll );    // doesn't parse as number, so invalid left over
    const WCHAR * wsz17 = L"LocaleName=en-us,SortID=00000048-30a8-0000-86AE-00000000A82A,Ver=zseze";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz17, wszLocaleName, &qwSortedVersion, &sortID ) );
    (void)COSLayerPreInit::SetAssertAction( wAssertActionSaved );

    const WCHAR * wsz18 = L"=";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz18, wszLocaleName, &qwSortedVersion, &sortID ) );

    (void)COSLayerPreInit::SetAssertAction( JET_AssertSkipAll );    // invalid key specification
    const WCHAR * wsz19 = L"LCID=1041,Ver=1234";
    CHECK( JET_errDatabaseCorrupted == ErrCATIParseLocaleNameInfo( wsz19, wszLocaleName, &qwSortedVersion, &sortID ) );
    (void)COSLayerPreInit::SetAssertAction( wAssertActionSaved );

    const WCHAR * wsz20 = L"LocaleName=en-US,SortID=00000000-0000-0000-0000-000000000000,Ver=6010100060101";
    memset( &sortIDComparison, 0, sizeof( SORTID ) );
    CHECKCALLS( ErrCATIParseLocaleNameInfo( wsz20, wszLocaleName, &qwSortedVersion, &sortID ) );
    CHECK( 0 == LOSStrCompareW( L"en-US", wszLocaleName, _countof( wszLocaleName ) ) );
    CHECK( FSortIDEquals( &sortID, &sortIDComparison ) );
    CHECK( 0x6010100060101 == qwSortedVersion );

    // Empty locale name.
    const WCHAR * wsz21 = L"LocaleName=,SortID=00000000-0000-0000-0000-000000000000,Ver=6010200060101";
    memset( &sortIDComparison, 0, sizeof( SORTID ) );
    CHECKCALLS( ErrCATIParseLocaleNameInfo( wsz21, wszLocaleName, &qwSortedVersion, &sortID ) );
    CHECK( 0 == LOSStrCompareW( L"", wszLocaleName, _countof( wszLocaleName ) ) );
    CHECK( FSortIDEquals( &sortID, &sortIDComparison ) );
    CHECK( 0x6010200060101 == qwSortedVersion );
}

#endif // ENABLE_JET_UNIT_TEST

ERR ErrCATIGetLocaleNameInfo(
    CKVPStore::CKVPSCursor * const pkvpscursor,
    LOCALENAMEINFO * plocaleinfo
    )
{
    ERR err = JET_errSuccess;
    SORTID sortID;
    QWORD qwSortedVersion;
    WCHAR wszLocaleName[NORM_LOCALE_NAME_MAX_LENGTH];

    if ( NULL != wcsstr( pkvpscursor->WszKVPSCursorCurrKey(), L"LCID" ) )
    {
        //old key format in KVP which was L"LCID=%d,Ver=%I64x"
        LOCALEINFO lcidInfo;
        err = ErrCATIGetLocaleInfo( pkvpscursor, &lcidInfo );
        if ( err == JET_errSuccess )
        {
            QWORD qwVersion;

            CallR( ErrNORMLcidToLocale( lcidInfo.m_lcid, plocaleinfo->m_wszLocaleName, NORM_LOCALE_NAME_MAX_LENGTH ) );
            CallR( ErrNORMGetSortVersion( plocaleinfo->m_wszLocaleName, &qwVersion, &(plocaleinfo->m_sortID) ) );
            plocaleinfo->m_qwVersion = lcidInfo.m_qwVersion;
            plocaleinfo->m_cIndices = lcidInfo.m_cIndices;
        }

        return err;
    }

    Call( ErrCATIParseLocaleNameInfo(
        pkvpscursor->WszKVPSCursorCurrKey(),
        wszLocaleName,
        &qwSortedVersion,
        &sortID ) );

    //
    //  Last, grab number of indices this applies to
    //
    INT cIndices = 0;
    ERR errGetValue;
    if ( ( errGetValue = pkvpscursor->ErrKVPSCursorGetValue( &cIndices ) ) < JET_errSuccess )
    {
        Assert( ErrcatERRLeastSpecific( errGetValue ) != JET_errcatApi );
        if ( ErrcatERRLeastSpecific( errGetValue ) == JET_errcatOperation )
        {
            return errGetValue;
        }
        else
        {
            return ErrERRCheck( JET_errDatabaseCorrupted );
        }
    }

    Call( err );

    //
    //  Set out parameters
    //
    Call( ErrOSStrCbCopyW( plocaleinfo->m_wszLocaleName, sizeof( plocaleinfo->m_wszLocaleName ), wszLocaleName ) );
    plocaleinfo->m_sortID = sortID;
    plocaleinfo->m_qwVersion = qwSortedVersion;
    plocaleinfo->m_cIndices = cIndices;

HandleError:

    return err;
}

ERR ErrCATIConsistencyMarkerKey(
    CKVPStore::CKVPSCursor * const pkvpscursor,
    BOOL * const pfConsistencyMarker
    )
{
    ERR err = JET_errSuccess;

    *pfConsistencyMarker = fFalse;

    INT iMSLocalesValue;
    Call( pkvpscursor->ErrKVPSCursorGetValue( &iMSLocalesValue ) );

    *pfConsistencyMarker = FCATIIsMSLocalesConsistencyMarker( pkvpscursor->WszKVPSCursorCurrKey(), iMSLocalesValue );

HandleError:

    return err;
}

//  This opens the MSysLocales KVPStore and checks that all locales are available and
//  at the correct sort version.

ERR ErrCATCheckForOutOfDateLocales(
        const IFMP ifmp,
        __out BOOL * const pfOutOfDateNLSVersion )
{
    ERR err = JET_errSuccess;
    CKVPStore kvpsMSysLocales( ifmp, wszMSLocales );
    CKVPStore::CKVPSCursor * pkvpscursor = NULL;
    BOOL fInitKVP = fFalse;

    *pfOutOfDateNLSVersion = fFalse;

    // We don't use g_rgfmp[ifmp].PkvpsMSysLocales() for 2 reasons, (A) it's not initialized
    // yet, and (B) it's not read only ...
    Call( kvpsMSysLocales.ErrKVPInitStore( NULL, CKVPStore::eReadOnly, g_dwMSLocalesMajorVersions ) );

    fInitKVP = fTrue;

    Assert( g_wszLocaleNameSortIDSortVersionLocaleKey[0] == L'L' /* from LCID= old key format */ );
    Assert( g_wszLocaleNameSortIDSortVersionLocaleKey[0] != g_wszMSLocalesConsistencyMarkerKey[0] );    // L* won't grab consistency marker

    Alloc( pkvpscursor = new CKVPStore::CKVPSCursor( &kvpsMSysLocales, L"L*" ) );

    while( ( err = pkvpscursor->ErrKVPSCursorLocateNextKVP() ) >= JET_errSuccess )
    {
        LOCALENAMEINFO li;
        Call( ErrCATIGetLocaleNameInfo( pkvpscursor, &li ) );

        if ( li.m_cIndices == 0 )
        {
            continue;
        }

        QWORD qwSortVersionOS;
        SORTID sortID;

        Call( ErrNORMGetSortVersion( li.m_wszLocaleName, &qwSortVersionOS, &sortID, fFalse /*fErrorOnInvalidId*/ ) );

        if ( !FNORMNLSVersionEquals( li.m_qwVersion, qwSortVersionOS ) )
        {
            WCHAR wszVersion1[ 32 ];
            WCHAR wszVersion2[ 32 ];
            WCHAR wszSortID1[ PERSISTED_SORTID_MAX_LENGTH ];
            WCHAR wszSortID2[ PERSISTED_SORTID_MAX_LENGTH ];
            const WCHAR *rgsz[6];

            WszCATFormatSortID( li.m_sortID, wszSortID1, _countof( wszSortID1 ) );
            WszCATFormatSortID( sortID, wszSortID2, _countof( wszSortID2 ) );

            OSStrCbFormatW( wszVersion1, sizeof(wszVersion1), L"%016I64X", li.m_qwVersion );
            OSStrCbFormatW( wszVersion2, sizeof(wszVersion2), L"%016I64X", qwSortVersionOS );

            rgsz[0] = g_rgfmp[ifmp].WszDatabaseName();
            rgsz[1] = li.m_wszLocaleName;
            rgsz[2] = wszSortID1;
            rgsz[3] = wszVersion1;
            rgsz[4] = wszSortID2;
            rgsz[5] = wszVersion2;

            UtilReportEvent(
                    eventWarning,
                    DATA_DEFINITION_CATEGORY,
                    DATABASE_HAS_OUT_OF_DATE_INDEX,
                    6,
                    rgsz,
                    0,
                    NULL,
                    PinstFromIfmp( ifmp ) );

            if ( li.m_qwVersion > qwSortVersionOS )
            {
                //  This database needs upgrading for localization ... no need to continue, as this is the major
                //  version and there isn't much we can do ...
                OSTrace( JET_tracetagUpgrade, OSFormat( "Due to localeName = %ws, which is a higher sort version (%#I64x) than the current OS (%#I64x), this DB needs index upgrades.\n",
                            li.m_wszLocaleName, li.m_qwVersion, qwSortVersionOS ) );
                *pfOutOfDateNLSVersion = fTrue;
            }

            else
            {
                Assert( li.m_qwVersion != qwSortVersionOS );

                OSTrace( JET_tracetagUpgrade, OSFormat( "Due to localeName = %ws, which is a different sort versions (%#I64x) than the current OS (%#I64x), this DB needs index rebuilds.\n",
                            li.m_wszLocaleName, li.m_qwVersion, qwSortVersionOS ) );
                *pfOutOfDateNLSVersion = fTrue;
            }
            continue;
        }

        if ( !FSortIDEquals( &(li.m_sortID), &sortID ) )
        {
            WCHAR wszVersion1[ 32 ];
            WCHAR wszVersion2[ 32 ];
            const WCHAR *rgsz[6];
            WCHAR wszSortID1[ PERSISTED_SORTID_MAX_LENGTH ];
            WCHAR wszSortID2[ PERSISTED_SORTID_MAX_LENGTH ];

            WszCATFormatSortID( li.m_sortID, wszSortID1, _countof( wszSortID1 ) );
            WszCATFormatSortID( sortID, wszSortID2, _countof( wszSortID2 ) );

            OSStrCbFormatW( wszVersion1, sizeof(wszVersion1), L"%016I64X", li.m_qwVersion );
            OSStrCbFormatW( wszVersion2, sizeof(wszVersion2), L"%016I64X", qwSortVersionOS );

            rgsz[0] = g_rgfmp[ifmp].WszDatabaseName();
            rgsz[1] = li.m_wszLocaleName;
            rgsz[2] = wszSortID1;
            rgsz[3] = wszVersion1;
            rgsz[4] = wszSortID2;
            rgsz[5] = wszVersion2;

            UtilReportEvent(
                    eventWarning,
                    DATA_DEFINITION_CATEGORY,
                    DATABASE_HAS_OUT_OF_DATE_INDEX,
                    6,
                    rgsz,
                    0,
                    NULL,
                    PinstFromIfmp( ifmp ) );

            OSTrace( JET_tracetagUpgrade, OSFormat( "Due to localeName = %ws, which changed Sort ID from %ws to %ws, this DB needs index upgrades.\n",
                        li.m_wszLocaleName,
                        wszSortID1,
                        wszSortID2 ) );

            *pfOutOfDateNLSVersion = fTrue;
            continue;
        }
        Assert( FNORMNLSVersionEquals( li.m_qwVersion, qwSortVersionOS ) || *pfOutOfDateNLSVersion );
    }

    if ( err == JET_errNoCurrentRecord )
    {
        err = JET_errSuccess;
    }
    Call( err );

HandleError:
    // If we could not find the MSLocales table, we should go ahead an try to make do without it.
    if ( JET_errObjectNotFound == err )
    {
        err = JET_errSuccess;
    }

    if ( pkvpscursor )
    {
        pkvpscursor->KVPSCursorEnd();
        delete pkvpscursor;
    }

    if ( fInitKVP )
    {
        kvpsMSysLocales.KVPTermStore();
    }

    return err;
}


ERR ErrCATVerifyMSLocales(
    __in PIB * const ppib,
    __in const IFMP ifmp,
    __in const BOOL fFixupMSysLocales
    )
{
    ERR err = JET_errSuccess;
    CKVPStore::CKVPSCursor * pkvpscursorMSysLocalesTable = NULL;
    CLocaleNameInfoArray    arrayLocalesInDB;
#ifdef DEBUG
    BOOL fInvariantLocaleSeen = fFalse;
#endif

    //  Ensure we have the MSysLocales table available to us. It may not be present
    //  in a ReadOnly attach.

    Assert( g_fRepair || g_rgfmp[ifmp].FReadOnlyAttach() || g_rgfmp[ifmp].PkvpsMSysLocales() );

    //  Open the database so we can walk the catalog.

    IFMP ifmpOpen = ifmpNil;
    Call( ErrDBOpenDatabase( ppib, g_rgfmp[ifmp].WszDatabaseName(), &ifmpOpen, NO_GRBIT ) );
    Assert( ifmpOpen == ifmp );

    //
    //  Collect all the index info from the traditional catalog / MSysObject (actual index definitions).
    //

    Call( ErrCATIAccumulateIndexLocales( ppib, ifmp, &arrayLocalesInDB ) );

    //  Validate no empty LocaleName values from DB (code below depends upon it), and version is
    //  reasonable and no zero counts.

    for ( ULONG i = 0; i < arrayLocalesInDB.Size(); i++ )
    {
#ifdef DEBUG
        LOCALENAMEINFO liFromMSysObjects = arrayLocalesInDB.Entry( i );
        if ( fInvariantLocaleSeen )
        {
            // There can be up to one 'empty' locale in the table.
            // (Or can there be multiple, if this database was used on different OS versions?)
            Assert( liFromMSysObjects.m_wszLocaleName[0] != L'\0' );
        }
        else if ( liFromMSysObjects.m_wszLocaleName[0] == L'\0' )
        {
            fInvariantLocaleSeen = fTrue;
        }
#endif

        // Very old upgrade database (such as those used in the MSUAndBackwardsCompatibility test) may
        // have the sort version set to 0.  So we can only Assert() this once we stop supporting those
        // old versions.
        //Expected( liFromMSysObjects.m_qwVersion != 0 );

        Expected( liFromMSysObjects.m_cIndices != 0 );
    }

    //  Bail if the MSLocales table doesn't exist.
    if ( !g_rgfmp[ifmp].PkvpsMSysLocales() )
    {
        Assert( g_fRepair || g_rgfmp[ifmp].FReadOnlyAttach() );
        if ( g_fRepair || g_rgfmp[ifmp].FReadOnlyAttach() )
        {
            // If this is a RO attach, one of the below functions will fail, and we
            // do not want to bubble that error up.
            err = JET_errSuccess;
            goto HandleError;
        }
    }

    pkvpscursorMSysLocalesTable = new CKVPStore::CKVPSCursor( g_rgfmp[ifmp].PkvpsMSysLocales(), L"L*" );
    Alloc( pkvpscursorMSysLocalesTable );

    //
    //  Walk all the MSysLocale entries, matching their contents to the MSysObjects / catalogs contents ...
    //

    while( ( err = pkvpscursorMSysLocalesTable->ErrKVPSCursorLocateNextKVP() ) >= JET_errSuccess )
    {
        //  Retrieve the current entries localeName info ( LocaleName, SortID, Version, and count of indices ).

        LOCALENAMEINFO liFromMSysLocales;           //  i.e. new "tracking" table added in Ex14 Sp1

        Call( ErrCATIGetLocaleNameInfo( pkvpscursorMSysLocalesTable, &liFromMSysLocales ) );

        //  Retrieve the entry for this LocaleName + Version from the catalog accumulation.

        LOCALENAMEINFO liFromMSysObjects;           //  i.e. from base / traditional catalog (actual index definitions)
        Call( ErrOSStrCbCopyW( liFromMSysObjects.m_wszLocaleName, sizeof( liFromMSysObjects.m_wszLocaleName ), liFromMSysLocales.m_wszLocaleName ) );
        liFromMSysObjects.m_qwVersion = liFromMSysLocales.m_qwVersion;
        liFromMSysObjects.m_sortID = liFromMSysLocales.m_sortID;
        liFromMSysObjects.m_cIndices = INT_MAX;
        Assert( liFromMSysObjects.m_qwVersion == liFromMSysLocales.m_qwVersion );
        Assert( liFromMSysObjects.m_cIndices > 0 );

        const size_t iEntryFromDB = arrayLocalesInDB.SearchLinear( liFromMSysObjects, PfnCmpLocaleNameInfo );

        //
        //  Now compare the liFromMSysLocales(tracking table) with the liFromMSysObjects(indices in actual base catalog).
        //

        if ( iEntryFromDB == CLocaleInfoArray::iEntryNotFound )
        {
            if ( 0 == liFromMSysLocales.m_cIndices )
            {
                //  Matches!  No entry in DB for this and we have zero indices in MSysLocales.
            }
            else
            {
                //  we have an entry in MSysLocales, but no indices in the actual DB ...

                //  this is fine, because the MSysLocales table can be too high.

                OSTrace( JET_tracetagUpgrade, OSFormat( "Detected out of step (too high) MSysLocales entry, should be 0 but is actually {%ws - %I64x} = %d (fFixupMSysLocales=%d)\n",
                                liFromMSysLocales.m_wszLocaleName, liFromMSysLocales.m_qwVersion, liFromMSysLocales.m_cIndices, fFixupMSysLocales ) );

                if ( fFixupMSysLocales )
                {
                    //  whatever the value from MSysLocales, we should now zero it.

                    Call( pkvpscursorMSysLocalesTable->ErrKVPSCursorUpdValue( 0 ) );
                }
            }
        }
        else
        {
            liFromMSysObjects = arrayLocalesInDB.Entry( iEntryFromDB );
            if ( liFromMSysObjects.m_cIndices == liFromMSysLocales.m_cIndices )
            {
                //  This entry matches perfectly!
            }
            else
            {
                if ( liFromMSysObjects.m_cIndices < liFromMSysLocales.m_cIndices )
                {
                    //  we have more entries in MSysLocales than actually in the DB ...

                    //  this is fine, because the MSysLocales table can be too high.

                    OSTrace( JET_tracetagUpgrade, OSFormat( "Detected out of step (too high) MSysLocales entry, should be {%ws - %I64x} = %d but is actually %d (fFixupMSysLocales=%d)\n",
                                    liFromMSysObjects.m_wszLocaleName, liFromMSysObjects.m_qwVersion, liFromMSysObjects.m_cIndices, liFromMSysLocales.m_cIndices, fFixupMSysLocales ) );

                    if ( fFixupMSysLocales )
                    {
                        //  whatever the value from MSysLocales, we should now fix it up ...

                        Call( pkvpscursorMSysLocalesTable->ErrKVPSCursorUpdValue( liFromMSysObjects.m_cIndices ) );
                    }
                }
                else
                {
                    OSTrace( JET_tracetagUpgrade, OSFormat( "Detected corrupt (too low) MSysLocales entry, should be {%ws - %I64x} = %d (fFixupMSysLocales=%d)\n",
                                liFromMSysObjects.m_wszLocaleName, liFromMSysObjects.m_qwVersion, liFromMSysObjects.m_cIndices, fFixupMSysLocales ) );

                    AssertSzRTL( fFalse, "Underflow in count of indices (for localeName = %ws, ver = %I64x) ... catalog (%d) has more than MSysLocales (%d)!  This is a bad inconsistency.",
                                liFromMSysLocales.m_wszLocaleName, liFromMSysLocales.m_qwVersion, liFromMSysObjects.m_cIndices, liFromMSysLocales.m_cIndices );

                    if ( fFixupMSysLocales )
                    {
                        //  whatever the value from MSysLocales, we should now fix it up ...

                        Call( pkvpscursorMSysLocalesTable->ErrKVPSCursorUpdValue( liFromMSysObjects.m_cIndices ) );
                    }
                    else
                    {
                        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
                    }
                }
            }

            //  "remove" this entry from DB, so we do not consider it below ...
            liFromMSysObjects.m_wszLocaleName[0] = L'\0';
            CLocaleNameInfoArray::ERR errT = arrayLocalesInDB.ErrSetEntry( iEntryFromDB, liFromMSysObjects );
            Assert( errT == CLocaleNameInfoArray::ERR::errSuccess );
        }

        // move to next entry ...
    }

    if ( err == JET_errNoCurrentRecord )
    {
        //  this just means we exhausted the locale names table ...
        err = JET_errSuccess;
    }
    Call( err );

    //  Reset the cursor ... as we may want to insert new entries ...
    //
    pkvpscursorMSysLocalesTable->KVPSCursorEnd();
    delete pkvpscursorMSysLocalesTable;
    pkvpscursorMSysLocalesTable = NULL;

    //  Look for the consistency marker.
    //
    BOOL fFoundConsistencyMarker = fFalse;

    Call( ErrCATICheckForMSLocalesConsistencyMarker( g_rgfmp[ifmp].PkvpsMSysLocales(), &fFoundConsistencyMarker ) );

    //  We must have found the marker record at this point.
    //
    if ( !fFoundConsistencyMarker )
    {
        OSTrace( JET_tracetagUpgrade, "Detected corrupt MSysLocales table (missing consistency marker)\n" );
        AssertSzRTL( fFalse, "Whoa, MSysLocales is missing the consistency marker record! This is a bad inconsistency." );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    //  one more part, we must enumerate any locales left in DB that we didn't hit as part
    //  of walking the MSysLocales table ... anything here is a problem.

    for ( ULONG i = 0; i < arrayLocalesInDB.Size(); i++ )
    {
        LOCALENAMEINFO liFromMSysObjects;
        liFromMSysObjects = arrayLocalesInDB.Entry( i );

        if ( liFromMSysObjects.m_wszLocaleName[0] != L'\0' )
        {
            AssertRTL( liFromMSysObjects.m_cIndices );  // Why would ErrCATIAccumulateIndexLocales() give us a 0 count?

            OSTrace( JET_tracetagUpgrade, OSFormat( "Detected corrupt (missing) MSysLocales entry, should be {%ws - %I64x} = %d (fFixupMSysLocales=%d)\n",
                        liFromMSysObjects.m_wszLocaleName, liFromMSysObjects.m_qwVersion, liFromMSysObjects.m_cIndices, fFixupMSysLocales ) );
            AssertSzRTL( fFalse, "Whoa, catalog found unrepresented LocaleName (%ws,%I64x) that is not in MSysLocales!  This is a bad inconsistency.",
                        liFromMSysObjects.m_wszLocaleName, liFromMSysObjects.m_qwVersion );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );

            //  We can't fixup here because the next attach will then invisibly succeed.
        }
    }

    CallS( err );   // should have success by here ...

HandleError:

    if ( pkvpscursorMSysLocalesTable )
    {
        pkvpscursorMSysLocalesTable->KVPSCursorEnd();
        delete pkvpscursorMSysLocalesTable;
    }

    if ( ifmpOpen != ifmpNil )
    {
        CallS( ErrDBCloseDatabase( ppib, ifmpOpen, 0 ) );
    }

    return err;
}

ERR ErrCATDumpMSLocales( JET_SESID sesid, const IFMP ifmp )
{
    ERR err = JET_errSuccess;
    BOOL fKVPInit = fFalse;

    CKVPStore kvpsMSysLocales( ifmp, wszMSLocales );

    err = kvpsMSysLocales.ErrKVPInitStore( NULL, CKVPStore::eReadOnly, g_dwMSLocalesMajorVersions );

    if ( JET_errObjectNotFound == err )
    {
        // There isn't an MSLocales table. There is nothing to dump.
        return JET_errSuccess;
    }
    Call( err );

    fKVPInit = fTrue;

    kvpsMSysLocales.Dump( CPRINTFSTDOUT::PcprintfInstance(), fTrue );

HandleError:

    if ( fKVPInit )
    {
        kvpsMSysLocales.KVPTermStore();
    }

    return err;
}

ERR ErrCATIAddLocale(
    _In_ PIB * const ppib,
    _In_ const IFMP ifmp,
    _In_ PCWSTR wszLocaleName,
    _In_ SORTID sortID,
    _In_ const QWORD qwSortVersion )
{
    ERR err = JET_errSuccess;
    WCHAR wszLocaleSortIDSortVerKey[g_cchLocaleNameSortIDSortVersionLocaleKeyMax];
    WCHAR wszSortID[PERSISTED_SORTID_MAX_LENGTH] = L"";
    WszCATFormatSortID( sortID, wszSortID, _countof( wszSortID ) );

    //old KVP LCID based format was L"LCID=%d,Ver=%I64x". here we write the new format to KVP
    OSStrCbFormatW( wszLocaleSortIDSortVerKey, sizeof( wszLocaleSortIDSortVerKey ), g_wszLocaleNameSortIDSortVersionLocaleKey, wszLocaleName, wszSortID, qwSortVersion );

#ifdef DEBUG
{
    WCHAR wszLocaleNameT[NORM_LOCALE_NAME_MAX_LENGTH];
    QWORD qwSortVersionT;
    SORTID sortIDT;
    Assert( JET_errSuccess == ErrCATIParseLocaleNameInfo( wszLocaleSortIDSortVerKey, wszLocaleNameT, &qwSortVersionT, &sortIDT ) );
    Assert( 0 == _wcsicmp( wszLocaleName, wszLocaleNameT ) );
    Assert( qwSortVersion == qwSortVersionT );
    Assert( FSortIDEquals( &sortID, &sortIDT ) );
}
#endif

    if ( ppib != ppibNil && g_rgfmp[ifmp].FExclusiveBySession( ppib ) )
    {
        //  The database session has it open exclusively ... this means that we can
        //  use this session, as any rollback will be single threaded and convergent,
        //  but it also means we MUST use this session or we will disable the ability
        //  of a client to add an index while attached with JET_bitDbExclusive.
        err = g_rgfmp[ifmp].PkvpsMSysLocales()->ErrKVPAdjValue( ppib, wszLocaleSortIDSortVerKey, 1 );
    }
    else
    {
        err = g_rgfmp[ifmp].PkvpsMSysLocales()->ErrKVPAdjValue( wszLocaleSortIDSortVerKey, 1 );
    }

    return err;
}

