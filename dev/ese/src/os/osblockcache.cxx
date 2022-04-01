// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

//  Block Cache Configuration

CDefaultCachedFileConfiguration::CDefaultCachedFileConfiguration()
    :   m_fCachingEnabled( fFalse ),
        m_wszAbsPathCachingFile( L"" ),
        m_cbBlockSize( 4096 ),
        m_cConcurrentBlockWriteBackMax( 100 ),
        m_ulCacheTelemetryFileNumber( dwMax ),
        m_ulPinnedHeaderSizeInBytes( 4096 )
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

ULONG CDefaultCachedFileConfiguration::UlCacheTelemetryFileNumber()
{
    return m_ulCacheTelemetryFileNumber;
}

ULONG CDefaultCachedFileConfiguration::UlPinnedHeaderSizeInBytes()
{
    return m_ulPinnedHeaderSizeInBytes;
}

CDefaultCacheConfiguration::CDefaultCacheConfiguration()
    :   m_fCacheEnabled( fFalse ),
        m_wszAbsPathCachingFile( L"" ),
        m_cbMaximumSize( 16 * 1024 * 1024 ),
        m_pctWrite( 100 ),
        m_cbJournalSegmentsMaximumSize( 2 * 1024 * 1024 ),
        m_pctJournalSegmentsInUse( 75 ),
        m_cbJournalSegmentsMaximumCacheSize( 1 * 1024 * 1024 ),
        m_cbJournalClustersMaximumSize( 2 * 1024 * 1024 ),
        m_cbCachingFilePerSlab( 1 * 102 * 4096 ),
        m_cbCachedFilePerSlab( 0 ),
        m_cbSlabMaximumCacheSize( 1 * 1024 * 1024 ),
        m_fAsyncWriteBackEnabled( fTrue )
{
    memcpy( m_rgbCacheType, CHashedLRUKCache::RgbCacheType(), cbGuid );
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

QWORD CDefaultCacheConfiguration::CbJournalSegmentsMaximumSize()
{
    return m_cbJournalSegmentsMaximumSize;
}

double CDefaultCacheConfiguration::PctJournalSegmentsInUse()
{
    return m_pctJournalSegmentsInUse;
}

QWORD CDefaultCacheConfiguration::CbJournalSegmentsMaximumCacheSize()
{
    return m_cbJournalSegmentsMaximumCacheSize;
}

QWORD CDefaultCacheConfiguration::CbJournalClustersMaximumSize()
{
    return m_cbJournalClustersMaximumSize;
}

QWORD CDefaultCacheConfiguration::CbCachingFilePerSlab()
{
    return m_cbCachingFilePerSlab;
}

QWORD CDefaultCacheConfiguration::CbCachedFilePerSlab()
{
    return m_cbCachedFilePerSlab;
}

QWORD CDefaultCacheConfiguration::CbSlabMaximumCacheSize()
{
    return m_cbSlabMaximumCacheSize;
}

BOOL CDefaultCacheConfiguration::FAsyncWriteBackEnabled()
{
    return m_fAsyncWriteBackEnabled;
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


//  Cached Block Slot

void CCachedBlockSlot::Pin( _In_    const CCachedBlockSlot& slot,
                            _Out_   CCachedBlockSlot* const pslot )
{
    new ( pslot ) CCachedBlockSlot( slot.IbSlab(),
                                    slot.Chno(),
                                    slot.Slno(),
                                    CCachedBlock(   slot.Cbid(),
                                                    slot.Clno(),
                                                    slot.DwECC(),
                                                    slot.Tono0(),
                                                    slot.Tono1(),
                                                    slot.FValid(),
                                                    fTrue,
                                                    slot.FDirty(),
                                                    slot.FEverDirty(),
                                                    slot.FPurged(),
                                                    slot.Updno() ) );
}

void CCachedBlockSlot::SwapClusters(    _In_    const CCachedBlockSlot& slotA,
                                        _In_    const CCachedBlockSlot& slotB,
                                        _Out_   CCachedBlockSlot* const pslotA,
                                        _Out_   CCachedBlockSlot* const pslotB )
{
    new ( pslotA ) CCachedBlockSlot(    slotA.IbSlab(),
                                        slotA.Chno(),
                                        slotA.Slno(),
                                        CCachedBlock(   slotA.Cbid(),
                                                        slotB.Clno(),  //  swap
                                                        slotA.DwECC(),
                                                        slotA.Tono0(),
                                                        slotA.Tono1(),
                                                        slotA.FValid(),
                                                        slotA.FPinned(),
                                                        slotA.FDirty(),
                                                        slotA.FEverDirty(),
                                                        slotA.FPurged(),
                                                        slotA.Updno() ) );
    new ( pslotB ) CCachedBlockSlot(    slotB.IbSlab(),
                                        slotB.Chno(),
                                        slotB.Slno(),
                                        CCachedBlock(   slotB.Cbid(),
                                                        slotA.Clno(),  //  swap
                                                        slotB.DwECC(),
                                                        slotB.Tono0(),
                                                        slotB.Tono1(),
                                                        slotB.FValid(),
                                                        slotB.FPinned(),
                                                        slotB.FDirty(),
                                                        slotB.FEverDirty(),
                                                        slotB.FPurged(),
                                                        slotB.Updno() ) );
}

const char* OSFormat( _In_ const CCachedBlockSlot& slot )
{
    return OSFormat(    "0x%016I64x,0x%01x,0x%02x %s,0x%08x 0x%08x (0x%08x) %c%c%c%c%c v%d",
                        slot.IbSlab(),
                        slot.Chno(),
                        slot.Slno(),
                        OSFormat(   slot.Cbid().Volumeid(),
                                    slot.Cbid().Fileid(),
                                    slot.Cbid().Fileserial() ),
                        slot.Cbid().Cbno(),
                        slot.Clno(),
                        slot.DwECC(),
                        slot.FValid() ? 'V' : '_',
                        slot.FPinned() ? 'P' : '_',
                        slot.FDirty() ? 'D' : '_',
                        slot.FEverDirty() ? 'E' : '_',
                        slot.FPurged() ? 'X' : '_',
                        slot.Updno() );
}

void CCachedBlockSlot::Dump(    _In_ const CCachedBlockSlot&    slot,
                                _In_ CPRINTF* const             pcprintf,
                                _In_ IFileIdentification* const pfident )
{
    OSTraceSuspendGC();
    (*pcprintf)( OSFormat( slot ) );
    OSTraceResumeGC();

    if ( slot.Cbid().Cbno() != cbnoInvalid )
    {
        WCHAR   wszAnyAbsPath[ IFileSystemAPI::cchPathMax ]         = { };
        WCHAR   wszKeyPath[ IFileIdentification::cwchKeyPathMax ]   = { };

        (void)pfident->ErrGetFilePathById(  slot.Cbid().Volumeid(),
                                            slot.Cbid().Fileid(),
                                            wszAnyAbsPath,
                                            wszKeyPath );

        (*pcprintf)(    _T( "  // file %ws at offset 0x%016I64x for 0x%08x bytes" ),
                        wszAnyAbsPath[0] ? wszAnyAbsPath : L"<unknown>",
                        (QWORD)slot.Cbid().Cbno() * cbCachedBlock,
                        cbCachedBlock );
    }
}

//  Block Cache Factory

COSBlockCacheFactoryImpl g_bcf;

ERR COSBlockCacheFactory::ErrCreate( _Out_ IBlockCacheFactory** const ppbcf )
{
    ERR err = JET_errSuccess;
    IBlockCacheFactory* pbcf = NULL;

    *ppbcf = NULL;

    Alloc( pbcf = new CBlockCacheFactoryWrapper( &g_bcf ) );

    *ppbcf = pbcf;
    pbcf = NULL;

HandleError:
    delete pbcf;
    if ( err < JET_errSuccess )
    {
        delete *ppbcf;
        *ppbcf = NULL;
    }
    return err;
}


//  Init / Term

BOOL FOSBlockCachePreinit()
{
    return fTrue;
}

void OSBlockCachePostterm()
{
    CFileWrapper::Cleanup();
    CFileFilter::Cleanup();
}