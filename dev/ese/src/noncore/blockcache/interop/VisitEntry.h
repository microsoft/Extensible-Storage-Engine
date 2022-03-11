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
                ref class VisitEntry : MarshalByRefObject
                {
                    public:

                        VisitEntry( _In_ const ::IJournal::PfnVisitEntry    pfnVisitEntry,
                                    _In_ const DWORD_PTR                    keyVisitEntry )
                            :   pfnVisitEntry( pfnVisitEntry ),
                                keyVisitEntry( keyVisitEntry )
                        {
                        }

                        bool VisitEntry_(
                            JournalPosition journalPosition,
                            JournalPosition journalPositionEnd,
                            ArraySegment<BYTE> entry )
                        {
                            pin_ptr<const BYTE> rgbEntry =
                                entry.Array == nullptr
                                    ? nullptr
                                    : ( entry.Count == 0 ? &(gcnew array<BYTE>(1))[0] : &entry.Array[ entry.Offset ]);
                            return pfnVisitEntry(   (::JournalPosition)journalPosition,
                                                    (::JournalPosition)journalPositionEnd,
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
