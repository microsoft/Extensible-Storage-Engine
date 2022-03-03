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
                ref class SlabSavedInverse
                {
                    public:

                        SlabSavedInverse( ICachedBlockSlab::SlabSaved^ slabSaved )
                            :   slabSaved( slabSaved ),
                                slabSavedInverse( gcnew SlabSavedDelegate( this, &SlabSavedInverse::SlabSaved ) ),
                                slabSavedInverseHandle( GCHandle::Alloc( this->slabSavedInverse ) )
                        {
                        }

                        ~SlabSavedInverse()
                        {
                            if ( this->slabSavedInverseHandle.IsAllocated )
                            {
                                this->slabSavedInverseHandle.Free();
                            }
                        }

                    internal:

                        property ::ICachedBlockSlab::PfnSlabSaved PfnSlabSaved
                        {
                            ::ICachedBlockSlab::PfnSlabSaved get()
                            {
                                if ( this->slabSaved == nullptr )
                                {
                                    return nullptr;
                                }

                                IntPtr pfnSlabSaved = Marshal::GetFunctionPointerForDelegate( this->slabSavedInverse );
                                return static_cast<::ICachedBlockSlab::PfnSlabSaved>( pfnSlabSaved.ToPointer() );
                            }
                        }

                        property DWORD_PTR KeySlabSaved
                        {
                            DWORD_PTR get()
                            {
                                return NULL;
                            }
                        }

                    private:

                        [UnmanagedFunctionPointer( CallingConvention::Cdecl )]
                        delegate void SlabSavedDelegate(    _In_ const ERR          err,
                                                            _In_ const DWORD_PTR    keySlabSaved );

                        void SlabSaved( _In_ const ERR          err,
                                        _In_ const DWORD_PTR    keySlabSaved )
                        {
                            try
                            {
                                this->slabSaved( EseException( err ) );
                            }
                            finally
                            {
                                delete this;
                            }
                        }

                    private:

                        ICachedBlockSlab::SlabSaved^    slabSaved;
                        SlabSavedDelegate^              slabSavedInverse;
                        GCHandle                        slabSavedInverseHandle;
                };
            }
        }
    }
}

#pragma pop_macro( "Alloc" )
