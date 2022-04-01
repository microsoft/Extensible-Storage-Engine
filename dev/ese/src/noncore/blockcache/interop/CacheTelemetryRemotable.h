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
                ref class CacheTelemetryRemotable : Remotable, ICacheTelemetry
                {
                    public:

                        CacheTelemetryRemotable( ICacheTelemetry^ target )
                            :   target( target )
                        {
                        }

                        ~CacheTelemetryRemotable() {}

                    public:

                        virtual void Miss(
                            ICacheTelemetry::FileNumber filenumber,
                            ICacheTelemetry::BlockNumber blocknumber,
                            bool isRead,
                            bool cacheIfPossible )
                        {
                            return this->target->Miss( filenumber, blocknumber, isRead, cacheIfPossible );
                        }

                        virtual void Hit(
                            ICacheTelemetry::FileNumber filenumber,
                            ICacheTelemetry::BlockNumber blocknumber,
                            bool isRead,
                            bool cacheIfPossible )
                        {
                            return this->target->Hit( filenumber, blocknumber, isRead, cacheIfPossible );
                        }

                        virtual void Update(
                            ICacheTelemetry::FileNumber filenumber,
                            ICacheTelemetry::BlockNumber blocknumber )
                        {
                            return this->target->Update( filenumber, blocknumber );
                        }

                        virtual void Write(
                            ICacheTelemetry::FileNumber filenumber,
                            ICacheTelemetry::BlockNumber blocknumber,
                            bool replacementPolicy )
                        {
                            return this->target->Write( filenumber, blocknumber, replacementPolicy );
                        }

                        virtual void Evict(
                            ICacheTelemetry::FileNumber filenumber,
                            ICacheTelemetry::BlockNumber blocknumber,
                            bool replacementPolicy )
                        {
                            return this->target->Evict( filenumber, blocknumber, replacementPolicy );
                        }

                    private:

                        ICacheTelemetry^ target;
                };
            }
        }
    }
}