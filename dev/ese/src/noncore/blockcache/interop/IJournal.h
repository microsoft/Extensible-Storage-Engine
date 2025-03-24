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
                /// Journal.
                /// </summary>
                public interface class IJournal : IDisposable
                {
                    /// <summary>
                    /// Gets the current properties of the set of segments.
                    /// </summary>
                    /// <param name="journalPositionReplay">The approximate position required for replay.</param>
                    /// <param name="journalPositionDurableForWriteBack">The approximate position of the last durable entry for which write back may occur.</param>
                    /// <param name="segmentPositionDurable">The approximate position of the last durable entry.</param>
                    void GetProperties(
                        [Out] JournalPosition% journalPositionReplay,
                        [Out] JournalPosition% journalPositionDurableForWriteBack,
                        [Out] JournalPosition% journalPositionDurable );

                    /// <summary>
                    /// Delegate used to visit an entry in the journal.
                    /// </summary>
                    /// <param name="journalPosition">The position of the entry to visit.</param>
                    /// <param name="entry">The payload of the entry to visit.</param>
                    /// <returns>True to continue visiting more entries.</returns>
                    delegate bool VisitEntry(
                        JournalPosition journalPosition,
                        ArraySegment<byte> entry );

                    /// <summary>
                    /// Visits every entry in the journal.
                    /// </summary>
                    /// <param name="visitSegment">The delegate to invoke to visit each entry.</param>
                    void VisitEntries( IJournal::VisitEntry^ visitEntry );

                    /// <summary>
                    /// Indicates that all journal entries newer than the specified entry are invalid and should be
                    /// overwritten.
                    /// </summary>
                    /// <remarks>
                    /// Note that any entries in the same segment as the invalidated entry and later will be
                    /// invalidated as well as any entry that is truncated by the invalidation of this segment.
                    /// </remarks>
                    /// <param name="journalPositionInvalidate">The journal position of the first invalid entry.</param>
                    void Repair( JournalPosition journalPositionInvalidate );

                    /// <summary>
                    /// Attempts to append an entry to the journal with the provided payload.
                    /// </summary>
                    /// <remarks>If there is not enough room in the journal to append the entry then the operation will
                    /// fail.  Truncate must be called with a sufficiently advanced replay pointer to resolve the
                    /// situation.
                    /// </remarks>
                    /// <param name="payload">The payload as an array of byte arrays.</param>
                    /// <param name="journalPosition">The position of the entry appended.</param>
                    JournalPosition AppendEntry( array<ArraySegment<byte>>^ payload );

                    /// <summary>
                    /// Causes all previously appended entries to become durable.
                    /// </summary>
                    /// <remarks>
                    /// This call will block until these entries are durable.
                    /// </remarks>
                    void Flush();

                    /// <summary>
                    /// Indicates that all journal entries older than the specified entry are no longer required.
                    /// </summary>
                    /// <param name="journalPositionReplay">The approximate position required for replay.</param>
                    void Truncate( JournalPosition journalPositionReplay );
                };
            }
        }
    }
}