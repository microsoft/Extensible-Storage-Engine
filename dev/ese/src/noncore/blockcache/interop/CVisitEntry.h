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
                class CVisitEntry
                {
                    public:

                        CVisitEntry( _In_ IJournal::VisitEntry^ visitentry )
                            :   m_visitentry( visitentry )
                        {
                        }

                        ~CVisitEntry()
                        {
                        }

                    private:

                        CVisitEntry( _In_ const CVisitEntry& other );

                    private:

                        mutable msclr::auto_gcroot<IJournal::VisitEntry^>   m_visitentry;

                    public:

                        static BOOL VisitEntry_(    _In_ const ::JournalPosition    jpos,
                                                    _In_ const CJournalBuffer       jb,
                                                    _In_ const DWORD_PTR            keyVisitEntry )
                        {
                            ::IJournal::PfnVisitEntry pfnVisitEntry = VisitEntry_;
                            Unused( pfnVisitEntry );

                            CVisitEntry* const pvisitentry = (CVisitEntry*)keyVisitEntry;

                            array<byte>^ buffer = jb.Rgb() ? gcnew array<byte>( jb.Cb() ) : nullptr;
                            pin_ptr<byte> rgbData = buffer == nullptr || buffer->Length == 0 ? nullptr : &buffer[ 0 ];
                            UtilMemCpy( (BYTE*)rgbData, jb.Rgb(), jb.Cb() );

                            bool result = pvisitentry->m_visitentry.get()(  (JournalPosition)jpos,
                                                                             ArraySegment<byte>( buffer ) );

                            return result ? fTrue : fFalse;
                        }
                };
            }
        }
    }
}