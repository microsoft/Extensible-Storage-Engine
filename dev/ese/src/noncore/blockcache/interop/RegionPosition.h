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
                /// Region Position
                /// </summary>
                public enum class RegionPosition : Int64
                {
                    /// <summary>
                    /// An invalid RegionPosition.
                    /// </summary>
                    Invalid = Int64::MaxValue,
                };
            }
        }
    }
}