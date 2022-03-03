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
                ref class VisitSlot : MarshalByRefObject
                {
                    public:

                        VisitSlot(  _In_ const ::ICachedBlockSlab::PfnVisitSlot pfnVisitSlot,
                                    _In_ const DWORD_PTR                        keyVisitSlot )
                            :   pfnVisitSlot( pfnVisitSlot ),
                                keyVisitSlot( keyVisitSlot )
                        {
                        }

                        bool VisitSlot_(
                            EsentErrorException^ ex,
                            CachedBlockSlot^ acceptedSlot,
                            CachedBlockSlotState^ currentSlotState )
                        {
                            if ( !pfnVisitSlot )
                            {
                                return false;
                            }

                            return pfnVisitSlot(    ex == nullptr ? JET_errSuccess : (ERR)(int)ex->Error,
                                                    *acceptedSlot->Pslot(),
                                                    *currentSlotState->Pslotst(),
                                                    keyVisitSlot );
                        }

                    private:

                        const ::ICachedBlockSlab::PfnVisitSlot  pfnVisitSlot;
                        const DWORD_PTR                         keyVisitSlot;
                };
            }
        }
    }
}
