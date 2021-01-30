// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef FLUSHMAP_H_INCLUDED
#define FLUSHMAP_H_INCLUDED

extern PERFInstanceLiveTotal<> cFMPagesWrittenAsync;
extern PERFInstanceLiveTotal<> cFMPagesWrittenSync;
extern PERFInstanceDelayedTotal<> cFMPagesDirty;
extern PERFInstanceDelayedTotal<> cFMPagesTotal;


class CFlushMap
{
    protected:

        static const DWORD s_cbitFlushType = 2;
        static_assert( ( 1 << s_cbitFlushType ) >= CPAGE::pgftMax, "We need to be able to represent all flush states." );
        static_assert( ( 8 % s_cbitFlushType ) == 0, "Flush type bits are not allowed to cross bytes for simplicity of the algorithm." );
        static_assert( 8 >= s_cbitFlushType, "Number of flush type bits cannot exceed the size of a byte for simplicity of the algorithm." );

        static const BYTE s_mskFlushType = 0xC0;
        static_assert( ( ( s_mskFlushType << s_cbitFlushType ) & 0xFF ) == 0, "Inconsistent CFlushMap::s_mskFlushType." );
        static_assert( ( ( ~s_mskFlushType & 0xFF ) >> ( 8 - s_cbitFlushType ) ) == 0, "Inconsistent CFlushMap::s_mskFlushType." );

        static const DWORD s_cbFlushMapPageOnDisk = 8 * 1024;

        static const WCHAR* const s_wszFmFileExtension;



        static const FMPGNO s_fmpgnoHdr = -1;
        static const FMPGNO s_fmpgnoUninit = -2;
        static_assert( s_fmpgnoHdr == -1, "If you change this, please review the code very carefully to make sure there aren't any implicit assumptions about it being -1." );
        static_assert( s_fmpgnoUninit == -2, "If you change this, please review the code very carefully to make sure there aren't any implicit assumptions about it being -2." );


        struct FlushMapPageDescriptor
        {
            public:
                void* pv;
                void* pvWriteBuffer;
                void* rgbitRuntime;
                DBTIME dbtimeMax;
                volatile ULONG fFlags;
                CSXWLatch sxwl;
                FMPGNO fmpgno;
                ERR errAsyncWrite;

            private:
                static const ULONG fAllocated                   = 0x00000001;
                static const ULONG fValid                       = 0x00000002;
                static const ULONG fDirty                       = 0x00000004;
                static const ULONG fReadInProgress              = 0x00000008;
                static const ULONG fSyncWriteInProgress         = 0x00000010;
                static const ULONG fAsyncWriteInProgress        = 0x00000020;
                static const ULONG fRecentlyAsyncWrittenHeader  = 0x00000040;

            public:
                FlushMapPageDescriptor( const FMPGNO fmpgnoT ) :
                    pv( NULL ),
                    pvWriteBuffer( NULL ),
                    rgbitRuntime( NULL ),
                    dbtimeMax( dbtimeNil ),
                    fFlags( 0 ),
                    sxwl( CLockBasicInfo( CSyncBasicInfo( "FlushMapPageDescriptor::sxwl" ), rankFlushMapAccess, IGetPageLockSubrank_( fmpgnoT ) ) ),
                    fmpgno( fmpgnoT ),
                    errAsyncWrite( JET_errSuccess )
                {
                }

                ~FlushMapPageDescriptor()
                {
                    Assert( pv == NULL );
                }

                BOOL FIsFmHeader() const
                {
                    return FIsFmHeader_( fmpgno );
                }

                INLINE BOOL FAllocated() const { return ( ( fFlags & fAllocated ) != 0 ); }
                INLINE BOOL FValid() const { return ( ( fFlags & fValid ) != 0 ); }
                INLINE BOOL FDirty() const { return ( ( fFlags & fDirty ) != 0 ); }
                INLINE BOOL FReadInProgress() const { return ( ( fFlags & fReadInProgress ) != 0 ); }
                INLINE BOOL FSyncWriteInProgress() const { return ( ( fFlags & fSyncWriteInProgress ) != 0 ); }
                INLINE BOOL FAsyncWriteInProgress() const { return ( ( fFlags & fAsyncWriteInProgress ) != 0 ); }
                INLINE BOOL FRecentlyAsyncWrittenHeader() const { return ( ( fFlags & fRecentlyAsyncWrittenHeader ) != 0 ); }
                INLINE BOOL FWriteInProgress() const { return ( FSyncWriteInProgress() || FAsyncWriteInProgress() ); }

                INLINE void SetAllocated() { AtomicExchangeSet( (ULONG*)&fFlags, fAllocated ); }
                INLINE void SetValid() { AtomicExchangeSet( (ULONG*)&fFlags, fValid ); }
                INLINE void SetDirty( const INST* const pinst )
                {
                    Assert( sxwl.FOwner() );
                    Assert( FAllocated() );
                    if ( ( ( AtomicExchangeSet( (ULONG*)( &fFlags ), fDirty ) & fDirty ) == 0 ) &&
                        !FIsFmHeader() &&
                        ( pinst != pinstNil ) )
                    {
                        PERFOpt( cFMPagesDirty.Inc( pinst->m_iInstance ) );
                    }
                }
                INLINE void SetReadInProgress() { AtomicExchangeSet( (ULONG*)&fFlags, fReadInProgress ); }
                INLINE void SetSyncWriteInProgress() { AtomicExchangeSet( (ULONG*)&fFlags, fSyncWriteInProgress ); }
                INLINE void SetAsyncWriteInProgress() { AtomicExchangeSet( (ULONG*)&fFlags, fAsyncWriteInProgress ); }
                INLINE void SetRecentlyAsyncWrittenHeader() { AtomicExchangeSet( (ULONG*)&fFlags, fRecentlyAsyncWrittenHeader ); }
                INLINE void ResetAllocated() { AtomicExchangeReset( (ULONG*)&fFlags, fAllocated ); }
                INLINE void ResetDirty( const INST* const pinst )
                {
                    if ( ( ( AtomicExchangeReset( (ULONG*)( &fFlags ), fDirty ) & fDirty ) != 0 ) &&
                        !FIsFmHeader() &&
                        ( pinst != pinstNil ) )
                    {
                        PERFOpt( cFMPagesDirty.Dec( pinst->m_iInstance ) );
                    }
                }
                INLINE void ResetReadInProgress() { AtomicExchangeReset( (ULONG*)&fFlags, fReadInProgress ); }
                INLINE void ResetSyncWriteInProgress() { AtomicExchangeReset( (ULONG*)&fFlags, fSyncWriteInProgress ); }
                INLINE void ResetAsyncWriteInProgress() { AtomicExchangeReset( (ULONG*)&fFlags, fAsyncWriteInProgress ); }
                INLINE void ResetRecentlyAsyncWrittenHeader() { AtomicExchangeReset( (ULONG*)&fFlags, fRecentlyAsyncWrittenHeader ); }
        };

        #include <pshpack1.h>

        PERSISTED
        struct FMFILEHDR : public CZeroInit
        {
            union
            {
                PERSISTED
                struct
                {
                    LittleEndian<LONG>  checksum;

                    LittleEndian<ULONG>         le_ulFmVersionMajor;
                    LittleEndian<ULONG>         le_ulFmVersionUpdateMajor;
                    LittleEndian<ULONG>         le_ulFmVersionUpdateMinor;

                    BYTE                        rgbFlagsAlignmentPadding[ 36 - 16  ];
                    LittleEndian<ULONG>         fFlags;
                };

                BYTE                            rgbFixedSizeBlob[ 40 ];
            };

            LOGTIME                     logtimeCreate;
            LOGTIME                     logtimeAttach;
            LOGTIME                     logtimeDetach;

            LittleEndian<LONG>          lGenMinRequired;
            LittleEndian<LONG>          lGenMinRequiredTarget;

            LOGTIME                     logtimeFlushLastStarted;

            LOGTIME                     logtimeFlushLastCompleted;

            SIGNATURE                   signDbHdrFlush;
            SIGNATURE                   signFlushMapHdrFlush;

            BYTE                            rgbFileTypeAlignmentPadding[ 667 - 144  ];
            UnalignedLittleEndian<ULONG>    filetype;

            static const ULONG fClean                   = 0x00000001;
            static const ULONG fPageNewChecksumFormat   = 0x00002000;

            FMFILEHDR() :
                CZeroInit( sizeof( FMFILEHDR ) )
            {
            }

            INLINE BOOL FClean() const { return ( ( fFlags & fClean ) != 0 ); }
            INLINE BOOL FNewChecksumFormat() const { return ( ( fFlags & fPageNewChecksumFormat ) != 0 ); }

            INLINE void SetClean() { fFlags |= fClean; }
            INLINE void ResetClean() { fFlags &= ~fClean; }
        };
        static_assert( FMFILEHDR::fPageNewChecksumFormat == CPAGE::fPageNewChecksumFormat, "Flush map header page and DB page new checksum format flag mask must match." );
        static_assert( OffsetOf( FMFILEHDR, checksum ) == OffsetOf( CPAGE::PGHDR, checksum ), "Offset of checksum in FMFILEHDR and CPAGE must match." );
        static_assert( OffsetOf( FMFILEHDR, fFlags ) == OffsetOf( CPAGE::PGHDR, fFlags ), "Offset of fFlags in FMFILEHDR and CPAGE must match." );
        static_assert( OffsetOf( FMFILEHDR, logtimeCreate ) == 40, "Size of the fixed part of FMFILEHDR is not expected to change. If intentional, make sure the upgrade is handled." );
        static_assert( OffsetOf( FMFILEHDR, filetype ) == 667, "Offset of filetype in FMFILEHDR must be fixed for compatibility with other file types." );
        static_assert( sizeof( FMFILEHDR ) == 671, "Size of FMFILEHDR is not expected to change. If intentional, make sure the upgrade is handled." );


        PERSISTED
        struct FlushMapDataPageHdr : public CZeroInit
        {
            LittleEndian<XECHECKSUM>    checksum;

            LittleEndian<DBTIME>        dbtimeMax;
            LittleEndian<FMPGNO>        fmpgno;

            BYTE                        rgbFlagsAlignmentPadding[ 36 - 20  ];
            LittleEndian<ULONG>         fFlags;

            BYTE                        rgbStructSizePadding[ 64 - 40  ];

            static const ULONG fPageNewChecksumFormat   = 0x00002000;

            FlushMapDataPageHdr() :
                CZeroInit( sizeof( FlushMapDataPageHdr ) )
            {
            }

            INLINE BOOL FNewChecksumFormat() const { return ( ( fFlags & fPageNewChecksumFormat ) != 0 ); }
        };
        static_assert( flushmapDataPage == databasePage, "Flush map data page type must be the same as database page type for checksum compatibility." );
        static_assert( FlushMapDataPageHdr::fPageNewChecksumFormat == CPAGE::fPageNewChecksumFormat, "Flush map data page and DB page new checksum format flag mask must match." );
        static_assert( OffsetOf( FlushMapDataPageHdr, checksum ) == OffsetOf( CPAGE::PGHDR, checksum ), "Offset of checksum in FlushMapDataPageHdr and CPAGE must match." );
        static_assert( OffsetOf( FlushMapDataPageHdr, fFlags ) == OffsetOf( CPAGE::PGHDR, fFlags ), "Offset of fFlags in FlushMapDataPageHdr and CPAGE must match." );
        static_assert( sizeof( FlushMapDataPageHdr ) == 64, "Size of FlushMapDataPageHdr is not expected to change. If intentional, make sure the upgrade is handled." );

        #include <poppack.h>

    protected:

        LONG m_lgenTargetedMax;

        BOOL m_fInitialized;

        BOOL m_fGetSetAllowed;

        BOOL m_fDumpMode;

        BOOL m_fPersisted_;

        BOOL m_fPersisted;

        BOOL m_fCreateNew_;

        BOOL m_fCreateNew;

        BOOL m_fRecoverable;

        BOOL m_fReadOnly;

        BOOL m_fCleanForTerm;

        INST* m_pinst;

        IFileSystemAPI* m_pfsapi;

        IFileAPI* m_pfapi;

        LONG m_cInFlightIo;

        CSXWLatch m_sxwlSectionFlush;

        FMPGNO m_fmpgnoSectionFlushFirst;

        FMPGNO m_fmpgnoSectionFlushLast;

        FMPGNO m_fmpgnoSectionFlushNext;

        BOOL m_fSectionCheckpointHeaderWrite;

        ERR m_errSectionFlushWriteLastError;

        CMeteredSection m_msSectionFlush;

        CMeteredSection::Group m_groupSectionFlushWrite;

        void* m_pvSectionFlushAsyncWriteBuffer;

        DWORD m_cbSectionFlushAsyncWriteBufferReserved;

        DWORD m_cbSectionFlushAsyncWriteBufferCommitted;

        DWORD m_ibSectionFlushAsyncWriteBufferNext;

        QWORD m_cbLogGeneratedAtStartOfFlush;

        QWORD m_cbLogGeneratedAtLastFlushProgress;

        DWORD m_cbFmPageInMemory;

        DWORD m_cbRuntimeBitmapPage;

        DWORD m_cbDescriptorPage;

        USHORT m_cDbPagesPerFlushMapPage;

        FlushMapPageDescriptor m_fmdFmHdr;

        FlushMapPageDescriptor m_fmdFmPg0;

        CCriticalSection m_critFmdGrowCapacity;

        FlushMapPageDescriptor* volatile m_rgfmd;

        volatile CFMPG m_cfmdCommitted;

        volatile CFMPG m_cfmpgAllocated;

        DWORD m_cbfmdReserved;

        DWORD m_cbfmdCommitted;

        DWORD m_cbfmAllocated;


    protected:
        static INT IGetPageLockSubrank_( const FMPGNO fmpgno );
        static BOOL FIsFmHeader_( const FMPGNO fmpgno );
        static QWORD OffsetOfFmPgno_( const FMPGNO fmpgno );
        static FMPGNO FmPgnoOfOffset_( const QWORD ibOffset );
        static BOOL FCheckForDbFmConsistency_(
            const SIGNATURE* const psignDbHdrFlushFromDb,
            const SIGNATURE* const psignFlushMapHdrFlushFromDb,
            const SIGNATURE* const psignDbHdrFlushFromFm,
            const SIGNATURE* const psignFlushMapHdrFlushFromFm );
        static ERR ErrChecksumFmPage_(
            void* const pv,
            const FMPGNO fmpgno,
            const BOOL fCorrectError = fTrue,
            XECHECKSUM* const pchecksumPersisted = NULL,
            XECHECKSUM* const pchecksumCalculated = NULL );
        CFMPG CfmpgGetRequiredFmDataPageCount_( const PGNO pgnoReq );
        CFMPG CfmpgGetPreferredFmDataPageCount_( const PGNO pgnoReq );
        QWORD CbGetRequiredFmFileSize_( const CFMPG cfmpgDataNeeded );
        INLINE void SetFmPageDirty_( FlushMapPageDescriptor* const pfmd );
        INLINE void ResetFmPageDirty_( FlushMapPageDescriptor* const pfmd );

        ERR ErrAttachFlushMap_();
        void TermFlushMap_();

        INLINE ERR ErrGetDescriptorFromFmPgno_( const FMPGNO fmpgno, FlushMapPageDescriptor** const ppfmd );
        INLINE ERR ErrGetDescriptorFromPgno_( const PGNO pgno, FlushMapPageDescriptor** const ppfmd );
        ERR ErrAllocateDescriptorsCapacity_( const CFMPG cfmdNeeded );
        ERR ErrAllocateFmDataPageCapacity_( const CFMPG cfmpgNeeded );
        ERR ErrAllocateFmPage_( FlushMapPageDescriptor* const pfmd );
        void FreeFmPage_( FlushMapPageDescriptor* const pfmd );
        void InitializeFmDataPageCapacity_( const CFMPG cfmpgNeeded );
        void PrereadFlushMap_( const CFMPG cfmpg );
        void InitializeFmHdr_( FlushMapPageDescriptor* const pfmd );
        void InitializeFmDataPage_( FlushMapPageDescriptor* const pfmd );

        void EnterDbHeaderFlush_( SIGNATURE* const psignDbHdrFlush, SIGNATURE* const psignFlushMapHdrFlush );
        void LeaveDbHeaderFlush_();

        void PrepareFmPageForRead_( FlushMapPageDescriptor* const pfmd, const BOOL fSync );
        ERR ErrPrepareFmPageForWrite_( FlushMapPageDescriptor* const pfmd, const BOOL fSync );
        ERR ErrReadFmPage_( FlushMapPageDescriptor* const pfmd, const BOOL fSync, const OSFILEQOS qos );
        ERR ErrWriteFmPage_( FlushMapPageDescriptor* const pfmd, const BOOL fSync, const OSFILEQOS qos );
        static void OsReadIoComplete_(
            ERR errIo,
            IFileAPI* const pfapi,
            const FullTraceContext& tc,
            const OSFILEQOS grbitQOS,
            const QWORD ibOffset,
            const DWORD cbData,
            const BYTE* const pbData,
            const DWORD_PTR keyIOComplete );
        static void OsWriteIoComplete_(
            const ERR errIo,
            IFileAPI* const pfapi,
            const FullTraceContext& tc,
            const OSFILEQOS grbitQOS,
            const QWORD ibOffset,
            const DWORD cbData,
            const BYTE* const pbData,
            const DWORD_PTR keyIOComplete );
        ERR ErrReadIoComplete_( const ERR errIo, const BOOL fSync, FlushMapPageDescriptor* const pfmd );
        ERR ErrWriteIoComplete_( const ERR errIo, const BOOL fSync, FlushMapPageDescriptor* const pfmd );
        ERR ErrSyncReadFmPage_( FlushMapPageDescriptor* const pfmd );
        ERR ErrSyncWriteFmPage_( FlushMapPageDescriptor* const pfmd );
        ERR ErrAsyncReadFmPage_( FlushMapPageDescriptor* const pfmd, const OSFILEQOS qos );
        ERR ErrAsyncWriteFmPage_( FlushMapPageDescriptor* const pfmd, const OSFILEQOS qos );
        void FlushOneSection_( OnDebug2( const BOOL fCleanFlushMap, const BOOL fMapPatching ) );
        ERR ErrFlushAllSections_( OnDebug2( const BOOL fCleanFlushMap, const BOOL fMapPatching ) );
        void CompleteOneSectionFlush_();
        static void SectionFlushCompletion_( const DWORD_PTR dwCompletionKey );
        void AllowFlushMapIo_();
        void DisallowFlushMapIo_();
        void WaitForFlushMapIoQuiesce_();
        void IncrementFlushMapIoCount_();
        void DecrementFlushMapIoCount_();

#ifdef DEBUG
        void AssertFlushMapIoAllowed_();
        void AssertFlushMapIoDisallowed_();
        void AssertFlushMapIoAllowedAndInFlight_();
        void AssertFlushMapIoAllowedAndNotInFlight_();
        void AssertPreIo_( const BOOL fWrite, FlushMapPageDescriptor* const pfmd );
        void AssertOnIoCompletion_( const BOOL fSync, const BOOL fWrite, FlushMapPageDescriptor* const pfmd );
        void AssertPostIo_( const ERR err, const BOOL fSync, const BOOL fWrite, FlushMapPageDescriptor* const pfmd );
#endif

        ERR ErrChecksumFmPage_( FlushMapPageDescriptor* const pfmd );
        ERR ErrValidateFmHdr_( FlushMapPageDescriptor* const pfmd );
        ERR ErrValidateFmDataPage_( FlushMapPageDescriptor* const pfmd );

        ERR ErrSetRangePgnoFlushType_( const PGNO pgnoFirst, const CPG cpg, const CPAGE::PageFlushType pgft, const DBTIME dbtime, const BOOL fWait );
        INLINE USHORT IbitGetBitInPage_( const PGNO pgno, const size_t cbHeader, const DWORD cbitPerState );
        INLINE INT IGetStateFromBitmap_(
            BYTE* const pbBitmap, const size_t cbBitmap,
            const PGNO pgno,
            const size_t cbHeader, const DWORD cbitPerState, const BYTE mask );
        INLINE void SetStateOnBitmap_(
            BYTE* const pbBitmap, const size_t cbBitmap,
            const PGNO pgno, const INT state,
            const size_t cbHeader, const DWORD cbitPerState, const BYTE mask );
        CPAGE::PageFlushType PgftGetFlushType_( FlushMapPageDescriptor* const pfmd, const PGNO pgno );
        void SetFlushType_( FlushMapPageDescriptor* const pfmd, const PGNO pgno, const CPAGE::PageFlushType pgft );
        BOOL FGetFlushTypeRuntime_( FlushMapPageDescriptor* const pfmd, const PGNO pgno );
        void SetFlushTypeRuntimeState_( FlushMapPageDescriptor* const pfmd, const PGNO pgno, const BOOL fRuntime );

        void LogEventFmFileAttachFailed_( const WCHAR* const wszFmFilePath, const ERR err );
        void LogEventFmFileDeleted_( const WCHAR* const wszFmFilePath, const WCHAR* const wszReason );
        void LogEventFmFileCreated_( const WCHAR* const wszFmFilePath );
        void LogEventFmHdrValidationFailed_( const ERR err, const WCHAR* const wszAdditionalInfo );
        void LogEventFmDataPageValidationFailed_( const ERR err, const FMPGNO fmpgno );
        void LogEventFmInconsistentDbTime_( const FMPGNO fmpgno, const DBTIME dbtimeMaxFm, const PGNO pgno, const DBTIME dbtimeMaxDb );



    protected:

        virtual BOOL FIsReadOnly_() = 0;
        virtual ERR ErrGetFmFilePath_( _Out_writes_z_(OSFSAPI_MAX_PATH) WCHAR* const wszFmFilePath ) = 0;
        virtual BOOL FIsRecoverable_() = 0;
        virtual INST* PinstGetInst_() = 0;
        virtual IFileSystemConfiguration* Pfsconfig_() = 0;
        virtual QWORD QwGetEseFileId_() = 0;
        virtual ULONG UlGetDbState_() = 0;
        virtual LONG LGetDbGenMinRequired_() = 0;
        virtual LONG LGetDbGenMinConsistent_() = 0;
        virtual void CopyDbHdrFlushSignatureFromDb_( SIGNATURE* const psignDbHdrFlushFromFm ) = 0;
        virtual void GetFlushSignaturesFromDb_( const FMFILEHDR* const pfmfhdr, SIGNATURE* const psignDbHdrFlushFromDb, SIGNATURE* const psignFlushMapHdrFlushFromDb ) = 0;
        virtual CPG CpgGetDbExtensionSize_() = 0;


    public:

        static const BYTE s_pctCheckpointDelay = 20;

        CFlushMap();
        ~CFlushMap();

        void SetPersisted( __in const BOOL fPersisted );
        void SetCreateNew( __in const BOOL fCreateNew );

        ERR ErrInitFlushMap();
        void TermFlushMap();

        ERR ErrCleanFlushMap();

        ERR ErrFlushMapFlushFileBuffers( _In_ const IOFLUSHREASON iofr );

        void SetPgnoFlushType( __in const PGNO pgno, __in const CPAGE::PageFlushType pgft, __in const DBTIME dbtime );
        void SetPgnoFlushType( __in const PGNO pgno, __in const CPAGE::PageFlushType pgft );
        void SetRangeFlushType( __in const PGNO pgnoFirst, __in const CPG cpg, __in const CPAGE::PageFlushType pgft );
        ERR ErrSetPgnoFlushTypeAndWait( __in const PGNO pgno, __in const CPAGE::PageFlushType pgft, __in const DBTIME dbtime );
        ERR ErrSetRangeFlushTypeAndWait( __in const PGNO pgnoFirst, __in const CPG cpg, __in const CPAGE::PageFlushType pgft );
        CPAGE::PageFlushType PgftGetPgnoFlushType( __in const PGNO pgno );
        CPAGE::PageFlushType PgftGetPgnoFlushType( __in const PGNO pgno, __in const DBTIME dbtime, __out BOOL* const pfRuntime );
        ERR ErrSyncRangeInvalidateFlushType( __in const PGNO pgnoFirst, __in const CPG cpg );

        LONG LGetFmGenMinRequired();

        BOOL FRecoverable();

        BOOL FRequestFmSectionWrite( __in const QWORD cbLogGenerated, __in const QWORD cbPreferredChktpDepth );

        void FlushOneSection( __in const QWORD cbLogGenerated );

        ERR ErrFlushAllSections( OnDebug( __in const BOOL fMapPatching ) );

        ERR ErrSetFlushMapCapacity( __in const PGNO pgnoReq );

        INLINE FMPGNO FmpgnoGetFmPgnoFromDbPgno( __in const PGNO pgno );

        static void EnterDbHeaderFlush( __in CFlushMap* const pfm, __out SIGNATURE* const psignDbHdrFlush, __out SIGNATURE* const psignFlushMapHdrFlush );
        static void LeaveDbHeaderFlush( __in CFlushMap* const pfm );

        static ERR ErrGetFmPathFromDbPath(
            _Out_writes_z_( OSFSAPI_MAX_PATH ) WCHAR* const wszFmPath,
            _In_z_ const WCHAR* const wszDbPath );

    public:

#ifdef ENABLE_JET_UNIT_TEST
    friend class TestCFlushMapSettingHighPageNumbers;
    friend class TestCFlushMapSettingAWholeFlushMapPage;
    friend class TestCFlushMapFragmentedFlushMapFileSizeIsConsistent;
    friend class TestCFlushMapNonFragmentedSmallClientFlushMapFileSizeIsConsistent;
    friend class TestCFlushMapNonFragmentedLargeClientFlushMapFileSizeIsConsistent;
    friend class TestCFlushMapNonFragmentedNonPersistedFlushMapAllocationIsConsistent;
    friend class TestCFlushMapDbAndFmHeaderFlushesAreCorrectlySynchronized;
    friend class TestCFlushMapFRecentlyAsyncWrittenHeaderIsProperlyMaintained;
    friend class TestCFlushMapAsyncMapFlushCorrectlySignalsFlush;
    friend class TestCFlushMapAsyncMapFlushCorrectlySignalsFlushWithZeroedCheckpoint;
    friend class TestCFlushMapSyncSetFlushTypePersistsState;
    friend class TestCFlushMapFlushSectionPersistsState;
    friend class TestCFlushMapAsyncMapFlushHonorsMaxWriteSize;
    friend class TestCFlushMapAsyncMapFlushCorrectlySkipsCleanPages;
    friend class TestCFlushMapAsyncMapFlushHonorsMaxWriteGapSize;
    friend class TestCFlushMapAttachFlushMapLargerThanMaxReadSizeWorks;
    friend class TestCFlushMapDumpAndChecksumOfUncorruptedFlushMapWorks;
    friend class TestCFlushMapDumpAndChecksumOfCorruptedFlushMapWorks;
    friend class TestCFlushMapDumpAndChecksumOfCorruptedFlushMapDoesNotWorkIfHeaderCorrupt;
    friend class TestCFlushMapVariousInvalidFlushMapConditionsAreCorrectlyDetected;
#endif

#ifdef DEBUGGER_EXTENSION
    friend LOCAL VOID EDBGUpdateFlushMapMemoryUsage(
        const CFlushMap* const pfm,
        DWORD* const pcbfmdReserved,
        DWORD* const pcbfmdCommitted,
        DWORD* const pcbfmAllocated );
#endif
};


class CFlushMapForAttachedDb : public CFlushMap
{
        FMP* m_pfmp;

    protected:
        BOOL FIsReadOnly_();
        ERR ErrGetFmFilePath_( _Out_writes_z_(OSFSAPI_MAX_PATH) WCHAR* const wszFmFilePath );
        BOOL FIsRecoverable_();
        INST* PinstGetInst_();
        IFileSystemConfiguration* Pfsconfig_() override;
        QWORD QwGetEseFileId_();
        ULONG UlGetDbState_();
        LONG LGetDbGenMinRequired_();
        LONG LGetDbGenMinConsistent_();
        void CopyDbHdrFlushSignatureFromDb_( SIGNATURE* const psignDbHdrFlushFromFm );
        void GetFlushSignaturesFromDb_( const FMFILEHDR* const pfmfhdr, SIGNATURE* const psignDbHdrFlushFromDb, SIGNATURE* const psignFlushMapHdrFlushFromDb );
        CPG CpgGetDbExtensionSize_();


    public:
        CFlushMapForAttachedDb();
        
        void SetDbFmp( __in FMP* const pfmp );
};


class CFlushMapForUnattachedDb : public CFlushMap
{
    private:
        BOOL m_fReadOnly_;
        const WCHAR* m_wszFmFilePath_;
        BOOL m_fRecoverable_;
        INST* m_pinst_;
        IFileSystemConfiguration* m_pfsconfig_;
        ULONG m_ulDbState_;
        LONG m_lGenDbMinRequired_;
        LONG m_lGenDbMinConsistent_;
        const SIGNATURE* m_psignDbHdrFlushFromDb_;
        const SIGNATURE* m_psignFlushMapHdrFlushFromDb_;
        CPG m_cpgDbExtensionSize_;


    protected:
        BOOL FIsReadOnly_();
        ERR ErrGetFmFilePath_( _Out_writes_z_(OSFSAPI_MAX_PATH) WCHAR* const wszFmFilePath );
        BOOL FIsRecoverable_();
        INST* PinstGetInst_();
        IFileSystemConfiguration* Pfsconfig_() override;
        QWORD QwGetEseFileId_();
        ULONG UlGetDbState_();
        LONG LGetDbGenMinRequired_();
        LONG LGetDbGenMinConsistent_();
        void CopyDbHdrFlushSignatureFromDb_( SIGNATURE* const psignDbHdrFlushFromFm );
        void GetFlushSignaturesFromDb_( const FMFILEHDR* const pfmfhdr, SIGNATURE* const psignDbHdrFlushFromDb, SIGNATURE* const psignFlushMapHdrFlushFromDb );
        CPG CpgGetDbExtensionSize_();


    public:
        CFlushMapForUnattachedDb();

        void SetReadOnly( __in const BOOL fReadOnly );
        void SetFmFilePath( __in const WCHAR* const wszFmFilePath );
        void SetRecoverable( __in const BOOL fRecoverable );
        void SetEseInstance( __in INST* const pinst );
        void SetFileSystemConfiguration( __in IFileSystemConfiguration* const pfsconfig );
        void SetDbState( __in const ULONG ulDbState );
        void SetDbGenMinRequired( __in const LONG lGenDbMinRequired );
        void SetDbGenMinConsistent( __in const LONG lGenDbMinConsistent );
        void SetDbHdrFlushSignaturePointerFromDb( __in const SIGNATURE* const psignDbHdrFlushFromDb );
        void SetFmHdrFlushSignaturePointerFromDb( __in const SIGNATURE* const m_psignFlushMapHdrFlushFromDb_ );
        void SetDbExtensionSize( __in const CPG cpgDbExtensionSize );


        static ERR ErrGetPersistedFlushMapOrNullObjectIfRuntime(
            _In_z_ const WCHAR* const wszDbFilePath,
            __in const DBFILEHDR* const pdbHdr,
            __in INST* const pinst,
            __out CFlushMapForUnattachedDb** const ppfm );
};


class CFlushMapForDump : public CFlushMap
{
    private:
        const WCHAR* m_wszFmFilePath_;
        INST* m_pinst_;
        IFileSystemConfiguration* m_pfsconfig_;

        void DumpFmPage_( const FlushMapPageDescriptor* const pfmd );
        void DumpFmHdr_( const FlushMapPageDescriptor* const pfmd );
        void DumpFmDataPage_( const FlushMapPageDescriptor* const pfmd, const BOOL fDumpFlushStates );

    protected:
        BOOL FIsReadOnly_();
        ERR ErrGetFmFilePath_( _Out_writes_z_(OSFSAPI_MAX_PATH) WCHAR* const wszFmFilePath );
        BOOL FIsRecoverable_();
        INST* PinstGetInst_();
        IFileSystemConfiguration* Pfsconfig_() override;
        QWORD QwGetEseFileId_();
        ULONG UlGetDbState_();
        LONG LGetDbGenMinRequired_();
        LONG LGetDbGenMinConsistent_();
        void CopyDbHdrFlushSignatureFromDb_( SIGNATURE* const psignDbHdrFlushFromFm );
        void GetFlushSignaturesFromDb_( const FMFILEHDR* const pfmfhdr, SIGNATURE* const psignDbHdrFlushFromDb, SIGNATURE* const psignFlushMapHdrFlushFromDb );
        CPG CpgGetDbExtensionSize_();


    public:
        static ERR ErrChecksumFlushMapFile( __in INST* const pinst, __in const WCHAR* const wszFmFilePath, __in_opt IFileSystemConfiguration* pfsconfig = NULL );
        static ERR ErrDumpFlushMapPage( __in INST* const pinst, __in const WCHAR* const wszFmFilePath, __in const FMPGNO fmpgno, __in const BOOL fDumpFlushStates, __in_opt IFileSystemConfiguration* pfsconfig = NULL );


    public:
        CFlushMapForDump();

        void SetFmFilePath( __in const WCHAR* const wszFmFilePath );
        void SetEseInstance( __in INST* const pinst );
        void SetFileSystemConfiguration( __in IFileSystemConfiguration* const pfsconfig );

        ERR ErrDumpFmPage( __in const FMPGNO fmpgno, __in const BOOL fDumpFlushStates );
        ERR ErrChecksumFmPage( __in const FMPGNO fmpgno );
};

#endif


