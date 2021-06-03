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
                };
            }
        }
    }
}
