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
                void Complete::Complete_(
                    EsentErrorException^ ex,
                    VolumeId volumeid,
                    FileId fileid,
                    FileSerial fileserial,
                    FileQOS fileQOS,
                    Int64 offsetInBytes,
                    ArraySegment<byte> data )
                {
                    ERR                 err = ex == nullptr ? JET_errSuccess : (ERR)ex->Error;
                    FullTraceContext    fullTc;

                    if ( !isWrite )
                    {
                        pin_ptr<const byte> rgbData = &data.Array[ data.Offset ];
                        UtilMemCpy( pbData, (const BYTE*)rgbData, data.Count );
                    }

                    pfnComplete(    err,
                                    (::VolumeId)volumeid,
                                    (::FileId)fileid,
                                    (::FileSerial)fileserial,
                                    fullTc,
                                    (OSFILEQOS)fileQOS,
                                    offsetInBytes,
                                    data.Count,
                                    pbData,
                                    keyComplete );
                }
            }
        }
    }
}
