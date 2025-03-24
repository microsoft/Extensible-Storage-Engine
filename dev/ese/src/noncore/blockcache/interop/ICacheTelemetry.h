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
                /// Cache Telemetery Interface.
                /// </summary>
                public interface class ICacheTelemetry : IDisposable
                {
                    /// <summary>
                    /// File Number.
                    /// </summary>
                    enum class FileNumber : int
                    {
                        /// <summary>
                        /// An invalid FileNumber.
                        /// </summary>
                        Invalid = -1,
                    };

                    /// <summary>
                    /// Block Number.
                    /// </summary>
                    enum class BlockNumber : int
                    {
                        /// <summary>
                        /// An invalid BlockNumber.
                        /// </summary>
                        Invalid = -1,
                    };

                    /// <summary>
                    /// Reports a cache miss for a file/block and if that block should be cached.
                    /// </summary>
                    /// <param name="filenumber">The file number of the block.</param>
                    /// <param name="blocknumber">The number of the block.</param>
                    /// <param name="isRead">Indicates if this cache miss is for a read.</param>
                    /// <param name="cacheIfPossible">Indicates if the block should be cached if possible.</param>
                    void Miss( FileNumber filenumber, BlockNumber blocknumber, bool isRead, bool cacheIfPossible );

                    /// <summary>
                    /// Reports a cache hit for a file/block and if that block should be cached.
                    /// </summary>
                    /// <param name="filenumber">The file number of the block.</param>
                    /// <param name="blocknumber">The number of the block.</param>
                    /// <param name="isRead">Indicates if this cache miss is for a read.</param>
                    /// <param name="cacheIfPossible">Indicates if the block should be cached if possible.</param>
                    void Hit( FileNumber filenumber, BlockNumber blocknumber, bool isRead, bool cacheIfPossible );

                    /// <summary>
                    /// Reports an update of a file/block.
                    /// </summary>
                    /// <param name="filenumber">The file number of the block.</param>
                    /// <param name="blocknumber">The number of the block.</param>
                    void Update( FileNumber filenumber, BlockNumber blocknumber );

                    /// <summary>
                    /// Reports a write of a file/block and if that write was due to limited resources.
                    /// </summary>
                    /// <param name="filenumber">The file number of the block.</param>
                    /// <param name="blocknumber">The number of the block.</param>
                    /// <param name="replacementPolicy">Indicates if the write was due to limited resources.</param>
                    void Write( FileNumber filenumber, BlockNumber blocknumber, bool replacementPolicy );

                    /// <summary>
                    /// Reports the removal of a file/block from the cache and if that removal was due to limited resources.
                    /// </summary>
                    /// <param name="filenumber">The file number of the block.</param>
                    /// <param name="blocknumber">The number of the block.</param>
                    /// <param name="replacementPolicy">Indicates if the write was due to limited resources.</param>
                    void Evict( FileNumber filenumber, BlockNumber blocknumber, bool replacementPolicy );
                };
            }
        }
    }
}