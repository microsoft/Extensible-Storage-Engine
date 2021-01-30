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
                public interface class IJournalSegmentManager : IDisposable
                {
                    void GetProperties(
                        [Out] SegmentPosition% segmentPositionReplay,
                        [Out] SegmentPosition% segmentPositionDurable,
                        [Out] SegmentPosition% segmentPositionLast );

                    delegate bool VisitSegment(
                        SegmentPosition segmentPosition,
                        EsentErrorException^ ex,
                        IJournalSegment^ journalSegment );

                    void VisitSegments( IJournalSegmentManager::VisitSegment^ visitSegment );

                    IJournalSegment^ AppendSegment(
                        SegmentPosition segmentPosition,
                        SegmentPosition segmentPositionReplay,
                        SegmentPosition segmentPositionDurable );

                    void Flush();
                };
            }
        }
    }
}
