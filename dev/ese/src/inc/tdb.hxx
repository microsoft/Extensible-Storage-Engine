// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

class MEMPOOL
{
    public:
        MEMPOOL()                       { m_pbuf = NULL; }
        ~MEMPOOL()                      { return; }
        typedef USHORT ITAG;

    private:
        BYTE    *m_pbuf;
        ULONG   m_cbBufSize;
        ULONG   m_ibBufFree;
        ITAG    m_itagUnused;
        ITAG    m_itagFreed;

    private:
        struct MEMPOOLTAG
        {
            ULONG   ib;
            ULONG   cb;
        };

    private:
        static const ULONG cbChunkSize;
        static const USHORT ctagChunkSize;
        static const USHORT cMaxEntries;
        static const ITAG itagTagArray;
        static const ITAG itagFirstUsable;

#ifdef DEBUGGER_EXTENSION
    public:
#else
    private:
#endif

        BYTE *Pbuf() const              { return m_pbuf; }
        VOID SetPbuf( BYTE *pbuf )      { m_pbuf = pbuf; }

        ULONG CbBufSize() const         { return m_cbBufSize; }

    private:
        ULONG IbBufFree() const         { return m_ibBufFree; }
        ITAG ItagUnused() const         { return m_itagUnused; }
        ITAG ItagFreed() const          { return m_itagFreed; }

        VOID SetCbBufSize( ULONG cb )   { m_cbBufSize = cb; }
        VOID SetIbBufFree( ULONG ib )   { m_ibBufFree = ib; }
        VOID SetItagUnused( ITAG itag ) { m_itagUnused = itag; }
        VOID SetItagFreed( ITAG itag )  { m_itagFreed = itag; }

        BOOL FGrowBuf( ULONG cbNeeded );
        BOOL FResizeBuf( ULONG cbNewBufSize );
        ERR ErrGrowEntry( ITAG itag, ULONG cbNew );
        VOID ShrinkEntry( ITAG itag, ULONG cbNew );

    public:
        VOID AssertValid() const;
        VOID AssertValidTag( ITAG iTagEntry ) const;
        BYTE *PbGetEntry( ITAG itag ) const;
        ULONG CbGetEntry( ITAG itag ) const;
        ERR ErrMEMPOOLInit( ULONG cbInitialSize, USHORT cInitialEntries, BOOL fPadding );
        VOID MEMPOOLRelease();
        ERR ErrAddEntry( BYTE *rgb, ULONG cb, ITAG *pitag );
        VOID DeleteEntry( ITAG itag );
        ERR ErrReplaceEntry( ITAG itag, BYTE *rgb, ULONG cb );
        BOOL FCompact();

#ifdef DEBUGGER_EXTENSION
        VOID Dump(
            CPRINTF *       pcprintf,
            const BOOL      fDumpTags,
            const BOOL      fDumpAll,
            const ITAG      itagDump,
            const DWORD_PTR dwOffset );

        VOID DumpTag( CPRINTF * pcprintf, const ITAG itag, const SIZE_T lShift ) const;
#endif
};

INLINE VOID MEMPOOL::AssertValid() const
{
#ifdef DEBUG
    MEMPOOLTAG  *rgbTags;
    ULONG       cTotalTags;

    Assert( Pbuf() != NULL );

    rgbTags = (MEMPOOLTAG *)Pbuf();

    Assert( rgbTags[itagTagArray].ib == 0 );
    Assert( rgbTags[itagTagArray].cb % sizeof(MEMPOOLTAG) == 0 );
    cTotalTags = rgbTags[itagTagArray].cb / sizeof(MEMPOOLTAG);
    Assert( cTotalTags <= (ULONG)cMaxEntries );

    Assert( CbBufSize() >= rgbTags[itagTagArray].cb );
    Assert( IbBufFree() >= rgbTags[itagTagArray].ib + rgbTags[itagTagArray].cb );
    Assert( IbBufFree() <= CbBufSize() );
    Assert( ItagUnused() >= itagFirstUsable );
    Assert( ItagUnused() <= cTotalTags );
    Assert( ItagFreed() >= itagFirstUsable );
    Assert( ItagFreed() <= ItagUnused() );
#endif
}

INLINE VOID MEMPOOL::AssertValidTag( ITAG iTagEntry ) const
{
#ifdef DEBUG
    MEMPOOLTAG  *rgbTags = (MEMPOOLTAG *)Pbuf();

    Assert( iTagEntry >= itagFirstUsable );
    Assert( iTagEntry < ItagUnused() );

    Assert( rgbTags[iTagEntry].cb > 0 );
    Assert( DWORD_PTR( rgbTags[iTagEntry].ib ) % sizeof( QWORD ) == 0 );
    Assert( rgbTags[iTagEntry].ib >= rgbTags[itagTagArray].ib + rgbTags[itagTagArray].cb );
    Assert( rgbTags[iTagEntry].ib + rgbTags[iTagEntry].cb <= IbBufFree() );
#endif
}


INLINE VOID MEMPOOL::MEMPOOLRelease()
{
    if ( Pbuf() != NULL )
    {
        AssertValid();
        OSMemoryHeapFree( Pbuf() );
        SetPbuf( NULL );
    }
}

INLINE BYTE *MEMPOOL::PbGetEntry( ITAG itag ) const
{
    MEMPOOLTAG  *rgbTags;

    AssertValid();
    AssertValidTag( itag );

    rgbTags = (MEMPOOLTAG *)Pbuf();

    return Pbuf() + rgbTags[itag].ib;
}

INLINE ULONG MEMPOOL::CbGetEntry( ITAG itag ) const
{
    MEMPOOLTAG  *rgbTags;

    AssertValid();
    AssertValidTag( itag );

    rgbTags = (MEMPOOLTAG *)Pbuf();

    return rgbTags[itag].cb;
}


typedef ULONG FIELDFLAG;


const FIELDFLAG ffieldNotNull                   = 0x0001;
const FIELDFLAG ffieldVersion                   = 0x0002;
const FIELDFLAG ffieldAutoincrement             = 0x0004;
const FIELDFLAG ffieldMultivalued               = 0x0008;
const FIELDFLAG ffieldDefault                   = 0x0010;
const FIELDFLAG ffieldEscrowUpdate              = 0x0020;
const FIELDFLAG ffieldFinalize                  = 0x0040;
const FIELDFLAG ffieldUserDefinedDefault        = 0x0080;
const FIELDFLAG ffieldTemplateColumnESE98       = 0x0100;
const FIELDFLAG ffieldDeleteOnZero              = 0x0200;
const FIELDFLAG ffieldPrimaryIndexPlaceholder   = 0x0800;
const FIELDFLAG ffieldCompressed                = 0x1000;
const FIELDFLAG ffieldEncrypted                 = 0x2000;

const FIELDFLAG ffieldPersistedMask             = 0xffff;

const FIELDFLAG ffieldVersioned                 = 0x10000;
const FIELDFLAG ffieldDeleted                   = 0x20000;
const FIELDFLAG ffieldVersionedAdd              = 0x40000;

INLINE BOOL FFIELDNotNull( FIELDFLAG ffield )           { return ( ffield & ffieldNotNull ); }
INLINE VOID FIELDSetNotNull( FIELDFLAG &ffield )        { ffield |= ffieldNotNull; }

INLINE BOOL FFIELDVersion( FIELDFLAG ffield )           { return ( ffield & ffieldVersion ); }
INLINE VOID FIELDSetVersion( FIELDFLAG &ffield )        { ffield |= ffieldVersion; }

INLINE BOOL FFIELDAutoincrement( FIELDFLAG ffield )     { return ( ffield & ffieldAutoincrement ); }
INLINE VOID FIELDSetAutoincrement( FIELDFLAG &ffield )  { ffield |= ffieldAutoincrement; }

INLINE BOOL FFIELDMultivalued( FIELDFLAG ffield )       { return ( ffield & ffieldMultivalued ); }
INLINE VOID FIELDSetMultivalued( FIELDFLAG &ffield )    { ffield |= ffieldMultivalued; }

INLINE BOOL FFIELDDefault( FIELDFLAG ffield )           { return ( ffield & ffieldDefault ); }
INLINE VOID FIELDSetDefault( FIELDFLAG &ffield )        { ffield |= ffieldDefault; }
INLINE VOID FIELDResetDefault( FIELDFLAG &ffield )      { ffield &= ~ffieldDefault; }

INLINE BOOL FFIELDEscrowUpdate( FIELDFLAG ffield )      { return ( ffield & ffieldEscrowUpdate ); }
INLINE VOID FIELDSetEscrowUpdate( FIELDFLAG &ffield )   { ffield |= ffieldEscrowUpdate; }

INLINE BOOL FFIELDFinalize( FIELDFLAG ffield )          { return ( ffield & ffieldFinalize ); }
INLINE VOID FIELDSetFinalize( FIELDFLAG &ffield )       { ffield |= ffieldFinalize; }

INLINE BOOL FFIELDDeleteOnZero( FIELDFLAG ffield )      { return ( ffield & ffieldDeleteOnZero ); }
INLINE VOID FIELDSetDeleteOnZero( FIELDFLAG &ffield )   { ffield |= ffieldDeleteOnZero; }

INLINE BOOL FFIELDUserDefinedDefault( FIELDFLAG ffield )        { return ( ffield & ffieldUserDefinedDefault ); }
INLINE VOID FIELDSetUserDefinedDefault( FIELDFLAG &ffield )     { ffield |= ffieldUserDefinedDefault; }

INLINE BOOL FFIELDTemplateColumnESE98( FIELDFLAG ffield )       { return ( ffield & ffieldTemplateColumnESE98 ); }
INLINE VOID FIELDSetTemplateColumnESE98( FIELDFLAG &ffield )    { ffield |= ffieldTemplateColumnESE98; }

INLINE BOOL FFIELDPrimaryIndexPlaceholder( FIELDFLAG ffield )   { return ( ffield & ffieldPrimaryIndexPlaceholder ); }
INLINE VOID FIELDSetPrimaryIndexPlaceholder( FIELDFLAG &ffield ){ ffield |= ffieldPrimaryIndexPlaceholder; }

INLINE BOOL FFIELDCompressed( FIELDFLAG ffield )                { return ( ffield & ffieldCompressed ); }
INLINE VOID FIELDSetCompressed( FIELDFLAG &ffield )             { ffield |= ffieldCompressed; }

INLINE BOOL FFIELDEncrypted( FIELDFLAG ffield )                 { return ( ffield & ffieldEncrypted ); }
INLINE VOID FIELDSetEncrypted( FIELDFLAG &ffield )              { ffield |= ffieldEncrypted; }

INLINE FIELDFLAG FIELDFLAGPersisted( const FIELDFLAG ffield )   { return FIELDFLAG( ffield & ffieldPersistedMask ); }


INLINE BOOL FFIELDVersioned( FIELDFLAG ffield )         { return ( ffield & ffieldVersioned ); }
INLINE VOID FIELDSetVersioned( FIELDFLAG &ffield )      { ffield |= ffieldVersioned; }
INLINE VOID FIELDResetVersioned( FIELDFLAG &ffield )    { ffield &= ~ffieldVersioned; }

INLINE BOOL FFIELDDeleted( FIELDFLAG ffield )           { return ( ffield & ffieldDeleted ); }
INLINE BOOL FFIELDCommittedDelete( FIELDFLAG ffield )
{
    return ( FFIELDDeleted( ffield ) && !FFIELDVersioned( ffield ) );
}
INLINE VOID FIELDSetDeleted( FIELDFLAG &ffield )
{
    Assert( !( ffield & ffieldDeleted ) );
    ffield |= ffieldDeleted;
}
INLINE VOID FIELDResetDeleted( FIELDFLAG &ffield )
{
    Assert( ffield & ffieldDeleted );
    ffield &= ~ffieldDeleted;
}

INLINE BOOL FFIELDVersionedAdd( FIELDFLAG ffield )
{
    return ( ffield & ffieldVersionedAdd );
}
INLINE VOID FIELDSetVersionedAdd( FIELDFLAG &ffield )
{
    Assert( !( ffield & ffieldVersionedAdd ) );
    ffield |= ffieldVersionedAdd;
}
INLINE VOID FIELDResetVersionedAdd( FIELDFLAG &ffield )
{
    ffield &= ~ffieldVersionedAdd;
}


struct CBDESC
{
    CBDESC          *pcbdescNext;
    CBDESC          **ppcbdescPrev;

    JET_CALLBACK    pcallback;
    JET_CBTYP       cbtyp;
    VOID            *pvContext;
    ULONG           cbContext;
    ULONG           ulId;

    ULONG           fPermanent:1;
    ULONG           fVersioned:1;

#ifdef VERSIONED_CALLBACKS
    TRX             trxRegisterBegin0;
    TRX             trxRegisterCommit0;
    TRX             trxUnregisterBegin0;
    TRX             trxUnregisterCommit0;
#endif
};




typedef USHORT  FIELD_COLTYP;

struct FIELD
{
    FIELD_COLTYP    coltyp;
    STRHASH         strhashFieldName;
    ULONG           cbMaxLen;
    FIELDFLAG       ffield;
    WORD            ibRecordOffset;
    MEMPOOL::ITAG   itagFieldName;
    USHORT          cp;
};

const ULONG cbFIELDPlaceholder      = 1;

const MEMPOOL::ITAG itagTDBFields                   = 1;
const MEMPOOL::ITAG itagTDBTableName                = 2;

const MEMPOOL::ITAG itagTDBTempTableIdxSeg          = 2;
const MEMPOOL::ITAG itagTDBTempTableNameWithIdxSeg  = 3;

extern const WORD ibRECStartFixedColumns;


typedef ULONG   DBK;
ERR ErrRECIInitAutoIncrement( _In_ FUCB* const pfucb, QWORD qwAutoInc );


class TDB
    :   public CZeroInit
{
    public:
        TDB(
            INST* pinst,
            FID fidFixedFirst,
            FID fidFixedLast,
            FID fidVarFirst,
            FID fidVarLast,
            FID fidTaggedFirst,
            FID fidTaggedLast,
            WORD ibEndFixedColumns,
            FCB *pfcbTemplateTable,
            INT subrank );
        ~TDB();

#pragma push_macro( "new" )
#undef new
    private:
        void* operator new( size_t );
        void* operator new[]( size_t );
        void operator delete[]( void* );
    public:
        void* operator new( size_t cbAlloc, INST* pinst )
        {
            return pinst->m_cresTDB.PvRESAlloc_( SzNewFile(), UlNewLine() );
        }
        void operator delete( void* pv )
        {
            if ( pv )
            {
                TDB* ptdb = reinterpret_cast< TDB* >( pv );
                ptdb->m_pinst->m_cresTDB.Free( pv );
            }
        }
#pragma pop_macro( "new" )

    private:
        const FID       m_fidTaggedFirst;
        const FID       m_fidTaggedLastInitial;
        const FID       m_fidFixedFirst;
        const FID       m_fidFixedLastInitial;
        const FID       m_fidVarFirst;
        const FID       m_fidVarLastInitial;
        FID             m_fidTaggedLast;
        FID             m_fidFixedLast;
        FID             m_fidVarLast;
        FID             m_fidTaggedLastOfESE97Template;
        MEMPOOL::ITAG   m_itagTableName;
        USHORT          m_ibEndFixedColumns;

        FIELD *         m_pfieldsInitial;

        DATA *          m_pdataDefaultRecord;
        FCB             *m_pfcbTemplateTable;

        union
        {
            ULONG       m_ulFlags;
            struct
            {
                ULONG   m_fTDBTemplateTable:1;
                ULONG   m_fTDBESE97TemplateTable:1;
                ULONG   m_fTDBDerivedTable:1;
                ULONG   m_fTDBESE97DerivedTable:1;
                ULONG   m_f8BytesAutoInc:1;
                ULONG   m_fTableHasFinalizeColumn:1;
                ULONG   m_fTableHasDeleteOnZeroColumn:1;
                ULONG   m_fTableHasDefault:1;
                ULONG   m_fTableHasNonEscrowDefault:1;
                ULONG   m_fTableHasUserDefinedDefault:1;
                ULONG   m_fTableHasNonNullFixedColumn:1;
                ULONG   m_fTableHasNonNullVarColumn:1;
                ULONG   m_fTableHasNonNullTaggedColumn:1;
                ULONG   m_fInitialisingDefaultRecord:1;
                ULONG   m_fAutoIncOldFormat:1;
                ULONG   m_fLid64:1;
            };
        };

        FCB             *m_pfcbLV;

        volatile unsigned __int64
                        m_lidLast;

        union
        {
            QWORD       m_qwAutoincrement;
            ULONG       m_ulAutoincrement;
        };
        FID             m_fidVersion;
        FID             m_fidAutoincrement;

        DBK             m_dbkMost;
        MEMPOOL         m_mempool;

        CBinaryLock     m_blIndexes;

        BYTE            m_rgbitAllIndex[cbRgbitIndex];

        CReaderWriterLock   m_rwlDDL;

        CBDESC          *m_pcbdesc;

        INST*           m_pinst;

        ULONG           m_cbPreferredIntrinsicLV;
        LONG            m_cbLVChunkMost;
        MEMPOOL::ITAG   m_itagDeferredLVSpacehints;


    private:
        DWORD               m_dwAutoIncBatchSize;
        QWORD               m_qwAllocatedAutoIncMax;
        BYTE                m_bReserved[8];


        CInitOnce< ERR, decltype( &ErrRECIInitAutoIncrement ), FUCB * const, QWORD >  m_AutoIncInitOnce;

#ifdef _AMD64_
        BYTE                m_bReserved2[8];
#else

#endif


    public:
        MEMPOOL& MemPool() const;
        FID FidFixedFirst() const;
        FID FidFixedLastInitial() const;
        FID FidFixedLast() const;
        FID FidVarFirst() const;
        FID FidVarLastInitial() const;
        FID FidVarLast() const;
        FID FidTaggedFirst() const;
        FID FidTaggedLastInitial() const;
        FID FidTaggedLast() const;
        MEMPOOL::ITAG ItagTableName() const;
    private:
        MEMPOOL::ITAG ItagDeferredLVSpacehints() const;
    public:
        const JET_SPACEHINTS * PjsphDeferredLV() const;
        WORD IbEndFixedColumns() const;
        FID FidTaggedLastOfESE97Template() const;
        FID FidVersion() const;
        FID FidAutoincrement() const;
        QWORD QwAutoincrement() const;
        FIELD * PfieldsInitial() const;
        DATA * PdataDefaultRecord() const;
        DBK DbkMost() const;
        unsigned __int64 Ui64LidLast() const;
        volatile unsigned __int64* PUi64LidLast();
        FCB *PfcbLV() const;
        ULONG CbPreferredIntrinsicLV() const;
        LONG CbLVChunkMost() const;
        FCB *PfcbTemplateTable() const;
        BYTE *RgbitAllIndex() const;
        const CBDESC *Pcbdesc() const;

    public:
        QWORD               QwGetAllocatedAutoIncMax() const { return m_qwAllocatedAutoIncMax; }
        VOID                SetAllocatedAutoIncMax( _In_ const QWORD qwAllocatedAutoIncMax ) { m_qwAllocatedAutoIncMax = qwAllocatedAutoIncMax; }
        BOOL                FAutoIncOldFormat() const { return m_fAutoIncOldFormat; }
        VOID                SetAutoIncOldFormat() { m_fAutoIncOldFormat = fTrue; }
        VOID                ResetAutoIncOldFormat() { m_fAutoIncOldFormat = fFalse; }
        DWORD               DwGetAutoIncBatchSize() const { return m_dwAutoIncBatchSize; }
        VOID                SetAutoIncBatchSize( _In_ const DWORD dwAutoIncBatchSize ) { m_dwAutoIncBatchSize = dwAutoIncBatchSize; }
        ERR                 ErrInitAutoInc( FUCB* const pfucb, QWORD qwAutoInc );
        VOID                ResetAutoIncInitOnce() { m_AutoIncInitOnce.Reset(); }

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif

    public:
        VOID SetMemPool( const MEMPOOL& mempool );
        VOID IncrementFidFixedLast();
        VOID IncrementFidVarLast();
        VOID IncrementFidTaggedLast();
        VOID SetItagTableName( MEMPOOL::ITAG itag );
        VOID SetItagDeferredLVSpacehints( MEMPOOL::ITAG itag );
        VOID SetIbEndFixedColumns( const WORD ibRec, const FID fidFixedLast );
        VOID SetFidTaggedLastOfESE97Template( const FID fid );
        VOID SetFidVersion( FID fid );
        VOID ResetFidVersion();
        VOID ResetFidAutoincrement();
        VOID SetFidAutoincrement( FID fid, BOOL f8BytesAutoInc );
        VOID InitAutoincrement( QWORD qw );
        ERR ErrGetAndIncrAutoincrement( QWORD * const qwT );

    public:
        VOID SetPfieldInitial( FIELD * const pfield );
        VOID SetPdataDefaultRecord( DATA * const pdata );
        VOID InitDbkMost( DBK dbk );

        BOOL FTemplateTable() const                     { return m_fTDBTemplateTable; }
        VOID SetFTemplateTable()                        { m_fTDBTemplateTable = fTrue; }
        BOOL FESE97TemplateTable() const                { return m_fTDBESE97TemplateTable; }
        VOID SetFESE97TemplateTable()                   { m_fTDBESE97TemplateTable = fTrue; }
        BOOL FDerivedTable() const                      { return m_fTDBDerivedTable; }
        VOID SetFDerivedTable()                         { m_fTDBDerivedTable = fTrue; }
        BOOL FESE97DerivedTable() const                 { return m_fTDBESE97DerivedTable; }
        VOID SetFESE97DerivedTable()                    { m_fTDBESE97DerivedTable = fTrue; }
        BOOL F8BytesAutoInc() const                     { return m_f8BytesAutoInc; }
        BOOL FTableHasFinalizeColumn() const            { return m_fTableHasFinalizeColumn; }
        VOID SetFTableHasFinalizeColumn()               { m_fTableHasFinalizeColumn = fTrue; }
        BOOL FTableHasDeleteOnZeroColumn() const        { return m_fTableHasDeleteOnZeroColumn; }
        VOID SetFTableHasDeleteOnZeroColumn()           { m_fTableHasDeleteOnZeroColumn = fTrue; }
        BOOL FTableHasDefault() const                   { return m_fTableHasDefault; }
        VOID SetFTableHasDefault()                      { m_fTableHasDefault = fTrue; }
        BOOL FTableHasNonEscrowDefault() const          { return m_fTableHasNonEscrowDefault; }
        VOID SetFTableHasNonEscrowDefault()             { m_fTableHasNonEscrowDefault = fTrue; }
        BOOL FTableHasUserDefinedDefault() const        { return m_fTableHasUserDefinedDefault; }
        VOID SetFTableHasUserDefinedDefault()           { m_fTableHasUserDefinedDefault = fTrue; }
        BOOL FTableHasNonNullFixedColumn() const        { return m_fTableHasNonNullFixedColumn; }
        VOID SetFTableHasNonNullFixedColumn()           { m_fTableHasNonNullFixedColumn = fTrue; }
        BOOL FTableHasNonNullVarColumn() const          { return m_fTableHasNonNullVarColumn; }
        VOID SetFTableHasNonNullVarColumn()             { m_fTableHasNonNullVarColumn = fTrue; }
        BOOL FTableHasNonNullTaggedColumn() const       { return m_fTableHasNonNullTaggedColumn; }
        VOID SetFTableHasNonNullTaggedColumn()          { m_fTableHasNonNullTaggedColumn = fTrue; }
        BOOL FLid64() const                             { return m_fLid64; }
        VOID SetFLid64( BOOL fLid64 );

        BOOL FInitialisingDefaultRecord() const         { return m_fInitialisingDefaultRecord; }
        VOID SetFInitialisingDefaultRecord()
        {
#ifdef DEBUG
            m_fInitialisingDefaultRecord = fTrue;
#endif
        }
        VOID ResetFInitialisingDefaultRecord()
        {
#ifdef DEBUG
            m_fInitialisingDefaultRecord = fFalse;
#endif
        }

        VOID SetDbkMost( DBK dbk ) { m_dbkMost = dbk; }

        ERR ErrGetAndIncrDbkMost( DBK * const pdbk );
        VOID SetPfcbLV( FCB *pfcbLV );
        VOID SetPreferredIntrinsicLV( ULONG cb );
        VOID SetLVChunkMost( LONG cb );
        VOID SetPfcbTemplateTable( FCB *pfcb );
        VOID AssertValidTemplateTable() const;
        VOID AssertValidDerivedTable() const;
        VOID SetRgbitAllIndex( const BYTE *rgbit );
        VOID ResetRgbitAllIndex();
        VOID RegisterPcbdesc( CBDESC * const pcbdesc );
        VOID UnregisterPcbdesc( CBDESC * const pcbdesc );

        VOID MaterializeFids( TDB *ptdbSrc );

        CHAR *SzTableName() const;
        CHAR *SzIndexName( MEMPOOL::ITAG itagIndexName, BOOL fDerivedIndex ) const;
        CHAR *SzFieldName( MEMPOOL::ITAG itagFieldName, BOOL fDerivedColumn ) const;
        VOID AssertFIELDValid( const FIELD * const pfield ) const;
        FIELD *Pfield( const COLUMNID columnid ) const;
        FIELD *PfieldFixed( const COLUMNID columnid ) const;
        FIELD *PfieldVar( const COLUMNID columnid ) const;
        FIELD *PfieldTagged( const COLUMNID columnid ) const;
        WORD IbOffsetOfNextColumn( const FID fid ) const;

        ULONG CInitialFixedColumns() const;
        ULONG CInitialVarColumns() const;
        ULONG CInitialTaggedColumns() const;
        ULONG CInitialColumns() const;
        ULONG CDynamicFixedColumns() const;
        ULONG CDynamicVarColumns() const;
        ULONG CDynamicTaggedColumns() const;
        ULONG CDynamicColumns() const;
        ULONG CFixedColumns() const;
        ULONG CVarColumns() const;
        ULONG CTaggedColumns() const;
        ULONG CColumns() const;

        BOOL FFixedTemplateColumn( const FID fid ) const;
        BOOL FVarTemplateColumn( const FID fid ) const;

    private:
        CHAR *SzName_( MEMPOOL::ITAG iTagEntry ) const;


    public:
        CBinaryLock& BlIndexes();
        VOID EnterUpdating();
        VOID LeaveUpdating();
        VOID EnterIndexing();
        VOID LeaveIndexing();

        CReaderWriterLock& RwlDDL();
};

INLINE TDB::TDB(
    INST*   pinst,
    FID     fidFixedFirst,
    FID     fidFixedLast,
    FID     fidVarFirst,
    FID     fidVarLast,
    FID     fidTaggedFirst,
    FID     fidTaggedLast,
    WORD    ibEndFixedColumns,
    FCB     *pfcbTemplateTable,
    INT     subrank )
    :   CZeroInit( sizeof( TDB ) ),
        m_pinst( pinst ),
        m_fidTaggedFirst( fidTaggedFirst ),
        m_fidTaggedLastInitial( fidTaggedLast ),
        m_fidFixedFirst( fidFixedFirst ),
        m_fidFixedLastInitial( fidFixedLast ),
        m_fidVarFirst( fidVarFirst ),
        m_fidVarLastInitial( fidVarLast ),
        m_blIndexes( CLockBasicInfo( CSyncBasicInfo( szIndexingUpdating ), rankIndexingUpdating, subrank ) ),
        m_rwlDDL( CLockBasicInfo( CSyncBasicInfo( szDDLDML ), rankDDLDML, subrank ) ),
        m_dwAutoIncBatchSize( 1 )
{
    m_pcbdesc = NULL;
    m_fidTaggedLast = fidTaggedLast;
    m_fidFixedLast = fidFixedLast;
    m_fidVarLast = fidVarLast;
    Assert( 0 == m_ulFlags );
    m_ibEndFixedColumns = ibEndFixedColumns;
    
    if ( pfcbTemplateTable != NULL )
    {
        pfcbTemplateTable->IncrementRefCount();
        m_pfcbTemplateTable = pfcbTemplateTable;
    }

    Assert( (LONG_PTR)m_rgbitAllIndex % sizeof(LONG_PTR) == 0 );

    Assert( pfieldNil == m_pfieldsInitial );
    Assert( NULL == m_pdataDefaultRecord );
    Assert( 0 == m_fidTaggedLastOfESE97Template );
    Assert( 0 == m_fidVersion );
    Assert( 0 == m_fidAutoincrement );

    Assert( m_fidFixedFirst.FFixed() || ( m_pfcbTemplateTable && m_fidFixedFirst.FFixedNoneFull() ) );
    Assert( m_fidFixedLastInitial.FFixedNone() || m_fidFixedLastInitial.FFixed() );
    Assert( m_fidFixedLast.FFixedNone() || m_fidFixedLast.FFixed() );

    Assert( m_fidVarFirst.FVar() || ( m_pfcbTemplateTable && m_fidVarFirst.FVarNoneFull() ) );
    Assert( m_fidVarLastInitial.FVarNone() || m_fidVarLastInitial.FVar() );
    Assert( m_fidVarLast.FVarNone() || m_fidVarLast.FVar() );

    Assert( m_fidTaggedFirst.FTagged() || ( m_pfcbTemplateTable && m_fidTaggedFirst.FTaggedNoneFull() ) );
    Assert( m_fidTaggedLastInitial.FTaggedNone() || m_fidTaggedLastInitial.FTagged() );
    Assert( m_fidTaggedLast.FTaggedNone() || m_fidTaggedLast.FTagged() );
   
}

INLINE TDB::~TDB()
{
    Assert( PfcbLV() == pfcbNil );

    if ( m_pfcbTemplateTable != NULL )
    {
        m_pfcbTemplateTable->Release();
        m_pfcbTemplateTable = NULL;
    }

    MemPool().MEMPOOLRelease();
    OSMemoryHeapFree( PfieldsInitial() );
    OSMemoryHeapFree( PdataDefaultRecord() );

    CBDESC * pcbdesc = m_pcbdesc;
    while( NULL != pcbdesc )
    {
        CBDESC * const pcbdescDelete = pcbdesc;
        pcbdesc = pcbdesc->pcbdescNext;
        if( pcbdescDelete->fPermanent )
        {
            OSMemoryHeapFree( pcbdescDelete->pvContext );
        }
        delete pcbdescDelete;
    }
}

INLINE MEMPOOL& TDB::MemPool() const                        { return const_cast<TDB *>( this )->m_mempool; }

INLINE FID TDB::FidFixedFirst() const
{
    Assert( m_fidFixedFirst == fidFixedLeast || pfcbNil != PfcbTemplateTable() );
    return m_fidFixedFirst;
}
INLINE FID TDB::FidFixedLastInitial() const                 { return m_fidFixedLastInitial; }
INLINE FID TDB::FidFixedLast() const                        { return m_fidFixedLast; }

INLINE FID TDB::FidVarFirst() const
{
    Assert( m_fidVarFirst == fidVarLeast || pfcbNil != PfcbTemplateTable() );
    return m_fidVarFirst;
}
INLINE FID TDB::FidVarLastInitial() const                   { return m_fidVarLastInitial; }
INLINE FID TDB::FidVarLast() const                          { return m_fidVarLast; }

INLINE FID TDB::FidTaggedFirst() const
{
    Assert( m_fidTaggedFirst == fidTaggedLeast || pfcbNil != PfcbTemplateTable() );
    return m_fidTaggedFirst;
}
INLINE FID TDB::FidTaggedLastInitial() const                { return m_fidTaggedLastInitial; }
INLINE FID TDB::FidTaggedLast() const                       { return m_fidTaggedLast; }

INLINE MEMPOOL::ITAG TDB::ItagTableName() const             { return m_itagTableName; }
INLINE MEMPOOL::ITAG TDB::ItagDeferredLVSpacehints() const  { return m_itagDeferredLVSpacehints; }
INLINE const JET_SPACEHINTS * TDB::PjsphDeferredLV() const
{
    const JET_SPACEHINTS * pjsph = NULL;
    if ( m_itagDeferredLVSpacehints != 0 )
    {
        pjsph = reinterpret_cast<const JET_SPACEHINTS *>( MemPool().PbGetEntry( ItagDeferredLVSpacehints() ) );
        Assert( sizeof(*pjsph) == MemPool().CbGetEntry( ItagDeferredLVSpacehints() ) );
    }
    return pjsph;
}
INLINE WORD TDB::IbEndFixedColumns() const
{
    Assert( m_ibEndFixedColumns >= ibRECStartFixedColumns );
    return m_ibEndFixedColumns;
}
INLINE FID TDB::FidTaggedLastOfESE97Template() const        { return m_fidTaggedLastOfESE97Template; }
INLINE FID TDB::FidVersion() const                          { return m_fidVersion; }
INLINE FID TDB::FidAutoincrement() const                    { return m_fidAutoincrement; }
INLINE QWORD TDB::QwAutoincrement() const                   { return m_qwAutoincrement; }
INLINE FIELD * TDB::PfieldsInitial() const                  { return m_pfieldsInitial; }
INLINE DATA * TDB::PdataDefaultRecord() const               { return m_pdataDefaultRecord; }
INLINE DBK TDB::DbkMost() const                             { return m_dbkMost; }
INLINE unsigned __int64 TDB::Ui64LidLast() const            { return m_lidLast; }
INLINE volatile unsigned __int64* TDB::PUi64LidLast()       { return &m_lidLast; }
INLINE FCB *TDB::PfcbLV() const                             { return m_pfcbLV; }
INLINE ULONG TDB::CbPreferredIntrinsicLV() const            { return m_cbPreferredIntrinsicLV; }
INLINE LONG TDB::CbLVChunkMost() const                      { return m_cbLVChunkMost; }
INLINE FCB *TDB::PfcbTemplateTable() const                  { return m_pfcbTemplateTable; }
INLINE BYTE *TDB::RgbitAllIndex() const                     { return const_cast<TDB *>( this )->m_rgbitAllIndex; }
INLINE const CBDESC *TDB::Pcbdesc() const                   { return m_pcbdesc; }

INLINE VOID TDB::SetMemPool( const MEMPOOL& mempool )       { m_mempool = mempool; }
INLINE VOID TDB::IncrementFidFixedLast()                    { m_fidFixedLast++; Expected( m_fidFixedLast.FFixed() ); }
INLINE VOID TDB::IncrementFidVarLast()                      { m_fidVarLast++; Expected( m_fidVarLast.FVar() ); }
INLINE VOID TDB::IncrementFidTaggedLast()                   { m_fidTaggedLast++; Expected( m_fidTaggedLast.FTagged() ); }
INLINE VOID TDB::SetPfieldInitial( FIELD * const pfield )   { m_pfieldsInitial = pfield; }
INLINE VOID TDB::SetPdataDefaultRecord( DATA * const pdata ){ m_pdataDefaultRecord = pdata; }
INLINE VOID TDB::SetPfcbLV( FCB *pfcbLV )                   { m_pfcbLV = pfcbLV; }
INLINE VOID TDB::SetPreferredIntrinsicLV( ULONG cb )        { m_cbPreferredIntrinsicLV = cb; }
INLINE VOID TDB::SetLVChunkMost( LONG cb )                  { m_cbLVChunkMost = cb; }

INLINE VOID TDB::SetPfcbTemplateTable( FCB *pfcb )          { m_pfcbTemplateTable = pfcb; }

INLINE VOID TDB::SetFidTaggedLastOfESE97Template( const FID fid )
{
    Assert( FTaggedFid( fid ) );
    m_fidTaggedLastOfESE97Template = fid;
}

INLINE VOID TDB::AssertValidTemplateTable() const
{
#ifdef DEBUG
    Assert( FTemplateTable() );
    Assert( !FDerivedTable() );
    Assert( pfcbNil == PfcbTemplateTable() );
    Assert( fidFixedLeast == FidFixedFirst() );
    Assert( fidVarLeast == FidVarFirst() );
    Assert( fidTaggedLeast == FidTaggedFirst() );
    Assert( FidFixedLastInitial() == FidFixedLast() );
    Assert( FidVarLastInitial() == FidVarLast() );
    Assert( FidTaggedLastInitial() <= FidTaggedLast() );
    Assert( fidFixedLeast-1 == FidFixedLast() || FFixedFid( FidFixedLast() ) );
    Assert( fidVarLeast-1 == FidVarLast() || FVarFid( FidVarLast() ) );
    Assert( fidTaggedLeast-1 == FidTaggedLast() || FTaggedFid( FidTaggedLast() ) );
    Assert( 0 == FidTaggedLastOfESE97Template() || FESE97TemplateTable() );
#endif
}
INLINE VOID TDB::AssertValidDerivedTable() const
{
#ifdef DEBUG
    Assert( FDerivedTable() );
    Assert( !FTemplateTable() );
    Assert( pfcbNil != PfcbTemplateTable() );
    Assert( PfcbTemplateTable()->FTemplateTable() );
    Assert( PfcbTemplateTable()->FFixedDDL() );

    const TDB   * const ptdbTemplate    = PfcbTemplateTable()->Ptdb();
    Assert( ptdbNil != ptdbTemplate );
    ptdbTemplate->AssertValidTemplateTable();

    if ( ptdbTemplate->FESE97TemplateTable() )
        Assert( FESE97DerivedTable() );
    else
        Assert( !FESE97DerivedTable() );

    Assert( fidTaggedLeast == FidTaggedFirst()
        || ( FESE97DerivedTable()
            && FidTaggedFirst() == ptdbTemplate->FidTaggedLastOfESE97Template() + 1 ) );
#endif
}

INLINE VOID TDB::SetItagTableName( MEMPOOL::ITAG itag )
{
    m_itagTableName = itag;
}

INLINE VOID TDB::SetItagDeferredLVSpacehints( MEMPOOL::ITAG itag )
{
    m_itagDeferredLVSpacehints = itag;
}

INLINE VOID TDB::SetIbEndFixedColumns( const WORD ibRec, const FID fidFixedLast )
{
    Assert( m_ibEndFixedColumns >= ibRECStartFixedColumns );
    m_ibEndFixedColumns = ibRec;

#ifdef DEBUG
    if ( fidFixedLast >= FidFixedFirst() )
    {
        FIELD *         pfield          = PfieldFixed( ColumnidOfFid( FidFixedFirst(), FTemplateTable() ) );
        FIELD *         pfieldPrev;

        Assert( ibRec > ibRECStartFixedColumns );
        if ( pfcbNil == PfcbTemplateTable() )
        {
            Assert( pfield->ibRecordOffset == ibRECStartFixedColumns );
        }
        else
        {
            Assert( PfcbTemplateTable()->FTemplateTable() );
            Assert( PfcbTemplateTable()->FFixedDDL() );
            Assert( PfcbTemplateTable()->Ptdb() != ptdbNil );
            Assert( pfield->ibRecordOffset == PfcbTemplateTable()->Ptdb()->IbEndFixedColumns() );
        }

        pfieldPrev = pfield;
        for ( FID fid = FidFixedFirst() + 1; fid <= fidFixedLast; fid++ )
        {
            pfield = PfieldFixed( ColumnidOfFid( fid, FTemplateTable() ) );
            Assert( pfield->ibRecordOffset > ibRECStartFixedColumns );
            Assert( pfield->ibRecordOffset > pfieldPrev->ibRecordOffset );
            Assert( pfield->ibRecordOffset < ibRec );
            pfieldPrev = pfield;
        }

        Assert( pfield->cbMaxLen > 0 );
        Assert( pfield->ibRecordOffset + pfield->cbMaxLen == ibRec );
    }
    else
    {
        Assert( fidFixedLast == FidFixedFirst() - 1 );
        if ( pfcbNil == PfcbTemplateTable() )
        {
            Assert( ibRECStartFixedColumns == ibRec );
        }
        else
        {
            Assert( PfcbTemplateTable()->FTemplateTable() );
            Assert( PfcbTemplateTable()->FFixedDDL() );
            Assert( PfcbTemplateTable()->Ptdb() != ptdbNil );
            Assert( PfcbTemplateTable()->Ptdb()->IbEndFixedColumns() == ibRec );
        }
    }
#endif
}

INLINE VOID TDB::SetFidVersion( FID fid )
{
    Assert( 0 == m_fidVersion );
    Assert( 0 == fid || FFixedFid( fid ) );
    m_fidVersion = fid;
}
INLINE VOID TDB::ResetFidVersion()
{
    Assert( m_fidVersion != 0 );
    Assert( FFixedFid( m_fidVersion ) );
    m_fidVersion = 0;
}

INLINE VOID TDB::SetFidAutoincrement( FID fid, BOOL f8BytesAutoInc )
{
    Assert( 0 == m_fidAutoincrement );
    Assert( 0 == fid || FFixedFid( fid ) );
    m_fidAutoincrement = fid;
    m_f8BytesAutoInc = (USHORT)( fid != 0 ? f8BytesAutoInc : fFalse );
}

INLINE VOID TDB::InitAutoincrement( QWORD qw )
{
    if ( FOS64Bit() || !m_f8BytesAutoInc )
    {
        (VOID)AtomicCompareExchangePointer( (VOID **)&m_qwAutoincrement, 0, (VOID *)qw );
    }
    else
    {
        RwlDDL().EnterAsWriter();
        if ( 0 == m_qwAutoincrement )
            m_qwAutoincrement = qw;
        RwlDDL().LeaveAsWriter();
    }
}

INLINE ERR TDB::ErrGetAndIncrAutoincrement( QWORD * const pqwT )
{
    ERR err = JET_errSuccess;

    Assert( m_qwAutoincrement > 0 );

    if ( FOS64Bit() )
    {
        const QWORD     qwCounterMaxT   = ( m_f8BytesAutoInc ? qwCounterMax : QWORD( dwCounterMax ) );
        if ( FAtomicIncrementPointerMax( (volatile VOID **)&m_qwAutoincrement, (VOID **)pqwT, (VOID *)qwCounterMaxT ) )
        {
            Assert( *pqwT > 0 );
        }
        else
        {
            OSUHAEmitFailureTag( m_pinst, HaDbFailureTagAlertOnly, L"b6521cac-c519-47b5-80c6-fb5d695def3b" );
            err = ErrERRCheck( JET_errOutOfAutoincrementValues );
        }
    }

    else if ( !m_f8BytesAutoInc )
    {
        DWORD   dwInitial;
        if ( FAtomicIncrementMax( &m_ulAutoincrement, &dwInitial, dwCounterMax ) )
        {
            Assert( dwInitial > 0 );
            *pqwT = dwInitial;
        }
        else
        {
            OSUHAEmitFailureTag( m_pinst, HaDbFailureTagAlertOnly, L"dbfd2411-5766-4875-8ef5-e6b86a421438" );
            err = ErrERRCheck( JET_errOutOfAutoincrementValues );
        }
    }
    else
    {
        RwlDDL().EnterAsWriter();
        if ( m_qwAutoincrement < qwCounterMax )
        {
            *pqwT = m_qwAutoincrement++;
        }
        else
        {
            OSUHAEmitFailureTag( m_pinst, HaDbFailureTagAlertOnly, L"d42796d5-7d6c-475e-b6e7-1694e74cfcb0" );
            err = ErrERRCheck( JET_errOutOfAutoincrementValues );
        }
        RwlDDL().LeaveAsWriter();
    }

    return err;
}

INLINE VOID TDB::ResetFidAutoincrement()
{
    Assert( m_fidAutoincrement != 0 );
    Assert( FFixedFid( m_fidAutoincrement ) );
    m_f8BytesAutoInc = fFalse;
    m_fidAutoincrement = 0;
}
INLINE VOID TDB::InitDbkMost( DBK dbk )
{
    AtomicCompareExchange( (LONG *)&m_dbkMost, 0, dbk );
}
INLINE ERR TDB::ErrGetAndIncrDbkMost( DBK * const pdbk )
{
    ERR err = JET_errSuccess;
    
    if ( !FAtomicIncrementMax( &m_dbkMost, pdbk, dwCounterMax ) )
    {
        OSUHAEmitFailureTag( m_pinst, HaDbFailureTagAlertOnly, L"96ec45cd-a8d8-46b1-98a2-7a977ccae049" );
        Error( ErrERRCheck( JET_errOutOfSequentialIndexValues ) );
    }

HandleError:
    return err;
}
INLINE VOID TDB::SetRgbitAllIndex( const BYTE *rgbit )
{
    UtilMemCpy( m_rgbitAllIndex, rgbit, cbRgbitIndex );
}
INLINE VOID TDB::ResetRgbitAllIndex()
{
    memset( m_rgbitAllIndex, 0, cbRgbitIndex );
}
INLINE VOID TDB::RegisterPcbdesc( CBDESC * const pcbdescInsert )
{
    pcbdescInsert->ppcbdescPrev = &m_pcbdesc;
    pcbdescInsert->pcbdescNext  = const_cast<CBDESC *>( m_pcbdesc );
    if( pcbdescInsert->pcbdescNext )
    {
        pcbdescInsert->pcbdescNext->ppcbdescPrev = &pcbdescInsert->pcbdescNext;
    }
    m_pcbdesc = pcbdescInsert;

    Assert( *(pcbdescInsert->ppcbdescPrev) == pcbdescInsert );
    Assert( NULL == pcbdescInsert->pcbdescNext
            || pcbdescInsert->pcbdescNext->ppcbdescPrev == &pcbdescInsert->pcbdescNext );
}
INLINE VOID TDB::UnregisterPcbdesc( CBDESC * const pcbdescRemove )
{
    Assert( *(pcbdescRemove->ppcbdescPrev) == pcbdescRemove );
    Assert( NULL == pcbdescRemove->pcbdescNext
            || pcbdescRemove->pcbdescNext->ppcbdescPrev == &pcbdescRemove->pcbdescNext );
    if( NULL != pcbdescRemove->pcbdescNext )
    {
        Assert( pcbdescRemove->pcbdescNext->ppcbdescPrev == &pcbdescRemove->pcbdescNext );
        pcbdescRemove->pcbdescNext->ppcbdescPrev = pcbdescRemove->ppcbdescPrev;
    }
    Assert( *(pcbdescRemove->ppcbdescPrev) == pcbdescRemove );
    *(pcbdescRemove->ppcbdescPrev) = pcbdescRemove->pcbdescNext;
}

INLINE VOID TDB::MaterializeFids( TDB *ptdbSrc )
{
    Assert( m_fidFixedLast.FFixedNone() );
    Assert( m_fidVarLast.FVarNone() );
    Assert( m_fidTaggedLast.FTaggedNone() );
    Assert( m_ibEndFixedColumns == ibRECStartFixedColumns );
    Assert( m_fidVersion.FFixedNone() );
    Assert( m_fidAutoincrement.FFixedNone() );
    Assert( ptdbSrc->FidFixedLast().FFixedNone()     || ptdbSrc->FidFixedLast().FFixed() );
    Assert( ptdbSrc->FidVarLast().FVarNone()         || ptdbSrc->FidVarLast().FVar() );
    Assert( ptdbSrc->FidTaggedLast().FTaggedNone()   || ptdbSrc->FidTaggedLast().FTagged() );

    Assert( ptdbSrc->FidVersion().FFixedNone()       || ptdbSrc->FidVersion().FFixed() );
    Assert( ptdbSrc->FidAutoincrement().FFixedNone() || ptdbSrc->FidAutoincrement().FFixed() );


    m_fidFixedLast      = ptdbSrc->FidFixedLast();
    m_fidVarLast        = ptdbSrc->FidVarLast();
    m_fidTaggedLast     = ptdbSrc->FidTaggedLast();
    m_fidVersion        = ptdbSrc->FidVersion();
    m_fidAutoincrement  = ptdbSrc->FidAutoincrement();
    m_ibEndFixedColumns = ptdbSrc->IbEndFixedColumns();
}

INLINE CHAR *TDB::SzName_( MEMPOOL::ITAG iTagEntry ) const
{
    CHAR    *szString;

    szString = reinterpret_cast<CHAR *>( MemPool().PbGetEntry( iTagEntry ) );
    Assert( strlen( szString ) == MemPool().CbGetEntry( iTagEntry ) - 1 );
    Assert( strlen( szString ) <= JET_cbNameMost );

    return szString;
}

INLINE CHAR *TDB::SzTableName() const
{
    return SzName_( ItagTableName() );
}

INLINE CHAR *TDB::SzIndexName( MEMPOOL::ITAG itagIndexName, const BOOL fDerivedIndex = fFalse ) const
{
    CHAR    *szIndexName;
    Assert( itagIndexName != 0 );
    Assert( itagIndexName != itagTDBFields );
    Assert( itagIndexName != itagTDBTableName );
    if ( fDerivedIndex )
    {
        AssertValidDerivedTable();
        szIndexName = PfcbTemplateTable()->Ptdb()->SzName_( itagIndexName );
    }
    else
    {
        szIndexName = SzName_( itagIndexName );
    }
    return szIndexName;
}

INLINE CHAR *TDB::SzFieldName( MEMPOOL::ITAG itagFieldName, const BOOL fDerivedColumn ) const
{
    CHAR    *szFieldName;

    Assert( itagFieldName != 0 );
    Assert( itagFieldName != itagTDBFields );
    Assert( itagFieldName != itagTDBTableName );

    if ( fDerivedColumn )
    {
        AssertValidDerivedTable();
        szFieldName = PfcbTemplateTable()->Ptdb()->SzName_( itagFieldName );
    }
    else
    {
        szFieldName = SzName_( itagFieldName );
    }

    return szFieldName;
}

INLINE VOID TDB::AssertFIELDValid( const FIELD * const pfield ) const
{
#ifdef DEBUG
    const FIELD *   pfieldStart     = PfieldsInitial();
    const ULONG     cFieldsInitial  = CInitialColumns();

    if ( pfield >= pfieldStart && pfield < pfieldStart + cFieldsInitial )
    {
        NULL;
    }
    else
    {
        const ULONG cbFieldInfo     = MemPool().CbGetEntry( itagTDBFields );

        pfieldStart = (FIELD *)MemPool().PbGetEntry( itagTDBFields );

        Assert( FidFixedLastInitial() < FidFixedLast()
            || FidVarLastInitial() < FidVarLast()
            || FidTaggedLastInitial() < FidTaggedLast() );

        Assert( cbFieldInfo > 0 );
        Assert( cbFieldInfo % sizeof(FIELD) == 0 );
        Assert( pfield >= pfieldStart );
        Assert( pfield < pfieldStart + ( cbFieldInfo / sizeof(FIELD ) ) );
    }

    Assert( ( (BYTE *)pfield - (BYTE *)pfieldStart ) % sizeof(FIELD) == 0 );
#endif
}


INLINE FIELD *TDB::PfieldFixed( const COLUMNID columnid ) const
{
    const TDB *     ptdb;
    FIELD *         pfield;
    const FID       fid     = FidOfColumnid( columnid );

    Assert( FCOLUMNIDFixed( columnid ) );

    if ( !FCOLUMNIDTemplateColumn( columnid ) || FTemplateTable() )
    {
        Assert( !FTemplateTable() || FCOLUMNIDTemplateColumn( columnid ) );
        ptdb = this;
    }
    else
    {
        AssertValidDerivedTable();
        ptdb = PfcbTemplateTable()->Ptdb();
    }

    Assert( fid >= ptdb->FidFixedFirst() );
    Assert( fid <= ptdb->FidFixedLast() );
    Assert( ptdb->FidFixedLastInitial() >= ptdb->FidFixedFirst() - 1 );
    Assert( ptdb->FidFixedLastInitial() <= ptdb->FidFixedLast() );

    if ( fid <= ptdb->FidFixedLastInitial() )
    {
        pfield = ptdb->PfieldsInitial() + ( fid - ptdb->FidFixedFirst() );;
    }
    else
    {
        Assert( !ptdb->FTemplateTable() );
        pfield = (FIELD *)ptdb->MemPool().PbGetEntry( itagTDBFields )
                                            + ( fid - ( ptdb->FidFixedLastInitial() + 1 ) );
    }

    ptdb->AssertFIELDValid( pfield );
    return pfield;
}

INLINE FIELD *TDB::PfieldVar( const COLUMNID columnid ) const
{
    const TDB *     ptdb;
    FIELD *         pfield;
    const FID       fid     = FidOfColumnid( columnid );

    Assert( FCOLUMNIDVar( columnid ) );

    if ( !FCOLUMNIDTemplateColumn( columnid ) || FTemplateTable() )
    {
        Assert( !FTemplateTable() || FCOLUMNIDTemplateColumn( columnid ) );
        ptdb = this;
    }
    else
    {
        AssertValidDerivedTable();
        ptdb = PfcbTemplateTable()->Ptdb();
    }

    Assert( fid >= ptdb->FidVarFirst() );
    Assert( fid <= ptdb->FidVarLast() );
    Assert( ptdb->FidVarLastInitial() >= ptdb->FidVarFirst() - 1 );
    Assert( ptdb->FidVarLastInitial() <= ptdb->FidVarLast() );

    if ( fid <= ptdb->FidVarLastInitial() )
    {
        pfield = ptdb->PfieldsInitial()
                            + ( ptdb->FidFixedLastInitial() + 1 - ptdb->FidFixedFirst() )
                            + ( fid - ptdb->FidVarFirst() );
    }
    else
    {
        Assert( !ptdb->FTemplateTable() );
        pfield = (FIELD *)ptdb->MemPool().PbGetEntry( itagTDBFields )
                                            + ( ptdb->FidFixedLast() - ptdb->FidFixedLastInitial() )
                                            + ( fid - ( ptdb->FidVarLastInitial() + 1 ) );
    }

    ptdb->AssertFIELDValid( pfield );
    return pfield;
}

INLINE FIELD *TDB::PfieldTagged( const COLUMNID columnid ) const
{
    const TDB *     ptdb;
    FIELD *         pfield;
    const FID       fid     = FidOfColumnid( columnid );

    Assert( FCOLUMNIDTagged( columnid ) );

    if ( !FCOLUMNIDTemplateColumn( columnid ) || FTemplateTable() )
    {
        Assert( !FTemplateTable() || FCOLUMNIDTemplateColumn( columnid ) );
        ptdb = this;
    }
    else
    {
        AssertValidDerivedTable();
        ptdb = PfcbTemplateTable()->Ptdb();
    }

    Assert( fid >= ptdb->FidTaggedFirst() );
    Assert( fid <= ptdb->FidTaggedLast() );
    Assert( ptdb->FidTaggedLastInitial() >= ptdb->FidTaggedFirst() - 1 );
    Assert( ptdb->FidTaggedLastInitial() <= ptdb->FidTaggedLast() );

    if ( fid <= ptdb->FidTaggedLastInitial() )
    {
        pfield = ptdb->PfieldsInitial()
                            + ( ptdb->FidFixedLastInitial() + 1 - ptdb->FidFixedFirst() )
                            + ( ptdb->FidVarLastInitial() + 1 - ptdb->FidVarFirst() )
                            + ( fid - ptdb->FidTaggedFirst() );
    }
    else
    {

#ifdef DEBUG
        if( ptdb->FTemplateTable() )
        {
            Assert( ptdb->FidFixedLast() == ptdb->FidFixedLastInitial() );
            Assert( ptdb->FidVarLast() == ptdb->FidVarLastInitial() );
        }
#endif
        
        pfield = (FIELD *)ptdb->MemPool().PbGetEntry( itagTDBFields )
                                            + ( ptdb->FidFixedLast() - ptdb->FidFixedLastInitial() )
                                            + ( ptdb->FidVarLast() - ptdb->FidVarLastInitial() )
                                            + ( fid - ( ptdb->FidTaggedLastInitial() + 1 ) );
    }

    ptdb->AssertFIELDValid( pfield );
    return pfield;
}

INLINE FIELD *TDB::Pfield( const COLUMNID columnid ) const
{
    FIELD   *pfield;

    if ( FCOLUMNIDTagged( columnid ) )
    {
        pfield = PfieldTagged( columnid );
    }
    else if ( FCOLUMNIDFixed( columnid ) )
    {
        Assert( FidOfColumnid( columnid ) <= FidFixedLast() );
        pfield = PfieldFixed( columnid );
    }
    else
    {
        Assert( FCOLUMNIDVar( columnid ) );
        Assert( FidOfColumnid( columnid ) <= FidVarLast() );
        pfield = PfieldVar( columnid );
    }

    Assert( !FCOLUMNIDTemplateColumn( columnid )
        || JET_coltypNil != pfield->coltyp );

    return pfield;
}

INLINE WORD TDB::IbOffsetOfNextColumn( const FID fid ) const
{
    WORD    ib;

    Assert( fidFixedLeast-1 == fid || FFixedFid( fid ) );
    Assert( fid <= FidFixedLast() );

    if ( FidFixedLast() == fid )
    {
        ib = IbEndFixedColumns();
    }
#ifdef DEBUG
    else if ( FInitialisingDefaultRecord() )
    {
        if ( fid >= fidFixedLeast )
        {
            const BOOL      fTemplateColumn     = FFixedTemplateColumn( fid );
            const COLUMNID  columnid            = ColumnidOfFid( fid, fTemplateColumn );
            const FIELD *   pfield              = PfieldFixed( columnid );
            ib = pfield->ibRecordOffset + USHORT(  pfield->cbMaxLen );
        }
        else
        {
            ib = ibRECStartFixedColumns;
        }
    }
#endif
    else
    {
        const FID       fidT                = FID( fid + 1 );
        const BOOL      fTemplateColumn     = FFixedTemplateColumn( fidT );
        const COLUMNID  columnidT           = ColumnidOfFid( fidT, fTemplateColumn );
        ib = PfieldFixed( columnidT )->ibRecordOffset;
    }

#ifdef DEBUG
    if ( FidFixedLast() == fid || FInitialisingDefaultRecord() )
    {
        Assert( ib <= IbEndFixedColumns() );

        if ( fid >= fidFixedLeast )
        {
            const COLUMNID  columnid    = ColumnidOfFid( fid, FFixedTemplateColumn( fid ) );
            Assert( ib > ibRECStartFixedColumns );
            Assert( PfieldFixed( columnid )->cbMaxLen > 0 );
            Assert( ib == PfieldFixed( columnid )->ibRecordOffset + PfieldFixed( columnid )->cbMaxLen );
        }
        else
        {
            Assert( ibRECStartFixedColumns == ib );
        }
    }
    else
    {
        Assert( ib < IbEndFixedColumns() );

        if ( fid >= fidFixedLeast )
        {
            const COLUMNID  columnid    = ColumnidOfFid( fid, FFixedTemplateColumn( fid ) );
            if ( !FFIELDCommittedDelete( PfieldFixed( columnid )->ffield ) )
            {
                Assert( PfieldFixed( columnid )->cbMaxLen > 0 );
                Assert( ib == PfieldFixed( columnid )->ibRecordOffset + PfieldFixed( columnid )->cbMaxLen );
            }
            Assert( ib > ibRECStartFixedColumns );
        }
        else
        {
            Assert( ibRECStartFixedColumns == ib );
        }
    }
#endif

    return ib;
}


INLINE ULONG TDB::CInitialFixedColumns() const
{
    return ( FidFixedLastInitial() + 1 - FidFixedFirst() );
}
INLINE ULONG TDB::CInitialVarColumns() const
{
    return ( FidVarLastInitial() + 1 - FidVarFirst() );
}
INLINE ULONG TDB::CInitialTaggedColumns() const
{
    return ( FidTaggedLastInitial() + 1 - FidTaggedFirst() );
}
INLINE ULONG TDB::CInitialColumns() const
{
    return ( CInitialFixedColumns() + CInitialVarColumns() + CInitialTaggedColumns() );
}

INLINE ULONG TDB::CDynamicFixedColumns() const
{
    return ( FidFixedLast() - FidFixedLastInitial() );
}
INLINE ULONG TDB::CDynamicVarColumns() const
{
    return ( FidVarLast() - FidVarLastInitial() );
}
INLINE ULONG TDB::CDynamicTaggedColumns() const
{
    return ( FidTaggedLast() - FidTaggedLastInitial() );
}
INLINE ULONG TDB::CDynamicColumns() const
{
    return ( CDynamicFixedColumns() + CDynamicVarColumns() + CDynamicTaggedColumns() );
}

INLINE ULONG TDB::CFixedColumns() const
{
    return ( FidFixedLast() + 1 - FidFixedFirst() );
}
INLINE ULONG TDB::CVarColumns() const
{
    return ( FidVarLast() + 1 - FidVarFirst() );
}
INLINE ULONG TDB::CTaggedColumns() const
{
    return ( FidTaggedLast() + 1 - FidTaggedFirst() );
}
INLINE ULONG TDB::CColumns() const
{
    return ( CFixedColumns() + CVarColumns() + CTaggedColumns() );
}


INLINE BOOL TDB::FFixedTemplateColumn( const FID fid ) const
{
    Assert( FFixedFid( fid ) );
    Assert( fid <= FidFixedLast() );
    if ( fid < FidFixedFirst() )
        AssertValidDerivedTable();
    return ( FTemplateTable() || fid < FidFixedFirst() );
}
INLINE BOOL TDB::FVarTemplateColumn( const FID fid ) const
{
    Assert( FVarFid( fid ) );
    Assert( fid <= FidVarLast() );
    if ( fid < FidVarFirst() )
        AssertValidDerivedTable();
    return ( FTemplateTable() || fid < FidVarFirst() );
}


INLINE CReaderWriterLock& TDB::RwlDDL()
{
    return m_rwlDDL;
}

INLINE CBinaryLock& TDB::BlIndexes()
{
    return m_blIndexes;
}
INLINE VOID TDB::EnterUpdating()
{
    BlIndexes().Enter1();
}
INLINE VOID TDB::LeaveUpdating()
{
    BlIndexes().Leave1();
}
INLINE VOID TDB::EnterIndexing()
{
    BlIndexes().Enter2();
}
INLINE VOID TDB::LeaveIndexing()
{
    BlIndexes().Leave2();
}
INLINE VOID TDB::SetFLid64( BOOL fLid64 )
{
    if ( m_fLid64 == fTrue && fLid64 == fFalse )
    {
        EnforceSz( Ui64LidLast() == 0, "DataCorruptionSwitchFromLID64toLID32" );
    }

    m_fLid64 = fLid64;
}

INLINE VOID FCB::ResetUpdating_()
{
    Assert( FTypeTable() );
    Assert( !FTemplateTable() );
    Assert( !FFixedDDL() );
    Assert( Ptdb() != ptdbNil );

    Ptdb()->LeaveUpdating();
}
INLINE VOID FCB::ResetUpdatingAndLeaveDML()
{
    if ( !FFixedDDL() )
    {
        LeaveDML_();
        ResetUpdating_();
    }
}
INLINE VOID FCB::ResetUpdating()
{
    if ( !FFixedDDL() )
    {
        ResetUpdating_();
    }
}


INLINE VOID FCB::SetIndexing()
{
    Assert( FTypeTable() );
    Assert( Ptdb() != ptdbNil );

    Assert( !FFixedDDL() || CrefDomainDenyRead() > 0 );

    Ptdb()->EnterIndexing();
}


INLINE VOID FCB::ResetIndexing()
{
    Assert( FTypeTable() );
    Assert( Ptdb() != ptdbNil );

    Assert( !FFixedDDL() || CrefDomainDenyRead() > 0 );

    Ptdb()->LeaveIndexing();
}

INLINE VOID FCB::EnterDML_()
{
    Ptdb()->RwlDDL().EnterAsReader();
    AssertDML_();
}

INLINE VOID FCB::LeaveDML_()
{
    AssertDML_();
    Ptdb()->RwlDDL().LeaveAsReader();
}

INLINE VOID FCB::AssertDML_() const
{
    Assert( Ptdb()->RwlDDL().FReader() );
}

INLINE VOID FCB::EnterDDL_()
{
    Ptdb()->RwlDDL().EnterAsWriter();
    AssertDDL_();
}

INLINE VOID FCB::LeaveDDL_()
{
    AssertDDL_();
    Ptdb()->RwlDDL().LeaveAsWriter();
}

INLINE VOID FCB::AssertDDL_() const
{
    Assert( Ptdb()->RwlDDL().FWriter() );
}

