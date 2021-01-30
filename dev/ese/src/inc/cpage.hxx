// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.



#ifdef DEBUG





#define DBGSZ(sz)   _tprintf( _T( "%s" ), sz )

#endif





enum CPAGEREORG
{
    reorgOther = 0,
    reorgFreeSpaceRequest,
    reorgPageMoveLogging,
    reorgDehydrateBuffer,
    reorgMax
};


class IPageValidationAction
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

class CPageValidationNullAction : public IPageValidationAction
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


struct TREEINFO;
class CPAGE;
LOCAL VOID EDBGUpdatePgCounts(
    TREEINFO * const    rgtreeinfo,
    const BF * const    pbf,
    const CPAGE * const pcpage );

#endif

class CPageValidationLogEvent : public IPageValidationAction
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

enum PAGEValidationFlags
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

class CPAGE
{
    public:
        CPAGE   ( );
        ~CPAGE  ( );
        CPAGE& operator=    ( CPAGE& );
        static ERR ErrResetHeader   ( PIB *ppib, const IFMP ifmp, const PGNO pgno );

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

        VOID ConsumePreInitPage( const ULONG fPageFlags );

        VOID FinalizePreInitPage( OnDebug( const BOOL fCheckPreInitPage ) );

        VOID GetShrunkPage          (   const IFMP ifmp,
                                        const PGNO pgno,
                                        VOID * const pv,
                                        const ULONG cb );

        VOID GetRevertedNewPage     (   const PGNO pgno,
                                        VOID * const pv,
                                        const ULONG cb );

        ERR ErrLoadPage             (   PIB * ppib,
                                        IFMP ifmp,
                                        PGNO pgno,
                                        const VOID * pv,
                                        const ULONG cb );

        VOID LoadNewPage            (   const IFMP ifmp,
                                        const PGNO pgno,
                                        const OBJID objidFDP,
                                        const ULONG fFlags,
                                        VOID * const pv,
                                        const ULONG cb );

#ifdef ENABLE_JET_UNIT_TEST
        VOID LoadNewTestPage( __in const ULONG cb, __in const IFMP ifmp = ifmpNil );
#endif

        VOID LoadPage( const IFMP ifmp, const PGNO pgno, VOID * const pv, const ULONG cb );
        VOID LoadDehydratedPage( const IFMP ifmp, const PGNO pgno, VOID * const pv, const ULONG cb, const ULONG cbPage );
        VOID LoadPage( VOID * const pv, const ULONG cb );
        VOID ReBufferPage( __in const BFLatch& bfl, const IFMP ifmp, const PGNO pgno, VOID * const pv, const ULONG cb );
        
        VOID UnloadPage( );
        VOID MarkAsSuperCold( );

        VOID PreInitializeNewPage_(    PIB * const ppib,
                                    const IFMP ifmp,
                                    const PGNO pgno,
                                    const OBJID objidFDP,
                                    const DBTIME dbtime );

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

        template< PageNodeBoundsChecking pgnbc = pgnbcNoChecks >
        VOID GetPtrExternalHeader   ( LINE * pline, _Out_opt_ ERR * perrNoEnforce = NULL ) const;
        VOID SetExternalHeader( const DATA * rgdata, INT cdata, INT fFlags );

        template< PageNodeBoundsChecking pgnbc = pgnbcNoChecks >
        VOID GetPtr             ( INT iline, LINE * pline, _Out_opt_ ERR * perrNoEnforce = NULL ) const;

        ERR  ErrGetPtr               ( INT iline, LINE * pline ) const;
        ERR  ErrGetPtrExternalHeader ( LINE * pline ) const;

        VOID GetLatchHint       ( INT iline, BFLatch** ppbfl ) const;

        VOID ReplaceFlags       ( INT iline, INT fFlags );

        VOID Insert             ( INT iline, const DATA * rgdata, INT cdata, INT fFlags );
        VOID Replace            ( INT iline, const DATA * rgdata, INT cdata, INT fFlags );
        VOID Delete             ( INT iline );

        USHORT  CbPageFree      ( ) const;
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

        BOOL    FShrunkPage ( ) const;
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

        VOID    SetPgno( PGNO pgno );
        VOID    SetPgnoNext ( PGNO pgno );
        VOID    SetPgnoPrev ( PGNO pgno );
        VOID    SetDbtime   ( const DBTIME dbtime );
        VOID    RevertDbtime ( const DBTIME dbtime, const ULONG fFlags );
        VOID    SetFlags    ( ULONG fFlags );
        VOID    ResetParentOfLeaf   ( );
        VOID    SetFEmpty    ( );
        VOID    SetFNewRecordFormat ( );

        VOID    ResetAllFlags( INT fFlags );
        bool    FAnyLineHasFlagSet( INT fFlags ) const;

        VOID    SetCbUncommittedFree    ( INT cbUncommittedFreeNew );
        VOID    AddUncommittedFreed     ( INT cbToAdd );
        VOID    ReclaimUncommittedFreed ( INT cbToReclaim );

        VOID    OverwriteUnusedSpace    ( const CHAR chZero );

        VOID    ReorganizePage(
                    __out const VOID ** pvHeader,
                    __out size_t * const  pcbHeader,
                    __out const VOID ** pvTrailer,
                    __out size_t * const pcbTrailer);

        BOOL        FOnPage( const void * pv, const INT cb ) const
        {
            ULONG_PTR pb = (ULONG_PTR) pv;
            return pb >= (ULONG_PTR)m_bfl.pv &&
                    pb < ( (ULONG_PTR)m_bfl.pv + CbBuffer() ) &&
                    ( cb == 0 || ( ( pb + cb ) >= (ULONG_PTR)m_bfl.pv ) ) &&
                    ( cb == 0 || ( ( pb + cb ) <= ( (ULONG_PTR)m_bfl.pv + CbBuffer() ) ) );
        }

        BOOL        FOnData( _In_ const void * pv, _In_ const INT cb ) const
        {
            Assert( cb >= 1 );

            const ULONG_PTR pbFirstByte = (ULONG_PTR) pv;
            const ULONG_PTR pbLastByte = pbFirstByte + cb - 1;
            Assert( pbFirstByte <= pbLastByte );

            const ULONG_PTR pbPageDataStart = PbDataStart_( FSmallPageFormat() );
            const ULONG_PTR pbPageDataEnd = PbDataEnd_( CbBuffer() );

            Assert( pbPageDataStart < pbPageDataEnd || FNegTest( fCorruptingPageLogically ) );

            return pbFirstByte >= pbPageDataStart &&
                    pbFirstByte < pbPageDataEnd &&
                    pbLastByte >= pbPageDataStart &&
                    pbLastByte <= pbPageDataEnd;
        }

        PAGECHECKSUM    LoggedDataChecksum() const;


        ULONG   CbContiguousDehydrateRequired_() const;
        ULONG   CbReorganizedDehydrateRequired_() const;
        VOID    DehydratePageUnadjusted_( __in const ULONG cbNewSize );
    public:
        VOID    OptimizeInternalFragmentation();
        BOOL    FPageIsDehydratable( __out ULONG * pcbMinimumSize, __in const BOOL fAllowReorg = fTrue ) const;
        VOID    DehydratePage( __in const ULONG cbNewSize, __in const BOOL fAllowReorg );
        VOID    RehydratePage();
        

        static BOOL FPageIsInitialized( __in const void* const pv, __in ULONG cb );
        BOOL    FPageIsInitialized() const;

        ERR     ErrValidatePage(
            __in const PAGEValidationFlags pgvf,
            __in IPageValidationAction * const pvalidationaction,
            __in CFlushMap* pflushmap = NULL );

        enum CheckPageMode
        {
            OnErrorReturnError          = 0,
            OnErrorFireWall             = 1,
            OnErrorEnforce              = 2,
        };

        enum CheckPageExtensiveness
        {
            CheckBasics                 =   0x0,
            CheckTagsNonOverlapping     =   0x1,
            CheckLineBoundedByTag       =   0x2,
            CheckLinesInOrder           =   0x4,

            CheckDefault                = ( CheckTagsNonOverlapping | CheckLineBoundedByTag ),
            CheckAll                    = ( CheckTagsNonOverlapping | CheckLineBoundedByTag | CheckLinesInOrder ),
        };

        ERR     ErrCheckPage        ( CPRINTF * const     pcprintf,
                                      const CheckPageMode mode = OnErrorReturnError,
                                      const CheckPageExtensiveness grbitExtensiveCheck = CheckDefault ) const;


        VOID    PreparePageForWrite( __in const CPAGE::PageFlushType pgft, 
                                     __in const BOOL fSkipFlushType = fFalse,
                                     __in const BOOL fSkipFMPCheck = fFalse );

#if defined( DEBUG ) || defined( ENABLE_JET_UNIT_TEST )
        #define ipgfldCorruptItagMicFree   (9)
        VOID    CorruptHdr          ( _In_ const ULONG ipgfld, const QWORD qwToAdd );
        VOID    CorruptTag          ( _In_ const ULONG itag, BOOL fCorruptCb , const USHORT usToAdd );
#endif

#ifdef DEBUG
        VOID    AssertValid         ( ) const;
        VOID    DebugCheckAll_      ( ) const;
        VOID    DebugCheckAll       ( ) const
        {
#ifndef DEBUG_PAGE
            if ( ( rand() % 100 ) == 42 )
#endif
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
        INLINE VOID DebugCheckAll       ( ) const   {}
#endif
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
        void    Dump ( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;

        friend LOCAL VOID ::EDBGUpdatePgCounts(
            TREEINFO * const    rgtreeinfo,
            const BF * const    pbf,
            const CPAGE * const pcpage );

        friend ERR ErrBFPageElemFromStruct( size_t eBFMember, size_t cbUnused, BYTE* pvDebuggee, BYTE* pv, ULONGLONG * pullValue );

#endif

        ERR     ErrTest         ( __in const ULONG cbPageSize );


        static ERR ErrGetPageHintCacheSize( ULONG_PTR* const pcbPageHintCache );
        static ERR ErrSetPageHintCacheSize( const ULONG_PTR cbPageHintCache );


        static ERR  ErrInit();
        static VOID Term();

    public:


        enum    { fPageRoot                 = 0x0001    };
        enum    { fPageLeaf                 = 0x0002    };
        enum    { fPageParentOfLeaf         = 0x0004    };

        enum    { fPageEmpty                = 0x0008    };
        enum    { fPageRepair               = 0x0010    };

        enum    { fPagePrimary              = 0x0000    };
        enum    { fPageSpaceTree            = 0x0020    };
        enum    { fPageIndex                = 0x0040    };
        enum    { fPageLongValue            = 0x0080    };

        enum    { fPageNonUniqueKeys        = 0x0400    };

        enum    { fPageNewRecordFormat      = 0x0800    };


        enum    { fPageNewChecksumFormat    = 0x2000    };


        enum    { fPageScrubbed             = 0x4000    };


        enum    { maskFlushType             = 0x18000 };


        enum    { fPagePreInit              = 0x20000 };


        enum PageFlushType
        {
                pgftUnknown = 0,
                pgftRockWrite = 1,
                pgftPaperWrite = 2,
                pgftScissorsWrite = 3,
                pgftMax
        };

        INLINE static PageFlushType PgftGetNextFlushType( const PageFlushType pgft )
        {
            Assert( pgft < CPAGE::pgftMax );
            return (PageFlushType)( ( pgft % ( CPAGE::pgftMax - 1 ) ) + 1 );
        };



    private:
        CPAGE   ( const CPAGE& );

        #define bflfPreInitLatchFlags ( bflfNewIfUncached | bflfNoFaultFail | bflfUninitPageOk )

    public:
        class TAG;

    private:
        friend class CPageTestFixture;
        
    private:
        template< PageNodeBoundsChecking pgnbc >
        VOID GetPtr_            ( INT itag, LINE * pline, _Out_opt_ ERR * perrNoEnforce = NULL ) const;
        VOID ReplaceFlags_      ( INT iline, INT fFlags );
        VOID Delete_            ( INT itag );
        VOID Replace_           ( INT itag, const DATA * rgdata, INT cdata, INT fFlags );
        VOID Insert_            ( INT itag, const DATA * rgdata, INT cdata, INT fFlags );

    private:
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
#endif

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

        VOID    FreeSpace_      ( __in const INT cb );

        VOID    SetFScrubbedValue_  ( const BOOL fValue );
        VOID    SetFScrubbed_       ( );
        VOID    ResetFScrubbed_     ( );


        ERR     ErrCheckForUninitializedPage_( IPageValidationAction *  const pvalidationaction ) const;
        ERR     ErrReadVerifyFailure_() const;
        VOID    ReportReadLostFlushVerifyFailure_(
                    IPageValidationAction * const pvalidationaction,
                    const FMPGNO fmpgno,
                    const PageFlushType pgftExpected,
                    const BOOL fUninitializedPage,
                    const BOOL fRuntime,
                    const BOOL fFailOnRuntimeOnly ) const;


        VOID    SetFlushType_( __in const CPAGE::PageFlushType pgft );

        
        static XECHECKSUM UpdateLoggedDataChecksum_(
                            const XECHECKSUM checksumLoggedData,
            __in_bcount(cb) const BYTE * const pb,
                            const INT cb );




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

        INLINE INT CbBufferData         ( ) const;
        INLINE INT CbPageData           ( ) const;


        INLINE static INT CbPageData( const ULONG cbPageSize ) { return cbPageSize - CbPageHeader( cbPageSize ); };

#ifdef DEBUG
    private:
        BOOL    FAssertUnused_      ( ) const;
        BOOL    FAssertNotOnPage_   ( const VOID * pv ) const;
        VOID    DebugMoveMemory_    ( );
#endif


    public:

#include <pshpack1.h>



        PERSISTED
        class TAG
        {
            private:
                const static USHORT mskTagFlags = 0xe000;
                const static USHORT shfTagFlags = 13;

                const static USHORT mskLargeTagIbCb = 0x7fff;
                const static USHORT mskSmallTagIbCb = 0x1fff;

            public:
                TAG();

                static VOID ErrTest( __in VOID * const pvBuffer, ULONG cbPageSize );

                INLINE USHORT Cb( __in const BOOL fSmallFormat ) const;
                INLINE USHORT Ib( __in const BOOL fSmallFormat ) const;
                INLINE USHORT FFlags( __in const CPAGE * const pPage, __in const BOOL fSmallFormat ) const;
                INLINE VOID SetIb( __in CPAGE * const pPage, __in USHORT ib );
                INLINE VOID SetCb( __in CPAGE * const pPage, __in USHORT cb );
                INLINE VOID SetFlags( __in CPAGE * const pPage, __in USHORT fFlags );

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
#endif
        };


        typedef BOOL (*PfnCmpTAG)( __in const CPAGE::TAG * ptag1, __in const CPAGE::TAG * ptag2 );

        static CPAGE::PfnCmpTAG PfnCmpPtagIb( const BOOL fSmallFormat );
        static CPAGE::PfnCmpTAG PfnCmpPtagIb( __in ULONG cbPage );
        CPAGE::PfnCmpTAG PfnCmpPtagIb() const;

    public:
        PERSISTED
        struct PGHDR
        {
            LittleEndian<XECHECKSUM>    checksum;
            LittleEndian<DBTIME>        dbtimeDirtied;
            LittleEndian<PGNO>          pgnoPrev;
            LittleEndian<PGNO>          pgnoNext;
            LittleEndian<OBJID>         objidFDP;
            LittleEndian<USHORT>        cbFree;
            LittleEndian<volatile USHORT>       cbUncommittedFree;
            LittleEndian<USHORT>        ibMicFree;
            LittleEndian<USHORT>        itagMicFree;

            LittleEndian<ULONG>         fFlags;
        };

        PERSISTED
        struct PGHDR2
        {
            PGHDR                       pghdr;
            LittleEndian<XECHECKSUM>        rgChecksum[ 3 ];
            LittleEndian<PGNO>          pgno;
            CHAR                        rgbReserved[ 12 ];
        };

#include <poppack.h>


    private:
        enum    { cchDumpAllocRow       = 64                        };
        enum    { ctagReserved          = 1                         };



    private:

        PIB *                   m_ppib;
        DBTIME                  m_dbtimePreInit;
        IFMP                    m_ifmp;
        PGNO                    m_pgno;
        BOOL                    m_fSmallFormat;
        OBJID                   m_objidPreInit;
        BFLatch                 m_bfl;
        const ILatchManager *   m_platchManager;
        INT                     m_iRuntimeScrubbingEnabled;


    private:
        static  DWORD_PTR *     rgdwHintCache;

    public:
        static  SIZE_T          cbHintCache;
        static  SIZE_T          maskHintCache;

    private:
        const static BOOL fFormatInvalid = -1;
        INLINE BOOL FSmallPageFormat() const { Assert( fFormatInvalid != m_fSmallFormat );  return m_fSmallFormat; }

    public:


        enum { cbInsertionOverhead = sizeof( CPAGE::TAG ) };
        INLINE static ULONG CbPageDataMax( const ULONG cbPageSize ) { return (ULONG)(
                    CbPageData( cbPageSize )
                    - ( (ULONG)sizeof( TAG ) * ctagReserved )
                    - cbInsertionOverhead ); };
        INLINE ULONG CbPageDataMax() const { return (ULONG)(
                    CbPageData()
                    - ( (ULONG)sizeof( TAG ) * ctagReserved )
                    - cbInsertionOverhead ); };
        INLINE static ULONG CbPageDataMaxNoInsert( const ULONG cbPageSize ) { return ( CbPageData( cbPageSize ) - ( sizeof( TAG ) * ctagReserved ) ); };
        INLINE ULONG CbPageDataMaxNoInsert() const { return ( CbPageData() - ( sizeof( TAG ) * ctagReserved ) ); };

        INLINE static USHORT CbPageHeader( const ULONG cbPageSize ) { return FIsSmallPage( cbPageSize ) ? sizeof ( PGHDR ) : sizeof( PGHDR2 ); }
        INLINE USHORT CbPageHeader() const { return FSmallPageFormat() ? sizeof ( PGHDR ) : sizeof( PGHDR2 ); }

};

DEFINE_ENUM_FLAG_OPERATORS_BASIC( CPAGE::CheckPageExtensiveness );

#define LVChunkOverheadCommon       42
#define LVChunkOverheadSmallPage ( LVChunkOverheadCommon + sizeof( CPAGE::PGHDR ) )
#define LVChunkOverheadLargePage ( LVChunkOverheadCommon + sizeof( CPAGE::PGHDR2 ) )


ERR ErrUPGRADEPossiblyConvertPage(
        CPAGE * const pcpage,
        const PGNO pgno,
        VOID * const pvWorkBuf );



#ifdef DEBUG
INLINE BOOL CPAGE::FAssertUnused_( ) const
{
    return ppibNil == m_ppib;
}
#endif


INLINE INT CPAGE::Clines ( ) const
{
    return ((PGHDR*)m_bfl.pv)->itagMicFree - ctagReserved;
}


INLINE ULONG CPAGE::FFlags ( ) const
{
    return ((PGHDR*)m_bfl.pv)->fFlags;
}


INLINE BOOL CPAGE::FLeafPage ( ) const
{
    return FFlags() & fPageLeaf;
}


INLINE VOID CPAGE::SetFlags( ULONG fFlags )
{
#ifdef DEBUG_PAGE
    ASSERT_VALID( this );
#endif

    ((PGHDR*)m_bfl.pv)->fFlags = fFlags;
    Assert( !( FLeafPage() && FParentOfLeaf() ) );
    Assert( !FEmptyPage() );
    
    ASSERT_VALID( this );
}


INLINE BOOL CPAGE::FInvisibleSons ( ) const
{
    return !FLeafPage() && !FPreInitPage();
}


INLINE BOOL CPAGE::FRootPage ( ) const
{
    return FFlags() & fPageRoot;
}


INLINE BOOL CPAGE::FFDPPage ( ) const
{
    return ( FRootPage() && !FSpaceTree() );
}


INLINE BOOL CPAGE::FEmptyPage ( ) const
{
    return FFlags() & fPageEmpty;
}


INLINE BOOL CPAGE::FPreInitPage ( ) const
{
    return FFlags() & fPagePreInit;
}


INLINE BOOL CPAGE::FParentOfLeaf ( ) const
{
    return FFlags() & fPageParentOfLeaf;
}


INLINE BOOL CPAGE::FSpaceTree ( ) const
{
    return FFlags() & fPageSpaceTree;
}


INLINE BOOL CPAGE::FScrubbed ( ) const
{
    return FFlags() & fPageScrubbed;
}


INLINE BOOL CPAGE::FRepairedPage ( ) const
{
    return FFlags() & fPageRepair;
}


INLINE BOOL CPAGE::FPrimaryPage ( ) const
{
    return !FIndexPage() && !FPreInitPage();
}


INLINE BOOL CPAGE::FIndexPage ( ) const
{
    return FFlags() & fPageIndex;
}


INLINE BOOL CPAGE::FNonUniqueKeys ( ) const
{
    Assert( !( FFlags() & fPageNonUniqueKeys ) || FIndexPage() );
    return FFlags() & fPageNonUniqueKeys;
}


INLINE BOOL CPAGE::FLongValuePage ( ) const
{
    return FFlags() & fPageLongValue;
}


INLINE BOOL CPAGE::FNewRecordFormat ( ) const
{
    return FFlags() & fPageNewRecordFormat;
}


INLINE BOOL CPAGE::FNewChecksumFormat ( ) const
{
    return FFlags() & fPageNewChecksumFormat;
}

INLINE CPAGE::PageFlushType CPAGE::Pgft( ) const
{
    return (CPAGE::PageFlushType)( ( FFlags() & maskFlushType ) >> 15 );
}

INLINE IFMP CPAGE::Ifmp ( ) const
{
    return m_ifmp;
}


INLINE OBJID CPAGE::ObjidFDP ( ) const
{
    return ((PGHDR*)m_bfl.pv)->objidFDP;
}


INLINE PGNO CPAGE::PgnoThis ( ) const
{
    if ( FSmallPageFormat() )
    {
        return m_pgno;
    }
    else
    {
        Assert( ( m_pgno == ((PGHDR2*)m_bfl.pv)->pgno ) ||
                FNegTest( fCorruptingPagePgnos ) ||
                FUtilZeroed( (BYTE*)m_bfl.pv, CbBuffer() ) );
        return ((PGHDR2*)m_bfl.pv)->pgno;
    }
}

INLINE PGNO CPAGE::PgnoREPAIRThis ( ) const
{
    Assert( g_fRepair );

    if ( FSmallPageFormat() )
    {
        return m_pgno;
    }
    else
    {
        Assert( m_pgno == ((PGHDR2*)m_bfl.pv)->pgno ||
                ( g_fRepair && ((PGHDR2*)m_bfl.pv)->pgno == pgnoNull ) );
        return ((PGHDR2*)m_bfl.pv)->pgno;
    }
}

INLINE PGNO CPAGE::PgnoNext ( ) const
{
    return ((PGHDR*)m_bfl.pv)->pgnoNext;
}


INLINE PGNO CPAGE::PgnoPrev ( ) const
{
    return ((PGHDR*)m_bfl.pv)->pgnoPrev;
}


INLINE DBTIME CPAGE::Dbtime ( ) const
{
    return ((PGHDR*)m_bfl.pv)->dbtimeDirtied;
}


INLINE USHORT CPAGE::CbUncommittedFree ( ) const
{
    return ((PGHDR*)m_bfl.pv)->cbUncommittedFree;
}


INLINE VOID * CPAGE::PvBuffer ( ) const
{
    return m_bfl.pv;
}


template< PageNodeBoundsChecking pgnbc >
INLINE VOID CPAGE::GetPtr( INT iline, LINE * pline, _Out_opt_ ERR * perrNoEnforce ) const
{
    Assert( iline >= 0 );
    Assert( pline );
    GetPtr_< pgnbc >( iline + ctagReserved, pline, perrNoEnforce );
}

INLINE ERR CPAGE::ErrGetPtr( INT iline, LINE * pline ) const
{
    ERR err = JET_errSuccess;
    Assert( iline >= 0 );
    Assert( pline );
    GetPtr< pgnbcChecked >( iline, pline, &err );
    return err;
}

template< PageNodeBoundsChecking pgnbc >
INLINE VOID CPAGE::GetPtrExternalHeader( LINE * pline, _Out_opt_ ERR * perrNoEnforce ) const
{
    Assert( pline );
    GetPtr_< pgnbc >( 0, pline, perrNoEnforce );
}

INLINE ERR CPAGE::ErrGetPtrExternalHeader( LINE * pline ) const
{
    ERR err = JET_errSuccess;
    Assert( pline );
    GetPtrExternalHeader< pgnbcChecked >( pline, &err );
    return err;
}

INLINE ULONG_PTR CPAGE::PbDataStart_( _In_ const BOOL fSmallPageCached ) const
{
    Assert( FSmallPageFormat() == fSmallPageCached );
    return (ULONG_PTR)m_bfl.pv + ( fSmallPageCached ? sizeof( PGHDR ) : sizeof( PGHDR2 ) );
}

INLINE ULONG_PTR CPAGE::PbDataStart_() const
{
    return PbDataStart_( FSmallPageFormat() );
}


INLINE ULONG_PTR CPAGE::PbDataEnd_( _In_ const ULONG cbBufferCached ) const
{
    Assert( CbBuffer() == cbBufferCached );
    return (ULONG_PTR)m_bfl.pv + cbBufferCached - CbTagArray_() - 1;
}

INLINE ULONG_PTR CPAGE::PbDataEnd_() const
{
    return PbDataEnd_( CbBuffer() );
}



#include <pshpack1.h>

PERSISTED
struct PATCHHDR
{
    CPAGE::PGHDR                    m_hdrNormalPage;

    BKINFO                          bkinfo;
    SIGNATURE                       signDb;
    SIGNATURE                       signLog;
    UnalignedLittleEndian< ULONG >  lgenMinReq;
    UnalignedLittleEndian< ULONG >  lgenMaxReq;
    UnalignedLittleEndian< ULONG >  lgenMaxCommitted;
    LOGTIME                         logtimeGenMaxRequired;
    LOGTIME                         logtimeGenMaxCommitted;
};

C_ASSERT( sizeof( PATCHHDR ) <= g_cbPageMin );

#include <poppack.h>


#define ErrCaptureCorruptedPageInfo( mode, wszCorruptionType )                                   ErrCaptureCorruptedPageInfo_( mode, wszCorruptionType, NULL, __FILE__, __LINE__ )
#define ErrCaptureCorruptedPageInfoSz( mode, wszCorruptionType, wszCorruptionDetails, fLogEvent ) ErrCaptureCorruptedPageInfo_( mode, wszCorruptionType, wszCorruptionDetails, __FILE__, __LINE__, fLogEvent )
#define PageEnforceSz( cpage, exp, wszType, wszDetails )      ( ( exp ) ? (void) 0 : cpage.ErrCaptureCorruptedPageInfoSz( CPAGE::CheckPageMode::OnErrorEnforce, wszType, wszDetails, fTrue ) )
#define PageEnforce( cpage, exp )   PageEnforceSz( cpage, exp, L#exp, NULL )
#define PageAssertTrack( cpage, exp, szType, ... )                            \
    {                                                                          \
    const bool fExp = ( exp );                                                  \
    if ( !fExp )                                                                 \
    {                                                                             \
        (cpage).ErrCaptureCorruptedPageInfo( CPAGE::OnErrorReturnError, szType );  \
        AssertTrackSz( fExp, szType, __VA_ARGS__ );                                 \
    }                                                                                \
    }
