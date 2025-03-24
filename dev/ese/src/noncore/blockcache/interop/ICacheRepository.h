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
                /// <summary>
                /// Cache Repository Interface.
                /// </summary>
                public interface class ICacheRepository : IDisposable
                {
                    /// <summary>
                    /// Gets the Cache for a given Cache Configuration.
                    /// </summary>
                    /// <param name="fsf>The File System Filter to use when opening the Cache.</param>
                    /// <param name="fsconfig>The File System Configuration to use when opening the Cache.</param>
                    /// <param name="cconf>The Cache Configuration.</param>
                    /// <returns>The Cache for a given Cache Configuration.</returns>
                    ICache^ Open(
                        IFileSystemFilter^ fsf, 
                        IFileSystemConfiguration^ fsconfig,
                        ICacheConfiguration^ cconf );

                    /// <summary>
                    /// Gets an existing Cache by its physical identity.
                    /// </summary>
                    /// <param name="fsf>The File System Filter to use when opening the Cache.</param>
                    /// <param name="fsconfig>The File System Configuration to use when opening the Cache.</param>
                    /// <param name="bcconfig>The Block Cache Configuration to use when opening the Cache.</param>
                    /// <param name="volumeid>The volume id of the cache.</param>
                    /// <param name="fileid>The file id of the cache.</param>
                    /// <param name="guid>The guid of the cache.</param>
                    /// <returns>The Cache for a given Cache Configuration.</returns>
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