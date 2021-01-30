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
                ref class Sealed
                {
                    public:

                        Sealed( const ::IJournalSegment::PfnSealed  pfnSealed,
                                const DWORD_PTR                     keySealed )
                            :   pfnSealed( pfnSealed ),
                                keySealed( keySealed )
                        {
                        }

                        void Sealed_(
                            EsentErrorException^ ex,
                            SegmentPosition segmentPosition )
                        {
                            pfnSealed(  ex == nullptr ? JET_errSuccess : (ERR)ex->Error,
                                        (::SegmentPosition)segmentPosition,
                                        keySealed );
                        }

                    private:

                        const ::IJournalSegment::PfnSealed  pfnSealed;
                        const DWORD_PTR                     keySealed;
                };
            }
        }
    }
}
