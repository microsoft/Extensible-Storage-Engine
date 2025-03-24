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
                /// File System Configuration Interface.
                /// </summary>
                public interface class IFileSystemConfiguration : IDisposable
                {
                    /// <summary>
                    /// Access denied retry period.
                    /// </summary>
                    /// <returns>The retry period.</returns>
                    TimeSpan AccessDeniedRetryPeriod();

                    /// <summary>
                    /// Hung IO threshold.
                    /// </summary>
                    /// <returns>The hung IO threshold delay.</returns>
                    TimeSpan HungIOThreshhold();

                    /// <summary>
                    /// Hung IO actions.
                    /// </summary>
                    /// <returns>The hung IO action options.</returns>
                    int GrbitHungIOActions();

                    /// <summary>
                    /// The maximum read size.
                    /// </summary>
                    /// <returns>The maximum size of a read in bytes.</returns>
                    int MaxReadSizeInBytes();

                    /// <summary>
                    /// The maximum write size.
                    /// </summary>
                    /// <returns>The maximum size of a write in bytes.</returns>
                    int MaxWriteSizeInBytes();

                    /// <summary>
                    /// The maximum read gap size.
                    /// </summary>
                    /// <returns>The maximum read gap size in bytes.</returns>
                    int MaxReadGapSizeInBytes();

                    /// <summary>
                    /// Permillage of IO forced to be smooth IO.
                    /// </summary>
                    /// <returns>The permillage of IO forced to be smooth IO.</returns>
                    int PermillageSmoothIo();

                    /// <summary>
                    /// Indicates if the block cache should be enabled.
                    /// </summary>
                    /// <returns>True if the block cache should be enabled.</returns>
                    bool IsBlockCacheEnabled();

                    /// <summary>
                    /// Gets the Block Cache Configuration.
                    /// </summary>
                    /// <returns>A Block Cache Configuration.</returns>
                    IBlockCacheConfiguration^ BlockCacheConfiguration();
                };
            }
        }
    }
}