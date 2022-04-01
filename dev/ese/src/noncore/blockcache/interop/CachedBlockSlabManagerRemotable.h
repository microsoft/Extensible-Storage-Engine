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
                ref class CachedBlockSlabManagerRemotable : Remotable, ICachedBlockSlabManager
                {
                    public:

                        CachedBlockSlabManagerRemotable( ICachedBlockSlabManager^ target )
                            :   target( target )
                        {
                        }

                        ~CachedBlockSlabManagerRemotable() {}

                    public:

                        virtual ICachedBlockSlab^ GetSlab( CachedBlockId^ cachedBlockId )
                        {
                            return this->target->GetSlab( cachedBlockId );
                        }

                        virtual bool IsSlabForCachedBlock( CachedBlockSlab^ slab, CachedBlockId^ cachedBlockId )
                        {
                            return this->target->IsSlabForCachedBlock( slab, cachedBlockId );
                        }

                        virtual ICachedBlockSlab^ GetSlab(  UInt64 offsetInBytes,
                                                            bool wait,
                                                            bool ignoreVerificationErrors, 
                                                            [Out] EsentErrorException^% ex )
                        {
                            return this->target->GetSlab( offsetInBytes, wait, ignoreVerificationErrors, ex );
                        }

                        virtual void VisitSlabs( ICachedBlockSlabManager::VisitSlab^ visitSlab )
                        {
                            return this->target->VisitSlabs( visitSlab );
                        }

                    private:

                        ICachedBlockSlabManager^ target;
                };

            }
        }
    }
}