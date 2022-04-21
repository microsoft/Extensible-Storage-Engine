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
                ref class FileSystemConfigurationRemotable : Remotable, IFileSystemConfiguration
                {
                    public:

                        FileSystemConfigurationRemotable( IFileSystemConfiguration^ target )
                            :   target( target )
                        {
                        }

                        ~FileSystemConfigurationRemotable() {}

                    public:

                        virtual TimeSpan AccessDeniedRetryPeriod()
                        {
                            return this->target->AccessDeniedRetryPeriod();
                        }

                        virtual Int32 MaxConcurrentIO()
                        {
                            return this->target->MaxConcurrentIO();
                        }

                        virtual Int32 MaxConcurrentBackgroundIO()
                        {
                            return this->target->MaxConcurrentBackgroundIO();
                        }

                        virtual TimeSpan HungIOThreshhold()
                        {
                            return this->target->HungIOThreshhold();
                        }

                        virtual int GrbitHungIOActions()
                        {
                            return this->target->GrbitHungIOActions();
                        }

                        virtual int MaxReadSizeInBytes()
                        {
                            return this->target->MaxReadSizeInBytes();
                        }

                        virtual int MaxWriteSizeInBytes()
                        {
                            return this->target->MaxWriteSizeInBytes();
                        }

                        virtual int MaxReadGapSizeInBytes()
                        {
                            return this->target->MaxReadGapSizeInBytes();
                        }

                        virtual int PermillageSmoothIo()
                        {
                            return this->target->PermillageSmoothIo();
                        }

                        virtual bool IsBlockCacheEnabled()
                        {
                            return this->target->IsBlockCacheEnabled();
                        }

                        virtual IBlockCacheConfiguration^ BlockCacheConfiguration()
                        {
                            return this->target->BlockCacheConfiguration();
                        }

                    private:

                        IFileSystemConfiguration^ target;
                };
            }
        }
    }
}