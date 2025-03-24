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
                class CIOComplete
                {
                    public:

                        CIOComplete(    BOOL                fIsHeapAlloc,
                                        IFile^              ifile,
                                        BOOL                fWrite,
                                        array<byte>^        rgbData,
                                        int                 ibData,
                                        int                 cbData,
                                        IFile::IOComplete^  ioComplete,
                                        IFile::IOHandoff^   ioHandoff,
                                        void*               pvBuffer )
                            :   m_fIsHeapAlloc( fIsHeapAlloc ),
                                m_ifile( ifile ),
                                m_fWrite( fWrite ),
                                m_rgbData( rgbData ),
                                m_ibData( ibData ),
                                m_cbData( cbData ),
                                m_ioComplete( ioComplete ),
                                m_ioHandoff( ioHandoff ),
                                m_pvBuffer( pvBuffer )
                        {
                        }

                        BOOL FWrite() const { return m_fWrite; }

                    protected:

                        virtual ~CIOComplete()
                        {
                            OSMemoryPageFree( m_pvBuffer );
                        }

                    public:

                        VOID Release()
                        {
                            if ( m_fIsHeapAlloc )
                            {
                                delete this;
                            }
                            else
                            {
                                this->~CIOComplete();
                            }
                        }

                    private:

                        CIOComplete( const CIOComplete& other );

                    private:

                        BOOL                                            m_fIsHeapAlloc;
                        mutable msclr::gcroot<IFile^>                   m_ifile;
                        BOOL                                            m_fWrite;
                        mutable msclr::gcroot<array<byte> ^>            m_rgbData;
                        int                                             m_ibData;
                        int                                             m_cbData;
                        mutable msclr::auto_gcroot<IFile::IOComplete^>  m_ioComplete;
                        mutable msclr::auto_gcroot<IFile::IOHandoff^>   m_ioHandoff;
                        void*                                           m_pvBuffer;

                    public:

                        static void Complete(   _In_                    const ERR               err,
                                                _In_                    IFileAPI* const         pfapi,
                                                _In_                    const FullTraceContext& tc,
                                                _In_                    const OSFILEQOS         grbitQOS,
                                                _In_                    const QWORD             ibOffset,
                                                _In_                    const DWORD             cbData,
                                                _In_reads_( cbData )    const BYTE* const       pbData,
                                                _In_                    const DWORD_PTR         keyIOComplete );

                        static void Handoff(    _In_                    const ERR               err,
                                                _In_                    IFileAPI* const         pfapi,
                                                _In_                    const FullTraceContext& tc,
                                                _In_                    const OSFILEQOS         grbitQOS,
                                                _In_                    const QWORD             ibOffset,
                                                _In_                    const DWORD             cbData,
                                                _In_reads_( cbData )    const BYTE* const       pbData,
                                                _In_                    const DWORD_PTR         keyIOComplete,
                                                _In_                    void* const             pvIOContext );
                };
            }
        }
    }
}