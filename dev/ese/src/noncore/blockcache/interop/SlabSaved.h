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
                ref class SlabSaved : MarshalByRefObject
                {
                    public:

                        SlabSaved(  _In_ const ::ICachedBlockSlab::PfnSlabSaved pfnSlabSaved,
                                    _In_ const DWORD_PTR                        keySlabSaved )
                            :   pfnSlabSaved( pfnSlabSaved ),
                                keySlabSaved( keySlabSaved )
                        {
                        }

                        void SlabSaved_( EsentErrorException^ ex )
                        {
                            pfnSlabSaved( ex == nullptr ? JET_errSuccess : (ERR)ex->Error, keySlabSaved );
                        }

                    private:

                        const ::ICachedBlockSlab::PfnSlabSaved  pfnSlabSaved;
                        const DWORD_PTR                         keySlabSaved;
                };
            }
        }
    }
}
