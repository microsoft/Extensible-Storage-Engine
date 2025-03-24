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
                /// Caching policy.
                /// </summary>
                public enum class CachingPolicy : int
                {
                    /// <summary>
                    /// Don't cache the request.
                    /// </summary>
                    DontCache = 0,

                    /// <summary>
                    /// Perform best effort caching of the request.
                    /// </summary>
                    BestEffort = 1,
                };
            }
        }
    }
}