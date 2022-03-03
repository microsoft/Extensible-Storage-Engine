// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

#undef interface

#pragma managed

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                /// <summary>
                /// Block Cache Factory Interface.
                /// </summary>
                public interface class IBlockCacheFactory : IDisposable
                {
                    FileSystem^ CreateFileSystemWrapper(IFileSystem^ ifsInner);

                    FileSystemFilter^ CreateFileSystemFilter(
                        FileSystemConfiguration^ fsconfig,
                        IFileSystem^ ifsInner,
                        FileIdentification^ fident,
                        CacheTelemetry^ ctm,
                        CacheRepository^ crep );

                    File^ CreateFileWrapper( IFile^ ifInner );

                    FileFilter^ CreateFileFilter(
                        IFile^ fInner,
                        FileSystemFilter^ fsf,
                        FileSystemConfiguration^ fsconfig,
                        CacheTelemetry^ ctm,
                        VolumeId volumeid,
                        FileId fileid,
                        ICachedFileConfiguration^ icfconfig,
                        ICache^ ic,
                        ArraySegment<byte> header );

                    FileFilter^ CreateFileFilterWrapper( IFileFilter^ iffInner, IOMode ioMode );

                    FileIdentification^ CreateFileIdentification();

                    Cache^ CreateCache(
                        FileSystemFilter^ fsf,
                        FileIdentification^ fident,
                        FileSystemConfiguration^ fsconfig,
                        ICacheConfiguration^ cconfig,
                        CacheTelemetry^ ctm,
                        IFileFilter^ iff );

                    Cache^ MountCache(
                        FileSystemFilter^ fsf,
                        FileIdentification^ fident,
                        FileSystemConfiguration^ fsconfig,
                        ICacheConfiguration^ cconfig,
                        CacheTelemetry^ ctm,
                        IFileFilter^ iff );

                    Cache^ CreateCacheWrapper( ICache^ icInner );

                    CacheRepository^ CreateCacheRepository(
                        FileIdentification^ fident,
                        CacheTelemetry^ ctm );

                    CacheTelemetry^ CreateCacheTelemetry();

                    JournalSegment^ CreateJournalSegment(
                        FileFilter^ ff,
                        Int64 offsetInBytes,
                        SegmentPosition segmentPosition,
                        Int32 uniqueIdPrevious,
                        SegmentPosition segmentPositionReplay,
                        SegmentPosition segmentPositionDurable );

                    JournalSegment^ LoadJournalSegment(
                        FileFilter^ ff,
                        Int64 offsetInBytes );

                    JournalSegmentManager^ CreateJournalSegmentManager(
                        FileFilter^ ff,
                        Int64 offsetInBytes,
                        Int64 sizeInBytes );

                    Journal^ CreateJournal(
                        IJournalSegmentManager^ ijsm,
                        Int64 cacheSizeInBytes );

                    CachedBlockWriteCountsManager^ LoadCachedBlockWriteCountsManager(
                        FileFilter^ ff,
                        Int64 offsetInBytes,
                        Int64 sizeInBytes );

                    CachedBlockSlab^ LoadCachedBlockSlab(
                        FileFilter^ ff,
                        Int64 offsetInBytes,
                        Int64 sizeInBytes,
                        CachedBlockWriteCountsManager^ cbwcm,
                        Int64 cachedBlockWriteCountNumberBase,
                        ClusterNumber clusterNumberMin,
                        ClusterNumber clusterNumberMax,
                        bool ignoreVerificationErrors );

                    CachedBlockSlab^ CreateCachedBlockSlabWrapper( ICachedBlockSlab^ icbsInner );

                    CachedBlockSlabManager^ LoadCachedBlockSlabManager(
                        FileFilter^ ff,
                        Int64 cachingFilePerSlab,
                        Int64 cachedFilePerSlab,
                        Int64 offsetInBytes,
                        Int64 sizeInBytes,
                        CachedBlockWriteCountsManager^ cbwcm,
                        Int64 cachedBlockWriteCountNumberBase,
                        ClusterNumber clusterNumberMin,
                        ClusterNumber clusterNumberMax );

                    CachedBlock^ CreateCachedBlock(
                        CachedBlockId^ cachedBlockId,
                        ClusterNumber clusterNumber,
                        bool isValid ,
                        bool isPinned,
                        bool isDirty,
                        bool wasEverDirty,
                        bool wasPurged,
                        UpdateNumber updateNumber );

                    CachedBlockSlot^ CreateCachedBlockSlot(
                        UInt64 offsetInBytes,
                        ChunkNumber chunkNumber,
                        SlotNumber slotNumber,
                        CachedBlock^ cachedBlock );

                    CachedBlockSlotState^ CreateCachedBlockSlotState(
                        CachedBlockSlot^ slot,
                        bool isSlabUpdated,
                        bool isChunkUpdated,
                        bool isSlotUpdated,
                        bool isClusterUpdated,
                        bool isSuperceded );
                };
            }
        }
    }
}