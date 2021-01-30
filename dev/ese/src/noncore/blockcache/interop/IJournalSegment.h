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
                public interface class IJournalSegment : IDisposable
                {
                    Int64 GetPhysicalId();

                    void GetProperties(
                        [Out] SegmentPosition% segmentPosition,
                        [Out] Int32% uniqueId,
                        [Out] Int32% uniqueIdPrevious,
                        [Out] SegmentPosition% segmentPositionReplay,
                        [Out] SegmentPosition% segmentPositionDurable,
                        [Out] bool% isSealed );

                    delegate bool VisitRegion(
                        RegionPosition regionPosition,
                        ArraySegment<byte> region );

                    void VisitRegions( VisitRegion^ visitRegion );

                    void AppendRegion(
                        array<ArraySegment<byte>>^ payload,
                        Int32 minimumAppendSizeInBytes,
                        [Out] RegionPosition% regionPosition,
                        [Out] Int32% payloadAppendedInBytes );

                    delegate void Sealed(
                        EsentErrorException^ ex,
                        SegmentPosition segmentPosition );

                    void Seal( Sealed^ sealed );
                };
            }
        }
    }
}
