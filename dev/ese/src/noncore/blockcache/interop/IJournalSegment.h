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
                /// Journal Segment.
                /// </summary>
                public interface class IJournalSegment : IDisposable
                {
                    /// <summary>
                    /// Gets the physical identity of the segment.
                    /// </summary>
                    /// <returns>The offset of the segment.</returns>
                    Int64 GetPhysicalId();

                    /// <summary>
                    /// Gets the current properties of the segment.
                    /// </summary>
                    /// <param name="segmentPosition">The position of the segment.</param>
                    /// <param name="uniqueId">The unique id of this segment.</param>
                    /// <param name="uniqueIdPrevious">The unique id of the previous segment.</param>
                    /// <param name="segmentPositionReplay">The position of the last known replay segment.</param>
                    /// <param name="segmentPositionDurable">The position of the last known durable segment.</param>
                    /// <param name="isSealed">Indicates that this segment is sealed and doesn't allow new appends.</param>
                    void GetProperties(
                        [Out] SegmentPosition% segmentPosition,
                        [Out] Int32% uniqueId,
                        [Out] Int32% uniqueIdPrevious,
                        [Out] SegmentPosition% segmentPositionReplay,
                        [Out] SegmentPosition% segmentPositionDurable,
                        [Out] bool% isSealed );

                    /// <summary>
                    /// Delegate used to visit a region in the segment.
                    /// </summary>
                    /// <param name="regionPosition">The position of the region to visit.</param>
                    /// <param name="region">The payload of the region to visit.</param>
                    /// <returns>True to continue visiting more regions.</returns>
                    delegate bool VisitRegion(
                        RegionPosition regionPosition,
                        ArraySegment<byte> region );

                    /// <summary>
                    /// Visits every region in the segment.
                    /// </summary>
                    /// <param name="visitRegion">The delegate to invoke to visit each region.</param>
                    void VisitRegions( VisitRegion^ visitRegion );

                    /// <summary>
                    /// Attempts to append a region to the segment containing a portion of the provided payload.
                    /// </summary>
                    /// <remarks>
                    /// The actual amount of the payload appended is returned.  This can be zero if the minimum payload
                    /// could not be appended.
                    /// </remarks>
                    /// <param name="payload">The payload as an array of byte arrays.</param>
                    /// <param name="minimumAppendSizeInBytes">The minimum amount of payload that must be appended.</param>
                    /// <param name="regionPosition">The position of the region appended.</param>
                    /// <param name="payloadAppendedInBytes">The actual amount of payload appended.</param>
                    void AppendRegion(
                        array<ArraySegment<byte>>^ payload,
                        Int32 minimumAppendSizeInBytes,
                        [Out] RegionPosition% regionPosition,
                        [Out] Int32% payloadAppendedInBytes );

                    /// <summary>
                    /// Delegate used to indicate that a segment is sealed.
                    /// </summary>
                    /// <param name="ex">An EsentErrorException or null if the seal was successful.</param>
                    /// <param name="segmentPosition">The position of the segment.</param>
                    delegate void Sealed(
                        EsentErrorException^ ex,
                        SegmentPosition segmentPosition );

                    /// <summary>
                    /// Closes the segment to further appends and writes it to storage.
                    /// </summary>
                    /// <param name="sealed">An optional delegate to invoke when the segment is sealed.</param>
                    void Seal( Sealed^ sealed );
                };
            }
        }
    }
}