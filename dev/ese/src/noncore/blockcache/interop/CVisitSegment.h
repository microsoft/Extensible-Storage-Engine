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
                class CVisitSegment
                {
                    public:

                        CVisitSegment( _In_ IJournalSegmentManager::VisitSegment^ visitsegment )
                            :   m_visitsegment( visitsegment )
                        {
                        }

                        ~CVisitSegment()
                        {
                        }

                    private:

                        CVisitSegment( _In_ const CVisitSegment& other );

                    private:

                        mutable msclr::auto_gcroot<IJournalSegmentManager::VisitSegment^>   m_visitsegment;

                    public:

                        static BOOL VisitSegment_(  _In_ const ::SegmentPosition    spos,
                                                    _In_ const ERR                  err,
                                                    _In_ ::IJournalSegment* const   pjs,
                                                    _In_ const DWORD_PTR            keyVisitSegment )
                        {
                            ::IJournalSegmentManager::PfnVisitSegment pfnVisitSegment = VisitSegment_;
                            Unused( pfnVisitSegment );

                            CVisitSegment* const pvisitsegment = (CVisitSegment*)keyVisitSegment;

                            bool result = pvisitsegment->m_visitsegment.get()(  (SegmentPosition)spos,
                                                                                EseException( err ),
                                                                                pjs ? gcnew JournalSegment( pjs ) : nullptr );

                            return result ? fTrue : fFalse;
                        }
                };
            }
        }
    }
}