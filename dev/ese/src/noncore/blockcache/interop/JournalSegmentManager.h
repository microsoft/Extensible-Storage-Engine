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
                public ref class JournalSegmentManager : JournalSegmentManagerBase<IJournalSegmentManager, ::IJournalSegmentManager, CJournalSegmentManagerWrapper<IJournalSegmentManager, ::IJournalSegmentManager>>
                {
                    public:

                        static ERR ErrWrap( IJournalSegmentManager^% jsm, ::IJournalSegmentManager** const ppjsm )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<IJournalSegmentManager, ::IJournalSegmentManager, CJournalSegmentManagerWrapper<IJournalSegmentManager, ::IJournalSegmentManager>, JournalSegmentManagerRemotable>( jsm, ppjsm );
                        }

                    public:

                        JournalSegmentManager( IJournalSegmentManager^ jsm ) : JournalSegmentManagerBase( jsm ) { }

                        JournalSegmentManager( ::IJournalSegmentManager** const ppjsm ) : JournalSegmentManagerBase( ppjsm ) {}
                };
            }
        }
    }
}
