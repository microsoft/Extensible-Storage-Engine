// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  TBlockCacheFactoryWrapper:  wrapper of an implementation of IBlockCacheFactory or its derivatives.

template< class I >
class TBlockCacheFactoryWrapper
    :   public I
{
    public:  //  specialized API

        TBlockCacheFactoryWrapper( _In_ I* const piInner )
            :   m_piInner( piInner ),
                m_fReleaseOnClose( fFalse )
        {
        }

        TBlockCacheFactoryWrapper( _Inout_ I** const ppiInner )
            :   m_piInner( *ppiInner ),
                m_fReleaseOnClose( fTrue )
        {
            *ppiInner = NULL;
        }

        ~TBlockCacheFactoryWrapper()
        {
            if ( m_fReleaseOnClose )
            {
                delete m_piInner;
            }
        }

    public:  //  IBlockCacheFactory

        ERR ErrCreateFileSystemWrapper( _Inout_ IFileSystemAPI** const  ppfsapiInner,
                                        _Out_   IFileSystemAPI** const  ppfsapi ) override
        {
            return m_piInner->ErrCreateFileSystemWrapper( ppfsapiInner, ppfsapi );
        }

        ERR ErrCreateFileSystemFilter(  _In_    IFileSystemConfiguration* const pfsconfig,
                                        _Inout_ IFileSystemAPI** const          ppfsapiInner,
                                        _In_    IFileIdentification* const      pfident,
                                        _In_    ICacheTelemetry* const          pctm,
                                        _In_    ICacheRepository* const         pcrep,
                                        _Out_   IFileSystemFilter** const       ppfsf ) override
        {
            return m_piInner->ErrCreateFileSystemFilter( pfsconfig, ppfsapiInner, pfident, pctm, pcrep, ppfsf );
        }

        ERR ErrCreateFileWrapper(   _Inout_ IFileAPI** const    ppfapiInner,
                                    _Out_   IFileAPI** const    ppfapi ) override
        {
            return m_piInner->ErrCreateFileWrapper( ppfapiInner, ppfapi );
        }

        ERR ErrCreateFileFilter(    _Inout_                     IFileAPI** const                    ppfapiInner,
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
                                    _Out_                       IFileFilter** const                 ppff ) override
        {
            return m_piInner->ErrCreateFileFilter( ppfapiInner, pfsf, pfsconfig, pfident, pctm, pcrep, volumeid, fileid, ppcfconfig, ppc, pbHeader, cbHeader, ppff );
        }

        ERR ErrCreateFileFilterWrapper( _Inout_ IFileFilter** const         ppffInner,
                                        _In_    const IFileFilter::IOMode   iom,
                                        _Out_   IFileFilter** const         ppff ) override
        {
            return m_piInner->ErrCreateFileFilterWrapper( ppffInner, iom, ppff );
        }

        ERR ErrCreateFileIdentification( _Out_ IFileIdentification** const ppfident ) override
        {
            return m_piInner->ErrCreateFileIdentification( ppfident );
        }

        ERR ErrCreateCache( _In_    IFileSystemFilter* const        pfsf,
                            _In_    IFileIdentification* const      pfident,
                            _In_    IFileSystemConfiguration* const pfsconfig,
                            _Inout_ ICacheConfiguration** const     ppcconfig,
                            _In_    ICacheTelemetry* const          pctm,
                            _Inout_ IFileFilter** const             ppffCaching,
                            _Out_   ICache** const                  ppc ) override
        {
            return m_piInner->ErrCreateCache( pfsf, pfident, pfsconfig, ppcconfig, pctm, ppffCaching, ppc );
        }

        ERR ErrMountCache(  _In_    IFileSystemFilter* const        pfsf,
                            _In_    IFileIdentification* const      pfident,
                            _In_    IFileSystemConfiguration* const pfsconfig,
                            _Inout_ ICacheConfiguration** const     ppcconfig,
                            _In_    ICacheTelemetry* const          pctm,
                            _Inout_ IFileFilter** const             ppffCaching,
                            _Out_   ICache** const                  ppc ) override
        {
            return m_piInner->ErrMountCache( pfsf, pfident, pfsconfig, ppcconfig, pctm, ppffCaching, ppc );
        }

        ERR ErrCreateCacheWrapper(  _Inout_ ICache** const  ppcInner,
                                    _Out_   ICache** const  ppc ) override
        {
            return m_piInner->ErrCreateCacheWrapper( ppcInner, ppc );
        }

        ERR ErrCreateCacheRepository(   _In_    IFileIdentification* const      pfident,
                                        _In_    ICacheTelemetry* const          pctm,
                                        _Out_   ICacheRepository** const        ppcrep ) override
        {
            return m_piInner->ErrCreateCacheRepository( pfident, pctm, ppcrep );
        }

        ERR ErrCreateCacheTelemetry( _Out_ ICacheTelemetry** const ppctm ) override
        {
            return m_piInner->ErrCreateCacheTelemetry( ppctm );
        }

        ERR ErrDumpCachedFileHeader(    _In_z_  const WCHAR* const  wszFilePath,
                                        _In_    const ULONG         grbit,
                                        _In_    CPRINTF* const      pcprintf ) override
        {
            return m_piInner->ErrDumpCachedFileHeader( wszFilePath, grbit, pcprintf );
        }

        ERR ErrDumpCacheFile(   _In_z_  const WCHAR* const  wszFilePath, 
                                _In_    const ULONG         grbit,
                                _In_    CPRINTF* const      pcprintf ) override
        {
            return m_piInner->ErrDumpCacheFile( wszFilePath, grbit, pcprintf );
        }

        ERR ErrCreateJournalSegment(    _In_    IFileFilter* const      pff,
                                        _In_    const QWORD             ib,
                                        _In_    const SegmentPosition   spos,
                                        _In_    const DWORD             dwUniqueIdPrev,
                                        _In_    const SegmentPosition   sposReplay,
                                        _In_    const SegmentPosition   sposDurable,
                                        _Out_   IJournalSegment** const ppjs ) override
        {
            return m_piInner->ErrCreateJournalSegment( pff, ib, spos, dwUniqueIdPrev, sposReplay, sposDurable, ppjs );
        }

        ERR ErrLoadJournalSegment(  _In_    IFileFilter* const      pff,
                                    _In_    const QWORD             ib,
                                    _Out_   IJournalSegment** const ppjs ) override
        {
            return m_piInner->ErrLoadJournalSegment( pff, ib, ppjs );
        }

        ERR ErrCreateJournalSegmentManager( _In_    IFileFilter* const              pff,
                                            _In_    const QWORD                     ib,
                                            _In_    const QWORD                     cb,
                                            _Out_   IJournalSegmentManager** const  ppjsm ) override
        {
            return m_piInner->ErrCreateJournalSegmentManager( pff, ib, cb, ppjsm );
        }

        ERR ErrCreateJournal(   _Inout_ IJournalSegmentManager** const  ppjsm,
                                _In_    const size_t                    cbCache,
                                _Out_   IJournal** const                ppj ) override
        {
            return m_piInner->ErrCreateJournal( ppjsm, cbCache, ppj );
        }

        ERR ErrLoadCachedBlockWriteCountsManager(   _In_    IFileFilter* const                      pff,
                                                    _In_    const QWORD                             ib,
                                                    _In_    const QWORD                             cb,
                                                    _In_    const QWORD                             ccbwcs,
                                                    _Out_   ICachedBlockWriteCountsManager** const  ppcbwcm ) override
        {
            return m_piInner->ErrLoadCachedBlockWriteCountsManager( pff, ib, cb, ccbwcs, ppcbwcm );
        }

        ERR ErrLoadCachedBlockSlab( _In_    IFileFilter* const                      pff,
                                    _In_    const QWORD                             ibSlab,
                                    _In_    const QWORD                             cbSlab,
                                    _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                                    _In_    const QWORD                             icbwcBase,
                                    _In_    const ClusterNumber                     clnoMin,
                                    _In_    const ClusterNumber                     clnoMax,
                                    _In_    const BOOL                              fIgnoreVerificationErrors,
                                    _Inout_ ICachedBlockSlab** const                ppcbs ) override
        {
            return m_piInner->ErrLoadCachedBlockSlab( pff, ibSlab, cbSlab, pcbwcm, icbwcBase, clnoMin, clnoMax, fIgnoreVerificationErrors, ppcbs );
        }

        ERR ErrCreateCachedBlockSlabWrapper(    _Inout_ ICachedBlockSlab** const    ppcbsInner,
                                                _Out_   ICachedBlockSlab** const    ppcbs ) override
        {
            return m_piInner->ErrCreateCachedBlockSlabWrapper( ppcbsInner, ppcbs );
        }

        ERR ErrLoadCachedBlockSlabManager(  _In_    IFileFilter* const                      pff,
                                            _In_    const QWORD                             cbCachingFilePerSlab,
                                            _In_    const QWORD                             cbCachedFilePerSlab,
                                            _In_    const QWORD                             ibChunk,
                                            _In_    const QWORD                             cbChunk,
                                            _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                                            _In_    const QWORD                             icbwcBase,
                                            _In_    const ClusterNumber                     clnoMin,
                                            _In_    const ClusterNumber                     clnoMax,
                                            _Out_   ICachedBlockSlabManager** const         ppcbsm ) override
        {
            return m_piInner->ErrLoadCachedBlockSlabManager( pff, cbCachingFilePerSlab, cbCachedFilePerSlab, ibChunk, cbChunk, pcbwcm, icbwcBase, clnoMin, clnoMax, ppcbsm );
        }

        ERR ErrCreateCachedBlock(   _In_    const CCachedBlockId    cbid,
                                    _In_    const ClusterNumber     clno,
                                    _In_    const BOOL              fValid,
                                    _In_    const BOOL              fPinned,
                                    _In_    const BOOL              fDirty,
                                    _In_    const BOOL              fEverDirty,
                                    _In_    const BOOL              fPurged,
                                    _In_    const UpdateNumber      updno,
                                    _Out_   CCachedBlock*           pcbl ) override
        {
            return m_piInner->ErrCreateCachedBlock( cbid, clno, fValid, fPinned, fDirty, fEverDirty, fPurged, updno, pcbl );
        }

        ERR ErrCreateCachedBlockSlot(   _In_    const QWORD         ibSlab,
                                        _In_    const ChunkNumber   chno,
                                        _In_    const SlotNumber    slno,
                                        _In_    const CCachedBlock& cbl,
                                        _Out_   CCachedBlockSlot*   pslot ) override
        {
            return m_piInner->ErrCreateCachedBlockSlot( ibSlab, chno, slno, cbl, pslot );
        }

        ERR ErrCreateCachedBlockSlotState(  _In_    const CCachedBlockSlot& slot,
                                            _In_    const BOOL              fSlabUpdated,
                                            _In_    const BOOL              fChunkUpdated,
                                            _In_    const BOOL              fSlotUpdated,
                                            _In_    const BOOL              fClusterUpdated,
                                            _In_    const BOOL              fSuperceded,
                                            _Out_   CCachedBlockSlotState*  pslotst ) override
        {
            return m_piInner->ErrCreateCachedBlockSlotState( slot, fSlabUpdated, fChunkUpdated, fSlotUpdated, fClusterUpdated, fSuperceded, pslotst );
        }

        ERR ErrDetachFile(  _In_z_      const WCHAR* const                              wszFilePath,
                            _In_opt_    const IBlockCacheFactory::PfnDetachFileStatus   pfnDetachFileStatus,
                            _In_opt_    const DWORD_PTR                                 keyDetachFileStatus ) override
        {
            return m_piInner->ErrDetachFile( wszFilePath, pfnDetachFileStatus, keyDetachFileStatus );
        }

    private:

        I* const   m_piInner;
        const BOOL m_fReleaseOnClose;
};

//  CBlockCacheFactoryWrapper:  concrete TBlockCacheFactoryWrapper<IBlockCacheFactory>.

class CBlockCacheFactoryWrapper : public TBlockCacheFactoryWrapper<IBlockCacheFactory>
{
    public:  //  specialized API

        CBlockCacheFactoryWrapper( _In_ IBlockCacheFactory* const pbcf )
            :   TBlockCacheFactoryWrapper<IBlockCacheFactory>( pbcf )
        {
        }

        CBlockCacheFactoryWrapper( _Inout_ IBlockCacheFactory** const ppbcf )
            :   TBlockCacheFactoryWrapper<IBlockCacheFactory>( ppbcf )
        {
        }

        virtual ~CBlockCacheFactoryWrapper() {}
};
