// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                ref class JournalSegmentRemotable : MarshalByRefObject, IJournalSegment
                {
                    public:

                        JournalSegmentRemotable( IJournalSegment^ target )
                            :   target( target )
                        {
                        }

                        ~JournalSegmentRemotable() {}

                    public:

                        virtual Int64 GetPhysicalId()
                        {
                            return this->target->GetPhysicalId();
                        }

                        virtual void GetProperties(
                            [Out] SegmentPosition% segmentPosition,
                            [Out] Int32% uniqueId,
                            [Out] Int32% uniqueIdPrevious,
                            [Out] SegmentPosition% segmentPositionReplay,
                            [Out] SegmentPosition% segmentPositionDurable,
                            [Out] bool% isSealed )
                        {
                            return this->target->GetProperties( segmentPosition, uniqueId, uniqueIdPrevious, segmentPositionReplay, segmentPositionDurable, isSealed );
                        }

                        virtual void VisitRegions( IJournalSegment::VisitRegion^ visitRegion )
                        {
                            return this->target->VisitRegions( visitRegion );
                        }

                        virtual RegionPosition AppendRegion(
                            array<ArraySegment<byte>>^ payload,
                            Int32 minimumAppendSizeInBytes,
                            [Out] RegionPosition% regionPositionEnd,
                            [Out] Int32% payloadAppendedInBytes )
                        {
                            return this->target->AppendRegion( payload, minimumAppendSizeInBytes, regionPositionEnd, payloadAppendedInBytes );
                        }

                        virtual void Seal( IJournalSegment::Sealed^ sealed )
                        {
                            return this->target->Seal( sealed );
                        }

                    private:

                        IJournalSegment^ target;
                };
            }
        }
    }
}
