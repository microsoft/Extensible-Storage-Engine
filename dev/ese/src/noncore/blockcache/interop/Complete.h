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
                ref class Complete : MarshalByRefObject
                {
                    public:

                        Complete(   const BOOL                  isWrite,
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
                            MemoryStream^ data )
                        {
                            ERR                 err = ex == nullptr ? JET_errSuccess : (ERR)ex->Error;
                            FullTraceContext    fullTc;

                            if ( !this->isWrite && data != nullptr && data->Length > 0 )
                            {
                                array<byte>^ bytes = data->ToArray();
                                pin_ptr<const byte> rgbData = &bytes[ 0 ];
                                UtilMemCpy( this->pbData, (const BYTE*)rgbData, bytes->Length );
                            }

                            this->pfnComplete(  err,
                                                (::VolumeId)volumeid,
                                                (::FileId)fileid,
                                                (::FileSerial)fileserial,
                                                fullTc,
                                                (OSFILEQOS)fileQOS,
                                                offsetInBytes,
                                                data == nullptr ? 0 : (DWORD)data->Length,
                                                this->pbData,
                                                this->keyComplete );
                        }

                    private:

                        const BOOL                  isWrite;
                        BYTE* const                 pbData;
                        const ::ICache::PfnComplete pfnComplete;
                        const DWORD_PTR             keyComplete;
                };
            }
        }
    }
}
