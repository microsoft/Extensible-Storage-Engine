// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

#undef interface

#pragma managed

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                /// <summary>
                /// Object Marshaller Interface.
                /// </summary>
                public interface class IObjectMarshaller : IDisposable
                {
                    /// <summary>
                    /// Fetches an object's proxy from another AppDomain by object id.
                    /// </summary>
                    /// <param name="id">The object's native id.</param>
                    /// <returns>The requested object's proxy.</returns>
                    Object^ Get( IntPtr id );

                    /// <summary>
                    /// Setup the resolve handler.
                    /// </summary>
                    void SetupResolveHandler();
                };
            }
        }
    }
}