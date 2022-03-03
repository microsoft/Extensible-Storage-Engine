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
                /// Cached Block Slab Manager Interface.
                /// </summary>
                public interface class ICachedBlockSlabManager : IDisposable
                {
                    /// <summary>
                    /// Gets exclusive access to the slab that may contain a cached block.
                    /// </summary>
                    /// <remarks>
                    /// The slab must be released to allow future access.
                    /// </remarks>
                    /// <param name="cachedBlockId">The unique id of the cached block.</param>
                    /// <returns>A slab with exclusive access.</returns>
                    ICachedBlockSlab^ GetSlab( CachedBlockId^ cachedBlockId );

                    /// <summary>
                    /// Indicates if the slab can hold a given cached block.
                    /// </summary>
                    /// <param name="slab">The slab.</param>
                    /// <param name="cachedBlockId">The unique id of the cached block.</param>
                    /// <returns>True if the slab can hold the given cached block.</returns>
                    bool IsSlabForCachedBlock( CachedBlockSlab^ slab, CachedBlockId^ cachedBlockId );

                    /// <summary>
                    /// Gets exclusive access to a particular slab by its physical id.
                    /// </summary>
                    /// <remarks>
                    /// The slab must be released to allow future access.
                    /// </remarks>
                    /// <param name="offsetInBytes">The physical location of the slab.</param>
                    /// <param name="ignoreVerificationErrors">Indicates if the operation should wait to acquire the slab.</param>
                    /// <param name="ignoreVerificationErrors">Indicates if the operation should succeed even if verification errors are encountered.</param>
                    /// <param name="ex">The verification error observed, if any.</param>
                    /// <returns>A slab with exclusive access.</returns>
                    ICachedBlockSlab^ GetSlab(  UInt64 offsetInBytes,
                                                bool wait,
                                                bool ignoreVerificationErrors,
                                                [Out] EsentErrorException^% ex );

                    /// <summary>
                    /// Delegate used to visit a slab in the cache.
                    /// </summary>
                    /// <remarks>
                    /// If a particular slab fails verification then that error will be provided.
                    /// 
                    /// The delegate must return true to continue visiting more slabs.
                    /// </remarks>
                    /// <param name="offsetInBytes">The physical location of the slab to visit.</param>
                    /// <param name="ex">An EsentErrorException or null if the slab is valid.</param>
                    /// <param name="slab">The slab to visit.</param>
                    /// <returns>True to continue visiting more slabs.</returns>
                    delegate bool VisitSlab( UInt64 offsetInBytes, EsentErrorException^ ex, ICachedBlockSlab^ slab );

                    /// <summary>
                    /// Visits every slab in the cache.
                    /// </summary>
                    /// <param name="visitSlab">The delegate to invoke to visit each slab.</param>
                    void VisitSlabs( ICachedBlockSlabManager::VisitSlab^ visitSlab );
                };
            }
        }
    }
}