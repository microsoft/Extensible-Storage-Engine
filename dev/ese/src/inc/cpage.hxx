// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  ****************************************************************
//  MACROS
//  ****************************************************************


#ifdef DEBUG

//  DEBUG_PAGE:
//          Goal is to get a reasonable level of debug checking, that costs only 50% - 300%. 
//      Details
//          call ASSERT_VALID( this ) before each method
//          Force call DebugCheckAll_() after each non-const public method
//
///#define DEBUG_PAGE

//  DEBUG_PAGE_EXTENSIVE:
//          Goal is to force all levels of checking, this is over 100x as slow as debug.
//      Details
//          call ASSERT_VALID( this ) before each method
//          Force call DebugCheckAll_() after each non-const public method
//
///#define DEBUG_PAGE_EXTENSIVE



#define DBGSZ(sz)   _tprintf( _T( "%s" ), sz )
///#define DBGSZ(sz)    0

#endif  // DEBUG




// Reasons for doing a page reorganization

enum CPAGEREORG
{
    reorgOther = 0,
    reorgFreeSpaceRequest,
    reorgPageMoveLogging,
    reorgDehydrateBuffer,
    reorgMax
};


//  ================================================================
class IPageValidationAction
//  ================================================================
{
public:
    virtual void BadChecksum(
        const PGNO pgno,
        const ERR err,
        const PAGECHECKSUM checksumExpected,
        const PAGECHECKSUM checksumActual ) = 0;
    virtual void UninitPage( const PGNO pgno, const ERR err ) = 0;
    virtual void BadPgno( const PGNO pgnoExpected, const PGNO pgnoActual, const ERR err ) = 0;
    virtual void BitCorrection( const PGNO pgno, const INT ibitCorrupted ) = 0;
    virtual void BitCorrectionFailed( const PGNO pgno, const ERR err, const INT ibitCorrupted ) = 0;
    virtual void LostFlush(
        const PGNO pgno,
        const FMPGNO fmpgno,
        const INT pgftExpected,
        const INT pgftActual,
        const ERR err,
        const BOOL fRuntime,
        const BOOL fFailOnRuntimeOnly ) = 0;

protected:
    IPageValidationAction() {}
};

//  ================================================================
class CPageValidationNullAction : public IPageValidationAction
//  ================================================================
{
public:
    CPageValidationNullAction() {}
    ~CPageValidationNullAction() {}
    
    void BadChecksum(
        const PGNO pgno,
        const ERR err,
        const PAGECHECKSUM checksumExpected,
        const PAGECHECKSUM checksumActual ) {}
    void UninitPage( const PGNO pgno, const ERR err ) {}
    void BadPgno( const PGNO pgnoExpected, const PGNO pgnoActual, const ERR err ) {}
    void BitCorrection( const PGNO pgno, const INT ibitCorrupted ) {}
    void BitCorrectionFailed( const PGNO pgno, const ERR err, const INT ibitCorrupted ) {}
    void LostFlush(
        const PGNO pgno,
        const FMPGNO fmpgno,
        const INT pgftExpected,
        const INT pgftActual,
        const ERR err,
        const BOOL fRuntime,
        const BOOL fFailOnRuntimeOnly ) { }
};


#ifdef DEBUGGER_EXTENSION

//  Some debugger pre-declarations

struct TREEINFO;
class CPAGE;
LOCAL VOID EDBGUpdatePgCounts(
    TREEINFO * const    rgtreeinfo,
    const BF * const    pbf,
    const CPAGE * const pcpage );

#endif

//  ================================================================
class CPageValidationLogEvent : public IPageValidationAction
//  ================================================================
{
public:
    CPageValidationLogEvent( const IFMP ifmp, const INT logflags, const CategoryId category );
    ~CPageValidationLogEvent();

    enum {  LOG_NONE            = 0x00 };
    enum {  LOG_BAD_CHECKSUM    = 0x01 };
    enum {  LOG_UNINIT_PAGE     = 0x02 };
    enum {  LOG_BAD_PGNO        = 0x04 };
    enum {  LOG_CORRECTION      = 0x08 };
    enum {  LOG_CHECKFAIL       = 0x10 };
    enum {  LOG_LOST_FLUSH      = 0x20 };
    enum {  LOG_EXTENSIVE_CHECKS= 0x40 };
    enum {  LOG_ALL             = LOG_BAD_CHECKSUM | LOG_UNINIT_PAGE | LOG_BAD_PGNO | LOG_CORRECTION | LOG_CHECKFAIL | LOG_LOST_FLUSH };

    void BadChecksum( const PGNO pgno, const ERR err, const PAGECHECKSUM checksumExpected, const PAGECHECKSUM checksumActual );
    void UninitPage( const PGNO pgno, const ERR err );
    void BadPgno( const PGNO pgnoExpected, const PGNO pgnoActual, const ERR err );
    void BitCorrection( const PGNO pgno, const INT ibitCorrupted );
    void BitCorrectionFailed( const PGNO pgno, const ERR err, const INT ibitCorrupted );
    void LostFlush(
        const PGNO pgno,
        const FMPGNO fmpgno,
        const INT pgftExpected,
        const INT pgftActual,
        const ERR err,
        const BOOL fRuntime,
        const BOOL fFailOnRuntimeOnly );

private:
    bool FRepairOrRecovery_( const IFMP ifmp ) const;
    VOID ReportBadChecksum_( const PGNO pgno, const ERR err, const PAGECHECKSUM checksumExpected, const PAGECHECKSUM checksumActual ) const;
    VOID ReportBadPgno_( const PGNO pgno, const ERR err ) const;
    VOID ReportUninitializedPage_( const PGNO pgno, const ERR err ) const;
    VOID ReportPageNumberFailed_( const PGNO pgno, const ERR err, const PGNO pgnoActual ) const;
    VOID ReportPageCorrection_( const PGNO pgno, const INT ibitCorrupted ) const;
    VOID ReportPageCorrectionFailed_( const PGNO pgno, const ERR err, const INT ibitCorrupted ) const;
    VOID ReportLostFlush_(
        const PGNO pgno,
        const FMPGNO fmpgno,
        const INT pgftExpected,
        const INT pgftActual,
        const ERR err,
        const BOOL fRuntime,
        const BOOL fFailOnRuntimeOnly,
        const WCHAR* const wszContext ) const;
    
private:
    const IFMP m_ifmp;
    const INT m_logflags;
    const CategoryId m_category;
};

class ILatchManager;

//  ================================================================
enum PAGEValidationFlags
//  ================================================================
{
    pgvfDefault                             = 0x00000000,
    pgvfFixErrors                           = 0x00000001,
    pgvfExtensiveChecks                     = 0x00000002,
    pgvfDoNotCheckForLostFlush              = 0x00000004,
    pgvfFailOnRuntimeLostFlushOnly          = 0x00000008,
    pgvfDoNotCheckForLostFlushIfNotRuntime  = 0x00000010,
    pgvfMax                                 = 0x00000020
};

DEFINE_ENUM_FLAG_OPERATORS_BASIC( PAGEValidationFlags );

class CFlushMap;

//  ================================================================
class CPAGE
//  ================================================================
//
//  A page is the basic physical storage unit of the B-tree
//  A page stores data blobs and flags associated with
//  each data blob. A special data blob, the external header
//  is provided. Internally the external header is treated
//  the same as the other blobs.
//
//  Each page holds flags associated with the page, the objid
//  of the FDP for the table the page is in, the pgno's of
//  the page's two neighbors, a checksum and the time of
//  last modification.
//
//  Externally the blobs are represented at LINE's
//
//  CPAGE interacts with the buffer manager to obtain and latch
//  pages, to upgrade the latches and to eventually release the
//  page
//
//  It is possible to assign one CPAGE to another, but only if
//  the destination CPAGE has released its underlying page. This
//  is to prevent aliasing errors
//
//-
{
    public:
        //  constructor/destructor
        CPAGE   ( );
        ~CPAGE  ( );
        CPAGE& operator=    ( CPAGE& );  //  ** this destroys the CPAGE being assigned from
        static ERR ErrResetHeader   ( PIB *ppib, const IFMP ifmp, const PGNO pgno );

        //  creation/destruction methods
        ERR ErrGetReadPage          (   PIB * ppib,
                                        IFMP ifmp,
                                        PGNO pgno,
                                        BFLatchFlags bflf,
                                        BFLatch* pbflHint = NULL );
        ERR ErrGetRDWPage           (   PIB * ppib,
                                        IFMP ifmp,
                                        PGNO pgno,
                                        BFLatchFlags bflf,
                                        BFLatch* pbflHint = NULL );
        ERR ErrGetNewPreInitPage    (   PIB * ppib,
                                        IFMP ifmp,
                                        PGNO pgno,
                                        OBJID objidFDP,
                                        DBTIME dbtimeNoLog,
                                        BOOL fLogNewPage );

        //  starts filling out the pre-initialized pages with flags
        //  and sets up an external header
        VOID ConsumePreInitPage( const ULONG fPageFlags );

        //  signals that the cycle of a pre-initialized page is now finalized
        VOID FinalizePreInitPage( OnDebug( const BOOL fCheckPreInitPage ) );

        //  this method is used to fabricate a page that will be used to
        //  indicate a page believed to be shrunk in a further DBTIME (to be
        //  shrunk in the future through recovery).
        VOID GetShrunkPage          (   const IFMP ifmp,
                                        const PGNO pgno,
                                        VOID * const pv,
                                        const ULONG cb );

        //  this method is used to fabricate a page that will be used to
        //  indicate a page reverted to a new page as part of revert operation using RBS.
        VOID GetRevertedNewPage     (   const PGNO pgno,
                                        VOID * const pv,
                                        const ULONG cb );

        //  this LoadPage method allocates memory and copies the given page into it
        //  the memory is freed when the page is released
        ERR ErrLoadPage             (   PIB * ppib,
                                        IFMP ifmp,
                                        PGNO pgno,
                                        const VOID * pv,
                                        const ULONG cb );

        //  these LoadPage methods use the passed-in buffer directly
        VOID LoadNewPage            (   const IFMP ifmp,
                                        const PGNO pgno,
                                        const OBJID objidFDP,
                                        const ULONG fFlags,
                                        VOID * const pv,
                                        const ULONG cb );

#ifdef ENABLE_JET_UNIT_TEST
        VOID LoadNewTestPage( _In_ const ULONG cb, _In_ const IFMP ifmp = ifmpNil );
#endif // ENABLE_JET_UNIT_TEST

        VOID LoadPage( const IFMP ifmp, const PGNO pgno, VOID * const pv, const ULONG cb );
        VOID LoadDehydratedPage( const IFMP ifmp, const PGNO pgno, VOID * const pv, const ULONG cb, const ULONG cbPage );
        VOID LoadPage( VOID * const pv, const ULONG cb );
        VOID ReBufferPage( _In_ const BFLatch& bfl, const IFMP ifmp, const PGNO pgno, VOID * const pv, const ULONG cb );
        
        VOID UnloadPage( );
        VOID MarkAsSuperCold( );

        //  initialize a new page
        VOID PreInitializeNewPage_(    PIB * const ppib,
                                    const IFMP ifmp,
                                    const PGNO pgno,
                                    const OBJID objidFDP,
                                    const DBTIME dbtime );

        //  latch manipulation methods
        ERR ErrUpgradeReadLatchToWriteLatch();
        VOID UpgradeRDWLatchToWriteLatch();

        ERR ErrTryUpgradeReadLatchToWARLatch();
        VOID UpgradeRDWLatchToWARLatch();

        VOID DowngradeWriteLatchToRDWLatch();
        VOID DowngradeWriteLatchToReadLatch();
        VOID DowngradeWARLatchToRDWLatch();
        VOID DowngradeWARLatchToReadLatch();
        VOID DowngradeRDWLatchToReadLatch();

        VOID ReleaseWriteLatch( BOOL fTossImmediate = fFalse );
        VOID ReleaseRDWLatch( BOOL fTossImmediate = fFalse );
        VOID ReleaseReadLatch( BOOL fTossImmediate = fFalse );

        VOID Dirty              ( const BFDirtyFlags bfdf );
        VOID DirtyForScrub      ( );
        VOID CoordinatedDirty   ( const DBTIME dbtime, const BFDirtyFlags bfdf );
        VOID SetLgposModify     ( LGPOS lgpos );

        //  get/insert/replace
        //
        //  the iline returned by get and used in other routines may be
        //  invalidated by Replace/Insert or Delete
        template< PageNodeBoundsChecking pgnbc = pgnbcNoChecks >
        VOID GetPtrExternalHeader   ( LINE * pline, _Out_opt_ ERR * perrNoEnforce = NULL ) const;
        VOID SetExternalHeader( const DATA * rgdata, INT cdata, INT fFlags );

        template< PageNodeBoundsChecking pgnbc = pgnbcNoChecks >
        VOID GetPtr             ( INT iline, LINE * pline, _Out_opt_ ERR * perrNoEnforce = NULL ) const;

        //  Note: Err enabled versions of these APIs assume you handle the error, even if LINE is filled out.
        ERR  ErrGetPtr               ( INT iline, LINE * pline ) const;
        ERR  ErrGetPtrExternalHeader ( LINE * pline ) const;

        VOID GetLatchHint       ( INT iline, BFLatch** ppbfl ) const;

        VOID ReplaceFlags       ( INT iline, INT fFlags );

        VOID Insert             ( INT iline, const DATA * rgdata, INT cdata, INT fFlags );
        VOID Replace            ( INT iline, const DATA * rgdata, INT cdata, INT fFlags );
        VOID Delete             ( INT iline );

        //  accessor methods
        USHORT  CbPageFree      ( ) const;  // see notes on CbFree_ below
        USHORT  CbUncommittedFree   ( ) const;

        INT     Clines          ( ) const;
        ULONG   FFlags          ( ) const;

        BOOL    FLeafPage       ( ) const;
        BOOL    FInvisibleSons  ( ) const;
        BOOL    FRootPage       ( ) const;
        BOOL    FFDPPage        ( ) const;
        BOOL    FEmptyPage      ( ) const;
        BOOL    FPreInitPage    ( ) const;
        BOOL    FParentOfLeaf   ( ) const;
        BOOL    FSpaceTree      ( ) const;
        BOOL    FScrubbed       ( ) const;

        BOOL    FRepairedPage   ( ) const;

        BOOL    FPrimaryPage    ( ) const;
        BOOL    FIndexPage      ( ) const;
        BOOL    FNonUniqueKeys  ( ) const;
        BOOL    FLongValuePage  ( ) const;
        BOOL    FNewRecordFormat( ) const;
        BOOL    FNewChecksumFormat  ( ) const;
        BOOL    FLastNodeHasNullKey( ) const;

        BOOL        FShrunkPage         ( ) const;
        BOOL        FRevertedNewPage    ( ) const;
        static BOOL FRevertedNewPage    ( const DBTIME dbtime ) { return dbtimeRevert == dbtime; }

        enum PageFlushType;
        PageFlushType Pgft      ( ) const;

        IFMP    Ifmp            ( ) const;
        OBJID   ObjidFDP        ( ) const;
        PGNO    PgnoThis        ( ) const;
        PGNO    PgnoREPAIRThis  ( ) const;
        PGNO    PgnoNext        ( ) const;
        PGNO    PgnoPrev        ( ) const;

        DBTIME  Dbtime          ( ) const;

        BOOL    FLoadedPage     ( ) const;
        VOID *  PvBuffer        ( ) const;
        BFLatch* PBFLatch       ( );
        BOOL    FIsNormalSized  ( ) const;

        //  setter methods
        VOID    SetPgno( PGNO pgno );
        VOID    SetPgnoNext ( PGNO pgno );
        VOID    SetPgnoPrev ( PGNO pgno );
        VOID    SetDbtime   ( const DBTIME dbtime );
        VOID    RevertDbtime ( const DBTIME dbtime, const ULONG fFlags );
        VOID    SetFlags    ( ULONG fFlags );
        VOID    ResetParentOfLeaf   ( );
        VOID    SetFEmpty    ( );
        VOID    SetFNewRecordFormat ( );

        //  bulk line flag manipulation
        VOID    ResetAllFlags( INT fFlags );
        bool    FAnyLineHasFlagSet( INT fFlags ) const;

        //  UncommittedFree methods
        //  FOR USE BY VERSION STORE ONLY
        VOID    SetCbUncommittedFree    ( INT cbUncommittedFreeNew );
        VOID    AddUncommittedFreed     ( INT cbToAdd );
        VOID    ReclaimUncommittedFreed ( INT cbToReclaim );

        //  Used to zero unused data on a page
        VOID    OverwriteUnusedSpace    ( const CHAR chZero );

        //  When logging a page image the amount of data to be logged is
        //  minimized by only logging the used data on the page. the easiest
        //  way to do this is by reorganizing the page so it looks like this
        //
        //  HEADER | LINES | FREE SPACE | TAGS
        //
        //  That way the page can be logged as a header and trailer
        //
        //  This method reorganizes the page and calculates the header/trailer
        //  pointers. As the page may be reorganized the page cannot be
        //  read-only
        VOID    ReorganizePage(
                    _Out_ const VOID ** pvHeader,
                    _Out_ size_t * const  pcbHeader,
                    _Out_ const VOID ** pvTrailer,
                    _Out_ size_t * const pcbTrailer);

        //  Used to validate if a pointer [and cb] is completely on the page
        BOOL        FOnPage( const void * pv, const INT cb ) const
        {
            ULONG_PTR pb = (ULONG_PTR) pv;  // convert to something we can do byte math on
            return pb >= (ULONG_PTR)m_bfl.pv &&
                    pb < ( (ULONG_PTR)m_bfl.pv + CbBuffer() ) &&
                    ( cb == 0 || ( ( pb + cb ) >= (ULONG_PTR)m_bfl.pv ) ) &&
                    ( cb == 0 || ( ( pb + cb ) <= ( (ULONG_PTR)m_bfl.pv + CbBuffer() ) ) );
        }

        //  Used to validate if a pointer [and cb] is completely in the data portion of the page
        //
        //    NOTE: Remember if this passes fFalse for a line that should be on the page, it may
        //    be the tag values having corrupt offsets, OR may be the itagMicFree value being
        //    corrupted to make TAG array looking artificially large.
        //
        BOOL        FOnData( _In_ const void * pv, _In_ const INT cb ) const
        {
            // A NULL pv can be used to ask about offsets.
            Assert( cb >= 1 );

            const ULONG_PTR pbFirstByte = (ULONG_PTR) pv;  // convert to something we can do byte math on
            const ULONG_PTR pbLastByte = pbFirstByte + cb - 1;
            Assert( pbFirstByte <= pbLastByte );

            const ULONG_PTR pbPageDataStart = PbDataStart_( FSmallPageFormat() );  // m_bfl.pv + header size
            const ULONG_PTR pbPageDataEnd = PbDataEnd_( CbBuffer() );              // m_bfl.pv + cbBufferCached - tag array size (off itagMicFree)

            Assert( pbPageDataStart < pbPageDataEnd || FNegTest( fCorruptingPageLogically ) ); // extreme corruption, where itagMicFree / tag array size is > than fits on page

            return pbFirstByte >= pbPageDataStart &&
                    pbFirstByte < pbPageDataEnd &&  // someday <= maybe if a 1-byte node ever shows up.
                    pbLastByte >= pbPageDataStart &&
                    pbLastByte <= pbPageDataEnd;
        }

        //  Used to calculate a checksum of all logged data on the page
        PAGECHECKSUM    LoggedDataChecksum() const;

        //  Page compress/decompression support for cache dehydration

        ULONG   CbContiguousDehydrateRequired_() const;
        ULONG   CbReorganizedDehydrateRequired_() const;
        VOID    DehydratePageUnadjusted_( _In_ const ULONG cbNewSize );
    public:
        VOID    OptimizeInternalFragmentation();
        BOOL    FPageIsDehydratable( _Out_ ULONG * pcbMinimumSize, _In_ const BOOL fAllowReorg = fTrue ) const;
        VOID    DehydratePage( _In_ const ULONG cbNewSize, _In_ const BOOL fAllowReorg );
        VOID    RehydratePage();
        
        //  Returns true if the page is zeroed

        static BOOL FPageIsInitialized( _In_ const void* const pv, _In_ ULONG cb );
        BOOL    FPageIsInitialized() const;

        ERR     ErrValidatePage(
            _In_ const PAGEValidationFlags pgvf,
            _In_ IPageValidationAction * const pvalidationaction,
            _In_ CFlushMap* pflushmap = NULL );

        //  integrity check
        enum CheckPageMode
        {
            OnErrorReturnError          = 0,    // Just return an error, for bit-correction, repair and unit tests.
            OnErrorFireWall             = 1,    // FireWall if retail RTM or a more detailed AssertRTL if non-RTM.
            OnErrorEnforce              = 2,    // Enforce (terminate the process).
        };

        enum CheckPageExtensiveness
        {
            CheckBasics                 =   0x0,    //  O(n) - iterates tags, we always check these things ...
            CheckTagsNonOverlapping     =   0x1,    //  allocates memory to map all data
            CheckLineBoundedByTag       =   0x2,    //  derefs TAG and checks cb prefix and cb suffix fit in overall node / TAG::cb
            CheckLinesInOrder           =   0x4,    //  makes sure the keys on the page are in ND key order

            CheckDefault                = ( CheckTagsNonOverlapping | CheckLineBoundedByTag ),
            CheckAll                    = ( CheckTagsNonOverlapping | CheckLineBoundedByTag | CheckLinesInOrder ),
        };

        ERR     ErrCheckPage        ( CPRINTF * const     pcprintf,
                                      const CheckPageMode mode = OnErrorReturnError,
                                      const CheckPageExtensiveness grbitExtensiveCheck = CheckDefault ) const;

        //  prepares a page to be written, setting checksum, flush type, etc.

        VOID    PreparePageForWrite( _In_ const CPAGE::PageFlushType pgft, 
                                     _In_ const BOOL fSkipFlushType = fFalse,
                                     _In_ const BOOL fSkipFMPCheck = fFalse );

        //  debugging methods
#if defined( DEBUG ) || defined( ENABLE_JET_UNIT_TEST )
        // Picked 9, b/c it's the 9th field.
        #define ipgfldCorruptItagMicFree   (9)
        VOID    CorruptHdr          ( _In_ const ULONG ipgfld, const QWORD qwToAdd );
        VOID    CorruptTag          ( _In_ const ULONG itag, BOOL fCorruptCb /* vs. tag's Ib */, const USHORT usToAdd );
#endif // DEBUG || ENABLE_JET_UNIT_TEST

#ifdef DEBUG
        VOID    AssertValid         ( ) const;
        VOID    DebugCheckAll_      ( ) const;      // check the ordering etc. of the tags
        VOID    DebugCheckAll       ( ) const
        {
#ifndef DEBUG_PAGE
            if ( ( rand() % 100 ) == 42 )   // 1% of operations
#endif // !DEBUG_PAGE
            {
                DebugCheckAll_();
            }
        }
        BOOL    FAssertDirty        ( ) const;
        BOOL    FAssertReadLatch    ( ) const;
        BOOL    FAssertRDWLatch     ( ) const;
        BOOL    FAssertWriteLatch   ( ) const;
        BOOL    FAssertWARLatch     ( ) const;
#else
        INLINE VOID DebugCheckAll       ( ) const   {}  //  do nothing
#endif // DEBUG
        struct PGHDR;
        struct PGHDR2;
        typedef INT (*PFNVISITNODE)( const CPAGE::PGHDR * const ppghdr, INT itag, DWORD fNodeFlags, const KEYDATAFLAGS * const pkdf, void * pvCtx );

        VOID    DumpAllocMap_   ( _TCHAR * rgchBuf, CPRINTF * pcprintf ) const;
        ERR     DumpAllocMap    ( CPRINTF * pcprintf ) const;
        ERR     DumpTags        ( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
        VOID    DumpTag         ( CPRINTF * pcprintf, const INT itag, const DWORD_PTR dwOffset ) const;
        ERR     DumpHeader      ( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
        ERR     ErrDumpToIrsRaw ( _In_ PCWSTR wszReason, _In_ PCWSTR wszDetails ) const;
        ERR     ErrEnumTags     ( PFNVISITNODE pfnEval, void * pvCtx ) const;

        NOINLINE ERR     ErrCaptureCorruptedPageInfo_( const CheckPageMode mode, const PCWSTR szCorruptionType, const PCWSTR szCorruptionDetails, const CHAR * const szFile, const LONG lLine, const BOOL fLogEvent = fTrue ) const;
        NOINLINE ERR     ErrCaptureCorruptedPageInfo_( const CheckPageMode mode, const PCSTR szCorruptionType, const PCWSTR wszCorruptionDetails, const CHAR * const szFile, const LONG lLine, const BOOL fLogEvent = fTrue ) const;

#ifdef DEBUGGER_EXTENSION
        // Dump members of CPAGE class
        void    Dump ( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;

        friend LOCAL VOID ::EDBGUpdatePgCounts(
            TREEINFO * const    rgtreeinfo,
            const BF * const    pbf,
            const CPAGE * const pcpage );

        friend ERR ErrBFPageElemFromStruct( size_t eBFMember, size_t cbUnused, BYTE* pvDebuggee, BYTE* pv, ULONGLONG * pullValue );

#endif  //  DEBUGGER_EXTENSION

        //  internal test routine
        ERR     ErrTest         ( _In_ const ULONG cbPageSize );

        //  Page Hint Cache

        static ERR ErrGetPageHintCacheSize( ULONG_PTR* const pcbPageHintCache );
        static ERR ErrSetPageHintCacheSize( const ULONG_PTR cbPageHintCache );

        //  module init / term

        static ERR  ErrInit();
        static VOID Term();

    public:

    //  ****************************************************************
    //  Page Level Flags
    //  ****************************************************************

        //  where we are in the BTree
        enum    { fPageRoot                 = 0x0001    };
        enum    { fPageLeaf                 = 0x0002    };
        enum    { fPageParentOfLeaf         = 0x0004    };

        //  special flags
        enum    { fPageEmpty                = 0x0008    };
        enum    { fPageRepair               = 0x0010    };

        //  what type of tree we are in
        enum    { fPagePrimary              = 0x0000    };
        enum    { fPageSpaceTree            = 0x0020    };
        enum    { fPageIndex                = 0x0040    };
        enum    { fPageLongValue            = 0x0080    };
        // enum { fPageSLVAvail             = 0x0100    }; // obsolete
        // enum { fPageSLVOwnerMap          = 0x0200    }; // obsolete

        //  type of BTree key validation
        enum    { fPageNonUniqueKeys        = 0x0400    };

        //  upgrade info
        enum    { fPageNewRecordFormat      = 0x0800    };

        //  the ECC checksum flag is only used by the checksum
        //  generation and verification code. don't change
        //  this flag without updating the checksum code as
        //  there are not accessor functions for it

        enum    { fPageNewChecksumFormat    = 0x2000    };

        //  this flag is set when the page is scrubbed and reset
        //  on any other dirties

        enum    { fPageScrubbed             = 0x4000    };

        //  technically, this is not a flag but rather two bits to
        //  represent the flush type of the page.

        enum    { maskFlushType             = 0x18000 };

        //  pre-initialized page, i.e., a page that was new'd, but not consumed
        //  yet. Should have a header with a fresh dbtime and objid, but nothing else
        //  (all 0s in its data portion).

        enum    { fPagePreInit              = 0x20000 };

        //  this is the logical inference we can make on the flush type
        //      state about a given page.
        //   - for the page state it is always RockWrite, PaperWrite or ScissorsWrite.
        //   - for the flush map it is Unknown or one of the known types above.
        //  A known page state is not considered a mismatch with an unknown flush map state.

        enum PageFlushType
        {
                pgftUnknown = 0,
                pgftRockWrite = 1,
                pgftPaperWrite = 2,
                pgftScissorsWrite = 3,
                pgftMax
        };
        // The previous line needs to be "enum PageFlushType : ULONG;" to be explicit for some
        // compilers, but let's compile this way for now to prove that it actually IS a ULONG.
        C_ASSERT( sizeof( PageFlushType ) == sizeof( ULONG ) );

        INLINE static PageFlushType PgftGetNextFlushType( const PageFlushType pgft )
        {
            Assert( pgft < CPAGE::pgftMax );
            return (PageFlushType)( ( pgft % ( CPAGE::pgftMax - 1 ) ) + 1 );
        };


    //  ****************************************************************
    //  PRIVATE DECLARATIONS
    //  ****************************************************************

    private:
        //  hidden, not implemented
        CPAGE   ( const CPAGE& );

        //  BF latch flags used to pre-initialize pages
        #define bflfPreInitLatchFlags ( bflfNewIfUncached | bflfNoFaultFail | bflfUninitPageOk )

    public:
        //  forward declarations
        class TAG;

    private:
        //  friend classes for testing
        friend class CPageTestFixture;
        
    private:
        //  get/insert/replace implementations
        template< PageNodeBoundsChecking pgnbc >
        VOID GetPtr_            ( INT itag, LINE * pline, _Out_opt_ ERR * perrNoEnforce = NULL ) const;
        VOID ReplaceFlags_      ( INT iline, INT fFlags );
        VOID Delete_            ( INT itag );
        VOID Replace_           ( INT itag, const DATA * rgdata, INT cdata, INT fFlags );
        VOID Insert_            ( INT itag, const DATA * rgdata, INT cdata, INT fFlags );

    private:
        //  auxiliary methods
        VOID    Abandon_        ( );
        INT     CbDataTotal_    ( const DATA * rgdata, INT cdata ) const;

        VOID    Dirty_          ( const BFDirtyFlags bfdf );
        VOID    ReorganizeData_ ( __in_range( reorgOther, reorgMax - 1 ) const CPAGEREORG reorgReason );
        VOID    UpdateDBTime_   ( );
        VOID    CoordinateDBTime_( const DBTIME dbtime );
        VOID    CopyData_       ( TAG * ptag, const DATA * rgdata, INT cdata );
        VOID    ZeroOutGaps_    ( const CHAR chZero );
        INLINE BOOL FRuntimeScrubbingEnabled_ ( ) const;

#ifdef ENABLE_JET_UNIT_TEST
        VOID    SetRuntimeScrubbingEnabled_( const BOOL fEnabled );
#endif // ENABLE_JET_UNIT_TEST

#ifdef ENABLE_JET_UNIT_TEST
    public:
#endif
        TAG *   PtagFromItag_   ( INT itag ) const;
    private:
#ifdef DEBUG
        INLINE CPAGE::TAG * CPAGE::PtagFromRgbCbItag_( BYTE *rgbPage, INT cbPage, INT itag ) const;
#endif
        ULONG   CbTagArray_     ( ) const;
        ULONG_PTR PbDataStart_  ( _In_ const BOOL fSmallPageCached ) const;
        ULONG_PTR PbDataStart_  () const;
        ULONG_PTR PbDataEnd_    ( _In_ const ULONG cbBufferCached ) const;
        ULONG_PTR PbDataEnd_    () const;

        BYTE *  PbFromIb_       ( USHORT ib ) const;

        VOID    FreeSpace_      ( _In_ const INT cb );

        //  scrubbing flags
        VOID    SetFScrubbedValue_  ( const BOOL fValue );
        VOID    SetFScrubbed_       ( );
        VOID    ResetFScrubbed_     ( );

        //  validate the checksum, fix ECC errors, log warnings and errors

        ERR     ErrCheckForUninitializedPage_( IPageValidationAction *  const pvalidationaction ) const;
        ERR     ErrReadVerifyFailure_() const;
        VOID    ReportReadLostFlushVerifyFailure_(
                    IPageValidationAction * const pvalidationaction,
                    const FMPGNO fmpgno,
                    const PageFlushType pgftExpected,
                    const BOOL fUninitializedPage,
                    const BOOL fRuntime,
                    const BOOL fFailOnRuntimeOnly ) const;

        //  sets the flush type on the page

        VOID    SetFlushType_( _In_ const CPAGE::PageFlushType pgft );

        //  data checksumming help
        
        static XECHECKSUM UpdateLoggedDataChecksum_(
                            const XECHECKSUM checksumLoggedData,
            __in_bcount(cb) const BYTE * const pb,
                            const INT cb );

        //  Sizing functions
        //
        //  Much caution must be taken when utilizing functions relating to the sizing
        //  of things in CPAGE, as we have both a current buffer size and a natural
        //  page size.  There are several sizing functions that have both a page and 
        //  buffer sized counterpart, so that various conditions can be checked whether
        //  the page is hydrated or dehydrated.

        //  All buffer-based sizing functions should be private, as all consumers of 
        //  CPAGE (node, bt, space, etc) should only know of and based decisions on a 
        //  page's natural size.

        //  The page header cbFree is based upon the current buffer size, so internally
        //  this should be treated as a buffer based sizing element.  So externally we
        //  provide a CPAGE::CbPageFree() to account for the true page's free size.

    public:
        INLINE ULONG CbPage             ( ) const;
#if !defined(DEBUG) && !defined(ENABLE_JET_UNIT_TEST)
    private:
#endif
        ULONG CbBuffer                  ( ) const;
        
    private:

        INLINE USHORT CbAdjustForPage   ( ) const;

        INT     CbContiguousBufferFree_ ( ) const;
        INT     CbContiguousFree_       ( ) const;

        USHORT  CbFree_                 ( ) const;
        //above in public accessors is corresponding CbPageFree()

        INLINE INT CbBufferData         ( ) const;
        INLINE INT CbPageData           ( ) const;

        //  static versions

        INLINE static INT CbPageData( const ULONG cbPageSize ) { return cbPageSize - CbPageHeader( cbPageSize ); };

#ifdef DEBUG
    private:
        BOOL    FAssertUnused_      ( ) const;
        BOOL    FAssertNotOnPage_   ( const VOID * pv ) const;
        VOID    DebugMoveMemory_    ( );
#endif  //  DEBUG

    //  ****************************************************************
    //  persisted sub-page structures
    //  ****************************************************************

    public:

#include <pshpack1.h>

        //  This is the basic struct of the page.

        //  ================================================================
        //struct PAGE_
        //
        //  sizeof(PAGE_) == "cbPage"
        //
        //  The data array grows forward from rgbData. The rgTag array grows backwards
        //  from rgTag. The first tag is always reserved for the external line tag
        //
        //-
        //  {
        //  PGHDR       pghdr;
        //  BYTE        rgbData[ "cbPage"
        //                          - sizeof( PGHDR )   //  pghdr
        //                          - sizeof( TAG )     //  rgtag
        //                          ];
        //  TAG         rgTag[1];
        //  };

        //  ================================================================
        PERSISTED
        class TAG
        //  ================================================================
        //-
        {
            private:
                const static USHORT mskTagFlags = 0xe000;
                const static USHORT shfTagFlags = 13;

                const static USHORT mskLargeTagIbCb = 0x7fff;
                const static USHORT mskSmallTagIbCb = 0x1fff;

            public:
                TAG();

                static VOID ErrTest( _In_ VOID * const pvBuffer, ULONG cbPageSize );

                INLINE USHORT Cb( _In_ const BOOL fSmallFormat ) const;
                INLINE USHORT Ib( _In_ const BOOL fSmallFormat ) const;
                INLINE USHORT FFlags( _In_ const CPAGE * const pPage, _In_ const BOOL fSmallFormat ) const;
                INLINE VOID SetIb( _In_ CPAGE * const pPage, _In_ USHORT ib );
                INLINE VOID SetCb( _In_ CPAGE * const pPage, _In_ USHORT cb );
                INLINE VOID SetFlags( _In_ CPAGE * const pPage, _In_ USHORT fFlags );

            private:
                LittleEndian<volatile USHORT>   cb_;
                LittleEndian<volatile USHORT>   ib_;

#if defined( DEBUG ) || defined( ENABLE_JET_UNIT_TEST )
            public:
                VOID CorruptCb( _In_ const USHORT usToAdd )
                {
                    const USHORT cbCurr = cb_;
                    cb_ = cbCurr + usToAdd;
                }
                VOID CorruptIb( _In_ const USHORT usToAdd )
                {
                    const USHORT ibCurr = ib_;
                    ib_ = ibCurr + usToAdd;
                }

            private:
#endif // DEBUG || ENABLE_JET_UNIT_TEST
        };  //  class TAG

        //  TAG comparitors
        //

        typedef BOOL (*PfnCmpTAG)( _In_ const CPAGE::TAG * ptag1, _In_ const CPAGE::TAG * ptag2 );

        static CPAGE::PfnCmpTAG PfnCmpPtagIb( const BOOL fSmallFormat );
        static CPAGE::PfnCmpTAG PfnCmpPtagIb( _In_ ULONG cbPage );
        CPAGE::PfnCmpTAG PfnCmpPtagIb() const;

    public:
        //  ================================================================
        PERSISTED
        struct PGHDR
        //  ================================================================
        {
            LittleEndian<XECHECKSUM>    checksum;           //  checksum of the page
            LittleEndian<DBTIME>        dbtimeDirtied;      //  database time dirtied
            LittleEndian<PGNO>          pgnoPrev;           //  previous and next pgno's
            LittleEndian<PGNO>          pgnoNext;
            LittleEndian<OBJID>         objidFDP;           //  objid of tree to which this page belongs - shouldn't change once set
            LittleEndian<USHORT>        cbFree;             //  count of free bytes
            LittleEndian<volatile USHORT>       cbUncommittedFree;  //  free but may be reclaimed by rollback. should always be <= cbFree
            LittleEndian<USHORT>        ibMicFree;          //  index of minimum free byte
            LittleEndian<USHORT>        itagMicFree;        //  index of maximum itag

            LittleEndian<ULONG>         fFlags;
        };

        //  ================================================================
        PERSISTED
        struct PGHDR2
        //  ================================================================
        {
            PGHDR                       pghdr;
            LittleEndian<XECHECKSUM>        rgChecksum[ 3 ];        // XOR-ECC (XE) checksums of the page
            LittleEndian<PGNO>          pgno;               //  this pgno
            CHAR                        rgbReserved[ 12 ];
        };

#include <poppack.h>


    private:
        //  private constants
        enum    { cchDumpAllocRow       = 64                        };
        enum    { ctagReserved          = 1                         };


    //  ****************************************************************
    //  Private Members
    //  ****************************************************************

    private:

        PIB *                   m_ppib;
        DBTIME                  m_dbtimePreInit;
        //  16 / 16 bytes
        IFMP                    m_ifmp;
        PGNO                    m_pgno;
        //  24 / 24 bytes
        BOOL                    m_fSmallFormat;
        OBJID                   m_objidPreInit;
        //  32 / 32 bytes
        BFLatch                 m_bfl;
        //  40 / 48 bytes
        const ILatchManager *   m_platchManager;
        //  44 / 56 bytes
        INT                     m_iRuntimeScrubbingEnabled; //  Technically, only needed when ENABLE_JET_UNIT_TEST is defined, but
                                                            //  we run into stack corruption problems when running unit tests because
                                                            //  they link against libs with mismatching versions of CPAGE (i.e., built
                                                            //  with or without ENABLE_JET_UNIT_TEST).
        //  48 / 64 bytes

    //  ****************************************************************
    //  Global Hint Cache
    //  ****************************************************************

    private:
        //  hint cache
        static  DWORD_PTR *     rgdwHintCache;

    public:
        static  SIZE_T          cbHintCache;
        static  SIZE_T          maskHintCache;

    private:
        const static BOOL fFormatInvalid = -1;
        INLINE BOOL FSmallPageFormat() const { Assert( fFormatInvalid != m_fSmallFormat );  return m_fSmallFormat; }

    public:

    //  ****************************************************************
    //  PUBLIC CONSTANTS
    //  ****************************************************************

        enum { cbInsertionOverhead = sizeof( CPAGE::TAG ) };
                                        // 4
        INLINE static ULONG CbPageDataMax( const ULONG cbPageSize ) { return (ULONG)(
                    CbPageData( cbPageSize )
                    - ( (ULONG)sizeof( TAG ) * ctagReserved )
                    - cbInsertionOverhead ); };
        INLINE ULONG CbPageDataMax() const { return (ULONG)(
                    CbPageData()
                    - ( (ULONG)sizeof( TAG ) * ctagReserved )
                    - cbInsertionOverhead ); };
                                        // 4056 - ( 4 * 1 ) - 4 = 4048
        INLINE static ULONG CbPageDataMaxNoInsert( const ULONG cbPageSize ) { return ( CbPageData( cbPageSize ) - ( sizeof( TAG ) * ctagReserved ) ); };
                                        // 4056 - ( 4 * 1 ) = 4052
        INLINE ULONG CbPageDataMaxNoInsert() const { return ( CbPageData() - ( sizeof( TAG ) * ctagReserved ) ); };
                                        // 4056 - ( 4 * 1 ) = 4052

        INLINE static USHORT CbPageHeader( const ULONG cbPageSize ) { return FIsSmallPage( cbPageSize ) ? sizeof ( PGHDR ) : sizeof( PGHDR2 ); }
        INLINE USHORT CbPageHeader() const { return FSmallPageFormat() ? sizeof ( PGHDR ) : sizeof( PGHDR2 ); }

};  //  class CPAGE

DEFINE_ENUM_FLAG_OPERATORS_BASIC( CPAGE::CheckPageExtensiveness );

#define LVChunkOverheadCommon       42
#define LVChunkOverheadSmallPage ( LVChunkOverheadCommon + sizeof( CPAGE::PGHDR ) )
#define LVChunkOverheadLargePage ( LVChunkOverheadCommon + sizeof( CPAGE::PGHDR2 ) )

//  forward declaration because upgrade.hxx is included after cpage.hxx

ERR ErrUPGRADEPossiblyConvertPage(
        CPAGE * const pcpage,
        const PGNO pgno,
        VOID * const pvWorkBuf );


//  ****************************************************************
//  PUBLIC INLINE METHODS
//  ****************************************************************

#ifdef DEBUG
//  ================================================================
INLINE BOOL CPAGE::FAssertUnused_( ) const
//  ================================================================
{
    return ppibNil == m_ppib;
}
#endif


//  ================================================================
INLINE INT CPAGE::Clines ( ) const
//  ================================================================
{
    return ((PGHDR*)m_bfl.pv)->itagMicFree - ctagReserved;
}


//  ================================================================
INLINE ULONG CPAGE::FFlags ( ) const
//  ================================================================
{
    return ((PGHDR*)m_bfl.pv)->fFlags;
}


//  ================================================================
INLINE BOOL CPAGE::FLeafPage ( ) const
//  ================================================================
{
    return FFlags() & fPageLeaf;
}


//  ================================================================
INLINE VOID CPAGE::SetFlags( ULONG fFlags )
//  ================================================================
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif

    ((PGHDR*)m_bfl.pv)->fFlags = fFlags;
    Assert( !( FLeafPage() && FParentOfLeaf() ) );
    Assert( !FEmptyPage() );
    
    ASSERT_VALID( this );
}


//  ================================================================
INLINE BOOL CPAGE::FInvisibleSons ( ) const
//  ================================================================
{
    return !FLeafPage() && !FPreInitPage();
}


//  ================================================================
INLINE BOOL CPAGE::FRootPage ( ) const
//  ================================================================
{
    return FFlags() & fPageRoot;
}


//  ================================================================
INLINE BOOL CPAGE::FFDPPage ( ) const
//  ================================================================
{
    return ( FRootPage() && !FSpaceTree() );
}


//  ================================================================
INLINE BOOL CPAGE::FEmptyPage ( ) const
//  ================================================================
{
    return FFlags() & fPageEmpty;
}


//  ================================================================
INLINE BOOL CPAGE::FPreInitPage ( ) const
//  ================================================================
{
    return FFlags() & fPagePreInit;
}


//  ================================================================
INLINE BOOL CPAGE::FParentOfLeaf ( ) const
//  ================================================================
{
    return FFlags() & fPageParentOfLeaf;
}


//  ================================================================
INLINE BOOL CPAGE::FSpaceTree ( ) const
//  ================================================================
{
    return FFlags() & fPageSpaceTree;
}


//  ================================================================
INLINE BOOL CPAGE::FScrubbed ( ) const
//  ================================================================
{
    return FFlags() & fPageScrubbed;
}


//  ================================================================
INLINE BOOL CPAGE::FRepairedPage ( ) const
//  ================================================================
{
    return FFlags() & fPageRepair;
}


//  ================================================================
INLINE BOOL CPAGE::FPrimaryPage ( ) const
//  ================================================================
{
    return !FIndexPage() && !FPreInitPage();
}


//  ================================================================
INLINE BOOL CPAGE::FIndexPage ( ) const
//  ================================================================
{
    return FFlags() & fPageIndex;
}


//  ================================================================
INLINE BOOL CPAGE::FNonUniqueKeys ( ) const
//  ================================================================
{
    //  non-unique btree's only valid on secondary indexes
    Assert( !( FFlags() & fPageNonUniqueKeys ) || FIndexPage() );
    return FFlags() & fPageNonUniqueKeys;
}


//  ================================================================
INLINE BOOL CPAGE::FLongValuePage ( ) const
//  ================================================================
{
    return FFlags() & fPageLongValue;
}


//  ================================================================
INLINE BOOL CPAGE::FNewRecordFormat ( ) const
//  ================================================================
{
    return FFlags() & fPageNewRecordFormat;
}


//  ================================================================
INLINE BOOL CPAGE::FNewChecksumFormat ( ) const
//  ================================================================
{
    return FFlags() & fPageNewChecksumFormat;
}

//  ================================================================
INLINE CPAGE::PageFlushType CPAGE::Pgft( ) const
//  ================================================================
{
    return (CPAGE::PageFlushType)( ( FFlags() & maskFlushType ) >> 15 );
}

//  ================================================================
INLINE IFMP CPAGE::Ifmp ( ) const
//  ================================================================
{
    return m_ifmp;
}


//  ================================================================
INLINE OBJID CPAGE::ObjidFDP ( ) const
//  ================================================================
{
    return ((PGHDR*)m_bfl.pv)->objidFDP;
}


//  ================================================================
INLINE PGNO CPAGE::PgnoThis ( ) const
//  ================================================================
{
    if ( FSmallPageFormat() )
    {
        //  Do not have PGNO on small pages (well technically we did pre-Vista / Win2k8),
        //  so pull it from the in memory CPAGE structure.
        return m_pgno;
    }
    else
    {
        Assert( ( m_pgno == ((PGHDR2*)m_bfl.pv)->pgno ) ||
                FNegTest( fCorruptingPagePgnos ) ||
                FUtilZeroed( (BYTE*)m_bfl.pv, CbBuffer() ) ); //  It might happen due to a lost flush, which we'll catch with a different assert.
        return ((PGHDR2*)m_bfl.pv)->pgno;
    }
}

//  ================================================================
INLINE PGNO CPAGE::PgnoREPAIRThis ( ) const
//  ================================================================
{
    Assert( g_fRepair );

    if ( FSmallPageFormat() )
    {
        //  Do not have PGNO on small pages (well technically we did pre-Vista / Win2k8),
        //  so pull it from the in memory CPAGE structure.
        return m_pgno;
    }
    else
    {
        //  Similar to PgnoThis() but designed to avoid asserting when the page has
        //  been zeroed out due to repair "correction" activities.
        Assert( m_pgno == ((PGHDR2*)m_bfl.pv)->pgno ||
                ( g_fRepair && ((PGHDR2*)m_bfl.pv)->pgno == pgnoNull ) );
        return ((PGHDR2*)m_bfl.pv)->pgno;
    }
}

//  ================================================================
INLINE PGNO CPAGE::PgnoNext ( ) const
//  ================================================================
{
    return ((PGHDR*)m_bfl.pv)->pgnoNext;
}


//  ================================================================
INLINE PGNO CPAGE::PgnoPrev ( ) const
//  ================================================================
{
    return ((PGHDR*)m_bfl.pv)->pgnoPrev;
}


//  ================================================================
INLINE DBTIME CPAGE::Dbtime ( ) const
//  ================================================================
{
    return ((PGHDR*)m_bfl.pv)->dbtimeDirtied;
}


//  ================================================================
INLINE USHORT CPAGE::CbUncommittedFree ( ) const
//  ================================================================
{
    return ((PGHDR*)m_bfl.pv)->cbUncommittedFree;
}


//  ================================================================
INLINE VOID * CPAGE::PvBuffer ( ) const
//  ================================================================
{
    return m_bfl.pv;
}


//  ================================================================
template< PageNodeBoundsChecking pgnbc >
INLINE VOID CPAGE::GetPtr( INT iline, LINE * pline, _Out_opt_ ERR * perrNoEnforce ) const
//  ================================================================
//
//  Sets the LINE argument to point to the node data on the page. A
//  further operation that causes a reorganization (Replace or Insert)
//  may invalidate this pointer.
//
//-
{
    Assert( iline >= 0 );
    Assert( pline );
    GetPtr_< pgnbc >( iline + ctagReserved, pline, perrNoEnforce );
}

//  ================================================================
INLINE ERR CPAGE::ErrGetPtr( INT iline, LINE * pline ) const
//  ================================================================
//
//  Sets the LINE argument to point to the node data on the page. A
//  further operation that causes a reorganization (Replace or Insert)
//  may invalidate this pointer.
//
//-
{
    ERR err = JET_errSuccess;
    Assert( iline >= 0 );
    Assert( pline );
    GetPtr< pgnbcChecked >( iline, pline, &err );
    return err;
}

//  ================================================================
template< PageNodeBoundsChecking pgnbc >
INLINE VOID CPAGE::GetPtrExternalHeader( LINE * pline, _Out_opt_ ERR * perrNoEnforce ) const
//  ================================================================
//
//  Gets a pointer to the external header.
//
//-
{
    Assert( pline );
    GetPtr_< pgnbc >( 0, pline, perrNoEnforce );
}

//  ================================================================
INLINE ERR CPAGE::ErrGetPtrExternalHeader( LINE * pline ) const
//  ================================================================
//
//  Gets a pointer to the external header.
//
//-
{
    ERR err = JET_errSuccess;
    Assert( pline );
    GetPtrExternalHeader< pgnbcChecked >( pline, &err );
    return err;
}

//  ================================================================
INLINE ULONG_PTR CPAGE::PbDataStart_( _In_ const BOOL fSmallPageCached ) const
//  ================================================================
//
//  Pointer to the first valid byte that can be used in a line/node of data.
//
//  This is the first byte past the page header (PGHDR | PGHDR2).
//
//-
{
    Assert( FSmallPageFormat() == fSmallPageCached );
    return (ULONG_PTR)m_bfl.pv + ( fSmallPageCached ? sizeof( PGHDR ) : sizeof( PGHDR2 ) );
}

//  ================================================================
INLINE ULONG_PTR CPAGE::PbDataStart_() const
//  ================================================================
{
    return PbDataStart_( FSmallPageFormat() );
}


//  ================================================================
INLINE ULONG_PTR CPAGE::PbDataEnd_( _In_ const ULONG cbBufferCached ) const
//  ================================================================
//
//  Pointer to the last valid byte that can be used in a line/node of data.
//
//  This is a pointer to the byte just before the TAG array starts (i.e. list of ib+cb's - at the end of 
//  the page, that grows down towards header).
//
//-
{
    Assert( CbBuffer() == cbBufferCached ); // we pass cached cbBufferCached for perf.
    return (ULONG_PTR)m_bfl.pv + cbBufferCached - CbTagArray_() - 1;
}

//  ================================================================
INLINE ULONG_PTR CPAGE::PbDataEnd_() const
//  ================================================================
{
    return PbDataEnd_( CbBuffer() );
}


//  ================================================================

// patch file page will start as a normal db page (PGHDR) and will be used as an empty page
// (pgno == 0)
#include <pshpack1.h>

PERSISTED
struct PATCHHDR                                                 //  PATCHHDR -- Formatted Patch Page Header
{
    CPAGE::PGHDR                    m_hdrNormalPage;

    BKINFO                          bkinfo;                     // the backup information stored in the patch page
    SIGNATURE                       signDb;                     //  (28 bytes) signature of the db (incl. creation time)
    SIGNATURE                       signLog;                    //  log signature
    UnalignedLittleEndian< ULONG >  lgenMinReq;                 //  databases's min lgen required, captured at the end of backup
    UnalignedLittleEndian< ULONG >  lgenMaxReq;                 //  databases's max lgen required, captured at the end of backup
    UnalignedLittleEndian< ULONG >  lgenMaxCommitted;           //  databases's max lgen committed, captured at the end of backup
    LOGTIME                         logtimeGenMaxRequired;      //  creation time of lgenMaxReq
    LOGTIME                         logtimeGenMaxCommitted;     //  creation time of lgenMaxCommitted
};

C_ASSERT( sizeof( PATCHHDR ) <= g_cbPageMin );

#include <poppack.h>

//  ================================================================

#define ErrCaptureCorruptedPageInfo( mode, wszCorruptionType )                                   ErrCaptureCorruptedPageInfo_( mode, wszCorruptionType, NULL, __FILE__, __LINE__ )
#define ErrCaptureCorruptedPageInfoSz( mode, wszCorruptionType, wszCorruptionDetails, fLogEvent ) ErrCaptureCorruptedPageInfo_( mode, wszCorruptionType, wszCorruptionDetails, __FILE__, __LINE__, fLogEvent )
#define PageEnforceSz( cpage, exp, wszType, wszDetails )      ( ( exp ) ? (void) 0 : cpage.ErrCaptureCorruptedPageInfoSz( CPAGE::CheckPageMode::OnErrorEnforce, wszType, wszDetails, fTrue ) )
#define PageEnforce( cpage, exp )   PageEnforceSz( cpage, exp, L#exp, NULL )
// Notes on PageAssertTrack():
//  1. Since we are not passing OnErrorFireWall we will not have an HA failure item, so no drastic action will be taken.
//  2. Double logs as AssertTrack assert / Event 901, and as DATABASE_PAGE_LOGICALLY_CORRUPT_ID in "capture".  Do we care?
#define PageAssertTrack( cpage, exp, szType, ... )                            \
    {                                                                          \
    const bool fExp = ( exp );                                                  \
    if ( !fExp )                                                                 \
    {                                                                             \
        (cpage).ErrCaptureCorruptedPageInfo( CPAGE::OnErrorReturnError, szType );  \
        AssertTrackSz( fExp, szType, __VA_ARGS__ );                                 \
    }                                                                                \
    }
