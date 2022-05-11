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
                template< class TM, class TN, class TW >
                public ref class BlockCacheFactoryBase : Base<TM, TN, TW>, IBlockCacheFactory
                {
                    public:

                        BlockCacheFactoryBase( TM^ bcf ) : Base( bcf ) { }

                        BlockCacheFactoryBase( TN** const ppbcf ) : Base( ppbcf ) {}

                        BlockCacheFactoryBase( TN* const pbcf ) : Base( pbcf ) {}

                    public:

                        virtual FileSystem^ CreateFileSystemWrapper( IFileSystem^ ifsInner )
                        {
                            ERR             err         = JET_errSuccess;
                            IFileSystemAPI* pfsapiInner = NULL;
                            IFileSystemAPI* pfsapi      = NULL;

                            Call( FileSystem::ErrWrap( ifsInner, &pfsapiInner ) );

                            Call( Pi->ErrCreateFileSystemWrapper( &pfsapiInner, &pfsapi ) );

                            return gcnew FileSystem( &pfsapi );

                        HandleError:
                            delete pfsapi;
                            delete pfsapiInner;
                            throw EseException( err );
                        }

                        virtual FileSystemFilter^ CreateFileSystemFilter(
                            FileSystemConfiguration^ fsconfig,
                            IFileSystem^ ifsInner,
                            FileIdentification^ fident,
                            CacheTelemetry^ ctm,
                            CacheRepository^ crep )
                        {
                            ERR                     err         = JET_errSuccess;
                            IFileSystemAPI*         pfsapiInner = NULL;
                            ::IFileSystemFilter*    pfsf        = NULL;

                            Call( FileSystem::ErrWrap( ifsInner, &pfsapiInner ) );

                            Call( Pi->ErrCreateFileSystemFilter(    fsconfig->Pi,
                                                                    &pfsapiInner, 
                                                                    fident->Pi, 
                                                                    ctm->Pi,
                                                                    crep->Pi, 
                                                                    &pfsf ) );

                            return gcnew FileSystemFilter( &pfsf );

                        HandleError:
                            delete pfsapiInner;
                            delete pfsf;
                            throw EseException( err );
                        }

                        virtual File^ CreateFileWrapper( IFile^ ifInner )
                        {
                            ERR         err         = JET_errSuccess;
                            IFileAPI*   pfapiInner  = NULL;
                            IFileAPI*   pfapi       = NULL;

                            Call( File::ErrWrap( ifInner, &pfapiInner ) );

                            Call( Pi->ErrCreateFileWrapper( &pfapiInner, &pfapi ) );

                            return gcnew File( &pfapi );

                        HandleError:
                            delete pfapiInner;
                            delete pfapi;
                            throw EseException( err );
                        }

                        virtual FileFilter^ CreateFileFilter(
                            IFile^ fInner,
                            FileSystemFilter^ fsf,
                            FileSystemConfiguration^ fsconfig,
                            FileIdentification^ fident,
                            CacheTelemetry^ ctm,
                            CacheRepository^ crep,
                            VolumeId volumeid,
                            FileId fileid,
                            ICachedFileConfiguration^ icfconfig,
                            ICache^ ic,
                            ArraySegment<BYTE> header )
                        {
                            ERR                         err         = JET_errSuccess;
                            IFileAPI*                   pfapiInner  = NULL;
                            ::ICachedFileConfiguration* pcfconfig   = NULL;
                            ::ICache*                   pc          = NULL;
                            ::IFileFilter*              pff         = NULL;

                            Call( File::ErrWrap( fInner, &pfapiInner ) );
                            Call( CachedFileConfiguration::ErrWrap( icfconfig, &pcfconfig ) );
                            Call( Cache::ErrWrap( ic, &pc ) );

                            pin_ptr<const BYTE> pbHeader = header.Count == 0 ? nullptr : &header.Array[ header.Offset ];
                            int cbHeader = header.Count;

                            Call( Pi->ErrCreateFileFilter(  &pfapiInner,
                                                            fsf->Pi,
                                                            fsconfig->Pi,
                                                            fident->Pi,
                                                            ctm->Pi,
                                                            crep->Pi,
                                                            (::VolumeId)volumeid,
                                                            (::FileId)fileid,
                                                            &pcfconfig,
                                                            &pc,
                                                            pbHeader, 
                                                            cbHeader,
                                                            &pff ) );

                            return gcnew FileFilter( &pff );

                        HandleError:
                            delete pff;
                            delete pcfconfig;
                            delete pc;
                            delete pfapiInner;
                            throw EseException( err );
                        }

                        virtual FileFilter^ CreateFileFilterWrapper( IFileFilter^ iffInner, IOMode ioMode )
                        {
                            ERR             err         = JET_errSuccess;
                            ::IFileFilter*  pffInner    = NULL;
                            ::IFileFilter*  pff         = NULL;

                            Call( FileFilter::ErrWrap( iffInner, &pffInner ) );

                            Call( Pi->ErrCreateFileFilterWrapper( &pffInner, (::IFileFilter::IOMode)ioMode, &pff ) );

                            return gcnew FileFilter( &pff );

                        HandleError:
                            delete pffInner;
                            delete pff;
                            throw EseException( err );
                        }

                        virtual FileIdentification^ CreateFileIdentification()
                        {
                            ERR                     err     = JET_errSuccess;
                            ::IFileIdentification*  pfident = NULL;

                            Call( Pi->ErrCreateFileIdentification( &pfident ) );

                            return gcnew FileIdentification( &pfident );

                        HandleError:
                            delete pfident;
                            throw EseException( err );
                        }

                        virtual Cache^ CreateCache(
                            FileSystemFilter^ fsf,
                            FileIdentification^ fident,
                            FileSystemConfiguration^ fsconfig,
                            ICacheConfiguration^ cconfig,
                            CacheTelemetry^ ctm,
                            IFileFilter^ iff )
                        {
                            ERR                     err         = JET_errSuccess;
                            ::ICacheConfiguration*  pcconfig    = NULL;
                            ::IFileFilter*          pff         = NULL;
                            ::ICache*               pc          = NULL;

                            Call( CacheConfiguration::ErrWrap( cconfig, &pcconfig ) );
                            Call( FileFilter::ErrWrap( iff, &pff ) );

                            Call( Pi->ErrCreateCache( fsf->Pi, fident->Pi, fsconfig->Pi, &pcconfig, ctm->Pi, &pff, &pc ) );

                            return gcnew Cache( &pc );

                        HandleError:
                            delete pff;
                            delete pcconfig;
                            delete pc;
                            throw EseException( err );
                        }

                        virtual Cache^ MountCache(
                            FileSystemFilter^ fsf,
                            FileIdentification^ fident,
                            FileSystemConfiguration^ fsconfig,
                            ICacheConfiguration^ cconfig,
                            CacheTelemetry^ ctm,
                            IFileFilter^ iff )
                        {
                            ERR                     err         = JET_errSuccess;
                            ::ICacheConfiguration*  pcconfig    = NULL;
                            ::IFileFilter*          pff         = NULL;
                            ::ICache*               pc          = NULL;

                            Call( CacheConfiguration::ErrWrap( cconfig, &pcconfig ) );
                            Call( FileFilter::ErrWrap( iff, &pff ) );

                            Call( Pi->ErrMountCache( fsf->Pi, fident->Pi, fsconfig->Pi, &pcconfig, ctm->Pi, &pff, &pc ) );

                            return gcnew Cache( &pc );

                        HandleError:
                            delete pff;
                            delete pcconfig;
                            delete pc;
                            throw EseException( err );
                        }

                        virtual Cache^ CreateCacheWrapper( ICache^ icInner )
                        {
                            ERR         err     = JET_errSuccess;
                            ::ICache*   pcInner = NULL;
                            ::ICache*   pc      = NULL;

                            Call( Cache::ErrWrap( icInner, &pcInner ) );

                            Call( Pi->ErrCreateCacheWrapper( &pcInner, &pc ) );

                            return gcnew Cache( &pc );

                        HandleError:
                            delete pcInner;
                            delete pc;
                            throw EseException( err );
                        }

                        virtual CacheRepository^ CreateCacheRepository(
                            FileIdentification^ fident,
                            CacheTelemetry^ ctm )
                        {
                            ERR                 err     = JET_errSuccess;
                            ::ICacheRepository* pcrep   = NULL;

                            Call( Pi->ErrCreateCacheRepository( fident->Pi, ctm->Pi, &pcrep ) );

                            return gcnew CacheRepository( &pcrep );

                        HandleError:
                            delete pcrep;
                            throw EseException( err );
                        }

                        virtual CacheTelemetry^ CreateCacheTelemetry()
                        {
                            ERR                 err     = JET_errSuccess;
                            ::ICacheTelemetry*  pctm    = NULL;

                            Call( Pi->ErrCreateCacheTelemetry( &pctm ) );

                            return gcnew CacheTelemetry( &pctm );

                        HandleError:
                            delete pctm;
                            throw EseException( err );
                        }

                        virtual JournalSegment^ CreateJournalSegment(
                            FileFilter^ ff,
                            Int64 offsetInBytes,
                            SegmentPosition segmentPosition,
                            Int32 uniqueIdPrevious,
                            SegmentPosition segmentPositionReplay,
                            SegmentPosition segmentPositionDurable )
                        {
                            ERR                 err = JET_errSuccess;
                            ::IJournalSegment*  pjs = NULL;

                            Call( Pi->ErrCreateJournalSegment(  ff->Pi,
                                                                offsetInBytes,
                                                                (::SegmentPosition)segmentPosition,
                                                                uniqueIdPrevious,
                                                                (::SegmentPosition)segmentPositionReplay,
                                                                (::SegmentPosition)segmentPositionDurable,
                                                                &pjs ) );

                            return gcnew JournalSegment( &pjs );

                        HandleError:
                            delete pjs;
                            throw EseException( err );
                        }

                        virtual JournalSegment^ LoadJournalSegment(
                            FileFilter^ ff,
                            Int64 offsetInBytes )
                        {
                            ERR                 err = JET_errSuccess;
                            ::IJournalSegment*  pjs = NULL;

                            Call( Pi->ErrLoadJournalSegment( ff->Pi, offsetInBytes, &pjs ) );

                            return gcnew JournalSegment( &pjs );

                        HandleError:
                            delete pjs;
                            throw EseException( err );
                        }

                        virtual JournalSegmentManager^ CreateJournalSegmentManager(
                            FileFilter^ ff,
                            Int64 offsetInBytes,
                            Int64 sizeInBytes )
                        {
                            ERR                         err     = JET_errSuccess;
                            ::IJournalSegmentManager*   pjsm    = NULL;

                            Call( Pi->ErrCreateJournalSegmentManager(   ff->Pi,
                                                                        offsetInBytes,
                                                                        sizeInBytes,
                                                                        &pjsm ) );

                            return gcnew JournalSegmentManager( &pjsm );

                        HandleError:
                            delete pjsm;
                            throw EseException( err );
                        }

                        virtual Journal^ CreateJournal(
                            IJournalSegmentManager^ ijsm,
                            Int64 cacheSizeInBytes )
                        {
                            ERR                         err     = JET_errSuccess;
                            ::IJournalSegmentManager*   pjsm    = NULL;
                            ::IJournal*                 pj      = NULL;

                            Call( JournalSegmentManager::ErrWrap( ijsm, &pjsm ) );

                            Call( Pi->ErrCreateJournal( &pjsm,
                                                        cacheSizeInBytes,
                                                        &pj ) );

                            return gcnew Journal( &pj );

                        HandleError:
                            delete pj;
                            delete pjsm;
                            throw EseException( err );
                        }

                        virtual CachedBlockWriteCountsManager^ LoadCachedBlockWriteCountsManager(
                            FileFilter^ ff,
                            Int64 offsetInBytes,
                            Int64 sizeInBytes,
                            Int64 countCachedBlockWriteCounts)
                        {
                            ERR                                 err     = JET_errSuccess;
                            ::ICachedBlockWriteCountsManager*   pcbwcm  = NULL;

                            Call( Pi->ErrLoadCachedBlockWriteCountsManager( ff->Pi,
                                                                            offsetInBytes,
                                                                            sizeInBytes,
                                                                            countCachedBlockWriteCounts,
                                                                            &pcbwcm ) );

                            return gcnew CachedBlockWriteCountsManager( &pcbwcm );

                        HandleError:
                            delete pcbwcm;
                            throw EseException( err );
                        }

                        virtual CachedBlockSlab^ LoadCachedBlockSlab(
                            FileFilter^ ff,
                            Int64 offsetInBytes,
                            Int64 sizeInBytes,
                            CachedBlockWriteCountsManager^ cbwcm,
                            Int64 cachedBlockWriteCountNumberBase,
                            ClusterNumber clusterNumberMin,
                            ClusterNumber clusterNumberMax,
                            bool ignoreVerificationErrors )
                        {
                            ERR                 err     = JET_errSuccess;
                            ::ICachedBlockSlab* pcbs    = NULL;

                            Call( Pi->ErrLoadCachedBlockSlab(   ff->Pi,
                                                                offsetInBytes,
                                                                sizeInBytes,
                                                                cbwcm->Pi,
                                                                cachedBlockWriteCountNumberBase,
                                                                (::ClusterNumber)clusterNumberMin,
                                                                (::ClusterNumber)clusterNumberMax,
                                                                ignoreVerificationErrors ? fTrue : fFalse,
                                                                &pcbs ) );

                            return gcnew CachedBlockSlab( &pcbs );

                        HandleError:
                            delete pcbs;
                            throw EseException( err );
                        }

                        virtual CachedBlockSlab^ CreateCachedBlockSlabWrapper( ICachedBlockSlab^ icbsInner )
                        {
                            ERR                 err         = JET_errSuccess;
                            ::ICachedBlockSlab* pcbsInner   = NULL;
                            ::ICachedBlockSlab* pcbs        = NULL;

                            Call( CachedBlockSlab::ErrWrap( icbsInner, &pcbsInner ) );

                            Call( Pi->ErrCreateCachedBlockSlabWrapper( &pcbsInner, &pcbs ) );

                            return gcnew CachedBlockSlab( &pcbs );

                        HandleError:
                            delete pcbsInner;
                            delete pcbs;
                            throw EseException( err );
                        }

                        virtual CachedBlockSlabManager^ LoadCachedBlockSlabManager(
                            FileFilter^ ff,
                            Int64 cachingFilePerSlab,
                            Int64 cachedFilePerSlab,
                            Int64 offsetInBytes,
                            Int64 sizeInBytes,
                            CachedBlockWriteCountsManager^ cbwcm,
                            Int64 cachedBlockWriteCountNumberBase,
                            ClusterNumber clusterNumberMin,
                            ClusterNumber clusterNumberMax )
                        {
                            ERR                         err     = JET_errSuccess;
                            ::ICachedBlockSlabManager*  pcbsm   = NULL;

                            Call( Pi->ErrLoadCachedBlockSlabManager(    ff->Pi,
                                                                        cachingFilePerSlab,
                                                                        cachedFilePerSlab,
                                                                        offsetInBytes,
                                                                        sizeInBytes,
                                                                        cbwcm->Pi,
                                                                        cachedBlockWriteCountNumberBase,
                                                                        (::ClusterNumber)clusterNumberMin,
                                                                        (::ClusterNumber)clusterNumberMax,
                                                                        &pcbsm ) );

                            return gcnew CachedBlockSlabManager( &pcbsm );

                        HandleError:
                            delete pcbsm;
                            throw EseException( err );
                        }

                        virtual CachedBlock^ CreateCachedBlock(
                            CachedBlockId^ cachedBlockId,
                            ClusterNumber clusterNumber,
                            bool isValid,
                            bool isPinned,
                            bool isDirty,
                            bool wasEverDirty,
                            bool wasPurged,
                            UpdateNumber updateNumber )
                        {
                            ERR              err    = JET_errSuccess;
                            ::CCachedBlock*  pcbl   = NULL;

                            Alloc( pcbl = new ::CCachedBlock() );

                            Call( Pi->ErrCreateCachedBlock( *cachedBlockId->Pcbid(),
                                                            (::ClusterNumber)clusterNumber,
                                                            isValid ? fTrue : fFalse,
                                                            isPinned ? fTrue : fFalse,
                                                            isDirty ? fTrue : fFalse,
                                                            wasEverDirty ? fTrue : fFalse,
                                                            wasPurged ? fTrue : fFalse,
                                                            (::UpdateNumber)updateNumber,
                                                            pcbl ) );

                            return gcnew CachedBlock( &pcbl );

                        HandleError:
                            delete pcbl;
                            throw EseException( err );
                        }

                        virtual CachedBlockSlot^ CreateCachedBlockSlot(
                            UInt64 offsetInBytes,
                            ChunkNumber chunkNumber,
                            SlotNumber slotNumber,
                            CachedBlock^ cachedBlock )
                        {
                            ERR                 err     = JET_errSuccess;
                            ::CCachedBlockSlot* pslot   = NULL;

                            Alloc( pslot = new ::CCachedBlockSlot() );

                            Call( Pi->ErrCreateCachedBlockSlot( offsetInBytes,
                                                                (::ChunkNumber)chunkNumber,
                                                                (::SlotNumber)slotNumber,
                                                                *cachedBlock->Pcbl(),
                                                                pslot ) );

                            return gcnew CachedBlockSlot( &pslot );

                        HandleError:
                            delete pslot;
                            throw EseException( err );
                        }

                        virtual CachedBlockSlotState^ CreateCachedBlockSlotState(
                            CachedBlockSlot^ slot,
                            bool isSlabUpdated,
                            bool isChunkUpdated,
                            bool isSlotUpdated,
                            bool isClusterUpdated,
                            bool isSuperceded )
                        {
                            ERR                         err     = JET_errSuccess;
                            ::CCachedBlockSlotState*    pslotst = NULL;

                            Alloc( pslotst = new ::CCachedBlockSlotState() );

                            Call( Pi->ErrCreateCachedBlockSlotState(    *slot->Pslot(),
                                                                        isSlabUpdated ? fTrue : fFalse,
                                                                        isChunkUpdated ? fTrue : fFalse,
                                                                        isSlotUpdated ? fTrue : fFalse,
                                                                        isClusterUpdated ? fTrue : fFalse,
                                                                        isSuperceded ? fTrue : fFalse,
                                                                        pslotst ) );

                            return gcnew CachedBlockSlotState( &pslotst );

                        HandleError:
                            delete pslotst;
                            throw EseException( err );
                        }
                };
            }
        }
    }
}
