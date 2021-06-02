// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


//  CCacheFactory:  creates or mounts a cache based on its configuration

class CCacheFactory
{
    public:

        enum { cbGuid = sizeof(GUID) };

    public:

        static ERR ErrCreate(   _In_    IFileSystemFilter* const        pfsf,
                                _In_    IFileIdentification* const      pfident,
                                _In_    IFileSystemConfiguration* const pfsconfig, 
                                _Inout_ ICacheConfiguration** const     ppcconfig,
                                _In_    ICacheTelemetry* const          pctm,
                                _Inout_ IFileFilter** const             ppffCaching,
                                _Out_   ICache** const                  ppc );

        static ERR ErrMount(    _In_    IFileSystemFilter* const        pfsf,
                                _In_    IFileIdentification* const      pfident,
                                _In_    IFileSystemConfiguration* const pfsconfig, 
                                _Inout_ ICacheConfiguration** const     ppcconfig,
                                _In_    ICacheTelemetry* const          pctm,
                                _Inout_ IFileFilter** const             ppffCaching,
                                _Out_   ICache** const                  ppc );

        static ERR ErrDump( _In_    IFileSystemFilter* const        pfsf,
                            _In_    IFileIdentification* const      pfident,
                            _In_    IFileSystemConfiguration* const pfsconfig, 
                            _Inout_ ICacheConfiguration** const     ppcconfig,
                            _In_    ICacheTelemetry* const          pctm,
                            _Inout_ IFileFilter** const             ppffCaching,
                            _In_    CPRINTF* const                  pcprintf );

    private:

        CCacheFactory() {}

    private:

        typedef ICache* (*PfnFactory)(  _In_    IFileSystemFilter* const            pfsf,
                                        _In_    IFileIdentification* const          pfident,
                                        _In_    IFileSystemConfiguration* const     pfsconfig, 
                                        _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                                        _Inout_ ICacheConfiguration** const         ppcconfig,
                                        _In_    ICacheTelemetry* const              pctm,
                                        _Inout_ IFileFilter** const                 ppffCaching,
                                        _Inout_ CCacheHeader** const                ppch );

    private:

        static ERR ErrGetFactory(   _In_                    ICacheConfiguration* const          pcconfig,
                                    _Out_writes_( cbGuid )  BYTE* const                         rgbCacheType,
                                    _Out_                   CCacheFactory::PfnFactory* const    ppfnFactory );
        static ERR ErrGetFactory(   _In_reads_( cbGuid )    const BYTE* const                   rgbCacheType,
                                    _Out_                   CCacheFactory::PfnFactory* const    ppfnFactory );
};

INLINE ERR CCacheFactory::ErrCreate(    _In_    IFileSystemFilter* const        pfsf,
                                        _In_    IFileIdentification* const      pfident,
                                        _In_    IFileSystemConfiguration* const pfsconfig, 
                                        _Inout_ ICacheConfiguration** const     ppcconfig,
                                        _In_    ICacheTelemetry* const          pctm,
                                        _Inout_ IFileFilter** const             ppffCaching,
                                        _Out_   ICache** const                  ppc )
{
    ERR                         err                     = JET_errSuccess;
    BYTE                        rgbCacheType[ cbGuid ]  = { 0 };
    PfnFactory                  pfnFactory              = NULL;
    QWORD                       cbSize                  = 0;
    CCacheHeader*               pch                     = NULL;
    TraceContextScope           tcScope( iorpBlockCache );
    IBlockCacheConfiguration*   pbcconfig               = NULL;
    ICache*                     pc                      = NULL;

    *ppc = NULL;

    //  determine the type of cache to create

    Call( ErrGetFactory( *ppcconfig, rgbCacheType, &pfnFactory ) );

    //  verify that the file is empty to avoid unintentional data destruction

    Call( (*ppffCaching)->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
    if ( cbSize != 0 )
    {
        Error( ErrERRCheck( JET_errInternalError ) );
    }

    //  initialize the caching file header

    Call( CCacheHeader::ErrCreate( rgbCacheType, *ppffCaching, &pch ) );

    //  write the caching file header

    Call( (*ppffCaching)->ErrIOWrite(   *tcScope,
                                        0, 
                                        sizeof( *pch ),
                                        (const BYTE*)pch,
                                        qosIONormal, 
                                        NULL, 
                                        NULL, 
                                        NULL ) );

    //  flush the caching file

    Call( (*ppffCaching)->ErrFlushFileBuffers( iofrBlockCache ) );

    //  get the block cache configuration

    Call( pfsconfig->ErrGetBlockCacheConfiguration( &pbcconfig ) );

    //  construct the cache

    Alloc( pc = pfnFactory( pfsf, pfident, pfsconfig, &pbcconfig, ppcconfig, pctm, ppffCaching, &pch ) );

    //  create the cache

    Call( pc->ErrCreate() );

    //  mount the cache

    Call( pc->ErrMount() );

    //  return the cache

    *ppc = pc;
    pc = NULL;

HandleError:
    delete pc;
    delete pbcconfig;
    delete pch;
    return err;
}

INLINE ERR CCacheFactory::ErrMount( _In_    IFileSystemFilter* const        pfsf,
                                    _In_    IFileIdentification* const      pfident,
                                    _In_    IFileSystemConfiguration* const pfsconfig, 
                                    _Inout_ ICacheConfiguration** const     ppcconfig,
                                    _In_    ICacheTelemetry* const          pctm,
                                    _Inout_ IFileFilter** const             ppffCaching,
                                    _Out_   ICache** const                  ppc )
{
    ERR                         err         = JET_errSuccess;
    CCacheHeader*               pch         = NULL;
    PfnFactory                  pfnFactory  = NULL;
    IBlockCacheConfiguration*   pbcconfig   = NULL;
    ICache*                     pc          = NULL;

    *ppc = NULL;

    //  read the header

    Call( CCacheHeader::ErrLoad( pfsconfig, *ppffCaching, &pch ) );

    //  determine the type of cache to mount

    Call( ErrGetFactory( pch->RgbCacheType(), &pfnFactory ) );

    //  get the block cache configuration

    Call( pfsconfig->ErrGetBlockCacheConfiguration( &pbcconfig ) );

    //  construct the cache

    Alloc( pc = pfnFactory( pfsf, pfident, pfsconfig, &pbcconfig, ppcconfig, pctm, ppffCaching, &pch ) );

    //  mount the cache

    Call( pc->ErrMount() );

    //  return the cache

    *ppc = pc;
    pc = NULL;

HandleError:
    delete pc;
    delete pbcconfig;
    delete pch;
    if ( err < JET_errSuccess )
    {
        delete *ppc;
        *ppc = NULL;
    }
    return err;
}

INLINE ERR CCacheFactory::ErrDump(  _In_    IFileSystemFilter* const        pfsf,
                                    _In_    IFileIdentification* const      pfident,
                                    _In_    IFileSystemConfiguration* const pfsconfig, 
                                    _Inout_ ICacheConfiguration** const     ppcconfig,
                                    _In_    ICacheTelemetry* const          pctm,
                                    _Inout_ IFileFilter** const             ppffCaching,
                                    _In_    CPRINTF* const                  pcprintf )
{
    ERR                         err         = JET_errSuccess;
    CCacheHeader*               pch         = NULL;
    PfnFactory                  pfnFactory  = NULL;
    IBlockCacheConfiguration*   pbcconfig   = NULL;
    ICache*                     pc          = NULL;

    //  read the header

    Call( CCacheHeader::ErrLoad( pfsconfig, *ppffCaching, &pch ) );

    //  determine the type of cache to mount

    Call( ErrGetFactory( pch->RgbCacheType(), &pfnFactory ) );

    //  get the block cache configuration

    Call( pfsconfig->ErrGetBlockCacheConfiguration( &pbcconfig ) );

    //  construct the cache

    Alloc( pc = pfnFactory( pfsf, pfident, pfsconfig, &pbcconfig, ppcconfig, pctm, ppffCaching, &pch ) );

    //  dump the cache

    Call( pc->ErrDump( pcprintf ) );

HandleError:
    delete pc;
    delete pbcconfig;
    delete pch;
    return err;
}

INLINE ERR CCacheFactory::ErrGetFactory(    _In_                    ICacheConfiguration* const          pcconfig,
                                            _Out_writes_( cbGuid )  BYTE* const                         rgbCacheType,
                                            _Out_                   CCacheFactory::PfnFactory* const    ppfnFactory )
{
    ERR err = JET_errSuccess;

    *ppfnFactory = NULL;
    memset( rgbCacheType, 0, cbGuid );

    //  get the cache type id from the cache configuration

    pcconfig->CacheType( rgbCacheType );

    //  find the factory method for this cache type id

    Call( ErrGetFactory( rgbCacheType, ppfnFactory ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        *ppfnFactory = NULL;
        memset( rgbCacheType, 0, cbGuid );
    }
    return err;
}

INLINE ERR CCacheFactory::ErrGetFactory(    _In_reads_( cbGuid )    const BYTE* const                   rgbCacheType,
                                            _Out_                   CCacheFactory::PfnFactory* const    ppfnFactory )
{
    ERR err = JET_errSuccess;

    *ppfnFactory = NULL;

    //  find the factory method for this cache type id

    if ( memcmp( CPassThroughCache::RgbCacheType(), rgbCacheType, cbGuid ) == 0 )
    {
        *ppfnFactory = CPassThroughCache::PcCreate;
    }
    else if ( memcmp( CHashedLRUKCache::RgbCacheType(), rgbCacheType, cbGuid ) == 0 )
    {
        *ppfnFactory = CHashedLRUKCache::PcCreate;
    }

    //  if we didn't find the factory method then fail

    if ( *ppfnFactory == NULL )
    {
        Error( ErrERRCheck( JET_errInternalError ) );
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        *ppfnFactory = NULL;
    }
    return err;
}
