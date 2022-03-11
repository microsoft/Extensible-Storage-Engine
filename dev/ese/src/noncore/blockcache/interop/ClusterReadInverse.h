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
                ref class ClusterReadInverse
                {
                    public:

                        ClusterReadInverse( MemoryStream^                       data,
                                            ICachedBlockSlab::ClusterRead^      clusterRead,
                                            ICachedBlockSlab::ClusterHandoff^   clusterHandoff,
                                            void**                              ppvBuffer )
                            :   data( data ),
                                clusterRead( clusterRead ),
                                clusterHandoff( clusterHandoff ),
                                pvBuffer( *ppvBuffer ),
                                clusterReadInverse( gcnew ClusterReadDelegate( this, &ClusterReadInverse::ClusterRead ) ),
                                clusterReadInverseHandle( GCHandle::Alloc( this->clusterReadInverse ) ),
                                clusterHandoffInverse( gcnew ClusterHandoffDelegate( this, &ClusterReadInverse::ClusterHandoff ) ),
                                clusterHandoffInverseHandle( GCHandle::Alloc( this->clusterHandoffInverse ) )
                        {
                            *ppvBuffer = NULL;
                        }

                        ~ClusterReadInverse()
                        {
                            if ( this->clusterReadInverseHandle.IsAllocated )
                            {
                                this->clusterReadInverseHandle.Free();
                            }

                            if ( this->clusterHandoffInverseHandle.IsAllocated )
                            {
                                this->clusterHandoffInverseHandle.Free();
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

                        property ::ICachedBlockSlab::PfnClusterRead PfnClusterRead
                        {
                            ::ICachedBlockSlab::PfnClusterRead get()
                            {
                                if ( this->clusterRead == nullptr )
                                {
                                    return nullptr;
                                }

                                IntPtr pfnClusterRead = Marshal::GetFunctionPointerForDelegate( this->clusterReadInverse );
                                return static_cast<::ICachedBlockSlab::PfnClusterRead>( pfnClusterRead.ToPointer() );
                            }
                        }

                        property DWORD_PTR KeyClusterRead
                        {
                            DWORD_PTR get()
                            {
                                return NULL;
                            }
                        }

                        property ::ICachedBlockSlab::PfnClusterHandoff PfnClusterHandoff
                        {
                            ::ICachedBlockSlab::PfnClusterHandoff get()
                            {
                                if ( this->clusterHandoff == nullptr )
                                {
                                    return nullptr;
                                }

                                IntPtr pfnClusterHandoff = Marshal::GetFunctionPointerForDelegate( this->clusterHandoffInverse );
                                return static_cast<::ICachedBlockSlab::PfnClusterHandoff>( pfnClusterHandoff.ToPointer() );
                            }
                        }

                    private:

                        [UnmanagedFunctionPointer( CallingConvention::Cdecl )]
                        delegate void ClusterReadDelegate(  _In_ const ERR          err,
                                                            _In_ const DWORD_PTR    keyClusterRead );

                        void ClusterRead(   _In_ const ERR          err,
                                            _In_ const DWORD_PTR    keyClusterRead )
                        {
                            try
                            {
                                if ( this->data != nullptr && this->data->Length > 0 )
                                {
                                    array<BYTE>^ bytes = gcnew array<BYTE>( (int)this->data->Length );
                                    pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
                                    UtilMemCpy( (BYTE*)rgbData, pvBuffer, bytes->Length );
                                    this->data->Position = 0;
                                    this->data->Write( bytes, 0, bytes->Length );
                                }

                                this->clusterRead( EseException( err ) );
                            }
                            finally
                            {
                                delete this;
                            }
                        }

                        [UnmanagedFunctionPointer( CallingConvention::Cdecl )]
                        delegate void ClusterHandoffDelegate( _In_ const DWORD_PTR keyClusterRead );

                        void ClusterHandoff( _In_ const DWORD_PTR keyClusterRead )
                        {
                            if ( this->clusterHandoff != nullptr )
                            {
                                this->clusterHandoff();
                            }
                        }

                    private:

                        MemoryStream^                       data;
                        ICachedBlockSlab::ClusterRead^      clusterRead;
                        ICachedBlockSlab::ClusterHandoff^   clusterHandoff;
                        void*                               pvBuffer;
                        ClusterReadDelegate^                clusterReadInverse;
                        GCHandle                            clusterReadInverseHandle;
                        ClusterHandoffDelegate^             clusterHandoffInverse;
                        GCHandle                            clusterHandoffInverseHandle;
                };
            }
        }
    }
}

#pragma pop_macro( "Alloc" )
