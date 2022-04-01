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
                public ref class CacheConfiguration : CacheConfigurationBase<ICacheConfiguration, ::ICacheConfiguration, CCacheConfigurationWrapper<ICacheConfiguration, ::ICacheConfiguration>>
                {
                    public:

                        static ERR ErrWrap( ICacheConfiguration^% cconfig, ::ICacheConfiguration** const ppcconfig )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<ICacheConfiguration, ::ICacheConfiguration, CCacheConfigurationWrapper<ICacheConfiguration, ::ICacheConfiguration>, CacheConfigurationRemotable>( cconfig, ppcconfig );
                        }

                    public:

                        CacheConfiguration( ICacheConfiguration^ cconfig ) : CacheConfigurationBase( cconfig ) { }

                        CacheConfiguration( ::ICacheConfiguration** const ppcconfig ) : CacheConfigurationBase( ppcconfig ) {}

                        CacheConfiguration( ::ICacheConfiguration* const pcconfig ) : CacheConfigurationBase( pcconfig ) {}
                };
            }
        }
    }
}
