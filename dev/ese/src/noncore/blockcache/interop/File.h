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
                public ref class File : public FileBase<IFile,IFileAPI,CFileWrapper<IFile,IFileAPI>>
                {
                    public:

                        static ERR ErrWrap( IFile^% f, IFileAPI** const ppfapi )
                        {
                            return Internal::Ese::BlockCache::Interop::ErrWrap<IFile,IFileAPI,CFileWrapper<IFile,IFileAPI>>( f, ppfapi );
                        }

                    public:

                        File( IFile^ f ) : FileBase( f ) { }

                        File( IFileAPI** const ppfapi ) : FileBase( ppfapi ) {}

                        File( IFileAPI* const pfapi ) : FileBase( pfapi ) {}
                };
            }
        }
    }
}
