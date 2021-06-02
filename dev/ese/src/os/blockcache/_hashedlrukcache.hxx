// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


//  THashedLRUKCache:  a persistent write back cache implemented as a fixed size hash of chunks each running the LRUK
//  cache replacement algorithm.  crash resilience is provided by a journal

template< class I >
class THashedLRUKCache
    :   public TCacheBase<I, CHashedLRUKCachedFileTableEntry>
{
    public:

        ~THashedLRUKCache()
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
        
        THashedLRUKCache(   _In_    IFileSystemFilter* const            pfsf,
                            _In_    IFileIdentification* const          pfident,
                            _In_    IFileSystemConfiguration* const     pfsconfig, 
                            _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                            _Inout_ ICacheConfiguration** const         ppcconfig,
                            _In_    ICacheTelemetry* const              pctm,
                            _Inout_ IFileFilter** const                 ppffCaching,
                            _Inout_ CCacheHeader** const                ppch )
            : TCacheBase<I, CHashedLRUKCachedFileTableEntry>(   pfsf,
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

        CHashedLRUKCacheHeader* m_pch;
};

template< class I >
ERR THashedLRUKCache<I>::ErrCreate()
{
    ERR                     err     = JET_errSuccess;
    CHashedLRUKCacheHeader* pch     = NULL;
    TraceContextScope       tcScope( iorpBlockCache );

    //  initialize the caching file header

    Call( CHashedLRUKCacheHeader::ErrCreate( &pch ) );

    //  set the initial caching file size

    Call( PffCaching()->ErrSetSize( *tcScope, Pcconfig()->CbMaximumSize(), fFalse, qosIONormal ) );

    //  write the caching file header

    Call( PffCaching()->ErrIOWrite( *tcScope,
                                    sizeof( CCacheHeader ),
                                    sizeof( *pch ),
                                    (const BYTE*)pch,
                                    qosIONormal, 
                                    NULL, 
                                    NULL,
                                    NULL ) );

    //  flush the caching file

    Call( PffCaching()->ErrFlushFileBuffers( iofrBlockCache ) );

HandleError:
    delete pch;
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrMount()
{
    ERR                     err     = JET_errSuccess;
    CHashedLRUKCacheHeader* pch     = NULL;

    //  read the header

    Call( CHashedLRUKCacheHeader::ErrLoad( Pfsconfig(), PffCaching(), &pch ) );

    //  save the caching file header

    m_pch = pch;
    pch = NULL;

    //  AEG

HandleError:
    delete pch;
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrDump( _In_ CPRINTF* const pcprintf )
{
    ERR err = JET_errSuccess;

    Call( m_pch->ErrDump( pcprintf ) );

    //  AEG

HandleError:
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrFlush(  _In_ const VolumeId     volumeid,
                                    _In_ const FileId       fileid,
                                    _In_ const FileSerial   fileserial )
{
    ERR                                 err     = JET_errSuccess;
    CHashedLRUKCachedFileTableEntry*    pcfte   = NULL;
   
    //  get the cached file.  note that we do not release this reference to leave the cached file open

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    //  AEG

HandleError:
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrInvalidate( _In_ const VolumeId     volumeid,
                                        _In_ const FileId       fileid,
                                        _In_ const FileSerial   fileserial,
                                        _In_ const QWORD        ibOffset,
                                        _In_ const QWORD        cbData )
{
    ERR                                 err     = JET_errSuccess;
    CHashedLRUKCachedFileTableEntry*    pcfte   = NULL;
   
    //  get the cached file.  note that we do not release this reference to leave the cached file open

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    //  AEG

HandleError:
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrRead(   _In_                    const TraceContext&         tc,
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
    CHashedLRUKCachedFileTableEntry*    pcfte           = NULL;
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

    //  AEG

HandleError:
    if ( pcomplete )
    {
        pcomplete->Release( err, tc, grbitQOS );
    }
    return pcomplete ? JET_errSuccess : err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrWrite(  _In_                    const TraceContext&         tc,
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
    CHashedLRUKCachedFileTableEntry*    pcfte           = NULL;
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

    //  AEG

HandleError:
    if ( pcomplete )
    {
        pcomplete->Release( err, tc, grbitQOS );
    }
    return pcomplete ? JET_errSuccess : err;
}

//  CHashedLRUKCache:  concrete THashedLRUKCache<ICache>

//  1c73cc4d-ac35-41d9-a9e0-e46112c03a87
const BYTE c_rgbHashedLRUKCacheType[ sizeof( GUID ) ] = { 0x4D, 0xCC, 0x73, 0x1C, 0x35, 0xAC, 0xD9, 0x41, 0xA9, 0xE0, 0xE4, 0x61, 0x12, 0xC0, 0x3A, 0x87 };

class CHashedLRUKCache : public THashedLRUKCache<ICache>
{
    public:  //  specialized API

        static const BYTE* RgbCacheType()
        {
            return c_rgbHashedLRUKCacheType;
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
            return new CHashedLRUKCache( pfsf, pfident, pfsconfig, ppbcconfig, ppcconfig, pctm, ppffCaching, ppch );
        }

    private:

        CHashedLRUKCache(   _In_    IFileSystemFilter* const            pfsf,
                            _In_    IFileIdentification* const          pfident,
                            _In_    IFileSystemConfiguration* const     pfsconfig,
                            _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                            _Inout_ ICacheConfiguration** const         ppcconfig,
                            _In_    ICacheTelemetry* const              pctm,
                            _Inout_ IFileFilter** const                 ppffCaching,
                            _Inout_ CCacheHeader** const                ppch )
            :   THashedLRUKCache<ICache>( pfsf, pfident, pfsconfig, ppbcconfig, ppcconfig, pctm, ppffCaching, ppch )
        {
        }
};
