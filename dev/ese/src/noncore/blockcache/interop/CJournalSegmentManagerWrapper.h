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
                template< class TM, class TN >
                class CJournalSegmentManagerWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CJournalSegmentManagerWrapper( TM^ jsm ) : CWrapper( jsm ) { }

                    public:

                        ERR ErrGetProperties(   _Out_opt_ ::SegmentPosition* const  psposFirst,
                                                _Out_opt_ ::SegmentPosition* const  psposReplay,
                                                _Out_opt_ ::SegmentPosition* const  psposDurable,
                                                _Out_opt_ ::SegmentPosition* const  psposLast,
                                                _Out_opt_ ::SegmentPosition* const  psposFull ) override;

                        ERR ErrVisitSegments(   _In_ const ::IJournalSegmentManager::PfnVisitSegment    pfnVisitSegment,
                                                _In_ const DWORD_PTR                                    keyVisitSegment ) override;

                        ERR ErrAppendSegment(   _In_    const ::SegmentPosition     spos,
                                                _In_    const ::SegmentPosition     sposReplay,
                                                _In_    const ::SegmentPosition     sposDurable,
                                                _Out_   ::IJournalSegment** const   ppjs ) override;

                        ERR ErrFlush() override;
                };

                template< class TM, class TN >
                inline ERR CJournalSegmentManagerWrapper<TM, TN>::ErrGetProperties( _Out_opt_ ::SegmentPosition* const  psposFirst,
                                                                                    _Out_opt_ ::SegmentPosition* const  psposReplay,
                                                                                    _Out_opt_ ::SegmentPosition* const  psposDurable,
                                                                                    _Out_opt_ ::SegmentPosition* const  psposLast,
                                                                                    _Out_opt_ ::SegmentPosition* const  psposFull )
                {
                    ERR             err                     = JET_errSuccess;
                    SegmentPosition segmentPositionFirst    = SegmentPosition::Invalid;
                    SegmentPosition segmentPositionReplay   = SegmentPosition::Invalid;
                    SegmentPosition segmentPositionDurable  = SegmentPosition::Invalid;
                    SegmentPosition segmentPositionLast     = SegmentPosition::Invalid;
                    SegmentPosition segmentPositionFull     = SegmentPosition::Invalid;

                    if ( psposFirst )
                    {
                        *psposFirst = ::sposInvalid;
                    }
                    if ( psposReplay )
                    {
                        *psposReplay = ::sposInvalid;
                    }
                    if ( psposDurable )
                    {
                        *psposDurable = ::sposInvalid;
                    }
                    if ( psposLast )
                    {
                        *psposLast = ::sposInvalid;
                    }
                    if ( psposFull )
                    {
                        *psposFull = ::sposInvalid;
                    }

                    ExCall( I()->GetProperties( segmentPositionFirst,
                                                segmentPositionReplay,
                                                segmentPositionDurable,
                                                segmentPositionLast,
                                                segmentPositionFull ) );

                    if ( psposFirst )
                    {
                        *psposFirst = (::SegmentPosition)segmentPositionFirst;
                    }
                    if ( psposReplay )
                    {
                        *psposReplay = (::SegmentPosition)segmentPositionReplay;
                    }
                    if ( psposDurable )
                    {
                        *psposDurable = (::SegmentPosition)segmentPositionDurable;
                    }
                    if ( psposLast )
                    {
                        *psposLast = (::SegmentPosition)segmentPositionLast;
                    }
                    if ( psposFull )
                    {
                        *psposFull = (::SegmentPosition)segmentPositionFull;
                    }

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        if ( psposFirst )
                        {
                            *psposFirst = ::sposInvalid;
                        }
                        if ( psposReplay )
                        {
                            *psposReplay = ::sposInvalid;
                        }
                        if ( psposDurable )
                        {
                            *psposDurable = ::sposInvalid;
                        }
                        if ( psposLast )
                        {
                            *psposLast = ::sposInvalid;
                        }
                        if ( psposFull )
                        {
                            *psposFull = ::sposInvalid;
                        }
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CJournalSegmentManagerWrapper<TM, TN>::ErrVisitSegments( _In_ const ::IJournalSegmentManager::PfnVisitSegment    pfnVisitSegment,
                                                                                    _In_ const DWORD_PTR                                    keyVisitSegment )
                {
                    ERR             err             = JET_errSuccess;
                    VisitSegment^   visitSegment    = gcnew VisitSegment( pfnVisitSegment, keyVisitSegment );

                    ExCall( I()->VisitSegments( gcnew Internal::Ese::BlockCache::Interop::IJournalSegmentManager::VisitSegment( visitSegment, &VisitSegment::VisitSegment_ ) ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CJournalSegmentManagerWrapper<TM, TN>::ErrAppendSegment( _In_    const ::SegmentPosition     spos,
                                                                                    _In_    const ::SegmentPosition     sposReplay,
                                                                                    _In_    const ::SegmentPosition     sposDurable,
                                                                                    _Out_   ::IJournalSegment** const   ppjs )
                {
                    ERR                 err = JET_errSuccess;
                    IJournalSegment^    js  = nullptr;
                    ::IJournalSegment*  pjs = NULL;

                    *ppjs = NULL;

                    ExCall( js = I()->AppendSegment(    (SegmentPosition)spos,
                                                        (SegmentPosition)sposReplay,
                                                        (SegmentPosition)sposDurable) );

                    if ( js != nullptr )
                    {
                        Call( JournalSegment::ErrWrap( js, &pjs ) );
                    }

                    *ppjs = pjs;
                    pjs = NULL;

                HandleError:
                    delete pjs;
                    if ( err < JET_errSuccess )
                    {
                        delete js;
                        delete *ppjs;
                        *ppjs = NULL;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CJournalSegmentManagerWrapper<TM, TN>::ErrFlush()
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->Flush() );

                HandleError:
                    return err;
                }
            }
        }
    }
}
