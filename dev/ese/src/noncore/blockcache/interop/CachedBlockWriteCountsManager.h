#pragma once
ref class CachedBlockWriteCountsManager
{
};

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
                public ref class CachedBlockWriteCountsManager : public CachedBlockWriteCountsManagerBase<ICachedBlockWriteCountsManager, ::ICachedBlockWriteCountsManager, CCachedBlockWriteCountsManagerWrapper<ICachedBlockWriteCountsManager, ::ICachedBlockWriteCountsManager>>
                {
                    public:

                        static ERR ErrWrap( ICachedBlockWriteCountsManager^% cbwcm, ::ICachedBlockWriteCountsManager** const ppcbwcm )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<ICachedBlockWriteCountsManager, ::ICachedBlockWriteCountsManager, CCachedBlockWriteCountsManagerWrapper<ICachedBlockWriteCountsManager, ::ICachedBlockWriteCountsManager>, CachedBlockWriteCountsManagerRemotable>( cbwcm, ppcbwcm );
                        }

                    public:

                        CachedBlockWriteCountsManager( ICachedBlockWriteCountsManager^ cbwcm ) : CachedBlockWriteCountsManagerBase( cbwcm ) { }

                        CachedBlockWriteCountsManager( ::ICachedBlockWriteCountsManager** const ppcbwcm ) : CachedBlockWriteCountsManagerBase( ppcbwcm ) {}

                        CachedBlockWriteCountsManager( ::ICachedBlockWriteCountsManager* const pcbwcm ) : CachedBlockWriteCountsManagerBase( pcbwcm ) {}
                };
            }
        }
    }
}
