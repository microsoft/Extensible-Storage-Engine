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
                public ref class CacheRepository : public CacheRepositoryBase<ICacheRepository, ::ICacheRepository, CCacheRepositoryWrapper<ICacheRepository, ::ICacheRepository>>
                {
                    public:

                        static ERR ErrWrap( ICacheRepository^% crep, ::ICacheRepository** const ppcrep )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<ICacheRepository, ::ICacheRepository, CCacheRepositoryWrapper<ICacheRepository, ::ICacheRepository>, CacheRepositoryRemotable>( crep, ppcrep );
                        }

                    public:

                        CacheRepository( ICacheRepository^ crep ) : CacheRepositoryBase( crep ) { }

                        CacheRepository( ::ICacheRepository** const ppcrep ) : CacheRepositoryBase( ppcrep ) {}
                };
            }
        }
    }
}
