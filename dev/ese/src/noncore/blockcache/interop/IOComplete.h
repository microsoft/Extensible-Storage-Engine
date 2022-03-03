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
                ref class IOComplete : MarshalByRefObject
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
                            MemoryStream^ data )
                        {
                            ERR                 err     = ex == nullptr ? JET_errSuccess : (ERR)ex->Error;
                            FullTraceContext    fullTc;

                            if ( !this->isWrite && data != nullptr && data->Length > 0 )
                            {
                                array<byte>^ bytes = data->ToArray();
                                pin_ptr<const byte> rgbData = &bytes[0];
                                UtilMemCpy( this->pbData, (const BYTE*)rgbData, bytes->Length );
                            }

                            this->pfnIOComplete(    err,
                                                    this->pfapi,
                                                    fullTc,
                                                    (OSFILEQOS)fileQOS,
                                                    offsetInBytes,
                                                    data == nullptr ? 0 : (DWORD)data->Length,
                                                    this->pbData,
                                                    this->keyIOComplete );
                        }

                        void Handoff(
                            EsentErrorException^ ex,
                            IFile^ file,
                            FileQOS fileQOS,
                            Int64 offsetInBytes,
                            MemoryStream^ data )
                        {
                            ERR err = ex == nullptr ? JET_errSuccess : (ERR)ex->Error;

                            if ( this->pfnIOHandoff )
                            {
                                FullTraceContext fullTc;

                                this->pfnIOHandoff( err,
                                                    this->pfapi,
                                                    fullTc,
                                                    (OSFILEQOS)fileQOS,
                                                    offsetInBytes,
                                                    data == nullptr ? 0 : (DWORD)data->Length,
                                                    this->pbData,
                                                    this->keyIOComplete,
                                                    NULL );
                            }
                        }

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
