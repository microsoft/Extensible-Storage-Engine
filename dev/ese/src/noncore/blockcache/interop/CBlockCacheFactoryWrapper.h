// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                template< class TM, class TN >
                class CBlockCacheFactoryWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CBlockCacheFactoryWrapper( TM^ bcf ) : CWrapper( bcf ) { }

                    public:  //  IBlockCacheFactory

                        ERR ErrCreateFileSystemWrapper( _Inout_ IFileSystemAPI** const  ppfsapiInner,
                                                        _Out_   IFileSystemAPI** const  ppfsapi ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateFileSystemFilter(  _In_    ::IFileSystemConfiguration* const   pfsconfig,
                                                        _Inout_ IFileSystemAPI** const              ppfsapiInner,
                                                        _In_    ::IFileIdentification* const        pfident,
                                                        _In_    ::ICacheTelemetry* const            pctm,
                                                        _In_    ::ICacheRepository* const           pcrep,
                                                        _Out_   ::IFileSystemFilter** const         ppfsf ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateFileWrapper(   _Inout_ IFileAPI** const    ppfapiInner,
                                                    _Out_   IFileAPI** const    ppfapi ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateFileFilter(    _Inout_                     IFileAPI** const                    ppfapiInner,
                                                    _In_                        ::IFileSystemFilter* const          pfsf,
                                                    _In_                        ::IFileSystemConfiguration* const   pfsconfig,
                                                    _In_                        ::IFileIdentification* const        pfident,
                                                    _In_                        ::ICacheTelemetry* const            pctm,
                                                    _In_                        ::ICacheRepository* const           pcrep,
                                                    _In_                        const ::VolumeId                    volumeid,
                                                    _In_                        const ::FileId                      fileid,
                                                    _Inout_                     ::ICachedFileConfiguration** const  ppcfconfig,
                                                    _Inout_                     ::ICache** const                    ppc,
                                                    _In_reads_opt_( cbHeader )  const BYTE* const                   pbHeader,
                                                    _In_                        const int                           cbHeader,
                                                    _Out_                       ::IFileFilter** const               ppff ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateFileFilterWrapper( _Inout_ ::IFileFilter** const       ppffInner,
                                                        _In_    const ::IFileFilter::IOMode iom,
                                                        _Out_   ::IFileFilter** const       ppff ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateFileIdentification( _Out_ ::IFileIdentification** const ppfident ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateCache( _In_    ::IFileSystemFilter* const          pfsf,
                                            _In_    ::IFileIdentification* const        pfident,
                                            _In_    ::IFileSystemConfiguration* const   pfsconfig,
                                            _Inout_ ::ICacheConfiguration** const       ppcconfig,
                                            _In_    ::ICacheTelemetry* const            pctm,
                                            _Inout_ ::IFileFilter** const               ppffCaching,
                                            _Out_   ::ICache** const                    ppc ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateCacheWrapper(  _Inout_ ::ICache** const    ppcInner,
                                                    _Out_   ::ICache** const    ppc ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateCacheRepository(   _In_    ::IFileIdentification* const    pfident,
                                                        _In_    ::ICacheTelemetry* const        pctm,
                                                        _Out_   ::ICacheRepository** const      ppcrep ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateCacheTelemetry( _Out_ ::ICacheTelemetry** const ppctm ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrDumpCachedFileHeader(    _In_z_  const WCHAR* const  wszFilePath,
                                                        _In_    const ULONG         grbit,
                                                        _In_    CPRINTF* const      pcprintf ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrDumpCacheFile(   _In_z_  const WCHAR* const  wszFilePath, 
                                                _In_    const ULONG         grbit,
                                                _In_    CPRINTF* const      pcprintf ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateJournalSegment(    _In_    ::IFileFilter* const        pff,
                                                        _In_    const QWORD                 ib,
                                                        _In_    const ::SegmentPosition     spos,
                                                        _In_    const DWORD                 dwUniqueIdPrev,
                                                        _In_    const ::SegmentPosition     sposReplay,
                                                        _In_    const ::SegmentPosition     sposDurable,
                                                        _Out_   ::IJournalSegment** const   ppjs ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrLoadJournalSegment(  _In_    ::IFileFilter* const        pff,
                                                    _In_    const QWORD                 ib,
                                                    _Out_   ::IJournalSegment** const   ppjs ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateJournalSegmentManager( _In_    ::IFileFilter* const                pff,
                                                            _In_    const QWORD                         ib,
                                                            _In_    const QWORD                         cb,
                                                            _Out_   ::IJournalSegmentManager** const    ppjsm ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateJournal(   _Inout_ ::IJournalSegmentManager** const    ppjsm,
                                                _In_    const size_t                        cbCache,
                                                _Out_   ::IJournal** const                  ppj ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrLoadCachedBlockWriteCountsManager(   _In_    ::IFileFilter* const                        pff,
                                                                    _In_    const QWORD                                 ib,
                                                                    _In_    const QWORD                                 cb,
                                                                    _In_    const QWORD                                 ccbwcs,
                                                                    _Out_   ::ICachedBlockWriteCountsManager** const    ppcbwcm ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrLoadCachedBlockSlab( _In_    ::IFileFilter* const                    pff,
                                                    _In_    const QWORD                             ibSlab,
                                                    _In_    const QWORD                             cbSlab,
                                                    _In_    ::ICachedBlockWriteCountsManager* const pcbwcm,
                                                    _In_    const QWORD                             icbwcBase,
                                                    _In_    const ::ClusterNumber                   clnoMin,
                                                    _In_    const ::ClusterNumber                   clnoMax,
                                                    _In_    const BOOL                              fIgnoreVerificationErrors,
                                                    _Inout_ ::ICachedBlockSlab** const              ppcbs ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateCachedBlockSlabWrapper(    _Inout_ ::ICachedBlockSlab** const  ppcbsInner,
                                                                _Out_   ::ICachedBlockSlab** const  ppcbs )
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrLoadCachedBlockSlabManager(  _In_    ::IFileFilter* const                    pff,
                                                            _In_    const QWORD                             cbCachingFilePerSlab,
                                                            _In_    const QWORD                             cbCachedFilePerSlab,
                                                            _In_    const QWORD                             ibChunk,
                                                            _In_    const QWORD                             cbChunk,
                                                            _In_    ::ICachedBlockWriteCountsManager* const pcbwcm,
                                                            _In_    const QWORD                             icbwcBase,
                                                            _In_    const ::ClusterNumber                   clnoMin,
                                                            _In_    const ::ClusterNumber                   clnoMax,
                                                            _Out_   ::ICachedBlockSlabManager** const       ppcbsm ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateCachedBlock(   _In_    const ::CCachedBlockId  cbid,
                                                    _In_    const ::ClusterNumber   clno,
                                                    _In_    const BOOL              fValid,
                                                    _In_    const BOOL              fPinned,
                                                    _In_    const BOOL              fDirty,
                                                    _In_    const BOOL              fEverDirty,
                                                    _In_    const BOOL              fPurged,
                                                    _In_    const ::UpdateNumber    updno,
                                                    _Out_   ::CCachedBlock*         pcbl ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateCachedBlockSlot(   _In_    const QWORD             ibSlab,
                                                        _In_    const ::ChunkNumber     chno,
                                                        _In_    const ::SlotNumber      slno,
                                                        _In_    const ::CCachedBlock&   cbl,
                                                        _Out_   ::CCachedBlockSlot*     pslot ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrCreateCachedBlockSlotState(  _In_    const ::CCachedBlockSlot&   slot,
                                                            _In_    const BOOL                  fSlabUpdated,
                                                            _In_    const BOOL                  fChunkUpdated,
                                                            _In_    const BOOL                  fSlotUpdated,
                                                            _In_    const BOOL                  fClusterUpdated,
                                                            _In_    const BOOL                  fSuperceded,
                                                            _Out_   ::CCachedBlockSlotState*    pslotst ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }

                        ERR ErrDetachFile(  _In_z_      const WCHAR* const                          wszFilePath,
                                            _In_opt_    ::IBlockCacheFactory::PfnDetachFileStatus   pfnDetachFileStatus,
                                            _In_opt_    const DWORD_PTR                             keyDetachFileStatus ) override
                        {
                            return ErrERRCheck( JET_wrnNyi );
                        }
                };
            }
        }
    }
}
