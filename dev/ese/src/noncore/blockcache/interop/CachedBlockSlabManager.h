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
                public ref class CachedBlockSlabManager : public CachedBlockSlabManagerBase<ICachedBlockSlabManager, ::ICachedBlockSlabManager, CCachedBlockSlabManagerWrapper<ICachedBlockSlabManager, ::ICachedBlockSlabManager>>
                {
                    public:

                        static ERR ErrWrap( ICachedBlockSlabManager^% cbsm, ::ICachedBlockSlabManager** const ppcbsm )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<ICachedBlockSlabManager, ::ICachedBlockSlabManager, CCachedBlockSlabManagerWrapper<ICachedBlockSlabManager, ::ICachedBlockSlabManager>, CachedBlockSlabManagerRemotable>( cbsm, ppcbsm );
                        }

                    public:

                        CachedBlockSlabManager( ICachedBlockSlabManager^ cbsm ) : CachedBlockSlabManagerBase( cbsm ) { }

                        CachedBlockSlabManager( ::ICachedBlockSlabManager** const ppcbsm ) : CachedBlockSlabManagerBase( ppcbsm ) {}

                        CachedBlockSlabManager( ::ICachedBlockSlabManager* const pcbsm ) : CachedBlockSlabManagerBase( pcbsm ) {}
                };
            }
        }
    }
}
