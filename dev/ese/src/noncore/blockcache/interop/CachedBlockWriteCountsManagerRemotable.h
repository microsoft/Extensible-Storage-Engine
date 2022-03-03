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
                ref class CachedBlockWriteCountsManagerRemotable : MarshalByRefObject, ICachedBlockWriteCountsManager
                {
                    public:

                        CachedBlockWriteCountsManagerRemotable( ICachedBlockWriteCountsManager^ target )
                            :   target( target )
                        {
                        }

                        ~CachedBlockWriteCountsManagerRemotable() {}

                    public:

                        virtual CachedBlockWriteCount GetWriteCount( Int64 index )
                        {
                            return this->target->GetWriteCount( index );
                        }

                        virtual void SetWriteCount( Int64 index, CachedBlockWriteCount writeCount )
                        {
                            return this->target->SetWriteCount( index, writeCount );
                        }

                        virtual void Save()
                        {
                            return this->target->Save();
                        }

                    private:

                        ICachedBlockWriteCountsManager^ target;
                };
            }
        }
    }
}