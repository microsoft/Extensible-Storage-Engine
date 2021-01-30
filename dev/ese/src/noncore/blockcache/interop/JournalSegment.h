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
                public ref class JournalSegment : public JournalSegmentBase<IJournalSegment, ::IJournalSegment, CJournalSegmentWrapper<IJournalSegment, ::IJournalSegment>>
                {
                    public:

                        static ERR ErrWrap( IJournalSegment^% js, ::IJournalSegment** const ppjs )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<IJournalSegment, ::IJournalSegment, CJournalSegmentWrapper<IJournalSegment, ::IJournalSegment>>( js, ppjs );
                        }

                    public:

                        JournalSegment( IJournalSegment^ js ) : JournalSegmentBase( js ) { }

                        JournalSegment( ::IJournalSegment* const pjs ) : JournalSegmentBase( pjs ) {}

                        JournalSegment( ::IJournalSegment** const ppjs ) : JournalSegmentBase( ppjs ) {}
                };
            }
        }
    }
}
