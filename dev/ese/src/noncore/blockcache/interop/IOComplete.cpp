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
                void IOComplete::Complete(
                    EsentErrorException^ ex,
                    IFile^ file,
                    FileQOS fileQOS,
                    Int64 offsetInBytes,
                    ArraySegment<byte> data )
                {
                    ERR                 err     = ex == nullptr ? JET_errSuccess : (ERR)ex->Error;
                    FullTraceContext    fullTc;

                    if ( !isWrite )
                    {
                        pin_ptr<const byte> rgbData = &data.Array[ data.Offset ];
                        UtilMemCpy( pbData, (const BYTE*)rgbData, data.Count );
                    }

                    pfnIOComplete(  err,
                                    pfapi,
                                    fullTc,
                                    (OSFILEQOS)fileQOS,
                                    offsetInBytes,
                                    data.Count,
                                    pbData,
                                    keyIOComplete );
                }

                void IOComplete::Handoff(
                    EsentErrorException^ ex,
                    IFile^ file,
                    FileQOS fileQOS,
                    Int64 offsetInBytes,
                    ArraySegment<byte> data )
                {
                    ERR err = ex == nullptr ? JET_errSuccess : (ERR)ex->Error;

                    if ( pfnIOHandoff )
                    {
                        FullTraceContext fullTc;

                        pfnIOHandoff(   err,
                                        pfapi,
                                        fullTc,
                                        (OSFILEQOS)fileQOS,
                                        offsetInBytes,
                                        data.Count,
                                        pbData,
                                        keyIOComplete,
                                        NULL );
                    }
                }
            }
        }
    }
}
