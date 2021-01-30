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
                public interface class ICacheTelemetry : IDisposable
                {
                    enum class FileNumber : int
                    {
                        Invalid = -1,
                    };

                    enum class BlockNumber : int
                    {
                        Invalid = -1,
                    };

                    void Miss( FileNumber filenumber, BlockNumber blocknumber, bool isRead, bool cacheIfPossible );

                    void Hit( FileNumber filenumber, BlockNumber blocknumber, bool isRead, bool cacheIfPossible );

                    void Update( FileNumber filenumber, BlockNumber blocknumber );

                    void Write( FileNumber filenumber, BlockNumber blocknumber, bool replacementPolicy );

                    void Evict( FileNumber filenumber, BlockNumber blocknumber, bool replacementPolicy );
                };
            }
        }
    }
}
