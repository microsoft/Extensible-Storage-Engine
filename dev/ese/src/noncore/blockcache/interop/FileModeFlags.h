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
                [Flags]
                public enum class FileModeFlags : int
                {
                    None = 0,

                    Regular = 0,

                    Cached = 0x1,

                    StorageWriteBack = 0x2,

                    LossyWriteBack = 0x4,

                    Sparse = 0x8,

                    Temporary = 0x100,

                    OverwriteExisting = 0x200,

                    ReadOnly = 0x10000,

                    ReadOnlyClient = 0x20000,

                    ReadOnlyPermissive = 0x40000,
                };
            }
        }
    }
}
