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
                /// Block Cache Configuration Interface.
                /// </summary>
                public interface class IBlockCacheConfiguration : IDisposable
                {
                    /// <summary>
                    /// Gets the Cached File Configuration for a file at the provided key path.
                    /// </summary>
                    /// <param name="absPathCachedFile>The key path for the cached file.</param>
                    /// <returns>A Cached File Configuration.</returns>
                    ICachedFileConfiguration^ CachedFileConfiguration( String^ keyPathCachedFile );

                    /// <summary>
                    /// Gets the Cache Configuration for a caching file at the provided key path.
                    /// </summary>
                    /// <param name="absPathCachedFile>The key path for the caching file.</param>
                    /// <returns>A Cache Configuration.</returns>
                    ICacheConfiguration^ CacheConfiguration( String^ keyPathCachingFile );
                };
            }
        }
    }
}
