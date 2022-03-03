// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

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
                /// IO operation mode.
                /// </summary>
                public enum class IOMode : int
                {
                    /// <summary>
                    /// A raw access of the underlying cached file.
                    /// </summary>
                    Raw = (int)::iomRaw,

                    /// <summary>
                    /// An access of the underlying cached file by the engine.
                    /// </summary>
                    Engine = (int)::iomEngine,

                    /// <summary>
                    /// A read from the underlying cached file by the cache to service a cache miss.
                    /// </summary>
                    CacheMiss = (int)::iomCacheMiss,

                    /// <summary>
                    /// A write to the underlying cached file by the cache to service an uncached write or write through.
                    /// </summary>
                    CacheWriteThrough = (int)::iomCacheWriteThrough,

                    /// <summary>
                    /// A write to the underlying cached file by the cache to perform a write back.
                    /// </summary>
                    CacheWriteBack = (int)::iomCacheWriteBack,
                };
            }
        }
    }
}