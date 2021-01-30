// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

#undef interface

#pragma managed

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                public interface class ICacheRepository : IDisposable
                {
                    ICache^ Open(
                        IFileSystemFilter^ fsf, 
                        IFileSystemConfiguration^ fsconfig,
                        ICacheConfiguration^ cconf );

                    ICache^ OpenById(
                        IFileSystemFilter^ fsf,
                        IFileSystemConfiguration^ fsconfig,
                        IBlockCacheConfiguration^ bcconfig,
                        VolumeId volumeid,
                        FileId fileid,
                        Guid^ guid );
                };
            }
        }
    }
}
