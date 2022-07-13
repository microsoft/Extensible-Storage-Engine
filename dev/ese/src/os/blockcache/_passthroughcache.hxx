// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


//  TPassThroughCache:  simple pass through cache implementation (all requests become cache miss / write through)

template< class I >
using TPassThroughCacheBase = TCacheBase<I, CPassThroughCachedFileTableEntry, CCacheThreadLocalStorageBase>;

template< class I >
class TPassThroughCache
    :   public TPassThroughCacheBase<I>
{
    public:

        ~TPassThroughCache()
        {
        }

    public:  //  ICache

        ERR ErrCreate() override;

        ERR ErrMount() override;

        ERR ErrPrepareToDismount() override;

        ERR ErrDump( _In_ CPRINTF* const pcprintf ) override;

        ERR ErrFlush(   _In_ const VolumeId     volumeid,
                        _In_ const FileId       fileid,
                        _In_ const FileSerial   fileserial ) override;

        ERR ErrDestage( _In_        const VolumeId                  volumeid,
                        _In_        const FileId                    fileid,
                        _In_        const FileSerial                fileserial,
                        _In_opt_    const ICache::PfnDestageStatus  pfnDestageStatus,
                        _In_opt_    const DWORD_PTR                 keyDestageStatus ) override;

        ERR ErrInvalidate(  _In_ const VolumeId     volumeid,
                            _In_ const FileId       fileid,
                            _In_ const FileSerial   fileserial,
                            _In_ const QWORD        ibOffset,
                            _In_ const QWORD        cbData ) override;

        ERR ErrRead(    _In_                    const TraceContext&         tc,
                        _In_                    const VolumeId              volumeid,
                        _In_                    const FileId                fileid,
                        _In_                    const FileSerial            fileserial,
                        _In_                    const QWORD                 ibOffset,
                        _In_                    const DWORD                 cbData,
                        _Out_writes_( cbData )  BYTE* const                 pbData,
                        _In_                    const OSFILEQOS             grbitQOS,
                        _In_                    const ICache::CachingPolicy cp,
                        _In_opt_                const ICache::PfnComplete   pfnComplete,
                        _In_opt_                const DWORD_PTR             keyComplete ) override;

        ERR ErrWrite(   _In_                    const TraceContext&         tc,
                        _In_                    const VolumeId              volumeid,
                        _In_                    const FileId                fileid,
                        _In_                    const FileSerial            fileserial,
                        _In_                    const QWORD                 ibOffset,
                        _In_                    const DWORD                 cbData,
                        _In_reads_( cbData )    const BYTE* const           pbData,
                        _In_                    const OSFILEQOS             grbitQOS,
                        _In_                    const ICache::CachingPolicy cp,
                        _In_opt_                const ICache::PfnComplete   pfnComplete,
                        _In_opt_                const DWORD_PTR             keyComplete ) override;

        ERR ErrIssue(   _In_ const VolumeId     volumeid,
                        _In_ const FileId       fileid,
                        _In_ const FileSerial   fileserial ) override;

    protected:
        
        TPassThroughCache(  _In_    IFileSystemFilter* const            pfsf,
                            _In_    IFileIdentification* const          pfident,
                            _In_    IFileSystemConfiguration* const     pfsconfig, 
                            _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                            _Inout_ ICacheConfiguration** const         ppcconfig,
                            _In_    ICacheTelemetry* const              pctm,
                            _Inout_ IFileFilter** const                 ppffCaching,
                            _Inout_ CCacheHeader** const                ppch )
            :   TPassThroughCacheBase<I>(   pfsf,
                                            pfident, 
                                            pfsconfig, 
                                            ppbcconfig, 
                                            ppcconfig, 
                                            pctm, 
                                            ppffCaching, 
                                            ppch )
        {
        }
};

template< class I >
ERR TPassThroughCache<I>::ErrCreate()
{
    //  trivial implementation:  nothing to do

    return JET_errSuccess;
}

template< class I >
ERR TPassThroughCache<I>::ErrMount()
{
    //  trivial implementation:  nothing to do

    return JET_errSuccess;
}

template< class I >
ERR TPassThroughCache<I>::ErrPrepareToDismount()
{
    //  trivial implementation:  nothing to do

    return JET_errSuccess;
}

template< class I >
ERR TPassThroughCache<I>::ErrDump( _In_ CPRINTF* const pcprintf )
{
    //  trivial implementation:  nothing to do

    return JET_errSuccess;
}

template< class I >
ERR TPassThroughCache<I>::ErrFlush( _In_ const VolumeId     volumeid,
                                    _In_ const FileId       fileid,
                                    _In_ const FileSerial   fileserial )
{
    ERR                                 err     = JET_errSuccess;
    CPassThroughCachedFileTableEntry*   pcfte   = NULL;

    //  get the cached file

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    //  trivial implementation:  nothing to do

HandleError:
    ReleaseCachedFile( &pcfte );
    return err;
}

template< class I >
ERR TPassThroughCache<I>::ErrDestage(   _In_        const VolumeId                  volumeid,
                                        _In_        const FileId                    fileid,
                                        _In_        const FileSerial                fileserial,
                                        _In_opt_    const ICache::PfnDestageStatus  pfnDestageStatus,
                                        _In_opt_    const DWORD_PTR                 keyDestageStatus )
{
    ERR                                 err     = JET_errSuccess;
    CPassThroughCachedFileTableEntry*   pcfte   = NULL;

    //  get the cached file

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    //  trivial implementation:  nothing to do

HandleError:
    ReleaseCachedFile( &pcfte );
    return err;
}

template< class I >
ERR TPassThroughCache<I>::ErrInvalidate(    _In_ const VolumeId     volumeid,
                                            _In_ const FileId       fileid,
                                            _In_ const FileSerial   fileserial,
                                            _In_ const QWORD        ibOffset,
                                            _In_ const QWORD        cbData )
{
    ERR                                 err                 = JET_errSuccess;
    const QWORD                         ibStart             = ibOffset;
    const QWORD                         ibEnd               = ibOffset >= qwMax - cbData ? qwMax : ibOffset - 1 + cbData;
    COffsets                            offsets             = COffsets( ibStart, ibEnd );
    CPassThroughCachedFileTableEntry*   pcfte               = NULL;
    QWORD                               cbCachedFile        = 0;
    ULONG                               cbPinned            = 0;
    COffsets                            offsetsPinned;
    DWORD                               cbHeaderInvalidated = 0;
    BYTE*                               rgbZero             = NULL;
    TraceContextScope                   tcScope( iorpBlockCache );

    //  trivial implementation:  invalidate the displaced data

    //  get the cached file

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    //  ignore empty requests

    if ( !cbData )
    {
        Error( JET_errSuccess );
    }

    //  if this request is not aligned to cached file blocks then fail

    if ( offsets.IbStart() % pcfte->CbBlockSize() != 0 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( offsets.IbEnd() != qwMax && offsets.Cb() % pcfte->CbBlockSize() != 0 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  determine the current cached file size.  we only need to invalidate up to that file size

    Call( pcfte->Pff()->ErrSize( &cbCachedFile, IFileAPI::filesizeLogical ) );

    if ( !cbCachedFile )
    {
        Error( JET_errSuccess );
    }

    offsets = COffsets( ibOffset, min( ibEnd, roundup( cbCachedFile, cbCachedBlock ) - 1 ) );

    //  compute the offsets protected from overwrite

    cbPinned = roundup( pcfte->Pcfconfig()->UlPinnedHeaderSizeInBytes(), pcfte->Pcfconfig()->CbBlockSize() );
    offsetsPinned = COffsets( 0, cbPinned - 1 );

    //  if we are invalidating the data displaced by the header then explicitly clear the displaced data

    if ( offsets.FOverlaps( offsetsPinned ) && offsets.Cb() > 0 )
    {
        cbHeaderInvalidated = (DWORD)min( offsets.Cb(), offsetsPinned.IbEnd() - offsets.IbStart() + 1 );

        Alloc( rgbZero = (BYTE*)PvOSMemoryPageAlloc( roundup( cbHeaderInvalidated, OSMemoryPageCommitGranularity() ), NULL ) );
        Call( pcfte->PffDisplacedData()->ErrIOWrite(    *tcScope,
                                                        offsets.IbStart(), 
                                                        cbHeaderInvalidated, 
                                                        rgbZero, 
                                                        qosIONormal ) );

        pcfte->PffDisplacedData()->SetNoFlushNeeded();
    }

HandleError:
    OSMemoryPageFree( rgbZero );
    ReleaseCachedFile( &pcfte );
    return err;
}

template< class I >
ERR TPassThroughCache<I>::ErrRead(  _In_                    const TraceContext&         tc,
                                    _In_                    const VolumeId              volumeid,
                                    _In_                    const FileId                fileid,
                                    _In_                    const FileSerial            fileserial,
                                    _In_                    const QWORD                 ibOffset,
                                    _In_                    const DWORD                 cbData,
                                    _Out_writes_( cbData )  BYTE* const                 pbData,
                                    _In_                    const OSFILEQOS             grbitQOS,
                                    _In_                    const ICache::CachingPolicy cp,
                                    _In_opt_                const ICache::PfnComplete   pfnComplete,
                                    _In_opt_                const DWORD_PTR             keyComplete )
{
    ERR                                 err             = JET_errSuccess;
    CCacheThreadLocalStorageBase*       pctls           = NULL;
    CPassThroughCachedFileTableEntry*   pcfte           = NULL;
    ULONG                               cbPinned        = 0;
    COffsets                            offsetsPinned;
    const COffsets                      offsets         = COffsets( ibOffset, ibOffset - 1 + cbData );
    DWORD                               cbHeaderRead    = 0;
    const BOOL                          fAsync          = pfnComplete != NULL;
    CRequest*                           prequest        = NULL;

    //  get our thread local storage

    if ( fAsync )
    {
        Call( ErrGetThreadLocalStorage( &pctls ) );
    }

    //  get the cached file

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    //  compute the offsets protected from overwrite

    cbPinned = roundup( pcfte->Pcfconfig()->UlPinnedHeaderSizeInBytes(), pcfte->Pcfconfig()->CbBlockSize() );
    offsetsPinned = COffsets( 0, cbPinned - 1 );

    //  if this request is not aligned to cached file blocks then fail

    Call( ErrValidateOffsets( pcfte, ibOffset, cbData ) );

    //  the pass through cache doesn't support arbitrary pinning

    if ( cp == cpPinned && !offsetsPinned.FContains( offsets ) )
    {
        Error( ErrERRCheck( JET_errInvalidOperation ) );
    }

    //  create our request context

    Alloc( prequest = new CRequest( fTrue,
                                    this,
                                    tc,
                                    &pcfte,
                                    ibOffset,
                                    cbData, 
                                    pbData,
                                    grbitQOS,
                                    cp,
                                    pfnComplete, 
                                    keyComplete ) );

    //  trivial implementation:  simply pass the IO on to the cached file as a cache miss, except the displaced header

    //  telemetry

    ReportMiss( prequest->Pcfte(), ibOffset, cbData, fTrue, cp != cpDontCache );

    //  if this read is accessing the data displaced by the header then redirect that portion of the IO to the
    //  displaced data

    if ( offsets.FOverlaps( offsetsPinned ) )
    {
        cbHeaderRead = (DWORD)COffsets( offsets.IbStart(), min( offsets.IbEnd(), offsetsPinned.IbEnd() ) ).Cb();
        cbHeaderRead = min( cbHeaderRead, cbData );
    }

    if ( cbHeaderRead > 0 )
    {
        Call( prequest->ErrRead(    prequest->Pcfte()->PffDisplacedData(),
                                    offsets.IbStart(),
                                    cbHeaderRead,
                                    pbData, 
                                    iomEngine ) );

        if ( fAsync )
        {
            CallS( prequest->Pcfte()->PffDisplacedData()->ErrIOIssue() );
        }
    }

    //  perform the original read less whatever displaced header was read

    if ( cbHeaderRead < cbData )
    {
        Call( prequest->ErrRead(    prequest->Pcfte()->Pff(),
                                    ibOffset + cbHeaderRead,
                                    cbData - cbHeaderRead,
                                    pbData + cbHeaderRead,
                                    iomCacheMiss ) );

        if ( fAsync )
        {
            Call( prequest->Pcfte()->Pff()->ErrIssue( iomCacheMiss ) );
        }
    }

HandleError:
    err = CRequest::ErrRelease( &prequest, err );
    ReleaseCachedFile( &pcfte );
    CCacheThreadLocalStorageBase::Release( &pctls );
    return err;
}

template< class I >
ERR TPassThroughCache<I>::ErrWrite( _In_                    const TraceContext&         tc,
                                    _In_                    const VolumeId              volumeid,
                                    _In_                    const FileId                fileid,
                                    _In_                    const FileSerial            fileserial,
                                    _In_                    const QWORD                 ibOffset,
                                    _In_                    const DWORD                 cbData,
                                    _In_reads_( cbData )    const BYTE* const           pbData,
                                    _In_                    const OSFILEQOS             grbitQOS,
                                    _In_                    const ICache::CachingPolicy cp,
                                    _In_opt_                const ICache::PfnComplete   pfnComplete,
                                    _In_opt_                const DWORD_PTR             keyComplete )
{
    ERR                                 err             = JET_errSuccess;
    CCacheThreadLocalStorageBase*       pctls           = NULL;
    CPassThroughCachedFileTableEntry*   pcfte           = NULL;
    ULONG                               cbPinned        = 0;
    COffsets                            offsetsPinned;
    const COffsets                      offsets         = COffsets( ibOffset, ibOffset - 1 + cbData );
    DWORD                               cbHeaderWritten = 0;
    const BOOL                          fAsync          = pfnComplete != NULL;
    CRequest*                           prequest        = NULL;

    //  get our thread local storage

    if ( fAsync )
    {
        Call( ErrGetThreadLocalStorage( &pctls ) );
    }

    //  get the cached file

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    //  compute the offsets protected from overwrite

    cbPinned = roundup( pcfte->Pcfconfig()->UlPinnedHeaderSizeInBytes(), pcfte->Pcfconfig()->CbBlockSize() );
    offsetsPinned = COffsets( 0, cbPinned - 1 );

    //  if this request is not aligned to cached file blocks then fail

    Call( ErrValidateOffsets( pcfte, ibOffset, cbData ) );

    //  the pass through cache doesn't support arbitrary pinning

    if ( cp == cpPinned && !offsetsPinned.FContains( offsets ) )
    {
        Error( ErrERRCheck( JET_errInvalidOperation ) );
    }

    //  create our request context

    Alloc( prequest = new CRequest( fFalse,
                                    this,
                                    tc,
                                    &pcfte,
                                    ibOffset,
                                    cbData,
                                    pbData,
                                    grbitQOS,
                                    cp,
                                    pfnComplete, 
                                    keyComplete ) );

    //  trivial implementation:  simply pass the IO on to the cached file as a write through, except the displaced header

    //  telemetry
  
    ReportMiss( prequest->Pcfte(), ibOffset, cbData, fFalse, cp != cpDontCache );
    ReportUpdate( prequest->Pcfte(), ibOffset, cbData );
    ReportWrite( prequest->Pcfte(), ibOffset, cbData, fTrue );

    //  if the write is accessing the data displaced by the header then redirect that portion of the IO to the
    //  displaced data

    if ( offsets.FOverlaps( offsetsPinned ) )
    {
        cbHeaderWritten = (DWORD)COffsets( offsets.IbStart(), min( offsets.IbEnd(), offsetsPinned.IbEnd() ) ).Cb();
        cbHeaderWritten = min( cbHeaderWritten, cbData );
    }

    if ( cbHeaderWritten > 0 )
    {
        Call( prequest->ErrWrite(   prequest->Pcfte()->PffDisplacedData(),
                                    offsets.IbStart(),
                                    cbHeaderWritten,
                                    pbData,
                                    iomEngine ) );

        if ( fAsync )
        {
            CallS( prequest->Pcfte()->PffDisplacedData()->ErrIOIssue() );
        }

        prequest->Pcfte()->PffDisplacedData()->SetNoFlushNeeded();
    }

    //  perform the original write less whatever displaced header was written

    if ( cbHeaderWritten < cbData )
    {
        Call( prequest->ErrWrite(   prequest->Pcfte()->Pff(),
                                    ibOffset + cbHeaderWritten,
                                    cbData - cbHeaderWritten,
                                    pbData + cbHeaderWritten,
                                    iomCacheWriteThrough ) );

        if ( fAsync )
        {
            Call( prequest->Pcfte()->Pff()->ErrIssue( iomCacheWriteThrough ) );
        }
    }

HandleError:
    err = CRequest::ErrRelease( &prequest, err );
    ReleaseCachedFile( &pcfte );
    CCacheThreadLocalStorageBase::Release( &pctls );
    return err;
}

template<class I>
inline ERR TPassThroughCache<I>::ErrIssue(  _In_ const VolumeId     volumeid,
                                            _In_ const FileId       fileid,
                                            _In_ const FileSerial   fileserial)
{
    ERR                                 err     = JET_errSuccess;
    CCacheThreadLocalStorageBase*       pctls   = NULL;
    CPassThroughCachedFileTableEntry*   pcfte   = NULL;

    //  get our thread local storage

    Call( ErrGetThreadLocalStorage( &pctls ) );

    //  get the cached file

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    //  trivial implementation:  nothing to do

HandleError:
    ReleaseCachedFile( &pcfte );
    CCacheThreadLocalStorageBase::Release( &pctls );
    return err;
}

//  CPassThroughCache:  concrete TPassThroughCache<ICache>

//  0dbbf833-444d-4025-ab3e-1d2cfd4336f0
const BYTE c_rgbPassThroughCacheType[ sizeof( GUID ) ] = { 0x33, 0xF8, 0xBB, 0x0D, 0x4D, 0x44, 0x25, 0x40, 0xAB, 0x3E, 0x1D, 0x2C, 0xFD, 0x43, 0x36, 0xF0 };

class CPassThroughCache : public TPassThroughCache<ICache>
{
    public:  //  specialized API

        static const BYTE* RgbCacheType()
        {
            return c_rgbPassThroughCacheType;
        }

        static ICache* PcCreate(    _In_    IFileSystemFilter* const            pfsf,
                                    _In_    IFileIdentification* const          pfident,
                                    _In_    IFileSystemConfiguration* const     pfsconfig, 
                                    _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                                    _Inout_ ICacheConfiguration** const         ppcconfig,
                                    _In_    ICacheTelemetry* const              pctm,
                                    _Inout_ IFileFilter** const                 ppffCaching,
                                    _Inout_ CCacheHeader** const                ppch )
        {
            return new CPassThroughCache( pfsf, pfident, pfsconfig, ppbcconfig, ppcconfig, pctm, ppffCaching, ppch );
        }

        ~CPassThroughCache() {}

    private:

        CPassThroughCache(  _In_    IFileSystemFilter* const            pfsf,
                            _In_    IFileIdentification* const          pfident,
                            _In_    IFileSystemConfiguration* const     pfsconfig,
                            _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                            _Inout_ ICacheConfiguration** const         ppcconfig,
                            _In_    ICacheTelemetry* const              pctm,
                            _Inout_ IFileFilter** const                 ppffCaching,
                            _Inout_ CCacheHeader** const                ppch )
            :   TPassThroughCache<ICache>( pfsf, pfident, pfsconfig, ppbcconfig, ppcconfig, pctm, ppffCaching, ppch )
        {
        }
};
