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
                ref class VisitSegmentInverse
                {
                    public:

                        VisitSegmentInverse( IJournalSegmentManager::VisitSegment^ visitSegment )
                            :   visitSegment( visitSegment ),
                                visitSegmentInverse( gcnew VisitSegmentDelegate( this, &VisitSegmentInverse::FVisitSegment ) ),
                                visitSegmentInverseHandle( GCHandle::Alloc( this->visitSegmentInverse ) )
                        {
                        }

                        ~VisitSegmentInverse()
                        {
                            if ( this->visitSegmentInverseHandle.IsAllocated )
                            {
                                this->visitSegmentInverseHandle.Free();
                            }
                        }

                    internal:

                        property ::IJournalSegmentManager::PfnVisitSegment PfnVisitSegment
                        {
                            ::IJournalSegmentManager::PfnVisitSegment get()
                            {
                                if ( this->visitSegment == nullptr )
                                {
                                    return nullptr;
                                }

                                IntPtr pfnVisitSegment = Marshal::GetFunctionPointerForDelegate( this->visitSegmentInverse );
                                return static_cast<::IJournalSegmentManager::PfnVisitSegment>( pfnVisitSegment.ToPointer() );
                            }
                        }

                        property DWORD_PTR KeyVisitSegment
                        {
                            DWORD_PTR get()
                            {
                                return NULL;
                            }
                        }

                    private:

                        [UnmanagedFunctionPointer( CallingConvention::Cdecl )]
                        delegate BOOL VisitSegmentDelegate( _In_ const ::SegmentPosition    spos,
                                                            _In_ const ERR                  err,
                                                            _In_ ::IJournalSegment* const   pjs,
                                                            _In_ const DWORD_PTR            keyVisitSegment );

                        BOOL FVisitSegment( _In_ const ::SegmentPosition    spos,
                                            _In_ const ERR                  err,
                                            _In_ ::IJournalSegment* const   pjs,
                                            _In_ const DWORD_PTR            keyVisitSegment )
                        {
                            if ( this->visitSegment == nullptr )
                            {
                                return fFalse;
                            }

                            return this->visitSegment(  (SegmentPosition)spos,
                                                        EseException( err ),
                                                        pjs ? gcnew JournalSegment( pjs ) : nullptr );
                        }

                    private:

                        IJournalSegmentManager::VisitSegment^   visitSegment;
                        VisitSegmentDelegate^                   visitSegmentInverse;
                        GCHandle                                visitSegmentInverseHandle;
                };
            }
        }
    }
}

#pragma pop_macro( "Alloc" )
