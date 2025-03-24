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
                    Raw = 0,

                    /// <summary>
                    /// An access of the underlying cached file by the engine.
                    /// </summary>
                    Engine = 1,

                    /// <summary>
                    /// A read from the underlying cached file by the cache to service a cache miss.
                    /// </summary>
                    CacheMiss = 2,

                    /// <summary>
                    /// A write to the underlying cached file by the cache to service an uncached write or write through.
                    /// </summary>
                    CacheWriteThrough = 3,

                    /// <summary>
                    /// A write to the underlying cached file by the cache to perform a write back.
                    /// </summary>
                    CacheWriteBack = 4,
                };
            }
        }
    }
}