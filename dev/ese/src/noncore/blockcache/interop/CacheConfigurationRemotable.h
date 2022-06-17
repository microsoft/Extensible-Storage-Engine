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
                ref class CacheConfigurationRemotable : Remotable, ICacheConfiguration
                {
                    public:

                        CacheConfigurationRemotable( ICacheConfiguration^ target )
                            :   target( target )
                        {
                        }

                        ~CacheConfigurationRemotable() {}

                    public:

                        virtual bool IsCacheEnabled()
                        {
                            return this->target->IsCacheEnabled();
                        }

                        virtual Guid CacheType()
                        {
                            return this->target->CacheType();
                        }

                        virtual String^ Path()
                        {
                            return this->target->Path();
                        }

                        virtual Int64 MaximumSize()
                        {
                            return this->target->MaximumSize();
                        }

                        virtual double PercentWrite()
                        {
                            return this->target->PercentWrite();
                        }

                        virtual Int64 JournalSegmentsMaximumSize()
                        {
                            return this->target->JournalSegmentsMaximumSize();
                        }

                        virtual double PercentJournalSegmentsInUse()
                        {
                            return this->target->PercentJournalSegmentsInUse();
                        }

                        virtual Int64 JournalSegmentsMaximumCacheSize()
                        {
                            return this->target->JournalSegmentsMaximumCacheSize();
                        }

                        virtual Int64 JournalClustersMaximumSize()
                        {
                            return this->target->JournalClustersMaximumSize();
                        }

                        virtual Int64 CachingFilePerSlab()
                        {
                            return this->target->CachingFilePerSlab();
                        }

                        virtual Int64 CachedFilePerSlab()
                        {
                            return this->target->CachedFilePerSlab();
                        }

                        virtual Int64 SlabMaximumCacheSize()
                        {
                            return this->target->SlabMaximumCacheSize();
                        }

                        virtual bool IsAsyncWriteBackEnabled()
                        {
                            return this->target->IsAsyncWriteBackEnabled();
                        }

                        virtual Int32 MaxConcurrentIODestage()
                        {
                            return this->target->MaxConcurrentIODestage();
                        }

                        virtual Int32 MaxConcurrentIOSizeDestage()
                        {
                            return this->target->MaxConcurrentIOSizeDestage();
                        }

                    private:

                        ICacheConfiguration^ target;
                };
            }
        }
    }
}
