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
                public interface class ICacheConfiguration : IDisposable
                {
                    bool IsCacheEnabled();

                    Guid CacheType();

                    String^ Path();

                    Int64 MaximumSize();

                    double PercentWrite();
                };
            }
        }
    }
}
