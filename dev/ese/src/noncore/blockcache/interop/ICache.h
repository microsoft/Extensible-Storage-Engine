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
                public interface class ICache : IDisposable
                {
                    delegate void Complete(
                        EsentErrorException^ ex,
                        VolumeId volumeid, 
                        FileId fileid,
                        FileSerial fileserial,
                        FileQOS fileQOS,
                        Int64 offsetInBytes,
                        ArraySegment<byte> data );

                    void Create();

                    void Mount();

                    Guid GetCacheType();

                    void GetPhysicalId( [Out] VolumeId% volumeid, [Out] FileId% fileid, [Out] Guid% guid );

                    void Close( VolumeId volumeid, FileId fileid, FileSerial fileserial );

                    void Flush( VolumeId volumeid, FileId fileid, FileSerial fileserial );

                    void Invalidate(
                        VolumeId volumeid, 
                        FileId fileid, 
                        FileSerial fileserial, 
                        Int64 offsetInBytes, 
                        Int64 sizeInBytes );

                    void Read(
                        VolumeId volumeid,
                        FileId fileid,
                        FileSerial fileserial,
                        Int64 offsetInBytes,
                        ArraySegment<byte> data,
                        FileQOS fileQOS,
                        CachingPolicy cachingPolicy,
                        Complete^ complete );

                    void Write(
                        VolumeId volumeid,
                        FileId fileid,
                        FileSerial fileserial,
                        Int64 offsetInBytes,
                        ArraySegment<byte> data,
                        FileQOS fileQOS,
                        CachingPolicy cachingPolicy,
                        Complete^ complete );
                };
            }
        }
    }
}
