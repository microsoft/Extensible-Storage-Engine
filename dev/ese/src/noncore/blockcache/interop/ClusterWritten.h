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
                ref class ClusterWritten : Remotable
                {
                    public:

                        ClusterWritten( _In_     const ::ICachedBlockSlab::PfnClusterWritten    pfnClusterWritten,
                                        _In_opt_ const DWORD_PTR                                keyClusterWritten,
                                        _In_opt_ const ::ICachedBlockSlab::PfnClusterHandoff    pfnClusterHandoff )
                            :   pfnClusterWritten( pfnClusterWritten ),
                                keyClusterWritten( keyClusterWritten ),
                                pfnClusterHandoff( pfnClusterHandoff )
                        {
                        }

                        void ClusterWritten_( EsentErrorException^ ex )
                        {
                            this->pfnClusterWritten(    ex == nullptr ? JET_errSuccess : (ERR)ex->Error, 
                                                        this->keyClusterWritten );
                        }

                        void ClusterHandoff_()
                        {
                            if ( this->pfnClusterHandoff )
                            {
                                this->pfnClusterHandoff( this->keyClusterWritten );
                            }
                        }

                    private:

                        const ::ICachedBlockSlab::PfnClusterWritten pfnClusterWritten;
                        const DWORD_PTR                             keyClusterWritten;
                        const ::ICachedBlockSlab::PfnClusterHandoff pfnClusterHandoff;
                };
            }
        }
    }
}
