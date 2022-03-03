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
                ref class FileFindRemotable : MarshalByRefObject, IFileFind
                {
                    public:

                        FileFindRemotable( IFileFind^ target )
                            :   target( target )
                        {
                        }

                        ~FileFindRemotable() {}

                    public:

                        virtual bool Next()
                        {
                            return this->target->Next();
                        }

                        virtual bool IsFolder()
                        {
                            return this->target->IsFolder();
                        }

                        virtual String^ Path()
                        {
                            return this->target->Path();
                        }

                        virtual Int64 Size( FileSize fileSize )
                        {
                            return this->target->Size( fileSize );
                        }

                        virtual bool IsReadOnly()
                        {
                            return this->target->IsReadOnly();
                        }

                    private:

                        IFileFind^ target;
                };
            }
        }
    }
}