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
                ref class JournalSegmentManagerRemotable : MarshalByRefObject, IJournalSegmentManager
                {
                    public:

                        JournalSegmentManagerRemotable( IJournalSegmentManager^ target )
                            :   target( target )
                        {
                        }

                        ~JournalSegmentManagerRemotable() {}

                    public:

                        virtual void GetProperties(
                            [Out] SegmentPosition% segmentPositionFirst,
                            [Out] SegmentPosition% segmentPositionReplay,
                            [Out] SegmentPosition% segmentPositionDurable,
                            [Out] SegmentPosition% segmentPositionLast,
                            [Out] SegmentPosition% segmentPositionFull )
                        {
                            return this->target->GetProperties(
                                segmentPositionFirst, 
                                segmentPositionReplay, 
                                segmentPositionDurable, 
                                segmentPositionLast, 
                                segmentPositionFull );
                        }

                        virtual void VisitSegments( IJournalSegmentManager::VisitSegment^ visitSegment )
                        {
                            return this->target->VisitSegments( visitSegment );
                        }

                        virtual IJournalSegment^ AppendSegment(
                            SegmentPosition segmentPosition,
                            SegmentPosition segmentPositionReplay,
                            SegmentPosition segmentPositionDurable )
                        {
                            return this->target->AppendSegment( segmentPosition, segmentPositionReplay, segmentPositionDurable );
                        }

                        virtual void Flush()
                        {
                            return this->target->Flush();
                        }

                    private:

                        IJournalSegmentManager^ target;
                };
            }
        }
    }
}