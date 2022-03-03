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
                /// Cache Configuration Interface.
                /// </summary>
                public interface class ICacheConfiguration : IDisposable
                {
                    /// <summary>
                    /// Indicates if this cache is enabled.
                    /// </summary>
                    /// <returns>True if the this cache is enabled.</returns>
                    bool IsCacheEnabled();

                    /// <summary>
                    /// Indicates the desired cache type.
                    /// </summary>
                    /// <returns>The desired cache type.</returns>
                    Guid CacheType();

                    /// <summary>
                    /// The path of this caching file.
                    /// </summary>
                    /// <returns>The absolute path of the caching file.</returns>
                    String^ Path();

                    /// <summary>
                    /// The maximum size of this caching file.
                    /// </summary>
                    /// <returns>The maximum size of this caching file in bytes.</returns>
                    Int64 MaximumSize();

                    /// <summary>
                    /// The amount of this caching file dedicated to write back caching.
                    /// </summary>
                    /// <returns>The percentage of this caching file holding data for write back caching only.</returns>
                    double PercentWrite();

                    /// <summary>
                    /// The maximum size of this caching file used by journal segments.
                    /// </summary>
                    /// <returns>The maximum size of this caching file used by journal segments.</returns>
                    Int64 JournalSegmentsMaximumSize();

                    /// <summary>
                    /// The amount of the journal segments that should be in use.
                    /// </summary>
                    /// <returns>The amount of the journal segments that should be in use.</returns>
                    double PercentJournalSegmentsInUse();

                    /// <summary>
                    /// The maximum amount of cache memory used by journal segments.
                    /// </summary>
                    /// <returns>The maximum amount of cache memory used by journal segments.</returns>
                    Int64 JournalSegmentsMaximumCacheSize();

                    /// <summary>
                    /// The maximum size of this caching file used by journal clusters.
                    /// </summary>
                    /// <returns>The maximum size of this caching file used by journal clusters.</returns>
                    Int64 JournalClustersMaximumSize();

                    /// <summary>
                    /// The amount of caching file managed as a unit.
                    /// </summary>
                    /// <returns>The amount of caching file managed as a unit.</returns>
                    Int64 CachingFilePerSlab();

                    /// <summary>
                    /// The number of consecutive bytes of a cached file that land in the same caching unit.
                    /// </summary>
                    /// <returns>The number of consecutive bytes of a cached file that land in the same caching unit.</returns>
                    Int64 CachedFilePerSlab();

                    /// <summary>
                    /// The maximum amount of cache memory used by slabs.
                    /// </summary>
                    /// <returns>The maximum amount of cache memory used by slabs.</returns>
                    Int64 SlabMaximumCacheSize();

                    /// <summary>
                    /// Indicates if asynchronous write back is enabled.
                    /// </summary>
                    /// <returns>True if asynchronous write back is enabled.</returns>
                    bool IsAsyncWriteBackEnabled();
                };
            }
        }
    }
}