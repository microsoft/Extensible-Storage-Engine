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
                class CSealed
                {
                    public:

                        CSealed( _In_ IJournalSegment::Sealed^ sealed )
                            :   m_sealed( sealed )
                        {
                        }

                        ~CSealed()
                        {
                        }

                    private:

                        CSealed( _In_ const CSealed& other );

                    private:

                        mutable msclr::auto_gcroot<IJournalSegment::Sealed^>   m_sealed;

                    public:

                        static void Sealed_(    _In_ const ERR                  err,
                                                _In_ const ::SegmentPosition    spos,
                                                _In_ const DWORD_PTR            keySealed )
                        {
                            ::IJournalSegment::PfnSealed pfnSealed = Sealed_;
                            Unused( pfnSealed );

                            CSealed* const psealed = (CSealed*)keySealed;

                            try
                            {
                                psealed->m_sealed.get()(    EseException( err ),
                                                            (SegmentPosition)spos );
                            }
                            finally
                            {
                                delete psealed;
                            }
                        }
                };
            }
        }
    }
}