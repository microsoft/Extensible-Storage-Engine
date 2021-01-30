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
                ref class IOComplete
                {
                    public:

                        IOComplete( IFileAPI* const                 pfapi,
                                    const bool                      isWrite,
                                    BYTE* const                     pbData,
                                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    const DWORD_PTR                 keyIOComplete,
                                    const IFileAPI::PfnIOHandoff    pfnIOHandoff )
                            :   pfapi( pfapi ),
                                isWrite( isWrite ),
                                pbData( pbData ),
                                pfnIOComplete( pfnIOComplete ),
                                keyIOComplete( keyIOComplete ),
                                pfnIOHandoff( pfnIOHandoff )
                        {
                        }

                        void Complete(
                            EsentErrorException^ ex,
                            IFile^ file,
                            FileQOS fileQOS,
                            Int64 offsetInBytes,
                            ArraySegment<byte> data );

                        void Handoff(
                            EsentErrorException^ ex,
                            IFile^ file,
                            FileQOS fileQOS,
                            Int64 offsetInBytes,
                            ArraySegment<byte> data );

                    private:

                        IFileAPI* const                 pfapi;
                        const bool                      isWrite;
                        BYTE* const                     pbData;
                        const IFileAPI::PfnIOComplete   pfnIOComplete;
                        const DWORD_PTR                 keyIOComplete;
                        const IFileAPI::PfnIOHandoff    pfnIOHandoff;
                };
            }
        }
    }
}
