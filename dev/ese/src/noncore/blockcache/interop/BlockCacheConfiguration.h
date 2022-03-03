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
                public ref class BlockCacheConfiguration : public BlockCacheConfigurationBase<IBlockCacheConfiguration, ::IBlockCacheConfiguration, CBlockCacheConfigurationWrapper<IBlockCacheConfiguration, ::IBlockCacheConfiguration>>
                {
                    public:

                        static ERR ErrWrap( IBlockCacheConfiguration^% bcconfig, ::IBlockCacheConfiguration** const ppbcconfig )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<IBlockCacheConfiguration, ::IBlockCacheConfiguration, CBlockCacheConfigurationWrapper<IBlockCacheConfiguration, ::IBlockCacheConfiguration>, BlockCacheConfigurationBaseRemotable>( bcconfig, ppbcconfig );
                        }

                    public:

                        BlockCacheConfiguration( IBlockCacheConfiguration^ bcconfig ) : BlockCacheConfigurationBase( bcconfig ) { }

                        BlockCacheConfiguration( ::IBlockCacheConfiguration** const ppbcconfig ) : BlockCacheConfigurationBase( ppbcconfig ) {}

                        BlockCacheConfiguration( ::IBlockCacheConfiguration* const pbcconfig ) : BlockCacheConfigurationBase( pbcconfig ) {}
                };
            }
        }
    }
}
