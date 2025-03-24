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
                /// Journal Segment Manager.
                /// </summary>
                public interface class IJournalSegmentManager : IDisposable
                {
                    /// <summary>
                    /// Gets the current properties of the set of segments.
                    /// </summary>
                    /// <param name="segmentPositionReplay">The position of the last known replay segment.</param>
                    /// <param name="segmentPositionDurable">The position of the last known durable segment.</param>
                    /// <param name="segmentPositionLast">The position of the last known segment.</param>
                    void GetProperties(
                        [Out] SegmentPosition% segmentPositionReplay,
                        [Out] SegmentPosition% segmentPositionDurable,
                        [Out] SegmentPosition% segmentPositionLast );

                    /// <summary>
                    /// Delegate used to visit a segment in the journal.
                    /// </summary>
                    /// <remarks>
                    /// If a particular segment cannot be loaded then the callback will be invoked with the failure and
                    /// the segment will not be provided.
                    /// </remarks>
                    /// <param name="segmentPosition">The position of the segment to visit.</param>
                    /// <param name="ex">An EsentErrorException or null if the segment is valid.</param>
                    /// <param name="journalSegment">The segment to visit.</param>
                    /// <returns>True to continue visiting more regions.</returns>
                    delegate bool VisitSegment(
                        SegmentPosition segmentPosition,
                        EsentErrorException^ ex,
                        IJournalSegment^ journalSegment );

                    /// <summary>
                    /// Visits every segment in the journal.
                    /// </summary>
                    /// <param name="visitSegment">The delegate to invoke to visit each segment.</param>
                    void VisitSegments( IJournalSegmentManager::VisitSegment^ visitSegment );

                    /// <summary>
                    /// Appends a new segment to the journal, reusing an existing segment if necessary.
                    /// </summary>
                    /// <remarks>
                    /// A sealed segment can become eligible for reuse if the provided sposReplay for the new segment
                    /// causes an existing segment to fall out of scope for replay.
                    ///
                    /// A sealed segment can also be reused directly to fixup the tail of the journal at mount time.
                    /// This segment must come after the last known durable segment.
                    ///
                    /// If a request would cause the journal to become 100% full then it will fail.
                    ///
                    /// If a request would cause the journal to fail to recover due to invalid metadata then it will fail.
                    ///
                    /// If a request would make sposReplay or sposDurable to move backwards then it will fail.
                    /// </remarks>
                    /// <param name="segmentPosition">The position of the segment to append.</param>
                    /// <param name="segmentPositionReplay">The position of the last known replay segment.</param>
                    /// <param name="segmentPositionDurable">The position of the last known durable segment.</param>
                    /// <returns>The segment.</returns>
                    IJournalSegment^ AppendSegment(
                        SegmentPosition segmentPosition,
                        SegmentPosition segmentPositionReplay,
                        SegmentPosition segmentPositionDurable );

                    /// <summary>
                    /// Causes all sealed segments to become durable.
                    /// </summary>
                    /// <remarks>
                    /// This call will block until these segments are durable.
                    /// </remarks>
                    void Flush();
                };
            }
        }
    }
}