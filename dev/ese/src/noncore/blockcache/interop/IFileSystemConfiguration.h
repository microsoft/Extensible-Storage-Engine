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
                public interface class IFileSystemConfiguration : IDisposable
                {
                    TimeSpan AccessDeniedRetryPeriod();

                    TimeSpan HungIOThreshhold();

                    int GrbitHungIOActions();

                    int MaxReadSizeInBytes();

                    int MaxWriteSizeInBytes();

                    int MaxReadGapSizeInBytes();

                    int PermillageSmoothIo();

                    bool IsBlockCacheEnabled();

                    IBlockCacheConfiguration^ BlockCacheConfiguration();
                };
            }
        }
    }
}
