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
                ref class VisitEntryInverse
                {
                    public:

                        VisitEntryInverse( IJournal::VisitEntry^ visitEntry )
                            :   visitEntry( visitEntry ),
                                visitEntryInverse( gcnew VisitEntryDelegate( this, &VisitEntryInverse::FVisitEntry ) ),
                                visitEntryInverseHandle( GCHandle::Alloc( this->visitEntryInverse ) )
                        {
                        }

                        ~VisitEntryInverse()
                        {
                            if ( this->visitEntryInverseHandle.IsAllocated )
                            {
                                this->visitEntryInverseHandle.Free();
                            }
                        }

                    internal:

                        property ::IJournal::PfnVisitEntry PfnVisitEntry
                        {
                            ::IJournal::PfnVisitEntry get()
                            {
                                if ( this->visitEntry == nullptr )
                                {
                                    return nullptr;
                                }

                                IntPtr pfnVisitEntry = Marshal::GetFunctionPointerForDelegate( this->visitEntryInverse );
                                return static_cast<::IJournal::PfnVisitEntry>( pfnVisitEntry.ToPointer() );
                            }
                        }

                        property DWORD_PTR KeyVisitEntry
                        {
                            DWORD_PTR get()
                            {
                                return NULL;
                            }
                        }

                    private:

                        [UnmanagedFunctionPointer( CallingConvention::Cdecl )]
                        delegate BOOL VisitEntryDelegate(   _In_ const ::JournalPosition    jpos,
                                                            _In_ const ::JournalPosition    jposEnd,
                                                            _In_ const CJournalBuffer       jb,
                                                            _In_ const DWORD_PTR            keyVisitEntry );

                        BOOL FVisitEntry(   _In_ const ::JournalPosition    jpos,
                                            _In_ const ::JournalPosition    jposEnd,
                                            _In_ const CJournalBuffer       jb,
                                            _In_ const DWORD_PTR            keyVisitEntry )
                        {
                            if ( this->visitEntry == nullptr )
                            {
                                return fFalse;
                            }

                            array<BYTE>^ buffer = jb.Rgb() ? gcnew array<BYTE>( jb.Cb() ) : nullptr;
                            pin_ptr<BYTE> rgbData = ( buffer == nullptr || buffer->Length == 0 ) ? nullptr : &buffer[ 0 ];
                            UtilMemCpy( (BYTE*)rgbData, jb.Rgb(), jb.Cb() );

                            return this->visitEntry(    (JournalPosition)jpos,
                                                        (JournalPosition)jposEnd,
                                                        ArraySegment<BYTE>( buffer ) );
                        }

                    private:

                        IJournal::VisitEntry^   visitEntry;
                        VisitEntryDelegate^     visitEntryInverse;
                        GCHandle                visitEntryInverseHandle;
                };
            }
        }
    }
}

#pragma pop_macro( "Alloc" )
