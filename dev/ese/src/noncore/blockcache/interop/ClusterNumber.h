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
                public enum class ClusterNumber : UInt32
                {
                    /// <summary>
                    /// An invalid ClusterNumber.
                    /// </summary>
                    Invalid = (UInt32)::clnoInvalid,
                };
            }
        }
    }
}