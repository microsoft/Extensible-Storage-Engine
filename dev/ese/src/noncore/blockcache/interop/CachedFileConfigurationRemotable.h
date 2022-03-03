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
                ref class CachedFileConfigurationRemotable : MarshalByRefObject, ICachedFileConfiguration
                {
                    public:

                        CachedFileConfigurationRemotable( ICachedFileConfiguration^ target )
                            :   target( target )
                        {
                        }

                        ~CachedFileConfigurationRemotable() {}

                    public:

                        virtual bool IsCachingEnabled()
                        {
                            return this->target->IsCachingEnabled();
                        }

                        virtual String^ CachingFilePath()
                        {
                            return this->target->CachingFilePath();
                        }

                        virtual int BlockSize()
                        {
                            return this->target->BlockSize();
                        }

                        virtual int MaxConcurrentBlockWriteBacks()
                        {
                            return this->target->MaxConcurrentBlockWriteBacks();
                        }

                        virtual int CacheTelemetryFileNumber()
                        {
                            return this->target->CacheTelemetryFileNumber();
                        }

                        virtual int PinnedHeaderSizeInBytes()
                        {
                            return this->target->PinnedHeaderSizeInBytes();
                        }

                    private:

                        ICachedFileConfiguration^ target;
                };
            }
        }
    }
}