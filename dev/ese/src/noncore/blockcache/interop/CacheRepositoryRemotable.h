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
                ref class CacheRepositoryRemotable : MarshalByRefObject, ICacheRepository
                {
                    public:

                        CacheRepositoryRemotable( ICacheRepository^ target )
                            :   target( target )
                        {
                        }

                        ~CacheRepositoryRemotable() {}

                    public:

                        virtual ICache^ Open(
                            IFileSystemFilter^ fsf,
                            IFileSystemConfiguration^ fsconfig,
                            ICacheConfiguration^ cconfig )
                        {
                            return this->target->Open( fsf, fsconfig, cconfig );
                        }

                        virtual ICache^ OpenById(
                            IFileSystemFilter^ fsf,
                            IFileSystemConfiguration^ fsconfig,
                            IBlockCacheConfiguration^ bcconfig,
                            VolumeId volumeid,
                            FileId fileid,
                            Guid^ guid )
                        {
                            return this->target->OpenById( fsf, fsconfig, bcconfig, volumeid, fileid, guid );
                        }

                    private:

                        ICacheRepository^ target;
                };
            }
        }
    }
}
