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
                ref class BlockCacheConfigurationBaseRemotable : MarshalByRefObject, IBlockCacheConfiguration
                {
                    public:

                        BlockCacheConfigurationBaseRemotable( IBlockCacheConfiguration^ target )
                            :   target( target )
                        {
                        }

                        ~BlockCacheConfigurationBaseRemotable() {}

                    public:

                        virtual ICachedFileConfiguration^ CachedFileConfiguration( String^ keyPathCachedFile )
                        {
                            return this->target->CachedFileConfiguration( keyPathCachedFile );
                        }

                        virtual ICacheConfiguration^ CacheConfiguration( String^ keyPathCachingFile )
                        {
                            return this->target->CacheConfiguration( keyPathCachingFile );
                        }

                    private:

                        IBlockCacheConfiguration^ target;
                };
            }
        }
    }
}