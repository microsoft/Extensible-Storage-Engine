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
                /// Cached Block Slab Interface.
                /// </summary>
                public interface class ICachedBlockSlab : IDisposable
                {
                    /// <summary>
                    /// Gets the physical identity of the slab.
                    /// </summary>
                    /// <returns>The physical identity of the slab.</returns>
                    UInt64 GetPhysicalId();

                    /// <summary>
                    /// Gets the next available slot for caching the existing data for a given cached block.
                    /// </summary>
                    /// <remarks>
                    /// The request will fail if the block is already cached.
                    /// 
                    /// The request can fail if there is no room to hold the new image of the cached block.
                    /// 
                    /// NOTE:  This operation is a function of the current state of the entire slab.  The output will
                    /// not change until the slot is actually consumed with UpdateSlot.
                    /// </remarks>
                    /// <param name="cachedBlockId">The unique id of the cached block.</param>
                    /// <param name="data">The data to cache.</param>
                    /// <returns>The cache slot to be updated.</returns>
                    CachedBlockSlot^ GetSlotForCache( CachedBlockId^ cachedBlockId, MemoryStream^ data );

                    /// <summary>
                    /// Gets the next available slot for writing a given cached block.
                    /// </summary>
                    /// <remarks>
                    /// The request can fail if there is no room to hold the new image of the cached block.
                    /// 
                    /// If the buffer is not provided then the slot will be updated without updating the cluster.
                    /// 
                    /// NOTE:  This operation is a function of the current state of the entire slab.  The output will
                    /// not change until the slot is actually consumed with UpdateSlot.
                    /// </remarks>
                    /// <param name="cachedBlockId">The unique id of the cached block.</param>
                    /// <param name="data">The data to cache.</param>
                    /// <returns>The cache slot to be updated.</returns>
                    CachedBlockSlot^ GetSlotForWrite( CachedBlockId^ cachedBlockId, MemoryStream^ data );

                    /// <summary>
                    /// Gets the slot containing the cluster of the given cached block.
                    /// </summary>
                    /// <remarks>
                    /// The request will fail if the slab doesn't contain the cached block.
                    /// 
                    /// UpdateSlot must be called to save the update to the block's access statistics.
                    /// </remarks>
                    /// <param name="cachedBlockId">The unique id of the cached block.</param>
                    /// <returns>The cache slot to be updated.</returns>
                    CachedBlockSlot^ GetSlotForRead( CachedBlockId^ cachedBlockId );

                    /// <summary>
                    /// Delegate used to indicate that a cluster has been written.
                    /// </summary>
                    /// <param name="ex">An EsentErrorException or null if the write was successful.</param>
                    delegate void ClusterWritten( EsentErrorException^ ex );

                    /// <summary>
                    /// Delegate used to indicate that a cluster IO has been started.
                    /// </summary>
                    delegate void ClusterHandoff();

                    /// <summary>
                    /// Writes the cluster and the state for the given cached block.
                    /// </summary>
                    /// <remarks>
                    /// The expected call sequence is:
                    /// -  GetSlotForCache or GetSlotForWrite
                    /// -  UpdateSlot
                    /// -  WriteCluster
                    /// 
                    /// The request will fail if the cluster data doesn't match the state.
                    /// 
                    /// The request will fail if it would cause an illegal state change in the slab.
                    /// </remarks>
                    /// <param name="slot">The slot with the cluster to write.</param>
                    /// <param name="data">The data to write.</param>
                    /// <param name="clusterWritten">A cluster written delegate.</param>
                    /// <param name="clusterHandoff">A cluster handoff delegate.</param>
                    void WriteCluster(
                        CachedBlockSlot^ slot, 
                        MemoryStream^ data,
                        ICachedBlockSlab::ClusterWritten^ clusterWritten,
                        ICachedBlockSlab::ClusterHandoff^ clusterHandoff );

                    /// <summary>
                    /// Updates the state for the given cached block.
                    /// </summary>
                    /// <remarks>
                    /// The request will fail if it would cause an illegal state change in the slab.
                    /// </remarks>
                    /// <param name="slot">The slot to be updated.</param>
                    void UpdateSlot( CachedBlockSlot^ slot );

                    /// <summary>
                    /// Delegate used to indicate that a cluster has been read.
                    /// </summary>
                    /// <param name="ex">An EsentErrorException or null if the read was successful.</param>
                    delegate void ClusterRead( EsentErrorException^ ex );

                    /// <summary>
                    /// Reads the cluster of the given cached block.
                    /// </summary>
                    /// <remarks>
                    /// The request will not verify the contents of the cluster.  This check is provided by
                    /// VerifyCluster.
                    /// </remarks>
                    /// <param name="slot">The slot with the cluster to read.</param>
                    /// <param name="data">The buffer for the data to read.</param>
                    /// <param name="clusterRead">A cluster read delegate.</param>
                    /// <param name="clusterHandoff">A cluster handoff delegate.</param>
                    void ReadCluster(
                        CachedBlockSlot^ slot,
                        MemoryStream^ data,
                        ICachedBlockSlab::ClusterRead^ clusterRead,
                        ICachedBlockSlab::ClusterHandoff^ clusterHandoff );

                    /// <summary>
                    /// Verifies that the cluster data matches the cluster state.
                    /// </summary>
                    /// <remarks>
                    /// The request will fail if the cluster data doesn't match the state.
                    /// </remarks>
                    /// <param name="slot">The slot with the cluster to verify.</param>
                    /// <param name="data">The data to verify.</param>
                    void VerifyCluster( CachedBlockSlot^ slot, MemoryStream^ data );

                    /// <summary>
                    /// Delegate used to visit and optionally update a slot in the slab.
                    /// </summary>
                    /// <remarks>
                    /// The new state of the slot may be the same as the current state of the slot if the requested
                    /// change is not possible.
                    /// 
                    /// The delegate must return true to continue visiting more slots.
                    /// </remarks>
                    /// <param name="currentSlotState">The current state of the slot.</param>
                    /// <param name="newSlot">The new state of the slot.</param>
                    /// <returns>True to continue visiting more slots.</returns>
                    delegate bool ConsiderUpdate(
                        CachedBlockSlotState^ currentSlotState,
                        CachedBlockSlot^ newSlot );

                    /// <summary>
                    /// Provides a mechanism for invalidating slots in the slab.
                    /// </summary>
                    /// <remarks>
                    /// All slots will be visited.  Any slot may be invalidated so use with caution.
                    /// 
                    /// A slot will only be invalidated by calling UpdateSlot on the slot as it is visited.
                    /// </remarks>
                    /// <param name="considerUpdate">The delegate to invoke to consider updating each slot.</param>
                    void InvalidateSlots( ICachedBlockSlab::ConsiderUpdate^ considerUpdate );

                    /// <summary>
                    /// Provides a mechanism for marking slots as written back in the slab.
                    /// </summary>
                    /// <remarks>
                    /// All slots will be visited.  Any slot may have its dirty flag cleared.
                    /// 
                    /// A slot will only be marked as written back by calling UpdateSlot on the slot as it is visited.
                    /// </remarks>
                    /// <param name="considerUpdate">The delegate to invoke to consider updating each slot.</param>
                    void CleanSlots( ICachedBlockSlab::ConsiderUpdate^ considerUpdate );

                    /// <summary>
                    /// Provides a mechanism to evicting slots in the slab.
                    /// </summary>
                    /// <remarks>
                    /// The candidate slots will be visited in least useful to most useful order to facilitate batch
                    /// evict.
                    /// 
                    /// All slots will be visited.  Slots may contain invalid data, valid data, or valid data that
                    /// needs to be written back.  Only slots with valid data that does not need to be written back can
                    /// be invalidated.
                    /// 
                    /// A slot will only be evicted by calling UpdateSlot on the slot as it is visited.
                    /// </remarks>
                    /// <param name="considerUpdate">The delegate to invoke to consider updating each slot.</param>
                    void EvictSlots( ICachedBlockSlab::ConsiderUpdate^ considerUpdate );

                    /// <summary>
                    /// Delegate used to visit a slot in the slab.
                    /// </summary>
                    /// <remarks>
                    /// The delegate must return true to continue visiting more slots.
                    /// </remarks>
                    /// <param name="ex">An EsentErrorException or null if the chunk containing the slot is valid.</param>
                    /// <param name="acceptedSlot">The accepted state of the slot.</param>
                    /// <param name="currentSlotState">The current state of the slot.</param>
                    /// <returns>True to continue visiting more slots.</returns>
                    delegate bool VisitSlot(
                        EsentErrorException^ ex,
                        CachedBlockSlot^ acceptedSlot,
                        CachedBlockSlotState^ currentSlotState );

                    /// <summary>
                    /// Visits every slot in the slab.
                    /// </summary>
                    /// <param name="visitSlot">The delegate to invoke to visit each slot.</param>
                    void VisitSlots( ICachedBlockSlab::VisitSlot^ visitSlot );

                    /// <summary>
                    /// Indicates if the slab has been updated since it was acquired.
                    /// </summary>
                    /// <returns>True if the slab has been updated since it was acquired.</returns>
                    bool IsUpdated();

                    /// <summary>
                    /// Commits any updates made to the slab since it was acquired.
                    /// </summary>
                    /// <remarks>
                    /// If the slab is released before AcceptUpdates() then any updates made since the slab was acquired
                    /// will be lost.
                    /// 
                    /// The request will fail if the process to cache data was started with GetSlotForCache() or
                    /// GetSlotForWrite() and the data was not written via WriteCluster().
                    /// </remarks>
                    void AcceptUpdates();

                    /// <summary>
                    /// Abandons any updates made to the slab since it was acquired.
                    /// </summary>
                    void RevertUpdates();

                    /// <summary>
                    /// Indicates if the slab contains changes that need to be written back to storage via Save.
                    /// </summary>
                    /// <returns>True if the slab contains changes that need to be written back to storage via ErrSave.</returns>
                    bool IsDirty();

                    /// <summary>
                    /// Delegate used to indicate that a slab is saved.
                    /// </summary>
                    /// <param name="ex">An EsentErrorException or null if the save was successful.</param>
                    delegate void SlabSaved( EsentErrorException^ ex );

                    /// <summary>
                    /// Writes the slab state to storage.
                    /// </summary>
                    /// <remarks>
                    /// Any updates that have not been accepted since the slab was acquired via AcceptUpdates() will
                    /// not be saved.
                    /// </remarks>
                    /// <param name="slabSaved"></param>
                    void Save( ICachedBlockSlab::SlabSaved^ slabSaved );
                };
            }
        }
    }
}