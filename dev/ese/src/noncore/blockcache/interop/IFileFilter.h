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
                public interface class IFileFilter : IFile
                {
                    void GetPhysicalId(
                        [Out] VolumeId% volumeid,
                        [Out] FileId% fileid,
                        [Out] FileSerial% fileserial );

                    void Read(
                        Int64 offsetInBytes,
                        ArraySegment<byte> data,
                        FileQOS fileQOS,
                        IOMode ioMode,
                        IOComplete^ ioComplete,
                        IOHandoff^ ioHandoff );

                    void Write(
                        Int64 offsetInBytes,
                        ArraySegment<byte> data,
                        FileQOS fileQOS,
                        IOMode ioMode,
                        IOComplete^ ioComplete,
                        IOHandoff^ ioHandoff );

                    void Issue( IOMode ioMode );
                };
            }
        }
    }
}
