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

                        ERR ErrGetProperties(   _Out_opt_ ::SegmentPosition* const  psposReplay,
                                                _Out_opt_ ::SegmentPosition* const  psposDurable,
                                                _Out_opt_ ::SegmentPosition* const  psposLast ) override;

                        ERR ErrVisitSegments(   _In_ const ::IJournalSegmentManager::PfnVisitSegment    pfnVisitSegment,
                                                _In_ const DWORD_PTR                                    keyVisitSegment ) override;

                        ERR ErrAppendSegment(   _In_    const ::SegmentPosition     spos,
                                                _In_    const ::SegmentPosition     sposReplay,
                                                _In_    const ::SegmentPosition     sposDurable,
                                                _Out_   ::IJournalSegment** const   ppjs ) override;

                        ERR ErrFlush() override;
                };

                template< class TM, class TN >
                inline ERR CJournalSegmentManagerWrapper<TM, TN>::ErrGetProperties( _Out_opt_ ::SegmentPosition* const  psposReplay,
                                                                                    _Out_opt_ ::SegmentPosition* const  psposDurable,
                                                                                    _Out_opt_ ::SegmentPosition* const  psposLast )
                {
                    ERR             err                     = JET_errSuccess;
                    SegmentPosition segmentPositionReplay   = SegmentPosition::Invalid;
                    SegmentPosition segmentPositionDurable  = SegmentPosition::Invalid;
                    SegmentPosition segmentPositionLast     = SegmentPosition::Invalid;

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

                    ExCall( I()->GetProperties( segmentPositionReplay,
                                                segmentPositionDurable,
                                                segmentPositionLast ) );

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

                HandleError:
                    if ( err < JET_errSuccess )
                    {
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
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CJournalSegmentManagerWrapper<TM, TN>::ErrVisitSegments( _In_ const ::IJournalSegmentManager::PfnVisitSegment    pfnVisitSegment,
                                                                                    _In_ const DWORD_PTR                                    keyVisitSegment )
                {
                    ERR             err             = JET_errSuccess;
                    VisitSegment^   visitsegment    = gcnew VisitSegment( pfnVisitSegment, keyVisitSegment );

                    ExCall( I()->VisitSegments( gcnew Internal::Ese::BlockCache::Interop::IJournalSegmentManager::VisitSegment( visitsegment, &VisitSegment::VisitSegment_ ) ) );

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
