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
                ref class SealedInverse
                {
                    public:

                        SealedInverse( IJournalSegment::Sealed^ sealed )
                            :   sealed( sealed ),
                                sealedInverse( gcnew SealedDelegate( this, &SealedInverse::Sealed ) ),
                                sealedInverseHandle( GCHandle::Alloc( this->sealedInverse ) )
                        {
                        }

                        ~SealedInverse()
                        {
                            if ( this->sealedInverseHandle.IsAllocated )
                            {
                                this->sealedInverseHandle.Free();
                            }
                        }

                    internal:

                        property ::IJournalSegment::PfnSealed PfnSealed
                        {
                            ::IJournalSegment::PfnSealed get()
                            {
                                if ( this->sealed == nullptr )
                                {
                                    return nullptr;
                                }

                                IntPtr pfnSealed = Marshal::GetFunctionPointerForDelegate( this->sealedInverse );
                                return static_cast<::IJournalSegment::PfnSealed>( pfnSealed.ToPointer() );
                            }
                        }

                        property DWORD_PTR KeySealed
                        {
                            DWORD_PTR get()
                            {
                                return NULL;
                            }
                        }

                    private:

                        [UnmanagedFunctionPointer( CallingConvention::Cdecl )]
                        delegate void SealedDelegate(   _In_ const ERR                  err,
                                                        _In_ const ::SegmentPosition    spos,
                                                        _In_ const DWORD_PTR            keySealed );

                        void Sealed(    _In_ const ERR                  err,
                                        _In_ const ::SegmentPosition    spos,
                                        _In_ const DWORD_PTR            keySealed )
                        {
                            try
                            {
                                this->sealed( EseException( err ), (SegmentPosition)spos );
                            }
                            finally
                            {
                                delete this;
                            }
                        }

                    private:

                        IJournalSegment::Sealed^    sealed;
                        SealedDelegate^             sealedInverse;
                        GCHandle                    sealedInverseHandle;
                };
            }
        }
    }
}

#pragma pop_macro( "Alloc" )
