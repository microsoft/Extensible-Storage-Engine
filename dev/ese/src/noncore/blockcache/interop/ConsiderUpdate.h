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
                ref class ConsiderUpdate : Remotable
                {
                    public:

                        ConsiderUpdate( _In_ const ::ICachedBlockSlab::PfnConsiderUpdate    pfnConsiderUpdate,
                                        _In_ const DWORD_PTR                                keyConsiderUpdate )
                            :   pfnConsiderUpdate( pfnConsiderUpdate ),
                                keyConsiderUpdate( keyConsiderUpdate )
                        {
                        }

                        bool ConsiderUpdate_( CachedBlockSlotState^ currentSlotState, CachedBlockSlot^ newSlot )
                        {
                            if ( !pfnConsiderUpdate )
                            {
                                return false;
                            }

                            return pfnConsiderUpdate(   *currentSlotState->Pslotst(),
                                                        *newSlot->Pslot(),
                                                        keyConsiderUpdate );
                        }

                    private:

                        const ::ICachedBlockSlab::PfnConsiderUpdate pfnConsiderUpdate;
                        const DWORD_PTR                             keyConsiderUpdate;
                };
            }
        }
    }
}
