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
                /// Touch Number
                /// </summary>
                public enum class TouchNumber : UInt32
                {
                    /// <summary>
                    /// An invalid TouchNumber.
                    /// </summary>
                    Invalid = (UInt32)::tonoInvalid,
                };
            }
        }
    }
}