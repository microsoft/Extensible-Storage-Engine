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
                ref class VisitSlotInverse
                {
                    public:

                        VisitSlotInverse( ICachedBlockSlab::VisitSlot^ visitSlot )
                            :   visitSlot( visitSlot ),
                                visitSlotInverse( gcnew VisitSlotDelegate( this, &VisitSlotInverse::FVisitSlot ) ),
                                visitSlotInverseHandle( GCHandle::Alloc( this->visitSlotInverse ) )
                        {
                        }

                        ~VisitSlotInverse()
                        {
                            if ( this->visitSlotInverseHandle.IsAllocated )
                            {
                                this->visitSlotInverseHandle.Free();
                            }
                        }

                    internal:

                        property ::ICachedBlockSlab::PfnVisitSlot PfnVisitSlot
                        {
                            ::ICachedBlockSlab::PfnVisitSlot get()
                            {
                                if ( this->visitSlot == nullptr )
                                {
                                    return nullptr;
                                }

                                IntPtr pfnVisitSlot = Marshal::GetFunctionPointerForDelegate( this->visitSlotInverse );
                                return static_cast<::ICachedBlockSlab::PfnVisitSlot>( pfnVisitSlot.ToPointer() );
                            }
                        }

                        property DWORD_PTR KeyVisitSlot
                        {
                            DWORD_PTR get()
                            {
                                return NULL;
                            }
                        }

                    private:

                        [UnmanagedFunctionPointer( CallingConvention::Cdecl )]
                        delegate BOOL VisitSlotDelegate(    _In_ const ERR                      err,
                                                            _In_ const ::CCachedBlockSlotState& slotstAccepted,
                                                            _In_ const ::CCachedBlockSlotState& slotstCurrent,
                                                            _In_ const DWORD_PTR                keyVisitSlot );

                        BOOL FVisitSlot(    _In_ const ERR                      err,
                                            _In_ const ::CCachedBlockSlotState& slotstAccepted,
                                            _In_ const ::CCachedBlockSlotState& slotstCurrent,
                                            _In_ const DWORD_PTR                keyVisitSlot )
                        {
                            if ( this->visitSlot == nullptr )
                            {
                                return fFalse;
                            }
                            
                            return this->visitSlot(
                                EseException( err ),
                                gcnew CachedBlockSlotState( &slotstAccepted ),
                                gcnew CachedBlockSlotState( &slotstCurrent ) );
                        }

                    private:

                        ICachedBlockSlab::VisitSlot^    visitSlot;
                        VisitSlotDelegate^              visitSlotInverse;
                        GCHandle                        visitSlotInverseHandle;
                };
            }
        }
    }
}

#pragma pop_macro( "Alloc" )
