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
                ref class ConsiderUpdateInverse
                {
                    public:

                        ConsiderUpdateInverse( ICachedBlockSlab::ConsiderUpdate^ considerUpdate )
                            :   considerUpdate( considerUpdate ),
                                considerUpdateInverse( gcnew ConsiderUpdateDelegate( this, &ConsiderUpdateInverse::FConsiderUpdate ) ),
                                considerUpdateInverseHandle( GCHandle::Alloc( this->considerUpdateInverse ) )
                        {
                        }

                        ~ConsiderUpdateInverse()
                        {
                            if ( this->considerUpdateInverseHandle.IsAllocated )
                            {
                                this->considerUpdateInverseHandle.Free();
                            }
                        }

                    internal:

                        property ::ICachedBlockSlab::PfnConsiderUpdate PfnConsiderUpdate
                        {
                            ::ICachedBlockSlab::PfnConsiderUpdate get()
                            {
                                if ( this->considerUpdate == nullptr )
                                {
                                    return nullptr;
                                }

                                IntPtr pfnConsiderUpdate = Marshal::GetFunctionPointerForDelegate( this->considerUpdateInverse );
                                return static_cast<::ICachedBlockSlab::PfnConsiderUpdate>( pfnConsiderUpdate.ToPointer() );
                            }
                        }

                        property DWORD_PTR KeyConsiderUpdate
                        {
                            DWORD_PTR get()
                            {
                                return NULL;
                            }
                        }

                    private:

                        [UnmanagedFunctionPointer( CallingConvention::Cdecl )]
                        delegate BOOL ConsiderUpdateDelegate(   _In_ const ::CCachedBlockSlotState& slotstCurrent,
                                                                _In_ const ::CCachedBlockSlot&      slotNew,
                                                                _In_ const DWORD_PTR                keyConsiderUpdate );

                        BOOL FConsiderUpdate(   _In_ const ::CCachedBlockSlotState& slotstCurrent,
                                                _In_ const ::CCachedBlockSlot&      slotNew,
                                                _In_ const DWORD_PTR                keyConsiderUpdate )
                        {
                            if ( this->considerUpdate == nullptr )
                            {
                                return fFalse;
                            }
                            
                            return this->considerUpdate(    gcnew CachedBlockSlotState( &slotstCurrent ),
                                                            gcnew CachedBlockSlot( &slotNew ) );
                        }

                    private:

                        ICachedBlockSlab::ConsiderUpdate^   considerUpdate;
                        ConsiderUpdateDelegate^             considerUpdateInverse;
                        GCHandle                            considerUpdateInverseHandle;
                };
            }
        }
    }
}

#pragma pop_macro( "Alloc" )
