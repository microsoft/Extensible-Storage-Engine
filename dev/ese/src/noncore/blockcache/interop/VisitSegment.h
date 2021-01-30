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
                ref class VisitSegment
                {
                    public:

                        VisitSegment(   const ::IJournalSegmentManager::PfnVisitSegment pfnVisitSegment,
                                        const DWORD_PTR                                 keyVisitSegment )
                            :   pfnVisitSegment( pfnVisitSegment ),
                                keyVisitSegment( keyVisitSegment )
                        {
                        }

                        bool VisitSegment_(
                            SegmentPosition segmentPosition,
                            EsentErrorException^ ex,
                            IJournalSegment^ ijs )
                        {
                            ERR                 err     = JET_errSuccess;
                            ::IJournalSegment*  pjs     = NULL;
                            BOOL                fResult = fFalse;

                            if ( ijs != nullptr )
                            {
                                Call( JournalSegment::ErrWrap( ijs, &pjs ) );
                            }

                            fResult = pfnVisitSegment(  (::SegmentPosition)segmentPosition,
                                                        ex == nullptr ? JET_errSuccess : (ERR)ex->Error,
                                                        pjs,
                                                        keyVisitSegment );

                        HandleError:
                            delete pjs;
                            if ( err == JET_errOutOfMemory )
                            {
                                throw gcnew OutOfMemoryException();
                            }
                            return fResult;
                        }

                    private:

                        const ::IJournalSegmentManager::PfnVisitSegment pfnVisitSegment;
                        const DWORD_PTR                                 keyVisitSegment;
                };
            }
        }
    }
}
