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
                /// Cached Block Number.
                /// </summary>
                public enum class CachedBlockNumber : UInt32
                {
                    /// <summary>
                    /// An invalid CachedBlockNumber.
                    /// </summary>
                    Invalid = (UInt32)::cbnoInvalid,
                };
            }
        }
    }
}