// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                void CComplete::Complete(   _In_                    const ERR               err,
                                            _In_                    const ::VolumeId        volumeid,
                                            _In_                    const ::FileId          fileid,
                                            _In_                    const ::FileSerial      fileserial,
                                            _In_                    const FullTraceContext& tc,
                                            _In_                    const OSFILEQOS         grbitQOS,
                                            _In_                    const QWORD             ibOffset,
                                            _In_                    const DWORD             cbData,
                                            _In_reads_( cbData )    const BYTE* const       pbData,
                                            _In_                    const DWORD_PTR         keyComplete )
                {
                    ::ICache::PfnComplete pfnComplete = Complete;
                    Unused( pfnComplete );

                    CComplete* const pcomplete = (CComplete*)keyComplete;
                    if ( !pcomplete->FWrite() )
                    {
                        pin_ptr<byte> rgbData = &pcomplete->m_rgbData[ pcomplete->m_ibData ];
                        UtilMemCpy( (BYTE*)rgbData, pbData, pcomplete->m_cbData );
                    }

                    pcomplete->m_complete.get()(    EseException( err ),
                                                    (VolumeId)volumeid,
                                                    (FileId)fileid,
                                                    (FileSerial)fileserial,
                                                    (FileQOS)grbitQOS,
                                                    ibOffset,
                                                    ArraySegment<byte>( pcomplete->m_rgbData,
                                                                        pcomplete->m_ibData,
                                                                        pcomplete->m_cbData ) );

                    delete pcomplete;
                }
            }
        }
    }
}