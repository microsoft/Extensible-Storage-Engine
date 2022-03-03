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
                ref class VisitSlab : MarshalByRefObject
                {
                    public:

                        VisitSlab(  _In_ const ::ICachedBlockSlabManager::PfnVisitSlab  pfnVisitSlab,
                                    _In_ const DWORD_PTR                                keyVisitSlab )
                            :   pfnVisitSlab( pfnVisitSlab ),
                                keyVisitSlab( keyVisitSlab )
                        {
                        }

                        bool VisitSlab_(
                            UInt64 offsetInBytes,
                            EsentErrorException^ ex,
                            ICachedBlockSlab^ icbs )
                        {
                            ERR                 err     = JET_errSuccess;
                            ::ICachedBlockSlab* pcbs    = NULL;
                            BOOL                fResult = fFalse;

                            if ( !pfnVisitSlab )
                            {
                                return false;
                            }

                            if ( icbs != nullptr )
                            {
                                Call( CachedBlockSlab::ErrWrap( icbs, &pcbs ) );
                            }

                            fResult = pfnVisitSlab( offsetInBytes,
                                                    ex == nullptr ? JET_errSuccess : (ERR)ex->Error,
                                                    pcbs,
                                                    keyVisitSlab );

                        HandleError:
                            delete pcbs;
                            if ( err == JET_errOutOfMemory )
                            {
                                throw gcnew OutOfMemoryException();
                            }
                            return fResult;
                        }

                    private:

                        const ::ICachedBlockSlabManager::PfnVisitSlab   pfnVisitSlab;
                        const DWORD_PTR                                 keyVisitSlab;
                };
            }
        }
    }
}
