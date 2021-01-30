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
                public interface class IFileFind : IDisposable
                {
                    bool Next();

                    bool IsFolder();

                    String^ Path();

                    Int64 Size( FileSize fileSize );

                    bool IsReadOnly();
                };
            }
        }
    }
}
