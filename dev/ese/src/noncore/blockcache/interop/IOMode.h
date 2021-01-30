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
                public enum class IOMode : int
                {
                    Raw = 0,

                    Engine = 1,

                    CacheMiss = 2,

                    CacheWriteThrough = 3,

                    CacheWriteBack = 4,
                };
            }
        }
    }
}
