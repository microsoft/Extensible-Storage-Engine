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
                ref class Complete
                {
                    public:

                        Complete(   const bool                  isWrite,
                                    BYTE* const                 pbData,
                                    const ::ICache::PfnComplete pfnComplete,
                                    const DWORD_PTR             keyComplete )
                            :   isWrite( isWrite ),
                                pbData( pbData ),
                                pfnComplete( pfnComplete ),
                                keyComplete( keyComplete )
                        {
                        }

                        void Complete_(
                            EsentErrorException^ ex,
                            VolumeId volumeid,
                            FileId fileid,
                            FileSerial fileserial,
                            FileQOS fileQOS,
                            Int64 offsetInBytes,
                            ArraySegment<byte> data );

                    private:

                        const bool                  isWrite;
                        BYTE* const                 pbData;
                        const ::ICache::PfnComplete pfnComplete;
                        const DWORD_PTR             keyComplete;
                };
            }
        }
    }
}
