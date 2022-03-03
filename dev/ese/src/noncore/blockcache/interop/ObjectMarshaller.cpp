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
                Object^ ObjectMarshaller::Get( IntPtr id )
                {
                    const CContainer* pcontainer = (const CContainer*)(void*)id;

                    return pcontainer->O();
                }
            }
        }
    }
}