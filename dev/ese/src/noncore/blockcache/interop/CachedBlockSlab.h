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
                public ref class CachedBlockSlab : CachedBlockSlabBase<ICachedBlockSlab, ::ICachedBlockSlab, CCachedBlockSlabWrapper<ICachedBlockSlab, ::ICachedBlockSlab>>
                {
                    public:

                        static ERR ErrWrap( ICachedBlockSlab^% cbs, ::ICachedBlockSlab** const ppcbs )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<ICachedBlockSlab, ::ICachedBlockSlab, CCachedBlockSlabWrapper<ICachedBlockSlab, ::ICachedBlockSlab>, CachedBlockSlabRemotable>( cbs, ppcbs );
                        }

                    public:

                        CachedBlockSlab( ICachedBlockSlab^ cbs ) : CachedBlockSlabBase( cbs ) { }

                        CachedBlockSlab( ::ICachedBlockSlab** const ppcbs ) : CachedBlockSlabBase( ppcbs ) {}

                        CachedBlockSlab( ::ICachedBlockSlab* const pcbs ) : CachedBlockSlabBase( pcbs ) {}
                };
            }
        }
    }
}
