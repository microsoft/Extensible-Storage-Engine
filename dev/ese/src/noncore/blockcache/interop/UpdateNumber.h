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
                /// Update Number
                /// </summary>
                public enum class UpdateNumber : UInt16
                {
                    /// <summary>
                    /// An invalid UpdateNumber.
                    /// </summary>
                    Invalid = (UInt16)::updnoInvalid,

                    /// <summary>
                    /// The maximum UpdateNumber.
                    /// </summary>
                    MaxValue = (UInt16)::updnoMax,
                };
            }
        }
    }
}