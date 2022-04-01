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
                ref class BlockCacheFactory : BlockCacheFactoryBase<IBlockCacheFactory, ::IBlockCacheFactory, CBlockCacheFactoryWrapper<IBlockCacheFactory, ::IBlockCacheFactory>>
                {
                    public:

                        BlockCacheFactory( ::IBlockCacheFactory** const ppbcf ) : BlockCacheFactoryBase( ppbcf ) {}
                };
            }
        }
    }
}
