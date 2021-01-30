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
                class CVisitRegion
                {
                    public:

                        CVisitRegion( _In_ IJournalSegment::VisitRegion^ visitregion )
                            :   m_visitregion( visitregion )
                        {
                        }

                        ~CVisitRegion()
                        {
                        }

                    private:

                        CVisitRegion( _In_ const CVisitRegion& other );

                    private:

                        mutable msclr::auto_gcroot<IJournalSegment::VisitRegion^>   m_visitregion;

                    public:

                        static BOOL VisitRegion_(   _In_ const ::RegionPosition rpos,
                                                    _In_ const CJournalBuffer   jb,
                                                    _In_ const DWORD_PTR        keyVisitRegion )
                        {
                            ::IJournalSegment::PfnVisitRegion pfnVisitRegion = VisitRegion_;
                            Unused( pfnVisitRegion );

                            CVisitRegion* const pvisitregion = (CVisitRegion*)keyVisitRegion;

                            array<byte>^ buffer = jb.Rgb() ? gcnew array<byte>( jb.Cb() ) : nullptr;
                            pin_ptr<byte> rgbData = buffer == nullptr || buffer->Length == 0 ? nullptr : &buffer[ 0 ];
                            UtilMemCpy( (BYTE*)rgbData, jb.Rgb(), jb.Cb() );

                            bool result = pvisitregion->m_visitregion.get()(    (RegionPosition)rpos,
                                                                                ArraySegment<byte>( buffer ) );

                            return result ? fTrue : fFalse;
                        }
                };
            }
        }
    }
}
