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
                public ref class JournalSegmentBase : public Base<TM, TN, TW>, IJournalSegment
                {
                    public:

                        JournalSegmentBase( TM^ js ) : Base( js ) { }

                        JournalSegmentBase( TN* const pjs ) : Base( pjs ) {}

                        JournalSegmentBase( TN** const ppjs ) : Base( ppjs ) {}

                    public:

                        virtual Int64 GetPhysicalId();

                        virtual void GetProperties(
                            [Out] SegmentPosition% segmentPosition,
                            [Out] Int32% uniqueId,
                            [Out] Int32% uniqueIdPrevious,
                            [Out] SegmentPosition% segmentPositionReplay,
                            [Out] SegmentPosition% segmentPositionDurable,
                            [Out] bool% isSealed );

                        virtual void VisitRegions( IJournalSegment::VisitRegion^ visitRegion );

                        virtual void AppendRegion(
                            array<ArraySegment<byte>>^ payload,
                            Int32 minimumAppendSizeInBytes,
                            [Out] RegionPosition% regionPosition,
                            [Out] Int32% payloadAppendedInBytes );

                        virtual void Seal( IJournalSegment::Sealed^ sealed );
                };

                template< class TM, class TN, class TW >
                inline Int64 JournalSegmentBase<TM, TN, TW>::GetPhysicalId()
                {
                    ERR     err = JET_errSuccess;
                    QWORD   ib  = 0;

                    Call( Pi->ErrGetPhysicalId( &ib ) );

                    return ib;

                HandleError:
                    throw EseException( err );
                }

                template<class TM, class TN, class TW>
                inline void JournalSegmentBase<TM, TN, TW>::GetProperties(
                    [Out] SegmentPosition% segmentPosition, 
                    [Out] Int32% uniqueId, 
                    [Out] Int32% uniqueIdPrevious, 
                    [Out] SegmentPosition% segmentPositionReplay, 
                    [Out] SegmentPosition% segmentPositionDurable, 
                    [Out] bool% isSealed )
                {
                    ERR                 err             = JET_errSuccess;
                    ::SegmentPosition   spos            = ::sposInvalid;
                    DWORD               dwUniqueId      = 0;
                    DWORD               dwUniqueIdPrev  = 0;
                    ::SegmentPosition   sposReplay      = ::sposInvalid;
                    ::SegmentPosition   sposDurable     = ::sposInvalid;
                    BOOL                fSealed         = fFalse;

                    segmentPosition = SegmentPosition::Invalid;
                    uniqueId = 0;
                    uniqueIdPrevious = 0;
                    segmentPositionReplay = SegmentPosition::Invalid;
                    segmentPositionDurable = SegmentPosition::Invalid;
                    isSealed = false;

                    Call( Pi->ErrGetProperties( &spos, &dwUniqueId, &dwUniqueIdPrev, &sposReplay, &sposDurable, &fSealed ) );

                    segmentPosition = (SegmentPosition)spos;
                    uniqueId = dwUniqueId;
                    uniqueIdPrevious = dwUniqueIdPrev;
                    segmentPositionReplay = (SegmentPosition)sposReplay;
                    segmentPositionDurable = (SegmentPosition)sposDurable;
                    isSealed = fSealed ? true : false;

                    return;

                HandleError:
                    segmentPosition = SegmentPosition::Invalid;
                    uniqueId = 0;
                    uniqueIdPrevious = 0;
                    segmentPositionReplay = SegmentPosition::Invalid;
                    segmentPositionDurable = SegmentPosition::Invalid;
                    isSealed = false;
                    throw EseException( err );
                }

                template<class TM, class TN, class TW>
                inline void JournalSegmentBase<TM, TN, TW>::VisitRegions( IJournalSegment::VisitRegion^ visitRegion )
                {
                    ERR             err             = JET_errSuccess;
                    CVisitRegion*   pvisitregion    = NULL;

                    Alloc( pvisitregion = new CVisitRegion( visitRegion ) );

                    Call( Pi->ErrVisitRegions(  ::IJournalSegment::PfnVisitRegion( CVisitRegion::VisitRegion_ ),
                                                DWORD_PTR( pvisitregion ) ) );

                HandleError:
                    delete pvisitregion;
                    if ( err < JET_errSuccess )
                    {
                        throw EseException( err );
                    }
                }

                template<class TM, class TN, class TW>
                inline void JournalSegmentBase<TM, TN, TW>::AppendRegion(
                    array<ArraySegment<byte>>^ payload, 
                    Int32 minimumAppendSizeInBytes, 
                    RegionPosition% regionPosition, 
                    Int32% payloadAppendedInBytes )
                {
                    ERR                 err         = JET_errSuccess;
                    const size_t        cjb         = payload == nullptr ? 0 : payload->Length;
                    CJournalBuffer*     rgjb        = NULL;
                    ::RegionPosition    rpos        = ::rposInvalid;
                    DWORD               cbActual    = 0;

                    regionPosition = RegionPosition::Invalid;
                    payloadAppendedInBytes = 0;

                    if ( payload != nullptr )
                    {
                        Alloc( rgjb = new CJournalBuffer[ cjb + 1 ] );
                    }

                    for ( int ijb = 0; ijb < cjb; ijb++ )
                    {
                        const size_t    cb  = payload[ ijb ].Count;
                        BYTE*           rgb = NULL;

                        if ( payload[ ijb ].Array != nullptr )
                        {
                            Alloc( rgb = new BYTE[ cb + 1 ] );
                        }

                        rgjb[ ijb ] = CJournalBuffer( cb, rgb );

                        if ( cb )
                        {
                            pin_ptr<byte> rgbIn = &payload[ ijb ].Array[ payload[ ijb ].Offset ];
                            UtilMemCpy( rgb, (BYTE*)rgbIn, cb );
                        }
                    }

                    Call( Pi->ErrAppendRegion( cjb, rgjb, minimumAppendSizeInBytes, &rpos, &cbActual ) );

                    regionPosition = (RegionPosition)rpos;
                    payloadAppendedInBytes = cbActual;
                    
                HandleError:
                    if ( rgjb != NULL )
                    {
                        for ( int ijb = 0; ijb < cjb; ijb++ )
                        {
                            delete[] rgjb[ ijb ].Rgb();
                        }
                    }
                    delete[] rgjb;
                    if ( err < JET_errSuccess )
                    {
                        regionPosition = RegionPosition::Invalid;
                        payloadAppendedInBytes = 0;
                        throw EseException( err );
                    }
                }

                template<class TM, class TN, class TW>
                inline void JournalSegmentBase<TM, TN, TW>::Seal( IJournalSegment::Sealed^ sealed )
                {
                    ERR         err     = JET_errSuccess;
                    CSealed*    psealed = NULL;

                    if ( sealed != nullptr )
                    {
                        Alloc( psealed = new CSealed( sealed ) );
                    }

                    Call( Pi->ErrSeal(  sealed != nullptr ? ::IJournalSegment::PfnSealed( CSealed::Sealed_ ) : NULL,
                                        DWORD_PTR( psealed ) ) );

                    return;

                HandleError:
                    delete psealed;
                    throw EseException( err );
                }
            }
        }
    }
}
