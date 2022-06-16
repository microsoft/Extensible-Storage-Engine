// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  describes exact placement relative to logical currency in bookmark
//  used only at DIR level
//
enum  LOC
    {   locOnCurBM,             // cursor is on bookmark
        locBeforeSeekBM,        // cursor is before bookmark [page should be latched]
        locAfterSeekBM,         // cursor is after bookmark [page should be latched]
        locOnSeekBM,            // cursor is on seek bookmark [virtual]
        locAfterLast,           // after last node in tree
        locBeforeFirst,         // before first node in tree
        locOnFDPRoot,           // used to extract prefix headers
        locDeferMoveFirst       // on first node
};


//  record modification copy buffer status
//
typedef BYTE    CBSTAT;

const CBSTAT    fCBSTATNull                                 = 0;
const CBSTAT    fCBSTATInsert                               = 0x01;
const CBSTAT    fCBSTATInsertCopy                           = 0x02;
const CBSTAT    fCBSTATReplace                              = 0x04;
const CBSTAT    fCBSTATLock                                 = 0x08;
const CBSTAT    fCBSTATInsertCopyDeleteOriginal             = 0x10;
const CBSTAT    fCBSTATUpdateForInsertCopyDeleteOriginal    = 0x20;
const CBSTAT    fCBSTATInsertReadOnlyCopy                   = 0x40;
const CBSTAT    fCBSTATMustSetAutoInc                       = 0x80;

//  describes structure used to hold before-images of LVs
//
//
struct LVBUF
{
    LONG    lid;
    BYTE    *pbLV;
    LONG    cbLVSize;
    LVBUF   *plvbufNext;
};


//  describes status of SearchKey buffer
//
typedef BYTE    KEYSTAT;
const KEYSTAT   keystatNull             = 0;
const KEYSTAT   keystatPrepared         = 0x01;
const KEYSTAT   keystatTooBig           = 0x02;
const KEYSTAT   keystatLimit            = 0x04;

//  size of FUCB-local buffer for bookmark
//
const UINT cbBMCache                    = 36;

struct MOVE_FILTER_CONTEXT;

//  Move filter callback
//
//  Returns:
//  -  JET_errSuccess:              Current entry is visible
//  -  wrnBTNotVisibleRejected:     Current entry is not visible because it has been rejected by a move filter
//  -  wrnBTNotVisibleAccumulated:  Current entry is not visible because it is being accumulated by a move filter
//
typedef ERR( *PFN_MOVE_FILTER )( FUCB * const pfucb, MOVE_FILTER_CONTEXT* const pmoveFilterContext );

struct MOVE_FILTER_CONTEXT
{
    MOVE_FILTER_CONTEXT*    pmoveFilterContextPrev; //  higher precedence move filter context (apply first)
    PFN_MOVE_FILTER         pfnMoveFilter;          //  move filter function to apply during lateral btree navigation
};

//  File Use Control Block
//  Used for Table, Temp table and Sort handles
//
struct FUCB
    :   public CZeroInit
{
#ifdef AMD64
    private:
        static VOID VerifyOptimalPacking();    // Just a bunch of static_asserts to verify optimality.
#endif
    public:
        FUCB( PIB* const ppibIn, const IFMP ifmpIn );
        ~FUCB();

        static SIZE_T OffsetOfIAE() { return OffsetOf(FUCB, m_iae); }

#ifdef DEBUG
        VOID AssertValid() const;
#endif

#pragma push_macro( "new" )
#undef new
    private:
        void* operator new( size_t );           //  meaningless without INST*
        void* operator new[]( size_t );         //  meaningless without INST*
        void operator delete[]( void* );        //  not supported
    public:
        void* operator new( size_t cbAlloc, INST* pinst )
        {
            return pinst->m_cresFUCB.PvRESAlloc_( SzNewFile(), UlNewLine() );
        }
        void operator delete( void* pv )
        {
            if ( pv )
            {
                FUCB* pfucb = reinterpret_cast< FUCB* >( pv );
                PinstFromIfmp( pfucb->ifmp )->m_cresFUCB.Free( pv );
            }
        }
#pragma pop_macro( "new" )

    //  dispatch layer
    const VTFNDEF   *pvtfndef;              // dispatch table. Must be the first field.

    //  chaining fields
    //
    PIB             *ppib;                  // session that opened this FUCB
    FUCB            *pfucbNextOfSession;    // Next FUCB of this session

    union
    {
        FCB         *pfcb;                  // if fFUCBIndex
        SCB         *pscb;                  // if fFUCBSort
    } u;

    IFMP            ifmp;                   // database id

    //  flags for FUCB
    //
    union {
        ULONG       ulFlags;
        struct {
            //  used by DIR
            //
            UINT    fIndex:1;               // FUCB is for index
            UINT    fSecondary:1;           // FUCB for secondary index
            UINT    fCurrentSecondary:1;    // FUCB is for secondary index opened by user (ie. SetCurrentIndex)
            UINT    fLV:1;                  // FUCB is for long-value tree

            //  used by SORT & Temp Tables
            //
            UINT    fSort:1;                // FUCB is for sort

            //  used only by IsamCloseTable
            //
            UINT    fSystemTable:1;         // system table cursor

            //  CONSIDER: should be set by FUCBOpen only
            UINT    fWrite:1;               // cursor can write

            //  used in rollback, commit & purge to reset domain-level locks
            //
            UINT    fDenyRead:1;            // deny read flag
            UINT    fDenyWrite:1;           // deny write flag

            //  used by Add/DeleteColumn and Create/DeleteIndex to override FCB's
            //  FCB's FixedDDL flag at CreateTable time.
            UINT    fPermitDDL:1;

            //  fFUCBDeferClose is set by cursor DIRClose and reset by ErrDIROpen
            //
            UINT    fDeferClose:1;          // FUCB is awaiting close
            UINT    fVersioned:1;           // FUCB made versions

            //  used by DIR, SORT, REC
            //
            UINT    fLimstat:1;             // range limit

            //  used only when index range is set [fLimstat]
            //
            UINT    fInclusive:1;           // inclusive range
            UINT    fUpper:1;               // upper range limit

            //  used by LV on table cursor to determine if LV cursor may be cached in pfucbLV
            //
            UINT    fMayCacheLVCursor:1;

            //  used for tracking updates to separated LV's in a Setcolumn
            //  used to obtain the before image of an LV change
            //  [only the part that affects an index - first 255 bytes
            //  lives till commit of transaction or corresponding RECReplace/Insert
            //
            UINT    fUpdateSeparateLV:1;    // long value updated


            //  has any column been set during the current record update?
            //
            UINT    fAnyColumnSet:1;

            //  checksum comparison is deferred for PrepareUpdate w/ noLock
            //  within a transaction
            //  checksum is deferred till commit of transaction
            //  or corresponding RECReplace/Insert
            //
            UINT    fDeferredChecksum:1;    // checksum calculation deferred.

            //  set and used by Space
            //
            UINT    fAvailExt   : 1;
            UINT    fOwnExt     : 1;

            //  used by BTree for preread
            //
            UINT    fSequential     : 1;    //  this cursor will traverse sequentially
            UINT    fPreread        : 1;    //  we are currently reading ahead
            UINT    fPrereadBackward: 1;    //  prereading backward in the tree
            UINT    fPrereadForward: 1; //  prereading forward in the tree
            UINT    fOpportuneRead: 1;      // attempt to read leaf pages using larger physical IOs
            
            //  has bookmark already been saved for this currency?
            UINT    fBookmarkPreviouslySaved: 1;

            //  If we need to touch the data page buffers of the saved bookmark
            UINT    fTouch      : 1;

            //  Is this the FUCB for a table that is being repaired?
            UINT    fRepair     : 1;

            //  User-defined-defaults: if this flag is set we will always/never act as though JET_bitRetrieveCopy was
            //  set for column retrieval. Needed for index updates
            UINT    fAlwaysRetrieveCopy : 1;
            UINT    fNeverRetrieveCopy : 1;

            //  Used to know when we implicitlty modify tagged columns without changing any column value.
            //  Bursting existing LV in record to free record space for example
            //  Usefull in logdiff
            UINT    fTagImplicitOp:1;

            //  ********************************************
            //  WARNING! WARNING! WARNING! WARNING! WARNING!
            //  ********************************************
            //  32 bits of this bitfield have now been used
            //  up. If more bitfields are required, use the
            //  usFlags variable below
            //  ********************************************
        };
    };

    //  physical currency
    //
    CSR             csr;
    KEYDATAFLAGS    kdfCurr;                // key-data-flags of current node

    //  logical currency
    //
    VOID            *pvBMBuffer;            // allocated bookmark buffer
    BOOKMARK        bmCurr;                 // bookmark of current location (could be a "virtual" location)
    BYTE            rgbBMCache[cbBMCache];  // local buffer for bookmark

    LOC             locLogical;             // logical position relative to BM

    FUCB            *pfucbCurIndex;         // current secondary index
    FUCB            *pfucbLV;               // cached LV cursor

    LONG            ispairCurr;             // (SORT) current record

    //  search key
    KEYSTAT         keystat;                //  search key buffer status
    BYTE            cColumnsInSearchKey;

    union
    {
        USHORT      usFlags;
        struct
        {
            //  for secondary indexes only, are we "borrowing"
            //  the table's search key buffer?
            //  WARNING: don't reset this flag when the
            //  cursor gets defer-closed because the
            //  key buffer is not freed on a defer-close
            USHORT  fUsingTableSearchKeyBuffer:1;

            //  During recovery, is this FUCB created by recovery
            //  (and hence in the CTableHash)?
            USHORT  fInRecoveryTableHash:1;
        };
    };

    DATA            dataSearchKey;          //  search key

    //  transactions
    //
    RCEID           rceidBeginUpdate;
    UPDATEID        updateid;
    ULONG           ulChecksum;             //  checksum of record -- used only for optimistic locking

    LEVEL           levelOpen;              //  level at open of FUCB
    LEVEL           levelNavigate;          //  level of navigation
    LEVEL           levelPrep;              //  level copy buffer prepared
    LEVEL           levelReuse;             //  level at reuse of FUCB
    CBSTAT          cbstat;                 //  copy buffer status
    BYTE            rgbAlign[3];            //  Padding.

    //  preread
    //
    CPG             cpgPreread;             //  number of pages to preread in BTDown
    CPG             cpgPrereadNotConsumed;  //  number of preread pages we have not consumed
    LONG            cbSequentialDataRead;   //  number of bytes we have retrieved in one direction

    FUCB            *pfucbTable;            //  for LV/index cursors only, cursor of
                                            //  owning table (used by rollback only to unlink LV cursor)
    //  maintained by record manager
    //
    DATA            dataWorkBuf;            //  working buffer for Insert/Replace
    VOID            *pvWorkBuf;             //  working buffer for Insert/Replace
    VOID            *pvRCEBuffer;           //  allocated RCE before-image buffer

    BYTE            rgbitSet[32];           //  bits to track column change

    //  record position of last seek
    //
    ULONGLONG       ullLTLast;
    ULONGLONG       ullTotalLast;

    //  record position of saved bookmark
    //
    ULONGLONG       ullLTCurr;
    ULONGLONG       ullTotalCurr;

    //  used only by space manager
    //
    CSR             *pcsrRoot;              //  used by BT to pass cursor on root page

    LSTORE          ls;                     //  local storage

    FUCB            *pfucbHashOverflow;     //  hash overflow (used only by recovery)

    DWORD           cbEncryptionKey;
    CPG             cpgSpaceRequestReserve; // Placed here to avoid 4 bytes of padding/wasted space.
    BYTE *          pbEncryptionKey;

    MOVE_FILTER_CONTEXT*    pmoveFilterContext; //  optional move filter context

    CInvasiveConcurrentModSet< FUCB, OffsetOfIAE>::CElement m_iae;

    //  set and used by Space
    //  Space tree use requires a latch on the FDP of the FDP related to the space tree.
    //  If this is a space tree, this holds a pointer to the FUCB that has that latch.
    //  It's not always maintained, but is accurate if it's non NULL.
    // 
    FUCB           *pfucbLatchHolderForSpace;

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;

    const WCHAR * WszFUCBType() const;
#endif  //  DEBUGGER_EXTENSION
};

// FCB.HXX needs access to FUCB::OffsetOfIAE (a static method of the FUCB struct)
// to make a CInvasiveThreadsafeArray of FUCBs. However, FUCB.HXX is not usually
// included until after FCB.HXX.  So, this global function is used to wrap
// FUCB::OffsetOfIAE, and the function is forward-declared in FCB.HXX.
inline SIZE_T FUCBOffsetOfIAE()
{
    return FUCB::OffsetOfIAE();
}

#define PinstFromPfucb( pfucb ) ( PinstFromPpib( (pfucb)->ppib ) )

#ifdef AMD64

INLINE VOID FUCB::VerifyOptimalPacking()
{
    // Verify optimal packing.  These matter because keeping this structure to a 32 byte boundary
    // makes it significantly more memory efficient when the resource manager allocates it.
    // We currently "waste" 12 bytes in rgbAlign[].
    //
    // No one ever calls this routine.  It's enough that it compiles cleanly for the static_asserts.
    //
    // Note: use a field to take up padding inside the structure.  See rgbReserved.
    //
    // Also note that we keep track of 64 byte boundaries as a cache line mark.
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

    static_assert( sizeof( FUCB ) == 512, "Current size" );

    static_assert( NoWastedSpaceAround( FUCB, pvtfndef, pfucbLatchHolderForSpace ) );
    static_assert( CacheLineMark( FUCB, pvtfndef, 0 ) );
    static_assert( NoWastedSpace( FUCB, pvtfndef,              ppib) );
    static_assert( NoWastedSpace( FUCB, ppib,                  pfucbNextOfSession) );
    static_assert( NoWastedSpace( FUCB, pfucbNextOfSession,    u) );
    static_assert( NoWastedSpace( FUCB, u,                     ifmp) );
    static_assert( NoWastedSpace( FUCB, ifmp,                  ulFlags) );
    static_assert( NoWastedSpace( FUCB, ulFlags,               csr) );

    static_assert( ( ( OffsetOf(FUCB, csr) % 64 ) == 40),      "Cache line boundary 1 occurs in the middle of csr" );
    static_assert( NoWastedSpace( FUCB, csr,                   kdfCurr) );

    static_assert( ( ( OffsetOf(FUCB, kdfCurr) % 64 ) == 56),  "Cache line boundary 2 occurs in the middle of kdfCurr" );
    static_assert( NoWastedSpace( FUCB, kdfCurr,               pvBMBuffer) );
    static_assert( NoWastedSpace( FUCB, pvBMBuffer,            bmCurr) );

    static_assert( ( ( OffsetOf(FUCB, bmCurr) % 64 ) == 56),   "Cache line boundary 3 occurs in the middle of bmCurr" );
    static_assert( NoWastedSpace( FUCB, bmCurr,                rgbBMCache) );

    static_assert( ( ( OffsetOf(FUCB, rgbBMCache) % 64 ) == 40), "Cache line boundary 4 occurs in the middle of rgbBMCache" );
    static_assert( NoWastedSpace( FUCB, rgbBMCache,            locLogical) );
    static_assert( NoWastedSpace( FUCB, locLogical,            pfucbCurIndex) );
    static_assert( NoWastedSpace( FUCB, pfucbCurIndex,         pfucbLV) );
    static_assert( NoWastedSpace( FUCB, pfucbLV,               ispairCurr) );
    static_assert( NoWastedSpace( FUCB, ispairCurr,            keystat) );
    static_assert( NoWastedSpace( FUCB, keystat,               cColumnsInSearchKey) );
    static_assert( NoWastedSpace( FUCB, cColumnsInSearchKey,   usFlags) );
    static_assert( NoWastedSpace( FUCB, usFlags,               dataSearchKey) );
    static_assert( NoWastedSpace( FUCB, dataSearchKey,         rceidBeginUpdate) );
    static_assert( NoWastedSpace( FUCB, rceidBeginUpdate,      updateid) );
    static_assert( NoWastedSpace( FUCB, updateid,              ulChecksum) );

    static_assert( CacheLineMark( FUCB, ulChecksum, 5 ) );
    static_assert( NoWastedSpace( FUCB, ulChecksum,            levelOpen) );
    static_assert( NoWastedSpace( FUCB, levelOpen,             levelNavigate) );
    static_assert( NoWastedSpace( FUCB, levelNavigate,         levelPrep) );
    static_assert( NoWastedSpace( FUCB, levelPrep,             levelReuse) );
    static_assert( NoWastedSpace( FUCB, levelReuse,            cbstat) );
    static_assert( NoWastedSpace( FUCB, cbstat,                rgbAlign) );
    static_assert( NoWastedSpace( FUCB, rgbAlign,              cpgPreread) );
    static_assert( NoWastedSpace( FUCB, cpgPreread,            cpgPrereadNotConsumed) );
    static_assert( NoWastedSpace( FUCB, cpgPrereadNotConsumed, cbSequentialDataRead) );
    static_assert( NoWastedSpace( FUCB, cbSequentialDataRead,  pfucbTable) );
    static_assert( NoWastedSpace( FUCB, pfucbTable,            dataWorkBuf) );
    static_assert( NoWastedSpace( FUCB, dataWorkBuf,           pvWorkBuf) );
    static_assert( NoWastedSpace( FUCB, pvWorkBuf,             pvRCEBuffer) );
    static_assert( NoWastedSpace( FUCB, pvRCEBuffer,           rgbitSet) );

    static_assert( CacheLineMark( FUCB, rgbitSet, 6 ) );
    static_assert( NoWastedSpace( FUCB, rgbitSet,              ullLTLast) );
    static_assert( NoWastedSpace( FUCB, ullLTLast,             ullTotalLast) );
    static_assert( NoWastedSpace( FUCB, ullTotalLast,          ullLTCurr) );
    static_assert( NoWastedSpace( FUCB, ullLTCurr,             ullTotalCurr) );
    static_assert( NoWastedSpace( FUCB, ullTotalCurr,          pcsrRoot) );

    static_assert( CacheLineMark( FUCB, pcsrRoot, 7) );
    static_assert( NoWastedSpace( FUCB, pcsrRoot,              ls) );
    static_assert( NoWastedSpace( FUCB, ls,                    pfucbHashOverflow) );
    static_assert( NoWastedSpace( FUCB, pfucbHashOverflow,     cbEncryptionKey) );
    static_assert( NoWastedSpace( FUCB, cbEncryptionKey,       cpgSpaceRequestReserve) );
    static_assert( NoWastedSpace( FUCB, cpgSpaceRequestReserve,pbEncryptionKey) );
    static_assert( NoWastedSpace( FUCB, pbEncryptionKey,       pmoveFilterContext) );
    static_assert( NoWastedSpace( FUCB, pmoveFilterContext,    m_iae) );
    static_assert( NoWastedSpace( FUCB, m_iae,                 pfucbLatchHolderForSpace) );
}
#endif

//
//  Resource management
//

INLINE VOID FUCBTerm( INST *pinst )
{
    //  term the FUCB resource manager
    //
    pinst->m_cresFUCB.Term();
    if ( pinst->FRecovering() )
    {
        CallS( ErrRESSetResourceParam( pinst, JET_residFUCB, JET_resoperEnableMaxUse, fTrue ) );
    }
}

INLINE ERR ErrFUCBInit( INST *pinst )
{
    ERR err = JET_errSuccess;

    //  init the FUCB resource manager
    //
    //  NOTE:  if we are recovering, then disable quotas
    //
    if ( pinst->FRecovering() )
    {
        CallS( ErrRESSetResourceParam( pinst, JET_residFUCB, JET_resoperEnableMaxUse, fFalse ) );
    }
    Call( pinst->m_cresFUCB.ErrInit( JET_residFUCB ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        FUCBTerm( pinst );
    }
    return err;
}


//  Flag managements
//

INLINE VOID FUCBResetFlags( FUCB *pfucb )
{
    pfucb->ulFlags = 0;
}

INLINE BOOL FFUCBSpace( const FUCB *pfucb )
{
    Assert( !( pfucb->fAvailExt && pfucb->fOwnExt ) );
    return pfucb->fAvailExt || pfucb->fOwnExt;
}

//  does this tree allow duplicates?
//
INLINE BOOL FFUCBUnique( const FUCB *pfucb )
{
    Assert( pfcbNil != pfucb->u.pfcb );

    const BOOL  fUnique = ( pfucb->u.pfcb->FUnique() || FFUCBSpace( pfucb ) );

#ifdef DEBUG
    if ( !PinstFromIfmp( pfucb->ifmp )->FRecovering() )
    {
        BOOL fLocked = fFalse;
        FCB *pfcb   = pfucb->u.pfcb;
        FCB *pfcbTable = pfcb->PfcbTable();
        if ( pfcb->FTypeSecondaryIndex() && pfcbTable != pfcbNil )
        {
            pfcbTable->EnterDML();
            fLocked = fTrue;
        }
        const IDB   *pidb   = pfcb->Pidb();

        if ( fUnique != ( FFUCBSpace( pfucb )
                            || pfcb->FPrimaryIndex()
                            || pidbNil == pidb
                            || pidb->FUnique() ) )
        {
            //  only time uniqueness check should fail is if we're in the middle of
            //  rolling back a CreateIndex and have lost the IDB, but have yet to
            //  nullify all the versions on the FCB, and someone else is accessing
            //  one of those versions
            Assert( pfcb->FTypeSecondaryIndex() );
            Assert( pidbNil == pfcb->Pidb() );
            Assert( !pfcb->FDeletePending() );
            Assert( !pfcb->FDeleteCommitted() );
        }
        if ( fLocked )
        {
            pfcbTable->LeaveDML();
        }
    }
#endif

    return fUnique;
}

//
//  the following flags need to be prevent reuse of cursor
//  after deferred closed.  This is done to correctly release
//  domain flags when commit/rollback to transaction level 0.
//
INLINE BOOL FFUCBNotReuse( const FUCB *pfucb )
{
    return ( pfucb->fDenyRead || pfucb->fDenyWrite );
}

INLINE BOOL FFUCBIndex( const FUCB *pfucb )
{
    return pfucb->fIndex;
}

INLINE VOID FUCBSetIndex( FUCB *pfucb )
{
    pfucb->fIndex = 1;
}

INLINE VOID FUCBResetIndex( FUCB *pfucb )
{
    pfucb->fIndex = 0;
}

INLINE BOOL FFUCBAvailExt( const FUCB *pfucb )
{
    return pfucb->fAvailExt;
}

INLINE VOID FUCBSetAvailExt( FUCB *pfucb )
{
    pfucb->fAvailExt = 1;
}

INLINE VOID FUCBResetAvailExt( FUCB *pfucb )
{
    pfucb->fAvailExt = 0;
}

INLINE BOOL FFUCBOwnExt( const FUCB *pfucb )
{
    return pfucb->fOwnExt;
}

INLINE VOID FUCBSetOwnExt( FUCB *pfucb )
{
    pfucb->fOwnExt = 1;
}

INLINE VOID FUCBResetOwnExt( FUCB *pfucb )
{
    pfucb->fOwnExt = 0;
}

INLINE BOOL FFUCBLongValue( FUCB * const pfucb )
{
    return pfucb->fLV;
}

INLINE VOID FUCBSetLongValue( FUCB * const pfucb )
{
    pfucb->fLV = 1;
}

INLINE VOID FUCBResetLongValue( FUCB * const pfucb )
{
    pfucb->fLV = 0;
}

INLINE BOOL FFUCBSecondary( const FUCB *pfucb )
{
    return pfucb->fSecondary;
}

INLINE BOOL FFUCBPrimary( FUCB *pfucb )
{
    return !FFUCBSecondary( pfucb );
}

INLINE VOID FUCBSetSecondary( FUCB *pfucb )
{
    pfucb->fSecondary = 1;
}

INLINE VOID FUCBResetSecondary( FUCB *pfucb )
{
    pfucb->fSecondary = 0;
}

INLINE BOOL FFUCBCurrentSecondary( const FUCB *pfucb )
{
    return pfucb->fCurrentSecondary;
}

INLINE VOID FUCBSetCurrentSecondary( FUCB *pfucb )
{
    pfucb->fCurrentSecondary = 1;
}

INLINE VOID FUCBResetCurrentSecondary( FUCB *pfucb )
{
    pfucb->fCurrentSecondary = 0;
}

INLINE BOOL FFUCBTagImplicitOp( const FUCB *pfucb )
{
    return pfucb->fTagImplicitOp;
}

INLINE VOID FUCBSetTagImplicitOp( FUCB *pfucb )
{
    pfucb->fTagImplicitOp = 1;
}

INLINE VOID FUCBResetTagImplicitOp( FUCB *pfucb )
{
    pfucb->fTagImplicitOp = 0;
}

//  ================================================================
INLINE BOOL FFUCBPreread( const FUCB* pfucb )
//  ================================================================
{
    Assert( !(pfucb->fPrereadForward && pfucb->fPrereadBackward) );
    return pfucb->fPreread;
}


//  ================================================================
INLINE BOOL FFUCBPrereadForward( const FUCB* pfucb )
//  ================================================================
{
    Assert( !(pfucb->fPrereadForward && pfucb->fPrereadBackward) );
    return pfucb->fPrereadForward;
}


//  ================================================================
INLINE BOOL FFUCBPrereadBackward( const FUCB* pfucb )
//  ================================================================
{
    Assert( !(pfucb->fPrereadForward && pfucb->fPrereadBackward) );
    return pfucb->fPrereadBackward;
}


//  ================================================================
INLINE VOID FUCBSetPrereadForward( FUCB *pfucb, CPG cpgPreread )
//  ================================================================
{
    pfucb->fPreread                 = fTrue;
    pfucb->fPrereadForward          = fTrue;
    pfucb->fPrereadBackward         = fFalse;
    pfucb->cpgPreread               = cpgPreread;
}


//  ================================================================
INLINE VOID FUCBSetPrereadBackward( FUCB *pfucb, CPG cpgPreread )
//  ================================================================
{
    pfucb->fPreread                 = fTrue;
    pfucb->fPrereadForward          = fFalse;
    pfucb->fPrereadBackward         = fTrue;
    pfucb->cpgPreread               = cpgPreread;
}

//  ================================================================
INLINE VOID FUCBResetPreread( FUCB *pfucb )
//  ================================================================
{
    pfucb->fPreread                 = fFalse;
    pfucb->fPrereadForward          = fFalse;
    pfucb->fPrereadBackward         = fFalse;
    pfucb->cpgPreread               = 0;
    pfucb->cpgPrereadNotConsumed    = 0;
    pfucb->cbSequentialDataRead     = 0;
}

//  ================================================================
INLINE BOOL FFUCBOpportuneRead( FUCB* pfucb )
//  ================================================================
{
    return pfucb->fOpportuneRead;
}

//  ================================================================
INLINE VOID FUCBSetOpportuneRead( FUCB* pfucb )
//  ================================================================
{
    pfucb->fOpportuneRead = fTrue;
}

//  ================================================================
INLINE VOID FUCBResetOpportuneRead( FUCB* pfucb )
//  ================================================================
{
    pfucb->fOpportuneRead = fFalse;
}

//  ================================================================
INLINE BOOL FFUCBRepair( const FUCB *pfucb )
//  ================================================================
{
    return pfucb->fRepair;
}

//  ================================================================
INLINE VOID FUCBSetRepair( FUCB *pfucb )
//  ================================================================
{
    pfucb->fRepair = 1;
}

//  ================================================================
INLINE VOID FUCBResetRepair( FUCB *pfucb )
//  ================================================================
{
    pfucb->fRepair = 0;
}


INLINE BOOL FFUCBSort( const FUCB *pfucb )
{
    return pfucb->fSort;
}

INLINE VOID FUCBSetSort( FUCB *pfucb )
{
    pfucb->fSort = 1;
}

INLINE VOID FUCBResetSort( FUCB *pfucb )
{
    pfucb->fSort = 0;
}

INLINE BOOL FFUCBSystemTable( const FUCB *pfucb )
{
    return pfucb->fSystemTable;
}

INLINE VOID FUCBSetSystemTable( FUCB *pfucb )
{
    pfucb->fSystemTable = 1;
}

INLINE VOID FUCBResetSystemTable( FUCB *pfucb )
{
    pfucb->fSystemTable = 0;
}

INLINE BOOL FFUCBUpdatable( const FUCB *pfucb )
{
    return pfucb->fWrite;
}

INLINE VOID FUCBSetUpdatable( FUCB *pfucb )
{
    pfucb->fWrite = 1;
}

INLINE VOID FUCBResetUpdatable( FUCB *pfucb )
{
    pfucb->fWrite = 0;
}

INLINE BOOL FFUCBDenyWrite( const FUCB *pfucb )
{
    return pfucb->fDenyWrite;
}

INLINE VOID FUCBSetDenyWrite( FUCB *pfucb )
{
    pfucb->fDenyWrite = 1;
}

INLINE VOID FUCBResetDenyWrite( FUCB *pfucb )
{
    pfucb->fDenyWrite = 0;
}

INLINE BOOL FFUCBDenyRead( const FUCB *pfucb )
{
    return pfucb->fDenyRead;
}

INLINE VOID FUCBSetDenyRead( FUCB *pfucb )
{
    pfucb->fDenyRead = 1;
}

INLINE VOID FUCBResetDenyRead( FUCB *pfucb )
{
    pfucb->fDenyRead = 0;
}

INLINE BOOL FFUCBPermitDDL( const FUCB *pfucb )
{
    return pfucb->fPermitDDL;
}

INLINE VOID FUCBSetPermitDDL( FUCB *pfucb )
{
    pfucb->fPermitDDL = fTrue;
}

INLINE VOID FUCBResetPermitDDL( FUCB *pfucb )
{
    pfucb->fPermitDDL = fFalse;
}

INLINE BOOL FFUCBDeferClosed( const FUCB *pfucb )
{
    return pfucb->fDeferClose;
}

INLINE VOID FUCBSetDeferClose( FUCB *pfucb )
{
    Assert( (pfucb)->ppib->Level() > 0 );
    pfucb->fDeferClose = 1;

    // When we are defering a close for reuse, we need
    // clear the reuse level as it may now be reused but
    // at this point it is effectively "closed".
    pfucb->levelReuse = 0;
}

INLINE VOID FUCBResetDeferClose( FUCB *pfucb )
{
    pfucb->fDeferClose = 0;
}

INLINE BOOL FFUCBLimstat( const FUCB *pfucb )
{
    return pfucb->fLimstat;
}

INLINE VOID FUCBSetLimstat( FUCB *pfucb )
{
    pfucb->fLimstat = 1;
}

INLINE VOID FUCBResetLimstat( FUCB *pfucb )
{
    pfucb->fLimstat = 0;
}

INLINE BOOL FFUCBInclusive( const FUCB *pfucb )
{
    return pfucb->fInclusive;
}

INLINE VOID FUCBSetInclusive( FUCB *pfucb )
{
    pfucb->fInclusive = 1;
}

INLINE VOID FUCBResetInclusive( FUCB *pfucb )
{
    pfucb->fInclusive = 0;
}

INLINE BOOL FFUCBUpper( const FUCB *pfucb )
{
    return pfucb->fUpper;
}

INLINE VOID FUCBSetUpper( FUCB *pfucb )
{
    pfucb->fUpper = 1;
}

INLINE VOID FUCBResetUpper( FUCB *pfucb )
{
    pfucb->fUpper = 0;
}

INLINE BOOL FFUCBMayCacheLVCursor( const FUCB *pfucb )
{
    return pfucb->fMayCacheLVCursor;
}

INLINE VOID FUCBSetMayCacheLVCursor( FUCB *pfucb )
{
    pfucb->fMayCacheLVCursor = 1;
}

INLINE VOID FUCBResetMayCacheLVCursor( FUCB *pfucb )
{
    pfucb->fMayCacheLVCursor = 0;
}

INLINE BOOL FFUCBUpdateSeparateLV( const FUCB *pfucb )
{
    return pfucb->fUpdateSeparateLV;
}

INLINE VOID FUCBSetUpdateSeparateLV( FUCB *pfucb )
{
    Assert( 0 != pfucb->cbstat );
    pfucb->fUpdateSeparateLV = 1;
}

INLINE VOID FUCBResetUpdateSeparateLV( FUCB *pfucb )
{
    pfucb->fUpdateSeparateLV = 0;
}

INLINE BOOL FFUCBVersioned( const FUCB *pfucb )
{
    return pfucb->fVersioned;
}

INLINE VOID FUCBSetVersioned( FUCB *pfucb )
{
    Assert( !FFUCBSpace( pfucb ) );
    pfucb->fVersioned = 1;
}

INLINE VOID FUCBResetVersioned( FUCB *pfucb )
{
    Assert( 0 == pfucb->ppib->Level() );        //  can only reset once we've committed/rolled back to level 0
    pfucb->fVersioned = 0;
}

INLINE BOOL FFUCBDeferredChecksum( const FUCB *pfucb )
{
    return pfucb->fDeferredChecksum;
}

INLINE VOID FUCBSetDeferredChecksum( FUCB *pfucb )
{
    pfucb->fDeferredChecksum = 1;
}

INLINE VOID FUCBResetDeferredChecksum( FUCB *pfucb )
{
    pfucb->fDeferredChecksum = 0;
}

INLINE BOOL FFUCBSequential( const FUCB *pfucb )
{
    return pfucb->fSequential;
}

INLINE VOID FUCBSetSequential( FUCB *pfucb )
{
    pfucb->fSequential  = 1;
}

INLINE VOID FUCBResetSequential( FUCB *pfucb )
{
    pfucb->fSequential = 0;
}

//  ================================================================
INLINE BOOL FFUCBAlwaysRetrieveCopy( const FUCB *pfucb )
//  ================================================================
{
    const BOOL fAlwaysRetrieveCopy = pfucb->fAlwaysRetrieveCopy;
    return fAlwaysRetrieveCopy;
}

//  ================================================================
INLINE VOID FUCBSetAlwaysRetrieveCopy( FUCB *pfucb )
//  ================================================================
{
    pfucb->fAlwaysRetrieveCopy = 1;
}

//  ================================================================
INLINE VOID FUCBResetAlwaysRetrieveCopy( FUCB *pfucb )
//  ================================================================
{
    pfucb->fAlwaysRetrieveCopy = 0;
}

//  ================================================================
INLINE BOOL FFUCBNeverRetrieveCopy( const FUCB *pfucb )
//  ================================================================
{
    const BOOL fNeverRetrieveCopy = pfucb->fNeverRetrieveCopy;
    return fNeverRetrieveCopy;
}

//  ================================================================
INLINE VOID FUCBSetNeverRetrieveCopy( FUCB *pfucb )
//  ================================================================
{
    pfucb->fNeverRetrieveCopy = 1;
}

//  ================================================================
INLINE VOID FUCBResetNeverRetrieveCopy( FUCB *pfucb )
//  ================================================================
{
    pfucb->fNeverRetrieveCopy = 0;
}

ULONG UlChecksum( VOID *pv, ULONG cb );

INLINE VOID StoreChecksum( FUCB *pfucb )
{
    pfucb->ulChecksum =
        UlChecksum( pfucb->kdfCurr.data.Pv(), pfucb->kdfCurr.data.Cb() );
}

INLINE VOID PrepareInsert( FUCB *pfucb )
{
    pfucb->updateid = AtomicExchangeAdd( &PinstFromPfucb( pfucb )->m_updateid, updateidIncrement );
    pfucb->cbstat = fCBSTATInsert;
    pfucb->levelPrep = pfucb->ppib->Level();
}

INLINE VOID SetPrepareMustSetAutoInc( FUCB *pfucb )
{
    Assert( pfucb->cbstat != fCBSTATNull );
    Expected( pfucb->cbstat == fCBSTATInsert );
    Assert( pfucb->levelPrep == pfucb->ppib->Level() );
    pfucb->cbstat |= fCBSTATMustSetAutoInc;
}

INLINE VOID PrepareInsertCopy( FUCB *pfucb )
{
    pfucb->updateid = AtomicExchangeAdd( &PinstFromPfucb( pfucb )->m_updateid, updateidIncrement );
    pfucb->cbstat = fCBSTATInsertCopy;
    pfucb->levelPrep = pfucb->ppib->Level();
}

INLINE VOID PrepareInsertReadOnlyCopy( FUCB *pfucb )
{
    pfucb->cbstat = fCBSTATInsertReadOnlyCopy;
    pfucb->levelPrep = pfucb->ppib->Level();
    FUCBSetAlwaysRetrieveCopy( pfucb );
}

INLINE VOID PrepareInsertCopyDeleteOriginal( FUCB *pfucb )
{
    pfucb->updateid = AtomicExchangeAdd( &PinstFromPfucb( pfucb )->m_updateid, updateidIncrement );
    pfucb->cbstat = fCBSTATInsertCopy | fCBSTATInsertCopyDeleteOriginal;
    pfucb->levelPrep = pfucb->ppib->Level();
}

INLINE VOID PrepareReplaceNoLock( FUCB *pfucb )
{
    pfucb->updateid = AtomicExchangeAdd( &PinstFromPfucb( pfucb )->m_updateid, updateidIncrement );
    pfucb->cbstat = fCBSTATReplace;
    pfucb->levelPrep = (pfucb)->ppib->Level();
}

INLINE VOID PrepareReplace( FUCB *pfucb )
{
    pfucb->updateid = AtomicExchangeAdd( &PinstFromPfucb( pfucb )->m_updateid, updateidIncrement );
    pfucb->cbstat = fCBSTATReplace | fCBSTATLock;
    pfucb->levelPrep = (pfucb)->ppib->Level();
}

INLINE VOID UpgradeReplaceNoLock( FUCB *pfucb )
{
    Assert( fCBSTATReplace == pfucb->cbstat );
    pfucb->cbstat = fCBSTATReplace | fCBSTATLock;
}

INLINE VOID FUCBResetUpdateid( FUCB *pfucb )
{
    // Never actually need to reset the updateid, because
    // it is only used by IsamSetColumn, which must always
    // be preceded by a PrepareUpdate, which will refresh
    // the updateid.
    Assert( fFalse );

    pfucb->updateid = updateidNil;
}

INLINE BOOL FFUCBCheckChecksum( const FUCB *pfucb )
{
    return UlChecksum( pfucb->kdfCurr.data.Pv(), pfucb->kdfCurr.data.Cb() ) ==
            pfucb->ulChecksum;
}

INLINE BOOL FFUCBReplacePrepared( const FUCB *pfucb )
{
    return pfucb->cbstat & fCBSTATReplace;
}

INLINE BOOL FFUCBInsertCopyDeleteOriginalPrepared( const FUCB *pfucb )
{
    return ( pfucb->cbstat & ( fCBSTATInsertCopyDeleteOriginal | fCBSTATInsertCopy ) )
            == ( fCBSTATInsertCopyDeleteOriginal | fCBSTATInsertCopy );
}

INLINE VOID FUCBSetUpdateForInsertCopyDeleteOriginal( FUCB *pfucb )
{
    //  graduate from Prepared mode to Update mode
    Assert( FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) );
    pfucb->cbstat = fCBSTATInsertCopy | fCBSTATUpdateForInsertCopyDeleteOriginal;
    Assert( !FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) );
}

INLINE BOOL FFUCBUpdateForInsertCopyDeleteOriginal( const FUCB *pfucb )
{
    return ( pfucb->cbstat & ( fCBSTATUpdateForInsertCopyDeleteOriginal | fCBSTATInsertCopy ) )
            == ( fCBSTATUpdateForInsertCopyDeleteOriginal | fCBSTATInsertCopy );
}

INLINE BOOL FFUCBReplaceNoLockPrepared( const FUCB *pfucb )
{
    return ( !( pfucb->cbstat & fCBSTATLock ) &&
             FFUCBReplacePrepared( pfucb ) );
}

INLINE BOOL FFUCBInsertPrepared( const FUCB *pfucb )
{
    return pfucb->cbstat & (fCBSTATInsert|fCBSTATInsertCopy);
}

INLINE BOOL FFUCBMustSetAutoIncPrepared( const FUCB *pfucb )
{
    const BOOL fMustSetAutoIncPrepared = pfucb->cbstat & fCBSTATMustSetAutoInc;

    // For now, we only allow override of the auto-inc during inserts.
    // If this changes in the future, please make sure codepaths that
    // test for FFUCBMustSetAutoIncPrepared() handle operations other
    // than inserts too.
    Expected( !fMustSetAutoIncPrepared || ( pfucb->cbstat == ( fCBSTATInsert | fCBSTATMustSetAutoInc ) ) );

    return fMustSetAutoIncPrepared;
}

INLINE BOOL FFUCBInsertCopyPrepared( const FUCB *pfucb )
{
    return pfucb->cbstat & fCBSTATInsertCopy;
}

INLINE BOOL FFUCBInsertReadOnlyCopyPrepared( const FUCB * const pfucb )
{
    return pfucb->cbstat & fCBSTATInsertReadOnlyCopy;
}

INLINE BOOL FFUCBUpdatePrepared( const FUCB *pfucb )
{
    return pfucb->cbstat & (fCBSTATInsert|fCBSTATInsertCopy|fCBSTATInsertReadOnlyCopy|fCBSTATReplace);
}

INLINE BOOL FFUCBUpdatePreparedLevel( const FUCB *pfucb, const LEVEL level )
{
    return ( FFUCBUpdatePrepared( pfucb ) && pfucb->levelPrep >= level );
}

INLINE BOOL FFUCBSetPrepared( const FUCB *pfucb )
{
    return ( ( pfucb->cbstat & (fCBSTATInsert|fCBSTATInsertCopy|fCBSTATReplace) )
            && pfucb->levelPrep <= pfucb->ppib->Level() );
}

INLINE VOID FUCBResetCbstat( FUCB *pfucb )
{
    pfucb->cbstat = fCBSTATNull;
}

INLINE BOOL FFUCBAtPrepareLevel( const FUCB *pfucb )
{
    return ( pfucb->levelPrep == pfucb->ppib->Level() );
}

INLINE VOID FUCBResetUpdateFlags( FUCB *pfucb )
{
    if( FFUCBInsertReadOnlyCopyPrepared( pfucb ) )
    {
        Assert( FFUCBAlwaysRetrieveCopy( pfucb ) );
        Assert( !FFUCBNeverRetrieveCopy( pfucb ) );
        FUCBResetAlwaysRetrieveCopy( pfucb );
    }

    FUCBResetDeferredChecksum( pfucb );
    FUCBResetUpdateSeparateLV( pfucb );
    FUCBResetCbstat( pfucb );
    FUCBResetTagImplicitOp( pfucb );
}

INLINE VOID FUCBAssertValidSearchKey( FUCB * const pfucb )
{
    Assert( NULL != pfucb->dataSearchKey.Pv() );
    
    // in theory, someone can try to seek with a NULL normalized key.
    // since this happened with the Store.exe and exposed 
    // a seek bug, we added test for it, hence we need to
    // disable this assert
    //
    //  Assert( pfucb->dataSearchKey.Cb() > 0 );
    Assert( pfucb->dataSearchKey.Cb() <= cbLimitKeyMostMost );
    Assert( pfucb->cColumnsInSearchKey > 0 );
    Assert( pfucb->cColumnsInSearchKey <= JET_ccolKeyMost );
}

INLINE VOID FUCBAssertNoSearchKey( FUCB * const pfucb )
{
    Assert( NULL == pfucb->dataSearchKey.Pv() );
    Assert( 0 == pfucb->dataSearchKey.Cb() );
    Assert( 0 == pfucb->cColumnsInSearchKey );
    Assert( keystatNull == pfucb->keystat );
}

INLINE BOOL FFUCBUsingTableSearchKeyBuffer( const FUCB *pfucb )
{
    return pfucb->fUsingTableSearchKeyBuffer;
}

INLINE VOID FUCBSetUsingTableSearchKeyBuffer( FUCB *pfucb )
{
    pfucb->fUsingTableSearchKeyBuffer = fTrue;
}

INLINE VOID FUCBResetUsingTableSearchKeyBuffer( FUCB *pfucb )
{
    pfucb->fUsingTableSearchKeyBuffer = fFalse;
}

INLINE VOID KSReset( FUCB *pfucb )
{
    pfucb->keystat = keystatNull;
}

INLINE VOID KSSetPrepare( FUCB *pfucb )
{
    pfucb->keystat |= keystatPrepared;
}

INLINE VOID KSSetTooBig( FUCB *pfucb )
{
    pfucb->keystat |= keystatTooBig;
}

INLINE VOID KSSetLimit( FUCB *pfucb )
{
    pfucb->keystat |= keystatLimit;
}

INLINE BOOL FKSPrepared( const FUCB *pfucb )
{
    return pfucb->keystat & keystatPrepared;
}

INLINE BOOL FKSTooBig( const FUCB *pfucb )
{
    return pfucb->keystat & keystatTooBig;
}

INLINE BOOL FKSLimit( const FUCB *pfucb )
{
    return pfucb->keystat & keystatLimit;
}


//  returns pgnoFDP of this tree
//
INLINE PGNO PgnoFDP( const FUCB *pfucb )
{
    Assert( pfcbNil != pfucb->u.pfcb );
    Assert( pgnoNull != pfucb->u.pfcb->PgnoFDP()
        || ( FFUCBSort( pfucb ) && pfucb->u.pfcb->FTypeSort() ) );
    return pfucb->u.pfcb->PgnoFDP();
}

//  returns the latch hint for the pgnoFDP of this tree
//
INLINE BFLatch* PBFLatchHintPgnoFDP( const FUCB *pfucb )
{
    Assert( pfcbNil != pfucb->u.pfcb );
    return pfucb->u.pfcb->PBFLatchHintPgnoFDP();
}

INLINE OBJID ObjidFDP( const FUCB *pfucb )
{
#ifdef DEBUG
    Assert( pfcbNil != pfucb->u.pfcb );
    if ( dbidTemp == g_rgfmp[ pfucb->u.pfcb->Ifmp() ].Dbid() )
    {
        if ( pgnoNull == pfucb->u.pfcb->PgnoFDP() )
        {
            Assert( objidNil == pfucb->u.pfcb->ObjidFDP() );
            Assert( FFUCBSort( pfucb ) && pfucb->u.pfcb->FTypeSort() );
        }
        else
        {
            Assert( objidNil != pfucb->u.pfcb->ObjidFDP() );
        }
    }
    else
    {
        Assert( objidNil != pfucb->u.pfcb->ObjidFDP() || g_fRepair );
        Assert( pgnoNull != pfucb->u.pfcb->PgnoFDP() || g_fRepair );
    }
#endif
    return pfucb->u.pfcb->ObjidFDP();
}


//  returns root page of OwnExt tree
//
INLINE PGNO PgnoOE( const FUCB *pfucb )
{
    Assert( pfcbNil != pfucb->u.pfcb );
    return pfucb->u.pfcb->PgnoOE();
}

//  returns the latch hint for the root page of OwnExt tree
//
INLINE BFLatch* PBFLatchHintPgnoOE( const FUCB *pfucb )
{
    Assert( pfcbNil != pfucb->u.pfcb );
    return pfucb->u.pfcb->PBFLatchHintPgnoOE();
}

//  returns root page of AvailExt tree
//
INLINE PGNO PgnoAE( const FUCB *pfucb )
{
    Assert( pfcbNil != pfucb->u.pfcb );
    return pfucb->u.pfcb->PgnoAE();
}

//  returns the latch hint for the root page of AvailExt tree
//
INLINE BFLatch* PBFLatchHintPgnoAE( const FUCB *pfucb )
{
    Assert( pfcbNil != pfucb->u.pfcb );
    return pfucb->u.pfcb->PBFLatchHintPgnoAE();
}

//  returns root of tree
//  if space tree, returns appropriate root page of space tree
//
INLINE PGNO PgnoRoot( const FUCB *pfucb )
{
    PGNO    pgno;

    if ( !FFUCBSpace( pfucb ) )
    {
        pgno = PgnoFDP( pfucb );
    }
    else
    {
        Assert( PgnoOE( pfucb ) != pgnoNull );
        Assert( PgnoAE( pfucb ) != pgnoNull );

        pgno = FFUCBOwnExt( pfucb ) ?
                        PgnoOE( pfucb )  :
                        PgnoAE( pfucb ) ;
    }
    Assert ( pgnoNull != pgno );
    return pgno;
}

//  returns the latch hint for the root of tree
//  if space tree, returns appropriate root page of space tree
//
INLINE BFLatch* PBFLatchHintPgnoRoot( const FUCB *pfucb )
{
    BFLatch* pbflHint;

    if ( !FFUCBSpace( pfucb ) )
    {
        pbflHint = PBFLatchHintPgnoFDP( pfucb );
    }
    else
    {
        Assert( PgnoOE( pfucb ) != pgnoNull );
        Assert( PgnoAE( pfucb ) != pgnoNull );

        pbflHint = FFUCBOwnExt( pfucb ) ?
                        PBFLatchHintPgnoOE( pfucb )  :
                        PBFLatchHintPgnoAE( pfucb ) ;
    }
    Assert( NULL != pbflHint );
    Assert( NULL == pbflHint->pv ); // the hints only have a dwContext / pbf.
    return pbflHint;
}

INLINE TCE TceFromFUCB(FUCB * pfucb)
{
    return pfucb->u.pfcb->TCE( FFUCBSpace( pfucb ) );
}

INLINE ERR ErrSetLS( FUCB *pfucb, const LSTORE ls )
{
    if ( JET_LSNil != pfucb->ls && JET_LSNil != ls )
    {
        return ErrERRCheck( JET_errLSAlreadySet );
    }
    else
    {
        pfucb->ls = ls;
        return JET_errSuccess;
    }
}

INLINE ERR ErrGetLS( FUCB *pfucb, LSTORE *pls, const BOOL fReset )
{
    *pls = pfucb->ls;

    if ( fReset )
        pfucb->ls = JET_LSNil;

    return ( JET_LSNil == *pls && !fReset ? ErrERRCheck( JET_errLSNotSet ) : JET_errSuccess );
}


//  navigate level support
//
INLINE LEVEL LevelFUCBNavigate( const FUCB *pfucb )
{
    return pfucb->levelNavigate;
}

INLINE VOID FUCBSetLevelNavigate( FUCB *pfucb, LEVEL level )
{
    const volatile LEVEL levelPib = pfucb->ppib->Level();
    
    Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() ||
            level >= levelMin &&
            ( level <= levelPib )
            );
            
    Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() ||
            pfucb->levelNavigate >= levelMin &&
            pfucb->levelNavigate <= levelPib + 1 );
    pfucb->levelNavigate = level;
}

#ifdef  DEBUG
INLINE VOID CheckTable( const PIB *ppib, const FUCB *pfucb )
{
    Assert( pfucb->ppib == ppib );
    Assert( PinstFromIfmp( pfucb->ifmp )->FRecovering() || FFUCBIndex( pfucb ) );
    Assert( !( FFUCBSort( pfucb ) ) );
    Assert( pfucb->u.pfcb != NULL );
}

INLINE VOID CheckSort( const PIB *ppib, const FUCB *pfucb )
{
    Assert( pfucb->ppib == ppib );
    Assert( FFUCBSort( pfucb ) );
    Assert( !( FFUCBIndex( pfucb ) ) );
    Assert( pfucb->u.pscb != NULL );
}

INLINE VOID CheckFUCB( const PIB *ppib, const FUCB *pfucb )
{
    Assert( pfucb->ppib == ppib );
    Assert( pfucb->u.pfcb != NULL );
}

//  ================================================================
INLINE VOID FUCB::AssertValid( ) const
//  ================================================================
{
    Assert( FAlignedForThisPlatform( this ) );
    CResource::AssertValid( JET_residFUCB, this );
    Assert( NULL != u.pfcb );
    ASSERT_VALID( ppib );
}

INLINE VOID CheckSecondary( const FUCB *pfucb )
{
    Assert( pfucb->pfucbCurIndex == pfucbNil ||
            FFUCBSecondary( pfucb->pfucbCurIndex ) );
}

#else   /* !DEBUG */
#define CheckSort( ppib, pfucb )
#define CheckTable( ppib, pfucb )
#define CheckFUCB( ppib, pfucb )
#define CheckSecondary( pfucb )
#endif  /* !DEBUG */

INLINE CSR *Pcsr( FUCB *pfucb )
{
    return &(pfucb->csr);
}

INLINE const CSR *Pcsr( const FUCB *pfucb )
{
    return &(pfucb->csr);
}

INLINE ERR ErrFUCBCheckUpdatable( const FUCB *pfucb )
{
    return FFUCBUpdatable( pfucb ) ?
                JET_errSuccess :
                ErrERRCheck( JET_errPermissionDenied );
}

//  set column bit array macros
//

INLINE VOID FUCBResetColumnSet( FUCB *pfucb )
{
    memset( pfucb->rgbitSet, 0x00, 32 );
    pfucb->fAnyColumnSet = fFalse;
}

INLINE VOID FUCBSetColumnSet( FUCB * pfucb, FID fid )
{
    pfucb->rgbitSet[ fid.IbHash()] |= fid.IbitHash();
    pfucb->fAnyColumnSet = fTrue;
}

INLINE BOOL FFUCBColumnSet( const FUCB * pfucb, FID fid )
{
    return (pfucb->rgbitSet[fid.IbHash()] & fid.IbitHash());
}

INLINE BOOL FFUCBAnyColumnSet( const FUCB * pfucb )
{
    return pfucb->fAnyColumnSet;
}

INLINE BOOL FFUCBTaggedColumnSet( const FUCB * pfucb )
{
    const LONG_PTR *        pl      = (LONG_PTR *)( pfucb->rgbitSet + ( cbitFixedVariable/8 ) );
    const LONG_PTR * const  plMax   = (LONG_PTR *)pfucb->rgbitSet
                                        + ( cbRgbitIndex / sizeof(LONG_PTR) );

    // Verify array is LONG_PTR-aligned and tagged-column portion of
    // the array is LONG_PTR-aligned.
    Assert( (LONG_PTR)pfucb->rgbitSet % sizeof(LONG_PTR) == 0 );
    Assert( ( cbitFixedVariable/8 ) % sizeof(LONG_PTR) == 0 );

    for ( ; pl < plMax; pl++ )
    {
        if ( 0 != *pl )
            return fTrue;
    }
    Assert( pl == (LONG_PTR *)( pfucb->rgbitSet + ( ( cbitTagged+cbitFixedVariable ) / 8 ) ) );

    return fFalse;
}

INLINE ERR ErrFUCBFromTableid( PIB *ppib, JET_TABLEID tableid, FUCB **ppfucb )
{
    FUCB *pfucb = (FUCB *) tableid;

    if ( pfucb->ppib != ppib || pfucb->pvtfndef == &vtfndefInvalidTableid )
        return ErrERRCheck( JET_errInvalidTableId );

    if ( pfucb->pvtfndef == &vtfndefIsamMustRollback || pfucb->pvtfndef == &vtfndefTTBaseMustRollback )
        return ErrERRCheck( JET_errIllegalOperation );
    
    Assert( pfucb->pvtfndef == &vtfndefIsam && FFUCBIndex( pfucb ) ||
            pfucb->pvtfndef == &vtfndefTTBase && FFUCBIndex( pfucb ) && g_rgfmp[ pfucb->ifmp ].Dbid() == dbidTemp ||
            pfucb->pvtfndef == &vtfndefTTSortIns && FFUCBSort( pfucb ) && g_rgfmp[ pfucb->ifmp ].Dbid() == dbidTemp ||
            pfucb->pvtfndef == &vtfndefTTSortRet && FFUCBSort( pfucb) && g_rgfmp[ pfucb->ifmp ].Dbid() == dbidTemp );
    *ppfucb = pfucb;
    return JET_errSuccess;
}


//  **************************************************
//  FUCB API

ERR ErrFUCBOpen( PIB *ppib, IFMP ifmp, FUCB **ppfucb, const LEVEL level = 0 );
VOID FUCBClose( FUCB *pfucb, FUCB *pfucbPrev = pfucbNil );

INLINE VOID FUCBCloseDeferredClosed( FUCB *pfucb )
{
#ifdef DEBUG
    // Deferred-closed cursors are closed during commit0 or rollback.
    if( NULL != pfucb->ppib->prceNewest )
    {
        Assert( PinstFromPpib( pfucb->ppib )->RwlTrx( pfucb->ppib ).FReader() );
    }
#endif  //  DEBUG

    // Secondary index FCB may have been deallocated by
    // rollback of CreateIndex or cleanup of DeleteIndex
    if ( pfcbNil != pfucb->u.pfcb )
    {
        if ( FFUCBDenyRead( pfucb ) )
            pfucb->u.pfcb->ResetDomainDenyRead();
        if ( FFUCBDenyWrite( pfucb ) )
            pfucb->u.pfcb->ResetDomainDenyWrite();
        pfucb->u.pfcb->Unlink( pfucb );
    }
    else
    {
        Assert( FFUCBSecondary( pfucb ) );
    }

    FUCBClose( pfucb );
}

INLINE VOID FUCBRemoveEncryptionKey( FUCB *pfucb )
{
    if ( pfucb->pbEncryptionKey != NULL )
    {
        OSMemorySecureZero( pfucb->pbEncryptionKey, pfucb->cbEncryptionKey );
        OSMemoryHeapFree( pfucb->pbEncryptionKey );
    }
    pfucb->pbEncryptionKey = NULL;
    pfucb->cbEncryptionKey = 0;
}

VOID FUCBCloseAllCursorsOnFCB(
    PIB         * const ppib,
    FCB         * const pfcb );


VOID FUCBSetIndexRange( FUCB *pfucb, JET_GRBIT grbit );
VOID FUCBResetIndexRange( FUCB *pfucb );
ERR ErrFUCBCheckIndexRange( FUCB *pfucb, const KEY& key );
VOID FUCBIllegalOperationFDPToBeDeleted( FUCB *pfucb, const OBJID objidFDP );


class CTableHash : public CSimpleHashTable<FUCB>
{
    public:
        CTableHash( const ULONG ctables ) :
            CSimpleHashTable<FUCB>( min( ctables, ctablesMax ), OffsetOf( FUCB, pfucbHashOverflow ) )   {}
        ~CTableHash()   {}
        
        enum    { ctablesMax = 0xFFFF };        //  max size of hash table

        ULONG UlHash( const PGNO pgnoFDP, const IFMP ifmp, const PROCID procid ) const
        {
            return ULONG( ( ( procid * ifmp ) + pgnoFDP ) % CEntries() );
        }

        VOID InsertFucb( FUCB * pfucb )
        {
            const ULONG ulHash  = UlHash( PgnoFDP( pfucb ), pfucb->ifmp, pfucb->ppib->procid );
            InsertEntry( pfucb, ulHash );
        }

        VOID RemoveFucb( FUCB * pfucb )
        {
            const ULONG ulHash  = UlHash( PgnoFDP( pfucb ), pfucb->ifmp, pfucb->ppib->procid );
            RemoveEntry( pfucb, ulHash );
        }

        FUCB * PfucbHashFind(
            const IFMP      ifmp,
            const PGNO      pgnoFDP,
            const PROCID    procid,
            const BOOL      fSpace ) const
        {
            const  ULONG        ulHash  = UlHash( pgnoFDP, ifmp, procid );

            for ( FUCB * pfucb = PentryOfHash( ulHash );
                pfucbNil != pfucb;
                pfucb = pfucb->pfucbHashOverflow )
            {
                if ( pfucb->ifmp == ifmp
                    && PgnoFDP( pfucb ) == pgnoFDP
                    && pfucb->ppib->procid == procid
                    && ( fSpace && FFUCBSpace( pfucb )
                        || !fSpace && !FFUCBSpace( pfucb ) ) )
                {
                    return pfucb;
                }
            }

            return pfucbNil;
        }

        VOID PurgeUnversionedTables();

        VOID PurgeTablesForFCB( const FCB * pfcb );
};

