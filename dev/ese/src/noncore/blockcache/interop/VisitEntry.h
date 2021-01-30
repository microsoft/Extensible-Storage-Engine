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
                ref class VisitEntry
                {
                    public:

                        VisitEntry( const ::IJournal::PfnVisitEntry pfnVisitEntry,
                                    const DWORD_PTR                 keyVisitEntry )
                            :   pfnVisitEntry( pfnVisitEntry ),
                                keyVisitEntry( keyVisitEntry )
                        {
                        }

                        bool VisitEntry_(
                            JournalPosition journalPosition,
                            ArraySegment<byte> entry )
                        {
                            pin_ptr<const byte> rgbEntry =
                                entry.Array == nullptr
                                    ? nullptr
                                    : ( entry.Count == 0 ? &(gcnew array<byte>(1))[0] : &entry.Array[ entry.Offset ]);
                            return pfnVisitEntry(   (::JournalPosition)journalPosition,
                                                    CJournalBuffer( entry.Count, rgbEntry ),
                                                    keyVisitEntry );
                        }

                    private:

                        const ::IJournal::PfnVisitEntry pfnVisitEntry;
                        const DWORD_PTR                 keyVisitEntry;
                };
            }
        }
    }
}
