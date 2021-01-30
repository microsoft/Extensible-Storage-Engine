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
                public enum class FileSize : int
                {
                    Logical = 0,

                    OnDisk = 1,
                };
            }
        }
    }
}
