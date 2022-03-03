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
                ref class ClusterWrittenInverse
                {
                    public:

                        ClusterWrittenInverse(  ICachedBlockSlab::ClusterWritten^   clusterWritten,
                                                ICachedBlockSlab::ClusterHandoff^   clusterHandoff,
                                                void**                              ppvBuffer )
                            :   clusterWritten( clusterWritten ),
                                clusterHandoff( clusterHandoff ),
                                pvBuffer( *ppvBuffer ),
                                clusterWrittenInverse( gcnew ClusterWrittenDelegate( this, &ClusterWrittenInverse::ClusterWritten ) ),
                                clusterWrittenInverseHandle( GCHandle::Alloc( this->clusterWrittenInverse ) ),
                                clusterHandoffInverse( gcnew ClusterHandoffDelegate( this, &ClusterWrittenInverse::ClusterHandoff ) ),
                                clusterHandoffInverseHandle( GCHandle::Alloc( this->clusterHandoffInverse ) )
                        {
                            *ppvBuffer = NULL;
                        }

                        ~ClusterWrittenInverse()
                        {
                            if ( this->clusterWrittenInverseHandle.IsAllocated )
                            {
                                this->clusterWrittenInverseHandle.Free();
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

                        property ::ICachedBlockSlab::PfnClusterWritten PfnClusterWritten
                        {
                            ::ICachedBlockSlab::PfnClusterWritten get()
                            {
                                if ( this->clusterWritten == nullptr )
                                {
                                    return nullptr;
                                }

                                IntPtr pfnClusterWritten = Marshal::GetFunctionPointerForDelegate( this->clusterWrittenInverse );
                                return static_cast<::ICachedBlockSlab::PfnClusterWritten>( pfnClusterWritten.ToPointer() );
                            }
                        }

                        property DWORD_PTR KeyClusterWritten
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
                        delegate void ClusterWrittenDelegate(   _In_ const ERR          err,
                                                                _In_ const DWORD_PTR    keyClusterWritten );

                        void ClusterWritten(   _In_ const ERR          err,
                                            _In_ const DWORD_PTR    keyClusterWritten )
                        {
                            try
                            {
                                this->clusterWritten( EseException( err ) );
                            }
                            finally
                            {
                                delete this;
                            }
                        }

                        [UnmanagedFunctionPointer( CallingConvention::Cdecl )]
                        delegate void ClusterHandoffDelegate( _In_ const DWORD_PTR keyClusterWritten );

                        void ClusterHandoff( _In_ const DWORD_PTR keyClusterWritten )
                        {
                            if ( this->clusterHandoff != nullptr )
                            {
                                this->clusterHandoff();
                            }
                        }

                    private:

                        ICachedBlockSlab::ClusterWritten^   clusterWritten;
                        ICachedBlockSlab::ClusterHandoff^   clusterHandoff;
                        void*                               pvBuffer;
                        ClusterWrittenDelegate^             clusterWrittenInverse;
                        GCHandle                            clusterWrittenInverseHandle;
                        ClusterHandoffDelegate^             clusterHandoffInverse;
                        GCHandle                            clusterHandoffInverseHandle;
                };
            }
        }
    }
}

#pragma pop_macro( "Alloc" )
