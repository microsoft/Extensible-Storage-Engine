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
                ref class VisitRegion : MarshalByRefObject
                {
                    public:

                        VisitRegion(    _In_ const ::IJournalSegment::PfnVisitRegion    pfnVisitRegion,
                                        _In_ const DWORD_PTR                            keyVisitRegion )
                            :   pfnVisitRegion( pfnVisitRegion ),
                                keyVisitRegion( keyVisitRegion )
                        {
                        }

                        bool VisitRegion_(
                            RegionPosition regionPosition,
                            RegionPosition regionPositionEnd,
                            ArraySegment<byte> region )
                        {
                            pin_ptr<const byte> rgbRegion =
                                region.Array == nullptr
                                    ? nullptr
                                    : (region.Count == 0 ? &(gcnew array<byte>(1))[0] : &region.Array[ region.Offset ]);
                            return pfnVisitRegion(  (::RegionPosition)regionPosition,
                                                    (::RegionPosition)regionPositionEnd,
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
