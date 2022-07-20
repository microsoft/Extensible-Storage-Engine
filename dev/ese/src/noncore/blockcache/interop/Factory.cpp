// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                public ref class Factory
                {
                    public:

                        static void DisableLeakDetection()
                        {
                            RFSSetKnownResourceLeak();
                        }

                        static IDisposable^ CreateOSLayer()
                        {
                            return gcnew OSLayer();
                        }

                        static FileSystem^ CreateFileSystem( FileSystemConfiguration^ fsconfig )
                        {
                            ERR             err     = JET_errSuccess;
                            IFileSystemAPI* pfsapi  = NULL;

                            Call( ErrOSFSCreate( fsconfig->Pi, &pfsapi ) );

                            return gcnew FileSystem( &pfsapi );

                        HandleError:
                            delete pfsapi;
                            throw EseException( err );
                        }

                        static FileSystemConfiguration^ WrapFileSystemConfiguration( IFileSystemConfiguration^ ifsconfig )
                        {
                            ERR                         err         = JET_errSuccess;
                            ::IFileSystemConfiguration* pfsconfig   = NULL;

                            Call( FileSystemConfiguration::ErrWrap( ifsconfig, &pfsconfig ) );

                            return gcnew FileSystemConfiguration( &pfsconfig );

                        HandleError:
                            delete pfsconfig;
                            throw EseException( err );
                        }

                        static FileSystem^ WrapFileSystem( IFileSystem^ ifs )
                        {
                            ERR                 err     = JET_errSuccess;
                            ::IFileSystemAPI*   pfsapi  = NULL;

                            Call( FileSystem::ErrWrap( ifs, &pfsapi ) );

                            return gcnew FileSystem( &pfsapi );

                        HandleError:
                            delete pfsapi;
                            throw EseException( err );
                        }

                        static File^ WrapFile( IFile^ f )
                        {
                            ERR         err     = JET_errSuccess;
                            ::IFileAPI* pfapi   = NULL;

                            Call( File::ErrWrap( f, &pfapi ) );

                            return gcnew File( &pfapi );

                        HandleError:
                            delete pfapi;
                            throw EseException( err );
                        }

                        static FileIdentification^ WrapFileIdentification( IFileIdentification^ ifident )
                        {
                            ERR                     err     = JET_errSuccess;
                            ::IFileIdentification*  pfident = NULL;

                            Call( FileIdentification::ErrWrap( ifident, &pfident ) );

                            return gcnew FileIdentification( &pfident );

                        HandleError:
                            delete pfident;
                            throw EseException( err );
                        }

                        static FileSystemFilter^ WrapFileSystemFilter( IFileSystemFilter^ ifsf )
                        {
                            ERR                     err     = JET_errSuccess;
                            ::IFileSystemFilter*    pfsd    = NULL;

                            Call( FileSystemFilter::ErrWrap( ifsf, &pfsd ) );

                            return gcnew FileSystemFilter( &pfsd );

                        HandleError:
                            delete pfsd;
                            throw EseException( err );
                        }

                        static FileFilter^ WrapFileFilter( IFileFilter^ iff )
                        {
                            ERR             err = JET_errSuccess;
                            ::IFileFilter*  pff = NULL;

                            Call( FileFilter::ErrWrap( iff, &pff ) );

                            return gcnew FileFilter( &pff );

                        HandleError:
                            delete pff;
                            throw EseException( err );
                        }

                        static BlockCacheConfiguration^ WrapBlockCacheConfiguration( IBlockCacheConfiguration^ ibcconfig )
                        {
                            ERR                         err         = JET_errSuccess;
                            ::IBlockCacheConfiguration* pbcconfig   = NULL;

                            Call( BlockCacheConfiguration::ErrWrap( ibcconfig, &pbcconfig ) );

                            return gcnew BlockCacheConfiguration( &pbcconfig );

                        HandleError:
                            delete pbcconfig;
                            throw EseException( err );
                        }

                        static CacheConfiguration^ WrapCacheConfiguration( ICacheConfiguration^ icconfig )
                        {
                            ERR                     err         = JET_errSuccess;
                            ::ICacheConfiguration*  pcconfig    = NULL;

                            Call( CacheConfiguration::ErrWrap( icconfig, &pcconfig ) );

                            return gcnew CacheConfiguration( &pcconfig );

                        HandleError:
                            delete pcconfig;
                            throw EseException( err );
                        }

                        static CachedFileConfiguration^ WrapCachedFileConfiguration( ICachedFileConfiguration^ icfconfig )
                        {
                            ERR                         err         = JET_errSuccess;
                            ::ICachedFileConfiguration* pcfconfig   = NULL;

                            Call( CachedFileConfiguration::ErrWrap( icfconfig, &pcfconfig ) );

                            return gcnew CachedFileConfiguration( &pcfconfig );

                        HandleError:
                            delete pcfconfig;
                            throw EseException( err );
                        }

                        static Cache^ WrapCache( ICache^ ic )
                        {
                            ERR         err = JET_errSuccess;
                            ::ICache*   pc  = NULL;

                            Call( Cache::ErrWrap( ic, &pc ) );

                            return gcnew Cache( &pc );

                        HandleError:
                            delete pc;
                            throw EseException( err );
                        }

                        static CacheRepository^ WrapCacheRepository( ICacheRepository^ icrep )
                        {
                            ERR                 err     = JET_errSuccess;
                            ::ICacheRepository* pcrep   = NULL;

                            Call( CacheRepository::ErrWrap( icrep, &pcrep ) );

                            return gcnew CacheRepository( &pcrep );

                        HandleError:
                            delete pcrep;
                            throw EseException( err );
                        }

                        static CacheTelemetry^ WrapCacheTelemetry( ICacheTelemetry^ ictm )
                        {
                            ERR                 err     = JET_errSuccess;
                            ::ICacheTelemetry*  pctm    = NULL;

                            Call( CacheTelemetry::ErrWrap( ictm, &pctm ) );

                            return gcnew CacheTelemetry( &pctm );

                        HandleError:
                            delete pctm;
                            throw EseException( err );
                        }
                        
                        static JournalSegment^ WrapJournalSegment( IJournalSegment^ ijs )
                        {
                            ERR                 err = JET_errSuccess;
                            ::IJournalSegment*  pjs = NULL;

                            Call( JournalSegment::ErrWrap( ijs, &pjs ) );

                            return gcnew JournalSegment( &pjs );

                        HandleError:
                            delete pjs;
                            throw EseException( err );
                        }
                        
                        static JournalSegmentManager^ WrapJournalSegmentManager( IJournalSegmentManager^ ijsm )
                        {
                            ERR                         err     = JET_errSuccess;
                            ::IJournalSegmentManager*   pjsm    = NULL;

                            Call( JournalSegmentManager::ErrWrap( ijsm, &pjsm ) );

                            return gcnew JournalSegmentManager( &pjsm );

                        HandleError:
                            delete pjsm;
                            throw EseException( err );
                        }
                        
                        static Journal^ WrapJournal( IJournal^ ij )
                        {
                            ERR         err = JET_errSuccess;
                            ::IJournal* pj  = NULL;

                            Call( Journal::ErrWrap( ij, &pj ) );

                            return gcnew Journal( &pj );

                        HandleError:
                            delete pj;
                            throw EseException( err );
                        }

                        static CachedBlockWriteCountsManager^ WrapCachedBlockWriteCountsManager( ICachedBlockWriteCountsManager^ icbwcm )
                        {
                            ERR                                 err     = JET_errSuccess;
                            ::ICachedBlockWriteCountsManager*   pcbwcm  = NULL;

                            Call( CachedBlockWriteCountsManager::ErrWrap( icbwcm, &pcbwcm ) );

                            return gcnew CachedBlockWriteCountsManager( &pcbwcm );

                        HandleError:
                            delete pcbwcm;
                            throw EseException( err );
                        }

                        static CachedBlockSlab^ WrapCachedBlockSlab( ICachedBlockSlab^ icbs )
                        {
                            ERR                 err     = JET_errSuccess;
                            ::ICachedBlockSlab* pcbs    = NULL;

                            Call( CachedBlockSlab::ErrWrap( icbs, &pcbs ) );

                            return gcnew CachedBlockSlab( &pcbs );

                        HandleError:
                            delete pcbs;
                            throw EseException( err );
                        }

                        static CachedBlockSlabManager^ WrapCachedBlockSlabManager( ICachedBlockSlabManager^ icbsm )
                        {
                            ERR                         err     = JET_errSuccess;
                            ::ICachedBlockSlabManager*  pcbsm   = NULL;

                            Call( CachedBlockSlabManager::ErrWrap( icbsm, &pcbsm ) );

                            return gcnew CachedBlockSlabManager( &pcbsm );

                        HandleError:
                            delete pcbsm;
                            throw EseException( err );
                        }

                        static FileSystem^ CreateFileSystemWrapper( IFileSystem^ ifsInner )
                        {
                            return factory->CreateFileSystemWrapper( ifsInner );
                        }

                        static FileSystemFilter^ CreateFileSystemFilter(    FileSystemConfiguration^ fsconfig,
                                                                            IFileSystem^ ifsInner,
                                                                            FileIdentification^ fident,
                                                                            CacheTelemetry^ ctm,
                                                                            CacheRepository^ crep )
                        {
                            return factory->CreateFileSystemFilter( fsconfig, ifsInner, fident, ctm, crep );
                        }

                        static File^ CreateFileWrapper( IFile^ ifInner )
                        {
                            return factory->CreateFileWrapper( ifInner );
                        }

                        static FileFilter^ CreateFileFilter(    IFile^ fInner,
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
                            return factory->CreateFileFilter( fInner, fsf, fsconfig, fident, ctm, crep, volumeid, fileid, icfconfig, ic, header );
                        }

                        static FileFilter^ CreateFileFilterWrapper( IFileFilter^ iffInner, IOMode ioMode )
                        {
                            return factory->CreateFileFilterWrapper( iffInner, ioMode );
                        }

                        static FileIdentification^ CreateFileIdentification()
                        {
                            return factory->CreateFileIdentification();
                        }

                        static Cache^ CreateCache(  FileSystemFilter^ fsf,
                                                    FileIdentification^ fident,
                                                    FileSystemConfiguration^ fsconfig,
                                                    ICacheConfiguration^ cconfig,
                                                    CacheTelemetry^ ctm,
                                                    IFileFilter^ iff )
                        {
                            return factory->CreateCache( fsf, fident, fsconfig, cconfig, ctm, iff );
                        }

                        static Cache^ MountCache(   FileSystemFilter^ fsf,
                                                    FileIdentification^ fident,
                                                    FileSystemConfiguration^ fsconfig,
                                                    ICacheConfiguration^ cconfig,
                                                    CacheTelemetry^ ctm,
                                                    IFileFilter^ iff )
                        {
                            return factory->MountCache( fsf, fident, fsconfig, cconfig, ctm, iff );
                        }

                        static Cache^ CreateCacheWrapper( ICache^ icInner )
                        {
                            return factory->CreateCacheWrapper( icInner );
                        }

                        static CacheRepository^ CreateCacheRepository(  FileIdentification^ fident,
                                                                        CacheTelemetry^ ctm )
                        {
                            return factory->CreateCacheRepository( fident, ctm );
                        }

                        static CacheTelemetry^ CreateCacheTelemetry()
                        {
                            return factory->CreateCacheTelemetry();
                        }

                        static JournalSegment^ CreateJournalSegment(    FileFilter^ ff,
                                                                        Int64 offsetInBytes,
                                                                        SegmentPosition segmentPosition,
                                                                        Int32 uniqueIdPrevious,
                                                                        SegmentPosition segmentPositionReplay,
                                                                        SegmentPosition segmentPositionDurable )
                        {
                            return factory->CreateJournalSegment(   ff,
                                                                    offsetInBytes,
                                                                    segmentPosition, 
                                                                    uniqueIdPrevious, 
                                                                    segmentPositionReplay, 
                                                                    segmentPositionDurable );
                        }

                        static JournalSegment^ LoadJournalSegment(  FileFilter^ ff,
                                                                    Int64 offsetInBytes )
                        {
                            return factory->LoadJournalSegment( ff, offsetInBytes );
                        }

                        static JournalSegmentManager^ CreateJournalSegmentManager(  FileFilter^ ff,
                                                                                    Int64 offsetInBytes,
                                                                                    Int64 sizeInBytes )
                        {
                            return factory->CreateJournalSegmentManager( ff, offsetInBytes, sizeInBytes );
                        }

                        static Journal^ CreateJournal(  IJournalSegmentManager^ ijsm,
                                                        Int64 cacheSizeInBytes )
                        {
                            return factory->CreateJournal( ijsm, cacheSizeInBytes );
                        }

                        static CachedBlockWriteCountsManager^ LoadCachedBlockWriteCountsManager(
                            FileFilter^ ff,
                            Int64 offsetInBytes,
                            Int64 sizeInBytes,
                            Int64 countCachedBlockWriteCounts )
                        {
                            return factory->LoadCachedBlockWriteCountsManager( ff, offsetInBytes, sizeInBytes, countCachedBlockWriteCounts );
                        }

                        static CachedBlockSlab^ LoadCachedBlockSlab(
                            FileFilter^ ff,
                            Int64 offsetInBytes,
                            Int64 sizeInBytes,
                            CachedBlockWriteCountsManager^ cbwcm,
                            Int64 cachedBlockWriteCountNumberBase,
                            ClusterNumber clusterNumberMin,
                            ClusterNumber clusterNumberMax,
                            bool ignoreVerificationErrors,
                            [Out] EsentErrorException^% ex )
                        {
                            return factory->LoadCachedBlockSlab(
                                ff,
                                offsetInBytes, 
                                sizeInBytes, 
                                cbwcm, 
                                cachedBlockWriteCountNumberBase, 
                                clusterNumberMin, 
                                clusterNumberMax,
                                ignoreVerificationErrors,
                                ex );
                        }

                        static CachedBlockSlab^ CreateCachedBlockSlabWrapper( ICachedBlockSlab^ icbsInner )
                        {
                            return factory->CreateCachedBlockSlabWrapper( icbsInner );
                        }

                        static CachedBlockSlabManager^ LoadCachedBlockSlabManager(
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
                            return factory->LoadCachedBlockSlabManager(
                                ff,
                                cachingFilePerSlab,
                                cachedFilePerSlab,
                                offsetInBytes,
                                sizeInBytes,
                                cbwcm,
                                cachedBlockWriteCountNumberBase,
                                clusterNumberMin,
                                clusterNumberMax );
                        }

                        static CachedBlock^ CreateCachedBlock(
                            CachedBlockId^ cachedBlockId,
                            ClusterNumber clusterNumber,
                            bool isValid,
                            bool isPinned,
                            bool isDirty,
                            bool wasEverDirty,
                            bool wasPurged,
                            UpdateNumber updateNumber )
                        {
                            return factory->CreateCachedBlock(
                                cachedBlockId,
                                clusterNumber,
                                isValid,
                                isPinned,
                                isDirty,
                                wasEverDirty,
                                wasPurged,
                                updateNumber );
                        }

                        static CachedBlockSlot^ CreateCachedBlockSlot(
                            UInt64 offsetInBytes,
                            ChunkNumber chunkNumber,
                            SlotNumber slotNumber,
                            CachedBlock^ cachedBlock )
                        {
                            return factory->CreateCachedBlockSlot( offsetInBytes, chunkNumber, slotNumber, cachedBlock );
                        }

                        static CachedBlockSlotState^ CreateCachedBlockSlotState(
                            CachedBlockSlot^ slot,
                            bool isSlabUpdated,
                            bool isChunkUpdated,
                            bool isSlotUpdated,
                            bool isClusterUpdated,
                            bool isSuperceded )
                        {
                            return factory->CreateCachedBlockSlotState(
                                slot,
                                isSlabUpdated,
                                isChunkUpdated,
                                isSlotUpdated,
                                isClusterUpdated,
                                isSuperceded );
                        }

                        static void DetachFile( String^ path, IBlockCacheFactory::DetachFileStatus^ status )
                        {
                            return factory->DetachFile( path, status );
                        }

                    private:

                        ref class OSLayer : IDisposable
                        {
                            public:
                            
                                OSLayer()
                                {
                                    COSLayerPreInit::SetDefaults();
                                    ERR err = ErrOSInit();
                                    if ( err < JET_errSuccess )
                                    {
                                        throw EsentExceptionHelper::JetErrToException( (JET_err)err );
                                    }
                                }

                                ~OSLayer()
                                {
                                    OSTerm();
                                }
                        };


                        static BlockCacheFactory^ CreateBlockCacheFactory()
                        {
                            ERR                     err     = JET_errSuccess;
                            ::IBlockCacheFactory*   pbcf    = NULL;

                            Call( COSBlockCacheFactory::ErrCreate( &pbcf ) );

                            WCHAR wszBuf[ 256 ] = { };
                            if (    IsDebuggerPresent() &&
                                    FOSConfigGet( L"DEBUG", L"Error Trap", wszBuf, sizeof( wszBuf ) ) &&
                                    wszBuf[0] )
                            {
                                (void)ErrERRSetErrTrap( _wtol( wszBuf ) );
                            }

                            return gcnew BlockCacheFactory( &pbcf );

                        HandleError:
                            delete pbcf;
                            throw EseException( err );
                        }

                        static BlockCacheFactory^ factory = CreateBlockCacheFactory();
                };
            }
        }
    }
}