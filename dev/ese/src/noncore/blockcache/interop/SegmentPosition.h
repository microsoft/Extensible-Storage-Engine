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
                /// Segment Position
                /// </summary>
                public enum class SegmentPosition : Int64
                {
                    /// <summary>
                    /// An invalid SegmentPosition.
                    /// </summary>
                    Invalid = (Int64)::sposInvalid,
                };
            }
        }
    }
}