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
                /// Cluster Number
                /// </summary>
                public enum class CachedBlockWriteCount : UInt32
                {
                    /// <summary>
                    /// The write count is unknown.
                    /// </summary>
                    Unknown = (UInt32)::cbwcUnknown,

                    /// <summary>
                    /// the cached block has never been written.
                    /// </summary>
                    NeverWritten = (UInt32)::cbwcNeverWritten,
                };
            }
        }
    }
}