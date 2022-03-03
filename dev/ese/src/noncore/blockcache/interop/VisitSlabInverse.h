// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

#pragma push_macro( "Alloc" )
#undef Alloc

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                ref class VisitSlabInverse
                {
                    public:

                        VisitSlabInverse( ICachedBlockSlabManager::VisitSlab^ visitSlab )
                            :   visitSlab( visitSlab ),
                                visitSlabInverse( gcnew VisitSlabDelegate( this, &VisitSlabInverse::FVisitSlab ) ),
                                visitSlabInverseHandle( GCHandle::Alloc( this->visitSlabInverse ) )
                        {
                        }

                        ~VisitSlabInverse()
                        {
                            if ( this->visitSlabInverseHandle.IsAllocated )
                            {
                                this->visitSlabInverseHandle.Free();
                            }
                        }

                    internal:

                        property ::ICachedBlockSlabManager::PfnVisitSlab PfnVisitSlab
                        {
                            ::ICachedBlockSlabManager::PfnVisitSlab get()
                            {
                                if ( this->visitSlab == nullptr )
                                {
                                    return nullptr;
                                }

                                IntPtr pfnVisitSlab = Marshal::GetFunctionPointerForDelegate( this->visitSlabInverse );
                                return static_cast<::ICachedBlockSlabManager::PfnVisitSlab>( pfnVisitSlab.ToPointer() );
                            }
                        }

                        property DWORD_PTR KeyVisitSlab
                        {
                            DWORD_PTR get()
                            {
                                return NULL;
                            }
                        }

                    private:

                        [UnmanagedFunctionPointer( CallingConvention::Cdecl )]
                        delegate BOOL VisitSlabDelegate(    _In_ const QWORD                ib,
                                                            _In_ const ERR                  err,
                                                            _In_ ::ICachedBlockSlab* const  pcbs,
                                                            _In_ const DWORD_PTR            keyVisitSlab );

                        BOOL FVisitSlab(    _In_ const QWORD                ib,
                                            _In_ const ERR                  err,
                                            _In_ ::ICachedBlockSlab* const  pcbs,
                                            _In_ const DWORD_PTR            keyVisitSlab )
                        {
                            if ( this->visitSlab == nullptr )
                            {
                                return fFalse;
                            }
                            
                            return this->visitSlab( ib,
                                                    EseException( err ),
                                                    pcbs ? gcnew CachedBlockSlab( pcbs ) : nullptr );
                        }

                    private:

                        ICachedBlockSlabManager::VisitSlab^ visitSlab;
                        VisitSlabDelegate^                  visitSlabInverse;
                        GCHandle                            visitSlabInverseHandle;
                };
            }
        }
    }
}

#pragma pop_macro( "Alloc" )
