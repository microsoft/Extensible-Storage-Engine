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
                class CComplete
                {
                    public:

                        CComplete(  _In_ BOOL               fWrite,
                                    _In_ array<byte>^       rgbData,
                                    _In_ int                ibData,
                                    _In_ int                cbData,
                                    _In_ ICache::Complete^  complete,
                                    _In_ void*              pvBuffer )
                            :   m_fWrite( fWrite ),
                                m_rgbData( rgbData ),
                                m_ibData( ibData ),
                                m_cbData( cbData ),
                                m_complete( complete ),
                                m_pvBuffer( pvBuffer )
                        {
                        }

                        ~CComplete()
                        {
                            OSMemoryPageFree( m_pvBuffer );
                        }

                        BOOL FWrite() const { return m_fWrite; }

                    private:

                        CComplete( _In_ const CComplete& other );

                    private:

                        BOOL                                            m_fWrite;
                        mutable msclr::gcroot<array<byte> ^>            m_rgbData;
                        int                                             m_ibData;
                        int                                             m_cbData;
                        mutable msclr::auto_gcroot<ICache::Complete^>   m_complete;
                        void*                                           m_pvBuffer;

                    public:

                        static void Complete(   _In_                    const ERR               err,
                                                _In_                    const ::VolumeId        volumeid,
                                                _In_                    const ::FileId          fileid,
                                                _In_                    const ::FileSerial      fileserial,
                                                _In_                    const FullTraceContext& tc,
                                                _In_                    const OSFILEQOS         grbitQOS,
                                                _In_                    const QWORD             ibOffset,
                                                _In_                    const DWORD             cbData,
                                                _In_reads_( cbData )    const BYTE* const       pbData,
                                                _In_                    const DWORD_PTR         keyComplete );
                };
            }
        }
    }
}