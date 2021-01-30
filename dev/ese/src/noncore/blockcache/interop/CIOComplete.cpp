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
                void CIOComplete::Complete( _In_                    const ERR               err,
                                            _In_                    IFileAPI* const         pfapi,
                                            _In_                    const FullTraceContext& tc,
                                            _In_                    const OSFILEQOS         grbitQOS,
                                            _In_                    const QWORD             ibOffset,
                                            _In_                    const DWORD             cbData,
                                            _In_reads_( cbData )    const BYTE* const       pbData,
                                            _In_                    const DWORD_PTR         keyIOComplete )
                {
                    IFileAPI::PfnIOComplete pfnIOComplete = Complete;
                    Unused( pfnIOComplete );

                    CIOComplete* const piocomplete = (CIOComplete*)keyIOComplete;
                    if ( !piocomplete->FWrite() )
                    {
                        pin_ptr<byte> rgbData = &piocomplete->m_rgbData[ piocomplete->m_ibData ];
                        UtilMemCpy( (BYTE*)rgbData, pbData, piocomplete->m_cbData );
                    }

                    piocomplete->m_ioComplete.get()(    EseException( err ),
                                                        piocomplete->m_ifile,
                                                        (FileQOS)grbitQOS,
                                                        ibOffset,
                                                        ArraySegment<byte>( piocomplete->m_rgbData,
                                                                            piocomplete->m_ibData,
                                                                            piocomplete->m_cbData ) );

                    piocomplete->Release();
                }

                void CIOComplete::Handoff(  _In_                    const ERR               err,
                                            _In_                    IFileAPI* const         pfapi,
                                            _In_                    const FullTraceContext& tc,
                                            _In_                    const OSFILEQOS         grbitQOS,
                                            _In_                    const QWORD             ibOffset,
                                            _In_                    const DWORD             cbData,
                                            _In_reads_( cbData )    const BYTE* const       pbData,
                                            _In_                    const DWORD_PTR         keyIOComplete,
                                            _In_                    void* const             pvIOContext )
                {
                    IFileAPI::PfnIOHandoff pfnIOHandoff = Handoff;
                    Unused( pfnIOHandoff );

                    CIOComplete* const piocomplete = (CIOComplete*)keyIOComplete;
                    if ( piocomplete->m_ioHandoff.get() )
                    {
                        piocomplete->m_ioHandoff.get()( EseException( err ),
                                                        piocomplete->m_ifile,
                                                        (FileQOS)grbitQOS,
                                                        ibOffset,
                                                        ArraySegment<byte>( piocomplete->m_rgbData,
                                                                            piocomplete->m_ibData,
                                                                            piocomplete->m_cbData ) );
                    }
                }
            }
        }
    }
}
