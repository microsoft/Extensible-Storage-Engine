// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                ref class ClusterRead : MarshalByRefObject
                {
                    public:

                        ClusterRead(    _In_                const size_t                                cb,
                                        _Out_writes_( cb )  BYTE* const                                 rgb,
                                        _In_                const ::ICachedBlockSlab::PfnClusterRead    pfnClusterRead,
                                        _In_opt_            const DWORD_PTR                             keyClusterRead,
                                        _In_opt_            const ::ICachedBlockSlab::PfnClusterHandoff pfnClusterHandoff,
                                        _In_                MemoryStream^                               buffer )
                            :   cb( cb ),
                                rgb( rgb ),
                                pfnClusterRead( pfnClusterRead ),
                                keyClusterRead( keyClusterRead ),
                                pfnClusterHandoff( pfnClusterHandoff ),
                                buffer( buffer )
                        {
                        }

                        void ClusterRead_( EsentErrorException^ ex )
                        {
                            if ( this->buffer != nullptr && this->buffer->Length > 0 )
                            {
                                array<BYTE>^ bytes = this->buffer->ToArray();
                                pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
                                UtilMemCpy( this->rgb, (const BYTE*)rgbData, bytes->Length );
                            }

                            this->pfnClusterRead(   ex == nullptr ? JET_errSuccess : (ERR)ex->Error,
                                                    this->keyClusterRead );
                        }

                        void ClusterHandoff_()
                        {
                            if ( this->pfnClusterHandoff )
                            {
                                this->pfnClusterHandoff( this->keyClusterRead );
                            }
                        }

                    private:

                        size_t                                      cb;
                        BYTE* const                                 rgb;
                        const ::ICachedBlockSlab::PfnClusterRead    pfnClusterRead;
                        const DWORD_PTR                             keyClusterRead;
                        const ::ICachedBlockSlab::PfnClusterHandoff pfnClusterHandoff;
                        MemoryStream^                               buffer;
                };
            }
        }
    }
}
