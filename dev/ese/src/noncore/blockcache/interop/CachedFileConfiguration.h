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
                public ref class CachedFileConfiguration : public CachedFileConfigurationBase<ICachedFileConfiguration, ::ICachedFileConfiguration, CCachedFileConfigurationWrapper<ICachedFileConfiguration, ::ICachedFileConfiguration>>
                {
                    public:

                        static ERR ErrWrap( ICachedFileConfiguration^% cfconfig, ::ICachedFileConfiguration** const ppcfconfig )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<ICachedFileConfiguration, ::ICachedFileConfiguration, CCachedFileConfigurationWrapper<ICachedFileConfiguration, ::ICachedFileConfiguration>, CachedFileConfigurationRemotable>( cfconfig, ppcfconfig );
                        }

                    public:

                        CachedFileConfiguration( ICachedFileConfiguration^ cfconfig ) : CachedFileConfigurationBase( cfconfig ) { }

                        CachedFileConfiguration( ::ICachedFileConfiguration** const ppcfconfig ) : CachedFileConfigurationBase( ppcfconfig ) {}
                };
            }
        }
    }
}
