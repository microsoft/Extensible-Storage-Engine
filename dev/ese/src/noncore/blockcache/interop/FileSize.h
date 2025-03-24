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
                /// The type of file size.
                /// </summary>
                public enum class FileSize : int
                {
                    /// <summary>
                    /// The logical file size, as reported by `dir`.
                    /// </summary>
                    Logical = 0,

                    /// <summary>
                    /// The physical files size; the number of bytes it takes up on disk.
                    /// </summary>
                    OnDisk = 1,
                };
            }
        }
    }
}