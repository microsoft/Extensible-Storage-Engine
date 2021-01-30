// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSBLOCKCACHE_HXX_INCLUDED
#define _OSBLOCKCACHE_HXX_INCLUDED

#include "osfsapi.hxx"


enum class VolumeId : DWORD
{
    volumeidInvalid = 0,
};

constexpr VolumeId volumeidInvalid = VolumeId::volumeidInvalid;


enum class FileId : QWORD
{
    fileidInvalid = 0,
};

constexpr FileId fileidInvalid = FileId::fileidInvalid;


enum class FileSerial : DWORD
{
    fileserialInvalid = 0,
};

constexpr FileSerial fileserialInvalid = FileSerial::fileserialInvalid;


class IFileIdentification
{
    public:

        virtual ~IFileIdentification() {}


        virtual ERR ErrGetFileId(   _In_z_  const WCHAR* const  wszPath,
                                    _Out_   VolumeId* const     pvolumeid,
                                    _Out_   FileId* const       pfileid ) = 0;


        virtual ERR ErrGetFileKeyPath(  _In_z_                                  const WCHAR* const  wszPath,
                                        _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const        wszKeyPath ) = 0;


        virtual ERR ErrGetFilePathById( _In_                                    const VolumeId  volumeid,
                                        _In_                                    const FileId    fileid,
                                        _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const    wszAnyAbsPath,
                                        _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const    wszKeyPath ) = 0;
};


class IFileSystemFilter
    :   public IFileSystemAPI
{
    public:


        virtual ERR ErrFileOpenById(    _In_    const VolumeId                  volumeid,
                                        _In_    const FileId                    fileid,
                                        _In_    const FileSerial                fileserial,
                                        _In_    const IFileAPI::FileModeFlags   fmf,
                                        _Out_   IFileAPI** const                ppfapi ) = 0;


        virtual ERR ErrFileRename(  _In_    IFileAPI* const    pfapi,
                                    _In_z_  const WCHAR* const wszPathDest,
                                    _In_    const BOOL         fOverwriteExisting ) = 0;
};


class IFileFilter
    :   public IFileAPI
{
    public:


        enum class IOMode
        {
            iomRaw = 0,
            iomEngine = 1,
            iomCacheMiss = 2,
            iomCacheWriteThrough = 3,
            iomCacheWriteBack = 4,
        };

    public:


        virtual ERR ErrGetPhysicalId(   _Out_ VolumeId* const   pvolumeid,
                                        _Out_ FileId* const     pfileid,
                                        _Out_ FileSerial* const pfileserial ) = 0;


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


        virtual ERR ErrWrite(   _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _In_reads_( cbData )    const BYTE* const               pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_                    const IFileFilter::IOMode       iom,
                                _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_                    const DWORD_PTR                 keyIOComplete,
                                _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff ) = 0;


        virtual ERR ErrIssue( _In_ const IOMode iom ) = 0;
};

constexpr IFileFilter::IOMode iomRaw = IFileFilter::IOMode::iomRaw;
constexpr IFileFilter::IOMode iomEngine = IFileFilter::IOMode::iomEngine;
constexpr IFileFilter::IOMode iomCacheMiss = IFileFilter::IOMode::iomCacheMiss;
constexpr IFileFilter::IOMode iomCacheWriteThrough = IFileFilter::IOMode::iomCacheWriteThrough;
constexpr IFileFilter::IOMode iomCacheWriteBack = IFileFilter::IOMode::iomCacheWriteBack;


class ICache
{
    public:

        enum { cbGuid = sizeof(GUID) };


        enum class CachingPolicy
        {
            cpDontCache = 0,
            cpBestEffort = 1,
        };

    public:

        virtual ~ICache() {}


        virtual ERR ErrCreate() = 0;


        virtual ERR ErrMount() = 0;


        virtual ERR ErrDump( _In_ CPRINTF* const pcprintf ) = 0;


        virtual ERR ErrGetCacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType ) = 0;


        virtual ERR ErrGetPhysicalId(   _Out_                   VolumeId* const pvolumeid,
                                        _Out_                   FileId* const   pfileid,
                                        _Out_writes_( cbGuid )  BYTE* const     rgbUniqueId ) = 0;


        virtual ERR ErrClose(   _In_ const VolumeId     volumeid,
                                _In_ const FileId       fileid,
                                _In_ const FileSerial   fileserial ) = 0;


        virtual ERR ErrFlush(   _In_ const VolumeId     volumeid,
                                _In_ const FileId       fileid,
                                _In_ const FileSerial   fileserial ) = 0;


        virtual ERR ErrInvalidate(  _In_ const VolumeId     volumeid,
                                    _In_ const FileId       fileid,
                                    _In_ const FileSerial   fileserial,
                                    _In_ const QWORD        ibOffset,
                                    _In_ const QWORD        cbData ) = 0;


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
};

constexpr ICache::CachingPolicy cpDontCache = ICache::CachingPolicy::cpDontCache;
constexpr ICache::CachingPolicy cpBestEffort = ICache::CachingPolicy::cpBestEffort;


class ICacheTelemetry
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


        virtual void Miss(  _In_ const FileNumber   filenumber, 
                            _In_ const BlockNumber  blocknumber,
                            _In_ const BOOL         fRead,
                            _In_ const BOOL         fCacheIfPossible ) = 0;


        virtual void Hit(   _In_ const FileNumber   filenumber,
                            _In_ const BlockNumber  blocknumber,
                            _In_ const BOOL         fRead,
                            _In_ const BOOL         fCacheIfPossible ) = 0;


        virtual void Update(    _In_ const FileNumber   filenumber, 
                                _In_ const BlockNumber  blocknumber ) = 0;


        virtual void Write( _In_ const FileNumber   filenumber, 
                            _In_ const BlockNumber  blocknumber,
                            _In_ const BOOL         fReplacementPolicy ) = 0;


        virtual void Evict( _In_ const FileNumber   filenumber, 
                            _In_ const BlockNumber  blocknumber,
                            _In_ const BOOL         fReplacementPolicy ) = 0;
};

constexpr ICacheTelemetry::FileNumber filenumberInvalid = ICacheTelemetry::FileNumber::filenumberInvalid;
constexpr ICacheTelemetry::BlockNumber blocknumberInvalid = ICacheTelemetry::BlockNumber::blocknumberInvalid;


class ICacheRepository
{
    public:

        enum { cbGuid = sizeof(GUID) };

    public:

        virtual ~ICacheRepository() {}


        virtual ERR ErrOpen(    _In_    IFileSystemFilter* const        pfsf,
                                _In_    IFileSystemConfiguration* const pfsconfig,
                                _Inout_ ICacheConfiguration** const     ppcconfig,
                                _Out_   ICache** const                  ppc ) = 0;


        virtual ERR ErrOpenById(    _In_                IFileSystemFilter* const        pfsf,
                                    _In_                IFileSystemConfiguration* const pfsconfig,
                                    _In_                IBlockCacheConfiguration* const pbcconfig,
                                    _In_                const VolumeId                  volumeid,
                                    _In_                const FileId                    fileid,
                                    _In_reads_(cbGuid)  const BYTE* const               rgbUniqueId,
                                    _Out_               ICache** const                  ppc ) = 0;
};


enum class SegmentPosition : QWORD
{
    sposInvalid = llMax,
};

constexpr SegmentPosition sposInvalid = SegmentPosition::sposInvalid;

INLINE SegmentPosition operator+( SegmentPosition spos, LONG64 i ) { return (SegmentPosition)( (LONG64)spos + i ); }
INLINE SegmentPosition operator-( SegmentPosition spos, LONG64 i ) { return (SegmentPosition)( (LONG64)spos - i ); }
INLINE LONG64 operator-( SegmentPosition sposA, SegmentPosition sposB ) { return (LONG64)sposA - (LONG64)sposB; }


enum class RegionPosition : QWORD
{
    rposInvalid = llMax,
};

constexpr RegionPosition rposInvalid = RegionPosition::rposInvalid;

INLINE RegionPosition operator+( RegionPosition rpos, LONG64 i ) { return (RegionPosition)( (LONG64)rpos + i ); }
INLINE LONG64 operator-( RegionPosition rposA, RegionPosition rposB ) { return (LONG64)rposA - (LONG64)rposB; }


class CJournalBuffer
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


class IJournalSegment
{
    public:

        virtual ~IJournalSegment() {}


        virtual ERR ErrGetPhysicalId( _Out_ QWORD* const pib ) = 0;


        virtual ERR ErrGetProperties(   _Out_opt_ SegmentPosition* const    pspos,
                                        _Out_opt_ DWORD* const              pdwUniqueId,
                                        _Out_opt_ DWORD* const              pdwUniqueIdPrev,
                                        _Out_opt_ SegmentPosition* const    psposReplay,
                                        _Out_opt_ SegmentPosition* const    psposDurable,
                                        _Out_opt_ BOOL* const               pfSealed ) = 0;
        

        typedef BOOL (*PfnVisitRegion)( _In_ const RegionPosition   rpos,
                                        _In_ const CJournalBuffer   jb,
                                        _In_ const DWORD_PTR        keyVisitRegion );


        virtual ERR ErrVisitRegions(    _In_ const IJournalSegment::PfnVisitRegion  pfnVisitRegion,
                                        _In_ const DWORD_PTR                        keyVisitRegion ) = 0;


        virtual ERR ErrAppendRegion(    _In_                const size_t            cjb,
                                        _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                                        _In_                const DWORD             cbMin,
                                        _Out_               RegionPosition* const   rpos,
                                        _Out_               DWORD* const            pcbActual ) = 0;
        

        typedef void (*PfnSealed)(  _In_ const ERR              err,
                                    _In_ const SegmentPosition  spos,
                                    _In_ const DWORD_PTR        keySealed );


        virtual ERR ErrSeal( _In_opt_ IJournalSegment::PfnSealed pfnSealed, _In_ const DWORD_PTR keySealed ) = 0;
};


class IJournalSegmentManager
{
    public:

        virtual ~IJournalSegmentManager() {}
       

        virtual ERR ErrGetProperties(   _Out_opt_ SegmentPosition* const    psposReplay,
                                        _Out_opt_ SegmentPosition* const    psposDurable,
                                        _Out_opt_ SegmentPosition* const    psposLast ) = 0;
 

        typedef BOOL (*PfnVisitSegment)(    _In_ const SegmentPosition  spos,
                                            _In_ const ERR              err,
                                            _In_ IJournalSegment* const pjs,
                                            _In_ const DWORD_PTR        keyVisitSegment );


        virtual ERR ErrVisitSegments(   _In_ const IJournalSegmentManager::PfnVisitSegment  pfnVisitSegment,
                                        _In_ const DWORD_PTR                                keyVisitSegment ) = 0;


        virtual ERR ErrAppendSegment(   _In_    const SegmentPosition   spos,
                                        _In_    const SegmentPosition   sposReplay,
                                        _In_    const SegmentPosition   sposDurable,
                                        _Out_   IJournalSegment** const ppjs ) = 0;


        virtual ERR ErrFlush() = 0;
};


enum class JournalPosition : QWORD
{
    jposInvalid = llMax,
};

constexpr JournalPosition jposInvalid = JournalPosition::jposInvalid;


class IJournal
{
    public:

        virtual ~IJournal() {}
       

        virtual ERR ErrGetProperties(   _Out_opt_ JournalPosition* const    pjposReplay,
                                        _Out_opt_ JournalPosition* const    pjposDurableForWriteBack,
                                        _Out_opt_ JournalPosition* const    pjposDurable ) = 0;


        typedef BOOL (*PfnVisitEntry)(  _In_ const JournalPosition  jpos,
                                        _In_ const CJournalBuffer   jb,
                                        _In_ const DWORD_PTR        keyVisitEntry );


        virtual ERR ErrVisitEntries(    _In_ const IJournal::PfnVisitEntry  pfnVisitEntry,
                                        _In_ const DWORD_PTR                keyVisitEntry ) = 0;


        virtual ERR ErrRepair( _In_ const JournalPosition jposInvalidate ) = 0;


        virtual ERR ErrAppendEntry( _In_                const size_t            cjb,
                                    _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                                    _Out_               JournalPosition* const  pjpos ) = 0;


        virtual ERR ErrFlush() = 0;


        virtual ERR ErrTruncate( _In_ const JournalPosition jposReplay ) = 0;
};


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

#endif

