// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSBLOCKCACHE_HXX_INCLUDED
#define _OSBLOCKCACHE_HXX_INCLUDED

#include "osfsapi.hxx"

//  Volume ID

enum class VolumeId : DWORD
{
    volumeidInvalid = 0,
};

constexpr VolumeId volumeidInvalid = VolumeId::volumeidInvalid;

//  File ID

enum class FileId : QWORD
{
    fileidInvalid = 0,
};

constexpr FileId fileidInvalid = FileId::fileidInvalid;

//  File Serial Number

enum class FileSerial : DWORD
{
    fileserialInvalid = 0,
};

constexpr FileSerial fileserialInvalid = FileSerial::fileserialInvalid;

//  File Identification

class IFileIdentification  //  fident
{
    public:

        virtual ~IFileIdentification() {}

        //  Returns the unique volume id and file id of a file by path.

        virtual ERR ErrGetFileId(   _In_z_  const WCHAR* const  wszPath,
                                    _Out_   VolumeId* const     pvolumeid,
                                    _Out_   FileId* const       pfileid ) = 0;

        //  Returns the unique path of a file by path.
        //
        //  The unique path returned for the file is intended for use in a hash table not for file system access.

        virtual ERR ErrGetFileKeyPath(  _In_z_                                  const WCHAR* const  wszPath,
                                        _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const        wszKeyPath ) = 0;

        //  Returns any absolute path and the unique path of a file by file id.
        //
        //  A given file may have many absolute paths due to multiple mount points, hard links, etc.
        //
        //  The unique path returned for the file is intended for use in a hash table not for file system access.

        virtual ERR ErrGetFilePathById( _In_                                    const VolumeId  volumeid,
                                        _In_                                    const FileId    fileid,
                                        _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const    wszAnyAbsPath,
                                        _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const    wszKeyPath ) = 0;
};

//  File System Filter

class IFileSystemFilter  //  fsf
    :   public IFileSystemAPI
{
    public:

        //  Opens the file specified with the specified access privileges and returns its handle.

        virtual ERR ErrFileOpenById(    _In_    const VolumeId                  volumeid,
                                        _In_    const FileId                    fileid,
                                        _In_    const FileSerial                fileserial,
                                        _In_    const IFileAPI::FileModeFlags   fmf,
                                        _Out_   IFileAPI** const                ppfapi ) = 0;

        //  Renames an open file.
        //
        //  The file must stay on the same volume.

        virtual ERR ErrFileRename(  _In_    IFileAPI* const    pfapi,
                                    _In_z_  const WCHAR* const wszPathDest,
                                    _In_    const BOOL         fOverwriteExisting ) = 0;
};

//  File Filter

class IFileFilter  //  ff
    :   public IFileAPI
{
    public:

        //  IO operation mode.

        enum class IOMode  //  iom
        {
            iomRaw = 0,
            iomEngine = 1,
            iomCacheMiss = 2,
            iomCacheWriteThrough = 3,
            iomCacheWriteBack = 4,
        };

    public:

        //  Returns the physical id of the current file.

        virtual ERR ErrGetPhysicalId(   _Out_ VolumeId* const   pvolumeid,
                                        _Out_ FileId* const     pfileid,
                                        _Out_ FileSerial* const pfileserial ) = 0;

        //  Reads a block of data of the specified size from the current file at the specified offset and
        //  places it into the specified buffer.  This is the same as IFileAPI::ErrIORead except it also
        //  takes an indication if the read is on behalf of the block cache.

        virtual ERR ErrRead(    _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _Out_writes_( cbData )  BYTE* const                     pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_                    const IFileFilter::IOMode       iom,
                                _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_                    const DWORD_PTR                 keyIOComplete,
                                _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                _In_                    const VOID *                    pioreq ) = 0;

        //  Writes a block of data of the specified size from the specified buffer at the specified offset and
        //  places it into the current file.  This is the same as IFileAPI::ErrIOWrite except it also
        //  takes an indication if the write is on behalf of the block cache.

        virtual ERR ErrWrite(   _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _In_reads_( cbData )    const BYTE* const               pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_                    const IFileFilter::IOMode       iom,
                                _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_                    const DWORD_PTR                 keyIOComplete,
                                _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff ) = 0;

        //  Causes any unissued asynchronous I/Os for the current file to be issued eventually.  This is the same as
        //  IFileAPI::ErrIOIssue except it also takes an indication if the write is on behalf of the block cache.

        virtual ERR ErrIssue( _In_ const IOMode iom ) = 0;
};

constexpr IFileFilter::IOMode iomRaw = IFileFilter::IOMode::iomRaw;
constexpr IFileFilter::IOMode iomEngine = IFileFilter::IOMode::iomEngine;
constexpr IFileFilter::IOMode iomCacheMiss = IFileFilter::IOMode::iomCacheMiss;
constexpr IFileFilter::IOMode iomCacheWriteThrough = IFileFilter::IOMode::iomCacheWriteThrough;
constexpr IFileFilter::IOMode iomCacheWriteBack = IFileFilter::IOMode::iomCacheWriteBack;

//  Cache

class ICache  //  c
{
    public:

        enum { cbGuid = sizeof(GUID) };

        //  Caching policy.

        enum class CachingPolicy  //  cp
        {
            cpDontCache = 0,    //  Don't cache the request.
            cpBestEffort = 1,   //  Perform best effort caching of the request.
        };

    public:

        virtual ~ICache() {}

        //  Creates a new cache.

        virtual ERR ErrCreate() = 0;

        //  Mounts an existing cache.

        virtual ERR ErrMount() = 0;

        //  Dumps information for an existing cache.

        virtual ERR ErrDump( _In_ CPRINTF* const pcprintf ) = 0;

        //  Returns the type of the caching file.

        virtual ERR ErrGetCacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType ) = 0;

        //  Returns the physical identity of the caching file.

        virtual ERR ErrGetPhysicalId(   _Out_                   VolumeId* const pvolumeid,
                                        _Out_                   FileId* const   pfileid,
                                        _Out_writes_( cbGuid )  BYTE* const     rgbUniqueId ) = 0;

        //  Closes the given cached file if currently open.

        virtual ERR ErrClose(   _In_ const VolumeId     volumeid,
                                _In_ const FileId       fileid,
                                _In_ const FileSerial   fileserial ) = 0;

        //  Flushes all data previously written for the given cached file.

        virtual ERR ErrFlush(   _In_ const VolumeId     volumeid,
                                _In_ const FileId       fileid,
                                _In_ const FileSerial   fileserial ) = 0;

        //  Invalidates cached data for the specified offset range of the given cached file.

        virtual ERR ErrInvalidate(  _In_ const VolumeId     volumeid,
                                    _In_ const FileId       fileid,
                                    _In_ const FileSerial   fileserial,
                                    _In_ const QWORD        ibOffset,
                                    _In_ const QWORD        cbData ) = 0;

        //  Cache read/write completion.

        typedef void (*PfnComplete)(    _In_                    const ERR               err,
                                        _In_                    const VolumeId          volumeid,
                                        _In_                    const FileId            fileid,
                                        _In_                    const FileSerial        fileserial,
                                        _In_                    const FullTraceContext& tc,
                                        _In_                    const OSFILEQOS         grbitQOS,
                                        _In_                    const QWORD             ibOffset,
                                        _In_                    const DWORD             cbData,
                                        _In_reads_( cbData )    const BYTE* const       pbData,
                                        _In_                    const DWORD_PTR         keyComplete );

        //  Reads a block of data of the specified size from the given cached file at the specified offset and places
        //  it into the specified buffer.
        //
        //  If asynchronous completion is requested by providing a completion function then there is no guarantee that
        //  completion will occur until ErrIssue() is called.

        virtual ERR ErrRead(    _In_                    const TraceContext&         tc,
                                _In_                    const VolumeId              volumeid,
                                _In_                    const FileId                fileid,
                                _In_                    const FileSerial            fileserial,
                                _In_                    const QWORD                 ibOffset,
                                _In_                    const DWORD                 cbData,
                                _Out_writes_( cbData )  BYTE* const                 pbData,
                                _In_                    const OSFILEQOS             grbitQOS,
                                _In_                    const ICache::CachingPolicy cp,
                                _In_opt_                const ICache::PfnComplete   pfnComplete,
                                _In_                    const DWORD_PTR             keyComplete ) = 0;

        //  Writes a block of data of the specified size from the specified buffer at the specified offset and places
        //  it into the given cached file.
        //
        //  If asynchronous completion is requested by providing a completion function then there is no guarantee that
        //  completion will occur until ErrIssue() is called.

        virtual ERR ErrWrite(   _In_                    const TraceContext&         tc,
                                _In_                    const VolumeId              volumeid,
                                _In_                    const FileId                fileid,
                                _In_                    const FileSerial            fileserial,
                                _In_                    const QWORD                 ibOffset,
                                _In_                    const DWORD                 cbData,
                                _In_reads_( cbData )    const BYTE* const           pbData,
                                _In_                    const OSFILEQOS             grbitQOS,
                                _In_                    const ICache::CachingPolicy cp,
                                _In_opt_                const ICache::PfnComplete   pfnComplete,
                                _In_                    const DWORD_PTR             keyComplete ) = 0;

        //  Causes any previously requested asynchronous cache reads or writes to be executed eventually.

        virtual ERR ErrIssue(   _In_ const VolumeId     volumeid,
                                _In_ const FileId       fileid,
                                _In_ const FileSerial   fileserial ) = 0;
};

constexpr ICache::CachingPolicy cpDontCache = ICache::CachingPolicy::cpDontCache;
constexpr ICache::CachingPolicy cpBestEffort = ICache::CachingPolicy::cpBestEffort;

//  Cache Telemetery

class ICacheTelemetry  //  ctm
{
    public:

        enum class FileNumber : DWORD
        {
            filenumberInvalid = dwMax,
        };

        enum class BlockNumber : DWORD
        {
            blocknumberInvalid = dwMax,
        };

    public:

        virtual ~ICacheTelemetry() {}

        //  Reports a cache miss for a file/block and if that block should be cached.

        virtual void Miss(  _In_ const FileNumber   filenumber, 
                            _In_ const BlockNumber  blocknumber,
                            _In_ const BOOL         fRead,
                            _In_ const BOOL         fCacheIfPossible ) = 0;

        //  Reports a cache hit for a file/block and if that block should be cached.

        virtual void Hit(   _In_ const FileNumber   filenumber,
                            _In_ const BlockNumber  blocknumber,
                            _In_ const BOOL         fRead,
                            _In_ const BOOL         fCacheIfPossible ) = 0;

        //  Reports an update of a file/block.

        virtual void Update(    _In_ const FileNumber   filenumber, 
                                _In_ const BlockNumber  blocknumber ) = 0;

        //  Reports a write of a file/block and if that write was due to limited resources.

        virtual void Write( _In_ const FileNumber   filenumber, 
                            _In_ const BlockNumber  blocknumber,
                            _In_ const BOOL         fReplacementPolicy ) = 0;

        //  Reports the removal of a file/block from the cache and if that removal was due to limited resources.

        virtual void Evict( _In_ const FileNumber   filenumber, 
                            _In_ const BlockNumber  blocknumber,
                            _In_ const BOOL         fReplacementPolicy ) = 0;
};

constexpr ICacheTelemetry::FileNumber filenumberInvalid = ICacheTelemetry::FileNumber::filenumberInvalid;

constexpr ICacheTelemetry::BlockNumber blocknumberInvalid = ICacheTelemetry::BlockNumber::blocknumberInvalid;

INLINE ICacheTelemetry::BlockNumber operator+( ICacheTelemetry::BlockNumber blocknumber, LONG i )
{ 
    return ( ICacheTelemetry::BlockNumber )( (LONG)blocknumber + i );
}

//  Cache Repository

class ICacheRepository  //  crep
{
    public:

        enum { cbGuid = sizeof(GUID) };

    public:

        virtual ~ICacheRepository() {}

        //  Gets the Cache for a given Cache Configuration.

        virtual ERR ErrOpen(    _In_    IFileSystemFilter* const        pfsf,
                                _In_    IFileSystemConfiguration* const pfsconfig,
                                _Inout_ ICacheConfiguration** const     ppcconfig,
                                _Out_   ICache** const                  ppc ) = 0;

        //  Gets an existing Cache by its physical identity.

        virtual ERR ErrOpenById(    _In_                IFileSystemFilter* const        pfsf,
                                    _In_                IFileSystemConfiguration* const pfsconfig,
                                    _In_                IBlockCacheConfiguration* const pbcconfig,
                                    _In_                const VolumeId                  volumeid,
                                    _In_                const FileId                    fileid,
                                    _In_reads_(cbGuid)  const BYTE* const               rgbUniqueId,
                                    _Out_               ICache** const                  ppc ) = 0;
};

//  Segment Position

enum class SegmentPosition : QWORD  //  spos
{
    sposInvalid = llMax,
};

constexpr SegmentPosition sposInvalid = SegmentPosition::sposInvalid;

INLINE SegmentPosition operator+( SegmentPosition spos, LONG64 i ) { return (SegmentPosition)( (LONG64)spos + i ); }
INLINE SegmentPosition operator-( SegmentPosition spos, LONG64 i ) { return (SegmentPosition)( (LONG64)spos - i ); }
INLINE LONG64 operator-( SegmentPosition sposA, SegmentPosition sposB ) { return (LONG64)sposA - (LONG64)sposB; }

//  Region Position

enum class RegionPosition : QWORD  //  rpos
{
    rposInvalid = llMax,
};

constexpr RegionPosition rposInvalid = RegionPosition::rposInvalid;

INLINE RegionPosition operator+( RegionPosition rpos, LONG64 i ) { return (RegionPosition)( (LONG64)rpos + i ); }
INLINE RegionPosition operator-( RegionPosition rpos, LONG64 i ) { return (RegionPosition)( (LONG64)rpos - i ); }
INLINE LONG64 operator-( RegionPosition rposA, RegionPosition rposB ) { return (LONG64)rposA - (LONG64)rposB; }

//  Journal Buffer

class CJournalBuffer  //  jb
{
    public:

        CJournalBuffer()
            :   CJournalBuffer( 0, NULL )
        {
        }

        CJournalBuffer( _In_                const size_t        cb,
                        _In_reads_( cb )    const BYTE* const   rgb )
            :   m_cb( cb ),
                m_rgb( rgb )
        {
        }

        size_t Cb() const { return m_cb; }
        const BYTE* Rgb() const { return m_rgb; }

    private:

        size_t      m_cb;
        const BYTE* m_rgb;
};

//  Journal Segment

class IJournalSegment  //  js
{
    public:

        virtual ~IJournalSegment() {}

        //  Gets the physical identity of the segment.

        virtual ERR ErrGetPhysicalId( _Out_ QWORD* const pib ) = 0;

        //  Gets the current properties of the segment.

        virtual ERR ErrGetProperties(   _Out_opt_ SegmentPosition* const    pspos,
                                        _Out_opt_ DWORD* const              pdwUniqueId,
                                        _Out_opt_ DWORD* const              pdwUniqueIdPrev,
                                        _Out_opt_ SegmentPosition* const    psposReplay,
                                        _Out_opt_ SegmentPosition* const    psposDurable,
                                        _Out_opt_ BOOL* const               pfSealed ) = 0;
        
        //  Callback used to visit a region in the segment.
        //
        //  The callback must return true to continue visiting more regions.

        typedef BOOL (*PfnVisitRegion)( _In_ const RegionPosition   rpos,
                                        _In_ const CJournalBuffer   jb,
                                        _In_ const DWORD_PTR        keyVisitRegion );

        //  Visits every region in the segment.

        virtual ERR ErrVisitRegions(    _In_ const IJournalSegment::PfnVisitRegion  pfnVisitRegion,
                                        _In_ const DWORD_PTR                        keyVisitRegion ) = 0;

        //  Attempts to append a region to the segment containing a portion of the provided payload.
        //
        //  The actual amount of the payload appended is returned.  This can be zero if the minimum payload could not
        //  be appended.

        virtual ERR ErrAppendRegion(    _In_                const size_t            cjb,
                                        _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                                        _In_                const DWORD             cbMin,
                                        _Out_               RegionPosition* const   rpos,
                                        _Out_               DWORD* const            pcbActual ) = 0;
        
        //  Callback used to indicate that a segment is sealed.

        typedef void (*PfnSealed)(  _In_ const ERR              err,
                                    _In_ const SegmentPosition  spos,
                                    _In_ const DWORD_PTR        keySealed );

        //  Closes the segment to further appends and writes it to storage.

        virtual ERR ErrSeal( _In_opt_ const IJournalSegment::PfnSealed pfnSealed, _In_ const DWORD_PTR keySealed ) = 0;
};

//  Journal Segment Manager

class IJournalSegmentManager  //  jsm
{
    public:

        virtual ~IJournalSegmentManager() {}
       
        //  Gets the current properties of the set of segments.

        virtual ERR ErrGetProperties(   _Out_opt_ SegmentPosition* const    psposReplay,
                                        _Out_opt_ SegmentPosition* const    psposDurable,
                                        _Out_opt_ SegmentPosition* const    psposLast ) = 0;
 
        //  Callback used to visit a segment in the journal.
        //
        //  If a particular segment cannot be loaded then the callback will be invoked with the failure and the segment
        //  will not be provided.
        //
        //  The callback must return true to continue visiting more segments.

        typedef BOOL (*PfnVisitSegment)(    _In_ const SegmentPosition  spos,
                                            _In_ const ERR              err,
                                            _In_ IJournalSegment* const pjs,
                                            _In_ const DWORD_PTR        keyVisitSegment );

        //  Visits every segment in the journal.

        virtual ERR ErrVisitSegments(   _In_ const IJournalSegmentManager::PfnVisitSegment  pfnVisitSegment,
                                        _In_ const DWORD_PTR                                keyVisitSegment ) = 0;

        //  Appends a new segment to the journal, reusing an existing segment if necessary.
        //
        //  A sealed segment can become eligible for reuse if the provided sposReplay for the new segment causes an
        //  existing segment to fall out of scope for replay.
        //
        //  A sealed segment can also be reused directly to fixup the tail of the journal at mount time.  This segment
        //  must come after the last known durable segment.
        //
        //  If a request would cause the journal to become 100% full then it will fail.
        //
        //  If a request would cause the journal to fail to recover due to invalid metadata then it will fail.
        //
        //  If a request would make sposReplay or sposDurable to move backwards then it will fail.

        virtual ERR ErrAppendSegment(   _In_    const SegmentPosition   spos,
                                        _In_    const SegmentPosition   sposReplay,
                                        _In_    const SegmentPosition   sposDurable,
                                        _Out_   IJournalSegment** const ppjs ) = 0;

        //  Causes all sealed segments to become durable.
        //
        //  This call will block until these segments are durable.

        virtual ERR ErrFlush() = 0;
};

//  Journal Position

enum class JournalPosition : QWORD  //  jpos
{
    jposInvalid = llMax,
};

constexpr JournalPosition jposInvalid = JournalPosition::jposInvalid;

//  Journal

class IJournal  //  j
{
    public:

        virtual ~IJournal() {}
       
        //  Gets the current properties of the journal.
        //
        //      replay:  The approximate position required for replay.
        //      durable for write back:  The approximate position of the last durable entry for which write back may occur.
        //      durable commit:  The approximate position of the last durable entry.

        virtual ERR ErrGetProperties(   _Out_opt_ JournalPosition* const    pjposReplay,
                                        _Out_opt_ JournalPosition* const    pjposDurableForWriteBack,
                                        _Out_opt_ JournalPosition* const    pjposDurable ) = 0;

        //  Callback used to visit an entry in the journal.
        //
        //  The callback must return true to continue visiting more entries.

        typedef BOOL (*PfnVisitEntry)(  _In_ const JournalPosition  jpos,
                                        _In_ const JournalPosition  jposEnd,
                                        _In_ const CJournalBuffer   jb,
                                        _In_ const DWORD_PTR        keyVisitEntry );

        //  Visits every entry in the journal.

        virtual ERR ErrVisitEntries(    _In_ const IJournal::PfnVisitEntry  pfnVisitEntry,
                                        _In_ const DWORD_PTR                keyVisitEntry ) = 0;

        //  Indicates that all journal entries newer than the specified entry are invalid and should be overwritten.
        //
        //  The repair may invalidate a limited number of journal entries prior to the invalidate position due to the
        //  internal format of the journal.  This includes the possibility of losing a journal entry due to truncation.
        //  The actual journal position invalidated will be returned.

        virtual ERR ErrRepair(  _In_    const JournalPosition   jposInvalidate,
                                _Out_   JournalPosition* const  pjposInvalidated ) = 0;

        //  Attempts to append an entry to the journal with the provided payload.
        //
        //  If there is not enough room in the journal to append the entry then the operation will fail.  ErrTruncate
        //  must be called with a sufficiently advanced replay pointer to resolve the situation.

        virtual ERR ErrAppendEntry( _In_                const size_t            cjb,
                                    _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                                    _Out_               JournalPosition* const  pjpos,
                                    _Out_               JournalPosition* const  pjposEnd ) = 0;

        //  Causes all previously appended entries to become durable.
        //
        //  This call will block until these entries are durable.

        virtual ERR ErrFlush() = 0;

        //  Indicates that all journal entries older than the specified entry are no longer required.

        virtual ERR ErrTruncate( _In_ const JournalPosition jposReplay ) = 0;
};

//  Factory methods

ERR ErrOSBCCreateFileSystemWrapper( _Inout_ IFileSystemAPI** const  ppfsapiInner,
                                    _Out_   IFileSystemAPI** const  ppfsapi );

ERR ErrOSBCCreateFileSystemFilter(  _In_    IFileSystemConfiguration* const pfsconfig,
                                    _Inout_ IFileSystemAPI** const          ppfsapiInner,
                                    _In_    IFileIdentification* const      pfident,
                                    _In_    ICacheTelemetry* const          pctm,
                                    _In_    ICacheRepository* const         pcrep,
                                    _Out_   IFileSystemFilter** const       ppfsf );

ERR ErrOSBCCreateFileWrapper(   _Inout_ IFileAPI** const    ppfapiInner,
                                _Out_   IFileAPI** const    ppfapi );

ERR ErrOSBCCreateFileFilter(    _Inout_                     IFileAPI** const                    ppfapiInner,
                                _In_                        IFileSystemFilter* const            pfsf,
                                _In_                        IFileSystemConfiguration* const     pfsconfig,
                                _In_                        ICacheTelemetry* const              pctm,
                                _In_                        const VolumeId                      volumeid,
                                _In_                        const FileId                        fileid,
                                _Inout_                     ICachedFileConfiguration** const    ppcfconfig,
                                _Inout_                     ICache** const                      ppc,
                                _In_reads_opt_( cbHeader )  const BYTE* const                   pbHeader,
                                _In_                        const int                           cbHeader,
                                _Out_                       IFileFilter** const                 ppff );
ERR ErrOSBCCreateFileFilterWrapper( _Inout_ IFileFilter** const         ppffInner,
                                    _In_    const IFileFilter::IOMode   iom,
                                    _Out_   IFileFilter** const         ppff );

ERR ErrOSBCCreateFileIdentification( _Out_ IFileIdentification** const ppfident );

ERR ErrOSBCCreateCache( _In_    IFileSystemFilter* const        pfsf,
                        _In_    IFileIdentification* const      pfident,
                        _In_    IFileSystemConfiguration* const pfsconfig,
                        _Inout_ ICacheConfiguration** const     ppcconfig,
                        _In_    ICacheTelemetry* const          pctm,
                        _Inout_ IFileFilter** const             ppffCaching,
                        _Out_   ICache** const                  ppc );

ERR ErrOSBCCreateCacheWrapper(  _Inout_ ICache** const  ppcInner,
                                _Out_   ICache** const  ppc );

ERR ErrOSBCCreateCacheRepository(   _In_    IFileIdentification* const      pfident,
                                    _In_    ICacheTelemetry* const          pctm,
                                    _Out_   ICacheRepository** const        ppcrep );

ERR ErrOSBCCreateCacheTelemetry( _Out_ ICacheTelemetry** const ppctm );

ERR ErrOSBCDumpCachedFileHeader(    _In_z_  const WCHAR* const  wszFilePath,
                                    _In_    const ULONG         grbit,
                                    _In_    CPRINTF* const      pcprintf );
ERR ErrOSBCDumpCacheFile(   _In_z_  const WCHAR* const  wszFilePath, 
                            _In_    const ULONG         grbit,
                            _In_    CPRINTF* const      pcprintf );

ERR ErrOSBCCreateJournalSegment(    _In_    IFileFilter* const      pff,
                                    _In_    const QWORD             ib,
                                    _In_    const SegmentPosition   spos,
                                    _In_    const DWORD             dwUniqueIdPrev,
                                    _In_    const SegmentPosition   sposReplay,
                                    _In_    const SegmentPosition   sposDurable,
                                    _Out_   IJournalSegment** const ppjs );

ERR ErrOSBCLoadJournalSegment(  _In_    IFileFilter* const      pff,
                                _In_    const QWORD             ib,
                                _Out_   IJournalSegment** const ppjs );

ERR ErrOSBCCreateJournalSegmentManager( _In_    IFileFilter* const              pff,
                                        _In_    const QWORD                     ib,
                                        _In_    const QWORD                     cb,
                                        _Out_   IJournalSegmentManager** const  ppjsm );

ERR ErrOSBCCreateJournal(   _Inout_ IJournalSegmentManager** const  ppjsm,
                            _In_    const size_t                    cbCache,
                            _Out_   IJournal** const                ppj );

#endif  //  _OSBLOCKCACHE_HXX_INCLUDED

