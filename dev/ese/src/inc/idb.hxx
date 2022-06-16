// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// Flags for IDB
typedef USHORT IDBFLAG;

const IDBFLAG   fidbUnique          = 0x0001;   // Duplicate keys not allowed
const IDBFLAG   fidbAllowAllNulls   = 0x0002;   // Make entries for NULL keys (all segments are null)
const IDBFLAG   fidbAllowFirstNull  = 0x0004;   // First index column NULL allowed in index
const IDBFLAG   fidbAllowSomeNulls  = 0x0008;   // Make entries for keys with some null segments
const IDBFLAG   fidbNoNullSeg       = 0x0010;   // Don't allow a NULL key segment
const IDBFLAG   fidbPrimary         = 0x0020;   // Index is the primary index
const IDBFLAG   fidbLocaleSet       = 0x0040;   // Index locale information (locale name) is set (JET_bitIndexUnicode was specified).
const IDBFLAG   fidbMultivalued     = 0x0080;   // Has a multivalued segment
const IDBFLAG   fidbTemplateIndex   = 0x0100;   // Index of a template table
const IDBFLAG   fidbDerivedIndex    = 0x0200;   // Index derived from template table
                                                //   Note that this flag is persisted, but
                                                //   never used in an in-memory IDB, because
                                                //   we use the template index IDB instead.
const IDBFLAG   fidbLocalizedText   = 0x0400;   // Has a unicode text column? (code page is 1200)
const IDBFLAG   fidbSortNullsHigh   = 0x0800;   // NULL sorts after data
// Jan 2012: MSU is being removed. fidbUnicodeFixupOn should no longer be referenced.
const IDBFLAG   fidbUnicodeFixupOn_Deprecated   = 0x1000;   // Track entries with undefined Unicode codepoints
const IDBFLAG   fidbCrossProduct    = 0x2000;   // all combinations of multi-valued columns are indexed
const IDBFLAG   fidbDisallowTruncation = 0x4000; // fail update rather than allow key truncation
const IDBFLAG   fidbNestedTable     = 0x8000;   // combinations of multi-valued columns of same itagSequence are indexed

// The following flags are not persisted.
// UNDONE: Eliminate the VersionedCreate flag -- it would increase complexity in the
// version store for the following scenarios, but it would be more uniform with NODE:
//      1) Rollback of DeleteIndex - does the Version bit get reset as well?
//      2) Cleanup of CreateIndex - don't reset Version bit if Delete bit is set
//      3) As soon as primary index is committed, updates will no longer conflict.
const IDBFLAG   fidbVersioned               = 0x1000;   // Create/DeleteIndex not yet committed
const IDBFLAG   fidbDeleted                 = 0x2000;   // index has been deleted
const IDBFLAG   fidbVersionedCreate         = 0x4000;   // CreateIndex not yet committed
const IDBFLAG   fidbHasPlaceholderColumn    = 0x0100;   // First column of index is a dummy column
const IDBFLAG   fidbSparseIndex             = 0x0200;   // optimisation: detect unnecessary index update
const IDBFLAG   fidbSparseConditionalIndex  = 0x0400;
const IDBFLAG   fidbTuples                  = 0x0800;   // indexed over substring tuples
const IDBFLAG   fidbBadSortVersion          = 0x0010;   // index was built with bad sort version
const IDBFLAG   fidbOwnedByFcb              = 0x0020;   // index was owned by FCB
const IDBFLAG   fidbOutOfDateSortVersion    = 0x0040;   // index was built with out-of-date sort version
const IDBFLAG   fidbDeferredPopulateCompleted = 0x0080; // index was deferred populate, but was completed.

INLINE BOOL FIDBUnique( const IDBFLAG idbflag )                 { return ( idbflag & fidbUnique ); }
INLINE BOOL FIDBAllowAllNulls( const IDBFLAG idbflag )          { return ( idbflag & fidbAllowAllNulls ); }
INLINE BOOL FIDBAllowFirstNull( const IDBFLAG idbflag )         { return ( idbflag & fidbAllowFirstNull ); }
INLINE BOOL FIDBAllowSomeNulls( const IDBFLAG idbflag )         { return ( idbflag & fidbAllowSomeNulls ); }
INLINE BOOL FIDBNoNullSeg( const IDBFLAG idbflag )              { return ( idbflag & fidbNoNullSeg ); }
INLINE BOOL FIDBPrimary( const IDBFLAG idbflag )                { return ( idbflag & fidbPrimary ); }
INLINE BOOL FIDBLocaleSet( const IDBFLAG idbflag )              { return ( idbflag & fidbLocaleSet ); }
INLINE BOOL FIDBMultivalued( const IDBFLAG idbflag )            { return ( idbflag & fidbMultivalued ); }
INLINE BOOL FIDBTemplateIndex( const IDBFLAG idbflag )          { return ( idbflag & fidbTemplateIndex ); }
INLINE BOOL FIDBDerivedIndex( const IDBFLAG idbflag )           { return ( idbflag & fidbDerivedIndex ); }
INLINE BOOL FIDBLocalizedText( const IDBFLAG idbflag )          { return ( idbflag & fidbLocalizedText ); }
INLINE BOOL FIDBSortNullsHigh( const IDBFLAG idbflag )          { return ( idbflag & fidbSortNullsHigh ); }
INLINE BOOL FIDBTuples( const IDBFLAG idbflag )                 { return ( idbflag & fidbTuples ); }
INLINE BOOL FIDBBadSortVersion( const IDBFLAG idbflag )         { return ( idbflag & fidbBadSortVersion ); }
INLINE BOOL FIDBOutOfDateSortVersion( const IDBFLAG idbflag )   { return ( idbflag & fidbOutOfDateSortVersion ); }
INLINE BOOL FIDBUnicodeFixupOn_Deprecated( const IDBFLAG idbflag )  { return ( idbflag & fidbUnicodeFixupOn_Deprecated ); }
INLINE BOOL FIDBCrossProduct( const IDBFLAG idbflag )           { return ( idbflag & fidbCrossProduct ); }
INLINE BOOL FIDBDisallowKeyTruncation( const IDBFLAG idbflag )  { return ( idbflag & fidbDisallowTruncation ); }

//  extended index flags
typedef USHORT  IDXFLAG;

//  fIDXExtendedColumns is special.  It is set on all catalogs, but not in IDBs, and is consumed only in cat.cxx
//  It may not be set in the IDB for an index that has this value set in the indexes catalog entry.
//
const IDXFLAG   fIDXExtendedColumns  = 0x0001;   // IDXSEGs are comprised of JET_COLUMNIDs, not FIDs
const IDXFLAG   fIDXDotNetGuid       = 0x0002;   // GUIDs sort according to .Net rules
const IDXFLAG   fIDXDeferredPopulate = 0x0004;   // Index is not completely populated and so is not normally usable.

INLINE BOOL FIDXExtendedColumns( const IDXFLAG idxflag )        { return ( idxflag & fIDXExtendedColumns ); }
INLINE BOOL FIDXDotNetGUID( const IDXFLAG idxflag )     { return ( idxflag & fIDXDotNetGuid ); }

// persisted index flags
PERSISTED
struct LE_IDXFLAG
{
    UnalignedLittleEndian<IDBFLAG>  fidb;
    UnalignedLittleEndian<IDXFLAG>  fIDXFlags;
};


/*  Index Descriptor Block: information about index key
/**/

// If there are at most this many columns in the index, then the columns will
// be listed in the IDB.  Otherwise, the array will be stored in the byte pool
// of the table's TDB.  Note that the value chosen satisfies 32-byte cache line
// alignment of the structure.
const INT       cIDBIdxSegMax                   = 6;

// If there are at most this many conditional columns in the index, then the columns will
// be listed in the IDB.  Otherwise, the array will be stored in the byte pool
// of the table's TDB.  Note that the value chosen satisfies 32-byte cache line
// alignment of the structure.
const INT       cIDBIdxSegConditionalMax        = 2;


//  for tuple indexes: absolute min/max length of substring tuples
//  and absolute max characters to index per string
const ULONG     cchIDXTuplesLengthMinAbsolute   = 2;
const ULONG     cchIDXTuplesLengthMaxAbsolute   = 255;
const ULONG     ichIDXTuplesToIndexMaxAbsolute  = 0x7fff;

//  for tuple indexes, default min/max length of substring tuples
//  and default max characters to index per string
const ULONG     cchIDXTuplesLengthMinDefault        = 3;
const ULONG     cchIDXTuplesLengthMaxDefault        = 10;
const ULONG     cchIDXTuplesToIndexMaxDefault   = 0x7fff;
const ULONG     cchIDXTupleIncrementDefault     = 1;
const ULONG     ichIDXTupleStartDefault         = 0;

PERSISTED
struct LE_TUPLELIMITS_V1
{
    UnalignedLittleEndian<ULONG>    le_chLengthMin;
    UnalignedLittleEndian<ULONG>    le_chLengthMax;
    UnalignedLittleEndian<ULONG>    le_chToIndexMax;
};

PERSISTED
struct LE_TUPLELIMITS
{
    UnalignedLittleEndian<ULONG>    le_chLengthMin;
    UnalignedLittleEndian<ULONG>    le_chLengthMax;
    UnalignedLittleEndian<ULONG>    le_chToIndexMax;
    UnalignedLittleEndian<ULONG>    le_cchIncrement;
    UnalignedLittleEndian<ULONG>    le_ichStart;
};


class IDB
{
#ifdef AMD64
    private:
        static VOID VerifyOptimalPacking();    // Just a bunch of static_asserts to verify optimality.     
#endif

    public:
        IDB( INST* const pinst )
            :   m_pinst( pinst )
        {
            //  ensure bit array is aligned for LONG_PTR traversal
            Assert( (LONG_PTR)m_rgbitIdx % sizeof(LONG_PTR) == 0 );
            m_nlv.m_dwNormalizationFlags = 0;
            m_nlv.m_dwNlsVersion = 0;
            m_nlv.m_dwDefinedNlsVersion = 0;
            SetWszLocaleName( wszLocaleNameNone );
            ResetSortidCustomSortVersion();

            m_trxDeferredPopulateMinTrx = trxMin;
        }
        ~IDB()      {}

#pragma push_macro( "new" )
#undef new
    private:
        void* operator new( size_t );           //  meaningless without INST*
        void* operator new[]( size_t );         //  meaningless without INST*
        void operator delete[]( void* );        //  not supported
    public:
        void* operator new( size_t cbAlloc, INST* pinst )
        {
            return pinst->m_cresIDB.PvRESAlloc_( SzNewFile(), UlNewLine() );
        }
        void operator delete( void* pv )
        {
            if ( pv )
            {
                IDB* pidb = reinterpret_cast< IDB* >( pv );
                pidb->m_pinst->m_cresIDB.Free( pv );
            }
        }
#pragma pop_macro( "new" )

    private:
        LONG        m_crefCurrentIndex;         //  for secondary indexes only, number of cursors with SetCurrentIndex on this IDB
        USHORT      m_crefVersionCheck;         //  number of cursors consulting catalog for versioned index info
                                                //  NOTE: m_crefVersionCheck must be on an atomically-modifiable alignment..
        IDBFLAG     m_fidbPersisted;            //  persisted index flags
        IDXFLAG     m_fidxPersisted;            //  additional persisted flags
        IDBFLAG     m_fidbNonPersisted;         //  non-persisted index flags
        USHORT      m_itagIndexName;            //  itag into ptdb->rgb where index name is stored
        USHORT      m_cbVarSegMac;              //  maximum variable segment size
        USHORT      m_cbKeyMost;                //  maximum key size
        BYTE        m_cidxseg;                  //  number of columns in index (<=12)
        BYTE        m_cidxsegConditional;       //  number of conditional columns (<=12)
        TRX         m_trxDeferredPopulateMinTrx;  // Minimum trx for deferred populate processing to be allowed.
        USHORT      m_ichTuplesToIndexMax;
        USHORT      m_cchTuplesIncrement;
        USHORT      m_ichTuplesStart;
        BYTE        m_rgbPadding1[2];

    public:
//      union
//          {
            IDXSEG  rgidxseg[cIDBIdxSegMax];    //  array of columnid for index
//          USHORT  itagrgidxseg;               //  if m_cidxseg > cIDBIdxSegMax, then rgidxseg
//                                              //      will be stored in byte pool at this itag
//          };

//      union
//          {
            IDXSEG  rgidxsegConditional[cIDBIdxSegConditionalMax];
//          USHORT  itagrgidxsegConditional;    //  if m_cidxsegConditional > cIDBIdxSegConditionalMax,
//          };                                  //      then rgidxseg will be stored in byte pool at this itag

            NORM_LOCALE_VER m_nlv;

            USHORT      m_cchTuplesLengthMin;
            USHORT      m_cchTuplesLengthMax;

            BYTE        m_rgbPadding2[4];

        union
        {
            INST*     m_pinst;                  //  INST
            ULONGLONG m_ullAlign;               //  forces alignment on all platforms
        };

    private:
        BYTE        m_rgbitIdx[32];             //  bit array for index columns

        BYTE        rgbPadding3[8];

    public:
        INLINE VOID InitRefcounts()
        {
            m_crefCurrentIndex = 0;
            m_crefVersionCheck = 0;
        }

        INLINE LONG CrefCurrentIndex() const
        {
            Assert( m_crefCurrentIndex >= 0 );
            return m_crefCurrentIndex;
        }
        INLINE VOID IncrementCurrentIndex()
        {
            Assert( !FPrimary() );
            Assert( !FDeleted() );

            // Don't need refcount on template indexes, since we
            // know they'll never go away.
            Assert( !FTemplateIndex() );

            Assert( CrefCurrentIndex() >= 0 );
            Assert( CrefCurrentIndex() < 0x7FFFFFFF );

            AtomicIncrement( &m_crefCurrentIndex );
        }
        INLINE VOID DecrementCurrentIndex()
        {
            Assert( !FPrimary() );

            // Don't need refcount on template indexes, since we
            // know they'll never go away.
            Assert( !FTemplateIndex() );

            Assert( CrefCurrentIndex() > 0 );
            AtomicDecrement( &m_crefCurrentIndex );
        }

        INLINE USHORT CrefVersionCheck() const      { return m_crefVersionCheck; }
        INLINE VOID IncrementVersionCheck()
        {
            // WARNING: Ensure table's critical section is held during this call.
            Assert( FVersioned() );
            Assert( !FTemplateIndex() );
            Assert( CrefVersionCheck() < 0xFFFF );

            AtomicIncrement( (LONG *)&m_crefVersionCheck );
        }
        INLINE VOID DecrementVersionCheck()
        {
            // WARNING: Ensure table's critical section is held during this call.

            // At this point, the Versioned flag is not necessarily set,
            // because it may have been reset while we were performing the
            // actual version check.

            Assert( !FTemplateIndex() );
            Assert( CrefVersionCheck() > 0 );

            AtomicDecrement( (LONG *)&m_crefVersionCheck );
        }

        INLINE VOID SetWszLocaleName( const WCHAR * const wszLocaleName )
        {
            C_ASSERT( sizeof( m_nlv.m_wszLocaleName ) == NORM_LOCALE_NAME_MAX_LENGTH * sizeof( m_nlv.m_wszLocaleName[0] ) );
            OSStrCbCopyW( m_nlv.m_wszLocaleName, sizeof( m_nlv.m_wszLocaleName ), wszLocaleName );
        }
        INLINE PCWSTR const WszLocaleName() const
        {
            return m_nlv.m_wszLocaleName;
        }
        INLINE QWORD QwSortVersion() const
        {
            return QwSortVersionFromNLSDefined( m_nlv.m_dwNlsVersion, m_nlv.m_dwDefinedNlsVersion );
        }
        INLINE VOID SetQwSortVersion( _In_ const QWORD qwSortVersion )
        {
            DWORD dwNlsVersion;
            DWORD dwDefinedNlsVersion;
            QwSortVersionToNLSDefined( qwSortVersion, &dwNlsVersion, &dwDefinedNlsVersion );

            m_nlv.m_dwNlsVersion = dwNlsVersion;
            m_nlv.m_dwDefinedNlsVersion = dwDefinedNlsVersion;
        }

        INLINE DWORD DwLCMapFlags() const               { return m_nlv.m_dwNormalizationFlags; }
        INLINE VOID SetDwLCMapFlags( const DWORD dw )
        {
            m_nlv.m_dwNormalizationFlags = dw;
        }
        INLINE USHORT ItagIndexName() const                 { return m_itagIndexName; }
        INLINE VOID SetItagIndexName( const USHORT itag )   { m_itagIndexName = itag; }

        INLINE USHORT CbVarSegMac() const                   { return m_cbVarSegMac; }
        INLINE VOID SetCbVarSegMac( const USHORT cb )       { m_cbVarSegMac = cb; }

        INLINE TRX TrxDeferredPopulateMinTrx() const
        {
            return m_trxDeferredPopulateMinTrx;
        }

        INLINE VOID SetTrxDeferredPopulateMinTrx( const TRX trxDeferredPopulateMinTrx )
        {
            m_trxDeferredPopulateMinTrx = trxDeferredPopulateMinTrx;
        }
        INLINE USHORT CbKeyMost() const
        {
            Assert( m_cbKeyMost > 0 );
            Assert( m_cbKeyMost <= CbKeyMostForPage() );
            return m_cbKeyMost;
        }

        INLINE VOID SetCbKeyMost( const USHORT cbKeyMost )
        {
            Assert( cbKeyMost > 0 );
            Assert( cbKeyMost <= CbKeyMostForPage() );
            m_cbKeyMost = cbKeyMost;
        }
        INLINE __out_range(0, JET_ccolKeyMost) BYTE Cidxseg() const
        {
            Assert(m_cidxseg <= JET_ccolKeyMost);
            return m_cidxseg;
        }
        INLINE VOID SetCidxseg( __in_range(0, JET_ccolKeyMost) const BYTE cidxseg )
        {
            Assert(cidxseg <= JET_ccolKeyMost);
            m_cidxseg = cidxseg;
        }

        INLINE __out_range(0, JET_ccolKeyMost) BYTE CidxsegConditional() const
        {
            Assert(m_cidxsegConditional <= JET_ccolKeyMost);
            return m_cidxsegConditional;
        }
        INLINE VOID SetCidxsegConditional( __in_range(0, JET_ccolKeyMost) const BYTE cidxseg )
        {
            Assert(cidxseg <= JET_ccolKeyMost);
            m_cidxsegConditional = cidxseg;
        }

        INLINE BOOL FIsRgidxsegInMempool() const                { return ( m_cidxseg > cIDBIdxSegMax ); }
        INLINE BOOL FIsRgidxsegConditionalInMempool() const     { return ( m_cidxsegConditional > cIDBIdxSegConditionalMax ); }

        INLINE USHORT ItagRgidxseg() const
        {
            Assert( FIsRgidxsegInMempool() );
            return rgidxseg[0].Fid();
        }
        INLINE VOID SetItagRgidxseg( const USHORT itag )
        {
            Assert( FIsRgidxsegInMempool() );
            rgidxseg[0].SetFid( itag );
        }

        INLINE USHORT ItagRgidxsegConditional() const
        {
            Assert( FIsRgidxsegConditionalInMempool() );
            return rgidxsegConditional[0].Fid();
        }
        INLINE VOID SetItagRgidxsegConditional( const USHORT itag )
        {
            Assert( FIsRgidxsegConditionalInMempool() );
            rgidxsegConditional[0].SetFid( itag );
        }

        INLINE USHORT CchTuplesLengthMin() const
        {
            Assert( FTuples() );

            Assert( m_cchTuplesLengthMin >= cchIDXTuplesLengthMinAbsolute );
            Assert( m_cchTuplesLengthMin <= cchIDXTuplesLengthMaxAbsolute );
            return m_cchTuplesLengthMin;
        }
        INLINE VOID SetCchTuplesLengthMin( const USHORT ch )
        {
            Assert( FTuples() );
            Assert( ch >= cchIDXTuplesLengthMinAbsolute );
            Assert( ch <= cchIDXTuplesLengthMaxAbsolute );
            m_cchTuplesLengthMin = ch;
        }

        INLINE USHORT CchTuplesLengthMax() const
        {
            Assert( FTuples() );

            Assert( m_cchTuplesLengthMax >= cchIDXTuplesLengthMinAbsolute );
            Assert( m_cchTuplesLengthMax <= cchIDXTuplesLengthMaxAbsolute );
            return m_cchTuplesLengthMax;
        }
        INLINE VOID SetCchTuplesLengthMax( const USHORT ch )
        {
            Assert( FTuples() );

            Assert( ch >= cchIDXTuplesLengthMinAbsolute );
            Assert( ch <= cchIDXTuplesLengthMaxAbsolute );
            m_cchTuplesLengthMax = ch;
        }

        INLINE USHORT IchTuplesToIndexMax() const
        {
            Assert( FTuples() );

            Assert( m_ichTuplesToIndexMax <= ichIDXTuplesToIndexMaxAbsolute );
            return m_ichTuplesToIndexMax;
        }
        INLINE VOID SetIchTuplesToIndexMax( const USHORT ich )
        {
            Assert( FTuples() );

            Assert( ich <= ichIDXTuplesToIndexMaxAbsolute );
            m_ichTuplesToIndexMax = ich;
        }
        INLINE USHORT CchTuplesIncrement() const
        {
            Assert( FTuples() );

            return m_cchTuplesIncrement;
        }
        INLINE VOID SetCchTuplesIncrement( const USHORT cch )
        {
            Assert( FTuples() );

            m_cchTuplesIncrement = cch;
        }

        INLINE USHORT IchTuplesStart() const
        {
            Assert( FTuples() );

            return m_ichTuplesStart;
        }
        INLINE VOID SetIchTuplesStart( const USHORT ich )
        {
            Assert( FTuples() );

            m_ichTuplesStart = ich;
        }

        INLINE const SORTID* PsortidCustomSortVersion() const
        {
            return &m_nlv.m_sortidCustomSortVersion;
        }

        // Accepts a zero'ed-out SORTID.
        INLINE VOID SetSortidCustomSortVersion( _In_ const SORTID* const psortidCustomSortVersion )
        {
            m_nlv.m_sortidCustomSortVersion = *psortidCustomSortVersion;
        }

        INLINE VOID ResetSortidCustomSortVersion()
        {
            memset( &m_nlv.m_sortidCustomSortVersion, 0, sizeof( m_nlv.m_sortidCustomSortVersion ) );;
        }

        INLINE DWORD DwNlsVersion() const
        {
            return m_nlv.m_dwNlsVersion;
        }

        INLINE DWORD DwDefinedNlsVersion() const
        {
            return m_nlv.m_dwDefinedNlsVersion;
        }

        INLINE const NORM_LOCALE_VER* const PnlvGetNormalizationStruct() const
        {
            return &m_nlv;
        }

        INLINE BYTE *RgbitIdx() const                   { return const_cast<IDB *>( this )->m_rgbitIdx; }
        INLINE BOOL FColumnIndex( const FID fid ) const { return ( m_rgbitIdx[fid.IbHash()] & fid.IbitHash() ); }
        INLINE VOID SetColumnIndex( const FID fid )     { m_rgbitIdx[fid.IbHash()] |= fid.IbitHash(); }

        INLINE IDBFLAG FPersistedFlags() const          { return m_fidbPersisted; }
        INLINE VOID SetPersistedFlags( const IDBFLAG fidb ) { m_fidbPersisted = fidb; m_fidbNonPersisted = 0; }
        INLINE IDBFLAG FPersistedFlagsX() const         { return m_fidxPersisted; }
        INLINE VOID SetPersistedFlagsX( const IDXFLAG fidx )    { m_fidxPersisted = fidx; }
        INLINE VOID ResetFlags()                        { m_fidbPersisted = 0; m_fidxPersisted = 0; m_fidbNonPersisted = 0; }

        INLINE BOOL FUnique() const                     { return ( m_fidbPersisted & fidbUnique ); }
        INLINE VOID SetFUnique()                        { m_fidbPersisted |= fidbUnique; }

        INLINE BOOL FAllowAllNulls() const              { return ( m_fidbPersisted & fidbAllowAllNulls ); }
        INLINE VOID SetFAllowAllNulls()                 { m_fidbPersisted |= fidbAllowAllNulls; }
        INLINE VOID ResetFAllowAllNulls()               { m_fidbPersisted &= ~fidbAllowAllNulls; }

        INLINE BOOL FAllowFirstNull() const             { return ( m_fidbPersisted & fidbAllowFirstNull ); }
        INLINE VOID SetFAllowFirstNull()                { m_fidbPersisted |= fidbAllowFirstNull; }

        INLINE BOOL FAllowSomeNulls() const             { return ( m_fidbPersisted & fidbAllowSomeNulls ); }
        INLINE VOID SetFAllowSomeNulls()                { m_fidbPersisted |= fidbAllowSomeNulls; }

        INLINE BOOL FNoNullSeg() const                  { return ( m_fidbPersisted & fidbNoNullSeg ); }
        INLINE VOID SetFNoNullSeg()                     { m_fidbPersisted |= fidbNoNullSeg; }

        INLINE BOOL FSortNullsHigh() const              { return ( m_fidbPersisted & fidbSortNullsHigh ); }
        INLINE VOID SetFSortNullsHigh()                 { m_fidbPersisted |= fidbSortNullsHigh; }

        INLINE BOOL FSparseIndex() const                { return ( m_fidbNonPersisted & fidbSparseIndex ); }
        INLINE VOID SetFSparseIndex()                   { m_fidbNonPersisted |= fidbSparseIndex; }
        INLINE VOID ResetFSparseIndex()                 { m_fidbNonPersisted &= ~fidbSparseIndex; }

        INLINE BOOL FSparseConditionalIndex() const     { return ( m_fidbNonPersisted & fidbSparseConditionalIndex ); }
        INLINE VOID SetFSparseConditionalIndex()        { m_fidbNonPersisted |= fidbSparseConditionalIndex; }
        INLINE VOID ResetFSparseConditionalIndex()      { m_fidbNonPersisted &= ~fidbSparseConditionalIndex; }

        INLINE BOOL FTuples() const                     { return ( m_fidbNonPersisted & fidbTuples ); }
        INLINE VOID SetFTuples()                        { m_fidbNonPersisted |= fidbTuples; }
        INLINE VOID ResetFTuples()                      { m_fidbNonPersisted &= ~fidbTuples; }

        INLINE BOOL FBadSortVersion() const             { return ( m_fidbNonPersisted & fidbBadSortVersion ); }
        INLINE VOID SetFBadSortVersion()                { m_fidbNonPersisted |= fidbBadSortVersion; }
        INLINE VOID ResetFBadSortVersion()              { m_fidbNonPersisted &= ~fidbBadSortVersion; }

        INLINE BOOL FOutOfDateSortVersion() const       { return ( m_fidbNonPersisted & fidbOutOfDateSortVersion ); }
        INLINE VOID SetFOutOfDateSortVersion()          { m_fidbNonPersisted |= fidbOutOfDateSortVersion; }
        INLINE VOID ResetFOutOfDateSortVersion()        { m_fidbNonPersisted &= ~fidbOutOfDateSortVersion; }

        INLINE BOOL FPrimary() const                    { return ( m_fidbPersisted & fidbPrimary ); }
        INLINE VOID SetFPrimary()                       { m_fidbPersisted |= fidbPrimary; }

        INLINE BOOL FDeferredPopulate() const           { return ( m_fidxPersisted & fIDXDeferredPopulate ); }
        INLINE VOID SetFDeferredPopulate()              { m_fidxPersisted |= fIDXDeferredPopulate; }
        INLINE VOID ResetFDeferredPopulate()            { m_fidxPersisted &= ~fIDXDeferredPopulate; }

        // Whether the user specified a locale when creating the index. The default locale may
        // still be used and persisted, but FLocaleSet() will be false in that case.
        // Also see FLocalizedText().
        INLINE BOOL FLocaleSet() const                  { return ( m_fidbPersisted & fidbLocaleSet ); }
        INLINE VOID SetFLocaleSet()                     { m_fidbPersisted |= fidbLocaleSet; }

        INLINE BOOL FMultivalued() const                { return ( m_fidbPersisted & fidbMultivalued ); }
        INLINE VOID SetFMultivalued()                   { m_fidbPersisted |= fidbMultivalued; }

        // Whether *any* column in the index is Text, and set to Unicode (codepage == 1200).
        // Also see FLocaleSet().
        INLINE BOOL FLocalizedText() const              { return ( m_fidbPersisted & fidbLocalizedText ); }
        INLINE VOID SetFLocalizedText()                 { m_fidbPersisted |= fidbLocalizedText; }

        INLINE BOOL FTemplateIndex() const              { return ( m_fidbPersisted & fidbTemplateIndex ); }
        INLINE VOID SetFTemplateIndex()                 { m_fidbPersisted |= fidbTemplateIndex; }
        INLINE VOID ResetFTemplateIndex()               { m_fidbPersisted &= ~fidbTemplateIndex; }

        INLINE BOOL FDerivedIndex() const               { return ( m_fidbPersisted & fidbDerivedIndex ); }
        INLINE VOID SetFDerivedIndex()                  { m_fidbPersisted |= fidbDerivedIndex; }

        INLINE BOOL FHasPlaceholderColumn() const       { return ( m_fidbNonPersisted & fidbHasPlaceholderColumn ); }
        INLINE VOID SetFHasPlaceholderColumn()          { m_fidbNonPersisted |= fidbHasPlaceholderColumn; }

        INLINE BOOL FVersioned() const                  { return ( m_fidbNonPersisted & fidbVersioned ); }
        INLINE VOID SetFVersioned()                     { m_fidbNonPersisted |= fidbVersioned; }
        INLINE VOID ResetFVersioned()                   { m_fidbNonPersisted &= ~fidbVersioned; }

        INLINE BOOL FDeleted() const                    { return ( m_fidbNonPersisted & fidbDeleted ); }
        INLINE VOID SetFDeleted()                       { m_fidbNonPersisted |= fidbDeleted; }
        INLINE VOID ResetFDeleted()                     { m_fidbNonPersisted &= ~fidbDeleted; }

        INLINE BOOL FVersionedCreate() const            { return ( m_fidbNonPersisted & fidbVersionedCreate ); }
        INLINE VOID SetFVersionedCreate()               { m_fidbNonPersisted |= fidbVersionedCreate; }
        INLINE VOID ResetFVersionedCreate()             { m_fidbNonPersisted &= ~fidbVersionedCreate; }

        INLINE BOOL FCrossProduct() const               { return ( m_fidbPersisted & fidbCrossProduct ); }
        INLINE VOID SetFCrossProduct()                  { m_fidbPersisted |= fidbCrossProduct; }
        INLINE VOID ResetFCrossProduct()                { m_fidbPersisted &= ~fidbCrossProduct; }

        INLINE BOOL FNestedTable() const                { return ( m_fidbPersisted & fidbNestedTable ); }
        INLINE VOID SetFNestedTable()                   { m_fidbPersisted |= fidbNestedTable; }
        INLINE VOID ResetFNestedTable()                 { m_fidbPersisted &= ~fidbNestedTable; }

        INLINE BOOL FDisallowTruncation() const         { return ( m_fidbPersisted & fidbDisallowTruncation ); }
        INLINE VOID SetFDisallowTruncation()            { m_fidbPersisted |= fidbDisallowTruncation; }
        INLINE VOID ResetFDisallowTruncation()          { m_fidbPersisted &= ~fidbDisallowTruncation; }

        INLINE BOOL FDotNetGuid() const                 { return ( m_fidxPersisted & fIDXDotNetGuid ); }
        INLINE VOID SetFDotNetGuid()                    { m_fidxPersisted |= fIDXDotNetGuid; }
        INLINE VOID ResetFDotNetGuid()                  { m_fidxPersisted &= ~fIDXDotNetGuid; }

        INLINE BOOL FIDBOwnedByFCB() const              { return (m_fidbNonPersisted & fidbOwnedByFcb ); }
        INLINE VOID SetFIDBOwnedByFCB()                 { m_fidbNonPersisted |= fidbOwnedByFcb; }
        INLINE VOID ResetFIDBOwnedByFCB()               { m_fidbNonPersisted &= ~fidbOwnedByFcb; }

        INLINE BOOL FDeferredPopulateCompleted() const  { return (m_fidbNonPersisted & fidbDeferredPopulateCompleted ); }
        INLINE VOID SetFDeferredPopulateCompleted()     { m_fidbNonPersisted |= fidbDeferredPopulateCompleted; }
        INLINE VOID RestFDeferredPopulateCompleted()    { m_fidbNonPersisted &= ~fidbDeferredPopulateCompleted; }
    
        JET_GRBIT GrbitFromFlags() const;
        VOID SetFlagsFromGrbit( const JET_GRBIT grbit );



#ifdef DEBUGGER_EXTENSION
        VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif  //  DEBUGGER_EXTENSION
};


#ifdef AMD64

INLINE VOID IDB::VerifyOptimalPacking()
{
    // Verify optimal packing.
    //
    // No one ever calls this routine.  It's enough that it compiles cleanly for the static_asserts.
    //
    // Note: use a field to take up padding inside the structure.  See rgbPadding* arrays.
    //
    //

#define sizeofField( TYPE, FIELD ) ( sizeof( (( TYPE * ) 0 )->FIELD) )

#define NoWastedSpaceAround(TYPE, FIELDFIRST, FIELDLAST)               \
    (                                                                   \
        ( OffsetOf( TYPE, FIELDFIRST ) == 0 ) &&                        \
        ( OffsetOf( TYPE, FIELDLAST ) + sizeofField( TYPE, FIELDLAST )  == sizeof( TYPE ) ) && \
        ( ( sizeof( TYPE ) % 32) == 0 )                                 \
        ),                                                              \
        "Unexpected padding around " #TYPE

#define NoWastedSpace(TYPE, FIELD1, FIELD2)                             \
    ( OffsetOf( TYPE, FIELD1) + sizeofField( TYPE, FIELD1 ) == OffsetOf( TYPE, FIELD2 ) ), \
        "Unexpected padding between " #FIELD1 " and " #FIELD2

#define CacheLineMark(TYPE, FIELD, NUM)                                 \
    ( OffsetOf( TYPE, FIELD ) == ( 64 * NUM ) ), "Cache line marker"


    static_assert( sizeof( IDB ) == 320, "Current size" );
    
    static_assert( NoWastedSpaceAround( IDB, m_crefCurrentIndex, rgbPadding3 ) );
    static_assert( CacheLineMark( IDB, m_crefCurrentIndex, 0 ) );
    static_assert( NoWastedSpace( IDB, m_crefCurrentIndex, m_crefVersionCheck ) ); 
    static_assert( NoWastedSpace( IDB, m_crefVersionCheck, m_fidbPersisted ) );
    static_assert( NoWastedSpace( IDB, m_fidbPersisted, m_fidxPersisted ) );
    static_assert( NoWastedSpace( IDB, m_fidxPersisted, m_fidbNonPersisted ) );
    static_assert( NoWastedSpace( IDB, m_fidbNonPersisted, m_itagIndexName ) );
    static_assert( NoWastedSpace( IDB, m_itagIndexName, m_cbVarSegMac ) );
    static_assert( NoWastedSpace( IDB, m_cbVarSegMac, m_cbKeyMost ) );
    static_assert( NoWastedSpace( IDB, m_cbKeyMost, m_cidxseg ) );
    static_assert( NoWastedSpace( IDB, m_cidxseg, m_cidxsegConditional ) );
    static_assert( NoWastedSpace( IDB, m_cidxsegConditional, m_trxDeferredPopulateMinTrx ) );
    static_assert( NoWastedSpace( IDB, m_trxDeferredPopulateMinTrx, m_ichTuplesToIndexMax ) );
    static_assert( NoWastedSpace( IDB, m_ichTuplesToIndexMax, m_cchTuplesIncrement ) );
    static_assert( NoWastedSpace( IDB, m_cchTuplesIncrement, m_ichTuplesStart ) );
    static_assert( NoWastedSpace( IDB, m_ichTuplesStart, m_rgbPadding1 ) );
    static_assert( NoWastedSpace( IDB, m_rgbPadding1, rgidxseg ) );  
    static_assert( NoWastedSpace( IDB, rgidxseg, rgidxsegConditional ) );
    static_assert( NoWastedSpace( IDB, rgidxsegConditional, m_nlv ) );
    
    static_assert( CacheLineMark( IDB, m_nlv, 1 ) ); // All the remaining cache lines hit in this member.
    static_assert( NoWastedSpace( IDB, m_nlv, m_cchTuplesLengthMin  ) ); 
    static_assert( NoWastedSpace( IDB, m_cchTuplesLengthMin, m_cchTuplesLengthMax ) );
    static_assert( NoWastedSpace( IDB, m_cchTuplesLengthMax, m_rgbPadding2 ) );
    static_assert( NoWastedSpace( IDB, m_rgbPadding2, m_pinst ) );
    static_assert( NoWastedSpace( IDB, m_pinst, m_rgbitIdx ) ); 
    static_assert( NoWastedSpace( IDB, m_rgbitIdx, rgbPadding3 ) ); 

    // The declarations of rgidxseg and rgidxsegConditional are arrays based on constants.
    // The constants have comments stating they are chosen to get 32 byte cache line alignment.
    // It is therefore required that these two stay next to each other, and that their total size is 32 bytes.
    // Note that in existing code, they are not and have not started at a 32 byte aligned offset.
    // Most recently, they started at offset 20.
    static_assert( NoWastedSpace( IDB, rgidxseg, rgidxsegConditional ) );
    static_assert( 32 == ( sizeofField( IDB, rgidxseg) + sizeofField( IDB, rgidxsegConditional) ), "Necessary size" );

}
#endif

//
//
//  0x130 bytes (AMD64 aligned)


ERR ErrIDBSetIdxSeg(
    IDB             * const pidb,
    TDB             * const ptdb,
    const IDXSEG    * const rgidxseg );
ERR ErrIDBSetIdxSegConditional(
    IDB             * const pidb,
    TDB             * const ptdb,
    const IDXSEG    * const rgidxseg );

ERR ErrIDBSetIdxSeg(
    IDB                         * const pidb,
    TDB                         * const ptdb,
    const BOOL                  fConditional,
    const LE_IDXSEG             * const rgidxseg );

VOID SetIdxSegFromOldFormat(
    const UnalignedLittleEndian< IDXSEG_OLD >   * const le_rgidxseg,
    IDXSEG                                      * const rgidxseg,
    const ULONG                                 cidxseg,
    const BOOL                                  fConditional,
    const BOOL                                  fTemplateTable,
    const TCIB                                  * const ptcibTemplateTable );
ERR ErrIDBSetIdxSegFromOldFormat(
    IDB                                         * const pidb,
    TDB                                         * const ptdb,
    const BOOL                                  fConditional,
    const UnalignedLittleEndian< IDXSEG_OLD >   * const le_rgidxseg );

const IDXSEG* PidxsegIDBGetIdxSeg( const IDB * const pidb, const TDB * const ptdb );
const IDXSEG* PidxsegIDBGetIdxSegConditional( const IDB * const pidb, const TDB * const ptdb );
