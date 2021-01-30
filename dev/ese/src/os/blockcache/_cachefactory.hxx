// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once



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


    Call( ErrGetFactory( *ppcconfig, rgbCacheType, &pfnFactory ) );


    Call( (*ppffCaching)->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
    if ( cbSize != 0 )
    {
        Error( ErrERRCheck( JET_errInternalError ) );
    }


    Call( CCacheHeader::ErrCreate( rgbCacheType, *ppffCaching, &pch ) );


    Call( (*ppffCaching)->ErrIOWrite(   *tcScope,
                                        0, 
                                        sizeof( *pch ),
                                        (const BYTE*)pch,
                                        qosIONormal, 
                                        NULL, 
                                        NULL, 
                                        NULL ) );


    Call( (*ppffCaching)->ErrFlushFileBuffers( iofrBlockCache ) );


    Call( pfsconfig->ErrGetBlockCacheConfiguration( &pbcconfig ) );


    Alloc( pc = pfnFactory( pfsf, pfident, pfsconfig, &pbcconfig, ppcconfig, pctm, ppffCaching, &pch ) );


    Call( pc->ErrCreate() );


    Call( pc->ErrMount() );


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


    Call( CCacheHeader::ErrLoad( pfsconfig, *ppffCaching, &pch ) );


    Call( ErrGetFactory( pch->RgbCacheType(), &pfnFactory ) );


    Call( pfsconfig->ErrGetBlockCacheConfiguration( &pbcconfig ) );


    Alloc( pc = pfnFactory( pfsf, pfident, pfsconfig, &pbcconfig, ppcconfig, pctm, ppffCaching, &pch ) );


    Call( pc->ErrMount() );


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


    Call( CCacheHeader::ErrLoad( pfsconfig, *ppffCaching, &pch ) );


    Call( ErrGetFactory( pch->RgbCacheType(), &pfnFactory ) );


    Call( pfsconfig->ErrGetBlockCacheConfiguration( &pbcconfig ) );


    Alloc( pc = pfnFactory( pfsf, pfident, pfsconfig, &pbcconfig, ppcconfig, pctm, ppffCaching, &pch ) );


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


    pcconfig->CacheType( rgbCacheType );


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


    if ( memcmp( CPassThroughCache::RgbCacheType(), rgbCacheType, cbGuid ) == 0 )
    {
        *ppfnFactory = CPassThroughCache::PcCreate;
    }
    else if ( memcmp( CHashedLRUKCache::RgbCacheType(), rgbCacheType, cbGuid ) == 0 )
    {
        *ppfnFactory = CHashedLRUKCache::PcCreate;
    }


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
