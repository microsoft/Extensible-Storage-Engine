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
                /// Journal Position
                /// </summary>
                public enum class JournalPosition : Int64
                {
                    /// <summary>
                    /// An invalid JournalPosition.
                    /// </summary>
                    Invalid = Int64::MaxValue,
                };
            }
        }
    }
}