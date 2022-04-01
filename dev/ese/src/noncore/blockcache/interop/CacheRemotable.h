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
                ref class CacheRemotable : Remotable, ICache
                {
                    public:

                        CacheRemotable( ICache^ target )
                            :   target( target )
                        {
                        }

                        ~CacheRemotable() {}

                    public:

                        virtual void Create()
                        {
                            this->target->Create();
                        }

                        virtual void Mount()
                        {
                            this->target->Mount();
                        }

                        virtual Guid GetCacheType()
                        {
                            return this->target->GetCacheType();
                        }

                        virtual void GetPhysicalId( [Out] VolumeId% volumeid, [Out] FileId% fileid, [Out] Guid% guid )
                        {
                            this->target->GetPhysicalId( volumeid, fileid, guid );
                        }

                        virtual void Close( VolumeId volumeid, FileId fileid, FileSerial fileserial )
                        {
                            return this->target->Close( volumeid, fileid, fileserial );
                        }

                        virtual void Flush( VolumeId volumeid, FileId fileid, FileSerial fileserial )
                        {
                            return this->target->Flush( volumeid, fileid, fileserial );
                        }

                        virtual void Invalidate(
                            VolumeId volumeid,
                            FileId fileid,
                            FileSerial fileserial,
                            Int64 offsetInBytes,
                            Int64 sizeInBytes )
                        {
                            return this->target->Invalidate( volumeid, fileid, fileserial, offsetInBytes, sizeInBytes );
                        }

                        virtual void Read(
                            VolumeId volumeid,
                            FileId fileid,
                            FileSerial fileserial,
                            Int64 offsetInBytes,
                            MemoryStream^ data,
                            FileQOS fileQOS,
                            CachingPolicy cachingPolicy,
                            ICache::Complete^ complete )
                        {
                            return this->target->Read( volumeid, fileid, fileserial, offsetInBytes, data, fileQOS, cachingPolicy, complete );
                        }

                        virtual void Write(
                            VolumeId volumeid,
                            FileId fileid,
                            FileSerial fileserial,
                            Int64 offsetInBytes,
                            MemoryStream^ data,
                            FileQOS fileQOS,
                            CachingPolicy cachingPolicy,
                            ICache::Complete^ complete )
                        {
                            return this->target->Write( volumeid, fileid, fileserial, offsetInBytes, data, fileQOS, cachingPolicy, complete );
                        }

                        virtual void Issue(
                            VolumeId volumeid,
                            FileId fileid,
                            FileSerial fileserial )
                        {
                            return this->target->Issue( volumeid, fileid, fileserial );
                        }

                    private:

                        ICache^ target;
                };
            }
        }
    }
}