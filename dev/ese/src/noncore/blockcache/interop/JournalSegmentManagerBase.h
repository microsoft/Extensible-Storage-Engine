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
                template< class TM, class TN, class TW >
                public ref class JournalSegmentManagerBase : public Base<TM, TN, TW>, IJournalSegmentManager
                {
                    public:

                        JournalSegmentManagerBase( TM^ jsm ) : Base( jsm ) { }

                        JournalSegmentManagerBase( TN** const ppjsm ) : Base( ppjsm ) {}

                    public:

                        virtual void GetProperties(
                            [Out] SegmentPosition% segmentPositionFirst,
                            [Out] SegmentPosition% segmentPositionReplay,
                            [Out] SegmentPosition% segmentPositionDurable,
                            [Out] SegmentPosition% segmentPositionLast,
                            [Out] SegmentPosition% segmentPositionFull );

                        virtual void VisitSegments( IJournalSegmentManager::VisitSegment^ visitSegment );

                        virtual IJournalSegment^ AppendSegment(
                            SegmentPosition segmentPosition,
                            SegmentPosition segmentPositionReplay,
                            SegmentPosition segmentPositionDurable );

                        virtual void Flush();
                };

                template< class TM, class TN, class TW >
                inline void JournalSegmentManagerBase<TM, TN, TW>::GetProperties(
                    [Out] SegmentPosition% segmentPositionFirst,
                    [Out] SegmentPosition% segmentPositionReplay,
                    [Out] SegmentPosition% segmentPositionDurable,
                    [Out] SegmentPosition% segmentPositionLast,
                    [Out] SegmentPosition% segmentPositionFull )
                {
                    ERR                 err         = JET_errSuccess;
                    ::SegmentPosition   sposFirst   = ::sposInvalid;
                    ::SegmentPosition   sposReplay  = ::sposInvalid;
                    ::SegmentPosition   sposDurable = ::sposInvalid;
                    ::SegmentPosition   sposLast    = ::sposInvalid;
                    ::SegmentPosition   sposFull    = ::sposInvalid;

                    segmentPositionFirst = SegmentPosition::Invalid;
                    segmentPositionReplay = SegmentPosition::Invalid;
                    segmentPositionDurable = SegmentPosition::Invalid;
                    segmentPositionLast = SegmentPosition::Invalid;
                    segmentPositionFull = SegmentPosition::Invalid;

                    Call( Pi->ErrGetProperties( &sposFirst, &sposReplay, &sposDurable, &sposLast, &sposFull ) );

                    segmentPositionFirst = (SegmentPosition)sposFirst;
                    segmentPositionReplay = (SegmentPosition)sposReplay;
                    segmentPositionDurable = (SegmentPosition)sposDurable;
                    segmentPositionLast = (SegmentPosition)sposLast;
                    segmentPositionFull = (SegmentPosition)sposFull;

                    return;

                HandleError:
                    segmentPositionFirst = SegmentPosition::Invalid;
                    segmentPositionReplay = SegmentPosition::Invalid;
                    segmentPositionDurable = SegmentPosition::Invalid;
                    segmentPositionLast = SegmentPosition::Invalid;
                    segmentPositionFull = SegmentPosition::Invalid;
                    throw EseException( err );
                }

                template<class TM, class TN, class TW>
                inline void JournalSegmentManagerBase<TM, TN, TW>::VisitSegments(
                    IJournalSegmentManager::VisitSegment^ visitSegment )
                {
                    ERR                     err                 = JET_errSuccess;
                    VisitSegmentInverse^    visitSegmentInverse = nullptr;

                    if ( visitSegment != nullptr )
                    {
                        visitSegmentInverse = gcnew VisitSegmentInverse( visitSegment );
                    }

                    Call( Pi->ErrVisitSegments( visitSegmentInverse == nullptr ? NULL : visitSegmentInverse->PfnVisitSegment,
                                                visitSegmentInverse == nullptr ? NULL : visitSegmentInverse->KeyVisitSegment ) );

                HandleError:
                    delete visitSegmentInverse;
                    if ( err < JET_errSuccess )
                    {
                        throw EseException( err );
                    }
                }

                template<class TM, class TN, class TW>
                inline IJournalSegment^ JournalSegmentManagerBase<TM, TN, TW>::AppendSegment(
                    SegmentPosition segmentPosition,
                    SegmentPosition segmentPositionReplay,
                    SegmentPosition segmentPositionDurable )
                {
                    ERR                 err = JET_errSuccess;
                    ::IJournalSegment*  pjs = NULL;

                    Call( Pi->ErrAppendSegment( (::SegmentPosition)segmentPosition,
                                                (::SegmentPosition)segmentPositionReplay,
                                                (::SegmentPosition)segmentPositionDurable,
                                                &pjs ) );

                    return pjs ? gcnew JournalSegment( &pjs ) : nullptr;

                HandleError:
                    delete pjs;
                    throw EseException( err );
                }

                template<class TM, class TN, class TW>
                inline void JournalSegmentManagerBase<TM, TN, TW>::Flush()
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrFlush() );

                    return;

                HandleError:
                    throw EseException( err );
                }
            }
        }
    }
}
