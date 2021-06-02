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
                /// Cached File Configuration Interface.
                /// </summary>
                public interface class ICachedFileConfiguration : IDisposable
                {
                    /// <summary>
                    /// Indicates if the file should be cached.
                    /// </summary>
                    /// <returns>True if the file should be cached.</returns>
                    bool IsCachingEnabled();

                    /// <summary>
                    /// Indicates the path to the caching file that should cache the contents of this file.
                    /// </summary>
                    /// <returns>The path to the caching file.</returns>
                    String^ CachingFilePath();

                    /// <summary>
                    /// The unit of caching for the file.
                    /// </summary>
                    /// <returns>The size of the minimum unit of caching for the file.</returns>
                    int BlockSize();

                    /// <summary>
                    /// The maximum number of concurrent block write backs that can occur for the file.
                    /// </summary>
                    /// <returns>The maximum number of concurrent block write backs.</returns>
                    int MaxConcurrentBlockWriteBacks();

                    /// <summary>
                    /// The file number to use for cache telemetry.
                    /// </summary>
                    /// <returns>The file number.</returns>
                    int CacheTelemetryFileNumber();
                };
            }
        }
    }
}
