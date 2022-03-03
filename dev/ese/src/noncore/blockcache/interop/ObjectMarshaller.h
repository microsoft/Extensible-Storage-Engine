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
                ref class ObjectMarshaller : public MarshalByRefObject, IObjectMarshaller
                {
                    public:

                        ObjectMarshaller()
                        {
                        }

                        ~ObjectMarshaller()
                        {
                            this->!ObjectMarshaller();
                        }

                        virtual Object^ Get( IntPtr id );

                    protected:

                        !ObjectMarshaller()
                        {
                        }
                };
            }
        }
    }
}