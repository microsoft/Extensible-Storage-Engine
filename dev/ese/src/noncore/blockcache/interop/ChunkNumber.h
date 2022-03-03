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
                /// Chunk Number
                /// </summary>
                public enum class ChunkNumber : UInt32
                {
                    /// <summary>
                    /// An invalid ChunkNumber.
                    /// </summary>
                    Invalid = (UInt32)::chnoInvalid,
                };
            }
        }
    }
}