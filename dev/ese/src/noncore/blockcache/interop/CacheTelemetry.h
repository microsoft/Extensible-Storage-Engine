// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                public ref class CacheTelemetry : public CacheTelemetryBase<ICacheTelemetry, ::ICacheTelemetry, CCacheTelemetryWrapper<ICacheTelemetry, ::ICacheTelemetry>>
                {
                public:

                    static ERR ErrWrap( ICacheTelemetry^% ctm, ::ICacheTelemetry** const ppctm )
                    {
                        return Internal::Ese::BlockCache::Interop::ErrWrap<ICacheTelemetry, ::ICacheTelemetry, CCacheTelemetryWrapper<ICacheTelemetry, ::ICacheTelemetry>, CacheTelemetryRemotable>( ctm, ppctm );
                    }

                public:

                    CacheTelemetry( ICacheTelemetry^ ctm ) : CacheTelemetryBase( ctm ) { }

                    CacheTelemetry( ::ICacheTelemetry** const ppctm ) : CacheTelemetryBase( ppctm ) {}
                };
            }
        }
    }
}
