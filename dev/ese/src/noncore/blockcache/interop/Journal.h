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
                public ref class Journal : public JournalBase<IJournal, ::IJournal, CJournalWrapper<IJournal, ::IJournal>>
                {
                    public:

                        static ERR ErrWrap( IJournal^% j, ::IJournal** const ppj )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<IJournal, ::IJournal, CJournalWrapper<IJournal, ::IJournal>>( j, ppj );
                        }

                    public:

                        Journal( IJournal^ j ) : JournalBase( j ) { }

                        Journal( ::IJournal** const ppj ) : JournalBase( ppj ) {}
                };
            }
        }
    }
}
