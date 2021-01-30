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
                ref class VisitRegion
                {
                    public:

                        VisitRegion(    const ::IJournalSegment::PfnVisitRegion pfnVisitRegion,
                                        const DWORD_PTR                         keyVisitRegion )
                            :   pfnVisitRegion( pfnVisitRegion ),
                                keyVisitRegion( keyVisitRegion )
                        {
                        }

                        bool VisitRegion_(
                            RegionPosition regionPosition,
                            ArraySegment<byte> region )
                        {
                            pin_ptr<const byte> rgbRegion =
                                region.Array == nullptr
                                    ? nullptr
                                    : (region.Count == 0 ? &(gcnew array<byte>(1))[0] : &region.Array[ region.Offset ]);
                            return pfnVisitRegion(  (::RegionPosition)regionPosition,
                                                    CJournalBuffer( region.Count, rgbRegion ),
                                                    keyVisitRegion );
                        }

                    private:

                        const ::IJournalSegment::PfnVisitRegion pfnVisitRegion;
                        const DWORD_PTR                         keyVisitRegion;
                };
            }
        }
    }
}
