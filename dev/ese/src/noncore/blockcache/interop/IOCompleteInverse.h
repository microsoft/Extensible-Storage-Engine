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
                ref class IOCompleteInverse
                {
                    public:

                        IOCompleteInverse(  IFile^              ifile,
                                            bool                isWrite,
                                            MemoryStream^       data,
                                            IFile::IOComplete^  ioComplete,
                                            IFile::IOHandoff^   ioHandoff,
                                            void**              ppvBuffer )
                            :   ifile( ifile ),
                                isWrite( isWrite ),
                                data( data ),
                                ioComplete( ioComplete ),
                                ioHandoff( ioHandoff ),
                                pvBuffer( *ppvBuffer ),
                                ioCompleteInverse( gcnew IOCompleteDelegate( this, &IOCompleteInverse::IOComplete ) ),
                                ioCompleteInverseHandle( GCHandle::Alloc( this->ioCompleteInverse ) ),
                                ioHandoffInverse( gcnew IOHandoffDelegate( this, &IOCompleteInverse::IOHandoff ) ),
                                ioHandoffInverseHandle( GCHandle::Alloc( this->ioHandoffInverse ) )
                        {
                            *ppvBuffer = NULL;
                        }

                        ~IOCompleteInverse()
                        {
                            if ( this->ioCompleteInverseHandle.IsAllocated )
                            {
                                this->ioCompleteInverseHandle.Free();
                            }

                            if ( this->ioHandoffInverseHandle.IsAllocated )
                            {
                                this->ioHandoffInverseHandle.Free();
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

                        property ::IFileAPI::PfnIOComplete PfnIOComplete
                        {
                            ::IFileAPI::PfnIOComplete get()
                            {
                                if ( this->ioComplete == nullptr )
                                {
                                    return nullptr;
                                }

                                IntPtr pfnIOComplete = Marshal::GetFunctionPointerForDelegate( this->ioCompleteInverse );
                                return static_cast<::IFileAPI::PfnIOComplete>( pfnIOComplete.ToPointer() );
                            }
                        }

                        property DWORD_PTR KeyIOComplete
                        {
                            DWORD_PTR get()
                            {
                                return NULL;
                            }
                        }

                        property ::IFileAPI::PfnIOHandoff PfnIOHandoff
                        {
                            ::IFileAPI::PfnIOHandoff get()
                            {
                                if ( this->ioHandoff == nullptr )
                                {
                                    return nullptr;
                                }

                                IntPtr pfnIOHandoff = Marshal::GetFunctionPointerForDelegate( this->ioHandoffInverse );
                                return static_cast<::IFileAPI::PfnIOHandoff>( pfnIOHandoff.ToPointer() );
                            }
                        }

                    private:

                        [UnmanagedFunctionPointer( CallingConvention::Cdecl )]
                        delegate void IOCompleteDelegate(   _In_                    const ERR               err,
                                                            _In_                    IFileAPI* const         pfapi,
                                                            _In_                    const FullTraceContext& tc,
                                                            _In_                    const OSFILEQOS         grbitQOS,
                                                            _In_                    const QWORD             ibOffset,
                                                            _In_                    const DWORD             cbData,
                                                            _In_reads_( cbData )    const BYTE* const       pbData,
                                                            _In_                    const DWORD_PTR         keyIOComplete );

                        void IOComplete(    _In_                    const ERR               err,
                                            _In_                    IFileAPI* const         pfapi,
                                            _In_                    const FullTraceContext& tc,
                                            _In_                    const OSFILEQOS         grbitQOS,
                                            _In_                    const QWORD             ibOffset,
                                            _In_                    const DWORD             cbData,
                                            _In_reads_( cbData )    const BYTE* const       pbData,
                                            _In_                    const DWORD_PTR         keyIOComplete )
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

                                this->ioComplete(   EseException( err ),
                                                    this->ifile,
                                                    (FileQOS)grbitQOS,
                                                    ibOffset,
                                                    this->data );
                            }
                            finally
                            {
                                delete this;
                            }
                        }

                        [UnmanagedFunctionPointer( CallingConvention::Cdecl )]
                        delegate void IOHandoffDelegate(    _In_                    const ERR               err,
                                                            _In_                    IFileAPI* const         pfapi,
                                                            _In_                    const FullTraceContext& tc,
                                                            _In_                    const OSFILEQOS         grbitQOS,
                                                            _In_                    const QWORD             ibOffset,
                                                            _In_                    const DWORD             cbData,
                                                            _In_reads_( cbData )    const BYTE* const       pbData,
                                                            _In_                    const DWORD_PTR         keyIOComplete,
                                                            _In_                    void* const             pvIOContext );

                        void IOHandoff( _In_                    const ERR               err,
                                        _In_                    IFileAPI* const         pfapi,
                                        _In_                    const FullTraceContext& tc,
                                        _In_                    const OSFILEQOS         grbitQOS,
                                        _In_                    const QWORD             ibOffset,
                                        _In_                    const DWORD             cbData,
                                        _In_reads_( cbData )    const BYTE* const       pbData,
                                        _In_                    const DWORD_PTR         keyIOComplete,
                                        _In_                    void* const             pvIOContext )
                        {
                            this->ioHandoff(    EseException( err ),
                                                this->ifile,
                                                (FileQOS)grbitQOS,
                                                ibOffset,
                                                this->data );
                        }

                    private:

                        IFile^              ifile;
                        bool                isWrite;
                        MemoryStream^       data;
                        IFile::IOComplete^  ioComplete;
                        IFile::IOHandoff^   ioHandoff;
                        void*               pvBuffer;
                        IOCompleteDelegate^ ioCompleteInverse;
                        GCHandle            ioCompleteInverseHandle;
                        IOHandoffDelegate^  ioHandoffInverse;
                        GCHandle            ioHandoffInverseHandle;
                };
            }
        }
    }
}

#pragma pop_macro( "Alloc" )
