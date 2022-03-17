// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Block Cache Factory

class COSBlockCacheFactoryImpl : public IBlockCacheFactory
{
    public:  //  specialized API

        ~COSBlockCacheFactoryImpl() {}

    public:  //  IBlockCacheFactory

        ERR ErrCreateFileSystemWrapper( _Inout_ IFileSystemAPI** const  ppfsapiInner,
                                        _Out_   IFileSystemAPI** const  ppfsapi ) override;

        ERR ErrCreateFileSystemFilter(  _In_    IFileSystemConfiguration* const pfsconfig,
                                        _Inout_ IFileSystemAPI** const          ppfsapiInner,
                                        _In_    IFileIdentification* const      pfident,
                                        _In_    ICacheTelemetry* const          pctm,
                                        _In_    ICacheRepository* const         pcrep,
                                        _Out_   IFileSystemFilter** const       ppfsf ) override;

        ERR ErrCreateFileWrapper(   _Inout_ IFileAPI** const    ppfapiInner,
                                    _Out_   IFileAPI** const    ppfapi ) override;

        ERR ErrCreateFileFilter(    _Inout_                     IFileAPI** const                    ppfapiInner,
                                    _In_                        IFileSystemFilter* const            pfsf,
                                    _In_                        IFileSystemConfiguration* const     pfsconfig,
                                    _In_                        ICacheTelemetry* const              pctm,
                                    _In_                        const VolumeId                      volumeid,
                                    _In_                        const FileId                        fileid,
                                    _Inout_                     ICachedFileConfiguration** const    ppcfconfig,
                                    _Inout_                     ICache** const                      ppc,
                                    _In_reads_opt_( cbHeader )  const BYTE* const                   pbHeader,
                                    _In_                        const int                           cbHeader,
                                    _Out_                       IFileFilter** const                 ppff ) override;
        ERR ErrCreateFileFilterWrapper( _Inout_ IFileFilter** const         ppffInner,
                                        _In_    const IFileFilter::IOMode   iom,
                                        _Out_   IFileFilter** const         ppff ) override;

        ERR ErrCreateFileIdentification( _Out_ IFileIdentification** const ppfident ) override;

        ERR ErrCreateCache( _In_    IFileSystemFilter* const        pfsf,
                            _In_    IFileIdentification* const      pfident,
                            _In_    IFileSystemConfiguration* const pfsconfig,
                            _Inout_ ICacheConfiguration** const     ppcconfig,
                            _In_    ICacheTelemetry* const          pctm,
                            _Inout_ IFileFilter** const             ppffCaching,
                            _Out_   ICache** const                  ppc ) override;

        ERR ErrMountCache(  _In_    IFileSystemFilter* const        pfsf,
                            _In_    IFileIdentification* const      pfident,
                            _In_    IFileSystemConfiguration* const pfsconfig,
                            _Inout_ ICacheConfiguration** const     ppcconfig,
                            _In_    ICacheTelemetry* const          pctm,
                            _Inout_ IFileFilter** const             ppffCaching,
                            _Out_   ICache** const                  ppc ) override;

        ERR ErrCreateCacheWrapper(  _Inout_ ICache** const  ppcInner,
                                    _Out_   ICache** const  ppc ) override;

        ERR ErrCreateCacheRepository(   _In_    IFileIdentification* const      pfident,
                                        _In_    ICacheTelemetry* const          pctm,
                                        _Out_   ICacheRepository** const        ppcrep ) override;

        ERR ErrCreateCacheTelemetry( _Out_ ICacheTelemetry** const ppctm ) override;

        ERR ErrDumpCachedFileHeader(    _In_z_  const WCHAR* const  wszFilePath,
                                        _In_    const ULONG         grbit,
                                        _In_    CPRINTF* const      pcprintf ) override;
        ERR ErrDumpCacheFile(   _In_z_  const WCHAR* const  wszFilePath, 
                                _In_    const ULONG         grbit,
                                _In_    CPRINTF* const      pcprintf ) override;

        ERR ErrCreateJournalSegment(    _In_    IFileFilter* const      pff,
                                        _In_    const QWORD             ib,
                                        _In_    const SegmentPosition   spos,
                                        _In_    const DWORD             dwUniqueIdPrev,
                                        _In_    const SegmentPosition   sposReplay,
                                        _In_    const SegmentPosition   sposDurable,
                                        _Out_   IJournalSegment** const ppjs ) override;

        ERR ErrLoadJournalSegment(  _In_    IFileFilter* const      pff,
                                    _In_    const QWORD             ib,
                                    _Out_   IJournalSegment** const ppjs ) override;

        ERR ErrCreateJournalSegmentManager( _In_    IFileFilter* const              pff,
                                            _In_    const QWORD                     ib,
                                            _In_    const QWORD                     cb,
                                            _Out_   IJournalSegmentManager** const  ppjsm ) override;

        ERR ErrCreateJournal(   _Inout_ IJournalSegmentManager** const  ppjsm,
                                _In_    const size_t                    cbCache,
                                _Out_   IJournal** const                ppj ) override;

        ERR ErrLoadCachedBlockWriteCountsManager(   _In_    IFileFilter* const                      pff,
                                                    _In_    const QWORD                             ib,
                                                    _In_    const QWORD                             cb,
                                                    _Out_   ICachedBlockWriteCountsManager** const  ppcbwcm ) override;

        ERR ErrCreateCachedBlockSlabWrapper(    _Inout_ ICachedBlockSlab** const    ppcbsInner,
                                                _Out_   ICachedBlockSlab** const    ppcbs ) override;

        ERR ErrLoadCachedBlockSlab( _In_    IFileFilter* const                      pff,
                                    _In_    const QWORD                             ibSlab,
                                    _In_    const QWORD                             cbSlab,
                                    _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                                    _In_    const QWORD                             icbwcBase,
                                    _In_    const ClusterNumber                     clnoMin,
                                    _In_    const ClusterNumber                     clnoMax,
                                    _In_    const BOOL                              fIgnoreVerificationErrors,
                                    _Inout_ ICachedBlockSlab** const                ppcbs ) override;

        ERR ErrLoadCachedBlockSlabManager(  _In_    IFileFilter* const                      pff,
                                            _In_    const QWORD                             cbCachingFilePerSlab,
                                            _In_    const QWORD                             cbCachedFilePerSlab,
                                            _In_    const QWORD                             ibChunk,
                                            _In_    const QWORD                             cbChunk,
                                            _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                                            _In_    const QWORD                             icbwcBase,
                                            _In_    const ClusterNumber                     clnoMin,
                                            _In_    const ClusterNumber                     clnoMax,
                                            _Out_   ICachedBlockSlabManager** const         ppcbsm ) override;

        ERR ErrCreateCachedBlock(   _In_    const CCachedBlockId    cbid,
                                    _In_    const ClusterNumber     clno,
                                    _In_    const BOOL              fValid,
                                    _In_    const BOOL              fPinned,
                                    _In_    const BOOL              fDirty,
                                    _In_    const BOOL              fEverDirty,
                                    _In_    const BOOL              fPurged,
                                    _In_    const UpdateNumber      updno,
                                    _Out_   CCachedBlock*           pcbl ) override;

        ERR ErrCreateCachedBlockSlot(   _In_    const QWORD         ibSlab,
                                        _In_    const ChunkNumber   chno,
                                        _In_    const SlotNumber    slno,
                                        _In_    const CCachedBlock& cbl,
                                        _Out_   CCachedBlockSlot*   pslot ) override;

        ERR ErrCreateCachedBlockSlotState(  _In_    const CCachedBlockSlot& slot,
                                            _In_    const BOOL              fSlabUpdated,
                                            _In_    const BOOL              fChunkUpdated,
                                            _In_    const BOOL              fSlotUpdated,
                                            _In_    const BOOL              fClusterUpdated,
                                            _In_    const BOOL              fSuperceded,
                                            _Out_   CCachedBlockSlotState*  pslotst ) override;
};

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateFileSystemWrapper(    _Inout_ IFileSystemAPI** const  ppfsapiInner,
                                                                    _Out_   IFileSystemAPI** const  ppfsapi )
{
    ERR             err     = JET_errSuccess;
    IFileSystemAPI* pfsapi  = NULL;

    *ppfsapi = NULL;

    //  create the file system wrapper

    Alloc( pfsapi = new CFileSystemWrapper( ppfsapiInner ) );

    *ppfsapi = pfsapi;
    pfsapi = NULL;

HandleError:
    delete pfsapi;
    if ( err < JET_errSuccess )
    {
        delete *ppfsapi;
        *ppfsapi = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateFileSystemFilter( _In_    IFileSystemConfiguration* const pfsconfig,
                                                                _Inout_ IFileSystemAPI** const          ppfsapiInner,
                                                                _In_    IFileIdentification* const      pfident,
                                                                _In_    ICacheTelemetry* const          pctm,
                                                                _In_    ICacheRepository* const         pcrep,
                                                                _Out_   IFileSystemFilter** const       ppfsf )
{
    ERR                 err     = JET_errSuccess;
    IFileSystemFilter*  pfsf    = NULL;

    *ppfsf = NULL;

    //  create the file system filter

    Alloc( pfsf = new CFileSystemFilter( pfsconfig, ppfsapiInner, pfident, pctm, pcrep ) );

    *ppfsf = pfsf;
    pfsf = NULL;

HandleError:
    delete pfsf;
    if ( err < JET_errSuccess )
    {
        delete *ppfsf;
        *ppfsf = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateFileWrapper(  _Inout_ IFileAPI** const    ppfapiInner,
                                                            _Out_   IFileAPI** const    ppfapi )
{
    ERR         err     = JET_errSuccess;
    IFileAPI*   pfapi   = NULL;

    *ppfapi = NULL;

    //  create the file wrapper

    Alloc( pfapi = new CFileWrapper( ppfapiInner ) );

    *ppfapi = pfapi;
    pfapi = NULL;

HandleError:
    delete pfapi;
    if ( err < JET_errSuccess )
    {
        delete *ppfapi;
        *ppfapi = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateFileFilter(   _Inout_                     IFileAPI** const                    ppfapiInner,
                                                            _In_                        IFileSystemFilter* const            pfsf,
                                                            _In_                        IFileSystemConfiguration* const     pfsconfig,
                                                            _In_                        ICacheTelemetry* const              pctm,
                                                            _In_                        const VolumeId                      volumeid,
                                                            _In_                        const FileId                        fileid,
                                                            _Inout_                     ICachedFileConfiguration** const    ppcfconfig,
                                                            _Inout_                     ICache** const                      ppc,
                                                            _In_reads_opt_( cbHeader )  const BYTE* const                   pbHeader,
                                                            _In_                        const int                           cbHeader,
                                                            _Out_                       IFileFilter** const                 ppff )
{
    ERR                 err     = JET_errSuccess;
    IFileFilter*        pff     = NULL;
    CCachedFileHeader*  pcfh    = NULL;

    *ppff = NULL;

    //  marshal the cached file header.  note that an expected case is not getting a valid header

    if ( pbHeader )
    {
        (void)CCachedFileHeader::ErrLoad( pfsconfig, pbHeader, cbHeader, &pcfh );
    }

    //  create the file filter

    Alloc( pff = new CFileFilter( ppfapiInner, pfsf, pfsconfig, pctm, volumeid, fileid, ppcfconfig, ppc, &pcfh ) );

    //  return the file filter

    *ppff = pff;
    pff = NULL;

HandleError:
    delete pff;
    delete pcfh;
    if ( err < JET_errSuccess )
    {
        delete *ppff;
        *ppff = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateFileFilterWrapper(    _Inout_ IFileFilter** const         ppffInner,
                                                                    _In_    const IFileFilter::IOMode   iom,
                                                                    _Out_   IFileFilter** const         ppff )
{
    ERR             err = JET_errSuccess;
    IFileFilter*    pff = NULL;

    *ppff = NULL;

    //  create the file filter wrapper

    Alloc( pff = new CFileFilterWrapper( ppffInner, iom ) );

    *ppff = pff;
    pff = NULL;

HandleError:
    delete pff;
    if ( err < JET_errSuccess )
    {
        delete *ppff;
        *ppff = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateFileIdentification( _Out_ IFileIdentification** const ppfident )
{
    ERR err = JET_errSuccess;

    *ppfident = NULL;

    //  create the file identification

    Alloc( *ppfident = new CFileIdentification() );

HandleError:
    if ( err < JET_errSuccess )
    {
        delete *ppfident;
        *ppfident = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateCache(    _In_    IFileSystemFilter* const        pfsf,
                                                        _In_    IFileIdentification* const      pfident,
                                                        _In_    IFileSystemConfiguration* const pfsconfig,
                                                        _Inout_ ICacheConfiguration** const     ppcconfig,
                                                        _In_    ICacheTelemetry* const          pctm,
                                                        _Inout_ IFileFilter** const             ppff,
                                                        _Out_   ICache** const                  ppc )
{
    ERR     err = JET_errSuccess;
    ICache* pc  = NULL;

    *ppc = NULL;

    //  create the cache

    Call( CCacheFactory::ErrCreate( pfsf, pfident, pfsconfig, ppcconfig, pctm, ppff, &pc ) );

    //  return the cache

    *ppc = pc;
    pc = NULL;

HandleError:
    delete pc;
    if ( err < JET_errSuccess )
    {
        delete *ppc;
        *ppc = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrMountCache( _In_    IFileSystemFilter* const        pfsf,
                                                    _In_    IFileIdentification* const      pfident,
                                                    _In_    IFileSystemConfiguration* const pfsconfig,
                                                    _Inout_ ICacheConfiguration** const     ppcconfig,
                                                    _In_    ICacheTelemetry* const          pctm,
                                                    _Inout_ IFileFilter** const             ppff,
                                                    _Out_   ICache** const                  ppc )
{
    ERR     err = JET_errSuccess;
    ICache* pc  = NULL;

    *ppc = NULL;

    //  mount the cache

    Call( CCacheFactory::ErrMount( pfsf, pfident, pfsconfig, ppcconfig, pctm, ppff, &pc ) );

    //  return the cache

    *ppc = pc;
    pc = NULL;

HandleError:
    delete pc;
    if ( err < JET_errSuccess )
    {
        delete *ppc;
        *ppc = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateCacheWrapper( _Inout_ ICache** const  ppcInner,
                                                            _Out_   ICache** const  ppc )
{
    ERR     err = JET_errSuccess;
    ICache* pc  = NULL;

    *ppc = NULL;

    //  create the cache wrapper

    Alloc( pc = new CCacheWrapper( ppcInner ) );

    *ppc = pc;
    pc = NULL;

HandleError:
    delete pc;
    if ( err < JET_errSuccess )
    {
        delete *ppc;
        *ppc = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateCacheRepository(  _In_    IFileIdentification* const      pfident,
                                                                _In_    ICacheTelemetry* const          pctm,
                                                                _Out_   ICacheRepository** const        ppcrep )
{
    ERR err = JET_errSuccess;

    *ppcrep = NULL;

    //  create the cache repository

    Alloc( *ppcrep = new CCacheRepository( pfident, pctm ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        delete *ppcrep;
        *ppcrep = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateCacheTelemetry( _Out_ ICacheTelemetry** const ppctm )
{
    ERR err = JET_errSuccess;

    *ppctm = NULL;

    //  create the cache telemetry

    Alloc( *ppctm = new CCacheTelemetry() );

HandleError:
    if ( err < JET_errSuccess )
    {
        delete * ppctm;
        *ppctm = NULL;
    }
    return err;
}

extern CFileIdentification g_fident;
extern CCacheTelemetry g_ctm;

INLINE ERR COSBlockCacheFactoryImpl::ErrDumpCachedFileHeader(   _In_z_  const WCHAR* const  wszFilePath,
                                                                _In_    const ULONG         grbit,
                                                                _In_    CPRINTF* const      pcprintf )
{
    ERR             err     = JET_errSuccess;
    IFileSystemAPI* pfsapi  = NULL;
    IFileFilter*    pff     = NULL;

    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
    {
        public:

            CFileSystemConfiguration()
            {
                m_dtickAccessDeniedRetryPeriod = 0;
                m_fBlockCacheEnabled = fTrue;
            }
    } fsconfig;

    Call( ErrOSFSCreate( &fsconfig, &pfsapi ) );

    Call( pfsapi->ErrFileOpen( wszFilePath, IFileAPI::fmfNone, (IFileAPI**)&pff ) );

    Call( CCachedFileHeader::ErrDump( &fsconfig, &g_fident, pff, CPRINTFSTDOUT::PcprintfInstance() ) );

HandleError:
    delete pff;
    delete pfsapi;
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrDumpCacheFile(  _In_z_  const WCHAR* const  wszFilePath,
                                                        _In_    const ULONG         grbit,
                                                        _In_    CPRINTF* const      pcprintf )
{
    ERR                     err         = JET_errSuccess;
    ICacheConfiguration*    pcconfig    = NULL;
    IFileSystemFilter*      pfsf        = NULL;
    IFileFilter*            pff         = NULL;

    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
    {
        public:

            CFileSystemConfiguration()
            {
                m_dtickAccessDeniedRetryPeriod = 0;
                m_fBlockCacheEnabled = fTrue;
            }
    } fsconfig;
    
    class CCacheConfiguration : public CDefaultCacheConfiguration
    {
        public:

            CCacheConfiguration()
            {
            }
    };
    Alloc( pcconfig = new CCacheConfiguration() );

    Call( ErrOSFSCreate( &fsconfig, (IFileSystemAPI**)&pfsf ) );

    Call( pfsf->ErrFileOpen( wszFilePath, IFileAPI::fmfNone, (IFileAPI**)&pff ) );

    Call( CCacheFactory::ErrDump( pfsf, &g_fident, &fsconfig, &pcconfig, &g_ctm, &pff, CPRINTFSTDOUT::PcprintfInstance() ) );

HandleError:
    delete pff;
    delete pfsf;
    delete pcconfig;
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateJournalSegment(   _In_    IFileFilter* const      pff,
                                                                _In_    const QWORD             ib,
                                                                _In_    const SegmentPosition   spos,
                                                                _In_    const DWORD             dwUniqueIdPrev,
                                                                _In_    const SegmentPosition   sposReplay,
                                                                _In_    const SegmentPosition   sposDurable,
                                                                _Out_   IJournalSegment** const ppjs )
{
    ERR                 err = JET_errSuccess;
    IJournalSegment*    pjs = NULL;

    *ppjs = NULL;

    Call( CJournalSegment::ErrCreate( pff, ib, spos, dwUniqueIdPrev, sposReplay, sposDurable, &pjs ) );

    *ppjs = pjs;
    pjs = NULL;

HandleError:
    delete pjs;
    if ( err < JET_errSuccess )
    {
        delete *ppjs;
        *ppjs = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrLoadJournalSegment( _In_    IFileFilter* const      pff,
                                                            _In_    const QWORD             ib,
                                                            _Out_   IJournalSegment** const ppjs )
{
    ERR                 err = JET_errSuccess;
    IJournalSegment*    pjs = NULL;

    *ppjs = NULL;

    Call( CJournalSegment::ErrLoad( pff, ib, &pjs ) );

    *ppjs = pjs;
    pjs = NULL;

HandleError:
    delete pjs;
    if ( err < JET_errSuccess )
    {
        delete *ppjs;
        *ppjs = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateJournalSegmentManager(    _In_    IFileFilter* const              pff,
                                                                        _In_    const QWORD                     ib,
                                                                        _In_    const QWORD                     cb,
                                                                        _Out_   IJournalSegmentManager** const  ppjsm )
{
    ERR                     err     = JET_errSuccess;
    IJournalSegmentManager* pjsm    = NULL;

    *ppjsm = NULL;

    Call( CJournalSegmentManager::ErrMount( pff, ib, cb, &pjsm ) );

    *ppjsm = pjsm;
    pjsm = NULL;

HandleError:
    delete pjsm;
    if ( err < JET_errSuccess )
    {
        delete *ppjsm;
        *ppjsm = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateJournal(  _Inout_ IJournalSegmentManager** const  ppjsm,
                                                        _In_    const size_t                    cbCache,
                                                        _Out_   IJournal** const                ppj )
{
    ERR         err = JET_errSuccess;
    IJournal*   pj  = NULL;

    *ppj = NULL;

    Call( CJournal::ErrMount( NULL, ppjsm, cbCache, &pj ) );

    *ppj = pj;
    pj = NULL;

HandleError:
    delete pj;
    if ( err < JET_errSuccess )
    {
        delete *ppj;
        *ppj = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrLoadCachedBlockWriteCountsManager(  _In_    IFileFilter* const                      pff, 
                                                                            _In_    const QWORD                             ib, 
                                                                            _In_    const QWORD                             cb,
                                                                            _Out_   ICachedBlockWriteCountsManager** const  ppcbwcm )
{
    ERR                             err     = JET_errSuccess;
    ICachedBlockWriteCountsManager* pcbwcm  = NULL;

    *ppcbwcm = NULL;

    Call( CCachedBlockWriteCountsManager::ErrInit( pff, ib, cb, &pcbwcm ) );

    *ppcbwcm = pcbwcm;
    pcbwcm = NULL;

HandleError:
    delete pcbwcm;
    if ( err < JET_errSuccess )
    {
        delete *ppcbwcm;
        *ppcbwcm = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateCachedBlockSlabWrapper(   _Inout_ ICachedBlockSlab** const    ppcbsInner,
                                                                        _Out_   ICachedBlockSlab** const    ppcbs )
{
    ERR                 err     = JET_errSuccess;
    ICachedBlockSlab*   pcbs    = NULL;

    *ppcbs = NULL;

    //  create the cached block slab wrapper

    Alloc( pcbs = new CCachedBlockSlabWrapper( ppcbsInner ) );

    *ppcbs = pcbs;
    pcbs = NULL;

HandleError:
    delete pcbs;
    if ( err < JET_errSuccess )
    {
        delete *ppcbs;
        *ppcbs = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrLoadCachedBlockSlab(    _In_    IFileFilter* const                      pff,
                                                                _In_    const QWORD                             ibSlab,
                                                                _In_    const QWORD                             cbSlab,
                                                                _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                                                                _In_    const QWORD                             icbwcBase,
                                                                _In_    const ClusterNumber                     clnoMin,
                                                                _In_    const ClusterNumber                     clnoMax,
                                                                _In_    const BOOL                              fIgnoreVerificationErrors,
                                                                _Inout_ ICachedBlockSlab** const                ppcbs )
{
    ERR                 err     = JET_errSuccess;
    ERR                 errSlab = JET_errSuccess;
    CCachedBlockSlab*   pcbs    = NULL;

    *ppcbs = NULL;

    errSlab = CCachedBlockSlab::ErrLoad(    pff,
                                            ibSlab, 
                                            cbSlab, 
                                            pcbwcm, 
                                            icbwcBase, 
                                            clnoMin, 
                                            clnoMax, 
                                            fIgnoreVerificationErrors,
                                            &pcbs );
    Call( fIgnoreVerificationErrors ? ErrIgnoreVerificationErrors( errSlab ) : errSlab );

    *ppcbs = pcbs;
    pcbs = NULL;

    err = errSlab;

HandleError:
    delete pcbs;
    if ( ( fIgnoreVerificationErrors ? ErrIgnoreVerificationErrors( err ) : err ) < JET_errSuccess )
    {
        delete *ppcbs;
        *ppcbs = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrLoadCachedBlockSlabManager( _In_    IFileFilter* const                      pff,
                                                                    _In_    const QWORD                             cbCachingFilePerSlab,
                                                                    _In_    const QWORD                             cbCachedFilePerSlab,
                                                                    _In_    const QWORD                             ibChunk,
                                                                    _In_    const QWORD                             cbChunk,
                                                                    _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                                                                    _In_    const QWORD                             icbwcBase,
                                                                    _In_    const ClusterNumber                     clnoMin,
                                                                    _In_    const ClusterNumber                     clnoMax,
                                                                    _Out_   ICachedBlockSlabManager** const         ppcbsm )
{
    ERR                         err     = JET_errSuccess;
    ICachedBlockSlabManager*    pcbsm   = NULL;

    *ppcbsm = NULL;

    Call( CCachedBlockSlabManager::ErrInit( pff,
                                            cbCachingFilePerSlab, 
                                            cbCachedFilePerSlab, 
                                            ibChunk, 
                                            cbChunk, 
                                            pcbwcm, 
                                            icbwcBase, 
                                            clnoMin, 
                                            clnoMax,
                                            &pcbsm ) );

    *ppcbsm = pcbsm;
    pcbsm = NULL;

HandleError:
    delete pcbsm;
    if ( err < JET_errSuccess )
    {
        delete *ppcbsm;
        *ppcbsm = NULL;
    }
    return err;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateCachedBlock(  _In_    const CCachedBlockId    cbid,
                                                            _In_    const ClusterNumber     clno,
                                                            _In_    const BOOL              fValid,
                                                            _In_    const BOOL              fPinned,
                                                            _In_    const BOOL              fDirty,
                                                            _In_    const BOOL              fEverDirty,
                                                            _In_    const BOOL              fPurged,
                                                            _In_    const UpdateNumber      updno,
                                                            _Out_   CCachedBlock*           pcbl )
{
    new( pcbl ) CCachedBlock( cbid, clno, 0, (TouchNumber)1, (TouchNumber)1, fValid, fPinned, fDirty, fEverDirty, fPurged, updno );

    return JET_errSuccess;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateCachedBlockSlot(  _In_    const QWORD         ibSlab,
                                                                _In_    const ChunkNumber   chno,
                                                                _In_    const SlotNumber    slno,
                                                                _In_    const CCachedBlock& cbl,
                                                                _Out_   CCachedBlockSlot*   pslot )
{
    new( pslot ) CCachedBlockSlot( ibSlab, chno, slno, cbl );

    return JET_errSuccess;
}

INLINE ERR COSBlockCacheFactoryImpl::ErrCreateCachedBlockSlotState( _In_    const CCachedBlockSlot& slot,
                                                                    _In_    const BOOL              fSlabUpdated,
                                                                    _In_    const BOOL              fChunkUpdated,
                                                                    _In_    const BOOL              fSlotUpdated,
                                                                    _In_    const BOOL              fClusterUpdated,
                                                                    _In_    const BOOL              fSuperceded,
                                                                    _Out_   CCachedBlockSlotState*  pslotst )
{
    new( pslotst ) CCachedBlockSlotState( slot, fSlabUpdated, fChunkUpdated, fSlotUpdated, fClusterUpdated, fSuperceded );

    return JET_errSuccess;
}
