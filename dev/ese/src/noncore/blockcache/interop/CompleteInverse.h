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
                ref class CompleteInverse
                {
                    public:

                        CompleteInverse(    bool                isWrite,
                                            MemoryStream^       data,
                                            ICache::Complete^   complete,
                                            void**              ppvBuffer )
                            :   isWrite( isWrite ),
                                data( data ),
                                complete( complete ),
                                pvBuffer( *ppvBuffer ),
                                completeInverse( gcnew CompleteDelegate( this, &CompleteInverse::Complete ) ),
                                completeInverseHandle( GCHandle::Alloc( this->completeInverse ) )
                        {
                            *ppvBuffer = NULL;
                        }

                        ~CompleteInverse()
                        {
                            if ( this->completeInverseHandle.IsAllocated )
                            {
                                this->completeInverseHandle.Free();
                            }

                            OSMemoryPageFree( this->pvBuffer );
                            this->pvBuffer = NULL;
                        }

                    internal:

                        property void* PvBuffer
                        {
                            void* get()
                            {
                                return this->pvBuffer;
                            }
                        }

                        property ::ICache::PfnComplete PfnComplete
                        {
                            ::ICache::PfnComplete get()
                            {
                                IntPtr pfnComplete = Marshal::GetFunctionPointerForDelegate( this->completeInverse );
                                return static_cast<::ICache::PfnComplete>( pfnComplete.ToPointer() );
                            }
                        }

                        property DWORD_PTR KeyComplete
                        {
                            DWORD_PTR get()
                            {
                                return NULL;
                            }
                        }

                    private:

                        [UnmanagedFunctionPointer( CallingConvention::Cdecl )]
                        delegate void CompleteDelegate( _In_                    const ERR               err,
                                                        _In_                    const ::VolumeId        volumeid,
                                                        _In_                    const ::FileId          fileid,
                                                        _In_                    const ::FileSerial      fileserial,
                                                        _In_                    const FullTraceContext& tc,
                                                        _In_                    const OSFILEQOS         grbitQOS,
                                                        _In_                    const QWORD             ibOffset,
                                                        _In_                    const DWORD             cbData,
                                                        _In_reads_( cbData )    const BYTE* const       pbData,
                                                        _In_                    const DWORD_PTR         keyComplete );

                        void Complete(  _In_                    const ERR               err,
                                        _In_                    const ::VolumeId        volumeid,
                                        _In_                    const ::FileId          fileid,
                                        _In_                    const ::FileSerial      fileserial,
                                        _In_                    const FullTraceContext& tc,
                                        _In_                    const OSFILEQOS         grbitQOS,
                                        _In_                    const QWORD             ibOffset,
                                        _In_                    const DWORD             cbData,
                                        _In_reads_( cbData )    const BYTE* const       pbData,
                                        _In_                    const DWORD_PTR         keyComplete )
                        {
                            try
                            {
                                if ( this->isWrite == false && this->data != nullptr && cbData )
                                {
                                    array<BYTE>^ bytes = gcnew array<BYTE>( cbData );
                                    pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
                                    UtilMemCpy( (BYTE*)rgbData, pbData, bytes->Length );
                                    this->data->Position = 0;
                                    this->data->Write( bytes, 0, bytes->Length );
                                }

                                this->complete( EseException( err ),
                                                (VolumeId)volumeid,
                                                (FileId)fileid,
                                                (FileSerial)fileserial,
                                                (FileQOS)grbitQOS,
                                                ibOffset,
                                                this->data );
                            }
                            finally
                            {
                                delete this;
                            }
                        }

                    private:

                        bool                isWrite;
                        MemoryStream^       data;
                        ICache::Complete^   complete;
                        void*               pvBuffer;
                        CompleteDelegate^   completeInverse;
                        GCHandle            completeInverseHandle;
                };
            }
        }
    }
}

#pragma pop_macro( "Alloc" )
