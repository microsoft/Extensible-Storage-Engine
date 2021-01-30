// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

typedef USHORT IDBFLAG;

const IDBFLAG   fidbUnique          = 0x0001;
const IDBFLAG   fidbAllowAllNulls   = 0x0002;
const IDBFLAG   fidbAllowFirstNull  = 0x0004;
const IDBFLAG   fidbAllowSomeNulls  = 0x0008;
const IDBFLAG   fidbNoNullSeg       = 0x0010;
const IDBFLAG   fidbPrimary         = 0x0020;
const IDBFLAG   fidbLocaleSet       = 0x0040;
const IDBFLAG   fidbMultivalued     = 0x0080;
const IDBFLAG   fidbTemplateIndex   = 0x0100;
const IDBFLAG   fidbDerivedIndex    = 0x0200;
const IDBFLAG   fidbLocalizedText   = 0x0400;
const IDBFLAG   fidbSortNullsHigh   = 0x0800;
const IDBFLAG   fidbUnicodeFixupOn_Deprecated   = 0x1000;
const IDBFLAG   fidbCrossProduct    = 0x2000;
const IDBFLAG   fidbDisallowTruncation = 0x4000;
const IDBFLAG   fidbNestedTable     = 0x8000;

const IDBFLAG   fidbVersioned               = 0x1000;
const IDBFLAG   fidbDeleted                 = 0x2000;
const IDBFLAG   fidbVersionedCreate         = 0x4000;
const IDBFLAG   fidbHasPlaceholderColumn    = 0x0100;
const IDBFLAG   fidbSparseIndex             = 0x0200;
const IDBFLAG   fidbSparseConditionalIndex  = 0x0400;
const IDBFLAG   fidbTuples                  = 0x0800;
const IDBFLAG   fidbBadSortVersion          = 0x0010;
const IDBFLAG   fidbOwnedByFcb              = 0x0020;
const IDBFLAG   fidbOutOfDateSortVersion    = 0x0040;

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

typedef USHORT  IDXFLAG;

const IDXFLAG   fIDXExtendedColumns = 0x0001;
const IDXFLAG   fIDXDotNetGuid      = 0x0002;

INLINE BOOL FIDXExtendedColumns( const IDXFLAG idxflag )        { return ( idxflag & fIDXExtendedColumns ); }
INLINE BOOL FIDXDotNetGUID( const IDXFLAG idxflag )     { return ( idxflag & fIDXDotNetGuid ); }

PERSISTED
struct LE_IDXFLAG
{
    UnalignedLittleEndian<IDBFLAG>  fidb;
    UnalignedLittleEndian<IDXFLAG>  fIDXFlags;
};




const INT       cIDBIdxSegMax                   = 6;

const INT       cIDBIdxSegConditionalMax        = 2;


const ULONG     cchIDXTuplesLengthMinAbsolute   = 2;
const ULONG     cchIDXTuplesLengthMaxAbsolute   = 255;
const ULONG     ichIDXTuplesToIndexMaxAbsolute  = 0x7fff;

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
    public:
        IDB( INST* const pinst )
            :   m_pinst( pinst )
        {
            Assert( (LONG_PTR)m_rgbitIdx % sizeof(LONG_PTR) == 0 );
            m_nlv.m_dwNormalizationFlags = 0;
            m_nlv.m_dwNlsVersion = 0;
            m_nlv.m_dwDefinedNlsVersion = 0;
            SetWszLocaleName( wszLocaleNameNone );
            ResetSortidCustomSortVersion();
        }
        ~IDB()      {}

#pragma push_macro( "new" )
#undef new
    private:
        void* operator new( size_t );
        void* operator new[]( size_t );
        void operator delete[]( void* );
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
        LONG        m_crefCurrentIndex;
        USHORT      m_crefVersionCheck;
        IDBFLAG     m_fidbPersisted;
        IDXFLAG     m_fidxPersisted;
        IDBFLAG     m_fidbNonPersisted;
        USHORT      m_itagIndexName;
        USHORT      m_cbVarSegMac;
        USHORT      m_cbKeyMost;
        BYTE        m_cidxseg;
        BYTE        m_cidxsegConditional;


    public:
            IDXSEG  rgidxseg[cIDBIdxSegMax];

            IDXSEG  rgidxsegConditional[cIDBIdxSegConditionalMax];

            NORM_LOCALE_VER m_nlv;
            USHORT      m_cchTuplesLengthMin;
            USHORT      m_cchTuplesLengthMax;

    public:
        union
        {
            INST*   m_pinst;
            BYTE    m_rgbAlign[8];
        };


    private:
        BYTE        m_rgbitIdx[32];


        USHORT      m_ichTuplesToIndexMax;
        USHORT      m_cchTuplesIncrement;
        USHORT      m_ichTuplesStart;


        USHORT      m_usReserved2[1];

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

            Assert( !FTemplateIndex() );

            Assert( CrefCurrentIndex() >= 0 );
            Assert( CrefCurrentIndex() < 0x7FFFFFFF );

            AtomicIncrement( &m_crefCurrentIndex );
        }
        INLINE VOID DecrementCurrentIndex()
        {
            Assert( !FPrimary() );

            Assert( !FTemplateIndex() );

            Assert( CrefCurrentIndex() > 0 );
            AtomicDecrement( &m_crefCurrentIndex );
        }

        INLINE USHORT CrefVersionCheck() const      { return m_crefVersionCheck; }
        INLINE VOID IncrementVersionCheck()
        {
            Assert( FVersioned() );
            Assert( !FTemplateIndex() );
            Assert( CrefVersionCheck() < 0xFFFF );

            AtomicIncrement( (LONG *)&m_crefVersionCheck );
        }
        INLINE VOID DecrementVersionCheck()
        {


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
        INLINE BOOL FColumnIndex( const FID fid ) const { return ( m_rgbitIdx[IbFromFid( fid )] & IbitFromFid( fid ) ); }
        INLINE VOID SetColumnIndex( const FID fid )     { m_rgbitIdx[IbFromFid( fid )] |= IbitFromFid( fid ); }

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

        INLINE BOOL FOutOfDateSortVersion() const             { return ( m_fidbNonPersisted & fidbOutOfDateSortVersion ); }
        INLINE VOID SetFOutOfDateSortVersion()                { m_fidbNonPersisted |= fidbOutOfDateSortVersion; }
        INLINE VOID ResetFOutOfDateSortVersion()              { m_fidbNonPersisted &= ~fidbOutOfDateSortVersion; }

        INLINE BOOL FPrimary() const                    { return ( m_fidbPersisted & fidbPrimary ); }
        INLINE VOID SetFPrimary()                       { m_fidbPersisted |= fidbPrimary; }

        INLINE BOOL FLocaleSet() const                  { return ( m_fidbPersisted & fidbLocaleSet ); }
        INLINE VOID SetFLocaleSet()                     { m_fidbPersisted |= fidbLocaleSet; }

        INLINE BOOL FMultivalued() const                { return ( m_fidbPersisted & fidbMultivalued ); }
        INLINE VOID SetFMultivalued()                   { m_fidbPersisted |= fidbMultivalued; }

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

        INLINE BOOL FCrossProduct() const           { return ( m_fidbPersisted & fidbCrossProduct ); }
        INLINE VOID SetFCrossProduct()              { m_fidbPersisted |= fidbCrossProduct; }
        INLINE VOID ResetFCrossProduct()                { m_fidbPersisted &= ~fidbCrossProduct; }

        INLINE BOOL FNestedTable() const            { return ( m_fidbPersisted & fidbNestedTable ); }
        INLINE VOID SetFNestedTable()               { m_fidbPersisted |= fidbNestedTable; }
        INLINE VOID ResetFNestedTable()             { m_fidbPersisted &= ~fidbNestedTable; }

        INLINE BOOL FDisallowTruncation() const         { return ( m_fidbPersisted & fidbDisallowTruncation ); }
        INLINE VOID SetFDisallowTruncation()                { m_fidbPersisted |= fidbDisallowTruncation; }
        INLINE VOID ResetFDisallowTruncation()              { m_fidbPersisted &= ~fidbDisallowTruncation; }

        INLINE BOOL FDotNetGuid() const         { return ( m_fidxPersisted & fIDXDotNetGuid ); }
        INLINE VOID SetFDotNetGuid()                { m_fidxPersisted |= fIDXDotNetGuid; }
        INLINE VOID ResetFDotNetGuid()              { m_fidxPersisted &= ~fIDXDotNetGuid; }

        INLINE BOOL FIDBOwnedByFCB() const          { return (m_fidbNonPersisted & fidbOwnedByFcb ); }
        INLINE VOID SetFIDBOwnedByFCB()         { m_fidbNonPersisted |= fidbOwnedByFcb; }
        INLINE VOID ResetFIDBOwnedByFCB()       { m_fidbNonPersisted &= ~fidbOwnedByFcb; }

        JET_GRBIT GrbitFromFlags() const;
        VOID SetFlagsFromGrbit( const JET_GRBIT grbit );



#ifdef DEBUGGER_EXTENSION
        VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif
};


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
