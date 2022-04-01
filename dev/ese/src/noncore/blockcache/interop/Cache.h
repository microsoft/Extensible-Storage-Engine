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
                public ref class Cache : CacheBase<ICache, ::ICache, CCacheWrapper<ICache, ::ICache>>
                {
                    public:

                        static ERR ErrWrap( ICache^% c, ::ICache** const ppc )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<ICache, ::ICache, CCacheWrapper<ICache, ::ICache>, CacheRemotable>( c, ppc );
                        }

                    public:

                        Cache( ICache^ c ) : CacheBase( c ) { }

                        Cache( ::ICache** const ppc ) : CacheBase( ppc ) {}
                };
            }
        }
    }
}
