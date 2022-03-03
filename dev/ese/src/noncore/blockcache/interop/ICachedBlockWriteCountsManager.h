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
                /// Cached Block Write Counts Manager Interface.
                /// </summary>
                public interface class ICachedBlockWriteCountsManager : IDisposable
                {
                    /// <summary>
                    /// Gets the write count at a given index corresponding to a cached block chunk.
                    /// </summary>
                    /// <param name="index">The cached block index.</param>
                    /// <returns>The write count.</returns>
                    CachedBlockWriteCount GetWriteCount( Int64 index );

                    /// <summary>
                    /// Sets the write count at a given index corresponding to a cached block chunk.
                    /// </summary>
                    /// <param name="index">The cached block index.</param>
                    /// <param name="writeCount">The write count.</param>
                    void SetWriteCount( Int64 index, CachedBlockWriteCount writeCount );

                    /// <summary>
                    /// Writes the write counts to storage.
                    /// </summary>
                    void Save();
                };
            }
        }
    }
}