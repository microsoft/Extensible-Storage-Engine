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

        static const size_t cwchKeyPathMax  = 49 + 256 + 1;  //  "\\?\Volume{00000000-0000-0000-0000-000000000000}\" + 256 char local path + "\0"
        static const size_t cbKeyPathMax    = cwchKeyPathMax * sizeof( WCHAR );

        virtual ~IFileIdentification() {}

        //  Returns the unique volume id and file id of a file by path.

        virtual ERR ErrGetFileId(   _In_z_  const WCHAR* const  wszPath,
                                    _Out_   VolumeId* const     pvolumeid,
                                    _Out_   FileId* const       pfileid ) = 0;

        //  Returns the unique path of a file by path.
        //
        //  The unique path returned for the file is intended for use in a hash table not for file system access.

        virtual ERR ErrGetFileKeyPath(  _In_z_                                                  const WCHAR* const  wszPath,
                                        _Out_bytecap_c_( IFileIdentification::cbKeyPathMax )    WCHAR* const        wszKeyPath ) = 0;

        //  Returns any absolute path and the unique path of a file by file id.
        //
        //  A given file may have many absolute paths due to multiple mount points, hard links, etc.
        //
        //  The unique path returned for the file is intended for use in a hash table not for file system access.

        virtual ERR ErrGetFilePathById( _In_                                                    const VolumeId  volumeid,
                                        _In_                                                    const FileId    fileid,
                                        _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )                  WCHAR* const    wszAnyAbsPath,
                                        _Out_bytecap_c_( IFileIdentification::cbKeyPathMax )    WCHAR* const    wszKeyPath ) = 0;
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
                                _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_opt_                const DWORD_PTR                 keyIOComplete,
                                _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                _In_opt_                const VOID *                    pioreq ) = 0;

        //  Writes a block of data of the specified size from the specified buffer at the specified offset and
        //  places it into the current file.  This is the same as IFileAPI::ErrIOWrite except it also
        //  takes an indication if the write is on behalf of the block cache.

        virtual ERR ErrWrite(   _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _In_reads_( cbData )    const BYTE* const               pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_                    const IFileFilter::IOMode       iom,
                                _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_opt_                const DWORD_PTR                 keyIOComplete,
                                _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff ) = 0;

        //  Causes any unissued asynchronous I/Os for the current file to be issued eventually.  This is the same as
        //  IFileAPI::ErrIOIssue except it also takes an indication if the write is on behalf of the block cache.

        virtual ERR ErrIssue( _In_ const IOMode iom ) = 0;

        //  Flushes all data previously written for the current file.

        virtual ERR ErrFlush( _In_ const IOFLUSHREASON iofr, _In_ const IFileFilter::IOMode iom ) = 0;
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
            cpPinned = 2,       //  Caching required and writeback is not allowed.
        };

    public:

        virtual ~ICache() {}

        //  Creates a new cache.

        virtual ERR ErrCreate() = 0;

        //  Mounts an existing cache.

        virtual ERR ErrMount() = 0;

        //  Dumps information for an existing cache.

        virtual ERR ErrDump( _In_ CPRINTF* const pcprintf ) = 0;

        //  Indicates if the cache is currently enabled.

        virtual BOOL FEnabled() = 0;

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

        //  Destage completion status.

        typedef void (*PfnDestageStatus)(   _In_        const int       i,
                                            _In_        const int       c,
                                            _In_opt_    const DWORD_PTR keyDestageStatus );

        //  Writes all data previously written for the given cached file back to that cached file.

        virtual ERR ErrDestage( _In_        const VolumeId                  volumeid,
                                _In_        const FileId                    fileid,
                                _In_        const FileSerial                fileserial,
                                _In_opt_    const ICache::PfnDestageStatus  pfnDestageStatus,
                                _In_opt_    const DWORD_PTR                 keyDestageStatus ) = 0;

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
                                _In_opt_                const DWORD_PTR             keyComplete ) = 0;

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
                                _In_opt_                const DWORD_PTR             keyComplete ) = 0;

        //  Causes any previously requested asynchronous cache reads or writes to be executed eventually.

        virtual ERR ErrIssue(   _In_ const VolumeId     volumeid,
                                _In_ const FileId       fileid,
                                _In_ const FileSerial   fileserial ) = 0;
};

constexpr ICache::CachingPolicy cpDontCache = ICache::CachingPolicy::cpDontCache;
constexpr ICache::CachingPolicy cpBestEffort = ICache::CachingPolicy::cpBestEffort;
constexpr ICache::CachingPolicy cpPinned = ICache::CachingPolicy::cpPinned;

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

INLINE ICacheTelemetry::BlockNumber operator+( _In_ const ICacheTelemetry::BlockNumber blocknumber, _In_ const LONG i )
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

INLINE SegmentPosition operator+( _In_ const SegmentPosition spos, _In_ const LONG64 i ) { return (SegmentPosition)( (LONG64)spos + i ); }
INLINE SegmentPosition operator-( _In_ const SegmentPosition spos, _In_ const LONG64 i ) { return (SegmentPosition)( (LONG64)spos - i ); }
INLINE LONG64 operator-( _In_ const SegmentPosition sposA, _In_ const SegmentPosition sposB ) { return (LONG64)sposA - (LONG64)sposB; }

//  Region Position

enum class RegionPosition : QWORD  //  rpos
{
    rposInvalid = llMax,
};

constexpr RegionPosition rposInvalid = RegionPosition::rposInvalid;

INLINE RegionPosition operator+( _In_ const RegionPosition rpos, _In_ const LONG64 i ) { return (RegionPosition)( (LONG64)rpos + i ); }
INLINE RegionPosition operator-( _In_ const RegionPosition rpos, _In_ const LONG64 i ) { return (RegionPosition)( (LONG64)rpos - i ); }
INLINE LONG64 operator-( _In_ const RegionPosition rposA, _In_ const RegionPosition rposB ) { return (LONG64)rposA - (LONG64)rposB; }

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
                                        _In_ const RegionPosition   rposEnd,
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
                                        _Out_               RegionPosition* const   prpos,
                                        _Out_               RegionPosition* const   prposEnd,
                                        _Out_               DWORD* const            pcbActual ) = 0;
        
        //  Callback used to indicate that a segment is sealed.

        typedef void (*PfnSealed)(  _In_ const ERR              err,
                                    _In_ const SegmentPosition  spos,
                                    _In_ const DWORD_PTR        keySealed );

        //  Closes the segment to further appends and writes it to storage.

        virtual ERR ErrSeal(    _In_opt_ const IJournalSegment::PfnSealed   pfnSealed,
                                _In_opt_ const DWORD_PTR                    keySealed ) = 0;
};

//  Journal Segment Manager

class IJournalSegmentManager  //  jsm
{
    public:

        virtual ~IJournalSegmentManager() {}
       
        //  Gets the current properties of the set of segments.
        //
        //      first:  The position of the first segment.
        //      replay:  The position of the replay segment.
        //      durable:  The position of the last durable segment.
        //      last:  The position of the last segment.
        //      full:  The position of the segment at which the journal will be full.

        virtual ERR ErrGetProperties(   _Out_opt_ SegmentPosition* const    psposFirst,
                                        _Out_opt_ SegmentPosition* const    psposReplay,
                                        _Out_opt_ SegmentPosition* const    psposDurable,
                                        _Out_opt_ SegmentPosition* const    psposLast,
                                        _Out_opt_ SegmentPosition* const    psposFull ) = 0;
 
        //  Callback used to visit a segment in the journal.
        //
        //  If a particular segment cannot be loaded then the callback will be invoked with the failure and the segment
        //  will not be provided.
        //
        //  The callback must return true to continue visiting more segments.

        typedef BOOL (*PfnVisitSegment)(    _In_        const SegmentPosition  spos,
                                            _In_        const ERR              err,
                                            _In_opt_    IJournalSegment* const pjs,
                                            _In_        const DWORD_PTR        keyVisitSegment );

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

INLINE JournalPosition operator+( _In_ const JournalPosition jpos, _In_ const LONG64 i ) { return (JournalPosition)( (LONG64)jpos + i ); }
INLINE JournalPosition operator-( _In_ const JournalPosition jpos, _In_ const LONG64 i ) { return (JournalPosition)( (LONG64)jpos - i ); }
INLINE LONG64 operator-( _In_ const JournalPosition jposA, _In_ const JournalPosition jposB ) { return (LONG64)jposA - (LONG64)jposB; }

//  Journal

class IJournal  //  j
{
    public:

        virtual ~IJournal() {}
       
        //  Gets the current properties of the journal.
        //
        //      replay:  The approximate position required for replay.
        //      durable for write back:  The approximate position of the last durable entry for which write back may occur.
        //      durable:  The approximate position of the last durable entry.
        //      append:  The approximate position that will be assigned to the next append.
        //      full:  The position at which the journal will be full.

        virtual ERR ErrGetProperties(   _Out_opt_ JournalPosition* const    pjposReplay,
                                        _Out_opt_ JournalPosition* const    pjposDurableForWriteBack,
                                        _Out_opt_ JournalPosition* const    pjposDurable,
                                        _Out_opt_ JournalPosition* const    pjposAppend,
                                        _Out_opt_ JournalPosition* const    pjposFull ) = 0;

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

//  Cached Block Size

constexpr size_t cbCachedBlock = 4096;

//  Cached Block Number

//PERSISTED
enum class CachedBlockNumber : DWORD  //  cbno
{
    cbnoInvalid = dwMax,
};

constexpr CachedBlockNumber cbnoInvalid = CachedBlockNumber::cbnoInvalid;

INLINE CachedBlockNumber operator+( _In_ const CachedBlockNumber cbno, _In_ const LONG i ) { return (CachedBlockNumber)( (LONG)cbno + i ); }

//  Cached Block Id

#pragma warning (push)
#pragma warning (disable: 4315)  //  '': 'this' pointer for member '' may not be aligned X as expected by the constructor

#include <pshpack1.h>

//PERSISTED
class CCachedBlockId  //  cbid
{
    public:

        CCachedBlockId()
            :   m_le_volumeid( volumeidInvalid ),
                m_le_fileid( fileidInvalid ),
                m_le_fileserial( fileserialInvalid ),
                m_le_cbno( cbnoInvalid )
        {
        }

        CCachedBlockId( _In_ const VolumeId             volumeid,
                        _In_ const FileId               fileid,
                        _In_ const FileSerial           fileserial,
                        _In_ const CachedBlockNumber    cbno )
            :   m_le_volumeid( volumeid ),
                m_le_fileid( fileid ),
                m_le_fileserial( fileserial ),
                m_le_cbno( cbno )
        {
            C_ASSERT( 20 == sizeof( CCachedBlockId ) );
        }

        VolumeId Volumeid() const { return m_le_volumeid; }
        FileId Fileid() const { return m_le_fileid; }
        FileSerial Fileserial() const { return m_le_fileserial; }
        CachedBlockNumber Cbno() const { return m_le_cbno; }

    private:

        const UnalignedLittleEndian<VolumeId>           m_le_volumeid;      //  the volume id of the cached file
        const UnalignedLittleEndian<FileId>             m_le_fileid;        //  the file id of the cached file
        const UnalignedLittleEndian<FileSerial>         m_le_fileserial;    //  the serial number of the cached file
        const UnalignedLittleEndian<CachedBlockNumber>  m_le_cbno;          //  the cached block number of the cached file for the data
};

#include <poppack.h>

#pragma warning (pop)

//  Cluster Number

//PERSISTED
enum class ClusterNumber : DWORD  //  clno
{
    clnoInvalid = dwMax,
};

constexpr ClusterNumber clnoInvalid = ClusterNumber::clnoInvalid;

INLINE ClusterNumber operator+( _In_ const ClusterNumber clno, _In_ const LONG i ) { return (ClusterNumber)( (LONG)clno + i ); }

//  Touch Number
//
//  NOTE:  this number is truncated to 32 bits.  wrap around aware comparisons are provided

//PERSISTED
enum class TouchNumber : DWORD  //  tono
{
    tonoInvalid = 0,
};

constexpr TouchNumber tonoInvalid = TouchNumber::tonoInvalid;

INLINE TouchNumber operator+( _In_ const TouchNumber tono, _In_ const LONG i ) { return ( (LONG)tono + i == (LONG)tonoInvalid ) ? (TouchNumber)( (LONG)tonoInvalid + 1 ) : (TouchNumber)( (LONG)tono + i ); }
INLINE int CmpTono( _In_ const TouchNumber tonoA, _In_ const TouchNumber tonoB ) { return (LONG)( (DWORD)tonoA - (DWORD)tonoB ); }
INLINE BOOL operator<( _In_ const TouchNumber tonoA, _In_ const TouchNumber tonoB ) { return CmpTono( tonoA, tonoB ) < 0; }
INLINE BOOL operator<=( _In_ const TouchNumber tonoA, _In_ const TouchNumber tonoB ) { return CmpTono( tonoA, tonoB ) <= 0; }
INLINE BOOL operator>( _In_ const TouchNumber tonoA, _In_ const TouchNumber tonoB ) { return CmpTono( tonoA, tonoB ) > 0; }
INLINE BOOL operator>=( _In_ const TouchNumber tonoA, _In_ const TouchNumber tonoB ) { return CmpTono( tonoA, tonoB ) >= 0; }

//  Update Number
//
//  NOTE:  this is intentionally truncated to a small number of bits because only so many versions of the same cluster
//  could possibly coexist in the same slab.  wrap around aware comparisons are provided

//PERSISTED
enum class UpdateNumber : USHORT  //  updno
{
    updnoInvalid = 0,
    updnoMax = 65535,
};

constexpr UpdateNumber updnoInvalid = UpdateNumber::updnoInvalid;
constexpr UpdateNumber updnoMax = UpdateNumber::updnoMax;

INLINE UpdateNumber operator+( _In_ const UpdateNumber updno, _In_ const LONG i ) { return ( (LONG)updno + i > (LONG)updnoMax ) ? (UpdateNumber)( (LONG)updnoInvalid + 1 ) : (UpdateNumber)( (LONG)updno + i ); }
INLINE int CmpUpdno( _In_ const UpdateNumber updnoA, _In_ const UpdateNumber updnoB ) { return (SHORT)( (USHORT)updnoA - (USHORT)updnoB ); }
INLINE BOOL operator<( _In_ const UpdateNumber updnoA, _In_ const UpdateNumber updnoB ) { return CmpUpdno( updnoA, updnoB ) < 0; }
INLINE BOOL operator<=( _In_ const UpdateNumber updnoA, _In_ const UpdateNumber updnoB ) { return CmpUpdno( updnoA, updnoB ) <= 0; }
INLINE BOOL operator>( _In_ const UpdateNumber updnoA, _In_ const UpdateNumber updnoB ) { return CmpUpdno( updnoA, updnoB ) > 0; }
INLINE BOOL operator>=( _In_ const UpdateNumber updnoA, _In_ const UpdateNumber updnoB ) { return CmpUpdno( updnoA, updnoB ) >= 0; }

//  Cached Block

#pragma warning (push)
#pragma warning (disable: 4315)  //  '': 'this' pointer for member '' may not be aligned X as expected by the constructor

#include <pshpack1.h>

//PERSISTED
class CCachedBlock  //  cbl
{
    public:

        CCachedBlock()
            :   m_cbid(),
                m_le_clno( clnoInvalid ),
                m_le_dwECC( 0 ),
                m_le_rgtono { tonoInvalid, tonoInvalid },
                m_fValid( 0 ),
                m_fPinned( 0 ),
                m_fDirty( 0 ),
                m_fEverDirty( 0 ),
                m_fPurged( 0 ),
                m_rgbitReserved0( 0 ),
                m_le_updno( updnoInvalid )
        {
        }

        const CCachedBlockId& Cbid() const { return m_cbid; }
        ClusterNumber Clno() const { return m_le_clno; }
        BOOL FValid() const { return m_fValid != 0; }
        BOOL FPinned() const { return m_fPinned != 0; }
        BOOL FDirty() const { return m_fDirty != 0; }
        BOOL FEverDirty() const { return m_fEverDirty != 0; }
        BOOL FPurged() const { return m_fPurged != 0; }

    protected:

        friend class CCachedBlockSlab;
        template<class I> friend class TCachedBlockSlab;
        friend class CCachedBlockSlot;
        friend class COSBlockCacheFactoryImpl;

        CCachedBlock(   _In_ const CCachedBlockId   cbid,
                        _In_ const ClusterNumber    clno,
                        _In_ const DWORD            dwECC,
                        _In_ const TouchNumber      tono0,
                        _In_ const TouchNumber      tono1,
                        _In_ const BOOL             fValid,
                        _In_ const BOOL             fPinned,
                        _In_ const BOOL             fDirty,
                        _In_ const BOOL             fEverDirty,
                        _In_ const BOOL             fPurged,
                        _In_ const UpdateNumber     updno )
            :   m_cbid( cbid ),
                m_le_clno( clno ),
                m_le_dwECC( dwECC ),
                m_le_rgtono { tono0, tono1 },
                m_fValid( fValid ? 1 : 0 ),
                m_fPinned( fPinned ? 1 : 0 ),
                m_fDirty( fDirty ? 1 : 0 ),
                m_fEverDirty( fEverDirty ? 1 : 0 ),
                m_fPurged( fPurged ? 1 : 0 ),
                m_rgbitReserved0( 0 ),
                m_le_updno( updno )
        {
            C_ASSERT( 39 == sizeof( CCachedBlock ) );
        }

        DWORD DwECC() const { return m_le_dwECC; }
        TouchNumber Tono0() const { return m_le_rgtono[ 0 ]; }
        TouchNumber Tono1() const { return m_le_rgtono[ 1 ]; }
        UpdateNumber Updno() const { return m_le_updno; }

        BYTE RgbitReserved0() const { return m_rgbitReserved0; }

    private:

        const CCachedBlockId                            m_cbid;                 //  the cached block id
        const UnalignedLittleEndian<ClusterNumber>      m_le_clno;              //  the cluster number of the caching file holding the cached block
        const UnalignedLittleEndian<DWORD>              m_le_dwECC;             //  ECC of the block's contents
        const UnalignedLittleEndian<TouchNumber>        m_le_rgtono[ 2 ];       //  touch numbers for the replacement policy
        const BYTE                                      m_fValid : 1;           //  indicates that the cached block is valid
        const BYTE                                      m_fPinned : 1;          //  indicates that the cached block must not be written back
        const BYTE                                      m_fDirty : 1;           //  indicates that the cached block needs to be written back
        const BYTE                                      m_fEverDirty : 1;       //  indicates that the cached block has been written since cached
        const BYTE                                      m_fPurged : 1;          //  indicates that the cached block had a current valid dirty image invalidated
        const BYTE                                      m_rgbitReserved0 : 3;   //  reserved; always zero
        const UnalignedLittleEndian<UpdateNumber>       m_le_updno;             //  the update number of the cached block
};

#include <poppack.h>

#pragma warning (pop)

//  Cached Block Write Count

enum class CachedBlockWriteCount : DWORD  //  cbwc
{
    cbwcUnknown = 0xFFFFFFFF,
    cbwcNeverWritten = 0,
};

constexpr CachedBlockWriteCount cbwcUnknown = CachedBlockWriteCount::cbwcUnknown;
constexpr CachedBlockWriteCount cbwcNeverWritten = CachedBlockWriteCount::cbwcNeverWritten;

INLINE CachedBlockWriteCount operator+( _In_ const CachedBlockWriteCount rpos, _In_ const DWORD i ) { return (CachedBlockWriteCount)( (DWORD)rpos + i ); }

//  Cached Block Write Counts Manager

class ICachedBlockWriteCountsManager  //  cbwcm
{
    public:

        virtual ~ICachedBlockWriteCountsManager() {}

        //  Gets the write count at a given index corresponding to a cached block chunk.

        virtual ERR ErrGetWriteCount( _In_ const QWORD icbwc, _Out_ CachedBlockWriteCount* const pcbwc ) = 0;

        //  Sets the write count at a given index corresponding to a cached block chunk.

        virtual ERR ErrSetWriteCount(   _In_ const QWORD                    icbwc,
                                        _In_ const CachedBlockWriteCount    cbwc ) = 0;

        //  Writes the write counts to storage.

        virtual ERR ErrSave() = 0;
};


//  Chunk Number

//PERSISTED
enum class ChunkNumber : DWORD  //  chno
{
    chnoInvalid = 0xFFFFFFFF,
};

constexpr ChunkNumber chnoInvalid = ChunkNumber::chnoInvalid;

//  Slot Number

//PERSISTED
enum class SlotNumber : DWORD  //  slno
{
    slnoInvalid = 0xFFFFFFFF,
};

constexpr SlotNumber slnoInvalid = SlotNumber::slnoInvalid;

//  Cached Block Slot

#pragma warning (push)
#pragma warning (disable: 4315)  //  '': 'this' pointer for member '' may not be aligned X as expected by the constructor

#include <pshpack1.h>

//PERSISTED
class CCachedBlockSlot : public CCachedBlock
{
    public:

        CCachedBlockSlot()
            :   m_le_ibSlab( 0 ),
                m_le_chno( chnoInvalid ),
                m_le_slno( slnoInvalid )
        {
        }

        QWORD IbSlab() const { return m_le_ibSlab; }
        ChunkNumber Chno() const { return m_le_chno; }
        SlotNumber Slno() const { return m_le_slno; }

        static void Pin(    _In_    const CCachedBlockSlot& slot,
                            _Out_   CCachedBlockSlot* const pslot );

        static void SwapClusters(   _In_    const CCachedBlockSlot& slotA,
                                    _In_    const CCachedBlockSlot& slotB,
                                    _Out_   CCachedBlockSlot* const pslotA,
                                    _Out_   CCachedBlockSlot* const pslotB );

        static void Dump(   _In_ const CCachedBlockSlot&    slot,
                            _In_ CPRINTF* const             pcprintf,
                            _In_ IFileIdentification* const pfident );

    protected:

        friend class CCachedBlockSlab;
        template<class I> friend class TCachedBlockSlab;
        friend class COSBlockCacheFactoryImpl;
        friend const char* OSFormat( _In_ const CCachedBlockSlot& slot );

        CCachedBlockSlot(   _In_ const QWORD            ibSlab,
                            _In_ const ChunkNumber      chno,
                            _In_ const SlotNumber       slno,
                            _In_ const CCachedBlock&    cbl )
            :   m_le_ibSlab( ibSlab ),
                m_le_chno( chno ),
                m_le_slno( slno ),
                CCachedBlock( cbl )
        {
            C_ASSERT( 55 == sizeof( CCachedBlockSlot ) );
        }

    private:

        const UnalignedLittleEndian<QWORD>          m_le_ibSlab;
        const UnalignedLittleEndian<ChunkNumber>    m_le_chno;
        const UnalignedLittleEndian<SlotNumber>     m_le_slno;
};

#include <poppack.h>

#pragma warning (pop)

const char* OSFormat( _In_ const CCachedBlockSlot& slot );

//  Cached Block Slot State

class CCachedBlockSlotState : public CCachedBlockSlot
{
    public:

        CCachedBlockSlotState()
            :   m_fSlabUpdated( fFalse ),
                m_fChunkUpdated( fFalse ),
                m_fSlotUpdated( fFalse ),
                m_fClusterUpdated( fFalse ),
                m_fSuperceded( fFalse )
        {
        }

        BOOL FSlabUpdated() const { return m_fSlabUpdated; }
        BOOL FChunkUpdated() const { return m_fChunkUpdated; }
        BOOL FSlotUpdated() const { return m_fSlotUpdated; }
        BOOL FClusterUpdated() const { return m_fClusterUpdated; }
        BOOL FSuperceded() const { return m_fSuperceded; }

    protected:

        friend class CCachedBlockSlab;
        template<class I> friend class TCachedBlockSlab;
        friend class COSBlockCacheFactoryImpl;

        CCachedBlockSlotState(  _In_ const CCachedBlockSlot&    slot,
                                _In_ const BOOL                 fSlabUpdated,
                                _In_ const BOOL                 fChunkUpdated,
                                _In_ const BOOL                 fSlotUpdated,
                                _In_ const BOOL                 fClusterUpdated,
                                _In_ const BOOL                 fSuperceded )
            :   CCachedBlockSlot( slot ),
                m_fSlabUpdated( fSlabUpdated ),
                m_fChunkUpdated( fChunkUpdated ),
                m_fSlotUpdated( fSlotUpdated ),
                m_fClusterUpdated( fClusterUpdated ),
                m_fSuperceded( fSuperceded )
        {
        }

    private:

        const BOOL  m_fSlabUpdated;
        const BOOL  m_fChunkUpdated;
        const BOOL  m_fSlotUpdated;
        const BOOL  m_fClusterUpdated;
        const BOOL  m_fSuperceded;
};

//  Cached Block Slab

class ICachedBlockSlab  //  cbs
{
    public:

        virtual ~ICachedBlockSlab() {}

        //  Gets the physical identity of the slab.

        virtual ERR ErrGetPhysicalId( _Out_ QWORD* const pib ) = 0;

        //  Gets the next available slot for caching the existing data for a given cached block.
        //
        //  The request will fail if the block is already cached.
        // 
        //  The request can fail if there is no room to hold the new image of the cached block.
        //
        //  NOTE:  This operation is a function of the current state of the entire slab.  The output will not change
        //  until the slot is actually consumed with ErrUpdateSlot.

        virtual ERR ErrGetSlotForCache( _In_                const CCachedBlockId&   cbid,
                                        _In_                const size_t            cb,
                                        _In_reads_( cb )    const BYTE* const       rgb,
                                        _Out_               CCachedBlockSlot* const pslot ) = 0;

        //  Gets the next available slot for writing a given cached block.
        //
        //  The request can fail if there is no room to hold the new image of the cached block.
        // 
        //  If the buffer is not provided then the slot will be updated without updating the cluster.
        //
        //  NOTE:  This operation is a function of the current state of the entire slab.  The output will not change
        //  until the slot is actually consumed with ErrUpdateSlot.

        virtual ERR ErrGetSlotForWrite( _In_                    const CCachedBlockId&   cbid,
                                        _In_                    const size_t            cb,
                                        _In_reads_opt_( cb )    const BYTE* const       rgb,
                                        _Out_                   CCachedBlockSlot* const pslot ) = 0;

        //  Gets the slot containing the cluster of the given cached block.
        //
        //  The request will fail if the slab doesn't contain the cached block.
        //
        //  ErrUpdateSlot must be called to save the update to the block's access statistics.

        virtual ERR ErrGetSlotForRead(  _In_    const CCachedBlockId&   cbid,
                                        _Out_   CCachedBlockSlot* const pslot ) = 0;
                
        //  Callback used to indicate that a cluster has been written.

        typedef void (*PfnClusterWritten)(  _In_ const ERR          err,
                                            _In_ const DWORD_PTR    keyClusterWritten );

        //  Callback used to indicate that a cluster IO has been started.

        typedef void (*PfnClusterHandoff)( _In_ const DWORD_PTR keyClusterHandoff );

        //  Writes the cluster and the state for the given cached block.
        // 
        //  The expected call sequence is:
        //  -  ErrGetSlotForCache or ErrGetSlotForWrite
        //  -  ErrUpdateSlot
        //  -  ErrWriteCluster
        //
        //  The request will fail if the cluster data doesn't match the state.
        //
        //  The request will fail if it would cause an illegal state change in the slab.

        virtual ERR ErrWriteCluster(    _In_                const CCachedBlockSlot&                     slot,
                                        _In_                const size_t                                cb,
                                        _In_reads_( cb )    const BYTE* const                           rgb,
                                        _In_opt_            const ICachedBlockSlab::PfnClusterWritten   pfnClusterWritten   = NULL,
                                        _In_opt_            const DWORD_PTR                             keyClusterWritten   = NULL,
                                        _In_opt_            const ICachedBlockSlab::PfnClusterHandoff   pfnClusterHandoff   = NULL ) = 0;

        //  Updates the state for the given cached block.
        //
        //  The request will fail if it would cause an illegal state change in the slab.

        virtual ERR ErrUpdateSlot( _In_ const CCachedBlockSlot& slotNew ) = 0;
                       
        //  Callback used to indicate that a cluster has been read.

        typedef void (*PfnClusterRead)( _In_ const ERR          err,
                                        _In_ const DWORD_PTR    keyClusterRead );
 
        //  Reads the cluster of the given cached block.
        //
        //  The request will not verify the contents of the cluster.  This check is provided by ErrVerifyCluster.

        virtual ERR ErrReadCluster( _In_                const CCachedBlockSlot&                     slot,
                                    _In_                const size_t                                cb,
                                    _Out_writes_( cb )  BYTE* const                                 rgb,
                                    _In_opt_            const ICachedBlockSlab::PfnClusterRead      pfnClusterRead      = NULL,
                                    _In_opt_            const DWORD_PTR                             keyClusterRead      = NULL,
                                    _In_opt_            const ICachedBlockSlab::PfnClusterHandoff   pfnClusterHandoff   = NULL ) = 0;

        //  Verifies that the cluster data matches the cluster state.
        //
        //  The request will fail if the cluster data doesn't match the state.

        virtual ERR ErrVerifyCluster(   _In_                const CCachedBlockSlot& slot,
                                        _In_                const size_t            cb,
                                        _In_reads_( cb )    const BYTE* const       rgb ) = 0;

        //  Callback used to visit and optionally update a slot in the slab.
        // 
        //  The new state of the slot may be the same as the current state of the slot if the requested change is not
        //  possible.
        //
        //  The callback must return true to continue visiting more slots.

        typedef BOOL (*PfnConsiderUpdate)(  _In_ const CCachedBlockSlotState&   slotstCurrent,
                                            _In_ const CCachedBlockSlot&        slotNew,
                                            _In_ const DWORD_PTR                keyConsiderUpdate );

        //  Provides a mechanism for invalidating slots in the slab.
        // 
        //  All slots will be visited.  Any slot may be invalidated so use with caution.
        //
        //  A slot will only be invalidated by calling ErrUpdateSlot on the slot as it is visited.

        virtual ERR ErrInvalidateSlots( _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                        _In_ const DWORD_PTR                            keyConsiderUpdate ) = 0;

        //  Provides a mechanism for marking slots as written back in the slab.
        //
        //  All slots will be visited.  Any slot may have its dirty flag cleared.
        //
        //  A slot will only be marked as written back by calling ErrUpdateSlot on the slot as it is visited.

        virtual ERR ErrCleanSlots(  _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                    _In_ const DWORD_PTR                            keyConsiderUpdate ) = 0;

        //  Provides a mechanism to evicting slots in the slab.
        //
        //  The candidate slots will be visited in least useful to most useful order to facilitate batch evict.
        // 
        //  All slots will be visited.  Slots may contain invalid data, valid data, or valid data that needs to be
        //  written back.  Only slots with valid data that does not need to be written back can be invalidated.
        //
        //  A slot will only be evicted by calling ErrUpdateSlot on the slot as it is visited.

        virtual ERR ErrEvictSlots(  _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                    _In_ const DWORD_PTR                            keyConsiderUpdate ) = 0;

        //  Callback used to visit a slot in the slab.
        //
        //  The callback must return true to continue visiting more slots.

        typedef BOOL (*PfnVisitSlot)(   _In_ const ERR                      err,
                                        _In_ const CCachedBlockSlot&        slotAccepted,
                                        _In_ const CCachedBlockSlotState&   slotstCurrent,
                                        _In_ const DWORD_PTR                keyVisitSlot );

        //  Visits every slot in the slab.

        virtual ERR ErrVisitSlots(  _In_ const ICachedBlockSlab::PfnVisitSlot   pfnVisitSlot,
                                    _In_ const DWORD_PTR                        keyVisitSlot ) = 0;

        //  Indicates if the slab has been updated since it was acquired.

        virtual BOOL FUpdated() = 0;

        //  Commits any updates made to the slab since it was acquired.
        //
        //  If the slab is released before ErrAcceptUpdates() then any updates made since the slab was acquired will be
        //  lost.
        //
        //  The request will fail if the process to cache data was started with ErrGetSlotForCache() or
        //  ErrGetSlotForWrite() and the data was not written via ErrWriteCluster().

        virtual ERR ErrAcceptUpdates() = 0;

        //  Abandons any updates made to the slab since it was acquired.

        virtual void RevertUpdates() = 0;

        //  Indicates if the slab contains changes that need to be written back to storage via ErrSave.

        virtual BOOL FDirty() = 0;

        //  Callback used to indicate that a slab is saved.

        typedef void (*PfnSlabSaved)(   _In_ const ERR          err,
                                        _In_ const DWORD_PTR    keySlabSaved );

        //  Writes the slab state to storage.
        //
        //  Any updates that have not been accepted since the slab was acquired via ErrAcceptUpdates() will not be
        //  saved.

        virtual ERR ErrSave(    _In_opt_    const ICachedBlockSlab::PfnClusterRead  pfnSlabSaved    = NULL,
                                _In_opt_    const DWORD_PTR                         keySlabSaved    = NULL ) = 0;
};

//  Cached Block Slab Manager

class ICachedBlockSlabManager  //  cbsm
{
    public:

        virtual ~ICachedBlockSlabManager() {}

        //  Gets exclusive access to the slab that may contain a cached block.
        //
        //  The slab must be released to allow future access.

        virtual ERR ErrGetSlab( _In_    const CCachedBlockId&       cbid,
                                _Out_   ICachedBlockSlab** const    ppcbs ) = 0;

        //  Indicates if the slab can hold a given cached block.

        virtual ERR ErrIsSlabForCachedBlock(    _In_    ICachedBlockSlab* const pcbs,
                                                _In_    const CCachedBlockId&   cbid,
                                                _Out_   BOOL* const             pfIsSlabForCachedBlock ) = 0;

        //  Gets exclusive access to a particular slab by its physical id.
        //
        //  The slab must be released to allow future access.

        virtual ERR ErrGetSlab( _In_    const QWORD                 ib,
                                _In_    const BOOL                  fWait,
                                _In_    const BOOL                  fIgnoreVerificationErrors,
                                _Out_   ICachedBlockSlab** const    ppcbs ) = 0;

        //  Callback used to visit a slab in the cache.
        //
        //  If a particular slab fails verification then that error will be provided.
        //
        //  The callback must return true to continue visiting more slabs.

        typedef BOOL (*PfnVisitSlab)(   _In_ const QWORD                ib,
                                        _In_ const ERR                  err,
                                        _In_ ICachedBlockSlab* const    pcbs,
                                        _In_ const DWORD_PTR            keyVisitSlab );

        //  Visits every slab in the cache.

        virtual ERR ErrVisitSlabs(  _In_ const ICachedBlockSlabManager::PfnVisitSlab    pfnVisitSlab,
                                    _In_ const DWORD_PTR                                keyVisitSlab ) = 0;
};

//  Block Cache Factory

class IBlockCacheFactory  //  bcf
{
    public:

        virtual ~IBlockCacheFactory() {}

        virtual ERR ErrCreateFileSystemWrapper( _Inout_ IFileSystemAPI** const  ppfsapiInner,
                                                _Out_   IFileSystemAPI** const  ppfsapi ) = 0;

        virtual ERR ErrCreateFileSystemFilter(  _In_    IFileSystemConfiguration* const pfsconfig,
                                                _Inout_ IFileSystemAPI** const          ppfsapiInner,
                                                _In_    IFileIdentification* const      pfident,
                                                _In_    ICacheTelemetry* const          pctm,
                                                _In_    ICacheRepository* const         pcrep,
                                                _Out_   IFileSystemFilter** const       ppfsf ) = 0;

        virtual ERR ErrCreateFileWrapper(   _Inout_ IFileAPI** const    ppfapiInner,
                                            _Out_   IFileAPI** const    ppfapi ) = 0;

        virtual ERR ErrCreateFileFilter(    _Inout_                     IFileAPI** const                    ppfapiInner,
                                            _In_                        IFileSystemFilter* const            pfsf,
                                            _In_                        IFileSystemConfiguration* const     pfsconfig,
                                            _In_                        IFileIdentification* const          pfident,
                                            _In_                        ICacheTelemetry* const              pctm,
                                            _In_                        ICacheRepository* const             pcrep,
                                            _In_                        const VolumeId                      volumeid,
                                            _In_                        const FileId                        fileid,
                                            _Inout_                     ICachedFileConfiguration** const    ppcfconfig,
                                            _Inout_                     ICache** const                      ppc,
                                            _In_reads_opt_( cbHeader )  const BYTE* const                   pbHeader,
                                            _In_                        const int                           cbHeader,
                                            _Out_                       IFileFilter** const                 ppff ) = 0;
        virtual ERR ErrCreateFileFilterWrapper( _Inout_ IFileFilter** const         ppffInner,
                                                _In_    const IFileFilter::IOMode   iom,
                                                _Out_   IFileFilter** const         ppff ) = 0;

        virtual ERR ErrCreateFileIdentification( _Out_ IFileIdentification** const ppfident ) = 0;

        virtual ERR ErrCreateCache( _In_    IFileSystemFilter* const        pfsf,
                                    _In_    IFileIdentification* const      pfident,
                                    _In_    IFileSystemConfiguration* const pfsconfig,
                                    _Inout_ ICacheConfiguration** const     ppcconfig,
                                    _In_    ICacheTelemetry* const          pctm,
                                    _Inout_ IFileFilter** const             ppffCaching,
                                    _Out_   ICache** const                  ppc ) = 0;

        virtual ERR ErrMountCache(  _In_    IFileSystemFilter* const        pfsf,
                                    _In_    IFileIdentification* const      pfident,
                                    _In_    IFileSystemConfiguration* const pfsconfig,
                                    _Inout_ ICacheConfiguration** const     ppcconfig,
                                    _In_    ICacheTelemetry* const          pctm,
                                    _Inout_ IFileFilter** const             ppffCaching,
                                    _Out_   ICache** const                  ppc ) = 0;

        virtual ERR ErrCreateCacheWrapper(  _Inout_ ICache** const  ppcInner,
                                            _Out_   ICache** const  ppc ) = 0;

        virtual ERR ErrCreateCacheRepository(   _In_    IFileIdentification* const      pfident,
                                                _In_    ICacheTelemetry* const          pctm,
                                                _Out_   ICacheRepository** const        ppcrep ) = 0;

        virtual ERR ErrCreateCacheTelemetry( _Out_ ICacheTelemetry** const ppctm ) = 0;

        virtual ERR ErrDumpCachedFileHeader(    _In_z_  const WCHAR* const  wszFilePath,
                                                _In_    const ULONG         grbit,
                                                _In_    CPRINTF* const      pcprintf ) = 0;
        virtual ERR ErrDumpCacheFile(   _In_z_  const WCHAR* const  wszFilePath,
                                        _In_    const ULONG         grbit,
                                        _In_    CPRINTF* const      pcprintf ) = 0;

        virtual ERR ErrCreateJournalSegment(    _In_    IFileFilter* const      pff,
                                                _In_    const QWORD             ib,
                                                _In_    const SegmentPosition   spos,
                                                _In_    const DWORD             dwUniqueIdPrev,
                                                _In_    const SegmentPosition   sposReplay,
                                                _In_    const SegmentPosition   sposDurable,
                                                _Out_   IJournalSegment** const ppjs ) = 0;

        virtual ERR ErrLoadJournalSegment(  _In_    IFileFilter* const      pff,
                                            _In_    const QWORD             ib,
                                            _Out_   IJournalSegment** const ppjs ) = 0;

        virtual ERR ErrCreateJournalSegmentManager( _In_    IFileFilter* const              pff,
                                                    _In_    const QWORD                     ib,
                                                    _In_    const QWORD                     cb,
                                                    _Out_   IJournalSegmentManager** const  ppjsm ) = 0;

        virtual ERR ErrCreateJournal(   _Inout_ IJournalSegmentManager** const  ppjsm,
                                        _In_    const size_t                    cbCache,
                                        _Out_   IJournal** const                ppj ) = 0;

        virtual ERR ErrLoadCachedBlockWriteCountsManager(   _In_    IFileFilter* const                      pff,
                                                            _In_    const QWORD                             ib,
                                                            _In_    const QWORD                             cb, 
                                                            _In_    const QWORD                             ccbwcs,
                                                            _Out_   ICachedBlockWriteCountsManager** const  ppcbwcm ) = 0;

        virtual ERR ErrLoadCachedBlockSlab( _In_    IFileFilter* const                      pff,
                                            _In_    const QWORD                             ibSlab,
                                            _In_    const QWORD                             cbSlab,
                                            _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                                            _In_    const QWORD                             icbwcBase,
                                            _In_    const ClusterNumber                     clnoMin,
                                            _In_    const ClusterNumber                     clnoMax,
                                            _In_    const BOOL                              fIgnoreVerificationErrors,
                                            _Inout_ ICachedBlockSlab** const                ppcbs ) = 0;

        virtual ERR ErrCreateCachedBlockSlabWrapper(    _Inout_ ICachedBlockSlab** const    ppcbsInner,
                                                        _Out_   ICachedBlockSlab** const    ppcbs ) = 0;

        virtual ERR ErrLoadCachedBlockSlabManager(  _In_    IFileFilter* const                      pff,
                                                    _In_    const QWORD                             cbCachingFilePerSlab,
                                                    _In_    const QWORD                             cbCachedFilePerSlab,
                                                    _In_    const QWORD                             ibChunk,
                                                    _In_    const QWORD                             cbChunk,
                                                    _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                                                    _In_    const QWORD                             icbwcBase,
                                                    _In_    const ClusterNumber                     clnoMin,
                                                    _In_    const ClusterNumber                     clnoMax,
                                                    _Out_   ICachedBlockSlabManager** const         ppcbsm ) = 0;

        virtual ERR ErrCreateCachedBlock(   _In_    const CCachedBlockId    cbid,
                                            _In_    const ClusterNumber     clno,
                                            _In_    const BOOL              fValid,
                                            _In_    const BOOL              fPinned,
                                            _In_    const BOOL              fDirty,
                                            _In_    const BOOL              fEverDirty,
                                            _In_    const BOOL              fPurged,
                                            _In_    const UpdateNumber      updno,
                                            _Out_   CCachedBlock*           pcbl ) = 0;

        virtual ERR ErrCreateCachedBlockSlot(   _In_    const QWORD         ibSlab,
                                                _In_    const ChunkNumber   chno,
                                                _In_    const SlotNumber    slno,
                                                _In_    const CCachedBlock& cbl,
                                                _Out_   CCachedBlockSlot*   pslot ) = 0;

        virtual ERR ErrCreateCachedBlockSlotState(  _In_    const CCachedBlockSlot& slot,
                                                    _In_    const BOOL              fSlabUpdated,
                                                    _In_    const BOOL              fChunkUpdated,
                                                    _In_    const BOOL              fSlotUpdated,
                                                    _In_    const BOOL              fClusterUpdated,
                                                    _In_    const BOOL              fSuperceded,
                                                    _Out_   CCachedBlockSlotState*  pslotst ) = 0;

        typedef void (*PfnDetachFileStatus)(    _In_        const int       i,
                                                _In_        const int       c,
                                                _In_opt_    const DWORD_PTR keyDetachFileStatus );

        virtual ERR ErrDetachFile(  _In_z_      const WCHAR* const                              wszFilePath,
                                    _In_opt_    const IBlockCacheFactory::PfnDetachFileStatus   pfnDetachFileStatus,
                                    _In_opt_    const DWORD_PTR                                 keyDetachFileStatus ) = 0;
};

class COSBlockCacheFactory
{
    public:

        static ERR ErrCreate( _Out_ IBlockCacheFactory** const ppbcf );
};

#endif  //  _OSBLOCKCACHE_HXX_INCLUDED

