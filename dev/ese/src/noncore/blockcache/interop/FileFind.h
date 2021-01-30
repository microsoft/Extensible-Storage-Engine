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
                public ref class FileFind : public FileFindBase<IFileFind,IFileFindAPI,CFileFindWrapper<IFileFind,IFileFindAPI>>
                {
                    public:

                        static ERR ErrWrap( IFileFind^% ff, IFileFindAPI** const ppffapi )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<IFileFind,IFileFindAPI,CFileFindWrapper<IFileFind,IFileFindAPI>>( ff, ppffapi );
                        }

                    public:

                        FileFind( IFileFind^ ff ) : FileFindBase( ff ) { }

                        FileFind( IFileFindAPI** const ppffapi ) : FileFindBase( ppffapi ) {}
                };
            }
        }
    }
}
