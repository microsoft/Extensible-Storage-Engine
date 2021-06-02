// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

//  Block Cache Configuration

CDefaultCachedFileConfiguration::CDefaultCachedFileConfiguration()
    :   m_fCachingEnabled( fFalse ),
        m_wszAbsPathCachingFile( L"" ),
        m_cbBlockSize( 4096 ),
        m_cConcurrentBlockWriteBackMax( 100 ),
        m_lCacheTelemetryFileNumber( dwMax )
{
}

BOOL CDefaultCachedFileConfiguration::FCachingEnabled()
{
    return m_fCachingEnabled;
}

VOID CDefaultCachedFileConfiguration::CachingFilePath( __out_bcount( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsPath )
{
    OSStrCbCopyW( wszAbsPath, cbOSFSAPI_MAX_PATHW, m_wszAbsPathCachingFile );
}

ULONG CDefaultCachedFileConfiguration::CbBlockSize()
{
    return m_cbBlockSize;
}

ULONG CDefaultCachedFileConfiguration::CConcurrentBlockWriteBackMax()
{
    return m_cConcurrentBlockWriteBackMax;
}

ULONG CDefaultCachedFileConfiguration::LCacheTelemetryFileNumber()
{
    return m_lCacheTelemetryFileNumber;
}

CDefaultCacheConfiguration::CDefaultCacheConfiguration()
    :   m_fCacheEnabled( fFalse ),
        m_wszAbsPathCachingFile( L"" ),
        m_cbMaximumSize( 16 * 1024 * 1024 ),
        m_pctWrite( 100 )
{
    memcpy( m_rgbCacheType, CPassThroughCache::RgbCacheType(), cbGuid );
}

BOOL CDefaultCacheConfiguration::FCacheEnabled()
{
    return m_fCacheEnabled;
}

VOID CDefaultCacheConfiguration::CacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType )
{
    memcpy( rgbCacheType, m_rgbCacheType, cbGuid );
}

VOID CDefaultCacheConfiguration::Path( __out_bcount( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsPath )
{
    OSStrCbCopyW( wszAbsPath, cbOSFSAPI_MAX_PATHW, m_wszAbsPathCachingFile );
}

QWORD CDefaultCacheConfiguration::CbMaximumSize()
{
    return m_cbMaximumSize;
}

double CDefaultCacheConfiguration::PctWrite()
{
    return m_pctWrite;
}

CDefaultBlockCacheConfiguration::CDefaultBlockCacheConfiguration()
{
}

ERR CDefaultBlockCacheConfiguration::ErrGetCachedFileConfiguration( _In_z_  const WCHAR* const                  wszKeyPathCachedFile,
                                                                    _Out_   ICachedFileConfiguration** const    ppcfconfig  )
{
    ERR err = JET_errSuccess;

    Alloc( *ppcfconfig = new CDefaultCachedFileConfiguration() );

HandleError:
    return err;
}

ERR CDefaultBlockCacheConfiguration::ErrGetCacheConfiguration(  _In_z_  const WCHAR* const          wszKeyPathCachingFile,
                                                                _Out_   ICacheConfiguration** const ppcconfig )
{
    ERR err = JET_errSuccess;

    Alloc( *ppcconfig = new CDefaultCacheConfiguration() );

HandleError:
    return err;
}

//  Factory methods

ERR ErrOSBCCreateFileSystemWrapper( _Inout_ IFileSystemAPI** const  ppfsapiInner,
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

ERR ErrOSBCCreateFileSystemFilter(  _In_    IFileSystemConfiguration* const pfsconfig,
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

ERR ErrOSBCCreateFileWrapper(   _Inout_ IFileAPI** const    ppfapiInner,
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

ERR ErrOSBCCreateFileFilterWrapper( _Inout_ IFileFilter** const         ppffInner,
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

ERR ErrOSBCCreateFileIdentification( _Out_ IFileIdentification** const ppfident )
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

ERR ErrOSBCCreateCache( _In_    IFileSystemFilter* const        pfsf,
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

ERR ErrOSBCCreateCacheWrapper(  _Inout_ ICache** const  ppcInner,
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

ERR ErrOSBCCreateCacheRepository(   _In_    IFileIdentification* const      pfident,
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

ERR ErrOSBCCreateCacheTelemetry( _Out_ ICacheTelemetry** const ppctm )
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

ERR ErrOSBCDumpCachedFileHeader(    _In_z_  const WCHAR* const  wszFilePath,
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

ERR ErrOSBCDumpCacheFile(   _In_z_  const WCHAR* const  wszFilePath, 
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

    Call( CCacheHeader::ErrDump( &fsconfig, &g_fident, pff, CPRINTFSTDOUT::PcprintfInstance() ) );

    Call( CCacheFactory::ErrDump( pfsf, &g_fident, &fsconfig, &pcconfig, &g_ctm, &pff, CPRINTFSTDOUT::PcprintfInstance() ) );

HandleError:
    delete pff;
    delete pfsf;
    delete pcconfig;
    return err;
}

ERR ErrOSBCCreateJournalSegment(    _In_    IFileFilter* const      pff,
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

ERR ErrOSBCLoadJournalSegment(  _In_    IFileFilter* const      pff,
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

ERR ErrOSBCCreateJournalSegmentManager( _In_    IFileFilter* const              pff,
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

ERR ErrOSBCCreateJournal(   _Inout_ IJournalSegmentManager** const  ppjsm,
                            _In_    const size_t                    cbCache,
                            _Out_   IJournal** const                ppj )
{
    ERR         err = JET_errSuccess;
    IJournal*   pj  = NULL;

    *ppj = NULL;

    Call( CJournal::ErrMount( ppjsm, cbCache, &pj ) );

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
