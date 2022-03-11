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
                ref class VisitRegionInverse
                {
                    public:

                        VisitRegionInverse( IJournalSegment::VisitRegion^ visitRegion )
                            :   visitRegion( visitRegion ),
                                visitRegionInverse( gcnew VisitRegionDelegate( this, &VisitRegionInverse::FVisitRegion ) ),
                                visitRegionInverseHandle( GCHandle::Alloc( this->visitRegionInverse ) )
                        {
                        }

                        ~VisitRegionInverse()
                        {
                            if ( this->visitRegionInverseHandle.IsAllocated )
                            {
                                this->visitRegionInverseHandle.Free();
                            }
                        }

                    internal:

                        property ::IJournalSegment::PfnVisitRegion PfnVisitRegion
                        {
                            ::IJournalSegment::PfnVisitRegion get()
                            {
                                if ( this->visitRegion == nullptr )
                                {
                                    return nullptr;
                                }

                                IntPtr pfnVisitRegion = Marshal::GetFunctionPointerForDelegate( this->visitRegionInverse );
                                return static_cast<::IJournalSegment::PfnVisitRegion>( pfnVisitRegion.ToPointer() );
                            }
                        }

                        property DWORD_PTR KeyVisitRegion
                        {
                            DWORD_PTR get()
                            {
                                return NULL;
                            }
                        }

                    private:

                        [UnmanagedFunctionPointer( CallingConvention::Cdecl )]
                        delegate BOOL VisitRegionDelegate(  _In_ const ::RegionPosition rpos,
                                                            _In_ const ::RegionPosition rposEnd,
                                                            _In_ const CJournalBuffer   jb,
                                                            _In_ const DWORD_PTR        keyVisitRegion );

                        BOOL FVisitRegion(  _In_ const ::RegionPosition rpos,
                                            _In_ const ::RegionPosition rposEnd,
                                            _In_ const CJournalBuffer   jb,
                                            _In_ const DWORD_PTR        keyVisitRegion )
                        {
                            if ( this->visitRegion == nullptr )
                            {
                                return fFalse;
                            }

                            array<BYTE>^ buffer = jb.Rgb() ? gcnew array<BYTE>( jb.Cb() ) : nullptr;
                            pin_ptr<BYTE> rgbData = ( buffer == nullptr || buffer->Length == 0 ) ? nullptr : &buffer[ 0 ];
                            UtilMemCpy( (BYTE*)rgbData, jb.Rgb(), jb.Cb() );

                            return this->visitRegion( (RegionPosition)rpos, (RegionPosition)rposEnd, ArraySegment<BYTE>( buffer ) );
                        }

                    private:

                        IJournalSegment::VisitRegion^   visitRegion;
                        VisitRegionDelegate^            visitRegionInverse;
                        GCHandle                        visitRegionInverseHandle;
                };
            }
        }
    }
}

#pragma pop_macro( "Alloc" )
