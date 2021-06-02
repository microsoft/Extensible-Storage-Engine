// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


//  TPassThroughCache:  simple pass through cache implementation (all requests become cache miss / write through)

template< class I >
class TPassThroughCache
    :   public TCacheBase<I, CPassThroughCachedFileTableEntry>
{
    public:

        ~TPassThroughCache()
        {
        }

    public:  //  ICache

        ERR ErrCreate() override;

        ERR ErrMount() override;

        ERR ErrDump( _In_ CPRINTF* const pcprintf ) override;

        ERR ErrFlush(   _In_ const VolumeId     volumeid,
                        _In_ const FileId       fileid,
                        _In_ const FileSerial   fileserial ) override;

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
                        _In_                    const ICache::PfnComplete   pfnComplete,
                        _In_                    const DWORD_PTR             keyComplete ) override;

        ERR ErrWrite(   _In_                    const TraceContext&         tc,
                        _In_                    const VolumeId              volumeid,
                        _In_                    const FileId                fileid,
                        _In_                    const FileSerial            fileserial,
                        _In_                    const QWORD                 ibOffset,
                        _In_                    const DWORD                 cbData,
                        _In_reads_( cbData )    const BYTE* const           pbData,
                        _In_                    const OSFILEQOS             grbitQOS,
                        _In_                    const ICache::CachingPolicy cp,
                        _In_                    const ICache::PfnComplete   pfnComplete,
                        _In_                    const DWORD_PTR             keyComplete ) override;

    protected:
        
        TPassThroughCache(  _In_    IFileSystemFilter* const            pfsf,
                            _In_    IFileIdentification* const          pfident,
                            _In_    IFileSystemConfiguration* const     pfsconfig, 
                            _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                            _Inout_ ICacheConfiguration** const         ppcconfig,
                            _In_    ICacheTelemetry* const              pctm,
                            _Inout_ IFileFilter** const                 ppffCaching,
                            _Inout_ CCacheHeader** const                ppch )
            : TCacheBase<I, CPassThroughCachedFileTableEntry>(  pfsf,
                                                                pfident, 
                                                                pfsconfig, 
                                                                ppbcconfig, 
                                                                ppcconfig, 
                                                                pctm, 
                                                                ppffCaching, 
                                                                ppch )
        {
        }

    private:

        const COffsets s_offsetsCachedFileHeader = COffsets( 0, sizeof( CCachedFileHeader ) - 1 );
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
    //  trivial implementation:  nothing to do

    return JET_errSuccess;
}

template< class I >
ERR TPassThroughCache<I>::ErrInvalidate(    _In_ const VolumeId     volumeid,
                                            _In_ const FileId       fileid,
                                            _In_ const FileSerial   fileserial,
                                            _In_ const QWORD        ibOffset,
                                            _In_ const QWORD        cbData )
{
    ERR                                 err                 = JET_errSuccess;
    CPassThroughCachedFileTableEntry*   pcfte               = NULL;
    const COffsets                      offsets             = COffsets( ibOffset, ibOffset + cbData - 1 );
    DWORD                               cbHeaderInvalidated = 0;
    BYTE*                               rgbZero             = NULL;
    TraceContextScope                   tcScope( iorpBlockCache );

    //  trivial implementation:  invalidate the displaced data
   
    //  get the cached file.  note that we do not release this reference to leave the cached file open

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    //  if we are invalidating the data displaced by the header then explicitly clear the displaced data

    if ( offsets.FOverlaps( s_offsetsCachedFileHeader ) && offsets.Cb() > 0 )
    {
        cbHeaderInvalidated = (DWORD)min( offsets.Cb(), s_offsetsCachedFileHeader.IbEnd() - offsets.IbStart() + 1 );

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
                                    _In_                    const DWORD_PTR             keyComplete )
{
    ERR                                 err             = JET_errSuccess;
    CPassThroughCachedFileTableEntry*   pcfte           = NULL;
    const COffsets                      offsets         = COffsets( ibOffset, ibOffset + cbData - 1 );
    DWORD                               cbHeaderRead    = 0;
    CComplete*                          pcomplete       = NULL;

    if ( pfnComplete )
    {
        Alloc( pcomplete = new CComplete(   volumeid,
                                            fileid,
                                            fileserial,
                                            ibOffset, 
                                            cbData,
                                            pbData,
                                            pfnComplete, 
                                            keyComplete ) );
    }

    //  get the cached file.  note that we do not release this reference to leave the cached file open

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    //  trivial implementation:  simply pass the IO on to the cached file as a cache miss, except the displaced header

    //  telemetry

    ReportMiss( pcfte, ibOffset, cbData, fTrue, cp != cpDontCache );

    //  if this read is accessing the data displaced by the header then redirect that portion of the IO to the
    //  displaced data

    if ( offsets.FOverlaps( s_offsetsCachedFileHeader ) )
    {
        cbHeaderRead = (DWORD)min( offsets.Cb(), s_offsetsCachedFileHeader.IbEnd() - offsets.IbStart() + 1 );
    }

    if ( cbHeaderRead > 0 )
    {
        Call( pcfte->PffDisplacedData()->ErrIORead( tc,
                                                    offsets.IbStart(),
                                                    cbHeaderRead,
                                                    pbData, 
                                                    grbitQOS,
                                                    pcomplete ? (IFileAPI::PfnIOComplete)CComplete::IOComplete_ : NULL,
                                                    DWORD_PTR( pcomplete ),
                                                    pcomplete ? (IFileAPI::PfnIOHandoff)CComplete::IOHandoff_ : NULL ) );
        CallS( pcfte->PffDisplacedData()->ErrIOIssue() );
    }

    //  perform the original read less whatever displaced header was read

    if ( cbHeaderRead < cbData )
    {
        Call( pcfte->Pff()->ErrRead(    tc,
                                        ibOffset + cbHeaderRead,
                                        cbData - cbHeaderRead,
                                        pbData + cbHeaderRead,
                                        grbitQOS,
                                        iomCacheMiss,
                                        pcomplete ? (IFileAPI::PfnIOComplete)CComplete::IOComplete_ : NULL, 
                                        DWORD_PTR( pcomplete ),
                                        pcomplete ? (IFileAPI::PfnIOHandoff)CComplete::IOHandoff_ : NULL,
                                        NULL ) );
    }

HandleError:
    if ( pcomplete )
    {
        pcomplete->Release( err, tc, grbitQOS );
    }
    return pcomplete ? JET_errSuccess : err;
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
                                    _In_                    const DWORD_PTR             keyComplete )
{
    ERR                                 err             = JET_errSuccess;
    CPassThroughCachedFileTableEntry*   pcfte           = NULL;
    const COffsets                      offsets         = COffsets( ibOffset, ibOffset + cbData - 1 );
    DWORD                               cbHeaderWritten = 0;
    CComplete*                          pcomplete       = NULL;
    
    if ( pfnComplete )
    {
        Alloc( pcomplete = new CComplete(   volumeid,
                                            fileid,
                                            fileserial,
                                            ibOffset, 
                                            cbData,
                                            pbData,
                                            pfnComplete, 
                                            keyComplete ) );
    }

    //  get the cached file.  note that we do not release this reference to leave the cached file open

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    //  trivial implementation:  simply pass the IO on to the cached file as a write through, except the displaced header

    //  telemetry
  
    ReportMiss( pcfte, ibOffset, cbData, fFalse, cp != cpDontCache );
    ReportUpdate( pcfte, ibOffset, cbData );
    ReportWrite( pcfte, ibOffset, cbData, fTrue );

    //  if the write is accessing the data displaced by the header then redirect that portion of the IO to the
    //  displaced data

    if ( offsets.FOverlaps( s_offsetsCachedFileHeader ) )
    {
        cbHeaderWritten = (DWORD)min( offsets.Cb(), s_offsetsCachedFileHeader.IbEnd() - offsets.IbStart() + 1 );
    }

    if ( cbHeaderWritten > 0 )
    {
        Call( pcfte->PffDisplacedData()->ErrIOWrite(    tc,
                                                        offsets.IbStart(), 
                                                        cbHeaderWritten, 
                                                        pbData, 
                                                        qosIONormal,
                                                        pcomplete ? (IFileAPI::PfnIOComplete)CComplete::IOComplete_ : NULL, 
                                                        DWORD_PTR( pcomplete ),
                                                        pcomplete ? (IFileAPI::PfnIOHandoff)CComplete::IOHandoff_ : NULL ) );
        CallS( pcfte->PffDisplacedData()->ErrIOIssue() );
        pcfte->PffDisplacedData()->SetNoFlushNeeded();
    }

    //  perform the original write less whatever displaced header was written

    if ( cbHeaderWritten < cbData )
    {
        Call( pcfte->Pff()->ErrWrite(   tc,
                                        ibOffset + cbHeaderWritten,
                                        cbData - cbHeaderWritten,
                                        pbData + cbHeaderWritten,
                                        qosIONormal,
                                        iomCacheWriteThrough,
                                        pcomplete ? (IFileAPI::PfnIOComplete)CComplete::IOComplete_ : NULL, 
                                        DWORD_PTR( pcomplete ),
                                        pcomplete ? (IFileAPI::PfnIOHandoff)CComplete::IOHandoff_ : NULL ) );
    }

HandleError:
    if ( pcomplete )
    {
        pcomplete->Release( err, tc, grbitQOS );
    }
    return pcomplete ? JET_errSuccess : err;
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
