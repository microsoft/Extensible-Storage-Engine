// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

#undef interface

#pragma managed

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                public interface class ICachedFileConfiguration : IDisposable
                {
                    bool IsCachingEnabled();

                    String^ CachingFilePath();

                    int BlockSize();

                    int MaxConcurrentBlockWriteBacks();

                    int CacheTelemetryFileNumber();
                };
            }
        }
    }
}
