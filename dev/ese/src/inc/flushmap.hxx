// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef FLUSHMAP_H_INCLUDED
#define FLUSHMAP_H_INCLUDED

extern PERFInstanceLiveTotal<> cFMPagesWrittenAsync;
extern PERFInstanceLiveTotal<> cFMPagesWrittenSync;
extern PERFInstanceDelayedTotal<> cFMPagesDirty;
extern PERFInstanceDelayedTotal<> cFMPagesTotal;


// Base class that implements a persisted version of the flush map for lost flush detection.
class CFlushMap
{
    protected:

        static const DWORD s_cbitFlushType = 2; // Number of bits used to represent a flush type: total of 133,088 flush map
                                                // pages for a maximum page count of pgnoSysMax + 1 (0x7FF00000).
        static_assert( ( 1 << s_cbitFlushType ) >= CPAGE::pgftMax, "We need to be able to represent all flush states." );
        static_assert( ( 8 % s_cbitFlushType ) == 0, "Flush type bits are not allowed to cross bytes for simplicity of the algorithm." );
        static_assert( 8 >= s_cbitFlushType, "Number of flush type bits cannot exceed the size of a byte for simplicity of the algorithm." );

        static const BYTE s_mskFlushType = 0xC0;    // Convenient mask to extract the s_cbitFlushType bits out of a byte. Note we're treating
                                                    // the most significant bit of the byte as the 0th flush type bit, which means s_mskFlushType
                                                    // is applied as is to extract the flush type of page X, s_mskFlushType >> s_cbitFlushType
                                                    // is used to extract the flush type of page X+1, etc...
        static_assert( ( ( s_mskFlushType << s_cbitFlushType ) & 0xFF ) == 0, "Inconsistent CFlushMap::s_mskFlushType." );
        static_assert( ( ( ~s_mskFlushType & 0xFF ) >> ( 8 - s_cbitFlushType ) ) == 0, "Inconsistent CFlushMap::s_mskFlushType." );

        static const DWORD s_cbFlushMapPageOnDisk = 8 * 1024;   // Flush map page size.

        static const WCHAR* const s_wszFmFileExtension; // Flush map file extension.


        // Internal data types.

        static const FMPGNO s_fmpgnoHdr = -1;       // Represents the FM header page.
        static const FMPGNO s_fmpgnoUninit = -2;    // Represents an uninitialized FM page number.
        static_assert( s_fmpgnoHdr == -1, "If you change this, please review the code very carefully to make sure there aren't any implicit assumptions about it being -1." );
        static_assert( s_fmpgnoUninit == -2, "If you change this, please review the code very carefully to make sure there aren't any implicit assumptions about it being -2." );


        // Auxiliary structure for flush map page management.
        //
        // Lifecycle and synchronization of FM pages and FM page descriptors:
        //  o The header page and page descriptor (fmpgno -1) are always pre-allocated during initialization so we're guaranteed
        //    to have at least the buffer allocated and the descriptor initialized at any point past a successful init.
        //
        //  o The first data page (fmpgno 0) descriptor is static and therefore always valid. Its respective buffer may not be though.
        //
        //  o Any descriptors (pfmd) up to m_cfmdCommitted can be accessed and manipulated, but don't necessarily have allocated
        //    buffers or valid data.
        //
        //  o Any descriptors (pfmd) up to m_cfmpgAllocated are guaranteed to be allocated. Yet, they might not contain valid data
        //    for a very short period of time: between their allocation and data fetch from disk (at attachment time), or between
        //    their allocation and initialization (at map-growth time).
        //
        //  o We may hit failures while trying to read or write from/to a persisted flush map. If the failure happens while trying to
        //    read a page, the entire page will be initialized with pgftUnknown and marked as dirty. If the error happens while
        //    trying to write to a page, the page is going to marked dirty and its contents will be unchanged.
        //
        //  o We need to enter m_critFmdGrowCapacity to grow the file or its capacity (commit more desriptors, allocate more buffers,
        //    or initialize new buffers).
        //
        //  o ErrGetDescriptor..._() returns an unlacthed descriptor/page. Consumers need to decide what kind of latch they need to proceed.
        //    The descriptor is guaranteed to be valid, but not the page itself.
        //
        //  o Once we have a valid descriptor:
        //     1- A write latch is needed to allocate the actual page and read its contents from the disk if needed.
        //
        //     2- A write latch is needed to copy off the page in preparation for flushing it to disk.
        //
        //     3- A read latch is needed for all other operations.
        //
        //     4- Consumers of sync I/O operations are supposed to handle the latch transitions (acquire/release) themselves.
        //        A sync write expects the page to be write-latched and it downgrades the latch to an exclusive one once the page
        //        contents are copied to the write buffer. A sync read expects the page to be write-latched.
        //
        //     5- Consumers of async I/O operations are supposed to handle acquiring the write latch themselves, but the I/O
        //        helpers are responsible for releasing it. An async write expects the page to be write-latched and it downgrades
        //        the latch to an exclusive one once the page contents are copied to the write buffer. It then releases the exclusive
        //        latch once the I/O completes. An async read expects the page to be write-latched and it releases the latch once
        //        the I/O completes.
        //
        struct FlushMapPageDescriptor
        {
            public:
                // Data members.
                void* pv;                               // Pointer to the page.
                void* pvWriteBuffer;                    // Pointer to the write buffer.
                void* rgbitRuntime;                     // Pointer to an array of bits representing the runtime state of each flush state.
                DBTIME dbtimeMax;                       // Highest DB time of all DB pages represented by this map page.
                volatile ULONG fFlags;                  // Generic flags (see masks below).
                CSXWLatch sxwl;                         // Object to synchronize access to the page.
                FMPGNO fmpgno;                          // Flush map page number.
                ERR errAsyncWrite;                      // Error encountered during an async write.

            private:
                // Flag masks.
                static const ULONG fAllocated                   = 0x00000001;   // Whether or not the memory to hold the page in memory page is allocated.
                static const ULONG fValid                       = 0x00000002;   // Whether or not the page is in a valid state, i.e., the data page is
                                                                                // allocated and contents have been read in, in case of running with
                                                                            // persistence enabled.
                static const ULONG fDirty                       = 0x00000004;   // Whether or not the page is dirty.
                static const ULONG fReadInProgress              = 0x00000008;   // Whether or not the contents of the page are currently being read from disk.
                static const ULONG fSyncWriteInProgress         = 0x00000010;   // Whether or not a synchronous write of the page is in progress.
                static const ULONG fAsyncWriteInProgress        = 0x00000020;   // Whether or not an asynchronous write of the page is in progress.
                static const ULONG fRecentlyAsyncWrittenHeader  = 0x00000040;   // This is a header page that has recently completed a successful async flush.

            public:
                // Constructors/destructors.
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

                // Helper functions.
                BOOL FIsFmHeader() const
                {
                    return FIsFmHeader_( fmpgno );
                }

                // Flag getters.
                INLINE BOOL FAllocated() const { return ( ( fFlags & fAllocated ) != 0 ); }
                INLINE BOOL FValid() const { return ( ( fFlags & fValid ) != 0 ); }
                INLINE BOOL FDirty() const { return ( ( fFlags & fDirty ) != 0 ); }
                INLINE BOOL FReadInProgress() const { return ( ( fFlags & fReadInProgress ) != 0 ); }
                INLINE BOOL FSyncWriteInProgress() const { return ( ( fFlags & fSyncWriteInProgress ) != 0 ); }
                INLINE BOOL FAsyncWriteInProgress() const { return ( ( fFlags & fAsyncWriteInProgress ) != 0 ); }
                INLINE BOOL FRecentlyAsyncWrittenHeader() const { return ( ( fFlags & fRecentlyAsyncWrittenHeader ) != 0 ); }
                INLINE BOOL FWriteInProgress() const { return ( FSyncWriteInProgress() || FAsyncWriteInProgress() ); }

                // Flag setters.
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

        // Flush map file header.
        PERSISTED
        struct FMFILEHDR : public CZeroInit
        {
            union
            {
                // This is the fixed part of the header. Changes to the FM file header format must not
                // change this struct/union. Instead, the rest of the structure is supposed to be interpreted
                // based on the version read from the fixed part.
                PERSISTED
                struct
                {
                    LittleEndian<LONG>  checksum;                   // Checksum of the FM file header.
                    // 4 bytes.

                    LittleEndian<ULONG>         le_ulFmVersionMajor;            // FM version (major).
                    LittleEndian<ULONG>         le_ulFmVersionUpdateMajor;      // FM version (update major).
                    LittleEndian<ULONG>         le_ulFmVersionUpdateMinor;      // FM version (update minor).
                    // 16 bytes.

                    BYTE                        rgbFlagsAlignmentPadding[ 36 - 16 /* OffsetOf( FMFILEHDR, rgbFlagsAlignmentPadding ) */ ];
                    LittleEndian<ULONG>         fFlags;                     // Generic flags (see masks below).
                    // 40 bytes.
                };

                BYTE                            rgbFixedSizeBlob[ 40 ];
            };

            LOGTIME                     logtimeCreate;              // Time at which the FM was created.
            LOGTIME                     logtimeAttach;              // Last time at which the FM was last attached.
            LOGTIME                     logtimeDetach;              // Last time at which the FM was last detached.
            // 64 bytes.

            LittleEndian<LONG>          lGenMinRequired;            // Minimum log generation required for the FM to be rebuilt upon process crash.
            LittleEndian<LONG>          lGenMinRequiredTarget;      // Targeted log generation required for the FM to be rebuilt upon process crash.
                                                                    // If a full FM flush is currently under way, lGenMinRequiredTarget >= lGenMinRequired,
                                                                    // otherwise, lGenMinRequiredTarget == 0.
            // 72 bytes.

            LOGTIME                     logtimeFlushLastStarted;    // Last time a full FM flush started.
            // 80 bytes.

            LOGTIME                     logtimeFlushLastCompleted;  // Last time a full FM flush completed.
            // 88 bytes.

            SIGNATURE                   signDbHdrFlush;             // Random signature generated at the time of the last DB header flush.
            SIGNATURE                   signFlushMapHdrFlush;       // Random signature generated at the time of the last FM header flush.
            // 144 bytes.

            BYTE                            rgbFileTypeAlignmentPadding[ 667 - 144 /* OffsetOf( FMFILEHDR, rgbFileTypeAlignmentPadding ) */ ];
            // WARNING: MUST be placed at this offset for uniformity with db/log headers.
            UnalignedLittleEndian<ULONG>    filetype;               // File type: JET_filetypeFlushMap.
            //  671 bytes

            // Flag masks.
            static const ULONG fClean                   = 0x00000001;   // Whether or not the flush map is clean.
            static const ULONG fPageNewChecksumFormat   = 0x00002000;   // ECC checksum flag.

            // Constructor.
            FMFILEHDR() :
                CZeroInit( sizeof( FMFILEHDR ) )
            {
            }

            // Flag getters.
            INLINE BOOL FClean() const { return ( ( fFlags & fClean ) != 0 ); }
            INLINE BOOL FNewChecksumFormat() const { return ( ( fFlags & fPageNewChecksumFormat ) != 0 ); }

            // Flag setters.
            INLINE void SetClean() { fFlags |= fClean; }
            INLINE void ResetClean() { fFlags &= ~fClean; }
        };
        static_assert( FMFILEHDR::fPageNewChecksumFormat == CPAGE::fPageNewChecksumFormat, "Flush map header page and DB page new checksum format flag mask must match." );
        static_assert( OffsetOf( FMFILEHDR, checksum ) == OffsetOf( CPAGE::PGHDR, checksum ), "Offset of checksum in FMFILEHDR and CPAGE must match." );
        static_assert( OffsetOf( FMFILEHDR, fFlags ) == OffsetOf( CPAGE::PGHDR, fFlags ), "Offset of fFlags in FMFILEHDR and CPAGE must match." );
        static_assert( OffsetOf( FMFILEHDR, logtimeCreate ) == 40, "Size of the fixed part of FMFILEHDR is not expected to change. If intentional, make sure the upgrade is handled." );
        static_assert( OffsetOf( FMFILEHDR, filetype ) == 667, "Offset of filetype in FMFILEHDR must be fixed for compatibility with other file types." );
        static_assert( sizeof( FMFILEHDR ) == 671, "Size of FMFILEHDR is not expected to change. If intentional, make sure the upgrade is handled." );


        // Flush map data page header.
        PERSISTED
        struct FlushMapDataPageHdr : public CZeroInit
        {
            LittleEndian<XECHECKSUM>    checksum;                   // Checksum of the page.
            // 8 bytes.

            LittleEndian<DBTIME>        dbtimeMax;                  // Highest DB time of all DB pages represented by this map page.
            LittleEndian<FMPGNO>        fmpgno;                     // Page number.
            // 20 bytes.

            BYTE                        rgbFlagsAlignmentPadding[ 36 - 20 /* OffsetOf( FMFILEHDR, rgbFlagsAlignmentPadding ) */ ];
            LittleEndian<ULONG>         fFlags;                     // Generic flags (see masks below).

            BYTE                        rgbStructSizePadding[ 64 - 40 /* OffsetOf( FlushMapDataPageHdr, rgbStructSizePadding ) */ ];

            // Flag masks.
            static const ULONG fPageNewChecksumFormat   = 0x00002000;   // ECC checksum flag.

            // Constructor.
            FlushMapDataPageHdr() :
                CZeroInit( sizeof( FlushMapDataPageHdr ) )
            {
            }

            // Flag getters.
            INLINE BOOL FNewChecksumFormat() const { return ( ( fFlags & fPageNewChecksumFormat ) != 0 ); }
        };
        static_assert( flushmapDataPage == databasePage, "Flush map data page type must be the same as database page type for checksum compatibility." );
        static_assert( FlushMapDataPageHdr::fPageNewChecksumFormat == CPAGE::fPageNewChecksumFormat, "Flush map data page and DB page new checksum format flag mask must match." );
        static_assert( OffsetOf( FlushMapDataPageHdr, checksum ) == OffsetOf( CPAGE::PGHDR, checksum ), "Offset of checksum in FlushMapDataPageHdr and CPAGE must match." );
        static_assert( OffsetOf( FlushMapDataPageHdr, fFlags ) == OffsetOf( CPAGE::PGHDR, fFlags ), "Offset of fFlags in FlushMapDataPageHdr and CPAGE must match." );
        static_assert( sizeof( FlushMapDataPageHdr ) == 64, "Size of FlushMapDataPageHdr is not expected to change. If intentional, make sure the upgrade is handled." );

        #include <poppack.h>

    protected:

        LONG m_lgenTargetedMax;             // The maximum log generation targeted by the flush map to let the checkpoint advance to.

        BOOL m_fInitialized;                // Whether or not the object is initialized.

        BOOL m_fGetSetAllowed;              // Whether or not flush types can be read or changed.

        BOOL m_fDumpMode;                   // Whether or not we are running in dump mode.

        BOOL m_fPersisted_;                 // Whether or not the flush map is supposed to be persisted on disk.

        BOOL m_fPersisted;                  // Whether or not the flush map is currently being persisted on disk.

        BOOL m_fCreateNew_;                 // Whether or not a new flush map is supposed to be created upon attachment.

        BOOL m_fCreateNew;                  // Whether or not a new flush map was created upon attachment.

        BOOL m_fRecoverable;                // Whether or not the flush map is currently recoverable upon crash.

        BOOL m_fReadOnly;                   // Whether or not we should create a new file if needed or write to existing files.

        BOOL m_fCleanForTerm;               // Whether or not the map is considered clean for termination, i.e., no more writes are allowed.

        INST* m_pinst;                      // Instance object.

        IFileSystemAPI* m_pfsapi;           // File system object (only consumed when an m_pinst is not available and the flush map is persisted).

        IFileAPI* m_pfapi;                  // File object for I/O operations to the persisted flush map.

        LONG m_cInFlightIo;                 // Number of flush map pages undergoing I/O.

        CSXWLatch m_sxwlSectionFlush;           // Latch for synchronizing flushing sections of the map.

        FMPGNO m_fmpgnoSectionFlushFirst;       // First FM data page of the range currently undergoing a section flush operation.

        FMPGNO m_fmpgnoSectionFlushLast;        // Last FM data page of the range currently undergoing a section flush operation.

        FMPGNO m_fmpgnoSectionFlushNext;        // First FM data page of the next section to be flushed.

        BOOL m_fSectionCheckpointHeaderWrite;   // Signals that this is the final header write in the process of writing the entire map.
                                                // When we do a flush map checkpoint (i.e., a "flush" of the whole map), we always write
                                                // the header twice, once at the beginning of checkpoint (not important), and once at the
                                                // end (important, as this allows the lgen*Mins of DB / ESE checkpoint to move forward).

        ERR m_errSectionFlushWriteLastError;    // Last error seen during completion of a section flush.

        CMeteredSection m_msSectionFlush;                   // Metered section to synchronize completion of flushes of sections of the map.

        CMeteredSection::Group m_groupSectionFlushWrite;    // Metered section group in which the currently pending map section flushes are.

        void* m_pvSectionFlushAsyncWriteBuffer;             // Pointer to the async write buffer used in map section flushes.

        DWORD m_cbSectionFlushAsyncWriteBufferReserved;     // Size of the async write buffer used in map section flushes (reserved).

        DWORD m_cbSectionFlushAsyncWriteBufferCommitted;    // Size of the async write buffer used in map section flushes (committed).

        DWORD m_ibSectionFlushAsyncWriteBufferNext;         // Offset to be used next in the async write buffer used in map section flushes.

        QWORD m_cbLogGeneratedAtStartOfFlush;       // Amount of transaction log generated at the start of the map flush (never
                                                    // gets reset to ullMax, gets re-initialized every time a full flush initiates).

        QWORD m_cbLogGeneratedAtLastFlushProgress;  // Amount of transaction log generated at the last time a section write was
                                                    // issued against the map (gets reset to ullMax every time a full flush completes).

        DWORD m_cbFmPageInMemory;           // Size of the flush map page in memory.

        DWORD m_cbRuntimeBitmapPage;        // Size of the runtime bitmap page.

        DWORD m_cbDescriptorPage;           // Size of the descriptor page.

        USHORT m_cDbPagesPerFlushMapPage;   // Number of database pages per flush map page.

        FlushMapPageDescriptor m_fmdFmHdr;  // Descriptor for the header of the flush map file.

        FlushMapPageDescriptor m_fmdFmPg0;  // Descriptor for the first data page of the flush map (specialized so we don't need to reserve
                                            // a big chunk of memory for small clients).

        CCriticalSection m_critFmdGrowCapacity;     // Lock to protect growing the file size or capacity.

        FlushMapPageDescriptor* volatile m_rgfmd;   // Array of flush map descriptors, note that the first descriptor is never used because for the first
                                                    // flush map data page, we always use the pre-allocated m_fmdFmPg0.

        volatile CFMPG m_cfmdCommitted;     // Number of committed flush map descriptors, not including the static one (m_fmdFmPg0).
                                            // This is directly related to the committed memory m_cbfmdCommitted, i.e. m_cfmdCommitted could
                                            // be 0 even if m_rgfmd is non-NULL. This can happen if we've successfully reserved memory
                                            // but failed to commit the first time.

        volatile CFMPG m_cfmpgAllocated;    // Number of flush map pages allocated, not including the file header (m_fmdFmHdr).

        DWORD m_cbfmdReserved;      // Size of VM pages reserved to store flush map descriptors, in bytes.
                                    // Currently, maximum of ~4MB for 32-bit and ~6MB for 64-bit, respectively.

        DWORD m_cbfmdCommitted;     // Size of VM pages committed to store flush map descriptors, in bytes.

        DWORD m_cbfmAllocated;      // Size of VM pages allocated to hold flush map data pages, in bytes.


    protected:
        // Miscellaneous helpers.
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

        // Attach/term.
        ERR ErrAttachFlushMap_();
        void TermFlushMap_();

        // Descriptor and page initialization.
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

        // DB header flushes must call these to synchronize signatures.
        void EnterDbHeaderFlush_( SIGNATURE* const psignDbHdrFlush, SIGNATURE* const psignFlushMapHdrFlush );
        void LeaveDbHeaderFlush_();

        // I/O.
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
#endif  // DEBUG

        // Page validation.
        ERR ErrChecksumFmPage_( FlushMapPageDescriptor* const pfmd );
        ERR ErrValidateFmHdr_( FlushMapPageDescriptor* const pfmd );
        ERR ErrValidateFmDataPage_( FlushMapPageDescriptor* const pfmd );

        // Flush state manipulation.
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

        // Events.
        void LogEventFmFileAttachFailed_( const WCHAR* const wszFmFilePath, const ERR err );
        void LogEventFmFileDeleted_( const WCHAR* const wszFmFilePath, const WCHAR* const wszReason );
        void LogEventFmFileCreated_( const WCHAR* const wszFmFilePath );
        void LogEventFmHdrValidationFailed_( const ERR err, const WCHAR* const wszAdditionalInfo );
        void LogEventFmDataPageValidationFailed_( const ERR err, const FMPGNO fmpgno );
        void LogEventFmInconsistentDbTime_( const FMPGNO fmpgno, const DBTIME dbtimeMaxFm, const PGNO pgno, const DBTIME dbtimeMaxDb );


    // API Mechanics and Thread Safety
    //
    // The init/term functions NOT thread safe, i. e., they are not supposed to be called
    // concurrently with one another or concurrently with any of the thread-safe functions
    //
    // The functions to manipulate the flush map state ARE thread safe, i. e., they may be
    // called concurrently with one another, though they may not be called concurrently with
    // the non-thread-safe ones. They also may NOT be called outside of an init/term cycle.
    //
    // Additional notes:
    //  o Init functions may not be called multiple times without a call to a term function.
    //  o It is safe to call term functions multiple times.
    //  o The pure virtual functions that are part of the interface must be thread-safe once
    //    implemented by derived classes.
    //  o The pure virtual functions that are part of the interface must be able to provide
    //    valid answers by the time ErrInitFlushMap() is called until the object is destroyed.
    //  o Do not use a CFlushMapForDump object to read or alter flush types in the
    //    actual file. That derived class is designed for checksumming and dumping only.
    //

    protected:

        // Interface.
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

        static const BYTE s_pctCheckpointDelay = 20;    // How much to delay the checkpoint by to coalesce writes to the flush map (%-wise).

        // Constructor/destructor.
        CFlushMap();
        ~CFlushMap();

        // Configuration functions.
        void SetPersisted( __in const BOOL fPersisted );
        void SetCreateNew( __in const BOOL fCreateNew );

        // Initializes/terminates the flush map object.
        ERR ErrInitFlushMap();
        void TermFlushMap();    // Does not write the header or any other dirty pages.

        // Flushes/cleans the map. No more changes to the map are allowed after this operation.
        ERR ErrCleanFlushMap();

        // Ensures previous written IOs are flushed to persistent storage.
        ERR ErrFlushMapFlushFileBuffers( _In_ const IOFLUSHREASON iofr );

        // Getters/setters of flush types.
        void SetPgnoFlushType( __in const PGNO pgno, __in const CPAGE::PageFlushType pgft, __in const DBTIME dbtime );
        void SetPgnoFlushType( __in const PGNO pgno, __in const CPAGE::PageFlushType pgft );
        void SetRangeFlushType( __in const PGNO pgnoFirst, __in const CPG cpg, __in const CPAGE::PageFlushType pgft );
        ERR ErrSetPgnoFlushTypeAndWait( __in const PGNO pgno, __in const CPAGE::PageFlushType pgft, __in const DBTIME dbtime );
        ERR ErrSetRangeFlushTypeAndWait( __in const PGNO pgnoFirst, __in const CPG cpg, __in const CPAGE::PageFlushType pgft );
        CPAGE::PageFlushType PgftGetPgnoFlushType( __in const PGNO pgno );
        CPAGE::PageFlushType PgftGetPgnoFlushType( __in const PGNO pgno, __in const DBTIME dbtime, __out BOOL* const pfRuntime );
        ERR ErrSyncRangeInvalidateFlushType( __in const PGNO pgnoFirst, __in const CPG cpg );

        // Gets the minimum log generation required to bring the flush map to a consistent state upon crash.
        LONG LGetFmGenMinRequired();

        // Returns wether or not the flush map is recoverable.
        BOOL FRecoverable();

        // Returns wether or not the layer consuming the flush map should request a map flush.
        BOOL FRequestFmSectionWrite( __in const QWORD cbLogGenerated, __in const QWORD cbPreferredChktpDepth );

        // Issues one asynchronous write against the flush map. The write operation may comprise multiple map pages.
        void FlushOneSection( __in const QWORD cbLogGenerated );

        // Flushes the entire map (skips clean pages for better performance).
        ERR ErrFlushAllSections( OnDebug( __in const BOOL fMapPatching ) );

        // Grows the flush map to accomodate a certain number of database pages. If set to a smaller size,
        // the function returns immediately and does not change anything.
        ERR ErrSetFlushMapCapacity( __in const PGNO pgnoReq );

        // Gets the flush map page number that holds the state for a given database page.
        INLINE FMPGNO FmpgnoGetFmPgnoFromDbPgno( __in const PGNO pgno );

        // DB header flushes must call these to synchronize signatures.
        static void EnterDbHeaderFlush( __in CFlushMap* const pfm, __out SIGNATURE* const psignDbHdrFlush, __out SIGNATURE* const psignFlushMapHdrFlush );
        static void LeaveDbHeaderFlush( __in CFlushMap* const pfm );

        static ERR ErrGetFmPathFromDbPath(
            _Out_writes_z_( OSFSAPI_MAX_PATH ) WCHAR* const wszFmPath,
            _In_z_ const WCHAR* const wszDbPath );

    public:
        // Friend declarations.

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
#endif  // ENABLE_JET_UNIT_TEST

#ifdef DEBUGGER_EXTENSION
    friend LOCAL VOID EDBGUpdateFlushMapMemoryUsage(
        const CFlushMap* const pfm,
        DWORD* const pcbfmdReserved,
        DWORD* const pcbfmdCommitted,
        DWORD* const pcbfmAllocated );
#endif  // DEBUGGER_EXTENSION
};


// Implements CFlushMap for attached DBs, i.e., DBs that have a valid FMP.
class CFlushMapForAttachedDb : public CFlushMap
{
        // Configuration.
        FMP* m_pfmp;

    protected:
        // Interface implementation.
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
        // Constructor.
        CFlushMapForAttachedDb();
        
        // Configuration setters.
        void SetDbFmp( __in FMP* const pfmp );
};


// Implements CFlushMap for unattached DBs, i.e., DBs that do not have a valid FMP.
class CFlushMapForUnattachedDb : public CFlushMap
{
    private:
        // Configuration.
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
        // Interface implementation.
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
        // Constructor.
        CFlushMapForUnattachedDb();

        // Configuration setters.
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

        // Static helpers.

        // Creates and returns a persisted flush map object (or a null object) to be used when manipulating unattached DBs.
        // An unsuccessful error code means we hit a condition that would have prevented even a runtime map from being created.
        // In that case, the object returned is always NULL. OOM is one of such examples.
        // A successful error code may return either a NULL or a non-NULL object. A NULL object means that we could not create
        // or attach a valid persisted flush map. A non-NULL object represents a persisted and consistent flush map. A non-existent
        // or inconsistent flush map would trigger this condition.
        static ERR ErrGetPersistedFlushMapOrNullObjectIfRuntime(
            _In_z_ const WCHAR* const wszDbFilePath,
            __in const DBFILEHDR* const pdbHdr,
            __in INST* const pinst,
            __out CFlushMapForUnattachedDb** const ppfm );
};


// Implements CFlushMap for purposes of checksumming and dumping.
class CFlushMapForDump : public CFlushMap
{
    private:
        // Configuration.
        const WCHAR* m_wszFmFilePath_;
        INST* m_pinst_;
        IFileSystemConfiguration* m_pfsconfig_;

        // Helpers.
        void DumpFmPage_( const FlushMapPageDescriptor* const pfmd );
        void DumpFmHdr_( const FlushMapPageDescriptor* const pfmd );
        void DumpFmDataPage_( const FlushMapPageDescriptor* const pfmd, const BOOL fDumpFlushStates );

    protected:
        // Interface implementation.
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
        // Static helpers.
        static ERR ErrChecksumFlushMapFile( __in INST* const pinst, __in const WCHAR* const wszFmFilePath, __in_opt IFileSystemConfiguration* pfsconfig = NULL );
        static ERR ErrDumpFlushMapPage( __in INST* const pinst, __in const WCHAR* const wszFmFilePath, __in const FMPGNO fmpgno, __in const BOOL fDumpFlushStates, __in_opt IFileSystemConfiguration* pfsconfig = NULL );


    public:
        // Constructor.
        CFlushMapForDump();

        // Configuration setters.
        void SetFmFilePath( __in const WCHAR* const wszFmFilePath );
        void SetEseInstance( __in INST* const pinst );
        void SetFileSystemConfiguration( __in IFileSystemConfiguration* const pfsconfig );

        // Extended methods.
        ERR ErrDumpFmPage( __in const FMPGNO fmpgno, __in const BOOL fDumpFlushStates );
        ERR ErrChecksumFmPage( __in const FMPGNO fmpgno );
};

#endif  // !FLUSHMAP_H_INCLUDED


